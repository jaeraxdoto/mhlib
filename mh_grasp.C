// mh_grasp.C

#include "mh_grasp.h"
#include "mh_interfaces.h"
#include "mh_pop.h"
#include "mh_util.h"
#include "mh_allalgs.h"

GRASP::GRASP(pop_base &p, const pstring &pg) : lsbase(p,pg)
{
	if ( dynamic_cast<gcProvider*>(tmpChrom) == 0 )
		mherror("Chromosome is not a gcProvider");
	if ( pop->size() < 2 )
		mherror("Population is to small");
	spop = new population(*tmpChrom, 1, true, pgroupext((pstring)pgroup,"sub"));
}

GRASP::~GRASP()
{
	delete spop;
}

void GRASP::performGeneration()
{
	checkPopulation();

	perfGenBeginCallback();

	/* Phase 1: greedy construction of a chromosome */
	gcProvider *gp = dynamic_cast<gcProvider*>(spop->at(0));
	gp->greedyConstruct();

	/* Phase 2: apply another local search alike algorithm */
	mh_advbase *alg = create_mh( *spop, pgroupext((pstring)pgroup,"sub") );

	alg->run();

	spop->setAlgorithm(this);

	addStatistics(alg);
	
	delete alg;

	tmpChrom->reproduce(*spop->at(0));
	if (pop->at(0)->isWorse(*tmpChrom))
		tmpChrom=replace(tmpChrom);

	nGeneration++;

	perfGenEndCallback();
}
