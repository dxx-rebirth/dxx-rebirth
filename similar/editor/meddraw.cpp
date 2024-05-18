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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Med drawing functions.
 *
 */

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "inferno.h"
#include "segment.h"
#include "segpoint.h"
#include "gameseg.h"
#include "gr.h"
#include "ui.h"
#include "editor/editor.h"
#include "editor/esegment.h"
#include "wall.h"
#include "switch.h"
#include "dxxerror.h"
#include "u_mem.h"
#include "render.h"
#include "func.h"
#include "textures.h"
#include "object.h"
#include "meddraw.h"
#include "d_enumerate.h"
#include "d_levelstate.h"
#include "d_range.h"
#include "compiler-range_for.h"
#include "partial_range.h"
#include "segiter.h"
#include "d_zip.h"

#if DXX_USE_OGL
#include "ogl_init.h"
#endif

using std::min;

//	Colors used in editor for indicating various kinds of segments.
#define	SELECT_COLOR		BM_XRGB( 63/2 , 41/2 ,  0/2)
#define	FOUND_COLOR			BM_XRGB(  0/2 , 30/2 , 45/2)
#define	WARNING_COLOR		BM_XRGB( 63/2 ,  0/2 ,  0/2)
#define	AXIS_COLOR			BM_XRGB( 63/2 ,  0/2 , 63/2)
#define	PLAINSEG_COLOR		BM_XRGB( 45/2 , 45/2 , 45/2)
#define	MARKEDSEG_COLOR	BM_XRGB(  0/2 , 63/2 ,  0/2)
#define	MARKEDSIDE_COLOR	BM_XRGB(  0/2 , 63/2 , 63/2)
#define	CURSEG_COLOR		BM_XRGB( 63/2 , 63/2 , 63/2)
#define	CURSIDE_COLOR		BM_XRGB( 63/2 , 63/2 ,  0/2)
#define	CUREDGE_COLOR		BM_XRGB(  0   , 63/2 ,  0  )
#define	GROUPSEG_COLOR		BM_XRGB(	 0/2 ,  0/2 , 63/2)
#define  GROUPSIDE_COLOR 	BM_XRGB(	63/2 ,  0/2 , 45/2)
#define 	GROUP_COLOR			BM_XRGB(  0/2 , 45/2 ,  0/2)	
#define	ROBOT_COLOR			BM_XRGB( 31   ,  0   ,  0  )
#define	PLAYER_COLOR		BM_XRGB(  0   ,  0   , 31  )

constexpr std::integral_constant<unsigned, MAX_VERTICES * 4> MAX_EDGES{};

namespace {

enum class packed_edge : uint8_t
{
};

static int     Search_mode=0;                      //if true, searching for segments at given x,y
static int Search_x,Search_y;
static int	Automap_test=0;		//	Set to 1 to show wireframe in automap mode.

static void draw_seg_objects(grs_canvas &canvas, const unique_segment &seg)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptridx = Objects.vcptridx;
	range_for (const auto obj, objects_in(seg, vcobjptridx, vcsegptr))
	{
		const uint8_t color = (obj->type == OBJ_PLAYER && static_cast<icobjptridx_t::index_type>(obj) > 0)
			? BM_XRGB(0,  25, 0)
			: (obj == ConsoleObject
				? PLAYER_COLOR
				: ROBOT_COLOR
			);
		g3_draw_sphere(canvas, /* sphere_point = */ g3_rotate_point(obj->pos), obj->size, color);
	}
}

static void draw_line(const g3_draw_line_context &context, const vertnum_t pnum0, const vertnum_t pnum1)
{
	g3_draw_line(context, Segment_points[pnum0], Segment_points[pnum1]);
}

