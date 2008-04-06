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
 * New Triggers and Switches.
 *
 */


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

	if (trigger_num != -1) {
		for (i=0;i<Triggers[trigger_num].num_links;i++) {
			wall_toggle(&Segments[Triggers[trigger_num].seg[i]], Triggers[trigger_num].side[i]); 
  		}
  	}
}

void do_matcen(sbyte trigger_num)
{
	int i;

	if (trigger_num != -1) {
		for (i=0;i<Triggers[trigger_num].num_links;i++) {
			trigger_matcen(Triggers[trigger_num].seg[i] ); 
  		}
  	}
}

	
void do_il_on(sbyte trigger_num)
{
	int i;

	if (trigger_num != -1) {
		for (i=0;i<Triggers[trigger_num].num_links;i++) {
			wall_illusion_on(&Segments[Triggers[trigger_num].seg[i]], Triggers[trigger_num].side[i]); 
  		}
  	}
}

void do_il_off(sbyte trigger_num)
{
	int i;
	
	if (trigger_num != -1) {
		for (i=0;i<Triggers[trigger_num].num_links;i++) {
			wall_illusion_off(&Segments[Triggers[trigger_num].seg[i]], Triggers[trigger_num].side[i]); 
  		}
  	}
}

int check_trigger_sub(int trigger_num, int pnum)
{

	if (pnum == Player_num) {
		if (Triggers[trigger_num].flags & TRIGGER_SHIELD_DAMAGE) {
			Players[Player_num].shields -= Triggers[trigger_num].value;
		}

		if (Triggers[trigger_num].flags & TRIGGER_EXIT) {
			start_endlevel_sequence();
		}

		if (Triggers[trigger_num].flags & TRIGGER_SECRET_EXIT) {
			if (Newdemo_state == ND_STATE_RECORDING)		// stop demo recording
				Newdemo_state = ND_STATE_PAUSED;

			Fuelcen_control_center_destroyed = 0;
			#ifdef NETWORK
			if (Game_mode & GM_MULTI)
				multi_send_endlevel_start(1);
			#endif
#ifdef NETWORK
			if (Game_mode & GM_NETWORK)
				network_do_frame(1, 1);
#endif
			PlayerFinishedLevel(1);		//1 means go to secret level
			return 1;
		}

		if (Triggers[trigger_num].flags & TRIGGER_ENERGY_DRAIN) {
			Players[Player_num].energy -= Triggers[trigger_num].value;
		}
	}

	if (Triggers[trigger_num].flags & TRIGGER_CONTROL_DOORS) {
		do_link(trigger_num);
	}

	if (Triggers[trigger_num].flags & TRIGGER_MATCEN) {
		if (!(Game_mode & GM_MULTI) || (Game_mode & GM_MULTI_ROBOTS))
			do_matcen(trigger_num);
	}

	if (Triggers[trigger_num].flags & TRIGGER_ILLUSION_ON) {
		do_il_on(trigger_num);
	}

	if (Triggers[trigger_num].flags & TRIGGER_ILLUSION_OFF) {
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
