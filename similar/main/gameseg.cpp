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
 * Functions moved from segment.c to make editor separable from game.
 *
 */

#include <algorithm>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>	//	for memset()

#include "u_mem.h"
#include "inferno.h"
#include "game.h"
#include "dxxerror.h"
#include "console.h"
#include "vecmat.h"
#include "gameseg.h"
#include "gameseq.h"
#include "wall.h"
#include "fuelcen.h"
#include "bm.h"
#include "fvi.h"
#include "byteutil.h"
#include "lighting.h"
#include "mission.h"
#ifdef EDITOR
#include "editor/editor.h"
#endif

#include "compiler-range_for.h"
#include "highest_valid.h"

using std::min;

// How far a point can be from a plane, and still be "in" the plane
#define PLANE_DIST_TOLERANCE	250

#if defined(DXX_BUILD_DESCENT_II)
dl_index		Dl_indices[MAX_DL_INDICES];
delta_light Delta_lights[MAX_DELTA_LIGHTS];
int	Num_static_lights;
#endif

// ------------------------------------------------------------------------------------------
// Compute the center point of a side of a segment.
//	The center point is defined to be the average of the 4 points defining the side.
void compute_center_point_on_side(vms_vector &vp,const vcsegptr_t sp,int side)
{
	vm_vec_zero(vp);

	range_for (auto &v, Side_to_verts[side])
		vm_vec_add2(vp,Vertices[sp->verts[v]]);

	vm_vec_scale(vp,F1_0/4);
}

// ------------------------------------------------------------------------------------------
// Compute segment center.
//	The center point is defined to be the average of the 8 points defining the segment.
void compute_segment_center(vms_vector &vp,const vcsegptr_t sp)
{
	vm_vec_zero(vp);

	range_for (auto &v, sp->verts)
		vm_vec_add2(vp,Vertices[v]);

	vm_vec_scale(vp,F1_0/8);
}

// -----------------------------------------------------------------------------
//	Given two segments, return the side index in the connecting segment which connects to the base segment
//	Optimized by MK on 4/21/94 because it is a 2% load.
int_fast32_t find_connect_side(const vcsegptridx_t base_seg, const vcsegptr_t con_seg)
{
	auto b = begin(con_seg->children);
	auto e = end(con_seg->children);
	auto i = std::find(b, e, base_seg);
	if (i != e)
		return std::distance(b, i);
	// legal to return -1, used in object_move_one(), mk, 06/08/94: Assert(0);		// Illegal -- there is no connecting side between these two segments
	return -1;
}

// -----------------------------------------------------------------------------------
//	Given a side, return the number of faces
int get_num_faces(const side *sidep)
{
	switch (sidep->get_type()) {
		case SIDE_IS_QUAD:	
			return 1;	
		case SIDE_IS_TRI_02:
		case SIDE_IS_TRI_13:	
			return 2;	
		default:
			throw side::illegal_type(sidep);
	}
}

// Fill in array with four absolute point numbers for a given side
void get_side_verts(side_vertnum_list_t &vertlist,segnum_t segnum,int sidenum)
{
	const sbyte   *sv = Side_to_verts[sidenum];
	auto &vp = Segments[segnum].verts;

	for (int i=4; i--;)
		vertlist[i] = vp[sv[i]];
}


#ifdef EDITOR
// -----------------------------------------------------------------------------------
//	Create all vertex lists (1 or 2) for faces on a side.
//	Sets:
//		num_faces		number of lists
//		vertices			vertices in all (1 or 2) faces
//	If there is one face, it has 4 vertices.
//	If there are two faces, they both have three vertices, so face #0 is stored in vertices 0,1,2,
//	face #1 is stored in vertices 3,4,5.
// Note: these are not absolute vertex numbers, but are relative to the segment
// Note:  for triagulated sides, the middle vertex of each trianle is the one NOT
//   adjacent on the diagonal edge
void create_all_vertex_lists(int *num_faces, vertex_array_list_t &vertices, segnum_t segnum, int sidenum)
{
	side	*sidep = &Segments[segnum].sides[sidenum];
	const int  *sv = Side_to_verts_int[sidenum];

	Assert((segnum <= Highest_segment_index) && (segnum >= 0));
	Assert((sidenum >= 0) && (sidenum < 6));

	switch (sidep->get_type()) {
		case SIDE_IS_QUAD:

			vertices[0] = sv[0];
			vertices[1] = sv[1];
			vertices[2] = sv[2];
			vertices[3] = sv[3];

			*num_faces = 1;
			break;
		case SIDE_IS_TRI_02:
			*num_faces = 2;

			vertices[0] = sv[0];
			vertices[1] = sv[1];
			vertices[2] = sv[2];

			vertices[3] = sv[2];
			vertices[4] = sv[3];
			vertices[5] = sv[0];

			//IMPORTANT: DON'T CHANGE THIS CODE WITHOUT CHANGING GET_SEG_MASKS()
			//CREATE_ABS_VERTEX_LISTS(), CREATE_ALL_VERTEX_LISTS(), CREATE_ALL_VERTNUM_LISTS()
			break;
		case SIDE_IS_TRI_13:
			*num_faces = 2;

			vertices[0] = sv[3];
			vertices[1] = sv[0];
			vertices[2] = sv[1];

			vertices[3] = sv[1];
			vertices[4] = sv[2];
			vertices[5] = sv[3];

			//IMPORTANT: DON'T CHANGE THIS CODE WITHOUT CHANGING GET_SEG_MASKS()
			//CREATE_ABS_VERTEX_LISTS(), CREATE_ALL_VERTEX_LISTS(), CREATE_ALL_VERTNUM_LISTS()
			break;
		default:
			throw side::illegal_type(&Segments[segnum], sidep);
	}

}
#endif

// -----------------------------------------------------------------------------------
// Like create all vertex lists, but returns the vertnums (relative to
// the side) for each of the faces that make up the side. 
//	If there is one face, it has 4 vertices.
//	If there are two faces, they both have three vertices, so face #0 is stored in vertices 0,1,2,
//	face #1 is stored in vertices 3,4,5.
void create_all_vertnum_lists(int *num_faces, vertex_array_list_t &vertnums, segnum_t segnum, int sidenum)
{
	side	*sidep = &Segments[segnum].sides[sidenum];

	Assert((segnum <= Highest_segment_index) && (segnum >= 0));

	switch (sidep->get_type()) {
		case SIDE_IS_QUAD:

			vertnums[0] = 0;
			vertnums[1] = 1;
			vertnums[2] = 2;
			vertnums[3] = 3;

			*num_faces = 1;
			break;
		case SIDE_IS_TRI_02:
			*num_faces = 2;

			vertnums[0] = 0;
			vertnums[1] = 1;
			vertnums[2] = 2;

			vertnums[3] = 2;
			vertnums[4] = 3;
			vertnums[5] = 0;

			//IMPORTANT: DON'T CHANGE THIS CODE WITHOUT CHANGING GET_SEG_MASKS()
			//CREATE_ABS_VERTEX_LISTS(), CREATE_ALL_VERTEX_LISTS(), CREATE_ALL_VERTNUM_LISTS()
			break;
		case SIDE_IS_TRI_13:
			*num_faces = 2;

			vertnums[0] = 3;
			vertnums[1] = 0;
			vertnums[2] = 1;

			vertnums[3] = 1;
			vertnums[4] = 2;
			vertnums[5] = 3;

			//IMPORTANT: DON'T CHANGE THIS CODE WITHOUT CHANGING GET_SEG_MASKS()
			//CREATE_ABS_VERTEX_LISTS(), CREATE_ALL_VERTEX_LISTS(), CREATE_ALL_VERTNUM_LISTS()
			break;
		default:
			throw side::illegal_type(&Segments[segnum], sidep);
	}

}

// -----
// like create_all_vertex_lists(), but generate absolute point numbers
void create_abs_vertex_lists(int *num_faces, vertex_array_list_t &vertices, segnum_t segnum, int sidenum)
{
	auto &vp = Segments[segnum].verts;
	side	*sidep = &Segments[segnum].sides[sidenum];
	const int  *sv = Side_to_verts_int[sidenum];

	Assert((segnum <= Highest_segment_index) && (segnum >= 0));
	
	switch (sidep->get_type()) {
		case SIDE_IS_QUAD:

			vertices[0] = vp[sv[0]];
			vertices[1] = vp[sv[1]];
			vertices[2] = vp[sv[2]];
			vertices[3] = vp[sv[3]];

			*num_faces = 1;
			break;
		case SIDE_IS_TRI_02:
			*num_faces = 2;

			vertices[0] = vp[sv[0]];
			vertices[1] = vp[sv[1]];
			vertices[2] = vp[sv[2]];

			vertices[3] = vp[sv[2]];
			vertices[4] = vp[sv[3]];
			vertices[5] = vp[sv[0]];

			//IMPORTANT: DON'T CHANGE THIS CODE WITHOUT CHANGING GET_SEG_MASKS(),
			//CREATE_ABS_VERTEX_LISTS(), CREATE_ALL_VERTEX_LISTS(), CREATE_ALL_VERTNUM_LISTS()
			break;
		case SIDE_IS_TRI_13:
			*num_faces = 2;

			vertices[0] = vp[sv[3]];
			vertices[1] = vp[sv[0]];
			vertices[2] = vp[sv[1]];

			vertices[3] = vp[sv[1]];
			vertices[4] = vp[sv[2]];
			vertices[5] = vp[sv[3]];

			//IMPORTANT: DON'T CHANGE THIS CODE WITHOUT CHANGING GET_SEG_MASKS()
			//CREATE_ABS_VERTEX_LISTS(), CREATE_ALL_VERTEX_LISTS(), CREATE_ALL_VERTNUM_LISTS()
			break;
		default:
			throw side::illegal_type(&Segments[segnum], sidep);
	}

}


