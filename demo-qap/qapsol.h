/*! \file qapsol.h
  \brief A quadratic assignment problem solution.
*/

#ifndef MH_QAPCHROM_H
#define MH_QAPCHROM_H

#include <vector>
#include "mh_solution.h"
#include "mh_feature.h"
#include "mh_interfaces.h"
#include "mh_random.h"
#include "qapinstance.h"
#include "qaptabuattribute.h"

namespace qap {

/** \ingroup param
    Alpha parameter for GRASP.

    Used for candidate restriction.
 */
extern mh::double_param graspa;

/** \ingroup param
    Beta parameter for GRASP.

    Used for candidate restriction.
 */
extern mh::double_param graspb;


/** A concrete solution class for the quadratic assignment problem. */
class qapSol : public mh::mh_solution, public mh::featureProvider, public mh::tabuProvider, public mh::gcProvider
{
	friend class qapFeature;
	
	/// Pointer to the current QAP instance.
	qapInstance *qi;
	
protected:
	/// Actual gene string.
	std::vector<int> data;

	/** Dynamically cast a solution reference to a qapSol reference.
		If the original object was of the correct type.
	
		\param ref Object to dynamically cast
	*/
	static const qapSol &cast(const mh::mh_solution &ref)
		{ return (dynamic_cast<const qapSol &>(ref)); }
public:
	mh::mh_solution *createUninitialized() const
		{ return new qapSol(alg, pgroup); }
	mh::mh_solution *clone() const
		{ return new qapSol(*this); }
	double objective();

	/** Copy constructor.
		\param c Object to copy from.
	*/
	qapSol(const mh::mh_solution &c);
	
	/** Normal constructor, number of genes must be passed to base class.
		\param t Associated algorithm object
		\param pg Parameter group
	*/
	qapSol(mh::mh_base* t, const std::string &pg="") : mh::mh_solution(qapInstance::getInstance()->n, t, pg), qi(qapInstance::getInstance()), data(length)
		{}
	
	/** Normal constructor, number of genes must be passed to base class.
		\param pg Parameter group
	*/
	qapSol(const std::string &pg="") : mh::mh_solution(qapInstance::getInstance()->n,pg), qi(qapInstance::getInstance()),  data(length)
		{}
	
	/** Copy all data from a given solution into the current one.
		\param orig Object to copy from.
	*/
	void copy(const mh::mh_solution &orig);
	
	/** Return true if the current solution is equal to *orig.
		\param orig Object to compare to.
	*/
	bool equals(mh::mh_solution &orig);
	
	/** Returns the hamming distance.
		\param c Solution to compute distance to.
	*/
	double dist(mh::mh_solution &c);
	
	/** Randomly initializes with an uniformly generated permutation.
		\param count Does not have any effect.
	*/
	void initialize(int count);
	
	/** Swap locations of two facilities.
		\param count Number of mutations.
	*/
	void mutate(int count);
	
	/** Cycle crossover.
		\param parA Parent A for crossover.
		\param parB Parent B for crossover.
	*/
	void crossover(const mh::mh_solution &parA,const mh::mh_solution &parB);
	
	/** Writes the solution to an ostream.
		The values of the permutation are incremented by one for better human readability.
		The detailed parameter is ignored.
	
		\param ostr Stream to use.
		\param detailed Does not have any effect.
	*/
	void write(std::ostream &ostr,int detailed=0) const;
	
	/** Saves a solution to a file.
		The values of the permutation are incremented by
		one for better human readability.
	
		\param fname Filename for the solution to save..
	*/
	void save(const std::string &fname);
	
	/** Loads a solution from a file.
		\param fname Filename of the solution to load.
	*/
	void load(const std::string &fname);
	
	/** Calculates a hash-value out of the permutation. */
	unsigned long int hashvalue();
	
	/** Function for getting the change in the objective function.
	        The change in the objective function if a certain move
		is applied is computed.

		Note: This version only works if a swapMove is passed.
	
		\param m The move to be evaluated.
	*/
	double delta_obj(const mh::nhmove &m);
	
	/** Function to apply a certain move.
	    Note: This version only works if a swapMove is passed.
	
		\param m The move to be applied.
	*/
	void applyMove(const mh::nhmove &m);
	
	/** Replace current solution with a better or even the best neighbour.
		\param find_best If true, the best solution in the neighbourhood is searched,
			if it is false the next improvement is selected (if one exists).
	*/
	void selectImprovement(bool find_best);
	
	/** Getter method for the associated feature object. */
	mh::feature* getFeature();

	/** Greedy construction heuristic. */
	void greedyConstruct();
};

} // namespace qap

#endif //MH_QAPCHROM_H
