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
 * Code for flying through the mines
 *
 */


#include <stdio.h>
#include <stdlib.h>

#include "joy.h"
#include "dxxerror.h"

#include "inferno.h"
#include "segment.h"
#include "object.h"
#include "physics.h"
#include "robot.h"
#include "key.h"
#include "game.h"
#include "collide.h"
#include "fvi.h"
#include "newdemo.h"
#include "gameseg.h"
#include "timer.h"
#include "ai.h"
#include "wall.h"
#include "laser.h"
#if defined(DXX_BUILD_DESCENT_II)
#include "bm.h"
#include "player.h"
#define MAX_OBJECT_VEL	i2f(100)
#endif

#include "dxxsconf.h"
#include "compiler-range_for.h"

//Global variables for physics system

#define ROLL_RATE	0x2000
#define DAMP_ANG 	0x400 //min angle to bank
#define TURNROLL_SCALE	(0x4ec4/2)

//check point against each side of segment. return bitmask, where bit
//set means behind that side

//make sure matrix is orthogonal
void check_and_fix_matrix(vms_matrix &m)
{
	m = vm_vector_2_matrix(m.fvec,&m.uvec,nullptr);
}


static void do_physics_align_object(object_base &obj)
{
	vms_vector desired_upvec;
	fixang delta_ang,roll_ang;
	//vms_vector forvec = {0,0,f1_0};
	fix largest_d=-f1_0;
	const shared_side *best_side = nullptr;
	// bank player according to segment orientation

	//find side of segment that player is most alligned with

	range_for (auto &i, vcsegptr(obj.segnum)->sides)
	{
		const auto d = vm_vec_dot(i.normals[0], obj.orient.uvec);

		if (largest_d < d)
		{
			largest_d = d;
			best_side = &i;
		}
	}

	// new player leveling code: use normal of side closest to our up vec
	if (!get_side_is_quad(*best_side))
	{
		desired_upvec = vm_vec_avg(best_side->normals[0], best_side->normals[1]);
				vm_vec_normalize(desired_upvec);
		}
		else
		desired_upvec = best_side->normals[0];

	if (labs(vm_vec_dot(desired_upvec, obj.orient.fvec)) < f1_0 / 2)
	{
		vms_angvec tangles;
		
		const auto temp_matrix = vm_vector_2_matrix(obj.orient.fvec, &desired_upvec, nullptr);

		delta_ang = vm_vec_delta_ang(obj.orient.uvec, temp_matrix.uvec, obj.orient.fvec);

		delta_ang += obj.mtype.phys_info.turnroll;

		if (abs(delta_ang) > DAMP_ANG) {
			roll_ang = fixmul(FrameTime,ROLL_RATE);

			if (abs(delta_ang) < roll_ang) roll_ang = delta_ang;
			else if (delta_ang<0) roll_ang = -roll_ang;

			tangles.p = tangles.h = 0;  tangles.b = roll_ang;
			const auto &&rotmat = vm_angles_2_matrix(tangles);
			obj.orient = vm_matrix_x_matrix(obj.orient, rotmat);
		}
	}

}

static void set_object_turnroll(object_base &obj)
{
	fixang desired_bank;

	desired_bank = -fixmul(obj.mtype.phys_info.rotvel.y, TURNROLL_SCALE);

	if (obj.mtype.phys_info.turnroll != desired_bank)
	{
		fixang delta_ang,max_roll;

		max_roll = fixmul(ROLL_RATE,FrameTime);

		delta_ang = desired_bank - obj.mtype.phys_info.turnroll;

		if (labs(delta_ang) < max_roll)
			max_roll = delta_ang;
		else
			if (delta_ang < 0)
				max_roll = -max_roll;

		obj.mtype.phys_info.turnroll += max_roll;
	}

}


#define MAX_IGNORE_OBJS 100

#ifndef NDEBUG
int	Total_retries=0, Total_sims=0;
int	Dont_move_ai_objects=0;
#endif

#define FT (f1_0/64)

