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

#if defined(DXX_BUILD_DESCENT_I)
#include "pstypes.h"
#include "player.h"
#elif defined(DXX_BUILD_DESCENT_II)
#include "escort.h"
#endif

#ifdef __cplusplus

#define N_SAVE_SLOTS    10
#define GAME_NAME_LEN   25      // +1 for terminating zero = 26

#if defined(DXX_BUILD_DESCENT_I)
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
#endif

struct hli
{
	char	Shortname[9];
	ubyte	LevelNum;
};

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
#include "kconfig.h"
#include "multi.h"
#include "weapon.h"

struct player_config
{
	ubyte ControlType;
	array<ubyte, MAX_PRIMARY_WEAPONS + 1> PrimaryOrder;
	array<ubyte, MAX_SECONDARY_WEAPONS + 1> SecondaryOrder;
	array<array<ubyte, MAX_CONTROLS>, 3> KeySettings;
	array<ubyte, MAX_DXX_REBIRTH_CONTROLS> KeySettingsRebirth;
	int DefaultDifficulty;
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
	int MissileViewEnabled;
	int HeadlightActiveDefault;
	int GuidedInBigWindow;
	ntstring<GUIDEBOT_NAME_LEN> GuidebotName, GuidebotNameReal;
#endif
	int HudMode;
#if defined(DXX_BUILD_DESCENT_II)
	int EscortHotKeys;
#endif
	int PersistentDebris;
	int PRShot;
	ubyte NoRedundancy;
	ubyte MultiMessages;
	ubyte NoRankings;
#if defined(DXX_BUILD_DESCENT_I)
	ubyte BombGauge;
#endif
	ubyte AutomapFreeFlight;
	ubyte NoFireAutoselect;
	ubyte CycleAutoselectOnly;
	int AlphaEffects;
	int DynLightColor;
};
#endif

extern struct player_config PlayerCfg;

#ifndef EZERO
#define EZERO 0
#endif

// Used to save kconfig values to disk.
void write_player_file();

int new_player_config();

int read_player_file();

// set a new highest level for player for this mission
void set_highest_level(int levelnum);

// gets the player's highest level from the file for this mission
int get_highest_level(void);

struct netgame_info;
void read_netgame_profile(struct netgame_info *ng);
void write_netgame_profile(struct netgame_info *ng);

#endif
