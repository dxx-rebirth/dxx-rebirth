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
 * Routines to handle collisions
 *
 */

#include <algorithm>
#include <cstdlib>
#include <stdexcept>
#include <stdlib.h>
#include <stdio.h>
#include <type_traits>

#include "rle.h"
#include "inferno.h"
#include "game.h"
#include "gr.h"
#include "bm.h"
#include "3d.h"
#include "segment.h"
#include "laser.h"
#include "key.h"
#include "gameseg.h"
#include "object.h"
#include "physics.h"
#include "wall.h"
#include "vclip.h"
#include "polyobj.h"
#include "fireball.h"
#include "hudmsg.h"
#include "laser.h"
#include "dxxerror.h"
#include "ai.h"
#include "hostage.h"
#include "sounds.h"
#include "robot.h"
#include "weapon.h"
#include "player.h"
#include "gauges.h"
#include "powerup.h"
#include "scores.h"
#include "effects.h"
#include "textures.h"
#include "multi.h"
#include "cntrlcen.h"
#include "newdemo.h"
#include "endlevel.h"
#include "multibot.h"
#include "playsave.h"
#include "piggy.h"
#include "text.h"
#include "automap.h"
#include "switch.h"
#include "palette.h"
#include "gameseq.h"
#include "collide.h"
#include "escort.h"

#include "d_levelstate.h"
#include "partial_range.h"
#include <utility>

using std::min;

#if defined(DXX_BUILD_DESCENT_I)
constexpr auto HAS_VULCAN_AND_GAUSS_FLAGS = HAS_VULCAN_FLAG;
#elif defined(DXX_BUILD_DESCENT_II)
constexpr auto HAS_VULCAN_AND_GAUSS_FLAGS = HAS_VULCAN_FLAG | HAS_GAUSS_FLAG;
#endif

#if defined(DXX_BUILD_DESCENT_II)
namespace dsx {
namespace {
constexpr std::array<uint8_t, MAX_WEAPON_TYPES> Weapon_is_energy{{
	1, 1, 1, 1, 1,
	1, 1, 1, 0, 1,
	1, 0, 1, 1, 1,
	0, 1, 0, 0, 1,
	1, 0, 0, 1, 1,
	1, 1, 1, 0, 1,
	1, 1, 0, 1, 1,
	1
}};
}
}
#endif

#define WALL_DAMAGE_SCALE (128) // Was 32 before 8:55 am on Thursday, September 15, changed by MK, walls were hurting me more than robots!
#define WALL_DAMAGE_THRESHOLD (F1_0/3)
#define WALL_LOUDNESS_SCALE (20)
#define FORCE_DAMAGE_THRESHOLD (F1_0/3)
#define STANDARD_EXPL_DELAY (F1_0/4)

namespace dcx {
void bump_one_object(object_base &obj0, const vms_vector &hit_dir, const fix damage)
{
	phys_apply_force(obj0, vm_vec_copy_scale(hit_dir, damage));
}

namespace {

static int check_collision_delayfunc_exec()
{
	static fix64 last_play_time=0;
	if (last_play_time + (F1_0/3) < GameTime64 || last_play_time > GameTime64)
	{
		last_play_time = {GameTime64};
		last_play_time -= (d_rand()/2); // add some randomness
		return 1;
	}
	return 0;
}

static fix64 Last_volatile_scrape_time;
static fix64 Last_volatile_scrape_sound_time;

}

}

//	-------------------------------------------------------------------------------------------------------------
//	The only reason this routine is called (as of 10/12/94) is so Brain guys can open doors.
namespace dsx {
namespace {
static void collide_robot_and_wall(fvcwallptr &vcwallptr, object &robot, const vmsegptridx_t hitseg, const sidenum_t hitwall, const vms_vector &)
{
	const auto robot_id{get_robot_id(robot)};
#if defined(DXX_BUILD_DESCENT_I)
	if (robot_id == robot_id::brain || robot.ctype.ai_info.behavior == ai_behavior::AIB_RUN_FROM)
#elif defined(DXX_BUILD_DESCENT_II)
	auto &BuddyState = LevelUniqueObjectState.BuddyState;
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	auto &robptr = Robot_info[robot_id];
	if (robot_id == robot_id::brain || robot.ctype.ai_info.behavior == ai_behavior::AIB_RUN_FROM || robot_is_companion(robptr) == 1 || robot.ctype.ai_info.behavior == ai_behavior::AIB_SNIPE)
#endif
	{
		const auto wall_num = hitseg->shared_segment::sides[hitwall].wall_num;
		if (wall_num != wall_none) {
			auto &w = *vcwallptr(wall_num);
			if (w.type == WALL_DOOR && w.keys == wall_key::none && w.state == wall_state::closed && !(w.flags & wall_flag::door_locked))
			{
				wall_open_door(hitseg, hitwall);
			// -- Changed from this, 10/19/95, MK: Don't want buddy getting stranded from player
			//-- } else if ((Robot_info[robot->id].companion == 1) && (Walls[wall_num].type == WALL_DOOR) && (Walls[wall_num].keys != KEY_NONE) && (Walls[wall_num].state == WALL_DOOR_CLOSED) && !(Walls[wall_num].flags & WALL_DOOR_LOCKED)) {
			}
#if defined(DXX_BUILD_DESCENT_II)
			else if (robot_is_companion(robptr) && w.type == WALL_DOOR)
			{
				ai_local *const ailp = &robot.ctype.ai_info.ail;
				if (ailp->mode == ai_mode::AIM_GOTO_PLAYER || BuddyState.Escort_special_goal == ESCORT_GOAL_SCRAM) {
					if (w.keys != wall_key::none)
					{
						auto &player_info = get_local_plrobj().ctype.player_info;
						if (player_info.powerup_flags & static_cast<PLAYER_FLAG>(w.keys))
							wall_open_door(hitseg, hitwall);
					}
					else if (!(w.flags & wall_flag::door_locked))
						wall_open_door(hitseg, hitwall);
				}
			} else if (Robot_info[get_robot_id(robot)].thief) {		//	Thief allowed to go through doors to which player has key.
				if (w.keys != wall_key::none)
				{
					auto &player_info = get_local_plrobj().ctype.player_info;
					if (player_info.powerup_flags & static_cast<PLAYER_FLAG>(w.keys))
						wall_open_door(hitseg, hitwall);
				}
			}
#endif
		}
	}

	return;
}

//	-------------------------------------------------------------------------------------------------------------

static int apply_damage_to_clutter(const d_robot_info_array &Robot_info, const vmobjptridx_t clutter, const fix damage)
{
	if ( clutter->flags&OF_EXPLODING) return 0;

	if (clutter->shields < 0 ) return 0;	//clutter already dead...

	clutter->shields -= damage;

	if (clutter->shields < 0) {
		explode_object(LevelUniqueObjectState, Robot_info, LevelSharedSegmentState, LevelUniqueSegmentState, clutter, 0);
		return 1;
	} else
		return 0;
}

//given the specified force, apply damage from that force to an object
static void apply_force_damage(const d_robot_info_array &Robot_info, const vmobjptridx_t obj, const vm_magnitude force, const vmobjptridx_t other_obj)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	fix damage;

	if (obj->flags & (OF_EXPLODING|OF_SHOULD_BE_DEAD))
		return;		//already exploding or dead

	damage = fixdiv(force,obj->mtype.phys_info.mass) / 8;

#if defined(DXX_BUILD_DESCENT_II)
	if ((other_obj->type == OBJ_PLAYER) && cheats.monsterdamage)
		damage = INT32_MAX;
#endif

	if (damage < FORCE_DAMAGE_THRESHOLD)
		return;

	switch (obj->type) {

		case OBJ_ROBOT:
		{
			auto &robptr = Robot_info[get_robot_id(obj)];
			const auto other_type = other_obj->type;
			const auto result = apply_damage_to_robot(Robot_info, obj, (robptr.attack_type == 1)
				? damage / 4
				: damage / 2,
				(other_type == OBJ_WEAPON)
				? other_obj->ctype.laser_info.parent_num
				: static_cast<objnum_t>(other_obj));
			if (!result)
				break;

			const auto console = ConsoleObject;
			if ((other_type == OBJ_PLAYER && other_obj == console) ||
				(other_type == OBJ_WEAPON && laser_parent_is_player(vcobjptr, other_obj->ctype.laser_info, *console)))
				add_points_to_score(console->ctype.player_info, robptr.score_value, Game_mode);
			break;
		}

		case OBJ_PLAYER:

			//	If colliding with a claw type robot, do damage proportional to FrameTime because you can collide with those
			//	bots every frame since they don't move.
			if ( (other_obj->type == OBJ_ROBOT) && (Robot_info[get_robot_id(other_obj)].attack_type) )
				damage = fixmul(damage, FrameTime*2);

#if defined(DXX_BUILD_DESCENT_II)
			//	Make trainee easier.
			if (GameUniqueState.Difficulty_level == Difficulty_level_type::_0)
				damage /= 2;
#endif

			apply_damage_to_player(obj,other_obj,damage, apply_damage_player::always);
			break;

		case OBJ_CLUTTER:

			apply_damage_to_clutter(Robot_info, obj,damage);
			break;

		case OBJ_CNTRLCEN: // Never hits! Reactor does not have object::movement_type::physics - it's stationary! So no force damage here.
			apply_damage_to_controlcen(Robot_info, obj, damage, other_obj);
			break;

		case OBJ_WEAPON:

			break; //weapons don't take damage

		default:

			Int3();

	}
}

//	-----------------------------------------------------------------------------
static void bump_this_object(const d_robot_info_array &Robot_info, const vmobjptridx_t objp, const vmobjptridx_t other_objp, const vms_vector &force, int damage_flag)
{
	if (! (objp->mtype.phys_info.flags & PF_PERSISTENT))
	{
		if (objp->type == OBJ_PLAYER) {
			vms_vector force2;
			force2.x = force.x/4;
			force2.y = force.y/4;
			force2.z = force.z/4;
			phys_apply_force(objp,force2);
			if (damage_flag && (other_objp->type != OBJ_ROBOT || !robot_is_companion(Robot_info[get_robot_id(other_objp)])))
			{
				auto force_mag = vm_vec_mag_quick(force2);
				apply_force_damage(Robot_info, objp, force_mag, other_objp);
			}
		} else if ((objp->type == OBJ_ROBOT) || (objp->type == OBJ_CLUTTER) || (objp->type == OBJ_CNTRLCEN)) {
			if (Robot_info[get_robot_id(objp)].boss_flag == boss_robot_id::None)
			{
				vms_vector force2;
				const auto Difficulty_level = underlying_value(GameUniqueState.Difficulty_level);
				force2.x = force.x/(4 + Difficulty_level);
				force2.y = force.y/(4 + Difficulty_level);
				force2.z = force.z/(4 + Difficulty_level);

				phys_apply_force(objp, force);
				phys_apply_rot(objp, force2);
				if (damage_flag) {
					auto force_mag = vm_vec_mag_quick(force);
					apply_force_damage(Robot_info, objp, force_mag, other_objp);
				}
			}
		}
	}
}

//	-----------------------------------------------------------------------------
//deal with two objects bumping into each other.  Apply force from collision
//to each robot.  The flags tells whether the objects should take damage from
//the collision.
static void bump_two_objects(const d_robot_info_array &Robot_info, const vmobjptridx_t obj0,const vmobjptridx_t obj1,int damage_flag)
{
	const vmobjptridx_t *pt;
	if ((obj0->movement_source != object::movement_type::physics && (pt = &obj1, true)) ||
		(obj1->movement_source != object::movement_type::physics && (pt = &obj0, true)))
	{
		object_base &t = *pt;
		Assert(t.movement_source == object::movement_type::physics);
		phys_apply_force(t, vm_vec_copy_scale(t.mtype.phys_info.velocity, -t.mtype.phys_info.mass));
		return;
	}

	auto force{vm_vec_sub(obj0->mtype.phys_info.velocity, obj1->mtype.phys_info.velocity)};
	vm_vec_scale2(force,2*fixmul(obj0->mtype.phys_info.mass,obj1->mtype.phys_info.mass),(obj0->mtype.phys_info.mass+obj1->mtype.phys_info.mass));

	bump_this_object(Robot_info, obj1, obj0, force, damage_flag);
	bump_this_object(Robot_info, obj0, obj1, vm_vec_negated(force), damage_flag);
}

static void collide_player_and_wall(const vmobjptridx_t playerobj, const fix hitspeed, const vmsegptridx_t hitseg, const sidenum_t hitwall, const vms_vector &hitpt)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	auto &TmapInfo = LevelUniqueTmapInfoState.TmapInfo;
	fix damage;

	if (get_player_id(playerobj) != Player_num) // Execute only for local player
		return;

	auto &tmi1 = TmapInfo[get_texture_index(hitseg->unique_segment::sides[hitwall].tmap_num)];

	//	If this wall does damage, don't make *BONK* sound, we'll be making another sound.
	if (tmi1.damage > 0)
		return;

#if defined(DXX_BUILD_DESCENT_I)
	const int ForceFieldHit = 0;
#elif defined(DXX_BUILD_DESCENT_II)
	int ForceFieldHit=0;
	if (tmi1.flags & tmapinfo_flag::force_field)
	{
		vms_vector force;

		PALETTE_FLASH_ADD(0, 0, 60);	//flash blue

		//knock player around
		force.x = 40*(d_rand() - 16384);
		force.y = 40*(d_rand() - 16384);
		force.z = 40*(d_rand() - 16384);
		phys_apply_rot(playerobj, force);

		//make sound
		multi_digi_link_sound_to_pos(SOUND_FORCEFIELD_BOUNCE_PLAYER, hitseg, sidenum_t::WLEFT, hitpt, 0, f1_0);
		ForceFieldHit=1;
	}
	else
#endif
	{
		auto &player_info = get_local_plrobj().ctype.player_info;
		wall_hit_process(player_info.powerup_flags, hitseg, hitwall, 20, get_player_id(playerobj), playerobj);
	}

	//	** Damage from hitting wall **
	//	If the player has less than 10% shields, don't take damage from bump
#if defined(DXX_BUILD_DESCENT_I)
	damage = hitspeed / WALL_DAMAGE_SCALE;
#elif defined(DXX_BUILD_DESCENT_II)
	// Note: Does quad damage if hit a force field - JL
	damage = (hitspeed / WALL_DAMAGE_SCALE) * (ForceFieldHit*8 + 1);

	const auto tmap_num2 = hitseg->unique_segment::sides[hitwall].tmap_num2;

	//don't do wall damage and sound if hit lava or water
	constexpr auto tmi_no_damage = (tmapinfo_flag::water | tmapinfo_flag::lava);
	if ((tmi1.flags & tmi_no_damage) || (tmap_num2 != texture2_value::None && (TmapInfo[get_texture_index(tmap_num2)].flags & tmi_no_damage)))
		damage = 0;
#endif

	if (damage >= WALL_DAMAGE_THRESHOLD) {
		int	volume;
		volume = (hitspeed-(WALL_DAMAGE_SCALE*WALL_DAMAGE_THRESHOLD)) / WALL_LOUDNESS_SCALE ;

		create_awareness_event(playerobj, player_awareness_type_t::PA_WEAPON_WALL_COLLISION, LevelUniqueRobotAwarenessState);

		if ( volume > F1_0 )
			volume = F1_0;
		if (volume > 0 && !ForceFieldHit) {  // uhhhgly hack
			multi_digi_link_sound_to_pos(SOUND_PLAYER_HIT_WALL, hitseg, sidenum_t::WLEFT, hitpt, 0, volume);
		}

		auto &player_info = playerobj->ctype.player_info;
		if (!(player_info.powerup_flags & PLAYER_FLAGS_INVULNERABLE))
			if (playerobj->shields > f1_0*10 || ForceFieldHit)
				apply_damage_to_player(playerobj, playerobj, damage, apply_damage_player::always);

		// -- No point in doing this unless we compute a reasonable hitpt.  Currently it is just the player's position. --MK, 01/18/96
		// -- if (!(TmapInfo[Segments[hitseg].unique_segment::sides[hitwall].tmap_num].flags & TMI_FORCE_FIELD)) {
		// -- 	vms_vector	hitpt1;
		// -- 	int			hitseg1;
		// --
		// -- 		vm_vec_avg(&hitpt1, hitpt, &Objects[Players[Player_num].objnum].pos);
		// -- 	hitseg1 = find_point_seg(&hitpt1, Objects[Players[Player_num].objnum].segnum);
		// -- 	if (hitseg1 != -1)
		// -- 		object_create_explosion( hitseg, hitpt, Weapon_info[0].impact_size, Weapon_info[0].wall_hit_vclip );
		// -- }

	}

