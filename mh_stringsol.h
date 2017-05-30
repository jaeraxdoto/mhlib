/*! \file mh_stringsol.h 
	\brief A generic string solution class for strings of variables having the
	same range of values 0...vmax. 
	*/

#ifndef MH_STRINGSOL_H
#define MH_STRINGSOL_H

#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include "mh_solution.h"
#include "mh_gaopsprov.h"
#include "mh_random.h"
#include "mh_stringsol.h"
#include "mh_util.h"

namespace mh {

/** \ingroup param
	Used crossover operator for string solutions:
	- 0: random choice (uniform, and multipoint with k=1...strxpts() equally
		likely)
	- 1: uniform crossover
	- 2: multi-point crossover. */
extern int_param strxop;

/** \ingroup param
	Number of crossover points in case of multi-point crossover (strxop()==2)
	for string solutions. */
extern int_param strxpts;

/** \ingroup param
	Used mutation operator for string solutions.
	- 0: random choice
	- 1: positional (reset a position to a new random value)
	- 2: inversion
	- 3: reciprocal exchange
	- 4: insertion of one position. */
extern int_param strmop;



/** A solution class for solutions represented by strings of integers of
	the same domain 0...vmax. */
template <class T> class stringSol : public mh_solution, public gaopsProvider
{
protected:
	std::vector<T> data;	/** Actual gene vector. */
	T vmax; 	/** Maximum value. */

	static const stringSol &cast(const mh_solution &ref)
		{ return (dynamic_cast<const stringSol &>(ref)); }

	/** Performs uniform crossover. */
	void crossover_uniform(const mh_solution &parA, const mh_solution &parB);
	/** Performs multi-point crossover with xp crossover points. */
	void crossover_multipoint(const mh_solution &parA,
		const mh_solution &parB, int xp=strxpts());
	/** Calls multi-point crossover, with crossing number 1. */
	void crossover_1point(const mh_solution &parA, const mh_solution &parB)
		{ crossover_multipoint(parA, parB, 1); }
	/** Calls multipoint crossover, with crossing number 2. */
	void crossover_2point(const mh_solution &parA, const mh_solution &parB)
		{ crossover_multipoint(parA, parB, 2); }

	/** Changes one gene to a random number. */
	void mutate_flip(int count);
	/** Performs inversion. */
	void mutate_inversion(int count);
	/** Performs insertion. */
	void mutate_insertion(int count);
	/** Performs reciprocal exchange. */
	void mutate_exchange(int count);

	/** Helper function: Get two cutpoints a and b, a<b. */
	void get_cutpoints(int &a, int &b);

public:
	stringSol(const mh_solution &c);
	/** normal constructor, number of genes must be passed to base
		class, as well as maximum value for each gene. */
	stringSol(int l, int v, const std::string &pg="") : mh_solution(l,pg), data(l)
		{ vmax=v; }
	/** normal constructor, number of genes must be passed to base
		class, as well as maximum value for each gene. */
	stringSol(int l, int v, mh_base *alg, const std::string &pg="") : mh_solution(l,alg,pg), data(l)
		{ vmax=v; }
	/** copy all data from a given solution into the current one. */
	void copy(const mh_solution &orig) override;
	/** return true if the current solution is equal to *orig. */
	virtual bool equals(mh_solution &orig) override;
	/** Returns the Hamming distance. */
	virtual double dist(mh_solution &c) override;
	virtual ~stringSol() { }
	/** Randomly initialize all genes. */
	void initialize(int count) override;
	/** Calls a mutation method, controlled by the parameter strmop(). */
	void mutate(int count) override;
	/** Calls a crossover method, controlled by the parameter strxop(). */
	void crossover(const mh_solution &parA,const mh_solution &parB) override;
	void write(std::ostream &ostr,int detailed=0) override;
	/** Saves the solution to the given file if fname!="NULL". */
	void save(const std::string &fname) override;
	/** Loads the solution from the given file. */
	void load(const std::string &fname) override;
	/** Calculates a hash-value out of the binary string. */
	unsigned long int hashvalue() override;
	/** Returns the gene with given index. */
	virtual T get_gene(int index) const
		{ return data[index]; }
	/** Sets gene with given index to the given value. */
	virtual void set_gene(int index,T v) 
		{ data[index]=v; invalidate(); }
	/** Returns the number of genes. */
	virtual int get_size() const
		{ return data.size(); }
};

/// Unsigned char string solution.
typedef stringSol<unsigned char> charStringSol;

/// Unsigned integer string solution.
typedef stringSol<unsigned int> intStringSol;

//---------------------- Implementation of stringSol -------------------------

template <class T> stringSol<T>::stringSol(const mh_solution &c) : mh_solution(c)
{
	const stringSol<T> &sc=cast(c);
	data=sc.data;
	vmax = sc.vmax;
}

template <class T> void stringSol<T>::copy(const mh_solution &orig)
{
	const stringSol<T> &sc=cast(orig);

	mh_solution::copy(sc);
	data = sc.data;
	vmax = sc.vmax;
}

template <class T> bool stringSol<T>::equals(mh_solution &orig)
{
	// to be efficient: check first objective values
	if (orig.obj()!=obj())
		return false;
	// and now all the genes
	const stringSol<T> &sc=cast(orig);
	for (int i=0;i<length;i++)
		if (data[i]!=sc.data[i])
			return false;
	return true;
}

template <class T> double stringSol<T>::dist(mh_solution &c)
{
	const stringSol<T> &sc=cast(c);
	int diffs=0;
	for (int i=0;i<length;i++)
		if (data[i]!=sc.data[i])
			diffs++;
	return diffs;
}

template <class T> void stringSol<T>::initialize(int count)
{
	for (int i=0;i<length;i++)
		data[i]=random_int(vmax+1);
	invalidate();
}

template <class T> void stringSol<T>::get_cutpoints(int &a, int &b)
{
	a = random_int(length);
	do
		b = random_int(length);
	while (a==b);
	if (a>b)
		std::swap(a,b);
}

template <class T> void stringSol<T>::mutate(int count)
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