// ----------------------------------------------------------------------------
static void draw_segment(const g3_draw_line_context &context, const shared_segment &seg)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	if (seg.segnum == segment_none)		//this segment doesn't exitst
		return;

	auto &svp = seg.verts;
	auto &vcvertptr = Vertices.vcptr;
	if (rotate_list(vcvertptr, svp).uand == clipping_code::None)
	{		//all off screen?
		for (const uint8_t i : xrange(4u))
			draw_line(context, svp[(segment_relative_vertnum{i})], svp[(segment_relative_vertnum{static_cast<uint8_t>(i + 4)})]);

		for (const uint8_t i : xrange(3u))
		{
			draw_line(context, svp[(segment_relative_vertnum{i})], svp[(segment_relative_vertnum{static_cast<uint8_t>(i + 1)})]);
			draw_line(context, svp[(segment_relative_vertnum{static_cast<uint8_t>(i + 4)})], svp[(segment_relative_vertnum{static_cast<uint8_t>(i + 4 + 1)})]);
		}

		draw_line(context, svp[segment_relative_vertnum::_0], svp[segment_relative_vertnum::_3]);
		draw_line(context, svp[segment_relative_vertnum::_4], svp[segment_relative_vertnum::_7]);
	}
}

//for looking for segment under a mouse click
static void check_segment(const vmsegptridx_t seg)
{
	auto &svp = seg->verts;
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vcvertptr = Vertices.vcptr;
	if (rotate_list(vcvertptr, svp).uand == clipping_code::None)
	{		//all off screen?
#if DXX_USE_OGL
		g3_end_frame();
#endif
		{
		uint8_t color = 0;
		gr_pixel(grd_curcanv->cv_bitmap, Search_x, Search_y, color);	//set our search pixel to color zero
		}
#if DXX_USE_OGL
		g3_start_frame(*grd_curcanv);
#endif
		{
		const uint8_t color = 1;
		//and render in color one

		range_for (auto &fn, Side_to_verts)
		{
			std::array<cg3s_point *, 3> vert_list;
			vert_list[0] = &Segment_points[seg->verts[fn[side_relative_vertnum::_0]]];
			vert_list[1] = &Segment_points[seg->verts[fn[side_relative_vertnum::_1]]];
			vert_list[2] = &Segment_points[seg->verts[fn[side_relative_vertnum::_2]]];
			g3_check_and_draw_poly(*grd_curcanv, vert_list, color);

			vert_list[1] = &Segment_points[seg->verts[fn[side_relative_vertnum::_2]]];
			vert_list[2] = &Segment_points[seg->verts[fn[side_relative_vertnum::_3]]];
			g3_check_and_draw_poly(*grd_curcanv, vert_list, color);
		}
		}

		if (gr_ugpixel(grd_curcanv->cv_bitmap,Search_x,Search_y) == 1)
                 {
					 Found_segs.emplace_back(seg);
                 }
	}
}

// ----------------------------------------------------------------------------
static void draw_seg_side(const shared_segment &seg, const sidenum_t side, const color_palette_index color)
{
	auto &svp = seg.verts;
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vcvertptr = Vertices.vcptr;
	if (rotate_list(vcvertptr, svp).uand == clipping_code::None)
	{		//all off screen?
		auto &stv = Side_to_verts[side];
		g3_draw_line_context context{*grd_curcanv, color};
		for (const auto i : MAX_VERTICES_PER_SIDE)
			draw_line(context, svp[stv[i]], svp[stv[next_side_vertex(i)]]);
	}
}

static void draw_side_edge(const shared_segment &seg, const sidenum_t side, const side_relative_vertnum edge, const color_palette_index color)
{
	auto &svp = seg.verts;
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vcvertptr = Vertices.vcptr;
	if (rotate_list(vcvertptr, svp).uand == clipping_code::None)		//on screen?
	{
		auto &stv = Side_to_verts[side];
		draw_line(g3_draw_line_context{*grd_curcanv, color}, svp[stv[edge]], svp[stv[next_side_vertex(edge)]]);
	}
}

__attribute__((used))
int Show_triangulations=0;

//edge types - lower number types have precedence
#define ET_FACING		0	//this edge on a facing face
#define ET_NOTFACING	1	//this edge on a non-facing face
#define ET_NOTUSED	2	//no face uses this edge
#define ET_NOTEXTANT	3	//would exist if side were triangulated 

#define ET_EMPTY		255	//this entry in array is empty

static
#if defined(DXX_BUILD_DESCENT_I)
constexpr
#endif
std::array<color_palette_index, 3> edge_colors{{54, 59, 64}};

