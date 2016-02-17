/*! \file mh_tabuattribute.h 
	\brief An abstract base class for all tabuAttributes. */

#ifndef MH_TABUATTRIBUTE_H
#define MH_TABUATTRIBUTE_H

#include <iostream>
#include "mh_param.h"

namespace mh {

/** An element of a tabulist. */
class tabuAttribute
{
protected:
	/// Parametergroup
	std::string pgroup;
	
public:
	/// The constructor
	tabuAttribute( const std::string &pg="") : pgroup(pg) {}
	
	/// virtual destructor
	virtual ~tabuAttribute() {};

	/// Comparison of two tabus.
	virtual bool equals( const tabuAttribute &o ) const = 0;

	/** Hashing function.
		This function returns a hash-value for the tabuAttribute.
		Two tabuAttributes that are considered as equal must return the
		same value; however, identical hash-values for two
		tabus do not imply that the tabuAttributes are equal.
		This is needed for the hash-table of the tabulists. 
		The default implementation return simply 0.0. */
	virtual unsigned long int hashvalue() const = 0;
};

} // end of namespace mh

#endif //MH_TABUATTRIBUTE_H
