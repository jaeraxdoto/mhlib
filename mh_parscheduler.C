// mh_parscheduler.C

#include <float.h>
#include <cstdint>
#include <stdio.h>
#include <vector>
#include <exception>
#include <stdexcept>
#include <assert.h>
#include <stdlib.h>
#include <cmath>
#include <algorithm>

#include "mh_parscheduler.h"
#include "mh_random.h"
#include "mh_util.h"
#include "mh_c11threads.h"
#include <bits/exception_ptr.h>

namespace mh {

using namespace std;

int_param schthreads("schthreads", "scheduler: number of threads used", 1, 1, 100);

bool_param schsync("schsync", "scheduler: synchronize all threads for being more deterministic", false);

double_param schpmig("schpmig", "scheduler: probability for migrating global best solutions at each shaking iteration", 0.1, 0, 1);


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

		if (!scheduler->terminate()) {
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
					scheduler->getNextMethod(this->id);	// try to find an available method for scheduling
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
					if (scheduler->workersWaiting < scheduler->_schthreads) {
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
							for (int i=0; i < scheduler->_schthreads; i++) {
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

				// run the scheduled method *******************************************************
				// scheduler->perfGenBeginCallback();
				// methodContext.callCounter has been initialized by getNextMethod
				methodContext->workerid = this->id;
				methodContext->incumbentSol = pop[0];
				tmpSolResult.reset();
				startTime[0] = mhcputime();
				method->run(tmpSol, *methodContext, tmpSolResult);
				double methodTime = mhcputime() - startTime[0];

				// augment missing information in tmpSolResult except tmpSOlResult.reconsider
				if (tmpSolResult.changed) {
					if (tmpSolResult.better == -1)
						tmpSolResult.better = tmpSol->isBetter(*pop[0]);
					if (tmpSolResult.accept == -1)
						tmpSolResult.accept = tmpSolResult.better;
				}
				else { // unchanged solution
					tmpSolResult.better = false;
					if (tmpSolResult.accept == -1)
						tmpSolResult.accept = false;
				}

				// update statistics and scheduler data
				scheduler->mutex.lock();

				scheduler->updateMethodStatistics(pop.at(0),tmpSol,method->idx,
						methodTime,tmpSolResult);
				scheduler->updateData(this->id, !scheduler->_schsync, scheduler->_schsync);

				bool termnow = scheduler->terminate();	// should we terminate?
				if (!termnow) {
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
	}
	catch (...) {
		// Pass any exceptions to main thread
		scheduler->mutex.lock();
		worker_exceptions.push_back(std::current_exception());
		scheduler->mutex.unlock();
	}
}


//--------------------------------- ParScheduler ---------------------------------------------

ParScheduler::ParScheduler(pop_base &p, const std::string &pg)
		: Scheduler(p, pg) {
	_schthreads = schthreads(pgroup);
	_schsync = _schthreads > 1 && schsync(pgroup); // only meaningful for more than one thread
	_schpmig = schpmig(pgroup);

 	workersWaiting = 0;
}

void ParScheduler::run() {
	checkPopulation();

	timStart = mhtime(_wctime);
	if (timFirstStart == 0)
		timFirstStart = timStart;
	if (lmethod(pgroup)) {
		writeLogHeader();
		writeLogEntry(false,true,"*");

	}

	// spawn the worker threads, each with its own random number generator having an own seed
	for (int i=0; i < _schthreads; i++) {
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
	if(_schsync)
		updateDataFromResultsVectors(true);

	for(auto w : workers)
		delete w;
	workers.clear();

	// handle possibly transferred exceptions
	rethrowExceptions();

	if (lmethod(pgroup)) {
		logmutex.lock();
		logstr.emptyEntry();
		logstr.flush();
		logmutex.unlock();
	}
}

void ParScheduler::reset() {
	Scheduler::reset();
	workersWaiting = 0;
}

void ParScheduler::rethrowExceptions() {
	for (const exception_ptr &ep : worker_exceptions)
		std::rethrow_exception(ep);
}

} // end of namespace mh

