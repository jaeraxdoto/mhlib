// mh_gaopsprov.C

#include <algorithm>
#include <cmath>
#include <string>
#include "mh_solution.h"
#include "mh_gaopsprov.h"
#include "mh_random.h"
#include "mh_util.h"

namespace mh {

using namespace std;

int_param mvnbop( "mvnbop", "step function 0:rand. neigh., 1:first imp. 2:best imp.", 0, 0, 2 );

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
		int length = dynamic_cast<const mh_solution &>(*this).length;
		// Interpret rate as rate/length per gene
		if (prob<=-1000)
			if (length<=1)
				nmut=max(unsigned(1),
					random_poisson(-prob-1000));
			else
				nmut=max(unsigned(1),
					random_poisson(-prob-1000,length));
		else
			if (length<=1)
				nmut=random_poisson(-prob);
			else
				nmut=random_poisson(-prob,length);
	}
	// actually perform nmut mutations and also return nmut
	if (nmut>0)
		mutate(nmut);
	return nmut;
}

void gaopsProvider::selectNeighbour()
{
	int mvnbo=mvnbop(dynamic_cast<const mh_solution &>(*this).pgroup);
	switch(mvnbo)
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
			mherror("Invalid parameter for mvnbop()", tostring(mvnbo));
			break;
	}
}

} // end of namespace mh

