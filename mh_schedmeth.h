/*! \file mh_schedmeth.h
 \brief Abstract base classes for methods (i.e., construction heuristics, improvement methods, shaking methods etc.)
 that can be scheduled by the Scheduler class.
 */

#ifndef MH_SCHEDMETH_H
#define MH_SCHEDMETH_H

#include <assert.h>
#include <string>

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

	unsigned int idx;			///< Index in methodPool of Scheduler.

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

	/** Applies the method to the given solution. The method returns true if the solution
	 * has been changed and false otherwise. */
	virtual bool run(mh_solution *sol) const = 0;

	/**
	 * Virtual destructor.
	 */
	virtual ~SchedulerMethod() {
	}
};

/** Template class for realizing concrete SchedulerMethods for bool(int) member functions
 *  of specific solution classes, i.e., classes derived from mh_solution.
 *  An integer parameter is maintained that is passed when calling the method by run for
 *  a specific solution. This integer can be used to control the methods functionality, e.g.
 *  for the neighborhood size, randomization factor etc. The return value must indicate
 *  whether or not the solution has actually been modified. */
template<class SpecSol> class SolMemberSchedulerMethod : public SchedulerMethod {
public:
	bool (SpecSol::* pmeth)(int);		///< Member function pointer to a bool(int) function
	const int par;						///< Integer parameter passed to the method

	/** Constructor initializing data.
	 * \param _name a string representing the method in an abbreviated form.
	 * \param _pmeth a pointer to a bool(int) member function of the class derived from mh_solution.
	 * \param _par an int user parameter that is stored and passed when calling the method.
	 * \param _arity the arity of the function, i.e., 0 if a solution is created from scratch and 1 if
	 * the operator acts on a current solution.
	 */
	SolMemberSchedulerMethod(const std::string &_name, bool (SpecSol::* _pmeth)(int),
			int _par, int _arity) :
		SchedulerMethod(_name,_arity), pmeth(_pmeth), par(_par) {
	}

	/** Apply the method for the given solution, passing par. The method returns true if the solution
	 * has been changed and false otherwise.*/
	bool run(mh_solution *sol) const {
		return ((static_cast<SpecSol *>(sol))->*pmeth)(par);
	}
};

} // end of namespace mh

#endif /* MH_SCHEDMETH_H */
