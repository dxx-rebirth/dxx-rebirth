/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 *
 * SDL library timer functions
 *
 */

#include <SDL.h>

#include "maths.h"
#include "timer.h"
#include "config.h"
#include "multi.h"

static fix64 F64_RunTime = 0;

void timer_update(void)
{
	static bool already_initialized;
	static fix64 last_tv;
	const fix64 cur_tv = static_cast<fix64>(SDL_GetTicks()) * F1_0 / 1000;
	const fix64 prev_tv = last_tv;
	last_tv = cur_tv;

	if (unlikely(!already_initialized))
	{
		already_initialized = true;
	}
	else if (likely(prev_tv < cur_tv)) // in case SDL_GetTicks wraps, don't update and have a little hickup
		F64_RunTime += (cur_tv - prev_tv); // increment! this value will overflow long after we are all dead... so why bother checking?
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
		if (Game_mode & GM_MULTI)
			multi_do_frame(); // during long wait, keep packets flowing
		if (FrameStart > tv_now)
			FrameStart = tv_now;
		if (!GameCfg.VSync)
			SDL_Delay(1);
		FrameLoop=tv_now-FrameStart;
	}

	FrameStart=SDL_GetTicks();
}
