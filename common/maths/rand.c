/*
 *
 * Descent random number stuff...
 * rand has different ranges on different machines...
 *
 */

#include <stdlib.h>
#include "maths.h"

#ifdef NO_WATCOM_RAND

void d_srand(unsigned int seed)
{
	srand(seed);
}

int d_rand()
{
	return rand() & 0x7fff;
}

#else

static unsigned int d_rand_seed;

int d_rand()
{
	return ((d_rand_seed = d_rand_seed * 0x41c64e6d + 0x3039) >> 16) & 0x7fff;
}

void d_srand(unsigned int seed)
{
	d_rand_seed = seed;
}

#endif
