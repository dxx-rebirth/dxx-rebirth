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
#include "fwd-valptridx.h"
#include "fwd-vclip.h"
#include "fwd-weapon.h"

#ifdef dsx
namespace dcx {

enum class vclip_index : uint8_t
{
	player_hit = 1,
	volatile_wall_hit = 5,
	small_explosion = 2,
	morphing_robot = 10,
	big_player_explosion = 58,
	player_appearance = 61,
	powerup_disappearance = 62,
	/* if DXX_BUILD_DESCENT_II */
	water_hit = 84,
	afterburner_blob = 95,
	monitor_static = 99,
	/* endif */
	None = UINT8_MAX
};

// vclip flags
#define VF_ROD      1       // draw as a rod, not a blob

struct vclip : public prohibit_void_ptr<vclip>
{
	fix             play_time;          // total time (in seconds) of clip
	unsigned        num_frames;
	fix             frame_time;         // time (in seconds) of each frame
	uint8_t         flags;
	short           sound_num;
	std::array<bitmap_index, 30>    frames;
	fix             light_value;
};

}

namespace dsx {
// draw an object which renders as a vclip.
void draw_vclip_object(grs_canvas &, vcobjptridx_t obj, fix timeleft, const vclip &);
void draw_weapon_vclip(const d_vclip_array &Vclip, const weapon_info_array &Weapon_info, grs_canvas &, vcobjptridx_t obj);
}

namespace dcx {
/*
 * reads n vclip structs from a PHYSFS_File
 */
void vclip_read(NamedPHYSFS_File fp, vclip &vc);
#if 0
void vclip_write(PHYSFS_File *fp, const vclip &vc);
#endif
}

/* Defer expansion to source file so that serial.h not needed here */
#define DEFINE_VCLIP_SERIAL_UDT()	\
	/* DEFINE_SERIAL_UDT_TO_MESSAGE(bitmap_index, bi, (bi.index)); */	\
	DEFINE_SERIAL_UDT_TO_MESSAGE(vclip, vc, (vc.play_time, vc.num_frames, vc.frame_time, vc.flags, serial::pad<3>(), vc.sound_num, vc.frames, vc.light_value));	\
	ASSERT_SERIAL_UDT_MESSAGE_SIZE(vclip, 82);
#endif

#endif
