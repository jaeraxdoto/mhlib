// mh_scheduler.C

#include <float.h>
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


//--------------------------------- SchedulerWorker ---------------------------------------------

/** Central stack of exceptions possibly occurring in threads,
 * which are passed to the main thread. */
static std::vector<std::exception_ptr> worker_exceptions;

void SchedulerWorker::run() {
	try {
		pop.update(1,pop[0]);	// Initialize pop[1] with a copy of pop[0]
		if (!scheduler->terminate())
			for(;;) {
				scheduler->checkPopulation();

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
					scheduler->mutex.unlock(); // End of atomic operation

					if(method == NULL)	// no method could be scheduled -> wait for other threads
						wait = true;
				} while (method == NULL);

				if (terminateThread) // if in the meanwhile, termination has been started, terminate this thread as well
					break;
#ifdef DEBUGMETH
				cout << *tmpSol << endl;
#endif
				// run the scheduled method
				// scheduler->perfGenBeginCallback();
				startTime = CPUtime();
				bool tmpSolChanged = method->run(tmpSol);
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
				scheduler->updateData(this);

				scheduler->cvNoMethodAvailable.notify_all(); // notify the possibly waiting threads

				// scheduler->perfGenEndCallback();

				if (scheduler->terminate()) {
					// write last generation info in any case
					scheduler->writeLogEntry(true);
					scheduler->mutex.unlock(); // End of atomic operation
					break;	// ... and stop
				}
				else {
					// write generation info
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
}

void Scheduler::run() {
	checkPopulation();

	timStart=CPUtime();

	writeLogHeader();
	writeLogEntry();
	logstr.flush();

	// spawn the worker threads
	unsigned int nthreads=threadsnum();
	for(unsigned int i=0; i < nthreads; i++) {
		mutex.lock(); // Begin of atomic operation
		SchedulerWorker *w = new SchedulerWorker(this, i, pop->at(0));
		mutex.unlock(); // End of atomic operation
		workers.push_back(w);
		w->thread = std::thread(&SchedulerWorker::run, w);
	}

	// wait for the threads to finish and delete them
	for(auto w : workers) {
		w->thread.join();
		delete w;
	}
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
	for (int t=0; t<threadsnum();t++) {
		locimpnh.push_back(new SchedulerMethodSelector(this,
				SchedulerMethodSelector::MSSequentialOnce));
		shakingnh.push_back(new SchedulerMethodSelector(this,
				SchedulerMethodSelector::MSSequential));
	}
	unsigned int i = 0;
	for (; i < nconstheu; i++)
			constheu.add(i);
	for (; i < nconstheu + nlocimpnh; i++)
		for (int t=0; t<threadsnum();t++)
			locimpnh[t]->add(i);
	for (; i < nconstheu + nlocimpnh + nshakingnh; i++)
		for (int t=0; t<threadsnum();t++)
			shakingnh[t]->add(i);
}

void GVNSScheduler::copyBetter(SchedulerWorker *worker) {
	worker->pop.update(0, worker->tmpSol);
	if (worker->pop[0]->isBetter(*(pop->at(0))))
		update(0, worker->pop[0]);
}

void GVNSScheduler::getNextMethod(SchedulerWorker *worker) {
	// must have the exact number of methods added
	assert(methodPool.size() == constheu.size() + locimpnh[0]->size() + shakingnh[0]->size());

	if (worker->method == NULL && !constheu.empty()) {
		// Worker has just been created, apply construction method first if one exists
		worker->method = constheu.select();
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

void GVNSScheduler::updateData(SchedulerWorker *worker) {
	if (worker->method->idx < constheu.size()) {
		// construction method has been applied
		copyBetter(worker);	// save new best solution
		return;
	}

	if (worker->method->idx < locimpnh[0]->size() + constheu.size()) {
		// local improvement neighborhood has been applied
		if (worker->tmpSolImproved == 1) {
			// improvement achieved, restart with first local improvement method
			copyBetter(worker);	// save new best solution within local improvement
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
				copyBetter(worker);	// save new best solution
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
				copyBetter(worker);	// save new best solution
			else
				// nevertheless store solution after shaking as incumbent of local improvement
				worker->pop.update(0, worker->tmpSol);
		}
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
		totTime[idx] += CPUtime() - worker->shakingStartTime;
		nIter[idx]++;
		// if the applied method was successful, update the success-counter and the total obj-gain
		if (improved) {
			nSuccess[idx]++;
			sumGain[idx] += abs(worker->pop[0]->obj() - worker->pop[1]->obj());
		}
	}
}
