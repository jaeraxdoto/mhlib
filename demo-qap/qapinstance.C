/*! \file qapinstance.C
  \include qapinstance.C */

#include <cmath>
#include <fstream>
#include <map>
#include "mh_util.h"
#include "qapchrom.h"
#include "qapinstance.h"

/// Name of file to load qap instance.
string_param qapfile( "qapfile", "name of file to load qap instance", "bur26a.dat" );


qapInstance *qapInstance::qi = 0;


qapInstance *qapInstance::getInstance()
{
	if ( qi != 0 )
		return qi;
	else
		return ( qi = new qapInstance() );
}

void qapInstance::initialize( const string &fname )
{
	// input file stream
	ifstream is( fname.c_str(), ios::in );

	// data
	int d;

	// indices
	int i;

	if ( is.is_open() )
	{
		n = 0;
	
		is >> n;

		if ( n > 0 )
		{
			// allocate memory
			a.resize(n*n);
			b.resize(n*n);

			// read first matrix
			for ( i=0; i<n*n && !is.eof(); i++ )
			{
				is >> d;
				a[i] = d;
			}
			
			// read second matrix
			for ( i=0; i<n*n && !is.eof(); i++ )
			{
				is >> d;
				b[i] = d;
			}
			
			// check for errors
			if ( is.fail() )
				mherror( "Error reading from file", fname.c_str() );

		}
	}
	else
		mherror( "Cannot open file", fname.c_str() );

	is.close();

	prepare();
}

void qapInstance::prepare()
{
	// size of sorted sets
	int nbeta = (int)floor(graspb(pgroup) * (n*n-n));
	
	// a-heap
	multimap< int, pair<int,int> > srta;
	
	// b-heap
	multimap< int, pair<int,int> > srtb;

	// cost-heap
	multimap<int,int> srtc;

	// iterators
	multimap< int, pair<int,int> >::iterator ita, itb;

	// index
	int i, j;


	indexa.resize(nbeta);
	indexb.resize(nbeta);
	cost.resize(nbeta);
	fdind.resize(nbeta);
	
	for ( i=0; i<n; i++ )
		for ( j=0; j<n; j++ )
		{
			if ( i!=j )
			{
				srta.insert( pair<int, pair<int,int> >(a[i*n+j], pair<int,int>(i,j) ) );
				srtb.insert( pair<int, pair<int,int> >(-b[i*n+j], pair<int,int>(i,j) ) );
			}
		}
	
	ita = srta.begin();
	itb = srtb.begin();
	
	for ( i=0; i<nbeta; i++ )
	{
		cost[i] = -(*ita).first * (*itb).first;
		indexa[i] = (*ita).second;
		indexb[i] = (*itb).second;
		srtc.insert( pair<int,int>( cost[i], i ) );

		ita++;
		itb++;
	}

	i=0;
	multimap<int,int>::iterator it=srtc.begin();

	while( i<nbeta && it != srtc.end() )
	{
		cost[i] = (*it).first;
		fdind[i] = (*it).second;

		i++;
		it++;
	}
}
