/*
 * $Source: /cvs/cvsroot/d2x/arch/sdl/timer.c,v $
 * $Revision: 1.3 $
 * $Author: bradleyb $
 * $Date: 2001-10-19 09:45:02 $
 *
 * SDL library timer functions
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2001/01/29 13:35:09  bradleyb
 * Fixed build system, minor fixes
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <SDL/SDL.h>
#include "maths.h"

fix timer_get_fixed_seconds(void) {
  fix x;
  unsigned long tv_now = SDL_GetTicks();
  x=i2f(tv_now/1000) | fixdiv(i2f(tv_now % 1000),i2f(1000));
  return x;
}
