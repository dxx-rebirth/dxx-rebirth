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
 * Functions moved from segment.c to make editor separable from game.
 *
 */

#include <algorithm>
#include <cassert>
#include <numeric>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>	//	for memset()

#include "u_mem.h"
#include "inferno.h"
#include "game.h"
#include "dxxerror.h"
#include "console.h"
#include "vecmat.h"
#include "gameseg.h"
#include "wall.h"
#include "textures.h"
#include "object.h"
#include "byteutil.h"
#include "lighting.h"
#include "mission.h"
#if DXX_USE_EDITOR
#include "editor/editor.h"
#endif

#include "compiler-range_for.h"
#include "d_array.h"
#include "d_levelstate.h"
#include "d_range.h"
#include "d_zip.h"
#include "cast_range_result.h"

using std::min;

namespace {

	/* The array can be of any type that can hold values in the range
	 * [0, AMBIENT_SEGMENT_DEPTH].
	 */
struct segment_lava_depth_array : enumerated_array<uint8_t, MAX_SEGMENTS, segnum_t> {};
struct segment_water_depth_array : enumerated_array<uint8_t, MAX_SEGMENTS, segnum_t> {};

class abs_vertex_lists_predicate
{
	const enumerated_array<vertnum_t, MAX_VERTICES_PER_SEGMENT, segment_relative_vertnum> &m_vp;
	const enumerated_array<segment_relative_vertnum, 4, side_relative_vertnum> &m_sv;
public:
	abs_vertex_lists_predicate(const shared_segment &seg, const sidenum_t sidenum) :
		m_vp(seg.verts), m_sv(Side_to_verts[sidenum])
	{
	}
	vertnum_t operator()(const side_relative_vertnum vv) const
	{
		return m_vp[m_sv[vv]];
	}
};

class all_vertnum_lists_predicate : public abs_vertex_lists_predicate
{
public:
	using abs_vertex_lists_predicate::abs_vertex_lists_predicate;
	vertex_vertnum_pair operator()(const side_relative_vertnum vv) const
	{
		return {this->abs_vertex_lists_predicate::operator()(vv), vv};
	}
};

struct verts_for_normal
{
	std::array<vertnum_t, 4> vsorted;
	verts_for_normal(vertnum_t v0, vertnum_t v1, vertnum_t v2, vertnum_t v3) :
		vsorted{{v0, v1, v2, v3}}
	{
	}
};

constexpr vm_distance fcd_abort_cache_value{F1_0 * 1000};
constexpr vm_distance fcd_abort_return_value{-1};

#if defined(DXX_BUILD_DESCENT_II)
static constexpr sidemask_t &operator&=(sidemask_t &a, const sidemask_t b)
{
	return a = static_cast<sidemask_t>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}
#endif

}

namespace dcx {
namespace {

// How far a point can be from a plane, and still be "in" the plane
#define PLANE_DIST_TOLERANCE	250

static void compute_center_point_on_side(fvcvertptr &vcvertptr, vms_vector &r, const enumerated_array<vertnum_t, MAX_VERTICES_PER_SEGMENT, segment_relative_vertnum> &verts, const sidenum_t side)
{
	vms_vector vp;
	vm_vec_zero(vp);
	range_for (auto &v, Side_to_verts[side])
		vm_vec_add2(vp, vcvertptr(verts[v]));
	vm_vec_copy_scale(r, vp, F1_0 / 4);
}

static void compute_segment_center(fvcvertptr &vcvertptr, vms_vector &r, const std::array<vertnum_t, MAX_VERTICES_PER_SEGMENT> &verts)
{
	vms_vector vp;
	vm_vec_zero(vp);
	range_for (auto &v, verts)
		vm_vec_add2(vp, vcvertptr(v));
	vm_vec_copy_scale(r, vp, F1_0 / 8);
}

[[noreturn]]
__attribute_cold
static void create_vertex_list_from_invalid_side(const shared_segment &segp, const shared_side &sidep)
{
	throw shared_side::illegal_type(segp, sidep);
}

// Fill in array with four absolute point numbers for a given side
static void get_side_verts(side_vertnum_list_t &vertlist, const enumerated_array<vertnum_t, MAX_VERTICES_PER_SEGMENT, segment_relative_vertnum> &vp, const sidenum_t sidenum)
{
	auto &sv = Side_to_verts[sidenum];
	for (auto &&[ovl, isv] : zip(vertlist, sv))
		ovl = vp[isv];
}

}

// ------------------------------------------------------------------------------------------
// Compute the center point of a side of a segment.
//	The center point is defined to be the average of the 4 points defining the side.
void compute_center_point_on_side(fvcvertptr &vcvertptr, vms_vector &vp, const shared_segment &sp, const sidenum_t side)
{
	compute_center_point_on_side(vcvertptr, vp, sp.verts, side);
}

// ------------------------------------------------------------------------------------------
// Compute segment center.
//	The center point is defined to be the average of the 8 points defining the segment.
void compute_segment_center(fvcvertptr &vcvertptr, vms_vector &vp, const shared_segment &sp)
{
	compute_segment_center(vcvertptr, vp, sp.verts);
}

// -----------------------------------------------------------------------------
//	Given two segments, return the side index in the connecting segment which connects to the base segment
//	Optimized by MK on 4/21/94 because it is a 2% load.
sidenum_t find_connect_side(const vcsegidx_t base_seg, const shared_segment &con_seg)
{
	auto &children = con_seg.children;
	const auto &&b = begin(children);
	return static_cast<sidenum_t>(std::distance(b, std::find(b, end(children), base_seg)));
}

// -----------------------------------------------------------------------------------
//	Given a side, return the number of faces
bool get_side_is_quad(const shared_side &sidep)
{
	switch (sidep.get_type())
	{
		case side_type::quad:
			return true;
		case side_type::tri_02:
		case side_type::tri_13:
			return false;
		default:
			throw shared_side::illegal_type(sidep);
	}
}

void get_side_verts(side_vertnum_list_t &vertlist, const shared_segment &segp, const sidenum_t sidenum)
{
	get_side_verts(vertlist, segp.verts, sidenum);
}

namespace {

template <typename T, typename V>
[[nodiscard]]
static uint_fast32_t create_vertex_lists_from_values(T &va, const shared_segment &segp, const shared_side &sidep, const V &&f0, const V &&f1, const V &&f2, const V &&f3)
{
	const auto type = sidep.get_type();
	if (type == side_type::tri_13)
	{
		va[0] = va[5] = f3;
		va[1] = f0;
		va[2] = va[3] = f1;
		va[4] = f2;
		return 2;
	}
	va[0] = f0;
	va[1] = f1;
	va[2] = f2;
	switch (type)
	{
		case side_type::quad:
			va[3] = f3;
			/* Unused, but required to prevent bogus
			 * -Wmaybe-uninitialized in check_segment_connections
			 */
			va[4] = va[5] = {};
			DXX_MAKE_MEM_UNDEFINED(std::span(va).template subspan<4, 2>());
			return 1;
		case side_type::tri_02:
			va[3] = f2;
			va[4] = f3;
			va[5] = f0;

			//IMPORTANT: DON'T CHANGE THIS CODE WITHOUT CHANGING GET_SEG_MASKS()
			//CREATE_ABS_VERTEX_LISTS(), CREATE_ALL_VERTEX_LISTS(), CREATE_ALL_VERTNUM_LISTS()
			return 2;
		default:
			create_vertex_list_from_invalid_side(segp, sidep);
	}
}

template <typename T, typename F>
static inline uint_fast32_t create_vertex_lists_by_predicate(T &va, const shared_segment &segp, const shared_side &sidep, const F &&f)
{
	return create_vertex_lists_from_values(va, segp, sidep, f(side_relative_vertnum::_0), f(side_relative_vertnum::_1), f(side_relative_vertnum::_2), f(side_relative_vertnum::_3));
}

}

}

