/*! \file mh_popbase.h 
	\brief An abstract base class for populations. */ 

#ifndef MH_POPBASE_H
#define MH_POPBASE_H

#include "mh_solution.h"
#include "mh_param.h"
#include "mh_popsupp.h"
#include "mh_random.h"

/** The population size.
	The number of chromosomes the population contains. */
extern int_param popsize;

/** Duplicate elimination.
	If set to >1, new solutions are checked if they are duplicates 
	of existing solutions, in which case they are not included in the
	population. The initial solutions are also checked against each
	other and reinitialized until they are all different if 
	this parameter is set to 2. */
extern int_param dupelim;


class pophashtable;


/** The EA population base.
	This is the abstract base class for an EA population which provides
	an interface for accessing a number of chromosomes by an algorithm. */
class pop_base
{
private:
	/** The Init-Method
	        Initializes the statistics fields and hashtable, if used. */
	void init(int psize);

protected:
	/// size of the chromosome array
	int nChroms;
	/** index to best chromosome of population.
		Is always kept up to date. */
	int indexBest;
	/** determines indexBest for the whole population.
		Called when best gets lost or after initialization. (O(n)). */
	virtual void determineBest() = 0;
	/** determines Index of worst chromosome.
		This is guaranteed to be never indexBest. (O(n)). */
	virtual int determineWorst() const = 0;
	/// mean objective value
	double statMean;
	/// standard deviation of fitness value
	double statDev;
	/// worst objective value
	double statWorst;
	/// true if all stat* variables are valid
	bool statValid;
	/** a hash-table containing all population members.
		Important for faster access when the population is large. 
		This object is only created when dupelim() is true;
		otherwise phash==0. */
	pophashtable *phash;

	/// Parametergroup
	string pgroup;
	
public:
	/** The Constructor.
	        Calls the Init-Method. */
	pop_base(int psize, const pstring &pg=(pstring)("")) : pgroup(pg.s) { init( psize ); }
	/** The Constructor.
  	        Calls the Init-Method. */
	pop_base(const pstring &pg=(pstring)("") ) : pgroup(pg.s) { init( popsize(pgroup) ); }
	/** Destructor. */
	virtual ~pop_base();
	/** Size of population.
		The chromosomes are indexed from 0 to size()-1. */
	int size() const
		{ return nChroms; }
	/** Get Chromosome via given index.
		The chromosome must not be modified or deleted! */
	virtual mh_solution *at(int index) = 0;
	/** Replaces a chromosome at a specific index with another one.
		The caller has to take care to delete or store the returned
		prior chromosome. */
	virtual mh_solution *replace(int index,mh_solution *newchrom) = 0;
	/** Index of best chromosome in population. */
	int bestIndex() const
		{ return indexBest; }
	/** Returns pointer to best chromosome of population. */
	virtual mh_solution *bestSol() const = 0;
	/** Returns objective value of best chromosome. */
	double bestObj()
		{ return bestSol()->obj(); }
	/** Index of worst chromosome in population. */
	int worstIndex() const
		{ return determineWorst(); }
	/** Index of a uniformly, randomly chosen chromosome. */
	int randomIndex()
		{ return random_int(nChroms); }
	/** Checks wheter the given chromosome has a duplicate in 
		the population.	Returns the index in the population in this
		case; otherwise, -1 is returned. */
	virtual int findDuplicate(mh_solution *p) = 0;
	/** Write out population on ostream.
		Usually used for debugging purposes. */
	virtual void write(ostream &ostr) = 0;
	/** Determines mean objective value of population. */
	double getMean()
		{ validateStat(); return statMean; }
	/** Determines the worst objective value of population. */
	double getWorst()
		{ validateStat(); return statWorst; }
	/** Determines deviation of objective values of population. */
	double getDev()
		{ validateStat(); return statDev; }
	/** Validate all statistic data of population.
		If the current statistic data are not valid
		(!statValid), then determine them. */
	virtual void validateStat() = 0;
	/** Set the algorithm for all chromosomes of the population.
	        An algorithm should call this before using the chromosomes
		of the population, to let the chromosomes know what algorithm
		is using them. */
	virtual void setAlgorithm(mh_base *alg) = 0;
};

#endif //MH_POPBASE_H