struct seg_edge
{
	vertnum_t v0, v1;
	ushort	type;
	ubyte		face_count, backface_count;
};

static std::array<seg_edge, MAX_EDGES> edge_list;
static std::array<int, MAX_EDGES> used_list;	//which entries in edge_list have been used
static int n_used;

static unsigned edge_list_size;		//set each frame

static constexpr packed_edge pack_edge(const unsigned a, const unsigned b)
{
	return static_cast<packed_edge>((a << 3) | b);
}

//define edge numberings
constexpr std::array<packed_edge, 24> edges = {{
	pack_edge(0, 1),	// edge  0
	pack_edge(0, 3),	// edge  1
	pack_edge(0, 4),	// edge  2
	pack_edge(1, 2),	// edge  3
	pack_edge(1, 5),	//	edge  4
	pack_edge(2, 3),	//	edge  5
	pack_edge(2, 6),	//	edge  6
	pack_edge(3, 7),	//	edge  7
	pack_edge(4, 5),	//	edge  8
	pack_edge(4, 7),	//	edge  9
	pack_edge(5, 6),	//	edge 10
	pack_edge(6, 7),	//	edge 11

	pack_edge(0, 5),	//	right cross
	pack_edge(0, 7),	// top cross
	pack_edge(1, 3),	//	front  cross
	pack_edge(2, 5),	// bottom cross
	pack_edge(2, 7),	// left cross
	pack_edge(4, 6),	//	back cross

//crosses going the other way

	pack_edge(1, 4),	//	other right cross
	pack_edge(3, 4),	// other top cross
	pack_edge(0, 2),	//	other front  cross
	pack_edge(1, 6),	// other bottom cross
	pack_edge(3, 6),	// other left cross
	pack_edge(5, 7),	//	other back cross
}};

#define N_NORMAL_EDGES			12		//the normal edges of a box
#define N_EXTRA_EDGES			12		//ones created by triangulation
#define N_EDGES_PER_SEGMENT (N_NORMAL_EDGES+N_EXTRA_EDGES)

//given two vertex numbers on a segment (range 0..7), tell what edge number it is
static std::size_t find_edge_num(const segment_relative_vertnum ev0, const segment_relative_vertnum ev1)
{
	const auto &&[v0, v1] = std::minmax(ev0, ev1);
	const auto vv = pack_edge(static_cast<uint8_t>(v0), static_cast<uint8_t>(v1));
	const auto iter = std::find(edges.begin(), edges.end(), vv);
	return std::distance(edges.begin(), iter);
}

//finds edge, filling in edge_ptr. if found old edge, returns index, else return -1
static std::pair<seg_edge &, std::size_t> find_edge(const vertnum_t v0, const vertnum_t v1)
{
	const auto &&hash_object = std::hash<vertnum_t>{};
	const auto initial_hash_slot = (hash_object(v0) ^ (hash_object(v1) << 10)) % edge_list_size;

	for (auto current_hash_slot = initial_hash_slot;;)
	{
		auto &e = edge_list[current_hash_slot];
		if (e.type == ET_EMPTY)
			return {e, UINT32_MAX};
		else if (e.v0 == v0 && e.v1 == v1)
			return {e, current_hash_slot};
		else {
			if (++ current_hash_slot == edge_list_size)
				current_hash_slot = 0;
			if (current_hash_slot == initial_hash_slot)
				throw std::runtime_error("edge list full: search wrapped without finding a free slot");
		}
	}
}

//adds an edge to the edge list
static void add_edge(const vertnum_t ev0, const vertnum_t ev1, const uint8_t type)
{
	const auto &&[v0, v1] = std::minmax(ev0, ev1);
	auto &&[e, current_hash_slot] = find_edge(v0, v1);

	if (current_hash_slot == UINT32_MAX)
	{
		e.v0 = v0;
		e.v1 = v1;
		e.type = type;
		used_list[n_used] = &e - edge_list.begin();
		if (type == ET_FACING)
			e.face_count++;
		else if (type == ET_NOTFACING)
			e.backface_count++;
		n_used++;
	} else {
		if (e.type > type)
			e.type = type;
		if (type == ET_FACING)
			e.face_count++;
		else if (type == ET_NOTFACING)
			e.backface_count++;
	}
}

