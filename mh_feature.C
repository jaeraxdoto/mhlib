// mh_feature.C

#include "mh_feature.h"

/// Penalty influence tuning parameter for GLS.
double_param glsa("glsa","Penalty influence tuning parameter for GLS.", 0.5, 0, 1, UPPER_INCLUSIVE );
