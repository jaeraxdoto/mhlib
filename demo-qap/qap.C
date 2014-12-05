/*! \file qap.C 
  \brief A template main program for the QAP.

  Use this main program and the other files in this
  directory as a basis for writing your application.
  \include qap.C */

#include <iostream>
#include <cstdlib>
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
#include "mh_fdc.h"
#include "qapchrom.h"
#include "qapinstance.h"


/// Name of file to save best chromosome.
string_param sfile("sfile","name of file to save solution to","");

/** The example main function.
	It should remain small. It contains only the creation 
	of the applications top-level objects and delegates the major work to 
	other parts. It catches all exceptions and reports them as well as 
	possible. */
int main(int argc, char *argv[])
{
	/// Pointer to the current QAP instance.
	qapInstance *qi = 0;
	
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
		param::printAll(out());
		out() << endl;

		// initialize the global qapinst
		qi = qapInstance::getInstance();
		qi->initialize(qapfile());
		
		// generate a template chromosome of the problem specific qapChrom tchrom;
		qapSol tchrom;
		
		// generate a population of these chromosomes
		population p(tchrom,popsize(),true);
		// p.write(out()); 	// write out initial population
		// generate the algorithm
		mh_advbase *alg;
		alg=create_mh(p);
		alg->run();		// run algorithm until termination cond.
		
		// p.write(out());	// write out final population
		if (sfile()!="")
			p.bestSol()->save(sfile());
		alg->printStatistics(out());	// write result & statistics

		// eventually perform fitness-distance correlation analysis
		FitnessDistanceCorrelation fdc;
		fdc.perform(p.bestSol(),"");
		fdc.write(out,"fdc.tsv");
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