//returns 3 different bitmasks with info telling if this sphere is in
//this segment.  See segmasks structure for info on fields  
segmasks get_seg_masks(const vms_vector &checkp, segnum_t segnum, fix rad, const char *calling_file, int calling_linenum)
{
	int			sn,facebit,sidebit;
	segmasks		masks;
	int			num_faces;
	vertex_array_list_t vertex_list;

	if (segnum < 0 || segnum > Highest_segment_index)
		Error("segnum == %i (%i) in get_seg_masks() \ncheckp: %i,%i,%i, rad: %i \nfrom file: %s, line: %i \nMission: %s (%i) \nPlease report this bug.\n",segnum,Highest_segment_index,checkp.x,checkp.y,checkp.z,rad,calling_file,calling_linenum, Current_mission_filename, Current_level_num);

	Assert((segnum <= Highest_segment_index) && (segnum >= 0));

	auto seg = &Segments[segnum];

	//check point against each side of segment. return bitmask

	masks.sidemask = masks.facemask = masks.centermask = 0;

	for (sn=0,facebit=sidebit=1;sn<6;sn++,sidebit<<=1) {
		side	*s = &seg->sides[sn];
		int	side_pokes_out;
		int	vertnum;
		
		// Get number of faces on this side, and at vertex_list, store vertices.
		//	If one face, then vertex_list indicates a quadrilateral.
		//	If two faces, then 0,1,2 define one triangle, 3,4,5 define the second.
		create_abs_vertex_lists(&num_faces, vertex_list, segnum, sn);

		//ok...this is important.  If a side has 2 faces, we need to know if
		//those faces form a concave or convex side.  If the side pokes out,
		//then a point is on the back of the side if it is behind BOTH faces,
		//but if the side pokes in, a point is on the back if behind EITHER face.

		if (num_faces==2) {
			fix	dist;
			int	side_count,center_count;

			vertnum = min(vertex_list[0],vertex_list[2]);
			
			
			if (vertex_list[4] < vertex_list[1])
					dist = vm_dist_to_plane(Vertices[vertex_list[4]],s->normals[0],Vertices[vertnum]);
			else
					dist = vm_dist_to_plane(Vertices[vertex_list[1]],s->normals[1],Vertices[vertnum]);

			side_pokes_out = (dist > PLANE_DIST_TOLERANCE);

			side_count = center_count = 0;

			for (int fn=0;fn<2;fn++,facebit<<=1) {

					dist = vm_dist_to_plane(checkp, s->normals[fn], Vertices[vertnum]);

				if (dist < -PLANE_DIST_TOLERANCE)	//in front of face
					center_count++;

				if (dist-rad < -PLANE_DIST_TOLERANCE) {
					masks.facemask |= facebit;
					side_count++;
				}
			}

			if (!side_pokes_out) {		//must be behind both faces

				if (side_count==2)
					masks.sidemask |= sidebit;

				if (center_count==2)
					masks.centermask |= sidebit;

			}
			else {							//must be behind at least one face

				if (side_count)
					masks.sidemask |= sidebit;

				if (center_count)
					masks.centermask |= sidebit;

			}


		}
		else {				//only one face on this side
			fix dist;
			//use lowest point number
			auto b = begin(vertex_list);
			vertnum = *std::min_element(b, std::next(b, 4));

				dist = vm_dist_to_plane(checkp, s->normals[0], Vertices[vertnum]);

	
			if (dist < -PLANE_DIST_TOLERANCE)
				masks.centermask |= sidebit;
	
			if (dist-rad < -PLANE_DIST_TOLERANCE) {
				masks.facemask |= facebit;
				masks.sidemask |= sidebit;
			}

			facebit <<= 2;
		}

	}

	return masks;

}

//this was converted from get_seg_masks()...it fills in an array of 6
//elements for the distace behind each side, or zero if not behind
//only gets centermask, and assumes zero rad
static ubyte get_side_dists(const vms_vector &checkp,const vsegptridx_t segnum,fix *side_dists)
{
	int			sn,facebit,sidebit;
	ubyte			mask;
	int			num_faces;
	vertex_array_list_t vertex_list;
	auto &seg = segnum;

	//check point against each side of segment. return bitmask

	mask = 0;

	for (sn=0,facebit=sidebit=1;sn<6;sn++,sidebit<<=1) {
		side	*s = &seg->sides[sn];
		int	side_pokes_out;
		side_dists[sn] = 0;

		// Get number of faces on this side, and at vertex_list, store vertices.
		//	If one face, then vertex_list indicates a quadrilateral.
		//	If two faces, then 0,1,2 define one triangle, 3,4,5 define the second.
		create_abs_vertex_lists(&num_faces, vertex_list, segnum, sn);

		//ok...this is important.  If a side has 2 faces, we need to know if
		//those faces form a concave or convex side.  If the side pokes out,
		//then a point is on the back of the side if it is behind BOTH faces,
		//but if the side pokes in, a point is on the back if behind EITHER face.

		if (num_faces==2) {
			fix	dist;
			int	center_count;
			int	vertnum;

			vertnum = min(vertex_list[0],vertex_list[2]);


			if (vertex_list[4] < vertex_list[1])
					dist = vm_dist_to_plane(Vertices[vertex_list[4]],s->normals[0],Vertices[vertnum]);
			else
					dist = vm_dist_to_plane(Vertices[vertex_list[1]],s->normals[1],Vertices[vertnum]);

			side_pokes_out = (dist > PLANE_DIST_TOLERANCE);

			center_count = 0;

			for (int fn=0;fn<2;fn++,facebit<<=1) {

					dist = vm_dist_to_plane(checkp, s->normals[fn], Vertices[vertnum]);

				if (dist < -PLANE_DIST_TOLERANCE) {	//in front of face
					center_count++;
					side_dists[sn] += dist;
				}

			}

			if (!side_pokes_out) {		//must be behind both faces

				if (center_count==2) {
					mask |= sidebit;
					side_dists[sn] /= 2;		//get average
				}
					

			}
			else {							//must be behind at least one face

				if (center_count) {
					mask |= sidebit;
					if (center_count==2)
						side_dists[sn] /= 2;		//get average

				}
			}


		}
		else {				//only one face on this side
			fix dist;
			//use lowest point number

			auto b = begin(vertex_list);
			auto vertnum = *std::min_element(b, std::next(b, 4));

				dist = vm_dist_to_plane(checkp, s->normals[0], Vertices[vertnum]);
	
			if (dist < -PLANE_DIST_TOLERANCE) {
				mask |= sidebit;
				side_dists[sn] = dist;
			}
	
			facebit <<= 2;
		}

	}

	return mask;

}

#ifndef NDEBUG
//returns true if errors detected
static int check_norms(segnum_t segnum,int sidenum,int facenum,segnum_t csegnum,int csidenum,int cfacenum)
{
	vms_vector *n0,*n1;

	n0 = &Segments[segnum].sides[sidenum].normals[facenum];
	n1 = &Segments[csegnum].sides[csidenum].normals[cfacenum];

	if (n0->x != -n1->x  ||  n0->y != -n1->y  ||  n0->z != -n1->z)
		return 1;
	else
		return 0;
}

