/*! \file qapsol.C
  \include qapsol.C */

#include <algorithm>
#include <cmath>
#include <fstream>
#include "mh_util.h"
#include "qapsol.h"

#include "../mh_nhmove.h"
#include "qapfeature.h"
#include "mh_tabusearch.h"

using namespace mh;
using namespace std;

namespace qap {

/// Alpha parameter for GRASP
double_param graspa( "graspa", "alpha for grasp", 0.25, 0.0, 1.0, UPPER_INCLUSIVE );

/// Beta parameter for GRASP
double_param graspb( "graspb", "beta for grasp", 0.5, 0.0, 1.0, UPPER_INCLUSIVE );

qapSol::qapSol(const mh_solution &c) : mh_solution(c), data(length)
{
	const qapSol &qapc=toQAPSol(c);
	qi=qapc.qi;
	for (int i=0;i<length;i++)
		data[i]=qapc.data[i];
}

void qapSol::copy(const mh_solution &orig)
{ 
	mh_solution::copy(orig);
	const qapSol &qapc=toQAPSol(orig);
	for (int i=0;i<length;i++) 
		data[i]=qapc.data[i]; 
}

bool qapSol::equals(mh_solution &o)
{ 
	// to be efficient: check first objective values
	if (o.obj()!=obj())
		return false;

	const qapSol &qapc=toQAPSol(o);
	// and now all the genes
	for (int i=0;i<length;i++) 
		if (data[i]!=qapc.data[i])
			return false;
	return true;
}

double qapSol::dist(mh_solution &c)
{
	const qapSol &qapc=toQAPSol(c);
	int diffs=0;
	for (int i=0;i<length;i++)
		if (data[i]!=qapc.data[i])
			diffs++;
	return diffs;
}

void qapSol::initialize(int count)
{
	for (int i=0;i<length;i++) 
		data[i]=i;
	for (int i=0;i<length-1;i++)
		swap(data[i],data[random_int(i,length-1)]);
	
	invalidate();
}

double qapSol::objective()
{
	double o = 0.0;
	int i, j;

	for ( i = 0; i < length; i++ )
	{
		for ( j = 0; j < length; j++ )
		{
			o += qi->A(i,j) * qi->B(data[i],data[j]);
		}
	}

	aObjProvider *ap = dynamic_cast<aObjProvider*>(alg);
	if (ap!=NULL)
		o += ap->aobj(this);
	
	return o;
}

void qapSol::mutate(int count) 
{
	tabuSearch *ts = dynamic_cast<tabuSearch*>(alg);
	qapTabuAttribute qta(pgroup);
	swapMove qm;

	if (!objval_valid)
	{
		objval = objective();
		objval_valid = true;
	}

	for (int i=0;i<count;i++)
	{
		qm.r=random_int(length);
		qm.s=random_int(length);

		objval += delta_obj(qm);
		applyMove(qm);

		if (ts!=NULL && ( ts->isTabu(&qta) && !ts->aspiration( this ) ) )
		{
			objval += delta_obj(qm);
			applyMove(qm);
		}
	
	}
}

void qapSol::crossover(const mh_solution &parA,const mh_solution &parB) 
{
	const qapSol &a = toQAPSol(parA);
	const qapSol &b = toQAPSol(parB);

	// fill all genes from one parent
	for (int i=0;i<length;i++)
		data[i] = b.data[i];

	vector<int> pos(length);

	// build a table with all positions in the solution
	for (int i=0;i<length;i++)
		pos[a.data[i]] = i;

	vector<bool> d(length,true);

	// tc = city to take
	int tc = a.data[0];

	// fill genes until a cycle is reached
	while (!d[tc])
	{
		data[pos[tc]] = a.data[pos[tc]];
		d[tc] = true;
		tc = b.data[pos[tc]];
	}
	invalidate();
}

void qapSol::write(ostream &ostr,int detailed) const
{
	for (int i=0;i<length;i++) 
		ostr << data[i]+1 << ' ';
	ostr << endl;
}

void qapSol::save(const char *fname)
{
	char s[40];
	sprintf(s,nformat(pgroup).c_str(),obj());

	ofstream of(fname);
	if (!of)
		mherror("Cannot open file",fname);
	of << length << ' ' << s << endl;
	for (int i=0;i<length;i++) 
		of << data[i]+1 << ' ';
	of << endl;
	if (!of)
		mherror("Cannot open file",fname);
}

void qapSol::load(const char *fname)
{
	ifstream inf(fname);
	int d;
	if (!inf)
		mherror("Cannot open file",fname);
	inf >> d >> d;
	for (int i=0;i<length;i++) 
	{
		inf >> d;
		if (!inf)
			mherror("Cannot open file",fname);
		data[i]=int(d-1);
	}
}

unsigned long int qapSol::hashvalue()
{
	unsigned h=0;
	unsigned window=sizeof(h)*8-unsigned(ceil(log(double(length+1))/
		log(2.0)));
	for (int i=0;i<length;i++)
		if (data[i])
			h^=data[i]<<(i%window);
	return h;
}

double qapSol::delta_obj(const nhmove &m)
{
	double delta = 0.0;
	const swapMove &qm = dynamic_cast<const swapMove &>(m);

	for ( int k = 0; k < length; k++ )
	{
		if ( k != qm.r && k != qm.s )
		{
			delta += (qi->A(k,qm.r) - qi->A(k,qm.s)) *
					(qi->B(data[k],data[qm.s]) - qi->B(data[k],data[qm.r]));
				
			delta += (qi->A(qm.r,k) - qi->A(qm.s,k)) *
					(qi->B(data[qm.s],data[k]) - qi->B(data[qm.r],data[k]));
		}
	}

	delta += (qi->A(qm.r,qm.r) - qi->A(qm.s,qm.s)) *
			(qi->B(data[qm.s],data[qm.s]) - qi->B(data[qm.r],data[qm.r]));

	delta += (qi->A(qm.r,qm.s) - qi->A(qm.s,qm.r)) *
			(qi->B(data[qm.s],data[qm.r]) - qi->B(data[qm.r],data[qm.s]));

	aObjProvider *ap = dynamic_cast<aObjProvider*>(alg);
	if (ap!=NULL)
		delta += ap->delta_aobj(this,&m);

	return delta;
}

inline void qapSol::applyMove(const nhmove &m)
{
	const swapMove &qm = dynamic_cast<const swapMove &>(m);
	swap(data[qm.r],data[qm.s]);
}

void qapSol::selectImprovement(bool find_best)
{
	// information about best known move
	swapMove bqm;
	double bestobj;

	// other local stuff
	bool cont=true;
	qapTabuAttribute qta(pgroup);
	swapMove qm;
	tabuSearch *ts = dynamic_cast<tabuSearch*>(alg);

	// initialization
	bqm.r=qm.r=0;
	bqm.s=qm.s=1;
	bestobj = obj();
	if (find_best)
		bestobj += delta_obj(qm);

	for (int i=0; i<length && cont; i++)
	{
		for (int j=i+1; j<length && cont; j++)
		{
			qm.r = i;
			qm.s = j;
			objval += delta_obj(qm);
			applyMove(qm);
			
			if (maxi(pgroup) ? objval>bestobj : objval<bestobj)
			{
				qta = qm;

				if ( ts==NULL || (ts!=NULL && ( !ts->isTabu(&qta) || ts->aspiration( this ) ) ) )
				{
					bqm = qm;
					bestobj = objval;

					if (!find_best)
						cont=false;
				}
			}

			objval += delta_obj(qm);
			applyMove(qm);
		}
	}

	objval += delta_obj(bqm);
	applyMove(bqm);

	if ( ts!=NULL )
	{
		qta = bqm;
		ts->tl_ne->add( new qapTabuAttribute( qta ) );
	}
}

feature* qapSol::getFeature()
{
	return new qapFeature( pgroup );
}

void qapSol::greedyConstruct()
{
	//BUG: invalid assignments are generated
	
	int i, j, k, l, tmp;
	int cost;
	vector<int> a(qi->n),b(qi->n);
	multimap< int, pair<int,int> > srtc;
	multimap< int, pair<int,int> >::iterator it;

	/** construction phase one:
	    make first two assignments */
	for ( i=0; i<qi->n; i++ )
	{
		a[i] = i;
		b[i] = i;
	}
	
	int nselct = random_int( (int) (graspa(pgroup)*graspb(pgroup)*(qi->n*qi->n-qi->n)) );

	i = qi->indexa[qi->fdind[nselct]].first;
	j = qi->indexa[qi->fdind[nselct]].second;
	k = qi->indexb[qi->fdind[nselct]].first;
	l = qi->indexb[qi->fdind[nselct]].second;
	
	// make assignment one
	swap( a[0], a[i] );
	swap( b[0], b[k] );

	// make assignment two
	for ( i=0; i<qi->n; i++ )
	{
		if ( a[i] == j )
		{
			swap( a[1], a[i] );
			break;
		}
	}
	
	for ( i=0; i<qi->n; i++ )
	{
		if ( b[i] == l )
		{
			swap( b[1], b[i] );
			break;
		}
	}

	/** construction phase two:
	    make final assignments */
	for ( i=2; i<qi->n-1; i++ )
	{
		srtc.clear();

		for ( k=i; k<qi->n; k++ )
			for ( l=i; l<qi->n; l++ )
			{
				cost = 0;

				for ( j=0; j<i-1; j++ )
					cost += qi->B(b[k],a[j])*qi->A(a[l],b[j]);
				
				srtc.insert( pair< int, pair<int,int> >(cost, pair<int,int>(a[l],b[k]) ) );
			}
		
		
		// make assignment
		nselct = random_int( (int) (graspa(pgroup)*srtc.size()) );
		
		it = srtc.begin();

		for ( j=0; j<nselct; j++)
			it++;

		for ( j=i; j<qi->n; j++ )
		{
			if ( a[j] == (*it).second.first )
			{
				k = j;
				break;
			}
		}
		
		for ( j=i; j<qi->n; j++ )
		{
			if ( b[j] == (*it).second.second )
			{
				l = j;
				break;
			}
		}
		
		tmp = a[i];
		a[i] = a[k];
		a[k] = tmp;

		tmp = b[i];
		b[i] = b[l];
		b[l] = tmp;
	}
	

	// save generated instance to data vector
	for ( i=0; i<qi->n; i++ )
	{
		data[a[i]] = b[i];
	}
}

} // qap namespace

