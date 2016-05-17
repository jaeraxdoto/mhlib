// mh_tabusearch.C

#include <limits.h>
#include "mh_gaopsprov.h"
#include "mh_tabusearch.h"
#include "mh_util.h"

namespace mh {

tabuSearch::tabuSearch(pop_base &p, const std::string &pg) : lsbase(p,pg)
{
	tl_ne = new tabulist(pgroup);
	curChrom = pop->at(0)->clone();

	// dynamic_cast to see if we are using a tabuProvider
	if ( dynamic_cast<tabuProvider*>(tmpSol) == 0 )
		mherror("Solution is not a tabuProvider");
}

tabuSearch::~tabuSearch()
{
	delete curChrom;
	delete tl_ne;
}

void tabuSearch::performIteration()
{
	checkPopulation();

	perfIterBeginCallback();

	mh_solution *pold=pop->at(0);
	gaopsProvider::cast(*curChrom).selectNeighbour();

	tmpSol->copy(*curChrom);
	
	if (tmpSol->isBetter(*pold))
		tmpSol=replace(tmpSol);

	nIteration++;

	perfIterEndCallback();
}

bool tabuSearch::isTabu(tabuAttribute *t)
{
	if ( tl_ne->match( t ) )
		return true;
	else
	{
		nTabus++;
		return false;
	}
			
}

bool tabuSearch::aspiration(mh_solution *c)
{
	if ( c->isBetter(*pop->at(0)) )
	{
		nAspirations++;
		return true;
	}
	else
		return false;
}

} // end of namespace mh

