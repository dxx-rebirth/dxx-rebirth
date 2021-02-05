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

#include "u_mem.h"
#include "gr.h"
#if DXX_USE_OGL
#include "ogl_init.h"
#endif

namespace dcx {

color_palette_index gr_ugpixel(const grs_bitmap &bitmap, int x, int y)
{
	switch (bitmap.get_type())
	{
		case bm_mode::linear:
			return bitmap.bm_data[ bitmap.bm_rowsize*y + x ];
#if DXX_USE_OGL
		case bm_mode::ogl:
			return ogl_ugpixel(bitmap, x, y);
#endif
	}
	return color_palette_index{0};
}

color_palette_index gr_gpixel(const grs_bitmap &bitmap, const unsigned x, const unsigned y)
{
	if (x >= bitmap.bm_w || y >= bitmap.bm_h)
		return color_palette_index{0};
	return gr_ugpixel(bitmap, x, y);
}

}
