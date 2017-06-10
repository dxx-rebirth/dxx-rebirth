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
 * Defnts for med drawing stuff
 *
 */

#pragma once

#ifdef __cplusplus
#include "fwd-segment.h"
#include "fwd-gr.h"

struct editor_view;

#ifdef dsx
namespace dsx {
void meddraw_init_views( grs_canvas * canvas);
}
#endif
#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
void draw_world(grs_canvas *screen_canvas,editor_view *v,vmsegptridx_t mine_ptr,int depth);
void find_segments(short x,short y,grs_canvas *screen_canvas,editor_view *v,vmsegptridx_t mine_ptr,int depth);
#endif

//    segp = pointer to segments array, probably always Segments.
//    automap_flag = 1 if this render is for the automap, else 0 (for editor)

#endif
