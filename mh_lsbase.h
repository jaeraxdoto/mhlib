/*! \file mh_lsbase.h 
	\brief An abstract base class for local search alike algorithms. */ 

#ifndef MH_LSBASE_H
#define MH_LSBASE_H

#include "mh_advbase.h"
#include "mh_param.h"

/** An abstract base class for local search alike algorithms. */
class lsbase : public mh_advbase
{
public:
	/** The constructor.
		An initialized population already containing chromosomes 
		must be given. Note that the population is NOT owned by the 
		algorithm and will not be deleted by its destructor. */
	lsbase(pop_base &p, const pstring &pg=(pstring)(""));
	/** Another constructor.
		Creates an empty EA that can only be used as a template. */
	lsbase(const pstring &pg=(pstring)("")) : mh_advbase(pg) {};
	/** Replaces the first chromosome in the population by p. */
	mh_solution *replace(mh_solution *p);
};

#endif //MH_LSBASE_H
