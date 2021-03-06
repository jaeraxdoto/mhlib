/*! \file mh_grasp.h
  \brief Greedy adaptive randomized search procedure. */

#ifndef MH_GRASP_H
#define MH_GRASP_H

#include "mh_lsbase.h"
#include "mh_param.h"

namespace mh {

/** Greedy adaptive randomized search procedure.
	A randomized construction heuristic generates solutions which
	are used as startingpoint for an inner localsearch. The chromosomes
	in the passed population must implement the gcProvider interface. */
class GRASP : public lsbase
{
protected:
	/* The subpopulation for the inner algorithm. */
	pop_base *spop = nullptr;

public:
	/** The constructor.
		An initialized population already containing chromosomes 
		must be given. Note that the population is NOT owned by the 
		algorithm and will not be deleted by its destructor. GRASP
		will only use the first two chromosomes. */
	GRASP(pop_base &p, const std::string &pg="");
	/** Another constructor.
		Creates an empty algorithm that can only be used as a template. */
	GRASP(const std::string &pg="") : lsbase(pg) {}
	/** The destructor. */
	~GRASP();
	/** Create new GRASP.
		Returns a pointer to a new GRASP. */
	mh_advbase *clone(pop_base &p, const std::string &pg="")
		{ return new GRASP(p,pg); }
	/** Performs a single generation.
		Is called from run() */
	virtual void performIteration();
};

} // end of namespace mh

#endif //MH_GRASP_H
