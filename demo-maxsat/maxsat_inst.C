/*! \file maxsat_inst.C
    \brief The implementation of the MAXSATInst class, which represents a MAXSAT
    problem instance.
    \include maxsat_inst.C */

#include <fstream>
#include "mh_util.h"

#include "maxsat_sol.h"

using namespace std;
using namespace mh;
using namespace maxsat;

namespace maxsat {

void MAXSATInst::load(const string &fname)
{
	ifstream is(fname,ios::in);
	if (!is)
		mherror("Cannot open problem instance file", fname);

	// file may start with comment lines "c ..."
	string s;
	for (is >> s; !!is && s!="p"; is >> s)
		getline(is,s);
	if (!is)
		mherror("Invalid instance file format, no `p` detected", fname);

	// first important line is "p cnf nVars nClauses"
	int nClauses;
	is >> s >> nVars >> nClauses;
	if (!is || s!="cnf" || nVars < 1 || nVars > 10000000 ||
			nClauses < 1 || nClauses > 10000000)
		mherror("Invalid instance file", fname);
	// now, each line corresponds to a clause, listing the variables
	// negative values indicate negated variables
	for (int i=0; i<nClauses; i++) {
		vector<int> c;	// current clause
		int v;	// current variable
		for (is >> v; !!is && is.peek()==' '; is >> v) {
			if (v<-int(nVars) || v>int(nVars) || v==0)
				mherror("Invalid instance file, variable index out of range", fname);
			c.push_back(v);
			}
		if (!is)
			mherror("Invalid instance file, not all clauses given", fname);
		clauses.push_back(c);
	}
	if (!is)
		mherror("Invalid problem instance file", fname);
}

void MAXSATInst::write(ostream &ostr, int detailed) const {
	ostr << "MAXSAT instance: vars=" << nVars
		<< " clauses=" << clauses.size() << endl;
}

} // maxsat namespace

