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
 * Code for rendering & otherwise dealing with explosions
 *
 */

#include <algorithm>
#include <numeric>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "dxxerror.h"
#include "maths.h"
#include "vecmat.h"
#include "gr.h"
#include "3d.h"

#include "inferno.h"
#include "object.h"
#include "vclip.h"
#include "game.h"
#include "robot.h"
#include "sounds.h"
#include "player.h"
#include "gauges.h"
#include "powerup.h"
#include "bm.h"
#include "ai.h"
#include "weapon.h"
#include "fireball.h"
#include "collide.h"
#include "newmenu.h"
#include "gameseq.h"
#include "physics.h"
#include "scores.h"
#include "laser.h"
#include "wall.h"
#include "multi.h"
#include "endlevel.h"
#include "timer.h"
#include "fuelcen.h"
#include "playsave.h"
#include "cntrlcen.h"
#include "gameseg.h"
#include "automap.h"
#include "byteutil.h"

#include "compiler-range_for.h"
#include "partial_range.h"
#include "segiter.h"

using std::min;

#define EXPLOSION_SCALE (F1_0*5/2)		//explosion is the obj size times this 

//--unused-- ubyte	Frame_processed[MAX_OBJECTS];

namespace dcx {

unsigned Num_exploding_walls;

void init_exploding_walls()
{
	Num_exploding_walls = 0;
}

}

