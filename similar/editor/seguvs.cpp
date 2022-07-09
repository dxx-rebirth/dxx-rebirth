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
 * u,v coordinate computation for segment faces
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include "inferno.h"
#include "segment.h"
#include "editor/editor.h"
#include "editor/esegment.h"
#include "gameseg.h"
#include "maths.h"
#include "dxxerror.h"
#include "wall.h"
#include "editor/kdefs.h"
#include "bm.h"		//	Needed for TmapInfo
#include "fvi.h"
#include "render.h"
#include "seguvs.h"

#include "compiler-range_for.h"
#include "d_enumerate.h"
#include "d_range.h"
#include "d_zip.h"

namespace dcx {
namespace {
static void cast_all_light_in_mine(int quick_flag);
}
}
//--rotate_uvs-- vms_vector Rightvec;

#define	MAX_LIGHT_SEGS 16

namespace {

//	---------------------------------------------------------------------------------------------
//	Scan all polys in all segments, return average light value for vnum.
//	segs = output array for segments containing vertex, terminated by -1.
static fix get_average_light_at_vertex(const vertnum_t vnum, segnum_t *segs)
{
	fix	total_light;
	int	num_occurrences;
//	#ifndef NDEBUG //Removed this ifdef because the version of Assert that I used to get it to compile doesn't work without this symbol. -KRB
        segnum_t   *original_segs;

        original_segs = segs;
//	#endif


	num_occurrences = 0;
	total_light = 0;

	range_for (const auto &&segp, vcsegptridx)
	{
		auto e = end(segp->verts);
		auto it = std::find(begin(segp->verts), e, vnum);
		if (it == e)
			continue;
		auto relvnum = static_cast<segment_relative_vertnum>(std::distance(it, e));
		{
			*segs++ = segp;
			Assert(segs - original_segs < MAX_LIGHT_SEGS);
			(void)original_segs;

			for (const auto &&[child_segnum, uside, vp] : zip(segp->children, segp->unique_segment::sides, Side_to_verts))
			{
				if (!IS_CHILD(child_segnum))
				{
					const auto vb = begin(vp);
					const auto ve = end(vp);
					const auto vi = std::find(vb, ve, relvnum);
					if (vi != ve)
					{
						const auto v = static_cast<side_relative_vertnum>(std::distance(vb, vi));
						total_light += uside.uvls[v].l;
							num_occurrences++;
						}
				}	// end if
			}	// end sidenum
		}
	}	// end segnum

	*segs = segment_none;

	if (num_occurrences)
		return total_light/num_occurrences;
	else
		return 0;

}

static void set_average_light_at_vertex(vertnum_t vnum)
{
	segnum_t	Segment_indices[MAX_LIGHT_SEGS];
	int	segind;

	fix average_light;

	average_light = get_average_light_at_vertex(vnum, Segment_indices);

	if (!average_light)
		return;

	segind = 0;
	while (Segment_indices[segind] != segment_none) {
		auto segnum = Segment_indices[segind++];

		auto &ssegp = *vcsegptr(segnum);
		unique_segment &usegp = *vmsegptr(segnum);

		for (const auto &&[relvnum, vert] : enumerate(ssegp.verts))
			if (vert == vnum)
			{
			for (const auto &&[child_segnum, sidep, vp] : zip(ssegp.children, usegp.sides, Side_to_verts))
			{
				if (!IS_CHILD(child_segnum))
				{
					const auto vb = begin(vp);
					const auto ve = end(vp);
					const auto vi = std::find(vb, ve, relvnum);
					if (vi != ve)
					{
						const auto v = static_cast<side_relative_vertnum>(std::distance(vb, vi));
						sidep.uvls[v].l = average_light;
					}
				}	// end if
			}	// end sidenum
				break;
			}	// end if
	}	// end while

	Update_flags |= UF_WORLD_CHANGED;
}

static void set_average_light_on_side(const shared_segment &segp, const sidenum_t sidenum)
{
	if (!IS_CHILD(segp.children[sidenum]))
		range_for (const auto v, Side_to_verts[sidenum])
		{
			set_average_light_at_vertex(segp.verts[v]);
		}
}

}

int set_average_light_on_curside(void)
{
	set_average_light_on_side(Cursegp, Curside);
	return 0;
}

int set_average_light_on_all(void)
{
	Doing_lighting_hack_flag = 1;
	cast_all_light_in_mine(0);
	Doing_lighting_hack_flag = 0;
	Update_flags |= UF_WORLD_CHANGED;

//	int seg, side;

//	for (seg=0; seg<=Highest_segment_index; seg++)
//		for (side=0; side<MAX_SIDES_PER_SEGMENT; side++)
//			if (Segments[seg].segnum != -1)
//				set_average_light_on_side(&Segments[seg], side);
	return 0;
}

int set_average_light_on_all_quick(void)
{
	cast_all_light_in_mine(1);
	Update_flags |= UF_WORLD_CHANGED;

	return 0;
}

namespace {

//	---------------------------------------------------------------------------------------------
//	Given a polygon, compress the uv coordinates so that they are as close to 0 as possible.
//	Do this by adding a constant u and v to each uv pair.
static void compress_uv_coordinates(std::array<uvl, 4> &uvls)
{
	fix	uc, vc;

	uc = 0;
	vc = 0;

	range_for (auto &uvl, uvls)
	{
		uc += uvl.u;
		vc += uvl.v;
	}

	uc /= 4;
	vc /= 4;
	uc = uc & 0xffff0000;
	vc = vc & 0xffff0000;

	range_for (auto &uvl, uvls)
	{
		uvl.u -= uc;
		uvl.v -= vc;
	}
}

static void assign_default_lighting_on_side(std::array<uvl, 4> &uvls)
{
	range_for (auto &uvl, uvls)
		uvl.l = DEFAULT_LIGHTING;
}

static void assign_default_lighting(unique_segment &segp)
{
	range_for (auto &side, segp.sides)
		assign_default_lighting_on_side(side.uvls);
}

}

void assign_default_lighting_all(void)
{
	range_for (const auto &&segp, vmsegptr)
	{
		if (segp->segnum != segment_none)
			assign_default_lighting(segp);
	}
}

