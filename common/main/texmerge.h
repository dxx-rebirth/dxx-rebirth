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
 * Definitions for texture merging caching stuff.
 *
 */


#ifndef _TEXMERGE_H
#define _TEXMERGE_H

#include "dsx-ns.h"
#include "fwd-piggy.h"	// GameBitmaps_array
#include "fwd-segment.h"
#include "textures.h"	// Textures_array

namespace dcx {
struct grs_bitmap;

void texmerge_close();
void texmerge_flush();

}

#ifdef DXX_BUILD_DESCENT
namespace dsx {

[[nodiscard]]
grs_bitmap &texmerge_get_cached_bitmap(GameBitmaps_array &GameBitmaps, const Textures_array &Textures, texture1_value tmap_bottom, texture2_value tmap_top);

}
#endif

#endif /* _TEXMERGE_H */
