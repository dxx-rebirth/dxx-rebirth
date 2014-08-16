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
 * Header for mission.h
 *
 */

#ifndef _MISSION_H
#define _MISSION_H

#include <memory>
#include "pstypes.h"
#include "inferno.h"

#ifdef __cplusplus

#define MAX_MISSIONS                    5000 // ZICO - changed from 300 to get more levels in list
#define MAX_LEVELS_PER_MISSION          127	// KREATOR - increased from 30 (limited by Demo and Multiplayer code)
static const ubyte MAX_SECRET_LEVELS_PER_MISSION = 127;	// KREATOR - increased from 6 (limited by Demo and Multiplayer code)
#define MISSION_NAME_LEN                25

#if defined(DXX_BUILD_DESCENT_I)
#define D1_MISSION_FILENAME             ""
#elif defined(DXX_BUILD_DESCENT_II)
#define D1_MISSION_FILENAME             "descent"
#endif
#define D1_MISSION_NAME                 "Descent: First Strike"
#define D1_MISSION_HOGSIZE              6856701 // v1.4 - 1.5
#define D1_MISSION_HOGSIZE2             6856183 // v1.4 - 1.5 - different patch-way
#define D1_10_MISSION_HOGSIZE           7261423 // v1.0
#define D1_MAC_MISSION_HOGSIZE          7456179
#define D1_OEM_MISSION_NAME             "Destination Saturn"
#define D1_OEM_MISSION_HOGSIZE          4492107 // v1.4a
#define D1_OEM_10_MISSION_HOGSIZE       4494862 // v1.0
#define D1_SHAREWARE_MISSION_NAME       "Descent Demo"
#define D1_SHAREWARE_MISSION_HOGSIZE    2339773 // v1.4
#define D1_SHAREWARE_10_MISSION_HOGSIZE 2365676 // v1.0 - 1.2
#define D1_MAC_SHARE_MISSION_HOGSIZE    3370339

#if defined(DXX_BUILD_DESCENT_II)
#define SHAREWARE_MISSION_FILENAME  "d2demo"
#define SHAREWARE_MISSION_NAME      "Descent 2 Demo"
#define SHAREWARE_MISSION_HOGSIZE   2292566 // v1.0 (d2demo.hog)
#define MAC_SHARE_MISSION_HOGSIZE   4292746

#define OEM_MISSION_FILENAME        "d2"
#define OEM_MISSION_NAME            "D2 Destination:Quartzon"
#define OEM_MISSION_HOGSIZE         6132957 // v1.1

#define FULL_MISSION_FILENAME       "d2"
#define FULL_MISSION_HOGSIZE        7595079 // v1.1 - 1.2
#define FULL_10_MISSION_HOGSIZE     7107354 // v1.0
#define MAC_FULL_MISSION_HOGSIZE    7110007 // v1.1 - 1.2
#endif

//where the missions go
#define MISSION_DIR "missions/"

struct Mission {
	char    *filename;          // filename
	int     builtin_hogsize;    // the size of the hogfile for a builtin mission, and 0 for an add-on mission
	char	mission_name[MISSION_NAME_LEN+1];
	ubyte   anarchy_only_flag;  // if true, mission is only for anarchy
	char	*path;				// relative file path
	d_fname	briefing_text_filename; // name of briefing file
	d_fname	ending_text_filename; // name of ending file
	std::unique_ptr<ubyte[]>	secret_level_table; // originating level no for each secret level 
	// arrays of names of the level files
	std::unique_ptr<d_fname[]>	level_names;
	std::unique_ptr<d_fname[]>	secret_level_names;
	ubyte	last_level;
	sbyte	last_secret_level;
	ubyte	n_secret_levels;
#if defined(DXX_BUILD_DESCENT_II)
	ubyte	descent_version;	// descent 1 or descent 2?
	ubyte	enhanced;	// 0: mission has "name", 1:"xname", 2:"zname"
	std::unique_ptr<d_fname> alternate_ham_file;
#endif
	~Mission();
};

typedef std::unique_ptr<Mission> Mission_ptr;
extern Mission_ptr Current_mission; // current mission

#define Current_mission_longname	Current_mission->mission_name
#define Current_mission_filename	Current_mission->filename
#define Briefing_text_filename		Current_mission->briefing_text_filename
#define Ending_text_filename		Current_mission->ending_text_filename
#define Last_level			Current_mission->last_level
#define Last_secret_level		Current_mission->last_secret_level
#define N_secret_levels			Current_mission->n_secret_levels
#define Secret_level_table		Current_mission->secret_level_table
#define Level_names			Current_mission->level_names
#define Secret_level_names		Current_mission->secret_level_names

#if defined(DXX_BUILD_DESCENT_II)
#define is_SHAREWARE (Current_mission->builtin_hogsize == SHAREWARE_MISSION_HOGSIZE)
#define is_MAC_SHARE (Current_mission->builtin_hogsize == MAC_SHARE_MISSION_HOGSIZE)
#define is_D2_OEM (Current_mission->builtin_hogsize == OEM_MISSION_HOGSIZE)

#define EMULATING_D1		(Current_mission->descent_version == 1)
#endif
#define PLAYING_BUILTIN_MISSION	(Current_mission->builtin_hogsize != 0)
#define ANARCHY_ONLY_MISSION	(Current_mission->anarchy_only_flag == 1)

//values for d1 built-in mission
#define BIMD1_LAST_LEVEL		27
#define BIMD1_LAST_SECRET_LEVEL		-3
#define BIMD1_BRIEFING_FILE		"briefing.txb"
#define BIMD1_BRIEFING_FILE_OEM		"briefsat.txb"
#define BIMD1_ENDING_FILE		"endreg.txb"
#define BIMD1_ENDING_FILE_OEM		"endsat.txb"
#define BIMD1_ENDING_FILE_SHARE		"ending.txb"

#if defined(DXX_BUILD_DESCENT_II)
//values for d2 built-in mission
#define BIMD2_BRIEFING_FILE		"robot.txb"
#define BIMD2_BRIEFING_FILE_OEM		"brief2o.txb"
#define BIMD2_BRIEFING_FILE_SHARE	"brief2.txb"
#define BIMD2_ENDING_FILE_OEM		"end2oem.txb"
#define BIMD2_ENDING_FILE_SHARE		"ending2.txb"

int load_mission_ham();
#endif

//loads the named mission if it exists.
//Returns true if mission loaded ok, else false.
int load_mission_by_name (const char *mission_name);

//Handles creating and selecting from the mission list.
//Returns 1 if a mission was loaded.
int select_mission (int anarchy_mode, const char *message, int (*when_selected)(void));

#ifdef EDITOR
void create_new_mission(void);
#endif

#endif

#endif
