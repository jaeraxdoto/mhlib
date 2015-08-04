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

// #define DEBUGMETH 1		// Enable for writing out debug info on applied methods

int_param threadsnum("threadsnum", "Number of threads used in the scheduler", 1, 1, 100);

bool_param synchronize_threads("synchronize_threads", "If set to true, the synchronization of the threads in the scheduler is active (default: false)", false);

//--------------------------------- SchedulerWorker ---------------------------------------------

/** Central stack of exceptions possibly occurring in threads,
 * which are passed to the main thread. */
static std::vector<std::exception_ptr> worker_exceptions;

void SchedulerWorker::run() {
	try {
		pop.update(1,pop[0]);	// Initialize pop[1] with a copy of pop[0]
		random_seed(threadSeed); // initialize thread's random seed

		if (!scheduler->terminate())
			for(;;) {
				scheduler->checkPopulation();

				// if thread synchronization is active and not all threads
				// having a lower id are runinng, yet, then wait
				if(scheduler->_synchronize_threads && id > 0) {
					std::unique_lock<std::mutex> lck(scheduler->mutexOrderThreads);
					while(!scheduler->workers[id-1]->hasStarted)
						scheduler->cvOrderThreads.wait(lck);
				}

				// 	schedule the next method
				bool wait = false;				// indicates if the thread needs to wait for another thread to finish
				bool terminateThread = false;	// indicates if the termination has been initiated

				do {
					if(wait) { // need to wait for other threads, block until notified
						std::unique_lock<std::mutex> lck(scheduler->mutexNoMethodAvailable);
						scheduler->cvNoMethodAvailable.wait(lck);
						if(scheduler->terminate()) {	// if the termination is in progress, terminate also this thread!
							terminateThread = true;
							break;
						}
					}

					scheduler->mutex.lock(); // Begin of atomic operation
					scheduler->getNextMethod(this);	// try to find an available method for scheduling
					// if thread synchronization is active and this thread has not started, yet,
					// set the hasStarted flag to true and notify possibly waiting threads
					if(scheduler->_synchronize_threads && !hasStarted) {
						scheduler->mutexOrderThreads.lock();
						hasStarted = true;
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

				if (terminateThread) // if in the meanwhile, termination has been started, terminate this thread as well
					break;
#ifdef DEBUGMETH
				cout << *tmpSol << endl;
#endif

				// if the threads should be synchronized and this thread's time budget has been used up...
				if(scheduler->_synchronize_threads && timeBudget <= 0) {
					scheduler->mutexNotAllWorkersInPrepPhase.lock();
					// possibly update the global time budget for the next working phase
					if(method != NULL && method->expectedRuntime > scheduler->globalTimeBudget)
						scheduler->globalTimeBudget = method->expectedRuntime;
					scheduler->workersWaiting++; // increment the counter for the waiting workers
					scheduler->mutexNotAllWorkersInPrepPhase.unlock();

					// and this is not the last thread to reach this point, block it
					if(scheduler->workersWaiting < scheduler->_threadsnum) {
						std::unique_lock<std::mutex> lck(scheduler->mutexNotAllWorkersInPrepPhase);
						scheduler->cvNotAllWorkersInPrepPhase.wait(lck);
					}
					// the last thread has reached this point
					else {
						// set the time budget for all worker threads and reset the global variables
						scheduler->updateDataFromResultsVectors(true);
						scheduler->writeLogEntry();
						for(unsigned int i=0; i < scheduler->_threadsnum; i++) {
							if(scheduler->workers[i]->method != NULL)
								scheduler->workers[i]->timeBudget = scheduler->globalTimeBudget;
						}

						scheduler->globalTimeBudget = 0;
						scheduler->workersWaiting = 0;

						scheduler->mutexNotAllWorkersInPrepPhase.lock();
						scheduler->cvNotAllWorkersInPrepPhase.notify_all(); // notify the possibly waiting threads
						scheduler->mutexNotAllWorkersInPrepPhase.unlock();
					}
					if(scheduler->terminate())
						break;
					if(method == NULL)
						continue;
				}

				// run the scheduled method
				// scheduler->perfGenBeginCallback();
				startTime = CPUtime();
				bool tmpSolChanged = method->run(tmpSol);
				// if thread synchronization is active, reduce the time budget for this worker, accordingly
				if(scheduler->_synchronize_threads)
					timeBudget -= method->expectedRuntime;
				double methodTime = CPUtime() - startTime;

				if (tmpSolChanged)
					tmpSolImproved = tmpSol->isBetter(*pop[0]);
				else
					tmpSolImproved = -1;
#ifdef DEBUGMETH
				cout << *tmpSol << "<-" << method->name << " " << tmpSolChanged << "/" << tmpSolImproved << endl;
#endif

				// update statistics and scheduler data
				scheduler->mutex.lock(); // Begin of atomic operation
				scheduler->updateMethodStatistics(this, methodTime);
				scheduler->updateData(this, !scheduler->_synchronize_threads, scheduler->_synchronize_threads);

				scheduler->mutexNoMethodAvailable.lock();
				scheduler->cvNoMethodAvailable.notify_all(); // notify the possibly waiting threads
				scheduler->mutexNoMethodAvailable.unlock();

				// scheduler->perfGenEndCallback();

				if (scheduler->terminate()) {
					// if synchronization of threads is active, ensure that threads potentially
					// blocked at the mutexNotAllWorkersReadyForRunning - mutex are freed
					if(scheduler->_synchronize_threads) {
						scheduler->mutexNotAllWorkersInPrepPhase.lock();
						scheduler->cvNotAllWorkersInPrepPhase.notify_all();
						scheduler->mutexNotAllWorkersInPrepPhase.unlock();
					}

					// write last generation info in any case
					if(!scheduler->_synchronize_threads)
						scheduler->writeLogEntry(true);
					scheduler->mutex.unlock(); // End of atomic operation
					break;	// ... and stop
				}
				else {
					// write generation info
					if(!scheduler->_synchronize_threads)
						scheduler->writeLogEntry();
					scheduler->mutex.unlock(); // End of atomic operation
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


//--------------------------------- Scheduler ---------------------------------------------

Scheduler::Scheduler(pop_base &p, const pstring &pg)
		: mh_advbase(p, pg), callback(NULL), finish(false) {
	initialSolutionExists = false;

	_threadsnum = threadsnum();
	_synchronize_threads = synchronize_threads();
	workersWaiting = 0;
	globalTimeBudget = 0;
}

void Scheduler::run() {
	checkPopulation();

	timStart=CPUtime();

	writeLogHeader();
	writeLogEntry();
	logstr.flush();

	// spawn the worker threads
	for(unsigned int i=0; i < _threadsnum; i++) {
		mutex.lock(); // Begin of atomic operation
		SchedulerWorker *w = new SchedulerWorker(this, i, pop->at(0), random_int(UINT32_MAX));
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

void Scheduler::updateMethodStatistics(SchedulerWorker *worker, double methodTime) {
	int idx=worker->method->idx;
	totTime[idx] += methodTime;
	nIter[idx]++;
	nIteration++;
	// if the applied method was successful, update the success-counter and the total obj-gain
	if (worker->tmpSolImproved == 1) {
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

	double tim=CPUtime();
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
	ostr << "CPU-time:\t" << tim << endl;
	ostr << "iterations:\t" << nIteration << endl;
	//ostr << "local improvements:\t"  << nLocalImprovements << endl;
	printMethodStatistics(ostr);
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

	// perform a construction method
	if (!constheu.empty() && (worker->method == NULL || constheu.hasFurtherMethod())) {
		// either, because there is still a method available that has not been applied, yet.
		if(worker->method != NULL) {
			mh_solution* tmp = worker->tmpSol;
			worker->tmpSol = tmp->createUninitialized();
			worker->tmpSol->initialize(0);
			delete tmp;
		}

		// or because the worker has just been created, apply construction method first.
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
			else
				worker->pop.update(0, pop->at(0)); // yes, then we assign the best known solution and schedule a method to be applied to it.
		}
		worker->method = shakingnh[worker->id]->select();
		if (worker->method != NULL) {
			worker->shakingStartTime = CPUtime();
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
		if(worker->tmpSolImproved == 1) {
			copyBetter(worker, updateSchedulerData);	// save new best solution
			initialSolutionExists = true;
		}
		return;
	}

	if (worker->method->idx < locimpnh[0]->size() + constheu.size()) {
		// local improvement neighborhood has been applied
		if (worker->tmpSolImproved == 1) {
			// improvement achieved, restart with first local improvement method
			copyBetter(worker, updateSchedulerData);	// save new best solution within local improvement
			locimpnh[worker->id]->resetLastMethod();
			return;
		}
		else {
			// unsuccessful local improvement method call
			if (locimpnh[worker->id]->hasFurtherMethod()) {
				// continue VND with next neighborhood and incumbent VND solution
				if (worker->tmpSolImproved == 0)
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
					if (worker->tmpSolImproved == 0)
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
			if (worker->tmpSolImproved == 1) {
				// improvement achieved:
				worker->pop.update(1,worker->pop[0]);
				copyBetter(worker, updateSchedulerData);	// save new best solution
				updateShakingMethodStatistics(worker,true);
				shakingnh[worker->id]->resetLastMethod();
			}
			else {
				// unsuccessful neighborhood method call
				updateShakingMethodStatistics(worker,false);
				if (worker->tmpSolImproved == 0)
					worker->tmpSol->copy(*worker->pop[0]); // restore worker's incumbent
			}
		}
		else {
			// do not update statistics for that method (will be done after local improvement)
			// start available local improvement neighborhoods
			if (worker->tmpSolImproved == 1)
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
	if (best->isBetter(*(pop->at(0))))
		update(0, best);
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
		totTime[idx] += CPUtime() - worker->shakingStartTime;
		nIter[idx]++;
		// if the applied method was successful, update the success-counter and the total obj-gain
		if (improved) {
			nSuccess[idx]++;
			sumGain[idx] += abs(worker->pop[0]->obj() - worker->pop[1]->obj());
		}
	}
}
