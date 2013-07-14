/*
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

int     Search_mode=0;                      //if true, searching for segments at given x,y
int Search_x,Search_y;
int	Automap_test=0;		//	Set to 1 to show wireframe in automap mode.

void draw_seg_objects(segment *seg)
{
	int objnum;

	for (objnum=seg->objects;objnum!=-1;objnum=Objects[objnum].next) {
		object *obj = &Objects[objnum];
		g3s_point sphere_point;

		if ((obj->type==OBJ_PLAYER) && (objnum > 0 ))
			gr_setcolor(BM_XRGB( 0,  25, 0  ));
		else
			gr_setcolor(obj==ConsoleObject?PLAYER_COLOR:ROBOT_COLOR);

		g3_rotate_point(&sphere_point,&obj->pos);

		g3_draw_sphere(&sphere_point,obj->size);
	}

}

void draw_line(int pnum0,int pnum1)
{
	g3_draw_line(&Segment_points[pnum0],&Segment_points[pnum1]);
}

// ----------------------------------------------------------------------------
void draw_segment(segment *seg)
{
	int	*svp;
	int	nv;
	g3s_codes cc;

	if (seg->segnum == -1)		//this segment doesn't exitst
		return;

	med_get_vertex_list(seg,&nv,&svp);				// set nv = number of vertices, svp = pointer to vertex indices
	cc=rotate_list(nv,svp);

	if (! cc.uand) {		//all off screen?
		int i;

		for (i=0;i<4;i++) draw_line(svp[i],svp[i+4]);

		for (i=0;i<3;i++) {
			draw_line(svp[i]  ,svp[i+1]);
			draw_line(svp[i+4],svp[i+4+1]);
		}

		draw_line(svp[0],svp[3]);
		draw_line(svp[0+4],svp[3+4]);

	}

}

//for looking for segment under a mouse click
void check_segment(segment *seg)
{
	int	*svp;
	int	nv;
	g3s_codes cc;

	med_get_vertex_list(seg,&nv,&svp);				// set nv = number of vertices, svp = pointer to vertex indices
	cc=rotate_list(nv,svp);

	if (! cc.uand) {		//all off screen?
		int fn;

		gr_setcolor(0);
#ifdef OGL
		g3_end_frame();
#endif
		gr_pixel(Search_x,Search_y);	//set our search pixel to color zero
#ifdef OGL
		g3_start_frame();
#endif
		gr_setcolor(1);					//and render in color one

		for (fn=0;fn<6;fn++) {
			const g3s_point *vert_list[4];
			
			vert_list[0] = &Segment_points[seg->verts[Side_to_verts[fn][0]]];
			vert_list[1] = &Segment_points[seg->verts[Side_to_verts[fn][1]]];
			vert_list[2] = &Segment_points[seg->verts[Side_to_verts[fn][2]]];
			g3_check_and_draw_poly(3,vert_list,NULL,NULL);

			vert_list[1] = &Segment_points[seg->verts[Side_to_verts[fn][2]]];
			vert_list[2] = &Segment_points[seg->verts[Side_to_verts[fn][3]]];
			g3_check_and_draw_poly(3,vert_list,NULL,NULL);

		}

		if (gr_ugpixel(&grd_curcanv->cv_bitmap,Search_x,Search_y) == 1)
                 {
			if (N_found_segs < MAX_FOUND_SEGS)
				Found_segs[N_found_segs++] = SEG_PTR_2_NUM(seg);
			else
				Warning("Found too many segs! (limit=%d)",MAX_FOUND_SEGS);
                 }
	}
}

// ----------------------------------------------------------------------------
void draw_seg_side(segment *seg,int side)
{
	int	*svp;
	int	nv;
	g3s_codes cc;

	med_get_vertex_list(seg,&nv,&svp);				// set nv = number of vertices, svp = pointer to vertex indices
	cc=rotate_list(nv,svp);

	if (! cc.uand) {		//all off screen?
		int i;

		for (i=0;i<3;i++)
			draw_line(svp[Side_to_verts[side][i]],svp[Side_to_verts[side][i+1]]);

		draw_line(svp[Side_to_verts[side][i]],svp[Side_to_verts[side][0]]);

	}
}

void draw_side_edge(segment *seg,int side,int edge)
{
	int	*svp;
	int	nv;
	g3s_codes cc;

	med_get_vertex_list(seg,&nv,&svp);				// set nv = number of vertices, svp = pointer to vertex indices
	cc=rotate_list(nv,svp);

	if (! cc.uand)		//on screen?
		draw_line(svp[Side_to_verts[side][edge]],svp[Side_to_verts[side][(edge+1)%4]]);
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

int edge_colors[] = { 54, 59, 64 };
							

typedef struct seg_edge {
	union {
		struct {int v0,v1;} __pack__ n;
		long vv;
	}v;
	ushort	type;
	ubyte		face_count, backface_count;
} seg_edge;

#define MAX_EDGES (MAX_VERTICES*4)

seg_edge edge_list[MAX_EDGES];

int	used_list[MAX_EDGES];	//which entries in edge_list have been used
int n_used;

int edge_list_size;		//set each frame

#define HASH(a,b)  ((a*5+b) % edge_list_size)

//define edge numberings
int edges[] = {
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

#define swap(a,b) do {int t; t=(a); (a)=(b); (b)=t;} while (0)

//given two vertex numbers on a segment (range 0..7), tell what edge number it is
int find_edge_num(int v0,int v1)
{
	int		i;
	int		vv;
	int		*edgep = edges;

	if (v0 > v1) swap(v0,v1);

	vv = v0*8+v1;

//	for (i=0;i<N_EDGES_PER_SEGMENT;i++)
//		if (edges[i]==vv) return i;

	for (i=N_EDGES_PER_SEGMENT; i; i--)
		if (*edgep++ == vv)
			return (N_EDGES_PER_SEGMENT-i);

	Error("couldn't find edge for %d,%d",v0,v1);

	//return -1;
}


//finds edge, filling in edge_ptr. if found old edge, returns index, else return -1
int find_edge(int v0,int v1,seg_edge **edge_ptr)
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
void add_edge(int v0,int v1,ubyte type)
{
	int found;

	seg_edge *e;

	if (v0 > v1) swap(v0,v1);

	found = find_edge(v0,v1,&e);

	if (found == -1) {
		e->v.n.v0 = v0;
		e->v.n.v1 = v1;
		e->type = type;
		used_list[n_used] = e-edge_list;
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
void add_edges(segment *seg)
{
	int	*svp;
	int	nv;
	g3s_codes cc;

	med_get_vertex_list(seg,&nv,&svp);				// set nv = number of vertices, svp = pointer to vertex indices
	cc=rotate_list(nv,svp);

	if (! cc.uand) {		//all off screen?
		int	i,sn,fn,vn;
		int	flag;
		ubyte	edge_flags[N_EDGES_PER_SEGMENT];

		for (i=0;i<N_NORMAL_EDGES;i++) edge_flags[i]=ET_NOTUSED;
		for (;i<N_EDGES_PER_SEGMENT;i++) edge_flags[i]=ET_NOTEXTANT;

		for (sn=0;sn<MAX_SIDES_PER_SEGMENT;sn++) {
			side	*sidep = &seg->sides[sn];
			int	num_faces, num_vertices;
			int	vertex_list[6];

			create_all_vertex_lists(&num_faces, vertex_list, seg-Segments, sn);
			if (num_faces == 1)
				num_vertices = 4;
			else
				num_vertices = 3;

			for (fn=0; fn<num_faces; fn++) {
				int	en;
				int	*v0;

				//Note: normal check appears to be the wrong way since the normals points in, but we're looking from the outside
				if (g3_check_normal_facing(&Vertices[seg->verts[vertex_list[fn*3]]],&sidep->normals[fn]))
					flag = ET_NOTFACING;
				else
					flag = ET_FACING;

				v0 = &vertex_list[fn*3];

				for (vn=0; vn<num_vertices-1; vn++) {

					// en = find_edge_num(vertex_list[fn*3 + vn], vertex_list[fn*3 + (vn+1)%num_vertices]);
					en = find_edge_num(*v0, *(v0+1));
					
					if (en!=-1)
						if (flag < edge_flags[en]) edge_flags[en] = flag;

					v0++;
				}
				en = find_edge_num(*v0, vertex_list[fn*3]);
				if (en!=-1)
					if (flag < edge_flags[en]) edge_flags[en] = flag;
			}
		}

		for (i=0; i<N_EDGES_PER_SEGMENT; i++)
			if (i<N_NORMAL_EDGES || (edge_flags[i]!=ET_NOTEXTANT && Show_triangulations))
				add_edge(seg->verts[edges[i]/8],seg->verts[edges[i]&7],edge_flags[i]);
		

	}
}

// ----------------------------------------------------------------------------
void draw_trigger_side(segment *seg,int side)
{
	int	*svp;
	int	nv;
	g3s_codes cc;

	med_get_vertex_list(seg,&nv,&svp);				// set nv = number of vertices, svp = pointer to vertex indices
	cc=rotate_list(nv,svp);

	if (! cc.uand) {		//all off screen?
		// Draw diagonals
		draw_line(svp[Side_to_verts[side][0]],svp[Side_to_verts[side][2]]);
		//g3_draw_line(svp[Side_to_verts[side][1]],svp[Side_to_verts[side][3]]);
	}
}

// ----------------------------------------------------------------------------
void draw_wall_side(segment *seg,int side)
{
	int	*svp;
	int	nv;
	g3s_codes cc;

	med_get_vertex_list(seg,&nv,&svp);				// set nv = number of vertices, svp = pointer to vertex indices
	cc=rotate_list(nv,svp);

	if (! cc.uand) {		//all off screen?
		// Draw diagonals
		draw_line(svp[Side_to_verts[side][0]],svp[Side_to_verts[side][2]]);
		draw_line(svp[Side_to_verts[side][1]],svp[Side_to_verts[side][3]]);

	}
}

#define	WALL_BLASTABLE_COLOR				BM_XRGB( 31/2 ,  0/2 ,  0/2)  // RED
#define	WALL_DOOR_COLOR					BM_XRGB(  0/2 ,  0/2 , 31/2)	// DARK BLUE
#define	WALL_DOOR_LOCKED_COLOR			BM_XRGB(  0/2 ,  0/2 , 63/2)	// BLUE
#define	WALL_AUTO_DOOR_COLOR				BM_XRGB(  0/2 , 31/2 ,  0/2)	//	DARK GREEN
#define	WALL_AUTO_DOOR_LOCKED_COLOR	BM_XRGB(  0/2 , 63/2 ,  0/2)	// GREEN
#define	WALL_ILLUSION_COLOR				BM_XRGB( 63/2 ,  0/2 , 63/2)	// PURPLE

#define	TRIGGER_COLOR						BM_XRGB( 63/2 , 63/2 ,  0/2)	// YELLOW
#define	TRIGGER_DAMAGE_COLOR				BM_XRGB( 63/2 , 63/2 ,  0/2)	// YELLOW

// ----------------------------------------------------------------------------------------------------------------
// Draws special walls (for now these are just removable walls.)
void draw_special_wall( segment *seg, int side )
{
	gr_setcolor(PLAINSEG_COLOR);

	if (Walls[seg->sides[side].wall_num].type == WALL_BLASTABLE)
		gr_setcolor(WALL_BLASTABLE_COLOR);	
	if (Walls[seg->sides[side].wall_num].type == WALL_DOOR)
		gr_setcolor(WALL_DOOR_COLOR);
	if (Walls[seg->sides[side].wall_num].type == WALL_ILLUSION)
		gr_setcolor(GROUPSIDE_COLOR);	
	if (Walls[seg->sides[side].wall_num].flags & WALL_DOOR_LOCKED)
		gr_setcolor(WALL_DOOR_LOCKED_COLOR);
	if (Walls[seg->sides[side].wall_num].flags & WALL_DOOR_AUTO)
		gr_setcolor(WALL_AUTO_DOOR_COLOR);
	if (Walls[seg->sides[side].wall_num].flags & WALL_DOOR_LOCKED)
	if (Walls[seg->sides[side].wall_num].flags & WALL_DOOR_AUTO)
		gr_setcolor(WALL_AUTO_DOOR_LOCKED_COLOR);
	if (Walls[seg->sides[side].wall_num].type == WALL_OPEN)
		gr_setcolor(PLAINSEG_COLOR);
	
	draw_wall_side(seg,side);

	if (Walls[seg->sides[side].wall_num].trigger != -1) {
		gr_setcolor(TRIGGER_COLOR);
		draw_trigger_side(seg,side);
	}

	gr_setcolor(PLAINSEG_COLOR);
}


// ----------------------------------------------------------------------------------------------------------------
// Recursively parse mine structure, drawing segments.
void draw_mine_sub(int segnum,int depth)
{
	segment *mine_ptr;

	if (Been_visited[segnum]) return;		// If segment already drawn, return.

	Been_visited[segnum] = 1;		// Say that this segment has been drawn.

	mine_ptr = &Segments[segnum];

	// If this segment is active, process it, else skip it.

	if (mine_ptr->segnum != -1) {
		int	side;

		if (Search_mode) check_segment(mine_ptr);
		else add_edges(mine_ptr);	//add this segments edges to list

		if (depth != 0) {
			for (side=0; side<MAX_SIDES_PER_SEGMENT; side++) {
				if (IS_CHILD(mine_ptr->children[side])) {
					if (mine_ptr->sides[side].wall_num != -1)
						draw_special_wall(mine_ptr, side);
					draw_mine_sub(mine_ptr->children[side],depth-1);
				}
			}
		}
	}
}

void draw_mine_edges(int automap_flag)
{
	int i,type;
	seg_edge *e;

	for (type=ET_NOTUSED;type>=ET_FACING;type--) {
		gr_setcolor(edge_colors[type]);
		for (i=0;i<n_used;i++) {
			e = &edge_list[used_list[i]];
			if (e->type == type)
				if ((!automap_flag) || (e->face_count == 1))
					draw_line(e->v.n.v0,e->v.n.v1);
		}
	}
}

//draws an entire mine
void draw_mine(segment *mine_ptr,int depth)
{
	int	i;

	// clear visited list
	for (i=0; i<=Highest_segment_index; i++)
		Been_visited[i] = 0;

	edge_list_size = min(Num_segments*12,MAX_EDGES);		//make maybe smaller than max

	// clear edge list
	for (i=0; i<edge_list_size; i++) {
		edge_list[i].type = ET_EMPTY;
		edge_list[i].face_count = 0;
		edge_list[i].backface_count = 0;
	}

	n_used = 0;

	draw_mine_sub(SEG_PTR_2_NUM(mine_ptr),depth);

	draw_mine_edges(0);

}

// -----------------------------------------------------------------------------
//	Draw all segments, ignoring connectivity.
//	A segment is drawn if its segnum != -1.
void draw_mine_all(segment *sp, int automap_flag)
{
	int	s;
	int	i;

	// clear visited list
	for (i=0; i<=Highest_segment_index; i++)
		Been_visited[i] = 0;

	edge_list_size = min(Num_segments*12,MAX_EDGES);		//make maybe smaller than max

	// clear edge list
	for (i=0; i<edge_list_size; i++) {
		edge_list[i].type = ET_EMPTY;
		edge_list[i].face_count = 0;
		edge_list[i].backface_count = 0;
	}

	n_used = 0;

	for (s=0; s<=Highest_segment_index; s++)
		if (sp[s].segnum != -1) {
			for (i=0; i<MAX_SIDES_PER_SEGMENT; i++)
				if (sp[s].sides[i].wall_num != -1)
					draw_special_wall(&sp[s], i);
			if (Search_mode)
				check_segment(&sp[s]);
			else {
				add_edges(&sp[s]);
				draw_seg_objects(&sp[s]);
			}
		}

	draw_mine_edges(automap_flag);

}

void draw_selected_segments(void)
{
	int	s;

	gr_setcolor(SELECT_COLOR);
	for (s=0; s<N_selected_segs; s++)
		if (Segments[Selected_segs[s]].segnum != -1)
			draw_segment(&Segments[Selected_segs[s]]);
}

void draw_found_segments(void)
{
	int	s;

	gr_setcolor(FOUND_COLOR);
	for (s=0; s<N_found_segs; s++)
		if (Segments[Found_segs[s]].segnum != -1)
			draw_segment(&Segments[Found_segs[s]]);
}

void draw_warning_segments(void)
{
	int	s;

	gr_setcolor(WARNING_COLOR);
	for (s=0; s<N_warning_segs; s++)
		if (Segments[Warning_segs[s]].segnum != -1)
			draw_segment(&Segments[Warning_segs[s]]);
}

void draw_group_segments(void)
{
	int	s;

	if (current_group > -1) {
		gr_setcolor(GROUP_COLOR);
		for (s=0; s<GroupList[current_group].num_segments; s++)
			if (Segments[GroupList[current_group].segments[s]].segnum != -1)
				draw_segment(&Segments[GroupList[current_group].segments[s]]);
		}
}


void draw_special_segments(void)
{
	short seg;
	ubyte color;

	// Highlight matcens, fuelcens, etc.
	for (seg=0;seg<=Highest_segment_index;seg++)
		if (Segments[seg].segnum != -1)
			switch(Segment2s[seg].special)
			{
			case SEGMENT_IS_FUELCEN:
				color = BM_XRGB( 29, 27, 13 );
				gr_setcolor(color);
				draw_segment(&Segments[seg]);
				break;
			case SEGMENT_IS_CONTROLCEN:
				color = BM_XRGB( 29, 0, 0 );
				gr_setcolor(color);
				draw_segment(&Segments[seg]);
				break;
			case SEGMENT_IS_ROBOTMAKER:
				color = BM_XRGB( 29, 0, 31 );
				gr_setcolor(color);
				draw_segment(&Segments[seg]);
				break;
			}
}
		

//find a free vertex. returns the vertex number
int alloc_vert()
{
	int vn;

	Assert(Num_vertices < MAX_SEGMENT_VERTICES);

	for (vn=0; (vn < Num_vertices) && Vertex_active[vn]; vn++) ;

	Vertex_active[vn] = 1;

	Num_vertices++;

	return vn;
}

//frees a vertex
void free_vert(int vert_num)
{
	Vertex_active[vert_num] = 0;
	Num_vertices--;
}

// -----------------------------------------------------------------------------
void draw_coordinate_axes(void)
{
	int			i;
	int			Axes_verts[16];
	vms_vector	tvec,xvec,yvec,zvec;

	for (i=0; i<16; i++)
		Axes_verts[i] = alloc_vert();

	create_coordinate_axes_from_segment(Cursegp,Axes_verts);

	vm_vec_sub(&xvec,&Vertices[Axes_verts[1]],&Vertices[Axes_verts[0]]);
	vm_vec_sub(&yvec,&Vertices[Axes_verts[2]],&Vertices[Axes_verts[0]]);
	vm_vec_sub(&zvec,&Vertices[Axes_verts[3]],&Vertices[Axes_verts[0]]);

	// Create the letter X
	tvec = xvec;
	vm_vec_add(&Vertices[Axes_verts[4]],&Vertices[Axes_verts[1]],vm_vec_scale(&tvec,F1_0/16));
	tvec = yvec;
	vm_vec_add2(&Vertices[Axes_verts[4]],vm_vec_scale(&tvec,F1_0/8));
	vm_vec_sub(&Vertices[Axes_verts[6]],&Vertices[Axes_verts[4]],vm_vec_scale(&tvec,F2_0));
	tvec = xvec;	vm_vec_scale(&tvec,F1_0/8);
	vm_vec_add(&Vertices[Axes_verts[7]],&Vertices[Axes_verts[4]],&tvec);
	vm_vec_add(&Vertices[Axes_verts[5]],&Vertices[Axes_verts[6]],&tvec);

	//	Create the letter Y
	tvec = yvec;
	vm_vec_add(&Vertices[Axes_verts[11]],&Vertices[Axes_verts[2]],vm_vec_scale(&tvec,F1_0/16));
	vm_vec_add(&Vertices[Axes_verts[8]],&Vertices[Axes_verts[11]],&tvec);
	vm_vec_add(&Vertices[Axes_verts[9]],&Vertices[Axes_verts[11]],vm_vec_scale(&tvec,F1_0*2));
	vm_vec_add(&Vertices[Axes_verts[10]],&Vertices[Axes_verts[11]],&tvec);
	tvec = xvec;	vm_vec_scale(&tvec,F1_0/16);
	vm_vec_sub2(&Vertices[Axes_verts[9]],&tvec);
	vm_vec_add2(&Vertices[Axes_verts[10]],&tvec);

	// Create the letter Z
	tvec = zvec;
	vm_vec_add(&Vertices[Axes_verts[12]],&Vertices[Axes_verts[3]],vm_vec_scale(&tvec,F1_0/16));
	tvec = yvec;
	vm_vec_add2(&Vertices[Axes_verts[12]],vm_vec_scale(&tvec,F1_0/8));
	vm_vec_sub(&Vertices[Axes_verts[14]],&Vertices[Axes_verts[12]],vm_vec_scale(&tvec,F2_0));
	tvec = zvec;	vm_vec_scale(&tvec,F1_0/8);
	vm_vec_add(&Vertices[Axes_verts[13]],&Vertices[Axes_verts[12]],&tvec);
	vm_vec_add(&Vertices[Axes_verts[15]],&Vertices[Axes_verts[14]],&tvec);

	rotate_list(16,Axes_verts);

	gr_setcolor(AXIS_COLOR);

	draw_line(Axes_verts[0],Axes_verts[1]);
	draw_line(Axes_verts[0],Axes_verts[2]);
	draw_line(Axes_verts[0],Axes_verts[3]);

	// draw the letter X
	draw_line(Axes_verts[4],Axes_verts[5]);
	draw_line(Axes_verts[6],Axes_verts[7]);

	// draw the letter Y
	draw_line(Axes_verts[8],Axes_verts[9]);
	draw_line(Axes_verts[8],Axes_verts[10]);
	draw_line(Axes_verts[8],Axes_verts[11]);

	// draw the letter Z
	draw_line(Axes_verts[12],Axes_verts[13]);
	draw_line(Axes_verts[13],Axes_verts[14]);
	draw_line(Axes_verts[14],Axes_verts[15]);

	for (i=0; i<16; i++)
		free_vert(Axes_verts[i]);
}

void draw_world(grs_canvas *screen_canvas,editor_view *v,segment *mine_ptr,int depth)
{
	vms_vector viewer_position;

	gr_set_current_canvas(screen_canvas);

	//g3_set_points(Segment_points,Vertices);

	viewer_position = v->ev_matrix.fvec;
	vm_vec_scale(&viewer_position,-v->ev_dist);

	vm_vec_add2(&viewer_position,&Ed_view_target);

	gr_clear_canvas(0);
	g3_start_frame();
	g3_set_view_matrix(&viewer_position,&v->ev_matrix,v->ev_zoom);

	render_start_frame();

	gr_setcolor(PLAINSEG_COLOR);

	// Draw all segments or only connected segments.
	// We might want to draw all segments if we have broken the mine into pieces.
	if (Draw_all_segments)
		draw_mine_all(Segments, Automap_test);
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
			gr_setcolor(GROUPSEG_COLOR);
			draw_segment(Groupsegp[current_group]);

			gr_setcolor(GROUPSIDE_COLOR);
			draw_seg_side(Groupsegp[current_group],Groupside[current_group]);
		}

		// Highlight marked segment and side.
		if (Markedsegp) {
			gr_setcolor(MARKEDSEG_COLOR);
			draw_segment(Markedsegp);

			gr_setcolor(MARKEDSIDE_COLOR);
			draw_seg_side(Markedsegp,Markedside);
		}

		// Highlight current segment and current side.
		gr_setcolor(CURSEG_COLOR);
		draw_segment(Cursegp);

		gr_setcolor(CURSIDE_COLOR);
		draw_seg_side(Cursegp,Curside);

		gr_setcolor(CUREDGE_COLOR);
		draw_side_edge(Cursegp,Curside,Curedge);

		// Draw coordinate axes if we are rendering the large view.
		if (Show_axes_flag)
			if (screen_canvas == LargeViewBox->canvas)
				draw_coordinate_axes();

		// Label the window
		gr_set_fontcolor((v==current_view)?CRED:CWHITE, -1 );
		if ( screen_canvas == LargeViewBox->canvas ) {
			gr_ustring( 5, 5, "USER VIEW" );
			switch (Large_view_index) {
				case 0: gr_ustring( 85, 5, "-- TOP");	break;
				case 1: gr_ustring( 85, 5, "-- FRONT");	break;
				case 2: gr_ustring( 85, 5, "-- RIGHT");	break;
			}			
		} else
#if ORTHO_VIEWS
		 else if ( screen_canvas == TopViewBox->canvas )
			gr_ustring( 5, 5, "TOP" );
		else if ( screen_canvas == FrontViewBox->canvas )
			gr_ustring( 5, 5, "FRONT" );
		else if ( screen_canvas == RightViewBox->canvas )
			gr_ustring( 5, 5, "RIGHT" );
#else
			Error("Ortho views have been removed, what gives?\n");
#endif

	}

	g3_end_frame();

}

//find the segments that render at a given screen x,y
//parms other than x,y are like draw_world
//fills in globals N_found_segs & Found_segs
void find_segments(short x,short y,grs_canvas *screen_canvas,editor_view *v,segment *mine_ptr,int depth)
{
	vms_vector viewer_position;

	gr_set_current_canvas(screen_canvas);

	//g3_set_points(Segment_points,Vertices);

	viewer_position = v->ev_matrix.fvec;
	vm_vec_scale(&viewer_position,-v->ev_dist);

	vm_vec_add(&viewer_position,&viewer_position,&Ed_view_target);

	g3_start_frame();
	g3_set_view_matrix(&viewer_position,&v->ev_matrix,v->ev_zoom);

	render_start_frame();

	gr_setcolor(0);
#ifdef OGL
	g3_end_frame();
#endif
	gr_pixel(x,y);	//set our search pixel to color zero
#ifdef OGL
	g3_start_frame();
#endif
	gr_setcolor(1);

	Search_mode = -1;
	N_found_segs = 0;
	Search_x = x; Search_y = y;

	if (Draw_all_segments)
		draw_mine_all(Segments, 0);
	else
		draw_mine(mine_ptr,depth);

	g3_end_frame();

	Search_mode = 0;

}

void meddraw_init_views( grs_canvas * canvas)
{
	// sticking these here so the correct D2 colors are used
	edge_colors[0] = BM_XRGB(45/2,45/2,45/2);
	edge_colors[1] = BM_XRGB(45/3,45/3,45/3);		//BM_RGB(0,0,45),	//
	edge_colors[2] = BM_XRGB(45/4,45/4,45/4);	//BM_RGB(0,45,0)};	//

	Views[0]->ev_canv = canvas;
#if ORTHO_VIEWS
	Views[1]->ev_canv = TopViewBox->canvas;
	Views[2]->ev_canv = FrontViewBox->canvas;
	Views[3]->ev_canv = RightViewBox->canvas;
#endif
}
