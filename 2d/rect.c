/*
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

#include <conf.h>
#include "u_mem.h"

#include "gr.h"
#include "grdef.h"

#ifdef OGL
#include "ogl_init.h"
#endif


void gr_urect(int left,int top,int right,int bot)
{
	int i;

#ifdef OGL
	if (TYPE == BM_OGL) {
		ogl_urect(left,top,right,bot);
		return;
	}
#endif
#ifdef D1XD3D
	if (TYPE == BM_DIRECTX) {
		if (left <= right && top <= bot)
			Win32_Rect (left, top, right, bot, grd_curcanv->cv_bitmap.bm_data, COLOR);
		return;
	}
#endif
	for ( i=top; i<=bot; i++ )
		gr_uscanline( left, right, i );
}

void gr_rect(int left,int top,int right,int bot)
{
	int i;

#ifdef OGL
	if (TYPE == BM_OGL) {
		ogl_urect(left,top,right,bot);
		return;
	}
#endif
#ifdef D1XD3D
	if (TYPE == BM_DIRECTX) {
		if (left <= right && top <= bot)
			Win32_Rect (left, top, right, bot, grd_curcanv->cv_bitmap.bm_data, COLOR);
		return;
	}
#endif
	for ( i=top; i<=bot; i++ )
		gr_scanline( left, right, i );
}
