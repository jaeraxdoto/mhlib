/*! \file mh_scheduler.h
 \brief Abstract scheduler classes for realizing diverse sequential and basic multithreaded
 	 GRASP, IG, VNS, and VLNS approaches.
 */

#ifndef MH_SCHEDULER_H
#define MH_SCHEDULER_H

#include <assert.h>
#include <array>
#include <vector>
#include <string>
#include <set>
#include "mh_advbase.h"
#include "mh_pop.h"
#include "mh_schedmeth.h"

namespace mh {

/** \ingroup param
 * Controls log output of the Scheduler:
 * - 0: no log
 * - 1: normal log
 * - 2: normal log and appended respective method to each log entry.
 */
extern int_param lmethod;

/** The maximum number of possible "embedded" method applications. E.g., in GVNS,
 * the VND's methods are embedded in the outer VNS shaking methods and
 * the number of embedded method applications is 2. The individual VND or VNS
 * methods are scheduled sequentially and therefore do not count as embedded here.
 * Used for calculating separate timings, storing individual incumbent solutions etc.
 */
const int maxStackedMethods=4;


//--------------------------- SchedulerMethodSelector ------------------------------

/**
 * Class for selecting a method out of a subset of alternative SchedulerMethods.
 * Different standard selection strategies such as sequential, uniformly random or self-adaptive
 * are provided. Also contains the contextual data required for the selection.
 * A SchedulerMethodSelector can be associated with a worker thread only or the whole Scheduler.
 * Derive your own class for realizing advanced selection strategies.
 */
class SchedulerMethodSelector {

public:
	/** Strategies for selecting a method from a method pool. */
	enum MethodSelStrat {
		/** Choose one method after the other in the given order, then restarting again with all methods
		 * that have not been disabled by doNotReconsiderLastMethod; return nullptr when no method
		 * remains. */
		MSSequentialRep,
		/** Choose one method after the other, each just once, and then return nullptr. */
		MSSequentialOnce,
		/** Uniform random selection. Methods for which doNotReconsiderLastMethod are disabled
		 * and not reconsidered. When no method remains, nullptr is returned. */
		MSRandomRep,
		/** Uniform random selection, but each just once; finally return nullptr. */
		MSRandomOnce,
		/** Random selection with self-adaptive probabilities. */
		MSSelfadaptive,
		/** Random selection, where the probability is indirect proportional to the time used until now for this method. */
		MSTimeAdaptive
	};

protected:

	class Scheduler *scheduler;				///< Associated Scheduler
	MethodSelStrat strategy;			///< The selection strategy to be used.
	std::vector<int> methodList; 		///< List of Indices of the methods in the methodPool.
	std::vector<SchedulerMethodContext> methodContextList;	///< The MethodContext objects associated with the methods, to be passed when calling a method.

	int lastMethod;			///< Index of last applied method in methodList or -1 if none.
	int firstActiveMethod;	///< Index of first active, i.e., to be considered, methods. If equal to methodList.size(), no further method remains.
	std::set<int> activeSeqRep;	///< Active methods indices in case of MSSequentialRep. Not used by other strategies.
	std::set<int>::iterator lastSeqRep; /// Last method iterator in activeSeqRep. Only used by MSSequtionRep.
	std::vector<double> probabilityWeights; ///< Probability intervals for the methods, only used for adaptive methods

public:

	/** Initialize SchedulerMethodSelector for given strategy. */
	SchedulerMethodSelector(Scheduler *scheduler_, MethodSelStrat strategy_)
		: scheduler(scheduler_), strategy(strategy_), lastMethod(-1), firstActiveMethod(0),
		  lastSeqRep(activeSeqRep.end()) {
	}

	/** Cleanup. */
	virtual ~SchedulerMethodSelector() {
	}

	/** Adds a the method with the given index to the methodList. */
	void add(int idx) {
		methodList.push_back(idx);
		probabilityWeights.push_back(0);
		methodContextList.emplace_back(SchedulerMethodContext());
		if (strategy==MSSequentialRep)
			activeSeqRep.insert(methodList.size()-1);
	}

	/** Returns the number of methods contained in the methodList. */
	int size() const {
		return methodList.size();
	}

	/** Returns true if the methodList is empty. */
	bool empty() const {
		return methodList.empty();
	}

