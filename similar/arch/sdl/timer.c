/*
 *
 * SDL library timer functions
 *
 */

#include <SDL/SDL.h>

#include "maths.h"
#include "timer.h"
#include "config.h"

static fix64 F64_RunTime = 0;

void timer_update(void)
{
	static ubyte init = 1;
	static fix64 last_tv = 0;
	fix64 cur_tv = SDL_GetTicks()*F1_0/1000;

	if (init)
	{
		last_tv = cur_tv;
		init = 0;
	}

	if (last_tv < cur_tv) // in case SDL_GetTicks wraps, don't update and have a little hickup
		F64_RunTime += (cur_tv - last_tv); // increment! this value will overflow long after we are all dead... so why bother checking?
	last_tv = cur_tv;
}

fix64 timer_query(void)
{
	return (F64_RunTime);
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

	while (FrameLoop < 1000u/(GameCfg.VSync?MAXIMUM_FPS:fps))
	{
		u_int32_t tv_now = SDL_GetTicks();
		if (FrameStart > tv_now)
			FrameStart = tv_now;
		if (!GameCfg.VSync)
			SDL_Delay(1);
		FrameLoop=tv_now-FrameStart;
	}

	FrameStart=SDL_GetTicks();
}
