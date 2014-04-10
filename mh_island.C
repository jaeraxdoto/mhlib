// mh_island.C

#include <iomanip>
#include "mh_island.h"
#include "mh_ssea.h"
#include "mh_subpop.h"
#include "mh_util.h"


int_param islk("islk","island count",4,2,100);

int_param migr("migr","migration strategy: 0:best, 1:none",0,0,1);

double_param pmig("pmig","migration probability",0.001,0.0,1.0);

islandModelEA::islandModelEA(pop_base &p, mh_advbase *mh_templ, const pstring &pg) : mh_advbase(p,pg)
{
	init(mh_templ);
}

islandModelEA::islandModelEA(pop_base &p, const pstring &pg) : mh_advbase(p,pg)
{
	init(new steadyStateEA((pstring)pgroup));
}

void islandModelEA::init(mh_advbase *mh_templ)
{
	subPopulation *subPop;
	
	if (pop->size() % islk(pgroup)!=0)
		mherror("Wrong island count, has to be a divider of the population size");
	int islandSize=pop->size()/islk(pgroup);
	
	subEAs = new mh_advbase*[islk(pgroup)];
	for (int i=0;i<islk(pgroup);i++)
	{
		subPop=new subPopulation(pop, i*islandSize, (i+1)*islandSize-1,(pstring)pgroup);
		subEAs[i]=mh_templ->clone(*subPop,(pstring)pgroup);
	}
}

islandModelEA::~islandModelEA()
{
	for (int i=0;i<islk(pgroup);i++)
	{
		delete subEAs[i];
	}
	delete subEAs;
}

void islandModelEA::run()
{
	checkPopulation();
	
	writeLogHeader();
	writeLogEntry();
	logstr.flush();
	//pop->bestChrom()->write(cout);
	if (!terminate())
		for(;;)
		{
			performGeneration();
			sumStatistics();
			if (terminate())
			{
				// write last generation info in any case
				writeLogEntry(true);
				//pop->bestChrom()->write(cout);
				break;	// ... and stop
			}
			else
			{
				// write generation info
				writeLogEntry();
				//pop->bestChrom()->write(cout);
				// do migration
				performMigration();
			}
		}
		logstr.flush();
}

void islandModelEA::performGeneration()
{
	checkPopulation();
	
	saveBest();
	for (int i=0;i<islk(pgroup);i++)
	{
		subEAs[i]->performGeneration();
	}
	checkBest();
	nGeneration++;
}

void islandModelEA::performMigration()
{
	checkPopulation();
	
	if (migr(pgroup)==1)
		return;
	
	for (int i=0;i<islk(pgroup);i++)
	{
		if (random_prob(pmig(pgroup)))
		{
			mh_solution *t=subEAs[i]->pop->bestSol();
			for (int iii=0;iii<islk(pgroup);iii++)
				if (i!=iii)
				{
					tmpChrom->copy(*t);
					tmpChrom=subEAs[iii]->replace(tmpChrom);
				}
		}
	}
}

void islandModelEA::sumStatistics()
{
	nSelections=0;
	nCrossovers=0;
	nMutations=0;
	nCrossoverDups=0;
	nMutationDups=0;
	nDupEliminations=0;
	nLocalImprovements=0;
	for (int i=0;i<islk(pgroup);i++)
	{
		nSelections+=subEAs[i]->nSelections;
		nCrossovers+=subEAs[i]->nCrossovers;
		nMutations+=subEAs[i]->nMutations;
		if (cntopd(pgroup))
		{
			nCrossoverDups+=subEAs[i]->nCrossoverDups;
			nMutationDups+=subEAs[i]->nMutationDups;
		}
		nLocalImprovements+=subEAs[i]->nLocalImprovements;
		nDupEliminations+=subEAs[i]->nDupEliminations;
	}
}
