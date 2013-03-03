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
 * Functions for sizing segments
 *
 */

#include <stdlib.h>
#include "inferno.h"
#include "editor.h"
#include "editor/esegment.h"
#include "dxxerror.h"
#include "gameseg.h"

#define XDIM	0
#define YDIM	1
#define ZDIM	2

#define	MAX_MODIFIED_VERTICES	32
int		Modified_vertices[MAX_MODIFIED_VERTICES];
int		Modified_vertex_index = 0;

// ------------------------------------------------------------------------------------------
void validate_modified_segments(void)
{
	int	v,w,v0,seg;
	char	modified_segments[MAX_SEGMENTS];

	for (v=0; v<=Highest_segment_index; v++)
		modified_segments[v] = 0;

	for (v=0; v<Modified_vertex_index; v++) {
		v0 = Modified_vertices[v];

		for (seg = 0; seg <= Highest_segment_index; seg++) {
			int *vp = Segments[seg].verts;
			if (Segments[seg].segnum != -1)
				for (w=0; w<MAX_VERTICES_PER_SEGMENT; w++)
					if (*vp++ == v0)
						modified_segments[seg] = 1;
		}
	}

	for (v=0; v<=Highest_segment_index; v++)
		if (modified_segments[v]) {
			int	s;

			validate_segment(&Segments[v]);
			for (s=0; s<MAX_SIDES_PER_SEGMENT; s++) {
				Num_tilings = 1;
				assign_default_uvs_to_side(&Segments[v], s);
			}
		}
}

// ------------------------------------------------------------------------------------------
//	Scale vertex *vertp by vector *vp, scaled by scale factor scale_factor
void scale_vert_aux(int vertex_ind, vms_vector *vp, fix scale_factor)
{
	vms_vector	*vertp = &Vertices[vertex_ind];

	vertp->x += fixmul(vp->x,scale_factor)/2;
	vertp->y += fixmul(vp->y,scale_factor)/2;
	vertp->z += fixmul(vp->z,scale_factor)/2;

	Assert(Modified_vertex_index < MAX_MODIFIED_VERTICES);
	Modified_vertices[Modified_vertex_index++] = vertex_ind;
}

// ------------------------------------------------------------------------------------------
void scale_vert(segment *sp, int vertex_ind, vms_vector *vp, fix scale_factor)
{
	switch (SegSizeMode) {
		case SEGSIZEMODE_FREE:
			if (is_free_vertex(vertex_ind))
				scale_vert_aux(vertex_ind, vp, scale_factor);
			break;
		case SEGSIZEMODE_ALL:
			scale_vert_aux(vertex_ind, vp, scale_factor);
			break;
		case SEGSIZEMODE_CURSIDE: {
			int	v;
			for (v=0; v<4; v++)
				if (sp->verts[Side_to_verts[Curside][v]] == vertex_ind)
					scale_vert_aux(vertex_ind, vp, scale_factor);
			break;
		}
		case SEGSIZEMODE_EDGE: {
			int	v;

			for (v=0; v<2; v++)
				if (sp->verts[Side_to_verts[Curside][(Curedge+v)%4]] == vertex_ind)
					scale_vert_aux(vertex_ind, vp, scale_factor);
			break;
		}
		case SEGSIZEMODE_VERTEX:
			if (sp->verts[Side_to_verts[Curside][Curvert]] == vertex_ind)
				scale_vert_aux(vertex_ind, vp, scale_factor);
			break;
		default:
			Error("Unsupported SegSizeMode in ksegsize.c/scale_vert = %i\n", SegSizeMode);
	}

}

// ------------------------------------------------------------------------------------------
void scale_free_verts(segment *sp, vms_vector *vp, int side, fix scale_factor)
{
	int		v;
	sbyte		*verts;
	int		vertex_ind;

	verts = Side_to_verts[side];

	for (v=0; v<4; v++) {
                vertex_ind = sp->verts[(int) verts[v]];
		if (SegSizeMode || is_free_vertex(vertex_ind))
			scale_vert(sp, vertex_ind, vp, scale_factor);
	}

}


// -----------------------------------------------------------------------------
//	Make segment *sp bigger in dimension dimension by amount amount.
void med_scale_segment_new(segment *sp, int dimension, fix amount)
{
	vms_matrix	mat;

	Modified_vertex_index = 0;

	med_extract_matrix_from_segment(sp, &mat);

	switch (dimension) {
		case XDIM:
			scale_free_verts(sp, &mat.rvec, WLEFT,   -amount);
			scale_free_verts(sp, &mat.rvec, WRIGHT,  +amount);
			break;
		case YDIM:
			scale_free_verts(sp, &mat.uvec, WBOTTOM, -amount);
			scale_free_verts(sp, &mat.uvec, WTOP,    +amount);
			break;
		case ZDIM:
			scale_free_verts(sp, &mat.fvec, WFRONT,  -amount);
			scale_free_verts(sp, &mat.fvec, WBACK,   +amount);
			break;
	}

	validate_modified_segments();
}