namespace {

//	---------------------------------------------------------------------------------------------
static void validate_uv_coordinates(unique_segment &segp)
{
	range_for (auto &side, segp.sides)
	{
		compress_uv_coordinates(side.uvls);
	}

}

#ifdef __WATCOMC__
fix zhypot(fix a,fix b);
#pragma aux zhypot parm [eax] [ebx] value [eax] modify [eax ebx ecx edx] = \
	"imul	eax" \
	"xchg eax,ebx" \
	"mov	ecx,edx" \
	"imul eax" \
	"add	eax,ebx" \
	"adc	edx,ecx" \
	"call	quad_sqrt";
#else
static fix zhypot(fix a,fix b) {
	double x = static_cast<double>(a) / 65536;
	double y = static_cast<double>(b) / 65536;
	return static_cast<long>(sqrt(x * x + y * y) * 65536);
}
#endif

}

//	---------------------------------------------------------------------------------------------
//	Assign lighting value to side, a function of the normal vector.
void assign_light_to_side(unique_side &s)
{
	range_for (auto &v, s.uvls)
		v.l = DEFAULT_LIGHTING;
}

fix	Stretch_scale_x = F1_0;
fix	Stretch_scale_y = F1_0;

namespace {

//	---------------------------------------------------------------------------------------------
//	Given u,v coordinates at two vertices, assign u,v coordinates to other two vertices on a side.
//	(Actually, assign them to the coordinates in the faces.)
//	va, vb = face-relative vertex indices corresponding to uva, uvb.  Ie, they are always in 0..3 and should be looked up in
//	Side_to_verts[side] to get the segment relative index.
static void assign_uvs_to_side(fvcvertptr &vcvertptr, const vmsegptridx_t segp, const sidenum_t sidenum, const uvl &uva, const uvl &uvb, const side_relative_vertnum va, const side_relative_vertnum vb)
{
	uvl ruvmag,fuvmag;
	const uvl *uvlo, *uvhi;
	fix			fmag,mag01;
#ifndef NDEBUG
	{
		const auto distance_between_side_vertices = underlying_value(va) - underlying_value(vb);
		const auto abs_distance_between_side_vertices = std::abs(distance_between_side_vertices);
		// make sure the vertices specify an edge
		assert(abs_distance_between_side_vertices == 1 || abs_distance_between_side_vertices == 3);
	}
#endif

	auto &vp = Side_to_verts[sidenum];

	side_relative_vertnum vlo, vhi;
	// We want vlo precedes vhi, ie vlo < vhi, or vlo = 3, vhi = 0
	if (va == next_side_vertex(vb))
	{		// va = vb + 1
		vlo = vb;
		vhi = va;
		uvlo = &uvb;
		uvhi = &uva;
	} else {
		vlo = va;
		vhi = vb;
		uvlo = &uva;
		uvhi = &uvb;
	}

	assert(next_side_vertex(vlo) == vhi);	// If we are on an edge, then uvhi is one more than uvlo (mod 4)
	enumerated_array<uvl, 4, side_relative_vertnum> uvls;
	uvls[vlo] = *uvlo;
	uvls[vhi] = *uvhi;

	// Now we have vlo precedes vhi, compute vertices ((vhi+1) % 4) and ((vhi+2) % 4)

	// Assign u,v scale to a unit length right vector.
	fmag = zhypot(uvhi->v - uvlo->v, uvhi->u - uvlo->u);
	if (fmag < 64) {		// this is a fix, so 64 = 1/1024
		ruvmag.u = F1_0*256;
		ruvmag.v = F1_0*256;
		fuvmag.u = F1_0*256;
		fuvmag.v = F1_0*256;
	} else {
		ruvmag.u = uvhi->v - uvlo->v;
		ruvmag.v = uvlo->u - uvhi->u;

		fuvmag.u = uvhi->u - uvlo->u;
		fuvmag.v = uvhi->v - uvlo->v;
	}

	const auto v0 = segp->verts[vp[vlo]];
	const auto v1 = segp->verts[vp[vhi]];
	const auto vhi1 = next_side_vertex(vhi);
	const auto vhi2 = next_side_vertex(vhi, 2);
	const auto v2 = segp->verts[vp[vhi1]];
	const auto v3 = segp->verts[vp[vhi2]];

	//	Compute right vector by computing orientation matrix from:
	//		forward vector = vlo:vhi
	//		  right vector = vlo:(vhi+2) % 4
	const auto &&vp0 = vcvertptr(v0);
	const auto &vv1v0 = vm_vec_sub(vcvertptr(v1), vp0);
	mag01 = vm_vec_mag(vv1v0);
	mag01 = fixmul(mag01, (va == side_relative_vertnum::_0 || va == side_relative_vertnum::_2) ? Stretch_scale_x : Stretch_scale_y);

	if (unlikely(mag01 < F1_0/1024))
		editor_status_fmt("U, V bogosity in segment #%hu, probably on side #%i.  CLEAN UP YOUR MESS!", segp.get_unchecked_index(), underlying_value(sidenum));
	else {
		struct frvec {
			vms_vector fvec, rvec;
			frvec(const vms_vector &tfvec, const vms_vector &trvec) {
				if ((tfvec.x == 0 && tfvec.y == 0 && tfvec.z == 0) ||
					(trvec.x == 0 && trvec.y == 0 && trvec.z == 0))
				{
					fvec = vmd_identity_matrix.fvec;
					rvec = vmd_identity_matrix.rvec;
				}
				else
				{
					const auto &m = vm_vector_2_matrix(tfvec, nullptr, &trvec);
					fvec = m.fvec;
					rvec = m.rvec;
				}
				vm_vec_negate(rvec);
			}
		};
		const auto &vv3v0 = vm_vec_sub(vcvertptr(v3), vp0);
		const frvec fr{
			vv1v0,
			vv3v0
		};
		const auto build_uvl = [&](const vms_vector &tvec, const uvl &uvi) {
			const auto drt = vm_vec_dot(fr.rvec, tvec);
			const auto dft = vm_vec_dot(fr.fvec, tvec);
			return uvl{
				uvi.u +
					fixdiv(fixmul(ruvmag.u, drt), mag01) +
					fixdiv(fixmul(fuvmag.u, dft), mag01),
				uvi.v +
					fixdiv(fixmul(ruvmag.v, drt), mag01) +
					fixdiv(fixmul(fuvmag.v, dft), mag01),
				uvi.l
			};
		};
		uvls[vhi1] = build_uvl(vm_vec_sub(vcvertptr(v2), vcvertptr(v1)), *uvhi);
		uvls[vhi2] = build_uvl(vv3v0, *uvlo);
		//	For all faces in side, copy uv coordinates from uvs array to face.
		segp->unique_segment::sides[sidenum].uvls = uvls;
	}
}

}


