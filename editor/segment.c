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
 * Interrogation functions for segment data structure.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "key.h"
#include "gr.h"
#include "inferno.h"
#include "segment.h"
#include "editor.h"
#include "editor/esegment.h"
#include "dxxerror.h"
#include "object.h"
#include "gameseg.h"
#include "render.h"
#include "game.h"
#include "wall.h"
#include "switch.h"
#include "fuelcen.h"
#include "cntrlcen.h"
#include "seguvs.h"
#include "gameseq.h"

#include "medwall.h"
#include "hostage.h"

int	Do_duplicate_vertex_check = 0;		// Gets set to 1 in med_create_duplicate_vertex, means to check for duplicate vertices in compress_mine

#define	BOTTOM_STUFF	0

//	Remap all vertices in polygons in a segment through translation table xlate_verts.
#if BOTTOM_STUFF
void remap_vertices(segment *segp, int *xlate_verts)
{
	int	sidenum, facenum, polynum, v;

	for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++)
		for (facenum=0; facenum<segp->sides[sidenum].num_faces; facenum++)
			for (polynum=0; polynum<segp->sides[sidenum].faces[facenum].num_polys; polynum++) {
				poly *pp = &segp->sides[sidenum].faces[facenum].polys[polynum];
				for (v=0; v<pp->num_vertices; v++)
					pp->verts[v] = xlate_verts[pp->verts[v]];
			}
}

//	Copy everything from sourceside to destside except sourceside->faces[xx].polys[xx].verts
void copy_side_except_vertex_ids(side *destside, side *sourceside)
{
	int	facenum, polynum, v;

	destside->num_faces = sourceside->num_faces;
	destside->tri_edge = sourceside->tri_edge;
	destside->wall_num = sourceside->wall_num;

	for (facenum=0; facenum<sourceside->num_faces; facenum++) {
		face *destface = &destside->faces[facenum];
		face *sourceface = &sourceside->faces[facenum];

		destface->num_polys = sourceface->num_polys;
		destface->normal = sourceface->normal;

		for (polynum=0; polynum<sourceface->num_polys; polynum++) {
			poly *destpoly = &destface->polys[polynum];
			poly *sourcepoly = &sourceface->polys[polynum];

			destpoly->num_vertices = sourcepoly->num_vertices;
			destpoly->face_type = sourcepoly->face_type;
			destpoly->tmap_num = sourcepoly->tmap_num;
			destpoly->tmap_num2 = sourcepoly->tmap_num2;

			for (v=0; v<sourcepoly->num_vertices; v++)
				destpoly->uvls[v] = sourcepoly->uvls[v];
		}

	}
}

//	[side] [index] [cur:next]
//	To remap the vertices on a side after a forward rotation
sbyte xlate_previous[6][4][2] = {
{ {7, 3}, {3, 2}, {2, 6}, {6, 7} },		// remapping left to left
{ {5, 4}, {4, 0}, {7, 3}, {6, 7} },		// remapping back to top
{ {5, 4}, {1, 5}, {0, 1}, {4, 0} },		// remapping right to right
{ {0, 1}, {1, 5}, {2, 6}, {3, 2} },		//	remapping front to bottom
{ {1, 5}, {5, 4}, {6, 7}, {2, 6} },		// remapping bottom to back
{ {4, 0}, {0, 1}, {3, 2}, {7, 3} },		// remapping top to front
};

void remap_vertices_previous(segment *segp, int sidenum)
{
	int	v, w, facenum, polynum;

	for (facenum=0; facenum<segp->sides[sidenum].num_faces; facenum++) {
		for (polynum=0; polynum<segp->sides[sidenum].faces[facenum].num_polys; polynum++) {
			poly *pp = &segp->sides[sidenum].faces[facenum].polys[polynum];

			for (v=0; v<pp->num_vertices; v++) {
				for (w=0; w<4; w++) {
					if (pp->verts[v] == xlate_previous[sidenum][w][0]) {
						pp->verts[v] = xlate_previous[sidenum][w][1];
						break;
					}
				}
				Assert(w<4);	// If w == 4, then didn't find current vertex in list, which means xlate_previous table is bogus
			}
		}
	}
}

sbyte xlate_previous_right[6][4][2] = {
{ {5, 6}, {6, 7}, {2, 3}, {1, 2} },		// bottom to left
{ {6, 7}, {7, 4}, {3, 0}, {2, 3} },		// left to top
{ {7, 4}, {4, 5}, {0, 1}, {3, 0} },		// top to right
{ {4, 5}, {5, 6}, {1, 2}, {0, 1} },		// right to bottom
{ {6, 7}, {5, 6}, {4, 5}, {7, 4} },		// back to back
{ {3, 2}, {0, 3}, {1, 0}, {2, 1} },		// front to front
};

void remap_vertices_previous_right(segment *segp, int sidenum)
{
	int	v, w, facenum, polynum;

	for (facenum=0; facenum<segp->sides[sidenum].num_faces; facenum++) {
		for (polynum=0; polynum<segp->sides[sidenum].faces[facenum].num_polys; polynum++) {
			poly *pp = &segp->sides[sidenum].faces[facenum].polys[polynum];

			for (v=0; v<pp->num_vertices; v++) {
				for (w=0; w<4; w++) {
					if (pp->verts[v] == xlate_previous_right[sidenum][w][0]) {
						pp->verts[v] = xlate_previous_right[sidenum][w][1];
						break;
					}
				}
				Assert(w<4);	// If w == 4, then didn't find current vertex in list, which means xlate_previous table is bogus
			}
		}
	}
}


// -----------------------------------------------------------------------------------
//	Takes top to front
void med_rotate_segment_forward(segment *segp)
{
	segment	seg_copy;
	int		i;

	seg_copy = *segp;

	seg_copy.verts[0] = segp->verts[4];
	seg_copy.verts[1] = segp->verts[0];
	seg_copy.verts[2] = segp->verts[3];
	seg_copy.verts[3] = segp->verts[7];
	seg_copy.verts[4] = segp->verts[5];
	seg_copy.verts[5] = segp->verts[1];
	seg_copy.verts[6] = segp->verts[2];
	seg_copy.verts[7] = segp->verts[6];

	seg_copy.children[WFRONT] = segp->children[WTOP];
	seg_copy.children[WTOP] = segp->children[WBACK];
	seg_copy.children[WBACK] = segp->children[WBOTTOM];
	seg_copy.children[WBOTTOM] = segp->children[WFRONT];

	seg_copy.sides[WFRONT] = segp->sides[WTOP];
	seg_copy.sides[WTOP] = segp->sides[WBACK];
	seg_copy.sides[WBACK] = segp->sides[WBOTTOM];
	seg_copy.sides[WBOTTOM] = segp->sides[WFRONT];

	for (i=0; i<6; i++)
		remap_vertices_previous(&seg_copy, i);

	*segp = seg_copy;
}

// -----------------------------------------------------------------------------------
//	Takes top to right
void med_rotate_segment_right(segment *segp)
{
	segment	seg_copy;
	int		i;

	seg_copy = *segp;

	seg_copy.verts[4] = segp->verts[7];
	seg_copy.verts[5] = segp->verts[4];
	seg_copy.verts[1] = segp->verts[0];
	seg_copy.verts[0] = segp->verts[3];
	seg_copy.verts[3] = segp->verts[2];
	seg_copy.verts[2] = segp->verts[1];
	seg_copy.verts[6] = segp->verts[5];
	seg_copy.verts[7] = segp->verts[6];

	seg_copy.children[WRIGHT] = segp->children[WTOP];
	seg_copy.children[WBOTTOM] = segp->children[WRIGHT];
	seg_copy.children[WLEFT] = segp->children[WBOTTOM];
	seg_copy.children[WTOP] = segp->children[WLEFT];

	seg_copy.sides[WRIGHT] = segp->sides[WTOP];
	seg_copy.sides[WBOTTOM] = segp->sides[WRIGHT];
	seg_copy.sides[WLEFT] = segp->sides[WBOTTOM];
	seg_copy.sides[WTOP] = segp->sides[WLEFT];

	for (i=0; i<6; i++)
		remap_vertices_previous_right(&seg_copy, i);

	*segp = seg_copy;
}

void make_curside_bottom_side(void)
{
	switch (Curside) {
		case WRIGHT:	med_rotate_segment_right(Cursegp);		break;
		case WTOP:		med_rotate_segment_right(Cursegp);		med_rotate_segment_right(Cursegp);		break;
		case WLEFT:		med_rotate_segment_right(Cursegp);		med_rotate_segment_right(Cursegp);		med_rotate_segment_right(Cursegp);		break;
		case WBOTTOM:	break;
		case WFRONT:	med_rotate_segment_forward(Cursegp);	break;
		case WBACK:		med_rotate_segment_forward(Cursegp);	med_rotate_segment_forward(Cursegp);	med_rotate_segment_forward(Cursegp);	break;
	}
	Update_flags = UF_WORLD_CHANGED;
}
#endif

