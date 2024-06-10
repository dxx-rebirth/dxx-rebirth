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
 * Escort robot behavior.
 *
 */

#include <ranges>
#include "dxxsconf.h"
#include <stdio.h>		// for printf()
#include <stdlib.h>		// for rand() and qsort()
#include <string.h>		// for memset()

#include "window.h"
#include "console.h"
#include "vecmat.h"
#include "gr.h"
#include "gameseg.h"
#include "3d.h"
#include "palette.h"
#include "timer.h"
#include "u_mem.h"
#include "weapon.h"

#include "object.h"
#include "dxxerror.h"
#include "ai.h"
#include "robot.h"
#include "fvi.h"
#include "physics.h"
#include "wall.h"
#include "player.h"
#include "fireball.h"
#include "game.h"
#include "powerup.h"
#include "hudmsg.h"
#include "cntrlcen.h"
#include "gauges.h"
#include "event.h"
#include "key.h"
#include "fuelcen.h"
#include "sounds.h"
#include "screens.h"
#include "text.h"
#include "gamefont.h"
#include "newmenu.h"
#include "playsave.h"
#include "gameseq.h"
#include "automap.h"
#include "laser.h"
#include "escort.h"
#include "vclip.h"
#include "segiter.h"
#include "compiler-range_for.h"
#include "d_enumerate.h"
#include "d_levelstate.h"
#include "d_range.h"
#include "d_zip.h"

#if DXX_USE_EDITOR
#include "editor/editor.h"
#endif

namespace dsx {

namespace {

static void say_escort_goal(escort_goal_t goal_num);

constexpr std::array<char[12], ESCORT_GOAL_MARKER9> Escort_goal_text = {{
	"BLUE KEY",
	"YELLOW KEY",
	"RED KEY",
	"REACTOR",
	"EXIT",
	"ENERGY",
	"ENERGYCEN",
	"SHIELD",
	"POWERUP",
	"ROBOT",
	"HOSTAGES",
	"SPEW",
	"SCRAM",
	"EXIT",
	"BOSS",
	"MARKER 1",
	"MARKER 2",
	"MARKER 3",
	"MARKER 4",
	"MARKER 5",
	"MARKER 6",
	"MARKER 7",
	"MARKER 8",
	"MARKER 9",
// -- too much work -- 	"KAMIKAZE  "
}};

constexpr std::integral_constant<unsigned, 200> Max_escort_length{};

#define DXX_GUIDEBOT_RENAME_MENU(VERB)	\
	DXX_MENUITEM(VERB, INPUT, guidebot_name_buffer, opt_name)	\

struct rename_guidebot_menu_items
{
	std::array<newmenu_item, DXX_GUIDEBOT_RENAME_MENU(COUNT)> m;
	decltype(PlayerCfg.GuidebotName) guidebot_name_buffer;
	enum
	{
		DXX_GUIDEBOT_RENAME_MENU(ENUM)
	};
	rename_guidebot_menu_items()
	{
		guidebot_name_buffer = PlayerCfg.GuidebotName;
		DXX_GUIDEBOT_RENAME_MENU(ADD);
	}
};

#undef DXX_GUIDEBOT_RENAME_MENU

struct rename_guidebot_menu : rename_guidebot_menu_items, passive_newmenu
{
	rename_guidebot_menu(grs_canvas &src) :
		passive_newmenu(menu_title{nullptr}, menu_subtitle{"Enter Guide-bot name:"}, menu_filename{nullptr}, tiny_mode_flag::normal, tab_processing_flag::ignore, adjusted_citem::create(m, 0), src)
	{
	}
	virtual window_event_result event_handler(const d_event &event) override;
};

window_event_result rename_guidebot_menu::event_handler(const d_event &event)
{
	switch (event.type)
	{
		case event_type::window_close:
			{
				uint8_t changed = 0;
				if (PlayerCfg.GuidebotName != guidebot_name_buffer)
				{
					PlayerCfg.GuidebotName = guidebot_name_buffer;
					changed = 1;
				}
				if (PlayerCfg.GuidebotNameReal != guidebot_name_buffer)
				{
					PlayerCfg.GuidebotNameReal = guidebot_name_buffer;
					changed = 1;
				}
				if (changed)
					write_player_file();
			}
			return window_event_result::ignored;
		default:
			return newmenu::event_handler(event);
	}
}

}

void init_buddy_for_level(void)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	auto &BuddyState = LevelUniqueObjectState.BuddyState;
	BuddyState = {};
	BuddyState.Buddy_gave_hint_count = 5;
	BuddyState.Looking_for_marker = game_marker_index::None;
	BuddyState.Escort_goal_object = ESCORT_GOAL_UNSPECIFIED;
	BuddyState.Escort_special_goal = ESCORT_GOAL_UNSPECIFIED;
	BuddyState.Last_buddy_key = -1;
	BuddyState.Buddy_sorry_time = -F1_0;
	BuddyState.Buddy_last_seen_player = 0;
	BuddyState.Buddy_last_missile_time = 0;
	BuddyState.Last_time_buddy_gave_hint = 0;
	BuddyState.Last_come_back_message_time = 0;
	BuddyState.Escort_last_path_created = 0;
	BuddyState.Buddy_last_player_path_created = 0;
	BuddyState.Last_buddy_message_time = 0;
	BuddyState.Buddy_objnum = find_escort(vmobjptridx, Robot_info);
}

namespace {

//	-----------------------------------------------------------------------------
//	See if segment from curseg through sidenum is reachable.
//	Return true if it is reachable, else return false.
static int segment_is_reachable(const object &robot, const robot_info &robptr, const shared_segment &segp, const sidenum_t sidenum, const player_flags powerup_flags)
{
	const auto wall_num = segp.sides[sidenum].wall_num;

	//	If no wall, then it is reachable
	if (wall_num == wall_none)
		return 1;

	return ai_door_is_openable(robot, &robptr, powerup_flags, segp, sidenum);

// -- MK, 10/17/95 -- 
// -- MK, 10/17/95 -- 	//	Hmm, a closed wall.  I think this mean not reachable.
// -- MK, 10/17/95 -- 	if (Walls[wall_num].type == WALL_CLOSED)
// -- MK, 10/17/95 -- 		return 0;
// -- MK, 10/17/95 -- 
// -- MK, 10/17/95 -- 	if (Walls[wall_num].type == WALL_DOOR) {
// -- MK, 10/17/95 -- 		if (Walls[wall_num].keys == KEY_NONE) {
// -- MK, 10/17/95 -- 			return 1;		//	@MK, 10/17/95: Be consistent with ai_door_is_openable
// -- MK, 10/17/95 -- // -- 			if (Walls[wall_num].flags & WALL_DOOR_LOCKED)
// -- MK, 10/17/95 -- // -- 				return 0;
// -- MK, 10/17/95 -- // -- 			else
// -- MK, 10/17/95 -- // -- 				return 1;
// -- MK, 10/17/95 -- 		} else if (Walls[wall_num].keys == KEY_BLUE)
// -- MK, 10/17/95 -- 			return (Players[Player_num].flags & PLAYER_FLAGS_BLUE_KEY);
// -- MK, 10/17/95 -- 		else if (Walls[wall_num].keys == KEY_GOLD)
// -- MK, 10/17/95 -- 			return (Players[Player_num].flags & PLAYER_FLAGS_GOLD_KEY);
// -- MK, 10/17/95 -- 		else if (Walls[wall_num].keys == KEY_RED)
// -- MK, 10/17/95 -- 			return (Players[Player_num].flags & PLAYER_FLAGS_RED_KEY);
// -- MK, 10/17/95 -- 		else
// -- MK, 10/17/95 -- 			Int3();	//	Impossible!  Doesn't have no key, but doesn't have any key!
// -- MK, 10/17/95 -- 	} else
// -- MK, 10/17/95 -- 		return 1;
// -- MK, 10/17/95 -- 
// -- MK, 10/17/95 -- 	Int3();	//	Hmm, thought 'if' above had to return!
// -- MK, 10/17/95 -- 	return 0;

}

}

//	-----------------------------------------------------------------------------
//	Create a breadth-first list of segments reachable from current segment.
//	max_segs is maximum number of segments to search.  Use MAX_SEGMENTS to search all.
//	On exit, *length <= max_segs.
//	Input:
//		start_seg
//	Output:
//		bfs_list:	array of shorts, each reachable segment.  Includes start segment.
//		length:		number of elements in bfs_list
std::size_t create_bfs_list(const object &robot, const robot_info &robptr, const vcsegidx_t start_seg, const player_flags powerup_flags, const std::span<segnum_t> bfs_list)
{
	std::size_t head = 0, tail = 0;
	visited_segment_bitarray_t visited;
	bfs_list[head++] = start_seg;
	visited[start_seg] = true;

	while (head != tail && head < bfs_list.size())
	{
		auto curseg = bfs_list[tail++];
		const auto &&cursegp = vcsegptr(curseg);
		for (const auto &&[i, connected_seg] : enumerate(cursegp->children))
		{
			if (IS_CHILD(connected_seg) && (!visited[connected_seg])) {
				if (segment_is_reachable(robot, robptr, cursegp, static_cast<sidenum_t>(i), powerup_flags)) {
					bfs_list[head++] = connected_seg;
					if (head >= bfs_list.size())
						break;
					visited[connected_seg] = true;
					Assert(head < MAX_SEGMENTS);
				}
			}
		}
	}
	return head;
}

namespace {

//	-----------------------------------------------------------------------------
//	Return true if ok for buddy to talk, else return false.
//	Buddy is allowed to talk if the segment he is in does not contain a blastable wall that has not been blasted
//	AND he has never yet, since being initialized for level, been allowed to talk.
static uint8_t ok_for_buddy_to_talk(void)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	auto &BuddyState = LevelUniqueObjectState.BuddyState;
	const auto Buddy_objnum = BuddyState.Buddy_objnum;
	if (Buddy_objnum == object_none)
		return 0;

	const vmobjptridx_t buddy = vmobjptridx(Buddy_objnum);
	const auto buddy_type = buddy->type;
	if (buddy_type != OBJ_ROBOT) {
		BuddyState.Buddy_allowed_to_talk = 0;
		BuddyState.Buddy_objnum = object_none;
		con_printf(CON_URGENT, "BUG: buddy is object %u, but that object is type %u.", Buddy_objnum.get_unchecked_index(), buddy_type);
		return 0;
	}

	if (const auto Buddy_allowed_to_talk = BuddyState.Buddy_allowed_to_talk)
		return Buddy_allowed_to_talk;

