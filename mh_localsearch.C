// mh_localsearch.C

#include "mh_localsearch.h"
#include "mh_ssea.h"
#include "mh_util.h"

void localSearch::performIteration()
{
	checkPopulation();

	perfIterBeginCallback();

	mh_solution *pold=pop->at(0);

	tmpSol->reproduce(*pold);
	tmpSol->selectNeighbour();

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
