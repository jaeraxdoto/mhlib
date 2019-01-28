/*! \file mh_log.h 
	\brief A generic output stream and a class for generating log files.

	A program should never directly write to cout or a log file but use
	this classes. In this way, the output can be redirected to files
	easily via common parameters. Class logging should be used for
	producing per-iteration log information. It supports a parameter
	controlled, flexible buffering mechanism. The iterations for which
	log information should be actually produced can also be controlled
	via parameters.
 */
	
#ifndef MH_LOG_H
#define MH_LOG_H

#include <iostream>
#include "mh_c11threads.h"
#include "mh_param.h"

namespace mh {

/** \ingroup param
	Default-basename (without extension) for all output files.
	If this name is "@" (which is the default) all output is
	written to the standard output cout and no buffering takes place,
	if this name is "NULL", all output is suppressed. */
extern string_param oname;

/** \ingroup param
	Directory for all output files. If empty the current directory is used*/
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
	The log is generated every lfreq iterations.
	Additionally to all positive values, there are the following
	special values:
	- 0: generate no log.
	- -1: generate log for iterations 0,1,2,5,10,20,50,100,200,... */
extern int_param lfreq;

/** \ingroup param
	Write log entries in dependence of best solution's objective value.
	- 0: Log entries are written at all iterations.
	- 1: Log entries are only written when a new best solution is obtained.
	- 2: Log entries are always written when the objective value changes.
	 */
extern int_param lchonly;

/** \ingroup param
	Flush frequency for the log file.
	The log is actually flushed to the file every lbuffer iterations. */
extern int_param lbuffer;

/** Mutex to be used in multithreading applications for ensuring atomic writing of log entries. */
extern std::mutex logmutex;

/** A class that represents an output stream which is either cout or a
	file, whose name is compiled from the parameters #odir, #oname and
	a given extension. */
class outStream
{
private:
	/** Actual initialization of the stream object. Is called by the constructor. */
	void init(const std::string &fname);
	/** Initializes the special global out and log outStream objects to correspond
	    to the default output and log streams. */
	friend void initOutAndLogstr();
	
public:
	/** Compiles a complete file name for an output stream to be opened from the
	    given directory, base-name and extension, which are typically given by parameters
	    like #odir and #oname. A name is equal to '@' indicates that
	    a reference to cout should be used. This, however, can be overridden by
		parameter atname: If atname is not empty, this default name is used
		instead of name if name is "@". Name "NULL" indicates that
		the null stream should be used. */
	static std::string getFileName(const std::string &ext, const std::string &atname = "",
			const std::string &name = oname(), const std::string &dir = odir());
	/** Generates a stream according to the given directory, name and
		extension. If the name is equal to '@' a reference to
		cout is generated. This behavior, however, can be overridden by
		parameter atname: If atname is not empty, this default name is used
		instead of name if name is "@". If the name is "NULL" a reference to the
		null stream is generated.  */
	outStream(const std::string &fname)
		{ init(fname); }
	/** Destructor.
		Closes stream if not cout. */
	virtual ~outStream();
	/** Access operator for using the stream. */
	std::ostream &operator()()
		{ return *str; }
	/** Returns true if the stream is the standard output. */
	bool isCout() const
		{ return isCoutFlag; }
protected:
	bool isCoutFlag;
	std::ostream *str;
};


/** Class for buffered writing of log information.
	Logs can be written to a file or cout, depending on parameter oname. 
	A log usually has a header (created by headerEntry(), successive
	calls to write(), and finishEntry()) followed by possible entries 
	for each iteration (created by startEntry(), successive calls to
	write(), and finishEntry(). */ 
class logging
{
private:
	/** Init-Method.
	    Does the actual initialization work. Is called by constructor. */
	void init();
	friend void initOutAndLogstr();
	bool previous=false; double prevobj; ///< for shouldWrite().
	int lastflush=0; ///< for finishEntry().
protected:
	/// A  single-linked list struct for buffering the entries.
	struct log_entry
	{
		std::string s;		///< Actual text of log entry.
		log_entry *next;	///< Pointer to next entry.
	};
	/// Pointer to the first and last log entry (buffer)
	log_entry *buffer_first,*buffer_last;
	/// The iteration number of the current entry.
	int curIter;
	/// The string stream for the current log entry.
	std::ostringstream curStream;
public:
	/// The actual output stream. Should only be used directly if none of the buffered operations is used.
	outStream st;
	/** Constructor.
		The default file name is built from the parameters
		odir, oname, and logext. Also the log frequency is given as
		a parameter. */
	logging(const std::string &fname = outStream::getFileName(logext(), "", oname(), odir()))
		: curStream(""), st(fname)
		{ init(); }
	/** Destructor. */
	virtual ~logging();	
	/** This function is supposed to be called to start the log-entry for a 
		iteration. It checks via shouldWrite(), whether a log
		should be generated according to lfreq and lchonly or not. It
		returns true if a log should be generated and writes the
		beginning of the iteration's log-entry (the iteration number
		and the best objective value). If this function returns
		true, the log data must be written by calls to write(),
		and finally finishEntry(). A log entry is generated
		regardless of lfreq and lchonly if inAnyCase is set (e.g.
		at the very beginning of a run or the last iteration). */
	bool startEntry(int gen,double bestobj,bool inAnyCase=false);
	/** Start a first comment line at the beginning of the log.
		This first line should describe all columns. The
		description for the iteration and best fitness is
		automatically inserted, all other descriptions should
		then added by calling write() and finally
		finishEntry() and flush(). */
	void headerEntry();
	/** Check if a log should be generated for the given iteration.
		Is called by shouldWrite().
		If inAnyCase is set, the log is written regardless of
		lfreq and lchonly, except lfreq==0. */
	virtual bool shouldWrite(int gen,double bestobj,bool inAnyCase=false);
	/** Write an int value to the log.
		A separator is inserted in the front of it. */ 
	void write(int val, int w);
	/** Write a double value to the log with a given width and precision.
		A separator is inserted in the front of it. */ 
	void write(double val, int w, int p);
	/** Write a string to the log.
		A separator is inserted in the front of it. */ 
	void write(const std::string &val);
	/** Finish the log for this iteration.
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
	/** Return iteration number from the last log entry started. */
	int lastIter() {
		return curIter;
	}
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

/** Reinitialize out and logstr according to the parameter settings, i.e. possibly to a file. */
void initOutAndLogstr();

} // end of namespace mh

#endif // MH_LOG_H

