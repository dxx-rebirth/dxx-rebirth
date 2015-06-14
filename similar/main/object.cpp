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
 * object rendering
 *
 */

#include <algorithm>
#include <stdio.h>

#include "inferno.h"
#include "game.h"
#include "gr.h"
#include "stdlib.h"
#include "bm.h"
#include "3d.h"
#include "segment.h"
#include "texmap.h"
#include "laser.h"
#include "key.h"
#include "gameseg.h"
#include "textures.h"
#include "byteutil.h"
#include "object.h"
#include "controls.h"
#include "physics.h"
#include "slew.h"
#include "render.h"
#include "wall.h"
#include "vclip.h"
#include "robot.h"
#include "interp.h"
#include "fireball.h"
#include "laser.h"
#include "dxxerror.h"
#include "ai.h"
#include "hostage.h"
#include "morph.h"
#include "cntrlcen.h"
#include "powerup.h"
#include "fuelcen.h"
#include "endlevel.h"
#include "hudmsg.h"
#include "sounds.h"
#include "collide.h"
#include "lighting.h"
#include "newdemo.h"
#include "player.h"
#include "weapon.h"
#include "newmenu.h"
#include "gauges.h"
#include "multi.h"
#include "menu.h"
#include "args.h"
#include "text.h"
#include "piggy.h"
#include "switch.h"
#include "gameseq.h"
#include "playsave.h"
#include "timer.h"
#ifdef EDITOR
#include "editor/editor.h"
#endif

#include "compiler-exchange.h"
#include "compiler-range_for.h"
#include "highest_valid.h"
#include "partial_range.h"
#include "poison.h"

using std::min;
using std::max;

static void obj_detach_all(const vobjptr_t parent);
static void obj_detach_one(const vobjptridx_t sub);

/*
 *  Global variables
 */

object *ConsoleObject;					//the object that is the player

static array<objnum_t, MAX_OBJECTS> free_obj_list;

//Data for objects

// -- Object stuff

//info on the various types of objects
#ifndef NDEBUG
object	Object_minus_one;
#endif

object_array_t Objects;
int num_objects=0;
int Highest_ever_object_index=0;

// grs_bitmap *robot_bms[MAX_ROBOT_BITMAPS];	//all bitmaps for all robots

// int robot_bm_nums[MAX_ROBOT_TYPES];		//starting bitmap num for each robot
// int robot_n_bitmaps[MAX_ROBOT_TYPES];		//how many bitmaps for each robot

// char *robot_names[MAX_ROBOT_TYPES];		//name of each robot

//--unused-- int Num_robot_types=0;

int print_object_info = 0;
//@@int Object_viewer = 0;

//object * Slew_object = NULL;	// Object containing slew object info.

//--unused-- int Player_controller_type = 0;

#ifndef RELEASE
//set viewer object to next object in array
void object_goto_next_viewer()
{
	int start_obj = 0;

	start_obj = Viewer - Objects;		//get viewer object number
	
	range_for (const auto i, highest_valid(Objects))
	{
		(void)i;
		start_obj++;
		if (start_obj > Highest_object_index ) start_obj = 0;

		if (Objects[start_obj].type != OBJ_NONE )	{
			Viewer = &Objects[start_obj];
			return;
		}
	}

	Error( "Couldn't find a viewer object!" );

}
#endif

object_array_t::object_array_t()
{
	DXX_MAKE_MEM_UNDEFINED(begin(), end());
	range_for (auto &o, *this)
		o.type = OBJ_NONE;
}

objptridx_t obj_find_first_of_type(int type)
{
	range_for (const auto o, highest_valid(Objects))
	{
		const auto i = vobjptridx(o);
		if (i->type==type)
			return i;
	}
	return object_none;
}

//draw an object that has one bitmap & doesn't rotate
void draw_object_blob(const vobjptr_t obj,bitmap_index bmi)
{
	grs_bitmap * bm = &GameBitmaps[bmi.index];
	vms_vector pos = obj->pos;

	PIGGY_PAGE_IN( bmi );

	// draw these with slight offset to viewer preventing too much ugly clipping
	if ( obj->type == OBJ_FIREBALL && obj->id == VCLIP_VOLATILE_WALL_HIT )
	{
		vms_vector offs_vec;
		vm_vec_normalized_dir_quick(offs_vec,Viewer->pos,obj->pos);
		vm_vec_scale_add2(pos,offs_vec,F1_0);
	}

	if (bm->bm_w > bm->bm_h)
		g3_draw_bitmap(pos,obj->size,fixmuldiv(obj->size,bm->bm_h,bm->bm_w),*bm);
	else
		g3_draw_bitmap(pos,fixmuldiv(obj->size,bm->bm_w,bm->bm_h),obj->size,*bm);
}

//draw an object that is a texture-mapped rod
void draw_object_tmap_rod(const vobjptridx_t obj,const bitmap_index bitmapi,int lighted)
{
	g3s_lrgb light;
	PIGGY_PAGE_IN(bitmapi);

	auto &bitmap = GameBitmaps[bitmapi.index];

	const auto delta = vm_vec_copy_scale(obj->orient.uvec,obj->size);

	const auto top_v = vm_vec_add(obj->pos,delta);
	const auto bot_v = vm_vec_sub(obj->pos,delta);

	const auto top_p = g3_rotate_point(top_v);
	const auto bot_p = g3_rotate_point(bot_v);

	if (lighted)
	{
		light = compute_object_light(obj,&top_p.p3_vec);
	}
	else
	{
		light.r = light.g = light.b = f1_0;
	}

	g3_draw_rod_tmap(bitmap,bot_p,obj->size,top_p,obj->size,light);
}

int	Linear_tmap_polygon_objects = 1;

//used for robot engine glow
#define MAX_VELOCITY i2f(50)

//what darkening level to use when cloaked
#define CLOAKED_FADE_LEVEL		28

#define	CLOAK_FADEIN_DURATION_PLAYER	F2_0
#define	CLOAK_FADEOUT_DURATION_PLAYER	F2_0

#define	CLOAK_FADEIN_DURATION_ROBOT	F1_0
#define	CLOAK_FADEOUT_DURATION_ROBOT	F1_0

//do special cloaked render
static void draw_cloaked_object(const vcobjptr_t obj,g3s_lrgb light,glow_values_t &glow,fix64 cloak_start_time,fix64 cloak_end_time)
{
	fix cloak_delta_time,total_cloaked_time;
	fix light_scale=F1_0;
	int cloak_value=0;
	int fading=0;		//if true, fading, else cloaking
	fix	Cloak_fadein_duration=F1_0;
	fix	Cloak_fadeout_duration=F1_0;


	total_cloaked_time = cloak_end_time-cloak_start_time;

	switch (obj->type) {
		case OBJ_PLAYER:
			Cloak_fadein_duration = CLOAK_FADEIN_DURATION_PLAYER;
			Cloak_fadeout_duration = CLOAK_FADEOUT_DURATION_PLAYER;
			break;
		case OBJ_ROBOT:
			Cloak_fadein_duration = CLOAK_FADEIN_DURATION_ROBOT;
			Cloak_fadeout_duration = CLOAK_FADEOUT_DURATION_ROBOT;
			break;
		default:
			Int3();		//	Contact Mike: Unexpected object type in draw_cloaked_object.
	}

	cloak_delta_time = GameTime64 - cloak_start_time;

	if (cloak_delta_time < Cloak_fadein_duration/2) {

#if defined(DXX_BUILD_DESCENT_I)
		light_scale = Cloak_fadein_duration/2 - cloak_delta_time;
#elif defined(DXX_BUILD_DESCENT_II)
		light_scale = fixdiv(Cloak_fadein_duration/2 - cloak_delta_time,Cloak_fadein_duration/2);
#endif
		fading = 1;

	}
	else if (cloak_delta_time < Cloak_fadein_duration) {

#if defined(DXX_BUILD_DESCENT_I)
		cloak_value = f2i((cloak_delta_time - Cloak_fadein_duration/2) * CLOAKED_FADE_LEVEL);
#elif defined(DXX_BUILD_DESCENT_II)
		cloak_value = f2i(fixdiv(cloak_delta_time - Cloak_fadein_duration/2,Cloak_fadein_duration/2) * CLOAKED_FADE_LEVEL);
#endif

	} else if (GameTime64 < cloak_end_time-Cloak_fadeout_duration) {
		static int cloak_delta=0,cloak_dir=1;
		static fix cloak_timer=0;

		//note, if more than one cloaked object is visible at once, the
		//pulse rate will change!

		cloak_timer -= FrameTime;
		while (cloak_timer < 0) {

			cloak_timer += Cloak_fadeout_duration/12;

			cloak_delta += cloak_dir;

			if (cloak_delta==0 || cloak_delta==4)
				cloak_dir = -cloak_dir;
		}

		cloak_value = CLOAKED_FADE_LEVEL - cloak_delta;
	
	} else if (GameTime64 < cloak_end_time-Cloak_fadeout_duration/2) {

#if defined(DXX_BUILD_DESCENT_I)
		cloak_value = f2i((total_cloaked_time - Cloak_fadeout_duration/2 - cloak_delta_time) * CLOAKED_FADE_LEVEL);
#elif defined(DXX_BUILD_DESCENT_II)
		cloak_value = f2i(fixdiv(total_cloaked_time - Cloak_fadeout_duration/2 - cloak_delta_time,Cloak_fadeout_duration/2) * CLOAKED_FADE_LEVEL);
#endif

	} else {

#if defined(DXX_BUILD_DESCENT_I)
		light_scale = Cloak_fadeout_duration/2 - (total_cloaked_time - cloak_delta_time);
#elif defined(DXX_BUILD_DESCENT_II)
		light_scale = fixdiv(Cloak_fadeout_duration/2 - (total_cloaked_time - cloak_delta_time),Cloak_fadeout_duration/2);
#endif
		fading = 1;
	}

	alternate_textures alt_textures;
#if defined(DXX_BUILD_DESCENT_II)
	if (fading)
#endif
	{
		if ( obj->rtype.pobj_info.alt_textures > 0 )
		{
			alt_textures = multi_player_textures[obj->rtype.pobj_info.alt_textures-1];
		}
	}

	if (fading) {
		fix save_glow;
		g3s_lrgb new_light;

		new_light.r = fixmul(light.r,light_scale);
		new_light.g = fixmul(light.g,light_scale);
		new_light.b = fixmul(light.b,light_scale);
		save_glow = glow[0];
		glow[0] = fixmul(glow[0],light_scale);
		draw_polygon_model(obj->pos,
				   &obj->orient,
				   obj->rtype.pobj_info.anim_angles,
				   obj->rtype.pobj_info.model_num,obj->rtype.pobj_info.subobj_flags,
				   new_light,
				   &glow,
				   alt_textures );
		glow[0] = save_glow;
	}
	else {
		gr_settransblend(cloak_value, GR_BLEND_NORMAL);
		gr_setcolor(BM_XRGB(0,0,0));	//set to black (matters for s3)
		g3_set_special_render(draw_tmap_flat);		//use special flat drawer
		draw_polygon_model(obj->pos,
				   &obj->orient,
				   obj->rtype.pobj_info.anim_angles,
				   obj->rtype.pobj_info.model_num,obj->rtype.pobj_info.subobj_flags,
				   light,
				   &glow,
				   alt_textures );
		g3_set_special_render(draw_tmap);
		gr_settransblend(GR_FADE_OFF, GR_BLEND_NORMAL);
	}

}