	/** Resets selector, lastMethod is set to none (= -1).
	 * \param hard if true, also disabled methods are enabled and callCounter are reset.
	 * Typically called when a new incumbent solution is obtained to reconsider all methods. */
	void reset(bool hard);

	/** Mark the last method to not be reconsidered until reset() is called. */
	void doNotReconsiderLastMethod();

	/* Returns true if a previously applied method is known. */
	bool hasLastMethod() {
		return lastMethod != -1;
	}

	/** Selects a method according to the chosen selection strategy from
	 * the methodList.
	 */
	SchedulerMethod *select();

	/** Returns true if at least one more method can be returned. */
	bool hasFurtherMethod() const;

	/** Returns the last selected method or nullptr if none has been selected yet. */
	SchedulerMethod *getLastMethod();

	/** Returns the SchedulerMethodContext object
	 * for the last selected method, to be passed when calling the method. */
	SchedulerMethodContext *getMethodContext() {
		assert(lastMethod >= 0);
		return &methodContextList[lastMethod];
	}
};

/** Pair combining a SchedulerMethod and associated SchedulerMethodContext for performing the method. */
typedef std::pair<SchedulerMethod *,SchedulerMethodContext *> SchedulerMethodAndContext;

//--------------------------- Scheduler ------------------------------

/**
 * The scheduler base class for flexibly realizing GRASP, VNS, VLNS etc. approaches in sequential as well as
 * multithreaded ways. It maintains a methodPool consisting of SchedulerMethods that are iteratively
 * called. The scheduler is in particular responsible for deciding at which point in the optimization which
 * specific method is applied.
 */
class Scheduler : public mh_advbase {
	friend class SchedulerMethodSelector;

protected:
	/** The method pool from which the scheduler chooses the methods to be used. */
	std::vector<SchedulerMethod *> methodPool;

	/* Statistical data on methods */
	std::vector<int> nIter;			///< Number of iterations of the particular methods.
	std::vector<double> totTime;	///< Total time spent running the particular methods.
	std::vector<double> totNetTime;	///< Total netto time spent for the method (e.g., excl. VND in case of shaking)
	std::vector<int> nSuccess;		///< Number of successful iterations of the particular methods.
	std::vector<double> sumGain;	///< Total gain achieved by the particular methods.
	double timFirstStart = 0;		///< Time of first start (relevant when started multiple times for aggregated statistics).

	/**
	 * Optional function pointer to a callback function passed by the interface.
	 * This function (if != nullptr)  is called periodically during the optimization, in particular each time
	 * a method returns.
	 * The objective value of the currently best known solution is passed as an argument and
	 * it returns an integer value that should indicate, if the optimization shall be stopped (1)
	 * or continued (0).
	 */
	bool (*callback)(double);

	/**
	 * Flag that can be set within the scheduler to indicate that the optimization should
	 * be terminated.
	 */
	bool finish;

public:
	/**
	 * Constructor: Initializes the scheduler.
	 */
	Scheduler(pop_base &p, const std::string &pg = "") : mh_advbase(p,pg), callback(nullptr), finish(false) {
	}

	/**
	 * Destructor, deletes the used methodPool and the solution objects.
	 */
	virtual ~Scheduler() {
		// clean up method pool
		for(auto method : methodPool)
			delete method;
	}

	/* Set a callback method, which is then periodically called with the currently best objective value
	 * during the optimization, whenever a method returns.  If it returns 1 the optimization will stop.
	 * Initially, no callback method is set, i.e., callback=nullptr. */
	void setCallback(bool (*_callback)(double) = nullptr) {
		callback = _callback;
	}

	/**
	 * Adds a new schedulable method to the scheduler's method pool
	 * and add corresponding initial values in the data structure used for the method statistics.
	 * These SchedulerMethod objects are assumed to belong to the Scheduler and will finally
	 * be deleted by its destructor.
	 */
	void addSchedulerMethod(SchedulerMethod* method) {
		method->idx = methodPool.size();
		methodPool.push_back(method);
		nIter.push_back(0);
		totTime.push_back(0);
		totNetTime.push_back(0);
		nSuccess.push_back(0);
		sumGain.push_back(0);
	}