namespace dsx {

#if DXX_USE_EDITOR
// -----------------------------------------------------------------------------------
//	Create all vertex lists (1 or 2) for faces on a side.
//	Sets:
//		num_faces		number of lists
//		vertices			vertices in all (1 or 2) faces
//	If there is one face, it has 4 vertices.
//	If there are two faces, they both have three vertices, so face #0 is stored in vertices 0,1,2,
//	face #1 is stored in vertices 3,4,5.
// Note: these are not absolute vertex numbers, but are relative to the segment
// Note:  for triagulated sides, the middle vertex of each trianle is the one NOT
//   adjacent on the diagonal edge
uint_fast32_t create_all_vertex_lists(vertex_array_list_t &vertices, const shared_segment &segp, const shared_side &sidep, const sidenum_t sidenum)
{
	assert(Side_to_verts.valid_index(sidenum));
	auto &sv = Side_to_verts[sidenum];
	return create_vertex_lists_by_predicate(vertices, segp, sidep, [&sv](const side_relative_vertnum vv) {
		return sv[vv];
	});
}
#endif

// -----------------------------------------------------------------------------------
// Like create all vertex lists, but returns the vertnums (relative to
// the side) for each of the faces that make up the side. 
//	If there is one face, it has 4 vertices.
//	If there are two faces, they both have three vertices, so face #0 is stored in vertices 0,1,2,
//	face #1 is stored in vertices 3,4,5.
void create_all_vertnum_lists(vertex_vertnum_array_list &vertnums, const shared_segment &segp, const shared_side &sidep, const sidenum_t sidenum)
{
	create_vertex_lists_by_predicate(vertnums, segp, sidep, all_vertnum_lists_predicate(segp, sidenum));
}

// -----
// like create_all_vertex_lists(), but generate absolute point numbers
uint_fast32_t create_abs_vertex_lists(vertnum_array_list_t &vertices, const shared_segment &segp, const shared_side &sidep, const sidenum_t sidenum)
{
	return create_vertex_lists_by_predicate(vertices, segp, sidep, abs_vertex_lists_predicate(segp, sidenum));
}

//returns 3 different bitmasks with info telling if this sphere is in
//this segment.  See segmasks structure for info on fields  
segmasks get_seg_masks(fvcvertptr &vcvertptr, const vms_vector &checkp, const shared_segment &seg, const fix rad)
{
	segmasks		masks{};

	//check point against each side of segment. return bitmask

	int facebit = 1;
	for (const auto sn : MAX_SIDES_PER_SEGMENT)
	{
		const auto sidebit = build_sidemask(sn);
		auto &s = seg.sides[sn];
		
		// Get number of faces on this side, and at vertex_list, store vertices.
		//	If one face, then vertex_list indicates a quadrilateral.
		//	If two faces, then 0,1,2 define one triangle, 3,4,5 define the second.
		const auto &&[num_faces, vertex_list] = create_abs_vertex_lists(seg, s, sn);

		//ok...this is important.  If a side has 2 faces, we need to know if
		//those faces form a concave or convex side.  If the side pokes out,
		//then a point is on the back of the side if it is behind BOTH faces,
		//but if the side pokes in, a point is on the back if behind EITHER face.

		if (num_faces==2) {
			int	side_count,center_count;

			const auto vertnum = min(vertex_list[0],vertex_list[2]);
			const auto &&mvert = vcvertptr(vertnum);

			auto a = vertex_list[4] < vertex_list[1]
				? std::make_pair(vertex_list[4], &s.normals[0])
				: std::make_pair(vertex_list[1], &s.normals[1]);
			const auto mdist = vm_dist_to_plane(vcvertptr(a.first), *a.second, mvert);

			side_count = center_count = 0;

			for (int fn=0;fn<2;fn++,facebit<<=1) {

				const auto dist = vm_dist_to_plane(checkp, s.normals[fn], mvert);

				if (dist-rad < -PLANE_DIST_TOLERANCE) {
					if (dist < -PLANE_DIST_TOLERANCE)	//in front of face
						center_count++;

					masks.facemask |= facebit;
					side_count++;
				}
			}

			if (!(mdist > PLANE_DIST_TOLERANCE)) {		//must be behind both faces

				if (side_count==2)
					masks.sidemask |= sidebit;

				if (center_count==2)
					masks.centermask |= sidebit;

			}
			else {							//must be behind at least one face

				if (side_count)
					masks.sidemask |= sidebit;

				if (center_count)
					masks.centermask |= sidebit;

			}


		}
		else {				//only one face on this side
			//use lowest point number
			auto b = begin(vertex_list);
			const auto vertnum = *std::min_element(b, std::next(b, 4));
			const auto &&mvert = vcvertptr(vertnum);

			const auto dist = vm_dist_to_plane(checkp, s.normals[0], mvert);
			if (dist-rad < -PLANE_DIST_TOLERANCE) {
				if (dist < -PLANE_DIST_TOLERANCE)
					masks.centermask |= sidebit;
	
				masks.facemask |= facebit;
				masks.sidemask |= sidebit;
			}

			facebit <<= 2;
		}

	}
	return masks;
}

namespace {
//this was converted from get_seg_masks()...it fills in an array of 6
//elements for the distace behind each side, or zero if not behind
//only gets centermask, and assumes zero rad
static sidemask_t get_side_dists(fvcvertptr &vcvertptr, const vms_vector &checkp, const shared_segment &segnum, per_side_array<fix> &side_dists)
{
	sidemask_t mask{};
	auto &sides = segnum.sides;

	//check point against each side of segment. return bitmask
	side_dists = {};
	int facebit = 1;
	for (const auto sn : MAX_SIDES_PER_SEGMENT)
	{
		const auto sidebit = build_sidemask(sn);
		auto &s = sides[sn];

		// Get number of faces on this side, and at vertex_list, store vertices.
		//	If one face, then vertex_list indicates a quadrilateral.
		//	If two faces, then 0,1,2 define one triangle, 3,4,5 define the second.
		const auto &&[num_faces, vertex_list] = create_abs_vertex_lists(segnum, s, sn);

		//ok...this is important.  If a side has 2 faces, we need to know if
		//those faces form a concave or convex side.  If the side pokes out,
		//then a point is on the back of the side if it is behind BOTH faces,
		//but if the side pokes in, a point is on the back if behind EITHER face.

		if (num_faces==2) {
			int	center_count;

			const auto vertnum = min(vertex_list[0],vertex_list[2]);
			auto &mvert = *vcvertptr(vertnum);

			auto a = vertex_list[4] < vertex_list[1]
				? std::make_pair(vertex_list[4], &s.normals[0])
				: std::make_pair(vertex_list[1], &s.normals[1]);
			const auto mdist = vm_dist_to_plane(vcvertptr(a.first), *a.second, mvert);

			center_count = 0;

			for (int fn=0;fn<2;fn++,facebit<<=1) {

				const auto dist = vm_dist_to_plane(checkp, s.normals[fn], mvert);

				if (dist < -PLANE_DIST_TOLERANCE) {	//in front of face
					center_count++;
					side_dists[sn] += dist;
				}

			}

			if (!(mdist > PLANE_DIST_TOLERANCE)) {		//must be behind both faces

				if (center_count==2) {
					mask |= sidebit;
					side_dists[sn] /= 2;		//get average
				}
					

			}
			else {							//must be behind at least one face

				if (center_count) {
					mask |= sidebit;
					if (center_count==2)
						side_dists[sn] /= 2;		//get average

				}
			}


		}
		else {				//only one face on this side
			//use lowest point number

			auto b = begin(vertex_list);
			auto vertnum = *std::min_element(b, std::next(b, 4));

			const auto dist = vm_dist_to_plane(checkp, s.normals[0], vcvertptr(vertnum));
	
			if (dist < -PLANE_DIST_TOLERANCE) {
				mask |= sidebit;
				side_dists[sn] = dist;
			}
	
			facebit <<= 2;
		}

	}

	return mask;

}

#ifndef NDEBUG
//returns true if errors detected
static int check_norms(const shared_segment &segp, const sidenum_t sidenum, const unsigned facenum, const shared_segment &csegp, const sidenum_t csidenum, const unsigned cfacenum)
{
	const auto &n0 = segp.sides[sidenum].normals[facenum];
	const auto &n1 = csegp.sides[csidenum].normals[cfacenum];
	if (n0.x != -n1.x || n0.y != -n1.y || n0.z != -n1.z)
		return 1;
	else
		return 0;
}

static void invert_shared_side_triangle_type(shared_side &s)
{
	const auto t = s.get_type();
	side_type nt;
	switch (t)
	{
		case side_type::tri_02:
			nt = side_type::tri_13;
			break;
		case side_type::tri_13:
			nt = side_type::tri_02;
			break;
		default:
			return;
	}
	s.set_type(nt);
}
#endif
}

//heavy-duty error checking
int check_segment_connections(void)
{
	int errors=0;

	range_for (const auto &&seg, vmsegptridx)
	{
		for (const auto sidenum : MAX_SIDES_PER_SEGMENT)
		{
#ifndef NDEBUG
			const auto &&[num_faces, vertex_list] = create_abs_vertex_lists(seg, sidenum);
#endif
			const auto csegnum = seg->shared_segment::children[sidenum];
			if (!IS_CHILD(csegnum))
				continue;
			{
				auto cseg = vcsegptr(csegnum);
				auto csidenum = find_connect_side(seg,cseg);

				if (csidenum == side_none)
				{
					shared_segment &rseg = *seg;
					const shared_segment &rcseg = *cseg;
					const auto segi = underlying_value(seg.get_unchecked_index());
					const auto csegi = underlying_value(csegnum);
					LevelError("Segment #%hu side %u has asymmetric link to segment %hu.  Coercing to segment_none; Segments[%hu].children={%hu, %hu, %hu, %hu, %hu, %hu}, Segments[%u].children={%hu, %hu, %hu, %hu, %hu, %hu}.", segi, underlying_value(sidenum), csegi, segi, rseg.children[sidenum_t::WLEFT], rseg.children[sidenum_t::WTOP], rseg.children[sidenum_t::WRIGHT], rseg.children[sidenum_t::WBOTTOM], rseg.children[sidenum_t::WBACK], rseg.children[sidenum_t::WFRONT], csegi, rcseg.children[sidenum_t::WLEFT], rcseg.children[sidenum_t::WTOP], rcseg.children[sidenum_t::WRIGHT], rcseg.children[sidenum_t::WBOTTOM], rcseg.children[sidenum_t::WBACK], rcseg.children[sidenum_t::WFRONT]);
					rseg.children[sidenum] = segment_none;
					errors = 1;
					continue;
				}

#ifndef NDEBUG
				const auto &&[con_num_faces, con_vertex_list] = create_abs_vertex_lists(cseg, csidenum);

				if (con_num_faces != num_faces) {
					LevelError("Segment #%u side %u: wrong faces: con_num_faces=%" PRIuFAST32 " num_faces=%" PRIuFAST32 ".", seg.get_unchecked_index(), underlying_value(sidenum), con_num_faces, num_faces);
					errors = 1;
				}
				else
					if (num_faces == 1) {
						int t;

						for (t=0;t<4 && con_vertex_list[t]!=vertex_list[0];t++);

						if (t==4 ||
							 vertex_list[0] != con_vertex_list[t] ||
							 vertex_list[1] != con_vertex_list[(t+3)%4] ||
							 vertex_list[2] != con_vertex_list[(t+2)%4] ||
							 vertex_list[3] != con_vertex_list[(t+1)%4]) {
							LevelError("Segment #%u side %u: bad vertices.", seg.get_unchecked_index(), underlying_value(sidenum));
							errors = 1;
						}
						else
							errors |= check_norms(seg,sidenum,0,cseg,csidenum,0);
	
					}
					else {
	
						if (vertex_list[1] == con_vertex_list[1]) {
		
							if (vertex_list[4] != con_vertex_list[4] ||
								 vertex_list[0] != con_vertex_list[2] ||
								 vertex_list[2] != con_vertex_list[0] ||
								 vertex_list[3] != con_vertex_list[5] ||
								 vertex_list[5] != con_vertex_list[3]) {
								auto &cside = vmsegptr(csegnum)->shared_segment::sides[csidenum];
								invert_shared_side_triangle_type(cside);
							} else {
								errors |= check_norms(seg,sidenum,0,cseg,csidenum,0);
								errors |= check_norms(seg,sidenum,1,cseg,csidenum,1);
							}
	
						} else {
		
							if (vertex_list[1] != con_vertex_list[4] ||
								 vertex_list[4] != con_vertex_list[1] ||
								 vertex_list[0] != con_vertex_list[5] ||
								 vertex_list[5] != con_vertex_list[0] ||
								 vertex_list[2] != con_vertex_list[3] ||
								 vertex_list[3] != con_vertex_list[2]) {
								auto &cside = vmsegptr(csegnum)->shared_segment::sides[csidenum];
								invert_shared_side_triangle_type(cside);
							} else {
								errors |= check_norms(seg,sidenum,0,cseg,csidenum,1);
								errors |= check_norms(seg,sidenum,1,cseg,csidenum,0);
							}
						}
					}
#endif
			}
		}
	}
	return errors;
}

// Used to become a constant based on editor, but I wanted to be able to set
// this for omega blob find_point_seg calls.
// Would be better to pass a paremeter to the routine...--MK, 01/17/96
#if defined(DXX_BUILD_DESCENT_II) || DXX_USE_EDITOR
int	Doing_lighting_hack_flag=0;
#else
#define Doing_lighting_hack_flag 0
#endif

namespace {
// figure out what seg the given point is in, tracing through segments
// returns segment number, or -1 if can't find segment
static icsegptridx_t trace_segs(const d_level_shared_segment_state &LevelSharedSegmentState, const vms_vector &p0, const vcsegptridx_t oldsegnum, const unsigned recursion_count, visited_segment_bitarray_t &visited)
{
	if (recursion_count >= LevelSharedSegmentState.Num_segments) {
		con_puts(CON_DEBUG, "trace_segs: Segment not found");
		return segment_none;
	}
	if (auto &&vs = visited[oldsegnum])
		return segment_none;
	else
		vs = true;

	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	per_side_array<fix> side_dists;
	const auto centermask = get_side_dists(Vertices.vcptr, p0, oldsegnum, side_dists);		//check old segment
	if (centermask == sidemask_t{}) // we are in the old segment
		return oldsegnum; //..say so

	auto &children = oldsegnum->shared_segment::children;
	for (;;) {
		std::optional<sidenum_t> biggest_side;
		fix biggest_val = 0;
		for (const auto sidenum : MAX_SIDES_PER_SEGMENT)
		{
			const auto bit = build_sidemask(sidenum);
			if ((centermask & bit) && IS_CHILD(children[sidenum])
			    && side_dists[sidenum] < biggest_val) {
				biggest_val = side_dists[sidenum];
				biggest_side = sidenum;
			}
		}

		if (!biggest_side)
			break;

		side_dists[*biggest_side] = 0;
		// trace into adjacent segment:
		const auto &&check = trace_segs(LevelSharedSegmentState, p0, oldsegnum.absolute_sibling(children[*biggest_side]), recursion_count + 1, visited);
		if (check != segment_none)		//we've found a segment
			return check;
	}
	return segment_none;		//we haven't found a segment
}
}

imsegptridx_t find_point_seg(const d_level_shared_segment_state &LevelSharedSegmentState, d_level_unique_segment_state &, const vms_vector &p, const imsegptridx_t segnum)
{
	return segnum.rebind_policy(find_point_seg(LevelSharedSegmentState, p, segnum));
}

//Tries to find a segment for a point, in the following way:
// 1. Check the given segment
// 2. Recursively trace through attached segments
// 3. Check all the segments
//Returns segnum if found, or -1
icsegptridx_t find_point_seg(const d_level_shared_segment_state &LevelSharedSegmentState, const vms_vector &p, const icsegptridx_t segnum)
{
	//allow segnum==-1, meaning we have no idea what segment point is in
	if (segnum != segment_none) {
		visited_segment_bitarray_t visited;
		const auto &&newseg = trace_segs(LevelSharedSegmentState, p, segnum, 0, visited);
		if (newseg != segment_none)			//we found a segment!
			return newseg;
	}

	//couldn't find via attached segs, so search all segs

	//	MK: 10/15/94
	//	This Doing_lighting_hack_flag thing added by mk because the hundreds of scrolling messages were
	//	slowing down lighting, and in about 98% of cases, it would just return -1 anyway.
	//	Matt: This really should be fixed, though.  We're probably screwing up our lighting in a few places.
	if (!Doing_lighting_hack_flag) {
		auto &Segments = LevelSharedSegmentState.get_segments();
		auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
		auto &Vertices = LevelSharedVertexState.get_vertices();
		range_for (const auto &&segp, Segments.vmptridx)
		{
			if (get_seg_masks(Vertices.vcptr, p, segp, 0).centermask == sidemask_t{})
				return segp;
		}
	}
	return segment_none;
}


//--repair-- //	------------------------------------------------------------------------------
//--repair-- void clsd_repair_center(int segnum)
//--repair-- {
//--repair-- 	int	sidenum;
//--repair--
//--repair-- 	//	--- Set repair center bit for all repair center segments.
//--repair-- 	if (Segments[segnum].special == SEGMENT_IS_REPAIRCEN) {
//--repair-- 		Lsegments[segnum].special_type |= SS_REPAIR_CENTER;
//--repair-- 		Lsegments[segnum].special_segment = segnum;
//--repair-- 	}
//--repair--
//--repair-- 	//	--- Set repair center bit for all segments adjacent to a repair center.
//--repair-- 	for (sidenum=0; sidenum < MAX_SIDES_PER_SEGMENT; sidenum++) {
//--repair-- 		int	s = Segments[segnum].children[sidenum];
//--repair--
//--repair-- 		if ( (s != -1) && (Segments[s].special==SEGMENT_IS_REPAIRCEN) ) {
//--repair-- 			Lsegments[segnum].special_type |= SS_REPAIR_CENTER;
//--repair-- 			Lsegments[segnum].special_segment = s;
//--repair-- 		}
//--repair-- 	}
//--repair-- }

//--repair-- //	------------------------------------------------------------------------------
//--repair-- //	--- Set destination points for all Materialization centers.
//--repair-- void clsd_materialization_center(int segnum)
//--repair-- {
//--repair-- 	if (Segments[segnum].special == SEGMENT_IS_ROBOTMAKER) {
//--repair--
//--repair-- 	}
//--repair-- }
//--repair--
//--repair-- int	Lsegment_highest_segment_index, Lsegment_highest_vertex_index;
//--repair--
//--repair-- //	------------------------------------------------------------------------------
//--repair-- //	Create data specific to mine which doesn't get written to disk.
//--repair-- //	Highest_segment_index and Highest_object_index must be valid.
//--repair-- //	07/21:	set repair center bit
//--repair-- void create_local_segment_data(void)
//--repair-- {
//--repair-- 	int	segnum;
//--repair--
//--repair-- 	//	--- Initialize all Lsegments.
//--repair-- 	for (segnum=0; segnum <= Highest_segment_index; segnum++) {
//--repair-- 		Lsegments[segnum].special_type = 0;
//--repair-- 		Lsegments[segnum].special_segment = -1;
//--repair-- 	}
//--repair--
//--repair-- 	for (segnum=0; segnum <= Highest_segment_index; segnum++) {
//--repair--
//--repair-- 		clsd_repair_center(segnum);
//--repair-- 		clsd_materialization_center(segnum);
//--repair-- 	
//--repair-- 	}
//--repair--
//--repair-- 	//	Set check variables.
//--repair-- 	//	In main game loop, make sure these are valid, else Lsegments is not valid.
//--repair-- 	Lsegment_highest_segment_index = Highest_segment_index;
//--repair-- 	Lsegment_highest_vertex_index = Highest_vertex_index;
//--repair-- }
//--repair--
//--repair-- //	------------------------------------------------------------------------------------------
//--repair-- //	Sort of makes sure create_local_segment_data has been called for the currently executing mine.
//--repair-- //	It is not failsafe, as you will see if you look at the code.
//--repair-- //	Returns 1 if Lsegments appears valid, 0 if not.
//--repair-- int check_lsegments_validity(void)
//--repair-- {
//--repair-- 	return ((Lsegment_highest_segment_index == Highest_segment_index) && (Lsegment_highest_vertex_index == Highest_vertex_index));
//--repair-- }

#if defined(DXX_BUILD_DESCENT_I)
namespace {
static inline void add_to_fcd_cache(segnum_t seg0, segnum_t seg1, vm_distance dist)
{
	(void)seg0;
	(void)seg1;
	(void)dist;
}
}
#elif defined(DXX_BUILD_DESCENT_II)
#define	MIN_CACHE_FCD_DIST	(F1_0*80)	//	Must be this far apart for cache lookup to succeed.  Recognizes small changes in distance matter at small distances.
namespace {

struct fcd_data {
	segnum_t	seg0, seg1;
	vm_distance dist;
};

fix64	Last_fcd_flush_time;
unsigned Fcd_index;
std::array<fcd_data, 8> Fcd_cache;

static void add_to_fcd_cache(const segnum_t seg0, const segnum_t seg1, const vm_distance dist)
{
	if (dist > MIN_CACHE_FCD_DIST) {
		if (Fcd_index >= Fcd_cache.size())
			Fcd_index = 0;
		auto &f = Fcd_cache[Fcd_index++];
		f.seg0 = seg0;
		f.seg1 = seg1;
		f.dist = dist;
	} else {
		//	If it's in the cache, remove it.
		for (auto &f : Fcd_cache)
			if (f.seg0 == seg0 && f.seg1 == seg1)
			{
				f.seg0 = segment_none;
				break;
			}
	}
}

}

//	----------------------------------------------------------------------------------------------------------
void flush_fcd_cache()
{
	Fcd_index = 0;
	Last_fcd_flush_time = {GameTime64};

	for (auto &f : Fcd_cache)
		f.seg0 = segment_none;
}
#endif

//	----------------------------------------------------------------------------------------------------------
//	Determine whether seg0 and seg1 are reachable in a way that allows sound to pass.
//	Search up to a maximum depth of max_depth.
//	Return the distance.
vm_distance find_connected_distance(const vms_vector &p0, const vcsegptridx_t seg0, const vms_vector &p1, const vcsegptridx_t seg1, int max_depth, const wall_is_doorway_mask wid_flag)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	segnum_t		cur_seg;
	int		qtail = 0, qhead = 0;
	seg_seg	seg_queue[MAX_SEGMENTS];
	int		cur_depth;
	int		num_points;
	struct point_seg
	{
		vms_vector point;
	};
	std::array<point_seg, 64> point_segs;

