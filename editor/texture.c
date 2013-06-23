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
 * Texture map assignment.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include "inferno.h"
#include "segment.h"
#include "seguvs.h"
#include "editor.h"
#include "editor/esegment.h"
#include "fix.h"
#include "dxxerror.h"
#include "kdefs.h"

void compute_uv_side_center(uvl *uvcenter, segment *segp, int sidenum);
void rotate_uv_points_on_side(segment *segp, int sidenum, fix *rotmat, uvl *uvcenter);

//	-----------------------------------------------------------
int	TexFlipX()
{
	uvl	uvcenter;
	fix	rotmat[4];

	compute_uv_side_center(&uvcenter, Cursegp, Curside);

	//	Create a rotation matrix
	rotmat[0] = -0xffff;
	rotmat[1] = 0;
	rotmat[2] = 0;
	rotmat[3] = 0xffff;

	rotate_uv_points_on_side(Cursegp, Curside, rotmat, &uvcenter);

 	Update_flags |= UF_WORLD_CHANGED;

	return	1;
}

//	-----------------------------------------------------------
int	TexFlipY()
{
	uvl	uvcenter;
	fix	rotmat[4];

	compute_uv_side_center(&uvcenter, Cursegp, Curside);

	//	Create a rotation matrix
	rotmat[0] = 0xffff;
	rotmat[1] = 0;
	rotmat[2] = 0;
	rotmat[3] = -0xffff;

	rotate_uv_points_on_side(Cursegp, Curside, rotmat, &uvcenter);

 	Update_flags |= UF_WORLD_CHANGED;

	return	1;
}

//	-----------------------------------------------------------
int DoTexSlideLeft(int value)
{
	side	*sidep;
	uvl	duvl03;
	fix	dist;
	sbyte	*vp;
	int	v;

	vp = Side_to_verts[Curside];
	sidep = &Cursegp->sides[Curside];

	dist = vm_vec_dist(&Vertices[Cursegp->verts[vp[3]]], &Vertices[Cursegp->verts[vp[0]]]);
	dist *= value;
	if (dist < F1_0/(64*value))
		dist = F1_0/(64*value);

	duvl03.u = fixdiv(sidep->uvls[3].u - sidep->uvls[0].u,dist);
	duvl03.v = fixdiv(sidep->uvls[3].v - sidep->uvls[0].v,dist);

	for (v=0; v<4; v++) {
		sidep->uvls[v].u -= duvl03.u;
		sidep->uvls[v].v -= duvl03.v;
	}

	Update_flags |= UF_WORLD_CHANGED;

	return	1;
}

int TexSlideLeft()
{
	return DoTexSlideLeft(3);
}

int TexSlideLeftBig()
{
	return DoTexSlideLeft(1);
}

//	-----------------------------------------------------------
int DoTexSlideUp(int value)
{
	side	*sidep;
	uvl	duvl03;
	fix	dist;
	sbyte	*vp;
	int	v;

	vp = Side_to_verts[Curside];
	sidep = &Cursegp->sides[Curside];

	dist = vm_vec_dist(&Vertices[Cursegp->verts[vp[1]]], &Vertices[Cursegp->verts[vp[0]]]);
	dist *= value;

	if (dist < F1_0/(64*value))
		dist = F1_0/(64*value);

	duvl03.u = fixdiv(sidep->uvls[1].u - sidep->uvls[0].u,dist);
	duvl03.v = fixdiv(sidep->uvls[1].v - sidep->uvls[0].v,dist);

	for (v=0; v<4; v++) {
		sidep->uvls[v].u -= duvl03.u;
		sidep->uvls[v].v -= duvl03.v;
	}

	Update_flags |= UF_WORLD_CHANGED;

	return	1;
}

int TexSlideUp()
{
	return DoTexSlideUp(3);
}

int TexSlideUpBig()
{
	return DoTexSlideUp(1);
}


