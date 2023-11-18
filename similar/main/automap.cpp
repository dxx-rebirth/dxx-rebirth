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

#if DXX_USE_OGL
#include "ogl_init.h"
#endif

#include "dxxerror.h"
#include "3d.h"
#include "inferno.h"
#include "u_mem.h"
#include "render.h"
#include "object.h"
#include "game.h"
#include "polyobj.h"
#include "sounds.h"
#include "player.h"
#include "bm.h"
#include "key.h"
#include "newmenu.h"
#include "textures.h"
#include "hudmsg.h"
#include "timer.h"
#include "segpoint.h"
#include "joy.h"
#include "pcx.h"
#include "palette.h"
#include "wall.h"
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
#include "timer.h"
#include "config.h"
#include "playsave.h"
#include "window.h"
#include "playsave.h"
#include "args.h"
#include "physics.h"

#include "compiler-range_for.h"
#include "d_enumerate.h"
#include "d_levelstate.h"
#include "d_range.h"
#include "d_zip.h"
#include <memory>

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
	std::array<vertnum_t, 2> verts;     // 8  bytes
	std::array<sidenum_t, 4> sides;     // 4  bytes
	std::array<segnum_t, 4> segnum;    // 16 bytes  // This might not need to be stored... If you can access the normals of a side.
	ubyte flags;        // 1  bytes  // See the EF_??? defines above.
	color_t color;        // 1  bytes
	ubyte num_faces;    // 1  bytes  // 31 bytes...
};

struct automap : ::dcx::window
{
	using ::dcx::window::window;
	fix64			entry_time;
	fix64			t1, t2;
	static_assert(PF_WIGGLE < UINT8_MAX, "storing PF_WIGGLE into old_wiggle would truncate the value");
	uint8_t			old_wiggle;
	uint8_t			leave_mode = 0;
	uint8_t			pause_game;
	vms_angvec		tangles;
	int max_segments_away = 0;
	int segment_limit = 1;

	// Edge list variables
	int num_edges = 0;
	unsigned max_edges; //set each frame
	unsigned end_valid_edges = 0;
	std::unique_ptr<Edge_info[]>		edges;
	std::unique_ptr<Edge_info *[]>			drawingListBright;

	// Screen canvas variables
	grs_subcanvas		automap_view;

	grs_main_bitmap		automap_background;

	// Rendering variables
	fix			zoom = 0x9000;
	vms_vector		view_target;
	vms_vector		view_position;
	fix			farthest_dist = (F1_0 * 20 * 50); // 50 segments away
	vms_matrix		viewMatrix;
	fix			viewDist = 0;

	segment_depth_array_t depth_array;
	color_t			wall_normal_color;
	color_t			wall_door_color;
	color_t			wall_door_blue;
	color_t			wall_door_gold;
	color_t			wall_door_red;
	color_t			hostage_color;
	color_t			green_31;
};

}

}

