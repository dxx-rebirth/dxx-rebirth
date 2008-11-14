/*
 * This is an alternate backend for the sound effect system.
 * It uses SDL_mixer to provide a more reliable playback,
 * and allow processing of multiple audio formats.
 *
 * This file is based on the original D1X arch/sdl/digi.c
 *
 *  -- MD2211 (2006-10-12)
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <SDL/SDL.h>
#include <SDL/SDL_audio.h>
#include <SDL/SDL_mixer.h>

#include "pstypes.h"
#include "error.h"
#include "sounds.h"
#include "digi.h"
#include "digi_mixer.h"
#include "digi_mixer_music.h"
#include "jukebox.h"
#include "console.h"

#include "fix.h"
#include "gr.h" // needed for piggy.h
#include "piggy.h"

#ifdef _WIN32
#include "hmpfile.h"
#endif

#define MIX_DIGI_DEBUG 0
#define MIX_OUTPUT_FORMAT	AUDIO_S16
#define MIX_OUTPUT_CHANNELS	2

//edited 05/17/99 Matt Mueller - added ifndef NO_ASM
//added on 980905 by adb to add inline fixmul for mixer on i386
#ifndef NO_ASM
#ifdef __i386__
#define do_fixmul(x,y)				\
({						\
	int _ax, _dx;				\
	asm("imull %2\n\tshrdl %3,%1,%0"	\
	    : "=a"(_ax), "=d"(_dx)		\
	    : "rm"(y), "i"(16), "0"(x));	\
	_ax;					\
})
extern inline fix fixmul(fix x, fix y) { return do_fixmul(x,y); }
#endif
#endif
//end edit by adb
//end edit -MM

#define MAX_SOUND_SLOTS 64
#define SOUND_BUFFER_SIZE 1024 //4096
#define MIN_VOLUME 10

static int digi_initialised = 0;
static int digi_max_channels = MAX_SOUND_SLOTS;
inline int fix2byte(fix f) { return (f / 256) % 256; }
Mix_Chunk SoundChunks[MAX_SOUNDS];

/* Initialise audio */
int digi_mixer_init() {
  digi_sample_rate = SAMPLE_RATE_44K;

  if (MIX_DIGI_DEBUG) con_printf(CON_DEBUG,"digi_init %d (SDL_Mixer)\n", MAX_SOUNDS);
  if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) Error("SDL audio initialisation failed: %s.", SDL_GetError());

  if (Mix_OpenAudio(digi_sample_rate, MIX_OUTPUT_FORMAT, MIX_OUTPUT_CHANNELS, SOUND_BUFFER_SIZE)) {
    //edited on 10/05/98 by Matt Mueller - should keep running, just with no sound.
    con_printf(CON_URGENT,"\nError: Couldn't open audio: %s\n", SDL_GetError());
    GameArg.SndNoSound = 1;
    return 1;
  }

  Mix_AllocateChannels(digi_max_channels);
  Mix_Pause(0);

  // Attempt to load jukebox
  jukebox_load();
  //jukebox_list();

  digi_initialised = 1;

  return 0;
}

/* Shut down audio */
void digi_mixer_close() {
  if (!digi_initialised) return;
  digi_initialised = 0;
  Mix_CloseAudio();
}

/*
 * Play-time conversion. Performs output conversion only once per sound effect used.
 * Once the sound sample has been converted, it is cached in SoundChunks[]
 */
