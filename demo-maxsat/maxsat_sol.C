/*! \file maxsat_sol.C
    \brief The implementation of the MAXSATSol class, which represents a soltuion to the
    MAXSAT problem.
    \include maxsat_sol.C */

#include <string>
#include <cstdlib>
#include "mh_util.h"

#include "maxsat_sol.h"

using namespace mh;
using namespace maxsat;

namespace maxsat {

double MAXSATSol::objective()
{
	int fulfilled=0;
	for (auto &clause : probinst->clauses) {
		for (auto &v : clause)
			if (data[std::abs(v)-1] == (v>0?1:0)) {
				// clause fulfilled
				fulfilled++;
				break;	// continue with next clause
			}
	}
	return fulfilled;
}

void MAXSATSol::construct(int k, SchedulerMethod::Result &result) {
	initialize(k);
	// invalidate();	// call if you provide your own method and reevaluation needed
	// result is kept at its default, i.e., is automatically derived
}

void MAXSATSol::localimp(int k, SchedulerMethod::Result &result)
{
	// invalidate();	// call if you provide your own method and reevaluation needed
	if (!k_flip_localsearch(k))
		result.changed = false; // solution is not changed, hint this to the further processing
	// Otherwise, result is kept at its default, i.e., is automatically derived
}

#include<iostream>

void MAXSATSol::randlocalimp(int k, SchedulerMethod::Result &result)
{
	// invalidate();	// call if you provide your own method and reevaluation needed
	MAXSATSol orig = *this;	// copy original
	// try
	for (int i=0; i<length; i++) {
		mutate_flip(k);
		if (this->isBetter(orig))
			return;	// better solution found, return with it
		copy(orig);
	}
	// no better solution found in length iterations, but maybe next time:
	result.changed = 0; // original solution has been restored
	// Reconsider this neighborhood in the VND 10 times:
	if (result.callCounter < 10)
		result.reconsider = 1; // this neighborhood should be reconsidered for this solution
}

void MAXSATShakingMethod::run(mh_solution *sol, SchedulerMethod::Result &result) const {
	MAXSATSol::cast(*sol).mutate_flip(par);
	// sol->invalidate();	// call if you provide your own method and reevaluation needed
	// result is kept at its default, i.e., is automatically derived
}

} // maxsat namespace

