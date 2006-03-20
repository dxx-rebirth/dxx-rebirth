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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/
/*
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/vclip.c,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:43:41 $
 * 
 * Routines for vclips.
 * 
 * $Log: vclip.c,v $
 * Revision 1.1.1.1  2006/03/17 19:43:41  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 22:11:54  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:32:41  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.8  1994/09/25  23:40:52  matt
 * Changed the object load & save code to read/write the structure fields one
 * at a time (rather than the whole structure at once).  This mean that the
 * object structure can be changed without breaking the load/save functions.
 * As a result of this change, the local_object data can be and has been 
 * incorporated into the object array.  Also, timeleft is now a property 
 * of all objects, and the object structure has been otherwise cleaned up.
 * 
 * Revision 1.7  1994/09/25  15:45:26  matt
 * Added OBJ_LIGHT, a type of object that casts light
 * Added generalized lifeleft, and moved it to local_object
 * 
 * Revision 1.6  1994/09/09  20:05:57  mike
 * Add vclips for weapons.
 * 
 * Revision 1.5  1994/06/14  21:14:35  matt
 * Made rod objects draw lighted or not depending on a parameter, so the
 * materialization effect no longer darkens.
 * 
 * Revision 1.4  1994/06/08  18:16:24  john
 * Bunch of new stuff that basically takes constants out of the code
 * and puts them into bitmaps.tbl.
 * 
 * Revision 1.3  1994/06/03  10:47:17  matt
 * Made vclips (used by explosions) which can be either rods or blobs, as
 * specified in BITMAPS.TBL.  (This is for the materialization center effect).
 * 
 * Revision 1.2  1994/05/11  09:25:25  john
 * Abandoned new vclip system for now because each wallclip, vclip,
 * etc, is different and it would be a huge pain to change all of them.
 * 
 * Revision 1.1  1994/05/10  15:21:12  john
 * Initial revision
 * 
 * 
 */


#ifdef RCS
#pragma off (unreferenced)
static char rcsid[] = "$Id: vclip.c,v 1.1.1.1 2006/03/17 19:43:41 zicodxx Exp $";
#pragma on (unreferenced)
#endif

#include "error.h"

#include "inferno.h"
#include "vclip.h"
#include "weapon.h"

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
	fix	modtime;

	//mprintf( 0, "[Drawing obj %d type %d fireball size %x]\n", obj-Objects, Weapon_info[obj->id].weapon_vclip, obj->size );

	vclip_num = Weapon_info[obj->id].weapon_vclip;
	modtime = obj->lifeleft;
	while (modtime > Vclip[vclip_num].play_time)
		modtime -= Vclip[vclip_num].play_time;

	draw_vclip_object(obj, modtime, 0, vclip_num);

}

