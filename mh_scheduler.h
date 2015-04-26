/*! \file mh_scheduler.h
 \brief Flexible scheduler classes for realizing diverse sequential and basic multithreaded
 	 GRASP, VNS, and VLNS approaches.
 */

#ifndef MH_SCHEDULER_H
#define MH_SCHEDULER_H

#include <string>
#include "mh_advbase.h"
#include "mh_pop.h"
#include "mh_c11threads.h"


/** \ingroup param
 * Sets the maximum number of parallel worker threads to be used by a scheduler instance.
 */
extern int_param threadsnum;

/** \ingroup param
 * Size of the population associated with each of the worker threads in the scheduler algorithm.
 */
extern int_param threadspsize;


//--------------------------- SchedulableMethod ------------------------------

/**
 * Abstract base class representing a method like a neighborhood search or construction method
 * that can be scheduled by the scheduler along with the meta-information relevant to the
 * scheduling process.
 * This base class does not yet contain a pointer or some other reference to the method
 * to be called.
 */
class SchedulableMethod {
public:
	const string name;			///< The method's (unique) name (possibly including method_par).
	const bool improvement;		///< Indicates whether this is an improvement method that operates on an already existing solution.
	const bool deterministic;	///< Indicates whether this is a deterministic method or not.

	int idx;				///< Index in methodPool of Scheduler.
	unsigned int weight;	///< The weight currently assigned to this method.
	unsigned int score;		///< Accumulated score that has been assigned to this method.

	/**
	 * Constructs a new SchedulableMethod from a MethodType function object using the
	 * given arguments, assigning a default weight of 1 and a score of 0.
	 */
	SchedulableMethod(const std::string &_name, int _par, bool _improvement,	bool _deterministic) :
				name(_name), improvement(_improvement), deterministic(_deterministic)  {
		idx = -1;
		weight = 1;
		score = 0;
	}

	/** Apply the method to the given solution. */
	virtual void run(mh_solution *sol) = 0;

	/**
	 * Virtual Destructor.
	 */
	virtual ~SchedulableMethod() {
	}
};

/** Template class for realizing concrete SchedulableMethods for void(int) member function
 *  of specific solution classes, i.e., classes derived from mh_solution.
 *  An integer parameter is maintained that is passed when calling the method by run for
 *  a specific solution. This integer can be used to control the methods functionality, e.g.
 *  for the neighborhood size, randomization factor etc. */
template<class SpecSol> class SolMemberSchedulableMethod : public SchedulableMethod {
public:
	void (SpecSol::* pmeth)(int);		///< Member function pointer to a void(int) function
	const int par;						///< Integer parameter passed to the method

	/** Constructor initializing data. */
	SolMemberSchedulableMethod(const std::string &_name, void (SpecSol::* _pmeth)(int),
			int _par, bool _improvement, bool _deterministic) :
		SchedulableMethod(_name,_par,_improvement,_deterministic), pmeth(_pmeth), par(_par) {
	}

	/** Apply the method for the given solution, passing par. */
	void run(mh_solution *sol) {
		((dynamic_cast<SpecSol *>(sol))->*pmeth)(par);
	}
};


//--------------------------- SchedulerWorker ------------------------------

/**
 * SchedulerWorker that runs as own thread spawned by the scheduler.
 * The class contains in particular pointers to the Scheduler, SchedulableMethod and
 * the workers own population to which the method is to be applied.
 */
class SchedulerWorker {
public:
	class Scheduler* scheduler;		///< Pointer to the scheduler this worker belongs to.
	SchedulableMethod* method;		///< Pointer to the method currently scheduled for this worker.
	std::thread thread;				///< Thread doing the work performing the method.

	/**
	 * Population of solutions associated with this worker.
	 * Note that positions 0 and 1 of the population are reserved:
	 * The working solution, i.e. the one that should be modified by the application
	 * of the method is expected to be at position 0.
	 * Furthermore, after the execution of the method, position 1 holds the modified solution
	 * and position 0 the original, unmodified solution.
	 */
	population pop;

