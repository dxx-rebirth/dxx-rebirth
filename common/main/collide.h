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

#ifdef __cplusplus
#include <cstdint>
#include "fwd-object.h"
#include "fwd-segment.h"
#include "fwd-vecmat.h"
#include "fwd-window.h"

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
void collide_two_objects(vobjptridx_t A, vobjptridx_t B, vms_vector &collision_point);
window_event_result collide_object_with_wall(vobjptridx_t A, fix hitspeed, vsegptridx_t hitseg, short hitwall, const vms_vector &hitpt);
namespace dsx {
void apply_damage_to_player(object &player, cobjptridx_t killer, fix damage, uint8_t possibly_friendly);
}

// Returns 1 if robot died, else 0.
#ifdef dsx
namespace dsx {
int apply_damage_to_robot(vobjptridx_t robot, fix damage, objnum_t killer_objnum);

}
#endif
#define PERSISTENT_DEBRIS (PlayerCfg.PersistentDebris && !(Game_mode & GM_MULTI)) // no persistent debris in multi

#ifdef dsx
namespace dsx {
void collide_player_and_materialization_center(vobjptridx_t objp);
}
#endif
void collide_robot_and_materialization_center(vobjptridx_t objp);

bool scrape_player_on_wall(vobjptridx_t obj, vsegptridx_t hitseg, short hitwall, const vms_vector &hitpt);
#ifdef dsx
namespace dsx {
int maybe_detonate_weapon(vobjptridx_t obj0p, vobjptr_t obj, const vms_vector &pos);

}
#endif
void collide_player_and_nasty_robot(vobjptridx_t player, vobjptridx_t robot, const vms_vector &collision_point);

void net_destroy_controlcen(objptridx_t controlcen);
void collide_live_local_player_and_powerup(const vobjptridx_t powerup);
#if defined(DXX_BUILD_DESCENT_I)
#define check_effect_blowup(seg,side,pnt,blower,force_blowup_flag,remote) check_effect_blowup(seg,side,pnt)
#endif
#ifdef dsx
namespace dsx {
int check_effect_blowup(vsegptridx_t seg,int side,const vms_vector &pnt, const laser_parent &blower, int force_blowup_flag, int remote);
}
#endif
void apply_damage_to_controlcen(vobjptridx_t controlcen, fix damage, vcobjptr_t who);
void bump_one_object(object_base &obj0, const vms_vector &hit_dir, fix damage);
#ifdef dsx
namespace dsx {
void drop_player_eggs(vobjptridx_t playerobj);

}
#endif
#if defined(DXX_BUILD_DESCENT_II)
window_event_result do_final_boss_frame(void);
void do_final_boss_hacks(void);
enum class volatile_wall_result : int8_t
{
	none = -1,
	lava,
	water,
};
volatile_wall_result check_volatile_wall(vobjptridx_t obj,vcsegptr_t seg,int sidenum);
extern int	Final_boss_is_dead;
#endif
#endif

#endif
