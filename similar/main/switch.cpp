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
#include "hudmsg.h"
#include "robot.h"
#include "bm.h"

#if DXX_USE_EDITOR
#include "editor/editor.h"
#endif

#include "physfs-serial.h"
#include "compiler-range_for.h"
#include "partial_range.h"

#if DXX_USE_EDITOR
//-----------------------------------------------------------------
// Initializes all the switches.
void trigger_init()
{
	Triggers.set_count(0);
}
#endif

template <typename T1, typename T2>
static inline void trigger_wall_op(const trigger &t, T1 &segment_factory, const T2 &op)
{
	for (unsigned i = 0, num_links = t.num_links; i != num_links; ++i)
		op(segment_factory(t.seg[i]), t.side[i]);
}

//-----------------------------------------------------------------
// Executes a link, attached to a trigger.
// Toggles all walls linked to the switch.
// Opens doors, Blasts blast walls, turns off illusions.
static void do_link(const trigger &t)
{
	trigger_wall_op(t, vmsegptridx, wall_toggle);
}

#if defined(DXX_BUILD_DESCENT_II)
namespace dsx {
//close a door
static void do_close_door(const trigger &t)
{
	trigger_wall_op(t, vmsegptridx, wall_close_door);
}

//turns lighting on.  returns true if lights were actually turned on. (they
//would not be if they had previously been shot out).
static int do_light_on(const trigger &t)
{
	int ret=0;
	const auto op = [&ret](const vmsegptridx_t segnum, const unsigned sidenum) {
			//check if tmap2 casts light before turning the light on.  This
			//is to keep us from turning on blown-out lights
			if (TmapInfo[segnum->sides[sidenum].tmap_num2 & 0x3fff].lighting) {
				ret |= add_light(segnum, sidenum); 		//any light sets flag
				enable_flicker(Flickering_light_state, segnum, sidenum);
			}
	};
	trigger_wall_op(t, vmsegptridx, op);
	return ret;
}

//turns lighting off.  returns true if lights were actually turned off. (they
//would not be if they had previously been shot out).
static int do_light_off(const trigger &t)
{
	int ret=0;
	const auto op = [&ret](const vmsegptridx_t segnum, const unsigned sidenum) {
			//check if tmap2 casts light before turning the light off.  This
			//is to keep us from turning off blown-out lights
			if (TmapInfo[segnum->sides[sidenum].tmap_num2 & 0x3fff].lighting) {
				ret |= subtract_light(segnum, sidenum); 	//any light sets flag
				disable_flicker(Flickering_light_state, segnum, sidenum);
			}
	};
	trigger_wall_op(t, vmsegptridx, op);
	return ret;
}

// Unlocks all doors linked to the switch.
static void do_unlock_doors(const trigger &t)
{
	const auto op = [](const vmsegptr_t segp, const unsigned sidenum) {
		const auto wall_num = segp->sides[sidenum].wall_num;
		auto &w = *vmwallptr(wall_num);
		w.flags &= ~WALL_DOOR_LOCKED;
		w.keys = KEY_NONE;
	};
	trigger_wall_op(t, vmsegptr, op);
}

// Locks all doors linked to the switch.
static void do_lock_doors(const trigger &t)
{
	const auto op = [](const vmsegptr_t segp, const unsigned sidenum) {
		const auto wall_num = segp->sides[sidenum].wall_num;
		auto &w = *vmwallptr(wall_num);
		w.flags |= WALL_DOOR_LOCKED;
	};
	trigger_wall_op(t, vmsegptr, op);
}

// Changes walls pointed to by a trigger. returns true if any walls changed
static int do_change_walls(const trigger &t, const uint8_t new_wall_type)
{
	int ret=0;

	for (unsigned i = 0; i < t.num_links; ++i)
	{
		uint8_t cside;
		const auto &&segp = vmsegptridx(t.seg[i]);
		const auto side = t.side[i];
			imsegptridx_t csegp = segment_none;

			if (!IS_CHILD(segp->children[side]))
			{
				cside = side_none;
			}
			else
			{
				csegp = imsegptridx(segp->children[side]);
				cside = find_connect_side(segp, csegp);
				Assert(cside != side_none);
			}

			wall *w0p;
				const auto w0num = segp->sides[side].wall_num;
			if (const auto &&uw0p = vmwallptr.check_untrusted(w0num))
				w0p = *uw0p;
			else
			{
				LevelError("trigger %p link %u tried to open segment %hu, side %u which is an invalid wall; ignoring.", addressof(t), i, static_cast<segnum_t>(segp), side);
				continue;
			}
			auto &wall0 = *w0p;
			imwallptr_t wall1 = nullptr;
			if ((cside == side_none || csegp->sides[cside].wall_num == wall_none ||
				(wall1 = vmwallptr(csegp->sides[cside].wall_num))->type == new_wall_type) &&
				wall0.type == new_wall_type)
				continue;		//already in correct state, so skip

			ret |= 1;

			switch (t.type)
			{
				case TT_OPEN_WALL:
					if ((TmapInfo[segp->sides[side].tmap_num].flags & TMI_FORCE_FIELD)) {
						ret |= 2;
						const auto &&pos = compute_center_point_on_side(vcvertptr, segp, side);
						digi_link_sound_to_pos( SOUND_FORCEFIELD_OFF, segp, side, pos, 0, F1_0 );
						digi_kill_sound_linked_to_segment(segp,side,SOUND_FORCEFIELD_HUM);
						wall0.type = new_wall_type;
						if (wall1)
						{
							wall1->type = new_wall_type;
							digi_kill_sound_linked_to_segment(csegp, cside, SOUND_FORCEFIELD_HUM);
						}
					}
					else
						start_wall_cloak(segp,side);
					break;

				case TT_CLOSE_WALL:
					if ((TmapInfo[segp->sides[side].tmap_num].flags & TMI_FORCE_FIELD)) {
						ret |= 2;
						{
						const auto &&pos = compute_center_point_on_side(vcvertptr, segp, side);
						digi_link_sound_to_pos(SOUND_FORCEFIELD_HUM,segp,side,pos,1, F1_0/2);
						}
					case TT_ILLUSORY_WALL:
						wall0.type = new_wall_type;
						if (wall1)
							wall1->type = new_wall_type;
					}
					else
						start_wall_decloak(segp,side);
					break;
				default:
					return 0;
			}

			LevelUniqueStuckObjectState.kill_stuck_objects(vmobjptr, segp->sides[side].wall_num);
			if (wall1)
				LevelUniqueStuckObjectState.kill_stuck_objects(vmobjptr, csegp->sides[cside].wall_num);
  	}
	flush_fcd_cache();

	return ret;
}

#define print_trigger_message(pnum,trig,shot,message)	\
	((void)((print_trigger_message(pnum,trig,shot)) &&		\
		HUD_init_message(HM_DEFAULT, message, &"s"[trig.num_links <= 1])))

static int (print_trigger_message)(int pnum, const trigger &t, int shot)
 {
	if (shot && pnum == Player_num && !(t.flags & TF_NO_MESSAGE))
		return 1;
	return 0;
 }
}
#endif

