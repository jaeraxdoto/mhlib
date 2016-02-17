/*! \file mh_interfaces.h 
	\brief Various interfaces provided by algorithm. */

#ifndef MH_INTERFACES_H
#define MH_INTERFACES_H

#include "mh_tabulist.h"

namespace mh {

class mh_solution;
class feature;
class guidedLS;
class nhmove;


/** An abstract interface class for algorithms which provide
    tabulist handling methods. */
class tabulistProvider
{
public:
	/// virtual destructor.
	virtual ~tabulistProvider() {};

	/** Checks if a tabu is currently tabu.
	    Is called from chromsome::neighbour() */
	virtual bool isTabu(tabuAttribute *t) = 0;

	/** Checks if a chromsome can overide it's tabustate. */
	virtual bool aspiration(mh_solution *c) = 0;
};

/** An abstract interface class for chromosomes which provide tabus. */
class tabuProvider
{
public:
	/// Virtual destructor.
	virtual ~tabuProvider() {};
};

/** An abstract interface class for algorithms which provide an augmented
    objective method. */
class aObjProvider
{
public:
	/// Virtual destructor.
	virtual ~aObjProvider() {};
	/** Augmented objective function.
	        This function returns the additional part of the augmented
		objective function depending on the passed chromosome. */
	virtual double aobj(mh_solution *c) = 0;
	/** Function for getting the change in the objective function.
	        The change in the augmented part of the objective function
		if a certain move is applied is computed. */
	virtual double delta_aobj(mh_solution *c, const nhmove *m) = 0;
};

/** An interface class for subalgorithms of guided local search. */
class glsSubAlgorithm : public aObjProvider
{
	friend class guidedLS;
	
private:
	guidedLS *gls;
	
public:
	/// The constructor.
	glsSubAlgorithm() : gls(NULL) {}
	/** Augmented objective function.
	        This function returns the additional part of the augmented
		objective function depending on the passed chromosome. */
	double aobj(mh_solution *c);
	/** Function for getting the change in the objective function.
	        The change in the augmented part of the objective function
		if a certain move is applied is computed. */
	double delta_aobj(mh_solution *c, const nhmove *m);
};

/** An abstract interface class for chromosmes which provide 
    feature support. */
class featureProvider
{
public:
	/// Virtual destructor.
	virtual ~featureProvider() {};

	/// Create and return a feature object.
	virtual feature* getFeature() = 0;
};

/** An abstract interface class for chromosomes which provide
    a greedy construction heuristic. */
class gcProvider
{
public:
	/// Virtual destructor.
	virtual ~gcProvider() {};

	/// The greedy construction heuristic.
	virtual void greedyConstruct() = 0;
};

} // end of namespace mh

#endif //MH_INTERFACES_H