	return;
}
}

#if defined(DXX_BUILD_DESCENT_I)
static
#endif
//see if wall is volatile or water
//if volatile, cause damage to player
//returns 1=lava, 2=water
volatile_wall_result check_volatile_wall(const vmobjptridx_t obj, const unique_side &side)
{
	Assert(obj->type==OBJ_PLAYER);

	auto &TmapInfo = LevelUniqueTmapInfoState.TmapInfo;
	const auto &tmi1 = TmapInfo[get_texture_index(side.tmap_num)];
	const fix d = tmi1.damage;
	if (d > 0
#if defined(DXX_BUILD_DESCENT_II)
		|| (tmi1.flags & tmapinfo_flag::water)
#endif
		)
	{
#if defined(DXX_BUILD_DESCENT_II)
		if (get_player_id(obj) == Player_num)
#endif
		{
			if (!((GameTime64 > Last_volatile_scrape_time + DESIGNATED_GAME_FRAMETIME) || (GameTime64 < Last_volatile_scrape_time)))
				return volatile_wall_result::none;
			Last_volatile_scrape_time = {GameTime64};

#if defined(DXX_BUILD_DESCENT_II)
			if (d > 0)
#endif
			{
				fix damage = fixmul(d,((FrameTime>DESIGNATED_GAME_FRAMETIME)?FrameTime:DESIGNATED_GAME_FRAMETIME));

#if defined(DXX_BUILD_DESCENT_II)
				if (GameUniqueState.Difficulty_level == Difficulty_level_type::_0)
					damage /= 2;
#endif

				apply_damage_to_player(obj, obj, damage, apply_damage_player::always);
				PALETTE_FLASH_ADD(f2i(damage*4), 0, 0);	//flash red
			}

			obj->mtype.phys_info.rotvel.x = (d_rand() - 16384)/2;
			obj->mtype.phys_info.rotvel.z = (d_rand() - 16384)/2;
		}

		return
#if defined(DXX_BUILD_DESCENT_II)
			(d <= 0)
			? volatile_wall_result::water
			:
#endif
			volatile_wall_result::lava;
	}
	else
	 {
		return volatile_wall_result::none;
	 }
}

//this gets called when an object is scraping along the wall
bool scrape_player_on_wall(const vmobjptridx_t obj, const vmsegptridx_t hitseg, const sidenum_t hitside, const vms_vector &hitpt)
{
	if (obj->type != OBJ_PLAYER || get_player_id(obj) != Player_num)
		return false;

	const auto type = check_volatile_wall(obj, hitseg->unique_segment::sides[hitside]);
	if (type != volatile_wall_result::none)
	{
		if ((GameTime64 > Last_volatile_scrape_sound_time + F1_0/4) || (GameTime64 < Last_volatile_scrape_sound_time)) {
			Last_volatile_scrape_sound_time = {GameTime64};
			const auto sound =
#if defined(DXX_BUILD_DESCENT_II)
				(type != volatile_wall_result::lava)
				? SOUND_SHIP_IN_WATER
				:
#endif
				SOUND_VOLATILE_WALL_HISS;
			multi_digi_link_sound_to_pos(sound, hitseg, sidenum_t::WLEFT, hitpt, 0, F1_0);
		}

		auto hit_dir = hitseg->shared_segment::sides[hitside].normals[0];

		vm_vec_scale_add2(hit_dir, make_random_vector(), F1_0/8);
		vm_vec_normalize_quick(hit_dir);
		bump_one_object(obj, hit_dir, F1_0*8);

		return true;
	}
	return false;
}

#if defined(DXX_BUILD_DESCENT_II)
namespace {
static int effect_parent_is_guidebot(fvcobjptr &vcobjptr, const laser_parent &laser)
{
	if (laser.parent_type != OBJ_ROBOT)
		return 0;
	const auto &&robot = vcobjptr(laser.parent_num);
	if (robot->type != OBJ_ROBOT)
		return 0;
	if (robot->signature != laser.parent_signature)
		/* parent replaced, no idea what it once was */
		return 0;
	const auto robot_id = get_robot_id(robot);
	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	auto &robptr = Robot_info[robot_id];
	return robot_is_companion(robptr);
}
}
#endif

//if an effect is hit, and it can blow up, then blow it up
//returns true if it blew up
int check_effect_blowup(const d_level_shared_destructible_light_state &LevelSharedDestructibleLightState, const d_vclip_array &Vclip, const vmsegptridx_t seg, const sidenum_t side, const vms_vector &pnt, const laser_parent &blower, int force_blowup_flag, int remote)
{
	auto &Effects = LevelUniqueEffectsClipState.Effects;
	auto &TmapInfo = LevelUniqueTmapInfoState.TmapInfo;
#if defined(DXX_BUILD_DESCENT_I)
	static constexpr std::integral_constant<int, 0> force_blowup_flag{};
#elif defined(DXX_BUILD_DESCENT_II)
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	const auto wall_num = seg->shared_segment::sides[side].wall_num;

	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcwallptr = Walls.vcptr;
	const auto is_trigger = wall_num != wall_none && vcwallptr(wall_num)->trigger != trigger_none;
	if (is_trigger)
	{
		if (force_blowup_flag || (
				(Game_mode & GM_MULTI)
	// If this wall has a trigger and the blower-upper is not the player or the buddy, abort!
	// For Multiplayer perform an additional check to see if it's a local-player hit. If a remote player hits, a packet is expected (remote 1) which would be followed by MULTI_TRIGGER to ensure sync with the switch and the actual trigger.
				? (!(blower.parent_type == OBJ_PLAYER && (blower.parent_num == get_local_player().objnum || remote)))
				: !(blower.parent_type == OBJ_PLAYER || effect_parent_is_guidebot(vcobjptr, blower)))
			)
			return(0);
	}
#endif

	if (const auto tmap2 = seg->unique_segment::sides[side].tmap_num2; tmap2 != texture2_value::None)
	{
		const auto tm = get_texture_index(tmap2);			//tm without flags
		auto &tmi2 = TmapInfo[tm];
		const auto ec = tmi2.eclip_num;
		unsigned db = 0;
		if (
#if defined(DXX_BUILD_DESCENT_I)
			ec != eclip_none &&
#elif defined(DXX_BUILD_DESCENT_II)
		//check if it's an animation (monitor) or casts light
			ec == eclip_none
			? tmi2.destroyed != -1
			:
#endif
			(db = Effects[ec].dest_bm_num) != ~0u && !(Effects[ec].flags & EF_ONE_SHOT)
		)
		{
			const auto tmf = get_texture_rotation_high(tmap2);		//tm flags
			auto &texture2 = Textures[tm];
			const grs_bitmap *bm = &GameBitmaps[texture2];
			int x=0,y=0,t;

			PIGGY_PAGE_IN(texture2);

			//this can be blown up...did we hit it?

			if (!force_blowup_flag) {
				const auto hitpoint = find_hitpoint_uv(pnt,seg,side,0);	//evil: always say face zero
				auto &u = hitpoint.u;
				auto &v = hitpoint.v;

				x = static_cast<unsigned>(f2i(u*bm->bm_w)) % bm->bm_w;
				y = static_cast<unsigned>(f2i(v*bm->bm_h)) % bm->bm_h;

				switch (tmf) {		//adjust for orientation of paste-on
					case texture2_rotation_high::Normal:
						break;
					case texture2_rotation_high::_1:
						t = y;
						y = x;
						x = bm->bm_w - t - 1;
						break;
					case texture2_rotation_high::_2:
						y = bm->bm_h - y - 1;
						x = bm->bm_w - x - 1;
						break;
					case texture2_rotation_high::_3:
						t = x;
						x = y;
						y = bm->bm_h - t - 1;
						break;
				}
				bm = rle_expand_texture(*bm);
			}

#if defined(DXX_BUILD_DESCENT_I)
			if (gr_gpixel (*bm, x, y) != TRANSPARENCY_COLOR)
#elif defined(DXX_BUILD_DESCENT_II)
			if (force_blowup_flag || (bm->bm_data[y*bm->bm_w+x] != TRANSPARENCY_COLOR))
#endif
			{		//not trans, thus on effect
				int sound_num;

#if defined(DXX_BUILD_DESCENT_II)
				if ((Game_mode & GM_MULTI) && Netgame.AlwaysLighting)
					if (!(ec != eclip_none && db != -1 && !(Effects[ec].flags & EF_ONE_SHOT)))
						return 0;

				//note: this must get called before the texture changes,
				//because we use the light value of the texture to change
				//the static light in the segment
				subtract_light(LevelSharedDestructibleLightState, seg, side);

				// we blew up something connected to a trigger. Send it to others!
				if ((Game_mode & GM_MULTI) && is_trigger && !remote && !force_blowup_flag)
					multi_send_effect_blowup(seg, side, pnt);
#endif
				if (Newdemo_state == ND_STATE_RECORDING)
					newdemo_record_effect_blowup( seg, side, pnt);

				fix dest_size;
				vclip_index vc;
				if (ec != eclip_none)
				{
					dest_size = Effects[ec].dest_size;
					vc = Effects[ec].dest_vclip;
				}
				else
				{
					dest_size = i2f(20);
					vc = vclip_index{3};
				}

				object_create_explosion_without_damage(Vclip, seg, pnt, dest_size, vc);

#if defined(DXX_BUILD_DESCENT_II)
				if (ec != eclip_none && db != -1 && !(Effects[ec].flags & EF_ONE_SHOT))
#endif
				{

					if ((sound_num = Vclip[vc].sound_num) != -1)
		  				digi_link_sound_to_pos( sound_num, seg, sidenum_t::WLEFT, pnt,  0, F1_0 );

					if ((sound_num=Effects[ec].sound_num)!=-1)		//kill sound
						digi_kill_sound_linked_to_segment(seg,side,sound_num);

					if (Effects[ec].dest_eclip!=-1 && Effects[Effects[ec].dest_eclip].segnum == segment_none)
					{
						int bm_num;
						eclip *new_ec;

						new_ec = &Effects[Effects[ec].dest_eclip];
						bm_num = new_ec->changing_wall_texture;

						new_ec->time_left = new_ec->vc.frame_time;
						new_ec->frame_count = 0;
						new_ec->segnum = seg;
						new_ec->sidenum = side;
						new_ec->flags |= EF_ONE_SHOT;
						new_ec->dest_bm_num = Effects[ec].dest_bm_num;

						assert(bm_num != 0);
						auto &tmap_num2 = seg->unique_segment::sides[side].tmap_num2;
						tmap_num2 = build_texture2_value(bm_num, tmf);		//replace with destroyed

					}
					else {
						assert(db != 0);
						auto &tmap_num2 = seg->unique_segment::sides[side].tmap_num2;
						tmap_num2 = build_texture2_value(db, tmf);		//replace with destroyed
					}
				}
#if defined(DXX_BUILD_DESCENT_II)
				else {
					seg->unique_segment::sides[side].tmap_num2 = build_texture2_value(tmi2.destroyed, tmf);

					//assume this is a light, and play light sound
		  			digi_link_sound_to_pos( SOUND_LIGHT_BLOWNUP, seg, sidenum_t::WLEFT, pnt,  0, F1_0 );
				}
#endif

				return 1;		//blew up!
			}
		}
	}

	return 0;		//didn't blow up
}

//these gets added to the weapon's values when the weapon hits a volitle wall
#define VOLATILE_WALL_EXPL_STRENGTH i2f(10)
#define VOLATILE_WALL_IMPACT_SIZE	i2f(3)
#define VOLATILE_WALL_DAMAGE_FORCE	i2f(5)
#define VOLATILE_WALL_DAMAGE_RADIUS	i2f(30)

