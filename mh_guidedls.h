/*! \file mh_guidedls.h
  \brief Guided local search. */


#ifndef MH_GUIDEDLS_H
#define MH_GUIDEDLS_H

#include "mh_feature.h"
#include "mh_interfaces.h"
#include "mh_lsbase.h"
#include "mh_param.h"

/** \ingroup param
    Interval for penalty resets.

    If set to 0 no penalty resets will be performed. */
extern int_param glsri;

/** Guided local search.
	A localsearch is performed with an augmented objective function,
	which is updated when a new local optimum is found. The chromosomes
	in the population must implement the featureProvider interface. */
class guidedLS : public lsbase
{
protected:
	/** The actual feature object. */
	feature *f;
	/** The subpopulation for the inner algorithm. */
	pop_base *spop;
	/** The actual tuned penalty influence. */
	double lambda;

public:
	/** The constructor.
	        An initialized population already containing chromosomes 
		must be given. Note that the population is NOT owned by the 
		algorithm, and will not be deleted by its destructor. guidedLS
		will only use the first two chromosomes. */
	guidedLS(pop_base &p, const pstring &pg=(pstring)(""));
	/** Another constructor.
		Creates an empty algorithm that can only be used as a template. */
	guidedLS(const pstring &pg=(pstring)("")) : lsbase(pg) {}
	/** The destructor. */
	~guidedLS();
	/** Create new guidedLS.
		Returns a pointer to a new guidedLS. */
	mh_advbase *clone(pop_base &p, const pstring &pg=(pstring)(""))
		{ return new guidedLS(p,pg); }	
	/** Performs a single generation.
		Is called from run() */
	virtual void performGeneration();
	/** Augmented objective function.
	        This provides an additional term for the objective value. */
	virtual double aobj(mh_solution *c);
	/** Function for getting the change in the objective function.
	        The change in the augmented part of the objective function
		if a certain move is applied is computed. */
	virtual double delta_aobj(mh_solution *c, const move *m);
};

#endif //MH_GUIDEDLS_H
