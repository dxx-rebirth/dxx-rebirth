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

#include "fwd-object.h"
#include "fwd-segment.h"
#include "3d.h"
#include "d_array.h"

#define MAX_LIGHT       0x10000     // max value

#define MIN_LIGHT_DIST  (F1_0*4)

namespace dcx {

struct d_level_unique_light_state
{
	enumerated_array<g3s_lrgb, MAX_VERTICES, vertnum_t> Dynamic_light;
};

}

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
namespace dsx {

#if defined(DXX_BUILD_DESCENT_II)
struct d_level_unique_headlight_state
{
	unsigned Num_headlights = 0;
	std::array<const object_base *, 8> Headlights = {};
};

struct d_level_unique_light_state :
	d_level_unique_headlight_state,
	::dcx::d_level_unique_light_state
{
};
#endif

extern d_level_unique_light_state LevelUniqueLightState;
}
extern const object *old_viewer;

namespace dsx {
// compute the lighting for an object.  Takes a pointer to the object,
// and possibly a rotated 3d point.  If the point isn't specified, the
// object's center point is rotated.
g3s_lrgb compute_object_light(const d_level_unique_light_state &LevelUniqueLightState, vcobjptridx_t obj);

// turn headlight boost on & off
#if defined(DXX_BUILD_DESCENT_II)
void toggle_headlight_active(object &);
#endif
}
void start_lighting_frame(const object &viewer);
#endif