//	-----------------------------------------------------------------------------------------------------------
// add rotational velocity & acceleration
namespace dsx {
static void do_physics_sim_rot(object_base &obj)
{
	vms_angvec	tangles;
	//fix		rotdrag_scale;
	physics_info *pi;

	Assert(FrameTime > 0);	//Get MATT if hit this!

	pi = &obj.mtype.phys_info;

	if (!(pi->rotvel.x || pi->rotvel.y || pi->rotvel.z || pi->rotthrust.x || pi->rotthrust.y || pi->rotthrust.z))
		return;

	if (obj.mtype.phys_info.drag)
	{
		int count;
		fix drag,r,k;

		count = FrameTime / FT;
		r = FrameTime % FT;
		k = fixdiv(r,FT);

		drag = (obj.mtype.phys_info.drag * 5) / 2;

		if (obj.mtype.phys_info.flags & PF_USES_THRUST)
		{
			const auto accel = vm_vec_copy_scale(obj.mtype.phys_info.rotthrust, fixdiv(f1_0, obj.mtype.phys_info.mass));
			while (count--) {
				vm_vec_add2(obj.mtype.phys_info.rotvel, accel);
				vm_vec_scale(obj.mtype.phys_info.rotvel, f1_0 - drag);
			}

			//do linear scale on remaining bit of time

			vm_vec_scale_add2(obj.mtype.phys_info.rotvel, accel, k);
			vm_vec_scale(obj.mtype.phys_info.rotvel, f1_0 - fixmul(k, drag));
		}
		else
#if defined(DXX_BUILD_DESCENT_II)
			if (! (obj.mtype.phys_info.flags & PF_FREE_SPINNING))
#endif
		{
			fix total_drag=f1_0;

			while (count--)
				total_drag = fixmul(total_drag,f1_0-drag);

			//do linear scale on remaining bit of time

			total_drag = fixmul(total_drag,f1_0-fixmul(k,drag));

			vm_vec_scale(obj.mtype.phys_info.rotvel, total_drag);
		}

	}

	//now rotate object

	//unrotate object for bank caused by turn
	if (obj.mtype.phys_info.turnroll)
	{
		tangles.p = tangles.h = 0;
		tangles.b = -obj.mtype.phys_info.turnroll;
		const auto &&rotmat = vm_angles_2_matrix(tangles);
		obj.orient = vm_matrix_x_matrix(obj.orient, rotmat);
	}

	const auto frametime = FrameTime;
	tangles.p = fixmul(obj.mtype.phys_info.rotvel.x, frametime);
	tangles.h = fixmul(obj.mtype.phys_info.rotvel.y, frametime);
	tangles.b = fixmul(obj.mtype.phys_info.rotvel.z, frametime);

	obj.orient = vm_matrix_x_matrix(obj.orient, vm_angles_2_matrix(tangles));

	if (obj.mtype.phys_info.flags & PF_TURNROLL)
		set_object_turnroll(obj);

	//re-rotate object for bank caused by turn
	if (obj.mtype.phys_info.turnroll)
	{
		tangles.p = tangles.h = 0;
		tangles.b = obj.mtype.phys_info.turnroll;
		obj.orient = vm_matrix_x_matrix(obj.orient, vm_angles_2_matrix(tangles));
	}

	check_and_fix_matrix(obj.orient);
}
}

// On joining edges fvi tends to get inaccurate as hell. Approach is to check if the object interects with the wall and if so, move away from it.
static void fix_illegal_wall_intersection(const vmobjptridx_t obj)
{
	if (!(obj->type == OBJ_PLAYER || obj->type == OBJ_ROBOT))
		return;

	object_intersects_wall_result_t hresult;
	if (object_intersects_wall_d(obj, hresult))
	{
		vm_vec_scale_add2(obj->pos, Segments[hresult.seg].sides[hresult.side].normals[0], FrameTime*10);
		update_object_seg(obj);
	}
}

