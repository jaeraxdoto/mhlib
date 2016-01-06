// mh_hash.C

#include <unordered_map>
#include "mh_hash.h"
#include "mh_tabulist.h"

namespace mhlib {

size_t hashstring::operator()( const std::string &s ) const
{
	std::hash<std::string> hash_fn;
	return hash_fn(s);
}

size_t hashdouble::operator()(double d) const
{
	return (size_t)d;
}

size_t hashtabulist_entry::operator()( const tabulist_entry &e ) const
{
	return (size_t)e.elem->hashvalue();
}

} // end of namespace mhlib

