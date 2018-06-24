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
#include "maths.h"
#include "dxxerror.h"
#include "kdefs.h"

#include "compiler-range_for.h"

static uvl compute_uv_side_center(const vcsegptr_t segp, sidenum_fast_t sidenum);
static void rotate_uv_points_on_side(const vmsegptr_t segp, sidenum_fast_t sidenum, const array<fix, 4> &rotmat, const uvl &uvcenter);

//	-----------------------------------------------------------
int	TexFlipX()
{
	const auto uvcenter = compute_uv_side_center(Cursegp, Curside);
	array<fix, 4> rotmat;
	//	Create a rotation matrix
	rotmat[0] = -0xffff;
	rotmat[1] = 0;
	rotmat[2] = 0;
	rotmat[3] = 0xffff;

	rotate_uv_points_on_side(Cursegp, Curside, rotmat, uvcenter);

 	Update_flags |= UF_WORLD_CHANGED;

	return	1;
}

//	-----------------------------------------------------------
int	TexFlipY()
{
	const auto uvcenter = compute_uv_side_center(Cursegp, Curside);
	array<fix, 4> rotmat;
	//	Create a rotation matrix
	rotmat[0] = 0xffff;
	rotmat[1] = 0;
	rotmat[2] = 0;
	rotmat[3] = -0xffff;

	rotate_uv_points_on_side(Cursegp, Curside, rotmat, uvcenter);

 	Update_flags |= UF_WORLD_CHANGED;

	return	1;
}

//	-----------------------------------------------------------
static int DoTexSlideLeft(int value)
{
	uvl	duvl03;
	fix	dist;
	auto &vp = Side_to_verts[Curside];
	const auto sidep = &Cursegp->sides[Curside];

	dist = vm_vec_dist(vcvertptr(Cursegp->verts[vp[3]]), vcvertptr(Cursegp->verts[vp[0]]));
	dist *= value;
	if (dist < F1_0/(64*value))
		dist = F1_0/(64*value);

	duvl03.u = fixdiv(sidep->uvls[3].u - sidep->uvls[0].u,dist);
	duvl03.v = fixdiv(sidep->uvls[3].v - sidep->uvls[0].v,dist);

	range_for (auto &v, sidep->uvls)
	{
		v.u -= duvl03.u;
		v.v -= duvl03.v;
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
static int DoTexSlideUp(int value)
{
	uvl	duvl03;
	fix	dist;
	auto &vp = Side_to_verts[Curside];
	auto &uvls = Cursegp->sides[Curside].uvls;

	dist = vm_vec_dist(vcvertptr(Cursegp->verts[vp[1]]), vcvertptr(Cursegp->verts[vp[0]]));
	dist *= value;

	if (dist < F1_0/(64*value))
		dist = F1_0/(64*value);

	duvl03.u = fixdiv(uvls[1].u - uvls[0].u,dist);
	duvl03.v = fixdiv(uvls[1].v - uvls[0].v,dist);

	range_for (auto &v, uvls)
	{
		v.u -= duvl03.u;
		v.v -= duvl03.v;
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
static int DoTexSlideDown(int value)
{
	uvl	duvl03;
	fix	dist;
	auto &vp = Side_to_verts[Curside];
	auto &uvls = Cursegp->sides[Curside].uvls;

	dist = vm_vec_dist(vcvertptr(Cursegp->verts[vp[1]]), vcvertptr(Cursegp->verts[vp[0]]));
	dist *= value;
	if (dist < F1_0/(64*value))
		dist = F1_0/(64*value);

	duvl03.u = fixdiv(uvls[1].u - uvls[0].u,dist);
	duvl03.v = fixdiv(uvls[1].v - uvls[0].v,dist);

	range_for (auto &v, uvls)
	{
		v.u += duvl03.u;
		v.v += duvl03.v;
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
static uvl compute_uv_side_center(const vcsegptr_t segp, sidenum_fast_t sidenum)
{
	uvl uvcenter{};
	auto sidep = &segp->sides[sidenum];
	range_for (auto &v, sidep->uvls)
	{
		uvcenter.u += v.u;
		uvcenter.v += v.v;
	}
	uvcenter.u /= 4;
	uvcenter.v /= 4;
	return uvcenter;
}

//	-----------------------------------------------------------
//	rotate point *uv by matrix rotmat, return *uvrot
static uvl rotate_uv_point(const array<fix, 4> &rotmat, const uvl &uv, const uvl &uvcenter)
{
	const auto centered_u = uv.u - uvcenter.u;
	const auto centered_v = uv.v - uvcenter.v;
	return {
		fixmul(centered_u, rotmat[0]) + fixmul(centered_v, rotmat[1]) + uvcenter.u,
		fixmul(centered_u, rotmat[2]) + fixmul(centered_v, rotmat[3]) + uvcenter.v,
		0
	};
}

//	-----------------------------------------------------------
//	Compute the center of the side in u,v coordinates.
static void rotate_uv_points_on_side(const vmsegptr_t segp, sidenum_fast_t sidenum, const array<fix, 4> &rotmat, const uvl &uvcenter)
{
	range_for (auto &v, segp->sides[sidenum].uvls)
	{
		v = rotate_uv_point(rotmat, v, uvcenter);
	}
}

//	-----------------------------------------------------------
//	ang is in 0..ffff = 0..359.999 degrees
//	rotmat is filled in with 4 fixes
static array<fix, 4> create_2d_rotation_matrix(fix ang)
{
	const auto &&a = fix_sincos(ang);
	const auto &sinang = a.sin;
	const auto &cosang = a.cos;
	return {{
		cosang,
		sinang,
		-sinang,
		cosang
	}};
}


//	-----------------------------------------------------------
static int DoTexRotateLeft(int value)
{
	const auto uvcenter = compute_uv_side_center(Cursegp, Curside);
	//	Create a rotation matrix
	const auto rotmat = create_2d_rotation_matrix(-F1_0/value);

	rotate_uv_points_on_side(Cursegp, Curside, rotmat, uvcenter);

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
static int DoTexSlideRight(int value)
{
	uvl	duvl03;
	fix	dist;
	auto &vp = Side_to_verts[Curside];
	auto &uvls = Cursegp->sides[Curside].uvls;

	dist = vm_vec_dist(vcvertptr(Cursegp->verts[vp[3]]), vcvertptr(Cursegp->verts[vp[0]]));
	dist *= value;
	if (dist < F1_0/(64*value))
		dist = F1_0/(64*value);

	duvl03.u = fixdiv(uvls[3].u - uvls[0].u,dist);
	duvl03.v = fixdiv(uvls[3].v - uvls[0].v,dist);

	range_for (auto &v, uvls)
	{
		v.u += duvl03.u;
		v.v += duvl03.v;
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
static int DoTexRotateRight(int value)
{
	const auto uvcenter = compute_uv_side_center(Cursegp, Curside);

	//	Create a rotation matrix
	const auto rotmat = create_2d_rotation_matrix(F1_0/value);

	rotate_uv_points_on_side(Cursegp, Curside, rotmat, uvcenter);

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
	const auto uvcenter = compute_uv_side_center(Cursegp, Curside);

	//	Create a rotation matrix
	const auto rotmat = create_2d_rotation_matrix(F1_0/4);

	rotate_uv_points_on_side(Cursegp, Curside, rotmat, uvcenter);

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
static int	TexStretchCommon(int direction)
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

