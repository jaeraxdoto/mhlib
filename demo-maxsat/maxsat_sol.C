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

bool MAXSATSol::construct(int k) {
	initialize(k);
	// invalidate();	// call if you provide your own method and reevaluation needed
	return true;
}

bool MAXSATSol::localimp(int k)
{
	// invalidate();	// call if you provide your own method and reevaluation needed
	return false;	// no change
}

bool MAXSATSol::shaking(int k)
{
	mutate(k);
	// invalidate();	// call if you provide your own method and reevaluation needed
	return true;		// solution changed in general
}

} // maxsat namespace

