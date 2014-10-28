/*! \file mh_util.h 
	\brief Various utilities functions and declarations. 

	They should be used instead of more primitive functions. */

#ifndef MH_UTIL_H
#define MH_UTIL_H

#include "mh_param.h"

// if flag USE_EXCEPTIONS is given we throw
// an exception instead of aborting the program
#ifdef USE_EXCEPTIONS
# define ERROR(msg) (throw msg)
#else
# define ERROR(msg) \
	std::cerr << msg << std::endl; \
	abort();
#endif

/** Writes an error message and exits.
	This function is used to present an error message together with up
	to three optional string arguments on cerr end exit. It should be used
	in case of any abnormal program termination. */
void mherror(const char *msg, const char* par1=0, const char* par2=0, 
	const char *p3=0);

/** Return CPU time in seconds since the process was started.
	The user CPU time is returned. */
double CPUtime();

#endif // MH_UTIL_H
