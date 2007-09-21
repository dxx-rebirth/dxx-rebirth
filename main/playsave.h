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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/playsave.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:42:10 $
 * 
 * Header for playsave.c
 * 
 * $Log: playsave.h,v $
 * Revision 1.1.1.1  2006/03/17 19:42:10  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 22:12:58  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:31:24  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.10  1995/01/22  18:57:04  matt
 * Made player highest level work with missions
 * 
 * Revision 1.9  1994/12/12  11:37:15  matt
 * Fixed auto leveling defaults & saving
 * 
 * Revision 1.8  1994/12/08  10:01:37  john
 * Changed the way the player callsign stuff works.
 * 
 * Revision 1.7  1994/11/25  22:46:56  matt
 * Made saved game descriptions longer
 * 
 * Revision 1.6  1994/10/24  20:00:02  john
 * Added prototype for read_player_file.
 * 
 * Revision 1.5  1994/10/17  13:07:12  john
 * Moved the descent.cfg info into the player config file.
 * 
 * Revision 1.4  1994/10/09  14:54:32  matt
 * Made player cockpit state & window size save/restore with saved games & automap
 * 
 * Revision 1.3  1994/10/08  23:08:09  matt
 * Added error check & handling for game load/save disk io
 * 
 * Revision 1.2  1994/09/28  17:25:06  matt
 * Added first draft of game save/load system
 * 
 * Revision 1.1  1994/09/27  15:47:23  matt
 * Initial revision
 * 
 * 
 */

#ifndef _PLAYSAVE_H
#define _PLAYSAVE_H

#define N_SAVE_SLOTS		10
#define GAME_NAME_LEN	25		//+1 for terminating zero = 26

extern int Default_leveling_on;

// adb: EZERO looks like a watcom extension that's only used here
#ifndef EZERO
#define EZERO 0
#endif

//fills in a list of pointers to strings describing saved games
//returns the number of non-empty slots
//returns -1 if this is a new player
int get_game_list(char *game_text[N_SAVE_SLOTS]);

//returns errno (0 == no error)
int save_player_game(int slot_num,char *text);

//returns errno (0 == no error)
int load_player_game(int slot_num);

//update the player's highest level.  returns errno (0 == no error)
int update_player_file();

//Used to save kconfig values to disk.
int write_player_file();

int new_player_config();
void init_game_list();

int read_player_file();

//set a new highest level for player for this mission
void set_highest_level(int levelnum);

//gets the player's highest level from the file for this mission
int get_highest_level(void);

// returns 1 if player exists (.plr file exists), 0 otherwise
int player_exists(const char *callsign);

//added on 10/15/98 by Victor Rachels to add lifetime effeciency
void plyr_read_stats();
void plyr_save_stats();
//end this section addition - Victor Rachels
#endif