int Vmag = VMAG;

namespace dsx {

// -----------------------------------------------------------------------------------------------------------
//	Assign default uvs to side.
//	This means:
//		v0 = 0,0
//		v1 = k,0 where k is 3d size dependent
//	v2, v3 assigned by assign_uvs_to_side
void assign_default_uvs_to_side(const vmsegptridx_t segp, const sidenum_t side)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	uvl			uv0,uv1;
	uv0.u = 0;
	uv0.v = 0;
	auto &vp = Side_to_verts[side];
	uv1.u = 0;
	auto &vcvertptr = Vertices.vcptr;
	uv1.v = Num_tilings * fixmul(Vmag, vm_vec_dist(vcvertptr(segp->verts[vp[side_relative_vertnum::_1]]), vcvertptr(segp->verts[vp[side_relative_vertnum::_0]])));

	assign_uvs_to_side(vcvertptr, segp, side, uv0, uv1, side_relative_vertnum::_0, side_relative_vertnum::_1);
}

// -----------------------------------------------------------------------------------------------------------
//	Assign default uvs to side.
//	This means:
//		v0 = 0,0
//		v1 = k,0 where k is 3d size dependent
//	v2, v3 assigned by assign_uvs_to_side
void stretch_uvs_from_curedge(const vmsegptridx_t segp, const sidenum_t side)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	uvl			uv0,uv1;

	const auto v0 = Curedge;
	const auto v1 = next_side_vertex(v0);

	const auto &uvls = segp->unique_segment::sides[side].uvls;
	uv0.u = uvls[v0].u;
	uv0.v = uvls[v0].v;

	uv1.u = uvls[v1].u;
	uv1.v = uvls[v1].v;

	auto &vcvertptr = Vertices.vcptr;
	assign_uvs_to_side(vcvertptr, segp, side, uv0, uv1, v0, v1);
}

// --------------------------------------------------------------------------------------------------------------
//	Assign default uvs to a segment.
void assign_default_uvs_to_segment(const vmsegptridx_t segp)
{
	for (const auto s : MAX_SIDES_PER_SEGMENT)
	{
		assign_default_uvs_to_side(segp,s);
		assign_light_to_side(segp, s);
	}
}


// -- mk021394 -- // --------------------------------------------------------------------------------------------------------------
// -- mk021394 -- //	Find the face:poly:vertex index in base_seg:base_common_side which is segment relative vertex v1
// -- mk021394 -- //	This very specific routine is subsidiary to med_assign_uvs_to_side.
// -- mk021394 -- void get_face_and_vert(segment *base_seg, int base_common_side, int v1, int *ff, int *vv, int *pi)
// -- mk021394 -- {
// -- mk021394 -- 	int	p,f,v;
// -- mk021394 -- 
// -- mk021394 -- 	for (f=0; f<base_seg->sides[base_common_side].num_faces; f++) {
// -- mk021394 -- 		face *fp = &base_seg->sides[base_common_side].faces[f];
// -- mk021394 -- 		for (p=0; p<fp->num_polys; p++) {
// -- mk021394 -- 			poly *pp = &fp->polys[p];
// -- mk021394 -- 			for (v=0; v<pp->num_vertices; v++)
// -- mk021394 -- 				if (pp->verts[v] == v1) {
// -- mk021394 -- 					*ff = f;
// -- mk021394 -- 					*vv = v;
// -- mk021394 -- 					*pi = p;
// -- mk021394 -- 					return;
// -- mk021394 -- 				}
// -- mk021394 -- 		}
// -- mk021394 -- 	}
// -- mk021394 -- 
// -- mk021394 -- 	Assert(0);	// Error -- Couldn't find face:vertex which matched vertex v1 on base_seg:base_common_side
// -- mk021394 -- }

// -- mk021394 -- // --------------------------------------------------------------------------------------------------------------
// -- mk021394 -- //	Find the vertex index in base_seg:base_common_side which is segment relative vertex v1
// -- mk021394 -- //	This very specific routine is subsidiary to med_assign_uvs_to_side.
// -- mk021394 -- void get_side_vert(segment *base_seg,int base_common_side,int v1,int *vv)
// -- mk021394 -- {
// -- mk021394 -- 	int	p,f,v;
// -- mk021394 -- 
// -- mk021394 -- 	Assert((base_seg->sides[base_common_side].tri_edge == 0) || (base_seg->sides[base_common_side].tri_edge == 1));
// -- mk021394 -- 	Assert(base_seg->sides[base_common_side].num_faces <= 2);
// -- mk021394 -- 
// -- mk021394 -- 	for (f=0; f<base_seg->sides[base_common_side].num_faces; f++) {
// -- mk021394 -- 		face *fp = &base_seg->sides[base_common_side].faces[f];
// -- mk021394 -- 		for (p=0; p<fp->num_polys; p++) {
// -- mk021394 -- 			poly	*pp = &fp->polys[p];
// -- mk021394 -- 			for (v=0; v<pp->num_vertices; v++)
// -- mk021394 -- 				if (pp->verts[v] == v1) {
// -- mk021394 -- 					if (pp->num_vertices == 4) {
// -- mk021394 -- 						*vv = v;
// -- mk021394 -- 						return;
// -- mk021394 -- 					}
// -- mk021394 -- 
// -- mk021394 -- 					if (base_seg->sides[base_common_side].tri_edge == 0) {	// triangulated 012, 023, so if f==0, *vv = v, if f==1, *vv = v if v=0, else v+1
// -- mk021394 -- 						if ((f == 1) && (v > 0))
// -- mk021394 -- 							v++;
// -- mk021394 -- 						*vv = v;
// -- mk021394 -- 						return;
// -- mk021394 -- 					} else {								// triangulated 013, 123
// -- mk021394 -- 						if (f == 0) {
// -- mk021394 -- 							if (v == 2)
// -- mk021394 -- 								v++;
// -- mk021394 -- 						} else
// -- mk021394 -- 							v++;
// -- mk021394 -- 						*vv = v;
// -- mk021394 -- 						return;
// -- mk021394 -- 					}
// -- mk021394 -- 				}
// -- mk021394 -- 		}
// -- mk021394 -- 	}
// -- mk021394 -- 
// -- mk021394 -- 	Assert(0);	// Error -- Couldn't find face:vertex which matched vertex v1 on base_seg:base_common_side
// -- mk021394 -- }