	/**
	 * Constructs a new worker object for the given scheduler, method and solution, which
	 * will executable by the run() method.
	 */
	SchedulerWorker(class Scheduler* _scheduler, const mh_solution& sol) :
		pop(sol, threadspsize(), false, false) {
		scheduler = _scheduler;
		method = NULL;
	}

	/**
	 * This method is the main procedure of a worker, which is spawend as an own thread.
	 * It contains the main loop consisting of the selection of the next method, running it
	 * and updating the Scheduler data.
	 * Additionally, the termination criteria are checked after each iteration by calling the
	 * terminate() method.
	 * mutex is used to ensure synchronization of the access to the optimization data
	 * structures shared by the worker threads.
	 * Note that a derived class overwriting this method must likewise guarantee
	 * proper synchronization.
	 * @param worker A pointer to the SchedulerWorker object for whose thread this method is called.
	 */
	void run();
};


/**
 * The scheduler base class for flexibly realizing GRASP, VNS, VLNS etc. approaches in sequential as well as
 * multithreaded ways. It maintains a methodPool consisting of SchedulableMethods that are iteratively
 * called. The scheduler is in particular responsible for deciding at which point in the optimization which
 * specific method is applied.
 */
class Scheduler : public mh_advbase {
	friend class SchedulerWorker;
protected:
	/** The method pool from which the scheduler chooses the methods to be used. */
	vector<SchedulableMethod*> methodPool;

	/* Statistical data on methods */
	vector<int> nIter;				///< Number of iterations of the particular methods.
	vector<double> totTime;			///< Total time spent running the particular methods.
	vector<int> nSuccess;			///< Number of successful iterations of the particular methods.
	vector<double> sumGain;			///< Total gain achieved by the particular methods.

	int numMethods() { return methodPool.size(); }	///< Total number of methods.

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

public:
	/**
	 * Constructor: Initializes the scheduler.
	 * TODO: Achtung: Soll die Population hashing verwenden? -> momentan abh‰ngig von dupelim
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
	 * These SchedulableMethod objects are assumed to belong to the Scheduler and will finally
	 * be deleted by its destructor.
	 */
	void addSchedulableMethod(SchedulableMethod* method) {
		method->idx = methodPool.size();
		methodPool.push_back(method);
		nIter.push_back(0);
		totTime.push_back(0);
		nSuccess.push_back(0);
		sumGain.push_back(0);
	}

	/**
	 * Initiates the scheduling and runs the optimization.
	 * A certain number of threads (defined by the threads() parameter) is created and started,
	 * each running its own loop .
	 */
	void run();

	/**
	 * This method does here nothing and is only implemented since it is required by the underlying
	 * base class mh_advbase.
	 */
	void performGeneration() {
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
	 * Determines a method and the solution to which
	 * the method is to be applied according to the defined selection rules and
	 * the weights associated to the methods.
	 * A pointer to the method in the given SchedulerWorker is stored and the worker's
	 * population is updated to contain the solutions required to apply the method.
	 * Note that the solution that should actually be modified must be found at position 0
	 * of the workers population.
	 * If currently nothing further can be done,possibly because other threads have to
	 * finish first, the method pointer in worker is set to NULL and nothing further is changed.
	 */
	virtual void getNextMethod(SchedulerWorker *worker);

	/**
	 * Updates the global information on the optimization after performing a method
	 * within the specified SchedulerWorker,
	 * i.e. in particular the following data structures are updated:
	 * 1. The weights associated with the methods in the method pool.
	 * 2. The solutions in the population.
	 * If this method is called with NULL, all data are initialized. TODO Macht das Sinn?
	 */
	virtual void updateData(SchedulerWorker* worker) = 0;

	/**
	 * Updates the statistics data after applying a method in worker.
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
	 TODO Ber√ºcksichtigung vom Multithreading! Wie wird das gemacht?
	 */
	void updateData(SchedulerWorker *worker);
};



#endif /* MH_SCHEDULER_H */
