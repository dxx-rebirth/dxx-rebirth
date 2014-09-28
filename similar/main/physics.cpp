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

int floor_levelling=0;

//make sure matrix is orthogonal
void check_and_fix_matrix(vms_matrix *m)
{
	vms_matrix tempm;

	vm_vector_2_matrix(&tempm,&m->fvec,&m->uvec,NULL);
	*m  = tempm;
}


static void do_physics_align_object( object * obj )
{
	vms_vector desired_upvec;
	fixang delta_ang,roll_ang;
	//vms_vector forvec = {0,0,f1_0};
	vms_matrix temp_matrix;
	fix d,largest_d=-f1_0;
	int best_side;

        best_side=0;
	// bank player according to segment orientation

	//find side of segment that player is most alligned with

	for (int i=0;i<6;i++) {
			d = vm_vec_dot(&Segments[obj->segnum].sides[i].normals[0],&obj->orient.uvec);

		if (d > largest_d) {largest_d = d; best_side=i;}
	}

	if (floor_levelling) {

		// old way: used floor's normal as upvec
			desired_upvec = Segments[obj->segnum].sides[3].normals[0];

	}
	else  // new player leveling code: use normal of side closest to our up vec
		if (get_num_faces(&Segments[obj->segnum].sides[best_side])==2) {
				side *s = &Segments[obj->segnum].sides[best_side];
				desired_upvec.x = (s->normals[0].x + s->normals[1].x) / 2;
				desired_upvec.y = (s->normals[0].y + s->normals[1].y) / 2;
				desired_upvec.z = (s->normals[0].z + s->normals[1].z) / 2;
		
				vm_vec_normalize(&desired_upvec);
		}
		else
				desired_upvec = Segments[obj->segnum].sides[best_side].normals[0];

	if (labs(vm_vec_dot(&desired_upvec,&obj->orient.fvec)) < f1_0/2) {
		vms_angvec tangles;
		
		vm_vector_2_matrix(&temp_matrix,&obj->orient.fvec,&desired_upvec,NULL);

		delta_ang = vm_vec_delta_ang(&obj->orient.uvec,&temp_matrix.uvec,&obj->orient.fvec);

		delta_ang += obj->mtype.phys_info.turnroll;

		if (abs(delta_ang) > DAMP_ANG) {
			vms_matrix rotmat;

			roll_ang = fixmul(FrameTime,ROLL_RATE);

			if (abs(delta_ang) < roll_ang) roll_ang = delta_ang;
			else if (delta_ang<0) roll_ang = -roll_ang;

			tangles.p = tangles.h = 0;  tangles.b = roll_ang;
			vm_angles_2_matrix(&rotmat,&tangles);
			obj->orient = vm_matrix_x_matrix(obj->orient,rotmat);
		}
		else floor_levelling=0;
	}

}

static void set_object_turnroll(object *obj)
{
	fixang desired_bank;

	desired_bank = -fixmul(obj->mtype.phys_info.rotvel.y,TURNROLL_SCALE);

	if (obj->mtype.phys_info.turnroll != desired_bank) {
		fixang delta_ang,max_roll;

		max_roll = fixmul(ROLL_RATE,FrameTime);

		delta_ang = desired_bank - obj->mtype.phys_info.turnroll;

		if (labs(delta_ang) < max_roll)
			max_roll = delta_ang;
		else
			if (delta_ang < 0)
				max_roll = -max_roll;

		obj->mtype.phys_info.turnroll += max_roll;
	}

}

//list of segments went through
int phys_seglist[MAX_FVI_SEGS],n_phys_segs;


#define MAX_IGNORE_OBJS 100

#ifndef NDEBUG
#define EXTRA_DEBUG 1		//no extra debug when NDEBUG is on
#endif

#ifdef EXTRA_DEBUG
object *debug_obj=NULL;
#endif

#ifndef NDEBUG
int	Total_retries=0, Total_sims=0;
int	Dont_move_ai_objects=0;
#endif

#define FT (f1_0/64)

