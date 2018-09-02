/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 *
 * Descent random number stuff...
 * rand has different ranges on different machines...
 *
 */

#include <stdlib.h>
#include "maths.h"

namespace dcx {

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

}
