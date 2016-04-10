// mh_solution.C

#include "mh_solution.h"
#include "mh_base.h"

namespace mh {

using namespace std;

bool_param maxi("maxi","optimization goal 1:maximize, 0:minimize",true);

void mh_solution::setAlgorithm(mh_base *a)
{ alg=a; if (a!=nullptr) pgroup=a->pgroup; }


} // end of namespace mh

