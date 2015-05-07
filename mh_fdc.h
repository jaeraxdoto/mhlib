/** \file mh_fdc.h
	\brief A class for performing a fitness-distance correlation analysis. 
	
	To use it, create an object and call method perform(); write()
	writes the results. */

#ifndef MH_FDC_H
#define MH_FDC_H

#include <string>
#include <vector>
#include "mh_solution.h"
#include "mh_log.h"
#include "mh_param.h"

/** \ingroup param
	The number of solutions that are created for the fitness-distance
	correlation analysis. 0 means no FDC should be performed. */
extern int_param fdcn;

/** \ingroup param
	The name of the file to which fitness-distance data are written out. */
extern string_param fdcfile;

/** \ingroup param
	The name of the file from which the optimum solution is eventually read
	for the fitness-distance correlation analysis. */
extern string_param fdcoptf;

/** A class for performing a fitness-distance correlation analysis. */
class FitnessDistanceCorrelation
{
protected:
	/// Parametergroup
	string pgroup;
	
public:
	struct FitDist
	{
		double f, d;
	};
	/** Fitness/distance values. The entry with index 0 corresponds to
		the given optimum. */
	vector<FitDist> vals;

	// values determined by perform():
	double favg;	///< average value of fitness values
	double davg;	///< average value of distances to optimum
	double corr;	///< correlation coefficient	
	double distbetween;	///< average distance between samples

	/// The constructor
	FitnessDistanceCorrelation( const pstring &pg=(pstring)("") ) : pgroup(pg.s) {};
	
	/** Performs fitness-distance correlation analysis. Creates n
		 random solutions via initialize and stores their objective
		 values and distances to the given optimum solution in
		 vals. If optfile is not NULL, the optimum solution is read
		 in from the file with the given name. Returns the
		 correlation coefficient and stores it in corr. */ 
	double perform(mh_solution *opt, const string &optfile, int n);
	/** Performs fitness-distance correlation analysis. Creates n
		 random solutions via initialize and stores their objective
		 values and distances to the given optimum solution in
		 vals. If optfile is not NULL, the optimum solution is read
		 in from the file with the given name. Returns the
		 correlation coefficient and stores it in corr. */ 
	double perform(mh_solution *opt, const string &optfile)
		{ return perform( opt, optfile, fdcn(pgroup) ); }
	/** Performs fitness-distance correlation analysis. Creates n
		 random solutions via initialize and stores their objective
		 values and distances to the given optimum solution in
		 vals. If optfile is not NULL, the optimum solution is read
		 in from the file with the given name. Returns the
		 correlation coefficient and stores it in corr. */ 
	double perform(mh_solution *opt)
		{ return perform( opt, fdcoptf(pgroup), fdcn(pgroup) ); }
	
	/** Writes out a message with the correlation coefficient of the
		last performed FDC and the fitness/distance-pairs to a file
		with the given name (if not NULL). */
	void write(outStream &out, const string &fname);
	
	/** Writes out a message with the correlation coefficient of the
		last performed FDC and the fitness/distance-pairs to a file
		with the given name (if not NULL). */
	void write(outStream &out)
		{ write( out, fdcfile(pgroup) ); }
	
	/** Creates one random solution. */
	virtual void initialize(mh_solution *c) 
		{ c->initialize(0); }

	virtual ~FitnessDistanceCorrelation() {}
};

#endif // MH_FDC_H

