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
 * Header file for stuff moved from segment.c to gameseg.c.
 *
 */

#pragma once

#include <random>
#include "pstypes.h"
#include "maths.h"
#include "vecmat.h"
#include "segment.h"

#include <utility>
#include "dxxsconf.h"
#include "dsx-ns.h"
#include "d_array.h"

namespace dcx {

struct segmasks
{
   short facemask;     //which faces sphere pokes through (12 bits)
   sidemask_t sidemask;     //which sides sphere pokes through (6 bits)
   sidemask_t centermask;   //which sides center point is on back of (6 bits)
};

struct segment_depth_array_t : public enumerated_array<uint8_t, MAX_SEGMENTS, segnum_t>
{
};

struct side_vertnum_list_t : std::array<vertnum_t, 4> {};

enum class vertex_array_side_type : bool
{
	quad,
	triangle,
};

#if DXX_BUILD_DESCENT == 2 || DXX_USE_EDITOR
#define DXX_internal_feature_lighting_hack	1
enum class lighting_hack : bool
{
	normal,
	ignore_out_of_mine_location,
};
#else
#define DXX_internal_feature_lighting_hack	0
#endif

struct vertnum_array_list_t : std::array<vertnum_t, 6> {};
struct vertex_array_list_t : std::array<segment_relative_vertnum, 6>
{
	vertex_array_side_type side_type;
};

struct vertex_vertnum_pair
{
	vertnum_t vertex;
	side_relative_vertnum vertnum;
};
using vertex_vertnum_array_list = std::array<vertex_vertnum_pair, 6>;

struct abs_vertex_list_result
{
	vertex_array_side_type num_faces;
	vertnum_array_list_t vertex_list;
};

#ifdef DXX_BUILD_DESCENT
[[nodiscard]]
sidenum_t find_connect_side(vcsegidx_t base_seg, const shared_segment &con_seg);

[[nodiscard]]
vms_vector compute_center_point_on_side(fvcvertptr &vcvertptr, const shared_segment &sp, sidenum_t side);

[[nodiscard]]
vms_vector compute_segment_center(fvcvertptr &vcvertptr, const shared_segment &sp);

// Fill in array with four absolute point numbers for a given side
side_vertnum_list_t get_side_verts(const shared_segment &segnum, sidenum_t sidenum);
#endif

enum class wall_is_doorway_mask : uint8_t;

#if DXX_USE_EDITOR
//      Create all vertex lists (1 or 2) for faces on a side.
//      Sets:
//              num_faces               number of lists
//              vertices                        vertices in all (1 or 2) faces
//      If there is one face, it has 4 vertices.
//      If there are two faces, they both have three vertices, so face #0 is stored in vertices 0,1,2,
//      face #1 is stored in vertices 3,4,5.
// Note: these are not absolute vertex numbers, but are relative to the segment
// Note:  for triangulated sides, the middle vertex of each trianle is the one NOT
//   adjacent on the diagonal edge
[[nodiscard]]
vertex_array_list_t create_all_vertex_lists(const shared_segment &segnum, const shared_side &sidep, const sidenum_t sidenum);
#endif

// -----------------------------------------------------------------------------------
// Like create all vertex lists, but returns the vertnums (relative to
// the side) for each of the faces that make up the side.
//      If there is one face, it has 4 vertices.
//      If there are two faces, they both have three vertices, so face #0 is stored in vertices 0,1,2,
//      face #1 is stored in vertices 3,4,5.
[[nodiscard]]
vertex_vertnum_array_list create_all_vertnum_lists(const shared_segment &seg, const shared_side &sidep, sidenum_t sidenum);

//like create_all_vertex_lists(), but generate absolute point numbers
[[nodiscard]]
abs_vertex_list_result create_abs_vertex_lists(const shared_segment &segnum, const shared_side &sidep, sidenum_t sidenum);

//create a matrix that describes the orientation of the given segment
[[nodiscard]]
vms_matrix extract_orient_from_segment(fvcvertptr &vcvertptr, const shared_segment &seg);

#if DXX_USE_EDITOR
//      Extract the forward vector from segment *sp, return it by value.
//      The forward vector is defined to be the vector from the center of the front face of the segment
// to the center of the back face of the segment.
[[nodiscard]]
vms_vector extract_forward_vector_from_segment(fvcvertptr &, const shared_segment &sp);

//      Extract the right vector from segment *sp, return it by value.
//      The forward vector is defined to be the vector from the center of the left face of the segment
// to the center of the right face of the segment.
[[nodiscard]]
vms_vector extract_right_vector_from_segment(fvcvertptr &, const shared_segment &sp);

//      Extract the up vector from segment *sp, return it by value.
//      The forward vector is defined to be the vector from the center of the bottom face of the segment
// to the center of the top face of the segment.
[[nodiscard]]
vms_vector extract_up_vector_from_segment(fvcvertptr &, const shared_segment &sp);
#endif

void validate_segment_all(d_level_shared_segment_state &);

#if DXX_USE_EDITOR
//      In segment.c
//      Make a just-modified segment valid.
//              check all sides to see how many faces they each should have (0,1,2)
//              create new vector normals
void validate_segment(fvcvertptr &vcvertptr, vmsegptridx_t sp);

void create_walls_on_side(fvcvertptr &, shared_segment &sp, sidenum_t sidenum);

void validate_segment_side(fvcvertptr &, vmsegptridx_t sp, sidenum_t sidenum);
#endif

[[nodiscard]]
vms_vector pick_random_point_in_seg(fvcvertptr &vcvertptr, const shared_segment &sp, std::minstd_rand r);

}

