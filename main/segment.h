/* $Id: segment.h,v 1.4 2003-10-04 03:14:47 btb Exp $ */
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
 * Include file for functions which need to access segment data structure.
 *
 * Old Log:
 * Revision 1.4  1995/11/03  12:53:11  allender
 * shareware changes
 *
 * Revision 1.3  1995/07/26  16:53:45  allender
 * put sides and segment structure back the PC way for checksumming reasons
 *
 * Revision 1.2  1995/06/19  07:55:22  allender
 * rearranged structure members for possible better alignment
 *
 * Revision 1.1  1995/05/16  16:02:22  allender
 * Initial revision
 *
 * Revision 2.1  1995/03/20  18:15:22  john
 * Added code to not store the normals in the segment structure.
 *
 * Revision 2.0  1995/02/27  11:26:49  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.89  1995/01/24  15:07:55  yuan
 * *** empty log message ***
 *
 * Revision 1.88  1994/12/12  01:04:06  yuan
 * Boosted MAX_GAME_VERTS.
 *
 * Revision 1.87  1994/12/11  16:18:14  mike
 * add constants so we can detect too-large mines for game while in editor.
 *
 * Revision 1.86  1994/12/08  15:07:29  yuan
 * *** empty log message ***
 *
 * Revision 1.85  1994/12/01  21:06:39  matt
 * Moved plane tolerance constant to gameseg.c, the only file that used it.
 *
 * Revision 1.84  1994/11/27  14:01:41  matt
 * Fixed segment structure so LVLs work
 *
 * Revision 1.83  1994/11/26  22:50:20  matt
 * Removed editor-only fields from segment structure when editor is compiled
 * out, and padded segment structure to even multiple of 4 bytes.
 *
 * Revision 1.82  1994/11/21  11:43:36  mike
 * smaller segment and vertex buffers.
 *
 * Revision 1.81  1994/11/17  11:39:35  matt
 * Ripped out code to load old mines
 *
 * Revision 1.80  1994/10/30  14:12:05  mike
 * rip out local segments stuff.
 *
 * Revision 1.79  1994/10/27  11:33:58  mike
 * lower number of segments by 100, saving 116K.
 *
 * Revision 1.78  1994/08/25  21:54:50  mike
 * Add macro IS_CHILD to make checking for the presence of a child centralized.
 *
 * Revision 1.77  1994/08/11  18:58:16  mike
 * Add prototype for Side_to_verts_int.
 *
 * Revision 1.76  1994/08/01  11:04:13  yuan
 * New materialization centers.
 *
 * Revision 1.75  1994/07/25  00:04:19  matt
 * Various changes to accomodate new 3d, which no longer takes point numbers
 * as parms, and now only takes pointers to points.
 *
 * Revision 1.74  1994/07/21  19:01:30  mike
 * new lsegment structure.
 *
 * Revision 1.73  1994/06/08  14:30:48  matt
 * Added static_light field to segment structure, and padded side struct
 * to be longword aligned.
 *
 * Revision 1.72  1994/05/19  23:25:17  mike
 * Change MINE_VERSION to 15, DEFAULT_LIGHTING to 0
 *
 * Revision 1.71  1994/05/12  14:45:54  mike
 * New segment data structure (!!), group, special, object, value = short.
 *
 * Revision 1.70  1994/05/03  11:06:46  mike
 * Remove constants VMAG and UMAG which are editor specific..
 *
 * Revision 1.69  1994/04/18  10:40:28  yuan
 * Increased segment limit to 1000
 * (From 500)
 *
 *
 */

#ifndef _SEGMENT_H
#define _SEGMENT_H

#include "pstypes.h"
#include "fix.h"
#include "vecmat.h"
//#include "3d.h"
//#include "inferno.h"
#include "cfile.h"

// Version 1 - Initial version
// Version 2 - Mike changed some shorts to bytes in segments, so incompatible!

#define SIDE_IS_QUAD    1   // render side as quadrilateral
#define SIDE_IS_TRI_02  2   // render side as two triangles, triangulated along edge from 0 to 2
#define SIDE_IS_TRI_13  3   // render side as two triangles, triangulated along edge from 1 to 3

