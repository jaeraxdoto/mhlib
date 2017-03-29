/*! \file mh_parscheduler.h
 \brief Flexible scheduler class supporting multithreading for realizing
 	 GRASP, IG, VNS, and VLNS approaches.
 */

#ifndef MH_PARSCHEDULER_H
#define MH_PARSCHEDULER_H

#include "mh_c11threads.h"
#include "mh_scheduler.h"

namespace mh {

/** \ingroup param
 * The number of parallel worker threads to be used by a scheduler instance.
 * If set to a larger value than one, multithreading is thus applied. Note that in this case
 * some more iterations might be performed than specified by #titer and #tciter, as all
 * threads are kept as busy as possible until a termination condition is fulfilled, and
 * then all active methods are completed. Especially in combination with #schsync turned on,
 * the log output might also be misleading since results of the individual methods are applied in
 * a delayed way in order to achieve a deterministic outcome.
 */
extern int_param schthreads;

/** \ingroup param
 * If set to true, the synchronization of the threads in the scheduler is active (default: false).
 * In particular, the optimization will consist of two repeating phases:
 * The actual working phase, where the threads run the currently scheduled methods in parallel,
 * and the synchronization phase, where threads wait until every thread is finished before starting
 * the next working phase.
 * Only if all threads are in the synchronization phase,the scheduler's data is updated.
 * No (global) updates happen during the working phase.
 */
extern bool_param schsync;

/** \ingroup param
 * Migration probability for a thread in the scheduler to update its incumbent solution after a major
 * iteration by copying the global best solution.
 * If the best global solution is better than the incumbent solution,
 * it becomes the thread's new incumbent solution.
 */
extern double_param schpmig;

//--------------------------- SchedulerWorker ------------------------------

/**
 * SchedulerWorker that runs as own thread spawned by the scheduler.
 * The class contains in particular pointers to the Scheduler, SchedulerMethod and
 * the workers own population to which the method is to be applied.
 * Furthermore, the class maintains a vector for storing SchedulerMethodResult entries.
 * Currently, this vector is only used in the context of thread synchronization to keep track
 * of all results until the update of the global data structures is performed for all threads.
 */
class SchedulerWorker {
public:
	class ParScheduler* scheduler;		///< Pointer to the scheduler this worker belongs to.
	int id;							///< Index of the worker in the scheduler's worker vector.
	SchedulerMethod* method;		///< Pointer to the method currently scheduled for this worker.
	SchedulerMethodContext* methodContext;		///< Pointer to the SchedulerMethodContext object associated to the method currently scheduled for this worker.
	std::thread thread;				///< Thread doing the work performing the method.
	std::array<double,maxStackedMethods> startTime;	///< Time when the last method call has been started.
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

	/** Indicates the outcome of the last method application w.r.t. tmpSol. */
	SchedulerMethodResult tmpSolResult;

	/**
	 * Constructs a new worker object for the given scheduler, method and solution, which
	 * will be executable by the run() method.
	 * The thread running this worker will use the value of threadSeed as random seed for
	 * the random number generator.
	 */
	SchedulerWorker(class ParScheduler* _scheduler, int _id, const mh_solution *sol, mh_randomNumberGenerator* _rng, int _popsize=2) :
		pop(*sol, _popsize, false, false) {
		scheduler = _scheduler;
		id=_id,
		method = nullptr;
		methodContext = nullptr;
		tmpSol = sol->clone();
		for (auto &&t : startTime) t = 0;
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


//--------------------------- ParScheduler ------------------------------

/**
 * The parallel scheduler base class for flexibly realizing GRASP, VNS, VLNS etc. approaches in multithreaded ways.
 * It maintains a methodPool consisting of SchedulerMethods that are iteratively
 * called. The scheduler is in particular responsible for deciding at which point in the optimization which
 * specific method is applied.
 */
class ParScheduler : public Scheduler {
	friend class SchedulerWorker;
protected:

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
	 * Mutex used for blocking threads (together with the condition variable #cvNoMethodAvailable)
	 * if there is currently no method available that can be assigned to the thread.
	 * If this is the case, the thread will wait until another thread
	 * finishes its current method and sends a notification.
	 */
	std::mutex mutexNoMethodAvailable;

	/**
	 * Condition variable for blocking threads (together with the mutex #mutexNoMethodAvailable)
	 * if there is currently no method available that can be assigned to the thread.
	 * If this is the case, the thread will wait until another thread
	 * finishes its current method and sends a notification.
	 */
	std::condition_variable cvNoMethodAvailable;

	int _schthreads;		///< Mirrored mh parameter #schthreads for performance reasons.
	bool _schsync;			///< Mirrored mh parameter #schsync for performance reasons.
	double _schpmig; 		///< Mirrored mh parameter #schpmig for performance reasons.

	/**
	 * Counts the number of threads that are currently waiting for the working phase to begin.
	 * Note: Only meaningful if #schsync is set to true.
	 */
	int workersWaiting;

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

	/**
	 * Rethrows the exceptions that have possibly occurred in the threads and have been collected in the worker_exceptions vector.
	 * I.e. the exceptions are passed to the main thread.
	 */
	void rethrowExceptions();

public:
	/**
	 * Constructor: Initializes the scheduler.
	 */
	ParScheduler(pop_base &p, const std::string &pg = "");

	/**
	 * Initiates the scheduling and runs the optimization.
	 * A certain number of SchedulerWorker objects, defined by the threads() parameter, is created and
	 * started in individual threads, each running its own loop .
	 */
	void run() override;

	/**
	 * This method may be called to reset the scheduler for a new run.
	 * Statistics data will be aggregated over the runs, but the next run will be entirely
	 * independent.
	 */
	void reset() override;

	/** Function that ensures mutual exclusion for the next block via mutex. */
	virtual void mutexLock() override {
		mutex.lock();
	}

	/** Function that ends a mutual exclusive block. */
	virtual void mutexUnlock() override {
		mutex.unlock();
	}

	/**
	 * Determines the next method to be applied and sets it in the worker given by idx.
	 * The solution to be modified is tmpSol and the method may depend on
	 * the worker's population.
	 * Also resets tmpSolResult, initializing it with the number how often the method has
	 * already been called for this solution.
	 * If currently nothing further can be done, possibly because other threads have to
	 * finish first, the method pointer in worker is set to nullptr and nothing further is changed.
	 * This method has to be always called in an exclusive way,
	 * i.e., mutex.lock() must be done outside.
	 */
	virtual SchedulerMethod *getNextMethod(int idx) = 0;

	/**
	 * Updates the worker->tmpSol, worker->pop, where the worker is given by idx, and the scheduler's population.
	 * If the flag updateSchedulerData is set to true, global data, such as the scheduler's
	 * population, is possibly updated as well, according to the result of the last method application.
	 * Furthermore, the worker's incumbent is updated to the global best one with probability
	 * #schpmig.
	 * If it is false, only the worker's population and no data is exchanged between the scheduler's and the
	 * worker's populations.
	 * If storeResult is true, a new MethodApplicationResult object storing the result of the last
	 * method application is appended to the SchedulerWorker's result list.
	 * This method is called with mutex locked.
	 * TODO: When worse solutions are actively set to be accepted via result.accept,
	 * a so far best solution is currently not yet stored and gets lost!
	 */
	virtual void updateData(int idx, bool updateSchedulerData, bool storeResult) = 0;

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
};

} // end of namespace mh

#endif /* MH_PARSCHEDULER_H */
