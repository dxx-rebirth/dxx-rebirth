/* $Id: hostage.c,v 1.3 2003-10-10 09:36:35 btb Exp $ */
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
 * Old Log:
 * Revision 1.1  1995/05/16  15:26:24  allender
 * Initial revision
 *
 * Revision 2.0  1995/02/27  11:28:36  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.65  1995/02/22  13:45:54  allender
 * remove anonymous unions from object structure
 *
 * Revision 1.64  1995/02/13  20:34:57  john
 * Lintized
 *
 * Revision 1.63  1995/01/15  19:41:48  matt
 * Ripped out hostage faces for registered version
 *
 * Revision 1.62  1995/01/14  19:16:53  john
 * First version of new bitmap paging code.
 *
 * Revision 1.61  1994/12/19  16:35:09  john
 * Made hoastage playback end when ship dies.
 *
 * Revision 1.60  1994/12/06  16:30:41  yuan
 * Localization
 *
 * Revision 1.59  1994/11/30  17:32:46  matt
 * Put hostage_face_clip array back in so editor would work
 *
 * Revision 1.58  1994/11/30  17:22:13  matt
 * Ripped out hostage faces in shareware version
 *
 * Revision 1.57  1994/11/30  16:11:25  matt
 * Use correct constant for hostage voice
 *
 * Revision 1.56  1994/11/27  23:15:19  matt
 * Made changes for new mprintf calling convention
 *
 * Revision 1.55  1994/11/19  19:53:44  matt
 * Added code to full support different hostage head clip & message for
 * each hostage.
 *
 * Revision 1.54  1994/11/19  16:35:15  matt
 * Got rid of unused code, & made an array smaller
 *
 * Revision 1.53  1994/11/14  12:42:03  matt
 * Increased palette flash when hostage rescued
 *
 * Revision 1.52  1994/10/28  14:43:09  john
 * Added sound volumes to all sound calls.
 *
 * Revision 1.51  1994/10/23  02:10:57  matt
 * Got rid of obsolete hostage_info stuff
 *
 * Revision 1.50  1994/10/22  00:08:44  matt
 * Fixed up problems with bonus & game sequencing
 * Player doesn't get credit for hostages unless he gets them out alive
 *
 * Revision 1.49  1994/10/20  22:52:49  matt
 * Fixed compiler warnings
 *
 * Revision 1.48  1994/10/20  21:25:44  matt
 * Took out silly scale down/scale up code for hostage anim
 *
 * Revision 1.47  1994/10/20  12:47:28  matt
 * Replace old save files (MIN/SAV/HOT) with new LVL files
 *
 * Revision 1.46  1994/10/04  15:33:33  john
 * Took out the old PLAY_SOUND??? code and replaced it
 * with direct calls into digi_link_??? so that all sounds
 * can be made 3d.
 *
 * Revision 1.45  1994/09/28  23:10:46  matt
 * Made hostage rescue do palette flash
 *
 * Revision 1.44  1994/09/20  00:11:00  matt
 * Finished gauges for Status Bar, including hostage video display.
 *
 * Revision 1.43  1994/09/15  21:24:19  matt
 * Changed system to keep track of whether & what cockpit is up
 * Made hostage clip not queue when no cockpit
 *
 *
 * Revision 1.42  1994/08/25  13:45:19  matt
 * Made hostage vclips queue
 *
 * Revision 1.41  1994/08/14  23:15:06  matt
 * Added animating bitmap hostages, and cleaned up vclips a bit
 *
 * Revision 1.40  1994/08/12  22:41:11  john
 * Took away Player_stats; add Players array.
 *
 * Revision 1.39  1994/07/14  22:06:35  john
 * Fix radar/hostage vclip conflict.
 *
 * Revision 1.38  1994/07/12  18:40:21  yuan
 * Tweaked location of radar and hostage screen...
 * Still needs work.
 *
 *
 * Revision 1.37  1994/07/07  09:52:17  john
 * Moved hostage screen.
 *
 * Revision 1.36  1994/07/06  15:23:52  john
 * Revamped hostage sound.
 *
 * Revision 1.35  1994/07/06  15:14:54  john
 * Added hostage sound effect picking.
 *
 * Revision 1.34  1994/07/06  13:25:33  john
 * Added compress hostages functions.
 *
 * Revision 1.33  1994/07/06  12:52:59  john
 * Fixed compiler warnings.
 *
 * Revision 1.32  1994/07/06  12:43:50  john
 * Made generic messages for hostages.
 *
 * Revision 1.31  1994/07/06  10:55:07  john
 * New structures for hostages.
 *
 * Revision 1.30  1994/07/05  12:49:09  john
 * Put functionality of New Hostage spec into code.
 *
 * Revision 1.29  1994/07/02  13:08:47  matt
 * Increment stats when hostage rescued
 *
 * Revision 1.28  1994/07/01  18:07:46  john
 * y
 *
 * Revision 1.27  1994/07/01  18:07:03  john
 * *** empty log message ***
 *
 * Revision 1.26  1994/07/01  17:55:26  john
 * First version of not-working hostage system.
 *
 * Revision 1.25  1994/06/27  15:53:21  john
 * #define'd out the newdemo stuff
 *
 *
 * Revision 1.24  1994/06/20  16:08:52  john
 * Added volume control; made doors 3d sounds.
 *
 * Revision 1.23  1994/06/16  10:15:32  yuan
 * Fixed location of face.
 *
 * Revision 1.22  1994/06/15  15:05:33  john
 * *** empty log message ***
 *
 * Revision 1.21  1994/06/14  21:15:20  matt
 * Made rod objects draw lighted or not depending on a parameter, so the
 * materialization effect no longer darkens.
 *
 * Revision 1.20  1994/06/08  18:16:26  john
 * Bunch of new stuff that basically takes constants out of the code
 * and puts them into bitmaps.tbl.
 *
 * Revision 1.19  1994/06/02  19:30:08  matt
 * Moved texture-mapped rod drawing stuff (used for hostage & now for the
 * materialization center) to object.c
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: hostage.c,v 1.3 2003-10-10 09:36:35 btb Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"

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
	//mprintf((0, "Rescued hostage %d", hostage_number));

	PALETTE_FLASH_ADD(0, 0, 25);		//small blue flash

	Players[Player_num].hostages_on_board++;

	// Do an audio effect
	if (Newdemo_state != ND_STATE_PLAYBACK)
		digi_play_sample(SOUND_HOSTAGE_RESCUED, F1_0);

	HUD_init_message(TXT_HOSTAGE_RESCUED);
}
