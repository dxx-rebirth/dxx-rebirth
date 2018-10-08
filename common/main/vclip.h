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
 * Stuff for video clips.
 *
 */

#pragma once

#include "piggy.h"

#ifdef __cplusplus
#include "dxxsconf.h"
#include "dsx-ns.h"
#include "compiler-array.h"
#include "fwd-valptridx.h"


#define VCLIP_SMALL_EXPLOSION       2
#define VCLIP_PLAYER_HIT            1
#define VCLIP_MORPHING_ROBOT        10
#define VCLIP_PLAYER_APPEARANCE     61
#define VCLIP_POWERUP_DISAPPEARANCE 62
#define VCLIP_VOLATILE_WALL_HIT     5
#ifdef dsx
namespace dsx {
#if defined(DXX_BUILD_DESCENT_I)
constexpr std::integral_constant<std::size_t, 70> VCLIP_MAXNUM{};
#elif defined(DXX_BUILD_DESCENT_II)
#define VCLIP_WATER_HIT             84
#define VCLIP_AFTERBURNER_BLOB      95
#define VCLIP_MONITOR_STATIC        99

constexpr std::integral_constant<std::size_t, 110> VCLIP_MAXNUM{};
#endif
}

namespace dcx {
#define VCLIP_MAX_FRAMES            30

// vclip flags
#define VF_ROD      1       // draw as a rod, not a blob

struct vclip : public prohibit_void_ptr<vclip>
{
	fix             play_time;          // total time (in seconds) of clip
	unsigned        num_frames;
	fix             frame_time;         // time (in seconds) of each frame
	int             flags;
	short           sound_num;
	array<bitmap_index, VCLIP_MAX_FRAMES>    frames;
	fix             light_value;
};

constexpr std::integral_constant<int, -1> vclip_none{};

extern unsigned Num_vclips;

}

namespace dsx {
using d_vclip_array = array<vclip, VCLIP_MAXNUM>;
extern d_vclip_array Vclip;

// draw an object which renders as a vclip.
void draw_vclip_object(grs_canvas &, vcobjptridx_t obj, fix timeleft, int lighted, const vclip &);
void draw_weapon_vclip(grs_canvas &, vcobjptridx_t obj);
}

namespace dcx {
/*
 * reads n vclip structs from a PHYSFS_File
 */
void vclip_read(PHYSFS_File *fp, vclip &vc);
#if 0
void vclip_write(PHYSFS_File *fp, const vclip &vc);
#endif
}

/* Defer expansion to source file so that serial.h not needed here */
#define DEFINE_VCLIP_SERIAL_UDT()	\
	DEFINE_SERIAL_UDT_TO_MESSAGE(bitmap_index, bi, (bi.index));	\
	DEFINE_SERIAL_UDT_TO_MESSAGE(vclip, vc, (vc.play_time, vc.num_frames, vc.frame_time, vc.flags, vc.sound_num, vc.frames, vc.light_value));	\
	ASSERT_SERIAL_UDT_MESSAGE_SIZE(vclip, 82);
#endif

#endif
