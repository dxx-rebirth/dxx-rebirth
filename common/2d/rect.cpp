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
#include "grdef.h"

#ifdef OGL
#include "ogl_init.h"
#endif

namespace dcx {

void gr_urect(int left,int top,int right,int bot, const uint8_t color)
{
#ifdef OGL
	if (TYPE == BM_OGL) {
		ogl_urect(left,top,right,bot, color);
		return;
	}
#else
	for ( int i=top; i<=bot; i++ )
		gr_uscanline(left, right, i, color);
#endif
}

void gr_rect(int left,int top,int right,int bot)
{
	const auto color = COLOR;
#ifdef OGL
	if (TYPE == BM_OGL) {
		ogl_urect(left,top,right,bot, color);
		return;
	}
#endif
	for ( int i=top; i<=bot; i++ )
		gr_scanline(left, right, i, color);
}

}
