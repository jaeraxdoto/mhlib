/*! \file qaptabuattribute.h
	\brief A simple specialized tabuAttribute class for the quadratic assignment.
	*/

#ifndef MH_QAPTABUATTRIBUTE_H
#define MH_QAPTABUATTRIBUTE_H

#include "mh_nhmove.h"
#include "mh_tabuattribute.h"

namespace qap {

/** A simple specialized tabuAttribute class for the quadratic assignment. */
class qapTabuAttribute : public mh::swapMove, public mh::tabuAttribute
{
	friend class qapSol;

protected:
	static const qapTabuAttribute &cast(const mh::tabuAttribute &ref)
		{ return (dynamic_cast<const qapTabuAttribute &>(ref)); }

public:
	/** Normal constructor.
		\param pg Parametergroup
	*/
	qapTabuAttribute(const std::string &pg="") : mh::swapMove(), mh::tabuAttribute(pg)  {};

	/** Copy constructor.
	
		\param t Object to copy from.
	*/
	qapTabuAttribute(const qapTabuAttribute &t);

	/** Copy constructor.
		\param m Object to copy from.
	*/
	qapTabuAttribute(const mh::swapMove &m) : mh::swapMove(m) {};

	/** Comparison of two tabuAttributes.
		\param o Object to compare to.
	*/
	bool equals( const mh::tabuAttribute &o ) const;

	/** Hashing function.
		This function returns a hash-value for the tabuAttribute.
		Two tabuAttributes that are considered as equal must return the
		same value; however, identical hash-values for two
		tabus do not imply that the tabuAttributess are equal.
		This is needed for the hash-table of the tabulists. */
	unsigned long int hashvalue() const;
};

} // namespace qap

#endif //MH_QAPTABUATTRIBUTE_H
