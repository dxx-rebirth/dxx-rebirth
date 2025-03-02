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

#include "newmenu.h"
#include "game.h"
#include "switch.h"
#include "inferno.h"
#include "segment.h"
#include "dxxerror.h"
#include "gameseg.h"
#include "wall.h"
#include "object.h"
#include "fuelcen.h"
#include "newdemo.h"
#include "player.h"
#include "endlevel.h"
#include "gameseq.h"
#include "net_udp.h"
#include "palette.h"
#include "hudmsg.h"
#include "robot.h"
#include "bm.h"

#include "physfs-serial.h"
#include "d_levelstate.h"
#include "d_underlying_value.h"
#include "d_zip.h"
#include "compiler-range_for.h"
#include "partial_range.h"

namespace {

template <typename... Oa>
static inline void trigger_wall_op(const trigger &t, auto &&segment_factory, auto &&op, Oa &&... oargs)
{
	for (unsigned i = 0, num_links = t.num_links; i != num_links; ++i)
		op(std::forward<Oa>(oargs)..., segment_factory(t.seg[i]), static_cast<sidenum_t>(t.side[i]));
}

//-----------------------------------------------------------------
// Executes a link, attached to a trigger.
// Toggles all walls linked to the switch.
// Opens doors, Blasts blast walls, turns off illusions.
static void do_link(const trigger &t)
{
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vmwallptr = Walls.vmptr;
	trigger_wall_op(t, vmsegptridx, wall_toggle, vmwallptr);
}

}

#if DXX_BUILD_DESCENT == 2
namespace dsx {
namespace {
//close a door
static void do_close_door(const trigger &t)
{
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	trigger_wall_op(t, vmsegptridx, wall_close_door, Walls);
}

//turns lighting on.  returns true if lights were actually turned on. (they
//would not be if they had previously been shot out).
static int do_light_on(const d_level_shared_destructible_light_state &LevelSharedDestructibleLightState, const d_level_unique_tmap_info_state::TmapInfo_array &TmapInfo, d_flickering_light_state &Flickering_light_state, const trigger &t)
{
	int ret{0};
	const auto op = [&LevelSharedDestructibleLightState, &Flickering_light_state, &TmapInfo, &ret](const vmsegptridx_t segnum, const sidenum_t sidenum) {
			//check if tmap2 casts light before turning the light on.  This
			//is to keep us from turning on blown-out lights
		const auto tm2 = get_texture_index(segnum->unique_segment::sides[sidenum].tmap_num2);
		if (TmapInfo[tm2].lighting) {
				ret |= add_light(LevelSharedDestructibleLightState, segnum, sidenum);		//any light sets flag
				enable_flicker(Flickering_light_state, segnum, sidenum);
			}
	};
	trigger_wall_op(t, vmsegptridx, op);
	return ret;
}

//turns lighting off.  returns true if lights were actually turned off. (they
//would not be if they had previously been shot out).
static int do_light_off(const d_level_shared_destructible_light_state &LevelSharedDestructibleLightState, const d_level_unique_tmap_info_state::TmapInfo_array &TmapInfo, d_flickering_light_state &Flickering_light_state, const trigger &t)
{
	int ret{0};
	const auto op = [&LevelSharedDestructibleLightState, &Flickering_light_state, &TmapInfo, &ret](const vmsegptridx_t segnum, const sidenum_t sidenum) {
			//check if tmap2 casts light before turning the light off.  This
			//is to keep us from turning off blown-out lights
		const auto tm2 = get_texture_index(segnum->unique_segment::sides[sidenum].tmap_num2);
		if (TmapInfo[tm2].lighting) {
				ret |= subtract_light(LevelSharedDestructibleLightState, segnum, sidenum);	//any light sets flag
				disable_flicker(Flickering_light_state, segnum, sidenum);
			}
	};
	trigger_wall_op(t, vmsegptridx, op);
	return ret;
}

// Unlocks all doors linked to the switch.
static void do_unlock_doors(fvcsegptr &vcsegptr, fvmwallptr &vmwallptr, const trigger &t)
{
	const auto op = [&vmwallptr](const shared_segment &segp, const sidenum_t sidenum) {
		const auto wall_num = segp.sides[sidenum].wall_num;
		if (wall_num == wall_none)
			return;
		auto &w = *vmwallptr(wall_num);
		w.flags &= ~wall_flag::door_locked;
		w.keys = wall_key::none;
	};
	trigger_wall_op(t, vcsegptr, op);
}

// Locks all doors linked to the switch.
static void do_lock_doors(fvcsegptr &vcsegptr, fvmwallptr &vmwallptr, const trigger &t)
{
	const auto op = [&vmwallptr](const shared_segment &segp, const sidenum_t sidenum) {
		const auto wall_num = segp.sides[sidenum].wall_num;
		if (wall_num == wall_none)
			return;
		auto &w = *vmwallptr(wall_num);
		w.flags |= wall_flag::door_locked;
	};
	trigger_wall_op(t, vcsegptr, op);
}

// Changes walls pointed to by a trigger. returns true if any walls changed
static int do_change_walls(const trigger &t, const uint8_t new_wall_type)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vmobjptr = Objects.vmptr;
	int ret{0};