//heavy-duty error checking
int check_segment_connections(void)
{
	int errors=0;

	range_for (auto segnum, highest_valid(Segments))
	{
		auto seg = &Segments[segnum];

		for (int sidenum=0;sidenum<6;sidenum++) {
			int num_faces,con_num_faces;
			vertex_array_list_t vertex_list, con_vertex_list;

			create_abs_vertex_lists(&num_faces, vertex_list, segnum, sidenum);

			segnum_t csegnum = seg->children[sidenum];

			if (csegnum >= 0) {
				auto cseg = &Segments[csegnum];
				auto csidenum = find_connect_side(seg,cseg);

				if (csidenum == -1) {
					errors = 1;
					continue;
				}

				create_abs_vertex_lists(&con_num_faces, con_vertex_list, csegnum, csidenum);

				if (con_num_faces != num_faces) {
					errors = 1;
				}
				else
					if (num_faces == 1) {
						int t;

						for (t=0;t<4 && con_vertex_list[t]!=vertex_list[0];t++);

						if (t==4 ||
							 vertex_list[0] != con_vertex_list[t] ||
							 vertex_list[1] != con_vertex_list[(t+3)%4] ||
							 vertex_list[2] != con_vertex_list[(t+2)%4] ||
							 vertex_list[3] != con_vertex_list[(t+1)%4]) {
							errors = 1;
						}
						else
							errors |= check_norms(segnum,sidenum,0,csegnum,csidenum,0);
	
					}
					else {
	
						if (vertex_list[1] == con_vertex_list[1]) {
		
							if (vertex_list[4] != con_vertex_list[4] ||
								 vertex_list[0] != con_vertex_list[2] ||
								 vertex_list[2] != con_vertex_list[0] ||
								 vertex_list[3] != con_vertex_list[5] ||
								 vertex_list[5] != con_vertex_list[3]) {
								Segments[csegnum].sides[csidenum].set_type(5-Segments[csegnum].sides[csidenum].get_type());
							} else {
								errors |= check_norms(segnum,sidenum,0,csegnum,csidenum,0);
								errors |= check_norms(segnum,sidenum,1,csegnum,csidenum,1);
							}
	
						} else {
		
							if (vertex_list[1] != con_vertex_list[4] ||
								 vertex_list[4] != con_vertex_list[1] ||
								 vertex_list[0] != con_vertex_list[5] ||
								 vertex_list[5] != con_vertex_list[0] ||
								 vertex_list[2] != con_vertex_list[3] ||
								 vertex_list[3] != con_vertex_list[2]) {
								Segments[csegnum].sides[csidenum].set_type(5-Segments[csegnum].sides[csidenum].get_type());
							} else {
								errors |= check_norms(segnum,sidenum,0,csegnum,csidenum,1);
								errors |= check_norms(segnum,sidenum,1,csegnum,csidenum,0);
							}
						}
					}
			}
		}
	}

	return errors;

}
#endif

// Used to become a constant based on editor, but I wanted to be able to set
// this for omega blob find_point_seg calls.
// Would be better to pass a paremeter to the routine...--MK, 01/17/96
#if defined(DXX_BUILD_DESCENT_II) || defined(EDITOR)
int	Doing_lighting_hack_flag=0;
#else
#define Doing_lighting_hack_flag 0
#endif

// figure out what seg the given point is in, tracing through segments
// returns segment number, or -1 if can't find segment
static segptridx_t trace_segs(const vms_vector &p0, const vsegptridx_t oldsegnum, int recursion_count, visited_segment_bitarray_t &visited)
{
	int centermask;
	fix side_dists[6];
	fix biggest_val;
	int sidenum, bit, biggest_side;
	if (recursion_count >= Num_segments) {
		con_printf (CON_DEBUG, "trace_segs: Segment not found");
		return segment_none;
	}
	if (visited [oldsegnum])
		return segment_none;
	visited[oldsegnum] = true;

	centermask = get_side_dists(p0,oldsegnum,side_dists);		//check old segment
	if (centermask == 0) // we are in the old segment
		return oldsegnum; //..say so

	for (;;) {
		auto seg = oldsegnum;
		biggest_side = -1;
		biggest_val = 0;
		for (sidenum = 0, bit = 1; sidenum < 6; sidenum++, bit <<= 1)
			if ((centermask & bit) && IS_CHILD(seg->children[sidenum])
			    && side_dists[sidenum] < biggest_val) {
				biggest_val = side_dists[sidenum];
				biggest_side = sidenum;
			}

			if (biggest_side == -1)
				break;

			side_dists[biggest_side] = 0;
			// trace into adjacent segment:
			segnum_t check = trace_segs(p0, seg->children[biggest_side], recursion_count + 1, visited);
			if (check != segment_none)		//we've found a segment
				return check;
	}
	return segment_none;		//we haven't found a segment
}

//Tries to find a segment for a point, in the following way:
// 1. Check the given segment
// 2. Recursively trace through attached segments
// 3. Check all the segmentns
//Returns segnum if found, or -1
segptridx_t find_point_seg(const vms_vector &p,const segptridx_t segnum)
{
	//allow segnum==-1, meaning we have no idea what segment point is in
	Assert((segnum <= Highest_segment_index) && (segnum >= segment_none));

	if (segnum != segment_none) {
		visited_segment_bitarray_t visited;
		auto newseg = trace_segs(p, segnum, 0, visited);

		if (newseg != segment_none)			//we found a segment!
			return newseg;
	}

	//couldn't find via attached segs, so search all segs

	//	MK: 10/15/94
	//	This Doing_lighting_hack_flag thing added by mk because the hundreds of scrolling messages were
	//	slowing down lighting, and in about 98% of cases, it would just return -1 anyway.
	//	Matt: This really should be fixed, though.  We're probably screwing up our lighting in a few places.
	if (!Doing_lighting_hack_flag) {
		range_for (auto newseg, highest_valid(Segments))
			if (get_seg_masks(p, newseg, 0, __FILE__, __LINE__).centermask == 0)
				return newseg;

		return segment_none;		//no segment found
	} else
		return segment_none;
}


//--repair-- //	------------------------------------------------------------------------------
//--repair-- void clsd_repair_center(int segnum)
//--repair-- {
//--repair-- 	int	sidenum;
//--repair--
//--repair-- 	//	--- Set repair center bit for all repair center segments.
//--repair-- 	if (Segments[segnum].special == SEGMENT_IS_REPAIRCEN) {
//--repair-- 		Lsegments[segnum].special_type |= SS_REPAIR_CENTER;
//--repair-- 		Lsegments[segnum].special_segment = segnum;
//--repair-- 	}
//--repair--
//--repair-- 	//	--- Set repair center bit for all segments adjacent to a repair center.
//--repair-- 	for (sidenum=0; sidenum < MAX_SIDES_PER_SEGMENT; sidenum++) {
//--repair-- 		int	s = Segments[segnum].children[sidenum];
//--repair--
//--repair-- 		if ( (s != -1) && (Segments[s].special==SEGMENT_IS_REPAIRCEN) ) {
//--repair-- 			Lsegments[segnum].special_type |= SS_REPAIR_CENTER;
//--repair-- 			Lsegments[segnum].special_segment = s;
//--repair-- 		}
//--repair-- 	}
//--repair-- }

//--repair-- //	------------------------------------------------------------------------------
//--repair-- //	--- Set destination points for all Materialization centers.
//--repair-- void clsd_materialization_center(int segnum)
//--repair-- {
//--repair-- 	if (Segments[segnum].special == SEGMENT_IS_ROBOTMAKER) {
//--repair--
//--repair-- 	}
//--repair-- }
//--repair--
//--repair-- int	Lsegment_highest_segment_index, Lsegment_highest_vertex_index;
//--repair--
//--repair-- //	------------------------------------------------------------------------------
//--repair-- //	Create data specific to mine which doesn't get written to disk.
//--repair-- //	Highest_segment_index and Highest_object_index must be valid.
//--repair-- //	07/21:	set repair center bit
//--repair-- void create_local_segment_data(void)
//--repair-- {
//--repair-- 	int	segnum;
//--repair--
//--repair-- 	//	--- Initialize all Lsegments.
//--repair-- 	for (segnum=0; segnum <= Highest_segment_index; segnum++) {
//--repair-- 		Lsegments[segnum].special_type = 0;
//--repair-- 		Lsegments[segnum].special_segment = -1;
//--repair-- 	}
//--repair--
//--repair-- 	for (segnum=0; segnum <= Highest_segment_index; segnum++) {
//--repair--
//--repair-- 		clsd_repair_center(segnum);
//--repair-- 		clsd_materialization_center(segnum);
//--repair-- 	
//--repair-- 	}
//--repair--
//--repair-- 	//	Set check variables.
//--repair-- 	//	In main game loop, make sure these are valid, else Lsegments is not valid.
//--repair-- 	Lsegment_highest_segment_index = Highest_segment_index;
//--repair-- 	Lsegment_highest_vertex_index = Highest_vertex_index;
//--repair-- }
//--repair--
//--repair-- //	------------------------------------------------------------------------------------------
//--repair-- //	Sort of makes sure create_local_segment_data has been called for the currently executing mine.
//--repair-- //	It is not failsafe, as you will see if you look at the code.
//--repair-- //	Returns 1 if Lsegments appears valid, 0 if not.
//--repair-- int check_lsegments_validity(void)
//--repair-- {
//--repair-- 	return ((Lsegment_highest_segment_index == Highest_segment_index) && (Lsegment_highest_vertex_index == Highest_vertex_index));
//--repair-- }

