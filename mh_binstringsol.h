/*! \file mh_binstringsol.h
	\brief A generic binary string solution.
	*/

#ifndef MH_BINSTRINGSOL_H
#define MH_BINSTRINGSOL_H

#include "mh_stringsol.h"

namespace mh {

/** A binary string solution. */
class binStringSol : public stringSol<bool>
{
protected:
	static const binStringSol &cast(const mh_solution &ref)
		{ return (dynamic_cast<const binStringSol &>(ref)); }
public:
	binStringSol(const mh_bare_solution &c) : stringSol<bool>(c) { }
	/** Normal constructor, number of genes must be passed to base class. */
	binStringSol(int l, mh_base *t, const std::string &pg="") : stringSol<bool>(l,1,t,pg) { }
	binStringSol(int l, const std::string &pg="") : stringSol<bool>(l,1,pg) { }
	/** Writes out the binary string. */
	void write(std::ostream &ostr,int detailed=0) const;
	/** Function to apply a certain move.
	        This will only work with a bitflipMove. */
	void applyMove(const nhmove &m);
	/** Performs a best improvement k-flip local search, returning
	    true if an improved solution has been found. */
	bool k_flip_localsearch(int k);
};

} // end of namespace mh

#endif //MH_BINSTRINGSOL_H