// Set maximum values for segment and face data structures.
#define MAX_VERTICES_PER_SEGMENT    8
#define MAX_SIDES_PER_SEGMENT       6
#define MAX_VERTICES_PER_POLY       4
#define WLEFT                       0
#define WTOP                        1
#define WRIGHT                      2
#define WBOTTOM                     3
#define WBACK                       4
#define WFRONT                      5

#if defined(SHAREWARE)
# define MAX_SEGMENTS           800
# define MAX_SEGMENT_VERTICES   2800
#else
# define MAX_SEGMENTS           900
# define MAX_SEGMENT_VERTICES   3600
#endif

//normal everyday vertices

#define DEFAULT_LIGHTING        0   // (F1_0/2)

#ifdef EDITOR   //verts for the new segment
# define NUM_NEW_SEG_VERTICES   8
# define NEW_SEGMENT_VERTICES   (MAX_SEGMENT_VERTICES)
# define MAX_VERTICES           (MAX_SEGMENT_VERTICES+NUM_NEW_SEG_VERTICES)
#else           //No editor
# define MAX_VERTICES           (MAX_SEGMENT_VERTICES)
#endif

// Returns true if segnum references a child, else returns false.
// Note that -1 means no connection, -2 means a connection to the outside world.
#define IS_CHILD(segnum) (segnum > -1)

//Structure for storing u,v,light values.
//NOTE: this structure should be the same as the one in 3d.h
typedef struct uvl {
	fix u, v, l;
} uvl;

#ifdef COMPACT_SEGS
typedef struct side {
	sbyte   type;           // replaces num_faces and tri_edge, 1 = quad, 2 = 0:2 triangulation, 3 = 1:3 triangulation
	ubyte   pad;            //keep us longword alligned
	short   wall_num;
	short   tmap_num;
	short   tmap_num2;
	uvl     uvls[4];
	//vms_vector normals[2];  // 2 normals, if quadrilateral, both the same.
} side;
#else
typedef struct side {
	sbyte   type;           // replaces num_faces and tri_edge, 1 = quad, 2 = 0:2 triangulation, 3 = 1:3 triangulation
	ubyte   pad;            //keep us longword alligned
	short   wall_num;
	short   tmap_num;
	short   tmap_num2;
	uvl     uvls[4];
	vms_vector normals[2];  // 2 normals, if quadrilateral, both the same.
} side;
#endif

typedef struct segment {
#ifdef EDITOR
	short   segnum;     // segment number, not sure what it means
#endif
	side    sides[MAX_SIDES_PER_SEGMENT];       // 6 sides
	short   children[MAX_SIDES_PER_SEGMENT];    // indices of 6 children segments, front, left, top, right, bottom, back
	short   verts[MAX_VERTICES_PER_SEGMENT];    // vertex ids of 4 front and 4 back vertices
#ifdef EDITOR
	short   group;      // group number to which the segment belongs.
	short   objects;    // pointer to objects in this segment
#else
	int     objects;    // pointer to objects in this segment
#endif

	// -- Moved to segment2 to make this struct 512 bytes long --
	//ubyte   special;    // what type of center this is
	//sbyte   matcen_num; // which center segment is associated with.
	//short   value;
	//fix     static_light; //average static light in segment
	//#ifndef EDITOR
	//short   pad;        //make structure longword aligned
	//#endif
} segment;

#define S2F_AMBIENT_WATER   0x01
#define S2F_AMBIENT_LAVA    0x02

typedef struct segment2 {
	ubyte   special;
	sbyte   matcen_num;
	sbyte   value;
	ubyte   s2_flags;
	fix     static_light;
} segment2;

//values for special field
#define SEGMENT_IS_NOTHING      0
#define SEGMENT_IS_FUELCEN      1
#define SEGMENT_IS_REPAIRCEN    2
#define SEGMENT_IS_CONTROLCEN   3
#define SEGMENT_IS_ROBOTMAKER   4
#define SEGMENT_IS_GOAL_BLUE    5
#define SEGMENT_IS_GOAL_RED     6
#define MAX_CENTER_TYPES        7

#ifdef COMPACT_SEGS
extern void get_side_normal(segment *sp, int sidenum, int normal_num, vms_vector * vm );
extern void get_side_normals(segment *sp, int sidenum, vms_vector * vm1, vms_vector *vm2 );
#endif

