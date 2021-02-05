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
 * Code for rendering external scenes
 *
 */

//#define _MARK_ON

#include <algorithm>
#include <stdlib.h>


#include <stdio.h>
#include <string.h>
#include <ctype.h> // for isspace

#include "maths.h"
#include "vecmat.h"
#include "gr.h"
#include "3d.h"
#include "dxxerror.h"
#include "palette.h"
#include "iff.h"
#include "console.h"
#include "texmap.h"
#include "fvi.h"
#include "u_mem.h"
#include "sounds.h"
#include "playsave.h"
#include "inferno.h"
#include "endlevel.h"
#include "object.h"
#include "game.h"
#include "gamepal.h"
#include "screens.h"
#include "terrain.h"
#include "robot.h"
#include "player.h"
#include "physfsx.h"
#include "bm.h"
#include "gameseg.h"
#include "gameseq.h"
#include "newdemo.h"
#include "gamepal.h"
#include "net_udp.h"
#include "vclip.h"
#include "fireball.h"
#include "text.h"
#include "digi.h"
#include "songs.h"
#if defined(DXX_BUILD_DESCENT_II)
#include "movie.h"
#endif
#include "render.h"
#include "hudmsg.h"
#if DXX_USE_OGL
#include "ogl_init.h"
#endif

#include "joy.h"

#if DXX_USE_EDITOR
#include "editor/editor.h"
#endif

#include "compiler-range_for.h"
#include "d_enumerate.h"
#include "d_levelstate.h"
#include "d_range.h"
#include <iterator>

using std::min;
using std::max;

#define SHORT_SEQUENCE	1		//if defined, end sequence when panning starts