	const shared_segment &segp = vcsegptr(buddy->segnum);

	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcwallptr = Walls.vcptr;
	for (const auto &&[side, child] : zip(segp.sides, segp.children))
	{
		const auto wall_num = side.wall_num;
		if (wall_num != wall_none) {
			auto &w = *vcwallptr(wall_num);
			if (w.type == WALL_BLASTABLE && !(w.flags & wall_flag::blasted))
				return 0;
		}

		//	Check one level deeper.
		if (IS_CHILD(child))
		{
			const shared_segment &cseg = *vcsegptr(child);
			range_for (const auto &j, cseg.sides)
			{
				auto wall2 = j.wall_num;
				if (wall2 != wall_none) {
					auto &w = *vcwallptr(wall2);
					if (w.type == WALL_BLASTABLE && !(w.flags & wall_flag::blasted))
						return 0;
				}
			}
		}
	}

	BuddyState.Buddy_allowed_to_talk = 1;
	return 1;
}

static void record_escort_goal_accomplished()
{
	auto &BuddyState = LevelUniqueObjectState.BuddyState;
	if (ok_for_buddy_to_talk()) {
		digi_play_sample_once(SOUND_BUDDY_MET_GOAL, F1_0);
		BuddyState.Escort_goal_objidx = object_none;
		BuddyState.Escort_goal_reachable = d_unique_buddy_state::Escort_goal_reachability::unreachable;
		BuddyState.Looking_for_marker = game_marker_index::None;
		BuddyState.Escort_goal_object = ESCORT_GOAL_UNSPECIFIED;
		BuddyState.Escort_special_goal = ESCORT_GOAL_UNSPECIFIED;
	}
}

}

//	--------------------------------------------------------------------------------------------
void detect_escort_goal_fuelcen_accomplished()
{
	auto &BuddyState = LevelUniqueObjectState.BuddyState;
	if (!BuddyState.Buddy_allowed_to_talk)
		return;
	if (BuddyState.Escort_special_goal == ESCORT_GOAL_ENERGYCEN)
		record_escort_goal_accomplished();
}

void detect_escort_goal_accomplished(const vmobjptridx_t index)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	auto &BuddyState = LevelUniqueObjectState.BuddyState;
	if (!BuddyState.Buddy_allowed_to_talk)
		return;

	// If goal is not an object (FUELCEN, EXIT, SCRAM), bail. FUELCEN is handled in detect_escort_goal_fuelcen_accomplished(), EXIT and SCRAM are never accomplished.
	if (BuddyState.Escort_goal_reachable == d_unique_buddy_state::Escort_goal_reachability::unreachable)
		return;
	const auto Escort_goal_objidx = BuddyState.Escort_goal_objidx;
	if (Escort_goal_objidx == object_none)
	{
		con_puts(CON_URGENT, "BUG: buddy goal is reachable, but goal object is object_none");
		return;
	}

	// See if goal found was a key.  Need to handle default goals differently.
	// Note, no buddy_met_goal sound when blow up reactor or exit.  Not great, but ok
	// since for reactor, noisy, for exit, buddy is disappearing.
	if (BuddyState.Escort_special_goal == ESCORT_GOAL_UNSPECIFIED && Escort_goal_objidx == index)
	{
		record_escort_goal_accomplished();
		return;
	}

	if (index->type == OBJ_POWERUP)  {
		const auto index_id = get_powerup_id(index);
		escort_goal_t goal_key;
		if ((index_id == powerup_type_t::POW_KEY_BLUE && (goal_key = ESCORT_GOAL_BLUE_KEY, true)) ||
			(index_id == powerup_type_t::POW_KEY_GOLD && (goal_key = ESCORT_GOAL_GOLD_KEY, true)) ||
			(index_id == powerup_type_t::POW_KEY_RED && (goal_key = ESCORT_GOAL_RED_KEY, true))
		)
		{
			if (BuddyState.Escort_goal_object == goal_key)
			{
				record_escort_goal_accomplished();
				return;
			}
		}
	}
	if (BuddyState.Escort_special_goal != ESCORT_GOAL_UNSPECIFIED)
	{
		if (index->type == OBJ_POWERUP && BuddyState.Escort_special_goal == ESCORT_GOAL_POWERUP)
			record_escort_goal_accomplished();	//	Any type of powerup picked up will do.
		else
		{
			auto &egi_obj = *vcobjptr(Escort_goal_objidx);
			if (index->type == egi_obj.type && index->id == egi_obj.id)
				// Note: This will help a little bit in making the buddy believe a goal is satisfied.  Won't work for a general goal like "find any powerup"
				// because of the insistence of both type and id matching.
				record_escort_goal_accomplished();
		}
	}
}

void change_guidebot_name()
{
	auto menu = window_create<rename_guidebot_menu>(grd_curscreen->sc_canvas);
	(void)menu;
}

namespace {

//	-----------------------------------------------------------------------------
static uint8_t show_buddy_message()
{
	auto &BuddyState = LevelUniqueObjectState.BuddyState;
	if (BuddyState.Buddy_messages_suppressed)
		return 0;

	if (Game_mode & GM_MULTI)
	{
		if (!Netgame.AllowGuidebot)
			return 0;
	}

	if (BuddyState.Last_buddy_message_time + F1_0 < GameTime64) {
		if (const auto r = ok_for_buddy_to_talk())
			return r;
	}
	return 0;
}

__attribute_nonnull()
static void buddy_message_force_str(const char *str)
{
	auto &BuddyState = LevelUniqueObjectState.BuddyState;
	BuddyState.Last_buddy_message_time = {GameTime64};
	HUD_init_message(HM_DEFAULT, "%c%c%s:%c%c %s", CC_COLOR, BM_XRGB(28, 0, 0), static_cast<const char *>(PlayerCfg.GuidebotName), CC_COLOR, BM_XRGB(0, 31, 0), str);
}

__attribute_format_printf(1, 0)
static void buddy_message_force_va(const char *const fmt, va_list vl)
{
	char buf[128];
	vsnprintf(buf, sizeof(buf), fmt, vl);
	buddy_message_force_str(buf);
}

__attribute_format_printf(1, 2)
static void buddy_message_ignore_time(const char *const fmt, ...)
{
	auto &BuddyState = LevelUniqueObjectState.BuddyState;
	if (BuddyState.Buddy_messages_suppressed)
		return;
	if (!ok_for_buddy_to_talk())
		return;
	va_list	args;
	va_start(args, fmt);
	buddy_message_force_va(fmt, args);
	va_end(args);
}

}

void (buddy_message)(const char * format, ... )
{
	if (!show_buddy_message())
		return;

			va_list	args;

			va_start(args, format );
	buddy_message_force_va(format, args);
			va_end(args);
}

void buddy_message_str(const char *str)
{
	if (!show_buddy_message())
		return;
	buddy_message_force_str(str);
}

namespace {

//	-----------------------------------------------------------------------------
static void thief_message_str(const char * str) __attribute_nonnull();
static void thief_message_str(const char * str)
{
	HUD_init_message(HM_DEFAULT, "%c%cTHIEF:%c%c %s", 1, BM_XRGB(28, 0, 0), 1, BM_XRGB(0, 31, 0), str);
}

static void thief_message(const char *) = delete;
__attribute_format_printf(1, 2)
static void thief_message(const char * format, ... )
{

	char	new_format[128];
	va_list	args;

	va_start(args, format );
	vsnprintf(new_format, sizeof(new_format), format, args);
	va_end(args);
	thief_message_str(new_format);
}

//	-----------------------------------------------------------------------------
//	Return true if marker #id has been placed.
static int marker_exists_in_mine(const game_marker_index id)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	for (auto &obj : vcobjptr)
	{
		if (obj.type == OBJ_MARKER)
			if (get_marker_id(obj) == id)
				return 1;
	}
	return 0;
}

}

//	-----------------------------------------------------------------------------
void set_escort_special_goal(d_unique_buddy_state &BuddyState, const int raw_special_key)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	int marker_key;

	BuddyState.Buddy_messages_suppressed = 0;

	if (!BuddyState.Buddy_allowed_to_talk) {
		if (!ok_for_buddy_to_talk()) {
			auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
			const auto &&o = find_escort(vmobjptridx, Robot_info);
			if (o == object_none)
				HUD_init_message_literal(HM_DEFAULT, "No Guide-Bot in mine.");
			else
				HUD_init_message(HM_DEFAULT, "%s has not been released.", static_cast<const char *>(PlayerCfg.GuidebotName));
			return;
		}
	}

	const int special_key = raw_special_key & (~KEY_SHIFTED);

	marker_key = special_key;
	
	if (BuddyState.Last_buddy_key == special_key)
	{
		auto &Looking_for_marker = BuddyState.Looking_for_marker;
		if (Looking_for_marker == game_marker_index::None && special_key != KEY_0)
		{
			const unsigned zero_based_marker_id = marker_key - KEY_1;
			const auto gmi = static_cast<game_marker_index>(zero_based_marker_id);
			if (marker_exists_in_mine(gmi))
				Looking_for_marker = gmi;
			else {
				buddy_message_ignore_time("Marker %i not placed.", zero_based_marker_id + 1);
				Looking_for_marker = game_marker_index::None;
			}
		} else {
			Looking_for_marker = game_marker_index::None;
		}
	}

	BuddyState.Last_buddy_key = special_key;

	if (special_key == KEY_0)
		BuddyState.Looking_for_marker = game_marker_index::None;
	else if (BuddyState.Looking_for_marker != game_marker_index::None)
	{
		BuddyState.Escort_special_goal = static_cast<escort_goal_t>(ESCORT_GOAL_MARKER1 + marker_key - KEY_1);
	} else {
		auto &Escort_special_goal = BuddyState.Escort_special_goal;
		switch (special_key) {
			case KEY_1:	Escort_special_goal = ESCORT_GOAL_ENERGY;			break;
			case KEY_2:	Escort_special_goal = ESCORT_GOAL_ENERGYCEN;		break;
			case KEY_3:	Escort_special_goal = ESCORT_GOAL_SHIELD;			break;
			case KEY_4:	Escort_special_goal = ESCORT_GOAL_POWERUP;		break;
			case KEY_5:	Escort_special_goal = ESCORT_GOAL_ROBOT;			break;
			case KEY_6:	Escort_special_goal = ESCORT_GOAL_HOSTAGE;		break;
			case KEY_7:	Escort_special_goal = ESCORT_GOAL_SCRAM;			break;
			case KEY_8:	Escort_special_goal = ESCORT_GOAL_PLAYER_SPEW;	break;
			case KEY_9:	Escort_special_goal = ESCORT_GOAL_EXIT;			break;
			case KEY_0:	Escort_special_goal = ESCORT_GOAL_UNSPECIFIED;								break;
			default:
				Int3();		//	Oops, called with illegal key value.
		}
	}

	BuddyState.Last_buddy_message_time = GameTime64 - 2*F1_0;	//	Allow next message to come through.

	say_escort_goal(BuddyState.Escort_special_goal);
	BuddyState.Escort_goal_object = ESCORT_GOAL_UNSPECIFIED;
	multi_send_escort_goal(BuddyState);
}

