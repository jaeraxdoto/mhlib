/*! \file qapinstance.h
	\brief A class for instances of quadratic assignment problems.

	For each application one global instance of this class is created. */

#ifndef MH_QAPINSTANCE_H
#define MH_QAPINSTANCE_H

#include <string>
#include <vector>
#include "mh_param.h"

using namespace std;
using namespace mhlib;

/** \ingroup param
    Filename of the quadratic assignment problem instance.
*/
extern string_param qapfile;


/** A class for instances of quadratic assignment problems. */
class qapInstance
{
private:
	/// Parametergroup.
	string pgroup;
	
	static qapInstance *qi;
	
public:
	/// Size of this instance.
	int n;

	/// The distance matrix.
	vector<int> a;

	/// The flow matrix.
	vector<int> b;

	/// a-index
	vector< pair<int,int> > indexa;

	/// b-index
	vector< pair<int,int> > indexb;

	/// cost vector
	vector<int> cost;

	/// fd-index vector
	vector<int> fdind;


	/** Default Constructor.
		With this the object is not yet fully useable, init() must be called properly.
	*/
	qapInstance() : pgroup(""), n(0), a(0), b(0) {};
	
	/** Constructor.
		The qapfile parameter is used as filename of the instance.
	
		\param pg Parametergroup
	*/
	qapInstance( const pstring &pg ) : pgroup(pg.s), n(0), a(0), b(0)
		{ initialize(qapfile(pgroup)); }

	/** Normal Constrcutor.
		The filename of the instance is passed as parameter.
	
		\param fname Filename from where to load the instance
		\param pg Parametergroup
	*/
	qapInstance( const string &fname, const pstring &pg=(pstring)("") ) : pgroup(pg.s), n(0), a(0), b(0)
		{ initialize(fname); };
	
	/** Actual initialization.
		This method loads qap instance in the specified filename.
	
		\param fname Filename from where to load the instance
	*/
	void initialize( const string &fname );

	/** Prepare sorted indices.
		This method prepares the sorted indices used by the greedy construction heuristic.
	*/
	void prepare();
	
	/** Getter method for elements of matrix a.
		This methods provide access with more intuitive indices.
	
		\param i Index i of matrix-element (i,j)
		\param j Index j of matrix-element (i,j)
		\return Value of matrix-element A[i*n+j]
	*/
	int A( int i, int j ) const { return a[i*n+j]; }
	
	/** Getter method for elements of matrix b.
		This methods provide access with more intuitive indices.
	
		\param i Index i of matrix-element (i,j)
		\param j Index j of matrix-element (i,j)
		\return Value of matrix-element B[i*n+j]
		 */
	int B( int i, int j ) const { return b[i*n+j]; }
	
	/** Getter method for qapInstance Singleton. */
	static qapInstance *getInstance();
};

#endif //MH_QAPINSTANCE_H