namespace dcx {

int Endlevel_sequence;
grs_bitmap *terrain_bitmap;	//!!*exit_bitmap,
unsigned exit_modelnum, destroyed_exit_modelnum;
vms_matrix surface_orient;

namespace {

#define FLY_ACCEL i2f(5)
#define MAX_FLY_OBJECTS 2

constexpr vms_vector vmd_zero_vector{};

d_unique_endlevel_state UniqueEndlevelState;
static void generate_starfield(d_unique_endlevel_state::starfield_type &stars);
static void draw_stars(grs_canvas &, const d_unique_endlevel_state::starfield_type &stars);
static int find_exit_side(const d_level_shared_segment_state &LevelSharedSegmentState, const d_level_shared_vertex_state &LevelSharedVertexState, const vms_vector &last_console_player_position, const object_base &obj);

static fix cur_fly_speed, desired_fly_speed;
static grs_bitmap *satellite_bitmap;
vms_vector satellite_pos,satellite_upvec;
static vms_vector station_pos{0xf8c4<<10,0x3c1c<<12,0x372<<10};

static vms_vector mine_exit_point;
static vms_vector mine_ground_exit_point;
static vms_vector mine_side_exit_point;
static vms_matrix mine_exit_orient;
static int outside_mine;

static grs_main_bitmap terrain_bm_instance, satellite_bm_instance;

static int ext_expl_playing,mine_destroyed;
static vms_angvec exit_angles = {-0xa00, 0, 0};
static int endlevel_data_loaded;

static vms_angvec player_angles,player_dest_angles;
#ifndef SHORT_SEQUENCE
static vms_angvec camera_desired_angles,camera_cur_angles;
#endif

static int exit_point_bmx,exit_point_bmy;
static fix satellite_size = i2f(400);

//find delta between two angles
static fixang delta_ang(fixang a,fixang b)
{
	fixang delta0,delta1;
	return (abs(delta0 = a - b) < abs(delta1 = b - a)) ? delta0 : delta1;
}

//return though which side of seg0 is seg1
static size_t matt_find_connect_side(const shared_segment &seg0, const vcsegidx_t seg1)
{
	auto &children = seg0.children;
	return std::distance(children.begin(), std::find(children.begin(), children.end(), seg1));
}

static unsigned get_tunnel_length(fvcsegptridx &vcsegptridx, const vcsegptridx_t console_seg, const unsigned exit_console_side)
{
	auto seg = console_seg;
	auto exit_side = exit_console_side;
	unsigned tunnel_length = 0;
	for (;;)
	{
		const auto child = seg->shared_segment::children[exit_side];
		if (child == segment_none)
			return 0;
		++tunnel_length;
		if (child == segment_exit)
		{
			assert(seg == PlayerUniqueEndlevelState.exit_segnum);
			return tunnel_length;
		}
		const vcsegidx_t old_segidx = seg;
		seg = vcsegptridx(child);
		const auto entry_side = matt_find_connect_side(seg, old_segidx);
		if (entry_side >= Side_opposite.size())
			return 0;
		exit_side = Side_opposite[entry_side];
	}
}

static vcsegidx_t get_tunnel_transition_segment(const unsigned tunnel_length, const vcsegptridx_t console_seg, const unsigned exit_console_side)
{
	auto seg = console_seg;
	auto exit_side = exit_console_side;
	for (auto i = tunnel_length / 3;; --i)
	{
		/*
		 * If the tunnel ended with segment_none, the caller would have
		 * returned immediately after the prior loop.  If the tunnel
		 * ended with segment_exit, then tunnel_length is the count of
		 * segments to reach the exit.  The termination condition on
		 * this loop quits at (tunnel_length / 3), so the loop will quit
		 * before it reaches segment_exit.
		 */
		const auto child = seg->shared_segment::children[exit_side];
		if (!IS_CHILD(child))
			/* This is only a sanity check.  It should never execute
			 * unless there is a bug elsewhere.
			 */
			return seg;
		if (!i)
			return child;
		const vcsegidx_t old_segidx = seg;
		seg = vcsegptridx(child);
		const auto entry_side = matt_find_connect_side(seg, old_segidx);
		exit_side = Side_opposite[entry_side];
	}
}

#define CHASE_TURN_RATE (0x4000/4)		//max turn per second

//returns bitmask of which angles are at dest. bits 0,1,2 = p,b,h
static int chase_angles(vms_angvec *cur_angles,vms_angvec *desired_angles)
{
	vms_angvec delta_angs,alt_angles,alt_delta_angs;
	fix total_delta,alt_total_delta;
	fix frame_turn;
	int mask=0;

	delta_angs.p = desired_angles->p - cur_angles->p;
	delta_angs.h = desired_angles->h - cur_angles->h;
	delta_angs.b = desired_angles->b - cur_angles->b;

	total_delta = abs(delta_angs.p) + abs(delta_angs.b) + abs(delta_angs.h);

	alt_angles.p = f1_0/2 - cur_angles->p;
	alt_angles.b = cur_angles->b + f1_0/2;
	alt_angles.h = cur_angles->h + f1_0/2;

	alt_delta_angs.p = desired_angles->p - alt_angles.p;
	alt_delta_angs.h = desired_angles->h - alt_angles.h;
	alt_delta_angs.b = desired_angles->b - alt_angles.b;

	alt_total_delta = abs(alt_delta_angs.p) + abs(alt_delta_angs.b) + abs(alt_delta_angs.h);

	if (alt_total_delta < total_delta) {
		*cur_angles = alt_angles;
		delta_angs = alt_delta_angs;
	}

	frame_turn = fixmul(FrameTime,CHASE_TURN_RATE);

	if (abs(delta_angs.p) < frame_turn) {
		cur_angles->p = desired_angles->p;
		mask |= 1;
	}
	else
		if (delta_angs.p > 0)
			cur_angles->p += frame_turn;
		else
			cur_angles->p -= frame_turn;

	if (abs(delta_angs.b) < frame_turn) {
		cur_angles->b = desired_angles->b;
		mask |= 2;
	}
	else
		if (delta_angs.b > 0)
			cur_angles->b += frame_turn;
		else
			cur_angles->b -= frame_turn;
//cur_angles->b = 0;

	if (abs(delta_angs.h) < frame_turn) {
		cur_angles->h = desired_angles->h;
		mask |= 4;
	}
	else
		if (delta_angs.h > 0)
			cur_angles->h += frame_turn;
		else
			cur_angles->h -= frame_turn;

	return mask;
}

//find the angle between the player's heading & the station
static void get_angs_to_object(vms_angvec &av,const vms_vector &targ_pos,const vms_vector &cur_pos)
{
	const auto tv = vm_vec_sub(targ_pos,cur_pos);
	vm_extract_angles_vector(av,tv);
}

#define MIN_D 0x100

//find which side to fly out of
static int find_exit_side(const d_level_shared_segment_state &LevelSharedSegmentState, const d_level_shared_vertex_state &LevelSharedVertexState, const vms_vector &last_console_player_position, const object_base &obj)
{
	auto &Vertices = LevelSharedVertexState.get_vertices();
	vms_vector prefvec;
	fix best_val=-f2_0;
	int best_side;

	//find exit side

	vm_vec_normalized_dir_quick(prefvec, obj.pos, last_console_player_position);

	const shared_segment &pseg = LevelSharedSegmentState.get_segments().vcptr(obj.segnum);
	auto &vcvertptr = Vertices.vcptr;
	const auto segcenter = compute_segment_center(vcvertptr, pseg);

	best_side=-1;
	for (int i=MAX_SIDES_PER_SEGMENT;--i >= 0;) {
		fix d;

		if (pseg.children[i] != segment_none)
		{
			auto sidevec = compute_center_point_on_side(vcvertptr, pseg, i);
			vm_vec_normalized_dir_quick(sidevec,sidevec,segcenter);
			d = vm_vec_dot(sidevec,prefvec);

			if (labs(d) < MIN_D) d=0;

			if (d > best_val) {best_val=d; best_side=i;}
		}
	}

	Assert(best_side!=-1);
	return best_side;
}

static void draw_mine_exit_cover(grs_canvas &canvas)
{
	const int of = 10;
	const fix u = i2f(6), d = i2f(9), ur = i2f(14), dr = i2f(17);
	const uint8_t color = BM_XRGB(0, 0, 0);
	vms_vector v;
	g3s_point p0, p1, p2, p3;

	vm_vec_scale_add(v,mine_exit_point,mine_exit_orient.fvec,i2f(of));

	auto mrd = mine_exit_orient.rvec;
	{
		vms_vector vu;
		vm_vec_scale_add(vu, v, mine_exit_orient.uvec, u);
		auto mru = mrd;
		vm_vec_scale(mru, ur);
		vms_vector p;
		g3_rotate_point(p0, (vm_vec_add(p, vu, mru), p));
		g3_rotate_point(p1, (vm_vec_sub(p, vu, mru), p));
	}
	{
		vms_vector vd;
		vm_vec_scale_add(vd, v, mine_exit_orient.uvec, -d);
		vm_vec_scale(mrd, dr);
		vms_vector p;
		g3_rotate_point(p2, (vm_vec_sub(p, vd, mrd), p));
		g3_rotate_point(p3, (vm_vec_add(p, vd, mrd), p));
	}
	const std::array<cg3s_point *, 4> pointlist{{
		&p0,
		&p1,
		&p2,
		&p3,
	}};

	g3_draw_poly(canvas, pointlist.size(), pointlist, color);
}

static void generate_starfield(d_unique_endlevel_state::starfield_type &stars)
{
	range_for (auto &i, stars)
	{
		i.x = (d_rand() - D_RAND_MAX / 2) << 14;
		i.z = (d_rand() - D_RAND_MAX / 2) << 14;
		i.y = (d_rand() / 2) << 14;
	}
}

void draw_stars(grs_canvas &canvas, const d_unique_endlevel_state::starfield_type &stars)
{
	int intensity=31;
	g3s_point p;

	uint8_t color = 0;
	range_for (auto &&e, enumerate(stars))
	{
		const auto i = e.idx;
		auto &si = e.value;

		if ((i&63) == 0) {
			color = BM_XRGB(intensity,intensity,intensity);
			intensity-=3;
		}

		g3_rotate_delta_vec(p.p3_vec, si);
		g3_code_point(p);

		if (p.p3_codes == 0) {

			p.p3_flags &= ~PF_PROJECTED;

			g3_project_point(p);
#if !DXX_USE_OGL
			gr_pixel(canvas.cv_bitmap, f2i(p.p3_sx), f2i(p.p3_sy), color);
#else
			g3_draw_sphere(canvas, p, F1_0 * 3, color);
#endif
		}
	}

//@@	{
//@@		vms_vector delta;
//@@		g3s_point top_pnt;
//@@
//@@		g3_rotate_point(&p,&satellite_pos);
//@@		g3_rotate_delta_vec(&delta,&satellite_upvec);
//@@
//@@		g3_add_delta_vec(&top_pnt,&p,&delta);
//@@
//@@		if (! (p.p3_codes & CC_BEHIND)) {
//@@			int save_im = Interpolation_method;
//@@			Interpolation_method = 0;
//@@			//p.p3_flags &= ~PF_PROJECTED;
//@@			g3_project_point(&p);
//@@			if (! (p.p3_flags & PF_OVERFLOW))
//@@				//gr_bitmapm(f2i(p.p3_sx)-32,f2i(p.p3_sy)-32,satellite_bitmap);
//@@				g3_draw_rod_tmap(satellite_bitmap,&p,SATELLITE_WIDTH,&top_pnt,SATELLITE_WIDTH,f1_0);
//@@			Interpolation_method = save_im;
//@@		}
//@@	}
}

static void angvec_add2_scale(vms_angvec &dest,const vms_vector &src,fix s)
{
	dest.p += fixmul(src.x,s);
	dest.b += fixmul(src.z,s);
	dest.h += fixmul(src.y,s);
}

static int convert_ext(d_fname &dest, const char (&ext)[4])
{
	auto b = std::begin(dest);
	auto e = std::end(dest);
	auto t = std::find(b, e, '.');
	if (t != e && std::distance(b, t) <= 8)
	{
		std::copy(std::begin(ext), std::end(ext), std::next(t));
		return 1;
	}
	else
		return 0;
}

static std::pair<icsegidx_t, sidenum_fast_t> find_exit_segment_side(fvcsegptridx &vcsegptridx)
{
	range_for (const auto &&segp, vcsegptridx)
	{
		for (const auto &&[child_segnum, sidenum] : enumerate(segp->shared_segment::children))
		{
			if (child_segnum == segment_exit)
			{
				return {segp, sidenum};
			}
		}
	}
	return {segment_none, side_none};
}

}

void free_endlevel_data()
{
	terrain_bm_instance.reset();
	satellite_bm_instance.reset();
	free_light_table();
	free_height_array();
}

}

