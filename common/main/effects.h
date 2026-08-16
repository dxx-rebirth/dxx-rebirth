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
 * Headerfile for effects.c
 *
 */

#pragma once

#include "vclip.h"
#include "dxxsconf.h"
#include "pack.h"
#include "bm.h"
#include "d_array.h"
#include "textures.h"

#ifdef DXX_BUILD_DESCENT
namespace dsx {
#if DXX_BUILD_DESCENT == 1
constexpr std::integral_constant<unsigned, 60> MAX_EFFECTS{};
#elif DXX_BUILD_DESCENT == 2
constexpr std::integral_constant<unsigned, 110> MAX_EFFECTS{};
#endif

enum class effect_index : uint8_t
{
	fuel_center = 2,
	boss = 53,
#if DXX_BUILD_DESCENT == 2
	diagonal_force_field = 78,	// diagonal force field texture
	straight_force_field = 93,	// straight force field texture
#endif
	None = UINT8_MAX,
};
}

//flags for eclips.  If no flags are set, always plays

#define EF_CRITICAL 1   //this doesn't get played directly (only when mine critical)
#define EF_ONE_SHOT 2   //this is a special that gets played once
#define EF_STOPPED  4   //this has been stopped

namespace dcx {

struct eclip : public prohibit_void_ptr<>
{
	vclip   vc;             //imbedded vclip
	fix     time_left;      //for sequencing
	uint32_t frame_count;    //for sequencing
	texture_index changing_wall_texture;      //Which element of Textures array to replace.
	union {
		::d1x::object_bitmap_index d1x;    //Which element of d1x::ObjBitmapPtrs array to replace.
		::d2x::object_bitmap_index d2x;    //Which element of d2x::ObjBitmapPtrs array to replace.
	} changing_object_texture;
	int     flags;          //see above
	texture_index dest_bm_num;    //use this bitmap when monitor destroyed
	effect_index crit_clip;      //use this clip instead of above one when mine critical
	effect_index dest_eclip;     //what eclip to play when exploding
	vclip_index dest_vclip;     //what vclip to play when exploding
	sound_effect sound_num;      //what sound this makes
	fix     dest_size;      //3d size of explosion
	segnum_t     segnum;
	sidenum_t sidenum; //what seg & side, for one-shot clips
};

}

extern unsigned Num_effects;

// Set up special effects.
extern void init_special_effects();

// Clear any active one-shots
void reset_special_effects();

// Function called in game loop to do effects.
extern void do_special_effects();

// Restore bitmap
extern void restore_effect_bitmap_icons();

#ifdef DXX_BUILD_DESCENT
namespace dsx {

namespace little_endian {

/*
 * reads n eclip structs from a PHYSFS_File
 */
void eclip_read(NamedPHYSFS_File fp, eclip &ec);

}
#if 0
void eclip_write(PHYSFS_File *fp, const eclip &ec);
#endif

using d_eclip_array = enumerated_array<eclip, MAX_EFFECTS, effect_index>;

struct d_level_unique_effects_clip_state
{
	d_eclip_array Effects;
};

extern d_level_unique_effects_clip_state LevelUniqueEffectsClipState;

//stop an effect from animating.  Show first frame.
void stop_boss_effect(d_eclip_array &Effects, object_bitmaps_array &ObjBitmaps, Textures_array &Textures);

//restart a stopped boss effect
void restart_boss_effect(d_eclip_array &);

}
#endif

#endif
