
#include "mongo/db/stats/extra_profiler.h"
#include "mongo/util/processinfo.h"

namespace mongo {


	void ExtraProfiler::reset(){
		 _recordingIO = false;
		 _currentIOType = NONE;
		_generalIO = 0;
		_filterIO = 0;
		_projectionIO = 0;
		 _sortIO = 0;
		 _touchIO = 0;

		_startOfIO = 0;

		_pageFaultsOriginal = pageFaultsNow();
	}

	string ExtraProfiler::report() const{
		StringBuilder s;
		s << " IOTimeSum (micro)" << IOSum();
		long long pages = pageFaultsNow();
		if (_pageFaultsOriginal != -1 && pages != -1)
			s << " PageFaults " << pages - _pageFaultsOriginal;
		return s.str();

	}

	BSONObj ExtraProfiler::toBSON() const{
		BSONObjBuilder b;

		b.appendNumber("IOTimeSum (micros)", IOSum());
		b.appendNumber("IOTimeGeneral (micros)", _generalIO);
		b.appendNumber("IOTimeFilter (micros)", _filterIO);
		b.appendNumber("IOTimeProjection (micros)", _projectionIO);
		b.appendNumber("IOTimeSort (micros)", _sortIO);
		b.appendNumber("IOTimeTouch (micros)", _touchIO);
		long long pages = pageFaultsNow();
		if (_pageFaultsOriginal != -1 && pages != -1)
			b.appendNumber("IOPageFaults", pages - _pageFaultsOriginal);

		b.done();

		return b.obj();

	}

	void ExtraProfiler::startIORecord (const IOTypes& ioType){
		if (_currentIOType != NONE || _recordingIO == true){
			log() << "IO started before previous was completed" << endl;
		}

		_currentIOType = ioType;
		_recordingIO = true;
		_startOfIO = curTimeMicros64();
	}

	void ExtraProfiler::stopIORecord (const IOTypes& ioType){
		if (_currentIOType != ioType || _recordingIO == false){
			log() << "IO stoped before it was started, not recording the measurement" << endl;
			return;
		}

		_currentIOType = NONE;
		_recordingIO = false;
		long long timeTaken = curTimeMicros64() - _startOfIO;
		if (timeTaken >= 10) {
		if (ioType == GENERAL) _generalIO += timeTaken;
		else if (ioType == FILTER) _filterIO += timeTaken;
		else if (ioType == PROJECTION) _projectionIO += timeTaken;
		else if (ioType == SORT) _sortIO += timeTaken;
		else if (ioType == TOUCH) _touchIO += timeTaken;
	}

	}

	long long ExtraProfiler::IOSum() const{
		return _generalIO + _filterIO + _projectionIO + _sortIO + _touchIO;
	}


	long long ExtraProfiler::pageFaultsNow() const{
		BSONObjBuilder bb;
                
        ProcessInfo p;
        p.getExtraInfo(bb);

        BSONObj extraInfo = bb.obj();

        if (!extraInfo.getField("page_faults").eoo()) {
        	return  extraInfo.getField("page_faults").safeNumberLong();
        }
        return -1;
                
	}

	/// STATIC ////
	void ExtraProfiler::startIO (const IOTypes& ioType) {
		const Client& client = cc();

		if (client.curop()){
			client.curop()->debug().extraP.startIORecord (ioType);
		}
	}
	void ExtraProfiler::stopIO (const IOTypes& ioType){
		const Client& client = cc();

		if (client.curop()){
			client.curop()->debug().extraP.stopIORecord (ioType);
		}
	}


}