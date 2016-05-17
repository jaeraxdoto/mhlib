// mh_scheduler.C

#include <float.h>
#include <cstdint>
#include <stdio.h>
#include <vector>
#include <exception>
#include <stdexcept>
#include <assert.h>
#include <stdlib.h>
#include <cmath>

#include "mh_scheduler.h"
#include "mh_random.h"
#include "mh_util.h"
#include "mh_c11threads.h"
#include <bits/exception_ptr.h>

namespace mh {

using namespace std;

int_param schthreads("schthreads", "scheduler: number of threads used", 1, 1, 100);

bool_param schsync("schsync", "scheduler: synchronize all threads for being more deterministic", false);

double_param schpmig("schpmig", "scheduler: probability for migrating global best solutions at each shaking iteration", 0.1, 0, 1);

bool_param lmethod("lmethod","scheduler: append method name to each log entry",true);

int_param schlisel("schlisel","GVNS: locimp selection 0:seqrep,1:seqonce,2:randomrep,3:rndonce,4:adapt",1,0,4);

bool_param schlirep("schlirep","GVNS: perform locimp nhs repeatedly",1);

//--------------------------------- SchedulerWorker ---------------------------------------------

/** Central stack of exceptions possibly occurring in threads,
 * which are passed to the main thread. */
static std::vector<std::exception_ptr> worker_exceptions;

void SchedulerWorker::checkGlobalBest() {
	if (pop[0]->isWorse(*scheduler->pop->at(0)) &&
			random_double() <= scheduler->_schpmig)
		pop.update(0, scheduler->pop->at(0));
}

void SchedulerWorker::run() {
	try {
		pop.update(1,pop[0]);			// Initialize pop[1] with a copy of pop[0]
		setRandomNumberGenerator(rng);	// set random number generator pointer to the one of this thread

		if (!scheduler->terminate())
			for(;;) {
				scheduler->checkPopulation();

				// if thread synchronization is active and not all threads
				// having a lower id are running, yet, then wait
				if(scheduler->_schsync && id > 0) {
					std::unique_lock<std::mutex> lck(scheduler->mutexOrderThreads);
					while(!scheduler->workers[id-1]->isWorking && !scheduler->terminate())
						scheduler->cvOrderThreads.wait(lck);
				}

				// 	schedule the next method
				bool wait = false;	// indicates if the thread needs to wait for another thread to finish
				do {
					if(wait) { // need to wait for other threads, block until notified
						std::unique_lock<std::mutex> lck(scheduler->mutexNoMethodAvailable);
						scheduler->cvNoMethodAvailable.wait(lck);
						if(scheduler->terminate())	// if termination is in progress, terminate also this thread!
							break;
					}

					scheduler->mutex.lock(); // Begin of atomic operation
					scheduler->getNextMethod(this);	// try to find an available method for scheduling
					// if thread synchronization is active and this thread has not started, yet,
					// set the isWorking flag to true and notify possibly waiting threads
					if (scheduler->_schsync && !isWorking) {
						scheduler->mutexOrderThreads.lock();
						isWorking = true;
						scheduler->cvOrderThreads.notify_all();
						scheduler->mutexOrderThreads.unlock();
					}
					scheduler->mutex.unlock(); // End of atomic operation

					if (method == nullptr) {	// no method could be scheduled
						if(scheduler->finish) // should the algorithm be terminated due to exhaustion of all available methods
							break;
						if(scheduler->_schsync)	// if thread synchronization is active, do not block here
							break;
						wait = true; // else, wait for other threads
					}
				} while (method == nullptr);

				if (scheduler->finish) // if in the meanwhile, termination has been started, terminate this thread as well
					break;

				// if the threads should be synchronized ...
				if(scheduler->_schsync) {
					scheduler->mutex.lock();
					scheduler->mutexNotAllWorkersInPrepPhase.lock();
					scheduler->workersWaiting++; // increment the counter for the waiting workers
					scheduler->mutexNotAllWorkersInPrepPhase.unlock();

					// ... and if this is not the last thread to reach this point, then block it
					if(scheduler->workersWaiting < scheduler->_schthreads) {
						if(!scheduler->terminate()) {
							std::unique_lock<std::mutex> lck(scheduler->mutexNotAllWorkersInPrepPhase);
							scheduler->mutex.unlock();
							scheduler->cvNotAllWorkersInPrepPhase.wait(lck);
						}
						else
							scheduler->mutex.unlock();
					}
					// ... else, the last thread has reached this point
					else {
						// update the global scheduler data and notify the waiting workers
						resetRandomNumberGenerator();	// use default rng during the global update, as the current thread is unknown
						scheduler->updateDataFromResultsVectors(true);
						setRandomNumberGenerator(rng); // reset to thread's rng

						// If termination after a certain number of iterations is requested,
						// ensure that only exactly titer updates are performed and that possibly
						// superfluous iterations are not considered in a deterministic way
						// (i.e. considering only the first threads (by id) and terminating the last ones that are too many).
						if(scheduler->_titer > -1) {
							int diff = scheduler->_titer - scheduler->nIteration;
							for(unsigned int i=0; i < scheduler->_schthreads; i++) {
								scheduler->workers[i]->isWorking = false;
								if((signed)scheduler->workers[i]->id > diff-1)
									scheduler->workers[i]->terminate = true;
							}
						}

						scheduler->mutexNotAllWorkersInPrepPhase.lock();
						scheduler->workersWaiting = 0;
						scheduler->cvNotAllWorkersInPrepPhase.notify_all(); // notify the possibly waiting threads
						scheduler->mutexNotAllWorkersInPrepPhase.unlock();
						scheduler->mutex.unlock();
					}

					if(method == nullptr)
						continue;
					if(terminate)
						break;
				}

				// run the scheduled method
				// scheduler->perfGenBeginCallback();
				startTime = mhcputime();
				bool tmpSolChanged = method->run(tmpSol);
				double methodTime = mhcputime() - startTime;

				if (tmpSolChanged) {
					if (tmpSol->isBetter(*pop[0]))
						tmpSolObjChange = SchedulerMethodResult::OBJ_BETTER;
					else
						tmpSolObjChange = SchedulerMethodResult::OBJ_WORSE;
				}
				else
					tmpSolObjChange = SchedulerMethodResult::OBJ_SAME;

				// update statistics and scheduler data
				scheduler->mutex.lock();

				bool termnow = scheduler->terminate();	// should we terminate?
				if (!termnow) {
					scheduler->updateMethodStatistics(this, methodTime);
					scheduler->updateData(this, !scheduler->_schsync, scheduler->_schsync);

					// notify threads that are waiting for an available method
					scheduler->mutexNoMethodAvailable.lock();
					scheduler->cvNoMethodAvailable.notify_all();
					scheduler->mutexNoMethodAvailable.unlock();
				}

				// scheduler->perfGenEndCallback();

				if (!termnow || scheduler->nIteration>logstr.lastIter())
					scheduler->writeLogEntry(termnow, true, method->name);

				scheduler->mutex.unlock();
				if (scheduler->terminate())
					break;
			}
			// if synchronization of threads is active, ensure that threads potentially
			// still blocked at mutexNotAllWorkersReadyForRunning or mutexOrderThreads are freed
			if(scheduler->_schsync) {
				scheduler->mutexNotAllWorkersInPrepPhase.lock();
				scheduler->cvNotAllWorkersInPrepPhase.notify_all();
				scheduler->mutexNotAllWorkersInPrepPhase.unlock();
				scheduler->mutexOrderThreads.lock();
				scheduler->cvOrderThreads.notify_all();
				scheduler->mutexOrderThreads.unlock();
			}
	}
	catch (...) {
		// Pass any exceptions to main thread
		scheduler->mutex.lock();
		worker_exceptions.push_back(std::current_exception());
		scheduler->mutex.unlock();
	}
}


//--------------------------------- Scheduler ---------------------------------------------

Scheduler::Scheduler(pop_base &p, const std::string &pg)
		: mh_advbase(p, pg), callback(nullptr), finish(false) {
	_schthreads = schthreads(pgroup);
	_schsync = _schthreads > 1 && schsync(pgroup); // only meaningful for more than one thread
	_titer = titer(pgroup);
	_schpmig = schpmig(pgroup);
 	workersWaiting = 0;
}

void Scheduler::run() {
	checkPopulation();

	timStart = (_wctime ? mhwctime() : mhcputime());

	writeLogHeader();
	writeLogEntry(false,true,"*");
	logstr.flush();

	// spawn the worker threads, each with its own random number generator having an own seed
	for(unsigned int i=0; i < _schthreads; i++) {
		mh_randomNumberGenerator* rng = new mh_randomNumberGenerator();
		rng->random_seed(random_int(INT32_MAX));
		mutex.lock(); // Begin of atomic operation
		SchedulerWorker *w = new SchedulerWorker(this, i, pop->at(0), rng);
		mutex.unlock(); // End of atomic operation
		workers.push_back(w);
		w->thread = std::thread(&SchedulerWorker::run, w);
	}

	// wait for the threads to finish and delete them
	for(auto w : workers)
		w->thread.join();
	// if thread synchronization is active, perform final update of the scheduler's population
	// and write final log entries
	if(_schsync)
		updateDataFromResultsVectors(true);
	for(auto w : workers)
		delete w;

	// handle possibly transferred exceptions
	for (const exception_ptr &ep : worker_exceptions)
		std::rethrow_exception(ep);

	logstr.emptyEntry();
	logstr.flush();
}

bool Scheduler::terminate() {
	if (finish)
		return true;
	if (callback != nullptr && callback(pop->bestObj())) {
		finish = true;
		return true;
	}

	// "standard" termination criteria, modified to allow for termination after a certain
	// wall clock time, rather than cpu time, if _wctime is set
	checkPopulation();
	if((titer(pgroup) >=0 && nIteration>=titer(pgroup)) ||
		(tciter(pgroup)>=0 && nIteration-iterBest>=tciter(pgroup)) ||
		(tobj(pgroup) >=0 && (maxi(pgroup)?getBestSol()->obj()>=tobj(pgroup):
					getBestSol()->obj()<=tobj(pgroup))) ||
		(ttime(pgroup)>=0 && ttime(pgroup)<=((_wctime ? mhwctime() : mhcputime()) - timStart))) {
		finish = true;
		return true;
	}
	return false;
}

void Scheduler::updateMethodStatistics(SchedulerWorker *worker, double methodTime) {
	int idx=worker->method->idx;
	totNetTime[idx] = (totTime[idx] += methodTime);
	nIter[idx]++;
	nIteration++;
	// if the applied method was successful, update the success-counter and the total obj-gain
	if (worker->tmpSolObjChange == SchedulerMethodResult::OBJ_BETTER) {
		nSuccess[idx]++;
		sumGain[idx] += abs(worker->pop.at(0)->obj() - worker->tmpSol->obj());
	}
}

void Scheduler::printMethodStatistics(ostream &ostr) {
	double totSchedulerTime = mhcputime() - timStart;
	ostr << endl << "Scheduler method statistics:" << endl;
	int sumSuccess=0,sumIter=0;
	double sumTime = 0;
	for (unsigned int k=0;k<methodPool.size();k++) {
		sumSuccess+=nSuccess[k];
		sumIter+=nIter[k];
		sumTime+=totNetTime[k];
	}
	ostr << "total num of iterations:\t" << sumIter << endl;
	ostr << "total num of successful iterations:\t" << sumSuccess << endl;
	ostr << "total netto time:\t" << sumTime << "\ttotal scheduler time:\t" << totSchedulerTime << endl;
	ostr << "method\t   iter\t   succ\tsucc-rate%\ttotal-obj-gain\tavg-obj-gain\trel-succ%\ttotal-time\trel-time%\ttot-net-time\trel-net-time%" << endl;
	for (unsigned int k = 0; k < methodPool.size(); k++) {
		char tmp[250];
		snprintf(tmp,sizeof(tmp),"%7s\t%7d\t%6d\t%9.4f\t%10.5f\t%10.5f\t%9.4f\t%9.4f\t%9.4f\t%9.4f\t%9.4f",
			methodPool[k]->name.c_str(),nIter[k],nSuccess[k],
			double(nSuccess[k])/double(nIter[k])*100.0,
			sumGain[k],
			double(sumGain[k])/double(nIter[k]),
			double(nSuccess[k])/double(sumSuccess)*100.0,
			totTime[k],
			double(totTime[k])/totSchedulerTime*100.0,
			totNetTime[k],
			double(totNetTime[k])/double(sumTime)*100.0);
		ostr << tmp << endl;
	}
	ostr << endl;
}

void Scheduler::printStatistics(ostream &ostr) {
	checkPopulation();

	char s[60];

	double wctime = mhwctime();
	double cputime = mhcputime();

	mh_solution *best=pop->bestSol();
	ostr << "# best solution:" << endl;
	snprintf( s, sizeof(s), nformat(pgroup).c_str(), pop->bestObj() );
	ostr << "best objective value:\t" << s << endl;
	ostr << "best obtained in iteration:\t" << iterBest << endl;
	snprintf( s, sizeof(s), nformat(pgroup).c_str(), timIterBest );
	ostr << "solution time for best:\t" << timIterBest << endl;
	ostr << "best solution:\t";
	best->write(ostr,0);
	ostr << endl;
	ostr << "CPU time:\t" << cputime << "\twall clock time:\t" << wctime << endl;
	ostr << "iterations:\t" << nIteration << endl;
	//ostr << "local improvements:\t"  << nLocalImprovements << endl;
	printMethodStatistics(ostr);
}

void Scheduler::writeLogHeader(bool finishEntry) {
	// mh_advbase::writeLogHeader(false);
	// if (lmethod(pgroup))
	// 	logstr.write("method");
	// if (finishEntry)
	//		logstr.finishEntry();
	checkPopulation();
	logstr.headerEntry();
	if (ltime(pgroup))
		logstr.write(_wctime ? "wctime" : "cputime");
	if (lmethod(pgroup))
		logstr.write("method");
	if (finishEntry)
		logstr.finishEntry();
}


bool Scheduler::writeLogEntry(bool inAnyCase, bool finishEntry, const std::string &method) {
	// if (mh_advbase::writeLogEntry(inAnyCase,false)) {
	//	if (lmethod(pgroup))
	//		logstr.write(method);
	//	if (finishEntry)
	//		logstr.finishEntry();
	//	return true;
	//}
	//return false;
	checkPopulation();
	if (logstr.startEntry(nIteration,pop->bestObj(),inAnyCase))
	{
		if (ltime(pgroup))
			logstr.write((_wctime ? (mhwctime() - timStart) : mhcputime()));
		if (lmethod(pgroup))
			logstr.write(method);
		if (finishEntry)
			logstr.finishEntry();
		return true;
	}
	return false;
}


//--------------------------------- SchedulerMethodSelector ---------------------------------------------

SchedulerMethod *SchedulerMethodSelector::select() {
	if (methodList.empty())
		return nullptr;
	switch (strategy) {
	case MSSequentialRep:
	case MSSequentialOnce:
		if (unsigned(lastMethod) == methodList.size()-1) {
			if (strategy == MSSequentialRep)
				lastMethod = 0;
			else
				return nullptr;	// MSSequentialOnce
		}
		else
			lastMethod++;
		return scheduler->methodPool[methodList[lastMethod]];
		break;
	case MSRandomRep:
		return scheduler->methodPool[methodList[random_int(methodList.size())]];
	case MSRandomOnce: {
		if (unsigned(lastMethod) == methodList.size()-1)
			return nullptr;	// no more methods
		lastMethod++;
		// Choose randomly a not yet selected method and swap it to position lastMethod
		int r = random_int(lastMethod, methodList.size()-1);
		if (r != lastMethod) {
			unsigned int tmp = methodList[lastMethod];
			methodList[lastMethod] = methodList[r];
			methodList[r] = tmp;
		}
		return scheduler->methodPool[methodList[lastMethod]];
	}
	case MSSelfadaptive:
		mherror("Selfadaptive strategy in SchedulerMethodSelector::select not yet implemented");
		break;
	default:
		mherror("Invalid strategy in SchedulerMethodSelector::select",tostring(strategy));
	}
	return nullptr;
}

SchedulerMethod *SchedulerMethodSelector::getLastMethod() {
	if (lastMethod == -1)
		return nullptr;
	else
		return scheduler->methodPool[methodList[lastMethod]];
}


//--------------------------------- GVNSScheduler ---------------------------------------------

SchedulerMethodSelector *GVNSScheduler::createSelector_constheu() {
	return new SchedulerMethodSelector(this,SchedulerMethodSelector::MSSequentialOnce);
}

SchedulerMethodSelector *GVNSScheduler::createSelector_locimpnh() {
	return new SchedulerMethodSelector(this,SchedulerMethodSelector::MethodSelStrat(_schlisel));
}

SchedulerMethodSelector *GVNSScheduler::createSelector_shakingnh() {
	return new SchedulerMethodSelector(this,SchedulerMethodSelector::MSSequentialRep);
}

GVNSScheduler::GVNSScheduler(pop_base &p, unsigned int nconstheu, unsigned int nlocimpnh,
		unsigned int nshakingnh, const std::string &pg) :
		Scheduler(p, pg) {
	initialSolutionExists = false;
	constheu = createSelector_constheu();
	for (unsigned int t=0; t<_schthreads; t++) {
		locimpnh.push_back(createSelector_locimpnh());
		shakingnh.push_back(createSelector_shakingnh());
	}
	unsigned int i = 0;
	for (; i < nconstheu; i++)
			constheu->add(i);
	for (; i < nconstheu + nlocimpnh; i++)
		for (unsigned int t=0; t<_schthreads; t++)
			locimpnh[t]->add(i);
	for (; i < nconstheu + nlocimpnh + nshakingnh; i++)
		for (unsigned int t=0; t<_schthreads; t++)
			shakingnh[t]->add(i);
}

void GVNSScheduler::copyBetter(SchedulerWorker *worker, bool updateSchedulerData) {
	worker->pop.update(0, worker->tmpSol);
	if (updateSchedulerData && worker->pop[0]->isBetter(*(pop->at(0))))
		update(0, worker->pop[0]);
}

void GVNSScheduler::getNextMethod(SchedulerWorker *worker) {
	// must have the correct number of methods added
	assert(methodPool.size() == constheu->size() + locimpnh[0]->size() + shakingnh[0]->size());

	// perform a construction method, either, because there is still a method available that
	// has not been applied yet, or because the worker has just been created.
	if (!constheu->empty() && (worker->method == nullptr || constheu->hasFurtherMethod())) {
		worker->method = constheu->select();
		if(worker->method != nullptr)
			return;
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
		if (worker->method != nullptr)
			return;
		else
			// all local improvement methods applied to this solution, VND done
			locimpnh[worker->id]->resetLastMethod();
	}
	// perform next shaking method
	if (!shakingnh[0]->empty()) {
		// if the worker's method is nullptr and no local improvement methods are available,
		// this means that no construction method has been scheduled before for this worker,
		// then check if globally a solution has already been constructed by some worker.
		if(worker->method == nullptr && locimpnh[0]->empty()) {
			if(!initialSolutionExists)
				return;	// no, then there is no need to schedule an improvement method, yet.
			else {
				// yes, then we assign the best known solution and schedule a method to be applied to it.
				worker->pop.update(0, pop->at(0));
				worker->tmpSol->copy(*worker->pop[0]);
			}
		}
		worker->method = shakingnh[worker->id]->select();
		if (worker->method != nullptr) {
			worker->shakingStartTime = (_wctime ? (mhwctime() - timStart) : mhcputime());
			return;
		}
	}

	// there exists no more method whose scheduling would be meaningful.
	// -> initiate termination
	finish = true;
	worker->method = nullptr;
	return;
}


void GVNSScheduler::updateData(SchedulerWorker *worker, bool updateSchedulerData, bool storeResult) {
	if (worker->method->idx < constheu->size()) {
		// construction method has been applied
		if(worker->tmpSolObjChange == SchedulerMethodResult::OBJ_BETTER) {
			copyBetter(worker, updateSchedulerData);	// save new best solution
			if (!_schsync)
				initialSolutionExists = true;
		}
		else {
			// unsuccessful construction method (i.e., no better solution)
			if(updateSchedulerData)
				worker->checkGlobalBest(); // possibly update worker's incumbent by global best solution
		}
		return;
	}

	if (worker->method->idx < locimpnh[0]->size() + constheu->size()) {
		// local improvement neighborhood has been applied
		if (worker->tmpSolObjChange == SchedulerMethodResult::OBJ_BETTER) {
			// improvement achieved, save solution and possibly restart with first local improvement method
			copyBetter(worker, updateSchedulerData);	// save new best solution within local improvement
			if (_schlirep) {
				// restart with first local improvement method
				locimpnh[worker->id]->resetLastMethod();
				return;
			}
		}
		else {
			// unsuccessful local improvement method call
			if (locimpnh[worker->id]->hasFurtherMethod()) {
				// continue VND with next neighborhood and incumbent VND solution
				if (worker->tmpSolObjChange != SchedulerMethodResult::OBJ_SAME)
					worker->tmpSol->copy(*worker->pop[0]); // restore worker's incumbent
				return;
			}
		}
		// the embedded VND is done
		// update statistics for last shaking method (including the just finished local improvement)
		if (worker->pop[0]->isBetter(*(worker->pop[1]))) {
			updateShakingMethodStatistics(worker,true);
			worker->pop.update(1,worker->pop[0]);
			shakingnh[worker->id]->resetLastMethod();
			if(updateSchedulerData)
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
			// no local improvement methods, directly handle result of shaking
			// update statistics for that method
			if (worker->tmpSolObjChange == SchedulerMethodResult::OBJ_BETTER) {
				// improvement achieved:
				worker->pop.update(1,worker->pop[0]);
				copyBetter(worker, updateSchedulerData);	// save new best solution
				updateShakingMethodStatistics(worker,true);
				shakingnh[worker->id]->resetLastMethod();
			}
			else {
				// unsuccessful neighborhood method call
				updateShakingMethodStatistics(worker,false);
				if(updateSchedulerData)
					worker->checkGlobalBest(); // possibly update worker's incumbent by global best solution
				worker->tmpSol->copy(*worker->pop[0]); // restore worker's incumbent
			}
		}
		else {
			// do not update statistics for that method (will be done after local improvement)
			// start available local improvement neighborhoods
			if (worker->tmpSolObjChange == SchedulerMethodResult::OBJ_BETTER)
				copyBetter(worker, updateSchedulerData);	// save new best solution
			else
				// nevertheless store solution after shaking as incumbent of local improvement
				worker->pop.update(0, worker->tmpSol);
		}
	}
}

void GVNSScheduler::updateDataFromResultsVectors(bool clearResults) {
	// update best solution in scheduler's population
	mh_solution* best = workers[0]->pop[0];
	for(unsigned int i=1; i < workers.size(); i++) {
		if (workers[i]->pop[0]->isBetter(*best))
			best = workers[i]->pop[0];
	}
	if (best->isBetter(*(pop->at(0)))) {
		initialSolutionExists = true;
		update(0, best);
	}
	// solution migration: possibly replace threads' incumbents by best global solution
	if(_schpmig > 0)
		for(unsigned int i=0; i < workers.size(); i++)
			workers[i]->checkGlobalBest();
}

void GVNSScheduler::updateMethodStatistics(SchedulerWorker *worker, double methodTime) {
	if (worker->method->idx < constheu->size() + locimpnh[0]->size())
		Scheduler::updateMethodStatistics(worker, methodTime);
	else {
		nIteration++;
		// skip shaking method statistics update except adding to totNetTime;
		// all remaining will be done separately when all local improvement neighborhoods have finished
		int idx=worker->method->idx;
		totNetTime[idx] += methodTime;
	}
}

void GVNSScheduler::updateShakingMethodStatistics(SchedulerWorker *worker, bool improved) {
	SchedulerMethod *sm = shakingnh[worker->id]->getLastMethod();
	if (sm != nullptr) {
		int idx=shakingnh[worker->id]->getLastMethod()->idx;
		totTime[idx] += ((_wctime ? (mhwctime() - timStart) : mhcputime()) - worker->shakingStartTime);
		nIter[idx]++;
		// if the applied method was successful, update the success-counter and the total obj-gain
		if (improved) {
			nSuccess[idx]++;
			sumGain[idx] += abs(worker->pop[0]->obj() - worker->pop[1]->obj());
		}
	}
}

} // end of namespace mh

