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

#include <cstddef>
#include "fwd-player.h"
#include "fwd-object.h"

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
struct player;

namespace dcx {
template <std::size_t>
struct PHYSFSX_gets_line_t;

constexpr unsigned LEVEL_NAME_LEN = 36;       //make sure this is multiple of 4!

// Current_level_num starts at 1 for the first level
// -1,-2,-3 are secret levels
// 0 used to mean not a real level loaded (i.e. editor generated level), but this hack has been removed
extern int Current_level_num, Next_level_num;
extern PHYSFSX_gets_line_t<LEVEL_NAME_LEN> Current_level_name;
extern array<obj_position, MAX_PLAYERS> Player_init;

// This is the highest level the player has ever reached
extern int Player_highest_level;
}

//
// New game sequencing functions
//

// starts a new game on the given level
#ifdef dsx
namespace dsx {
void StartNewGame(int start_level);

// starts the next level
void StartNewLevel(int level_num);

}
#endif
void InitPlayerObject();            //make sure player's object set up
namespace dsx {
void init_player_stats_game(ubyte pnum);      //clear all stats
}

// called when the player has finished a level
// if secret flag is true, advance to secret level, else next normal level
#ifdef dsx
namespace dsx {
void PlayerFinishedLevel(int secret_flag);

}
#endif
namespace dsx {
// called when the player has died
void DoPlayerDead(void);
}

#if defined(DXX_BUILD_DESCENT_I)
static inline void load_level_robots(int level_num)
{
	(void)level_num;
}
#elif defined(DXX_BUILD_DESCENT_II)
namespace dsx {
// load just the hxm file
void load_level_robots(int level_num);
extern int	First_secret_visit;
void ExitSecretLevel();
}
#endif

// load a level off disk. level numbers start at 1.
// Secret levels are -1,-2,-3
#ifdef dsx
namespace dsx {
void LoadLevel(int level_num, int page_in_textures);

}
#endif
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
void bash_to_shield(const vobjptr_t i);
#ifdef dsx
namespace dsx {
void create_player_appearance_effect(const object_base &player_obj);
void copy_defaults_to_robot(vobjptr_t objp);
void gameseq_remove_unused_players();
}
#endif

// Show endlevel bonus scores
namespace dcx {
// stuff for multiplayer
extern unsigned NumNetPlayerPositions;
extern fix StartingShields;
extern int	Do_appearance_effect;
}


int p_secret_level_destroyed(void);
namespace dsx {
#if defined(DXX_BUILD_DESCENT_II)
void do_cloak_invul_secret_stuff(fix64 old_gametime, player_info &player_info);
#endif
void EnterSecretLevel(void);
void init_player_stats_new_ship(ubyte pnum);
}
#endif
