// mh_tabusearch.C

#include <limits.h>
#include "mh_tabusearch.h"
#include "mh_util.h"


tabuSearch::tabuSearch(pop_base &p, const pstring &pg) : lsbase(p,pg)
{
	tl_ne = new tabulist((pstring)pgroup);
	curChrom = pop->at(0)->clone();

	// dynamic_cast to see if we are using a tabuProvider chromosome
	if ( dynamic_cast<tabuProvider*>(tmpChrom) == 0 )
		mherror("Chromosome is not a tabuProvider");
}

tabuSearch::~tabuSearch()
{
	delete curChrom;
	delete tl_ne;
}

void tabuSearch::performGeneration()
{
	checkPopulation();

	perfGenBeginCallback();

	mh_solution *pold=pop->at(0);
	curChrom->selectNeighbour();

	tmpChrom->copy(*curChrom);
	
	if (tmpChrom->isBetter(*pold))
		tmpChrom=replace(tmpChrom);

	nGeneration++;

	perfGenEndCallback();
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
