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
 * New Triggers and Switches.
 *
 */

#include <stdexcept>
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
#include "dxxerror.h"
#include "gameseg.h"
#include "wall.h"
#include "texmap.h"
#include "object.h"
#include "fuelcen.h"
#include "cntrlcen.h"
#include "newdemo.h"
#include "player.h"
#include "endlevel.h"
#include "gameseq.h"
#include "multi.h"
#include "palette.h"
#include "robot.h"
#include "bm.h"
#include "byteutil.h"

#ifdef EDITOR
#include "editor/editor.h"
#endif

#include "physfs-serial.h"
#include "compiler-range_for.h"
#include "partial_range.h"

unsigned Num_triggers;
array<trigger, MAX_TRIGGERS> Triggers;

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
		Triggers[i].flags = 0;
#if defined(DXX_BUILD_DESCENT_I)
		Triggers[i].link_num = -1;
#elif defined(DXX_BUILD_DESCENT_II)
		Triggers[i].num_links = 0;
#endif
		Triggers[i].value = 0;
		Triggers[i].time = -1;
		}
}
#endif

//-----------------------------------------------------------------
// Executes a link, attached to a trigger.
// Toggles all walls linked to the switch.
// Opens doors, Blasts blast walls, turns off illusions.
static void do_link(sbyte trigger_num)
{
	int i;

	if (trigger_num != -1) {
		for (i=0;i<Triggers[trigger_num].num_links;i++) {
			wall_toggle(Triggers[trigger_num].seg[i], Triggers[trigger_num].side[i]);
  		}
  	}
}

#if defined(DXX_BUILD_DESCENT_II)
//close a door
static void do_close_door(sbyte trigger_num)
{
	int i;

	if (trigger_num != -1) {
		for (i=0;i<Triggers[trigger_num].num_links;i++)
			wall_close_door(&Segments[Triggers[trigger_num].seg[i]], Triggers[trigger_num].side[i]);
  	}
}

