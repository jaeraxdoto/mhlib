/*! \file sched.C
    \brief A template main program for the Scheduler algorithm and multithreading.  

	A  main program demonstrating the Scheduler algorithm 	
	class, which allows exploiting multithreading and presents a uniform
	framework for implementing GRASP, VNS, VLNS ans similar metaheuristics.
	This exemplary program solves the simple ONEMAX and ONEPERM problems.
	Additionally, it applies a basic test of multithreading if parameter 	
	mthreadtest ist set to 1.

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
//#include "mh_allalgs.h"

#include "mh_pop.h"
#include "mh_advbase.h"
#include "mh_log.h"
#include "mh_interfaces.h"
#include "mh_binstringsol.h"
#include "mh_permsol.h"

#include "mh_c11threads.h"
#include "mh_scheduler.h"

using namespace std;
using namespace mh;

/// Namespace for demo-sched, the demo program for using the scheduler classes.
namespace sched {

/** \ingroup param
	Number of VNS shaking neighborhoods. */
int_param prob("prob","problem to be solved (0:ONEMAX,1:ONEPERM)",0,0,1);

/** \ingroup param
	Number of variables in the ONEMAX/ONEPERM problem. 
	May be overriden by an instance file if one is specified by parameter
	ifile. */ 
int_param vars("vars","number of variables",20,1,100000);

/** \ingroup param
	Problem instance file name. If a problem instance file is given, it is
	expected to just contain the values for parameters prob and vars,
	which are overwritten. */ 
string_param ifile("ifile","problem instance file name","");

/** \ingroup param
	Name of file to save best solution. */
string_param sfile("sfile","name of file to save solution to","");

/** \ingroup param
	Number of construction heuristics. */
int_param constheus("constheus","number of construction heuristics",1,0,10000);

/** \ingroup param
	Number of VND shaking neighborhoods. */
int_param vndnhs("vndnhs","number of VND neighborhoods",0,0,10000);

/** \ingroup param
	Number of VNS shaking neighborhoods. */
int_param vnsnhs("vnsnhs","number of VNS neighborhoods",5,0,10000);

/** \ingroup param
	Number of VNS shaking neighborhoods. */
double_param methdel("methdel","delay all methods by this number of sec",0,0,100);

/** Function spending the given number of seconds by active waiting. 
	Just for testing purposes. */
void spendTime(double s=methdel()) {
	double starttime = mhcputime();
	double a;
	while (starttime + s > mhcputime()) {
		// some meaningless calculation
		a*=sin(a+0.33);
	}
}


//-- 1. Example problem: ONEMAX ------------------------------------------

/** This is the solution class for the OneMax problem.
	In larger applications, it should be implemented in a separate
	module. */