//	-----------------------------------------------------------------------------------------------------------
// add rotational velocity & acceleration
static void do_physics_sim_rot(object *obj)
{
	vms_angvec	tangles;
	vms_matrix	rotmat,new_orient;
	//fix		rotdrag_scale;
	physics_info *pi;

	Assert(FrameTime > 0);	//Get MATT if hit this!

	pi = &obj->mtype.phys_info;

	if (!(pi->rotvel.x || pi->rotvel.y || pi->rotvel.z || pi->rotthrust.x || pi->rotthrust.y || pi->rotthrust.z))
		return;

	if (obj->mtype.phys_info.drag) {
		int count;
		vms_vector accel;
		fix drag,r,k;

		count = FrameTime / FT;
		r = FrameTime % FT;
		k = fixdiv(r,FT);

		drag = (obj->mtype.phys_info.drag*5)/2;

		if (obj->mtype.phys_info.flags & PF_USES_THRUST) {

			vm_vec_copy_scale(&accel,&obj->mtype.phys_info.rotthrust,fixdiv(f1_0,obj->mtype.phys_info.mass));

			while (count--) {

				vm_vec_add2(&obj->mtype.phys_info.rotvel,&accel);

				vm_vec_scale(&obj->mtype.phys_info.rotvel,f1_0-drag);
			}

			//do linear scale on remaining bit of time

			vm_vec_scale_add2(&obj->mtype.phys_info.rotvel,&accel,k);
			vm_vec_scale(&obj->mtype.phys_info.rotvel,f1_0-fixmul(k,drag));
		}
		else
#if defined(DXX_BUILD_DESCENT_II)
			if (! (obj->mtype.phys_info.flags & PF_FREE_SPINNING))
#endif
		{
			fix total_drag=f1_0;

			while (count--)
				total_drag = fixmul(total_drag,f1_0-drag);

			//do linear scale on remaining bit of time

			total_drag = fixmul(total_drag,f1_0-fixmul(k,drag));

			vm_vec_scale(&obj->mtype.phys_info.rotvel,total_drag);
		}

	}

	//now rotate object

	//unrotate object for bank caused by turn
	if (obj->mtype.phys_info.turnroll) {
		tangles.p = tangles.h = 0;
		tangles.b = -obj->mtype.phys_info.turnroll;
		vm_angles_2_matrix(&rotmat,&tangles);
		obj->orient = vm_matrix_x_matrix(obj->orient,rotmat);
	}

	tangles.p = fixmul(obj->mtype.phys_info.rotvel.x,FrameTime);
	tangles.h = fixmul(obj->mtype.phys_info.rotvel.y,FrameTime);
	tangles.b = fixmul(obj->mtype.phys_info.rotvel.z,FrameTime);

	vm_angles_2_matrix(&rotmat,&tangles);
	vm_matrix_x_matrix(&new_orient,&obj->orient,&rotmat);
	obj->orient = new_orient;

	if (obj->mtype.phys_info.flags & PF_TURNROLL)
		set_object_turnroll(obj);

	//re-rotate object for bank caused by turn
	if (obj->mtype.phys_info.turnroll) {
		tangles.p = tangles.h = 0;
		tangles.b = obj->mtype.phys_info.turnroll;
		vm_angles_2_matrix(&rotmat,&tangles);
		obj->orient = vm_matrix_x_matrix(obj->orient,rotmat);
	}

	check_and_fix_matrix(&obj->orient);
}

// On joining edges fvi tends to get inaccurate as hell. Approach is to check if the object interects with the wall and if so, move away from it.
static void fix_illegal_wall_intersection(vobjptridx_t obj, vms_vector *origin)
{
	int hside = -1, hface = -1;

	if (!(obj->type == OBJ_PLAYER || obj->type == OBJ_ROBOT))
		return;

	segnum_t hseg = segment_none;
	if ( object_intersects_wall_d(obj,&hseg,&hside,&hface) )
	{
		vm_vec_scale_add2(&obj->pos,&Segments[hseg].sides[hside].normals[0],FrameTime*10);
		update_object_seg(obj);
	}
}