//adds a segment's edges to the edge list
static void add_edges(const shared_segment &seg)
{
	auto &svp = seg.verts;
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vcvertptr = Vertices.vcptr;
	if (rotate_list(vcvertptr, svp).uand == clipping_code::None)
	{		//all off screen?
		int	i,fn,vn;
		int	flag;
		std::array<uint8_t, std::size(edges)> edge_flags;
		for (i=0;i<N_NORMAL_EDGES;i++) edge_flags[i]=ET_NOTUSED;
		for (;i<N_EDGES_PER_SEGMENT;i++) edge_flags[i]=ET_NOTEXTANT;

		for (auto &&[idx, sidep] : enumerate(seg.sides))
		{
			int	num_vertices;
			const auto &&[num_faces, vertex_list] = create_all_vertex_lists(seg, sidep, idx);
			if (num_faces == 1)
				num_vertices = 4;
			else
				num_vertices = 3;

			for (fn=0; fn<num_faces; fn++) {
				//Note: normal check appears to be the wrong way since the normals points in, but we're looking from the outside
				if (g3_check_normal_facing(vcvertptr(seg.verts[vertex_list[fn*3]]), sidep.normals[fn]))
					flag = ET_NOTFACING;
				else
					flag = ET_FACING;

				auto v0 = &vertex_list[fn*3];
				for (vn=0; vn<num_vertices-1; vn++) {
					if (const auto en = find_edge_num(*v0, *(v0 + 1)); en != edges.size())
						if (flag < edge_flags[en]) edge_flags[en] = flag;

					v0++;
				}
				if (const auto en = find_edge_num(*v0, vertex_list[fn * 3]); en != edges.size())
					if (flag < edge_flags[en]) edge_flags[en] = flag;
			}
		}

		for (i=0; i<N_EDGES_PER_SEGMENT; i++)
			if (i<N_NORMAL_EDGES || (edge_flags[i]!=ET_NOTEXTANT && Show_triangulations))
			{
				const uint8_t e0 = static_cast<uint8_t>(edges[i]) >> 3;
				const uint8_t e1 = static_cast<uint8_t>(edges[i]) & 7;
				add_edge(seg.verts[(segment_relative_vertnum{e0})], seg.verts[(segment_relative_vertnum{e1})], edge_flags[i]);
			}
	}
}

// ----------------------------------------------------------------------------
static void draw_trigger_side(const shared_segment &seg, const sidenum_t side, const color_palette_index color)
{
	auto &svp = seg.verts;
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vcvertptr = Vertices.vcptr;
	if (rotate_list(vcvertptr, svp).uand == clipping_code::None)
	{		//all off screen?
		// Draw diagonals
		auto &stv = Side_to_verts[side];
		draw_line(g3_draw_line_context{*grd_curcanv, color}, svp[stv[side_relative_vertnum::_0]], svp[stv[side_relative_vertnum::_2]]);
	}
}

// ----------------------------------------------------------------------------
static void draw_wall_side(const shared_segment &seg, const sidenum_t side, const color_palette_index color)
{
	auto &svp = seg.verts;
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vcvertptr = Vertices.vcptr;
	if (rotate_list(vcvertptr, svp).uand == clipping_code::None)
	{		//all off screen?
		// Draw diagonals
		g3_draw_line_context context{*grd_curcanv, color};
		auto &stv = Side_to_verts[side];
		draw_line(context, svp[stv[side_relative_vertnum::_0]], svp[stv[side_relative_vertnum::_2]]);
		draw_line(context, svp[stv[side_relative_vertnum::_1]], svp[stv[side_relative_vertnum::_3]]);
	}
}

#define	WALL_BLASTABLE_COLOR		rgb_t{31, 0, 0}  // RED
#define	WALL_DOOR_COLOR				rgb_t{0, 0, 31}	// DARK BLUE
#define	WALL_DOOR_LOCKED_COLOR		rgb_t{0, 0, 63}	// BLUE
#define	WALL_AUTO_DOOR_COLOR		rgb_t{0, 31, 0}	//	DARK GREEN
#define	WALL_AUTO_DOOR_LOCKED_COLOR	rgb_t{0, 63, 0}	// GREEN
#define	WALL_ILLUSION_COLOR			rgb_t{63, 0, 63}	// PURPLE

