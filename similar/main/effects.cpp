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
 * Special effects, such as rotating fans, electrical walls, and
 * other cool animations.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "gr.h"
#include "inferno.h"
#include "game.h"
#include "vclip.h"
#include "effects.h"
#include "bm.h"
#include "u_mem.h"
#include "textures.h"
#include "physfs-serial.h"
#include "segment.h"
#include "dxxerror.h"
#include "object.h"

#include "compiler-range_for.h"
#include "d_levelstate.h"
#include "partial_range.h"

unsigned Num_effects;

void init_special_effects()
{
	auto &Effects = LevelUniqueEffectsClipState.Effects;
	range_for (eclip &ec, partial_range(Effects, Num_effects))
		ec.time_left = ec.vc.frame_time;
}

void reset_special_effects()
{
	auto &Effects = LevelUniqueEffectsClipState.Effects;
	range_for (eclip &ec, partial_range(Effects, Num_effects))
	{
		ec.segnum = segment_none;					//clear any active one-shots
		ec.flags &= ~(EF_STOPPED|EF_ONE_SHOT);		//restart any stopped effects

		//reset bitmap, which could have been changed by a crit_clip
		if (ec.changing_wall_texture != -1)
			Textures[ec.changing_wall_texture] = ec.vc.frames[ec.frame_count];

		if (const auto changing_object_texture = ec.changing_object_texture; changing_object_texture != object_bitmap_index::None)
			ObjBitmaps[changing_object_texture] = ec.vc.frames[ec.frame_count];

	}
}

void do_special_effects()
{
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;
	auto &Effects = LevelUniqueEffectsClipState.Effects;
	range_for (eclip &ec, partial_range(Effects, Num_effects))
	{
		const auto vc_frame_time = ec.vc.frame_time;
		if (!vc_frame_time)
			continue;
		if (ec.changing_wall_texture == -1 && ec.changing_object_texture == object_bitmap_index::None)
			continue;

		if (ec.flags & EF_STOPPED)
			continue;

		ec.time_left -= FrameTime;

		while (ec.time_left < 0) {

			ec.time_left += vc_frame_time;
			
			ec.frame_count++;
			if (ec.frame_count >= ec.vc.num_frames) {
				if (ec.flags & EF_ONE_SHOT) {
					ec.flags &= ~EF_ONE_SHOT;
					unique_segment &seg = *vmsegptr(ec.segnum);
					ec.segnum = segment_none;		//done with this
					assert(ec.sidenum < 6);
					auto &side = seg.sides[ec.sidenum];
					assert(ec.dest_bm_num != 0 && side.tmap_num2 != texture2_value::None);
					side.tmap_num2 = build_texture2_value(ec.dest_bm_num, get_texture_rotation_high(side.tmap_num2));		//replace with destroyed
				}

				ec.frame_count = 0;
			}
		}

		if (ec.flags & EF_CRITICAL)
			continue;

		if (ec.crit_clip!=-1 && LevelUniqueControlCenterState.Control_center_destroyed)
		{
			int n = ec.crit_clip;

			if (ec.changing_wall_texture != -1)
				Textures[ec.changing_wall_texture] = Effects[n].vc.frames[Effects[n].frame_count];

			if (const auto changing_object_texture = ec.changing_object_texture; changing_object_texture != object_bitmap_index::None)
				ObjBitmaps[changing_object_texture] = Effects[n].vc.frames[Effects[n].frame_count];

		}
		else	{
			if (ec.changing_wall_texture != -1)
				Textures[ec.changing_wall_texture] = ec.vc.frames[ec.frame_count];
	
			if (const auto changing_object_texture = ec.changing_object_texture; changing_object_texture != object_bitmap_index::None)
				ObjBitmaps[changing_object_texture] = ec.vc.frames[ec.frame_count];
		}

	}
}

void restore_effect_bitmap_icons()
{
	auto &Effects = LevelUniqueEffectsClipState.Effects;
	range_for (auto &ec, partial_const_range(Effects, Num_effects))
	{
		if (! (ec.flags & EF_CRITICAL))	{
			if (ec.changing_wall_texture != -1)
				Textures[ec.changing_wall_texture] = ec.vc.frames[0];
	
			if (const auto changing_object_texture = ec.changing_object_texture; changing_object_texture != object_bitmap_index::None)
				ObjBitmaps[changing_object_texture] = ec.vc.frames[0];
		}
	}
}

//stop an effect from animating.  Show first frame.
void stop_effect(int effect_num)
{
	auto &Effects = LevelUniqueEffectsClipState.Effects;
	eclip *ec = &Effects[effect_num];
	
	//Assert(ec->bm_ptr != -1);

	ec->flags |= EF_STOPPED;

	ec->frame_count = 0;
	//*ec->bm_ptr = &GameBitmaps[ec->vc.frames[0].index];

	if (ec->changing_wall_texture != -1)
		Textures[ec->changing_wall_texture] = ec->vc.frames[0];
	
	if (const auto changing_object_texture = ec->changing_object_texture; changing_object_texture != object_bitmap_index::None)
		ObjBitmaps[changing_object_texture] = ec->vc.frames[0];
}

//restart a stopped effect
void restart_effect(int effect_num)
{
	auto &Effects = LevelUniqueEffectsClipState.Effects;
	Effects[effect_num].flags &= ~EF_STOPPED;

	//Assert(Effects[effect_num].bm_ptr != -1);
}

DEFINE_VCLIP_SERIAL_UDT();
DEFINE_SERIAL_UDT_TO_MESSAGE(eclip, ec, (ec.vc, ec.time_left, ec.frame_count, ec.changing_wall_texture, ec.changing_object_texture, ec.flags, ec.crit_clip, ec.dest_bm_num, ec.dest_vclip, ec.dest_eclip, ec.dest_size, ec.sound_num, ec.segnum, serial::pad<2>(), ec.sidenum, serial::pad<3>()));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(eclip, 130);

/*
 * reads n eclip structs from a PHYSFS_File
 */
void eclip_read(PHYSFS_File *fp, eclip &ec)
{
	PHYSFSX_serialize_read(fp, ec);
}

#if 0
void eclip_write(PHYSFS_File *fp, const eclip &ec)
{
	PHYSFSX_serialize_write(fp, ec);
}
#endif
