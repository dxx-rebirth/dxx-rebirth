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
 * Rendering Stuff
 *
 */

#include <algorithm>
#include <bitset>
#include <limits>
#include <cstdlib>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "render_state.h"
#include "inferno.h"
#include "segment.h"
#include "dxxerror.h"
#include "bm.h"
#include "texmap.h"
#include "render.h"
#include "game.h"
#include "object.h"
#include "laser.h"
#include "textures.h"
#include "screens.h"
#include "segpoint.h"
#include "wall.h"
#include "texmerge.h"
#include "physics.h"
#include "3d.h"
#include "gameseg.h"
#include "vclip.h"
#include "lighting.h"
#include "cntrlcen.h"
#include "newdemo.h"
#include "automap.h"
#include "endlevel.h"
#include "key.h"
#include "newmenu.h"
#include "u_mem.h"
#include "piggy.h"
#include "timer.h"
#include "effects.h"
#include "playsave.h"
#if DXX_USE_OGL
#include "ogl_init.h"
#endif
#include "args.h"

#include "compiler-integer_sequence.h"
#include "compiler-range_for.h"
#include "partial_range.h"
#include "segiter.h"

#if DXX_USE_EDITOR
#include "editor/editor.h"
#include "editor/esegment.h"
#endif

using std::min;
using std::max;

// (former) "detail level" values
#if DXX_USE_OGL
int Render_depth = MAX_RENDER_SEGS; //how many segments deep to render
#else
int Render_depth = 20; //how many segments deep to render
unsigned Max_linear_depth = 50; // Deepest segment at which linear interpolation will be used.
#endif

//used for checking if points have been rotated
int	Clear_window_color=-1;
int	Clear_window=2;	// 1 = Clear whole background window, 2 = clear view portals into rest of world, 0 = no clear

static uint16_t s_current_generation;

// When any render function needs to know what's looking at it, it should 
// access Viewer members.
namespace dsx {
object * Viewer = NULL;
}

vms_vector Viewer_eye;  //valid during render

#if !DXX_USE_EDITOR && defined(RELEASE)
constexpr
#endif
fix Render_zoom = 0x9000;					//the player's zoom factor

#ifndef NDEBUG
static std::bitset<MAX_OBJECTS> object_rendered;
#endif

#if DXX_USE_EDITOR
int	Render_only_bottom=0;
int	Bottom_bitmap_num = 9;
#endif

namespace dcx {

//Global vars for window clip test
int Window_clip_left,Window_clip_top,Window_clip_right,Window_clip_bot;

}

#if DXX_USE_EDITOR
int _search_mode = 0;			//true if looking for curseg,side,face
short _search_x,_search_y;	//pixel we're looking at
static int found_side,found_face;
static segnum_t found_seg;
static objnum_t found_obj;
#else
constexpr int _search_mode = 0;
#endif

#ifdef NDEBUG		//if no debug code, set these vars to constants
#else

int Outline_mode=0;

int toggle_outline_mode(void)
{
	return Outline_mode = !Outline_mode;
}
#endif

#ifndef NDEBUG
#if DXX_USE_OGL
#define draw_outline(C,a,b)	draw_outline(a,b)
#endif
static void draw_outline(grs_canvas &canvas, const int nverts, cg3s_point *const *const pointlist)
{
	int i;

	const uint8_t color = BM_XRGB(63, 63, 63);

	for (i=0;i<nverts-1;i++)
		g3_draw_line(canvas, *pointlist[i], *pointlist[i + 1], color);
	g3_draw_line(canvas, *pointlist[i], *pointlist[0], color);
}
#endif

fix flash_scale;

#define FLASH_CYCLE_RATE f1_0

constexpr std::integral_constant<fix, FLASH_CYCLE_RATE> Flash_rate{};

//cycle the flashing light for when mine destroyed
namespace dsx {
void flash_frame()
{
	static fixang flash_ang=0;

	if (!Control_center_destroyed && !Seismic_tremor_magnitude)
		return;

	if (Endlevel_sequence)
		return;

	if (PaletteBlueAdd > 10 )		//whiting out
		return;

//	flash_ang += fixmul(FLASH_CYCLE_RATE,FrameTime);
#if defined(DXX_BUILD_DESCENT_II)
	if (Seismic_tremor_magnitude) {
		fix	added_flash;

		added_flash = abs(Seismic_tremor_magnitude);
		if (added_flash < F1_0)
			added_flash *= 16;

		flash_ang += fixmul(Flash_rate, fixmul(FrameTime, added_flash+F1_0));
		flash_scale = fix_fastsin(flash_ang);
		flash_scale = (flash_scale + F1_0*3)/4;	//	gets in range 0.5 to 1.0
	} else
#endif
	{
		flash_ang += fixmul(Flash_rate,FrameTime);
		flash_scale = fix_fastsin(flash_ang);
		flash_scale = (flash_scale + f1_0)/2;
#if defined(DXX_BUILD_DESCENT_II)
		if (Difficulty_level == 0)
			flash_scale = (flash_scale+F1_0*3)/4;
#endif
	}


}

static inline int is_alphablend_eclip(int eclip_num)
{
#if defined(DXX_BUILD_DESCENT_II)
	if (eclip_num == ECLIP_NUM_FORCE_FIELD)
		return 1;
#endif
	return eclip_num == ECLIP_NUM_FUELCEN;
}

// ----------------------------------------------------------------------------
//	Render a face.
//	It would be nice to not have to pass in segnum and sidenum, but
//	they are used for our hideously hacked in headlight system.
//	vp is a pointer to vertex ids.
//	tmap1, tmap2 are texture map ids.  tmap2 is the pasty one.
static void render_face(grs_canvas &canvas, const shared_segment &segp, const unsigned sidenum, const unsigned nv, const array<unsigned, 4> &vp, const unsigned tmap1, const unsigned tmap2, array<g3s_uvl, 4> uvl_copy, const WALL_IS_DOORWAY_result_t wid_flags)
{
	grs_bitmap  *bm;

	array<cg3s_point *, 4> pointlist;

	Assert(nv <= pointlist.size());

	for (uint_fast32_t i = 0; i < nv; i++) {
		pointlist[i] = &Segment_points[vp[i]];
	}

#if defined(DXX_BUILD_DESCENT_I)
	(void)segp;
	(void)wid_flags;
#if !DXX_USE_EDITOR
	(void)sidenum;
#endif
#elif defined(DXX_BUILD_DESCENT_II)
	//handle cloaked walls
	if (wid_flags & WID_CLOAKED_FLAG) {
		const auto wall_num = segp.sides[sidenum].wall_num;
		gr_settransblend(canvas, vcwallptr(wall_num)->cloak_value, GR_BLEND_NORMAL);
		const uint8_t color = BM_XRGB(0, 0, 0);
		// set to black (matters for s3)

		g3_draw_poly(canvas, nv, pointlist, color);    // draw as flat poly
		gr_settransblend(canvas, GR_FADE_OFF, GR_BLEND_NORMAL);

		return;
	}
#endif

	if (tmap1 >= NumTextures) {
		Int3();
	}

#if DXX_USE_OGL
	grs_bitmap *bm2 = nullptr;
	if (!CGameArg.DbgUseOldTextureMerge)
	{
		PIGGY_PAGE_IN(Textures[tmap1]);
		bm = &GameBitmaps[Textures[tmap1].index];
		if (tmap2){
			PIGGY_PAGE_IN(Textures[tmap2&0x3FFF]);
			bm2 = &GameBitmaps[Textures[tmap2&0x3FFF].index];
			if (bm2->get_flag_mask(BM_FLAG_SUPER_TRANSPARENT))
			{
				bm2 = nullptr;
			bm = &texmerge_get_cached_bitmap( tmap1, tmap2 );
			}
		}
	}else
#endif

		// New code for overlapping textures...
		if (tmap2 != 0) {
			bm = &texmerge_get_cached_bitmap( tmap1, tmap2 );
		} else {
			bm = &GameBitmaps[Textures[tmap1].index];
			PIGGY_PAGE_IN(Textures[tmap1]);
		}

	assert(!bm->get_flag_mask(BM_FLAG_PAGED_OUT));

	array<g3s_lrgb, 4>		dyn_light;
	const auto seismic_tremor_magnitude = Seismic_tremor_magnitude;
	const auto control_center_destroyed = Control_center_destroyed;
	const auto need_flashing_lights = (control_center_destroyed | seismic_tremor_magnitude);	//make lights flash
	//set light values for each vertex & build pointlist
	for (uint_fast32_t i = 0;i < nv;i++)
	{
		auto &dli = dyn_light[i];
		auto &uvli = uvl_copy[i];
		auto &Dlvpi = Dynamic_light[vp[i]];
		dli.r = dli.g = dli.b = uvli.l;
		//the uvl struct has static light already in it

		//scale static light for destruction effect
		if (need_flashing_lights)	//make lights flash
			uvli.l = fixmul(flash_scale, uvli.l);
		//add in dynamic light (from explosions, etc.)
		uvli.l += (Dlvpi.r + Dlvpi.g + Dlvpi.b) / 3;
		//saturate at max value
		if (uvli.l > MAX_LIGHT)
			uvli.l = MAX_LIGHT;

		// And now the same for the ACTUAL (rgb) light we want to use

		//scale static light for destruction effect
		if (need_flashing_lights)	//make lights flash
		{
			dli.g = dli.b = fixmul(flash_scale, uvli.l);
			dli.r = (!seismic_tremor_magnitude && PlayerCfg.DynLightColor)
				? fixmul(std::max(static_cast<double>(flash_scale), f0_5 * 1.5), uvli.l) // let the mine glow red a little
				: dli.g;
		}

		// add light color
		dli.r += Dlvpi.r;
		dli.g += Dlvpi.g;
		dli.b += Dlvpi.b;
		// saturate at max value
		if (dli.r > MAX_LIGHT)
			dli.r = MAX_LIGHT;
		if (dli.g > MAX_LIGHT)
			dli.g = MAX_LIGHT;
		if (dli.b > MAX_LIGHT)
			dli.b = MAX_LIGHT;
		if (PlayerCfg.AlphaEffects) // due to additive blending, transparent sprites will become invivible in font of white surfaces (lamps). Fix that with a little desaturation
		{
			dli.r *= .93;
			dli.g *= .93;
			dli.b *= .93;
		}
	}

	bool alpha = false;
	if (PlayerCfg.AlphaBlendEClips && is_alphablend_eclip(TmapInfo[tmap1].eclip_num)) // set nice transparency/blending for some special effects (if we do more, we should maybe use switch here)
	{
		alpha = true;
		gr_settransblend(canvas, GR_FADE_OFF, GR_BLEND_ADDITIVE_C);
	}

#if DXX_USE_EDITOR
	if ((Render_only_bottom) && (sidenum == WBOTTOM))
		g3_draw_tmap(canvas, nv, pointlist, uvl_copy, dyn_light, GameBitmaps[Textures[Bottom_bitmap_num].index]);
	else
#endif

#if DXX_USE_OGL
		if (bm2){
			g3_draw_tmap_2(canvas, nv, pointlist, uvl_copy, dyn_light, *bm, *bm2, ((tmap2 & 0xC000) >> 14) & 3);
		}else
#endif
			g3_draw_tmap(canvas, nv, pointlist, uvl_copy, dyn_light, *bm);

	if (alpha)
		gr_settransblend(canvas, GR_FADE_OFF, GR_BLEND_NORMAL); // revert any transparency / blending setting back to normal

#ifndef NDEBUG
	if (Outline_mode) draw_outline(canvas, nv, &pointlist[0]);
#endif
}
}

