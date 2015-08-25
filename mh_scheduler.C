// mh_scheduler.C

#include <float.h>
#include <cstdint>
#include <stdio.h>
#include <vector>
#include <exception>
#include <stdexcept>
#include <assert.h>

#include "mh_scheduler.h"
#include "mh_random.h"
#include "mh_util.h"
#include "mh_c11threads.h"
#include <bits/exception_ptr.h>

int_param threadsnum("threadsnum", "Number of threads used in the scheduler", 1, 1, 100);

bool_param synchronize_threads("synchronize_threads", "If set to true, the synchronization of the threads in the scheduler is active (default: false)", false);

bool_param wall_clock_time("wall_clock_time", "If set to true, the times measured for the statistics of the scheduler are measured in wall clock time. Otherwise (default), they refer to the CPU time.", false);

//--------------------------------- SchedulerWorker ---------------------------------------------

/** Central stack of exceptions possibly occurring in threads,
 * which are passed to the main thread. */
static std::vector<std::exception_ptr> worker_exceptions;

void SchedulerWorker::run() {
	try {
		pop.update(1,pop[0]);			// Initialize pop[1] with a copy of pop[0]
		randomNumberGenerator = rng;	// set random number generator pointer to the one of this thread

		if (!scheduler->terminate())
			for(;;) {
				scheduler->checkPopulation();

				// if thread synchronization is active and not all threads
				// having a lower id are runinng, yet, then wait
				if(scheduler->_synchronize_threads && id > 0) {
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
					if(scheduler->_synchronize_threads && !isWorking) {
						scheduler->mutexOrderThreads.lock();
						isWorking = true;
						scheduler->cvOrderThreads.notify_all();
						scheduler->mutexOrderThreads.unlock();
					}
					scheduler->mutex.unlock(); // End of atomic operation

					if(method == NULL) {	// no method could be scheduled
						if(scheduler->_synchronize_threads)	// if thread synchronization is active, do not block here
							break;
						wait = true; // else, wait for other threads
					}
				} while (method == NULL);

				if (scheduler->finish) // if in the meanwhile, termination has been started, terminate this thread as well
					break;

				// if the threads should be synchronized ...
				if(scheduler->_synchronize_threads) {
					scheduler->mutex.lock();
					scheduler->mutexNotAllWorkersInPrepPhase.lock();
					scheduler->workersWaiting++; // increment the counter for the waiting workers
					scheduler->mutexNotAllWorkersInPrepPhase.unlock();

					// ... and if this is not the last thread to reach this point, then block it
					if(scheduler->workersWaiting < scheduler->_threadsnum) {
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
						scheduler->updateDataFromResultsVectors(true);
						scheduler->writeLogEntry();

						// ensure that only exactly titer updates are performed and that possibly
						// superfluous iterations are not considered in a deterministic way
						// (i.e. considering only the first threads (by id) and terminating the last ones that are too many
						int diff = scheduler->_titer - scheduler->nIteration;
						for(unsigned int i=0; i < scheduler->_threadsnum; i++) {
							scheduler->workers[i]->isWorking = false;
							if((signed)scheduler->workers[i]->id > diff-1)
								scheduler->workers[i]->terminate = true;
						}

						scheduler->mutexNotAllWorkersInPrepPhase.lock();
						scheduler->workersWaiting = 0;
						scheduler->cvNotAllWorkersInPrepPhase.notify_all(); // notify the possibly waiting threads
						scheduler->mutexNotAllWorkersInPrepPhase.unlock();
						scheduler->mutex.unlock();
					}

					if(method == NULL)
						continue;
					if(terminate)
						break;
				}

				// run the scheduled method
				// scheduler->perfGenBeginCallback();
				startTime = (scheduler->_wall_clock_time ? (WallClockTime() - scheduler->timStart) : CPUtime());
				bool tmpSolChanged = method->run(tmpSol);
				double methodTime = (scheduler->_wall_clock_time ? (WallClockTime() - scheduler->timStart) : CPUtime()) - startTime;


				if (tmpSolChanged) {
					if(tmpSol->isBetter(*pop[0]))
						tmpSolObjChange = OBJ_BETTER;
					else
						tmpSolObjChange = OBJ_WORSE;
				}
				else
					tmpSolObjChange = OBJ_NONE;

				// update statistics and scheduler data
				scheduler->mutex.lock();
				if(!scheduler->terminate()) {
					scheduler->updateMethodStatistics(this, methodTime);
					scheduler->updateData(this, !scheduler->_synchronize_threads, scheduler->_synchronize_threads);

					// notify threads that are waiting for an available method
					scheduler->mutexNoMethodAvailable.lock();
					scheduler->cvNoMethodAvailable.notify_all();
					scheduler->mutexNoMethodAvailable.unlock();
				}
				scheduler->mutex.unlock();

				// scheduler->perfGenEndCallback();

				if (scheduler->terminate()) {
					// write last generation info in any case
					if(!scheduler->_synchronize_threads) {
						scheduler->mutex.lock();
						scheduler->writeLogEntry(true);
						scheduler->mutex.unlock();
					}
					break;	// ... and stop
				}
				else {
					// write generation info
					if(!scheduler->_synchronize_threads) {
						scheduler->mutex.lock();
						scheduler->writeLogEntry();
						scheduler->mutex.unlock();
					}
				}
			}
			// if synchronization of threads is active, ensure that threads potentially
			// still blocked at mutexNotAllWorkersReadyForRunning or mutexOrderThreads are freed
			if(scheduler->_synchronize_threads) {
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

Scheduler::Scheduler(pop_base &p, const pstring &pg)
		: mh_advbase(p, pg), callback(NULL), finish(false) {

	wallClockTime = 0.0;

	_threadsnum = threadsnum(pgroup);
	_synchronize_threads = _threadsnum > 1 && synchronize_threads(pgroup); // only meaningful for more than one thread
	_titer = titer(pgroup);
	_wall_clock_time = wall_clock_time(pgroup);

 	workersWaiting = 0;
}

void Scheduler::run() {
	checkPopulation();

	timStart = (_wall_clock_time ? WallClockTime() : CPUtime());

	writeLogHeader();
	writeLogEntry();
	logstr.flush();

	// spawn the worker threads, each with its own random number generator having an own seed
	for(unsigned int i=0; i < _threadsnum; i++) {
		mh_random_number_generator* rng = new mh_random_number_generator();
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
	// if thread synchronization is active, perform final update of the scheduler's population and write final log entry
	if(_synchronize_threads) {
		updateDataFromResultsVectors(true);
		writeLogEntry(true);
	}
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
	if (callback != NULL && callback(pop->bestObj())) {
		finish = true;
		return true;
	}

	// "standard" termination criteria, modified to allow for termination after a certain
	// wall clock time, rather than cpu time, if _wall_clock_time == 1
	checkPopulation();
	if((titer(pgroup) >=0 && nIteration>=titer(pgroup)) ||
		(tciter(pgroup)>=0 && nIteration-iterBest>=tciter(pgroup)) ||
		(tobj(pgroup) >=0 && (maxi(pgroup)?getBestSol()->obj()>=tobj(pgroup):
					getBestSol()->obj()<=tobj(pgroup))) ||
		(ttime(pgroup)>=0 && ttime(pgroup)<=((_wall_clock_time ? WallClockTime() : CPUtime()) - timStart))) {
		finish = true;
		return true;
	}
	return false;
}

void Scheduler::updateMethodStatistics(SchedulerWorker *worker, double methodTime) {
	int idx=worker->method->idx;
	totTime[idx] += methodTime;
	nIter[idx]++;
	nIteration++;
	// if the applied method was successful, update the success-counter and the total obj-gain
	if (worker->tmpSolObjChange == OBJ_BETTER) {
		nSuccess[idx]++;
		sumGain[idx] += abs(worker->pop.at(0)->obj() - worker->tmpSol->obj());
	}
}

void Scheduler::printMethodStatistics(ostream &ostr) {
	ostr << endl << "Scheduler method statistics:" << endl;
	int sumSuccess=0,sumIter=0;
	double sumTime = 0;
	for (unsigned int k=0;k<methodPool.size();k++) {
		sumSuccess+=nSuccess[k];
		sumIter+=nIter[k];
		sumTime+= totTime[k];
	}
	ostr << "total num of iterations:\t" << sumIter << endl;
	ostr << "total num of successful iterations:\t" << sumSuccess << endl;
	ostr << "method\titerations\tsuccessful\tsuccess rate\ttotal obj-gain\tavg obj-gain\t rel success\ttotal time\t rel time" << endl;
	for (unsigned int k = 0; k < methodPool.size(); k++) {
		char tmp[200];
		sprintf(tmp,"%7s\t%6d\t\t%6d\t\t%9.4f %%\t%10.5f\t%10.5f\t%9.4f %%\t%9.4f\t%9.4f %%",
			methodPool[k]->name.c_str(),nIter[k],nSuccess[k],
			double(nSuccess[k])/double(nIter[k])*100.0,
			sumGain[k],
			double(sumGain[k])/double(nIter[k]),
			double(nSuccess[k])/double(sumSuccess)*100.0,
			totTime[k],
			double(totTime[k])/double(sumTime)*100.0);
		ostr << tmp << endl;
	}
	ostr << endl;
}

void Scheduler::printStatistics(ostream &ostr) {
	checkPopulation();

	char s[60];

	double tim = (_wall_clock_time ? (WallClockTime() - timStart) : CPUtime());
	const mh_solution *best=pop->bestSol();
	ostr << "# best solution:" << endl;
	sprintf( s, nformat(pgroup).c_str(), pop->bestObj() );
	ostr << "best objective value:\t" << s << endl;
	ostr << "best obtained in iteration:\t" << iterBest << endl;
	sprintf( s, nformat(pgroup).c_str(), timIterBest );
	ostr << "solution time for best:\t" << timIterBest << endl;
	ostr << "best solution:\t";
	best->write(ostr,0);
	ostr << endl;
	ostr << (_wall_clock_time ? "Wall clock time:\t" : "CPU-time:\t") << tim << endl;
	ostr << "iterations:\t" << nIteration << endl;
	//ostr << "local improvements:\t"  << nLocalImprovements << endl;
	printMethodStatistics(ostr);
}

void Scheduler::checkBest() {
	double nb=pop->bestObj();
	if (maxi(pgroup)?nb>bestObj:nb<bestObj)
	{
		iterBest=nIteration;
		timIterBest = (_wall_clock_time ? (WallClockTime() - timStart) : CPUtime());
	}
}

//--------------------------------- SchedulerMethodSelector ---------------------------------------------

SchedulerMethod *SchedulerMethodSelector::select() {
	if (methodList.empty())
		return NULL;
	switch (strategy) {
	case MSSequential:
	case MSSequentialOnce:
		if (unsigned(lastMethod) == methodList.size()-1) {
			if (strategy == MSSequential)
				lastMethod = 0;
			else
				return NULL;	// MSSequentialOnce
		}
		else
			lastMethod++;
		return scheduler->methodPool[methodList[lastMethod]];
		break;
	case MSRandom:
		return scheduler->methodPool[methodList[random_int(methodList.size())]];
	case MSRandomOnce: {
		if (unsigned(lastMethod) == methodList.size()-1)
			return NULL;	// no more methods
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
	return NULL;
}

SchedulerMethod *SchedulerMethodSelector::getLastMethod() {
	if (lastMethod == -1)
		return NULL;
	else
		return scheduler->methodPool[methodList[lastMethod]];
}


//--------------------------------- VNSScheduler ---------------------------------------------

GVNSScheduler::GVNSScheduler(pop_base &p, unsigned int nconstheu, unsigned int nlocimpnh,
		unsigned int nshakingnh, const pstring &pg) :
		Scheduler(p, pg),
		constheu(this, SchedulerMethodSelector::MSSequentialOnce) {
	initialSolutionExists = false;

	for (unsigned int t=0; t<_threadsnum; t++) {
		locimpnh.push_back(new SchedulerMethodSelector(this,
				SchedulerMethodSelector::MSSequentialOnce));
		shakingnh.push_back(new SchedulerMethodSelector(this,
				SchedulerMethodSelector::MSSequential));
	}
	unsigned int i = 0;
	for (; i < nconstheu; i++)
			constheu.add(i);
	for (; i < nconstheu + nlocimpnh; i++)
		for (unsigned int t=0; t<_threadsnum; t++)
			locimpnh[t]->add(i);
	for (; i < nconstheu + nlocimpnh + nshakingnh; i++)
		for (unsigned int t=0; t<_threadsnum; t++)
			shakingnh[t]->add(i);
}

void GVNSScheduler::copyBetter(SchedulerWorker *worker, bool updateSchedulerData) {
	worker->pop.update(0, worker->tmpSol);
	if (updateSchedulerData && worker->pop[0]->isBetter(*(pop->at(0))))
		update(0, worker->pop[0]);
}

void GVNSScheduler::getNextMethod(SchedulerWorker *worker) {
	// must have the exact number of methods added
	assert(methodPool.size() == constheu.size() + locimpnh[0]->size() + shakingnh[0]->size());

	// perform a construction method, either, because there is still a method available that
	// has not been applied, yet, or because the worker has just been created.
	if (!constheu.empty() && (worker->method == NULL || constheu.hasFurtherMethod())) {
		worker->method = constheu.select();
		if(worker->method != NULL)
			return;
	}
	if (!locimpnh[0]->empty()) {
		// choose next local improvement method
		worker->method = locimpnh[worker->id]->select();
		if (worker->method != NULL)
			return;
		else
			// all local improvement methods applied to this solution, VND done
			locimpnh[worker->id]->resetLastMethod();
	}
	// perform next shaking method
	if (!shakingnh[0]->empty()) {
		// if the worker's method is NULL, i.e. no construction method has been scheduled before
		// for this worker, check if globally a solution has already been constructed by some worker.
		if(worker->method == NULL) {
			if(!initialSolutionExists)
				return;	// no, then there is no need to schedule an improvement method, yet.
			else {
				// yes, then we assign the best known solution and schedule a method to be applied to it.
				worker->pop.update(0, pop->at(0));
				worker->tmpSol->copy(*worker->pop[0]);
			}
		}
		worker->method = shakingnh[worker->id]->select();
		if (worker->method != NULL) {
			worker->shakingStartTime = (_wall_clock_time ? (WallClockTime() - timStart) : CPUtime());
			return;
		}
	}

	if (!constheu.empty())
		worker->method = constheu.select();
	else
		mherror("Cannot find a suitable method in VNSScheduler::getNextMethod");
}


void GVNSScheduler::updateData(SchedulerWorker *worker, bool updateSchedulerData, bool storeResult) {
	if (worker->method->idx < constheu.size()) {
		// construction method has been applied
		if(worker->tmpSolObjChange == OBJ_BETTER) {
			copyBetter(worker, updateSchedulerData);	// save new best solution
			if(!_synchronize_threads)
				initialSolutionExists = true;
		}
		return;
	}

	if (worker->method->idx < locimpnh[0]->size() + constheu.size()) {
		// local improvement neighborhood has been applied
		if (worker->tmpSolObjChange == OBJ_BETTER) {
			// improvement achieved, restart with first local improvement method
			copyBetter(worker, updateSchedulerData);	// save new best solution within local improvement
			locimpnh[worker->id]->resetLastMethod();
			return;
		}
		else {
			// unsuccessful local improvement method call
			if (locimpnh[worker->id]->hasFurtherMethod()) {
				// continue VND with next neighborhood and incumbent VND solution
				worker->tmpSol->copy(*worker->pop[0]); // restore worker's incumbent
				return;
			}
			else {
				// the embedded VND is done
				// update statistics for last shaking method (including the just finished local improvement)
				if (worker->pop[0]->isBetter(*(worker->pop[1]))) {
					updateShakingMethodStatistics(worker,true);
					worker->pop.update(1,worker->pop[0]);
					shakingnh[worker->id]->resetLastMethod();
					worker->tmpSol->copy(*worker->pop[0]); // restore worker's incumbent
				}
				else {
					// Go back to best solution before last shaking
					updateShakingMethodStatistics(worker,false);
					worker->tmpSol->copy(*worker->pop[1]);
					worker->pop.update(0,worker->tmpSol);
				}
			}
		}
	}
	else {
		// shaking neighborhood method has been applied
		if (locimpnh[0]->empty()) {
			// no local improvement methods, directly handle result of shaking
			// update statistics for that method
			if (worker->tmpSolObjChange == OBJ_BETTER) {
				// improvement achieved:
				worker->pop.update(1,worker->pop[0]);
				copyBetter(worker, updateSchedulerData);	// save new best solution
				updateShakingMethodStatistics(worker,true);
				shakingnh[worker->id]->resetLastMethod();
			}
			else {
				// unsuccessful neighborhood method call
				updateShakingMethodStatistics(worker,false);
				worker->tmpSol->copy(*worker->pop[0]); // restore worker's incumbent
			}
		}
		else {
			// do not update statistics for that method (will be done after local improvement)
			// start available local improvement neighborhoods
			if (worker->tmpSolObjChange == OBJ_BETTER)
				copyBetter(worker, updateSchedulerData);	// save new best solution
			else
				// nevertheless store solution after shaking as incumbent of local improvement
				worker->pop.update(0, worker->tmpSol);
		}
	}
}

void GVNSScheduler::updateDataFromResultsVectors(bool clearResults) {
	mh_solution* best = workers[0]->pop[0];
	for(unsigned int i=1; i < workers.size(); i++) {
		if (workers[i]->pop[0]->isBetter(*best))
			best = workers[i]->pop[0];
	}
	if (best->isBetter(*(pop->at(0)))) {
		initialSolutionExists = true;
		update(0, best);
	}
}

void GVNSScheduler::updateMethodStatistics(SchedulerWorker *worker, double methodTime) {
	if (worker->method->idx < constheu.size() + locimpnh[0]->size())
		Scheduler::updateMethodStatistics(worker, methodTime);
	else
		nIteration++;
	// else skip shaking method statistics updated; will be done separately
}

void GVNSScheduler::updateShakingMethodStatistics(SchedulerWorker *worker, bool improved) {
	SchedulerMethod *sm = shakingnh[worker->id]->getLastMethod();
	if (sm != NULL) {
		int idx=shakingnh[worker->id]->getLastMethod()->idx;
		totTime[idx] += ((_wall_clock_time ? (WallClockTime() - timStart) : CPUtime()) - worker->shakingStartTime);
		nIter[idx]++;
		// if the applied method was successful, update the success-counter and the total obj-gain
		if (improved) {
			nSuccess[idx]++;
			sumGain[idx] += abs(worker->pop[0]->obj() - worker->pop[1]->obj());
		}
	}
}
