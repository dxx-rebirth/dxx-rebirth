/* $Id: timer.c,v 1.1.1.1 2006/03/17 19:53:40 zicodxx Exp $ */
/*
 *
 * SDL library timer functions
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <SDL/SDL.h>

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

// Replacement for timer_delay which considers calc time the program needs between frames (not reentrant)
void timer_delay2(int fps)
{
	static u_int32_t last_render_time=0;

	if (last_render_time > SDL_GetTicks()) // Fallback for SDL_GetTicks() wraparound
		last_render_time = 0;
	else
	{
		int FrameDelay = (1000/fps) // ms to pause between frames for desired FPS rate
				 - (SDL_GetTicks()-last_render_time)/(F1_0/1000) // Substract the time the game needs to do it's operations
				 - 10; // Substract 10ms inaccuracy due to OS scheduling

		if (FrameDelay > 0)
			SDL_Delay(FrameDelay);
	
		last_render_time = SDL_GetTicks();
	}
}
