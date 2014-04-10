// mh_popbase.C

#include "mh_popbase.h"

int_param popsize("popsize","size of the population",100,1,10000000);

int_param dupelim("dupelim","eliminate duplicates: 0:no 1:child 2:ini+child"
	,1,0,2);

void pop_base::init(int psize)
{
	if (dupelim(pgroup))
		phash=new pophashtable;
	else
		phash=0;
	nChroms=psize;
	statValid=false;
}
	
pop_base::~pop_base()
{
	if (phash)
		delete phash;
}
