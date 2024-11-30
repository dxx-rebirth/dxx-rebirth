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
 * Header for collide.c
 *
 */

#pragma once

#include "maths.h"

#include <cstdint>
#include "fwd-object.h"
#include "fwd-segment.h"
#include "fwd-vclip.h"
#include "fwd-vecmat.h"
#include "fwd-window.h"
#include "robot.h"

#ifdef DXX_BUILD_DESCENT
namespace dcx {

enum class apply_damage_player : bool
{
	always,
	check_for_friendly,
};

}

namespace dsx {
void collide_two_objects(const d_robot_info_array &Robot_info, vmobjptridx_t A, vmobjptridx_t B, vms_vector &collision_point);
window_event_result collide_object_with_wall(
#if DXX_BUILD_DESCENT == 2
	const d_level_shared_destructible_light_state &LevelSharedDestructibleLightState,
#endif
	const d_robot_info_array &Robot_info, vmobjptridx_t A, fix hitspeed, vmsegptridx_t hitseg, sidenum_t hitwall, const vms_vector &hitpt);
void apply_damage_to_player(object &player, icobjptridx_t killer, fix damage, apply_damage_player possibly_friendly);
// Returns 1 if robot died, else 0.
int apply_damage_to_robot(const d_robot_info_array &Robot_info, vmobjptridx_t robot, fix damage, objnum_t killer_objnum);

#define PERSISTENT_DEBRIS (PlayerCfg.PersistentDebris && !(Game_mode & GM_MULTI)) // no persistent debris in multi

void collide_player_and_materialization_center(vmobjptridx_t objp);
void collide_robot_and_materialization_center(const d_robot_info_array &Robot_info, vmobjptridx_t objp);
bool scrape_player_on_wall(vmobjptridx_t obj, vmsegptridx_t hitseg, sidenum_t hitwall, const vms_vector &hitpt);
void collide_player_and_nasty_robot(const d_robot_info_array &Robot_info, vmobjptridx_t player, vmobjptridx_t robot, const vms_vector &collision_point);
}

namespace dcx {
void bump_one_object(object_base &obj0, const vms_vector &hit_dir, fix damage);
}
#if DXX_BUILD_DESCENT == 1
#define check_effect_blowup(DestructibleLightsState,Vclip,seg,side,pnt,blower,force_blowup_flag,remote) check_effect_blowup(Vclip,seg,side,pnt)
#endif
namespace dsx {
void collide_live_local_player_and_powerup(vmobjptridx_t powerup);
void net_destroy_controlcen_object(const d_robot_info_array &Robot_info, imobjptridx_t controlcen);
int check_effect_blowup(const d_level_shared_destructible_light_state &LevelSharedDestructibleLightState, const d_vclip_array &Vclip, vmsegptridx_t seg, sidenum_t side, const vms_vector &pnt, const laser_parent &blower, int force_blowup_flag, int remote);
void apply_damage_to_controlcen(const d_robot_info_array &Robot_info, vmobjptridx_t controlcen, fix damage, const object &who);
void drop_player_eggs(vmobjptridx_t playerobj);
enum class volatile_wall_result : int8_t
{
	none = -1,
	lava,
#if DXX_BUILD_DESCENT == 2
	water,
#endif
};
#if DXX_BUILD_DESCENT == 2
window_event_result do_final_boss_frame(void);
void do_final_boss_hacks(void);
volatile_wall_result check_volatile_wall(vmobjptridx_t obj, const unique_side &seg);
#endif
}
#endif