namespace {

static window_event_result collide_weapon_and_wall(
#if defined(DXX_BUILD_DESCENT_II)
	const d_level_shared_destructible_light_state &LevelSharedDestructibleLightState,
#endif
	const d_robot_info_array &Robot_info, object_array &Objects, fvmsegptridx &vmsegptridx, const vmobjptridx_t weapon, const vmsegptridx_t hitseg, const sidenum_t hitwall, const vms_vector &hitpt)
{
	auto &TmapInfo = LevelUniqueTmapInfoState.TmapInfo;
	auto &imobjptridx = Objects.imptridx;
	auto &vcobjptr = Objects.vcptr;
	auto &vmobjptr = Objects.vmptr;
	int blew_up;
	int playernum;
	auto result = window_event_result::handled;

#if defined(DXX_BUILD_DESCENT_I)
	if (weapon->mtype.phys_info.flags & PF_BOUNCE)
		return window_event_result::ignored;
	auto &uhitside = hitseg->unique_segment::sides[hitwall];
#elif defined(DXX_BUILD_DESCENT_II)
	auto &vcobjptridx = Objects.vcptridx;
	if (get_weapon_id(weapon) == weapon_id_type::OMEGA_ID)
		if (!ok_to_do_omega_damage(weapon)) // see comment in laser.c
			return window_event_result::ignored;

	//	If this is a guided missile and it strikes fairly directly, clear bounce flag.
	if (get_weapon_id(weapon) == weapon_id_type::GUIDEDMISS_ID) {
		fix	dot;

		dot = vm_vec_dot(weapon->orient.fvec, hitseg->shared_segment::sides[hitwall].normals[0]);
		if (dot < -F1_0/6) {
			weapon->mtype.phys_info.flags &= ~PF_BOUNCE;
		}
	}

	auto &uhitside = hitseg->unique_segment::sides[hitwall];
	auto &tmi1 = TmapInfo[get_texture_index(uhitside.tmap_num)];
	//if an energy weapon hits a forcefield, let it bounce
	if ((tmi1.flags & tmapinfo_flag::force_field) &&
		 !(weapon->type == OBJ_WEAPON && Weapon_info[get_weapon_id(weapon)].energy_usage==0)) {

		//make sound
		multi_digi_link_sound_to_pos(SOUND_FORCEFIELD_BOUNCE_WEAPON, hitseg, sidenum_t::WLEFT, hitpt, 0, f1_0);
		return window_event_result::ignored;	//bail here. physics code will bounce this object
	}
#endif

	#ifndef NDEBUG
	if (keyd_pressed[KEY_LAPOSTRO])
		if (weapon->ctype.laser_info.parent_num == get_local_player().objnum) {
			//	MK: Real pain when you need to know a seg:side and you've got quad lasers.
			HUD_init_message(HM_DEFAULT, "Hit at segment = %hu, side = %i", static_cast<vmsegptridx_t::integral_type>(hitseg), underlying_value(hitwall));
#if defined(DXX_BUILD_DESCENT_II)
			if (get_weapon_id(weapon) < 4)
				subtract_light(LevelSharedDestructibleLightState, hitseg, hitwall);
			else if (get_weapon_id(weapon) == weapon_id_type::FLARE_ID)
				add_light(LevelSharedDestructibleLightState, hitseg, hitwall);
#endif
		}
	#endif

	if (weapon->mtype.phys_info.velocity == vms_vector{})
	{
		Int3();	//	Contact Matt: This is impossible.  A weapon with 0 velocity hit a wall, which doesn't move.
		return window_event_result::ignored;
	}

	blew_up = check_effect_blowup(LevelSharedDestructibleLightState, Vclip, hitseg, hitwall, hitpt, weapon->ctype.laser_info, 0, 0);

#if defined(DXX_BUILD_DESCENT_I)
	const unsigned robot_escort = 0;
#elif defined(DXX_BUILD_DESCENT_II)
	const unsigned robot_escort = effect_parent_is_guidebot(vcobjptr, weapon->ctype.laser_info);
	if (robot_escort) {
		/* Guidebot is always associated with the host */
		playernum = 0;
	}
	else
#endif
	{
		auto &objp = *vcobjptr(weapon->ctype.laser_info.parent_num);
		if (objp.type == OBJ_PLAYER)
			playernum = get_player_id(objp);
		else
			playernum = -1;		//not a player (thus a robot)
	}

	auto &plrobj = get_local_plrobj();
	auto &player_info = plrobj.ctype.player_info;
#if defined(DXX_BUILD_DESCENT_II)
	if (blew_up) {		//could be a wall switch
		//for wall triggers, always say that the player shot it out.  This is
		//because robots can shoot out wall triggers, and so the trigger better
		//take effect
		//	NO -- Changed by MK, 10/18/95.  We don't want robots blowing puzzles.  Only player or buddy can open!
		result = check_trigger(hitseg, hitwall, plrobj, vcobjptridx(weapon->ctype.laser_info.parent_num), 1);
	}

	if (get_weapon_id(weapon) == weapon_id_type::EARTHSHAKER_ID)
		smega_rock_stuff();
#endif

	const auto wall_type = wall_hit_process(player_info.powerup_flags, hitseg, hitwall, weapon->shields, playernum, weapon);

	const auto Difficulty_level = GameUniqueState.Difficulty_level;
#if defined(DXX_BUILD_DESCENT_I)
	auto &tmi1 = TmapInfo[get_texture_index(uhitside.tmap_num)];
#endif
	// Wall is volatile if either tmap 1 or 2 is volatile
	if ((tmi1.flags & tmapinfo_flag::lava) ||
		(uhitside.tmap_num2 != texture2_value::None && (TmapInfo[get_texture_index(uhitside.tmap_num2)].flags & tmapinfo_flag::lava))
		)
	{
		weapon_info *wi = &Weapon_info[get_weapon_id(weapon)];

		//we've hit a volatile wall

		digi_link_sound_to_pos( SOUND_VOLATILE_WALL_HIT,hitseg, sidenum_t::WLEFT, hitpt, 0, F1_0 );

		//	New by MK: If powerful badass, explode as badass, not due to lava, fixes megas being wimpy in lava.
		if (wi->damage_radius >= VOLATILE_WALL_DAMAGE_RADIUS/2) {
			explode_badass_weapon(Robot_info, weapon, hitpt);
		}
		else
		{
			object_create_badass_explosion(Robot_info, weapon, hitseg, hitpt,
				wi->impact_size + VOLATILE_WALL_IMPACT_SIZE,
		//for most weapons, use volatile wall hit.  For mega, use its special vclip
				(get_weapon_id(weapon) == weapon_id_type::MEGA_ID) ? wi->robot_hit_vclip : vclip_index::volatile_wall_hit,
				wi->strength[Difficulty_level]/4+VOLATILE_WALL_EXPL_STRENGTH,	//	diminished by mk on 12/08/94, i was doing 70 damage hitting lava on lvl 1.
				wi->damage_radius+VOLATILE_WALL_DAMAGE_RADIUS,
				wi->strength[Difficulty_level]/2+VOLATILE_WALL_DAMAGE_FORCE,
				imobjptridx(weapon->ctype.laser_info.parent_num));
		}

		weapon->flags |= OF_SHOULD_BE_DEAD;		//make flares die in lava

	}
#if defined(DXX_BUILD_DESCENT_II)
	else if ((tmi1.flags & tmapinfo_flag::water) ||
			(uhitside.tmap_num2 != texture2_value::None && (TmapInfo[get_texture_index(uhitside.tmap_num2)].flags & tmapinfo_flag::water))
			)
	{
		weapon_info *wi = &Weapon_info[get_weapon_id(weapon)];

		//we've hit water

		//	MK: 09/13/95: Badass in water is 1/2 normal intensity.
		if (Weapon_info[get_weapon_id(weapon)].matter != weapon_info::matter_flag::energy)
		{

			digi_link_sound_to_pos( SOUND_MISSILE_HIT_WATER,hitseg, sidenum_t::WLEFT, hitpt, 0, F1_0 );

			if ( Weapon_info[get_weapon_id(weapon)].damage_radius ) {

				digi_link_sound_to_object(SOUND_BADASS_EXPLOSION, weapon, 0, F1_0, sound_stack::allow_stacking);

				//	MK: 09/13/95: Badass in water is 1/2 normal intensity.
				object_create_badass_explosion(Robot_info, weapon, hitseg, hitpt,
					wi->impact_size/2,
					wi->robot_hit_vclip,
					wi->strength[Difficulty_level]/4,
					wi->damage_radius,
					wi->strength[Difficulty_level]/2,
					imobjptridx(weapon->ctype.laser_info.parent_num));
			}
			else
				object_create_explosion_without_damage(Vclip, vmsegptridx(weapon->segnum), weapon->pos, wi->impact_size, wi->wall_hit_vclip);

		} else {
			digi_link_sound_to_pos( SOUND_LASER_HIT_WATER,hitseg, sidenum_t::WLEFT, hitpt, 0, F1_0 );
			object_create_explosion_without_damage(Vclip, vmsegptridx(weapon->segnum), weapon->pos, wi->impact_size, vclip_index::water_hit);
		}

		weapon->flags |= OF_SHOULD_BE_DEAD;		//make flares die in water

	}
#endif
	else {

#if defined(DXX_BUILD_DESCENT_II)
		if (weapon->mtype.phys_info.flags & PF_BOUNCE) {

			//do special bound sound & effect

		}
		else
#endif
		{

			//if it's not the player's weapon, or it is the player's and there
			//is no wall, and no blowing up monitor, then play sound
			if ((weapon->ctype.laser_info.parent_type != OBJ_PLAYER) ||	((hitseg->shared_segment::sides[hitwall].wall_num == wall_none || wall_type == wall_hit_process_t::WHP_NOT_SPECIAL) && !blew_up))
				if ((Weapon_info[get_weapon_id(weapon)].wall_hit_sound > -1 ) && (!(weapon->flags & OF_SILENT)))
				digi_link_sound_to_pos(Weapon_info[get_weapon_id(weapon)].wall_hit_sound, vmsegptridx(weapon->segnum), sidenum_t::WLEFT, weapon->pos, 0, F1_0);

			if (const auto wall_hit_vclip = Weapon_info[get_weapon_id(weapon)].wall_hit_vclip; Vclip.valid_index(wall_hit_vclip))
			{
				if ( Weapon_info[get_weapon_id(weapon)].damage_radius )
#if defined(DXX_BUILD_DESCENT_I)
					explode_badass_weapon(Robot_info, weapon, weapon->pos);
#elif defined(DXX_BUILD_DESCENT_II)
					explode_badass_weapon(Robot_info, weapon, hitpt);
#endif
				else
					object_create_explosion_without_damage(Vclip, vmsegptridx(weapon->segnum), weapon->pos, Weapon_info[get_weapon_id(weapon)].impact_size, wall_hit_vclip);
			}
		}
	}

	//	If weapon fired by player or companion...
	if (( weapon->ctype.laser_info.parent_type== OBJ_PLAYER ) || robot_escort) {

		if (!(weapon->flags & OF_SILENT) && (weapon->ctype.laser_info.parent_num == get_local_player().objnum))
			create_awareness_event(weapon, player_awareness_type_t::PA_WEAPON_WALL_COLLISION, LevelUniqueRobotAwarenessState);			// object "weapon" can attract attention to player

//	We now allow flares to open doors.
		{
#if defined(DXX_BUILD_DESCENT_I)
			if (get_weapon_id(weapon) != weapon_id_type::FLARE_ID)
				weapon->flags |= OF_SHOULD_BE_DEAD;
#elif defined(DXX_BUILD_DESCENT_II)
			if (((get_weapon_id(weapon) != weapon_id_type::FLARE_ID) || (weapon->ctype.laser_info.parent_type != OBJ_PLAYER)) && !(weapon->mtype.phys_info.flags & PF_BOUNCE))
				weapon->flags |= OF_SHOULD_BE_DEAD;

			//don't let flares stick in force fields
			if ((get_weapon_id(weapon) == weapon_id_type::FLARE_ID) && (tmi1.flags & tmapinfo_flag::force_field))
				weapon->flags |= OF_SHOULD_BE_DEAD;
#endif

			if (!(weapon->flags & OF_SILENT)) {
				switch (wall_type) {

					case wall_hit_process_t::WHP_NOT_SPECIAL:
						//should be handled above
						//digi_link_sound_to_pos( Weapon_info[weapon->id].wall_hit_sound, weapon->segnum, 0, &weapon->pos, 0, F1_0 );
						break;

					case wall_hit_process_t::WHP_NO_KEY:
						//play special hit door sound (if/when we get it)
						multi_digi_link_sound_to_pos(SOUND_WEAPON_HIT_DOOR, vmsegptridx(weapon->segnum), sidenum_t::WLEFT, weapon->pos, 0, F1_0);

						break;

					case wall_hit_process_t::WHP_BLASTABLE:
						//play special blastable wall sound (if/when we get it)
#if defined(DXX_BUILD_DESCENT_II)
						if ((Weapon_info[get_weapon_id(weapon)].wall_hit_sound > -1 ) && (!(weapon->flags & OF_SILENT)))
#endif
							digi_link_sound_to_pos(SOUND_WEAPON_HIT_BLASTABLE, vmsegptridx(weapon->segnum), sidenum_t::WLEFT, weapon->pos, 0, F1_0);
						break;

					case wall_hit_process_t::WHP_DOOR:
						//don't play anything, since door open sound will play
						break;
				}
			} // else
		} // else {
			//	if (weapon->lifeleft <= 0)
			//	weapon->flags |= OF_SHOULD_BE_DEAD;
		// }

	} else {
		// This is a robot's laser
#if defined(DXX_BUILD_DESCENT_II)
		if (!(weapon->mtype.phys_info.flags & PF_BOUNCE))
#endif
			weapon->flags |= OF_SHOULD_BE_DEAD;
	}

	return result;
}
}
}

namespace {

static void collide_debris_and_wall(const d_robot_info_array &Robot_info, const vmobjptridx_t debris, const unique_segment &hitseg, const sidenum_t hitwall, const vms_vector &)
{
	auto &TmapInfo = LevelUniqueTmapInfoState.TmapInfo;
	if (!PERSISTENT_DEBRIS || TmapInfo[get_texture_index(hitseg.sides[hitwall].tmap_num)].damage)
		explode_object(LevelUniqueObjectState, Robot_info, LevelSharedSegmentState, LevelUniqueSegmentState, debris, 0);
}

//	-------------------------------------------------------------------------------------------------------------------
static void collide_robot_and_robot(const d_robot_info_array &Robot_info, const vmobjptridx_t robot1, const vmobjptridx_t robot2, const vms_vector &)
{
	bump_two_objects(Robot_info, robot1, robot2, 1);
}

static void collide_robot_and_controlcen(const d_robot_info_array &, object_base &obj_robot, const object_base &obj_cc, const vms_vector &)
{
	assert(obj_cc.type == OBJ_CNTRLCEN);
	assert(obj_robot.type == OBJ_ROBOT);
	const auto &&hitvec = vm_vec_normalized(vm_vec_sub(obj_cc.pos, obj_robot.pos));
	bump_one_object(obj_robot, hitvec, 0);
}

}

