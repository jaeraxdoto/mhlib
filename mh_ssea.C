// mh_ssea.C

#include <iomanip>
#include "mh_util.h"
#include "mh_ssea.h"


bool_param dcdag("dcdag","don't count duplicate as generation",false);

double_param pmutnc("pmutnc","mutation prob. for chroms created via crossover",
	0.0,-2000.0,1000.0);

bool_param cntopd("cntopd","count operator duplicates",false);

void steadyStateEA::performGeneration()
{
	checkPopulation();

	perfGenBeginCallback();
	
	// create a new solution
	int p1=select();
	if (random_prob(pcross(pgroup)))
	{
		// recombination and mutation
		int p2=select();
		mh_solution *pp1=pop->at(p1);
		mh_solution *pp2=pop->at(p2);
		performCrossover(pp1,pp2,tmpChrom);
		performMutation(tmpChrom,pmut(pgroup));
	}
	else
	{
		// no recombination
		tmpChrom->reproduce(*pop->at(p1));
		double pm=pmutnc(pgroup);
		if (pm==0)
			pm=pmut(pgroup);
		performMutation(tmpChrom,pm);
	}

	// optionally locally improve the chromosome
	if (plocim(pgroup) && random_prob(plocim(pgroup)))
	{
		tmpChrom->locallyImprove();
		nLocalImprovements++;
	}

	// replace in population
	mh_solution *r=tmpChrom;
	tmpChrom=replace(tmpChrom);
	if (!dcdag(pgroup) || r!=tmpChrom)
		nGeneration++;

	perfGenEndCallback();

}