int ToggleBottom(void)
{
	Render_only_bottom = !Render_only_bottom;
	Update_flags = UF_WORLD_CHANGED;
	return 0;
}
		
// ---------------------------------------------------------------------------------------------
//           ---------- Segment interrogation functions ----------
// ----------------------------------------------------------------------------
//	Return a pointer to the list of vertex indices for the current segment in vp and
//	the number of vertices in *nv.
void med_get_vertex_list(segment *s,int *nv,int **vp)
{
	*vp = s->verts;
	*nv = MAX_VERTICES_PER_SEGMENT;
}

// -------------------------------------------------------------------------------
//	Return number of times vertex vi appears in all segments.
//	This function can be used to determine whether a vertex is used exactly once in
//	all segments, in which case it can be freely moved because it is not connected
//	to any other segment.
int med_vertex_count(int vi)
{
	int		s,v;
	segment	*sp;
	int		count;

	count = 0;

	for (s=0; s<MAX_SEGMENTS; s++) {
		sp = &Segments[s];
		if (sp->segnum != -1)
			for (v=0; v<MAX_VERTICES_PER_SEGMENT; v++)
				if (sp->verts[v] == vi)
					count++;
	}

	return count;
}

// -------------------------------------------------------------------------------
int is_free_vertex(int vi)
{
	return med_vertex_count(vi) == 1;
}


// -------------------------------------------------------------------------------
// Move a free vertex in the segment by adding the vector *vofs to its coordinates.
//	Error handling:
// 	If the point is not free then:
//		If the point is not valid (probably valid = in 0..7) then:
//		If adding *vofs will cause a degenerate segment then:
//	Note, pi is the point index relative to the segment, not an absolute point index.
// For example, 3 is always the front upper left vertex.
void med_move_vertex(segment *sp, int pi, vms_vector *vofs)
{
	int	abspi;

	Assert((pi >= 0) && (pi <= 7));		// check valid range of point indices.

	abspi = sp->verts[pi];

	// Make sure vertex abspi is free.  If it is free, it appears exactly once in Vertices
	Assert(med_vertex_count(abspi) == 1);

	Assert(abspi <= MAX_SEGMENT_VERTICES);			// Make sure vertex id is not bogus.

	vm_vec_add(&Vertices[abspi],&Vertices[abspi],vofs);

	// Here you need to validate the geometry of the segment, which will be quite tricky.
	// You need to make sure:
	//		The segment is not concave.
	//		None of the sides are concave.
	validate_segment(sp);

}

// -------------------------------------------------------------------------------
//	Move a free wall in the segment by adding the vector *vofs to its coordinates.
//	Wall indices: 0/1/2/3/4/5 = left/top/right/bottom/back/front
void med_move_wall(segment *sp,int wi, vms_vector *vofs)
{
	const sbyte *vp;
	int	i;

	Assert( (wi >= 0) && (wi <= 5) );

	vp = Side_to_verts[wi];
	for (i=0; i<4; i++) {
		med_move_vertex(sp,*vp,vofs);
		vp++;
	}

	validate_segment(sp);
}

// -------------------------------------------------------------------------------
//	Return true if one fixed point number is very close to another, else return false.
int fnear(fix f1, fix f2)
{
	return (abs(f1 - f2) <= FIX_EPSILON);
}

// -------------------------------------------------------------------------------
int vnear(vms_vector *vp1, vms_vector *vp2)
{
	return fnear(vp1->x, vp2->x) && fnear(vp1->y, vp2->y) && fnear(vp1->z, vp2->z);
}

// -------------------------------------------------------------------------------
//	Add the vertex *vp to the global list of vertices, return its index.
//	Search until a matching vertex is found (has nearly the same coordinates) or until Num_vertices
// vertices have been looked at without a match.  If no match, add a new vertex.
int med_add_vertex(vms_vector *vp)
{
	int	v,free_index;
	int	count;					// number of used vertices found, for loops exits when count == Num_vertices

//	set_vertex_counts();

	Assert(Num_vertices < MAX_SEGMENT_VERTICES);

	count = 0;
	free_index = -1;
	for (v=0; (v < MAX_SEGMENT_VERTICES) && (count < Num_vertices); v++)
		if (Vertex_active[v]) {
			count++;
			if (vnear(vp,&Vertices[v])) {
				return v;
			}
		} else if (free_index == -1)
			free_index = v;					// we want free_index to be the first free slot to add a vertex

	if (free_index == -1)
		free_index = Num_vertices;

	while (Vertex_active[free_index] && (free_index < MAX_VERTICES))
		free_index++;

	Assert(free_index < MAX_VERTICES);

	Vertices[free_index] = *vp;
	Vertex_active[free_index] = 1;

	Num_vertices++;

	if (free_index > Highest_vertex_index)
		Highest_vertex_index = free_index;

	return free_index;
}

// ------------------------------------------------------------------------------------------
//	Returns the index of a free segment.
//	Scans the Segments array.
int get_free_segment_number(void)
{
	int	segnum;

	for (segnum=0; segnum<MAX_SEGMENTS; segnum++)
		if (Segments[segnum].segnum == -1) {
			Num_segments++;
			if (segnum > Highest_segment_index)
				Highest_segment_index = segnum;
			return segnum;
		}

	Assert(0);

	return 0;
}

// -------------------------------------------------------------------------------
//	Create a new segment, duplicating exactly, including vertex ids and children, the passed segment.
int med_create_duplicate_segment(segment *sp)
{
	int	segnum;

	segnum = get_free_segment_number();

	Segments[segnum] = *sp;	

	return segnum;
}

// -------------------------------------------------------------------------------
//	Add the vertex *vp to the global list of vertices, return its index.
//	This is the same as med_add_vertex, except that it does not search for the presence of the vertex.
int med_create_duplicate_vertex(vms_vector *vp)
{
	int	free_index;

	Assert(Num_vertices < MAX_SEGMENT_VERTICES);

	Do_duplicate_vertex_check = 1;

	free_index = Num_vertices;

	while (Vertex_active[free_index] && (free_index < MAX_VERTICES))
		free_index++;

	Assert(free_index < MAX_VERTICES);

	Vertices[free_index] = *vp;
	Vertex_active[free_index] = 1;

	Num_vertices++;

	if (free_index > Highest_vertex_index)
		Highest_vertex_index = free_index;

	return free_index;
}


// -------------------------------------------------------------------------------
//	Set the vertex *vp at index vnum in the global list of vertices, return its index (just for compatibility).
int med_set_vertex(int vnum,vms_vector *vp)
{
	Assert(vnum < MAX_VERTICES);

	Vertices[vnum] = *vp;

	// Just in case this vertex wasn't active, mark it as active.
	if (!Vertex_active[vnum]) {
		Vertex_active[vnum] = 1;
		Num_vertices++;
		if ((vnum > Highest_vertex_index) && (vnum < NEW_SEGMENT_VERTICES)) {
			Highest_vertex_index = vnum;
		}
	}

	return vnum;
}

// -------------------------------------------------------------------------------
void create_removable_wall(segment *sp, int sidenum, int tmap_num)
{
	create_walls_on_side(sp, sidenum);

	sp->sides[sidenum].tmap_num = tmap_num;

	assign_default_uvs_to_side(sp, sidenum);
	assign_light_to_side(sp, sidenum);
}

#if 0

