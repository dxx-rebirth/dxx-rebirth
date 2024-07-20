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
 * Header for playsave.c
 *
 */

#pragma once

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
#if defined(DXX_BUILD_DESCENT_I)
#include "pstypes.h"
#include "player.h"
#elif defined(DXX_BUILD_DESCENT_II)
#include "escort.h"

enum class MissileViewMode : uint8_t
{
	None,
	EnabledSelfOnly,
	EnabledSelfAndAllies,
};

enum class cockpit_3d_view : uint8_t
{
	None,
	Escort,
	Rear,
	Coop,
	Marker,
};
#endif

#include "gameplayopt.h"
#include <cstdint>

#define N_SAVE_SLOTS    10
#define GAME_NAME_LEN   25      // +1 for terminating zero = 26

#ifdef dsx
#if defined(DXX_BUILD_DESCENT_I)
namespace dsx {
// NOTE: Obsolete structure - only kept for compability of shareware plr file
struct saved_game_sw
{
	char		name[GAME_NAME_LEN+1];		//extra char for terminating zero
	struct player_rw sg_player;
	int		difficulty_level;		//which level game is played at
	int		primary_weapon;		//which weapon selected
	int		secondary_weapon;		//which weapon selected
	int		cockpit_mode;			//which cockpit mode selected
	int		window_w,window_h;	//size of player's window
	int		next_level_num;		//which level we're going to
	int		auto_leveling_on;		//does player have autoleveling on?
} __pack__;

void plyr_save_stats(const char *callsign, int kills, int deaths);
}
#endif
#endif

struct hli
{
	std::array<char, 9> Shortname;
	uint8_t LevelNum;
};

#ifdef dsx
#include "kconfig.h"
#include "multi.h"
#include "fwd-game.h"
#include "fwd-weapon.h"
#include "d_array.h"

namespace dcx {

enum class reticle_type : uint8_t;

enum class FiringAutoselectMode : uint8_t
{
	Immediate,
	Never,
	Delayed,
};

enum class HudType : uint8_t
{
	Standard,
	Alternate1,
	Alternate2,
	Hidden,
};

enum class RespawnPress : bool
{
	Any,
	Fire,
};

enum MouselookMode : uint8_t
{
	Singleplayer = 1,
	MPCoop = 2,
	MPAnarchy = 4,
};

enum class player_config_keyboard_index : std::size_t
{
	turn_lr,
	pitch_ud,
	slide_lr,
	slide_ud,
	bank_lr,
};

enum class player_config_mouse_index : std::size_t
{
	turn_lr,
	pitch_ud,
	slide_lr,
	slide_ud,
	bank_lr,
	throttle,
};

enum class player_config_joystick_index : std::size_t
{
	turn_lr,
	pitch_ud,
	slide_lr,
	slide_ud,
	bank_lr,
	throttle,
};

}

namespace dsx {

struct player_config : prohibit_void_ptr<player_config>
{
	ubyte ControlType;
	HudType HudMode;
	RespawnPress RespawnMode;
	uint8_t MouselookFlags;
	uint8_t PitchLockFlags;
	using primary_weapon_order = std::array<primary_weapon_index_t, MAX_PRIMARY_WEAPONS + 1>;
	using secondary_weapon_order = std::array<secondary_weapon_index_t, MAX_SECONDARY_WEAPONS + 1>;
	primary_weapon_order PrimaryOrder;
	secondary_weapon_order SecondaryOrder;
	struct KeySettings {
		enumerated_array<uint8_t, MAX_CONTROLS, dxx_kconfig_ui_kc_keyboard> Keyboard;
#if DXX_MAX_JOYSTICKS
		enumerated_array<uint8_t, MAX_CONTROLS, dxx_kconfig_ui_kc_joystick> Joystick;
#endif
		enumerated_array<uint8_t, MAX_CONTROLS, dxx_kconfig_ui_kc_mouse> Mouse;
	} KeySettings;
	std::array<uint8_t, MAX_DXX_REBIRTH_CONTROLS> KeySettingsRebirth;
	Difficulty_level_type DefaultDifficulty;
	int AutoLeveling;
	uint16_t NHighestLevels;
	std::array<hli, MAX_MISSIONS> HighestLevels;
	enumerated_array<int, 5, player_config_keyboard_index> KeyboardSens;
	enumerated_array<int, 6, player_config_joystick_index> JoystickSens;
	enumerated_array<int, 6, player_config_joystick_index> JoystickDead;
	enumerated_array<int, 6, player_config_joystick_index> JoystickLinear;
	enumerated_array<int, 6, player_config_joystick_index> JoystickSpeed;
	ubyte MouseFlightSim;
	enumerated_array<int, 6, player_config_mouse_index> MouseSens;
	enumerated_array<int, 6, player_config_mouse_index> MouseOverrun;
	int MouseFSDead;
	int MouseFSIndicator;
	std::array<cockpit_mode_t, 2> CockpitMode; // 0 saves the "real" cockpit, 1 also saves letterbox and rear. Used to properly switch between modes and restore the real one.
#if defined(DXX_BUILD_DESCENT_II)
	enumerated_array<cockpit_3d_view, 2, gauge_inset_window_view> Cockpit3DView = {{{
		cockpit_3d_view::None,
		cockpit_3d_view::None,
	}}};
#endif
	enumerated_array<ntstring<MAX_MESSAGE_LEN - 1>, 4, multi_macro_message_index> NetworkMessageMacro;
	int NetlifeKills;
	int NetlifeKilled;
	std::array<int, 4> ReticleRGBA;
	int ReticleSize;
	reticle_type ReticleType;
#if defined(DXX_BUILD_DESCENT_II)
	MissileViewMode MissileViewEnabled;
	uint8_t ThiefModifierFlags;
	bool HeadlightActiveDefault;
	bool GuidedInBigWindow;
	ntstring<GUIDEBOT_NAME_LEN> GuidebotName, GuidebotNameReal;
	bool EscortHotKeys;
#endif
	bool PersistentDebris;
	bool PRShot;
	ubyte NoRedundancy;
	ubyte MultiMessages;
        ubyte MultiPingHud;
	ubyte NoRankings;
#if defined(DXX_BUILD_DESCENT_I)
	ubyte BombGauge;
#endif
	ubyte AutomapFreeFlight;
	FiringAutoselectMode NoFireAutoselect;
	ubyte CycleAutoselectOnly;
	uint8_t CloakInvulTimer;
	union {
		/* For now, manage all these choices in a single variable, but
		 * give them separate names to make them easier to find.
		 */
		int AlphaEffects;
		int AlphaBlendMineExplosion;
		int AlphaBlendMarkers;
		int AlphaBlendFireballs;
		int AlphaBlendPowerups;
		int AlphaBlendLasers;
		int AlphaBlendWeapons;
		int AlphaBlendEClips;
	};
	int DynLightColor;
	d_sp_gameplay_options SPGameplayOptions;
};

}
#endif

extern struct player_config PlayerCfg;

#ifndef EZERO
#define EZERO 0
#endif

// Used to save kconfig values to disk.
#ifdef dsx
namespace dsx {

extern const struct player_config::KeySettings DefaultKeySettings;

void write_player_file();

void new_player_config();

int read_player_file();

// set a new highest level for player for this mission
}
#endif
void set_highest_level(uint8_t levelnum);

// gets the player's highest level from the file for this mission
int get_highest_level(void);

namespace dsx {
struct netgame_info;
void read_netgame_profile(struct netgame_info *ng);
void write_netgame_profile(struct netgame_info *ng);
}

#endif
