/*
 * Digital audio support
 * Library-independent stub for dynamic selection of sound system
 */

#include "pstypes.h"
#include "error.h"
#include "fix.h"
#include "vecmat.h"
#include "gr.h"
#include "piggy.h"
#include "digi.h"
#include "sounds.h"
#include "wall.h"
#include "newdemo.h"
#include "kconfig.h"
#include "console.h"
#include "rbaudio.h"
#include "jukebox.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <digi.h>
#include <digi_audio.h>

#ifdef USE_SDLMIXER
#include <digi_mixer.h>
#endif

/* Sound system function pointers */

int  (*fptr_init)() = NULL;
void (*fptr_close)() = NULL;
void (*fptr_reset)() = NULL;

void (*fptr_set_channel_volume)(int, int) = NULL;
void (*fptr_set_channel_pan)(int, int) = NULL;

int  (*fptr_start_sound)(short, fix, int, int, int, int, int) = NULL;
void (*fptr_stop_sound)(int) = NULL;
void (*fptr_end_sound)(int) = NULL;

int  (*fptr_find_channel)(int) = NULL;
int  (*fptr_is_sound_playing)(int) = NULL;
int  (*fptr_is_channel_playing)(int) = NULL;
void (*fptr_stop_all_channels)() = NULL;
void (*fptr_set_digi_volume)(int) = NULL;

void digi_select_system(int n) {
	switch (n) {
#ifdef USE_SDLMIXER
	case SDLMIXER_SYSTEM:
	con_printf(CON_NORMAL,"Using SDL_mixer library\n");
	fptr_init = digi_mixer_init;
	fptr_close = digi_mixer_close;
	fptr_reset = digi_mixer_reset;
	fptr_set_channel_volume = digi_mixer_set_channel_volume;
	fptr_set_channel_pan = digi_mixer_set_channel_pan;
	fptr_start_sound = digi_mixer_start_sound;
	fptr_stop_sound = digi_mixer_stop_sound;
	fptr_end_sound = digi_mixer_end_sound;
	fptr_find_channel = digi_mixer_find_channel;
	fptr_is_sound_playing = digi_mixer_is_sound_playing;
	fptr_is_channel_playing = digi_mixer_is_channel_playing;
	fptr_stop_all_channels = digi_mixer_stop_all_channels;
	fptr_set_digi_volume = digi_mixer_set_digi_volume;
	break;
#endif
	case SDLAUDIO_SYSTEM:
	default:
	con_printf(CON_NORMAL,"Using plain old SDL audio\n");
        fptr_init = digi_audio_init;
        fptr_close = digi_audio_close;
        fptr_reset = digi_audio_reset;
        fptr_set_channel_volume = digi_audio_set_channel_volume;
        fptr_set_channel_pan = digi_audio_set_channel_pan;
        fptr_start_sound = digi_audio_start_sound;
        fptr_stop_sound = digi_audio_stop_sound;
        fptr_end_sound = digi_audio_end_sound;
        fptr_find_channel = digi_audio_find_channel;
        fptr_is_sound_playing = digi_audio_is_sound_playing;
        fptr_is_channel_playing = digi_audio_is_channel_playing;
        fptr_stop_all_channels = digi_audio_stop_all_channels;
	fptr_set_digi_volume = digi_audio_set_digi_volume;
 	break;
	}
}

/* Common digi functions */
#ifndef NDEBUG
static int digi_initialised = 0;
#endif
extern int digi_max_channels;
int digi_volume = SOUND_MAX_VOLUME;

void digi_set_volume(int dvolume) {
	digi_volume = dvolume;
	if (fptr_set_digi_volume) digi_set_digi_volume(dvolume);
}

void digi_set_sample_rate(int r) { GameArg.SndDigiSampleRate = r; }

/* Stub functions */

int  digi_init()
{
	digi_init_sounds();
	return fptr_init();
}

void digi_close() { fptr_close(); }
void digi_reset() { fptr_reset(); }

void digi_set_channel_volume(int channel, int volume) { fptr_set_channel_volume(channel, volume); }
void digi_set_channel_pan(int channel, int pan) { fptr_set_channel_pan(channel, pan); }

int  digi_start_sound(short soundnum, fix volume, int pan, int looping, int loop_start, int loop_end, int soundobj) { return fptr_start_sound(soundnum, volume, pan, looping, loop_start, loop_end, soundobj); }
void digi_stop_sound(int channel) { fptr_stop_sound(channel); }
void digi_end_sound(int channel) { fptr_end_sound(channel); }

int  digi_find_channel(int soundno) { return fptr_find_channel(soundno); }
int  digi_is_sound_playing(int soundno) { return fptr_is_sound_playing(soundno); }
int  digi_is_channel_playing(int channel) { return fptr_is_channel_playing(channel); }
void digi_stop_all_channels() { fptr_stop_all_channels(); }
void digi_set_digi_volume(int dvolume) { fptr_set_digi_volume(dvolume); }

#ifndef NDEBUG
void digi_debug()
{
	int i;
	int n_voices = 0;

	if (!digi_initialised) return;

	for (i = 0; i < digi_max_channels; i++)
	{
		if (digi_is_channel_playing(i))
			n_voices++;
        }
}
#endif

#ifdef _WIN32
// Windows native-MIDI stuff.
void digi_win32_set_midi_volume( int mvolume )
{
	int mm_volume;

	if (mvolume < 0)
		midi_volume = 0;
	else if (mvolume > 127)
		midi_volume = 127;
	else
		midi_volume = mvolume;

	// scale up from 0-127 to 0-0xffff
	mm_volume = (midi_volume << 1) | (midi_volume & 1);
	mm_volume |= (mm_volume << 8);

	if (hmp)
		midiOutSetVolume((HMIDIOUT)hmp->hmidi, mm_volume | mm_volume << 16);
}

int digi_win32_play_midi_song( char * filename, char * melodic_bank, char * drum_bank, int loop )
{
	digi_stop_current_song();

	if (filename == NULL)
		return 0;

	if ((hmp = hmp_open(filename)))
	{
		if (hmp_play(hmp,loop) != 0)
			return 0;	// error
		digi_midi_song_playing = 1;
		digi_set_midi_volume(midi_volume);
		return 1;
	}

	return 0;
}

void digi_win32_stop_current_song()
{
	if (digi_midi_song_playing)
	{
		hmp_close(hmp);
		hmp = NULL;
		digi_midi_song_playing = 0;
	}
}
#endif
