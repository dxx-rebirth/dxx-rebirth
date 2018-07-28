/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license below.
 * See COPYING.txt for license details.

THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Include file for sound hardware.
 *
 */

#pragma once

#include <SDL_version.h>
#include <type_traits>
#include "pstypes.h"
#include "vecmat.h"

#ifdef __cplusplus
#include "dxxsconf.h"
#include "dsx-ns.h"
#include "fwd-object.h"
#include "fwd-segment.h"
#include "compiler-exchange.h"

#ifdef dsx
namespace dcx {

struct sound_object;

}
namespace dsx {
struct digi_sound
{
        int bits;
        int freq;
	int length;
	uint8_t * data;
};

extern int digi_get_settings();
extern int digi_init();
#ifndef RELEASE
extern void digi_reset();
#endif
extern void digi_close();

// Volume is max at F1_0.
extern void digi_play_sample( int sndnum, fix max_volume );
extern void digi_play_sample_once( int sndnum, fix max_volume );
#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
enum class sound_stack : uint8_t
{
	allow_stacking,
	cancel_previous,
};
void digi_link_sound_to_object(unsigned soundnum, vcobjptridx_t objnum, uint8_t forever, fix max_volume, sound_stack once);
void digi_kill_sound_linked_to_segment(segnum_t segnum, int sidenum, int soundnum);
void digi_link_sound_to_pos( int soundnum, vcsegptridx_t segnum, short sidenum, const vms_vector &pos, int forever, fix max_volume );
// Same as above, but you pass the max distance sound can be heard.  The old way uses f1_0*256 for max_distance.
void digi_link_sound_to_object2(unsigned soundnum, vcobjptridx_t objnum, uint8_t forever, fix max_volume, sound_stack once, vm_distance max_distance);
void digi_link_sound_to_object3(unsigned soundnum, vcobjptridx_t objnum, uint8_t forever, fix max_volume, sound_stack once, vm_distance max_distance, int loop_start, int loop_end);
void digi_kill_sound_linked_to_object(vcobjptridx_t);
#endif

void digi_play_sample_3d(int soundno, int angle, int volume); // Volume from 0-0x7fff

extern void digi_init_sounds();
extern void digi_sync_sounds();

extern void digi_set_digi_volume( int dvolume );

extern void digi_pause_digi_sounds();
extern void digi_resume_digi_sounds();

extern int digi_xlat_sound(int soundno);

extern void digi_stop_sound( int channel );

// Volume 0-F1_0
constexpr sound_object *sound_object_none = nullptr;
int digi_start_sound(short soundnum, fix volume, int pan, int looping, int loop_start, int loop_end, sound_object *);

// Stops all sounds that are playing
void digi_stop_all_channels();

void digi_stop_digi_sounds();

extern void digi_end_sound( int channel );
extern void digi_set_channel_pan( int channel, int pan );
extern void digi_set_channel_volume( int channel, int volume );
extern int digi_is_channel_playing(int channel);

extern void digi_play_sample_looping( int soundno, fix max_volume,int loop_start, int loop_end );
extern void digi_change_looping_volume( fix volume );
extern void digi_stop_looping_sound();

// Plays a queued voice sound.
extern void digi_start_sound_queued( short soundnum, fix volume );

// Following declarations are for the runtime switching system

#define SAMPLE_RATE_11K 11025
#define SAMPLE_RATE_22K 22050
#define SAMPLE_RATE_44K 44100

#define SDLMIXER_SYSTEM 1
#define SDLAUDIO_SYSTEM 2

#define MUSIC_TYPE_NONE		0
#define MUSIC_TYPE_BUILTIN	1
#if SDL_MAJOR_VERSION == 1
#define MUSIC_TYPE_REDBOOK	2
#endif
#define MUSIC_TYPE_CUSTOM	3

// play-order definitions for custom music
#define MUSIC_CM_PLAYORDER_CONT 0
#define MUSIC_CM_PLAYORDER_LEVEL 1
#define MUSIC_CM_PLAYORDER_RAND 2

#define SOUND_MAX_VOLUME F1_0 / 2

extern int digi_volume;
#if defined(DXX_BUILD_DESCENT_I)
extern int digi_sample_rate;
#elif defined(DXX_BUILD_DESCENT_II)
extern int Dont_start_sound_objects;
#endif
extern int SoundQ_channel;
void digi_select_system();

#ifdef _WIN32
// Windows native-MIDI stuff.
void digi_win32_set_midi_volume( int mvolume );
int digi_win32_play_midi_song(const char * filename, int loop );
void digi_win32_pause_midi_song();
void digi_win32_resume_midi_song();
void digi_win32_stop_midi_song();
#endif
void digi_end_soundobj(sound_object &);
void SoundQ_end();
#ifndef NDEBUG
int verify_sound_channel_free( int channel );
#endif

}

namespace dcx {

class RAIIdigi_sound
{
	static constexpr auto invalid_channel = std::integral_constant<int, -1>{};
	int channel;
	static void stop(int channel)
	{
		if (channel != invalid_channel)
			digi_stop_sound(channel);
	}
public:
	RAIIdigi_sound() :
		channel(invalid_channel)
	{
	}
	~RAIIdigi_sound()
	{
		stop(channel);
	}
	void reset(int c = invalid_channel)
	{
		stop(exchange(channel, c));
	}
	operator int() const = delete;
	explicit operator bool() const
	{
		return channel != invalid_channel;
	}
};

}
#endif
#endif
