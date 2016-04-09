/*! \file mh_subpop.h 
	\brief The sub-population class. */

#ifndef MH_SUBPOP_H
#define MH_SUBPOP_H

#include "mh_pop.h"
#include "mh_popbase.h"

namespace mh {

/** The island model EA sub-population.
	This class implements a sub-population for use in the island model. */
class subPopulation : public pop_base
{
protected:
	/** The super-population.
		This is used to get the solutions, the sub-population only
		saves the indices. */
	pop_base *superPopulation;
	/** Indices in the super-population. */
	int indexFrom;
	int indexTo;
	/** determines indexBest for the sub-population.
		Called when best gets lost or after initialization. (O(n)). */
	virtual void determineBest();
	/** determines Index of worst solution.
		This is guaranteed to be never indexBest. (O(n)). */
	virtual int determineWorst() const;
public:
	/** The constructor.
		Creates a new sub-population for the given population, using the
		solutions in the given range. */
	subPopulation(pop_base *super,int from,int to, const std::string &pg="");
	/** Get Chromosome via given index.
		The solution must not be modified or deleted! */
	virtual mh_solution *at(int index)
		{ return superPopulation->at(indexFrom+index); }
	/** Replaces a solution at a specific index with another one.
		The caller has to take care to delete or store the returned
		prior solution. */
	virtual mh_solution *replace(int index, mh_solution *newchrom);
	/** Returns pointer to best solution of population. */
	virtual mh_solution *bestSol() const
		{ return superPopulation->at(indexFrom+indexBest); }
	/** Checks wheter the given solution has a duplicate in
		the population.	Returns the index in the population in this
		case; otherwise, -1 is returned. */
	virtual int findDuplicate(mh_solution *p);
	/** Write out population on ostream.
		Usually used for debugging purposes. */
	virtual void write(std::ostream &ostr);
	/** Validate all statistic data of population.
		If the current statistic data are not valid
		(!statValid), then determine them. */
	virtual void validateStat();
	/** Set the algorithm for all solutions of the population. */
	virtual void setAlgorithm(mh_base *alg);
};

} // end of namespace mh

#endif //MH_SUBPOP_H
