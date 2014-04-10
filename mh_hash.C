// mh_hash.C

#include <ext/hash_map>
#include "mh_hash.h"
#include "mh_tabulist.h"

using namespace __gnu_cxx;

size_t hashstring::operator()( const string &s ) const
{
	return __stl_hash_string( s.c_str() );
}

size_t hashdouble::operator()(double d) const
{
	return (size_t)d;
}

size_t hashtabulist_entry::operator()( const tabulist_entry &e ) const
{
	return (size_t)e.elem->hashvalue();
}