namespace dsx {

namespace {

static void collide_robot_and_player(const d_robot_info_array &Robot_info, const vmobjptridx_t robot, const vmobjptridx_t playerobj, const vms_vector &collision_point)
{
#if defined(DXX_BUILD_DESCENT_II)
	int	steal_attempt = 0;

	if (robot->flags&OF_EXPLODING)
		return;
#endif

	if (get_player_id(playerobj) == Player_num) {
#if defined(DXX_BUILD_DESCENT_II)
		auto &robptr = Robot_info[get_robot_id(robot)];
		if (robot_is_companion(robptr))	//	Player and companion don't collide.
			return;
		if (robptr.kamikaze) {
			apply_damage_to_robot(Robot_info, robot, robot->shields+1, playerobj);
			if (playerobj == ConsoleObject)
				add_points_to_score(playerobj->ctype.player_info, robptr.score_value, Game_mode);
		}

		if (robot_is_thief(robptr)) {
			static fix64 Last_thief_hit_time;
			ai_local		*ailp = &robot->ctype.ai_info.ail;
			if (ailp->mode == ai_mode::AIM_THIEF_ATTACK)
			{
				Last_thief_hit_time = {GameTime64};
				attempt_to_steal_item(robot, robptr, playerobj);
				steal_attempt = 1;
			} else if (GameTime64 - Last_thief_hit_time < F1_0*2)
				return;		//	ZOUNDS!  BRILLIANT!  Thief not collide with player if not stealing!
								// NO!  VERY DUMB!  makes thief look very stupid if player hits him while cloaked! -AP
			else
				Last_thief_hit_time = {GameTime64};
		}
#endif

		create_awareness_event(playerobj, player_awareness_type_t::PA_PLAYER_COLLISION, LevelUniqueRobotAwarenessState);			// object robot can attract attention to player
		do_ai_robot_hit_attack(Robot_info, robot, playerobj, collision_point);
		do_ai_robot_hit(robot, robptr, player_awareness_type_t::PA_WEAPON_ROBOT_COLLISION);
	}
	else
	{
		multi_robot_request_change(robot, get_player_id(playerobj));
		return; // only controlling player should make damage otherwise we might juggle robot back and forth, killing it instantly
	}

	if (check_collision_delayfunc_exec())
	{
		const auto &&player_segp = Segments.vmptridx(playerobj->segnum);
		const auto &&collision_seg = find_point_seg(LevelSharedSegmentState, LevelUniqueSegmentState, collision_point, player_segp);

#if defined(DXX_BUILD_DESCENT_II)
		// added this if to remove the bump sound if it's the thief.
		// A "steal" sound was added and it was getting obscured by the bump. -AP 10/3/95
		//	Changed by MK to make this sound unless the robot stole.
		if ((!steal_attempt) && !Robot_info[get_robot_id(robot)].energy_drain)
#endif
			digi_link_sound_to_pos(SOUND_ROBOT_HIT_PLAYER, player_segp, sidenum_t::WLEFT, collision_point, 0, F1_0);

		if (collision_seg != segment_none)
			object_create_explosion_without_damage(Vclip, collision_seg, collision_point, Weapon_info[0].impact_size, Weapon_info[0].wall_hit_vclip);
	}

	bump_two_objects(Robot_info, robot, playerobj, 1);
}

}

// Provide a way for network message to instantly destroy the control center
// without awarding points or anything.

//	if controlcen == NULL, that means don't do the explosion because the control center
//	was actually in another object.
void net_destroy_controlcen_object(const d_robot_info_array &Robot_info, const imobjptridx_t controlcen)
{
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;
	if (LevelUniqueControlCenterState.Control_center_destroyed != 1)
	{
		do_controlcen_destroyed_stuff(controlcen);

		if ((controlcen != object_none) && !(controlcen->flags&(OF_EXPLODING|OF_DESTROYED))) {
			digi_link_sound_to_pos(SOUND_CONTROL_CENTER_DESTROYED, vmsegptridx(controlcen->segnum), sidenum_t::WLEFT, controlcen->pos, 0, F1_0);
			explode_object(LevelUniqueObjectState, Robot_info, LevelSharedSegmentState, LevelUniqueSegmentState, controlcen, 0);
		}
	}
}

//	-----------------------------------------------------------------------------
void apply_damage_to_controlcen(const d_robot_info_array &Robot_info, const vmobjptridx_t controlcen, fix damage, const object &who)
{
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;
	int	whotype;

	//	Only allow a player to damage the control center.
	whotype = who.type;
	if (whotype != OBJ_PLAYER) {
		return;
	}

	if ((Game_mode & GM_MULTI) &&
		!(Game_mode & GM_MULTI_COOP))
	{
		auto &player = get_local_player();
		const auto t = i2f(player.hours_level * 3600) + player.time_level;
		if (t < Netgame.control_invul_time)
		{
		if (get_player_id(who) == Player_num) {
			const auto r = f2i(Netgame.control_invul_time - t);
			HUD_init_message(HM_DEFAULT, "%s %d:%02d.", TXT_CNTRLCEN_INVUL, r / 60, r % 60);
		}
		return;
		}
	}

	if (get_player_id(who) == Player_num) {
		LevelUniqueControlCenterState.Control_center_been_hit = 1;
		ai_do_cloak_stuff();
	}

	if ( controlcen->shields >= 0 )
		controlcen->shields -= damage;

	if ( (controlcen->shields < 0) && !(controlcen->flags&(OF_EXPLODING|OF_DESTROYED)) ) {
		do_controlcen_destroyed_stuff(controlcen);

		if (Game_mode & GM_MULTI) {
			if (get_player_id(who) == Player_num)
				add_points_to_score(ConsoleObject->ctype.player_info, CONTROL_CEN_SCORE, Game_mode);
			multi_send_destroy_controlcen(controlcen, get_player_id(who) );
		}
		else
			add_points_to_score(ConsoleObject->ctype.player_info, CONTROL_CEN_SCORE, Game_mode);

		digi_link_sound_to_pos(SOUND_CONTROL_CENTER_DESTROYED, vmsegptridx(controlcen->segnum), sidenum_t::WLEFT, controlcen->pos, 0, F1_0);

		explode_object(LevelUniqueObjectState, Robot_info, LevelSharedSegmentState, LevelUniqueSegmentState, controlcen, 0);
	}
}

namespace {

static void collide_player_and_controlcen(const d_robot_info_array &Robot_info, const vmobjptridx_t playerobj, const vmobjptridx_t controlcen, const vms_vector &collision_point)
{
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;
	if (get_player_id(playerobj) == Player_num) {
		LevelUniqueControlCenterState.Control_center_been_hit = 1;
		ai_do_cloak_stuff();				//	In case player cloaked, make control center know where he is.
	}

	if (check_collision_delayfunc_exec())
		digi_link_sound_to_pos(SOUND_ROBOT_HIT_PLAYER, vmsegptridx(playerobj->segnum), sidenum_t::WLEFT, collision_point, 0, F1_0);

	bump_two_objects(Robot_info, controlcen, playerobj, 1);
}

#if defined(DXX_BUILD_DESCENT_II)
static void collide_player_and_marker(const d_robot_info_array &, const object_base &playerobj, const vmobjptridx_t marker, const vms_vector &)
{
	if (get_player_id(playerobj)==Player_num) {
		int drawn;

		const auto marker_id = get_marker_id(marker);
		auto &msg = MarkerState.message[marker_id];
		auto &p = msg[0u];
		if (Game_mode & GM_MULTI)
		{
			const auto marker_owner = get_marker_owner(Game_mode, marker_id, Netgame.max_numplayers);
			drawn = HUD_init_message(HM_DEFAULT|HM_MAYDUPL, "MARKER %s: %s", vcplayerptr(marker_owner)->callsign.operator const char *(), &p);
		}
		else
		{
			const auto displayed_marker_id = static_cast<unsigned>(marker_id) + 1;
			if (p)
				drawn = HUD_init_message(HM_DEFAULT|HM_MAYDUPL, "MARKER %d: %s", displayed_marker_id, &p);
			else
				drawn = HUD_init_message(HM_DEFAULT|HM_MAYDUPL, "MARKER %d", displayed_marker_id);
	   }

		if (drawn)
			digi_play_sample( SOUND_MARKER_HIT, F1_0 );

		detect_escort_goal_accomplished(marker);
   }
}
#endif

//	If a persistent weapon and other object is not a weapon, weaken it, else kill it.
//	If both objects are weapons, weaken the weapon.
static void maybe_kill_weapon(object_base &weapon, const object_base &other_obj)
{
	if (is_proximity_bomb_or_player_smart_mine_or_placed_mine(get_weapon_id(weapon))) {
		weapon.flags |= OF_SHOULD_BE_DEAD;
		return;
	}

#if defined(DXX_BUILD_DESCENT_I)
	if (weapon.mtype.phys_info.flags & PF_PERSISTENT || other_obj.type == OBJ_WEAPON)
#elif defined(DXX_BUILD_DESCENT_II)
	//	Changed, 10/12/95, MK: Make weapon-weapon collisions always kill both weapons if not persistent.
	//	Reason: Otherwise you can't use proxbombs to detonate incoming homing missiles (or mega missiles).
	if (weapon.mtype.phys_info.flags & PF_PERSISTENT)
#endif
	{
		//	Weapons do a lot of damage to weapons, other objects do much less.
		if (!(weapon.mtype.phys_info.flags & PF_PERSISTENT)) {
			if (other_obj.type == OBJ_WEAPON)
				weapon.shields -= other_obj.shields/2;
			else
				weapon.shields -= other_obj.shields/4;

			if (weapon.shields <= 0) {
				weapon.shields = 0;
				weapon.flags |= OF_SHOULD_BE_DEAD;
			}
		}
	} else
		weapon.flags |= OF_SHOULD_BE_DEAD;

// -- 	if ((weapon->mtype.phys_info.flags & PF_PERSISTENT) || (other_obj->type == OBJ_WEAPON)) {
// -- 		//	Weapons do a lot of damage to weapons, other objects do much less.
// -- 		if (!(weapon->mtype.phys_info.flags & PF_PERSISTENT)) {
// -- 			if (other_obj->type == OBJ_WEAPON)
// -- 				weapon->shields -= other_obj->shields/2;
// -- 			else
// -- 				weapon->shields -= other_obj->shields/4;
// --
// -- 			if (weapon->shields <= 0) {
// -- 				weapon->shields = 0;
// -- 				weapon->flags |= OF_SHOULD_BE_DEAD;
// -- 			}
// -- 		}
// -- 	} else
// -- 		weapon->flags |= OF_SHOULD_BE_DEAD;
}

static void collide_weapon_and_controlcen(const d_robot_info_array &Robot_info, const vmobjptridx_t weapon, const vmobjptridx_t controlcen, vms_vector &collision_point)
{
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;

#if defined(DXX_BUILD_DESCENT_I)
	fix explosion_size = ((controlcen->size/3)*3)/4;
#elif defined(DXX_BUILD_DESCENT_II)
	if (get_weapon_id(weapon) == weapon_id_type::OMEGA_ID)
		if (!ok_to_do_omega_damage(weapon)) // see comment in laser.c
			return;

	fix explosion_size = controlcen->size*3/20;
#endif
	if (weapon->ctype.laser_info.parent_type == OBJ_PLAYER) {
		fix	damage = weapon->shields;

		/*
		* Check if persistent weapon already hit this object. If yes, abort.
		* If no, add this object to hitobj_list and do it's damage.
		*/
		if (weapon->mtype.phys_info.flags & PF_PERSISTENT)
		{
			damage = weapon->shields*2; // to not alter Gameplay too much, multiply damage by 2.
			if (weapon->ctype.laser_info.test_set_hitobj(controlcen))
				return;
		}

		if (get_player_id(vcobjptr(weapon->ctype.laser_info.parent_num)) == Player_num)
			LevelUniqueControlCenterState.Control_center_been_hit = 1;

		if ( Weapon_info[get_weapon_id(weapon)].damage_radius )
		{
			const auto obj2weapon{vm_vec_sub(collision_point, controlcen->pos)};
			const auto mag = vm_vec_mag(obj2weapon);
			if(mag < controlcen->size && mag > 0) // FVI code does not necessarily update the collision point for object2object collisions. Do that now.
			{
				collision_point = vm_vec_scale_add(controlcen->pos, obj2weapon, fixdiv(controlcen->size, mag)); 
				weapon->pos = collision_point;
			}
#if defined(DXX_BUILD_DESCENT_I)
			explode_badass_weapon(Robot_info, weapon, weapon->pos);
#elif defined(DXX_BUILD_DESCENT_II)
			explode_badass_weapon(Robot_info, weapon, collision_point);
#endif
		}
		else
			object_create_explosion_without_damage(Vclip, vmsegptridx(controlcen->segnum), collision_point, explosion_size, vclip_index::small_explosion);

		digi_link_sound_to_pos(SOUND_CONTROL_CENTER_HIT, vmsegptridx(controlcen->segnum), sidenum_t::WLEFT, collision_point, 0, F1_0);

		damage = fixmul(damage, weapon->ctype.laser_info.multiplier);

		apply_damage_to_controlcen(Robot_info, controlcen, damage, vcobjptr(weapon->ctype.laser_info.parent_num));

		maybe_kill_weapon(weapon,controlcen);
	} else {	//	If robot weapon hits control center, blow it up, make it go away, but do no damage to control center.
		object_create_explosion_without_damage(Vclip, vmsegptridx(controlcen->segnum), collision_point, explosion_size, vclip_index::small_explosion);
		maybe_kill_weapon(weapon,controlcen);
	}
}

static void collide_weapon_and_clutter(const d_robot_info_array &Robot_info, object_base &weapon, const vmobjptridx_t clutter, const vms_vector &collision_point)
{
	if ( clutter->shields >= 0 )
		clutter->shields -= weapon.shields;

	digi_link_sound_to_pos(SOUND_LASER_HIT_CLUTTER, vmsegptridx(weapon.segnum), sidenum_t::WLEFT, collision_point, 0, F1_0);

	object_create_explosion_without_damage(Vclip, vmsegptridx(clutter->segnum), collision_point, ((clutter->size / 3) * 3) / 4, vclip_index::small_explosion);

	if ( (clutter->shields < 0) && !(clutter->flags&(OF_EXPLODING|OF_DESTROYED)))
		explode_object(LevelUniqueObjectState, Robot_info, LevelSharedSegmentState, LevelUniqueSegmentState, clutter, STANDARD_EXPL_DELAY);

	maybe_kill_weapon(weapon,clutter);
}

}

//--mk, 121094 -- extern void spin_robot(object *robot, vms_vector *collision_point);

#if defined(DXX_BUILD_DESCENT_II)
//	------------------------------------------------------------------------------------------------------
window_event_result do_final_boss_frame(void)
{
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;
	auto Final_boss_countdown_time = GameUniqueState.Final_boss_countdown_time;
	if (!Final_boss_countdown_time)
		return window_event_result::ignored;

	if (!LevelUniqueControlCenterState.Control_center_destroyed)
		return window_event_result::ignored;

	Final_boss_countdown_time -= FrameTime;
	if (Final_boss_countdown_time > 0)
	{
		if (Final_boss_countdown_time <= F1_0*2)
		{
			int flash_value = f2i(((F1_0*2)-Final_boss_countdown_time)*16); // during final 2 seconds of final boss countdown (as set by do_final_boss_hacks()), make a flash value that maxes out over that time
			PALETTE_FLASH_SET(-flash_value,-flash_value,-flash_value); // set palette to inverted flash_value to fade to black
		}
		GameUniqueState.Final_boss_countdown_time = Final_boss_countdown_time;
		return window_event_result::ignored;
	}
	/* Ensure that GameUniqueState.Final_boss_countdown_time is not 0,
	 * so that the flag remains raised.  If Final_boss_countdown_time
	 * were greater than 0, the function already returned.  If
	 * Final_boss_countdown_time is exactly 0, write -1.  Otherwise,
	 * write a slightly more negative number.  If
	 * Final_boss_countdown_time is INT32_MIN, this would roll over, but
	 * that would only happen if FrameTime had an absurd value.
	 */
	GameUniqueState.Final_boss_countdown_time = --Final_boss_countdown_time;

	return start_endlevel_sequence();		//pretend we hit the exit trigger
}

//	------------------------------------------------------------------------------------------------------
//	This is all the ugly stuff we do when you kill the final boss so that you don't die or something
//	which would ruin the logic of the cut sequence.
void do_final_boss_hacks(void)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	if (Player_dead_state != player_dead_state::no)
	{
		Int3();		//	Uh-oh, player is dead.  Try to rescue him.
		Player_dead_state = player_dead_state::no;
	}

	auto &plrobj = get_local_plrobj();
	auto &player_info = plrobj.ctype.player_info;
	{
		auto &shields = plrobj.shields;
		if (shields <= 0)
			shields = 1;
	}

	//	If you're not invulnerable, get invulnerable!
	auto &pl_flags = player_info.powerup_flags;
	if (!(pl_flags & PLAYER_FLAGS_INVULNERABLE)) {
		pl_flags |= PLAYER_FLAGS_INVULNERABLE;
		player_info.FakingInvul = 0;
		player_info.invulnerable_time = {GameTime64};
	}
	if (!(Game_mode & GM_MULTI))
		buddy_message("Nice job, %s!", static_cast<const char *>(get_local_player().callsign));

	GameUniqueState.Final_boss_countdown_time = F1_0*4; // was F1_0*2 originally. extended to F1_0*4 to play fade to black which happened after countdown ended in the original game.
}
#endif

//	------------------------------------------------------------------------------------------------------
//	Return 1 if robot died, else return 0
int apply_damage_to_robot(const d_robot_info_array &Robot_info, const vmobjptridx_t robot, fix damage, objnum_t killer_objnum)
{
	if ( robot->flags&OF_EXPLODING) return 0;

	if (robot->shields < 0 ) return 0;	//robot already dead...

	auto &robptr = Robot_info[get_robot_id(robot)];
#if defined(DXX_BUILD_DESCENT_II)
	auto &BossUniqueState = LevelUniqueObjectState.BossState;
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	if (robptr.boss_flag != boss_robot_id::None)
		BossUniqueState.Boss_hit_time = {GameTime64};

	//	Buddy invulnerable on level 24 so he can give you his important messages.  Bah.
	//	Also invulnerable if his cheat for firing weapons is in effect.
	if (robot_is_companion(robptr)) {
		if (PLAYING_BUILTIN_MISSION && Current_level_num == Current_mission->last_level)
			return 0;
	}
#endif

	robot->shields -= damage;

#if defined(DXX_BUILD_DESCENT_II)
	//	Do unspeakable hacks to make sure player doesn't die after killing boss.  Or before, sort of.
	if (robptr.boss_flag != boss_robot_id::None)
		if (PLAYING_BUILTIN_MISSION && Current_level_num == Current_mission->last_level)
			if (robot->shields < 0)
			 {
				if (Game_mode & GM_MULTI)
				  {
					 if (!multi_all_players_alive(Objects.vcptr, partial_range(Players, N_players))) // everyones gotta be alive
					   robot->shields=1;
					 else
					  {
					    multi_send_finish_game();
					    do_final_boss_hacks();
					  }

					}
				else
				  {	// NOTE LINK TO ABOVE!!!
					if (get_local_plrobj().shields < 0 || Player_dead_state != player_dead_state::no)
						robot->shields = 1;		//	Sorry, we can't allow you to kill the final boss after you've died.  Rough luck.
					else
						do_final_boss_hacks();
				  }
			  }
#endif

	if (robot->shields < 0) {
		auto &plr = get_local_player();
		plr.num_kills_level++;
		plr.num_kills_total++;
		if (Game_mode & GM_MULTI) {
			if (multi_explode_robot_sub(Robot_info, robot))
			{
				multi_send_robot_explode(robot, killer_objnum);
				return 1;
			}
			else
				return 0;
		}

		if (robptr.boss_flag != boss_robot_id::None)
		{
			start_boss_death_sequence(LevelUniqueObjectState.BossState, Robot_info, robot);
		}
#if defined(DXX_BUILD_DESCENT_II)
		else if (robptr.death_roll) {
			start_robot_death_sequence(robot);	//do_controlcen_destroyed_stuff(NULL);
		}
#endif
		else {
#if defined(DXX_BUILD_DESCENT_II)
			if (get_robot_id(robot) == robot_id::special_reactor)
				special_reactor_stuff();
#endif
			// Kamikaze, explode right away, IN YOUR FACE!
			explode_object(LevelUniqueObjectState, Robot_info, LevelSharedSegmentState, LevelUniqueSegmentState, robot,
#if defined(DXX_BUILD_DESCENT_II)
						   robptr.kamikaze ? 1 :
#endif
						   STANDARD_EXPL_DELAY);
		}
		return 1;
	} else
		return 0;
}