#define	TRIGGER_COLOR						BM_XRGB( 63/2 , 63/2 ,  0/2)	// YELLOW

// ----------------------------------------------------------------------------------------------------------------
// Draws special walls (for now these are just removable walls.)
static void draw_special_wall(const shared_segment &seg, const sidenum_t side)
{
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcwallptr = Walls.vcptr;
	auto &w = *vcwallptr(seg.sides[side].wall_num);
	const auto get_color = [=]() {
		const auto type = w.type;
		if (type != WALL_OPEN)
		{
			const auto flags = w.flags;
			if (flags & wall_flag::door_locked)
				return (flags & wall_flag::door_auto) ? WALL_AUTO_DOOR_LOCKED_COLOR : WALL_DOOR_LOCKED_COLOR;
			if (flags & wall_flag::door_auto)
				return WALL_AUTO_DOOR_COLOR;
			if (type == WALL_BLASTABLE)
				return WALL_BLASTABLE_COLOR;
			if (type == WALL_DOOR)
				return WALL_DOOR_COLOR;
			if (type == WALL_ILLUSION)
				return WALL_ILLUSION_COLOR;
		}
		return rgb_t{45, 45, 45};
	};
	const auto color = get_color();
	draw_wall_side(seg,side, gr_find_closest_color(color.r, color.g, color.b));

	if (w.trigger != trigger_none)
	{
		draw_trigger_side(seg,side, TRIGGER_COLOR);
	}
}


// ----------------------------------------------------------------------------------------------------------------
// Recursively parse mine structure, drawing segments.
static void draw_mine_sub(const vmsegptridx_t segnum,int depth, visited_segment_bitarray_t &visited)
{
	if (visited[segnum]) return;		// If segment already drawn, return.
	visited[segnum] = true;		// Say that this segment has been drawn.
	auto mine_ptr = segnum;
	// If this segment is active, process it, else skip it.

	if (mine_ptr->segnum != segment_none) {

		if (Search_mode) check_segment(mine_ptr);
		else add_edges(mine_ptr);	//add this segments edges to list

		if (depth != 0) {
			const shared_segment &sseg = *mine_ptr;
			for (const auto &&[idx, child_segnum, sside] : enumerate(zip(sseg.children, sseg.sides)))
			{
				if (IS_CHILD(child_segnum))
				{
					if (sside.wall_num != wall_none)
						draw_special_wall(mine_ptr, static_cast<sidenum_t>(idx));
					draw_mine_sub(segnum.absolute_sibling(child_segnum), depth-1, visited);
				}
			}
		}
	}
}

static void draw_mine_edges(int automap_flag)
{
	int i,type;
	seg_edge *e;

	for (type=ET_NOTUSED;type>=ET_FACING;type--) {
		const auto color = edge_colors[type];
		g3_draw_line_context context{*grd_curcanv, color};
		for (i=0;i<n_used;i++) {
			e = &edge_list[used_list[i]];
			if (e->type == type)
				if ((!automap_flag) || (e->face_count == 1))
					draw_line(context, e->v0, e->v1);
		}
	}
}

static void clear_edge_list()
{
	range_for (auto &i, partial_range(edge_list, edge_list_size))
	{
		i.type = ET_EMPTY;
		i.face_count = 0;
		i.backface_count = 0;
	}
}

//draws an entire mine
static void draw_mine(const vmsegptridx_t mine_ptr,int depth)
{
	visited_segment_bitarray_t visited;

	edge_list_size = min(LevelSharedSegmentState.Num_segments * 12, MAX_EDGES.value);		//make maybe smaller than max

	// clear edge list
	clear_edge_list();

	n_used = 0;

	draw_mine_sub(mine_ptr,depth, visited);

	draw_mine_edges(0);

}