// ----------------------------------------------------------------------------
//	Only called if editor active.
//	Used to determine which face was clicked on.
static void check_face(grs_canvas &canvas, const vmsegidx_t segnum, const unsigned sidenum, const unsigned facenum, const unsigned nv, const array<unsigned, 4> &vp, const unsigned tmap1, const unsigned tmap2, const array<g3s_uvl, 4> &uvl_copy)
{
#if DXX_USE_EDITOR
	if (_search_mode) {
		array<g3s_lrgb, 4> dyn_light{};
		array<cg3s_point *, 4> pointlist;
#if DXX_USE_OGL
		(void)tmap1;
		(void)tmap2;
#else
		grs_bitmap *bm;
		if (tmap2 > 0 )
			bm = &texmerge_get_cached_bitmap( tmap1, tmap2 );
		else
			bm = &GameBitmaps[Textures[tmap1].index];
#endif
		for (uint_fast32_t i = 0; i < nv; i++) {
			dyn_light[i].r = dyn_light[i].g = dyn_light[i].b = uvl_copy[i].l;
			pointlist[i] = &Segment_points[vp[i]];
		}

#if DXX_USE_OGL
		ogl_end_frame();
#endif
		{
		uint8_t color = 0;
			gr_pixel(canvas.cv_bitmap, _search_x, _search_y, color);	//set our search pixel to color zero
		}
#if DXX_USE_OGL
		ogl_start_frame(canvas);
#endif
		{
#if DXX_USE_OGL
			const uint8_t color = 1;
			g3_draw_poly(canvas, nv, pointlist, color);
#else
			const auto save_lighting = Lighting_on;
			Lighting_on = 2;
			g3_draw_tmap(canvas, nv, pointlist, uvl_copy, dyn_light, *bm);
			Lighting_on = save_lighting;
#endif
		}

		if (gr_ugpixel(canvas.cv_bitmap,_search_x,_search_y) == 1) {
			found_seg = segnum;
			found_obj = object_none;
			found_side = sidenum;
			found_face = facenum;
		}
	}
#else
	(void)canvas;
	(void)segnum;
	(void)sidenum;
	(void)facenum;
	(void)nv;
	(void)vp;
	(void)tmap1;
	(void)tmap2;
	(void)uvl_copy;
#endif
}

template <std::size_t... N>
static inline void check_render_face(grs_canvas &canvas, index_sequence<N...>, const vcsegptridx_t segnum, const unsigned sidenum, const unsigned facenum, const array<unsigned, 4> &ovp, const unsigned tmap1, const unsigned tmap2, const array<uvl, 4> &uvlp, const WALL_IS_DOORWAY_result_t wid_flags, const std::size_t nv)
{
	const array<unsigned, 4> vp{{ovp[N]...}};
	const array<g3s_uvl, 4> uvl_copy{{
		{uvlp[N].u, uvlp[N].v, uvlp[N].l}...
	}};
	render_face(canvas, segnum, sidenum, nv, vp, tmap1, tmap2, uvl_copy, wid_flags);
	check_face(canvas, segnum, sidenum, facenum, nv, vp, tmap1, tmap2, uvl_copy);
}

template <std::size_t N0, std::size_t N1, std::size_t N2, std::size_t N3>
static inline void check_render_face(grs_canvas &canvas, index_sequence<N0, N1, N2, N3> is, const vcsegptridx_t segnum, const unsigned sidenum, const unsigned facenum, const array<unsigned, 4> &vp, const unsigned tmap1, const unsigned tmap2, const array<uvl, 4> &uvlp, const WALL_IS_DOORWAY_result_t wid_flags)
{
	check_render_face(canvas, is, segnum, sidenum, facenum, vp, tmap1, tmap2, uvlp, wid_flags, 4);
}

/* Avoid default constructing final element of uvl_copy; if any members
 * are default constructed, gcc zero initializes all members.
 */
