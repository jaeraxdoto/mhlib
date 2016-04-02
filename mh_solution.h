/*! \file mh_solution.h
	\brief Abstract class for solution candidates in metaheuristics, including basic operator declarations.

	A problem specific solution class must be derived from this
	base class for a concrete problem. */

#ifndef MH_SOLUTION_H
#define MH_SOLUTION_H

#include <cassert>
#include "mh_baresol.h"
#include "mh_base.h"
#include "mh_nhmove.h"

namespace mh {

/** \ingroup param
    Neighbour selection function to use
    - 0: random neighbour,
    - 1: next improvement,
    - 2: best improvement. */
extern int_param mvnbop;


/** Abstract solution class for the metaheuristics.
	A concrete class must be derived for a specific problem.
	In contrast to class mh_bare_solution, this class containts
	basic operator declarations for metaheuristics. */
class mh_solution : public mh_bare_solution
{
protected:
	virtual double objective()=0;
	/** Returns the number of elements.
		In case a concrete class represents a solution of varying or undefined number of elements,
		use the default implementation returning 1. */
	const int length = 1;

	/** Algorithm for this solution. */
	mh_base *alg = nullptr;


public:
	/** Constructor for uninitialized solution with given point to algorithm.
		Must also be defined for a concrete solution class.
		Sets objval_valid to false and the number of genes
		(which should be 1 in case of solutions of arbitrary
		length. */
	mh_solution(int l, mh_base *t, const std::string &pg="") : mh_bare_solution(pg), length(l), alg(t)
		{ }
	/** Constructor for uninitialized solution.
		Must also be defined for a concrete solution class.
		Sets objval_valid to false and the number of genes
		(which should be 1 in case of solutions of arbitrary
		length. */
	mh_solution(int l, const std::string &pg="") : mh_bare_solution(pg), length(l)
		{ }
	/** The assignment operator "=" is declared virtual.
	 * It must be specifically defined in a concrete derived class. */
    virtual mh_solution & operator = (const mh_solution &orig) {
    	assert(length==orig.length);
    	mh_bare_solution::operator=(orig);
    	return *this;
    }	/** The assignment operator "=" is declared virtual.
	 * It must be specifically defined in a concrete derived class. */
    mh_bare_solution & operator = (const mh_bare_solution &orig) {
    	return mh_solution::operator=(to_mh_solution(orig));
    }
	/** Creates an uninitialized object of the same class as the
		current object.
		Must be overloaded for a concrete solution class.
		Creates a solution of the same class and with the same
		constant parameters (e.g. number of genes).
		The new solution is not initialized. Needed e.g. for
		creating a population of individuals out of one given
		template object. */
	virtual mh_solution *createUninitialized() const=0;
	/** DEPRECATED: Copy a solution; better use assignment operator.
		Must be overloaded for a concrete solution class.
		Only two solutions of identical classes may be copied. */
	void copy(const mh_solution &orig) { *this = orig; }
	/** Convenience function for dynamically casting a mh_bare_solution to a mh_solution. */
	static mh_solution &to_mh_solution(mh_bare_solution &ref)
	{ return (dynamic_cast<mh_solution &>(ref)); }
	/** Convenience function for dynamically casting a const mh_bare_solution to a const mh_solution. */
	static const mh_solution &to_mh_solution(const mh_bare_solution &ref)
	{ return (dynamic_cast<const mh_solution &>(ref)); }
	/** Convenience function for dynamically casting a mh_bare_solution ptr to a mh_solution ptr. */
	static mh_solution *to_mh_solution(mh_bare_solution *ref)
	{ return (dynamic_cast<mh_solution *>(ref)); }
	/** Convenience function for dynamically casting a const mh_bare_solution ptr to a const mh_solution ptr. */
	static const mh_solution *to_mh_solution(const mh_bare_solution *ref)
	{ return (dynamic_cast<const mh_solution *>(ref)); }
	/** Function for getting the change in the objective function.
	        The change in the objective function if a certain move
		is applied is computed.
		Note: The default implementation does return 0.0 */
	virtual double delta_obj(const nhmove &m) { return 0.0; }
	/** Function to apply a certain move.
	        The solution is changed according to the move, but
		the objective value is not invalidated. */
	virtual void applyMove(const nhmove &m) { mherror("mh_solution::applyMove() not implemented"); };
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
	virtual void mutate(int count)
	{ }
	/** Generic crossover operator.
		Builds new solution out of two given parental solutions. Must
		call invalidate() when the solution changes. */
	virtual void crossover(const mh_solution &parA,const mh_solution &parB) {
		mherror("mh_solution::crossover: not implemented"); }
	/** Locally improve the current solution.
		Optional local improve the current solution.
		Must call invalidate() when the solution changes. */
	virtual void locallyImprove()
	{ }
	/** Reproduce a solution from a parent.
		If no crossover is used to generate a new solution, this
		reproduction function is called. By default, a copy of the 
		parent is made. */
	virtual void reproduce(const mh_solution &par)
		{ copy(par); }
	/** Neighbor selection function.
	        Replaces the current solution with one of its neighbourhood.
		The actual neighbour selection method can be chosen with
		parameter mvnbop. */
	void selectNeighbour();
	/** Replace current solution with a random neighbour.
	        The default of this operator is to use the mutate method. */
	virtual void selectRandomNeighbour()
		{ mutate(1); }
	/** Replace current solution with a better or even the best neighbour.
	        If find_best is true, the best solution in the neighbourhood
		is searched, if it is false the next improvment is selected (if
		one exists).

		Note: The default of this method is to do nothing. */
	virtual void selectImprovement(bool find_best) {}
	/** Set the algorithm for this solution.
	        An algorithm should call this before using a solution,
	        to let the solutions know what algorithm is using them. */
	void setAlgorithm(mh_base *alg)
		{ this->alg=alg; if (alg!=0) this->pgroup=alg->pgroup; }
};

} // end of namespace mh

#endif //MH_SOLUTION_H
