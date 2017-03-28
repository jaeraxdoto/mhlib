/*! \file mh_gvns.h
 \brief A flexible Generalized Variable Neighborhood Search framework based on the parallel general scheduler
 implemented in mh_parscheduler.h. This framework is also suitable for realizing GRASP, VLNS, ILS, IG
 approaches.
 */

#ifndef MH_GVNS_H
#define MH_GVNS_H

#include "mh_parscheduler.h"

namespace mh {

/** \ingroup param
 * GVNS selection strategy for local search neighborhoods.
 * 	 * 0 MSSequentialRep: choose one after the other in the given order, then restarting again with first
	 * 1 MSSequentialOnce: choose one after the other, each just once, and then return nullptr
	 * 2 MSRandomRep: uniform random selection with repetitions
	 * 3 MSRandomOnce: uniform random selection, but each just once; finally return nullptr
	 * 4 MSSelfadaptive: random selection with self-adaptive probabilities
	 * 5 MSTimeAdaptive: random selection with time-adaptive probabilities (probabilities indirect proportional to used time)*/
extern int_param schlisel;


/** \ingroup param
 * GVNS selection strategy for shaking neighborhoods.
	 * 0 MSSequentialRep: choose one after the other in the given order, then restarting again with first
	 * 1 MSSequentialOnce: choose one after the other, each just once, and then return nullptr
	 * 2 MSRandomRep: uniform random selection with repetitions
	 * 3 MSRandomOnce: uniform random selection, but each just once; finally return nullptr
	 * 4 MSSelfadaptive: random selection with self-adaptive probabilities
	 * 5 MSTimeAdaptive: random selection with time-adaptive probabilities (probabilities indirect proportional to used time)*/
extern int_param schshasel;

/** \ingroup param
 * GVNS: If set, the local improvement methods are repeatedly performed until a locally optimum solution
 * is reached, as in classical VND. Otherwise, each local improvement neighborhood is only
 * performed once for each constructed solution and within each shaking iteration.
 */
extern bool_param schlirep;

//--------------------------- GVNS ------------------------------

/**
 * A flexible Generalized Variable Neighborhood Search framework based on the general scheduler
 * implemented in mh_scheduler.h. This framework is also suitable for realizing GRASP, VLNS, ILS, IG
 * approaches. An arbitrary number of construction heuristics, local improvement methods,
 * and shaking (or large neighborhood search methods) can be applied.
 * For each of this three categories of methods, it can be chosen whether the
 * different methods are applied in a strictly sequential way, uniform random way,
 * or random with with self-adaptive application probabilities.
 * Each worker thread performs an independent VNS, migration is performed at major iterations,
 * the overall best solution is adopted in the main population.
 */
class GVNS : public ParScheduler {

protected:
	SchedulerMethodSelector *constheu;	///< Selector for construction heuristic methods
	std::vector<SchedulerMethodSelector *> locimpnh;	///< Selectors for local improvement neighborhood methods for each worker
	std::vector<SchedulerMethodSelector *> shakingnh;	///< Selectors for shaking/LNS methods for each worker

	/**
	 * Indicates whether a construction method has already been scheduled and executed before,
	 * i.e. some solution that can be used as an initial solution for improvement methods already
	 * exists.
	 */
	bool initialSolutionExists;

	int _schlisel=schlisel(pgroup);	///< Mirrored mhlib parameter #schlisel.
	bool _schlirep=schlirep(pgroup); ///< Mirrored mhlib parameter #schlirep.
	int _schshasel=schshasel(pgroup);	///< Mirrored mhlib parameter #schshasel.

	/* Dynamically creates a selector for the construction heuristic methods. */
	virtual SchedulerMethodSelector *createSelector_constheu();
	/* Dynamically creates a selector for the local improvement methods. */
	virtual SchedulerMethodSelector *createSelector_locimpnh();
	/* Dynamically creates a selector for the shaking/LNS methods.*/
	virtual SchedulerMethodSelector *createSelector_shakingnh();

	/**
	 * An improved solution has been obtained by a method and is stored in tmpSol.
	 * This method updates worker->pop[0] holding the worker's so far best solution and,
	 * if updateSchedulerData is set to true, possibly the Scheduler's global best solution at pop[0].
	 */
	void copyBetter(SchedulerWorker *worker, bool updateSchedulerData);

public:
	/**
	 * Constructor: Initializes the VNSScheduler. Construction, improvement and shaking methods
	 * are then added by addSchedulerMethod, whereas nconstheu>=0 construction heuristics
	 * must come first, followed nlocimpnh>=0 local improvement heuristics, and
	 * finally nshakingnh shaking or large neighborhood search neighborhoods.
	 */
	GVNS(pop_base &p, int nconstheu, int nlocimpnh, int nshakingnh, const std::string &pg = "");