	auto &TmapInfo = LevelUniqueTmapInfoState.TmapInfo;
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vmwallptr = Walls.vmptr;
	for (unsigned i = 0; i < t.num_links; ++i)
	{
		sidenum_t cside;
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
			const auto w0num = segp->shared_segment::sides[side].wall_num;
			if (const auto &&uw0p = vmwallptr.check_untrusted(w0num))
				w0p = *uw0p;
			else
			{
				LevelError("trigger %p link %u tried to open segment %hu, side %u which is an invalid wall; ignoring.", std::addressof(t), i, static_cast<segnum_t>(segp), underlying_value(side));
				continue;
			}
			auto &wall0 = *w0p;
			imwallptr_t wall1 = nullptr;
			if ((cside == side_none || csegp->shared_segment::sides[cside].wall_num == wall_none ||
				(wall1 = vmwallptr(csegp->shared_segment::sides[cside].wall_num))->type == new_wall_type) &&
				wall0.type == new_wall_type)
				continue;		//already in correct state, so skip

			ret |= 1;

			auto &vcvertptr = Vertices.vcptr;
			switch (t.type)
			{
				case trigger_action::open_wall:
					if ((TmapInfo[get_texture_index(segp->unique_segment::sides[side].tmap_num)].flags & tmapinfo_flag::force_field))
					{
						ret |= 2;
						digi_link_sound_to_pos(sound_effect::SOUND_FORCEFIELD_OFF, segp, side, compute_center_point_on_side(vcvertptr, segp, side), 0, F1_0);
						digi_kill_sound_linked_to_segment(segp,side,sound_effect::SOUND_FORCEFIELD_HUM);
						wall0.type = new_wall_type;
						if (wall1)
						{
							wall1->type = new_wall_type;
							digi_kill_sound_linked_to_segment(csegp, cside, sound_effect::SOUND_FORCEFIELD_HUM);
						}
					}
					else
						start_wall_cloak(segp,side);
					break;

				case trigger_action::close_wall:
					if ((TmapInfo[get_texture_index(segp->unique_segment::sides[side].tmap_num)].flags & tmapinfo_flag::force_field))
					{
						ret |= 2;
						{
							digi_link_sound_to_pos(sound_effect::SOUND_FORCEFIELD_HUM, segp, side, compute_center_point_on_side(vcvertptr, segp, side), 1, F1_0 / 2);
						}
					case trigger_action::illusory_wall:
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

			LevelUniqueStuckObjectState.kill_stuck_objects(vmobjptr, segp->shared_segment::sides[side].wall_num);
			if (wall1)
				LevelUniqueStuckObjectState.kill_stuck_objects(vmobjptr, csegp->shared_segment::sides[cside].wall_num);
  	}
	flush_fcd_cache();

	return ret;
}

#define print_trigger_message(pnum,trig,shot,message)	\
	((void)((print_trigger_message(pnum,trig,shot)) &&		\
		HUD_init_message(HM_DEFAULT, message, &"s"[trig.num_links <= 1])))

static int (print_trigger_message)(int pnum, const trigger &t, int shot)
 {
	if (shot && pnum == Player_num && !(t.flags & trigger_behavior_flags::no_message))
		return 1;
	return 0;
 }
}

}
#endif

namespace {

static void do_matcen(const trigger &t)
{
	range_for (auto &i, partial_const_range(t.seg, t.num_links))
		trigger_matcen(vmsegptridx(i));
}

static void do_il_on(fvcsegptridx &vcsegptridx, fvmwallptr &vmwallptr, const trigger &t)
{
	trigger_wall_op(t, vcsegptridx, wall_illusion_on, vmwallptr);
}

}

namespace dsx {
namespace {

#if DXX_BUILD_DESCENT == 1
static void do_il_off(fvcsegptridx &vcsegptridx, fvmwallptr &vmwallptr, const trigger &t)
{
	trigger_wall_op(t, vcsegptridx, wall_illusion_off, vmwallptr);
}
#elif DXX_BUILD_DESCENT == 2
static void do_il_off(fvcsegptridx &vcsegptridx, fvcvertptr &vcvertptr, fvmwallptr &vmwallptr, const trigger &t)
{
	const auto &&op = [&vcvertptr, &vmwallptr](const vcsegptridx_t seg, const sidenum_t side) {
		wall_illusion_off(vmwallptr, seg, side);
		digi_link_sound_to_pos(sound_effect::SOUND_WALL_REMOVED, seg, side, compute_center_point_on_side(vcvertptr, seg, side), 0, F1_0);
	};
	trigger_wall_op(t, vcsegptridx, op);
}
#endif

}

// Slight variation on window_event_result meaning
// 'ignored' means we still want check_trigger to call multi_send_trigger
// 'handled' or 'close' means we don't
// 'close' will still close the game window
window_event_result check_trigger_sub(object &plrobj, const trgnum_t trigger_num, const playernum_t pnum, const unsigned shot)
{
#if DXX_BUILD_DESCENT == 2
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
#endif
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;
	auto result = window_event_result::ignored;

	if ((Game_mode & GM_MULTI) && vcplayerptr(pnum)->connected != player_connection_status::playing) // as a host we may want to handle triggers for our clients. to do that properly we must check wether we (host) or client is actually playing.
		return window_event_result::handled;
	auto &Triggers = LevelUniqueWallSubsystemState.Triggers;
	auto &vmtrgptr = Triggers.vmptr;
	auto &trigger = *vmtrgptr(trigger_num);
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vmwallptr = Walls.vmptr;

#if DXX_BUILD_DESCENT == 1
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
				LevelError("Trigger %u is both a regular and secret exit! This is not a recommended combination.", underlying_value(trigger_num));
			if (Newdemo_state == ND_STATE_RECORDING)		// stop demo recording
				Newdemo_state = ND_STATE_PAUSED;

			if (Game_mode & GM_MULTI)
				multi_send_endlevel_start(multi_endlevel_type::secret);
			if (Game_mode & GM_NETWORK)
				multi::dispatch->do_protocol_frame(1, 1);
			result = std::max(PlayerFinishedLevel(next_level_request_secret_flag::use_secret), result);
			LevelUniqueControlCenterState.Control_center_destroyed = 0;
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
		do_il_on(vcsegptridx, vmwallptr, trigger);
	}

