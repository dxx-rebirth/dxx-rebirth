/*
 * $Source: /cvs/cvsroot/d2x/arch/sdl/timer.c,v $
 * $Revision: 1.4 $
 * $Author: btb $
 * $Date: 2002-09-01 02:46:06 $
 *
 * SDL library timer functions
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.3  2001/10/19 09:45:02  bradleyb
 * Moved arch/sdl_* to arch/sdl
 *
 * Revision 1.2  2001/01/29 13:35:09  bradleyb
 * Fixed build system, minor fixes
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <SDL/SDL.h>

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