namespace {

class ignore_objects_array_t
{
	typedef array<objnum_t, MAX_IGNORE_OBJS> array_t;
	array_t a;
	array_t::iterator e;
public:
	ignore_objects_array_t() :
		e(a.begin())
	{
	}
	bool push_back(objnum_t o)
	{
		if (e == a.end())
			return false;
		*e++ = o;
		return true;
	}
	operator std::pair<const objnum_t *, const objnum_t *>() const
	{
		return {a.begin(), e};
	}
};

}

//	-----------------------------------------------------------------------------------------------------------
//Simulate a physics object for this frame
namespace dsx {
window_event_result do_physics_sim(const vmobjptridx_t obj, phys_visited_seglist *const phys_segs)
{
	ignore_objects_array_t ignore_obj_list;
	int try_again;
	int fate=0;
	vms_vector ipos;		//position after this frame
	segnum_t WallHitSeg;
	int WallHitSide;
	fvi_info hit_info;
	fvi_query fq;
	vms_vector save_pos;
	fix drag;
	fix sim_time;
	vms_vector start_pos;
	int obj_stopped=0;
	fix moved_time;			//how long objected moved before hit something
	physics_info *pi;
	auto orig_segnum = obj->segnum;
	int bounced=0;
	bool Player_ScrapeFrame=false;
	auto result = window_event_result::handled;

	Assert(obj->movement_type == MT_PHYSICS);

#ifndef NDEBUG
	if (Dont_move_ai_objects)
		if (obj->control_type == CT_AI)
			return window_event_result::ignored;
#endif

	pi = &obj->mtype.phys_info;

	do_physics_sim_rot(obj);

	if (!(pi->velocity.x || pi->velocity.y || pi->velocity.z || pi->thrust.x || pi->thrust.y || pi->thrust.z))
		return window_event_result::ignored;

	sim_time = FrameTime;

	start_pos = obj->pos;

		//if uses thrust, cannot have zero drag
	Assert(!(obj->mtype.phys_info.flags&PF_USES_THRUST) || obj->mtype.phys_info.drag!=0);

	//do thrust & drag
	if ((drag = obj->mtype.phys_info.drag) != 0) {

		int count;
		fix r,k,have_accel;

		count = FrameTime / FT;
		r = FrameTime % FT;
		k = fixdiv(r,FT);

		if (obj->mtype.phys_info.flags & PF_USES_THRUST) {

			const auto accel = vm_vec_copy_scale(obj->mtype.phys_info.thrust,fixdiv(f1_0,obj->mtype.phys_info.mass));
			have_accel = (accel.x || accel.y || accel.z);

			while (count--) {
				if (have_accel)
					vm_vec_add2(obj->mtype.phys_info.velocity,accel);

				vm_vec_scale(obj->mtype.phys_info.velocity,f1_0-drag);
			}

			//do linear scale on remaining bit of time

			vm_vec_scale_add2(obj->mtype.phys_info.velocity,accel,k);
			if (drag)
				vm_vec_scale(obj->mtype.phys_info.velocity,f1_0-fixmul(k,drag));
		}
		else if (drag)
		{
			fix total_drag=f1_0;

			while (count--)
				total_drag = fixmul(total_drag,f1_0-drag);

			//do linear scale on remaining bit of time

			total_drag = fixmul(total_drag,f1_0-fixmul(k,drag));

			vm_vec_scale(obj->mtype.phys_info.velocity,total_drag);
		}
	}

	int count = 0;
	do {
		try_again = 0;

		//Move the object
		const auto frame_vec = vm_vec_copy_scale(obj->mtype.phys_info.velocity, sim_time);

		if ( (frame_vec.x==0) && (frame_vec.y==0) && (frame_vec.z==0) )	
			break;

		count++;

		//	If retry count is getting large, then we are trying to do something stupid.
		if (count > 8) break; // in original code this was 3 for all non-player objects. still leave us some limit in case fvi goes apeshit.

		const auto new_pos = vm_vec_add(obj->pos,frame_vec);
		fq.p0						= &obj->pos;
		fq.startseg				= obj->segnum;
		fq.p1						= &new_pos;
		fq.rad					= obj->size;
		fq.thisobjnum			= obj;
		fq.ignore_obj_list	= ignore_obj_list;
		fq.flags					= FQ_CHECK_OBJS;

		if (obj->type == OBJ_WEAPON)
			fq.flags |= FQ_TRANSPOINT;

		if (phys_segs)
			fq.flags |= FQ_GET_SEGLIST;

		fate = find_vector_intersection(fq, hit_info);
		//	Matt: Mike's hack.
		if (fate == HIT_OBJECT) {
			auto &objp = *vcobjptr(hit_info.hit_object);

			if ((objp.type == OBJ_WEAPON && is_proximity_bomb_or_smart_mine(get_weapon_id(objp))) ||
				objp.type == OBJ_POWERUP) // do not increase count for powerups since they *should* not change our movement
				count--;
		}

#ifndef NDEBUG
		if (fate == HIT_BAD_P0) {
			Int3();
		}
#endif

		if (phys_segs && !hit_info.seglist.empty())
		{
			auto n_phys_segs = phys_segs->nsegs;
			if (n_phys_segs && phys_segs->seglist[n_phys_segs - 1] == hit_info.seglist[0])
				n_phys_segs--;
			range_for (const auto &hs, hit_info.seglist)
			{
				if (!(n_phys_segs < phys_segs->seglist.size() - 1))
					break;
				phys_segs->seglist[n_phys_segs++] = hs;
			}
			phys_segs->nsegs = n_phys_segs;
		}

		ipos = hit_info.hit_pnt;
		auto iseg = hit_info.hit_seg;
		WallHitSide = hit_info.hit_side;
		WallHitSeg = hit_info.hit_side_seg;

		if (iseg==segment_none) {		//some sort of horrible error
			if (obj->type == OBJ_WEAPON)
				obj->flags |= OF_SHOULD_BE_DEAD;
			break;
		}

		Assert(!((fate==HIT_WALL) && ((WallHitSeg == segment_none) || (WallHitSeg > Highest_segment_index))));

		save_pos = obj->pos;			//save the object's position
		auto save_seg = obj->segnum;

		// update object's position and segment number
		obj->pos = ipos;

		if ( iseg != obj->segnum )
			obj_relink(vmobjptr, vmsegptr, obj, vmsegptridx(iseg));

		//if start point not in segment, move object to center of segment
		if (get_seg_masks(vcvertptr, obj->pos, vcsegptr(obj->segnum), 0).centermask !=0 )
		{
			auto n = find_object_seg(obj);
			if (n == segment_none)
			{
				//Int3();
				if (obj->type == OBJ_PLAYER && (n = find_point_seg(obj->last_pos, vmsegptridx(obj->segnum))) != segment_none)
				{
					obj->pos = obj->last_pos;
					obj_relink(vmobjptr, vmsegptr, obj, n);
				}
				else {
					compute_segment_center(vcvertptr, obj->pos, vcsegptr(obj->segnum));
					obj->pos.x += obj;
				}
				if (obj->type == OBJ_WEAPON)
					obj->flags |= OF_SHOULD_BE_DEAD;
			}
			return window_event_result::ignored;
		}

		//calulate new sim time
		{
			//vms_vector moved_vec;
			vms_vector moved_vec_n;
			fix attempted_dist,actual_dist;

			actual_dist = vm_vec_normalized_dir(moved_vec_n,obj->pos,save_pos);

			if (fate==HIT_WALL && vm_vec_dot(moved_vec_n,frame_vec) < 0) {		//moved backwards

				//don't change position or sim_time

				obj->pos = save_pos;
		
				//iseg = obj->segnum;		//don't change segment

				obj_relink(vmobjptr, vmsegptr, obj, vmsegptridx(save_seg));

				moved_time = 0;
			}
			else {
				fix old_sim_time;

				attempted_dist = vm_vec_mag(frame_vec);

				old_sim_time = sim_time;

				sim_time = fixmuldiv(sim_time,attempted_dist-actual_dist,attempted_dist);

				moved_time = old_sim_time - sim_time;

				if (sim_time < 0 || sim_time>old_sim_time) {
					sim_time = old_sim_time;
					//WHY DOES THIS HAPPEN??

					moved_time = 0;
				}
			}
		}


		switch( fate )		{

			case HIT_WALL:		{
				fix hit_speed=0,wall_part=0;

				// Find hit speed	

				const auto moved_v = vm_vec_sub(obj->pos,save_pos);

				wall_part = vm_vec_dot(moved_v,hit_info.hit_wallnorm);

				if ((wall_part != 0 && moved_time>0 && (hit_speed=-fixdiv(wall_part,moved_time))>0) || obj->type == OBJ_WEAPON || obj->type == OBJ_DEBRIS)
					result = collide_object_with_wall(obj, hit_speed, vmsegptridx(WallHitSeg), WallHitSide, hit_info.hit_pnt);
				/*
				 * Due to the nature of this loop, it's possible that a local player may receive scrape damage multiple times in one frame.
				 * Check if we received damage and do not apply more damage (nor produce damage sounds/flashes/bumps, etc) for the rest of the loop.
				 * It's possible that other walls later in the loop would still be valid for scraping but due to the generalized outcome, this should be negligible (practical wall sliding is handled below).
				 * NOTE: Remote players will return false and never receive damage. But since we handle only one object (remote or local) per loop, this is no problem. 
				 */
				if (obj->type == OBJ_PLAYER && Player_ScrapeFrame == false)
					Player_ScrapeFrame = scrape_player_on_wall(obj, vmsegptridx(WallHitSeg), WallHitSide, hit_info.hit_pnt);

				Assert( WallHitSeg != segment_none );
				Assert( WallHitSide > -1 );

				if ( !(obj->flags&OF_SHOULD_BE_DEAD) )	{
					int forcefield_bounce;		//bounce off a forcefield

#if defined(DXX_BUILD_DESCENT_II)
					if (!cheats.bouncyfire)
#endif
					Assert(!(obj->mtype.phys_info.flags & PF_STICK && obj->mtype.phys_info.flags & PF_BOUNCE));	//can't be bounce and stick

#if defined(DXX_BUILD_DESCENT_I)
					/*
					 * Force fields are not supported in Descent 1.  Use
					 * this as a placeholder to make the code match the
					 * force field handling in Descent 2.
					 */
					forcefield_bounce = 0;
#elif defined(DXX_BUILD_DESCENT_II)
					forcefield_bounce = (TmapInfo[Segments[WallHitSeg].sides[WallHitSide].tmap_num].flags & TMI_FORCE_FIELD);
					int check_vel=0;
#endif

					if (!forcefield_bounce && (obj->mtype.phys_info.flags & PF_STICK)) {		//stop moving

						LevelUniqueStuckObjectState.add_stuck_object(vcwallptr, obj, vmsegptr(WallHitSeg), WallHitSide);

						vm_vec_zero(obj->mtype.phys_info.velocity);
						obj_stopped = 1;
						try_again = 0;
					}
					else {					// Slide object along wall

						wall_part = vm_vec_dot(hit_info.hit_wallnorm,obj->mtype.phys_info.velocity);

						// if wall_part, make sure the value is sane enough to get usable velocity computed
						if (wall_part < 0 && wall_part > -f1_0) wall_part = -f1_0;
						if (wall_part > 0 && wall_part < f1_0) wall_part = f1_0;

						if (forcefield_bounce || (obj->mtype.phys_info.flags & PF_BOUNCE)) {		//bounce off wall
							wall_part *= 2;	//Subtract out wall part twice to achieve bounce

#if defined(DXX_BUILD_DESCENT_II)
							if (forcefield_bounce) {
								check_vel = 1;				//check for max velocity
								if (obj->type == OBJ_PLAYER)
									wall_part *= 2;		//player bounce twice as much
							}

							if ( obj->mtype.phys_info.flags & PF_BOUNCES_TWICE) {
								Assert(obj->mtype.phys_info.flags & PF_BOUNCE);
								if (obj->mtype.phys_info.flags & PF_BOUNCED_ONCE)
									obj->mtype.phys_info.flags &= ~(PF_BOUNCE+PF_BOUNCED_ONCE+PF_BOUNCES_TWICE);
								else
									obj->mtype.phys_info.flags |= PF_BOUNCED_ONCE;
							}

							bounced = 1;		//this object bounced
#endif
						}

						vm_vec_scale_add2(obj->mtype.phys_info.velocity,hit_info.hit_wallnorm,-wall_part);

#if defined(DXX_BUILD_DESCENT_II)
						if (check_vel) {
							fix vel = vm_vec_mag_quick(obj->mtype.phys_info.velocity);

							if (vel > MAX_OBJECT_VEL)
								vm_vec_scale(obj->mtype.phys_info.velocity,fixdiv(MAX_OBJECT_VEL,vel));
						}

						if (bounced && obj->type == OBJ_WEAPON)
							vm_vector_2_matrix(obj->orient,obj->mtype.phys_info.velocity,&obj->orient.uvec,nullptr);
#endif

						try_again = 1;
					}
				}

				break;
			}

			case HIT_OBJECT:		{
				vms_vector old_vel;

				// Mark the hit object so that on a retry the fvi code
				// ignores this object.

				Assert(hit_info.hit_object != object_none);
				//	Calculcate the hit point between the two objects.
				{
					fix			size0, size1;
					const auto &&hit = obj.absolute_sibling(hit_info.hit_object);
					const auto &ppos0 = hit->pos;
					const auto &ppos1 = obj->pos;
					size0 = hit->size;
					size1 = obj->size;
					Assert(size0+size1 != 0);	// Error, both sizes are 0, so how did they collide, anyway?!?
					//vm_vec_scale(vm_vec_sub(&pos_hit, ppos1, ppos0), fixdiv(size0, size0 + size1));
					//vm_vec_add2(&pos_hit, ppos0);
					auto pos_hit = vm_vec_sub(ppos1, ppos0);
					vm_vec_scale_add(pos_hit,ppos0,pos_hit,fixdiv(size0, size0 + size1));

					old_vel = obj->mtype.phys_info.velocity;

					collide_two_objects( obj, hit, pos_hit);

				}

				// Let object continue its movement
				if ( !(obj->flags&OF_SHOULD_BE_DEAD)  )	{
					//obj->pos = save_pos;

					if (obj->mtype.phys_info.flags&PF_PERSISTENT || (old_vel.x == obj->mtype.phys_info.velocity.x && old_vel.y == obj->mtype.phys_info.velocity.y && old_vel.z == obj->mtype.phys_info.velocity.z)) {
						//if (Objects[hit_info.hit_object].type == OBJ_POWERUP)
						if (ignore_obj_list.push_back(hit_info.hit_object))
						try_again = 1;
					}
				}

				break;
			}	
			case HIT_NONE:		
				break;

#ifndef NDEBUG
			case HIT_BAD_P0:
				Int3();		// Unexpected collision type: start point not in specified segment.
				break;
			default:
				// Unknown collision type returned from find_vector_intersection!!
				Int3();
				break;
#endif
		}
	} while ( try_again );

	//	Pass retry count info to AI.
	if (obj->control_type == CT_AI) {
		if (count > 0) {
			obj->ctype.ai_info.ail.retry_count = count-1;
#ifndef NDEBUG
			Total_retries += count-1;
			Total_sims++;
#endif
		}
	}

	// After collision with objects and walls, set velocity from actual movement
	if (!obj_stopped && !bounced 
		&& ((obj->type == OBJ_PLAYER) || (obj->type == OBJ_ROBOT) || (obj->type == OBJ_DEBRIS)) 
		&& ((fate == HIT_WALL) || (fate == HIT_OBJECT) || (fate == HIT_BAD_P0))
		)
	{	
		const auto moved_vec = vm_vec_sub(obj->pos,start_pos);
		vm_vec_copy_scale(obj->mtype.phys_info.velocity,moved_vec,fixdiv(f1_0,FrameTime));
	}

	fix_illegal_wall_intersection(obj);

	//Assert(check_point_in_seg(&obj->pos,obj->segnum,0).centermask==0);

	//if (obj->control_type == CT_FLYING)
	if (obj->mtype.phys_info.flags & PF_LEVELLING)
		do_physics_align_object( obj );

	//hack to keep player from going through closed doors
	if (obj->type==OBJ_PLAYER && obj->segnum!=orig_segnum && (!cheats.ghostphysics) ) {

		const auto orig_segp = vcsegptr(orig_segnum);
		const auto &&sidenum = find_connect_side(vcsegptridx(obj->segnum), orig_segp);
		if (sidenum != side_none)
		{

			if (! (WALL_IS_DOORWAY(GameBitmaps, Textures, vcwallptr, orig_segp, orig_segp, sidenum) & WID_FLY_FLAG))
			{
				fix dist;

				//bump object back

				auto &s = orig_segp->sides[sidenum];

				const auto v = create_abs_vertex_lists(orig_segp, s, sidenum);
				const auto &vertex_list = v.second;

				//let's pretend this wall is not triangulated
				const auto b = begin(vertex_list);
				const auto vertnum = *std::min_element(b, std::next(b, 4));

				dist = vm_dist_to_plane(start_pos, s.normals[0], vcvertptr(vertnum));
				vm_vec_scale_add(obj->pos, start_pos, s.normals[0], obj->size-dist);
				update_object_seg(obj);

			}
		}
	}

//--WE ALWYS WANT THIS IN, MATT AND MIKE DECISION ON 12/10/94, TWO MONTHS AFTER FINAL 	#ifndef NDEBUG
	//if end point not in segment, move object to last pos, or segment center
	if (get_seg_masks(vcvertptr, obj->pos, vcsegptr(obj->segnum), 0).centermask != 0)
	{
		if (find_object_seg(obj)==segment_none) {
			segnum_t n;

			//Int3();
			const auto &&obj_segp = vmsegptridx(obj->segnum);
			if (obj->type==OBJ_PLAYER && (n = find_point_seg(obj->last_pos,obj_segp)) != segment_none)
			{
				obj->pos = obj->last_pos;
				obj_relink(vmobjptr, vmsegptr, obj, vmsegptridx(n));
			}
			else {
				compute_segment_center(vcvertptr, obj->pos, obj_segp);
				obj->pos.x += obj;
			}
			if (obj->type == OBJ_WEAPON)
				obj->flags |= OF_SHOULD_BE_DEAD;
		}
	}

	return result;
//--WE ALWYS WANT THIS IN, MATT AND MIKE DECISION ON 12/10/94, TWO MONTHS AFTER FINAL 	#endif
}
}

