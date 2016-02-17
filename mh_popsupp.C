// mh_popsupp.C

#include "mh_popsupp.h"

namespace mh {

using namespace std;

//------------------------- pophashtable -----------------------------

bool_param wheap("wheap","Use the worstheap data structure",true);

inline bool worstcompare::operator()(const double &a, const double &b) const
{
	return maxi(pgroup)?a<b:a>b;
}

void pophashtable::add(mh_solution *p,int idx)
{ 
	worstheap_item it=worstheap.end();
	if (wheap(pgroup))
		it=worstheap.insert(pair<double,int>(p->obj(),idx));
	pophashtable_elem e(p,idx,it);
	unsigned long int h=p->hashvalue();
	table[h].push_back(e);
}

void pophashtable::remove(mh_solution *p)
{
	unsigned long int h=p->hashvalue();
	list<pophashtable_elem> &l=table[h];
	for (list<pophashtable_elem>::iterator it=l.begin();it!=l.end();it++)
		if ((*it).p==p)
		{
			if (wheap(pgroup))
				worstheap.erase((*it).pqi);
			l.erase(it);
			if (l.empty())
				table.erase(h);
			return;
		}
}

int pophashtable::findDuplicate(mh_solution *p)
{
	list<pophashtable_elem> l=table[p->hashvalue()];
	if (!l.empty())
	{
		for (list<pophashtable_elem>::iterator it=l.begin();it!=l.end();it++)
			if ((*it).p->equals(*p))
				return (*it).idx;
	}
	return -1;
}

int pophashtable::worstIndex()
{
	if (!wheap(pgroup))
		mherror("worstIndex() called without wheap() set");
	return (*worstheap.begin()).second;
}

istream & operator>>(istream &is,pophashtable_elem &e)
{ mherror("No operator>> for pophashtable_elem defined"); return is; }

ostream & operator<<(ostream &os,const pophashtable_elem &e)
{ os << e.p << " # " << e.idx; return os; }

} // end of namespace mh

