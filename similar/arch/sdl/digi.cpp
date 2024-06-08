/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 * Digital audio support
 * Library-independent stub for dynamic selection of sound system
 */

#include <stdexcept>
#include "dxxerror.h"
#include "vecmat.h"
#include "config.h"
#include "digi.h"
#include "sounds.h"
#include "console.h"
#include "rbaudio.h"
#include "args.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <digi.h>
#include <digi_audio.h>

#if DXX_USE_SDLMIXER
#include <digi_mixer.h>
#include <SDL_mixer.h>
#endif
#ifdef _WIN32
#include "hmp.h"
#endif

namespace dcx {

int digi_volume = SOUND_MAX_VOLUME;

/* The values for these three defines are arbitrary and can be changed,
 * provided that they remain unique with respect to each other.
 */
#define DXX_STS_MIXER_WITH_POINTER	0
#define DXX_STS_MIXER_WITH_COPY	1
#define DXX_STS_NO_MIXER	2

#if DXX_USE_SDLMIXER
#ifndef DXX_SOUND_TABLE_STYLE
#ifdef __PIE__
/* PIE -> paranoid checks
 * No PIE -> prefer speed
 */
#define DXX_SOUND_TABLE_STYLE	DXX_STS_MIXER_WITH_POINTER
#else
#define DXX_SOUND_TABLE_STYLE	DXX_STS_MIXER_WITH_COPY
#endif
#endif
#else
#define DXX_SOUND_TABLE_STYLE	DXX_STS_NO_MIXER
#endif

/* Sound system function pointers */

namespace {

struct sound_function_table_t
{
	int  (*init)();
	void (*close)();
	void (*set_channel_volume)(sound_channel, int);
	void (*set_channel_pan)(sound_channel, sound_pan);
	sound_channel (*start_sound)(short, fix, sound_pan, int, int, int, sound_object *);
	void (*stop_sound)(sound_channel);
	void (*end_sound)(sound_channel);
	int  (*is_channel_playing)(sound_channel);
	void (*stop_all_channels)();
	void (*set_digi_volume)(int);
};

#if DXX_SOUND_TABLE_STYLE == DXX_STS_MIXER_WITH_POINTER
[[noreturn]]
__attribute_cold
void report_invalid_table()
{
	/* Out of line due to length of generated code */
	throw std::logic_error("invalid sound table pointer");
}
#endif

}

}

#if DXX_SOUND_TABLE_STYLE != DXX_STS_MIXER_WITH_COPY
namespace dsx
#else
namespace dcx
#endif
{

namespace {

class sound_function_pointers_t
{
#if DXX_SOUND_TABLE_STYLE == DXX_STS_MIXER_WITH_COPY
	sound_function_table_t table;
#elif DXX_SOUND_TABLE_STYLE == DXX_STS_MIXER_WITH_POINTER
	const sound_function_table_t *table;
#endif
public:
	inline const sound_function_table_t *operator->();
	inline sound_function_pointers_t &operator=(const sound_function_table_t &t);
};

#if DXX_SOUND_TABLE_STYLE == DXX_STS_MIXER_WITH_COPY
const sound_function_table_t *sound_function_pointers_t::operator->()
{
	return &table;
}

sound_function_pointers_t &sound_function_pointers_t::operator=(const sound_function_table_t &t)
{
	table = t;
	return *this;
}
#elif DXX_SOUND_TABLE_STYLE == DXX_STS_MIXER_WITH_POINTER
sound_function_pointers_t &sound_function_pointers_t::operator=(const sound_function_table_t &t)
{
	table = &t;
	/* Trap bad initial assignment */
	operator->();
	return *this;
}
#elif DXX_SOUND_TABLE_STYLE == DXX_STS_NO_MIXER
sound_function_pointers_t &sound_function_pointers_t::operator=(const sound_function_table_t &)
{
	return *this;
}
#endif

static sound_function_pointers_t fptr;

}

}

namespace dsx {

namespace {

	/* Some of the functions are in dsx, so the definition and
	 * initializer must be in dsx.
	 */
#if DXX_USE_SDLMIXER
constexpr sound_function_table_t digi_mixer_table{
	&digi_mixer_init,
	&digi_mixer_close,
	&digi_mixer_set_channel_volume,
	&digi_mixer_set_channel_pan,
	&digi_mixer_start_sound,
	&digi_mixer_stop_sound,
	&digi_mixer_end_sound,
	&digi_mixer_is_channel_playing,
	&digi_mixer_stop_all_channels,
	&digi_mixer_set_digi_volume,
};
#endif

constexpr sound_function_table_t digi_audio_table{
	&digi_audio_init,
	&digi_audio_close,
	&digi_audio_set_channel_volume,
	&digi_audio_set_channel_pan,
	&digi_audio_start_sound,
	&digi_audio_stop_sound,
	&digi_audio_end_sound,
	&digi_audio_is_channel_playing,
	&digi_audio_stop_all_channels,
	&digi_audio_set_digi_volume,
};

#if DXX_SOUND_TABLE_STYLE == DXX_STS_MIXER_WITH_POINTER
const sound_function_table_t *sound_function_pointers_t::operator->()
{
	if (table != &digi_audio_table && table != &digi_mixer_table)
		report_invalid_table();
	return table;
}
#elif DXX_SOUND_TABLE_STYLE == DXX_STS_NO_MIXER
const sound_function_table_t *sound_function_pointers_t::operator->()
{
	return &digi_audio_table;
}
#endif

}

void digi_select_system()
{
#if DXX_USE_SDLMIXER
	if (!CGameArg.SndDisableSdlMixer)
	{
		const auto vl = Mix_Linked_Version();
		con_printf(CON_NORMAL, "Using SDL_mixer library v%u.%u.%u", vl->major, vl->minor, vl->patch);
		fptr = digi_mixer_table;
		return;
	}
#endif
	con_puts(CON_NORMAL,"Using plain old SDL audio");
		fptr = digi_audio_table;
}

/* Stub functions */

int  digi_init()
{
	digi_init_sounds();
	return fptr->init();
}

void digi_close() { fptr->close(); }

void digi_set_channel_volume(const sound_channel channel, int volume)
{
	fptr->set_channel_volume(channel, volume);
}

void digi_set_channel_pan(const sound_channel channel, const sound_pan pan)
{
	fptr->set_channel_pan(channel, pan);
}

sound_channel digi_start_sound(const short soundnum, const fix volume, const sound_pan pan, const int looping, const int loop_start, const int loop_end, sound_object *const soundobj)
{
	return fptr->start_sound(soundnum, volume, pan, looping, loop_start, loop_end, soundobj);
}

void digi_stop_sound(const sound_channel channel)
{
	fptr->stop_sound(channel);
}

void digi_end_sound(const sound_channel channel)
{
	fptr->end_sound(channel);
}

int  digi_is_channel_playing(const sound_channel channel)
{
	return fptr->is_channel_playing(channel);
}

void digi_stop_all_channels() { fptr->stop_all_channels(); }
void digi_set_digi_volume(int dvolume) { fptr->set_digi_volume(dvolume); }

}
