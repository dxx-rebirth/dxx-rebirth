/*
 * Digital audio support
 * Library-independent stub for dynamic selection of sound system
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

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

void (*fptr_play_midi_song)(char *, char *, char *, int) = NULL;
void (*fptr_stop_current_song)() = NULL;
void (*fptr_pause_midi)() = NULL;
void (*fptr_resume_midi)() = NULL;
int  (*fptr_music_exists)(const char *) = NULL;
void (*fptr_set_midi_volume)(int) = NULL;

void (*fptr_extmusic_load)(void) = NULL;
void (*fptr_extmusic_unload)(void) = NULL;
int (*fptr_extmusic_is_loaded)(void) = NULL;
int (*fptr_extmusic_play_track)(int track) = NULL;
int (*fptr_extmusic_play_tracks)(int first, int last, void (*hook_finished)()) = NULL;
void (*fptr_extmusic_eject_disk)(void) = NULL;
int (*fptr_extmusic_get_track_playing)(void) = NULL;
int (*fptr_extmusic_get_numtracks)(void) = NULL;
void (*fptr_extmusic_stop)(void) = NULL;
void (*fptr_extmusic_pause)() = NULL;
int (*fptr_extmusic_resume)() = NULL;
int (*fptr_extmusic_pause_resume)(void) = NULL;
void (*fptr_extmusic_set_volume)(int volume) = NULL;
void (*fptr_extmusic_list)(void) = NULL;

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
	fptr_play_midi_song = digi_mixer_play_midi_song;
	fptr_music_exists = digi_mixer_music_exists;
	fptr_stop_current_song = digi_mixer_stop_current_song;
	fptr_pause_midi = digi_mixer_pause_midi;
	fptr_resume_midi = digi_mixer_resume_midi;
	fptr_set_midi_volume = digi_mixer_set_midi_volume;
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
        fptr_play_midi_song = digi_audio_play_midi_song;
		fptr_music_exists = PHYSFS_exists;
        fptr_stop_current_song = digi_audio_stop_current_song;
        fptr_pause_midi = digi_audio_pause_midi;
        fptr_resume_midi = digi_audio_resume_midi;
	fptr_set_midi_volume = digi_audio_set_midi_volume;
	fptr_set_digi_volume = digi_audio_set_digi_volume;
 	break;
	}
}

/* Common digi functions */
#ifndef NDEBUG
static int digi_initialised = 0;
#endif
extern int digi_max_channels;
int digi_sample_rate = SAMPLE_RATE_11K;
int digi_volume = SOUND_MAX_VOLUME;
int midi_volume = SOUND_MAX_VOLUME;

void digi_set_volume(int dvolume, int mvolume) {
	digi_volume = dvolume;
	midi_volume = mvolume;
	if (fptr_set_digi_volume) digi_set_digi_volume(dvolume);
	if (fptr_set_midi_volume) digi_set_midi_volume(mvolume);
}

void digi_set_sample_rate(int r) { digi_sample_rate = r; }

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

void digi_play_midi_song(char * filename, char * melodic_bank, char * drum_bank, int loop ) { fptr_play_midi_song(filename, melodic_bank, drum_bank, loop); }
int  digi_music_exists(const char *filename) { return fptr_music_exists(filename); }
void digi_set_midi_volume(int mvolume) { fptr_set_midi_volume(mvolume); }
void digi_stop_current_song() { fptr_stop_current_song(); }
void digi_pause_midi() { fptr_pause_midi(); }
void digi_resume_midi() { fptr_resume_midi(); }

void ext_music_select_system(int n)
{
	switch (n)
	{
#ifdef USE_SDLMIXER
		case EXT_MUSIC_JUKEBOX:
			if (fptr_init == digi_mixer_init)	// using SDL_mixer
			{
				fptr_extmusic_load = jukebox_load;
				fptr_extmusic_unload = jukebox_unload;
				fptr_extmusic_is_loaded = jukebox_is_loaded;
				fptr_extmusic_play_tracks = jukebox_play_tracks;
				fptr_extmusic_eject_disk = jukebox_unload;
				fptr_extmusic_get_track_playing = jukebox_is_playing;
				fptr_extmusic_get_numtracks = jukebox_numtracks;
				fptr_extmusic_stop = jukebox_stop;
				fptr_extmusic_pause = jukebox_pause;
				fptr_extmusic_resume = jukebox_resume;
				fptr_extmusic_pause_resume = jukebox_pause_resume;
				fptr_extmusic_set_volume = jukebox_set_volume;
				fptr_extmusic_list = jukebox_list;
				break;
			}	// else fall through
#endif
			
			case EXT_MUSIC_REDBOOK:
			default:
			fptr_extmusic_load = RBAInit;
			fptr_extmusic_unload = RBAExit;
			fptr_extmusic_is_loaded = RBAEnabled;
			fptr_extmusic_play_track = RBAPlayTrack;
			fptr_extmusic_play_tracks = RBAPlayTracks;
			fptr_extmusic_eject_disk = RBAEjectDisk;
			fptr_extmusic_get_track_playing = RBAGetTrackNum;
			fptr_extmusic_get_numtracks = RBAGetNumberOfTracks;
			fptr_extmusic_stop = RBAStop;
			fptr_extmusic_pause = RBAPause;
			fptr_extmusic_resume = RBAResume;
			fptr_extmusic_pause_resume = RBAPauseResume;
			fptr_extmusic_set_volume = RBASetVolume;
			fptr_extmusic_list = RBAList;
			break;
	}
}

void ext_music_load(void) { fptr_extmusic_load(); }
void ext_music_unload(void) { fptr_extmusic_unload(); }
int ext_music_is_loaded(void) { return fptr_extmusic_is_loaded(); }
int ext_music_play_tracks(int first, int last, void (*hook_finished)(void)) { return fptr_extmusic_play_tracks(first, last, hook_finished); }
void ext_music_eject_disk(void) { fptr_extmusic_eject_disk(); }
int ext_music_get_track_playing(void) { return fptr_extmusic_get_track_playing(); }
int ext_music_get_numtracks(void) { return fptr_extmusic_get_numtracks(); }
void ext_music_stop() { fptr_extmusic_stop(); }
void ext_music_pause() { fptr_extmusic_pause(); }
int ext_music_resume() { return fptr_extmusic_resume(); }
int ext_music_pause_resume(void) { return fptr_extmusic_pause_resume(); }
void ext_music_set_volume(int volume) { fptr_extmusic_set_volume(volume); }
void ext_music_list(void) { fptr_extmusic_list(); }

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
