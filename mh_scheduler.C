// mh_scheduler.C

#include <float.h>
#include <stdio.h>

#include "mh_scheduler.h"
#include "mh_random.h"
#include "mh_util.h"
#include "mh_c11threads.h"
#include <future>
#include <chrono>


int_param numthreads("numthreads", "Maximum number of threads used in the scheduler", 1, 1, 100);



//--------------------------------- SchedulerWorker ---------------------------------------------

void SchedulerWorker::run() {
		thread = std::thread(&Scheduler::runWorker, scheduler, this);
}



//--------------------------------- Scheduler ---------------------------------------------

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
		SchedulerWorker *w = new SchedulerWorker(this);
		workers.push_back(w);
		w->run();
	}

	// wait for the threads to finish and delete them
	for(auto w : workers) {
		w->thread.join();
		delete w;
	}

	logstr.emptyEntry();
	logstr.flush();
}

void Scheduler::runWorker(SchedulerWorker *worker) {
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
			worker->method->run(worker->solution);

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

	mh_solution* curSol = NULL;
	// if the method is an improvement method that needs to operate on an existing solution,
	// select one from the population.
	// TODO: more meaningful selection of solution to which the method is applied.
	// TODO: WICHTIG: solutions nie neu anlegen sondern aus dem Pool der vorhandenen zu entfernende "recyclen"!
	//				Siehe andere in der mhlib implementierete Algorithmen
	curSol = pop->at(random_int(pop->size()))->clone();

	worker->method = scheduledMethod;
	worker->solution = curSol;
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
		bool imp = methodPool[k]->improvement;
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
	//const mh_solution *best=pop->bestSol();
	ostr << "# best solution:" << endl;
	sprintf( s, nformat(pgroup).c_str(), pop->bestObj() );
	ostr << "best objective value:\t" << s << endl;
	ostr << "best obtained in iteration:\t" << genBest << endl;
	sprintf( s, nformat(pgroup).c_str(), timGenBest );
	ostr << "solution time for best:\t" << timGenBest << endl;
	//ostr << "best chromosome:\t";
	//best->write(ostr,0);
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
		k=2;
	}
	else
		worker->method = methodPool[1];
	worker->solution = pop->at(0)->clone();
}

void VNSScheduler::updateSchedulerData(SchedulerWorker* worker) {
	// TODO Simple, quick hack!! Just performs simple local search
	// Update the first solution in the population if new solution is better
	if (worker->solution->isBetter(*pop->at(0))) {
		// statistics (only meaningful for the neighborhoods)
		nSuccess[1]++;
		sumGain[1] += abs(pop->at(0)->obj() - worker->solution->obj());
		delete pop->replace(0, worker->solution);
		timGenBest = CPUtime() - timStart;	// update time for best solution
		genBest = nGeneration;				// update generation in which the best solution was found
	}
	else
		delete worker->solution;
}




