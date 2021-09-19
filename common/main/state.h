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
 * Prototypes for state saving functions.
 *
 */

#pragma once

#if defined(DXX_BUILD_DESCENT_I)
#elif defined(DXX_BUILD_DESCENT_II)
#define SECRETB_FILENAME	PLAYER_DIRECTORY_STRING("secret.sgb")
#define SECRETC_FILENAME	PLAYER_DIRECTORY_STRING("secret.sgc")
#endif

#include <cstddef>
#include "dsx-ns.h"
#include "fwd-window.h"
#include "fwd-object.h"
#include "game.h"
#include "gameplayopt.h"

extern unsigned state_game_id;

#ifdef dsx
#include "fwd-player.h"
namespace dsx {

enum class secret_save
{
	none,
#if defined(DXX_BUILD_DESCENT_II)
	b,
	c,
#endif
};

enum class secret_restore
{
	none,
#if defined(DXX_BUILD_DESCENT_II)
	survived,
	died,
#endif
};

}
#endif

namespace dcx {

enum class blind_save
{
	no,
	yes,
};

enum class deny_save_result
{
	allowed,
	denied,
};

int state_get_game_id(const d_game_unique_state::savegame_file_path &filename);

}

#ifdef dsx
namespace dsx {
deny_save_result deny_save_game(fvcobjptr &vcobjptr, const d_level_unique_control_center_state &LevelUniqueControlCenterState);
deny_save_result deny_save_game(fvcobjptr &vcobjptr, const d_level_unique_control_center_state &LevelUniqueControlCenterState, const d_game_unique_state &GameUniqueState);
void state_poll_autosave_game(d_game_unique_state &GameUniqueState, const d_level_unique_object_state &LevelUniqueObjectState);
void state_set_immediate_autosave(d_game_unique_state &GameUniqueState);
void state_set_next_autosave(d_game_unique_state &GameUniqueState, std::chrono::steady_clock::time_point now, autosave_interval_type interval);
void state_set_next_autosave(d_game_unique_state &GameUniqueState, autosave_interval_type interval);
int state_save_all_sub(const char *filename, const char *desc);

d_game_unique_state::save_slot state_get_save_file(grs_canvas &canvas, d_game_unique_state::savegame_file_path &fname, d_game_unique_state::savegame_description *dsc, blind_save);
d_game_unique_state::save_slot state_get_restore_file(grs_canvas &canvas, d_game_unique_state::savegame_file_path &fname, blind_save);

#if defined(DXX_BUILD_DESCENT_I)
int state_restore_all_sub(const char *filename);
static inline void set_pos_from_return_segment(void)
{
}
int state_save_all(blind_save b);
static inline int state_save_all(secret_save, blind_save b)
{
	return state_save_all(b);
}
int state_restore_all(int in_game, std::nullptr_t, blind_save);
static inline int state_restore_all(int in_game, secret_restore, std::nullptr_t, blind_save blind)
{
	return state_restore_all(in_game, nullptr, blind);
}
window_event_result StartNewLevelSub(int level_num, int page_in_textures);
// Actually does the work to start new level
static inline window_event_result StartNewLevelSub(int level_num, int page_in_textures, secret_restore)
{
	return StartNewLevelSub(level_num, page_in_textures);
}
#elif defined(DXX_BUILD_DESCENT_II)
int state_restore_all_sub(const d_level_shared_destructible_light_state &LevelSharedDestructibleLightState, secret_restore, const char *filename);
void set_pos_from_return_segment(void);
int state_save_all(secret_save, blind_save);
int state_restore_all(int in_game, secret_restore, const char *filename_override, blind_save);
window_event_result StartNewLevelSub(int level_num, int page_in_textures, secret_restore);
#endif
}
#endif
