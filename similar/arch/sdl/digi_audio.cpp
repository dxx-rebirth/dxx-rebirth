/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 *
 * SDL digital audio support
 *
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <SDL.h>
#include <digi_audio.h>
#include "dxxerror.h"
#include "fmtcheck.h"
#include "vecmat.h"
#include "digi.h"
#include "sounds.h"
#include "config.h"
#include "args.h"
#include "piggy.h"

#include "compiler-range_for.h"
#include "d_sdl_audio.h"
#include "d_underlying_value.h"

namespace dcx {

namespace {

/* This table is used to add two sound values together and pin
 * the value to avoid overflow.  (used with permission from ARDI)
 * DPH: Taken from SDL/src/SDL_mixer.c.
 */
constexpr uint8_t mix8[] =
{
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03,
  0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
  0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
  0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24,
  0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
  0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A,
  0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45,
  0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50,
  0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B,
  0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
  0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71,
  0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C,
  0x7D, 0x7E, 0x7F, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
  0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x90, 0x91, 0x92,
  0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D,
  0x9E, 0x9F, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8,
  0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3,
  0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE,
  0xBF, 0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9,
  0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0, 0xD1, 0xD2, 0xD3, 0xD4,
  0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
  0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA,
  0xEB, 0xEC, 0xED, 0xEE, 0xEF, 0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5,
  0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

static int digi_initialised = 0;

struct sound_slot {
	sound_effect soundno;
	bool playing;   // Is there a sample playing on this channel?
	bool looped;    // Play this sample looped?
	bool persistent; // This can't be pre-empted
	sound_pan pan;       // 0 = far left, 1 = far right
	fix volume;    // 0 = nothing, 1 = fully on
	//changed on 980905 by adb from char * to unsigned char *
	std::span<const uint8_t> samples;
	sound_object *soundobj;   // Which soundobject is on this channel
	//end changes by adb
	unsigned int position; // Position we are at at the moment.
};

static void digi_audio_stop_sound(sound_slot &s)
{
	s.playing = 0;
	s.soundobj = sound_object_none;
	s.persistent = 0;
}

static enumerated_array<sound_slot, 32, sound_channel> SoundSlots;
static SDL_AudioSpec WaveSpec;
static sound_channel next_channel;

/* Return the next sound_channel after `c`, and roll back to 0 if incrementing
 * `c` exceeds the maximum available channels.
 */
static sound_channel next(const sound_channel c)
{
	const std::underlying_type<sound_channel>::type v = underlying_value(c) + 1;
	return (v >= digi_max_channels) ? sound_channel{} : sound_channel{v};
}

}

}

namespace dsx {

namespace {

/* Audio mixing callback */
//changed on 980905 by adb to cleanup, add pan support and optimize mixer
static void audio_mixcallback(void *, Uint8 *stream, int len)
{
	Uint8 *streamend = stream + len;
	if (!digi_initialised)
		return;

	memset(stream, 0x80, len); // fix "static" sound bug on Mac OS X

	RAII_SDL_LockAudio lock_audio{};

	range_for (auto &sl, SoundSlots)
	{
		if (sl.playing) {
			auto sldata = std::next(sl.samples.begin(), sl.position), slend = sl.samples.end();
			Uint8 *sp = stream, s;
			signed char v;
			fix vl, vr;

			if (const auto x = static_cast<fix>(sl.pan); x & 0x8000) {
				vl = 0x20000 - x * 2;
				vr = 0x10000;
			} else {
				vl = 0x10000;
				vr = x * 2;
			}
			const auto sl_volume{sl.volume};
			vl = fixmul(vl, sl_volume);
			vr = fixmul(vr, sl_volume);
			while (sp < streamend) {
				if (sldata == slend) {
					if (!sl.looped) {
						sl.playing = 0;
						break;
					}
					sldata = sl.samples.begin();
				}
				v = *(sldata++) - 0x80;
				s = *sp;
				*(sp++) = mix8[ s + fixmul(v, vl) + 0x80 ];
				s = *sp;
				*(sp++) = mix8[ s + fixmul(v, vr) + 0x80 ];
			}
			sl.position = std::distance(sl.samples.begin(), sldata);
		}
	}
}

}
//end changes by adb

/* Initialise audio devices. */
int digi_audio_init()
{
	if (SDL_InitSubSystem(SDL_INIT_AUDIO)<0) {
		Error("SDL audio initialisation failed: %s.",SDL_GetError());
	}

#if DXX_BUILD_DESCENT == 1
	/* Descent 1 sounds are always 11Khz. */
	WaveSpec.freq = underlying_value(sound_sample_rate::_11k);
#elif DXX_BUILD_DESCENT == 2
	/* Descent 2 sounds are available in both 11Khz and 22Khz.  The user may
	 * pick at program start time which to use.
	 */
	WaveSpec.freq = underlying_value(GameArg.SndDigiSampleRate);
#endif
	//added/changed by Sam Lantinga on 12/01/98 for new SDL version
	WaveSpec.format = AUDIO_U8;
	WaveSpec.channels = 2;
	//end this section addition/change - SL
	WaveSpec.samples = /* SOUND_BUFFER_SIZE = */ 1024;
	WaveSpec.callback = audio_mixcallback;

	if ( SDL_OpenAudio(&WaveSpec, NULL) < 0 ) {
		//edited on 10/05/98 by Matt Mueller - should keep running, just with no sound.
		Warning("Failed to open audio: %s", SDL_GetError());
		//killed  exit(2);
		return 1;
		//end edit -MM
	}
	SDL_PauseAudio(0);

	digi_initialised = 1;

	digi_audio_set_digi_volume((CGameCfg.DigiVolume * 32768) / 8);
	return 0;
}

/* Shut down audio */
void digi_audio_close()
{
	if (!digi_initialised) return;
	digi_initialised = 0;
#ifdef __MINGW32__
	SDL_Delay(500); // CloseAudio hangs if it's called too soon after opening?
#endif
	SDL_CloseAudio();
}

void digi_audio_stop_all_channels()
{
	range_for (auto &i, SoundSlots)
		digi_audio_stop_sound(i);
}


// Volume 0-F1_0
sound_channel digi_audio_start_sound(sound_effect soundnum, fix volume, sound_pan pan, int looping, int, int, sound_object *const soundobj)
{
	if (!digi_initialised)
		return sound_channel::None;

	if (soundnum == sound_effect::None)
		return sound_channel::None;

	RAII_SDL_LockAudio lock_audio{};

	const auto starting_channel{next_channel};

	while(1)
	{
		if (!SoundSlots[next_channel].playing)
			break;

		if (!SoundSlots[next_channel].persistent)
			break;	// use this channel!

		next_channel = next(next_channel);
		if (next_channel == starting_channel)
		{
			return sound_channel::None;
		}
	}
	if (SoundSlots[next_channel].playing)
	{
		SoundSlots[next_channel].playing = 0;
		if (SoundSlots[next_channel].soundobj != sound_object_none)
		{
			digi_end_soundobj(*SoundSlots[next_channel].soundobj);
		}
		if (SoundQ_channel == next_channel)
			SoundQ_end();
	}

#ifndef NDEBUG
	verify_sound_channel_free(next_channel);
#endif

	SoundSlots[next_channel].soundno = soundnum;
	SoundSlots[next_channel].samples = GameSounds[soundnum].span();
	SoundSlots[next_channel].volume = fixmul(digi_volume, volume);
	SoundSlots[next_channel].pan = pan;
	SoundSlots[next_channel].position = 0;
	SoundSlots[next_channel].looped = looping;
	SoundSlots[next_channel].playing = 1;
	SoundSlots[next_channel].soundobj = soundobj;
	SoundSlots[next_channel].persistent = 0;
	if (soundobj || looping || volume > F1_0)
		SoundSlots[next_channel].persistent = 1;

	const auto i{next_channel};
	next_channel = next(next_channel);

	return i;
}

//added on 980905 by adb from original source to make sfx volume work
void digi_audio_set_digi_volume( int dvolume )
{
	dvolume = fixmuldiv( dvolume, SOUND_MAX_VOLUME, 0x7fff);
	if ( dvolume > SOUND_MAX_VOLUME )
		digi_volume = SOUND_MAX_VOLUME;
	else if ( dvolume < 0 )
		digi_volume = 0;
	else
		digi_volume = dvolume;

	if ( !digi_initialised ) return;

	digi_sync_sounds();
}
//end edit by adb

int digi_audio_is_channel_playing(const sound_channel channel)
{
	if (!digi_initialised)
		return 0;

	return SoundSlots[channel].playing;
}

void digi_audio_set_channel_volume(const sound_channel channel, int volume)
{
	if (!digi_initialised)
		return;

	if (!SoundSlots[channel].playing)
		return;

	SoundSlots[channel].volume = fixmuldiv(volume, digi_volume, F1_0);
}

void digi_audio_set_channel_pan(const sound_channel channel, const sound_pan pan)
{
	if (!digi_initialised)
		return;

	if (!SoundSlots[channel].playing)
		return;

	SoundSlots[channel].pan = pan;
}

void digi_audio_stop_sound(const sound_channel channel)
{
	digi_audio_stop_sound(SoundSlots[channel]);
}

void digi_audio_end_sound(const sound_channel channel)
{
	if (!digi_initialised)
		return;

	if (!SoundSlots[channel].playing)
		return;

	SoundSlots[channel].soundobj = sound_object_none;
	SoundSlots[channel].persistent = 0;
}

}
