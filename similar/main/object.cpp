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
#include <cstdlib>
#include <stdio.h>

#include "inferno.h"
#include "game.h"
#include "gr.h"
#include "bm.h"
#include "3d.h"
#include "segment.h"
#include "texmap.h"
#include "laser.h"
#include "key.h"
#include "gameseg.h"
#include "textures.h"
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
#include "gauges.h"
#include "multi.h"
#include "text.h"
#include "piggy.h"
#include "switch.h"
#include "gameseq.h"
#include "playsave.h"
#include "timer.h"
#if DXX_USE_EDITOR
#include "editor/editor.h"
#endif

#include "compiler-range_for.h"
#include "d_range.h"
#include "d_levelstate.h"
#include "partial_range.h"
#include <utility>

using std::min;
using std::max;

namespace dsx {
static void obj_detach_all(object_array &Objects, object_base &parent);
static void obj_detach_one(object_array &Objects, object &sub);

static int is_proximity_bomb_or_any_smart_mine(const weapon_id_type id)
{
	const auto r = is_proximity_bomb_or_player_smart_mine(id);
#if defined(DXX_BUILD_DESCENT_II)
	if (r)
		return r;
	// superprox dropped by robots have their own ID not considered by is_proximity_bomb_or_player_smart_mine() and since that function is used in many other places, I didn't feel safe to add this weapon type in it
	if (id == weapon_id_type::ROBOT_SUPERPROX_ID)
		return 1;
#endif
	return r;
}

/*
 *  Global variables
 */

object *ConsoleObject;					//the object that is the player
}

namespace dcx {

//Data for objects

// -- Object stuff

//info on the various types of objects
}

namespace dsx {
#ifndef RELEASE
//set viewer object to next object in array
void object_goto_next_viewer()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	auto &vcobjptridx = Objects.vcptridx;
	auto &vmobjptr = Objects.vmptr;
	objnum_t start_obj;
	start_obj = vcobjptridx(Viewer);		//get viewer object number
	
	range_for (const auto &&i, vcobjptr)
	{
		(void)i;
		start_obj++;
		if (start_obj > Highest_object_index ) start_obj = 0;

		auto &objp = *vmobjptr(start_obj);
		if (objp.type != OBJ_NONE)
		{
			Viewer = &objp;
			return;
		}
	}

	Error( "Could not find a viewer object!" );

}
#endif

imobjptridx_t obj_find_first_of_type(fvmobjptridx &vmobjptridx, const object_type_t type)
{
	range_for (const auto &&i, vmobjptridx)
	{
		if (i->type==type)
			return i;
	}
	return object_none;
}

}

namespace dcx {

icobjidx_t laser_info::get_last_hitobj() const
{
	if (!hitobj_count)
		/* If no elements, return object_none */
		return object_none;
	/* Return the most recently written element.  `hitobj_pos`
	 * indicates the element to write next, so return
	 * hitobj_values[hitobj_pos - 1].  When hitobj_pos == 0, the
	 * most recently written element is at the end of the array, not
	 * before the beginning of the array.
	 */
	if (!hitobj_pos)
		return hitobj_values.back();
	return hitobj_values[hitobj_pos - 1];
}

//draw an object that has one bitmap & doesn't rotate
void draw_object_blob(grs_canvas &canvas, const object_base &obj, const bitmap_index bmi)
{
	auto &bm = GameBitmaps[bmi.index];
	PIGGY_PAGE_IN( bmi );

	const auto osize = obj.size;
	// draw these with slight offset to viewer preventing too much ugly clipping
	auto pos = obj.pos;
	if (obj.type == OBJ_FIREBALL && get_fireball_id(obj) == VCLIP_VOLATILE_WALL_HIT)
	{
		vms_vector offs_vec;
		vm_vec_normalized_dir_quick(offs_vec, Viewer->pos, pos);
		vm_vec_scale_add2(pos,offs_vec,F1_0);
	}

	using wh = std::pair<fix, fix>;
	const auto bm_w = bm.bm_w;
	const auto bm_h = bm.bm_h;
	const auto p = (bm_w > bm_h)
		? wh(osize, fixmuldiv(osize, bm_h, bm_w))
		: wh(fixmuldiv(osize, bm_w, bm_h), osize);
	g3_draw_bitmap(canvas, pos, p.first, p.second, bm);
}

}

namespace dsx {

//draw an object that is a texture-mapped rod
void draw_object_tmap_rod(grs_canvas &canvas, const d_level_unique_light_state *const LevelUniqueLightState, const vcobjptridx_t obj, const bitmap_index bitmapi)
{
	g3s_lrgb light;
	PIGGY_PAGE_IN(bitmapi);

	auto &bitmap = GameBitmaps[bitmapi.index];

	const auto delta = vm_vec_copy_scale(obj->orient.uvec,obj->size);

	const auto top_v = vm_vec_add(obj->pos,delta);
	const auto bot_v = vm_vec_sub(obj->pos,delta);

	const auto top_p = g3_rotate_point(top_v);
	const auto bot_p = g3_rotate_point(bot_v);

	if (LevelUniqueLightState)
	{
		light = compute_object_light(*LevelUniqueLightState, obj);
	}
	else
	{
		light.r = light.g = light.b = f1_0;
	}
	g3_draw_rod_tmap(canvas, bitmap, bot_p, obj->size, top_p, obj->size, light);
}

//used for robot engine glow
#define MAX_VELOCITY i2f(50)

//what darkening level to use when cloaked
#define CLOAKED_FADE_LEVEL		28

#define	CLOAK_FADEIN_DURATION_PLAYER	F2_0
#define	CLOAK_FADEOUT_DURATION_PLAYER	F2_0

#define	CLOAK_FADEIN_DURATION_ROBOT	F1_0
#define	CLOAK_FADEOUT_DURATION_ROBOT	F1_0

//do special cloaked render
static void draw_cloaked_object(grs_canvas &canvas, const object_base &obj, const g3s_lrgb light, glow_values_t glow, const fix64 cloak_start_time, const fix total_cloaked_time, const fix Cloak_fadein_duration, const fix Cloak_fadeout_duration)
{
	fix cloak_delta_time;
	fix light_scale=F1_0;
	int cloak_value=0;
	int fading=0;		//if true, fading, else cloaking

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

	} else if (GameTime64 < (cloak_start_time + total_cloaked_time) -Cloak_fadeout_duration) {
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
	
	} else if (GameTime64 < (cloak_start_time + total_cloaked_time) -Cloak_fadeout_duration/2) {

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
		const unsigned ati = static_cast<unsigned>(obj.rtype.pobj_info.alt_textures) - 1;
		if (ati < multi_player_textures.size())
		{
			alt_textures = multi_player_textures[ati];
		}
	}

	if (fading) {
		g3s_lrgb new_light;

		new_light.r = fixmul(light.r,light_scale);
		new_light.g = fixmul(light.g,light_scale);
		new_light.b = fixmul(light.b,light_scale);
		glow[0] = fixmul(glow[0],light_scale);
		draw_polygon_model(canvas, obj.pos,
				   obj.orient,
				   obj.rtype.pobj_info.anim_angles,
				   obj.rtype.pobj_info.model_num, obj.rtype.pobj_info.subobj_flags,
				   new_light,
				   &glow,
				   alt_textures );
	}
	else {
		gr_settransblend(canvas, cloak_value, gr_blend::normal);
		g3_set_special_render(draw_tmap_flat);		//use special flat drawer
		draw_polygon_model(canvas, obj.pos,
				   obj.orient,
				   obj.rtype.pobj_info.anim_angles,
				   obj.rtype.pobj_info.model_num, obj.rtype.pobj_info.subobj_flags,
				   light,
				   &glow,
				   alt_textures );
		g3_set_special_render(draw_tmap);
		gr_settransblend(canvas, GR_FADE_OFF, gr_blend::normal);
	}

}

