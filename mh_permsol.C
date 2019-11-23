// mh_permsol.C

#include <vector>
#include "mh_permsol.h"
#include "mh_util.h"

namespace mh {

using namespace std;

int_param permxop("permxop","permutation crossover operator 0:random 1:pmx 2:ox 3:cx 4:uobx 5:c1",1,0,50);
// 6:erx 7:eerx 8:mpx not yet implemented

int_param permmop("permmop","permutation mutation operator 0:random 1:inversion 2:exchange 3:insertion",1,0,50);
// 4:displacement not yet implemented

void permSol::initialize(int count)
{
	for (int i=0;i<length;i++)
		data[i] = permSolVarType(i);
	random_shuffle(data);
	invalidate();
}

void permSol::mutate(int count)
{
	int c=permmop(pgroup);
	if (c==0) 
		c=random_int(1,3);
	switch(c)
	{
		case 1: mutate_inversion(count);
			break;

		case 2: mutate_exchange(count);
			break;

		case 3: mutate_insertion(count);
			break;

		/*
		case 4: mutate_displacement(count);
			break;
		*/

		default: mherror("Wrong mutation operator permmop selected", tostring(c)); break;
	}
}

void permSol::mutate_inversion(int count)
{
	int c1,c2;
	for (int i=0;i<count;i++)
	{
		get_cutpoints(c1,c2);
		while (c1<c2)
		{
			int t=data[c1];
			data[c1++]=data[c2];
			data[c2--]=t;
		}
	}
	invalidate();
}

void permSol::mutate_exchange(int count)
{
	int c1,c2;
	for (int i=0;i<count;i++)
	{
		get_cutpoints(c1,c2);
		permSolVarType t = data[c1];
		data[c1] = data[c2];
		data[c2] = t;
	}
	invalidate();
}

void permSol::mutate_insertion(int count)
{
	int cs,ci;
	for (int i=0;i<count;i++)
	{
		get_cutpoints(cs,ci,false);
		permSolVarType t = data[cs];
		if (cs < ci) {
			for (int i = cs; i < ci; i++)
				data[i] = data[i + 1];
			data[ci] = t;
		} else {
			for (int i = cs; i > ci; i--)
				data[i] = data[i - 1];
			data[ci] = t;
		}
	}
	invalidate();
}

void permSol::crossover(const mh_solution &parA,const mh_solution &parB)
{
	int c;
	if (permxop(pgroup)) 
		c=permxop(pgroup); 
	else 
		c=random_int(1,5);
	switch(c)
	{
		case 1: crossover_pmx(parA,parB);
			break;

		case 2: crossover_ox(parA,parB);
			break;

		case 3: crossover_cx(parA,parB);
			break;

		case 4: crossover_uobx(parA,parB);
			break;

		case 5: crossover_c1(parA,parB);
			break;
		/*
		case 6: crossover_erx(parA,parB);
			break;

		case 7: crossover_eerx(parA,parB);
			break;

		case 8: crossover_mpx(parA,parB);
			break;
		*/
		default: mherror("Wrong crossover operator permxop selected", tostring(c)); break;
	}
}

// PMX operator
void permSol::crossover_pmx(const mh_solution &parA, const mh_solution &parB)
{
	const permSol &a = cast(parA);
	const permSol &b = cast(parB);

	int c1,c2;      // cutpoints [0,length[

	get_cutpoints(c1,c2);

	vector<int> m(length,-1); // m defines the mapping
    
	// all variables between c1-c2 are taken from parent A
	// and their mapping is stored in m
	for (int i=c1;i<c2;i++)
		m[data[i] = a.data[i]] =  b.data[i];

	// all other variables are taken from parent B and checked with the mapping m
	for (int i=0;i<c1;i++)
	{
		data[i]=b.data[i];
		while (m[data[i]]!=-1)
			data[i]=m[data[i]];
	}

	for (int i=c2;i<length;i++)
	{
		data[i]=b.data[i];
		while (m[data[i]]!=-1) 
			data[i]=m[data[i]];
	}
	invalidate();
}

// OX operator
void permSol::crossover_ox(const mh_solution &parA,const mh_solution &parB)
{
	const permSol &a = cast(parA);
	const permSol &b = cast(parB);

	int c1,c2;      // cutpoints [0,length[

	get_cutpoints(c1,c2);

	int t = c2;
    
	vector<bool> s(length,false);

	for (int i=c1;i<c2;i++)
		s[data[i]=a.data[i]]=true;

	for (int i=c2;i<length;i++)
		if (!s[b.data[i]])
			data[t++%length]=b.data[i];

	for (int i=0;i<c2;i++)
		if (!s[b.data[i]])
			data[t++%length]=b.data[i];
	invalidate();
}

// CX operator
void permSol::crossover_cx(const mh_solution &parA,const mh_solution &parB)
{
	const permSol &a = cast(parA);
	const permSol &b = cast(parB);

	// fill all variables from one parent
	for (int i=0;i<length;i++)
		data[i] = b.data[i];

	vector<int> pos(length);

	// build a table with all positions in the solution
	for (int i=0;i<length;i++)
		pos[a.data[i]] = i;

	vector<bool> d(length,true);

	// tc = city to take
	int tc = a.data[0];

	// fill variables until a cycle is reached
	while (!d[tc])
	{
		data[pos[tc]] = a.data[pos[tc]];
		d[tc] = true;
		tc = b.data[pos[tc]];
	}
	invalidate();
}

// UOBX operator
void permSol::crossover_uobx(const mh_solution &parA,const mh_solution &parB)
{
	const permSol &a = cast(parA);
	const permSol &b = cast(parB);

	vector<bool> s(length,false);
	for (int i=0;i<length;i++)
		if (random_bool())
			data[i]=a.data[i];
		else
			s[a.data[i]]=true;
	int pos=0;
	for (int i=0;i<length;i++)
		if (s[a.data[i]]) // position i needs to be filled
		{
			while (!s[b.data[pos]])
				pos++;
			data[i]=b.data[pos++];
		}
	invalidate();
}

// C1 operator
// a variation of the UOBX operator
void permSol::crossover_c1(const mh_solution &parA,const mh_solution &parB)
{
	const permSol &a = cast(parA);
	const permSol &b = cast(parB);

	int c=random_int(1,length-1);

	vector<bool> s(length,false);
	int i;
	for (i=0;i<c;i++)
		data[i]=a.data[i];
	for (;i<length;i++)
		s[a.data[i]]=true;
	int pos=0;
	for (i=c;i<length;i++)
	{
		while (!s[b.data[pos]])
			pos++;
		data[i]=b.data[pos++];
	}
	invalidate();
}

#ifdef notused

// ERX operator
void permSol::crossover_erx(const mh_solution &parA,const mh_solution &parB)
{
	const permSol &a = cast(parA);
	const permSol &b = cast(parB);

	LEDA::d_array<int,LEDA::set<int> > hash;
	LEDA::list<int> res;                    // result list

	// construct edge list
	for (int i=0;i<length;i++)
	{
		hash[a.data[i%length]].insert(a.data[(i+1)%length]);
		hash[a.data[(i+1)%length]].insert(a.data[i%length]);
		hash[b.data[i%length]].insert(b.data[(i+1)%length]);
		hash[b.data[(i+1)%length]].insert(b.data[i%length]);
	}

	// rescue bag
	LEDA::set<int> set;
	for (int i=0;i<length;i++)
		set.insert(a.data[i]);

	int city=hash[a.data[0]].size() < hash[b.data[0]].size() ? a.data[0] : b.data[0];
	int nr=1;
	int tc;

	for (int i=0;i<length;i++)
	{
		if (!nr)
		{
			forall(tc,hash[city])
			{
				if (!nr || hash[tc].size() < nr)
				{
					res.clear();
					nr = hash[tc].size();
					res.append(tc);
				}
				if (hash[tc].size() == nr)
					res.append(tc);
			}
			if (!nr)
				city=set.choose();
			else
			{
				res.permute();
				city=res.front();
			}
		}
		forall_defined(tc, hash) hash[tc].del(city);
		set.del(city);
		data[i]=city;
		nr=0;
	}
	invalidate();
}

// EERX operator
// enhanced ERX operator
void permSol::crossover_eerx(const mh_solution &parA,const mh_solution &parB)
{
	const permSol &a = cast(parA);
	const permSol &b = cast(parB);

	LEDA::d_array<int,LEDA::set<int> > hash;
	LEDA::list<int> res;                    // result list

	// construct edge list
	for (int i=0;i<length;i++)
	{
		if (hash[a.data[i%length]].member(a.data[(i+1)%length]))
		{
			hash[a.data[i%length]].del(a.data[(i+1)%length]);
			hash[a.data[i%length]].insert(a.data[(i+1)%length]*(-1));
		}
		else hash[a.data[i%length]].insert(a.data[(i+1)%length]);
		if (hash[a.data[(i+1)%length]].member(a.data[i%length]))
		{
			hash[a.data[(i+1)%length]].del(a.data[i%length]);
			hash[a.data[(i+1)%length]].insert(a.data[i%length]*(-1));
		}
		else hash[a.data[(i+1)%length]].insert(a.data[i%length]);
		if (hash[b.data[i%length]].member(b.data[(i+1)%length]))
		{
			hash[b.data[i%length]].del(b.data[(i+1)%length]);
			hash[b.data[i%length]].insert(b.data[(i+1)%length]*(-1));
		}
		else hash[b.data[i%length]].insert(b.data[(i+1)%length]);
		if (hash[b.data[(i+1)%length]].member(b.data[i%length]))
		{
			hash[b.data[(i+1)%length]].del(b.data[i%length]);
			hash[b.data[(i+1)%length]].insert(b.data[i%length]*(-1));
		}
		else hash[b.data[(i+1)%length]].insert(b.data[i%length]);
	}

	// rescue bag
	LEDA::set<int> set;
	for (int i=0;i<length;i++)
		set.insert(a.data[i]);

	int city=hash[a.data[0]].size() < hash[b.data[0]].size() ? a.data[0] : b.data[0];
	int nr=1;
	int tc;

	for (int i=0;i<length;i++)
	{
		if (!nr)
		{
			forall(tc,hash[city])
			{
				if (tc < 0)
				{
					res.clear();
					res.push(abs(tc));
					nr = 1;
					break;
				}
				if (!nr || hash[tc].size() < nr)
				{
					res.clear();
					nr = hash[tc].size();
					res.append(tc);
				}
				if (hash[tc].size() == nr)
					res.append(tc);
			}
			if (!nr)
				city=set.choose();
			else
			{
				res.permute();
				city=res.front();
			}
		}
		forall_defined(tc,hash)
		{
			if (hash[tc].member(city)) hash[tc].del(city);
			else hash[tc].del(city*(-1));
		}
		set.del(city);
		data[i]=city;
		nr=0;
	}
	invalidate();
}

// MPX operator
// Maximal preservative crossover
void permSol::crossover_mpx(const mh_solution &parA,const mh_solution &parB)
{
	const permSol &a = cast(parA);
	const permSol &b = cast(parB);

	int c1,c2,t=0,r=random_bool();

	get_cutpoints(c1,c2);

	LEDA::set<int> set;

	for (int i=c1;i<c2;i++)
	{
		data[t++]= r ? a.data[i] : b.data[i];
		set.insert(r ? a.data[i] : b.data[i]);
	}

	for (int i=0;i<length;i++)
		if (!set.member(r ? b.data[i] : a.data[i]))
			data[t++]=r ? b.data[i] : a.data[i];
	invalidate();
}

#endif // notused

void permSol::applyMove(const nhmove &m)
{
	const swapMove &qm = dynamic_cast<const swapMove &>(m);
	swap(data[qm.r],data[qm.s]);
}

} // end of namespace mh

