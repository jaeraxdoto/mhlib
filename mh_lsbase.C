// mh_lsbase.C

#include "mh_lsbase.h"
#include "mh_ssea.h"
#include "mh_util.h"

namespace mhlib {

lsbase::lsbase(pop_base &p, const std::string &pg) : mh_advbase(p,pg)
{
	checkPopulation();
	wheap.set(false,pgroup);
	mh_solution *c=pop->at(0);
	c->copy(*pop->bestSol());
}

mh_solution *lsbase::replace(mh_solution *p)
{
	checkPopulation();
	mh_solution *pold=pop->at(0);
	if (dupelim(pgroup) && p->equals(*pold))
		return p;
	saveBest();
	//p->setAlgorithm(this);
	mh_solution *replaced=pop->replace(0,p);
	checkBest();
	return replaced;
}

} // end of namespace mhlib