// -----------------------------------------------------------------------------
//	Draw all segments, ignoring connectivity.
//	A segment is drawn if its segnum != -1.
static void draw_mine_all(int automap_flag)
{
	edge_list_size = min(LevelSharedSegmentState.Num_segments * 12, MAX_EDGES.value);		//make maybe smaller than max

	// clear edge list
	clear_edge_list();

	n_used = 0;

	range_for (const auto &&segp, vmsegptridx)
	{
		if (segp->segnum != segment_none)
		{
			for (auto &&[idx, value] : enumerate(segp->shared_segment::sides))
				if (value.wall_num != wall_none)
					draw_special_wall(segp, idx);
			if (Search_mode)
				check_segment(segp);
			else {
				add_edges(segp);
				draw_seg_objects(*grd_curcanv, segp);
			}
		}
	}

	draw_mine_edges(automap_flag);

}

static void draw_listed_segments(g3_draw_line_context &context, count_segment_array_t &s)
{
	range_for (const auto &ss, s)
	{
		const auto &&segp = vcsegptr(ss);
		if (segp->segnum != segment_none)
			draw_segment(context, segp);
	}
}

static void draw_selected_segments(void)
{
	g3_draw_line_context context{*grd_curcanv, SELECT_COLOR};
	draw_listed_segments(context, Selected_segs);
}

static void draw_found_segments(void)
{
	g3_draw_line_context context{*grd_curcanv, FOUND_COLOR};
	draw_listed_segments(context, Found_segs);
}

static void draw_warning_segments(void)
{
	g3_draw_line_context context{*grd_curcanv, WARNING_COLOR};
	draw_listed_segments(context, Warning_segs);
}

static void draw_group_segments(void)
{
	if (current_group > -1)
	{
		g3_draw_line_context context{*grd_curcanv, GROUP_COLOR};
		draw_listed_segments(context, GroupList[current_group].segments);
	}
}


static void draw_special_segments(void)
{
	// Highlight matcens, fuelcens, etc.
	for (auto &seg : vcsegptr)
	{
		if (seg.segnum != segment_none)
		{
			unsigned r, g, b;
			switch (seg.special)
			{
				case segment_special::fuelcen:
					r = 29 * 2, g = 27 * 2, b = 13 * 2;
				break;
				case segment_special::controlcen:
					r = 29 * 2, g = 0, b = 0;
				break;
				case segment_special::robotmaker:
					r = 29 * 2, g = 0, b = 31 * 2;
				break;
				default:
					continue;
			}
			draw_segment(g3_draw_line_context{*grd_curcanv, /* color = */ gr_find_closest_color(r, g, b)}, seg);
		}
	}
}
		

//find a free vertex. returns the vertex number
static vertnum_t alloc_vert()
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();

	const auto Num_vertices = LevelSharedVertexState.Num_vertices;
	assert(Num_vertices < MAX_SEGMENT_VERTICES);

	auto &Vertex_active = LevelSharedVertexState.get_vertex_active();
	vertnum_t vn{};
	for (; static_cast<unsigned>(vn) < Num_vertices && Vertex_active[vn]; ++vn)
	{
	}

	Vertex_active[vn] = 1;

	++LevelSharedVertexState.Num_vertices;

	return vn;
}

//frees a vertex
static void free_vert(vertnum_t vert_num)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertex_active = LevelSharedVertexState.get_vertex_active();
	Vertex_active[vert_num] = 0;
	--LevelSharedVertexState.Num_vertices;
}