#define	MAX_LOC_POINT_SEGS	64

int	Connected_segment_distance;

#if defined(DXX_BUILD_DESCENT_I)
static inline void add_to_fcd_cache(int seg0, int seg1, int depth, fix dist)
{
	(void)(seg0||seg1||depth||dist);
}
#elif defined(DXX_BUILD_DESCENT_II)
#define	MIN_CACHE_FCD_DIST	(F1_0*80)	//	Must be this far apart for cache lookup to succeed.  Recognizes small changes in distance matter at small distances.
#define	MAX_FCD_CACHE	8

struct fcd_data {
	segnum_t	seg0, seg1;
	int csd;
	fix	dist;
};

int	Fcd_index = 0;
fcd_data Fcd_cache[MAX_FCD_CACHE];
fix64	Last_fcd_flush_time;

//	----------------------------------------------------------------------------------------------------------
void flush_fcd_cache(void)
{
	Fcd_index = 0;

	for (int i=0; i<MAX_FCD_CACHE; i++)
		Fcd_cache[i].seg0 = segment_none;
}

//	----------------------------------------------------------------------------------------------------------
static void add_to_fcd_cache(int seg0, int seg1, int depth, fix dist)
{
	if (dist > MIN_CACHE_FCD_DIST) {
		Fcd_cache[Fcd_index].seg0 = seg0;
		Fcd_cache[Fcd_index].seg1 = seg1;
		Fcd_cache[Fcd_index].csd = depth;
		Fcd_cache[Fcd_index].dist = dist;

		Fcd_index++;

		if (Fcd_index >= MAX_FCD_CACHE)
			Fcd_index = 0;
	} else {
		//	If it's in the cache, remove it.
		for (int i=0; i<MAX_FCD_CACHE; i++)
			if (Fcd_cache[i].seg0 == seg0)
				if (Fcd_cache[i].seg1 == seg1) {
					Fcd_cache[Fcd_index].seg0 = segment_none;
					break;
				}
	}

}
#endif

//	----------------------------------------------------------------------------------------------------------
//	Determine whether seg0 and seg1 are reachable in a way that allows sound to pass.
//	Search up to a maximum depth of max_depth.
//	Return the distance.
fix find_connected_distance(const vms_vector &p0, segnum_t seg0, const vms_vector &p1, segnum_t seg1, int max_depth, WALL_IS_DOORWAY_mask_t wid_flag)
{
	segnum_t		cur_seg;
	int		qtail = 0, qhead = 0;
	seg_seg	seg_queue[MAX_SEGMENTS];
	short		depth[MAX_SEGMENTS];
	int		cur_depth;
	int		num_points;
	point_seg	point_segs[MAX_LOC_POINT_SEGS];
	fix		dist;

	//	If > this, will overrun point_segs buffer
#ifdef WINDOWS
	if (max_depth == -1) max_depth = 200;
#endif	

	if (max_depth > MAX_LOC_POINT_SEGS-2) {
		max_depth = MAX_LOC_POINT_SEGS-2;
	}

	if (seg0 == seg1) {
		Connected_segment_distance = 0;
		return vm_vec_dist_quick(p0, p1);
	} else {
		auto conn_side = find_connect_side(&Segments[seg0], &Segments[seg1]);
		if (conn_side != -1) {
#if defined(DXX_BUILD_DESCENT_II)
			if (WALL_IS_DOORWAY(&Segments[seg1], conn_side) & wid_flag)
#endif
			{
				Connected_segment_distance = 1;
				return vm_vec_dist_quick(p0, p1);
			}
		}
	}

#if defined(DXX_BUILD_DESCENT_II)
	//	Periodically flush cache.
	if ((GameTime64 - Last_fcd_flush_time > F1_0*2) || (GameTime64 < Last_fcd_flush_time)) {
		flush_fcd_cache();
		Last_fcd_flush_time = GameTime64;
	}

	//	Can't quickly get distance, so see if in Fcd_cache.
	for (int i=0; i<MAX_FCD_CACHE; i++)
		if ((Fcd_cache[i].seg0 == seg0) && (Fcd_cache[i].seg1 == seg1)) {
			Connected_segment_distance = Fcd_cache[i].csd;
			return Fcd_cache[i].dist;
		}
#endif

	num_points = 0;

	visited_segment_bitarray_t visited;
	memset(depth, 0, sizeof(depth[0]) * (Highest_segment_index+1));

	cur_seg = seg0;
	visited[cur_seg] = true;
	cur_depth = 0;

	while (cur_seg != seg1) {
		segment	*segp = &Segments[cur_seg];

		for (int sidenum = 0; sidenum < MAX_SIDES_PER_SEGMENT; sidenum++) {

			int	snum = sidenum;

			if (WALL_IS_DOORWAY(segp, snum) & wid_flag) {
				segnum_t	this_seg = segp->children[snum];

				if (!visited[this_seg]) {
					seg_queue[qtail].start = cur_seg;
					seg_queue[qtail].end = this_seg;
					visited[this_seg] = true;
					depth[qtail++] = cur_depth+1;
					if (max_depth != -1) {
						if (depth[qtail-1] == max_depth) {
							Connected_segment_distance = 1000;
							add_to_fcd_cache(seg0, seg1, Connected_segment_distance, F1_0*1000);
							return -1;
						}
					} else if (this_seg == seg1) {
						goto fcd_done1;
					}
				}

			}
		}	//	for (sidenum...

		if (qhead >= qtail) {
			Connected_segment_distance = 1000;
			add_to_fcd_cache(seg0, seg1, Connected_segment_distance, F1_0*1000);
			return -1;
		}

		cur_seg = seg_queue[qhead].end;
		cur_depth = depth[qhead];
		qhead++;

fcd_done1: ;
	}	//	while (cur_seg ...

	//	Set qtail to the segment which ends at the goal.
	while (seg_queue[--qtail].end != seg1)
		if (qtail < 0) {
			Connected_segment_distance = 1000;
			add_to_fcd_cache(seg0, seg1, Connected_segment_distance, F1_0*1000);
			return -1;
		}

	while (qtail >= 0) {
		segnum_t	parent_seg, this_seg;

		this_seg = seg_queue[qtail].end;
		parent_seg = seg_queue[qtail].start;
		point_segs[num_points].segnum = this_seg;
		compute_segment_center(point_segs[num_points].point,&Segments[this_seg]);
		num_points++;

		if (parent_seg == seg0)
			break;

		while (seg_queue[--qtail].end != parent_seg)
			Assert(qtail >= 0);
	}

	point_segs[num_points].segnum = seg0;
	compute_segment_center(point_segs[num_points].point,&Segments[seg0]);
	num_points++;

	if (num_points == 1) {
		Connected_segment_distance = num_points;
		return vm_vec_dist_quick(p0, p1);
	} else {
		dist = vm_vec_dist_quick(p1, point_segs[1].point);
		dist += vm_vec_dist_quick(p0, point_segs[num_points-2].point);

		for (int i=1; i<num_points-2; i++) {
			fix	ndist;
			ndist = vm_vec_dist_quick(point_segs[i].point, point_segs[i+1].point);
			dist += ndist;
		}

	}

	Connected_segment_distance = num_points;
	add_to_fcd_cache(seg0, seg1, num_points, dist);

	return dist;

}

static sbyte convert_to_byte(fix f)
{
	if (f >= 0x00010000)
		return MATRIX_MAX;
	else if (f <= -0x00010000)
		return -MATRIX_MAX;
	else
		return f >> MATRIX_PRECISION;
}

#define VEL_PRECISION 12

//	Create a shortpos struct from an object.
//	Extract the matrix into byte values.
//	Create a position relative to vertex 0 with 1/256 normal "fix" precision.
//	Stuff segment in a short.
void create_shortpos(shortpos *spp, const vcobjptr_t objp, int swap_bytes)
{
	// int	segnum;
	sbyte   *sp;

	sp = spp->bytemat;

	*sp++ = convert_to_byte(objp->orient.rvec.x);
	*sp++ = convert_to_byte(objp->orient.uvec.x);
	*sp++ = convert_to_byte(objp->orient.fvec.x);
	*sp++ = convert_to_byte(objp->orient.rvec.y);
	*sp++ = convert_to_byte(objp->orient.uvec.y);
	*sp++ = convert_to_byte(objp->orient.fvec.y);
	*sp++ = convert_to_byte(objp->orient.rvec.z);
	*sp++ = convert_to_byte(objp->orient.uvec.z);
	*sp++ = convert_to_byte(objp->orient.fvec.z);

	spp->xo = (objp->pos.x - Vertices[Segments[objp->segnum].verts[0]].x) >> RELPOS_PRECISION;
	spp->yo = (objp->pos.y - Vertices[Segments[objp->segnum].verts[0]].y) >> RELPOS_PRECISION;
	spp->zo = (objp->pos.z - Vertices[Segments[objp->segnum].verts[0]].z) >> RELPOS_PRECISION;

	spp->segment = objp->segnum;

 	spp->velx = (objp->mtype.phys_info.velocity.x) >> VEL_PRECISION;
	spp->vely = (objp->mtype.phys_info.velocity.y) >> VEL_PRECISION;
	spp->velz = (objp->mtype.phys_info.velocity.z) >> VEL_PRECISION;

// swap the short values for the big-endian machines.

	if (swap_bytes) {
		spp->xo = INTEL_SHORT(spp->xo);
		spp->yo = INTEL_SHORT(spp->yo);
		spp->zo = INTEL_SHORT(spp->zo);
		spp->segment = INTEL_SHORT(spp->segment);
		spp->velx = INTEL_SHORT(spp->velx);
		spp->vely = INTEL_SHORT(spp->vely);
		spp->velz = INTEL_SHORT(spp->velz);
	}
}