namespace dsx {

#if defined(DXX_BUILD_DESCENT_II)
fix	Flash_effect=0;
constexpr int	PK1=1, PK2=8;
#endif

static bool can_collide(const object *const weapon_object, const object_base &iter_object, const object *const parent_object)
{
#if defined(DXX_BUILD_DESCENT_I)
	(void)weapon_object;
#elif defined(DXX_BUILD_DESCENT_II)
	if (weapon_object == &iter_object)
		return false;
	if (iter_object.type == OBJ_NONE)
		return false;
	if (iter_object.flags & OF_SHOULD_BE_DEAD)
		return false;
#endif
	switch (iter_object.type)
	{
#if defined(DXX_BUILD_DESCENT_II)
		case OBJ_WEAPON:
			return is_proximity_bomb_or_smart_mine_or_placed_mine(get_weapon_id(iter_object));
#endif
		case OBJ_CNTRLCEN:
		case OBJ_PLAYER:
			return true;
		case OBJ_ROBOT:
			if (parent_object == nullptr)
				return false;
			if (parent_object->type != OBJ_ROBOT)
				return true;
			return get_robot_id(*parent_object) != iter_object.id;
		default:
			return false;
	}
}

static imobjptridx_t object_create_explosion_sub(fvmobjptridx &vmobjptridx, const imobjptridx_t objp, const vmsegptridx_t segnum, const vms_vector &position, fix size, int vclip_type, fix maxdamage, fix maxdistance, fix maxforce, const icobjptridx_t parent )
{
	auto obj = obj_create( OBJ_FIREBALL,vclip_type,segnum,position,&vmd_identity_matrix,size,
					CT_EXPLOSION,MT_NONE,RT_FIREBALL);

	if (obj == object_none)
	{
		return object_none;
	}

	//now set explosion-specific data

	obj->lifeleft = Vclip[vclip_type ].play_time;
	obj->ctype.expl_info.spawn_time = -1;
	obj->ctype.expl_info.delete_objnum = object_none;
	obj->ctype.expl_info.delete_time = -1;

	if (maxdamage > 0) {
		fix force;
		vms_vector pos_hit, vforce;
		fix damage;
		// -- now legal for badass explosions on a wall. Assert(objp != NULL);

		range_for (const auto &&obj0p, vmobjptridx)
		{
			//	Weapons used to be affected by badass explosions, but this introduces serious problems.
			//	When a smart bomb blows up, if one of its children goes right towards a nearby wall, it will
			//	blow up, blowing up all the children.  So I remove it.  MK, 09/11/94
			if (can_collide(objp, obj0p, parent))
			{
				const auto dist = vm_vec_dist_quick( obj0p->pos, obj->pos );
				// Make damage be from 'maxdamage' to 0.0, where 0.0 is 'maxdistance' away;
				if ( dist < maxdistance ) {
					if (object_to_object_visibility(obj, obj0p, FQ_TRANSWALL)) {

						damage = maxdamage - fixmuldiv( dist, maxdamage, maxdistance );
						force = maxforce - fixmuldiv( dist, maxforce, maxdistance );

						// Find the force vector on the object
						vm_vec_normalized_dir_quick( vforce, obj0p->pos, obj->pos );
						vm_vec_scale(vforce, force );
	
						// Find where the point of impact is... ( pos_hit )
						vm_vec_scale(vm_vec_sub(pos_hit, obj->pos, obj0p->pos), fixdiv(obj0p->size, obj0p->size + dist));
	
						switch ( obj0p->type )	{
#if defined(DXX_BUILD_DESCENT_II)
							case OBJ_WEAPON:
								phys_apply_force(obj0p,vforce);

								if (is_proximity_bomb_or_smart_mine(get_weapon_id(obj0p)))
								{		//prox bombs have chance of blowing up
									if (fixmul(dist,force) > i2f(8000)) {
										obj0p->flags |= OF_SHOULD_BE_DEAD;
										explode_badass_weapon(obj0p, obj0p->pos);
									}
								}
								break;
#endif
							case OBJ_ROBOT:
								{
								phys_apply_force(obj0p,vforce);
#if defined(DXX_BUILD_DESCENT_II)
								//	If not a boss, stun for 2 seconds at 32 force, 1 second at 16 force
								if (objp != object_none && objp->type == OBJ_WEAPON && !Robot_info[get_robot_id(obj0p)].boss_flag && Weapon_info[get_weapon_id(objp)].flash)
								{
									ai_static	*aip = &obj0p->ctype.ai_info;
									int			force_val = f2i(fixdiv(vm_vec_mag_quick(vforce) * Weapon_info[get_weapon_id(objp)].flash, FrameTime)/128) + 2;

									if (obj->ctype.ai_info.SKIP_AI_COUNT * FrameTime < F1_0) {
										aip->SKIP_AI_COUNT += force_val;
										obj0p->mtype.phys_info.rotthrust.x = ((d_rand() - 16384) * force_val)/16;
										obj0p->mtype.phys_info.rotthrust.y = ((d_rand() - 16384) * force_val)/16;
										obj0p->mtype.phys_info.rotthrust.z = ((d_rand() - 16384) * force_val)/16;
										obj0p->mtype.phys_info.flags |= PF_USES_THRUST;

										//@@if (Robot_info[obj0p->id].companion)
										//@@	buddy_message("Daisy, Daisy, Give me...");
									} else
										aip->SKIP_AI_COUNT--;
								}
#endif

								//	When a robot gets whacked by a badass force, he looks towards it because robots tend to get blasted from behind.
								{
									vms_vector neg_vforce;
									neg_vforce.x = vforce.x * -2 * (7 - Difficulty_level)/8;
									neg_vforce.y = vforce.y * -2 * (7 - Difficulty_level)/8;
									neg_vforce.z = vforce.z * -2 * (7 - Difficulty_level)/8;
									phys_apply_rot(obj0p,neg_vforce);
								}
								if ( obj0p->shields >= 0 ) {
#if defined(DXX_BUILD_DESCENT_II)
									const auto &robot_info = Robot_info[get_robot_id(obj0p)];
									if (robot_info.boss_flag >= BOSS_D2 && Boss_invulnerable_matter[robot_info.boss_flag - BOSS_D2])
											damage /= 4;
#endif
									if (apply_damage_to_robot(obj0p, damage, parent))
										if ((objp != object_none) && (parent == get_local_player().objnum))
											add_points_to_score(ConsoleObject->ctype.player_info, Robot_info[get_robot_id(obj0p)].score_value);
								}
#if defined(DXX_BUILD_DESCENT_II)
								if (objp != object_none && Robot_info[get_robot_id(obj0p)].companion && !Weapon_info[get_weapon_id(objp)].flash)
								{
									static const char ouch_str[] = "ouch! " "ouch! " "ouch! " "ouch! ";
									int	count;

									count = f2i(damage/8);
									if (count > 4)
										count = 4;
									else if (count <= 0)
										count = 1;
									buddy_message_str(&ouch_str[(4 - count) * 6]);
								}
#endif
								break;
								}
							case OBJ_CNTRLCEN:
								if (parent != object_none && obj0p->shields >= 0)
								{
									apply_damage_to_controlcen(obj0p, damage, parent );
								}
								break;
							case OBJ_PLAYER:	{
								icobjptridx_t killer = object_none;
#if defined(DXX_BUILD_DESCENT_II)
								//	Hack! Warning! Test code!
								if (objp != object_none && Weapon_info[get_weapon_id(objp)].flash && get_player_id(obj0p) == Player_num)
								{
									int	fe;

									fe = min(F1_0 * 4, force*Weapon_info[get_weapon_id(objp)].flash / 32);	//	For four seconds or less

									if (objp->ctype.laser_info.parent_signature == ConsoleObject->signature) {
										fe /= 2;
										force /= 2;
									}
									if (force > F1_0) {
										Flash_effect = fe;
										PALETTE_FLASH_ADD(PK1 + f2i(PK2*force), PK1 + f2i(PK2*force), PK1 + f2i(PK2*force));
									}
								}
#endif
								if ((objp != object_none) && (Game_mode & GM_MULTI) && (objp->type == OBJ_PLAYER)) {
									killer = objp;
								}
								auto vforce2 = vforce;
								if (parent != object_none ) {
									killer = parent;
									if (killer != ConsoleObject)		// if someone else whacks you, cut force by 2x
									{
										vforce2.x /= 2;	vforce2.y /= 2;	vforce2.z /= 2;
									}
								}
								vforce2.x /= 2;	vforce2.y /= 2;	vforce2.z /= 2;

								phys_apply_force(obj0p,vforce);
								phys_apply_rot(obj0p,vforce2);
								if (obj0p->shields >= 0)
								{
#if defined(DXX_BUILD_DESCENT_II)
								if (Difficulty_level == 0)
									damage /= 4;
#endif
									apply_damage_to_player(obj0p, killer, damage, 1 );
								}
							}
								break;

							default:
								Int3();	//	Illegal object type
						}	// end switch
					} else {
						;
					}	// end if (object_to_object_visibility...
				}	// end if (dist < maxdistance)
			}
		}	// end for
	}	// end if (maxdamage...

	return obj;

}

void object_create_muzzle_flash(const vmsegptridx_t segnum, const vms_vector &position, fix size, int vclip_type )
{
	object_create_explosion_sub(vmobjptridx, object_none, segnum, position, size, vclip_type, 0, 0, 0, object_none );
}

imobjptridx_t object_create_explosion(const vmsegptridx_t segnum, const vms_vector &position, fix size, int vclip_type )
{
	return object_create_explosion_sub(vmobjptridx, object_none, segnum, position, size, vclip_type, 0, 0, 0, object_none );
}

imobjptridx_t object_create_badass_explosion(const imobjptridx_t objp, const vmsegptridx_t segnum, const vms_vector &position, fix size, int vclip_type, fix maxdamage, fix maxdistance, fix maxforce, const icobjptridx_t parent )
{
	const imobjptridx_t rval = object_create_explosion_sub(vmobjptridx, objp, segnum, position, size, vclip_type, maxdamage, maxdistance, maxforce, parent );

	if ((objp != object_none) && (objp->type == OBJ_WEAPON))
		create_weapon_smart_children(objp);
	return rval;
}

//blows up a badass weapon, creating the badass explosion
//return the explosion object
void explode_badass_weapon(const vmobjptridx_t obj,const vms_vector &pos)
{
	const auto weapon_id = get_weapon_id(obj);
	const weapon_info *wi = &Weapon_info[weapon_id];

	Assert(wi->damage_radius);
#if defined(DXX_BUILD_DESCENT_II)
	if (weapon_id == weapon_id_type::EARTHSHAKER_ID || weapon_id == weapon_id_type::ROBOT_EARTHSHAKER_ID)
		smega_rock_stuff();
#endif
	digi_link_sound_to_object(SOUND_BADASS_EXPLOSION, obj, 0, F1_0, sound_stack::allow_stacking);

	object_create_badass_explosion(obj, vmsegptridx(obj->segnum), pos,
	                                      wi->impact_size,
	                                      wi->robot_hit_vclip,
	                                      wi->strength[Difficulty_level],
	                                      wi->damage_radius,wi->strength[Difficulty_level],
	                                      imobjptridx(obj->ctype.laser_info.parent_num));

}

static void explode_badass_object(fvmsegptridx &vmsegptridx, const vmobjptridx_t objp, fix damage, fix distance, fix force)
{
	const auto &&rval = object_create_badass_explosion(objp, vmsegptridx(objp->segnum), objp->pos, objp->size,
					get_explosion_vclip(objp, explosion_vclip_stage::s0),
					damage, distance, force,
					objp);
	if (rval != object_none)
		digi_link_sound_to_object(SOUND_BADASS_EXPLOSION, rval, 0, F1_0, sound_stack::allow_stacking);
}

//blows up the player with a badass explosion
//return the explosion object
void explode_badass_player(const vmobjptridx_t objp)
{
	explode_badass_object(vmsegptridx, objp, F1_0*50, F1_0*40, F1_0*150);
}


#define DEBRIS_LIFE (f1_0 * (PERSISTENT_DEBRIS?60:2))		//lifespan in seconds

static void object_create_debris(fvmsegptridx &vmsegptridx, const object_base &parent, int subobj_num)
{
	Assert(parent.type == OBJ_ROBOT || parent.type == OBJ_PLAYER);

	const auto &&obj = obj_create(OBJ_DEBRIS, 0, vmsegptridx(parent.segnum), parent.pos, &parent.orient, Polygon_models[parent.rtype.pobj_info.model_num].submodel_rads[subobj_num],
				CT_DEBRIS,MT_PHYSICS,RT_POLYOBJ);

	if ((obj == object_none ) && (Highest_object_index >= MAX_OBJECTS-1)) {
//		Int3(); // this happens often and is normal :-)
		return;
	}
	if ( obj == object_none )
		return;				// Not enough debris slots!

	Assert(subobj_num < 32);

	//Set polygon-object-specific data

	obj->rtype.pobj_info.model_num = parent.rtype.pobj_info.model_num;
	obj->rtype.pobj_info.subobj_flags = 1<<subobj_num;
	obj->rtype.pobj_info.tmap_override = parent.rtype.pobj_info.tmap_override;

	//Set physics data for this object

	obj->mtype.phys_info.velocity.x = D_RAND_MAX/2 - d_rand();
	obj->mtype.phys_info.velocity.y = D_RAND_MAX/2 - d_rand();
	obj->mtype.phys_info.velocity.z = D_RAND_MAX/2 - d_rand();
	vm_vec_normalize_quick(obj->mtype.phys_info.velocity);
	vm_vec_scale(obj->mtype.phys_info.velocity,i2f(10 + (30 * d_rand() / D_RAND_MAX)));

	vm_vec_add2(obj->mtype.phys_info.velocity, parent.mtype.phys_info.velocity);

	// -- used to be: Notice, not random! vm_vec_make(&obj->mtype.phys_info.rotvel,10*0x2000/3,10*0x4000/3,10*0x7000/3);
	obj->mtype.phys_info.rotvel = {d_rand() + 0x1000, d_rand()*2 + 0x4000, d_rand()*3 + 0x2000};
	vm_vec_zero(obj->mtype.phys_info.rotthrust);

	obj->lifeleft = 3*DEBRIS_LIFE/4 + fixmul(d_rand(), DEBRIS_LIFE);	//	Some randomness, so they don't all go away at the same time.

	obj->mtype.phys_info.mass = fixmuldiv(parent.mtype.phys_info.mass, obj->size, parent.size);
	obj->mtype.phys_info.drag = 0; //fl2f(0.2);		//parent->mtype.phys_info.drag;

	if (PERSISTENT_DEBRIS)
	{
		obj->mtype.phys_info.flags |= PF_BOUNCE;
		obj->mtype.phys_info.drag = 128;
	}
}

void draw_fireball(grs_canvas &canvas, const vcobjptridx_t obj)
{
	if ( obj->lifeleft > 0 )
		draw_vclip_object(canvas, obj, obj->lifeleft, 0, Vclip[get_fireball_id(obj)]);
}

// --------------------------------------------------------------------------------------------------------------------
//	Return true if there is a door here and it is openable
//	It is assumed that the player has all keys.
static int door_is_openable_by_player(fvcwallptr &vcwallptr, const shared_segment &segp, const unsigned sidenum)
{
	const auto wall_num = segp.sides[sidenum].wall_num;

	if (wall_num == wall_none)
		return 0;						//	no wall here.

	auto &w = *vcwallptr(wall_num);
	const auto wall_type = w.type;
	//	Can't open locked doors.
	if ((wall_type == WALL_DOOR && (w.flags & WALL_DOOR_LOCKED)) || wall_type == WALL_CLOSED)
		return 0;

	return 1;
}

// --------------------------------------------------------------------------------------------------------------------
//	Return a segment %i segments away from initial segment.
//	Returns -1 if can't find a segment that distance away.
imsegidx_t pick_connected_segment(const vcsegidx_t start_seg, int max_depth)
{
	using std::swap;
	int		i;
	int		cur_depth;
	int		head, tail;
	constexpr unsigned QUEUE_SIZE = 64;
	array<segnum_t, QUEUE_SIZE * 2> seg_queue{};
	array<uint8_t, MAX_SEGMENTS> depth{};
	array<uint8_t, MAX_SIDES_PER_SEGMENT> side_rand;

	visited_segment_bitarray_t visited;

	head = 0;
	tail = 0;
	seg_queue[head++] = start_seg;

	cur_depth = 0;

	std::iota(side_rand.begin(), side_rand.end(), 0);

	//	Now, randomize a bit to start, so we don't always get started in the same direction.
	for (i=0; i<4; i++) {
		int	ind1;

		ind1 = (d_rand() * MAX_SIDES_PER_SEGMENT) >> 15;
		swap(side_rand[ind1], side_rand[i]);
	}

	while (tail != head) {
		int		sidenum, count;
		int		ind1, ind2;

		if (cur_depth >= max_depth) {
			return seg_queue[tail];
		}

		const auto &&segp = vcsegptr(seg_queue[tail++]);
		tail &= QUEUE_SIZE-1;

		//	to make random, switch a pair of entries in side_rand.
		ind1 = (d_rand() * MAX_SIDES_PER_SEGMENT) >> 15;
		ind2 = (d_rand() * MAX_SIDES_PER_SEGMENT) >> 15;
		swap(side_rand[ind1], side_rand[ind2]);
		count = 0;
		for (sidenum=ind1; count<MAX_SIDES_PER_SEGMENT; count++) {
			int	snrand;

			if (sidenum == MAX_SIDES_PER_SEGMENT)
				sidenum = 0;

			snrand = side_rand[sidenum];
			auto wall_num = segp->sides[snrand].wall_num;
			sidenum++;

			if ((wall_num == wall_none || door_is_openable_by_player(vcwallptr, segp, snrand)) && IS_CHILD(segp->children[snrand]))
			{
				if (!visited[segp->children[snrand]]) {
					seg_queue[head++] = segp->children[snrand];
					visited[segp->children[snrand]] = true;
					depth[segp->children[snrand]] = cur_depth+1;
					head &= QUEUE_SIZE-1;
					if (head > tail) {
						if (head == tail + QUEUE_SIZE-1)
							Int3();	//	queue overflow.  Make it bigger!
					} else
						if (head+QUEUE_SIZE == tail + QUEUE_SIZE-1)
							Int3();	//	queue overflow.  Make it bigger!
				}
			}
		}

		if (seg_queue[tail] > Highest_segment_index)
		{
			// -- Int3();	//	Something bad has happened.  Queue is trashed.  --MK, 12/13/94
			return segment_none;
		}
		cur_depth = depth[seg_queue[tail]];
	}
	return segment_none;
}

#if defined(DXX_BUILD_DESCENT_I)
#define BASE_NET_DROP_DEPTH 10
#elif defined(DXX_BUILD_DESCENT_II)
#define BASE_NET_DROP_DEPTH 8
#endif

static imsegidx_t pick_connected_drop_segment(const segment_array &Segments, fvcvertptr &vcvertptr, const vcsegidx_t start_seg, const unsigned cur_drop_depth, const vms_vector &player_pos, const vcsegptridx_t &player_seg)
{
	const auto segnum = pick_connected_segment(start_seg, cur_drop_depth);
	if (segnum == segment_none)
		return segnum;
	const auto &&segp = Segments.vcptridx(segnum);
	if (segp->special == SEGMENT_IS_CONTROLCEN)
		return segment_none;
	//don't drop in any children of control centers
	range_for (const auto ch, segp->children)
	{
		if (!IS_CHILD(ch))
			continue;
		auto &childsegp = *Segments.vcptr(ch);
		if (childsegp.special == SEGMENT_IS_CONTROLCEN)
			return segment_none;
	}
	//bail if not far enough from original position
	const auto &&tempv = compute_segment_center(vcvertptr, segp);
	if (find_connected_distance(player_pos, player_seg, tempv, segp, -1, WID_FLY_FLAG) < static_cast<fix>(i2f(20) * cur_drop_depth))
		return segment_none;
	return segnum;
}

//	------------------------------------------------------------------------------------------------------
//	Choose segment to drop a powerup in.
//	For all active net players, try to create a N segment path from the player.  If possible, return that
//	segment.  If not possible, try another player.  After a few tries, use a random segment.
//	Don't drop if control center in segment.
static vmsegptridx_t choose_drop_segment(segment_array &segments, const playernum_t drop_pnum)
{
	auto &vmsegptridx = segments.vmptridx;
	playernum_t	pnum = 0;
	int	cur_drop_depth;
	int	count;
	auto &drop_player = *vcplayerptr(drop_pnum);
	auto &drop_playerobj = *vmobjptr(drop_player.objnum);

	d_srand(static_cast<fix>(timer_query()));

	cur_drop_depth = BASE_NET_DROP_DEPTH + ((d_rand() * BASE_NET_DROP_DEPTH*2) >> 15);

	auto &player_pos = drop_playerobj.pos;
	const auto &&player_seg = segments.vcptridx(drop_playerobj.segnum);

	segnum_t	segnum = segment_none;
	for (; (segnum == segment_none) && (cur_drop_depth > BASE_NET_DROP_DEPTH/2); --cur_drop_depth)
	{
		pnum = (d_rand() * N_players) >> 15;
		count = 0;
#if defined(DXX_BUILD_DESCENT_I)
		while (count < N_players && (vcplayerptr(pnum)->connected == CONNECT_DISCONNECTED || pnum == drop_pnum))
#elif defined(DXX_BUILD_DESCENT_II)
		while (count < N_players && (vcplayerptr(pnum)->connected == CONNECT_DISCONNECTED || pnum == drop_pnum || ((Game_mode & (GM_TEAM|GM_CAPTURE)) && (get_team(pnum)==get_team(drop_pnum)))))
#endif
		{
			pnum = (pnum+1)%N_players;
			count++;
		}

		if (count == N_players) {
			//if can't valid non-player person, use the player
			pnum = drop_pnum;
		}

		segnum = pick_connected_drop_segment(segments, vcvertptr, vcobjptr(vcplayerptr(pnum)->objnum)->segnum, cur_drop_depth, player_pos, player_seg);
	}

	if (segnum == segment_none) {
		cur_drop_depth = BASE_NET_DROP_DEPTH;
		while (cur_drop_depth > 0 && segnum == segment_none) // before dropping in random segment, try to find ANY segment which is connected to the player responsible for the drop so object will not spawn in inaccessible areas
		{
			segnum = pick_connected_segment(vcobjptr(vcplayerptr(drop_pnum)->objnum)->segnum, --cur_drop_depth);
			if (segnum != segment_none && vcsegptr(segnum)->special == SEGMENT_IS_CONTROLCEN)
				segnum = segment_none;
		}
		if (segnum == segment_none) // basically it should be impossible segnum == -1 now... but oh well...
			return vmsegptridx(static_cast<segnum_t>((d_rand() * Highest_segment_index) >> 15));
	}
	return vmsegptridx(segnum);
}

//	------------------------------------------------------------------------------------------------------
//	(Re)spawns powerup if in a network game.
void maybe_drop_net_powerup(powerup_type_t powerup_type, bool adjust_cap, bool random_player)
{
        playernum_t pnum = Player_num;
	if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP)) {
		if ((Game_mode & GM_NETWORK) && adjust_cap)
		{
			MultiLevelInv_Recount(); // recount current items
			if (!MultiLevelInv_AllowSpawn(powerup_type))
				return;
		}

		if (Control_center_destroyed || (Network_status == NETSTAT_ENDLEVEL))
			return;

                if (random_player)
                {
                        uint_fast32_t failsafe_count = 0;
                        do {
                                pnum = d_rand() % MAX_PLAYERS;
                                if (failsafe_count > MAX_PLAYERS*4) // that was plenty of tries to find a good player...
                                {
                                        pnum = Player_num; // ... go with Player_num instead
                                        break;
                                }
                                failsafe_count++;
                        } while (vcplayerptr(pnum)->connected != CONNECT_PLAYING);
                }

//--old-- 		segnum = (d_rand() * Highest_segment_index) >> 15;
//--old-- 		Assert((segnum >= 0) && (segnum <= Highest_segment_index));
//--old-- 		if (segnum < 0)
//--old-- 			segnum = -segnum;
//--old-- 		while (segnum > Highest_segment_index)
//--old-- 			segnum /= 2;

		Net_create_loc = 0;
		const auto &&objnum = call_object_create_egg(vmobjptr(vcplayerptr(pnum)->objnum), 1, powerup_type);

		if (objnum == object_none)
			return;

		const auto &&segnum = choose_drop_segment(Segments, pnum);
		const auto &&new_pos = pick_random_point_in_seg(segnum);
		multi_send_create_powerup(powerup_type, segnum, objnum, new_pos);
		objnum->pos = new_pos;
		vm_vec_zero(objnum->mtype.phys_info.velocity);
		obj_relink(vmobjptr, vmsegptr, objnum, segnum);

		object_create_explosion(segnum, new_pos, i2f(5), VCLIP_POWERUP_DISAPPEARANCE );
	}
}