	//	If > this, will overrun point_segs buffer
#ifdef WINDOWS
	if (max_depth == -1) max_depth = 200;
#endif	

	if (const auto s = std::size(point_segs) - 2u; max_depth > s)
		max_depth = s;

	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcwallptr = Walls.vcptr;
	if (seg0 == seg1) {
		return vm_vec_dist_quick(p0, p1);
	} else {
		auto conn_side = find_connect_side(seg0, seg1);
		if (conn_side != side_none)
		{
#if defined(DXX_BUILD_DESCENT_II)
			if (WALL_IS_DOORWAY(GameBitmaps, Textures, vcwallptr, seg1, conn_side) & wid_flag)
#endif
			{
				return vm_vec_dist_quick(p0, p1);
			}
		}
	}

#if defined(DXX_BUILD_DESCENT_II)
	//	Periodically flush cache.
	if ((GameTime64 - Last_fcd_flush_time > F1_0*2) || (GameTime64 < Last_fcd_flush_time)) {
		flush_fcd_cache();
	}

	else
	//	Can't quickly get distance, so see if in Fcd_cache.
	range_for (auto &i, Fcd_cache)
		if (i.seg0 == seg0 && i.seg1 == seg1)
		{
			return i.dist;
		}
#endif

	num_points = 0;

