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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Interrogation functions for segment data structure.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "gr.h"
#include "inferno.h"
#include "segment.h"
#include "editor.h"
#include "editor/esegment.h"
#include "dxxerror.h"
#include "object.h"
#include "gameseg.h"
#include "render.h"
#include "wall.h"
#include "switch.h"
#include "fuelcen.h"
#include "seguvs.h"
#include "kdefs.h"

#include "medwall.h"

#include "compiler-poison.h"
#include "compiler-range_for.h"
#include "d_levelstate.h"
#include "d_range.h"
#include "d_enumerate.h"
#include "d_zip.h"
#include "partial_range.h"
#include "segiter.h"

int	Do_duplicate_vertex_check = 0;		// Gets set to 1 in med_create_duplicate_vertex, means to check for duplicate vertices in compress_mine

//	Remap all vertices in polygons in a segment through translation table xlate_verts.
int ToggleBottom(void)
{
	Render_only_bottom = !Render_only_bottom;
	Update_flags = UF_WORLD_CHANGED;
	return 0;
}

namespace dcx {

// -------------------------------------------------------------------------------
//	This function can be used to determine whether a vertex is used exactly once in
//	all segments, in which case it can be freely moved because it is not connected
//	to any other segment.
// -------------------------------------------------------------------------------
int is_free_vertex(const fvcsegptr &vcsegptr, const vertnum_t vi)
{
	unsigned count = 0;
	for (const shared_segment &s : vcsegptr)
	{
		auto sp = &s;
		if (sp->segnum != segment_none)
			range_for (auto &v, s.verts)
				if (v == vi)
				{
					if (++ count > 1)
						return 0;
				}
	}
	return count;
}

}

namespace {

// -------------------------------------------------------------------------------
//	Return true if one fixed point number is very close to another, else return false.
static int fnear(fix f1, fix f2)
{
	return (abs(f1 - f2) <= FIX_EPSILON);
}

// -------------------------------------------------------------------------------
static int vnear(const vms_vector &vp1, const vms_vector &vp2)
{
	return fnear(vp1.x, vp2.x) && fnear(vp1.y, vp2.y) && fnear(vp1.z, vp2.z);
}

static void maintain_vertex_count(valptridx<vertex>::array_managed_type &Vertices, const vertnum_t v)
{
	const unsigned u = static_cast<unsigned>(v) + 1;
	if (Vertices.get_count() < u)
		Vertices.set_count(u);
}

}

// -------------------------------------------------------------------------------
//	Add the vertex *vp to the global list of vertices, return its index.
//	Search until a matching vertex is found (has nearly the same coordinates) or until Num_vertices
// vertices have been looked at without a match.  If no match, add a new vertex.
vertnum_t med_add_vertex(const vertex &vp)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	int	count;					// number of used vertices found, for loops exits when count == Num_vertices

//	set_vertex_counts();

	const auto Num_vertices = LevelSharedVertexState.Num_vertices;
	Assert(Num_vertices < MAX_SEGMENT_VERTICES);

	count = 0;
	constexpr vertnum_t free_index_sentinel{UINT32_MAX};
	vertnum_t free_index = free_index_sentinel;
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &Vertex_active = LevelSharedVertexState.get_vertex_active();
	for (const unsigned vi : xrange(MAX_SEGMENT_VERTICES))
	{
		const vertnum_t v{vi};
		if (Vertex_active[v]) {
			if (vnear(vp, Vertices.vcptr(v))) {
				return v;
			}
			if (++ count >= Num_vertices)
				break;
		} else if (free_index == free_index_sentinel)
			/* No break here.  There might be a later vertex which
			 * matches the vnear test, and if there is, it should be
			 * used instead of adding a new vertex at the first free
			 * slot.
			 */
			free_index = v;					// we want free_index to be the first free slot to add a vertex
	}

	if (free_index == free_index_sentinel)
		free_index = vertnum_t{Num_vertices};

	while (Vertex_active.valid_index(free_index) && Vertex_active[free_index])
		free_index = static_cast<vertnum_t>(static_cast<unsigned>(free_index) + 1u);

	*Vertices.vmptr(free_index) = vp;
	Vertex_active[free_index] = 1;

	++LevelSharedVertexState.Num_vertices;

	maintain_vertex_count(Vertices, free_index);

	return free_index;
}

namespace dsx {

// ------------------------------------------------------------------------------------------
//	Returns the index of a free segment.
//	Scans the Segments array.
segnum_t get_free_segment_number(segment_array &Segments)
{
	for (const auto &&[segnum, seg] : enumerate(Segments))
		if (seg.segnum == segment_none)
		{
			++ LevelSharedSegmentState.Num_segments;
			if (segnum > Highest_segment_index)
				Segments.set_count(segnum + 1);
			return segnum;
		}

	Assert(0);
	return segnum_t{};
}

// -------------------------------------------------------------------------------
//	Create a new segment, duplicating exactly, including vertex ids and children, the passed segment.
segnum_t med_create_duplicate_segment(segment_array &Segments, const segment &sp)
{
	const auto segnum = get_free_segment_number(Segments);

	auto &nsp = *Segments.vmptr(segnum);
	nsp = sp;
	unique_segment &unsp = nsp;
	unsp.objects = object_none;

	return segnum;
}

}

// -------------------------------------------------------------------------------
//	Add the vertex *vp to the global list of vertices, return its index.
//	This is the same as med_add_vertex, except that it does not search for the presence of the vertex.
vertnum_t med_create_duplicate_vertex(const vertex &vp)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	const auto Num_vertices = LevelSharedVertexState.Num_vertices;
	Assert(Num_vertices < MAX_SEGMENT_VERTICES);

	Do_duplicate_vertex_check = 1;

	vertnum_t free_index = vertnum_t{Num_vertices};

	auto &Vertex_active = LevelSharedVertexState.get_vertex_active();
	while (Vertex_active.valid_index(free_index) && Vertex_active[free_index])
		free_index = static_cast<vertnum_t>(static_cast<unsigned>(free_index) + 1u);

	auto &Vertices = LevelSharedVertexState.get_vertices();
	*Vertices.vmptr(free_index) = vp;
	Vertex_active[free_index] = 1;

	++LevelSharedVertexState.Num_vertices;

	maintain_vertex_count(Vertices, free_index);

	return vertnum_t{free_index};
}

namespace {

// -------------------------------------------------------------------------------
//	Set the vertex *vp at index vnum in the global list of vertices, return its index (just for compatibility).
vertnum_t med_set_vertex(d_level_shared_vertex_state &LevelSharedVertexState, const vertnum_t vnum, const vertex &vp)
{
	auto &Vertices = LevelSharedVertexState.get_vertices();
	*Vertices.vmptr(vnum) = vp;

	// Just in case this vertex wasn't active, mark it as active.
	auto &Vertex_active = LevelSharedVertexState.get_vertex_active();
	if (auto &va = Vertex_active[vnum]; !va) {
		va = 1;
	}

	return vnum;
}

}