//	------------------------------------------------------------------------------------------------------
//	Return true if current segment contains some object.
static icsegptr_t segment_contains_powerup(const vcsegptr_t segnum, const powerup_type_t obj_id)
{
	range_for (const auto objp, objects_in(segnum, vcobjptridx, vcsegptr))
		if (objp->type == OBJ_POWERUP && get_powerup_id(objp) == obj_id)
			return segnum;
	return segment_none;
}

//	------------------------------------------------------------------------------------------------------
static icsegptr_t powerup_nearby_aux(const vcsegptr_t segnum, powerup_type_t object_id, uint_fast32_t depth)
{
	if (auto r = segment_contains_powerup(segnum, object_id))
		return r;
	if (! -- depth)
		return segment_none;
	range_for (const auto seg2, segnum->children)
	{
		if (seg2 != segment_none)
			if (auto r = powerup_nearby_aux(vcsegptr(seg2), object_id, depth))
				return r;
	}
	return segment_none;
}


//	------------------------------------------------------------------------------------------------------
//	Return true if some powerup is nearby (within 3 segments).
static icsegptr_t weapon_nearby(const object_base &objp, powerup_type_t weapon_id)
{
	return powerup_nearby_aux(vcsegptr(objp.segnum), weapon_id, 2);
}

//	------------------------------------------------------------------------------------------------------
void maybe_replace_powerup_with_energy(object_base &del_obj)
{
	int	weapon_index=-1;

	if (del_obj.contains_type != OBJ_POWERUP)
		return;

	switch (del_obj.contains_id) {
		case POW_CLOAK:
			if (weapon_nearby(del_obj, POW_CLOAK) != nullptr)
			{
				del_obj.contains_count = 0;
			}
			return;
		case POW_VULCAN_WEAPON:
			weapon_index = primary_weapon_index_t::VULCAN_INDEX;
			break;
		case POW_SPREADFIRE_WEAPON:
			weapon_index = primary_weapon_index_t::SPREADFIRE_INDEX;
			break;
		case POW_PLASMA_WEAPON:
			weapon_index = primary_weapon_index_t::PLASMA_INDEX;
			break;
		case POW_FUSION_WEAPON:
			weapon_index = primary_weapon_index_t::FUSION_INDEX;
			break;
#if defined(DXX_BUILD_DESCENT_II)
		case POW_GAUSS_WEAPON:
			weapon_index = primary_weapon_index_t::GAUSS_INDEX;
			break;
		case POW_HELIX_WEAPON:
			weapon_index = primary_weapon_index_t::HELIX_INDEX;
			break;
		case POW_PHOENIX_WEAPON:
			weapon_index = primary_weapon_index_t::PHOENIX_INDEX;
			break;
		case POW_OMEGA_WEAPON:
			weapon_index = primary_weapon_index_t::OMEGA_INDEX;
			break;
#endif
	}

	//	Don't drop vulcan ammo if player maxed out.
	auto &player_info = get_local_plrobj().ctype.player_info;
	if ((weapon_index_uses_vulcan_ammo(weapon_index) || del_obj.contains_id == POW_VULCAN_AMMO) &&
		player_info.vulcan_ammo >= VULCAN_AMMO_MAX)
		del_obj.contains_count = 0;
	else if (weapon_index != -1) {
		if (player_has_primary_weapon(player_info, weapon_index).has_weapon() || weapon_nearby(del_obj, static_cast<powerup_type_t>(del_obj.contains_id)) != nullptr)
		{
			if (d_rand() > 16384) {
#if defined(DXX_BUILD_DESCENT_I)
				del_obj.contains_count = 1;
#endif
				del_obj.contains_type = OBJ_POWERUP;
				if (weapon_index_uses_vulcan_ammo(weapon_index)) {
					del_obj.contains_id = POW_VULCAN_AMMO;
				}
				else {
					del_obj.contains_id = POW_ENERGY;
				}
			} else {
#if defined(DXX_BUILD_DESCENT_I)
				del_obj.contains_count = 0;
#elif defined(DXX_BUILD_DESCENT_II)
				del_obj.contains_type = OBJ_POWERUP;
				del_obj.contains_id = POW_SHIELD_BOOST;
#endif
			}
		}
	} else if (del_obj.contains_id == POW_QUAD_FIRE)
	{
		if ((player_info.powerup_flags & PLAYER_FLAGS_QUAD_LASERS) || weapon_nearby(del_obj, static_cast<powerup_type_t>(del_obj.contains_id)) != nullptr)
		{
			if (d_rand() > 16384) {
#if defined(DXX_BUILD_DESCENT_I)
				del_obj.contains_count = 1;
#endif
				del_obj.contains_type = OBJ_POWERUP;
				del_obj.contains_id = POW_ENERGY;
			} else {
#if defined(DXX_BUILD_DESCENT_I)
				del_obj.contains_count = 0;
#elif defined(DXX_BUILD_DESCENT_II)
				del_obj.contains_type = OBJ_POWERUP;
				del_obj.contains_id = POW_SHIELD_BOOST;
#endif
			}
		}
	}

	//	If this robot was gated in by the boss and it now contains energy, make it contain nothing,
	//	else the room gets full of energy.
	if ( (del_obj.matcen_creator == BOSS_GATE_MATCEN_NUM) && (del_obj.contains_id == POW_ENERGY) && (del_obj.contains_type == OBJ_POWERUP) ) {
		del_obj.contains_count = 0;
	}

	// Change multiplayer extra-lives into invulnerability
	if ((Game_mode & GM_MULTI) && (del_obj.contains_id == POW_EXTRA_LIFE))
	{
		del_obj.contains_id = POW_INVULNERABILITY;
	}
}

