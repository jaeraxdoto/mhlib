// mh_binstringchrom.C - binary string chromosome

#include <fstream>
#include "mh_binstringsol.h"
#include "mh_nhmove.h"
#include "mh_util.h"

// instantiate template
template class stringSol<bool>;

void binStringSol::write(ostream &ostr,int detailed) const
{
	for (int i=0;i<length;i++) 
		ostr << (data[i]?1:0);
}

void binStringSol::applyMove(const nhmove &m)
{
	const bitflipMove &qm = dynamic_cast<const bitflipMove &>(m);
	data[qm.r] = !data[qm.r];
}

