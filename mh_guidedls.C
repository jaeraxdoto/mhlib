// mh_guidedls.C

#include "mh_guidedls.h"
#include "mh_localsearch.h"
#include "mh_pop.h"
#include "mh_util.h"
#include "mh_allalgs.h"

namespace mh {

/// Interval for penalty resets.
int_param glsri("glsri","GLS interval for penalty resets", 0, 0, LOWER_EQUAL );

guidedLS::guidedLS(pop_base &p, const std::string &pg) : lsbase(p,pg), lambda(0)
{
	featureProvider *fp = dynamic_cast<featureProvider*>(tmpSol);
	if ( fp==nullptr )
		mherror("Solution is not a featureProvider");
	else
		f = fp->getFeature();
	if ( pop->size() < 2 )
		mherror("Population is to small");
	spop = new population(*tmpSol, 1, true, false, pgroupext(pgroup,"sub"));
}

guidedLS::~guidedLS()
{
	delete f;
	delete spop;
}

void guidedLS::performIteration()
{
	mh_advbase *alg;

	checkPopulation();

	perfIterBeginCallback();

	// Phase 1: apply local search 
	alg = create_mh( *spop, pgroupext(pgroup,"sub") );

	glsSubAlgorithm *sa = dynamic_cast<glsSubAlgorithm*>(alg);

	if ( sa==nullptr )
		mherror("Subalgorithm is not a glsSubAlgorithm");
	
	sa->gls=this;

	alg->run();

	spop->setAlgorithm(this);

	addStatistics(alg);
	
	delete alg;

	if ( lambda==0 )
		lambda = f->tuneLambda(pop->at(1));

	tmpSol->copy(*spop->at(0));
	if (pop->at(0)->isWorse(*tmpSol))
		tmpSol=replace(tmpSol);

	// Phase 2: update penalties
	if ( glsri(pgroup) > 0 && nIteration % glsri(pgroup) == 0 )
		f->resetPenalties();
	else
		f->updatePenalties(tmpSol);
	
	nIteration++;

	perfIterEndCallback();
}

double guidedLS::aobj(mh_solution *c)
{
	return lambda * f->penalty(c);
}

double guidedLS::delta_aobj(mh_solution *c, const nhmove *m)
{
	return lambda * f->delta_penalty(c,m);
}

} // end of namespace mh

