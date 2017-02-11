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
 * UI init and close functions.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "maths.h"
#include "pstypes.h"
#include "gr.h"
#include "key.h"
#include "ui.h"
#include "mouse.h"
#include "dxxerror.h"

namespace dcx {

static int Initialized = 0;

unsigned char CBLACK,CGREY,CWHITE,CBRIGHT,CRED;

grs_font_ptr ui_small_font;

int ui_init()
{

	if (Initialized) return 1;

	const grs_font *org_font = grd_curcanv->cv_font;
	ui_small_font = gr_init_font( "pc6x8.fnt" );
	if (!ui_small_font)
	{
		Warning("Could not find pc6x8.fnt");
		return 0;
	}
	grd_curcanv->cv_font =org_font;

	CBLACK = gr_find_closest_color( 1, 1, 1 );
	CGREY = gr_find_closest_color( 45, 45, 45 );
	CWHITE = gr_find_closest_color( 50, 50, 50 );
	CBRIGHT = gr_find_closest_color( 58, 58, 58 );
	CRED = gr_find_closest_color( 63, 0, 0 );
	
	//key_init();

	gr_set_fontcolor(*grd_curcanv, CBLACK, CWHITE);

	ui_pad_init();
	
	atexit(ui_close );

	Initialized = 1;

	return 1;
}

void ui_close()
{
	if (Initialized)
	{
		Initialized = 0;

		menubar_close();
		
		ui_pad_close();
		ui_small_font.reset();
	}

	return;
}

}
