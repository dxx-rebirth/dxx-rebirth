/*
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
#include "fix.h"
#include "dxxerror.h"
#include "wall.h"
#include "editor/kdefs.h"
#include "bm.h"		//	Needed for TmapInfo
#include	"effects.h"     //      Needed for effects_bm_num
#include "fvi.h"

void cast_all_light_in_mine(int quick_flag);
//--rotate_uvs-- vms_vector Rightvec;

//	---------------------------------------------------------------------------------------------
//	Returns approximate area of a side
fix area_on_side(side *sidep)
{
	fix	du,dv,width,height;

	du = sidep->uvls[1].u - sidep->uvls[0].u;
	dv = sidep->uvls[1].v - sidep->uvls[0].v;

	width = fix_sqrt(fixmul(du,du) + fixmul(dv,dv));

	du = sidep->uvls[3].u - sidep->uvls[0].u;
	dv = sidep->uvls[3].v - sidep->uvls[0].v;

	height = fix_sqrt(fixmul(du,du) + fixmul(dv,dv));

	return fixmul(width, height);
}

//	-------------------------------------------------------------------------------------------
//	DEBUG function -- callable from debugger.
//	Returns approximate area of all sides which get mapped (ie, are not a connection).
//	I wrote this because I was curious how much memory would be required to texture map all
//	sides individually with custom artwork.  For demo1.min on 2/18/94, it would be about 5 meg.
int area_on_all_sides(void)
{
	int	i,s;
	int	total_area = 0;

	for (i=0; i<=Highest_segment_index; i++) {
		segment *segp = &Segments[i];

		for (s=0; s<MAX_SIDES_PER_SEGMENT; s++)
			if (!IS_CHILD(segp->children[s]))
				total_area += f2i(area_on_side(&segp->sides[s]));
	}

	return total_area;
}

fix average_connectivity(void)
{
	int	i,s;
	int	total_sides = 0, total_mapped_sides = 0;

	for (i=0; i<=Highest_segment_index; i++) {
		segment *segp = &Segments[i];

		for (s=0; s<MAX_SIDES_PER_SEGMENT; s++) {
			if (!IS_CHILD(segp->children[s]))
				total_mapped_sides++;
			total_sides++;
		}
	}

	return 6 * fixdiv(total_mapped_sides, total_sides);
}

#define	MAX_LIGHT_SEGS 16

//	---------------------------------------------------------------------------------------------
//	Scan all polys in all segments, return average light value for vnum.
//	segs = output array for segments containing vertex, terminated by -1.
fix get_average_light_at_vertex(int vnum, short *segs)
{
	int	segnum, relvnum, sidenum;
	fix	total_light;
	int	num_occurrences;
//	#ifndef NDEBUG //Removed this ifdef because the version of Assert that I used to get it to compile doesn't work without this symbol. -KRB
        short   *original_segs;

        original_segs = segs;
//	#endif


	num_occurrences = 0;
	total_light = 0;

	for (segnum=0; segnum<=Highest_segment_index; segnum++) {
		segment *segp = &Segments[segnum];
		int *vp = segp->verts;

		for (relvnum=0; relvnum<MAX_VERTICES_PER_SEGMENT; relvnum++)
			if (*vp++ == vnum)
				break;

		if (relvnum < MAX_VERTICES_PER_SEGMENT) {

			*segs++ = segnum;
			Assert(segs - original_segs < MAX_LIGHT_SEGS);
			(void)original_segs;

			for (sidenum=0; sidenum < MAX_SIDES_PER_SEGMENT; sidenum++) {
				if (!IS_CHILD(segp->children[sidenum])) {
					side	*sidep = &segp->sides[sidenum];
					const sbyte	*vp = Side_to_verts[sidenum];
					int	v;

					for (v=0; v<4; v++)
						if (*vp++ == relvnum) {
							total_light += sidep->uvls[v].l;
							num_occurrences++;
						}
				}	// end if
			}	// end sidenum
		}
	}	// end segnum

	*segs = -1;

	if (num_occurrences)
		return total_light/num_occurrences;
	else
		return 0;

}

void set_average_light_at_vertex(int vnum)
{
	int	relvnum, sidenum;
	short	Segment_indices[MAX_LIGHT_SEGS];
	int	segind;

	fix average_light;

	average_light = get_average_light_at_vertex(vnum, Segment_indices);

	if (!average_light)
		return;

	segind = 0;
	while (Segment_indices[segind] != -1) {
		int segnum = Segment_indices[segind++];

		segment *segp = &Segments[segnum];

		for (relvnum=0; relvnum<MAX_VERTICES_PER_SEGMENT; relvnum++)
			if (segp->verts[relvnum] == vnum)
				break;

		if (relvnum < MAX_VERTICES_PER_SEGMENT) {
			for (sidenum=0; sidenum < MAX_SIDES_PER_SEGMENT; sidenum++) {
				if (!IS_CHILD(segp->children[sidenum])) {
					side *sidep = &segp->sides[sidenum];
					const sbyte	*vp = Side_to_verts[sidenum];
					int	v;

					for (v=0; v<4; v++)
						if (*vp++ == relvnum)
							sidep->uvls[v].l = average_light;
				}	// end if
			}	// end sidenum
		}	// end if
	}	// end while

	Update_flags |= UF_WORLD_CHANGED;
}

void set_average_light_on_side(segment *segp, int sidenum)
{
	int	v;

	if (!IS_CHILD(segp->children[sidenum]))
		for (v=0; v<4; v++) {
			set_average_light_at_vertex(segp->verts[Side_to_verts[sidenum][v]]);
		}

}

int set_average_light_on_curside(void)
{
	set_average_light_on_side(Cursegp, Curside);
	return 0;
}

//	-----------------------------------------------------------------------------------------
void set_average_light_on_all_fast(void)
{
	int	s,v,relvnum;
	fix	al;
	int	alc;
	int	seglist[MAX_LIGHT_SEGS];
	int	*segptr;

	set_vertex_counts();

	//	Set total light value for all vertices in array average_light.
	for (v=0; v<=Highest_vertex_index; v++) {
		al = 0;
		alc = 0;

		if (Vertex_active[v]) {
			segptr = seglist;

			for (s=0; s<=Highest_segment_index; s++) {
				segment *segp = &Segments[s];
				for (relvnum=0; relvnum<MAX_VERTICES_PER_SEGMENT; relvnum++)
					if (segp->verts[relvnum] == v)
						break;

					if (relvnum != MAX_VERTICES_PER_SEGMENT) {
						int		si;

						*segptr++ = s;			// Note this segment in list, so we can process it below.
						Assert(segptr - seglist < MAX_LIGHT_SEGS);

						for (si=0; si<MAX_SIDES_PER_SEGMENT; si++) {
							if (!IS_CHILD(segp->children[si])) {
								side	*sidep = &segp->sides[si];
								const sbyte	*vp = Side_to_verts[si];
								int	vv;

								for (vv=0; vv<4; vv++)
									if (*vp++ == relvnum) {
										al += sidep->uvls[vv].l;
										alc++;
									}
							}	// if (segp->children[si == -1) {
						}	// for (si=0...
					}	// if (relvnum != ...
			}	// for (s=0; ...

			*segptr = -1;

			//	Now, divide average_light by number of number of occurrences for each vertex
			if (alc)
				al /= alc;
			else
				al = 0;

			segptr = seglist;
			while (*segptr != -1) {
				int 		segnum = *segptr++;
				segment	*segp = &Segments[segnum];
				int		sidenum;

				for (relvnum=0; relvnum<MAX_VERTICES_PER_SEGMENT; relvnum++)
					if (segp->verts[relvnum] == v)
						break;

				Assert(relvnum < MAX_VERTICES_PER_SEGMENT);	// IMPOSSIBLE! This segment is in seglist, but vertex v does not occur!
				for (sidenum=0; sidenum < MAX_SIDES_PER_SEGMENT; sidenum++) {
					int	wid_result;
					wid_result = WALL_IS_DOORWAY(segp, sidenum);
					if ((wid_result != WID_FLY_FLAG) && (wid_result != WID_NO_WALL)) {
						side *sidep = &segp->sides[sidenum];
						const sbyte	*vp = Side_to_verts[sidenum];
						int	v;

						for (v=0; v<4; v++)
							if (*vp++ == relvnum)
								sidep->uvls[v].l = al;
					}	// end if
				}	// end sidenum
			}	// end while

		}	// if (Vertex_active[v]...

	}	// for (v=0...

}

extern int Doing_lighting_hack_flag;
int set_average_light_on_all(void)
{
//	set_average_light_on_all_fast();

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
fix compute_uv_dist(uvl *uv0, uvl *uv1)
{
	vms_vector	v0,v1;

	v0.x = uv0->u;
	v0.y = 0;
	v0.z = uv0->v;

	v1.x = uv1->u;
	v1.y = 0;
	v1.z = uv1->v;

	return vm_vec_dist(&v0,&v1);
}

//	---------------------------------------------------------------------------------------------
//	Given a polygon, compress the uv coordinates so that they are as close to 0 as possible.
//	Do this by adding a constant u and v to each uv pair.
void compress_uv_coordinates(side *sidep)
{
	int	v;
	fix	uc, vc;

	uc = 0;
	vc = 0;

	for (v=0; v<4; v++) {
		uc += sidep->uvls[v].u;
		vc += sidep->uvls[v].v;
	}

	uc /= 4;
	vc /= 4;
	uc = uc & 0xffff0000;
	vc = vc & 0xffff0000;

	for (v=0; v<4; v++) {
		sidep->uvls[v].u -= uc;
		sidep->uvls[v].v -= vc;
	}

}

//	---------------------------------------------------------------------------------------------
void compress_uv_coordinates_on_side(side *sidep)
{
	compress_uv_coordinates(sidep);
}

//	---------------------------------------------------------------------------------------------
void validate_uv_coordinates_on_side(segment *segp, int sidenum)
{
//	int			v;
//	fix			uv_dist,threed_dist;
//	vms_vector	tvec;
//	fix			dist_ratios[MAX_VERTICES_PER_POLY];
	side			*sidep = &segp->sides[sidenum];
//	sbyte			*vp = Side_to_verts[sidenum];

//	This next hunk doesn't seem to affect anything. @mk, 02/13/94
//	for (v=1; v<4; v++) {
//		uv_dist = compute_uv_dist(&sidep->uvls[v],&sidep->uvls[0]);
//		threed_dist = vm_vec_mag(vm_vec_sub(&tvec,&Vertices[segp->verts[vp[v]],&Vertices[vp[0]]));
//		dist_ratios[v-1] = fixdiv(uv_dist,threed_dist);
//	}

	compress_uv_coordinates_on_side(sidep);
}

void compress_uv_coordinates_in_segment(segment *segp)
{
	int	side;

	for (side=0; side<MAX_SIDES_PER_SEGMENT; side++)
		compress_uv_coordinates_on_side(&segp->sides[side]);
}

void compress_uv_coordinates_all(void)
{
	int	seg;

	for (seg=0; seg<=Highest_segment_index; seg++)
		if (Segments[seg].segnum != -1)
			compress_uv_coordinates_in_segment(&Segments[seg]);
}

void check_lighting_side(segment *sp, int sidenum)
{
	int	v;
	side	*sidep = &sp->sides[sidenum];

	for (v=0; v<4; v++)
		if ((sidep->uvls[v].l > F1_0*16) || (sidep->uvls[v].l < 0))
			Int3();
}

void check_lighting_segment(segment *segp)
{
	int	side;

	for (side=0; side<MAX_SIDES_PER_SEGMENT; side++)
		check_lighting_side(segp, side);
}

//	Flag bogus lighting values.
void check_lighting_all(void)
{
	int	seg;

	for (seg=0; seg<=Highest_segment_index; seg++)
		if (Segments[seg].segnum != -1)
			check_lighting_segment(&Segments[seg]);
}

void assign_default_lighting_on_side(segment *segp, int sidenum)
{
	int	v;
	side	*sidep = &segp->sides[sidenum];

	for (v=0; v<4; v++)
		sidep->uvls[v].l = DEFAULT_LIGHTING;
}

void assign_default_lighting(segment *segp)
{
	int	sidenum;

	for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++)
		assign_default_lighting_on_side(segp, sidenum);
}

void assign_default_lighting_all(void)
{
	int	seg;

	for (seg=0; seg<=Highest_segment_index; seg++)
		if (Segments[seg].segnum != -1)
			assign_default_lighting(&Segments[seg]);
}

//	---------------------------------------------------------------------------------------------
void validate_uv_coordinates(segment *segp)
{
	int	s;

	for (s=0; s<MAX_SIDES_PER_SEGMENT; s++)
		validate_uv_coordinates_on_side(segp,s);

}

//	---------------------------------------------------------------------------------------------
//	For all faces in side, copy uv coordinates from uvs array to face.
void copy_uvs_from_side_to_faces(segment *segp, int sidenum, uvl uvls[])
{
	int	v;
	side	*sidep = &segp->sides[sidenum];

	for (v=0; v<4; v++)
		sidep->uvls[v] = uvls[v];

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
fix zhypot(fix a,fix b) {
	double x = (double)a / 65536;
	double y = (double)b / 65536;
	return (long)(sqrt(x * x + y * y) * 65536);
}
#endif

//	---------------------------------------------------------------------------------------------
//	Assign lighting value to side, a function of the normal vector.
void assign_light_to_side(segment *sp, int sidenum)
{
	int	v;
	side	*sidep = &sp->sides[sidenum];

	for (v=0; v<4; v++)
		sidep->uvls[v].l = DEFAULT_LIGHTING;
}

fix	Stretch_scale_x = F1_0;
fix	Stretch_scale_y = F1_0;

//	---------------------------------------------------------------------------------------------
//	Given u,v coordinates at two vertices, assign u,v coordinates to other two vertices on a side.
//	(Actually, assign them to the coordinates in the faces.)
//	va, vb = face-relative vertex indices corresponding to uva, uvb.  Ie, they are always in 0..3 and should be looked up in
//	Side_to_verts[side] to get the segment relative index.
void assign_uvs_to_side(segment *segp, int sidenum, uvl *uva, uvl *uvb, int va, int vb)
{
	int			vlo,vhi,v0,v1,v2,v3;
	vms_vector	fvec,rvec,tvec;
	vms_matrix	rotmat;
	uvl			uvls[4],ruvmag,fuvmag,uvlo,uvhi;
	fix			fmag,mag01;
	sbyte			*vp;

	Assert( (va<4) && (vb<4) );
	Assert((abs(va - vb) == 1) || (abs(va - vb) == 3));		// make sure the verticies specify an edge

	vp = (sbyte *)&Side_to_verts[sidenum];

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
	vm_vec_sub(&fvec,&Vertices[v1],&Vertices[v0]);
	vm_vec_sub(&rvec,&Vertices[v3],&Vertices[v0]);

	if (((fvec.x == 0) && (fvec.y == 0) && (fvec.z == 0)) || ((rvec.x == 0) && (rvec.y == 0) && (rvec.z == 0))) {
		rotmat = vmd_identity_matrix;
	} else
		vm_vector_2_matrix(&rotmat,&fvec,0,&rvec);

	rvec = rotmat.rvec; vm_vec_negate(&rvec);
	fvec = rotmat.fvec;

	mag01 = vm_vec_dist(&Vertices[v1],&Vertices[v0]);
	if ((va == 0) || (va == 2))
		mag01 = fixmul(mag01, Stretch_scale_x);
	else
		mag01 = fixmul(mag01, Stretch_scale_y);

	if (mag01 < F1_0/1024 )
		editor_status_fmt("U, V bogosity in segment #%hu, probably on side #%i.  CLEAN UP YOUR MESS!", (unsigned short)(segp-Segments), sidenum);
	else {
		vm_vec_sub(&tvec,&Vertices[v2],&Vertices[v1]);
		uvls[(vhi+1)%4].u = uvhi.u + 
			fixdiv(fixmul(ruvmag.u,vm_vec_dotprod(&rvec,&tvec)),mag01) +
			fixdiv(fixmul(fuvmag.u,vm_vec_dotprod(&fvec,&tvec)),mag01);

		uvls[(vhi+1)%4].v = uvhi.v + 
			fixdiv(fixmul(ruvmag.v,vm_vec_dotprod(&rvec,&tvec)),mag01) +
			fixdiv(fixmul(fuvmag.v,vm_vec_dotprod(&fvec,&tvec)),mag01);


		vm_vec_sub(&tvec,&Vertices[v3],&Vertices[v0]);
		uvls[(vhi+2)%4].u = uvlo.u + 
			fixdiv(fixmul(ruvmag.u,vm_vec_dotprod(&rvec,&tvec)),mag01) +
			fixdiv(fixmul(fuvmag.u,vm_vec_dotprod(&fvec,&tvec)),mag01);

		uvls[(vhi+2)%4].v = uvlo.v + 
			fixdiv(fixmul(ruvmag.v,vm_vec_dotprod(&rvec,&tvec)),mag01) +
			fixdiv(fixmul(fuvmag.v,vm_vec_dotprod(&fvec,&tvec)),mag01);

		uvls[(vhi+1)%4].l = uvhi.l;
		uvls[(vhi+2)%4].l = uvlo.l;

		copy_uvs_from_side_to_faces(segp, sidenum, uvls);
	}
}


int Vmag = VMAG;

// -----------------------------------------------------------------------------------------------------------
//	Assign default uvs to side.
//	This means:
//		v0 = 0,0
//		v1 = k,0 where k is 3d size dependent
//	v2, v3 assigned by assign_uvs_to_side
void assign_default_uvs_to_side(segment *segp,int side)
{
	uvl			uv0,uv1;
	const sbyte			*vp;

	uv0.u = 0;
	uv0.v = 0;

	vp = Side_to_verts[side];

	uv1.u = 0;
	uv1.v = Num_tilings * fixmul(Vmag, vm_vec_dist(&Vertices[segp->verts[vp[1]]],&Vertices[segp->verts[vp[0]]]));

	assign_uvs_to_side(segp, side, &uv0, &uv1, 0, 1);
}

// -----------------------------------------------------------------------------------------------------------
//	Assign default uvs to side.
//	This means:
//		v0 = 0,0
//		v1 = k,0 where k is 3d size dependent
//	v2, v3 assigned by assign_uvs_to_side
void stretch_uvs_from_curedge(segment *segp, int side)
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
void assign_default_uvs_to_segment(segment *segp)
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
void med_assign_uvs_to_side(segment *con_seg, int con_common_side, segment *base_seg, int base_common_side, int abs_id1, int abs_id2)
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
void get_side_ids(segment *base_seg, segment *con_seg, int base_side, int con_side, int abs_id1, int abs_id2, int *base_common_side, int *con_common_side)
{
	const sbyte	*base_vp,*con_vp;
	int		v0,side;

	*base_common_side = -1;

	//	Find side in base segment which contains the two global vertex ids.
	for (side=0; side<MAX_SIDES_PER_SEGMENT; side++) {
		if (side != base_side) {
			base_vp = Side_to_verts[side];
			for (v0=0; v0<4; v0++)
                                if (((base_seg->verts[(int) base_vp[v0]] == abs_id1) && (base_seg->verts[(int) base_vp[(v0+1) % 4]] == abs_id2)) || ((base_seg->verts[(int) base_vp[v0]] == abs_id2) && (base_seg->verts[(int)base_vp[ (v0+1) % 4]] == abs_id1))) {
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
			con_vp = Side_to_verts[side];
			for (v0=0; v0<4; v0++)
                                if (((con_seg->verts[(int) con_vp[(v0 + 1) % 4]] == abs_id1) && (con_seg->verts[(int) con_vp[v0]] == abs_id2)) || ((con_seg->verts[(int) con_vp[(v0 + 1) % 4]] == abs_id2) && (con_seg->verts[(int) con_vp[v0]] == abs_id1))) {
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
void propagate_tmaps_to_segment_side(segment *base_seg, int base_side, segment *con_seg, int con_side, int abs_id1, int abs_id2, int uv_only_flag)
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
			int	cside;

			cside = find_connect_side(base_seg, &Segments[base_seg->children[base_common_side]]);
			propagate_tmaps_to_segment_side(&Segments[base_seg->children[base_common_side]], cside, con_seg, con_side, abs_id1, abs_id2, uv_only_flag);
		}
	}

}

sbyte	Edge_between_sides[MAX_SIDES_PER_SEGMENT][MAX_SIDES_PER_SEGMENT][2] = {
//		left		top		right		bottom	back		front
	{ {-1,-1}, { 3, 7}, {-1,-1}, { 2, 6}, { 6, 7}, { 2, 3} },	// left
	{ { 3, 7}, {-1,-1}, { 0, 4}, {-1,-1}, { 4, 7}, { 0, 3} },	// top
	{ {-1,-1}, { 0, 4}, {-1,-1}, { 1, 5}, { 4, 5}, { 0, 1} },	// right
	{ { 2, 6}, {-1,-1}, { 1, 5}, {-1,-1}, { 5, 6}, { 1, 2} },	// bottom
	{ { 6, 7}, { 4, 7}, { 4, 5}, { 5, 6}, {-1,-1}, {-1,-1} },	// back
	{ { 2, 3}, { 0, 3}, { 0, 1}, { 1, 2}, {-1,-1}, {-1,-1} }};	// front

// -----------------------------------------------------------------------------
//	Propagate texture map u,v coordinates to base_seg:back_side from base_seg:some-other-side
//	There is no easy way to figure out which side is adjacent to another side along some edge, so we do a bit of searching.
void med_propagate_tmaps_to_back_side(segment *base_seg, int back_side, int uv_only_flag)
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

void fix_bogus_uvs_on_side1(segment *sp, int sidenum, int uvonly_flag)
{
	side	*sidep = &sp->sides[sidenum];

	if ((sidep->uvls[0].u == 0) && (sidep->uvls[1].u == 0) && (sidep->uvls[2].u == 0)) {
		med_propagate_tmaps_to_back_side(sp, sidenum, uvonly_flag);
	}
}

void fix_bogus_uvs_seg(segment *segp)
{
	int	s;

	for (s=0; s<MAX_SIDES_PER_SEGMENT; s++) {
		if (!IS_CHILD(segp->children[s]))
			fix_bogus_uvs_on_side1(segp, s, 1);
	}
}

int fix_bogus_uvs_all(void)
{
	int	seg;

	for (seg=0; seg<=Highest_segment_index; seg++)
		if (Segments[seg].segnum != -1)
			fix_bogus_uvs_seg(&Segments[seg]);
	return 0;
}

// -----------------------------------------------------------------------------
//	Propagate texture map u,v coordinates to base_seg:back_side from base_seg:some-other-side
//	There is no easy way to figure out which side is adjacent to another side along some edge, so we do a bit of searching.
void med_propagate_tmaps_to_any_side(segment *base_seg, int back_side, int tmap_num, int uv_only_flag)
{
        int     v1=0,v2=0;
	int	s;

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

	base_seg->sides[back_side].tmap_num = tmap_num;

}

// -----------------------------------------------------------------------------
//	Segment base_seg is connected through side base_side to segment con_seg on con_side.
//	For all walls in con_seg, find the wall in base_seg which shares an edge.  Copy tmap_num
//	from that side in base_seg to the wall in con_seg.  If the wall in base_seg is not present
//	(ie, there is another segment connected through it), follow the connection through that
//	segment to get the wall in the connected segment which shares the edge, and get tmap_num from there.
void propagate_tmaps_to_segment_sides(segment *base_seg, int base_side, segment *con_seg, int con_side, int uv_only_flag)
{
	const sbyte		*base_vp;
	int		abs_id1,abs_id2;
	int		v;

	base_vp = Side_to_verts[base_side];

	// Do for each edge on connecting face.
	for (v=0; v<4; v++) {
                abs_id1 = base_seg->verts[(int) base_vp[v]];
                abs_id2 = base_seg->verts[(int) base_vp[(v+1) % 4]];
		propagate_tmaps_to_segment_side(base_seg, base_side, con_seg, con_side, abs_id1, abs_id2, uv_only_flag);
	}

}

// -----------------------------------------------------------------------------
//	Propagate texture maps in base_seg to con_seg.
//	For each wall in con_seg, find the wall in base_seg which shared an edge.  Copy tmap_num from that
//	wall in base_seg to the wall in con_seg.  If the wall in base_seg is not present, then look at the
//	segment connected through base_seg through the wall.  The wall with a common edge is the new wall
//	of interest.  Continue searching in this way until a wall of interest is present.
void med_propagate_tmaps_to_segments(segment *base_seg,segment *con_seg, int uv_only_flag)
{
	int		s;

	for (s=0; s<MAX_SIDES_PER_SEGMENT; s++)
		if (base_seg->children[s] == con_seg-Segments)
			propagate_tmaps_to_segment_sides(base_seg, s, con_seg, find_connect_side(base_seg, con_seg), uv_only_flag);

	s2s2(con_seg)->static_light = s2s2(base_seg)->static_light;

	validate_uv_coordinates(con_seg);
}


// -------------------------------------------------------------------------------
//	Copy texture map uvs from srcseg to destseg.
//	If two segments have different face structure (eg, destseg has two faces on side 3, srcseg has only 1)
//	then assign uvs according to side vertex id, not face vertex id.
void copy_uvs_seg_to_seg(segment *destseg,segment *srcseg)
{
	int	s;

	for (s=0; s<MAX_SIDES_PER_SEGMENT; s++) {
		destseg->sides[s].tmap_num = srcseg->sides[s].tmap_num;
		destseg->sides[s].tmap_num2 = srcseg->sides[s].tmap_num2;
	}

	s2s2(destseg)->static_light = s2s2(srcseg)->static_light;
}

//	_________________________________________________________________________________________________________________________
//	Maximum distance between a segment containing light to a segment to receive light.
#define	LIGHT_DISTANCE_THRESHOLD	(F1_0*80)
fix	Magical_light_constant = (F1_0*16);

// int	Seg0, Seg1;

//int	Bugseg = 27;

typedef struct {
	sbyte			flag, hit_type;
	vms_vector	vector;
} hash_info;

#define	FVI_HASH_SIZE 8
#define	FVI_HASH_AND_MASK (FVI_HASH_SIZE - 1)

//	Note: This should be malloced.
//			Also, the vector should not be 12 bytes, you should only care about some smaller portion of it.
hash_info	fvi_cache[FVI_HASH_SIZE];
int	Hash_hits=0, Hash_retries=0, Hash_calcs=0;

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
void cast_light_from_side(segment *segp, int light_side, fix light_intensity, int quick_light)
{
	vms_vector	segment_center;
	int			segnum,sidenum,vertnum, lightnum;

	compute_segment_center(&segment_center, segp);

	//	Do for four lights, one just inside each corner of side containing light.
	for (lightnum=0; lightnum<4; lightnum++) {
		int			light_vertex_num, i;
		vms_vector	vector_to_center;
		vms_vector	light_location;
		// fix			inverse_segment_magnitude;

		light_vertex_num = segp->verts[Side_to_verts[light_side][lightnum]];
		light_location = Vertices[light_vertex_num];


	//	New way, 5/8/95: Move towards center irrespective of size of segment.
	vm_vec_sub(&vector_to_center, &segment_center, &light_location);
	vm_vec_normalize_quick(&vector_to_center);
	vm_vec_add2(&light_location, &vector_to_center);

// -- Old way, before 5/8/95 --		// -- This way was kind of dumb.  In larger segments, you move LESS towards the center.
// -- Old way, before 5/8/95 --		//    Main problem, though, is vertices don't illuminate themselves well in oblong segments because the dot product is small.
// -- Old way, before 5/8/95 --		vm_vec_sub(&vector_to_center, &segment_center, &light_location);
// -- Old way, before 5/8/95 --		inverse_segment_magnitude = fixdiv(F1_0/5, vm_vec_mag(&vector_to_center));
// -- Old way, before 5/8/95 --		vm_vec_scale_add(&light_location, &light_location, &vector_to_center, inverse_segment_magnitude);

		for (segnum=0; segnum<=Highest_segment_index; segnum++) {
			segment		*rsegp = &Segments[segnum];
			vms_vector	r_segment_center;
			fix			dist_to_rseg;

			for (i=0; i<FVI_HASH_SIZE; i++)
				fvi_cache[i].flag = 0;

			//	efficiency hack (I hope!), for faraway segments, don't check each point.
			compute_segment_center(&r_segment_center, rsegp);
			dist_to_rseg = vm_vec_dist_quick(&r_segment_center, &segment_center);

			if (dist_to_rseg <= LIGHT_DISTANCE_THRESHOLD) {
				for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++) {
					if (WALL_IS_DOORWAY(rsegp, sidenum) != WID_NO_WALL) {
						side			*rsidep = &rsegp->sides[sidenum];
						vms_vector	*side_normalp = &rsidep->normals[0];	//	kinda stupid? always use vector 0.

						for (vertnum=0; vertnum<4; vertnum++) {
							fix			distance_to_point, light_at_point, light_dot;
							vms_vector	vert_location, vector_to_light;
							int			abs_vertnum;

							abs_vertnum = rsegp->verts[Side_to_verts[sidenum][vertnum]];
							vert_location = Vertices[abs_vertnum];
							distance_to_point = vm_vec_dist_quick(&vert_location, &light_location);
							vm_vec_sub(&vector_to_light, &light_location, &vert_location);
							vm_vec_normalize(&vector_to_light);

							//	Hack: In oblong segments, it's possible to get a very small dot product
							//	but the light source is very nearby (eg, illuminating light itself!).
							light_dot = vm_vec_dot(&vector_to_light, side_normalp);
							if (distance_to_point < F1_0)
								if (light_dot > 0)
									light_dot = (light_dot + F1_0)/2;

							if (light_dot > 0) {
								light_at_point = fixdiv(fixmul(light_dot, light_dot), distance_to_point);
								light_at_point = fixmul(light_at_point, Magical_light_constant);
								if (light_at_point >= 0) {
									fvi_info	hit_data;
									int		hit_type;
									vms_vector	vert_location_1, r_vector_to_center;
									fix		inverse_segment_magnitude;

									vm_vec_sub(&r_vector_to_center, &r_segment_center, &vert_location);
									inverse_segment_magnitude = fixdiv(F1_0/3, vm_vec_mag(&r_vector_to_center));
									vm_vec_scale_add(&vert_location_1, &vert_location, &r_vector_to_center, inverse_segment_magnitude);
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
												fq.startseg				= segp-Segments;
												fq.p1						= &vert_location;
												fq.rad					= 0;
												fq.thisobjnum			= -1;
												fq.ignore_obj_list	= NULL;
												fq.flags					= 0;

												hit_type = find_vector_intersection(&fq,&hit_data);
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
void calim_zero_light_values(void)
{
	int	segnum, sidenum, vertnum;

	for (segnum=0; segnum<=Highest_segment_index; segnum++) {
		segment *segp = &Segments[segnum];
		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++) {
			side	*sidep = &segp->sides[sidenum];
			for (vertnum=0; vertnum<4; vertnum++)
				sidep->uvls[vertnum].l = F1_0/64;	// Put a tiny bit of light here.
		}
		Segment2s[segnum].static_light = F1_0 / 64;
	}
}


//	------------------------------------------------------------------------------------------
//	Used in setting average light value in a segment, cast light from a side to the center
//	of all segments.
void cast_light_from_side_to_center(segment *segp, int light_side, fix light_intensity, int quick_light)
{
	vms_vector	segment_center;
	int			segnum, lightnum;

	compute_segment_center(&segment_center, segp);

	//	Do for four lights, one just inside each corner of side containing light.
	for (lightnum=0; lightnum<4; lightnum++) {
		int			light_vertex_num;
		vms_vector	vector_to_center;
		vms_vector	light_location;

		light_vertex_num = segp->verts[Side_to_verts[light_side][lightnum]];
		light_location = Vertices[light_vertex_num];
		vm_vec_sub(&vector_to_center, &segment_center, &light_location);
		vm_vec_scale_add(&light_location, &light_location, &vector_to_center, F1_0/64);

		for (segnum=0; segnum<=Highest_segment_index; segnum++) {
			segment		*rsegp = &Segments[segnum];
			vms_vector	r_segment_center;
			fix			dist_to_rseg;
//if ((segp == &Segments[Bugseg]) && (rsegp == &Segments[Bugseg]))
//	Int3();
			compute_segment_center(&r_segment_center, rsegp);
			dist_to_rseg = vm_vec_dist_quick(&r_segment_center, &segment_center);

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
						fq.startseg				= segp-Segments;
						fq.p1						= &r_segment_center;
						fq.rad					= 0;
						fq.thisobjnum			= -1;
						fq.ignore_obj_list	= NULL;
						fq.flags					= 0;

						hit_type = find_vector_intersection(&fq,&hit_data);
					}
					else
						hit_type = HIT_NONE;

					switch (hit_type) {
						case HIT_NONE:
							light_at_point = fixmul(light_at_point, light_intensity);
							if (light_at_point >= F1_0)
								light_at_point = F1_0-1;
							s2s2(rsegp)->static_light += light_at_point;
							if (s2s2(segp)->static_light < 0)	// if it went negative, saturate
								s2s2(segp)->static_light = 0;
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
void calim_process_all_lights(int quick_light)
{
	int	segnum, sidenum;

	for (segnum=0; segnum<=Highest_segment_index; segnum++) {
		segment	*segp = &Segments[segnum];
		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++) {
			// if (!IS_CHILD(segp->children[sidenum])) {
			if (WALL_IS_DOORWAY(segp, sidenum) != WID_NO_WALL) {
				side	*sidep = &segp->sides[sidenum];
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
void cast_all_light_in_mine(int quick_flag)
{

	validate_segment_all();

	calim_zero_light_values();

	calim_process_all_lights(quick_flag);

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

vms_vector	Normals[MAX_SEGMENTS*12];

int	Normal_nearness = 4;

int normal_near(vms_vector *v1, vms_vector *v2)
{
	if (abs(v1->x - v2->x) < Normal_nearness)
		if (abs(v1->y - v2->y) < Normal_nearness)
			if (abs(v1->z - v2->z) < Normal_nearness)
				return 1;
	return 0;
}

int	Total_normals=0;
int	Diff_normals=0;

void print_normals(void)
{
	int			i,j,s,n,nn;
	// vms_vector	*normal;
	int			num_normals=0;

	Total_normals = 0;
	Diff_normals = 0;

	for (i=0; i<=Highest_segment_index; i++)
		for (s=0; s<6; s++) {
			if (Segments[i].sides[s].type == SIDE_IS_QUAD)
				nn=1;
			else
				nn=2;
			for (n=0; n<nn; n++) {
				for (j=0; j<num_normals; j++)
					if (normal_near(&Segments[i].sides[s].normals[n],&Normals[j]))
						break;
				if (j == num_normals) {
					Normals[num_normals++] = Segments[i].sides[s].normals[n];
					Diff_normals++;
				}
				Total_normals++;
			}
		}

}

