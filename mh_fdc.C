// mh_fdc.C

#include <fstream>
#include <cmath>
#include "mh_fdc.h"
#include "mh_util.h"

namespace mh {

using namespace std;

int_param fdcn("fdcn","number of samples for FDC analysis",0,0,10000000);

string_param fdcfile("fdcfile","name of FDC output file","fdc.tsv");

string_param fdcoptf("fdcoptf","name of file for opt. solution for FDC",
	"opt.sol");

double FitnessDistanceCorrelation::perform(mh_solution *opt,
	const string &optfile, int n)
{
	if (n<=0)
	{ 
		corr=9999;
		return corr;
	}
	vals.resize(n);

	if (optfile!="")
		opt->load(optfile);
	vals[0].f=opt->obj();
	vals[0].d=opt->dist(*opt);

	mh_solution *c,*cl;
	c=opt->createUninitialized();
	cl=opt->createUninitialized();
	initialize(cl);

	double sumdistbetween=0;

	for (int i=1;i<n;i++)
	{
		initialize(c);
		vals[i].f=c->obj();
		vals[i].d=c->dist(*opt);
		sumdistbetween+=c->dist(*cl);
		cl->copy(*c);
	}
	
	// actually calculate average values and correlation coefficient
	distbetween=sumdistbetween/n;
	favg=davg=0;
	for (int i=0;i<n;i++)
	{
		favg+=vals[i].f;
		davg+=vals[i].d;
	}
	favg/=n;
	davg/=n;

	double sf=0,sd=0,sfd=0;
	for (int i=0;i<n;i++)
	{
		double df=vals[i].f-favg;
		sf+=df*df;
		double dd=vals[i].d-davg;
		sd+=dd*dd;
		sfd+=df*dd;
	}
	corr=sfd/sqrt(sf*sd);

	return corr;
}

void FitnessDistanceCorrelation::write(outStream &out, const string &fname)
{
	if (vals.size()<=0)
		return;
	out() << "FDC - correlation:\t" << corr << endl;
	out() << "FDC - avg fitness:\t" << favg << endl;
	out() << "FDC - avg dist to optimum:\t" << davg << endl;
	out() << "FDC - avg dist between:\t" << distbetween << endl;

	if (fname!="")
	{
		ofstream of(fname.c_str());
		int n=vals.size();
		for (int i=0;i<n;i++)
		{
			of << vals[i].f << '\t' << vals[i].d << endl;
		}
	}
}

} // end of namespace mh

