// mh_stringchrom_impl.h - string chromosome implementation

#ifndef MH_STRINGCHROM_IMPL_HXX
#define MH_STRINGCHROM_IMPL_HXX

#include <cmath>
#include <fstream>
#include "mh_stringchrom.h"
#include "mh_util.h"


template <class T> stringChrom<T>::stringChrom(const mh_solution &c) : 
	mh_solution(c),data(length)
{ 
	const stringChrom<T> &sc=toSChrom(c);
	for (int i=0;i<length;i++) 
		data[i]=sc.data[i];
	vmax = sc.vmax;
}

template <class T> void stringChrom<T>::copy(const mh_solution &orig)
{ 
	mh_solution::copy(orig);
	const stringChrom<T> &sc=toSChrom(orig);
	for (int i=0;i<length;i++) 
		data[i]=sc.data[i];
	vmax = sc.vmax;
}

template <class T> bool stringChrom<T>::equals(mh_solution &orig)
{ 
	// to be efficient: check first objective values
	if (orig.obj()!=obj())
		return false;
	// and now all the genes
	const stringChrom<T> &sc=toSChrom(orig);
	for (int i=0;i<length;i++) 
		if (data[i]!=sc.data[i])
			return false;
	return true;
}

template <class T> double stringChrom<T>::dist(mh_solution &c)
{
	const stringChrom<T> &sc=toSChrom(c);
	int diffs=0;
	for (int i=0;i<length;i++)
		if (data[i]!=sc.data[i])
			diffs++;
	return diffs;
}

template <class T> void stringChrom<T>::initialize(int count)
{
	for (int i=0;i<length;i++) 
		data[i]=random_int(vmax+1);
	invalidate();
}

template <class T> void stringChrom<T>::get_cutpoints(int &a, int &b)
{
	a = random_int(length);
	do
		b = random_int(length);
	while (a==b);
	if (a>b)
		swap(a,b);
}

template <class T> void stringChrom<T>::mutate(int count)
{
	int c;
	if (strmop(pgroup)) c=strmop(pgroup); else c=random_int(1,4);
	switch(c)
	{
		case 1: mutate_flip(count);
			break;

		case 2: mutate_inversion(count);
			break;

		case 3: mutate_exchange(count);
			break;

		case 4: mutate_insertion(count);
			break;

		default: mherror("Wring mutate operator for strings selected"); break;
	}
}

template <class T> void stringChrom<T>::mutate_flip(int count) 
{
	for (int i=0;i<count;i++)
	{
		int genno=random_int(length);
		int r=random_int(0,vmax-1); 
		if (unsigned(r)!=unsigned(data[genno]))
			data[genno]=r;
		else
			data[genno]=vmax;
	}
	invalidate();
}

template <class T> void stringChrom<T>::mutate_inversion(int count)
{
	int c1,c2;
	for (int i=0;i<count;i++)
	{
		get_cutpoints(c1,c2);
		while (c1<c2)
		{
			T t=data[c1];
			data[c1++]=data[c2];
			data[c2--]=t;
		}
	}
	invalidate();
}

template <class T> void stringChrom<T>::mutate_exchange(int count)
{
	int c1,c2;
	for (int i=0;i<count;i++)
	{
		get_cutpoints(c1,c2);
		T t=data[c2];
		data[c2]=data[c1];
		data[c1]=t;
	}
	invalidate();
}

template <class T> void stringChrom<T>::mutate_insertion(int count)
{
	int cs,ci;
	for (int i=0;i<count;i++)
	{
		get_cutpoints(cs,ci);
		T t = data[cs];
		for (int i=cs;i<ci;i++)
			data[i]=data[i+1];
		data[ci]=t;
	}
	invalidate();
}

template <class T> void stringChrom<T>::crossover(const mh_solution &parA,const mh_solution &parB)
{
	int c=strxop(pgroup),pts=strxpts(pgroup);
	if (c==0) 
	{
		int r=random_int(pts+1);
		if (r==0)
			c=1;
		else
		{
			pts=r+1; c=2;
		}
	}
	switch(c)
	{
		case 1: crossover_uniform(parA,parB);
			break;

		case 2: crossover_multipoint(parA,parB,pts);
			break;

		default: 
			mherror("Wrong crossover operator for strings selected");
			break;
	}
}


template <class T> void stringChrom<T>::crossover_uniform(const mh_solution &parA,const mh_solution &parB) 
{
	// uniform crossover
	const stringChrom<T> &a=toSChrom(parA);
	const stringChrom<T> &b=toSChrom(parB);
	for (int i=0;i<length;i++) 
		data[i]=random_bool()?a.data[i]:b.data[i];
	invalidate();
}


template <class T> void stringChrom<T>::crossover_multipoint(const mh_solution &parA,const mh_solution &parB,int xp)
{
	// k point crossover
	const stringChrom<T> &a=toSChrom(parA);
	const stringChrom<T> &b=toSChrom(parB);

	int clength = 0;        // length between 2 crossover points (changes each turn)
	int current = 0;        // current gen to move
	int tf = random_bool(); // take gens from parA or parB

	for (int i=0; i<=xp; i++)
	{
		if (i==xp)
			clength = length-current;
		else
			clength = random_int(length-current-1)-(strxpts(pgroup)-i)+1;

		for (int t=0; t<clength; t++)
		{
			data[current] = tf ? a.data[current] : b.data[current];
			current++;
		}
		tf ? tf=0 : tf=1;
	}
	invalidate();
}

template <class T> void stringChrom<T>::write(ostream &ostr,int detailed) const
{
	for (int i=0;i<length;i++) 
		ostr << int(data[i]) << ' ';
	ostr << endl;
}

template <class T> void stringChrom<T>::save(const char *fname)
{
	ofstream of(fname);
	if (!of)
		mherror("Cannot open file",fname);
	for (int i=0;i<length;i++) 
		of << int(data[i]) << ' ';
	of << endl;
	if (!of)
		mherror("Cannot open file",fname);
}

template <class T> void stringChrom<T>::load(const char *fname)
{
	ifstream inf(fname);
	if (!inf)
		mherror("Cannot open file",fname);
	for (int i=0;i<length;i++) 
	{
		T d;
		inf >> d;
		if (!inf)
			mherror("Cannot open file",fname);
		data[i]=d;
	}
	invalidate();
}

template <class T> unsigned long int stringChrom<T>::hashvalue()
{
	unsigned h=0;
	unsigned window=sizeof(h)*8-unsigned(ceil(log(double(vmax+1))/
		log(2.0)));
	for (int i=0;i<length;i++)
		if (data[i])
			h^=data[i]<<(i%window);
	return h;
}

#endif // MH_STRINGCHROM_IMPL_HXX
