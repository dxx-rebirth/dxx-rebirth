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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/switch.c,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:42:24 $
 * 
 * New Triggers and Switches.
 * 
 * $Log: switch.c,v $
 * Revision 1.1.1.1  2006/03/17 19:42:24  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 22:11:41  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.1  1995/03/21  14:39:08  john
 * Ifdef'd out the NETWORK code.
 * 
 * Revision 2.0  1995/02/27  11:28:41  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.51  1995/01/31  15:26:23  rob
 * Don't trigger matcens in anarchy games.
 * 
 * Revision 1.50  1995/01/26  12:18:26  rob
 * Changed network_do_frame call.
 * 
 * Revision 1.49  1995/01/18  18:50:35  allender
 * don't process triggers if in demo playback mode.  Fix for Rob to only do
 * multi_send_endlevel_start if in multi player game
 * 
 * Revision 1.48  1995/01/13  11:59:40  rob
 * Added palette fade after secret level exit.
 * 
 * Revision 1.47  1995/01/12  17:00:41  rob
 * Fixed a problem with switches and secret levels.
 * 
 * Revision 1.46  1995/01/12  13:35:11  rob
 * Added data flush after secret level exit.
 * 
 * Revision 1.45  1995/01/03  15:25:11  rob
 * Fixed a compile error.
 * 
 * Revision 1.44  1995/01/03  15:12:02  rob
 * Adding multiplayer switching.
 * 
 * Revision 1.43  1994/11/29  16:52:12  yuan
 * Removed some obsolete commented out code.
 * 
 * Revision 1.42  1994/11/27  23:15:07  matt
 * Made changes for new mprintf calling convention
 * 
 * Revision 1.41  1994/11/22  18:36:45  rob
 * Added new hook for endlevel for secret doors.
 * 
 * Revision 1.40  1994/11/21  17:29:43  matt
 * Cleaned up sequencing & game saving for secret levels
 * 
 * Revision 1.39  1994/11/19  15:20:32  mike
 * rip out unused code and data
 * 
 * Revision 1.38  1994/10/25  16:09:52  yuan
 * Fixed byte bug.
 * 
 * Revision 1.37  1994/10/24  16:05:28  matt
 * Removed clear of fuelcen_control_center_destroyed
 * 
 * Revision 1.36  1994/10/08  14:21:13  matt
 * Added include
 * 
 * Revision 1.35  1994/10/07  12:34:09  matt
 * Added code fot going to/from secret levels
 * 
 * Revision 1.34  1994/10/05  15:16:10  rob
 * Used to be that only player #0 could trigger switches, now only the
 * LOCAL player can do it (and he's expected to tell the other guy with
 * a com message if its important!)
 * 
 * Revision 1.33  1994/09/24  17:42:03  mike
 * Kill temporary version of function written by Yuan, replaced by MK.
 * 
 * Revision 1.32  1994/09/24  17:10:00  yuan
 * Added Matcen triggers.
 * 
 * Revision 1.31  1994/09/23  18:02:21  yuan
 * Completed wall checking.
 * 
 * Revision 1.30  1994/08/19  20:09:41  matt
 * Added end-of-level cut scene with external scene
 * 
 * Revision 1.29  1994/08/18  10:47:36  john
 * Cleaned up game sequencing and player death stuff
 * in preparation for making the player explode into
 * pieces when dead.
 * 
 * Revision 1.28  1994/08/12  22:42:11  john
 * Took away Player_stats; added Players array.
 * 
 * Revision 1.27  1994/07/02  13:50:44  matt
 * Cleaned up includes
 * 
 * Revision 1.26  1994/06/27  16:32:25  yuan
 * Commented out incomplete code...
 * 
 * Revision 1.25  1994/06/27  15:53:27  john
 * #define'd out the newdemo stuff
 * 
 * 
 * Revision 1.24  1994/06/27  15:10:04  yuan
 * Might mess up triggers.
 * 
 * Revision 1.23  1994/06/24  17:01:43  john
 * Add VFX support; Took Game Sequencing, like EndGame and stuff and
 * took it out of game.c and into gameseq.c
 * 
 * Revision 1.22  1994/06/16  16:20:15  john
 * Made player start out in physics mode; Neatend up game loop a bit.
 * 
 * Revision 1.21  1994/06/15  14:57:22  john
 * Added triggers to demo recording.
 * 
 * Revision 1.20  1994/06/10  17:44:25  mike
 * Assert on result of find_connect_side == -1
 * 
 * Revision 1.19  1994/06/08  10:20:15  yuan
 * Removed unused testing.
 * 
 * 
 * Revision 1.18  1994/06/07  13:10:48  yuan
 * Fixed bug in check trigger... Still working on other bugs.
 * 
 * Revision 1.17  1994/05/30  20:22:04  yuan
 * New triggers.
 * 
 * Revision 1.16  1994/05/27  10:32:46  yuan
 * New dialog boxes (Walls and Triggers) added.
 * 
 * 
 * Revision 1.15  1994/05/25  18:06:46  yuan
 * Making new dialog box controls for walls and triggers.
 * 
 * Revision 1.14  1994/05/10  19:05:32  yuan
 * Made end of level flag rather than menu popping up
 * 
 * Revision 1.13  1994/04/29  15:05:25  yuan
 * Added menu pop-up at exit trigger.
 * 
 */


