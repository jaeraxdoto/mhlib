/*! \file mh_random.h 
	\brief A more reliable random number generator.

	This random number generator should be used instead of the
	standard library random functions. In general, the random number
	generator is very critical in EAs, and therefore a good but also
	fast implementation as this one is needed. */

#ifndef MH_RANDOM_H
#define MH_RANDOM_H

#include <vector>
#include "mh_c11threads.h"
#include "mh_param.h"

namespace mh {

/** \ingroup param
	Seed value for the random number generator.
	The default value of 0 implies an initialization based on the time 
	and process id */
extern int_param seed;	

/**
 * This class implements a more reliable random number generator than the one realized in
 * the standard library.
 */
class mh_randomNumberGenerator {
protected:
	long idum2;
	long iy;
	long *iv;
	long idum;
	unsigned long iseed;

	std::mutex rndmutex;
	std::mutex rndnormalmutex;

	void rndseed(unsigned int seed);

	void bitseed(unsigned int seed);

public:

	/** Constructor, initializes essential variables for the random number generator. */
	mh_randomNumberGenerator();

	/** Destructor, freed memory reserved for the iv array. */
	~mh_randomNumberGenerator() {
		delete[] iv;
	}

	/** Set seed value for the random number generator. If lseed!=0, use this
	    value; otherwise, use the global parameter seed(). If it is also 0,
	    derive a seed value from the current time and pid. */
	void random_seed(unsigned int lseed=0);

	/** Random value (0,1).
		Returns a double random uniformly distributed with
		stdandard deviation 1. */
	double random_double();

	/** Returns random boolean.
		Returns either true or false with equal probability */
	bool random_bool();

	/** returns a double random normally distributed with stdandard deviation 1 */
	double random_normal();

	/** returns a Poisson-distributed random number for a given mu in [0,inf] */
	unsigned int random_poisson(double mu);

	/** A pseudo-random function mapping an unsigned value x to another
	 unsigned value. Returns always the same value when called with the same
	 parameters. Implemented via the pseudo-DES function as described in the book
	 "Numerical Recipes", section 7.5. */
	unsigned random_intfunc(unsigned seed, unsigned x);

	/** A pseudo-random function mapping an unsigned value x to a double value in [0,1).
	  Returns always the same value when called with the same parameters.
	  Implemented via the pseudo-DES function as described in the book "Numerical Recipes",
	  section 7.5. */
	double random_doublefunc(unsigned seed, unsigned x);
};

/**
 * Sets the random number generator object to be used by all successive global random number functions.
 * This function is usually only called by a new thread that should run in a synchronized way, i.e.,
 * its sequence of random numbers should always be the same.
 */
void setRandomNumberGenerator(mh_randomNumberGenerator *);

/**
 * Returns a pointer to the random number generator object to be used. Usually, this is  by the current thread.
 * Usually this is the global default random number generator object.
 * A thread can, however, redirect the pointer to its own random number generator.
 * All calls to random number functions are then redirected to that own object. */
mh_randomNumberGenerator *randomNumberGenerator();

/**
 * Resets the used random number generator to the default random number generator.
 */
void resetRandomNumberGenerator();

// formerly: (does not work with cygwin g++/64
// extern thread_local mh_random_number_generator* randomNumberGenerator;

//* Inline methods calling the respective methods of the default random number generator object *//

/** Set seed value for the random number generator. If lseed!=0, use this    
    value; otherwise, use the global parameter seed(). If it is also 0,
    derive a seed value from the current time and pid. */
inline void random_seed(unsigned int lseed=0) {
	randomNumberGenerator()->random_seed(lseed);
}

/** Random value (0,1).
	Returns a double random uniformly distributed with 
	stdandard deviation 1. */
inline double random_double() {
	return randomNumberGenerator()->random_double();
}

/** Returns true with with given probability. */
inline bool random_prob(double prob) {
	return randomNumberGenerator()->random_double()<=prob;
}

/** Returns random boolean.
	Returns either true or false with equal probability */
inline bool random_bool() {
	return randomNumberGenerator()->random_bool();
}

/** Returns randomly 0 or 1.
	Each value is returned with probability 1/2. */
inline int random_int() {
	return randomNumberGenerator()->random_double() > 0.5 ? 1 : 0; }

/** Returns a random integer in [0,high-1]. */
inline int random_int(int high) {
	return int(randomNumberGenerator()->random_double()*high); }

/** returns an int random number in [low,high] */
inline int random_int(int low, int high) {
	float val=high-low+1;
	val*=randomNumberGenerator()->random_double();
	return int(val)+low;
}

/** returns a double random number uniformly distributed in (low,high) */
inline double random_double(double low, double high) {
	double val=high-low;
	val*=randomNumberGenerator()->random_double();
	return val+low;
}

/** returns a double random normally distributed with stdandard deviation 1 */
inline double random_normal() {
	return randomNumberGenerator()->random_normal();
}

/** returns a normally distributed double random number with given deviation */
inline double random_normal(double dev) {
	return randomNumberGenerator()->random_normal() * dev;
}

/** returns a Poisson-distributed random number for a given mu in [0,inf] */
inline unsigned int random_poisson(double mu) {
	return randomNumberGenerator()->random_poisson(mu);
}

/** returns a Poisson-distributed random number for a given mu in [0,maxi-1] */
inline unsigned int random_poisson(double mu,unsigned maxi) {
	return randomNumberGenerator()->random_poisson(mu) % maxi; }

/** A pseudo-random function mapping an unsigned value x to another
 unsigned value. Returns always the same value when called with the same 
 parameters. Implemented via the pseudo-DES function as described in the book 
 "Numerical Recipes", section 7.5. */
unsigned random_intfunc(unsigned seed, unsigned x);

/** A pseudo-random function mapping an unsigned value x to a double value in [0,1). 
  Returns always the same value when called with the same parameters. 
  Implemented via the pseudo-DES function as described in the book "Numerical Recipes", 
  section 7.5. */
double random_doublefunc(unsigned seed, unsigned x);

/** The elements of a given vector are randomly shuffled.
 * With the parameters, the a subrange can be selected. A negative value
 * for parameter end refers to the last element of the vector. */
template<typename T> void random_shuffle(std::vector<T> &v, int start=0, int end=-1) {
	if (end < 0) end = v.size()-1;
	for (int i=start; i<end; i++) {
		int x=random_int(i,end);
		if (x != i)
			std::swap(v[i],v[x]);
	}
}

} // end of namespace mh

#endif //MH_RANDOM_H

