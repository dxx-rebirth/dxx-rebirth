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
 * Header for fvi.c
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
#define FQ_IGNORE_POWERUPS	16		//ignore powerups

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
//fills in u & v. if l is non-NULL fills it in also
void find_hitpoint_uv(fix *u,fix *v,fix *l, vms_vector *pnt,segment *seg,int sidenum,int facenum);

//Returns true if the object is through any walls
int object_intersects_wall(object *objp);
int object_intersects_wall_d(object *objp,int *hseg,int *hside,int *hface); // same as above but more detailed

#endif

