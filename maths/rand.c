/* $Id: rand.c,v 1.4 2004-05-12 07:31:37 btb Exp $ */
/*
 *
 * Descent random number stuff...
 * rand has different ranges on different machines...
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>

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
