/*! \file sched.C
    \brief A testbed/demo main program for the Scheduler algorithm including a test for multithreading.

	A  demo/template main program demonstrating the Scheduler algorithm
	class, which allows exploiting multithreading and presents a uniform
	framework for implementing GRASP, VNS, VLNS and similar metaheuristics.
	This exemplary program solves the simple ONEMAX problem (find binary string (1,1,...,1)
	and ONEPERM problem (find permutation (0,1,2,3,...vars()-1) in
	very trivial, demonstrative ways.
	Additionally, it applies a basic test of multithreading if parameter 	
	threadstest is set to 1.

	Use this main program as a basis for writing your own application based
	on the Scheduler.
	\include sched.C */

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
#include "mh_pbig.h"

using namespace std;
using namespace mh;

/// Namespace for sched, the demo program for using the scheduler classes.
namespace sched {

/** \ingroup param
	The problem to be solved, which is either the ONEMAX problem or
	the ONEPERM problem. */
int_param prob("prob","problem to be solved 0:ONEMAX,1:ONEPERM",0,0,1);

/** \ingroup param
	Number of variables in the ONEMAX/ONEPERM problem, i.e., the length of the
	solution string. May be overriden by an instance file if one is specified
	by parameter #ifile. */
int_param vars("vars","number of variables",20,1,100000);

/** \ingroup param
	Problem instance file name. If a problem instance file is given, it is
	expected to just contain the values for parameters #prob and #vars,
	which are overwritten. Obviously, this is just a simple demo for how to
	deal with instance files. */
string_param ifile("ifile","problem instance file name","");

/** \ingroup param
	Number of construction heuristics. If set to the default value of -1, the number of used
	threads #schthreads will be used. This parameter is just to demonstrate
	that multiple construction heuristics can be used. */
int_param methsch("methsch","number of construction heuristics",-1,-1,100000);

/** \ingroup param
	Number of local improvement (VND) methods (neighborhoods). */
int_param methsli("methsli","number of local improvement methods",1,0,1000);

/** \ingroup param
	Number of shaking (VNS) methods (neighborhoods). */
int_param methssh("methssh","number of shaking methods",5,0,10000);

/** \ingroup param
	A value in seconds (wall clock time) by which each method is delayed by active waiting
	for testing multithreading. */
double_param methdel("methdel","delay all methods by this number of sec",0,0,100);

/** \ingroup param
 	Scheduler class to use. */
 int_param schedalg("schedalg","scheduler algorithm to use: 0:basic, 1:GVNS, 2:PBIG",1,0,2);


/* Just spending some time, used by spendTime. */
static void spend() {
		volatile double a;
		// some meaningless calculation
		for (volatile int i=0;i<10000;i++)
			a*=sin(a+0.33);
	}

/** Function spending approximately the given number of CPU seconds by active waiting.
	Just for testing purposes of the multithreading. The function needs to be called once
	with a negative parameter value at the beginning for calibrating the delay loop. */
void spendTime(double s=methdel()) {
	static int iters=0;	// number of iterations for approximately requiring one second
	double starttime = mhcputime();
	if (s<0) {
		iters = 0;
		// do calibration of iters
		while (starttime + 1 > mhcputime()) {
			spend();
			iters++;
		}
		return;
	}
	for (int i=0;i<iters*s;i++)
		spend();
}


//-- 1. Example problem: ONEMAX ------------------------------------------

/** This is the solution class for the ONEMAX problem (finding binary string
    (1,...,1)).	In real applications, it should be implemented in a separate
	module. */
class oneMaxSol : public binStringSol
{
public:
	/** The default constructor. Here it passes the string length vars() to
	 * the parent class constructor.
	 */
	oneMaxSol() : binStringSol(vars())
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
		spendTime();
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
	spendTime();
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
	spendTime();
	for (int j=0; j<k; j++) {
		int i=random_int(length);
		data[i]=!data[i];
	}
	invalidate();
	// mutate(k);
	// Values in result are kept at default, i.e., are automatically determined
}


//-- 2. example problem: ONEPERM -----------------------------------------

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
		spendTime();
		initialize(k);
		// Values in result are kept at default, i.e., are automatically determined
	}
	/** A simple local improvement function: Here we just call the
	 * mutate function from the base class.
	 */
	void localimp(int k, SchedulerMethodContext &context, SchedulerMethodResult &result) {
		// out() << "Obj before localimp = " << obj() << endl;
		spendTime();
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
		spendTime();
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

//--------- Test for multithreading ---------------------------------

/** Problem specific parameters (the number of variables). */
int_param threadstest("threadstest","test mutlithreading before starting actual application",false);

std::mutex mymutex;

/** A test thread doing some meaningless work and writing out the given parameter t. */
static void mythread(int t)
{
	for (int i=1;i<20;i++)
	{
		double a = 1;
		for (int j=1;j<3879999;j++)
			a*=sin(a+0.33);
    	//mymutex.lock();
		cout << t; cout.flush();
		//mymutex.unlock();
	}
}

/** A simple test for multithreading, doing some meaningless work in sequential and
 * then parallel fashion and writing out the needed CPU-times. */
static void testmultithreading()
{
		cerr << "Time: " << mhcputime() << endl;
		cout << "Test multithreading, available hardware threads: " << 
			thread::hardware_concurrency() << " " << endl;

		cerr << "Time: " << mhcputime() << endl;
		cout << "Sequential execution:" << endl;
		mythread(1);
		mythread(2);
		mythread(3);
		mythread(4);
		cout << endl << "Sequential execution finished" << endl;
		cerr << "Time: " << mhcputime() << endl;
		cout << "Parallel execution:" << endl;
		std::thread t1(mythread,1);
		std::thread t2(mythread,2);
		std::thread t3(mythread,3);
		std::thread t4(mythread,4);
		t1.join();
		t2.join();
		t3.join();
		t4.join();
		cout << endl << "All threads finished" << endl << endl;
		cerr << "Time: " << mhcputime() << endl;
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
template <class SolClass> void registerSchedulerMethods(Scheduler *alg) {
	for (int i=1;i<=methsch();i++)
		alg->addSchedulerMethod(new SolMemberSchedulerMethod<SolClass>("conh"+tostring(i),
			&SolClass::construct,i,0));
	for (int i=1;i<=methsli();i++)
		alg->addSchedulerMethod(new SolMemberSchedulerMethod<SolClass>("locim"+tostring(i),
			&SolClass::localimp,i,1));
	for (int i=1;i<=methssh();i++)
		alg->addSchedulerMethod(new SolMemberSchedulerMethod<SolClass>("shake"+tostring(i),
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
		// Set some parameters to new default values
		maxi.setDefault(1);
		popsize.setDefault(1);
		titer.setDefault(1000);
		
		// parse arguments and initialize random number generator
		param::parseArgs(argc,argv);
		random_seed();

		if (methdel()!=0)
			spendTime(-1);	// calibrate spendTime function

		/* if #methsch, the number of construction heuristics to be used,
		   is -1, then set it to the number of used threads */
		if (methsch()==-1)
			methsch.set(schthreads());

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

		// possible testing of the multithreading
		if (threadstest())
			testmultithreading();

		if (ifile()!="") {
			// problem instance file given, read it, overwriting 
			// parameters #prob and #vars in this simple example
			ifstream is(ifile());
			if (!is)
				mherror("Cannot open problem instance file", ifile());
			int p=0,v=0;
			is >> p >> v;
			if (!is)
				mherror("Invalid problem instance file", ifile());
			prob.set(p);
			vars.set(v);
		}

		// generate a template solution of the problem specific class
		std::function<mh_solution *()> createsol;
		switch (prob()) {
		case 0: createsol = [](){return new oneMaxSol();}; break;
		case 1: createsol = [](){return new onePermSol();}; break;
		default: mherror("Invalid problem", tostring(prob()));
		}
		// generate a population of uninitialized solutions; do not use hashing
		// be aware that the third parameter indicates that the initial solution is
		// not initialized here, i.e., it is the solution (0,0,...,0), which even
		// is invalid in case of ONEPERM; we consider this in objective().
		population p(createsol, popsize(), false, false);
		// p.write(out()); 	// write out initial population

		// generate the Scheduler and add SchedulableMethods
		Scheduler *alg = nullptr;
		switch (schedalg()) {
		case 0: alg = new Scheduler(p); break;
		case 1: alg = new GVNS(p,methsch(),methsli(),methssh()); break;
		case 2: alg = new PBIG(p,methsch()+methsli()+methssh()-1); break;
		default: mherror("Invalid scheduler algorithm selected",tostring(schedalg()));
		}
		switch (prob()) {
		case 0: registerSchedulerMethods<oneMaxSol>(alg); break;
		case 1: registerSchedulerMethods<onePermSol>(alg); break;
		default: mherror("Invalid problem", tostring(prob()));
		}

		alg->run();		// run Scheduler until a termination condition is fulfilled
		
		mh_solution *bestSol = p.bestSol();	// final solution

	    // p.write(out(),1);	// write out final population in detailed form
		// save best solution in file if oname()!="@"
		bestSol->save(outStream::getFileName(".sol","NULL"));

		alg->printStatistics(out());	// write result & statistics

		delete alg;

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

