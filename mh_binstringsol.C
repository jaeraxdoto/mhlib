// mh_binstringchrom.C - binary string chromosome

#include <fstream>
#include <assert.h>
#include "mh_binstringsol.h"
#include "mh_nhmove.h"
#include "mh_util.h"

namespace mh {

using namespace std;

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

bool binStringSol::k_flip_localsearch(int k) {
	assert(k>0 && k<=length);
	bool better_found=false;
	mh_solution *best_sol=mh_solution::to_mh_solution(clone());
	vector<int> p(k,-1);	// flipped positions
	// initialize 
	int i=0;	// current index in p to consider
	while (i>=0) {
		// evaluate solution
		if (i==k) {
			invalidate();
			if (isBetter(*best_sol)) {
				best_sol->copy(*this);
				better_found=true;
			}
			i--;	// backtrack
		} else {
			if (p[i]==-1) {
				// this index has not yet been placed
				p[i]=p[i-1]+1;
				data[p[i]]=!data[p[i]];
				i++; 	// continue with next position (if any)
			} else if (p[i]<length-(k-i)) {
				// further positions to explore with this index
				data[p[i]]=!data[p[i]];
				p[i]++;
				data[p[i]]=!data[p[i]];
				i++;
			} else {
				// we are at the last position with the i-th index, backtrack
				data[p[i]]=!data[p[i]];
				p[i]=-1;	// unset position
				i--;
			}
		}
	}
	if (better_found)
		copy(*best_sol);
	return better_found;
}

} // end of namespace mh

