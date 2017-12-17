// mh_popbase.C

#include "mh_popbase.h"

namespace mh {

int_param popsize("popsize","size of the population",100,1,10000000);

int_param dupelim("dupelim","eliminate duplicates 0:no 1:child 2:ini+child"
	,1,0,2);

void pop_base::init(bool nohashing)
{
	if (!nohashing && dupelim(pgroup))
		phash=new pophashtable;
	statValid=false;
}
	
pop_base::~pop_base()
{
	if (phash)
		delete phash;
}

void pop_base::recreateHashtable() {
	if (!phash)
		return;
	phash->clear();
	for (int i=0;i<nSolutions;i++)
		phash->add(at(i),i);
}

} // end of namespace mh

