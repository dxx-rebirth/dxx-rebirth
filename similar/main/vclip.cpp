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
 * Routines for vclips.
 *
 */


#include <stdlib.h>
#include "dxxerror.h"
#include "inferno.h"
#include "vclip.h"
#include "physfs-serial.h"
#include "weapon.h"
#include "object.h"
#if defined(DXX_BUILD_DESCENT_II)
#include "laser.h"
#endif

#include "compiler-range_for.h"

//----------------- Variables for video clips -------------------
namespace dcx {
unsigned 					Num_vclips;
}

namespace dsx {
array<vclip, VCLIP_MAXNUM> 				Vclip;		// General purpose vclips.

//draw an object which renders as a vclip
void draw_vclip_object(const vobjptridx_t obj,fix timeleft,int lighted, int vclip_num)
{
	int nf,bitmapnum;

	nf = Vclip[vclip_num].num_frames;

	bitmapnum =  (nf - f2i(fixdiv( (nf-1)*timeleft,Vclip[vclip_num].play_time))) - 1;

	if (bitmapnum >= Vclip[vclip_num].num_frames)
		bitmapnum=Vclip[vclip_num].num_frames-1;

	if (bitmapnum >= 0 )	{

		if (Vclip[vclip_num].flags & VF_ROD)
			draw_object_tmap_rod(*grd_curcanv, obj, Vclip[vclip_num].frames[bitmapnum],lighted);
		else {
			Assert(lighted==0);		//blob cannot now be lighted
			draw_object_blob(*grd_curcanv, obj, Vclip[vclip_num].frames[bitmapnum] );
		}
	}
}


void draw_weapon_vclip(const vobjptridx_t obj)
{
	int	vclip_num;
	fix	modtime,play_time;

	Assert(obj->type == OBJ_WEAPON);

	vclip_num = Weapon_info[get_weapon_id(obj)].weapon_vclip;

	modtime = obj->lifeleft;
	play_time = Vclip[vclip_num].play_time;

#if defined(DXX_BUILD_DESCENT_II)
	//	Special values for modtime were causing enormous slowdown for omega blobs.
	if (modtime == IMMORTAL_TIME)
		modtime = play_time;

	if (get_weapon_id(obj) == weapon_id_type::PROXIMITY_ID) {		//make prox bombs spin out of sync
		int objnum = obj;

		modtime += (modtime * (objnum&7)) / 16;	//add variance to spin rate

		while (modtime > play_time)
			modtime -= play_time;

		if ((objnum&1) ^ ((objnum>>1)&1))			//make some spin other way
			modtime = play_time - modtime;

	}
	else
#endif
		while (modtime > play_time)
			modtime -= play_time;

	draw_vclip_object(obj, modtime, 0, vclip_num);

}

}

DEFINE_VCLIP_SERIAL_UDT();

namespace dcx {
void vclip_read(PHYSFS_File *fp, vclip &vc)
{
	PHYSFSX_serialize_read(fp, vc);
}

#if 0
void vclip_write(PHYSFS_File *fp, const vclip &vc)
{
	PHYSFSX_serialize_write(fp, vc);
}
#endif
}
