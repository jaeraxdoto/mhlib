/*! \file mh_move.h 
	\brief Classes related to neighbourhood moves.

	\todo possibly create a moveProvider
 */

#ifndef MH_MOVE_H
#define MH_MOVE_H

/** Abstract move class.
        A concrete class must be derived for a specific
	move in a neighbourhood. */
class move
{
public:
	virtual ~move() {}
};

/** A swapMove class.
        This class represents a swap of two values. */
class swapMove : public move
{
public:
	/// Index of first gene.
	int r;
	/// Index of second gene.
	int s;

	/** Default constructor. */
	swapMove() : r(0), s(0) {}
	/** Another constructor. */
	swapMove( const int _r, const int &_s ) : r(_r), s(_s) {}
};

/** A bitflip move class.
        This class represents a single bitflip. */
class bitflipMove : public move
{
public:
	/// Index of gene.
	int r;
	
	/** Default constructor. */
	bitflipMove() : r(0) {}
	/** Another constructor. */
	bitflipMove( const int _r ) : r(_r) {}
};

/** An exchange move class.
        This class represents an exchange of a value with an other. */
template <class T> class xchgMove : public move
{
public:
	/// Index of gene.
	int r;

	/// Old value.
	T o;

	/// New value.
	T n;

	/** Default constructor. */
	xchgMove() : r(0), o(T()), n(T()) {}
	/** Another constructor. */
	xchgMove( const int _r, const T &_o, const T &_n ) : r(_r), o(_o), n(_n) {}
};

#endif //MH_MOVE_H