namespace {

#if defined(DXX_BUILD_DESCENT_II)
enum class boss_weapon_collision_result : uint8_t
{
	normal,
	invulnerable,
	reflect,	// implies invulnerable
};

static inline int Boss_invulnerable_dot()
{
	return F1_0 / 4 - i2f(underlying_value(GameUniqueState.Difficulty_level)) / 8;
}

//	------------------------------------------------------------------------------------------------------
//	Return true if damage done to boss, else return false.
static boss_weapon_collision_result do_boss_weapon_collision(const d_robot_info_array &Robot_info, const d_level_shared_segment_state &LevelSharedSegmentState, d_level_unique_segment_state &LevelUniqueSegmentState, const vcobjptridx_t robotptridx, object &weapon, const vms_vector &collision_point)
{
	auto &BossUniqueState = LevelUniqueObjectState.BossState;
	auto &BuddyState = LevelUniqueObjectState.BuddyState;
	const object_base &robot = robotptridx;
	const auto d2_boss_index = build_boss_robot_index_from_boss_robot_id(Robot_info[get_robot_id(robot)].boss_flag);
	assert(Boss_spew_more.valid_index(d2_boss_index));

	//	See if should spew a bot.
	if (weapon.ctype.laser_info.parent_type == OBJ_PLAYER && weapon.ctype.laser_info.parent_num == get_local_player().objnum)
		if ((Weapon_info[get_weapon_id(weapon)].matter != weapon_info::matter_flag::energy
			? Boss_spews_bots_matter
			: Boss_spews_bots_energy)[d2_boss_index])
		{
			if (Boss_spew_more[d2_boss_index])
				if (d_rand() > 16384) {
					const auto &&spew = boss_spew_robot(Robot_info, robot, collision_point);
					if (spew != object_none)
					{
						BossUniqueState.Last_gate_time = GameTime64 - GameUniqueState.Boss_gate_interval - 1;	//	Force allowing spew of another bot.
						multi_send_boss_create_robot(robotptridx, spew);
					}
				}
			const auto &&spew = boss_spew_robot(Robot_info, robot, collision_point);
			if (spew != object_none)
				multi_send_boss_create_robot(robotptridx, spew);
		}

	if (Boss_invulnerable_spot[d2_boss_index]) {
		fix			dot;
		//	Boss only vulnerable in back.  See if hit there.
		//	Note, if BOSS_INVULNERABLE_DOT is close to F1_0 (in magnitude), then should probably use non-quick version.
		const auto tvec1 = vm_vec_normalized_quick(vm_vec_sub(collision_point, robot.pos));
		dot = vm_vec_dot(tvec1, robot.orient.fvec);
		if (dot > Boss_invulnerable_dot()) {
			if (const auto &&segp = find_point_seg(LevelSharedSegmentState, LevelUniqueSegmentState, collision_point, Segments.vmptridx(robot.segnum)))
				digi_link_sound_to_pos(SOUND_WEAPON_HIT_DOOR, segp, sidenum_t::WLEFT, collision_point, 0, F1_0);
			if (BuddyState.Buddy_objnum != object_none)
			{
				if (BuddyState.Buddy_gave_hint_count) {
					if (BuddyState.Last_time_buddy_gave_hint + F1_0*20 < GameTime64) {
						int	sval;

						BuddyState.Buddy_gave_hint_count--;
						BuddyState.Last_time_buddy_gave_hint = {GameTime64};
						sval = (d_rand()*4) >> 15;
						const char *msg;
						switch (sval) {
							case 0:
								msg = "Hit him in the back!";
								break;
							case 1:
								msg = "He's invulnerable there!";
								break;
							case 2:
								msg = "Get behind him and fire!";
								break;
							case 3:
							default:
								msg = "Hit the glowing spot!";
								break;
						}
						buddy_message_str(msg);
					}
				}
			}

			//	Cause weapon to bounce.
			if (Weapon_info[get_weapon_id(weapon)].matter == weapon_info::matter_flag::energy)
			{
					auto vec_to_point = vm_vec_normalized_quick(vm_vec_sub(collision_point, robot.pos));
					const auto &&[speed, weap_vec] = vm_vec_normalize_quick_with_magnitude(weapon.mtype.phys_info.velocity);
					vm_vec_scale_add2(vec_to_point, weap_vec, -F1_0*2);
					vm_vec_scale(vec_to_point, speed/4);
					weapon.mtype.phys_info.velocity = vec_to_point;
				return boss_weapon_collision_result::reflect;
			}
			return boss_weapon_collision_result::invulnerable;
		}
	}
	else if ((Weapon_info[get_weapon_id(weapon)].matter != weapon_info::matter_flag::energy
			? Boss_invulnerable_matter
			: Boss_invulnerable_energy)[d2_boss_index])
	{
		if (const auto &&segp = find_point_seg(LevelSharedSegmentState, LevelUniqueSegmentState, collision_point, Segments.vmptridx(robot.segnum)))
			digi_link_sound_to_pos(SOUND_WEAPON_HIT_DOOR, segp, sidenum_t::WLEFT, collision_point, 0, F1_0);
		return boss_weapon_collision_result::invulnerable;
	}
	return boss_weapon_collision_result::normal;
}
#endif
//	------------------------------------------------------------------------------------------------------
static void collide_robot_and_weapon(const d_robot_info_array &Robot_info, const vmobjptridx_t robot, const vmobjptridx_t weapon, vms_vector &collision_point)
{
	auto &BossUniqueState = LevelUniqueObjectState.BossState;
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
#if defined(DXX_BUILD_DESCENT_II)
	auto &icobjptridx = Objects.icptridx;
	if (get_weapon_id(weapon) == weapon_id_type::OMEGA_ID)
		if (!ok_to_do_omega_damage(weapon)) // see comment in laser.c
			return;
#endif

	auto &robptr = Robot_info[get_robot_id(robot)];
	if (robptr.boss_flag != boss_robot_id::None)
	{
		BossUniqueState.Boss_hit_this_frame = 1;
#if defined(DXX_BUILD_DESCENT_II)
		BossUniqueState.Boss_hit_time = {GameTime64};
#endif
	}
#if defined(DXX_BUILD_DESCENT_II)
	const boss_weapon_collision_result damage_flag = Boss_invulnerable_matter.valid_index(build_boss_robot_index_from_boss_robot_id(robptr.boss_flag))
		? do_boss_weapon_collision(Robot_info, LevelSharedSegmentState, LevelUniqueSegmentState, robot, weapon, collision_point)
		: boss_weapon_collision_result::normal;
#endif

#if defined(DXX_BUILD_DESCENT_II)
	//	Put in at request of Jasen (and Adam) because the Buddy-Bot gets in their way.
	//	MK has so much fun whacking his butt around the mine he never cared...
	if ((robot_is_companion(robptr)) && ((weapon->ctype.laser_info.parent_type != OBJ_ROBOT) && !cheats.robotskillrobots))
		return;

	if (get_weapon_id(weapon) == weapon_id_type::EARTHSHAKER_ID)
		smega_rock_stuff();
#endif

	/*
	 * Check if persistent weapon already hit this object. If yes, abort.
	 * If no, add this object to hitobj_list and do it's damage.
	 */
	if (weapon->mtype.phys_info.flags & PF_PERSISTENT)
	{
		if (weapon->ctype.laser_info.test_set_hitobj(robot))
			return;
	}

	if (laser_parent_is_object(weapon->ctype.laser_info, robot))
		return;

#if defined(DXX_BUILD_DESCENT_II)
	const auto Difficulty_level = GameUniqueState.Difficulty_level;
	//	Changed, 10/04/95, put out blobs based on skill level and power of weapon doing damage.
	//	Also, only a weapon hit from a player weapon causes smart blobs.
	if ((weapon->ctype.laser_info.parent_type == OBJ_PLAYER) && (robptr.energy_blobs))
		if ((robot->shields > 0) && Weapon_is_energy[get_weapon_id(weapon)]) {
			fix	probval;
			int	num_blobs;

			probval = (underlying_value(Difficulty_level) + 2) * min(weapon->shields, robot->shields);
			probval = robptr.energy_blobs * probval/(NDL*32);

			num_blobs = probval >> 16;
			if (2*d_rand() < (probval & 0xffff))
				num_blobs++;

			if (num_blobs)
				create_robot_smart_children(robot, num_blobs);
		}

	//	Note: If weapon hits an invulnerable boss, it will still do badass damage, including to the boss,
	//	unless this is trapped elsewhere.
#endif
	weapon_info *wi = &Weapon_info[get_weapon_id(weapon)];
	if ( wi->damage_radius )
	{
		const auto obj2weapon{vm_vec_sub(collision_point, robot->pos)};
		const auto mag = vm_vec_mag(obj2weapon);
		if(mag < robot->size && mag > 0) // FVI code does not necessarily update the collision point for object2object collisions. Do that now.
		{
			collision_point = vm_vec_scale_add(robot->pos, obj2weapon, fixdiv(robot->size, mag)); 
			weapon->pos = collision_point;
		}
#if defined(DXX_BUILD_DESCENT_I)
		explode_badass_weapon(Robot_info, weapon, weapon->pos);
#elif defined(DXX_BUILD_DESCENT_II)
		if (damage_flag != boss_weapon_collision_result::normal) {			//don't make badass sound

			//this code copied from explode_badass_weapon()

			object_create_badass_explosion(Robot_info, weapon, vmsegptridx(weapon->segnum), collision_point,
							wi->impact_size,
							wi->robot_hit_vclip,
							wi->strength[Difficulty_level],
							wi->damage_radius,wi->strength[Difficulty_level],
							icobjptridx(weapon->ctype.laser_info.parent_num));

		}
		else		//normal badass explosion
			explode_badass_weapon(Robot_info, weapon, collision_point);
#endif
	}

#if defined(DXX_BUILD_DESCENT_I)
	if ( (weapon->ctype.laser_info.parent_type==OBJ_PLAYER) && !(robot->flags & OF_EXPLODING) )
#elif defined(DXX_BUILD_DESCENT_II)
	if ( ((weapon->ctype.laser_info.parent_type==OBJ_PLAYER) || cheats.robotskillrobots) && !(robot->flags & OF_EXPLODING) )
#endif
	{
		if (weapon->ctype.laser_info.parent_num == get_local_player().objnum) {
			create_awareness_event(weapon, player_awareness_type_t::PA_WEAPON_ROBOT_COLLISION, LevelUniqueRobotAwarenessState);			// object "weapon" can attract attention to player
			do_ai_robot_hit(robot, robptr, player_awareness_type_t::PA_WEAPON_ROBOT_COLLISION);
		}
	       	else
			multi_robot_request_change(robot, get_player_id(vcobjptr(weapon->ctype.laser_info.parent_num)));

		std::pair<fix, vclip_index> explosion_size_and_vclip;
		if ((Vclip.valid_index(robptr.exp1_vclip_num) && (explosion_size_and_vclip = {(robot->size / 2 * 3) / 4, robptr.exp1_vclip_num}, true))
#if defined(DXX_BUILD_DESCENT_II)
			|| (Vclip.valid_index(wi->robot_hit_vclip) && (explosion_size_and_vclip = {wi->impact_size, wi->robot_hit_vclip}, true))
#endif
			)
		{
			const auto &&expl_obj = object_create_explosion_without_damage(Vclip, vmsegptridx(weapon->segnum), collision_point, explosion_size_and_vclip.first, explosion_size_and_vclip.second);

		if (expl_obj != object_none)
				obj_attach(Objects, robot, expl_obj);
		}

#if defined(DXX_BUILD_DESCENT_II)
		if (damage_flag == boss_weapon_collision_result::normal)
#endif
		{
			if (robptr.exp1_sound_num > -1)
				digi_link_sound_to_pos(robptr.exp1_sound_num, vcsegptridx(robot->segnum), sidenum_t::WLEFT, collision_point, 0, F1_0);
			fix	damage = weapon->shields;

				damage = fixmul(damage, weapon->ctype.laser_info.multiplier);

#if defined(DXX_BUILD_DESCENT_II)
			//	Cut Gauss damage on bosses because it just breaks the game.  Bosses are so easy to
			//	hit, and missing a robot is what prevents the Gauss from being game-breaking.
			if (get_weapon_id(weapon) == weapon_id_type::GAUSS_ID)
				if (robptr.boss_flag != boss_robot_id::None)
					damage = damage * (2 * NDL - underlying_value(Difficulty_level)) / (2 * NDL);
#endif

			if (!apply_damage_to_robot(Robot_info, robot, damage, weapon->ctype.laser_info.parent_num))
				bump_two_objects(Robot_info, robot, weapon, 0);		//only bump if not dead. no damage from bump
			else if (laser_parent_is_player(vcobjptr, weapon->ctype.laser_info, *ConsoleObject))
			{
				add_points_to_score(ConsoleObject->ctype.player_info, robptr.score_value, Game_mode);
				detect_escort_goal_accomplished(robot);
			}
		}

#if defined(DXX_BUILD_DESCENT_II)
		//	If Gauss Cannon, spin robot.
		if (!robot_is_companion(robptr) && robptr.boss_flag == boss_robot_id::None && (get_weapon_id(weapon) == weapon_id_type::GAUSS_ID))
		{
			ai_static	*aip = &robot->ctype.ai_info;

			if (aip->SKIP_AI_COUNT * FrameTime < F1_0) {
				aip->SKIP_AI_COUNT++;
				robot->mtype.phys_info.rotthrust.x = fixmul((d_rand() - 16384), FrameTime * aip->SKIP_AI_COUNT);
				robot->mtype.phys_info.rotthrust.y = fixmul((d_rand() - 16384), FrameTime * aip->SKIP_AI_COUNT);
				robot->mtype.phys_info.rotthrust.z = fixmul((d_rand() - 16384), FrameTime * aip->SKIP_AI_COUNT);
				robot->mtype.phys_info.flags |= PF_USES_THRUST;

			}
		}
#endif

	}

#if defined(DXX_BUILD_DESCENT_II)
	if (damage_flag != boss_weapon_collision_result::reflect)
#endif
	maybe_kill_weapon(weapon,robot);

	return;
}

static void collide_hostage_and_player(const d_robot_info_array &, const vmobjptridx_t hostage, object &player, const vms_vector &)
{
	// Give player points, etc.
	if (&player == ConsoleObject)
	{
		detect_escort_goal_accomplished(hostage);
		add_points_to_score(player.ctype.player_info, HOSTAGE_SCORE, Game_mode);

		// Do effect
		hostage_rescue();

		// Remove the hostage object.
		hostage->flags |= OF_SHOULD_BE_DEAD;

		if (Game_mode & GM_MULTI)
			multi_send_remobj(hostage);
	}
	return;
}

static void collide_player_and_player(const d_robot_info_array &Robot_info, const vmobjptridx_t player1, const vmobjptridx_t player2, const vms_vector &collision_point)
{
	int damage_flag = 1;

	if (check_collision_delayfunc_exec())
		digi_link_sound_to_pos(SOUND_ROBOT_HIT_PLAYER, vcsegptridx(player1->segnum), sidenum_t::WLEFT, collision_point, 0, F1_0);

	// Multi is special - as always. Clients do the bump damage locally but the remote players do their collision result (change velocity) on their own. So after our initial collision, ignore further collision damage till remote players (hopefully) react.
	if (Game_mode & GM_MULTI)
	{
		damage_flag = 0;
		if (!(get_player_id(player1) == Player_num || get_player_id(player2) == Player_num))
			return;
		auto &last_player_bump = ((get_player_id(player1) == Player_num)
			? player2
			: player1)->ctype.player_info.Last_bumped_local_player;
		if (last_player_bump + (F1_0/Netgame.PacketsPerSec) < GameTime64 || last_player_bump > GameTime64)
		{
			last_player_bump = {GameTime64};
			damage_flag = 1;
		}
	}
	bump_two_objects(Robot_info, player1, player2, damage_flag);
}

static imobjptridx_t maybe_drop_primary_weapon_egg(const object &playerobj, const primary_weapon_index_t weapon_index)
{
	int weapon_flag = HAS_PRIMARY_FLAG(weapon_index);
	const auto powerup_num = Primary_weapon_to_powerup[weapon_index];
	auto &player_info = playerobj.ctype.player_info;
	if (player_info.primary_weapon_flags & weapon_flag)
		return call_object_create_egg(playerobj, powerup_num);
	else
		return object_none;
}

static void maybe_drop_secondary_weapon_egg(const object_base &playerobj, const secondary_weapon_index_t weapon_index, const unsigned count)
{
	const auto powerup_num = Secondary_weapon_to_powerup[weapon_index];
	call_object_create_egg(playerobj, std::min(count, 3u), powerup_num);
}

static void maybe_drop_primary_weapon_with_adjustment(const object &playerobj, const primary_weapon_index_t weapon_index, auto adjust)
{
	if (const auto objp{maybe_drop_primary_weapon_egg(playerobj, weapon_index)}; objp != object_none)
		adjust(*objp);
}

static void maybe_drop_primary_vulcan_based_weapon(const object &playerobj, const primary_weapon_index_t weapon_index, const uint16_t ammo)
{
	maybe_drop_primary_weapon_with_adjustment(playerobj, weapon_index,
		[ammo](object &weapon) {
		//make sure gun has at least as much as a powerup
			weapon.ctype.powerup_info.count = std::max(ammo, VULCAN_AMMO_AMOUNT);
		}
	);
}

static void maybe_drop_primary_vulcan_weapons(const object &playerobj)
{
	auto &player_info = playerobj.ctype.player_info;
	const auto has_weapons{player_info.primary_weapon_flags & HAS_VULCAN_AND_GAUSS_FLAGS};
	if (!has_weapons)
		return;
	const auto total_vulcan_ammo = player_info.vulcan_ammo;
	auto vulcan_ammo = total_vulcan_ammo;
#if defined(DXX_BUILD_DESCENT_II)
	auto gauss_ammo = vulcan_ammo;
	if (has_weapons == HAS_VULCAN_AND_GAUSS_FLAGS)
	{
		//if both vulcan & gauss, each gets half
		vulcan_ammo /= 2;
		gauss_ammo -= vulcan_ammo;
	}
#endif
	maybe_drop_primary_vulcan_based_weapon(playerobj, primary_weapon_index_t::VULCAN_INDEX, vulcan_ammo);
#if defined(DXX_BUILD_DESCENT_II)
	maybe_drop_primary_vulcan_based_weapon(playerobj, primary_weapon_index_t::GAUSS_INDEX, gauss_ammo);
#endif
}

#if defined(DXX_BUILD_DESCENT_II)
static void maybe_drop_primary_omega_weapon(const object &playerobj)
{
	maybe_drop_primary_weapon_with_adjustment(playerobj, primary_weapon_index_t::OMEGA_INDEX,
		[&playerobj](object &weapon) {
			weapon.ctype.powerup_info.count = (get_player_id(playerobj) == Player_num) ? playerobj.ctype.player_info.Omega_charge : MAX_OMEGA_CHARGE;
		}
	);
}
#endif

static void drop_missile_1_or_4(const object &playerobj, const secondary_weapon_index_t missile_index)
{
	unsigned num_missiles = playerobj.ctype.player_info.secondary_ammo[missile_index];
	const auto powerup_id = Secondary_weapon_to_powerup[missile_index];

	if (num_missiles > 10)
		num_missiles = 10;

	call_object_create_egg(playerobj, num_missiles / 4, static_cast<powerup_type_t>(underlying_value(powerup_id) + 1));
	call_object_create_egg(playerobj, num_missiles % 4, powerup_id);
}

}

