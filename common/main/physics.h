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
 * Headers for physics functions and data
 *
 */


#ifndef _PHYSICS_H
#define _PHYSICS_H

#include "vecmat.h"
#include "fvi.h"
#include "fwd-window.h"

#ifdef __cplusplus

//#define FL_NORMAL  0
//#define FL_TURBO   1
//#define FL_HOVER   2
//#define FL_REVERSE 3

// Simulate a physics object for this frame
#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
struct phys_visited_seglist
{
	unsigned nsegs;
	array<segnum_t, MAX_FVI_SEGS> seglist;
};

#ifdef dsx
namespace dsx {
window_event_result do_physics_sim(vmobjptridx_t obj, phys_visited_seglist *phys_segs);

}
#endif
namespace dcx {

// Applies an instantaneous force on an object, resulting in an instantaneous
// change in velocity.
void phys_apply_force(object_base &obj, const vms_vector &force_vec);

}
namespace dsx {
void phys_apply_rot(object &obj, const vms_vector &force_vec);
}

// this routine will set the thrust for an object to a value that will
// (hopefully) maintain the object's current velocity
namespace dcx {
void set_thrust_from_velocity(object_base &obj);
}
void check_and_fix_matrix(vms_matrix &m);
namespace dcx {
void physics_turn_towards_vector(const vms_vector &goal_vector, object_base &obj, fix rate);
}
#endif

#endif

#endif /* _PHYSICS_H */
