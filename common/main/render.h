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


#ifndef _RENDER_H
#define _RENDER_H

#include "3d.h"

#ifdef __cplusplus
#include "segnum.h"
#include "fwdvalptridx.h"

struct window_rendered_data;

extern int Render_depth; //how many segments deep to render
static const unsigned Max_perspective_depth = 8; //	Deepest segment at which perspective extern interpolation will be used.
extern unsigned Max_linear_depth; //	Deepest segment at which linear extern interpolation will be used.
extern int Max_linear_depth_objects;
static const unsigned Simple_model_threshhold_scale = 50; // switch to simpler model when the object has depth greater than this value times its radius.
static const unsigned Max_debris_objects = 15; // How many debris objects to create

#ifdef OGL
#define DETRIANGULATION 0
#else
#define DETRIANGULATION 1
#endif

extern int Clear_window;    // 1 = Clear whole background window, 2 = clear view portals into rest of world, 0 = no clear

void render_frame(fix eye_offset, int window_num);  //draws the world into the current canvas

// cycle the flashing light for when mine destroyed
void flash_frame();

int find_seg_side_face(short x,short y,int *seg,int *side,int *face,int *poly);

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
int toggle_show_only_curside(void);

// When any render function needs to know what's looking at it, it
// should access Render_viewer_object members.
extern fix Render_zoom;     // the player's zoom factor

#if defined(DXX_BUILD_DESCENT_I)
static const fix Seismic_tremor_magnitude = 0;
static const ubyte RenderingType = 0;
#elif defined(DXX_BUILD_DESCENT_II)
extern fix Seismic_tremor_magnitude;
extern ubyte RenderingType;
#endif

extern fix flash_scale;
extern vms_vector Viewer_eye;

#ifdef EDITOR
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

void render_mine(segnum_t start_seg_num, fix eye_offset, window_rendered_data &);

#if defined(DXX_BUILD_DESCENT_II)
void update_rendered_data(window_rendered_data &window, vobjptr_t viewer, int rear_view_flag);
#endif

#endif

#endif /* _RENDER_H */
