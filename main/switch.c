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
#include "segment.h"
#include "error.h"
#include "gameseg.h"
#include "wall.h"
#include "texmap.h"
#include "fuelcen.h"
#include "cntrlcen.h"
#include "newdemo.h"
#include "player.h"
#include "endlevel.h"
#include "gameseq.h"
#include "multi.h"
#include "palette.h"
#include "byteswap.h"

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
			wall_toggle(Triggers[trigger_num].seg[i], Triggers[trigger_num].side[i]); 
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

			#ifdef NETWORK
			if (Game_mode & GM_MULTI)
				multi_send_endlevel_start(1);
			#endif
#ifdef NETWORK
			if (Game_mode & GM_NETWORK)
				multi_do_protocol_frame(1, 1);
#endif
			PlayerFinishedLevel(1);		//1 means go to secret level
			Control_center_destroyed = 0;
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
 * reads a v29_trigger structure from a PHYSFS_file
 */
extern void trigger_read(trigger *t, PHYSFS_file *fp)
{
	int i;
	
	t->type = PHYSFSX_readByte(fp);
	t->flags = PHYSFSX_readShort(fp);
	t->value = PHYSFSX_readFix(fp);
	t->time = PHYSFSX_readFix(fp);
	t->link_num = PHYSFSX_readByte(fp);
	t->num_links = PHYSFSX_readShort(fp);
	for (i=0; i<MAX_WALLS_PER_LINK; i++ )
		t->seg[i] = PHYSFSX_readShort(fp);
	for (i=0; i<MAX_WALLS_PER_LINK; i++ )
		t->side[i] = PHYSFSX_readShort(fp);
}

void trigger_swap(trigger *t, int swap)
{
	int i;
	
	if (!swap)
		return;
	
	t->flags = SWAPSHORT(t->flags);
	t->value = SWAPINT(t->value);
	t->time = SWAPINT(t->time);
	t->num_links = SWAPSHORT(t->num_links);
	for (i=0; i<MAX_WALLS_PER_LINK; i++ )
		t->seg[i] = SWAPSHORT(t->seg[i]);
	for (i=0; i<MAX_WALLS_PER_LINK; i++ )
		t->side[i] = SWAPSHORT(t->side[i]);
}

/*
 * reads n trigger structs from a PHYSFS_file and swaps if specified
 */
void trigger_read_n_swap(trigger *t, int n, int swap, PHYSFS_file *fp)
{
	int i;
	
	PHYSFS_read(fp, t, sizeof(trigger), n);
	
	if (swap)
		for (i = 0; i < n; i++)
			trigger_swap(&t[i], swap);
}

void trigger_write(trigger *t, short version, PHYSFS_file *fp)
{
	int i;

	if (version <= 29)
		PHYSFSX_writeU8(fp, 0);	// unused 'type'
	else if (version >= 31)
	{
		if (t->flags & TRIGGER_CONTROL_DOORS)
			PHYSFSX_writeU8(fp, 0); // door
		else if (t->flags & TRIGGER_MATCEN)
			PHYSFSX_writeU8(fp, 2); // matcen
		else if (t->flags & TRIGGER_EXIT)
			PHYSFSX_writeU8(fp, 3); // exit
		else if (t->flags & TRIGGER_SECRET_EXIT)
			PHYSFSX_writeU8(fp, 4); // secret exit
		else if (t->flags & TRIGGER_ILLUSION_OFF)
			PHYSFSX_writeU8(fp, 5); // illusion off
		else if (t->flags & TRIGGER_ILLUSION_ON)
			PHYSFSX_writeU8(fp, 6); // illusion on
	}
	
	if (version <= 30)
		PHYSFS_writeSLE16(fp, t->flags);
	else
		PHYSFSX_writeU8(fp, (t->flags & TRIGGER_ONE_SHOT) ? 2 : 0);		// flags
	
	if (version >= 30)
	{
		PHYSFSX_writeU8(fp, t->num_links);
		PHYSFSX_writeU8(fp, 0);	// t->pad
	}
	
	PHYSFSX_writeFix(fp, t->value);
	PHYSFSX_writeFix(fp, t->time);
	
	if (version <= 29)
	{
		PHYSFSX_writeU8(fp, t->link_num);
		PHYSFS_writeSLE16(fp, t->num_links);
	}
	
	for (i = 0; i < MAX_WALLS_PER_LINK; i++)
		PHYSFS_writeSLE16(fp, t->seg[i]);
	for (i = 0; i < MAX_WALLS_PER_LINK; i++)
		PHYSFS_writeSLE16(fp, t->side[i]);
}