namespace dsx {

namespace {

struct flythrough_data
{
	object		*obj;
	vms_angvec	angles;			//orientation in angles
	vms_vector	step;				//how far in a second
	vms_vector	angstep;			//rotation per second
	fix			speed;			//how fast object is moving
	vms_vector 	headvec;			//where we want to be pointing
	int			first_time;		//flag for if first time through
	fix			offset_frac;	//how far off-center as portion of way
	fix			offset_dist;	//how far currently off-center
};

static std::array<flythrough_data, MAX_FLY_OBJECTS> fly_objects;

//endlevel sequence states

#define EL_OFF				0		//not in endlevel
#define EL_FLYTHROUGH	1		//auto-flythrough in tunnel
#define EL_LOOKBACK		2		//looking back at player
#define EL_OUTSIDE		3		//flying outside for a while
#define EL_STOPPED		4		//stopped, watching explosion
#define EL_PANNING		5		//panning around, watching player
#define EL_CHASING		6		//chasing player to station

static object *endlevel_camera;
static object *external_explosion;

#define FLY_SPEED i2f(50)

static void do_endlevel_flythrough(flythrough_data *flydata);

#define DEFAULT_SPEED i2f(16)

#if defined(DXX_BUILD_DESCENT_II)
constexpr std::array<const char, 24> movie_table{{
	'A','B','C','A',
	'D','F','D','F',
	'G','H','I','G',
	'J','K','L','J',
	'M','O','M','O',
	'P','Q','P','Q'
}};
static auto endlevel_movie_played = movie_play_status::skipped;

#define MOVIE_REQUIRED 1

//returns movie played status.  see movie.h
static movie_play_status start_endlevel_movie()
{
	palette_array_t save_pal;

	//Assert(PLAYING_BUILTIN_MISSION); //only play movie for built-in mission

	//Assert(N_MOVIES >= Last_level);
	//Assert(N_MOVIES_SECRET >= -Last_secret_level);

	const auto current_level_num = Current_level_num;
	if (is_SHAREWARE)
		return movie_play_status::skipped;
	if (!is_D2_OEM)
		if (current_level_num == Last_level)
			return movie_play_status::started;   //don't play movie

	if (!(current_level_num > 0))
		return movie_play_status::skipped;       //no escapes for secret level
	char movie_name[] = "ESA.MVE";
	movie_name[2] = movie_table[Current_level_num-1];

	save_pal = gr_palette;

	const auto r = PlayMovie(NULL, movie_name,(Game_mode & GM_MULTI)?0:MOVIE_REQUIRED);
	if (Newdemo_state == ND_STATE_PLAYBACK) {
		set_screen_mode(SCREEN_GAME);
		gr_palette = save_pal;
	}
	return (r);
}
#endif

//if speed is zero, use default speed
void start_endlevel_flythrough(flythrough_data &flydata, object &obj, const fix speed)
{
	flydata.obj = &obj;
	flydata.first_time = 1;
	flydata.speed = speed?speed:DEFAULT_SPEED;
	flydata.offset_frac = 0;
}

void draw_exit_model(grs_canvas &canvas)
{
	int f=15,u=0;	//21;
	g3s_lrgb lrgb = { f1_0, f1_0, f1_0 };

	if (mine_destroyed)
	{
		// draw black shape to mask out terrain in exit hatch
		draw_mine_exit_cover(canvas);
	}

	auto model_pos = vm_vec_scale_add(mine_exit_point,mine_exit_orient.fvec,i2f(f));
	vm_vec_scale_add2(model_pos,mine_exit_orient.uvec,i2f(u));
	draw_polygon_model(canvas, model_pos, mine_exit_orient, nullptr, mine_destroyed ? destroyed_exit_modelnum : exit_modelnum, 0, lrgb, nullptr, nullptr);
}

#define SATELLITE_DIST		i2f(1024)
#define SATELLITE_WIDTH		satellite_size
#define SATELLITE_HEIGHT	((satellite_size*9)/4)		//((satellite_size*5)/2)

static void render_external_scene(fvcobjptridx &vcobjptridx, grs_canvas &canvas, const d_level_unique_light_state &LevelUniqueLightState, const fix eye_offset)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
#if DXX_USE_OGL
	int orig_Render_depth = Render_depth;
#endif
	g3s_lrgb lrgb = { f1_0, f1_0, f1_0 };

	auto Viewer_eye = Viewer->pos;

	if (eye_offset)
		vm_vec_scale_add2(Viewer_eye,Viewer->orient.rvec,eye_offset);

	g3_set_view_matrix(Viewer->pos,Viewer->orient,Render_zoom);

	//g3_draw_horizon(BM_XRGB(0,0,0),BM_XRGB(16,16,16));		//,-1);
	gr_clear_canvas(canvas, BM_XRGB(0,0,0));

	g3_start_instance_matrix(vmd_zero_vector, surface_orient);
	draw_stars(canvas, UniqueEndlevelState.stars);
	g3_done_instance();