	std::array<uint16_t, MAX_SEGMENTS> depth{};
	visited_segment_bitarray_t visited;

	cur_seg = seg0;
	visited[cur_seg] = true;
	cur_depth = 0;

	while (cur_seg != seg1) {
		const cscusegment segp = *vmsegptr(cur_seg);
		for (const auto snum : MAX_SIDES_PER_SEGMENT)
		{
			const auto this_seg = segp.s.children[snum];
			if (!IS_CHILD(this_seg))
				continue;
			auto &&v = visited[this_seg];
			if (v)
				continue;
			if (wid_flag == wall_is_doorway_mask::None || (WALL_IS_DOORWAY(GameBitmaps, Textures, vcwallptr, segp, snum) & wid_flag))
			{
				v = true;
					seg_queue[qtail].start = cur_seg;
					seg_queue[qtail].end = this_seg;
					depth[qtail++] = cur_depth+1;
					if (max_depth != -1) {
						if (depth[qtail-1] == max_depth) {
							add_to_fcd_cache(seg0, seg1, fcd_abort_cache_value);
							return fcd_abort_return_value;
						}
					} else if (this_seg == seg1) {
						goto fcd_done1;
					}
			}
		}	//	for (sidenum...

		if (qhead >= qtail) {
			add_to_fcd_cache(seg0, seg1, fcd_abort_cache_value);
			return fcd_abort_return_value;
		}

		cur_seg = seg_queue[qhead].end;
		cur_depth = depth[qhead];
		qhead++;

fcd_done1: ;
	}	//	while (cur_seg ...

	//	Set qtail to the segment which ends at the goal.
	while (seg_queue[--qtail].end != seg1)
		if (qtail < 0) {
			add_to_fcd_cache(seg0, seg1, fcd_abort_cache_value);
			return fcd_abort_return_value;
		}

	auto &vcvertptr = Vertices.vcptr;
	while (qtail >= 0) {
		segnum_t	parent_seg, this_seg;

		this_seg = seg_queue[qtail].end;
		parent_seg = seg_queue[qtail].start;
		compute_segment_center(vcvertptr, point_segs[num_points].point, vcsegptr(this_seg));
		num_points++;

		if (parent_seg == seg0)
			break;

		while (seg_queue[--qtail].end != parent_seg)
			Assert(qtail >= 0);
	}

	compute_segment_center(vcvertptr, point_segs[num_points].point,seg0);
	num_points++;

	if (num_points == 1) {
		return vm_vec_dist_quick(p0, p1);
	}
	auto dist = vm_vec_dist_quick(p1, point_segs[1].point);
		dist += vm_vec_dist_quick(p0, point_segs[num_points-2].point);

		for (int i=1; i<num_points-2; i++) {
			dist += vm_vec_dist_quick(point_segs[i].point, point_segs[i+1].point);
		}

	add_to_fcd_cache(seg0, seg1, dist);

	return dist;

}

}

namespace dcx {
namespace {

static sbyte convert_to_byte(fix f)
{
	const uint8_t MATRIX_MAX = 0x7f;    // This is based on MATRIX_PRECISION, 9 => 0x7f
	if (f >= 0x00010000)
		return MATRIX_MAX;
	else if (f <= -0x00010000)
		return -MATRIX_MAX;
	else
		return f >> MATRIX_PRECISION;
}

}
}

#define VEL_PRECISION 12

namespace dsx {

//	Create a shortpos struct from an object.
//	Extract the matrix into byte values.
//	Create a position relative to vertex 0 with 1/256 normal "fix" precision.
//	Stuff segment in a short.
void create_shortpos_native(const d_level_shared_segment_state &LevelSharedSegmentState, shortpos &spp, const object_base &objp)
{
	auto &vcsegptr = LevelSharedSegmentState.get_segments().vcptr;
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vcvertptr = Vertices.vcptr;
	spp.bytemat[0] = convert_to_byte(objp.orient.rvec.x);
	spp.bytemat[1] = convert_to_byte(objp.orient.uvec.x);
	spp.bytemat[2] = convert_to_byte(objp.orient.fvec.x);
	spp.bytemat[3] = convert_to_byte(objp.orient.rvec.y);
	spp.bytemat[4] = convert_to_byte(objp.orient.uvec.y);
	spp.bytemat[5] = convert_to_byte(objp.orient.fvec.y);
	spp.bytemat[6] = convert_to_byte(objp.orient.rvec.z);
	spp.bytemat[7] = convert_to_byte(objp.orient.uvec.z);
	spp.bytemat[8] = convert_to_byte(objp.orient.fvec.z);

	spp.segment = objp.segnum;
	const shared_segment &segp = *vcsegptr(objp.segnum);
	auto &vert = *vcvertptr(segp.verts[segment_relative_vertnum::_0]);
	spp.xo = (objp.pos.x - vert.x) >> RELPOS_PRECISION;
	spp.yo = (objp.pos.y - vert.y) >> RELPOS_PRECISION;
	spp.zo = (objp.pos.z - vert.z) >> RELPOS_PRECISION;

	spp.velx = (objp.mtype.phys_info.velocity.x) >> VEL_PRECISION;
	spp.vely = (objp.mtype.phys_info.velocity.y) >> VEL_PRECISION;
	spp.velz = (objp.mtype.phys_info.velocity.z) >> VEL_PRECISION;
}

void create_shortpos_little(const d_level_shared_segment_state &LevelSharedSegmentState, shortpos &spp, const object_base &objp)
{
	create_shortpos_native(LevelSharedSegmentState, spp, objp);
// swap the short values for the big-endian machines.

	if constexpr (words_bigendian)
	{
		spp.xo = INTEL_SHORT(spp.xo);
		spp.yo = INTEL_SHORT(spp.yo);
		spp.zo = INTEL_SHORT(spp.zo);
		spp.segment = segnum_t{INTEL_SHORT(underlying_value(spp.segment))};
		spp.velx = INTEL_SHORT(spp.velx);
		spp.vely = INTEL_SHORT(spp.vely);
		spp.velz = INTEL_SHORT(spp.velz);
	}
}

void extract_shortpos_little(const vmobjptridx_t objp, const shortpos *spp)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vmobjptr = Objects.vmptr;
	auto sp = spp->bytemat.data();