namespace dcx {

//Applies an instantaneous force on an object, resulting in an instantaneous
//change in velocity.
void phys_apply_force(object_base &obj, const vms_vector &force_vec)
{
	if (obj.movement_type != MT_PHYSICS)
		return;

	//	Put in by MK on 2/13/96 for force getting applied to Omega blobs, which have 0 mass,
	//	in collision with crazy reactor robot thing on d2levf-s.
	if (obj.mtype.phys_info.mass == 0)
		return;

	//Add in acceleration due to force
	vm_vec_scale_add2(obj.mtype.phys_info.velocity, force_vec, fixdiv(f1_0, obj.mtype.phys_info.mass));
}

//	----------------------------------------------------------------
//	Do *dest = *delta unless:
//				*delta is pretty small
//		and	they are of different signs.
static void physics_set_rotvel_and_saturate(fix &dest, fix delta)
{
	if ((delta ^ dest) < 0) {
		if (abs(delta) < F1_0/8) {
			dest = delta/4;
		} else
			dest = delta;
	} else {
		dest = delta;
	}
}

static inline vms_angvec vm_extract_angles_vector(const vms_vector &v)
{
	vms_angvec a;
	return vm_extract_angles_vector(a, v), a;
}

//	------------------------------------------------------------------------------------------------------
//	Note: This is the old ai_turn_towards_vector code.
//	phys_apply_rot used to call ai_turn_towards_vector until I fixed it, which broke phys_apply_rot.
void physics_turn_towards_vector(const vms_vector &goal_vector, object_base &obj, fix rate)
{
	fix			delta_p, delta_h;

	// Make this object turn towards the goal_vector.  Changes orientation, doesn't change direction of movement.
	// If no one moves, will be facing goal_vector in 1 second.

	//	Detect null vector.
	if ((goal_vector.x == 0) && (goal_vector.y == 0) && (goal_vector.z == 0))
		return;

	//	Make morph objects turn more slowly.
	if (obj.control_type == CT_MORPH)
		rate *= 2;

	const auto dest_angles = vm_extract_angles_vector(goal_vector);
	const auto cur_angles = vm_extract_angles_vector(obj.orient.fvec);

	delta_p = (dest_angles.p - cur_angles.p);
	delta_h = (dest_angles.h - cur_angles.h);

	if (delta_p > F1_0/2) delta_p = dest_angles.p - cur_angles.p - F1_0;
	if (delta_p < -F1_0/2) delta_p = dest_angles.p - cur_angles.p + F1_0;
	if (delta_h > F1_0/2) delta_h = dest_angles.h - cur_angles.h - F1_0;
	if (delta_h < -F1_0/2) delta_h = dest_angles.h - cur_angles.h + F1_0;

	delta_p = fixdiv(delta_p, rate);
	delta_h = fixdiv(delta_h, rate);

	if (abs(delta_p) < F1_0/16) delta_p *= 4;
	if (abs(delta_h) < F1_0/16) delta_h *= 4;

	auto &rotvel_ptr = obj.mtype.phys_info.rotvel;
	physics_set_rotvel_and_saturate(rotvel_ptr.x, delta_p);
	physics_set_rotvel_and_saturate(rotvel_ptr.y, delta_h);
	rotvel_ptr.z = 0;
}

}

