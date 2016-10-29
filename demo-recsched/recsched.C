/*! \file recsched.C
	\brief This program demonstrates the recursive use of Scheduler objects, i.e., one Scheduler
	is used within another for augmenting, repairing, locally improving, or evaluating candidate solutions.

	In this demo, ONEMAX is solved for every new candidate solution of ONEPERM, which does not
	really make much sense. However, it shows how subproblems can be solved independently
	in order to augment, repair, evaluate, or locally improve candidate solutions of an outer Scheduler.
	\include recsched.C */

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
#include "mh_binstringsol.h"
#include "mh_permsol.h"
#include "mh_gvns.h"
#include "mh_c11threads.h"

using namespace std;
using namespace mh;

/// Namespace for sched, the demo program for using the scheduler classes.
namespace sched {

/** \ingroup param
	Number of variables in the ONEMAX/ONEPERM problem, i.e., the length of the
	solution string. May be overriden by an instance file if one is specified
	by parameter ifile. */
int_param vars("vars","number of variables",20,1,100000);

/** \ingroup param
	Problem instance file name. If a problem instance file is given, it is
	expected to just contain the values for parameters prob and vars,
	which are overwritten. Obviously, this is just a simple demo for how to
	deal with instance files. */
string_param ifile("ifile","problem instance file name","");

/** \ingroup param
	Name of file to save best solution. If empty, the final solution will
	not be saved. */
string_param sfile("sfile","name of file to save solution to","");

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

/** \ingroup param
	A value in seconds by which each method is delayed by active waiting
	for testing multithreading. */
double_param methdel("methdel","delay all methods by this number of sec",0,0,100);


//-- Embedded problem: ONEMAX ------------------------------------------

/** parameter group for the embedded scheduler solving OneMax. */
const string ONEMAX_PG="onemax";

/** This is the solution class for the ONEMAX problem (finding binary string
    (1,...,1)).	In real applications, it should be implemented in a separate
	module. */
class oneMaxSol : public binStringSol
{
public:
	/** The default constructor. Here it passes the string length vars(ONEMAX_PG) to
	 * the parent class constructor.
	 */
	oneMaxSol() : binStringSol(vars(ONEMAX_PG))
		{}
	/** Create a new uninitialized instance of this class. */
	virtual mh_solution *createUninitialized() const override
		{ return new oneMaxSol; }
	/** Clone this solution, i.e., return a new copy. */
	virtual mh_solution *clone() const override
		{ return new oneMaxSol(*this); }
	/** Determine the objective value of the solution. In this example
	 * we count the 1s in the solution string.
	 */
	double objective() override;
	/** A simple construction heuristic, just calling the base class' initialize
	 * function, initializing each bit randomly.
	 */
	void construct(int k, SchedulerMethodContext &context, SchedulerMethodResult &result) {
		initialize(k);
		// Values in result are kept at default, i.e., are automatically determined
	}
	/** A simple local improvement function: Locally optimize position k,
	 * i.e., set it to 1 if 0.
	 */
	void localimp(int k, SchedulerMethodContext &context, SchedulerMethodResult &result);
	/** A simple shaking function: Invert k randomly chosen positions. */
	void shaking(int k, SchedulerMethodContext &context, SchedulerMethodResult &result);
};

double oneMaxSol::objective()
{
	int sum=0;
	for (int i=0;i<length;i++) 
		if (data[i]) 
			sum++;
	return sum;
}

void oneMaxSol::localimp(int k, SchedulerMethodContext &context, SchedulerMethodResult &result)
{
	// call embedded scheduler:


	if (!data[k])
	{
		data[k] = 1;
		invalidate();
		// Values in result are kept at default, i.e., are automatically determined
		return;
	}
	result.changed = false; // solution not changed
}

void oneMaxSol::shaking(int k, SchedulerMethodContext &context, SchedulerMethodResult &result)
{
	for (int j=0; j<k; j++) {
		int i=random_int(length);
		data[i]=!data[i];
	}
	invalidate();
	// mutate(k);
	// Values in result are kept at default, i.e., are automatically determined
}

/** Pointer to GVNS for solving the embedded problem.
 * Required so that the OnePerm Methods can call it.
 * May be more cleanly embedded somewhere in a real application.
 */
GVNS *algOneMax = nullptr;


//-- Outer problem: ONEPERM -----------------------------------------

/** This is the solution class for the ONEPERM problem (find the
 * permutation (0,1,2,3,...,vars()-1).
	In real applications, it should be implemented in a separate
	module. */
class onePermSol : public permSol
{
public:
	/** The default constructor. Here it passes the string length vars() to
	 * the parent class constructor.
	 */
	onePermSol() : permSol(vars())
		{}
	/** Create a new uninitialized instance of this class. */
	virtual mh_solution *createUninitialized() const override
		{ return new onePermSol; }
	/** Clone this solution, i.e., return a new copy. */
	virtual mh_solution *clone() const override
		{ return new onePermSol(*this); }
	/** Determine the objective value of the solution. In this example
	 * we count the the number of values that are on the same place as in the
	 * target permutation (0,1,2,...,vars()-1). Should the solution be uninitialized,
	 * in which case all variables have value 0, return value -1.
	 */
	double objective() override;
	/** A simple local improvement function: Locally optimize position k,
	 * i.e., set it to 1 if 0.
	 */
	void construct(int k, SchedulerMethodContext &context, SchedulerMethodResult &result) {
		initialize(k);
		// Values in result are kept at default, i.e., are automatically determined
	}
	/** A simple local improvement function: Here we just call the
	 * mutate function from the base class.
	 */
	void localimp(int k, SchedulerMethodContext &context, SchedulerMethodResult &result) {
		// Call the embedded Scheduler:
		algOneMax->reset();
		// create new initial solution
		mh_solution *s = algOneMax->pop->at(0);
		s->initialize(0);
		algOneMax->pop->update(0,s);
		algOneMax->run();
		// out() << "Obj before localimp = " << obj() << endl;
		mutate(k);
		// Values in result are kept at default, i.e., are automatically determined
	}
	/** A simple shaking function: Here we just call the
	 * mutate function from the base class.
	 */
	void shaking(int k, SchedulerMethodContext &context, SchedulerMethodResult &result) {
		/** A simple shaking function: Here we just call the
		 * mutate function from the base class. */
		// out() << "Obj before shaking = " << obj() << endl;
		mutate(k);
		// Values in result are kept at default, i.e., are automatically determined
	}
};

double onePermSol::objective()
{
	// check for uninitialized solution (0,...,0) and return -1 as it
	// is not feasible in case of ONEPERM
	if (data[0]==0 && data[1]==0)
		return -1;
	// count the number of correctly placed values
	int sum=0;
	for (int i=0;i<length;i++)
		if (int(data[i])==i)
			sum++;
	return sum;
}

} // sched namespace