void extract_shortpos(const vobjptridx_t objp, shortpos *spp, int swap_bytes)
{
	sbyte   *sp;

	sp = spp->bytemat;

	objp->orient.rvec.x = *sp++ << MATRIX_PRECISION;
	objp->orient.uvec.x = *sp++ << MATRIX_PRECISION;
	objp->orient.fvec.x = *sp++ << MATRIX_PRECISION;
	objp->orient.rvec.y = *sp++ << MATRIX_PRECISION;
	objp->orient.uvec.y = *sp++ << MATRIX_PRECISION;
	objp->orient.fvec.y = *sp++ << MATRIX_PRECISION;
	objp->orient.rvec.z = *sp++ << MATRIX_PRECISION;
	objp->orient.uvec.z = *sp++ << MATRIX_PRECISION;
	objp->orient.fvec.z = *sp++ << MATRIX_PRECISION;

	if (swap_bytes) {
		spp->xo = INTEL_SHORT(spp->xo);
		spp->yo = INTEL_SHORT(spp->yo);
		spp->zo = INTEL_SHORT(spp->zo);
		spp->segment = INTEL_SHORT(spp->segment);
		spp->velx = INTEL_SHORT(spp->velx);
		spp->vely = INTEL_SHORT(spp->vely);
		spp->velz = INTEL_SHORT(spp->velz);
	}

	segnum_t segnum = spp->segment;

	Assert((segnum >= 0) && (segnum <= Highest_segment_index));

	objp->pos.x = (spp->xo << RELPOS_PRECISION) + Vertices[Segments[segnum].verts[0]].x;
	objp->pos.y = (spp->yo << RELPOS_PRECISION) + Vertices[Segments[segnum].verts[0]].y;
	objp->pos.z = (spp->zo << RELPOS_PRECISION) + Vertices[Segments[segnum].verts[0]].z;

	objp->mtype.phys_info.velocity.x = (spp->velx << VEL_PRECISION);
	objp->mtype.phys_info.velocity.y = (spp->vely << VEL_PRECISION);
	objp->mtype.phys_info.velocity.z = (spp->velz << VEL_PRECISION);

	obj_relink(objp, segnum);

}

// create and extract quaternion structure from object data which greatly saves bytes by using quaternion instead or orientation matrix
void create_quaternionpos(quaternionpos * qpp, const vobjptr_t objp, int swap_bytes)
{
	vms_quaternion_from_matrix(&qpp->orient, &objp->orient);

	qpp->pos = objp->pos;
	qpp->segment = objp->segnum;
	qpp->vel = objp->mtype.phys_info.velocity;
	qpp->rotvel = objp->mtype.phys_info.rotvel;

	if (swap_bytes)
	{
		qpp->orient.w = INTEL_SHORT(qpp->orient.w);
		qpp->orient.x = INTEL_SHORT(qpp->orient.x);
		qpp->orient.y = INTEL_SHORT(qpp->orient.y);
		qpp->orient.z = INTEL_SHORT(qpp->orient.z);
		qpp->pos.x = INTEL_INT(qpp->pos.x);
		qpp->pos.y = INTEL_INT(qpp->pos.y);
		qpp->pos.z = INTEL_INT(qpp->pos.z);
		qpp->vel.x = INTEL_INT(qpp->vel.x);
		qpp->vel.y = INTEL_INT(qpp->vel.y);
		qpp->vel.z = INTEL_INT(qpp->vel.z);
		qpp->rotvel.x = INTEL_INT(qpp->rotvel.x);
		qpp->rotvel.y = INTEL_INT(qpp->rotvel.y);
		qpp->rotvel.z = INTEL_INT(qpp->rotvel.z);
	}
}

void extract_quaternionpos(const vobjptridx_t objp, quaternionpos *qpp, int swap_bytes)
{
	if (swap_bytes)
	{
		qpp->orient.w = INTEL_SHORT(qpp->orient.w);
		qpp->orient.x = INTEL_SHORT(qpp->orient.x);
		qpp->orient.y = INTEL_SHORT(qpp->orient.y);
		qpp->orient.z = INTEL_SHORT(qpp->orient.z);
		qpp->pos.x = INTEL_INT(qpp->pos.x);
		qpp->pos.y = INTEL_INT(qpp->pos.y);
		qpp->pos.z = INTEL_INT(qpp->pos.z);
		qpp->vel.x = INTEL_INT(qpp->vel.x);
		qpp->vel.y = INTEL_INT(qpp->vel.y);
		qpp->vel.z = INTEL_INT(qpp->vel.z);
		qpp->rotvel.x = INTEL_INT(qpp->rotvel.x);
		qpp->rotvel.y = INTEL_INT(qpp->rotvel.y);
		qpp->rotvel.z = INTEL_INT(qpp->rotvel.z);
	}

	vms_matrix_from_quaternion(&objp->orient, &qpp->orient);

	objp->pos = qpp->pos;
	objp->mtype.phys_info.velocity = qpp->vel;
	objp->mtype.phys_info.rotvel = qpp->rotvel;
        
	segnum_t segnum = qpp->segment;
	Assert((segnum >= 0) && (segnum <= Highest_segment_index));
	obj_relink(objp, segnum);
}


//	-----------------------------------------------------------------------------
//	Segment validation functions.
//	Moved from editor to game so we can compute surface normals at load time.
// -------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------
//	Extract a vector from a segment.  The vector goes from the start face to the end face.
//	The point on each face is the average of the four points forming the face.
static void extract_vector_from_segment(const vcsegptr_t sp, vms_vector &vp, int start, int end)
{
	vms_vector	vs,ve;

	vm_vec_zero(vs);
	vm_vec_zero(ve);

	for (int i=0; i<4; i++) {
		vm_vec_add2(vs,Vertices[sp->verts[Side_to_verts[start][i]]]);
		vm_vec_add2(ve,Vertices[sp->verts[Side_to_verts[end][i]]]);
	}
	vm_vec_sub(vp,ve,vs);
	vm_vec_scale(vp,F1_0/4);
}

//create a matrix that describes the orientation of the given segment
void extract_orient_from_segment(vms_matrix *m,const vcsegptr_t seg)
{
	vms_vector fvec,uvec;

	extract_vector_from_segment(seg,fvec,WFRONT,WBACK);
	extract_vector_from_segment(seg,uvec,WBOTTOM,WTOP);

	//vector to matrix does normalizations and orthogonalizations
	vm_vector_2_matrix(*m,fvec,&uvec,nullptr);
}

// ------------------------------------------------------------------------------------------
//	Extract the forward vector from segment *sp, return in *vp.
//	The forward vector is defined to be the vector from the the center of the front face of the segment
// to the center of the back face of the segment.
void extract_forward_vector_from_segment(const vcsegptr_t sp,vms_vector &vp)
{
	extract_vector_from_segment(sp,vp,WFRONT,WBACK);
}

// ------------------------------------------------------------------------------------------
//	Extract the right vector from segment *sp, return in *vp.
//	The forward vector is defined to be the vector from the the center of the left face of the segment
// to the center of the right face of the segment.
void extract_right_vector_from_segment(const vcsegptr_t sp,vms_vector &vp)
{
	extract_vector_from_segment(sp,vp,WLEFT,WRIGHT);
}

// ------------------------------------------------------------------------------------------
//	Extract the up vector from segment *sp, return in *vp.
//	The forward vector is defined to be the vector from the the center of the bottom face of the segment
// to the center of the top face of the segment.
void extract_up_vector_from_segment(const vcsegptr_t sp,vms_vector &vp)
{
	extract_vector_from_segment(sp,vp,WBOTTOM,WTOP);
}