//--rotate_uvs-- // --------------------------------------------------------------------------------------------------------------
//--rotate_uvs-- //	Rotate uvl coordinates uva, uvb about their center point by heading
//--rotate_uvs-- void rotate_uvs(uvl *uva, uvl *uvb, vms_vector *rvec)
//--rotate_uvs-- {
//--rotate_uvs-- 	uvl	uvc, uva1, uvb1;
//--rotate_uvs-- 
//--rotate_uvs-- 	uvc.u = (uva->u + uvb->u)/2;
//--rotate_uvs-- 	uvc.v = (uva->v + uvb->v)/2;
//--rotate_uvs-- 
//--rotate_uvs-- 	uva1.u = fixmul(uva->u - uvc.u, rvec->x) - fixmul(uva->v - uvc.v, rvec->z);
//--rotate_uvs-- 	uva1.v = fixmul(uva->u - uvc.u, rvec->z) + fixmul(uva->v - uvc.v, rvec->x);
//--rotate_uvs-- 
//--rotate_uvs-- 	uva->u = uva1.u + uvc.u;
//--rotate_uvs-- 	uva->v = uva1.v + uvc.v;
//--rotate_uvs-- 
//--rotate_uvs-- 	uvb1.u = fixmul(uvb->u - uvc.u, rvec->x) - fixmul(uvb->v - uvc.v, rvec->z);
//--rotate_uvs-- 	uvb1.v = fixmul(uvb->u - uvc.u, rvec->z) + fixmul(uvb->v - uvc.v, rvec->x);
//--rotate_uvs-- 
//--rotate_uvs-- 	uvb->u = uvb1.u + uvc.u;
//--rotate_uvs-- 	uvb->v = uvb1.v + uvc.v;
//--rotate_uvs-- }

namespace {

// --------------------------------------------------------------------------------------------------------------
//	Assign u,v coordinates to con_seg, con_common_side from base_seg, base_common_side
//	They are connected at the edge defined by the vertices abs_id1, abs_id2.
static void med_assign_uvs_to_side(const vmsegptridx_t con_seg, const sidenum_t con_common_side, const cscusegment base_seg, const sidenum_t base_common_side, const vertnum_t abs_id1, const vertnum_t abs_id2)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	constexpr segment_relative_vertnum invalid_vertnum{0xff};
	segment_relative_vertnum cv1 = invalid_vertnum, cv2 = invalid_vertnum;
	segment_relative_vertnum bv1 = invalid_vertnum, bv2 = invalid_vertnum;

	// Find which vertices in segment match abs_id1, abs_id2
	for (const auto &&[v, b, c] : enumerate(zip(base_seg.s.verts, con_seg->verts), segment_relative_vertnum{}))
	{
		if (b == abs_id1)
			bv1 = v;
		if (b == abs_id2)
			bv2 = v;
		if (c == abs_id1)
			cv1 = v;
		if (c == abs_id2)
			cv2 = v;
	}

	//	Now, bv1, bv2 are segment relative vertices in base segment which are the same as absolute vertices abs_id1, abs_id2
	//	     cv1, cv2 are segment relative vertices in conn segment which are the same as absolute vertices abs_id1, abs_id2

	assert(bv1 != invalid_vertnum);
	assert(bv2 != invalid_vertnum);
	assert(cv1 != invalid_vertnum);
	assert(cv2 != invalid_vertnum);

	//	Now, scan 4 vertices in base side and 4 vertices in connected side.
	//	Set uv1, uv2 to uv coordinates from base side which correspond to vertices bv1, bv2.
	//	Set vv1, vv2 to relative vertex ids (in 0..3) in connecting side which correspond to cv1, cv2
	std::optional<side_relative_vertnum> vv1, vv2;
	auto &base_uvls = base_seg.u.sides[base_common_side].uvls;
	const uvl *uv1 = nullptr, *uv2 = nullptr;
	auto &svbase = Side_to_verts[base_common_side];
	auto &svconn = Side_to_verts[con_common_side];
	for (auto &&[v, base_uvl, svb, svc] : enumerate(zip(base_uvls, svbase, svconn)))
	{
		if (bv1 == svb)
			uv1 = &base_uvl;
		if (bv2 == svb)
			uv2 = &base_uvl;
		if (cv1 == svc)
			vv1 = v;
		if (cv2 == svc)
			vv2 = v;
	}

	assert(uv1->u != uv2->u || uv1->v != uv2->v);
	assert(vv1.has_value() && vv2.has_value());
	auto &vcvertptr = Vertices.vcptr;
	assign_uvs_to_side(vcvertptr, con_seg, con_common_side, *uv1, *uv2, *vv1, *vv2);
}