	{	//draw satellite

		vms_vector delta;
		g3s_point top_pnt;

		const auto p = g3_rotate_point(satellite_pos);
		g3_rotate_delta_vec(delta,satellite_upvec);

		g3_add_delta_vec(top_pnt,p,delta);

		if (! (p.p3_codes & CC_BEHIND)) {
			//p.p3_flags &= ~PF_PROJECTED;
			//g3_project_point(&p);
			if (! (p.p3_flags & PF_OVERFLOW)) {
				push_interpolation_method save_im(0);
				//gr_bitmapm(f2i(p.p3_sx)-32,f2i(p.p3_sy)-32,satellite_bitmap);
				g3_draw_rod_tmap(canvas, *satellite_bitmap, p, SATELLITE_WIDTH, top_pnt, SATELLITE_WIDTH, lrgb);
			}
		}
	}

#if DXX_USE_OGL
	ogl_toggle_depth_test(0);
	Render_depth = (200-(vm_vec_dist_quick(mine_ground_exit_point, Viewer_eye)/F1_0))/36;
#endif
	render_terrain(canvas, Viewer_eye, mine_ground_exit_point, exit_point_bmx, exit_point_bmy);
#if DXX_USE_OGL
	Render_depth = orig_Render_depth;
	ogl_toggle_depth_test(1);
#endif

	draw_exit_model(canvas);
	if (ext_expl_playing)
	{
		const auto alpha = PlayerCfg.AlphaBlendMineExplosion;
		if (alpha) // set nice transparency/blending for the big explosion
			gr_settransblend(canvas, GR_FADE_OFF, gr_blend::additive_c);
		draw_fireball(Vclip, canvas, vcobjptridx(external_explosion));
#if DXX_USE_OGL
		/* If !OGL, the third argument is discarded, so this call
		 * becomes the same as the one above.
		 */
		if (alpha)
			gr_settransblend(canvas, GR_FADE_OFF, gr_blend::normal); // revert any transparency/blending setting back to normal
#endif
	}

#if !DXX_USE_OGL
	Lighting_on=0;
#endif
	render_object(canvas, LevelUniqueLightState, vmobjptridx(ConsoleObject));
#if !DXX_USE_OGL
	Lighting_on=1;
#endif
}

}

window_event_result start_endlevel_sequence()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &vmobjptr = Objects.vmptr;
	reset_rear_view(); //turn off rear view if set - NOTE: make sure this happens before we pause demo recording!!

	if (Newdemo_state == ND_STATE_RECORDING)		// stop demo recording
		Newdemo_state = ND_STATE_PAUSED;

	if (Newdemo_state == ND_STATE_PLAYBACK) {		// don't do this if in playback mode
#if defined(DXX_BUILD_DESCENT_II)
		if (PLAYING_BUILTIN_MISSION) // only play movie for built-in mission
		{
			window_set_visible(*Game_wind, 0);	// suspend the game, including drawing
			start_endlevel_movie();
			window_set_visible(*Game_wind, 1);
		}
		strcpy(last_palette_loaded,"");		//force palette load next time
#endif
		return window_event_result::ignored;
	}

	if (Player_dead_state != player_dead_state::no ||
		(ConsoleObject->flags & OF_SHOULD_BE_DEAD))
		return window_event_result::ignored;				//don't start if dead!
	con_puts(CON_NORMAL, "You have escaped the mine!");

#if defined(DXX_BUILD_DESCENT_II)
	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	//	Dematerialize Buddy!
	range_for (const auto &&objp, vmobjptr)
	{
		if (objp->type == OBJ_ROBOT)
			if (Robot_info[get_robot_id(objp)].companion) {
				object_create_explosion(vmsegptridx(objp->segnum), objp->pos, F1_0*7/2, VCLIP_POWERUP_DISAPPEARANCE );
				objp->flags |= OF_SHOULD_BE_DEAD;
			}
	}
#endif

	get_local_plrobj().ctype.player_info.homing_object_dist = -F1_0; // Turn off homing sound.

	if (Game_mode & GM_MULTI) {
		multi_send_endlevel_start(multi_endlevel_type::normal);
		multi::dispatch->do_protocol_frame(1, 1);
	}

#if defined(DXX_BUILD_DESCENT_I)
	if (!endlevel_data_loaded)
#elif defined(DXX_BUILD_DESCENT_II)

	if (PLAYING_BUILTIN_MISSION) // only play movie for built-in mission
		if (!(Game_mode & GM_MULTI))
		{
			window_set_visible(*Game_wind, 0);	// suspend the game, including drawing
			endlevel_movie_played = start_endlevel_movie();
			window_set_visible(*Game_wind, 1);
		}

	if (!(!(Game_mode & GM_MULTI) && (endlevel_movie_played == movie_play_status::skipped) && endlevel_data_loaded))
#endif
	{

		return PlayerFinishedLevel(0);		//done with level
	}
#if defined(DXX_BUILD_DESCENT_II)
	int exit_models_loaded = 0;

	if (Piggy_hamfile_version < 3)
		exit_models_loaded = 1; // built-in for PC shareware

	else
		exit_models_loaded = load_exit_models();

	if (!exit_models_loaded)
		return window_event_result::ignored;
#endif
	{
		//count segments in exit tunnel

		const object_base &console = vmobjptr(ConsoleObject);
		const auto exit_console_side = find_exit_side(LevelSharedSegmentState, LevelSharedVertexState, LevelUniqueObjectState.last_console_player_position, console);
		const auto &&console_seg = vcsegptridx(console.segnum);
		const auto tunnel_length = get_tunnel_length(vcsegptridx, console_seg, exit_console_side);
		if (!tunnel_length)
		{
				return PlayerFinishedLevel(0);		//don't do special sequence
		}
		//now pick transition segnum 1/3 of the way in

		PlayerUniqueEndlevelState.transition_segnum = get_tunnel_transition_segment(tunnel_length, console_seg, exit_console_side);
	}

	if (Game_mode & GM_MULTI) {
		multi_send_endlevel_start(multi_endlevel_type::normal);
		multi::dispatch->do_protocol_frame(1, 1);
	}
	songs_play_song( SONG_ENDLEVEL, 0 );

	Endlevel_sequence = EL_FLYTHROUGH;

	ConsoleObject->movement_source = object::movement_type::None;			//movement handled by flythrough
	ConsoleObject->control_source = object::control_type::None;

	Game_suspended |= SUSP_ROBOTS;          //robots don't move

	cur_fly_speed = desired_fly_speed = FLY_SPEED;

	start_endlevel_flythrough(fly_objects[0], vmobjptr(ConsoleObject), cur_fly_speed);		//initialize

	HUD_init_message_literal(HM_DEFAULT, TXT_EXIT_SEQUENCE );

	outside_mine = ext_expl_playing = 0;

	flash_scale = f1_0;

	generate_starfield(UniqueEndlevelState.stars);

	mine_destroyed=0;

	return window_event_result::handled;
}

