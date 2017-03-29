// mh_gvns.C

#include <float.h>
#include <cstdint>
#include <stdio.h>
#include <vector>
#include <exception>
#include <stdexcept>
#include <assert.h>
#include <stdlib.h>
#include <cmath>

#include "mh_gvns.h"
#include "mh_random.h"
#include "mh_util.h"
#include "mh_c11threads.h"
#include <bits/exception_ptr.h>

namespace mh {

using namespace std;

int_param schlisel("schlisel","GVNS: locimp selection 0:seqrep,1:seqonce,2:randomrep,3:rndonce,4:adapt,5:timeapt",0,0,5);

int_param schshasel("schshasel","GVNS: shaking selection 0:seqrep,1:seqonce,2:randomrep,3:rndonce,4:adapt,5:timeapt",0,0,5);

bool_param schlirep("schlirep","GVNS: perform locimp nhs repeatedly",1);


//--------------------------------- GVNS ---------------------------------------------

SchedulerMethodSelector *GVNS::createSelector_constheu() {
	return new SchedulerMethodSelector(this,SchedulerMethodSelector::MSSequentialOnce);
}

SchedulerMethodSelector *GVNS::createSelector_locimpnh() {
	return new SchedulerMethodSelector(this,SchedulerMethodSelector::MethodSelStrat(_schlisel));
}

SchedulerMethodSelector *GVNS::createSelector_shakingnh() {
	return new SchedulerMethodSelector(this,SchedulerMethodSelector::MethodSelStrat(_schshasel));
}

GVNS::GVNS(pop_base &p, int nconstheu, int nlocimpnh, int nshakingnh, const std::string &pg) :
		ParScheduler(p, pg) {
	initialSolutionExists = false;
	constheu = createSelector_constheu();
	for (int t=0; t<_schthreads; t++) {
		locimpnh.push_back(createSelector_locimpnh());
		shakingnh.push_back(createSelector_shakingnh());
	}
	int i = 0;
	for (; i < nconstheu; i++)
			constheu->add(i);
	for (; i < nconstheu + nlocimpnh; i++)
		for (int t=0; t<_schthreads; t++)
			locimpnh[t]->add(i);
	for (; i < nconstheu + nlocimpnh + nshakingnh; i++)
		for (int t=0; t<_schthreads; t++)
			shakingnh[t]->add(i);
}

void GVNS::copyBetter(SchedulerWorker *worker, bool updateSchedulerData) {
	worker->pop.update(0, worker->tmpSol);
	if (updateSchedulerData && worker->pop[0]->isBetter(*(pop->at(0))))
		update(0, worker->pop[0]);
}

SchedulerMethod *GVNS::getNextMethod(int idx) {
	SchedulerWorker *worker = workers[idx];

	// must have the correct number of methods added
	assert(int(methodPool.size()) == constheu->size() + locimpnh[0]->size() + shakingnh[0]->size());

	// perform a construction method, either, because there is still a method available that
	// has not been applied yet, or because the worker has just been created.
	if (!constheu->empty() && (worker->method == nullptr || constheu->hasFurtherMethod())) {
		worker->method = constheu->select();
		if (worker->method != nullptr) {
			worker->methodContext=constheu->getMethodContext();
			return worker->method;
		}
	}
	// When proceeding from the construction methods to local improvement or shaking,
	// continue with the best solution from all construction methods
	if (!locimpnh[worker->id]->hasLastMethod() && !shakingnh[worker->id]->hasLastMethod()
			&& worker->pop[0]->isBetter(*worker->tmpSol)) {
		worker->tmpSol->copy(*worker->pop[0]);	// Copy best solution from former construction heuristic
	}
	// perform local improvement
	if (!locimpnh[0]->empty()) {

		// choose next local improvement method
		worker->method = locimpnh[worker->id]->select();
		if (worker->method != nullptr) {
			worker->methodContext=locimpnh[worker->id]->getMethodContext();
			return worker->method;
		}
		else
			// all local improvement methods applied to this solution, VND done
			locimpnh[worker->id]->reset(true);
	}
	// perform next shaking method
	if (!shakingnh[0]->empty()) {
		// if the worker's method is nullptr and no local improvement methods are available,
		// this means that no construction method has been scheduled before for this worker,
		// then check if globally a solution has already been constructed by some worker.
		if (worker->method == nullptr && locimpnh[0]->empty()) {
			if (!initialSolutionExists && (pop->size()==0 || !constheu->empty()))
				return nullptr;	// no, then there is no need to schedule an improvement method, yet.
			else {
				// yes, then we assign the best known solution and schedule a method to be applied to it.
				worker->pop.update(0, pop->at(0));
				worker->tmpSol->copy(*worker->pop[0]);
			}
		}
		worker->method = shakingnh[worker->id]->select();
		if (worker->method != nullptr) {
			worker->methodContext=shakingnh[worker->id]->getMethodContext();
			worker->startTime[1] = _wctime ? mhwctime() : mhcputime();
			return worker->method;
		}
	}

	// there exists no more method whose scheduling would be meaningful.
	// -> initiate termination
	finish = true;
	worker->method = nullptr;
	return nullptr;
}

void GVNS::updateData(int idx, bool updateSchedulerData, bool storeResult) {
	SchedulerWorker *worker = workers[idx];
	if (worker->method->idx < int(constheu->size())) {
		// construction method has been applied
		if (worker->tmpSolResult.accept) {
			preAcceptConstructionSolHook(worker->tmpSol);
			copyBetter(worker, updateSchedulerData);	// save new best solution
			if (!_schsync)
				initialSolutionExists = true;
		}
		else {
			// unsuccessful construction method (i.e., no better solution)
			if (updateSchedulerData)
				worker->checkGlobalBest(); // possibly update worker's incumbent by global best solution
		}
		return;
	}

	if (worker->method->idx < locimpnh[0]->size() + constheu->size()) {
		// local improvement neighborhood has been applied
		if (worker->tmpSolResult.reconsider==0 || (!worker->tmpSolResult.changed && worker->tmpSolResult.reconsider==-1))
			locimpnh[worker->id]->doNotReconsiderLastMethod();	// switch off this neighborhood for this solution
		if (worker->tmpSolResult.accept) {
			// solution to be accepted: save solution and possibly restart with first local improvement method
			preAcceptLocImpSolHook(worker->tmpSol);
			copyBetter(worker, updateSchedulerData);	// save new incumbent solution within local improvement
			if (_schlirep) {
				// restart with first local improvement method
				locimpnh[worker->id]->reset(true);
				return;
			}
		}
		else {
			// unsuccessful local improvement method call
			if (locimpnh[worker->id]->hasFurtherMethod()) {
				// continue VND with next neighborhood and incumbent VND solution
				if (worker->tmpSolResult.changed)
					worker->tmpSol->copy(*worker->pop[0]); // restore worker's incumbent
				return;
			}
		}
		// the embedded VND is done
		// update statistics for last shaking method (including the just finished local improvement)
		if (worker->pop[0]->isBetter(*(worker->pop[1]))) {
			updateShakingMethodStatistics(worker,true);
			worker->pop.update(1,worker->pop[0]);
			shakingnh[worker->id]->reset(true);
			if (updateSchedulerData)
				worker->checkGlobalBest(); // possibly update worker's incumbent by global best solution
			worker->tmpSol->copy(*worker->pop[0]); // restore worker's incumbent
		}
		else {
			// Go back to best solution before last shaking
			updateShakingMethodStatistics(worker,false);
			worker->tmpSol->copy(*worker->pop[1]);
			worker->pop.update(0,worker->tmpSol);
		}
	}
	else {
		// shaking neighborhood method has been applied
		if (locimpnh[0]->empty()) {
			if (worker->tmpSolResult.reconsider==0)
				shakingnh[worker->id]->doNotReconsiderLastMethod();	// switch off this neighborhood for this solution
			// no local improvement methods, directly handle result of shaking
			// update statistics for that method
			if (worker->tmpSolResult.accept) {
				// improvement achieved:
				preAcceptShakingSolHook(worker->tmpSol);
				worker->pop.update(1,worker->pop[0]);
				copyBetter(worker, updateSchedulerData);	// save new incumbent solution
				updateShakingMethodStatistics(worker,true);
				shakingnh[worker->id]->reset(true);
			}
			else {
				// unsuccessful neighborhood method call
				updateShakingMethodStatistics(worker,false);
				if (updateSchedulerData)
					worker->checkGlobalBest(); // possibly update worker's incumbent by global best solution
				worker->tmpSol->copy(*worker->pop[0]); // restore worker's incumbent
			}
		}
		else {
			// do not update statistics for that method (will be done after local improvement)
			// start available local improvement neighborhoods
			if (worker->tmpSolResult.accept) {
				preAcceptShakingSolHook(worker->tmpSol);
				copyBetter(worker, updateSchedulerData);	// save new best solution
			}
			else
				// nevertheless store solution after shaking as incumbent of local improvement
				worker->pop.update(0, worker->tmpSol);
		}
	}
}

void GVNS::updateDataFromResultsVectors(bool clearResults) {
	// update best solution in scheduler's population
	mh_solution* best = workers[0]->pop[0];
	for (int i=1; i < int(workers.size()); i++) {
		if (workers[i]->pop[0]->isBetter(*best))
			best = workers[i]->pop[0];
	}
	if (best->isBetter(*(pop->at(0)))) {
		initialSolutionExists = true;
		preAcceptFromResultsVectorsHook(best);
		update(0, best);
	}
	// solution migration: possibly replace threads' incumbents by best global solution
	if (_schpmig > 0)
		for (int i=0; i < int(workers.size()); i++)
			workers[i]->checkGlobalBest();
}

void GVNS::updateMethodStatistics(SchedulerWorker *worker, double methodTime) {
	if (worker->method->idx < constheu->size() + locimpnh[0]->size())
		Scheduler::updateMethodStatistics(worker->pop.at(0), worker->tmpSol,worker->method->idx,
				methodTime,worker->tmpSolResult);
	else {
		nIteration++;
		// skip shaking method statistics update except adding to totNetTime;
		// all remaining will be done separately when all local improvement neighborhoods have finished
		int idx=worker->method->idx;
		totNetTime[idx] += methodTime;
	}
}

void GVNS::updateShakingMethodStatistics(SchedulerWorker *worker, bool improved) {
	SchedulerMethod *sm = shakingnh[worker->id]->getLastMethod();
	if (sm != nullptr) {
		int idx=shakingnh[worker->id]->getLastMethod()->idx;
		totTime[idx] += (_wctime ? mhwctime() : mhcputime()) - 
				worker->startTime[1]; 
		nIter[idx]++;
		// if the applied method was successful, update the success-counter and the total obj-gain
		if (improved) {
			nSuccess[idx]++;
			sumGain[idx] += abs(worker->pop[0]->obj() - worker->pop[1]->obj());
		}
	}
}

void GVNS::reset() {
	Scheduler::reset();
	initialSolutionExists = false;
	constheu->reset(true);
	for (int t=0; t<_schthreads; t++) {
		locimpnh[t]->reset(true);
		shakingnh[t]->reset(true);
	}
}

} // end of namespace mh

