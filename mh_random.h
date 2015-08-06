/*! \file mh_random.h 
	\brief A more reliable random number generator.

	This random number generator should be used instead of the
	standard library random functions. In general, the random number
	generator is very critical in EAs, and therefore a good but also
	fast implementation as this one is needed. */

#ifndef MH_RANDOM_H
#define MH_RANDOM_H

#include "mh_param.h"

/** \ingroup param
	Seed value for the random number generator.
	The default value of 0 implies an initialization based on the time 
	and process id */
extern int_param seed;	

/** Set seed value for the random number generator. If lseed!=0, use this    
    value; otherwise, use the global parameter seed(). If it is also 0,
    derive a seed value from the current time and pid. */
void random_seed(unsigned int lseed=0);

/** Random value (0,1).
	Returns a double random uniformly distributed with 
	stdandard deviation 1. */
double random_double();

/** Returns true with with given probability. */
inline bool random_prob(double prob)
{ return random_double()<=prob; }

/** Returns random boolean.
	Returns either true or false with equal probability */
bool random_bool();

/** Returns randomly 0 or 1.
	Each value is returned with probability 1/2. */
inline int random_int()
{ return random_double() > 0.5 ? 1 : 0; }

/** Returns a random integer in [0,high-1]. */
inline int random_int(int high)
{ return int(random_double()*high); }

/// returns an int random number in [low,high]
inline int random_int(int low, int high)
{ 
	float val=high-low+1; 
	val*=random_double(); 
	return int(val)+low;
}

/// returns a double random number uniformly distributed in (low,high)
inline double random_double(double low, double high)
{
	double val=high-low; 
	val*=random_double(); 
	return val+low;
}

/// returns a double random normally distributed with stdandard deviation 1
double random_normal();

/// returns a normally distributed double random number with given deviation
inline double random_normal(double dev)
{ 
	return random_normal() * dev; 
}

/// returns a Poisson-distributed random number for a given mu in [0,inf]
unsigned int random_poisson(double mu);

/// returns a Poisson-distributed random number for a given mu in [0,maxi-1]
inline unsigned int random_poisson(double mu,unsigned maxi)
{ return random_poisson(mu) % maxi; }

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

#endif //MH_UTIL_H

