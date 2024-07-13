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

#include "dxxsconf.h"
#include "dsx-ns.h"
#include "fwd-object.h"
#include "fwd-segment.h"
#include "fwd-piggy.h"
#include <memory>
#include <span>
#include <utility>

namespace dcx {

#ifdef _WIN32
// Windows native-MIDI stuff.
void digi_win32_pause_midi_song();
void digi_win32_resume_midi_song();
void digi_win32_stop_midi_song();
void digi_win32_set_midi_volume(int mvolume);
int digi_win32_play_midi_song(const char * filename, int loop);
#endif

}

#ifdef dsx
namespace dcx {

enum class sound_pan : int
{
};

enum class sound_channel : uint8_t
{
	None = UINT8_MAX,
};

struct sound_object;
extern int digi_volume;

enum class sound_stack : bool
{
	allow_stacking,
	cancel_previous,
};

enum class sound_sample_rate : uint16_t
{
	_11k = 11025,
	_22k = 22050,
	_44k = 44100,
};

extern sound_channel SoundQ_channel;

/* These values are written to a file as integers, so they must not be
 * renumbered.
 */
enum class music_type : uint8_t
{
	None,
	Builtin,
#if DXX_USE_SDL_REDBOOK_AUDIO
	Redbook,
#endif
	Custom = 3,
};

}

namespace dsx {

extern int digi_init();
extern void digi_close();

// Volume is max at F1_0.
extern void digi_play_sample( int sndnum, fix max_volume );
extern void digi_play_sample_once( int sndnum, fix max_volume );
#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
void digi_link_sound_to_object(unsigned soundnum, vcobjptridx_t objnum, uint8_t forever, fix max_volume, sound_stack once);
void digi_kill_sound_linked_to_segment(vmsegidx_t segnum, sidenum_t sidenum, int soundnum);
void digi_link_sound_to_pos(unsigned soundnum, vcsegptridx_t segnum, sidenum_t sidenum, const vms_vector &pos, int forever, fix max_volume);
// Same as above, but you pass the max distance sound can be heard.  The old way uses f1_0*256 for max_distance.
void digi_link_sound_to_object2(unsigned soundnum, vcobjptridx_t objnum, uint8_t forever, fix max_volume, sound_stack once, vm_distance max_distance);
void digi_link_sound_to_object3(unsigned soundnum, vcobjptridx_t objnum, uint8_t forever, fix max_volume, sound_stack once, vm_distance max_distance, int loop_start, int loop_end);
void digi_kill_sound_linked_to_object(vcobjptridx_t);
#endif

void digi_play_sample_3d(int soundno, sound_pan angle, int volume); // Volume from 0-0x7fff

extern void digi_init_sounds();
extern void digi_sync_sounds();

extern void digi_set_digi_volume( int dvolume );

extern void digi_pause_digi_sounds();
extern void digi_resume_digi_sounds();

extern int digi_xlat_sound(int soundno);

void digi_stop_sound(sound_channel channel);

// Volume 0-F1_0
constexpr sound_object *sound_object_none = nullptr;
sound_channel digi_start_sound(short soundnum, fix volume, sound_pan pan, int looping, int loop_start, int loop_end, sound_object *);

// Stops all sounds that are playing
void digi_stop_all_channels();

void digi_stop_digi_sounds();

void digi_end_sound(sound_channel channel);
void digi_set_channel_pan(sound_channel channel, sound_pan pan);
void digi_set_channel_volume(sound_channel channel, int volume);
int digi_is_channel_playing(sound_channel channel);

extern void digi_play_sample_looping( int soundno, fix max_volume,int loop_start, int loop_end );
extern void digi_change_looping_volume( fix volume );
extern void digi_stop_looping_sound();

// Plays a queued voice sound.
extern void digi_start_sound_queued( short soundnum, fix volume );

// Following declarations are for the runtime switching system

#define SOUND_MAX_VOLUME F1_0 / 2

extern int Dont_start_sound_objects;
void digi_select_system();

void digi_end_soundobj(sound_object &);
void SoundQ_end();
#ifndef NDEBUG
void verify_sound_channel_free(sound_channel channel);
#endif

class RAIIdigi_sound
{
	static constexpr std::integral_constant<sound_channel, sound_channel::None> invalid_channel{};
	sound_channel channel = invalid_channel;
	static void stop(const sound_channel channel)
	{
		if (channel != invalid_channel)
			digi_stop_sound(channel);
	}
public:
	~RAIIdigi_sound()
	{
		stop(channel);
	}
	void reset(const sound_channel c = invalid_channel)
	{
		stop(std::exchange(channel, c));
	}
	operator int() const = delete;
	explicit operator bool() const
	{
		return channel != invalid_channel;
	}
};

}
#endif

namespace dcx {

struct digi_sound_deleter : std::default_delete<uint8_t[]>
{
	game_sound_offset offset{};
	constexpr digi_sound_deleter() = default;
	constexpr digi_sound_deleter(const game_sound_offset offset) :
		offset{offset}
	{
	}
	constexpr bool must_free_buffer() const
	{
		return static_cast<std::underlying_type<game_sound_offset>::type>(offset) <= 0;
	}
	void operator()(uint8_t *const p) const
	{
		if (must_free_buffer())
			this->std::default_delete<uint8_t[]>::operator()(p);
		/* Else, this pointer was not owned by the unique_ptr, and should not
		 * be freed.  This happens for some sounds that are stored in a single
		 * large allocation containing all the sounds, rather than individual
		 * per-sound allocations.
		 */
	}
};

struct digi_sound
{
	struct allocated_data : private std::unique_ptr<uint8_t[], digi_sound_deleter>
	{
		using base_type = std::unique_ptr<uint8_t[], digi_sound_deleter>;
		using base_type::get;
		using base_type::get_deleter;
		using base_type::operator bool;
		constexpr allocated_data() :
			base_type{}
		{
		}
		allocated_data(const base_type::pointer p, const game_sound_offset o) :
			base_type{p, o}
		{
		}
		/* This is only used in the Descent 1 build. */
		explicit allocated_data(std::unique_ptr<uint8_t[]> p, const game_sound_offset o) :
			allocated_data{p.release(), o}
		{
		}
		/* Define reset() instead of inheriting via `using base_type::reset`,
		 * because only the zero-argument form of reset() should be exposed.
		 * The one-argument form should be hidden.
		 */
		void reset()
		{
			this->base_type::reset();
		}
		allocated_data &operator=(allocated_data &&rhs)
		{
			auto &d = rhs.get_deleter();
			if (d.must_free_buffer())
				/* If the other object exclusively owns the data it controls,
				 * then use the standard move semantics of unique_ptr.
				 */
				return static_cast<allocated_data &>(this->base_type::operator=(std::move(rhs)));
			/* Otherwise, the source object does not free its data, so assume
			 * that it is safe, and sometimes necessary, to copy from the
			 * source instead of moving from it.
			 */
			if (this == &rhs)
				return *this;
			/* Free the old data, if needed. */
			this->base_type::reset();
			/* Mark this deleter as not requiring a free. */
			get_deleter() = d;
			/* Change this pointer to share ownership with the other object. */
			this->base_type::reset(rhs.get());
			return *this;
		}
	};
	std::size_t length;
	int freq;
	allocated_data data;
	std::span<const uint8_t> span() const
	{
		return {data.get(), length};
	}
};

}