//	----
//	A side is determined to be degenerate if the cross products of 3 consecutive points does not point outward.
static int check_for_degenerate_side(const vcsegptr_t sp, int sidenum)
{
	const sbyte		*vp = Side_to_verts[sidenum];
	vms_vector	vec1, vec2, cross;
	fix			dot;
	int			degeneracy_flag = 0;

	const auto segc = compute_segment_center(sp);
	const auto sidec = compute_center_point_on_side(sp, sidenum);
	const auto vec_to_center = vm_vec_sub(segc, sidec);

	//vm_vec_sub(&vec1, &Vertices[sp->verts[vp[1]]], &Vertices[sp->verts[vp[0]]]);
	//vm_vec_sub(&vec2, &Vertices[sp->verts[vp[2]]], &Vertices[sp->verts[vp[1]]]);
	//vm_vec_normalize(&vec1);
	//vm_vec_normalize(&vec2);
        vm_vec_normalized_dir(vec1, Vertices[sp->verts[(int) vp[1]]], Vertices[sp->verts[(int) vp[0]]]);
        vm_vec_normalized_dir(vec2, Vertices[sp->verts[(int) vp[2]]], Vertices[sp->verts[(int) vp[1]]]);
	vm_vec_cross(cross, vec1, vec2);

	dot = vm_vec_dot(vec_to_center, cross);
	if (dot <= 0)
		degeneracy_flag |= 1;

	//vm_vec_sub(&vec1, &Vertices[sp->verts[vp[2]]], &Vertices[sp->verts[vp[1]]]);
	//vm_vec_sub(&vec2, &Vertices[sp->verts[vp[3]]], &Vertices[sp->verts[vp[2]]]);
	//vm_vec_normalize(&vec1);
	//vm_vec_normalize(&vec2);
        vm_vec_normalized_dir(vec1, Vertices[sp->verts[(int) vp[2]]], Vertices[sp->verts[(int) vp[1]]]);
        vm_vec_normalized_dir(vec2, Vertices[sp->verts[(int) vp[3]]], Vertices[sp->verts[(int) vp[2]]]);
	vm_vec_cross(cross, vec1, vec2);

	dot = vm_vec_dot(vec_to_center, cross);
	if (dot <= 0)
		degeneracy_flag |= 1;

	return degeneracy_flag;

}

//	----
//	See if a segment has gotten turned inside out, or something.
//	If so, set global Degenerate_segment_found and return 1, else return 0.
static int check_for_degenerate_segment(const vcsegptr_t sp)
{
	vms_vector	fvec, rvec, uvec, cross;
	fix			dot;
	int			i, degeneracy_flag = 0;				// degeneracy flag for current segment

	extract_forward_vector_from_segment(sp, fvec);
	extract_right_vector_from_segment(sp, rvec);
	extract_up_vector_from_segment(sp, uvec);

	vm_vec_normalize(fvec);
	vm_vec_normalize(rvec);
	vm_vec_normalize(uvec);

	vm_vec_cross(cross, fvec, rvec);
	dot = vm_vec_dot(cross, uvec);

	if (dot > 0)
		degeneracy_flag = 0;
	else {
		degeneracy_flag = 1;
	}

	//	Now, see if degenerate because of any side.
	for (i=0; i<MAX_SIDES_PER_SEGMENT; i++)
		degeneracy_flag |= check_for_degenerate_side(sp, i);

#ifdef EDITOR
	Degenerate_segment_found |= degeneracy_flag;
#endif

	return degeneracy_flag;

}

static void add_side_as_quad(const vsegptr_t sp, int sidenum, vms_vector *normal)
{
	side	*sidep = &sp->sides[sidenum];

	sidep->set_type(SIDE_IS_QUAD);

	sidep->normals[0] = *normal;
	sidep->normals[1] = *normal;

	//	If there is a connection here, we only formed the faces for the purpose of determining segment boundaries,
	//	so don't generate polys, else they will get rendered.
//	if (sp->children[sidenum] != -1)
//		sidep->render_flag = 0;
//	else
//		sidep->render_flag = 1;

}


// -------------------------------------------------------------------------------
//	Return v0, v1, v2 = 3 vertices with smallest numbers.  If *negate_flag set, then negate normal after computation.
//	Note, you cannot just compute the normal by treating the points in the opposite direction as this introduces
//	small differences between normals which should merely be opposites of each other.
static void get_verts_for_normal(int va, int vb, int vc, int vd, int *v0, int *v1, int *v2, int *v3, int *negate_flag)
{
	int	v[4],w[4];

	//	w is a list that shows how things got scrambled so we know if our normal is pointing backwards
	for (int i=0; i<4; i++)
		w[i] = i;

	v[0] = va;
	v[1] = vb;
	v[2] = vc;
	v[3] = vd;

	for (int i=1; i<4; i++)
		for (int j=0; j<i; j++)
			if (v[j] > v[i]) {
				int	t;
				t = v[j];	v[j] = v[i];	v[i] = t;
				t = w[j];	w[j] = w[i];	w[i] = t;
			}

	Assert((v[0] < v[1]) && (v[1] < v[2]) && (v[2] < v[3]));

	//	Now, if for any w[i] & w[i+1]: w[i+1] = (w[i]+3)%4, then must swap
	*v0 = v[0];
	*v1 = v[1];
	*v2 = v[2];
	*v3 = v[3];

	if ( (((w[0]+3) % 4) == w[1]) || (((w[1]+3) % 4) == w[2]))
		*negate_flag = 1;
	else
		*negate_flag = 0;

}

// -------------------------------------------------------------------------------
static void add_side_as_2_triangles(const vsegptr_t sp, int sidenum)
{
	vms_vector	norm;
	const sbyte       *vs = Side_to_verts[sidenum];
	fix			dot;

	side	*sidep = &sp->sides[sidenum];

	//	Choose how to triangulate.
	//	If a wall, then
	//		Always triangulate so segment is convex.
	//		Use Matt's formula: Na . AD > 0, where ABCD are vertices on side, a is face formed by A,B,C, Na is normal from face a.
	//	If not a wall, then triangulate so whatever is on the other side is triangulated the same (ie, between the same absoluate vertices)
	if (!IS_CHILD(sp->children[sidenum])) {
		vm_vec_normal(norm, Vertices[sp->verts[vs[0]]], Vertices[sp->verts[vs[1]]], Vertices[sp->verts[vs[2]]]);
		const auto vec_13 =	vm_vec_sub(Vertices[sp->verts[vs[3]]], Vertices[sp->verts[vs[1]]]);	//	vector from vertex 1 to vertex 3
		dot = vm_vec_dot(norm, vec_13);

		//	Now, signifiy whether to triangulate from 0:2 or 1:3
		if (dot >= 0)
			sidep->set_type(SIDE_IS_TRI_02);
		else
			sidep->set_type(SIDE_IS_TRI_13);

		//	Now, based on triangulation type, set the normals.
		if (sidep->get_type() == SIDE_IS_TRI_02) {
			vm_vec_normal(norm, Vertices[sp->verts[vs[0]]], Vertices[sp->verts[vs[1]]], Vertices[sp->verts[vs[2]]]);
			sidep->normals[0] = norm;
			vm_vec_normal(norm, Vertices[sp->verts[vs[0]]], Vertices[sp->verts[vs[2]]], Vertices[sp->verts[vs[3]]]);
			sidep->normals[1] = norm;
		} else {
			vm_vec_normal(norm, Vertices[sp->verts[vs[0]]], Vertices[sp->verts[vs[1]]], Vertices[sp->verts[vs[3]]]);
			sidep->normals[0] = norm;
			vm_vec_normal(norm, Vertices[sp->verts[vs[1]]], Vertices[sp->verts[vs[2]]], Vertices[sp->verts[vs[3]]]);
			sidep->normals[1] = norm;
		}
	} else {
		int	i,v[4], vsorted[4];
		int	negate_flag;

		for (i=0; i<4; i++)
			v[i] = sp->verts[vs[i]];

		get_verts_for_normal(v[0], v[1], v[2], v[3], &vsorted[0], &vsorted[1], &vsorted[2], &vsorted[3], &negate_flag);

		if ((vsorted[0] == v[0]) || (vsorted[0] == v[2])) {
			sidep->set_type(SIDE_IS_TRI_02);
			//	Now, get vertices for normal for each triangle based on triangulation type.
			get_verts_for_normal(v[0], v[1], v[2], 32767, &vsorted[0], &vsorted[1], &vsorted[2], &vsorted[3], &negate_flag);
			vm_vec_normal(norm, Vertices[vsorted[0]], Vertices[vsorted[1]], Vertices[vsorted[2]]);
			if (negate_flag)
				vm_vec_negate(norm);
			sidep->normals[0] = norm;

			get_verts_for_normal(v[0], v[2], v[3], 32767, &vsorted[0], &vsorted[1], &vsorted[2], &vsorted[3], &negate_flag);
			vm_vec_normal(norm, Vertices[vsorted[0]], Vertices[vsorted[1]], Vertices[vsorted[2]]);
			if (negate_flag)
				vm_vec_negate(norm);
			sidep->normals[1] = norm;
		} else {
			sidep->set_type(SIDE_IS_TRI_13);
			//	Now, get vertices for normal for each triangle based on triangulation type.
			get_verts_for_normal(v[0], v[1], v[3], 32767, &vsorted[0], &vsorted[1], &vsorted[2], &vsorted[3], &negate_flag);
			vm_vec_normal(norm, Vertices[vsorted[0]], Vertices[vsorted[1]], Vertices[vsorted[2]]);
			if (negate_flag)
				vm_vec_negate(norm);
			sidep->normals[0] = norm;

			get_verts_for_normal(v[1], v[2], v[3], 32767, &vsorted[0], &vsorted[1], &vsorted[2], &vsorted[3], &negate_flag);
			vm_vec_normal(norm, Vertices[vsorted[0]], Vertices[vsorted[1]], Vertices[vsorted[2]]);
			if (negate_flag)
				vm_vec_negate(norm);
			sidep->normals[1] = norm;
		}
	}
}

