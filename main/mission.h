/* $Id: mission.h,v 1.7 2003-01-07 03:10:20 btb Exp $ */
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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Header for mission.h
 *
 * Old Log:
 * Revision 1.1  1995/05/16  15:59:22  allender
 * Initial revision
 *
 * Revision 2.0  1995/02/27  11:31:35  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.6  1995/01/30  12:55:41  matt
 * Added vars to point to mission names
 *
 * Revision 1.5  1995/01/22  18:57:21  matt
 * Made player highest level work with missions
 *
 * Revision 1.4  1995/01/22  14:13:21  matt
 * Added flag in mission list for anarchy-only missions
 *
 * Revision 1.3  1995/01/21  23:13:12  matt
 * Made high scores with (not work, really) with loaded missions
 * Don't give player high score when quit game
 *
 * Revision 1.2  1995/01/20  22:47:53  matt
 * Mission system implemented, though imcompletely
 *
 * Revision 1.1  1995/01/20  13:42:26  matt
 * Initial revision
 *
 *
 */

#ifndef _MISSION_H
#define _MISSION_H

#include <pstypes.h>

#define MAX_MISSIONS                    300
#define MAX_LEVELS_PER_MISSION          30
#define MAX_SECRET_LEVELS_PER_MISSION   6
#define MISSION_NAME_LEN                25

#define D1_MISSION_FILENAME             "descent"
#define D1_MISSION_NAME                 "Descent: First Strike"
#define D1_MISSION_HOGSIZE              6856701
#define D1_SHAREWARE_MISSION_NAME       "Descent Demo"
#define D1_SHAREWARE_MISSION_HOGSIZE    2339773

#define SHAREWARE_MISSION_FILENAME  "d2demo"
#define SHAREWARE_MISSION_NAME      "Descent 2 Demo"
#define SHAREWARE_MISSION_HOGSIZE   2292566
#define MAC_SHARE_MISSION_HOGSIZE   4292746

#define OEM_MISSION_FILENAME        "d2"
#define OEM_MISSION_NAME            "D2 Destination:Quartzon"
#define OEM_MISSION_HOGSIZE         6132957

#define FULL_MISSION_FILENAME       "d2"
#define FULL_MISSION_HOGSIZE        7595079

//mission list entry
typedef struct mle {
	char    filename[9];                // path and filename without extension
	char    mission_name[MISSION_NAME_LEN+1];
	ubyte   anarchy_only_flag;          // if true, mission is anarchy only
	ubyte   location;                   // see defines below
	ubyte   descent_version;            // descent 1 or descent 2?
} mle;

//values that describe where a mission is located
#define ML_CURDIR       0
#define ML_MISSIONDIR   1
#define ML_CDROM        2

extern mle Mission_list[MAX_MISSIONS];

extern int Current_mission_num, Builtin_mission_num;
extern char *Current_mission_filename,*Current_mission_longname;
extern char Builtin_mission_filename[9];
extern int Builtin_mission_hogsize;

//arrays of name of the level files
extern char Level_names[MAX_LEVELS_PER_MISSION][FILENAME_LEN];
extern char Secret_level_names[MAX_SECRET_LEVELS_PER_MISSION][FILENAME_LEN];

//fills in the global list of missions.  Returns the number of missions
//in the list.  If anarchy_mode set, don't include non-anarchy levels.
//if there is only one mission, this function will call load_mission on it.
int build_mission_list(int anarchy_mode);

//loads the specfied mission from the mission list.  build_mission_list()
//must have been called.  If build_mission_list() returns 0, this function
//does not need to be called.  Returns true if mission loaded ok, else false.
int load_mission(int mission_num);

//loads the named mission if exists.
//Returns true if mission loaded ok, else false.
int load_mission_by_name(char *mission_name);

#endif
