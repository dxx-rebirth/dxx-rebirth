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

namespace dsx {

static void arch_close(void)
{
	songs_uninit();

	gr_close();

	if (!CGameArg.CtlNoJoystick)
		joy_close();

	if (!CGameArg.CtlNoMouse)
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
#if SDL_MAJOR_VERSION == 2
	/* In SDL1, grabbing input grabbed both the keyboard and the mouse.
	 * Many game management keys assume a keyboard grab.
	 * Tell SDL2 to grab the keyboard.
	 *
	 * Unlike with SDL1, players have the option of overriding this grab
	 * by setting an environment variable.  In SDL1, the only choice was
	 * to skip both the keyboard grab and the mouse grab.  Now, players
	 * can enable grabbing in the UI, but disable keyboard grab with the
	 * environment variable.
	 */
	SDL_SetHint(SDL_HINT_GRAB_KEYBOARD, "1");
	/* Gameplay continues regardless of focus, so keep the window
	 * visible.
	 */
	SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");
#endif

	key_init();

	digi_select_system();

	if (!CGameArg.SndNoSound)
		digi_init();

	if (!CGameArg.CtlNoMouse)
		mouse_init();

	if (!CGameArg.CtlNoJoystick)
		joy_init();

	if ((t = gr_init()) != 0)
		Error(TXT_CANT_INIT_GFX,t);

	atexit(arch_close);
}

}
