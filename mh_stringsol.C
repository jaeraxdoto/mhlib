// mh_stringsol.C

#include "mh_stringsol.h"

int_param strxop("strxop","crossover operator for stringSol (0:random 1:uniform 2:multi-point)",
	1,0,50);

int_param strmop("strmop","mutate operator for stringSol (0:random 1:flip 2:inversion 3:exchange 4:insertion)",1,0,50);

int_param strxpts("strxpts","number of x-over points, for k-point crossover",
	1,1,1000);


// instantiate:
template class stringSol<unsigned char>;
template class stringSol<unsigned int>;