// ---------------------------------------------------------------------------------------------
//	Orthogonalize matrix smat, returning result in rmat.
//	Does not modify smat.
//	Uses Gram-Schmidt process.
//	See page 172 of Strang, Gilbert, Linear Algebra and its Applications
//	Matt -- This routine should be moved to the vector matrix library.
//	It IS legal for smat == rmat.
//	We should also have the functions:
//		mat_a = mat_b * scalar;				// we now have mat_a = mat_a * scalar;
//		mat_a = mat_b + mat_c * scalar;	// or maybe not, maybe this is not primitive
void make_orthogonal(vms_matrix *rmat,vms_matrix *smat)
{
	vms_matrix		tmat;
	vms_vector		tvec1,tvec2;
	float				dot;

	// Copy source matrix to work area.
	tmat = *smat;

	// Normalize the three rows of the matrix tmat.
	vm_vec_normalize(&tmat.xrow);
	vm_vec_normalize(&tmat.yrow);
	vm_vec_normalize(&tmat.zrow);

	//	Now, compute the first vector.
	// This is very easy -- just copy the (normalized) source vector.
	rmat->zrow = tmat.zrow;

	// Now, compute the second vector.
	// From page 172 of Strang, we use the equation:
	//		b' = b - [transpose(q1) * b] * q1
	//	where:	b  = the second row of tmat
	//				q1 = the first row of rmat
	//				b' = the second row of rmat

	// Compute: transpose(q1) * b
	dot = vm_vec_dotprod(&rmat->zrow,&tmat.yrow);

	// Compute: b - dot * q1
	rmat->yrow.x = tmat.yrow.x - fixmul(dot,rmat->zrow.x);
	rmat->yrow.y = tmat.yrow.y - fixmul(dot,rmat->zrow.y);
	rmat->yrow.z = tmat.yrow.z - fixmul(dot,rmat->zrow.z);

	// Now, compute the third vector.
	// From page 173 of Strang, we use the equation:
	//		c' = c - (q1*c)*q1 - (q2*c)*q2
	//	where:	c  = the third row of tmat
	//				q1 = the first row of rmat
	//				q2 = the second row of rmat
	//				c' = the third row of rmat

	// Compute: q1*c
	dot = vm_vec_dotprod(&rmat->zrow,&tmat.xrow);

	tvec1.x = fixmul(dot,rmat->zrow.x);
	tvec1.y = fixmul(dot,rmat->zrow.y);
	tvec1.z = fixmul(dot,rmat->zrow.z);

	// Compute: q2*c
	dot = vm_vec_dotprod(&rmat->yrow,&tmat.xrow);

	tvec2.x = fixmul(dot,rmat->yrow.x);
	tvec2.y = fixmul(dot,rmat->yrow.y);
	tvec2.z = fixmul(dot,rmat->yrow.z);

	vm_vec_sub(&rmat->xrow,vm_vec_sub(&rmat->xrow,&tmat.xrow,&tvec1),&tvec2);
}

#endif

// ------------------------------------------------------------------------------------------
// Given a segment, extract the rotation matrix which defines it.
// Do this by extracting the forward, right, up vectors and then making them orthogonal.
// In the process of making the vectors orthogonal, favor them in the order forward, up, right.
// This means that the forward vector will remain unchanged.
void med_extract_matrix_from_segment(segment *sp,vms_matrix *rotmat)
{
	vms_vector	forwardvec,upvec;

	extract_forward_vector_from_segment(sp,&forwardvec);
	extract_up_vector_from_segment(sp,&upvec);

	if (((forwardvec.x == 0) && (forwardvec.y == 0) && (forwardvec.z == 0)) || ((upvec.x == 0) && (upvec.y == 0) && (upvec.z == 0))) {
		*rotmat = vmd_identity_matrix;
		return;
	}


	vm_vector_2_matrix(rotmat,&forwardvec,&upvec,NULL);

#if 0
	vms_matrix	rm;

	extract_forward_vector_from_segment(sp,&rm.zrow);
	extract_right_vector_from_segment(sp,&rm.xrow);
	extract_up_vector_from_segment(sp,&rm.yrow);

	vm_vec_normalize(&rm.xrow);
	vm_vec_normalize(&rm.yrow);
	vm_vec_normalize(&rm.zrow);

	make_orthogonal(rotmat,&rm);

	vm_vec_normalize(&rotmat->xrow);
	vm_vec_normalize(&rotmat->yrow);
	vm_vec_normalize(&rotmat->zrow);

// *rotmat = rm; // include this line (and remove the call to make_orthogonal) if you don't want the matrix orthogonalized
#endif

}

// ------------------------------------------------------------------------------------------
//	Given a rotation matrix *rotmat which describes the orientation of a segment
//	and a side destside, return the rotation matrix which describes the orientation for the side.
void	set_matrix_based_on_side(vms_matrix *rotmat,int destside)
{
        vms_angvec      rotvec,*tmpvec;
        vms_matrix      r1,rtemp;

	switch (destside) {
		case WLEFT:
                        tmpvec=vm_angvec_make(&rotvec,0,0,-16384);
			vm_angles_2_matrix(&r1,&rotvec);
			vm_matrix_x_matrix(&rtemp,rotmat,&r1);
			*rotmat = rtemp;
			break;

		case WTOP:
                        tmpvec=vm_angvec_make(&rotvec,-16384,0,0);
			vm_angles_2_matrix(&r1,&rotvec);
			vm_matrix_x_matrix(&rtemp,rotmat,&r1);
			*rotmat = rtemp;
			break;

		case WRIGHT:
                        tmpvec=vm_angvec_make(&rotvec,0,0,16384);
			vm_angles_2_matrix(&r1,&rotvec);
			vm_matrix_x_matrix(&rtemp,rotmat,&r1);
			*rotmat = rtemp;
			break;

		case WBOTTOM:
                        tmpvec=vm_angvec_make(&rotvec,+16384,-32768,0);        // bank was -32768, but I think that was an erroneous compensation
			vm_angles_2_matrix(&r1,&rotvec);
			vm_matrix_x_matrix(&rtemp,rotmat,&r1);
			*rotmat = rtemp;
			break;

		case WFRONT:
                        tmpvec=vm_angvec_make(&rotvec,0,0,-32768);
			vm_angles_2_matrix(&r1,&rotvec);
			vm_matrix_x_matrix(&rtemp,rotmat,&r1);
			*rotmat = rtemp;
			break;

		case WBACK:
			break;
	}
	(void)tmpvec;
}

//	-------------------------------------------------------------------------------------
void change_vertex_occurrences(int dest, int src)
{
	int	g,s,v;

	// Fix vertices in groups
	for (g=0;g<num_groups;g++) 
		for (v=0; v<GroupList[g].num_vertices; v++)
			if (GroupList[g].vertices[v] == src)
				GroupList[g].vertices[v] = dest;

	// now scan all segments, changing occurrences of src to dest
	for (s=0; s<=Highest_segment_index; s++)
		if (Segments[s].segnum != -1)
			for (v=0; v<MAX_VERTICES_PER_SEGMENT; v++)
				if (Segments[s].verts[v] == src)
					Segments[s].verts[v] = dest;
}

// --------------------------------------------------------------------------------------------------
void compress_vertices(void)
{
	int		hole,vert;

	if (Highest_vertex_index == Num_vertices - 1)
		return;

	vert = Highest_vertex_index;	//MAX_SEGMENT_VERTICES-1;

	for (hole=0; hole < vert; hole++)
		if (!Vertex_active[hole]) {
			// found an unused vertex which is a hole if a used vertex follows (not necessarily immediately) it.
			for ( ; (vert>hole) && (!Vertex_active[vert]); vert--)
				;

			if (vert > hole) {

				// Ok, hole is the index of a hole, vert is the index of a vertex which follows it.
				// Copy vert into hole, update pointers to it.
				Vertices[hole] = Vertices[vert];
				
				change_vertex_occurrences(hole, vert);

				vert--;
			}
		}

	Highest_vertex_index = Num_vertices-1;
}

// --------------------------------------------------------------------------------------------------
void compress_segments(void)
{
	int		hole,seg;

	if (Highest_segment_index == Num_segments - 1)
		return;

	seg = Highest_segment_index;

	for (hole=0; hole < seg; hole++)
		if (Segments[hole].segnum == -1) {
			// found an unused segment which is a hole if a used segment follows (not necessarily immediately) it.
			for ( ; (seg>hole) && (Segments[seg].segnum == -1); seg--)
				;

			if (seg > hole) {
				int		f,g,l,s,t,w;
				segment	*sp;
				int objnum;

				// Ok, hole is the index of a hole, seg is the index of a segment which follows it.
				// Copy seg into hole, update pointers to it, update Cursegp, Markedsegp if necessary.
				Segments[hole] = Segments[seg];
				Segments[seg].segnum = -1;

				if (Cursegp == &Segments[seg])
					Cursegp = &Segments[hole];

				if (Markedsegp == &Segments[seg])
					Markedsegp = &Segments[hole];

				// Fix segments in groups
				for (g=0;g<num_groups;g++) 
					for (s=0; s<GroupList[g].num_segments; s++)
						if (GroupList[g].segments[s] == seg)
							GroupList[g].segments[s] = hole;

				// Fix walls
				for (w=0;w<Num_walls;w++)
					if (Walls[w].segnum == seg)
						Walls[w].segnum = hole;

				// Fix fuelcenters, robotcens, and triggers... added 2/1/95 -Yuan
				for (f=0;f<Num_fuelcenters;f++)
					if (Station[f].segnum == seg)
						Station[f].segnum = hole;

				for (f=0;f<Num_robot_centers;f++)
					if (RobotCenters[f].segnum == seg)
						RobotCenters[f].segnum = hole;

				for (t=0;t<Num_triggers;t++)
					for (l=0;l<Triggers[t].num_links;l++)
						if (Triggers[t].seg[l] == seg)
							Triggers[t].seg[l] = hole;

				sp = &Segments[hole];
				for (s=0; s<MAX_SIDES_PER_SEGMENT; s++) {
					if (IS_CHILD(sp->children[s])) {
						segment *csegp;
						csegp = &Segments[sp->children[s]];

						// Find out on what side the segment connection to the former seg is on in *csegp.
						for (t=0; t<MAX_SIDES_PER_SEGMENT; t++) {
							if (csegp->children[t] == seg) {
								csegp->children[t] = hole;					// It used to be connected to seg, so make it connected to hole
							}
						}	// end for t
					}	// end if
				}	// end for s

				//Update object segment pointers
				for (objnum = sp->objects; objnum != -1; objnum = Objects[objnum].next) {
					Assert(Objects[objnum].segnum == seg);
					Objects[objnum].segnum = hole;
				}

				seg--;

			}	// end if (seg > hole)
		}	// end if

	Highest_segment_index = Num_segments-1;
	med_create_new_segment_from_cursegp();

}