//draw an object which renders as a polygon model
static void draw_polygon_object(const vobjptridx_t obj)
{
	g3s_lrgb light;
	int	imsave;
	glow_values_t engine_glow_value;
	engine_glow_value[0] = 0;
#if defined(DXX_BUILD_DESCENT_II)
	engine_glow_value[1] = -1;		//element 0 is for engine glow, 1 for headlight
#endif

	light = compute_object_light(obj,nullptr);

	//	If option set for bright players in netgame, brighten them!
	if ((Game_mode & GM_MULTI) && (obj->type == OBJ_PLAYER))
		if (Netgame.BrightPlayers)
			light.r = light.g = light.b = F1_0*2;

#if defined(DXX_BUILD_DESCENT_II)
	//make robots brighter according to robot glow field
	if (obj->type == OBJ_ROBOT)
	{
		light.r += (Robot_info[get_robot_id(obj)].glow<<12); //convert 4:4 to 16:16
		light.g += (Robot_info[get_robot_id(obj)].glow<<12); //convert 4:4 to 16:16
		light.b += (Robot_info[get_robot_id(obj)].glow<<12); //convert 4:4 to 16:16
	}

	if (obj->type == OBJ_WEAPON)
	{
		if (get_weapon_id(obj) == FLARE_ID)
		{
			light.r += F1_0*2;
			light.g += F1_0*2;
			light.b += F1_0*2;
		}
	}

	if (obj->type == OBJ_MARKER)
	{
 		light.r += F1_0*2;
		light.g += F1_0*2;
		light.b += F1_0*2;
	}
#endif

	imsave = Interpolation_method;
	if (Linear_tmap_polygon_objects)
		Interpolation_method = 1;

	//set engine glow value
	engine_glow_value[0] = f1_0/5;
	if (obj->movement_type == MT_PHYSICS) {

		if (obj->mtype.phys_info.flags & PF_USES_THRUST && obj->type==OBJ_PLAYER && get_player_id(obj)==Player_num) {
			fix thrust_mag = vm_vec_mag_quick(obj->mtype.phys_info.thrust);
			engine_glow_value[0] += (fixdiv(thrust_mag,Player_ship->max_thrust)*4)/5;
		}
		else {
			fix speed = vm_vec_mag_quick(obj->mtype.phys_info.velocity);
#if defined(DXX_BUILD_DESCENT_I)
			engine_glow_value[0] += (fixdiv(speed,MAX_VELOCITY)*4)/5;
#elif defined(DXX_BUILD_DESCENT_II)
			engine_glow_value[0] += (fixdiv(speed,MAX_VELOCITY)*3)/5;
#endif
		}
	}

#if defined(DXX_BUILD_DESCENT_II)
	//set value for player headlight
	if (obj->type == OBJ_PLAYER) {
		if (Players[get_player_id(obj)].flags & PLAYER_FLAGS_HEADLIGHT && !Endlevel_sequence)
			if (Players[get_player_id(obj)].flags & PLAYER_FLAGS_HEADLIGHT_ON)
				engine_glow_value[1] = -2;		//draw white!
			else
				engine_glow_value[1] = -1;		//draw normal color (grey)
		else
			engine_glow_value[1] = -3;			//don't draw
	}
#endif

	if (obj->rtype.pobj_info.tmap_override != -1) {
#ifndef NDEBUG
		polymodel *pm = &Polygon_models[obj->rtype.pobj_info.model_num];
#endif
		array<bitmap_index, 12> bm_ptrs;
		Assert(pm->n_textures<=12);

		//fill whole array, in case simple model needs more
		bm_ptrs.fill(Textures[obj->rtype.pobj_info.tmap_override]);
		draw_polygon_model(obj->pos,
				   &obj->orient,
				   obj->rtype.pobj_info.anim_angles,
				   obj->rtype.pobj_info.model_num,
				   obj->rtype.pobj_info.subobj_flags,
				   light,
				   &engine_glow_value,
				   bm_ptrs);
	}
	else {

		if (obj->type==OBJ_PLAYER && (Players[get_player_id(obj)].flags&PLAYER_FLAGS_CLOAKED))
			draw_cloaked_object(obj,light,engine_glow_value,Players[get_player_id(obj)].cloak_time,Players[get_player_id(obj)].cloak_time+CLOAK_TIME_MAX);
		else if ((obj->type == OBJ_ROBOT) && (obj->ctype.ai_info.CLOAKED)) {
			if (Robot_info[get_robot_id(obj)].boss_flag)
				draw_cloaked_object(obj,light,engine_glow_value, Boss_cloak_start_time, Boss_cloak_end_time);
			else
				draw_cloaked_object(obj,light,engine_glow_value, GameTime64-F1_0*10, GameTime64+F1_0*10);
		} else {
			alternate_textures alt_textures;
			if ( obj->rtype.pobj_info.alt_textures > 0 )
				alt_textures = multi_player_textures[obj->rtype.pobj_info.alt_textures-1];

#if defined(DXX_BUILD_DESCENT_II)
			//	Snipers get bright when they fire.
			ai_local		*ailp = &obj->ctype.ai_info.ail;
			if (ailp->next_fire < F1_0/8) {
				if (obj->ctype.ai_info.behavior == ai_behavior::AIB_SNIPE)
				{
					light.r = 2*light.r + F1_0;
					light.g = 2*light.g + F1_0;
					light.b = 2*light.b + F1_0;
				}
			}
#endif

			if (obj->type == OBJ_WEAPON && (Weapon_info[get_weapon_id(obj)].model_num_inner > -1 )) {
				fix dist_to_eye = vm_vec_dist_quick(Viewer->pos, obj->pos);
				gr_settransblend(GR_FADE_OFF, GR_BLEND_ADDITIVE_A);
				if (dist_to_eye < Simple_model_threshhold_scale * F1_0*2)
					draw_polygon_model(obj->pos,
							   &obj->orient,
							   obj->rtype.pobj_info.anim_angles,
							   Weapon_info[get_weapon_id(obj)].model_num_inner,
							   obj->rtype.pobj_info.subobj_flags,
							   light,
							   &engine_glow_value,
							   alt_textures);
			}
			
			draw_polygon_model(obj->pos,
					   &obj->orient,
					   obj->rtype.pobj_info.anim_angles,obj->rtype.pobj_info.model_num,
					   obj->rtype.pobj_info.subobj_flags,
					   light,
					   &engine_glow_value,
					   alt_textures);

#ifndef OGL // in software rendering must draw inner model last
			if (obj->type == OBJ_WEAPON && (Weapon_info[obj->id].model_num_inner > -1 )) {
				fix dist_to_eye = vm_vec_dist_quick(Viewer->pos, obj->pos);
				gr_settransblend(GR_FADE_OFF, GR_BLEND_ADDITIVE_A);
				if (dist_to_eye < Simple_model_threshhold_scale * F1_0*2)
					draw_polygon_model(obj->pos,
							   &obj->orient,
							   obj->rtype.pobj_info.anim_angles,
							   Weapon_info[obj->id].model_num_inner,
							   obj->rtype.pobj_info.subobj_flags,
							   light,
							   &engine_glow_value,
							   alt_textures);
			}
#endif

			if (obj->type == OBJ_WEAPON && (Weapon_info[get_weapon_id(obj)].model_num_inner > -1 ))
				gr_settransblend(GR_FADE_OFF, GR_BLEND_NORMAL);
		}
	}

	Interpolation_method = imsave;

}

//------------------------------------------------------------------------------
// These variables are used to keep a list of the 3 closest robots to the viewer.
// The code works like this: Every time render object is called with a polygon model,
// it finds the distance of that robot to the viewer.  If this distance if within 10
// segments of the viewer, it does the following: If there aren't already 3 robots in
// the closet-robots list, it just sticks that object into the list along with its distance.
// If the list already contains 3 robots, then it finds the robot in that list that is
// farthest from the viewer. If that object is farther than the object currently being
// rendered, then the new object takes over that far object's slot.  *Then* after all
// objects are rendered, object_render_targets is called an it draws a target on top
// of all the objects.

//091494: #define MAX_CLOSE_ROBOTS 3
//--unused-- static int Object_draw_lock_boxes = 0;
//091494: static int Object_num_close = 0;
//091494: static object * Object_close_ones[MAX_CLOSE_ROBOTS];
//091494: static fix Object_close_distance[MAX_CLOSE_ROBOTS];

//091494: set_close_objects(object *obj)
//091494: {
//091494: 	fix dist;
//091494:
//091494: 	if ( (obj->type != OBJ_ROBOT) || (Object_draw_lock_boxes==0) )	
//091494: 		return;
//091494:
//091494: 	// The following code keeps a list of the 10 closest robots to the
//091494: 	// viewer.  See comments in front of this function for how this works.
//091494: 	dist = vm_vec_dist( &obj->pos, &Viewer->pos );
//091494: 	if ( dist < i2f(20*10) )	{				
//091494: 		if ( Object_num_close < MAX_CLOSE_ROBOTS )	{
//091494: 			Object_close_ones[Object_num_close] = obj;
//091494: 			Object_close_distance[Object_num_close] = dist;
//091494: 			Object_num_close++;
//091494: 		} else {
//091494: 			int i, farthest_robot;
//091494: 			fix farthest_distance;
//091494: 			// Find the farthest robot in the list
//091494: 			farthest_robot = 0;
//091494: 			farthest_distance = Object_close_distance[0];
//091494: 			for (i=1; i<Object_num_close; i++ )	{
//091494: 				if ( Object_close_distance[i] > farthest_distance )	{
//091494: 					farthest_distance = Object_close_distance[i];
//091494: 					farthest_robot = i;
//091494: 				}
//091494: 			}
//091494: 			// If this object is closer to the viewer than
//091494: 			// the farthest in the list, replace the farthest with this object.
//091494: 			if ( farthest_distance > dist )	{
//091494: 				Object_close_ones[farthest_robot] = obj;
//091494: 				Object_close_distance[farthest_robot] = dist;
//091494: 			}
//091494: 		}
//091494: 	}
//091494: }

objnum_t	Player_fired_laser_this_frame=object_none;



