#ifndef __DIGI_AUDIO__
#define __DIGI_AUDIO__

#include "fix.h"

int digi_audio_init();
void digi_audio_reset();
void digi_audio_close();
void digi_audio_stop_all_channels();
int digi_audio_start_sound(short, fix, int, int, int, int, int );
int digi_audio_is_sound_playing(int );
int digi_audio_is_channel_playing(int );
void digi_audio_set_channel_volume(int, int );
void digi_audio_set_channel_pan(int, int );
void digi_audio_stop_sound(int );
void digi_audio_end_sound(int );
void digi_audio_set_digi_volume(int);
void digi_audio_debug();

#endif