template <std::size_t N0, std::size_t N1, std::size_t N2>
static inline void check_render_face(grs_canvas &canvas, index_sequence<N0, N1, N2>, const vcsegptridx_t segnum, const unsigned sidenum, const unsigned facenum, const array<unsigned, 4> &vp, const unsigned tmap1, const unsigned tmap2, const array<uvl, 4> &uvlp, const WALL_IS_DOORWAY_result_t wid_flags)
{
	check_render_face(canvas, index_sequence<N0, N1, N2, 3>(), segnum, sidenum, facenum, vp, tmap1, tmap2, uvlp, wid_flags, 3);
}

constexpr std::integral_constant<fix, (F1_0/4)> Tulate_min_dot{};
//--unused-- fix	Tulate_min_ratio = (2*F1_0);
constexpr std::integral_constant<fix, (F1_0*15/16)> Min_n0_n1_dot{};

// -----------------------------------------------------------------------------------
//	Render a side.
//	Check for normal facing.  If so, render faces on side dictated by sidep->type.
namespace dsx {
static void render_side(fvcvertptr &vcvertptr, grs_canvas &canvas, const vcsegptridx_t segp, const unsigned sidenum, const WALL_IS_DOORWAY_result_t wid_flags, const vms_vector &Viewer_eye)
{
	fix		min_dot, max_dot;

	if (!(wid_flags & WID_RENDER_FLAG))		//if (WALL_IS_DOORWAY(segp, sidenum) == WID_NO_WALL)
		return;

	const auto vertnum_list = get_side_verts(segp,sidenum);

	//	Regardless of whether this side is comprised of a single quad, or two triangles, we need to know one normal, so
	//	deal with it, get the dot product.
	const auto sidep = &segp->sides[sidenum];
	const unsigned which_vertnum =
		(sidep->get_type() == SIDE_IS_TRI_13)
			? 1
			: 0;
	const auto tvec = vm_vec_normalized_quick(vm_vec_sub(Viewer_eye, vcvertptr(vertnum_list[which_vertnum])));
	auto &normals = sidep->normals;
	const auto v_dot_n0 = vm_vec_dot(tvec, normals[0]);
	//	========== Mark: Here is the change...beginning here: ==========

	index_sequence<0, 1, 2, 3> is_quad;
	if (sidep->get_type() == SIDE_IS_QUAD) {

		if (v_dot_n0 >= 0) {
			check_render_face(canvas, is_quad, segp, sidenum, 0, vertnum_list, sidep->tmap_num, sidep->tmap_num2, sidep->uvls, wid_flags);
		}
	} else {
		//	========== Mark: The change ends here. ==========

		//	Although this side has been triangulated, because it is not planar, see if it is acceptable
		//	to render it as a single quadrilateral.  This is a function of how far away the viewer is, how non-planar
		//	the face is, how normal to the surfaces the view is.
		//	Now, if both dot products are close to 1.0, then render two triangles as a single quad.
		const auto v_dot_n1 = vm_vec_dot(tvec, normals[1]);

		if (v_dot_n0 < v_dot_n1) {
			min_dot = v_dot_n0;
			max_dot = v_dot_n1;
		} else {
			min_dot = v_dot_n1;
			max_dot = v_dot_n0;
		}

		//	Determine whether to detriangulate side: (speed hack, assumes Tulate_min_ratio == F1_0*2, should fixmul(min_dot, Tulate_min_ratio))
		if (DETRIANGULATION && ((min_dot+F1_0/256 > max_dot) || ((Viewer->segnum != segp) &&  (min_dot > Tulate_min_dot) && (max_dot < min_dot*2)))) {
			fix	n0_dot_n1;

			//	The other detriangulation code doesn't deal well with badly non-planar sides.
			n0_dot_n1 = vm_vec_dot(normals[0], normals[1]);
			if (n0_dot_n1 < Min_n0_n1_dot)
				goto im_so_ashamed;

			check_render_face(canvas, is_quad, segp, sidenum, 0, vertnum_list, sidep->tmap_num, sidep->tmap_num2, sidep->uvls, wid_flags);
		} else {
im_so_ashamed: ;
			if (sidep->get_type() == SIDE_IS_TRI_02) {
				if (v_dot_n0 >= 0) {
					check_render_face(canvas, index_sequence<0, 1, 2>(), segp, sidenum, 0, vertnum_list, sidep->tmap_num, sidep->tmap_num2, sidep->uvls, wid_flags);
				}

				if (v_dot_n1 >= 0) {
					// want to render from vertices 0, 2, 3 on side
					check_render_face(canvas, index_sequence<0, 2, 3>(), segp, sidenum, 1, vertnum_list, sidep->tmap_num, sidep->tmap_num2, sidep->uvls, wid_flags);
				}
			} else if (sidep->get_type() ==  SIDE_IS_TRI_13) {
				if (v_dot_n1 >= 0) {
					// rendering 1,2,3, so just skip 0
					check_render_face(canvas, index_sequence<1, 2, 3>(), segp, sidenum, 1, vertnum_list, sidep->tmap_num, sidep->tmap_num2, sidep->uvls, wid_flags);
				}

				if (v_dot_n0 >= 0) {
					// want to render from vertices 0,1,3
					check_render_face(canvas, index_sequence<0, 1, 3>(), segp, sidenum, 0, vertnum_list, sidep->tmap_num, sidep->tmap_num2, sidep->uvls, wid_flags);
				}

			} else
				throw side::illegal_type(segp, *sidep);
		}
	}

}
}

#if DXX_USE_EDITOR
static void render_object_search(grs_canvas &canvas, const vmobjptridx_t obj)
{
	int changed=0;

	//note that we draw each pixel object twice, since we cannot control
	//what color the object draws in, so we try color 0, then color 1,
	//in case the object itself is rendering color 0

	{
	const uint8_t color = 0;
	//set our search pixel to color zero
#if DXX_USE_OGL
	ogl_end_frame();

	// For OpenGL we use gr_rect instead of gr_pixel,
	// because in some implementations (like my Macbook Pro 5,1)
	// point smoothing can't be turned off.
	// Point smoothing would change the pixel to dark grey, but it MUST be black.
	// Making a 3x3 rectangle wouldn't matter
	// (but it only seems to draw a single pixel anyway)
	gr_rect(canvas, _search_x - 1, _search_y - 1, _search_x + 1, _search_y + 1, color);

	ogl_start_frame(canvas);
#else
	gr_pixel(canvas.cv_bitmap, _search_x, _search_y, color);
#endif
	}
	render_object(canvas, obj);
	if (gr_ugpixel(canvas.cv_bitmap,_search_x,_search_y) != 0)
		changed=1;

	{
		const uint8_t color = 1;
#if DXX_USE_OGL
	ogl_end_frame();
	gr_rect(canvas, _search_x - 1, _search_y - 1, _search_x + 1, _search_y + 1, color);
	ogl_start_frame(canvas);
#else
	gr_pixel(canvas.cv_bitmap, _search_x, _search_y, color);
#endif
	}
	render_object(canvas, obj);
	if (gr_ugpixel(canvas.cv_bitmap,_search_x,_search_y) != 1)
		changed=1;

	if (changed) {
		if (obj->segnum != segment_none)
			Cursegp = imsegptridx(obj->segnum);
		found_seg = segment_none;
		found_obj = obj;
	}
}
#endif

