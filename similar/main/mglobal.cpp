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
 * Global variables for main directory
 *
 */

#include "maths.h"
#include "vecmat.h"
#include "inferno.h"
#include "segment.h"
#include "switch.h"
#include "object.h"
#include "bm.h"
#include "3d.h"
#include "game.h"
#include "textures.h"
#include "valptridx.tcc"
#include "wall.h"

namespace dcx {
unsigned Num_segments;
// Global array of vertices, common to one mine.
array<vertex, MAX_VERTICES> Vertices;
valptridx<active_door>::array_managed_type ActiveDoors;
}
array<g3s_point, MAX_VERTICES> Segment_points;

namespace dcx {
fix FrameTime = 0x1000;	// Time since last frame, in seconds
fix64 GameTime64 = 0;			//	Time in game, in seconds

int d_tick_count = 0; // increments every 33.33ms
int d_tick_step = 0;  // true once every 33.33ms

//	This is the global mine which create_new_mine returns.
//lsegment	Lsegments[MAX_SEGMENTS];

// Number of vertices in current mine (ie, Vertices, pointed to by Vp)

//	Translate table to get opposite side of a face on a segment.
unsigned Highest_vertex_index;

unsigned Num_vertices;
const array<uint8_t, MAX_SIDES_PER_SEGMENT> Side_opposite{{
	WRIGHT, WBOTTOM, WLEFT, WTOP, WFRONT, WBACK
}};

#define TOLOWER(c) ((((c)>='A') && ((c)<='Z'))?((c)+('a'-'A')):(c))

const array<array<unsigned, 4>, MAX_SIDES_PER_SEGMENT>  Side_to_verts_int{{
	{{7,6,2,3}},			// left
	{{0,4,7,3}},			// top
	{{0,1,5,4}},			// right
	{{2,6,5,1}},			// bottom
	{{4,5,6,7}},			// back
	{{3,2,1,0}},			// front
}};

// Texture map stuff

//--unused-- fix	Laser_delay_time = F1_0/6;		//	Delay between laser fires.

#define DEFAULT_DIFFICULTY		1

int	Difficulty_level=DEFAULT_DIFFICULTY;	//	Difficulty level in 0..NDL-1, 0 = easiest, NDL-1 = hardest
}

#if DXX_HAVE_POISON_UNDEFINED
template <typename managed_type>
valptridx<managed_type>::array_managed_type::array_managed_type()
{
	DXX_MAKE_MEM_UNDEFINED(this->begin(), this->end());
}
#endif

namespace dsx {
#if defined(DXX_BUILD_DESCENT_II)
valptridx<cloaking_wall>::array_managed_type CloakingWalls;
#endif
valptridx<object>::array_managed_type Objects;
valptridx<segment>::array_managed_type Segments;
valptridx<trigger>::array_managed_type Triggers;
valptridx<wall>::array_managed_type Walls;
}

template class valptridx<active_door>;
#if defined(DXX_BUILD_DESCENT_II)
template class valptridx<cloaking_wall>;
#endif
template class valptridx<object>;
template class valptridx<segment>;
template class valptridx<trigger>;
template class valptridx<wall>;
