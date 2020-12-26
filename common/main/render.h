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
 * Header for rendering-based functions
 *
 */

#pragma once

#include "3d.h"

#ifdef __cplusplus
#include <vector>
#include "objnum.h"
#include "fwd-object.h"
#include "fwd-segment.h"
#include "fwd-vclip.h"
#include "lighting.h"

#ifdef dsx
namespace dsx {

struct window_rendered_data
{
#if defined(DXX_BUILD_DESCENT_II)
	fix64   time;
	const object *viewer;
	int     rear_view;
#endif
	std::vector<objnum_t> rendered_robots;
};

}
#endif

extern int Render_depth; //how many segments deep to render
constexpr std::integral_constant<unsigned, 8> Max_perspective_depth{}; //	Deepest segment at which perspective extern interpolation will be used.
constexpr std::integral_constant<unsigned, 20> Max_linear_depth_objects{};
constexpr std::integral_constant<unsigned, 50> Simple_model_threshhold_scale{}; // switch to simpler model when the object has depth greater than this value times its radius.
constexpr std::integral_constant<unsigned, 15> Max_debris_objects{}; // How many debris objects to create

#if DXX_USE_OGL
#define DETRIANGULATION 0
#else
#define DETRIANGULATION 1
extern unsigned Max_linear_depth; //	Deepest segment at which linear extern interpolation will be used.
#endif

extern int Clear_window;    // 1 = Clear whole background window, 2 = clear view portals into rest of world, 0 = no clear

// cycle the flashing light for when mine destroyed
#ifdef dsx
namespace dsx {
void flash_frame();

}
#endif
int find_seg_side_face(short x,short y,segnum_t &seg,objnum_t &obj,int &side,int &face);

// these functions change different rendering parameters
// all return the new value of the parameter

// misc toggles
int toggle_outline_mode(void);

// When any render function needs to know what's looking at it, it
// should access Render_viewer_object members.
extern
#if !DXX_USE_EDITOR && defined(RELEASE)
const
#endif
fix Render_zoom;     // the player's zoom factor

#ifdef dsx
namespace dsx {
#if defined(DXX_BUILD_DESCENT_I)
constexpr std::integral_constant<uint8_t, 0> RenderingType{};
#elif defined(DXX_BUILD_DESCENT_II)
extern uint8_t RenderingType;
#endif
}
#endif

extern fix flash_scale;

#if DXX_USE_EDITOR
extern int Render_only_bottom;
#endif

//
// Routines for conditionally rotating & projecting points
//

// This must be called at the start of the frame if rotate_list() will be used
void render_start_frame(void);

// Given a list of point numbers, rotate any that haven't been rotated
// this frame
g3s_codes rotate_list(fvcvertptr &vcvertptr, std::size_t nv, const vertnum_t *pointnumlist);

template <std::size_t N>
static inline g3s_codes rotate_list(fvcvertptr &vcvertptr, const std::array<vertnum_t, N> &a)
{
	return rotate_list(vcvertptr, a.size(), &a[0]);
}

#ifdef dsx
namespace dsx {
void render_frame(grs_canvas &, fix eye_offset, window_rendered_data &);  //draws the world into the current canvas
void render_mine(grs_canvas &, const vms_vector &, vcsegidx_t start_seg_num, fix eye_offset, window_rendered_data &);

#if defined(DXX_BUILD_DESCENT_II)
void update_rendered_data(window_rendered_data &window, const object &viewer, int rear_view_flag);
#endif

static inline void render_frame(grs_canvas &canvas, fix eye_offset)
{
	window_rendered_data window;
	render_frame(canvas, eye_offset, window);
}

// Render an object.  Calls one of several routines based on type
void render_object(grs_canvas &, const d_level_unique_light_state &LevelUniqueLightState, vmobjptridx_t obj);

// draw an object that is a texture-mapped rod
void draw_object_tmap_rod(grs_canvas &, const d_level_unique_light_state *const LevelUniqueLightState, vcobjptridx_t obj, bitmap_index bitmap);

void draw_hostage(const d_vclip_array &Vclip, grs_canvas &, const d_level_unique_light_state &, vmobjptridx_t obj);
void draw_morph_object(grs_canvas &, const d_level_unique_light_state &LevelUniqueLightState, vmobjptridx_t obj);
}
#endif

#endif
