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
#include <future>
#include <chrono>
#include <bits/exception_ptr.h>

int_param numthreads("numthreads", "Maximum number of threads used in the scheduler", 1, 1, 100);

int_param worker_popsize("thread_popsize", "Size of the population associated with each of the worker threads", 2, 2, 100);

//--------------------------------- SchedulerWorker ---------------------------------------------

void SchedulerWorker::run() {
		thread = std::thread(&Scheduler::runWorker, scheduler, this);
}



//--------------------------------- Scheduler ---------------------------------------------

/** Central stack of exceptions possibly occurring in threads,
 * which are passed to the main thread. */
static std::vector<std::exception_ptr> thread_exceptions;

Scheduler::Scheduler(pop_base &p, const pstring &pg)
		: mh_advbase(p, pg), callback(NULL), finish(false) {
}

void Scheduler::run() {
	// initialize the optimization data and run the scheduler
	// updateSchedulerData(NULL);	// TODO Ist ein call mit NULL zur Initialisierung wirklich sinnvoll?

	checkPopulation();

	timStart=CPUtime();

	writeLogHeader();
	writeLogEntry();
	logstr.flush();

	// spawn the worker threads
	unsigned int nthreads=numthreads();
	for(unsigned int i=0; i < nthreads; i++) {
		SchedulerWorker *w = new SchedulerWorker(this, *pop->at(0));
		workers.push_back(w);
		w->run();
	}

	// wait for the threads to finish and delete them
	for(auto w : workers) {
		w->thread.join();
		delete w;
	}
	// handle possibly transferred exceptions
	for (const exception_ptr &ep : thread_exceptions)
		std::rethrow_exception(ep);

	logstr.emptyEntry();
	logstr.flush();
}

void Scheduler::runWorker(SchedulerWorker *worker) {
	try {
		if (!terminate())
			for(;;) {
				//performGeneration();

				checkPopulation();

				// 	schedule the next method
				mutexScheduler.lock(); 	// Begin of atomic action
				perfGenBeginCallback();
				getNextMethod(worker);
				// if there is no method left to be scheduled in a meaningful way stop thread
				if (worker->method == NULL) {
					mutexScheduler.unlock(); // End of atomic action
					return;
				}
				mutexScheduler.unlock(); // End of atomic action

				// run the scheduled method
				double startTime = CPUtime();
				// copy solution data and run the scheduled method:
				// In the end, the 0th entry of the population is the original solution
				// and the 1st entry is the modified one.
				mh_solution* tmp = worker->p.at(1);
				tmp->copy(*worker->p.at(0));
				worker->p.replace(1, tmp);
				worker->method->run(worker->p.at(1));

				// update method statistics
				int idx = worker->method->idx;
				mutexScheduler.lock(); // Begin of atomic action
				totTime[idx] += CPUtime() - startTime;
				nIter[idx]++;
				nGeneration++;

				// update the optimization data
				updateSchedulerData(worker);

				perfGenEndCallback();

				if (terminate()) {
					// write last generation info in any case
					writeLogEntry(true);
					mutexScheduler.unlock(); // End of atomic operation
					break;	// ... and stop
				}
				else {
					// write generation info
					writeLogEntry();
					mutexScheduler.unlock(); // End of atomic operation
				}
			}
	}
	catch (...) {
		// Pass any exceptions to main thread
		mutexScheduler.lock();
		thread_exceptions.push_back(std::current_exception());
		mutexScheduler.unlock();
	}
}

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
	mh_solution* tmp = worker->p.at(0);
	tmp->copy(*pop->at(random_int(pop->size())));
	worker->p.replace(0, tmp);

	worker->method = scheduledMethod;
	return;
}

void Scheduler::printMethodStatistics(ostream &ostr) {
	ostr << endl << "Scheduler method statistics:" << endl;
	int sumSuccess=0,sumIter=0;
	double sumTime = 0;
	for (int k=0;k<numMethods();k++) {
		sumSuccess+=nSuccess[k];
		sumIter+=nIter[k];
		sumTime+= totTime[k];
	}
	ostr << "total num of iterations:\t" << sumIter << endl;
	ostr << "total num of successful iterations:\t" << sumSuccess << endl;
	ostr << "method\titerations\tsuccessful\tsuccess rate\ttotal obj-gain\tavg obj-gain\t rel success\ttotal time\t rel time" << endl;
	for (int k = 0; k < numMethods(); k++) {
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

void VNSScheduler::getNextMethod(SchedulerWorker *worker) {
	// TODO sehr schneller, schlechter Hack!!
	// first, we call the construction method supposed to be at idx 0, then always method 1
	static int k=1;

	// for constructing the initial solution operate on a new empty solution
	if (k == 1) {
		worker->method = methodPool[0];
		// assign a copy of an (empty) solution from the elite set
		mh_solution* tmp = worker->p.at(0);
		tmp->copy(*pop->at(0));
		worker->p.replace(0, tmp);
		k=2;
	}
	// otherwise, no new solution assignment is necessary, keep working on
	// current solution
	else
		worker->method = methodPool[1];
}

void VNSScheduler::updateSchedulerData(SchedulerWorker* worker) {
	// TODO Simple, quick hack!! Just performs simple local search

	// Determine, if an improvement could be achieved by the method applied by the worker
	if(worker->p.at(1)->isBetter(*worker->p.at(0))) {	// method was successful
		// update statistics (only meaningful for the neighborhoods)
		nSuccess[1]++;
		sumGain[1] += abs(worker->p.at(1)->obj() - worker->p.at(0)->obj());
		// the improved solutions is this worker's new working solution
		mh_solution* tmp = worker->p.at(0);
		worker->p.replace(0, worker->p.at(1));
		worker->p.replace(1, tmp);

		// update the first solution in the population if new solution is better
		if (worker->p.at(0)->isBetter(*pop->at(0))) {
			mh_solution* tmp = pop->at(0);
			tmp->copy(*worker->p.at(0));
			pop->replace(0, tmp);
			timGenBest = CPUtime() - timStart;	// update time for best solution
			genBest = nGeneration;				// update generation in which the best solution was found
		}
	}
	else { // method was not successful

	}
}




