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
#endif

#ifdef __cplusplus
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

void plyr_save_stats();
}
#endif
#endif

struct hli
{
	char	Shortname[9];
	uint8_t LevelNum;
};

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
#include "kconfig.h"
#include "multi.h"
#include "fwd-weapon.h"

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

enum class RespawnPress : uint8_t
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

struct player_config : prohibit_void_ptr<player_config>
{
	ubyte ControlType;
	HudType HudMode;
	RespawnPress RespawnMode;
	uint8_t MouselookFlags;
	using primary_weapon_order = array<uint8_t, MAX_PRIMARY_WEAPONS + 1>;
	using secondary_weapon_order = array<uint8_t, MAX_SECONDARY_WEAPONS + 1>;
	primary_weapon_order PrimaryOrder;
	secondary_weapon_order SecondaryOrder;
	struct KeySettings {
		array<uint8_t, MAX_CONTROLS> Keyboard,
#if DXX_MAX_JOYSTICKS
			Joystick,
#endif
			Mouse;
	} KeySettings;
	array<ubyte, MAX_DXX_REBIRTH_CONTROLS> KeySettingsRebirth;
	Difficulty_level_type DefaultDifficulty;
	int AutoLeveling;
	uint16_t NHighestLevels;
	array<hli, MAX_MISSIONS> HighestLevels;
	array<int, 5> KeyboardSens;
	array<int, 6> JoystickSens;
	array<int, 6> JoystickDead;
	array<int, 6> JoystickLinear;
	array<int, 6> JoystickSpeed;
	ubyte MouseFlightSim;
	array<int, 6> MouseSens;
        array<int, 6> MouseOverrun;
	int MouseFSDead;
	int MouseFSIndicator;
	array<cockpit_mode_t, 2> CockpitMode; // 0 saves the "real" cockpit, 1 also saves letterbox and rear. Used to properly switch between modes and restore the real one.
#if defined(DXX_BUILD_DESCENT_II)
	array<int, 2> Cockpit3DView;
#endif
	array<ntstring<MAX_MESSAGE_LEN - 1>, 4> NetworkMessageMacro;
	int NetlifeKills;
	int NetlifeKilled;
	ubyte ReticleType;
	array<int, 4> ReticleRGBA;
	int ReticleSize;
#if defined(DXX_BUILD_DESCENT_II)
	MissileViewMode MissileViewEnabled;
	uint8_t ThiefModifierFlags;
	int HeadlightActiveDefault;
	int GuidedInBigWindow;
	ntstring<GUIDEBOT_NAME_LEN> GuidebotName, GuidebotNameReal;
	int EscortHotKeys;
#endif
	int PersistentDebris;
	int PRShot;
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
};
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

int new_player_config();

int read_player_file();

// set a new highest level for player for this mission
}
#endif
void set_highest_level(int levelnum);

// gets the player's highest level from the file for this mission
int get_highest_level(void);

namespace dsx {
struct netgame_info;
void read_netgame_profile(struct netgame_info *ng);
void write_netgame_profile(struct netgame_info *ng);
}

#endif
#endif
