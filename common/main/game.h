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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Constants & prototypes which pertain to the game only
 *
 */

#pragma once

#include "fwd-game.h"

#include "pack.h"
#include "fwd-segment.h"
#include "robot.h"
#include "window.h"
#include "wall.h"
#include "d_underlying_value.h"
#include <optional>

namespace dcx {

enum class game_mode_flag : uint16_t
{
	normal = 0,
	network			= 1u << 2,
	multi_robots	= 1u << 3,
	multi_coop		= 1u << 4,
	unknown			= 1u << 6,
	team			= 1u << 8,
	bounty			= 1u << 9,
	/* if DXX_BUILD_DESCENT_II */
	capture			= 1u << 10,
	hoard			= 1u << 11,
	/* endif */
};

enum class game_mode_flags : uint16_t
{
	normal = 0,
	anarchy_no_robots = static_cast<uint16_t>(game_mode_flag::network),
	team_anarchy_no_robots = static_cast<uint16_t>(game_mode_flag::network) | static_cast<uint16_t>(game_mode_flag::team),
	anarchy_with_robots = static_cast<uint16_t>(game_mode_flag::network) | static_cast<uint16_t>(game_mode_flag::multi_robots),
	cooperative = static_cast<uint16_t>(game_mode_flag::network) | static_cast<uint16_t>(game_mode_flag::multi_robots) | static_cast<uint16_t>(game_mode_flag::multi_coop),
	bounty = static_cast<uint16_t>(game_mode_flag::network) | static_cast<uint16_t>(game_mode_flag::bounty),
	/* if DXX_BUILD_DESCENT_II */
	capture_flag = static_cast<uint16_t>(game_mode_flag::network) | static_cast<uint16_t>(game_mode_flag::team) | static_cast<uint16_t>(game_mode_flag::capture),
	hoard = static_cast<uint16_t>(game_mode_flag::network) | static_cast<uint16_t>(game_mode_flag::hoard),
	team_hoard = static_cast<uint16_t>(game_mode_flag::network) | static_cast<uint16_t>(game_mode_flag::team) | static_cast<uint16_t>(game_mode_flag::hoard),
	/* endif */
};

static constexpr auto operator&(const game_mode_flags game_mode, const game_mode_flag f)
{
	return underlying_value(game_mode) & underlying_value(f);
}

/* This must be a signed type.  Some sites, such as `bump_this_object`,
 * use Difficulty_level_type in arithmetic expressions, and those
 * expressions must be signed to produce the correct result.
 */
enum class Difficulty_level_type : signed int
{
	_0,
	_1,
	_2,
	_3,
	_4,
};

constexpr Difficulty_level_type DEFAULT_DIFFICULTY = Difficulty_level_type::_1;

static inline Difficulty_level_type cast_clamp_difficulty(const int8_t d)
{
	return (static_cast<uint8_t>(d) <= static_cast<uint8_t>(Difficulty_level_type::_4)) ? Difficulty_level_type{d} : Difficulty_level_type::_4;
}

std::optional<Difficulty_level_type> build_difficulty_level_from_untrusted(int8_t untrusted);

struct d_game_shared_state
{
};

struct d_game_unique_state
{
	using savegame_file_path = std::array<char, PATH_MAX>;
	/* 20 is required by the save game ABI and the multiplayer
	 * save/restore command ABI.
	 */
	using savegame_description = std::array<char, 20>;
	static constexpr std::integral_constant<unsigned, 11> MAXIMUM_SAVE_SLOTS{};
	enum class save_slot : unsigned
	{
		_0,
		_1,
		_2,
		_3,
		_4,
		_5,
		_6,
		_7,
		_8,
		_9,
		_autosave,
		secret_save_filename_override = MAXIMUM_SAVE_SLOTS + 1,
		None = UINT32_MAX
	};
	Difficulty_level_type Difficulty_level;    // Difficulty level in 0..NDL-1, 0 = easiest, NDL-1 = hardest
	fix Boss_gate_interval;
	unsigned accumulated_robots;
	unsigned total_hostages;
	std::chrono::steady_clock::time_point Next_autosave;
	save_slot quicksave_selection = save_slot::None;
	static constexpr unsigned valid_save_slot(const save_slot s)
	{
		return static_cast<unsigned>(s) < static_cast<unsigned>(save_slot::_autosave);
	}
	static constexpr unsigned valid_load_slot(const save_slot s)
	{
		return static_cast<unsigned>(s) <= static_cast<unsigned>(save_slot::_autosave);
	}
};

#if DXX_USE_STEREOSCOPIC_RENDER
// Stereo viewport formats
enum class StereoFormat : uint8_t
{
	None = 0,
	AboveBelow,
	SideBySideFullHeight,
	SideBySideHalfHeight,
	AboveBelowSync,
	QuadBuffers,
	HighestFormat = QuadBuffers
};

// stereo viewport adjustments
inline void gr_stereo_viewport_resize(const StereoFormat stereo, int &w, int &h)
{
	switch (stereo) {
		case StereoFormat::None:
		case StereoFormat::QuadBuffers:
			return;
		case StereoFormat::AboveBelowSync:
			h -= VR_sync_width;
			[[fallthrough]];
		case StereoFormat::AboveBelow:
			h /= 2;
			break;
		case StereoFormat::SideBySideHalfHeight:
			h /= 2;
			[[fallthrough]];
		case StereoFormat::SideBySideFullHeight:
			w /= 2;
			break;
	}
}

inline void gr_stereo_viewport_offset(const StereoFormat stereo, int &x, int &y, const int eye = 0)
{
	if (!grd_curscreen)
		return;

	// left eye viewport origin
	if (eye <= 0) {
		if (stereo == StereoFormat::SideBySideHalfHeight)
			y += SHEIGHT/4;
		return;
	}

	// right eye viewport origin
	switch (stereo) {
		case StereoFormat::None:
		case StereoFormat::QuadBuffers:
			return;
		case StereoFormat::AboveBelowSync:
			y += VR_sync_width/2;
			[[fallthrough]];
		case StereoFormat::AboveBelow:
			y += SHEIGHT/2;
			break;
		case StereoFormat::SideBySideHalfHeight:
			if (y == 0 && eye > F0_1)	// for automap 2nd pass
				y += SHEIGHT/4;
			[[fallthrough]];
		case StereoFormat::SideBySideFullHeight:
			x += SWIDTH/2;
			break;
	}
}

inline void gr_stereo_viewport_window(const StereoFormat stereo, int &x, int &y, int &w, int &h)
{
	switch (stereo) {
		case StereoFormat::None:
		case StereoFormat::QuadBuffers:
			return;
		case StereoFormat::AboveBelowSync:
			h -= VR_sync_width / 2;
			[[fallthrough]];
		case StereoFormat::AboveBelow:
			y -= y / 2;
			h -= h / 2;
			break;
		case StereoFormat::SideBySideHalfHeight:
			y -= y / 2;
			h -= h / 2;
			[[fallthrough]];
		case StereoFormat::SideBySideFullHeight:
			x -= x / 2;
			w -= w / 2;
			break;
	}
}
#endif	// DXX_USE_STEREOSCOPIC_RENDER

}

