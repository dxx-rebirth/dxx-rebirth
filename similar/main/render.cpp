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
#include <optional>
#include "render_state.h"
#include "inferno.h"
#include "segment.h"
#include "dxxerror.h"
#include "bm.h"
#include "texmap.h"
#include "render.h"
#include "game.h"
#include "object.h"
#include "textures.h"
#include "segpoint.h"
#include "wall.h"
#include "texmerge.h"
#include "3d.h"
#include "gameseg.h"
#include "vclip.h"
#include "lighting.h"
#include "newdemo.h"
#include "endlevel.h"
#include "u_mem.h"
#include "piggy.h"
#include "timer.h"
#include "effects.h"
#include "playsave.h"
#if DXX_USE_OGL
#include "ogl_init.h"
#endif
#include "args.h"

#include "compiler-range_for.h"
#include "d_levelstate.h"
#include "d_enumerate.h"
#include "d_range.h"
#include "partial_range.h"
#include "segiter.h"

#if DXX_USE_EDITOR
#include "editor/editor.h"
#include "editor/esegment.h"
#endif
#include <utility>

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

// When any render function needs to know what's looking at it, it should 
// access Viewer members.
namespace dsx {
const object * Viewer = NULL;
}

#if !DXX_USE_EDITOR && defined(RELEASE)
constexpr
#endif
fix Render_zoom = 0x9000;					//the player's zoom factor

namespace {
static uint16_t s_current_generation;
#ifndef NDEBUG
static std::bitset<MAX_OBJECTS> object_rendered;
#endif
}

namespace dcx {

//Global vars for window clip test
int Window_clip_left,Window_clip_top,Window_clip_right,Window_clip_bot;

}

#if DXX_USE_EDITOR
int	Render_only_bottom=0;
int	Bottom_bitmap_num = 9;
int _search_mode = 0;			//true if looking for curseg,side,face
short _search_x,_search_y;	//pixel we're looking at
namespace {
static int found_face;
static sidenum_t found_side;
static segnum_t found_seg;
static objnum_t found_obj;
}
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
namespace {
static void draw_outline(const g3_draw_line_context &context, const unsigned nverts, cg3s_point *const *const pointlist)
{
	const unsigned e = nverts - 1;
	range_for (const unsigned i, xrange(e))
		g3_draw_line(context, *pointlist[i], *pointlist[i + 1]);
	g3_draw_line(context, *pointlist[e], *pointlist[0]);
}
}
#endif

fix flash_scale;

#define FLASH_CYCLE_RATE f1_0

constexpr std::integral_constant<fix, FLASH_CYCLE_RATE> Flash_rate{};

