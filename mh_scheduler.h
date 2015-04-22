/*! \file mh_scheduler.h
 \brief A flexible scheduler class for realizing diverse sequential and basic multithreaded
 	 GRASP, VNS, and VLNS approaches.
 */

#ifndef MH_SCHEDULER_H
#define MH_SCHEDULER_H

#include <string>
#include "mh_advbase.h"
#include "mh_pop.h"
#include "mh_c11threads.h"


/** \ingroup param
 * Sets the maximum number of parallel threads to be used by a scheduler instance.
 */
extern int_param numthreads;


/**
 * Class representing a method like a neighborhood search or construction method that can be scheduled by the
 * scheduler along with the meta-information relevant to the scheduling process.
 * The method itself is realized as a member function of mh_solution with an integer parameter that might be used
 * to more specifically choose the functionality of the method (e.g. neighborhood size or randomization factor).
 */
class SchedulableMethod {
public:
	const string name;			///> The method's (unique) name (possibly including method_par).
	const int method_par;		///> A method-specific integer parameter that might be used within the method.
	void (* solmethod)(mh_solution *,int);	///> The actual method to be executed, realized as member function of mh_solution.
	const bool deterministic;	///> Indicates whether this is a deterministic method or not.
	const bool improvement;		///> Indicates whether this is an improvement method that operates on an already existing solution.

	int idx;				///> Index in methodPool of Scheduler
	unsigned int weight;	///> The weight currently assigned to this method.
	unsigned int score;		///> Accumulated score that has been assigned to this method.

	/**
	 * Constructs a new schedulable method using the given arguments, assigning a default weight of 1 and
	 * a score of 0.
	 */
	SchedulableMethod(const std::string &_name, void (* _solmethod)(mh_solution *,int), bool _improvement, bool _deterministic,
			int _method_par = 0) : name(_name), method_par(_method_par), solmethod(_solmethod),
					deterministic(_deterministic), improvement(_improvement) {
		idx = -1;
		weight = 1;
		score = 0;
	}

	/** Apply the method using method_par to the given solution. */
	void run(mh_solution *sol) {
		(*solmethod)(sol, method_par);
	}

	/**
	 * Virtual Destructor.
	 */
	virtual ~SchedulableMethod() {
	}
};



/**
 * Structure for a SchedulerWorker that runs as own thread spawned by the scheduler.
 * The structure contains pointers to the Scheduler, Schedulable Method and the solution
 * to which the method is to be applied.
 */
class SchedulerWorker {
public:
	class Scheduler* scheduler;		///< Pointer to the scheduler this worker belongs to.
	SchedulableMethod* method;	///< Pointer to the method currently scheduled for this worker.
	mh_solution *solution;		///< Solution to which the method is to be applied.
	std::thread thread;			///< Thread doing the work performing the method.

	/**
	 * Constructs a new worker object for the given scheduler, method and solution, which
	 * will executable by the run() method.
	 */
	SchedulerWorker(class Scheduler* _scheduler) {
		scheduler = _scheduler;
		method = NULL;
		solution = NULL;
	}

	/**
	 * Starts the worker
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
	std::mutex mutexScheduler;

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
	 * NOT IMPLEMENTED!
	 * This method does nothing and is only implemented since it is required by the underlying
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
		if(finish)
			return true;
		if(callback != NULL && callback(pop->bestObj()))
			return true;
		if (mh_advbase::terminate())
			return true;

		return false;
	}

	/**
	 * This method is run by a worker thread after it is created.
	 * It contains the main loop consisting of the selection of the next method, running it
	 * and updating the Scheduler data.
	 * Additionally, the termination criteria are checked after each iteration by calling the
	 * terminate() method.
	 * mutexScheduler is used to ensure synchronization of the access to the optimization data
	 * structures shared by the worker threads.
	 * Note that a derived class overwriting this method must likewise guarantee
	 * proper synchronization!
	 * @worker A pointer to the SchedulerWorker object for whose thread this method is called.
	 */
	void runWorker(SchedulerWorker *worker);

	/**
	 * Determines a method and a solution to which
	 * the method is to be applied according to the defined selection rules and
	 * the weights associated to the methods and stores pointers to the method and solution in
	 * the given SchedulerWorker. If currently nothing further can be done,
	 * possibly because other threads have to finish first, the method pointer in
	 * worker ist set to NULL and nother further is changes.
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
	virtual void updateOptimizationData(SchedulerWorker* worker) = 0;

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


#endif /* MH_SCHEDULER_H */