namespace dsx {

// -------------------------------------------------------------------------------
void create_removable_wall(fvcvertptr &vcvertptr, const vmsegptridx_t sp, const sidenum_t sidenum, const texture1_value tmap_num)
{
	create_walls_on_side(vcvertptr, sp, sidenum);

	sp->unique_segment::sides[sidenum].tmap_num = tmap_num;

	assign_default_uvs_to_side(sp, sidenum);
	assign_light_to_side(sp, sidenum);
}

#if 0

// ---------------------------------------------------------------------------------------------
//	Orthogonalize matrix smat, returning result in rmat.
//	Does not modify smat.
//	Uses Gram-Schmidt process.
//	See page 172 of Strang, Gilbert, Linear Algebra and its Applications
//	Matt -- This routine should be moved to the vector matrix library.
//	It IS legal for smat == rmat.
//	We should also have the functions:
//		mat_a = mat_b * scalar;				// we now have mat_a = mat_a * scalar;
//		mat_a = mat_b + mat_c * scalar;	// or maybe not, maybe this is not primitive
void make_orthogonal(vms_matrix *rmat,vms_matrix *smat)
{
	vms_matrix		tmat;
	vms_vector		tvec1,tvec2;
	float				dot;

	// Copy source matrix to work area.
	tmat = *smat;

	// Normalize the three rows of the matrix tmat.
	vm_vec_normalize(&tmat.xrow);
	vm_vec_normalize(&tmat.yrow);
	vm_vec_normalize(&tmat.zrow);

	//	Now, compute the first vector.
	// This is very easy -- just copy the (normalized) source vector.
	rmat->zrow = tmat.zrow;

	// Now, compute the second vector.
	// From page 172 of Strang, we use the equation:
	//		b' = b - [transpose(q1) * b] * q1
	//	where:	b  = the second row of tmat
	//				q1 = the first row of rmat
	//				b' = the second row of rmat

	// Compute: transpose(q1) * b
	dot = vm_vec_dot(&rmat->zrow,&tmat.yrow);

	// Compute: b - dot * q1
	rmat->yrow.x = tmat.yrow.x - fixmul(dot,rmat->zrow.x);
	rmat->yrow.y = tmat.yrow.y - fixmul(dot,rmat->zrow.y);
	rmat->yrow.z = tmat.yrow.z - fixmul(dot,rmat->zrow.z);

	// Now, compute the third vector.
	// From page 173 of Strang, we use the equation:
	//		c' = c - (q1*c)*q1 - (q2*c)*q2
	//	where:	c  = the third row of tmat
	//				q1 = the first row of rmat
	//				q2 = the second row of rmat
	//				c' = the third row of rmat

	// Compute: q1*c
	dot = vm_vec_dot(&rmat->zrow,&tmat.xrow);

	tvec1.x = fixmul(dot,rmat->zrow.x);
	tvec1.y = fixmul(dot,rmat->zrow.y);
	tvec1.z = fixmul(dot,rmat->zrow.z);

	// Compute: q2*c
	dot = vm_vec_dot(&rmat->yrow,&tmat.xrow);

	tvec2.x = fixmul(dot,rmat->yrow.x);
	tvec2.y = fixmul(dot,rmat->yrow.y);
	tvec2.z = fixmul(dot,rmat->yrow.z);

	vm_vec_sub(&rmat->xrow,vm_vec_sub(&rmat->xrow,&tmat.xrow,&tvec1),&tvec2);
}

#endif

// ------------------------------------------------------------------------------------------
// Given a segment, extract the rotation matrix which defines it.
// Do this by extracting the forward, right, up vectors and then making them orthogonal.
// In the process of making the vectors orthogonal, favor them in the order forward, up, right.
// This means that the forward vector will remain unchanged.
void med_extract_matrix_from_segment(const shared_segment &sp, vms_matrix &rotmat)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	vms_vector	forwardvec,upvec;

	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vcvertptr = Vertices.vcptr;
	extract_forward_vector_from_segment(vcvertptr, sp, forwardvec);
	extract_up_vector_from_segment(vcvertptr, sp, upvec);

	if (((forwardvec.x == 0) && (forwardvec.y == 0) && (forwardvec.z == 0)) || ((upvec.x == 0) && (upvec.y == 0) && (upvec.z == 0))) {
		rotmat = vmd_identity_matrix;
		return;
	}


	vm_vector_2_matrix(rotmat, forwardvec, &upvec, nullptr);
}

}

// ------------------------------------------------------------------------------------------
//	Given a rotation matrix *rotmat which describes the orientation of a segment
//	and a side destside, return the rotation matrix which describes the orientation for the side.
void update_matrix_based_on_side(vms_matrix &rotmat, const sidenum_t destside)
{
	switch (destside) {
		case sidenum_t::WLEFT:
			rotmat = vm_matrix_x_matrix(rotmat, vm_angles_2_matrix(
					vms_angvec{
						.p = 0,
						.b = 0,
						.h = -16384
					}));
			break;

		case sidenum_t::WTOP:
			rotmat = vm_matrix_x_matrix(rotmat, vm_angles_2_matrix(
					vms_angvec{
						.p = -16384,
						.b = 0,
						.h = 0
					}));
			break;

		case sidenum_t::WRIGHT:
			rotmat = vm_matrix_x_matrix(rotmat, vm_angles_2_matrix(
					vms_angvec{
						.p = 0,
						.b = 0,
						.h = 16384
					}));
			break;

		case sidenum_t::WBOTTOM:
			rotmat = vm_matrix_x_matrix(rotmat, vm_angles_2_matrix(
					vms_angvec{
						.p = 16384,
						.b = -32768,        // bank was -32768, but I think that was an erroneous compensation
						.h = 0
					}));
			break;

		case sidenum_t::WFRONT:
			rotmat = vm_matrix_x_matrix(rotmat, vm_angles_2_matrix(
					vms_angvec{
						.p = 0,
						.b = 0,
						.h = -32768
					}));
			break;

		case sidenum_t::WBACK:
			break;
	}
}

namespace {

//	-------------------------------------------------------------------------------------
static void change_vertex_occurrences(fvmsegptr &vmsegptr, const vertnum_t dest, const vertnum_t src)
{
	// Fix vertices in groups
	range_for (auto &g, partial_range(GroupList, num_groups))
		g.vertices.replace(src, dest);

	// now scan all segments, changing occurrences of src to dest
	for (shared_segment &segp : vmsegptr)
		if (segp.segnum != segment_none)
		{
			auto &verts = segp.verts;
			std::replace(verts.begin(), verts.end(), src, dest);
		}
}

// --------------------------------------------------------------------------------------------------
static void compress_vertices(void)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	const auto Num_vertices = LevelSharedVertexState.Num_vertices;
	auto &Vertices = LevelSharedVertexState.get_vertices();
	if (Vertices.get_count() == Num_vertices)
		return;

	unsigned vert = Vertices.get_count() - 1;	//MAX_SEGMENT_VERTICES-1;