	if (trigger.flags & TRIGGER_ILLUSION_OFF) {
		do_il_off(vcsegptridx, vmwallptr, trigger);
	}
#elif DXX_BUILD_DESCENT == 2
	if (trigger.flags & trigger_behavior_flags::disabled)
		return window_event_result::handled;		// don't send trigger hit to other players

	if (trigger.flags & trigger_behavior_flags::one_shot)		//if this is a one-shot...
		trigger.flags |= trigger_behavior_flags::disabled;		//..then don't let it happen again

	auto &LevelSharedDestructibleLightState = LevelSharedSegmentState.DestructibleLights;
	auto &TmapInfo = LevelUniqueTmapInfoState.TmapInfo;
	auto &vcvertptr = Vertices.vcptr;
	switch (trigger.type)
	{
		case trigger_action::normal_exit:
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
			}
			return std::max(result, window_event_result::handled);
			break;

		case trigger_action::secret_exit: {
			int	truth;

			if (pnum!=Player_num)
				break;

			if (plrobj.shields < 0 ||
				Player_dead_state != player_dead_state::no)
				break;

			if (is_SHAREWARE || is_MAC_SHARE) {
				HUD_init_message_literal(HM_DEFAULT, "Secret Level Teleporter disabled in Descent 2 Demo");
				digi_play_sample( sound_effect::SOUND_BAD_SELECTION, F1_0 );
				break;
			}

			if (Game_mode & GM_MULTI) {
				HUD_init_message_literal(HM_DEFAULT, "Secret Level Teleporter disabled in multiplayer!");
				digi_play_sample( sound_effect::SOUND_BAD_SELECTION, F1_0 );
				break;
			}

			truth = p_secret_level_destroyed();

			if (Newdemo_state == ND_STATE_RECORDING)			// record whether we're really going to the secret level
				newdemo_record_secret_exit_blown(truth);

			if ((Newdemo_state != ND_STATE_PLAYBACK) && truth) {
				HUD_init_message_literal(HM_DEFAULT, "Secret Level destroyed.  Exit disabled.");
				digi_play_sample( sound_effect::SOUND_BAD_SELECTION, F1_0 );
				break;
			}

			if (Newdemo_state == ND_STATE_RECORDING)		// stop demo recording
				Newdemo_state = ND_STATE_PAUSED;

			digi_stop_digi_sounds();

			EnterSecretLevel();
			LevelUniqueControlCenterState.Control_center_destroyed = 0;
			return window_event_result::handled;
		}