#if defined(DXX_BUILD_DESCENT_I)
static
#endif
imobjptridx_t drop_powerup(int id, const unsigned num, const vms_vector &init_vel, const vms_vector &pos, const vmsegptridx_t segnum, const bool player)
{
	imobjptridx_t	objnum = object_none;
	unsigned count;

			for (count=0; count<num; count++) {
				int	rand_scale;
				auto new_velocity = init_vel;
				const auto old_mag = vm_vec_mag_quick(init_vel);

				//	We want powerups to move more in network mode.
				if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_ROBOTS)) {
					rand_scale = 4;
					//	extra life powerups are converted to invulnerability in multiplayer, for what is an extra life, anyway?
					if (id == POW_EXTRA_LIFE)
						id = POW_INVULNERABILITY;
				} else
					rand_scale = 2;

				new_velocity.x += fixmul(old_mag+F1_0*32, d_rand()*rand_scale - 16384*rand_scale);
				new_velocity.y += fixmul(old_mag+F1_0*32, d_rand()*rand_scale - 16384*rand_scale);
				new_velocity.z += fixmul(old_mag+F1_0*32, d_rand()*rand_scale - 16384*rand_scale);

				// Give keys zero velocity so they can be tracked better in multi

				if ((Game_mode & GM_MULTI) && (id >= POW_KEY_BLUE) && (id <= POW_KEY_GOLD))
					vm_vec_zero(new_velocity);

				auto new_pos = pos;
