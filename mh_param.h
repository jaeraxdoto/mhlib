/*! \file mh_param.h 
	\brief Generic parameter handling.

	These classes allow to simply define independent global parameters
	in separate object files by a single-line object definition.
	The parameter values can then be used by the overloaded ()-operator
	(thus, just write "paramname"()).
	Use the typedefs #mh::int_param, #mh::double_param,
	#mh::bool_param, and mh::string_param which instanciate the
	generic class mh::gen_param.  These parameters get their values
	either by a an argument in the command line, from a configuration file 
	(name given as command line argument), or by using a default value. 
	In order to initialize and read command line arguments,
	param::parseArgs() must be called in main(). 
	Giving "-h" as command line argument will print out all existing
	parameters with their default values, ranges and a brief
	explanation. In the argument list parameter names may optionally be prefixed
	by "-" or "--". By parameter "@ <filename>" parameter settings are read from
	the specified file, in which, again, parameter names may be optionally prefixed
	by "-" or "--". When reading in a parameter, "''" is replaced by an empty string.

	To allow fine grained control it is possible to define parameter-groups
	for which each parameter can have an individual value. The name of a
	parameter-group can be passed to the constructor of a class to let the
	created object use this parameter-group instead of the global parameter
	values.
*/

#ifndef MH_PARAM_H
#define MH_PARAM_H

#include <iostream>
#include <unordered_map>
#include <sstream>
#include <string>
#include <functional>

#include "mh_util.h"

