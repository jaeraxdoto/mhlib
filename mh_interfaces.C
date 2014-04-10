// mh_interfaces.C

#include "mh_guidedls.h"
#include "mh_interfaces.h"

inline double glsSubAlgorithm::aobj(mh_solution *c)
{
	if (gls!=NULL)
		return gls->aobj(c);
	else
		return 0.0;
}

inline double glsSubAlgorithm::delta_aobj(mh_solution *c, const move *m)
{
	if (gls!=NULL)
		return gls->delta_aobj(c,m);
	else
		return 0.0;
}
