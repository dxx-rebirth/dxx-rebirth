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
 * Routines to read/write pcx images.
 *
 */

#pragma once

#include "pstypes.h"
#include "dxxsconf.h"
#include "dsx-ns.h"
#include "fwd-gr.h"
#if !DXX_USE_OGL && DXX_USE_SCREENSHOT_FORMAT_LEGACY
#include <physfs.h>
#endif

namespace dcx {
struct palette_array_t;

enum class pcx_result
{
	SUCCESS = 0,
	ERROR_OPENING = 1,
	ERROR_NO_HEADER = 2,
	ERROR_WRONG_VERSION = 3,
	ERROR_READING = 4,
	ERROR_NO_PALETTE = 5,
	ERROR_WRITING = 6,
	ERROR_MEMORY = 7
};

// Reads filename into bitmap bmp, and fills in palette.  If bmp->bm_data==NULL,
// then bmp->bm_data is allocated and the w,h are filled.
// If palette==NULL the palette isn't read in.  Returns error code.

pcx_result pcx_read_bitmap(const char * filename, grs_main_bitmap &bmp, palette_array_t &palette);
pcx_result pcx_read_bitmap_or_default(const char * filename, grs_main_bitmap &bmp, palette_array_t &palette);

// Writes the bitmap bmp to filename, using palette. Returns error code.

#if !DXX_USE_OGL && DXX_USE_SCREENSHOT_FORMAT_LEGACY
unsigned pcx_write_bitmap(PHYSFS_File *, const grs_bitmap *bmp, palette_array_t &palette);
#endif

const char *pcx_errormsg(pcx_result error_number);

}

#if defined(DXX_BUILD_DESCENT_I)
namespace dsx {
// Load bitmap for little-known 'baldguy' cheat.
pcx_result bald_guy_load(const char *filename, grs_main_bitmap &bmp, palette_array_t &palette);
}
#endif
