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
 * Functions for sizing segments
 *
 */

#include <stdlib.h>
#include "editor.h"
#include "editor/esegment.h"
#include "dxxerror.h"
#include "gameseg.h"
#include "kdefs.h"

#include "compiler-range_for.h"
#include "d_range.h"

#define XDIM	0
#define YDIM	1
#define ZDIM	2

#define	MAX_MODIFIED_VERTICES	32
namespace {
static std::array<vertnum_t, MAX_MODIFIED_VERTICES>		Modified_vertices;
int		Modified_vertex_index = 0;
}

namespace dsx {

namespace {

// ------------------------------------------------------------------------------------------
static void validate_modified_segments(void)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	visited_segment_bitarray_t modified_segments;
	auto &vcvertptr = Vertices.vcptr;
	for (int v=0; v<Modified_vertex_index; v++) {
		const auto v0 = Modified_vertices[v];

		range_for (const auto &&segp, vmsegptridx)
		{
			if (segp->segnum != segment_none)
			{
				if (modified_segments[segp])
					continue;
				range_for (const auto w, segp->verts)
					if (w == v0)
					{
						modified_segments[segp] = true;
						validate_segment(vcvertptr, segp);
						for (const auto s : MAX_SIDES_PER_SEGMENT)
						{
							Num_tilings = 1;
							assign_default_uvs_to_side(segp, s);
						}
						break;
					}
			}
		}
	}
}

// ------------------------------------------------------------------------------------------
//	Scale vertex *vertp by vector *vp, scaled by scale factor scale_factor
static void scale_vert_aux(const vertnum_t vertex_ind, const vms_vector &vp, const fix scale_factor)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vmvertptr = Vertices.vmptr;
	auto &vertp = *vmvertptr(vertex_ind);

	vertp.x += fixmul(vp.x,scale_factor)/2;
	vertp.y += fixmul(vp.y,scale_factor)/2;
	vertp.z += fixmul(vp.z,scale_factor)/2;

	Assert(Modified_vertex_index < MAX_MODIFIED_VERTICES);
	Modified_vertices[Modified_vertex_index++] = vertex_ind;
}

// ------------------------------------------------------------------------------------------
static void scale_vert(const shared_segment &sp, const vertnum_t vertex_ind, const vms_vector &vp, const fix scale_factor)
{
	auto &verts = sp.verts;
	switch (SegSizeMode) {
		case SEGSIZEMODE_FREE:
			if (is_free_vertex(vcsegptr, vertex_ind))
				scale_vert_aux(vertex_ind, vp, scale_factor);
			break;
		case SEGSIZEMODE_ALL:
			scale_vert_aux(vertex_ind, vp, scale_factor);
			break;
		case SEGSIZEMODE_CURSIDE: {
			range_for (const auto v, Side_to_verts[Curside])
				if (verts[v] == vertex_ind)
					scale_vert_aux(vertex_ind, vp, scale_factor);
			break;
		}
		case SEGSIZEMODE_EDGE: {
			auto &sv = Side_to_verts[Curside];
			const auto edge = Curedge;
			range_for (const int v, xrange(2u))
				if (verts[sv[next_side_vertex(edge, v)]] == vertex_ind)
					scale_vert_aux(vertex_ind, vp, scale_factor);
			break;
		}
		case SEGSIZEMODE_VERTEX:
			if (verts[Side_to_verts[Curside][Curvert]] == vertex_ind)
				scale_vert_aux(vertex_ind, vp, scale_factor);
			break;
		default:
			Error("Unsupported SegSizeMode in ksegsize.c/scale_vert = %i\n", SegSizeMode);
	}

}

// ------------------------------------------------------------------------------------------
static void scale_free_verts(const shared_segment &sp, const vms_vector &vp, const sidenum_t side, fix scale_factor)
{
	range_for (auto &v, Side_to_verts[side])
	{
		const auto vertex_ind = sp.verts[v];
		if (SegSizeMode || is_free_vertex(vcsegptr, vertex_ind))
			scale_vert(sp, vertex_ind, vp, scale_factor);
	}

}


