/* $Id: physics.h,v 1.2 2003-10-10 09:36:35 btb Exp $ */
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
 * Old Log:
 * Revision 1.2  1995/08/23  21:33:04  allender
 * fix mcc compiler warnings
 *
 * Revision 1.1  1995/05/16  16:00:56  allender
 * Initial revision
 *
 * Revision 2.0  1995/02/27  11:33:06  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.29  1995/02/06  19:47:18  matt
 * New function (untested), set_thrust_from_velocity()
 *
 * Revision 1.28  1994/12/04  22:14:20  mike
 * apply instantaneous rotation to an object due to a force blow.
 *
 * Revision 1.27  1994/08/01  13:29:42  matt
 * Made fvi() check holes in transparent walls, and changed fvi() calling
 * parms to take all input data in query structure.
 *
 * Revision 1.26  1994/07/28  12:35:22  matt
 * Added prototype
 *
 * Revision 1.25  1994/07/13  21:48:05  matt
 * FVI() and physics now keep lists of segments passed through which the
 * trigger code uses.
 *
 * Revision 1.24  1994/06/30  19:01:55  matt
 * Moved flying controls code from physics.c to controls.c
 *
 * Revision 1.23  1994/06/16  14:14:20  mike
 * Change physics_lookahead to return hit_info.
 *
 * Revision 1.22  1994/06/09  09:58:43  matt
 * Moved find_vector_intersection() from physics.c to new file fvi.c
 *
 * Revision 1.21  1994/05/20  16:11:07  matt
 * Added new parm, ignore_obj, to find_vector_intersection()
 *
 * Revision 1.20  1994/05/20  15:16:58  matt
 * Added new fvi return type; took out some troublesome (and troubling) asserts
 *
 *
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
