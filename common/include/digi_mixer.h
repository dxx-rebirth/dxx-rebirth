/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#ifdef DXX_BUILD_DESCENT
#include "maths.h"
#include <SDL.h>
#include "digi_audio.h"

#ifndef DXX_FEATURE_EXTERNAL_RESAMPLER_SDL_NATIVE
#define DXX_FEATURE_EXTERNAL_RESAMPLER_SDL_NATIVE	1
#endif

#ifndef DXX_FEATURE_INTERNAL_RESAMPLER_EMULATE_SDL1
#define DXX_FEATURE_INTERNAL_RESAMPLER_EMULATE_SDL1	(SDL_MAJOR_VERSION == 2)
#endif

#ifndef DXX_FEATURE_INTERNAL_RESAMPLER_EMULATE_SOUNDBLASTER16
#define DXX_FEATURE_INTERNAL_RESAMPLER_EMULATE_SOUNDBLASTER16	1
#endif

#if !(DXX_FEATURE_EXTERNAL_RESAMPLER_SDL_NATIVE ||	\
		DXX_FEATURE_INTERNAL_RESAMPLER_EMULATE_SDL1 ||	\
		DXX_FEATURE_INTERNAL_RESAMPLER_EMULATE_SOUNDBLASTER16	\
		)
#error "Rebirth with SDL_mixer must enable at least one resampler.  Disable SDL_mixer or enable a resampler."
#endif

namespace dcx {

enum class sound_pan : int;
struct sound_object;

enum class digi_mixer_method : uint8_t
{
#if DXX_FEATURE_EXTERNAL_RESAMPLER_SDL_NATIVE
	/* Delegate resampling to SDL. */
	sdl_native,
#endif
#if DXX_FEATURE_INTERNAL_RESAMPLER_EMULATE_SDL1
	/* Use an internal resampler designed to produce the same results as SDL1's
	 * native resampler.  Use this when you use SDL2 for other processing, but
	 * prefer the results of the SDL1 sound resampler.
	 */
	emulate_sdl1,
#endif
#if DXX_FEATURE_INTERNAL_RESAMPLER_EMULATE_SOUNDBLASTER16
	/* Use an internal resampler that attempts to mimic the SoundBlaster16
	 * behavior.
	 *
	 * Note that some users have reported distortion of concurrent effects with
	 * this resampler.
	 */
	emulate_soundblaster16,
#endif
};

void digi_mixer_close();
void digi_mixer_set_channel_volume(sound_channel, int);
void digi_mixer_set_channel_pan(sound_channel, sound_pan);
void digi_mixer_stop_sound(sound_channel);
void digi_mixer_end_sound(sound_channel);
void digi_mixer_set_digi_volume(int);
int digi_mixer_is_channel_playing(sound_channel);
void digi_mixer_stop_all_channels();
int digi_mixer_init();
}
namespace dsx {
sound_channel digi_mixer_start_sound(short, fix, sound_pan, int, int, int, sound_object *);
}
#endif
