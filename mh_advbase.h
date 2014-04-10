/*! \file mh_advbase.h 
	\brief An abstract base class for metaheuristics.

	This module contains definitions which are common in many metaheuristics. */

#ifndef MH_ADVBASE_H
#define MH_ADVBASE_H

#include "mh_base.h"
#include "mh_param.h"
#include "mh_popbase.h"

/** \defgroup param Global parameters */

/** \ingroup param 
	The termination condition (DEPRECATED).
	Decides the strategy used as termination criterion:
	- -1: terminate when #tgen>0 && #tgen generations are
	  reached or when #tcgen>0 && #tcgen generations without
	  improvement or when #tobj>0 && best solution reaches #tobj
	  or when #ttime>0 && effective runtime reaches #ttime.

	Depracated values:
	- 0: terminate after #tgen generations.
	- 1: terminate after convergence, which is defined as:
		the objective value of the best solution in the population
		did not change within the last tgen (not #tcgen!) 
		generations. 
	- 2: terminate when the given objective value limit (tobj()) is 
		reached. */
extern int_param tcond;

/** \ingroup param
	The number of generations until termination. Active if >=0. */
extern int_param tgen;

/** \ingroup param 
	The number of generations for termination according to convergence. Active if >=0. */
extern int_param tcgen;

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
	- 0: A new chromosome replaces a randomly chosen existing chromosome
		with the exception of the best solution.
	- 1: A new chromosome replaces the worst existing solution.
	- -k: The chromosome to be replaced is selected via a tournament
		selection of group size k (with replacement of actual).

	In addition, duplicate eliminiation takes place according to
	parameter #dupelim. */ 
extern int_param repl;

/** \ingroup param
	Control output of logging.
	If #logdups==1, the current number of eliminated duplicates is
	printed as last entry in the log. */
extern bool_param logdups;

/** \ingroup param
	Control output of logging.
	If #logcputime is set, the elapsed cpu time is printed as last
	entry in the log. */
extern bool_param logcputime;

/** \ingroup param
	The crossover probability.
	Probability for generating a new chromosome by crossover. */
extern double_param pcross;

/** \ingroup param
	Probability/rate of mutating a new chromosome. 

	If #pmut is negative, its absolute value is interpreted as an
	average value per chromosome instead of a fixed rate, and for each
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
	chromosome. */
extern double_param plocim;

/** \ingroup param
	Count operator duplicates. If set, the metaheuristic counts how often
	crossover and mutation creates only duplicates of the parents
	(nCrossoverDups, nMutationDups). */
extern bool_param cntopd;

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
		An initialized population already containing chromosomes 
		must be given. Note that the population is NOT owned by the 
		EA and will not be deleted by its destructor. */
	mh_advbase(pop_base &p, const pstring &pg=(pstring)(""));
	/** Another constructor.
		Creates an empty EA that can only be used as a template. */
	mh_advbase(const pstring &pg=(pstring)(""));
	/** The destructor.
		It does delete the temporary chromosome, but not the 
		population. */
	virtual ~mh_advbase();
	/** Create new object of same class.
		Virtual method, uses the classes constructor to create a
		new EA object of the same class as the called object. */
	virtual mh_advbase *clone(pop_base &p, const pstring &pg=(pstring)(""));
	/** The EA's main loop.
		Performs generations until the termination criterion is
		fullfilled.
		Called for a stand-alone EA, but never if used as island. */
	virtual void run();
	/** Performs a single generation.
		Is called from run(); is also called if used as island. */
	virtual void performGeneration() = 0;
	/** Performs crossover on the given chromosomes and updates 
		statistics. */
	void performCrossover(mh_solution *p1,mh_solution *p2,
		mh_solution *c);
	/** Performs mutation on the given chromosome with the given
		probability and updates statistics. */
	void performMutation(mh_solution *c,double prob);
	/** The termination criterion.
		Calls a concerete termination functions and returns true
		if the algorithm should terminate. */
	virtual bool terminate();
	/** Returns an index for a chromosome to be replaced.
		According to the used replacement strategy (parameter repl),
		an index is returned.
		Replacement strategies: 0: random, 1: worst. */
	virtual int replaceIndex();
	/** Replaces a chromosome in the population.
		The given chromosome replaces another one in the population,
		determined by the replaceIndex method.
		The replaced chromosome is returned. 
		Duplicate elimination is performed if #dupelim is set. */
	virtual mh_solution *replace(mh_solution *);
	/** Print statistic informations.
		Prints out various statistic informations including
		the best chromosome of the population.. */
	virtual void printStatistics(ostream &ostr);
	/** Writes the log entry for the current generation.
		If inAnyCase is set, then the entry is written in any case. */
	virtual void writeLogEntry(bool inAnyCase=false);
	/** Writes the log header */
	virtual void writeLogHeader();
	/** Returns pointer to best solution obtained so far. */
	mh_solution *getBestChrom() const
		{ return pop->bestSol(); }
	/** Returns number of generation */
	virtual int getGen(void)
		{ return nGeneration; }      
	/** Returns generation in which best chrom was generated */
	virtual int getGenBest(void)
		{ return genBest; }
	/** Returns time at which best chrom was generated */
	virtual double getTimeGenBest(void)
		{ return timGenBest; }       
	/** Performs a classical tournament.
		The group size is tournk and randomly chosen chromosoms can
		be chose multiple times. */
	int tournamentSelection();

protected:
	/** Exits with error if no population is set. */
	void checkPopulation();
	/** Saves the best objective value. */
	virtual void saveBest();
	/** Checks to see wether the best objective value has changed
		and updated genBest and timGenBest if so. */
	virtual void checkBest();
	/** Adds statistics from a subalgorithm. */
	void addStatistics(const mh_advbase *a);
	
	/** Method called at the begin of performGeneration(). */
	virtual void perfGenBeginCallback(){};
	
	/** Method called at the end of performGeneration(). */
	virtual void perfGenEndCallback(){};
	
public:
	int nGeneration;	///< Number of generations
	int nSubGenerations;	///< Number of generations in subalgorithms
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
	/** Number of solutions that were acception due to an aspiration criterion. */
	int nAspirations;
	/** Number of solutions that were acception due to the acceptance criterion. */
	int nDeteriorations;
	
protected:
	int genBest;	    ///< Generation in which best chrom was generated
	double timGenBest;  ///< Time at which best chrom was generated
	
	// other class variables
	mh_solution *tmpChrom;	// a temporary chromosome
	
//private:
	double bestObj;		// temporary best objective value
	double timStart;        // CPUtime when run() was called
};

#endif //MH_ADVBASE_H