	auto &vmvertptr = Vertices.vmptr;
	auto &Vertex_active = LevelSharedVertexState.get_vertex_active();
	for (unsigned hole = 0; hole < vert; ++hole)
	{
		const vertnum_t vhole{hole};
		if (auto &active_hole = Vertex_active[vhole]; !active_hole)
		{
			// found an unused vertex which is a hole if a used vertex follows (not necessarily immediately) it.
			for (; hole < vert; --vert)
			{
				const vertnum_t vvert{vert};
				auto &active_vert = Vertex_active[vvert];
				if (!active_vert)
					continue;
				active_hole = std::exchange(active_vert, 0);
				// Ok, hole is the index of a hole, vert is the index of a vertex which follows it.
				// Copy vert into hole, update pointers to it.
				auto &vp_vert = *vmvertptr(vvert);
				*vmvertptr(vhole) = vp_vert;
				vp_vert = {};
				DXX_MAKE_VAR_UNDEFINED(vp_vert);
				change_vertex_occurrences(vmsegptr, vhole, vvert);
				vert--;
				break;
			}
		}
	}

	Vertices.set_count(Num_vertices);
}

// --------------------------------------------------------------------------------------------------
static void compress_segments(void)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	if (Highest_segment_index == LevelSharedSegmentState.Num_segments - 1)
		return;

	auto &RobotCenters = LevelSharedRobotcenterState.RobotCenters;
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vmwallptr = Walls.vmptr;
	auto holep = Segments.vmptridx.begin();
	auto segp = Segments.vmptridx.end();
	for (; holep != segp; ++holep)
	{
		const vmsegptridx_t hole = *holep;
		const msmusegment suhole = hole;
		if (suhole.s.segnum != segment_none)
			continue;
			// found an unused segment which is a hole if a used segment follows (not necessarily immediately) it.
		for (; holep != --segp && segp.base()->segnum == segment_none;)
		{
		}
		if (holep == segp)
			break;
				// Ok, hole is the index of a hole, seg is the index of a segment which follows it.
				// Copy seg into hole, update pointers to it, update Cursegp, Markedsegp if necessary.
		const vmsegptridx_t seg = *segp;
		const msmusegment suseg = seg;
		suhole.s = std::move(suseg.s);
		suhole.u = std::move(suseg.u);
		/* As a debugging measure, reset the expired segment to default values.
		 * Nothing should access this segment until it is reallocated and
		 * reinitialized, so any values should be safe to write here.
		 */
		suseg.u = {};
		suseg.s = {};
		suseg.s.segnum = segment_none;
		if (Cursegp == seg)
			Cursegp = hole;
		if (Markedsegp == seg)
			Markedsegp = hole;
				// Fix segments in groups
		for (auto &g : partial_range(GroupList, num_groups))
			g.segments.replace(seg, hole);

				// Fix walls
		for (wall &w : vmwallptr)
			if (auto &s = w.segnum; s == seg)
				s = hole;

				// Fix fuelcenters, robotcens, and triggers... added 2/1/95 -Yuan
		for (auto &f : partial_range(LevelUniqueFuelcenterState.Station, LevelUniqueFuelcenterState.Num_fuelcenters))
			if (auto &s = f.segnum; s == seg)
				s = hole;

		for (auto &r : partial_range(RobotCenters, LevelSharedRobotcenterState.Num_robot_centers))
			if (auto &s = r.segnum; s == seg)
				s = hole;

		auto &Triggers = LevelUniqueWallSubsystemState.Triggers;
		auto &vmtrgptr = Triggers.vmptr;
		for (trigger &t : vmtrgptr)
		{
			for (auto &l : partial_range(t.seg, t.num_links))
				if (l == seg)
					l = hole;
		}

		for (auto &s : suhole.s.children)
		{
			if (IS_CHILD(s))
			{
				// Find out on what side the segment connection to the former seg is on in *csegp.
				shared_segment &ss = vmsegptr(s);
				for (auto &t : ss.children)
				{
					if (t == seg) {
						t = hole;					// It used to be connected to seg, so make it connected to hole
						break;
					}
				}	// end for t
			}	// end if
		}	// end for s

				//Update object segment pointers
		for (object_base &objp : objects_in(suhole.u, vmobjptridx, vmsegptr))
		{
			assert(objp.segnum == seg);
			objp.segnum = hole;
		}
	}

	Segments.set_count(LevelSharedSegmentState.Num_segments);
	med_create_new_segment_from_cursegp();

}

}

// -------------------------------------------------------------------------------
//	Combine duplicate vertices.
//	If two vertices have the same coordinates, within some small tolerance, then assign
//	the same vertex number to the two vertices, freeing up one of the vertices.
void med_combine_duplicate_vertices(enumerated_array<uint8_t, MAX_VERTICES, vertnum_t> &vlp)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vcvertptridx = Vertices.vcptridx;
	const std::ranges::subrange range{vcvertptridx};
	// Note: ok to do to <, rather than <= because w for loop starts at v+1
	if (range.empty())
		return;
	for (auto i = range.begin();;)
	{
		const auto &&v = *i;
		if (++i == range.end())
			return;
		if (vlp[v]) {
			auto &vvp = *v;
			for (auto &&w : ranges::subrange(i, range.end()))
				if (vlp[w]) {	//	used to be Vertex_active[w]
					if (vnear(vvp, *w)) {
						change_vertex_occurrences(vmsegptr, v, w);
					}
				}
		}
	}
}

// ------------------------------------------------------------------------------
//	Compress mine at Segments and Vertices by squeezing out all holes.
//	If no holes (ie, an unused segment followed by a used segment), then no action.
//	If Cursegp or Markedsegp is a segment which gets moved to fill in a hole, then
//	they are properly updated.
void med_compress_mine(void)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	if (Do_duplicate_vertex_check) {
		auto &Vertex_active = LevelSharedVertexState.get_vertex_active();
		med_combine_duplicate_vertices(Vertex_active);
		Do_duplicate_vertex_check = 0;
	}

	compress_segments();
	compress_vertices();
	set_vertex_counts();

	//--repair-- create_local_segment_data();

	//	This is necessary becuase a segment search (due to click in 3d window) uses the previous frame's
	//	segment information, which could get changed by this.
	Update_flags = UF_WORLD_CHANGED;
}

namespace dsx {

namespace {

// ------------------------------------------------------------------------------------------
//	Copy texture map ids for each face in sseg to dseg.
static void copy_tmap_ids(unique_segment &dseg, const unique_segment &sseg)
{
	for (auto &&[ss, ds] : zip(sseg.sides, dseg.sides))
	{
		ds.tmap_num = ss.tmap_num;
		ds.tmap_num2 = texture2_value::None;
	}
}

// ------------------------------------------------------------------------------------------
//	Attach a segment with a rotated orientation.
// Return value:
//  0 = successful attach
//  1 = No room in Segments[].
//  2 = No room in Vertices[].
//  3 = newside != WFRONT -- for now, the new segment must be attached at its (own) front side
//	 4 = already a face attached on destseg:destside
static int med_attach_segment_rotated(const vmsegptridx_t destseg, const csmusegment newseg, const sidenum_t destside, const sidenum_t newside, const vms_matrix &attmat)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	vms_matrix	rotmat,rotmat2,rotmat3;
	vms_vector	forvec,upvec;

	// Return if already a face attached on this side.
	if (IS_CHILD(destseg->children[destside]))
		return 4;

	const auto segnum = get_free_segment_number(Segments);

	forvec = attmat.fvec;
	upvec = attmat.uvec;

	//	We are pretty confident we can add the segment.
	const auto &&nsp = destseg.absolute_sibling(segnum);

	nsp->segnum = segnum;
	static_cast<unique_segment &>(nsp).objects = object_none;
	nsp->matcen_num = materialization_center_number::None;