window_event_result stop_endlevel_sequence()
{
#if !DXX_USE_OGL
	Interpolation_method = 0;
#endif

	select_cockpit(PlayerCfg.CockpitMode[0]);

	Endlevel_sequence = EL_OFF;
	return PlayerFinishedLevel(0);
}

#define VCLIP_BIG_PLAYER_EXPLOSION	58

//--unused-- vms_vector upvec = {0,f1_0,0};

window_event_result do_endlevel_frame()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	static fix timer;
	static fix bank_rate;
	static fix explosion_wait1=0;
	static fix explosion_wait2=0;
	static fix ext_expl_halflife;

	auto result = endlevel_move_all_objects();

	if (ext_expl_playing) {

		do_explosion_sequence(vmobjptr(external_explosion));

		if (external_explosion->lifeleft < ext_expl_halflife)
			mine_destroyed = 1;

		if (external_explosion->flags & OF_SHOULD_BE_DEAD)
			ext_expl_playing = 0;
	}

	if (cur_fly_speed != desired_fly_speed) {
		fix delta = desired_fly_speed - cur_fly_speed;
		fix frame_accel = fixmul(FrameTime,FLY_ACCEL);

		if (abs(delta) < frame_accel)
			cur_fly_speed = desired_fly_speed;
		else
			if (delta > 0)
				cur_fly_speed += frame_accel;
			else
				cur_fly_speed -= frame_accel;
	}

	//do big explosions
	if (!outside_mine) {

		if (Endlevel_sequence==EL_OUTSIDE) {
			const auto tvec = vm_vec_sub(ConsoleObject->pos,mine_side_exit_point);
			if (vm_vec_dot(tvec,mine_exit_orient.fvec) > 0) {
				vms_vector mov_vec;

				outside_mine = 1;

				const auto &&exit_segp = vmsegptridx(PlayerUniqueEndlevelState.exit_segnum);
				const auto &&tobj = object_create_explosion(exit_segp, mine_side_exit_point, i2f(50), VCLIP_BIG_PLAYER_EXPLOSION);

				if (tobj) {
				// Move explosion to Viewer to draw it in front of mine exit model
				vm_vec_normalized_dir_quick(mov_vec,Viewer->pos,tobj->pos);
				vm_vec_scale_add2(tobj->pos,mov_vec,i2f(30));
					external_explosion = tobj;

					flash_scale = 0;	//kill lights in mine

					ext_expl_halflife = tobj->lifeleft;

					ext_expl_playing = 1;
				}

				digi_link_sound_to_pos(SOUND_BIG_ENDLEVEL_EXPLOSION, exit_segp, 0, mine_side_exit_point, 0, i2f(3)/4);
			}
		}

		//do explosions chasing player
		if ((explosion_wait1-=FrameTime) < 0) {
			static int sound_count;

			auto tpnt = vm_vec_scale_add(ConsoleObject->pos,ConsoleObject->orient.fvec,-ConsoleObject->size*5);
			vm_vec_scale_add2(tpnt,ConsoleObject->orient.rvec,(d_rand()-D_RAND_MAX/2)*15);
			vm_vec_scale_add2(tpnt,ConsoleObject->orient.uvec,(d_rand()-D_RAND_MAX/2)*15);

			const auto &&segnum = find_point_seg(LevelSharedSegmentState, LevelUniqueSegmentState, tpnt, Segments.vmptridx(ConsoleObject->segnum));

			if (segnum != segment_none) {
				object_create_explosion(segnum,tpnt,i2f(20),VCLIP_BIG_PLAYER_EXPLOSION);
			       	if (d_rand()<10000 || ++sound_count==7) {		//pseudo-random
					digi_link_sound_to_pos( SOUND_TUNNEL_EXPLOSION, segnum, 0, tpnt, 0, F1_0 );
					sound_count=0;
				}
			}

			explosion_wait1 = 0x2000 + d_rand()/4;

		}
	}

	//do little explosions on walls
	if (Endlevel_sequence >= EL_FLYTHROUGH && Endlevel_sequence < EL_OUTSIDE)
		if ((explosion_wait2-=FrameTime) < 0) {
			fvi_query fq;
			fvi_info hit_data;

			//create little explosion on wall

			auto tpnt = vm_vec_copy_scale(ConsoleObject->orient.rvec,(d_rand()-D_RAND_MAX/2)*100);
			vm_vec_scale_add2(tpnt,ConsoleObject->orient.uvec,(d_rand()-D_RAND_MAX/2)*100);
			vm_vec_add2(tpnt,ConsoleObject->pos);

			if (Endlevel_sequence == EL_FLYTHROUGH)
				vm_vec_scale_add2(tpnt,ConsoleObject->orient.fvec,d_rand()*200);
			else
				vm_vec_scale_add2(tpnt,ConsoleObject->orient.fvec,d_rand()*60);

			//find hit point on wall

			fq.p0 = &ConsoleObject->pos;
			fq.p1 = &tpnt;
			fq.startseg = ConsoleObject->segnum;
			fq.rad = 0;
			fq.thisobjnum = object_first;
			fq.ignore_obj_list.first = nullptr;
			fq.flags = 0;

			find_vector_intersection(fq, hit_data);

			if (hit_data.hit_type==HIT_WALL && hit_data.hit_seg!=segment_none)
				object_create_explosion(vmsegptridx(hit_data.hit_seg), hit_data.hit_pnt, i2f(3) + d_rand() * 6, VCLIP_SMALL_EXPLOSION);

			explosion_wait2 = (0xa00 + d_rand()/8)/2;
		}

	switch (Endlevel_sequence) {

		case EL_OFF: return result;

		case EL_FLYTHROUGH: {

			do_endlevel_flythrough(&fly_objects[0]);

			if (ConsoleObject->segnum == PlayerUniqueEndlevelState.transition_segnum)
			{
#if defined(DXX_BUILD_DESCENT_II)
				if (PLAYING_BUILTIN_MISSION && endlevel_movie_played != movie_play_status::skipped)
					result = std::max(stop_endlevel_sequence(), result);
				else
#endif
				{

					//songs_play_song( SONG_ENDLEVEL, 0 );

					Endlevel_sequence = EL_LOOKBACK;

					auto objnum = obj_create(OBJ_CAMERA, 0,
					                    vmsegptridx(ConsoleObject->segnum), ConsoleObject->pos, &ConsoleObject->orient, 0, 
					                    object::control_type::None, object::movement_type::None, RT_NONE);

					if (objnum == object_none) { //can't get object, so abort
						return std::max(stop_endlevel_sequence(), result);
					}

					Viewer = endlevel_camera = objnum;

					select_cockpit(CM_LETTERBOX);

					fly_objects[1] = fly_objects[0];
					fly_objects[1].obj = endlevel_camera;
					fly_objects[1].speed = (5*cur_fly_speed)/4;
					fly_objects[1].offset_frac = 0x4000;

					vm_vec_scale_add2(endlevel_camera->pos,endlevel_camera->orient.fvec,i2f(7));

					timer=0x20000;

				}
			}

			break;
		}


		case EL_LOOKBACK: {

			do_endlevel_flythrough(&fly_objects[0]);
			do_endlevel_flythrough(&fly_objects[1]);

			if (timer>0) {

				timer -= FrameTime;

				if (timer < 0)		//reduce speed
					fly_objects[1].speed = fly_objects[0].speed;
			}

			if (endlevel_camera->segnum == PlayerUniqueEndlevelState.exit_segnum)
			{
				Endlevel_sequence = EL_OUTSIDE;

				timer = i2f(2);

				vm_vec_negate(endlevel_camera->orient.fvec);
				vm_vec_negate(endlevel_camera->orient.rvec);

				const auto cam_angles = vm_extract_angles_matrix(endlevel_camera->orient);
				const auto exit_seg_angles = vm_extract_angles_matrix(mine_exit_orient);
				bank_rate = (-exit_seg_angles.b - cam_angles.b)/2;

				ConsoleObject->control_source = endlevel_camera->control_source = object::control_type::None;

			}

			break;
		}

		case EL_OUTSIDE: {
			vm_vec_scale_add2(ConsoleObject->pos,ConsoleObject->orient.fvec,fixmul(FrameTime,cur_fly_speed));
			vm_vec_scale_add2(endlevel_camera->pos,endlevel_camera->orient.fvec,fixmul(FrameTime,-2*cur_fly_speed));
			vm_vec_scale_add2(endlevel_camera->pos,endlevel_camera->orient.uvec,fixmul(FrameTime,-cur_fly_speed/10));

			auto cam_angles = vm_extract_angles_matrix(endlevel_camera->orient);
			cam_angles.b += fixmul(bank_rate,FrameTime);
			vm_angles_2_matrix(endlevel_camera->orient,cam_angles);

			timer -= FrameTime;

			if (timer < 0) {

				Endlevel_sequence = EL_STOPPED;

				vm_extract_angles_matrix(player_angles,ConsoleObject->orient);

				timer = i2f(3);

			}

			break;
		}

		case EL_STOPPED: {

			get_angs_to_object(player_dest_angles,station_pos,ConsoleObject->pos);
			chase_angles(&player_angles,&player_dest_angles);
			vm_angles_2_matrix(ConsoleObject->orient,player_angles);

			vm_vec_scale_add2(ConsoleObject->pos,ConsoleObject->orient.fvec,fixmul(FrameTime,cur_fly_speed));

			timer -= FrameTime;

			if (timer < 0) {


				#ifdef SHORT_SEQUENCE

				result = std::max(stop_endlevel_sequence(), result);

				#else
				Endlevel_sequence = EL_PANNING;

				vm_extract_angles_matrix(camera_cur_angles,endlevel_camera->orient);


				timer = i2f(3);

				if (Game_mode & GM_MULTI) { // try to skip part of the seq if multiplayer
					result = std::max(stop_endlevel_sequence(), result);
					return result;
				}

				#endif		//SHORT_SEQUENCE

			}
			break;
		}

		#ifndef SHORT_SEQUENCE
		case EL_PANNING: {
			int mask;

			get_angs_to_object(player_dest_angles,station_pos,ConsoleObject->pos);
			chase_angles(&player_angles,&player_dest_angles);
			vm_angles_2_matrix(ConsoleObject->orient,player_angles);
			vm_vec_scale_add2(ConsoleObject->pos,ConsoleObject->orient.fvec,fixmul(FrameTime,cur_fly_speed));


			get_angs_to_object(camera_desired_angles,ConsoleObject->pos,endlevel_camera->pos);
			mask = chase_angles(&camera_cur_angles,&camera_desired_angles);
			vm_angles_2_matrix(endlevel_camera->orient,camera_cur_angles);

			if ((mask&5) == 5) {

				vms_vector tvec;

				Endlevel_sequence = EL_CHASING;

				vm_vec_normalized_dir_quick(tvec,station_pos,ConsoleObject->pos);
				vm_vector_2_matrix(ConsoleObject->orient,tvec,&surface_orient.uvec,nullptr);

				desired_fly_speed *= 2;
			}

			break;
		}

		case EL_CHASING: {
			fix d,speed_scale;


			get_angs_to_object(camera_desired_angles,ConsoleObject->pos,endlevel_camera->pos);
			chase_angles(&camera_cur_angles,&camera_desired_angles);

			vm_angles_2_matrix(endlevel_camera->orient,camera_cur_angles);

			d = vm_vec_dist_quick(ConsoleObject->pos,endlevel_camera->pos);

			speed_scale = fixdiv(d,i2f(0x20));
			if (d<f1_0) d=f1_0;

			get_angs_to_object(player_dest_angles,station_pos,ConsoleObject->pos);
			chase_angles(&player_angles,&player_dest_angles);
			vm_angles_2_matrix(ConsoleObject->orient,player_angles);

			vm_vec_scale_add2(ConsoleObject->pos,ConsoleObject->orient.fvec,fixmul(FrameTime,cur_fly_speed));
			vm_vec_scale_add2(endlevel_camera->pos,endlevel_camera->orient.fvec,fixmul(FrameTime,fixmul(speed_scale,cur_fly_speed)));

			if (vm_vec_dist(ConsoleObject->pos,station_pos) < i2f(10))
				result = std::max(stop_endlevel_sequence(), result);

			break;

		}
		#endif		//ifdef SHORT_SEQUENCE

	}

	return result;
}