// -----------------------------------------------------------------------------
//	Given a base and a connecting segment, a side on each of those segments and two global vertex ids,
//	determine which side in each of the segments shares those two vertices.
//	This is used to propagate a texture map id to a connecting segment in an expected and desired way.
//	Since we can attach any side of a segment to any side of another segment, and do so in each case in
//	four different rotations (for a total of 6*6*4 = 144 ways), not having this nifty function will cause
//	great confusion.
static std::pair<sidenum_t, sidenum_t> get_side_ids(const shared_segment &base_seg, const shared_segment &con_seg, sidenum_t base_side, sidenum_t con_side, const vertnum_t abs_id1, const vertnum_t abs_id2)
{
	if (&base_seg == &con_seg)
		return {base_side, con_side};
	std::optional<sidenum_t> base_common_side;

	//	Find side in base segment which contains the two global vertex ids.
	for (const auto &&[idx, base_vp] : enumerate(Side_to_verts))
	{
		if (idx != base_side) {
			for (const auto v0 : MAX_VERTICES_PER_SIDE)
			{
				auto &verts = base_seg.verts;
				const auto candidate_abs_v0 = verts[base_vp[v0]];
				const auto candidate_abs_v1 = verts[base_vp[next_side_vertex(v0)]];
				if ((candidate_abs_v0 == abs_id1 && candidate_abs_v1 == abs_id2) ||
					(candidate_abs_v0 == abs_id2 && candidate_abs_v1 == abs_id1))
				{
					assert(!base_common_side);		// This means two different sides shared the same edge with base_side == impossible!
					base_common_side = idx;
				}
			}
		}
	}

	// Note: For connecting segment, process vertices in reversed order.
	std::optional<sidenum_t> con_common_side;

	//	Find side in connecting segment which contains the two global vertex ids.
	for (const auto &&[idx, con_vp] : enumerate(Side_to_verts))
	{
		if (idx != con_side) {
			for (const auto v0 : MAX_VERTICES_PER_SIDE)
			{
				auto &verts = con_seg.verts;
				const auto candidate_abs_v0 = verts[con_vp[next_side_vertex(v0)]];
				const auto candidate_abs_v1 = verts[con_vp[v0]];
				if ((candidate_abs_v0 == abs_id1 && candidate_abs_v1 == abs_id2) ||
					(candidate_abs_v0 == abs_id2 && candidate_abs_v1 == abs_id1))
				{
					assert(!con_common_side);		// This means two different sides shared the same edge with con_side == impossible!
					con_common_side = idx;
				}
			}
		}
	}
	return {*base_common_side, *con_common_side};
}

// -----------------------------------------------------------------------------
//	Propagate texture map u,v coordinates from base_seg:base_side to con_seg:con_side.
//	The two vertices abs_id1 and abs_id2 are the only two vertices common to the two sides.
//	If uv_only_flag is 1, then don't assign texture map ids, only update the uv coordinates
//	If uv_only_flag is -1, then ONLY assign texture map ids, don't update the uv coordinates
static void propagate_tmaps_to_segment_side(const vcsegptridx_t base_seg, const sidenum_t base_side, const vmsegptridx_t con_seg, const sidenum_t con_side, const vertnum_t abs_id1, const vertnum_t abs_id2, const int uv_only_flag)
{
	Assert ((uv_only_flag == -1) || (uv_only_flag == 0) || (uv_only_flag == 1));

	// Set base_common_side = side in base_seg which contains edge abs_id1:abs_id2
	// Set con_common_side = side in con_seg which contains edge abs_id1:abs_id2
	const auto [base_common_side, con_common_side] = get_side_ids(base_seg, con_seg, base_side, con_side, abs_id1, abs_id2);

	// Now, all faces in con_seg which are on side con_common_side get their tmap_num set to whatever tmap is assigned
	// to whatever face I find which is on side base_common_side.
	// First, find tmap_num for base_common_side.  If it doesn't exist (ie, there is a connection there), look at the segment
	// that is connected through it.
	if (!IS_CHILD(con_seg->children[con_common_side])) {
		if (!IS_CHILD(base_seg->children[base_common_side])) {
			// There is at least one face here, so get the tmap_num from there.

			// Now assign all faces in the connecting segment on side con_common_side to tmap_num.
			if ((uv_only_flag == -1) || (uv_only_flag == 0))
			{
				const auto tmap_num = base_seg->unique_segment::sides[base_common_side].tmap_num;
				con_seg->unique_segment::sides[con_common_side].tmap_num = tmap_num;
			}

			if (uv_only_flag != -1)
				med_assign_uvs_to_side(con_seg, static_cast<sidenum_t>(con_common_side), base_seg, static_cast<sidenum_t>(base_common_side), abs_id1, abs_id2);

		} else {			// There are no faces here, there is a connection, trace through the connection.
			const auto &&csegp = base_seg.absolute_sibling(base_seg->children[base_common_side]);
			auto cside = find_connect_side(base_seg, csegp);
			propagate_tmaps_to_segment_side(csegp, cside, con_seg, con_side, abs_id1, abs_id2, uv_only_flag);
		}
	}

}

}

// -----------------------------------------------------------------------------
//	Propagate texture map u,v coordinates to base_seg:back_side from base_seg:some-other-side
//	There is no easy way to figure out which side is adjacent to another side along some edge, so we do a bit of searching.
void med_propagate_tmaps_to_back_side(const vmsegptridx_t base_seg, const sidenum_t back_side, int uv_only_flag)
{
	if (IS_CHILD(base_seg->children[back_side]))
		return;		// connection, so no sides here.

	//	Scan all sides, look for an occupied side which is not back_side or Side_opposite[back_side]
	for (const auto &&[s, ebs] : enumerate(Two_sides_to_edge))
	{
		if ((s != back_side) && (s != Side_opposite[back_side])) {
			const auto v1 = ebs[back_side][0];
			const auto v2 = ebs[back_side][1];
			if (!base_seg->verts.valid_index(v1))
				// This means there was no shared edge between the two sides.
				return;
			if (!base_seg->verts.valid_index(v2))
				return;
			propagate_tmaps_to_segment_side(base_seg, s, base_seg, back_side, base_seg->verts[v1], base_seg->verts[v2], uv_only_flag);
			goto found1;
		}
	}
	return;		// Error -- couldn't find edge != back_side and Side_opposite[back_side]
found1: ;

	//	Assign an unused tmap id to the back side.
	//	Note that this can get undone by the caller if this was not part of a new attach, but a rotation or a scale (which
	//	both do attaches).
	//	First see if tmap on back side is anywhere else.
	if (!uv_only_flag) {
		const auto back_side_tmap = base_seg->unique_segment::sides[back_side].tmap_num;
		for (const auto &&[idx, value] : enumerate(base_seg->unique_segment::sides))
		{
			if (idx != back_side)
				if (value.tmap_num == back_side_tmap)
				{
					for (const auto tmap_num : MAX_SIDES_PER_SEGMENT)
					{
						for (const auto ss : MAX_SIDES_PER_SEGMENT)
							if (ss != back_side)
								if (base_seg->unique_segment::sides[ss].tmap_num == New_segment.unique_segment::sides[tmap_num].tmap_num)
									goto found2;		// current texture map (tmap_num) is used on current (ss) side, so try next one
						// Current texture map (tmap_num) has not been used, assign to all faces on back_side.
						base_seg->unique_segment::sides[back_side].tmap_num = New_segment.unique_segment::sides[tmap_num].tmap_num;
						goto done1;
					found2: ;
					}
				}
		}
	done1: ;
	}

}