void mixdigi_convert_sound(int i) {

  SDL_AudioCVT cvt;
  Uint8 *data = GameSounds[i].data;
  Uint32 dlen = GameSounds[i].length;
  int freq = GameSounds[i].freq;
  //int bits = GameSounds[i].bits;

  if (SoundChunks[i].abuf) return; //proceed only if not converted yet

  if (data) {
    if (MIX_DIGI_DEBUG) con_printf(CON_DEBUG,"converting %d (%d)\n", i, dlen);
    SDL_BuildAudioCVT(&cvt, AUDIO_U8, 1, freq, MIX_OUTPUT_FORMAT, MIX_OUTPUT_CHANNELS, digi_sample_rate);

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

  channel = Mix_PlayChannel(-1, &(SoundChunks[soundnum]), mix_loop);
  Mix_SetPanning(channel, 255-mix_pan, mix_pan);
  if (volume > F1_0)
    Mix_SetDistance(channel, 0);
  else
    Mix_SetDistance(channel, 255-mix_vol);

  return channel;
}

void digi_mixer_set_channel_volume(int channel, int volume) {
  int mix_vol = fix2byte(volume);
  if (!digi_initialised) return;
  Mix_SetDistance(channel, 255-mix_vol);
}

void digi_mixer_set_channel_pan(int channel, int pan) {
  int mix_pan = fix2byte(pan);
  Mix_SetPanning(channel, 255-mix_pan, mix_pan);
}

void digi_mixer_stop_sound(int channel) {
  if (!digi_initialised) return;
  if (MIX_DIGI_DEBUG) con_printf(CON_DEBUG,"digi_stop_sound %d\n", channel);
  Mix_HaltChannel(channel);
}

void digi_mixer_end_sound(int channel) {
  digi_mixer_stop_sound(channel);
}

void digi_mixer_set_digi_volume( int dvolume )
{
  digi_volume = dvolume;
  if (!digi_initialised) return;
  Mix_Volume(-1, fix2byte(dvolume));
}

void digi_mixer_set_midi_volume( int mvolume ) {
#ifdef _WIN32
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
#endif
  midi_volume = mvolume;
  if (!digi_initialised) return;
  mix_set_music_volume(mvolume);
}

void digi_mixer_set_volume( int dvolume, int mvolume ) {
  digi_mixer_set_digi_volume(dvolume);
  digi_mixer_set_midi_volume(mvolume);
}


int digi_mixer_find_channel(int soundno) { return 0; }
int digi_mixer_is_sound_playing(int soundno) { return 0; }
int digi_mixer_is_channel_playing(int channel) { return 0; }

void digi_mixer_reset() {}
void digi_mixer_stop_all_channels() {}

extern void digi_end_soundobj(int channel);	
int verify_sound_channel_free(int channel);

 //added on 980905 by adb to make sound channel setting work
void digi_mixer_set_max_channels(int n) { }
int digi_mixer_get_max_channels() { return digi_max_channels; }
// end edit by adb


// MIDI stuff follows.

void digi_mixer_play_midi_song(char * filename, char * melodic_bank, char * drum_bank, int loop ) {
  if (!digi_initialised) return;
  if (GameArg.SndNoMusic)
    return;

  mix_set_music_volume(midi_volume);
  jukebox_load(); // update jukebox state

  // quick hack to check if filename begins with "game" -- MD2211
  if (jukebox_is_loaded() && strstr(filename, "game") == filename) {
    // use jukebox
    jukebox_play(loop);
  }
  else {
    // standard song playback
#ifdef _WIN32
    if (!GameArg.SndExternalMusic)
    {
      if ((hmp = hmp_open(filename)))
      {
        hmp_play(hmp,loop);
        digi_midi_song_playing = 1;
        digi_set_midi_volume(midi_volume);
      }
    }
    else
#endif
      mix_play_music(filename, loop);
  }
}

int digi_mixer_music_exists(const char *filename)
{
	return mix_music_exists(filename);
}

void digi_mixer_stop_current_song() {
#ifdef _WIN32
  if (digi_midi_song_playing)
  {
    hmp_close(hmp);
    hmp = NULL;
    digi_midi_song_playing = 0;
  }
#endif
  jukebox_stop();
  mix_stop_music();
}

void digi_mixer_pause_midi() {}
void digi_mixer_resume_midi() {}

#ifndef NDEBUG
void digi_mixer_debug() {}
#endif