//------------------------------------------------------------------------

using namespace sched;

/** Template function for registering the problem-specific scheduler methods in the algorithm.
 * When considering just one kind of problem, this does not need to be a template function
 * but can directly be implemented in the main function. The following parameters are passed to
 * the constructor of SolMemberSchedulerMethod: an abbreviated name of the method as string,
 * the pointer to the method, a user-specific int parameter that might be used to control
 * the method, and the arity of the method, which is either 0 in case of a method that
 * determines a new solution from scratch or 1 in case of a method that starts from the current
 * solution as initial solution. */
template <class SolClass> void registerSchedulerMethods(GVNS *alg, const string &prefix="") {
	for (int i=1;i<=methsch();i++)
		alg->addSchedulerMethod(new SolMemberSchedulerMethod<SolClass>(prefix+"conh"+tostring(i),
			&SolClass::construct,i,0));
	for (int i=1;i<=methsli();i++)
		alg->addSchedulerMethod(new SolMemberSchedulerMethod<SolClass>(prefix+"locim"+tostring(i),
			&SolClass::localimp,i,1));
	for (int i=1;i<=methssh();i++)
		alg->addSchedulerMethod(new SolMemberSchedulerMethod<SolClass>(prefix+"shake"+tostring(i),
			&SolClass::shaking,i,1));
}

/** The example main function.
	It should remain small. It contains only the creation 
	of the applications top-level objects and delegates the major work to 
	other parts. It catches all exceptions and reports them as well as 
	possible. Always call the given methods for initializing mhlib-parameters, the random
	number generator, out() and logstr, as otherwise many of mhlib's modules might not
	work correctly. */
int main(int argc, char *argv[])
{
	try 
	{
		// Probably set some parameters to new default values
		// main algorithm for OnePerm:
		maxi.setDefault(1);
		popsize.setDefault(1);
		titer.setDefault(1000);
		// embedded algorithm for OneMax:
		schthreads.set(1, ONEMAX_PG);
		schsync.set(false, ONEMAX_PG);
		tciter.set(-1, ONEMAX_PG);
		titer.set(200, ONEMAX_PG);
		
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

		if (ifile()!="") {
			// problem instance file given, read it, overwriting
			// parameters vars in this simple example
			ifstream is(ifile());
			if (!is)
				mherror("Cannot open problem instance file", ifile());
			int v=0;
			is >> v;
			if (!is)
				mherror("Invalid problem instance file", ifile());
			vars.set(v);
		}

		// generate template solutions of the problem specific classes
		std::function<mh_solution *()> createOnePermSol = [](){return new onePermSol();};

		// generate populations of uninitialized solutions; do not use hashing
		// be aware that the third parameter indicates that the initial solution is
		// not initialized here, i.e., it is the solution (0,0,...,0), which even
		// is invalid in case of ONEPERM; we consider this in objective().
		population pOnePerm(createOnePermSol, popsize(), false, false);
		// p.write(out()); 	// write out initial population

		// generate the Schedulers and add SchedulableMethods
		GVNS *algOnePerm = new GVNS(pOnePerm,methsch(),methsli(),methssh());
		registerSchedulerMethods<onePermSol>(algOnePerm);

		// also create a template solution, population and the Scheduler object for Onemax
		std::function<mh_solution *()> createOneMaxSol = [](){return new oneMaxSol();};
		population pOneMax(createOneMaxSol, popsize(), false, false);
		algOneMax = new GVNS(pOneMax,methsch(),methsli(),methssh(),ONEMAX_PG);
		registerSchedulerMethods<oneMaxSol>(algOneMax,"om-");

		algOnePerm->run();		// run outer Scheduler until a termination condition is fulfilled
		
		mh_solution *bestSol = pOnePerm.bestSol();	// final solution

	    // pOnePerm.write(out(),1);	// write out final population in detailed form
		if (sfile()!="")	// save best solution in file if sfile() given
			bestSol->save(sfile());

		// write result & statistics
		algOneMax->printStatistics(out());
		algOnePerm->printStatistics(out());

		delete algOnePerm;
		delete algOneMax;

		// eventually perform fitness-distance correlation analysis
		// FitnessDistanceCorrelation fdc;
		// fdc.perform(p.bestSol(),"");
		// fdc.write(out,"fdc.tsv");
	}
	// catch all exceptions and write error message
	catch (mh_exception &s)
	{ writeErrorMessage(s.what());  return 1; }
	catch (exception &e)
	{ writeErrorMessage(string("Standard exception occurred: ") + e.what()); return 1; }
	catch (...)
	{ writeErrorMessage("Unknown exception occurred"); return 1; }
	return 0;
}