// -----------------------------------------------------------------------------
static void draw_coordinate_axes(void)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	std::array<vertnum_t, 16> Axes_verts;
	vms_vector	tvec;

	range_for (auto &i, Axes_verts)
		i = alloc_vert();

	create_coordinate_axes_from_segment(Cursegp,Axes_verts);

	auto &vcvertptr = Vertices.vcptr;
	auto &vmvertptr = Vertices.vmptr;
	const auto &&av0 = vcvertptr(Axes_verts[0]);
	const auto &&av1 = vcvertptr(Axes_verts[1]);
	const auto &&av2 = vcvertptr(Axes_verts[2]);
	const auto &&av3 = vcvertptr(Axes_verts[3]);
	const auto &&av4 = vmvertptr(Axes_verts[4]);
	const auto &&av6 = vmvertptr(Axes_verts[6]);
	const auto &&av9 = vmvertptr(Axes_verts[9]);
	const auto &&av10 = vmvertptr(Axes_verts[10]);
	const auto &&av11 = vmvertptr(Axes_verts[11]);
	const auto &&av12 = vmvertptr(Axes_verts[12]);
	const auto &&av14 = vmvertptr(Axes_verts[14]);
	const auto &&xvec{vm_vec_sub(av1, av0)};
	const auto &&yvec{vm_vec_sub(av2, av0)};
	const auto &&zvec{vm_vec_sub(av3, av0)};

	// Create the letter X
	tvec = xvec;
	vm_vec_add(av4, av1, vm_vec_scale(tvec, F1_0 / 16));
	tvec = yvec;
	vm_vec_add2(av4, vm_vec_scale(tvec, F1_0 / 8));
	vm_vec_sub(av6, av4, vm_vec_scale(tvec, F2_0));
	tvec = xvec;
	vm_vec_scale(tvec, F1_0 / 8);
	vm_vec_add(vmvertptr(Axes_verts[7]), av4, tvec);
	vm_vec_add(vmvertptr(Axes_verts[5]), av6, tvec);

	//	Create the letter Y
	tvec = yvec;
	vm_vec_add(av11, av2, vm_vec_scale(tvec, F1_0 / 16));
	vm_vec_add(vmvertptr(Axes_verts[8]), av11, tvec);
	vm_vec_add(av9, av11, vm_vec_scale(tvec, F1_0 * 2));
	vm_vec_add(av10, av11, tvec);
	tvec = xvec;
	vm_vec_scale(tvec, F1_0 / 16);
	vm_vec_sub2(av9, tvec);
	vm_vec_add2(av10, tvec);

	// Create the letter Z
	tvec = zvec;
	vm_vec_add(av12, av3, vm_vec_scale(tvec, F1_0 / 16));
	tvec = yvec;
	vm_vec_add2(av12, vm_vec_scale(tvec, F1_0 / 8));
	vm_vec_sub(av14, av12, vm_vec_scale(tvec, F2_0));
	tvec = zvec;
	vm_vec_scale(tvec, F1_0 / 8);
	vm_vec_add(vmvertptr(Axes_verts[13]), av12, tvec);
	vm_vec_add(vmvertptr(Axes_verts[15]), av14, tvec);

	rotate_list(vcvertptr, Axes_verts);

	const color_palette_index color = AXIS_COLOR;
	g3_draw_line_context context{*grd_curcanv, color};

	draw_line(context, Axes_verts[0], Axes_verts[1]);
	draw_line(context, Axes_verts[0], Axes_verts[2]);
	draw_line(context, Axes_verts[0], Axes_verts[3]);

	// draw the letter X
	draw_line(context, Axes_verts[4], Axes_verts[5]);
	draw_line(context, Axes_verts[6], Axes_verts[7]);

	// draw the letter Y
	draw_line(context, Axes_verts[8], Axes_verts[9]);
	draw_line(context, Axes_verts[8], Axes_verts[10]);
	draw_line(context, Axes_verts[8], Axes_verts[11]);

	// draw the letter Z
	draw_line(context, Axes_verts[12], Axes_verts[13]);
	draw_line(context, Axes_verts[13], Axes_verts[14]);
	draw_line(context, Axes_verts[14], Axes_verts[15]);

	range_for (auto &i, Axes_verts)
		free_vert(i);
}

}

