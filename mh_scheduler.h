/*! \file mh_scheduler.h
 \brief Flexible scheduler classes for realizing diverse sequential and basic multithreaded
 	 GRASP, VNS, and VLNS approaches.
 */

#ifndef MH_SCHEDULER_H
#define MH_SCHEDULER_H

#include <assert.h>
#include <vector>
#include <string>
#include "mh_advbase.h"
#include "mh_pop.h"
#include "mh_c11threads.h"
#include "mh_schedmeth.h"

namespace mh {

/** \ingroup param
 * Sets the maximum number of parallel worker threads to be used by a scheduler instance.
 */
extern int_param schthreads;

/** \ingroup param
 * If set to true, the synchronization of the threads in the scheduler is active (default: false).
 * In particular, the optimization will consist of two repeating phases:
 * The actual working phase, where the threads run the currently scheduled methods in parallel,
 * and the synchronization phase, where threads wait until every thread is finished before starting
 * the next working phase.
 * Only if all threads are in the synchronization phase,the scheduler'data is updated.
 * No (global) updates happen during the working phase.
 */
extern bool_param schsync;

/** \ingroup param
 * Migration probability for a thread in the scheduler to update its incumbent solution after a major
 * shaking iteration by copying the global best solution.
 * If the best global solution is better than the incumbent solution,
 * it becomes the thread's new incumbent solution.
 */
extern double_param schpmig;

/** \ingroup param
 * If set then the Scheduler's method name is also appended to each log entry.
 */
extern bool_param lmethod;

/** \ingroup param
 * GVNS selection strategy for local search neighborhoods.
 * 	 * 0 MSSequentialRep: choose one after the other in the given order, then restarting again with first
	 * 1 MSSequentialOnce: choose one after the other, each just once, and then return nullptr
	 * 2 MSRandomRep: uniform random selection with repetitions
	 * 3 MSRandomOnce: uniform random selection, but each just once; finally return nullptr
	 * 4 MSSelfadaptive: random selection with self-adaptive probabilities */
extern int_param schlisel;

/** \ingroup param
 * GVNS: If set, the local improvement methods are repeatedly performed until a locally optimum solution
 * is reached, as in classical VND. Otherwise, each local improvement neighborhood is only
 * performed once for each constructed solution and within each shaking iteration.
 */
extern bool_param schlirep;


//--------------------------- MethodApplicationResult ------------------------------

/**
 * This struct stores all the relevant data in the context of the application of a specific method
 * in order to have access to the result later.
 */
struct SchedulerMethodResult {

	/**
	 * Stores the method that has been applied.
	 */
	SchedulerMethod* method;

	/**
	 * Enum for the different outcomes for the objective of a solution after applying
	 * a method in comparison to the original objective value.
	 */
	enum ObjChange {
		OBJ_WORSE = -1,
		OBJ_SAME = 0,
		OBJ_BETTER = 1
	};

	/** Indicates the result of the last method call w.r.t. tmpSol */
	ObjChange solObjChange;

	/**
	 * Stores the absolute difference in the objective values between the incumbent solution and
	 * the one obtained by the application of this method.
	 */
	double objDiff;

	/**
	 * Constructor initializing data.
	 */
	SchedulerMethodResult(SchedulerMethod* _method, ObjChange _solObjChange, double _objDiff) {
		method = _method;
		solObjChange = _solObjChange;
		objDiff = _objDiff;
	}
};

//--------------------------- SchedulerWorker ------------------------------

/**
 * SchedulerWorker that runs as own thread spawned by the scheduler.
 * The class contains in particular pointers to the Scheduler, SchedulerMethod and
 * the workers own population to which the method is to be applied.
 * Furthermore, the class maintains a vector for storing MethodApplicationResult entries.
 * Currently, this vector is only used in the context of thread synchronization to keep track
 * of all results until the update of the global data structures is performed for all threads.
 */
class SchedulerWorker {
public:
	class Scheduler* scheduler;		///< Pointer to the scheduler this worker belongs to.
	unsigned int id;				///< Index of the worker in the scheduler's worker vector.
	SchedulerMethod* method;		///< Pointer to the method currently scheduled for this worker.
	std::thread thread;				///< Thread doing the work performing the method.
	double startTime;				///< Time when the last method call has been started.

	double shakingStartTime;		///< CPUtime when this worker has started the last shaking operation.
	mh_randomNumberGenerator* rng;  ///< The random number generator used in this thread.

	bool isWorking;				///< Indicates if the thread is currently in the working phase, i.e. a method has been assigned to it (only meaningful, if #schsync is set to true).
	bool terminate;				///< Indicates if this thread specifically is to be terminated.
	std::vector<SchedulerMethodResult> results;	///< List for storing the results achieved with this worker. Currently only used in the context of thread synchronization.