namespace {

//	-----------------------------------------------------------------------------
//	Return id of boss.
static robot_id get_boss_id(void)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	for (auto &obj : vcobjptr)
	{
		if (obj.type == OBJ_ROBOT)
		{
			const auto objp_id{get_robot_id(obj)};
			if (Robot_info[objp_id].boss_flag != boss_robot_id::None)
				return objp_id;
		}
	}
	return robot_id::None;
}

//	-----------------------------------------------------------------------------
//	Return object index if object of objtype, objid exists in mine, else return -1
//	"special" is used to find objects spewed by player which is hacked into flags field of powerup.
static icobjidx_t exists_in_mine_2(const unique_segment &segp, const std::optional<object_type_t> objtype, const std::optional<uint8_t> objid, const int special)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptridx = Objects.vcptridx;
	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	range_for (const auto curobjp, objects_in(segp, vcobjptridx, vcsegptr))
	{
			if (special == ESCORT_GOAL_PLAYER_SPEW && curobjp->type == OBJ_POWERUP)
			{
				if (curobjp->flags & OF_PLAYER_DROPPED)
					return curobjp;
			}

			if (!objtype)
				continue;
			const auto t = *objtype;
			if (curobjp->type == t) {
				//	Don't find escort robots if looking for robot!
				if (t == OBJ_ROBOT && (Robot_info[get_robot_id(curobjp)].companion))
					;
				else if (!objid)
				{
					/* If no object id is specified, then any object of the
					 * correct type is acceptable.
					 */
						return curobjp;
				}
				else if (curobjp->id == *objid)
					/* A specific object id is required, and the current object
					 * satisfies the requirement.
					 */
					return curobjp;
			}

			if (t == OBJ_POWERUP && objid && curobjp->type == OBJ_ROBOT)
				/* As a special case, if seeking a specific powerup and the
				 * current object is a robot that will drop that powerup, then
				 * consider the robot to be a match.
				 */
				if (curobjp->contains.count)
					if (curobjp->contains.type == contained_object_type::powerup)
						if (curobjp->contains.id.powerup == powerup_type_t{*objid})
							return curobjp;
	}
	return object_none;
}

//	-----------------------------------------------------------------------------
static std::pair<icsegidx_t, d_unique_buddy_state::Escort_goal_reachability> exists_fuelcen_in_mine(const object &Buddy_objp, const player_flags powerup_flags)
{
	const vcsegidx_t start_seg{Buddy_objp.segnum};
	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	std::array<segnum_t, MAX_SEGMENTS> bfs_list;
	auto &robptr = Robot_info[get_robot_id(Buddy_objp)];
	const auto length = create_bfs_list(Buddy_objp, robptr, start_seg, powerup_flags, bfs_list);
	{
		const auto &&predicate = [](const segnum_t &s) {
			return vcsegptr(s)->special == segment_special::fuelcen;
		};
		const auto &&rb = partial_const_range(bfs_list, length);
		const auto &&i{std::ranges::find_if(rb, predicate)};
		if (i != rb.end())
			return {*i, d_unique_buddy_state::Escort_goal_reachability::reachable};
	}
	{
		const std::ranges::subrange rh{vcsegptridx};
		const auto &&i{std::ranges::find(rh, segment_special::fuelcen, &shared_segment::special)};
		if (i != rh.end())
			return {*i, d_unique_buddy_state::Escort_goal_reachability::unreachable};
	}
	return {segment_none, d_unique_buddy_state::Escort_goal_reachability::unreachable};
}

//	Return nearest object of interest.
//	If special == ESCORT_GOAL_PLAYER_SPEW, then looking for any object spewed by player.
//	-1 means object does not exist in mine.
//	-2 means object does exist in mine, but buddy-bot can't reach it (eg, behind triggered wall)
static std::pair<icobjidx_t, d_unique_buddy_state::Escort_goal_reachability> exists_in_mine(const object &Buddy_objp, const vcsegidx_t start_seg, const std::optional<object_type_t> objtype, const std::optional<uint8_t> objid, const int special, const player_flags powerup_flags)
{
	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	std::array<segnum_t, MAX_SEGMENTS> bfs_list;
	auto &robptr = Robot_info[get_robot_id(Buddy_objp)];
	const auto length = create_bfs_list(Buddy_objp, robptr, start_seg, powerup_flags, bfs_list);

	range_for (const auto segnum, partial_const_range(bfs_list, length))
	{
		const auto &&objnum = exists_in_mine_2(vcsegptr(segnum), objtype, objid, special);
			if (objnum != object_none)
				return {objnum, d_unique_buddy_state::Escort_goal_reachability::reachable};
	}

	//	Couldn't find what we're looking for by looking at connectivity.
	//	See if it's in the mine.  It could be hidden behind a trigger or switch
	//	which the buddybot doesn't understand.
	for (auto &seg : vcsegptr)
		{
		const auto &&objnum{exists_in_mine_2(seg, objtype, objid, special)};
			if (objnum != object_none)
				return {objnum, d_unique_buddy_state::Escort_goal_reachability::unreachable};
		}
	return {object_none, d_unique_buddy_state::Escort_goal_reachability::unreachable};
}

//	-----------------------------------------------------------------------------
//	Return true if it happened, else return false.
static imsegidx_t find_exit_segment()
{
	//	---------- Find exit doors ----------
	range_for (const auto &&segp, vcsegptridx)
	{
		range_for (const auto j, segp->children)
			if (j == segment_exit)
				return segp;
	}
	return segment_none;
}

//	-----------------------------------------------------------------------------
static void say_escort_goal(const escort_goal_t goal_num)
{
	if (Player_dead_state != player_dead_state::no)
		return;

	const char *str;
	switch (goal_num) {
		default:
		case ESCORT_GOAL_UNSPECIFIED:
			return;
		case ESCORT_GOAL_BLUE_KEY:
			str = "Finding BLUE KEY";
			break;
		case ESCORT_GOAL_GOLD_KEY:
			str = "Finding YELLOW KEY";
			break;
		case ESCORT_GOAL_RED_KEY:
			str = "Finding RED KEY";
			break;
		case ESCORT_GOAL_CONTROLCEN:
			str = "Finding REACTOR";
			break;
		case ESCORT_GOAL_EXIT:
			str = "Finding EXIT";
			break;
		case ESCORT_GOAL_ENERGY:
			str = "Finding ENERGY";
			break;
		case ESCORT_GOAL_ENERGYCEN:
			str = "Finding ENERGY CENTER";
			break;
		case ESCORT_GOAL_SHIELD:
			str = "Finding a SHIELD";
			break;
		case ESCORT_GOAL_POWERUP:
			str = "Finding a POWERUP";
			break;
		case ESCORT_GOAL_ROBOT:
			str = "Finding a ROBOT";
			break;
		case ESCORT_GOAL_HOSTAGE:
			str = "Finding a HOSTAGE";
			break;
		case ESCORT_GOAL_SCRAM:
			str = "Staying away...";
			break;
		case ESCORT_GOAL_BOSS:
			str = "Finding BOSS robot";
			break;
		case ESCORT_GOAL_PLAYER_SPEW:
			str = "Finding your powerups";
			break;
		case ESCORT_GOAL_MARKER1:
		case ESCORT_GOAL_MARKER2:
		case ESCORT_GOAL_MARKER3:
		case ESCORT_GOAL_MARKER4:
		case ESCORT_GOAL_MARKER5:
		case ESCORT_GOAL_MARKER6:
		case ESCORT_GOAL_MARKER7:
		case ESCORT_GOAL_MARKER8:
		case ESCORT_GOAL_MARKER9:
			{
				const uint8_t zero_based_goal_num = goal_num - ESCORT_GOAL_MARKER1;
				buddy_message("Finding marker %i: '%.24s'", zero_based_goal_num + 1, &MarkerState.message[(game_marker_index{zero_based_goal_num})][0u]);
			}
			return;
	}
	buddy_message_str(str);
}

static void clear_escort_goals()
{
	auto &BuddyState = LevelUniqueObjectState.BuddyState;
	BuddyState.Looking_for_marker = game_marker_index::None;
	BuddyState.Escort_goal_object = ESCORT_GOAL_UNSPECIFIED;
	BuddyState.Escort_special_goal = ESCORT_GOAL_UNSPECIFIED;
}

static void escort_goal_does_not_exist(const escort_goal_t goal)
{
	buddy_message_ignore_time("No %s in mine.", Escort_goal_text[goal - 1]);
	clear_escort_goals();
}

static void escort_goal_unreachable(const escort_goal_t goal)
{
	buddy_message_ignore_time("Can't reach %s.", Escort_goal_text[goal - 1]);
	clear_escort_goals();
}

static void escort_go_to_goal(const vmobjptridx_t objp, const robot_info &robptr, ai_static *const aip, const segnum_t goal_seg)
{
	auto &BuddyState = LevelUniqueObjectState.BuddyState;
	auto &Objects = LevelUniqueObjectState.Objects;
	create_path_to_segment(objp, robptr, goal_seg, Max_escort_length, create_path_safety_flag::safe);	//	MK!: Last parm (safety_flag) used to be 1!!
	if (aip->path_length > 3)
		aip->path_length = polish_path(objp, &Point_segs[aip->hide_index], aip->path_length);
	if ((aip->path_length > 0) && (Point_segs[aip->hide_index + aip->path_length - 1].segnum != goal_seg)) {
		const unsigned goal_text_index = std::exchange(BuddyState.Escort_goal_object, ESCORT_GOAL_SCRAM) - 1;
		BuddyState.Looking_for_marker = game_marker_index::None;
		auto &plr = get_player_controlling_guidebot(BuddyState, Players);
		if (plr.objnum == object_none)
			return;
		auto &plrobj = *Objects.vcptr(plr.objnum);
		if (plrobj.type != OBJ_PLAYER)
			return;
		buddy_message_ignore_time("Cannot reach %s.", goal_text_index < Escort_goal_text.size() ? Escort_goal_text[goal_text_index] : "<unknown>");
		const auto goal_segment = plrobj.segnum;
		const fix dist_to_player = find_connected_distance(objp->pos, vmsegptridx(objp->segnum), plrobj.pos, vmsegptridx(goal_segment), 100, wall_is_doorway_mask::fly);
		if (dist_to_player > MIN_ESCORT_DISTANCE)
			create_path_to_segment(objp, robptr, Max_escort_length, create_path_safety_flag::safe, goal_segment);
		else {
			create_n_segment_path(objp, robptr, 8 + d_rand() * 8, segment_none);
			aip->path_length = polish_path(objp, &Point_segs[aip->hide_index], aip->path_length);
		}
	}
}

//	-----------------------------------------------------------------------------
static imsegidx_t escort_get_goal_segment(const object &buddy_obj, const object_type_t objtype, const std::optional<uint8_t> objid, const player_flags powerup_flags)
{
	auto &BuddyState = LevelUniqueObjectState.BuddyState;
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	const auto &&eim = exists_in_mine(buddy_obj, buddy_obj.segnum, objtype, objid, -1, powerup_flags);
	BuddyState.Escort_goal_objidx = eim.first;
	BuddyState.Escort_goal_reachable = eim.second;
	if (eim.second != d_unique_buddy_state::Escort_goal_reachability::unreachable)
		return vcobjptr(eim.first)->segnum;
	return segment_none;
}

