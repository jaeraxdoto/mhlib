// mh_stringchrom_impl.C - string chromosome implementation

#include "mh_stringchrom.h"
#include "mh_stringchrom_impl.h"

int_param strxop("strxop","crossover operator for stringChrom (0:random 1:uniform 2:multi-point)",
	1,0,100);

int_param strmop("strmop","mutate operator for stringChrom (0:random 1:flip 2:inversion 3:exchange 4:insertion)",1,0,100);

int_param strxpts("strxpts","number of x-over points, for k-point crossover",
	1,1,1000);


// instantiate:
template class stringChrom<unsigned char>;
template class stringChrom<unsigned int>;
