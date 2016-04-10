/*! \file qapfeature.h
  \brief A quadratic assignment feature class.
*/


#ifndef MH_QAPFEATURE_H
#define MH_QAPFEATURE_H

#include <vector>
#include "mh_feature.h"
#include "qapsol.h"

namespace qap {

/** A quadratic assignment feature class. */
class qapFeature : public mh::feature
{
	/// Pointer to the current QAP instance.
	qapInstance *qi;
	
protected:
	/// The penalty values.
	std::vector<double> pv;
	
public:
	/** Normal constructor.
	 \param pg Parameter group */
	qapFeature(const std::string &pg="" );
 	
	/** Function for getting the penalty.
		The penalty is computed with respect to a passed solution.
	
		\param c The solution to be penalized.
		\return The penalty.
	*/
	virtual double penalty(const mh::mh_solution *c);
	
	/** Function for getting the change in the penalty.
		The change in the objective function if a certain move is applied is computed.

		Note: This version only works if a swapMove is passed.
	
		\param c The solution to be penalized.
		\param m The move to be evaluated.
		\return The change of the penalty.
	*/
	virtual double delta_penalty(const mh::mh_solution *c, const mh::nhmove *m);
	
	/** Update penalty values.
		With respect to a given solution.
	
		\param c The solution whose features are used for the penalty update.
	*/
	virtual void updatePenalties(const mh::mh_solution *c);
	
	/** Reset penalties of all features to zero. */
	virtual void resetPenalties();
	
	/** Function to compute a tuned influence of penalties.
		The calculation is parametrized with the glsa parameter.
	
		\param c The solution 
	*/
	virtual double tuneLambda(mh::mh_solution *c);
};

} // namespace qap

#endif //MH_QAPFEATURE_H