	// Copy group value.
	nsp->group = destseg->group;

	// Add segment to proper group list.
	if (nsp->group > -1)
		add_segment_to_group(nsp, nsp->group);

	// Copy the texture map ids.
	copy_tmap_ids(nsp,newseg);

	// clear all connections
	for (const auto side : MAX_SIDES_PER_SEGMENT)
	{
		nsp->children[side] = segment_none;
		nsp->shared_segment::sides[side].wall_num = wall_none;	
	}

	// Form the connection
	destseg->children[destside] = segnum;
//	destseg->sides[destside].render_flag = 0;
	nsp->children[newside] = destseg;

	// Copy vertex indices of the four vertices forming the joint
	auto &dvp = Side_to_verts[destside];

	// Set the vertex indices for the four vertices forming the front of the new side
	for (auto &&[dst, src] : zip(unchecked_partial_range(nsp->verts, 4u), dvp))
		dst = destseg->verts[src];

	// The other 4 vertices must be created.
	// Their coordinates are determined by the 4 welded vertices and the vector from front
	// to back of the original *newseg.

	// Do lots of hideous matrix stuff, about 3/4 of which could probably be simplified out.
	med_extract_matrix_from_segment(destseg, rotmat);		// get orientation matrix for destseg (orthogonal rotation matrix)
	update_matrix_based_on_side(rotmat,destside);
	const auto rotmat1 = vm_vector_2_matrix(forvec,&upvec,nullptr);
	const auto rotmat4 = vm_matrix_x_matrix(rotmat,rotmat1);			// this is the desired orientation of the new segment
	med_extract_matrix_from_segment(newseg, rotmat3);		// this is the current orientation of the new segment
	vm_transpose_matrix(rotmat3);								// get the inverse of the current orientation matrix
	vm_matrix_x_matrix(rotmat2,rotmat4,rotmat3);			// now rotmat2 takes the current segment to the desired orientation

	// Warning -- look at this line!
	vm_transpose_matrix(rotmat2);	// added 12:33 pm, 10/01/93

	// Compute and rotate the center point of the attaching face.
	auto &vcvertptr = Vertices.vcptr;
	const auto &&vc0 = compute_center_point_on_side(vcvertptr, newseg, newside);
	const auto vr = vm_vec_rotate(vc0,rotmat2);

	// Now translate the new segment so that the center point of the attaching faces are the same.
	const auto &&vc1 = compute_center_point_on_side(vcvertptr, destseg, destside);
	const auto xlate_vec = vm_vec_sub(vc1,vr);

	// Now rotate the free vertices in the segment
	// Create and add the 4 new vertices.
	for (auto &&[dst, src] : zip(unchecked_partial_range(nsp->verts, 4u, MAX_VERTICES_PER_SEGMENT.value), newseg.s.verts))
	{
		vertex tv;
		vm_vec_rotate(tv, vcvertptr(src), rotmat2);
		vm_vec_add2(tv, xlate_vec);
		dst = med_add_vertex(tv);
	}

	set_vertex_counts();

	// Now all the vertices are in place.  Create the faces.
	validate_segment(vcvertptr, nsp);

	//	Say to not render at the joint.
//	destseg->sides[destside].render_flag = 0;
//	nsp->sides[newside].render_flag = 0;

	Cursegp = nsp;

	return	0;
}

}

// ------------------------------------------------------------------------------------------
// Attach side newside of newseg to side destside of destseg.
// Copies *newseg into global array Segments, increments Num_segments.
// Forms a weld between the two segments by making the new segment fit to the old segment.
// Updates number of faces per side if necessitated by new vertex coordinates.
//	Updates Cursegp.
// Return value:
//  0 = successful attach
//  1 = No room in Segments[].
//  2 = No room in Vertices[].
//  3 = newside != WFRONT -- for now, the new segment must be attached at its (own) front side
//	 4 = already a face attached on side newside
int med_attach_segment(const vmsegptridx_t destseg, const csmusegment newseg, const sidenum_t destside, const sidenum_t newside)
{
	int		rval;
	const auto ocursegp = Cursegp;

	vms_angvec	tang = {0,0,0};
	const auto &&rotmat = vm_angles_2_matrix(tang);
	rval = med_attach_segment_rotated(destseg,newseg,destside,newside,rotmat);
	med_propagate_tmaps_to_segments(ocursegp,Cursegp,0);
	med_propagate_tmaps_to_back_side(Cursegp, Side_opposite[newside],0);
	copy_uvs_seg_to_seg(vmsegptr(&New_segment), Cursegp);

	return rval;
}

}

namespace {

// -------------------------------------------------------------------------------
//	Delete a vertex, sort of.
//	Decrement the vertex count.  If the count goes to 0, then the vertex is free (has been deleted).
static void delete_vertex(const vertnum_t v)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertex_active = LevelSharedVertexState.get_vertex_active();
	auto &va = Vertex_active[v];
	assert(va >= 1);	// abort if trying to delete a non-existent vertex
	--va;
}

// -------------------------------------------------------------------------------
//	Update Num_vertices.
//	This routine should be called by anyone who calls delete_vertex.  It could be called in delete_vertex,
//	but then it would be called much more often than necessary, and it is a slow routine.
static void update_num_vertices(void)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	// Now count the number of vertices.
	unsigned n = 0;
	auto &Vertex_active = LevelSharedVertexState.get_vertex_active();
	range_for (const auto v, partial_range(Vertex_active, Vertices.get_count()))
		if (v)
			++n;
	LevelSharedVertexState.Num_vertices = n;
}

}

namespace dsx {

// -------------------------------------------------------------------------------
//	Set Vertex_active to number of occurrences of each vertex.
//	Set Num_vertices.
void set_vertex_counts(void)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertex_active = LevelSharedVertexState.get_vertex_active();
	unsigned Num_vertices = 0;

	Vertex_active = {};

	// Count number of occurrences of each vertex.
	range_for (const auto &&segp, vmsegptr)
	{
		if (segp->segnum != segment_none)
			range_for (auto &v, segp->verts)
			{
				if (!Vertex_active[v])
					Num_vertices++;
				++ Vertex_active[v];
			}
	}
	LevelSharedVertexState.Num_vertices = Num_vertices;
}

namespace {

// -------------------------------------------------------------------------------
//	Delete all vertices in segment *sp from the vertex list if they are not contained in another segment.
//	This is kind of a dangerous routine.  It modifies the global array Vertex_active, using the field as
//	a count.
static void delete_vertices_in_segment(const shared_segment &sp)
{
//	init_vertices();
	set_vertex_counts();
	// Subtract one count for each appearance of vertex in deleted segment
	range_for (auto &v, sp.verts)
		delete_vertex(v);

	update_num_vertices();
}

}

