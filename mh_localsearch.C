// mh_localsearch.C

#include "mh_localsearch.h"
#include "mh_ssea.h"
#include "mh_util.h"
#include "mh_gaopsprov.h"

namespace mh {

void localSearch::performIteration()
{
	checkPopulation();

	perfIterBeginCallback();

	mh_solution *pold=pop->at(0);

	tmpSol->copy(*pold);
	gaopsProvider::cast(*tmpSol).selectNeighbour();

	if (pold->isWorse(*tmpSol))
	{
		mh_solution *r=tmpSol;
		tmpSol=replace(tmpSol);
		if (!dcdag(pgroup) || r!=tmpSol)
			nIteration++;
		return;
	}

	nIteration++;

	perfIterEndCallback();
}

} // end of namespace mh

