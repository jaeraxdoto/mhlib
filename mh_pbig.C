// mh_parscheduler.C

#include "mh_pbig.h"

namespace mh {

using namespace std;

//--------------------------------- PBIG ---------------------------------------------

PBIG::PBIG(pop_base &p, const std::string &pg)
		: Scheduler(p, pg) {
}

SchedulerMethod *PBIG::getNextMethod(int idx) {
	return Scheduler::getNextMethod(idx);
}

void PBIG::updateData(SchedulerMethodResult &tmpSolResult, int idx,
		bool updateSchedulerData, bool storeResult) {
	Scheduler::updateData(tmpSolResult,idx,updateSchedulerData,storeResult);
}

} // end of namespace mh

