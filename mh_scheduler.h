/*! \file mh_scheduler.h
 \brief Flexible scheduler classes for realizing diverse sequential and basic multithreaded
 	 GRASP, VNS, and VLNS approaches.
 */

#ifndef MH_SCHEDULER_H
#define MH_SCHEDULER_H

#include <assert.h>
#include <string>
#include "mh_advbase.h"
#include "mh_pop.h"
#include "mh_c11threads.h"


/** \ingroup param
 * Sets the maximum number of parallel worker threads to be used by a scheduler instance.
 */
extern int_param threadsnum;


//--------------------------- SchedulerMethod ------------------------------

/**
 * Abstract base class representing a method like a neighborhood search or construction method
 * that can be scheduled by the scheduler along with the meta-information relevant to the
 * scheduling process.
 * This base class does not yet contain a pointer or some other reference to the method
 * to be called.
 */
class SchedulerMethod {
public:
	const string name;			///< The method's (unique) name (possibly including method_par).
	const int arity;			///< Arity, i.e., number of input solutions of the method.

	unsigned int idx;				///< Index in methodPool of Scheduler.
	// TODO Was sind weight und score? Erklärung fehlt!
	// unsigned int weight;	///< The weight currently assigned to this method.
	// unsigned int score;		///< Accumulated score that has been assigned to this method.

	/**
	 * Constructs a new SchedulerMethod from a MethodType function object using the
	 * given arguments, assigning a default weight of 1 and a score of 0.
	 */
	SchedulerMethod(const std::string &_name, int _par, int _arity) :
				name(_name), arity(_arity)  {
		idx = -1;
		// weight = 1;
		// score = 0;
		// so far only construction and simple improvement methods are considered
		assert(arity>=0 && arity<=1);
	}

	/** Apply the method to the given solution. The method returns true if the solution
	 * has been changed and false otherwise. */
	virtual bool run(mh_solution *sol) = 0;

	/**
	 * Virtual Destructor.
	 */
	virtual ~SchedulerMethod() {
	}
};

/** Template class for realizing concrete SchedulerMethods for bool(int) member function
 *  of specific solution classes, i.e., classes derived from mh_solution.
 *  An integer parameter is maintained that is passed when calling the method by run for
 *  a specific solution. This integer can be used to control the methods functionality, e.g.
 *  for the neighborhood size, randomization factor etc. The return value must indicate
 *  whether or not the solution has actually been modified. */
template<class SpecSol> class SolMemberSchedulerMethod : public SchedulerMethod {
public:
	bool (SpecSol::* pmeth)(int);		///< Member function pointer to a bool(int) function
	const int par;						///< Integer parameter passed to the method

	/** Constructor initializing data. */
	SolMemberSchedulerMethod(const std::string &_name, bool (SpecSol::* _pmeth)(int),
			int _par, int _arity) :
		SchedulerMethod(_name,_par,_arity), pmeth(_pmeth), par(_par) {
	}

	/** Apply the method for the given solution, passing par. */
	bool run(mh_solution *sol) {
		return ((dynamic_cast<SpecSol *>(sol))->*pmeth)(par);
	}
};


//--------------------------- SchedulerWorker ------------------------------

/**
 * SchedulerWorker that runs as own thread spawned by the scheduler.
 * The class contains in particular pointers to the Scheduler, SchedulerMethod and
 * the workers own population to which the method is to be applied.
 */
class SchedulerWorker {
public:
	class Scheduler* scheduler;		///< Pointer to the scheduler this worker belongs to.
	SchedulerMethod* method;		///< Pointer to the method currently scheduled for this worker.
	std::thread thread;				///< Thread doing the work performing the method.

	/**
	 * Population of solutions associated with this worker.
	 * The exact meaning depends on the specific, derived scheduler class.
	 */
	population pop;

	/**
	 * The solution which is actually created/modified by a called method.
	 */
	mh_solution *tmpSol;

	/** Indicates the result of the last method call w.r.t. tmpChrom:
	 * - -1: solution not changed
	 * -  0: solution not improved but changed
	 * -  1  solution improved */
	bool tmpSolImproved;

