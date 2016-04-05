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
#include "mh_gaopsprov.h"

namespace mh {

/** Abstract solution class for the metaheuristics.
	A concrete class must be derived for a specific problem.
	In contrast to class mh_bare_solution, this class containts
	basic operator declarations for metaheuristics. */
class mh_solution : public mh_bare_solution, public gaopsProvider
{
public:
	/** Constructor for uninitialized solution with given point to algorithm.
		Must also be defined for a concrete solution class.
		Sets objval_valid to false and the number of genes
		(which should be 1 in case of solutions of arbitrary
		length. */
	mh_solution(int l, mh_base *alg, const std::string &pg="")
		: mh_bare_solution(l, alg, pg)
		{ }
	/** Constructor for uninitialized solution.
		Must also be defined for a concrete solution class.
		Sets objval_valid to false and the number of genes
		(which should be 1 in case of solutions of arbitrary
		length. */
	mh_solution(int l, const std::string &pg="") : mh_bare_solution(l,pg) { }
	mh_solution(const mh_bare_solution &s) : mh_bare_solution(s) { }
	/** Convenience function for dynamically casting a mh_bare_solution to a mh_solution. */
	static mh_solution &cast(mh_bare_solution &ref)
	{ return (dynamic_cast<mh_solution &>(ref)); }
	/** Convenience function for dynamically casting a const mh_bare_solution to a const mh_solution. */
	static const mh_solution &cast(const mh_bare_solution &ref)
	{ return (dynamic_cast<const mh_solution &>(ref)); }
	/** Convenience function for dynamically casting a mh_bare_solution ptr to a mh_solution ptr. */
	static mh_solution *cast(mh_bare_solution *ref)
	{ return &dynamic_cast<mh_solution &>(*ref); }
	/** Convenience function for dynamically casting a const mh_bare_solution ptr to a const mh_solution ptr. */
	static const mh_solution *cast(const mh_bare_solution *ref)
	{ return &dynamic_cast<const mh_solution &>(*ref); }

	void setAlgorithm(mh_base *alg)
		{ this->alg=alg; if (alg!=0) this->pgroup=alg->pgroup; }
};

} // end of namespace mh

#endif //MH_SOLUTION_H
