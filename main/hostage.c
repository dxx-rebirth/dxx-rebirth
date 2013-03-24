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
 *
 * Code to render and manipulate hostages
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dxxerror.h"
#include "3d.h"
#include "inferno.h"
#include "object.h"
#include "game.h"
#include "player.h"
#include "fireball.h"
#include "gauges.h"
#include "hostage.h"
#include "lighting.h"
#include "sounds.h"
#include "vclip.h"
#include "newdemo.h"
#include "text.h"
#include "piggy.h"
#include "playsave.h"

//------------- Globaly used hostage variables --------------------------------------------------
int N_hostage_types = 0;		  			// Number of hostage types
int Hostage_vclip_num[MAX_HOSTAGE_TYPES];	// vclip num for each tpye of hostage
hostage_data 		Hostages[MAX_HOSTAGES];						// Data for each hostage in mine

//-------------- Renders a hostage ----------------------------------------------------------------
void draw_hostage(object *obj)
{
	Assert( obj->id < MAX_HOSTAGES );

	draw_object_tmap_rod(obj, Vclip[obj->rtype.vclip_info.vclip_num].frames[obj->rtype.vclip_info.framenum], 1);

}

//------------- Called once when a hostage is rescued ------------------------------------------
void hostage_rescue( int hostage_number )
{
	if ( (hostage_number<0) || (hostage_number>=MAX_HOSTAGES) )	{
			Int3();		// Get John!
			return;
	}

	PALETTE_FLASH_ADD(0, 0, 25);		//small blue flash

	Players[Player_num].hostages_on_board++;

	// Do an audio effect
	if (Newdemo_state != ND_STATE_PLAYBACK)
		digi_play_sample(SOUND_HOSTAGE_RESCUED, F1_0);

	HUD_init_message(HM_DEFAULT, TXT_HOSTAGE_RESCUED);
 }

#define LINEBUF_SIZE 100

//------------------- Useful macros and variables ---------------

int hostage_is_valid( int hostage_num )	{
	if ( hostage_num < 0 ) return 0;
	if ( hostage_num >= MAX_HOSTAGES ) return 0;
	if ( Hostages[hostage_num].objnum < 0 ) return 0;
	if ( Hostages[hostage_num].objnum > Highest_object_index ) return 0;
	if ( Objects[Hostages[hostage_num].objnum].type != OBJ_HOSTAGE ) return 0;
	if ( Objects[Hostages[hostage_num].objnum].signature != Hostages[hostage_num].objsig ) return 0;
	if ( Objects[Hostages[hostage_num].objnum].id != hostage_num) return 0;
	return 1;
}

int hostage_object_is_valid( int objnum )	{
	if ( objnum < 0 ) return 0;
	if ( objnum > Highest_object_index ) return 0;
	if ( Objects[objnum].type != OBJ_HOSTAGE ) return 0;
	return hostage_is_valid(Objects[objnum].id);
}


static int hostage_get_next_slot()	{
	int i;
	for (i=0; i<MAX_HOSTAGES; i++ )	{
		if (!hostage_is_valid(i))
			return i;
	}
	return MAX_HOSTAGES;
}

void hostage_init_info( int objnum )	{
	int i;

	i = hostage_get_next_slot();
	Assert( i > -1 );
	Hostages[i].objnum = objnum;
	Hostages[i].objsig = Objects[objnum].signature;
	//Hostages[i].type = 0;
	//Hostages[i].sound_num = -1;
	Objects[objnum].id = i;
}

void hostage_init_all()
{
	int i;

	// Initialize all their values...
	for (i=0; i<MAX_HOSTAGES; i++ )	{
		Hostages[i].objnum = -1;
		Hostages[i].objsig = -1;
		//Hostages[i].type = 0;
		//Hostages[i].sound_num = -1;
	}

	//@@hostage_read_global_messages();
}


#ifdef EDITOR

void hostage_compress_all()	{
	int i,newslot;
	
	for (i=0; i<MAX_HOSTAGES; i++ )	{
		if ( hostage_is_valid(i) )	{
			newslot = hostage_get_next_slot();
			if ( newslot < i )	{
				Hostages[newslot] = Hostages[i];
				Objects[Hostages[newslot].objnum].id = newslot;
				Hostages[i].objnum = -1;
				i = 0;		// start over
			}
		}
	}

	for (i=0; i<MAX_HOSTAGES; i++ )	{
		if ( hostage_is_valid(i) )	
			;
	}
}

#endif