// -------------------------------------------------------------------------------
//	Delete segment *sp in Segments array.
// Return value:
//		0	successfully deleted.
//		1	unable to delete.
int med_delete_segment(const vmsegptridx_t sp)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vmobjptr = Objects.vmptr;
	auto &vmobjptridx = Objects.vmptridx;
	segnum_t segnum = sp;
	// Cannot delete segment if only segment.
	if (LevelSharedSegmentState.Num_segments == 1)
		return 1;

	// Don't try to delete if segment doesn't exist.
	if (sp->segnum == segment_none) {
		return 1;
	}

	// Delete its refueling center if it has one
	fuelcen_delete(sp);

	delete_vertices_in_segment(sp);

	-- LevelSharedSegmentState.Num_segments;

	// If deleted segment has walls on any side, wipe out the wall.
	for (const auto side : MAX_SIDES_PER_SEGMENT)
		if (sp->shared_segment::sides[side].wall_num != wall_none) 
			wall_remove_side(sp, side);

	auto &vcvertptr = Vertices.vcptr;
	// Find out what this segment was connected to and break those connections at the other end.
	range_for (auto &side, sp->children)
		if (IS_CHILD(side)) {
			const auto &&csp = sp.absolute_sibling(side);
			for (const auto s : MAX_SIDES_PER_SEGMENT)
				if (csp->children[s] == segnum) {
					csp->children[s] = segment_none;				// this is the side of connection, break it
					validate_segment_side(vcvertptr, csp, s);					// we have converted a connection to a side so validate the segment
					med_propagate_tmaps_to_back_side(csp,s,0);
				}
			Cursegp = csp;
			med_create_new_segment_from_cursegp();
			copy_uvs_seg_to_seg(vmsegptr(&New_segment), Cursegp);
		}

	sp->segnum = segment_none;										// Mark segment as inactive.

	// If deleted segment = marked segment, then say there is no marked segment
	if (sp == Markedsegp)
		Markedsegp = segment_none;
	
	//	If deleted segment = a Group segment ptr, then wipe it out.
	range_for (auto &s, partial_range(Groupsegp, num_groups))
		if (s == sp)
			s = nullptr;

	// If deleted segment = group segment, wipe it off the group list.
	if (sp->group > -1) 
			delete_segment_from_group(sp, sp->group);

	// If we deleted something which was not connected to anything, must now select a new current segment.
	if (Cursegp == sp)
		for (auto &&[s, seg] : enumerate(Segments))
			if (seg.segnum != segment_none && s != segnum)
			{
				Cursegp = imsegptridx(s);
				med_create_new_segment_from_cursegp();
		   	break;
			}

	// If deleted segment contains objects, wipe out all objects
		range_for (const auto objnum, objects_in(*sp, vmobjptridx, vmsegptr))
		{
			//if an object is in the seg, delete it
			//if the object is the player, move to new curseg
			if (objnum == ConsoleObject)	{
				compute_segment_center(vcvertptr, ConsoleObject->pos,Cursegp);
				obj_relink(vmobjptr, vmsegptr, objnum, Cursegp);
			} else
				obj_delete(LevelUniqueObjectState, Segments, objnum);
		}

	// If we are leaving many holes in Segments or Vertices, then compress mine, because it is inefficient to be that way
//	if ((Highest_segment_index > Num_segments+4) || (Highest_vertex_index > Num_vertices+4*8))
//		med_compress_mine();

	return 0;
}

namespace {

// ------------------------------------------------------------------------------------------
//	Copy texture maps from sseg to dseg
static void copy_tmaps_to_segment(segment &dstseg, const segment &srcseg)
{
	shared_segment &shared_dst_seg = dstseg;
	unique_segment &unique_dst_seg = dstseg;
	const shared_segment &shared_src_seg = srcseg;
	const unique_segment &unique_src_seg = srcseg;
	range_for (const auto &&z, zip(shared_src_seg.sides, shared_dst_seg.sides, unique_src_seg.sides, unique_dst_seg.sides))
	{
		auto &shared_src_side = std::get<0>(z);
		auto &shared_dst_side = std::get<1>(z);
		auto &unique_src_side = std::get<2>(z);
		auto &unique_dst_side = std::get<3>(z);
		shared_dst_side.set_type(shared_src_side.get_type());
		unique_dst_side.tmap_num = unique_src_side.tmap_num;
		unique_dst_side.tmap_num2 = unique_src_side.tmap_num2;
	}

}

}

// ------------------------------------------------------------------------------------------
// Rotate the segment *seg by the pitch, bank, heading defined by *rot, destructively
// modifying its four free vertices in the global array Vertices.
// It is illegal to rotate a segment which has connectivity != 1.
// Pitch, bank, heading are about the point which is the average of the four points
// forming the side of connection.
// Return value:
//  0 = successful rotation
//  1 = Connectivity makes rotation illegal (connected to 0 or 2+ segments)
//  2 = Rotation causes degeneracy, such as self-intersecting segment.
//	 3 = Unable to rotate because not connected to exactly 1 segment.
int med_rotate_segment(const vmsegptridx_t seg, const vms_matrix &rotmat)
{
	std::optional<sidenum_t> newside;

	// Find side of attachment
	for (const auto &&[idx, value] : enumerate(seg->children))
		if (IS_CHILD(value))
		{
	// Return if passed in segment is connected to other than 1 segment.
			if (newside)
				return 3;
			newside = idx;
		}

	const auto &&destseg = seg.absolute_sibling(seg->children[*newside]);

	if (Curside == sidenum_t::WFRONT)
		Curside = sidenum_t::WBACK;

	// Before deleting the segment, copy its texture maps to New_segment
	copy_tmaps_to_segment(vmsegptr(&New_segment), seg);
	for (const auto &&[destside, child] : enumerate(destseg->children))
		if (child == seg)
		{
			med_attach_segment_rotated(destseg, vmsegptr(&New_segment), static_cast<sidenum_t>(destside), AttachSide, rotmat);
			break;
		}

	//	Save tmap_num on each side to restore after call to med_propagate_tmaps_to_segments and _back_side
	//	which will change the tmap nums.
	per_side_array<texture1_value> side_tmaps;
	for (auto &&[tm, us] : zip(side_tmaps, seg->unique_segment::sides))
		tm = us.tmap_num;

	auto back_side = Side_opposite[find_connect_side(destseg, seg)];

	med_propagate_tmaps_to_segments(destseg, seg,0);
	med_propagate_tmaps_to_back_side(seg, back_side,0);

	for (const auto &&[idx, side_tmap, us] : enumerate(zip(side_tmaps, seg->unique_segment::sides)))
		if (idx != back_side)
		{
			us.tmap_num = side_tmap;
		}

	return	0;
}

namespace {

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

// ----------------------------------------------------------------------------
//	Compute the sum of the distances between the four pairs of points.
//	The connections are:
//		firstv1 : 0		(firstv1+1)%4 : 1		(firstv1+2)%4 : 2		(firstv1+3)%4 : 3
static fix seg_seg_vertex_distsum(const shared_segment &seg1, const sidenum_t side1, const shared_segment &seg2, const sidenum_t side2, const unsigned firstv1)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	fix	distsum;

	distsum = 0;
	auto &vcvertptr = Vertices.vcptr;
	auto &sv1 = Side_to_verts[side1];
	auto &sv2 = Side_to_verts[side2];
	for (const auto secondv : MAX_VERTICES_PER_SIDE)
	{
		const auto firstv = static_cast<side_relative_vertnum>((4 - underlying_value(secondv) + (3 - firstv1)) % 4);
		auto &seg1vert = *vcvertptr(seg1.verts[sv1[firstv]]);
		auto &seg2vert = *vcvertptr(seg2.verts[sv2[secondv]]);
		distsum += vm_vec_dist(seg1vert, seg2vert);
	}

