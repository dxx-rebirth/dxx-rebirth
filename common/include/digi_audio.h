/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

#pragma once

#include "maths.h"

#ifdef DXX_BUILD_DESCENT
namespace dcx {
enum class sound_pan : int;
enum class sound_channel : uint8_t;
enum sound_effect : uint8_t;
struct sound_object;
constexpr std::integral_constant<int, 16> digi_max_channels{};
}
namespace dsx {
int digi_audio_init();
void digi_audio_close();
void digi_audio_stop_all_channels();
sound_channel digi_audio_start_sound(sound_effect, fix, sound_pan, int, int, int, sound_object *);
int digi_audio_is_channel_playing(sound_channel);
void digi_audio_set_channel_volume(sound_channel, int);
void digi_audio_set_channel_pan(sound_channel, sound_pan);
void digi_audio_stop_sound(sound_channel);
void digi_audio_end_sound(sound_channel);
void digi_audio_set_digi_volume(int);
}
#endif
