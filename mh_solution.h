/*! \file mh_solution.h
	\brief Abstract class for representing a solution independently of any algorithm.

	A problem specific solution class must be derived from this
	base class for a concrete problem. */

#ifndef MH_SOLUTION_H
#define MH_SOLUTION_H

#include <iostream>
#include "mh_param.h"

namespace mh {

/** \ingroup param
	Should be maximized?
	True if maximization, false for minimization. */
extern bool_param maxi;

class mh_base; // abstract class for algorithms

/** Abstract class representing the bare solution independently from any algorithm.
	A concrete class must be derived for a specific problem.
*/
class mh_solution
{
protected:
	/** Objective value of solution.
		Implicitly set and read by obj(). */
	double objval = -1;
	/// Set if the objective value is valid.
	bool objval_valid = false;

	/** Objective function.
		Must be overloaded and defined for a concrete problem.
		If the objective value for a given solution should be
		determined, obj() must be used instead of a direct call
		to objective(). */
	virtual double objective() = 0;

public:
	/// Parameter group
	std::string pgroup = "";
	/** Possible pointer to algorithm handling this solution. */
	mh_base *alg = nullptr;
	/** Value indicating the length of the solution in a generic way,
	 * e.g., the size of a vector. Keep the default value of 1 if
	 * your solution does not have a dedicated length.
	 */
	int length = 1;

	/** Constructor for uninitialized solution.
		Must also be defined for a concrete solution class.
		Sets objval_valid to false. */
	mh_solution(int l=1, mh_base *al=nullptr, const std::string &pg="")
		: pgroup(pg), alg(al), length(l) {}
	/** Constructor for uninitialized solution.
		Must also be defined for a concrete solution class.
		Sets objval_valid to false. */
	mh_solution(int l=1, const std::string &pg="")
		: pgroup(pg), length(l) {}
	/** The assignment operator "=" calls the virtual copy method.
	 * For copying solutions, the copy method should generally be used
	 * as it is safer when copying objects that are only referenced by
	 * a base class. */
    virtual mh_solution & operator = (const mh_solution &orig) = delete;
    /** The virtual method to copy a solution. Should be used instead of the classical
     * assignment operator to avoid problems when referring to solutions via a base class. */
    virtual void copy(const mh_solution &s) {
    	if (&s==this)
			return;
    	pgroup = s.pgroup; alg = s.alg; length = s.length;
    	objval = s.objval; objval_valid = s.objval_valid;
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
	/** Generates a duplicate.
		Generates an identical copy of a solution.
		Needs not necessarily to be overloaded for a concrete solution class,
		since it uses createUninitialized and copy.
		But for better performance, it may be overloaded by a 
		concrete function which does not use the copy constructor. */
	virtual mh_solution *clone() const
		{ mh_solution *p=createUninitialized();
			p->copy(*this); return p; }
	/** Initialization function.
		The solution is initialized (e.g., randomly).
		The parameter count is the number of the individual within
		the population (starting with 0) and need only to be
		considered in the case when not all individuals of a
		population should be initialized in the same way. */
	virtual void initialize(int count) { }
	/** Comparison of two solutions of identical classes.
		Needed e.g. for duplicate elimination. 
		Should be implemented in an efficient way, e.g. by first
		looking at the objective values, and only if they are
		equal then on all the genes or phenotypic properties. By default
		we assume here that solutions are always different if it is not
		the identical solution. */
	virtual bool equals(mh_solution &orig)
		{ return this==&orig; }
	/** Sets the pointer to the associated algorithm. */
    void setAlgorithm(mh_base *a);
	/** Returns the (phenotypic) distance between the current solution
		and solution c. The distance should be a metric.
		Used e.g. for a fitness-distance correlation analysis.
		By default this method calls equals() and returns 0 or 1 correspondingly. */
	virtual double dist(mh_solution &c)
		{ return equals(c) ? 0 : 1; }
	/** Virtual destructor.
		Needed if dynamic data structures are involved. */
	virtual ~mh_solution() {}
	/** Function for getting the objective value.
		The actual objective function is only called on demand, 
		if the value is not yet known. The result must be 
		written into objval. Needs usually not to be overloaded. */
	virtual double obj() {
		if (objval_valid)
			return objval;
		else {
		  	objval=objective();	objval_valid=true; return objval;
		}
	}
	/** Writes the solution to an ostream.
		The solution is written to the given ostream in	text format.
		@param ostr the output stream
		@param detailed tells how detailed the description
		should be (0...least detailed). */
	virtual void write(std::ostream &ostr,int detailed=0) {
		mherror("mh_solution::write: not implemented");
	}
	/** Saves a solution to a file. (Not necessarily needed.) */
	virtual void save(const std::string &fname) { 
		mherror("mh_solution::save: not implemented"); }
	/** Loads a solution from a file. (Not necessarily needed.) */
	virtual void load(const std::string &fname) { 
		mherror("mh_solution::load: not implemented"); }
	/** Returns true if the current solution is better in terms
		of the objective function than the one
		given as parameter. Takes care on parameter mh::maxi. */
	bool isBetter(mh_solution &p)
		{ return maxi(pgroup)?obj()>p.obj():
			obj()<p.obj(); }
	/** Returns true if the current solution is worse in terms
		of the objective function than the one
		given as parameter. Takes care on parameter mh::maxi. */
	bool isWorse(mh_solution &p)
		{ return maxi(pgroup)?obj()<p.obj():
			obj()>p.obj(); }
	/** Invalidates the solution.
		Sets objval to be invalid. During the next call to obj(), the
		solution is evaluated anew. Must be called when the
		solution changes. */
	virtual void invalidate()
		{ objval_valid=false; }
	/** Hashing function.
		This function returns a hash-value for the solution.
		Two solutions that are considered as equal must return the
		same value; however, identical hash-values for two
		solutions do not imply that the solution are equal.
		This is needed for the hash-table of the population. 
		The default implementation derives a value from obj(). */
	virtual unsigned long int hashvalue()
		{ return (unsigned long int)obj(); }
};

/** Operator << overloaded for writing solutions to an ostream. */
inline std::ostream &operator<<(std::ostream &ostr, mh_solution &sol) {
	sol.write(ostr); return ostr;
}

} // end of namespace mh

#endif //MH_SOLUTION_H
