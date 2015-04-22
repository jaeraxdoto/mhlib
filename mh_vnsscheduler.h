/*! \file mh_vnsscheduler.h
 \brief Realizes a simple VNS with a fixed order of neighborhoods 
 on the basis of the Scheduler class.
 */

#ifndef MH_VNSSCHEDULER_H_
#define MH_VNSSCHEDULER_H_

#include "mh_scheduler.h"

/**
 * This class implements a simple variable neighborhood search with a fixed order of neighborhoods.
 */
class VNSScheduler : public Scheduler {
protected:

public:
	/**
	 * Constructor: Initializes the scheduler and fills the method pool.
	 */
	VNSScheduler(pop_base &p, const pstring &pg = (pstring) (""));


	/** Cloning is not implemented for this class. */
	virtual VNSScheduler* clone() const {
		mherror("Cloning not implemented in VNSScheduler");
		return NULL;
	}

	/**
	 * Schedules the next method. Initially this is the provided construction method included as first SchedulerMethod.
	 * In every following iteration an improvement method is selected
	 * according to the classical VNS neighborhood selection.
     */
	void getNextMethod(SchedulerWorker *worker);

	/**
	 * Updates the population if a better solution has been found and sets the current
	 * neighborhood accordingly.
	 * If no solution has been computed yet, the construction method (index 0 in the methodPool) shall be scheduled next.
	 * Otherwise, if an improvement to the incumbent solution has been found in the previous
	 * iteration, the first neighborhood shall be scheduled next (index 1). If no improvement
	 * has been found, the next neighborhood shall be scheduled (increment k).
	 TODO Berücksichtigung vom Multithreading! Wie wird das gemacht?
	 */
	virtual void updateOptimizationData(SchedulerWorker *worker);
};

#endif /* MH_VNSSCHEDULER_H_ */