namespace dsx {
static void do_render_object(grs_canvas &canvas, const vmobjptridx_t obj, window_rendered_data &window)
{
#if DXX_USE_EDITOR
	int save_3d_outline=0;
	#endif
	int count = 0;

	#ifndef NDEBUG
	if (object_rendered[obj]) {		//already rendered this...
		Int3();		//get Matt!!!
		return;
	}

	object_rendered[obj] = true;
	#endif

#if defined(DXX_BUILD_DESCENT_II)
   if (Newdemo_state==ND_STATE_PLAYBACK)  
	 {
	  if ((DemoDoingLeft==6 || DemoDoingRight==6) && obj->type==OBJ_PLAYER)
		{
			// A nice fat hack: keeps the player ship from showing up in the
			// small extra view when guiding a missile in the big window
			
  			return; 
		}
	 }
#endif

	//	Added by MK on 09/07/94 (at about 5:28 pm, CDT, on a beautiful, sunny late summer day!) so
	//	that the guided missile system will know what objects to look at.
	//	I didn't know we had guided missiles before the release of D1. --MK
	if (obj->type == OBJ_ROBOT)
		window.rendered_robots.emplace_back(obj);

	if ((count++ > MAX_OBJECTS) || (obj->next == obj)) {
		Int3();					// infinite loop detected
		obj->next = object_none;		// won't this clean things up?
		return;					// get out of this infinite loop!
	}

		//g3_draw_object(obj->class_id,&obj->pos,&obj->orient,obj->size);

	//check for editor object

#if DXX_USE_EDITOR
	if (EditorWindow && obj==Cur_object_index) {
		save_3d_outline = g3d_interp_outline;
		g3d_interp_outline=1;
	}
	#endif

#if DXX_USE_EDITOR
	if (_search_mode)
		render_object_search(canvas, obj);
	else
	#endif
		//NOTE LINK TO ABOVE
		render_object(canvas, obj);

	for (auto n = obj->attached_obj; n != object_none;)
	{
		const auto &&o = obj.absolute_sibling(n);
		Assert(o->type == OBJ_FIREBALL);
		Assert(o->control_type == CT_EXPLOSION);
		Assert(o->flags & OF_ATTACHED);
		n = o->ctype.expl_info.next_attach;

		render_object(canvas, o);
	}


#if DXX_USE_EDITOR
	if (EditorWindow && obj==Cur_object_index)
		g3d_interp_outline = save_3d_outline;
	#endif


}
}

//increment counter for checking if points rotated
//This must be called at the start of the frame if rotate_list() will be used
void render_start_frame()
{
	if (s_current_generation == std::numeric_limits<decltype(s_current_generation)>::max())
	{
		Segment_points = {};
		s_current_generation = 0;
	}
	++ s_current_generation;
}

//Given a lit of point numbers, rotate any that haven't been rotated this frame
g3s_codes rotate_list(const std::size_t nv, const unsigned *const pointnumlist)
{
	g3s_codes cc;
	const auto current_generation = s_current_generation;
	const auto cheats_acid = cheats.acid;
	float f = likely(!cheats_acid)
		? 0.0f /* unused */
		: 2.0f * (static_cast<float>(timer_query()) / F1_0);

	range_for (const auto pnum, unchecked_partial_range(pointnumlist, nv))
	{
		auto &pnt = Segment_points[pnum];
		if (pnt.p3_last_generation != current_generation)
		{
			pnt.p3_last_generation = current_generation;
			auto &v = *vcvertptr(pnum);
			vertex tmpv;
			g3_rotate_point(pnt, likely(!cheats_acid) ? v : (
				tmpv = v,
				tmpv.x += fl2f(sinf(f + f2fl(tmpv.x))),
				tmpv.y += fl2f(sinf(f * 1.5f + f2fl(tmpv.y))),
				tmpv.z += fl2f(sinf(f * 2.5f + f2fl(tmpv.z))),
				tmpv
			));
		}
		cc.uand &= pnt.p3_codes;
		cc.uor  |= pnt.p3_codes;
	}

	return cc;

}

//Given a lit of point numbers, project any that haven't been projected
static void project_list(const array<unsigned, 8> &pointnumlist)
{
	range_for (const auto pnum, pointnumlist)
	{
		auto &p = Segment_points[pnum];
		if (!(p.p3_flags & PF_PROJECTED))
			g3_project_point(p);
	}
}


// -----------------------------------------------------------------------------------
#if !DXX_USE_OGL
namespace dsx {
static void render_segment(grs_canvas &canvas, const vcsegptridx_t seg)
{
	int			sn;

	if (!rotate_list(seg->verts).uand)
	{		//all off screen?

#if defined(DXX_BUILD_DESCENT_II)
		if (Viewer->type != OBJ_ROBOT)
#endif
		{
		Automap_visited[seg]=1;
		}

		for (sn=0; sn<MAX_SIDES_PER_SEGMENT; sn++)
			render_side(vcvertptr, canvas, seg, sn, WALL_IS_DOORWAY(GameBitmaps, Textures, vcwallptr, seg, seg, sn), Viewer_eye);
	}

	//draw any objects that happen to be in this segment

	//sort objects!
	//object_sort_segment_objects( seg );
}
}
#endif

#if DXX_USE_EDITOR
#ifndef NDEBUG

constexpr fix CROSS_WIDTH = i2f(8);
constexpr fix CROSS_HEIGHT = i2f(8);

//draw outline for curside
static void outline_seg_side(grs_canvas &canvas, const shared_segment &seg, const unsigned _side, const unsigned edge, const unsigned vert)
{
	auto &verts = seg.verts;
	if (!rotate_list(verts).uand)
	{		//all off screen?
		g3s_point *pnt;

		//render curedge of curside of curseg in green

		const uint8_t color = BM_XRGB(0, 63, 0);
		auto &sv = Side_to_verts[_side];
		g3_draw_line(canvas, Segment_points[verts[sv[edge]]], Segment_points[verts[sv[(edge + 1)%4]]], color);

		//draw a little cross at the current vert

		pnt = &Segment_points[verts[Side_to_verts[_side][vert]]];

		g3_project_point(*pnt);		//make sure projected

//		gr_line(pnt->p3_sx-CROSS_WIDTH,pnt->p3_sy,pnt->p3_sx+CROSS_WIDTH,pnt->p3_sy);
//		gr_line(pnt->p3_sx,pnt->p3_sy-CROSS_HEIGHT,pnt->p3_sx,pnt->p3_sy+CROSS_HEIGHT);

		gr_line(canvas, pnt->p3_sx - CROSS_WIDTH, pnt->p3_sy, pnt->p3_sx, pnt->p3_sy - CROSS_HEIGHT, color);
		gr_line(canvas, pnt->p3_sx, pnt->p3_sy - CROSS_HEIGHT, pnt->p3_sx + CROSS_WIDTH, pnt->p3_sy, color);
		gr_line(canvas, pnt->p3_sx + CROSS_WIDTH, pnt->p3_sy, pnt->p3_sx, pnt->p3_sy + CROSS_HEIGHT, color);
		gr_line(canvas, pnt->p3_sx, pnt->p3_sy + CROSS_HEIGHT, pnt->p3_sx - CROSS_WIDTH, pnt->p3_sy, color);
	}
}

#endif
#endif

static ubyte code_window_point(fix x,fix y,const rect &w)
{
	ubyte code=0;

	if (x <= w.left)  code |= 1;
	if (x >= w.right) code |= 2;

	if (y <= w.top) code |= 4;
	if (y >= w.bot) code |= 8;

	return code;
}

#ifndef NDEBUG
static array<char, MAX_SEGMENTS> visited2;
#endif

//Given two sides of segment, tell the two verts which form the 
//edge between them
constexpr array<
	array<
		array<int_fast8_t, 2>,
		6>,
	6> Two_sides_to_edge = {{
	{{  {{edge_none,edge_none}},	 {{3,7}},	 {{edge_none,edge_none}},	 {{2,6}},	 {{6,7}},	 {{2,3}}	}},
	{{  {{3,7}},	 {{edge_none,edge_none}},	 {{0,4}},	 {{edge_none,edge_none}},	 {{4,7}},	 {{0,3}}	}},
	{{  {{edge_none,edge_none}},	 {{0,4}},	 {{edge_none,edge_none}},	 {{1,5}},	 {{4,5}},	 {{0,1}}	}},
	{{  {{2,6}},	 {{edge_none,edge_none}},	 {{1,5}},	 {{edge_none,edge_none}},	 {{5,6}},	 {{1,2}}	}},
	{{  {{6,7}},	 {{4,7}},	 {{4,5}},	 {{5,6}},	 {{edge_none,edge_none}},	 {{edge_none,edge_none}}	}},
	{{  {{2,3}},	 {{0,3}},	 {{0,1}},	 {{1,2}},	 {{edge_none,edge_none}},	 {{edge_none,edge_none}}	}}
}};

