// mh_random.C

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <unordered_map>
#include <sstream>
#include <sys/timeb.h>
#include "mh_random.h"
#include "mh_util.h"

#ifdef __WIN32__
#include <process.h>
#define GETPID _getpid
#else
#include <unistd.h>
#define GETPID getpid
#endif

namespace mhlib {

using namespace std;

int_param seed("seed","seed value for the random number generator",0);

static const long IM1=2147483563L;
static const long IM2=2147483399L;
static const double AM=(1.0/IM1);
static const long IMM1=(IM1-1);
static const long IA1=40014L;
static const long IA2=40692L;
static const long IQ1=53668L;
static const long IQ2=52774L;
static const long IR1=12211L;
static const long IR2=3791;
static const int NTAB=32;
static const long NDIV=(1+IMM1/NTAB);
static const double EPS=1.2e-7;
static const double RNMX=(1.0-EPS);

mh_random_number_generator defaultRNG;

thread_local mh_random_number_generator* randomNumberGenerator = &defaultRNG;

#include<iostream>

mh_random_number_generator::mh_random_number_generator(){
	idum2=123456789L;
	iy=0;
	iv = new long[NTAB];
	idum=0;
	iseed = 0;
}

void mh_random_number_generator::random_seed(unsigned int lseed)
{
	rndmutex.lock();
	// when lseed==0 use seed parameter; if also 0 use time & pid
	if (lseed == 0)
	    lseed = unsigned(seed(""));
	if (lseed == 0) 
	{
		while(lseed == 0) 
		{
			unsigned long int tmp;
			unsigned long tim = time(NULL);
			timeb tb;
			ftime(&tb);

			int pid = GETPID();
			tmp=tim*pid;
			for(unsigned int i=0; i<32*sizeof(unsigned int); i++)
				lseed += (tmp & (1 << i));
			lseed+=tb.millitm;
			if (int(lseed) < 0)
				lseed=unsigned(-int(lseed));
		}
		seed.set(int(lseed));
	}
	rndseed(lseed); 
	bitseed(lseed);
	rndmutex.unlock();
}


void mh_random_number_generator::rndseed(unsigned int seed)
{
	int j;
	long k;

	idum = long(seed);
	if (idum < 1) 
		idum=1;
	idum2=(idum);
	for (j=NTAB+7;j>=0;j--) 
	{
		k=(idum)/IQ1;
		idum=IA1*(idum-k*IQ1)-k*IR1;
		if (idum < 0) 
			idum += IM1;
		if (j < NTAB) 
			iv[j] = idum;
	}
	iy=iv[0];
}


double mh_random_number_generator::random_double()
{
	rndmutex.lock();
	int j;
	long k;
	float temp;
	k=idum/IQ1;
	idum=IA1*(idum-k*IQ1)-k*IR1;
	if (idum < 0) 
		idum += IM1;
	k=idum2/IQ2;
	idum2=IA2*(idum2-k*IQ2)-k*IR2;
	if (idum2 < 0) 
		idum2 += IM2;
	j=iy/NDIV;
	iy=iv[j]-idum2;
	iv[j] = idum;
	if (iy < 1) 
		iy += IMM1;
	temp=AM*iy; 
	rndmutex.unlock();
	if (temp > RNMX)
		return RNMX;
	else
		return temp;
}


/*
static const int RSTATELEN=128;
static char rstate[RSTATELEN];
static void rndseed(unsigned int seed) 
{
	initstate(seed,rstate,RSTATELEN);
	setstate(rstate);
}

double random_double() 
{
	return double(random())/(RAND_MAX+1.0);	
}
*/

double mh_random_number_generator::random_normal()
{
	rndnormalmutex.lock();
	static bool cached=false;
	static double cachevalue;
	if(cached)
	{
		cached = false;
		rndnormalmutex.unlock();
		return cachevalue;
	}
	double rsquare, factor, var1, var2;
	do 
	{
	var1 = 2.0 * random_double() - 1.0;
	var2 = 2.0 * random_double() - 1.0;
	rsquare = var1*var1 + var2*var2;
	} while(rsquare >= 1.0 || rsquare == 0.0);
	double val = -2.0 * log(rsquare) / rsquare;
	if(val > 0.0) 
		factor = sqrt(val);
	else 
		factor = 0.0;	// should not happen, but might due to roundoff
	cachevalue = var1 * factor;
	cached = true;
	rndnormalmutex.unlock();
	return (var2 * factor);
}


//---------- a separate, faster random number generator for bits -----------

void mh_random_number_generator::bitseed(unsigned int seed)
{
	iseed = seed;
}

bool mh_random_number_generator::random_bool()
{
	rndmutex.lock();
	// return random_int()?true:false;
	static const int IB1=1;
	static const int IB2=2;
	static const int IB5=16;
	static const int IB18=131072L;
	static const int MASK=IB1+IB2+IB5;
	if (iseed & IB18) 
	{
		iseed=((iseed ^ MASK) << 1) | IB1;
		rndmutex.unlock();
		return true;
	} 
	else 
	{
		iseed <<= 1;
		rndmutex.unlock();
		return false;
	}
}


//------------- for Poisson-distributed random numbers ---------------

/** A class that caches the distribution values of the poisson distribution
for a given mu. */
class poisson_cache
{
	
public:
	const int maxidx;	// highest valid index in maxidx
	double *dens;		// the actual array holding the densities
	poisson_cache(double mu);
	~poisson_cache()
	{
		delete [] dens;
	}
};

poisson_cache::poisson_cache(double mu):
	maxidx(max(12,int(3*mu)))
{
	if (mu>100) {
		stringstream sstr;
		string str;
		sstr << mu;
		sstr >> str;
		mherror("Too large mu for Poisson distribution",
			str.c_str());
	}
	dens=new double[maxidx+1];
	double emu=exp(-mu);
	dens[0]=emu;
	double mk=1;

	for (int k=1;k<maxidx;k++)
	{
		mk*=mu/k;
		dens[k]=dens[k-1]+emu*mk;
	}
	dens[maxidx]=1;
}

unsigned int mh_random_number_generator::random_poisson(double mu)
{
	double r=random_double();

	rndmutex.lock();
	typedef poisson_cache *ppoisson_cache;
	static unordered_map<double,ppoisson_cache> cache(4);

	poisson_cache *pc;
	if (cache.count(mu)==0)
	{
		pc=new poisson_cache(mu);
		cache[mu]=pc;
	}
	else
		pc=cache[mu];
	double *dens=pc->dens;
	int maxidx=pc->maxidx;

	// binary search in the density array:
	int k=int(mu);
	int kl=0,ku=maxidx;
	
	for (;;)
	{
		if (r<=dens[k])
			if (k==0 || r>=dens[k-1])
			{
				rndmutex.unlock();
				return k;
			}
			else
				ku=k-1;
		else
			kl=k+1;
		k=(kl+ku)/2;
	}
	rndmutex.unlock();
	return k;
}

unsigned mh_random_number_generator::random_intfunc(unsigned seed, unsigned x)
//static void psdes(unsigned long *lword, unsigned long *irword)
// Pseudo-DES hashing of the 64-bit word (lword,irword). Both 32-bit arguments
// are returned hashed on all bits.
{
	rndmutex.lock();
	unsigned long i,ia,ib,iswap,itmph=0,itmpl=0;
	static unsigned long c1[4]={
		0xbaa96887L, 0x1e17d32cL, 0x03bcdc3cL, 0x0f33d1b2L};
	static unsigned long c2[4]={
		0x4b0f3b58L, 0xe874f0c3L, 0x6955c5a6L, 0x55a7ca46L};
	for (i=0;i<4;i++) {
		// Perform 4 iterations of DES logic, using a simpler 
		// (non-cryptographic) nonlinear function instead of DES's.
		ia=(iswap=x) ^ c1[i]; 
		itmpl = ia & 0xffff;
		itmph = ia >> 16;
		ib=itmpl*itmpl+ ~(itmph*itmph);
		x=seed ^ (((ia = (ib >> 16) |
		((ib & 0xffff) << 16)) ^ c2[i])+itmpl*itmph);
		seed=iswap;
	}
	rndmutex.unlock();
	return x;
}

// From book "Numerical Recipes":
//float ran4(long *idum)
///* Returns a uniform random deviate in the range 0.0 to 1.0, generated by
//pseudo-DES (DESlike) hashing of the 64-bit word (idums,idum), where idums was
//set by a previous call with negative idum. Also increments idum. Routine can be
//used to generate a random sequence by successive calls, leaving idum unaltered
//between calls; or it can randomly access the nth deviate in a sequence by
//calling with idum = n. Different sequences are initialized by calls with
//differing negative values of idum. */
//{
//	unsigned long irword,itemp,lword;
//	static long idums = 0;
//	// The hexadecimal constants jflone and jflmsk below are 
//	// used to produce a floating number between 1 and 2 by bitwise 
//	// masking. They are machine-dependent. See text.
//	#if defined(vax) || defined(_vax_) || defined(__vax__) || defined(VAX)
//	static unsigned long jflone = 0x00004080;
//	static unsigned long jflmsk = 0xffff007f;
//	#else
//	static unsigned long jflone = 0x3f800000;
//	static unsigned long jflmsk = 0x007fffff;
//	#endif
//	if (*idum < 0) { 		// Reset idums and prepare to return
//		idums = -(*idum); 	// the first deviate in its sequence.
//		*idum=1;
//	}
//	irword=(*idum);
//	lword=idums;
//	random_intfunc(lword,irword);
//	itemp=jflone | (jflmsk & irword); // Mask to a floating number between 1 and 2
//	++(*idum); 
//	return (*(float *)&itemp)-1.0; // Subtraction moves range to 0. to 1.
//}

double mh_random_number_generator::random_doublefunc(unsigned seed, unsigned x)
{
	union
	{
		unsigned long l;
		float f;
	} itemp;
	// unsigned long itemp;
	// The hexadecimal constants jflone and jflmsk below are 
	// used to produce a floating number between 1 and 2 by bitwise 
	// masking. They are machine-dependent. See text.
	//	#if defined(vax) || defined(_vax_) || defined(__vax__) || defined(VAX)
	//	static unsigned long jflone = 0x00004080;
	//	static unsigned long jflmsk = 0xffff007f;
	//	#else
	static unsigned long jflone = 0x3f800000;
	static unsigned long jflmsk = 0x007fffff;
	//	#endif
	x=random_intfunc(seed,x);
	itemp.l=jflone | (jflmsk & x); // Mask to a floating number between 1 and 2
	return itemp.f-1.0;
	//return double(*(float *)&itemp)-1.0;
	//return double(*(float *)&itemp)-1.0; // Subtraction moves range to 0. to 1.
}

void random_resetRNG() {
	randomNumberGenerator = &defaultRNG;
}

} // end of namespace mhlib

