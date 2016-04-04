/*! \file mh_localsearch.h 
	\brief Simple local search. */ 

#ifndef MH_LOCALSEARCH_H
#define MH_LOCALSEARCH_H

#include "mh_interfaces.h"
#include "mh_lsbase.h"
#include "mh_param.h"

namespace mh {

/** \ingroup param
    neighbor selection function to use
    - 0: random neighbor,
    - 1: next improvement,
    - 2: best improvement. */
extern int_param mvnbop;

/** Simple local search.
	During each iteration, a move in the neighborhood is performed
	and the current solution is replaced if the new solution is not
	worse. */
class localSearch : public lsbase, public glsSubAlgorithm
{
public:
	/** The constructor.
		An initialized population already containing solutions
		must be given. Note that the population is NOT owned by the 
		algorithm and will not be deleted by its destructor. 
		localSearch always only uses the first solution. */
	localSearch(pop_base &p, const std::string &pg="") : lsbase(p,pg) {};
	/** Another constructor.
		Creates an empty EA that can only be used as a template. */
	localSearch(const std::string &pg="") : lsbase(pg) {};
	/** Create new localSearch.
		Returns a pointer to a new localSearch. */
	mh_advbase *clone(pop_base &p, const std::string &pg="")
	    { return new localSearch(p,pg); }
	/** Performs a single generation. */
	void performIteration();
};

} // end of namespace mh

#endif //MH_LOCALSEARCH_H
