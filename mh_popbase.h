/*! \file mh_popbase.h 
	\brief An abstract base class for populations. */ 

#ifndef MH_POPBASE_H
#define MH_POPBASE_H

#include "mh_solution.h"
#include "mh_param.h"
#include "mh_popsupp.h"
#include "mh_random.h"

namespace mhlib {

/** The population size.
	The number of solutions the population contains. */
extern int_param popsize;

/** Duplicate elimination.
	If set to >1, new solutions are checked if they are duplicates 
	of existing solutions, in which case they are not included in the
	population. The initial solutions are also checked against each
	other and reinitialized until they are all different if 
	this parameter is set to 2. */
extern int_param dupelim;


class pophashtable;


/** The population base.
	This is the abstract base class for an algorithm's population which provides
	an interface for accessing a number of candidate solutions by an algorithm. */
class pop_base
{
private:
	/** The Init-Method.
	        Initializes the statistics fields and hashtable, if used.
	        If nohashing is set, no hashtable and worst heap will be used.
	        Otherwise, this depends on the dupelim parameter etc. */
	void init(int psize, bool nohashing=false);

protected:
	/// size of the solutions array
	int nSolutions;
	/** index to best solution of population.
		Is always kept up to date. */
	int indexBest;
	/** determines indexBest for the whole population.
		Called when best gets lost or after initialization. (O(n)). */
	virtual void determineBest() = 0;
	/** determines Index of worst solution.
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
	std::string pgroup;
	
public:
	/** The Constructor.
	        Calls the Init-Method with default population size according to parameter popsize. */
	pop_base(int psize, const std::string &pg="") : pgroup(pg) { init( psize ); }

	/** The Constructor.
  	        Calls the Init-Method with a specific population size. */
	pop_base(const std::string &pg="" ) : pgroup(pg) { init( popsize(pgroup) ); }

	/** The Constructor.
	        Calls the Init-Method with the population size and nohashing parameter. */
	pop_base(int psize, bool nohashing = false, const std::string &pg="") : pgroup(pg) { init( psize ); }

	/** Destructor. */
	virtual ~pop_base();
	/** Size of population.
		The solutions are indexed from 0 to size()-1. */
	int size() const
		{ return nSolutions; }
	/** Get solution via given index.
		The solution must not be modified or deleted! */
	virtual mh_solution *at(int index) = 0;
	mh_solution *operator[](int index)
		{ return at(index); }
	/** Replaces a solution at a specific index with another one.
		The caller has to take care to delete or store the returned
		prior solution. */
	virtual mh_solution *replace(int index,mh_solution *newchrom) = 0;
	/** Copy the given solution into the solution at position index in
	 * the population and update population data.
	 */
	virtual void update(int index, mh_solution *newchrom) {
		mherror("Update not supported in popbase");
	}
	/** Index of best solution in population. */
	int bestIndex() const
		{ return indexBest; }
	/** Returns pointer to best solution of population. */
	virtual mh_solution *bestSol() const = 0;
	/** Returns objective value of best solution. */
	double bestObj()
		{ return bestSol()->obj(); }
	/** Index of worst solution in population. */
	int worstIndex() const
		{ return determineWorst(); }
	/** Index of a uniformly, randomly chosen solution. */
	int randomIndex()
		{ return random_int(nSolutions); }
	/** Checks wheter the given solution has a duplicate in
		the population.	Returns the index in the population in this
		case; otherwise, -1 is returned. */
	virtual int findDuplicate(mh_solution *p) = 0;
	/** Write out population on ostream.
		Usually used for debugging purposes. */
	virtual void write(std::ostream &ostr) = 0;
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
	/** Set the algorithm for all solutions of the population.
	        An algorithm should call this before using the solutions
		of the population, to let the solutions know what algorithm
		is using them. */
	virtual void setAlgorithm(mh_base *alg) = 0;
};

} // end of namespace mhlib

#endif //MH_POPBASE_H
