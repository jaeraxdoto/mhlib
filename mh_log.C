// mh_log.C

#include <cstdio>
#include <fstream>
#include <iomanip>
#include <string>
#include "mh_log.h"
#include "mh_util.h"

namespace mh {

using namespace std;

string_param oname("oname","base-name for all output files ('@':stdout,'NULL':no output)","@");

string_param odir("odir","directory for all output files","");

string_param outext("outext","extension of stdout file",".out");

string_param logext("logext","extension for log file",".log");

int_param lfreq("lfreq","frequency for writing log entries",1);

int_param lchonly("lchonly","log in dependence of obj. val.: 0:always, 1:only if new best, 2: any change",
		  1,0,2);

int_param lbuffer("lbuffer","number of log entries that are buffered",10,1,
		  10000000);

string_param nformat("nformat","format for writing double values","%f");

mutex logmutex;

/* standard output object, open "stdout" stream (either cout or a file) and
	write out a header including all parameters with
	their values. Never write to cout directly (except
	when temporarily debugging your program), but use
	the configurable outStream class. */
outStream out(outStream::getFileName(".out","@","@",""));
logging logstr(outStream::getFileName(".log","@","@",""));

/* A stream buffer for omitting output. */
class NullBuffer : public std::streambuf
{
public:
  int overflow(int c) { return c; }
};

static NullBuffer null_buffer;

string outStream::getFileName(const string &ext, const string &atname,
		const string &name, const string &dir)
{
	string usename = name;
	if (name == "@") {
		if (atname == "" || atname == "@")
			return usename;
		else
			usename = atname;
	}
	if (usename == "NULL")
		return usename;
	usename += ext;
	if (dir != "")
		usename = dir + "/" + usename;
	return usename;
}


void outStream::init(const string &fname)
{
	if (fname == "@") {
		// use cout as stream
		isCoutFlag = true;
		str = &cout;
	}
	else if (fname == "NULL") {
		// use NullBuffer
		isCoutFlag=true;
		str=new std::ostream(&null_buffer);
		if (!(*str))
			mherror("Cannot open NULL stream for writing");
	}
	else {
		// open file stream
		str = new ofstream(fname);
		if (!(*str))
			mherror("Cannot open file for writing", fname);
	}
}

outStream::~outStream()
{ 
	if (!isCoutFlag) 
		delete str; 
	str=0; 
}


void logging::init()
{
	curIter = 0;
	buffer_first=buffer_last=nullptr;
}

logging::~logging()
{
	flush();
}

void logging::headerEntry()
{
	if (lfreq()!=0)
	{
		// reset curStream
		// curStream.seekp(long(0),ios_base::beg);
		curStream.str("");
		curStream.clear();
		curStream << "iter" << delimiter << "best";
		curIter=0;
	}
}

bool logging::startEntry(int gen, double bestobj,bool inAnyCase)
{
	if (shouldWrite(gen,bestobj,inAnyCase))
	{
		// reset curStream
		// curStream.seekp(long(0),ios_base::beg);
		curStream.str("");
		curStream.clear();
		curIter=gen;
		// write out iteration number
		curStream << setfill('0') << setw(7) << gen;
		// write out best objective value
		write(bestobj);
		return true;
	}
	return false;
}

void logging::emptyEntry()
{
	curStream.str("");
	curStream.clear();
	finishEntry();
}

bool logging::shouldWrite(int gen,double bestobj,bool inAnyCase)
{
	if (lfreq()==0)
		return false;
	if (inAnyCase)
	{
		prevobj=bestobj;
		return true;
	}
	if (lchonly()==2 && (bestobj!=prevobj || !previous))
	{
		prevobj=bestobj;
		previous=true;
		return true;
	}
	if (!previous)
	{
		prevobj=bestobj;
		previous=true;
	}
	else if (lchonly()==1 && bestobj==prevobj)
		return false;
	if (gen==0)
	{
		prevobj=bestobj;
		return true;
	}
	if (lfreq()>0)
	{
		if (gen%lfreq()==0)
		{
			prevobj=bestobj;
			previous=true;
		}
		else
			return false;
	}
	switch (lfreq())
	{
		case -1:
		{
			for (int i=1;i<=gen;i*=10)
				if (gen==i || gen==i*2 || gen==i*5)
				{
					prevobj=bestobj;
					return true;
				}
			return false;
		}
	}
	prevobj=bestobj;
	return true;
}

void logging::write(int val)
{
	curStream << delimiter << val;
} 

void logging::write(double val)
{
	char s[40];
	snprintf(s,sizeof(s),nformat().c_str(),val);
	curStream << delimiter << s;
}

void logging::write(const std::string &val)
{
	curStream << delimiter << val;
}

void logging::finishEntry()
{
	// append curStream to buffer
	log_entry *p=new log_entry;
	p->next=nullptr;
	p->s=curStream.str();
	if (buffer_last)
	{
		buffer_last->next=p;
		buffer_last=p;
	}
	else
		buffer_first=buffer_last=p;
	if (st.isCout() || (curIter-lastflush)>=lbuffer())
	{
		flush();
		lastflush=curIter-curIter%lbuffer();
	}
}

void logging::flush()
{
	log_entry *p=buffer_first;
	while (p)
	{
		st() << p->s << endl;
		log_entry *q=p;
		p=p->next;
		delete q;
	}
	buffer_first=buffer_last=nullptr;
	st().flush();
}

void initOutAndLogstr()
{
	out.init(outStream::getFileName(outext(),"@",oname(),odir()));
	logstr.st.init(outStream::getFileName(logext(),"@",oname(),odir()));
}

} // end of namespace mh