	return distsum;

}

// ----------------------------------------------------------------------------
//	Determine how to connect two segments together with the least amount of twisting.
//	Returns vertex index in 0..3 on first segment.  Assumed ordering of vertices
//	on second segment is 0,1,2,3.
//	So, if return value is 2, connect 2:0 3:1 0:2 1:3.
//	Theory:
//		We select an ordering of vertices for connection.  For the first pair of vertices to be connected,
//		compute the vector.  For the three remaining pairs of vertices, compute the vectors from one vertex
//		to the other.  Compute the dot products of these vectors with the original vector.  Add them up.
//		The close we are to 3, the better fit we have.  Reason:  The largest value for the dot product is
//		1.0, and this occurs for a parallel set of vectors.
static int get_index_of_best_fit(const shared_segment &seg1, const sidenum_t side1, const shared_segment &seg2, const sidenum_t side2)
{
	int	firstv;
	fix	min_distance;
	int	best_index=0;

	min_distance = F1_0*30000;

	for (firstv=0; firstv<4; firstv++) {
		fix t;
		t = seg_seg_vertex_distsum(seg1, side1, seg2, side2, firstv);
		if (t <= min_distance) {
			min_distance = t;
			best_index = firstv;
		}
	}

	return best_index;

}

}

#define MAX_VALIDATIONS 50

// ----------------------------------------------------------------------------
//	Modify seg2 to share side2 with seg1:side1.  This forms a connection between
//	two segments without creating a new segment.  It modifies seg2 by sharing
//	vertices from seg1.  seg1 is not modified.  Four vertices from seg2 are
//	deleted.
//	Return code:
//		0			joint formed
//		1			-- no, this is legal! -- unable to form joint because one or more vertices of side2 is not free
//		2			unable to form joint because side1 is already used
int med_form_joint(const vmsegptridx_t seg1, const sidenum_t side1, const vmsegptridx_t seg2, const sidenum_t side2)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	int		bfi,s1;
	std::array<segnum_t, MAX_VALIDATIONS> validation_list;
	uint_fast32_t nv;

	//	Make sure that neither side is connected.
	if (IS_CHILD(seg1->children[side1]) || IS_CHILD(seg2->children[side2]))
		return 2;

	// Make sure there is no wall there 
	if ((seg1->shared_segment::sides[side1].wall_num != wall_none) || (seg2->shared_segment::sides[side2].wall_num != wall_none))
		return 2;

	//	We can form the joint.  Find the best orientation of vertices.
	bfi = get_index_of_best_fit(seg1, side1, seg2, side2);

	auto &vp1 = Side_to_verts[side1];
	auto &vp2 = Side_to_verts[side2];

	//	Make a copy of the list of vertices in seg2 which will be deleted and set the
	//	associated vertex number, so that all occurrences of the vertices can be replaced.
	std::array<vertnum_t, 4> lost_vertices, remap_vertices;
	for (auto &&[lv, vi] : zip(lost_vertices, vp2))
		lv = seg2->verts[vi];

	//	Now, for each vertex in lost_vertices, determine which vertex it maps to.
	for (const auto v : MAX_VERTICES_PER_SIDE)
		remap_vertices[3 - ((underlying_value(v) + bfi) % 4)] = seg1->verts[vp1[v]];

	// Now, in all segments, replace all occurrences of vertices in lost_vertices with remap_vertices

	// Put the one segment we know are being modified into the validation list.
	// Note: seg1 does not require a full validation, only a validation of the affected side.  Its vertices do not move.
	nv = 1;
	validation_list[0] = seg2;

	for (auto &&[lv, rv] : zip(lost_vertices, remap_vertices))
		range_for (const auto &&segp, vmsegptridx)
		{
			if (segp->segnum != segment_none)
				range_for (auto &sv, segp->verts)
					if (sv == lv) {
						sv = rv;
						// Add segment to list of segments to be validated.
						for (s1=0; s1<nv; s1++)
							if (validation_list[s1] == segp)
								break;
						if (s1 == nv)
							validation_list[nv++] = segp;
						Assert(nv < MAX_VALIDATIONS);
					}
		}

	//	Form new connections.
	seg1->children[side1] = seg2;
	seg2->children[side2] = seg1;

	// validate all segments
	auto &vcvertptr = Vertices.vcptr;
	validate_segment_side(vcvertptr, seg1, side1);
	range_for (auto &s, partial_const_range(validation_list, nv))
	{
		const auto &&segp = seg1.absolute_sibling(s);
		validate_segment(vcvertptr, segp);
		warn_if_concave_segment(segp);
	}

	set_vertex_counts();

	return 0;
}

// ----------------------------------------------------------------------------
//	Create a new segment and use it to form a bridge between two existing segments.
//	Specify two segment:side pairs.  If either segment:side is not open (ie, segment->children[side] != -1)
//	then it is not legal to form the brider.
//	Return:
//		0	bridge segment formed
//		1	unable to form bridge because one (or both) of the sides is not open.
//	Note that no new vertices are created by this process.
int med_form_bridge_segment(const vmsegptridx_t seg1, const sidenum_t side1, const vmsegptridx_t seg2, const sidenum_t side2)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();

	if (IS_CHILD(seg1->children[side1]) || IS_CHILD(seg2->children[side2]))
		return 1;

	const auto &&bs = seg1.absolute_sibling(get_free_segment_number(Segments));
	shared_segment &sbs = *bs;
	unique_segment &ubs = *bs;
	sbs.segnum = bs;
	ubs.objects = object_none;

	// Copy vertices from seg2 into last 4 vertices of bridge segment.
	{
	auto &sv = Side_to_verts[side2];
		for (const auto v : MAX_VERTICES_PER_SIDE)
			sbs.verts[(segment_relative_vertnum{static_cast<uint8_t>((3 - underlying_value(v)) + 4)})] = seg2->verts[sv[v]];
	}

	// Copy vertices from seg1 into first 4 vertices of bridge segment.
	const auto bfi = get_index_of_best_fit(seg1, side1, seg2, side2);

	{
	auto &sv = Side_to_verts[side1];
		for (const auto v : MAX_VERTICES_PER_SIDE)
			bs->verts[(segment_relative_vertnum{static_cast<uint8_t>((underlying_value(v) + bfi) % 4)})] = seg1->verts[sv[v]];
	}

	// Form connections to children, first initialize all to unconnected.
	for (auto &&[child, side] : zip(sbs.children, sbs.sides))
	{
		child = segment_none;
		side.wall_num = wall_none;
	}

	// Now form connections between segments.

	bs->children[AttachSide] = seg1;
	bs->children[Side_opposite[AttachSide]] = seg2;

	seg1->children[side1] = bs; //seg2 - Segments;
	seg2->children[side2] = bs; //seg1 - Segments;

	//	Validate bridge segment, and if degenerate, clean up mess.
	Degenerate_segment_found = 0;

	auto &vcvertptr = Vertices.vcptr;
	validate_segment(vcvertptr, bs);

	if (Degenerate_segment_found) {
		seg1->children[side1] = segment_none;
		seg2->children[side2] = segment_none;
		bs->children[AttachSide] = segment_none;
		bs->children[Side_opposite[AttachSide]] = segment_none;
		if (med_delete_segment(bs)) {
			Int3();
		}
		editor_status("Bridge segment would be degenerate, not created.\n");
		return 1;
	} else {
		validate_segment(vcvertptr, seg1);	// used to only validate side, but segment does more error checking: ,side1);
		validate_segment(vcvertptr, seg2);	// ,side2);
		med_propagate_tmaps_to_segments(seg1,bs,0);

		editor_status("Bridge segment formed.");
		warn_if_concave_segment(bs);
		return 0;
	}
}

