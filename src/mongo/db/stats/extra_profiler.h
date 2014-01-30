
#include "mongo/db/curop.h"
#include "mongo/util/time_support.h"

namespace mongo {

#ifndef EXTRA_PROFILER
#define  EXTRA_PROFILER
class ExtraProfiler {
public:
	enum IOTypes { 
		NONE,

		GENERAL,

		FILTER,

		PROJECTION,

		SORT,

		TOUCH

	 };
	ExtraProfiler() {reset();}

	void reset();

	string report() const;

	BSONObj toBSON() const;

	long long IOSum() const;

	long long pageFaultsNow() const;


	void startIORecord (const IOTypes& ioType);
	void stopIORecord (const IOTypes& ioType);

	static void startIO (const IOTypes& ioType);
	static void stopIO (const IOTypes& ioType);
private:
	bool _recordingIO;
	IOTypes _currentIOType;

	long long _generalIO;
	long long _filterIO;
	long long _projectionIO;
	long long _sortIO;
	long long _startOfIO;
	long long _touchIO;

	long long _pageFaultsOriginal;


};

#endif
}
