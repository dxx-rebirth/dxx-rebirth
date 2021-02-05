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
#include "laser.h"
#include "vclip.h"
#include "physfs-serial.h"
#include "render.h"
#include "weapon.h"
#include "object.h"
#include "compiler-range_for.h"

//----------------- Variables for video clips -------------------
namespace dcx {
unsigned 					Num_vclips;
}

namespace dsx {
d_vclip_array Vclip;		// General purpose vclips.

//draw an object which renders as a vclip
void draw_vclip_object(grs_canvas &canvas, const vcobjptridx_t obj, const fix timeleft, const vclip &vc)
{
	const auto nf = vc.num_frames;
	int bitmapnum = (nf - f2i(fixdiv((nf - 1) * timeleft, vc.play_time))) - 1;

	if (bitmapnum >= vc.num_frames)
		bitmapnum = vc.num_frames - 1;

	if (bitmapnum >= 0 )	{
		if (vc.flags & VF_ROD)
			draw_object_tmap_rod(canvas, nullptr, obj, vc.frames[bitmapnum]);
		else {
			draw_object_blob(canvas, obj, vc.frames[bitmapnum]);
		}
	}
}

void draw_weapon_vclip(const d_vclip_array &Vclip, const weapon_info_array &Weapon_info, grs_canvas &canvas, const vcobjptridx_t obj)
{
	Assert(obj->type == OBJ_WEAPON);

	const auto lifeleft = obj->lifeleft;
	const auto vclip_num = Weapon_info[get_weapon_id(obj)].weapon_vclip;
	const auto play_time = Vclip[vclip_num].play_time;
	fix modtime = lifeleft % play_time;

	if (is_proximity_bomb_or_player_smart_mine_or_placed_mine(get_weapon_id(obj)))
	{		//make prox bombs spin out of sync
		int objnum = obj;

		modtime += (lifeleft * (objnum & 7)) / 16;	//add variance to spin rate

		if (modtime > play_time)
			modtime %= play_time;

		if ((objnum&1) ^ ((objnum>>1)&1))			//make some spin other way
			modtime = play_time - modtime;

	}

	draw_vclip_object(canvas, obj, modtime, Vclip[vclip_num]);
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
