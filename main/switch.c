/* $Id: switch.c,v 1.9 2003-10-04 03:14:48 btb Exp $ */
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
 * New Triggers and Switches.
 *
 * Old Log:
 * Revision 1.2  1995/10/31  10:18:10  allender
 * shareware stuff
 *
 * Revision 1.1  1995/05/16  15:31:21  allender
 * Initial revision
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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: switch.c,v 1.9 2003-10-04 03:14:48 btb Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "gauges.h"
#include "newmenu.h"
#include "game.h"
#include "switch.h"
#include "inferno.h"
#include "segment.h"
#include "error.h"
#include "gameseg.h"
#include "mono.h"
#include "wall.h"
#include "texmap.h"
#include "fuelcen.h"
#include "cntrlcen.h"
#include "newdemo.h"
#include "player.h"
#include "endlevel.h"
#include "gameseq.h"
#include "multi.h"
#ifdef NETWORK
#include "network.h"
#endif
#include "palette.h"
#include "robot.h"
#include "bm.h"

#ifdef EDITOR
#include "editor/editor.h"
#endif

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

	for (i=0;i<MAX_TRIGGERS;i++)
		{
		Triggers[i].type = 0;
		Triggers[i].flags = 0;
		Triggers[i].num_links = 0;
		Triggers[i].value = 0;
		Triggers[i].time = -1;
		}
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

//close a door
void do_close_door(sbyte trigger_num)
{
	int i;

	mprintf((0, "Door close!\n"));

	if (trigger_num != -1) {
		for (i=0;i<Triggers[trigger_num].num_links;i++)
			wall_close_door(&Segments[Triggers[trigger_num].seg[i]], Triggers[trigger_num].side[i]);
  	}
}

//turns lighting on.  returns true if lights were actually turned on. (they
//would not be if they had previously been shot out).
int do_light_on(sbyte trigger_num)
{
	int i,ret=0;

	mprintf((0, "Lighting on!\n"));

	if (trigger_num != -1) {
		for (i=0;i<Triggers[trigger_num].num_links;i++) {
			int segnum,sidenum;
			segnum = Triggers[trigger_num].seg[i];
			sidenum = Triggers[trigger_num].side[i];

			//check if tmap2 casts light before turning the light on.  This
			//is to keep us from turning on blown-out lights
			if (TmapInfo[Segments[segnum].sides[sidenum].tmap_num2 & 0x3fff].lighting) {
				ret |= add_light(segnum, sidenum); 		//any light sets flag
				enable_flicker(segnum, sidenum);
			}
		}
	}

	return ret;
}

//turns lighting off.  returns true if lights were actually turned off. (they
//would not be if they had previously been shot out).
int do_light_off(sbyte trigger_num)
{
	int i,ret=0;

	mprintf((0, "Lighting off!\n"));

	if (trigger_num != -1) {
		for (i=0;i<Triggers[trigger_num].num_links;i++) {
			int segnum,sidenum;
			segnum = Triggers[trigger_num].seg[i];
			sidenum = Triggers[trigger_num].side[i];

			//check if tmap2 casts light before turning the light off.  This
			//is to keep us from turning off blown-out lights
			if (TmapInfo[Segments[segnum].sides[sidenum].tmap_num2 & 0x3fff].lighting) {
				ret |= subtract_light(segnum, sidenum); 	//any light sets flag
				disable_flicker(segnum, sidenum);
			}
  		}
  	}

	return ret;
}

// Unlocks all doors linked to the switch.
void do_unlock_doors(sbyte trigger_num)
{
	int i;

	mprintf((0, "Door unlock!\n"));

	if (trigger_num != -1) {
		for (i=0;i<Triggers[trigger_num].num_links;i++) {
			Walls[Segments[Triggers[trigger_num].seg[i]].sides[Triggers[trigger_num].side[i]].wall_num].flags &= ~WALL_DOOR_LOCKED;
			Walls[Segments[Triggers[trigger_num].seg[i]].sides[Triggers[trigger_num].side[i]].wall_num].keys = KEY_NONE;
  		}
  	}
}

// Return trigger number if door is controlled by a wall switch, else return -1.
int door_is_wall_switched(int wall_num)
{
	int i, t;

	for (t=0; t<Num_triggers; t++) {
		for (i=0; i<Triggers[t].num_links; i++) {
			if (Segments[Triggers[t].seg[i]].sides[Triggers[t].side[i]].wall_num == wall_num) {
				mprintf((0, "Wall #%i is keyed to trigger #%i, link #%i\n", wall_num, t, i));
				return t;
			}
	  	}
	}

	return -1;
}

void flag_wall_switched_doors(void)
{
	int	i;

	for (i=0; i<Num_walls; i++) {
		if (door_is_wall_switched(i))
			Walls[i].flags |= WALL_WALL_SWITCH;
	}

}