//				new_pos.x += (d_rand()-16384)*8;
//				new_pos.y += (d_rand()-16384)*8;
//				new_pos.z += (d_rand()-16384)*8;

				if (Game_mode & GM_MULTI)
				{	
					if (Net_create_loc >= MAX_NET_CREATE_OBJECTS)
					{
						return object_none;
					}
#if defined(DXX_BUILD_DESCENT_II)
					if ((Game_mode & GM_NETWORK) && Network_status == NETSTAT_ENDLEVEL)
					 return object_none;
#endif
				}
				auto		obj = obj_create( OBJ_POWERUP, id, segnum, new_pos, &vmd_identity_matrix, Powerup_info[id].size, CT_POWERUP, MT_PHYSICS, RT_POWERUP);
				objnum = obj;

				if (objnum == object_none)
				{
					Int3();
					return object_none;
				}
#if defined(DXX_BUILD_DESCENT_II)
				if (player)
					obj->flags |= OF_PLAYER_DROPPED;
#endif

				if (Game_mode & GM_MULTI)
				{
					Net_create_objnums[Net_create_loc++] = objnum;
				}
				obj->mtype.phys_info.velocity = new_velocity;

				obj->mtype.phys_info.drag = 512;	//1024;
				obj->mtype.phys_info.mass = F1_0;

				obj->mtype.phys_info.flags = PF_BOUNCE;

				obj->rtype.vclip_info.vclip_num = Powerup_info[get_powerup_id(obj)].vclip_num;
				obj->rtype.vclip_info.frametime = Vclip[obj->rtype.vclip_info.vclip_num].frame_time;
				obj->rtype.vclip_info.framenum = 0;

				switch (get_powerup_id(obj)) {
					case POW_MISSILE_1:
					case POW_MISSILE_4:
					case POW_SHIELD_BOOST:
					case POW_ENERGY:
						obj->lifeleft = (d_rand() + F1_0*3) * 64;		//	Lives for 3 to 3.5 binary minutes (a binary minute is 64 seconds)
						if (Game_mode & GM_MULTI)
							obj->lifeleft /= 2;
						break;
#if defined(DXX_BUILD_DESCENT_II)
					case POW_OMEGA_WEAPON:
						if (!player)
							obj->ctype.powerup_info.count = MAX_OMEGA_CHARGE;
						break;
					case POW_GAUSS_WEAPON:
#endif
					case POW_VULCAN_WEAPON:
						if (!player)
							obj->ctype.powerup_info.count = VULCAN_WEAPON_AMMO_AMOUNT;
						break;
					default:
//						if (Game_mode & GM_MULTI)
//							obj->lifeleft = (d_rand() + F1_0*3) * 64;		//	Lives for 5 to 5.5 binary minutes (a binary minute is 64 seconds)
						break;
				}
			}
	return objnum;
}