// -----------------------------------------------------------------------------
//	Make segment *sp bigger in dimension dimension by amount amount.
static void med_scale_segment_new(const shared_segment &sp, const int dimension, const fix amount)
{
	vms_matrix	mat;

	Modified_vertex_index = 0;

	med_extract_matrix_from_segment(sp, mat);

	const vms_vector *vec;
	sidenum_t side0, side1;
	switch (dimension) {
		case XDIM:
			side0 = sidenum_t::WLEFT;
			side1 = sidenum_t::WRIGHT;
			vec = &mat.rvec;
			break;
		case YDIM:
			side0 = sidenum_t::WBOTTOM;
			side1 = sidenum_t::WTOP;
			vec = &mat.uvec;
			break;
		case ZDIM:
			side0 = sidenum_t::WFRONT;
			side1 = sidenum_t::WBACK;
			vec = &mat.fvec;
			break;
		default:
			return;
	}
	scale_free_verts(sp, *vec, side0, -amount);
	scale_free_verts(sp, *vec, side1, +amount);

	validate_modified_segments();
}

// ------------------------------------------------------------------------------------------
//	Extract a vector from a segment.  The vector goes from the start face to the end face.
//	The point on each face is the average of the four points forming the face.
static void extract_vector_from_segment_side(const shared_segment &sp, const sidenum_t side, vms_vector &vp, const side_relative_vertnum vla, const side_relative_vertnum vlb, const side_relative_vertnum vra, const side_relative_vertnum vrb)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &sv = Side_to_verts[side];
	auto &verts = sp.verts;
	auto &vcvertptr = Vertices.vcptr;
	const auto v1 = vm_vec_sub(vcvertptr(verts[sv[vra]]), vcvertptr(verts[sv[vla]]));
	const auto v2 = vm_vec_sub(vcvertptr(verts[sv[vrb]]), vcvertptr(verts[sv[vlb]]));
	vm_vec_add(vp, v1, v2);
	vm_vec_scale(vp, F1_0/2);
}

}

// ------------------------------------------------------------------------------------------
//	Extract the right vector from segment *sp, return in *vp.
//	The forward vector is defined to be the vector from the the center of the left face of the segment
// to the center of the right face of the segment.
void med_extract_right_vector_from_segment_side(const shared_segment &sp, const sidenum_t sidenum, vms_vector &vp)
{
	extract_vector_from_segment_side(sp, sidenum, vp, side_relative_vertnum::_3, side_relative_vertnum::_2, side_relative_vertnum::_0, side_relative_vertnum::_1);
}

// ------------------------------------------------------------------------------------------
//	Extract the up vector from segment *sp, return in *vp.
//	The forward vector is defined to be the vector from the the center of the bottom face of the segment
// to the center of the top face of the segment.
void med_extract_up_vector_from_segment_side(const shared_segment &sp, const sidenum_t sidenum, vms_vector &vp)
{
	extract_vector_from_segment_side(sp, sidenum, vp, side_relative_vertnum::_1, side_relative_vertnum::_2, side_relative_vertnum::_0, side_relative_vertnum::_3);
}

namespace {

// -----------------------------------------------------------------------------
//	Increase the size of Cursegp in dimension dimension by amount
static int segsize_common(int dimension, fix amount)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	vms_vector	uvec, rvec, fvec, scalevec;

	Degenerate_segment_found = 0;

	med_scale_segment_new(Cursegp, dimension, amount);

	med_extract_up_vector_from_segment_side(Cursegp, Curside, uvec);
	med_extract_right_vector_from_segment_side(Cursegp, Curside, rvec);
	auto &vcvertptr = Vertices.vcptr;
	extract_forward_vector_from_segment(vcvertptr, Cursegp, fvec);

	scalevec.x = vm_vec_mag(rvec);
	scalevec.y = vm_vec_mag(uvec);
	scalevec.z = vm_vec_mag(fvec);

	if (Degenerate_segment_found) {
		Degenerate_segment_found = 0;
		editor_status("Applying scale would create degenerate segments.  Aborting scale.");
		med_scale_segment_new(Cursegp, dimension, -amount);
		return 1;
	}

	med_create_new_segment(scalevec);

	//	For all segments to which Cursegp is connected, propagate tmap (uv coordinates) from the connected
	//	segment back to Cursegp.  This will meaningfully propagate uv coordinates to all sides which havve
	//	an incident edge.  It will also do some sides more than once.  And it is probably just not what you want.
	per_side_array<int> propagated{};
	for (const auto i : MAX_SIDES_PER_SEGMENT)
	{
		const auto c = Cursegp->children[i];
		if (IS_CHILD(c))
		{
			range_for (auto &s, propagated)
				++s;
			-- propagated[Side_opposite[i]];
			med_propagate_tmaps_to_segments(vmsegptridx(c), Cursegp, 1);
		}
	}