int fix_bogus_uvs_on_side(void)
{
	med_propagate_tmaps_to_back_side(Cursegp, Curside, 1);
	return 0;
}

namespace {

static void fix_bogus_uvs_on_side1(const vmsegptridx_t sp, const sidenum_t sidenum, const int uvonly_flag)
{
	auto &uvls = sp->unique_segment::sides[sidenum].uvls;
	if (uvls[side_relative_vertnum::_0].u == 0 && uvls[side_relative_vertnum::_1].u == 0 && uvls[side_relative_vertnum::_2].u == 0)
	{
		med_propagate_tmaps_to_back_side(sp, static_cast<sidenum_t>(sidenum), uvonly_flag);
	}
}

static void fix_bogus_uvs_seg(const vmsegptridx_t segp)
{
	for (const auto &&[idx, value] : enumerate(segp->children))
	{
		if (!IS_CHILD(value))
			fix_bogus_uvs_on_side1(segp, static_cast<sidenum_t>(idx), 1);
	}
}

}

int fix_bogus_uvs_all(void)
{
	range_for (const auto &&segp, vmsegptridx)
	{
		if (segp->segnum != segment_none)
			fix_bogus_uvs_seg(segp);
	}
	return 0;
}

namespace {

// -----------------------------------------------------------------------------
//	Segment base_seg is connected through side base_side to segment con_seg on con_side.
//	For all walls in con_seg, find the wall in base_seg which shares an edge.  Copy tmap_num
//	from that side in base_seg to the wall in con_seg.  If the wall in base_seg is not present
//	(ie, there is another segment connected through it), follow the connection through that
//	segment to get the wall in the connected segment which shares the edge, and get tmap_num from there.
static void propagate_tmaps_to_segment_sides(const vcsegptridx_t base_seg, const sidenum_t base_side, const vmsegptridx_t con_seg, const sidenum_t con_side, const int uv_only_flag)
{
	auto &base_vp = Side_to_verts[base_side];

	// Do for each edge on connecting face.
	for (const auto v : MAX_VERTICES_PER_SIDE)
	{
		const auto abs_id1 = base_seg->verts[base_vp[v]];
		const auto abs_id2 = base_seg->verts[base_vp[next_side_vertex(v)]];
		propagate_tmaps_to_segment_side(base_seg, base_side, con_seg, con_side, abs_id1, abs_id2, uv_only_flag);
	}
}

}

// -----------------------------------------------------------------------------
//	Propagate texture maps in base_seg to con_seg.
//	For each wall in con_seg, find the wall in base_seg which shared an edge.  Copy tmap_num from that
//	wall in base_seg to the wall in con_seg.  If the wall in base_seg is not present, then look at the
//	segment connected through base_seg through the wall.  The wall with a common edge is the new wall
//	of interest.  Continue searching in this way until a wall of interest is present.
void med_propagate_tmaps_to_segments(const vcsegptridx_t base_seg, const vmsegptridx_t con_seg, const int uv_only_flag)
{
	for (const auto &&[idx, value] : enumerate(base_seg->children))
		if (value == con_seg)
			propagate_tmaps_to_segment_sides(base_seg, static_cast<sidenum_t>(idx), con_seg, find_connect_side(base_seg, con_seg), uv_only_flag);

	const unique_segment &ubase = base_seg;
	unique_segment &ucon = con_seg;
	ucon.static_light = ubase.static_light;

	validate_uv_coordinates(con_seg);
}


// -------------------------------------------------------------------------------
//	Copy texture map uvs from srcseg to destseg.
//	If two segments have different face structure (eg, destseg has two faces on side 3, srcseg has only 1)
//	then assign uvs according to side vertex id, not face vertex id.
void copy_uvs_seg_to_seg(unique_segment &destseg, const unique_segment &srcseg)
{
	range_for (const auto &&z, zip(destseg.sides, srcseg.sides))
	{
		auto &ds = std::get<0>(z);
		auto &ss = std::get<1>(z);
		ds.tmap_num = ss.tmap_num;
		ds.tmap_num2 = ss.tmap_num2;
	}

	destseg.static_light = srcseg.static_light;
}

}