// -----------------------------------------------------------------------------
//this routine checks to see if an robot rendered near the middle of
//the screen, and if so and the player had fired, "warns" the robot
static void set_robot_location_info(const vobjptr_t objp)
{
	if (Player_fired_laser_this_frame != object_none) {
		const auto temp = g3_rotate_point(objp->pos);
		if (temp.p3_codes & CC_BEHIND)		//robot behind the screen
			return;

		//the code below to check for object near the center of the screen
		//completely ignores z, which may not be good

		if ((abs(temp.p3_x) < F1_0*4) && (abs(temp.p3_y) < F1_0*4)) {
			objp->ctype.ai_info.danger_laser_num = Player_fired_laser_this_frame;
			objp->ctype.ai_info.danger_laser_signature = Objects[Player_fired_laser_this_frame].signature;
		}
	}


}

//	------------------------------------------------------------------------------------------------------------------
void create_small_fireball_on_object(const vobjptridx_t objp, fix size_scale, int sound_flag)
{
	fix			size;
	vms_vector	pos;

	pos = objp->pos;
	auto rand_vec = make_random_vector();

	vm_vec_scale(rand_vec, objp->size/2);

	vm_vec_add2(pos, rand_vec);

#if defined(DXX_BUILD_DESCENT_I)
	size = fixmul(size_scale, F1_0 + d_rand()*4);
#elif defined(DXX_BUILD_DESCENT_II)
	size = fixmul(size_scale, F1_0/2 + d_rand()*4/2);
#endif

	auto segnum = find_point_seg(pos, objp->segnum);
	if (segnum != segment_none) {
		auto expl_obj = object_create_explosion(segnum, pos, size, VCLIP_SMALL_EXPLOSION);
		if (!expl_obj)
			return;
		obj_attach(objp,expl_obj);
		if (d_rand() < 8192) {
			fix	vol = F1_0/2;
			if (objp->type == OBJ_ROBOT)
				vol *= 2;
			if (sound_flag)
				digi_link_sound_to_object(SOUND_EXPLODING_WALL, objp, 0, vol);
		}
	}
}

// -- mk, 02/05/95 -- #define	VCLIP_INVULNERABILITY_EFFECT	VCLIP_SMALL_EXPLOSION
// -- mk, 02/05/95 --
// -- mk, 02/05/95 -- // -----------------------------------------------------------------------------
// -- mk, 02/05/95 -- void do_player_invulnerability_effect(object *objp)
// -- mk, 02/05/95 -- {
// -- mk, 02/05/95 -- 	if (d_rand() < FrameTime*8) {
// -- mk, 02/05/95 -- 		create_vclip_on_object(objp, F1_0, VCLIP_INVULNERABILITY_EFFECT);
// -- mk, 02/05/95 -- 	}
// -- mk, 02/05/95 -- }

// -----------------------------------------------------------------------------
//	Render an object.  Calls one of several routines based on type
void render_object(const vobjptridx_t obj)
{

	if ( obj == Viewer )
		return;

	if ( obj->type==OBJ_NONE )
	{
		Int3();
		return;
	}

#ifndef OGL
	const auto mld_save = exchange(Max_linear_depth, Max_linear_depth_objects);
#endif

	switch (obj->render_type)
	{
		case RT_NONE:
			break; //doesn't render, like the player

		case RT_POLYOBJ:
#if defined(DXX_BUILD_DESCENT_II)
			if ( PlayerCfg.AlphaEffects && obj->type == OBJ_MARKER ) // set nice transparency/blending for certrain objects
				gr_settransblend( 10, GR_BLEND_ADDITIVE_A );
#endif
			draw_polygon_object(obj);

			if (obj->type == OBJ_ROBOT) //"warn" robot if being shot at
				set_robot_location_info(obj);
			break;

		case RT_MORPH:
			draw_morph_object(obj);
			break;

		case RT_FIREBALL:
			if ( PlayerCfg.AlphaEffects ) // set nice transparency/blending for certrain objects
				gr_settransblend( GR_FADE_OFF, GR_BLEND_ADDITIVE_C );

			draw_fireball(obj);
			break;

		case RT_WEAPON_VCLIP:
			if ( PlayerCfg.AlphaEffects && !is_proximity_bomb_or_smart_mine(get_weapon_id(obj))) // set nice transparency/blending for certain objects
				gr_settransblend( 7, GR_BLEND_ADDITIVE_A );

			draw_weapon_vclip(obj);
			break;

		case RT_HOSTAGE:
			draw_hostage(obj);
			break;

		case RT_POWERUP:
			if ( PlayerCfg.AlphaEffects ) // set nice transparency/blending for certrain objects
				switch ( get_powerup_id(obj) )
				{
					case POW_EXTRA_LIFE:
					case POW_ENERGY:
					case POW_SHIELD_BOOST:
					case POW_CLOAK:
					case POW_INVULNERABILITY:
#if defined(DXX_BUILD_DESCENT_II)
					case POW_HOARD_ORB:
#endif
						gr_settransblend( 7, GR_BLEND_ADDITIVE_A );
						break;
					case POW_LASER:
					case POW_KEY_BLUE:
					case POW_KEY_RED:
					case POW_KEY_GOLD:
					case POW_MISSILE_1:
					case POW_MISSILE_4:
					case POW_QUAD_FIRE:
					case POW_VULCAN_WEAPON:
					case POW_SPREADFIRE_WEAPON:
					case POW_PLASMA_WEAPON:
					case POW_FUSION_WEAPON:
					case POW_PROXIMITY_WEAPON:
					case POW_HOMING_AMMO_1:
					case POW_HOMING_AMMO_4:
					case POW_SMARTBOMB_WEAPON:
					case POW_MEGA_WEAPON:
					case POW_VULCAN_AMMO:
					case POW_TURBO:
					case POW_MEGAWOW:
#if defined(DXX_BUILD_DESCENT_II)
					case POW_FULL_MAP:
					case POW_HEADLIGHT:
					case POW_GAUSS_WEAPON:
					case POW_HELIX_WEAPON:
					case POW_PHOENIX_WEAPON:
					case POW_OMEGA_WEAPON:
					case POW_SUPER_LASER:
					case POW_CONVERTER:
					case POW_AMMO_RACK:
					case POW_AFTERBURNER:
					case POW_SMISSILE1_1:
					case POW_SMISSILE1_4:
					case POW_GUIDED_MISSILE_1:
					case POW_GUIDED_MISSILE_4:
					case POW_SMART_MINE:
					case POW_MERCURY_MISSILE_1:
					case POW_MERCURY_MISSILE_4:
					case POW_EARTHSHAKER_MISSILE:
					case POW_FLAG_BLUE:
					case POW_FLAG_RED:
#endif
						break;
				}

			draw_powerup(obj);
			break;

		case RT_LASER:
			if ( PlayerCfg.AlphaEffects ) // set nice transparency/blending for certrain objects
				gr_settransblend( 7, GR_BLEND_ADDITIVE_A );

			Laser_render(obj);
			break;

		default:
			Error("Unknown render_type <%d>",obj->render_type);
	}

	gr_settransblend( GR_FADE_OFF, GR_BLEND_NORMAL ); // revert any transparency/blending setting back to normal

	if ( obj->render_type != RT_NONE && Newdemo_state == ND_STATE_RECORDING )
		newdemo_record_render_object(obj);
#ifndef OGL
	Max_linear_depth = mld_save;
#endif
}

#define vm_angvec_zero(v) (v)->p=(v)->b=(v)->h=0

void reset_player_object()
{
	//Init physics

	vm_vec_zero(ConsoleObject->mtype.phys_info.velocity);
	vm_vec_zero(ConsoleObject->mtype.phys_info.thrust);
	vm_vec_zero(ConsoleObject->mtype.phys_info.rotvel);
	vm_vec_zero(ConsoleObject->mtype.phys_info.rotthrust);
	ConsoleObject->mtype.phys_info.turnroll = 0;
	ConsoleObject->mtype.phys_info.mass = Player_ship->mass;
	ConsoleObject->mtype.phys_info.drag = Player_ship->drag;
	ConsoleObject->mtype.phys_info.flags |= PF_TURNROLL | PF_LEVELLING | PF_WIGGLE | PF_USES_THRUST;

	//Init render info

	ConsoleObject->render_type = RT_POLYOBJ;
	ConsoleObject->rtype.pobj_info.model_num = Player_ship->model_num;		//what model is this?
	ConsoleObject->rtype.pobj_info.subobj_flags = 0;		//zero the flags
	ConsoleObject->rtype.pobj_info.tmap_override = -1;		//no tmap override!

	range_for (auto &i, ConsoleObject->rtype.pobj_info.anim_angles)
		vm_angvec_zero(&i);

	// Clear misc

	ConsoleObject->flags = 0;

}


//make object0 the player, setting all relevant fields
void init_player_object()
{
	ConsoleObject->type = OBJ_PLAYER;
	set_player_id(ConsoleObject, 0);					//no sub-types for player

#if defined(DXX_BUILD_DESCENT_II)
	ConsoleObject->signature = object_signature_t{0};			//player has zero, others start at 1
#endif
	ConsoleObject->size = Polygon_models[Player_ship->model_num].rad;

	ConsoleObject->control_type = CT_SLEW;			//default is player slewing
	ConsoleObject->movement_type = MT_PHYSICS;		//change this sometime

	ConsoleObject->lifeleft = IMMORTAL_TIME;

	ConsoleObject->attached_obj = object_none;

	reset_player_object();

}

//sets up the free list & init player & whatever else
void init_objects()
{
	DXX_MAKE_MEM_UNDEFINED(Objects.begin(), Objects.end());
	for (int i=0;i<MAX_OBJECTS;i++) {
		free_obj_list[i] = i;
		Objects[i].type = OBJ_NONE;
		Objects[i].segnum = segment_none;
	}

	range_for (auto &j, Segments)
		j.objects = object_none;

	ConsoleObject = Viewer = &Objects[0];

	init_player_object();
	obj_link(vobjptridx(ConsoleObject),segment_first);	//put in the world in segment 0
	num_objects = 1;						//just the player
	Highest_object_index = 0;
}

//after calling init_object(), the network code has grabbed specific
//object slots without allocating them.  Go though the objects & build
//the free list, then set the apporpriate globals
void special_reset_objects(void)
{
	num_objects=MAX_OBJECTS;

	Highest_object_index = 0;
	Assert(Objects[0].type != OBJ_NONE);		//0 should be used

	DXX_MAKE_MEM_UNDEFINED(free_obj_list.begin(), free_obj_list.end());
	for (int i=MAX_OBJECTS;i--;)
		if (Objects[i].type == OBJ_NONE)
			free_obj_list[--num_objects] = i;
		else
			if (i > Highest_object_index)
				Highest_object_index = i;
}

//link the object into the list for its segment
void obj_link(const vobjptridx_t obj,const vsegptridx_t segnum)
{
	Assert(obj->segnum == segment_none);
	obj->segnum = segnum;
	
	obj->next = segnum->objects;
	obj->prev = object_none;

	segnum->objects = obj;

	if (obj->next != object_none) Objects[obj->next].prev = obj;
	
	//list_seg_objects( segnum );
	//check_duplicate_objects();

	Assert(Objects[0].next != 0);
	if (Objects[0].next == 0)
		Objects[0].next = object_none;

	Assert(Objects[0].prev != 0);
	if (Objects[0].prev == 0)
		Objects[0].prev = object_none;
}

