// mh_advbase.C

#include <stdio.h>
#include <iomanip>
#include "mh_advbase.h"
#include "mh_island.h"
#include "mh_genea.h"
#include "mh_grasp.h"
#include "mh_guidedls.h"
#include "mh_pop.h"
#include "mh_localsearch.h"
#include "mh_simanneal.h"
#include "mh_ssea.h"
#include "mh_tabusearch.h"
#include "mh_util.h"
#include "mh_vnd.h"
#include "mh_vns.h"

namespace mh {

int_param mhalg("mhalg","algorithm to use 0:ss 1:gen 2:ss-isl 3:gen-isl 4:ls 5:sa 6:ts 7:grasp 8:gls 9:vns 10:vnd",
        0,0,10);

mh_advbase *create_mh(pop_base &p,int a, const std::string &pg)
{
	mh_advbase *ea=nullptr;
	switch (a)
	{
		case 0:
			ea = new steadyStateEA(p,pg);
			break;
		case 1:
			ea = new generationalEA(p,pg);
			break;
		case 2:
			ea = new islandModelEA(p,new steadyStateEA(),pg);
			break;
		case 3:
			ea = new islandModelEA(p,new generationalEA(),pg);
			break;
		case 4:
			ea = new localSearch(p,pg);
			break;
		case 5:
			ea = new simulatedAnnealing(p,pg);
			break;
		case 6:
			ea = new tabuSearch(p,pg);
			break;
		case 7:
			ea = new GRASP(p,pg);
			break;
		case 8:
			ea = new guidedLS(p,pg);
			break;
		case 9:
			ea = new VNS(p,pg);
			break;
		case 10:
			ea = new VND(p,pg);
			break;
		default:
			mherror("Invalid parameter for mhalg()",
				mhalg.getStringValue(pg).c_str());
			return nullptr;
	}
	return ea;
}

} // end of namespace mh


