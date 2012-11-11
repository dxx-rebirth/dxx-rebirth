/*
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
#include "cntrlcen.h"
#include "dxxerror.h"

int Num_effects;
eclip Effects[MAX_EFFECTS];

void init_special_effects()
{
	int i;

	for (i=0;i<Num_effects;i++)
		Effects[i].time_left = Effects[i].vc.frame_time;
}

void reset_special_effects()
{
	int i;

	for (i=0;i<Num_effects;i++) {
		Effects[i].segnum = -1;					//clear any active one-shots
		Effects[i].flags &= ~(EF_STOPPED|EF_ONE_SHOT);		//restart any stopped effects

		//reset bitmap, which could have been changed by a crit_clip
		if (Effects[i].changing_wall_texture != -1)
			Textures[Effects[i].changing_wall_texture] = Effects[i].vc.frames[Effects[i].frame_count];

		if (Effects[i].changing_object_texture != -1)
			ObjBitmaps[Effects[i].changing_object_texture] = Effects[i].vc.frames[Effects[i].frame_count];

	}
}

void do_special_effects()
{
	int i;
	eclip *ec;

	for (i=0,ec=Effects;i<Num_effects;i++,ec++) {

		if ((Effects[i].changing_wall_texture == -1) && (Effects[i].changing_object_texture==-1) )
			continue;

		if (ec->flags & EF_STOPPED)
			continue;

		ec->time_left -= FrameTime;

		while (ec->time_left < 0) {

			ec->time_left += ec->vc.frame_time;
			
			ec->frame_count++;
			if (ec->frame_count >= ec->vc.num_frames) {
				if (ec->flags & EF_ONE_SHOT) {
					Assert(ec->segnum!=-1);
					Assert(ec->sidenum>=0 && ec->sidenum<6);
					Assert(ec->dest_bm_num!=0 && Segments[ec->segnum].sides[ec->sidenum].tmap_num2!=0);
					Segments[ec->segnum].sides[ec->sidenum].tmap_num2 = ec->dest_bm_num | (Segments[ec->segnum].sides[ec->sidenum].tmap_num2&0xc000);		//replace with destoyed
					ec->flags &= ~EF_ONE_SHOT;
					ec->segnum = -1;		//done with this
				}

				ec->frame_count = 0;
			}
		}

		if (ec->flags & EF_CRITICAL)
			continue;

		if (ec->crit_clip!=-1 && Control_center_destroyed) {
			int n = ec->crit_clip;

			//*ec->bm_ptr = &GameBitmaps[Effects[n].vc.frames[Effects[n].frame_count].index];
			if (ec->changing_wall_texture != -1)
				Textures[ec->changing_wall_texture] = Effects[n].vc.frames[Effects[n].frame_count];

			if (ec->changing_object_texture != -1)
				ObjBitmaps[ec->changing_object_texture] = Effects[n].vc.frames[Effects[n].frame_count];

		}
		else	{
			// *ec->bm_ptr = &GameBitmaps[ec->vc.frames[ec->frame_count].index];
			if (ec->changing_wall_texture != -1)
				Textures[ec->changing_wall_texture] = ec->vc.frames[ec->frame_count];
	
			if (ec->changing_object_texture != -1)
				ObjBitmaps[ec->changing_object_texture] = ec->vc.frames[ec->frame_count];
		}

	}
}

void restore_effect_bitmap_icons()
{
	int i;
	
	for (i=0;i<Num_effects;i++)
		if (! (Effects[i].flags & EF_CRITICAL))	{
			if (Effects[i].changing_wall_texture != -1)
				Textures[Effects[i].changing_wall_texture] = Effects[i].vc.frames[0];
	
			if (Effects[i].changing_object_texture != -1)
				ObjBitmaps[Effects[i].changing_object_texture] = Effects[i].vc.frames[0];
		}
			//if (Effects[i].bm_ptr != -1)
			//	*Effects[i].bm_ptr = &GameBitmaps[Effects[i].vc.frames[0].index];
}

//stop an effect from animating.  Show first frame.
void stop_effect(int effect_num)
{
	eclip *ec = &Effects[effect_num];
	
	//Assert(ec->bm_ptr != -1);

	ec->flags |= EF_STOPPED;

	ec->frame_count = 0;
	//*ec->bm_ptr = &GameBitmaps[ec->vc.frames[0].index];

	if (ec->changing_wall_texture != -1)
		Textures[ec->changing_wall_texture] = ec->vc.frames[0];
	
	if (ec->changing_object_texture != -1)
		ObjBitmaps[ec->changing_object_texture] = ec->vc.frames[0];

}

//restart a stopped effect
void restart_effect(int effect_num)
{
	Effects[effect_num].flags &= ~EF_STOPPED;

	//Assert(Effects[effect_num].bm_ptr != -1);
}

/*
 * reads n eclip structs from a PHYSFS_file
 */
int eclip_read_n(eclip *ec, int n, PHYSFS_file *fp)
{
	int i;

	for (i = 0; i < n; i++) {
		vclip_read_n(&ec[i].vc, 1, fp);
		ec[i].time_left = PHYSFSX_readFix(fp);
		ec[i].frame_count = PHYSFSX_readInt(fp);
		ec[i].changing_wall_texture = PHYSFSX_readShort(fp);
		ec[i].changing_object_texture = PHYSFSX_readShort(fp);
		ec[i].flags = PHYSFSX_readInt(fp);
		ec[i].crit_clip = PHYSFSX_readInt(fp);
		ec[i].dest_bm_num = PHYSFSX_readInt(fp);
		ec[i].dest_vclip = PHYSFSX_readInt(fp);
		ec[i].dest_eclip = PHYSFSX_readInt(fp);
		ec[i].dest_size = PHYSFSX_readFix(fp);
		ec[i].sound_num = PHYSFSX_readInt(fp);
		ec[i].segnum = PHYSFSX_readInt(fp);
		ec[i].sidenum = PHYSFSX_readInt(fp);
	}
	return i;
}
