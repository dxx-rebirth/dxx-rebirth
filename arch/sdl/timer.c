/* $Id: timer.c,v 1.5 2003-01-15 02:42:41 btb Exp $ */
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

fix timer_get_fixed_seconds(void) {
#if 1
	return approx_msec_to_fsec(SDL_GetTicks());
#else
  fix x;
  unsigned long tv_now = SDL_GetTicks();
  x=i2f(tv_now/1000) | fixdiv(i2f(tv_now % 1000),i2f(1000));
  return x;
#endif
}

void timer_delay(fix seconds)
{
#if 1
	SDL_Delay(approx_fsec_to_msec(seconds));
#else
	SDL_Delay(f2i(fixmul(seconds, i2f(1000))));
#endif
}