void drop_player_eggs(const vmobjptridx_t playerobj)
{
	if ((playerobj->type == OBJ_PLAYER) || (playerobj->type == OBJ_GHOST)) {
		// Seed the random number generator so in net play the eggs will always
		// drop the same way
		if (Game_mode & GM_MULTI)
		{
			Net_create_loc = 0;
		}
		auto &player_info = playerobj->ctype.player_info;
		auto &plr_laser_level = player_info.laser_level;
		if (const auto GrantedItems = (Game_mode & GM_MULTI) ? (d_srand(5483L), Netgame.SpawnGrantedItems) : netgrant_flag::None)
		{
			if (const auto granted_laser_level = map_granted_flags_to_laser_level(GrantedItems); granted_laser_level != laser_level::_1)
			{
				if (plr_laser_level <= granted_laser_level)
					/* All levels were from grant */
					plr_laser_level = laser_level::_1;
#if defined(DXX_BUILD_DESCENT_II)
				else if (granted_laser_level > MAX_LASER_LEVEL)
				{
					/* Grant gives super laser 5.
					 * Player has super laser 6.
					 */
					-- plr_laser_level;
				}
				else if (plr_laser_level > MAX_LASER_LEVEL)
				{
					/* Grant gives only regular lasers.
					 * Player has super lasers, will drop only
					 * super lasers.
					 */
				}
#endif
				else
					plr_laser_level -= granted_laser_level;
			}
			if (uint16_t subtract_vulcan_ammo = map_granted_flags_to_vulcan_ammo(GrantedItems))
			{
				auto &v = playerobj->ctype.player_info.vulcan_ammo;
				if (v < subtract_vulcan_ammo)
					v = 0;
				else
					v -= subtract_vulcan_ammo;
			}
			player_info.powerup_flags &= ~map_granted_flags_to_player_flags(GrantedItems);
			player_info.primary_weapon_flags &= ~map_granted_flags_to_primary_weapon_flags(GrantedItems);
		}

		auto &secondary_ammo = playerobj->ctype.player_info.secondary_ammo;
#if defined(DXX_BUILD_DESCENT_II)
		//	If the player had smart mines, maybe arm one of them.
		const auto drop_armed_bomb = [&](uint8_t mines, weapon_id_type id) {
			mines %= 4;
			for (int rthresh = 30000; mines && d_rand() < rthresh; rthresh /= 2)
		{
			const auto randvec = make_random_vector();
			const auto tvec = vm_vec_add(playerobj->pos, randvec);
			const auto &&newseg = find_point_seg(LevelSharedSegmentState, LevelUniqueSegmentState, tvec, Segments.vmptridx(playerobj->segnum));
			if (newseg != segment_none)
			{
				-- mines;
				/* As a special case, decrease the count of mines regardless of
				 * whether the object creation succeeds.  These mines will be
				 * lost at the end of the death sequence if they are not
				 * dropped here, so there is no need to count them accurately.
				 */
				Laser_create_new(randvec, tvec, newseg, playerobj, id, weapon_sound_flag::silent);
			}
		}
		};
		drop_armed_bomb(secondary_ammo[secondary_weapon_index_t::SMART_MINE_INDEX], weapon_id_type::SUPERPROX_ID);

		//	If the player had proximity bombs, maybe arm one of them.
		if (Game_mode & GM_MULTI)
			drop_armed_bomb(secondary_ammo[secondary_weapon_index_t::PROXIMITY_INDEX], weapon_id_type::PROXIMITY_ID);
#endif

		//	If the player dies and he has powerful lasers, create the powerups here.

		if (plr_laser_level != laser_level::_1)
		{
			auto &&[laser_count, laser_powerup] =
#if defined(DXX_BUILD_DESCENT_II)
				(plr_laser_level > MAX_LASER_LEVEL)
				? std::tuple(static_cast<unsigned>(plr_laser_level) - static_cast<unsigned>(MAX_LASER_LEVEL), powerup_type_t::POW_SUPER_LASER)
				:
#endif
				std::tuple(static_cast<unsigned>(plr_laser_level), powerup_type_t::POW_LASER);
			call_object_create_egg(playerobj, laser_count, laser_powerup);
		}

		//	Drop quad laser if appropos
		if (player_info.powerup_flags & PLAYER_FLAGS_QUAD_LASERS)
			call_object_create_egg(playerobj, powerup_type_t::POW_QUAD_FIRE);

		if (player_info.powerup_flags & PLAYER_FLAGS_CLOAKED)
			call_object_create_egg(playerobj, powerup_type_t::POW_CLOAK);

#if defined(DXX_BUILD_DESCENT_II)
		if (player_info.powerup_flags & PLAYER_FLAGS_MAP_ALL)
			call_object_create_egg(playerobj, powerup_type_t::POW_FULL_MAP);

		if (player_info.powerup_flags & PLAYER_FLAGS_AFTERBURNER)
			call_object_create_egg(playerobj, powerup_type_t::POW_AFTERBURNER);

		if (player_info.powerup_flags & PLAYER_FLAGS_AMMO_RACK)
			call_object_create_egg(playerobj, powerup_type_t::POW_AMMO_RACK);

		if (player_info.powerup_flags & PLAYER_FLAGS_CONVERTER)
			call_object_create_egg(playerobj, powerup_type_t::POW_CONVERTER);

		if (player_info.powerup_flags & PLAYER_FLAGS_HEADLIGHT)
			call_object_create_egg(playerobj, powerup_type_t::POW_HEADLIGHT);

		// drop the other enemies flag if you have it

		if (game_mode_capture_flag(Game_mode) && (player_info.powerup_flags & PLAYER_FLAGS_FLAG))
		{
			call_object_create_egg(playerobj, get_team(get_player_id(playerobj)) == team_number::blue ? powerup_type_t::POW_FLAG_RED : powerup_type_t::POW_FLAG_BLUE);
		}


		if (game_mode_hoard(Game_mode))
		{
			// Drop hoard orbs
			call_object_create_egg(playerobj, std::min(player_info.hoard.orbs, player_info.max_hoard_orbs), powerup_type_t::POW_HOARD_ORB);
		}
#endif

		/* Drop the Vulcan Cannon and Gauss Cannon, if the player has them.
		 */
		maybe_drop_primary_vulcan_weapons(playerobj);

		//	Drop the rest of the primary weapons
		maybe_drop_primary_weapon_egg(playerobj, primary_weapon_index_t::SPREADFIRE_INDEX);
		maybe_drop_primary_weapon_egg(playerobj, primary_weapon_index_t::PLASMA_INDEX);
		maybe_drop_primary_weapon_egg(playerobj, primary_weapon_index_t::FUSION_INDEX);

#if defined(DXX_BUILD_DESCENT_II)
		maybe_drop_primary_weapon_egg(playerobj, primary_weapon_index_t::HELIX_INDEX);
		maybe_drop_primary_weapon_egg(playerobj, primary_weapon_index_t::PHOENIX_INDEX);
		maybe_drop_primary_omega_weapon(playerobj);
#endif

		//	Drop the secondary weapons
		//	Note, proximity weapon only comes in packets of 4.  So drop n/2, but a max of 3 (handled inside maybe_drop..)  Make sense?

		maybe_drop_secondary_weapon_egg(playerobj, secondary_weapon_index_t::PROXIMITY_INDEX, secondary_ammo[secondary_weapon_index_t::PROXIMITY_INDEX] / 4);

		maybe_drop_secondary_weapon_egg(playerobj, secondary_weapon_index_t::SMART_INDEX, secondary_ammo[secondary_weapon_index_t::SMART_INDEX]);
		maybe_drop_secondary_weapon_egg(playerobj, secondary_weapon_index_t::MEGA_INDEX, secondary_ammo[secondary_weapon_index_t::MEGA_INDEX]);

#if defined(DXX_BUILD_DESCENT_II)
		maybe_drop_secondary_weapon_egg(playerobj, secondary_weapon_index_t::SMART_MINE_INDEX, secondary_ammo[secondary_weapon_index_t::SMART_MINE_INDEX] / 4);
		maybe_drop_secondary_weapon_egg(playerobj, secondary_weapon_index_t::SMISSILE5_INDEX, secondary_ammo[secondary_weapon_index_t::SMISSILE5_INDEX]);
#endif

		//	Drop the player's missiles in packs of 1 and/or 4
		drop_missile_1_or_4(playerobj, secondary_weapon_index_t::HOMING_INDEX);
#if defined(DXX_BUILD_DESCENT_II)
		drop_missile_1_or_4(playerobj, secondary_weapon_index_t::GUIDED_INDEX);
#endif
		drop_missile_1_or_4(playerobj, secondary_weapon_index_t::CONCUSSION_INDEX);
#if defined(DXX_BUILD_DESCENT_II)
		drop_missile_1_or_4(playerobj, secondary_weapon_index_t::SMISSILE1_INDEX);
		drop_missile_1_or_4(playerobj, secondary_weapon_index_t::SMISSILE4_INDEX);
#endif

		//	If player has vulcan ammo, but no vulcan or gauss cannon, drop the ammo.
		if (!(player_info.primary_weapon_flags & HAS_VULCAN_AND_GAUSS_FLAGS))
		{
			auto amount = player_info.vulcan_ammo;
			if (amount > 200) {
				amount = 200;
			}
			if (amount)
				for (;;)
			{
				call_object_create_egg(playerobj, powerup_type_t::POW_VULCAN_AMMO);
				if (amount <= VULCAN_AMMO_AMOUNT)
					break;
				amount -= VULCAN_AMMO_AMOUNT;
			}
		}

		//	Always drop a shield and energy powerup.
		if (Game_mode & GM_MULTI) {
			call_object_create_egg(playerobj, powerup_type_t::POW_SHIELD_BOOST);
			call_object_create_egg(playerobj, powerup_type_t::POW_ENERGY);
		}
	}
}

void apply_damage_to_player(object &playerobj, const icobjptridx_t killer, const fix damage, const apply_damage_player possibly_friendly)
{
#if defined(DXX_BUILD_DESCENT_II)
	auto &BuddyState = LevelUniqueObjectState.BuddyState;
#endif
	if (Player_dead_state != player_dead_state::no)
		return;

	auto &player_info = playerobj.ctype.player_info;
	if (player_info.powerup_flags & PLAYER_FLAGS_INVULNERABLE)
		return;

	if (possibly_friendly != apply_damage_player::always && multi_maybe_disable_friendly_fire(static_cast<const object *>(killer)))
		return;

	if (Endlevel_sequence)
		return;

	//for the player, the 'real' shields are maintained in the Players[]
	//array.  The shields value in the player's object are, I think, not
	//used anywhere.  This routine, however, sets the objects shields to
	//be a mirror of the value in the Player structure.

	if (get_player_id(playerobj) == Player_num) {		//is this the local player?
		PALETTE_FLASH_ADD(f2i(damage)*4,-f2i(damage/2),-f2i(damage/2));	//flash red
		if (playerobj.shields < 0)
		{
			/*
			 * If player is already dead (but Player_dead_state has not
			 * caught up yet), preserve the original killer data.
			 */
			assert(playerobj.flags & OF_SHOULD_BE_DEAD);
			return;
		}
		const auto shields = (playerobj.shields -= damage);

		if (shields < 0)
		{
			playerobj.ctype.player_info.killer_objnum = killer;
			playerobj.flags |= OF_SHOULD_BE_DEAD;

#if defined(DXX_BUILD_DESCENT_II)
			auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
				if (killer && killer->type == OBJ_ROBOT && robot_is_companion(Robot_info[get_robot_id(killer)]))
					BuddyState.Buddy_sorry_time = {GameTime64};
#endif
		}
	}
}

namespace {

static void collide_player_and_weapon(const d_robot_info_array &Robot_info, const vmobjptridx_t playerobj, const vmobjptridx_t weapon, vms_vector &collision_point)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &imobjptridx = Objects.imptridx;
	fix		damage = weapon->shields;

#if defined(DXX_BUILD_DESCENT_II)
	if (get_weapon_id(weapon) == weapon_id_type::OMEGA_ID)
		if (!ok_to_do_omega_damage(weapon)) // see comment in laser.c
			return;

