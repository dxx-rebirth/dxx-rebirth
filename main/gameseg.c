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
 * Functions moved from segment.c to make editor separable from game.
 * 
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>	//	for memset()

#include "inferno.h"
#include "game.h"
#include "error.h"
#include "console.h"
#include "vecmat.h"
#include "gameseg.h"
#include "wall.h"
#include "fuelcen.h"
#include "byteswap.h"
#include "mission.h"

#ifdef RCS
static char rcsid[] = "$Id: gameseg.c,v 1.1.1.1 2006/03/17 19:45:01 zicodxx Exp $";
#endif

// How far a point can be from a plane, and still be "in" the plane
#define PLANE_DIST_TOLERANCE	250

// ------------------------------------------------------------------------------------------
// Compute the center point of a side of a segment.
//	The center point is defined to be the average of the 4 points defining the side.
void compute_center_point_on_side(vms_vector *vp,segment *sp,int side)
{
	int			v;

	vm_vec_zero(vp);

	for (v=0; v<4; v++)
		vm_vec_add2(vp,&Vertices[sp->verts[Side_to_verts[side][v]]]);

	vm_vec_scale(vp,F1_0/4);
}

// ------------------------------------------------------------------------------------------
// Compute segment center.
//	The center point is defined to be the average of the 8 points defining the segment.
void compute_segment_center(vms_vector *vp,segment *sp)
{
	int			v;

	vm_vec_zero(vp);

	for (v=0; v<8; v++)
		vm_vec_add2(vp,&Vertices[sp->verts[v]]);

	vm_vec_scale(vp,F1_0/8);
}

// -----------------------------------------------------------------------------
//	Given two segments, return the side index in the connecting segment which connects to the base segment
//	Optimized by MK on 4/21/94 because it is a 2% load.
int find_connect_side(segment *base_seg, segment *con_seg)
{
	int	s;
	short	base_seg_num = base_seg - Segments;
	short *childs = con_seg->children;

	for (s=0; s<MAX_SIDES_PER_SEGMENT; s++) {
		if (*childs++ == base_seg_num)
			return s;
	}


	// legal to return -1, used in object_move_one(), mk, 06/08/94: Assert(0);		// Illegal -- there is no connecting side between these two segments
	return -1;

}

// -----------------------------------------------------------------------------------
//	Given a side, return the number of faces
int get_num_faces(side *sidep)
{
	switch (sidep->type) {
		case SIDE_IS_QUAD:	return 1;	break;
		case SIDE_IS_TRI_02:
		case SIDE_IS_TRI_13:	return 2;	break;
		default:
			Error("Illegal type = %i\n", sidep->type);
                        return 0;
			break;
	}
}

