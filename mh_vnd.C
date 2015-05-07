/**! \file mh_vnd.C
 \include mh_vnd.C */

#include <stdio.h>
#include <cassert>
#include "mh_subpop.h"
#include "mh_util.h"
#include "mh_vnd.h"

bool_param vndlog("vndlog","Logging is performed in VND",false);

int_param vndnum("vndnum","Max. number of VND neighborhood to be used",10000,0,10000);

int_param vndorder("vndorder","VND nb-order 0:static, 1:random, 2:adaptive",0,0,2);


//------------------------ VNDProvider --------------------------

int VNDProvider::get_lmax(const pstring &pg)
{
	return  min(getVNDNNum(),vndnum(pg.s));
}


//--------------------------- VND -------------------------------

VND::VND(pop_base &p, const pstring &pg, NBStructureOrder *nbo) : lsbase(p,pg)
{
	VNDProvider *vndsol = dynamic_cast<VNDProvider *>(tmpSol);

	if ( dynamic_cast<VNDProvider*>(tmpSol) == 0 )
		mherror("Solution is not a VNDProvider");

	lmax = vndsol->get_lmax(pg);
	l=1;

	own_nborder=nbo?true:false;

	if (own_nborder)	// create own static neighborhood order
		nborder=new NBStructureOrder(lmax,vndorder(pg.s));
	else
		nborder=nbo;	// use provided neighborhood order object

	tciter.set(lmax, pg.s);
	nSearch.assign(lmax+1,0);
	nSearchSuccess.assign(lmax+1,0);
	sumSearchGain.assign(lmax+1,0.0);
	time.assign(lmax+1,0.0);
}

/*
We had to override the run method, because the main
loop had to be adapted for VND. We can only loop once
through all the neighborhoods without improvement.
If an improved solution is found in performGeneration,
the counter l is reset to 1 there.
*/
void VND::run(){

	checkPopulation();

	// double timStart=CPUtime();

	if(vndlog(pgroup)){
	  writeLogHeader();
	  writeLogEntry();
	  logstr.flush();
	}

	if (!terminate())
	{
		while(true)
		{
			performIteration();
			if (terminate())
			{
				// write last generation info in any case
				if(vndlog(pgroup))
					writeLogEntry(true);
				break;	// ... and stop
			}
			else
			{
				// write generation info
				if(vndlog(pgroup))
					writeLogEntry();
			}
		}
	}
	if(vndlog(pgroup))
	{
		logstr.emptyEntry();
		logstr.flush();
	}
}

void VND::performIteration(){

	checkPopulation();

	perfIterBeginCallback();
	VNDProvider *vnd = dynamic_cast<VNDProvider *>(tmpSol);

	double starttime=CPUtime();
	tmpSol->reproduce(*pop->at(0));

	/* Select neighborhood */
	int lidx=nborder->get(l);
	vnd->searchVNDNeighborhood(lidx);
	nSearch[lidx]++;
	time[lidx]+=CPUtime()-starttime;

	/* Move or not */
	if (pop->at(0)->isWorse(*tmpSol))
	{
		// Improved solution found
		nSearchSuccess[lidx]++;
		sumSearchGain[lidx]+=pop->at(0)->obj()-
			tmpSol->obj();
		tmpSol = replace(tmpSol);
		l = 1;
	}
	else 
		l++;

	nIteration++;

	perfIterEndCallback();
}

void VND::writeLogEntry(bool inAnyCase) {
	checkPopulation();
	if (logstr.startEntry(nIteration, pop->bestObj(), inAnyCase)) {
		// 		logstr.write(pop->getWorst());
		// 		logstr.write(pop->getMean());
		// 		logstr.write(pop->getDev());
		//if (logdups(pgroup))
		//	logstr.write(nDupEliminations);
		if (logcputime(pgroup))
			logstr.write(CPUtime());
		logstr.finishEntry();
	}
}

void VND::writeLogHeader() {
	checkPopulation();

	logstr.headerEntry();
	// 	logstr.write("worst");
	// 	logstr.write("mean");
	// 	logstr.write("dev");
	// 	if (logdups(pgroup))
	// 		logstr.write("dupelim");
	// 	if (logcputime(pgroup))
	// 		logstr.write("cputime");
	logstr.finishEntry();
}

void VND::printStatisticsVND(ostream &ostr)
{
	ostr << endl << "VND neighborhoods statistics:" << endl;
	int sumSearchSuccess=0,sumSearch=0,sumTime=0;
	for (int l=1;l<=lmax;l++)
	{
		sumSearchSuccess+=nSearchSuccess[l];
		sumSearch+=nSearch[l];
		sumTime+=time[l];
	}
	ostr << "total num of VND-searches:\t" << sumSearch << endl;
	ostr << "total num of successful VND-searches:\t" << sumSearchSuccess << endl;
	ostr << "total time in VND:\t" << sumTime << endl;
	for (int l = 1; l <= lmax; l++) 
	{
		char tmp[200];
		sprintf(tmp,"VND-NH %2d: %6d success: %6d\t= %9.4f %%\tavg obj-gain: %12.5f\trel success: %9.4f %%\ttime: %8.3f",
		l,nSearch[l],nSearchSuccess[l],
		double(nSearchSuccess[l])/double(nSearch[l])*100.0,
		double(sumSearchGain[l])/double(nSearch[l]),
		double(nSearchSuccess[l]/double(sumSearchSuccess)*100.0),
		time[l]);
		ostr << tmp << endl;
	}
	ostr << endl;
}

