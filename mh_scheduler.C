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

int_param threadsnum("threadsnum", "Number of threads used in the scheduler", 1, 1, 100);


//--------------------------------- SchedulerWorker ---------------------------------------------

/** Central stack of exceptions possibly occurring in threads,
 * which are passed to the main thread. */
static std::vector<std::exception_ptr> worker_exceptions;

void SchedulerWorker::run() {
	try {
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

					scheduler->getNextMethod(this);	// try to find and available method for scheduling
					if(method == NULL)	// no method could be scheduled -> wait for other threads
						wait = true;
				} while (method == NULL);

				if(terminateThread) // if in the meanwhile, termination has been started, terminate this thread as well
					break;

				// run the scheduled method
				// scheduler->perfGenBeginCallback();
				double startTime = CPUtime();
				bool tmpSolChanged = method->run(tmpSol);
				double methodTime = CPUtime() - startTime;

				if (tmpSolChanged)
					tmpSolImproved = tmpSol->isBetter(*pop[0]);
				else
					tmpSolImproved = -1;

				// update statistics and scheduler data
				scheduler->mutex.lock(); // Begin of atomic action
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
		SchedulerWorker *w = new SchedulerWorker(this, i, pop->at(0));
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
			double(nSuccess[k]/double(sumSuccess)*100.0),
			totTime[k],
			double(totTime[k]/double(sumTime)*100.0));
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
		lastMethod++;
		if (unsigned(lastMethod) == methodList.size())
			if (strategy == MSSequential)
				lastMethod = 0;
			else {
				lastMethod--;
				return NULL;	// MSSequentialOnce
			}
		return scheduler->methodPool[methodList[lastMethod]];
		break;
	case MSRandom:
		return scheduler->methodPool[methodList[random_int(methodList.size())]];
	case MSRandomOnce: {
		int remaining = methodList.size() - numSelected;
		if (remaining == 0)
			return NULL;	// no more methods
		int r = random_int(remaining);
		int i=0;
		while (r>0 || selected[i]) {
			if (!selected[i])
				i++;
		}
		return scheduler->methodPool[methodList[i]];
	}
	case MSSelfadaptive:
		mherror("Selfadaptive strategy in SchedulerMethodSelector::select not yet implemented");
		break;
	default:
		mherror("Invalid strategy in SchedulerMethodSelector::select",tostring(strategy));
	}
	return NULL;
}


//--------------------------------- VNSScheduler ---------------------------------------------

VNSScheduler::VNSScheduler(pop_base &p, unsigned int nconstheu, unsigned int nlocimpnh,
		unsigned int nshakingnh, const pstring &pg) :
		Scheduler(p, pg),
		constheu(this, SchedulerMethodSelector::MSrandom) {
	for (int t=0; t<threadsnum();t++) {
		locimpnh.push_back(new SchedulerMethodSelector(this,
				SchedulerMethodSelector::MSsequential));
		shakingnh.push_back(new SchedulerMethodSelector(this,
				SchedulerMethodSelector::MSsequential));
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
	if (!locimpnh[0]->empty())
		mherror("Local improvement neighborhoods not yet supported in VNSScheduler");
}

void VNSScheduler::copyBetter(SchedulerWorker *worker) {
	worker->pop.update(0, worker->tmpSol);
	if (worker->pop[0]->isBetter(*(pop->at(0))))
		update(0, worker->pop[0]);
}

void VNSScheduler::getNextMethod(SchedulerWorker *worker) {
	// must have the exact number of methods added
	assert(methodPool.size() == constheu.size() + locimpnh[0]->size() + shakingnh[0]->size());

	if (worker->method == NULL) {
		// Worker has just been created, apply construction method first if one exists
		if (!constheu.empty())
			worker->method = constheu.select();
		else
			worker->method = shakingnh[worker->id]->select();
		// Uninitialized solution in the worker's population is fine
	}
	else {
		// shaking neighborhood method has been applied before
		worker->method = shakingnh[worker->id]->select();
	}
}

void VNSScheduler::updateData(SchedulerWorker *worker) {
	if (worker->method->idx < constheu.size()) {
		// construction method has been applied
		copyBetter(worker);	// save new best solution
	}
	else {
		// neighborhood method has been applied
		if (worker->tmpSolImproved == 1) {
			// improvement achieved:
			shakingnh[worker->id]->resetLastMethod();
			copyBetter(worker);	// save new best solution
		}
		else {
			// unsuccessful neighborhood method call
			if (worker->tmpSolImproved == 0)
				worker->tmpSol->copy(*worker->pop[0]); // restore worker's incumbent
		}
	}
}