static inline imsegidx_t escort_get_goal_segment(const object &buddy_obj, const powerup_type_t objid, const player_flags powerup_flags)
{
	return escort_get_goal_segment(buddy_obj, OBJ_POWERUP, underlying_value(objid), powerup_flags);
}

static void set_escort_goal_non_object(d_unique_buddy_state &BuddyState)
{
	BuddyState.Escort_goal_objidx = object_none;
	BuddyState.Escort_goal_reachable = d_unique_buddy_state::Escort_goal_reachability::unreachable;
}

static void escort_create_path_to_goal(const vmobjptridx_t objp, const robot_info &robptr, const player_info &player_info)
{
	auto &BuddyState = LevelUniqueObjectState.BuddyState;
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	segnum_t	goal_seg = segment_none;
	ai_static	*aip = &objp->ctype.ai_info;
	ai_local		*ailp = &objp->ctype.ai_info.ail;

	if (BuddyState.Escort_special_goal != ESCORT_GOAL_UNSPECIFIED)
		BuddyState.Escort_goal_object = BuddyState.Escort_special_goal;

	const auto powerup_flags = player_info.powerup_flags;
	const auto Escort_goal_object = BuddyState.Escort_goal_object;
	if (BuddyState.Looking_for_marker != game_marker_index::None)
	{
		goal_seg = escort_get_goal_segment(objp, OBJ_MARKER, Escort_goal_object - ESCORT_GOAL_MARKER1, powerup_flags);
	} else {
		switch (Escort_goal_object) {
			case ESCORT_GOAL_BLUE_KEY:
				goal_seg = escort_get_goal_segment(objp, powerup_type_t::POW_KEY_BLUE, powerup_flags);
				break;
			case ESCORT_GOAL_GOLD_KEY:
				goal_seg = escort_get_goal_segment(objp, powerup_type_t::POW_KEY_GOLD, powerup_flags);
				break;
			case ESCORT_GOAL_RED_KEY:
				goal_seg = escort_get_goal_segment(objp, powerup_type_t::POW_KEY_RED, powerup_flags);
				break;
			case ESCORT_GOAL_CONTROLCEN:
				goal_seg = escort_get_goal_segment(objp, OBJ_CNTRLCEN, std::nullopt, powerup_flags);
				break;
			case ESCORT_GOAL_EXIT:
				goal_seg = find_exit_segment();
				set_escort_goal_non_object(BuddyState);
				if (goal_seg == segment_none)
					escort_goal_does_not_exist(ESCORT_GOAL_EXIT);
				else if (goal_seg == segment_exit)
					escort_goal_unreachable(ESCORT_GOAL_EXIT);
				else
					escort_go_to_goal(objp, robptr, aip, goal_seg);
				return;
			case ESCORT_GOAL_ENERGY:
				goal_seg = escort_get_goal_segment(objp, powerup_type_t::POW_ENERGY, powerup_flags);
				break;
			case ESCORT_GOAL_ENERGYCEN:
				{
					const auto &&ef = exists_fuelcen_in_mine(objp, powerup_flags);
					set_escort_goal_non_object(BuddyState);
				if (ef.second != d_unique_buddy_state::Escort_goal_reachability::unreachable)
					escort_go_to_goal(objp, robptr, aip, ef.first);
				else if (ef.first == segment_none)
					escort_goal_does_not_exist(ESCORT_GOAL_ENERGYCEN);
				else
					escort_goal_unreachable(ESCORT_GOAL_ENERGYCEN);
				return;
				}
			case ESCORT_GOAL_SHIELD:
				goal_seg = escort_get_goal_segment(objp, powerup_type_t::POW_SHIELD_BOOST, powerup_flags);
				break;
			case ESCORT_GOAL_POWERUP:
				goal_seg = escort_get_goal_segment(objp, OBJ_POWERUP, std::nullopt, powerup_flags);
				break;
			case ESCORT_GOAL_ROBOT:
				goal_seg = escort_get_goal_segment(objp, OBJ_ROBOT, std::nullopt, powerup_flags);
				break;
			case ESCORT_GOAL_HOSTAGE:
				goal_seg = escort_get_goal_segment(objp, OBJ_HOSTAGE, std::nullopt, powerup_flags);
				break;
			case ESCORT_GOAL_PLAYER_SPEW:
				{
					const auto &&egi = exists_in_mine(objp, objp->segnum, std::nullopt, std::nullopt, ESCORT_GOAL_PLAYER_SPEW, powerup_flags);
					BuddyState.Escort_goal_objidx = egi.first;
					BuddyState.Escort_goal_reachable = egi.second;
					if (egi.second != d_unique_buddy_state::Escort_goal_reachability::unreachable)
					{
						auto &o = *vcobjptr(egi.first);
						goal_seg = o.segnum;
					}
				}
				break;
			case ESCORT_GOAL_BOSS: {
				const auto boss_id = get_boss_id();
				assert(boss_id != robot_id::None);
				goal_seg = escort_get_goal_segment(objp, OBJ_ROBOT, underlying_value(boss_id), powerup_flags);
				break;
			}
			default:
				Int3();	//	Oops, Illegal value in Escort_goal_object.
				con_printf(CON_URGENT, "BUG: buddy goal is %.8x, resetting to SCRAM", Escort_goal_object);
				BuddyState.Escort_goal_object = ESCORT_GOAL_SCRAM;
				[[fallthrough]];
			case ESCORT_GOAL_SCRAM:
				set_escort_goal_non_object(BuddyState);
				create_n_segment_path(objp, robptr, 16 + d_rand() * 16, segment_none);
				aip->path_length = polish_path(objp, &Point_segs[aip->hide_index], aip->path_length);
				ailp->mode = ai_mode::AIM_GOTO_OBJECT;
				say_escort_goal(Escort_goal_object);
				return;
		}
	}
	const auto Escort_goal_objidx = BuddyState.Escort_goal_objidx;
	if (BuddyState.Escort_goal_reachable == d_unique_buddy_state::Escort_goal_reachability::unreachable)
	{
		if (Escort_goal_objidx == object_none) {
			escort_goal_does_not_exist(Escort_goal_object);
		} else {
			escort_goal_unreachable(Escort_goal_object);
		}
	} else {
		escort_go_to_goal(objp, robptr, aip, goal_seg);
		ailp->mode = ai_mode::AIM_GOTO_OBJECT;
		say_escort_goal(Escort_goal_object);
	}
}

//	-----------------------------------------------------------------------------
//	Escort robot chooses goal object based on player's keys, location.
//	Returns goal object.
static escort_goal_t escort_set_goal_object(const object &Buddy_objp, const player_flags pl_flags)
{
	auto &Boss_teleport_segs = LevelSharedBossState.Teleport_segs;
	auto &BuddyState = LevelUniqueObjectState.BuddyState;
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;
	auto &Objects = LevelUniqueObjectState.Objects;
	if (BuddyState.Escort_special_goal != ESCORT_GOAL_UNSPECIFIED)
		return ESCORT_GOAL_UNSPECIFIED;

	auto &plr = get_player_controlling_guidebot(BuddyState, Players);
	if (plr.objnum == object_none)
		/* should never happen */
		return ESCORT_GOAL_UNSPECIFIED;
	auto &plrobj = *Objects.vcptr(plr.objnum);
	if (plrobj.type != OBJ_PLAYER)
		return ESCORT_GOAL_UNSPECIFIED;

	const auto need_key_and_key_exists = [&Buddy_objp, pl_flags, start_search_seg = plrobj.segnum](const PLAYER_FLAG flag_key, const powerup_type_t powerup_key) {
		if (pl_flags & flag_key)
			/* Player already has this key, so no need to get it again.
			 */
			return false;
		const auto &&e = exists_in_mine(Buddy_objp, start_search_seg, OBJ_POWERUP, underlying_value(powerup_key), -1, pl_flags);
		/* For compatibility with classic Descent 2, test only whether
		 * the key exists, but ignore whether it can be reached by the
		 * guide bot.
		 */
		return e.first != object_none;
	};
	if (need_key_and_key_exists(PLAYER_FLAGS_BLUE_KEY, powerup_type_t::POW_KEY_BLUE))
		return ESCORT_GOAL_BLUE_KEY;
	else if (need_key_and_key_exists(PLAYER_FLAGS_GOLD_KEY, powerup_type_t::POW_KEY_GOLD))
		return ESCORT_GOAL_GOLD_KEY;
	else if (need_key_and_key_exists(PLAYER_FLAGS_RED_KEY, powerup_type_t::POW_KEY_RED))
		return ESCORT_GOAL_RED_KEY;
	else if (LevelUniqueControlCenterState.Control_center_destroyed == 0)
	{
		if (!Boss_teleport_segs.empty())
			return ESCORT_GOAL_BOSS;
		else
			return ESCORT_GOAL_CONTROLCEN;
	} else
		return ESCORT_GOAL_EXIT;
}

#define	MAX_ESCORT_TIME_AWAY		(F1_0*4)

//	-----------------------------------------------------------------------------
static const player *time_to_visit_player(const d_level_unique_object_state &LevelUniqueObjectState, const object &buddy_object)
{
	auto &BuddyState = LevelUniqueObjectState.BuddyState;
	auto &plr = get_player_controlling_guidebot(BuddyState, Players);
	if (plr.objnum == object_none)
		/* should never happen */
		return nullptr;
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &plrobj = *Objects.vcptr(plr.objnum);
	if (plrobj.type != OBJ_PLAYER)
		return nullptr;
	//	Note: This one has highest priority because, even if already going towards player,
	//	might be necessary to create a new path, as player can move.
	if (GameTime64 - BuddyState.Buddy_last_seen_player > MAX_ESCORT_TIME_AWAY)
		if (GameTime64 - BuddyState.Buddy_last_player_path_created > F1_0)
			return &plr;

	auto &ais = buddy_object.ctype.ai_info;
	if (ais.ail.mode == ai_mode::AIM_GOTO_PLAYER)
		return nullptr;

	if (ais.cur_path_index < ais.path_length / 2)
		return nullptr;

	if (buddy_object.segnum == plrobj.segnum)
		return nullptr;
	return &plr;
}

//	-----------------------------------------------------------------------------
static void bash_buddy_weapon_info(d_unique_buddy_state &BuddyState, fvmobjptridx &vmobjptridx, object &weapon_obj)
{
	auto &plr = get_player_controlling_guidebot(BuddyState, Players);
	if (plr.objnum == object_none)
		/* should never happen */
		return;
	/* Buddy can still fire while player is dead, so skip check for
	 * plrobj.type */
	auto &&plrobj = vmobjptridx(plr.objnum);
	auto &laser_info = weapon_obj.ctype.laser_info;
	laser_info.parent_num = plrobj;
	laser_info.parent_type = OBJ_PLAYER;
	laser_info.parent_signature = plrobj->signature;
}