// -------------------------------------------------------------------------------
//	Combine duplicate vertices.
//	If two vertices have the same coordinates, within some small tolerance, then assign
//	the same vertex number to the two vertices, freeing up one of the vertices.
void med_combine_duplicate_vertices(sbyte *vlp)
{
	int	v,w;

	for (v=0; v<Highest_vertex_index; v++)		// Note: ok to do to <, rather than <= because w for loop starts at v+1
		if (vlp[v]) {
			vms_vector *vvp = &Vertices[v];
			for (w=v+1; w<=Highest_vertex_index; w++)
				if (vlp[w]) {	//	used to be Vertex_active[w]
					if (vnear(vvp, &Vertices[w])) {
						change_vertex_occurrences(v, w);
					}
				}
		}

}

// ------------------------------------------------------------------------------
//	Compress mine at Segments and Vertices by squeezing out all holes.
//	If no holes (ie, an unused segment followed by a used segment), then no action.
//	If Cursegp or Markedsegp is a segment which gets moved to fill in a hole, then
//	they are properly updated.
void med_compress_mine(void)
{
	if (Do_duplicate_vertex_check) {
		med_combine_duplicate_vertices(Vertex_active);
		Do_duplicate_vertex_check = 0;
	}

	compress_segments();
	compress_vertices();
	set_vertex_counts();

	//--repair-- create_local_segment_data();

	//	This is necessary becuase a segment search (due to click in 3d window) uses the previous frame's
	//	segment information, which could get changed by this.
	Update_flags = UF_WORLD_CHANGED;
}


// ------------------------------------------------------------------------------------------
//	Copy texture map ids for each face in sseg to dseg.
void copy_tmap_ids(segment *dseg, segment *sseg)
{
	int	s;

	for (s=0; s<MAX_SIDES_PER_SEGMENT; s++) {
		dseg->sides[s].tmap_num = sseg->sides[s].tmap_num;
		dseg->sides[s].tmap_num2 = 0;
	}
}

// ------------------------------------------------------------------------------------------
//	Attach a segment with a rotated orientation.
// Return value:
//  0 = successful attach
//  1 = No room in Segments[].
//  2 = No room in Vertices[].
//  3 = newside != WFRONT -- for now, the new segment must be attached at its (own) front side
//	 4 = already a face attached on destseg:destside
int med_attach_segment_rotated(segment *destseg, segment *newseg, int destside, int newside,vms_matrix *attmat)
{
	const sbyte		*dvp;
	segment		*nsp;
	segment2	*nsp2;
	int			side,v;
	vms_matrix	rotmat,rotmat1,rotmat2,rotmat3,rotmat4;
	vms_vector	vr,vc,tvs[4],xlate_vec;
	int			segnum;
	vms_vector	forvec,upvec;

	// Return if already a face attached on this side.
	if (IS_CHILD(destseg->children[destside]))
		return 4;

	segnum = get_free_segment_number();

	forvec = attmat->fvec;
	upvec = attmat->uvec;

	//	We are pretty confident we can add the segment.
	nsp = &Segments[segnum];
	nsp2 = &Segment2s[segnum];

	nsp->segnum = segnum;
	nsp->objects = -1;
	nsp2->matcen_num = -1;

	// Copy group value.
	nsp->group = destseg->group;

	// Add segment to proper group list.
	if (nsp->group > -1)
		add_segment_to_group(nsp-Segments, nsp->group);

	// Copy the texture map ids.
	copy_tmap_ids(nsp,newseg);

	// clear all connections
	for (side=0; side<MAX_SIDES_PER_SEGMENT; side++) {
		nsp->children[side] = -1;
		nsp->sides[side].wall_num = -1;	
	}

	// Form the connection
	destseg->children[destside] = segnum;
//	destseg->sides[destside].render_flag = 0;
	nsp->children[newside] = destseg-Segments;

	// Copy vertex indices of the four vertices forming the joint
	dvp = Side_to_verts[destside];

	// Set the vertex indices for the four vertices forming the front of the new side
	for (v=0; v<4; v++)
                nsp->verts[v] = destseg->verts[(int) dvp[v]];

	// The other 4 vertices must be created.
	// Their coordinates are determined by the 4 welded vertices and the vector from front
	// to back of the original *newseg.

	// Do lots of hideous matrix stuff, about 3/4 of which could probably be simplified out.
	med_extract_matrix_from_segment(destseg,&rotmat);		// get orientation matrix for destseg (orthogonal rotation matrix)
	set_matrix_based_on_side(&rotmat,destside);
	vm_vector_2_matrix(&rotmat1,&forvec,&upvec,NULL);
	vm_matrix_x_matrix(&rotmat4,&rotmat,&rotmat1);			// this is the desired orientation of the new segment
	med_extract_matrix_from_segment(newseg,&rotmat3);		// this is the current orientation of the new segment
	vm_transpose_matrix(&rotmat3);								// get the inverse of the current orientation matrix
	vm_matrix_x_matrix(&rotmat2,&rotmat4,&rotmat3);			// now rotmat2 takes the current segment to the desired orientation

	// Warning -- look at this line!
	vm_transpose_matrix(&rotmat2);	// added 12:33 pm, 10/01/93

	// Compute and rotate the center point of the attaching face.
	compute_center_point_on_side(&vc,newseg,newside);
	vm_vec_rotate(&vr,&vc,&rotmat2);

	// Now rotate the free vertices in the segment
	for (v=0; v<4; v++)
		vm_vec_rotate(&tvs[v],&Vertices[newseg->verts[v+4]],&rotmat2);

	// Now translate the new segment so that the center point of the attaching faces are the same.
	compute_center_point_on_side(&vc,destseg,destside);
	vm_vec_sub(&xlate_vec,&vc,&vr);

	// Create and add the 4 new vertices.
	for (v=0; v<4; v++) {
		vm_vec_add2(&tvs[v],&xlate_vec);
		nsp->verts[v+4] = med_add_vertex(&tvs[v]);
	}

	set_vertex_counts();

	// Now all the vertices are in place.  Create the faces.
	validate_segment(nsp);

	//	Say to not render at the joint.
//	destseg->sides[destside].render_flag = 0;
//	nsp->sides[newside].render_flag = 0;

	Cursegp = nsp;

	return	0;
}

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

// ------------------------------------------------------------------------------------------
void scale_free_vertices(segment *sp,vms_vector *vp,fix scale_factor,int min_side,int max_side)
{
	int	i;
	const sbyte	*verts;

	verts = Side_to_verts[min_side];

	for (i=0; i<4; i++)
                if (is_free_vertex(sp->verts[(int) verts[i]])) {
                        Vertices[sp->verts[(int) verts[i]]].x = fixmul(vp->x,scale_factor)/2;
                        Vertices[sp->verts[(int) verts[i]]].y = fixmul(vp->y,scale_factor)/2;
                        Vertices[sp->verts[(int) verts[i]]].z = fixmul(vp->z,scale_factor)/2;
		}

	verts = Side_to_verts[max_side];

	for (i=0; i<4; i++)
                if (is_free_vertex(sp->verts[(int) verts[i]])) {
                        Vertices[sp->verts[(int) verts[i]]].x = fixmul(vp->x,scale_factor)/2;
                        Vertices[sp->verts[(int) verts[i]]].y = fixmul(vp->y,scale_factor)/2;
                        Vertices[sp->verts[(int) verts[i]]].z = fixmul(vp->z,scale_factor)/2;
		}
}


