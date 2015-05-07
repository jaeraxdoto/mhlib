/*! \file mh_island.h 
	\brief Implements an EA using the island model and several sub-EAs. */

#ifndef MH_ISLAND_H
#define MH_ISLAND_H

#include "mh_advbase.h"
#include "mh_param.h"


/** \ingroup param
	The number of islands.
	Number of islands to use. The population is split using this number.
	The size of the population has to be a multiple of this. */
extern int_param islk;

/** \ingroup param
	The migration strategy:
	- 0: The best solution is taken and cloned.
	- 1: No migration takes place. */
extern int_param migr;

/** \ingroup param
	The migration probability.
	The probability with which migration takes place after each
	generation. */
extern double_param pmig;

/** A GA using the island model and several sub-GAs.
	During each generation, the performGeneration-function is called for
	each island, afterwards migration between the islands is performed. */
class islandModelEA : public mh_advbase
{
public:
	/** The constructor.
		If no sub-EA template is given, a steady-state EA is used. */
	islandModelEA(pop_base &p, mh_advbase *mh_templ, const pstring &pg=(pstring)(""));
	islandModelEA(pop_base &p, const pstring &pg=(pstring)(""));
	/** The destructor.
		Deletes sub-EAs. */
	~islandModelEA();
	/** Create new islandModelEA.
		Returns a pointer to a new islandModelEA. */
	mh_advbase *clone(pop_base &p, const pstring &ps=(pstring)(""))
	    { return new islandModelEA(p,ps); }
	/** The EA's main loop.
		Performs generations and migration until the termination
		criterion is fullfilled. */
	void run();
	/** Performs a single generation.
		Calls the islands' corresponding function. */
	void performIteration();

protected:
	/** Performs migration between islands. */
	virtual void performMigration();
	/** Initializes the island-model EA. */
	void init(mh_advbase *mh_templ);
	/** Updates the statistics values with the sums of all sub-EAs. */
	virtual void sumStatistics();
	
	mh_advbase **subEAs;  // Sub-EAs used for the islands.
};

#endif //MH_ISLAND_H