	/**
	 * Population of solutions associated with this worker.
	 * The exact meaning depends on the specific, derived scheduler class.
	 */
	population pop;

	/**
	 * The solution which is actually created/modified by a called method.
	 */
	mh_solution *tmpSol;

	/** Indicates the result of the last method call w.r.t. tmpSol */
	SchedulerMethodResult::ObjChange tmpSolObjChange;

	/**
	 * Constructs a new worker object for the given scheduler, method and solution, which
	 * will be executable by the run() method.
	 * The thread running this worker will use the value of threadSeed as random seed for
	 * the random number generator.
	 */
	SchedulerWorker(class Scheduler* _scheduler, unsigned int _id, const mh_solution *sol, mh_randomNumberGenerator* _rng, int _popsize=2) :
		pop(*sol, _popsize, false, false) {
		scheduler = _scheduler;
		id=_id,
		method = nullptr;
		startTime = 0;
		tmpSol = sol->clone();
		tmpSolObjChange = SchedulerMethodResult::OBJ_SAME;
		shakingStartTime = 0;
		rng = _rng;

		isWorking = false;
		terminate = false;
	}

	/** Destructor of SchedulerWorker */
	virtual ~SchedulerWorker() {
		delete tmpSol;
		delete rng;
	}

	/**
	 * Checks the globally best solution in the scheduler's population.
	 * If it is better, the worker's incumbent solution is updated with probability #schpmig.
	 */
	void checkGlobalBest();

	/**
	 * This method is the main procedure of a worker, which is spawned as an own thread.
	 * It contains the main loop consisting of the selection of the next method and solutions
	 * to which it is applied, running it, and updating relevant data.
	 * Additionally, the termination criteria are checked after each iteration by calling the
	 * terminate() method.
	 * mutex is used to ensure synchronization of the access to the optimization data
	 * structures shared by the worker threads.
	 */
	void run();
};


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
	/** Different strategies for selecting a method from a method pool:
	 * - MSSequential: choose one after the other in the given order, then restarting again with first
	 * - MSSequentialOnce: choose one after the other, each just once, and then return nullptr
	 * - MSRandom: uniform random selection
	 * - MSRandomOnce: uniform random selection, but each just once; finally return nullptr
	 * - MSSelfadaptive: random selection with self-adaptive probabilities */
	enum MethodSelStrat { MSSequentialRep, MSSequentialOnce, MSRandomRep, MSRandomOnce, MSSelfadaptive };

protected:

	Scheduler *scheduler;				///< Associated Scheduler
	MethodSelStrat strategy;			///< The selection strategy to be used.
	std::vector<unsigned int> methodList; 	///< List of Indices of the methods in the methodPool.

	int lastMethod;			///< Index of last applied method in methodList or -1 if none.

public:

	/** Initialize SchedulerMethodSelector for given strategy. */
	SchedulerMethodSelector(Scheduler *scheduler_, MethodSelStrat strategy_)
		: scheduler(scheduler_), strategy(strategy_), lastMethod(-1) {
	}

	/** Cleanup. */
	virtual ~SchedulerMethodSelector() {
	}

	/** Adds a the method with the given index to the methodList. */
	void add(unsigned int idx) {
		methodList.push_back(idx);
	}

	/** Returns the number of methods contained in the methodList. */
	unsigned int size() {
		return methodList.size();
	}

	/** Returns true if the methodList is empty. */
	bool empty() {
		return methodList.empty();
	}

	/** Returns the i-th method in the methodList. */
	unsigned int operator[](unsigned int i) {
		return methodList[i];
	}

	/** Resets lastMethod to none (= -1). */
	void resetLastMethod() {
		lastMethod = -1;
	}

	/* Returns true if a previously applied method is known. */
	bool hasLastMethod() {
		return lastMethod != -1;
	}

	/** Selects a method according to the chosen selection strategy from the methodList.
	 */
	virtual SchedulerMethod *select();

	/** Returns true if at least one more method can be returned. */
	virtual bool hasFurtherMethod() {
		return lastMethod < int(methodList.size())-1;
	}

	/** Returns the last selected method or nullptr if none has been selected yet. */
	SchedulerMethod *getLastMethod();
};

//--------------------------- Scheduler ------------------------------

/**
 * The scheduler base class for flexibly realizing GRASP, VNS, VLNS etc. approaches in sequential as well as
 * multi-threaded ways. It maintains a methodPool consisting of SchedulerMethods that are iteratively
 * called. The scheduler is in particular responsible for deciding at which point in the optimization which
 * specific method is applied.
 */
