/* $Id: timer.c,v 1.6 2003-02-21 04:08:48 btb Exp $ */
/*
 *
 * SDL library timer functions
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <SDL.h>

#include "maths.h"
#include "timer.h"

fix timer_get_approx_seconds(void)
{
	return approx_msec_to_fsec(SDL_GetTicks());
}

fix timer_get_fixed_seconds(void)
{
	fix x;
	unsigned long tv_now = SDL_GetTicks();
	x=i2f(tv_now/1000) | fixdiv(i2f(tv_now % 1000),i2f(1000));
	return x;
}

void timer_delay(fix seconds)
{
	SDL_Delay(f2i(fixmul(seconds, i2f(1000))));
}