static imobjptridx_t drop_robot_egg(const int type, const int id, const unsigned num, const vms_vector &init_vel, const vms_vector &pos, const vmsegptridx_t segnum)
{
	switch (type)
	{
		case OBJ_POWERUP:
			return drop_powerup(id, num, init_vel, pos, segnum, false);
		case OBJ_ROBOT:
			break;
		default:
			con_printf(CON_URGENT, DXX_STRINGIZE_FL(__FILE__, __LINE__, "ignoring invalid object type; expected OBJ_POWERUP or OBJ_ROBOT, got type=%i, id=%i"), type, id);
			return object_none;
	}
	imobjptridx_t	objnum = object_none;
	unsigned count;

			for (count=0; count<num; count++) {
				int	rand_scale;
				auto new_velocity = vm_vec_normalized_quick(init_vel);
				const auto old_mag = vm_vec_mag_quick(init_vel);

				//	We want powerups to move more in network mode.
//				if (Game_mode & GM_MULTI)
//					rand_scale = 4;
//				else
					rand_scale = 2;

				new_velocity.x += (d_rand()-16384)*2;
				new_velocity.y += (d_rand()-16384)*2;
				new_velocity.z += (d_rand()-16384)*2;

				vm_vec_normalize_quick(new_velocity);
				vm_vec_scale(new_velocity, (F1_0*32 + old_mag) * rand_scale);
				auto new_pos = pos;
				//	This is dangerous, could be outside mine.
//				new_pos.x += (d_rand()-16384)*8;
//				new_pos.y += (d_rand()-16384)*7;
//				new_pos.z += (d_rand()-16384)*6;

#if defined(DXX_BUILD_DESCENT_I)
				const auto robot_id = ObjId[type];
#elif defined(DXX_BUILD_DESCENT_II)
				const auto robot_id = id;
#endif
				const auto &&obj = robot_create(id, segnum, new_pos, &vmd_identity_matrix, Polygon_models[Robot_info[robot_id].model_num].rad, ai_behavior::AIB_NORMAL);

				objnum = obj;
				if (objnum == object_none)
				{
					Int3();
					return object_none;
				}

				if (Game_mode & GM_MULTI)
				{
					Net_create_objnums[Net_create_loc++] = objnum;
				}
				//Set polygon-object-specific data

				obj->rtype.pobj_info.model_num = Robot_info[get_robot_id(obj)].model_num;
				obj->rtype.pobj_info.subobj_flags = 0;

				//set Physics info
		
				obj->mtype.phys_info.velocity = new_velocity;

				obj->mtype.phys_info.mass = Robot_info[get_robot_id(obj)].mass;
				obj->mtype.phys_info.drag = Robot_info[get_robot_id(obj)].drag;

				obj->mtype.phys_info.flags |= (PF_LEVELLING);

				obj->shields = Robot_info[get_robot_id(obj)].strength;

				ai_local		*ailp = &obj->ctype.ai_info.ail;
				ailp->player_awareness_type = player_awareness_type_t::PA_WEAPON_ROBOT_COLLISION;
				ailp->player_awareness_time = F1_0*3;
				obj->ctype.ai_info.CURRENT_STATE = AIS_LOCK;
				obj->ctype.ai_info.GOAL_STATE = AIS_LOCK;
				obj->ctype.ai_info.REMOTE_OWNER = -1;
			}

#if defined(DXX_BUILD_DESCENT_II)
			// At JasenW's request, robots which contain robots
			// sometimes drop shields.
			if (d_rand() > 16384)
				drop_powerup(POW_SHIELD_BOOST, 1, init_vel, pos, segnum, false);
#endif
	return objnum;
}

#if defined(DXX_BUILD_DESCENT_II)
static bool skip_create_egg_powerup(const object &player, powerup_type_t powerup)
{
	fix current;
	auto &player_info = player.ctype.player_info;
	if (powerup == POW_SHIELD_BOOST)
		current = player.shields;
	else if (powerup == POW_ENERGY)
		current = player_info.energy;
	else
		return false;
	int limit;
	if (current >= i2f(150))
		limit = 8192;
	else if (current >= i2f(100))
		limit = 16384;
	else
		return false;
	return d_rand() > limit;
}
#endif

imobjptridx_t object_create_robot_egg(const int type, const int id, const int num, const vms_vector &init_vel, const vms_vector &pos, const vmsegptridx_t segnum)
{
#if defined(DXX_BUILD_DESCENT_II)
	if (!(Game_mode & GM_MULTI))
	{
		if (type == OBJ_POWERUP)
		{
			if (skip_create_egg_powerup(get_local_plrobj(), static_cast<powerup_type_t>(id)))
				return object_none;
		}
	}
#endif
	return drop_robot_egg(type, id, num, init_vel, pos, segnum);
}

imobjptridx_t object_create_robot_egg(const vmobjptr_t objp)
{
	return object_create_robot_egg(objp->contains_type, objp->contains_id, objp->contains_count, objp->mtype.phys_info.velocity, objp->pos, vmsegptridx(objp->segnum));
}

//	-------------------------------------------------------------------------------------------------------
//	Put count objects of type type (eg, powerup), id = id (eg, energy) into *objp, then drop them!  Yippee!
//	Returns created object number.
imobjptridx_t call_object_create_egg(const object_base &objp, const unsigned count, const int id)
{
	if (count > 0) {
		return drop_powerup(id, count, objp.mtype.phys_info.velocity, objp.pos, vmsegptridx(objp.segnum), true);
	}

	return object_none;
}

//what vclip does this explode with?
int get_explosion_vclip(const object_base &obj, explosion_vclip_stage stage)
{
	if (obj.type == OBJ_ROBOT)
	{
		const auto vclip_ptr = stage == explosion_vclip_stage::s0
			? &robot_info::exp1_vclip_num
			: &robot_info::exp2_vclip_num;
		const auto vclip_num = Robot_info[get_robot_id(obj)].*vclip_ptr;
		if (vclip_num > -1)
			return vclip_num;
	}
	else if (obj.type == OBJ_PLAYER && Player_ship->expl_vclip_num > -1)
			return Player_ship->expl_vclip_num;

	return VCLIP_SMALL_EXPLOSION;		//default
}

//blow up a polygon model
static void explode_model(object_base &obj)
{
	Assert(obj.render_type == RT_POLYOBJ);

	const auto poly_model_num = obj.rtype.pobj_info.model_num;
	const auto dying_model_num = Dying_modelnums[poly_model_num];
	const auto model_num = (dying_model_num != -1)
		? (obj.rtype.pobj_info.model_num = dying_model_num)
		: poly_model_num;
	const auto n_models = Polygon_models[model_num].n_models;
	if (n_models > 1) {
		for (unsigned i = 1; i < n_models; ++i)
#if defined(DXX_BUILD_DESCENT_II)
			if (!(i == 5 && obj.type == OBJ_ROBOT && get_robot_id(obj) == 44))	//energy sucker energy part
#endif
				object_create_debris(vmsegptridx, obj, i);

		//make parent object only draw center part
		obj.rtype.pobj_info.subobj_flags = 1;
	}
}

//if the object has a destroyed model, switch to it.  Otherwise, delete it.
static void maybe_delete_object(object_base &del_obj)
{
	const auto dead_modelnum = Dead_modelnums[del_obj.rtype.pobj_info.model_num];
	if (dead_modelnum != -1)
	{
		del_obj.rtype.pobj_info.model_num = dead_modelnum;
		del_obj.flags |= OF_DESTROYED;
	}
	else {		//normal, multi-stage explosion
		if (del_obj.type == OBJ_PLAYER)
			del_obj.render_type = RT_NONE;
		else
			del_obj.flags |= OF_SHOULD_BE_DEAD;
	}
}

