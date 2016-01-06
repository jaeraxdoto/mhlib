/*! \file mh_param.h 
	\brief Generic parameter handling.

	These classes allow to simply define independent global parameters
	in separate object files by a single-line object definition.
	The parameter values can then be used by the overloaded ()-operator
	(thus, just write "paramname"()).
	Use the typedefs \ref int_param, \ref double_param, \ref bool_param, 
	and \ref string_param which instanciate the generic class gen_param.
	These parameters get their values either by a
	an argument in the command line, from a configuration file 
	(name given as command line argument), or by using a default value. 
	In order to initialize and read command line arguments,
	param::parseArgs() must be called in main(). 
	Giving "-h" as command line argument will print out all existing
	parameters with their default values, ranges and a brief
	explanation.

	To allow fine grained control it is possible to define parametergroups
	for which each parameter can have an individual value. The name of a
	parametergroup can be passed to the constructor of a class to let the
	created object use this parametergroup instead of the global parameter
	values.
*/

#ifndef MH_PARAM_H
#define MH_PARAM_H

#include <iostream>
#include <unordered_map>
#include <sstream>
#include <string>
#include "mh_hash.h"
#include "mh_util.h"

namespace mhlib {

class param;

/**
 * A simple class encapsulating a parameter name.
 */
class pstring
{
public:
	std::string s;