//draw an object which renders as a polygon model
static void draw_polygon_object(grs_canvas &canvas, const d_level_unique_light_state &LevelUniqueLightState, const vcobjptridx_t obj)
{
	auto &BossUniqueState = LevelUniqueObjectState.BossState;
	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	g3s_lrgb light;
	glow_values_t engine_glow_value;
	engine_glow_value[0] = 0;
#if defined(DXX_BUILD_DESCENT_II)
	engine_glow_value[1] = -1;		//element 0 is for engine glow, 1 for headlight
#endif

	//	If option set for bright players in netgame, brighten them!
	light = unlikely(Netgame.BrightPlayers && (Game_mode & GM_MULTI) && obj->type == OBJ_PLAYER)
		? g3s_lrgb{F1_0 * 2, F1_0 * 2, F1_0 * 2}
		: compute_object_light(LevelUniqueLightState, obj);

#if defined(DXX_BUILD_DESCENT_II)
	//make robots brighter according to robot glow field
	if (obj->type == OBJ_ROBOT)
	{
		const auto glow = Robot_info[get_robot_id(obj)].glow<<12;
		light.r += glow; //convert 4:4 to 16:16
		light.g += glow; //convert 4:4 to 16:16
		light.b += glow; //convert 4:4 to 16:16
	}

	if ((obj->type == OBJ_WEAPON &&
			get_weapon_id(obj) == weapon_id_type::FLARE_ID) ||
		obj->type == OBJ_MARKER
		)
		{
			light.r += F1_0*2;
			light.g += F1_0*2;
			light.b += F1_0*2;
		}
#endif

	push_interpolation_method imsave(1, true);

	//set engine glow value
	engine_glow_value[0] = f1_0/5;
	if (obj->movement_source == object::movement_type::physics) {

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
		auto &player_flags = obj->ctype.player_info.powerup_flags;
		if (player_flags & PLAYER_FLAGS_HEADLIGHT && !Endlevel_sequence)
			if (player_flags & PLAYER_FLAGS_HEADLIGHT_ON)
				engine_glow_value[1] = -2;		//draw white!
			else
				engine_glow_value[1] = -1;		//draw normal color (grey)
		else
			engine_glow_value[1] = -3;			//don't draw
	}
#endif

	if (obj->rtype.pobj_info.tmap_override != -1) {
		std::array<bitmap_index, 12> bm_ptrs;

		//fill whole array, in case simple model needs more
		bm_ptrs.fill(Textures[obj->rtype.pobj_info.tmap_override]);
		draw_polygon_model(canvas, obj->pos,
				   obj->orient,
				   obj->rtype.pobj_info.anim_angles,
				   obj->rtype.pobj_info.model_num,
				   obj->rtype.pobj_info.subobj_flags,
				   light,
				   &engine_glow_value,
				   bm_ptrs);
	}
	else {
		std::pair<fix64, fix> cloak_duration;
		std::pair<fix, fix> cloak_fade;
		if (obj->type==OBJ_PLAYER && (obj->ctype.player_info.powerup_flags & PLAYER_FLAGS_CLOAKED))
		{
			auto &cloak_time = obj->ctype.player_info.cloak_time;
			cloak_duration = {cloak_time, CLOAK_TIME_MAX};
			cloak_fade = {CLOAK_FADEIN_DURATION_PLAYER, CLOAK_FADEOUT_DURATION_PLAYER};
		}
		else if ((obj->type == OBJ_ROBOT) && (obj->ctype.ai_info.CLOAKED)) {
			if (Robot_info[get_robot_id(obj)].boss_flag)
				cloak_duration = {BossUniqueState.Boss_cloak_start_time, Boss_cloak_duration};
			else
				cloak_duration = {GameTime64-F1_0*10, F1_0 * 20};
			cloak_fade = {CLOAK_FADEIN_DURATION_ROBOT, CLOAK_FADEOUT_DURATION_ROBOT};
		} else {
			alternate_textures alt_textures;
			const unsigned ati = static_cast<unsigned>(obj->rtype.pobj_info.alt_textures) - 1;
			if (ati < multi_player_textures.size())
				alt_textures = multi_player_textures[ati];

#if defined(DXX_BUILD_DESCENT_II)
			if (obj->type == OBJ_ROBOT)
			{
			//	Snipers get bright when they fire.
				if (obj->ctype.ai_info.ail.next_fire < F1_0/8) {
				if (obj->ctype.ai_info.behavior == ai_behavior::AIB_SNIPE)
				{
					light.r = 2*light.r + F1_0;
					light.g = 2*light.g + F1_0;
					light.b = 2*light.b + F1_0;
				}
			}
			}
#endif

			const auto is_weapon_with_inner_model = (obj->type == OBJ_WEAPON && Weapon_info[get_weapon_id(obj)].model_num_inner > -1);
			bool draw_simple_model;
			if (is_weapon_with_inner_model)
			{
				gr_settransblend(canvas, GR_FADE_OFF, gr_blend::additive_a);
				draw_simple_model = static_cast<fix>(vm_vec_dist_quick(Viewer->pos, obj->pos)) < Simple_model_threshhold_scale * F1_0*2;
				if (draw_simple_model)
					draw_polygon_model(canvas, obj->pos,
							   obj->orient,
							   obj->rtype.pobj_info.anim_angles,
							   Weapon_info[get_weapon_id(obj)].model_num_inner,
							   obj->rtype.pobj_info.subobj_flags,
							   light,
							   &engine_glow_value,
							   alt_textures);
			}
			
			draw_polygon_model(canvas, obj->pos,
					   obj->orient,
					   obj->rtype.pobj_info.anim_angles,obj->rtype.pobj_info.model_num,
					   obj->rtype.pobj_info.subobj_flags,
					   light,
					   &engine_glow_value,
					   alt_textures);

			if (is_weapon_with_inner_model)
			{
#if !DXX_USE_OGL // in software rendering must draw inner model last
				gr_settransblend(canvas, GR_FADE_OFF, gr_blend::additive_a);
				if (draw_simple_model)
					draw_polygon_model(canvas, obj->pos,
							   obj->orient,
							   obj->rtype.pobj_info.anim_angles,
							   Weapon_info[obj->id].model_num_inner,
							   obj->rtype.pobj_info.subobj_flags,
							   light,
							   &engine_glow_value,
							   alt_textures);
#endif
				gr_settransblend(canvas, GR_FADE_OFF, gr_blend::normal);
			}
			return;
		}
		draw_cloaked_object(canvas, obj, light, engine_glow_value, cloak_duration.first, cloak_duration.second, cloak_fade.first, cloak_fade.second);
	}
}

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

namespace dcx {
objnum_t	Player_fired_laser_this_frame=object_none;

static bool predicate_debris(const object_base &o)
{
	return o.type == OBJ_DEBRIS;
}

static bool predicate_flare(const object_base &o)
{
	return (o.type == OBJ_WEAPON) && (get_weapon_id(o) == weapon_id_type::FLARE_ID);
}

static bool predicate_nonflare_weapon(const object_base &o)
{
	return (o.type == OBJ_WEAPON) && (get_weapon_id(o) != weapon_id_type::FLARE_ID);
}

}



namespace dsx {

static bool predicate_fireball(const object &o)
{
	return o.type == OBJ_FIREBALL && o.ctype.expl_info.delete_objnum == object_none;
}

// -----------------------------------------------------------------------------
//this routine checks to see if an robot rendered near the middle of
//the screen, and if so and the player had fired, "warns" the robot
static void set_robot_location_info(object &objp)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	if (Player_fired_laser_this_frame != object_none) {
		const auto &&temp = g3_rotate_point(objp.pos);
		if (temp.p3_codes & CC_BEHIND)		//robot behind the screen
			return;

		//the code below to check for object near the center of the screen
		//completely ignores z, which may not be good

		if ((abs(temp.p3_x) < F1_0*4) && (abs(temp.p3_y) < F1_0*4)) {
			objp.ctype.ai_info.danger_laser_num = Player_fired_laser_this_frame;
			objp.ctype.ai_info.danger_laser_signature = vcobjptr(Player_fired_laser_this_frame)->signature;
		}
	}
}

