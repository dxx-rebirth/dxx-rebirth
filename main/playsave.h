/*
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
 * Header for playsave.c
 *
 */

#ifndef _PLAYSAVE_H
#define _PLAYSAVE_H

#include "pstypes.h"
#include "kconfig.h"
#include "mission.h"
#include "weapon.h"
#include "multi.h"
#include "player.h"

#define N_SAVE_SLOTS		10
#define GAME_NAME_LEN	25		//+1 for terminating zero = 26

// NOTE: Obsolete structure - only kept for compability of the plr file
typedef struct saved_game {
	char		name[GAME_NAME_LEN+1];		//extra char for terminating zero
	struct player	player;
	int		difficulty_level;		//which level game is played at
	int		primary_weapon;		//which weapon selected
	int		secondary_weapon;		//which weapon selected
	int		cockpit_mode;			//which cockpit mode selected
	int		window_w,window_h;	//size of player's window
	int		next_level_num;		//which level we're going to
	int		auto_leveling_on;		//does player have autoleveling on?
} __pack__ saved_game;

typedef struct hli {
	char	Shortname[9];
	ubyte	LevelNum;
} hli;

typedef struct player_config
{
	ubyte ControlType;
	ubyte PrimaryOrder[MAX_PRIMARY_WEAPONS+1];
	ubyte SecondaryOrder[MAX_SECONDARY_WEAPONS+1];
	ubyte KeySettings[CONTROL_MAX_TYPES][MAX_CONTROLS];
	ubyte KeySettingsD1X[MAX_D1X_CONTROLS];
	int DefaultDifficulty;
	int AutoLeveling;
	short NHighestLevels;
	hli HighestLevels[MAX_MISSIONS];
	ubyte MouseSensitivityX;
	ubyte MouseSensitivityY;
	int MouseFilter;
	ubyte JoystickSensitivityX;
	ubyte JoystickSensitivityY;
	int JoystickDeadzone;
	int CockpitMode;
	char NetworkMessageMacro[4][MAX_MESSAGE_LEN];
	int NetlifeKills;
	int NetlifeKilled;
	ubyte ReticleOn;
	int HudMode;
	int PersistentDebris;
	int PRShot;
	int OglAlphaEffects;
	int OglReticle;
} __pack__ player_config;

extern struct player_config PlayerCfg;

extern int Default_leveling_on;

// adb: EZERO looks like a watcom extension that's only used here
#ifndef EZERO
#define EZERO 0
#endif

//Used to save kconfig values to disk.
int write_player_file();

int new_player_config();

int read_player_file();

//set a new highest level for player for this mission
void set_highest_level(int levelnum);

//gets the player's highest level from the file for this mission
int get_highest_level(void);

void plyr_read_stats();
void plyr_save_stats();
#endif
