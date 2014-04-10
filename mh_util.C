// mh_util.C

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <unistd.h>
#include <sys/times.h>
#include "mh_util.h"

void mherror(const char *msg, const char* par1, const char* par2, 
	const char *par3)
{
	cerr << "\n" << msg;
	if (par1) 
	{
		cerr << ": " << par1;
		if (par2)
		{
			cerr << ", " << par2;
			if (par3)
				cerr << ", " << par3;
		}
	}
	else
		cerr << "!";
	cerr << endl;
	abort();
}

double CPUtime()
{
	tms t;
	times(&t);
	double ct=sysconf(_SC_CLK_TCK);
	return t.tms_utime/ct; 
	// return clock()/double(CLOCKS_PER_SEC);
}
