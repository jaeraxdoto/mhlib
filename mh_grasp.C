// mh_grasp.C

#include "mh_grasp.h"
#include "mh_interfaces.h"
#include "mh_pop.h"
#include "mh_util.h"
#include "mh_allalgs.h"

namespace mh {

GRASP::GRASP(pop_base &p, const std::string &pg) : lsbase(p,pg)
{
	if ( dynamic_cast<gcProvider*>(tmpSol) == 0 )
		mherror("Chromosome is not a gcProvider");
	if ( pop->size() < 2 )
		mherror("Population is to small");
	spop = new population(*tmpSol, 1, true, false, pgroupext(pgroup,"sub"));
}

GRASP::~GRASP()
{
	delete spop;
}

void GRASP::performIteration()
{
	checkPopulation();

	perfIterBeginCallback();

	/* Phase 1: greedy construction of a chromosome */
	gcProvider *gp = dynamic_cast<gcProvider*>(spop->at(0));
	gp->greedyConstruct();

	/* Phase 2: apply another local search alike algorithm */
	mh_advbase *alg = create_mh( *spop, pgroupext(pgroup,"sub") );

	alg->run();

	spop->setAlgorithm(this);

	addStatistics(alg);
	
	delete alg;

	mh_solution::cast(tmpSol)->copy(mh_solution::cast(*spop->at(0)));
	if (pop->at(0)->isWorse(*tmpSol))
		tmpSol=replace(mh_solution::cast(tmpSol));

	nIteration++;

	perfIterEndCallback();
}

} // end of namespace mh

