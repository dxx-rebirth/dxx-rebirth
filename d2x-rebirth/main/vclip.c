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
 * Routines for vclips.
 *
 */


#include <stdlib.h>
#include "dxxerror.h"
#include "inferno.h"
#include "vclip.h"
#include "weapon.h"
#include "laser.h"

//----------------- Variables for video clips -------------------
int 					Num_vclips = 0;
vclip 				Vclip[VCLIP_MAXNUM];		// General purpose vclips.

//draw an object which renders as a vclip
void draw_vclip_object(object *obj,fix timeleft,int lighted, int vclip_num)
{
	int nf,bitmapnum;

	nf = Vclip[vclip_num].num_frames;

	bitmapnum =  (nf - f2i(fixdiv( (nf-1)*timeleft,Vclip[vclip_num].play_time))) - 1;

	if (bitmapnum >= Vclip[vclip_num].num_frames)
		bitmapnum=Vclip[vclip_num].num_frames-1;

	if (bitmapnum >= 0 )	{

		if (Vclip[vclip_num].flags & VF_ROD)
			draw_object_tmap_rod(obj, Vclip[vclip_num].frames[bitmapnum],lighted);
		else {
			Assert(lighted==0);		//blob cannot now be lighted

			draw_object_blob(obj, Vclip[vclip_num].frames[bitmapnum] );
		}
	}

}


void draw_weapon_vclip(object *obj)
{
	int	vclip_num;
	fix	modtime,play_time;

	Assert(obj->type == OBJ_WEAPON);

	vclip_num = Weapon_info[obj->id].weapon_vclip;

	modtime = obj->lifeleft;
	play_time = Vclip[vclip_num].play_time;

	//	Special values for modtime were causing enormous slowdown for omega blobs.
	if (modtime == IMMORTAL_TIME)
		modtime = play_time;

	if (obj->id == PROXIMITY_ID) {		//make prox bombs spin out of sync
		int objnum = obj-Objects;

		modtime += (modtime * (objnum&7)) / 16;	//add variance to spin rate

		while (modtime > play_time)
			modtime -= play_time;

		if ((objnum&1) ^ ((objnum>>1)&1))			//make some spin other way
			modtime = play_time - modtime;

	}
	else {
		while (modtime > play_time)
			modtime -= play_time;
	}

	draw_vclip_object(obj, modtime, 0, vclip_num);

}

/*
 * reads n vclip structs from a PHYSFS_file
 */
int vclip_read_n(vclip *vc, int n, PHYSFS_file *fp)
{
	int i, j;

	for (i = 0; i < n; i++) {
		vc[i].play_time = PHYSFSX_readFix(fp);
		vc[i].num_frames = PHYSFSX_readInt(fp);
		vc[i].frame_time = PHYSFSX_readFix(fp);
		vc[i].flags = PHYSFSX_readInt(fp);
		vc[i].sound_num = PHYSFSX_readShort(fp);
		for (j = 0; j < VCLIP_MAX_FRAMES; j++)
			vc[i].frames[j].index = PHYSFSX_readShort(fp);
		vc[i].light_value = PHYSFSX_readFix(fp);
	}
	return i;
}