// ------------------------------------------------------------------------------------------
//	Extract a vector from a segment.  The vector goes from the start face to the end face.
//	The point on each face is the average of the four points forming the face.
void extract_vector_from_segment_side(segment *sp, int side, vms_vector *vp, int vla, int vlb, int vra, int vrb)
{
	vms_vector	v1, v2;

	vm_vec_sub(&v1,&Vertices[sp->verts[Side_to_verts[side][vra]]],&Vertices[sp->verts[Side_to_verts[side][vla]]]);
	vm_vec_sub(&v2,&Vertices[sp->verts[Side_to_verts[side][vrb]]],&Vertices[sp->verts[Side_to_verts[side][vlb]]]);
	vm_vec_add(vp, &v1, &v2);

	vm_vec_scale(vp, F1_0/2);
}

// ------------------------------------------------------------------------------------------
//	Extract the right vector from segment *sp, return in *vp.
//	The forward vector is defined to be the vector from the the center of the left face of the segment
// to the center of the right face of the segment.
void med_extract_right_vector_from_segment_side(segment *sp, int sidenum, vms_vector *vp)
{
	extract_vector_from_segment_side(sp, sidenum, vp, 3, 2, 0, 1);
}

// ------------------------------------------------------------------------------------------
//	Extract the up vector from segment *sp, return in *vp.
//	The forward vector is defined to be the vector from the the center of the bottom face of the segment
// to the center of the top face of the segment.
void med_extract_up_vector_from_segment_side(segment *sp, int sidenum, vms_vector *vp)
{
	extract_vector_from_segment_side(sp, sidenum, vp, 1, 2, 0, 3);
}


// -----------------------------------------------------------------------------
//	Increase the size of Cursegp in dimension dimension by amount
int segsize_common(int dimension, fix amount)
{
	int	i;
	int	propagated[MAX_SIDES_PER_SEGMENT];
	vms_vector	uvec, rvec, fvec, scalevec;

	Degenerate_segment_found = 0;

	med_scale_segment_new(Cursegp, dimension, amount);

	med_extract_up_vector_from_segment_side(Cursegp, Curside, &uvec);
	med_extract_right_vector_from_segment_side(Cursegp, Curside, &rvec);
	extract_forward_vector_from_segment(Cursegp, &fvec);

	scalevec.x = vm_vec_mag(&rvec);
	scalevec.y = vm_vec_mag(&uvec);
	scalevec.z = vm_vec_mag(&fvec);

	if (Degenerate_segment_found) {
		Degenerate_segment_found = 0;
		editor_status("Applying scale would create degenerate segments.  Aborting scale.");
		med_scale_segment_new(Cursegp, dimension, -amount);
		return 1;
	}

	med_create_new_segment(&scalevec);

	//	For all segments to which Cursegp is connected, propagate tmap (uv coordinates) from the connected
	//	segment back to Cursegp.  This will meaningfully propagate uv coordinates to all sides which havve
	//	an incident edge.  It will also do some sides more than once.  And it is probably just not what you want.
	for (i=0; i<MAX_SIDES_PER_SEGMENT; i++)
		propagated[i] = 0;

	for (i=0; i<MAX_SIDES_PER_SEGMENT; i++)
		if (IS_CHILD(Cursegp->children[i])) {
			int	s;
			for (s=0; s<MAX_SIDES_PER_SEGMENT; s++)
				propagated[s]++;
                        propagated[(int) Side_opposite[i]]--;
			med_propagate_tmaps_to_segments(&Segments[Cursegp->children[i]],Cursegp,1);
		}

	//	Now, for all sides that were not adjacent to another side, and therefore did not get tmaps
	//	propagated to them, treat as a back side.
	for (i=0; i<MAX_SIDES_PER_SEGMENT; i++)
		if (!propagated[i]) {
			med_propagate_tmaps_to_back_side(Cursegp, i, 1);
		}

	//	New stuff, assign default texture to all affected sides.

	Update_flags |= UF_WORLD_CHANGED;
	mine_changed = 1;
	return 1;
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

//	---------------------------------------------------------------------------
int	PerturbCursideCommon(fix amount)
{
	int			saveSegSizeMode = SegSizeMode;
	vms_vector	fvec, rvec, uvec;
	fix			fmag, rmag, umag;
	int			v;

	SegSizeMode = SEGSIZEMODE_CURSIDE;

	Modified_vertex_index = 0;

	extract_forward_vector_from_segment(Cursegp, &fvec);
	extract_right_vector_from_segment(Cursegp, &rvec);
	extract_up_vector_from_segment(Cursegp, &uvec);

	fmag = vm_vec_mag(&fvec);
	rmag = vm_vec_mag(&rvec);
	umag = vm_vec_mag(&uvec);

	for (v=0; v<4; v++) {
		vms_vector perturb_vec;

		perturb_vec.x = fixmul(rmag, d_rand()*2 - 32767);
		perturb_vec.y = fixmul(umag, d_rand()*2 - 32767);
		perturb_vec.z = fixmul(fmag, d_rand()*2 - 32767);

		scale_vert(Cursegp, Cursegp->verts[Side_to_verts[Curside][v]], &perturb_vec, amount);
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