void obj_unlink(const vobjptridx_t obj)
{
	segment *seg = &Segments[obj->segnum];
	if (obj->prev == object_none)
		seg->objects = obj->next;
	else
		Objects[obj->prev].next = obj->next;

	if (obj->next != object_none) Objects[obj->next].prev = obj->prev;

	obj->segnum = segment_none;

	Assert(Objects[0].next != 0);
	Assert(Objects[0].prev != 0);
}

// Returns a new, unique signature for a new object
object_signature_t obj_get_signature()
{
	static short sig = 0; // Yes! Short! a) We do not need higher values b) the demo system only stores shorts
	uint_fast32_t lsig = sig;
	for (const auto &range = highest_valid(Objects);;)
	{
		if (unlikely(lsig == std::numeric_limits<decltype(sig)>::max()))
			lsig = 0;
		++ lsig;
		const auto predicate = [lsig](const vcobjptridx_t &o) {
			return o->type != OBJ_NONE && o->signature.get() == lsig;
		};
		if (std::find_if(range.begin(), range.end(), predicate) != range.end())
			continue;
		sig = static_cast<int16_t>(lsig);
		return object_signature_t{static_cast<uint16_t>(lsig)};
	}
}

static unsigned Debris_object_count;
static int Unused_object_slots;

//returns the number of a free object, updating Highest_object_index.
//Generally, obj_create() should be called to get an object, since it
//fills in important fields and does the linking.
//returns -1 if no free objects
objptridx_t obj_allocate()
{
	if ( num_objects >= MAX_OBJECTS ) {
		return object_none;
	}

	auto objnum = free_obj_list[num_objects++];

	if (objnum > Highest_object_index) {
		Highest_object_index = objnum;
		if (Highest_object_index > Highest_ever_object_index)
			Highest_ever_object_index = Highest_object_index;
	}

{
Unused_object_slots=0;
	range_for (const auto i, highest_valid(Objects))
	{
		const auto &&objp = vcobjptr(static_cast<objnum_t>(i));
		if (objp->type == OBJ_NONE)
		Unused_object_slots++;
	}
}
	return objptridx(objnum);
}

//frees up an object.  Generally, obj_delete() should be called to get
//rid of an object.  This function deallocates the object entry after
//the object has been unlinked
static void obj_free(objnum_t objnum)
{
	free_obj_list[--num_objects] = objnum;
	Assert(num_objects >= 0);

	if (objnum == Highest_object_index)
	{
		int o = Highest_object_index;
		for (;;)
		{
			--o;
			if (Objects[o].type != OBJ_NONE)
				break;
			if (o == 0)
				break;
		}
		Highest_object_index = o;
	}
}

//-----------------------------------------------------------------------------
//	Scan the object list, freeing down to num_used objects
//	Returns number of slots freed.
static void free_object_slots(uint_fast32_t num_used)
{
	array<object *, MAX_OBJECTS>	obj_list;
	unsigned	num_already_free, num_to_free, olind = 0;

	num_already_free = MAX_OBJECTS - Highest_object_index - 1;

	if (MAX_OBJECTS - num_already_free < num_used)
		return;

	range_for (const auto i, highest_valid(Objects))
	{
		const auto &&objp = vobjptr(static_cast<objnum_t>(i));
		if (objp->flags & OF_SHOULD_BE_DEAD)
		{
			num_already_free++;
			if (MAX_OBJECTS - num_already_free < num_used)
				return;
		} else
			switch (objp->type)
			{
				case OBJ_NONE:
					num_already_free++;
					if (MAX_OBJECTS - num_already_free < num_used)
						return;
					break;
				case OBJ_WALL:
					Int3();		//	This is curious.  What is an object that is a wall?
					break;
				case OBJ_FIREBALL:
				case OBJ_WEAPON:
				case OBJ_DEBRIS:
					obj_list[olind++] = objp;
					break;
				case OBJ_ROBOT:
				case OBJ_HOSTAGE:
				case OBJ_PLAYER:
				case OBJ_CNTRLCEN:
				case OBJ_CLUTTER:
				case OBJ_GHOST:
				case OBJ_LIGHT:
				case OBJ_CAMERA:
				case OBJ_POWERUP:
					break;
			}

	}

	num_to_free = MAX_OBJECTS - num_used - num_already_free;

	if (num_to_free > olind) {
		num_to_free = olind;
	}

	// Capture before num_to_free modified
	const auto r = partial_range(obj_list, num_to_free);
	auto l = [&r, &num_to_free](bool (*predicate)(const vcobjptr_t)) -> bool {
		range_for (const auto o, r)
			if (predicate(o)) {
				o->flags |= OF_SHOULD_BE_DEAD;
				if (!-- num_to_free)
					return true;
			}
		return false;
	};

	auto predicate_debris = [](const vcobjptr_t o) { return o->type == OBJ_DEBRIS; };
	if (l(predicate_debris))
		return;

	auto predicate_fireball = [](const vcobjptr_t o) { return o->type == OBJ_FIREBALL && o->ctype.expl_info.delete_objnum == object_none; };
	if (l(predicate_fireball))
		return;

	auto predicate_flare = [](const vcobjptr_t o) { return (o->type == OBJ_WEAPON) && (get_weapon_id(o) == FLARE_ID); };
	if (l(predicate_flare))
		return;

	auto predicate_nonflare_weapon = [](const vcobjptr_t o) { return (o->type == OBJ_WEAPON) && (get_weapon_id(o) != FLARE_ID); };
	if (l(predicate_nonflare_weapon))
		return;
}

//-----------------------------------------------------------------------------
//initialize a new object.  adds to the list for the given segment
//note that segnum is really just a suggestion, since this routine actually
//searches for the correct segment
//returns the object number
objptridx_t obj_create(object_type_t type, ubyte id,vsegptridx_t segnum,const vms_vector &pos,
				const vms_matrix *orient,fix size,ubyte ctype,ubyte mtype,ubyte rtype)
{
	// Some consistency checking. FIXME: Add more debug output here to probably trace all possible occurances back.
	Assert(ctype <= CT_CNTRLCEN);

	if (type==OBJ_DEBRIS && Debris_object_count>=Max_debris_objects && !PERSISTENT_DEBRIS)
		return object_none;

	if (get_seg_masks(pos, segnum, 0).centermask != 0)
	{
		auto p = find_point_seg(pos,segnum);
		if (p == segment_none) {
			return object_none;		//don't create this object
		}
		segnum = p;
	}

	// Find next free object
	const objptridx_t obj = obj_allocate();

	if (obj == object_none)		//no free objects
		return obj;

	Assert(obj->type == OBJ_NONE);		//make sure unused
	auto signature = obj_get_signature();
	// Zero out object structure to keep weird bugs from happening
	// in uninitialized fields.
	*obj = {};
	// Tell Valgrind to warn on any uninitialized fields.
	DXX_MAKE_MEM_UNDEFINED(&*obj, sizeof(*obj));

	obj->signature				= signature;
	obj->type 					= type;
	obj->id 						= id;
	obj->last_pos				= pos;
	obj->pos 					= pos;
	obj->size 					= size;
	obj->flags 					= 0;
	//@@if (orient != NULL)
	//@@	obj->orient 			= *orient;

	obj->orient 				= orient?*orient:vmd_identity_matrix;

	obj->control_type 		= ctype;
	obj->movement_type 		= mtype;
	obj->render_type 			= rtype;
	obj->contains_type		= -1;

	obj->lifeleft 				= IMMORTAL_TIME;		//assume immortal
	obj->attached_obj			= object_none;

	if (obj->control_type == CT_POWERUP)
		obj->ctype.powerup_info.count = 1;

	// Init physics info for this object
	if (obj->movement_type == MT_PHYSICS) {

		vm_vec_zero(obj->mtype.phys_info.velocity);
		vm_vec_zero(obj->mtype.phys_info.thrust);
		vm_vec_zero(obj->mtype.phys_info.rotvel);
		vm_vec_zero(obj->mtype.phys_info.rotthrust);

		obj->mtype.phys_info.mass		= 0;
		obj->mtype.phys_info.drag 		= 0;
		obj->mtype.phys_info.turnroll	= 0;
		obj->mtype.phys_info.flags		= 0;
	}

	if (obj->render_type == RT_POLYOBJ)
		obj->rtype.pobj_info.tmap_override = -1;

	obj->shields 				= 20*F1_0;

	{
		auto p = find_point_seg(pos,segnum);		//find correct segment
		// Previously this was only an assert check.  Now it is also
		// checked at runtime.
		segnum = p;
	}

	obj->segnum = segment_none;					//set to zero by memset, above
	obj_link(obj,segnum);

	//	Set (or not) persistent bit in phys_info.
	if (obj->type == OBJ_WEAPON) {
		Assert(obj->control_type == CT_WEAPON);
		obj->mtype.phys_info.flags |= (Weapon_info[get_weapon_id(obj)].persistent*PF_PERSISTENT);
		obj->ctype.laser_info.creation_time = GameTime64;
		obj->ctype.laser_info.last_hitobj = object_none;
		obj->ctype.laser_info.hitobj_list.clear();
		obj->ctype.laser_info.multiplier = F1_0;
#if defined(DXX_BUILD_DESCENT_II)
		obj->ctype.laser_info.last_afterburner_time = 0;
#endif
	}

#if defined(DXX_BUILD_DESCENT_II)
	if (obj->control_type == CT_POWERUP)
		obj->ctype.powerup_info.creation_time = GameTime64;
#endif

	if (obj->control_type == CT_EXPLOSION)
		obj->ctype.expl_info.next_attach = obj->ctype.expl_info.prev_attach = obj->ctype.expl_info.attach_parent = object_none;

	if (obj->type == OBJ_DEBRIS)
		Debris_object_count++;

	return obj;
}

#ifdef EDITOR
//create a copy of an object. returns new object number
objptridx_t obj_create_copy(objnum_t objnum, const vms_vector &new_pos, segnum_t newsegnum)
{
	// Find next free object
	const objptridx_t obj = obj_allocate();

	if (obj == object_none)
		return obj;

	*obj = Objects[objnum];

	obj->pos = obj->last_pos = new_pos;

	obj->next = obj->prev = object_none;
	obj->segnum = segment_none;

	obj_link(obj,newsegnum);

	obj->signature				= obj_get_signature();

	//we probably should initialize sub-structures here

	return obj;

}
#endif

