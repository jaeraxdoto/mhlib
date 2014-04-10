/*! \file mh_localsearch.h 
	\brief Simple local search. */ 

#ifndef MH_LOCALSEARCH_H
#define MH_LOCALSEARCH_H

#include "mh_interfaces.h"
#include "mh_lsbase.h"
#include "mh_param.h"

/** Simple local search.
	During each iteration, a move in the neighbourhood is performed
	and the current solution is replaced if the new solution is not
	worse. */
class localSearch : public lsbase, public glsSubAlgorithm
{
public:
	/** The constructor.
		An initialized population already containing chromosomes 
		must be given. Note that the population is NOT owned by the 
		algorithm and will not be deleted by its destructor. 
		localSearch always only uses the first chromosome. */
	localSearch(pop_base &p, const pstring &pg=(pstring)("")) : lsbase(p,pg) {};
	/** Another constructor.
		Creates an empty EA that can only be used as a template. */
	localSearch(const pstring &pg=(pstring)("")) : lsbase(pg) {};
	/** Create new localSearch.
		Returns a pointer to a new localSearch. */
	mh_advbase *clone(pop_base &p, const pstring &pg=(pstring)(""))
	    { return new localSearch(p,pg); }
	/** Performs a single generation. */
	void performGeneration();
};

#endif //MH_LOCALSEARCH_H
