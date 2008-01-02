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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/ui/ui.c,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:39:06 $
 *
 * UI init and close functions.
 *
 * $Log: ui.c,v $
 * Revision 1.1.1.1  2006/03/17 19:39:06  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 22:14:42  donut
 * Import of d1x 1.37 source.
 *
 * Revision 1.9  1994/11/13  15:37:18  john
 * Made ui_close not close key and mouse.
 * 
 * Revision 1.8  1994/10/27  09:58:53  john
 * *** empty log message ***
 * 
 * Revision 1.7  1994/10/27  09:43:00  john
 * Fixed colors after removing gr_inverse_table.
 * 
 * Revision 1.6  1994/09/23  14:57:41  john
 * Took out calls to init_mouse and init_keyboard.
 * 
 * Revision 1.5  1993/12/10  14:16:45  john
 * made buttons have 2 user-functions.
 * 
 * Revision 1.4  1993/12/07  12:31:32  john
 * new version.
 * 
 * Revision 1.3  1993/10/26  13:46:12  john
 * *** empty log message ***
 * 
 * Revision 1.2  1993/10/05  17:31:55  john
 * *** empty log message ***
 * 
 * Revision 1.1  1993/09/20  10:35:49  john
 * Initial revision
 * 
 *
 */

#ifdef RCS
static char rcsid[] = "$Id: ui.c,v 1.1.1.1 2006/03/17 19:39:06 zicodxx Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#ifdef __MSDOS__
#include <dos.h>
#endif

#include "fix.h"
#include "pstypes.h"
#include "gr.h"
#include "key.h"
#include "ui.h"

#include "mouse.h"

#include "mono.h"

extern void InstallErrorHandler();

static int Initialized = 0;

unsigned char CBLACK,CGREY,CWHITE,CBRIGHT,CRED;

grs_font * ui_small_font = NULL;

void ui_init()
{
	grs_font * org_font;

	if (Initialized) return;

	Initialized = 1;

	org_font = grd_curcanv->cv_font;
	ui_small_font = gr_init_font( "pc6x8.fnt" );
	grd_curcanv->cv_font =org_font;

	CBLACK = gr_find_closest_color( 1, 1, 1 );
	CGREY = gr_find_closest_color( 45, 45, 45 );
	CWHITE = gr_find_closest_color( 50, 50, 50 );
	CBRIGHT = gr_find_closest_color( 58, 58, 58 );
	CRED = gr_find_closest_color( 63, 0, 0 );

	//key_init();

	ui_mouse_init();

	gr_set_fontcolor( CBLACK, CWHITE );

	CurWindow = NULL;

	InstallErrorHandler();

	ui_pad_init();
	
	atexit(ui_close );

}

void ui_close()
{
	if (Initialized)
	{
		Initialized = 0;

		ui_pad_close();

		ui_mouse_close();

//		mouse_close();
// 	key_close();

#ifdef __WATCOMC__
		_harderr( NULL );
#endif

		gr_close_font( ui_small_font );

	}

	return;
}