static void do_matcen(const trigger &t)
{
	range_for (auto &i, partial_const_range(t.seg, t.num_links))
		trigger_matcen(vmsegptridx(i));
}

static void do_il_on(const trigger &t)
{
	trigger_wall_op(t, vmsegptridx, wall_illusion_on);
}

namespace dsx {
static void do_il_off(const trigger &t)
{
#if defined(DXX_BUILD_DESCENT_I)
	auto &op = wall_illusion_off;
#elif defined(DXX_BUILD_DESCENT_II)
	const auto op = [](const vmsegptridx_t &seg, unsigned side) {
		wall_illusion_off(seg, side);
		const auto &&cp = compute_center_point_on_side(vcvertptr, seg, side);
		digi_link_sound_to_pos(SOUND_WALL_REMOVED, seg, side, cp, 0, F1_0);
	};
#endif
	trigger_wall_op(t, vmsegptridx, op);
}

// Slight variation on window_event_result meaning
// 'ignored' means we still want check_trigger to call multi_send_trigger
// 'handled' or 'close' means we don't
// 'close' will still close the game window
window_event_result check_trigger_sub(object &plrobj, const trgnum_t trigger_num, const playernum_t pnum, const unsigned shot)
{
	auto result = window_event_result::ignored;

	if ((Game_mode & GM_MULTI) && vcplayerptr(pnum)->connected != CONNECT_PLAYING) // as a host we may want to handle triggers for our clients. to do that properly we must check wether we (host) or client is actually playing.
		return window_event_result::handled;
	auto &trigger = *vmtrgptr(trigger_num);

#if defined(DXX_BUILD_DESCENT_I)
	(void)shot;
	if (pnum == Player_num) {
		auto &player_info = plrobj.ctype.player_info;
		if (trigger.flags & TRIGGER_SHIELD_DAMAGE) {
			plrobj.shields -= trigger.value;
		}

		if (trigger.flags & TRIGGER_EXIT)
		{
			result = start_endlevel_sequence();
			if (result == window_event_result::handled)
				result = window_event_result::ignored;	// call multi_send_trigger, or end game anyway
		}

		if (trigger.flags & TRIGGER_SECRET_EXIT) {
			if (trigger.flags & TRIGGER_EXIT)
				LevelError("Trigger %u is both a regular and secret exit! This is not a recommended combination.", trigger_num);
			if (Newdemo_state == ND_STATE_RECORDING)		// stop demo recording
				Newdemo_state = ND_STATE_PAUSED;

			if (Game_mode & GM_MULTI)
				multi_send_endlevel_start(multi_endlevel_type::secret);
			if (Game_mode & GM_NETWORK)
				multi_do_protocol_frame(1, 1);
			result = std::max(PlayerFinishedLevel(1), result);		//1 means go to secret level
			Control_center_destroyed = 0;
			return std::max(result, window_event_result::handled);
		}

		if (trigger.flags & TRIGGER_ENERGY_DRAIN) {
			player_info.energy -= trigger.value;
		}
	}

	if (trigger.flags & TRIGGER_CONTROL_DOORS) {
		do_link(trigger);
	}

	if (trigger.flags & TRIGGER_MATCEN) {
		if (!(Game_mode & GM_MULTI) || (Game_mode & GM_MULTI_ROBOTS))
			do_matcen(trigger);
	}

	if (trigger.flags & TRIGGER_ILLUSION_ON) {
		do_il_on(trigger);
	}

	if (trigger.flags & TRIGGER_ILLUSION_OFF) {
		do_il_off(trigger);
	}
#elif defined(DXX_BUILD_DESCENT_II)
	if (trigger.flags & TF_DISABLED)
		return window_event_result::handled;		// don't send trigger hit to other players

	if (trigger.flags & TF_ONE_SHOT)		//if this is a one-shot...
		trigger.flags |= TF_DISABLED;		//..then don't let it happen again

	switch (trigger.type)
	{
		case TT_EXIT:
			if (pnum!=Player_num)
				break;

			if (!EMULATING_D1)
				digi_stop_digi_sounds();  //Sound shouldn't cut out when exiting a D1 lvl

			if (Current_level_num > 0) {
				result = start_endlevel_sequence();
			} else if (Current_level_num < 0) {
				if (plrobj.shields < 0 ||
					Player_dead_state != player_dead_state::no)
					break;
				// NMN 04/09/07 Do endlevel movie if we are
				//             playing a D1 secret level
				if (EMULATING_D1)
				{
					result = start_endlevel_sequence();
				} else {
					result = ExitSecretLevel();
				}
				return std::max(result, window_event_result::handled);
			} else {
#if DXX_USE_EDITOR
					nm_messagebox_str( "Yo!", "You have hit the exit trigger!", "" );
				#else
					Int3();		//level num == 0, but no editor!
				#endif
			}
			return std::max(result, window_event_result::handled);
			break;

		case TT_SECRET_EXIT: {
			int	truth;

			if (pnum!=Player_num)
				break;

			if (plrobj.shields < 0 ||
				Player_dead_state != player_dead_state::no)
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
			return window_event_result::handled;
			break;

		}

		case TT_OPEN_DOOR:
			do_link(trigger);
			print_trigger_message(pnum, trigger, shot, "Door%s opened!");

			break;

		case TT_CLOSE_DOOR:
			do_close_door(trigger);
			print_trigger_message(pnum, trigger, shot, "Door%s closed!");
			break;

		case TT_UNLOCK_DOOR:
			do_unlock_doors(trigger);
			print_trigger_message(pnum, trigger, shot, "Door%s unlocked!");

			break;

		case TT_LOCK_DOOR:
			do_lock_doors(trigger);
			print_trigger_message(pnum, trigger, shot, "Door%s locked!");

			break;

		case TT_OPEN_WALL:
			if (const auto w = do_change_walls(trigger, WALL_OPEN))
				print_trigger_message(pnum, trigger, shot, (w & 2) ? "Force field%s deactivated!" : "Wall%s opened!");
			break;

		case TT_CLOSE_WALL:
			if (const auto w = do_change_walls(trigger, WALL_CLOSED))
				print_trigger_message(pnum, trigger, shot, (w & 2) ? "Force field%s activated!" : "Wall%s closed!");
			break;

		case TT_ILLUSORY_WALL:
			//don't know what to say, so say nothing
			do_change_walls(trigger, WALL_ILLUSION);
			break;

		case TT_MATCEN:
			if (!(Game_mode & GM_MULTI) || (Game_mode & GM_MULTI_ROBOTS))
				do_matcen(trigger);
			break;

		case TT_ILLUSION_ON:
			do_il_on(trigger);
			print_trigger_message(pnum, trigger, shot, "Illusion%s on!");
			break;

		case TT_ILLUSION_OFF:
			do_il_off(trigger);
			print_trigger_message(pnum, trigger, shot, "Illusion%s off!");
			break;

		case TT_LIGHT_OFF:
			if (do_light_off(trigger))
				print_trigger_message(pnum, trigger, shot, "Light%s off!");
			break;

		case TT_LIGHT_ON:
			if (do_light_on(trigger))
				print_trigger_message(pnum, trigger, shot, "Light%s on!");

			break;

		default:
			Int3();
			break;
	}
#endif

	return result;
}

//-----------------------------------------------------------------
// Checks for a trigger whenever an object hits a trigger side.
window_event_result check_trigger(const vcsegptridx_t seg, short side, object &plrobj, const vcobjptridx_t objnum, int shot)
{
	if ((Game_mode & GM_MULTI) && (get_local_player().connected != CONNECT_PLAYING)) // as a host we may want to handle triggers for our clients. so this function may be called when we are not playing.
		return window_event_result::ignored;

#if defined(DXX_BUILD_DESCENT_I)
	if (objnum == &plrobj)
#elif defined(DXX_BUILD_DESCENT_II)
	if (objnum == &plrobj || (objnum->type == OBJ_ROBOT && Robot_info[get_robot_id(objnum)].companion))
#endif
	{

#if defined(DXX_BUILD_DESCENT_I)
		if ( Newdemo_state == ND_STATE_PLAYBACK )
			return window_event_result::ignored;
#elif defined(DXX_BUILD_DESCENT_II)
		if ( Newdemo_state == ND_STATE_RECORDING )
			newdemo_record_trigger( seg, side, objnum,shot);
#endif

		const auto wall_num = seg->sides[side].wall_num;
		if ( wall_num == wall_none ) return window_event_result::ignored;

		const auto trigger_num = vmwallptr(wall_num)->trigger;
		if (trigger_num == trigger_none)
			return window_event_result::ignored;

		{
			auto result = check_trigger_sub(plrobj, trigger_num, Player_num,shot);
			if (result != window_event_result::ignored)
				return result;
		}

#if defined(DXX_BUILD_DESCENT_I)
		auto &t = *vmtrgptr(trigger_num);
		if (t.flags & TRIGGER_ONE_SHOT)
		{
			t.flags &= ~TRIGGER_ON;
	
			auto &csegp = *vcsegptr(seg->children[side]);
			auto cside = find_connect_side(seg, csegp);
			Assert(cside != side_none);
		
			const auto cwall_num = csegp.sides[cside].wall_num;
			if (cwall_num == wall_none)
				return window_event_result::ignored;
			
			const auto ctrigger_num = vmwallptr(cwall_num)->trigger;
			auto &ct = *vmtrgptr(ctrigger_num);
			ct.flags &= ~TRIGGER_ON;
		}
#endif
		if (Game_mode & GM_MULTI)
			multi_send_trigger(trigger_num);
	}

	return window_event_result::handled;
}

/*
 * reads a v29_trigger structure from a PHYSFS_File
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
	if (PHYSFSX_readByte(fp) & 2)	// one shot
		t.flags |= TRIGGER_ONE_SHOT;
	t.num_links = PHYSFSX_readShort(fp);
	t.value = PHYSFSX_readInt(fp);
	PHYSFSX_readInt(fp);
	for (unsigned i=0; i < MAX_WALLS_PER_LINK; i++ )
		t.seg[i] = PHYSFSX_readShort(fp);
	for (unsigned i=0; i < MAX_WALLS_PER_LINK; i++ )
		t.side[i] = PHYSFSX_readShort(fp);
}

void v25_trigger_read(PHYSFS_File *fp, trigger *t)
#elif defined(DXX_BUILD_DESCENT_II)
extern void v29_trigger_read(v29_trigger *t, PHYSFS_File *fp)
#endif
{
#if defined(DXX_BUILD_DESCENT_I)
	PHYSFSX_readByte(fp);
#elif defined(DXX_BUILD_DESCENT_II)
	t->type = PHYSFSX_readByte(fp);
#endif
	t->flags = PHYSFSX_readShort(fp);
	t->value = PHYSFSX_readFix(fp);
	PHYSFSX_readFix(fp);
	t->link_num = PHYSFSX_readByte(fp);
	t->num_links = PHYSFSX_readShort(fp);
	for (unsigned i=0; i<MAX_WALLS_PER_LINK; i++ )
		t->seg[i] = PHYSFSX_readShort(fp);
	for (unsigned i=0; i<MAX_WALLS_PER_LINK; i++ )
		t->side[i] = PHYSFSX_readShort(fp);
}

#if defined(DXX_BUILD_DESCENT_II)
/*
 * reads a v30_trigger structure from a PHYSFS_File
 */
extern void v30_trigger_read(v30_trigger *t, PHYSFS_File *fp)
{
	t->flags = PHYSFSX_readShort(fp);
	t->num_links = PHYSFSX_readByte(fp);
	t->pad = PHYSFSX_readByte(fp);
	t->value = PHYSFSX_readFix(fp);
	t->time = PHYSFSX_readFix(fp);
	for (unsigned i=0; i<MAX_WALLS_PER_LINK; i++ )
		t->seg[i] = PHYSFSX_readShort(fp);
	for (unsigned i=0; i<MAX_WALLS_PER_LINK; i++ )
		t->side[i] = PHYSFSX_readShort(fp);
}

/*
 * reads a trigger structure from a PHYSFS_File
 */
extern void trigger_read(trigger *t, PHYSFS_File *fp)
{
	t->type = PHYSFSX_readByte(fp);
	t->flags = PHYSFSX_readByte(fp);
	t->num_links = PHYSFSX_readByte(fp);
	PHYSFSX_readByte(fp);
	t->value = PHYSFSX_readFix(fp);
	PHYSFSX_readFix(fp);
	for (unsigned i=0; i<MAX_WALLS_PER_LINK; i++ )
		t->seg[i] = PHYSFSX_readShort(fp);
	for (unsigned i=0; i<MAX_WALLS_PER_LINK; i++ )
		t->side[i] = PHYSFSX_readShort(fp);
}

static ubyte trigger_type_from_flags(short flags)
{
	if (flags & TRIGGER_CONTROL_DOORS)
		return TT_OPEN_DOOR;
	else if (flags & (TRIGGER_SHIELD_DAMAGE | TRIGGER_ENERGY_DRAIN))
	{
	}
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
	throw std::runtime_error("unsupported trigger type");
}

static void v30_trigger_to_v31_trigger(trigger &t, const v30_trigger &trig)
{
	t.type        = trigger_type_from_flags(trig.flags & ~TRIGGER_ON);
	t.flags       = (trig.flags & TRIGGER_ONE_SHOT) ? TF_ONE_SHOT : 0;
	t.num_links   = trig.num_links;
	t.num_links   = trig.num_links;
	t.value       = trig.value;
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
DEFINE_SERIAL_UDT_TO_MESSAGE(trigger, t, (serial::pad<1>(), t.flags, t.value, serial::pad<4>(), t.link_num, t.num_links, t.seg, t.side));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(trigger, 54);
#elif defined(DXX_BUILD_DESCENT_II)
DEFINE_SERIAL_UDT_TO_MESSAGE(trigger, t, (t.type, t.flags, t.num_links, serial::pad<1>(), t.value, serial::pad<4>(), t.seg, t.side));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(trigger, 52);
#endif
}

/*
 * reads n trigger structs from a PHYSFS_File and swaps if specified
 */
void trigger_read(PHYSFS_File *fp, trigger &t)
{
	PHYSFSX_serialize_read(fp, t);
}

void trigger_write(PHYSFS_File *fp, const trigger &t)
{
	PHYSFSX_serialize_write(fp, t);
}

namespace dsx {
void v29_trigger_write(PHYSFS_File *fp, const trigger &rt)
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
	PHYSFSX_writeFix(fp, 0);

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
}

namespace dsx {
void v30_trigger_write(PHYSFS_File *fp, const trigger &rt)
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
	PHYSFSX_writeFix(fp, 0);

	for (unsigned i = 0; i < MAX_WALLS_PER_LINK; i++)
		PHYSFS_writeSLE16(fp, t->seg[i]);
	for (unsigned i = 0; i < MAX_WALLS_PER_LINK; i++)
		PHYSFS_writeSLE16(fp, t->side[i]);
}
}

namespace dsx {
void v31_trigger_write(PHYSFS_File *fp, const trigger &rt)
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
	PHYSFSX_writeFix(fp, 0);

	for (unsigned i = 0; i < MAX_WALLS_PER_LINK; i++)
		PHYSFS_writeSLE16(fp, t->seg[i]);
	for (unsigned i = 0; i < MAX_WALLS_PER_LINK; i++)
		PHYSFS_writeSLE16(fp, t->side[i]);
}
}
