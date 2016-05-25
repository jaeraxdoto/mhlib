/*! \file mh_util.h 
	\brief Various utilities functions and declarations. 

	They should be used instead of more primitive functions. */

#ifndef MH_UTIL_H
#define MH_UTIL_H

#include <string>
#include <exception>
#include "mh_param.h"

/** The mhlib has this single namespace containing all its components.
	Only the demo applications have their own namespaces. */
namespace mh {

/** General class for mhlib-specific exceptions. */
class mh_exception : public std::exception {
protected:
	std::string msg; ///< Description for the cause of the exception.
public:
	/** Constructor stores given error message. */
	mh_exception(const std::string &msg) : msg(msg) {}
	/** Destructor. */
	virtual ~mh_exception() {}
	/** Get exception message. */
	virtual const char *what() const noexcept {
		return msg.c_str();
	}
};

/** Writes an error message and exits.
	This function is used to present an error message together with up
	to three optional string arguments on cerr end exit. It should be used
	in case of any abnormal program termination. */
void mherror(const std::string &msg, const std::string &par1 = "", const std::string &par2 = "",
		const std::string &par3 = "");

/** Write given error message to stderr as well as out(), if the latter is associated with a file.
 * Usually called when catching an Exception in the main program e.g. due to mherror. */
void writeErrorMessage(const std::string &msg);

/** Return CPU time in seconds since the process was started. */
double mhcputime();

/** Return the wall clock time in seconds since the process was started. */
double mhwctime();

/** Convert a basic type, e.g., int, into a string. Implemented here as
 * std::to_string is not yet implemented in certain C++ libraries.
 */
template < typename T > std::string tostring( const T& n )
{
    std::ostringstream stm ;
    stm << n ;
    return stm.str() ;
}

/** Returns a string stating the git version of the mhlib source. */
std::string mhversion();

} // end of namespace mh

#endif // MH_UTIL_H

