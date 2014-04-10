/*! \file mh_log.h 
	\brief A generic output stream and a class for generating log files.
	A program should never directly write to cout or a log file but use
	this classes. In this way, the output can be redirected to files
	easily via common parameters. Class logging should be used for
	producing per-generation log information. It supports a parameter
	controlled, flexible buffering mechanism. The generations for which
	log information should be actually produced can also be controlled
	via parameters.

	\todo rework logging output for subalgorithms (e.g. prefix, ...)
 */
	
#ifndef MH_LOG_H
#define MH_LOG_H

#include <iostream>
#include "mh_param.h"

/** \ingroup param
	Default-basename (without extension) for all output files.
	If this name is '@' (which is the default) all output is
	written to the standard output cout and no buffering takes place. */
extern string_param oname;

/** \ingroup param
	Directory for all output files. */
extern string_param odir;

/** \ingroup param
 * The extension for the file if the standard output is redirected with
 * parameter ofile.
 */
extern string_param outext;

/** \ingroup param
	Extension for the log file. */
extern string_param logext;

/** \ingroup param
	Log frequency for writing to the file.
	The log is generated every lfreq generations. 
	Additionally to all positive values, there are the following
	special values:
	- 0: generate no log.
	- -1: generate log for generation 0,1,2,5,10,20,50,100,200,... */
extern int_param lfreq;

/** \ingroup param
	Write log entries only/always when best objective value changes.
	- 1: Log-entries are only written if the objective value
	has changed since the last generation. 
	- 2: Log entries are always written (regardless of lfreq) when the
	  objective value changes. */
extern int_param lchonly;

/** \ingroup param
	Flush frequency for the log file.
	The log is actually flushed to the file every lbuffer generations. */
extern int_param lbuffer;

/** \ingroup param
	Number format.
	Format for printing out double values (e.g. the fitness). */
extern string_param nformat;
	

/** A class that represents a stream which is either simply cout or a 
	file. */
class outStream
{
private:
	/** Init-Method.
	    Does the actual initialization work. Is called by constructor. */
	void init(const string &fext, const string &fname, const string &fdir);
	friend void initOutAndLogstr();
	
public:
	/** Generates a stream according to the given directory, name and
		extension. If the name is equal to '@', a reference to
		cout is simply generated. */
	outStream()
		{ init(logext(),oname(),odir()); }
	
	outStream(const string &fext)
		{ init( fext, oname(), odir() ); }
	
	outStream(const string &fext, const string &fname)
		{ init( fext, fname, odir() ); }
	
	outStream(const string &fext, const string &fname,
		  const string &fdir)
		{ init( fext, fname, fdir ); }
	/** Destructor.
		Closes stream (if not cout). */
	virtual ~outStream();
	/** Access operator for using the stream. */
	ostream &operator()() 
		{ return *str; }
	/** Returns true if the stream is the standard output. */
	bool isCout() const
		{ return isCoutFlag; }
protected:
	bool isCoutFlag;
	ostream *str;
};


/** Class for buffered writing of log information.
	Logs can be written to a file or cout, depending on parameter oname. 
	A log usually has a header (created by headerEntry(), successive
	calls to write(), and finishEntry()) followed by possible entries 
	for each generation (created by startEntry(), successive calls to
	write(), and finishEntry(). */ 
class logging
{
private:
	/** Init-Method.
	    Does the actual initialization work. Is called by constructor. */
	void init();
	friend void initOutAndLogstr();
	
public:
	/** Constructor.
		The default file name is build from the parameters
		odir, oname, and oext. Also the log frequency is given as
		a parameter. */
	logging()
		: st(logext(),oname(),odir()), curStream("")
		{ init(); }
	logging(const string &fext)
		: st(fext,oname(),odir()), curStream("")
		{ init(); }
	logging(const string &fext, const string &fname)
		: st(fext,fname,odir()), curStream("")
		{ init(); }
	logging(const string &fext, const string &fname,
		const string &fdir)
		: st(fext,fname,fdir), curStream("")
		{ init(); }

	/** Destructor. */
	virtual ~logging();	
	/** This function is supposed to be called to start the log-entry for a 
		generation. It checks via shouldWrite(), whether a log
		should be generated according to lfreq and lchonly or not. It
		returns true if a log should be generated and writes the
		beginning of the generation's log-entry (the generation number
		and the best objective value). If this function returns
		true, the log data must be written by calls to write(),
		and finally finishEntry(). A log entry is generated
		regardless of lfreq and lchonly if inAnyCase is set (e.g.
		at the very beginning of a run or the last generation). */ 
	bool startEntry(int gen,double bestobj,bool inAnyCase=false);
	/** Start a first comment line at the beginning of the log.
		This first line should describe all columns. The
		description for the generation and best fitness is
		automatically inserted, all other descriptions should
		then added by calling write() and finally
		finishEntry() and flush(). */
	void headerEntry();
	/** Check if a log should be generated for the given generation.
		Is called by shouldWrite().
		If inAnyCase is set, the log is written regardless of
		lfreq and lchonly, except lfreq==0. */
	virtual bool shouldWrite(int gen,double bestobj,bool inAnyCase=false);
	/** Write an int value to the log.
		A separator is inserted in the front of it. */ 
	void write(int val); 
	/** Write a double value to the log.
		A separator is inserted in the front of it. */ 
	void write(double val);
	/** Write a string to the log.
		A separator is inserted in the front of it. */ 
	void write(const char *val);
	/** Finish the log for this generation.
		The line is inserted into the buffer or actually written to the
		stream (depending on ffreq). */ 
	void finishEntry();
	/** Flush the buffer.
		Write all not yet written log entries from the buffer to the
		stream. */ 
	void flush(); 
	/** Write an empty entry (i.e., an empty line)
	 */
	void emptyEntry();
	/// delimiter for data in a log entry
	static const char delimiter='\t';
protected:
	// The actual output stream.
	outStream st;
	// A  single-linked list struct for buffering the entries
	struct log_entry
	{
		string s;
		log_entry *next;
	};
	// Pointer to the first and last log entry (buffer)
	log_entry *buffer_first,*buffer_last;
	// the generation number of the current entry
	int curGen;
	// the string stream for the current log entry
	ostringstream curStream;
};

/* Global standard output object, initialized to standard output. It is supposed
 * that initOutAndLogstr() is called to reinitialize it according to the parameter
 * settings, i.e. possibly to a file. To use it, never write to cout directly but
 * always to out(). */
extern outStream out;

/** Global logging object, initialized to standard output. It is supposed
 * that initOutAndLogstr() is called to reinitialize it according to the parameter
 * settings, i.e. possibly to a file. */
extern logging logstr;

/** Reinitialize #out and #logstr according to the parameter settings, i.e. possibly to a file. */
void initOutAndLogstr();

#endif // MH_LOG_H

