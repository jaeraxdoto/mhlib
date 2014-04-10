// mh_localsearch.C

#include "mh_localsearch.h"
#include "mh_ssea.h"
#include "mh_util.h"

void localSearch::performGeneration()
{
	checkPopulation();

	perfGenBeginCallback();

	mh_solution *pold=pop->at(0);

	tmpChrom->reproduce(*pold);
	tmpChrom->selectNeighbour();

	if (pold->isWorse(*tmpChrom))
	{
		mh_solution *r=tmpChrom;
		tmpChrom=replace(tmpChrom);
		if (!dcdag(pgroup) || r!=tmpChrom)
			nGeneration++;
		return;
	}

	nGeneration++;

	perfGenEndCallback();
}
