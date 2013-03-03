/*
 * This is an alternate backend for the sound effect system.
 * It uses SDL_mixer to provide a more reliable playback,
 * and allow processing of multiple audio formats.
 *
 * This file is based on the original D1X arch/sdl/digi.c
 *
 *  -- MD2211 (2006-10-12)
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <SDL/SDL.h>
#include <SDL/SDL_audio.h>
#if !(defined(__APPLE__) && defined(__MACH__))
#include <SDL/SDL_mixer.h>
#else
#include <SDL_mixer/SDL_mixer.h>
#endif

#include "pstypes.h"
#include "dxxerror.h"
#include "sounds.h"
#include "digi.h"
#include "digi_mixer.h"
#include "digi_mixer_music.h"
#include "console.h"
#include "config.h"
#include "args.h"

#include "fix.h"
#include "gr.h" // needed for piggy.h
#include "piggy.h"

#define MIX_DIGI_DEBUG 0
#define MIX_OUTPUT_FORMAT	AUDIO_S16
#define MIX_OUTPUT_CHANNELS	2

#define MAX_SOUND_SLOTS 64
#if !((defined(__APPLE__) && defined(__MACH__)) || defined(macintosh))
#define SOUND_BUFFER_SIZE 2048
#else
#define SOUND_BUFFER_SIZE 1024
#endif
#define MIN_VOLUME 10

static int digi_initialised = 0;
static int digi_max_channels = MAX_SOUND_SLOTS;
static inline int fix2byte(fix f) { return (f / 256) % 256; }
Mix_Chunk SoundChunks[MAX_SOUNDS];
ubyte channels[MAX_SOUND_SLOTS];

/* Initialise audio */
int digi_mixer_init()
{
	if (MIX_DIGI_DEBUG) con_printf(CON_DEBUG,"digi_init %d (SDL_Mixer)\n", MAX_SOUNDS);
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) Error("SDL audio initialisation failed: %s.", SDL_GetError());

	if (Mix_OpenAudio(SAMPLE_RATE_44K, MIX_OUTPUT_FORMAT, MIX_OUTPUT_CHANNELS, SOUND_BUFFER_SIZE))
	{
		//edited on 10/05/98 by Matt Mueller - should keep running, just with no sound.
		con_printf(CON_URGENT,"\nError: Couldn't open audio: %s\n", SDL_GetError());
		GameArg.SndNoSound = 1;
		return 1;
	}

	digi_max_channels = Mix_AllocateChannels(digi_max_channels);
	memset(channels, 0, MAX_SOUND_SLOTS);
	Mix_Pause(0);

	digi_initialised = 1;

	digi_mixer_set_digi_volume( (GameCfg.DigiVolume*32768)/8 );

	return 0;
}

/* Shut down audio */
void digi_mixer_close() {
	if (MIX_DIGI_DEBUG) con_printf(CON_DEBUG,"digi_close (SDL_Mixer)\n");
	if (!digi_initialised) return;
	digi_initialised = 0;
	Mix_CloseAudio();
}

/* channel management */
int digi_mixer_find_channel()
{
	int i;
	for (i = 0; i < digi_max_channels; i++)
		if (channels[i] == 0)
			return i;
	return -1;
}

void digi_mixer_free_channel(int channel_num)
{
	channels[channel_num] = 0;
}

/*
 * Play-time conversion. Performs output conversion only once per sound effect used.
 * Once the sound sample has been converted, it is cached in SoundChunks[]
 */
void mixdigi_convert_sound(int i)
{
	SDL_AudioCVT cvt;
	Uint8 *data = GameSounds[i].data;
	Uint32 dlen = GameSounds[i].length;
	int out_freq;
	Uint16 out_format;
	int out_channels;

	Mix_QuerySpec(&out_freq, &out_format, &out_channels); // get current output settings

	if (SoundChunks[i].abuf) return; //proceed only if not converted yet

	if (data)
	{
		if (MIX_DIGI_DEBUG) con_printf(CON_DEBUG,"converting %d (%d)\n", i, dlen);
		SDL_BuildAudioCVT(&cvt, AUDIO_U8, 1, GameArg.SndDigiSampleRate, out_format, out_channels, out_freq);

		cvt.buf = malloc(dlen * cvt.len_mult);
		cvt.len = dlen;
		memcpy(cvt.buf, data, dlen);
		if (SDL_ConvertAudio(&cvt)) con_printf(CON_DEBUG,"conversion of %d failed\n", i);

		SoundChunks[i].abuf = cvt.buf;
		SoundChunks[i].alen = dlen * cvt.len_mult;
		SoundChunks[i].allocated = 1;
		SoundChunks[i].volume = 128; // Max volume = 128
	}
}

// Volume 0-F1_0
int digi_mixer_start_sound(short soundnum, fix volume, int pan, int looping, int loop_start, int loop_end, int soundobj)
{
	int mix_vol = fix2byte(fixmul(digi_volume, volume));
	int mix_pan = fix2byte(pan);
	int mix_loop = looping * -1;
	int channel;

	if (!digi_initialised) return -1;
	Assert(GameSounds[soundnum].data != (void *)-1);

	mixdigi_convert_sound(soundnum);

	if (MIX_DIGI_DEBUG) con_printf(CON_DEBUG,"digi_start_sound %d, volume %d, pan %d (start=%d, end=%d)\n", soundnum, mix_vol, mix_pan, loop_start, loop_end);

	channel = digi_mixer_find_channel();

	Mix_PlayChannel(channel, &(SoundChunks[soundnum]), mix_loop);
	Mix_SetPanning(channel, 255-mix_pan, mix_pan);
	if (volume > F1_0)
		Mix_SetDistance(channel, 0);
	else
		Mix_SetDistance(channel, 255-mix_vol);
	channels[channel] = 1;
	Mix_ChannelFinished(digi_mixer_free_channel);

	return channel;
}

void digi_mixer_set_channel_volume(int channel, int volume)
{
	int mix_vol = fix2byte(volume);
	if (!digi_initialised) return;
	Mix_SetDistance(channel, 255-mix_vol);
}

void digi_mixer_set_channel_pan(int channel, int pan)
{
	int mix_pan = fix2byte(pan);
	Mix_SetPanning(channel, 255-mix_pan, mix_pan);
}

void digi_mixer_stop_sound(int channel) {
	if (!digi_initialised) return;
	if (MIX_DIGI_DEBUG) con_printf(CON_DEBUG,"digi_stop_sound %d\n", channel);
	Mix_HaltChannel(channel);
	channels[channel] = 0;
}

void digi_mixer_end_sound(int channel)
{
	digi_mixer_stop_sound(channel);
	channels[channel] = 0;
}

void digi_mixer_set_digi_volume( int dvolume )
{
	digi_volume = dvolume;
	if (!digi_initialised) return;
	Mix_Volume(-1, fix2byte(dvolume));
}

int digi_mixer_is_sound_playing(int soundno) { return 0; }
int digi_mixer_is_channel_playing(int channel) { return 0; }

void digi_mixer_reset() {}
void digi_mixer_stop_all_channels()
{
	Mix_HaltChannel(-1);
	memset(channels, 0, MAX_SOUND_SLOTS);
}

extern void digi_end_soundobj(int channel);

 //added on 980905 by adb to make sound channel setting work
void digi_mixer_set_max_channels(int n) { }
int digi_mixer_get_max_channels() { return digi_max_channels; }
// end edit by adb

#ifndef NDEBUG
void digi_mixer_debug() {}
#endif