//given an edge specified by two verts, give the two sides on that edge
constexpr array<
	array<
		array<int_fast8_t, 2>,
		8>,
	8> Edge_to_sides = {{
	{{  {{side_none,side_none}},	 {{2,5}},	 {{side_none,side_none}},	 {{1,5}},	 {{1,2}},	 {{side_none,side_none}},	 {{side_none,side_none}},	 {{side_none,side_none}}	}},
	{{  {{2,5}},	 {{side_none,side_none}},	 {{3,5}},	 {{side_none,side_none}},	 {{side_none,side_none}},	 {{2,3}},	 {{side_none,side_none}},	 {{side_none,side_none}}	}},
	{{  {{side_none,side_none}},	 {{3,5}},	 {{side_none,side_none}},	 {{0,5}},	 {{side_none,side_none}},	 {{side_none,side_none}},	 {{0,3}},	 {{side_none,side_none}}	}},
	{{  {{1,5}},	 {{side_none,side_none}},	 {{0,5}},	 {{side_none,side_none}},	 {{side_none,side_none}},	 {{side_none,side_none}},	 {{side_none,side_none}},	 {{0,1}}	}},
	{{  {{1,2}},	 {{side_none,side_none}},	 {{side_none,side_none}},	 {{side_none,side_none}},	 {{side_none,side_none}},	 {{2,4}},	 {{side_none,side_none}},	 {{1,4}}	}},
	{{  {{side_none,side_none}},	 {{2,3}},	 {{side_none,side_none}},	 {{side_none,side_none}},	 {{2,4}},	 {{side_none,side_none}},	 {{3,4}},	 {{side_none,side_none}}	}},
	{{  {{side_none,side_none}},	 {{side_none,side_none}},	 {{0,3}},	 {{side_none,side_none}},	 {{side_none,side_none}},	 {{3,4}},	 {{side_none,side_none}},	 {{0,4}}	}},
	{{  {{side_none,side_none}},	 {{side_none,side_none}},	 {{side_none,side_none}},	 {{0,1}},	 {{1,4}},	 {{side_none,side_none}},	 {{0,4}},	 {{side_none,side_none}}	}},
}};

//@@//perform simple check on tables
//@@check_check()
//@@{
//@@	int i,j;
//@@
//@@	for (i=0;i<8;i++)
//@@		for (j=0;j<8;j++)
//@@			Assert(Edge_to_sides[i][j][0] == Edge_to_sides[j][i][0] && 
//@@					Edge_to_sides[i][j][1] == Edge_to_sides[j][i][1]);
//@@
//@@	for (i=0;i<6;i++)
//@@		for (j=0;j<6;j++)
//@@			Assert(Two_sides_to_edge[i][j][0] == Two_sides_to_edge[j][i][0] && 
//@@					Two_sides_to_edge[i][j][1] == Two_sides_to_edge[j][i][1]);
//@@
//@@
//@@}


//given an edge, tell what side is on that edge
__attribute_warn_unused_result
static int find_seg_side(const vcsegptr_t seg, const array<unsigned, 2> &verts, const unsigned notside)
{
	if (notside >= MAX_SIDES_PER_SEGMENT)
		throw std::logic_error("invalid notside");

	const auto v0 = verts[0];
	const auto v1 = verts[1];

	auto b = begin(seg->verts);
	auto e = end(seg->verts);
	auto iv0 = e;
	auto iv1 = e;
	for (auto i = b;;)
	{
		if (iv0 == e && *i == v0)
		{
			iv0 = i;
			if (iv1 != e)
				break;
		}
		if (iv1 == e && *i == v1)
		{
			iv1 = i;
			if (iv0 != e)
				break;
		}
		if (++i == e)
			return side_none;
	}

	const auto &eptr = Edge_to_sides[std::distance(b, iv0)][std::distance(b, iv1)];

	const auto side0 = eptr[0];
	const auto side1 = eptr[1];

	Assert(side0 != side_none && side1 != side_none);

	if (side0 != notside) {
		Assert(side1==notside);
		return side0;
	}
	else {
		Assert(side0==notside);
		return side1;
	}

}

__attribute_warn_unused_result
static bool compare_child(const vcsegptridx_t seg, const vcsegptridx_t cseg, const sidenum_fast_t edgeside)
{
	const auto &cside = cseg->sides[edgeside];
	const auto &sv = Side_to_verts[edgeside][cside.get_type() == SIDE_IS_TRI_13 ? 1 : 0];
	const auto &temp = vm_vec_sub(Viewer_eye, vcvertptr(seg->verts[sv]));
	const auto &cnormal = cseg->sides[edgeside].normals;
	return vm_vec_dot(cnormal[0], temp) < 0 || vm_vec_dot(cnormal[1], temp) < 0;
}

//see if the order matters for these two children.
//returns 0 if order doesn't matter, 1 if c0 before c1, -1 if c1 before c0
__attribute_warn_unused_result
static bool compare_children(const vcsegptridx_t seg, sidenum_fast_t s0, sidenum_fast_t s1)
{
	Assert(s0 != side_none && s1 != side_none);

	if (s0 == s1)
		return false;
	if (Side_opposite[s0] == s1)
		return false;
	//find normals of adjoining sides
	const array<unsigned, 2> edge_verts = {
		{seg->verts[Two_sides_to_edge[s0][s1][0]], seg->verts[Two_sides_to_edge[s0][s1][1]]}
	};
	if (edge_verts[0] == -1 || edge_verts[1] == -1)
		throw std::logic_error("invalid edge vert");
	const auto &&seg0 = seg.absolute_sibling(seg->children[s0]);
	auto edgeside0 = find_seg_side(seg0,edge_verts,find_connect_side(seg,seg0));
	if (edgeside0 == side_none)
		return false;
	auto r0 = compare_child(seg, seg0, edgeside0);
	if (!r0)
		return r0;
	const auto &&seg1 = seg.absolute_sibling(seg->children[s1]);
	auto edgeside1 = find_seg_side(seg1,edge_verts,find_connect_side(seg,seg1));
	if (edgeside1 == side_none)
		return false;
	return !compare_child(seg, seg1, edgeside1);
}

//short the children of segment to render in the correct order
//returns non-zero if swaps were made
typedef array<sidenum_fast_t, MAX_SIDES_PER_SEGMENT> sort_child_array_t;
static void sort_seg_children(const vcsegptridx_t seg, const partial_range_t<sort_child_array_t::iterator> &r)
{
	//for each child,  compare with other children and see if order matters
	//if order matters, fix if wrong
	auto predicate = [seg](sidenum_fast_t a, sidenum_fast_t b)
	{
		return compare_children(seg, a, b);
	};
		std::sort(r.begin(), r.end(), predicate);
}

static void add_obj_to_seglist(render_state_t &rstate, objnum_t objnum, segnum_t segnum)
{
	auto p = rstate.render_seg_map.emplace(segnum, render_state_t::per_segment_state_t{});
	auto &o = p.first->second.objects;
	if (p.second)
		o.reserve(16);
	o.emplace_back(render_state_t::per_segment_state_t::distant_object{objnum});
}

