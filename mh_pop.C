// mh_pop.C

#include <cmath>
#include <iomanip>
#include "mh_pop.h"
#include "mh_solution.h"

namespace mh {

using namespace std;

void population::determineBest()
{
	indexBest=0;
	for (int i=1;i<nSolutions;i++)
		if (chroms[i]->isBetter(*chroms[indexBest]))
			indexBest=i;
}

int population::determineWorst() const
{
	int idx=0;
	if (phash && wheap(pgroup))
	{
		idx=phash->worstIndex();
		if (idx==indexBest)	// avoid that indexBest is returned
			idx=(idx+1)%popsize(pgroup);
	}
	else
	{
		for (int i=1;i<nSolutions;i++)
			if (!chroms[i]->isBetter(*chroms[idx]) && i!=indexBest)
				idx=i;
	}
	return idx;
}

population::population(std::function<mh_solution *()> createsol, int psize, bool binit,
		bool nohashing, const std::string &pg) : pop_base(psize,nohashing,pg)
{
	chroms=new mh_solution *[nSolutions];
	for (int i=0;i<nSolutions;i++)
		chroms[i]=createsol();
	if (binit)
		initialize();
	determineBest();
}

population::population(const mh_solution &c_template, int psize, bool binit, bool nohashing, const std::string &pg)
	: pop_base(psize,nohashing,pg)
{
	chroms=new mh_solution *[nSolutions];
	for (int i=0;i<nSolutions;i++)
		if (binit)
			chroms[i]=c_template.createUninitialized();
		else
			chroms[i]=c_template.clone();
	if (binit)
		initialize();
	determineBest();
}

population::population(const mh_solution &c_template, const std::string &pg)
	: pop_base(pg)
{
	chroms=new mh_solution *[nSolutions];
	for (int i=0;i<nSolutions;i++) {
		chroms[i]=c_template.createUninitialized();
	}
	initialize();
	determineBest();
}

population::~population()
{
        for (int i=0;i<nSolutions;i++)
		delete chroms[i];
	delete [] chroms;
}

void population::initialize()
{
	int initcall=0;
	for (int i=0;i<nSolutions;i++)
	{
		do
		{
			chroms[i]->initialize(initcall++);
		}
		while (dupelim(pgroup)==2 && findDuplicate(chroms[i])!=-1);
		if (phash) {
			phash->add(chroms[i],i);
		}
	}
	statValid=false;
}

mh_solution *population::replace(int index,mh_solution *newchrom)
{
	mh_solution *old=chroms[index];
	chroms[index]=newchrom;
	statValid=false;
	if (phash)
	{
		phash->remove(old);
		phash->add(newchrom,index);
	}
	if (newchrom->isBetter(*chroms[indexBest]))
		indexBest=index;
	else if (index==indexBest)
		determineBest();
	return old; 
}

void population::update(int index, mh_solution *newchrom)
{
	statValid=false;
	if (phash)
		phash->remove(chroms[index]);
	chroms[index]->copy(*newchrom);
	if (phash)
		phash->add(newchrom,index);
	if (newchrom->isBetter(*chroms[indexBest]))
		indexBest=index;
	else if (index==indexBest)
		determineBest();
}


int population::findDuplicate(mh_solution *p)
{
	if (phash)
		return phash->findDuplicate(p);
	else
	{
		for (int i=0;i<nSolutions;i++)
			if (p->equals(*chroms[i]))
				return i;
		return -1;
	}
}

void population::write(ostream &ostr)
{
	ostr << "# Population:" << endl;
	for (int i=0;i<nSolutions;i++)
	{
		ostr << i << ":\t" << chroms[i]->obj() << '\t';
		chroms[i]->write(ostr,0);
		ostr << endl;
	}
	ostr << endl;
}

void population::validateStat()
{
	if (statValid)
		return;
	double sum=0,sum2=0;
	int idxwor=0;
	for (int i=0;i<nSolutions;i++)
	{
		double o=chroms[i]->obj();
		sum+=o;
		sum2+=o*o;
		if (chroms[i]->isWorse(*chroms[idxwor]))
			idxwor=i;
	}
	statMean=sum/nSolutions;
	statWorst=chroms[idxwor]->obj();
	statDev=sqrt(sum2/nSolutions-statMean*statMean);
	statValid=true;
}

void population::setAlgorithm(mh_base *alg)
{
	for (int i=0;i<nSolutions;i++)
		chroms[i]->setAlgorithm(alg);
}

} // end of namespace mh

