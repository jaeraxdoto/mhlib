/*! \file mh_allalgs.h 
	\brief A function creating any of the implemented algorithms
	in dependence of a parameter.
*/

#ifndef MH_ALLALGS_H
#define MH_ALLALGS_H

#include "mh_base.h"
#include "mh_advbase.h"
#include "mh_param.h"
#include "mh_popbase.h"

namespace mhlib {

/** \defgroup param Global parameters */

/** \ingroup param 
	The actual metaheuristic to use: 
	- 0: steady-state EA,  
	- 1: generational EA,
	- 2: steady-state EA with island model,
	- 3: generational EA with island model,
	- 4: simple randomized local search,
	- 5: simulated annealing,
	- 6: tabu search,
	- 7: greedy randomized adaptive search procedure,
	- 8: guided local search.
	- 9: general variable neighborhood search
	- 10: variable neighborhood descent */
extern int_param mhalg;


/** Creation of any of the implemented algorithms.
    Depending on the parameter a, a specific instance of a
    metaheuristic is created.  */
	mh_advbase *create_mh(pop_base &p,int a, const pstring &pg=(pstring)(""));
/** Creation of any of the implemented algorithms.
    Depending on the parameter mhalg(), a specific instance of a
    metaheuristic is created.  */
	inline mh_advbase *create_mh(pop_base &p, const pstring &pg=(pstring)(""))
	{ return create_mh(p,mhalg(pg.s),pg); }

} // end of namespace mhlib

#endif //MH_ALLALGS_H
