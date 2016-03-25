/*! \file mh_simanneal.h
  \brief Simulated annealing.
*/

#ifndef MH_SIMANNEAL_H
#define MH_SIMANNEAL_H

#include "mh_lsbase.h"

namespace mh {

/** \ingroup param
    Slope for geometric cooling scheme method.

    g(T,t) = T * saca, saca < 1
 */
extern double_param saca;

/** \ingroup param
    Interval between cooling steps.
    Defines after how many iterations the cooling scheme method
    should be called.
 */
extern int_param sacint;

/** \ingroup param
    Initial temperature for simulated annealing.
 */
extern double_param satemp;

/** Simulated annealing.
	During each iteration, a move in the neighbourhood is performed
	and the current solution is replaced if the new solution is not
	worse, or if an acceptance method allows the move. */
class simulatedAnnealing : public lsbase
{
protected:
	/** Temperature of the annealing process. */
	double T=0;

public:
	/** The constructor.
		An initialized population already containing chromosomes 
		must be given. Note that the population is NOT owned by the 
		algorithm and will not be deleted by its destructor. */
	simulatedAnnealing(pop_base &p, const std::string &pg="");
	/** Another constructor.
		Creates an empty Algorithm that can only be used as a template. */
	simulatedAnnealing(const std::string &pg="") : lsbase(pg) {}
	/** Create new simulatedAnnealing.
		Returns a pointer to a new simulatedAnnealing. */
	mh_advbase *clone(pop_base &p, const std::string &pg="")
		{ return new simulatedAnnealing(p,pg); }
	/** Performs a single generation.
		Is called from run() */
	virtual void performIteration();
	/** Cooling scheme.
	        Is called from performGeneration; adapts current temperature;
		defaults to geometric cooling */
	virtual void cooling();
	/** Acceptance cirterion.
	        Is called from performGeneration; decides wether a generated
		chromosome is accepted as current solution;
		defaults to the metropolis criterion. */
	virtual bool accept( mh_solution *o, mh_solution *n );
};

} // end of namespace mh

#endif //MH_SIMANNEAL_H