		default: mherror("Wrong mutate operator for strings (strmop) selected",tostring(c)); break;
	}
}

template <class T> void stringSol<T>::mutate_flip(int count)
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

template <class T> void stringSol<T>::mutate_inversion(int count)
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

template <class T> void stringSol<T>::mutate_exchange(int count)
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

template <class T> void stringSol<T>::mutate_insertion(int count)
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

template <class T> void stringSol<T>::crossover(const mh_solution &parA, const mh_solution &parB)
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
			mherror("Wrong crossover operator for strings (strxop) selected",tostring(c));
			break;
	}
}


template <class T> void stringSol<T>::crossover_uniform(const mh_solution &parA,const mh_solution &parB)
{
	// uniform crossover
	const stringSol<T> &a=cast(parA);
	const stringSol<T> &b=cast(parB);
	for (int i=0;i<length;i++)
		data[i]=random_bool()?a.data[i]:b.data[i];
	invalidate();
}


template <class T> void stringSol<T>::crossover_multipoint(const mh_solution &parA,const mh_solution &parB,int xp)
{
	// k point crossover
	const stringSol<T> &a=cast(parA);
	const stringSol<T> &b=cast(parB);

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

template <class T> void stringSol<T>::write(std::ostream &ostr,int detailed)
{
	for (int i=0;i<length;i++)
		ostr << int(data[i]) << ' ';
}

template <class T> void stringSol<T>::save(const std::string &fname)
{
	if (fname == "NULL")
		return;
	std::ofstream of(fname);
	if (!of)
		mherror("Cannot open file",fname);
	for (int i=0;i<length;i++)
		of << int(data[i]) << ' ';
	of << std::endl;
	if (!of)
		mherror("Cannot open file",fname);
}

template <class T> void stringSol<T>::load(const std::string &fname)
{
	std::ifstream inf(fname);
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

template <class T> unsigned long int stringSol<T>::hashvalue()
{
	unsigned h=0;
	unsigned window=sizeof(h)*8-unsigned(ceil(log(double(vmax+1))/
		log(2.0)));
	for (int i=0;i<length;i++)
		if (data[i])
			h^=data[i]<<(i%window);
	return h;
}

} // end of namespace mh

#endif // MH_STRINGSOL_H
