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
 * Header file for multiplayer robot support.
 *
 */

#pragma once

#include "pstypes.h"

#ifdef __cplusplus
#include "fwd-player.h"	// playernum_t
#include "fwd-vecmat.h"

constexpr std::size_t MAX_ROBOTS_CONTROLLED = 5;
constexpr std::size_t HANDS_OFF_PERIOD = MAX_ROBOTS_CONTROLLED; // i.e. one slow above max

extern array<objnum_t, MAX_ROBOTS_CONTROLLED> robot_controlled;
extern array<int, MAX_ROBOTS_CONTROLLED> robot_agitation, robot_fired;

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
int multi_can_move_robot(vobjptridx_t objnum, int agitation);
void multi_send_robot_position(object &objnum, int fired);
void multi_send_robot_fire(vobjptridx_t objnum, int gun_num, const vms_vector &fire);
void multi_send_claim_robot(vobjptridx_t objnum);
void multi_send_robot_explode(objptridx_t objnum, objnum_t killer);
void multi_send_create_robot(int robotcen, objnum_t objnum, int type);
#ifdef dsx
namespace dsx {
void multi_send_boss_teleport(vobjptridx_t bossobjnum, vcsegidx_t where);
}
#endif
void multi_send_boss_cloak(objnum_t bossobjnum);
void multi_send_boss_start_gate(objnum_t bossobjnum);
void multi_send_boss_stop_gate(objnum_t bossobjnum);
void multi_send_boss_create_robot(vobjidx_t bossobjnum, vobjptridx_t objnum);
#ifdef dsx
namespace dsx {
int multi_explode_robot_sub(vobjptridx_t botnum);
}
#endif
void multi_drop_robot_powerups(vobjptridx_t objnum);
int multi_send_robot_frame(int sent);
#ifdef dsx
namespace dsx {
void multi_robot_request_change(vobjptridx_t robot, int playernum);
}
#endif
#if defined(DXX_BUILD_DESCENT_II)
void multi_send_thief_frame();
#endif
#endif

void multi_do_robot_explode(const ubyte *buf);
void multi_do_robot_position(playernum_t pnum, const ubyte *buf);
void multi_do_claim_robot(playernum_t pnum, const ubyte *buf);
void multi_do_release_robot(playernum_t pnum, const ubyte *buf);
#ifdef dsx
namespace dsx {
void multi_do_robot_fire(const ubyte *buf);
}
#endif
void multi_do_create_robot(playernum_t pnum, const ubyte *buf);
void multi_do_create_robot_powerups(playernum_t pnum, const ubyte *buf);
void multi_do_boss_teleport(playernum_t pnum, const ubyte *buf);
#ifdef dsx
namespace dsx {
void multi_do_boss_cloak(const ubyte *buf);
}
#endif
void multi_do_boss_start_gate(const ubyte *buf);
void multi_do_boss_stop_gate(const ubyte *buf);
void multi_do_boss_create_robot(playernum_t pnum, const ubyte *buf);

void multi_strip_robots(int playernum);
void multi_check_robot_timeout(void);

#endif
