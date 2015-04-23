// mh_advbase.C

#include <stdio.h>
#include <iomanip>
#include "mh_advbase.h"
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

double_param pcross("pcross","crossover probability",1.0,0.0,1.0);

double_param pmut("pmut","mutation probability",-1.0,-2000.0,1000.0);

double_param plocim("plocim","probability for applying local improvement",
	0.0,0.0,1.0);

int_param tcond("tcond","(DEPRICATED) term. crit. (0:gens, 1:conv, 2:obj,\
-1:auto)",-1,-1,2);

int_param tcgen("tcgen","termination on convergence iterations",-1,-1,100000000);

int_param tgen("tgen","termination at iteration",100000,-1,100000000);

double_param tobj("tobj","objective value limit for termination",-1);

double_param ttime("ttime","time limit for termination (in seconds)" ,-1.0, -1.0, LOWER_EQUAL );

int_param tselk("tselk","group size for tournament selection",2,1,10000);

int_param repl("repl","replacement scheme (0:random, 1:worst, -k:TS)",
	1,-1000,1);

bool_param logdups("logdups","log number of elim. dups.?",false);

bool_param logcputime("logcputime","log elapsed cpu time?",false);


mh_advbase::mh_advbase(pop_base &p, const pstring &pg) : mh_base(pg)
{
	pop = &p;
	pop->setAlgorithm(this);
	// create one temporary chromosome which is always used to
	// generate a new solution.
	tmpChrom=pop->bestSol()->createUninitialized();
	// initialize all statistic data
	nGeneration=0;
	nSubGenerations=0;
	nSelections=0;
	nCrossovers=0;
	nMutations=0;
	nCrossoverDups=0;
	nMutationDups=0;
	nDupEliminations=0;
	nLocalImprovements=0;
	nTabus = 0;
	nAspirations = 0;
	nDeteriorations = 0;
	genBest=0;
	timGenBest=0.0;
	timStart=0.0;
	bestObj=0;
	// use worstheap only if wheap() is set and repl()==1 
	// (replace worst)
	if (repl(pgroup)!=1)
		wheap.set(false,pgroup);
}

mh_advbase::mh_advbase(const pstring &pg) : mh_base(pg)
{
	pop=NULL;
	tmpChrom=NULL;
	nGeneration=0;
	nSubGenerations=0;
	nSelections=0;
	nCrossovers=0;
	nMutations=0;
	nCrossoverDups=0;
	nMutationDups=0;
	nDupEliminations=0;
	nLocalImprovements=0;
	nTabus = 0;
	nAspirations = 0;
	nDeteriorations = 0;
	genBest=0;
	timGenBest=0.0;
	timStart=0.0;
	bestObj=0;
}

mh_advbase *mh_advbase::clone(pop_base &p, const pstring &pg)
{
	mherror("clone in class derived from mh_advbase not supported");
	return NULL;
}

mh_advbase::~mh_advbase()
{
	delete tmpChrom;
}

void mh_advbase::run()
{
	checkPopulation();

	timStart=CPUtime();
	
	writeLogHeader();
	writeLogEntry();
	logstr.flush();
	//pop->bestChrom()->write(cout);
	
	if (!terminate())
		for(;;)
		{
			performGeneration();
			if (terminate())
			{
				// write last generation info in any case
				writeLogEntry(true);
				//pop->bestChrom()->write(cout);
				break;	// ... and stop
			}
			else
			{
				// write generation info
				writeLogEntry();
				//pop->bestChrom()->write(cout);
			}
		}
	logstr.emptyEntry();
	logstr.flush();
}

int mh_advbase::tournamentSelection()
{
	checkPopulation();
	
	int k=tselk(pgroup);
	int besti=pop->randomIndex();
	for (int i=1;i<k;i++)
	{
		int ci=pop->randomIndex();
		if (pop->at(ci)->isBetter(*pop->at(besti)))
			besti=ci;
	}
	return besti;
}

void mh_advbase::performCrossover(mh_solution *p1,mh_solution *p2,
	mh_solution *c)
{
	c->crossover(*p1,*p2);
	nCrossovers++;
	if (cntopd(pgroup))
	{
		if (c->equals(*p1) || c->equals(*p2))
			nCrossoverDups++;
	}
}

void mh_advbase::performMutation(mh_solution *c,double prob)
{
	if (prob==0)
		return;
	if (!cntopd(pgroup))
		nMutations+=c->mutation(prob);
	else
	{
		static mh_solution *tmp2Chrom=c->createUninitialized();
		tmp2Chrom->copy(*c);
		int muts=tmpChrom->mutation(prob);
		nMutations+=muts;
		if (muts>0 && tmp2Chrom->equals(*c))
			nMutationDups+=muts;
	}
}

