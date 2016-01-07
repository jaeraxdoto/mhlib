/*! \file onemax.C
	\brief A template main program for the ONEMAX and ONEPERM problems.

	Use this main program as a simple basis for writing your application.
	\include onemax.C */

#include <cstdlib>
#include <iostream>
#include <exception>
#include "mh_allalgs.h"
#include "mh_util.h"
#include "mh_param.h"
#include "mh_random.h"
#include "mh_pop.h"
#include "mh_advbase.h"
#include "mh_island.h"
#include "mh_genea.h"
#include "mh_grasp.h"
#include "mh_guidedls.h"
#include "mh_localsearch.h"
#include "mh_simanneal.h"
#include "mh_ssea.h"
#include "mh_tabusearch.h"
#include "mh_log.h"
#include "mh_interfaces.h"
// #include "mh_fdc.h"
#include "mh_binstringsol.h"
#include "mh_permsol.h"

using namespace std;
using namespace mhlib;

/** Problem specific parameters (the number of variables). */
int_param vars("vars","number of variables",20,1,10000);

/** Name of file to save best solution. */
string_param sfile("sfile","name of file to save solution to","");

//-- 1. Example problem: ONEMAX ------------------------------------------

/** This is the solution class for the OneMax problem.
	In larger applications, it should be implemented in a separate
	module. */
class oneMaxSol : public binStringSol, public gcProvider
{
public:
	oneMaxSol() : binStringSol(vars())
		{}
	virtual mh_solution *createUninitialized() const
		{ return new oneMaxSol; }
	virtual mh_solution *clone() const
		{ return new oneMaxSol(*this); }
	double objective();
	void greedyConstruct();
	double delta_obj(const nhmove &m);
};

/// The actual objective function counts the number of variables set to 1.
double oneMaxSol::objective()
{
	int sum=0;
	for (int i=0;i<length;i++) 
		if (data[i]) 
			sum++;
	return sum;
}

/** The actual greedy construction heuristik.
        This simply sets each gene of the solution to 1. */
void oneMaxSol::greedyConstruct()
{
	for (int i=0;i<length;i++) 
		data[i] = 1;
}

double oneMaxSol::delta_obj(const nhmove &m)
{
	const bitflipMove &bfm = dynamic_cast<const bitflipMove &>(m);
	return (data[bfm.r]?-1:1);
}

//-- 2. example problem: ONEPERM -----------------------------------------

/** This is the solution class for the OnePerm problem.
	In larger appications, it should be implemented in a separate
	module. */
class onePermSol : public permSol, public gcProvider
{
public:
	onePermSol() : permSol(vars())
		{}
	virtual mh_solution *createUninitialized() const
		{ return new onePermSol; }
	virtual mh_solution *clone() const
		{ return new onePermSol(*this); }
	double objective();
	void greedyConstruct();
};

/** The actual objective function counts the number of variables equal to the
 permutation 0,1,...vars()-1. */
double onePermSol::objective()
{
	int sum=0;
	for (int i=0;i<length;i++) 
		if (int(data[i])==i) 
			sum++;
	return sum;
}

/** The actual greedy construction heuristik.
        This simply sets each gene of the solution to its index. */
void onePermSol::greedyConstruct()
{
	for (int i=0;i<length;i++) 
		data[i] = i;
}

//------------------------------------------------------------------------

/** The example main function.
	It should remain small. It contains only the creation 
	of the applications top-level objects and delegates the major work to 
	other parts. It catches all exceptions and reports them as well as 
	possible. */
int main(int argc, char *argv[])
{
	try 
	{
		// Probably set some parameters to new default values
		//pmut.setDefault(2);
		
		// parse arguments and initialize random number generator
		param::parseArgs(argc,argv);
		random_seed();

		/* initialize out() stream for standard output and logstr object for logging
		   according to set parameters. */
		initOutAndLogstr();

		out() << "#--------------------------------------------------" 
			<< endl;
		out() << "# ";
		for (int i=0;i<argc;i++)
			out() << argv[i] << ' ';
		out() << endl;
		out() << "#--------------------------------------------------" 
			<< endl;
		out () << "# " << mhversion() << endl;
		param::printAll(out());
		out() << endl;

		// generate a template solution of the problem specific class
		onePermSol tchrom;
		//oneMaxSol tchrom;

		// generate a population of these solutions
		population p(tchrom);
		// p.write(out()); 	// write out initial population
		// generate the EA
		mh_advbase *alg;
		alg=create_mh(p);
		alg->run();		// run EA until termination cond.
		
		// p.write(out());	// write out final population
		if (sfile()!="")
			p.bestSol()->save(sfile());
		alg->printStatistics(out());	// write result & statistics

		// eventually perform fitness-distance correlation analysis
		// FitnessDistanceCorrelation fdc;
		// fdc.perform(p.bestSol(),"");
		// fdc.write(out,"fdc.tsv");

		delete alg;
	}
	// catch all exceptions and write error message
	catch (std::string &s)
	{ cerr << s << endl; return 1; }
	catch (exception &e)
	{ cerr << "Standard exception occured" << e.what() << endl; return 1; }
	catch (...)
	{ cerr << "Unknown exception occured" << endl; return 1; }
	return 0;
}
