// mh_param.C - Parameter handling

#include <fstream>
#include <string>
#include <string.h>
#include <stdlib.h>
#include "mh_param.h"
#include "mh_util.h"

namespace mh {

using namespace std;



//------------------------ param ---------------------------

// pointer to begin of list
param *param::list=0;

param::param(const std::string &nam,const std::string &descr,
	const paramValidator *val) :
name(nam), description(descr), validator(val)
{
	// search position
	param *p,*op=0;
	for (p=list;p;op=p,p=p->next)
	{
		int c=name.compare(p->name);
		if (c<=0)
		{
			// place found
			if (c==0)
				mherror("Duplicate parameter",name);
			break;
		}

	}
	// insert before *p
	next=p;
	if (op)
		op->next=this;
	else
		list=this;
}

void param::printAll(std::ostream &os)
{
	os << "# params:" << std::endl;
	for (param *p=list;p;p=p->next)
		p->print(os);
}

void param::printAllHelp(std::ostream &os)
{
	os << "Valid parameters (default values) [valid ranges]:" << std::endl;
	os << "@\tread parameters from specified file" << std::endl;
	for (param *p=list;p;p=p->next)
		p->printHelp(os);
}

void param::validate(const std::string pgroup) const
{
	if (validator) 
		(*validator)(*this,pgroup);
}

void param::printHelp(std::ostream &os) const
{
	os << name << "\t(" << getStringDefValue() << ") ";
	if (validator) 
		validator->printHelp(os);
	os << description;
	os << std::endl;
}

void param::print(std::ostream &os) const
{ 
	os << getName() << '\t' << getStringValue() << std::endl;
}

void param::parseArgs(int argc, char *argv[])
{
	if (argc>=2 && !strcmp(argv[1],"-h"))
	{
		param::printAllHelp(cout);
		exit(0);
	}
	if (argc%2!=1)
		mherror("Uneven number of parameters in command line");
	for (int i=1;i<argc;i+=2)
	{
		char *purename=argv[i];
		// skip '-'s at the beginning of parameter names
		while (purename[0]=='-')
			purename++;
		if (purename[0]==0)
			mherror("Empty parameter name:",argv[i]);
		setParam(purename,argv[i+1]);
	}
}

void param::setParam(const std::string &nam,const std::string &sval)
{
	std::string rnam(nam);
	std::string pgroup;
	std::string::size_type pos = rnam.rfind('.');

	if (pos != std::string::npos)
	{
		pgroup = rnam.substr(0,pos);
		rnam.erase(0,pos+1);
	}

	// find parameter
	for (param *p=list;p;p=p->next)
	{
		if (rnam.compare("@")==0)
		{
			parseFile(sval);
			return;
		}
		else if (rnam.compare(p->name)==0)
		{
			string ss(sval);
			istringstream is(ss);
			p->read(is,pgroup);
			char buf[200]; buf[0]='\0';
			is >> buf;
			if (!is.eof() || buf[0]!='\0')
				mherror("Invalid value for parameter",
					rnam.c_str(),sval);
			return;
		}
	}
	mherror("Unknown parameter (use -h for a list of possible parameters)",
		rnam.c_str(),sval);
}

void param::parseFile(const std::string &fname)
{
	std::ifstream ifil(fname);
	if (!ifil)
		mherror("Cannot open parameter file",fname);
	char nam[200],val[400];
	ifil >> nam;
	while (!ifil.eof())
	{
		// skip "--" at the beginning of parameter names
		char *purename = nam;
		while (purename[0]=='-')
			purename++;
		if (purename[0]==0)
			mherror("Empty parameter name:",nam);
		if (nam[0]=='#')	// ignore to end of line
		{
			ifil.getline(val,sizeof(val));
		}
		else
		{
			ifil >> val;
			if (!ifil)
				mherror("Error in reading parameter file",
					fname,nam);
			setParam(purename,val);
		}
		ifil >> nam;
	}
}

			
//------------------------ paramValidator ---------------------------

void paramValidator::operator()(const param &par,const std::string pgroup) const
	{ if (!validate(par,pgroup)) error(par,pgroup); }

bool paramValidator::validate(const param &,const std::string) const
	{ return true; }

void paramValidator::error(const param &par,const std::string pgroup) const
{ 
	char buf[500];
	if ( pgroup.empty() )
		strcpy(buf,par.getName().c_str());
	else
	{
		strcpy(buf,pgroup.c_str());
		strcat(buf,".");
		strcat(buf,par.getName().c_str());
	}
	strcat(buf,"=\"");
	strcat(buf,par.getStringValue(pgroup).c_str());
	strcat(buf,"\"");
	mherror("Invalid value for parameter",buf);
}

//------------------------ gen_param<T> ---------------------------

} // end of namespace mh

