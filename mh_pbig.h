/*! \file mh_pbig.h
 \brief Scheduler class implementing a Population-Based Iterated Greedy Algorithm (PBIG).
 */

#ifndef MH_PBIGH
#define MH_PBIG_H

#include "mh_scheduler.h"

namespace mh {


//--------------------------- PBIG ------------------------------

/** Class implementing a Population-Based Iterated Greedy Algorithm on the basis of the
 * Scheduler class. The first method to be added must be a randomized construction
 * heuristic used for initializing the whole population. All further methods
 * are supposed to be destroy-and-recreate methods ordered according to an increasing
 * rate of destruction. At least one destroy-and-recreate method must be added.
 */
class PBIG : public Scheduler {

protected:
	/** A SchedulerMethodSelector for the construction heuristic(s) for initialization. */
	SchedulerMethodSelector *constheu;
	/** A SchedulerMethodSelector for the D&R methods for each solution in the population. */
	std::vector<SchedulerMethodSelector *> destrec;

	std::vector<mh_solution *> pop2;	///< The newly derived population.

public:
	/**
	 * Constructor: Initializes the scheduler.
	 * @param p The population to use.
	 * @param destRecMethods The number of D&R methods to be added and used.
	 */
	PBIG(pop_base &p, int destRecMethods, const std::string &pg = "");

	/** Destructor for deleting dynamically allocated objects. */
	~PBIG();

	/**
	 * Determines the next method to be applied. Parameter idx is specific to the implementations
	 * in derived classes.
	 * The implementation in this class always just returns the first method in the methodPool.
	 */
	std::pair<SchedulerMethod *,SchedulerMethodContext *> getNextMethod(int idx) override;

	/**
	 * Actually performs the PBIG.
	 */
	void run() override;

	/**
	 * Updates the schedulers internal data (e.g., population) in accordance to the last method application.
	 * The parameters are specific to the implementations in derived classes.
	 */
	void updateData(SchedulerMethodResult &tmpSolResult, int idx,
			bool updateSchedulerData, bool storeResult) override;
};

} // end of namespace mh

#endif /* MH_SCHEDULER_H */