	objp->orient.rvec.x = *sp++ << MATRIX_PRECISION;
	objp->orient.uvec.x = *sp++ << MATRIX_PRECISION;
	objp->orient.fvec.x = *sp++ << MATRIX_PRECISION;
	objp->orient.rvec.y = *sp++ << MATRIX_PRECISION;
	objp->orient.uvec.y = *sp++ << MATRIX_PRECISION;
	objp->orient.fvec.y = *sp++ << MATRIX_PRECISION;
	objp->orient.rvec.z = *sp++ << MATRIX_PRECISION;
	objp->orient.uvec.z = *sp++ << MATRIX_PRECISION;
	objp->orient.fvec.z = *sp++ << MATRIX_PRECISION;

	const auto segnum = segnum_t{INTEL_SHORT(underlying_value(spp->segment))};

	Assert(segnum <= Highest_segment_index);

	const auto &&segp = vmsegptridx(segnum);
	auto &vcvertptr = Vertices.vcptr;
	auto &vp = *vcvertptr(segp->verts[segment_relative_vertnum::_0]);
	objp->pos.x = (INTEL_SHORT(spp->xo) << RELPOS_PRECISION) + vp.x;
	objp->pos.y = (INTEL_SHORT(spp->yo) << RELPOS_PRECISION) + vp.y;
	objp->pos.z = (INTEL_SHORT(spp->zo) << RELPOS_PRECISION) + vp.z;

	objp->mtype.phys_info.velocity.x = (INTEL_SHORT(spp->velx) << VEL_PRECISION);
	objp->mtype.phys_info.velocity.y = (INTEL_SHORT(spp->vely) << VEL_PRECISION);
	objp->mtype.phys_info.velocity.z = (INTEL_SHORT(spp->velz) << VEL_PRECISION);

	obj_relink(vmobjptr, vmsegptr, objp, segp);
}

// create and extract quaternion structure from object data which greatly saves bytes by using quaternion instead or orientation matrix
void create_quaternionpos(quaternionpos &qpp, const object_base &objp)
{
	vms_quaternion_from_matrix(qpp.orient, objp.orient);

	qpp.pos = objp.pos;
	qpp.segment = objp.segnum;
	qpp.vel = objp.mtype.phys_info.velocity;
	qpp.rotvel = objp.mtype.phys_info.rotvel;
}

void extract_quaternionpos(const vmobjptridx_t objp, quaternionpos &qpp)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	vms_matrix_from_quaternion(objp->orient, qpp.orient);

	objp->pos = qpp.pos;
	objp->mtype.phys_info.velocity = qpp.vel;
	objp->mtype.phys_info.rotvel = qpp.rotvel;
        
	const auto segnum = qpp.segment;
	Assert(segnum <= Highest_segment_index);
	obj_relink(vmobjptr, vmsegptr, objp, vmsegptridx(segnum));
}


//	-----------------------------------------------------------------------------
//	Segment validation functions.
//	Moved from editor to game so we can compute surface normals at load time.
// -------------------------------------------------------------------------------

namespace {
// ------------------------------------------------------------------------------------------
//	Extract a vector from a segment.  The vector goes from the start face to the end face.
//	The point on each face is the average of the four points forming the face.
[[nodiscard]]
static vms_vector extract_vector_from_segment(fvcvertptr &vcvertptr, const shared_segment &sp, const sidenum_t istart, const sidenum_t iend)
{
	vms_vector vp{};
	auto &start = Side_to_verts[istart];
	auto &end = Side_to_verts[iend];
	auto &verts = sp.verts;
	for (const auto &&[vs, ve] : zip(start, end))
	{
		vm_vec_sub2(vp, vcvertptr(verts[vs]));
		vm_vec_add2(vp, vcvertptr(verts[ve]));
	}
	vm_vec_scale(vp,F1_0/4);
	return vp;
}
}

//create a matrix that describes the orientation of the given segment
void extract_orient_from_segment(fvcvertptr &vcvertptr, vms_matrix &m, const shared_segment &seg)
{
	const auto fvec{extract_vector_from_segment(vcvertptr, seg, sidenum_t::WFRONT, sidenum_t::WBACK)};
	const auto uvec{extract_vector_from_segment(vcvertptr, seg, sidenum_t::WBOTTOM, sidenum_t::WTOP)};

	//vector to matrix does normalizations and orthogonalizations
	vm_vector_2_matrix(m, fvec, &uvec, nullptr);
}

#if !DXX_USE_EDITOR
namespace {
#endif

// ------------------------------------------------------------------------------------------
//	Extract the forward vector from segment *sp, return in *vp.
//	The forward vector is defined to be the vector from the the center of the front face of the segment
// to the center of the back face of the segment.
void extract_forward_vector_from_segment(fvcvertptr &vcvertptr, const shared_segment &sp, vms_vector &vp)
{
	vp = extract_vector_from_segment(vcvertptr, sp, sidenum_t::WFRONT, sidenum_t::WBACK);
}

// ------------------------------------------------------------------------------------------
//	Extract the right vector from segment *sp, return in *vp.
//	The forward vector is defined to be the vector from the the center of the left face of the segment
// to the center of the right face of the segment.
vms_vector extract_right_vector_from_segment(fvcvertptr &vcvertptr, const shared_segment &sp)
{
	return extract_vector_from_segment(vcvertptr, sp, sidenum_t::WLEFT, sidenum_t::WRIGHT);
}

// ------------------------------------------------------------------------------------------
//	Extract the up vector from segment *sp, return in *vp.
//	The forward vector is defined to be the vector from the the center of the bottom face of the segment
// to the center of the top face of the segment.
void extract_up_vector_from_segment(fvcvertptr &vcvertptr, const shared_segment &sp, vms_vector &vp)
{
	vp = extract_vector_from_segment(vcvertptr, sp, sidenum_t::WBOTTOM, sidenum_t::WTOP);
}

#if !DXX_USE_EDITOR
}
#endif

namespace {

//	----
//	A side is determined to be degenerate if the cross products of 3 consecutive points does not point outward.
[[nodiscard]]
static unsigned check_for_degenerate_side(fvcvertptr &vcvertptr, const shared_segment &sp, const sidenum_t sidenum)
{
	auto &vp = Side_to_verts[sidenum];
	vms_vector	vec1, vec2;
	fix			dot;
	int			degeneracy_flag = 0;

	const auto segc = compute_segment_center(vcvertptr, sp);
	const auto &&sidec = compute_center_point_on_side(vcvertptr, sp, sidenum);
	const auto vec_to_center = vm_vec_sub(segc, sidec);

	//vm_vec_sub(&vec1, &Vertices[sp->verts[vp[1]]], &Vertices[sp->verts[vp[0]]]);
	//vm_vec_sub(&vec2, &Vertices[sp->verts[vp[2]]], &Vertices[sp->verts[vp[1]]]);
	//vm_vec_normalize(&vec1);
	//vm_vec_normalize(&vec2);
	const auto vp1 = vp[side_relative_vertnum::_1];
	const auto vp2 = vp[side_relative_vertnum::_2];
	auto &vert1 = *vcvertptr(sp.verts[vp1]);
	auto &vert2 = *vcvertptr(sp.verts[vp2]);
	vm_vec_normalized_dir(vec1, vert1, vcvertptr(sp.verts[vp[side_relative_vertnum::_0]]));
	vm_vec_normalized_dir(vec2, vert2, vert1);
	const auto cross0{vm_vec_cross(vec1, vec2)};

	dot = vm_vec_dot(vec_to_center, cross0);
	if (dot <= 0)
		degeneracy_flag |= 1;

	//vm_vec_sub(&vec1, &Vertices[sp->verts[vp[2]]], &Vertices[sp->verts[vp[1]]]);
	//vm_vec_sub(&vec2, &Vertices[sp->verts[vp[3]]], &Vertices[sp->verts[vp[2]]]);
	//vm_vec_normalize(&vec1);
	//vm_vec_normalize(&vec2);
	vm_vec_normalized_dir(vec1, vert2, vert1);
	vm_vec_normalized_dir(vec2, vcvertptr(sp.verts[vp[side_relative_vertnum::_3]]), vert2);
	const auto cross1{vm_vec_cross(vec1, vec2)};

	dot = vm_vec_dot(vec_to_center, cross1);
	if (dot <= 0)
		degeneracy_flag |= 1;

	return degeneracy_flag;
}

//	----
//	See if a segment has gotten turned inside out, or something.
//	If so, set global Degenerate_segment_found and return 1, else return 0.
static unsigned check_for_degenerate_segment(fvcvertptr &vcvertptr, const shared_segment &sp)
{
	vms_vector	fvec, uvec;
	fix			dot;
	int			degeneracy_flag = 0;				// degeneracy flag for current segment

	extract_forward_vector_from_segment(vcvertptr, sp, fvec);
	auto rvec{extract_right_vector_from_segment(vcvertptr, sp)};
	extract_up_vector_from_segment(vcvertptr, sp, uvec);

	vm_vec_normalize(fvec);
	vm_vec_normalize(rvec);
	vm_vec_normalize(uvec);

	const auto cross{vm_vec_cross(fvec, rvec)};
	dot = vm_vec_dot(cross, uvec);

	if (dot > 0)
		degeneracy_flag = 0;
	else {
		degeneracy_flag = 1;
	}

	//	Now, see if degenerate because of any side.
	for (const auto i : MAX_SIDES_PER_SEGMENT)
		degeneracy_flag |= check_for_degenerate_side(vcvertptr, sp, i);

#if DXX_USE_EDITOR
	Degenerate_segment_found |= degeneracy_flag;
#endif

	return degeneracy_flag;

}

static void add_side_as_quad(shared_side &sidep, const vms_vector &normal)
{
	sidep.set_type(side_type::quad);
	sidep.normals[0] = normal;
	sidep.normals[1] = normal;
	//	If there is a connection here, we only formed the faces for the purpose of determining segment boundaries,
	//	so don't generate polys, else they will get rendered.
//	if (sp->children[sidenum] != -1)
//		sidep->render_flag = 0;
//	else
//		sidep->render_flag = 1;
}

}

}

