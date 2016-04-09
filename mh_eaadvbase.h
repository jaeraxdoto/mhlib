/*! \file mh_eaadvbase.h
	\brief An abstract base class for evolutionary algorithm like metaheuristics.

	This module contains definitions which are common in many
	evolutionary algorithms. */

#ifndef MH_EAADVBASE_H
#define MH_EAADVBASE_H

#include "mh_advbase.h"

namespace mh {

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

/** The abstract base class for evolutionary algorithms like metaheuristics.
	This abstract base contains methods supporting e.g. mutation and crossover. */
class mh_eaadvbase : public mh_advbase
{
public:
	int nCrossovers = 0;	///< Number of performed crossovers
	int nMutations = 0;		///< Number of performed mutations
	/** Number of crossovers that didn't result in a new solution.
		Only used when parameter cntopd() is set. */
	int nCrossoverDups = 0;
	/** Number of mutations that didn't result in a new solution.
		Only used when parameter cntopd() is set. */
	int nMutationDups = 0;
	/** Number of performed local improvements excl. those in
		initialization. */
	int nLocalImprovements = 0;
	/** Number of solutions that were tabu. */
	int nTabus = 0;
	/** Number of solutions that were accepted due to an aspiration criterion. */
	int nAspirations = 0;
	/** Number of not better solutions that were accepted due to the acceptance criterion. */
	int nDeteriorations = 0;

	/** The constructor.
		An initialized population already containing solutions
		must be given. Note that the population is NOT owned by the 
		EA and will not be deleted by its destructor. */
	mh_eaadvbase(pop_base &p, const std::string &pg="") : mh_advbase(p,pg) {}
	/** Another constructor.
		Creates an empty EA that can only be used as a template. */
	mh_eaadvbase(const std::string &pg="") : mh_advbase(pg) {}
	/** The destructor.
		It does delete the temporary solution, but not the
		population. */
	virtual ~mh_eaadvbase() {}
	/** Performs crossover on the given solutions and updates
		statistics. */
	void performCrossover(mh_bare_solution *p1, mh_bare_solution *p2,
		mh_bare_solution *c);
	/** Performs mutation on the given solution with the given
		probability and updates statistics. */
	void performMutation(mh_bare_solution *c, double prob);
	/** Print statistic informations.
		Prints out various statistic informations including
		the best solution of the population.. */
	virtual void printStatistics(std::ostream &ostr);

protected:
	/** Adds statistics from a subalgorithm. */
	void addStatistics(const mh_advbase *a);
	
	/** Method called at the begin of performIteration(). */
	virtual void perfIterBeginCallback(){};
	
	/** Method called at the end of performIteration(). */
	virtual void perfIterEndCallback(){};
};

} // end of namespace mh

#endif //MH_ADVBASE_H
