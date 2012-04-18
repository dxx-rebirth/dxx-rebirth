/* $Id: gameseq.h,v 1.1.1.1 2006/03/17 19:57:54 zicodxx Exp $ */
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
 * Prototypes for functions for game sequencing.
 *
 */


#ifndef _GAMESEQ_H
#define _GAMESEQ_H

#include "player.h"
#include "mission.h"
#include "object.h"

#define SUPER_MISSILE       0
#define SUPER_SEEKER        1
#define SUPER_SMARTBOMB     2
#define SUPER_SHOCKWAVE     3

#ifdef SHAREWARE
#define Last_level			7			//the number of the very last level for shareware
#define Last_secret_level	0 			// No secret levels!
// #else
// extern int Last_level,Last_secret_level;	//set by mission code
#endif

// extern int Secret_level_table[MAX_SECRET_LEVELS_PER_MISSION];

#define LEVEL_NAME_LEN 36       //make sure this is multiple of 4!

// Current_level_num starts at 1 for the first level
// -1,-2,-3 are secret levels
// 0 means not a real level loaded
extern int Current_level_num, Next_level_num;
extern char Current_level_name[LEVEL_NAME_LEN];
extern obj_position Player_init[MAX_PLAYERS];


// This is the highest level the player has ever reached
extern int Player_highest_level;

//
// New game sequencing functions
//

// starts a new game on the given level
void StartNewGame(int start_level);

// starts the next level
void StartNewLevel(int level_num);

// Actually does the work to start new level
void StartNewLevelSub(int level_num, int page_in_textures);

void InitPlayerObject();            //make sure player's object set up
void init_player_stats_game(ubyte pnum);      //clear all stats

// called when the player has finished a level
// if secret flag is true, advance to secret level, else next normal level
void PlayerFinishedLevel(int secret_flag);

// called when the player has died
void DoPlayerDead(void);

// load a level off disk. level numbers start at 1.
// Secret levels are -1,-2,-3
void LoadLevel(int level_num);

extern void gameseq_remove_unused_players();

extern void show_help();
extern void update_player_stats();

// from scores.c

extern void show_high_scores(int place);
extern void draw_high_scores(int place);
extern int add_player_to_high_scores(player *pp);
extern void input_name (int place);
extern int reset_high_scores();
extern void init_player_stats_level();

void open_message_window(void);
void close_message_window(void);

// create flash for player appearance
extern void create_player_appearance_effect(object *player_obj);

// goto whatever secrect level is appropriate given the current level
//extern int goto_secret_level();

// reset stuff so game is semi-normal when playing from editor
void editor_reset_stuff_on_level();

// Show endlevel bonus scores
extern void DoEndLevelScoreGlitz(int network);

// stuff for multiplayer
extern int NumNetPlayerPositions;

void bash_to_shield(int, char *);

#endif /* _GAMESEQ_H */