//	-------------------------------------------------------------------------------------------------------
//blow up an object.  Takes the object to destroy, and the point of impact
void explode_object(const vmobjptridx_t hitobj,fix delay_time)
{
	if (hitobj->flags & OF_EXPLODING) return;

	if (delay_time) {		//wait a little while before creating explosion
		//create a placeholder object to do the delay, with id==-1
		auto obj = obj_create(OBJ_FIREBALL, -1, vmsegptridx(hitobj->segnum), hitobj->pos, &vmd_identity_matrix, 0,
						CT_EXPLOSION,MT_NONE,RT_NONE);
		if (obj == object_none ) {
			maybe_delete_object(hitobj);		//no explosion, die instantly
			Int3();
			return;
		}
		//now set explosion-specific data
	
		obj->lifeleft = delay_time;
		obj->ctype.expl_info.delete_objnum = hitobj;
		obj->ctype.expl_info.delete_time = -1;
		obj->ctype.expl_info.spawn_time = 0;

	}
	else {
		int vclip_num;

		vclip_num = get_explosion_vclip(hitobj, explosion_vclip_stage::s0);

		auto expl_obj = object_create_explosion(vmsegptridx(hitobj->segnum), hitobj->pos, fixmul(hitobj->size,EXPLOSION_SCALE), vclip_num);
	
		if (! expl_obj) {
			maybe_delete_object(hitobj);		//no explosion, die instantly
			return;
		}

		//don't make debris explosions have physics, because they often
		//happen when the debris has hit the wall, so the fireball is trying
		//to move into the wall, which shows off FVI problems.   	
		if (hitobj->type!=OBJ_DEBRIS && hitobj->movement_type==MT_PHYSICS) {
			expl_obj->movement_type = MT_PHYSICS;
			expl_obj->mtype.phys_info = hitobj->mtype.phys_info;
		}
	
		if (hitobj->render_type==RT_POLYOBJ && hitobj->type!=OBJ_DEBRIS)
			explode_model(hitobj);

		maybe_delete_object(hitobj);
	}

	hitobj->flags |= OF_EXPLODING;		//say that this is blowing up
	hitobj->control_type = CT_NONE;		//become inert while exploding

}


//do whatever needs to be done for this piece of debris for this frame
void do_debris_frame(const vmobjptridx_t obj)
{
	Assert(obj->control_type == CT_DEBRIS);

	if (obj->lifeleft < 0)
		explode_object(obj,0);

}

//do whatever needs to be done for this explosion for this frame
void do_explosion_sequence(const vmobjptr_t obj)
{
	Assert(obj->control_type == CT_EXPLOSION);

	//See if we should die of old age
	if (obj->lifeleft <= 0 ) 	{	// We died of old age
		obj->flags |= OF_SHOULD_BE_DEAD;
		obj->lifeleft = 0;
	}

	//See if we should create a secondary explosion
	if (obj->lifeleft <= obj->ctype.expl_info.spawn_time) {
		auto del_obj = vmobjptridx(obj->ctype.expl_info.delete_objnum);
		auto &spawn_pos = del_obj->pos;
		Assert(del_obj->type==OBJ_ROBOT || del_obj->type==OBJ_CLUTTER || del_obj->type==OBJ_CNTRLCEN || del_obj->type == OBJ_PLAYER);
		Assert(del_obj->segnum != segment_none);

		const auto &&expl_obj = [&]{
			const auto vclip_num = get_explosion_vclip(del_obj, explosion_vclip_stage::s1);
#if defined(DXX_BUILD_DESCENT_II)
			if (del_obj->type == OBJ_ROBOT)
			{
				const auto &ri = Robot_info[get_robot_id(del_obj)];
				if (ri.badass)
					return object_create_badass_explosion(object_none, vmsegptridx(del_obj->segnum), spawn_pos, fixmul(del_obj->size, EXPLOSION_SCALE), vclip_num, F1_0 * ri.badass, i2f(4) * ri.badass, i2f(35) * ri.badass, object_none);
			}
#endif
			return object_create_explosion(vmsegptridx(del_obj->segnum), spawn_pos, fixmul(del_obj->size, EXPLOSION_SCALE), vclip_num);
		}();

		if ((del_obj->contains_count > 0) && !(Game_mode & GM_MULTI)) { // Multiplayer handled outside of this code!!
			//	If dropping a weapon that the player has, drop energy instead, unless it's vulcan, in which case drop vulcan ammo.
			if (del_obj->contains_type == OBJ_POWERUP)
				maybe_replace_powerup_with_energy(del_obj);
			object_create_robot_egg(del_obj);
		} else if ((del_obj->type == OBJ_ROBOT) && !(Game_mode & GM_MULTI)) { // Multiplayer handled outside this code!!
			auto &robptr = Robot_info[get_robot_id(del_obj)];
			if (robptr.contains_count) {
				if (((d_rand() * 16) >> 15) < robptr.contains_prob) {
					del_obj->contains_count = ((d_rand() * robptr.contains_count) >> 15) + 1;
					del_obj->contains_type = robptr.contains_type;
					del_obj->contains_id = robptr.contains_id;
					maybe_replace_powerup_with_energy(del_obj);
					object_create_robot_egg(del_obj);
				}
			}
#if defined(DXX_BUILD_DESCENT_II)
			if (robot_is_thief(robptr))
				drop_stolen_items(del_obj);
			else if (robot_is_companion(robptr))
			{
				DropBuddyMarker(del_obj);
			}
#endif
		}

		auto &robptr = Robot_info[get_robot_id(del_obj)];
		if (robptr.exp2_sound_num > -1)
			digi_link_sound_to_pos(robptr.exp2_sound_num, vmsegptridx(del_obj->segnum), 0, spawn_pos, 0, F1_0);
			//PLAY_SOUND_3D( Robot_info[del_obj->id].exp2_sound_num, spawn_pos, del_obj->segnum  );

		obj->ctype.expl_info.spawn_time = -1;

		//make debris
		if (del_obj->render_type==RT_POLYOBJ)
			explode_model(del_obj);		//explode a polygon model

		//set some parm in explosion
		//If num_objects < MAX_USED_OBJECTS, expl_obj could be set to dead before this setting causing the delete_obj not to be removed. If so, directly delete del_obj
		if (expl_obj && !(expl_obj->flags & OF_SHOULD_BE_DEAD))
		{
			if (del_obj->movement_type == MT_PHYSICS) {
				expl_obj->movement_type = MT_PHYSICS;
				expl_obj->mtype.phys_info = del_obj->mtype.phys_info;
			}

			expl_obj->ctype.expl_info.delete_time = expl_obj->lifeleft/2;
			expl_obj->ctype.expl_info.delete_objnum = del_obj;
		}
		else {
			maybe_delete_object(del_obj);
		}

	}

	//See if we should delete an object
	if (obj->lifeleft <= obj->ctype.expl_info.delete_time) {
		const auto &&del_obj = vmobjptr(obj->ctype.expl_info.delete_objnum);
		obj->ctype.expl_info.delete_time = -1;
		maybe_delete_object(del_obj);
	}
}

#define EXPL_WALL_TIME					UINT16_MAX
#define EXPL_WALL_TOTAL_FIREBALLS	32
#if defined(DXX_BUILD_DESCENT_I)
#define EXPL_WALL_FIREBALL_SIZE 		0x48000	//smallest size
#elif defined(DXX_BUILD_DESCENT_II)
#define EXPL_WALL_FIREBALL_SIZE 		(0x48000*6/10)	//smallest size
#endif

//explode the given wall
void explode_wall(fvcvertptr &vcvertptr, const vcsegptridx_t segnum, const unsigned sidenum, wall &w)
{
	if (w.flags & WALL_EXPLODING)
		/* Already exploding */
		return;
	w.explode_time_elapsed = 0;
	w.flags |= WALL_EXPLODING;
	++ Num_exploding_walls;

	//play one long sound for whole door wall explosion
	const auto &&pos = compute_center_point_on_side(vcvertptr, segnum, sidenum);
	digi_link_sound_to_pos( SOUND_EXPLODING_WALL,segnum, sidenum, pos, 0, F1_0 );
}