//	-----------------------------------------------------------
int DoTexSlideDown(int value)
{
	side	*sidep;
	uvl	duvl03;
	fix	dist;
	sbyte	*vp;
	int	v;

	vp = Side_to_verts[Curside];
	sidep = &Cursegp->sides[Curside];

	dist = vm_vec_dist(&Vertices[Cursegp->verts[vp[1]]], &Vertices[Cursegp->verts[vp[0]]]);
	dist *= value;
	if (dist < F1_0/(64*value))
		dist = F1_0/(64*value);

	duvl03.u = fixdiv(sidep->uvls[1].u - sidep->uvls[0].u,dist);
	duvl03.v = fixdiv(sidep->uvls[1].v - sidep->uvls[0].v,dist);

	for (v=0; v<4; v++) {
		sidep->uvls[v].u += duvl03.u;
		sidep->uvls[v].v += duvl03.v;
	}

	Update_flags |= UF_WORLD_CHANGED;

	return	1;
}

int TexSlideDown()
{
	return DoTexSlideDown(3);
}

int TexSlideDownBig()
{
	return DoTexSlideDown(1);
}

//	-----------------------------------------------------------
//	Compute the center of the side in u,v coordinates.
void compute_uv_side_center(uvl *uvcenter, segment *segp, int sidenum)
{
	int	v;
	side	*sidep = &segp->sides[sidenum];

	uvcenter->u = 0;
	uvcenter->v = 0;

	for (v=0; v<4; v++) {
		uvcenter->u += sidep->uvls[v].u;
		uvcenter->v += sidep->uvls[v].v;
	}

	uvcenter->u /= 4;
	uvcenter->v /= 4;
}


//	-----------------------------------------------------------
//	rotate point *uv by matrix rotmat, return *uvrot
void rotate_uv_point(uvl *uvrot, fix *rotmat, uvl *uv, uvl *uvcenter)
{
	uvrot->u = fixmul(uv->u - uvcenter->u,rotmat[0]) + fixmul(uv->v - uvcenter->v,rotmat[1]) + uvcenter->u;
	uvrot->v = fixmul(uv->u - uvcenter->u,rotmat[2]) + fixmul(uv->v - uvcenter->v,rotmat[3]) + uvcenter->v;
}

//	-----------------------------------------------------------
//	Compute the center of the side in u,v coordinates.
void rotate_uv_points_on_side(segment *segp, int sidenum, fix *rotmat, uvl *uvcenter)
{
	int	v;
	side	*sidep = &segp->sides[sidenum];
	uvl	tuv;

	for (v=0; v<4; v++) {
		rotate_uv_point(&tuv, rotmat, &sidep->uvls[v], uvcenter);
		sidep->uvls[v] = tuv;
	}
}

//	-----------------------------------------------------------
//	ang is in 0..ffff = 0..359.999 degrees
//	rotmat is filled in with 4 fixes
void create_2d_rotation_matrix(fix *rotmat, fix ang)
{
	fix	sinang, cosang;

	fix_sincos(ang, &sinang, &cosang);

	rotmat[0] = cosang;
	rotmat[1] = sinang;
	rotmat[2] = -sinang;
	rotmat[3] = cosang;
	
}


//	-----------------------------------------------------------
int DoTexRotateLeft(int value)
{
	uvl	uvcenter;
	fix	rotmat[4];

	compute_uv_side_center(&uvcenter, Cursegp, Curside);

	//	Create a rotation matrix
	create_2d_rotation_matrix(rotmat, -F1_0/value);

	rotate_uv_points_on_side(Cursegp, Curside, rotmat, &uvcenter);

 	Update_flags |= UF_WORLD_CHANGED;

	return	1;
}

int TexRotateLeft()
{
	return DoTexRotateLeft(192);
}

int TexRotateLeftBig()
{
	return DoTexRotateLeft(64);
}


