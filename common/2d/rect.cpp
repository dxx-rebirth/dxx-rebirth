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
 * Graphical routines for drawing rectangles.
 *
 */

#include "u_mem.h"
#include "gr.h"

#if DXX_USE_OGL
#include "ogl_init.h"
#endif

namespace dcx {

void gr_urect(grs_canvas &canvas, const int left, const int top, const int right, const int bot, const color_palette_index color)
{
#if DXX_USE_OGL
	if (canvas.cv_bitmap.get_type() == bm_mode::ogl)
	{
		ogl_urect(canvas, left, top, right, bot, color);
		return;
	}
#else
	for ( int i=top; i<=bot; i++ )
		gr_uscanline(canvas, left, right, i, color);
#endif
}

void gr_rect(grs_canvas &canvas, const int left, const int top, const int right, const int bot, const color_palette_index color)
{
#if DXX_USE_OGL
	if (canvas.cv_bitmap.get_type() == bm_mode::ogl)
	{
		ogl_urect(canvas, left, top, right, bot, color);
		return;
	}
#endif
	for ( int i=top; i<=bot; i++ )
		gr_scanline(canvas, left, right, i, color);
}

}
