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
 * heuristic used for initializing each solution in the population. All further methods
 * are supposed to be destroy-and-recreate (D&R) methods ordered according to an increasing
 * rate of destruction. At least one D&R method must be added.
 * After initialization, PBIG performs generation-wise: D&R is applied to each solution, deriving a
 * new temporary population. The best solutions of the original population and the new temporary
 * population are then adopted as new current population. 
 * Concerning the different D&R methods, a method selector is maintained for each solution, i.e.,
 * each newly created solution starts with the first D&R method, and over the successive generations
 * the further D&R methods are applied until, possibly, the solution is not adopted into the next 
 * generation anymore. After the last D&R method, the method selection always restarts with the
 * first D&R method.
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
