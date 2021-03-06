// mh_interfaces.C

#include "mh_guidedls.h"
#include "mh_interfaces.h"

namespace mh {

inline double glsSubAlgorithm::aobj(mh_solution *c)
{
	if (gls!=nullptr)
		return gls->aobj(c);
	else
		return 0.0;
}

inline double glsSubAlgorithm::delta_aobj(mh_solution *c, const nhmove *m)
{
	if (gls!=nullptr)
		return gls->delta_aobj(c,m);
	else
		return 0.0;
}

} // end of namespace mh