//	------------------------------------------------------------------------------------------------------------------
void create_small_fireball_on_object(const vmobjptridx_t objp, fix size_scale, int sound_flag)
{
	auto &Objects = LevelUniqueObjectState.Objects;
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

	const auto &&segnum = find_point_seg(LevelSharedSegmentState, LevelUniqueSegmentState, pos, Segments.vmptridx(objp->segnum));
	if (segnum != segment_none) {
		auto expl_obj = object_create_explosion(segnum, pos, size, VCLIP_SMALL_EXPLOSION);
		if (!expl_obj)
			return;
		obj_attach(Objects, objp, expl_obj);
		if (d_rand() < 8192) {
			fix	vol = F1_0/2;
			if (objp->type == OBJ_ROBOT)
				vol *= 2;
			if (sound_flag)
				digi_link_sound_to_object(SOUND_EXPLODING_WALL, objp, 0, vol, sound_stack::allow_stacking);
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
void render_object(grs_canvas &canvas, const d_level_unique_light_state &LevelUniqueLightState, const vmobjptridx_t obj)
{
	if (unlikely(obj == Viewer))
		return;
	if (unlikely(obj->type==OBJ_NONE))
	{
		Int3();
		return;
	}

#if !DXX_USE_OGL
	const auto mld_save = std::exchange(Max_linear_depth, Max_linear_depth_objects);
#endif

	bool alpha = false;
	switch (obj->render_type)
	{
		case RT_NONE:
			break; //doesn't render, like the player

		case RT_POLYOBJ:
#if defined(DXX_BUILD_DESCENT_II)
			if ( PlayerCfg.AlphaBlendMarkers && obj->type == OBJ_MARKER ) // set nice transparency/blending for certrain objects
			{
				alpha = true;
				gr_settransblend(canvas, 10, gr_blend::additive_a);
			}
#endif
			draw_polygon_object(canvas, LevelUniqueLightState, obj);

			if (obj->type == OBJ_ROBOT) //"warn" robot if being shot at
				set_robot_location_info(obj);
			break;

		case RT_MORPH:
			draw_morph_object(canvas, LevelUniqueLightState, obj);
			break;

		case RT_FIREBALL:
			if (PlayerCfg.AlphaBlendFireballs) // set nice transparency/blending for certrain objects
			{
				alpha = true;
				gr_settransblend(canvas, GR_FADE_OFF, gr_blend::additive_c);
			}

			draw_fireball(Vclip, canvas, obj);
			break;

		case RT_WEAPON_VCLIP:
			if (PlayerCfg.AlphaBlendWeapons && (!is_proximity_bomb_or_any_smart_mine(get_weapon_id(obj))
                )) // set nice transparency/blending for certain objects
			{
				alpha = true;
				gr_settransblend(canvas, 7, gr_blend::additive_a);
			}

			draw_weapon_vclip(Vclip, Weapon_info, canvas, obj);
			break;

		case RT_HOSTAGE:
			draw_hostage(Vclip, canvas, LevelUniqueLightState, obj);
			break;

		case RT_POWERUP:
			if (PlayerCfg.AlphaBlendPowerups) // set nice transparency/blending for certrain objects
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
						alpha = true;
						gr_settransblend(canvas, 7, gr_blend::additive_a);
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

			draw_powerup(Vclip, canvas, obj);
			break;

		case RT_LASER:
			if (PlayerCfg.AlphaBlendLasers) // set nice transparency/blending for certrain objects
			{
				alpha = true;
				gr_settransblend(canvas, 7, gr_blend::additive_a);
			}

			Laser_render(canvas, obj);
			break;

		default:
			Error("Unknown render_type <%d>",obj->render_type);
	}

	if (alpha)
		gr_settransblend(canvas, GR_FADE_OFF, gr_blend::normal); // revert any transparency/blending setting back to normal

	if ( obj->render_type != RT_NONE && Newdemo_state == ND_STATE_RECORDING )
		newdemo_record_render_object(obj);
#if !DXX_USE_OGL
	Max_linear_depth = mld_save;
#endif
}

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
	ConsoleObject->mtype.phys_info.flags = PF_TURNROLL | PF_LEVELLING | PF_WIGGLE | PF_USES_THRUST;

	//Init render info

	ConsoleObject->render_type = RT_POLYOBJ;
	ConsoleObject->rtype.pobj_info.model_num = Player_ship->model_num;		//what model is this?
	ConsoleObject->rtype.pobj_info.subobj_flags = 0;		//zero the flags
	ConsoleObject->rtype.pobj_info.tmap_override = -1;		//no tmap override!
	ConsoleObject->rtype.pobj_info.anim_angles = {};

	// Clear misc

	ConsoleObject->flags = 0;
}


//make object0 the player, setting all relevant fields
void init_player_object()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	const auto &&console = vmobjptr(ConsoleObject);
	console->type = OBJ_PLAYER;
	set_player_id(console, 0);					//no sub-types for player
	console->signature = object_signature_t{0};
	auto &Polygon_models = LevelSharedPolygonModelState.Polygon_models;
	console->size = Polygon_models[Player_ship->model_num].rad;
	console->control_source = object::control_type::slew;			//default is player slewing
	console->movement_source = object::movement_type::physics;		//change this sometime
	console->lifeleft = IMMORTAL_TIME;
	console->attached_obj = object_none;
	reset_player_object();
}

//sets up the free list & init player & whatever else
void init_objects()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	for (objnum_t i = 0; i< MAX_OBJECTS; ++i)
	{
		LevelUniqueObjectState.free_obj_list[i] = i;
		auto &obj = *vmobjptr(i);
		DXX_POISON_VAR(obj, 0xfd);
		obj.type = OBJ_NONE;
	}

	range_for (unique_segment &j, Segments)
		j.objects = object_none;

	Viewer = ConsoleObject = &Objects.front();

	init_player_object();
	obj_link_unchecked(Objects.vmptr, Objects.vmptridx(ConsoleObject), Segments.vmptridx(segment_first));	//put in the world in segment 0
	LevelUniqueObjectState.num_objects = 1;						//just the player
	Objects.set_count(1);
}

//after calling init_object(), the network code has grabbed specific
//object slots without allocating them.  Go though the objects & build
//the free list, then set the apporpriate globals
void special_reset_objects(d_level_unique_object_state &LevelUniqueObjectState)
{
	unsigned num_objects = MAX_OBJECTS;

	auto &Objects = LevelUniqueObjectState.get_objects();
	Objects.set_count(1);
	assert(Objects.front().type != OBJ_NONE);		//0 should be used

	DXX_POISON_VAR(LevelUniqueObjectState.free_obj_list, 0xfd);
	for (objnum_t i = MAX_OBJECTS; i--;)
		if (Objects.vcptr(i)->type == OBJ_NONE)
			LevelUniqueObjectState.free_obj_list[--num_objects] = i;
		else
			if (i > Highest_object_index)
				Objects.set_count(i + 1);
	LevelUniqueObjectState.num_objects = num_objects;
}

//link the object into the list for its segment
void obj_link(fvmobjptr &vmobjptr, const vmobjptridx_t obj, const vmsegptridx_t segnum)
{
	assert(obj->segnum == segment_none);
	assert(obj->next == object_none);
	assert(obj->prev == object_none);
	obj_link_unchecked(vmobjptr, obj, segnum);
}

void obj_link_unchecked(fvmobjptr &vmobjptr, const vmobjptridx_t obj, const vmsegptridx_t segnum)
{
	obj->segnum = segnum;
	unique_segment &useg = segnum;
	obj->next = std::exchange(useg.objects, obj);
	obj->prev = object_none;

	if (obj->next != object_none)
		vmobjptr(obj->next)->prev = obj;
}

void obj_unlink(fvmobjptr &vmobjptr, fvmsegptr &vmsegptr, object_base &obj)
{
	const auto next = obj.next;
	/* It is a bug elsewhere if vmsegptr ever fails here.  However, it is
	 * expensive to check, so only force verification in debug builds.
	 *
	 * In debug builds, always compute it, for the side effect of
	 * validating the segment number.
	 *
	 * In release builds, compute it when it is needed.
	 */
#ifndef NDEBUG
	unique_segment &segp = vmsegptr(obj.segnum);
#endif
	((obj.prev == object_none)
		? (
#ifdef NDEBUG
			static_cast<unique_segment &>(vmsegptr(obj.segnum))
#else
			segp
#endif
		).objects
		: vmobjptr(obj.prev)->next) = next;

	obj.segnum = segment_none;

	if (next != object_none)
		vmobjptr(next)->prev = obj.prev;
	DXX_POISON_VAR(obj.next, 0xfa);
	DXX_POISON_VAR(obj.prev, 0xfa);
}

//returns the number of a free object, updating Highest_object_index.
//Generally, obj_create() should be called to get an object, since it
//fills in important fields and does the linking.
//returns -1 if no free objects
imobjptridx_t obj_allocate(d_level_unique_object_state &LevelUniqueObjectState)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	if (LevelUniqueObjectState.num_objects >= Objects.size())
		return object_none;

	const auto objnum = LevelUniqueObjectState.free_obj_list[LevelUniqueObjectState.num_objects++];
	if (objnum >= Objects.get_count())
	{
		Objects.set_count(objnum + 1);
	}
	const auto &&r = Objects.vmptridx(objnum);
	assert(r->type == OBJ_NONE);
	return r;
}

//frees up an object.  Generally, obj_delete() should be called to get
//rid of an object.  This function deallocates the object entry after
//the object has been unlinked
static void obj_free(d_level_unique_object_state &LevelUniqueObjectState, const vmobjidx_t objnum)
{
	const auto num_objects = -- LevelUniqueObjectState.num_objects;
	assert(num_objects < LevelUniqueObjectState.free_obj_list.size());
	LevelUniqueObjectState.free_obj_list[num_objects] = objnum;
	auto &Objects = LevelUniqueObjectState.get_objects();

	objnum_t o = objnum;
	if (o == Highest_object_index)
	{
		for (;;)
		{
			--o;
			if (Objects.vcptr(o)->type != OBJ_NONE)
				break;
			if (o == 0)
				break;
		}
		Objects.set_count(o + 1);
	}
}

//-----------------------------------------------------------------------------
//	Scan the object list, freeing down to num_used objects
//	Returns number of slots freed.
static void free_object_slots(uint_fast32_t num_used)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	std::array<object *, MAX_OBJECTS>	obj_list;
	unsigned	num_already_free, num_to_free, olind = 0;

	num_already_free = MAX_OBJECTS - Highest_object_index - 1;

	if (MAX_OBJECTS - num_already_free < num_used)
		return;

	range_for (const auto &&objp, vmobjptr)
	{
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
				case OBJ_COOP:
				case OBJ_MARKER:
					break;
			}

	}

	num_to_free = MAX_OBJECTS - num_used - num_already_free;

	if (num_to_free > olind) {
		num_to_free = olind;
	}

	// Capture before num_to_free modified
	const auto &&r = partial_const_range(obj_list, num_to_free);
	auto l = [&vmobjptr, &r, &num_to_free](const auto predicate) -> bool {
		range_for (const auto i, r)
		{
			auto &o = *vmobjptr(i);
			if (predicate(o))
			{
				o.flags |= OF_SHOULD_BE_DEAD;
				if (!-- num_to_free)
					return true;
			}
		}
		return false;
	};

	if (l(predicate_debris))
		return;

	if (l(predicate_fireball))
		return;

	if (l(predicate_flare))
		return;

	if (l(predicate_nonflare_weapon))
		return;
}

