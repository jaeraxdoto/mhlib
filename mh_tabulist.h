/*! \file mh_tabulist.h 
	\brief Data structure for handling of Tabus */ 

#ifndef MH_TABULIST_H
#define MH_TABULIST_H

#include <unordered_map>
#include <queue>
#include "mh_hash.h"
#include "mh_param.h"
#include "mh_tabuattribute.h"

namespace mhlib {

/** \ingroup param
    Number of tabuattributes per tabulist. */
extern int_param tlsize;

/** An element of the hashtable in a tabulist. */
class tabulist_entry
{
public:
	const tabuAttribute *elem;

	explicit tabulist_entry( const tabuAttribute *t ) : elem( t ) {};

	bool operator==( const tabulist_entry &o ) const { return elem->equals( *o.elem ); };
};


/** A hash-table for the members of the tabulist. 
    This includes a queue of the tabus so dropping of old
    tabus is possible. */
class tabulist
{
protected:
	/// Number of tabus
	size_t size;
	
	/// The tabu list
	std::unordered_map<tabulist_entry,int, hashtabulist_entry> tlist;

	/// The history cycle buffer
	std::queue<tabuAttribute*> tqueue;

	/// Parametergroup
	std::string pgroup;
	
public:
	/** Normal constructor.
	        A tabulist of given size is created. */
	explicit tabulist( int N, const pstring &pg=(pstring)("")) : size(N), pgroup(pg.s) {}
	/** Default constructor.
	        Size of tabulist is determined through parameters. */      
	explicit tabulist( const pstring &pg=(pstring)("") ) : size(tlsize(pg.s)), pgroup(pg.s) {}
	/** The destructor. */
	virtual ~tabulist() {}
	/** Removes all entries in the hash-table. */
	virtual void clear() { tlist.empty(); tqueue.empty(); };
	/** Adds one new tabuAttribute to the tabulist. */
	virtual void add( tabuAttribute *t );
	/** Checks if a given tabuAttribute is already known. */
	virtual bool match( const tabuAttribute *t );
};

} // end of namespace mhlib

#endif //MH_TABULIST_H
