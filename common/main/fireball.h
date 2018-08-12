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

#ifdef __cplusplus
#include "maths.h"
#include "fwd-partial_range.h"
#include "fwd-object.h"
#include "fwd-segment.h"
#include "fwd-vecmat.h"
#include "fwd-wall.h"
#include "pack.h"

enum powerup_type_t : uint8_t;

namespace dcx {
extern unsigned Num_exploding_walls;
}

#ifdef dsx
namespace dsx {

#if defined(DXX_BUILD_DESCENT_II)
struct disk_expl_wall
{
	int segnum, sidenum;
	fix time;
};
static_assert(sizeof(disk_expl_wall) == 12, "sizeof(disk_expl_wall) wrong");
#endif

imobjptridx_t object_create_explosion(vmsegptridx_t segnum, const vms_vector &position, fix size, int vclip_type);
void object_create_muzzle_flash(vmsegptridx_t segnum, const vms_vector &position, fix size, int vclip_type);

imobjptridx_t object_create_badass_explosion(imobjptridx_t objp, vmsegptridx_t segnum, const vms_vector &position, fix size, int vclip_type,
		fix maxdamage, fix maxdistance, fix maxforce, icobjptridx_t parent);

// blows up a badass weapon, creating the badass explosion
// return the explosion object
void explode_badass_weapon(vmobjptridx_t obj,const vms_vector &pos);

// blows up the player with a badass explosion
void explode_badass_player(vmobjptridx_t obj);

void explode_object(vmobjptridx_t obj,fix delay_time);
void do_explosion_sequence(vmobjptr_t obj);
void do_debris_frame(vmobjptridx_t obj);      // deal with debris for this frame

void draw_fireball(grs_canvas &, vcobjptridx_t obj);

void explode_wall(fvcvertptr &, vcsegptridx_t, unsigned sidenum, wall &);
void do_exploding_wall_frame(wall &);
void maybe_drop_net_powerup(powerup_type_t powerup_type, bool adjust_cap, bool random_player);
void maybe_replace_powerup_with_energy(object_base &del_obj);
}

namespace dcx {
void init_exploding_walls();
enum class explosion_vclip_stage : int
{
	s0,
	s1,
};
}

namespace dsx {
int get_explosion_vclip(const object_base &obj, explosion_vclip_stage stage);

#if defined(DXX_BUILD_DESCENT_II)
imobjptridx_t drop_powerup(int id, unsigned num, const vms_vector &init_vel, const vms_vector &pos, vmsegptridx_t segnum, bool player);

// creates afterburner blobs behind the specified object
void drop_afterburner_blobs(vmobjptr_t obj, int count, fix size_scale, fix lifetime);

/*
 * reads n expl_wall structs from a PHYSFS_File and swaps if specified
 */
void expl_wall_read_n_swap(fvmwallptr &, PHYSFS_File *fp, int swap, unsigned);
void expl_wall_write(fvcwallptr &, PHYSFS_File *);
extern fix	Flash_effect;
#endif

imsegidx_t pick_connected_segment(vcsegidx_t objp, int max_depth);
}
#endif

#endif
