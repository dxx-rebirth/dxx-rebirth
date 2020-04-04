/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include "maths.h"

#ifdef __cplusplus

#ifdef dsx
namespace dcx {
struct sound_object;
void digi_mixer_close();
void digi_mixer_set_channel_volume(int, int);
void digi_mixer_set_channel_pan(int, int);
void digi_mixer_stop_sound(int);
void digi_mixer_end_sound(int);
void digi_mixer_set_digi_volume(int);
int digi_mixer_is_channel_playing(int);
void digi_mixer_reset();
void digi_mixer_stop_all_channels();
}
namespace dsx {
int digi_mixer_init();
int digi_mixer_start_sound(short, fix, int, int, int, int, sound_object *);
}
#endif

#endif
