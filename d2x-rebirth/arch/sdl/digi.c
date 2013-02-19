/*
 * Digital audio support
 * Library-independent stub for dynamic selection of sound system
 */

#include "pstypes.h"
#include "dxxerror.h"
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
#include "args.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <digi.h>
#include <digi_audio.h>

#ifdef USE_SDLMIXER
#include <digi_mixer.h>
#endif
#ifdef _WIN32
#include "hmp.h"
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
int digi_win32_midi_song_playing=0;
static hmp_file *cur_hmp=NULL;
static int firstplay = 1;

void digi_win32_set_midi_volume( int mvolume )
{
	hmp_setvolume(cur_hmp, mvolume*MIDI_VOLUME_SCALE/8);
}

int digi_win32_play_midi_song( char * filename, int loop )
{
	if (firstplay)
	{
		hmp_reset();
		firstplay = 0;
	}
	digi_win32_stop_midi_song();

	if (filename == NULL)
		return 0;

	if ((cur_hmp = hmp_open(filename)))
	{
		/* 
		 * FIXME: to be implemented as soon as we have some kind or checksum function - replacement for ugly hack in hmp.c for descent.hmp
		 * if (***filesize check*** && ***CRC32 or MD5 check***)
		 *	(((*cur_hmp).trks)[1]).data[6] = 0x6C;
		 */
		if (hmp_play(cur_hmp,loop) != 0)
			return 0;	// error
		digi_win32_midi_song_playing = 1;
		digi_win32_set_midi_volume(GameCfg.MusicVolume);
		return 1;
	}

	return 0;
}

void digi_win32_pause_midi_song()
{
	hmp_pause(cur_hmp);
}

void digi_win32_resume_midi_song()
{
	hmp_resume(cur_hmp);
}

void digi_win32_stop_midi_song()
{
	if (!digi_win32_midi_song_playing)
		return;
	hmp_close(cur_hmp);
	cur_hmp = NULL;
	digi_win32_midi_song_playing = 0;
	hmp_reset();
}
#endif