namespace {

static void endlevel_render_mine(const d_level_shared_segment_state &LevelSharedSegmentState, grs_canvas &canvas, fix eye_offset)
{
	auto Viewer_eye = Viewer->pos;

	if (Viewer->type == OBJ_PLAYER )
		vm_vec_scale_add2(Viewer_eye,Viewer->orient.fvec,(Viewer->size*3)/4);

	if (eye_offset)
		vm_vec_scale_add2(Viewer_eye,Viewer->orient.rvec,eye_offset);

#if DXX_USE_EDITOR
	if (EditorWindow)
		Viewer_eye = Viewer->pos;
	#endif

	segnum_t start_seg_num;
	if (Endlevel_sequence >= EL_OUTSIDE) {
		start_seg_num = PlayerUniqueEndlevelState.exit_segnum;
	}
	else {
		start_seg_num = find_point_seg(LevelSharedSegmentState, Viewer_eye, Segments.vcptridx(Viewer->segnum));

		if (start_seg_num==segment_none)
			start_seg_num = Viewer->segnum;
	}

	g3_set_view_matrix(Viewer_eye, Endlevel_sequence == EL_LOOKBACK
		? vm_matrix_x_matrix(Viewer->orient, vm_angles_2_matrix(vms_angvec{0, 0, INT16_MAX}))
		: Viewer->orient, Render_zoom);

	window_rendered_data window;
	render_mine(canvas, Viewer_eye, start_seg_num, eye_offset, window);
}

}