//	-----------------------------------------------------------------------------
static int maybe_buddy_fire_mega(const vmobjptridx_t objp, const vcobjptridx_t Buddy_objp)
{
	auto &BuddyState = LevelUniqueObjectState.BuddyState;
	auto &Objects = LevelUniqueObjectState.Objects;
	const auto &&[dist, vec_to_robot] = vm_vec_normalize_quick_with_magnitude(vm_vec_sub(Buddy_objp->pos, objp->pos));

	if (dist > F1_0*100)
		return 0;

	const auto dot = vm_vec_dot(vec_to_robot, Buddy_objp->orient.fvec);

	if (dot < F1_0/2)
		return 0;

	if (!object_to_object_visibility(Buddy_objp, objp, FQ_TRANSWALL))
		return 0;

	if (Weapon_info[weapon_id_type::MEGA_ID].render == weapon_info::render_type::laser) {
		con_puts(CON_VERBOSE, "Buddy can't fire mega (shareware)");
		buddy_message_str("CLICK!");
		return 0;
	}

	buddy_message_str("GAHOOGA!");

	auto &buddy = *Buddy_objp;
	const imobjptridx_t weapon_objnum = Laser_create_new_easy(LevelSharedRobotInfoState.Robot_info, buddy.orient.fvec, buddy.pos, objp, weapon_id_type::MEGA_ID, weapon_sound_flag::audible);

	if (weapon_objnum != object_none)
		bash_buddy_weapon_info(BuddyState, Objects.vmptridx, weapon_objnum);

	return 1;
}

//-----------------------------------------------------------------------------
static int maybe_buddy_fire_smart(const vmobjptridx_t objp, const vcobjptridx_t buddy_objp)
{
	auto &BuddyState = LevelUniqueObjectState.BuddyState;
	auto &Objects = LevelUniqueObjectState.Objects;
	fix		dist;

	dist = vm_vec_dist_quick(buddy_objp->pos, objp->pos);

	if (dist > F1_0*80)
		return 0;

	if (!object_to_object_visibility(buddy_objp, objp, FQ_TRANSWALL))
		return 0;

	buddy_message_str("WHAMMO!");

	const imobjptridx_t weapon_objnum = Laser_create_new_easy(LevelSharedRobotInfoState.Robot_info, buddy_objp->orient.fvec, buddy_objp->pos, objp, weapon_id_type::SMART_ID, weapon_sound_flag::audible);

	if (weapon_objnum != object_none)
		bash_buddy_weapon_info(BuddyState, Objects.vmptridx, weapon_objnum);

	return 1;
}

//	-----------------------------------------------------------------------------
static void do_buddy_dude_stuff(const vmobjptridx_t buddy_objp)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &BuddyState = LevelUniqueObjectState.BuddyState;
	auto &vmobjptridx = Objects.vmptridx;
	if (!ok_for_buddy_to_talk())
		return;

	if (BuddyState.Buddy_last_missile_time + F1_0*2 < GameTime64) {
		//	See if a robot potentially in view cone
		auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
		const std::ranges::subrange rh{vmobjptridx};
		range_for (const auto &&objp, rh)
		{
			if ((objp->type == OBJ_ROBOT) && !Robot_info[get_robot_id(objp)].companion)
				if (maybe_buddy_fire_mega(objp, buddy_objp)) {
					BuddyState.Buddy_last_missile_time = {GameTime64};
					return;
				}
		}

		//	See if a robot near enough that buddy should fire smart missile
		range_for (const auto &&objp, rh)
		{
			if ((objp->type == OBJ_ROBOT) && !Robot_info[get_robot_id(objp)].companion)
				if (maybe_buddy_fire_smart(objp, buddy_objp)) {
					BuddyState.Buddy_last_missile_time = {GameTime64};
					return;
				}
		}
	}
}

static void escort_set_goal_toward_controlling_player(d_unique_buddy_state &BuddyState, fvcobjptr &vcobjptr, const vmobjptridx_t buddy_obj, const robot_info &robptr)
{
	auto &aip = buddy_obj->ctype.ai_info;
	aip.path_length = polish_path(buddy_obj, &Point_segs[aip.hide_index], aip.path_length);
	if (aip.path_length < 3) {
		auto &plr = get_player_controlling_guidebot(BuddyState, Players);
		if (plr.objnum != object_none)
		{
			auto &plrobj = *vcobjptr(plr.objnum);
			if (plrobj.type == OBJ_PLAYER)
				create_n_segment_path(buddy_obj, robptr, 5, plrobj.segnum);
		}
	}
	aip.ail.mode = ai_mode::AIM_GOTO_OBJECT;
}

}

//	-----------------------------------------------------------------------------
//	Called every frame (or something).
void do_escort_frame(const vmobjptridx_t objp, const robot_info &robptr, const object &plrobj, const player_visibility_state player_visibility)
{
	auto &BuddyState = LevelUniqueObjectState.BuddyState;
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	const auto dist_to_player = vm_vec_dist_quick(plrobj.pos, objp->pos);
	ai_static	*aip = &objp->ctype.ai_info;
	ai_local		*ailp = &objp->ctype.ai_info.ail;

	auto &player_info = plrobj.ctype.player_info;
	if (player_is_visible(player_visibility))
	{
		BuddyState.Buddy_last_seen_player = {GameTime64};
		if (player_info.powerup_flags & PLAYER_FLAGS_HEADLIGHT_ON)	//	DAMN! MK, stupid bug, fixed 12/08/95, changed PLAYER_FLAGS_HEADLIGHT to PLAYER_FLAGS_HEADLIGHT_ON
		{
			const auto energy = player_info.energy;
			const auto ienergy = f2i(energy);
			if (ienergy < 40)
				if (ienergy & 4)
						buddy_message_str("Hey, your headlight's on!");
		}
	}

	if (cheats.buddyangry)
		do_buddy_dude_stuff(objp);

	{
		const auto buddy_sorry_time = BuddyState.Buddy_sorry_time;
		if (buddy_sorry_time + F1_0 > GameTime64)
		{
			BuddyState.Buddy_sorry_time = -F1_0*2;
			if (buddy_sorry_time < GameTime64 + F1_0*2)
			{
				buddy_message_ignore_time("Oops, sorry 'bout that...");
			}
		}
	}

	//	If buddy not allowed to talk, then he is locked in his room.  Make him mostly do nothing unless you're nearby.
	if (!BuddyState.Buddy_allowed_to_talk)
		if (dist_to_player > F1_0*100)
			aip->SKIP_AI_COUNT = (F1_0/4)/FrameTime;

	//	ai_mode::AIM_WANDER has been co-opted for buddy behavior (didn't want to modify aistruct.h)
	//	It means the object has been told to get lost and has come to the end of its path.
	//	If the player is now visible, then create a path.
	if (ailp->mode == ai_mode::AIM_WANDER)
		if (player_is_visible(player_visibility))
		{
			create_n_segment_path(objp, robptr, 16 + d_rand() * 16, segment_none);
			aip->path_length = polish_path(objp, &Point_segs[aip->hide_index], aip->path_length);
		}

	if (BuddyState.Escort_special_goal == ESCORT_GOAL_SCRAM) {
		auto &plr = get_player_controlling_guidebot(BuddyState, Players);
		if (plr.objnum == object_none)
			return;
		auto &plrobj = *Objects.vcptr(plr.objnum);
		if (plrobj.type != OBJ_PLAYER)
			return;
		if (player_is_visible(player_visibility))
			if (BuddyState.Escort_last_path_created + F1_0*3 < GameTime64) {
				BuddyState.Escort_last_path_created = {GameTime64};
				create_n_segment_path(objp, robptr, 10 + d_rand() * 16, plrobj.segnum);
			}

		return;
	}

	//	Force checking for new goal every 5 seconds, and create new path, if necessary.
	if (BuddyState.Escort_last_path_created + (BuddyState.Escort_special_goal != ESCORT_GOAL_SCRAM ? (F1_0 * 5) : (F1_0 * 15)) < GameTime64)
	{
		BuddyState.Escort_goal_object = ESCORT_GOAL_UNSPECIFIED;
		BuddyState.Escort_last_path_created = {GameTime64};
	}

	const player *guidebot_controller_player;
	if (BuddyState.Escort_special_goal != ESCORT_GOAL_SCRAM && (guidebot_controller_player = time_to_visit_player(LevelUniqueObjectState, objp)))
	{
		unsigned max_len;
		BuddyState.Buddy_last_player_path_created = {GameTime64};
		ailp->mode = ai_mode::AIM_GOTO_PLAYER;
		if (!player_is_visible(player_visibility))
		{
			if (BuddyState.Last_come_back_message_time + F1_0 < GameTime64)
			{
				BuddyState.Last_come_back_message_time = {GameTime64};
				auto &local_player = *Players.vcptr(Player_num);
				if (guidebot_controller_player == &local_player)
					buddy_message_str("Coming back to get you.");
				else
					buddy_message("Going back to get %s.", guidebot_controller_player->callsign.operator const char *());
			}
		}
		//	No point in Buddy creating very long path if he's not allowed to talk.  Really kills framerate.
		max_len = Max_escort_length;
		if (!BuddyState.Buddy_allowed_to_talk)
			max_len = 3;
		create_path_to_segment(objp, robptr, max_len, create_path_safety_flag::safe, Believed_player_seg);	//	MK!: Last parm used to be 1!
		aip->path_length = polish_path(objp, &Point_segs[aip->hide_index], aip->path_length);
		ailp->mode = ai_mode::AIM_GOTO_PLAYER;
	}
	else if (GameTime64 - BuddyState.Buddy_last_seen_player > MAX_ESCORT_TIME_AWAY)
	{
		//	This is to prevent buddy from looking for a goal, which he will do because we only allow path creation once/second.
		return;
	} else if ((ailp->mode == ai_mode::AIM_GOTO_PLAYER) && (dist_to_player < MIN_ESCORT_DISTANCE)) {
		BuddyState.Escort_goal_object = escort_set_goal_object(objp, player_info.powerup_flags);
		ailp->mode = ai_mode::AIM_GOTO_OBJECT;		//	May look stupid to be before path creation, but ai_door_is_openable uses mode to determine what doors can be got through
		escort_create_path_to_goal(objp, robptr, player_info);
		escort_set_goal_toward_controlling_player(BuddyState, vcobjptr, objp, robptr);
	}
	else if (BuddyState.Escort_goal_object == ESCORT_GOAL_UNSPECIFIED)
	{
		if ((ailp->mode != ai_mode::AIM_GOTO_PLAYER) || (dist_to_player < MIN_ESCORT_DISTANCE)) {
			BuddyState.Escort_goal_object = escort_set_goal_object(objp, player_info.powerup_flags);
			ailp->mode = ai_mode::AIM_GOTO_OBJECT;		//	May look stupid to be before path creation, but ai_door_is_openable uses mode to determine what doors can be got through
			escort_create_path_to_goal(objp, robptr, player_info);
			escort_set_goal_toward_controlling_player(BuddyState, vcobjptr, objp, robptr);
		}
	}
}