	explicit pstring( const char *_s ) : s(_s) {};
	explicit pstring( const std::string &_s ) : s(_s) {};
};

/** Extend the parametergroup p with n.
    This method prepends the string n to the existing parametergroup p.

    pgroupext( "", "ls" ) == "ls"

    pgroupext( "foo", "bar" ) == "bar.foo" */
pstring pgroupext( const pstring &pg, const std::string &n);

/** Abstract validator object for validating a value to be set for a 
	parameter. 
	Used within param. */
class paramValidator
{
public:
	/// Virtual destructor.
	virtual ~paramValidator() {}
	/** Perform validation.
		Function call operator for actually performing validation
		in case of invalidity, error is called which calls by default
		error will be called. */
	void operator()(const param &par,const std::string pgroup = "") const;
	/** actual validation function; returns true if parameter is okay
		defaults to "everything is valid" */
	virtual bool validate(const param &par, const std::string pgroup = "") const;
	/// called in case of an invalid parameter; calls eaerror
	virtual void error(const param &par, const std::string pgroup = "") const;
	/// write out short help for valid values
	virtual void printHelp(std::ostream &os) const { }
};


//--------------------------- param ------------------------------

/** Abstract base of a configurable parameter. */
class param
{
public:
	/** Register parameter with its name and description.
		An optional validator can be provided. */
	param(const char *nam,const char *descr,
		const paramValidator *val=0);
	/// Write parameter with its value to an ostream.
	virtual void print(std::ostream &os) const;
	/// Write list of all parameters with their values to an ostream.
	static void printAll(std::ostream &os);
	/// Read value from istream.
	virtual void read(std::istream &os, const std::string pgroup = "")=0;
	/// Get parameter name as string.
	const char *getName() const
		{ return name; }
	/// Get parameter value as string.
	virtual std::string getStringValue(const std::string &pgroup = "" ) const =0;
	/// Get default value as string.
	virtual std::string getStringDefValue() const =0;
	/// Checks value with optionally privided validator.
	void validate( const std::string pgroup = "" ) const;
	/** Writes out a helping message for the parameter
		(with description, default value,...). */
	void printHelp(std::ostream &os) const;
	/** Writes out a help message for all registered parameters
		(with description, default value,...). */
	static void printAllHelp(std::ostream &os);
	/** Parse argument vector.
		All existing arguments are supposed to be parameters! */
	static void parseArgs(int argc,char *argv[]);
	/** Set a parameter given by its name to a new value 
		given by a string. */
	static void setParam(const char nam[],const char sval[]);
	/// Read a parameter file.
	static void parseFile(const char fname[]);
	/// Destructor deletes owned validator.
	virtual ~param() 
		{ if (validator) delete validator; }
private:
	// pointer to sorted list of all parameters
	static param *list;	
	// pointer for linear linking of objects
	param *next;
	// name of parameter
	const char *name;
	// description for parameter
	const char *description;
	// validator
	const paramValidator *validator;
};

// output operator for printing the value of a parameter
//ostream & operator<<(ostream &os, const param &p);

// input operator for reading a value for the parameter
//inline istream & operator>>(istream &is, param &p);


//--------------------------- rangeValidator ------------------------------

/** Enumeration of valid range checks. */
enum rangecheck
{
	/** Range includes bounds. */
	INCLUSIVE,
	/** Range excludes bounds. */
	EXCLUSIVE,
	/** Range includes lower bound. */
	LOWER_INCLUSIVE,
	/** Range excludes upper bound. */
	UPPER_EXCLUSIVE,
	/** Range includes upper bound. */
	UPPER_INCLUSIVE,
	/** Range excludes lower bound. */
	LOWER_EXCLUSIVE
};

/** Generic range-check validator for numerical parameters.
        Used within class param. */
template <class T> class rangeValidator : public paramValidator
{
public:
	/// Give upper and lower bound as parameters and type of rangecheck to constructor.
	rangeValidator(T low, T high, rangecheck c) : lbound(low), ubound(high), check(c) {}
	/// It is checked if the value lies in [low,high].
	bool validate(const param &par, const std::string pgroup="") const;
	/// Write out short help for valid values.
	virtual void printHelp(std::ostream &os) const;
private:
	// lower and upper bounds
	const T lbound,ubound;
	/// check to perform
	rangecheck check;
};


//--------------------------- boundValidator ------------------------------

/** Enumeration of valid unary checks. */
enum unarycheck
{
	/** Lower bound. */
	LOWER,
	/** Lower bound (inclusive). */
	LOWER_EQUAL,
	/** Upper bound. */
	UPPER,
	/** Upper bound (inclusive). */
	UPPER_EQUAL,
	/** Not equal. */
	NOT_EQUAL
};

/** Generic unary-check validator for numerical parameters.
	Used within class param. */
template <class T> class unaryValidator : public paramValidator
{
public:
	/// Give a value and type ofcheck as parameters to constructor.
	unaryValidator(T v, unarycheck c) : value(v), check(c) {}
	/// It is checked if the value conforms to the unarycheck.
	bool validate(const param &par, const std::string pgroup="") const;
	/// Write out short help for valid values.
	virtual void printHelp(std::ostream &os) const;
private:
	/// value to check
	const T value;
	/// check to perform
	unarycheck check;
};


//--------------------------- gen_param ------------------------------

/** Configurable parameter of generic type T;
	use the typedefs \ref int_param, \ref double_param, \ref bool_param,
	and \ref string_param for instantiating this generic class for
	the types int, double, bool, and string */
template <class T> class gen_param : public param
{
public:
	/** Register a parameter.
		Most important for generating a parameter (without a valid
		range of values). The description must be only a few words.
		def is the default value. */ 
	gen_param(const char *nam,const char *descr,const T &def) :
		param(nam,descr), value(def), defval(def) 
		{ validate(); }
	/** Register a parameter with a valid range;
		A range check validator is automatically created. */
	gen_param(const char *nam,const char *descr,const T &def,
		const T low,const T high,const rangecheck check=INCLUSIVE) :
		param(nam,descr,new rangeValidator<T>(low,high,check)), 
		value(def), defval(def)
		{ validate(); }
	/** Register a parameter with an unary check;
		An unary check validator is automatically created. */
	gen_param(const char *nam,const char *descr,const T &def,
		const T value,const unarycheck check) :
		param(nam,descr,new unaryValidator<T>(value,check)),
		value(def), defval(def)
		{ validate(); }
	/** Access of a parameters value. 
		Parameter values should be accessed by using this 
		operator, therefore by the function call notation. */
	T operator()( const std::string pgroup = ""  ) const
		{ if (pgroup == "" || qvals.count(pgroup)==0)
			return value;
		else
			return (*qvals.find(pgroup)).second; }
	/// Set a new value and default value for a parameter.
	void setDefault(const T &newval)
		{ defval=value=newval; validate(); }
	/// If you really have to explicitly set the parameter to a value.
	void set(const T &newval, const std::string pgroup = "" ) {
		if ( pgroup == "" ) {
			value=newval; validate(); }
		else {
			qvals[pgroup] = newval; validate( pgroup ); } }
	/// Determine string representation for value.
	std::string getStringValue( const std::string &pgroup = "" ) const
		{ if ( pgroup == "" )
			return getStringValue_impl(value);
		else
			return getStringValue_impl((*qvals.find(pgroup)).second); }
	/// Determine string representation for default value.
	std::string getStringDefValue() const
		{ return getStringValue_impl(defval); }
	/// Read value from ostream.
	void read(std::istream &is, const std::string pgroup = "")
		{ if ( pgroup == "" ) {
			is >> value; validate(); }
		else {
			is >> qvals[pgroup]; validate( pgroup ); } }
	void print(std::ostream &os) const
		{
			std::string i;

			param::print(os);

			typename std::unordered_map<std::string,T,hashstring>::const_iterator it = qvals.begin();

			while (it != qvals.end())
			{
				os << (*it).first << "." << getName() << '\t' << getStringValue((*it).first);
				os << std::endl;
				it++;
			}
		}
	
private:
	// the actual parameter value
	T value;
	// the default value
	T defval;
	// the additional qualified parameter values
	std::unordered_map<std::string,T,hashstring>  qvals;
	std::string getStringValue_impl(const T &val) const;
};

//template<class T> T twice(T t);

// typedefs for using int, double, bool and string parameters in an easy way:

/** A global int parameter.  */
typedef gen_param<int> int_param;
/// A global double parameter.
typedef gen_param<double> double_param;
/// A global bool parameter.
typedef gen_param<bool> bool_param;
/// A global string parameter.
typedef gen_param<std::string> string_param;


//------------------- larger inline functions -------------------------

template <class T> bool rangeValidator<T>::validate(const param &par, const std::string pgroup)
	const
{ 
	const gen_param<T> &p=dynamic_cast<const gen_param<T> &>(par);
	switch (check)
	{
		case EXCLUSIVE:
			return p(pgroup)>lbound && p(pgroup)<ubound;
		case INCLUSIVE:
			return p(pgroup)>=lbound && p(pgroup)<=ubound;
		case LOWER_INCLUSIVE:
		case UPPER_EXCLUSIVE:
			return p(pgroup)>=lbound && p(pgroup)<ubound;
		case UPPER_INCLUSIVE:
		case LOWER_EXCLUSIVE:
			return p(pgroup)>lbound && p(pgroup)<=ubound;
		default:
			return false;
	}
}

template <class T> void rangeValidator<T>::printHelp(std::ostream &os)
	const 
{
	switch (check)
	{
		case EXCLUSIVE:
			os << '(' << lbound << ',' << ubound << ") ";
			break;
		case INCLUSIVE:
			os << '[' << lbound << ',' << ubound << "] ";
			break;
		case LOWER_INCLUSIVE:
		case UPPER_EXCLUSIVE:
			os << '[' << lbound << ',' << ubound << ") ";
			break;
		case UPPER_INCLUSIVE:
		case LOWER_EXCLUSIVE:
			os << '(' << lbound << ',' << ubound << "] ";
			break;
		default:
			break;
	}
}

template <class T> bool unaryValidator<T>::validate(const param &par, const std::string pgroup)
	const
{ 
	const gen_param<T> &p=dynamic_cast<const gen_param<T> &>(par);
	switch (check)
	{
		case LOWER:
			return p(pgroup)>value;
		case LOWER_EQUAL:
			return p(pgroup)>=value;
		case UPPER:
			return p(pgroup)<value;
		case UPPER_EQUAL:
			return p(pgroup)<=value;
		case NOT_EQUAL:
			return p(pgroup)!=value;
		default:
			return false;
	}
}

template <class T> void unaryValidator<T>::printHelp(std::ostream &os)
	const
{
	switch (check)
	{
		case LOWER:
			os << ">" << value << " ";
			break;
		case LOWER_EQUAL:
			os << ">=" << value << " ";
			break;
		case UPPER:
			os << "<" << value << " ";
			break;
		case UPPER_EQUAL:
			os << "<=" << value << " ";
			break;
		case NOT_EQUAL:
			os << "!=" << value << " ";
			break;
		default:
			break;
	}
}

template <class T> std::string gen_param<T>::getStringValue_impl(const T &val)
	const
{
	std::ostringstream os("");
	os << val;
	return os.str();
}

} // end of namespace mhlib

#endif //MH_PARAM_H