class oneMaxSol : public binStringSol //, public gcProvider
{
public:
	oneMaxSol() : binStringSol(vars())
		{}
	virtual mh_solution *createUninitialized() const
		{ return new oneMaxSol; }
	virtual mh_solution *clone() const
		{ return new oneMaxSol(*this); }
	double objective();
	double delta_obj(const nhmove &m);
	bool construct(int k) {
		spendTime();
		initialize(k); return true;
	}
	bool localimp(int k);
	bool shaking(int k);
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

double oneMaxSol::delta_obj(const nhmove &m)
{
	const bitflipMove &bfm = dynamic_cast<const bitflipMove &>(m);
	return (data[bfm.r]?-1:1);
}

bool oneMaxSol::localimp(int k)
{
	spendTime();
	// a rather meaningless demo local improvement:
	// "locally optimize" position k, i.e., set it to 1 if 0
	if (!data[k])
	{
		data[k] = 1;
		invalidate();
		return true;
	}
	return false;	// no change
}

bool oneMaxSol::shaking(int k)
{
	spendTime();
	for (int j=0; j<k; j++) {
		int i=random_int(length);
		data[i]=!data[i];
	}
	invalidate();
	// mutate(k);
	return true;
}


//-- 2. example problem: ONEPERM -----------------------------------------

/** This is the solution class for the OnePerm problem.
	In larger appications, it should be implemented in a separate
	module. */
class onePermSol : public permSol //, public gcProvider
{
public:
	onePermSol() : permSol(vars())
		{}
	virtual mh_solution *createUninitialized() const
		{ return new onePermSol; }
	virtual mh_solution *clone() const
		{ return new onePermSol(*this); }
	double objective();
	bool construct(int k) {
		spendTime();
		initialize(k); return true;
	}
	bool localimp(int k) {
		spendTime();
		mutate(k); return true;
	}
	bool shaking(int k) {
		spendTime();
		mutate(k); return true;
	}
};

/** The actual objective function counts the number of variables equal to the
 permutation 0,1,...vars()-1. */
double onePermSol::objective()
{
	// check for uninitialized solution (0,...,0) and return -1 as it
	// is not feasible in case of ONEPERM
	if (data[0]==0 && data[1]==0)
		return -1;
	int sum=0;
	for (int i=0;i<length;i++) 
		if (int(data[i])==i) 
			sum++;
	return sum;
}

//--------- Test for multithreading ---------------------------------

/** Problem specific parameters (the number of variables). */
int_param threadstest("threadstest","Test mutlithreading before starting actual application",false);

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
 * but can directly be implemented in the main function. */
template <class SolClass> void registerSchedulerMethods(GVNSScheduler *alg) {
	for (int i=1;i<=constheus();i++)
		alg->addSchedulerMethod(new SolMemberSchedulerMethod<SolClass>("conh"+tostring(i),
			&SolClass::construct,i,0));
	for (int i=0;i<vndnhs();i++)
		alg->addSchedulerMethod(new SolMemberSchedulerMethod<SolClass>("locim"+tostring(i),
			&SolClass::localimp,i,1));
	for (int i=1;i<=vnsnhs();i++)
		alg->addSchedulerMethod(new SolMemberSchedulerMethod<SolClass>("shake"+tostring(i),
			&SolClass::shaking,i,1));
}

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
		popsize.setDefault(1);
		
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

		if (threadstest())
			testmultithreading();

		if (ifile()!="") {
			// problem instance file given, read it, overwriting 
			// parameters prob and vars 
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
		mh_solution *tsol = NULL;
		switch (prob()) {
		case 0: tsol = new oneMaxSol; break;
		case 1: tsol = new onePermSol; break;
		default: mherror("Invalid problem", tostring(prob()));
		}
		// generate a population of uninitialized solutions; don't use hashing
		// be aware that the third parameter indicates that the initial solution is
		// not initialized here, i.e., it is the solution (0,0,...,0), which even
		// is invalid in case of ONEPERM; we consider this in objective().
		population p(*tsol,popsize(),false,false);
		// p.write(out()); 	// write out initial population

		// generate the Scheduler and add SchedulableMethods
		GVNSScheduler *alg;
		alg=new GVNSScheduler(p,constheus(),vndnhs(),vnsnhs());
		switch (prob()) {
		case 0: registerSchedulerMethods<oneMaxSol>(alg); break;
		case 1: registerSchedulerMethods<onePermSol>(alg); break;
		default: mherror("Invalid problem", tostring(prob()));
		}

		alg->run();		// run Scheduler until termination cond.
		
	    // p.write(out());	// write out final population
		if (sfile()!="")
			p.bestSol()->save(sfile());
		alg->printStatistics(out());	// write result & statistics

		delete alg;
		delete tsol;

		// eventually perform fitness-distance correlation analysis
		// FitnessDistanceCorrelation fdc;
		// fdc.perform(p.bestSol(),"");
		// fdc.write(out,"fdc.tsv");
	}
	// catch all exceptions and write error message
	catch (std::string &s)
	{ writeErrorMessage(s);  return 1; }
	catch (exception &e)
	{ writeErrorMessage(string("Standard exception occured: ") + e.what()); return 1; }
	catch (...)
	{ writeErrorMessage("Unknown exception occured"); return 1; }
	return 0;
}