namespace dcx {
namespace {

// -------------------------------------------------------------------------------
//	Return v0, v1, v2 = 3 vertices with smallest numbers.  If *negate_flag set, then negate normal after computation.
//	Note, you cannot just compute the normal by treating the points in the opposite direction as this introduces
//	small differences between normals which should merely be opposites of each other.
static bool get_verts_for_normal(verts_for_normal &r)
{
	auto &v = r.vsorted;
	std::array<uint8_t, 4> w;
	//	w is a list that shows how things got scrambled so we know if our normal is pointing backwards
	std::iota(w.begin(), w.end(), 0);

	range_for (const unsigned i, xrange(1u, 4u))
		range_for (const unsigned j, xrange(i))
			if (v[j] > v[i]) {
				using std::swap;
				swap(v[j], v[i]);
				swap(w[j], w[i]);
			}

	if (!((v[0] < v[1]) && (v[1] < v[2]) && (v[2] < v[3])))
		LevelError("Level contains malformed geometry.");

	//	Now, if for any w[i] & w[i+1]: w[i+1] = (w[i]+3)%4, then must swap
	return ((w[0] + 3) % 4) == w[1] || ((w[1] + 3) % 4) == w[2];
}

static void assign_side_normal(fvcvertptr &vcvertptr, vms_vector &n, const vertnum_t v0, const vertnum_t v1, const vertnum_t v2)
{
	verts_for_normal vfn{v0, v1, v2, vertnum_t{UINT32_MAX}};
	const auto negate_flag = get_verts_for_normal(vfn);
	n = vm_vec_normal(vcvertptr(vfn.vsorted[0]), vcvertptr(vfn.vsorted[1]), vcvertptr(vfn.vsorted[2]));
	if (negate_flag)
		vm_vec_negate(n);
}

}
}

namespace dsx {
namespace {

// -------------------------------------------------------------------------------
static void add_side_as_2_triangles(fvcvertptr &vcvertptr, shared_segment &sp, const sidenum_t sidenum)
{
	auto &vs = Side_to_verts[sidenum];
	fix			dot;

	const auto sidep = &sp.sides[sidenum];

	//	Choose how to triangulate.
	//	If a wall, then
	//		Always triangulate so segment is convex.
	//		Use Matt's formula: Na . AD > 0, where ABCD are vertices on side, a is face formed by A,B,C, Na is normal from face a.
	//	If not a wall, then triangulate so whatever is on the other side is triangulated the same (ie, between the same absoluate vertices)
	if (!IS_CHILD(sp.children[sidenum]))
	{
		auto &verts = sp.verts;
		auto &vvs0 = *vcvertptr(verts[vs[side_relative_vertnum::_0]]);
		auto &vvs1 = *vcvertptr(verts[vs[side_relative_vertnum::_1]]);
		auto &vvs2 = *vcvertptr(verts[vs[side_relative_vertnum::_2]]);
		auto &vvs3 = *vcvertptr(verts[vs[side_relative_vertnum::_3]]);
		const auto norm{vm_vec_normal(vvs0, vvs1, vvs2)};
		const auto &&vec_13 =	vm_vec_sub(vvs3, vvs1);	//	vector from vertex 1 to vertex 3
		dot = vm_vec_dot(norm, vec_13);

		const vertex *n0v3, *n1v1;
		//	Now, signifiy whether to triangulate from 0:2 or 1:3
		sidep->set_type(dot >= 0 ? (n0v3 = &vvs2, n1v1 = &vvs0, side_type::tri_02) : (n0v3 = &vvs3, n1v1 = &vvs1, side_type::tri_13));

		//	Now, based on triangulation type, set the normals.
		sidep->normals[0] = vm_vec_normal(vvs0, vvs1, *n0v3);
		sidep->normals[1] = vm_vec_normal(*n1v1, vvs2, vvs3);
	} else {
		enumerated_array<vertnum_t, 4, side_relative_vertnum> v;

		for (const auto i : MAX_VERTICES_PER_SIDE)
			v[i] = sp.verts[vs[i]];

		verts_for_normal vfn{v[side_relative_vertnum::_0], v[side_relative_vertnum::_1], v[side_relative_vertnum::_2], v[side_relative_vertnum::_3]};
		get_verts_for_normal(vfn);

		vertnum_t s0v2, s1v0;
		if ((vfn.vsorted[0] == v[side_relative_vertnum::_0]) || (vfn.vsorted[0] == v[side_relative_vertnum::_2])) {
			sidep->set_type(side_type::tri_02);
			//	Now, get vertices for normal for each triangle based on triangulation type.
			s0v2 = v[side_relative_vertnum::_2];
			s1v0 = v[side_relative_vertnum::_0];
		} else {
			sidep->set_type(side_type::tri_13);
			//	Now, get vertices for normal for each triangle based on triangulation type.
			s0v2 = v[side_relative_vertnum::_3];
			s1v0 = v[side_relative_vertnum::_1];
		}
		assign_side_normal(vcvertptr, sidep->normals[0], v[side_relative_vertnum::_0], v[side_relative_vertnum::_1], s0v2);
		assign_side_normal(vcvertptr, sidep->normals[1], s1v0, v[side_relative_vertnum::_2], v[side_relative_vertnum::_3]);
	}
}

}
}

namespace dcx {
namespace {

static int sign(fix v)
{

	if (v > PLANE_DIST_TOLERANCE)
		return 1;
	else if (v < -(PLANE_DIST_TOLERANCE+1))		//neg & pos round differently
		return -1;
	else
		return 0;
}

}
}