#ifdef dsx
namespace dsx {

struct game_window final : window
{
	using window::window;
	virtual window_event_result event_handler(const d_event &) override;
};

struct d_game_shared_state : ::dcx::d_game_shared_state
{
	wall_animations_array WallAnims;
};

#if defined(DXX_BUILD_DESCENT_II)
struct d_game_unique_state : ::dcx::d_game_unique_state
{
	fix Final_boss_countdown_time;
};

struct d_level_shared_seismic_state
{
	fix Level_shake_frequency;
	fix Level_shake_duration;
};

struct d_level_unique_seismic_state
{
	fix64 Seismic_disturbance_end_time;
	fix64 Next_seismic_sound_time;
	int Seismic_tremor_volume;
	fix Seismic_tremor_magnitude;
	std::array<fix64, 4> Earthshaker_detonate_times;
};
#endif

}
#endif

namespace dcx {

class pause_game_world_time
{
public:
	pause_game_world_time();
	~pause_game_world_time();
};

enum class cockpit_mode_t : uint8_t
{
//valid modes for cockpit
	full_cockpit,   // normal screen with cockpit
	rear_view,   // looking back with bitmap
	status_bar,   // small status bar, w/ reticle
	full_screen,   // full screen, no cockpit (w/ reticle)
	letterbox   // half-height window (for cutscenes)
};

}

#ifdef dsx
#if defined(DXX_BUILD_DESCENT_I)
static inline void full_palette_save(void)
{
	palette_save();
}
#endif
namespace dsx {
#if defined(DXX_BUILD_DESCENT_I)
static constexpr uint8_t game_mode_capture_flag()
{
	return 0;
}
static constexpr uint8_t game_mode_hoard()
{
	return 0;
}
#elif defined(DXX_BUILD_DESCENT_II)
static inline uint16_t game_mode_capture_flag()
{
	return (Game_mode & GM_CAPTURE);
}
static inline uint16_t game_mode_hoard()
{
	return (Game_mode & GM_HOARD);
}

//Flickering light system
struct flickering_light {
	segnum_t segnum;
	sidenum_t sidenum;
	uint32_t mask;     // determines flicker pattern
	fix timer;              // time until next change
	fix delay;              // time between changes
};

struct d_flickering_light_state
{
	using Flickering_light_array_t = std::array<flickering_light, 100>;
	unsigned Num_flickering_lights;
	Flickering_light_array_t Flickering_lights;
};
#endif

//Cheats
struct game_cheats : prohibit_void_ptr<game_cheats>
{
	int enabled;
	int wowie;
	int allkeys;
	int invul;
	int shields;
	int killreactor;
	int exitpath;
	int levelwarp;
	int fullautomap;
	int ghostphysics;
	int rapidfire;
	int turbo;
	int robotfiringsuspended;
	int acid;
#if defined(DXX_BUILD_DESCENT_I)
	int wowie2;
	int cloak;
	int extralife;
	int baldguy;
#elif defined(DXX_BUILD_DESCENT_II)
	int lamer;
	int accessory;
	int bouncyfire;
	int homingfire;
	int killallrobots;
	int robotskillrobots;
	int monsterdamage;
	int buddyclone;
	int buddyangry;
#endif
};

void game_render_frame(const d_robot_info_array &Robot_info, const control_info &Controls);
void game_render_frame_mono(const d_robot_info_array &Robot_info, const control_info &Controls);
window_event_result ReadControls(const d_level_shared_robot_info_state &LevelSharedRobotInfoState, const d_event &event, control_info &Controls);

}
#endif
