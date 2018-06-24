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
#include	"effects.h"     //      Needed for effects_bm_num
#include "fvi.h"
#include "seguvs.h"

#include "compiler-range_for.h"

namespace dcx {
static void cast_all_light_in_mine(int quick_flag);
}
//--rotate_uvs-- vms_vector Rightvec;

#define	MAX_LIGHT_SEGS 16

//	---------------------------------------------------------------------------------------------
//	Scan all polys in all segments, return average light value for vnum.
//	segs = output array for segments containing vertex, terminated by -1.
static fix get_average_light_at_vertex(int vnum, segnum_t *segs)
{
	int	sidenum;
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
		auto relvnum = std::distance(std::find(begin(segp->verts), e, vnum), e);
		if (relvnum < MAX_VERTICES_PER_SEGMENT) {

			*segs++ = segp;
			Assert(segs - original_segs < MAX_LIGHT_SEGS);
			(void)original_segs;

			for (sidenum=0; sidenum < MAX_SIDES_PER_SEGMENT; sidenum++) {
				if (!IS_CHILD(segp->children[sidenum])) {
					const auto sidep = &segp->sides[sidenum];
					auto &vp = Side_to_verts[sidenum];
					const auto vb = begin(vp);
					const auto ve = end(vp);
					const auto vi = std::find(vb, ve, relvnum);
					if (vi != ve)
					{
						const auto v = std::distance(vb, vi);
							total_light += sidep->uvls[v].l;
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

static void set_average_light_at_vertex(int vnum)
{
	int	relvnum, sidenum;
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
		auto &usegp = *vmsegptr(segnum);

		for (relvnum=0; relvnum<MAX_VERTICES_PER_SEGMENT; relvnum++)
			if (ssegp.verts[relvnum] == vnum)
				break;

		if (relvnum < MAX_VERTICES_PER_SEGMENT) {
			for (sidenum=0; sidenum < MAX_SIDES_PER_SEGMENT; sidenum++) {
				if (!IS_CHILD(ssegp.children[sidenum])) {
					unique_side &sidep = usegp.sides[sidenum];
					auto &vp = Side_to_verts[sidenum];
					const auto vb = begin(vp);
					const auto ve = end(vp);
					const auto vi = std::find(vb, ve, relvnum);
					if (vi != ve)
					{
						const auto v = std::distance(vb, vi);
						sidep.uvls[v].l = average_light;
					}
				}	// end if
			}	// end sidenum
		}	// end if
	}	// end while

	Update_flags |= UF_WORLD_CHANGED;
}

static void set_average_light_on_side(const vmsegptr_t segp, int sidenum)
{
	if (!IS_CHILD(segp->children[sidenum]))
		range_for (const auto v, Side_to_verts[sidenum])
		{
			set_average_light_at_vertex(segp->verts[v]);
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

//	---------------------------------------------------------------------------------------------
//	Given a polygon, compress the uv coordinates so that they are as close to 0 as possible.
//	Do this by adding a constant u and v to each uv pair.
static void compress_uv_coordinates(array<uvl, 4> &uvls)
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

static void assign_default_lighting_on_side(array<uvl, 4> &uvls)
{
	range_for (auto &uvl, uvls)
		uvl.l = DEFAULT_LIGHTING;
}

static void assign_default_lighting(const vmsegptr_t segp)
{
	range_for (auto &side, segp->sides)
		assign_default_lighting_on_side(side.uvls);
}

void assign_default_lighting_all(void)
{
	range_for (const auto &&segp, vmsegptr)
	{
		if (segp->segnum != segment_none)
			assign_default_lighting(segp);
	}
}

//	---------------------------------------------------------------------------------------------
static void validate_uv_coordinates(const vmsegptr_t segp)
{
	range_for (auto &side, segp->sides)
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

//	---------------------------------------------------------------------------------------------
//	Assign lighting value to side, a function of the normal vector.
void assign_light_to_side(unique_side &s)
{
	range_for (auto &v, s.uvls)
		v.l = DEFAULT_LIGHTING;
}

fix	Stretch_scale_x = F1_0;
fix	Stretch_scale_y = F1_0;

//	---------------------------------------------------------------------------------------------
//	Given u,v coordinates at two vertices, assign u,v coordinates to other two vertices on a side.
//	(Actually, assign them to the coordinates in the faces.)
//	va, vb = face-relative vertex indices corresponding to uva, uvb.  Ie, they are always in 0..3 and should be looked up in
//	Side_to_verts[side] to get the segment relative index.
static void assign_uvs_to_side(const vmsegptridx_t segp, int sidenum, uvl *uva, uvl *uvb, int va, int vb)
{
	int			vlo,vhi;
	unsigned v0, v1, v2, v3;
	array<uvl, 4> uvls;
	uvl ruvmag,fuvmag,uvlo,uvhi;
	fix			fmag,mag01;
	Assert( (va<4) && (vb<4) );
	Assert((abs(va - vb) == 1) || (abs(va - vb) == 3));		// make sure the verticies specify an edge

	auto &vp = Side_to_verts[sidenum];

	// We want vlo precedes vhi, ie vlo < vhi, or vlo = 3, vhi = 0
	if (va == ((vb + 1) % 4)) {		// va = vb + 1
		vlo = vb;
		vhi = va;
		uvlo = *uvb;
		uvhi = *uva;
	} else {
		vlo = va;
		vhi = vb;
		uvlo = *uva;
		uvhi = *uvb;
	}

	Assert(((vlo+1) % 4) == vhi);	// If we are on an edge, then uvhi is one more than uvlo (mod 4)
	uvls[vlo] = uvlo;
	uvls[vhi] = uvhi;

	// Now we have vlo precedes vhi, compute vertices ((vhi+1) % 4) and ((vhi+2) % 4)

	// Assign u,v scale to a unit length right vector.
	fmag = zhypot(uvhi.v - uvlo.v,uvhi.u - uvlo.u);
	if (fmag < 64) {		// this is a fix, so 64 = 1/1024
		ruvmag.u = F1_0*256;
		ruvmag.v = F1_0*256;
		fuvmag.u = F1_0*256;
		fuvmag.v = F1_0*256;
	} else {
		ruvmag.u = uvhi.v - uvlo.v;
		ruvmag.v = uvlo.u - uvhi.u;

		fuvmag.u = uvhi.u - uvlo.u;
		fuvmag.v = uvhi.v - uvlo.v;
	}

	v0 = segp->verts[vp[vlo]];
	v1 = segp->verts[vp[vhi]];
	v2 = segp->verts[vp[(vhi+1)%4]];
	v3 = segp->verts[vp[(vhi+2)%4]];

	//	Compute right vector by computing orientation matrix from:
	//		forward vector = vlo:vhi
	//		  right vector = vlo:(vhi+2) % 4
	const auto &&vp0 = vcvertptr(v0);
	const auto &vv1v0 = vm_vec_sub(vcvertptr(v1), vp0);
	mag01 = vm_vec_mag(vv1v0);
	mag01 = fixmul(mag01, (va == 0 || va == 2) ? Stretch_scale_x : Stretch_scale_y);

	if (unlikely(mag01 < F1_0/1024))
		editor_status_fmt("U, V bogosity in segment #%hu, probably on side #%i.  CLEAN UP YOUR MESS!", static_cast<uint16_t>(segp), sidenum);
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
		const auto assign_uvl = [&](const vms_vector &tvec, const uvl &uvi) {
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
		uvls[(vhi+1)%4] = assign_uvl(vm_vec_sub(vcvertptr(v2), vcvertptr(v1)), uvhi);
		uvls[(vhi+2)%4] = assign_uvl(vv3v0, uvlo);
		//	For all faces in side, copy uv coordinates from uvs array to face.
		segp->sides[sidenum].uvls = uvls;
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
void assign_default_uvs_to_side(const vmsegptridx_t segp, const unsigned side)
{
	uvl			uv0,uv1;
	uv0.u = 0;
	uv0.v = 0;
	auto &vp = Side_to_verts[side];
	uv1.u = 0;
	uv1.v = Num_tilings * fixmul(Vmag, vm_vec_dist(vcvertptr(segp->verts[vp[1]]), vcvertptr(segp->verts[vp[0]])));

	assign_uvs_to_side(segp, side, &uv0, &uv1, 0, 1);
}

// -----------------------------------------------------------------------------------------------------------
//	Assign default uvs to side.
//	This means:
//		v0 = 0,0
//		v1 = k,0 where k is 3d size dependent
//	v2, v3 assigned by assign_uvs_to_side
void stretch_uvs_from_curedge(const vmsegptridx_t segp, int side)
{
	uvl			uv0,uv1;
	int			v0, v1;

	v0 = Curedge;
	v1 = (v0 + 1) % 4;

	uv0.u = segp->sides[side].uvls[v0].u;
	uv0.v = segp->sides[side].uvls[v0].v;

	uv1.u = segp->sides[side].uvls[v1].u;
	uv1.v = segp->sides[side].uvls[v1].v;

	assign_uvs_to_side(segp, side, &uv0, &uv1, v0, v1);
}

// --------------------------------------------------------------------------------------------------------------
//	Assign default uvs to a segment.
void assign_default_uvs_to_segment(const vmsegptridx_t segp)
{
	int	s;

	for (s=0; s<MAX_SIDES_PER_SEGMENT; s++) {
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


// --------------------------------------------------------------------------------------------------------------
void med_assign_uvs_to_side(const vmsegptridx_t con_seg, int con_common_side, const vmsegptr_t base_seg, int base_common_side, int abs_id1, int abs_id2)
{
	uvl		uv1,uv2;
        int             v,bv1,bv2, vv1, vv2;
        int             cv1=0, cv2=0;

	bv1 = -1;	bv2 = -1;

	// Find which vertices in segment match abs_id1, abs_id2
	for (v=0; v<MAX_VERTICES_PER_SEGMENT; v++) {
		if (base_seg->verts[v] == abs_id1)
			bv1 = v;
		if (base_seg->verts[v] == abs_id2)
			bv2 = v;
		if (con_seg->verts[v] == abs_id1)
			cv1 = v;
		if (con_seg->verts[v] == abs_id2)
			cv2 = v;
	}

	//	Now, bv1, bv2 are segment relative vertices in base segment which are the same as absolute vertices abs_id1, abs_id2
	//	     cv1, cv2 are segment relative vertices in conn segment which are the same as absolute vertices abs_id1, abs_id2

	Assert((bv1 != -1) && (bv2 != -1) && (cv1 != -1) && (cv2 != -1));

	//	Now, scan 4 vertices in base side and 4 vertices in connected side.
	//	Set uv1, uv2 to uv coordinates from base side which correspond to vertices bv1, bv2.
	//	Set vv1, vv2 to relative vertex ids (in 0..3) in connecting side which correspond to cv1, cv2
	vv1 = -1;	vv2 = -1;
	for (v=0; v<4; v++) {
		if (bv1 == Side_to_verts[base_common_side][v])
			uv1 = base_seg->sides[base_common_side].uvls[v];

		if (bv2 == Side_to_verts[base_common_side][v])
			uv2 = base_seg->sides[base_common_side].uvls[v];

		if (cv1 == Side_to_verts[con_common_side][v])
			vv1 = v;

		if (cv2 == Side_to_verts[con_common_side][v])
			vv2 = v;
	}

	Assert((uv1.u != uv2.u) || (uv1.v != uv2.v));
	Assert( (vv1 != -1) && (vv2 != -1) );
	assign_uvs_to_side(con_seg, con_common_side, &uv1, &uv2, vv1, vv2);
}


// -----------------------------------------------------------------------------
//	Given a base and a connecting segment, a side on each of those segments and two global vertex ids,
//	determine which side in each of the segments shares those two vertices.
//	This is used to propagate a texture map id to a connecting segment in an expected and desired way.
//	Since we can attach any side of a segment to any side of another segment, and do so in each case in
//	four different rotations (for a total of 6*6*4 = 144 ways), not having this nifty function will cause
//	great confusion.
static void get_side_ids(const vmsegptr_t base_seg, const vmsegptr_t con_seg, int base_side, int con_side, int abs_id1, int abs_id2, int *base_common_side, int *con_common_side)
{
	int		v0,side;

	*base_common_side = -1;

	//	Find side in base segment which contains the two global vertex ids.
	for (side=0; side<MAX_SIDES_PER_SEGMENT; side++) {
		if (side != base_side) {
			auto &base_vp = Side_to_verts[side];
			for (v0=0; v0<4; v0++)
                                if (((base_seg->verts[static_cast<int>(base_vp[v0])] == abs_id1) && (base_seg->verts[static_cast<int>(base_vp[(v0+1) % 4])] == abs_id2)) || ((base_seg->verts[static_cast<int>(base_vp[v0])] == abs_id2) && (base_seg->verts[static_cast<int>(base_vp[ (v0+1) % 4])] == abs_id1))) {
					Assert(*base_common_side == -1);		// This means two different sides shared the same edge with base_side == impossible!
					*base_common_side = side;
				}
		}
	}

	// Note: For connecting segment, process vertices in reversed order.
	*con_common_side = -1;

	//	Find side in connecting segment which contains the two global vertex ids.
	for (side=0; side<MAX_SIDES_PER_SEGMENT; side++) {
		if (side != con_side) {
			auto &con_vp = Side_to_verts[side];
			for (v0=0; v0<4; v0++)
                                if (((con_seg->verts[static_cast<int>(con_vp[(v0 + 1) % 4])] == abs_id1) && (con_seg->verts[static_cast<int>(con_vp[v0])] == abs_id2)) || ((con_seg->verts[static_cast<int>(con_vp[(v0 + 1) % 4])] == abs_id2) && (con_seg->verts[static_cast<int>(con_vp[v0])] == abs_id1))) {
					Assert(*con_common_side == -1);		// This means two different sides shared the same edge with con_side == impossible!
					*con_common_side = side;
				}
		}
	}

	Assert((*base_common_side != -1) && (*con_common_side != -1));
}

// -----------------------------------------------------------------------------
//	Propagate texture map u,v coordinates from base_seg:base_side to con_seg:con_side.
//	The two vertices abs_id1 and abs_id2 are the only two vertices common to the two sides.
//	If uv_only_flag is 1, then don't assign texture map ids, only update the uv coordinates
//	If uv_only_flag is -1, then ONLY assign texture map ids, don't update the uv coordinates
static void propagate_tmaps_to_segment_side(const vmsegptridx_t base_seg, int base_side, const vmsegptridx_t con_seg, int con_side, int abs_id1, int abs_id2, int uv_only_flag)
{
	int		base_common_side,con_common_side;
	int		tmap_num;

	Assert ((uv_only_flag == -1) || (uv_only_flag == 0) || (uv_only_flag == 1));

	// Set base_common_side = side in base_seg which contains edge abs_id1:abs_id2
	// Set con_common_side = side in con_seg which contains edge abs_id1:abs_id2
	if (base_seg != con_seg)
		get_side_ids(base_seg, con_seg, base_side, con_side, abs_id1, abs_id2, &base_common_side, &con_common_side);
	else {
		base_common_side = base_side;
		con_common_side = con_side;
	}

	// Now, all faces in con_seg which are on side con_common_side get their tmap_num set to whatever tmap is assigned
	// to whatever face I find which is on side base_common_side.
	// First, find tmap_num for base_common_side.  If it doesn't exist (ie, there is a connection there), look at the segment
	// that is connected through it.
	if (!IS_CHILD(con_seg->children[con_common_side])) {
		if (!IS_CHILD(base_seg->children[base_common_side])) {
			// There is at least one face here, so get the tmap_num from there.
			tmap_num = base_seg->sides[base_common_side].tmap_num;

			// Now assign all faces in the connecting segment on side con_common_side to tmap_num.
			if ((uv_only_flag == -1) || (uv_only_flag == 0))
				con_seg->sides[con_common_side].tmap_num = tmap_num;

			if (uv_only_flag != -1)
				med_assign_uvs_to_side(con_seg, con_common_side, base_seg, base_common_side, abs_id1, abs_id2);

		} else {			// There are no faces here, there is a connection, trace through the connection.
			const auto &&csegp = base_seg.absolute_sibling(base_seg->children[base_common_side]);
			auto cside = find_connect_side(base_seg, csegp);
			propagate_tmaps_to_segment_side(csegp, cside, con_seg, con_side, abs_id1, abs_id2, uv_only_flag);
		}
	}

}

}

namespace dcx {

constexpr int8_t Edge_between_sides[MAX_SIDES_PER_SEGMENT][MAX_SIDES_PER_SEGMENT][2] = {
//		left		top		right		bottom	back		front
	{ {-1,-1}, { 3, 7}, {-1,-1}, { 2, 6}, { 6, 7}, { 2, 3} },	// left
	{ { 3, 7}, {-1,-1}, { 0, 4}, {-1,-1}, { 4, 7}, { 0, 3} },	// top
	{ {-1,-1}, { 0, 4}, {-1,-1}, { 1, 5}, { 4, 5}, { 0, 1} },	// right
	{ { 2, 6}, {-1,-1}, { 1, 5}, {-1,-1}, { 5, 6}, { 1, 2} },	// bottom
	{ { 6, 7}, { 4, 7}, { 4, 5}, { 5, 6}, {-1,-1}, {-1,-1} },	// back
	{ { 2, 3}, { 0, 3}, { 0, 1}, { 1, 2}, {-1,-1}, {-1,-1} }};	// front

}

namespace dsx {

// -----------------------------------------------------------------------------
//	Propagate texture map u,v coordinates to base_seg:back_side from base_seg:some-other-side
//	There is no easy way to figure out which side is adjacent to another side along some edge, so we do a bit of searching.
void med_propagate_tmaps_to_back_side(const vmsegptridx_t base_seg, int back_side, int uv_only_flag)
{
        int     v1=0,v2=0;
	int	s,ss,tmap_num,back_side_tmap;

	if (IS_CHILD(base_seg->children[back_side]))
		return;		// connection, so no sides here.

	//	Scan all sides, look for an occupied side which is not back_side or Side_opposite[back_side]
	for (s=0; s<MAX_SIDES_PER_SEGMENT; s++)
		if ((s != back_side) && (s != Side_opposite[back_side])) {
			v1 = Edge_between_sides[s][back_side][0];
			v2 = Edge_between_sides[s][back_side][1];
			goto found1;
		}
	Assert(0);		// Error -- couldn't find edge != back_side and Side_opposite[back_side]
found1: ;
	Assert( (v1 != -1) && (v2 != -1));		// This means there was no shared edge between the two sides.

	propagate_tmaps_to_segment_side(base_seg, s, base_seg, back_side, base_seg->verts[v1], base_seg->verts[v2], uv_only_flag);

	//	Assign an unused tmap id to the back side.
	//	Note that this can get undone by the caller if this was not part of a new attach, but a rotation or a scale (which
	//	both do attaches).
	//	First see if tmap on back side is anywhere else.
	if (!uv_only_flag) {
		back_side_tmap = base_seg->sides[back_side].tmap_num;
		for (s=0; s<MAX_SIDES_PER_SEGMENT; s++) {
			if (s != back_side)
				if (base_seg->sides[s].tmap_num == back_side_tmap) {
					for (tmap_num=0; tmap_num < MAX_SIDES_PER_SEGMENT; tmap_num++) {
						for (ss=0; ss<MAX_SIDES_PER_SEGMENT; ss++)
							if (ss != back_side)
								if (base_seg->sides[ss].tmap_num == New_segment.sides[tmap_num].tmap_num)
									goto found2;		// current texture map (tmap_num) is used on current (ss) side, so try next one
						// Current texture map (tmap_num) has not been used, assign to all faces on back_side.
						base_seg->sides[back_side].tmap_num = New_segment.sides[tmap_num].tmap_num;
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

static void fix_bogus_uvs_on_side1(const vmsegptridx_t sp, const unsigned sidenum, const int uvonly_flag)
{
	auto &uvls = sp->sides[sidenum].uvls;
	if (uvls[0].u == 0 && uvls[1].u == 0 && uvls[2].u == 0)
	{
		med_propagate_tmaps_to_back_side(sp, sidenum, uvonly_flag);
	}
}

static void fix_bogus_uvs_seg(const vmsegptridx_t segp)
{
	int	s;

	for (s=0; s<MAX_SIDES_PER_SEGMENT; s++) {
		if (!IS_CHILD(segp->children[s]))
			fix_bogus_uvs_on_side1(segp, s, 1);
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

// -----------------------------------------------------------------------------
//	Segment base_seg is connected through side base_side to segment con_seg on con_side.
//	For all walls in con_seg, find the wall in base_seg which shares an edge.  Copy tmap_num
//	from that side in base_seg to the wall in con_seg.  If the wall in base_seg is not present
//	(ie, there is another segment connected through it), follow the connection through that
//	segment to get the wall in the connected segment which shares the edge, and get tmap_num from there.
static void propagate_tmaps_to_segment_sides(const vmsegptridx_t base_seg, int base_side, const vmsegptridx_t con_seg, int con_side, int uv_only_flag)
{
	int		abs_id1,abs_id2;
	int		v;

	auto &base_vp = Side_to_verts[base_side];

	// Do for each edge on connecting face.
	for (v=0; v<4; v++) {
                abs_id1 = base_seg->verts[static_cast<int>(base_vp[v])];
                abs_id2 = base_seg->verts[static_cast<int>(base_vp[(v+1) % 4])];
		propagate_tmaps_to_segment_side(base_seg, base_side, con_seg, con_side, abs_id1, abs_id2, uv_only_flag);
	}

}

// -----------------------------------------------------------------------------
//	Propagate texture maps in base_seg to con_seg.
//	For each wall in con_seg, find the wall in base_seg which shared an edge.  Copy tmap_num from that
//	wall in base_seg to the wall in con_seg.  If the wall in base_seg is not present, then look at the
//	segment connected through base_seg through the wall.  The wall with a common edge is the new wall
//	of interest.  Continue searching in this way until a wall of interest is present.
void med_propagate_tmaps_to_segments(const vmsegptridx_t base_seg,const vmsegptridx_t con_seg, int uv_only_flag)
{
	int		s;

	for (s=0; s<MAX_SIDES_PER_SEGMENT; s++)
		if (base_seg->children[s] == con_seg)
			propagate_tmaps_to_segment_sides(base_seg, s, con_seg, find_connect_side(base_seg, con_seg), uv_only_flag);

	con_seg->static_light = base_seg->static_light;

	validate_uv_coordinates(con_seg);
}


// -------------------------------------------------------------------------------
//	Copy texture map uvs from srcseg to destseg.
//	If two segments have different face structure (eg, destseg has two faces on side 3, srcseg has only 1)
//	then assign uvs according to side vertex id, not face vertex id.
void copy_uvs_seg_to_seg(const vmsegptr_t destseg, const vcsegptr_t srcseg)
{
	int	s;

	for (s=0; s<MAX_SIDES_PER_SEGMENT; s++) {
		destseg->sides[s].tmap_num = srcseg->sides[s].tmap_num;
		destseg->sides[s].tmap_num2 = srcseg->sides[s].tmap_num2;
	}

	destseg->static_light = srcseg->static_light;
}

}

namespace dcx {

//	_________________________________________________________________________________________________________________________
//	Maximum distance between a segment containing light to a segment to receive light.
#define	LIGHT_DISTANCE_THRESHOLD	(F1_0*80)
fix	Magical_light_constant = (F1_0*16);

// int	Seg0, Seg1;

//int	Bugseg = 27;

struct hash_info {
	sbyte			flag, hit_type;
	vms_vector	vector;
};

#define	FVI_HASH_SIZE 8
#define	FVI_HASH_AND_MASK (FVI_HASH_SIZE - 1)

//	Note: This should be malloced.
//			Also, the vector should not be 12 bytes, you should only care about some smaller portion of it.
static array<hash_info, FVI_HASH_SIZE> fvi_cache;
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
static void cast_light_from_side(const vmsegptridx_t segp, int light_side, fix light_intensity, int quick_light)
{
	int			sidenum,vertnum;
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
				for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++) {
					if (WALL_IS_DOORWAY(GameBitmaps, Textures, vcwallptr, rsegp, rsegp, sidenum) != WID_NO_WALL)
					{
						const auto rsidep = &rsegp->sides[sidenum];
						auto &side_normalp = rsidep->normals[0];	//	kinda stupid? always use vector 0.

						for (vertnum=0; vertnum<4; vertnum++) {
							fix			distance_to_point, light_at_point, light_dot;

							const auto abs_vertnum = rsegp->verts[Side_to_verts[sidenum][vertnum]];
							vms_vector vert_location = *vcvertptr(abs_vertnum);
							distance_to_point = vm_vec_dist_quick(vert_location, light_location);
							const auto vector_to_light = vm_vec_normalized(vm_vec_sub(light_location, vert_location));

							//	Hack: In oblong segments, it's possible to get a very small dot product
							//	but the light source is very nearby (eg, illuminating light itself!).
							light_dot = vm_vec_dot(vector_to_light, side_normalp);
							if (distance_to_point < F1_0)
								if (light_dot > 0)
									light_dot = (light_dot + F1_0)/2;

							if (light_dot > 0) {
								light_at_point = fixdiv(fixmul(light_dot, light_dot), distance_to_point);
								light_at_point = fixmul(light_at_point, Magical_light_constant);
								if (light_at_point >= 0) {
									fvi_info	hit_data;
									int		hit_type;
									fix		inverse_segment_magnitude;

									const auto r_vector_to_center = vm_vec_sub(r_segment_center, vert_location);
									inverse_segment_magnitude = fixdiv(F1_0/3, vm_vec_mag(r_vector_to_center));
									const auto vert_location_1 = vm_vec_scale_add(vert_location, r_vector_to_center, inverse_segment_magnitude);
									vert_location = vert_location_1;

//if ((segp-Segments == 199) && (rsegp-Segments==199))
//	Int3();
// Seg0 = segp-Segments;
// Seg1 = rsegp-Segments;
									if (!quick_light) {
										int hash_value = Side_to_verts[sidenum][vertnum];
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
												fvi_query fq;

												Hash_calcs++;
												hashp->vector = vector_to_light;
												hashp->flag = 1;

												fq.p0						= &light_location;
												fq.startseg				= segp;
												fq.p1						= &vert_location;
												fq.rad					= 0;
												fq.thisobjnum			= object_none;
												fq.ignore_obj_list.first = nullptr;
												fq.flags					= 0;

												hit_type = find_vector_intersection(fq, hit_data);
												hashp->hit_type = hit_type;
												break;
											}
										}
									} else
										hit_type = HIT_NONE;
									switch (hit_type) {
										case HIT_NONE:
											light_at_point = fixmul(light_at_point, light_intensity);
											rsidep->uvls[vertnum].l += light_at_point;
											if (rsidep->uvls[vertnum].l > F1_0)
												rsidep->uvls[vertnum].l = F1_0;
											break;
										case HIT_WALL:
											break;
										case HIT_OBJECT:
											Int3();	// Hit object, should be ignoring objects!
											break;
										case HIT_BAD_P0:
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
	range_for (const auto &&segp, vmsegptr)
	{
		range_for (auto &side, segp->sides)
		{
			range_for (auto &uvl, side.uvls)
				uvl.l = F1_0/64;	// Put a tiny bit of light here.
		}
		segp->static_light = F1_0 / 64;
	}
}


//	------------------------------------------------------------------------------------------
//	Used in setting average light value in a segment, cast light from a side to the center
//	of all segments.
static void cast_light_from_side_to_center(const vmsegptridx_t segp, int light_side, fix light_intensity, int quick_light)
{
	const auto segment_center = compute_segment_center(vcvertptr, segp);
	//	Do for four lights, one just inside each corner of side containing light.
	range_for (const auto lightnum, Side_to_verts[light_side])
	{
		const auto light_vertex_num = segp->verts[lightnum];
		auto &vert_light_location = *vcvertptr(light_vertex_num);
		const auto vector_to_center = vm_vec_sub(segment_center, vert_light_location);
		const auto light_location = vm_vec_scale_add(vert_light_location, vector_to_center, F1_0/64);

		range_for (const auto &&rsegp, vmsegptr)
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
					int		hit_type;

					if (!quick_light) {
						fvi_query fq;
						fvi_info	hit_data;

						fq.p0						= &light_location;
						fq.startseg				= segp;
						fq.p1						= &r_segment_center;
						fq.rad					= 0;
						fq.thisobjnum			= object_none;
						fq.ignore_obj_list.first = nullptr;
						fq.flags					= 0;

						hit_type = find_vector_intersection(fq, hit_data);
					}
					else
						hit_type = HIT_NONE;

					switch (hit_type) {
						case HIT_NONE:
							light_at_point = fixmul(light_at_point, light_intensity);
							if (light_at_point >= F1_0)
								light_at_point = F1_0-1;
							rsegp->static_light += light_at_point;
							if (segp->static_light < 0)	// if it went negative, saturate
								segp->static_light = 0;
							break;
						case HIT_WALL:
							break;
						case HIT_OBJECT:
							Int3();	// Hit object, should be ignoring objects!
							break;
						case HIT_BAD_P0:
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
	int	sidenum;

	range_for (const auto &&segp, vmsegptridx)
	{
		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++) {
			// if (!IS_CHILD(segp->children[sidenum])) {
			if (WALL_IS_DOORWAY(GameBitmaps, Textures, vcwallptr, segp, segp, sidenum) != WID_NO_WALL)
			{
				const auto sidep = &segp->sides[sidenum];
				fix	light_intensity;

				light_intensity = TmapInfo[sidep->tmap_num].lighting + TmapInfo[sidep->tmap_num2 & 0x3fff].lighting;

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
					cast_light_from_side(segp, sidenum, light_intensity, quick_light);
					cast_light_from_side_to_center(segp, sidenum, light_intensity, quick_light);
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

	validate_segment_all();

	calim_zero_light_values();

	calim_process_all_lights(quick_flag);
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