	//	Now, for all sides that were not adjacent to another side, and therefore did not get tmaps
	//	propagated to them, treat as a back side.
	for (const auto i : MAX_SIDES_PER_SEGMENT)
		if (!propagated[i]) {
			med_propagate_tmaps_to_back_side(Cursegp, i, 1);
		}

	//	New stuff, assign default texture to all affected sides.

	Update_flags |= UF_WORLD_CHANGED;
	mine_changed = 1;
	return 1;
}

}

// -----------------------------------------------------------------------------
// ---------- segment size control ----------

int IncreaseSegLength()
{
	return segsize_common(ZDIM,+F1_0);
}

int DecreaseSegLength()
{
	return segsize_common(ZDIM,-F1_0);
}

int DecreaseSegWidth()
{
	return segsize_common(XDIM,-F1_0);
}

int IncreaseSegWidth()
{
	return segsize_common(XDIM,+F1_0);
}

int IncreaseSegHeight()
{
	return segsize_common(YDIM,+F1_0);
}

int DecreaseSegHeight()
{
	return segsize_common(YDIM,-F1_0);
}


int IncreaseSegLengthBig()
{
	return segsize_common(ZDIM,+5 * F1_0);
}

int DecreaseSegLengthBig()
{
	return segsize_common(ZDIM,-5 * F1_0);
}

int DecreaseSegWidthBig()
{
	return segsize_common(XDIM,-5 * F1_0);
}

int IncreaseSegWidthBig()
{
	return segsize_common(XDIM,+5 * F1_0);
}

int IncreaseSegHeightBig()
{
	return segsize_common(YDIM,+5 * F1_0);
}

int DecreaseSegHeightBig()
{
	return segsize_common(YDIM,-5 * F1_0);
}


int IncreaseSegLengthDefault()
{
	return segsize_common(ZDIM,+40 *F1_0);
}

int DecreaseSegLengthDefault()
{
	return segsize_common(ZDIM,-40*F1_0);
}

int IncreaseSegWidthDefault()
{
	return segsize_common(XDIM,+40*F1_0);
}

int DecreaseSegWidthDefault()
{
	return segsize_common(XDIM,-40*F1_0);
}

int IncreaseSegHeightDefault()
{
	return segsize_common(YDIM,+40 * F1_0);
}

int DecreaseSegHeightDefault()
{
	return segsize_common(YDIM,-40 * F1_0);
}


//	---------------------------------------------------------------------------
int ToggleSegSizeMode(void)
{
	SegSizeMode++;
	if (SegSizeMode > SEGSIZEMODE_MAX)
		SegSizeMode = SEGSIZEMODE_MIN;

	return 1;
}

namespace {

//	---------------------------------------------------------------------------
static int	PerturbCursideCommon(fix amount)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	int			saveSegSizeMode = SegSizeMode;
	vms_vector	fvec, uvec;
	SegSizeMode = SEGSIZEMODE_CURSIDE;

	Modified_vertex_index = 0;

	auto &vcvertptr = Vertices.vcptr;
	extract_forward_vector_from_segment(vcvertptr, Cursegp, fvec);
	const auto rvec{extract_right_vector_from_segment(vcvertptr, Cursegp)};
	extract_up_vector_from_segment(vcvertptr, Cursegp, uvec);

	const auto fmag = vm_vec_mag(fvec);
	const auto rmag = vm_vec_mag(rvec);
	const auto umag = vm_vec_mag(uvec);

	range_for (const auto v, Side_to_verts[Curside])
	{
		vms_vector perturb_vec;
		perturb_vec.x = fixmul(rmag, d_rand()*2 - 32767);
		perturb_vec.y = fixmul(umag, d_rand()*2 - 32767);
		perturb_vec.z = fixmul(fmag, d_rand()*2 - 32767);
		scale_vert(Cursegp, Cursegp->verts[v], perturb_vec, amount);
	}

//	validate_segment(Cursegp);
//	if (SegSizeMode) {
//		for (i=0; i<MAX_SIDES_PER_SEGMENT; i++)
//			if (Cursegp->children[i] != -1)
//				validate_segment(&Segments[Cursegp->children[i]]);
//	}

	validate_modified_segments();
	SegSizeMode = saveSegSizeMode;

	Update_flags |= UF_WORLD_CHANGED;
	mine_changed = 1;

	return 1;
}

}

//	---------------------------------------------------------------------------
int	PerturbCurside(void)
{
	PerturbCursideCommon(F1_0/10);

	return 1;
}

//	---------------------------------------------------------------------------
int	PerturbCursideBig(void)
{
	PerturbCursideCommon(F1_0/2);

	return 1;
}

}
