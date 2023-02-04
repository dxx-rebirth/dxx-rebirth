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
enum class sound_pan : int;
struct sound_object;
void digi_mixer_close();
void digi_mixer_set_channel_volume(sound_channel, int);
void digi_mixer_set_channel_pan(sound_channel, sound_pan);
void digi_mixer_stop_sound(sound_channel);
void digi_mixer_end_sound(sound_channel);
void digi_mixer_set_digi_volume(int);
int digi_mixer_is_channel_playing(sound_channel);
void digi_mixer_stop_all_channels();
int digi_mixer_init();
}
namespace dsx {
sound_channel digi_mixer_start_sound(short, fix, sound_pan, int, int, int, sound_object *);
}
#endif

#endif
