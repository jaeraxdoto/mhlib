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
#include <algorithm>

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

int_param lmethod("lmethod","scheduler: 0:no log, 1:normal log, 2:append method name to each entry",2,0,2);


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

	timStart = mhtime(_wctime);

	writeLogHeader();
	writeLogEntry(false,true,"*");
	logstr.flush();

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
		logstr.emptyEntry();
		logstr.flush();
	}
}

void Scheduler::reset() {
	mh_advbase::reset();
	finish = false;
	workersWaiting = 0;
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
		(ttime(pgroup)>=0 && ttime(pgroup)<=(mhtime(_wctime) - timStart))) {
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
	// if the applied method was successful, i.e., is accepted, update the success-counter and the total obj-gain
	if (worker->tmpSolResult.accept) {
		nSuccess[idx]++;
		sumGain[idx] += abs(worker->pop.at(0)->obj() - worker->tmpSol->obj());
	}
}

void Scheduler::printMethodStatistics(ostream &ostr) {
	double totSchedulerTime = mhcputime() - timStart;
	ostr << endl << "Scheduler method statistics:" << endl;
	int sumSuccess=0,sumIter=0;
	double sumTime = 0;
	for (int k=0;k<int(methodPool.size());k++) {
		sumSuccess+=nSuccess[k];
		sumIter+=nIter[k];
		sumTime+=totNetTime[k];
	}
	ostr << "total num of completed iterations:\t" << sumIter << endl;
	ostr << "total num of successful iterations:\t" << sumSuccess << endl;
	ostr << "total netto time:\t" << sumTime << "\ttotal scheduler time:\t" << totSchedulerTime << endl;
	ostr << "method\t   iter\t   succ\tsucc-rate%\ttotal-obj-gain\tavg-obj-gain\trel-succ%\ttotal-time\trel-time%\ttot-net-time\trel-net-time%" << endl;
	for (int k = 0; k < int(methodPool.size()); k++) {
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
	if (!lmethod(pgroup))
		return;
	logstr.headerEntry();
	if (ltime(pgroup))
		logstr.write(_wctime ? "wctime" : "cputime");
	if (lmethod(pgroup)==2)
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
	if (!lmethod(pgroup))
		return false;
	if (logstr.startEntry(nIteration,pop->bestObj(),inAnyCase))
	{
		if (ltime(pgroup))
			logstr.write(mhtime(_wctime));
		if (lmethod(pgroup)==2)
			logstr.write(method);
		if (finishEntry)
			logstr.finishEntry();
		return true;
	}
	return false;
}

void Scheduler::rethrowExceptions() {
	for (const exception_ptr &ep : worker_exceptions)
		std::rethrow_exception(ep);
}


//--------------------------------- SchedulerMethodSelector ---------------------------------------------

SchedulerMethod *SchedulerMethodSelector::select() {
	if (methodList.empty())
		return nullptr;
	switch (strategy) {
	case MSSequentialRep:
		if (activeSeqRep.empty())
			return nullptr;
		if (lastSeqRep == activeSeqRep.end())
			lastSeqRep = activeSeqRep.begin();
		else {
			lastSeqRep++;
			if (lastSeqRep == activeSeqRep.end())
				lastSeqRep = activeSeqRep.begin();
		}
		lastMethod = *lastSeqRep;
		methodContextList[lastMethod].callCounter++;
		return scheduler->methodPool[methodList[lastMethod]];
	case MSSequentialOnce:
		if (lastMethod == size()-1)
			return nullptr;
		else
			lastMethod++;
		methodContextList[lastMethod].callCounter++;
		return scheduler->methodPool[methodList[lastMethod]];
	case MSRandomRep: {
		if (firstActiveMethod == size())
			return nullptr;
		int r = random_int(firstActiveMethod,methodList.size()-1);
		methodContextList[r].callCounter++;
		return scheduler->methodPool[methodList[r]];
	}
	case MSRandomOnce: {
		if (lastMethod == size()-1)
			return nullptr;	// no more methods
		lastMethod++;
		// Choose randomly a not yet selected method and swap it to position lastMethod
		int r = random_int(lastMethod, methodList.size()-1);
		if (r != lastMethod) {
			swap(methodList[lastMethod],methodList[r]);
			swap(methodContextList[lastMethod],methodContextList[r]);
		}
		methodContextList[lastMethod].callCounter++;
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

void SchedulerMethodSelector::reset(bool hard) {
	lastMethod = -1;
	if (hard) { // also enable disabled methods
		firstActiveMethod = 0;
		for (auto &c : methodContextList)
			c.callCounter = 0;
		if (strategy==MSSequentialRep) {
			activeSeqRep.clear();
			for (int i=0; i<size(); i++)
				activeSeqRep.insert(i);
			lastSeqRep=activeSeqRep.end();
		}
	} else { // soft reset
		if (strategy==MSSequentialRep)
			lastSeqRep=activeSeqRep.end();
	}
}

bool SchedulerMethodSelector::hasFurtherMethod() const {
	switch (strategy) {
	case MSSequentialRep:
		return !activeSeqRep.empty();
	case MSRandomRep:
		return firstActiveMethod < size();
	default:
		return lastMethod < size()-1;
	}
}

void SchedulerMethodSelector::doNotReconsiderLastMethod() {
	if (lastMethod==-1) return;
	switch (strategy) {
	case MSSequentialRep: {
		set<int>::iterator t=lastSeqRep; t--;
		activeSeqRep.erase(lastSeqRep);
		lastSeqRep=t;
		break;
	}
	case MSRandomRep: {
		int tmp = methodList[lastMethod];
		methodList[lastMethod]=methodList[firstActiveMethod];
		methodList[firstActiveMethod]=tmp;
		lastMethod=firstActiveMethod;
		firstActiveMethod++;
		break;
	}
	default:
		break;
	}
}

} // end of namespace mh