bool mh_advbase::terminate()
{
	checkPopulation();
	
	switch(tcond(pgroup))
	{
		case -1: // new auto scheme, which does not use tcond
			return ((tgen(pgroup) >=0 && nGeneration>=tgen(pgroup)) ||
				(tcgen(pgroup)>=0 && nGeneration-genBest>=tcgen(pgroup)) ||
				(tobj(pgroup) >=0 && (maxi(pgroup)?getBestChrom()->obj()>=tobj(pgroup):
						    getBestChrom()->obj()<=tobj(pgroup))) ||
				(ttime(pgroup)>=0 && ttime(pgroup)<=(CPUtime()-timStart)));
		// DEPRECATED:
		case 0: // terminate after tgen (not tcgen!) generations
			return nGeneration>=tgen(pgroup);
		case 1: // terminate after convergence 
			return nGeneration-genBest>=tgen(pgroup);
		case 2:	// terminate when obj. value tobj() reached
			return maxi(pgroup)? getBestChrom()->obj()>=tobj(pgroup) :
				getBestChrom()->obj()<=tobj(pgroup);
		default:
			mherror("Invalid tcond",tcond.getStringValue(pgroup).c_str());
			return true;
	}
}

int mh_advbase::replaceIndex()
{
	checkPopulation();
	
	int r=0;
	if (repl(pgroup)<0)
	{
		// tournament selection (with replacement of actual)
		r=pop->randomIndex();
		while (r==pop->bestIndex())
			r=pop->randomIndex();
		int k=-repl(pgroup);
		for (int i=1;i<k;i++)
		{
			int s=pop->randomIndex();
			if (r==s)
			{
				i--;
				continue;
			}
			if (pop->at(s)->isWorse(*pop->at(r)))
				r=s;
		}
	}
	else switch(repl(pgroup))
	{
	case 0:	// random
		r=pop->randomIndex();
		while (r==pop->bestIndex())
			r=pop->randomIndex();
		break;
	case 1:	// worst
		r=pop->worstIndex();
		break;
	default:
		mherror("Wrong replacement strategy",repl.getStringValue(pgroup).c_str());
		return 0;
	}
	return r;
}

mh_solution *mh_advbase::replace(mh_solution *p)
{
	checkPopulation();
	
	if (dupelim(pgroup))	// duplicate elimination
	{
		int r;
		r=pop->findDuplicate(p);
		if (r!=-1)
		{
			// replace the duplicate in the population
			nDupEliminations++;
			mh_solution *replaced=pop->replace(r,p);
			return replaced;
		}
	}

	int r=replaceIndex();
	saveBest();
	mh_solution *replaced=pop->replace(r,p);
	checkBest();
	return replaced;
}

void mh_advbase::printStatistics(ostream &ostr)
{
	checkPopulation();
	
	char s[40];
	
	double tim=CPUtime();
	const mh_solution *best=pop->bestSol();
	ostr << "# best solution:" << endl;
	sprintf( s, nformat(pgroup).c_str(), pop->bestObj() );
	ostr << "best objective value:\t" << s << endl;
	ostr << "best obtained in generation:\t" << genBest << endl;
	sprintf( s, nformat(pgroup).c_str(), timGenBest );
	ostr << "solution time for best:\t" << timGenBest << endl;
	ostr << "best chromosome:\t"; 
	best->write(ostr,0);
	ostr << endl;
	ostr << "CPU-time:\t" << tim << endl;
	ostr << "generations:\t" << nGeneration << endl;
	ostr << "subgenerations:\t" << nSubGenerations << endl;
	ostr << "selections:\t" << nSelections << endl;
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

void mh_advbase::writeLogEntry(bool inAnyCase)
{
	checkPopulation();
	
	if (logstr.startEntry(nGeneration,pop->bestObj(),inAnyCase))
	{
		logstr.write(pop->getWorst());
		logstr.write(pop->getMean());
		logstr.write(pop->getDev());
		if (logdups(pgroup)) 
			logstr.write(nDupEliminations);
		if (logcputime(pgroup))
			logstr.write(CPUtime());
		logstr.finishEntry();
	}
}

void mh_advbase::writeLogHeader()
{
	checkPopulation();
	
	logstr.headerEntry();
	logstr.write("worst");
	logstr.write("mean");
	logstr.write("dev");
	if (logdups(pgroup)) 
		logstr.write("dupelim");
	if (logcputime(pgroup))
		logstr.write("cputime");
	logstr.finishEntry();
}

void mh_advbase::checkPopulation()
{
	if (pop==NULL)
		mherror("No population set");
}

void mh_advbase::saveBest()
{
	bestObj=pop->bestObj();
}

void mh_advbase::checkBest()
{
	double nb=pop->bestObj();
	if (maxi(pgroup)?nb>bestObj:nb<bestObj)
	{
		genBest=nGeneration+1;
		timGenBest = CPUtime();
	}
}

void mh_advbase::addStatistics(const mh_advbase *a)
{
	if ( a!=NULL )
	{
		nSubGenerations    += a->nGeneration+a->nSubGenerations;
		nSelections        += a->nSelections;
		nCrossovers        += a->nCrossovers;
		nMutations         += a->nMutations;
		nCrossoverDups     += a->nCrossoverDups;
		nMutationDups      += a->nMutationDups;  
		nDupEliminations   += a->nDupEliminations;
		nLocalImprovements += a->nLocalImprovements;
		nTabus             += a->nTabus;
		nAspirations       += a->nAspirations; 
		nDeteriorations    += a->nDeteriorations;
	}
}