namespace dsx {

namespace {

struct automap : ::dcx::automap
{
	using ::dcx::automap::automap;
#if defined(DXX_BUILD_DESCENT_II)
	color_t white_63;
	color_t blue_48;
	color_t wall_revealed_color;
#endif
	control_info controls;
	virtual window_event_result event_handler(const d_event &) override;
};

static void init_automap_subcanvas(grs_subcanvas &view, grs_canvas &container)
{
#if DXX_USE_STEREOSCOPIC_RENDER
	if (VR_stereo != StereoFormat::None) {
		int x = (SWIDTH/23), y = (SHEIGHT/6), w = (SWIDTH/1.1), h = (SHEIGHT/1.45);
		gr_stereo_viewport_window(VR_stereo, x, y, w, h);
		gr_init_sub_canvas(view, container, x, y, w, h);
		return;
	}
#endif
#if defined(DXX_BUILD_DESCENT_I)
	if (MacHog)
		gr_init_sub_canvas(view, container, 38*(SWIDTH/640.0), 77*(SHEIGHT/480.0), 564*(SWIDTH/640.0), 381*(SHEIGHT/480.0));
	else
#endif
		gr_init_sub_canvas(view, container, (SWIDTH/23), (SHEIGHT/6), (SWIDTH/1.1), (SHEIGHT/1.45));
}

static void automap_build_edge_list(automap &am, int add_all_edges);
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

namespace {

#ifndef NDEBUG
static uint8_t Automap_debug_show_all_segments;
#endif

static void automap_clear_visited(d_level_unique_automap_state &LevelUniqueAutomapState)
{
#ifndef NDEBUG
	Automap_debug_show_all_segments = 0;
#endif
	LevelUniqueAutomapState.Automap_visited = {};
}

static void init_automap_colors(automap &am)
{
	am.wall_normal_color = K_WALL_NORMAL_COLOR;
	am.wall_door_color = K_WALL_DOOR_COLOR;
	am.wall_door_blue = K_WALL_DOOR_BLUE;
	am.wall_door_gold = K_WALL_DOOR_GOLD;
	am.wall_door_red = K_WALL_DOOR_RED;
	am.hostage_color = K_HOSTAGE_COLOR;
	am.green_31 = K_GREEN_31;
}

void adjust_segment_limit(automap &am, const unsigned SegmentLimit)
{
	const auto &depth_array = am.depth_array;
	const auto predicate = [&depth_array, SegmentLimit](const segnum_t &e1) {
		return depth_array[e1] <= SegmentLimit;
	};
	for (auto &i : unchecked_partial_range(am.edges.get(), am.end_valid_edges))
	{
		// Unchecked for speed
		const auto &&range = unchecked_partial_range(i.segnum, i.num_faces);
		if (std::any_of(range.begin(), range.end(), predicate))
			i.flags &= ~EF_TOO_FAR;
		else
			i.flags |= EF_TOO_FAR;
	}
}

}

}

namespace dsx {

namespace {

static void recompute_automap_segment_visibility(const d_level_unique_automap_state &LevelUniqueAutomapState, unsigned compute_depth_all_segments, const segnum_t initial_segnum, automap &am)
{
#ifndef NDEBUG
	if (Automap_debug_show_all_segments)
		compute_depth_all_segments = 1;
#endif
	if (cheats.fullautomap)
		compute_depth_all_segments = 1;
	automap_build_edge_list(am, compute_depth_all_segments);
	am.max_segments_away = set_segment_depths(initial_segnum, compute_depth_all_segments ? nullptr : &LevelUniqueAutomapState.Automap_visited, am.depth_array);
	am.segment_limit = am.max_segments_away;
	adjust_segment_limit(am, am.segment_limit);
}

#if defined(DXX_BUILD_DESCENT_II)
#ifdef RELEASE
constexpr
#endif
static float MarkerScale = 2.0;
static unsigned Marker_index;

struct marker_delete_are_you_sure_menu : std::array<newmenu_item, 2>, newmenu
{
	using array_type = std::array<newmenu_item, 2>;
	d_level_unique_object_state &LevelUniqueObjectState;
	segment_array &Segments;
	d_marker_state &MarkerState;
	marker_delete_are_you_sure_menu(grs_canvas &canvas, d_level_unique_object_state &LevelUniqueObjectState, segment_array &Segments, d_marker_state &MarkerState) :
		array_type{{
			newmenu_item::nm_item_menu{TXT_YES},
			newmenu_item::nm_item_menu{TXT_NO},
		}},
		newmenu(menu_title{nullptr}, menu_subtitle{"Delete Marker?"}, menu_filename{nullptr}, tiny_mode_flag::normal, tab_processing_flag::ignore, adjusted_citem::create(*static_cast<array_type *>(this), 0), canvas),
		LevelUniqueObjectState(LevelUniqueObjectState),
		Segments(Segments),
		MarkerState(MarkerState)
	{
	}
	virtual window_event_result event_handler(const d_event &) override;
	static std::pair<imobjidx_t *, game_marker_index> get_marker_object(d_marker_state &MarkerState);
	void handle_selected_yes() const;
};

window_event_result marker_delete_are_you_sure_menu::event_handler(const d_event &event)
{
	switch (event.type)
	{
		case EVENT_NEWMENU_SELECTED:
		{
			const auto citem = static_cast<const d_select_event &>(event).citem;
			if (citem == 0)
				/* User chose Yes */
				handle_selected_yes();
			/* The dialog should close after the user picks Yes or No.
			 */
			return window_event_result::close;
		}
		default:
			return newmenu::event_handler(event);
	}
}

std::pair<imobjidx_t *, game_marker_index> marker_delete_are_you_sure_menu::get_marker_object(d_marker_state &MarkerState)
{
	const auto HighlightMarker = MarkerState.HighlightMarker;
	if (!MarkerState.imobjidx.valid_index(HighlightMarker))
		return {nullptr, HighlightMarker};
	auto &mo = MarkerState.imobjidx[HighlightMarker];
	return {mo == object_none ? nullptr : &mo, HighlightMarker};
}

void marker_delete_are_you_sure_menu::handle_selected_yes() const
{
	const auto [mo, HighlightMarker] = get_marker_object(MarkerState);
	if (!mo)
		/* Check that the selected marker is still a valid object. */
		return;
	/* FIXME: this event should be sent to other players
	 * so that they remove the marker.
	 */
	obj_delete(LevelUniqueObjectState, Segments, LevelUniqueObjectState.Objects.vmptridx(std::exchange(*mo, object_none)));
	MarkerState.message[HighlightMarker] = {};
	MarkerState.HighlightMarker = game_marker_index::None;
}

void init_automap_colors(automap &am)
{
	::dcx::init_automap_colors(am);
	am.white_63 = gr_find_closest_color_current(63, 63, 63);
	am.blue_48 = gr_find_closest_color_current(0, 0, 48);
	am.wall_revealed_color = K_WALL_REVEALED_COLOR;
}
#endif

// Map movement defines
#define PITCH_DEFAULT 9000
#define ZOOM_DEFAULT i2f(20*10)
#define ZOOM_MIN_VALUE i2f(20*5)
#define ZOOM_MAX_VALUE i2f(20*100)

}

/* MAX_DROP_MULTI_* must be a power of 2 for LastMarkerDropped to work
 * properly.
 */
#define	MAX_DROP_MULTI_COOP_0	2
#define	MAX_DROP_MULTI_COOP_1	4
#define MAX_DROP_MULTI_COOP_P	(max_numplayers > 4)
#define	MAX_DROP_MULTI_COOP	(MAX_DROP_MULTI_COOP_P ? MAX_DROP_MULTI_COOP_0 : MAX_DROP_MULTI_COOP_1)
#define	MAX_DROP_MULTI_COMPETITIVE	2
#define	MAX_DROP_SINGLE	9

#if defined(DXX_BUILD_DESCENT_II)
marker_message_text_t Marker_input;

d_marker_state MarkerState;

game_marker_index convert_player_marker_index_to_game_marker_index(const game_mode_flags game_mode, const unsigned max_numplayers, const unsigned player_num, const player_marker_index player_marker_num)
{
	if (game_mode & GM_MULTI_COOP)
		return static_cast<game_marker_index>((player_num * MAX_DROP_MULTI_COOP) + static_cast<unsigned>(player_marker_num));
	if (game_mode & GM_MULTI)
		return static_cast<game_marker_index>((player_num * MAX_DROP_MULTI_COMPETITIVE) + static_cast<unsigned>(player_marker_num));
	return static_cast<game_marker_index>(player_marker_num);
}

unsigned d_marker_state::get_markers_per_player(const game_mode_flags game_mode, const unsigned max_numplayers)
{
	if (game_mode & GM_MULTI_COOP)
		return MAX_DROP_MULTI_COOP;
	if (game_mode & GM_MULTI)
		return MAX_DROP_MULTI_COMPETITIVE;
	return MAX_DROP_SINGLE;
}

xrange<player_marker_index> get_player_marker_range(const unsigned maxdrop)
{
	const auto base = player_marker_index::_0;
	return {base, static_cast<player_marker_index>(static_cast<unsigned>(base) + maxdrop)};
}

playernum_t get_marker_owner(const game_mode_flags game_mode, const game_marker_index gmi, const unsigned max_numplayers)
{
	const auto ugmi = static_cast<unsigned>(gmi);
	if (game_mode & GM_MULTI_COOP)
	{
		/* This is split out to encourage the compiler to recognize that
		 * the divisor is a constant in every path, and in every path,
		 * the divisor was chosen to allow use of right shift in place
		 * of division.
		 */
		if (MAX_DROP_MULTI_COOP_P)
			return ugmi / MAX_DROP_MULTI_COOP_0;
		return ugmi / MAX_DROP_MULTI_COOP_1;
	}
	if (game_mode & GM_MULTI)
		return ugmi / MAX_DROP_MULTI_COMPETITIVE;
	return 0;
}

namespace {

xrange<game_marker_index> get_game_marker_range(const game_mode_flags game_mode, const unsigned max_numplayers, const unsigned player_num, const unsigned maxdrop)
{
	const auto base = convert_player_marker_index_to_game_marker_index(game_mode, max_numplayers, player_num, player_marker_index::_0);
	return {base, static_cast<game_marker_index>(static_cast<unsigned>(base) + maxdrop)};
}

}
#endif

// -------------------------------------------------------------

namespace {

static void draw_all_edges(automap &am);
#if defined(DXX_BUILD_DESCENT_II)
static void DrawMarkerNumber(grs_canvas &canvas, const automap &am, const game_marker_index gmi, const player_marker_index pmi, const g3s_point &BasePoint)
{
	struct xy
	{
		float x0, y0, x1, y1;
	};
	static constexpr enumerated_array<std::array<xy, 5>, 9, player_marker_index> sArray = {{{
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
	}}};
	static constexpr enumerated_array<uint_fast8_t, 9, player_marker_index> NumOfPoints = {{{3, 5, 4, 3, 5, 5, 2, 5, 4}}};

	const auto color = (gmi == MarkerState.HighlightMarker ? am.white_63 : am.blue_48);
	const g3_draw_line_context text_line_context{canvas, color};
	const auto scale_x = Matrix_scale.x;
	const auto scale_y = Matrix_scale.y;
	range_for (const auto &i, unchecked_partial_range(sArray[pmi], NumOfPoints[pmi]))
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
		g3_draw_line(text_line_context, FromPoint, ToPoint);
	}
}

static void DropMarker(fvmobjptridx &vmobjptridx, fvmsegptridx &vmsegptridx, const object &plrobj, const game_marker_index marker_num, const player_marker_index player_marker_num)
{
	auto &marker_objidx = MarkerState.imobjidx[marker_num];
	if (marker_objidx != object_none)
		obj_delete(LevelUniqueObjectState, Segments, vmobjptridx(marker_objidx));

	marker_objidx = drop_marker_object(plrobj.pos, vmsegptridx(plrobj.segnum), plrobj.orient, marker_num);

	if (Game_mode & GM_MULTI)
		multi_send_drop_marker(Player_num, plrobj.pos, player_marker_num, MarkerState.message[marker_num]);
}

}

void DropBuddyMarker(object &objp)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;