#ifdef RCS
#pragma off (unreferenced)
static char rcsid[] = "$Id: switch.c,v 1.1.1.1 2006/03/17 19:42:24 zicodxx Exp $";
#pragma on (unreferenced)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "gauges.h"
#include "game.h"
#include "switch.h"
#include "inferno.h"
#ifdef EDITOR
#include "editor/editor.h"
#endif
#include "segment.h"
#include "error.h"
#include "gameseg.h"
#include "mono.h"
#include "wall.h"
#include "texmap.h"
#include "fuelcen.h"
#include "newdemo.h"
#include "player.h"
#include "endlevel.h"
#include "gameseq.h"
#include "multi.h"
#include "network.h"
#include "palette.h"

trigger Triggers[MAX_TRIGGERS];
int Num_triggers;

//link Links[MAX_WALL_LINKS];
//int Num_links;

#ifdef EDITOR
fix trigger_time_count=F1_0;

//-----------------------------------------------------------------
// Initializes all the switches.
void trigger_init()
{
	int i;

	Num_triggers = 0;
//	Num_links = 0;	

	for (i=0;i<MAX_TRIGGERS;i++)
		{
		Triggers[i].type = 0;
		Triggers[i].flags = 0;
		Triggers[i].value = 0;
		Triggers[i].link_num = -1;
		Triggers[i].time = -1;
		}

//	for (i=0;i<MAX_WALL_LINKS;i++)
//		Links[i].num_walls = 0;
}
#endif

//-----------------------------------------------------------------
// Executes a link, attached to a trigger.
// Toggles all walls linked to the switch.
// Opens doors, Blasts blast walls, turns off illusions.
void do_link(sbyte trigger_num)
{
	int i;

	mprintf((0, "Door link!\n"));

	if (trigger_num != -1) {
		for (i=0;i<Triggers[trigger_num].num_links;i++) {
			wall_toggle(&Segments[Triggers[trigger_num].seg[i]], Triggers[trigger_num].side[i]); 
			mprintf((0," trigger_num %d : seg %d, side %d\n", 
				trigger_num, Triggers[trigger_num].seg[i], Triggers[trigger_num].side[i]));
  		}
  	}
}

void do_matcen(sbyte trigger_num)
{
	int i;

	mprintf((0, "Matcen link!\n"));

	if (trigger_num != -1) {
		for (i=0;i<Triggers[trigger_num].num_links;i++) {
			trigger_matcen(Triggers[trigger_num].seg[i] ); 
			mprintf((0," trigger_num %d : seg %d\n", 
				trigger_num, Triggers[trigger_num].seg[i]));
  		}
  	}
}

	
void do_il_on(sbyte trigger_num)
{
	int i;

	mprintf((0, "Illusion ON\n"));

	if (trigger_num != -1) {
		for (i=0;i<Triggers[trigger_num].num_links;i++) {
			wall_illusion_on(&Segments[Triggers[trigger_num].seg[i]], Triggers[trigger_num].side[i]); 
			mprintf((0," trigger_num %d : seg %d, side %d\n", 
				trigger_num, Triggers[trigger_num].seg[i], Triggers[trigger_num].side[i]));
  		}
  	}
}

void do_il_off(sbyte trigger_num)
{
	int i;
	
	mprintf((0, "Illusion OFF\n"));

	if (trigger_num != -1) {
		for (i=0;i<Triggers[trigger_num].num_links;i++) {
			wall_illusion_off(&Segments[Triggers[trigger_num].seg[i]], Triggers[trigger_num].side[i]); 
			mprintf((0," trigger_num %d : seg %d, side %d\n", 
				trigger_num, Triggers[trigger_num].seg[i], Triggers[trigger_num].side[i]));
  		}
  	}
}

