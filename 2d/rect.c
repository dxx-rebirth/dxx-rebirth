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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/2d/rect.c,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:39:00 $
 *
 * Graphical routines for drawing rectangles.
 *
 * $Log: rect.c,v $
 * Revision 1.1.1.1  2006/03/17 19:39:00  zicodxx
 * initial import
 *
 * Revision 1.4  1999/10/07 02:27:14  donut
 * OGL includes to remove warnings
 *
 * Revision 1.3  1999/09/30 23:02:27  donut
 * opengl direct support for ingame and normal menus, fonts as textures, and automap support
 *
 * Revision 1.2  1999/08/05 22:53:40  sekmu
 *
 * D3D patch(es) from ADB
 *
 * Revision 1.1.1.1  1999/06/14 21:57:32  donut
 * Import of d1x 1.37 source.
 *
 * Revision 1.3  1994/11/18  22:50:19  john
 * Changed shorts to ints in parameters.
 * 
 * Revision 1.2  1993/10/15  16:23:27  john
 * y
 * 
 * Revision 1.1  1993/09/08  11:44:22  john
 * Initial revision
 * 
 *
 */

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