void VND::printStatistics(ostream &ostr)
{
	checkPopulation();
	
	char s[60];
	
	double tim=CPUtime();
	const mh_solution *best=pop->bestSol();
	ostr << "# best solution:" << endl;
	sprintf( s, nformat(pgroup).c_str(), pop->bestObj() );
	ostr << "best objective value:\t" << s << endl;
	ostr << "best obtained in generation:\t" << iterBest << endl;
	sprintf( s, nformat(pgroup).c_str(), timIterBest );
	ostr << "solution time for best:\t" << timIterBest << endl;
	ostr << "best chromosome:\t"; 
	best->write(ostr,0);
	ostr << endl;
	ostr << "CPU-time:\t" << tim << endl;
	ostr << "generations:\t" << nIteration << endl;
	ostr << "subgenerations:\t" << nSubIterations << endl;
	ostr << "selections:\t" << nSelections << endl;
	ostr << "crossovers:\t" << nCrossovers << endl;
	ostr << "mutations:\t" << nMutations << endl;
	//if (cntopd(pgroup))
	//{
	//	ostr << "crossover-duplicates:\t" << nCrossoverDups << endl;
	//	ostr << "mutation-duplicates:\t" << nMutationDups << endl;
	//}
	ostr << "local improvements:\t"  << nLocalImprovements << endl;
	//ostr << "duplicate eliminations:\t" << nDupEliminations << endl;
	//ostr << "deteriorations\t" << nDeteriorations << endl;
	//ostr << "aspirations:\t" << nAspirations << endl;
	//ostr << "tabus:\t\t" << nTabus << endl;
	printStatisticsVND(ostr);
}

//------------------------ VNDStatAggregator -----------------------

VNDStatAggregator::VNDStatAggregator(int _lmax)
{
	lmax=_lmax;
	vndCalls=0;
	nSearch.assign(lmax+1,0);
	nSearchSuccess.assign(lmax+1,0);
	sumSearchGain.assign(lmax+1,0.0);
	time.assign(lmax+1,0.0);
}

void VNDStatAggregator::add(const VND &vnd)
{
	assert(lmax==vnd.lmax);
	for (int l=1;l<=lmax;l++)
	{
		nSearch[l]+=vnd.nSearch[l];
		nSearchSuccess[l]+=vnd.nSearchSuccess[l];
		sumSearchGain[l]+=vnd.sumSearchGain[l];
		time[l]+=vnd.time[l];
	}
}

void VNDStatAggregator::printStatisticsVND(ostream &ostr)
{
	ostr << endl << "Aggregated VND neighborhoods statistics:" << endl;
	int sumSearchSuccess=0,sumSearch=0,sumTime=0;
	for (int l=1;l<=lmax;l++)
	{
		sumSearchSuccess+=nSearchSuccess[l];
		sumSearch+=nSearch[l];
		sumTime+=time[l];
	}
	ostr << "total num of VND-searches:\t" << sumSearch << endl;
	ostr << "total num of successful VND-searches:\t" << sumSearchSuccess << endl;
	ostr << "total time in VND:\t" << sumTime << endl;
	for (int l = 1; l <= lmax; l++) 
	{
		char tmp[200];
		sprintf(tmp,"VND-NH %2d: %6d success: %6d\t= %9.4f %%\tavg obj-gain: %12.5f\trel success: %9.4f %%\ttime: %8.3f",
		l,nSearch[l],nSearchSuccess[l],
		double(nSearchSuccess[l])/double(nSearch[l])*100.0,
		double(sumSearchGain[l])/double(nSearch[l]),
		double(nSearchSuccess[l]/double(sumSearchSuccess)*100.0),
		time[l]);
		ostr << tmp << endl;
	}
	ostr << endl;
}


//--------------------- NBStructureOrder -----------------------

void NBStructureOrder::permuteRandomly()
{
	for (int i=1;i<lmax;i++)
	{
		int j=random_int(i,lmax);
		pair<int,double> h=order[i];
		order[i]=order[j];
		order[j]=h;
	}
}

NBStructureOrder::NBStructureOrder(int _lmax,int _strategy)
	: lmax(_lmax), strategy(_strategy)
{
	order.reserve(_lmax+1);
	// initialize neighborhood order
	for (int i=0;i<=lmax;i++)
		order.push_back(pair<int,double>(i,0.0));
	if (strategy>0)
		permuteRandomly();
}

void NBStructureOrder::calculateNewOrder()
{
	switch (strategy)
	{
		case 0:	// static order
			break;
		case 1:	// random order
			permuteRandomly();
			break;
		default:
			mherror("NBStructureOrder::calculateNewOrder: strategy not implemented");
			break;
	}
}

