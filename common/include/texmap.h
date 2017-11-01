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
 * $Source: /cvsroot/dxx-rebirth/d2x-rebirth/include/texmap.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 20:01:30 $
 *
 * Include file for entities using texture mapper library.
 *
 */

#pragma once

#include "maths.h"
#include "3d.h"

#define	NUM_LIGHTING_LEVELS 32
#define MAX_LIGHTING_VALUE	((NUM_LIGHTING_LEVELS-1)*F1_0/NUM_LIGHTING_LEVELS)
#define MIN_LIGHTING_VALUE	(F1_0/NUM_LIGHTING_LEVELS)

#ifdef __cplusplus
#include "dxxsconf.h"
#include "dsx-ns.h"
#include "compiler-array.h"

namespace dcx {

constexpr std::integral_constant<unsigned, 25> MAX_TMAP_VERTS{};

#if !DXX_USE_OGL
// -------------------------------------------------------------------------------------------------------
// This is the main texture mapper call.
//	tmap_num references a texture map defined in Texmap_ptrs.
//	nverts = number of vertices
//	vertbuf is a pointer to an array of vertex pointers
void draw_tmap(grs_canvas &, const grs_bitmap &bp, uint_fast32_t nverts, const g3s_point *const *vertbuf);

//function that takes the same parms as draw_tmap, but renders as flat poly
//we need this to do the cloaked effect
void draw_tmap_flat(grs_canvas &, const grs_bitmap &bp, uint_fast32_t nverts, const g3s_point *const *vertbuf);
#endif

// -------------------------------------------------------------------------------------------------------
// Texture map vertex.
//	The fields r,g,b and l are mutually exclusive.  r,g,b are used for rgb lighting.
//	l is used for intensity based lighting.
struct g3ds_vertex {
	fix	x,y,z;
	fix	u,v;
	fix	x2d,y2d;
	fix	l;
};

// A texture map is defined as a polygon with u,v coordinates associated with
// one point in the polygon, and a pair of vectors describing the orientation
// of the texture map in the world, from which the deltas Du_dx, Dv_dy, etc.
// are computed.
struct g3ds_tmap {
	int	nv;			// number of vertices
	array<g3ds_vertex, MAX_TMAP_VERTS> verts;	// up to 8 vertices, this is inefficient, change
};

// -------------------------------------------------------------------------------------------------------

//	Note:	Not all interpolation method and lighting combinations are supported.
//	Set Interpolation_method to 0/1/2 for linear/linear, perspective/linear, perspective/perspective
#if !DXX_USE_OGL
extern	int	Interpolation_method;
extern uint8_t Transparency_on;

// Set Lighting_on to 0/1/2 for no lighting/intensity lighting/rgb lighting
extern	int	Lighting_on;

// HACK INTERFACE: how far away the current segment (& thus texture) is
extern unsigned Current_seg_depth;
void init_interface_vars_to_assembler();
#endif
class push_interpolation_method
{
#if DXX_USE_OGL
public:
	push_interpolation_method(int, bool = true) {}
#else
	int previous;
public:
	push_interpolation_method(int next, bool condition = true) :
		previous(Interpolation_method)
	{
		if (condition)
			Interpolation_method = next;
	}
	~push_interpolation_method()
	{
		Interpolation_method = previous;
	}
#endif
};

//	These are pointers to texture maps.  If you want to render texture map #7, then you will render
//	the texture map defined by Texmap_ptrs[7].

extern int Window_clip_left, Window_clip_bot, Window_clip_right, Window_clip_top;

// for ugly hack put in to be sure we don't overflow render buffer

#define FIX_XLIMIT	(639 * F1_0)
#define FIX_YLIMIT	(479 * F1_0)

}

#endif