// ------------------------------------------------------------------------------------------
// Attach side newside of newseg to side destside of destseg.
// Copies *newseg into global array Segments, increments Num_segments.
// Forms a weld between the two segments by making the new segment fit to the old segment.
// Updates number of faces per side if necessitated by new vertex coordinates.
//	Updates Cursegp.
// Return value:
//  0 = successful attach
//  1 = No room in Segments[].
//  2 = No room in Vertices[].
//  3 = newside != WFRONT -- for now, the new segment must be attached at its (own) front side
//	 4 = already a face attached on side newside
int med_attach_segment(segment *destseg, segment *newseg, int destside, int newside)
{
	int		rval;
	segment	*ocursegp = Cursegp;

	vms_angvec	tang = {0,0,0};
	vms_matrix	rotmat;

	vm_angles_2_matrix(&rotmat,&tang);
	rval = med_attach_segment_rotated(destseg,newseg,destside,newside,&rotmat);
	med_propagate_tmaps_to_segments(ocursegp,Cursegp,0);
	med_propagate_tmaps_to_back_side(Cursegp, Side_opposite[newside],0);
	copy_uvs_seg_to_seg(&New_segment,Cursegp);

	return rval;
}

// -------------------------------------------------------------------------------
//	Delete a vertex, sort of.
//	Decrement the vertex count.  If the count goes to 0, then the vertex is free (has been deleted).
void delete_vertex(int v)
{
	Assert(v < MAX_VERTICES);			// abort if vertex is not in array Vertices
	Assert(Vertex_active[v] >= 1);	// abort if trying to delete a non-existent vertex

	Vertex_active[v]--;
}

// -------------------------------------------------------------------------------
//	Update Num_vertices.
//	This routine should be called by anyone who calls delete_vertex.  It could be called in delete_vertex,
//	but then it would be called much more often than necessary, and it is a slow routine.
void update_num_vertices(void)
{
	int	v;

	// Now count the number of vertices.
	Num_vertices = 0;
	for (v=0; v<=Highest_vertex_index; v++)
		if (Vertex_active[v])
			Num_vertices++;
}

// -------------------------------------------------------------------------------
//	Set Vertex_active to number of occurrences of each vertex.
//	Set Num_vertices.
void set_vertex_counts(void)
{
	int	s,v;

	Num_vertices = 0;

	for (v=0; v<=Highest_vertex_index; v++)
		Vertex_active[v] = 0;

	// Count number of occurrences of each vertex.
	for (s=0; s<=Highest_segment_index; s++)
		if (Segments[s].segnum != -1)
			for (v=0; v<MAX_VERTICES_PER_SEGMENT; v++) {
				if (!Vertex_active[Segments[s].verts[v]])
					Num_vertices++;
				Vertex_active[Segments[s].verts[v]]++;
			}
}

// -------------------------------------------------------------------------------
//	Delete all vertices in segment *sp from the vertex list if they are not contained in another segment.
//	This is kind of a dangerous routine.  It modifies the global array Vertex_active, using the field as
//	a count.
void delete_vertices_in_segment(segment *sp)
{
	int	v;

//	init_vertices();

	set_vertex_counts();

	// Subtract one count for each appearance of vertex in deleted segment
	for (v=0; v<MAX_VERTICES_PER_SEGMENT; v++)
		delete_vertex(sp->verts[v]);

	update_num_vertices();
}

// -------------------------------------------------------------------------------
//	Delete segment *sp in Segments array.
// Return value:
//		0	successfully deleted.
//		1	unable to delete.
int med_delete_segment(segment *sp)
{
	int		s,side,segnum;
	int 		objnum;

	segnum = sp-Segments;

	// Cannot delete segment if only segment.
	if (Num_segments == 1)
		return 1;

	// Don't try to delete if segment doesn't exist.
	if (sp->segnum == -1) {
		return 1;
	}

	// Delete its refueling center if it has one
	fuelcen_delete(sp);

	delete_vertices_in_segment(sp);

	Num_segments--;

	// If deleted segment has walls on any side, wipe out the wall.
	for (side=0; side < MAX_SIDES_PER_SEGMENT; side++)
		if (sp->sides[side].wall_num != -1) 
			wall_remove_side(sp, side);

	// Find out what this segment was connected to and break those connections at the other end.
	for (side=0; side < MAX_SIDES_PER_SEGMENT; side++)
		if (IS_CHILD(sp->children[side])) {
			segment	*csp;									// the connecting segment
			int		s;

			csp = &Segments[sp->children[side]];
			for (s=0; s<MAX_SIDES_PER_SEGMENT; s++)
				if (csp->children[s] == segnum) {
					csp->children[s] = -1;				// this is the side of connection, break it
					validate_segment_side(csp,s);					// we have converted a connection to a side so validate the segment
					med_propagate_tmaps_to_back_side(csp,s,0);
				}
			Cursegp = csp;
			med_create_new_segment_from_cursegp();
			copy_uvs_seg_to_seg(&New_segment,Cursegp);
		}

	sp->segnum = -1;										// Mark segment as inactive.

	// If deleted segment = marked segment, then say there is no marked segment
	if (sp == Markedsegp)
		Markedsegp = 0;
	
	//	If deleted segment = a Group segment ptr, then wipe it out.
	for (s=0;s<num_groups;s++) 
		if (sp == Groupsegp[s]) 
			Groupsegp[s] = 0;

	// If deleted segment = group segment, wipe it off the group list.
	if (sp->group > -1) 
			delete_segment_from_group(sp-Segments, sp->group);

	// If we deleted something which was not connected to anything, must now select a new current segment.
	if (Cursegp == sp)
		for (s=0; s<MAX_SEGMENTS; s++)
			if ((Segments[s].segnum != -1) && (s!=segnum) ) {
				Cursegp = &Segments[s];
				med_create_new_segment_from_cursegp();
		   	break;
			}

	// If deleted segment contains objects, wipe out all objects
	if (sp->objects != -1) 	{
		for (objnum=sp->objects;objnum!=-1;objnum=Objects[objnum].next) 	{

			//if an object is in the seg, delete it
			//if the object is the player, move to new curseg

			if (objnum == (ConsoleObject-Objects))	{
				compute_segment_center(&ConsoleObject->pos,Cursegp);
				obj_relink(objnum,Cursegp-Segments);
			} else
				obj_delete(objnum);
		}
	}

	// Make sure everything deleted ok...
	Assert( sp->objects==-1 );

	// If we are leaving many holes in Segments or Vertices, then compress mine, because it is inefficient to be that way
//	if ((Highest_segment_index > Num_segments+4) || (Highest_vertex_index > Num_vertices+4*8))
//		med_compress_mine();

	return 0;
}

// ------------------------------------------------------------------------------------------
//	Copy texture maps from sseg to dseg
void copy_tmaps_to_segment(segment *dseg, segment *sseg)
{
	int	s;

	for (s=0; s<MAX_SIDES_PER_SEGMENT; s++) {
		dseg->sides[s].type = sseg->sides[s].type;
		dseg->sides[s].tmap_num = sseg->sides[s].tmap_num;
		dseg->sides[s].tmap_num2 = sseg->sides[s].tmap_num2;
	}

}

// ------------------------------------------------------------------------------------------
// Rotate the segment *seg by the pitch, bank, heading defined by *rot, destructively
// modifying its four free vertices in the global array Vertices.
// It is illegal to rotate a segment which has connectivity != 1.
// Pitch, bank, heading are about the point which is the average of the four points
// forming the side of connection.
// Return value:
//  0 = successful rotation
//  1 = Connectivity makes rotation illegal (connected to 0 or 2+ segments)
//  2 = Rotation causes degeneracy, such as self-intersecting segment.
//	 3 = Unable to rotate because not connected to exactly 1 segment.
int med_rotate_segment(segment *seg, vms_matrix *rotmat)
{
	segment	*destseg;
        int             newside=0,destside,s;
	int		count;
	int		back_side,side_tmaps[MAX_SIDES_PER_SEGMENT];

	// Find side of attachment
	count = 0;
	for (s=0; s<MAX_SIDES_PER_SEGMENT; s++)
		if (IS_CHILD(seg->children[s])) {
			count++;
			newside = s;
		}

	// Return if passed in segment is connected to other than 1 segment.
	if (count != 1)
		return 3;

	destseg = &Segments[seg->children[newside]];

	destside = 0;
	while ((destseg->children[destside] != seg-Segments) && (destside < MAX_SIDES_PER_SEGMENT))
		destside++;
		
	// Before deleting the segment, copy its texture maps to New_segment
	copy_tmaps_to_segment(&New_segment,seg);

	if (Curside == WFRONT)
		Curside = WBACK;

	med_attach_segment_rotated(destseg,&New_segment,destside,AttachSide,rotmat);

	//	Save tmap_num on each side to restore after call to med_propagate_tmaps_to_segments and _back_side
	//	which will change the tmap nums.
	for (s=0; s<MAX_SIDES_PER_SEGMENT; s++)
		side_tmaps[s] = seg->sides[s].tmap_num;

	back_side = Side_opposite[find_connect_side(destseg, seg)];

	med_propagate_tmaps_to_segments(destseg, seg,0);
	med_propagate_tmaps_to_back_side(seg, back_side,0);

	for (s=0; s<MAX_SIDES_PER_SEGMENT; s++)
		if (s != back_side)
			seg->sides[s].tmap_num = side_tmaps[s];

	return	0;
}

