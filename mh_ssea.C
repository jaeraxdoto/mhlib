// mh_ssea.C

#include <iomanip>
#include "mh_util.h"
#include "mh_solution.h"
#include "mh_ssea.h"
#include "mh_gaopsprov.h"

namespace mh {

bool_param dcdag("dcdag","do not count duplicate as generation",false);

double_param pmutnc("pmutnc","mutation prob. for sols created via recombination",
	0.0,-2000.0,1000.0);

bool_param cntopd("cntopd","count operator duplicates",false);

void steadyStateEA::performIteration()
{
	checkPopulation();

	perfIterBeginCallback();
	
	// create a new solution
	int p1=select();
	if (random_prob(pcross(pgroup)))
	{
		// recombination and mutation
		int p2=select();
		mh_solution *pp1=pop->at(p1);
		mh_solution *pp2=pop->at(p2);
		performCrossover(pp1,pp2,tmpSol);
		performMutation(tmpSol,pmut(pgroup));
	}
	else
	{
		// no recombination
		tmpSol->copy(*pop->at(p1));
		double pm=pmutnc(pgroup);
		if (pm==0)
			pm=pmut(pgroup);
		performMutation(tmpSol,pm);
	}

	// optionally locally improve the chromosome
	if (plocim(pgroup) && random_prob(plocim(pgroup)))
	{
		gaopsProvider::cast(*tmpSol).locallyImprove();
		nLocalImprovements++;
	}

	// replace in population
	mh_solution *r=tmpSol;
	tmpSol=replace(tmpSol);
	if (!dcdag(pgroup) || r!=tmpSol)
		nIteration++;

	perfIterEndCallback();

}

} // end of namespace mh

