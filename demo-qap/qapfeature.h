/*! \file qapfeature.h
  \brief A quadratic assignment feature class.
*/


#ifndef MH_QAPFEATURE_H
#define MH_QAPFEATURE_H

#include <vector>
#include "mh_feature.h"
#include "qapchrom.h"

/** A quadratic assignment feature class. */
class qapFeature : public feature
{
	/// Pointer to the current QAP instance.
	qapInstance *qi;
	
protected:
	/// The penalty values.
	vector<double> pv;
	
public:
	/** Normal constructor.
	 \param pg Parametergroup */
	qapFeature(const pstring &pg=(pstring)("") );
 	
	/** Function for getting the penalty.
		The penalty is computed with respect to a passed chromosome.
	
		\param c The chromosome to be penalized.
		\return The penalty.
	*/
	virtual double penalty(const mh_solution *c);
	
	/** Function for getting the change in the penalty.
		The change in the objective function if a certain move is applied is computed.

		Note: This version only works if a swapMove is passed.
	
		\param c The chromosome to be penalized.
		\param m The move to be evaluated.
		\return The change of the penalty.
	*/
	virtual double delta_penalty(const mh_solution *c, const move *m);
	
	/** Update penalty values.
		With respect to a given chromosome.
	
		\param c The chromosome whose features are used for the penalty update.
	*/
	virtual void updatePenalties(const mh_solution *c);
	
	/** Reset penalties of all features to zero. */
	virtual void resetPenalties();
	
	/** Function to compute a tuned influence of penalties.
		The calculation is parametrized with the glsa parameter.
	
		\param c The chromosome 
	*/
	virtual double tuneLambda(mh_solution *c);
};

#endif //MH_QAPFEATURE_H
