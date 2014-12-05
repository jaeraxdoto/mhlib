/*! \file mh_util.h 
	\brief Various utilities functions and declarations. 

	They should be used instead of more primitive functions. */

#ifndef MH_UTIL_H
#define MH_UTIL_H

#include <string>
#include "mh_param.h"

/** Writes an error message and exits.
	This function is used to present an error message together with up
	to three optional string arguments on cerr end exit. It should be used
	in case of any abnormal program termination. */
void mherror(const std::string &msg, const std::string &par1 = "", const std::string &par2 = "",
		const std::string &par3 = "");

/** Return CPU time in seconds since the process was started.
	The user CPU time is returned. */
double CPUtime();

#endif // MH_UTIL_H
