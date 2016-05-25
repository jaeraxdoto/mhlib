/*! \file mh_gaopsprov.h
	\brief A provider class for mh_solution objects containing various abstract operators used in genetic algorithms.

	A problem specific solution class must be derived from this
	base class for a concrete problem. */

#ifndef MH_GAOPSPROV_H
#define MH_GAOPSPROV_H

#include <cassert>
#include "mh_solution.h"
#include "mh_base.h"
#include "mh_nhmove.h"

namespace mh {

/** \ingroup param
    neighbor selection function to use
    - 0: random neighbor,
    - 1: next improvement,
    - 2: best improvement. */
extern int_param mvnbop;

/** A provider class for mh_solution objects containing various abstract operators used in genetic algorithms. */
class gaopsProvider
{
public:
	/** Destructor. */
	virtual ~gaopsProvider() { }
	/** Cast to a const gaopsProvider. */
	static const gaopsProvider &cast(const mh_solution &ref)
	{ return (dynamic_cast<const gaopsProvider &>(ref)); }
	/** Cast to a gaopsProvider. */
	static gaopsProvider &cast(mh_solution &ref)
	{ return (dynamic_cast<gaopsProvider &>(ref)); }
	/** Function for getting the change in the objective function.
	        The change in the objective function if a certain move
		is applied is computed.
		Note: The default implementation does return 0.0 */
	virtual double delta_obj(const nhmove &m) { return 0.0; }
	/** Function to apply a certain move.
	        The solution is changed according to the move, but
		the objective value is not invalidated. */
	virtual void applyMove(const nhmove &m) {
		mherror("gaopsProvider::applyMove() not implemented"); };
	/** Mutate solution with given probability/rate (per solution).
		If prob is negative, the absolute value is interpreted in
		such a way that each element is mutated with probability
		prob/nGenes. The actual number of mutations per solution
		is then not always the same but determined via a
		Poisson-distribution. 
		If prob<1000, a Poisson-distribution with mean |prob-1000| is 
		also used, but in addition, it is assured that at least one
		mutation is performed.
		This method finally calls mutate(int count) with the
		number of mutations that should actually be performed, if
		count>0.  Does not call invalidate() itself. This is
		expected to be done in mutate when the solution is actually
		changed.  Returns the number of actually performed
		mutations.  Need usually not to be overloaded in a derived
		class. */
	virtual int mutation(double prob);
	/** Actual mutation function which must be overloaded
	    when implementing mutation.
		Perform the given number of mutations and call invalidate()
		if the solution changes. */
	virtual void mutate(int count) {
		mherror("gaopsProvider::mutate() not implemented"); }
	/** Generic crossover operator.
		Builds new solution out of two given parental solutions. Must
		call invalidate() when the solution changes. */
	virtual void crossover(const mh_solution &parA, const mh_solution &parB) {
		mherror("gaopsProvider::crossover() not implemented"); }
	/** Locally improve the current solution.
		Optional local improve the current solution.
		Must call invalidate() when the solution changes. */
	virtual void locallyImprove(){ }
	/** Neighbor selection function.
	    Replaces the current solution with one of its neighborhood.
		The actual neighbor selection method can be chosen with
		parameter mvnbop. */
	void selectNeighbour();
	/** Replace current solution with a random neighbor.
	        The default of this operator is to use the mutate method. */
	virtual void selectRandomNeighbour()
		{ mutate(1); }
	/** Replace current solution with a better or even the best neighbor.
	    If find_best is true, the best solution in the neighborhood
		is searched, if it is false the next improvement is selected (if
		one exists). The default of this method is to do nothing. */
	virtual void selectImprovement(bool find_best) {}
};

} // end of namespace mh

#endif // MH_GAOPSPROVH
