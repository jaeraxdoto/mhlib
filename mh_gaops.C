// mh_gaops.C

#include <algorithm>
#include <cmath>
#include <string>
#include "mh_gaops.h"
#include "mh_random.h"
#include "mh_util.h"

namespace mh {

using namespace std;

int gaopsProvider::mutation(double prob)
{
	int nmut;
	if (prob>=0)
	{
		nmut=int(prob);
		nmut+=random_prob(prob-nmut);
	}
	else 
	{
		// Interpret rate as rate/length per gene
		int l=length();
		if (prob<=-1000)
			if (l<=1)
				nmut=max(unsigned(1),
					random_poisson(-prob-1000));
			else
				nmut=max(unsigned(1),
					random_poisson(-prob-1000,l));
		else
			if (l<=1)
				nmut=random_poisson(-prob);
			else
				nmut=random_poisson(-prob,l);
	}
	// actually perform nmut mutations and also return nmut
	if (nmut>0)
		mutate(nmut);
	return nmut;
}

void gaopsProvider::selectNeighbour(int mvnbop)
{
	switch(mvnbop)
	{
		case 0:
			selectRandomNeighbour();
			break;

		case 1:
			selectImprovement( false );
			break;

		case 2:
			selectImprovement( true );
			break;

		default:
			mherror("Invalid parameter for mvnbop()", tostring(mvnbop));
			break;
	}
}

} // end of namespace mh

