// mh_lsbase.C

#include "mh_lsbase.h"
#include "mh_ssea.h"
#include "mh_util.h"

namespace mh {

lsbase::lsbase(pop_base &p, const std::string &pg) : mh_eaadvbase(p,pg)
{
	checkPopulation();
	wheap.set(false,pgroup);
	mh_solution *c=mh_solution::cast(pop->at(0));
	c->copy(*pop->bestSol());
}

mh_solution *lsbase::replace(mh_solution *p)
{
	checkPopulation();
	mh_solution *pold=mh_solution::cast(pop->at(0));
	if (dupelim(pgroup) && p->equals(*pold))
		return p;
	saveBest();
	//p->setAlgorithm(this);
	mh_solution *replaced=mh_solution::cast(pop->replace(0,p));
	checkBest();
	return replaced;
}

} // end of namespace mh

