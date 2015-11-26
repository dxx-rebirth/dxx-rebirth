/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
// Holds the main init and de-init functions for arch-related program parts

#include <SDL.h>
#include "songs.h"
#include "key.h"
#include "digi.h"
#include "mouse.h"
#include "joy.h"
#include "gr.h"
#include "dxxerror.h"
#include "text.h"
#include "args.h"
#include "window.h"

static void arch_close(void)
{
	songs_uninit();

	gr_close();

	if (!CGameArg.CtlNoJoystick)
		joy_close();

	mouse_close();

	if (!CGameArg.SndNoSound)
	{
		digi_close();
	}
	SDL_Quit();
}

void arch_init(void)
{
	int t;

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		Error("SDL library initialisation failed: %s.",SDL_GetError());

	key_init();

	int system = SDLAUDIO_SYSTEM;
	if (!CGameArg.SndDisableSdlMixer)
		system = SDLMIXER_SYSTEM;
	digi_select_system( system );

	if (!CGameArg.SndNoSound)
		digi_init();

	mouse_init();

	if (!CGameArg.CtlNoJoystick)
		joy_init();

	if ((t = gr_init()) != 0)
		Error(TXT_CANT_INIT_GFX,t);

	atexit(arch_close);
}

