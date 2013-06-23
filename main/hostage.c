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
 * Code to render and manipulate hostages
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dxxerror.h"
#include "inferno.h"
#include "object.h"
#include "game.h"
#include "player.h"
#include "gauges.h"
#include "hostage.h"
#include "vclip.h"
#include "newdemo.h"
#include "text.h"


//------------- Globaly used hostage variables --------------------------------

int N_hostage_types = 0;		  			// Number of hostage types
int Hostage_vclip_num[MAX_HOSTAGE_TYPES];	// vclip num for each tpye of hostage


//-------------- Renders a hostage --------------------------------------------
void draw_hostage(object *obj)
{
	draw_object_tmap_rod(obj, Vclip[obj->rtype.vclip_info.vclip_num].frames[obj->rtype.vclip_info.framenum], 1);
}


//------------- Called once when a hostage is rescued -------------------------
void hostage_rescue(int blah)
{
	PALETTE_FLASH_ADD(0, 0, 25);		//small blue flash

	Players[Player_num].hostages_on_board++;

	// Do an audio effect
	if (Newdemo_state != ND_STATE_PLAYBACK)
		digi_play_sample(SOUND_HOSTAGE_RESCUED, F1_0);

	HUD_init_message_literal(HM_DEFAULT, TXT_HOSTAGE_RESCUED);
}
