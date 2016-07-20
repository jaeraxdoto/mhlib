/*! \file maxsat_sol.h
  \brief A class representing a solution to the MAXSAT problem.

  This class represents a solution to the MAXSAT problem.
  It is derived from mh_solution so that the algorithms
  within mhlib can deal with it. Here, we derive it more specifically from
  binStringSol, which implements methods for dealing with binary strings.
 */

#ifndef MAXSAT_SOL_H
#define MAXSAT_SOL_H

#include "mh_schedmeth.h"
#include "mh_binstringsol.h"
#include "maxsat_inst.h"

namespace maxsat {

/**
  This class represents a solution to the MAXSAT problem, i.e.,
  assignments of values to the Boolean variables.
  It is derived from mh_solution so that the algorithms
  within mhlib can deal with it. Here, we derive it more specifically from
  binStringSol, which implements methods for dealing with binary strings.
  Note that while in binStringSol the variables are indexed from 0 on,
  MAXSATInst assumes variables to be indexed from 1 on, and therefore
  a corresponding transformation is done in the objective function.
 */
class MAXSATSol : public mh::binStringSol {
public:
	const MAXSATInst *probinst;	/// A pointer to the problem instance for which this is a solution

	/** The default constructor. It stores the pointer to the problem instance
	    and passes the number of variables to the binStringSol constructor. */
	MAXSATSol(const MAXSATInst *probinst) :
		binStringSol(probinst->nVars), probinst(probinst) {}
	/** Copy constructor. */	
	MAXSATSol(const MAXSATSol &sol) :
		binStringSol(sol), probinst(sol.probinst) {}
	/** Create a new uninitialized instance of this class. */
	mh_solution *createUninitialized() const override
		{ return new MAXSATSol(probinst); }
	/** Clone this solution, i.e., return a new copy. */
	mh_solution *clone() const override
		{ return new MAXSATSol(*this); }
	/** Dynamically cast an mh::mh_solution reference to a reference of this class. */
	static const MAXSATSol &cast(const mh::mh_solution &ref) {
		return (dynamic_cast<const MAXSATSol &>(ref));
	}
	/** Determine the objective value of the solution. Here we count the number
	 * of satisfied clauses. */
	double objective() override;
	/** A simple construction heuristic, just calling the base class' initialize
	 * function, initializing each bit randomly. */
	mh::SchedulerMethod::Result construct(int k);
	/** A best improvement local search in the k-flip neighborhood. */
	mh::SchedulerMethod::Result localimp(int k);
	/** A simple shaking method: Flip k randomly chosen positions. */
	mh::SchedulerMethod::Result shaking(int k);
};

} // end of namespace maxsat


#endif /* MAXSAT_SOL_H */
