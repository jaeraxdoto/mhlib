/*! \file mh_binstringchrom.h 
	\brief A generic binary string chromosome.
	*/

#ifndef MH_BINSTRINGCHROM_H
#define MH_BINSTRINGCHROM_H

#include "mh_stringchrom.h"


/** A binary string chromosome. */
class binStringChrom : public stringChrom<bool>
{
protected:
	static const binStringChrom &toBSChrom(const mh_solution &ref)
		{ return (dynamic_cast<const binStringChrom &>(ref)); }
public:
	binStringChrom(const mh_solution &c) : stringChrom<bool>(c) { }
	/** Cormal constructor, number of genes must be passed to base class. */
	binStringChrom(int l, mh_base *t, const pstring &pg=(pstring)("")) : stringChrom<bool>(l,1,t,pg) { }
	binStringChrom(int l, const pstring &pg=(pstring)("")) : stringChrom<bool>(l,1,pg) { }
	/** Writes out the binary string. */
	void write(ostream &ostr,int detailed=0) const;
	/** Function to apply a certain move.
	        This will only work with a bitflipMove. */
	void applyMove(const move &m);
};

#endif //MH_BINSTRINGCHROM_H
