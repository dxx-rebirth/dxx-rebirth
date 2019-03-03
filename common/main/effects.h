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
#include "compiler-array.h"
#include "pack.h"

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
namespace dsx {
#if defined(DXX_BUILD_DESCENT_I)
constexpr std::integral_constant<unsigned, 60> MAX_EFFECTS{};
constexpr std::integral_constant<uint32_t, UINT32_MAX> eclip_none{};
#elif defined(DXX_BUILD_DESCENT_II)
constexpr std::integral_constant<unsigned, 110> MAX_EFFECTS{};
constexpr std::integral_constant<uint16_t, UINT16_MAX> eclip_none{};
#endif
}

//flags for eclips.  If no flags are set, always plays

#define EF_CRITICAL 1   //this doesn't get played directly (only when mine critical)
#define EF_ONE_SHOT 2   //this is a special that gets played once
#define EF_STOPPED  4   //this has been stopped

#define ECLIP_NUM_FUELCEN     2
#define ECLIP_NUM_BOSS        53
#ifdef DXX_BUILD_DESCENT_II
#define ECLIP_NUM_FORCE_FIELD 78
#endif

struct eclip : public prohibit_void_ptr<eclip>
{
	vclip   vc;             //imbedded vclip
	fix     time_left;      //for sequencing
	uint32_t frame_count;    //for sequencing
	short   changing_wall_texture;      //Which element of Textures array to replace.
	short   changing_object_texture;    //Which element of ObjBitmapPtrs array to replace.
	int     flags;          //see above
	int     crit_clip;      //use this clip instead of above one when mine critical
	unsigned dest_bm_num;    //use this bitmap when monitor destroyed
	int     dest_vclip;     //what vclip to play when exploding
	int     dest_eclip;     //what eclip to play when exploding
	fix     dest_size;      //3d size of explosion
	int     sound_num;      //what sound this makes
	segnum_t     segnum;
	uint8_t sidenum; //what seg & side, for one-shot clips
};

extern unsigned Num_effects;

// Set up special effects.
extern void init_special_effects();

// Clear any active one-shots
void reset_special_effects();

// Function called in game loop to do effects.
extern void do_special_effects();

// Restore bitmap
extern void restore_effect_bitmap_icons();

//stop an effect from animating.  Show first frame.
void stop_effect(int effect_num);

//restart a stopped effect
void restart_effect(int effect_num);

/*
 * reads n eclip structs from a PHYSFS_File
 */
void eclip_read(PHYSFS_File *fp, eclip &ec);
#if 0
void eclip_write(PHYSFS_File *fp, const eclip &ec);
#endif

#ifdef dsx
namespace dsx {

using d_eclip_array = array<eclip, MAX_EFFECTS>;

struct d_level_unique_effects_clip_state
{
	d_eclip_array Effects;
};

extern d_level_unique_effects_clip_state LevelUniqueEffectsClipState;

}
#endif

#endif
