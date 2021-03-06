/**
 * Starts a replica set with arbiter, builds an index in background,
 * run through drop indexes, drop collection, drop database. 
 */

var checkOp = function(checkDB) {
    var curOp = checkDB.currentOp(true);
    for (var i=0; i < curOp.inprog.length; i++) {
        try {
            if (curOp.inprog[i].query.background){
                printjson(curOp.inprog[i].msg);
                return true; 
            }
        } catch (e) {
            // catchem if you can
        }
    }
    return false;
}

var dbname = 'bgIndexSec';
var collection = 'jstests_feh';
var size = 100000;

// Set up replica set
var replTest = new ReplSetTest({ name: 'bgIndex', nodes: 3 });
var nodes = replTest.nodeList();

// We need an arbiter to ensure that the primary doesn't step down when we restart the secondary
replTest.startSet();
replTest.initiate({"_id" : "bgIndex",
                   "members" : [
                       {"_id" : 0, "host" : nodes[0]},
                       {"_id" : 1, "host" : nodes[1]},
                       {"_id" : 2, "host" : nodes[2], "arbiterOnly" : true}]});

var master = replTest.getMaster();
var second = replTest.getSecondary();

var masterDB = master.getDB(dbname);
var secondDB = second.getDB(dbname);

var dropAction = [ 
    {dropIndexes: collection, index: "*"},
    {dropIndexes: collection, index: "i_1"},
    {drop: collection},
    {dropDatabase: 1 }
];


for (var idx = 0; idx < dropAction.length; idx++) {
    var dc = dropAction[idx];
    jsTest.log("Setting up collection " + collection + " for test of: " + JSON.stringify(dc));

    // set up collections
    masterDB.dropDatabase();
    jsTest.log("creating test data " + size + " documents");
    for(var i = 0; i < size; ++i ) {
        masterDB.getCollection(collection).save( {i:i} );
    }

    jsTest.log("Starting background indexing for test of: " + JSON.stringify(dc));
    masterDB.getCollection(collection).ensureIndex( {i:1}, {background:true} );
    assert.eq(2, masterDB.system.indexes.count( {ns:dbname + "." + collection} ) );

    // Wait for the secondary to get the index entry
    assert.soon( function() { 
        return 2 == secondDB.system.indexes.count( {ns:dbname + "." + collection} ); }, 
                 "index not created on secondary", 30000, 50 );

    jsTest.log("Index created and system.indexes entry exists on secondary");

    jsTest.log("running command " + JSON.stringify(dc));
    masterDB.runCommand( dc );
    jsTest.log("Waiting on replication");
    replTest.awaitReplication();

    // This doesn't assert, but waits for the bg index op to complete
    assert.soon( function() {
        return !checkOp(secondDB); }, "index not cancelled on secondary", 30000, 50 );
    assert.soon( function() { 
        var idx_count = secondDB.system.indexes.count( {ns:dbname + "." + collection} );
        return (idx_count == 1 || idx_count == 0);}, 
    //assert.soon( function() { return 2 == secondDB.system.indexes.count(); }, 
                 "Index not dropped on secondary", 30000, 50 );
}
jsTest.log("indexbg-interrupts.js done");