namespace {

using visited_twobit_array_t = visited_segment_mask_t<2>;

class render_compare_context_t
{
	typedef render_state_t::per_segment_state_t::distant_object distant_object;
	struct element
	{
		fix64 dist_squared;
#if defined(DXX_BUILD_DESCENT_II)
		object *objp;
#endif
	};
	typedef array<element, MAX_OBJECTS> array_t;
	array_t m_array;
public:
	array_t::reference operator[](std::size_t i) { return m_array[i]; }
	array_t::const_reference operator[](std::size_t i) const { return m_array[i]; }
	render_compare_context_t(const render_state_t::per_segment_state_t &segstate)
	{
		range_for (const auto t, segstate.objects)
		{
			auto &objp = *vmobjptr(t.objnum);
			auto &e = (*this)[t.objnum];
#if defined(DXX_BUILD_DESCENT_II)
			e.objp = &objp;
#endif
			e.dist_squared = vm_vec_dist2(objp.pos, Viewer_eye);
		}
	}
	bool operator()(const distant_object &a, const distant_object &b) const;
};

//compare function for object sort. 
bool render_compare_context_t::operator()(const distant_object &a, const distant_object &b) const
{
	const auto delta_dist_squared = (*this)[a.objnum].dist_squared - (*this)[b.objnum].dist_squared;

#if defined(DXX_BUILD_DESCENT_II)
	const auto obj_a = (*this)[a.objnum].objp;
	const auto obj_b = (*this)[b.objnum].objp;

	auto abs_delta_dist_squared = std::abs(delta_dist_squared);
	fix combined_size = obj_a->size + obj_b->size;
	/*
	 * First check without squaring.  If true, the square can be
	 * skipped.
	 */
	if (abs_delta_dist_squared < combined_size || abs_delta_dist_squared < (static_cast<fix64>(combined_size) * combined_size))
	{		//same position

		//these two objects are in the same position.  see if one is a fireball
		//or laser or something that should plot on top.  Don't do this for
		//the afterburner blobs, though.

		if (obj_a->type == OBJ_WEAPON || (obj_a->type == OBJ_FIREBALL && get_fireball_id(*obj_a) != VCLIP_AFTERBURNER_BLOB))
		{
			if (!(obj_b->type == OBJ_WEAPON || obj_b->type == OBJ_FIREBALL))
				return true;	//a is weapon, b is not, so say a is closer
			//both are weapons 
		}
		else
		{
			if (obj_b->type == OBJ_WEAPON || (obj_b->type == OBJ_FIREBALL && get_fireball_id(*obj_b) != VCLIP_AFTERBURNER_BLOB))
				return false;	//b is weapon, a is not, so say a is farther
		}

		//no special case, fall through to normal return
	}
#endif
	return delta_dist_squared > 0;	//return distance
}

}

static void sort_segment_object_list(render_state_t::per_segment_state_t &segstate)
{
	render_compare_context_t context(segstate);
	auto &v = segstate.objects;
	std::sort(v.begin(), v.end(), std::cref(context));
}

namespace dsx {
static void build_object_lists(render_state_t &rstate)
{
	int nn;
	const auto viewer = Viewer;
	for (nn=0;nn < rstate.N_render_segs;nn++) {
		const auto segnum = rstate.Render_list[nn];
		if (segnum != segment_none) {
			range_for (const auto obj, objects_in(vcsegptr(segnum), vcobjptridx, vcsegptr))
			{
				int list_pos;
				if (obj->type == OBJ_NONE)
				{
					assert(obj->type != OBJ_NONE);
					continue;
				}
				if (unlikely(obj == viewer) && likely(obj->attached_obj == object_none))
					continue;
				if (obj->flags & OF_ATTACHED)
					continue;		//ignore this object

				auto new_segnum = segnum;
				list_pos = nn;

#if defined(DXX_BUILD_DESCENT_I)
				int did_migrate;
				if (obj->type != OBJ_CNTRLCEN)		//don't migrate controlcen
#elif defined(DXX_BUILD_DESCENT_II)
				const int did_migrate = 0;
				if (obj->type != OBJ_CNTRLCEN && !(obj->type==OBJ_ROBOT && get_robot_id(obj)==65))		//don't migrate controlcen
#endif
				do {
#if defined(DXX_BUILD_DESCENT_I)
					did_migrate = 0;
#endif
					const uint_fast32_t sidemask = get_seg_masks(vcvertptr, obj->pos, vcsegptr(new_segnum), obj->size).sidemask;
	
					if (sidemask) {
						int sn,sf;

						for (sn=0,sf=1;sn<6;sn++,sf<<=1)
							if (sidemask & sf)
							{
#if defined(DXX_BUILD_DESCENT_I)
								const auto &&seg = vcsegptr(obj->segnum);
#elif defined(DXX_BUILD_DESCENT_II)
								const auto &&seg = vcsegptr(new_segnum);
#endif
		
								if (WALL_IS_DOORWAY(GameBitmaps, Textures, vcwallptr, seg, seg, sn) & WID_FLY_FLAG)
								{		//can explosion migrate through
									int child = seg->children[sn];
									int checknp;
		
									for (checknp=list_pos;checknp--;)
										if (rstate.Render_list[checknp] == child) {
											new_segnum = child;
											list_pos = checknp;
#if defined(DXX_BUILD_DESCENT_I)
											did_migrate = 1;
#endif
										}
								}
								if (sidemask <= sf)
									break;
							}
					}
	
				} while (did_migrate);
				add_obj_to_seglist(rstate, obj, new_segnum);
			}
		}
	}

	//now that there's a list for each segment, sort the items in those lists
	range_for (const auto segnum, partial_const_range(rstate.Render_list, rstate.N_render_segs))
	{
		if (segnum != segment_none) {
			sort_segment_object_list(rstate.render_seg_map[segnum]);
		}
	}
}
}

int Rear_view=0;

