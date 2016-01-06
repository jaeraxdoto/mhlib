/*! \file mh_hash.h 
	\brief Various hash functions. */

#ifndef MH_HASH_H
#define MH_HASH_H

#include <string>

namespace mhlib {

// Forward declaration of class tabulist_entry
class tabulist_entry;


/** Functor providing a hash function for std::string. */
struct hashstring
{
	/** Hashing function.  */
	size_t operator()( const std::string &s ) const;
};


/** Functionclass hashing double's. */
struct hashdouble
{
	/** Hashing function.
		Performs a simple cast. */
	size_t operator()(double d) const;
};


/** A functor providing a hash function for tabulist_entry. */
struct hashtabulist_entry
{
	/** Hashing function.
		Incorporation tabuAttribute::hashvalue(). */
	size_t operator()( const tabulist_entry &e ) const;
};

} // end of namespace mhlib

#endif // MH_HASH_H
