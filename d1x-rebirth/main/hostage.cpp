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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Code to render and manipulate hostages
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dxxerror.h"
#include "3d.h"
#include "object.h"
#include "game.h"
#include "player.h"
#include "hostage.h"
#include "lighting.h"
#include "sounds.h"
#include "vclip.h"
#include "newdemo.h"
#include "text.h"
#include "piggy.h"

#include "dxxsconf.h"
#include "dsx-ns.h"
#include "compiler-range_for.h"

namespace dsx {

array<hostage_data, MAX_HOSTAGES> 		Hostages;						// Data for each hostage in mine

//------------------- Useful macros and variables ---------------

int hostage_is_valid( int hostage_num )	{
	if ( hostage_num < 0 ) return 0;
	if ( hostage_num >= MAX_HOSTAGES ) return 0;
	if ( Hostages[hostage_num].objnum > Highest_object_index ) return 0;
	const auto &&objp = vcobjptr(Hostages[hostage_num].objnum);
	if (objp->type != OBJ_HOSTAGE)
		return 0;
	if (objp->signature != Hostages[hostage_num].objsig)
		return 0;
	if (get_hostage_id(objp) != hostage_num)
		return 0;
	return 1;
}

int hostage_object_is_valid(const vmobjptridx_t objnum)	{
	if ( objnum->type != OBJ_HOSTAGE ) return 0;
	return hostage_is_valid(get_hostage_id(objnum));
}


static int hostage_get_next_slot()	{
	for (int i=0; i<MAX_HOSTAGES; i++ ) {
		if (!hostage_is_valid(i))
			return i;
	}
	return MAX_HOSTAGES;
}

void hostage_init_info(const vmobjptridx_t objnum)
{
	int i;

	i = hostage_get_next_slot();
	Assert( i > -1 );
	Hostages[i].objnum = objnum;
	Hostages[i].objsig = objnum->signature;
	//Hostages[i].type = 0;
	//Hostages[i].sound_num = -1;
	set_hostage_id(objnum, i);
}

void hostage_init_all()
{
	// Initialize all their values...
	range_for (auto &i, Hostages)
	{
		i.objnum = object_none;
		//Hostages[i].type = 0;
		//Hostages[i].sound_num = -1;
	}

	//@@hostage_read_global_messages();
}

void hostage_compress_all()	{
	int newslot;
	
	for (int i=0; i<MAX_HOSTAGES; i++ ) {
		if ( hostage_is_valid(i) )	{
			newslot = hostage_get_next_slot();
			if ( newslot < i )	{
				Hostages[newslot] = Hostages[i];
				set_hostage_id(vmobjptr(Hostages[newslot].objnum), newslot);
				Hostages[i].objnum = object_none;
				i = 0;		// start over
			}
		}
	}
}

}
