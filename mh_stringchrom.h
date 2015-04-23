/*! \file mh_stringchrom.h 
	\brief A generic string chromosome for strings of genes having the
	same range of values 0...vmax. 
	*/

#ifndef MH_STRINGCHROM_H
#define MH_STRINGCHROM_H

#include <iostream>
#include <vector>
#include "mh_solution.h"
#include "mh_random.h"


/** \ingroup param
	Used crossover operator for string chromosomes:
	- 0: random choice (uniform, and multipoint with k=1...strxpts() equally
		likely)
	- 1: uniform crossover
	- 2: multi-point crossover. */
extern int_param strxop;

/** \ingroup param
	Number of crossover points in case of multi-point crossover (strxop()==2)
	for string chromosomes. */
extern int_param strxpts;

/** \ingroup param
	Used mutation operator for string chromosomes.
	- 0: random choice
	- 1: positional (reset a position to a new random value)
	- 2: inversion
	- 3: reciprocal exchange
	- 4: insertion of one position. */
extern int_param strmop;



/** A chromosome class for solutions represented by strings of integers of
	the same domain 0...vmax. */
template <class T> class stringChrom : public mh_solution
{
protected:
	vector<T> data;	/** Actual gene vector. */
	T vmax; 	/** Maximum value. */

	static const stringChrom &toSChrom(const mh_solution &ref)
		{ return (dynamic_cast<const stringChrom &>(ref)); }

	/** Performs uniform crossover. */
	void crossover_uniform(const mh_solution &parA, const mh_solution &parB);
	/** Performs multi-point crossover with xp crossover points. */
	void crossover_multipoint(const mh_solution &parA, 
		const mh_solution &parB, int xp=strxpts());
	/** Calls multipoint crossover, with crossing number 1. */
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
	stringChrom(const mh_solution &c);
	/** normal constructor, number of genes must be passed to base
		class, as well as maximum value for each gene. */
	stringChrom(int l, int v, mh_base *t, const pstring &pg=(pstring)("")) : mh_solution(l,t,pg), data(l)
		{ vmax=v; }
	stringChrom(int l, int v, const pstring &pg=(pstring)("")) : mh_solution(l,NULL,pg), data(l)
		{ vmax=v; }
	/** copy all data from a given chromosome into the current one. */
	virtual void copy(const mh_solution &orig);
	/** return true if the current chromosome is equal to *orig. */
	virtual bool equals(mh_solution &orig);
	/** Returns the hamming distance. */
	virtual double dist(mh_solution &c);
	virtual ~stringChrom() { }
	/** randomly initialize all genes. */
	void initialize(int count);
	/** Calls a mutation method, controlled by the parameter strmop(). */
	void mutate(int count);
	/** Calls a crossover method, controlled by the parameter strxop(). */
	void crossover(const mh_solution &parA,const mh_solution &parB);
	void write(ostream &ostr,int detailed=0) const;
	void save(const char *fname);
	void load(const char *fname);
	/** Calculates a hash-value out of the binary string. */
	unsigned long int hashvalue();
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

/// Unsigned char string chromosome.
typedef stringChrom<unsigned char> charStringChrom;

/// Unsigned integer string chromosome.
typedef stringChrom<unsigned int> intStringChrom;


#endif //MH_STRINGCHROM_H
