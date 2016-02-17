/*! \file mh_base.h 
	\brief An abstract base class for all evolutionary algorithms.

	This module contains only a few very basic definitions which
	are common in any EA. */

#ifndef MH_BASE_H
#define MH_BASE_H

#include "mh_log.h"

namespace mh {

/** The most abstract base class for all EAs.
	This abstract base contains methods and attributes that might be
	needed in any EA.
	If you derive a new EA, use mh_base as the
	base class, if no other, derived class suits your needs. */
class mh_base
{
public:
	/// The Constructor.
	mh_base( const std::string &pg = "") : pgroup(pg) {}
	/// Virtual destructor.
	virtual ~mh_base() {}
	/// Run method.
	virtual void run() = 0;

public:
	/// Parametergroup
	std::string pgroup;
};

} // end of namespace mh

#endif //MH_BASE_H