void draw_world(grs_canvas *screen_canvas,editor_view *v,const vmsegptridx_t mine_ptr,int depth)
{
	vms_vector viewer_position;

	gr_set_current_canvas(screen_canvas);

	viewer_position = v->ev_matrix.fvec;
	vm_vec_scale(viewer_position,-v->ev_dist);

	vm_vec_add2(viewer_position,Ed_view_target);

	gr_clear_canvas(*grd_curcanv, 0);
	g3_start_frame(*grd_curcanv);
	g3_set_view_matrix(viewer_position,v->ev_matrix,v->ev_zoom);

	render_start_frame();

	// Draw all segments or only connected segments.
	// We might want to draw all segments if we have broken the mine into pieces.
	if (Draw_all_segments)
		draw_mine_all(Automap_test);
	else
		draw_mine(mine_ptr,depth);

	// Draw the found segments
	if (!Automap_test) {
		draw_warning_segments();
		draw_group_segments();
		draw_found_segments();
		draw_selected_segments();
		draw_special_segments();

		// Highlight group segment and side.
		if (current_group > -1)
		if (Groupsegp[current_group]) {
			draw_segment(g3_draw_line_context{*grd_curcanv, GROUPSEG_COLOR}, vcsegptr(Groupsegp[current_group]));
			draw_seg_side(vcsegptr(Groupsegp[current_group]), Groupside[current_group], GROUPSIDE_COLOR);
		}

		// Highlight marked segment and side.
		if (Markedsegp) {
			draw_segment(g3_draw_line_context{*grd_curcanv, MARKEDSEG_COLOR}, Markedsegp);
			draw_seg_side(Markedsegp,Markedside, MARKEDSIDE_COLOR);
		}

		// Highlight current segment and current side.
		draw_segment(g3_draw_line_context{*grd_curcanv, CURSEG_COLOR}, Cursegp);

		draw_seg_side(Cursegp,Curside, CURSIDE_COLOR);
		draw_side_edge(Cursegp,Curside,Curedge, CUREDGE_COLOR);

		// Draw coordinate axes if we are rendering the large view.
		if (Show_axes_flag)
			if (screen_canvas == LargeViewBox->canvas.get())
				draw_coordinate_axes();

		// Label the window
		gr_set_fontcolor(*grd_curcanv, (v==current_view)?CRED:CWHITE, -1);
		if ( screen_canvas == LargeViewBox->canvas.get() ) {
			gr_ustring(*grd_curcanv, *grd_curcanv->cv_font, 5, 5, "USER VIEW");
			switch (Large_view_index) {
				case 0: gr_ustring(*grd_curcanv, *grd_curcanv->cv_font, 85, 5, "-- TOP");	break;
				case 1: gr_ustring(*grd_curcanv, *grd_curcanv->cv_font, 85, 5, "-- FRONT");	break;
				case 2: gr_ustring(*grd_curcanv, *grd_curcanv->cv_font, 85, 5, "-- RIGHT");	break;
			}			
		} else
			Error("Ortho views have been removed, what gives?\n");

	}

	g3_end_frame();

}

//find the segments that render at a given screen x,y
//parms other than x,y are like draw_world
//fills in globals N_found_segs & Found_segs
void find_segments(short x,short y,grs_canvas *screen_canvas,editor_view *v,const vmsegptridx_t mine_ptr,int depth)
{
	vms_vector viewer_position;

	gr_set_current_canvas(screen_canvas);

	viewer_position = v->ev_matrix.fvec;
	vm_vec_scale(viewer_position,-v->ev_dist);

	vm_vec_add2(viewer_position,Ed_view_target);

	g3_start_frame(*grd_curcanv);
	g3_set_view_matrix(viewer_position,v->ev_matrix,v->ev_zoom);

	render_start_frame();

#if DXX_USE_OGL
	g3_end_frame();
#endif
	uint8_t color = 0;
	gr_pixel(grd_curcanv->cv_bitmap, x, y, color);	//set our search pixel to color zero
#if DXX_USE_OGL
	g3_start_frame(*grd_curcanv);
#endif

	Search_mode = -1;
	Found_segs.clear();
	Search_x = x; Search_y = y;

	if (Draw_all_segments)
		draw_mine_all(0);
	else
		draw_mine(mine_ptr,depth);

	g3_end_frame();

	Search_mode = 0;

}

namespace dsx {
void meddraw_init_views( grs_canvas * canvas)
{
#if defined(DXX_BUILD_DESCENT_II)
	// sticking these here so the correct D2 colors are used
	edge_colors[0] = BM_XRGB(45/2,45/2,45/2);
	edge_colors[1] = BM_XRGB(45/3,45/3,45/3);
	edge_colors[2] = BM_XRGB(45/4,45/4,45/4);
#endif

	Views[0]->ev_canv = canvas;
}
}
