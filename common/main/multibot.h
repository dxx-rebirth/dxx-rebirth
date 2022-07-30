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

#include <bitset>
#include "pstypes.h"

#include "fwd-player.h"	// playernum_t
#include "fwd-vclip.h"
#include "fwd-vecmat.h"
#include "robot.h"

namespace dcx {

enum class multi_send_robot_position_priority : uint8_t
{
	_0,
	_1,
	_2,
};

constexpr std::integral_constant<std::size_t, 5> MAX_ROBOTS_CONTROLLED{};
constexpr std::integral_constant<std::size_t, MAX_ROBOTS_CONTROLLED> HANDS_OFF_PERIOD{}; // i.e. one slow above max

extern std::array<objnum_t, MAX_ROBOTS_CONTROLLED> robot_controlled;
extern std::array<int, MAX_ROBOTS_CONTROLLED> robot_agitation;
extern std::bitset<MAX_ROBOTS_CONTROLLED> robot_fired;
}

#ifdef dsx
int multi_can_move_robot(vmobjptridx_t objnum, int agitation);
void multi_send_robot_position(object &objnum, multi_send_robot_position_priority fired);
void multi_send_robot_fire(vmobjptridx_t objnum, int gun_num, const vms_vector &fire);
void multi_send_claim_robot(vmobjptridx_t objnum);
void multi_send_robot_explode(imobjptridx_t objnum, objnum_t killer);
void multi_send_create_robot(station_number robotcen, objnum_t objnum, int type);
namespace dsx {
void multi_send_boss_teleport(vmobjptridx_t bossobjnum, vcsegidx_t where);
}
void multi_send_boss_cloak(objnum_t bossobjnum);
void multi_send_boss_start_gate(objnum_t bossobjnum);
void multi_send_boss_stop_gate(objnum_t bossobjnum);
void multi_send_boss_create_robot(vmobjidx_t bossobjnum, vmobjptridx_t objnum);
void multi_send_robot_frame();
namespace dsx {
int multi_explode_robot_sub(const d_robot_info_array &Robot_info, vmobjptridx_t botnum);
void multi_robot_request_change(vmobjptridx_t robot, int playernum);
#if defined(DXX_BUILD_DESCENT_II)
void multi_send_thief_frame();
#endif
}
#endif

void multi_do_create_robot_powerups(playernum_t pnum, const ubyte *buf);
void multi_do_boss_create_robot(playernum_t pnum, const ubyte *buf);

void multi_strip_robots(int playernum);
void multi_check_robot_timeout(void);