	//	Don't collide own smart mines unless direct hit.
	if (get_weapon_id(weapon) == weapon_id_type::SUPERPROX_ID)
		if (playerobj == weapon->ctype.laser_info.parent_num)
			if (vm_vec_dist_quick(collision_point, playerobj->pos) > playerobj->size)
				return;

	if (get_weapon_id(weapon) == weapon_id_type::EARTHSHAKER_ID)
		smega_rock_stuff();
#endif

	damage = fixmul(damage, weapon->ctype.laser_info.multiplier);
#if defined(DXX_BUILD_DESCENT_II)
	if (Game_mode & GM_MULTI)
		damage = fixmul(damage, Weapon_info[get_weapon_id(weapon)].multi_damage_scale);
#endif

#if 1
	/*
	 * Check if persistent weapon already hit this object. If yes, abort.
	 * If no, add this object to hitobj_list and do it's damage.
	 */
	if (weapon->mtype.phys_info.flags & PF_PERSISTENT)
	{
		if (weapon->ctype.laser_info.test_set_hitobj(playerobj))
			return;
	}
#endif

	const auto &&player_segp = vmsegptridx(playerobj->segnum);
	if (get_player_id(playerobj) == Player_num)
	{
		auto &player_info = playerobj->ctype.player_info;
		multi_digi_link_sound_to_pos((player_info.powerup_flags & PLAYER_FLAGS_INVULNERABLE) ? SOUND_WEAPON_HIT_DOOR : SOUND_PLAYER_GOT_HIT, player_segp, sidenum_t::WLEFT, collision_point, 0, F1_0);
	}

	object_create_explosion_without_damage(Vclip, player_segp, collision_point, i2f(10) / 2, vclip_index::player_hit);
	if ( Weapon_info[get_weapon_id(weapon)].damage_radius )
	{
		const auto obj2weapon{vm_vec_sub(collision_point, playerobj->pos)};
		const auto mag = vm_vec_mag(obj2weapon);
		if(mag > 0) // FVI code does not necessarily update the collision point for object2object collisions. Do that now.
		{
			collision_point = vm_vec_scale_add(playerobj->pos, obj2weapon, fixdiv(playerobj->size, mag)); 
			weapon->pos = collision_point;
		}
#if defined(DXX_BUILD_DESCENT_I)
		explode_badass_weapon(Robot_info, weapon, weapon->pos);
#elif defined(DXX_BUILD_DESCENT_II)
		explode_badass_weapon(Robot_info, weapon, collision_point);
#endif
	}

	maybe_kill_weapon(weapon,playerobj);

	bump_two_objects(Robot_info, playerobj, weapon, 0);	//no damage from bump

	if ( !Weapon_info[get_weapon_id(weapon)].damage_radius ) {
		imobjptridx_t killer = object_none;
		if ( weapon->ctype.laser_info.parent_num != object_none )
			killer = imobjptridx(weapon->ctype.laser_info.parent_num);

//		if (weapon->id == SMART_HOMING_ID)
//			damage /= 4;

		apply_damage_to_player(playerobj, killer, damage, apply_damage_player::check_for_friendly);
	}

	//	Robots become aware of you if you get hit.
	ai_do_cloak_stuff();

	return;
}

}

//	Nasty robots are the ones that attack you by running into you and doing lots of damage.
void collide_player_and_nasty_robot(const d_robot_info_array &Robot_info, const vmobjptridx_t playerobj, const vmobjptridx_t robot, const vms_vector &collision_point)
{
	const auto &&player_segp = vmsegptridx(playerobj->segnum);
	digi_link_sound_to_pos(Robot_info[get_robot_id(robot)].claw_sound, player_segp, sidenum_t::WLEFT, collision_point, 0, F1_0);
	object_create_explosion_without_damage(Vclip, player_segp, collision_point, i2f(10) / 2, vclip_index::player_hit);

	bump_two_objects(Robot_info, playerobj, robot, 0);	//no damage from bump

	apply_damage_to_player(playerobj, robot, F1_0 * (underlying_value(GameUniqueState.Difficulty_level) + 1), apply_damage_player::always);
}

namespace {

static vms_vector find_exit_direction(vms_vector result, const object &objp, const cscusegment segp)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vcvertptr = Vertices.vcptr;
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcwallptr = Walls.vcptr;
	for (const auto side : MAX_SIDES_PER_SEGMENT)
		if (WALL_IS_DOORWAY(GameBitmaps, Textures, vcwallptr, segp, side) & WALL_IS_DOORWAY_FLAG::fly)
		{
			const auto exit_point{compute_center_point_on_side(vcvertptr, segp, side)};
			vm_vec_add2(result, vm_vec_normalized_quick(vm_vec_sub(exit_point, objp.pos)));
			break;
		}
	return result;
}

}

void collide_player_and_materialization_center(const vmobjptridx_t objp)
{
	const auto &&segp = vmsegptridx(objp->segnum);
	digi_link_sound_to_pos(SOUND_PLAYER_GOT_HIT, segp, sidenum_t::WLEFT, objp->pos, 0, F1_0);
	object_create_explosion_without_damage(Vclip, segp, objp->pos, i2f(10) / 2, vclip_index::player_hit);

	if (get_player_id(objp) != Player_num)
		return;

	auto rand_vec = make_random_vector();
	rand_vec.x /= 4;
	rand_vec.y /= 4;
	rand_vec.z /= 4;
	auto exit_dir = find_exit_direction(rand_vec, objp, segp);
	vm_vec_normalize_quick(exit_dir);
	bump_one_object(objp, exit_dir, 64*F1_0);

#if defined(DXX_BUILD_DESCENT_I)
	const auto killer = object_none;
#elif defined(DXX_BUILD_DESCENT_II)
	auto &&killer = objp;	//	Changed, MK, 2/19/96, make killer the player, so if you die in matcen, will say you killed yourself
#endif
	apply_damage_to_player(objp, killer, 4*F1_0, apply_damage_player::always);
}

void collide_robot_and_materialization_center(const d_robot_info_array &Robot_info, const vmobjptridx_t objp)
{
	const auto &&segp = vmsegptridx(objp->segnum);
	digi_link_sound_to_pos(SOUND_ROBOT_HIT, segp, sidenum_t::WLEFT, objp->pos, 0, F1_0);

	if (const auto exp1_vclip_num = Robot_info[get_robot_id(objp)].exp1_vclip_num; Vclip.valid_index(exp1_vclip_num))
		object_create_explosion_without_damage(Vclip, segp, objp->pos, (objp->size / 2 * 3) / 4, exp1_vclip_num);

	auto exit_dir = find_exit_direction({}, objp, segp);
	bump_one_object(objp, exit_dir, 8*F1_0);

	apply_damage_to_robot(Robot_info, objp, F1_0, object_none);
}

void collide_live_local_player_and_powerup(const vmobjptridx_t powerup)
{
	if (do_powerup(powerup))
	{
		powerup->flags |= OF_SHOULD_BE_DEAD;
		if (Game_mode & GM_MULTI)
			multi_send_remobj(powerup);
	}
}

}

namespace {

static void collide_player_and_powerup(const d_robot_info_array &, object &playerobj, const vmobjptridx_t powerup, const vms_vector &)
{
	if (!Endlevel_sequence &&
		Player_dead_state == player_dead_state::no &&
		get_player_id(playerobj) == Player_num)
	{
		collide_live_local_player_and_powerup(powerup);
	}
	else if ((Game_mode & GM_MULTI_COOP) && (get_player_id(playerobj) != Player_num))
	{
		switch (get_powerup_id(powerup)) {
			case powerup_type_t::POW_KEY_BLUE:
				playerobj.ctype.player_info.powerup_flags |= PLAYER_FLAGS_BLUE_KEY;
				break;
			case powerup_type_t::POW_KEY_RED:
				playerobj.ctype.player_info.powerup_flags |= PLAYER_FLAGS_RED_KEY;
				break;
			case powerup_type_t::POW_KEY_GOLD:
				playerobj.ctype.player_info.powerup_flags |= PLAYER_FLAGS_GOLD_KEY;
				break;
			default:
				break;
		}
	}
	return;
}

static void collide_player_and_clutter(const d_robot_info_array &Robot_info, const vmobjptridx_t playerobj, const vmobjptridx_t clutter, const vms_vector &collision_point)
{
	if (check_collision_delayfunc_exec())
		digi_link_sound_to_pos(SOUND_ROBOT_HIT_PLAYER, vcsegptridx(playerobj->segnum), sidenum_t::WLEFT, collision_point, 0, F1_0);
	bump_two_objects(Robot_info, clutter, playerobj, 1);
}

}

