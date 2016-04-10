/*! \file mh_permsol.h 
	\brief A permutation string solution with values 0...length-1.
	*/

#ifndef MH_PERMSOL_H
#define MH_PERMSOL_H

#include <iostream>

#include "mh_nhmove.h"
#include "mh_stringsol.h"

namespace mh {

/** \ingroup param
	Used crossover operator for permSol:
	- 0: random choice,
	- 1: PMX,
	- 2: OX,
	- 3: CX,
	- 4: UOBX,
	- 5: C1. */
extern int_param permxop;

/** \ingroup param
	Used mutation operator for permSol:
	- 0: random choice,
	- 1: inversion,
	- 2: reciprocal exchange,
	- 3: insertion. */
extern int_param permmop;

/** Type of individual variables in permSol. */
typedef unsigned int permSolVarType;

/** A solution class for permutation problems with values 0...length-1. */
class permSol : public stringSol<permSolVarType>
{
protected:
	static const permSol &cast(const mh_solution &ref)
	{ return (dynamic_cast<const permSol &>(ref)); }

	/** Performs inversion. */
	void mutate_inversion(int count);
	/** Performs reciprocal exchange. */
	void mutate_exchange(int count);
	/** Performs insertion. */
	void mutate_insertion(int count);

	/** This is the partially matched crossover (PMX). */
	void crossover_pmx(const mh_solution &parA,const mh_solution &parB);
	/** This is the order crossover (OX). */
	void crossover_ox(const mh_solution &parA,const mh_solution &parB);
	/** This is the cycle crossover (CX). */
	void crossover_cx(const mh_solution &parA,const mh_solution &parB);
	/** This is the uniform order based crossover (UOBX). */
	void crossover_uobx(const mh_solution &parA,const mh_solution &parB);
	/** This is the C1 crossover. Up to a random cut point, all variables
	are copied from the first solution; all remaining variables are
	appended in the order as they appear in the second solution. */
	void crossover_c1(const mh_solution &parA,const mh_solution &parB);

public:
	permSol(const mh_solution &c);
	permSol(int l, mh_base *t, const std::string &pg="") : stringSol<permSolVarType>(l,l-1,t,pg) { }
	permSol(int l, const std::string &pg="") : stringSol<permSolVarType>(l,l-1,pg) { }
	/** Initialization with random permutation. */
	void initialize(int count);
	/** Calls concrete mutation method. Controlled by paramter permmop(). */
	void mutate(int count);
	/** Calls crossover according to the permxop() parameter. */
	void crossover(const mh_solution &parA,const mh_solution &parB);
	/** Function to apply a certain move.
	        This will only work with a swapMove. */
	void applyMove(const nhmove &m);
};

} // end of namespace mh

#endif //MH_PERMSOL_H
