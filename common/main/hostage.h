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
 * Header for hostage.c
 *
 */

#pragma once

#ifdef __cplusplus
#include "fwd-object.h"
#include "compiler-array.h"

#define HOSTAGE_SIZE        i2f(5)  // 3d size of a hostage

#define MAX_HOSTAGE_TYPES   1       //only one hostage bitmap
#if defined(DXX_BUILD_DESCENT_I)
namespace dsx {

#define MAX_HOSTAGES				10		//max per any one level

// 1 per hostage
struct hostage_data
{
	objnum_t		objnum;
	object_signature_t objsig;
};

extern array<hostage_data, MAX_HOSTAGES> Hostages;

#if DXX_USE_EDITOR
void hostage_init_all();
void hostage_compress_all();
int hostage_is_valid( int hostage_num );
int hostage_object_is_valid(vobjptridx_t objnum);
void hostage_init_info(vobjptridx_t objnum);
#endif
}
#elif defined(DXX_BUILD_DESCENT_II)
#if DXX_USE_EDITOR
static inline void hostage_init_all() {}
static inline void hostage_init_info(const objnum_t &) {}
#endif
#endif

namespace dcx {

extern unsigned N_hostage_types;
extern array<int, MAX_HOSTAGE_TYPES> Hostage_vclip_num;    // for each type of hostage

}

#ifdef dsx
namespace dsx {
void draw_hostage(grs_canvas &, vobjptridx_t obj);
void hostage_rescue();
}
#endif

#endif
