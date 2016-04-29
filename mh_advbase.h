/*! \file mh_advbase.h 
	\brief An advanced abstract base class for metaheuristics.

	This module contains definitions which are common in many metaheuristics.
	The advanced abstract base classe for metaheuristics extends mh_base
	by maintaining a population, methods for accessing it, and providing methods
	for iterating until a stopping criterion is fulfilled and printing calculating
	and printing progress statistics. It is designed so that it can be used as
	sub-algorithm in an island model. */

#ifndef MH_ADVBASE_H
#define MH_ADVBASE_H

#include "mh_base.h"
#include "mh_param.h"
#include "mh_popbase.h"
#include "mh_solution.h"

namespace mh {

/** \defgroup param Global parameters */

/* \ingroup param
	The termination condition (DEPRECATED).
	Decides the strategy used as termination criterion:
	- -1: terminate when #titer>0 && #titer iterations are
	  reached or when #tciter>0 && #tciter iterations without
	  improvement or when #tobj>0 && best solution reaches #tobj
	  or when #ttime>0 && effective runtime reaches #ttime.

	Deprecated values:
	- 0: terminate after #titer iterations.
	- 1: terminate after convergence, which is defined as:
		the objective value of the best solution in the population
		did not change within the last titer (not #tciter!)
		iterations.
	- 2: terminate when the given objective value limit (tobj()) is 
		reached. */
//extern int_param tcond;

/** \ingroup param
	The number of iterations until termination. Active if >=0. */
extern int_param titer;

/** \ingroup param 
	The number of iterations for termination according to convergence. Active if >=0. */
extern int_param tciter;

/** \ingroup param
	The objective value for termination. Active if >=0. */
extern double_param tobj;

/** \ingroup param
	The time limit for termination. Active if >=0. */
extern double_param ttime;

/** \ingroup param
	Group size for tournament selection. */
extern int_param tselk;

/** \ingroup param
	Replacement scheme:
	- 0: A new solution replaces a randomly chosen existing solution
		with the exception of the best solution.
	- 1: A new solution replaces the worst existing solution.
	- -k: The solution to be replaced is selected via a tournament
		selection of group size k (with replacement of actual).

	In addition, duplicate elimination takes place according to
	parameter #dupelim. */ 
extern int_param repl;

/** \ingroup param
	If set the current number of eliminated duplicates is
	printed at the end of each log entry. */
extern bool_param ldups;

/** \ingroup param
	If set, the elapsed time is printed as last entry in each log entry. */
extern bool_param ltime;

/** \ingroup param
 * Measure runtime by wall clock time.
 * If set to true, the runtime measured for the statistics of the metaheuristic is measured in wall clock time.
 * Otherwise, they refer to the CPU time. This does, however, not affect the runtimes measured
 * for specific neighborhoods in e.g. a VNS. This setting might be particularly useful in the context
 * of multithreading in the scheduler.
 * Note that if this parameter is set to true, a termination specified by the ttime parameter
 * will also be interpreted as wall clock time.
 */
extern bool_param wctime;

/** An advanced abstract base class for metaheuristics.
	This advanced abstract base classe for metaheuristics extends mh_base
	by maintaining a population, methods for accessing it, and providing methods
	for iterating until a stopping criterion is fulfilled and printing calculating
	and printing progress statistics. It is designed so that it can be used as
	sub-algorithm in an island model. */
class mh_advbase : public mh_base
{
public:
	/** The population of the metaheuristic.
		It is not owned by the metaheuristic and therefore not deleted by it. */
	pop_base *pop = nullptr;

	int nIteration = 0;	///< Number of current iteration.
	int nSubIterations = 0;	///< Number of iterations in subalgorithms.
	int nSelections = 0;	///< Number of performed selections
	int nDupEliminations = 0;	///< Number of performed duplicate eliminations

protected:
	int iterBest = 0;	    ///< Iteration in which best solution was generated.
	double timIterBest = 0;  ///< Time at which best solution was generated.

	// other class variables
	mh_solution *tmpSol = nullptr;	///< a temporary solution in which the result of operations is stored

	double bestObj = 0;		///< temporary best objective value
	double timStart = 0;        ///< CPUtime when run() was called

	bool _wctime;	///< Mirrored mh parameter wall_clock_time for performance reasons.

public:
	/** The constructor.
		An initialized population already containing solutions
		must be given. Note that the population is NOT owned by the 
		EA and will not be deleted by its destructor. */
	mh_advbase(pop_base &p, const std::string &pg="");
	/** Another constructor.
		Creates an empty algorithm that can only be used as a template. */
	mh_advbase(const std::string &pg="");
	/** The destructor.
		It does delete the temporary solution, but not the
		population. */
	virtual ~mh_advbase();
	/** Create new object of same class.
		Virtual method, uses the classes constructor to create a
		new algorithm object of the same class as the called object. */
	virtual mh_advbase *clone(pop_base &p, const std::string &pg="") const;
	/** The algorithms's main loop.
		Performs iterations until the termination criterion is
		fulfilled.
		Called for a stand-alone algorithm, but never if used as island. */
	virtual void run();
	/** Performs a single iteration.
		Is called from run(); is also called if used as island. */
	virtual void performIteration() = 0;
	/** Performs crossover on the given solutions and updates
		statistics. */
	virtual bool terminate();
	/** Returns an index for a solution to be replaced.
		According to the used replacement strategy (parameter repl),
		an index is returned.
		Replacement strategies: 0: random, 1: worst. */
	virtual int replaceIndex();
	/** Replaces a solution in the population.
		The given solution replaces another one in the population,
		determined by the replaceIndex method.
		The replaced solution is returned.
		Duplicate elimination is performed if #dupelim is set. */
	virtual mh_solution *replace(mh_solution *);
	/** Updates the solution with the given index by copying it from *sol. */
	virtual void update(int index, mh_solution *sol);
	/** Print statistic informations.
		Prints out various statistic informations including
		the best solution of the population.. */
	virtual void printStatistics(std::ostream &ostr);
	/** Writes the log entry for the current iteration.
		If inAnyCase is set, then the entry is written in any case. */
	virtual void writeLogEntry(bool inAnyCase=false);
	/** Writes the log header */
	virtual void writeLogHeader();
	/** Returns pointer to best solution obtained so far. */
	mh_solution *getBestSol() const
		{ return pop->bestSol(); }
	/** Returns number of the current iteration. */
	virtual int getIter(void)
		{ return nIteration; }
	/** Returns iteration in which best solution was generated. */
	virtual int getIterBest(void)
		{ return iterBest; }
	/** Returns time at which best solution was generated. */
	virtual double getTimeIterBest(void)
		{ return timIterBest; }
	/** Performs a classical tournament.
		The group size is tournk and randomly chosen solutions can
		be chose multiple times. */
	int tournamentSelection();

protected:
	/** Exits with error if no population is set. */
	void checkPopulation();
	/** Saves the best objective value. */
	virtual void saveBest();
	/** Checks to see whether the best objective value has changed
		and updated iterBest and timIterBest if so. */
	virtual void checkBest();
	/** Adds statistics from a subalgorithm. */
	void addStatistics(const mh_advbase *a);
	
	/** Method called at the begin of performIteration(). */
	virtual void perfIterBeginCallback(){};
	
	/** Method called at the end of performIteration(). */
	virtual void perfIterEndCallback(){};
};

} // end of namespace mh

#endif // MH_ADVBASE_H
