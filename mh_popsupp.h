/*! \file mh_popsupp.h 
	\brief Data structures to support efficient population handling,
	in particular for the steady-state replacement scheme.. */ 

#ifndef MH_POPSUPP_H
#define MH_POPSUPP_H

#include <list>
#include <map>
#include <unordered_map>
#include "mh_base.h"
#include "mh_popbase.h"
#include "mh_util.h"

namespace mhlib {

/** Compare functor for worstheap. */
struct worstcompare : public std::binary_function<double,double,bool>
{
	/// Parametergroup.
	std::string pgroup;
	worstcompare(const std::string &pg="") : pgroup(pg) {}

	/// Compare function.
	bool operator()(const double &a, const double &b) const;
};


/** \ingroup param
	Should the worstheap data structure be maintained?
	This is only meaningful when #repl==1 (i.e. replace the worst)
	in a steady-state evolutionary algorithm.
	In this case it will speed up the EA, in particular on large populations. */
extern bool_param wheap;


typedef std::multimap<double,int,worstcompare>::iterator worstheap_item;

/** An element of pophashtable. */
struct pophashtable_elem
{
	mh_solution *p;	// pointer to solution
	int idx;	// index in population
	worstheap_item pqi;	// reference into worstheap
	pophashtable_elem() {}
	pophashtable_elem(mh_solution *p1,int idx1, worstheap_item pqi1)
		{ p=p1; idx=idx1; pqi=pqi1; }
};


/** A hash-table for the members of the population. 
	This includes a heap of the solutions so that the worst solution
	can efficiently be retrieved, if #wheap is set. */
class pophashtable
{
protected:
	/// Parametergroup
	std::string pgroup;

	std::unordered_map<unsigned long int,std::list<pophashtable_elem> > table;
	/** heap to obtain worst solution efficiently.
		Only maintained if wheap is set. 
		The info component is the index in the population. */
	std::multimap<double,int,worstcompare> worstheap;

public:
	/// Initialize the hash-table to be empty.
	pophashtable(const std::string &pg="") : pgroup(pg), worstheap(worstcompare(pg)) {}
	/// Cleans up.
	virtual ~pophashtable() {}
	/// Removes all entries in the hash-table.
	void clear()
		{ table.clear(); worstheap.clear(); }
	/** Adds one new population member into the hash-table with its
		pointer and population-index. */
	void add(mh_solution *p,int idx);
	/// Removes entry for this solution from the hash-table.
	void remove(mh_solution *p);
	/** Looks, if a duplicate of *p is already in the hash-table.
		If this is the case, its index in the population is
		returned; otherwise, -1 is returned. */
	int findDuplicate(mh_solution *p);
	/** if wheap() is set, the worstheap is maintained; in this
		case, this function returns the index of the worst
		solution. */
	int worstIndex();
};

std::istream & operator>>(std::istream &is,pophashtable_elem &e);
std::ostream & operator<<(std::ostream &os,const pophashtable_elem &e);

} // end of namespace mhlib

#endif //MH_POPSUPP_H

