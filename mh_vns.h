/*! \file mh_vns.h
An class for general Variable Neighborhood Search (VNS).
*/

#ifndef MH_VNS_H
#define MH_VNS_H

#include "mh_param.h"
#include "mh_pop.h"
#include "mh_lsbase.h"
#include "mh_vnd.h"

/** \ingroup param
 	Maximum value of used shaking NHs. */
extern int_param vnsnum;

/** \ingroup param
    VNS neighborhood ordering:
    - 0: static
    - 1: random
    - 2: adaptive */
extern int_param vnsorder;

/** \ingroup param
    Parameter tgen for embedded VND
 */
extern int_param vnsvndtgen;

/** \ingroup param
    Parameter ttime for embedded VND
 */
extern int_param vnsvndttime;

/** An abstract interface class for chromosomes
    used in a VNS heuristic. */
class VNSProvider
{
public:
	/// Virtual destructor.
	virtual ~VNSProvider() {};
	
	/** Performs shaking in neighborhood k (in 1..getVNSNNum)
	    of the current solution. */
	virtual void shakeInVNSNeighborhood(int l) = 0;

	/// Returns the number of neighbrhood structures
	virtual int getVNSNNum() = 0;
};


/** 
The VNS base algorithm. For local search it calls VND or another sub algorithm.
The neighborhoodsize is increased if shaking and the subsequent local search
do not find an improved solution.
The chromosome for this algorithm should implement the vnsProvider interface. 
*/
class VNS : public lsbase
{
protected:
	/// The subpopulation for the inner local improvement algorithm.
	pop_base *spop;
	/// current neighborhood.
	int k;		///< current neighborhood
	int kmax;	///< total number of neighborhoods
	int nFullIter;	///< counter for full VNS iterations
	vector<int> nShake;		///< number of shaking calls
	vector<int> nShakeSuccess;	///< number of successful shakes
	vector<double> sumShakeGain;	///< total gain achieved
	NBStructureOrder *nborder; ///< Order of neighborhood structures
	// for possibly embedded VND:
	pstring vndpg;	///< parameter group for VND
	VNDStatAggregator *vndstat;	///< aggegator for VND statistics
	NBStructureOrder *vnd_nborder; ///< VND neighborhood order
public:
	/** The constructor.
		An initialized population already containing chromosomes 
		must be given. Note that the population is NOT owned by the 
		algorithm and will not be deleted by its destructor. VNS
		will only use the first two chromosomes. */
	VNS(pop_base &p, const pstring &pg = (pstring) (""));
	/** Destructor. */
	virtual ~VNS();
	/** Performs a single generation, is called from run() */
	virtual void performGeneration();

	/** Write only meaningful information into log. */
	virtual void writeLogHeader();
	/** Write only meaningful information into log. */
	virtual void writeLogEntry(bool inAnyCase = false);
	/** Write detailed statistics on shaking neighborhoods. */
	void printStatisticsShaking(ostream &ostr);
	/** General print Statistics method extended. */
	void printStatistics(ostream &ostr);
};

#endif // VNS_H