//remove object from the world
void obj_delete(const vobjptridx_t obj)
{
	Assert(obj->type != OBJ_NONE);
	Assert(obj != ConsoleObject);

#if defined(DXX_BUILD_DESCENT_II)
	if (obj->type==OBJ_WEAPON && get_weapon_id(obj)==GUIDEDMISS_ID && obj->ctype.laser_info.parent_type==OBJ_PLAYER)
	{
		int pnum=get_player_id(&Objects[obj->ctype.laser_info.parent_num]);

		if (pnum!=Player_num) {
			Guided_missile[pnum]=NULL;
		}
		else if (Newdemo_state==ND_STATE_RECORDING)
			newdemo_record_guided_end();
		
	}
#endif

	if (obj == Viewer)		//deleting the viewer?
		Viewer = ConsoleObject;						//..make the player the viewer

	if (obj->flags & OF_ATTACHED)		//detach this from object
		obj_detach_one(obj);

	if (obj->attached_obj != object_none)		//detach all objects from this
		obj_detach_all(obj);

	if (obj->type == OBJ_DEBRIS)
		Debris_object_count--;

	obj_unlink(obj);

	Assert(Objects[0].next != 0);
	DXX_MAKE_MEM_UNDEFINED(&*obj, sizeof(*obj));
	obj->type = OBJ_NONE;		//unused!
	obj_free(obj);
}

#define	DEATH_SEQUENCE_LENGTH			(F1_0*5)
#define	DEATH_SEQUENCE_EXPLODE_TIME	(F1_0*2)

int		Player_is_dead = 0;			//	If !0, then player is dead, but game continues so he can watch.
object	*Dead_player_camera = NULL;	//	Object index of object watching deader.
object	*Viewer_save;
int		Player_flags_save;
int		Player_exploded = 0;
int		Death_sequence_aborted=0;
int		Player_eggs_dropped=0;
fix		Camera_to_player_dist_goal=F1_0*4;
ubyte		Control_type_save, Render_type_save;

//	------------------------------------------------------------------------------------------------------------------
void dead_player_end(void)
{
	if (!Player_is_dead)
		return;

	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_restore_cockpit();

	Player_is_dead = 0;
	Player_exploded = 0;
	obj_delete(Dead_player_camera-Objects);
	Dead_player_camera = NULL;
	select_cockpit(PlayerCfg.CockpitMode[0]);
	Viewer = Viewer_save;
	ConsoleObject->type = OBJ_PLAYER;
	ConsoleObject->flags = Player_flags_save;

	Assert((Control_type_save == CT_FLYING) || (Control_type_save == CT_SLEW));

	ConsoleObject->control_type = Control_type_save;
	ConsoleObject->render_type = Render_type_save;
	Players[Player_num].flags &= ~PLAYER_FLAGS_INVULNERABLE;
	Player_eggs_dropped = 0;

}

//	------------------------------------------------------------------------------------------------------------------
//	Camera is less than size of player away from
static void set_camera_pos(vms_vector &camera_pos, const vobjptridx_t objp)
{
	int	count = 0;
	fix	camera_player_dist;
	fix	far_scale;

	camera_player_dist = vm_vec_dist_quick(camera_pos, objp->pos);

	if (camera_player_dist < Camera_to_player_dist_goal) { //2*objp->size) {
		//	Camera is too close to player object, so move it away.
		fvi_query	fq;
		fvi_info		hit_data;

		auto player_camera_vec = vm_vec_sub(camera_pos, objp->pos);
		if ((player_camera_vec.x == 0) && (player_camera_vec.y == 0) && (player_camera_vec.z == 0))
			player_camera_vec.x += F1_0/16;

		hit_data.hit_type = HIT_WALL;
		far_scale = F1_0;

		while ((hit_data.hit_type != HIT_NONE) && (count++ < 6)) {
			vm_vec_normalize_quick(player_camera_vec);
			vm_vec_scale(player_camera_vec, Camera_to_player_dist_goal);

			fq.p0 = &objp->pos;
			const auto closer_p1 = vm_vec_add(objp->pos, player_camera_vec);		//	This is the actual point we want to put the camera at.
			vm_vec_scale(player_camera_vec, far_scale);						//	...but find a point 50% further away...
			const auto local_p1 = vm_vec_add(objp->pos, player_camera_vec);		//	...so we won't have to do as many cuts.

			fq.p1 = &local_p1;
			fq.startseg = objp->segnum;
			fq.rad = 0;
			fq.thisobjnum = objp;
			fq.ignore_obj_list.first = nullptr;
			fq.flags = 0;
			find_vector_intersection(fq, hit_data);

			if (hit_data.hit_type == HIT_NONE) {
				camera_pos = closer_p1;
			} else {
				make_random_vector(player_camera_vec);
				far_scale = 3*F1_0/2;
			}
		}
	}
}

//	------------------------------------------------------------------------------------------------------------------
void dead_player_frame(void)
{
	static fix	time_dead = 0;

	if (Player_is_dead)
	{
		time_dead += FrameTime;

		//	If unable to create camera at time of death, create now.
		if (Dead_player_camera == Viewer_save) {
			object	*player = &Objects[Players[Player_num].objnum];

			auto objnum = obj_create(OBJ_CAMERA, 0, player->segnum, player->pos, &player->orient, 0, CT_NONE, MT_NONE, RT_NONE);

			if (objnum != object_none)
				Viewer = Dead_player_camera = objnum;
			else {
				Int3();
			}
		}		

		ConsoleObject->mtype.phys_info.rotvel.x = max(0, DEATH_SEQUENCE_EXPLODE_TIME - time_dead)/4;
		ConsoleObject->mtype.phys_info.rotvel.y = max(0, DEATH_SEQUENCE_EXPLODE_TIME - time_dead)/2;
		ConsoleObject->mtype.phys_info.rotvel.z = max(0, DEATH_SEQUENCE_EXPLODE_TIME - time_dead)/3;

		Camera_to_player_dist_goal = min(time_dead*8, F1_0*20) + ConsoleObject->size;

		set_camera_pos(Dead_player_camera->pos, ConsoleObject);

		// the following line uncommented by WraithX, 4-12-00
		if (time_dead < DEATH_SEQUENCE_EXPLODE_TIME + F1_0 * 2)
		{
			const auto fvec = vm_vec_sub(ConsoleObject->pos, Dead_player_camera->pos);
			vm_vector_2_matrix(Dead_player_camera->orient, fvec, nullptr, nullptr);
			Dead_player_camera->mtype.phys_info = ConsoleObject->mtype.phys_info;

			// the following "if" added by WraithX to get rid of camera "wiggle"
			if (Dead_player_camera->mtype.phys_info.flags & PF_WIGGLE)
			{
				Dead_player_camera->mtype.phys_info.flags = (Dead_player_camera->mtype.phys_info.flags & ~PF_WIGGLE);
			}// end "if" added by WraithX, 4/13/00

		// the following line uncommented by WraithX, 4-12-00
		}
		else
		{
			// the following line uncommented by WraithX, 4-11-00
			Dead_player_camera->movement_type = MT_PHYSICS;
			//Dead_player_camera->mtype.phys_info.rotvel.y = F1_0/8;
		// the following line uncommented by WraithX, 4-12-00
		}
		// end addition by WX

		if (time_dead > DEATH_SEQUENCE_EXPLODE_TIME) {
			if (!Player_exploded) {

				if (Players[Player_num].hostages_on_board > 1)
					HUD_init_message(HM_DEFAULT, TXT_SHIP_DESTROYED_2, Players[Player_num].hostages_on_board);
				else if (Players[Player_num].hostages_on_board == 1)
					HUD_init_message_literal(HM_DEFAULT, TXT_SHIP_DESTROYED_1);
				else
					HUD_init_message_literal(HM_DEFAULT, TXT_SHIP_DESTROYED_0);
				Players[Player_num].hostages_on_board = 0;

				Player_exploded = 1;
				if (Game_mode & GM_NETWORK)
					multi_powcap_cap_objects();
				
				const auto cobjp = vobjptridx(ConsoleObject);
				drop_player_eggs(cobjp);
				Player_eggs_dropped = 1;
				if (Game_mode & GM_MULTI)
				{
					multi_send_player_deres(deres_explode);
				}

				explode_badass_player(cobjp);

				//is this next line needed, given the badass call above?
				explode_object(cobjp,0);
				ConsoleObject->flags &= ~OF_SHOULD_BE_DEAD;		//don't really kill player
				ConsoleObject->render_type = RT_NONE;				//..just make him disappear
				ConsoleObject->type = OBJ_GHOST;						//..and kill intersections
#if defined(DXX_BUILD_DESCENT_II)
				Players[Player_num].flags &= ~PLAYER_FLAGS_HEADLIGHT_ON;
#endif
			}
		} else {
			if (d_rand() < FrameTime*4) {
				if (Game_mode & GM_MULTI)
					multi_send_create_explosion(Player_num);
				create_small_fireball_on_object(vobjptridx(ConsoleObject), F1_0, 1);
			}
		}


		if (Death_sequence_aborted)
		{
			if (!Player_eggs_dropped) {
			
				if (Game_mode & GM_NETWORK)
					multi_powcap_cap_objects();
				
				drop_player_eggs(vobjptridx(ConsoleObject));
				Player_eggs_dropped = 1;
				if (Game_mode & GM_MULTI)
				{
					multi_send_player_deres(deres_explode);
				}
			}

			DoPlayerDead();		//kill_player();
		}
	}
	else
		time_dead = 0;
}

//	------------------------------------------------------------------------------------------------------------------
static void start_player_death_sequence(const vobjptr_t player)
{
	Assert(player == ConsoleObject);
	if ((Player_is_dead != 0) || (Dead_player_camera != NULL) || ((Game_mode & GM_MULTI) && (Players[Player_num].connected != CONNECT_PLAYING)))
		return;

	//Assert(Player_is_dead == 0);
	//Assert(Dead_player_camera == NULL);

	reset_rear_view();

	if (!(Game_mode & GM_MULTI))
		HUD_clear_messages();

	Death_sequence_aborted = 0;

	if (Game_mode & GM_MULTI)
	{
#if defined(DXX_BUILD_DESCENT_II)
		playernum_t killer_pnum = Player_num;
		if (Players[Player_num].killer_objnum > 0 && Players[Player_num].killer_objnum < Highest_object_index)
			killer_pnum = get_player_id(&Objects[Players[Player_num].killer_objnum]);
		
		// If Hoard, increase number of orbs by 1. Only if you haven't killed yourself. This prevents cheating
		if (game_mode_hoard())
			if (Players[Player_num].secondary_ammo[PROXIMITY_INDEX]<12)
				if (!(Players[Player_num].killer_objnum == Players[Player_num].objnum || ((Game_mode & GM_TEAM) && get_team(Player_num) == get_team(killer_pnum))))
					Players[Player_num].secondary_ammo[PROXIMITY_INDEX]++;
#endif
		multi_send_kill(vobjptridx(Players[Player_num].objnum));
	}
	
	PaletteRedAdd = 40;
	Player_is_dead = 1;

	vm_vec_zero(player->mtype.phys_info.rotthrust);
	vm_vec_zero(player->mtype.phys_info.thrust);

	auto objnum = obj_create(OBJ_CAMERA, 0, player->segnum, player->pos, &player->orient, 0, CT_NONE, MT_NONE, RT_NONE);
	Viewer_save = Viewer;
	if (objnum != object_none)
		Viewer = Dead_player_camera = objnum;
	else {
		Int3();
		Dead_player_camera = Viewer;
	}

	select_cockpit(CM_LETTERBOX);
	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_letterbox();

	Player_flags_save = player->flags;
	Control_type_save = player->control_type;
	Render_type_save = player->render_type;

	player->flags &= ~OF_SHOULD_BE_DEAD;
//	Players[Player_num].flags |= PLAYER_FLAGS_INVULNERABLE;
	player->control_type = CT_NONE;
	player->shields = F1_0*1000;

	PALETTE_FLASH_SET(0,0,0);
}

