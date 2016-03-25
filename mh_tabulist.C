// mh_tabulist.C

#include "mh_tabulist.h"
#include "mh_util.h"

namespace mh {

/// Length of tabulists
int_param tlsize( "tlsize", "length of tabulists", 10, 1, LOWER_EQUAL );

void tabulist::add( tabuAttribute *t )
{
	if ( t!=nullptr )
	{
		tabulist_entry n( t );
		tlist[n]++;

		if ( tqueue.size() >= size )
		{
			tabulist_entry u( tqueue.front() );
			tlist.erase( u );
			tqueue.pop();

			delete u.elem;
		}

		tqueue.push( t );
	}
}

bool tabulist::match( const tabuAttribute *t )
{
	if ( t!=nullptr )
	{
		tabulist_entry n( t );
		return ( tlist.count( n ) > 0 );
	}
	else
	{
		return false;
	}
}

} // end of namespace mh