//-----------------------------------------------------------------------------
//initialize a new object.  adds to the list for the given segment
//note that segnum is really just a suggestion, since this routine actually
//searches for the correct segment
//returns the object number
imobjptridx_t obj_create(const object_type_t type, const unsigned id, vmsegptridx_t segnum, const vms_vector &pos, const vms_matrix *const orient, const fix size, const typename object::control_type ctype, const typename object::movement_type mtype, const render_type_t rtype)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &Vertices = LevelSharedVertexState.get_vertices();
	// Some consistency checking. FIXME: Add more debug output here to probably trace all possible occurances back.
	assert(ctype <= object::control_type::cntrlcen);

	if (type == OBJ_DEBRIS && LevelUniqueObjectState.Debris_object_count >= Max_debris_objects && !PERSISTENT_DEBRIS)
		return object_none;

	auto &vcvertptr = Vertices.vcptr;
	if (get_seg_masks(vcvertptr, pos, segnum, 0).centermask != 0)
	{
		const auto &&p = find_point_seg(LevelSharedSegmentState, LevelUniqueSegmentState, pos, segnum);
		if (p == segment_none) {
			return object_none;		//don't create this object
		}
		segnum = p;
	}

	// Find next free object
	const auto &&obj = obj_allocate(LevelUniqueObjectState);

	if (obj == object_none)		//no free objects
		return object_none;

	// Zero out object structure to keep weird bugs from happening
	// in uninitialized fields.
	const auto signature = obj->signature;
	/* Test the version in the object structure, not the local copy.
	 * This produces a more useful diagnostic from Valgrind if the
	 * test reports a problem.
	 */
	DXX_CHECK_VAR_IS_DEFINED(obj->signature);
	*obj = {};
	// Tell Valgrind to warn on any uninitialized fields.
	DXX_POISON_VAR(*obj, 0xfd);

	obj->signature = next(signature);
	obj->type 				= type;
	obj->id 				= id;
	obj->pos 				= pos;
	obj->size 				= size;
	obj->flags 				= 0;
	//@@if (orient != NULL)
	//@@	obj->orient 			= *orient;

	obj->orient 				= orient?*orient:vmd_identity_matrix;

	obj->control_source 		        = ctype;
	obj->movement_source = mtype;
	obj->render_type 			= rtype;
        obj->contains_count                     = 0;
        obj->matcen_creator                     = 0;
	obj->lifeleft 				= IMMORTAL_TIME;		//assume immortal
	obj->attached_obj			= object_none;

	if (obj->control_source == object::control_type::powerup)
        {
		obj->ctype.powerup_info.count = 1;
                obj->ctype.powerup_info.flags = 0;
                obj->ctype.powerup_info.creation_time = GameTime64;
        }

	// Init physics info for this object
	if (obj->movement_source == object::movement_type::physics) {
		obj->mtype.phys_info = {};
	}

	if (obj->render_type == RT_POLYOBJ)
        {
                obj->rtype.pobj_info.subobj_flags = 0;
		obj->rtype.pobj_info.tmap_override = -1;
                obj->rtype.pobj_info.alt_textures = 0;
        }

	obj->shields 				= 20*F1_0;

	{
		const auto &&p = find_point_seg(LevelSharedSegmentState, LevelUniqueSegmentState, pos, segnum);		//find correct segment
		// Previously this was only an assert check.  Now it is also
		// checked at runtime.
		segnum = p;
	}

	obj_link_unchecked(Objects.vmptr, obj, segnum);

	//	Set (or not) persistent bit in phys_info.
	if (obj->type == OBJ_WEAPON) {
		assert(obj->control_source == object::control_type::weapon);
		obj->mtype.phys_info.flags |= (Weapon_info[get_weapon_id(obj)].persistent*PF_PERSISTENT);
		obj->ctype.laser_info.creation_time = GameTime64;
		obj->ctype.laser_info.clear_hitobj();
		obj->ctype.laser_info.multiplier = F1_0;
#if defined(DXX_BUILD_DESCENT_II)
		obj->ctype.laser_info.last_afterburner_time = 0;
#endif
	}

	if (obj->control_source == object::control_type::explosion)
		obj->ctype.expl_info.next_attach = obj->ctype.expl_info.prev_attach = obj->ctype.expl_info.attach_parent = object_none;

	if (obj->type == OBJ_DEBRIS)
		++ LevelUniqueObjectState.Debris_object_count;

	return obj;
}

//create a copy of an object. returns new object number
imobjptridx_t obj_create_copy(const object &srcobj, const vmsegptridx_t newsegnum)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	// Find next free object
	const auto &&obj = obj_allocate(LevelUniqueObjectState);

	if (obj == object_none)
		return object_none;

	const auto signature = obj->signature;
	*obj = srcobj;

	obj_link_unchecked(Objects.vmptr, obj, newsegnum);
	obj->signature = next(signature);

	//we probably should initialize sub-structures here

	return obj;
}

//remove object from the world
void obj_delete(d_level_unique_object_state &LevelUniqueObjectState, segment_array &Segments, const vmobjptridx_t obj)
{
	auto &Objects = LevelUniqueObjectState.get_objects();
	Assert(obj->type != OBJ_NONE);
	Assert(obj != ConsoleObject);

#if defined(DXX_BUILD_DESCENT_II)
	if (obj->type==OBJ_WEAPON && get_weapon_id(obj)==weapon_id_type::GUIDEDMISS_ID && obj->ctype.laser_info.parent_type==OBJ_PLAYER)
	{
		const auto pnum = get_player_id(Objects.vcptr(obj->ctype.laser_info.parent_num));
		const auto &&gimobj = LevelUniqueObjectState.Guided_missile.get_player_active_guided_missile(Objects.vmptridx, pnum);
		if (gimobj == obj)
		{
			LevelUniqueObjectState.Guided_missile.clear_player_active_guided_missile(pnum);
			if (pnum == Player_num)
			{
				if (!PlayerCfg.GuidedInBigWindow)
					do_cockpit_window_view(gauge_inset_window_view::secondary, weapon_box_user::post_missile_static);
				if (Newdemo_state == ND_STATE_RECORDING)
					newdemo_record_guided_end();
			}
		}
	}
#endif

	if (obj == Viewer)		//deleting the viewer?
		Viewer = ConsoleObject;						//..make the player the viewer

	if (obj->flags & OF_ATTACHED)		//detach this from object
		obj_detach_one(Objects, obj);

	if (obj->attached_obj != object_none)		//detach all objects from this
		obj_detach_all(Objects, obj);

	if (obj->type == OBJ_DEBRIS)
		-- LevelUniqueObjectState.Debris_object_count;

	if (obj->movement_source == object::movement_type::physics && (obj->mtype.phys_info.flags & PF_STICK))
		LevelUniqueStuckObjectState.remove_stuck_object(obj);
	obj_unlink(Objects.vmptr, Segments.vmptr, obj);
	const auto signature = obj->signature;
	DXX_POISON_VAR(*obj, 0xfa);
	obj->type = OBJ_NONE;		//unused!
	/* Preserve signature across the poison value.  When the object slot
	 * is reused, the allocator will need the old signature so that the
	 * new one can be derived from it.  No other sites should read it
	 * until that happens.
	 */
	obj->signature = signature;
	obj_free(LevelUniqueObjectState, obj);
}

#define	DEATH_SEQUENCE_LENGTH			(F1_0*5)
#define	DEATH_SEQUENCE_EXPLODE_TIME	(F1_0*2)

object	*Dead_player_camera = NULL;	//	Object index of object watching deader.
static const object *Viewer_save;
}
namespace dcx {
player_dead_state Player_dead_state = player_dead_state::no;			//	If !0, then player is dead, but game continues so he can watch.
static int Player_flags_save;
static fix Camera_to_player_dist_goal = F1_0*4;
static typename object::control_type Control_type_save;
static render_type_t Render_type_save;

unsigned laser_parent_is_matching_signature(const laser_parent &l, const object_base &o)
{
	if (l.parent_type != o.type)
		return 0;
	return l.parent_signature == o.signature;
}

}

namespace dsx {
//	------------------------------------------------------------------------------------------------------------------
void dead_player_end(void)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	if (Player_dead_state == player_dead_state::no)
		return;

	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_restore_cockpit();

	Player_dead_state = player_dead_state::no;
	obj_delete(LevelUniqueObjectState, Segments, vmobjptridx(Dead_player_camera));
	Dead_player_camera = NULL;
	select_cockpit(PlayerCfg.CockpitMode[0]);
	Viewer = Viewer_save;
	ConsoleObject->type = OBJ_PLAYER;
	ConsoleObject->flags = Player_flags_save;

	assert(Control_type_save == object::control_type::flying || Control_type_save == object::control_type::slew);

	ConsoleObject->control_source = Control_type_save;
	ConsoleObject->render_type = Render_type_save;
	auto &player_info = ConsoleObject->ctype.player_info;
	player_info.powerup_flags &= ~PLAYER_FLAGS_INVULNERABLE;
	player_info.Player_eggs_dropped = false;
}

