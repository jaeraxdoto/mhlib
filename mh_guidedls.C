// mh_guidedls.C

#include "mh_guidedls.h"
#include "mh_localsearch.h"
#include "mh_pop.h"
#include "mh_util.h"
#include "mh_allalgs.h"


/// Interval for penalty resets.
int_param glsri("glsri","Interval for penalty resets.", 0, 0, LOWER_EQUAL );

guidedLS::guidedLS(pop_base &p, const pstring &pg) : lsbase(p,pg), lambda(0)
{
	featureProvider *fp = dynamic_cast<featureProvider*>(tmpChrom);
	if ( fp==NULL )
		mherror("Chromosome is not a featureProvider");
	else
		f = fp->getFeature();
	if ( pop->size() < 2 )
		mherror("Population is to small");
	spop = new population(*tmpChrom, 1, true, false, pgroupext((pstring)pgroup,"sub"));
}

guidedLS::~guidedLS()
{
	delete f;
	delete spop;
}

void guidedLS::performGeneration()
{
	mh_advbase *alg;

	checkPopulation();

	perfGenBeginCallback();

	// Phase 1: apply local search 
	alg = create_mh( *spop, pgroupext((pstring)pgroup,"sub") );

	glsSubAlgorithm *sa = dynamic_cast<glsSubAlgorithm*>(alg);

	if ( sa==NULL )
		mherror("Sub-Algorithm is not a glsSubAlgorithm");
	
	sa->gls=this;

	alg->run();

	spop->setAlgorithm(this);

	addStatistics(alg);
	
	delete alg;

	if ( lambda==0 )
		lambda = f->tuneLambda(pop->at(1));

	tmpChrom->reproduce(*spop->at(0));
	if (pop->at(0)->isWorse(*tmpChrom))
		tmpChrom=replace(tmpChrom);

	// Phase 2: update penalties
	if ( glsri(pgroup) > 0 && nGeneration % glsri(pgroup) == 0 )
		f->resetPenalties();
	else
		f->updatePenalties(tmpChrom);
	
	nGeneration++;

	perfGenEndCallback();
}

double guidedLS::aobj(mh_solution *c)
{
	return lambda * f->penalty(c);
}

double guidedLS::delta_aobj(mh_solution *c, const nhmove *m)
{
	return lambda * f->delta_penalty(c,m);
}
