/*! \file mh_tabusearch.h
  \brief Tabu search.

*/

#ifndef MH_TABUSEARCH_H
#define MH_TABUSEARCH_H

#include "mh_interfaces.h"
#include "mh_lsbase.h"
#include "mh_tabulist.h"

namespace mh {

/** Tabu search.
	During each iteration the best neighbour of the current solution
	is searched with respect to its tabustate and an aspieration
	criterion. */
class tabuSearch : public lsbase, public tabulistProvider
{
protected:
	/** The current active chromosome.
	        This must not necessarily be the best known chromosome so
		far, therefore it is not in the population. */
	mh_solution *curChrom = NULL;
	
public:
	/** A tabulist containing tabuattributes which are tabu. */
	tabulist *tl_ne = NULL;

	/** The constructor.
	        An initialized population already containing chromosomes 
		must be given. Note that the population is NOT owned by the 
		algorithm, and will not be deleted by its destructor. */
	tabuSearch(pop_base &p, const std::string &pg="");
	/** Another constructor.
		Creates an empty Algorithm that can only be used as a template. */
	tabuSearch(const std::string &pg="") : lsbase(pg) {}
	/** The destructor. */
	~tabuSearch();
	/** Create new simulatedAnnealing.
		Returns a pointer to a new simulatedAnnealing. */
	mh_advbase *clone(pop_base &p, const std::string &pg="")
		{ return new tabuSearch(p,pg); }
	/** Performs a single generation.
		Is called from run() */
	virtual void performIteration();
	/** Checks if a tabu is currently tabu.
	        Is called from chromsome::neighbour() */
	virtual bool isTabu(tabuAttribute *t);
	/** Checks if a chromsome can overide its tabustate. */
	virtual bool aspiration(mh_solution *c);
};

} // end of namespace mh

#endif //MH_TABUSEARCH_H