static int sign(fix v)
{

	if (v > PLANE_DIST_TOLERANCE)
		return 1;
	else if (v < -(PLANE_DIST_TOLERANCE+1))		//neg & pos round differently
		return -1;
	else
		return 0;
}

// -------------------------------------------------------------------------------
void create_walls_on_side(const vsegptridx_t sp, int sidenum)
{
	int	vm0, vm1, vm2, vm3, negate_flag;
	int	v0, v1, v2, v3;
	vms_vector vn;
	fix	dist_to_plane;

	v0 = sp->verts[Side_to_verts[sidenum][0]];
	v1 = sp->verts[Side_to_verts[sidenum][1]];
	v2 = sp->verts[Side_to_verts[sidenum][2]];
	v3 = sp->verts[Side_to_verts[sidenum][3]];

	get_verts_for_normal(v0, v1, v2, v3, &vm0, &vm1, &vm2, &vm3, &negate_flag);

	vm_vec_normal(vn, Vertices[vm0], Vertices[vm1], Vertices[vm2]);
	dist_to_plane = abs(vm_dist_to_plane(Vertices[vm3], vn, Vertices[vm0]));

	if (negate_flag)
		vm_vec_negate(vn);

	if (dist_to_plane <= PLANE_DIST_TOLERANCE)
		add_side_as_quad(sp, sidenum, &vn);
	else {
		add_side_as_2_triangles(sp, sidenum);

		//this code checks to see if we really should be triangulated, and
		//de-triangulates if we shouldn't be.

		{
			int			num_faces;
			vertex_array_list_t vertex_list;
			fix			dist0,dist1;
			int			s0,s1;
			int			vertnum;
			side			*s;

			create_abs_vertex_lists(&num_faces, vertex_list, sp, sidenum);

			Assert(num_faces == 2);

			s = &sp->sides[sidenum];

			vertnum = min(vertex_list[0],vertex_list[2]);

			dist0 = vm_dist_to_plane(Vertices[vertex_list[1]],s->normals[1],Vertices[vertnum]);
			dist1 = vm_dist_to_plane(Vertices[vertex_list[4]],s->normals[0],Vertices[vertnum]);

			s0 = sign(dist0);
			s1 = sign(dist1);

			if (s0==0 || s1==0 || s0!=s1) {
				sp->sides[sidenum].set_type(SIDE_IS_QUAD); 	//detriangulate!
				sp->sides[sidenum].normals[0] = vn;
				sp->sides[sidenum].normals[1] = vn;
			}

		}
	}

}



// -------------------------------------------------------------------------------
static void validate_removable_wall(const vsegptridx_t sp, int sidenum, int tmap_num)
{
	create_walls_on_side(sp, sidenum);

	sp->sides[sidenum].tmap_num = tmap_num;

//	assign_default_uvs_to_side(sp, sidenum);
//	assign_light_to_side(sp, sidenum);
}

// -------------------------------------------------------------------------------
//	Make a just-modified segment side valid.
void validate_segment_side(const vsegptridx_t sp, int sidenum)
{
	if (sp->sides[sidenum].wall_num == wall_none)
		create_walls_on_side(sp, sidenum);
	else
		// create_removable_wall(sp, sidenum, sp->sides[sidenum].tmap_num);
		validate_removable_wall(sp, sidenum, sp->sides[sidenum].tmap_num);

	//	Set render_flag.
	//	If side doesn't have a child, then render wall.  If it does have a child, but there is a temporary
	//	wall there, then do render wall.
//	if (sp->children[sidenum] == -1)
//		sp->sides[sidenum].render_flag = 1;
//	else if (sp->sides[sidenum].wall_num != -1)
//		sp->sides[sidenum].render_flag = 1;
//	else
//		sp->sides[sidenum].render_flag = 0;
}

// -------------------------------------------------------------------------------
//	Make a just-modified segment valid.
//		check all sides to see how many faces they each should have (0,1,2)
//		create new vector normals
void validate_segment(const vsegptridx_t sp)
{
	check_for_degenerate_segment(sp);

	for (int side = 0; side < MAX_SIDES_PER_SEGMENT; side++)
		validate_segment_side(sp, side);

//	assign_default_uvs_to_segment(sp);
}

// -------------------------------------------------------------------------------
//	Validate all segments.
//	Highest_segment_index must be set.
//	For all used segments (number <= Highest_segment_index), segnum field must be != -1.
void validate_segment_all(void)
{
	range_for (auto s, highest_valid(Segments))
		#ifdef EDITOR
		if (Segments[s].segnum != segment_none)
		#endif
			validate_segment(&Segments[s]);

	#ifdef EDITOR
	{
		for (int s=Highest_segment_index+1; s<MAX_SEGMENTS; s++)
			if (Segments[s].segnum != segment_none) {
				Segments[s].segnum = segment_none;
			}
	}
	#endif
}


//	------------------------------------------------------------------------------------------------------
//	Picks a random point in a segment like so:
//		From center, go up to 50% of way towards any of the 8 vertices.
void pick_random_point_in_seg(vms_vector &new_pos, const vcsegptr_t sp)
{
	int			vnum;
	compute_segment_center(new_pos, sp);
	vnum = (d_rand() * MAX_VERTICES_PER_SEGMENT) >> 15;
	auto vec2 = vm_vec_sub(Vertices[sp->verts[vnum]], new_pos);
	vm_vec_scale(vec2, d_rand());          // d_rand() always in 0..1/2
	vm_vec_add2(new_pos, vec2);
}


//	----------------------------------------------------------------------------------------------------------
//	Set the segment depth of all segments from start_seg in *segbuf.
//	Returns maximum depth value.
unsigned set_segment_depths(int start_seg, array<ubyte, MAX_SEGMENTS> *limit, segment_depth_array_t &depth)
{
	int	curseg;
	int	queue[MAX_SEGMENTS];
	int	head, tail;

	head = 0;
	tail = 0;

	visited_segment_bitarray_t visited;

	queue[tail++] = start_seg;
	visited[start_seg] = true;
	depth[start_seg] = 1;

	unsigned parent_depth=0;
	while (head < tail) {
		curseg = queue[head++];
		parent_depth = depth[curseg];

		for (int i=0; i<MAX_SIDES_PER_SEGMENT; i++) {
			segnum_t childnum = Segments[curseg].children[i];
			if (childnum != segment_none && childnum != segment_exit)
				if (!limit || (*limit)[childnum])
					if (!visited[childnum]) {
						visited[childnum] = true;
						depth[childnum] = min(static_cast<unsigned>(std::numeric_limits<segment_depth_array_t::value_type>::max()), parent_depth + 1);
						queue[tail++] = childnum;
					}
		}
	}

	return parent_depth+1;
}

#if defined(DXX_BUILD_DESCENT_II)
//these constants should match the ones in seguvs
#define	LIGHT_DISTANCE_THRESHOLD	(F1_0*80)
#define	Magical_light_constant  (F1_0*16)

//	------------------------------------------------------------------------------------------
//cast static light from a segment to nearby segments
static void apply_light_to_segment(visited_segment_bitarray_t &visited, const vsegptridx_t segp,const vms_vector &segment_center, fix light_intensity,int recursion_depth)
{
	fix			dist_to_rseg;
	segnum_t segnum=segp;

	if (!visited[segnum])
	{
		visited[segnum] = true;
		const auto r_segment_center = compute_segment_center(segp);
		dist_to_rseg = vm_vec_dist_quick(r_segment_center, segment_center);
	
		if (dist_to_rseg <= LIGHT_DISTANCE_THRESHOLD) {
			fix	light_at_point;
			if (dist_to_rseg > F1_0)
				light_at_point = fixdiv(Magical_light_constant, dist_to_rseg);
			else
				light_at_point = Magical_light_constant;
	
			if (light_at_point >= 0) {
				light_at_point = fixmul(light_at_point, light_intensity);
#if 0   // don't see the point, static_light can be greater than F1_0
				if (light_at_point >= F1_0)
					light_at_point = F1_0-1;
				if (light_at_point <= -F1_0)
					light_at_point = -(F1_0-1);
#endif
				segp->static_light += light_at_point;
				if (segp->static_light < 0)	// if it went negative, saturate
					segp->static_light = 0;
			}	//	end if (light_at_point...
		}	//	end if (dist_to_rseg...
	}

	if (recursion_depth < 2)
		for (int sidenum=0; sidenum<6; sidenum++) {
			if (WALL_IS_DOORWAY(segp,sidenum) & WID_RENDPAST_FLAG)
				apply_light_to_segment(visited, &Segments[segp->children[sidenum]],segment_center,light_intensity,recursion_depth+1);
		}

}


