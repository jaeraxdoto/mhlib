/*! \file mh_feature.h
  \brief An abstract GLS feature class.
*/


#ifndef MH_FEATURE_H
#define MH_FEATURE_H

#include "mh_solution.h"
#include "mh_param.h"
#include "mh_nhmove.h"

namespace mh {

/** \ingroup param
    Penalty influence tuning parameter for GLS. */
extern double_param glsa;

/** An abstract GLS feature class. */
class feature
{
protected:
	/// Parameter group
	std::string pgroup;
	
public:
	/** The constructor. */
	feature(const std::string &pg="") : pgroup(pg) {}
	/** Virtual destructor.
	        Needed if dynamic data structures are involved. */
	virtual ~feature() {}
 	/** Function for getting the penalty.
	        The penalty is computed with respect to a passed solution. */
	virtual double penalty(const mh_solution *c) = 0;
	/** Function for getting the change in the penalty.
	        The change in the objective function if a certain move
		is applied is computed. */
	virtual double delta_penalty(const mh_solution *c, const nhmove *m) = 0;
	/** Update penalty values.
	        With respect to a given solution.	*/
	virtual void updatePenalties(const mh_solution *c) = 0;
	/** Reset penalties of all features to zero. */
	virtual void resetPenalties() = 0;
	/** Function to compute a tuned influence of penalties.
	        The calculation is parametrized with the glsa parameter. */
	virtual double tuneLambda(mh_solution *c) = 0;
};

} // end of namespace mh

#endif //MH_FEATURE_H
