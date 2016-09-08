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
	friend class MAXSATShakingMethod;
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
	static MAXSATSol &cast(mh::mh_solution &ref) {
		return (dynamic_cast<MAXSATSol &>(ref));
	}
	/** Determine the objective value of the solution. Here we count the number
	 * of satisfied clauses. */
	double objective() override;
	/** A simple construction heuristic, just calling the base class' initialize
	 * function, initializing each bit randomly. */
	void construct(int k, mh::SchedulerMethodContext &context, mh::SchedulerMethodResult &result);
	/** A best improvement local search in the k-flip neighborhood. */
	void localimp(int k, mh::SchedulerMethodContext &context, mh::SchedulerMethodResult &result);
	/** A random sampling of length solutions in the k-flip neighborhood. */
	void randlocalimp(int k, mh::SchedulerMethodContext &context, mh::SchedulerMethodResult &result);
};

/** This class demonstrates how a SchedulerMethod may alternatively be implemented
 * outside of the solution class by an own class. Here, a shaking method based on
 * a random k-opt move is realized.
 */
class MAXSATShakingMethod : public mh::SchedulerMethod {
	const int par;						///< Integer parameter passed to the method
public:
	/** Constructor of MAXSATShakingMethod. */
	MAXSATShakingMethod(const std::string &name, int par, int arity) :
		SchedulerMethod(name,arity), par(par) {
	}
	/** Apply the method for the given solution, passing par. The method returns true if the solution
	 * has been changed and false otherwise.*/
	void run(mh::mh_solution *sol, mh::SchedulerMethodContext &context, mh::SchedulerMethodResult &result) const override;
};

} // end of namespace maxsat


#endif /* MAXSAT_SOL_H */
