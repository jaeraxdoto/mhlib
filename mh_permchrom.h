/*! \file mh_permchrom.h 
	\brief A permutation string chromosom with values 0...length-1.
	*/

#ifndef MH_PERMCHROM_H
#define MH_PERMCHROM_H

#include <iostream>

#include "mh_nhmove.h"
#include "mh_stringchrom.h"

/** \ingroup param
	Used crossover operator for permChrom:
	- 0: random choice,
	- 1: PMX,
	- 2: OX,
	- 3: CX,
	- 4: UOBX,
	- 5: C1. */
extern int_param permxop;

/** \ingroup param
	Used mutation operator for permChrom:
	- 0: random choice,
	- 1: inversion,
	- 2: reciprocal exchange,
	- 3: insertion. */
extern int_param permmop;

/** Type of individual genes in permChrom. */
typedef unsigned int permChromGeneType;

/** A chromosome class for permutation problems with values 0...length-1. */
class permChrom : public stringChrom<permChromGeneType>
{
protected:
	static const permChrom &toPermChrom(const mh_solution &ref)
	{ return (dynamic_cast<const permChrom &>(ref)); }

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
	/** This is the C1 crossover. Up to a random cut point, all genes
	are copied from the first chromosome; all remaining genes are
	appended in the order as they appear in the second chromosome. */
	void crossover_c1(const mh_solution &parA,const mh_solution &parB);
	/* This is the ERX operator. */
	// void crossover_erx(const chromosome &parA,const chromosome &parB);
	/* This is the enhanced ERX operator. */
	// void crossover_eerx(const chromosome &parA,const chromosome &parB);
	/* This is the MPX operator. */
	// void crossover_mpx(const chromosome &parA,const chromosome &parB);

public:
	permChrom(const mh_solution &c);
	permChrom(int l, mh_base *t, const pstring &pg=(pstring)("")) : stringChrom<permChromGeneType>(l,l-1,t,pg) { }
	permChrom(int l, const pstring &pg=(pstring)("")) : stringChrom<permChromGeneType>(l,l-1,pg) { }
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

#endif //MH_PERMCHROM_H
