// mh_param.C - Parameter handling

#include <fstream>
#include <string>
#include <string.h>
#include "mh_param.h"
#include "mh_util.h"

pstring pgroupext( const pstring &pg, const string &n)
{
	if (pg.s=="")
		return (pstring)(n);
	else
		return (pstring)(pg.s+"."+n);
}

//------------------------ param ---------------------------

// pointer to begin of list
param *param::list=0;

param::param(const char *nam,const char *descr,
	const paramValidator *val) :
name(nam), description(descr), validator(val)
{
	// search position
	param *p,*op=0;
	for (p=list;p;op=p,p=p->next)
	{
		int c=strcmp(name,p->name);
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

void param::printAll(ostream &os)
{
	os << "# params:" << endl;
	for (param *p=list;p;p=p->next)
		p->print(os);
}

void param::printAllHelp(ostream &os)
{
	os << "Valid parameters (default values) [valid ranges]:" << endl;
	os << "@\tread parameters from specified file" << endl;
	for (param *p=list;p;p=p->next)
		p->printHelp(os);
}

void param::validate(const string pgroup) const
{
	if (validator) 
		(*validator)(*this,pgroup);
}

void param::printHelp(ostream &os) const
{
	os << name << "\t(" << getStringDefValue() << ") ";
	if (validator) 
		validator->printHelp(os);
	os << description;
	os << endl;
}

void param::print(ostream &os) const
{ 
	os << getName() << '\t' << getStringValue() << endl;
}

void param::parseArgs(int argc, char *argv[])
{
	if (argc>=2 && !strcmp(argv[1],"-h"))
	{
		param::printAllHelp(cerr);
		exit(-1);
	}
	if (argc%2!=1)
		mherror("Uneven number of parameters in command line");
	for (int i=1;i<argc;i+=2)
	{
		setParam(argv[i],argv[i+1]);
	}
}

void param::setParam(const char nam[],const char sval[])
{
	string rnam(nam);
	string pgroup;
	string::size_type pos = rnam.rfind('.');

	if (pos != string::npos)
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
	mherror("Unknown parameter (use -h for a list of possible parameters)"
		,rnam.c_str(),sval);
}

void param::parseFile(const char fname[])
{
	ifstream ifil(fname);
	if (!ifil)
		mherror("Cannot open parameter file",fname);
	char nam[200],val[400];
	ifil >> nam;
	while (!ifil.eof())
	{
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
			setParam(nam,val);
		}
		ifil >> nam;
	}
}

			
//------------------------ paramValidator ---------------------------

void paramValidator::operator()(const param &par,const string pgroup) const
	{ if (!validate(par,pgroup)) error(par,pgroup); }

bool paramValidator::validate(const param &,const string) const
	{ return true; }

void paramValidator::error(const param &par,const string pgroup) const
{ 
	char buf[500];
	if ( pgroup == "" )
		strcpy(buf,par.getName());
	else
	{
		strcpy(buf,pgroup.c_str());
		strcat(buf,".");
		strcat(buf,par.getName());
	}
	strcat(buf,"=\"");
	strcat(buf,par.getStringValue(pgroup));
	strcat(buf,"\"");
	mherror("Invalid param",buf);
}

//------------------------ gen_param<T> ---------------------------



