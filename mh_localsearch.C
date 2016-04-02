// mh_localsearch.C

#include "mh_localsearch.h"
#include "mh_ssea.h"
#include "mh_util.h"

namespace mh {

void localSearch::performIteration()
{
	checkPopulation();

	perfIterBeginCallback();

	mh_solution *pold=mh_solution::to_mh_solution(pop->at(0));

	mh_solution::to_mh_solution(tmpSol)->reproduce(mh_solution::to_mh_solution(*pold));
	mh_solution::to_mh_solution(tmpSol)->selectNeighbour();

	if (pold->isWorse(*tmpSol))
	{
		mh_solution *r=mh_solution::to_mh_solution(tmpSol);
		tmpSol=replace(mh_solution::to_mh_solution(tmpSol));
		if (!dcdag(pgroup) || r!=tmpSol)
			nIteration++;
		return;
	}

	nIteration++;

	perfIterEndCallback();
}

} // end of namespace mh

