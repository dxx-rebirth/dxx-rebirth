#ifndef __DIGI_MIXER__
#define __DIGI_MIXER__

#include "fix.h"

int digi_mixer_init();
void digi_mixer_close();
int digi_mixer_start_sound(short, fix, int, int, int, int, int);
void digi_mixer_set_channel_volume(int, int);
void digi_mixer_set_channel_pan(int, int);
void digi_mixer_stop_sound(int);
void digi_mixer_end_sound(int);
int digi_mixer_is_sound_playing(int);
int digi_mixer_is_channel_playing(int);
void digi_mixer_reset();
void digi_set_max_channels(int);
int digi_get_max_channels();
void digi_mixer_stop_all_channels();
void digi_mixer_set_digi_volume(int);
void digi_mixer_debug();

#endif