namespace dsx {
//renders onto current canvas
void render_frame(grs_canvas &canvas, fix eye_offset, window_rendered_data &window)
{
	if (Endlevel_sequence) {
		render_endlevel_frame(canvas, eye_offset);
		return;
	}

	if ( Newdemo_state == ND_STATE_RECORDING && eye_offset >= 0 )	{
     
      if (RenderingType==0)
   		newdemo_record_start_frame(FrameTime );
      if (RenderingType!=255)
   		newdemo_record_viewer_object(vmobjptridx(Viewer));
	}
  
   //Here:

	start_lighting_frame(vmobjptr(Viewer));		//this is for ugly light-smoothing hack
  
	g3_start_frame(canvas);

	Viewer_eye = Viewer->pos;

//	if (Viewer->type == OBJ_PLAYER && (PlayerCfg.CockpitMode[1]!=CM_REAR_VIEW))
//		vm_vec_scale_add2(&Viewer_eye,&Viewer->orient.fvec,(Viewer->size*3)/4);

	if (eye_offset)	{
		vm_vec_scale_add2(Viewer_eye,Viewer->orient.rvec,eye_offset);
	}

#if DXX_USE_EDITOR
	if (EditorWindow)
		Viewer_eye = Viewer->pos;
	#endif

	auto start_seg_num = find_point_seg(Viewer_eye, vmsegptridx(Viewer->segnum));

	if (start_seg_num==segment_none)
		start_seg_num = imsegptridx(Viewer->segnum);

	g3_set_view_matrix(Viewer_eye,
		(Rear_view && Viewer == ConsoleObject)
		? vm_matrix_x_matrix(Viewer->orient, vm_angles_2_matrix(vms_angvec{0, 0, INT16_MAX}))
		: Viewer->orient, Render_zoom);

	if (Clear_window == 1) {
		if (Clear_window_color == -1)
			Clear_window_color = BM_XRGB(0, 0, 0);	//BM_XRGB(31, 15, 7);
		gr_clear_canvas(canvas, Clear_window_color);
	}

	render_mine(canvas, start_seg_num, eye_offset, window);

	g3_end_frame();

   //RenderingType=0;

	// -- Moved from here by MK, 05/17/95, wrong if multiple renders/frame! FrameCount++;		//we have rendered a frame
}

#if defined(DXX_BUILD_DESCENT_II)
void update_rendered_data(window_rendered_data &window, const vmobjptr_t viewer, int rear_view_flag)
{
	window.time = timer_query();
	window.viewer = viewer;
	window.rear_view = rear_view_flag;
}
#endif

//build a list of segments to be rendered
//fills in Render_list & N_render_segs
static void build_segment_list(render_state_t &rstate, visited_twobit_array_t &visited, unsigned &first_terminal_seg, const vcsegidx_t start_seg_num)
{
	int	lcnt,scnt,ecnt;
	int	l;

	rstate.render_pos.fill(-1);

	#ifndef NDEBUG
	visited2 = {};
	#endif

	lcnt = scnt = 0;

	rstate.Render_list[lcnt] = start_seg_num;
	visited[start_seg_num]=1;
	lcnt++;
	ecnt = lcnt;
	rstate.render_pos[start_seg_num] = 0;
	{
		auto &rsm_start_seg = rstate.render_seg_map[start_seg_num];
		auto &rw = rsm_start_seg.render_window;
		rw.left = rw.top = 0;
		rw.right = grd_curcanv->cv_bitmap.bm_w-1;
		rw.bot = grd_curcanv->cv_bitmap.bm_h-1;
	}

	//breadth-first renderer

	//build list

	for (l=0;l<Render_depth;l++) {
		for (scnt=0;scnt < ecnt;scnt++) {
			auto segnum = rstate.Render_list[scnt];
			if (unlikely(segnum == segment_none))
			{
				assert(segnum != segment_none);
				continue;
			}

			auto &srsm = rstate.render_seg_map[segnum];
			auto &processed = srsm.processed;
			if (processed)
				continue;
			const auto &check_w = srsm.render_window;

			processed = true;

			const auto &&seg = vcsegptridx(segnum);
			const auto uor = rotate_list(seg->verts).uor & CC_BEHIND;

			//look at all sides of this segment.
			//tricky code to look at sides in correct order follows

			sort_child_array_t child_list;		//list of ordered sides to process
			uint_fast32_t n_children = 0;							//how many sides in child_list
			for (uint_fast32_t c = 0;c < MAX_SIDES_PER_SEGMENT;c++) {		//build list of sides
				const auto wid = WALL_IS_DOORWAY(GameBitmaps, Textures, vcwallptr, seg, seg, c);
				if (wid & WID_RENDPAST_FLAG)
				{
					if (auto codes_and = uor)
					{
						range_for (const auto i, Side_to_verts[c])
							codes_and &= Segment_points[seg->verts[i]].p3_codes;
						if (codes_and)
							continue;
					}
					child_list[n_children++] = c;
				}
			}
			if (!n_children)
				continue;

			//now order the sides in some magical way
			const auto &&child_range = partial_range(child_list, n_children);
			sort_seg_children(seg, child_range);
			project_list(seg->verts);
			range_for (const auto siden, child_range)
			{
				const auto ch = seg->children[siden];
				{
					{
						short min_x=32767,max_x=-32767,min_y=32767,max_y=-32767;
						int no_proj_flag=0;	//a point wasn't projected
						uint8_t codes_and_3d = 0xff, codes_and_2d = codes_and_3d;
						range_for (const auto i, Side_to_verts[siden])
						{
							g3s_point *pnt = &Segment_points[seg->verts[i]];

							if (! (pnt->p3_flags&PF_PROJECTED)) {no_proj_flag=1; break;}

							const int16_t _x = f2i(pnt->p3_sx), _y = f2i(pnt->p3_sy);

							if (_x < min_x) min_x = _x;
							if (_x > max_x) max_x = _x;

							if (_y < min_y) min_y = _y;
							if (_y > max_y) max_y = _y;
							codes_and_3d &= pnt->p3_codes;
							codes_and_2d &= code_window_point(_x,_y,check_w);
						}
						if (no_proj_flag || (!codes_and_3d && !codes_and_2d)) {	//maybe add this segment
							auto rp = rstate.render_pos[ch];
							rect nw;

							if (no_proj_flag)
								nw = check_w;
							else {
								nw.left  = max(check_w.left,min_x);
								nw.right = min(check_w.right,max_x);
								nw.top   = max(check_w.top,min_y);
								nw.bot   = min(check_w.bot,max_y);
							}

							//see if this seg already visited, and if so, does current window
							//expand the old window?
							if (rp != -1) {
								auto &old_w = rstate.render_seg_map[rstate.Render_list[rp]].render_window;
								if (nw.left < old_w.left ||
										 nw.top < old_w.top ||
										 nw.right > old_w.right ||
										 nw.bot > old_w.bot) {

									nw.left  = min(nw.left, old_w.left);
									nw.right = max(nw.right, old_w.right);
									nw.top   = min(nw.top, old_w.top);
									nw.bot   = max(nw.bot, old_w.bot);

									{
										//no_render_flag[lcnt] = 1;
										rstate.render_seg_map[ch].processed = false;		//force reprocess
										rstate.Render_list[lcnt] = segment_none;
										old_w = nw;		//get updated window
										goto no_add;
									}
								}
								else goto no_add;
							}
							rstate.render_pos[ch] = lcnt;
							rstate.Render_list[lcnt] = ch;
							{
								auto &chrsm = rstate.render_seg_map[ch];
								chrsm.Seg_depth = l;
								chrsm.render_window = nw;
							}
							lcnt++;
							if (lcnt >= MAX_RENDER_SEGS) {goto done_list;}
							visited[ch] = 1;
no_add:
	;

						}
					}
				}
			}
		}

		scnt = ecnt;
		ecnt = lcnt;

	}
done_list:

	first_terminal_seg = scnt;
	rstate.N_render_segs = lcnt;

}

//renders onto current canvas
void render_mine(grs_canvas &canvas, const vcsegidx_t start_seg_num, const fix eye_offset, window_rendered_data &window)
{
	using std::advance;
	render_state_t rstate;
	#ifndef NDEBUG
	object_rendered = {};
	#endif

	//set up for rendering

	render_start_frame();

	visited_twobit_array_t visited;

	unsigned first_terminal_seg;
#if DXX_USE_EDITOR
#if defined(DXX_BUILD_DESCENT_I)
	if (_search_mode || eye_offset>0)
#elif defined(DXX_BUILD_DESCENT_II)
	if (_search_mode)
#endif
	{
		first_terminal_seg = 0;
	}
	//else
	#endif
		//NOTE LINK TO ABOVE!!	-Link killed by kreatordxx to get editor selection working again
		build_segment_list(rstate, visited, first_terminal_seg, start_seg_num);		//fills in Render_list & N_render_segs

	const auto &&render_range = partial_const_range(rstate.Render_list, rstate.N_render_segs);
	const auto &&reversed_render_range = render_range.reversed();
	//render away
	#ifndef NDEBUG
#if defined(DXX_BUILD_DESCENT_I)
	if (!(_search_mode || eye_offset>0))
#elif defined(DXX_BUILD_DESCENT_II)
	if (!(_search_mode))
#endif
	{
		range_for (const auto segnum, render_range)
		{
			if (segnum != segment_none)
			{
				if (visited2[segnum])
					Int3();		//get Matt
				else
					visited2[segnum] = 1;
			}
		}
	}
	#endif

	//if (!(_search_mode))
		build_object_lists(rstate);

	if (eye_offset<=0) // Do for left eye or zero.
		set_dynamic_light(rstate);

	if (reversed_render_range.empty())
		/* Impossible, but later code has undefined behavior if this
		 * happens
		 */
		return;

	if (!_search_mode && Clear_window == 2) {
		if (first_terminal_seg < rstate.N_render_segs) {
			if (Clear_window_color == -1)
				Clear_window_color = BM_XRGB(0, 0, 0);	//BM_XRGB(31, 15, 7);
	
			const uint8_t color = Clear_window_color;
	
			range_for (const auto segnum, partial_const_range(rstate.Render_list, first_terminal_seg, rstate.N_render_segs))
			{
				if (segnum != segment_none) {
					const auto &rw = rstate.render_seg_map[segnum].render_window;
					#ifndef NDEBUG
					if (rw.left == -1 || rw.top == -1 || rw.right == -1 || rw.bot == -1)
						Int3();
					else
					#endif
						//NOTE LINK TO ABOVE!
						gr_rect(canvas, rw.left, rw.top, rw.right, rw.bot, color);
				}
			}
		}
	}
#if !DXX_USE_OGL
	range_for (const auto segnum, reversed_render_range)
	{
		// Interpolation_method = 0;
		auto &srsm = rstate.render_seg_map[segnum];

		//if (!no_render_flag[nn])
		if (segnum!=segment_none && (_search_mode || visited[segnum]!=3)) {
			//set global render window vars

			Current_seg_depth = srsm.Seg_depth;
			{
				const auto &rw = srsm.render_window;
				Window_clip_left  = rw.left;
				Window_clip_top   = rw.top;
				Window_clip_right = rw.right;
				Window_clip_bot   = rw.bot;
			}

			render_segment(*grd_curcanv, vcsegptridx(segnum));
			visited[segnum]=3;
			if (srsm.objects.empty())
				continue;

			{		//reset for objects
				Window_clip_left  = Window_clip_top = 0;
				Window_clip_right = canvas.cv_bitmap.bm_w-1;
				Window_clip_bot   = canvas.cv_bitmap.bm_h-1;
			}

			{
				//int n_expl_objs=0,expl_objs[5],i;
				const auto save_linear_depth = exchange(Max_linear_depth, Max_linear_depth_objects);
				range_for (auto &v, srsm.objects)
				{
					do_render_object(canvas, vmobjptridx(v.objnum), window);	// note link to above else
				}
				Max_linear_depth = save_linear_depth;
			}

		}
	}
#else
        // Two pass rendering. Since sprites and some level geometry can have transparency (blending), we need some fancy sorting.
        // GL_DEPTH_TEST helps to sort everything in view but we should make sure translucent sprites are rendered after geometry to prevent them to turn walls invisible (if rendered BEFORE geometry but still in FRONT of it).
        // If walls use blending, they should be rendered along with objects (in same pass) to prevent some ugly clipping.

        // First Pass: render opaque level geometry and level geometry with alpha pixels (high Alpha-Test func)
	range_for (const auto segnum, reversed_render_range)
	{
		auto &srsm = rstate.render_seg_map[segnum];

		if (segnum!=segment_none && (_search_mode || visited[segnum]!=3)) {
			//set global render window vars

			{
				const auto &rw = srsm.render_window;
				Window_clip_left  = rw.left;
				Window_clip_top   = rw.top;
				Window_clip_right = rw.right;
				Window_clip_bot   = rw.bot;
			}

			// render segment
			{
				const auto &&seg = vcsegptridx(segnum);
				int			sn;
				Assert(segnum!=segment_none && segnum<=Highest_segment_index);
				if (!rotate_list(seg->verts).uand)
				{		//all off screen?

					if (Viewer->type!=OBJ_ROBOT)
						Automap_visited[segnum]=1;

					for (sn=0; sn<MAX_SIDES_PER_SEGMENT; sn++)
					{
						const auto wid = WALL_IS_DOORWAY(GameBitmaps, Textures, vcwallptr, seg, seg, sn);
						if (wid == WID_TRANSPARENT_WALL || wid == WID_TRANSILLUSORY_WALL
#if defined(DXX_BUILD_DESCENT_II)
							|| (wid & WID_CLOAKED_FLAG)
#endif
							)
						{
                                                        if (PlayerCfg.AlphaBlendEClips && is_alphablend_eclip(TmapInfo[seg->sides[sn].tmap_num].eclip_num)) // Do NOT render geometry with blending textures. Since we've not rendered any objects, yet, they would disappear behind them.
                                                                continue;
							glAlphaFunc(GL_GEQUAL,0.8); // prevent ugly outlines if an object (which is rendered later) is shown behind a grate, door, etc. if texture filtering is enabled. These sides are rendered later again with normal AlphaFunc
							render_side(vcvertptr, canvas, seg, sn, wid, Viewer_eye);
							glAlphaFunc(GL_GEQUAL,0.02);
						}
						else
							render_side(vcvertptr, canvas, seg, sn, wid, Viewer_eye);
					}
				}
			}
		}
	}

        // Second pass: Render objects and level geometry with alpha pixels (normal Alpha-Test func) and eclips with blending
	range_for (const auto segnum, reversed_render_range)
	{
		auto &srsm = rstate.render_seg_map[segnum];

		if (segnum!=segment_none && (_search_mode || visited[segnum]!=3)) {
			//set global render window vars

			{
				const auto &rw = srsm.render_window;
				Window_clip_left  = rw.left;
				Window_clip_top   = rw.top;
				Window_clip_right = rw.right;
				Window_clip_bot   = rw.bot;
			}

			// render segment
			{
				const auto &&seg = vcsegptridx(segnum);
				int			sn;
				Assert(segnum!=segment_none && segnum<=Highest_segment_index);
				if (!rotate_list(seg->verts).uand)
				{		//all off screen?

					if (Viewer->type!=OBJ_ROBOT)
						Automap_visited[segnum]=1;

					for (sn=0; sn<MAX_SIDES_PER_SEGMENT; sn++)
					{
						const auto wid = WALL_IS_DOORWAY(GameBitmaps, Textures, vcwallptr, seg, seg, sn);
						if (wid == WID_TRANSPARENT_WALL || wid == WID_TRANSILLUSORY_WALL
#if defined(DXX_BUILD_DESCENT_II)
							|| (wid & WID_CLOAKED_FLAG)
#endif
							)
						{
							render_side(vcvertptr, canvas, seg, sn, wid, Viewer_eye);
						}
					}
				}
			}
			visited[segnum]=3;
			if (srsm.objects.empty())
				continue;
			{		//reset for objects
				Window_clip_left  = Window_clip_top = 0;
				Window_clip_right = canvas.cv_bitmap.bm_w-1;
				Window_clip_bot   = canvas.cv_bitmap.bm_h-1;
			}

			{
				range_for (auto &v, srsm.objects)
				{
					do_render_object(canvas, vmobjptridx(v.objnum), window);	// note link to above else
				}
			}
		}
	}
#endif

	// -- commented out by mk on 09/14/94...did i do a good thing??  object_render_targets();

#if DXX_USE_EDITOR
	#ifndef NDEBUG
	//draw curedge stuff
	if (Outline_mode)
		outline_seg_side(canvas, Cursegp, Curside, Curedge, Curvert);
	#endif
#endif
}

}
#if DXX_USE_EDITOR
//finds what segment is at a given x&y -  seg,side,face are filled in
//works on last frame rendered. returns true if found
//if seg<0, then an object was found, and the object number is -seg-1
int find_seg_side_face(short x,short y,segnum_t &seg,objnum_t &obj,int &side,int &face)
{
	_search_mode = -1;

	_search_x = x; _search_y = y;

	found_seg = segment_none;
	found_obj = object_none;

	render_frame(*(render_3d_in_big_window ? LargeView.ev_canv : Canv_editor_game), 0);

	_search_mode = 0;

	seg = found_seg;
	obj = found_obj;
	side = found_side;
	face = found_face;
	return found_seg != segment_none || found_obj != object_none;
}
#endif
