/*
 *
 * SDL library timer functions
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <SDL/SDL.h>

#include "maths.h"
#include "timer.h"
#include "config.h"

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

// Replacement for timer_delay which considers calc time the program needs between frames (not reentrant)
void timer_delay2(int fps)
{
	static u_int32_t FrameStart=0;
	u_int32_t FrameLoop=0;

	while (FrameLoop < 1000/(GameCfg.VSync?MAXIMUM_FPS:fps))
	{
		if (!GameCfg.VSync)
			SDL_Delay(1);
		FrameLoop=SDL_GetTicks()-FrameStart;
	}

	FrameStart=SDL_GetTicks();
}
