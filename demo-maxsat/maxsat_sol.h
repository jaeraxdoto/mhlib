/*! \file maxsat_sol.h
  \brief A class representing a solution to the MAXSAT problem.

  This class represents a solution to the MAXSAT problem.
  It is derived from mh_solution so that the algorithms
  within mhlib can deal with it. Here, we derive it more specifically from
  binStringSol, which implements methods for dealing with binary strings.
 */

#ifndef MAXSAT_SOL_H
#define MAXSAT_SOL_H

#include "mh_binstringsol.h"
#include "maxsat_inst.h"

namespace maxsat {

/**
  This class represents a solution to the MAXSAT problem, i.e.,
  assignments of values to the Boolean variables.
  It is derived from mh_solution so that the algorithms
  within mhlib can deal with it. Here, we derive it more specifically from
  binStringSol, which implements methods for dealing with binary strings.
 */
class MAXSATSol : public mh::binStringSol {
public:
	const MAXSATInst *probinst;	/// A pointer to the problem instance for which this is a solution

	/** The default constructor. It stores the pointer to the problem instance
	    and passes the number of variables to the binStringSol constructor. */
	MAXSATSol(const MAXSATInst *_probinst) :
		binStringSol(_probinst->nVars), probinst(_probinst)
		{}
	/** Copy constructor. */	
	MAXSATSol(const MAXSATSol &sol) :
		binStringSol(sol), probinst(sol.probinst)
		{}
	/** Create a new uninitialized instance of this class. */
	virtual mh_solution *createUninitialized() const
		{ return new MAXSATSol(probinst); }
	/** Clone this solution, i.e., return a new copy. */
	virtual mh_solution *clone() const
		{ return new MAXSATSol(*this); }
	/** Dynamically cast an mh::mh_solution reference to a reference of this class. */
	static const MAXSATSol &toMAXSATSol(const mh::mh_solution &ref) {
		return (dynamic_cast<const MAXSATSol &>(ref));
	}
	/** Determine the objective value of the solution. Here we count the number
	 * of satisfied clauses. */
	double objective();
	/** A simple construction heuristic, just calling the base class' initialize
	 * function, initializing each bit randomly. Returns true as the solution
	 * has (most likely) changed.
	 */
	bool construct(int k);
	/** A local improvement function which currently does nothing. */
	bool localimp(int k);
	/** A simple shaking method: Flip k randomly chosen positions. */
	bool shaking(int k);
};

} // end of namespace maxsat


#endif /* MAXSAT_SOL_H */