void invalidate_escort_goal(d_unique_buddy_state &BuddyState)
{
	BuddyState.Escort_goal_object = ESCORT_GOAL_UNSPECIFIED;
}

//	-------------------------------------------------------------------------------------------------
void do_snipe_frame(const vmobjptridx_t objp, const robot_info &robptr, const fix dist_to_player, const player_visibility_state player_visibility, const vms_vector &vec_to_player)
{
	ai_local		*ailp = &objp->ctype.ai_info.ail;
	fix			connected_distance;

	if (dist_to_player > F1_0*500)
		return;

	switch (ailp->mode) {
		case ai_mode::AIM_SNIPE_WAIT:
			if ((dist_to_player > F1_0*50) && (ailp->next_action_time > 0))
				return;

			ailp->next_action_time = SNIPE_WAIT_TIME;

			connected_distance = find_connected_distance(objp->pos, vmsegptridx(objp->segnum), Believed_player_pos, vmsegptridx(Believed_player_seg), 30, wall_is_doorway_mask::fly);
			if (connected_distance < F1_0*500) {
				create_path_to_believed_player_segment(objp, robptr, 30, create_path_safety_flag::safe);
				ailp->mode = ai_mode::AIM_SNIPE_ATTACK;
				ailp->next_action_time = SNIPE_ATTACK_TIME;	//	have up to 10 seconds to find player.
			}
			break;

		case ai_mode::AIM_SNIPE_RETREAT:
		case ai_mode::AIM_SNIPE_RETREAT_BACKWARDS:
			if (ailp->next_action_time < 0) {
				ailp->mode = ai_mode::AIM_SNIPE_WAIT;
				ailp->next_action_time = SNIPE_WAIT_TIME;
			}
			else if (player_visibility == player_visibility_state::no_line_of_sight || ailp->next_action_time > SNIPE_ABORT_RETREAT_TIME)
			{
				ai_follow_path(LevelSharedRobotInfoState.Robot_info, objp, player_visibility, &vec_to_player);
				ailp->mode = ai_mode::AIM_SNIPE_RETREAT_BACKWARDS;
			} else {
				ailp->mode = ai_mode::AIM_SNIPE_FIRE;
				ailp->next_action_time = SNIPE_FIRE_TIME/2;
			}
			break;

		case ai_mode::AIM_SNIPE_ATTACK:
			if (ailp->next_action_time < 0) {
				ailp->mode = ai_mode::AIM_SNIPE_RETREAT;
				ailp->next_action_time = SNIPE_WAIT_TIME;
			} else {
				ai_follow_path(LevelSharedRobotInfoState.Robot_info, objp, player_visibility, &vec_to_player);
				if (player_is_visible(player_visibility))
				{
					ailp->mode = ai_mode::AIM_SNIPE_FIRE;
					ailp->next_action_time = SNIPE_FIRE_TIME;
				} else
					ailp->mode = ai_mode::AIM_SNIPE_ATTACK;
			}
			break;

		case ai_mode::AIM_SNIPE_FIRE:
			if (ailp->next_action_time < 0) {
				ai_static	*aip = &objp->ctype.ai_info;
				create_n_segment_path(objp, robptr, 10 + d_rand()/2048, ConsoleObject->segnum);
				aip->path_length = polish_path(objp, &Point_segs[aip->hide_index], aip->path_length);
				if (d_rand() < 8192)
					ailp->mode = ai_mode::AIM_SNIPE_RETREAT_BACKWARDS;
				else
					ailp->mode = ai_mode::AIM_SNIPE_RETREAT;
				ailp->next_action_time = SNIPE_RETREAT_TIME;
			} else {
			}
			break;

		default:
			Int3();	//	Oops, illegal mode for snipe behavior.
			ailp->mode = ai_mode::AIM_SNIPE_ATTACK;
			ailp->next_action_time = F1_0;
			break;
	}

}

static fix64	Re_init_thief_time = 0x3f000000;

//	----------------------------------------------------------------------
void recreate_thief(const d_robot_info_array &Robot_info, const robot_id thief_id)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	const auto segnum = choose_thief_recreation_segment(vcsegptr, LevelUniqueWallSubsystemState.Walls.vcptr, ConsoleObject->segnum);
	const auto &&segp = vmsegptridx(segnum);
	auto &vcvertptr = Vertices.vcptr;
	const auto center_point{compute_segment_center(vcvertptr, segp)};

	const auto &&new_obj = create_morph_robot(Robot_info, segp, center_point, thief_id);
	if (new_obj == object_none)
		return;
	Re_init_thief_time = GameTime64 + F1_0*10;		//	In 10 seconds, re-initialize thief.
}

//	----------------------------------------------------------------------------
#define	THIEF_ATTACK_TIME		(F1_0*10)

constexpr enumerated_array<fix, NDL, Difficulty_level_type> Thief_wait_times = {{{
	F1_0*30, F1_0*25, F1_0*20, F1_0*15, F1_0*10
}}};

//	-------------------------------------------------------------------------------------------------
void do_thief_frame(const vmobjptridx_t objp, const robot_info &robptr, const fix dist_to_player, const player_visibility_state player_visibility, const vms_vector &vec_to_player)
{
	const auto Difficulty_level = GameUniqueState.Difficulty_level;
	ai_local		*ailp = &objp->ctype.ai_info.ail;
	fix			connected_distance;

	if ((Current_level_num < 0) && (Re_init_thief_time < GameTime64)) {
		if (Re_init_thief_time > GameTime64 - F1_0*2)
			init_thief_for_level();
		Re_init_thief_time = 0x3f000000;
	}

	if ((dist_to_player > F1_0*500) && (ailp->next_action_time > 0))
		return;

	if (Player_dead_state != player_dead_state::no)
		ailp->mode = ai_mode::AIM_THIEF_RETREAT;

	switch (ailp->mode) {
		case ai_mode::AIM_THIEF_WAIT:
			if (ailp->player_awareness_type >= player_awareness_type_t::PA_PLAYER_COLLISION) {
				ailp->player_awareness_type = player_awareness_type_t::PA_NONE;
				create_path_to_believed_player_segment(objp, robptr, 30, create_path_safety_flag::safe);
				ailp->mode = ai_mode::AIM_THIEF_ATTACK;
				ailp->next_action_time = THIEF_ATTACK_TIME/2;
				return;
			}
			else if (player_is_visible(player_visibility))
			{
				create_n_segment_path(objp, robptr, 15, ConsoleObject->segnum);
				ailp->mode = ai_mode::AIM_THIEF_RETREAT;
				return;
			}

			if ((dist_to_player > F1_0*50) && (ailp->next_action_time > 0))
				return;

			ailp->next_action_time = Thief_wait_times[Difficulty_level]/2;

			connected_distance = find_connected_distance(objp->pos, vmsegptridx(objp->segnum), Believed_player_pos, vmsegptridx(Believed_player_seg), 30, wall_is_doorway_mask::fly);
			if (connected_distance < F1_0*500) {
				create_path_to_believed_player_segment(objp, robptr, 30, create_path_safety_flag::safe);
				ailp->mode = ai_mode::AIM_THIEF_ATTACK;
				ailp->next_action_time = THIEF_ATTACK_TIME;	//	have up to 10 seconds to find player.
			}

			break;

		case ai_mode::AIM_THIEF_RETREAT:
			if (ailp->next_action_time < 0) {
				ailp->mode = ai_mode::AIM_THIEF_WAIT;
				ailp->next_action_time = Thief_wait_times[Difficulty_level];
			}
			else if (dist_to_player < F1_0 * 100 || player_is_visible(player_visibility) || ailp->player_awareness_type >= player_awareness_type_t::PA_PLAYER_COLLISION)
			{
				ai_follow_path(LevelSharedRobotInfoState.Robot_info, objp, player_visibility, &vec_to_player);
				if ((dist_to_player < F1_0*100) || (ailp->player_awareness_type >= player_awareness_type_t::PA_PLAYER_COLLISION)) {
					ai_static	*aip = &objp->ctype.ai_info;
					if (((aip->cur_path_index <=1) && (aip->PATH_DIR == -1)) || ((aip->cur_path_index >= aip->path_length-1) && (aip->PATH_DIR == 1))) {
						ailp->player_awareness_type = player_awareness_type_t::PA_NONE;
						create_n_segment_path(objp, robptr, 10, ConsoleObject->segnum);

						//	If path is real short, try again, allowing to go through player's segment
						if (aip->path_length < 4) {
							create_n_segment_path(objp, robptr, 10, segment_none);
						} else if (objp->shields* 4 < robptr.strength) {
							//	If robot really low on hits, will run through player with even longer path
							if (aip->path_length < 8) {
								create_n_segment_path(objp, robptr, 10, segment_none);
							}
						}

						ailp->mode = ai_mode::AIM_THIEF_RETREAT;
					}
				} else
					ailp->mode = ai_mode::AIM_THIEF_RETREAT;

			}

			break;

		//	This means the thief goes from wherever he is to the player.
		//	Note: When thief successfully steals something, his action time is forced negative and his mode is changed
		//			to retreat to get him out of attack mode.
		case ai_mode::AIM_THIEF_ATTACK:
			if (ailp->player_awareness_type >= player_awareness_type_t::PA_PLAYER_COLLISION) {
				ailp->player_awareness_type = player_awareness_type_t::PA_NONE;
				if (d_rand() > 8192) {
					create_n_segment_path(objp, robptr, 10, ConsoleObject->segnum);
					ailp->next_action_time = Thief_wait_times[Difficulty_level]/2;
					ailp->mode = ai_mode::AIM_THIEF_RETREAT;
				}
			} else if (ailp->next_action_time < 0) {
				//	This forces him to create a new path every second.
				ailp->next_action_time = F1_0;
				create_path_to_believed_player_segment(objp, robptr, 100, create_path_safety_flag::unsafe);
				ailp->mode = ai_mode::AIM_THIEF_ATTACK;
			} else {
				if (player_is_visible(player_visibility) && dist_to_player < F1_0*100)
				{
					//	If the player is close to looking at the thief, thief shall run away.
					//	No more stupid thief trying to sneak up on you when you're looking right at him!
					if (dist_to_player > F1_0*60) {
						fix	dot = vm_vec_dot(vec_to_player, ConsoleObject->orient.fvec);
						if (dot < -F1_0/2) {	//	Looking at least towards thief, so thief will run!
							create_n_segment_path(objp, robptr, 10, ConsoleObject->segnum);
							ailp->next_action_time = Thief_wait_times[Difficulty_level]/2;
							ailp->mode = ai_mode::AIM_THIEF_RETREAT;
						}
					} 
					ai_turn_towards_vector(vec_to_player, objp, F1_0/4);
					move_towards_player(LevelSharedRobotInfoState.Robot_info, objp, vec_to_player);
				} else {
					ai_static	*aip = &objp->ctype.ai_info;
					//	If path length == 0, then he will keep trying to create path, but he is probably stuck in his closet.
					if ((aip->path_length > 1) || ((d_tick_count & 0x0f) == 0)) {
						ai_follow_path(LevelSharedRobotInfoState.Robot_info, objp, player_visibility, &vec_to_player);
						ailp->mode = ai_mode::AIM_THIEF_ATTACK;
					}
				}
			}
			break;

		default:
			ailp->mode = ai_mode::AIM_THIEF_ATTACK;
			ailp->next_action_time = F1_0;
			break;
	}

}

