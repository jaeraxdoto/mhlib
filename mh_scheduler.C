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

					scheduler->mutex.lock(); 		// Begin of atomic operation
					scheduler->getNextMethod(this);	// try to find and available method for scheduling
					if(method == NULL)	// no method could be scheduled -> wait for other threads
						wait = true;
					scheduler->mutex.unlock(); 		// End of atomic operation
				} while (method == NULL);

				if(terminateThread) // if in the meanwhile, termination has been started, terminate this thread as well
					break;

				// run the scheduled method
				scheduler->perfGenBeginCallback();
				double startTime = CPUtime();
				// copy solution data and run the scheduled method:
				// In the end, the 0th entry of the population is the original solution
				// and the 1st entry is the modified one.
				/* TODO: Copying solutions is in general a relatively expensive operation.
				 * We should think about whether or not this is really always necessary here in
				 * general. An alternative might be that the threads act on individual exclusive
				 * ranges of the scheduler's population. */
				mh_solution* tmp = pop.at(1);
				tmp->copy(*pop.at(0));
				pop.replace(1, tmp);

				bool solchanged = method->run(pop.at(0));
				double methodTime = CPUtime() - startTime;

				scheduler->mutex.lock(); // Begin of atomic action

				// update scheduler data
				scheduler->updateMethodStatistics(this, methodTime, solchanged);
				scheduler->updateData(this, solchanged);

				scheduler->cvNoMethodAvailable.notify_all(); // notify the possibly waiting threads

				scheduler->perfGenEndCallback();

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
		SchedulerWorker *w = new SchedulerWorker(this, *pop->at(0));
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

void Scheduler::updateMethodStatistics(SchedulerWorker *worker, double methodTime,
		bool solchanged) {
	int idx=worker->method->idx;
	totTime[idx] += methodTime;
	nIter[idx]++;
	nGeneration++;
	// if the applied method was successful, update the success-counter and the total obj-gain
	// TODO: only meaningful for improvement methods?
	if(worker->pop.at(1)->isBetter(*worker->pop.at(0))) {
		nSuccess[idx]++;
		sumGain[idx] += abs(worker->pop.at(1)->obj() - worker->pop.at(0)->obj());
	}
}

void Scheduler::printMethodStatistics(ostream &ostr) {
	ostr << endl << "Scheduler method statistics:" << endl;
	int sumSuccess=0,sumIter=0;
	double sumTime = 0;
	for (int k=0;k<methodPool.size();k++) {
		sumSuccess+=nSuccess[k];
		sumIter+=nIter[k];
		sumTime+= totTime[k];
	}
	ostr << "total num of iterations:\t" << sumIter << endl;
	ostr << "total num of successful iterations:\t" << sumSuccess << endl;
	ostr << "method\titerations\tsuccessful\tsuccess rate\ttotal obj-gain\tavg obj-gain\t rel success\ttotal time\t rel time" << endl;
	for (int k = 0; k < methodPool.size(); k++) {
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

void VNSScheduler::getNextMethod(SchedulerWorker *worker) {
	if (worker->method == NULL) {
		// Worker has just been created, apply construction method first
		worker->method = methodPool[0];
		// Uninitialized solution in the worker's population is fine
	}
	else {
		if (worker->)
	}





	int k=-1;
	// if no initial solution exists, yet, we must select a construction method.
	if(!initialSolutionExists) {
		for(unsigned int i=0; i < methodPool.size(); i++) {
			if(!methodPool[i]->improvement) {
				// prevent deterministic construction methods from being called more than once
				if(methodPool[i]->deterministic &&
						(nIter[methodPool[i]->idx] > 0 || 		// method has been called before
						methodPool[i]->scheduledCounter > 0 )) 	// method is currently scheduled for another thread
					continue;
				else {
					k = i;
					break;
				}
			}
		}
		if(k == -1) {	// no suitable method has been found
			worker->method = NULL;
			return;
		}
	}
	// otherwise, select the current neighborhood, according to the predefined order
	else {
		// TODO: currently, select always the first neighborhood
		k = curMethodIndices[worker->id];
		// if this is not an improvement method, go to the next method that is.
		if(!methodPool[k]->improvement) {
			while(!methodPool[k]->improvement) {
				k++;
				if((unsigned)k == methodPool.size())
					k=0;
			}

			// if we reached again the first method that was tried, there is no method to be currently scheduled
			if((unsigned)k == curMethodIndices[worker->id]) {
				worker->method = NULL;
				return;
			}
			curMethodIndices[worker->id] = k; // update the worker's current neighborhood idx accordingly
		}
	}

	// for constructing a new solution, operate on a new empty  solution:
	if(!methodPool[k]->improvement) {
		mh_solution* tmp = worker->pop.at(0);
		tmp->copy(*pop->at(0)->createUninitialized());
		worker->pop.replace(0, tmp);
	}

	// otherwise, no new solution assignment is necessary, keep working on
	// current solution
	// else {}
	worker->method = methodPool[k];
}

void VNSScheduler::copyBetter(SchedulerWorker *worker) {
	worker->pop->at(1)->copy(*worker->pop->at(0));
	if (worker->pop->at(1)->isBetter(pop->at(0)))
		pop->at(0)->copy(worker->pop->at(1));
}

void VNSScheduler::updateData(SchedulerWorker *worker, bool solchanged) {
	assert(methodPool.size()>=2);
	if (worker->method->idx == 0) {
		// construction method has been applied,
		// just save new solution also in position 1 (= so far best solution of worker)
		copyBetter(worker);
	}
	else {
		// neighborhood method has been applied
		if (solchanged && worker->pop->at(0).isbetter(worker->pop->at(1))) {
			// improvement achieved:
			// copy new best sol to pos 1 and restart with first neighborhood
			copyBetter(worker);
			worker->method = methodPool[1];
		}
		else {
			// unsuccessful neighborhood method call
			if (solchanged)
				worker->pop->at(1)->copy(*worker->pop->at(0)); // restore old solution
			// go to next neighborhood
			int idx = worker->medhod->idx + 1;
			if (idx ==  methodPool.size())
				idx = 1;	// after last neighborhood start again with first
		}
	}
}




/*
void Scheduler::getNextMethod(SchedulerWorker *worker) {
	vector<SchedulableMethod*> allowedMethods;
	allowedMethods.reserve(methodPool.size());	// methods currently allowed for scheduling
	unsigned int sum = 0;
	for (auto sm : methodPool) {
		// only allow methods having a weight greater than 0
		if (sm->weight == 0)
			continue;

		// TODO: for multithreading:
		// only allow methods that are not already scheduled at the moment;
		// maintain data structure of active threads!

		allowedMethods.push_back(sm);
		sum += sm->weight;
	}
	if (allowedMethods.empty()) { // no more method to schedule
		worker = NULL;
		return;
	}

	// roulette-wheel selection from the allowed methods (proportional to the weight)
	int rand = random_int(1, sum);
	unsigned int i=0;
	while(rand > 0) {
		rand -= allowedMethods[i]->weight;
		i++;
	}
	SchedulableMethod* scheduledMethod = allowedMethods[i-1];

	// if the method is an improvement method that needs to operate on an existing solution,
	// select one from the population.
	// TODO: more meaningful selection of solution to which the method is applied.
	// In general, the worker should keep its current working solution, whenever possible.
	mh_solution* tmp = worker->pop.at(0);
	tmp->copy(*pop->at(random_int(pop->size())));
	worker->pop.replace(0, tmp);

	worker->method = scheduledMethod;
}
*/