#ifdef DXX_BUILD_DESCENT
namespace dcx {
//      Given a side, return the number of faces
bool get_side_is_quad(const shared_side &sidep);

//returns 3 different bitmasks with info telling if this sphere is in
//this segment.  See segmasks structure for info on fields
segmasks get_seg_masks(fvcvertptr &, const vms_vector &checkp, const shared_segment &segnum, fix rad);
int check_segment_connections();

}

namespace dsx {

#if DXX_internal_feature_lighting_hack
extern lighting_hack Doing_lighting_hack_flag;
#endif

//this macro returns true if the segnum for an object is correct
#define check_obj_seg(vcvertptr, obj) (get_seg_masks(vcvertptr, (obj)->pos, vcsegptr((obj)->segnum), 0).centermask == 0)

//Tries to find a segment for a point, in the following way:
// 1. Check the given segment
// 2. Recursively trace through attached segments
// 3. Check all the segmentns
//Returns segnum if found, or -1
imsegptridx_t find_point_seg(const d_level_shared_segment_state &, d_level_unique_segment_state &, const vms_vector &p, imsegptridx_t segnum);
icsegptridx_t find_point_seg(const d_level_shared_segment_state &, const vms_vector &p, icsegptridx_t segnum);

//      ----------------------------------------------------------------------------------------------------------
//      Determine whether seg0 and seg1 are reachable using wid_flag to go through walls.
//      For example, set to WALL_IS_DOORWAY::rendpast to see if sound can get from one segment to the other.
//      set to WALL_IS_DOORWAY::fly to see if a robot could fly from one to the other.
//      Search up to a maximum depth of max_depth.
//      Return the distance.
vm_distance find_connected_distance(const vms_vector &p0, vcsegptridx_t seg0, const vms_vector &p1, vcsegptridx_t seg1, int max_depth, wall_is_doorway_mask wid_flag);

#if DXX_BUILD_DESCENT == 1
static inline void flush_fcd_cache() {}
#elif DXX_BUILD_DESCENT == 2
void flush_fcd_cache();
void apply_all_changed_light(const d_level_shared_destructible_light_state &LevelSharedDestructibleLightState, fvmsegptridx &vmsegptridx);
void	set_ambient_sound_flags(void);
#endif
}
#endif