//	See if weapon1 creates a badass explosion.  If so, create the explosion
//	Return true if weapon does proximity (as opposed to only contact) damage when it explodes.
namespace dsx {

namespace {

int maybe_detonate_weapon(const d_robot_info_array &Robot_info, const vmobjptridx_t weapon1, object &weapon2, const vms_vector &collision_point)
{
	if ( Weapon_info[get_weapon_id(weapon1)].damage_radius ) {
		auto dist = vm_vec_dist_quick(weapon1->pos, weapon2.pos);
		if (dist < F1_0*5) {
			maybe_kill_weapon(weapon1,weapon2);
			if (weapon1->flags & OF_SHOULD_BE_DEAD) {
#if defined(DXX_BUILD_DESCENT_I)
				explode_badass_weapon(Robot_info, weapon1, weapon1->pos);
#elif defined(DXX_BUILD_DESCENT_II)
				explode_badass_weapon(Robot_info, weapon1, collision_point);
#endif
				digi_link_sound_to_pos(Weapon_info[get_weapon_id(weapon1)].robot_hit_sound, vcsegptridx(weapon1->segnum), sidenum_t::WLEFT, collision_point, 0, F1_0);
			}
			return 1;
		} else {
			weapon1->lifeleft = min(static_cast<fix>(dist) / 64, F1_0);
			return 1;
		}
	} else
		return 0;
}

static void collide_weapon_and_weapon(const d_robot_info_array &Robot_info, const vmobjptridx_t weapon1, const vmobjptridx_t weapon2, const vms_vector &collision_point)
{
#if defined(DXX_BUILD_DESCENT_II)
	// -- Does this look buggy??:  if (weapon1->id == PMINE_ID && weapon1->id == PMINE_ID)
	if (get_weapon_id(weapon1) == weapon_id_type::PMINE_ID && get_weapon_id(weapon2) == weapon_id_type::PMINE_ID)
		return;		//these can't blow each other up

	if (get_weapon_id(weapon1) == weapon_id_type::OMEGA_ID) {
		if (!ok_to_do_omega_damage(weapon1)) // see comment in laser.c
			return;
	} else if (get_weapon_id(weapon2) == weapon_id_type::OMEGA_ID) {
		if (!ok_to_do_omega_damage(weapon2)) // see comment in laser.c
			return;
	}
#endif

	if ((Weapon_info[get_weapon_id(weapon1)].destroyable) || (Weapon_info[get_weapon_id(weapon2)].destroyable)) {
		// shooting Plasma will make bombs explode one drops at the same time since hitboxes overlap. Small HACK to get around this issue. if the player moves away from the bomb at least...
		if ((GameTime64 < weapon1->ctype.laser_info.creation_time + (F1_0/5)) && (GameTime64 < weapon2->ctype.laser_info.creation_time + (F1_0/5)) && (weapon1->ctype.laser_info.parent_num == weapon2->ctype.laser_info.parent_num))
			return;
		//	Bug reported by Adam Q. Pletcher on September 9, 1994, smart bomb homing missiles were toasting each other.
		if ((get_weapon_id(weapon1) == get_weapon_id(weapon2)) && (weapon1->ctype.laser_info.parent_num == weapon2->ctype.laser_info.parent_num))
			return;

#if defined(DXX_BUILD_DESCENT_I)
		if (Weapon_info[get_weapon_id(weapon1)].destroyable)
			if (maybe_detonate_weapon(Robot_info, weapon1, weapon2, collision_point))
				maybe_kill_weapon(weapon2,weapon1);

		if (Weapon_info[get_weapon_id(weapon2)].destroyable)
			if (maybe_detonate_weapon(Robot_info, weapon2, weapon1, collision_point))
				maybe_kill_weapon(weapon1,weapon2);
#elif defined(DXX_BUILD_DESCENT_II)
		if (Weapon_info[get_weapon_id(weapon1)].destroyable)
			if (maybe_detonate_weapon(Robot_info, weapon1, weapon2, collision_point))
				maybe_detonate_weapon(Robot_info, weapon2, weapon1, collision_point);

		if (Weapon_info[get_weapon_id(weapon2)].destroyable)
			if (maybe_detonate_weapon(Robot_info, weapon2, weapon1, collision_point))
				maybe_detonate_weapon(Robot_info, weapon1, weapon2, collision_point);
#endif
	}
}

static void collide_weapon_and_debris(const d_robot_info_array &Robot_info, const vmobjptridx_t weapon, const vmobjptridx_t debris, const vms_vector &collision_point)
{
#if defined(DXX_BUILD_DESCENT_II)
	//	Hack!  Prevent debris from causing bombs spewed at player death to detonate!
	if ((get_weapon_id(weapon) == weapon_id_type::PROXIMITY_ID) || (get_weapon_id(weapon) == weapon_id_type::SUPERPROX_ID)) {
		if (weapon->ctype.laser_info.creation_time + F1_0/2 > GameTime64)
			return;
	}
#endif
	if ( (weapon->ctype.laser_info.parent_type==OBJ_PLAYER) && !(debris->flags & OF_EXPLODING) )	{
		digi_link_sound_to_pos(SOUND_ROBOT_HIT, vcsegptridx(weapon->segnum), sidenum_t::WLEFT, collision_point, 0, F1_0);

		explode_object(LevelUniqueObjectState, Robot_info, LevelSharedSegmentState, LevelUniqueSegmentState, debris, 0);
		if ( Weapon_info[get_weapon_id(weapon)].damage_radius )
			explode_badass_weapon(Robot_info, weapon, collision_point);
		maybe_kill_weapon(weapon,debris);
		if (!(weapon->mtype.phys_info.flags & PF_PERSISTENT))
			weapon->flags |= OF_SHOULD_BE_DEAD;
	}
	return;
}

#if defined(DXX_BUILD_DESCENT_I)
#define DXX_COLLISION_TABLE(NO,DO)	\

#elif defined(DXX_BUILD_DESCENT_II)
#define DXX_COLLISION_TABLE(NO,DO)	\
	NO##_SAME_COLLISION(OBJ_MARKER)	\
	DO##_COLLISION(OBJ_PLAYER, OBJ_MARKER, collide_player_and_marker)	\
	NO##_COLLISION(OBJ_ROBOT, OBJ_MARKER)	\
	NO##_COLLISION(OBJ_HOSTAGE, OBJ_MARKER)	\
	NO##_COLLISION(OBJ_WEAPON, OBJ_MARKER)	\
	NO##_COLLISION(OBJ_CAMERA, OBJ_MARKER)	\
	NO##_COLLISION(OBJ_POWERUP, OBJ_MARKER)	\
	NO##_COLLISION(OBJ_DEBRIS, OBJ_MARKER)	\

#endif

#define COLLIDE_IGNORE_COLLISION(Robot_info,O1,O2,C)

#define COLLISION_TABLE(NO,DO)	\
	NO##_SAME_COLLISION(OBJ_FIREBALL)	\
	DO##_SAME_COLLISION(OBJ_ROBOT, collide_robot_and_robot)	\
	NO##_SAME_COLLISION(OBJ_HOSTAGE)	\
	DO##_SAME_COLLISION(OBJ_PLAYER, collide_player_and_player)	\
	DO##_SAME_COLLISION(OBJ_WEAPON, collide_weapon_and_weapon)	\
	NO##_SAME_COLLISION(OBJ_CAMERA)	\
	NO##_SAME_COLLISION(OBJ_POWERUP)	\
	NO##_SAME_COLLISION(OBJ_DEBRIS)	\
	DO##_COLLISION(OBJ_WALL, OBJ_ROBOT, COLLIDE_IGNORE_COLLISION)	\
	DO##_COLLISION(OBJ_WALL, OBJ_WEAPON, COLLIDE_IGNORE_COLLISION)	\
	DO##_COLLISION(OBJ_WALL, OBJ_PLAYER, COLLIDE_IGNORE_COLLISION)	\
	DO##_COLLISION(OBJ_WALL, OBJ_POWERUP, COLLIDE_IGNORE_COLLISION)	\
	DO##_COLLISION(OBJ_WALL, OBJ_DEBRIS, COLLIDE_IGNORE_COLLISION)	\
	NO##_COLLISION(OBJ_FIREBALL, OBJ_ROBOT)	\
	NO##_COLLISION(OBJ_FIREBALL, OBJ_HOSTAGE)	\
	NO##_COLLISION(OBJ_FIREBALL, OBJ_PLAYER)	\
	NO##_COLLISION(OBJ_FIREBALL, OBJ_WEAPON)	\
	NO##_COLLISION(OBJ_FIREBALL, OBJ_CAMERA)	\
	NO##_COLLISION(OBJ_FIREBALL, OBJ_POWERUP)	\
	NO##_COLLISION(OBJ_FIREBALL, OBJ_DEBRIS)	\
	NO##_COLLISION(OBJ_ROBOT, OBJ_HOSTAGE)	\
	DO##_COLLISION(OBJ_ROBOT, OBJ_PLAYER,  collide_robot_and_player)	\
	DO##_COLLISION(OBJ_ROBOT, OBJ_WEAPON,  collide_robot_and_weapon)	\
	NO##_COLLISION(OBJ_ROBOT, OBJ_CAMERA)	\
	NO##_COLLISION(OBJ_ROBOT, OBJ_POWERUP)	\
	NO##_COLLISION(OBJ_ROBOT, OBJ_DEBRIS)	\
	DO##_COLLISION(OBJ_HOSTAGE, OBJ_PLAYER,  collide_hostage_and_player)	\
	DO##_COLLISION(OBJ_HOSTAGE, OBJ_WEAPON, COLLIDE_IGNORE_COLLISION)	\
	NO##_COLLISION(OBJ_HOSTAGE, OBJ_CAMERA)	\
	NO##_COLLISION(OBJ_HOSTAGE, OBJ_POWERUP)	\
	NO##_COLLISION(OBJ_HOSTAGE, OBJ_DEBRIS)	\
	DO##_COLLISION(OBJ_PLAYER, OBJ_WEAPON,  collide_player_and_weapon)	\
	NO##_COLLISION(OBJ_PLAYER, OBJ_CAMERA)	\
	DO##_COLLISION(OBJ_PLAYER, OBJ_POWERUP, collide_player_and_powerup)	\
	NO##_COLLISION(OBJ_PLAYER, OBJ_DEBRIS)	\
	DO##_COLLISION(OBJ_PLAYER, OBJ_CNTRLCEN, collide_player_and_controlcen)	\
	DO##_COLLISION(OBJ_PLAYER, OBJ_CLUTTER, collide_player_and_clutter)	\
	NO##_COLLISION(OBJ_WEAPON, OBJ_CAMERA)	\
	NO##_COLLISION(OBJ_WEAPON, OBJ_POWERUP)	\
	DO##_COLLISION(OBJ_WEAPON, OBJ_DEBRIS,  collide_weapon_and_debris)	\
	NO##_COLLISION(OBJ_CAMERA, OBJ_POWERUP)	\
	NO##_COLLISION(OBJ_CAMERA, OBJ_DEBRIS)	\
	NO##_COLLISION(OBJ_POWERUP, OBJ_DEBRIS)	\
	DO##_COLLISION(OBJ_WEAPON, OBJ_CNTRLCEN, collide_weapon_and_controlcen)	\
	DO##_COLLISION(OBJ_ROBOT, OBJ_CNTRLCEN, collide_robot_and_controlcen)	\
	DO##_COLLISION(OBJ_WEAPON, OBJ_CLUTTER, collide_weapon_and_clutter)	\
	DXX_COLLISION_TABLE(NO,DO)	\


/* DPH: Put these macros on one long line to avoid CR/LF problems on linux */
#define COLLISION_OF(a,b) (((a)<<4) + (b))

#define DO_COLLISION(type1,type2,collision_function)	\
	case COLLISION_OF( (type1), (type2) ):	\
		static_assert(type1 < type2, "do " #type1 " < " #type2);	\
		collision_function(Robot_info, (A), (B), collision_point);	\
		break;
#define DO_SAME_COLLISION(type1,collision_function)	\
	case COLLISION_OF( (type1), (type1) ):	\
		collision_function(Robot_info, (A), (B), collision_point);	\
		break;

//these next two macros define a case that does nothing
#define NO_COLLISION(type1,type2)	\
	case COLLISION_OF( (type1), (type2) ):	\
		static_assert(type1 < type2, "no " #type1 " < " #type2);	\
		break;

#define NO_SAME_COLLISION(type1)	\
	case COLLISION_OF( (type1), (type1) ):	\
		break;

template <typename T, std::size_t V>
struct assert_no_truncation
{
	static_assert(static_cast<T>(V) == V, "truncation error");
};

}

void collide_two_objects(const d_robot_info_array &Robot_info, vmobjptridx_t A, vmobjptridx_t B, vms_vector &collision_point)
{
	if (B->type < A->type)
	{
		using std::swap;
		swap(A, B);
	}
	uint_fast8_t at, bt;
	const char *emsg;
	if (((at = A->type) >= MAX_OBJECT_TYPES && (emsg = "illegal object type A", true)) ||
		((bt = B->type) >= MAX_OBJECT_TYPES && (emsg = "illegal object type B", true)))
		throw std::runtime_error(emsg);
	uint_fast8_t collision_type = COLLISION_OF(at, bt);
	struct assert_object_type_not_truncated : std::pair<assert_no_truncation<decltype(at), MAX_OBJECT_TYPES>, assert_no_truncation<decltype(bt), MAX_OBJECT_TYPES>> {};
	struct assert_collision_of_not_truncated : assert_no_truncation<decltype(collision_type), COLLISION_OF(MAX_OBJECT_TYPES - 1, MAX_OBJECT_TYPES - 1)> {};
	switch( collision_type )	{
		COLLISION_TABLE(NO,DO)
	default:
		Int3();	//Error( "Unhandled collision_type in collide.c!\n" );
	}
}

}

#define ENABLE_COLLISION(type1,type2,f)	\
	COLLISION_RESULT(type1, type2, collision_result::check);	\
	COLLISION_RESULT(type2, type1, collision_result::check);	\

#define DISABLE_COLLISION(type1,type2)	\
	COLLISION_RESULT(type1, type2, collision_result::ignore);	\
	COLLISION_RESULT(type2, type1, collision_result::ignore);	\

#define ENABLE_SAME_COLLISION(type,f)	COLLISION_RESULT(type, type, collision_result::check);
#define DISABLE_SAME_COLLISION(type)	COLLISION_RESULT(type, type, collision_result::ignore);

namespace {

/* If not otherwise specified, collision results are ignored. */
template <object_type_t A, object_type_t B>
inline constexpr collision_result collision_result_t{collision_result::ignore};

/* Create symmetric collisions.  For any <A,B> where (B < A), use the result of
 * <B,A>.
 */
template <object_type_t A, object_type_t B>
requires(B < A)
inline constexpr collision_result collision_result_t<A, B>{collision_result_t<B, A>};

#define COLLISION_RESULT(type1,type2,result)	\
	template <>	\
	inline constexpr collision_result collision_result_t<type1, type2>{result}

COLLISION_TABLE(DISABLE, ENABLE);

template <std::size_t R, typename C>
[[deprecated("only specializations can be used")]]
inline constexpr collision_inner_array_t collision_inner_array
#ifdef __clang__
{}
#endif
;

template <std::size_t R, std::size_t... C>
requires(
	(COLLISION_OF(R, 0) < COLLISION_OF(R, sizeof...(C) - 1)) &&
	(COLLISION_OF(R, sizeof...(C) - 1) < COLLISION_OF(R + 1, 0))
)
inline constexpr collision_inner_array_t collision_inner_array<R, std::index_sequence<C...>>{{
	collision_result_t<static_cast<object_type_t>(R), static_cast<object_type_t>(C)>...
}};

template <typename R, typename C>
[[deprecated("only specializations can be used")]]
inline constexpr collision_outer_array_t collision_outer_array
#ifdef __clang__
{}
#endif
;

template <std::size_t... R, typename C>
inline constexpr collision_outer_array_t collision_outer_array<std::index_sequence<R...>, C>{{
	collision_inner_array<static_cast<object_type_t>(R), C>...
}};

}

namespace dsx {

constexpr collision_outer_array_t CollisionResult{collision_outer_array<std::make_index_sequence<MAX_OBJECT_TYPES>, std::make_index_sequence<MAX_OBJECT_TYPES>>};

}

#undef DISABLE_COLLISION
#undef ENABLE_COLLISION

#define ENABLE_COLLISION(T1,T2)	static_assert(	\
	collision_result_t<T1, T2> == collision_result::check &&	\
	collision_result_t<T2, T1> == collision_result::check,	\
	#T1 " " #T2	\
	)
#define DISABLE_COLLISION(T1,T2)	static_assert(	\
	collision_result_t<T1, T2> == collision_result::ignore &&	\
	collision_result_t<T2, T1> == collision_result::ignore,	\
	#T1 " " #T2	\
	)

	ENABLE_COLLISION( OBJ_WALL, OBJ_ROBOT );
	ENABLE_COLLISION( OBJ_WALL, OBJ_WEAPON );
	ENABLE_COLLISION( OBJ_WALL, OBJ_PLAYER  );
	DISABLE_COLLISION( OBJ_FIREBALL, OBJ_FIREBALL );

	ENABLE_COLLISION( OBJ_ROBOT, OBJ_ROBOT );
//	DISABLE_COLLISION( OBJ_ROBOT, OBJ_ROBOT );	//	ALERT: WARNING: HACK: MK = RESPONSIBLE! TESTING!!
	DISABLE_COLLISION( OBJ_HOSTAGE, OBJ_HOSTAGE );
	ENABLE_COLLISION( OBJ_PLAYER, OBJ_PLAYER );
	ENABLE_COLLISION( OBJ_WEAPON, OBJ_WEAPON );
	DISABLE_COLLISION( OBJ_CAMERA, OBJ_CAMERA );
	DISABLE_COLLISION( OBJ_POWERUP, OBJ_POWERUP );
	DISABLE_COLLISION( OBJ_DEBRIS, OBJ_DEBRIS );
	DISABLE_COLLISION( OBJ_FIREBALL, OBJ_ROBOT );
	DISABLE_COLLISION( OBJ_FIREBALL, OBJ_HOSTAGE );
	DISABLE_COLLISION( OBJ_FIREBALL, OBJ_PLAYER );
	DISABLE_COLLISION( OBJ_FIREBALL, OBJ_WEAPON );
	DISABLE_COLLISION( OBJ_FIREBALL, OBJ_CAMERA );
	DISABLE_COLLISION( OBJ_FIREBALL, OBJ_POWERUP );
	DISABLE_COLLISION( OBJ_FIREBALL, OBJ_DEBRIS );
	DISABLE_COLLISION( OBJ_ROBOT, OBJ_HOSTAGE );
	ENABLE_COLLISION( OBJ_ROBOT, OBJ_PLAYER );
	ENABLE_COLLISION( OBJ_ROBOT, OBJ_WEAPON );
	DISABLE_COLLISION( OBJ_ROBOT, OBJ_CAMERA );
	DISABLE_COLLISION( OBJ_ROBOT, OBJ_POWERUP );
	DISABLE_COLLISION( OBJ_ROBOT, OBJ_DEBRIS );
	ENABLE_COLLISION( OBJ_HOSTAGE, OBJ_PLAYER );
	ENABLE_COLLISION( OBJ_HOSTAGE, OBJ_WEAPON );
	DISABLE_COLLISION( OBJ_HOSTAGE, OBJ_CAMERA );
	DISABLE_COLLISION( OBJ_HOSTAGE, OBJ_POWERUP );
	DISABLE_COLLISION( OBJ_HOSTAGE, OBJ_DEBRIS );
	ENABLE_COLLISION( OBJ_PLAYER, OBJ_WEAPON );
	DISABLE_COLLISION( OBJ_PLAYER, OBJ_CAMERA );
	ENABLE_COLLISION( OBJ_PLAYER, OBJ_POWERUP );
	DISABLE_COLLISION( OBJ_PLAYER, OBJ_DEBRIS );
	DISABLE_COLLISION( OBJ_WEAPON, OBJ_CAMERA );
	DISABLE_COLLISION( OBJ_WEAPON, OBJ_POWERUP );
	ENABLE_COLLISION( OBJ_WEAPON, OBJ_DEBRIS );
	DISABLE_COLLISION( OBJ_CAMERA, OBJ_POWERUP );
	DISABLE_COLLISION( OBJ_CAMERA, OBJ_DEBRIS );
	DISABLE_COLLISION( OBJ_POWERUP, OBJ_DEBRIS );
	ENABLE_COLLISION( OBJ_POWERUP, OBJ_WALL );
	ENABLE_COLLISION( OBJ_WEAPON, OBJ_CNTRLCEN );
	ENABLE_COLLISION( OBJ_WEAPON, OBJ_CLUTTER );
	ENABLE_COLLISION( OBJ_PLAYER, OBJ_CNTRLCEN );
	ENABLE_COLLISION( OBJ_ROBOT, OBJ_CNTRLCEN );
	ENABLE_COLLISION( OBJ_PLAYER, OBJ_CLUTTER );
#if defined(DXX_BUILD_DESCENT_II)
	ENABLE_COLLISION( OBJ_PLAYER, OBJ_MARKER );
#endif
	ENABLE_COLLISION( OBJ_DEBRIS, OBJ_WALL );

namespace dsx {

window_event_result collide_object_with_wall(
#if defined(DXX_BUILD_DESCENT_II)
	const d_level_shared_destructible_light_state &LevelSharedDestructibleLightState,
#endif
	const d_robot_info_array &Robot_info, const vmobjptridx_t A, fix hitspeed, const vmsegptridx_t hitseg, const sidenum_t hitwall, const vms_vector &hitpt)
{
	auto &Objects = LevelUniqueObjectState.Objects;

	switch( A->type )	{
	case OBJ_NONE:
		Error( "A object of type NONE hit a wall!\n");
		break;
	case OBJ_PLAYER:		collide_player_and_wall(A,hitspeed,hitseg,hitwall,hitpt); break;
		case OBJ_WEAPON:
			return collide_weapon_and_wall(
#if defined(DXX_BUILD_DESCENT_II)
				LevelSharedDestructibleLightState,
#endif
				Robot_info, Objects, vmsegptridx, A, hitseg, hitwall, hitpt);
	case OBJ_DEBRIS:
		collide_debris_and_wall(Robot_info, A, hitseg, hitwall, hitpt);
		break;

	case OBJ_FIREBALL:	break;		//collide_fireball_and_wall(A,hitspeed,hitseg,hitwall,hitpt);
		case OBJ_ROBOT:
		{
			auto &Walls = LevelUniqueWallSubsystemState.Walls;
			auto &vcwallptr = Walls.vcptr;
			collide_robot_and_wall(vcwallptr, A, hitseg, hitwall, hitpt);
			break;
		}
	case OBJ_HOSTAGE:		break;		//collide_hostage_and_wall(A,hitspeed,hitseg,hitwall,hitpt);
	case OBJ_CAMERA:		break;		//collide_camera_and_wall(A,hitspeed,hitseg,hitwall,hitpt);
	case OBJ_POWERUP:		break;		//collide_powerup_and_wall(A,hitspeed,hitseg,hitwall,hitpt);
	case OBJ_GHOST:		break;	//do nothing

	default:
		Error( "Unhandled object type hit wall in collide.c\n" );
	}

	return window_event_result::handled;	// assume handled
}

}
