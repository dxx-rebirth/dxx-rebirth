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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/fvi.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:42:03 $
 * 
 * Header for fvi.c
 * 
 * $Log: fvi.h,v $
 * Revision 1.1.1.1  2006/03/17 19:42:03  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 22:12:15  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.1  1995/03/20  18:15:58  john
 * Added code to not store the normals in the segment structure.
 * 
 * Revision 2.0  1995/02/27  11:32:02  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.10  1995/02/02  14:07:58  matt
 * Fixed confusion about which segment you are touching when you're 
 * touching a wall.  This manifested itself in spurious lava burns.
 * 
 * Revision 1.9  1994/12/04  22:48:04  matt
 * Physics & FVI now only build seglist for player objects, and they 
 * responsilby deal with buffer full conditions
 * 
 * Revision 1.8  1994/10/31  12:28:01  matt
 * Added new function object_intersects_wall()
 * 
 * Revision 1.7  1994/10/10  13:10:00  matt
 * Increased max_fvi_segs
 * 
 * Revision 1.6  1994/09/25  00:38:29  matt
 * Made the 'find the point in the bitmap where something hit' system
 * publicly accessible.
 * 
 * Revision 1.5  1994/08/01  13:30:35  matt
 * Made fvi() check holes in transparent walls, and changed fvi() calling
 * parms to take all input data in query structure.
 * 
 * Revision 1.4  1994/07/13  21:47:59  matt
 * FVI() and physics now keep lists of segments passed through which the
 * trigger code uses.
 * 
 * Revision 1.3  1994/07/08  14:27:26  matt
 * Non-needed powerups don't get picked up now; this required changing FVI to
 * take a list of ingore objects rather than just one ignore object.
 * 
 * Revision 1.2  1994/06/09  09:58:39  matt
 * Moved find_vector_intersection() from physics.c to new file fvi.c
 * 
 * Revision 1.1  1994/06/09  09:26:14  matt
 * Initial revision
 * 
 * 
 */



#ifndef _FVI_H
#define _FVI_H

#include "vecmat.h"
#include "segment.h"
#include "object.h"

//return values for find_vector_intersection() - what did we hit?
#define HIT_NONE		0		//we hit nothing
#define HIT_WALL		1		//we hit - guess - a wall
#define HIT_OBJECT	2		//we hit an object - which one?  no way to tell...
#define HIT_BAD_P0	3		//start point not is specified segment

#define MAX_FVI_SEGS 100

//this data structure gets filled in by find_vector_intersection()
typedef struct fvi_info {
	int hit_type;					//what sort of intersection
	vms_vector hit_pnt;			//where we hit
	int hit_seg;					//what segment hit_pnt is in
	int hit_side;					//if hit wall, which side
	int hit_side_seg;				//what segment the hit side is in
	int hit_object;				//if object hit, which object
	vms_vector hit_wallnorm;	//if hit wall, ptr to its surface normal
	int n_segs;						//how many segs we went through
	int seglist[MAX_FVI_SEGS];	//list of segs vector went through
} fvi_info;

//flags for fvi query
#define FQ_CHECK_OBJS	1		//check against objects?
#define FQ_TRANSWALL		2		//go through transparent walls
#define FQ_TRANSPOINT	4		//go through trans wall if hit point is transparent
#define FQ_GET_SEGLIST	8		//build a list of segments

//this data contains the parms to fvi()
typedef struct fvi_query {
	vms_vector *p0,*p1;
	int startseg;
	fix rad;
	short thisobjnum;
	int *ignore_obj_list;
	int flags;
} fvi_query;

//Find out if a vector intersects with anything.
//Fills in hit_data, an fvi_info structure (see above).
//Parms:
//  p0 & startseg 	describe the start of the vector
//  p1 					the end of the vector
//  rad 					the radius of the cylinder
//  thisobjnum 		used to prevent an object with colliding with itself
//  ingore_obj_list	NULL, or ptr to a list of objnums to ignore, terminated with -1
//  check_obj_flag	determines whether collisions with objects are checked
//Returns the hit_data->hit_type
int find_vector_intersection(fvi_query *fq,fvi_info *hit_data);

//finds the uv coords of the given point on the given seg & side
//fills in u & v
void find_hitpoint_uv(fix *u,fix *v,vms_vector *pnt,segment *seg,int sidenum,int facenum);

//Returns true if the object is through any walls
int object_intersects_wall(object *objp);
int object_intersects_wall_d(object *objp,int *hseg,int *hside,int *hface); // same as above but more detailed

#endif
 
