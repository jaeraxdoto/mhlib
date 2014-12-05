// mh_util.C

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <unistd.h>
#include <sys/times.h>
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


double CPUtime()
{
	tms t;
	times(&t);
	double ct=sysconf(_SC_CLK_TCK);
	return t.tms_utime/ct; 
	// return clock()/double(CLOCKS_PER_SEC);
}