namespace dsx {

#if !DXX_USE_EDITOR
namespace {
#endif

// -------------------------------------------------------------------------------
void create_walls_on_side(fvcvertptr &vcvertptr, shared_segment &sp, const sidenum_t sidenum)
{
	auto &vs = Side_to_verts[sidenum];
	const auto v0 = sp.verts[vs[side_relative_vertnum::_0]];
	const auto v1 = sp.verts[vs[side_relative_vertnum::_1]];
	const auto v2 = sp.verts[vs[side_relative_vertnum::_2]];
	const auto v3 = sp.verts[vs[side_relative_vertnum::_3]];

	verts_for_normal vfn{v0, v1, v2, v3};
	const auto negate_flag = get_verts_for_normal(vfn);
	auto &vvm0 = *vcvertptr(vfn.vsorted[0]);
	auto vn{vm_vec_normal(vvm0, vcvertptr(vfn.vsorted[1]), vcvertptr(vfn.vsorted[2]))};
	const fix dist_to_plane = abs(vm_dist_to_plane(vcvertptr(vfn.vsorted[3]), vn, vvm0));

	if (negate_flag)
		vm_vec_negate(vn);

	auto &s = sp.sides[sidenum];
	if (dist_to_plane > PLANE_DIST_TOLERANCE)
	{
		add_side_as_2_triangles(vcvertptr, sp, sidenum);

		//this code checks to see if we really should be triangulated, and
		//de-triangulates if we shouldn't be.

			int			s0,s1;

			const auto &&[num_faces, vertex_list] = create_abs_vertex_lists(sp, s, sidenum);
			assert(num_faces == 2);
			(void)num_faces;

			auto &vvn = *vcvertptr(min(vertex_list[0],vertex_list[2]));

			const fix dist0 = vm_dist_to_plane(vcvertptr(vertex_list[1]), s.normals[1], vvn);
			const fix dist1 = vm_dist_to_plane(vcvertptr(vertex_list[4]), s.normals[0], vvn);

			s0 = sign(dist0);
			s1 = sign(dist1);

		if (!(s0 == 0 || s1 == 0 || s0 != s1))
			return;
		//detriangulate!
	}
	add_side_as_quad(s, vn);
}

// -------------------------------------------------------------------------------
//	Make a just-modified segment side valid.
void validate_segment_side(fvcvertptr &vcvertptr, const vmsegptridx_t sp, const sidenum_t sidenum)
{
	auto &sside = sp->shared_segment::sides[sidenum];
	auto &uside = sp->unique_segment::sides[sidenum];
	create_walls_on_side(vcvertptr, sp, sidenum);
	/*
	 * If the texture was wrong, then fix it and log a diagnostic.  For
	 * builtin missions, log the diagnostic at level CON_VERBOSE, since
	 * retail levels trigger this during normal play.  For external
	 * missions, log the diagnostic at level CON_URGENT.  External
	 * levels might be fixable by contacting the author, but the retail
	 * levels can only be fixed by using a Rebirth level patch file (not
	 * supported yet).  When fixing the texture, change it to 0 for
	 * walls and 1 for non-walls.  This should make walls transparent
	 * for their primary texture; transparent non-walls usually generate
	 * ugly visual artifacts, so choose a non-zero texture for them.
	 *
	 * Known affected retail levels (incomplete list):

Descent 2: Counterstrike
sha256:	f1abf516512739c97b43e2e93611a2398fc9f8bc7a014095ebc2b6b2fd21b703  descent2.hog
Levels 1-3: clean

Level #4
segment #170 side #4 has invalid tmap 910 (NumTextures=910)
segment #171 side #5 has invalid tmap 910 (NumTextures=910)
segment #184 side #2 has invalid tmap 910 (NumTextures=910)
segment #188 side #5 has invalid tmap 910 (NumTextures=910)

Level #5
segment #141 side #4 has invalid tmap 910 (NumTextures=910)

Level #6
segment #128 side #4 has invalid tmap 910 (NumTextures=910)

Level #7
segment #26 side #5 has invalid tmap 910 (NumTextures=910)
segment #28 side #5 has invalid tmap 910 (NumTextures=910)
segment #60 side #5 has invalid tmap 910 (NumTextures=910)
segment #63 side #5 has invalid tmap 910 (NumTextures=910)
segment #161 side #4 has invalid tmap 910 (NumTextures=910)
segment #305 side #4 has invalid tmap 910 (NumTextures=910)
segment #427 side #4 has invalid tmap 910 (NumTextures=910)
segment #533 side #5 has invalid tmap 910 (NumTextures=910)
segment #536 side #4 has invalid tmap 910 (NumTextures=910)
segment #647 side #4 has invalid tmap 910 (NumTextures=910)
segment #648 side #5 has invalid tmap 910 (NumTextures=910)

Level #8
segment #0 side #4 has invalid tmap 910 (NumTextures=910)
segment #92 side #0 has invalid tmap 910 (NumTextures=910)
segment #92 side #5 has invalid tmap 910 (NumTextures=910)
segment #94 side #1 has invalid tmap 910 (NumTextures=910)
segment #94 side #2 has invalid tmap 910 (NumTextures=910)
segment #95 side #0 has invalid tmap 910 (NumTextures=910)
segment #95 side #1 has invalid tmap 910 (NumTextures=910)
segment #97 side #5 has invalid tmap 910 (NumTextures=910)
segment #98 side #3 has invalid tmap 910 (NumTextures=910)
segment #100 side #1 has invalid tmap 910 (NumTextures=910)
segment #102 side #1 has invalid tmap 910 (NumTextures=910)
segment #104 side #3 has invalid tmap 910 (NumTextures=910)

Levels 9-end: unchecked

	 */
	const auto old_tmap_num = uside.tmap_num;
	if (const auto old_tmap_idx = get_texture_index(old_tmap_num); old_tmap_idx >= NumTextures)
		uside.tmap_num = build_texture1_value((
			LevelErrorV(PLAYING_BUILTIN_MISSION ? CON_VERBOSE : CON_URGENT, "segment #%hu side #%i has invalid tmap %u (NumTextures=%u).", sp.get_unchecked_index(), underlying_value(sidenum), old_tmap_idx, NumTextures),
			(sside.wall_num == wall_none)
		));

	//	Set render_flag.
	//	If side doesn't have a child, then render wall.  If it does have a child, but there is a temporary
	//	wall there, then do render wall.
//	if (sp->children[sidenum] == -1)
//		sp->sides[sidenum].render_flag = 1;
//	else if (sp->sides[sidenum].wall_num != -1)
//		sp->sides[sidenum].render_flag = 1;
//	else
//		sp->sides[sidenum].render_flag = 0;
}

// -------------------------------------------------------------------------------
//	Make a just-modified segment valid.
//		check all sides to see how many faces they each should have (0,1,2)
//		create new vector normals
void validate_segment(fvcvertptr &vcvertptr, const vmsegptridx_t sp)
{
	check_for_degenerate_segment(vcvertptr, sp);

	for (const auto side : MAX_SIDES_PER_SEGMENT)
		validate_segment_side(vcvertptr, sp, side);
}

#if !DXX_USE_EDITOR
}
#endif

// -------------------------------------------------------------------------------
//	Validate all segments.
//	Highest_segment_index must be set.
//	For all used segments (number <= Highest_segment_index), segnum field must be != -1.
void validate_segment_all(d_level_shared_segment_state &LevelSharedSegmentState)
{
	auto &Segments = LevelSharedSegmentState.get_segments();
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	range_for (const auto &&segp, Segments.vmptridx)
	{
#if DXX_USE_EDITOR
		if (segp->shared_segment::segnum != segment_none)
		#endif
			validate_segment(Vertices.vcptr, segp);
	}

#if DXX_USE_EDITOR
	for (shared_segment &s : partial_range(Segments, Segments.get_count(), Segments.size()))
		s.segnum = segment_none;
	#endif
}


//	------------------------------------------------------------------------------------------------------
//	Picks a random point in a segment like so:
//		From center, go up to 50% of way towards any of the 8 vertices.
void pick_random_point_in_seg(fvcvertptr &vcvertptr, vms_vector &new_pos, const shared_segment &sp, std::minstd_rand r)
{
	compute_segment_center(vcvertptr, new_pos, sp);
	const auto vnum = segment_relative_vertnum{std::uniform_int_distribution<uint8_t>(0, MAX_VERTICES_PER_SEGMENT - 1)(r)};
	auto &&vec2 = vm_vec_sub(vcvertptr(sp.verts[vnum]), new_pos);
	vm_vec_scale(vec2, std::uniform_int_distribution<fix>(0, f0_5 - 1)(r));          // always in 0..1/2 fix
	vm_vec_add2(new_pos, vec2);
}


//	----------------------------------------------------------------------------------------------------------
//	Set the segment depth of all segments from start_seg in *segbuf.
//	Returns maximum depth value.
unsigned set_segment_depths(vcsegidx_t start_seg, const std::array<uint8_t, MAX_SEGMENTS> *const limit, segment_depth_array_t &depth)
{
	std::array<segnum_t, MAX_SEGMENTS> queue;
	int	head, tail;

	head = 0;
	tail = 0;

	visited_segment_bitarray_t visited;

	queue[tail++] = start_seg;
	visited[start_seg] = true;
	depth[start_seg] = 1;

	unsigned parent_depth;
	do {
		const auto curseg = queue[head++];
		auto &children = vcsegptr(curseg)->shared_segment::children;
		parent_depth = depth[curseg];

		for (const auto childnum : children)
		{
			if (childnum != segment_none && childnum != segment_exit)
				if (!limit || (*limit)[childnum])
				{
					auto &&v = visited[childnum];
					if (!v)
					{
						v = true;
						depth[childnum] = min(static_cast<unsigned>(std::numeric_limits<segment_depth_array_t::value_type>::max()), parent_depth + 1);
						queue[tail++] = childnum;
					}
				}
		}
	} while (head < tail);

	return parent_depth+1;
}

#if defined(DXX_BUILD_DESCENT_II)
//these constants should match the ones in seguvs
#define	LIGHT_DISTANCE_THRESHOLD	(F1_0*80)
#define	Magical_light_constant  (F1_0*16)

namespace {

//	------------------------------------------------------------------------------------------
//cast static light from a segment to nearby segments
static void apply_light_to_segment(visited_segment_bitarray_t &visited, const vmsegptridx_t segp, const vms_vector &segment_center, const fix light_intensity, const unsigned recursion_depth)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	fix			dist_to_rseg;
	if (auto &&visited_ref = visited[segp])
	{
	}
	else
	{
		visited_ref = true;
		auto &vcvertptr = Vertices.vcptr;
		const auto r_segment_center = compute_segment_center(vcvertptr, segp);
		dist_to_rseg = vm_vec_dist_quick(r_segment_center, segment_center);
	
		if (dist_to_rseg <= LIGHT_DISTANCE_THRESHOLD) {
			fix	light_at_point;
			if (dist_to_rseg > F1_0)
				light_at_point = fixdiv(Magical_light_constant, dist_to_rseg);
			else
				light_at_point = Magical_light_constant;
	
			if (light_at_point >= 0) {
				light_at_point = fixmul(light_at_point, light_intensity);
#if 0   // don't see the point, static_light can be greater than F1_0
				if (light_at_point >= F1_0)
					light_at_point = F1_0-1;
				if (light_at_point <= -F1_0)
					light_at_point = -(F1_0-1);
#endif
				unique_segment &useg = segp;
				auto &static_light = useg.static_light;
				static_light += light_at_point;
				if (static_light < 0)	// if it went negative, saturate
					static_light = 0;
			}	//	end if (light_at_point...
		}	//	end if (dist_to_rseg...
	}

	if (recursion_depth < 2)
	{
		auto &Walls = LevelUniqueWallSubsystemState.Walls;
		auto &vcwallptr = Walls.vcptr;
		for (const auto sidenum : MAX_SIDES_PER_SEGMENT)
		{
			if (WALL_IS_DOORWAY(GameBitmaps, Textures, vcwallptr, segp, sidenum) & WALL_IS_DOORWAY_FLAG::rendpast)
				apply_light_to_segment(visited, segp.absolute_sibling(segp->children[sidenum]), segment_center, light_intensity, recursion_depth+1);
		}
	}
}


//update the static_light field in a segment, which is used for object lighting
//this code is copied from the editor routine calim_process_all_lights()
static void change_segment_light(const vmsegptridx_t segp, const sidenum_t sidenum, const unsigned dir)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &TmapInfo = LevelUniqueTmapInfoState.TmapInfo;
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcwallptr = Walls.vcptr;
	if (WALL_IS_DOORWAY(GameBitmaps, Textures, vcwallptr, segp, sidenum) & WALL_IS_DOORWAY_FLAG::render)
	{
		auto &sidep = segp->unique_segment::sides[sidenum];
		const auto light_intensity = TmapInfo[get_texture_index(sidep.tmap_num)].lighting + TmapInfo[get_texture_index(sidep.tmap_num2)].lighting;
		if (light_intensity) {
			auto &vcvertptr = Vertices.vcptr;
			const auto segment_center = compute_segment_center(vcvertptr, segp);
			visited_segment_bitarray_t visited;
			apply_light_to_segment(visited, segp, segment_center, light_intensity * dir, 0);
		}
	}