// -------------------------------------------------------------------------------
//	Create a segment given center, dimensions, rotation matrix.
//	Note that the created segment will always have planar sides and rectangular cross sections.
//	It will be created with walls on all sides, ie not connected to anything.
void med_create_segment(const vmsegptridx_t sp,fix cx, fix cy, fix cz, fix length, fix width, fix height, const vms_matrix &mp)
{
	unique_segment &usp = sp;
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	++ LevelSharedSegmentState.Num_segments;

	sp->segnum = segnum_t{1};						// What to put here?  I don't know.

	// Form connections to children, of which it has none.
	for (auto &&[child, side] : zip(sp->children, sp->shared_segment::sides))
	{
		child = segment_none;
		side.wall_num  = wall_none;
	}

	sp->group = -1;
	sp->matcen_num = materialization_center_number::None;

	//	Create relative-to-center vertices, which are the rotated points on the box defined by length, width, height
	sp->verts[segment_relative_vertnum::_0] = med_add_vertex(vertex{vm_vec_rotate({+width/2, +height/2, -length/2}, mp)});
	sp->verts[segment_relative_vertnum::_1] = med_add_vertex(vertex{vm_vec_rotate({+width/2, -height/2, -length/2}, mp)});
	sp->verts[segment_relative_vertnum::_2] = med_add_vertex(vertex{vm_vec_rotate({-width/2, -height/2, -length/2}, mp)});
	sp->verts[segment_relative_vertnum::_3] = med_add_vertex(vertex{vm_vec_rotate({-width/2, +height/2, -length/2}, mp)});
	sp->verts[segment_relative_vertnum::_4] = med_add_vertex(vertex{vm_vec_rotate({+width/2, +height/2, +length/2}, mp)});
	sp->verts[segment_relative_vertnum::_5] = med_add_vertex(vertex{vm_vec_rotate({+width/2, -height/2, +length/2}, mp)});
	sp->verts[segment_relative_vertnum::_6] = med_add_vertex(vertex{vm_vec_rotate({-width/2, -height/2, +length/2}, mp)});
	sp->verts[segment_relative_vertnum::_7] = med_add_vertex(vertex{vm_vec_rotate({-width/2, +height/2, +length/2}, mp)});

	// Now create the vector which is the center of the segment and add that to all vertices.
	const vms_vector cv{cx, cy, cz};

	//	Now, add the center to all vertices, placing the segment in 3 space.
	auto &vmvertptr = Vertices.vmptr;
	range_for (auto &i, sp->verts)
		vm_vec_add2(vmvertptr(i), cv);

	//	Set scale vector.
//	sp->scale.x = width;
//	sp->scale.y = height;
//	sp->scale.z = length;

	//	Add faces to all sides.
	auto &vcvertptr = Vertices.vcptr;
	for (const auto f : MAX_SIDES_PER_SEGMENT)
		create_walls_on_side(vcvertptr, sp, f);

	usp.objects = object_none;		//no objects in this segment

	// Assume nothing special about this segment
	sp->special = segment_special::nothing;
	sp->station_idx = station_number::None;
	usp.static_light = 0;
	sp->matcen_num = materialization_center_number::None;

	copy_tmaps_to_segment(sp, vcsegptr(&New_segment));

	assign_default_uvs_to_segment(sp);
}

// ----------------------------------------------------------------------------------------------
//	Create New_segment using a specified scale factor.
void med_create_new_segment(const vms_vector &scale)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	const auto &&sp = vmsegptridx(&New_segment);
	unique_segment &usp = sp;
	fix			length,width,height;

	length = scale.z;
	width = scale.x;
	height = scale.y;

	sp->segnum = segnum_t{1};						// What to put here?  I don't know.

	//	Create relative-to-center vertices, which are the points on the box defined by length, width, height
	auto &verts = sp->verts;
	verts[segment_relative_vertnum::_0] = med_set_vertex(LevelSharedVertexState, vertnum_t{NEW_SEGMENT_VERTICES + 0}, {+width / 2, +height / 2, -length / 2});
	verts[segment_relative_vertnum::_1] = med_set_vertex(LevelSharedVertexState, vertnum_t{NEW_SEGMENT_VERTICES + 1}, {+width / 2, -height / 2, -length / 2});
	verts[segment_relative_vertnum::_2] = med_set_vertex(LevelSharedVertexState, vertnum_t{NEW_SEGMENT_VERTICES + 2}, {-width / 2, -height / 2, -length / 2});
	verts[segment_relative_vertnum::_3] = med_set_vertex(LevelSharedVertexState, vertnum_t{NEW_SEGMENT_VERTICES + 3}, {-width / 2, +height / 2, -length / 2});
	verts[segment_relative_vertnum::_4] = med_set_vertex(LevelSharedVertexState, vertnum_t{NEW_SEGMENT_VERTICES + 4}, {+width / 2, +height / 2, +length / 2});
	verts[segment_relative_vertnum::_5] = med_set_vertex(LevelSharedVertexState, vertnum_t{NEW_SEGMENT_VERTICES + 5}, {+width / 2, -height / 2, +length / 2});
	verts[segment_relative_vertnum::_6] = med_set_vertex(LevelSharedVertexState, vertnum_t{NEW_SEGMENT_VERTICES + 6}, {-width / 2, -height / 2, +length / 2});
	verts[segment_relative_vertnum::_7] = med_set_vertex(LevelSharedVertexState, vertnum_t{NEW_SEGMENT_VERTICES + 7}, {-width / 2, +height / 2, +length / 2});

//	sp->scale = *scale;

	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vcvertptr = Vertices.vcptr;
	// Form connections to children, of which it has none, init faces and tmaps.
	for (const auto &&[s, child, ss, us] : enumerate(zip(sp->children, sp->shared_segment::sides, sp->unique_segment::sides)))
	{
		child = segment_none;
		ss.wall_num = wall_none;
		create_walls_on_side(vcvertptr, sp, static_cast<sidenum_t>(s));
		us.tmap_num = build_texture1_value(1);					// assign some stupid old tmap to this side.
		us.tmap_num2 = texture2_value::None;
	}

	Seg_orientation = {};

	usp.objects = object_none;		//no objects in this segment

	assign_default_uvs_to_segment(sp);

	// Assume nothing special about this segment
	sp->special = segment_special::nothing;
	sp->station_idx = station_number::None;
	usp.static_light = 0;
	sp->matcen_num = materialization_center_number::None;
}

