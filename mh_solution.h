/*! \file mh_solution.h
	\brief Abstract class for solution candidates inmetaheuristics.

	A problem specific solution class must be derived from this
	base class for a concrete problem. */

#ifndef MH_SOLUTION_H
#define MH_SOLUTION_H

#include <iostream>
#include "mh_base.h"
#include "mh_nhmove.h"
#include "mh_param.h"

/** \ingroup param
	Should be maximized?
	True if maximization, false for minimization. */
extern bool_param maxi;

/** \ingroup param
    Neighbour selection function to use
    - 0: random neighbour,
    - 1: next improvement,
    - 2: best improvement. */
extern int_param mvnbop;


/** Abstract solution class for the metaheuristics.
	A concrete class must be derived for a specific problem.
	This class should be mostly, but not completely
	independent of the used EA. */
class mh_solution
{
protected:
	/** Objective value of solution.
		Implicitly set and read by obj(). */
	double objval;
	/// Set if the objective value is valid.
	bool objval_valid;
	/** Objective function.
		Must be overloaded and defined for a concrete problem. 
		If the objective value for a given solution should be
		determined, obj() must be used instead of a direct call
		to objective(). */
	virtual double objective()=0;
	/** Number of elements.
		Set via the constructor. In case a concrete class
		represents a solution of varying number of elements, set the
		value to 1. */
	const int length;

	/** Algorithm for this solution. */
	mh_base *alg;