//update the static_light field in a segment, which is used for object lighting
//this code is copied from the editor routine calim_process_all_lights()
static void change_segment_light(segnum_t segnum,int sidenum,int dir)
{
	segment *segp = &Segments[segnum];

	if (WALL_IS_DOORWAY(segp, sidenum) & WID_RENDER_FLAG) {
		side	*sidep = &segp->sides[sidenum];
		fix	light_intensity;

		light_intensity = TmapInfo[sidep->tmap_num].lighting + TmapInfo[sidep->tmap_num2 & 0x3fff].lighting;

		light_intensity *= dir;

		if (light_intensity) {
			const auto segment_center = compute_segment_center(segp);
			visited_segment_bitarray_t visited;
			apply_light_to_segment(visited, segp,segment_center,light_intensity,0);
		}
	}

	//this is a horrible hack to get around the horrible hack used to
	//smooth lighting values when an object moves between segments
	old_viewer = NULL;

}

//	------------------------------------------------------------------------------------------
//	dir = +1 -> add light
//	dir = -1 -> subtract light
//	dir = 17 -> add 17x light
//	dir =  0 -> you are dumb
static void change_light(segnum_t segnum, int sidenum, int dir)
{
	for (int i=0; i<Num_static_lights; i++) {
		if ((Dl_indices[i].segnum == segnum) && (Dl_indices[i].sidenum == sidenum)) {
			delta_light	*dlp;
			dlp = &Delta_lights[Dl_indices[i].index];

			for (int j=0; j<Dl_indices[i].count; j++) {
				for (int k=0; k<4; k++) {
					fix	dl,new_l;
					dl = dir * dlp->vert_light[k] * DL_SCALE;
					Assert((dlp->segnum >= 0) && (dlp->segnum <= Highest_segment_index));
					Assert((dlp->sidenum >= 0) && (dlp->sidenum < MAX_SIDES_PER_SEGMENT));
					new_l = (Segments[dlp->segnum].sides[dlp->sidenum].uvls[k].l += dl);
					if (new_l < 0)
						Segments[dlp->segnum].sides[dlp->sidenum].uvls[k].l = 0;
				}
				dlp++;
			}
		}
	}

	//recompute static light for segment
	change_segment_light(segnum,sidenum,dir);
}

//	Subtract light cast by a light source from all surfaces to which it applies light.
//	This is precomputed data, stored at static light application time in the editor (the slow lighting function).
// returns 1 if lights actually subtracted, else 0
int subtract_light(segnum_t segnum, int sidenum)
{
	if (Segments[segnum].light_subtracted & (1 << sidenum)) {
		return 0;
	}

	Segments[segnum].light_subtracted |= (1 << sidenum);
	change_light(segnum, sidenum, -1);
	return 1;
}

//	Add light cast by a light source from all surfaces to which it applies light.
//	This is precomputed data, stored at static light application time in the editor (the slow lighting function).
//	You probably only want to call this after light has been subtracted.
// returns 1 if lights actually added, else 0
int add_light(segnum_t segnum, int sidenum)
{
	if (!(Segments[segnum].light_subtracted & (1 << sidenum))) {
		return 0;
	}

	Segments[segnum].light_subtracted &= ~(1 << sidenum);
	change_light(segnum, sidenum, 1);
	return 1;
}

//	Parse the Light_subtracted array, turning on or off all lights.
void apply_all_changed_light(void)
{
	range_for (auto i, highest_valid(Segments))
	{
		for (int j=0; j<MAX_SIDES_PER_SEGMENT; j++)
			if (Segments[i].light_subtracted & (1 << j))
				change_light(i, j, -1);
	}
}

//@@//	Scans Light_subtracted bit array.
//@@//	For all light sources which have had their light subtracted, adds light back in.
//@@void restore_all_lights_in_mine(void)
//@@{
//@@	int	i, j, k;
//@@
//@@	for (i=0; i<Num_static_lights; i++) {
//@@		int	segnum, sidenum;
//@@		delta_light	*dlp;
//@@
//@@		segnum = Dl_indices[i].segnum;
//@@		sidenum = Dl_indices[i].sidenum;
//@@		if (Light_subtracted[segnum] & (1 << sidenum)) {
//@@			dlp = &Delta_lights[Dl_indices[i].index];
//@@
//@@			Light_subtracted[segnum] &= ~(1 << sidenum);
//@@			for (j=0; j<Dl_indices[i].count; j++) {
//@@				for (k=0; k<4; k++) {
//@@					fix	dl;
//@@					dl = dlp->vert_light[k] * DL_SCALE;
//@@					Assert((dlp->segnum >= 0) && (dlp->segnum <= Highest_segment_index));
//@@					Assert((dlp->sidenum >= 0) && (dlp->sidenum < MAX_SIDES_PER_SEGMENT));
//@@					Segments[dlp->segnum].sides[dlp->sidenum].uvls[k].l += dl;
//@@				}
//@@				dlp++;
//@@			}
//@@		}
//@@	}
//@@}

//	Should call this whenever a new mine gets loaded.
//	More specifically, should call this whenever something global happens
//	to change the status of static light in the mine.
void clear_light_subtracted(void)
{
	range_for (auto i, highest_valid(Segments))
		Segments[i].light_subtracted = 0;

}

#define	AMBIENT_SEGMENT_DEPTH		5

//	-----------------------------------------------------------------------------
//	Do a bfs from segnum, marking slots in marked_segs if the segment is reachable.
static void ambient_mark_bfs(const vsegptr_t segp, segnum_t segnum, visited_segment_multibit_array_t<2> &marked_segs, unsigned depth, uint_fast8_t s2f_bit)
{
	/*
	 * High first, then low: write here.
	 * Low first, then high: safe to write here, but overwritten later by marked_segs value.
	 */
	segp->s2_flags |= s2f_bit;
	marked_segs[segnum] = s2f_bit | marked_segs[segnum];
	if (!depth)
		return;

	for (int i=0; i<MAX_SIDES_PER_SEGMENT; i++) {
		auto	child = Segments[segnum].children[i];

		/*
		 * No explicit check for IS_CHILD.  If !IS_CHILD, then
		 * WALL_IS_DOORWAY never sets WID_RENDPAST_FLAG.
		 */
		if ((WALL_IS_DOORWAY(&Segments[segnum],i) & WID_RENDPAST_FLAG) && !(marked_segs[child] & s2f_bit))
			ambient_mark_bfs(&Segments[child], child, marked_segs, depth-1, s2f_bit);
	}

}

//	-----------------------------------------------------------------------------
//	Indicate all segments which are within audible range of falling water or lava,
//	and so should hear ambient gurgles.
//	Bashes values in Segment2s array.
void set_ambient_sound_flags()
{
	struct sound_flags_t {
		uint_fast8_t texture_flag, sound_flag;
	};
	const sound_flags_t sound_textures[] = {
		{TMI_VOLATILE, S2F_AMBIENT_LAVA},
		{TMI_WATER, S2F_AMBIENT_WATER},
	};
	visited_segment_multibit_array_t<sizeof(sound_textures) / sizeof(sound_textures[0])> marked_segs;

	//	Now, all segments containing ambient lava or water sound makers are flagged.
	//	Additionally flag all segments which are within range of them.
	//	Mark all segments which are sources of the sound.
	range_for (auto i, highest_valid(Segments))
	{
		segment	*segp = &Segments[i];
		range_for (auto &s, sound_textures)
		{
			for (int j=0; j<MAX_SIDES_PER_SEGMENT; j++) {
				side	*sidep = &segp->sides[j];
				uint_fast8_t texture_flags = TmapInfo[sidep->tmap_num].flags | TmapInfo[sidep->tmap_num2 & 0x3fff].flags;
				if (!(texture_flags & s.texture_flag))
					continue;
				if (!IS_CHILD(segp->children[j]) || (sidep->wall_num != wall_none)) {
					ambient_mark_bfs(segp, i, marked_segs, AMBIENT_SEGMENT_DEPTH, s.sound_flag);
					break;
				}
			}
		}
		segp->s2_flags = (segp->s2_flags & ~(S2F_AMBIENT_LAVA | S2F_AMBIENT_WATER)) | marked_segs[i];
	}
}
#endif
