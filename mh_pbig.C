// mh_parscheduler.C

#include "mh_pbig.h"

namespace mh {

using namespace std;

//--------------------------------- PBIG ---------------------------------------------

PBIG::PBIG(pop_base &p, int destRecMethods, const std::string &pg)
		: Scheduler(p, pg) {
	constheu = new SchedulerMethodSelector(this,SchedulerMethodSelector::MSSequentialRep);
	constheu->add(0);
	for (int s=0; s<pop->size(); s++) {
		destrec.push_back(new SchedulerMethodSelector(this,SchedulerMethodSelector::MSSequentialRep));
		destrec.back()->add(0);
		for (int i=1; i < destRecMethods; i++)
			destrec.back()->add(i);
	}
}

std::pair<SchedulerMethod *,SchedulerMethodContext *> PBIG::getNextMethod(int idx) {
	assert(int(methodPool.size())==constheu->size() + destrec[0]->size());
	int s = nIteration % pop->size();	// Index of solution to be considered
	SchedulerMethodSelector *sel = (nIteration < pop->size() ? constheu : destrec[s]);
	SchedulerMethod *method = sel->select();
	assert(method!=nullptr);
	SchedulerMethodContext *context = sel->getMethodContext();
	return SchedulerMethodAndContext(method, context);
}

void PBIG::updateData(SchedulerMethodResult &tmpSolResult, int idx,
		bool updateSchedulerData, bool storeResult) {
	Scheduler::updateData(tmpSolResult,idx,updateSchedulerData,storeResult);
}

} // end of namespace mh