	/** Cloning is not implemented for this class. */
	virtual GVNS* clone() const {
		mherror("Cloning not implemented in GVNS");
		return nullptr;
	}

	/** Cleanup: delete SchedulerMethodSelectors. */
	~GVNS() {
		delete constheu;
		for (int t=0;t<_schthreads;t++) {
			delete locimpnh[t];
			delete shakingnh[t];
		}
	}

	/**
	 * Schedules the next method according to the general VNS scheme, i.e., with the VND embedded
	 * in the VNS. If multiple construction heuristics exist, it is ensured that first all of them are
	 * applied in the order they are defined. After each construction heuristic has been executed,
	 * the selection follows the order of the shaking and local improvement methods defined in the VNS
	 * and the embedded VND.
	 * If currently nothing further can be done, possibly because other threads have to
	 * finish first, the method pointer in worker is set to nullptr and nothing further is changed.
	 * This method has to be always called in an exclusive way,
	 * i.e., mutex.lock() must be done outside.
     */
	void getNextMethod(SchedulerWorker *worker) override;

	/**
	 * Updates the tmpSol, worker->pop and, if updateSchedulerData is set to true, the scheduler's
	 * population according to the result of the last method application.
	 * Furthermore, the worker's incumbent is updated to the global best one with probability
	 * #schpmig.
	 * As the exact history of results is irrelevant to the GVNS, the value of storeResult
	 * is ignored and no result information is appended to the vector's result list.
	 * This method is called with mutex locked.
	 */
	void updateData(SchedulerWorker *worker, bool updateSchedulerData, bool storeResult) override;

	/** Procedure that is called from updateData before a solution obtained from a construction method
	 * is actually accepted as new incumbent. The default implementation does nothing. Can be used
	 * for postprocessing or cleaning up the solution.
	 */
	virtual void preAcceptConstructionSolHook(mh_solution *sol) {
	}

	/** Procedure that is called from updateData before a solution obtained from a local improvement method
	 * is actually accepted as new incumbent. The default implementation does nothing. Can be used
	 * for postprocessing or cleaning up the solution.
	 */
	virtual void preAcceptLocImpSolHook(mh_solution *sol) {
	}

	/** Procedure that is called from updateData before a solution obtained from a shaking method
	 * is actually accepted as new incumbent. The default implementation does nothing. Can be used
	 * for postprocessing or cleaning up the solution.
	 */
	virtual void preAcceptShakingSolHook(mh_solution *sol) {
	}

	/**
	 * Updates the the scheduler's population in case the best incumbent solution among all workers
	 * is better than the best solution stored in the scheduler's population.
	 * As the exact history of results is irrelevant to the GVNS and no results are ever stored,
	 * the value of clearResults is ignored and the results vectors are not cleared.
	 */
	void updateDataFromResultsVectors(bool clearResults);


	/** Procedure that is called from updateDataFromResultsVectors before a solution obtained from a shaking method
	 * is actually accepted as new incumbent. The default implementation does nothing. Can be used
	 * for postprocessing or cleaning up the solution.
	 */
	virtual void preAcceptFromResultsVectorsHook(mh_solution *sol) {
	}

	/**
	 * Updates the statistics data after applying a method in worker.
	 * The special aspect here is that method times and success rates of shaking neighborhoods
	 * consider the embedded local improvement and they are therefore not done here but in the separate
	 * method updateShakingMethodStatistics.
	 * @param worker current worker object
	 * @param methodTime CPU time used by the method call
	 */
	void updateMethodStatistics(SchedulerWorker *worker, double methodTime);

	/**
	 * Separate statistics update for shaking methods, which is called after performing
	 * a full local improvement.
	 */
	void updateShakingMethodStatistics(SchedulerWorker *worker, bool improved);

	/**
   	 * This method may be called to reset the scheduler for a new run.
	 * Statistics data will be aggregated over the runs, but the next run will be entirely
	 * independent.
	 */
	void reset() override;

};

} // end of namespace mh

#endif /* MH_GVNS_H */