//	------------------------------------------------------------------------------------------------------------------
//	Camera is less than size of player away from
static void set_camera_pos(vms_vector &camera_pos, const vcobjptridx_t objp)
{
	int	count = 0;
	fix	camera_player_dist;
	fix	far_scale;

	camera_player_dist = vm_vec_dist_quick(camera_pos, objp->pos);

	if (camera_player_dist < Camera_to_player_dist_goal) {
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
window_event_result dead_player_frame()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptridx = Objects.vcptridx;
	auto &vmobjptr = Objects.vmptr;
	auto &vmobjptridx = Objects.vmptridx;
	static fix	time_dead = 0;

	if (Player_dead_state != player_dead_state::no)
	{
		time_dead += FrameTime;

		//	If unable to create camera at time of death, create now.
		if (Dead_player_camera == Viewer_save) {
			const auto &player = get_local_plrobj();
			const auto &&objnum = obj_create(OBJ_CAMERA, 0, vmsegptridx(player.segnum), player.pos, &player.orient, 0, object::control_type::None, object::movement_type::None, RT_NONE);

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

		set_camera_pos(Dead_player_camera->pos, vcobjptridx(ConsoleObject));

		// the following line uncommented by WraithX, 4-12-00
		if (time_dead < DEATH_SEQUENCE_EXPLODE_TIME + F1_0 * 2)
		{
			const auto fvec = vm_vec_sub(ConsoleObject->pos, Dead_player_camera->pos);
			vm_vector_2_matrix(Dead_player_camera->orient, fvec, nullptr, nullptr);
			Dead_player_camera->mtype.phys_info = ConsoleObject->mtype.phys_info;

			// the following "if" added by WraithX to get rid of camera "wiggle"
			Dead_player_camera->mtype.phys_info.flags &= ~PF_WIGGLE;
			// end "if" added by WraithX, 4/13/00

		// the following line uncommented by WraithX, 4-12-00
		}
		else
		{
			// the following line uncommented by WraithX, 4-11-00
			Dead_player_camera->movement_source = object::movement_type::physics;
			//Dead_player_camera->mtype.phys_info.rotvel.y = F1_0/8;
		// the following line uncommented by WraithX, 4-12-00
		}
		// end addition by WX

		if (time_dead > DEATH_SEQUENCE_EXPLODE_TIME) {
			if (Player_dead_state != player_dead_state::exploded)
			{
				auto &player_info = get_local_plrobj().ctype.player_info;
				const auto hostages_lost = std::exchange(player_info.mission.hostages_on_board, 0);

				if (hostages_lost > 1)
					HUD_init_message(HM_DEFAULT, TXT_SHIP_DESTROYED_2, hostages_lost);
				else
					HUD_init_message_literal(HM_DEFAULT, hostages_lost == 1 ? TXT_SHIP_DESTROYED_1 : TXT_SHIP_DESTROYED_0);

				Player_dead_state = player_dead_state::exploded;
				
				const auto cobjp = vmobjptridx(ConsoleObject);
				drop_player_eggs(cobjp);
				player_info.Player_eggs_dropped = true;
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
				player_info.powerup_flags &= ~PLAYER_FLAGS_HEADLIGHT_ON;
#endif
			}
		} else {
			if (d_rand() < FrameTime*4) {
				if (Game_mode & GM_MULTI)
					multi_send_create_explosion(Player_num);
				create_small_fireball_on_object(vmobjptridx(ConsoleObject), F1_0, 1);
			}
		}


		if (GameViewUniqueState.Death_sequence_aborted)
		{
			auto &player_info = get_local_plrobj().ctype.player_info;
			if (!player_info.Player_eggs_dropped) {
				player_info.Player_eggs_dropped = true;
				drop_player_eggs(vmobjptridx(ConsoleObject));
				if (Game_mode & GM_MULTI)
				{
					multi_send_player_deres(deres_explode);
				}
			}

			return DoPlayerDead();		//kill_player();
		}
	}
	else
		time_dead = 0;

	return window_event_result::handled;
}

//	------------------------------------------------------------------------------------------------------------------
static void start_player_death_sequence(object &player)
{
	auto &Objects = LevelUniqueObjectState.Objects;
#if defined(DXX_BUILD_DESCENT_II)
	auto &vmobjptr = Objects.vmptr;
#endif
	auto &vmobjptridx = Objects.vmptridx;
	assert(&player == ConsoleObject);
	if (Player_dead_state != player_dead_state::no ||
		Dead_player_camera != NULL ||
		((Game_mode & GM_MULTI) && (get_local_player().connected != CONNECT_PLAYING)))
		return;

	//Assert(Dead_player_camera == NULL);

	reset_rear_view();

	if (!(Game_mode & GM_MULTI))
		HUD_clear_messages();

	GameViewUniqueState.Death_sequence_aborted = 0;

	if (Game_mode & GM_MULTI)
	{
#if defined(DXX_BUILD_DESCENT_II)
		// If Hoard, increase number of orbs by 1. Only if you haven't killed yourself. This prevents cheating
		if (game_mode_hoard())
		{
			auto &player_info = player.ctype.player_info;
			auto &proximity = player_info.hoard.orbs;
			if (proximity < player_info.max_hoard_orbs)
			{
				const auto is_bad_kill = [&vmobjptr]{
					auto &lplr = get_local_player();
					auto &lplrobj = get_local_plrobj();
					const auto killer_objnum = lplrobj.ctype.player_info.killer_objnum;
					if (killer_objnum == lplr.objnum)
						/* Self kill */
						return true;
					if (killer_objnum == object_none)
						/* Non-player kill */
						return true;
					const auto &&killer_objp = vmobjptr(killer_objnum);
					if (killer_objp->type != OBJ_PLAYER)
						return true;
					if (!(Game_mode & GM_TEAM))
						return false;
					return get_team(Player_num) == get_team(get_player_id(killer_objp));
				};
				if (!is_bad_kill())
					++ proximity;
			}
		}
#endif
		multi_send_kill(vmobjptridx(get_local_player().objnum));
	}
	
	PaletteRedAdd = 40;
	Player_dead_state = player_dead_state::yes;

	vm_vec_zero(player.mtype.phys_info.rotthrust);
	vm_vec_zero(player.mtype.phys_info.thrust);

	const auto &&objnum = obj_create(OBJ_CAMERA, 0, vmsegptridx(player.segnum), player.pos, &player.orient, 0, object::control_type::None, object::movement_type::None, RT_NONE);
	Viewer_save = Viewer;
	if (objnum != object_none)
		Viewer = Dead_player_camera = objnum;
	else {
		Int3();
		Dead_player_camera = ConsoleObject;
	}

	select_cockpit(CM_LETTERBOX);
	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_letterbox();

	Player_flags_save = player.flags;
	Control_type_save = player.control_source;
	Render_type_save = player.render_type;

	player.flags &= ~OF_SHOULD_BE_DEAD;
//	Players[Player_num].flags |= PLAYER_FLAGS_INVULNERABLE;
	player.control_source = object::control_type::None;

	PALETTE_FLASH_SET(0,0,0);
}

//	------------------------------------------------------------------------------------------------------------------
static void obj_delete_all_that_should_be_dead()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	objnum_t		local_dead_player_object=object_none;

	// Move all objects
	range_for (const auto &&objp, vmobjptridx)
	{
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
				obj_delete(LevelUniqueObjectState, Segments, objp);
			}
		}
	}
}

//when an object has moved into a new segment, this function unlinks it
//from its old segment, and links it into the new segment
void obj_relink(fvmobjptr &vmobjptr, fvmsegptr &vmsegptr, const vmobjptridx_t objnum, const vmsegptridx_t newsegnum)
{
	obj_unlink(vmobjptr, vmsegptr, objnum);
	obj_link_unchecked(vmobjptr, objnum, newsegnum);
}

// for getting out of messed up linking situations (i.e. caused by demo playback)
void obj_relink_all(void)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	for (unique_segment &useg : vmsegptr)
	{
		useg.objects = object_none;
	}

	range_for (const auto &&obj, Objects.vmptridx)
	{
		if (obj->type != OBJ_NONE)
		{
			auto segnum = obj->segnum;
			if (segnum > Highest_segment_index)
				segnum = segment_first;
			obj_link_unchecked(Objects.vmptr, obj, Segments.vmptridx(segnum));
		}
	}
}

//process a continuously-spinning object
static void spin_object(object_base &obj)
{
	vms_angvec rotangs;
	assert(obj.movement_source == object::movement_type::spinning);

	const fix frametime = FrameTime;
	rotangs.p = fixmul(obj.mtype.spin_rate.x, frametime);
	rotangs.h = fixmul(obj.mtype.spin_rate.y, frametime);
	rotangs.b = fixmul(obj.mtype.spin_rate.z, frametime);

	const auto &&rotmat = vm_angles_2_matrix(rotangs);
	obj.orient = vm_matrix_x_matrix(obj.orient, rotmat);
	check_and_fix_matrix(obj.orient);
}

#if defined(DXX_BUILD_DESCENT_II)
imobjidx_t d_guided_missile_indices::get_player_active_guided_missile(const unsigned pnum) const
{
	return operator[](pnum);
}

/* Place debug checks out of line so that they are shared among the
 * template instantiations.
 */
bool d_guided_missile_indices::debug_check_current_object(const object_base &obj)
{
	assert(obj.type == OBJ_WEAPON);
	const auto gmid = get_weapon_id(obj);
	if (obj.type != OBJ_WEAPON)
		return false;
	assert(gmid == weapon_id_type::GUIDEDMISS_ID);
	if (gmid != weapon_id_type::GUIDEDMISS_ID)
		return false;
	return true;
}

template <typename R, typename F>
R d_guided_missile_indices::get_player_active_guided_missile_tmpl(F &fobjptr, const unsigned pnum) const
{
	const auto gmidx = get_player_active_guided_missile(pnum);
	if (gmidx == object_none)
		return object_none;
	auto &&gmobj = fobjptr(gmidx);
	if (!debug_check_current_object(gmobj))
		return object_none;
	return gmobj;
}

