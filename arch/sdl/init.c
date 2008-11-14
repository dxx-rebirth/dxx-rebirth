// Holds the main init and de-init functions for arch-related program parts

#include <SDL/SDL.h>
#include "rbaudio.h"
#include "key.h"
#include "digi.h"
#include "jukebox.h"
#include "mouse.h"
#include "joy.h"
#include "gr.h"
#include "error.h"
#include "text.h"
#include "args.h"

void arch_close(void)
{
	RBAStop();
	RBAExit();

	gr_close();

	if (!GameArg.CtlNoJoystick)
		joy_close();

	if (!GameArg.SndNoSound)
	{
		digi_close();
#ifdef USE_SDLMIXER
		jukebox_unload();
#endif
	}

	key_close();

	SDL_Quit();
}

void arch_init(void)
{
	int t;

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		Error("SDL library initialisation failed: %s.",SDL_GetError());

	key_init();

	digi_select_system( GameArg.SndDisableSdlMixer ? SDLAUDIO_SYSTEM : SDLMIXER_SYSTEM );
	if (!GameArg.SndNoSound)
		digi_init();
	
	if (!GameArg.CtlNoMouse)
		d_mouse_init();

	if (!GameArg.CtlNoJoystick)
		joy_init();

	if ((t = gr_init(0)) != 0)
		Error(TXT_CANT_INIT_GFX,t);

	atexit(arch_close);
}