// Local segment data.
// This is stuff specific to a segment that does not need to get
// written to disk.  This is a handy separation because we can add to
// this structure without obsoleting existing data on disk.

#define SS_REPAIR_CENTER    0x01    // Bitmask for this segment being part of repair center.

//--repair-- typedef struct {
//--repair-- 	int     special_type;
//--repair-- 	short   special_segment; // if special_type indicates repair center, this is the base of the repair center
//--repair-- } lsegment;

typedef struct {
	int     num_segments;
	int     num_vertices;
	short   segments[MAX_SEGMENTS];
	short   vertices[MAX_VERTICES];
} group;

// Globals from mglobal.c
extern vms_vector   Vertices[];
extern segment      Segments[];
extern segment2     Segment2s[];
extern int          Num_segments;
extern int          Num_vertices;

extern sbyte Side_to_verts[MAX_SIDES_PER_SEGMENT][4];       // Side_to_verts[my_side] is list of vertices forming side my_side.
extern int  Side_to_verts_int[MAX_SIDES_PER_SEGMENT][4];    // Side_to_verts[my_side] is list of vertices forming side my_side.
extern char Side_opposite[];                                // Side_opposite[my_side] returns side opposite cube from my_side.

#define SEG_PTR_2_NUM(segptr) (Assert((unsigned) (segptr-Segments)<MAX_SEGMENTS),(segptr)-Segments)

// New stuff, 10/14/95: For shooting out lights and monitors.
// Light cast upon vert_light vertices in segnum:sidenum by some light
typedef struct {
	short   segnum;
	sbyte   sidenum;
	sbyte   dummy;
	ubyte   vert_light[4];
} delta_light;

// Light at segnum:sidenum casts light on count sides beginning at index (in array Delta_lights)
typedef struct {
	short   segnum;
	sbyte   sidenum;
	sbyte   count;
	short   index;
} dl_index;

#define MAX_DL_INDICES      500
#define MAX_DELTA_LIGHTS    10000

#define DL_SCALE            2048    // Divide light to allow 3 bits integer, 5 bits fraction.

extern dl_index     Dl_indices[MAX_DL_INDICES];
extern delta_light  Delta_lights[MAX_DELTA_LIGHTS];
extern int          Num_static_lights;

extern int subtract_light(int segnum, int sidenum);
extern int add_light(int segnum, int sidenum);
extern void restore_all_lights_in_mine(void);
extern void clear_light_subtracted(void);

extern ubyte Light_subtracted[MAX_SEGMENTS];

// ----------------------------------------------------------------------------
// --------------------- Segment interrogation functions ----------------------
// Do NOT read the segment data structure directly.  Use these
// functions instead.  The segment data structure is GUARANTEED to
// change MANY TIMES.  If you read the segment data structure
// directly, your code will break, I PROMISE IT!

// Return a pointer to the list of vertex indices for the current
// segment in vp and the number of vertices in *nv.
extern void med_get_vertex_list(segment *s,int *nv,short **vp);

// Return a pointer to the list of vertex indices for face facenum in
// vp and the number of vertices in *nv.
extern void med_get_face_vertex_list(segment *s,int side, int facenum,int *nv,short **vp);

// Set *nf = number of faces in segment s.
extern void med_get_num_faces(segment *s,int *nf);

void med_validate_segment_side(segment *sp,int side);

// Delete segment function added for curves.c
extern int med_delete_segment(segment *sp);

// Delete segment from group
extern void delete_segment_from_group(int segment_num, int group_num);

// Add segment to group
extern void add_segment_to_group(int segment_num, int group_num);

// Verify that all vertices are legal.
extern void med_check_all_vertices();

#ifdef FAST_FILE_IO
#define segment2_read(s2, fp) cfread(s2, sizeof(segment2), 1, fp)
#define delta_light_read(dl, fp) cfread(dl, sizeof(delta_light), 1, fp)
#define dl_index_read(di, fp) cfread(di, sizeof(dl_index), 1, fp)
#else
/*
 * reads a segment2 structure from a CFILE
 */
void segment2_read(segment2 *s2, CFILE *fp);

/*
 * reads a delta_light structure from a CFILE
 */
void delta_light_read(delta_light *dl, CFILE *fp);

/*
 * reads a dl_index structure from a CFILE
 */
void dl_index_read(dl_index *di, CFILE *fp);
#endif

#endif
