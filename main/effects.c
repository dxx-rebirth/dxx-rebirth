/* $Id: effects.c,v 1.5 2003-10-10 09:36:34 btb Exp $ */
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
 * Old Log:
 * Revision 1.1  1995/05/16  15:24:25  allender
 * Initial revision
 *
 * Revision 2.0  1995/02/27  11:32:49  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.21  1995/02/13  20:35:06  john
 * Lintized
 *
 * Revision 1.20  1994/12/10  16:44:50  matt
 * Added debugging code to track down door that turns into rock
 *
 * Revision 1.19  1994/12/06  16:27:14  matt
 * Fixed horrible bug that was referencing segment -1
 *
 * Revision 1.18  1994/12/02  23:20:51  matt
 * Reset bitmaps possibly changed by crit clips
 *
 * Revision 1.17  1994/11/14  14:00:19  matt
 * Fixed stupid bug
 *
 * Revision 1.16  1994/11/14  12:42:43  matt
 * Allow holes in effects list
 *
 * Revision 1.15  1994/11/08  21:11:52  matt
 * Added functions to stop & start effects
 *
 * Revision 1.14  1994/10/04  18:59:08  matt
 * Exploding eclips now play eclip while exploding, then switch to static bm
 *
 * Revision 1.13  1994/10/04  15:17:42  matt
 * Took out references to unused constant
 *
 * Revision 1.12  1994/09/29  11:00:01  matt
 * Made eclips (wall animations) not frame-rate dependent (for now)
 *
 * Revision 1.11  1994/09/25  00:40:24  matt
 * Added the ability to make eclips (monitors, fans) which can be blown up
 *
 * Revision 1.10  1994/08/14  23:15:14  matt
 * Added animating bitmap hostages, and cleaned up vclips a bit
 *
 * Revision 1.9  1994/08/05  15:56:04  matt
 * Cleaned up effects system, and added alternate effects for after mine
 * destruction.
 *
 * Revision 1.8  1994/08/01  23:17:21  matt
 * Add support for animating textures on robots
 *
 * Revision 1.7  1994/05/23  15:10:46  yuan
 * Make Eclips read directly...
 * No more need for $EFFECTS list.
 *
 * Revision 1.6  1994/04/06  14:42:44  yuan
 * Adding new powerups.
 *
 * Revision 1.5  1994/03/15  16:31:54  yuan
 * Cleaned up bm-loading code.
 * (And structures)
 *
 * Revision 1.4  1994/03/04  17:09:09  yuan
 * New door stuff.
 *
 * Revision 1.3  1994/01/11  11:18:50  yuan
 * Fixed frame_count
 *
 * Revision 1.2  1994/01/11  10:38:55  yuan
 * Special effects new implementation
 *
 * Revision 1.1  1994/01/10  09:45:29  yuan
 * Initial revision
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: effects.c,v 1.5 2003-10-10 09:36:34 btb Exp $";
#endif

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
#include "mono.h"
#include "u_mem.h"
#include "textures.h"
#include "cntrlcen.h"
#include "error.h"

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

#ifndef FAST_FILE_IO
/*
 * reads n eclip structs from a CFILE
 */
int eclip_read_n(eclip *ec, int n, CFILE *fp)
{
	int i;

	for (i = 0; i < n; i++) {
		vclip_read_n(&ec[i].vc, 1, fp);
		ec[i].time_left = cfile_read_fix(fp);
		ec[i].frame_count = cfile_read_int(fp);
		ec[i].changing_wall_texture = cfile_read_short(fp);
		ec[i].changing_object_texture = cfile_read_short(fp);
		ec[i].flags = cfile_read_int(fp);
		ec[i].crit_clip = cfile_read_int(fp);
		ec[i].dest_bm_num = cfile_read_int(fp);
		ec[i].dest_vclip = cfile_read_int(fp);
		ec[i].dest_eclip = cfile_read_int(fp);
		ec[i].dest_size = cfile_read_fix(fp);
		ec[i].sound_num = cfile_read_int(fp);
		ec[i].segnum = cfile_read_int(fp);
		ec[i].sidenum = cfile_read_int(fp);
	}
	return i;
}
#endif