	//this is a horrible hack to get around the horrible hack used to
	//smooth lighting values when an object moves between segments
	old_viewer = NULL;

}

//	------------------------------------------------------------------------------------------
//	dir = +1 -> add light
//	dir = -1 -> subtract light
//	dir = 17 -> add 17x light
//	dir =  0 -> you are dumb
static void change_light(const d_level_shared_destructible_light_state &LevelSharedDestructibleLightState, const vmsegptridx_t segnum, const sidenum_t sidenum, const int dir)
{
	const fix ds = dir * DL_SCALE;
	auto &Dl_indices = LevelSharedDestructibleLightState.Dl_indices;
	const auto &&pr = cast_range_result<const dl_index &>(Dl_indices.vcptr);
	const auto &&er = std::equal_range(pr.begin(), pr.end(), dl_index{segnum, sidenum, 0, {}});
	auto &Delta_lights = LevelSharedDestructibleLightState.Delta_lights;
	range_for (auto &i, partial_range_t<const dl_index *>(er.first.base().base(), er.second.base().base()))
	{
		const std::size_t idx = underlying_value(i.index);
		for (auto &j : partial_const_range(Delta_lights, idx, idx + i.count))
			{
				assert(j.sidenum < MAX_SIDES_PER_SEGMENT);
				const auto &&segp = vmsegptr(j.segnum);
				auto &uvls = segp->unique_segment::sides[j.sidenum].uvls;
				for (const auto k : MAX_VERTICES_PER_SIDE)
				{
					auto &l = uvls[k].l;
					const fix dl = ds * j.vert_light[k];
					if ((l += dl) < 0)
						l = 0;
				}
			}
	}

	//recompute static light for segment
	change_segment_light(segnum,sidenum,dir);
}

}

//	Subtract light cast by a light source from all surfaces to which it applies light.
//	This is precomputed data, stored at static light application time in the editor (the slow lighting function).
// returns 1 if lights actually subtracted, else 0
int subtract_light(const d_level_shared_destructible_light_state &LevelSharedDestructibleLightState, const vmsegptridx_t segnum, const sidenum_t sidenum)
{
	unique_segment &useg = segnum;
	auto &light_subtracted = useg.light_subtracted;
	const auto mask = build_sidemask(sidenum);
	if (light_subtracted & mask)
		return 0;
	light_subtracted |= mask;
	change_light(LevelSharedDestructibleLightState, segnum, sidenum, -1);
	return 1;
}

//	Add light cast by a light source from all surfaces to which it applies light.
//	This is precomputed data, stored at static light application time in the editor (the slow lighting function).
//	You probably only want to call this after light has been subtracted.
// returns 1 if lights actually added, else 0
int add_light(const d_level_shared_destructible_light_state &LevelSharedDestructibleLightState, const vmsegptridx_t segnum, sidenum_t sidenum)
{
	const auto mask = build_sidemask(sidenum);
	unique_segment &useg = segnum;
	auto &light_subtracted = useg.light_subtracted;
	if (!(light_subtracted & mask))
		return 0;
	light_subtracted &= ~mask;
	change_light(LevelSharedDestructibleLightState, segnum, sidenum, 1);
	return 1;
}

//	Parse the Light_subtracted array, turning on or off all lights.
void apply_all_changed_light(const d_level_shared_destructible_light_state &LevelSharedDestructibleLightState, fvmsegptridx &vmsegptridx)
{
	range_for (const auto &&segp, vmsegptridx)
	{
		for (const auto j : MAX_SIDES_PER_SEGMENT)
		{
			unique_segment &useg = segp;
			if (useg.light_subtracted & build_sidemask(j))
				change_light(LevelSharedDestructibleLightState, segp, j, -1);
		}
	}
}

//	Should call this whenever a new mine gets loaded.
//	More specifically, should call this whenever something global happens
//	to change the status of static light in the mine.
void clear_light_subtracted(void)
{
	for (unique_segment &useg : vmsegptr)
		useg.light_subtracted = {};
}

#define	AMBIENT_SEGMENT_DEPTH		5

namespace {

static inline sound_ambient_flags &operator|=(sound_ambient_flags &l, const sound_ambient_flags r)
{
	return l = static_cast<sound_ambient_flags>(static_cast<uint8_t>(l) | static_cast<uint8_t>(r));
}

/* On entry, `segdepth_lava` or `segdepth_water` may be nullptr if the
 * corresponding type is not tracked.  `s2f_bit` tracks what flags the initial
 * caller is tallying, which may be both types even if one of the tracking
 * arrays was reset to nullptr during an inner iteration.
 */
static void ambient_mark_bfs(const vmsegptridx_t segp, segment_lava_depth_array *segdepth_lava, segment_water_depth_array *segdepth_water, const unsigned depth, const sound_ambient_flags s2f_bit)
{
	segp->s2_flags |= s2f_bit;
	/* For each of lava and water, check whether the segment was previously
	 * visited by an iteration of ambient_mark_bfs that had a higher (more
	 * permissive) depth count.
	 *
	 * If no, record the current depth count in the array.
	 * If yes, then do not visit it again now, because any work that would be
	 * done now would already have been done by the prior visit.  Track this by
	 * setting the corresponding array pointer to nullptr, which serves
	 * multiple purposes:
	 * - If one array pointer is null, no further tracking is done for that
	 *   type, because every subsequent iteration would also find that no
	 *   further recursion is needed for that type.
	 * - Once both array pointers are null, every child segment that this call
	 *   could visit would be skipped by both lava and water, so return early
	 *   without visiting them.
	 */
	if (segdepth_lava)
	{
		auto &d = (*segdepth_lava)[segp];
		if (d < depth)
			d = depth;
		else
			segdepth_lava = nullptr;
	}
	if (segdepth_water)
	{
		auto &d = (*segdepth_water)[segp];
		if (d < depth)
			d = depth;
		else
			segdepth_water = nullptr;
	}
	if (!segdepth_lava && !segdepth_water)
		return;

	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcwallptr = Walls.vcptr;
	for (const auto i : MAX_SIDES_PER_SEGMENT)
	{
		const auto child = segp->children[i];

		/*
		 * No explicit check for IS_CHILD.  If !IS_CHILD, then
		 * WALL_IS_DOORWAY never sets WALL_IS_DOORWAY_FLAG::rendpast.
		 */
		if (!(WALL_IS_DOORWAY(GameBitmaps, Textures, vcwallptr, segp, i) & WALL_IS_DOORWAY_FLAG::rendpast))
			continue;
		ambient_mark_bfs(segp.absolute_sibling(child), segdepth_lava, segdepth_water, depth - 1, s2f_bit);
	}
}

}

//	-----------------------------------------------------------------------------
//	Indicate all segments which are within audible range of falling water or lava,
//	and so should hear ambient gurgles.
void set_ambient_sound_flags()
{
	auto &TmapInfo = LevelUniqueTmapInfoState.TmapInfo;
	range_for (const auto &&segp, vmsegptr)
		segp->s2_flags = {};
	//	Now, all segments containing ambient lava or water sound makers are flagged.
	//	Additionally flag all segments which are within range of them.
	//	Mark all segments which are sources of the sound.
	segment_lava_depth_array segdepth_lava{};
	segment_water_depth_array segdepth_water{};

	range_for (const auto &&segp, vmsegptridx)
	{
		for (const auto j : MAX_SIDES_PER_SEGMENT)
		{
			const auto &sside = segp->shared_segment::sides[j];
			const auto &uside = segp->unique_segment::sides[j];
			if (IS_CHILD(segp->children[j]) && sside.wall_num == wall_none)
				/* If this side is open and there is no wall defined,
				 * then the texture is never visible to the player.
				 * This happens normally in some level editors if the
				 * texture is not cleared when the child segment is
				 * added.  Skip this side.
				 */
				continue;
			/* No other call site needs to combine two sets of
			 * tmapinfo_flags, so there is no overloaded operator to
			 * handle this.  Use the casts to allow it here.
			 */
			const auto texture_flags = static_cast<tmapinfo_flags>(underlying_value(TmapInfo[get_texture_index(uside.tmap_num)].flags) | underlying_value(TmapInfo[get_texture_index(uside.tmap_num2)].flags));
			/* These variables do not need to be named, but naming them
			 * is the easiest way to establish sequence points, so that
			 * `sound_flag` is passed to `ambient_mark_bfs` only after
			 * both ternary expressions have finished.
			 */
			sound_ambient_flags sound_flag{};
			const auto pl = (texture_flags & tmapinfo_flag::lava) ? (sound_flag |= sound_ambient_flags::lava, &segdepth_lava) : nullptr;
			const auto pw = (texture_flags & tmapinfo_flag::water) ? (sound_flag |= sound_ambient_flags::water, &segdepth_water) : nullptr;
			if (sound_flag != sound_ambient_flags::None)
				ambient_mark_bfs(segp, pl, pw, AMBIENT_SEGMENT_DEPTH, sound_flag);
		}
	}
}
#endif
}
