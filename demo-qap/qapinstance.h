/*! \file qapinstance.h
	\brief A class for instances of quadratic assignment problems.

	For each application one global instance of this class is created. */

#ifndef MH_QAPINSTANCE_H
#define MH_QAPINSTANCE_H

#include <string>
#include <vector>
#include "mh_param.h"

namespace qap {

/** \ingroup param
    Filename of the problem instance.
*/
extern mhlib::string_param ifile;


/** A class for instances of quadratic assignment problems. */
class qapInstance
{
private:
	/// Parametergroup.
	std::string pgroup;
	
	static qapInstance *qi;
	
public:
	/// Size of this instance.
	int n;

	/// The distance matrix.
	std::vector<int> a;

	/// The flow matrix.
	std::vector<int> b;

	/// a-index
	std::vector< std::pair<int,int> > indexa;

	/// b-index
	std::vector< std::pair<int,int> > indexb;

	/// cost vector
	std::vector<int> cost;

	/// fd-index vector
	std::vector<int> fdind;


	/** Default Constructor.
		With this the object is not yet fully useable, init() must be called properly.
	*/
	qapInstance() : pgroup(""), n(0), a(0), b(0) {};
	
	/** Constructor.
		The ifile parameter is used as filename of the instance.
	
		\param pg Parametergroup
	*/
	qapInstance( const std::string &pg ) : pgroup(pg), n(0), a(0), b(0)
		{ initialize(ifile(pgroup)); }

	/** Normal Constrcutor.
		The filename of the instance is passed as parameter.
	
		\param fname Filename from where to load the instance
		\param pg Parametergroup
	*/
	qapInstance( const std::string &fname, const std::string &pg="" ) : pgroup(pg), n(0), a(0), b(0)
		{ initialize(fname); };
	
	/** Actual initialization.
		This method loads qap instance in the specified filename.
	
		\param fname Filename from where to load the instance
	*/
	void initialize( const std::string &fname );

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

} // namespace qap

#endif //MH_QAPINSTANCE_H