/*! mhlib's namespace. */
namespace mh {

class param;

/** Extend the parameter-group pg with n.
    This method prepends the string n to the existing parameter-group p.

    pgroupext( "", "ls" ) => "ls"

    pgroupext( "foo", "bar" ) => "bar.foo" */
inline std::string pgroupext( const std::string &pg, const std::string &n)
{
	if (pg.empty())
		return n;
	else
		return pg + "." + n;
}

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
	param(const std::string &nam,const std::string &descr,
		const paramValidator *val=0);
	/// Write parameter with its value to an ostream. An empty string is printed as ''
	virtual void print(std::ostream &os) const;
	/// Write list of all parameters with their values to an ostream.
	static void printAll(std::ostream &os);
	/// Read value from istream.
	virtual void read(std::istream &os, const std::string pgroup = "")=0;
	/// Get parameter name as string.
	const std::string &getName() const
		{ return name; }
	/// Get parameter value as string.
	virtual std::string getStringValue(const std::string &pgroup = "" ) const =0;
	/// Get default value as string.
	virtual std::string getStringDefValue() const =0;
	/// Checks value with optionally provided validator.
	void validate( const std::string pgroup = "" ) const;
	/** Writes out a one-line help message for the parameter
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
	static void setParam(const std::string &nam,const std::string &sval);
	/// Read a parameter file.
	static void parseFile(const std::string &fname);
	/// Destructor deletes owned validator.
	virtual ~param() 
		{ if (validator) delete validator; }
private:
	// pointer to sorted list of all parameters
	static param *list;	
	// pointer for linear linking of objects
	param *next;
	// name of parameter
	const std::string name;
	// description for parameter
	const std::string description;
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

//--------------------------- uFctValidator ------------------------------

/** Generic unary-function-check validator.
	Used within class param. */
template<class T>
class uFctValidator : public paramValidator
{
public:
	/// Stores a function used for validation.
	uFctValidator(const std::function<bool(T)> &c) : check(c) {}
	/// It is checked if the value conforms to the unary check of the stored function.
	virtual bool validate(const mh::param &par, const std::string pgroup="") const override;
	/// Write out short help for valid values.
	virtual void printHelp(std::ostream &os) const override;
private:
	/// check to perform
	std::function<bool(T)> check;
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
		Used for generating a parameter without a valid
		range of values. 
		\param nam Short name of parameter, corresponding to variable name.
		\param descr Short description (less than one line).
		\param def Default value.
		*/ 
	gen_param(const std::string &nam,const std::string &descr,const T &def) :
			param(nam,descr), value(def), defval(def) 
		{ validate(); }
	/** Register a parameter with a valid range;
		A range check validator is automatically created. 
		\param nam Short name of parameter, corresponding to variable name.
		\param descr Short description (less than one line).
		\param def Default value.
		\param low Lower bound of allowed range of values.
		\param high Upper bound of allowed range of values.
		\param check The kind of bounds, i.e., inclusive or exclusive.
		*/
	gen_param(const std::string &nam,const std::string &descr,const T &def,
			const T low,const T high,const rangecheck check=INCLUSIVE) :
			param(nam,descr,new rangeValidator<T>(low,high,check)), 
			value(def), defval(def)
		{ validate(); }
	/** Register a parameter with an unary check;
		An unary check validator is automatically created. 
		\param nam Short name of parameter, corresponding to variable name.
		\param descr Short description (less than one line).
		\param def Default value.
		\param value Value for unary check, i.e., lower or upper bound.
		\param check Kind of bound specified by value.
		*/
	gen_param(const std::string &nam,const std::string &descr,const T &def,
			const T value,const unarycheck check) :
			param(nam,descr,new unaryValidator<T>(value,check)),
			value(def), defval(def)
		{ validate(); }
	/** Register a parameter with a paramValidator
		\param nam Short name of parameter, corresponding to variable name.
		\param descr Short description (less than one line).
		\param def Default value.
		\param validator The validator to use for checking.
		(Ownership of the allocated object is passed to the callee,
		who will therefore take care of it's deletion.)
		*/
	gen_param(const std::string &nam,const std::string &descr,const T &def,
			const paramValidator* validator) :
			param(nam,descr,validator),
			value(def), defval(def)
		{ validate(); }
	/** Access of a parameter's value without a specified parameter-group.
		Parameter values should be accessed by using this
		operator, therefore by the function call notation. */
	const T operator()() const
		{ return value; }
	/** Access of a parameters value with specified parameter group.
		Parameter values should be accessed by using this 
		operator, therefore by the function call notation. */
	const T operator()( const std::string pgroup) const {
		auto it = qvals.find(pgroup);
		if (it == qvals.end())
			return value;
		else
			return it->second; 
	}
	/// Set a new value and default value for a parameter.
	void setDefault(const T &newval)
		{ defval=value=newval; validate(); }
	/// If you really have to explicitly set the parameter to a value.
	void set(const T &newval, const std::string pgroup = "" ) {
		if ( pgroup.empty() ) {
			value=newval; validate(); }
		else {
			qvals[pgroup] = newval; validate( pgroup ); } }
	/// Determine string representation for value.
	std::string getStringValue( const std::string &pgroup = "" ) const
		{ if ( pgroup.empty() )
			return getStringValue_impl(value);
		else
			return getStringValue_impl((*qvals.find(pgroup)).second); }
	/// Determine string representation for default value.
	std::string getStringDefValue() const
		{ return getStringValue_impl(defval); }
	/// Read value from ostream. Replace '' by an empty string value
	void read(std::istream &is, const std::string pgroup = "") { 
		std::string sval;
		is >> sval;
		// change '' into empty string
		if (sval == "''")
				sval = "";
		std::stringstream sis(sval);
		if ( pgroup.empty() ) {
			sis >> value; validate(); 
		}
		else {
			sis >> qvals[pgroup]; validate( pgroup ); 
		} 
	}
	/// Print value to ostream.
	void print(std::ostream &os) const {
		param::print(os);

		for (auto &it : qvals) {
			std::string v = getStringValue(it.first);
			if (v.empty())
				v = "''";
			os << it.first << "." << getName() << '\t' << v << std::endl;
		}
	}
	
private:
	// the actual parameter value
	T value;
	// the default value
	T defval;
	// the additional qualified parameter values
	std::unordered_map<std::string,T> qvals;
	std::string getStringValue_impl(const T &val) const;
};


// typedefs for using int, double, bool and string parameters in an easy way:

/** A global int parameter. See template class #mh::gen_param for methods. */
typedef gen_param<int> int_param;
/** A global double parameter. See template class #mh::gen_param for methods. */
typedef gen_param<double> double_param;
/** A global bool parameter. See template class #mh::gen_param for methods. */
typedef gen_param<bool> bool_param;
/** A global string parameter. See template class #mh::gen_param for methods. */
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

template <class T>
bool uFctValidator<T>::validate(const mh::param &par, const std::string pgroup) const {
	const gen_param<T> &p=dynamic_cast<const gen_param<T> &>(par);
	return check(p(pgroup));
}

template <class T>
void uFctValidator<T>::printHelp(std::ostream &os) const {
		os << "Unary function validator" << std::endl;
}

template <class T> std::string gen_param<T>::getStringValue_impl(const T &val)
	const
{
	std::ostringstream os("");
	os << val;
	return os.str();
}

} // end of namespace mh

#endif //MH_PARAM_H