void do_exploding_wall_frame(wall &w1)
{
	fix w1_explode_time_elapsed = w1.explode_time_elapsed;
	const fix oldfrac = fixdiv(w1_explode_time_elapsed, EXPL_WALL_TIME);

	w1_explode_time_elapsed += FrameTime;
	if (w1_explode_time_elapsed > EXPL_WALL_TIME)
		w1_explode_time_elapsed = EXPL_WALL_TIME;
	w1.explode_time_elapsed = w1_explode_time_elapsed;

	const auto w1sidenum = w1.sidenum;
	const auto &&seg = vmsegptridx(w1.segnum);
	if (w1_explode_time_elapsed > (EXPL_WALL_TIME * 3) / 4)
	{
		const auto &&csegp = seg.absolute_sibling(seg->children[w1sidenum]);
		const auto cside = find_connect_side(seg, csegp);

		const auto a = w1.clip_num;
		const auto n = WallAnims[a].num_frames;
		wall_set_tmap_num(WallAnims[a], seg, w1sidenum, csegp, cside, n - 1);

		auto &w2 = *vmwallptr(csegp->sides[cside].wall_num);
		assert(&w1 != &w2);
		assert((w1.flags & WALL_EXPLODING) || (w2.flags & WALL_EXPLODING));
		w1.flags |= WALL_BLASTED;
		w2.flags |= WALL_BLASTED;
		if (w1_explode_time_elapsed >= EXPL_WALL_TIME)
		{
			unsigned num_exploding_walls = Num_exploding_walls;
			if (w1.flags & WALL_EXPLODING)
			{
				w1.flags &= ~WALL_EXPLODING;
				-- num_exploding_walls;
			}
			if (w2.flags & WALL_EXPLODING)
			{
				w2.flags &= ~WALL_EXPLODING;
				-- num_exploding_walls;
			}
			Num_exploding_walls = num_exploding_walls;
		}
	}

	const fix newfrac = fixdiv(w1_explode_time_elapsed, EXPL_WALL_TIME);

	const int old_count = f2i(EXPL_WALL_TOTAL_FIREBALLS * fixmul(oldfrac, oldfrac));
	const int new_count = f2i(EXPL_WALL_TOTAL_FIREBALLS * fixmul(newfrac, newfrac));
	if (old_count >= new_count)
		/* for loop would exit with zero iterations if this `if` is
		 * true.  Skip the setup for the loop in that case.
		 */
		return;

	const auto vertnum_list = get_side_verts(seg, w1sidenum);

	auto &v0 = *vcvertptr(vertnum_list[0]);
	auto &v1 = *vcvertptr(vertnum_list[1]);
	auto &v2 = *vcvertptr(vertnum_list[2]);

	const auto &&vv0 = vm_vec_sub(v0, v1);
	const auto &&vv1 = vm_vec_sub(v2, v1);

	//now create all the next explosions

	auto &w1normal0 = seg->sides[w1sidenum].normals[0];
	for (int e = old_count; e < new_count; ++e)
	{
		//calc expl position

		auto pos = vm_vec_scale_add(v1,vv0,d_rand() * 2);
		vm_vec_scale_add2(pos,vv1,d_rand() * 2);

		const fix size = EXPL_WALL_FIREBALL_SIZE + (2 * EXPL_WALL_FIREBALL_SIZE * e / EXPL_WALL_TOTAL_FIREBALLS);

		//fireballs start away from door, with subsequent ones getting closer
		vm_vec_scale_add2(pos, w1normal0, size * (EXPL_WALL_TOTAL_FIREBALLS - e) / EXPL_WALL_TOTAL_FIREBALLS);

		if (e & 3)		//3 of 4 are normal
			object_create_explosion(seg, pos, size, VCLIP_SMALL_EXPLOSION);
		else
			object_create_badass_explosion(object_none, seg, pos,
										   size,
										   VCLIP_SMALL_EXPLOSION,
										   i2f(4),		// damage strength
										   i2f(20),		//	damage radius
										   i2f(50),		//	damage force
										   object_none		//	parent id
			);
	}
}

#if defined(DXX_BUILD_DESCENT_II)
//creates afterburner blobs behind the specified object
void drop_afterburner_blobs(const vmobjptr_t obj, int count, fix size_scale, fix lifetime)
{
	auto pos_left = vm_vec_scale_add(obj->pos, obj->orient.fvec, -obj->size);
	vm_vec_scale_add2(pos_left, obj->orient.rvec, -obj->size/4);
	const auto pos_right = vm_vec_scale_add(pos_left, obj->orient.rvec, obj->size/2);

	if (count == 1)
		vm_vec_avg(pos_left, pos_left, pos_right);

	{
	const auto &&segnum = find_point_seg(pos_left, vmsegptridx(obj->segnum));
	if (segnum != segment_none)
		object_create_explosion(segnum, pos_left, size_scale, VCLIP_AFTERBURNER_BLOB );
	}

	if (count > 1) {
		const auto &&segnum = find_point_seg(pos_right, vmsegptridx(obj->segnum));
		if (segnum != segment_none) {
			auto blob_obj = object_create_explosion(segnum, pos_right, size_scale, VCLIP_AFTERBURNER_BLOB );
			if (lifetime != -1 && blob_obj != object_none)
				blob_obj->lifeleft = lifetime;
		}
	}
}

/*
 * reads n expl_wall structs from a PHYSFS_File and swaps if specified
 */
void expl_wall_read_n_swap(fvmwallptr &vmwallptr, PHYSFS_File *const fp, const int swap, const unsigned count)
{
	assert(!Num_exploding_walls);
	unsigned num_exploding_walls = 0;
	/* Legacy versions of Descent always write a fixed number of
	 * entries, even if some or all of those entries are empty.  This
	 * loop needs to count how many entries were valid, as well as load
	 * them into the correct walls.
	 */
	for (unsigned i = count; i--;)
	{
		disk_expl_wall d;
		PHYSFS_read(fp, &d, sizeof(d), 1);
		if (swap)
		{
			d.segnum = SWAPINT(d.segnum);
			d.sidenum = SWAPINT(d.sidenum);
			d.time = SWAPINT(d.time);
		}
		const icsegidx_t dseg = d.segnum;
		if (dseg == segment_none)
			continue;
		range_for (auto &&wp, vmwallptr)
		{
			auto &w = *wp;
			if (w.segnum != dseg)
				continue;
			if (w.sidenum != d.sidenum)
				continue;
			w.flags |= WALL_EXPLODING;
			w.explode_time_elapsed = d.time;
			++ num_exploding_walls;
			break;
		}
	}
	Num_exploding_walls = num_exploding_walls;
}

void expl_wall_write(fvcwallptr &vcwallptr, PHYSFS_File *const fp)
{
	const unsigned num_exploding_walls = Num_exploding_walls;
	PHYSFS_write(fp, &num_exploding_walls, sizeof(unsigned), 1);
	range_for (auto &&wp, vcwallptr)
	{
		auto &e = *wp;
		if (!(e.flags & WALL_EXPLODING))
			continue;
		disk_expl_wall d;
		d.segnum = e.segnum;
		d.sidenum = e.sidenum;
		d.time = e.explode_time_elapsed;
		PHYSFS_write(fp, &d, sizeof(d), 1);
	}
}
#endif
}