	constexpr auto marker_num = game_marker_index::GuidebotDeathSite;
	static_assert(MarkerState.message.valid_index(marker_num), "not enough markers");

	auto &MarkerMessage = MarkerState.message[marker_num];
	snprintf(&MarkerMessage[0u], MarkerMessage.size(), "RIP: %s", static_cast<const char *>(PlayerCfg.GuidebotName));

	auto &marker_objidx = MarkerState.imobjidx[marker_num];
	if (marker_objidx != object_none)
		obj_delete(LevelUniqueObjectState, Segments, vmobjptridx(marker_objidx));

	marker_objidx = drop_marker_object(objp.pos, vmsegptridx(objp.segnum), objp.orient, marker_num);
}

namespace {

#define MARKER_SPHERE_SIZE 0x58000

static void DrawMarkers(fvcobjptr &vcobjptr, grs_canvas &canvas, automap &am)
{
	static int cyc=10,cycdir=1;

	const auto game_mode = Game_mode;
	const auto max_numplayers = Netgame.max_numplayers;
	const auto maxdrop = MarkerState.get_markers_per_player(game_mode, max_numplayers);
	const auto &&game_marker_range = get_game_marker_range(game_mode, max_numplayers, Player_num, maxdrop);
	const auto &&player_marker_range = get_player_marker_range(maxdrop);
	const auto &&zipped_marker_range = zip(game_marker_range, player_marker_range, unchecked_partial_range(&MarkerState.imobjidx[*game_marker_range.begin()], maxdrop));
	const auto &&mb = zipped_marker_range.begin();
	const auto &&me = zipped_marker_range.end();
	auto iter = mb;
	/* Find the first marker object in the player's marker range that is
	 * not object_none.  If every marker object in the range is
	 * object_none, then there are no markers to draw, so return.
	 */
	for (;;)
	{
		auto &&[gmi, pmi, objidx] = *iter;
		(void)gmi;
		(void)pmi;
		if (objidx != object_none)
			break;
		if (++ iter == me)
			return;
	}
	/* A marker was found, so at least one marker will be drawn.  Set up
	 * colors for the markers.
	 */
	const auto current_cycle_color = cyc;
	const std::array<color_t, 3> colors{{
		gr_find_closest_color_current(current_cycle_color, 0, 0),
		gr_find_closest_color_current(current_cycle_color + 10, 0, 0),
		gr_find_closest_color_current(current_cycle_color + 20, 0, 0),
	}};
	for (; iter != me; ++iter)
	{
		auto &&[gmi, pmi, objidx] = *iter;
		if (objidx != object_none)
		{
			/* Use cg3s_point so that the type is const for OpenGL and
			 * mutable for SDL-only.
			 */
			cg3s_point &&sphere_point = g3_rotate_point(vcobjptr(objidx)->pos);
			g3_draw_sphere(canvas, sphere_point, MARKER_SPHERE_SIZE, colors[0]);
			g3_draw_sphere(canvas, sphere_point, MARKER_SPHERE_SIZE / 2, colors[1]);
			g3_draw_sphere(canvas, sphere_point, MARKER_SPHERE_SIZE / 4, colors[2]);
			DrawMarkerNumber(canvas, am, gmi, pmi, sphere_point);
		}
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
	static_cast<d_marker_object_numbers &>(MarkerState) = {};
	MarkerState.message = {};
}
#endif

}

void automap_clear_visited(d_level_unique_automap_state &LevelUniqueAutomapState)
{
#if defined(DXX_BUILD_DESCENT_II)
	ClearMarkers();
#endif
	::dcx::automap_clear_visited(LevelUniqueAutomapState);
}

namespace {

static void draw_player(const g3_draw_line_context &context, const object_base &obj)
{
	// Draw Console player -- shaped like a ellipse with an arrow.
	auto sphere_point = g3_rotate_point(obj.pos);
	const auto obj_size = obj.size;
	g3_draw_sphere(context.canvas, sphere_point, obj_size, context.color);

	// Draw shaft of arrow
	const auto &&head_pos = vm_vec_scale_add(obj.pos, obj.orient.fvec, obj_size * 2);
	{
	auto &&arrow_point = g3_rotate_point(vm_vec_scale_add(obj.pos, obj.orient.fvec, obj_size * 3));
	g3_draw_line(context, sphere_point, arrow_point);

	// Draw right head of arrow
	{
		const auto &&rhead_pos = vm_vec_scale_add(head_pos, obj.orient.rvec, obj_size);
		auto head_point = g3_rotate_point(rhead_pos);
		g3_draw_line(context, arrow_point, head_point);
	}

	// Draw left head of arrow
	{
		const auto &&lhead_pos = vm_vec_scale_add(head_pos, obj.orient.rvec, -obj_size);
		auto head_point = g3_rotate_point(lhead_pos);
		g3_draw_line(context, arrow_point, head_point);
	}
	}

	// Draw player's up vector
	{
		const auto &&arrow_pos = vm_vec_scale_add(obj.pos, obj.orient.uvec, obj_size * 2);
	auto arrow_point = g3_rotate_point(arrow_pos);
		g3_draw_line(context, sphere_point, arrow_point);
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

static void name_frame(grs_canvas &canvas, automap &am, int dx = 0, int dy = 0)
{
	gr_set_fontcolor(canvas, am.green_31, -1);
	char		name_level_left[128];

	auto &game_font = *GAME_FONT;
#if defined(DXX_BUILD_DESCENT_I)
	const char *name_level;
	if (Current_level_num > 0)
	{
		snprintf(name_level_left, sizeof(name_level_left), "%s %i: %s",TXT_LEVEL, Current_level_num, static_cast<const char *>(Current_level_name));
		name_level = name_level_left;
	}
	else
		name_level = Current_level_name;

	gr_string(canvas, game_font, dx + (SWIDTH / 64), dy + (SHEIGHT / 48), name_level);
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

	gr_string(canvas, game_font, dx + (SWIDTH / 64), dy + (SHEIGHT / 48), name_level_left);
	const auto &&[wr, h] = gr_get_string_size(game_font, name_level_right);
	gr_string(canvas, game_font, dx + canvas.cv_bitmap.bm_w - wr - (SWIDTH / 64), dy + (SHEIGHT / 48), name_level_right, wr, h);
#endif
}

static void automap_apply_input(automap &am, const vms_matrix &plrorient, const vms_vector &plrpos)
{
	constexpr int SLIDE_SPEED = 350;
	constexpr int ZOOM_SPEED_FACTOR = 500;	//(1500)
	constexpr int ROT_SPEED_DIVISOR = 115000;
	if (PlayerCfg.AutomapFreeFlight)
	{
		if (am.controls.state.fire_primary)
		{
			// Reset orientation
			am.controls.state.fire_primary = 0;
			am.viewMatrix = plrorient;
			vm_vec_scale_add(am.view_position, plrpos, am.viewMatrix.fvec, -ZOOM_DEFAULT);
		}
		
		if (am.controls.pitch_time || am.controls.heading_time || am.controls.bank_time)
		{
			vms_angvec tangles;

			tangles.p = fixdiv(am.controls.pitch_time, ROT_SPEED_DIVISOR);
			tangles.h = fixdiv(am.controls.heading_time, ROT_SPEED_DIVISOR);
			tangles.b = fixdiv(am.controls.bank_time, ROT_SPEED_DIVISOR * 2);

			const auto &&tempm = vm_angles_2_matrix(tangles);
			am.viewMatrix = vm_matrix_x_matrix(am.viewMatrix, tempm);
			check_and_fix_matrix(am.viewMatrix);
		}
		
		if (am.controls.forward_thrust_time || am.controls.vertical_thrust_time || am.controls.sideways_thrust_time)
		{
			vm_vec_scale_add2(am.view_position, am.viewMatrix.fvec, am.controls.forward_thrust_time * ZOOM_SPEED_FACTOR);
			vm_vec_scale_add2(am.view_position, am.viewMatrix.uvec, am.controls.vertical_thrust_time * SLIDE_SPEED);
			vm_vec_scale_add2(am.view_position, am.viewMatrix.rvec, am.controls.sideways_thrust_time * SLIDE_SPEED);
			
			// Crude wrapping check
			clamp_fix_symmetric(am.view_position.x, F1_0*32000);
			clamp_fix_symmetric(am.view_position.y, F1_0*32000);
			clamp_fix_symmetric(am.view_position.z, F1_0*32000);
		}
	}
	else
	{
		if (am.controls.state.fire_primary)
		{
			// Reset orientation
			am.viewDist = ZOOM_DEFAULT;
			am.tangles.p = PITCH_DEFAULT;
			am.tangles.h  = 0;
			am.tangles.b  = 0;
			am.view_target = plrpos;
			am.controls.state.fire_primary = 0;
		}

		am.viewDist -= am.controls.forward_thrust_time * ZOOM_SPEED_FACTOR;
		am.tangles.p += fixdiv(am.controls.pitch_time, ROT_SPEED_DIVISOR);
		am.tangles.h += fixdiv(am.controls.heading_time, ROT_SPEED_DIVISOR);
		am.tangles.b += fixdiv(am.controls.bank_time, ROT_SPEED_DIVISOR * 2);

		if (am.controls.vertical_thrust_time || am.controls.sideways_thrust_time)
		{
			vms_angvec      tangles1;
			vms_vector      old_vt;

			old_vt = am.view_target;
			tangles1 = am.tangles;
			const auto &&tempm = vm_angles_2_matrix(tangles1);
			vm_matrix_x_matrix(am.viewMatrix, plrorient, tempm);
			vm_vec_scale_add2(am.view_target, am.viewMatrix.uvec, am.controls.vertical_thrust_time * SLIDE_SPEED);
			vm_vec_scale_add2(am.view_target, am.viewMatrix.rvec, am.controls.sideways_thrust_time * SLIDE_SPEED);
			if (vm_vec_dist_quick(am.view_target, plrpos) > i2f(1000))
				am.view_target = old_vt;
		}

		const auto &&tempm = vm_angles_2_matrix(am.tangles);
		vm_matrix_x_matrix(am.viewMatrix, plrorient, tempm);

		clamp_fix_lh(am.viewDist, ZOOM_MIN_VALUE, ZOOM_MAX_VALUE);
	}
}

static void draw_automap(fvcobjptr &vcobjptr, automap &am, fix eye = 0)
{
#if DXX_USE_STEREOSCOPIC_RENDER
	int sw = SWIDTH;
	int sh = SHEIGHT;
	int dx = 0, dy = 0, dw = sw, dh = sh;
	if (VR_stereo != StereoFormat::None) {
		gr_stereo_viewport_resize(VR_stereo, dw, dh);
		gr_stereo_viewport_offset(VR_stereo, dx, dy, eye);
	}
	#define SCREEN_SIZE_STEREO 	grd_curscreen->set_screen_width_height(dw, dh)
	#define SCREEN_SIZE_NORMAL 	grd_curscreen->set_screen_width_height(sw, sh)
#else
	#define SCREEN_SIZE_STEREO 	// no-op
	#define SCREEN_SIZE_NORMAL 	// no-op
	const int dx = 0, dy = 0;	// no x,y adjust
#endif

	if ( am.leave_mode==0 && am.controls.state.automap && (timer_query()-am.entry_time)>LEAVE_TIME)
		am.leave_mode = 1;

	{
		auto &canvas = am.w_canv;
		if (eye <= 0)
			show_fullscr(canvas, am.automap_background);
		gr_set_fontcolor(canvas, BM_XRGB(20, 20, 20), -1);
		SCREEN_SIZE_STEREO;
	{
		int x, y;
#if defined(DXX_BUILD_DESCENT_I)
	if (MacHog)
			x = 80 * (SWIDTH / 640.), y = 36 * (SHEIGHT / 480.);
	else
#endif
			x = SWIDTH / 8, y = SHEIGHT / 16;
		x += dx, y += dy;
		gr_string(canvas, *HUGE_FONT, x, y, TXT_AUTOMAP);
	}
	{
		int x;
		int y0, y1, y2;
#if defined(DXX_BUILD_DESCENT_I)
		const auto s1 = TXT_SLIDE_UPDOWN;
		const auto &s2 = "F9/F10 Changes viewing distance";
	if (!MacHog)
	{
			x = SWIDTH / 4.923;
			y0 = SHEIGHT / 1.126;
			y1 = SHEIGHT / 1.083;
			y2 = SHEIGHT / 1.043;
	}
	else
	{
		// for the Mac automap they're shown up the top, hence the different layout
			x = 265 * (SWIDTH / 640.);
			y0 = 27 * (SHEIGHT / 480.);
			y1 = 44 * (SHEIGHT / 480.);
			y2 = 61 * (SHEIGHT / 480.);
	}
#elif defined(DXX_BUILD_DESCENT_II)
		const auto &s1 = "F9/F10 Changes viewing distance";
		const auto s2 = TXT_AUTOMAP_MARKER;
		x = SWIDTH / 10.666;
		y0 = SHEIGHT / 1.126;
		y1 = SHEIGHT / 1.083;
		y2 = SHEIGHT / 1.043;
#endif
		x += dx, y0 += dy, y1 += dy, y2 += dy;
		auto &game_font = *GAME_FONT;
		gr_string(canvas, game_font, x, y0, TXT_TURN_SHIP);
		gr_string(canvas, game_font, x, y1, s1);
		gr_string(canvas, game_font, x, y2, s2);
	}

	}
	SCREEN_SIZE_NORMAL;
	auto &canvas = am.automap_view;

	if (eye == 0)
		gr_clear_canvas(canvas, BM_XRGB(0,0,0));

	g3_start_frame(canvas);
	render_start_frame();

#if DXX_USE_STEREOSCOPIC_RENDER
	// select stereo viewport/transform/buffer per left/right eye
	if (VR_stereo != StereoFormat::None)
		g3_stereo_frame(eye, VR_eye_offset);
#endif

	if (!PlayerCfg.AutomapFreeFlight)
		vm_vec_scale_add(am.view_position,am.view_target,am.viewMatrix.fvec,-am.viewDist);

	am.view_position.x += eye;	// stereo viewpoint offset
	g3_set_view_matrix(am.view_position,am.viewMatrix,am.zoom);

	draw_all_edges(am);

	// Draw player...
	const auto &self_ship_rgb = player_rgb[get_player_or_team_color(Player_num)];
	const auto closest_color = BM_XRGB(self_ship_rgb.r, self_ship_rgb.g, self_ship_rgb.b);
	draw_player(g3_draw_line_context{canvas, closest_color}, vcobjptr(get_local_player().objnum));

#if defined(DXX_BUILD_DESCENT_II)
	DrawMarkers(vcobjptr, canvas, am);
#endif
	
	// Draw player(s)...
	const unsigned show_all_players = (Game_mode & GM_MULTI_COOP) || Netgame.game_flag.show_on_map;
	if (show_all_players || (Game_mode & GM_TEAM))
	{
		const auto local_player_team = get_team(Player_num);
		const auto n_players = N_players;
		for (const auto iu : xrange(n_players))
		{
			const auto i = static_cast<playernum_t>(iu);
			if (i == Player_num)
				continue;
			if (show_all_players || local_player_team == get_team(i))
			{
				auto &plr = *vcplayerptr(i);
				auto &objp = *vcobjptr(plr.objnum);
				if (objp.type == OBJ_PLAYER)
				{
					const auto &other_ship_rgb = player_rgb[get_player_or_team_color(i)];
					draw_player(g3_draw_line_context{canvas, BM_XRGB(other_ship_rgb.r, other_ship_rgb.g, other_ship_rgb.b)}, objp);
				}
			}
		}
	}

	range_for (const auto &&objp, vcobjptr)
	{
		switch( objp->type )	{
		case OBJ_HOSTAGE:
			{
			auto sphere_point = g3_rotate_point(objp->pos);
			g3_draw_sphere(canvas, sphere_point, objp->size, am.hostage_color);
			}
			break;
		case OBJ_POWERUP:
			if (LevelUniqueAutomapState.Automap_visited[objp->segnum]
#ifndef NDEBUG
				|| Automap_debug_show_all_segments
#endif
				)
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
				g3_draw_sphere(canvas, sphere_point, objp->size * 4, color);
				}
			}
			break;
			default:
				break;
		}
	}

	g3_end_frame();

	SCREEN_SIZE_STEREO;
	name_frame(canvas, am, dx, dy);
	SCREEN_SIZE_NORMAL;

#if defined(DXX_BUILD_DESCENT_II)
	{
		const auto HighlightMarker = MarkerState.HighlightMarker;
		if (MarkerState.message.valid_index(HighlightMarker))
		{
			auto &m = MarkerState.message[HighlightMarker];
			auto &p = m[0u];
			gr_printf(canvas, *canvas.cv_font, (SWIDTH / 64), (SHEIGHT / 18), "Marker %u%c %s", static_cast<unsigned>(HighlightMarker) + 1, p ? ':' : 0, &p);
		}
	}
#endif

	if (PlayerCfg.MouseFlightSim && PlayerCfg.MouseFSIndicator)
	{
		const auto gwidth = canvas.cv_bitmap.bm_w;
		const auto gheight = canvas.cv_bitmap.bm_h;
		auto &raw_mouse_axis = am.controls.raw_mouse_axis;
		show_mousefs_indicator(canvas, raw_mouse_axis[0], raw_mouse_axis[1], raw_mouse_axis[2], gwidth - (gheight / 8), gheight - (gheight / 8), gheight / 5);
	}

	am.t2 = timer_query();
	const auto vsync = CGameCfg.VSync;
	const auto bound = F1_0 / (vsync ? MAXIMUM_FPS : CGameArg.SysMaxFPS);
	const auto may_sleep = !CGameArg.SysNoNiceFPS && !vsync;
	while (am.t2 - am.t1 < bound) // ogl is fast enough that the automap can read the input too fast and you start to turn really slow.  So delay a bit (and free up some cpu :)
	{
		if (Game_mode & GM_MULTI)
			multi_do_frame(); // during long wait, keep packets flowing
		if (may_sleep)
			timer_delay(F1_0>>8);
		am.t2 = timer_update();
	}
	if (am.pause_game)
	{
		FrameTime=am.t2-am.t1;
		calc_d_tick();
	}
	am.t1 = am.t2;
}

#if defined(DXX_BUILD_DESCENT_I)
#define MAP_BACKGROUND_FILENAME (((SWIDTH>=640&&SHEIGHT>=480) && PHYSFSX_exists("maph.pcx",1))?"maph.pcx":"map.pcx")
#elif defined(DXX_BUILD_DESCENT_II)
#define MAP_BACKGROUND_FILENAME ((HIRESMODE && PHYSFSX_exists("mapb.pcx",1))?"mapb.pcx":"map.pcx")
#endif

static void recompute_automap_segment_visibility(const d_level_unique_automap_state &LevelUniqueAutomapState, const object &plrobj, automap &am)
{
	recompute_automap_segment_visibility(LevelUniqueAutomapState, plrobj.ctype.player_info.powerup_flags & PLAYER_FLAGS_MAP_ALL, plrobj.segnum, am);
}

static window_event_result automap_key_command(const d_event &event, automap &am)
{
#if defined(DXX_BUILD_DESCENT_I) || !defined(NDEBUG)
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
#endif
	int c = event_key_get(event);

	switch (c)
	{
#if DXX_USE_SCREENSHOT
		case KEY_PRINT_SCREEN: {
			gr_set_default_canvas();
			save_screen_shot(1);
			return window_event_result::handled;
		}
#endif
		case KEY_ESC:
			if (am.leave_mode == 0)
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
				auto &plrobj = get_local_plrobj();
				recompute_automap_segment_visibility(LevelUniqueAutomapState, plrobj, am);
			}
			return window_event_result::handled;
#endif
#ifndef NDEBUG
		case KEY_DEBUGGED+KEY_F: 	{
				Automap_debug_show_all_segments = !Automap_debug_show_all_segments;
				auto &plrobj = get_local_plrobj();
				recompute_automap_segment_visibility(LevelUniqueAutomapState, plrobj, am);
			}
			return window_event_result::handled;
#endif
		case KEY_F9:
			if (am.segment_limit > 1)
			{
				am.segment_limit--;
				adjust_segment_limit(am, am.segment_limit);
			}
			return window_event_result::handled;
		case KEY_F10:
			if (am.segment_limit < am.max_segments_away) 	{
				am.segment_limit++;
				adjust_segment_limit(am, am.segment_limit);
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
			{
				const auto game_mode = Game_mode;
				const auto max_numplayers = Netgame.max_numplayers;
				const auto maxdrop = MarkerState.get_markers_per_player(game_mode, max_numplayers);
				const uint8_t marker_num = c - KEY_1;
				if (marker_num <= maxdrop)
				{
					const auto gmi = convert_player_marker_index_to_game_marker_index(game_mode, max_numplayers, Player_num, player_marker_index{marker_num});
					if (MarkerState.imobjidx[gmi] != object_none)
						MarkerState.HighlightMarker = gmi;
				}
				else
					MarkerState.HighlightMarker = game_marker_index::None;
			}
			return window_event_result::handled;
		case KEY_D+KEY_CTRLED:
			{
				if (const auto [mo, HighlightMarker] = marker_delete_are_you_sure_menu::get_marker_object(MarkerState); mo == nullptr)
				{
					(void)HighlightMarker;
					/* If the selected index is not a valid marker, do
					 * not offer to delete anything.
					 */
					return window_event_result::handled;
				}
				auto menu = window_create<marker_delete_are_you_sure_menu>(grd_curscreen->sc_canvas, LevelUniqueObjectState, Segments, MarkerState);
				(void)menu;
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

static window_event_result automap_process_input(const d_event &event, automap &am)
{
	kconfig_read_controls(am.controls, event, 1);

	if (!am.controls.state.automap && am.leave_mode == 1)
	{
		return window_event_result::close;
	}
	if (am.controls.state.automap)
	{
		am.controls.state.automap = 0;
		if (am.leave_mode == 0)
		{
			return window_event_result::close;
		}
	}
	return window_event_result::ignored;
}

}

window_event_result automap::event_handler(const d_event &event)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	auto &vmobjptr = Objects.vmptr;
	switch (event.type)
	{
		case EVENT_WINDOW_ACTIVATED:
			game_flush_inputs(controls);
			event_toggle_focus(1);
			key_toggle_repeat(0);
			break;

		case EVENT_WINDOW_DEACTIVATED:
			event_toggle_focus(0);
			key_toggle_repeat(1);
			break;

#if SDL_MAJOR_VERSION == 2
		case EVENT_WINDOW_RESIZE:
			init_automap_subcanvas(automap_view, grd_curscreen->sc_canvas);
			break;
#endif

		case EVENT_IDLE:
#if DXX_MAX_BUTTONS_PER_JOYSTICK
		case EVENT_JOYSTICK_BUTTON_UP:
		case EVENT_JOYSTICK_BUTTON_DOWN:
#endif
#if DXX_MAX_AXES_PER_JOYSTICK
		case EVENT_JOYSTICK_MOVED:
#endif
		case EVENT_MOUSE_BUTTON_UP:
		case EVENT_MOUSE_BUTTON_DOWN:
		case EVENT_MOUSE_MOVED:
		case EVENT_KEY_RELEASE:
			return automap_process_input(event, *this);
		case EVENT_KEY_COMMAND:
		{
			window_event_result kret = automap_key_command(event, *this);
			if (kret == window_event_result::ignored)
				kret = automap_process_input(event, *this);
			return kret;
		}
			
		case EVENT_WINDOW_DRAW:
			{
				auto &plrobj = get_local_plrobj();
				automap_apply_input(*this, plrobj.orient, plrobj.pos);
			}
#if DXX_USE_STEREOSCOPIC_RENDER
			if (VR_stereo != StereoFormat::None) {
				draw_automap(vcobjptr, *this, -VR_eye_width);
				draw_automap(vcobjptr, *this,  VR_eye_width);
			}
			else
#endif
			draw_automap(vcobjptr, *this);
			break;
			
		case EVENT_WINDOW_CLOSE:
			if (!pause_game)
				ConsoleObject->mtype.phys_info.flags |= old_wiggle;		// Restore wiggle
			event_toggle_focus(0);
			key_toggle_repeat(1);
			/* grd_curcanv points to `automap_view`, so grd_curcanv
			 * would become a dangling pointer after the call to delete.
			 * Redirect it to the default screen to avoid pointing to
			 * freed memory.  Setting grd_curcanv to nullptr would be
			 * better, but some code assumes that grd_curcanv is never
			 * nullptr, so instead set it to the default canvas.
			 * Eventually, grd_curcanv will be removed entirely.
			 */
			gr_set_default_canvas();
			Game_wind->set_visible(1);
			Automap_active = 0;
			multi_send_msgsend_state(msgsend_state::none);
			return window_event_result::ignored;	// continue closing

		case EVENT_LOOP_BEGIN_LOOP:
			kconfig_begin_loop(controls);
			break;
		case EVENT_LOOP_END_LOOP:
			kconfig_end_loop(controls, FrameTime);
			break;

		default:
			return window_event_result::ignored;
			break;
	}
	return window_event_result::handled;
}

void do_automap()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	palette_array_t pal;
	auto am = window_create<automap>(grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT);
	const auto max_edges = LevelSharedSegmentState.Num_segments * 12;
	am->max_edges = max_edges;
	am->edges = std::make_unique<Edge_info[]>(max_edges);
	am->drawingListBright = std::make_unique<Edge_info *[]>(max_edges);

	init_automap_colors(*am);
	am->pause_game = !((Game_mode & GM_MULTI) && (!Endlevel_sequence)); // Set to 1 if everything is paused during automap...No pause during net.

	if (am->pause_game) {
		Game_wind->set_visible(0);
	}
	else
	{
		am->old_wiggle = ConsoleObject->mtype.phys_info.flags & PF_WIGGLE;	// Save old wiggle
		ConsoleObject->mtype.phys_info.flags &= ~PF_WIGGLE;		// Turn off wiggle
	}

	//Max_edges = min(MAX_EDGES_FROM_VERTS(Num_vertices),MAX_EDGES); //make maybe smaller than max

	gr_set_default_canvas();

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
	recompute_automap_segment_visibility(LevelUniqueAutomapState, plrobj, *am);

	// ZICO - code from above to show frame in OGL correctly. Redundant, but better readable.
	// KREATOR - Now applies to all platforms so double buffering is supported
	{
		const auto pcx_error = pcx_read_bitmap_or_default(MAP_BACKGROUND_FILENAME, am->automap_background, pal);
		if (pcx_error != pcx_result::SUCCESS)
			con_printf(CON_URGENT, DXX_STRINGIZE_FL(__FILE__, __LINE__, "automap: File %s - PCX error: %s"), MAP_BACKGROUND_FILENAME, pcx_errormsg(pcx_error));
		gr_remap_bitmap_good(am->automap_background, pal, -1, -1);
	}
	init_automap_subcanvas(am->automap_view, grd_curscreen->sc_canvas);

	gr_palette_load( gr_palette );
	Automap_active = 1;
	multi_send_msgsend_state(msgsend_state::automap);
}

namespace {

void draw_all_edges(automap &am)
{
	auto &canvas = am.automap_view;
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	int j;
	unsigned nbright = 0;
	ubyte nfacing,nnfacing;
	fix distance;
	fix min_distance = INT32_MAX;

	auto &vcvertptr = Vertices.vcptr;
	range_for (auto &i, unchecked_partial_range(am.edges.get(), am.end_valid_edges))
	{
		const auto e = &i;
		if (!(e->flags & EF_USED)) continue;

		if ( e->flags & EF_TOO_FAR) continue;

		if (e->flags&EF_FRONTIER) { 	// A line that is between what we have seen and what we haven't
			if ((!(e->flags & EF_SECRET)) && (e->color == am.wall_normal_color))
				continue; 	// If a line isn't secret and is normal color, then don't draw it
		}
		distance = Segment_points[e->verts[1]].p3_z;

		if (min_distance>distance )
			min_distance = distance;

		if (rotate_list(vcvertptr, e->verts).uand == clipping_code::None)
		{			//all off screen?
			nfacing = nnfacing = 0;
			auto &tv1 = *vcvertptr(e->verts[0]);
			j = 0;
			while( j<e->num_faces && (nfacing==0 || nnfacing==0) )	{
				if (!g3_check_normal_facing(tv1, vcsegptr(e->segnum[j])->shared_segment::sides[e->sides[j]].normals[0]))
					nfacing++;
				else
					nnfacing++;
				j++;
			}

			if ( nfacing && nnfacing )	{
				// a contour line
				am.drawingListBright[nbright++] = e;
			} else if ( e->flags&(EF_DEFINING|EF_GRATE) )	{
				if ( nfacing == 0 )	{
					const uint8_t color = (e->flags & EF_NO_FADE)
						? e->color
						: gr_fade_table[(gr_fade_level{8})][e->color];
					g3_draw_line(g3_draw_line_context{canvas, color}, Segment_points[e->verts[0]], Segment_points[e->verts[1]]);
				} 	else {
					am.drawingListBright[nbright++] = e;
				}
			}
		}
	}
		
	if ( min_distance < 0 ) min_distance = 0;

	// Sort the bright ones using a shell sort
	const auto &&range = unchecked_partial_range(am.drawingListBright.get(), nbright);
	std::sort(range.begin(), range.end(), [](const Edge_info *const a, const Edge_info *const b) {
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
		if ( dist >= am.farthest_dist ) continue;

		const auto color = (e->flags & EF_NO_FADE)
			? e->color
			: gr_fade_table[static_cast<gr_fade_level>(f2i((F1_0 - fixdiv(dist, am.farthest_dist)) * 31))][e->color];	
		g3_draw_line(g3_draw_line_context{canvas, color}, *p1, *p2);
	}
}

//==================================================================
//
// All routines below here are used to build the Edge list
//
//==================================================================

//finds edge, filling in edge_ptr. if found old edge, returns index, else return -1
static std::pair<Edge_info &, std::size_t> automap_find_edge(automap &am, const vertnum_t v0, const vertnum_t v1)
{
	const auto &&hash_object = std::hash<vertnum_t>{};
	const auto initial_hash_slot = (hash_object(v0) ^ (hash_object(v1) << 10)) % am.max_edges;

	for (auto current_hash_slot = initial_hash_slot;;)
	{
		auto &e = am.edges[current_hash_slot];
		const auto ev0 = e.verts[0];
		const auto ev1 = e.verts[1];
		if (e.num_faces == 0)
			return {e, current_hash_slot};
		if (v1 == ev1 && v0 == ev0)
			return {e, UINT32_MAX};
		else {
			if (++ current_hash_slot == am.max_edges)
				current_hash_slot = 0;
			if (current_hash_slot == initial_hash_slot)
				throw std::runtime_error("edge list full: search wrapped without finding a free slot");
		}
	}
}

static void add_one_edge(automap &am, vertnum_t va, vertnum_t vb, const uint8_t color, const sidenum_t side, const segnum_t segnum, const uint8_t flags)
{
	if (am.num_edges >= am.max_edges)
	{
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
	const auto &&ef = automap_find_edge(am, va, vb);
	const auto e = &ef.first;
		
	if (ef.second != UINT32_MAX)
	{
		e->verts[0] = va;
		e->verts[1] = vb;
		e->color = color;
		e->num_faces = 1;
		e->flags = EF_USED | EF_DEFINING;			// Assume a normal line
		e->sides[0] = side;
		e->segnum[0] = segnum;
		++ am.num_edges;
		const auto i = ef.second + 1;
		if (am.end_valid_edges < i)
			am.end_valid_edges = i;
	} else {
		if ( color != am.wall_normal_color )
#if defined(DXX_BUILD_DESCENT_II)
			if (color != am.wall_revealed_color)
#endif
				e->color = color;

		if ( e->num_faces < 4 ) {
			e->sides[e->num_faces] = side;
			e->segnum[e->num_faces] = segnum;
			e->num_faces++;
		}
	}
	e->flags |= flags;
}

static void add_one_unknown_edge(automap &am, vertnum_t va, vertnum_t vb)
{
	if ( va > vb )	{
		std::swap(va, vb);
	}

	const auto &&ef = automap_find_edge(am, va, vb);
	if (ef.second == UINT32_MAX)
		ef.first.flags |= EF_FRONTIER;		// Mark as a border edge
}

static void add_segment_edges(fvcsegptr &vcsegptr, fvcwallptr &vcwallptr, automap &am, const vcsegptridx_t seg)
{
	auto &ControlCenterState = LevelUniqueObjectState.ControlCenterState;
	auto &WallAnims = GameSharedState.WallAnims;
#if defined(DXX_BUILD_DESCENT_II)
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
#endif
	for (const auto sn : MAX_SIDES_PER_SEGMENT)
	{
		uint8_t hidden_flag = 0;
		uint8_t is_grate = 0;
		uint8_t no_fade = 0;

		uint8_t color = 255;
		if (seg->shared_segment::children[sn] == segment_none) {
			color = am.wall_normal_color;
		}

		switch( seg->special )	{
			case segment_special::nothing:
			case segment_special::goal_blue:
			case segment_special::goal_red:
				break;
			case segment_special::repaircen:
				color = BM_XRGB(0, 0, 26);
				break;
			case segment_special::fuelcen:
			color = BM_XRGB( 29, 27, 13 );
			break;
			case segment_special::controlcen:
			if (ControlCenterState.Control_center_present)
				color = BM_XRGB( 29, 0, 0 );
			break;
			case segment_special::robotmaker:
			color = BM_XRGB( 29, 0, 31 );
			break;
		}

		const auto wall_num = seg->shared_segment::sides[sn].wall_num;
		if (wall_num != wall_none)
		{
			auto &w = *vcwallptr(wall_num);
#if defined(DXX_BUILD_DESCENT_II)
			auto trigger_num = w.trigger;
			auto &Triggers = LevelUniqueWallSubsystemState.Triggers;
			auto &vmtrgptr = Triggers.vmptr;
			if (trigger_num != trigger_none && vmtrgptr(trigger_num)->type == trigger_action::secret_exit)
				{
			    color = BM_XRGB( 29, 0, 31 );
				no_fade = EF_NO_FADE;
				 goto Here;
				} 	
#endif

			switch(w.type)
			{
			case WALL_DOOR:
				if ((w.keys == wall_key::blue && (color = am.wall_door_blue, true)) ||
					(w.keys == wall_key::gold && (color = am.wall_door_gold, true)) ||
					(w.keys == wall_key::red && (color = am.wall_door_red, true)))
				{
					no_fade = EF_NO_FADE;
				} else if (!(WallAnims[w.clip_num].flags & WCF_HIDDEN)) {
					const auto connected_seg = seg->shared_segment::children[sn];
					if (connected_seg != segment_none) {
						const shared_segment &vcseg = *vcsegptr(connected_seg);
						const auto &connected_side = find_connect_side(seg, vcseg);
						if (connected_side == side_none)
							break;
						auto &wall = *vcwallptr(vcseg.sides[connected_side].wall_num);
						switch (wall.keys)
						{
							case wall_key::blue:
								color = am.wall_door_blue;
								no_fade = EF_NO_FADE;
								break;
							case wall_key::gold:
								color = am.wall_door_gold;
								no_fade = EF_NO_FADE;
								break;
							case wall_key::red:
								color = am.wall_door_red;
								no_fade = EF_NO_FADE;
								break;
							default:
								color = am.wall_door_color;
								break;
						}
					}
				} else {
					color = am.wall_normal_color;
					hidden_flag = EF_SECRET;
				}
				break;
			case WALL_CLOSED:
				// Make grates draw properly
				// NOTE: In original D1, is_grate is 1, hidden_flag not used so grates never fade. I (zico) like this so I leave this alone for now. 
				if (!(is_grate = WALL_IS_DOORWAY(GameBitmaps, Textures, vcwallptr, seg, sn) & WALL_IS_DOORWAY_FLAG::rendpast))
					hidden_flag = EF_SECRET;
				color = am.wall_normal_color;
				break;
			case WALL_BLASTABLE:
				// Hostage doors
				color = am.wall_door_color;	
				break;
			}
		}
	
		if (seg==Player_init[Player_num].segnum)
			color = BM_XRGB(31,0,31);

		if ( color != 255 )	{
#if defined(DXX_BUILD_DESCENT_II) 
			// If they have a map powerup, draw unvisited areas in dark blue.
			// NOTE: D1 originally had this part of code but w/o cheat-check. It's only supposed to draw blue with powerup that does not exist in D1. So make this D2-only
#ifndef NDEBUG
			if (!Automap_debug_show_all_segments)
#endif
			{
			auto &player_info = get_local_plrobj().ctype.player_info;
				if ((cheats.fullautomap || player_info.powerup_flags & PLAYER_FLAGS_MAP_ALL) && !LevelUniqueAutomapState.Automap_visited[seg])
				color = am.wall_revealed_color;
			}
			Here:
#endif
			const auto vertex_list = get_side_verts(seg,sn);
			const uint8_t flags = hidden_flag | no_fade;
			add_one_edge(am, vertex_list[0], vertex_list[1], color, sn, seg, flags);
			add_one_edge(am, vertex_list[1], vertex_list[2], color, sn, seg, flags);
			add_one_edge(am, vertex_list[2], vertex_list[3], color, sn, seg, flags);
			add_one_edge(am, vertex_list[3], vertex_list[0], color, sn, seg, flags);

			if ( is_grate )	{
				const uint8_t grate_flags = flags | EF_GRATE;
				add_one_edge(am, vertex_list[0], vertex_list[2], color, sn, seg, grate_flags);
				add_one_edge(am, vertex_list[1], vertex_list[3], color, sn, seg, grate_flags);
			}
		}
	}
}


// Adds all the edges from a segment we haven't visited yet.

static void add_unknown_segment_edges(automap &am, const shared_segment &seg)
{
	for (const auto &&[sn, child] : enumerate(seg.children))
	{
		// Only add edges that have no children
		if (child == segment_none)
		{
			const auto vertex_list = get_side_verts(seg, static_cast<sidenum_t>(sn));
	
			add_one_unknown_edge( am, vertex_list[0], vertex_list[1] );
			add_one_unknown_edge( am, vertex_list[1], vertex_list[2] );
			add_one_unknown_edge( am, vertex_list[2], vertex_list[3] );
			add_one_unknown_edge( am, vertex_list[3], vertex_list[0] );
		}
	}
}

void automap_build_edge_list(automap &am, int add_all_edges)
{
	// clear edge list
	for (auto &i : unchecked_partial_range(am.edges.get(), am.max_edges))
	{
		i.num_faces = 0;
		i.flags = 0;
	}
	am.num_edges = 0;
	am.end_valid_edges = 0;

	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcwallptr = Walls.vcptr;
	if (add_all_edges)	{
		// Cheating, add all edges as visited
		range_for (const auto &&segp, vcsegptridx)
		{
#if DXX_USE_EDITOR
			if (segp->shared_segment::segnum != segment_none)
#endif
			{
				add_segment_edges(vcsegptr, vcwallptr, am, segp);
			}
		}
	} else {
		// Not cheating, add visited edges, and then unvisited edges
		range_for (const auto &&segp, vcsegptridx)
		{
#if DXX_USE_EDITOR
			if (segp->shared_segment::segnum != segment_none)
#endif
				if (LevelUniqueAutomapState.Automap_visited[segp])
				{
					add_segment_edges(vcsegptr, vcwallptr, am, segp);
				}
		}
		range_for (const auto &&segp, vcsegptridx)
		{
#if DXX_USE_EDITOR
			if (segp->shared_segment::segnum != segment_none)
#endif
				if (!LevelUniqueAutomapState.Automap_visited[segp])
				{
					add_unknown_segment_edges(am, segp);
				}
		}
	}

	// Find unnecessary lines (These are lines that don't have to be drawn because they have small curvature)
	for (auto &i : unchecked_partial_range(am.edges.get(), am.end_valid_edges))
	{
		const auto e = &i;
		if (!(e->flags&EF_USED)) continue;

		const auto num_faces = e->num_faces;
		if (num_faces < 2)
			continue;
		for (unsigned e1 = 0; e1 < num_faces; ++e1)
		{
			const auto e1segnum = e->segnum[e1];
			const auto &e1siden0 = vcsegptr(e1segnum)->shared_segment::sides[e->sides[e1]].normals[0];
			for (unsigned e2 = 1; e2 < num_faces; ++e2)
			{
				if (e1 == e2)
					continue;
				const auto e2segnum = e->segnum[e2];
				if (e1segnum == e2segnum)
					continue;
				if (vm_vec_dot(e1siden0, vcsegptr(e2segnum)->shared_segment::sides[e->sides[e2]].normals[0]) > (F1_0 - (F1_0 / 10)))
				{
					e->flags &= (~EF_DEFINING);
					break;
				}
			}
			if (!(e->flags & EF_DEFINING))
				break;
		}
	}
}

}

#if defined(DXX_BUILD_DESCENT_II)
void InitMarkerInput ()
{
	//find free marker slot
	const auto game_mode = Game_mode;
	const auto max_numplayers = Netgame.max_numplayers;
	const auto maxdrop = MarkerState.get_markers_per_player(game_mode, max_numplayers);
	const auto &&game_marker_range = get_game_marker_range(game_mode, max_numplayers, Player_num, maxdrop);
	const auto &&player_marker_range = get_player_marker_range(maxdrop);
	const auto &&zipped_marker_range = zip(player_marker_range, unchecked_partial_range(&MarkerState.imobjidx[*game_marker_range.begin()], maxdrop));
	const auto &&mb = zipped_marker_range.begin();
	const auto &&me = zipped_marker_range.end();
	auto iter = mb;
	for (;;)
	{
		auto &&[pmi, objidx] = *iter;
		if (objidx == object_none)		//found free slot!
		{
			MarkerState.MarkerBeingDefined = pmi;
			break;
		}
		if (++ iter == me)		//no free slot
		{
			if (game_mode & GM_MULTI)
			{
				//in multi, replace oldest
				MarkerState.MarkerBeingDefined = static_cast<player_marker_index>((static_cast<unsigned>(MarkerState.LastMarkerDropped) + 1) & (maxdrop - 1));
				break;
			}
			else
			{
				HUD_init_message_literal(HM_DEFAULT, "No free marker slots");
				return;
			}
		}
	}

	//got a free slot.  start inputting marker message

	Marker_input.front() = 0;
	Marker_index=0;
	key_toggle_repeat(1);
}

window_event_result MarkerInputMessage(int key, control_info &Controls)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	auto &vmobjptridx = Objects.vmptridx;
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
			{
				const auto player_marker_num = MarkerState.MarkerBeingDefined;
				MarkerState.LastMarkerDropped = player_marker_num;
				const auto game_marker_num = convert_player_marker_index_to_game_marker_index(Game_mode, Netgame.max_numplayers, Player_num, player_marker_num);
				MarkerState.message[game_marker_num] = Marker_input;
				DropMarker(vmobjptridx, vmsegptridx, get_local_plrobj(), game_marker_num, player_marker_num);
			}
			[[fallthrough]];
		case KEY_F8:
		case KEY_ESC:
			MarkerState.MarkerBeingDefined = player_marker_index::None;
			key_toggle_repeat(0);
			game_flush_inputs(Controls);
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
