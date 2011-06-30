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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/gameseg.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:45:02 $
 * 
 * Header file for stuff moved from segment.c to gameseg.c.
 * 
 * $Log: gameseg.h,v $
 * Revision 1.1.1.1  2006/03/17 19:45:02  zicodxx
 * initial import
 *
 * Revision 1.3  2000/06/25 08:34:29  sekmu
 * file-line for segfault info
 *
 * Revision 1.2  2000/01/19 04:48:37  sekmu
 * added debug info for illegal side type
 *
 * Revision 1.1.1.1  1999/06/14 22:12:25  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:31:20  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.24  1995/02/01  16:34:03  john
 * Linted.
 * 
 * Revision 1.23  1995/01/16  21:06:36  mike
 * Move function pick_random_point_in_segment from fireball.c to gameseg.c.
 * 
 * Revision 1.22  1994/11/23  12:18:59  mike
 * prototype for level names.
 * 
 * Revision 1.21  1994/11/17  14:56:59  mike
 * moved segment validation functions from editor to main.
 * 
 * Revision 1.20  1994/11/16  23:38:46  mike
 * new improved boss teleportation behavior.
 * 
 * Revision 1.19  1994/10/30  14:12:14  mike
 * rip out local segments stuff.
 * 
 * Revision 1.18  1994/10/09  23:51:07  matt
 * Made find_hitpoint_uv() work with triangulated sides
 * 
 * Revision 1.17  1994/10/06  14:08:22  matt
 * Added new function, extract_orient_from_segment()
 * 
 * Revision 1.16  1994/09/19  21:05:52  mike
 * Prototype for find_connected_distance.
 * 
 * Revision 1.15  1994/08/11  18:58:45  mike
 * Change shorts to ints.
 * 
 * Revision 1.14  1994/08/04  00:21:09  matt
 * Cleaned up fvi & physics error handling; put in code to make sure objects
 * are in correct segment; simplified segment finding for objects and points
 * 
 * Revision 1.13  1994/08/02  19:04:25  matt
 * Cleaned up vertex list functions
 * 
 * Revision 1.12  1994/07/21  19:01:53  mike
 * lsegment stuff.
 * 
 * Revision 1.11  1994/07/07  09:31:13  matt
 * Added comments
 * 
 * Revision 1.10  1994/06/14  12:21:20  matt
 * Added new function, find_point_seg()
 * 
 * Revision 1.9  1994/05/29  23:17:38  matt
 * Move find_object_seg() from physics.c to gameseg.c
 * Killed unused find_point_seg()
 * 
 * Revision 1.8  1994/05/20  11:56:57  matt
 * Cleaned up find_vector_intersection() interface
 * Killed check_point_in_seg(), check_player_seg(), check_object_seg()
 * 
 * Revision 1.7  1994/03/17  18:07:38  yuan
 * Removed switch code... Now we just have Walls, Triggers, and Links...
 * 
 * Revision 1.6  1994/02/22  18:14:44  yuan
 * Added new wall system
 * 
 * Revision 1.5  1994/02/17  11:33:22  matt
 * Changes in object system
 * 
 * Revision 1.4  1994/02/16  13:48:33  mike
 * enable editor to compile out.
 * 
 * Revision 1.3  1994/02/14  12:05:07  mike
 * change segment data structure.
 * 
 * Revision 1.2  1994/02/10  16:07:20  mike
 * separate editor from game based on EDITOR flag.
 * 
 * Revision 1.1  1994/02/09  15:45:38  mike
 * Initial revision
 * 
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
   sbyte  sidemask;     //which sides sphere pokes through (6 bits)
   sbyte  centermask;   //which sides center point is on back of (6 bits)
} segmasks;

extern int	Highest_vertex_index;			// Highest index in Vertices and Vertex_active, an efficiency hack
extern int	Highest_segment_index;		// Highest index in Segments, an efficiency hack