// Locks all doors linked to the switch.
void do_lock_doors(sbyte trigger_num)
{
	int i;

	mprintf((0, "Door lock!\n"));

	if (trigger_num != -1) {
		for (i=0;i<Triggers[trigger_num].num_links;i++) {
			Walls[Segments[Triggers[trigger_num].seg[i]].sides[Triggers[trigger_num].side[i]].wall_num].flags |= WALL_DOOR_LOCKED;
  		}
  	}
}

// Changes walls pointed to by a trigger. returns true if any walls changed
int do_change_walls(sbyte trigger_num)
{
	int i,ret=0;

	mprintf((0, "Wall remove!\n"));

	if (trigger_num != -1) {
		for (i=0;i<Triggers[trigger_num].num_links;i++) {
			segment *segp,*csegp;
			short side,cside;
			int new_wall_type;

			segp = &Segments[Triggers[trigger_num].seg[i]];
			side = Triggers[trigger_num].side[i];

			csegp = &Segments[segp->children[side]];
			cside = find_connect_side(segp, csegp);
			Assert(cside != -1);

			//segp->sides[side].wall_num = -1;
			//csegp->sides[cside].wall_num = -1;

			switch (Triggers[trigger_num].type) {
				case TT_OPEN_WALL:		new_wall_type = WALL_OPEN; break;
				case TT_CLOSE_WALL:		new_wall_type = WALL_CLOSED; break;
				case TT_ILLUSORY_WALL:	new_wall_type = WALL_ILLUSION; break;
			        default:
					Assert(0); /* new_wall_type unset */
					return(0);
					break;
			}

			if (Walls[segp->sides[side].wall_num].type == new_wall_type && Walls[csegp->sides[cside].wall_num].type == new_wall_type)
				continue;		//already in correct state, so skip

			ret = 1;

			switch (Triggers[trigger_num].type) {
	
				case TT_OPEN_WALL:
					mprintf((0,"Open wall\n"));

					if ((TmapInfo[segp->sides[side].tmap_num].flags & TMI_FORCE_FIELD)) {
						vms_vector pos;
						compute_center_point_on_side(&pos, segp, side );
						digi_link_sound_to_pos( SOUND_FORCEFIELD_OFF, segp-Segments, side, &pos, 0, F1_0 );
						Walls[segp->sides[side].wall_num].type = new_wall_type;
						Walls[csegp->sides[cside].wall_num].type = new_wall_type;
						digi_kill_sound_linked_to_segment(segp-Segments,side,SOUND_FORCEFIELD_HUM);
						digi_kill_sound_linked_to_segment(csegp-Segments,cside,SOUND_FORCEFIELD_HUM);
					}
					else
						start_wall_cloak(segp,side);

					ret = 1;

					break;

				case TT_CLOSE_WALL:
					mprintf((0,"Close wall\n"));

					if ((TmapInfo[segp->sides[side].tmap_num].flags & TMI_FORCE_FIELD)) {
						vms_vector pos;
						compute_center_point_on_side(&pos, segp, side );
						digi_link_sound_to_pos(SOUND_FORCEFIELD_HUM,segp-Segments,side,&pos,1, F1_0/2);
						Walls[segp->sides[side].wall_num].type = new_wall_type;
						Walls[csegp->sides[cside].wall_num].type = new_wall_type;
					}
					else
						start_wall_decloak(segp,side);
					break;

				case TT_ILLUSORY_WALL:
					mprintf((0,"Illusory wall\n"));
					Walls[segp->sides[side].wall_num].type = new_wall_type;
					Walls[csegp->sides[cside].wall_num].type = new_wall_type;
					break;
			}


			kill_stuck_objects(segp->sides[side].wall_num);
			kill_stuck_objects(csegp->sides[cside].wall_num);

  		}
  	}

	return ret;
}

void print_trigger_message (int pnum,int trig,int shot,char *message)
 {
	char *pl;		//points to 's' or nothing for plural word

   if (pnum!=Player_num)
		return;

	pl = (Triggers[trig].num_links>1)?"s":"";

    if (!(Triggers[trig].flags & TF_NO_MESSAGE) && shot)
     HUD_init_message (message,pl);
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
			vms_vector	cp;
			segment		*seg = &Segments[Triggers[trigger_num].seg[i]];
			int			side = Triggers[trigger_num].side[i];

			wall_illusion_off(seg, side);

			mprintf((0," trigger_num %d : seg %d, side %d\n",
				trigger_num, Triggers[trigger_num].seg[i], Triggers[trigger_num].side[i]));

			compute_center_point_on_side(&cp, seg, side );
			digi_link_sound_to_pos( SOUND_WALL_REMOVED, seg-Segments, side, &cp, 0, F1_0 );

  		}
  	}
}