class Scheduler : public mh_advbase {
	friend class SchedulerWorker;
	friend class SchedulerMethodSelector;
protected:
	/** The method pool from which the scheduler chooses the methods to be used. */
	std::vector<SchedulerMethod*> methodPool;

	/* Statistical data on methods */
	std::vector<int> nIter;			///< Number of iterations of the particular methods.
	std::vector<double> totTime;	///< Total time spent running the particular methods.
	std::vector<double> totNetTime;	///< Total netto time spent for the method, excl. VND in case of shaking
	std::vector<int> nSuccess;			///< Number of successful iterations of the particular methods.
	std::vector<double> sumGain;			///< Total gain achieved by the particular methods.

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

	/**
	 * The SchedulerWorkers spawned by the scheduler in individual threads.
	 */
	std::vector<SchedulerWorker *> workers;

	/**
	 * Mutex used for the synchronization of access to the Scheduler data not owned by the workers,
	 * i.e., the population of solutions, the method pool, etc.
	 */
	std::mutex mutex;

	/**
	 * Mutex used for blocking threads (together with the condition variable cvNoMethodAvailable)
	 * if there is currently no method available that can be assigned to the thread.
	 * If this is the case, the thread will wait until another thread
	 * finishes its current method and sends a notification.
	 */
	std::mutex mutexNoMethodAvailable;

	/**
	 * Condition variable for blocking threads (together with the mutex mutexNoMethodAvailable)
	 * if there is currently no method available that can be assigned to the thread.
	 * If this is the case, the thread will wait until another thread
	 * finishes its current method and sends a notification.
	 */
	std::condition_variable cvNoMethodAvailable;

	unsigned int _schthreads;		///< Mirrored mh parameter #schthreads for performance reasons.
	bool _schsync;			///< Mirrored mh parameter #schsync for performance reasons.
	int _titer;							///< Mirrored mh parameter #titer for performance reasons.
	double _schpmig; 		///< Mirrored mh parameter #schpmig for performance reasons.

	/**
	 * Counts the number of threads that are currently waiting for the working phase to begin.
	 * Note: Only meaningful if #schsync is set to true.
	 */
	unsigned int workersWaiting;

	/**
	 * Mutex used for blocking threads (together with the condition variable cvNotAllWorkersInPrepPhase)
	 * if the synchronization of threads is active to wait after methods have been scheduled for all threads
	 * and before these methods are actually run.
	 * If the last thread reaches this point it sends a notification so that all threads can start the
	 * next working phase.
	 */
	std::mutex mutexNotAllWorkersInPrepPhase;

	/**
	 * Condition variable used for blocking threads (together with the mutex mutexNotAllWorkersInPrepPhase)
	 * if the synchronization of threads is active to wait after methods have been scheduled for all threads
	 * and before these methods are actually run.
	 * If the last thread reaches this point it sends a notification so that all threads can start the
	 * next working phase.
	 */
	std::condition_variable cvNotAllWorkersInPrepPhase;

	/**
	 * Mutex used for ensuring an ordering of threads (together with the condition variable cvOrderThreads)
	 * by which they have to start the optimization, i.e. by which a first method is assigned to them.
	 */
	std::mutex mutexOrderThreads;

	/**
	 * Condition variable used for ensuring an ordering of threads (together with the mutex mutexOrderThreads)
	 * by which they have to start the optimization, i.e. by which a first method is assigned to them.
	 */
	std::condition_variable cvOrderThreads;

public:
	/**
	 * Constructor: Initializes the scheduler.
	 */
	Scheduler(pop_base &p, const std::string &pg = "");

	/**
	 * Destructor, deletes the used methodPool and the solution objects.
	 */
	virtual ~Scheduler() {
		// clean up method pool
		for(auto method : methodPool)
			delete method;
	}

	/* Set a callback method, which is then periodically called with the currently best objective value
	 * during the optimization, whenever a method returs.  If it returns 1 the optimization will stop.
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
	 * Initiates the scheduling and runs the optimization.
	 * A certain number of SchedulerWorker objects, defined by the threads() parameter, is created and
	 * started in individual threads, each running its own loop .
	 */
	void run() override;

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

	/**
	 * Determines the next method to be applied and sets it in the given worker.
	 * The solution to be modified is tmpSol and the method may depend on
	 * the worker's population.
	 * If currently nothing further can be done, possibly because other threads have to
	 * finish first, the method pointer in worker is set to nullptr and nothing further is changed.
	 * This method has to be always called in an exclusive way,
	 * i.e., mutex.lock() must be done outside.
	 */
	virtual void getNextMethod(SchedulerWorker *worker) = 0;

