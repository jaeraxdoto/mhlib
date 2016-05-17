// mh_feature.C

#include "mh_feature.h"

namespace mh {

/// Penalty influence tuning parameter for GLS.
double_param glsa("glsa","GLS penalty influence tuning parameter.", 0.5, 0, 1, UPPER_INCLUSIVE );

} // end of namespace mh

