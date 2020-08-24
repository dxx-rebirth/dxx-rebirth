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
 * Header for fvi.c
 *
 */

#pragma once

#include "vecmat.h"

#ifdef __cplusplus
#include "dxxsconf.h"
#include "fwd-object.h"
#include "pack.h"
#include "countarray.h"

//return values for find_vector_intersection() - what did we hit?
#define HIT_NONE		0		//we hit nothing
#define HIT_WALL		1		//we hit - guess - a wall
#define HIT_OBJECT	2		//we hit an object - which one?  no way to tell...
#define HIT_BAD_P0	3		//start point not is specified segment

#define MAX_FVI_SEGS 100

//this data structure gets filled in by find_vector_intersection()
struct fvi_info : prohibit_void_ptr<fvi_info>
{
	struct segment_array_t : public count_array_t<segnum_t, MAX_FVI_SEGS> {};
	int hit_type;					//what sort of intersection
	vms_vector hit_pnt;			//where we hit
	segnum_t hit_seg;					//what segment hit_pnt is in
	int hit_side;					//if hit wall, which side
	segnum_t hit_side_seg;				//what segment the hit side is in
	objnum_t hit_object;				//if object hit, which object
	vms_vector hit_wallnorm;	//if hit wall, ptr to its surface normal
	segment_array_t seglist;
};

//flags for fvi query
#define FQ_CHECK_OBJS	1		//check against objects?
#define FQ_TRANSWALL		2		//go through transparent walls
#define FQ_TRANSPOINT	4		//go through trans wall if hit point is transparent
#define FQ_GET_SEGLIST	8		//build a list of segments
#define FQ_IGNORE_POWERUPS	16		//ignore powerups

#ifdef dsx
//this data contains the parms to fvi()
struct fvi_query : prohibit_void_ptr<fvi_query>
{
	const vms_vector *p0,*p1;
	segnum_t startseg;
	objnum_t thisobjnum;
	fix rad;
	std::pair<const vcobjidx_t *, const vcobjidx_t *> ignore_obj_list;
	int flags;
};

struct fvi_hitpoint
{
	fix u, v;
};

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
int find_vector_intersection(const fvi_query &fq, fvi_info &hit_data);

//finds the uv coords of the given point on the given seg & side
//fills in u & v. if l is non-NULL fills it in also
namespace dsx {
__attribute_warn_unused_result
fvi_hitpoint find_hitpoint_uv(const vms_vector &pnt, const cscusegment seg, uint_fast32_t sidenum, uint_fast32_t facenum);
}

struct sphere_intersects_wall_result
{
	const shared_segment *seg;
	uint_fast32_t side;
};

//Returns true if the object is through any walls
sphere_intersects_wall_result sphere_intersects_wall(fvcsegptridx &vcsegptridx, fvcvertptr &vcvertptr, const vms_vector &pnt, vcsegptridx_t seg, fix rad);
#endif

#endif
