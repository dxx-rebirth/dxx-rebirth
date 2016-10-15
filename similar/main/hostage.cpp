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
 * Code to render and manipulate hostages
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dxxerror.h"
#include "inferno.h"
#include "object.h"
#include "hudmsg.h"
#include "game.h"
#include "player.h"
#include "gauges.h"
#include "hostage.h"
#include "vclip.h"
#include "newdemo.h"
#include "text.h"


//------------- Globaly used hostage variables --------------------------------

namespace dcx {
unsigned N_hostage_types;		  			// Number of hostage types
array<int, MAX_HOSTAGE_TYPES> Hostage_vclip_num;	// vclip num for each tpye of hostage
}

namespace dsx {

//-------------- Renders a hostage --------------------------------------------
void draw_hostage(const vobjptridx_t obj)
{
	draw_object_tmap_rod(obj, Vclip[obj->rtype.vclip_info.vclip_num].frames[obj->rtype.vclip_info.framenum], 1);
}


//------------- Called once when a hostage is rescued -------------------------
void hostage_rescue()
{
	PALETTE_FLASH_ADD(0, 0, 25);		//small blue flash

	auto &player_info = get_local_plrobj().ctype.player_info;
	++player_info.mission.hostages_on_board;

	// Do an audio effect
	if (Newdemo_state != ND_STATE_PLAYBACK)
		digi_play_sample(SOUND_HOSTAGE_RESCUED, F1_0);

	HUD_init_message_literal(HM_DEFAULT, TXT_HOSTAGE_RESCUED);
}

}