extern void EnterSecretLevel(void);
extern void ExitSecretLevel(void);
extern int p_secret_level_destroyed(void);

int wall_is_forcefield(trigger *trig)
{
	int i;

	for (i=0;i<trig->num_links;i++)
		if ((TmapInfo[Segments[trig->seg[i]].sides[trig->side[i]].tmap_num].flags & TMI_FORCE_FIELD))
			break;

	return (i<trig->num_links);
}

int check_trigger_sub(int trigger_num, int pnum,int shot)
{
	trigger *trig = &Triggers[trigger_num];

	mprintf ((0,"trignum=%d type=%d shot=%d\n",trigger_num,trig->type,shot));

	if (trig->flags & TF_DISABLED)
		return 1;		//1 means don't send trigger hit to other players

	if (trig->flags & TF_ONE_SHOT)		//if this is a one-shot...
		trig->flags |= TF_DISABLED;		//..then don't let it happen again

	switch (trig->type) {

		case TT_EXIT:

			if (pnum!=Player_num)
			  break;

			digi_stop_all();		//kill the sounds
			
			if (Current_level_num > 0) {
				start_endlevel_sequence();
				mprintf((0,"WOOHOO! (leaving the mine!)\n"));
			} else if (Current_level_num < 0) {
				if ((Players[Player_num].shields < 0) || Player_is_dead)
					break;

				ExitSecretLevel();
				return 1;
			} else {
				#ifdef EDITOR
					nm_messagebox( "Yo!", 1, "You have hit the exit trigger!", "" );
				#else
					Int3();		//level num == 0, but no editor!
				#endif
			}
			return 1;
			break;

		case TT_SECRET_EXIT: {
#ifndef SHAREWARE
			int	truth;
#endif

			if (pnum!=Player_num)
				break;

			if ((Players[Player_num].shields < 0) || Player_is_dead)
				break;

			if (Game_mode & GM_MULTI) {
				HUD_init_message("Secret Level Teleporter disabled in multiplayer!");
				digi_play_sample( SOUND_BAD_SELECTION, F1_0 );
				break;
			}

			#ifndef SHAREWARE
			truth = p_secret_level_destroyed();

			if (Newdemo_state == ND_STATE_RECORDING)			// record whether we're really going to the secret level
				newdemo_record_secret_exit_blown(truth);

			if ((Newdemo_state != ND_STATE_PLAYBACK) && truth) {
				HUD_init_message("Secret Level destroyed.  Exit disabled.");
				digi_play_sample( SOUND_BAD_SELECTION, F1_0 );
				break;
			}
			#endif

			#ifdef SHAREWARE
				HUD_init_message("Secret Level Teleporter disabled in Descent 2 Demo");
				digi_play_sample( SOUND_BAD_SELECTION, F1_0 );
				break;
			#endif

			if (Newdemo_state == ND_STATE_RECORDING)		// stop demo recording
				Newdemo_state = ND_STATE_PAUSED;

			digi_stop_all();		//kill the sounds

			digi_play_sample( SOUND_SECRET_EXIT, F1_0 );
			mprintf((0,"Exiting to secret level\n"));

			// -- BOGUS -- IMPOSSIBLE -- if (Game_mode & GM_MULTI)
			// -- BOGUS -- IMPOSSIBLE -- 	multi_send_endlevel_start(1);
			// -- BOGUS -- IMPOSSIBLE --
			// -- BOGUS -- IMPOSSIBLE -- if (Game_mode & GM_NETWORK)
			// -- BOGUS -- IMPOSSIBLE -- 	network_do_frame(1, 1);

			gr_palette_fade_out(gr_palette, 32, 0);
			EnterSecretLevel();
			Control_center_destroyed = 0;
			return 1;
			break;

		}

		case TT_OPEN_DOOR:
			mprintf((0,"D"));
			do_link(trigger_num);
			print_trigger_message (pnum,trigger_num,shot,"Door%s opened!");
			
			break;

		case TT_CLOSE_DOOR:
			do_close_door(trigger_num);
			print_trigger_message (pnum,trigger_num,shot,"Door%s closed!");
			break;

		case TT_UNLOCK_DOOR:
			mprintf((0,"D"));
			do_unlock_doors(trigger_num);
			print_trigger_message (pnum,trigger_num,shot,"Door%s unlocked!");
			
			break;
	
		case TT_LOCK_DOOR:
			mprintf((0,"D"));
			do_lock_doors(trigger_num);
			print_trigger_message (pnum,trigger_num,shot,"Door%s locked!");

			break;
	
		case TT_OPEN_WALL:
			if (do_change_walls(trigger_num))
			{
				if (wall_is_forcefield(trig))
					print_trigger_message (pnum,trigger_num,shot,"Force field%s deactivated!");
				else
					print_trigger_message (pnum,trigger_num,shot,"Wall%s opened!");
			}
			break;

		case TT_CLOSE_WALL:
			if (do_change_walls(trigger_num))
			{
				if (wall_is_forcefield(trig))
					print_trigger_message (pnum,trigger_num,shot,"Force field%s activated!");
				else
					print_trigger_message (pnum,trigger_num,shot,"Wall%s closed!");
			}
			break;

		case TT_ILLUSORY_WALL:
			//don't know what to say, so say nothing
			do_change_walls(trigger_num);
			break;

		case TT_MATCEN:
			if (!(Game_mode & GM_MULTI) || (Game_mode & GM_MULTI_ROBOTS))
				do_matcen(trigger_num);
			break;
	
		case TT_ILLUSION_ON:
			mprintf((0,"I"));
			do_il_on(trigger_num);
			print_trigger_message (pnum,trigger_num,shot,"Illusion%s on!");
			break;
	
		case TT_ILLUSION_OFF:
			mprintf((0,"i"));
			do_il_off(trigger_num);
			print_trigger_message (pnum,trigger_num,shot,"Illusion%s off!");
			break;

		case TT_LIGHT_OFF:
			if (do_light_off(trigger_num))
				print_trigger_message (pnum,trigger_num,shot,"Lights off!");
			break;

		case TT_LIGHT_ON:
			if (do_light_on(trigger_num))
				print_trigger_message (pnum,trigger_num,shot,"Lights on!");

			break;

		default:
			Int3();
			break;
	}

	return 0;
}

