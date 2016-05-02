/**! \file mh_vns.C
 \include mh_vns.C */

#include <cmath>
#include "mh_subpop.h"
#include "stdio.h"
#include "mh_vnd.h"
#include "mh_vns.h"

namespace mh {

using namespace std;

int_param vnsnum("vnsnum", "maximum number of VNS neighborhood used",10000,0,10000);

int_param vnsorder("vnsorder","VNS nb-order 0:static, 1:random, 2:adaptive",0,0,2);

int_param vnsvndtiter("vnsvndtgen","tgen for VND embedded in VSN",100000,-1,100000000);

int_param vnsvndttime("vnsvndttime","ttime for VND embedded in VSN",-1);

VNS::VNS(pop_base &p, const string &pg) :
	lsbase(p,pg), vndpg(pgroupext(pgroup,"vnd"))
{
	if ( dynamic_cast<VNSProvider*>(tmpSol) == 0 )
		mherror("Chromosome is not an VNSProvider");
	
	titer.set(vnsvndtiter(),"vnd");
	ttime.set(vnsvndttime(),"vnd");
	if ( pop->size() < 1 )
		mherror("Population is too small");
	spop = new population(*pop->bestSol(), 1, false, false, vndpg);
	// start with best chromosome
	// spop->at(0)->reproduce(*pop->bestSol());
	VNSProvider *vns = dynamic_cast<VNSProvider *> (spop->at(0));
	kmax = min(vns->getVNSNNum(),vnsnum(pg));
	nborder=new NBStructureOrder(kmax,vnsorder(pg));
	k = (vndnum(vndpg)>0?0:1); // When using VND, do not shake in 1st iteration
	nShake.assign(kmax+1,0);
	nShakeSuccess.assign(kmax+1,0);
	sumShakeGain.assign(kmax+1,0.0);
	nFullIter=0;
	if (vndnum(vndpg)>0)
	{
		VNDProvider *vndsol = dynamic_cast<VNDProvider *>(tmpSol);

		if ( dynamic_cast<VNDProvider*>(tmpSol) == 0 )
			mherror("Solution is not a VNDProvider");

		int lmax = vndsol->get_lmax(vndpg);
		vndstat=new VNDStatAggregator(lmax);
		vnd_nborder=new NBStructureOrder(lmax,vndorder(vndpg));
	}
}

VNS::~VNS()
{
	delete spop;
	delete nborder;
	if (vndnum(vndpg)>0)
	{
		delete vnd_nborder;
		delete vndstat;
	}
}

void VNS::performIteration()
{
	checkPopulation();

	perfIterBeginCallback();

	VNSProvider *vns = dynamic_cast<VNSProvider *> (spop->at(0));

	if (k > kmax)
	{
		k=1;
		nFullIter++;
		nborder->calculateNewOrder();
	}

	if (k>0)
	{
		/* Shaking in neighborhood k from 2nd iteration on */
		int kidx=nborder->get(k);
		vns->shakeInVNSNeighborhood(kidx);
		nShake[kidx]++;
	}

	// apply selected local search:
	if (vndnum(vndpg)>0)
	{
		vnd_nborder->calculateNewOrder();
		VND *alg=new VND(*spop,vndpg,vnd_nborder);
		alg->run();
		addStatistics(alg);
		vndstat->add(*alg);
		delete alg;
	}

	/* Move or not */
	tmpSol->copy(*spop->at(0));
	if (pop->at(0)->isWorse(*tmpSol))
	{
		// Improved solution found
		if (k>0)
		{
			int kidx=nborder->get(k);
			nShakeSuccess[kidx]++;
			sumShakeGain[kidx]+=abs(pop->at(0)->obj()-spop->at(0)->obj());
		}
		tmpSol = replace(tmpSol);
		k = 1;
	}
	else
	{
		// copy best solution into subpopulation
		spop->at(0)->copy(*pop->bestSol());
		k++;
	}

	nIteration++;

	perfIterEndCallback();
}

bool VNS::writeLogEntry(bool inAnyCase,bool finishEntry) {
	checkPopulation();
	if (logstr.startEntry(nIteration, pop->bestObj(), inAnyCase)) {
		// 		logstr.write(pop->getWorst());
		// 		logstr.write(pop->getMean());
		// 		logstr.write(pop->getDev());
		//if (logdups(pgroup))
		//	logstr.write(nDupEliminations);
		if (ltime(pgroup))
			logstr.write(mhcputime());
		if (finishEntry)
			logstr.finishEntry();
		return true;
	}
	return false;
}

void VNS::writeLogHeader(bool finishEntry) {
	checkPopulation();

	logstr.headerEntry();
	// 	logstr.write("mean");
	// 	logstr.write("dev");
	// 	if (logdups(pgroup))
	// 		logstr.write("dupelim");
	// 	if (logcputime(pgroup))
	// 		logstr.write("cputime");
	if (finishEntry)
		logstr.finishEntry();
}

void VNS::printStatisticsShaking(ostream &ostr)
{
	ostr << endl << "VNS neighborhoods statistics:" << endl;
	int sumShakeSuccess=0,sumShake=0;
	for (int k=1;k<=kmax;k++)
	{
		sumShakeSuccess+=nShakeSuccess[k];
		sumShake+=nShake[k];
	}
	ostr << "total num of shakes:\t" << sumShake << endl;
	ostr << "total num of successful shakes:\t" << sumShakeSuccess << endl;
	ostr << "num reached kmax:\t" << nFullIter << endl;
	for (int k = 1; k <= kmax; k++) 
	{
		char tmp[200];
		snprintf(tmp,sizeof(tmp),"VNS-NH %2d: %6d success: %6d\t= %9.4f %%\tavg obj-gain: %12.5f\trel success: %9.4f %%",
		k,nShake[k],nShakeSuccess[k],
		double(nShakeSuccess[k])/double(nShake[k])*100.0,
		double(sumShakeGain[k])/double(nShake[k]),
		double(nShakeSuccess[k]/double(sumShakeSuccess)*100.0));
		ostr << tmp << endl;
	}
	ostr << endl;
}

void VNS::printStatistics(ostream &ostr)
{
	checkPopulation();
	
	char s[60];
	
	double tim=mhcputime();
	mh_solution *best=pop->bestSol();
	ostr << "# best solution:" << endl;
	snprintf( s, sizeof(s), nformat(pgroup).c_str(), pop->bestObj() );
	ostr << "best objective value:\t" << s << endl;
	ostr << "best obtained in generation:\t" << iterBest << endl;
	snprintf( s, sizeof(s), nformat(pgroup).c_str(), timIterBest );
	ostr << "solution time for best:\t" << timIterBest << endl;
	ostr << "best chromosome:\t"; 
	best->write(ostr,0);
	ostr << endl;
	ostr << "CPU-time:\t" << tim << endl;
	ostr << "generations:\t" << nIteration << endl;
	ostr << "subgenerations:\t" << nSubIterations << endl;
	ostr << "selections:\t" << nSelections << endl;
	// ostr << "crossovers:\t" << nCrossovers << endl;
	// ostr << "mutations:\t" << nMutations << endl;
	//if (cntopd(pgroup))
	//{
	//	ostr << "crossover-duplicates:\t" << nCrossoverDups << endl;
	//	ostr << "mutation-duplicates:\t" << nMutationDups << endl;
	//}
	// ostr << "local improvements:\t"  << nLocalImprovements << endl;
	//ostr << "duplicate eliminations:\t" << nDupEliminations << endl;
	//ostr << "deteriorations\t" << nDeteriorations << endl;
	//ostr << "aspirations:\t" << nAspirations << endl;
	//ostr << "tabus:\t\t" << nTabus << endl;
	if (vndnum(vndpg)>0)
		vndstat->printStatisticsVND(ostr);
	printStatisticsShaking(ostr);
}

} // end of namespace mh