// -------------------------------------------------------------------------------
void med_create_new_segment_from_cursegp(void)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	vms_vector	scalevec;
	vms_vector	uvec, rvec, fvec;

	med_extract_up_vector_from_segment_side(Cursegp, Curside, uvec);
	med_extract_right_vector_from_segment_side(Cursegp, Curside, rvec);
	auto &vcvertptr = Vertices.vcptr;
	extract_forward_vector_from_segment(vcvertptr, Cursegp, fvec);

	scalevec.x = vm_vec_mag(rvec);
	scalevec.y = vm_vec_mag(uvec);
	scalevec.z = vm_vec_mag(fvec);
	med_create_new_segment(scalevec);
}

// -------------------------------------------------------------------------------
//	Initialize all vertices to inactive status.
void init_all_vertices(void)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertex_active = LevelSharedVertexState.get_vertex_active();
	Vertex_active = {};
	range_for (auto &s, Segments)
		s.segnum = segment_none;
}

// -----------------------------------------------------------------------------
//	Create coordinate axes in orientation of specified segment, stores vertices at *vp.
void create_coordinate_axes_from_segment(const shared_segment &sp, std::array<vertnum_t, 16> &vertnums)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	vms_matrix	rotmat;
	vms_vector t;

	med_extract_matrix_from_segment(sp, rotmat);

	auto &vcvertptr = Vertices.vcptr;
	auto &vmvertptr = Vertices.vmptr;
	const auto &&v0 = vmvertptr(vertnums[0]);
	compute_segment_center(vcvertptr, v0, sp);

	t = rotmat.rvec;
	vm_vec_scale(t,i2f(32));
	vm_vec_add(vmvertptr(vertnums[1]), v0, t);

	t = rotmat.uvec;
	vm_vec_scale(t,i2f(32));
	vm_vec_add(vmvertptr(vertnums[2]), v0, t);

	t = rotmat.fvec;
	vm_vec_scale(t,i2f(32));
	vm_vec_add(vmvertptr(vertnums[3]), v0, t);
}

namespace {

// -----------------------------------------------------------------------------
//	Determine if a segment is concave. Returns true if concave
static int check_seg_concavity(const shared_segment &s)
{
	vms_vector n0;
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vcvertptr = Vertices.vcptr;
	range_for (auto &sn, Side_to_verts)
		for (unsigned vn = 0; vn <= 4; ++vn)
		{
			const auto n1{vm_vec_normal(
				vcvertptr(s.verts[sn[static_cast<side_relative_vertnum>(vn % 4)]]),
				vcvertptr(s.verts[sn[static_cast<side_relative_vertnum>((vn + 1) % 4)]]),
				vcvertptr(s.verts[sn[static_cast<side_relative_vertnum>((vn + 2) % 4)]])
			)};

			//vm_vec_normalize(&n1);

			if (vn>0) if (vm_vec_dot(n0,n1) < f0_5) return 1;

			n0 = n1;
		}

	return 0;
}

}

// -----------------------------------------------------------------------------
//	Find all concave segments and add to list
void find_concave_segs()
{
	Warning_segs.clear();

	range_for (const auto &&s, vcsegptridx)
		if (s->segnum != segment_none)
			if (check_seg_concavity(s))
				Warning_segs.emplace_back(s);
}


// -----------------------------------------------------------------------------
void warn_if_concave_segments(void)
{
	find_concave_segs();

	if (!Warning_segs.empty())
	{
		editor_status_fmt("*** WARNING *** %d concave segments in mine! *** WARNING ***", Warning_segs.size());
    }
}

// -----------------------------------------------------------------------------
//	Check segment s, if concave, warn
void warn_if_concave_segment(const vmsegptridx_t s)
{
	int	result;

	result = check_seg_concavity(s);

	if (result) {
		Warning_segs.emplace_back(s);

			editor_status("*** WARNING *** New segment is concave! *** WARNING ***");
	} //else
        //editor_status("");
}


// -------------------------------------------------------------------------------
//	Find segment adjacent to sp:side.
//	Adjacent means a segment which shares all four vertices.
//	If found, return a pair containing the found segment and side.
//	If not found, return an empty optional.
std::optional<std::pair<vmsegptridx_t, sidenum_t>> med_find_adjacent_segment_side(const vmsegptridx_t sp, sidenum_t side)
{
	enumerated_array<vertnum_t, 4, side_relative_vertnum> abs_verts;

	//	Stuff abs_verts[4] array with absolute vertex indices
	for (const auto v : MAX_VERTICES_PER_SIDE)
		abs_verts[v] = sp->verts[Side_to_verts[side][v]];

	//	Scan all segments, looking for a segment which contains the four abs_verts
	range_for (const auto &&segp, vmsegptridx)
	{
		if (segp != sp)
		{
			range_for (auto &v, abs_verts)
			{												// do for each vertex in abs_verts
				range_for (auto &vv, segp->verts) // do for each vertex in segment
					if (v == vv)
						goto fass_found1;											// Current vertex (indexed by v) is present in segment, try next
				goto fass_next_seg;												// This segment doesn't contain the vertex indexed by v
			fass_found1: ;
			}		// end for v

			//	All four vertices in sp:side are present in segment seg.
			//	Determine side and return
			for (const auto &&[idx, value] : enumerate(Side_to_verts))
			{
				for (const auto v : value)
				{
					range_for (auto &vv, abs_verts)
					{
						if (segp->verts[v] == vv)
							goto fass_found2;
					}
					goto fass_next_side;											// Couldn't find vertex v in current side, so try next side.
				fass_found2: ;
				}
				// Found all four vertices in current side.  We are done!
				return std::pair(segp, static_cast<sidenum_t>(idx));
			fass_next_side: ;
			}
			Assert(0);	// Impossible -- we identified this segment as containing all 4 vertices of side "side", but we couldn't find them.
			return std::nullopt;
		fass_next_seg: ;
		}
	}
	return std::nullopt;
}


#define JOINT_THRESHOLD	10000*F1_0 		// (Huge threshold)

// -------------------------------------------------------------------------------
//	Find segment closest to sp:side.
//	Return true if segment found and fill in segment in adj_sp and side in adj_side.
//	Return false if unable to find, in which case adj_sp and adj_side are undefined.
std::optional<std::pair<vmsegptridx_t, sidenum_t>> med_find_closest_threshold_segment_side(const vmsegptridx_t sp, sidenum_t side, const fix threshold)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	fix			closest_seg_dist;

	if (IS_CHILD(sp->children[side]))
		return std::nullopt;

	auto &vcvertptr = Vertices.vcptr;
	const auto &&vsc = compute_center_point_on_side(vcvertptr, sp, side);

	closest_seg_dist = JOINT_THRESHOLD;

	std::optional<std::pair<vmsegptridx_t, sidenum_t>> result;
	//	Scan all segments, looking for a segment which contains the four abs_verts
	range_for (const auto &&segp, vmsegptridx)
	{
		if (segp != sp) 
			for (const auto &&[idx, value] : enumerate(segp->children))
			{
				if (!IS_CHILD(value))
				{
					const auto &&vtc = compute_center_point_on_side(vcvertptr, segp, idx);
					const auto current_dist = vm_vec_dist(vsc, vtc);
					if (closest_seg_dist > current_dist)
					{
						result = {segp, static_cast<sidenum_t>(idx)};
						closest_seg_dist = current_dist;
					}
				}
			}	
	}

	if (closest_seg_dist < threshold)
		return result;
	else
		return std::nullopt;
}

}