//	-----------------------------------------------------------
int DoTexSlideRight(int value)
{
	side	*sidep;
	uvl	duvl03;
	fix	dist;
	sbyte	*vp;
	int	v;

	vp = Side_to_verts[Curside];
	sidep = &Cursegp->sides[Curside];

	dist = vm_vec_dist(&Vertices[Cursegp->verts[vp[3]]], &Vertices[Cursegp->verts[vp[0]]]);
	dist *= value;
	if (dist < F1_0/(64*value))
		dist = F1_0/(64*value);

	duvl03.u = fixdiv(sidep->uvls[3].u - sidep->uvls[0].u,dist);
	duvl03.v = fixdiv(sidep->uvls[3].v - sidep->uvls[0].v,dist);

	for (v=0; v<4; v++) {
		sidep->uvls[v].u += duvl03.u;
		sidep->uvls[v].v += duvl03.v;
	}

	Update_flags |= UF_WORLD_CHANGED;

	return	1;
}

int TexSlideRight()
{
	return DoTexSlideRight(3);
}

int TexSlideRightBig()
{
	return DoTexSlideRight(1);
}

//	-----------------------------------------------------------
int DoTexRotateRight(int value)
{
	uvl	uvcenter;
	fix	rotmat[4];

	compute_uv_side_center(&uvcenter, Cursegp, Curside);

	//	Create a rotation matrix
	create_2d_rotation_matrix(rotmat, F1_0/value);

	rotate_uv_points_on_side(Cursegp, Curside, rotmat, &uvcenter);

 	Update_flags |= UF_WORLD_CHANGED;

	return	1;
}

int TexRotateRight()
{
	return DoTexRotateRight(192);
}

int TexRotateRightBig()
{
	return DoTexRotateRight(64);
}

//	-----------------------------------------------------------
int	TexSelectActiveEdge()
{
	return	1;
}

//	-----------------------------------------------------------
int	TexRotate90Degrees()
{
	uvl	uvcenter;
	fix	rotmat[4];

	compute_uv_side_center(&uvcenter, Cursegp, Curside);

	//	Create a rotation matrix
	create_2d_rotation_matrix(rotmat, F1_0/4);

	rotate_uv_points_on_side(Cursegp, Curside, rotmat, &uvcenter);

 	Update_flags |= UF_WORLD_CHANGED;

	return	1;
}

//	-----------------------------------------------------------
int	TexSetDefault()
{
	Num_tilings = 1;

	Stretch_scale_x = F1_0;
	Stretch_scale_y = F1_0;

	assign_default_uvs_to_side(Cursegp,Curside);

	Update_flags |= UF_GAME_VIEW_CHANGED;
	return	1;
}

//	-----------------------------------------------------------
int	TexIncreaseTiling()
{

	Num_tilings++;
	assign_default_uvs_to_side(Cursegp, Curside);
	Update_flags |= UF_GAME_VIEW_CHANGED;

	return	1;
}

//	-----------------------------------------------------------
int	TexDecreaseTiling()
{

	if (--Num_tilings < 1)
		Num_tilings = 1;

	assign_default_uvs_to_side(Cursegp, Curside);
	Update_flags |= UF_GAME_VIEW_CHANGED;

	return	1;
}


//	direction = -1 or 1 depending on direction
int	TexStretchCommon(int direction)
{
	fix	*sptr;

	if ((Curedge == 0) || (Curedge == 2))
		sptr = &Stretch_scale_x;
	else
		sptr = &Stretch_scale_y;

	*sptr += direction*F1_0/64;

	if (*sptr < F1_0/16)
		*sptr = F1_0/16;

	if (*sptr > 2*F1_0)
		*sptr = 2*F1_0;

	stretch_uvs_from_curedge(Cursegp, Curside);

	editor_status_fmt("Stretch scale = %7.4f, use Set Default to return to 1.0", f2fl(*sptr));

	Update_flags |= UF_GAME_VIEW_CHANGED;
	return	1;

}

int	TexStretchDown(void)
{
	return TexStretchCommon(-1);

}

int	TexStretchUp(void)
{
	return TexStretchCommon(1);

}

