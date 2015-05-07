// mh_genea.C

#include <iomanip>
#include "mh_util.h"
#include "mh_genea.h"


bool_param elit("elit","use elitism?",true);

generationalEA::generationalEA(pop_base &p, const pstring &pg) : mh_advbase(p,pg)
{
	selectedChroms=new int[pop->size()];
	nextGeneration=new mh_solution*[pop->size()];
	for (int i=0;i<pop->size();i++)
	{
		nextGeneration[i]=pop->bestSol()->createUninitialized();
	}
}

generationalEA::~generationalEA()
{
	delete selectedChroms;
	for (int i=0;i<pop->size();i++)
	{
		delete nextGeneration[i];
	}
	delete nextGeneration;
}

void generationalEA::performIteration()
{
	checkPopulation();
	
	perfIterBeginCallback();

	// create new generation
	createNextGeneration();
	
	// replace generation
	saveBest();
	for (int i=0;i<pop->size();i++)
	{
		nextGeneration[i]=pop->replace(i,nextGeneration[i]);
	}
	checkBest();
	nIteration++;
	
	perfIterEndCallback();
}

void generationalEA::createNextGeneration()
{
	checkPopulation();
	
	int start=elit(pgroup) ? 1 : 0;
	
	// perform crossover
	if (elit(pgroup))
	{
		mh_solution *pp1=pop->bestSol();
		nextGeneration[0]->copy(*pp1);
	}
	for (int i=start;i<pop->size();i++)
	{
		int p1=select();
		if (random_prob(pcross(pgroup)))
		{
			// recombination and mutation
			int p2=select();
			mh_solution *pp1=pop->at(p1);
			mh_solution *pp2=pop->at(p2);
			performCrossover(pp1,pp2,nextGeneration[i]);
		}
		else
		{
			// no recombination
			nextGeneration[i]->reproduce(*pop->at(p1));
		}
	}
	
	// perform mutation and local improvement
	if (pmut(pgroup)!=0)
		for (int i=start;i<pop->size();i++)
		{
			performMutation(nextGeneration[i],pmut(pgroup));
			if (plocim(pgroup)>0 && random_prob(plocim(pgroup)))
			{
				tmpSol->locallyImprove();
				nLocalImprovements++;
			}
		}
}
