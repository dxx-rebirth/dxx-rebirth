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
 * Routines for displaying the auto-map.
 *
 */

#include "dxxsconf.h"
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef OGL
#include "ogl_init.h"
#endif

#include "dxxerror.h"
#include "3d.h"
#include "inferno.h"
#include "u_mem.h"
#include "render.h"
#include "object.h"
#include "vclip.h"
#include "game.h"
#include "polyobj.h"
#include "sounds.h"
#include "player.h"
#include "bm.h"
#include "key.h"
#include "newmenu.h"
#include "menu.h"
#include "screens.h"
#include "textures.h"
#include "hudmsg.h"
#include "mouse.h"
#include "timer.h"
#include "segpoint.h"
#include "joy.h"
#include "iff.h"
#include "pcx.h"
#include "palette.h"
#include "wall.h"
#include "hostage.h"
#include "fuelcen.h"
#include "physfsx.h"
#include "gameseq.h"
#include "gamefont.h"
#include "gameseg.h"
#include "common/3d/globvars.h"
#include "multi.h"
#include "kconfig.h"
#include "endlevel.h"
#include "text.h"
#include "gauges.h"
#include "powerup.h"
#include "switch.h"
#include "automap.h"
#include "cntrlcen.h"
#include "timer.h"
#include "config.h"
#include "playsave.h"
#include "rbaudio.h"
#include "window.h"
#include "playsave.h"
#include "args.h"
#include "physics.h"

#include "compiler-make_unique.h"
#include "compiler-range_for.h"
#include "partial_range.h"

#define LEAVE_TIME 0x4000

#define EF_USED     1   // This edge is used
#define EF_DEFINING 2   // A structure defining edge that should always draw.
#define EF_FRONTIER 4   // An edge between the known and the unknown.
#define EF_SECRET   8   // An edge that is part of a secret wall.
#define EF_GRATE    16  // A grate... draw it all the time.
#define EF_NO_FADE  32  // An edge that doesn't fade with distance
#define EF_TOO_FAR  64  // An edge that is too far away

namespace dcx {

namespace {

struct Edge_info
{
	array<int, 2>   verts;     // 8  bytes
	array<uint8_t, 4> sides;     // 4  bytes
	array<segnum_t, 4> segnum;    // 16 bytes  // This might not need to be stored... If you can access the normals of a side.
	ubyte flags;        // 1  bytes  // See the EF_??? defines above.
	color_t color;        // 1  bytes
	ubyte num_faces;    // 1  bytes  // 31 bytes...
};

}

}

namespace dsx {

namespace {

struct automap : ignore_window_pointer_t
{
	fix64			entry_time;
	fix64			t1, t2;
	int			leave_mode;
	int			pause_game;
	vms_angvec		tangles;
	ushort			old_wiggle; // keep 4 byte aligned
	int			max_segments_away;
	int			segment_limit;
	
	// Edge list variables
	int			num_edges;
	int			max_edges; //set each frame
	int			highest_edge_index;
	std::unique_ptr<Edge_info[]>		edges;
	std::unique_ptr<Edge_info *[]>			drawingListBright;
	
	// Screen canvas variables
	grs_canvas		automap_view;
	
	grs_bitmap		automap_background;
	
	// Rendering variables
	fix			zoom;
	vms_vector		view_target;
	vms_vector		view_position;
	fix			farthest_dist;
	vms_matrix		viewMatrix;
	fix			viewDist;
	
	color_t			wall_normal_color;
	color_t			wall_door_color;
	color_t			wall_door_blue;
	color_t			wall_door_gold;
	color_t			wall_door_red;
#if defined(DXX_BUILD_DESCENT_II)
	color_t			wall_revealed_color;
#endif
	color_t			hostage_color;
	color_t			green_31;
	color_t			white_63;
	color_t			blue_48;
	color_t			red_48;
	control_info controls;
	segment_depth_array_t depth_array;
};

}

}

namespace dcx {

#define MAX_EDGES_FROM_VERTS(v)     ((v)*4)

#define K_WALL_NORMAL_COLOR     BM_XRGB(29, 29, 29 )
#define K_WALL_DOOR_COLOR       BM_XRGB(5, 27, 5 )
#define K_WALL_DOOR_BLUE        BM_XRGB(0, 0, 31)
#define K_WALL_DOOR_GOLD        BM_XRGB(31, 31, 0)
#define K_WALL_DOOR_RED         BM_XRGB(31, 0, 0)
#define K_WALL_REVEALED_COLOR   BM_XRGB(0, 0, 25 ) //what you see when you have the full map powerup
#define K_HOSTAGE_COLOR         BM_XRGB(0, 31, 0 )
#define K_FONT_COLOR_20         BM_XRGB(20, 20, 20 )
#define K_GREEN_31              BM_XRGB(0, 31, 0)

int Automap_active = 0;
static int Automap_debug_show_all_segments;
}

namespace dsx {
static void init_automap_colors(automap *am)
{
	am->wall_normal_color = K_WALL_NORMAL_COLOR;
	am->wall_door_color = K_WALL_DOOR_COLOR;
	am->wall_door_blue = K_WALL_DOOR_BLUE;
	am->wall_door_gold = K_WALL_DOOR_GOLD;
	am->wall_door_red = K_WALL_DOOR_RED;
#if defined(DXX_BUILD_DESCENT_II)
	am->wall_revealed_color = K_WALL_REVEALED_COLOR;
#endif
	am->hostage_color = K_HOSTAGE_COLOR;
	am->green_31 = K_GREEN_31;

	am->white_63 = gr_find_closest_color_current(63,63,63);
	am->blue_48 = gr_find_closest_color_current(0,0,48);
	am->red_48 = gr_find_closest_color_current(48,0,0);
}
}

namespace dcx {
array<ubyte, MAX_SEGMENTS> Automap_visited; // Segment visited list
}

// Map movement defines
#define PITCH_DEFAULT 9000
#define ZOOM_DEFAULT i2f(20*10)
#define ZOOM_MIN_VALUE i2f(20*5)
#define ZOOM_MAX_VALUE i2f(20*100)

#define SLIDE_SPEED 			(350)
#define ZOOM_SPEED_FACTOR		(500)	//(1500)
#define ROT_SPEED_DIVISOR		(115000)

// Function Prototypes
namespace dsx {
static void adjust_segment_limit(automap *am, int SegmentLimit);
static void draw_all_edges(automap *am);
static void automap_build_edge_list(automap *am, int add_all_edges);
}

#define	MAX_DROP_MULTI	2
#define	MAX_DROP_SINGLE	9

#if defined(DXX_BUILD_DESCENT_II)
#include "compiler-integer_sequence.h"