//turns lighting on.  returns true if lights were actually turned on. (they
//would not be if they had previously been shot out).
static int do_light_on(sbyte trigger_num)
{
	int i,ret=0;

	if (trigger_num != -1) {
		for (i=0;i<Triggers[trigger_num].num_links;i++) {
			int sidenum;
			auto segnum = Triggers[trigger_num].seg[i];
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
static int do_light_off(sbyte trigger_num)
{
	int i,ret=0;

	if (trigger_num != -1) {
		for (i=0;i<Triggers[trigger_num].num_links;i++) {
			int sidenum;
			auto segnum = Triggers[trigger_num].seg[i];
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
static void do_unlock_doors(sbyte trigger_num)
{
	int i;

	if (trigger_num != -1) {
		for (i=0;i<Triggers[trigger_num].num_links;i++) {
			Walls[Segments[Triggers[trigger_num].seg[i]].sides[Triggers[trigger_num].side[i]].wall_num].flags &= ~WALL_DOOR_LOCKED;
			Walls[Segments[Triggers[trigger_num].seg[i]].sides[Triggers[trigger_num].side[i]].wall_num].keys = KEY_NONE;
  		}
  	}
}

// Locks all doors linked to the switch.
static void do_lock_doors(sbyte trigger_num)
{
	int i;

	if (trigger_num != -1) {
		for (i=0;i<Triggers[trigger_num].num_links;i++) {
			Walls[Segments[Triggers[trigger_num].seg[i]].sides[Triggers[trigger_num].side[i]].wall_num].flags |= WALL_DOOR_LOCKED;
  		}
  	}
}

// Changes walls pointed to by a trigger. returns true if any walls changed
static int do_change_walls(sbyte trigger_num)
{
	int i,ret=0;

	if (trigger_num != -1) {
		for (i=0;i<Triggers[trigger_num].num_links;i++) {
			segptridx_t csegp = segment_none;
			short side,cside;
			int new_wall_type;

			auto segp = vsegptridx(Triggers[trigger_num].seg[i]);
			side = Triggers[trigger_num].side[i];

			if (segp->children[side] < 0)
			{
				cside = -1;
			}
			else
			{
				csegp = segptridx(segp->children[side]);
				cside = find_connect_side(segp, csegp);
				Assert(cside != -1);
			}

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

			if (Walls[segp->sides[side].wall_num].type == new_wall_type &&
			    (cside < 0 || csegp->sides[cside].wall_num == wall_none ||
			     Walls[csegp->sides[cside].wall_num].type == new_wall_type))
				continue;		//already in correct state, so skip

			ret = 1;

			switch (Triggers[trigger_num].type) {

				case TT_OPEN_WALL:
					if ((TmapInfo[segp->sides[side].tmap_num].flags & TMI_FORCE_FIELD)) {
						const auto pos = compute_center_point_on_side(segp, side );
						digi_link_sound_to_pos( SOUND_FORCEFIELD_OFF, segp, side, pos, 0, F1_0 );
						Walls[segp->sides[side].wall_num].type = new_wall_type;
						digi_kill_sound_linked_to_segment(segp,side,SOUND_FORCEFIELD_HUM);
						if (cside > -1 && csegp->sides[cside].wall_num != wall_none)
						{
							Walls[csegp->sides[cside].wall_num].type = new_wall_type;
							digi_kill_sound_linked_to_segment(csegp, cside, SOUND_FORCEFIELD_HUM);
						}
					}
					else
						start_wall_cloak(segp,side);

					ret = 1;

					break;

				case TT_CLOSE_WALL:
					if ((TmapInfo[segp->sides[side].tmap_num].flags & TMI_FORCE_FIELD)) {
						const auto pos = compute_center_point_on_side(segp, side );
						digi_link_sound_to_pos(SOUND_FORCEFIELD_HUM,segp,side,pos,1, F1_0/2);
						Walls[segp->sides[side].wall_num].type = new_wall_type;
						if (cside > -1 && csegp->sides[cside].wall_num != wall_none)
							Walls[csegp->sides[cside].wall_num].type = new_wall_type;
					}
					else
						start_wall_decloak(segp,side);
					break;

				case TT_ILLUSORY_WALL:
					Walls[segp->sides[side].wall_num].type = new_wall_type;
					if (cside > -1 && csegp->sides[cside].wall_num != wall_none)
						Walls[csegp->sides[cside].wall_num].type = new_wall_type;
					break;
			}


			kill_stuck_objects(segp->sides[side].wall_num);
			if (cside > -1 && csegp->sides[cside].wall_num != wall_none)
				kill_stuck_objects(csegp->sides[cside].wall_num);

  		}
  	}

	return ret;
}

#define print_trigger_message(pnum,trig,shot,message)	\
	((void)((__print_trigger_message(pnum,trig,shot)) &&		\
		(HUD_init_message(HM_DEFAULT, message, &"s"[((Triggers[trig].num_links>1)?0:1)]))))

static int __print_trigger_message(int pnum,int trig,int shot)
 {
   if (pnum!=Player_num)
		return 0;
    if (!(Triggers[trig].flags & TF_NO_MESSAGE) && shot)
		return 1;
	return 0;
 }
#endif

static void do_matcen(sbyte trigger_num)
{
	int i;

	if (trigger_num != -1) {
		for (i=0;i<Triggers[trigger_num].num_links;i++) {
			trigger_matcen(Triggers[trigger_num].seg[i] );
  		}
  	}
}


static void do_il_on(sbyte trigger_num)
{
	int i;

	if (trigger_num != -1) {
		for (i=0;i<Triggers[trigger_num].num_links;i++) {
			wall_illusion_on(&Segments[Triggers[trigger_num].seg[i]], Triggers[trigger_num].side[i]);
  		}
  	}
}

static void do_il_off(sbyte trigger_num)
{
	int i;

	if (trigger_num != -1) {
		for (i=0;i<Triggers[trigger_num].num_links;i++) {
			segment		*seg = &Segments[Triggers[trigger_num].seg[i]];
			int			side = Triggers[trigger_num].side[i];

			wall_illusion_off(seg, side);

#if defined(DXX_BUILD_DESCENT_II)
			const auto cp = compute_center_point_on_side(seg, side );
			digi_link_sound_to_pos( SOUND_WALL_REMOVED, seg-Segments, side, cp, 0, F1_0 );
#endif
  		}
  	}
}

#if defined(DXX_BUILD_DESCENT_II)
static int wall_is_forcefield(trigger *trig)
{
	int i;

	for (i=0;i<trig->num_links;i++)
		if ((TmapInfo[Segments[trig->seg[i]].sides[trig->side[i]].tmap_num].flags & TMI_FORCE_FIELD))
			break;

	return (i<trig->num_links);
}
#endif

int check_trigger_sub(int trigger_num, int pnum,int shot)
{
	if (pnum < 0 || pnum > MAX_PLAYERS)
		return 1;
	if ((Game_mode & GM_MULTI) && (Players[pnum].connected != CONNECT_PLAYING)) // as a host we may want to handle triggers for our clients. to do that properly we must check wether we (host) or client is actually playing.
		return 1;

#if defined(DXX_BUILD_DESCENT_I)
	(void)shot;
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

			if (Game_mode & GM_MULTI)
				multi_send_endlevel_start(1);
			if (Game_mode & GM_NETWORK)
				multi_do_protocol_frame(1, 1);
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
#elif defined(DXX_BUILD_DESCENT_II)
	trigger *trig = &Triggers[trigger_num];

	if (trig->flags & TF_DISABLED)
		return 1;		//1 means don't send trigger hit to other players

	if (trig->flags & TF_ONE_SHOT)		//if this is a one-shot...
		trig->flags |= TF_DISABLED;		//..then don't let it happen again

	switch (trig->type) {

		case TT_EXIT:

			if (pnum!=Player_num)
			  break;

                        if (!EMULATING_D1)
			  digi_stop_digi_sounds();  //Sound shouldn't cut out when exiting a D1 lvl

			if (Current_level_num > 0) {
				start_endlevel_sequence();
			} else if (Current_level_num < 0) {
				if ((Players[Player_num].shields < 0) || Player_is_dead)
					break;
				// NMN 04/09/07 Do endlevel movie if we are
				//             playing a D1 secret level
				if (EMULATING_D1)
				{
					start_endlevel_sequence();
				} else {
					ExitSecretLevel();
				}
				return 1;
			} else {
				#ifdef EDITOR
					nm_messagebox_str( "Yo!", "You have hit the exit trigger!", "" );
				#else
					Int3();		//level num == 0, but no editor!
				#endif
			}
			return 1;
			break;

		case TT_SECRET_EXIT: {
			int	truth;

			if (pnum!=Player_num)
				break;

			if ((Players[Player_num].shields < 0) || Player_is_dead)
				break;

			if (is_SHAREWARE || is_MAC_SHARE) {
				HUD_init_message_literal(HM_DEFAULT, "Secret Level Teleporter disabled in Descent 2 Demo");
				digi_play_sample( SOUND_BAD_SELECTION, F1_0 );
				break;
			}

			if (Game_mode & GM_MULTI) {
				HUD_init_message_literal(HM_DEFAULT, "Secret Level Teleporter disabled in multiplayer!");
				digi_play_sample( SOUND_BAD_SELECTION, F1_0 );
				break;
			}

			truth = p_secret_level_destroyed();

			if (Newdemo_state == ND_STATE_RECORDING)			// record whether we're really going to the secret level
				newdemo_record_secret_exit_blown(truth);

			if ((Newdemo_state != ND_STATE_PLAYBACK) && truth) {
				HUD_init_message_literal(HM_DEFAULT, "Secret Level destroyed.  Exit disabled.");
				digi_play_sample( SOUND_BAD_SELECTION, F1_0 );
				break;
			}

			if (Newdemo_state == ND_STATE_RECORDING)		// stop demo recording
				Newdemo_state = ND_STATE_PAUSED;

			digi_stop_digi_sounds();

			EnterSecretLevel();
			Control_center_destroyed = 0;
			return 1;
			break;

		}

		case TT_OPEN_DOOR:
			do_link(trigger_num);
			print_trigger_message (pnum,trigger_num,shot,"Door%s opened!");

			break;

		case TT_CLOSE_DOOR:
			do_close_door(trigger_num);
			print_trigger_message (pnum,trigger_num,shot,"Door%s closed!");
			break;

		case TT_UNLOCK_DOOR:
			do_unlock_doors(trigger_num);
			print_trigger_message (pnum,trigger_num,shot,"Door%s unlocked!");

			break;

		case TT_LOCK_DOOR:
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
			do_il_on(trigger_num);
			print_trigger_message (pnum,trigger_num,shot,"Illusion%s on!");
			break;

		case TT_ILLUSION_OFF:
			do_il_off(trigger_num);
			print_trigger_message (pnum,trigger_num,shot,"Illusion%s off!");
			break;

		case TT_LIGHT_OFF:
			if (do_light_off(trigger_num))
				print_trigger_message (pnum,trigger_num,shot,"Light%s off!");
			break;

		case TT_LIGHT_ON:
			if (do_light_on(trigger_num))
				print_trigger_message (pnum,trigger_num,shot,"Light%s on!");

			break;

		default:
			Int3();
			break;
	}
#endif

	return 0;
}

//-----------------------------------------------------------------
// Checks for a trigger whenever an object hits a trigger side.
void check_trigger(const vsegptridx_t seg, short side, objnum_t objnum,int shot)
{
	int trigger_num;	//, ctrigger_num;

	if ((Game_mode & GM_MULTI) && (Players[Player_num].connected != CONNECT_PLAYING)) // as a host we may want to handle triggers for our clients. so this function may be called when we are not playing.
		return;

#if defined(DXX_BUILD_DESCENT_I)
	if (objnum == Players[Player_num].objnum)
#elif defined(DXX_BUILD_DESCENT_II)
	if ((objnum == Players[Player_num].objnum) || ((Objects[objnum].type == OBJ_ROBOT) && (Robot_info[get_robot_id(&Objects[objnum])].companion)))
#endif
	{

#if defined(DXX_BUILD_DESCENT_I)
		if ( Newdemo_state == ND_STATE_PLAYBACK )
			return;
#elif defined(DXX_BUILD_DESCENT_II)
		if ( Newdemo_state == ND_STATE_RECORDING )
			newdemo_record_trigger( seg, side, objnum,shot);
#endif

		auto wall_num = seg->sides[side].wall_num;
		if ( wall_num == wall_none ) return;

		trigger_num = Walls[wall_num].trigger;

		if (trigger_num == -1)
			return;

		if (check_trigger_sub(trigger_num, Player_num,shot))
			return;

#if defined(DXX_BUILD_DESCENT_I)
		if (Triggers[trigger_num].flags & TRIGGER_ONE_SHOT) {
			int ctrigger_num;
			Triggers[trigger_num].flags &= ~TRIGGER_ON;
	
			auto csegp = &Segments[seg->children[side]];
			auto cside = find_connect_side(seg, csegp);
			Assert(cside != -1);
		
			wall_num = csegp->sides[cside].wall_num;
			if ( wall_num == wall_none ) return;
			
			ctrigger_num = Walls[wall_num].trigger;
	
			Triggers[ctrigger_num].flags &= ~TRIGGER_ON;
		}
#endif
		if (Game_mode & GM_MULTI)
			multi_send_trigger(trigger_num);
	}
}

void triggers_frame_process()
{
	range_for (auto &i, partial_range(Triggers, Num_triggers))
		if (i.time >= 0)
			i.time -= FrameTime;
}

/*
 * reads a v29_trigger structure from a PHYSFS_file
 */
#if defined(DXX_BUILD_DESCENT_I)
void v26_trigger_read(PHYSFS_File *fp, trigger &t)
{
	int type;
	switch ((type = PHYSFSX_readByte(fp)))
	{
		case TT_OPEN_DOOR: // door
			t.flags = TRIGGER_CONTROL_DOORS;
			break;
		case TT_MATCEN: // matcen
			t.flags = TRIGGER_MATCEN;
			break;
		case TT_EXIT: // exit
			t.flags = TRIGGER_EXIT;
			break;
		case TT_SECRET_EXIT: // secret exit
			t.flags = TRIGGER_SECRET_EXIT;
			break;
		case TT_ILLUSION_OFF: // illusion off
			t.flags = TRIGGER_ILLUSION_OFF;
			break;
		case TT_ILLUSION_ON: // illusion on
			t.flags = TRIGGER_ILLUSION_ON;
			break;
		default:
			con_printf(CON_URGENT,"Warning: unsupported trigger type %d", type);
			throw std::runtime_error("unsupported trigger type");
	}
	t.flags = (PHYSFSX_readByte(fp) & 2) ? // one shot
		TRIGGER_ONE_SHOT :
		0;
	t.num_links = PHYSFSX_readShort(fp);
	t.value = PHYSFSX_readInt(fp);
	t.time = PHYSFSX_readInt(fp);
	for (unsigned i=0; i < MAX_WALLS_PER_LINK; i++ )
		t.seg[i] = PHYSFSX_readShort(fp);
	for (unsigned i=0; i < MAX_WALLS_PER_LINK; i++ )
		t.side[i] = PHYSFSX_readShort(fp);
}

void v25_trigger_read(PHYSFS_file *fp, trigger *t)
#elif defined(DXX_BUILD_DESCENT_II)
extern void v29_trigger_read(v29_trigger *t, PHYSFS_file *fp)
#endif
{
	int i;

#if defined(DXX_BUILD_DESCENT_I)
	PHYSFSX_readByte(fp);
#elif defined(DXX_BUILD_DESCENT_II)
	t->type = PHYSFSX_readByte(fp);
#endif
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

#if defined(DXX_BUILD_DESCENT_II)
/*
 * reads a v30_trigger structure from a PHYSFS_file
 */
extern void v30_trigger_read(v30_trigger *t, PHYSFS_file *fp)
{
	int i;

	t->flags = PHYSFSX_readShort(fp);
	t->num_links = PHYSFSX_readByte(fp);
	t->pad = PHYSFSX_readByte(fp);
	t->value = PHYSFSX_readFix(fp);
	t->time = PHYSFSX_readFix(fp);
	for (i=0; i<MAX_WALLS_PER_LINK; i++ )
		t->seg[i] = PHYSFSX_readShort(fp);
	for (i=0; i<MAX_WALLS_PER_LINK; i++ )
		t->side[i] = PHYSFSX_readShort(fp);
}

/*
 * reads a trigger structure from a PHYSFS_file
 */
extern void trigger_read(trigger *t, PHYSFS_file *fp)
{
	int i;

	t->type = PHYSFSX_readByte(fp);
	t->flags = PHYSFSX_readByte(fp);
	t->num_links = PHYSFSX_readByte(fp);
	PHYSFSX_readByte(fp);
	t->value = PHYSFSX_readFix(fp);
	t->time = PHYSFSX_readFix(fp);
	for (i=0; i<MAX_WALLS_PER_LINK; i++ )
		t->seg[i] = PHYSFSX_readShort(fp);
	for (i=0; i<MAX_WALLS_PER_LINK; i++ )
		t->side[i] = PHYSFSX_readShort(fp);
}

static ubyte trigger_type_from_flags(short flags)
{
	if (flags & TRIGGER_CONTROL_DOORS)
		return TT_OPEN_DOOR;
	else if (flags & TRIGGER_SHIELD_DAMAGE)
		throw std::runtime_error("unsupported trigger type");
	else if (flags & TRIGGER_ENERGY_DRAIN)
		throw std::runtime_error("unsupported trigger type");
	else if (flags & TRIGGER_EXIT)
		return TT_EXIT;
	else if (flags & TRIGGER_MATCEN)
		return TT_MATCEN;
	else if (flags & TRIGGER_ILLUSION_OFF)
		return TT_ILLUSION_OFF;
	else if (flags & TRIGGER_SECRET_EXIT)
		return TT_SECRET_EXIT;
	else if (flags & TRIGGER_ILLUSION_ON)
		return TT_ILLUSION_ON;
	else if (flags & TRIGGER_UNLOCK_DOORS)
		return TT_UNLOCK_DOOR;
	else if (flags & TRIGGER_OPEN_WALL)
		return TT_OPEN_WALL;
	else if (flags & TRIGGER_CLOSE_WALL)
		return TT_CLOSE_WALL;
	else if (flags & TRIGGER_ILLUSORY_WALL)
		return TT_ILLUSORY_WALL;
	else
		throw std::runtime_error("unsupported trigger type");
}

static void v30_trigger_to_v31_trigger(trigger &t, const v30_trigger &trig)
{
	t.type        = trigger_type_from_flags(trig.flags & ~TRIGGER_ON);
	t.flags       = (trig.flags & TRIGGER_ONE_SHOT) ? TF_ONE_SHOT : 0;
	t.num_links   = trig.num_links;
	t.num_links   = trig.num_links;
	t.value       = trig.value;
	t.time        = trig.time;
	t.seg = trig.seg;
	t.side = trig.side;
}

static void v29_trigger_read_as_v30(PHYSFS_File *fp, v30_trigger &trig)
{
	v29_trigger trig29;
	v29_trigger_read(&trig29, fp);
	trig.flags	= trig29.flags;
	// skip trig29.link_num. v30_trigger does not need it
	trig.num_links	= trig29.num_links;
	trig.value	= trig29.value;
	trig.time	= trig29.time;
	trig.seg = trig29.seg;
	trig.side = trig29.side;
}

void v29_trigger_read_as_v31(PHYSFS_File *fp, trigger &t)
{
	v30_trigger trig;
	v29_trigger_read_as_v30(fp, trig);
	v30_trigger_to_v31_trigger(t, trig);
}

void v30_trigger_read_as_v31(PHYSFS_File *fp, trigger &t)
{
	v30_trigger trig;
	v30_trigger_read(&trig, fp);
	v30_trigger_to_v31_trigger(t, trig);
}
#endif

#if defined(DXX_BUILD_DESCENT_I)
DEFINE_SERIAL_UDT_TO_MESSAGE(trigger, t, (serial::pad<1>(), t.flags, t.value, t.time, t.link_num, t.num_links, t.seg, t.side));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(trigger, 54);
#elif defined(DXX_BUILD_DESCENT_II)
DEFINE_SERIAL_UDT_TO_MESSAGE(trigger, t, (t.type, t.flags, t.num_links, serial::pad<1>(), t.value, t.time, t.seg, t.side));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(trigger, 52);
#endif

/*
 * reads n trigger structs from a PHYSFS_file and swaps if specified
 */
void trigger_read(PHYSFS_file *fp, trigger &t)
{
	PHYSFSX_serialize_read(fp, t);
}

void trigger_write(PHYSFS_file *fp, const trigger &t)
{
	PHYSFSX_serialize_write(fp, t);
}

void v29_trigger_write(PHYSFS_file *fp, const trigger &rt)
{
	const trigger *t = &rt;
	PHYSFSX_writeU8(fp, 0);		// unused 'type'
#if defined(DXX_BUILD_DESCENT_I)
	PHYSFS_writeSLE16(fp, t->flags);
#elif defined(DXX_BUILD_DESCENT_II)
	switch (t->type)
	{
		case TT_OPEN_DOOR:
			PHYSFS_writeSLE16(fp, TRIGGER_CONTROL_DOORS | ((t->flags & TF_ONE_SHOT) ? TRIGGER_ONE_SHOT : 0));
			break;

		case TT_EXIT:
			PHYSFS_writeSLE16(fp, TRIGGER_EXIT | ((t->flags & TF_ONE_SHOT) ? TRIGGER_ONE_SHOT : 0));
			break;

		case TT_MATCEN:
			PHYSFS_writeSLE16(fp, TRIGGER_MATCEN | ((t->flags & TF_ONE_SHOT) ? TRIGGER_ONE_SHOT : 0));
			break;

		case TT_ILLUSION_OFF:
			PHYSFS_writeSLE16(fp, TRIGGER_ILLUSION_OFF | ((t->flags & TF_ONE_SHOT) ? TRIGGER_ONE_SHOT : 0));
			break;

		case TT_SECRET_EXIT:
			PHYSFS_writeSLE16(fp, TRIGGER_SECRET_EXIT | ((t->flags & TF_ONE_SHOT) ? TRIGGER_ONE_SHOT : 0));
			break;

		case TT_ILLUSION_ON:
			PHYSFS_writeSLE16(fp, TRIGGER_ILLUSION_ON | ((t->flags & TF_ONE_SHOT) ? TRIGGER_ONE_SHOT : 0));
			break;

		case TT_UNLOCK_DOOR:
			PHYSFS_writeSLE16(fp, TRIGGER_UNLOCK_DOORS | ((t->flags & TF_ONE_SHOT) ? TRIGGER_ONE_SHOT : 0));
			break;

		case TT_OPEN_WALL:
			PHYSFS_writeSLE16(fp, TRIGGER_OPEN_WALL | ((t->flags & TF_ONE_SHOT) ? TRIGGER_ONE_SHOT : 0));
			break;

		case TT_CLOSE_WALL:
			PHYSFS_writeSLE16(fp, TRIGGER_CLOSE_WALL | ((t->flags & TF_ONE_SHOT) ? TRIGGER_ONE_SHOT : 0));
			break;

		case TT_ILLUSORY_WALL:
			PHYSFS_writeSLE16(fp, TRIGGER_ILLUSORY_WALL | ((t->flags & TF_ONE_SHOT) ? TRIGGER_ONE_SHOT : 0));
			break;

		default:
			Int3();
			PHYSFS_writeSLE16(fp, 0);
			break;
	}
#endif

	PHYSFSX_writeFix(fp, t->value);
	PHYSFSX_writeFix(fp, t->time);

#if defined(DXX_BUILD_DESCENT_I)
	PHYSFSX_writeU8(fp, t->link_num);
#elif defined(DXX_BUILD_DESCENT_II)
	PHYSFSX_writeU8(fp, -1);	//t->link_num
#endif
	PHYSFS_writeSLE16(fp, t->num_links);

	for (unsigned i = 0; i < MAX_WALLS_PER_LINK; i++)
		PHYSFS_writeSLE16(fp, t->seg[i]);
	for (unsigned i = 0; i < MAX_WALLS_PER_LINK; i++)
		PHYSFS_writeSLE16(fp, t->side[i]);
}

void v30_trigger_write(PHYSFS_file *fp, const trigger &rt)
{
	const trigger *t = &rt;
#if defined(DXX_BUILD_DESCENT_I)
	if (t->flags & TRIGGER_CONTROL_DOORS)
		PHYSFSX_writeU8(fp, TT_OPEN_DOOR); // door
	else if (t->flags & TRIGGER_MATCEN)
		PHYSFSX_writeU8(fp, TT_MATCEN); // matcen
	else if (t->flags & TRIGGER_EXIT)
		PHYSFSX_writeU8(fp, TT_EXIT); // exit
	else if (t->flags & TRIGGER_SECRET_EXIT)
		PHYSFSX_writeU8(fp, TT_SECRET_EXIT); // secret exit
	else if (t->flags & TRIGGER_ILLUSION_OFF)
		PHYSFSX_writeU8(fp, TT_ILLUSION_OFF); // illusion off
	else if (t->flags & TRIGGER_ILLUSION_ON)
		PHYSFSX_writeU8(fp, TT_ILLUSION_ON); // illusion on
#elif defined(DXX_BUILD_DESCENT_II)
	PHYSFSX_writeU8(fp, t->type);
#endif

#if defined(DXX_BUILD_DESCENT_I)
	PHYSFS_writeSLE16(fp, t->flags);
#elif defined(DXX_BUILD_DESCENT_II)
	switch (t->type)
	{
		case TT_OPEN_DOOR:
			PHYSFS_writeSLE16(fp, TRIGGER_CONTROL_DOORS | ((t->flags & TF_ONE_SHOT) ? TRIGGER_ONE_SHOT : 0));
			break;

		case TT_EXIT:
			PHYSFS_writeSLE16(fp, TRIGGER_EXIT | ((t->flags & TF_ONE_SHOT) ? TRIGGER_ONE_SHOT : 0));
			break;

		case TT_MATCEN:
			PHYSFS_writeSLE16(fp, TRIGGER_MATCEN | ((t->flags & TF_ONE_SHOT) ? TRIGGER_ONE_SHOT : 0));
			break;

		case TT_ILLUSION_OFF:
			PHYSFS_writeSLE16(fp, TRIGGER_ILLUSION_OFF | ((t->flags & TF_ONE_SHOT) ? TRIGGER_ONE_SHOT : 0));
			break;

		case TT_SECRET_EXIT:
			PHYSFS_writeSLE16(fp, TRIGGER_SECRET_EXIT | ((t->flags & TF_ONE_SHOT) ? TRIGGER_ONE_SHOT : 0));
			break;

		case TT_ILLUSION_ON:
			PHYSFS_writeSLE16(fp, TRIGGER_ILLUSION_ON | ((t->flags & TF_ONE_SHOT) ? TRIGGER_ONE_SHOT : 0));
			break;

		case TT_UNLOCK_DOOR:
			PHYSFS_writeSLE16(fp, TRIGGER_UNLOCK_DOORS | ((t->flags & TF_ONE_SHOT) ? TRIGGER_ONE_SHOT : 0));
			break;

		case TT_OPEN_WALL:
			PHYSFS_writeSLE16(fp, TRIGGER_OPEN_WALL | ((t->flags & TF_ONE_SHOT) ? TRIGGER_ONE_SHOT : 0));
			break;

		case TT_CLOSE_WALL:
			PHYSFS_writeSLE16(fp, TRIGGER_CLOSE_WALL | ((t->flags & TF_ONE_SHOT) ? TRIGGER_ONE_SHOT : 0));
			break;

		case TT_ILLUSORY_WALL:
			PHYSFS_writeSLE16(fp, TRIGGER_ILLUSORY_WALL | ((t->flags & TF_ONE_SHOT) ? TRIGGER_ONE_SHOT : 0));
			break;

		default:
			Int3();
			PHYSFS_writeSLE16(fp, 0);
			break;
	}
#endif

	PHYSFSX_writeU8(fp, t->num_links);
	PHYSFSX_writeU8(fp, 0);	// t->pad

	PHYSFSX_writeFix(fp, t->value);
	PHYSFSX_writeFix(fp, t->time);

	for (unsigned i = 0; i < MAX_WALLS_PER_LINK; i++)
		PHYSFS_writeSLE16(fp, t->seg[i]);
	for (unsigned i = 0; i < MAX_WALLS_PER_LINK; i++)
		PHYSFS_writeSLE16(fp, t->side[i]);
}

void v31_trigger_write(PHYSFS_file *fp, const trigger &rt)
{
	const trigger *t = &rt;
#if defined(DXX_BUILD_DESCENT_I)
	if (t->flags & TRIGGER_CONTROL_DOORS)
		PHYSFSX_writeU8(fp, TT_OPEN_DOOR); // door
	else if (t->flags & TRIGGER_MATCEN)
		PHYSFSX_writeU8(fp, TT_MATCEN); // matcen
	else if (t->flags & TRIGGER_EXIT)
		PHYSFSX_writeU8(fp, TT_EXIT); // exit
	else if (t->flags & TRIGGER_SECRET_EXIT)
		PHYSFSX_writeU8(fp, TT_SECRET_EXIT); // secret exit
	else if (t->flags & TRIGGER_ILLUSION_OFF)
		PHYSFSX_writeU8(fp, TT_ILLUSION_OFF); // illusion off
	else if (t->flags & TRIGGER_ILLUSION_ON)
		PHYSFSX_writeU8(fp, TT_ILLUSION_ON); // illusion on
#elif defined(DXX_BUILD_DESCENT_II)
	PHYSFSX_writeU8(fp, t->type);
#endif

#if defined(DXX_BUILD_DESCENT_I)
	PHYSFSX_writeU8(fp, (t->flags & TRIGGER_ONE_SHOT) ? 2 : 0);		// flags
#elif defined(DXX_BUILD_DESCENT_II)
	PHYSFSX_writeU8(fp, t->flags);
#endif

	PHYSFSX_writeU8(fp, t->num_links);
	PHYSFSX_writeU8(fp, 0);	// t->pad

	PHYSFSX_writeFix(fp, t->value);
	PHYSFSX_writeFix(fp, t->time);

	for (unsigned i = 0; i < MAX_WALLS_PER_LINK; i++)
		PHYSFS_writeSLE16(fp, t->seg[i]);
	for (unsigned i = 0; i < MAX_WALLS_PER_LINK; i++)
		PHYSFS_writeSLE16(fp, t->side[i]);
}
