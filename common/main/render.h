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
#include "segnum.h"
#include "objnum.h"
#include "fwd-object.h"

#ifdef dsx
namespace dsx {

struct window_rendered_data
{
#if defined(DXX_BUILD_DESCENT_II)
	fix64   time;
	object  *viewer;
	int     rear_view;
#endif
	std::vector<objnum_t> rendered_robots;
};

}
#endif

extern int Render_depth; //how many segments deep to render
constexpr unsigned Max_perspective_depth = 8; //	Deepest segment at which perspective extern interpolation will be used.
constexpr unsigned Max_linear_depth_objects = 20;
constexpr unsigned Simple_model_threshhold_scale = 50; // switch to simpler model when the object has depth greater than this value times its radius.
constexpr unsigned Max_debris_objects = 15; // How many debris objects to create

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
int find_seg_side_face(short x,short y,segnum_t &seg,objnum_t &obj,int &side,int &face,int &poly);

// these functions change different rendering parameters
// all return the new value of the parameter

// how may levels deep to render
int inc_render_depth(void);
int dec_render_depth(void);
int reset_render_depth(void);

// how many levels deep to render in perspective
int inc_perspective_depth(void);
int dec_perspective_depth(void);
int reset_perspective_depth(void);

// misc toggles
int toggle_outline_mode(void);

// When any render function needs to know what's looking at it, it
// should access Render_viewer_object members.
extern fix Render_zoom;     // the player's zoom factor

#ifdef dsx
namespace dsx {
#if defined(DXX_BUILD_DESCENT_I)
constexpr fix Seismic_tremor_magnitude = 0;
constexpr uint8_t RenderingType = 0;
#elif defined(DXX_BUILD_DESCENT_II)
extern fix Seismic_tremor_magnitude;
extern ubyte RenderingType;
#endif
}
#endif

extern fix flash_scale;
extern vms_vector Viewer_eye;

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
g3s_codes rotate_list(std::size_t nv, const int *pointnumlist);

template <typename T, std::size_t N>
static inline g3s_codes rotate_list(const array<T, N> &a)
{
	return rotate_list(a.size(), &a[0]);
}

#ifdef dsx
namespace dsx {
void render_frame(fix eye_offset, window_rendered_data &);  //draws the world into the current canvas

void render_mine(segnum_t start_seg_num, fix eye_offset, window_rendered_data &);

#if defined(DXX_BUILD_DESCENT_II)
void update_rendered_data(window_rendered_data &window, vobjptr_t viewer, int rear_view_flag);
#endif

static inline void render_frame(fix eye_offset)
{
	window_rendered_data window;
	render_frame(eye_offset, window);
}
}
#endif

#endif
