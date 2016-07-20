/*! \file maxsat.C
    \brief A demo/template application showing the usage of mhlib and in particular
    the Scheduler classes for solving the MAXSAT problem with a simple
    Generalized Variable Neighborhood Search.

	A  demo/template application showing the usage of mhlib and in particular
	the Scheduler classes for solving the MAXSAT problem with a simple
	Generalized Variable Neighborhood Search (GVNS).
	In the MAXSAT problem, a set of CNF clauses on binary variables is given, and
	we seek an assignment of values to the binary variables satisfying as many
	clauses as possible.
	The Scheduler classes are a uniform framework for implementing GRASP,
	VND, VNS, GVNS, VLNS and related (hybrid) metaheuristics, also supporting
	multithreading.

	Use this main program as a basis for writing your own application based
	on the Scheduler.
	\include maxsat.C */

#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>
#include "mh_util.h"
#include "mh_param.h"
#include "mh_random.h"
#include "mh_pop.h"
#include "mh_advbase.h"
#include "mh_log.h"
#include "mh_gvns.h"

#include "maxsat_inst.h"
#include "maxsat_sol.h"

using namespace std;
using namespace mh;

/// Namespace for maxsat, the demo application for solving the MAXSAT problem.
namespace maxsat {

/** \ingroup param
	Problem instance file name. A MAXSAT instance in DIMACS CNF format is read from
	this file. */
string_param ifile("ifile","problem instance file name","s3v70c800-1.cnf");

/** \ingroup param
	Name of file to save final solution. If empty, the final solution will
	not be saved. */
string_param sfile("sfile","name of file to save final solution to","");

/** \ingroup param
	Number of construction heuristics. This parameter is just to demonstrate
	that multiple construction heuristics can be used. */
int_param methsch("methsch","number of construction heuristics",1,0,100000);

/** \ingroup param
	Number of local improvement (VND) methods (neighborhoods). */
int_param methsli("methsli","number of local improvement methods",1,0,1000);

/** \ingroup param
	Number of shaking (VNS) methods (neighborhoods). */
int_param methssh("methssh","number of shaking methods",5,0,10000);

} // maxsat namespace


//------------------------------------------------------------------------

using namespace maxsat;

/** The example main function.
	It contains only the creation of the applications top-level objects and
	delegates the major work to	other parts. It catches all exceptions and reports them.
	Always call the given methods for initializing mhlib-parameters, the random
	number generator, out() and logstr, as otherwise many of mhlib's modules might not
	work correctly. */
int main(int argc, char *argv[])
{
	try 
	{
		// Probably set some parameters to new default values
		maxi.setDefault(1);			// we maximize here
		popsize.setDefault(1);		// other values make no sense with the Scheduler
		titer.setDefault(1000);	// the maximum number of performed iterations
		
		// parse arguments and initialize random number generator
		param::parseArgs(argc,argv);
		random_seed();

		/* initialize out() stream for standard output and logstr object for logging
		   according to set parameters. */
		initOutAndLogstr();

		// write out all mhlib parameters and the mhlib version to make runs reproducable
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

		// create and load problem instance
		MAXSATInst probinst;
		probinst.load(ifile());
		probinst.write(out());	// write out some info on instance
		out() << endl;

		// generate a population of uninitialized solutions; do not use hashing;
		// be aware that the third parameter indicates that the initial solution is
		// not initialized here, i.e., it is the solution (0,0,...,0), which even
		// is invalid in case of ONEPERM; we consider this in objective().
		population p([&probinst](){return new MAXSATSol(&probinst);}, popsize(), false, false);
		// p.write(out()); 	// write out initial population

		// create the the Scheduler and register SchedulableMethods
		GVNS *alg = new GVNS(p,methsch(),methsli(),methssh());
		/* Add construction heuristic, local improvement and shaking methods to scheduler.
		 * The following parameters are passed to the constructor of
		 * SolMemberSchedulerMethod: an abbreviated name of the method as string,
		 * the pointer to the method, a user-specific int parameter that might be used to control
		 * the method, and the arity of the method, which is either 0 in case of a method that
		 * determines a new solution from scratch or 1 in case of a method that starts from the current
		 * solution as initial solution. */
		for (int i=1;i<=methsch();i++)
				alg->addSchedulerMethod(new SolMemberSchedulerMethod<MAXSATSol>("conh"+tostring(i),
					&MAXSATSol::construct,i,0));
			for (int i=1;i<=methsli();i++)
				alg->addSchedulerMethod(new SolMemberSchedulerMethod<MAXSATSol>("locim"+tostring(i),
					&MAXSATSol::localimp,i,1));
			for (int i=1;i<=methssh();i++)
				alg->addSchedulerMethod(new SolMemberSchedulerMethod<MAXSATSol>("shake"+tostring(i),
					&MAXSATSol::shaking,i,1));

		alg->run();		// run Scheduler until a termination condition is fulfilled
		
		mh_solution *bestSol = p.bestSol();	// final solution

	    // p.write(out(),1);	// write out final population in detailed form
		if (sfile()!="")	// save best solution in file if sfile() given
			bestSol->save(sfile());

		alg->printStatistics(out());	// write result & statistics

		delete alg;
	}
	// catch all exceptions and write error message
	catch (mh_exception &e)
	{ writeErrorMessage(e.what());  return 1; }
	catch (exception &e)
	{ writeErrorMessage(string("Standard exception occurred: ") + e.what()); return 1; }
	catch (...)
	{ writeErrorMessage("Unknown exception occurred"); return 1; }
	return 0;
}

