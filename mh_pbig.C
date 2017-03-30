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
		for (int i=1; i <= destRecMethods; i++)
			destrec.back()->add(i);
	}
	for (int i=0;i<pop->size();i++)
		pop2.push_back(pop->at(i)->createUninitialized());
}

PBIG::~PBIG() {
	delete constheu;
	for (int i=0;i<pop->size();i++) {
		delete destrec[i];
		delete pop2[i];
	}
}

pair<SchedulerMethod *,SchedulerMethodContext *> PBIG::getNextMethod(int idx) {
	assert(int(methodPool.size()) == constheu->size() + destrec[0]->size());
	int s = nIteration % pop->size();	// Index of solution to be considered
	SchedulerMethodSelector *sel = (nIteration < pop->size() ? constheu : destrec[s]);
	SchedulerMethod *method = sel->select();
	assert(method!=nullptr);
	SchedulerMethodContext *context = sel->getMethodContext();
	return SchedulerMethodAndContext(method, context);
}

void PBIG::run() {
	checkPopulation();
	int psize = pop->size();

	timStart = mhtime(_wctime);
	if (timFirstStart == 0)
		timFirstStart = timStart;
	if (lmethod(pgroup)) {
		writeLogHeader();
		writeLogEntry(false,true,"*");
	}

	if (!terminate()) {
		for(;;) {
			checkPopulation();

			SchedulerMethodResult tmpSolResult;

			SchedulerMethodAndContext mc = getNextMethod(0);
			SchedulerMethod *method = mc.first;
			SchedulerMethodContext *methodContext = mc.second;
			if (methodContext == nullptr)
				methodContext = new SchedulerMethodContext;

			if (mc.first == nullptr || finish) // if in the meanwhile, termination has been started, terminate this thread as well
				break;
			int s = nIteration % psize;	// index of solution to consider
			pop2[s]->copy(*pop->at(s));

			// run the scheduled method
			// methodContext.callCounter has been initialized by getNextMethod
			double startTime=mhcputime();
			method->run(pop2[s], *methodContext, tmpSolResult);
			double methodTime = mhcputime() - startTime;

			// augment missing information in tmpSolResult except tmpSOlResult.reconsider
			if (tmpSolResult.changed) {
				if (tmpSolResult.better == -1)
					tmpSolResult.better = pop2[s]->isBetter(*pop->at(0));
				if (tmpSolResult.accept == -1)
					tmpSolResult.accept = tmpSolResult.better;
			}
			else { // unchanged solution
				tmpSolResult.better = false;
				if (tmpSolResult.accept == -1)
					tmpSolResult.accept = false;
			}

			// update statistics and scheduler data
			updateMethodStatistics(pop->at(s),pop2[s],method->idx,methodTime,tmpSolResult);
			// updateData(tmpSolResult, 0, true, false);
			bool termnow = terminate();	// should we terminate?

			if (!termnow || nIteration>logstr.lastIter())
				writeLogEntry(termnow, true, method->name);

			if (nIteration == psize) {
				// just copy first generation of constructed solutions
				for (int i=0;i<psize;i++)
					pop2[i] = pop->replace(i,pop2[i]);
				pop->recreateHashtable();
			}
			else if (nIteration > psize && nIteration % psize == 0) {
				// new population completed with D&R, merge into main population
				for (int i=0;i<psize;i++) {
					int r = pop->worstIndex();	// index of solution to be replaced
					if (pop->at(r)->isWorse(*pop2[i])) {
						// actually replace
						saveBest();
						pop2[i] = pop->replace(r,pop2[i]);
						checkBest();
						destrec[r]->reset(false);
					}
				}
				// out() << "Updated pop:" << endl;	pop->write(out());
			}

			if (terminate())
				break;
		}
	}

	if (lmethod(pgroup)) {
		logmutex.lock();
		logstr.emptyEntry();
		logstr.flush();
		logmutex.unlock();
	}
}

void PBIG::updateData(SchedulerMethodResult &tmpSolResult, int idx,
		bool updateSchedulerData, bool storeResult) {
}

} // end of namespace mh