void render_endlevel_frame(grs_canvas &canvas, fix eye_offset)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptridx = Objects.vcptridx;
	g3_start_frame(canvas);

	if (Endlevel_sequence < EL_OUTSIDE)
		endlevel_render_mine(LevelSharedSegmentState, canvas, eye_offset);
	else
		render_external_scene(vcobjptridx, canvas, LevelUniqueLightState, eye_offset);

	g3_end_frame();
}

///////////////////////// copy of flythrough code for endlevel

#define MAX_ANGSTEP	0x4000		//max turn per second

#define MAX_SLIDE_PER_SEGMENT 0x10000

namespace {

void do_endlevel_flythrough(flythrough_data *flydata)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vmobjptr = Objects.vmptr;
	auto &vmobjptridx = Objects.vmptridx;
	const auto &&obj = vmobjptridx(flydata->obj);
	
	vcsegidx_t old_player_seg = obj->segnum;

	//move the player for this frame

	if (!flydata->first_time) {

		vm_vec_scale_add2(obj->pos,flydata->step,FrameTime);
		angvec_add2_scale(flydata->angles,flydata->angstep,FrameTime);

		vm_angles_2_matrix(obj->orient,flydata->angles);
	}

	//check new player seg

	update_object_seg(vmobjptr, LevelSharedSegmentState, LevelUniqueSegmentState, obj);
	const shared_segment &pseg = *vcsegptr(obj->segnum);

	if (flydata->first_time || obj->segnum != old_player_seg) {		//moved into new seg
		fix seg_time;
		short entry_side,exit_side = -1;//what sides we entry and leave through
		int up_side=0;

		entry_side=0;

		//find new exit side

		if (!flydata->first_time) {

			entry_side = matt_find_connect_side(vcsegptr(obj->segnum), old_player_seg);
			exit_side = Side_opposite[entry_side];
		}

		if (flydata->first_time || entry_side == side_none || pseg.children[exit_side] == segment_none)
			exit_side = find_exit_side(LevelSharedSegmentState, LevelSharedVertexState, LevelUniqueObjectState.last_console_player_position, obj);

		{										//find closest side to align to
			fix d,largest_d=-f1_0;
			range_for (const int i, xrange(6u)) {
				d = vm_vec_dot(pseg.sides[i].normals[0], flydata->obj->orient.uvec);
				if (d > largest_d) {largest_d = d; up_side=i;}
			}

		}

		//update target point & angles

		//where we are heading (center of exit_side)
		auto &vcvertptr = Vertices.vcptr;
		auto dest_point = compute_center_point_on_side(vcvertptr, pseg, exit_side);
		const vms_vector nextcenter = (pseg.children[exit_side] == segment_exit)
			? dest_point
			: compute_segment_center(vcvertptr, vcsegptr(pseg.children[exit_side]));

		//update target point and movement points

		//offset object sideways
		if (flydata->offset_frac) {
			int s0=-1,s1=0;
			fix dist;

			range_for (const int i, xrange(6u))
				if (i!=entry_side && i!=exit_side && i!=up_side && i!=Side_opposite[up_side])
				 {
					if (s0==-1)
						s0 = i;
					else
						s1 = i;
				 }

			const auto &&s0p = compute_center_point_on_side(vcvertptr, pseg, s0);
			const auto &&s1p = compute_center_point_on_side(vcvertptr, pseg, s1);
			dist = fixmul(vm_vec_dist(s0p,s1p),flydata->offset_frac);

			if (dist-flydata->offset_dist > MAX_SLIDE_PER_SEGMENT)
				dist = flydata->offset_dist + MAX_SLIDE_PER_SEGMENT;

			flydata->offset_dist = dist;

			vm_vec_scale_add2(dest_point,obj->orient.rvec,dist);

		}

		vm_vec_sub(flydata->step,dest_point,obj->pos);
		auto step_size = vm_vec_normalize_quick(flydata->step);
		vm_vec_scale(flydata->step,flydata->speed);

		const auto curcenter = compute_segment_center(vcvertptr, pseg);
		vm_vec_sub(flydata->headvec,nextcenter,curcenter);

		const auto dest_orient = vm_vector_2_matrix(flydata->headvec,&pseg.sides[up_side].normals[0],nullptr);
		//where we want to be pointing
		const auto dest_angles = vm_extract_angles_matrix(dest_orient);

		if (flydata->first_time)
			vm_extract_angles_matrix(flydata->angles,obj->orient);

		seg_time = fixdiv(step_size,flydata->speed);	//how long through seg

		if (seg_time) {
			flydata->angstep.x = max(-MAX_ANGSTEP,min(MAX_ANGSTEP,fixdiv(delta_ang(flydata->angles.p,dest_angles.p),seg_time)));
			flydata->angstep.z = max(-MAX_ANGSTEP,min(MAX_ANGSTEP,fixdiv(delta_ang(flydata->angles.b,dest_angles.b),seg_time)));
			flydata->angstep.y = max(-MAX_ANGSTEP,min(MAX_ANGSTEP,fixdiv(delta_ang(flydata->angles.h,dest_angles.h),seg_time)));

		}
		else {
			flydata->angles = dest_angles;
			flydata->angstep.x = flydata->angstep.y = flydata->angstep.z = 0;
		}
	}

	flydata->first_time=0;
}

}

