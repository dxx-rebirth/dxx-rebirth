/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 *
 * Header for escort.c
 *
 */

#pragma once

#include "maths.h"
#include "fwd-object.h"
#include "fwd-vclip.h"

#ifdef dsx
#if DXX_BUILD_DESCENT == 2
#include "fwd-robot.h"
#endif

namespace dsx {
#if DXX_BUILD_DESCENT == 1
static inline void detect_escort_goal_accomplished(const vmobjptridx_t &)
{
}
#elif defined(DXX_BUILD_DESCENT_II)
#define GUIDEBOT_NAME_LEN 9
struct netgame_info;
extern void change_guidebot_name(void);
extern void do_escort_menu(void);
void detect_escort_goal_accomplished(vmobjptridx_t index);
void detect_escort_goal_fuelcen_accomplished();
void set_escort_special_goal(d_unique_buddy_state &BuddyState, int key);
void init_buddy_for_level(void);
void invalidate_escort_goal(d_unique_buddy_state &);
unsigned check_warn_local_player_can_control_guidebot(fvcobjptr &vcobjptr, const d_unique_buddy_state &, const netgame_info &Netgame);

enum escort_goal_t : uint8_t
{
	ESCORT_GOAL_UNSPECIFIED = UINT8_MAX,
	ESCORT_GOAL_BLUE_KEY = 1,
	ESCORT_GOAL_GOLD_KEY = 2,
	ESCORT_GOAL_RED_KEY = 3,
	ESCORT_GOAL_CONTROLCEN = 4,
	ESCORT_GOAL_EXIT = 5,

// Custom escort goals.
	ESCORT_GOAL_ENERGY = 6,
	ESCORT_GOAL_ENERGYCEN = 7,
	ESCORT_GOAL_SHIELD = 8,
	ESCORT_GOAL_POWERUP = 9,
	ESCORT_GOAL_ROBOT = 10,
	ESCORT_GOAL_HOSTAGE = 11,
	ESCORT_GOAL_PLAYER_SPEW = 12,
	ESCORT_GOAL_SCRAM = 13,
	ESCORT_GOAL_BOSS = 15,
	ESCORT_GOAL_MARKER1 = 16,
	ESCORT_GOAL_MARKER2 = 17,
	ESCORT_GOAL_MARKER3 = 18,
	ESCORT_GOAL_MARKER4 = 19,
	ESCORT_GOAL_MARKER5 = 20,
	ESCORT_GOAL_MARKER6 = 21,
	ESCORT_GOAL_MARKER7 = 22,
	ESCORT_GOAL_MARKER8 = 23,
	ESCORT_GOAL_MARKER9 = 24,
};
#endif
}
#endif