namespace dcx {

//	_________________________________________________________________________________________________________________________
//	Maximum distance between a segment containing light to a segment to receive light.
#define	LIGHT_DISTANCE_THRESHOLD	(F1_0*80)
fix	Magical_light_constant = (F1_0*16);

// int	Seg0, Seg1;

//int	Bugseg = 27;

namespace {

struct hash_info {
	sbyte flag;
	fvi_hit_type hit_type;
	vms_vector	vector;
};

#define	FVI_HASH_SIZE 8
#define	FVI_HASH_AND_MASK (FVI_HASH_SIZE - 1)

//	Note: This should be malloced.
//			Also, the vector should not be 12 bytes, you should only care about some smaller portion of it.
static std::array<hash_info, FVI_HASH_SIZE> fvi_cache;
static int Hash_hits=0, Hash_retries=0, Hash_calcs=0;

//	-----------------------------------------------------------------------------------------
//	Set light from a light source.
//	Light incident on a surface is defined by the light incident at its points.
//	Light at a point = K * (V . N) / d
//	where:
//		K = some magical constant to make everything look good
//		V = normalized vector from light source to point
//		N = surface normal at point
//		d = distance from light source to point
//	(Note that the above equation can be simplified to K * (VV . N) / d^2 where VV = non-normalized V)
//	Light intensity emitted from a light source is defined to be cast from four points.
//	These four points are 1/64 of the way from the corners of the light source to the center
//	of its segment.  By assuming light is cast from these points, rather than from on the
//	light surface itself, light will be properly cast on the light surface.  Otherwise, the
//	vector V would be the null vector.
//	If quick_light set, then don't use find_vector_intersection
static void cast_light_from_side(const vmsegptridx_t segp, const sidenum_t light_side, fix light_intensity, int quick_light)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vcvertptr = Vertices.vcptr;
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcwallptr = Walls.vcptr;
	const auto segment_center = compute_segment_center(vcvertptr, segp);
	//	Do for four lights, one just inside each corner of side containing light.
	range_for (const auto lightnum, Side_to_verts[light_side])
	{
		// fix			inverse_segment_magnitude;

		const auto light_vertex_num = segp->verts[lightnum];
		auto light_location = *vcvertptr(light_vertex_num);

	//	New way, 5/8/95: Move towards center irrespective of size of segment.
		const auto vector_to_center = vm_vec_normalized_quick(vm_vec_sub(segment_center, light_location));
	vm_vec_add2(light_location, vector_to_center);

// -- Old way, before 5/8/95 --		// -- This way was kind of dumb.  In larger segments, you move LESS towards the center.
// -- Old way, before 5/8/95 --		//    Main problem, though, is vertices don't illuminate themselves well in oblong segments because the dot product is small.
// -- Old way, before 5/8/95 --		vm_vec_sub(&vector_to_center, &segment_center, &light_location);
// -- Old way, before 5/8/95 --		inverse_segment_magnitude = fixdiv(F1_0/5, vm_vec_mag(&vector_to_center));
// -- Old way, before 5/8/95 --		vm_vec_scale_add(&light_location, &light_location, &vector_to_center, inverse_segment_magnitude);

		range_for (const auto &&rsegp, vmsegptr)
		{
			fix			dist_to_rseg;

			range_for (auto &i, fvi_cache)
				i.flag = 0;

			//	efficiency hack (I hope!), for faraway segments, don't check each point.
			const auto r_segment_center = compute_segment_center(vcvertptr, rsegp);
			dist_to_rseg = vm_vec_dist_quick(r_segment_center, segment_center);

			if (dist_to_rseg <= LIGHT_DISTANCE_THRESHOLD) {
				for (const auto &&[sidenum, srside, urside] : enumerate(zip(rsegp->shared_segment::sides, rsegp->unique_segment::sides)))
				{
					if (WALL_IS_DOORWAY(GameBitmaps, Textures, vcwallptr, rsegp, static_cast<sidenum_t>(sidenum)) != WID_NO_WALL)
					{
						auto &side_normalp = srside.normals[0];	//	kinda stupid? always use vector 0.

						auto &sv_side = Side_to_verts[static_cast<sidenum_t>(sidenum)];
						for (const auto vertnum : MAX_VERTICES_PER_SIDE)
						{
							const auto segment_relative_vert = sv_side[vertnum];
							const auto abs_vertnum = rsegp->verts[segment_relative_vert];
							vms_vector vert_location = *vcvertptr(abs_vertnum);
							const fix distance_to_point = vm_vec_dist_quick(vert_location, light_location);
							const auto vector_to_light = vm_vec_normalized(vm_vec_sub(light_location, vert_location));

							//	Hack: In oblong segments, it's possible to get a very small dot product
							//	but the light source is very nearby (eg, illuminating light itself!).
							fix light_dot = vm_vec_dot(vector_to_light, side_normalp);
							if (distance_to_point < F1_0)
								if (light_dot > 0)
									light_dot = (light_dot + F1_0)/2;

							if (light_dot > 0) {
								fix light_at_point = fixdiv(fixmul(light_dot, light_dot), distance_to_point);
								light_at_point = fixmul(light_at_point, Magical_light_constant);
								if (light_at_point >= 0) {
									fvi_info	hit_data;
									fvi_hit_type hit_type;

									const auto r_vector_to_center = vm_vec_sub(r_segment_center, vert_location);
									const auto inverse_segment_magnitude = fixdiv(F1_0/3, vm_vec_mag(r_vector_to_center));
									const auto vert_location_1 = vm_vec_scale_add(vert_location, r_vector_to_center, inverse_segment_magnitude);
									vert_location = vert_location_1;

									if (!quick_light) {
										auto hash_value = underlying_value(segment_relative_vert);
										hash_info	*hashp = &fvi_cache[hash_value];
										while (1) {
											if (hashp->flag) {
												if ((hashp->vector.x == vector_to_light.x) && (hashp->vector.y == vector_to_light.y) && (hashp->vector.z == vector_to_light.z)) {
													hit_type = hashp->hit_type;
													Hash_hits++;
													break;
												} else {
													Int3();	// How is this possible?  Should be no hits!
													Hash_retries++;
													hash_value = (hash_value+1) & FVI_HASH_AND_MASK;
													hashp = &fvi_cache[hash_value];
												}
											} else {
												Hash_calcs++;
												hashp->vector = vector_to_light;
												hashp->flag = 1;

												hit_type = find_vector_intersection(fvi_query{
													light_location,
													vert_location,
													fvi_query::unused_ignore_obj_list,
													0,
													object_none,
												}, segp, 0, hit_data);
												hashp->hit_type = hit_type;
												break;
											}
										}
									} else
										hit_type = fvi_hit_type::None;
									switch (hit_type) {
										case fvi_hit_type::None:
											light_at_point = fixmul(light_at_point, light_intensity);
											urside.uvls[vertnum].l += light_at_point;
											if (urside.uvls[vertnum].l > F1_0)
												urside.uvls[vertnum].l = F1_0;
											break;
										case fvi_hit_type::Wall:
											break;
										case fvi_hit_type::Object:
											Int3();	// Hit object, should be ignoring objects!
											break;
										case fvi_hit_type::BadP0:
											Int3();	//	Ugh, this thing again, what happened, what does it mean?
											break;
									}
								}	//	end if (light_at_point...
							}	// end if (light_dot >...
						}	//	end for (vertnum=0...
					}	//	end if (rsegp...
				}	//	end for (sidenum=0...
			}	//	end if (dist_to_rseg...

		}	//	end for (segnum=0...

	}	//	end for (lightnum=0...
}


//	------------------------------------------------------------------------------------------
//	Zero all lighting values.
static void calim_zero_light_values(void)
{
	range_for (unique_segment &segp, vmsegptr)
	{
		range_for (auto &side, segp.sides)
		{
			range_for (auto &uvl, side.uvls)
				uvl.l = F1_0/64;	// Put a tiny bit of light here.
		}
		segp.static_light = F1_0 / 64;
	}
}


//	------------------------------------------------------------------------------------------
//	Used in setting average light value in a segment, cast light from a side to the center
//	of all segments.
static void cast_light_from_side_to_center(const vmsegptridx_t segp, const sidenum_t light_side, fix light_intensity, int quick_light)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vcvertptr = Vertices.vcptr;
	const auto &&segment_center = compute_segment_center(vcvertptr, segp);
	//	Do for four lights, one just inside each corner of side containing light.
	range_for (const auto lightnum, Side_to_verts[light_side])
	{
		const auto light_vertex_num = segp->verts[lightnum];
		auto &vert_light_location = *vcvertptr(light_vertex_num);
		const auto vector_to_center = vm_vec_sub(segment_center, vert_light_location);
		const auto light_location = vm_vec_scale_add(vert_light_location, vector_to_center, F1_0/64);

		for (const csmusegment &&rsegp : vmsegptr)
		{
			fix			dist_to_rseg;
//if ((segp == &Segments[Bugseg]) && (rsegp == &Segments[Bugseg]))
//	Int3();
			const auto r_segment_center = compute_segment_center(vcvertptr, rsegp);
			dist_to_rseg = vm_vec_dist_quick(r_segment_center, segment_center);

			if (dist_to_rseg <= LIGHT_DISTANCE_THRESHOLD) {
				fix	light_at_point;
				if (dist_to_rseg > F1_0)
					light_at_point = fixdiv(Magical_light_constant, dist_to_rseg);
				else
					light_at_point = Magical_light_constant;

				if (light_at_point >= 0) {
					fvi_hit_type hit_type;

					if (!quick_light) {
						fvi_info	hit_data;

						hit_type = find_vector_intersection(fvi_query{
							light_location,
							r_segment_center,
							fvi_query::unused_ignore_obj_list,
							0,
							object_none,
						}, segp, 0, hit_data);
					}
					else
						hit_type = fvi_hit_type::None;

					switch (hit_type) {
						case fvi_hit_type::None:
							light_at_point = fixmul(light_at_point, light_intensity);
							if (light_at_point >= F1_0)
								light_at_point = F1_0-1;
							{
								auto &static_light = rsegp.u.static_light;
								static_light += light_at_point;
								if (static_light < 0)	// if it went negative, saturate
									static_light = 0;
							}
							break;
						case fvi_hit_type::Wall:
							break;
						case fvi_hit_type::Object:
							Int3();	// Hit object, should be ignoring objects!
							break;
						case fvi_hit_type::BadP0:
							Int3();	//	Ugh, this thing again, what happened, what does it mean?
							break;
					}
				}	//	end if (light_at_point...
			}	//	end if (dist_to_rseg...

		}	//	end for (segnum=0...

	}	//	end for (lightnum=0...

}

