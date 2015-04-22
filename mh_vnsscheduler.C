/*! \file ,h_vnsscheduler.cpp
 */

#include "mh_vnsscheduler.h"


VNSScheduler::VNSScheduler(pop_base &p, bool (*callback)(double), const pstring &pg) :
Scheduler(p, callback, pg) {
}

// TODO Anpassen:

SchedulableMethod* VNSScheduler::getNextMethod() {
	SchedulableMethod* scheduledMethod = _methodPool[k-1];

	K2CSVMHSol* curSol = NULL;
	// for constructing the initial solution operate on a new empty solution
	if(k == 1) {
		curSol = new K2CSVMHSol(problem);
		indSol = 0;	// make this the first solution in the elite set
	}
	// in the neighborhoods operate on the incumbent solution
	else {
		curSol = new K2CSVMHSol(*dynamic_cast<K2CSVMHSol*>(pop->at(0)));
		indSol = 0;	// in the basic vns we always operate on the first solution
		//curSol->makeIndependent();
	}
	curSol->setAlgorithm(this);
	scheduledMethod->alg->setSolution(curSol);

	return scheduledMethod;
}

void VNSScheduler::updateOptimizationData(SchedulableMethod* scheduledMethod, K2CSVMHSol* sol) {
	//** Update the weights **//
	if(dynamic_cast<K2CSVMHSol*>(pop->bestSol())->sheets()->size() == 0) {	// no solution has been constructed, yet
		// ... and no method has been scheduled, yet
		// -> start with the initial construction method
		if(scheduledMethod == NULL)	{
			k = 1;
			return;
		}
	}

	sol->normalize();
	sol->invalidate();

	//** Update the pattern container **//
	//sol->addSheetsToPatternContainer(_pc);

	//** Update the elite set **//
	// check, if the chosen solution could be improved
	if(sol->obj() < pop->at(indSol)->obj()) {
		// statistics (only meaningful for the neighborhoods)
		if(k > 1) {
			nSuccess[k]++;
			sumGain[k] += abs(pop->at(indSol)->obj() - sol->obj());
		}

		delete pop->replace(indSol, sol);
		timGenBest = CPUtime() - timStart;	// update time for best solution
		genBest = nGeneration;				// update generation in which the best solution was found
		k = 2;								// continue with the first neighborhood
	}
	else { // otherwise, continue with the next neighborhood
		// if the last neighborhood was searched start over with the first one
		if (k == kmax) {
			k = 2;
			nFullIter++; // statistics: increase number of full iterations
		}
		else
			k++;
		delete sol;
	}
}

*/