//-----------------------------------------------------------------
// Checks for a trigger whenever an object hits a trigger side.
void check_trigger(segment *seg, short side, short objnum,int shot)
{
	int wall_num, trigger_num;	//, ctrigger_num;
	//segment *csegp;
 	//short cside;

//	mprintf(0,"T");

	if ((objnum == Players[Player_num].objnum) || ((Objects[objnum].type == OBJ_ROBOT) && (Robot_info[Objects[objnum].id].companion))) {

		if ( Newdemo_state == ND_STATE_RECORDING )
			newdemo_record_trigger( seg-Segments, side, objnum,shot);

		wall_num = seg->sides[side].wall_num;
		if ( wall_num == -1 ) return;
		
		trigger_num = Walls[wall_num].trigger;

		if (trigger_num == -1)
			return;

		//##if ( Newdemo_state == ND_STATE_PLAYBACK ) {
		//##	if (Triggers[trigger_num].type == TT_EXIT) {
		//##		start_endlevel_sequence();
		//##	}
		//##	//return;
		//##}

		if (check_trigger_sub(trigger_num, Player_num,shot))
			return;

		//@@if (Triggers[trigger_num].flags & TRIGGER_ONE_SHOT) {
		//@@	Triggers[trigger_num].flags &= ~TRIGGER_ON;
		//@@
		//@@	csegp = &Segments[seg->children[side]];
		//@@	cside = find_connect_side(seg, csegp);
		//@@	Assert(cside != -1);
		//@@
		//@@	wall_num = csegp->sides[cside].wall_num;
		//@@	if ( wall_num == -1 ) return;
		//@@	
		//@@	ctrigger_num = Walls[wall_num].trigger;
		//@@
		//@@	Triggers[ctrigger_num].flags &= ~TRIGGER_ON;
		//@@}

#ifdef NETWORK
		if (Game_mode & GM_MULTI)
			multi_send_trigger(trigger_num);
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
extern void v29_trigger_read(v29_trigger *t, CFILE *fp)
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

/*
 * reads a v30_trigger structure from a CFILE
 */
extern void v30_trigger_read(v30_trigger *t, CFILE *fp)
{
	int i;

	t->flags = cfile_read_short(fp);
	t->num_links = cfile_read_byte(fp);
	t->pad = cfile_read_byte(fp);
	t->value = cfile_read_fix(fp);
	t->time = cfile_read_fix(fp);
	for (i=0; i<MAX_WALLS_PER_LINK; i++ )
		t->seg[i] = cfile_read_short(fp);
	for (i=0; i<MAX_WALLS_PER_LINK; i++ )
		t->side[i] = cfile_read_short(fp);
}

/*
 * reads a trigger structure from a CFILE
 */
extern void trigger_read(trigger *t, CFILE *fp)
{
	int i;

	t->type = cfile_read_byte(fp);
	t->flags = cfile_read_byte(fp);
	t->num_links = cfile_read_byte(fp);
	t->pad = cfile_read_byte(fp);
	t->value = cfile_read_fix(fp);
	t->time = cfile_read_fix(fp);
	for (i=0; i<MAX_WALLS_PER_LINK; i++ )
		t->seg[i] = cfile_read_short(fp);
	for (i=0; i<MAX_WALLS_PER_LINK; i++ )
		t->side[i] = cfile_read_short(fp);
}
#endif
