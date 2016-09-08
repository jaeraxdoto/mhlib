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


//--------------------------- SchedulerMethodResult ------------------------------

/** Structure indicating the result of the method application and what to do. Default values of -1
 * are replaced by the Scheduler by 0 or 1 in a standard way. I.e., the solution is compared to
 * the incumbent in order to determine if it is better, and only better solutions are always accepted.
 * The reconsider flag, which is only important when the solution is not accepted, is chosen
 * according to the metaheuristic strategy (e.g., VND: 0, VNS: 1).
 */
struct SchedulerMethodResult {
	bool changed = true;	///< Did the solution change?
	int better = -1;		///< Is this solution better than the incumbent?
	int accept = -1;		///< Accept or reject the solution?
	int reconsider = -1; 	///< Should this method be reconsidered when the solution has not changed?
	/** Resets the structure to its default values. */
	void reset() {
		changed = true; better = accept = reconsider = -1;
	}
};

//--------------------------- SchedulerMethodContext ------------------------------

/** Structure that is passed when a SchedulerMethod is applied. It provides additional information
 * from the calling context and the possibility to store user-defined information from one call
 * to the next. */
struct SchedulerMethodContext {
	int callCounter = 0;		///< Number, how often this method was called already for this solution
	mh_solution *incumbentSol = nullptr;	///< Pointer to incumbent solution (= copy of initially provided solution).
	int userInt = 0; ///< User-defined int that can be set by the SchedulerMethod and is preserved between successive calls. For method-specific purposes.
	/** Abstract base class for arbitrary user data that can be preserved from one method call
	    to the next. */
	class UserData {
	public:
		virtual ~UserData() {}
	};
	/** Pointer to a class derived from UserData, possibly created and
	 * maintained by the method to preserve further data besides userInt between
	 * successive calls. If an object exists, it will finally be deleted by
	 * the destructor of the SchedulerMethodContext. */
	UserData *userData = nullptr;
	/** Destructor deletes userData object if one is left. */
	~SchedulerMethodContext() {
			if (userData != nullptr)
					delete userData;
	}
};

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

	/** Applies the method to the given solution.
	 * \param sol pointer to the solution.
	 * \param result pointer to a #Result structure, where information on the outcome and
	 * 			how to further proceed may be provided; if this is not done, the solution
	 * 			is handled in a default way. */
	virtual void run(mh_solution *sol, SchedulerMethodContext &context, SchedulerMethodResult &result) const = 0;

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
	void (SpecSol::* pmeth)(int, SchedulerMethodContext &, SchedulerMethodResult &);		///< Member function pointer to a Result(int) function
	const int par;						///< Integer parameter passed to the method

	/** Constructor initializing data.
	 * \param _name a string representing the method in an abbreviated form.
	 * \param _pmeth a pointer to a bool(int) member function of the class derived from mh_solution.
	 * \param _par an int user parameter that is stored and passed when calling the method.
	 * \param _arity the arity of the function, i.e., 0 if a solution is created from scratch and 1 if
	 * the operator acts on a current solution.
	 */
	SolMemberSchedulerMethod(const std::string &_name, void (SpecSol::* _pmeth)(int,
				SchedulerMethodContext &, SchedulerMethodResult &),
			int _par, int _arity) :
		SchedulerMethod(_name,_arity), pmeth(_pmeth), par(_par) {
	}

	/** Apply the method for the given solution, passing par. The method returns true if the solution
	 * has been changed and false otherwise.*/
	void run(mh_solution *sol, SchedulerMethodContext &context, SchedulerMethodResult &result) const {
		((static_cast<SpecSol *>(sol))->*pmeth)(par, context, result);
	}
};

} // end of namespace mh

#endif /* MH_SCHEDMETH_H */