	/**
	 * Initiates the scheduling and runs the optimization. Here, only a very simple local
	 * search with the first method is performed.
	 */
	void run() override;

	/**
	 * This method may be called to reset the scheduler for a new run.
	 * Statistics data will be aggregated over the runs, but the next run will be entirely
	 * independent.
	 */
	void reset() override;

	/**
	 * This method does here nothing and is only implemented since it is required by the underlying
	 * base class mh_advbase.
	 */
	void performIteration() override {
		mherror("Scheduler does not implement/use performGeneration");
	}

	/**
	 * Returns true, if the external application has requested the optimization to terminate,
	 * if the scheduler's terminate flag has been set to true,
	 * or if any of the given termination criteria applies.
	 * Is called within the SchedulerWorker's run method to stop the respective thread when
	 * termination is requested.
	 * As this method is called rather often and to avoid unnecessary, potentially expensive checks
	 * for all termination criteria, the scheduler's termination flag is set to true,
	 * if any criterion applies.
	 */
	bool terminate() override;

	/** Function that can be overriden for achieving mutual exclusion in the next block. */
	virtual void mutexLock() {
	}

	/** Function that can be overriden for ending mutual exclusive block. */
	virtual void mutexUnlock() {
	}

	/**
	 * Returns true, if the external application has requested the optimization to terminate,
	 * if the scheduler's terminate flag has been set to true,
	 * or if the ttime termination criterion applies.
	 * Can be called within time expensive Scheduler methods to stop when termination
	 * was requested or the time is up. The method is thread-safe
	 */
	virtual bool terminateMethod();

	/** Updates the statistics data after applying a method. */
	void updateMethodStatistics(mh_solution *origsol, mh_solution *tmpsol, int methodIdx,
			double methodTime, SchedulerMethodResult &tmpSolResult);

	/**
	 * Prints more detailed statistics on the methods used by the scheduler.
	 * The output contains the number of iterations used for each method, the number of
	 * successful iterations, the total and average gain in objective value the method yielded,
	 * and the total, relative, and total netto times spent with applying the method.
	 * The total netto times normally correspond to the total times. Just in case of
	 * methods that include actions one may consider as auxiliary,
	 * like in shaking neighborhoods the automatically applied VND, the total netto time
	 * reports the total times excluding these further actions. It is the responsibility of a
	 * derived class (like GVNSScheduler) to set the total netto time to other values
	 * than the total time.
	 */
	virtual void printMethodStatistics(std::ostream &ostr);

	/**
	 * Prints general statistics on the optimization.
	 * In particular, the total runtime of the algorithm, the best found objective value, and
	 * in which iteration and after how much time it was found.
	 */
	void printStatistics(std::ostream &ostr) override;

	/**
	 * Writing the log header.
	 */
	virtual void writeLogHeader(bool finishEntry=true) override;

	/**
	 * Standard method for writing the log entry.
	 * It calls the new extended method with "-" as method name.
	 */
	virtual bool writeLogEntry(bool inAnyCase=false, bool finishEntry=true) override {
		return writeLogEntry(inAnyCase,finishEntry,"-");
	}

	/**
	 * Write the log entry including the given method name according to lmethod.
	 */
	virtual bool writeLogEntry(bool inAnyCase, bool finishEntry, const std::string &method);

	/** Add the statistical data collected for the methods in another Scheduler object
	 * with the same methods to the current one. Used when running multiple Scheduler objects in parallel
	 * threads.
	 */
	void addStatistics(const Scheduler &s);

	/**
	 * Determines the next method to be applied together with its associated SchedulerMethodContext.
	 * nullptr might be returned as SchedulerMethodContext if not a specific one shall be used.
	 * Parameter idx is specific to the implementations in derived classes.
	 * The implementation in this class always just returns the first method in the methodPool.
	 */
	virtual SchedulerMethodAndContext getNextMethod(int idx);

	/**
	 * Updates the schedulers internal data (e.g., population) in accordance to the last method application.
	 * The parameters are specific to the implementations in derived classes.
	 */
	virtual void updateData(SchedulerMethodResult &tmpSolResult, int idx,
			bool updateSchedulerData, bool storeResult);
};

} // end of namespace mh

#endif /* MH_SCHEDULER_H */
