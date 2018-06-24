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
#include "key.h"
#include "mouse.h"
#include "dxxerror.h"
#include "medlisp.h"
#include "u_mem.h"
#include "render.h"
#include "game.h"
#include "kdefs.h"
#include "func.h"
#include "textures.h"
#include "screens.h"
#include "texmap.h"
#include "object.h"
#include "fuelcen.h"
#include "meddraw.h"
#include "d_enumerate.h"
#include "compiler-range_for.h"
#include "segiter.h"

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

static int     Search_mode=0;                      //if true, searching for segments at given x,y
static int Search_x,Search_y;
static int	Automap_test=0;		//	Set to 1 to show wireframe in automap mode.

static void draw_seg_objects(grs_canvas &canvas, const unique_segment &seg)
{
	range_for (const auto obj, objects_in(seg, vcobjptridx, vcsegptr))
	{
		auto sphere_point = g3_rotate_point(obj->pos);
		const uint8_t color = (obj->type == OBJ_PLAYER && static_cast<icobjptridx_t::index_type>(obj) > 0)
			? BM_XRGB(0,  25, 0)
			: (obj == ConsoleObject
				? PLAYER_COLOR
				: ROBOT_COLOR
			);
		g3_draw_sphere(canvas, sphere_point, obj->size, color);
	}
}

#if DXX_USE_OGL
#define draw_line(C,P0,P1,c)	draw_line(P0,P1,c)
#define draw_segment(C,S,c)	draw_segment(S,c)
#define draw_listed_segments(C,S,c)	draw_listed_segments(S,c)
#endif
static void draw_line(grs_canvas &canvas, const unsigned pnum0, const unsigned pnum1, const uint8_t color)
{
	g3_draw_line(canvas, Segment_points[pnum0], Segment_points[pnum1], color);
}

// ----------------------------------------------------------------------------
static void draw_segment(grs_canvas &canvas, const vcsegptr_t seg, const uint8_t color)
{
	if (seg->segnum == segment_none)		//this segment doesn't exitst
		return;

	auto &svp = seg->verts;
	if (!rotate_list(svp).uand)
	{		//all off screen?
		for (unsigned i = 0; i < 4; ++i)
			draw_line(canvas, svp[i], svp[i+4], color);

		for (unsigned i = 0; i < 3; ++i)
		{
			draw_line(canvas, svp[i], svp[i+1], color);
			draw_line(canvas, svp[i+4], svp[i+4+1], color);
		}

		draw_line(canvas, svp[0], svp[3], color);
		draw_line(canvas, svp[4], svp[3+4], color);
	}
}