		case trigger_action::open_door:
			do_link(trigger);
			print_trigger_message(pnum, trigger, shot, "Door%s opened!");

			break;

		case trigger_action::close_door:
			do_close_door(trigger);
			print_trigger_message(pnum, trigger, shot, "Door%s closed!");
			break;

		case trigger_action::unlock_door:
			do_unlock_doors(vcsegptr, vmwallptr, trigger);
			print_trigger_message(pnum, trigger, shot, "Door%s unlocked!");

			break;

		case trigger_action::lock_door:
			do_lock_doors(vcsegptr, vmwallptr, trigger);
			print_trigger_message(pnum, trigger, shot, "Door%s locked!");
			break;

		case trigger_action::open_wall:
			if (const auto w = do_change_walls(trigger, WALL_OPEN))
				print_trigger_message(pnum, trigger, shot, (w & 2) ? "Force field%s deactivated!" : "Wall%s opened!");
			break;

		case trigger_action::close_wall:
			if (const auto w = do_change_walls(trigger, WALL_CLOSED))
				print_trigger_message(pnum, trigger, shot, (w & 2) ? "Force field%s activated!" : "Wall%s closed!");
			break;

		case trigger_action::illusory_wall:
			//don't know what to say, so say nothing
			do_change_walls(trigger, WALL_ILLUSION);
			break;

		case trigger_action::matcen:
			if (!(Game_mode & GM_MULTI) || (Game_mode & GM_MULTI_ROBOTS))
				do_matcen(trigger);
			break;

		case trigger_action::illusion_on:
			do_il_on(vcsegptridx, vmwallptr, trigger);
			print_trigger_message(pnum, trigger, shot, "Illusion%s on!");
			break;

		case trigger_action::illusion_off:
			do_il_off(vcsegptridx, vcvertptr, vmwallptr, trigger);
			print_trigger_message(pnum, trigger, shot, "Illusion%s off!");
			break;

		case trigger_action::light_off:
			if (do_light_off(LevelSharedDestructibleLightState, TmapInfo, Flickering_light_state, trigger))
				print_trigger_message(pnum, trigger, shot, "Light%s off!");
			break;

