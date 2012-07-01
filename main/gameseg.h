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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Header file for stuff moved from segment.c to gameseg.c.
 *
 */


#ifndef _GAMESEG_H
#define _GAMESEG_H

#include "pstypes.h"
#include "fix.h"
#include "vecmat.h"
#include "segment.h"

//figure out what seg the given point is in, tracing through segments
int get_new_seg(vms_vector *p0,int startseg);

typedef struct segmasks {
   short facemask;     //which faces sphere pokes through (12 bits)
   sbyte sidemask;     //which sides sphere pokes through (6 bits)
   sbyte centermask;   //which sides center point is on back of (6 bits)
} segmasks;

extern int      Highest_vertex_index;                   // Highest index in Vertices and Vertex_active, an efficiency hack
extern int      Highest_segment_index;          // Highest index in Segments, an efficiency hack

extern void compute_center_point_on_side(vms_vector *vp,segment *sp,int side);
extern void compute_segment_center(vms_vector *vp,segment *sp);
extern int find_connect_side(segment *base_seg, segment *con_seg);

// Fill in array with four absolute point numbers for a given side
void get_side_verts(int *vertlist,int segnum,int sidenum);

//      Create all vertex lists (1 or 2) for faces on a side.
//      Sets:
//              num_faces               number of lists
//              vertices                        vertices in all (1 or 2) faces
//      If there is one face, it has 4 vertices.
//      If there are two faces, they both have three vertices, so face #0 is stored in vertices 0,1,2,
//      face #1 is stored in vertices 3,4,5.
// Note: these are not absolute vertex numbers, but are relative to the segment
// Note:  for triagulated sides, the middle vertex of each trianle is the one NOT
//   adjacent on the diagonal edge
extern void create_all_vertex_lists(int *num_faces, int *vertices, int segnum, int sidenum);

//like create_all_vertex_lists(), but generate absolute point numbers
extern void create_abs_vertex_lists(int *num_faces, int *vertices, int segnum, int sidenum, char *calling_file, int calling_linenum);

// -----------------------------------------------------------------------------------
// Like create all vertex lists, but returns the vertnums (relative to
// the side) for each of the faces that make up the side.
//      If there is one face, it has 4 vertices.
//      If there are two faces, they both have three vertices, so face #0 is stored in vertices 0,1,2,
//      face #1 is stored in vertices 3,4,5.
void create_all_vertnum_lists(int *num_faces, int *vertnums, int segnum, int sidenum);

//      Given a side, return the number of faces
extern int get_num_faces(side *sidep);

//returns 3 different bitmasks with info telling if this sphere is in
//this segment.  See segmasks structure for info on fields
segmasks get_seg_masks(const vms_vector *checkp, int segnum, fix rad, char *calling_file, int calling_linenum);

//this macro returns true if the segnum for an object is correct
#define check_obj_seg(obj) (get_seg_masks(&(obj)->pos, (obj)->segnum, 0, __FILE__, __LINE__).centermask == 0)

//Tries to find a segment for a point, in the following way:
// 1. Check the given segment
// 2. Recursively trace through attached segments
// 3. Check all the segmentns
//Returns segnum if found, or -1
int find_point_seg(const vms_vector *p,int segnum);

//--repair-- // Create data specific to segments which does not need to get written to disk.
//--repair-- extern void create_local_segment_data(void);

//      Sort of makes sure create_local_segment_data has been called for the currently executing mine.
//      Returns 1 if Lsegments appears valid, 0 if not.
int check_lsegments_validity(void);

//      ----------------------------------------------------------------------------------------------------------
//      Determine whether seg0 and seg1 are reachable using wid_flag to go through walls.
//      For example, set to WID_RENDPAST_FLAG to see if sound can get from one segment to the other.
//      set to WID_FLY_FLAG to see if a robot could fly from one to the other.
//      Search up to a maximum depth of max_depth.
//      Return the distance.
extern fix find_connected_distance(vms_vector *p0, int seg0, vms_vector *p1, int seg1, int max_depth, int wid_flag);

//create a matrix that describes the orientation of the given segment
extern void extract_orient_from_segment(vms_matrix *m,segment *seg);

//      In segment.c
//      Make a just-modified segment valid.
//              check all sides to see how many faces they each should have (0,1,2)
//              create new vector normals
extern void validate_segment(segment *sp);

extern void validate_segment_all(void);

//      Extract the forward vector from segment *sp, return in *vp.
//      The forward vector is defined to be the vector from the the center of the front face of the segment
// to the center of the back face of the segment.
extern  void extract_forward_vector_from_segment(segment *sp,vms_vector *vp);

//      Extract the right vector from segment *sp, return in *vp.
//      The forward vector is defined to be the vector from the the center of the left face of the segment
// to the center of the right face of the segment.
extern  void extract_right_vector_from_segment(segment *sp,vms_vector *vp);

//      Extract the up vector from segment *sp, return in *vp.
//      The forward vector is defined to be the vector from the the center of the bottom face of the segment
// to the center of the top face of the segment.
extern  void extract_up_vector_from_segment(segment *sp,vms_vector *vp);

extern void create_walls_on_side(segment *sp, int sidenum);

extern void pick_random_point_in_seg(vms_vector *new_pos, int segnum);
extern void validate_segment_side(segment *sp, int sidenum);

#endif


