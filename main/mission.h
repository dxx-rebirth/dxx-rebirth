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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/mission.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:42:23 $
 * 
 * Header for mission.h
 * 
 * $Log: mission.h,v $
 * Revision 1.1.1.1  2006/03/17 19:42:23  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 22:12:36  donut
 * Import of d1x 1.37 source.
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

#include "pstypes.h"

#define MAX_MISSIONS                 5000 // ZICO - changed from 100 to get more levels in list
#define MAX_LEVELS_PER_MISSION         30
#define MAX_SECRET_LEVELS_PER_MISSION   5
#define MISSION_NAME_LEN               21
#define MISSION_FILENAME_LEN            9

//mission list entry
typedef struct mle {
	char	filename[9];							//filename without extension
	char	mission_name[MISSION_NAME_LEN+1];
	ubyte	anarchy_only_flag;					//if true, mission is anarchy only
} mle;

extern mle Mission_list[MAX_MISSIONS];

extern int Current_mission_num;
extern char *Current_mission_filename,*Current_mission_longname;

//arrays of name of the level files
extern char Level_names[MAX_LEVELS_PER_MISSION][13];
extern char Secret_level_names[MAX_SECRET_LEVELS_PER_MISSION][13];

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