namespace {

//	----------------------------------------------------------------------------
//	Return true if this item (whose presence is indicated by Players[player_num].flags) gets stolen.
static int maybe_steal_flag_item(object &playerobj, const PLAYER_FLAG flagval)
{
	auto &ThiefUniqueState = LevelUniqueObjectState.ThiefState;
	auto &plr_flags = playerobj.ctype.player_info.powerup_flags;
	if (plr_flags & flagval)
	{
		if (d_rand() < THIEF_PROBABILITY) {
			powerup_type_t powerup_index;
			const char *msg;
			plr_flags &= (~flagval);
			switch (flagval) {
				case PLAYER_FLAGS_INVULNERABLE:
					powerup_index = powerup_type_t::POW_INVULNERABILITY;
					msg = "Invulnerability stolen!";
					break;
				case PLAYER_FLAGS_CLOAKED:
					powerup_index = powerup_type_t::POW_CLOAK;
					msg = "Cloak stolen!";
					break;
				case PLAYER_FLAGS_MAP_ALL:
					powerup_index = powerup_type_t::POW_FULL_MAP;
					msg = "Full map stolen!";
					break;
				case PLAYER_FLAGS_QUAD_LASERS:
					update_laser_weapon_info();
					powerup_index = powerup_type_t::POW_QUAD_FIRE;
					msg = "Quad lasers stolen!";
					break;
				case PLAYER_FLAGS_AFTERBURNER:
					powerup_index = powerup_type_t::POW_AFTERBURNER;
					msg = "Afterburner stolen!";
					break;
				case PLAYER_FLAGS_CONVERTER:
					powerup_index = powerup_type_t::POW_CONVERTER;
					msg = "Converter stolen!";
					break;
				case PLAYER_FLAG::HEADLIGHT_PRESENT_AND_ON:
					powerup_index = powerup_type_t::POW_HEADLIGHT;
					msg = "Headlight stolen!";
					break;
				default:
					assert(false);
					return 0;
			}
			ThiefUniqueState.Stolen_items[ThiefUniqueState.Stolen_item_index] = powerup_index;
			thief_message_str(msg);
			return 1;
		}
	}

	return 0;
}

//	----------------------------------------------------------------------------
static int maybe_steal_secondary_weapon(object &playerobj, const secondary_weapon_index_t weapon_num)
{
	auto &ThiefUniqueState = LevelUniqueObjectState.ThiefState;
	auto &player_info = playerobj.ctype.player_info;
	if (auto &secondary_ammo = player_info.secondary_ammo[weapon_num])
		if (d_rand() < THIEF_PROBABILITY) {
			if (weapon_index_is_player_bomb(weapon_num))
			{
				if (d_rand() > 8192)		//	Come in groups of 4, only add 1/4 of time.
					return 0;
			}
			//	Smart mines and proxbombs don't get dropped because they only come in 4 packs.
			else
				ThiefUniqueState.Stolen_items[ThiefUniqueState.Stolen_item_index] = Secondary_weapon_to_powerup[weapon_num];
			thief_message("%s stolen!", SECONDARY_WEAPON_NAMES(weapon_num));		//	Danger! Danger! Use of literal!  Danger!
			if (-- secondary_ammo == 0)
				auto_select_secondary_weapon(player_info);

			// -- compress_stolen_items();
			return 1;
		}

	return 0;
}

//	----------------------------------------------------------------------------
static int maybe_steal_primary_weapon(object &playerobj, const primary_weapon_index_t weapon_num)
{
	auto &ThiefUniqueState = LevelUniqueObjectState.ThiefState;
	auto &player_info = playerobj.ctype.player_info;
	bool is_energy_weapon = true;
	switch (static_cast<primary_weapon_index_t>(weapon_num))
	{
		case primary_weapon_index_t::LASER_INDEX:
			if (player_info.laser_level == laser_level::_1)
				return 0;
			break;
		case primary_weapon_index_t::VULCAN_INDEX:
		case primary_weapon_index_t::GAUSS_INDEX:
			if (!player_info.vulcan_ammo)
				return 0;
			is_energy_weapon = false;
			[[fallthrough]];
		default:
			if (!(player_info.primary_weapon_flags & HAS_PRIMARY_FLAG(weapon_num)))
				return 0;
			break;
	}
	if (is_energy_weapon)
	{
		if (((Game_mode & GM_MULTI)
			? Netgame.ThiefModifierFlags
			: PlayerCfg.ThiefModifierFlags) & ThiefModifier::NoEnergyWeapons)
			return 0;
	}
	{
		if (d_rand() < THIEF_PROBABILITY) {
			powerup_type_t primary_weapon_powerup;
			if (weapon_num == primary_weapon_index_t::LASER_INDEX)
			{
				auto &laser_level = player_info.laser_level;
				primary_weapon_powerup = (laser_level > MAX_LASER_LEVEL)
					? powerup_type_t::POW_SUPER_LASER
					: Primary_weapon_to_powerup[weapon_num];
					/* Laser levels are zero-based, so print the old
					 * level, then decrement it.  Decrementing first
					 * would produce confusing output, particularly when
					 * the user loses the final laser powerup and drops
					 * to level 0 lasers.
					 */
					const unsigned l = static_cast<unsigned>(laser_level);
					-- laser_level;
					thief_message("%s level decreased to %u!", PRIMARY_WEAPON_NAMES(weapon_num), l);
			}
			else
			{
				player_info.primary_weapon_flags &= ~HAS_PRIMARY_FLAG(weapon_num);
				primary_weapon_powerup = Primary_weapon_to_powerup[weapon_num];

				thief_message("%s stolen!", PRIMARY_WEAPON_NAMES(weapon_num));		//	Danger! Danger! Use of literal!  Danger!
				auto_select_primary_weapon(player_info);
			}
			ThiefUniqueState.Stolen_items[ThiefUniqueState.Stolen_item_index] = primary_weapon_powerup;
			return 1;
		}
	}

	return 0;
}

//	----------------------------------------------------------------------------
//	Called for a thief-type robot.
//	If a item successfully stolen, returns true, else returns false.
//	If a wapon successfully stolen, do everything, removing it from player,
//	updating Stolen_items information, deselecting, etc.
static int attempt_to_steal_item_3(object &thief, object &player_num)
{
	(void)thief;
	//	First, try to steal equipped items.

	if (auto r = maybe_steal_flag_item(player_num, PLAYER_FLAGS_INVULNERABLE))
		return r;

	//	If primary weapon = laser, first try to rip away those nasty quad lasers!
	const auto Primary_weapon = player_num.ctype.player_info.Primary_weapon;
	if (Primary_weapon == primary_weapon_index_t::LASER_INDEX)
		if (auto r = maybe_steal_flag_item(player_num, PLAYER_FLAGS_QUAD_LASERS))
			return r;

	//	Makes it more likely to steal primary than secondary.
	range_for (const int i, xrange(2u))
	{
		(void)i;
		if (auto r = maybe_steal_primary_weapon(player_num, Primary_weapon))
			return r;
	}

	if (auto r = maybe_steal_secondary_weapon(player_num, player_num.ctype.player_info.Secondary_weapon))
		return r;

	//	See what the player has and try to snag something.
	//	Try best things first.
	if (auto r = maybe_steal_flag_item(player_num, PLAYER_FLAGS_INVULNERABLE))
		return r;
	if (auto r = maybe_steal_flag_item(player_num, PLAYER_FLAGS_CLOAKED))
		return r;
	if (auto r = maybe_steal_flag_item(player_num, PLAYER_FLAGS_QUAD_LASERS))
		return r;
	if (auto r = maybe_steal_flag_item(player_num, PLAYER_FLAGS_AFTERBURNER))
		return r;
	if (auto r = maybe_steal_flag_item(player_num, PLAYER_FLAGS_CONVERTER))
		return r;
// --	if (maybe_steal_flag_item(player_num, PLAYER_FLAGS_AMMO_RACK))	//	Can't steal because what if have too many items, say 15 homing missiles?
// --		return 1;
	if (auto r = maybe_steal_flag_item(player_num, PLAYER_FLAG::HEADLIGHT_PRESENT_AND_ON))
		return r;
	if (auto r = maybe_steal_flag_item(player_num, PLAYER_FLAGS_MAP_ALL))
		return r;

	for (int i=MAX_SECONDARY_WEAPONS-1; i>=0; i--) {
		if (auto r = maybe_steal_primary_weapon(player_num, static_cast<primary_weapon_index_t>(i)))
			return r;
		if (auto r = maybe_steal_secondary_weapon(player_num, static_cast<secondary_weapon_index_t>(i)))
			return r;
	}

	return 0;
}

//	----------------------------------------------------------------------------
static int attempt_to_steal_item_2(object &thief, object &player_num)
{
	auto &ThiefUniqueState = LevelUniqueObjectState.ThiefState;
	const auto rval = attempt_to_steal_item_3(thief, player_num);
	if (rval) {
		digi_play_sample_once(SOUND_WEAPON_STOLEN, F1_0);
		auto i = ThiefUniqueState.Stolen_item_index;
		if (d_rand() > 20000)	//	Occasionally, boost the value again
			++i;
		constexpr auto size = std::tuple_size<decltype(ThiefUniqueState.Stolen_items)>::value;
		if (++ i >= size)
			i -= size;
		ThiefUniqueState.Stolen_item_index = i;
	}
	return rval;
}

}

//	----------------------------------------------------------------------------
//	Called for a thief-type robot.
//	If a item successfully stolen, returns true, else returns false.
//	If a wapon successfully stolen, do everything, removing it from player,
//	updating Stolen_items information, deselecting, etc.
void attempt_to_steal_item(const vmobjptridx_t thiefp, const robot_info &robptr, object &player_num)
{
	int	rval = 0;

	auto &thief = *thiefp;
	if (thief.ctype.ai_info.dying_start_time)
		return;

	if (thief.ctype.ai_info.ail.mode == ai_mode::AIM_THIEF_ATTACK)
	{
		for (auto i = 4u;; i--)
		{
			rval += attempt_to_steal_item_2(thief, player_num);
			if (!i)
				break;
			if (rval && !(d_rand() < 11000)) {	//	about 1/3 of time, steal another item
				break;
			}
		}
	}
	create_n_segment_path(thiefp, robptr, 10, ConsoleObject->segnum);
	auto &ailp = thief.ctype.ai_info.ail;
	ailp.next_action_time = Thief_wait_times[GameUniqueState.Difficulty_level] / 2;
	ailp.mode = ai_mode::AIM_THIEF_RETREAT;
	if (rval) {
		PALETTE_FLASH_ADD(30, 15, -20);
                if (Game_mode & GM_NETWORK)
                 multi_send_stolen_items();
	}
}

