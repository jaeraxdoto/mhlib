/*! \file mh_schedmeth.h
 \brief Abstract base classes for methods (i.e., construction heuristics, improvement methods, shaking methods etc.)
 that can be scheduled by the Scheduler class.
 */

#ifndef MH_SCHEDMETH_H
#define MH_SCHEDMETH_H

#include <assert.h>
#include <string>

#include "mh_solution.h"

namespace mh {

//--------------------------- SchedulerMethod ------------------------------

/**
 * Abstract base class representing a method like a construction heuristic, neighborhood search
 * or shaking method that can be scheduled by the scheduler along with the meta-information relevant to the
 * scheduling process.
 */
class SchedulerMethod {
public:
	const std::string name;		///< The method's (unique) name (possibly including method_par).
	const int arity;			///< Arity, i.e., number of input solutions of the method, which is currently either 0 or 1.
	int idx;				///< Index in methodPool of Scheduler.

	/**
	 * Status returned by the application of the method. Controls what is done with the solution.
	 * In a standard GVNS application, just RESULT_UNCHANGED and RESULT_CHANGED need to be considered.
	 * The other values are to possibly control what is done with the solution in a more fine-grained
	 * way.
	 */
	enum Result {
		/** The solution has not been changed, do not reconsider this method for this solution. */
		RESULT_UNCHANGED = 0,
		/** The solution has been changed, evaluate and use classical GVNS mechanisms to decide acceptance. */
		RESULT_CHANGED = 1,
		/** The solution has not been changed, but this method should be reconsidered to be applied
		 * to the same solution (e.g., because this is a randomized method not sampling a whole neighborhood).
		 */
		RESULT_UNCHANGED_RECONSIDER = 2,
		/** The solution has not been changed, and this method should not be reconsidered to be applied
		 * to the same solution (even when this is a "shaking" method, e.g., because it is deterministic).
		 */
		RESULT_UNCHANGED_NORECONSIDER = 3,
		/** The solution has changed and should be accepted without further comparing it to the incumbent,
		 * i.e., even if it is worse.
		 */
		RESULT_CHANGED_ACCEPT = 4,
		/** The solution has changed and should be discarded without further comparing it to the incumbent.
		 * The method should then be reconsidered to be applied again to the same original solution.
		 */
		RESULT_CHANGED_DISCARD_RECONSIDER = 5,
		/** The solution has changed and should be discarded without further comparing it to the incumbent.
		 * The method should then not be reconsidered to be applied again to the same original solution.
		 */
		RESULT_CHANGED_DISCARD_NORECONSIDER = 6
	};

	/**
	 * Constructs a new SchedulerMethod from a MethodType function object using the
	 * given arguments, assigning a default weight of 1 and a score of 0.
	 * \param _name a string representing the method in an abbreviated form.
	 * \param _arity the arity of the function, i.e., 0 if a solution is created from scratch and 1 if
	 * the operator acts on a current solution.
	 *
	 */
	SchedulerMethod(const std::string &_name, int _arity) :
				name(_name), arity(_arity) {
		idx = -1;
		// so far only construction and simple improvement methods are considered
		assert(arity>=0 && arity<=1);
	}

	/** Applies the method to the given solution. The method returns the #Result of the application. */
	virtual Result run(mh_solution *sol) const = 0;

	/**
	 * Virtual destructor.
	 */
	virtual ~SchedulerMethod() {
	}
};

/** Template class for realizing concrete #SchedulerMethod classes for Result(int) member functions
 *  of specific solution classes, i.e., classes derived from mh_solution.
 *  An integer parameter is maintained that is passed when calling the method for
 *  a specific solution. This integer can be used to control the methods functionality, e.g.
 *  for the neighborhood size, randomization factor etc. The return value must indicate
 *  the #Result of the application. */
template<class SpecSol> class SolMemberSchedulerMethod : public SchedulerMethod {
public:
	Result (SpecSol::* pmeth)(int);		///< Member function pointer to a Result(int) function
	const int par;						///< Integer parameter passed to the method

	/** Constructor initializing data.
	 * \param _name a string representing the method in an abbreviated form.
	 * \param _pmeth a pointer to a bool(int) member function of the class derived from mh_solution.
	 * \param _par an int user parameter that is stored and passed when calling the method.
	 * \param _arity the arity of the function, i.e., 0 if a solution is created from scratch and 1 if
	 * the operator acts on a current solution.
	 */
	SolMemberSchedulerMethod(const std::string &_name, SchedulerMethod::Result (SpecSol::* _pmeth)(int),
			int _par, int _arity) :
		SchedulerMethod(_name,_arity), pmeth(_pmeth), par(_par) {
	}

	/** Apply the method for the given solution, passing par. The method returns true if the solution
	 * has been changed and false otherwise.*/
	SchedulerMethod::Result run(mh_solution *sol) const {
		return ((static_cast<SpecSol *>(sol))->*pmeth)(par);
	}
};

} // end of namespace mh

#endif /* MH_SCHEDMETH_H */
