/*! \file mh_pop.h 
	\brief The population class. */ 

#ifndef MH_POP_H
#define MH_POP_H

#include "mh_solution.h"
#include "mh_param.h"
#include "mh_popbase.h"

namespace mhlib {

/** \ingroup param
	The population size.
	The number of solutions the population contains. */
extern int_param popsize;

/** \ingroup param
	Duplicate elimination.
	If set to >1, new solutions are checked if they are duplicates 
	of existing solutions, in which case they are not included in the
	population. The initial solutions are also checked against each
	other and reinitialized until they are all different if 
	this parameter is set to 2. */
extern int_param dupelim;


/** The optimization algorithm's population.
	This is the base class for an optimization algorithm's
	population which provides the most
	elementary things for holding a number of solutions.
	It already contains necessary methods for an efficient
	implementation of a steady-state evol. algorithm, although it does not depend on
	a specific algorithm class.
	Classes with various special methods such as caching
	and more efficient data structures for certain
	methods can be derived. */
class population : public pop_base
{
protected:
	/// the solution array
	mh_solution **chroms;
	/** determines indexBest for the whole population.
		Called when best gets lost or after initialization. (O(n)). */
	void determineBest();
	/** determines Index of worst solution.
		This is guaranteed to be never indexBest. (O(n)). */
	int determineWorst() const;
public:
	/** A population of solutions is created.
		The size is given as parameter psize.
		The template solution is used to create solutions of the same type.
		If binit is set, the new solutions are all initialized, otherwise they
		are a copy of the template solution.
		If nohashing is set, hashing is avoided in the population; otherwise it depends on dupelim(). */
	population(const mh_solution &c_template, int psize, bool binit, bool nohashing=false, const std::string &pg="");
	/** A population of solutions is created.
		The size is taken from popsize().
		The template solution is used to create solutions of the same type,
		which are finally all initialized by calling initialize(). */
	population(const mh_solution &c_template, const std::string &pg="");
	
	/** Destructor.
		The population and all its contained solutions are deleted. */
	virtual ~population();
	/** Initialization of the population.
		Implicitly called by the constructor.
		For each solution contained in the population, its
		method initialize() is called. Local improvement is by
		default not performed. It must eventually
		be explicitly done in the solution's initialize function.  */
	virtual void initialize();
	/** Get solution via given index.
		The solution must not be modified or deleted! */
	mh_solution *at(int index)
		{ return chroms[index]; }
	/** Replaces a solution at a specific index with another one.
		The caller has to take care to delete or store the returned
		prior solution. Population data is updated. */
	mh_solution *replace(int index,mh_solution *newchrom);
	/** Copy the given solution into the solution at position index in
	 * the population and update population data.
	 */
	void update(int index, mh_solution *newchrom);
	/** Returns pointer to best solution of population. */
	mh_solution *bestSol() const
		{ return chroms[indexBest]; }
	/** Checks wheter the given solution has a duplicate in
		the population.	Returns the index in the population in this
		case; otherwise, -1 is returned. */
	int findDuplicate(mh_solution *p);
	/** Write out population on ostream.
		Usually used for debugging purposes. */
	void write(std::ostream &ostr);
	/** Validate all statistic data of population.
		If the current statistic data are not valid
		(!statValid), the determine them. */
	virtual void validateStat();
	/** Set the algorithm for all solutions of the population. */
	virtual void setAlgorithm(mh_base *alg);
};

} // end of namespace mhlib

#endif // MH_POP_H