// ----------------------------------------------------------------------------------------
int med_rotate_segment_ang(segment *seg, vms_angvec *ang)
{
	vms_matrix	rotmat;

	return med_rotate_segment(seg,vm_angles_2_matrix(&rotmat,ang));
}

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

// ----------------------------------------------------------------------------
//	Compute the sum of the distances between the four pairs of points.
//	The connections are:
//		firstv1 : 0		(firstv1+1)%4 : 1		(firstv1+2)%4 : 2		(firstv1+3)%4 : 3
fix seg_seg_vertex_distsum(segment *seg1, int side1, segment *seg2, int side2, int firstv1)
{
	fix	distsum;
	int	secondv;

	distsum = 0;
	for (secondv=0; secondv<4; secondv++) {
		int			firstv;

		firstv = (4-secondv + (3 - firstv1)) % 4;
		distsum += vm_vec_dist(&Vertices[seg1->verts[Side_to_verts[side1][firstv]]],&Vertices[seg2->verts[Side_to_verts[side2][secondv]]]);
	}

	return distsum;

}

// ----------------------------------------------------------------------------
//	Determine how to connect two segments together with the least amount of twisting.
//	Returns vertex index in 0..3 on first segment.  Assumed ordering of vertices
//	on second segment is 0,1,2,3.
//	So, if return value is 2, connect 2:0 3:1 0:2 1:3.
//	Theory:
//		We select an ordering of vertices for connection.  For the first pair of vertices to be connected,
//		compute the vector.  For the three remaining pairs of vertices, compute the vectors from one vertex
//		to the other.  Compute the dot products of these vectors with the original vector.  Add them up.
//		The close we are to 3, the better fit we have.  Reason:  The largest value for the dot product is
//		1.0, and this occurs for a parallel set of vectors.
int get_index_of_best_fit(segment *seg1, int side1, segment *seg2, int side2)
{
	int	firstv;
	fix	min_distance;
	int	best_index=0;

	min_distance = F1_0*30000;

	for (firstv=0; firstv<4; firstv++) {
		fix t;
		t = seg_seg_vertex_distsum(seg1, side1, seg2, side2, firstv);
		if (t <= min_distance) {
			min_distance = t;
			best_index = firstv;
		}
	}

	return best_index;

}


#define MAX_VALIDATIONS 50

// ----------------------------------------------------------------------------
//	Remap uv coordinates in all sides in segment *sp which have a vertex in vp[4].
//	vp contains absolute vertex indices.
void remap_side_uvs(segment *sp,int *vp)
{
	int	s,i,v;

	for (s=0; s<MAX_SIDES_PER_SEGMENT; s++) {
		for (v=0; v<4; v++)
			for (i=0; i<4; i++)												// scan each vertex in vp[4]
				if (Side_to_verts[s][v] == vp[i]) {
					assign_default_uvs_to_side(sp,s);					// Side s needs to be remapped
					goto next_side;
				}
next_side: ;
	}
}

// ----------------------------------------------------------------------------
//	Assign default uv coordinates to Curside.
void assign_default_uvs_to_curside(void)
{
	assign_default_uvs_to_side(Cursegp, Curside);
}

// ----------------------------------------------------------------------------
//	Assign default uv coordinates to all sides in Curside.
void assign_default_uvs_to_curseg(void)
{
	int	s;

	for (s=0; s<MAX_SIDES_PER_SEGMENT; s++)
		assign_default_uvs_to_side(Cursegp,s);					// Side s needs to be remapped
}

// ----------------------------------------------------------------------------
//	Modify seg2 to share side2 with seg1:side1.  This forms a connection between
//	two segments without creating a new segment.  It modifies seg2 by sharing
//	vertices from seg1.  seg1 is not modified.  Four vertices from seg2 are
//	deleted.
//	Return code:
//		0			joint formed
//		1			-- no, this is legal! -- unable to form joint because one or more vertices of side2 is not free
//		2			unable to form joint because side1 is already used
int med_form_joint(segment *seg1, int side1, segment *seg2, int side2)
{
	const sbyte	*vp1,*vp2;
	int		bfi,v,s,sv,s1,nv;
	int		lost_vertices[4],remap_vertices[4];
	int		validation_list[MAX_VALIDATIONS];

	//	Make sure that neither side is connected.
	if (IS_CHILD(seg1->children[side1]) || IS_CHILD(seg2->children[side2]))
		return 2;

	// Make sure there is no wall there 
	if ((seg1->sides[side1].wall_num != -1) || (seg2->sides[side2].wall_num != -1))
		return 2;

	//	We can form the joint.  Find the best orientation of vertices.
	bfi = get_index_of_best_fit(seg1, side1, seg2, side2);

	vp1 = Side_to_verts[side1];
	vp2 = Side_to_verts[side2];

	//	Make a copy of the list of vertices in seg2 which will be deleted and set the
	//	associated vertex number, so that all occurrences of the vertices can be replaced.
	for (v=0; v<4; v++)
                lost_vertices[v] = seg2->verts[(int) vp2[v]];

	//	Now, for each vertex in lost_vertices, determine which vertex it maps to.
	for (v=0; v<4; v++)
                remap_vertices[3 - ((v + bfi) % 4)] = seg1->verts[(int) vp1[v]];

	// Now, in all segments, replace all occurrences of vertices in lost_vertices with remap_vertices

	// Put the one segment we know are being modified into the validation list.
	// Note: seg1 does not require a full validation, only a validation of the affected side.  Its vertices do not move.
	nv = 1;
	validation_list[0] = seg2 - Segments;

	for (v=0; v<4; v++)
		for (s=0; s<=Highest_segment_index; s++)
			if (Segments[s].segnum != -1)
				for (sv=0; sv<MAX_VERTICES_PER_SEGMENT; sv++)
					if (Segments[s].verts[sv] == lost_vertices[v]) {
						Segments[s].verts[sv] = remap_vertices[v];
						// Add segment to list of segments to be validated.
						for (s1=0; s1<nv; s1++)
							if (validation_list[s1] == s)
								break;
						if (s1 == nv)
							validation_list[nv++] = s;
						Assert(nv < MAX_VALIDATIONS);
					}

	//	Form new connections.
	seg1->children[side1] = seg2 - Segments;
	seg2->children[side2] = seg1 - Segments;

	// validate all segments
	validate_segment_side(seg1,side1);
	for (s=0; s<nv; s++) {
		validate_segment(&Segments[validation_list[s]]);
		remap_side_uvs(&Segments[validation_list[s]],remap_vertices);	// remap uv coordinates on sides which were reshaped (ie, have a vertex in lost_vertices)
		warn_if_concave_segment(&Segments[validation_list[s]]);
	}

	set_vertex_counts();

	return 0;
}

// ----------------------------------------------------------------------------
//	Create a new segment and use it to form a bridge between two existing segments.
//	Specify two segment:side pairs.  If either segment:side is not open (ie, segment->children[side] != -1)
//	then it is not legal to form the brider.
//	Return:
//		0	bridge segment formed
//		1	unable to form bridge because one (or both) of the sides is not open.
//	Note that no new vertices are created by this process.
int med_form_bridge_segment(segment *seg1, int side1, segment *seg2, int side2)
{
	segment		*bs;
	const sbyte		*sv;
	int			v,bfi,i;

	if (IS_CHILD(seg1->children[side1]) || IS_CHILD(seg2->children[side2]))
		return 1;

	bs = &Segments[get_free_segment_number()];

	bs->segnum = bs-Segments;
	bs->objects = -1;

	// Copy vertices from seg2 into last 4 vertices of bridge segment.
	sv = Side_to_verts[side2];
	for (v=0; v<4; v++)
                bs->verts[(3-v)+4] = seg2->verts[(int) sv[v]];

	// Copy vertices from seg1 into first 4 vertices of bridge segment.
	bfi = get_index_of_best_fit(seg1, side1, seg2, side2);

	sv = Side_to_verts[side1];
	for (v=0; v<4; v++)
                bs->verts[(v + bfi) % 4] = seg1->verts[(int) sv[v]];

	// Form connections to children, first initialize all to unconnected.
	for (i=0; i<MAX_SIDES_PER_SEGMENT; i++) {
		bs->children[i] = -1;
		bs->sides[i].wall_num = -1;
	}

	// Now form connections between segments.

	bs->children[AttachSide] = seg1 - Segments;
        bs->children[(int) Side_opposite[AttachSide]] = seg2 - Segments;

	seg1->children[side1] = bs-Segments; //seg2 - Segments;
	seg2->children[side2] = bs-Segments; //seg1 - Segments;

	//	Validate bridge segment, and if degenerate, clean up mess.
	Degenerate_segment_found = 0;

	validate_segment(bs);

	if (Degenerate_segment_found) {
		seg1->children[side1] = -1;
		seg2->children[side2] = -1;
		bs->children[AttachSide] = -1;
                bs->children[(int) Side_opposite[AttachSide]] = -1;
		if (med_delete_segment(bs)) {
			Int3();
		}
		editor_status("Bridge segment would be degenerate, not created.\n");
		return 1;
	} else {
		validate_segment(seg1);	// used to only validate side, but segment does more error checking: ,side1);
		validate_segment(seg2);	// ,side2);
		med_propagate_tmaps_to_segments(seg1,bs,0);

		editor_status("Bridge segment formed.");
		warn_if_concave_segment(bs);
		return 0;
	}
}

