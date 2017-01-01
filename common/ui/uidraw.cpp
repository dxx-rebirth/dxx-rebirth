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


#include "maths.h"
#include "pstypes.h"
#include "gr.h"
#include "ui.h"

namespace dcx {

void Hline(short x1, short x2, short y, const uint8_t color)
{
	gr_uline(*grd_curcanv, i2f(x1), i2f(y), i2f(x2), i2f(y), color);
}

void Vline(short y1, short y2, short x, const uint8_t color)
{
	gr_uline(*grd_curcanv, i2f(x), i2f(y1), i2f(x), i2f(y2), color);
}

void ui_string_centered( short x, short y, const char * s )
{
	int height, width;

	gr_get_string_size(s, &width, &height, nullptr);

	//baseline = height-grd_curcanv->cv_font->ft_baseline;

	gr_ustring(x-((width-1)/2), y-((height-1)/2), s );

}


void ui_draw_shad( short x1, short y1, short x2, short y2, short c1, short c2 )
{
	Hline(x1+0, x2-1, y1+0, c1);
	Vline(y1+1, y2+0, x1+0, c1);

	Hline(x1+1, x2, y2-0, c2);
	Vline(y1+0, y2-1, x2-0, c2);
}

void ui_draw_frame( short x1, short y1, short x2, short y2 )
{

	ui_draw_shad( x1+0, y1+0, x2-0, y2-0, CBRIGHT, CGREY );
	ui_draw_shad( x1+1, y1+1, x2-1, y2-1, CBRIGHT, CGREY );

	ui_draw_shad( x1+2, y1+2, x2-2, y2-2, CWHITE, CWHITE );
	ui_draw_shad( x1+3, y1+3, x2-3, y2-3, CWHITE, CWHITE );
	ui_draw_shad( x1+4, y1+4, x2-4, y2-4, CWHITE, CWHITE );
	ui_draw_shad( x1+5, y1+5, x2-5, y2-5, CWHITE, CWHITE );

	ui_draw_shad( x1+6, y1+6, x2-6, y2-6, CGREY, CBRIGHT );
	ui_draw_shad( x1+7, y1+7, x2-7, y2-7, CGREY, CBRIGHT );

}

void ui_draw_box_out( short x1, short y1, short x2, short y2 )
{
	const uint8_t color = CWHITE;
	gr_urect(*grd_curcanv, x1+2, y1+2, x2-2, y2-2, color);

	ui_draw_shad( x1+0, y1+0, x2-0, y2-0, CBRIGHT, CGREY );
	ui_draw_shad( x1+1, y1+1, x2-1, y2-1, CBRIGHT, CGREY );

}

void ui_draw_box_in( short x1, short y1, short x2, short y2 )
{
	ui_draw_shad( x1+0, y1+0, x2-0, y2-0, CGREY, CBRIGHT );
	ui_draw_shad( x1+1, y1+1, x2-1, y2-1, CGREY, CBRIGHT );
}

void ui_draw_line_in( short x1, short y1, short x2, short y2 )
{
	const uint8_t cgrey = CGREY;
	const uint8_t cbright = CBRIGHT;
	Hline(x1, x2, y1, cgrey);
	Hline(x1, x2-1, y2-1, cgrey);
	Vline(y1+1, y2-2, x1, cgrey);
	Vline(y1+1, y2-2, x2-1, cgrey);

	Hline(x1+1, x2-1, y1+1, cbright);
	Hline(x1, x2, y2, cbright);
	Vline(y1+2, y2-2, x1+1, cbright);
	Vline(y1+1, y2-1, x2, cbright);
}

}