//for looking for segment under a mouse click
static void check_segment(const vmsegptridx_t seg)
{
	auto &svp = seg->verts;
	if (!rotate_list(svp).uand)
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
			array<cg3s_point *, 3> vert_list;
			vert_list[0] = &Segment_points[seg->verts[fn[0]]];
			vert_list[1] = &Segment_points[seg->verts[fn[1]]];
			vert_list[2] = &Segment_points[seg->verts[fn[2]]];
			g3_check_and_draw_poly(*grd_curcanv, vert_list, color);

			vert_list[1] = &Segment_points[seg->verts[fn[2]]];
			vert_list[2] = &Segment_points[seg->verts[fn[3]]];
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
static void draw_seg_side(const vcsegptr_t seg, int side, const uint8_t color)
{
	auto &svp = seg->verts;
	if (!rotate_list(svp).uand)
	{		//all off screen?
		int i;

		auto &stv = Side_to_verts[side];
		for (i=0;i<3;i++)
			draw_line(*grd_curcanv, svp[stv[i]], svp[stv[i+1]], color);

		draw_line(*grd_curcanv, svp[stv[i]], svp[stv[0]], color);
	}
}

static void draw_side_edge(const vcsegptr_t seg,int side,int edge, const uint8_t color)
{
	auto &svp = seg->verts;
	if (!rotate_list(svp).uand)		//on screen?
	{
		auto &stv = Side_to_verts[side];
		draw_line(*grd_curcanv, svp[stv[edge]], svp[stv[(edge + 1) % 4]], color);
	}
}

int Show_triangulations=0;

//edge types - lower number types have precedence
#define ET_FACING		0	//this edge on a facing face
#define ET_NOTFACING	1	//this edge on a non-facing face
#define ET_NOTUSED	2	//no face uses this edge
#define ET_NOTEXTANT	3	//would exist if side were triangulated 

#define ET_EMPTY		255	//this entry in array is empty

//colors for those types
//int edge_colors[] = {BM_RGB(45/2,45/2,45/2),
//							BM_RGB(45/3,45/3,45/3),		//BM_RGB(0,0,45),	//
//							BM_RGB(45/4,45/4,45/4)};	//BM_RGB(0,45,0)};	//

static
#if defined(DXX_BUILD_DESCENT_I)
const
#endif
array<color_t, 3> edge_colors{{54, 59, 64}};

namespace {

struct seg_edge
{
	union {
		struct {int v0,v1;} __pack__ n;
		long vv;
	}v;
	ushort	type;
	ubyte		face_count, backface_count;
};

}

static array<seg_edge, MAX_EDGES> edge_list;
static array<int, MAX_EDGES> used_list;	//which entries in edge_list have been used
static int n_used;

static unsigned edge_list_size;		//set each frame

#define HASH(a,b)  ((a*5+b) % edge_list_size)

//define edge numberings
constexpr int edges[] = {
		0*8+1,	// edge  0
		0*8+3,	// edge  1
		0*8+4,	// edge  2
		1*8+2,	// edge  3
		1*8+5,	//	edge  4
		2*8+3,	//	edge  5
		2*8+6,	//	edge  6
		3*8+7,	//	edge  7
		4*8+5,	//	edge  8
		4*8+7,	//	edge  9
		5*8+6,	//	edge 10
		6*8+7,	//	edge 11

		0*8+5,	//	right cross
		0*8+7,	// top cross
		1*8+3,	//	front  cross
		2*8+5,	// bottom cross
		2*8+7,	// left cross
		4*8+6,	//	back cross

//crosses going the other way

		1*8+4,	//	other right cross
		3*8+4,	// other top cross
		0*8+2,	//	other front  cross
		1*8+6,	// other bottom cross
		3*8+6,	// other left cross
		5*8+7,	//	other back cross
};

#define N_NORMAL_EDGES			12		//the normal edges of a box
#define N_EXTRA_EDGES			12		//ones created by triangulation
#define N_EDGES_PER_SEGMENT (N_NORMAL_EDGES+N_EXTRA_EDGES)

using std::swap;

//given two vertex numbers on a segment (range 0..7), tell what edge number it is
static int find_edge_num(int v0,int v1)
{
	int		i;
	int		vv;
	const int		*edgep = edges;

	if (v0 > v1) swap(v0,v1);

	vv = v0*8+v1;

//	for (i=0;i<N_EDGES_PER_SEGMENT;i++)
//		if (edges[i]==vv) return i;

	for (i=N_EDGES_PER_SEGMENT; i; i--)
		if (*edgep++ == vv)
			return (N_EDGES_PER_SEGMENT-i);

	Error("Could not find edge for %d,%d",v0,v1);

	//return -1;
}


//finds edge, filling in edge_ptr. if found old edge, returns index, else return -1
static int find_edge(int v0,int v1,seg_edge **edge_ptr)
{
	long vv;
	int hash,oldhash;
	int ret;

	vv = (v1<<16) + v0;

	oldhash = hash = HASH(v0,v1);

	ret = -1;

	while (ret==-1) {

		if (edge_list[hash].type == ET_EMPTY) ret=0;
		else if (edge_list[hash].v.vv == vv) ret=1;
		else {
			if (++hash==edge_list_size) hash=0;
			if (hash==oldhash) Error("Edge list full!");
		}
	}

	*edge_ptr = &edge_list[hash];

	if (ret == 0)
		return -1;
	else
		return hash;

}

//adds an edge to the edge list
static void add_edge(int v0,int v1,ubyte type)
{
	int found;

	seg_edge *e;

	if (v0 > v1) swap(v0,v1);

	found = find_edge(v0,v1,&e);

	if (found == -1) {
		e->v.n.v0 = v0;
		e->v.n.v1 = v1;
		e->type = type;
		used_list[n_used] = e - edge_list.begin();
		if (type == ET_FACING)
			edge_list[used_list[n_used]].face_count++;
		else if (type == ET_NOTFACING)
			edge_list[used_list[n_used]].backface_count++;
		n_used++;
	} else {
		if (type < e->type)
			e->type = type;
		if (type == ET_FACING)
			edge_list[found].face_count++;
		else if (type == ET_NOTFACING)
			edge_list[found].backface_count++;
	}
}

//adds a segment's edges to the edge list
static void add_edges(const vcsegptridx_t seg)
{
	auto &svp = seg->verts;
	if (!rotate_list(svp).uand)
	{		//all off screen?
		int	i,fn,vn;
		int	flag;
		ubyte	edge_flags[N_EDGES_PER_SEGMENT];

		for (i=0;i<N_NORMAL_EDGES;i++) edge_flags[i]=ET_NOTUSED;
		for (;i<N_EDGES_PER_SEGMENT;i++) edge_flags[i]=ET_NOTEXTANT;

		range_for (auto &&e, enumerate(seg->sides))
		{
			auto &sidep = e.value;
			int	num_vertices;
			const auto v = create_all_vertex_lists(seg, sidep, e.idx);
			const auto &num_faces = v.first;
			const auto &vertex_list = v.second;
			if (num_faces == 1)
				num_vertices = 4;
			else
				num_vertices = 3;

			for (fn=0; fn<num_faces; fn++) {
				int	en;

				//Note: normal check appears to be the wrong way since the normals points in, but we're looking from the outside
				if (g3_check_normal_facing(vcvertptr(seg->verts[vertex_list[fn*3]]), sidep.normals[fn]))
					flag = ET_NOTFACING;
				else
					flag = ET_FACING;

				auto v0 = &vertex_list[fn*3];
				for (vn=0; vn<num_vertices-1; vn++) {

					// en = find_edge_num(vertex_list[fn*3 + vn], vertex_list[fn*3 + (vn+1)%num_vertices]);
					en = find_edge_num(*v0, *(v0+1));
					
					if (en!=edge_none)
						if (flag < edge_flags[en]) edge_flags[en] = flag;

					v0++;
				}
				en = find_edge_num(*v0, vertex_list[fn*3]);
				if (en!=edge_none)
					if (flag < edge_flags[en]) edge_flags[en] = flag;
			}
		}

		for (i=0; i<N_EDGES_PER_SEGMENT; i++)
			if (i<N_NORMAL_EDGES || (edge_flags[i]!=ET_NOTEXTANT && Show_triangulations))
				add_edge(seg->verts[edges[i]/8],seg->verts[edges[i]&7],edge_flags[i]);
		

	}
}

// ----------------------------------------------------------------------------
static void draw_trigger_side(const vcsegptr_t seg,int side, const uint8_t color)
{
	auto &svp = seg->verts;
	if (!rotate_list(svp).uand)
	{		//all off screen?
		// Draw diagonals
		auto &stv = Side_to_verts[side];
		draw_line(*grd_curcanv, svp[stv[0]], svp[stv[2]], color);
	}
}

// ----------------------------------------------------------------------------
static void draw_wall_side(const vcsegptr_t seg,int side, const uint8_t color)
{
	auto &svp = seg->verts;
	if (!rotate_list(svp).uand)
	{		//all off screen?
		// Draw diagonals
		auto &stv = Side_to_verts[side];
		draw_line(*grd_curcanv, svp[stv[0]], svp[stv[2]], color);
		draw_line(*grd_curcanv, svp[stv[1]], svp[stv[3]], color);
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
static void draw_special_wall(const vcsegptr_t seg, int side )
{
	auto &w = *vcwallptr(seg->sides[side].wall_num);
	const auto get_color = [=]() {
		const auto type = w.type;
		if (type != WALL_OPEN)
		{
			const auto flags = w.flags;
			if (flags & WALL_DOOR_LOCKED)
				return (flags & WALL_DOOR_AUTO) ? WALL_AUTO_DOOR_LOCKED_COLOR : WALL_DOOR_LOCKED_COLOR;
			if (flags & WALL_DOOR_AUTO)
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
		int	side;

		if (Search_mode) check_segment(mine_ptr);
		else add_edges(mine_ptr);	//add this segments edges to list

		if (depth != 0) {
			for (side=0; side<MAX_SIDES_PER_SEGMENT; side++) {
				const auto child_segnum = mine_ptr->children[side];
				if (IS_CHILD(child_segnum))
				{
					if (mine_ptr->sides[side].wall_num != wall_none)
						draw_special_wall(mine_ptr, side);
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
		for (i=0;i<n_used;i++) {
			e = &edge_list[used_list[i]];
			if (e->type == type)
				if ((!automap_flag) || (e->face_count == 1))
					draw_line(*grd_curcanv, e->v.n.v0, e->v.n.v1, color);
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

	edge_list_size = min(Num_segments * 12, MAX_EDGES.value);		//make maybe smaller than max

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
	edge_list_size = min(Num_segments * 12, MAX_EDGES.value);		//make maybe smaller than max

	// clear edge list
	clear_edge_list();

	n_used = 0;

	range_for (const auto &&segp, vmsegptridx)
	{
		if (segp->segnum != segment_none)
		{
			range_for (auto &&e, enumerate(segp->sides))
				if (e.value.wall_num != wall_none)
					draw_special_wall(segp, e.idx);
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

static void draw_listed_segments(grs_canvas &canvas, count_segment_array_t &s, const uint8_t color)
{
	range_for (const auto &ss, s)
	{
		const auto &&segp = vcsegptr(ss);
		if (segp->segnum != segment_none)
			draw_segment(canvas, segp, color);
	}
}

static void draw_selected_segments(void)
{
	draw_listed_segments(*grd_curcanv, Selected_segs, SELECT_COLOR);
}

static void draw_found_segments(void)
{
	draw_listed_segments(*grd_curcanv, Found_segs, FOUND_COLOR);
}

static void draw_warning_segments(void)
{
	draw_listed_segments(*grd_curcanv, Warning_segs, WARNING_COLOR);
}

static void draw_group_segments(void)
{
	if (current_group > -1) {
		draw_listed_segments(*grd_curcanv, GroupList[current_group].segments, GROUP_COLOR);
		}
}


static void draw_special_segments(void)
{
	// Highlight matcens, fuelcens, etc.
	range_for (const auto &&segp, vcsegptr)
	{
		if (segp->segnum != segment_none)
		{
			unsigned r, g, b;
			switch(segp->special)
			{
			case SEGMENT_IS_FUELCEN:
					r = 29 * 2, g = 27 * 2, b = 13 * 2;
				break;
			case SEGMENT_IS_CONTROLCEN:
					r = 29 * 2, g = 0, b = 0;
				break;
			case SEGMENT_IS_ROBOTMAKER:
					r = 29 * 2, g = 0, b = 31 * 2;
				break;
				default:
					continue;
			}
			const auto color = gr_find_closest_color(r, g, b);
			draw_segment(*grd_curcanv, segp, color);
		}
	}
}
		

//find a free vertex. returns the vertex number
static int alloc_vert()
{
	int vn;

	Assert(Num_vertices < MAX_SEGMENT_VERTICES);

	for (vn=0; (vn < Num_vertices) && Vertex_active[vn]; vn++) ;

	Vertex_active[vn] = 1;

	Num_vertices++;

	return vn;
}

//frees a vertex
static void free_vert(int vert_num)
{
	Vertex_active[vert_num] = 0;
	Num_vertices--;
}

// -----------------------------------------------------------------------------
static void draw_coordinate_axes(void)
{
	array<unsigned, 16> Axes_verts;
	vms_vector	tvec;

	range_for (auto &i, Axes_verts)
		i = alloc_vert();

	create_coordinate_axes_from_segment(Cursegp,Axes_verts);

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
	const auto &&xvec = vm_vec_sub(av1, av0);
	const auto &&yvec = vm_vec_sub(av2, av0);
	const auto &&zvec = vm_vec_sub(av3, av0);

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

	rotate_list(Axes_verts);

	const uint8_t color = AXIS_COLOR;

	draw_line(*grd_curcanv, Axes_verts[0], Axes_verts[1], color);
	draw_line(*grd_curcanv, Axes_verts[0], Axes_verts[2], color);
	draw_line(*grd_curcanv, Axes_verts[0], Axes_verts[3], color);

	// draw the letter X
	draw_line(*grd_curcanv, Axes_verts[4], Axes_verts[5], color);
	draw_line(*grd_curcanv, Axes_verts[6], Axes_verts[7], color);

	// draw the letter Y
	draw_line(*grd_curcanv, Axes_verts[8], Axes_verts[9], color);
	draw_line(*grd_curcanv, Axes_verts[8], Axes_verts[10], color);
	draw_line(*grd_curcanv, Axes_verts[8], Axes_verts[11], color);

	// draw the letter Z
	draw_line(*grd_curcanv, Axes_verts[12], Axes_verts[13], color);
	draw_line(*grd_curcanv, Axes_verts[13], Axes_verts[14], color);
	draw_line(*grd_curcanv, Axes_verts[14], Axes_verts[15], color);

	range_for (auto &i, Axes_verts)
		free_vert(i);
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
			draw_segment(*grd_curcanv, vcsegptr(Groupsegp[current_group]), GROUPSEG_COLOR);
			draw_seg_side(vcsegptr(Groupsegp[current_group]), Groupside[current_group], GROUPSIDE_COLOR);
		}

		// Highlight marked segment and side.
		if (Markedsegp) {
			draw_segment(*grd_curcanv, Markedsegp, MARKEDSEG_COLOR);
			draw_seg_side(Markedsegp,Markedside, MARKEDSIDE_COLOR);
		}

		// Highlight current segment and current side.
		draw_segment(*grd_curcanv, Cursegp, CURSEG_COLOR);

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
#if ORTHO_VIEWS
		 else if ( screen_canvas == TopViewBox->canvas )
			gr_ustring(*grd_curcanv, *grd_curcanv->cv_font, 5, 5, "TOP");
		else if ( screen_canvas == FrontViewBox->canvas )
			gr_ustring(*grd_curcanv, *grd_curcanv->cv_font, 5, 5, "FRONT");
		else if ( screen_canvas == RightViewBox->canvas )
			gr_ustring(*grd_curcanv, *grd_curcanv->cv_font, 5, 5, "RIGHT");
#else
			Error("Ortho views have been removed, what gives?\n");
#endif

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
	edge_colors[1] = BM_XRGB(45/3,45/3,45/3);		//BM_RGB(0,0,45),	//
	edge_colors[2] = BM_XRGB(45/4,45/4,45/4);	//BM_RGB(0,45,0)};	//
#endif

	Views[0]->ev_canv = canvas;
#if ORTHO_VIEWS
	Views[1]->ev_canv = TopViewBox->canvas;
	Views[2]->ev_canv = FrontViewBox->canvas;
	Views[3]->ev_canv = RightViewBox->canvas;
#endif
}
}
