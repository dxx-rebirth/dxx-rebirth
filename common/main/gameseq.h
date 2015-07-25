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
 * Prototypes for functions for game sequencing.
 *
 */

#pragma once

#include "fwd-player.h"

#ifdef __cplusplus
#include "fwdobject.h"

struct player;

template <std::size_t>
struct PHYSFSX_gets_line_t;

const unsigned LEVEL_NAME_LEN = 36;       //make sure this is multiple of 4!

// Current_level_num starts at 1 for the first level
// -1,-2,-3 are secret levels
// 0 means not a real level loaded
extern int Current_level_num, Next_level_num;
extern PHYSFSX_gets_line_t<LEVEL_NAME_LEN> Current_level_name;
extern array<obj_position, MAX_PLAYERS> Player_init;

// This is the highest level the player has ever reached
extern int Player_highest_level;

//
// New game sequencing functions
//

// starts a new game on the given level
void StartNewGame(int start_level);

// starts the next level
void StartNewLevel(int level_num);

void InitPlayerObject();            //make sure player's object set up
void init_player_stats_game(ubyte pnum);      //clear all stats

// called when the player has finished a level
// if secret flag is true, advance to secret level, else next normal level
void PlayerFinishedLevel(int secret_flag);

// called when the player has died
void DoPlayerDead(void);

#if defined(DXX_BUILD_DESCENT_I)
static inline void load_level_robots(int level_num)
{
	(void)level_num;
}
#elif defined(DXX_BUILD_DESCENT_II)
// load just the hxm file
void load_level_robots(int level_num);
extern int	First_secret_visit;
#endif

// load a level off disk. level numbers start at 1.
// Secret levels are -1,-2,-3
void LoadLevel(int level_num, int page_in_textures);

extern void gameseq_remove_unused_players();

extern void update_player_stats();

// from scores.c

extern void show_high_scores(int place);
extern void draw_high_scores(int place);
extern int add_player_to_high_scores(player *pp);
extern void input_name (int place);
extern int reset_high_scores();

void open_message_window(void);
void close_message_window(void);

// create flash for player appearance
void create_player_appearance_effect(vobjptridx_t player_obj);

// reset stuff so game is semi-normal when playing from editor
void editor_reset_stuff_on_level();

// Show endlevel bonus scores
extern void DoEndLevelScoreGlitz(int network);

// stuff for multiplayer
extern unsigned NumNetPlayerPositions;
extern fix StartingShields;
extern int	Do_appearance_effect;

void bash_to_shield(const vobjptr_t i);

int p_secret_level_destroyed(void);
void ExitSecretLevel(void);
void do_cloak_invul_secret_stuff(fix64 old_gametime);
void EnterSecretLevel(void);
void copy_defaults_to_robot(vobjptr_t objp);
void init_player_stats_new_ship(ubyte pnum);

#endif
