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
#include <random>
#include <ranges>
#include <optional>
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
#include "physics.h"
#include "laser.h"
#include "wall.h"
#include "multi.h"
#include "timer.h"
#include "playsave.h"
#include "gameseg.h"
#include "automap.h"
#include "byteutil.h"

#include "compiler-range_for.h"
#include "d_array.h"
#include "d_enumerate.h"
#include "d_levelstate.h"
#include "d_range.h"
#include "d_zip.h"
#include "partial_range.h"
#include "segiter.h"

using std::min;

#define EXPLOSION_SCALE (F1_0*5/2)		//explosion is the obj size times this 

namespace dcx {

namespace {

/* C arrays are normally indexed starting from 0.  For some use cases,
 * values in [0, Bias) for some Bias are not useful.  Callers must
 * either waste Bias unused elements at the head of the array, or
 * subtract Bias from all indexes when accessing the array.  This class
 * automates the latter approach.  Its actual size is N, and all index
 * operations are offset by subtracting Bias.  As with std::array,
 * callers are responsible for passing only valid index values.
 */
template <std::size_t Bias, typename T, std::size_t N>
struct biased_index_array : std::array<T, N>
{
	using base_type = std::array<T, N>;
	using typename base_type::reference;
	using typename base_type::const_reference;
	using typename base_type::size_type;
	/* Not implemented, so delete it to avoid using the base class
	 * implementation, which would not adjust by Bias.  This could be
	 * implemented if needed.
	 */
	constexpr reference at(size_type position) = delete;
	constexpr reference at(size_type position) const = delete;
	constexpr reference operator[](size_type pos)
	{
		return this->base_type::operator[](pos - Bias);
	}
	constexpr const_reference operator[](size_type pos) const
	{
		return this->base_type::operator[](pos - Bias);
	}
	[[nodiscard]]
	static constexpr bool valid_index(size_type pos)
	{
		return pos >= Bias && pos - Bias < N;
	}
};

struct connected_segment_raw_distances
{
	using segment_distance_count_type = uint8_t;
	/* Minimum depth must be at least 2, since very low values are used
	 * for special purposes.
	 */
	static constexpr std::integral_constant<segment_distance_count_type, 4> minimum_supported_max_depth{};
	static constexpr std::integral_constant<segment_distance_count_type, 30> maximum_supported_max_depth{};
	static constexpr std::integral_constant<segment_distance_count_type, 1 + maximum_supported_max_depth - minimum_supported_max_depth> supported_max_depth_range{};
	/* The value_type must be sufficient to hold the largest number of
	 * segments that could be at a distance of
	 * `maximum_supported_max_depth`.
	 */
	using depth_count_array = biased_index_array<minimum_supported_max_depth, uint16_t, supported_max_depth_range>;
	enum class biased_distance : uint8_t
	{
		indeterminate,
		depth_0,
	};
	static segment_distance_count_type get_distance_from_biased_distance(const biased_distance d)
	{
		return static_cast<segment_distance_count_type>(d) - static_cast<unsigned>(biased_distance::depth_0);
	}
	static biased_distance get_biased_distance_from_distance(const segment_distance_count_type d)
	{
		return static_cast<biased_distance>(d + static_cast<unsigned>(biased_distance::depth_0));
	}
	/* To reduce the stack footprint when recursively visiting segments,
	 * all the arguments that remain constant during the search are
	 * stored in an instance of `builder`, and `visit_segment` is a
	 * member method on `builder`.
	 */
	struct builder
	{
		fvcsegptr &vcsegptr;
		fvcwallptr &vcwallptr;
		const segment_distance_count_type max_depth;
		depth_count_array &count_segments_at_depth;
		enumerated_array<biased_distance, MAX_SEGMENTS, segnum_t> &depth_by_segment;
		/* Segments that are farther away than max_depth do not have a
		 * specific distance computed, and are only known to be too far
		 * away.
		 */
		void visit_segment(vcsegidx_t current_segment_idx, segment_distance_count_type current_depth) const;
	};
	/* Initialize all depths to 0.  These will be updated as the
	 * constructor traverses the level.  Counts are only maintained for
	 * supported depths.  If a segment is closer than
	 * `minimum_supported_max_depth` or farther than
	 * `maximum_supported_max_depth`, then it is not counted.
	 */
	depth_count_array count_segments_at_depth{};
	/* Initialize all segments to be at the indeterminate distance.
	 *
	 * Traversal of the level data will replace the placeholder distance
	 * with a real value.  Traversal requires that every element either
	 * be the value `indeterminate` or have a valid depth from earlier
	 * in the traversal.
	 */
	static_assert(static_cast<unsigned>(biased_distance::indeterminate) == 0, "`indeterminate` must be 0 so that zero-initialization assigns the value `indeterminate` to every element in the array");
	enumerated_array<biased_distance, MAX_SEGMENTS, segnum_t> depth_by_segment{};
	/* Prefer that the maximum depth be passed as a
	 * std::integral_constant so that it can be checked at compile time.
	 * Allow use of non-integral_constant expressions if the caller uses
	 * the explicit constructor.
	 */
	explicit connected_segment_raw_distances(fvcsegptr &vcsegptr, fvcwallptr &vcwallptr, segment_distance_count_type max_depth, vcsegidx_t current_segment_idx);
	template <segment_distance_count_type max_depth>
		connected_segment_raw_distances(fvcsegptr &vcsegptr, fvcwallptr &vcwallptr, std::integral_constant<segment_distance_count_type, max_depth>, vcsegidx_t current_segment_idx) : connected_segment_raw_distances(vcsegptr, vcwallptr, max_depth, current_segment_idx)
		{
			static_assert(max_depth <= maximum_supported_max_depth);
		}
	/* Scan depth_by_segment to find the Nth segment of the desired
	 * depth, where N is drawn from `uniform_int_distribution(0,
	 * count_of_segments_at_depth - 1)(mrd)`.  If there are no segments
	 * at this depth, return an empty std::optional.
	 */
	std::optional<segnum_t> scan_segment_depths(unsigned desired_depth, std::minstd_rand &mrd) const;
};

}

unsigned Num_exploding_walls;

void init_exploding_walls()
{
	Num_exploding_walls = 0;
}

connected_segment_raw_distances::connected_segment_raw_distances(fvcsegptr &vcsegptr, fvcwallptr &vcwallptr, segment_distance_count_type max_depth, vcsegidx_t current_segment_idx)
{
	builder{vcsegptr, vcwallptr, max_depth, count_segments_at_depth, depth_by_segment}.visit_segment(current_segment_idx, 0u);
}

std::optional<segnum_t> connected_segment_raw_distances::scan_segment_depths(const unsigned desired_depth, std::minstd_rand &mrd) const
{
	const auto count_segments_at_desired_depth{count_segments_at_depth[desired_depth]};
	if (!count_segments_at_desired_depth)
		/* No segments exist at this depth.  Move to next depth.
		*/
		return std::nullopt;
	/* Pick a random segment within the segments at the selected
	 * depth.
	 */
	std::uniform_int_distribution uid(0u, count_segments_at_desired_depth - 1u);
	/* Walk over all segments.  Ignore any segment at the wrong depth.
	 * Ignore the first `skip_count` segments at the right depth.
	 */
	const auto initial_skip_count{uid(mrd)};
	unsigned skip_count{initial_skip_count};
	const auto desired_biased_depth{get_biased_distance_from_distance(desired_depth)};
	for (const auto &&[sn, biased_depth] : enumerate(depth_by_segment))
	{
		if (biased_depth != desired_biased_depth)
			continue;
		if (!skip_count)
			/* Found a segment at the correct depth and the required
			 * number of segments have been skipped.  Pick this segment.
			 */
			return sn;
		-- skip_count;
	}
	/* This should not happen.  `skip_count` is computed such that it is
	 * below the number of segments at a depth of `desired_depth`, so
	 * control should always `return` from the loop.
	 */
	con_printf(CON_URGENT, DXX_STRINGIZE_FL(__FILE__, __LINE__, "error: count=%u, skip_count=%u, and no segment found at depth %u"), count_segments_at_desired_depth, initial_skip_count, desired_depth);
	return std::nullopt;
}

void connected_segment_raw_distances::builder::visit_segment(const vcsegidx_t current_segment_idx, const segment_distance_count_type current_depth) const
{
	auto &biased_depth{depth_by_segment[current_segment_idx]};
	if (biased_depth != biased_distance::indeterminate)
	{
		const auto d{get_distance_from_biased_distance(biased_depth)};
		if (d <= current_depth)
			/* This segment was already found through some other
			 * route.  Leave that result in place.
			 */
			return;
		/* If this segment was previously found at a greater depth, and
		 * the current step found a shorter path to that segment, then
		 * reduce the count of segments at the greater depth, since the
		 * recorded depth of this segment will be changed.
		 */
		if (count_segments_at_depth.valid_index(d))
			-- count_segments_at_depth[d];
	}
	biased_depth = get_biased_distance_from_distance(current_depth);
	if (current_depth >= max_depth)
		return;
	const shared_segment &seg{*vcsegptr(current_segment_idx)};
	if (seg.special == segment_special::controlcen)
	{
		/* Every caller wants to exclude the control-center segment from
		 * the set of choices.  If the segment is encountered here,
		 * assign it a depth value that prevents it being selected, and
		 * stop.  Stopping here prevents exploring segments beyond the
		 * control-center.  If there is a path around the
		 * control-center, that path can be used to reach those segments
		 * instead.  This will lead to a slightly higher than accurate
		 * distance value for those segments, but the original Descent
		 * implementation had a random walk that could wander about and
		 * assign high values to any segment it reached.  By refusing to
		 * traverse through the control center, this implementation
		 * avoids selecting areas that are only reachable by flying
		 * through the control center.
		 */
		biased_depth = get_biased_distance_from_distance(0);
		return;
	}
	/* Segments closer than minimum_supported_max_depth or farther than
	 * maximum_supported_max_depth will not be analyzed later, so no
	 * count is maintained for them.
	 *
	 * This increment is deferred until after the control-center check,
	 * so that it does not need to be undone when the control-center is
	 * found.
	 */
	if (count_segments_at_depth.valid_index(current_depth))
		++ count_segments_at_depth[current_depth];
	for (const auto &&[child_segnum, side] : zip(seg.children, seg.sides))
	{
		if (!IS_CHILD(child_segnum))
			continue;
		if (const auto wall_num{side.wall_num}; wall_num != wall_none)
		{
			auto &w{*vcwallptr(wall_num)};
			if (w.type == WALL_CLOSED)
				continue;
			if (w.type == WALL_DOOR && (w.flags & wall_flag::door_locked))
				continue;
		}
		visit_segment(child_segnum, current_depth + 1);
	}
}

}

