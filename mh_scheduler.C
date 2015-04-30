// mh_scheduler.C

#include <float.h>
#include <stdio.h>
#include <vector>
#include <exception>
#include <stdexcept>


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
				bool tmpChromChanged = method->run(tmpChrom);
				double methodTime = CPUtime() - startTime;

				if (tmpChromChanged)
					tmpChromImproved = tmpChrom->isBetter(*pop[0]);
				else
					tmpChromImproved = -1;

				scheduler->mutex.lock(); // Begin of atomic action

				// update scheduler data
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

	// so far only construction and simple improvement methods are considered
	checkPopulation();

	timStart=CPUtime();

	writeLogHeader();
	writeLogEntry();
	logstr.flush();

	// spawn the worker threads
	unsigned int nthreads=threadsnum();
	for(unsigned int i=0; i < nthreads; i++) {
		SchedulerWorker *w = new SchedulerWorker(this, pop->at(0));
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
	nGeneration++;
	// if the applied method was successful, update the success-counter and the total obj-gain
	if (worker->tmpChrom->isBetter(*worker->pop.at(0))) {
		nSuccess[idx]++;
		sumGain[idx] += abs(worker->pop.at(0)->obj() - worker->tmpChrom->obj());
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
	ostr << "best obtained in iteration:\t" << genBest << endl;
	sprintf( s, nformat(pgroup).c_str(), timGenBest );
	ostr << "solution time for best:\t" << timGenBest << endl;
	ostr << "best chromosome:\t";
	best->write(ostr,0);
	ostr << endl;
	ostr << "CPU-time:\t" << tim << endl;
	ostr << "iterations:\t" << nGeneration << endl;
	//ostr << "local improvements:\t"  << nLocalImprovements << endl;
	printMethodStatistics(ostr);
}


//--------------------------------- VNSScheduler ---------------------------------------------


VNSScheduler::VNSScheduler(pop_base &p, const pstring &pg) :
		Scheduler(p, pg) {
}

void VNSScheduler::copyBetter(SchedulerWorker *worker) {
	worker->pop.copy(0,worker->tmpChrom);
	if (worker->pop[0]->isBetter(*(pop->at(0))))
		pop->copy(0,worker->pop[0]);
}

void VNSScheduler::getNextMethod(SchedulerWorker *worker) {
	assert(methodPool.size()>=2);
	if (worker->method == NULL) {
		// Worker has just been created, apply construction method first
		worker->method = methodPool[0];
		// Uninitialized solution in the worker's population is fine
	}
	else {
		// neighborhood method has been applied
		if (worker->tmpChromImproved == 1) {
			// improvement achieved:
			// restart with first neighborhood
			worker->method = methodPool[1];
		}
		else {
			// unsuccessful neighborhood method call
			// go to next neighborhood
			unsigned int idx = worker->method->idx + 1;
			if (idx ==  methodPool.size())
				idx = 1;	// after last neighborhood start again with first
			worker->method = methodPool[idx];
		}
	}
}

void VNSScheduler::updateData(SchedulerWorker *worker) {
	if (worker->method->idx == 0) {
		// construction method has been applied,
		// just save new solution also in position 1 (= so far best solution of worker)
		copyBetter(worker);
	}
	else {
		// neighborhood method has been applied
		if (worker->tmpChromImproved == 1) {
			// improvement achieved:
			// copy new best sol to pos 1 and restart with first neighborhood
			copyBetter(worker);
		}
		else {
			// unsuccessful neighborhood method call
			if (worker->tmpChromImproved == 0)
				tmpChrom->copy(*worker->pop[0]); // restore old solution
		}
	}
}
