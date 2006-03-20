// SDL library timer functions

#include <SDL/SDL.h>
#include "maths.h"

fix timer_get_fixed_seconds(void) {
  fix x;
  unsigned long tv_now = SDL_GetTicks();
  x=i2f(tv_now/1000) | fixdiv(i2f(tv_now % 1000),i2f(1000));
  return x;
}