namespace dsx {

#if defined(DXX_BUILD_DESCENT_II)
fix	Flash_effect=0;
constexpr int	PK1=1, PK2=8;
#endif

namespace {

static bool can_collide(const object *const weapon_object, const object_base &iter_object, const object *const parent_object)
{
	/* `weapon_object` may not be a weapon in some cases, though the
	 * caller originally expected it would be.  For this function, the
	 * distinction is currently irrelevant.
	 */
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
			return is_proximity_bomb_or_player_smart_mine_or_placed_mine(get_weapon_id(iter_object));
#endif
		case OBJ_CNTRLCEN:
		case OBJ_PLAYER:
			return true;
		case OBJ_ROBOT:
			if (parent_object == nullptr)
				return false;
			if (parent_object->type != OBJ_ROBOT)
				return true;
			return get_robot_id(*parent_object) != get_robot_id(iter_object);
		default:
			return false;
	}
}

}

imobjptridx_t object_create_explosion_without_damage(const d_vclip_array &Vclip, const vmsegptridx_t segnum, const vms_vector &position, const fix size, const vclip_index vclip_type)
{
	const auto &&obj_fireball = obj_create(LevelUniqueObjectState, LevelSharedSegmentState, LevelUniqueSegmentState, OBJ_FIREBALL, underlying_value(vclip_type), segnum, position, &vmd_identity_matrix, size,
					object::control_type::explosion, object::movement_type::None, render_type::RT_FIREBALL);

	if (obj_fireball == object_none)
	{
		return object_none;
	}

	//now set explosion-specific data

	obj_fireball->lifeleft = Vclip[vclip_type].play_time;
	obj_fireball->ctype.expl_info.spawn_time = -1;
	obj_fireball->ctype.expl_info.delete_objnum = object_none;
	obj_fireball->ctype.expl_info.delete_time = -1;
	return obj_fireball;
}