// -------------------------------------------------------------------------------
//	Create a segment given center, dimensions, rotation matrix.
//	Note that the created segment will always have planar sides and rectangular cross sections.
//	It will be created with walls on all sides, ie not connected to anything.
void med_create_segment(segment *sp,fix cx, fix cy, fix cz, fix length, fix width, fix height, vms_matrix *mp)
{
	int			i,f;
	vms_vector	v0,v1,cv;
	segment2 *sp2;

	Num_segments++;

	sp->segnum = 1;						// What to put here?  I don't know.
	sp2 = &Segment2s[sp->segnum];

	// Form connections to children, of which it has none.
	for (i=0; i<MAX_SIDES_PER_SEGMENT; i++) {
		sp->children[i] = -1;
//		sp->sides[i].render_flag = 0;
		sp->sides[i].wall_num  = -1;
	}

	sp->group = -1;
	sp2->matcen_num = -1;

	//	Create relative-to-center vertices, which are the rotated points on the box defined by length, width, height
	sp->verts[0] = med_add_vertex(vm_vec_rotate(&v1,vm_vec_make(&v0,+width/2,+height/2,-length/2),mp));
	sp->verts[1] = med_add_vertex(vm_vec_rotate(&v1,vm_vec_make(&v0,+width/2,-height/2,-length/2),mp));
	sp->verts[2] = med_add_vertex(vm_vec_rotate(&v1,vm_vec_make(&v0,-width/2,-height/2,-length/2),mp));
	sp->verts[3] = med_add_vertex(vm_vec_rotate(&v1,vm_vec_make(&v0,-width/2,+height/2,-length/2),mp));
	sp->verts[4] = med_add_vertex(vm_vec_rotate(&v1,vm_vec_make(&v0,+width/2,+height/2,+length/2),mp));
	sp->verts[5] = med_add_vertex(vm_vec_rotate(&v1,vm_vec_make(&v0,+width/2,-height/2,+length/2),mp));
	sp->verts[6] = med_add_vertex(vm_vec_rotate(&v1,vm_vec_make(&v0,-width/2,-height/2,+length/2),mp));
	sp->verts[7] = med_add_vertex(vm_vec_rotate(&v1,vm_vec_make(&v0,-width/2,+height/2,+length/2),mp));

	// Now create the vector which is the center of the segment and add that to all vertices.
	while (!vm_vec_make(&cv,cx,cy,cz));

	//	Now, add the center to all vertices, placing the segment in 3 space.
	for (i=0; i<MAX_VERTICES_PER_SEGMENT; i++)
		vm_vec_add(&Vertices[sp->verts[i]],&Vertices[sp->verts[i]],&cv);

	//	Set scale vector.
//	sp->scale.x = width;
//	sp->scale.y = height;
//	sp->scale.z = length;

	//	Add faces to all sides.
	for (f=0; f<MAX_SIDES_PER_SEGMENT; f++)
		create_walls_on_side(sp,f);

	sp->objects = -1;		//no objects in this segment

	// Assume nothing special about this segment
	sp2->special = 0;
	sp2->value = 0;
	sp2->static_light = 0;
	sp2->matcen_num = -1;

	copy_tmaps_to_segment(sp, &New_segment);

	assign_default_uvs_to_segment(sp);
}

// ----------------------------------------------------------------------------------------------
//	Create New_segment using a specified scale factor.
void med_create_new_segment(vms_vector *scale)
{
	int			s,t;
	vms_vector	v0;
	segment		*sp = &New_segment;
	segment2 *sp2;

	fix			length,width,height;

	length = scale->z;
	width = scale->x;
	height = scale->y;

	sp->segnum = 1;						// What to put here?  I don't know.
	sp2 = &Segment2s[sp->segnum];

	//	Create relative-to-center vertices, which are the points on the box defined by length, width, height
	t = Num_vertices;
	sp->verts[0] = med_set_vertex(NEW_SEGMENT_VERTICES+0,vm_vec_make(&v0,+width/2,+height/2,-length/2));
	sp->verts[1] = med_set_vertex(NEW_SEGMENT_VERTICES+1,vm_vec_make(&v0,+width/2,-height/2,-length/2));
	sp->verts[2] = med_set_vertex(NEW_SEGMENT_VERTICES+2,vm_vec_make(&v0,-width/2,-height/2,-length/2));
	sp->verts[3] = med_set_vertex(NEW_SEGMENT_VERTICES+3,vm_vec_make(&v0,-width/2,+height/2,-length/2));
	sp->verts[4] = med_set_vertex(NEW_SEGMENT_VERTICES+4,vm_vec_make(&v0,+width/2,+height/2,+length/2));
	sp->verts[5] = med_set_vertex(NEW_SEGMENT_VERTICES+5,vm_vec_make(&v0,+width/2,-height/2,+length/2));
	sp->verts[6] = med_set_vertex(NEW_SEGMENT_VERTICES+6,vm_vec_make(&v0,-width/2,-height/2,+length/2));
	sp->verts[7] = med_set_vertex(NEW_SEGMENT_VERTICES+7,vm_vec_make(&v0,-width/2,+height/2,+length/2));
	Num_vertices = t;

//	sp->scale = *scale;

	// Form connections to children, of which it has none, init faces and tmaps.
	for (s=0; s<MAX_SIDES_PER_SEGMENT; s++) {
		sp->children[s] = -1;
//		sp->sides[s].render_flag = 0;
		sp->sides[s].wall_num = -1;
		create_walls_on_side(sp,s);
		sp->sides[s].tmap_num = s;					// assign some stupid old tmap to this side.
		sp->sides[s].tmap_num2 = 0;
	}

	Seg_orientation.p = 0;	Seg_orientation.b = 0;	Seg_orientation.h = 0;

	sp->objects = -1;		//no objects in this segment

	assign_default_uvs_to_segment(sp);

	// Assume nothing special about this segment
	sp2->special = 0;
	sp2->value = 0;
	sp2->static_light = 0;
	sp2->matcen_num = -1;
}

// -------------------------------------------------------------------------------
void med_create_new_segment_from_cursegp(void)
{
	vms_vector	scalevec;
	vms_vector	uvec, rvec, fvec;

	med_extract_up_vector_from_segment_side(Cursegp, Curside, &uvec);
	med_extract_right_vector_from_segment_side(Cursegp, Curside, &rvec);
	extract_forward_vector_from_segment(Cursegp, &fvec);

	scalevec.x = vm_vec_mag(&rvec);
	scalevec.y = vm_vec_mag(&uvec);
	scalevec.z = vm_vec_mag(&fvec);

	med_create_new_segment(&scalevec);
}

// -------------------------------------------------------------------------------
//	Initialize all vertices to inactive status.
void init_all_vertices(void)
{
	for (unsigned v=0; v<sizeof(Vertex_active)/sizeof(Vertex_active[0]); v++)
		Vertex_active[v] = 0;

	for (unsigned s=0; s<sizeof(Segments)/sizeof(Segments[0]); s++)
		Segments[s].segnum = -1;

}

