// mh_advbase.C

#include <stdio.h>
#include <iomanip>
#include "mh_advbase.h"
#include "mh_solution.h"
#include "mh_gaopsprov.h"
#include "mh_island.h"
#include "mh_genea.h"
#include "mh_grasp.h"
#include "mh_guidedls.h"
#include "mh_pop.h"
#include "mh_localsearch.h"
#include "mh_simanneal.h"
#include "mh_ssea.h"
#include "mh_tabusearch.h"
#include "mh_util.h"
#include "mh_vnd.h"
#include "mh_vns.h"

namespace mh {

using namespace std;

double_param pcross("pcross","crossover probability",1.0,0.0,1.0);

double_param pmut("pmut","mutation probability",-1.0,-2000.0,1000.0);

double_param plocim("plocim","probability for applying local improvement",
	0.0,0.0,1.0);


void mh_eaadvbase::performCrossover(mh_solution *p1,mh_solution *p2, mh_solution *c)
{
	gaopsProvider::cast(*c).crossover(*p1, *p2);
	nCrossovers++;
	if (cntopd(pgroup))
	{
		if (c->equals(*p1) || c->equals(*p2))
			nCrossoverDups++;
	}
}

void mh_eaadvbase::performMutation(mh_solution *c, double prob)
{
	if (prob==0)
		return;
	if (!cntopd(pgroup))
		nMutations+=gaopsProvider::cast(*c).mutation(prob);
	else
	{
		static mh_solution *tmp2Sol=c->createUninitialized();
		tmp2Sol->copy(*c);
		int muts=gaopsProvider::cast(*tmpSol).mutation(prob);
		nMutations+=muts;
		if (muts>0 && tmp2Sol->equals(*c))
			nMutationDups+=muts;
	}
}

void mh_eaadvbase::printStatistics(ostream &ostr)
{
	mh_advbase::printStatistics(ostr);
	ostr << "crossovers:\t" << nCrossovers << endl;
	ostr << "mutations:\t" << nMutations << endl;
	if (cntopd(pgroup))
	{
		ostr << "crossover-duplicates:\t" << nCrossoverDups << endl;
		ostr << "mutation-duplicates:\t" << nMutationDups << endl;
	}
	ostr << "local improvements:\t"  << nLocalImprovements << endl;
	ostr << "duplicate eliminations:\t" << nDupEliminations << endl;
	ostr << "deteriorations\t" << nDeteriorations << endl;
	ostr << "aspirations:\t" << nAspirations << endl;
	ostr << "tabus:\t\t" << nTabus << endl;
}

void mh_eaadvbase::addStatistics(const mh_advbase *a)
{
	if ( a!=nullptr )
	{
		mh_advbase::addStatistics(a);
		const mh_eaadvbase &ea = dynamic_cast<const mh_eaadvbase &>(*a);
		nCrossovers        += ea.nCrossovers;
		nMutations         += ea.nMutations;
		nCrossoverDups     += ea.nCrossoverDups;
		nMutationDups      += ea.nMutationDups;
		nLocalImprovements += ea.nLocalImprovements;
		nTabus             += ea.nTabus;
		nAspirations       += ea.nAspirations;
		nDeteriorations    += ea.nDeteriorations;
	}
}

} // end of namespace mh