namespace {

static imobjptridx_t object_create_explosion_with_damage(const d_robot_info_array &Robot_info, const d_vclip_array &Vclip, fvmobjptridx &vmobjptridx, const imobjptridx_t obj_explosion_origin, const vmsegptridx_t segnum, const vms_vector &position, const fix size, const vclip_index vclip_type, const fix maxdamage, const fix maxdistance, const fix maxforce, const icobjptridx_t parent)
{
	/* `obj_explosion_origin` may not be a weapon in some cases, though
	 * this function originally expected it would be.
	 */
	const auto &&obj_fireball{object_create_explosion_without_damage(Vclip, segnum, position, size, vclip_type)};
	if (obj_fireball == object_none)
		return object_none;
#if defined(DXX_BUILD_DESCENT_II)
	auto &Objects{LevelUniqueObjectState.Objects};
#endif
	if (maxdamage > 0) {
		fix force;
		vms_vector pos_hit, vforce;
		fix damage;
		// -- now legal for badass explosions on a wall. Assert(obj_explosion_origin != NULL);

		range_for (const auto &&obj_iter, vmobjptridx)
		{
			//	Weapons used to be affected by badass explosions, but this introduces serious problems.
			//	When a smart bomb blows up, if one of its children goes right towards a nearby wall, it will
			//	blow up, blowing up all the children.  So I remove it.  MK, 09/11/94
			if (can_collide(obj_explosion_origin, obj_iter, parent))
			{
				const auto dist{vm_vec_dist_quick(obj_iter->pos, obj_fireball->pos)};
				// Make damage be from 'maxdamage' to 0.0, where 0.0 is 'maxdistance' away;
				if ( dist < maxdistance ) {
					if (object_to_object_visibility(obj_fireball, obj_iter, FQ_TRANSWALL))
					{
						damage = maxdamage - fixmuldiv( dist, maxdamage, maxdistance );
						force = maxforce - fixmuldiv( dist, maxforce, maxdistance );

						// Find the force vector on the object
						vm_vec_normalized_dir_quick(vforce, obj_iter->pos, obj_fireball->pos);
						vm_vec_scale(vforce, force );
	
						// Find where the point of impact is... ( pos_hit )
						vm_vec_scale(vm_vec_sub(pos_hit, obj_fireball->pos, obj_iter->pos), fixdiv(obj_iter->size, obj_iter->size + dist));
						switch (obj_iter->type)
						{
#if defined(DXX_BUILD_DESCENT_II)
							case OBJ_WEAPON:
								phys_apply_force(obj_iter, vforce);

								if (is_proximity_bomb_or_player_smart_mine(get_weapon_id(obj_iter)))
								{		//prox bombs have chance of blowing up
									if (fixmul(dist,force) > i2f(8000)) {
										obj_iter->flags |= OF_SHOULD_BE_DEAD;
										explode_badass_weapon(Robot_info, obj_iter, obj_iter->pos);
									}
								}
								break;
#endif
							case OBJ_ROBOT:
								{
								phys_apply_force(obj_iter, vforce);
#if defined(DXX_BUILD_DESCENT_II)
								//	If not a boss, stun for 2 seconds at 32 force, 1 second at 16 force
								fix flash;
								if (obj_explosion_origin != object_none && obj_explosion_origin->type == OBJ_WEAPON && Robot_info[get_robot_id(obj_iter)].boss_flag == boss_robot_id::None && (flash = Weapon_info[get_weapon_id(obj_explosion_origin)].flash))
								{
									ai_static *const aip{&obj_iter->ctype.ai_info};

									if (obj_fireball->ctype.ai_info.SKIP_AI_COUNT * FrameTime < F1_0)
									{
										const int force_val = f2i(fixdiv(vm_vec_mag_quick(vforce) * flash, FrameTime) / 128) + 2;
										aip->SKIP_AI_COUNT += force_val;
										obj_iter->mtype.phys_info.rotthrust.x = ((d_rand() - 16384) * force_val) / 16;
										obj_iter->mtype.phys_info.rotthrust.y = ((d_rand() - 16384) * force_val) / 16;
										obj_iter->mtype.phys_info.rotthrust.z = ((d_rand() - 16384) * force_val) / 16;
										obj_iter->mtype.phys_info.flags |= PF_USES_THRUST;
									} else
										aip->SKIP_AI_COUNT--;
								}
#endif

								const auto Difficulty_level{underlying_value(GameUniqueState.Difficulty_level)};
								//	When a robot gets whacked by a badass force, he looks towards it because robots tend to get blasted from behind.
								{
									vms_vector neg_vforce;
									neg_vforce.x = vforce.x * -2 * (7 - Difficulty_level)/8;
									neg_vforce.y = vforce.y * -2 * (7 - Difficulty_level)/8;
									neg_vforce.z = vforce.z * -2 * (7 - Difficulty_level)/8;
									phys_apply_rot(obj_iter, neg_vforce);
								}
								if (obj_iter->shields >= 0)
								{
#if defined(DXX_BUILD_DESCENT_II)
									const auto &robot_info{Robot_info[get_robot_id(obj_iter)]};
									/* Descent 1 bosses produce an index that
									 * is not valid for this array, so they
									 * never test for invulnerability.
									 */
									if (const auto boss_index{build_boss_robot_index_from_boss_robot_id(robot_info.boss_flag)}; Boss_invulnerable_matter.valid_index(boss_index) && Boss_invulnerable_matter[boss_index])
											damage /= 4;
#endif
									if (apply_damage_to_robot(Robot_info, obj_iter, damage, parent))
										if (obj_explosion_origin != object_none && parent == get_local_player().objnum)
											add_points_to_score(ConsoleObject->ctype.player_info, Robot_info[get_robot_id(obj_iter)].score_value, Game_mode);
								}
#if defined(DXX_BUILD_DESCENT_II)
								if (obj_explosion_origin != object_none && Robot_info[get_robot_id(obj_iter)].companion && !(obj_explosion_origin->type == OBJ_WEAPON && Weapon_info[get_weapon_id(obj_explosion_origin)].flash))
								{
									static const char ouch_str[]{"ouch! " "ouch! " "ouch! " "ouch! "};
									int	count = f2i(damage / 8);
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
								if (parent != object_none && obj_iter->shields >= 0)
									apply_damage_to_controlcen(Robot_info, obj_iter, damage, parent);
								break;
							case OBJ_PLAYER:	{
								icobjptridx_t killer{object_none};
#if defined(DXX_BUILD_DESCENT_II)
								//	Hack! Warning! Test code!
								fix flash;
								if (obj_explosion_origin != object_none && obj_explosion_origin->type == OBJ_WEAPON && (flash = Weapon_info[get_weapon_id(obj_explosion_origin)].flash) && get_player_id(obj_iter) == Player_num)
								{
									int fe = min(F1_0 * 4, force * flash / 32);	//	For four seconds or less

									if (laser_parent_is_player(Objects.vcptr, obj_explosion_origin->ctype.laser_info, *ConsoleObject))
									{
										fe /= 2;
										force /= 2;
									}
									if (force > F1_0) {
										Flash_effect = fe;
										PALETTE_FLASH_ADD(PK1 + f2i(PK2*force), PK1 + f2i(PK2*force), PK1 + f2i(PK2*force));
									}
								}
#endif
								if (obj_explosion_origin != object_none && (Game_mode & GM_MULTI) && obj_explosion_origin->type == OBJ_PLAYER)
								{
									killer = obj_explosion_origin;
								}
								auto vforce2{vforce};
								if (parent != object_none ) {
									killer = parent;
									if (killer != ConsoleObject)		// if someone else whacks you, cut force by 2x
									{
										vforce2.x /= 2;	vforce2.y /= 2;	vforce2.z /= 2;
									}
								}
								vforce2.x /= 2;	vforce2.y /= 2;	vforce2.z /= 2;

								phys_apply_force(obj_iter, vforce);
								phys_apply_rot(obj_iter, vforce2);
								if (obj_iter->shields >= 0)
								{
#if defined(DXX_BUILD_DESCENT_II)
									if (GameUniqueState.Difficulty_level == Difficulty_level_type::_0)
									damage /= 4;
#endif
									apply_damage_to_player(obj_iter, killer, damage, apply_damage_player::check_for_friendly);
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
	return obj_fireball;
}

}

imobjptridx_t object_create_badass_explosion(const d_robot_info_array &Robot_info, const imobjptridx_t objp, const vmsegptridx_t segnum, const vms_vector &position, fix size, const vclip_index vclip_type, fix maxdamage, fix maxdistance, fix maxforce, const icobjptridx_t parent)
{
	auto &Objects{LevelUniqueObjectState.Objects};
	auto &vmobjptridx{Objects.vmptridx};
	const imobjptridx_t rval{object_create_explosion_with_damage(Robot_info, Vclip, vmobjptridx, objp, segnum, position, size, vclip_type, maxdamage, maxdistance, maxforce, parent)};

	if ((objp != object_none) && (objp->type == OBJ_WEAPON))
		create_weapon_smart_children(objp);
	return rval;
}

//blows up a badass weapon, creating the badass explosion
//return the explosion object
void explode_badass_weapon(const d_robot_info_array &Robot_info, const vmobjptridx_t obj,const vms_vector &pos)
{
	auto &Objects{LevelUniqueObjectState.Objects};
	auto &imobjptridx{Objects.imptridx};
	const auto weapon_id{get_weapon_id(obj)};
	const weapon_info *const wi{&Weapon_info[weapon_id]};

	Assert(wi->damage_radius);
#if defined(DXX_BUILD_DESCENT_II)
	if (weapon_id == weapon_id_type::EARTHSHAKER_ID || weapon_id == weapon_id_type::ROBOT_EARTHSHAKER_ID)
		smega_rock_stuff();
#endif
	digi_link_sound_to_object(SOUND_BADASS_EXPLOSION, obj, 0, F1_0, sound_stack::allow_stacking);

	const auto Difficulty_level{GameUniqueState.Difficulty_level};
	object_create_badass_explosion(Robot_info, obj, vmsegptridx(obj->segnum), pos,
	                                      wi->impact_size,
	                                      wi->robot_hit_vclip,
	                                      wi->strength[Difficulty_level],
	                                      wi->damage_radius,wi->strength[Difficulty_level],
	                                      imobjptridx(obj->ctype.laser_info.parent_num));

}

namespace {

static void explode_badass_object(const d_robot_info_array &Robot_info, fvmsegptridx &vmsegptridx, const vmobjptridx_t objp, fix damage, fix distance, fix force)
{
	const auto &&rval = object_create_badass_explosion(Robot_info, objp, vmsegptridx(objp->segnum), objp->pos, objp->size,
					get_explosion_vclip(Robot_info, objp, explosion_vclip_stage::s0),
					damage, distance, force,
					objp);
	if (rval != object_none)
		digi_link_sound_to_object(SOUND_BADASS_EXPLOSION, rval, 0, F1_0, sound_stack::allow_stacking);
}

}

//blows up the player with a badass explosion
//return the explosion object
void explode_badass_player(const d_robot_info_array &Robot_info, const vmobjptridx_t objp)
{
	explode_badass_object(Robot_info, vmsegptridx, objp, F1_0*50, F1_0*40, F1_0*150);
}

#define DEBRIS_LIFE (f1_0 * (PERSISTENT_DEBRIS?60:2))		//lifespan in seconds

namespace {

static void object_create_debris(fvmsegptridx &vmsegptridx, const object_base &parent, int subobj_num)
{
	Assert(parent.type == OBJ_ROBOT || parent.type == OBJ_PLAYER);

	auto &Polygon_models{LevelSharedPolygonModelState.Polygon_models};
	const auto &&obj{obj_create(LevelUniqueObjectState, LevelSharedSegmentState, LevelUniqueSegmentState, OBJ_DEBRIS, 0, vmsegptridx(parent.segnum), parent.pos, &parent.orient, Polygon_models[parent.rtype.pobj_info.model_num].submodel_rads[subobj_num], object::control_type::debris, object::movement_type::physics, render_type::RT_POLYOBJ)};

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
	obj->mtype.phys_info.rotthrust = {};

	obj->lifeleft = 3*DEBRIS_LIFE/4 + fixmul(d_rand(), DEBRIS_LIFE);	//	Some randomness, so they don't all go away at the same time.

	obj->mtype.phys_info.mass = fixmuldiv(parent.mtype.phys_info.mass, obj->size, parent.size);
	obj->mtype.phys_info.drag = 0; //fl2f(0.2);		//parent->mtype.phys_info.drag;

	if (PERSISTENT_DEBRIS)
	{
		obj->mtype.phys_info.flags |= PF_BOUNCE;
		obj->mtype.phys_info.drag = 128;
	}
}

}

void draw_fireball(const d_vclip_array &Vclip, grs_canvas &canvas, const vcobjptridx_t obj)
{
	const auto lifeleft{obj->lifeleft};
	if (lifeleft > 0)
		draw_vclip_object(canvas, obj, lifeleft, Vclip[get_fireball_id(obj)]);
}

namespace {

static unsigned disallowed_cc_dist(fvcsegptr &vcsegptr, fvcvertptr &vcvertptr, const vcsegptridx_t &segp, const vms_vector &player_pos, const vcsegptridx_t player_seg, const unsigned cur_drop_depth)
{
	const shared_segment &sseg{*segp};
	//don't drop in any children of control centers
	for (const auto ch : sseg.children)
	{
		if (!IS_CHILD(ch))
			continue;
		const shared_segment &childsegp{*vcsegptr(ch)};
		if (childsegp.special == segment_special::controlcen)
			return 1;
	}
	//bail if not far enough from original position
	const auto tempv{compute_segment_center(vcvertptr, sseg)};
	if (find_connected_distance(player_pos, player_seg, tempv, segp, -1, wall_is_doorway_mask::fly) < static_cast<fix>(i2f(20) * cur_drop_depth))
		return 1;
	return 0;
}

//	------------------------------------------------------------------------------------------------------
//	Choose segment to drop a powerup in.
//	For all active net players, try to create a N segment path from the player.  If possible, return that
//	segment.  If not possible, try another player.  After a few tries, use a random segment.
//	Don't drop if control center in segment.
static vmsegptridx_t choose_drop_segment(fvmsegptridx &vmsegptridx, fvcvertptr &vcvertptr, fvcwallptr &vcwallptr, const playernum_t drop_pnum)
{
	auto &Objects{LevelUniqueObjectState.Objects};
	auto &vcobjptr{Objects.vcptr};
	auto &vmobjptr{Objects.vmptr};
	auto &drop_player{*vcplayerptr(drop_pnum)};
	auto &drop_playerobj{*vmobjptr(drop_player.objnum)};

	auto mrd{std::minstd_rand(timer_query())};
	per_player_array<playernum_t> candidate_drop_players;
	const auto end_drop_players = [&candidate_drop_players, drop_pnum]() {
		const auto b{candidate_drop_players.begin()};
		auto r{b};
#if defined(DXX_BUILD_DESCENT_II)
		/* Build a list of active players, excluding the initiator and
		 * anyone from the same team as the initiator.
		 */
		const auto team_of_drop_player{get_team(drop_pnum)};
		for (auto &&[pnum, plr] : enumerate(Players))
		{
			/* Skip the initiator, so that items will try to jump from
			 * player to player as the item is spent and respawned.
			 */
			if (pnum == drop_pnum)
				continue;
			if (plr.connected == player_connection_status::disconnected)
				continue;
			if ((Game_mode & GM_TEAM) && get_team(pnum) == team_of_drop_player)
				continue;
			*r++ = pnum;
		}
		if (b != r)
			/* If at least one such player found, stop and use that
			 * list.
			 */
			return r;
#endif
		/* Otherwise, try again, but allow teammates of the initiator.
		 * Exclude the drop initiator.
		 */
		for (auto &&[pnum, plr] : enumerate(Players))
		{
			if (pnum == drop_pnum)
				continue;
			if (plr.connected == player_connection_status::disconnected)
				continue;
			*r++ = pnum;
		}
		if (b != r)
			/* If a teammate was found, use that.  This encourages the
			 * game to move the items among players.
			 */
			return r;
		/* If no player found through other rules, use the player
		 * who triggered the drop event.
		 */
		*r++ = drop_pnum;
		return r;
	}();
	/* candidate_drop_players now contains the player IDs of all players
	 * that can be used for a drop.  Shuffle them for randomness to vary
	 * who will be checked first for each new item.
	 */
	std::shuffle(candidate_drop_players.begin(), end_drop_players, mrd);
	static constexpr std::integral_constant<connected_segment_raw_distances::segment_distance_count_type, 8> net_drop_max_depth_lower{};
	static constexpr std::integral_constant<connected_segment_raw_distances::segment_distance_count_type, 24> net_drop_max_depth_upper{};
	static constexpr std::integral_constant<connected_segment_raw_distances::segment_distance_count_type, 4> net_drop_min_depth{};
	static_assert(connected_segment_raw_distances::depth_count_array::valid_index(net_drop_max_depth_lower));
	static_assert(connected_segment_raw_distances::depth_count_array::valid_index(net_drop_max_depth_upper));
	static_assert(connected_segment_raw_distances::depth_count_array::valid_index(net_drop_min_depth + 1u));

	const auto &&drop_player_seg{vmsegptridx(drop_playerobj.segnum)};

	/* Defer constructing instances until needed.  In most games, a
	 * segment should be found before all MAX_PLAYERS instances are
	 * constructed.
	 */
	per_player_array<std::unique_ptr<connected_segment_raw_distances>> distance_by_player;
	std::optional<vmsegptridx_t> fallback_drop;
	for (const unsigned candidate_depth : xrange(std::uniform_int_distribution(net_drop_max_depth_lower + 0u, net_drop_max_depth_upper + 0u)(mrd), net_drop_min_depth, xrange_descending()))
	{
		for (const auto pnum : std::ranges::subrange(candidate_drop_players.begin(), end_drop_players))
		{
			auto &plr{*vcplayerptr(pnum)};
			auto &plrobj{*vcobjptr(plr.objnum)};
			auto &rdp{distance_by_player[pnum]};
			if (!rdp)
				/* connected_segment_raw_distances computes data for all
				 * distances below candidate_depth too.  Since the
				 * xrange counts down, this initialization is
				 * sufficient to cover all data required by all passes
				 * of the loop.
				 */
				rdp = std::make_unique<connected_segment_raw_distances>(vcsegptr, vcwallptr, candidate_depth, plrobj.segnum);
			auto &rd{*rdp};
			const auto sn{rd.scan_segment_depths(candidate_depth, mrd)};
			if (!sn)
				continue;
			const auto segnum{*sn};
			const auto &&segp{vmsegptridx(segnum)};
			if (disallowed_cc_dist(vcsegptr, vcvertptr, segp, drop_playerobj.pos, drop_player_seg, candidate_depth))
			{
				if (!fallback_drop.has_value())
					fallback_drop = segp;
				continue;
			}
			return segp;
		}
	}
	if (fallback_drop.has_value())
		/* If no player found a segment acceptable to the first pass
		 * rules, but a player found a reachable segment, use that
		 * segment.  This differs slightly from retail, which would
		 * compute the fallback from the initiator, rather than a random
		 * successful player.  However, since saving the fallback is
		 * cheap, this is done instead of performing yet another search.
		 */
		return *fallback_drop;
	/* This can be reached if there are no segments found at any of the
	 * acceptable depths.  If the origin segment is in a small room with
	 * no passable exits, this can happen.
	 *
	 * Give up and pick a completely random segment.  This is compatible
	 * with retail.
	 */
	std::uniform_int_distribution<typename std::underlying_type<segnum_t>::type> uid(0u, Highest_segment_index);
	return vmsegptridx(vmsegidx_t(segnum_t{uid(mrd)}));
}

}

//	------------------------------------------------------------------------------------------------------
//	(Re)spawns powerup if in a network game.
void maybe_drop_net_powerup(powerup_type_t powerup_type, bool adjust_cap, bool random_player)
{
	auto &LevelSharedVertexState{LevelSharedSegmentState.get_vertex_state()};
	auto &LevelUniqueControlCenterState{LevelUniqueObjectState.ControlCenterState};
	auto &Vertices{LevelSharedVertexState.get_vertices()};
	playernum_t pnum{Player_num};
	if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP)) {
		if ((Game_mode & GM_NETWORK) && adjust_cap)
		{
			MultiLevelInv_Recount(); // recount current items
			if (!MultiLevelInv_AllowSpawn(powerup_type))
				return;
		}

		if (LevelUniqueControlCenterState.Control_center_destroyed || Network_status == network_state::endlevel)
			return;

                if (random_player)
                {
					uint_fast32_t failsafe_count{0};
                        do {
                                pnum = d_rand() % MAX_PLAYERS;
                                if (failsafe_count > MAX_PLAYERS*4) // that was plenty of tries to find a good player...
                                {
                                        pnum = Player_num; // ... go with Player_num instead
                                        break;
                                }
                                failsafe_count++;
                        } while (vcplayerptr(pnum)->connected != player_connection_status::playing);
                }

//--old-- 		segnum = (d_rand() * Highest_segment_index) >> 15;
//--old-- 		Assert((segnum >= 0) && (segnum <= Highest_segment_index));
//--old-- 		if (segnum < 0)
//--old-- 			segnum = -segnum;
//--old-- 		while (segnum > Highest_segment_index)
//--old-- 			segnum /= 2;

		Net_create_loc = 0;

		auto &vcvertptr{Vertices.vcptr};
		const auto &&segnum{choose_drop_segment(LevelUniqueSegmentState.get_segments().vmptridx, vcvertptr, LevelUniqueWallSubsystemState.Walls.vcptr, pnum)};
		const auto &&new_pos{pick_random_point_in_seg(vcvertptr, segnum, std::minstd_rand(d_rand()))};
		const auto &&objnum{drop_powerup(LevelUniqueObjectState, LevelSharedSegmentState, LevelUniqueSegmentState, Vclip, powerup_type, {}, new_pos, segnum, true)};
		if (objnum == object_none)
			return;
		multi_send_create_powerup(powerup_type, segnum, objnum, new_pos);
		object_create_explosion_without_damage(Vclip, segnum, new_pos, i2f(5), vclip_index::powerup_disappearance);
	}
}

namespace {

//	------------------------------------------------------------------------------------------------------
//	Return true if current segment contains some object.
static const object_base *segment_contains_powerup(fvcobjptridx &vcobjptridx, fvcsegptr &vcsegptr, const unique_segment &segnum, const powerup_type_t obj_id)
{
	for (auto &o : objects_in<const object_base>(segnum, vcobjptridx, vcsegptr))
	{
		if (o.type == OBJ_POWERUP && get_powerup_id(o) == obj_id)
			return &o;
	}
	return nullptr;
}

//	------------------------------------------------------------------------------------------------------
static const object_base *powerup_nearby_aux(fvcobjptridx &vcobjptridx, fvcsegptr &vcsegptr, const vcsegidx_t segnum, const powerup_type_t object_id, uint_fast32_t depth)
{
	const cscusegment &&segp{vcsegptr(segnum)};
	if (auto r = segment_contains_powerup(vcobjptridx, vcsegptr, segp, object_id))
		return r;
	if (! -- depth)
		return nullptr;
	for (const auto seg2 : segp.s.children)
	{
		if (seg2 != segment_none)
			if (auto r = powerup_nearby_aux(vcobjptridx, vcsegptr, seg2, object_id, depth))
				return r;
	}
	return nullptr;
}

//	------------------------------------------------------------------------------------------------------
//	Return true if some powerup is nearby (within 3 segments).
static const object_base *weapon_nearby(fvcobjptridx &vcobjptridx, fvcsegptr &vcsegptr, const object_base &objp, const powerup_type_t weapon_id)
{
	return powerup_nearby_aux(vcobjptridx, vcsegptr, objp.segnum, weapon_id, 2);
}

}

//	------------------------------------------------------------------------------------------------------
void maybe_replace_powerup_with_energy(object_base &del_obj)
{
	auto &Objects{LevelUniqueObjectState.Objects};
	auto &vcobjptridx{Objects.vcptridx};
	auto &vmobjptr{Objects.vmptr};
	/* This function has no special handling to remap laser weapons, so
	 * borrow LASER_INDEX as a flag value to indicate that the powerup
	 * ID was not recognized.
	 */
	static constexpr primary_weapon_index_t unset_weapon_index{primary_weapon_index_t::LASER_INDEX};
	primary_weapon_index_t weapon_index{unset_weapon_index};

	if (del_obj.contains.type != contained_object_type::powerup)
		return;

	switch (del_obj.contains.id.powerup)
	{
		case powerup_type_t::POW_CLOAK:
			if (weapon_nearby(vcobjptridx, vcsegptr, del_obj, powerup_type_t::POW_CLOAK) != nullptr)
				del_obj.contains.count = 0;
			return;
		case powerup_type_t::POW_VULCAN_WEAPON:
			weapon_index = primary_weapon_index_t::VULCAN_INDEX;
			break;
		case powerup_type_t::POW_SPREADFIRE_WEAPON:
			weapon_index = primary_weapon_index_t::SPREADFIRE_INDEX;
			break;
		case powerup_type_t::POW_PLASMA_WEAPON:
			weapon_index = primary_weapon_index_t::PLASMA_INDEX;
			break;
		case powerup_type_t::POW_FUSION_WEAPON:
			weapon_index = primary_weapon_index_t::FUSION_INDEX;
			break;
#if defined(DXX_BUILD_DESCENT_II)
		case powerup_type_t::POW_GAUSS_WEAPON:
			weapon_index = primary_weapon_index_t::GAUSS_INDEX;
			break;
		case powerup_type_t::POW_HELIX_WEAPON:
			weapon_index = primary_weapon_index_t::HELIX_INDEX;
			break;
		case powerup_type_t::POW_PHOENIX_WEAPON:
			weapon_index = primary_weapon_index_t::PHOENIX_INDEX;
			break;
		case powerup_type_t::POW_OMEGA_WEAPON:
			weapon_index = primary_weapon_index_t::OMEGA_INDEX;
			break;
#endif
		case powerup_type_t::POW_EXTRA_LIFE:
		case powerup_type_t::POW_ENERGY:
		case powerup_type_t::POW_SHIELD_BOOST:
		case powerup_type_t::POW_LASER:
		case powerup_type_t::POW_KEY_BLUE:
		case powerup_type_t::POW_KEY_RED:
		case powerup_type_t::POW_KEY_GOLD:
		case powerup_type_t::POW_MISSILE_1:
		case powerup_type_t::POW_MISSILE_4:
		case powerup_type_t::POW_QUAD_FIRE:
		case powerup_type_t::POW_PROXIMITY_WEAPON:
		case powerup_type_t::POW_HOMING_AMMO_1:
		case powerup_type_t::POW_HOMING_AMMO_4:
		case powerup_type_t::POW_SMARTBOMB_WEAPON:
		case powerup_type_t::POW_MEGA_WEAPON:
		case powerup_type_t::POW_VULCAN_AMMO:
		case powerup_type_t::POW_TURBO:
		case powerup_type_t::POW_INVULNERABILITY:
		case powerup_type_t::POW_MEGAWOW:
#if defined(DXX_BUILD_DESCENT_II)
		case powerup_type_t::POW_SUPER_LASER:
		case powerup_type_t::POW_FULL_MAP:
		case powerup_type_t::POW_CONVERTER:
		case powerup_type_t::POW_AMMO_RACK:
		case powerup_type_t::POW_AFTERBURNER:
		case powerup_type_t::POW_HEADLIGHT:
		case powerup_type_t::POW_SMISSILE1_1:
		case powerup_type_t::POW_SMISSILE1_4:
		case powerup_type_t::POW_GUIDED_MISSILE_1:
		case powerup_type_t::POW_GUIDED_MISSILE_4:
		case powerup_type_t::POW_SMART_MINE:
		case powerup_type_t::POW_MERCURY_MISSILE_1:
		case powerup_type_t::POW_MERCURY_MISSILE_4:
		case powerup_type_t::POW_EARTHSHAKER_MISSILE:
		case powerup_type_t::POW_FLAG_BLUE:
		case powerup_type_t::POW_FLAG_RED:
		case powerup_type_t::POW_HOARD_ORB:
#endif
			break;
	}

	//	Don't drop vulcan ammo if player maxed out.
	auto &player_info{get_local_plrobj().ctype.player_info};
	if ((weapon_index_uses_vulcan_ammo(weapon_index) || del_obj.contains.id.powerup == powerup_type_t::POW_VULCAN_AMMO) &&
		player_info.vulcan_ammo >= VULCAN_AMMO_MAX)
		del_obj.contains.count = 0;
	else if (weapon_index != unset_weapon_index)
	{
		if (has_weapon(player_has_primary_weapon(player_info, weapon_index)) || weapon_nearby(vcobjptridx, vcsegptr, del_obj, del_obj.contains.id.powerup) != nullptr)
		{
			if (d_rand() > 16384) {
#if defined(DXX_BUILD_DESCENT_I)
				del_obj.contains.count = 1;
#endif
				del_obj.contains.type = contained_object_type::powerup;
				if (weapon_index_uses_vulcan_ammo(weapon_index)) {
					del_obj.contains.id.powerup = powerup_type_t::POW_VULCAN_AMMO;
				}
				else {
					del_obj.contains.id.powerup = powerup_type_t::POW_ENERGY;
				}
			} else {
#if defined(DXX_BUILD_DESCENT_I)
				del_obj.contains.count = 0;
#elif defined(DXX_BUILD_DESCENT_II)
				del_obj.contains.type = contained_object_type::powerup;
				del_obj.contains.id.powerup = powerup_type_t::POW_SHIELD_BOOST;
#endif
			}
		}
	} else if (del_obj.contains.id.powerup == powerup_type_t::POW_QUAD_FIRE)
	{
		if ((player_info.powerup_flags & PLAYER_FLAGS_QUAD_LASERS) || weapon_nearby(vcobjptridx, vcsegptr, del_obj, del_obj.contains.id.powerup) != nullptr)
		{
			if (d_rand() > 16384) {
#if defined(DXX_BUILD_DESCENT_I)
				del_obj.contains.count = 1;
#endif
				del_obj.contains.type = contained_object_type::powerup;
				del_obj.contains.id.powerup = powerup_type_t::POW_ENERGY;
			} else {
#if defined(DXX_BUILD_DESCENT_I)
				del_obj.contains.count = 0;
#elif defined(DXX_BUILD_DESCENT_II)
				del_obj.contains.type = contained_object_type::powerup;
				del_obj.contains.id.powerup = powerup_type_t::POW_SHIELD_BOOST;
#endif
			}
		}
	}

	//	If this robot was gated in by the boss and it now contains energy, make it contain nothing,
	//	else the room gets full of energy.
	if (del_obj.matcen_creator == BOSS_GATE_MATCEN_NUM && del_obj.contains.id.powerup == powerup_type_t::POW_ENERGY)
		del_obj.contains.count = 0;

	// Change multiplayer extra-lives into invulnerability
	if ((Game_mode & GM_MULTI) && (del_obj.contains.id.powerup == powerup_type_t::POW_EXTRA_LIFE))
	{
		del_obj.contains.id.powerup = powerup_type_t::POW_INVULNERABILITY;
	}
}

imobjptridx_t drop_powerup(d_level_unique_object_state &LevelUniqueObjectState, const d_level_shared_segment_state &LevelSharedSegmentState, d_level_unique_segment_state &LevelUniqueSegmentState, const d_vclip_array &Vclip, powerup_type_t id, const vms_vector &init_vel, const vms_vector &pos, const vmsegptridx_t segnum, const bool player)
{
	const auto old_mag{vm_vec_mag_quick(init_vel)};
				int	rand_scale;

				//	We want powerups to move more in network mode.
				if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_ROBOTS)) {
					rand_scale = 4;
					//	extra life powerups are converted to invulnerability in multiplayer, for what is an extra life, anyway?
					if (id == powerup_type_t::POW_EXTRA_LIFE)
						id = powerup_type_t::POW_INVULNERABILITY;
				} else
					rand_scale = 2;

				if (Game_mode & GM_MULTI)
				{	
					if (Net_create_loc >= MAX_NET_CREATE_OBJECTS)
					{
						con_printf(CON_URGENT, DXX_STRINGIZE_FL("%s", __LINE__, "unable to record network object; Net_create_loc=%u"), __FILE__, Net_create_loc);
						return object_none;
					}
#if defined(DXX_BUILD_DESCENT_II)
					if ((Game_mode & GM_NETWORK) && Network_status == network_state::endlevel)
					 return object_none;
#endif
				}
				const auto &&objp{obj_create(LevelUniqueObjectState, LevelSharedSegmentState, LevelUniqueSegmentState, OBJ_POWERUP, underlying_value(id), segnum, pos, &vmd_identity_matrix, Powerup_info[id].size, object::control_type::powerup, object::movement_type::physics, render_type::RT_POWERUP)};

				if (objp == object_none)
					return objp;
				auto &obj{*objp};
#if defined(DXX_BUILD_DESCENT_II)
				if (player)
					obj.flags |= OF_PLAYER_DROPPED;
#endif

				if (Game_mode & GM_MULTI)
				{
					Net_create_objnums[Net_create_loc++] = objp;
				}

				// Give keys zero velocity so they can be tracked better in multi
				auto &object_velocity{obj.mtype.phys_info.velocity};
				if ((Game_mode & GM_MULTI) && (id >= powerup_type_t::POW_KEY_BLUE) && (id <= powerup_type_t::POW_KEY_GOLD))
					object_velocity = {};
				else
				{
					object_velocity = init_vel;
					const auto random_velocity_adjustment = [old_mag, rand_scale]() {
						return fixmul(old_mag + F1_0 * 32, d_rand() * rand_scale - 16384 * rand_scale);
					};
					object_velocity.x += random_velocity_adjustment();
					object_velocity.y += random_velocity_adjustment();
					object_velocity.z += random_velocity_adjustment();
				}

				obj.mtype.phys_info.drag = 512;	//1024;
				obj.mtype.phys_info.mass = F1_0;

				obj.mtype.phys_info.flags = PF_BOUNCE;

				obj.rtype.vclip_info.vclip_num = Powerup_info[id].vclip_num;
				obj.rtype.vclip_info.frametime = Vclip[obj.rtype.vclip_info.vclip_num].frame_time;
				obj.rtype.vclip_info.framenum = 0;

				switch (id)
				{
					case powerup_type_t::POW_MISSILE_1:
					case powerup_type_t::POW_MISSILE_4:
					case powerup_type_t::POW_SHIELD_BOOST:
					case powerup_type_t::POW_ENERGY:
						obj.lifeleft = (d_rand() + F1_0*3) * 64;		//	Lives for 3 to 3.5 binary minutes (a binary minute is 64 seconds)
						if (Game_mode & GM_MULTI)
							obj.lifeleft /= 2;
						break;
#if defined(DXX_BUILD_DESCENT_II)
					case powerup_type_t::POW_OMEGA_WEAPON:
						if (!player)
							obj.ctype.powerup_info.count = MAX_OMEGA_CHARGE;
						break;
					case powerup_type_t::POW_GAUSS_WEAPON:
#endif
					case powerup_type_t::POW_VULCAN_WEAPON:
						if (!player)
							obj.ctype.powerup_info.count = VULCAN_WEAPON_AMMO_AMOUNT;
						break;
					default:
						break;
				}
	return objp;
}

bool drop_powerup(d_level_unique_object_state &LevelUniqueObjectState, const d_level_shared_segment_state &LevelSharedSegmentState, d_level_unique_segment_state &LevelUniqueSegmentState, const d_vclip_array &Vclip, const powerup_type_t id, const unsigned num, const vms_vector &init_vel, const vms_vector &pos, const vmsegptridx_t segnum, const bool player)
{
	bool created{false};
	for (const auto i : xrange(num))
	{
		(void)i;
		const auto &&obj{drop_powerup(LevelUniqueObjectState, LevelSharedSegmentState, LevelUniqueSegmentState, Vclip, id, init_vel, pos, segnum, player)};
		if (obj == object_none)
			/* If one drop failed, assume every additional drop will also fail.
			 */
			break;
		created = true;
	}
	return created;
}

namespace {

static bool drop_robot_egg(const d_robot_info_array &Robot_info, const contained_object_type type, const contained_object_id id, const unsigned num, const vms_vector &init_vel, const vms_vector &pos, const vmsegptridx_t segnum)
{
	if (!num)
		return false;
	switch (type)
	{
		case contained_object_type::powerup:
			return drop_powerup(LevelUniqueObjectState, LevelSharedSegmentState, LevelUniqueSegmentState, Vclip, id.powerup, num, init_vel, pos, segnum, false);
		case contained_object_type::robot:
			break;
		default:
			con_printf(CON_URGENT, DXX_STRINGIZE_FL(__FILE__, __LINE__, "ignoring invalid object type; expected OBJ_POWERUP or OBJ_ROBOT, got type=%i, id=%i"), underlying_value(type), underlying_value(id.powerup));
			return false;
	}
	auto &Polygon_models{LevelSharedPolygonModelState.Polygon_models};
	bool created{false};
	for (const auto count : xrange(num))
	{
		auto new_velocity{vm_vec_normalized_quick(init_vel)};
		const auto old_mag{vm_vec_mag_quick(init_vel)};
		(void)count;
				int	rand_scale;

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
				auto new_pos{pos};
				//	This is dangerous, could be outside mine.
//				new_pos.x += (d_rand()-16384)*8;
//				new_pos.y += (d_rand()-16384)*7;
//				new_pos.z += (d_rand()-16384)*6;

				const auto rid{id.robot};
				auto &robptr{Robot_info[rid]};
				const auto &&objp{robot_create(Robot_info, rid, segnum, new_pos, &vmd_identity_matrix, Polygon_models[robptr.model_num].rad, ai_behavior::AIB_NORMAL)};
				if (objp == object_none)
					break;
				auto &obj{*objp};
				created = true;
				++LevelUniqueObjectState.accumulated_robots;
				++GameUniqueState.accumulated_robots;
				if (Game_mode & GM_MULTI)
				{
					Net_create_objnums[Net_create_loc++] = objp;
				}
				//Set polygon-object-specific data

				obj.rtype.pobj_info.model_num = robptr.model_num;
				obj.rtype.pobj_info.subobj_flags = 0;

				//set Physics info
		
				obj.mtype.phys_info.velocity = new_velocity;

				obj.mtype.phys_info.mass = robptr.mass;
				obj.mtype.phys_info.drag = robptr.drag;

				obj.mtype.phys_info.flags |= (PF_LEVELLING);

				obj.shields = robptr.strength;

				ai_local *const ailp{&obj.ctype.ai_info.ail};
				ailp->player_awareness_type = player_awareness_type_t::PA_WEAPON_ROBOT_COLLISION;
				ailp->player_awareness_time = F1_0*3;
				obj.ctype.ai_info.CURRENT_STATE = AIS_LOCK;
				obj.ctype.ai_info.GOAL_STATE = AIS_LOCK;
				obj.ctype.ai_info.REMOTE_OWNER = -1;
			}

#if defined(DXX_BUILD_DESCENT_II)
			// At JasenW's request, robots which contain robots
			// sometimes drop shields.
			if (d_rand() > 16384)
			{
				const auto &&objp{drop_powerup(LevelUniqueObjectState, LevelSharedSegmentState, LevelUniqueSegmentState, Vclip, powerup_type_t::POW_SHIELD_BOOST, init_vel, pos, segnum, false)};
				if (objp != object_none)
					created = true;
			}
#endif
	return created;
}

#if defined(DXX_BUILD_DESCENT_II)
static bool skip_create_egg_powerup(const object &player, powerup_type_t powerup)
{
	fix current;
	auto &player_info{player.ctype.player_info};
	if (powerup == powerup_type_t::POW_SHIELD_BOOST)
		current = player.shields;
	else if (powerup == powerup_type_t::POW_ENERGY)
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

}

bool object_create_robot_egg(const d_robot_info_array &Robot_info, const contained_object_type type, const contained_object_id id, const unsigned num, const vms_vector &init_vel, const vms_vector &pos, const vmsegptridx_t segnum)
{
#if defined(DXX_BUILD_DESCENT_II)
	auto &Objects{LevelUniqueObjectState.Objects};
	auto &vmobjptr{Objects.vmptr};
	if (!(Game_mode & GM_MULTI))
	{
		if (type == contained_object_type::powerup)
		{
			if (skip_create_egg_powerup(get_local_plrobj(), id.powerup))
				return false;
		}
	}
#endif
	return drop_robot_egg(Robot_info, type, id, num, init_vel, pos, segnum);
}

bool object_create_robot_egg(const d_robot_info_array &Robot_info, object_base &objp)
{
	return object_create_robot_egg(Robot_info, objp.contains.type, objp.contains.id, objp.contains.count, objp.mtype.phys_info.velocity, objp.pos, vmsegptridx(objp.segnum));
}

//	-------------------------------------------------------------------------------------------------------
//	Put count objects of type type (eg, powerup), id = id (eg, energy) into *objp, then drop them!  Yippee!
//	Returns created object number.
imobjptridx_t call_object_create_egg(const object_base &objp, const powerup_type_t id)
{
	return drop_powerup(LevelUniqueObjectState, LevelSharedSegmentState, LevelUniqueSegmentState, Vclip, id, objp.mtype.phys_info.velocity, objp.pos, vmsegptridx(objp.segnum), true);
}

void call_object_create_egg(const object_base &objp, const unsigned count, const powerup_type_t id)
{
	if (count > 0) {
		drop_powerup(LevelUniqueObjectState, LevelSharedSegmentState, LevelUniqueSegmentState, Vclip, id, count, objp.mtype.phys_info.velocity, objp.pos, vmsegptridx(objp.segnum), true);
	}
}

//what vclip does this explode with?
vclip_index get_explosion_vclip(const d_robot_info_array &Robot_info, const object_base &obj, explosion_vclip_stage stage)
{
	if (obj.type == OBJ_ROBOT)
	{
		const auto vclip_ptr = stage == explosion_vclip_stage::s0
			? &robot_info::exp1_vclip_num
			: &robot_info::exp2_vclip_num;
		const auto vclip_num{Robot_info[get_robot_id(obj)].*vclip_ptr};
		if (Vclip.valid_index(vclip_num))
			return vclip_num;
	}
	else if (obj.type == OBJ_PLAYER)
	{
		if (const auto expl_vclip_num{Player_ship->expl_vclip_num}; Vclip.valid_index(expl_vclip_num))
			return expl_vclip_num;
	}

	return vclip_index::small_explosion;		//default
}

namespace {

//blow up a polygon model
static void explode_model(object_base &obj)
{
	assert(obj.render_type == render_type::RT_POLYOBJ);

	const auto poly_model_num{obj.rtype.pobj_info.model_num};
	const auto dying_model_num{Dying_modelnums[poly_model_num]};
	const auto model_num{(dying_model_num != polygon_model_index::None)
		? (obj.rtype.pobj_info.model_num = dying_model_num)
		: poly_model_num
	};
	auto &Polygon_models{LevelSharedPolygonModelState.Polygon_models};
	const auto n_models{Polygon_models[model_num].n_models};
	if (n_models > 1) {
		for (unsigned i{1}; i < n_models; ++i)
#if defined(DXX_BUILD_DESCENT_II)
			if (!(i == 5 && obj.type == OBJ_ROBOT && get_robot_id(obj) == robot_id::energy_bandit))	//energy sucker energy part
#endif
				object_create_debris(vmsegptridx, obj, i);

		//make parent object only draw center part
		obj.rtype.pobj_info.subobj_flags = 1;
	}
}

//if the object has a destroyed model, switch to it.  Otherwise, delete it.
static void maybe_delete_object(object_base &del_obj)
{
	const auto dead_modelnum{Dead_modelnums[del_obj.rtype.pobj_info.model_num]};
	if (dead_modelnum != polygon_model_index::None)
	{
		del_obj.rtype.pobj_info.model_num = dead_modelnum;
		del_obj.flags |= OF_DESTROYED;
	}
	else {		//normal, multi-stage explosion
		if (del_obj.type == OBJ_PLAYER)
			del_obj.render_type = render_type::RT_NONE;
		else
			del_obj.flags |= OF_SHOULD_BE_DEAD;
	}
}

}

//	-------------------------------------------------------------------------------------------------------
//blow up an object.  Takes the object to destroy, and the point of impact
void explode_object(d_level_unique_object_state &LevelUniqueObjectState, const d_robot_info_array &Robot_info, const d_level_shared_segment_state &LevelSharedSegmentState, d_level_unique_segment_state &LevelUniqueSegmentState, const vmobjptridx_t hitobj, const fix delay_time)
{
	if (hitobj->flags & OF_EXPLODING) return;

	if (delay_time) {		//wait a little while before creating explosion
		//create a placeholder object to do the delay, with id==-1
		const auto &&obj = obj_create(LevelUniqueObjectState, LevelSharedSegmentState, LevelUniqueSegmentState, OBJ_FIREBALL, -1, vmsegptridx(hitobj->segnum), hitobj->pos, &vmd_identity_matrix, 0,
						object::control_type::explosion, object::movement_type::None, render_type::RT_NONE);
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
		const auto vclip_num{get_explosion_vclip(Robot_info, hitobj, explosion_vclip_stage::s0)};
		const imobjptr_t expl_obj{object_create_explosion_without_damage(Vclip, vmsegptridx(hitobj->segnum), hitobj->pos, fixmul(hitobj->size, EXPLOSION_SCALE), vclip_num)};
		if (! expl_obj) {
			maybe_delete_object(hitobj);		//no explosion, die instantly
			return;
		}

		//don't make debris explosions have physics, because they often
		//happen when the debris has hit the wall, so the fireball is trying
		//to move into the wall, which shows off FVI problems.   	
		if (hitobj->type!=OBJ_DEBRIS && hitobj->movement_source==object::movement_type::physics) {
			expl_obj->movement_source = object::movement_type::physics;
			expl_obj->mtype.phys_info = hitobj->mtype.phys_info;
		}
	
		if (hitobj->render_type == render_type::RT_POLYOBJ && hitobj->type != OBJ_DEBRIS)
			explode_model(hitobj);

		maybe_delete_object(hitobj);
	}

	hitobj->flags |= OF_EXPLODING;		//say that this is blowing up
	hitobj->control_source = object::control_type::None;		//become inert while exploding

}

//do whatever needs to be done for this piece of debris for this frame
void do_debris_frame(const d_robot_info_array &Robot_info, const vmobjptridx_t obj)
{
	assert(obj->control_source == object::control_type::debris);

	if (obj->lifeleft < 0)
		explode_object(LevelUniqueObjectState, Robot_info, LevelSharedSegmentState, LevelUniqueSegmentState, obj, 0);
}

//do whatever needs to be done for this explosion for this frame
void do_explosion_sequence(const d_robot_info_array &Robot_info, object &obj)
{
	auto &Objects{LevelUniqueObjectState.Objects};
	auto &vmobjptr{Objects.vmptr};
	auto &vmobjptridx{Objects.vmptridx};
	assert(obj.control_source == object::control_type::explosion);

	//See if we should die of old age
	if (obj.lifeleft <= 0 ) 	{	// We died of old age
		obj.flags |= OF_SHOULD_BE_DEAD;
		obj.lifeleft = 0;
	}

	//See if we should create a secondary explosion
	if (obj.lifeleft <= obj.ctype.expl_info.spawn_time) {
		auto del_obj{vmobjptridx(obj.ctype.expl_info.delete_objnum)};
		auto &spawn_pos{del_obj->pos};
		Assert(del_obj->type==OBJ_ROBOT || del_obj->type==OBJ_CLUTTER || del_obj->type==OBJ_CNTRLCEN || del_obj->type == OBJ_PLAYER);
		Assert(del_obj->segnum != segment_none);

		const auto &&expl_obj{[&]{
			const auto vclip_num{get_explosion_vclip(Robot_info, del_obj, explosion_vclip_stage::s1)};
#if defined(DXX_BUILD_DESCENT_II)
			if (del_obj->type == OBJ_ROBOT)
			{
				const auto &ri{Robot_info[get_robot_id(del_obj)]};
				if (ri.badass)
					return object_create_badass_explosion(Robot_info, object_none, vmsegptridx(del_obj->segnum), spawn_pos, fixmul(del_obj->size, EXPLOSION_SCALE), vclip_num, F1_0 * ri.badass, i2f(4) * ri.badass, i2f(35) * ri.badass, object_none);
			}
#endif
			return object_create_explosion_without_damage(Vclip, vmsegptridx(del_obj->segnum), spawn_pos, fixmul(del_obj->size, EXPLOSION_SCALE), vclip_num);
		}()};

		if (del_obj->contains.count > 0 && !(Game_mode & GM_MULTI)) { // Multiplayer handled outside of this code!!
			//	If dropping a weapon that the player has, drop energy instead, unless it's vulcan, in which case drop vulcan ammo.
			if (del_obj->contains.type == contained_object_type::powerup)
				maybe_replace_powerup_with_energy(del_obj);
			object_create_robot_egg(Robot_info, del_obj);
		} else if ((del_obj->type == OBJ_ROBOT) && !(Game_mode & GM_MULTI)) { // Multiplayer handled outside this code!!
			auto &robptr{Robot_info[get_robot_id(del_obj)]};
			if (const auto contains_count{robptr.contains.count})
			{
				if (((d_rand() * 16) >> 15) < robptr.contains_prob) {
					del_obj->contains = robptr.contains;
					del_obj->contains.count = ((d_rand() * contains_count) >> 15) + 1;
					maybe_replace_powerup_with_energy(del_obj);
					object_create_robot_egg(Robot_info, del_obj);
				}
			}
#if defined(DXX_BUILD_DESCENT_II)
			if (robot_is_thief(robptr))
				drop_stolen_items_local(LevelUniqueObjectState, LevelSharedSegmentState, LevelUniqueSegmentState, Vclip, vmsegptridx(del_obj->segnum), del_obj->mtype.phys_info.velocity, del_obj->pos, LevelUniqueObjectState.ThiefState.Stolen_items);
			else if (robot_is_companion(robptr))
			{
				DropBuddyMarker(del_obj);
			}
#endif
		}

		auto &robptr{Robot_info[get_robot_id(del_obj)]};
		if (robptr.exp2_sound_num > -1)
			digi_link_sound_to_pos(robptr.exp2_sound_num, vmsegptridx(del_obj->segnum), sidenum_t::WLEFT, spawn_pos, 0, F1_0);
			//PLAY_SOUND_3D( Robot_info[del_obj->id].exp2_sound_num, spawn_pos, del_obj->segnum  );

		obj.ctype.expl_info.spawn_time = -1;

		//make debris
		if (del_obj->render_type == render_type::RT_POLYOBJ)
			explode_model(del_obj);		//explode a polygon model

		//set some parm in explosion
		//If num_objects < MAX_USED_OBJECTS, expl_obj could be set to dead before this setting causing the delete_obj not to be removed. If so, directly delete del_obj
		if (expl_obj && !(expl_obj->flags & OF_SHOULD_BE_DEAD))
		{
			if (del_obj->movement_source == object::movement_type::physics) {
				expl_obj->movement_source = object::movement_type::physics;
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
	if (obj.lifeleft <= obj.ctype.expl_info.delete_time) {
		const auto &&del_obj{vmobjptr(obj.ctype.expl_info.delete_objnum)};
		obj.ctype.expl_info.delete_time = -1;
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
void explode_wall(fvcvertptr &vcvertptr, const vcsegptridx_t segnum, const sidenum_t sidenum, wall &w)
{
	if (w.flags & wall_flag::exploding)
		/* Already exploding */
		return;
	w.explode_time_elapsed = 0;
	w.flags |= wall_flag::exploding;
	++ Num_exploding_walls;

	//play one long sound for whole door wall explosion
	digi_link_sound_to_pos(sound_effect::SOUND_EXPLODING_WALL, segnum, sidenum, compute_center_point_on_side(vcvertptr, segnum, sidenum), 0, F1_0);
}

unsigned do_exploding_wall_frame(const d_robot_info_array &Robot_info, wall &w1)
{
	auto &LevelSharedVertexState{LevelSharedSegmentState.get_vertex_state()};
	auto &Vertices{LevelSharedVertexState.get_vertices()};
	auto &WallAnims{GameSharedState.WallAnims};
	assert(w1.flags & wall_flag::exploding);
	fix w1_explode_time_elapsed{w1.explode_time_elapsed};
	const fix oldfrac{fixdiv(w1_explode_time_elapsed, EXPL_WALL_TIME)};

	w1_explode_time_elapsed += FrameTime;
	if (w1_explode_time_elapsed > EXPL_WALL_TIME)
		w1_explode_time_elapsed = EXPL_WALL_TIME;
	w1.explode_time_elapsed = w1_explode_time_elapsed;

	const auto w1sidenum{w1.sidenum};
	const auto &&seg{vmsegptridx(w1.segnum)};
	unsigned walls_updated{0};
	if (w1_explode_time_elapsed > (EXPL_WALL_TIME * 3) / 4)
	{
		const auto &&csegp{seg.absolute_sibling(seg->shared_segment::children[w1sidenum])};
		const auto cside{find_connect_side(seg, csegp)};

		const auto a{w1.clip_num};
		auto &wa{WallAnims[a]};
		const auto n{wa.num_frames};
		wall_set_tmap_num(wa, seg, w1sidenum, csegp, cside, n - 1);

		auto &Walls{LevelUniqueWallSubsystemState.Walls};
		auto &vmwallptr{Walls.vmptr};

		auto cwall_num{csegp->shared_segment::sides[cside].wall_num};
		if (cwall_num != wall_none)
		{
			auto &w2{*vmwallptr(cwall_num)};
			assert(&w1 != &w2);
			w2.flags |= wall_flag::blasted;
			assert((w1.flags & wall_flag::exploding) || (w2.flags & wall_flag::exploding));
			if (w1_explode_time_elapsed >= EXPL_WALL_TIME && w2.flags & wall_flag::exploding)
			{
				w2.flags &= ~wall_flag::exploding;
				++ walls_updated;
			}
		}
		else
			assert(w1.flags & wall_flag::exploding);

		w1.flags |= wall_flag::blasted;
		if (w1_explode_time_elapsed >= EXPL_WALL_TIME && w1.flags & wall_flag::exploding)
		{
			w1.flags &= ~wall_flag::exploding;
			++ walls_updated;
		}

		Num_exploding_walls -= walls_updated;
	}

	const fix newfrac{fixdiv(w1_explode_time_elapsed, EXPL_WALL_TIME)};

	const int old_count{f2i(EXPL_WALL_TOTAL_FIREBALLS * fixmul(oldfrac, oldfrac))};
	const int new_count{f2i(EXPL_WALL_TOTAL_FIREBALLS * fixmul(newfrac, newfrac))};
	if (old_count >= new_count)
		/* for loop would exit with zero iterations if this `if` is
		 * true.  Skip the setup for the loop in that case.
		 */
		return walls_updated;

	const auto vertnum_list{get_side_verts(seg, w1sidenum)};

	auto &vcvertptr{Vertices.vcptr};
	auto &v0{*vcvertptr(vertnum_list[0])};
	auto &v1{*vcvertptr(vertnum_list[1])};
	auto &v2{*vcvertptr(vertnum_list[2])};

	const auto &&vv0{vm_vec_sub(v0, v1)};
	const auto &&vv1{vm_vec_sub(v2, v1)};

	//now create all the next explosions

	auto &w1normal0{seg->shared_segment::sides[w1sidenum].normals[0]};
	for (int e{old_count}; e < new_count; ++e)
	{
		//calc expl position

		auto pos{vm_vec_scale_add(v1, vv0, d_rand() * 2)};
		vm_vec_scale_add2(pos,vv1,d_rand() * 2);

		const fix size{EXPL_WALL_FIREBALL_SIZE + (2 * EXPL_WALL_FIREBALL_SIZE * e / EXPL_WALL_TOTAL_FIREBALLS)};

		//fireballs start away from door, with subsequent ones getting closer
		vm_vec_scale_add2(pos, w1normal0, size * (EXPL_WALL_TOTAL_FIREBALLS - e) / EXPL_WALL_TOTAL_FIREBALLS);

		if (e & 3)		//3 of 4 are normal
			object_create_explosion_without_damage(Vclip, seg, pos, size, vclip_index::small_explosion);
		else
			object_create_badass_explosion(Robot_info, object_none, seg, pos,
										   size,
										   vclip_index::small_explosion,
										   i2f(4),		// damage strength
										   i2f(20),		//	damage radius
										   i2f(50),		//	damage force
										   object_none		//	parent id
			);
	}
	return walls_updated;
}

#if defined(DXX_BUILD_DESCENT_II)
//creates afterburner blobs behind the specified object
void drop_afterburner_blobs(object &obj, int count, fix size_scale, fix lifetime)
{
	auto pos_left{vm_vec_scale_add(obj.pos, obj.orient.fvec, -obj.size)};
	vm_vec_scale_add2(pos_left, obj.orient.rvec, -obj.size/4);
	const auto pos_right{vm_vec_scale_add(pos_left, obj.orient.rvec, obj.size / 2)};

	if (count == 1)
		pos_left = vm_vec_avg(pos_left, pos_right);

	const auto &&objseg{Segments.vmptridx(obj.segnum)};
	{
		const auto &&segnum{find_point_seg(LevelSharedSegmentState, LevelUniqueSegmentState, pos_left, objseg)};
	if (segnum != segment_none)
		object_create_explosion_without_damage(Vclip, segnum, pos_left, size_scale, vclip_index::afterburner_blob);
	}

	if (count > 1) {
		const auto &&segnum{find_point_seg(LevelSharedSegmentState, LevelUniqueSegmentState, pos_right, objseg)};
		if (segnum != segment_none) {
			const auto &&blob_obj{object_create_explosion_without_damage(Vclip, segnum, pos_right, size_scale, vclip_index::afterburner_blob)};
			if (lifetime != -1 && blob_obj != object_none)
				blob_obj->lifeleft = lifetime;
		}
	}
}

/*
 * reads n expl_wall structs from a PHYSFS_File and swaps if specified
 */
void expl_wall_read_n_swap(fvmwallptr &vmwallptr, PHYSFS_File *const fp, const physfsx_endian swap, const unsigned count)
{
	assert(!Num_exploding_walls);
	unsigned num_exploding_walls{0};
	/* Legacy versions of Descent always write a fixed number of
	 * entries, even if some or all of those entries are empty.  This
	 * loop needs to count how many entries were valid, as well as load
	 * them into the correct walls.
	 */
	for (unsigned i{count}; i--;)
	{
		disk_expl_wall d;
		PHYSFSX_readBytes(fp, &d, sizeof(d));
		if (swap != physfsx_endian::native)
		{
			d.segnum = SWAPINT(d.segnum);
			d.sidenum = SWAPINT(d.sidenum);
			d.time = SWAPINT(d.time);
		}
		if (const auto s{vcsegidx_t::check_nothrow_index(d.segnum)})
			for (const vcsegidx_t dseg{*s}; auto &w : vmwallptr)
		{
			if (w.segnum != dseg)
				continue;
			if (underlying_value(w.sidenum) != d.sidenum)
				continue;
			w.flags |= wall_flag::exploding;
			w.explode_time_elapsed = d.time;
			++ num_exploding_walls;
			break;
		}
	}
	Num_exploding_walls = num_exploding_walls;
}

void expl_wall_write(fvcwallptr &vcwallptr, PHYSFS_File *const fp)
{
	const unsigned num_exploding_walls{Num_exploding_walls};
	PHYSFSX_writeBytes(fp, &num_exploding_walls, sizeof(unsigned));
	for (auto &e : vcwallptr)
	{
		if (!(e.flags & wall_flag::exploding))
			continue;
		disk_expl_wall d;
		d.segnum = e.segnum;
		d.sidenum = underlying_value(e.sidenum);
		d.time = e.explode_time_elapsed;
		PHYSFSX_writeBytes(fp, &d, sizeof(d));
	}
}

//	------------------------------------------------------------------------------------------------------
//	Choose segment to recreate thief in.
vmsegidx_t choose_thief_recreation_segment(fvcsegptr &vcsegptr, fvcwallptr &vcwallptr, const vcsegidx_t plrseg)
{
	static constexpr std::integral_constant<connected_segment_raw_distances::segment_distance_count_type, 20> thief_max_depth{};
	static constexpr std::integral_constant<connected_segment_raw_distances::segment_distance_count_type, thief_max_depth / 2> thief_min_depth{};
	static_assert(thief_min_depth >= connected_segment_raw_distances::minimum_supported_max_depth);
	const connected_segment_raw_distances rd(vcsegptr, vcwallptr, thief_max_depth, plrseg);
	auto mrd{std::minstd_rand(d_rand())};
	/* connected_segment_raw_distances explicitly avoids reporting any
	 * segment with a ->special of segment_special::controlcen, even if that
	 * segment is in range.  Therefore, any returned segment of the
	 * appropriate distance is usable.
	 */
	static_assert(rd.count_segments_at_depth.valid_index(thief_max_depth));
	static_assert(rd.count_segments_at_depth.valid_index(thief_min_depth + 1u));
	for (const unsigned candidate_depth : xrange(thief_max_depth, thief_min_depth, xrange_descending()))
	{
		if (const auto sn = rd.scan_segment_depths(candidate_depth, mrd))
			return *sn;
	}
	/* This can be reached if there are no segments found at any of the
	 * acceptable depths.  If the origin segment is in a small room with
	 * no passable exits, this can happen.
	 *
	 * Give up and pick a completely random segment.  This is compatible
	 * with retail Descent 2.
	 */
	using distribution_type = typename std::underlying_type<segnum_t>::type;
	std::uniform_int_distribution uid(distribution_type{0}, static_cast<distribution_type>(Highest_segment_index));
	return segnum_t{uid(mrd)};
}
#endif
}
