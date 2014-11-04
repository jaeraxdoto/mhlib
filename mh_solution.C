// solution.C

#include <algorithm>
#include <cmath>
#include <string>
#include "mh_solution.h"
#include "mh_random.h"
#include "mh_util.h"

bool_param maxi("maxi","should be maximized?",true);

int_param mvnbop( "mvnbop", "Neighbour function selection to use", 0, 0, 2 );

int mh_solution::mutation(double prob)
{
	int nmut;
	if (prob>=0)
	{
		nmut=int(prob);
		nmut+=random_prob(prob-nmut);
	}
	else 
	{
		// interprete rate as rate/length per gene
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

void mh_solution::selectNeighbour()
{
	switch( mvnbop(pgroup) )
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
			mherror("Invalid parameter for mvnbop()",
				mvnbop.getStringValue(pgroup).c_str());
			break;
	}
}