	/**
	 * Updates the worker->tmpSol, worker->pop and the scheduler's population.
	 * If the flag updateSchedulerData is set to true, global data, such as the scheduler's
	 * population, is possibly updated as well, according to the result of the last method application.
	 * Furthermore, the worker's incumbent is updated to the global best one with probability
	 * #schpmig.
	 * If it is false, only the worker's population and no data is exchanged between the scheduler's and the
	 * worker's populations.
	 * If storeResult is true, a new MethodApplicationResult object storing the result of the last
	 * method application is appended to the SchedulerWorker's result list.
	 * This method is called with mutex locked.
	 */
	virtual void updateData(SchedulerWorker* worker, bool updateSchedulerData, bool storeResult) = 0;

	/**
	 * Updates the global data based on the entries in the results vectors of the workers.
	 * It needs to be ensured that the result of this update is always the same independent from the
	 * order of the workers.
	 * Furthermore, each worker's incumbent solution is updated to the globally best one by
	 * probability #schpmig.
	 * If clearResults is set to true, the vector is cleared after the results have been processed.
	 * Otherwise, the results remain in the vector.
	 */
	virtual void updateDataFromResultsVectors(bool clearResults) = 0;

	/**
	 * Updates the statistics data after applying a method in worker.
	 * @param worker current worker object
	 * @param methodTime CPU time used by the method call
	 */
	virtual void updateMethodStatistics(SchedulerWorker *worker, double methodTime);

	/**
	 * Prints more detailed statistics on the methods used by the scheduler.
	 * The output contains the number of iterations used for each method, the number of
	 * successful iterations, the total and average gain in objective value the method yielded,
	 * and the total, relative, and total netto times spent with applying the method.
	 * The total netto times normally correspond to the total times. Just in case of
	 * methods that include actions one may consider as auxiliary,
	 * like in shaking neighborhoods the automatically applied VND, the total netto time
	 * reports the total times excluding these further actions. It is the responsibility of a
	 * derived class (like GVNSScheduler below) to set the total netto time to other values
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
	 * Write the log entry including the given method name if parameter lmethod is set.
	 */
	virtual bool writeLogEntry(bool inAnyCase, bool finishEntry, const std::string &method);
};


//--------------------------- GVNSScheduler ------------------------------

/**
 * This class implements a general variable neighborhood search (GVNS) with an arbitrary
 * many construction heuristics, local improvement methods, and shaking or large neighborhood
 * search methods. For each of the categories of methods, it can be chosen whether the
 * different methods are applied in a strictly sequential way, uniform random way,
 * or random with with self-adaptive application probabilities.
 * Each worker performs an independent VNS, the overall best solution is adopted to the
 * Scheduler's main population.
 */
class GVNSScheduler : public Scheduler {

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
	GVNSScheduler(pop_base &p, unsigned int nconstheu, unsigned int nlocimpnh,
			unsigned int nshakingnh, const std::string &pg = "");

	/** Cloning is not implemented for this class. */
	virtual GVNSScheduler* clone() const {
		mherror("Cloning not implemented in GVNSScheduler");
		return nullptr;
	}

	/** Cleanup: delete SchedulerMethodSelectors. */
	~GVNSScheduler() {
		delete constheu;
		for (int t=0;t<schthreads();t++) {
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
	 * As the exact history of results is irrelevant to the GVNSScheduler, the value of storeResult
	 * is ignored and no result information is appended to the vector's result list.
	 * This method is called with mutex locked.
	 */
	void updateData(SchedulerWorker *worker, bool updateSchedulerData, bool storeResult) override;

	/**
	 * Updates the the scheduler's population in case the best incumbent solution among all workers
	 * is better than the best solution stored in the scheduler's population.
	 * As the exact history of results is irrelevant to the GVNSScheduler and no results are ever stored,
	 * the value of clearResults is ignored and the results vectors are not cleared.
	 */
	void updateDataFromResultsVectors(bool clearResults);

	/**
	 * Updates the statistics data after applying a method in worker.
	 * The special aspect here is that method times and success rates of shaking neighborhoods
	 * consider the embedded local improvement and they are therefore not done here but in the separate
	 * method updateShakingMethodStatistics.
	 * @param worker current worker object
	 * @param methodTime CPU time used by the method call
	 */
	void updateMethodStatistics(SchedulerWorker *worker, double methodTime) override;

	/**
	 * Separate statistics update for shaking methods, which is called after performing
	 * a full local improvement.
	 */
	void updateShakingMethodStatistics(SchedulerWorker *worker, bool improved);
};

} // end of namespace mh

#endif /* MH_SCHEDULER_H */