	/**
	 * Constructs a new worker object for the given scheduler, method and solution, which
	 * will executable by the run() method.
	 */
	SchedulerWorker(class Scheduler* _scheduler, const mh_solution *sol) :
		pop(*sol, 1, false, false) {
		scheduler = _scheduler;
		method = NULL;
		tmpSol = sol->clone();
		tmpSolImproved = -1;
	}

	/** Destructor of SchedulerWorker */
	virtual ~SchedulerWorker() {
		delete tmpSol;
	}

	/**
	 * This method is the main procedure of a worker, which is spawend as an own thread.
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
 * Different selection strategies such as sequential, uniformly random or self-adaptive
 * are provided. Also contains the contextual data required for the selection.
 * A SchedulerMethodSelector can be associated with a worker thread only or the whole Scheduler.
 */
class SchedulerMethodSelector {

public:
	/** Different strategies for selecting a method from a method pool:
	 * - sequential: choose one after the other in the given order
	 * - random: uniform random selection
	 * - selfadaptive: random selection with self-adaptive probabilities */
	enum MethodSelStrat { MSsequential, MSrandom, MSselfadaptive };

protected:

	Scheduler *scheduler;				///< Associated Scheduler
	MethodSelStrat strategy;			///< The selection strategy to be used.
	vector<unsigned int> methodList; 	///< List of Indexes of the methods in the methodPool.

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

	/** Returns the number of method contained in the methodList. */
	unsigned int size() {
		return methodList.size();
	}

	/** Returns true if them methodList is empty. */
	bool empty() {
		return methodList.empty();
	}

	/** Returns the i-th method in the methodList. */
	unsigned int operator[](unsigned int i) {
		return methodList[i];
	}

	/** Reset lastMethod to none (= -1). */
	void resetLastMethod() {
		lastMethod = -1;
	}

	/** Select a method according to the Selector's strategy from the methodList.
	 */
	SchedulerMethod *select();
};

//--------------------------- Scheduler ------------------------------

/**
 * The scheduler base class for flexibly realizing GRASP, VNS, VLNS etc. approaches in sequential as well as
 * multithreaded ways. It maintains a methodPool consisting of SchedulerMethods that are iteratively
 * called. The scheduler is in particular responsible for deciding at which point in the optimization which
 * specific method is applied.
 */
class Scheduler : public mh_advbase {
	friend class SchedulerWorker;
	friend class SchedulerMethodSelector;
protected:
	/** The method pool from which the scheduler chooses the methods to be used. */
	vector<SchedulerMethod*> methodPool;

	/* Statistical data on methods */
	vector<int> nIter;				///< Number of iterations of the particular methods.
	vector<double> totTime;			///< Total time spent running the particular methods.
	vector<int> nSuccess;			///< Number of successful iterations of the particular methods.
	vector<double> sumGain;			///< Total gain achieved by the particular methods.

	/**
	 * Optional function pointer to a callback function passed by the interface.
	 * This function (if != NULL)  is called periodically during the optimization, in particular each time
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
	vector<SchedulerWorker *> workers;

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

public:
	/**
	 * Constructor: Initializes the scheduler.
	 */
	Scheduler(pop_base &p, const pstring &pg = (pstring) (""));

	/**
	 * Destructor, deletes the used methodPool and the solution objects.
	 */
	virtual ~Scheduler() {
		// clean up method pool
		for(auto method : methodPool)
			delete method;
	}

	/** Cloning is prohibited for the scheduler. */
	virtual Scheduler* clone() const {
		mherror("Scheduler cannot be cloned");
		return NULL;
	}

	/* Set a callback method, which is then periodically called with the currently best objective value
	 * during the optimization, whenever a method returs.  If it returns 1 the optimization will stop.
	 * Initially, no callback method is set, i.e., callback=NULL. */
	void setCallback(bool (*_callback)(double) = NULL) {
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
		nSuccess.push_back(0);
		sumGain.push_back(0);
	}

	/**
	 * Initiates the scheduling and runs the optimization.
	 * A certain number of SchedulerWorker objects, defined by the threads() parameter, is created and
	 * started in individual threads, each running its own loop .
	 */
	void run();

	/**
	 * This method does here nothing and is only implemented since it is required by the underlying
	 * base class mh_advbase.
	 */
	void performIteration() {
		mherror("Scheduler does not implement/use performGeneration");
	}

