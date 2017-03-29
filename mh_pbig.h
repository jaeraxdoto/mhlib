/*! \file mh_pbig.h
 \brief Scheduler class imlementing a Population-Based Iterated Greedy Algorithm (PBIG).
 */

#ifndef MH_PBIGH
#define MH_PBIG_H

#include "mh_scheduler.h"

namespace mh {


//--------------------------- PBIG ------------------------------

/** Class implementing a Population-Based Iterated Greedy Algorithm on the basis of the
 * Scheduler class.
 */
class PBIG : public Scheduler {

protected:

public:
	/**
	 * Constructor: Initializes the scheduler.
	 */
	PBIG(pop_base &p, const std::string &pg = "");

	/**
	 * Determines the next method to be applied. Parameter idx is specific to the implementations
	 * in derived classes.
	 * The implementation in this class always just returns the first method in the methodPool.
	 */
	SchedulerMethod *getNextMethod(int idx) override;

	/**
	 * Updates the schedulers internal data (e.g., population) in accordance to the last method application.
	 * The parameters are specific to the implementations in derived classes.
	 */
	void updateData(SchedulerMethodResult &tmpSolResult, int idx,
			bool updateSchedulerData, bool storeResult) override;
};

} // end of namespace mh

#endif /* MH_SCHEDULER_H */