		case trigger_action::light_on:
			if (do_light_on(LevelSharedDestructibleLightState, TmapInfo, Flickering_light_state, trigger))
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
window_event_result check_trigger(const vcsegptridx_t seg, const sidenum_t side, object &plrobj, const vcobjptridx_t objnum, int shot)
{
	if ((Game_mode & GM_MULTI) && (get_local_player().connected != player_connection_status::playing)) // as a host we may want to handle triggers for our clients. so this function may be called when we are not playing.
		return window_event_result::ignored;

#if DXX_BUILD_DESCENT == 1
	if (objnum == &plrobj)
#elif DXX_BUILD_DESCENT == 2
	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	if (objnum == &plrobj || (objnum->type == OBJ_ROBOT && Robot_info[get_robot_id(objnum)].companion))
#endif
	{

#if DXX_BUILD_DESCENT == 1
		if ( Newdemo_state == ND_STATE_PLAYBACK )
			return window_event_result::ignored;
#elif DXX_BUILD_DESCENT == 2
		if ( Newdemo_state == ND_STATE_RECORDING )
			newdemo_record_trigger( seg, side, objnum,shot);
#endif

		const auto wall_num = seg->shared_segment::sides[side].wall_num;
		if ( wall_num == wall_none ) return window_event_result::ignored;

		auto &Walls = LevelUniqueWallSubsystemState.Walls;
		auto &vcwallptr = Walls.vcptr;
		const auto trigger_num = vcwallptr(wall_num)->trigger;
		if (trigger_num == trigger_none)
			return window_event_result::ignored;

		{
			auto result = check_trigger_sub(plrobj, trigger_num, Player_num,shot);
			if (result != window_event_result::ignored)
				return result;
		}

		if (Game_mode & GM_MULTI)
			multi_send_trigger(trigger_num);
	}

	return window_event_result::handled;
}

/*
 * reads a v29_trigger structure from a PHYSFS_File
 */
#if DXX_BUILD_DESCENT == 1
void v26_trigger_read(const NamedPHYSFS_File fp, trigger &t)
{
	switch (const auto type = static_cast<trigger_action>(PHYSFSX_readByte(fp)))
	{
		case trigger_action::open_door: // door
			t.flags = TRIGGER_CONTROL_DOORS;
			break;
		case trigger_action::matcen: // matcen
			t.flags = TRIGGER_MATCEN;
			break;
		case trigger_action::normal_exit: // exit
			t.flags = TRIGGER_EXIT;
			break;
		case trigger_action::secret_exit: // secret exit
			t.flags = TRIGGER_SECRET_EXIT;
			break;
		case trigger_action::illusion_off: // illusion off
			t.flags = TRIGGER_ILLUSION_OFF;
			break;
		case trigger_action::illusion_on: // illusion on
			t.flags = TRIGGER_ILLUSION_ON;
			break;
		default:
			con_printf(CON_URGENT, "error: unsupported trigger type %d", static_cast<int>(type));
			throw std::runtime_error("unsupported trigger type");
	}
	if (PHYSFSX_readByte(fp) & 2)	// one shot
		t.flags |= TRIGGER_ONE_SHOT;
	t.num_links = PHYSFSX_readSLE16(fp);
	t.value = PHYSFSX_readInt(fp);
	PHYSFSX_skipBytes<4>(fp);
	for (unsigned i=0; i < MAX_WALLS_PER_LINK; i++ )
		t.seg[i] = read_untrusted_segnum_le16(fp);
	for (unsigned i=0; i < MAX_WALLS_PER_LINK; i++ )
	{
		auto s = build_sidenum_from_untrusted(PHYSFSX_readSLE16(fp));
		t.side[i] = s.value_or(sidenum_t::WLEFT);
	}
}

void v25_trigger_read(const NamedPHYSFS_File fp, trigger *t)
#elif DXX_BUILD_DESCENT == 2
void v29_trigger_read(v29_trigger *t, const NamedPHYSFS_File fp)
#endif
{
#if DXX_BUILD_DESCENT == 1
	PHYSFSX_skipBytes<1>(fp);
#elif DXX_BUILD_DESCENT == 2
	t->type = PHYSFSX_readByte(fp);
#endif
	t->flags = PHYSFSX_readSLE16(fp);
	t->value = PHYSFSX_readFix(fp);
	PHYSFSX_skipBytes<5>(fp);
	t->num_links = PHYSFSX_readSLE16(fp);
	for (unsigned i=0; i<MAX_WALLS_PER_LINK; i++ )
		t->seg[i] = read_untrusted_segnum_le16(fp);
	for (unsigned i=0; i<MAX_WALLS_PER_LINK; i++ )
	{
		auto s = build_sidenum_from_untrusted(PHYSFSX_readSLE16(fp));
		t->side[i] = s.value_or(sidenum_t::WLEFT);
	}
}

#if DXX_BUILD_DESCENT == 2
/*
 * reads a v30_trigger structure from a PHYSFS_File
 */
void v30_trigger_read(v30_trigger *t, const NamedPHYSFS_File fp)
{
	t->flags = PHYSFSX_readSLE16(fp);
	t->num_links = PHYSFSX_readByte(fp);
	t->pad = PHYSFSX_readByte(fp);
	t->value = PHYSFSX_readFix(fp);
	PHYSFSX_skipBytes<4>(fp);
	for (unsigned i=0; i<MAX_WALLS_PER_LINK; i++ )
		t->seg[i] = read_untrusted_segnum_le16(fp);
	for (unsigned i=0; i<MAX_WALLS_PER_LINK; i++ )
		t->side[i] = build_sidenum_from_untrusted(PHYSFSX_readSLE16(fp)).value_or(sidenum_t::WLEFT);
}

namespace {

static trigger_action trigger_type_from_flags(short flags)
{
	if (flags & TRIGGER_CONTROL_DOORS)
		return trigger_action::open_door;
	else if (flags & (TRIGGER_SHIELD_DAMAGE | TRIGGER_ENERGY_DRAIN))
	{
	}
	else if (flags & TRIGGER_EXIT)
		return trigger_action::normal_exit;
	else if (flags & TRIGGER_MATCEN)
		return trigger_action::matcen;
	else if (flags & TRIGGER_ILLUSION_OFF)
		return trigger_action::illusion_off;
	else if (flags & TRIGGER_SECRET_EXIT)
		return trigger_action::secret_exit;
	else if (flags & TRIGGER_ILLUSION_ON)
		return trigger_action::illusion_on;
	else if (flags & TRIGGER_UNLOCK_DOORS)
		return trigger_action::unlock_door;
	else if (flags & TRIGGER_OPEN_WALL)
		return trigger_action::open_wall;
	else if (flags & TRIGGER_CLOSE_WALL)
		return trigger_action::close_wall;
	else if (flags & TRIGGER_ILLUSORY_WALL)
		return trigger_action::illusory_wall;
	throw std::runtime_error("unsupported trigger type");
}

static void v30_trigger_to_v31_trigger(trigger &t, const v30_trigger &trig)
{
	t.type        = trigger_type_from_flags(trig.flags);
	t.flags       = (trig.flags & TRIGGER_ONE_SHOT) ? trigger_behavior_flags::one_shot : trigger_behavior_flags{0};
	t.num_links   = trig.num_links;
	t.num_links   = trig.num_links;
	t.value       = trig.value;
	t.seg = trig.seg;
	for (auto &&[w, r] : zip(t.side, trig.side))
	{
		w = r;
	}
}

static void v29_trigger_read_as_v30(const NamedPHYSFS_File fp, v30_trigger &trig)
{
	v29_trigger trig29;
	v29_trigger_read(&trig29, fp);
	trig.flags	= trig29.flags;
	// skip trig29.link_num. v30_trigger does not need it
	trig.num_links	= trig29.num_links;
	trig.value	= trig29.value;
	trig.seg = trig29.seg;
	trig.side = trig29.side;
}

}

void v29_trigger_read_as_v31(const NamedPHYSFS_File fp, trigger &t)
{
	v30_trigger trig;
	v29_trigger_read_as_v30(fp, trig);
	v30_trigger_to_v31_trigger(t, trig);
}

void v30_trigger_read_as_v31(const NamedPHYSFS_File fp, trigger &t)
{
	v30_trigger trig;
	v30_trigger_read(&trig, fp);
	v30_trigger_to_v31_trigger(t, trig);
}
#endif

namespace {

template <typename array_type>
struct serialize_wide_trigger_side_numbers
{
	array_type &side;
	serialize_wide_trigger_side_numbers(array_type &t) :
		side(t)
	{
	}
};

template <typename array_type>
serial::message<std::array<uint16_t, std::tuple_size<array_type>::value>> udt_to_message(const serialize_wide_trigger_side_numbers<array_type> &);

template <typename Accessor, typename array_type>
void process_udt(Accessor &accessor, serialize_wide_trigger_side_numbers<array_type> wrapper)
{
	static_assert(std::is_const<array_type>::value != std::is_const<typename Accessor::value_type>::value);
	std::array<uint16_t, MAX_WALLS_PER_LINK> a;
	if constexpr (std::is_const<array_type>::value)
	{
		/* The array is constant, so it is already initialized and
		 * should be written out.
		 */
		for (auto &&[w, r] : zip(a, wrapper.side))
			w = underlying_value(r);
		process_buffer(accessor, a);
	}
	else
	{
		/* The array is non-constant, so it is initially undefined and
		 * should be read in.
		 */
		process_buffer(accessor, a);
		for (auto &&[w, r] : zip(wrapper.side, a))
		{
			auto s = build_sidenum_from_untrusted(r);
			w = s.value_or(sidenum_t::WLEFT);
		}
	}
}

}

#if DXX_BUILD_DESCENT == 1
DEFINE_SERIAL_UDT_TO_MESSAGE(trigger, t, (serial::pad<1>(), t.flags, t.value, serial::pad<5>(), t.num_links, serial::pad<1, 0>(), t.seg, serialize_wide_trigger_side_numbers{t.side}));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(trigger, 54);
#elif DXX_BUILD_DESCENT == 2
DEFINE_SERIAL_UDT_TO_MESSAGE(trigger, t, (t.type, t.flags, t.num_links, serial::pad<1>(), t.value, serial::pad<4>(), t.seg, serialize_wide_trigger_side_numbers{t.side}));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(trigger, 52);
#endif

void trigger_read(const NamedPHYSFS_File fp, trigger &t)
{
	PHYSFSX_serialize_read(fp, t);
}

void trigger_write(PHYSFS_File *fp, const trigger &t)
{
	PHYSFSX_serialize_write(fp, t);
}

void v29_trigger_write(PHYSFS_File *fp, const trigger &rt)
{
	const trigger *t = &rt;
	PHYSFSX_writeU8(fp, 0);		// unused 'type'
#if DXX_BUILD_DESCENT == 1
	PHYSFS_writeSLE16(fp, t->flags);
#elif DXX_BUILD_DESCENT == 2
	const auto one_shot_flag = (t->flags & trigger_behavior_flags::one_shot) ? TRIGGER_ONE_SHOT : TRIGGER_FLAG{0};
	switch (t->type)
	{
		case trigger_action::open_door:
			PHYSFS_writeSLE16(fp, TRIGGER_CONTROL_DOORS | one_shot_flag);
			break;

		case trigger_action::normal_exit:
			PHYSFS_writeSLE16(fp, TRIGGER_EXIT | one_shot_flag);
			break;

		case trigger_action::matcen:
			PHYSFS_writeSLE16(fp, TRIGGER_MATCEN | one_shot_flag);
			break;

		case trigger_action::illusion_off:
			PHYSFS_writeSLE16(fp, TRIGGER_ILLUSION_OFF | one_shot_flag);
			break;

		case trigger_action::secret_exit:
			PHYSFS_writeSLE16(fp, TRIGGER_SECRET_EXIT | one_shot_flag);
			break;

		case trigger_action::illusion_on:
			PHYSFS_writeSLE16(fp, TRIGGER_ILLUSION_ON | one_shot_flag);
			break;

		case trigger_action::unlock_door:
			PHYSFS_writeSLE16(fp, TRIGGER_UNLOCK_DOORS | one_shot_flag);
			break;

		case trigger_action::open_wall:
			PHYSFS_writeSLE16(fp, TRIGGER_OPEN_WALL | one_shot_flag);
			break;

		case trigger_action::close_wall:
			PHYSFS_writeSLE16(fp, TRIGGER_CLOSE_WALL | one_shot_flag);
			break;

		case trigger_action::illusory_wall:
			PHYSFS_writeSLE16(fp, TRIGGER_ILLUSORY_WALL | one_shot_flag);
			break;

		default:
			Int3();
			PHYSFS_writeSLE16(fp, 0);
			break;
	}
#endif

	PHYSFSX_writeFix(fp, t->value);
	PHYSFSX_writeFix(fp, 0);

	PHYSFSX_writeU8(fp, -1);	//t->link_num
	PHYSFS_writeSLE16(fp, t->num_links);

	for (unsigned i = 0; i < MAX_WALLS_PER_LINK; i++)
		PHYSFS_writeSLE16(fp, t->seg[i]);
	for (const auto i : xrange(MAX_WALLS_PER_LINK))
		PHYSFS_writeSLE16(fp, underlying_value(t->side[i]));
}
}

namespace dsx {
void v30_trigger_write(PHYSFS_File *fp, const trigger &rt)
{
	const trigger *t = &rt;
#if DXX_BUILD_DESCENT == 1
	uint8_t action;
	if (t->flags & TRIGGER_CONTROL_DOORS)
		action = static_cast<uint8_t>(trigger_action::open_door); // door
	else if (t->flags & TRIGGER_MATCEN)
		action = static_cast<uint8_t>(trigger_action::matcen); // matcen
	else if (t->flags & TRIGGER_EXIT)
		action = static_cast<uint8_t>(trigger_action::normal_exit); // exit
	else if (t->flags & TRIGGER_SECRET_EXIT)
		action = static_cast<uint8_t>(trigger_action::secret_exit); // secret exit
	else if (t->flags & TRIGGER_ILLUSION_OFF)
		action = static_cast<uint8_t>(trigger_action::illusion_off); // illusion off
	else if (t->flags & TRIGGER_ILLUSION_ON)
		action = static_cast<uint8_t>(trigger_action::illusion_on); // illusion on
	else
		action = 0;
	PHYSFSX_writeU8(fp, action);
#elif DXX_BUILD_DESCENT == 2
	PHYSFSX_writeU8(fp, static_cast<uint8_t>(t->type));
#endif

#if DXX_BUILD_DESCENT == 1
	PHYSFS_writeSLE16(fp, t->flags);
#elif DXX_BUILD_DESCENT == 2
	const auto one_shot_flag = (t->flags & trigger_behavior_flags::one_shot) ? TRIGGER_ONE_SHOT : TRIGGER_FLAG{0};
	switch (t->type)
	{
		case trigger_action::open_door:
			PHYSFS_writeSLE16(fp, TRIGGER_CONTROL_DOORS | one_shot_flag);
			break;

		case trigger_action::normal_exit:
			PHYSFS_writeSLE16(fp, TRIGGER_EXIT | one_shot_flag);
			break;

		case trigger_action::matcen:
			PHYSFS_writeSLE16(fp, TRIGGER_MATCEN | one_shot_flag);
			break;

		case trigger_action::illusion_off:
			PHYSFS_writeSLE16(fp, TRIGGER_ILLUSION_OFF | one_shot_flag);
			break;

		case trigger_action::secret_exit:
			PHYSFS_writeSLE16(fp, TRIGGER_SECRET_EXIT | one_shot_flag);
			break;

		case trigger_action::illusion_on:
			PHYSFS_writeSLE16(fp, TRIGGER_ILLUSION_ON | one_shot_flag);
			break;

		case trigger_action::unlock_door:
			PHYSFS_writeSLE16(fp, TRIGGER_UNLOCK_DOORS | one_shot_flag);
			break;

		case trigger_action::open_wall:
			PHYSFS_writeSLE16(fp, TRIGGER_OPEN_WALL | one_shot_flag);
			break;

		case trigger_action::close_wall:
			PHYSFS_writeSLE16(fp, TRIGGER_CLOSE_WALL | one_shot_flag);
			break;

		case trigger_action::illusory_wall:
			PHYSFS_writeSLE16(fp, TRIGGER_ILLUSORY_WALL | one_shot_flag);
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
	for (const auto i : xrange(MAX_WALLS_PER_LINK))
		PHYSFS_writeSLE16(fp, underlying_value(t->side[i]));
}
}

namespace dsx {
void v31_trigger_write(PHYSFS_File *fp, const trigger &rt)
{
	const trigger *t = &rt;
#if DXX_BUILD_DESCENT == 1
	uint8_t action;
	if (t->flags & TRIGGER_CONTROL_DOORS)
		action = static_cast<uint8_t>(trigger_action::open_door); // door
	else if (t->flags & TRIGGER_MATCEN)
		action = static_cast<uint8_t>(trigger_action::matcen); // matcen
	else if (t->flags & TRIGGER_EXIT)
		action = static_cast<uint8_t>(trigger_action::normal_exit); // exit
	else if (t->flags & TRIGGER_SECRET_EXIT)
		action = static_cast<uint8_t>(trigger_action::secret_exit); // secret exit
	else if (t->flags & TRIGGER_ILLUSION_OFF)
		action = static_cast<uint8_t>(trigger_action::illusion_off); // illusion off
	else if (t->flags & TRIGGER_ILLUSION_ON)
		action = static_cast<uint8_t>(trigger_action::illusion_on); // illusion on
	else
		action = 0;
	PHYSFSX_writeU8(fp, action);
#elif DXX_BUILD_DESCENT == 2
	PHYSFSX_writeU8(fp, static_cast<uint8_t>(t->type));
#endif

#if DXX_BUILD_DESCENT == 1
	PHYSFSX_writeU8(fp, (t->flags & TRIGGER_ONE_SHOT) ? 2 : 0);		// flags
#elif DXX_BUILD_DESCENT == 2
	PHYSFSX_writeU8(fp, static_cast<uint8_t>(t->flags));
#endif

	PHYSFSX_writeU8(fp, t->num_links);
	PHYSFSX_writeU8(fp, 0);	// t->pad

	PHYSFSX_writeFix(fp, t->value);
	PHYSFSX_writeFix(fp, 0);

	for (unsigned i = 0; i < MAX_WALLS_PER_LINK; i++)
		PHYSFS_writeSLE16(fp, t->seg[i]);
	for (const auto i : xrange(MAX_WALLS_PER_LINK))
		PHYSFS_writeSLE16(fp, underlying_value(t->side[i]));
}
}