//	------------------------------------------------------------------------------------------------------------------
static void obj_delete_all_that_should_be_dead()
{
	objnum_t		local_dead_player_object=object_none;

	// Move all objects
	range_for (const auto i, highest_valid(Objects))
	{
		auto objp = vobjptridx(i);
		if ((objp->type!=OBJ_NONE) && (objp->flags&OF_SHOULD_BE_DEAD) )	{
			Assert(!(objp->type==OBJ_FIREBALL && objp->ctype.expl_info.delete_time!=-1));
			if (objp->type==OBJ_PLAYER) {
				if ( get_player_id(objp) == Player_num ) {
					if (local_dead_player_object == object_none) {
						start_player_death_sequence(objp);
						local_dead_player_object = objp;
					} else
						Int3();	//	Contact Mike: Illegal, killed player twice in this frame!
									// Ok to continue, won't start death sequence again!
					// kill_player();
				}
			} else {					
				obj_delete(objp);
			}
		}
	}
}

//when an object has moved into a new segment, this function unlinks it
//from its old segment, and links it into the new segment
void obj_relink(const vobjptridx_t objnum,const vsegptridx_t newsegnum)
{
	obj_unlink(objnum);
	obj_link(objnum,newsegnum);
}

// for getting out of messed up linking situations (i.e. caused by demo playback)
void obj_relink_all(void)
{
	range_for (const auto segnum, highest_valid(Segments))
	{
		const auto &&segp = vsegptr(static_cast<segnum_t>(segnum));
		segp->objects = object_none;
	}
	
	range_for (const auto objnum, highest_valid(Objects))
	{
		auto obj = vobjptridx(objnum);
		if (obj->type != OBJ_NONE)
		{
			auto segnum = exchange(obj->segnum, segment_none);
			obj->next = obj->prev = object_none;
			if (segnum > Highest_segment_index)
				segnum = segment_first;
			obj_link(obj, segnum);
		}
	}
}

//process a continuously-spinning object
static void spin_object(const vobjptr_t obj)
{
	vms_angvec rotangs;
	Assert(obj->movement_type == MT_SPINNING);

	rotangs.p = fixmul(obj->mtype.spin_rate.x,FrameTime);
	rotangs.h = fixmul(obj->mtype.spin_rate.y,FrameTime);
	rotangs.b = fixmul(obj->mtype.spin_rate.z,FrameTime);

	const auto &&rotmat = vm_angles_2_matrix(rotangs);
	obj->orient = vm_matrix_x_matrix(obj->orient,rotmat);
	check_and_fix_matrix(obj->orient);
}

#if defined(DXX_BUILD_DESCENT_II)
int Drop_afterburner_blob_flag;		//ugly hack
//see if wall is volatile, and if so, cause damage to player
//returns true if player is in lava
#endif

//--------------------------------------------------------------------
//move an object for the current frame
static void object_move_one(const vobjptridx_t obj)
{
	int	previous_segment = obj->segnum;

	obj->last_pos = obj->pos;			// Save the current position

	if ((obj->type==OBJ_PLAYER) && (Player_num==get_player_id(obj)))	{
#if defined(DXX_BUILD_DESCENT_II)
      if (game_mode_capture_flag())
			 fuelcen_check_for_goal (&Segments[obj->segnum]);
      if (game_mode_hoard())
			 fuelcen_check_for_hoard_goal (&Segments[obj->segnum]);
#endif

		fix fuel=fuelcen_give_fuel( &Segments[obj->segnum], INITIAL_ENERGY-Players[Player_num].energy );
		if (fuel > 0 )	{
			Players[Player_num].energy += fuel;
		}
#if defined(DXX_BUILD_DESCENT_II)
		fix shields = repaircen_give_shields( &Segments[obj->segnum], INITIAL_SHIELDS-Players[Player_num].shields );
		if (shields > 0) {
			Players[Player_num].shields += shields;
		}
#endif
	}

	if (obj->lifeleft != IMMORTAL_TIME) //if not immortal...
		obj->lifeleft -= FrameTime; //...inevitable countdown towards death

#if defined(DXX_BUILD_DESCENT_II)
	Drop_afterburner_blob_flag = 0;
#endif

	switch (obj->control_type) {

		case CT_NONE: break;

		case CT_FLYING:

			read_flying_controls( obj );

			break;

		case CT_REPAIRCEN: Int3();	// -- hey! these are no longer supported!! -- do_repair_sequence(obj); break;

		case CT_POWERUP: do_powerup_frame(obj); break;
	
		case CT_MORPH:			//morph implies AI
			do_morph_frame(obj);
			//NOTE: FALLS INTO AI HERE!!!!

		case CT_AI:
			//NOTE LINK TO CT_MORPH ABOVE!!!
			if (Game_suspended & SUSP_ROBOTS) return;
			do_ai_frame(obj);
			break;

		case CT_WEAPON:		Laser_do_weapon_sequence(obj); break;
		case CT_EXPLOSION:	do_explosion_sequence(obj); break;

		#ifndef RELEASE
		case CT_SLEW:
			if ( keyd_pressed[KEY_PAD5] ) slew_stop();
			if ( keyd_pressed[KEY_NUMLOCK] ) 		{
				slew_reset_orient();
			}
			slew_frame(0 );		// Does velocity addition for us.
			break;
		#endif

//		case CT_FLYTHROUGH:
//			do_flythrough(obj,0);			// HACK:do_flythrough should operate on an object!!!!
//			//check_object_seg(obj);
//			return;	// DON'T DO THE REST OF OBJECT STUFF SINCE THIS IS A SPECIAL CASE!!!
//			break;

		case CT_DEBRIS: do_debris_frame(obj); break;

		case CT_LIGHT: break;		//doesn't do anything

		case CT_REMOTE: break;		//doesn't do anything

		case CT_CNTRLCEN: do_controlcen_frame(obj); break;

		default:

			Error("Unknown control type %d in object %hu, sig/type/id = %i/%i/%i",obj->control_type, static_cast<objnum_t>(obj), obj->signature.get(), obj->type, obj->id);

			break;

	}

	if (obj->lifeleft < 0 ) {		// We died of old age
		obj->flags |= OF_SHOULD_BE_DEAD;
		if ( obj->type==OBJ_WEAPON && Weapon_info[get_weapon_id(obj)].damage_radius )
			explode_badass_weapon(obj, obj->pos);
#if defined(DXX_BUILD_DESCENT_II)
		else if ( obj->type==OBJ_ROBOT)	//make robots explode
			explode_object(obj,0);
#endif
	}

	if (obj->type == OBJ_NONE || obj->flags&OF_SHOULD_BE_DEAD)
		return;         // object has been deleted

	switch (obj->movement_type) {

		case MT_NONE:			break;				//this doesn't move

		case MT_PHYSICS:		do_physics_sim(obj);	break;	//move by physics

		case MT_SPINNING:		spin_object(obj); break;

	}

	//	If player and moved to another segment, see if hit any triggers.
	// also check in player under a lavafall
	if (obj->type == OBJ_PLAYER && obj->movement_type==MT_PHYSICS)	{

		if (previous_segment != obj->segnum && n_phys_segs > 1)
		{
			auto seg0 = vsegptridx(phys_seglist[0]);
#if defined(DXX_BUILD_DESCENT_II)
			int	old_level = Current_level_num;
#endif
			range_for (const auto i, partial_range(phys_seglist, 1u, n_phys_segs))
			{
				const auto seg1 = vsegptridx(i);
				const auto connect_side = find_connect_side(seg1, seg0);
				if (connect_side != -1)
				{
					check_trigger(seg0, connect_side, obj,0);
#if defined(DXX_BUILD_DESCENT_II)
				//maybe we've gone on to the next level.  if so, bail!
				if (Current_level_num != old_level)
					return;
#endif
				}
				seg0 = seg1;
			}
		}
#if defined(DXX_BUILD_DESCENT_II)
		{
			int sidemask,under_lavafall=0;
			static int lavafall_hiss_playing[MAX_PLAYERS]={0};

			sidemask = get_seg_masks(obj->pos, obj->segnum, obj->size).sidemask;
			if (sidemask) {
				int sidenum,bit,wall_num;
	
				for (sidenum=0,bit=1;sidenum<6;bit<<=1,sidenum++)
					if ((sidemask & bit) && ((wall_num=Segments[obj->segnum].sides[sidenum].wall_num)!=-1) && Walls[wall_num].type==WALL_ILLUSION) {
						int type;
						if ((type=check_volatile_wall(obj,vsegptridx(obj->segnum),sidenum))!=0) {
							int sound = (type==1)?SOUND_LAVAFALL_HISS:SOUND_SHIP_IN_WATERFALL;
							under_lavafall = 1;
							if (!lavafall_hiss_playing[obj->id]) {
								digi_link_sound_to_object3( sound, obj, 1, F1_0, vm_distance{i2f(256)}, -1, -1);
								lavafall_hiss_playing[obj->id] = 1;
							}
						}
					}
			}
	
			if (!under_lavafall && lavafall_hiss_playing[obj->id]) {
				digi_kill_sound_linked_to_object( obj);
				lavafall_hiss_playing[obj->id] = 0;
			}
		}
#endif
	}

#if defined(DXX_BUILD_DESCENT_II)
	//see if guided missile has flown through exit trigger
	if (obj==Guided_missile[Player_num] && obj->signature==Guided_missile_sig[Player_num]) {
		if (previous_segment != obj->segnum) {
			auto connect_side = find_connect_side(&Segments[obj->segnum], &Segments[previous_segment]);
			if (connect_side != -1) {
				auto wall_num = Segments[previous_segment].sides[connect_side].wall_num;
				if ( wall_num != wall_none ) {
					auto trigger_num = Walls[wall_num].trigger;
					if (trigger_num != trigger_none)
						if (Triggers[trigger_num].type == TT_EXIT)
							Guided_missile[Player_num]->lifeleft = 0;
				}
			}
		}
	}

	if (Drop_afterburner_blob_flag) {
		Assert(obj==ConsoleObject);
		drop_afterburner_blobs(obj, 2, i2f(5)/2, -1);	//	-1 means use default lifetime
		if (Game_mode & GM_MULTI)
			multi_send_drop_blobs(Player_num);
		Drop_afterburner_blob_flag = 0;
	}

	if ((obj->type == OBJ_WEAPON) && (Weapon_info[get_weapon_id(obj)].afterburner_size)) {
		fix	vel = vm_vec_mag_quick(obj->mtype.phys_info.velocity);
		fix	delay, lifetime;

		if (vel > F1_0*200)
			delay = F1_0/16;
		else if (vel > F1_0*40)
			delay = fixdiv(F1_0*13,vel);
		else
			delay = F1_0/4;

		lifetime = (delay * 3)/2;
		if (!(Game_mode & GM_MULTI)) {
			delay /= 2;
			lifetime *= 2;
		}

		assert(obj->control_type == CT_WEAPON);
		if ((obj->ctype.laser_info.last_afterburner_time + delay < GameTime64) || (obj->ctype.laser_info.last_afterburner_time > GameTime64)) {
			drop_afterburner_blobs(obj, 1, i2f(Weapon_info[get_weapon_id(obj)].afterburner_size)/16, lifetime);
			obj->ctype.laser_info.last_afterburner_time = GameTime64;
		}
	}
#endif
}