// --------------------------------------------------------------------------------------------------------------
//	Indicate no items have been stolen.
void init_thief_for_level(void)
{
	auto &ThiefUniqueState = LevelUniqueObjectState.ThiefState;
	ThiefUniqueState.Stolen_item_index = 0;
	auto &Stolen_items = ThiefUniqueState.Stolen_items;
	Stolen_items.fill(ThiefUniqueState.stolen_item_type_none);

	constexpr unsigned iterations = 3;
	static_assert (std::tuple_size<decltype(ThiefUniqueState.Stolen_items)>::value >= iterations * 2, "Stolen_items too small");	//	Oops!  Loop below will overwrite memory!
   if (!(Game_mode & GM_MULTI))    
		for (unsigned i = 0; i < iterations; i++)
		{
			Stolen_items[2*i] = powerup_type_t::POW_SHIELD_BOOST;
			Stolen_items[2*i+1] = powerup_type_t::POW_ENERGY;
		}
}

// --------------------------------------------------------------------------------------------------------------
void drop_stolen_items_local(d_level_unique_object_state &LevelUniqueObjectState, const d_level_shared_segment_state &LevelSharedSegmentState, d_level_unique_segment_state &LevelUniqueSegmentState, const d_vclip_array &Vclip, const vmsegptridx_t segp, const vms_vector &thief_velocity, const vms_vector &thief_position, d_thief_unique_state::stolen_item_storage &Stolen_items)
{
	for (auto &i : Stolen_items)
	{
		if (i != d_thief_unique_state::stolen_item_type_none)
			drop_powerup(LevelUniqueObjectState, LevelSharedSegmentState, LevelUniqueSegmentState, Vclip, std::exchange(i, d_thief_unique_state::stolen_item_type_none), thief_velocity, thief_position, segp, true);
	}
}

// --------------------------------------------------------------------------------------------------------------
namespace {

struct escort_menu_items
{
	std::array<char, 300> msg;
	const unsigned border_x, border_y;
	escort_menu_items(int next_goal, const char *Buddy_messages_suppressed, grs_canvas &src, gr_string_size &);
};

struct escort_menu : escort_menu_items, window
{
	escort_menu(int next_goal, const char *Buddy_messages_suppressed, grs_canvas &src, gr_string_size = {});
	virtual window_event_result event_handler(const d_event &) override;
	static window_event_result event_key_command(const d_event &event);
	void show_escort_menu();
};

escort_menu::escort_menu(int next_goal, const char *Buddy_messages_suppressed, grs_canvas &src, gr_string_size text_size) :
	escort_menu_items(next_goal, Buddy_messages_suppressed, src, text_size),
	window(src, ((src.cv_bitmap.bm_w - text_size.width) / 2) - border_x, ((src.cv_bitmap.bm_h - text_size.height) / 2) - border_y, text_size.width + (border_x * 2), text_size.height + (border_y * 2))
{
	gr_set_fontcolor(w_canv, BM_XRGB(0, 28, 0), -1);
}

window_event_result escort_menu::event_key_command(const d_event &event)
{
	auto &BuddyState = LevelUniqueObjectState.BuddyState;
	switch (const auto key = event_key_get(event))
	{
		case KEY_0:
		case KEY_1:
		case KEY_2:
		case KEY_3:
		case KEY_4:
		case KEY_5:
		case KEY_6:
		case KEY_7:
		case KEY_8:
		case KEY_9:
			BuddyState.Looking_for_marker = game_marker_index::None;
			BuddyState.Last_buddy_key = -1;
			set_escort_special_goal(BuddyState, key);
			BuddyState.Last_buddy_key = -1;
			return window_event_result::close;
		case KEY_ESC:
		case KEY_ENTER:
			return window_event_result::close;
		case KEY_T: {
			const auto temp = std::exchange(BuddyState.Buddy_messages_suppressed, 0);
			buddy_message("Messages %s.", temp ? "enabled" : "suppressed");
			BuddyState.Buddy_messages_suppressed = ~temp;
			return window_event_result::close;
		}
			
		default:
			break;
	}
	return window_event_result::ignored;
}

window_event_result escort_menu::event_handler(const d_event &event)
{
	switch (event.type)
	{
		case event_type::window_activated:
			game_flush_inputs(Controls);
			break;
		case event_type::key_command:
			return event_key_command(event);
		case event_type::idle:
			break;
		case event_type::window_draw:
			show_escort_menu();
			break;
		case event_type::window_close:
			return window_event_result::ignored;	// continue closing
		default:
			return window_event_result::ignored;
	}
	return window_event_result::handled;
}

//	-------------------------------------------------------------------------------
//	Show the Buddy menu!
void escort_menu::show_escort_menu()
{
	nm_draw_background(w_canv, 0, 0, w_canv.cv_bitmap.bm_w, w_canv.cv_bitmap.bm_h);
	gr_ustring(w_canv, *GAME_FONT, border_x, border_y, msg.data());
}

}

unsigned check_warn_local_player_can_control_guidebot(fvcobjptr &vcobjptr, const d_unique_buddy_state &BuddyState, const netgame_info &Netgame)
{
	if (!Netgame.AllowGuidebot || !(Game_mode & GM_MULTI_COOP))
	{
		HUD_init_message_literal(HM_DEFAULT, "Guide-Bot is not enabled!");
		return 0;
	}
	auto &plr = get_player_controlling_guidebot(BuddyState, Players);
	if (plr.objnum == object_none)
		return 0;
	auto &plrobj = *vcobjptr(plr.objnum);
	if (ConsoleObject != &plrobj)
	{
		HUD_init_message(HM_DEFAULT, "Guide-Bot is controlled by %s!", plr.callsign.operator const char *());
		return 0;
	}
	return 1;
}

void do_escort_menu(void)
{
	auto &BuddyState = LevelUniqueObjectState.BuddyState;
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	auto &vmobjptr = Objects.vmptr;
	auto &vmobjptridx = Objects.vmptridx;
	int	next_goal;

	if (Game_mode & GM_MULTI) {
		if (!check_warn_local_player_can_control_guidebot(vcobjptr, BuddyState, Netgame))
			return;
	}

	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	const auto &&buddy = find_escort(vmobjptridx, Robot_info);
	if (buddy == object_none)
	{
		HUD_init_message_literal(HM_DEFAULT, "No Guide-Bot present in mine!");
		return;
	}
	//	Needed here or we might not know buddy can talk when he can.
	if (!ok_for_buddy_to_talk()) {
		HUD_init_message(HM_DEFAULT, "%s has not been released.", static_cast<const char *>(PlayerCfg.GuidebotName));
		return;
	}

	auto &plrobj = get_local_plrobj();
	//	This prevents the buddy from coming back if you've told him to scram.
	//	If we don't set next_goal, we get garbage there.
	if (BuddyState.Escort_special_goal == ESCORT_GOAL_SCRAM) {
		BuddyState.Escort_special_goal = ESCORT_GOAL_UNSPECIFIED;	//	Else setting next goal might fail.
		next_goal = escort_set_goal_object(buddy, plrobj.ctype.player_info.powerup_flags);
		BuddyState.Escort_special_goal = ESCORT_GOAL_SCRAM;
	} else {
		BuddyState.Escort_special_goal = ESCORT_GOAL_UNSPECIFIED;	//	Else setting next goal might fail.
		next_goal = escort_set_goal_object(buddy, plrobj.ctype.player_info.powerup_flags);
	}

	const auto Buddy_messages_suppressed = BuddyState.Buddy_messages_suppressed
		? "Enable"
		: "Suppress";
	auto wind = window_create<escort_menu>(next_goal, Buddy_messages_suppressed, grd_curscreen->sc_canvas);
	(void)wind;
}

namespace {

escort_menu_items::escort_menu_items(const int next_goal, const char *Buddy_messages_suppressed, grs_canvas &src, gr_string_size &text_size) :
	border_x(get_border_x(src)), border_y(get_border_y(src))
{
	std::array<char, 12> goal_str;
	const char *goal_txt;
	switch (next_goal) {
		default:
		case ESCORT_GOAL_UNSPECIFIED:
			Int3();
			goal_txt = "ERROR";
			break;
		case ESCORT_GOAL_BLUE_KEY:
			goal_txt = "blue key";
			break;
		case ESCORT_GOAL_GOLD_KEY:
			goal_txt = "yellow key";
			break;
		case ESCORT_GOAL_RED_KEY:
			goal_txt = "red key";
			break;
		case ESCORT_GOAL_CONTROLCEN:
			goal_txt = "reactor";
			break;
		case ESCORT_GOAL_BOSS:
			goal_txt = "boss";
			break;
		case ESCORT_GOAL_EXIT:
			goal_txt = "exit";
			break;
		case ESCORT_GOAL_MARKER1:
		case ESCORT_GOAL_MARKER2:
		case ESCORT_GOAL_MARKER3:
		case ESCORT_GOAL_MARKER4:
		case ESCORT_GOAL_MARKER5:
		case ESCORT_GOAL_MARKER6:
		case ESCORT_GOAL_MARKER7:
		case ESCORT_GOAL_MARKER8:
		case ESCORT_GOAL_MARKER9:
			goal_txt = goal_str.data();
			std::snprintf(goal_str.data(), goal_str.size(), "marker %i", next_goal - ESCORT_GOAL_MARKER1 + 1);
			break;
	}

	snprintf(msg.data(), msg.size(), "Select Guide-Bot Command:\n\n\n"
						"0.  Next Goal: %s" CC_LSPACING_S "3\n\n"
						"\x84.  Find Energy Powerup" CC_LSPACING_S "3\n\n"
						"2.  Find Energy Center" CC_LSPACING_S "3\n\n"
						"3.  Find Shield Powerup" CC_LSPACING_S "3\n\n"
						"4.  Find Any Powerup" CC_LSPACING_S "3\n\n"
						"5.  Find a Robot" CC_LSPACING_S "3\n\n"
						"6.  Find a Hostage" CC_LSPACING_S "3\n\n"
						"7.  Stay Away From Me" CC_LSPACING_S "3\n\n"
						"8.  Find My Powerups" CC_LSPACING_S "3\n\n"
						"9.  Find the exit\n\n"
						"T.  %s Messages"
						// -- "9.	Find the exit" CC_LSPACING_S "3\n"
				, goal_txt, Buddy_messages_suppressed);
	text_size = gr_get_string_size(*GAME_FONT, msg.data());
}

}

}
