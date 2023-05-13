/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * See COPYING.txt for license details.
 */

#pragma once
#include <SDL_audio.h>

namespace dcx {

struct RAII_SDL_LockAudio
{
	RAII_SDL_LockAudio()
	{
		SDL_LockAudio();
	}
	~RAII_SDL_LockAudio()
	{
		SDL_UnlockAudio();
	}
	RAII_SDL_LockAudio(const RAII_SDL_LockAudio &) = delete;
	RAII_SDL_LockAudio &operator=(const RAII_SDL_LockAudio &) = delete;
};

}
