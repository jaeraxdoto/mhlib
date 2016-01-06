/*! \file mh_advbase.h 
	\brief An abstract base class for metaheuristics.

	This module contains definitions which are common in many metaheuristics. */

#ifndef MH_ADVBASE_H
#define MH_ADVBASE_H

#include "mh_base.h"
#include "mh_param.h"
#include "mh_popbase.h"

namespace mhlib {

/** \defgroup param Global parameters */

/** \ingroup param 
	The termination condition (DEPRECATED).
	Decides the strategy used as termination criterion:
	- -1: terminate when #titer>0 && #titer iterations are
	  reached or when #tciter>0 && #tciter iterations without
	  improvement or when #tobj>0 && best solution reaches #tobj
	  or when #ttime>0 && effective runtime reaches #ttime.

	Depracated values:
	- 0: terminate after #titer iterations.
	- 1: terminate after convergence, which is defined as:
		the objective value of the best solution in the population
		did not change within the last titer (not #tciter!)
		iterations.
	- 2: terminate when the given objective value limit (tobj()) is 
		reached. */
extern int_param tcond;

/** \ingroup param
	The number of iterations until termination. Active if >=0. */
extern int_param titer;

/** \ingroup param 
	The number of iterations for termination according to convergence. Active if >=0. */
extern int_param tciter;

/** \ingroup param
	The objective value for termination when #tcond==2. Active if >=0. */
extern double_param tobj;

/** \ingroup param
	The time limit for termination. Active if >=0. */
extern double_param ttime;

/** \ingroup param
	Group size for tournament selection. */
extern int_param tselk;

/** \ingroup param
	Replacement scheme:
	- 0: A new solutionosome replaces a randomly chosen existing solution
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
	The crossover probability.
	Probability for generating a new solution by crossover. */
extern double_param pcross;

/** \ingroup param
	Probability/rate of mutating a new solution.

	If #pmut is negative, its absolute value is interpreted as an
	average value per solution instead of a fixed rate, and for each
	gene it is randomly decided if mutation takes place or not. 
	(More precisely, the actual number of mutations is determined via
	a Poisson-distributed random number.)

	If #pmut<1000, a Poisson-distribution with mean |#pmut -
	1000| is also used, but in addition, it is assured that at least
	one mutation is performed.

	Note further, that in case of a steady state algorithm, there is
	the additional parameter pmutnc that is used when an individual is
	not created by crossover. However, the value of #pmut is inherited
	if #pmutnc==0. */
extern double_param pmut;

/** \ingroup param
	Probability with which locallyImprove is called for a new
	solution. */
extern double_param plocim;

/** \ingroup param
	Count operator duplicates. If set, the metaheuristic counts how often
	crossover and mutation creates only duplicates of the parents
	(nCrossoverDups, nMutationDups). */
extern bool_param cntopd;

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

/** The abstract base class for metaheuristics.
	This abstract base contains methods and attributes that are needed in
	order to use an algorithm as sub-algorithm in an island model.
	If you derive a new algorithm, use mh_advbase as the base class, if no other,
	derived class suits your needs. */
class mh_advbase : public mh_base
{
public:
	/** The population of the metaheuristic.
		It is not owned by the metaheuristic and therefore not deleted by it. */
	pop_base *pop;
	/** The constructor.
		An initialized population already containing solutions
		must be given. Note that the population is NOT owned by the 
		EA and will not be deleted by its destructor. */
	mh_advbase(pop_base &p, const pstring &pg=(pstring)(""));
	/** Another constructor.
		Creates an empty EA that can only be used as a template. */
	mh_advbase(const pstring &pg=(pstring)(""));
	/** The destructor.
		It does delete the temporary solution, but not the
		population. */
	virtual ~mh_advbase();
	/** Create new object of same class.
		Virtual method, uses the classes constructor to create a
		new EA object of the same class as the called object. */
	virtual mh_advbase *clone(pop_base &p, const pstring &pg=(pstring)(""));
	/** The EA's main loop.
		Performs iterations until the termination criterion is
		fulfilled.
		Called for a stand-alone EA, but never if used as island. */
	virtual void run();
	/** Performs a single iteration.
		Is called from run(); is also called if used as island. */
	virtual void performIteration() = 0;
	/** Performs crossover on the given solutions and updates
		statistics. */
	void performCrossover(mh_solution *p1,mh_solution *p2,
		mh_solution *c);
	/** Performs mutation on the given solution with the given
		probability and updates statistics. */
	void performMutation(mh_solution *c,double prob);
	/** The termination criterion.
		Calls a concrete termination functions and returns true
		if the algorithm should terminate. */
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
	
public:
	int nIteration;	///< Number of current iteration.
	int nSubIterations;	///< Number of iterations in subalgorithms.
	int nSelections;	///< Number of performed selections
	int nCrossovers;	///< Number of performed crossovers
	int nMutations;		///< Number of performed mutations
	int nDupEliminations;	///< Number of performed duplicate eliminations
	/** Number of crossovers that didn't result in a new solution. 
		Only used when parameter cntopd() is set. */
	int nCrossoverDups;
	/** Number of mutations that didn't result in a new solution. 
		Only used when parameter cntopd() is set. */
	int nMutationDups;
	/** Number of performed local improvements excl. those in 
		initialization. */
	int nLocalImprovements;
	/** Number of solutions that were tabu. */
	int nTabus;
	/** Number of solutions that were accepted due to an aspiration criterion. */
	int nAspirations;
	/** Number of solutions that were accepted due to the acceptance criterion. */
	int nDeteriorations;

protected:
	int iterBest;	    ///< Iteration in which best solution was generated.
	double timIterBest;  ///< Time at which best solution was generated.
	
	// other class variables
	mh_solution *tmpSol;	///< a temporary solution in which the result of operations is stored
	
	double bestObj;		///< temporary best objective value
	double timStart;        ///< CPUtime when run() was called

	bool _wctime;	///< Mirrored mhlib parameter wall_clock_time for performance reasons.
};

} // end of namespace mhlib

#endif //MH_ADVBASE_H