//cycle the flashing light for when mine destroyed
namespace dsx {
void flash_frame()
{
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;
	static fixang flash_ang=0;

	if (Endlevel_sequence)
		return;

	if (PaletteBlueAdd > 10 )		//whiting out
		return;

//	flash_ang += fixmul(FLASH_CYCLE_RATE,FrameTime);
#if defined(DXX_BUILD_DESCENT_II)
	if (const auto Seismic_tremor_magnitude = LevelUniqueSeismicState.Seismic_tremor_magnitude)
	{
		fix	added_flash;

		added_flash = abs(Seismic_tremor_magnitude);
		if (added_flash < F1_0)
			added_flash *= 16;

		flash_ang += fixmul(Flash_rate, fixmul(FrameTime, added_flash+F1_0));
		flash_scale = fix_fastsin(flash_ang);
		flash_scale = (flash_scale + F1_0*3)/4;	//	gets in range 0.5 to 1.0
	} else
#endif
	if (LevelUniqueControlCenterState.Control_center_destroyed)
	{
		flash_ang += fixmul(Flash_rate,FrameTime);
		flash_scale = fix_fastsin(flash_ang);
		flash_scale = (flash_scale + f1_0)/2;
#if defined(DXX_BUILD_DESCENT_II)
		if (GameUniqueState.Difficulty_level == Difficulty_level_type::_0)
			flash_scale = (flash_scale+F1_0*3)/4;
#endif
	}


}

namespace {
static inline int is_alphablend_eclip(int eclip_num)
{
#if defined(DXX_BUILD_DESCENT_II)
	if (eclip_num == ECLIP_NUM_FORCE_FIELD || eclip_num == ECLIP_NUM_FORCE_FIELD2)
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
static void render_face(grs_canvas &canvas, const shared_segment &segp, const sidenum_t sidenum, const unsigned nv, const std::array<vertnum_t, 4> &vp, const texture1_value tmap1, const texture2_value tmap2, std::array<g3s_uvl, 4> uvl_copy, const WALL_IS_DOORWAY_result_t wid_flags)
{
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;
	auto &TmapInfo = LevelUniqueTmapInfoState.TmapInfo;
	grs_bitmap  *bm;

	std::array<cg3s_point *, 4> pointlist;

	Assert(nv <= pointlist.size());

	range_for (const uint_fast32_t i, xrange(nv))
	{
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
	if (wid_flags & WALL_IS_DOORWAY_FLAG::cloaked) {
		const auto wall_num = segp.shared_segment::sides[sidenum].wall_num;
		auto &Walls = LevelUniqueWallSubsystemState.Walls;
		auto &vcwallptr = Walls.vcptr;
		gr_settransblend(canvas, static_cast<gr_fade_level>(vcwallptr(wall_num)->cloak_value), gr_blend::normal);
		const uint8_t color = BM_XRGB(0, 0, 0);
		// set to black (matters for s3)

		g3_draw_poly(canvas, nv, pointlist, color);    // draw as flat poly
		gr_settransblend(canvas, GR_FADE_OFF, gr_blend::normal);

		return;
	}
#endif

#if DXX_USE_OGL
	grs_bitmap *bm2 = nullptr;
	if (!CGameArg.DbgUseOldTextureMerge)
	{
		const auto texture1 = Textures[get_texture_index(tmap1)];
		PIGGY_PAGE_IN(texture1);
		bm = &GameBitmaps[texture1];
		if (tmap2 != texture2_value::None)
		{
			const auto texture2 = Textures[get_texture_index(tmap2)];
			PIGGY_PAGE_IN(texture2);
			bm2 = &GameBitmaps[texture2];
			if (bm2->get_flag_mask(BM_FLAG_SUPER_TRANSPARENT))
			{
				bm2 = nullptr;
			bm = &texmerge_get_cached_bitmap( tmap1, tmap2 );
			}
		}
	}else
#endif
		// New code for overlapping textures...
		if (tmap2 != texture2_value::None)
		{
			bm = &texmerge_get_cached_bitmap( tmap1, tmap2 );
		} else {
			const auto texture1 = Textures[get_texture_index(tmap1)];
			bm = &GameBitmaps[texture1];
			PIGGY_PAGE_IN(texture1);
		}

	assert(!bm->get_flag_mask(BM_FLAG_PAGED_OUT));

	std::array<g3s_lrgb, 4>		dyn_light;
#if defined(DXX_BUILD_DESCENT_I)
	const auto Seismic_tremor_magnitude = 0;
#elif defined(DXX_BUILD_DESCENT_II)
	const auto Seismic_tremor_magnitude = LevelUniqueSeismicState.Seismic_tremor_magnitude;
#endif
	const auto control_center_destroyed = LevelUniqueControlCenterState.Control_center_destroyed;
	const auto need_flashing_lights = (control_center_destroyed | Seismic_tremor_magnitude);	//make lights flash
	auto &Dynamic_light = LevelUniqueLightState.Dynamic_light;
	//set light values for each vertex & build pointlist
	range_for (const uint_fast32_t i, xrange(nv))
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
			dli.r = (!Seismic_tremor_magnitude && PlayerCfg.DynLightColor)
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
	if (PlayerCfg.AlphaBlendEClips && is_alphablend_eclip(TmapInfo[get_texture_index(tmap1)].eclip_num)) // set nice transparency/blending for some special effects (if we do more, we should maybe use switch here)
	{
		alpha = true;
		gr_settransblend(canvas, GR_FADE_OFF, gr_blend::additive_c);
	}

#if DXX_USE_EDITOR
	if (Render_only_bottom && sidenum == sidenum_t::WBOTTOM)
		g3_draw_tmap(canvas, nv, pointlist, uvl_copy, dyn_light, GameBitmaps[Textures[Bottom_bitmap_num]]);
	else
#endif

#if DXX_USE_OGL
		if (bm2){
			g3_draw_tmap_2(canvas, nv, pointlist, uvl_copy, dyn_light, *bm, *bm2, get_texture_rotation_low(tmap2));
		}else
#endif
			g3_draw_tmap(canvas, nv, pointlist, uvl_copy, dyn_light, *bm);

	if (alpha)
		gr_settransblend(canvas, GR_FADE_OFF, gr_blend::normal); // revert any transparency / blending setting back to normal

#ifndef NDEBUG
	if (Outline_mode)
	{
		const uint8_t color = BM_XRGB(63, 63, 63);
		g3_draw_line_context context{canvas, color};
		draw_outline(context, nv, &pointlist[0]);
	}
#endif
}
}
}

namespace {
// ----------------------------------------------------------------------------
//	Only called if editor active.
//	Used to determine which face was clicked on.
static void check_face(grs_canvas &canvas, const vmsegidx_t segnum, const sidenum_t sidenum, const unsigned facenum, const unsigned nv, const std::array<vertnum_t, 4> &vp, const texture1_value tmap1, const texture2_value tmap2, const std::array<g3s_uvl, 4> &uvl_copy)
{
#if DXX_USE_EDITOR
	if (_search_mode) {
		std::array<g3s_lrgb, 4> dyn_light{};
		std::array<cg3s_point *, 4> pointlist;
#if DXX_USE_OGL
		(void)tmap1;
		(void)tmap2;
#else
		grs_bitmap *bm;
		if (tmap2 != texture2_value::None)
			bm = &texmerge_get_cached_bitmap( tmap1, tmap2 );
		else
			bm = &GameBitmaps[Textures[get_texture_index(tmap1)].index];
#endif
		range_for (const uint_fast32_t i, xrange(nv))
		{
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
static inline void check_render_face(grs_canvas &canvas, std::index_sequence<N...>, const vcsegptridx_t segnum, const sidenum_t sidenum, const unsigned facenum, const std::array<vertnum_t, 4> &ovp, const texture1_value tmap1, const texture2_value tmap2, const std::array<uvl, 4> &uvlp, const WALL_IS_DOORWAY_result_t wid_flags, const std::size_t nv)
{
	const std::array<vertnum_t, 4> vp{{ovp[N]...}};
	const std::array<g3s_uvl, 4> uvl_copy{{
		{uvlp[N].u, uvlp[N].v, uvlp[N].l}...
	}};
	render_face(canvas, segnum, sidenum, nv, vp, tmap1, tmap2, uvl_copy, wid_flags);
	check_face(canvas, segnum, sidenum, facenum, nv, vp, tmap1, tmap2, uvl_copy);
}

template <std::size_t N0, std::size_t N1, std::size_t N2, std::size_t N3>
static inline void check_render_face(grs_canvas &canvas, std::index_sequence<N0, N1, N2, N3> is, const vcsegptridx_t segnum, const sidenum_t sidenum, const unsigned facenum, const std::array<vertnum_t, 4> &vp, const texture1_value tmap1, const texture2_value tmap2, const std::array<uvl, 4> &uvlp, const WALL_IS_DOORWAY_result_t wid_flags)
{
	check_render_face(canvas, is, segnum, sidenum, facenum, vp, tmap1, tmap2, uvlp, wid_flags, 4);
}

/* Avoid default constructing final element of uvl_copy; if any members
 * are default constructed, gcc zero initializes all members.
 */
template <std::size_t N0, std::size_t N1, std::size_t N2>
static inline void check_render_face(grs_canvas &canvas, std::index_sequence<N0, N1, N2>, const vcsegptridx_t segnum, const sidenum_t sidenum, const unsigned facenum, const std::array<vertnum_t, 4> &vp, const texture1_value tmap1, const texture2_value tmap2, const std::array<uvl, 4> &uvlp, const WALL_IS_DOORWAY_result_t wid_flags)
{
	check_render_face(canvas, std::index_sequence<N0, N1, N2, 3>(), segnum, sidenum, facenum, vp, tmap1, tmap2, uvlp, wid_flags, 3);
}

constexpr std::integral_constant<fix, (F1_0/4)> Tulate_min_dot{};
//--unused-- fix	Tulate_min_ratio = (2*F1_0);
constexpr std::integral_constant<fix, (F1_0*15/16)> Min_n0_n1_dot{};
}

// -----------------------------------------------------------------------------------
//	Render a side.
//	Check for normal facing.  If so, render faces on side dictated by sidep->type.
namespace dsx {
namespace {
static void render_side(fvcvertptr &vcvertptr, grs_canvas &canvas, const vcsegptridx_t segp, const sidenum_t sidenum, const WALL_IS_DOORWAY_result_t wid_flags, const vms_vector &Viewer_eye)
{
	fix		min_dot, max_dot;

	if (!(wid_flags & WALL_IS_DOORWAY_FLAG::render))		//if (WALL_IS_DOORWAY(segp, sidenum) == WID_NO_WALL)
		return;

	const auto vertnum_list = get_side_verts(segp,sidenum);

	//	Regardless of whether this side is comprised of a single quad, or two triangles, we need to know one normal, so
	//	deal with it, get the dot product.
	const auto &sside = segp->shared_segment::sides[sidenum];
	const unsigned which_vertnum =
		(sside.get_type() == side_type::tri_13)
			? 1
			: 0;
	const auto tvec = vm_vec_normalized_quick(vm_vec_sub(Viewer_eye, vcvertptr(vertnum_list[which_vertnum])));
	auto &normals = sside.normals;
	const auto v_dot_n0 = vm_vec_dot(tvec, normals[0]);
	//	========== Mark: Here is the change...beginning here: ==========

	std::index_sequence<0, 1, 2, 3> is_quad;
	const auto &uside = segp->unique_segment::sides[sidenum];
	if (sside.get_type() == side_type::quad)
	{
		if (v_dot_n0 >= 0) {
			check_render_face(canvas, is_quad, segp, sidenum, 0, vertnum_list, uside.tmap_num, uside.tmap_num2, uside.uvls, wid_flags);
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

			check_render_face(canvas, is_quad, segp, sidenum, 0, vertnum_list, uside.tmap_num, uside.tmap_num2, uside.uvls, wid_flags);
		} else {
im_so_ashamed: ;
			if (sside.get_type() == side_type::tri_02)
			{
				if (v_dot_n0 >= 0) {
					check_render_face(canvas, std::index_sequence<0, 1, 2>(), segp, sidenum, 0, vertnum_list, uside.tmap_num, uside.tmap_num2, uside.uvls, wid_flags);
				}

				if (v_dot_n1 >= 0) {
					// want to render from vertices 0, 2, 3 on side
					check_render_face(canvas, std::index_sequence<0, 2, 3>(), segp, sidenum, 1, vertnum_list, uside.tmap_num, uside.tmap_num2, uside.uvls, wid_flags);
				}
			}
			else if (sside.get_type() == side_type::tri_13)
			{
				if (v_dot_n1 >= 0) {
					// rendering 1,2,3, so just skip 0
					check_render_face(canvas, std::index_sequence<1, 2, 3>(), segp, sidenum, 1, vertnum_list, uside.tmap_num, uside.tmap_num2, uside.uvls, wid_flags);
				}

				if (v_dot_n0 >= 0) {
					// want to render from vertices 0,1,3
					check_render_face(canvas, std::index_sequence<0, 1, 3>(), segp, sidenum, 0, vertnum_list, uside.tmap_num, uside.tmap_num2, uside.uvls, wid_flags);
				}

			} else
				throw shared_side::illegal_type(segp, sside);
		}
	}

}

#if DXX_USE_EDITOR
static void render_object_search(grs_canvas &canvas, const d_level_unique_light_state &LevelUniqueLightState, const vmobjptridx_t obj)
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
	render_object(canvas, LevelUniqueLightState, obj);
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
	render_object(canvas, LevelUniqueLightState, obj);
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

static void do_render_object(grs_canvas &canvas, const d_level_unique_light_state &LevelUniqueLightState, const vmobjptridx_t obj, window_rendered_data &window)
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
	if (obj->type == OBJ_ROBOT && obj->ctype.ai_info.ail.player_awareness_type == player_awareness_type_t::PA_NONE && (obj.get_unchecked_index() & 3) == (d_tick_count & 3))
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
		render_object_search(canvas, LevelUniqueLightState, obj);
	else
	#endif
		//NOTE LINK TO ABOVE
		render_object(canvas, LevelUniqueLightState, obj);

	for (auto n = obj->attached_obj; n != object_none;)
	{
		const auto &&o = obj.absolute_sibling(n);
		Assert(o->type == OBJ_FIREBALL);
		assert(o->control_source == object::control_type::explosion);
		Assert(o->flags & OF_ATTACHED);
		n = o->ctype.expl_info.next_attach;

		render_object(canvas, LevelUniqueLightState, o);
	}


#if DXX_USE_EDITOR
	if (EditorWindow && obj==Cur_object_index)
		g3d_interp_outline = save_3d_outline;
	#endif


}
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
g3s_codes rotate_list(fvcvertptr &vcvertptr, const std::size_t nv, const vertnum_t *const pointnumlist)
{
	g3s_codes cc;
	const auto current_generation = s_current_generation;
	const auto cheats_acid = cheats.acid;
	const float f = likely(!cheats_acid)
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

namespace {
//Given a lit of point numbers, project any that haven't been projected
static void project_list(const std::array<vertnum_t, 8> &pointnumlist)
{
	range_for (const auto pnum, pointnumlist)
	{
		auto &p = Segment_points[pnum];
		if (!(p.p3_flags & projection_flag::projected))
			g3_project_point(p);
	}
}
}


// -----------------------------------------------------------------------------------
#if !DXX_USE_OGL
namespace dsx {
namespace {
static void render_segment(fvcvertptr &vcvertptr, fvcwallptr &vcwallptr, const vms_vector &Viewer_eye, grs_canvas &canvas, const vcsegptridx_t seg)
{
	if (rotate_list(vcvertptr, seg->verts).uand == clipping_code::None)
	{		//all off screen?

#if defined(DXX_BUILD_DESCENT_II)
		if (Viewer->type != OBJ_ROBOT)
#endif
		{
			LevelUniqueAutomapState.Automap_visited[seg] = 1;
		}

		for (const auto sn : MAX_SIDES_PER_SEGMENT)
			render_side(vcvertptr, canvas, seg, sn, WALL_IS_DOORWAY(GameBitmaps, Textures, vcwallptr, seg, sn), Viewer_eye);
	}

	//draw any objects that happen to be in this segment

	//sort objects!
	//object_sort_segment_objects( seg );
}
}
}
#endif

#if DXX_USE_EDITOR
#ifndef NDEBUG

namespace {
constexpr fix CROSS_WIDTH = i2f(8);
constexpr fix CROSS_HEIGHT = i2f(8);

//draw outline for curside
static void outline_seg_side(grs_canvas &canvas, const shared_segment &seg, const sidenum_t _side, const side_relative_vertnum edge, const side_relative_vertnum vert)
{
	auto &verts = seg.verts;
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vcvertptr = Vertices.vcptr;
	if (rotate_list(vcvertptr, verts).uand == clipping_code::None)
	{		//all off screen?
		//render curedge of curside of curseg in green

		const uint8_t color = BM_XRGB(0, 63, 0);
		auto &sv = Side_to_verts[_side];
		g3_draw_line(g3_draw_line_context{canvas, color}, Segment_points[verts[sv[edge]]], Segment_points[verts[sv[next_side_vertex(edge)]]]);

		//draw a little cross at the current vert

		const auto pnt = &Segment_points[verts[sv[vert]]];

		g3_project_point(*pnt);		//make sure projected

//		gr_line(pnt->p3_sx-CROSS_WIDTH,pnt->p3_sy,pnt->p3_sx+CROSS_WIDTH,pnt->p3_sy);
//		gr_line(pnt->p3_sx,pnt->p3_sy-CROSS_HEIGHT,pnt->p3_sx,pnt->p3_sy+CROSS_HEIGHT);

		gr_line(canvas, pnt->p3_sx - CROSS_WIDTH, pnt->p3_sy, pnt->p3_sx, pnt->p3_sy - CROSS_HEIGHT, color);
		gr_line(canvas, pnt->p3_sx, pnt->p3_sy - CROSS_HEIGHT, pnt->p3_sx + CROSS_WIDTH, pnt->p3_sy, color);
		gr_line(canvas, pnt->p3_sx + CROSS_WIDTH, pnt->p3_sy, pnt->p3_sx, pnt->p3_sy + CROSS_HEIGHT, color);
		gr_line(canvas, pnt->p3_sx, pnt->p3_sy + CROSS_HEIGHT, pnt->p3_sx - CROSS_WIDTH, pnt->p3_sy, color);
	}
}
}

#endif
#endif

namespace dcx {

namespace {

static clipping_code code_window_point(const fix x, const fix y, const rect &w)
{
	clipping_code code{};
	if (x <= w.left)
		code |= clipping_code::off_left;
	if (x >= w.right)
		code |= clipping_code::off_right;

	if (y <= w.top)
		code |= clipping_code::off_top;
	if (y >= w.bot)
		code |= clipping_code::off_bot;

	return code;
}

}

//Given two sides of segment, tell the two verts which form the 
//edge between them
constexpr enumerated_array<
	enumerated_array<
		std::array<segment_relative_vertnum, 2>,
		6, sidenum_t>,
	6, sidenum_t> Two_sides_to_edge = {{{
	{{{
		{{segment_relative_vertnum{0xff}, segment_relative_vertnum{0xff}}},
		{{segment_relative_vertnum::_3, segment_relative_vertnum::_7}},
		{{segment_relative_vertnum{0xff}, segment_relative_vertnum{0xff}}},
		{{segment_relative_vertnum::_2, segment_relative_vertnum::_6}},
		{{segment_relative_vertnum::_6, segment_relative_vertnum::_7}},
		{{segment_relative_vertnum::_2, segment_relative_vertnum::_3}}
	}}},
	{{{
		{{segment_relative_vertnum::_3, segment_relative_vertnum::_7}},
		{{segment_relative_vertnum{0xff}, segment_relative_vertnum{0xff}}},
		{{segment_relative_vertnum::_0, segment_relative_vertnum::_4}},
		{{segment_relative_vertnum{0xff}, segment_relative_vertnum{0xff}}},
		{{segment_relative_vertnum::_4, segment_relative_vertnum::_7}},
		{{segment_relative_vertnum::_0, segment_relative_vertnum::_3}}
	}}},
	{{{
		{{segment_relative_vertnum{0xff}, segment_relative_vertnum{0xff}}},
		{{segment_relative_vertnum::_0, segment_relative_vertnum::_4}},
		{{segment_relative_vertnum{0xff}, segment_relative_vertnum{0xff}}},
		{{segment_relative_vertnum::_1, segment_relative_vertnum::_5}},
		{{segment_relative_vertnum::_4, segment_relative_vertnum::_5}},
		{{segment_relative_vertnum::_0, segment_relative_vertnum::_1}}
	}}},
	{{{
		{{segment_relative_vertnum::_2, segment_relative_vertnum::_6}},
		{{segment_relative_vertnum{0xff}, segment_relative_vertnum{0xff}}},
		{{segment_relative_vertnum::_1, segment_relative_vertnum::_5}},
		{{segment_relative_vertnum{0xff}, segment_relative_vertnum{0xff}}},
		{{segment_relative_vertnum::_5, segment_relative_vertnum::_6}},
		{{segment_relative_vertnum::_1, segment_relative_vertnum::_2}}
	}}},
	{{{
		{{segment_relative_vertnum::_6, segment_relative_vertnum::_7}},
		{{segment_relative_vertnum::_4, segment_relative_vertnum::_7}},
		{{segment_relative_vertnum::_4, segment_relative_vertnum::_5}},
		{{segment_relative_vertnum::_5, segment_relative_vertnum::_6}},
		{{segment_relative_vertnum{0xff}, segment_relative_vertnum{0xff}}},
		{{segment_relative_vertnum{0xff}, segment_relative_vertnum{0xff}}}
	}}},
	{{{
		{{segment_relative_vertnum::_2, segment_relative_vertnum::_3}},
		{{segment_relative_vertnum::_0, segment_relative_vertnum::_3}},
		{{segment_relative_vertnum::_0, segment_relative_vertnum::_1}},
		{{segment_relative_vertnum::_1, segment_relative_vertnum::_2}},
		{{segment_relative_vertnum{0xff}, segment_relative_vertnum{0xff}}},
		{{segment_relative_vertnum{0xff}, segment_relative_vertnum{0xff}}}
	}}}
}}};

namespace {

//given an edge specified by two verts, give the two sides on that edge
constexpr std::array<
	std::array<
		std::array<sidenum_t, 2>,
		8>,
	8> Edge_to_sides = {{
	{{  {{side_none,side_none}},	 {{sidenum_t::WRIGHT,sidenum_t::WFRONT}},	 {{side_none,side_none}},	 {{sidenum_t::WTOP,sidenum_t::WFRONT}},	 {{sidenum_t::WTOP,sidenum_t::WRIGHT}},	 {{side_none,side_none}},	 {{side_none,side_none}},	 {{side_none,side_none}}	}},
	{{  {{sidenum_t::WRIGHT,sidenum_t::WFRONT}},	 {{side_none,side_none}},	 {{sidenum_t::WBOTTOM,sidenum_t::WFRONT}},	 {{side_none,side_none}},	 {{side_none,side_none}},	 {{sidenum_t::WRIGHT,sidenum_t::WBOTTOM}},	 {{side_none,side_none}},	 {{side_none,side_none}}	}},
	{{  {{side_none,side_none}},	 {{sidenum_t::WBOTTOM,sidenum_t::WFRONT}},	 {{side_none,side_none}},	 {{sidenum_t::WLEFT,sidenum_t::WFRONT}},	 {{side_none,side_none}},	 {{side_none,side_none}},	 {{sidenum_t::WLEFT,sidenum_t::WBOTTOM}},	 {{side_none,side_none}}	}},
	{{  {{sidenum_t::WTOP,sidenum_t::WFRONT}},	 {{side_none,side_none}},	 {{sidenum_t::WLEFT,sidenum_t::WFRONT}},	 {{side_none,side_none}},	 {{side_none,side_none}},	 {{side_none,side_none}},	 {{side_none,side_none}},	 {{sidenum_t::WLEFT,sidenum_t::WTOP}}	}},
	{{  {{sidenum_t::WTOP,sidenum_t::WRIGHT}},	 {{side_none,side_none}},	 {{side_none,side_none}},	 {{side_none,side_none}},	 {{side_none,side_none}},	 {{sidenum_t::WRIGHT,sidenum_t::WBACK}},	 {{side_none,side_none}},	 {{sidenum_t::WTOP,sidenum_t::WBACK}}	}},
	{{  {{side_none,side_none}},	 {{sidenum_t::WRIGHT,sidenum_t::WBOTTOM}},	 {{side_none,side_none}},	 {{side_none,side_none}},	 {{sidenum_t::WRIGHT,sidenum_t::WBACK}},	 {{side_none,side_none}},	 {{sidenum_t::WBOTTOM,sidenum_t::WBACK}},	 {{side_none,side_none}}	}},
	{{  {{side_none,side_none}},	 {{side_none,side_none}},	 {{sidenum_t::WLEFT,sidenum_t::WBOTTOM}},	 {{side_none,side_none}},	 {{side_none,side_none}},	 {{sidenum_t::WBOTTOM,sidenum_t::WBACK}},	 {{side_none,side_none}},	 {{sidenum_t::WLEFT,sidenum_t::WBACK}}	}},
	{{  {{side_none,side_none}},	 {{side_none,side_none}},	 {{side_none,side_none}},	 {{sidenum_t::WLEFT,sidenum_t::WTOP}},	 {{sidenum_t::WTOP,sidenum_t::WBACK}},	 {{side_none,side_none}},	 {{sidenum_t::WLEFT,sidenum_t::WBACK}},	 {{side_none,side_none}}	}},
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
static std::optional<sidenum_t> find_seg_side(const shared_segment &seg, const std::array<vertnum_t, 2> &verts, const sidenum_t notside)
{
	const auto v0 = verts[0];
	const auto v1 = verts[1];

	const auto b = begin(seg.verts);
	const auto e = end(seg.verts);
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
			return {};
	}

	const auto &eptr = Edge_to_sides[std::distance(b, iv0)][std::distance(b, iv1)];

	const auto side0 = eptr[0];
	const auto side1 = eptr[1];

	Assert(side0 != side_none && side1 != side_none);

	if (side0 != notside) {
		Assert(side1==notside);
		return static_cast<sidenum_t>(side0);
	}
	else {
		Assert(side0==notside);
		return static_cast<sidenum_t>(side1);
	}
}

[[nodiscard]]
static bool compare_child(fvcvertptr &vcvertptr, const vms_vector &Viewer_eye, const shared_segment &seg, const shared_segment &cseg, const sidenum_t edgeside)
{
	const auto &cside = cseg.sides[edgeside];
	const auto &sv = Side_to_verts[edgeside][cside.get_type() == side_type::tri_13 ? side_relative_vertnum::_1 : side_relative_vertnum::_0];
	const auto &temp = vm_vec_sub(Viewer_eye, vcvertptr(seg.verts[sv]));
	const auto &cnormal = cside.normals;
	return vm_vec_dot(cnormal[0], temp) < 0 || vm_vec_dot(cnormal[1], temp) < 0;
}

//see if the order matters for these two children.
//returns 0 if order doesn't matter, 1 if c0 before c1, -1 if c1 before c0
[[nodiscard]]
static bool compare_children(fvcvertptr &vcvertptr, const vms_vector &Viewer_eye, const vcsegptridx_t seg, const sidenum_t s0, const sidenum_t s1)
{
	Assert(s0 != side_none && s1 != side_none);

	if (s0 == s1)
		return false;
	if (Side_opposite[s0] == s1)
		return false;
	//find normals of adjoining sides
	const shared_segment &sseg = seg;
	auto &edge = Two_sides_to_edge[s0][s1];
	const std::array<vertnum_t, 2> edge_verts = {
		{sseg.verts[edge[0]], sseg.verts[edge[1]]}
	};
	const auto &&seg0 = seg.absolute_sibling(sseg.children[s0]);
	const auto edgeside0 = find_seg_side(seg0, edge_verts, find_connect_side(seg, seg0));
	if (!edgeside0)
		return false;
	const auto r0 = compare_child(vcvertptr, Viewer_eye, seg, seg0, *edgeside0);
	if (!r0)
		return r0;
	const auto &&seg1 = seg.absolute_sibling(sseg.children[s1]);
	const auto edgeside1 = find_seg_side(seg1, edge_verts, find_connect_side(seg, seg1));
	if (!edgeside1)
		return false;
	return !compare_child(vcvertptr, Viewer_eye, seg, seg1, *edgeside1);
}

//short the children of segment to render in the correct order
//returns non-zero if swaps were made
using sort_child_array_t = per_side_array<sidenum_t>;
static void sort_seg_children(fvcvertptr &vcvertptr, const vms_vector &Viewer_eye, const vcsegptridx_t seg, const ranges::subrange<sort_child_array_t::iterator> &r)
{
	//for each child,  compare with other children and see if order matters
	//if order matters, fix if wrong
	auto predicate = [&vcvertptr, &Viewer_eye, seg](const sidenum_t a, const sidenum_t b)
	{
		return compare_children(vcvertptr, Viewer_eye, seg, a, b);
	};
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
		// gcc produces invalid bounds checks when sorting arrays of size 6 due to a compiler optimization bug.
		// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=107986
		std::sort(r.begin(), r.end(), predicate);
#pragma GCC diagnostic pop
}

static void add_obj_to_seglist(render_state_t &rstate, objnum_t objnum, segnum_t segnum)
{
	auto p = rstate.render_seg_map.emplace(segnum, render_state_t::per_segment_state_t{});
	auto &o = p.first->second.objects;
	if (p.second)
		o.reserve(16);
	o.emplace_back(render_state_t::per_segment_state_t::distant_object{objnum});
}

using visited_twobit_array_t = visited_segment_mask_t<2>;

class render_compare_context_t
{
	typedef render_state_t::per_segment_state_t::distant_object distant_object;
	struct element
	{
		fix64 dist_squared;
#if defined(DXX_BUILD_DESCENT_II)
		const object *objp;
#endif
	};
	using array_t = std::array<element, MAX_OBJECTS>;
	array_t m_array;
public:
	array_t::reference operator[](std::size_t i) { return m_array[i]; }
	array_t::const_reference operator[](std::size_t i) const { return m_array[i]; }
	render_compare_context_t(fvcobjptr &vcobjptr, const vms_vector &Viewer_eye, const render_state_t::per_segment_state_t &segstate)
	{
		range_for (const auto t, segstate.objects)
		{
			const auto objnum = t.objnum;
			auto &objp = *vcobjptr(objnum);
			auto &e = (*this)[objnum];
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
	auto &doa = operator[](a.objnum);
	auto &dob = operator[](b.objnum);
	const auto delta_dist_squared = doa.dist_squared - dob.dist_squared;

#if defined(DXX_BUILD_DESCENT_II)
	const auto obj_a = doa.objp;
	const auto obj_b = dob.objp;

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

static void sort_segment_object_list(fvcobjptr &vcobjptr, const vms_vector &Viewer_eye, render_state_t::per_segment_state_t &segstate)
{
	render_compare_context_t context(vcobjptr, Viewer_eye, segstate);
	auto &v = segstate.objects;
	std::sort(v.begin(), v.end(), std::cref(context));
}

}

}

namespace dsx {
namespace {

static void build_object_lists(object_array &Objects, fvcsegptr &vcsegptr, const vms_vector &Viewer_eye, render_state_t &rstate)
{
	const auto viewer = Viewer;
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcvertptr = Vertices.vcptr;
	auto &vcwallptr = Walls.vcptr;
	const auto N_render_segs = rstate.N_render_segs;
	range_for (const unsigned nn, xrange(N_render_segs))
	{
		const auto segnum = rstate.Render_list[nn];
		if (segnum != segment_none) {
			range_for (const auto obj, objects_in(vcsegptr(segnum), Objects.vcptridx, vcsegptr))
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
					if (const auto sidemask = get_seg_masks(vcvertptr, obj->pos, vcsegptr(new_segnum), obj->size).sidemask; sidemask != sidemask_t{})
					{
						for (const auto sn : MAX_SIDES_PER_SEGMENT)
						{
							const auto sf = build_sidemask(sn);
							if (sidemask & sf)
							{
#if defined(DXX_BUILD_DESCENT_I)
								const cscusegment &&seg = vcsegptr(obj->segnum);
#elif defined(DXX_BUILD_DESCENT_II)
								const cscusegment &&seg = vcsegptr(new_segnum);
#endif
		
								if (WALL_IS_DOORWAY(GameBitmaps, Textures, vcwallptr, seg, sn) & WALL_IS_DOORWAY_FLAG::fly)
								{		//can explosion migrate through
									const auto child = seg.s.children[sn];
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
			sort_segment_object_list(Objects.vcptr, Viewer_eye, rstate.render_seg_map[segnum]);
		}
	}
}
}
}

int Rear_view=0;

namespace dsx {
//renders onto current canvas
void render_frame(grs_canvas &canvas, fix eye_offset, window_rendered_data &window)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptridx = Objects.vcptridx;
	if (Endlevel_sequence) {
		render_endlevel_frame(canvas, eye_offset);
		return;
	}

	if ( Newdemo_state == ND_STATE_RECORDING && eye_offset >= 0 )	{
     
      if (RenderingType==0)
   		newdemo_record_start_frame(FrameTime );
      if (RenderingType!=255)
   		newdemo_record_viewer_object(vcobjptridx(Viewer));
	}
  
   //Here:

	start_lighting_frame(*Viewer);		//this is for ugly light-smoothing hack
  
	g3_start_frame(canvas);

#if DXX_USE_STEREOSCOPIC_RENDER
#if DXX_USE_OGL
	// select stereo viewport/transform/buffer per left/right eye
	if (VR_stereo != StereoFormat::None && eye_offset)
		ogl_stereo_frame(eye_offset < 0, VR_eye_offset);
#endif
#endif

	auto Viewer_eye = Viewer->pos;

//	if (Viewer->type == OBJ_PLAYER && (PlayerCfg.CockpitMode[1]!=CM_REAR_VIEW))
//		vm_vec_scale_add2(&Viewer_eye,&Viewer->orient.fvec,(Viewer->size*3)/4);

	if (eye_offset)	{
		vm_vec_scale_add2(Viewer_eye,Viewer->orient.rvec,eye_offset);
	}

#if DXX_USE_EDITOR
	if (EditorWindow)
		Viewer_eye = Viewer->pos;
	#endif

	const auto &&viewer_segp = Segments.vmptridx(Viewer->segnum);
	auto start_seg_num = find_point_seg(LevelSharedSegmentState, LevelUniqueSegmentState, Viewer_eye, viewer_segp);

	if (start_seg_num==segment_none)
		start_seg_num = viewer_segp;

	g3_set_view_matrix(Viewer_eye,
		(Rear_view && Viewer == ConsoleObject)
		? vm_matrix_x_matrix(Viewer->orient, vm_angles_2_matrix(vms_angvec{0, 0, INT16_MAX}))
		: Viewer->orient, Render_zoom);

	if (Clear_window == 1) {
		if (Clear_window_color == -1)
			Clear_window_color = BM_XRGB(0, 0, 0);	//BM_XRGB(31, 15, 7);
		gr_clear_canvas(canvas, Clear_window_color);
	}

	render_mine(canvas, Viewer_eye, start_seg_num, eye_offset, window);

	g3_end_frame();

   //RenderingType=0;

	// -- Moved from here by MK, 05/17/95, wrong if multiple renders/frame! FrameCount++;		//we have rendered a frame
}

window_rendered_data::window_rendered_data()
#if defined(DXX_BUILD_DESCENT_II)
	: time(timer_query())
#endif
{
}

namespace {
//build a list of segments to be rendered
//fills in Render_list & N_render_segs
static void build_segment_list(render_state_t &rstate, const vms_vector &Viewer_eye, visited_twobit_array_t &visited, unsigned &first_terminal_seg, const vcsegidx_t start_seg_num)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	int	lcnt,scnt,ecnt;
	int	l;

	rstate.render_pos.fill(-1);

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

	auto &vcvertptr = Vertices.vcptr;
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcwallptr = Walls.vcptr;
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
			const auto uor = rotate_list(vcvertptr, seg->verts).uor & clipping_code::behind;

			//look at all sides of this segment.
			//tricky code to look at sides in correct order follows

			sort_child_array_t child_list;		//list of ordered sides to process
			const auto child_begin = child_list.begin();
			auto child_iter = child_begin;
			for (const auto &&[c, sv] : enumerate(Side_to_verts))
			{		//build list of sides
				const auto wid = WALL_IS_DOORWAY(GameBitmaps, Textures, vcwallptr, seg, c);
				if (wid & WALL_IS_DOORWAY_FLAG::rendpast)
				{
					if (uor != clipping_code::None)
					{
						auto codes_and = uor;
						range_for (const auto i, sv)
							codes_and &= Segment_points[seg->verts[i]].p3_codes;
						if (codes_and != clipping_code::None)
							continue;
					}
					*child_iter++ = c;
				}
			}
			if (child_iter == child_begin)
				continue;

			//now order the sides in some magical way
			const auto &&child_range = ranges::subrange(child_begin, child_iter);
			sort_seg_children(vcvertptr, Viewer_eye, seg, child_range);
			project_list(seg->verts);
			range_for (const auto siden, child_range)
			{
				const auto ch = seg->shared_segment::children[siden];
				{
					{
						short min_x=32767,max_x=-32767,min_y=32767,max_y=-32767;
						int no_proj_flag=0;	//a point wasn't projected
						clipping_code codes_and_3d{0xff};
						auto codes_and_2d = codes_and_3d;
						range_for (const auto i, Side_to_verts[siden])
						{
							g3s_point *pnt = &Segment_points[seg->verts[i]];

							if (! (pnt->p3_flags&projection_flag::projected)) {no_proj_flag=1; break;}

							const int16_t _x = f2i(pnt->p3_sx), _y = f2i(pnt->p3_sy);

							if (_x < min_x) min_x = _x;
							if (_x > max_x) max_x = _x;

							if (_y < min_y) min_y = _y;
							if (_y > max_y) max_y = _y;
							codes_and_3d &= pnt->p3_codes;
							codes_and_2d &= code_window_point(_x,_y,check_w);
						}
						if (no_proj_flag || (codes_and_3d == clipping_code::None && codes_and_2d == clipping_code::None)) {	//maybe add this segment
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
}

//renders onto current canvas
void render_mine(grs_canvas &canvas, const vms_vector &Viewer_eye, const vcsegidx_t start_seg_num, const fix eye_offset, window_rendered_data &window)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vmobjptridx = Objects.vmptridx;
#if DXX_USE_OGL
	auto &TmapInfo = LevelUniqueTmapInfoState.TmapInfo;
#else
	auto &vcvertptr = Vertices.vcptr;
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcwallptr = Walls.vcptr;
#endif
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
		build_segment_list(rstate, Viewer_eye, visited, first_terminal_seg, start_seg_num);		//fills in Render_list & N_render_segs

	const auto &&render_range = partial_const_range(rstate.Render_list, rstate.N_render_segs);
	const auto &&reversed_render_range = render_range.reversed();
	//render away

	//if (!(_search_mode))
		build_object_lists(Objects, vcsegptr, Viewer_eye, rstate);

	if (eye_offset<=0) // Do for left eye or zero.
		set_dynamic_light(LevelSharedRobotInfoState.Robot_info, rstate);

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

			render_segment(vcvertptr, vcwallptr, Viewer_eye, *grd_curcanv, vcsegptridx(segnum));
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
				const auto save_linear_depth = std::exchange(Max_linear_depth, Max_linear_depth_objects);
				range_for (auto &v, srsm.objects)
				{
					do_render_object(canvas, LevelUniqueLightState, vmobjptridx(v.objnum), window);	// note link to above else
				}
				Max_linear_depth = save_linear_depth;
			}

		}
	}
#else
        // Two pass rendering. Since sprites and some level geometry can have transparency (blending), we need some fancy sorting.
        // GL_DEPTH_TEST helps to sort everything in view but we should make sure translucent sprites are rendered after geometry to prevent them to turn walls invisible (if rendered BEFORE geometry but still in FRONT of it).
        // If walls use blending, they should be rendered along with objects (in same pass) to prevent some ugly clipping.

	auto &vcvertptr = Vertices.vcptr;
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcwallptr = Walls.vcptr;
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
				Assert(segnum!=segment_none && segnum<=Highest_segment_index);
				if (rotate_list(vcvertptr, seg->verts).uand == clipping_code::None)
				{		//all off screen?

					if (Viewer->type!=OBJ_ROBOT)
						LevelUniqueAutomapState.Automap_visited[segnum] = 1;

					for (const auto sn : MAX_SIDES_PER_SEGMENT)
					{
						const auto wid = WALL_IS_DOORWAY(GameBitmaps, Textures, vcwallptr, seg, sn);
						if (wid == WID_TRANSPARENT_WALL || wid == WID_TRANSILLUSORY_WALL
#if defined(DXX_BUILD_DESCENT_II)
							|| (wid & WALL_IS_DOORWAY_FLAG::cloaked)
#endif
							)
						{
							if (PlayerCfg.AlphaBlendEClips && is_alphablend_eclip(TmapInfo[get_texture_index(seg->unique_segment::sides[sn].tmap_num)].eclip_num)) // Do NOT render geometry with blending textures. Since we've not rendered any objects, yet, they would disappear behind them.
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
				Assert(segnum!=segment_none && segnum<=Highest_segment_index);
				if (rotate_list(vcvertptr, seg->verts).uand == clipping_code::None)
				{		//all off screen?

					if (Viewer->type!=OBJ_ROBOT)
						LevelUniqueAutomapState.Automap_visited[segnum] = 1;

					for (const auto sn : MAX_SIDES_PER_SEGMENT)
					{
						const auto wid = WALL_IS_DOORWAY(GameBitmaps, Textures, vcwallptr, seg, sn);
						if (wid == WID_TRANSPARENT_WALL || wid == WID_TRANSILLUSORY_WALL
#if defined(DXX_BUILD_DESCENT_II)
							|| (wid & WALL_IS_DOORWAY_FLAG::cloaked)
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
					do_render_object(canvas, LevelUniqueLightState, vmobjptridx(v.objnum), window);	// note link to above else
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

//-------------- Renders a hostage --------------------------------------------
void draw_hostage(const d_vclip_array &Vclip, grs_canvas &canvas, const d_level_unique_light_state &LevelUniqueLightState, const vmobjptridx_t obj)
{
	auto &vci = obj->rtype.vclip_info;
	draw_object_tmap_rod(canvas, &LevelUniqueLightState, obj, Vclip[vci.vclip_num].frames[vci.framenum]);
}

}
#if DXX_USE_EDITOR
//finds what segment is at a given x&y -  seg,side,face are filled in
//works on last frame rendered. returns true if found
//if seg<0, then an object was found, and the object number is -seg-1
int find_seg_side_face(short x, short y, segnum_t &seg, objnum_t &obj, sidenum_t &side, int &face)
{
	_search_mode = -1;

	_search_x = x; _search_y = y;

	found_seg = segment_none;
	found_obj = object_none;

	{
		window_rendered_data window;
		render_frame(*(render_3d_in_big_window ? LargeView.ev_canv : Canv_editor_game), 0, window);
	}

	_search_mode = 0;

	seg = found_seg;
	obj = found_obj;
	side = found_side;
	face = found_face;
	return found_seg != segment_none || found_obj != object_none;
}
#endif