//	-----------------------------------------------------------------------------------------------------------
//Simulate a physics object for this frame
void do_physics_sim(vobjptridx_t obj)
{
	objnum_t ignore_obj_list[MAX_IGNORE_OBJS];
	int n_ignore_objs;
	int try_again;
	int fate=0;
	vms_vector frame_vec;			//movement in this frame
	vms_vector new_pos,ipos;		//position after this frame
	int count=0;
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
	segnum_t orig_segnum = obj->segnum;
	int bounced=0;

	Assert(obj->movement_type == MT_PHYSICS);

#ifndef NDEBUG
	if (Dont_move_ai_objects)
		if (obj->control_type == CT_AI)
			return;
#endif

	pi = &obj->mtype.phys_info;

	do_physics_sim_rot(obj);

	if (!(pi->velocity.x || pi->velocity.y || pi->velocity.z || pi->thrust.x || pi->thrust.y || pi->thrust.z))
		return;

	n_phys_segs = 0;

	sim_time = FrameTime;

	//debug_obj = obj;

#ifdef EXTRA_DEBUG
	//check for correct object segment
	if(!get_seg_masks(&obj->pos, obj->segnum, 0, __FILE__, __LINE__).centermask == 0)
	{
		if (!update_object_seg(obj)) {
			if (!(Game_mode & GM_MULTI))
				Int3();
			compute_segment_center(&obj->pos,&Segments[obj->segnum]);
			obj->pos.x += obj;
		}
	}
#endif

	start_pos = obj->pos;

	n_ignore_objs = 0;

		//if uses thrust, cannot have zero drag
	Assert(!(obj->mtype.phys_info.flags&PF_USES_THRUST) || obj->mtype.phys_info.drag!=0);

	//do thrust & drag
	if ((drag = obj->mtype.phys_info.drag) != 0) {

		int count;
		vms_vector accel;
		fix r,k,have_accel;

		count = FrameTime / FT;
		r = FrameTime % FT;
		k = fixdiv(r,FT);

		if (obj->mtype.phys_info.flags & PF_USES_THRUST) {

			vm_vec_copy_scale(&accel,&obj->mtype.phys_info.thrust,fixdiv(f1_0,obj->mtype.phys_info.mass));
			have_accel = (accel.x || accel.y || accel.z);

			while (count--) {
				if (have_accel)
					vm_vec_add2(&obj->mtype.phys_info.velocity,&accel);

				vm_vec_scale(&obj->mtype.phys_info.velocity,f1_0-drag);
			}

			//do linear scale on remaining bit of time

			vm_vec_scale_add2(&obj->mtype.phys_info.velocity,&accel,k);
			if (drag)
				vm_vec_scale(&obj->mtype.phys_info.velocity,f1_0-fixmul(k,drag));
		}
		else if (drag)
		{
			fix total_drag=f1_0;

			while (count--)
				total_drag = fixmul(total_drag,f1_0-drag);

			//do linear scale on remaining bit of time

			total_drag = fixmul(total_drag,f1_0-fixmul(k,drag));

			vm_vec_scale(&obj->mtype.phys_info.velocity,total_drag);
		}
	}

	do {
		try_again = 0;

		//Move the object
		vm_vec_copy_scale(&frame_vec, &obj->mtype.phys_info.velocity, sim_time);

		if ( (frame_vec.x==0) && (frame_vec.y==0) && (frame_vec.z==0) )	
			break;

		count++;

		//	If retry count is getting large, then we are trying to do something stupid.
		if (count > 8) break; // in original code this was 3 for all non-player objects. still leave us some limit in case fvi goes apeshit.

		vm_vec_add(&new_pos,&obj->pos,&frame_vec);

		ignore_obj_list[n_ignore_objs] = object_none;

		fq.p0						= &obj->pos;
		fq.startseg				= obj->segnum;
		fq.p1						= &new_pos;
		fq.rad					= obj->size;
		fq.thisobjnum			= obj;
		fq.ignore_obj_list	= ignore_obj_list;
		fq.flags					= FQ_CHECK_OBJS;

		if (obj->type == OBJ_WEAPON)
			fq.flags |= FQ_TRANSPOINT;

		if (obj->type == OBJ_PLAYER)
			fq.flags |= FQ_GET_SEGLIST;

		fate = find_vector_intersection(&fq,&hit_info);
		//	Matt: Mike's hack.
		if (fate == HIT_OBJECT) {
			object	*objp = &Objects[hit_info.hit_object];

			if (((objp->type == OBJ_WEAPON) && is_proximity_bomb_or_smart_mine(get_weapon_id(objp))) || objp->type == OBJ_POWERUP) // do not increase count for powerups since they *should* not change our movement
				count--;
		}

#ifndef NDEBUG
		if (fate == HIT_BAD_P0) {
			Int3();
		}
#endif

		if (obj->type == OBJ_PLAYER) {
			if (n_phys_segs && phys_seglist[n_phys_segs-1]==hit_info.seglist[0])
				n_phys_segs--;

			range_for (const auto &hs, hit_info.seglist)
			{
				if (!(n_phys_segs < MAX_FVI_SEGS-1))
					break;
				phys_seglist[n_phys_segs++] = hs;
			}
		}

		ipos = hit_info.hit_pnt;
		segnum_t iseg = hit_info.hit_seg;
		WallHitSide = hit_info.hit_side;
		WallHitSeg = hit_info.hit_side_seg;

		if (iseg==segment_none) {		//some sort of horrible error
			if (obj->type == OBJ_WEAPON)
				obj->flags |= OF_SHOULD_BE_DEAD;
			break;
		}

		Assert(!((fate==HIT_WALL) && ((WallHitSeg == segment_none) || (WallHitSeg > Highest_segment_index))));

		save_pos = obj->pos;			//save the object's position
		segnum_t save_seg = obj->segnum;

		// update object's position and segment number
		obj->pos = ipos;

		if ( iseg != obj->segnum )
			obj_relink(obj, iseg );

		//if start point not in segment, move object to center of segment
		if (get_seg_masks(&obj->pos, obj->segnum, 0, __FILE__, __LINE__).centermask !=0 )
		{
			segnum_t n;

			if ((n=find_object_seg(obj))==segment_none) {
				//Int3();
				if (obj->type==OBJ_PLAYER && (n=find_point_seg(&obj->last_pos,obj->segnum))!=segment_none) {
					obj->pos = obj->last_pos;
					obj_relink(obj, n );
				}
				else {
					compute_segment_center(&obj->pos,&Segments[obj->segnum]);
					obj->pos.x += obj;
				}
				if (obj->type == OBJ_WEAPON)
					obj->flags |= OF_SHOULD_BE_DEAD;
			}
			return;
		}

		//calulate new sim time
		{
			//vms_vector moved_vec;
			vms_vector moved_vec_n;
			fix attempted_dist,actual_dist;

			actual_dist = vm_vec_normalized_dir(&moved_vec_n,&obj->pos,&save_pos);

			if (fate==HIT_WALL && vm_vec_dot(&moved_vec_n,&frame_vec) < 0) {		//moved backwards

				//don't change position or sim_time

				obj->pos = save_pos;
		
				//iseg = obj->segnum;		//don't change segment

				obj_relink(obj, save_seg );

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
				vms_vector moved_v;
				fix hit_speed=0,wall_part=0;

				// Find hit speed	

				vm_vec_sub(&moved_v,&obj->pos,&save_pos);

				wall_part = vm_vec_dot(&moved_v,&hit_info.hit_wallnorm);

				if ((wall_part != 0 && moved_time>0 && (hit_speed=-fixdiv(wall_part,moved_time))>0) || obj->type == OBJ_WEAPON || obj->type == OBJ_DEBRIS)
					collide_object_with_wall( obj, hit_speed, WallHitSeg, WallHitSide, &hit_info.hit_pnt );
				if (obj->type == OBJ_PLAYER)
					scrape_player_on_wall(obj, WallHitSeg, WallHitSide, &hit_info.hit_pnt );

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

						add_stuck_object(obj, WallHitSeg, WallHitSide);

						vm_vec_zero(&obj->mtype.phys_info.velocity);
						obj_stopped = 1;
						try_again = 0;
					}
					else {					// Slide object along wall

						wall_part = vm_vec_dot(&hit_info.hit_wallnorm,&obj->mtype.phys_info.velocity);

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

						vm_vec_scale_add2(&obj->mtype.phys_info.velocity,&hit_info.hit_wallnorm,-wall_part);

#if defined(DXX_BUILD_DESCENT_II)
						if (check_vel) {
							fix vel = vm_vec_mag_quick(obj->mtype.phys_info.velocity);

							if (vel > MAX_OBJECT_VEL)
								vm_vec_scale(&obj->mtype.phys_info.velocity,fixdiv(MAX_OBJECT_VEL,vel));
						}

						if (bounced && obj->type == OBJ_WEAPON)
							vm_vector_2_matrix(&obj->orient,&obj->mtype.phys_info.velocity,&obj->orient.uvec,NULL);
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
				{	vms_vector	*ppos0, *ppos1, pos_hit;
					fix			size0, size1;
					auto hit = vobjptridx(hit_info.hit_object);
					ppos0 = &hit->pos;
					ppos1 = &obj->pos;
					size0 = hit->size;
					size1 = obj->size;
					Assert(size0+size1 != 0);	// Error, both sizes are 0, so how did they collide, anyway?!?
					//vm_vec_scale(vm_vec_sub(&pos_hit, ppos1, ppos0), fixdiv(size0, size0 + size1));
					//vm_vec_add2(&pos_hit, ppos0);
					vm_vec_sub(&pos_hit, ppos1, ppos0);
					vm_vec_scale_add(&pos_hit,ppos0,&pos_hit,fixdiv(size0, size0 + size1));

					old_vel = obj->mtype.phys_info.velocity;

					collide_two_objects( obj, hit, &pos_hit);

				}

				// Let object continue its movement
				if ( !(obj->flags&OF_SHOULD_BE_DEAD)  )	{
					//obj->pos = save_pos;

					if (obj->mtype.phys_info.flags&PF_PERSISTENT || (old_vel.x == obj->mtype.phys_info.velocity.x && old_vel.y == obj->mtype.phys_info.velocity.y && old_vel.z == obj->mtype.phys_info.velocity.z)) {
						//if (Objects[hit_info.hit_object].type == OBJ_POWERUP)
							ignore_obj_list[n_ignore_objs++] = hit_info.hit_object;
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
		vms_vector moved_vec;
		vm_vec_sub(&moved_vec,&obj->pos,&start_pos);
		vm_vec_copy_scale(&obj->mtype.phys_info.velocity,&moved_vec,fixdiv(f1_0,FrameTime));
	}

	fix_illegal_wall_intersection(obj, &start_pos);

	//Assert(check_point_in_seg(&obj->pos,obj->segnum,0).centermask==0);

	//if (obj->control_type == CT_FLYING)
	if (obj->mtype.phys_info.flags & PF_LEVELLING)
		do_physics_align_object( obj );

	//hack to keep player from going through closed doors
	if (obj->type==OBJ_PLAYER && obj->segnum!=orig_segnum && (!cheats.ghostphysics) ) {

		auto sidenum = find_connect_side(&Segments[obj->segnum],&Segments[orig_segnum]);

		if (sidenum != -1) {

			if (! (WALL_IS_DOORWAY(&Segments[orig_segnum],sidenum) & WID_FLY_FLAG)) {
				side *s;
				int num_faces;
				fix dist;
				vertex_array_list_t vertex_list;

				//bump object back

				s = &Segments[orig_segnum].sides[sidenum];

				create_abs_vertex_lists(&num_faces, vertex_list, orig_segnum, sidenum, __FILE__, __LINE__);

				//let's pretend this wall is not triangulated
				auto b = begin(vertex_list);
				auto vertnum = *std::min_element(b, std::next(b, 4));

					dist = vm_dist_to_plane(&start_pos, &s->normals[0], &Vertices[vertnum]);
					vm_vec_scale_add(&obj->pos,&start_pos,&s->normals[0],obj->size-dist);
				update_object_seg(obj);

			}
		}
	}

//--WE ALWYS WANT THIS IN, MATT AND MIKE DECISION ON 12/10/94, TWO MONTHS AFTER FINAL 	#ifndef NDEBUG
	//if end point not in segment, move object to last pos, or segment center
	if (get_seg_masks(&obj->pos, obj->segnum, 0, __FILE__, __LINE__).centermask != 0)
	{
		if (find_object_seg(obj)==segment_none) {
			segnum_t n;

			//Int3();
			if (obj->type==OBJ_PLAYER && (n=find_point_seg(&obj->last_pos,obj->segnum))!=segment_none) {
				obj->pos = obj->last_pos;
				obj_relink(obj, n );
			}
			else {
				compute_segment_center(&obj->pos,&Segments[obj->segnum]);
				obj->pos.x += obj;
			}
			if (obj->type == OBJ_WEAPON)
				obj->flags |= OF_SHOULD_BE_DEAD;
		}
	}
//--WE ALWYS WANT THIS IN, MATT AND MIKE DECISION ON 12/10/94, TWO MONTHS AFTER FINAL 	#endif
}

//Applies an instantaneous force on an object, resulting in an instantaneous
//change in velocity.
void phys_apply_force(object *obj,vms_vector *force_vec)
{
	if (obj->movement_type != MT_PHYSICS)
		return;

	//	Put in by MK on 2/13/96 for force getting applied to Omega blobs, which have 0 mass,
	//	in collision with crazy reactor robot thing on d2levf-s.
	if (obj->mtype.phys_info.mass == 0)
		return;

	//Add in acceleration due to force
	vm_vec_scale_add2(&obj->mtype.phys_info.velocity,force_vec,fixdiv(f1_0,obj->mtype.phys_info.mass));


}

//	----------------------------------------------------------------
//	Do *dest = *delta unless:
//				*delta is pretty small
//		and	they are of different signs.
static void physics_set_rotvel_and_saturate(fix *dest, fix delta)
{
	if ((delta ^ *dest) < 0) {
		if (abs(delta) < F1_0/8) {
			*dest = delta/4;
		} else
			*dest = delta;
	} else {
		*dest = delta;
	}
}

//	------------------------------------------------------------------------------------------------------
//	Note: This is the old ai_turn_towards_vector code.
//	phys_apply_rot used to call ai_turn_towards_vector until I fixed it, which broke phys_apply_rot.
void physics_turn_towards_vector(vms_vector *goal_vector, object *obj, fix rate)
{
	vms_angvec	dest_angles, cur_angles;
	fix			delta_p, delta_h;
	vms_vector	*rotvel_ptr = &obj->mtype.phys_info.rotvel;

	// Make this object turn towards the goal_vector.  Changes orientation, doesn't change direction of movement.
	// If no one moves, will be facing goal_vector in 1 second.

	//	Detect null vector.
	if ((goal_vector->x == 0) && (goal_vector->y == 0) && (goal_vector->z == 0))
		return;

	//	Make morph objects turn more slowly.
	if (obj->control_type == CT_MORPH)
		rate *= 2;

	vm_extract_angles_vector(&dest_angles, goal_vector);
	vm_extract_angles_vector(&cur_angles, &obj->orient.fvec);

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

#ifdef WORDS_NEED_ALIGNMENT
	if ((delta_p ^ rotvel_ptr->x) < 0) {
		if (abs(delta_p) < F1_0/8)
			rotvel_ptr->x = delta_p/4;
		else
			rotvel_ptr->x = delta_p;
	} else
		rotvel_ptr->x = delta_p;
	if ((delta_h ^ rotvel_ptr->y) < 0) {
		if (abs(delta_h) < F1_0/8)
			rotvel_ptr->y = delta_h/4;
		else
			rotvel_ptr->y = delta_h;
	} else
		rotvel_ptr->y = delta_h;

#else
 	physics_set_rotvel_and_saturate(&rotvel_ptr->x, delta_p);
	physics_set_rotvel_and_saturate(&rotvel_ptr->y, delta_h);
#endif
	rotvel_ptr->z = 0;
}

//	-----------------------------------------------------------------------------
//	Applies an instantaneous whack on an object, resulting in an instantaneous
//	change in orientation.
void phys_apply_rot(object *obj,vms_vector *force_vec)
{
	fix	rate, vecmag;

	if (obj->movement_type != MT_PHYSICS)
		return;

	vecmag = vm_vec_mag(*force_vec)/8;
	if (vecmag < F1_0/256)
		rate = 4*F1_0;
	else if (vecmag < obj->mtype.phys_info.mass >> 14)
		rate = 4*F1_0;
	else {
		rate = fixdiv(obj->mtype.phys_info.mass, vecmag);
		if (obj->type == OBJ_ROBOT) {
			if (rate < F1_0/4)
				rate = F1_0/4;
#if defined(DXX_BUILD_DESCENT_I)
			obj->ctype.ai_info.SKIP_AI_COUNT = 2;
#elif defined(DXX_BUILD_DESCENT_II)
			//	Changed by mk, 10/24/95, claw guys should not slow down when attacking!
			if (!Robot_info[get_robot_id(obj)].thief && !Robot_info[get_robot_id(obj)].attack_type) {
				if (obj->ctype.ai_info.SKIP_AI_COUNT * FrameTime < 3*F1_0/4) {
					fix	tval = fixdiv(F1_0, 8*FrameTime);
					int	addval;

					addval = f2i(tval);

					if ( (d_rand() * 2) < (tval & 0xffff))
						addval++;
					obj->ctype.ai_info.SKIP_AI_COUNT += addval;
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


//this routine will set the thrust for an object to a value that will
//(hopefully) maintain the object's current velocity
void set_thrust_from_velocity(object *obj)
{
	fix k;

	Assert(obj->movement_type == MT_PHYSICS);

	k = fixmuldiv(obj->mtype.phys_info.mass,obj->mtype.phys_info.drag,(f1_0-obj->mtype.phys_info.drag));

	vm_vec_copy_scale(&obj->mtype.phys_info.thrust,&obj->mtype.phys_info.velocity,k);

}