	/// Parametergroup
	string pgroup;

public:
	/** Copy constructor.
		Needed for clone().
		Must also be defined in derived classes accordignly. */
	mh_solution(const mh_solution &c) : length(c.length)
		{ objval=c.objval; objval_valid=c.objval_valid; alg=c.alg; pgroup=c.pgroup; }
	/** Constructor for unitinialized solution.
		Must also be defined for a concrete solution class.
		Sets objval_valid to false and the number of genes
		(which should be 1 in case of chromosomes of arbitrary
		length. */
	mh_solution(int l, mh_base *t, const pstring &pg=(pstring)("")) : length(l), alg(t), pgroup(pg.s)
		{ objval_valid=false; objval=0; }
	/** Constructor for unitinialized solution.
		Must also be defined for a concrete solution class.
		Sets objval_valid to false and the number of genes
		(which should be 1 in case of chromosomes of arbitrary
		length. */
	mh_solution(int l, const pstring &pg=(pstring)("") ) : length(l), alg(0), pgroup(pg.s)
		{ objval_valid=false; objval=0; }
	/** Creates an uninitialized object of the same class as the
		current object.
		Must be overloaded for a concrete solution class.
		Creates a solution of the same class and with the same
		constant parameters (e.g. number of genes).
		The new solution is not initialized. Needed e.g. for
		creating a population of individuals out of one given
		template object. */
	virtual mh_solution *createUninitialized() const=0;
	/** Generates a duplicate.
		Generates an identical copy of a solution.
		Need not to be overloaded for a concrete solution class,
		since it uses createUninitialized and copy.
		But for better performance, it may be overloaded by a 
		concrete function which does not use the copy constructor. */
	virtual mh_solution *clone() const
		{ mh_solution *p=createUninitialized(); 
			p->copy(*this); return p; }
	/** Copy a solution.
		Must be overloaded for a concrete solution class.
		Only two chromosomes of identical classes may be copied. */
	virtual void copy(const mh_solution &orig)
		{ objval=orig.objval; objval_valid=orig.objval_valid; alg=orig.alg; pgroup=orig.pgroup; }
	/** The operator "=" simply calls the copy method. */
        const mh_solution & operator = (const mh_solution &orig)
                { copy(orig); return *this; }
	/** Comparison of two chromosomes of identical classes.
		Needed e.g. for duplicate elimination. 
		Should be implemented in an efficient way, e.g. by first
		looking at the objective values, and only if they are
		equal then on all the genes or phenotypic properties. */
	virtual bool equals(mh_solution &orig)
		{ return false; }
	/** Returns the (phenotypic) distance between the current solution
		and solution c. The distance should be a metric.
		Used e.g. for a fitness-distance correlation analysis. */
	virtual double dist(mh_solution &c)
		{ return equals(c) ? 0 : 1; }
	/** Virtual descructor.
		Needed if dynamic data structures are involved. */
	virtual ~mh_solution() {}
	/** Function for getting the objective value.
		The actual objective function is only called on demand, 
		if the value is not yet known. The result must be 
		written into objval. Need usually not to be overloaded. */
	virtual double obj();
	/** Function for getting the change in the objective function.
	        The change in the objective function if a certain move
		is applied is computed.

		Note: The default implementation does return 0.0 */
	virtual double delta_obj(const nhmove &m) { return 0.0; }
	/** Function to apply a certain move.
	        The solution is changed according to the move, but
		the objective value is not invalidated. */
	virtual void applyMove(const nhmove &m){ cerr << "BUG: applyMove() not implemented !" << endl;}; 
	/** Initialization function.
		The solution is initialized (usually randomly).
		Must be overloaded accordingly.
		The parameter count is the number of the individual within
		the population (starting with 0) and need only to be
		considered in the case when not all individuals of a
		population should be initialized in the same way. */
	virtual void initialize(int count)=0;
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
		expected to be done in mutate when the genotype is actually
		changed.  Returns the number of actually performed
		mutations.  Need usually not to be overloaded in a derived
		class. */
	virtual int mutation(double prob);
	/** Actual mutation function which must be overloaded
	    when implementing mutation.
		Perform the given number of mutations and call invalidate()
		if the genotype changes. */
	virtual void mutate(int count)
	{ }
	/** Generic crossover operator.
		Builds new genotype out of two given parents. Must 
		call invalidate() when the genotype changes. */
	virtual void crossover(const mh_solution &parA,const mh_solution &parB)
	{ }
	/** Locally improve the current solution.
		Optional local improve the current solution.
		Must call invalidate() when the genotype changes. */
	virtual void locallyImprove()
	{ }
	/** Reproduce a solution from a parent.
		If no crossover is used to generate a new solution, this
		reproduction function is called. By default, a copy of the 
		parent is made. */
	virtual void reproduce(const mh_solution &par)
		{ copy(par); }
	/** Writes the solution to an ostream.
		The solution should be written to the given ostream in in
		text format. "detailed" tells how detailed the description 
		should be (0...least detailed). The objective value need
		not to be written out. */
	virtual void write(ostream &ostr,int detailed=0)const=0;
	/** Saves a solution to a file. (Not necessarily needed.) */
	virtual void save(const char *fname) {}
	/** Saves a solution to a file. (Not necessarily needed.) */
	void save(const string &fname) { save(fname.c_str()); }
	/** Loads a solution from a file. (Not necessarily needed.) */
	virtual void load(const char *fname) {}
	/** Loads a solution from a file. (Not necessarily needed.) */
	void load(const string &fname) { load(fname.c_str()); }
	/** Compare the fitness.
		returns true if the current solution is fitter than that
		given as parameter. Takes care on parameter maxi. */
	bool isBetter(mh_solution &p)
		{ return maxi(pgroup)?obj()>p.obj()+0.00001:
			obj()<p.obj()-0.00001; }
	/** Compare the fitness.
		returns true if the current solution is worse than that
		given as parameter. Takes care on parameter maxi. */
	bool isWorse(mh_solution &p)
		{ return maxi(pgroup)?obj()<p.obj():
			obj()>p.obj(); }
	/** Invalidates the solution.
		Sets objval to be invalid. During the next call to obj(), the
		solution is evaluated anew. Must be called when the
		genotype changes. */
	void invalidate()
		{ objval_valid=false; }
	/** Hashing function.
		This function returns a hash-value for the solution.
		Two chromosomes that are considered as equal must return the
		same value; however, identical hash-values for two
		chromosomes do not imply that the solution are equal.
		This is needed for the hash-table of the population. 
		The default implementation derives a value from obj(). */
	virtual unsigned long int hashvalue()
		{ return (unsigned long int)obj(); }
	/** Neighbour selection function.
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
	        to let the chromosomes know what algorithm is using them. */
	void setAlgorithm(mh_base *alg)
		{ this->alg=alg; if (alg!=0) this->pgroup=alg->pgroup; }
};

inline double mh_solution::obj()
{
	if (objval_valid) 
		return objval; 
	else 
	{ 
	  	objval=objective(); 
		objval_valid=true; 
	  	return objval; 
	} 
}

#endif //MH_SOLUTION_H