// --------------------------------------------------------------------------------------------------
// Copy a segment from *ssp to *dsp.  Do not simply copy the struct.  Use *dsp's vertices, copying in
//	just the values, not the indices.
void med_copy_segment(segment *dsp,segment *ssp)
{
	int	v;
	int	verts_copy[MAX_VERTICES_PER_SEGMENT];

	//	First make a copy of the vertex list.
	for (v=0; v<MAX_VERTICES_PER_SEGMENT; v++)
		verts_copy[v] = dsp->verts[v];

	// Now copy the whole struct.
	*dsp = *ssp;

	// Now restore the vertex indices.
	for (v=0; v<MAX_VERTICES_PER_SEGMENT; v++)
		dsp->verts[v] = verts_copy[v];

	// Now destructively modify the vertex values for all vertex indices.
	for (v=0; v<MAX_VERTICES_PER_SEGMENT; v++)
		Vertices[dsp->verts[v]] = Vertices[ssp->verts[v]];
}

// -----------------------------------------------------------------------------
//	Create coordinate axes in orientation of specified segment, stores vertices at *vp.
void create_coordinate_axes_from_segment(segment *sp,int *vertnums)
{
	vms_matrix	rotmat;
	vms_vector t;

	med_extract_matrix_from_segment(sp,&rotmat);

	compute_segment_center(&Vertices[vertnums[0]],sp);

	t = rotmat.rvec;
	vm_vec_scale(&t,i2f(32));
	vm_vec_add(&Vertices[vertnums[1]],&Vertices[vertnums[0]],&t);

	t = rotmat.uvec;
	vm_vec_scale(&t,i2f(32));
	vm_vec_add(&Vertices[vertnums[2]],&Vertices[vertnums[0]],&t);

	t = rotmat.fvec;
	vm_vec_scale(&t,i2f(32));
	vm_vec_add(&Vertices[vertnums[3]],&Vertices[vertnums[0]],&t);
}

// -----------------------------------------------------------------------------
//	Determine if a segment is concave. Returns true if concave
int check_seg_concavity(segment *s)
{
	int sn,vn;
	vms_vector n0,n1;

	for (sn=0;sn<MAX_SIDES_PER_SEGMENT;sn++)
		for (vn=0;vn<=4;vn++) {

			vm_vec_normal(&n1,
				&Vertices[s->verts[Side_to_verts[sn][vn%4]]],
				&Vertices[s->verts[Side_to_verts[sn][(vn+1)%4]]],
				&Vertices[s->verts[Side_to_verts[sn][(vn+2)%4]]]);

			//vm_vec_normalize(&n1);

			if (vn>0) if (vm_vec_dotprod(&n0,&n1) < f0_5) return 1;

			n0 = n1;
		}

	return 0;
}


// -----------------------------------------------------------------------------
//	Find all concave segments and add to list
void find_concave_segs()
{
	int i;
	segment *s;

	N_warning_segs = 0;

	for (s=Segments,i=Highest_segment_index;i>=0;s++,i--)
		if (s->segnum != -1)
			if (check_seg_concavity(s)) Warning_segs[N_warning_segs++]=SEG_PTR_2_NUM(s);


}


// -----------------------------------------------------------------------------
void warn_if_concave_segments(void)
{
	char temp[1];

	find_concave_segs();

	if (N_warning_segs) {
		editor_status_fmt("*** WARNING *** %d concave segments in mine! *** WARNING ***",N_warning_segs);
		sprintf( temp, "%d", N_warning_segs );
    }
}

// -----------------------------------------------------------------------------
//	Check segment s, if concave, warn
void warn_if_concave_segment(segment *s)
{
    char temp[1];
	int	result;

	result = check_seg_concavity(s);

	if (result) {
		Warning_segs[N_warning_segs++] = s-Segments;

        if (N_warning_segs) {
			editor_status("*** WARNING *** New segment is concave! *** WARNING ***");
            sprintf( temp, "%d", N_warning_segs );
        }
        //else
           // editor_status("");
	} //else
        //editor_status("");
}


// -------------------------------------------------------------------------------
//	Find segment adjacent to sp:side.
//	Adjacent means a segment which shares all four vertices.
//	Return true if segment found and fill in segment in adj_sp and side in adj_side.
//	Return false if unable to find, in which case adj_sp and adj_side are undefined.
int med_find_adjacent_segment_side(segment *sp, int side, segment **adj_sp, int *adj_side)
{
	int			seg,s,v,vv;
	int			abs_verts[4];

	//	Stuff abs_verts[4] array with absolute vertex indices
	for (v=0; v<4; v++)
		abs_verts[v] = sp->verts[Side_to_verts[side][v]];

	//	Scan all segments, looking for a segment which contains the four abs_verts
	for (seg=0; seg<=Highest_segment_index; seg++) {
		if (seg != sp-Segments) {
			for (v=0; v<4; v++) {												// do for each vertex in abs_verts
				for (vv=0; vv<MAX_VERTICES_PER_SEGMENT; vv++)			// do for each vertex in segment
					if (abs_verts[v] == Segments[seg].verts[vv])
						goto fass_found1;											// Current vertex (indexed by v) is present in segment, try next
				goto fass_next_seg;												// This segment doesn't contain the vertex indexed by v
			fass_found1: ;
			}		// end for v

			//	All four vertices in sp:side are present in segment seg.
			//	Determine side and return
			for (s=0; s<MAX_SIDES_PER_SEGMENT; s++) {
				for (v=0; v<4; v++) {
					for (vv=0; vv<4; vv++) {
						if (Segments[seg].verts[Side_to_verts[s][v]] == abs_verts[vv])
							goto fass_found2;
					}
					goto fass_next_side;											// Couldn't find vertex v in current side, so try next side.
				fass_found2: ;
				}
				// Found all four vertices in current side.  We are done!
				*adj_sp = &Segments[seg];
				*adj_side = s;
				return 1;
			fass_next_side: ;
			}
			Assert(0);	// Impossible -- we identified this segment as containing all 4 vertices of side "side", but we couldn't find them.
			return 0;
		fass_next_seg: ;
		}
	}

	return 0;
}


#define JOINT_THRESHOLD	10000*F1_0 		// (Huge threshold)

// -------------------------------------------------------------------------------
//	Find segment closest to sp:side.
//	Return true if segment found and fill in segment in adj_sp and side in adj_side.
//	Return false if unable to find, in which case adj_sp and adj_side are undefined.
int med_find_closest_threshold_segment_side(segment *sp, int side, segment **adj_sp, int *adj_side, fix threshold)
{
	int			seg,s;
	vms_vector  vsc, vtc; 		// original segment center, test segment center
	fix			current_dist, closest_seg_dist;

	if (IS_CHILD(sp->children[side]))
		return 0;

	compute_center_point_on_side(&vsc, sp, side); 

	closest_seg_dist = JOINT_THRESHOLD;

	//	Scan all segments, looking for a segment which contains the four abs_verts
	for (seg=0; seg<=Highest_segment_index; seg++) 
		if (seg != sp-Segments) 
			for (s=0;s<MAX_SIDES_PER_SEGMENT;s++) {
				if (!IS_CHILD(Segments[seg].children[s])) {
					compute_center_point_on_side(&vtc, &Segments[seg], s); 
					current_dist = vm_vec_dist( &vsc, &vtc );
					if (current_dist < closest_seg_dist) {
						*adj_sp = &Segments[seg];
						*adj_side = s;
						closest_seg_dist = current_dist;
					}
				}
			}	

	if (closest_seg_dist < threshold)
		return 1;
	else
		return 0;
}



void med_check_all_vertices()
{
	int		s,v;
	segment	*sp;

	for (s=0; s<Num_segments; s++) {
		sp = &Segments[s];
		if (sp->segnum != -1)
			for (v=0; v<MAX_VERTICES_PER_SEGMENT; v++)
				Assert(sp->verts[v] <= Highest_vertex_index);
					
	}

}

//	-----------------------------------------------------------------------------------------------------
void check_for_overlapping_segment(int segnum)
{
	int	i, v;
	segmasks	masks;
	vms_vector	segcenter;

	compute_segment_center(&segcenter, &Segments[segnum]);

	for (i=0;i<=Highest_segment_index; i++) {
		if (i != segnum) {
			masks = get_seg_masks(&segcenter, i, 0, __FILE__, __LINE__);
			if (masks.centermask == 0) {
				continue;
			}

			for (v=0; v<8; v++) {
				vms_vector	pdel, presult;

				vm_vec_sub(&pdel, &Vertices[Segments[segnum].verts[v]], &segcenter);
				vm_vec_scale_add(&presult, &segcenter, &pdel, (F1_0*15)/16);
				masks = get_seg_masks(&presult, i, 0, __FILE__, __LINE__);
				if (masks.centermask == 0) {
					break;
				}
			}
		}
	}

}

//	-----------------------------------------------------------------------------------------------------
//	Check for overlapping segments.
void check_for_overlapping_segments(void)
{
	int	i;

	med_compress_mine();

	for (i=0; i<=Highest_segment_index; i++) {
		check_for_overlapping_segment(i);
	}
}
