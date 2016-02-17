// mh_subpop.C

#include <cmath>
#include <iomanip>
#include "mh_subpop.h"
#include "mh_util.h"

namespace mh {

using namespace std;

void subPopulation::determineBest()
{
	indexBest=0;
	for (int i=1;i<nSolutions;i++)
		if (at(i)->isBetter(*at(indexBest)))
			indexBest=i;
}

int subPopulation::determineWorst() const
{
	int idx=0;
	if (phash && wheap(pgroup))
	{
		idx=phash->worstIndex();
		if (idx==indexBest)	// avoid that indexBest is returned
			idx=(idx+1)%nSolutions;
	}
	else
	{
		for (int i=1;i<nSolutions;i++)
			if (!superPopulation->at(indexFrom+i)->isBetter(
					*superPopulation->at(indexFrom+idx)) && i!=indexBest)
				idx=i;
	}
	return idx;
}

subPopulation::subPopulation(pop_base *super,int from,int to, const string &pg)
	: pop_base(to-from+1,pg)
{
	superPopulation=super;
	indexFrom=from; indexTo=to;
	if ((from<0) || (to>super->size()))
		mherror("Bad indizes for Sub-population");
	if (phash)
		for (int i=0;i<nSolutions;i++)
			phash->add(at(i),i);
	determineBest();
}

mh_solution *subPopulation::replace(int index,mh_solution *newchrom)
{
	mh_solution *old=superPopulation->replace(indexFrom+index,newchrom);
	statValid=false;
	if (phash)
	{
		phash->remove(old);
		phash->add(newchrom,index);
	}
	if (newchrom->isBetter(*at(indexBest)))
		indexBest=index;
	else if (index==indexBest)
		determineBest();
	return old; 
}

int subPopulation::findDuplicate(mh_solution *p)
{
	if (phash)
		return phash->findDuplicate(p);
	else
	{
		for (int i=0;i<nSolutions;i++)
			if (p->equals(*at(i)))
				return i;
		return -1;
	}
}

void subPopulation::write(ostream &ostr)
{
	ostr << "# Population:" << endl;
	for (int i=0;i<nSolutions;i++)
	{
		ostr << i << ":\t" << at(i)->obj() << '\t';
		at(i)->write(ostr,0);
		ostr << std::endl;
	}
	ostr << endl;
}

void subPopulation::validateStat()
{
	if (statValid)
		return;
	double sum=0,sum2=0;
	int idxwor=0;
	for (int i=0;i<nSolutions;i++)
	{
		double o=at(i)->obj();
		sum+=o;
		sum2+=o*o;
		if (at(i)->isWorse(*at(idxwor)))
			idxwor=i;
	}
	statMean=sum/nSolutions;
	statWorst=at(idxwor)->obj();
	statDev=sqrt(sum2/nSolutions-statMean*statMean);
	statValid=true;
}

void subPopulation::setAlgorithm(mh_base *alg)
{
	for (int i=0;i<nSolutions;i++)
		at(i)->setAlgorithm(alg);
}

} // end of namespace mh

