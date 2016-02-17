/*! \file qaptabuattribute.C
  \include qaptabuattribute.C */

#include "qaptabuattribute.h"

using namespace mh;
using namespace std;

namespace qap { 

qapTabuAttribute::qapTabuAttribute(const qapTabuAttribute &t) : swapMove(t.r,t.s), tabuAttribute(t)
{
}

bool qapTabuAttribute::equals( const tabuAttribute &o ) const
{
	const qapTabuAttribute &qapt=toQAPTabuAttribute(o);
	return ( (r==qapt.r && s==qapt.s) ||  (r==qapt.s && s==qapt.r) );
}

unsigned long int qapTabuAttribute::hashvalue() const
{
	return r*s;
}

} // namespace qap

