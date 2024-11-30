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
 * Header for fireball.c
 *
 */

#pragma once

#include <physfs.h>

#include "maths.h"
#include "fwd-powerup.h"
#include "fwd-segment.h"
#include "fwd-vecmat.h"
#include "fwd-wall.h"
#include "pack.h"
#include "robot.h"

namespace dcx {
extern unsigned Num_exploding_walls;
}

#ifdef dsx
namespace dsx {

#if DXX_BUILD_DESCENT == 2
struct disk_expl_wall
{
	uint32_t segnum;
	int sidenum;
	fix time;
};
static_assert(sizeof(disk_expl_wall) == 12, "sizeof(disk_expl_wall) wrong");
#endif

imobjptridx_t object_create_explosion_without_damage(const d_vclip_array &Vclip, vmsegptridx_t segnum, const vms_vector &position, fix size, vclip_index vclip_type);

imobjptridx_t object_create_badass_explosion(const d_robot_info_array &Robot_info, imobjptridx_t objp, vmsegptridx_t segnum, const vms_vector &position, fix size, vclip_index vclip_type,
		fix maxdamage, fix maxdistance, fix maxforce, icobjptridx_t parent);

// blows up a badass weapon, creating the badass explosion
// return the explosion object
void explode_badass_weapon(const d_robot_info_array &Robot_info, vmobjptridx_t obj,const vms_vector &pos);

// blows up the player with a badass explosion
void explode_badass_player(const d_robot_info_array &Robot_info, vmobjptridx_t obj);

void explode_object(d_level_unique_object_state &LevelUniqueObjectState, const d_robot_info_array &Robot_info, const d_level_shared_segment_state &LevelSharedSegmentState, d_level_unique_segment_state &LevelUniqueSegmentState, vmobjptridx_t obj, fix delay_time);
void do_explosion_sequence(const d_robot_info_array &Robot_info, object &obj);
void do_debris_frame(const d_robot_info_array &Robot_info, vmobjptridx_t obj);      // deal with debris for this frame

void draw_fireball(const d_vclip_array &Vclip, grs_canvas &, vcobjptridx_t obj);

void explode_wall(fvcvertptr &, vcsegptridx_t, sidenum_t sidenum, wall &);
unsigned do_exploding_wall_frame(const d_robot_info_array &Robot_info, wall &);
void maybe_drop_net_powerup(powerup_type_t powerup_type, bool adjust_cap, bool random_player);
void maybe_replace_powerup_with_energy(object_base &del_obj);
}

namespace dcx {
void init_exploding_walls();
enum class explosion_vclip_stage : bool
{
	s0,
	s1,
};
}

namespace dsx {
vclip_index get_explosion_vclip(const d_robot_info_array &Robot_info, const object_base &obj, explosion_vclip_stage stage);

imobjptridx_t drop_powerup(d_level_unique_object_state &LevelUniqueObjectState, const d_level_shared_segment_state &LevelSharedSegmentState, d_level_unique_segment_state &LevelUniqueSegmentState, const d_vclip_array &Vclip, powerup_type_t id, const vms_vector &init_vel, const vms_vector &pos, vmsegptridx_t segnum, bool player);
bool drop_powerup(d_level_unique_object_state &LevelUniqueObjectState, const d_level_shared_segment_state &LevelSharedSegmentState, d_level_unique_segment_state &LevelUniqueSegmentState, const d_vclip_array &Vclip, powerup_type_t id, unsigned num, const vms_vector &init_vel, const vms_vector &pos, vmsegptridx_t segnum, bool player);

#if DXX_BUILD_DESCENT == 2
// creates afterburner blobs behind the specified object
void drop_afterburner_blobs(object &obj, int count, fix size_scale, fix lifetime);

/*
 * reads n expl_wall structs from a PHYSFS_File and swaps if specified
 */
void expl_wall_read_n_swap(fvmwallptr &, PHYSFS_File *fp, physfsx_endian swap, unsigned);
void expl_wall_write(fvcwallptr &, PHYSFS_File *);
extern fix	Flash_effect;
#endif

vmsegidx_t choose_thief_recreation_segment(fvcsegptr &vcsegptr, fvcwallptr &vcwallptr, const vcsegidx_t plrseg);
}
#endif
