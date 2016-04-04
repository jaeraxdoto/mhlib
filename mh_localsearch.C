// mh_localsearch.C

#include "mh_localsearch.h"
#include "mh_ssea.h"
#include "mh_util.h"

namespace mh {

int_param mvnbop( "mvnbop", "step function 0:rand. neigh., 1:first imp. 2:best imp.", 0, 0, 2 );

void localSearch::performIteration()
{
	checkPopulation();

	perfIterBeginCallback();

	mh_solution *pold=&mh_solution::cast(*pop->at(0));

	mh_solution::cast(*tmpSol).copy(mh_solution::cast(*pold));
	mh_solution::cast(*tmpSol).selectNeighbour(mvnbop(pgroup));

	if (pold->isWorse(*tmpSol))
	{
		mh_solution *r=&mh_solution::cast(*tmpSol);
		tmpSol=replace(&mh_solution::cast(*tmpSol));
		if (!dcdag(pgroup) || r!=tmpSol)
			nIteration++;
		return;
	}

	nIteration++;

	perfIterEndCallback();
}

} // end of namespace mh

