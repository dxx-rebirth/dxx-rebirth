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
 * Lighting system prototypes, structures, etc.
 *
 */

#pragma once

#include "maths.h"
#include "vecmat.h"

#ifdef __cplusplus
#include "fwd-object.h"
#include "fwd-segment.h"

struct g3s_lrgb;

#define MAX_LIGHT       0x10000     // max value

#define MIN_LIGHT_DIST  (F1_0*4)

extern array<g3s_lrgb, MAX_VERTICES> Dynamic_light;
#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
extern object *old_viewer;

// compute the lighting for an object.  Takes a pointer to the object,
// and possibly a rotated 3d point.  If the point isn't specified, the
// object's center point is rotated.
g3s_lrgb compute_object_light(vcobjptridx_t obj);

// turn headlight boost on & off
namespace dsx {
void toggle_headlight_active();
}
void start_lighting_frame(vobjptr_t viewer);
#endif

#endif