int check_trigger_sub(int trigger_num, int pnum)
{

	if (pnum == Player_num) {
		if (Triggers[trigger_num].flags & TRIGGER_SHIELD_DAMAGE) {
			Players[Player_num].shields -= Triggers[trigger_num].value;
			mprintf((0,"BZZT!\n"));
		}

		if (Triggers[trigger_num].flags & TRIGGER_EXIT) {
			start_endlevel_sequence();
			mprintf((0,"WOOHOO! (leaving the mine!)\n"));
		}

		if (Triggers[trigger_num].flags & TRIGGER_SECRET_EXIT) {
			if (Newdemo_state == ND_STATE_RECORDING)		// stop demo recording
				Newdemo_state = ND_STATE_PAUSED;

			Fuelcen_control_center_destroyed = 0;
			mprintf((0,"Exiting to secret level\n"));
			#ifdef NETWORK
			if (Game_mode & GM_MULTI)
				multi_send_endlevel_start(1);
			#endif
#ifdef NETWORK
			if (Game_mode & GM_NETWORK)
				network_do_frame(1, 1);
#endif
			gr_palette_fade_out(gr_palette, 32, 0);
			PlayerFinishedLevel(1);		//1 means go to secret level
			return 1;
		}

		if (Triggers[trigger_num].flags & TRIGGER_ENERGY_DRAIN) {
			Players[Player_num].energy -= Triggers[trigger_num].value;
			mprintf((0,"SLURP!\n"));
		}
	}

	if (Triggers[trigger_num].flags & TRIGGER_CONTROL_DOORS) {
		mprintf((0,"D"));
		do_link(trigger_num);
	}

	if (Triggers[trigger_num].flags & TRIGGER_MATCEN) {
		if (!(Game_mode & GM_MULTI) || (Game_mode & GM_MULTI_ROBOTS))
			do_matcen(trigger_num);
	}

	if (Triggers[trigger_num].flags & TRIGGER_ILLUSION_ON) {
		mprintf((0,"I"));
		do_il_on(trigger_num);
	}

	if (Triggers[trigger_num].flags & TRIGGER_ILLUSION_OFF) {
//		Triggers[trigger_num].time = TRIGGER_DELAY_DOOR;
		mprintf((0,"i"));
		do_il_off(trigger_num);
	}
	return 0;
}

//-----------------------------------------------------------------
// Checks for a trigger whenever an object hits a trigger side.
void check_trigger(segment *seg, short side, short objnum)
{
	int wall_num, trigger_num, ctrigger_num;
	segment *csegp;
 	short cside;

//	mprintf(0,"T");

	if (objnum == Players[Player_num].objnum) {

//		if ( Newdemo_state == ND_STATE_RECORDING )
//			newdemo_record_trigger( seg-Segments, side, objnum );

		if ( Newdemo_state == ND_STATE_PLAYBACK )
			return;

		wall_num = seg->sides[side].wall_num;
		if ( wall_num == -1 ) return;
		
		trigger_num = Walls[wall_num].trigger;

		if (trigger_num == -1)
			return;

		if (check_trigger_sub(trigger_num, Player_num))
			return;

		if (Triggers[trigger_num].flags & TRIGGER_ONE_SHOT) {
			Triggers[trigger_num].flags &= ~TRIGGER_ON;
	
			csegp = &Segments[seg->children[side]];
			cside = find_connect_side(seg, csegp);
			Assert(cside != -1);
		
			wall_num = csegp->sides[cside].wall_num;
			if ( wall_num == -1 ) return;
			
			ctrigger_num = Walls[wall_num].trigger;
	
			Triggers[ctrigger_num].flags &= ~TRIGGER_ON;
		}
#ifndef SHAREWARE
#ifdef NETWORK
		if (Game_mode & GM_MULTI)
			multi_send_trigger(trigger_num);
#endif
#endif
	}
}
  
void triggers_frame_process()
{
	int i;

	for (i=0;i<Num_triggers;i++)
		if (Triggers[i].time >= 0)
			Triggers[i].time -= FrameTime;
}

#ifndef FAST_FILE_IO
/*
 * reads a v29_trigger structure from a CFILE
 */
extern void trigger_read(trigger *t, CFILE *fp)
{
	int i;
	
	t->type = cfile_read_byte(fp);
	t->flags = cfile_read_short(fp);
	t->value = cfile_read_fix(fp);
	t->time = cfile_read_fix(fp);
	t->link_num = cfile_read_byte(fp);
	t->num_links = cfile_read_short(fp);
	for (i=0; i<MAX_WALLS_PER_LINK; i++ )
		t->seg[i] = cfile_read_short(fp);
	for (i=0; i<MAX_WALLS_PER_LINK; i++ )
		t->side[i] = cfile_read_short(fp);
}
#endif
