/* $ Id: $ */
/*
 *
 * SDL architecture support
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <SDL/SDL.h>
#include "text.h"
#include "event.h"
#include "error.h"
#include "args.h"
#include "digi.h"

extern void d_mouse_init();

void sdl_close()
{
	SDL_Quit();
}

void arch_sdl_init()
{
#if defined(SDL_VIDEO) || defined(SDL_GL_VIDEO)
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		Error("SDL library video initialisation failed: %s.",SDL_GetError());
	}
#endif
#ifdef SDL_INPUT
	if (!FindArg("-nomouse"))
		d_mouse_init();
#endif
	if (!FindArg("-nosound"))
		digi_init();
	atexit(sdl_close);
}