extern void compute_center_point_on_side(vms_vector *vp,segment *sp,int side);
extern void compute_segment_center(vms_vector *vp,segment *sp);
extern int find_connect_side(segment *base_seg, segment *con_seg);

// Fill in array with four absolute point numbers for a given side
void get_side_verts(int *vertlist,int segnum,int sidenum);

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
extern void create_all_vertex_lists(int *num_faces, int *vertices, int segnum, int sidenum);

//like create_all_vertex_lists(), but generate absolute point numbers
extern void create_abs_vertex_lists(int *num_faces, int *vertices, int segnum, int sidenum, char *calling_file, int calling_linenum);

// -----------------------------------------------------------------------------------
// Like create all vertex lists, but returns the vertnums (relative to
// the side) for each of the faces that make up the side.  
//	If there is one face, it has 4 vertices.
//	If there are two faces, they both have three vertices, so face #0 is stored in vertices 0,1,2,
//	face #1 is stored in vertices 3,4,5.
void create_all_vertnum_lists(int *num_faces, int *vertnums, int segnum, int sidenum);

//	Given a side, return the number of faces
extern int get_num_faces(side *sidep);

//returns 3 different bitmasks with info telling if this sphere is in
//this segment.  See segmasks structure for info on fields   
segmasks get_seg_masks(vms_vector *checkp,int segnum,fix rad, char *calling_file, int calling_linenum);

//this macro returns true if the segnum for an object is correct
#define check_obj_seg(obj) (get_seg_masks(&(obj)->pos,(obj)->segnum,0,__FILE__,__LINE__).centermask == 0)

//Tries to find a segment for a point, in the following way:
// 1. Check the given segment
// 2. Recursively trace through attached segments
// 3. Check all the segmentns
//Returns segnum if found, or -1
int find_point_seg(vms_vector *p,int segnum);

//--repair-- //	Create data specific to segments which does not need to get written to disk.
//--repair-- extern void create_local_segment_data(void);

//	Sort of makes sure create_local_segment_data has been called for the currently executing mine.
//	Returns 1 if Lsegments appears valid, 0 if not.
int check_lsegments_validity(void);

//	----------------------------------------------------------------------------------------------------------
//	Determine whether seg0 and seg1 are reachable using wid_flag to go through walls.
//	For example, set to WID_RENDPAST_FLAG to see if sound can get from one segment to the other.
//	set to WID_FLY_FLAG to see if a robot could fly from one to the other.
//	Search up to a maximum depth of max_depth.
//	Return the distance.
extern fix find_connected_distance(vms_vector *p0, int seg0, vms_vector *p1, int seg1, int max_depth, int wid_flag);

//create a matrix that describes the orientation of the given segment
extern void extract_orient_from_segment(vms_matrix *m,segment *seg);

//	In segment.c
//	Make a just-modified segment valid.
//		check all sides to see how many faces they each should have (0,1,2)
//		create new vector normals
extern void validate_segment(segment *sp);

extern void validate_segment_all(void);

//	Extract the forward vector from segment *sp, return in *vp.
//	The forward vector is defined to be the vector from the the center of the front face of the segment
// to the center of the back face of the segment.
extern	void extract_forward_vector_from_segment(segment *sp,vms_vector *vp);

//	Extract the right vector from segment *sp, return in *vp.
//	The forward vector is defined to be the vector from the the center of the left face of the segment
// to the center of the right face of the segment.
extern	void extract_right_vector_from_segment(segment *sp,vms_vector *vp);

//	Extract the up vector from segment *sp, return in *vp.
//	The forward vector is defined to be the vector from the the center of the bottom face of the segment
// to the center of the top face of the segment.
extern	void extract_up_vector_from_segment(segment *sp,vms_vector *vp);

extern void create_walls_on_side(segment *sp, int sidenum);

extern void pick_random_point_in_seg(vms_vector *new_pos, int segnum);

#endif
 