//--------------------------------------------------------------------
//move all objects for the current frame
void object_move_all()
{
	if (Highest_object_index > MAX_USED_OBJECTS)
		free_object_slots(MAX_USED_OBJECTS);		//	Free all possible object slots.

	obj_delete_all_that_should_be_dead();

	if (PlayerCfg.AutoLeveling)
		ConsoleObject->mtype.phys_info.flags |= PF_LEVELLING;
	else
		ConsoleObject->mtype.phys_info.flags &= ~PF_LEVELLING;

	// Move all objects
	range_for (const auto i, highest_valid(Objects))
	{
		const auto objp = vobjptridx(i);
		if ( (objp->type != OBJ_NONE) && (!(objp->flags&OF_SHOULD_BE_DEAD)) )	{
			object_move_one( objp );
		}
	}

//	check_duplicate_objects();
//	remove_incorrect_objects();

}


//--unused-- // -----------------------------------------------------------
//--unused-- //	Moved here from eobject.c on 02/09/94 by MK.
//--unused-- int find_last_obj(int i)
//--unused-- {
//--unused-- 	for (i=MAX_OBJECTS;--i>=0;)
//--unused-- 		if (Objects[i].type != OBJ_NONE) break;
//--unused--
//--unused-- 	return i;
//--unused--
//--unused-- }


//make object array non-sparse
void compress_objects(void)
{
	//last_i = find_last_obj(MAX_OBJECTS);

	//	Note: It's proper to do < (rather than <=) Highest_object_index here because we
	//	are just removing gaps, and the last object can't be a gap.
	for (objnum_t start_i=0;start_i<Highest_object_index;start_i++)

		if (Objects[start_i].type == OBJ_NONE) {
			const auto h = vobjptridx(Highest_object_index);
			auto segnum_copy = h->segnum;

			obj_unlink(h);

			Objects[start_i] = *h;

			#ifdef EDITOR
			if (Cur_object_index == Highest_object_index)
				Cur_object_index = start_i;
			#endif

			h->type = OBJ_NONE;

			obj_link(vobjptridx(start_i),segnum_copy);

			while (Objects[--Highest_object_index].type == OBJ_NONE);

			//last_i = find_last_obj(last_i);
			
		}

	reset_objects(num_objects);

}

//called after load.  Takes number of objects,  and objects should be
//compressed.  resets free list, marks unused objects as unused
void reset_objects(int n_objs)
{
	num_objects = n_objs;

	Assert(num_objects>0);

	for (int i=num_objects;i<MAX_OBJECTS;i++) {
		free_obj_list[i] = i;
		Objects[i] = {};
		Objects[i].type = OBJ_NONE;
		Objects[i].segnum = segment_none;
	}

	Highest_object_index = num_objects-1;

	Debris_object_count = 0;
}

//Tries to find a segment for an object, using find_point_seg()
segnum_t find_object_seg(const vobjptr_t obj)
{
	return find_point_seg(obj->pos,obj->segnum);
}


//If an object is in a segment, set its segnum field and make sure it's
//properly linked.  If not in any segment, returns 0, else 1.
//callers should generally use find_vector_intersection()
int update_object_seg(const vobjptridx_t obj)
{
	auto newseg = find_object_seg(obj);
	if (newseg == segment_none)
		return 0;

	if ( newseg != obj->segnum )
		obj_relink(obj, newseg );

	return 1;
}

void set_powerup_id(const vobjptr_t o, powerup_type_t id)
{
	o->id = id;
	o->size = Powerup_info[id].size;
	const auto vclip_num = Powerup_info[id].vclip_num;
	o->rtype.vclip_info.vclip_num = vclip_num;
	o->rtype.vclip_info.frametime = Vclip[vclip_num].frame_time;
}

//go through all objects and make sure they have the correct segment numbers
void fix_object_segs()
{
	range_for (const auto i, highest_valid(Objects))
	{
		const auto o = vobjptridx(i);
		if (o->type != OBJ_NONE)
			if (update_object_seg(o) == 0) {
				const auto pos = o->pos;
				compute_segment_center(o->pos,&Segments[o->segnum]);
				con_printf(CON_URGENT, "Object %hu claims segment %u, but has position {%i,%i,%i}; moving to {%i,%i,%i}", static_cast<objnum_t>(o), o->segnum, pos.x, pos.y, pos.z, o->pos.x, o->pos.y, o->pos.z);
			}
	}
}


//--unused-- void object_use_new_object_list( object * new_list )
//--unused-- {
//--unused-- 	int i, segnum;
//--unused-- 	object *obj;
//--unused--
//--unused-- 	// First, unlink all the old objects for the segments array
//--unused-- 	for (segnum=0; segnum <= Highest_segment_index; segnum++) {
//--unused-- 		Segments[segnum].objects = -1;
//--unused-- 	}
//--unused-- 	// Then, erase all the objects
//--unused-- 	reset_objects(1);
//--unused--
//--unused-- 	// Fill in the object array
//--unused-- 	memcpy( Objects, new_list, sizeof(object)*MAX_OBJECTS );
//--unused--
//--unused-- 	Highest_object_index=-1;
//--unused--
//--unused-- 	// Relink 'em
//--unused-- 	for (i=0; i<MAX_OBJECTS; i++ )	{
//--unused-- 		obj = &Objects[i];
//--unused-- 		if ( obj->type != OBJ_NONE )	{
//--unused-- 			num_objects++;
//--unused-- 			Highest_object_index = i;
//--unused-- 			segnum = obj->segnum;
//--unused-- 			obj->next = obj->prev = obj->segnum = -1;
//--unused-- 			obj_link(i,segnum);
//--unused-- 		} else {
//--unused-- 			obj->next = obj->prev = obj->segnum = -1;
//--unused-- 		}
//--unused-- 	}
//--unused-- 	
//--unused-- }

static int object_is_clearable_weapon(const vcobjptr_t obj, int clear_all)
{
	if (!(obj->type == OBJ_WEAPON))
		return 0;
#if defined(DXX_BUILD_DESCENT_II)
	if (Weapon_info[get_weapon_id(obj)].flags&WIF_PLACABLE)
		return 0;
#endif
	return (clear_all || !is_proximity_bomb_or_smart_mine(get_weapon_id(obj)));
}

//delete objects, such as weapons & explosions, that shouldn't stay between levels
//	Changed by MK on 10/15/94, don't remove proximity bombs.
//if clear_all is set, clear even proximity bombs
void clear_transient_objects(int clear_all)
{
	range_for (const auto objnum, highest_valid(Objects))
	{
		auto obj = vobjptridx(objnum);
		if (object_is_clearable_weapon(obj, clear_all) ||
			 obj->type == OBJ_FIREBALL ||
			 obj->type == OBJ_DEBRIS ||
			 obj->type == OBJ_DEBRIS ||
			 (obj->type!=OBJ_NONE && obj->flags & OF_EXPLODING)) {
			obj_delete(obj);
		}
	}
}

//attaches an object, such as a fireball, to another object, such as a robot
void obj_attach(const vobjptridx_t parent,const vobjptridx_t sub)
{
	Assert(sub->type == OBJ_FIREBALL);
	Assert(sub->control_type == CT_EXPLOSION);

	Assert(sub->ctype.expl_info.next_attach==object_none);
	Assert(sub->ctype.expl_info.prev_attach==object_none);

	Assert(parent->attached_obj==object_none || Objects[parent->attached_obj].ctype.expl_info.prev_attach==object_none);

	sub->ctype.expl_info.next_attach = parent->attached_obj;

	if (sub->ctype.expl_info.next_attach != object_none)
		Objects[sub->ctype.expl_info.next_attach].ctype.expl_info.prev_attach = sub;

	parent->attached_obj = sub;

	sub->ctype.expl_info.attach_parent = parent;
	sub->flags |= OF_ATTACHED;

	Assert(sub->ctype.expl_info.next_attach != sub);
	Assert(sub->ctype.expl_info.prev_attach != sub);
}

//dettaches one object
void obj_detach_one(const vobjptridx_t sub)
{
	Assert(sub->flags & OF_ATTACHED);
	Assert(sub->ctype.expl_info.attach_parent != object_none);

	if ((Objects[sub->ctype.expl_info.attach_parent].type == OBJ_NONE) || (Objects[sub->ctype.expl_info.attach_parent].attached_obj == object_none))
	{
		sub->flags &= ~OF_ATTACHED;
		return;
	}

	if (sub->ctype.expl_info.next_attach != object_none) {
		Assert(Objects[sub->ctype.expl_info.next_attach].ctype.expl_info.prev_attach==sub);
		Objects[sub->ctype.expl_info.next_attach].ctype.expl_info.prev_attach = sub->ctype.expl_info.prev_attach;
	}

	if (sub->ctype.expl_info.prev_attach != object_none) {
		Assert(Objects[sub->ctype.expl_info.prev_attach].ctype.expl_info.next_attach==sub);
		Objects[sub->ctype.expl_info.prev_attach].ctype.expl_info.next_attach = sub->ctype.expl_info.next_attach;
	}
	else {
		Assert(Objects[sub->ctype.expl_info.attach_parent].attached_obj==sub);
		Objects[sub->ctype.expl_info.attach_parent].attached_obj = sub->ctype.expl_info.next_attach;
	}

	sub->ctype.expl_info.next_attach = sub->ctype.expl_info.prev_attach = object_none;
	sub->flags &= ~OF_ATTACHED;

}

//dettaches all objects from this object
void obj_detach_all(const vobjptr_t parent)
{
	while (parent->attached_obj != object_none)
		obj_detach_one(vobjptridx(parent->attached_obj));
}

