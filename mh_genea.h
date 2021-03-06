/*! \file mh_genea.h 
	\brief A generational EA. */

#ifndef MH_GENEA_H
#define MH_GENEA_H

#include "mh_eaadvbase.h"
#include "mh_param.h"
#include "mh_solution.h"

namespace mh {

/** \ingroup param
	Should elitism be used?.
	True if the best solution should be kept, false if not. */
extern bool_param elit;

/** A generational EA.
	During each generation, all solutions are replaced by new ones
	generated by means of variation operators (crossover and mutation). */
class generationalEA : public mh_eaadvbase
{
public:
	/** The constructor.
		An initialized population already containing solutions
		must be given. Note that the population is NOT owned by the 
		EA and will not be deleted by its destructor. */
	generationalEA(pop_base &p, const std::string &pg="");
	/** Another constructor.
		Creates an empty EA that can only be used as a template. */
	generationalEA(const std::string &pg="") : mh_eaadvbase(pg) {};
	/** The destructor. */
	~generationalEA();
	/** Create new steadyStateGA.
		Returns a pointer to a new steadyStateEA. */
	mh_advbase *clone(pop_base &p,const std::string &pg="")
	    { return new generationalEA(p,pg); }
	/** Performs a single generation. */
	void performIteration();
	/** The selection function.
		Calls a concrete selection technique and returns the index
		of the selected chromosome in the population. */
	virtual int select()
		{ nSelections++; return tournamentSelection(); }

protected:
	/** The crossover and mutation function.
		Creates a new generation from the selected solutions by using
		crossover and mutation. */
	virtual void createNextGeneration();
	
	int *selectedChroms=nullptr;          // indices of selected solutions
	mh_solution **nextGeneration=nullptr;  // used to build the next generation
};

} // end of namespace mh

#endif //MH_GENGA_H
