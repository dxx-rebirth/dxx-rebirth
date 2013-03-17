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
 * Headers for physics functions and data
 *
 */


#ifndef _PHYSICS_H
#define _PHYSICS_H

#include "vecmat.h"
#include "fvi.h"

//#define FL_NORMAL  0
//#define FL_TURBO   1
//#define FL_HOVER   2
//#define FL_REVERSE 3

// these global vars are set after a call to do_physics_sim().  Ugly, I know.
// list of segments went through
extern int phys_seglist[MAX_FVI_SEGS], n_phys_segs;

// Read contrls and set physics vars
void read_flying_controls(object *obj);

// Simulate a physics object for this frame
void do_physics_sim(object *obj);

// tell us what the given object will do (as far as hiting walls) in
// the given time (in seconds) t.  Igores acceleration (sorry)
// if check_objects is set, check with objects, else just with walls
// returns fate, fills in hit time.  If fate==HIT_NONE, hit_time undefined
// Stuff hit_info with fvi data as set by find_vector_intersection.
// for fvi_flags, refer to fvi.h for the fvi query flags
int physics_lookahead(object *obj, fix t, int fvi_flags, fix *hit_time, fvi_info *hit_info);

// Applies an instantaneous force on an object, resulting in an instantaneous
// change in velocity.
void phys_apply_force(object *obj, vms_vector *force_vec);
void phys_apply_rot(object *obj, vms_vector *force_vec);

// this routine will set the thrust for an object to a value that will
// (hopefully) maintain the object's current velocity
void set_thrust_from_velocity(object *obj);

#endif /* _PHYSICS_H */