	/**
	 * Returns true, if the external application has requested the optimization to terminate,
	 * if the scheduler's terminate flag has been set to true,
	 * or if any of the given termination criteria applies.
	 * Is called from run() after each call to performGeneration().
	 */
	bool terminate() {
		if (finish)
			return true;
		if (callback != NULL && callback(pop->bestObj()))
			return true;
		if (mh_advbase::terminate())
			return true;
		return false;
	}

	/**
	 * Determines the next method to be applied and sets it in the given worker.
	 * The solution to be modified is tmpChrom and the method may depend on
	 * the worker's population.
	 * If currently nothing further can be done, possibly because other threads have to
	 * finish first, the method pointer in worker is set to NULL and nothing further is changed.
	 */
	virtual void getNextMethod(SchedulerWorker *worker) = 0;

	/**
	 * Updates the worker->tmpChrom, worker->pop and the schedulers population and possibly
	 * other data according to the result of the last method application.
	 * This method is called with mutex locked.
	 */
	virtual void updateData(SchedulerWorker* worker) = 0;

	/**
	 * Updates the statistics data after applying a method in worker.
	 * @param methodTime CPU time used by the method call
	 */
	virtual void updateMethodStatistics(SchedulerWorker *worker, double methodTime);

	/**
	 * Prints more detailed statistics on the methods used by the scheduler.
	 * The output contains the number of iterations used for each method, the number of
	 * successful iterations, the total and average gain in objective value the method yielded
	 * and the total and relative time spent with applying the method.
	 */
	virtual void printMethodStatistics(ostream &ostr);

	/**
	 * Prints general statistics on the optimization.
	 * In particular, the total runtime of the algorithm, the best found objective value, and
	 * in which iteration and after how much time it was found.
	 */
	virtual void printStatistics(ostream &ostr);
};


//--------------------------- VNSScheduler ------------------------------

/**
 * This class implements a general variable neighborhood search (GVNS) with an arbitrary
 * many construction heuristics, local improvement methods, and shaking or large neighborhood
 * search methods. For each of the categories of methods, it can be chosen whether the
 * different methods are applied in a strictly sequential way, uniform random way,
 * or random with with self-adaptive application probabilities.
 * Each worker performs an independent VNS, the overall best solution is adopted to the
 * Scheduler's main population.
 */
class VNSScheduler : public Scheduler {

protected:
	SchedulerMethodSelector constheu;	///< Selector for construction heuristic methods
	SchedulerMethodSelector locimpnh;	///< Selector for local improvement neighborhood methods
	SchedulerMethodSelector shakingnh;	///< Selector for shaking/LNS methods

	/** An improved solution has been obtained by a method and is stored in tmpChrom.
	 * This method updates worker->pop[0] holding the worker's so far best solution and
	 * possibly the Scheduler's global best solution at pop[0].
	 */
	void copyBetter(SchedulerWorker *worker);

public:
	/**
	 * Constructor: Initializes the VNSScheduler. Construction, improvement and shaking methods
	 * are then added by addSchedulerMethod, whereas nconstheu>=0 construction heuristics
	 * must come first, followed nlocimpnh>=0 local improvement heuristics, and
	 * finally nshakingnh  shaking or large neighborhood search neighborhoods.
	 */
	VNSScheduler(pop_base &p, unsigned int nconstheu, unsigned int nlocimpnh,
			unsigned int nshakingnh, const pstring &pg = (pstring) (""));

	/** Cloning is not implemented for this class. */
	virtual VNSScheduler* clone() const {
		mherror("Cloning not implemented in VNSScheduler");
		return NULL;
	}

	/**
	 * Schedules the next method. Initially, if the worker just started and no other method
	 * has been performed yet, the construction method with index 0 is scheduled.
	 * Otherwise the next method is chosen according to the classic VNS strategy.
     */
	void getNextMethod(SchedulerWorker *worker);

	/**
	 * Updates the tmpChrom, worker->pop and the schedulers population according to
	 * the result of the last method application.
	 */
	void updateData(SchedulerWorker *worker);
};



#endif /* MH_SCHEDULER_H */
