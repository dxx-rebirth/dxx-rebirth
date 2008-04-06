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