//	------------------------------------------------------------------------------------------
//	Process all lights.
static void calim_process_all_lights(int quick_light)
{
	auto &TmapInfo = LevelUniqueTmapInfoState.TmapInfo;
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcwallptr = Walls.vcptr;
	range_for (const auto &&segp, vmsegptridx)
	{
		for (const auto &&[sidenum, value] : enumerate(segp->unique_segment::sides))
		{
			if (WALL_IS_DOORWAY(GameBitmaps, Textures, vcwallptr, segp, static_cast<sidenum_t>(sidenum)) != WID_NO_WALL)
			{
				const auto sidep = &value;
				fix	light_intensity;

				light_intensity = TmapInfo[get_texture_index(sidep->tmap_num)].lighting + TmapInfo[get_texture_index(sidep->tmap_num2)].lighting;

//				if (segp->sides[sidenum].wall_num != -1) {
//					int	wall_num, bitmap_num, effect_num;
//					wall_num = segp->sides[sidenum].wall_num;
//					effect_num = Walls[wall_num].type;
//					bitmap_num = effects_bm_num[effect_num];
//
//					light_intensity += TmapInfo[bitmap_num].lighting;
//				}

				if (light_intensity) {
					light_intensity /= 4;			// casting light from four spots, so divide by 4.
					cast_light_from_side(segp, static_cast<sidenum_t>(sidenum), light_intensity, quick_light);
					cast_light_from_side_to_center(segp, static_cast<sidenum_t>(sidenum), light_intensity, quick_light);
				}
			}
		}
	}
}

//	------------------------------------------------------------------------------------------
//	Apply static light in mine.
//	First, zero all light values.
//	Then, for all light sources, cast their light.
static void cast_all_light_in_mine(int quick_flag)
{
	validate_segment_all(LevelSharedSegmentState);
	calim_zero_light_values();

	calim_process_all_lights(quick_flag);
}

}

}

// int	Fvit_num = 1000;
// 
// fix find_vector_intersection_test(void)
// {
// 	int		i;
// 	fvi_info	hit_data;
// 	int		p0_seg, p1_seg, this_objnum, ignore_obj, check_obj_flag;
// 	fix		rad;
// 	int		start_time = timer_get_milliseconds();;
// 	vms_vector	p0,p1;
// 
// 	ignore_obj = 1;
// 	check_obj_flag = 0;
// 	this_objnum = -1;
// 	rad = F1_0/4;
// 
// 	for (i=0; i<Fvit_num; i++) {
//		p0_seg = d_rand()*(Highest_segment_index+1)/32768;
// 		compute_segment_center(&p0, &Segments[p0_seg]);
// 
//		p1_seg = d_rand()*(Highest_segment_index+1)/32768;
// 		compute_segment_center(&p1, &Segments[p1_seg]);
// 
// 		find_vector_intersection(&hit_data, &p0, p0_seg, &p1, rad, this_objnum, ignore_obj, check_obj_flag);
// 	}
// 
// 	return timer_get_milliseconds() - start_time;
// }
