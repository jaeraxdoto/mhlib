// mh_util.C

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sys/time.h>
#include "mh_util.h"

void mherror(const std::string &msg, const std::string &par1, const std::string &par2,
		const std::string &par3)
{
	stringstream ss;
	ss << "\n" << msg;
	if (!par1.empty())
	{
		ss << ": " << par1;

		if (!par2.empty())
		{
			ss << ", " << par2;
			if (!par3.empty())
				ss << ", " << par3;
		}
	}
	else
		ss << "!";
	ss << endl;

	throw ss.str();
}


#if defined(_WIN32)
#include <Windows.h>

#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#include <sys/resource.h>
#include <sys/times.h>
#include <time.h>

#else
#error "Unable to define getCPUTime( ) for an unknown OS."
#endif

double CPUtime( )
{
#if defined(_WIN32)
	/* Windows -------------------------------------------------- */
	FILETIME createTime;
	FILETIME exitTime;
	FILETIME kernelTime;
	FILETIME userTime;
	if ( GetProcessTimes( GetCurrentProcess( ),
		&createTime, &exitTime, &kernelTime, &userTime ) != -1 )
	{
		SYSTEMTIME userSystemTime;
		if ( FileTimeToSystemTime( &userTime, &userSystemTime ) != -1 )
			return (double)userSystemTime.wHour * 3600.0 +
				(double)userSystemTime.wMinute * 60.0 +
				(double)userSystemTime.wSecond +
				(double)userSystemTime.wMilliseconds / 1000.0;
	}

#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
	/* AIX, BSD, Cygwin, HP-UX, Linux, OSX, and Solaris --------- */

#if defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0)
	/* Prefer high-res POSIX timers, when available. */
	{
		clockid_t id;
		struct timespec ts;
#if _POSIX_CPUTIME > 0
		/* Clock ids vary by OS.  Query the id, if possible. */
		if ( clock_getcpuclockid( 0, &id ) == -1 )
#endif
#if defined(CLOCK_PROCESS_CPUTIME_ID)
			/* Use known clock id for AIX, Linux, or Solaris. */
			id = CLOCK_PROCESS_CPUTIME_ID;
#elif defined(CLOCK_VIRTUAL)
			/* Use known clock id for BSD or HP-UX. */
			id = CLOCK_VIRTUAL;
#else
			id = (clockid_t)-1;
#endif
		if ( id != (clockid_t)-1 && clock_gettime( id, &ts ) != -1 )
			return (double)ts.tv_sec +
				(double)ts.tv_nsec / 1000000000.0;
	}
#endif

#if defined(RUSAGE_SELF)
	{
		struct rusage rusage;
		if ( getrusage( RUSAGE_SELF, &rusage ) != -1 )
			return (double)rusage.ru_utime.tv_sec +
				(double)rusage.ru_utime.tv_usec / 1000000.0;
	}
#endif

#if defined(_SC_CLK_TCK)
	{
		const double ticks = (double)sysconf( _SC_CLK_TCK );
		struct tms tms;
		if ( times( &tms ) != (clock_t)-1 )
			return (double)tms.tms_utime / ticks;
	}
#endif

#if defined(CLOCKS_PER_SEC)
	{
		clock_t cl = clock( );
		if ( cl != (clock_t)-1 )
			return (double)cl / (double)CLOCKS_PER_SEC;
	}
#endif

#endif

	mherror("No time function available");		/* Failed. */
	return -1;
}



#ifdef NEVER

#include <unistd.h>
#include <sys/times.h>

// old version just for Linux environments
double CPUtime()
{
	tms t;
	times(&t);
	double ct=sysconf(_SC_CLK_TCK);
	return t.tms_utime/ct;

	//std::cout << "CLOCKS_PER_SEC" << double(CLOCKS_PER_SEC) << double(clock()) << std::endl;
	return clock()/double(CLOCKS_PER_SEC);
}
#endif // NEVER



double WallClockTime() {
    struct timeval time;
    if (gettimeofday(&time,NULL)){
        mherror("Could not obtain wall clock time.");
        return -1;
    }
    return (double)time.tv_sec + (double)time.tv_usec * .000001;
}

string mhversion() {
	return string("mhlib version: ")+string(VERSION);
}