// Fill in array with four absolute point numbers for a given side
void get_side_verts(int *vertlist,int segnum,int sidenum)
{
	int	i;
	sbyte  *sv = Side_to_verts[sidenum];
	int	*vp = Segments[segnum].verts;

	for (i=4; i--;)
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
void create_all_vertex_lists(int *num_faces, int *vertices, int segnum, int sidenum)
{
	side	*sidep = &Segments[segnum].sides[sidenum];
	int  *sv = Side_to_verts_int[sidenum];

	Assert((segnum <= Highest_segment_index) && (segnum >= 0));
	Assert((sidenum >= 0) && (sidenum < 6));

	switch (sidep->type) {
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
                        Error("Illegal side type(1), type = %i, segment # = %i, side # = %i\n Please report this bug.\n", sidep->type, segnum, sidenum);
			break;
	}

}
#endif

// -----------------------------------------------------------------------------------
// Like create all vertex lists, but returns the vertnums (relative to
// the side) for each of the faces that make up the side.  
//	If there is one face, it has 4 vertices.
//	If there are two faces, they both have three vertices, so face #0 is stored in vertices 0,1,2,
//	face #1 is stored in vertices 3,4,5.
void create_all_vertnum_lists(int *num_faces, int *vertnums, int segnum, int sidenum)
{
	side	*sidep = &Segments[segnum].sides[sidenum];

	Assert((segnum <= Highest_segment_index) && (segnum >= 0));

	switch (sidep->type) {
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
                        Error("Illegal side type(2), type = %i, segment # = %i, side # = %i\n Please report this bug.\n", sidep->type, segnum, sidenum);
			break;
	}

}

// -----
//like create_all_vertex_lists(), but generate absolute point numbers
void create_abs_vertex_lists(int *num_faces, int *vertices, int segnum, int sidenum,  char *calling_file, int calling_linenum)
{
	int	*vp = Segments[segnum].verts;
	side	*sidep = &Segments[segnum].sides[sidenum];
	int  *sv = Side_to_verts_int[sidenum];

	Assert((segnum <= Highest_segment_index) && (segnum >= 0));
	
	switch (sidep->type) {
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
                        Error("Illegal side type(3), type = %i, segment # = %i, side # = %i caller:%s:%i\n Please report this bug.\n", sidep->type, segnum, sidenum ,calling_file,calling_linenum);
			break;
	}

}


//returns 3 different bitmasks with info telling if this sphere is in
//this segment.  See segmasks structure for info on fields   
segmasks get_seg_masks(vms_vector *checkp,int segnum,fix rad,char *calling_file, int calling_linenum)
{
	int			sn,facebit,sidebit;
	segmasks		masks;
	int			num_faces;
	int			vertex_list[6];
	segment		*seg;
	extern int Current_level_num;

	if (segnum < 0 || segnum > Highest_segment_index)
		Error("segnum == %i (%i) in get_seg_masks() \ncheckp: %i,%i,%i, rad: %i \nfrom file: %s, line: %i \nMission: %s (%i) \nPlease report this bug.\n",segnum,Highest_segment_index,checkp->x,checkp->y,checkp->z,rad,calling_file,calling_linenum, Current_mission_filename, Current_level_num);

	Assert((segnum <= Highest_segment_index) && (segnum >= 0));

	seg = &Segments[segnum];

	//check point against each side of segment. return bitmask

	masks.sidemask = masks.facemask = masks.centermask = 0;

	for (sn=0,facebit=sidebit=1;sn<6;sn++,sidebit<<=1) {
		#ifndef COMPACT_SEGS
		side	*s = &seg->sides[sn];
		#endif
		int	side_pokes_out;
		int	vertnum,fn;
		
		// Get number of faces on this side, and at vertex_list, store vertices.
		//	If one face, then vertex_list indicates a quadrilateral.
		//	If two faces, then 0,1,2 define one triangle, 3,4,5 define the second.
                create_abs_vertex_lists( &num_faces, vertex_list, segnum, sn, calling_file, calling_linenum);

		//ok...this is important.  If a side has 2 faces, we need to know if
		//those faces form a concave or convex side.  If the side pokes out,
		//then a point is on the back of the side if it is behind BOTH faces,
		//but if the side pokes in, a point is on the back if behind EITHER face.

		if (num_faces==2) {
			fix	dist;
			int	side_count,center_count;
			#ifdef COMPACT_SEGS
			vms_vector normals[2];
			#endif

			vertnum = min(vertex_list[0],vertex_list[2]);
			
			#ifdef COMPACT_SEGS
			get_side_normals(seg, sn, &normals[0], &normals[1] );
			#endif
			
			if (vertex_list[4] < vertex_list[1])
				#ifdef COMPACT_SEGS
					dist = vm_dist_to_plane(&Vertices[vertex_list[4]],&normals[0],&Vertices[vertnum]);
				#else
					dist = vm_dist_to_plane(&Vertices[vertex_list[4]],&s->normals[0],&Vertices[vertnum]);
				#endif
			else
				#ifdef COMPACT_SEGS
					dist = vm_dist_to_plane(&Vertices[vertex_list[1]],&normals[1],&Vertices[vertnum]);
				#else
					dist = vm_dist_to_plane(&Vertices[vertex_list[1]],&s->normals[1],&Vertices[vertnum]);
				#endif

			side_pokes_out = (dist > PLANE_DIST_TOLERANCE);

			side_count = center_count = 0;

			for (fn=0;fn<2;fn++,facebit<<=1) {

				#ifdef COMPACT_SEGS
					dist = vm_dist_to_plane(checkp, &normals[fn], &Vertices[vertnum]);
				#else
					dist = vm_dist_to_plane(checkp, &s->normals[fn], &Vertices[vertnum]);
				#endif

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
			int i;
			#ifdef COMPACT_SEGS			
			vms_vector normal;
			#endif

			//use lowest point number

			vertnum = vertex_list[0];
			for (i=1;i<4;i++)
				if (vertex_list[i] < vertnum)
					vertnum = vertex_list[i];

			#ifdef COMPACT_SEGS
				get_side_normal(seg, sn, 0, &normal );
				dist = vm_dist_to_plane(checkp, &normal, &Vertices[vertnum]);
			#else
				dist = vm_dist_to_plane(checkp, &s->normals[0], &Vertices[vertnum]);
			#endif

	
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
ubyte get_side_dists(vms_vector *checkp,int segnum,fix *side_dists)
{
	int			sn,facebit,sidebit;
	ubyte			mask;
	int			num_faces;
	int			vertex_list[6];
	segment		*seg;

	Assert((segnum <= Highest_segment_index) && (segnum >= 0));

	seg = &Segments[segnum];

	//check point against each side of segment. return bitmask

	mask = 0;

	for (sn=0,facebit=sidebit=1;sn<6;sn++,sidebit<<=1) {
		#ifndef COMPACT_SEGS
		side	*s = &seg->sides[sn];
		#endif
		int	side_pokes_out;
		int	fn;

		side_dists[sn] = 0;

		// Get number of faces on this side, and at vertex_list, store vertices.
		//	If one face, then vertex_list indicates a quadrilateral.
		//	If two faces, then 0,1,2 define one triangle, 3,4,5 define the second.
                create_abs_vertex_lists( &num_faces, vertex_list, segnum, sn, __FILE__, __LINE__);

		//ok...this is important.  If a side has 2 faces, we need to know if
		//those faces form a concave or convex side.  If the side pokes out,
		//then a point is on the back of the side if it is behind BOTH faces,
		//but if the side pokes in, a point is on the back if behind EITHER face.

		if (num_faces==2) {
			fix	dist;
			int	center_count;
			int	vertnum;
			#ifdef COMPACT_SEGS
			vms_vector normals[2];
			#endif

			vertnum = min(vertex_list[0],vertex_list[2]);

			#ifdef COMPACT_SEGS
			get_side_normals(seg, sn, &normals[0], &normals[1] );
			#endif

			if (vertex_list[4] < vertex_list[1])
				#ifdef COMPACT_SEGS
					dist = vm_dist_to_plane(&Vertices[vertex_list[4]],&normals[0],&Vertices[vertnum]);
				#else
					dist = vm_dist_to_plane(&Vertices[vertex_list[4]],&s->normals[0],&Vertices[vertnum]);
				#endif
			else
				#ifdef COMPACT_SEGS
					dist = vm_dist_to_plane(&Vertices[vertex_list[1]],&normals[1],&Vertices[vertnum]);
				#else
					dist = vm_dist_to_plane(&Vertices[vertex_list[1]],&s->normals[1],&Vertices[vertnum]);
				#endif

			side_pokes_out = (dist > PLANE_DIST_TOLERANCE);

			center_count = 0;

			for (fn=0;fn<2;fn++,facebit<<=1) {

				#ifdef COMPACT_SEGS
					dist = vm_dist_to_plane(checkp, &normals[fn], &Vertices[vertnum]);
				#else
					dist = vm_dist_to_plane(checkp, &s->normals[fn], &Vertices[vertnum]);
				#endif

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
			int i,vertnum;
			#ifdef COMPACT_SEGS			
			vms_vector normal;
			#endif


			//use lowest point number

			vertnum = vertex_list[0];
			for (i=1;i<4;i++)
				if (vertex_list[i] < vertnum)
					vertnum = vertex_list[i];

			#ifdef COMPACT_SEGS
				get_side_normal(seg, sn, 0, &normal );
				dist = vm_dist_to_plane(checkp, &normal, &Vertices[vertnum]);
			#else
				dist = vm_dist_to_plane(checkp, &s->normals[0], &Vertices[vertnum]);
			#endif
	
			if (dist < -PLANE_DIST_TOLERANCE) {
				mask |= sidebit;
				side_dists[sn] = dist;
			}
	
			facebit <<= 2;
		}

	}

	return mask;

}

#if !defined(NDEBUG) || defined(EDITOR)
#ifndef COMPACT_SEGS
//returns true if errors detected
int check_norms(int segnum,int sidenum,int facenum,int csegnum,int csidenum,int cfacenum)
{
	vms_vector *n0,*n1;

	n0 = &Segments[segnum].sides[sidenum].normals[facenum];
	n1 = &Segments[csegnum].sides[csidenum].normals[cfacenum];

	if (n0->x != -n1->x  ||  n0->y != -n1->y  ||  n0->z != -n1->z) {
		return 1;
	}
	else
		return 0;
}

//heavy-duty error checking
int check_segment_connections(void)
{
	int segnum,sidenum;
	int errors=0;

	for (segnum=0;segnum<=Highest_segment_index;segnum++) {
		segment *seg;

		seg = &Segments[segnum];

		for (sidenum=0;sidenum<6;sidenum++) {
			segment *cseg;
			int num_faces,csegnum,csidenum,con_num_faces;
			int vertex_list[6],con_vertex_list[6];

                        create_abs_vertex_lists( &num_faces, vertex_list, segnum, sidenum, __FILE__, __LINE__);

			csegnum = seg->children[sidenum];

			if (csegnum >= 0) {
				cseg = &Segments[csegnum];
				csidenum = find_connect_side(seg,cseg);

				if (csidenum == -1) {
					errors = 1;
					continue;
				}

                                create_abs_vertex_lists( &con_num_faces, con_vertex_list, csegnum, csidenum, __FILE__, __LINE__);

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
								Segments[csegnum].sides[csidenum].type = 5-Segments[csegnum].sides[csidenum].type;
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
								Segments[csegnum].sides[csidenum].type = 5-Segments[csegnum].sides[csidenum].type;
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
#endif

#ifdef EDITOR
int	Doing_lighting_hack_flag=0;
#else
#define Doing_lighting_hack_flag 0
#endif

//figure out what seg the given point is in, tracing through segments
//returns segment number, or -1 if can't find segment
int trace_segs(vms_vector *p0, int oldsegnum, int recursion_count)
{
	int centermask;
	segment *seg;
	fix side_dists[6];
	fix biggest_val;
	int sidenum, bit, check, biggest_side;
	static ubyte visited [MAX_SEGMENTS];

	Assert((oldsegnum <= Highest_segment_index) && (oldsegnum >= 0));

	if (recursion_count >= Num_segments) {
		con_printf (CON_DEBUG, "trace_segs: Segment not found\n");
		return -1;
	}
	if (recursion_count == 0)
		memset (visited, 0, sizeof (visited));
	if (visited [oldsegnum])
		return -1;
	visited [oldsegnum] = 1;

	centermask = get_side_dists(p0,oldsegnum,side_dists);		//check old segment
	if (centermask == 0) // we are in the old segment
		return oldsegnum; //..say so

	for (;;) {
		seg = &Segments[oldsegnum];
		biggest_side = -1;
		biggest_val = 0;
		for (sidenum = 0, bit = 1; sidenum < 6; sidenum++, bit <<= 1)
			if ((centermask & bit) && (seg->children[sidenum] > -1)
			    && side_dists[sidenum] < biggest_val) {
				biggest_val = side_dists[sidenum];
				biggest_side = sidenum;
			}

			if (biggest_side == -1)
				break;

			side_dists[biggest_side] = 0;
			// trace into adjacent segment:
			check = trace_segs(p0, seg->children[biggest_side], recursion_count + 1);
			if (check >= 0)		//we've found a segment
				return check;
	}
	return -1;		//we haven't found a segment
}


int	Exhaustive_count=0, Exhaustive_failed_count=0;

//Tries to find a segment for a point, in the following way:
// 1. Check the given segment
// 2. Recursively trace through attached segments
// 3. Check all the segmentns
//Returns segnum if found, or -1
int find_point_seg(vms_vector *p,int segnum)
{
	int newseg;

	//allow segnum==-1, meaning we have no idea what segment point is in
	Assert((segnum <= Highest_segment_index) && (segnum >= -1));

	if (segnum != -1) {
		newseg = trace_segs(p,segnum,0);

		if (newseg != -1)			//we found a segment!
			return newseg;
	}

	//couldn't find via attached segs, so search all segs

	//	MK: 10/15/94
	//	This Doing_lighting_hack_flag thing added by mk because the hundreds of scrolling messages were
	//	slowing down lighting, and in about 98% of cases, it would just return -1 anyway.
	//	Matt: This really should be fixed, though.  We're probably screwing up our lighting in a few places.
	if (!Doing_lighting_hack_flag) {
		for (newseg=0;newseg <= Highest_segment_index;newseg++)
                        if (get_seg_masks(p,newseg,0,__FILE__,__LINE__).centermask == 0)
				return newseg;
		return -1;		//no segment found
	} else
		return -1;
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

//	----------------------------------------------------------------------------------------------------------
//	Determine whether seg0 and seg1 are reachable in a way that allows sound to pass.
//	Search up to a maximum depth of max_depth.
//	Return the distance.
fix find_connected_distance(vms_vector *p0, int seg0, vms_vector *p1, int seg1, int max_depth, int wid_flag)
{
	int		cur_seg;
	int		sidenum;
	int		qtail = 0, qhead = 0;
	int		i;
	sbyte		visited[MAX_SEGMENTS];
	seg_seg	seg_queue[MAX_SEGMENTS];
	short		depth[MAX_SEGMENTS];
	int		cur_depth;
	int		num_points;
	point_seg	point_segs[MAX_LOC_POINT_SEGS];
	fix		dist;

	//	If > this, will overrun point_segs buffer
	if (max_depth > MAX_LOC_POINT_SEGS-2) {
		max_depth = MAX_LOC_POINT_SEGS-2;
	}

	if (seg0 == seg1) {
		Connected_segment_distance = 0;
		return vm_vec_dist_quick(p0, p1);
	} else if (find_connect_side(&Segments[seg0], &Segments[seg1]) != -1) {
		Connected_segment_distance = 1;
		return vm_vec_dist_quick(p0, p1);
	}

	num_points = 0;

//	for (i=0; i<=Highest_segment_index; i++) {
//		visited[i] = 0;
//		depth[i] = 0;
//	}
memset(visited, 0, Highest_segment_index+1);
memset(depth, 0, Highest_segment_index+1);

	cur_seg = seg0;
	visited[cur_seg] = 1;
	cur_depth = 0;

	while (cur_seg != seg1) {
		segment	*segp = &Segments[cur_seg];

		for (sidenum = 0; sidenum < MAX_SIDES_PER_SEGMENT; sidenum++) {

			int	snum = sidenum;

			if (WALL_IS_DOORWAY(segp, snum) & wid_flag) {
				int	this_seg = segp->children[snum];

				if (!visited[this_seg]) {
					seg_queue[qtail].start = cur_seg;
					seg_queue[qtail].end = this_seg;
					visited[this_seg] = 1;
					depth[qtail++] = cur_depth+1;
					if (max_depth != -1) {
						if (depth[qtail-1] == max_depth) {
							Connected_segment_distance = 1000;
							return -1;
//							seg1 = seg_queue[qtail-1].end;
//							goto fcd_done1;
						}
					} else if (this_seg == seg1) {
						goto fcd_done1;
					}
				}

			}
		}	//	for (sidenum...

		if (qhead >= qtail) {
			Connected_segment_distance = 1000;
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
			return -1;
		}

	while (qtail >= 0) {
		int	parent_seg, this_seg;

		this_seg = seg_queue[qtail].end;
		parent_seg = seg_queue[qtail].start;
		point_segs[num_points].segnum = this_seg;
		compute_segment_center(&point_segs[num_points].point,&Segments[this_seg]);
		num_points++;

		if (parent_seg == seg0)
			break;

		while (seg_queue[--qtail].end != parent_seg)
			Assert(qtail >= 0);
	}

	point_segs[num_points].segnum = seg0;
	compute_segment_center(&point_segs[num_points].point,&Segments[seg0]);
	num_points++;

	//	Compute distance

	if (num_points == 1) {
		Connected_segment_distance = num_points;
		return vm_vec_dist_quick(p0, p1);
	} else {
		dist = vm_vec_dist_quick(p1, &point_segs[1].point);
		dist += vm_vec_dist_quick(p0, &point_segs[num_points-2].point);

		for (i=1; i<num_points-2; i++) {
			fix	ndist;
			ndist = vm_vec_dist_quick(&point_segs[i].point, &point_segs[i+1].point);
			dist += ndist;
		}
	}

	Connected_segment_distance = num_points;
	return dist;

}

sbyte convert_to_byte(fix f)
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
//added/edited 03/05/99 Matt Mueller - newer shorterpos type
//	Create a shortpos struct from an object.
//	Extract the matrix into byte values.
//	Create a position relative to vertex 0 with 1/256 normal "fix" precision.
//	Stuff segment in a short.
void create_shortpos(shortpos *spp, object *objp, int swap_bytes)
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

void extract_shortpos(object *objp, shortpos *spp, int swap_bytes)
{
	int	segnum;
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

	segnum = spp->segment;

	Assert((segnum >= 0) && (segnum <= Highest_segment_index));

	objp->pos.x = (spp->xo << RELPOS_PRECISION) + Vertices[Segments[segnum].verts[0]].x;
	objp->pos.y = (spp->yo << RELPOS_PRECISION) + Vertices[Segments[segnum].verts[0]].y;
	objp->pos.z = (spp->zo << RELPOS_PRECISION) + Vertices[Segments[segnum].verts[0]].z;

	objp->mtype.phys_info.velocity.x = (spp->velx << VEL_PRECISION);
	objp->mtype.phys_info.velocity.y = (spp->vely << VEL_PRECISION);
	objp->mtype.phys_info.velocity.z = (spp->velz << VEL_PRECISION);

	obj_relink(objp-Objects, segnum);
}

void extract_shorterpos(object *objp, shorterpos *spp)
{
	int	segnum;
	sbyte	*sp;

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

	segnum = spp->segment;

	Assert((segnum >= 0) && (segnum <= Highest_segment_index));

	objp->pos.x = (spp->xo << RELPOS_PRECISION) + Vertices[Segments[segnum].verts[0]].x;
	objp->pos.y = (spp->yo << RELPOS_PRECISION) + Vertices[Segments[segnum].verts[0]].y;
	objp->pos.z = (spp->zo << RELPOS_PRECISION) + Vertices[Segments[segnum].verts[0]].z;

//moved 03/05/99 Matt Mueller - up to new extract_shortpos
//	objp->mtype.phys_info.velocity.x = (spp->velx << VEL_PRECISION);
//	objp->mtype.phys_info.velocity.y = (spp->vely << VEL_PRECISION);
//	objp->mtype.phys_info.velocity.z = (spp->velz << VEL_PRECISION);
//end move -MM
	obj_relink(objp-Objects, segnum);
}

//end edit -MM

//--unused-- void test_shortpos(void)
//--unused-- {
//--unused-- 	shortpos	spp;
//--unused-- 
//--unused-- 	create_shortpos(&spp, &Objects[0]);
//--unused-- 	extract_shortpos(&Objects[0], &spp);
//--unused-- 
//--unused-- }

//	-----------------------------------------------------------------------------
//	Segment validation functions.
//	Moved from editor to game so we can compute surface normals at load time.
// -------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------
//	Extract a vector from a segment.  The vector goes from the start face to the end face.
//	The point on each face is the average of the four points forming the face.
void extract_vector_from_segment(segment *sp, vms_vector *vp, int start, int end)
{
	int			i;
	vms_vector	vs,ve;

	vm_vec_zero(&vs);
	vm_vec_zero(&ve);

	for (i=0; i<4; i++) {
		vm_vec_add2(&vs,&Vertices[sp->verts[Side_to_verts[start][i]]]);
		vm_vec_add2(&ve,&Vertices[sp->verts[Side_to_verts[end][i]]]);
	}

	vm_vec_sub(vp,&ve,&vs);
	vm_vec_scale(vp,F1_0/4);

}

//create a matrix that describes the orientation of the given segment
void extract_orient_from_segment(vms_matrix *m,segment *seg)
{
	vms_vector fvec,uvec;

	extract_vector_from_segment(seg,&fvec,WFRONT,WBACK);
	extract_vector_from_segment(seg,&uvec,WBOTTOM,WTOP);

	//vector to matrix does normalizations and orthogonalizations
	vm_vector_2_matrix(m,&fvec,&uvec,NULL);
}

// ------------------------------------------------------------------------------------------
//	Extract the forward vector from segment *sp, return in *vp.
//	The forward vector is defined to be the vector from the the center of the front face of the segment
// to the center of the back face of the segment.
void extract_forward_vector_from_segment(segment *sp,vms_vector *vp)
{
	extract_vector_from_segment(sp,vp,WFRONT,WBACK);
}

// ------------------------------------------------------------------------------------------
//	Extract the right vector from segment *sp, return in *vp.
//	The forward vector is defined to be the vector from the the center of the left face of the segment
// to the center of the right face of the segment.
void extract_right_vector_from_segment(segment *sp,vms_vector *vp)
{
	extract_vector_from_segment(sp,vp,WLEFT,WRIGHT);
}

// ------------------------------------------------------------------------------------------
//	Extract the up vector from segment *sp, return in *vp.
//	The forward vector is defined to be the vector from the the center of the bottom face of the segment
// to the center of the top face of the segment.
void extract_up_vector_from_segment(segment *sp,vms_vector *vp)
{
	extract_vector_from_segment(sp,vp,WBOTTOM,WTOP);
}

//	----
//	A side is determined to be degenerate if the cross products of 3 consecutive points does not point outward.
int check_for_degenerate_side(segment *sp, int sidenum)
{
	sbyte		*vp = Side_to_verts[sidenum];
	vms_vector	vec1, vec2, cross, vec_to_center;
	vms_vector	segc, sidec;
	fix			dot;
	int			degeneracy_flag = 0;

	compute_segment_center(&segc, sp);
	compute_center_point_on_side(&sidec, sp, sidenum);
	vm_vec_sub(&vec_to_center, &segc, &sidec);

	//vm_vec_sub(&vec1, &Vertices[sp->verts[vp[1]]], &Vertices[sp->verts[vp[0]]]);
	//vm_vec_sub(&vec2, &Vertices[sp->verts[vp[2]]], &Vertices[sp->verts[vp[1]]]);
	//vm_vec_normalize(&vec1);
	//vm_vec_normalize(&vec2);
        vm_vec_normalized_dir(&vec1, &Vertices[sp->verts[(int) vp[1]]], &Vertices[sp->verts[(int) vp[0]]]);
        vm_vec_normalized_dir(&vec2, &Vertices[sp->verts[(int) vp[2]]], &Vertices[sp->verts[(int) vp[1]]]);
	vm_vec_cross(&cross, &vec1, &vec2);

	dot = vm_vec_dot(&vec_to_center, &cross);
	if (dot <= 0)
		degeneracy_flag |= 1;

	//vm_vec_sub(&vec1, &Vertices[sp->verts[vp[2]]], &Vertices[sp->verts[vp[1]]]);
	//vm_vec_sub(&vec2, &Vertices[sp->verts[vp[3]]], &Vertices[sp->verts[vp[2]]]);
	//vm_vec_normalize(&vec1);
	//vm_vec_normalize(&vec2);
        vm_vec_normalized_dir(&vec1, &Vertices[sp->verts[(int) vp[2]]], &Vertices[sp->verts[(int) vp[1]]]);
        vm_vec_normalized_dir(&vec2, &Vertices[sp->verts[(int) vp[3]]], &Vertices[sp->verts[(int) vp[2]]]);
	vm_vec_cross(&cross, &vec1, &vec2);

	dot = vm_vec_dot(&vec_to_center, &cross);
	if (dot <= 0)
		degeneracy_flag |= 1;

	return degeneracy_flag;

}

extern int Degenerate_segment_found;
//	----
//	See if a segment has gotten turned inside out, or something.
//	If so, set global Degenerate_segment_found and return 1, else return 0.
int check_for_degenerate_segment(segment *sp)
{
	vms_vector	fvec, rvec, uvec, cross;
	fix			dot;
	int			i, degeneracy_flag = 0;				// degeneracy flag for current segment

	extract_forward_vector_from_segment(sp, &fvec);
	extract_right_vector_from_segment(sp, &rvec);
	extract_up_vector_from_segment(sp, &uvec);

	vm_vec_normalize(&fvec);
	vm_vec_normalize(&rvec);
	vm_vec_normalize(&uvec);

	vm_vec_cross(&cross, &fvec, &rvec);
	dot = vm_vec_dot(&cross, &uvec);

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

void add_side_as_quad(segment *sp, int sidenum, vms_vector *normal)
{
	side	*sidep = &sp->sides[sidenum];

	sidep->type = SIDE_IS_QUAD;

	#ifdef COMPACT_SEGS
		normal = normal;		//avoid compiler warning
	#else
	sidep->normals[0] = *normal;
	sidep->normals[1] = *normal;
	#endif

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
void get_verts_for_normal(int va, int vb, int vc, int vd, int *v0, int *v1, int *v2, int *v3, int *negate_flag)
{
	int	i,j;
	int	v[4],w[4];

	//	w is a list that shows how things got scrambled so we know if our normal is pointing backwards
	for (i=0; i<4; i++)
		w[i] = i;

	v[0] = va;
	v[1] = vb;
	v[2] = vc;
	v[3] = vd;

	for (i=1; i<4; i++)
		for (j=0; j<i; j++)
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
void add_side_as_2_triangles(segment *sp, int sidenum)
{
	vms_vector	norm;
	sbyte			*vs = Side_to_verts[sidenum];
	fix			dot;
	vms_vector	vec_13;		//	vector from vertex 1 to vertex 3

	side	*sidep = &sp->sides[sidenum];

	//	Choose how to triangulate.
	//	If a wall, then
	//		Always triangulate so segment is convex.
	//		Use Matt's formula: Na . AD > 0, where ABCD are vertices on side, a is face formed by A,B,C, Na is normal from face a.
	//	If not a wall, then triangulate so whatever is on the other side is triangulated the same (ie, between the same absoluate vertices)
	if (!IS_CHILD(sp->children[sidenum])) {
		vm_vec_normal(&norm,  &Vertices[sp->verts[vs[0]]], &Vertices[sp->verts[vs[1]]], &Vertices[sp->verts[vs[2]]]);
		vm_vec_sub(&vec_13, &Vertices[sp->verts[vs[3]]], &Vertices[sp->verts[vs[1]]]);
		dot = vm_vec_dot(&norm, &vec_13);

		//	Now, signifiy whether to triangulate from 0:2 or 1:3
		if (dot >= 0)
			sidep->type = SIDE_IS_TRI_02;
		else
			sidep->type = SIDE_IS_TRI_13;

		#ifndef COMPACT_SEGS
		//	Now, based on triangulation type, set the normals.
		if (sidep->type == SIDE_IS_TRI_02) {
			vm_vec_normal(&norm,  &Vertices[sp->verts[(int)vs[0]]], &Vertices[sp->verts[(int)vs[1]]], &Vertices[sp->verts[(int)vs[2]]]);
			sidep->normals[0] = norm;
			vm_vec_normal(&norm, &Vertices[sp->verts[(int)vs[0]]], &Vertices[sp->verts[(int)vs[2]]], &Vertices[sp->verts[(int)vs[3]]]);
			sidep->normals[1] = norm;
		} else {
			vm_vec_normal(&norm, &Vertices[sp->verts[(int)vs[0]]], &Vertices[sp->verts[(int)vs[1]]], &Vertices[sp->verts[(int)vs[3]]]);
			sidep->normals[0] = norm;
			vm_vec_normal(&norm, &Vertices[sp->verts[(int)vs[1]]], &Vertices[sp->verts[(int)vs[2]]], &Vertices[sp->verts[(int)vs[3]]]);
			sidep->normals[1] = norm;
		}
		#endif
	} else {
		int	i,v[4], vsorted[4];
		int	negate_flag;

		for (i=0; i<4; i++)
			v[i] = sp->verts[vs[i]];

		get_verts_for_normal(v[0], v[1], v[2], v[3], &vsorted[0], &vsorted[1], &vsorted[2], &vsorted[3], &negate_flag);

		if ((vsorted[0] == v[0]) || (vsorted[0] == v[2])) {
			sidep->type = SIDE_IS_TRI_02;
			#ifndef COMPACT_SEGS
			//	Now, get vertices for normal for each triangle based on triangulation type.
			get_verts_for_normal(v[0], v[1], v[2], 32767, &vsorted[0], &vsorted[1], &vsorted[2], &vsorted[3], &negate_flag);
			vm_vec_normal(&norm,  &Vertices[vsorted[0]], &Vertices[vsorted[1]], &Vertices[vsorted[2]]);
			if (negate_flag)
				vm_vec_negate(&norm);
			sidep->normals[0] = norm;

			get_verts_for_normal(v[0], v[2], v[3], 32767, &vsorted[0], &vsorted[1], &vsorted[2], &vsorted[3], &negate_flag);
			vm_vec_normal(&norm,  &Vertices[vsorted[0]], &Vertices[vsorted[1]], &Vertices[vsorted[2]]);
			if (negate_flag)
				vm_vec_negate(&norm);
			sidep->normals[1] = norm;
			#endif
		} else {
			sidep->type = SIDE_IS_TRI_13;
			#ifndef COMPACT_SEGS
			//	Now, get vertices for normal for each triangle based on triangulation type.
			get_verts_for_normal(v[0], v[1], v[3], 32767, &vsorted[0], &vsorted[1], &vsorted[2], &vsorted[3], &negate_flag);
			vm_vec_normal(&norm,  &Vertices[vsorted[0]], &Vertices[vsorted[1]], &Vertices[vsorted[2]]);
			if (negate_flag)
				vm_vec_negate(&norm);
			sidep->normals[0] = norm;

			get_verts_for_normal(v[1], v[2], v[3], 32767, &vsorted[0], &vsorted[1], &vsorted[2], &vsorted[3], &negate_flag);
			vm_vec_normal(&norm,  &Vertices[vsorted[0]], &Vertices[vsorted[1]], &Vertices[vsorted[2]]);
			if (negate_flag)
				vm_vec_negate(&norm);
			sidep->normals[1] = norm;
			#endif
		}
	}
}

int sign(fix v)
{

	if (v > PLANE_DIST_TOLERANCE)
		return 1;
	else if (v < -(PLANE_DIST_TOLERANCE+1))		//neg & pos round differently
		return -1;
	else
		return 0;
}

// -------------------------------------------------------------------------------
void create_walls_on_side(segment *sp, int sidenum)
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

	vm_vec_normal(&vn, &Vertices[vm0], &Vertices[vm1], &Vertices[vm2]);
	dist_to_plane = abs(vm_dist_to_plane(&Vertices[vm3], &vn, &Vertices[vm0]));

	if (negate_flag)
		vm_vec_negate(&vn);

	if (dist_to_plane <= PLANE_DIST_TOLERANCE)
		add_side_as_quad(sp, sidenum, &vn);
	else {
		add_side_as_2_triangles(sp, sidenum);

		//this code checks to see if we really should be triangulated, and
		//de-triangulates if we shouldn't be.

		{
			int			num_faces;
			int			vertex_list[6];
			fix			dist0,dist1;
			int			s0,s1;
			int			vertnum;
			side			*s;

                        create_abs_vertex_lists( &num_faces, vertex_list, sp-Segments, sidenum, __FILE__, __LINE__);

			Assert(num_faces == 2);

			s = &sp->sides[sidenum];

			vertnum = min(vertex_list[0],vertex_list[2]);

			#ifdef COMPACT_SEGS
			{
			vms_vector normals[2];
			get_side_normals(sp, sidenum, &normals[0], &normals[1] );
			dist0 = vm_dist_to_plane(&Vertices[vertex_list[1]],&normals[1],&Vertices[vertnum]);
			dist1 = vm_dist_to_plane(&Vertices[vertex_list[4]],&normals[0],&Vertices[vertnum]);
			}
			#else
			dist0 = vm_dist_to_plane(&Vertices[vertex_list[1]],&s->normals[1],&Vertices[vertnum]);
			dist1 = vm_dist_to_plane(&Vertices[vertex_list[4]],&s->normals[0],&Vertices[vertnum]);
			#endif

			s0 = sign(dist0);
			s1 = sign(dist1);

			if (s0==0 || s1==0 || s0!=s1) {
				sp->sides[sidenum].type = SIDE_IS_QUAD; 	//detriangulate!
				#ifndef COMPACT_SEGS
				sp->sides[sidenum].normals[0] = vn;
				sp->sides[sidenum].normals[1] = vn;
				#endif
			}

		}
	}

}


#ifdef COMPACT_SEGS

//#define CACHE_DEBUG 1
#define MAX_CACHE_NORMALS 128
#define CACHE_MASK 127

typedef struct ncache_element {
	short segnum;
	ubyte sidenum;
	vms_vector normals[2];
} ncache_element;

int ncache_initialized = 0;
ncache_element ncache[MAX_CACHE_NORMALS];

#ifdef CACHE_DEBUG
int ncache_counter = 0;
int ncache_hits = 0;
int ncache_misses = 0;
#endif

void ncache_init()
{
	ncache_flush();
	ncache_initialized = 1;
}

void ncache_flush()
{
	int i;
	for (i=0; i<MAX_CACHE_NORMALS; i++ )	{
		ncache[i].segnum = -1;
	}	
}



// -------------------------------------------------------------------------------
int find_ncache_element( int segnum, int sidenum, int face_flags )
{
	uint i;

	if (!ncache_initialized) ncache_init();

	i = ((segnum<<2) ^ sidenum) & CACHE_MASK;

	if ((ncache[i].segnum == segnum) && ((ncache[i].sidenum&0xf)==sidenum) ) 	{
		uint f1;
#ifdef CACHE_DEBUG
		ncache_hits++;
#endif
		f1 = ncache[i].sidenum>>4;
		if ( (f1&face_flags)==face_flags )
			return i;
		if ( f1 & 1 )
			uncached_get_side_normal( &Segments[segnum], sidenum, 1, &ncache[i].normals[1] );
		else
			uncached_get_side_normal( &Segments[segnum], sidenum, 0, &ncache[i].normals[0] );
		ncache[i].sidenum |= face_flags<<4;
		return i;
	}
#ifdef CACHE_DEBUG
	ncache_misses++;
#endif

	switch( face_flags )	{
	case 1:	
		uncached_get_side_normal( &Segments[segnum], sidenum, 0, &ncache[i].normals[0] );
		break;
	case 2:
		uncached_get_side_normal( &Segments[segnum], sidenum, 1, &ncache[i].normals[1] );
		break;
	case 3:
		uncached_get_side_normals(&Segments[segnum], sidenum, &ncache[i].normals[0], &ncache[i].normals[1] );
		break;
	}
	ncache[i].segnum = segnum;
	ncache[i].sidenum = sidenum | (face_flags<<4);
	return i;
}

void get_side_normal(segment *sp, int sidenum, int face_num, vms_vector * vm )
{
	int i;
	i = find_ncache_element( sp - Segments, sidenum, 1 << face_num );
	*vm = ncache[i].normals[face_num];
	if (0) {
		vms_vector tmp;
		uncached_get_side_normal(sp, sidenum, face_num, &tmp );
		Assert( tmp.x == vm->x );
		Assert( tmp.y == vm->y );
		Assert( tmp.z == vm->z );
	}
}

void get_side_normals(segment *sp, int sidenum, vms_vector * vm1, vms_vector * vm2 )
{
	int i;
	i = find_ncache_element( sp - Segments, sidenum, 3 );
	*vm1 = ncache[i].normals[0];
	*vm2 = ncache[i].normals[1];

	if (0) {
		vms_vector tmp;
		uncached_get_side_normal(sp, sidenum, 0, &tmp );
		Assert( tmp.x == vm1->x );
		Assert( tmp.y == vm1->y );
		Assert( tmp.z == vm1->z );
		uncached_get_side_normal(sp, sidenum, 1, &tmp );
		Assert( tmp.x == vm2->x );
		Assert( tmp.y == vm2->y );
		Assert( tmp.z == vm2->z );
	}

}

void uncached_get_side_normal(segment *sp, int sidenum, int face_num, vms_vector * vm )
{
	int	vm0, vm1, vm2, vm3, negate_flag;
	char	*vs = Side_to_verts[sidenum];

	switch( sp->sides[sidenum].type )	{
	case SIDE_IS_QUAD:
		get_verts_for_normal(sp->verts[vs[0]], sp->verts[vs[1]], sp->verts[vs[2]], sp->verts[vs[3]], &vm0, &vm1, &vm2, &vm3, &negate_flag);
		vm_vec_normal(vm, &Vertices[vm0], &Vertices[vm1], &Vertices[vm2]);
		if (negate_flag)
			vm_vec_negate(vm);
		break;
	case SIDE_IS_TRI_02:
		if ( face_num == 0 )
			vm_vec_normal(vm, &Vertices[sp->verts[vs[0]]], &Vertices[sp->verts[vs[1]]], &Vertices[sp->verts[vs[2]]]);
		else
			vm_vec_normal(vm, &Vertices[sp->verts[vs[0]]], &Vertices[sp->verts[vs[2]]], &Vertices[sp->verts[vs[3]]]);
		break;
	case SIDE_IS_TRI_13:
		if ( face_num == 0 )
			vm_vec_normal(vm, &Vertices[sp->verts[vs[0]]], &Vertices[sp->verts[vs[1]]], &Vertices[sp->verts[vs[3]]]);
		else
			vm_vec_normal(vm, &Vertices[sp->verts[vs[1]]], &Vertices[sp->verts[vs[2]]], &Vertices[sp->verts[vs[3]]]);
		break;
	}
}

void uncached_get_side_normals(segment *sp, int sidenum, vms_vector * vm1, vms_vector * vm2 )
{
	int	vvm0, vvm1, vvm2, vvm3, negate_flag;
	char	*vs = Side_to_verts[sidenum];

	switch( sp->sides[sidenum].type )	{
	case SIDE_IS_QUAD:
		get_verts_for_normal(sp->verts[vs[0]], sp->verts[vs[1]], sp->verts[vs[2]], sp->verts[vs[3]], &vvm0, &vvm1, &vvm2, &vvm3, &negate_flag);
		vm_vec_normal(vm1, &Vertices[vvm0], &Vertices[vvm1], &Vertices[vvm2]);
		if (negate_flag)
			vm_vec_negate(vm1);
		*vm2 = *vm1;
		break;
	case SIDE_IS_TRI_02:
		vm_vec_normal(vm1, &Vertices[sp->verts[vs[0]]], &Vertices[sp->verts[vs[1]]], &Vertices[sp->verts[vs[2]]]);
		vm_vec_normal(vm2, &Vertices[sp->verts[vs[0]]], &Vertices[sp->verts[vs[2]]], &Vertices[sp->verts[vs[3]]]);
		break;
	case SIDE_IS_TRI_13:
		vm_vec_normal(vm1, &Vertices[sp->verts[vs[0]]], &Vertices[sp->verts[vs[1]]], &Vertices[sp->verts[vs[3]]]);
		vm_vec_normal(vm2, &Vertices[sp->verts[vs[1]]], &Vertices[sp->verts[vs[2]]], &Vertices[sp->verts[vs[3]]]);
		break;
	}
}

#endif

// -------------------------------------------------------------------------------
void validate_removable_wall(segment *sp, int sidenum, int tmap_num)
{
	create_walls_on_side(sp, sidenum);

	sp->sides[sidenum].tmap_num = tmap_num;

//	assign_default_uvs_to_side(sp, sidenum);
//	assign_light_to_side(sp, sidenum);
}

// -------------------------------------------------------------------------------
//	Make a just-modified segment side valid.
void validate_segment_side(segment *sp, int sidenum)
{
	if (sp->sides[sidenum].wall_num == -1)
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
void validate_segment(segment *sp)
{
	int	side;

	sp->degenerated = check_for_degenerate_segment(sp);

	for (side = 0; side < MAX_SIDES_PER_SEGMENT; side++)
		validate_segment_side(sp, side);

//	assign_default_uvs_to_segment(sp);
}

// -------------------------------------------------------------------------------
//	Validate all segments.
//	Highest_segment_index must be set.
//	For all used segments (number <= Highest_segment_index), segnum field must be != -1.
void validate_segment_all(void)
{
	int	s;

	for (s=0; s<=Highest_segment_index; s++)
		#ifdef EDITOR
		if (Segments[s].segnum != -1)
		#endif
			validate_segment(&Segments[s]);

	#ifdef EDITOR
	{
		for (s=Highest_segment_index+1; s<MAX_SEGMENTS; s++)
			if (Segments[s].segnum != -1) {
				Segments[s].segnum = -1;
			}
	}
	#endif
}


//	------------------------------------------------------------------------------------------------------
//	Picks a random point in a segment like so:
//		From center, go up to 50% of way towards any of the 8 vertices.
void pick_random_point_in_seg(vms_vector *new_pos, int segnum)
{
	int			vnum;
	vms_vector	vec2;

	compute_segment_center(new_pos, &Segments[segnum]);
	vnum = (d_rand() * MAX_VERTICES_PER_SEGMENT) >> 15;
	vm_vec_sub(&vec2, &Vertices[Segments[segnum].verts[vnum]], new_pos);
	vm_vec_scale(&vec2, d_rand());			//	d_rand() always in 0..1/2
	vm_vec_add2(new_pos, &vec2);
}


//	----------------------------------------------------------------------------------------------------------
//	Set the segment depth of all segments from start_seg in *segbuf.
//	Returns maximum depth value.
int set_segment_depths(int start_seg, ubyte *segbuf)
{
	int	i, curseg;
	ubyte	visited[MAX_SEGMENTS];
	int	queue[MAX_SEGMENTS];
	int	head, tail;
	int	depth;
        int     parent_depth=0;

	depth = 1;
	head = 0;
	tail = 0;

	for (i=0; i<=Highest_segment_index; i++)
		visited[i] = 0;

	if (segbuf[start_seg] == 0)
		return 1;

	queue[tail++] = start_seg;
	visited[start_seg] = 1;
	segbuf[start_seg] = depth++;

	if (depth == 0)
		depth = 255;

	while (head < tail) {
		curseg = queue[head++];
		parent_depth = segbuf[curseg];

		for (i=0; i<MAX_SIDES_PER_SEGMENT; i++) {
			int	childnum;

			childnum = Segments[curseg].children[i];
			if (childnum != -1)
				if (segbuf[childnum])
					if (!visited[childnum]) {
						visited[childnum] = 1;
						segbuf[childnum] = parent_depth+1;
						queue[tail++] = childnum;
					}
		}
	}

	return parent_depth+1;
}
