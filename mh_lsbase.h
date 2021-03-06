/*! \file mh_lsbase.h 
	\brief An abstract base class for local search alike algorithms. */ 

#ifndef MH_LSBASE_H
#define MH_LSBASE_H

#include "mh_eaadvbase.h"
#include "mh_param.h"
#include "mh_solution.h"

namespace mh {

/** An abstract base class for local search alike algorithms. */
class lsbase : public mh_eaadvbase
{
public:
	/** The constructor.
		An initialized population already containing solutions
		must be given. Note that the population is NOT owned by the 
		algorithm and will not be deleted by its destructor. */
	lsbase(pop_base &p, const std::string &pg="");
	/** Another constructor.
		Creates an empty EA that can only be used as a template. */
	lsbase(const std::string &pg="") : mh_eaadvbase(pg) {};
	/** Replaces the first solution in the population by p. */
	mh_solution *replace(mh_solution *p);
};

} // end of namespace mh

#endif // MH_LSBASE_H