#if defined(DXX_BUILD_DESCENT_II)
//creates a marker object in the world.  returns the object number
objnum_t drop_marker_object(const vms_vector &pos,segnum_t segnum,const vms_matrix &orient, int marker_num)
{
	Assert(Marker_model_num != -1);
	auto obj = obj_create(OBJ_MARKER, marker_num, segnum, pos, &orient, Polygon_models[Marker_model_num].rad, CT_NONE, MT_NONE, RT_POLYOBJ);
	if (obj != object_none) {
		obj->rtype.pobj_info.model_num = Marker_model_num;

		vm_vec_copy_scale(obj->mtype.spin_rate,obj->orient.uvec,F1_0/2);

		//	MK, 10/16/95: Using lifeleft to make it flash, thus able to trim lightlevel from all objects.
		obj->lifeleft = IMMORTAL_TIME - 1;
	}
	return obj;	
}

//	*viewer is a viewer, probably a missile.
//	wake up all robots that were rendered last frame subject to some constraints.
void wake_up_rendered_objects(const vobjptr_t viewer, window_rendered_data &window)
{
	//	Make sure that we are processing current data.
	if (timer_query() != window.time) {
		return;
	}

	Ai_last_missile_camera = viewer;

	range_for (const auto objnum, window.rendered_robots)
	{
		object *objp;
		int	fcval = d_tick_count & 3;
		if ((objnum & 3) == fcval) {
			objp = &Objects[objnum];
	
			if (objp->type == OBJ_ROBOT) {
				if (vm_vec_dist_quick(viewer->pos, objp->pos) < F1_0*100) {
					ai_local		*ailp = &objp->ctype.ai_info.ail;
					if (ailp->player_awareness_type == player_awareness_type_t::PA_NONE) {
						objp->ctype.ai_info.SUB_FLAGS |= SUB_FLAGS_CAMERA_AWAKE;
						ailp->player_awareness_type = player_awareness_type_t::PA_WEAPON_ROBOT_COLLISION;
						ailp->player_awareness_time = F1_0*3;
						ailp->previous_visibility = 2;
					}
				}
			}
		}
	}
}
#endif

// Swap endianess of given object_rw if swap == 1
void object_rw_swap(object_rw *obj, int swap)
{
	if (!swap)
		return;

	obj->signature     = SWAPINT(obj->signature);
	obj->next          = SWAPSHORT(obj->next);
	obj->prev          = SWAPSHORT(obj->prev);
	obj->segnum        = SWAPSHORT(obj->segnum);
	obj->attached_obj  = SWAPSHORT(obj->attached_obj);
	obj->pos.x         = SWAPINT(obj->pos.x);
	obj->pos.y         = SWAPINT(obj->pos.y);
	obj->pos.z         = SWAPINT(obj->pos.z);
	obj->orient.rvec.x = SWAPINT(obj->orient.rvec.x);
	obj->orient.rvec.y = SWAPINT(obj->orient.rvec.y);
	obj->orient.rvec.z = SWAPINT(obj->orient.rvec.z);
	obj->orient.fvec.x = SWAPINT(obj->orient.fvec.x);
	obj->orient.fvec.y = SWAPINT(obj->orient.fvec.y);
	obj->orient.fvec.z = SWAPINT(obj->orient.fvec.z);
	obj->orient.uvec.x = SWAPINT(obj->orient.uvec.x);
	obj->orient.uvec.y = SWAPINT(obj->orient.uvec.y);
	obj->orient.uvec.z = SWAPINT(obj->orient.uvec.z);
	obj->size          = SWAPINT(obj->size);
	obj->shields       = SWAPINT(obj->shields);
	obj->last_pos.x    = SWAPINT(obj->last_pos.x);
	obj->last_pos.y    = SWAPINT(obj->last_pos.y);
	obj->last_pos.z    = SWAPINT(obj->last_pos.z);
	obj->lifeleft      = SWAPINT(obj->lifeleft);
	
	switch (obj->movement_type)
	{
		case MT_PHYSICS:
			obj->mtype.phys_info.velocity.x  = SWAPINT(obj->mtype.phys_info.velocity.x);
			obj->mtype.phys_info.velocity.y  = SWAPINT(obj->mtype.phys_info.velocity.y);
			obj->mtype.phys_info.velocity.z  = SWAPINT(obj->mtype.phys_info.velocity.z);
			obj->mtype.phys_info.thrust.x    = SWAPINT(obj->mtype.phys_info.thrust.x);
			obj->mtype.phys_info.thrust.y    = SWAPINT(obj->mtype.phys_info.thrust.y);
			obj->mtype.phys_info.thrust.z    = SWAPINT(obj->mtype.phys_info.thrust.z);
			obj->mtype.phys_info.mass        = SWAPINT(obj->mtype.phys_info.mass);
			obj->mtype.phys_info.drag        = SWAPINT(obj->mtype.phys_info.drag);
			obj->mtype.phys_info.rotvel.x    = SWAPINT(obj->mtype.phys_info.rotvel.x);
			obj->mtype.phys_info.rotvel.y    = SWAPINT(obj->mtype.phys_info.rotvel.y);
			obj->mtype.phys_info.rotvel.z    = SWAPINT(obj->mtype.phys_info.rotvel.z);
			obj->mtype.phys_info.rotthrust.x = SWAPINT(obj->mtype.phys_info.rotthrust.x);
			obj->mtype.phys_info.rotthrust.y = SWAPINT(obj->mtype.phys_info.rotthrust.y);
			obj->mtype.phys_info.rotthrust.z = SWAPINT(obj->mtype.phys_info.rotthrust.z);
			obj->mtype.phys_info.turnroll    = SWAPINT(obj->mtype.phys_info.turnroll);
			obj->mtype.phys_info.flags       = SWAPSHORT(obj->mtype.phys_info.flags);
			break;
			
		case MT_SPINNING:
			obj->mtype.spin_rate.x = SWAPINT(obj->mtype.spin_rate.x);
			obj->mtype.spin_rate.y = SWAPINT(obj->mtype.spin_rate.y);
			obj->mtype.spin_rate.z = SWAPINT(obj->mtype.spin_rate.z);
			break;
	}
	
	switch (obj->control_type)
	{
		case CT_WEAPON:
			obj->ctype.laser_info.parent_type      = SWAPSHORT(obj->ctype.laser_info.parent_type);
			obj->ctype.laser_info.parent_num       = SWAPSHORT(obj->ctype.laser_info.parent_num);
			obj->ctype.laser_info.parent_signature = SWAPINT(obj->ctype.laser_info.parent_signature);
			obj->ctype.laser_info.creation_time    = SWAPINT(obj->ctype.laser_info.creation_time);
			obj->ctype.laser_info.last_hitobj      = SWAPSHORT(obj->ctype.laser_info.last_hitobj);
			obj->ctype.laser_info.track_goal       = SWAPSHORT(obj->ctype.laser_info.track_goal);
			obj->ctype.laser_info.multiplier       = SWAPINT(obj->ctype.laser_info.multiplier);
			break;
			
		case CT_EXPLOSION:
			obj->ctype.expl_info.spawn_time    = SWAPINT(obj->ctype.expl_info.spawn_time);
			obj->ctype.expl_info.delete_time   = SWAPINT(obj->ctype.expl_info.delete_time);
			obj->ctype.expl_info.delete_objnum = SWAPSHORT(obj->ctype.expl_info.delete_objnum);
			obj->ctype.expl_info.attach_parent = SWAPSHORT(obj->ctype.expl_info.attach_parent);
			obj->ctype.expl_info.prev_attach   = SWAPSHORT(obj->ctype.expl_info.prev_attach);
			obj->ctype.expl_info.next_attach   = SWAPSHORT(obj->ctype.expl_info.next_attach);
			break;
			
		case CT_AI:
			obj->ctype.ai_info.hide_segment           = SWAPSHORT(obj->ctype.ai_info.hide_segment);
			obj->ctype.ai_info.hide_index             = SWAPSHORT(obj->ctype.ai_info.hide_index);
			obj->ctype.ai_info.path_length            = SWAPSHORT(obj->ctype.ai_info.path_length);
#if defined(DXX_BUILD_DESCENT_I)
			obj->ctype.ai_info.cur_path_index         = SWAPSHORT(obj->ctype.ai_info.cur_path_index);
#elif defined(DXX_BUILD_DESCENT_II)
			obj->ctype.ai_info.dying_start_time       = SWAPINT(obj->ctype.ai_info.dying_start_time);
#endif
			obj->ctype.ai_info.danger_laser_num       = SWAPSHORT(obj->ctype.ai_info.danger_laser_num);
			obj->ctype.ai_info.danger_laser_signature = SWAPINT(obj->ctype.ai_info.danger_laser_signature);
			break;
			
		case CT_LIGHT:
			obj->ctype.light_info.intensity = SWAPINT(obj->ctype.light_info.intensity);
			break;
			
		case CT_POWERUP:
			obj->ctype.powerup_info.count         = SWAPINT(obj->ctype.powerup_info.count);
#if defined(DXX_BUILD_DESCENT_II)
			obj->ctype.powerup_info.creation_time = SWAPINT(obj->ctype.powerup_info.creation_time);
			obj->ctype.powerup_info.flags         = SWAPINT(obj->ctype.powerup_info.flags);
#endif
			break;
	}
	
	switch (obj->render_type)
	{
		case RT_MORPH:
		case RT_POLYOBJ:
		case RT_NONE: // HACK below
		{
			if (obj->render_type == RT_NONE && obj->type != OBJ_GHOST) // HACK: when a player is dead or not connected yet, clients still expect to get polyobj data - even if render_type == RT_NONE at this time.
				break;
			obj->rtype.pobj_info.model_num                = SWAPINT(obj->rtype.pobj_info.model_num);
			for (uint_fast32_t i=0;i<MAX_SUBMODELS;i++)
			{
				obj->rtype.pobj_info.anim_angles[i].p = SWAPINT(obj->rtype.pobj_info.anim_angles[i].p);
				obj->rtype.pobj_info.anim_angles[i].b = SWAPINT(obj->rtype.pobj_info.anim_angles[i].b);
				obj->rtype.pobj_info.anim_angles[i].h = SWAPINT(obj->rtype.pobj_info.anim_angles[i].h);
			}
			obj->rtype.pobj_info.subobj_flags             = SWAPINT(obj->rtype.pobj_info.subobj_flags);
			obj->rtype.pobj_info.tmap_override            = SWAPINT(obj->rtype.pobj_info.tmap_override);
			obj->rtype.pobj_info.alt_textures             = SWAPINT(obj->rtype.pobj_info.alt_textures);
			break;
		}
			
		case RT_WEAPON_VCLIP:
		case RT_HOSTAGE:
		case RT_POWERUP:
		case RT_FIREBALL:
			obj->rtype.vclip_info.vclip_num = SWAPINT(obj->rtype.vclip_info.vclip_num);
			obj->rtype.vclip_info.frametime = SWAPINT(obj->rtype.vclip_info.frametime);
			break;
			
		case RT_LASER:
			break;
			
	}
}