namespace dsx {
int HighlightMarker=-1;
marker_message_text_t Marker_input;
marker_messages_array_t MarkerMessage;
static float MarkerScale=2.0;

template <std::size_t... N>
static inline constexpr array<objnum_t, sizeof...(N)> init_MarkerObject(index_sequence<N...>)
{
	return {{((void)N, object_none)...}};
}

array<objnum_t, NUM_MARKERS> MarkerObject = init_MarkerObject(make_tree_index_sequence<NUM_MARKERS>());
}
#endif

# define automap_draw_line g3_draw_line

// -------------------------------------------------------------

namespace dsx {
#if defined(DXX_BUILD_DESCENT_I)
static inline void DrawMarkers (automap *am)
{
	(void)am;
}

static inline void ClearMarkers()
{
}
#elif defined(DXX_BUILD_DESCENT_II)
static void DrawMarkerNumber(const automap *am, unsigned num, const g3s_point &BasePoint)
{
	struct xy
	{
		float x0, y0, x1, y1;
	};
	static constexpr array<array<xy, 5>, 9> sArray = {{
		{{
			{-0.25, 0.75, 0, 1},
			{0, 1, 0, -1},
			{-1, -1, 1, -1},
		}},
		{{
			{-1, 1, 1, 1},
			{1, 1, 1, 0},
			{-1, 0, 1, 0},
			{-1, 0, -1, -1},
			{-1, -1, 1, -1}
		}},
		{{
			{-1, 1, 1, 1},
			{1, 1, 1, -1},
			{-1, -1, 1, -1},
			{0, 0, 1, 0},
		}},
		{{
			{-1, 1, -1, 0},
			{-1, 0, 1, 0},
			{1, 1, 1, -1},
		}},
		{{
			{-1, 1, 1, 1},
			{-1, 1, -1, 0},
			{-1, 0, 1, 0},
			{1, 0, 1, -1},
			{-1, -1, 1, -1}
		}},
		{{
			{-1, 1, 1, 1},
			{-1, 1, -1, -1},
			{-1, -1, 1, -1},
			{1, -1, 1, 0},
			{-1, 0, 1, 0}
		}},
		{{
			{-1, 1, 1, 1},
			{1, 1, 1, -1},
		}},
		{{
			{-1, 1, 1, 1},
			{1, 1, 1, -1},
			{-1, -1, 1, -1},
			{-1, -1, -1, 1},
			{-1, 0, 1, 0}
		}},
		{{
			{-1, 1, 1, 1},
			{1, 1, 1, -1},
			{-1, 0, 1, 0},
			{-1, 0, -1, 1},
		 }}
	}};
	static constexpr array<uint_fast8_t, 9> NumOfPoints = {{3, 5, 4, 3, 5, 5, 2, 5, 4}};

	const auto color = (num == HighlightMarker ? am->white_63 : am->blue_48);
	const auto scale_x = Matrix_scale.x;
	const auto scale_y = Matrix_scale.y;
	range_for (const auto &i, unchecked_partial_range(&sArray[num][0], NumOfPoints[num]))
	{
		const auto ax0 = i.x0 * MarkerScale;
		const auto ay0 = i.y0 * MarkerScale;
		const auto ax1 = i.x1 * MarkerScale;
		const auto ay1 = i.y1 * MarkerScale;
		auto FromPoint = BasePoint;
		auto ToPoint = BasePoint;
		FromPoint.p3_x += fixmul(fl2f(ax0), scale_x);
		FromPoint.p3_y += fixmul(fl2f(ay0), scale_y);
		ToPoint.p3_x += fixmul(fl2f(ax1), scale_x);
		ToPoint.p3_y += fixmul(fl2f(ay1), scale_y);
		g3_code_point(FromPoint);
		g3_code_point(ToPoint);
		g3_project_point(FromPoint);
		g3_project_point(ToPoint);
		automap_draw_line(FromPoint, ToPoint, color);
	}
}

static void DropMarker (int player_marker_num)
{
	int marker_num = (Player_num*2)+player_marker_num;

	if (MarkerObject[marker_num] != object_none)
		obj_delete(vobjptridx(MarkerObject[marker_num]));

	const auto &playerp = get_local_plrobj();
	MarkerObject[marker_num] = drop_marker_object(playerp.pos, vsegptridx(playerp.segnum), playerp.orient, marker_num);

	if (Game_mode & GM_MULTI)
		multi_send_drop_marker(Player_num, playerp.pos, player_marker_num, MarkerMessage[marker_num]);
}

void DropBuddyMarker(const vobjptr_t objp)
{
	int marker_num;

	// Find spare marker slot.  "if" code below should be an assert, but what if someone changes NUM_MARKERS or MAX_CROP_SINGLE and it never gets hit?
	static_assert(MAX_DROP_SINGLE + 1 <= NUM_MARKERS - 1, "not enough markers");
	marker_num = MAX_DROP_SINGLE+1;
	if (marker_num > NUM_MARKERS-1)
		marker_num = NUM_MARKERS-1;

	snprintf(&MarkerMessage[marker_num][0], MarkerMessage[marker_num].size(), "RIP: %s", static_cast<const char *>(PlayerCfg.GuidebotName));

	if (MarkerObject[marker_num] != object_none)
		obj_delete(vobjptridx(MarkerObject[marker_num]));

	MarkerObject[marker_num] = drop_marker_object(objp->pos, vsegptridx(objp->segnum), objp->orient, marker_num);
}

#define MARKER_SPHERE_SIZE 0x58000

static void DrawMarkers (automap *am)
 {
	static int cyc=10,cycdir=1;

	const auto mb = &MarkerObject[(Player_num * 2)];
	const auto me = std::next(mb, (Game_mode & GM_MULTI) ? 2 : 9);
	for (auto iter = mb;;)
	{
		if (*iter != object_none)
			break;
		if (++ iter == me)
			return;
	}
	const auto current_cycle_color = cyc;
	const array<color_t, 3> colors{{
		gr_find_closest_color_current(current_cycle_color, 0, 0),
		gr_find_closest_color_current(current_cycle_color + 10, 0, 0),
		gr_find_closest_color_current(current_cycle_color + 20, 0, 0),
	}};
	unsigned i = 0;
	for (auto iter = mb; iter != me; ++iter, ++i)
		if (*iter != object_none)
		{
			auto sphere_point = g3_rotate_point(vcobjptr(*iter)->pos);
			g3_draw_sphere(sphere_point,MARKER_SPHERE_SIZE, colors[0]);
			g3_draw_sphere(sphere_point,MARKER_SPHERE_SIZE/2, colors[1]);
			g3_draw_sphere(sphere_point,MARKER_SPHERE_SIZE/4, colors[2]);
			DrawMarkerNumber(am, i, sphere_point);
		}

	if (cycdir)
		cyc+=2;
	else
		cyc-=2;

	if (cyc>43)
	{
		cyc=43;
		cycdir=0;
	}
	else if (cyc<10)
	{
		cyc=10;
		cycdir=1;
	}

}

static void ClearMarkers()
{
	int i;

	for (i=0;i<NUM_MARKERS;i++) {
		MarkerMessage[i][0]=0;
		MarkerObject[i]=object_none;
	}
}
#endif

void automap_clear_visited()	
{
	Automap_visited = {};
#ifndef NDEBUG
	Automap_debug_show_all_segments = 0;
#endif
		ClearMarkers();
}

static void draw_player(const object_base &obj, const uint8_t color)
{
	// Draw Console player -- shaped like a ellipse with an arrow.
	auto sphere_point = g3_rotate_point(obj.pos);
	const auto obj_size = obj.size;
	g3_draw_sphere(sphere_point, obj_size, color);

	// Draw shaft of arrow
	const auto &&head_pos = vm_vec_scale_add(obj.pos, obj.orient.fvec, obj_size * 2);
	{
	auto &&arrow_point = g3_rotate_point(vm_vec_scale_add(obj.pos, obj.orient.fvec, obj_size * 3));
	automap_draw_line(sphere_point, arrow_point, color);

	// Draw right head of arrow
	{
		const auto &&rhead_pos = vm_vec_scale_add(head_pos, obj.orient.rvec, obj_size);
		auto head_point = g3_rotate_point(rhead_pos);
		automap_draw_line(arrow_point, head_point, color);
	}

	// Draw left head of arrow
	{
		const auto &&lhead_pos = vm_vec_scale_add(head_pos, obj.orient.rvec, -obj_size);
		auto head_point = g3_rotate_point(lhead_pos);
		automap_draw_line(arrow_point, head_point, color);
	}
	}

	// Draw player's up vector
	{
		const auto &&arrow_pos = vm_vec_scale_add(obj.pos, obj.orient.uvec, obj_size * 2);
	auto arrow_point = g3_rotate_point(arrow_pos);
		automap_draw_line(sphere_point, arrow_point, color);
	}
}

#if defined(DXX_BUILD_DESCENT_II)
//name for each group.  maybe move somewhere else
constexpr char system_name[][17] = {
			"Zeta Aquilae",
			"Quartzon System",
			"Brimspark System",
			"Limefrost Spiral",
			"Baloris Prime",
			"Omega System"};
#endif

static void name_frame(automap *am)
{
	gr_set_curfont(GAME_FONT);
	gr_set_fontcolor(am->green_31,-1);
	char		name_level_left[128];

#if defined(DXX_BUILD_DESCENT_I)
	const char *name_level;
	if (Current_level_num > 0)
	{
		snprintf(name_level_left, sizeof(name_level_left), "%s %i: %s",TXT_LEVEL, Current_level_num, static_cast<const char *>(Current_level_name));
		name_level = name_level_left;
	}
	else
		name_level = Current_level_name;

	gr_string((SWIDTH/64),(SHEIGHT/48),name_level);
#elif defined(DXX_BUILD_DESCENT_II)
	char	name_level_right[128];
	if (Current_level_num > 0)
		snprintf(name_level_left, sizeof(name_level_left), "%s %i",TXT_LEVEL, Current_level_num);
	else
		snprintf(name_level_left, sizeof(name_level_left), "Secret Level %i",-Current_level_num);

	const char *const current_level_name = Current_level_name;
	if (PLAYING_BUILTIN_MISSION && Current_level_num > 0)
		snprintf(name_level_right, sizeof(name_level_right), "%s %d: %s", system_name[(Current_level_num-1)/4], ((Current_level_num - 1) % 4) + 1, current_level_name);
	else
		snprintf(name_level_right, sizeof(name_level_right), " %s", current_level_name);

	gr_string((SWIDTH/64),(SHEIGHT/48),name_level_left);
	int wr,h;
	gr_get_string_size(name_level_right, &wr, &h, nullptr);
	gr_string(grd_curcanv->cv_bitmap.bm_w-wr-(SWIDTH/64),(SHEIGHT/48),name_level_right, wr, h);
#endif
}

static void automap_apply_input(automap *am)
{
	if (PlayerCfg.AutomapFreeFlight)
	{
		if ( am->controls.state.fire_primary)
		{
			// Reset orientation
			am->controls.state.fire_primary = 0;
			auto &plrobj = get_local_plrobj();
			am->viewMatrix = plrobj.orient;
			vm_vec_scale_add(am->view_position, plrobj.pos, am->viewMatrix.fvec, -ZOOM_DEFAULT);
		}
		
		if (am->controls.pitch_time || am->controls.heading_time || am->controls.bank_time)
		{
			vms_angvec tangles;

			tangles.p = fixdiv( am->controls.pitch_time, ROT_SPEED_DIVISOR );
			tangles.h = fixdiv( am->controls.heading_time, ROT_SPEED_DIVISOR );
			tangles.b = fixdiv( am->controls.bank_time, ROT_SPEED_DIVISOR*2 );

			const auto &&tempm = vm_angles_2_matrix(tangles);
			am->viewMatrix = vm_matrix_x_matrix(am->viewMatrix,tempm);
			check_and_fix_matrix(am->viewMatrix);
		}
		
		if ( am->controls.forward_thrust_time || am->controls.vertical_thrust_time || am->controls.sideways_thrust_time )
		{
			vm_vec_scale_add2( am->view_position, am->viewMatrix.fvec, am->controls.forward_thrust_time*ZOOM_SPEED_FACTOR );
			vm_vec_scale_add2( am->view_position, am->viewMatrix.uvec, am->controls.vertical_thrust_time*SLIDE_SPEED );
			vm_vec_scale_add2( am->view_position, am->viewMatrix.rvec, am->controls.sideways_thrust_time*SLIDE_SPEED );
			
			// Crude wrapping check
			clamp_fix_symmetric(am->view_position.x, F1_0*32000);
			clamp_fix_symmetric(am->view_position.y, F1_0*32000);
			clamp_fix_symmetric(am->view_position.z, F1_0*32000);
		}
	}
	else
	{
		if ( am->controls.state.fire_primary)
		{
			// Reset orientation
			am->viewDist = ZOOM_DEFAULT;
			am->tangles.p = PITCH_DEFAULT;
			am->tangles.h  = 0;
			am->tangles.b  = 0;
			am->view_target = get_local_plrobj().pos;
			am->controls.state.fire_primary = 0;
		}

		am->viewDist -= am->controls.forward_thrust_time*ZOOM_SPEED_FACTOR;
		am->tangles.p += fixdiv( am->controls.pitch_time, ROT_SPEED_DIVISOR );
		am->tangles.h  += fixdiv( am->controls.heading_time, ROT_SPEED_DIVISOR );
		am->tangles.b  += fixdiv( am->controls.bank_time, ROT_SPEED_DIVISOR*2 );

		auto &plrobj = get_local_plrobj();
		if ( am->controls.vertical_thrust_time || am->controls.sideways_thrust_time )
		{
			vms_angvec      tangles1;
			vms_vector      old_vt;

			old_vt = am->view_target;
			tangles1 = am->tangles;
			const auto &&tempm = vm_angles_2_matrix(tangles1);
			vm_matrix_x_matrix(am->viewMatrix, plrobj.orient, tempm);
			vm_vec_scale_add2( am->view_target, am->viewMatrix.uvec, am->controls.vertical_thrust_time*SLIDE_SPEED );
			vm_vec_scale_add2( am->view_target, am->viewMatrix.rvec, am->controls.sideways_thrust_time*SLIDE_SPEED );
			if (vm_vec_dist_quick(am->view_target, plrobj.pos) > i2f(1000))
				am->view_target = old_vt;
		}

		const auto &&tempm = vm_angles_2_matrix(am->tangles);
		vm_matrix_x_matrix(am->viewMatrix, plrobj.orient, tempm);

		clamp_fix_lh(am->viewDist, ZOOM_MIN_VALUE, ZOOM_MAX_VALUE);
	}
}

static void draw_automap(automap *am)
{
	int i;
	if ( am->leave_mode==0 && am->controls.state.automap && (timer_query()-am->entry_time)>LEAVE_TIME)
		am->leave_mode = 1;

	gr_set_current_canvas(NULL);
	show_fullscr(am->automap_background);
	gr_set_curfont(HUGE_FONT);
	gr_set_fontcolor(BM_XRGB(20, 20, 20), -1);
#if defined(DXX_BUILD_DESCENT_I)
	if (MacHog)
		gr_string(80*(SWIDTH/640.0), 36*(SHEIGHT/480.0), TXT_AUTOMAP);
	else
#endif
		gr_string((SWIDTH/8), (SHEIGHT/16), TXT_AUTOMAP);
	gr_set_curfont(GAME_FONT);
	gr_set_fontcolor(BM_XRGB(20, 20, 20), -1);
#if defined(DXX_BUILD_DESCENT_I)
	if (!MacHog)
	{
		gr_string((SWIDTH/4.923), (SHEIGHT/1.126), TXT_TURN_SHIP);
		gr_string((SWIDTH/4.923), (SHEIGHT/1.083), TXT_SLIDE_UPDOWN);
		gr_string((SWIDTH/4.923), (SHEIGHT/1.043), "F9/F10 Changes viewing distance");
	}
	else
	{
		// for the Mac automap they're shown up the top, hence the different layout
		gr_string(265*(SWIDTH/640.0), 27*(SHEIGHT/480.0), TXT_TURN_SHIP);
		gr_string(265*(SWIDTH/640.0), 44*(SHEIGHT/480.0), TXT_SLIDE_UPDOWN);
		gr_string(265*(SWIDTH/640.0), 61*(SHEIGHT/480.0), "F9/F10 Changes viewing distance");
	}
#elif defined(DXX_BUILD_DESCENT_II)
	gr_string((SWIDTH/10.666), (SHEIGHT/1.126), TXT_TURN_SHIP);
	gr_string((SWIDTH/10.666), (SHEIGHT/1.083), "F9/F10 Changes viewing distance");
	gr_string((SWIDTH/10.666), (SHEIGHT/1.043), TXT_AUTOMAP_MARKER);
#endif

	gr_set_current_canvas(&am->automap_view);

	gr_clear_canvas(BM_XRGB(0,0,0));

	g3_start_frame();
	render_start_frame();

	if (!PlayerCfg.AutomapFreeFlight)
		vm_vec_scale_add(am->view_position,am->view_target,am->viewMatrix.fvec,-am->viewDist);

	g3_set_view_matrix(am->view_position,am->viewMatrix,am->zoom);

	draw_all_edges(am);

	// Draw player...
	const auto &self_ship_rgb = player_rgb[get_player_or_team_color(Player_num)];
	const auto closest_color = BM_XRGB(self_ship_rgb.r, self_ship_rgb.g, self_ship_rgb.b);
	draw_player(vcobjptr(get_local_player().objnum), closest_color);

	DrawMarkers(am);
	
	// Draw player(s)...
	if ( (Game_mode & (GM_TEAM | GM_MULTI_COOP)) || (Netgame.game_flag.show_on_map) )	{
		for (i=0; i<N_players; i++)		{
			if ( (i != Player_num) && ((Game_mode & GM_MULTI_COOP) || (get_team(Player_num) == get_team(i)) || (Netgame.game_flag.show_on_map)) )	{
				const auto &&objp = vcobjptr(Players[i].objnum);
				if (objp->type == OBJ_PLAYER)
				{
					const auto &other_ship_rgb = player_rgb[get_player_or_team_color(i)];
					draw_player(objp, BM_XRGB(other_ship_rgb.r, other_ship_rgb.g, other_ship_rgb.b));
				}
			}
		}
	}

	range_for (const auto &&objp, vobjptridx)
	{
		switch( objp->type )	{
		case OBJ_HOSTAGE:
			{
			auto sphere_point = g3_rotate_point(objp->pos);
			g3_draw_sphere(sphere_point,objp->size, am->hostage_color);
			}
			break;
		case OBJ_POWERUP:
			if (Automap_visited[objp->segnum] || Automap_debug_show_all_segments)
			{
				ubyte id = get_powerup_id(objp);
				unsigned r, g, b;
				if (id==POW_KEY_RED)
					r = 63 * 2, g = 5 * 2, b = 5 * 2;
				else if (id==POW_KEY_BLUE)
					r = 5 * 2, g = 5 * 2, b = 63 * 2;
				else if (id==POW_KEY_GOLD)
					r = 63 * 2, g = 63 * 2, b = 10 * 2;
				else
					break;
				{
					const auto color = gr_find_closest_color(r, g, b);
				auto sphere_point = g3_rotate_point(objp->pos);
				g3_draw_sphere(sphere_point,objp->size*4, color);
				}
			}
			break;
		}
	}

	g3_end_frame();

	name_frame(am);

#if defined(DXX_BUILD_DESCENT_II)
	if (HighlightMarker>-1 && MarkerMessage[HighlightMarker][0]!=0)
	{
		gr_printf((SWIDTH/64),(SHEIGHT/18), "Marker %d: %s",HighlightMarker+1,&MarkerMessage[(Player_num*2)+HighlightMarker][0]);
	}
#endif

	if (PlayerCfg.MouseFlightSim && PlayerCfg.MouseFSIndicator)
		show_mousefs_indicator(am->controls.raw_mouse_axis[0], am->controls.raw_mouse_axis[1], am->controls.raw_mouse_axis[2], GWIDTH-(GHEIGHT/8), GHEIGHT-(GHEIGHT/8), GHEIGHT/5);

	am->t2 = timer_query();
	const auto vsync = CGameCfg.VSync;
	const auto bound = F1_0 / (vsync ? MAXIMUM_FPS : CGameArg.SysMaxFPS);
	const auto may_sleep = !CGameArg.SysNoNiceFPS && !vsync;
	while (am->t2 - am->t1 < bound) // ogl is fast enough that the automap can read the input too fast and you start to turn really slow.  So delay a bit (and free up some cpu :)
	{
		if (Game_mode & GM_MULTI)
			multi_do_frame(); // during long wait, keep packets flowing
		if (may_sleep)
			timer_delay(F1_0>>8);
		am->t2 = timer_update();
	}
	if (am->pause_game)
	{
		FrameTime=am->t2-am->t1;
		calc_d_tick();
	}
	am->t1 = am->t2;
}

#if defined(DXX_BUILD_DESCENT_I)
#define MAP_BACKGROUND_FILENAME (((SWIDTH>=640&&SHEIGHT>=480) && PHYSFSX_exists("maph.pcx",1))?"MAPH.PCX":"MAP.PCX")
#elif defined(DXX_BUILD_DESCENT_II)
#define MAP_BACKGROUND_FILENAME ((HIRESMODE && PHYSFSX_exists("mapb.pcx",1))?"MAPB.PCX":"MAP.PCX")
#endif

static void recompute_automap_segment_visibility(automap *am)
{
	int compute_depth_all_segments = (cheats.fullautomap || (get_local_player_flags() & PLAYER_FLAGS_MAP_ALL));
	if (Automap_debug_show_all_segments)
		compute_depth_all_segments = 1;
	automap_build_edge_list(am, compute_depth_all_segments);
	am->max_segments_away = set_segment_depths(get_local_plrobj().segnum, compute_depth_all_segments ? NULL : &Automap_visited, am->depth_array);
	am->segment_limit = am->max_segments_away;
	adjust_segment_limit(am, am->segment_limit);
}

static window_event_result automap_key_command(window *, const d_event &event, automap *am)
{
	int c = event_key_get(event);
#if defined(DXX_BUILD_DESCENT_II)
	int marker_num;
	char maxdrop;
#endif

	switch (c)
	{
		case KEY_PRINT_SCREEN: {
			gr_set_current_canvas(NULL);
			save_screen_shot(1);
			return window_event_result::handled;
		}
		case KEY_ESC:
			if (am->leave_mode==0)
			{
				return window_event_result::close;
			}
			return window_event_result::handled;
#if defined(DXX_BUILD_DESCENT_I)
		case KEY_ALTED+KEY_F:           // Alt+F shows full map, if cheats enabled
			if (cheats.enabled) 	 
			{
				cheats.fullautomap = !cheats.fullautomap;
				// if cheat of map powerup, work with full depth
				recompute_automap_segment_visibility(am);
			}
			return window_event_result::handled;
#endif
#ifndef NDEBUG
		case KEY_DEBUGGED+KEY_F: 	{
				Automap_debug_show_all_segments = !Automap_debug_show_all_segments;
				recompute_automap_segment_visibility(am);
			}
			return window_event_result::handled;
#endif
		case KEY_F9:
			if (am->segment_limit > 1) 		{
				am->segment_limit--;
				adjust_segment_limit(am, am->segment_limit);
			}
			return window_event_result::handled;
		case KEY_F10:
			if (am->segment_limit < am->max_segments_away) 	{
				am->segment_limit++;
				adjust_segment_limit(am, am->segment_limit);
			}
			return window_event_result::handled;
#if defined(DXX_BUILD_DESCENT_II)
		case KEY_1:
		case KEY_2:
		case KEY_3:
		case KEY_4:
		case KEY_5:
		case KEY_6:
		case KEY_7:
		case KEY_8:
		case KEY_9:
		case KEY_0:
			if (Game_mode & GM_MULTI)
				maxdrop=2;
			else
				maxdrop=9;
			
			marker_num = c-KEY_1;
			if (marker_num<=maxdrop)
			{
				if (MarkerObject[marker_num] != object_none)
					HighlightMarker=marker_num;
			}
			return window_event_result::handled;
		case KEY_D+KEY_CTRLED:
			if (HighlightMarker > -1 && MarkerObject[HighlightMarker] != object_none) {
				gr_set_current_canvas(NULL);
				if (nm_messagebox( NULL, 2, TXT_YES, TXT_NO, "Delete Marker?" ) == 0) {
					/* FIXME: this event should be sent to other players
					 * so that they remove the marker.
					 */
					obj_delete(vobjptridx(exchange(MarkerObject[HighlightMarker], object_none)));
					MarkerMessage[HighlightMarker][0]=0;
					HighlightMarker = -1;
				}
				set_screen_mode(SCREEN_GAME);
			}
			return window_event_result::handled;
#ifndef RELEASE
		case KEY_F11:	//KEY_COMMA:
			if (MarkerScale>.5)
				MarkerScale-=.5;
			return window_event_result::handled;
		case KEY_F12:	//KEY_PERIOD:
			if (MarkerScale<30.0)
				MarkerScale+=.5;
			return window_event_result::handled;
#endif
#endif
	}
	return window_event_result::ignored;
}

static window_event_result automap_process_input(window *, const d_event &event, automap *am)
{
	Controls = am->controls;
	kconfig_read_controls(event, 1);
	am->controls = Controls;
	Controls = {};

	if ( !am->controls.state.automap && (am->leave_mode==1) )
	{
		return window_event_result::close;
	}
	
	if ( am->controls.state.automap)
	{
		am->controls.state.automap = 0;
		if (am->leave_mode==0)
		{
			return window_event_result::close;
		}
	}
	
	return window_event_result::ignored;
}

static window_event_result automap_handler(window *wind,const d_event &event, automap *am)
{
	switch (event.type)
	{
		case EVENT_WINDOW_ACTIVATED:
			game_flush_inputs();
			event_toggle_focus(1);
			key_toggle_repeat(0);
			break;

		case EVENT_WINDOW_DEACTIVATED:
			event_toggle_focus(0);
			key_toggle_repeat(1);
			break;

		case EVENT_IDLE:
		case EVENT_JOYSTICK_BUTTON_UP:
		case EVENT_JOYSTICK_BUTTON_DOWN:
		case EVENT_JOYSTICK_MOVED:
		case EVENT_MOUSE_BUTTON_UP:
		case EVENT_MOUSE_BUTTON_DOWN:
		case EVENT_MOUSE_MOVED:
		case EVENT_KEY_RELEASE:
			return automap_process_input(wind, event, am);
		case EVENT_KEY_COMMAND:
		{
			window_event_result kret = automap_key_command(wind, event, am);
			if (kret == window_event_result::ignored)
				kret = automap_process_input(wind, event, am);
			return kret;
		}
			
		case EVENT_WINDOW_DRAW:
                        automap_apply_input(am);
			draw_automap(am);
			break;
			
		case EVENT_WINDOW_CLOSE:
			if (!am->pause_game)
				ConsoleObject->mtype.phys_info.flags |= am->old_wiggle;		// Restore wiggle
			event_toggle_focus(0);
			key_toggle_repeat(1);
#ifdef OGL
			gr_free_bitmap_data(am->automap_background);
#endif
			std::default_delete<automap>()(am);
			window_set_visible(Game_wind, 1);
			Automap_active = 0;
			multi_send_msgsend_state(msgsend_none);
			return window_event_result::ignored;	// continue closing
			break;

		default:
			return window_event_result::ignored;
			break;
	}
	return window_event_result::handled;
}

void do_automap()
{
	int pcx_error;
	palette_array_t pal;
	automap *am = new automap{};
	window_create(&grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT, automap_handler, am);
	am->leave_mode = 0;
	am->pause_game = 1; // Set to 1 if everything is paused during automap...No pause during net.
	am->max_segments_away = 0;
	am->segment_limit = 1;
	am->num_edges = 0;
	am->highest_edge_index = -1;
	const auto max_edges = Num_segments * 12;
	am->max_edges = max_edges;
	am->edges = make_unique<Edge_info[]>(max_edges);
	am->drawingListBright = make_unique<Edge_info *[]>(max_edges);
	am->zoom = 0x9000;
	am->farthest_dist = (F1_0 * 20 * 50); // 50 segments away
	am->viewDist = 0;

	init_automap_colors(am);
	if ((Game_mode & GM_MULTI) && (!Endlevel_sequence))
		am->pause_game = 0;

	if (am->pause_game) {
		window_set_visible(Game_wind, 0);
	}
	else
	{
		am->old_wiggle = ConsoleObject->mtype.phys_info.flags & PF_WIGGLE;	// Save old wiggle
		ConsoleObject->mtype.phys_info.flags &= ~PF_WIGGLE;		// Turn off wiggle
	}

	//Max_edges = min(MAX_EDGES_FROM_VERTS(Num_vertices),MAX_EDGES); //make maybe smaller than max

	gr_set_current_canvas(NULL);

	if ( am->viewDist==0 )
		am->viewDist = ZOOM_DEFAULT;

	auto &plrobj = get_local_plrobj();
	am->viewMatrix = plrobj.orient;
	am->tangles.p = PITCH_DEFAULT;
	am->tangles.h  = 0;
	am->tangles.b  = 0;
	am->view_target = plrobj.pos;
	
	if (PlayerCfg.AutomapFreeFlight)
		vm_vec_scale_add(am->view_position, plrobj.pos, am->viewMatrix.fvec, -ZOOM_DEFAULT);

	am->t1 = am->entry_time = timer_query();
	am->t2 = am->t1;

	//Fill in Automap_visited from Objects[Players[Player_num].objnum].segnum
	recompute_automap_segment_visibility(am);

	// ZICO - code from above to show frame in OGL correctly. Redundant, but better readable.
	// KREATOR - Now applies to all platforms so double buffering is supported
	gr_init_bitmap_data(am->automap_background);
	pcx_error = pcx_read_bitmap(MAP_BACKGROUND_FILENAME, am->automap_background, pal);
	if (pcx_error != PCX_ERROR_NONE)
		Error("File %s - PCX error: %s", MAP_BACKGROUND_FILENAME, pcx_errormsg(pcx_error));
	gr_remap_bitmap_good(am->automap_background, pal, -1, -1);
#if defined(DXX_BUILD_DESCENT_I)
	if (MacHog)
		gr_init_sub_canvas(am->automap_view, grd_curscreen->sc_canvas, 38*(SWIDTH/640.0), 77*(SHEIGHT/480.0), 564*(SWIDTH/640.0), 381*(SHEIGHT/480.0));
	else
#endif
		gr_init_sub_canvas(am->automap_view, grd_curscreen->sc_canvas, (SWIDTH/23), (SHEIGHT/6), (SWIDTH/1.1), (SHEIGHT/1.45));

	gr_palette_load( gr_palette );
	Automap_active = 1;
	multi_send_msgsend_state(msgsend_automap);
}

void adjust_segment_limit(automap *am, int SegmentLimit)
{
	int i;
	Edge_info * e;

	const auto &depth_array = am->depth_array;
	const auto predicate = [&depth_array, SegmentLimit](const segnum_t &e1) {
		return depth_array[e1] <= SegmentLimit;
	};
	for (i=0; i<=am->highest_edge_index; i++ )	{
		e = &am->edges[i];
		// Unchecked for speed
		const auto &&range = unchecked_partial_range(e->segnum.begin(), e->num_faces);
		if (std::any_of(range.begin(), range.end(), predicate))
			e->flags &= ~EF_TOO_FAR;
		else
			e->flags |= EF_TOO_FAR;
	}
}

void draw_all_edges(automap *am)	
{
	int i,j;
	unsigned nbright = 0;
	ubyte nfacing,nnfacing;
	fix distance;
	fix min_distance = 0x7fffffff;

	for (i=0; i<=am->highest_edge_index; i++ )	{
		//e = &am->edges[Edge_used_list[i]];
		const auto e = &am->edges[i];
		if (!(e->flags & EF_USED)) continue;

		if ( e->flags & EF_TOO_FAR) continue;

		if (e->flags&EF_FRONTIER) { 	// A line that is between what we have seen and what we haven't
			if ( (!(e->flags&EF_SECRET))&&(e->color==am->wall_normal_color))
				continue; 	// If a line isn't secret and is normal color, then don't draw it
		}
		distance = Segment_points[e->verts[1]].p3_z;

		if (min_distance>distance )
			min_distance = distance;

		if (!rotate_list(e->verts).uand)
		{			//all off screen?
			nfacing = nnfacing = 0;
			auto &tv1 = Vertices[e->verts[0]];
			j = 0;
			while( j<e->num_faces && (nfacing==0 || nnfacing==0) )	{
				if (!g3_check_normal_facing( tv1, Segments[e->segnum[j]].sides[e->sides[j]].normals[0] ) )
					nfacing++;
				else
					nnfacing++;
				j++;
			}

			if ( nfacing && nnfacing )	{
				// a contour line
				am->drawingListBright[nbright++] = e;
			} else if ( e->flags&(EF_DEFINING|EF_GRATE) )	{
				if ( nfacing == 0 )	{
					const uint8_t color = (e->flags & EF_NO_FADE)
						? e->color
						: gr_fade_table[8][e->color];
					g3_draw_line(Segment_points[e->verts[0]], Segment_points[e->verts[1]], color);
				} 	else {
					am->drawingListBright[nbright++] = e;
				}
			}
		}
	}
		
	if ( min_distance < 0 ) min_distance = 0;

	// Sort the bright ones using a shell sort
	const auto &&range = unchecked_partial_range(am->drawingListBright.get(), nbright);
	std::sort(range.begin(), range.end(), [am](const Edge_info *const a, const Edge_info *const b) {
		const auto &v1 = a->verts[0];
		const auto &v2 = b->verts[0];
		return Segment_points[v1].p3_z < Segment_points[v2].p3_z;
	});
	// Draw the bright ones
	range_for (const auto e, range)
	{
		const auto p1 = &Segment_points[e->verts[0]];
		const auto p2 = &Segment_points[e->verts[1]];
		fix dist;
		dist = p1->p3_z - min_distance;
		// Make distance be 1.0 to 0.0, where 0.0 is 10 segments away;
		if ( dist < 0 ) dist=0;
		if ( dist >= am->farthest_dist ) continue;

		const auto color = (e->flags & EF_NO_FADE)
			? e->color
			: gr_fade_table[f2i((F1_0 - fixdiv(dist, am->farthest_dist)) * 31)][e->color];	
		g3_draw_line(*p1, *p2, color);
	}
}


//==================================================================
//
// All routines below here are used to build the Edge list
//
//==================================================================


//finds edge, filling in edge_ptr. if found old edge, returns index, else return -1
static int automap_find_edge(automap *am, int v0,int v1,Edge_info *&edge_ptr)
{
	long vv, evv;
	int hash, oldhash;
	int ret, ev0, ev1;

	vv = (v1<<16) + v0;

	oldhash = hash = ((v0*5+v1) % am->max_edges);

	ret = -1;

	while (ret==-1) {
		ev0 = am->edges[hash].verts[0];
		ev1 = am->edges[hash].verts[1];
		evv = (ev1<<16)+ev0;
		if (am->edges[hash].num_faces == 0 ) ret=0;
		else if (evv == vv) ret=1;
		else {
			if (++hash==am->max_edges) hash=0;
			if (hash==oldhash) Error("Edge list full!");
		}
	}

	edge_ptr = &am->edges[hash];

	if (ret == 0)
		return -1;
	else
		return hash;

}


static void add_one_edge( automap *am, int va, int vb, ubyte color, ubyte side, segnum_t segnum, int hidden, int grate, int no_fade )	{
	int found;
	Edge_info *e;

	if ( am->num_edges >= am->max_edges)	{
		// GET JOHN! (And tell him that his
		// MAX_EDGES_FROM_VERTS formula is hosed.)
		// If he's not around, save the mine,
		// and send him  mail so he can look
		// at the mine later. Don't modify it.
		// This is important if this happens.
		Int3();		// LOOK ABOVE!!!!!!
		return;
	}

	if ( va > vb )	{
		std::swap(va, vb);
	}
	found = automap_find_edge(am,va,vb,e);
		
	if (found == -1) {
		e->verts[0] = va;
		e->verts[1] = vb;
		e->color = color;
		e->num_faces = 1;
		e->flags = EF_USED | EF_DEFINING;			// Assume a normal line
		e->sides[0] = side;
		e->segnum[0] = segnum;
		//Edge_used_list[am->num_edges] = e-am->edges;
		if ( (e-am->edges.get()) > am->highest_edge_index )
			am->highest_edge_index = e - am->edges.get();
		am->num_edges++;
	} else {
		if ( color != am->wall_normal_color )
#if defined(DXX_BUILD_DESCENT_II)
			if (color != am->wall_revealed_color)
#endif
				e->color = color;

		if ( e->num_faces < 4 ) {
			e->sides[e->num_faces] = side;
			e->segnum[e->num_faces] = segnum;
			e->num_faces++;
		}
	}

	if ( grate )
		e->flags |= EF_GRATE;

	if ( hidden )
		e->flags|=EF_SECRET;		// Mark this as a hidden edge
	if ( no_fade )
		e->flags |= EF_NO_FADE;
}

static void add_one_unknown_edge( automap *am, int va, int vb )
{
	int found;
	Edge_info *e;

	if ( va > vb )	{
		std::swap(va, vb);
	}

	found = automap_find_edge(am,va,vb,e);
	if (found != -1) 	
		e->flags|=EF_FRONTIER;		// Mark as a border edge
}

static void add_segment_edges(automap *am, const vcsegptridx_t seg)
{
	int 	is_grate, no_fade;
	ubyte	color;
	int	sn;
	const auto &segnum = seg;
	int	hidden_flag;
	
	for (sn=0;sn<MAX_SIDES_PER_SEGMENT;sn++) {
		hidden_flag = 0;

		is_grate = 0;
		no_fade = 0;

		color = 255;
		if (seg->children[sn] == segment_none) {
			color = am->wall_normal_color;
		}

		switch( seg->special )	{
		case SEGMENT_IS_FUELCEN:
			color = BM_XRGB( 29, 27, 13 );
			break;
		case SEGMENT_IS_CONTROLCEN:
			if (Control_center_present)
				color = BM_XRGB( 29, 0, 0 );
			break;
		case SEGMENT_IS_ROBOTMAKER:
			color = BM_XRGB( 29, 0, 31 );
			break;
		}

		if (seg->sides[sn].wall_num != wall_none)	{
		
			auto &w = *vcwallptr(seg->sides[sn].wall_num);
#if defined(DXX_BUILD_DESCENT_II)
			auto trigger_num = w.trigger;
			if (trigger_num != trigger_none && vtrgptr(trigger_num)->type == TT_SECRET_EXIT)
				{
			    color = BM_XRGB( 29, 0, 31 );
				 no_fade=1;
				 goto Here;
				} 	
#endif

			switch(w.type)
			{
			case WALL_DOOR:
				if (w.keys == KEY_BLUE) {
					no_fade = 1;
					color = am->wall_door_blue;
				} else if (w.keys == KEY_GOLD) {
					no_fade = 1;
					color = am->wall_door_gold;
				} else if (w.keys == KEY_RED) {
					no_fade = 1;
					color = am->wall_door_red;
				} else if (!(WallAnims[w.clip_num].flags & WCF_HIDDEN)) {
					auto connected_seg = seg->children[sn];
					if (connected_seg != segment_none) {
						const auto &vcseg = vcsegptr(connected_seg);
						const auto &connected_side = find_connect_side(seg, vcseg);
						switch (vcwallptr(vcseg->sides[connected_side].wall_num)->keys)
						{
								case KEY_BLUE:	color = am->wall_door_blue;	no_fade = 1; break;
								case KEY_GOLD:	color = am->wall_door_gold;	no_fade = 1; break;
								case KEY_RED:	color = am->wall_door_red;	no_fade = 1; break;
							default:
								color = am->wall_door_color;
								break;
						}
					}
				} else {
					color = am->wall_normal_color;
					hidden_flag = 1;
				}
				break;
			case WALL_CLOSED:
				// Make grates draw properly
				// NOTE: In original D1, is_grate is 1, hidden_flag not used so grates never fade. I (zico) like this so I leave this alone for now. 
				if (WALL_IS_DOORWAY(seg,sn) & WID_RENDPAST_FLAG)
					is_grate = 1;
				else
					hidden_flag = 1;
				color = am->wall_normal_color;
				break;
			case WALL_BLASTABLE:
				// Hostage doors
				color = am->wall_door_color;	
				break;
			}
		}
	
		if (segnum==Player_init[Player_num].segnum)
			color = BM_XRGB(31,0,31);

		if ( color != 255 )	{
#if defined(DXX_BUILD_DESCENT_II) 
			// If they have a map powerup, draw unvisited areas in dark blue.
			// NOTE: D1 originally had this part of code but w/o cheat-check. It's only supposed to draw blue with powerup that does not exist in D1. So make this D2-only
			if (!Automap_debug_show_all_segments)
			if ((cheats.fullautomap || get_local_player_flags() & PLAYER_FLAGS_MAP_ALL) && (!Automap_visited[segnum]))	
				color = am->wall_revealed_color;
			Here:
#endif
			const auto vertex_list = get_side_verts(segnum,sn);
			add_one_edge( am, vertex_list[0], vertex_list[1], color, sn, segnum, hidden_flag, 0, no_fade );
			add_one_edge( am, vertex_list[1], vertex_list[2], color, sn, segnum, hidden_flag, 0, no_fade );
			add_one_edge( am, vertex_list[2], vertex_list[3], color, sn, segnum, hidden_flag, 0, no_fade );
			add_one_edge( am, vertex_list[3], vertex_list[0], color, sn, segnum, hidden_flag, 0, no_fade );

			if ( is_grate )	{
				add_one_edge( am, vertex_list[0], vertex_list[2], color, sn, segnum, hidden_flag, 1, no_fade );
				add_one_edge( am, vertex_list[1], vertex_list[3], color, sn, segnum, hidden_flag, 1, no_fade );
			}
		}
	}
}


// Adds all the edges from a segment we haven't visited yet.

static void add_unknown_segment_edges(automap *am, const vcsegptridx_t seg)
{
	int sn;
	const auto &segnum = seg;
	for (sn=0;sn<MAX_SIDES_PER_SEGMENT;sn++) {
		// Only add edges that have no children
		if (seg->children[sn] == segment_none) {
			const auto vertex_list = get_side_verts(segnum,sn);
	
			add_one_unknown_edge( am, vertex_list[0], vertex_list[1] );
			add_one_unknown_edge( am, vertex_list[1], vertex_list[2] );
			add_one_unknown_edge( am, vertex_list[2], vertex_list[3] );
			add_one_unknown_edge( am, vertex_list[3], vertex_list[0] );
		}
	}
}

void automap_build_edge_list(automap *am, int add_all_edges)
{	
	int	i,e1,e2;
	Edge_info * e;

	// clear edge list
	for (i=0; i<am->max_edges; i++) {
		am->edges[i].num_faces = 0;
		am->edges[i].flags = 0;
	}
	am->num_edges = 0;
	am->highest_edge_index = -1;

	if (add_all_edges)	{
		// Cheating, add all edges as visited
		range_for (const auto &&segp, vcsegptridx)
		{
#if DXX_USE_EDITOR
			if (segp->segnum != segment_none)
#endif
			{
				add_segment_edges(am, segp);
			}
		}
	} else {
		// Not cheating, add visited edges, and then unvisited edges
		range_for (const auto &&segp, vcsegptridx)
		{
#if DXX_USE_EDITOR
			if (segp->segnum != segment_none)
#endif
				if (Automap_visited[segp]) {
					add_segment_edges(am, segp);
				}
		}
		range_for (const auto &&segp, vcsegptridx)
		{
#if DXX_USE_EDITOR
			if (segp->segnum != segment_none)
#endif
				if (!Automap_visited[segp]) {
					add_unknown_segment_edges(am, segp);
				}
		}
	}

	// Find unnecessary lines (These are lines that don't have to be drawn because they have small curvature)
	for (i=0; i<=am->highest_edge_index; i++ )	{
		e = &am->edges[i];
		if (!(e->flags&EF_USED)) continue;

		for (e1=0; e1<e->num_faces; e1++ )	{
			for (e2=1; e2<e->num_faces; e2++ )	{
				if ( (e1 != e2) && (e->segnum[e1] != e->segnum[e2]) )	{
					if ( vm_vec_dot( Segments[e->segnum[e1]].sides[e->sides[e1]].normals[0], Segments[e->segnum[e2]].sides[e->sides[e2]].normals[0] ) > (F1_0-(F1_0/10))  )	{
						e->flags &= (~EF_DEFINING);
						break;
					}
				}
			}
			if (!(e->flags & EF_DEFINING))
				break;
		}
	}
}

#if defined(DXX_BUILD_DESCENT_II)
static unsigned Marker_index;
ubyte DefiningMarkerMessage=0;
static uint8_t MarkerBeingDefined;
static uint8_t LastMarkerDropped;

void InitMarkerInput ()
{
	int maxdrop,i;

	//find free marker slot

	if (Game_mode & GM_MULTI)
	maxdrop=MAX_DROP_MULTI;
	else
	maxdrop=MAX_DROP_SINGLE;

	for (i=0;i<maxdrop;i++)
		if (MarkerObject[(Player_num*2)+i] == object_none)		//found free slot!
			break;

	if (i==maxdrop)		//no free slot
	{
		if (Game_mode & GM_MULTI)
			i = !LastMarkerDropped;		//in multi, replace older of two
		else {
			HUD_init_message_literal(HM_DEFAULT, "No free marker slots");
			return;
		}
	}

	//got a free slot.  start inputting marker message

	Marker_input[0]=0;
	Marker_index=0;
	DefiningMarkerMessage=1;
	MarkerBeingDefined = i;
	key_toggle_repeat(1);
}

window_event_result MarkerInputMessage(int key)
{
	switch( key )
	{
		case KEY_LEFT:
		case KEY_BACKSP:
		case KEY_PAD4:
			if (Marker_index > 0)
				Marker_index--;
			Marker_input[Marker_index] = 0;
			break;
		case KEY_ENTER:
			MarkerMessage[(Player_num*2)+MarkerBeingDefined] = Marker_input;
			DropMarker(MarkerBeingDefined);
			LastMarkerDropped = MarkerBeingDefined;
			/* fallthrough */
		case KEY_F8:
		case KEY_ESC:
			key_toggle_repeat(0);
			game_flush_inputs();
			DefiningMarkerMessage = 0;
			break;
		default:
		{
			int ascii = key_ascii();
			if ((ascii < 255 ))
				if (Marker_index < Marker_input.size() - 1)
				{
					Marker_input[Marker_index++] = ascii;
					Marker_input[Marker_index] = 0;
				}
			return window_event_result::ignored;
		}
	}
	return window_event_result::handled;
}
#endif
}
