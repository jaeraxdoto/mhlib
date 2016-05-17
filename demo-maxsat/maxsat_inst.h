/*! \file maxsat_inst.h
  \brief A class representing a MAXSAT problem instance.

  This class represents a MAXSAT problem instance.
  \include maxsat_inst.C
 */

#include<string>
#include<vector>

#ifndef MAXSAT_INST_H
#define MAXSAT_INST_H

namespace maxsat {

/**
  This class represents a MAXSAT problem instance.
 */
class MAXSATInst {
public:
	unsigned int nVars=0;	/// The number of binary variables.

	/** A vector of CNF clauses, where one clause is represented as a vector of signed integers.
	 * Each integer denotes a variable occurring in the corresponding clause.
	 * A positive integer v refers to v-th variable,
	 * while a negative integer refers to negated form of the v-th variable.
	 */
	std::vector<std::vector<int>> clauses;

	/** Load an instance from the given file in DMACS CNF form. */
	void load(const std::string &fname);
	/** Write out instance data to stream. */
	void write(std::ostream &ostr, int detailed=0) const;
};

} // end of namespace maxsat


#endif /* MAXSAT_INST_H */
