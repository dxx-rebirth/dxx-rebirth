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

#ifndef _MISSION_H
#define _MISSION_H

#include <pstypes.h>

#define MAX_MISSIONS 						300
#define MAX_LEVELS_PER_MISSION			30
#define MAX_SECRET_LEVELS_PER_MISSION	6
#define MISSION_NAME_LEN 					25

//mission list entry
typedef struct mle {
	char	filename[9];                    //path and filename without extension
	char	mission_name[MISSION_NAME_LEN+1];
	ubyte	anarchy_only_flag;					//if true, mission is anarchy only
	ubyte	location;								//see defines below
} mle;

//values that describe where a mission is located
#define ML_CURDIR			0
#define ML_MISSIONDIR	1
#define ML_CDROM			2

extern mle Mission_list[MAX_MISSIONS];

extern int Current_mission_num;
extern char *Current_mission_filename,*Current_mission_longname;

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