imobjptr_t d_guided_missile_indices::get_player_active_guided_missile(fvmobjptr &vmobjptr, const unsigned pnum) const
{
	return this->template get_player_active_guided_missile_tmpl<imobjptr_t>(vmobjptr, pnum);
}

imobjptridx_t d_guided_missile_indices::get_player_active_guided_missile(fvmobjptridx &vmobjptridx, const unsigned pnum) const
{
	return this->template get_player_active_guided_missile_tmpl<imobjptridx_t>(vmobjptridx, pnum);
}

void d_guided_missile_indices::set_player_active_guided_missile(const vmobjidx_t obji, const unsigned pnum)
{
	auto &i = operator[](pnum);
	i = obji;
}

void d_guided_missile_indices::clear_player_active_guided_missile(const unsigned pnum)
{
	auto &i = operator[](pnum);
	i = object_none;
}

int Drop_afterburner_blob_flag;		//ugly hack
//see if wall is volatile, and if so, cause damage to player
//returns true if player is in lava
#endif

//--------------------------------------------------------------------
//move an object for the current frame
static window_event_result object_move_one(const vmobjptridx_t obj, control_info &Controls)
{
#if defined(DXX_BUILD_DESCENT_II)
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
#endif
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	const auto previous_segment = obj->segnum;
	auto result = window_event_result::handled;

	const auto obj_previous_position = obj->pos;			// Save the current position

	if ((obj->type==OBJ_PLAYER) && (Player_num==get_player_id(obj)))	{
		const auto &&segp = vmsegptr(obj->segnum);
#if defined(DXX_BUILD_DESCENT_II)
		if (game_mode_capture_flag())
			fuelcen_check_for_goal(obj, segp);
		else if (game_mode_hoard())
			fuelcen_check_for_hoard_goal(obj, segp);
#endif

		auto &player_info = obj->ctype.player_info;
		auto &energy = player_info.energy;
		const fix fuel = fuelcen_give_fuel(segp, INITIAL_ENERGY - energy);
		if (fuel > 0 )	{
			energy += fuel;
		}
#if defined(DXX_BUILD_DESCENT_II)
		auto &pl_shields = get_local_plrobj().shields;
		const fix shields = repaircen_give_shields(segp, INITIAL_SHIELDS - pl_shields);
		if (shields > 0) {
			pl_shields += shields;
		}
#endif
	}

	{
		auto lifeleft = obj->lifeleft;
		if (lifeleft != IMMORTAL_TIME) //if not immortal...
		{
			lifeleft -= FrameTime; //...inevitable countdown towards death
#if defined(DXX_BUILD_DESCENT_II)
			if (obj->type == OBJ_MARKER)
			{
				if (lifeleft < F1_0*1000)
					lifeleft += F1_0; // Make sure this object doesn't go out.
			}
#endif
			obj->lifeleft = lifeleft;
		}
	}
#if defined(DXX_BUILD_DESCENT_II)
	Drop_afterburner_blob_flag = 0;
#endif

	switch (obj->control_source) {

		case object::control_type::None:
			break;

		case object::control_type::flying:
			read_flying_controls(obj, Controls);
			break;

		case object::control_type::repaircen:
			Int3();
			// -- hey! these are no longer supported!! -- do_repair_sequence(obj);
			break;

		case object::control_type::powerup:
			do_powerup_frame(Vclip, obj);
			break;
	
		case object::control_type::morph:			//morph implies AI
			do_morph_frame(obj);
			//NOTE: FALLS INTO AI HERE!!!!
			DXX_BOOST_FALLTHROUGH;

		case object::control_type::ai:
			//NOTE LINK TO object::control_type::morph ABOVE!!!
			if (Game_suspended & SUSP_ROBOTS) return window_event_result::ignored;
			do_ai_frame(obj);
			break;

		case object::control_type::weapon:
			Laser_do_weapon_sequence(obj);
			break;
		case object::control_type::explosion:
			do_explosion_sequence(obj);
			break;

		case object::control_type::slew:
#ifdef RELEASE
			obj->control_source = object::control_type::None;
			con_printf(CON_URGENT, DXX_STRINGIZE_FL(__FILE__, __LINE__, "BUG: object %hu has control type object::control_type::slew, sig/type/id = %hu/%i/%i"), static_cast<objnum_t>(obj), static_cast<uint16_t>(obj->signature), obj->type, obj->id);
#else
			if ( keyd_pressed[KEY_PAD5] ) slew_stop();
			if ( keyd_pressed[KEY_NUMLOCK] ) 		{
				slew_reset_orient();
			}
			slew_frame(0 );		// Does velocity addition for us.
#endif
			break;

//		case object::control_type::flythrough:
//			do_flythrough(obj,0);			// HACK:do_flythrough should operate on an object!!!!
//			//check_object_seg(obj);
//			return;	// DON'T DO THE REST OF OBJECT STUFF SINCE THIS IS A SPECIAL CASE!!!
//			break;

		case object::control_type::debris:
			do_debris_frame(obj);
			break;

		case object::control_type::light:
			break;		//doesn't do anything

		case object::control_type::remote:
			break;		//doesn't do anything

		case object::control_type::cntrlcen:
			do_controlcen_frame(obj);
			break;

		default:
			Error("Unknown control type %u in object %hu, sig/type/id = %i/%i/%i", static_cast<unsigned>(obj->control_source), static_cast<objnum_t>(obj), static_cast<uint16_t>(obj->signature), obj->type, obj->id);
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
		return window_event_result::ignored;         // object has been deleted

	bool prepare_seglist = false;
	phys_visited_seglist phys_visited_segs;
	switch (obj->movement_source) {

		case object::movement_type::None:
			break;				//this doesn't move

		case object::movement_type::physics:	//move by physics
			result = do_physics_sim(obj, obj_previous_position, obj->type == OBJ_PLAYER ? (prepare_seglist = true, phys_visited_segs.nsegs = 0, &phys_visited_segs) : nullptr);
			break;

		case object::movement_type::spinning:
			spin_object(obj);
			break;
	}

#if defined(DXX_BUILD_DESCENT_II)
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcwallptr = Walls.vcptr;
#endif
	//	If player and moved to another segment, see if hit any triggers.
	// also check in player under a lavafall
	if (prepare_seglist)
	{
		if (previous_segment != obj->segnum && phys_visited_segs.nsegs > 1)
		{
			auto seg0 = vmsegptridx(phys_visited_segs.seglist[0]);
#if defined(DXX_BUILD_DESCENT_II)
			int	old_level = Current_level_num;
#endif
			range_for (const auto i, partial_const_range(phys_visited_segs.seglist, 1u, phys_visited_segs.nsegs))
			{
				const auto &&seg1 = seg0.absolute_sibling(i);
				const auto connect_side = find_connect_side(seg1, seg0);
				if (connect_side != side_none)
				{
					result = check_trigger(seg0, connect_side, get_local_plrobj(), obj, 0);
#if defined(DXX_BUILD_DESCENT_II)
				//maybe we've gone on to the next level.  if so, bail!
				if (Current_level_num != old_level)
					return window_event_result::ignored;
#endif
				}
				seg0 = seg1;
			}
		}
#if defined(DXX_BUILD_DESCENT_II)
		{
			bool under_lavafall = false;

			auto &playing = obj->ctype.player_info.lavafall_hiss_playing;
			const auto &&segp = vcsegptr(obj->segnum);
			auto &vcvertptr = Vertices.vcptr;
			if (const auto sidemask = get_seg_masks(vcvertptr, obj->pos, segp, obj->size).sidemask)
			{
				range_for (const unsigned sidenum, xrange(MAX_SIDES_PER_SEGMENT))
				{
					if (!(sidemask & (1 << sidenum)))
						continue;
					const auto wall_num = segp->shared_segment::sides[sidenum].wall_num;
					if (wall_num != wall_none && vcwallptr(wall_num)->type == WALL_ILLUSION)
					{
						const auto type = check_volatile_wall(obj, segp->unique_segment::sides[sidenum]);
						if (type != volatile_wall_result::none)
						{
							under_lavafall = 1;
							if (!playing)
							{
								playing = 1;
								const auto sound = (type == volatile_wall_result::lava) ? SOUND_LAVAFALL_HISS : SOUND_SHIP_IN_WATERFALL;
								digi_link_sound_to_object3(sound, obj, 1, F1_0, sound_stack::allow_stacking, vm_distance{i2f(256)}, -1, -1);
								break;
							}
						}
					}
				}
			}
	
			if (!under_lavafall && playing)
			{
				playing = 0;
				digi_kill_sound_linked_to_object( obj);
			}
		}
#endif
	}

#if defined(DXX_BUILD_DESCENT_II)
	//see if guided missile has flown through exit trigger
	if (obj == LevelUniqueObjectState.Guided_missile.get_player_active_guided_missile(Player_num))
	{
		if (previous_segment != obj->segnum) {
			const auto &&psegp = vcsegptr(previous_segment);
			const auto &&connect_side = find_connect_side(vcsegptridx(obj->segnum), psegp);
			if (connect_side != side_none)
			{
				const auto wall_num = psegp->shared_segment::sides[connect_side].wall_num;
				if ( wall_num != wall_none ) {
					auto trigger_num = vcwallptr(wall_num)->trigger;
					if (trigger_num != trigger_none)
					{
						auto &Triggers = LevelUniqueWallSubsystemState.Triggers;
						auto &vctrgptr = Triggers.vcptr;
						const auto &&t = vctrgptr(trigger_num);
						if (t->type == trigger_action::normal_exit)
							obj->lifeleft = 0;
					}
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

		assert(obj->control_source == object::control_type::weapon);
		if ((obj->ctype.laser_info.last_afterburner_time + delay < GameTime64) || (obj->ctype.laser_info.last_afterburner_time > GameTime64)) {
			drop_afterburner_blobs(obj, 1, i2f(Weapon_info[get_weapon_id(obj)].afterburner_size)/16, lifetime);
			obj->ctype.laser_info.last_afterburner_time = GameTime64;
		}
	}

#endif
	return result;
}

//--------------------------------------------------------------------
//move all objects for the current frame
static window_event_result object_move_all()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	auto result = window_event_result::ignored;

	if (Highest_object_index > MAX_USED_OBJECTS)
		free_object_slots(MAX_USED_OBJECTS);		//	Free all possible object slots.

	obj_delete_all_that_should_be_dead();

	if (PlayerCfg.AutoLeveling)
		ConsoleObject->mtype.phys_info.flags |= PF_LEVELLING;
	else
		ConsoleObject->mtype.phys_info.flags &= ~PF_LEVELLING;

	// Move all objects
	range_for (const auto &&objp, vmobjptridx)
	{
		if ( (objp->type != OBJ_NONE) && (!(objp->flags&OF_SHOULD_BE_DEAD)) )	{
			result = std::max(object_move_one(objp, Controls), result);
		}
	}

//	check_duplicate_objects();
//	remove_incorrect_objects();

	return result;
}

window_event_result game_move_all_objects()
{
	LevelUniqueObjectState.last_console_player_position = ConsoleObject->pos;
	return object_move_all();
}

window_event_result endlevel_move_all_objects()
{
	return object_move_all();
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
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	auto &vmobjptridx = Objects.vmptridx;
	//last_i = find_last_obj(MAX_OBJECTS);

	//	Note: It's proper to do < (rather than <=) Highest_object_index here because we
	//	are just removing gaps, and the last object can't be a gap.
	for (objnum_t start_i=0;start_i<Highest_object_index;start_i++)
	{
		const auto &&start_objp = vmobjptridx(start_i);
		if (start_objp->type == OBJ_NONE) {
			auto highest = Highest_object_index;
			const auto &&h = vmobjptr(static_cast<objnum_t>(highest));
			auto segnum_copy = h->segnum;

			obj_unlink(Objects.vmptr, Segments.vmptr, h);

			*start_objp = *h;

#if DXX_USE_EDITOR
			if (Cur_object_index == Highest_object_index)
				Cur_object_index = start_i;
			#endif

			h->type = OBJ_NONE;

			obj_link(Objects.vmptr, start_objp, vmsegptridx(segnum_copy));

			while (vmobjptr(static_cast<objnum_t>(--highest))->type == OBJ_NONE)
			{
			}
			Objects.set_count(highest + 1);

			//last_i = find_last_obj(last_i);
			
		}
	}
	reset_objects(LevelUniqueObjectState, LevelUniqueObjectState.num_objects);
}

//called after load.  Takes number of objects,  and objects should be
//compressed.  resets free list, marks unused objects as unused
void reset_objects(d_level_unique_object_state &LevelUniqueObjectState, const unsigned n_objs)
{
	LevelUniqueObjectState.Debris_object_count = 0;
	LevelUniqueObjectState.num_objects = n_objs;
	assert(LevelUniqueObjectState.num_objects > 0);
	auto &Objects = LevelUniqueObjectState.get_objects();
	assert(LevelUniqueObjectState.num_objects < Objects.size());
	Objects.set_count(n_objs);

	for (objnum_t i = n_objs; i < MAX_OBJECTS; ++i)
	{
		LevelUniqueObjectState.free_obj_list[i] = i;
		auto &obj = *Objects.vmptr(i);
		DXX_POISON_VAR(obj, 0xfd);
		obj.type = OBJ_NONE;
		obj.signature = object_signature_t{0};
	}
}

//Tries to find a segment for an object, using find_point_seg()
imsegptridx_t find_object_seg(const d_level_shared_segment_state &LevelSharedSegmentState, d_level_unique_segment_state &LevelUniqueSegmentState, const object_base &obj)
{
	auto &Segments = LevelUniqueSegmentState.get_segments();
	return find_point_seg(LevelSharedSegmentState, LevelUniqueSegmentState, obj.pos, Segments.vmptridx(obj.segnum));
}


//If an object is in a segment, set its segnum field and make sure it's
//properly linked.  If not in any segment, returns 0, else 1.
//callers should generally use find_vector_intersection()
int update_object_seg(fvmobjptr &vmobjptr, const d_level_shared_segment_state &LevelSharedSegmentState, d_level_unique_segment_state &LevelUniqueSegmentState, const vmobjptridx_t obj)
{
	const auto &&newseg = find_object_seg(LevelSharedSegmentState, LevelUniqueSegmentState, obj);
	if (newseg == segment_none)
		return 0;

	if ( newseg != obj->segnum )
		obj_relink(vmobjptr, LevelUniqueSegmentState.get_segments().vmptr, obj, newseg);

	return 1;
}

unsigned laser_parent_is_player(fvcobjptr &vcobjptr, const laser_parent &l, const object_base &o)
{
	/* Player objects are never recycled, so skip the signature check.
	 */
	if (l.parent_type != OBJ_PLAYER)
		return 0;
	/* As a special case, let the player be recognized even if he died
	 * before the weapon hit the target.
	 */
	if (o.type != OBJ_PLAYER && o.type != OBJ_GHOST)
		return 0;
	auto &parent_object = *vcobjptr(l.parent_num);
	return (&parent_object == &o);
}

unsigned laser_parent_is_object(fvcobjptr &vcobjptr, const laser_parent &l, const object_base &o)
{
	auto &parent_object = *vcobjptr(l.parent_num);
	if (&parent_object != &o)
		return 0;
	return laser_parent_is_matching_signature(l, o);
}

unsigned laser_parent_is_object(const laser_parent &l, const vcobjptridx_t o)
{
	if (l.parent_num != o.get_unchecked_index())
		return 0;
	return laser_parent_is_matching_signature(l, *o);
}

unsigned laser_parent_object_exists(fvcobjptr &vcobjptr, const laser_parent &l)
{
	return laser_parent_is_matching_signature(l, *vcobjptr(l.parent_num));
}

void set_powerup_id(const d_powerup_info_array &Powerup_info, const d_vclip_array &Vclip, object_base &o, powerup_type_t id)
{
	o.id = id;
	o.size = Powerup_info[id].size;
	const auto vclip_num = Powerup_info[id].vclip_num;
	o.rtype.vclip_info.vclip_num = vclip_num;
	o.rtype.vclip_info.frametime = Vclip[vclip_num].frame_time;
}

//go through all objects and make sure they have the correct segment numbers
void fix_object_segs()
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vmobjptr = Objects.vmptr;
	auto &vmobjptridx = Objects.vmptridx;
	auto &vcvertptr = Vertices.vcptr;
	range_for (const auto &&o, vmobjptridx)
	{
		if (o->type != OBJ_NONE)
		{
			const auto oldsegnum = o->segnum;
			if (update_object_seg(vmobjptr, LevelSharedSegmentState, LevelUniqueSegmentState, o) == 0)
			{
				const auto pos = o->pos;
				const auto segnum = o->segnum;
				compute_segment_center(vcvertptr, o->pos, vcsegptr(segnum));
				con_printf(CON_URGENT, "Object %hu claims segment %hu, but has position {%i,%i,%i}; moving to %hu:{%i,%i,%i}", o.get_unchecked_index(), oldsegnum, pos.x, pos.y, pos.z, segnum, o->pos.x, o->pos.y, o->pos.z);
			}
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

#if defined(DXX_BUILD_DESCENT_I)
#define object_is_clearable_weapon(W,a,b)	object_is_clearable_weapon(a,b)
#endif
static unsigned object_is_clearable_weapon(const weapon_info_array &Weapon_info, const object_base obj, const unsigned clear_all)
{
	if (!(obj.type == OBJ_WEAPON))
		return 0;
	const auto weapon_id = get_weapon_id(obj);
#if defined(DXX_BUILD_DESCENT_II)
	if (Weapon_info[weapon_id].flags & WIF_PLACABLE)
		return 0;
#endif
	if (clear_all)
		return clear_all;
	return !is_proximity_bomb_or_player_smart_mine(weapon_id);
}

//delete objects, such as weapons & explosions, that shouldn't stay between levels
//	Changed by MK on 10/15/94, don't remove proximity bombs.
//if clear_all is set, clear even proximity bombs
void clear_transient_objects(int clear_all)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	range_for (const auto &&obj, vmobjptridx)
	{
		if (object_is_clearable_weapon(Weapon_info, obj, clear_all) ||
			 obj->type == OBJ_FIREBALL ||
			 obj->type == OBJ_DEBRIS ||
			 (obj->type!=OBJ_NONE && obj->flags & OF_EXPLODING)) {
			obj_delete(LevelUniqueObjectState, Segments, obj);
		}
	}
}

//attaches an object, such as a fireball, to another object, such as a robot
void obj_attach(object_array &Objects, const vmobjptridx_t parent, const vmobjptridx_t sub)
{
	Assert(sub->type == OBJ_FIREBALL);
	assert(sub->control_source == object::control_type::explosion);

	Assert(sub->ctype.expl_info.next_attach==object_none);
	Assert(sub->ctype.expl_info.prev_attach==object_none);

	assert(parent->attached_obj == object_none || Objects.vcptr(parent->attached_obj)->ctype.expl_info.prev_attach == object_none);

	sub->ctype.expl_info.next_attach = parent->attached_obj;

	if (sub->ctype.expl_info.next_attach != object_none)
		Objects.vmptr(sub->ctype.expl_info.next_attach)->ctype.expl_info.prev_attach = sub;

	parent->attached_obj = sub;

	sub->ctype.expl_info.attach_parent = parent;
	sub->flags |= OF_ATTACHED;

	Assert(sub->ctype.expl_info.next_attach != sub);
	Assert(sub->ctype.expl_info.prev_attach != sub);
}

//dettaches one object
void obj_detach_one(object_array &Objects, object &sub)
{
	Assert(sub.flags & OF_ATTACHED);
	Assert(sub.ctype.expl_info.attach_parent != object_none);

	const auto &&parent_objp = Objects.vcptr(sub.ctype.expl_info.attach_parent);
	if (parent_objp->type == OBJ_NONE || parent_objp->attached_obj == object_none)
	{
		sub.flags &= ~OF_ATTACHED;
		return;
	}

	if (sub.ctype.expl_info.next_attach != object_none)
	{
		auto &a = Objects.vmptr(sub.ctype.expl_info.next_attach)->ctype.expl_info.prev_attach;
		assert(Objects.vcptr(a) == &sub);
		a = sub.ctype.expl_info.prev_attach;
	}

	const auto use_prev_attach = (sub.ctype.expl_info.prev_attach != object_none);
	auto &o = *Objects.vmptr(use_prev_attach ? std::exchange(sub.ctype.expl_info.prev_attach, object_none) : sub.ctype.expl_info.attach_parent);
	auto &update_attach = use_prev_attach ? o.ctype.expl_info.next_attach : o.attached_obj;
	assert(Objects.vcptr(update_attach) == &sub);
	update_attach = sub.ctype.expl_info.next_attach;

	sub.ctype.expl_info.next_attach = object_none;
	sub.flags &= ~OF_ATTACHED;

}

//dettaches all objects from this object
static void obj_detach_all(object_array &Objects, object_base &parent)
{
	while (parent.attached_obj != object_none)
		obj_detach_one(Objects, Objects.vmptr(parent.attached_obj));
}

#if defined(DXX_BUILD_DESCENT_II)
//creates a marker object in the world.  returns the object number
imobjptridx_t drop_marker_object(const vms_vector &pos, const vmsegptridx_t segnum, const vms_matrix &orient, const game_marker_index marker_num)
{
	auto &Polygon_models = LevelSharedPolygonModelState.Polygon_models;
	const auto Marker_model_num = LevelSharedPolygonModelState.Marker_model_num;
	if (Marker_model_num >= Polygon_models.size())
	{
		con_printf(CON_URGENT, "%s:%u: failed to drop marker object: invalid model number %u", __FILE__, __LINE__, Marker_model_num);
		return object_none;
	}
	const auto movement_type =
		((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP) && Netgame.Allow_marker_view)
		? object::movement_type::None
		: object::movement_type::spinning;
	const auto &&obj = obj_create(OBJ_MARKER, static_cast<unsigned>(marker_num), segnum, pos, &orient, Polygon_models[Marker_model_num].rad, object::control_type::None, movement_type, RT_POLYOBJ);
	if (obj != object_none) {
		auto &o = *obj;
		o.rtype.pobj_info.model_num = Marker_model_num;

		if (movement_type == object::movement_type::spinning)
		{
		constexpr fix scale = F1_0 / 2;
		const auto oi = obj.get_unchecked_index();
		auto &spin_vec = o.mtype.spin_rate;
		spin_vec = {};
		if (oi & 1)
			vm_vec_scale_add2(spin_vec, o.orient.fvec, (oi & 8) ? scale : -scale);
		if (oi & 2)
			vm_vec_scale_add2(spin_vec, o.orient.uvec, (oi & 16) ? scale : -scale);
		if (oi & 4)
			vm_vec_scale_add2(spin_vec, o.orient.rvec, (oi & 32) ? scale : -scale);
		}

		//	MK, 10/16/95: Using lifeleft to make it flash, thus able to trim lightlevel from all objects.
		o.lifeleft = IMMORTAL_TIME - 1;
	}
	return obj;	
}

//	*viewer is a viewer, probably a missile.
//	wake up all robots that were rendered last frame subject to some constraints.
void wake_up_rendered_objects(const object &viewer, window_rendered_data &window)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	//	Make sure that we are processing current data.
	if (timer_query() != window.time) {
		return;
	}

	Ai_last_missile_camera = &viewer;

	range_for (const auto objnum, window.rendered_robots)
	{
		int	fcval = d_tick_count & 3;
		if ((objnum & 3) == fcval) {
			const auto &&objp = vmobjptr(objnum);
	
			if (objp->type == OBJ_ROBOT) {
				if (vm_vec_dist_quick(viewer.pos, objp->pos) < F1_0*100)
				{
					ai_local		*ailp = &objp->ctype.ai_info.ail;
					if (ailp->player_awareness_type == player_awareness_type_t::PA_NONE) {
						objp->ctype.ai_info.SUB_FLAGS |= SUB_FLAGS_CAMERA_AWAKE;
						ailp->player_awareness_type = player_awareness_type_t::PA_WEAPON_ROBOT_COLLISION;
						ailp->player_awareness_time = F1_0*3;
						ailp->previous_visibility = player_visibility_state::visible_and_in_field_of_view;
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
	
	switch (typename object::movement_type{obj->movement_source})
	{
		case object::movement_type::None:
			obj->mtype = {};
			break;
		case object::movement_type::physics:
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
			
		case object::movement_type::spinning:
			obj->mtype.spin_rate.x = SWAPINT(obj->mtype.spin_rate.x);
			obj->mtype.spin_rate.y = SWAPINT(obj->mtype.spin_rate.y);
			obj->mtype.spin_rate.z = SWAPINT(obj->mtype.spin_rate.z);
			break;
	}
	
	switch (typename object::control_type{obj->control_source})
	{
		case object::control_type::weapon:
			obj->ctype.laser_info.parent_type      = SWAPSHORT(obj->ctype.laser_info.parent_type);
			obj->ctype.laser_info.parent_num       = SWAPSHORT(obj->ctype.laser_info.parent_num);
			obj->ctype.laser_info.parent_signature = SWAPINT(obj->ctype.laser_info.parent_signature);
			obj->ctype.laser_info.creation_time    = SWAPINT(obj->ctype.laser_info.creation_time);
			obj->ctype.laser_info.last_hitobj      = SWAPSHORT(obj->ctype.laser_info.last_hitobj);
			obj->ctype.laser_info.track_goal       = SWAPSHORT(obj->ctype.laser_info.track_goal);
			obj->ctype.laser_info.multiplier       = SWAPINT(obj->ctype.laser_info.multiplier);
			break;
			
		case object::control_type::explosion:
			obj->ctype.expl_info.spawn_time    = SWAPINT(obj->ctype.expl_info.spawn_time);
			obj->ctype.expl_info.delete_time   = SWAPINT(obj->ctype.expl_info.delete_time);
			obj->ctype.expl_info.delete_objnum = SWAPSHORT(obj->ctype.expl_info.delete_objnum);
			obj->ctype.expl_info.attach_parent = SWAPSHORT(obj->ctype.expl_info.attach_parent);
			obj->ctype.expl_info.prev_attach   = SWAPSHORT(obj->ctype.expl_info.prev_attach);
			obj->ctype.expl_info.next_attach   = SWAPSHORT(obj->ctype.expl_info.next_attach);
			break;
			
		case object::control_type::ai:
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
			
		case object::control_type::light:
			obj->ctype.light_info.intensity = SWAPINT(obj->ctype.light_info.intensity);
			break;
			
		case object::control_type::powerup:
			obj->ctype.powerup_info.count         = SWAPINT(obj->ctype.powerup_info.count);
#if defined(DXX_BUILD_DESCENT_II)
			obj->ctype.powerup_info.creation_time = SWAPINT(obj->ctype.powerup_info.creation_time);
			obj->ctype.powerup_info.flags         = SWAPINT(obj->ctype.powerup_info.flags);
#endif
			break;
		case object::control_type::None:
		case object::control_type::flying:
		case object::control_type::slew:
		case object::control_type::flythrough:
		case object::control_type::repaircen:
		case object::control_type::morph:
		case object::control_type::debris:
		case object::control_type::remote:
		default:
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

}

namespace dcx {

void (check_warn_object_type)(const object_base &o, object_type_t t, const char *file, unsigned line)
{
	if (o.type != t)
		con_printf(CON_URGENT, "%s:%u: BUG: object %p has type %u, expected %u", file, line, &o, o.type, t);
}

}
