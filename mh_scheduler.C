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

// TODO macht dieser Parameter Sinn? Es gibt popsize und Parameter-Groups...
int_param threadspsize("threadspsize", "Size of the population associated with each thread in the scheduler", 2, 2, 100);

//--------------------------------- SchedulerWorker ---------------------------------------------

/** Central stack of exceptions possibly occurring in threads,
 * which are passed to the main thread. */
static std::vector<std::exception_ptr> worker_exceptions;

void SchedulerWorker::run() {
	try {
		if (!scheduler->terminate())
			for(;;) {
				//performGeneration();

				scheduler->checkPopulation();

				// 	schedule the next method
				scheduler->mutex.lock(); 	// Begin of atomic action
				scheduler->perfGenBeginCallback();
				scheduler->getNextMethod(this);
				// if there is no method left to be scheduled in a meaningful way stop thread
				if (method == NULL) {
					scheduler->mutex.unlock(); // End of atomic action
					return;
				}
				scheduler->mutex.unlock(); // End of atomic action

				// run the scheduled method
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

				method->run(pop.at(1));
				double methodTime = CPUtime() - startTime;

				scheduler->mutex.lock(); // Begin of atomic action


				// update scheduler data
				scheduler->updateMethodStatistics(this,methodTime);
				scheduler->updateData(this);

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

// TODO: Achtung: Soll die Population hashing verwenden? -> momentan abhängig von dupelim

Scheduler::Scheduler(pop_base &p, const pstring &pg)
		: mh_advbase(p, pg), callback(NULL), finish(false) {
}

void Scheduler::run() {
	// initialize the optimization data and run the scheduler
	// updateData(NULL);	// TODO Ist ein call mit NULL zur Initialisierung wirklich sinnvoll?

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

// TODO Hier fehlt doch das Updaten des obj-Gains bzw. der Erfolgsrate, oder?
void Scheduler::updateMethodStatistics(SchedulerWorker *worker, double methodTime) {
	int idx=worker->method->idx;
	totTime[idx] += methodTime;
	nIter[idx]++;
	nGeneration++;
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

// TODO Ich meine, dass wir eine generische VNS-Scheduler Klasse vermeinde sollten und diese
// Methoden besser direkt in den Scheduler integrieren, evtl. alternative Methoden zum umschalten

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
		mh_solution* tmp = worker->pop.at(0);
		tmp->copy(*pop->at(0));
		worker->pop.replace(0, tmp);
		k=2;
	}
	// otherwise, no new solution assignment is necessary, keep working on
	// current solution
	else
		worker->method = methodPool[1];
}

void VNSScheduler::updateData(SchedulerWorker* worker) {
	// TODO Simple, quick hack!! Just performs simple local search

	// Determine, if an improvement could be achieved by the method applied by the worker
	if(worker->pop.at(1)->isBetter(*worker->pop.at(0))) {	// method was successful
		// update statistics (only meaningful for the neighborhoods)
		nSuccess[1]++;
		sumGain[1] += abs(worker->pop.at(1)->obj() - worker->pop.at(0)->obj());
		// the improved solutions is this worker's new working solution
		mh_solution* tmp = worker->pop.at(0);
		worker->pop.replace(0, worker->pop.at(1));
		worker->pop.replace(1, tmp);

		// update the first solution in the population if new solution is better
		if (worker->pop.at(0)->isBetter(*pop->at(0))) {
			mh_solution* tmp = pop->at(0);
			tmp->copy(*worker->pop.at(0));
			pop->replace(0, tmp);
			timGenBest = CPUtime() - timStart;	// update time for best solution
			genBest = nGeneration;				// update generation in which the best solution was found
		}
	}
	else { // method was not successful

	}
}




