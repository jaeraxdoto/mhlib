/*! \file qapfeature.C
  \include qapfeature.C */

#include <deque>
#include <vector>
#include "qapfeature.h"

#include "../mh_nhmove.h"
#include "mh_util.h"

using namespace mhlib;
using namespace std;

namespace qap {

qapFeature::qapFeature(const string &pg) : feature(pg), qi(qapInstance::getInstance()), pv(qi->n*qi->n)
{
}

void qapFeature::updatePenalties(const mh_solution *c)
{
	double util;
	double maxutil = 0.0;
	deque<int> muind;
	
	const qapSol *qapc = dynamic_cast<const qapSol*>(c);

	// check if a valid qapSol* was provided
	if ( qapc==NULL )
		mherror("Solution is not a qapSol");
	
	for (int i=0; i<qi->n; i++)
	{
		double c=0.0;
			
		for ( int j=0; j<qi->n; j++ )
			c += qi->A(i,j) * qi->B(qapc->data[i],qapc->data[j]);

		util = c / ( 1.0 + pv[i*qi->n+qapc->data[i]] );

		if ( util>=maxutil )
		{
			if ( util>maxutil )
			{
				muind.clear();
				maxutil = util;
			}

			muind.push_back(i*qi->n+qapc->data[i]);			
		}
	}

	deque<int>::iterator it = muind.begin();

	while (it != muind.end())
	{
		pv[*it]++;
		it++;
	}
}

double qapFeature::penalty(const mh_solution *c)
{
	const qapSol *qapc = dynamic_cast<const qapSol*>(c);

	if ( qapc==NULL )
		mherror("Solution is not a qapSol");

	double p = 0.0;

	for (int i=0; i<qapc->length; i++)
	{
		p += pv[i*qi->n+qapc->data[i]];
	}
	
	return p;
}

double qapFeature::delta_penalty(const mh_solution *c, const nhmove *m)
{
	const qapSol *qapc = dynamic_cast<const qapSol*>(c);
	const swapMove *qm = dynamic_cast<const swapMove*>(m);

	if ( qapc==NULL )
		mherror("Solution is not a qapSol");

	if ( qm==NULL )
		mherror("Move is not a swapMove");

	double delta = 0.0;

	delta += ( pv[qm->r*qi->n+qapc->data[qm->s]] +
			pv[qm->s*qi->n+qapc->data[qm->r]] );
	
	delta -= ( pv[qm->r*qi->n+qapc->data[qm->r]] +
			pv[qm->s*qi->n+qapc->data[qm->s]] );

	return delta;
}

void qapFeature::resetPenalties()
{
	for( int i=0; i<qi->n*qi->n; i++ )
	{
		pv[i] = 0.0;
	}
}

double qapFeature::tuneLambda(mh_solution *c)
{
	return glsa(pgroup) * c->obj() / (qi->n*qi->n);
}

} // namespace qap