namespace dsx {

//	-----------------------------------------------------------------------------
//	Applies an instantaneous whack on an object, resulting in an instantaneous
//	change in orientation.
void phys_apply_rot(object &obj, const vms_vector &force_vec)
{
	fix	rate;

	if (obj.movement_type != MT_PHYSICS)
		return;

	auto vecmag = vm_vec_mag(force_vec);
	if (vecmag < F1_0/32)
		rate = 4*F1_0;
	else if (vecmag < obj.mtype.phys_info.mass >> 11)
		rate = 4*F1_0;
	else {
		rate = fixdiv(obj.mtype.phys_info.mass, vecmag / 8);
		if (obj.type == OBJ_ROBOT) {
			if (rate < F1_0/4)
				rate = F1_0/4;
#if defined(DXX_BUILD_DESCENT_I)
			obj.ctype.ai_info.SKIP_AI_COUNT = 2;
#elif defined(DXX_BUILD_DESCENT_II)
			//	Changed by mk, 10/24/95, claw guys should not slow down when attacking!
			if (!Robot_info[get_robot_id(obj)].thief && !Robot_info[get_robot_id(obj)].attack_type) {
				if (obj.ctype.ai_info.SKIP_AI_COUNT * FrameTime < 3*F1_0/4) {
					fix	tval = fixdiv(F1_0, 8*FrameTime);
					int	addval;

					addval = f2i(tval);

					if ( (d_rand() * 2) < (tval & 0xffff))
						addval++;
					obj.ctype.ai_info.SKIP_AI_COUNT += addval;
				}
			}
#endif
		} else {
			if (rate < F1_0/2)
				rate = F1_0/2;
		}
	}

	//	Turn amount inversely proportional to mass.  Third parameter is seconds to do 360 turn.
	physics_turn_towards_vector(force_vec, obj, rate);
}

}

namespace dcx {

//this routine will set the thrust for an object to a value that will
//(hopefully) maintain the object's current velocity
void set_thrust_from_velocity(object_base &obj)
{
	Assert(obj.movement_type == MT_PHYSICS);
	auto &phys_info = obj.mtype.phys_info;
	vm_vec_copy_scale(phys_info.thrust, phys_info.velocity,
		fixmuldiv(phys_info.mass, phys_info.drag, F1_0 - phys_info.drag)
	);
}

}
