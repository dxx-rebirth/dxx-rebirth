/* $Id: init.c,v 1.1.1.1 2006/03/17 19:53:43 zicodxx Exp $ */
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
	if (SDL_Init(SDL_INIT_VIDEO)<0)
		Error("SDL library initialisation failed: %s.",SDL_GetError());

	key_init();
	joy_init();

	if (!FindArg("-nomouse"))
		d_mouse_init();
	if (!FindArg("-nosound"))
		digi_init();
	atexit(sdl_close);
}