#define LINE_LEN	80
#define NUM_VARS	8

#define STATION_DIST	i2f(1024)

//called for each level to load & setup the exit sequence

void load_endlevel_data(int level_num)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	d_fname filename;
	char *p;
	int var;
	int have_binary = 0;

	endlevel_data_loaded = 0;		//not loaded yet

try_again:
	;

	if (level_num<0)		//secret level
		filename = Secret_level_names[-level_num-1];
	else					//normal level
		filename = Level_names[level_num-1];

#if defined(DXX_BUILD_DESCENT_I)
	if (!convert_ext(filename,"end"))
		return;
#elif defined(DXX_BUILD_DESCENT_II)
	if (!convert_ext(filename,"END"))
		Error("Error converting filename <%s> for endlevel data\n",static_cast<const char *>(filename));
#endif

	auto ifile = PHYSFSX_openReadBuffered(filename);

	if (!ifile) {

		convert_ext(filename,"txb");
                if (!strcmp(filename, Briefing_text_filename) || 
                !strcmp(filename, Ending_text_filename))
                    return;	// Don't want to interpret the briefing as an end level sequence!

		ifile = PHYSFSX_openReadBuffered(filename);

		if (!ifile) {
			if (level_num==1) {
#if defined(DXX_BUILD_DESCENT_II)
				con_printf(CON_DEBUG, "Cannot load file text of binary version of <%s>",static_cast<const char *>(filename));
				endlevel_data_loaded = 0; // won't be able to play endlevel sequence
#endif
				return;
			}
			else {
				level_num = 1;
				goto try_again;
			}
		}

		have_binary = 1;
	}

	//ok...this parser is pretty simple.  It ignores comments, but
	//everything else must be in the right place

	var = 0;

	PHYSFSX_gets_line_t<LINE_LEN> line;
	while (PHYSFSX_fgets(line,ifile)) {

		if (have_binary)
			decode_text_line (line);

		if ((p=strchr(line,';'))!=NULL)
			*p = 0;		//cut off comment

		for (p = line; isspace(static_cast<unsigned>(*p)); ++p)
			;
		if (!*p)		//empty line
			continue;
		auto ns = p;
		for (auto p2 = p; *p2; ++p2)
			if (!isspace(static_cast<unsigned>(*p2)))
				ns = p2;
		*++ns = 0;

		switch (var) {

			case 0: {						//ground terrain
				int iff_error;
				palette_array_t pal;
				terrain_bm_instance.reset();
				iff_error = iff_read_bitmap(p, terrain_bm_instance, &pal);
				if (iff_error != IFF_NO_ERROR) {
					con_printf(CON_DEBUG, "Can't load exit terrain from file %s: IFF error: %s",
                                                p, iff_errormsg(iff_error));
					endlevel_data_loaded = 0; // won't be able to play endlevel sequence
					return;
				}
				terrain_bitmap = &terrain_bm_instance;
				gr_remap_bitmap_good(terrain_bm_instance, pal, iff_transparent_color, -1);
				break;
			}

			case 1:							//height map

				load_terrain(p);
				break;


			case 2:

				sscanf(p,"%d,%d",&exit_point_bmx,&exit_point_bmy);
				break;

			case 3:							//exit heading

				exit_angles.h = i2f(atoi(p))/360;
				break;

			case 4: {						//planet bitmap
				int iff_error;
				palette_array_t pal;
				satellite_bm_instance.reset();
				iff_error = iff_read_bitmap(p, satellite_bm_instance, &pal);
				if (iff_error != IFF_NO_ERROR) {
					con_printf(CON_DEBUG, "Can't load exit satellite from file %s: IFF error: %s",
                                                p, iff_errormsg(iff_error));
					endlevel_data_loaded = 0; // won't be able to play endlevel sequence
					return;
				}

				satellite_bitmap = &satellite_bm_instance;
				gr_remap_bitmap_good(satellite_bm_instance, pal, iff_transparent_color, -1);

				break;
			}

			case 5:							//earth pos
			case 7: {						//station pos
				vms_angvec ta;
				int pitch,head;

				sscanf(p,"%d,%d",&head,&pitch);

				ta.h = i2f(head)/360;
				ta.p = -i2f(pitch)/360;
				ta.b = 0;

				const auto &&tm = vm_angles_2_matrix(ta);

				if (var==5)
					satellite_pos = tm.fvec;
					//vm_vec_copy_scale(&satellite_pos,&tm.fvec,SATELLITE_DIST);
				else
					station_pos = tm.fvec;

				break;
			}

			case 6:						//planet size
				satellite_size = i2f(atoi(p));
				break;
		}

		var++;

	}

	Assert(var == NUM_VARS);


	// OK, now the data is loaded.  Initialize everything

	//find the exit sequence by searching all segments for a side with
	//children == -2

	const auto &&exit_segside = find_exit_segment_side(vcsegptridx);
	const icsegidx_t &exit_segnum = exit_segside.first;
	const auto &exit_side = exit_segside.second;

	PlayerUniqueEndlevelState.exit_segnum = exit_segnum;
	if (exit_segnum == segment_none)
		return;

	const auto &&exit_seg = vmsegptr(exit_segnum);
	auto &vcvertptr = Vertices.vcptr;
	compute_segment_center(vcvertptr, mine_exit_point, exit_seg);
	extract_orient_from_segment(vcvertptr, mine_exit_orient, exit_seg);
	compute_center_point_on_side(vcvertptr, mine_side_exit_point, exit_seg, exit_side);

	vm_vec_scale_add(mine_ground_exit_point,mine_exit_point,mine_exit_orient.uvec,-i2f(20));

	//compute orientation of surface
	{
		auto &&exit_orient = vm_angles_2_matrix(exit_angles);
		vm_transpose_matrix(exit_orient);
		vm_matrix_x_matrix(surface_orient,mine_exit_orient,exit_orient);

		vms_matrix tm = vm_transposed_matrix(surface_orient);
		const auto tv0 = vm_vec_rotate(station_pos,tm);
		vm_vec_scale_add(station_pos,mine_exit_point,tv0,STATION_DIST);

		const auto tv = vm_vec_rotate(satellite_pos,tm);
		vm_vec_scale_add(satellite_pos,mine_exit_point,tv,SATELLITE_DIST);

		const auto tm2 = vm_vector_2_matrix(tv,&surface_orient.uvec,nullptr);
		vm_vec_copy_scale(satellite_upvec,tm2.uvec,SATELLITE_HEIGHT);


	}
	endlevel_data_loaded = 1;
}

}
