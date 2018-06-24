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

#include "segiter.h"
#include "compiler-exchange.h"
#include "compiler-range_for.h"
#include "partial_range.h"

#if DXX_USE_EDITOR
#include "editor/editor.h"
#endif

namespace dsx {

static void show_escort_menu(const array<char, 300> &);
static void say_escort_goal(escort_goal_t goal_num);
static fix64 Last_come_back_message_time;
static fix64 Buddy_last_missile_time;

constexpr array<char[12], ESCORT_GOAL_MARKER9> Escort_goal_text = {{
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
stolen_items_t Stolen_items;
int	Stolen_item_index;
fix64	Escort_last_path_created = 0;
static int Buddy_messages_suppressed;
escort_goal_t Escort_goal_object = ESCORT_GOAL_UNSPECIFIED;
escort_goal_t Escort_special_goal = ESCORT_GOAL_UNSPECIFIED;
fix64	Buddy_sorry_time;
objnum_t	 Escort_goal_index,Buddy_objnum;
int Buddy_allowed_to_talk;
static int Looking_for_marker;
static int Last_buddy_key;

static fix64 Last_buddy_message_time;

void init_buddy_for_level(void)
{
	Buddy_allowed_to_talk = 0;
	Escort_goal_object = ESCORT_GOAL_UNSPECIFIED;
	Escort_special_goal = ESCORT_GOAL_UNSPECIFIED;
	Escort_goal_index = object_none;
	Buddy_messages_suppressed = 0;
	Buddy_objnum = find_escort();
	Buddy_sorry_time = -F1_0;

	Looking_for_marker = -1;
	Last_buddy_key = -1;
}

//	-----------------------------------------------------------------------------
//	See if segment from curseg through sidenum is reachable.
//	Return true if it is reachable, else return false.
static int segment_is_reachable(const vmobjptr_t robot, const vcsegptr_t segp, int sidenum, const player_flags powerup_flags)
{
	auto wall_num = segp->sides[sidenum].wall_num;

	//	If no wall, then it is reachable
	if (wall_num == wall_none)
		return 1;

	return ai_door_is_openable(robot, powerup_flags, segp, sidenum);

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


//	-----------------------------------------------------------------------------
//	Create a breadth-first list of segments reachable from current segment.
//	max_segs is maximum number of segments to search.  Use MAX_SEGMENTS to search all.
//	On exit, *length <= max_segs.
//	Input:
//		start_seg
//	Output:
//		bfs_list:	array of shorts, each reachable segment.  Includes start segment.
//		length:		number of elements in bfs_list
std::size_t create_bfs_list(const vmobjptr_t robot, const vcsegidx_t start_seg, const player_flags powerup_flags, segnum_t *const bfs_list, std::size_t max_segs)
{
	std::size_t head = 0, tail = 0;
	visited_segment_bitarray_t visited;
	bfs_list[head++] = start_seg;
	visited[start_seg] = true;

	while ((head != tail) && (head < max_segs)) {
		auto curseg = bfs_list[tail++];
		const auto &&cursegp = vcsegptr(curseg);
		for (int i=0; i<MAX_SIDES_PER_SEGMENT; i++) {
			auto connected_seg = cursegp->children[i];

			if (IS_CHILD(connected_seg) && (!visited[connected_seg])) {
				if (segment_is_reachable(robot, cursegp, i, powerup_flags)) {
					bfs_list[head++] = connected_seg;
					if (head >= max_segs)
						break;
					visited[connected_seg] = true;
					Assert(head < MAX_SEGMENTS);
				}
			}
		}
	}
	return head;
}

//	-----------------------------------------------------------------------------
//	Return true if ok for buddy to talk, else return false.
//	Buddy is allowed to talk if the segment he is in does not contain a blastable wall that has not been blasted
//	AND he has never yet, since being initialized for level, been allowed to talk.
static int ok_for_buddy_to_talk(void)
{
	if (Buddy_objnum == object_none)
		return 0;

	const vmobjptridx_t buddy = vmobjptridx(Buddy_objnum);
	if (buddy->type != OBJ_ROBOT) {
		Buddy_allowed_to_talk = 0;
		return 0;
	}

	if (Buddy_allowed_to_talk)
		return Buddy_allowed_to_talk;

	const shared_segment &segp = vcsegptr(buddy->segnum);

	for (int i=0; i<MAX_SIDES_PER_SEGMENT; i++) {
		const auto wall_num = segp.sides[i].wall_num;

		if (wall_num != wall_none) {
			auto &w = *vcwallptr(wall_num);
			if (w.type == WALL_BLASTABLE && !(w.flags & WALL_BLASTED))
				return 0;
		}

		//	Check one level deeper.
		const auto child = segp.children[i];
		if (IS_CHILD(child))
		{
			auto &cseg = *vcsegptr(child);
			range_for (const auto &j, cseg.sides)
			{
				auto wall2 = j.wall_num;
				if (wall2 != wall_none) {
					auto &w = *vcwallptr(wall2);
					if (w.type == WALL_BLASTABLE && !(w.flags & WALL_BLASTED))
						return 0;
				}
			}
		}
	}

	Buddy_allowed_to_talk = 1;
	return 1;
}

static void record_escort_goal_accomplished()
{
	if (ok_for_buddy_to_talk()) {
		digi_play_sample_once(SOUND_BUDDY_MET_GOAL, F1_0);
		Escort_goal_index = object_none;
		Escort_goal_object = ESCORT_GOAL_UNSPECIFIED;
		Escort_special_goal = ESCORT_GOAL_UNSPECIFIED;
		Looking_for_marker = -1;
	}
}

//	--------------------------------------------------------------------------------------------
void detect_escort_goal_fuelcen_accomplished()
{
	if (!Buddy_allowed_to_talk)
		return;
	if (Escort_special_goal == ESCORT_GOAL_ENERGYCEN)
		record_escort_goal_accomplished();
}

void detect_escort_goal_accomplished(const vmobjptridx_t index)
{
	if (!Buddy_allowed_to_talk)
		return;

	// If goal is not an object (FUELCEN, EXIT, SCRAM), bail. FUELCEN is handled in detect_escort_goal_fuelcen_accomplished(), EXIT and SCRAM are never accomplished.
	if (Escort_goal_index == object_none)
		return;

	// See if goal found was a key.  Need to handle default goals differently.
	// Note, no buddy_met_goal sound when blow up reactor or exit.  Not great, but ok
	// since for reactor, noisy, for exit, buddy is disappearing.
	if (Escort_special_goal == ESCORT_GOAL_UNSPECIFIED && Escort_goal_index == index)
	{
		record_escort_goal_accomplished();
		return;
	}

	if (index->type == OBJ_POWERUP)  {
		const auto index_id = get_powerup_id(index);
		escort_goal_t goal_key;
		if ((index_id == POW_KEY_BLUE && (goal_key = ESCORT_GOAL_BLUE_KEY, true)) ||
			(index_id == POW_KEY_GOLD && (goal_key = ESCORT_GOAL_GOLD_KEY, true)) ||
			(index_id == POW_KEY_RED && (goal_key = ESCORT_GOAL_RED_KEY, true))
		)
		{
			if (Escort_goal_object == goal_key)
			{
				record_escort_goal_accomplished();
				return;
			}
		}
	}
	if (Escort_special_goal != ESCORT_GOAL_UNSPECIFIED)
	{
		if ((index->type == OBJ_POWERUP) && (Escort_special_goal == ESCORT_GOAL_POWERUP))
			record_escort_goal_accomplished();	//	Any type of powerup picked up will do.
		else if (Escort_goal_index != object_guidebot_cannot_reach)
		{
			const auto &&egi_objp = vcobjptr(Escort_goal_index);
			if (index->type == egi_objp->type && index->id == egi_objp->id)
				// Note: This will help a little bit in making the buddy believe a goal is satisfied.  Won't work for a general goal like "find any powerup"
				// because of the insistence of both type and id matching.
				record_escort_goal_accomplished();
		}
	}
}

#define DXX_GUIDEBOT_RENAME_MENU(VERB)	\
	DXX_MENUITEM(VERB, INPUT, text, opt_name)	\

void change_guidebot_name()
{
	auto text = PlayerCfg.GuidebotName;
	array<newmenu_item, DXX_GUIDEBOT_RENAME_MENU(COUNT)> m;
	enum
	{
		DXX_GUIDEBOT_RENAME_MENU(ENUM)
	};
	DXX_GUIDEBOT_RENAME_MENU(ADD);
	const auto item = newmenu_do(nullptr, "Enter Guide-bot name:", m, unused_newmenu_subfunction, unused_newmenu_userdata);

	if (item != -1) {
		PlayerCfg.GuidebotName = PlayerCfg.GuidebotNameReal = text;
		write_player_file();
	}
}

#undef DXX_GUIDEBOT_RENAME_MENU

//	-----------------------------------------------------------------------------
static int show_buddy_message()
{
	if (Buddy_messages_suppressed)
		return 0;

	if (Game_mode & GM_MULTI)
		return 0;

	if (Last_buddy_message_time + F1_0 < GameTime64) {
		if (auto r = ok_for_buddy_to_talk())
			return r;
	}
	return 0;
}

__attribute_nonnull()
static void buddy_message_force_str(const char *str)
{
	Last_buddy_message_time = GameTime64;
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
	if (Buddy_messages_suppressed)
		return;
	va_list	args;
	va_start(args, fmt);
	buddy_message_force_va(fmt, args);
	va_end(args);
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

//	-----------------------------------------------------------------------------
static void thief_message_str(const char * str) __attribute_nonnull();
static void thief_message_str(const char * str)
{
	HUD_init_message(HM_DEFAULT, "%c%cTHIEF:%c%c %s", 1, BM_XRGB(28, 0, 0), 1, BM_XRGB(0, 31, 0), str);
}

static void thief_message(const char * format, ... ) __attribute_format_printf(1, 2);
static void thief_message(const char * format, ... )
#define thief_message(F,...)	dxx_call_printf_checked(thief_message,thief_message_str,(),(F),##__VA_ARGS__)
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
static int marker_exists_in_mine(int id)
{
	range_for (const auto &&objp, vcobjptr)
	{
		if (objp->type == OBJ_MARKER)
			if (get_marker_id(objp) == id)
				return 1;
	}
	return 0;
}

//	-----------------------------------------------------------------------------
void set_escort_special_goal(int special_key)
{
	int marker_key;

	Buddy_messages_suppressed = 0;

	if (!Buddy_allowed_to_talk) {
		ok_for_buddy_to_talk();
		if (!Buddy_allowed_to_talk) {
			auto o = find_escort();
			if (o == object_none)
				HUD_init_message_literal(HM_DEFAULT, "No Guide-Bot in mine.");
			else
				HUD_init_message(HM_DEFAULT, "%s has not been released.", static_cast<const char *>(PlayerCfg.GuidebotName));
			return;
		}
	}

	special_key = special_key & (~KEY_SHIFTED);

	marker_key = special_key;
	
	if (Last_buddy_key == special_key)
	{
		if ((Looking_for_marker == -1) && (special_key != KEY_0)) {
			if (marker_exists_in_mine(marker_key - KEY_1))
				Looking_for_marker = marker_key - KEY_1;
			else {
				buddy_message_ignore_time("Marker %i not placed.", marker_key - KEY_1 + 1);
				Looking_for_marker = -1;
			}
		} else {
			Looking_for_marker = -1;
		}
	}

	Last_buddy_key = special_key;

	if (special_key == KEY_0)
		Looking_for_marker = -1;
		
	if ( Looking_for_marker != -1 ) {
		Escort_special_goal = static_cast<escort_goal_t>(ESCORT_GOAL_MARKER1 + marker_key - KEY_1);
	} else {
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

	Last_buddy_message_time = GameTime64 - 2*F1_0;	//	Allow next message to come through.

	say_escort_goal(Escort_special_goal);
	// -- Escort_goal_object = escort_set_goal_object();

	Escort_goal_object = ESCORT_GOAL_UNSPECIFIED;
}

//	-----------------------------------------------------------------------------
//	Return id of boss.
static int get_boss_id(void)
{
	range_for (const auto &&objp, vcobjptr)
	{
		if (objp->type == OBJ_ROBOT)
		{
			const auto objp_id = get_robot_id(objp);
			if (Robot_info[objp_id].boss_flag)
				return objp_id;
		}
	}
	return -1;
}

//	-----------------------------------------------------------------------------
//	Return object index if object of objtype, objid exists in mine, else return -1
//	"special" is used to find objects spewed by player which is hacked into flags field of powerup.
static objnum_t exists_in_mine_2(const unique_segment &segp, const int objtype, const int objid, const int special)
{
	range_for (const auto curobjp, objects_in(segp, vcobjptridx, vcsegptr))
	{
		const auto &objnum = curobjp;
			if (special == ESCORT_GOAL_PLAYER_SPEW && curobjp->type == OBJ_POWERUP)
			{
				if (curobjp->flags & OF_PLAYER_DROPPED)
					return objnum;
			}

			if (curobjp->type == objtype) {
				//	Don't find escort robots if looking for robot!
				if ((curobjp->type == OBJ_ROBOT) && (Robot_info[get_robot_id(curobjp)].companion))
					;
				else if (objid == -1) {
						return objnum;
				} else if (curobjp->id == objid)
					return objnum;
			}

			if (objtype == OBJ_POWERUP && curobjp->type == OBJ_ROBOT)
				if (curobjp->contains_count)
					if (curobjp->contains_type == OBJ_POWERUP)
						if (curobjp->contains_id == objid)
							return objnum;
	}
	return object_none;
}

//	-----------------------------------------------------------------------------
static segnum_t exists_fuelcen_in_mine(const vcsegidx_t start_seg, const player_flags powerup_flags)
{
	array<segnum_t, MAX_SEGMENTS> bfs_list;
	const auto length = create_bfs_list(vmobjptr(Buddy_objnum), start_seg, powerup_flags, bfs_list);
	auto predicate = [](const segnum_t &s) { return vcsegptr(s)->special == SEGMENT_IS_FUELCEN; };
	{
		const auto &&rb = partial_const_range(bfs_list, length);
		auto i = std::find_if(rb.begin(), rb.end(), predicate);
		if (i != rb.end())
			return *i;
	}
	{
		const auto &rh = vcsegptr;
		const auto a = [](const vcsegptr_t &s) {
			return s->special == SEGMENT_IS_FUELCEN;
		};
		if (std::find_if(rh.begin(), rh.end(), a) != rh.end())
			return segment_exit;
	}
	return segment_none;
}

//	Return nearest object of interest.
//	If special == ESCORT_GOAL_PLAYER_SPEW, then looking for any object spewed by player.
//	-1 means object does not exist in mine.
//	-2 means object does exist in mine, but buddy-bot can't reach it (eg, behind triggered wall)
static objnum_t exists_in_mine(const vcsegidx_t start_seg, const int objtype, const int objid, const int special, const player_flags powerup_flags)
{
	array<segnum_t, MAX_SEGMENTS> bfs_list;
	const auto length = create_bfs_list(vmobjptr(Buddy_objnum), start_seg, powerup_flags, bfs_list);

	range_for (const auto segnum, partial_const_range(bfs_list, length))
	{
		const auto &&objnum = exists_in_mine_2(vcsegptr(segnum), objtype, objid, special);
			if (objnum != object_none)
				return objnum;

		}

	//	Couldn't find what we're looking for by looking at connectivity.
	//	See if it's in the mine.  It could be hidden behind a trigger or switch
	//	which the buddybot doesn't understand.
	range_for (const auto &&segnum, vcsegptr)
		{
		const auto &&objnum = exists_in_mine_2(segnum, objtype, objid, special);
			if (objnum != object_none)
				return object_guidebot_cannot_reach;
		}

	return object_none;
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
				const unsigned zero_based_goal_num = goal_num - ESCORT_GOAL_MARKER1;
				buddy_message("Finding marker %i: '%.24s'", zero_based_goal_num + 1, &MarkerState.message[zero_based_goal_num][0]);
			}
			return;
	}
	buddy_message_str(str);
}

static void clear_escort_goals()
{
	Looking_for_marker = -1;
	Escort_goal_object = ESCORT_GOAL_UNSPECIFIED;
	Escort_special_goal = ESCORT_GOAL_UNSPECIFIED;
}

static void escort_goal_does_not_exist(escort_goal_t goal)
{
	buddy_message_ignore_time("No %s in mine.", Escort_goal_text[goal - 1]);
	clear_escort_goals();
}

static void escort_goal_unreachable(escort_goal_t goal)
{
	buddy_message_ignore_time("Can't reach %s.", Escort_goal_text[goal - 1]);
	clear_escort_goals();
}

static void escort_go_to_goal(const vmobjptridx_t objp, ai_static *aip, segnum_t goal_seg)
{
	create_path_to_segment(objp, goal_seg, Max_escort_length, create_path_safety_flag::safe);	//	MK!: Last parm (safety_flag) used to be 1!!
	if (aip->path_length > 3)
		aip->path_length = polish_path(objp, &Point_segs[aip->hide_index], aip->path_length);
	if ((aip->path_length > 0) && (Point_segs[aip->hide_index + aip->path_length - 1].segnum != goal_seg)) {
		fix	dist_to_player;
		buddy_message_ignore_time("Can't reach %s.", Escort_goal_text[Escort_goal_object - 1]);
		Looking_for_marker = -1;
		Escort_goal_object = ESCORT_GOAL_SCRAM;
		dist_to_player = find_connected_distance(objp->pos, vmsegptridx(objp->segnum), Believed_player_pos, vmsegptridx(Believed_player_seg), 100, WID_FLY_FLAG);
		if (dist_to_player > MIN_ESCORT_DISTANCE)
			create_path_to_player(objp, Max_escort_length, create_path_safety_flag::safe);	//	MK!: Last parm used to be 1!
		else {
			create_n_segment_path(objp, 8 + d_rand() * 8, segment_none);
			aip->path_length = polish_path(objp, &Point_segs[aip->hide_index], aip->path_length);
		}
	}
}

//	-----------------------------------------------------------------------------
static imsegidx_t escort_get_goal_segment(const vcobjptr_t objp, int objtype, int objid, const player_flags powerup_flags)
{
	const auto egi = exists_in_mine(objp->segnum, objtype, objid, -1, powerup_flags);
	Escort_goal_index = egi;
	if (egi != object_none && egi != object_guidebot_cannot_reach)
		return vcobjptr(egi)->segnum;
	return segment_none;
}

static void escort_create_path_to_goal(const vmobjptridx_t objp, const player_info &player_info)
{
	segnum_t	goal_seg = segment_none;
	ai_static	*aip = &objp->ctype.ai_info;
	ai_local		*ailp = &objp->ctype.ai_info.ail;

	if (Escort_special_goal != ESCORT_GOAL_UNSPECIFIED)
		Escort_goal_object = Escort_special_goal;

	const auto powerup_flags = player_info.powerup_flags;
	if (Looking_for_marker != -1) {
		goal_seg = escort_get_goal_segment(objp, OBJ_MARKER, Escort_goal_object - ESCORT_GOAL_MARKER1, powerup_flags);
	} else {
		switch (Escort_goal_object) {
			case ESCORT_GOAL_BLUE_KEY:
				goal_seg = escort_get_goal_segment(objp, OBJ_POWERUP, POW_KEY_BLUE, powerup_flags);
				break;
			case ESCORT_GOAL_GOLD_KEY:
				goal_seg = escort_get_goal_segment(objp, OBJ_POWERUP, POW_KEY_GOLD, powerup_flags);
				break;
			case ESCORT_GOAL_RED_KEY:
				goal_seg = escort_get_goal_segment(objp, OBJ_POWERUP, POW_KEY_RED, powerup_flags);
				break;
			case ESCORT_GOAL_CONTROLCEN:
				goal_seg = escort_get_goal_segment(objp, OBJ_CNTRLCEN, -1, powerup_flags);
				break;
			case ESCORT_GOAL_EXIT:
				goal_seg = find_exit_segment();
				Escort_goal_index = object_none;
				if (goal_seg == segment_none)
					escort_goal_does_not_exist(ESCORT_GOAL_EXIT);
				else if (goal_seg == segment_exit)
					escort_goal_unreachable(ESCORT_GOAL_EXIT);
				else
					escort_go_to_goal(objp, aip, goal_seg);
				return;
			case ESCORT_GOAL_ENERGY:
				goal_seg = escort_get_goal_segment(objp, OBJ_POWERUP, POW_ENERGY, powerup_flags);
				break;
			case ESCORT_GOAL_ENERGYCEN:
				goal_seg = exists_fuelcen_in_mine(objp->segnum, powerup_flags);
				Escort_goal_index = object_none;
				if (goal_seg == segment_none)
					escort_goal_does_not_exist(ESCORT_GOAL_ENERGYCEN);
				else if (goal_seg == segment_exit)
					escort_goal_unreachable(ESCORT_GOAL_ENERGYCEN);
				else
					escort_go_to_goal(objp, aip, goal_seg);
				return;
			case ESCORT_GOAL_SHIELD:
				goal_seg = escort_get_goal_segment(objp, OBJ_POWERUP, POW_SHIELD_BOOST, powerup_flags);
				break;
			case ESCORT_GOAL_POWERUP:
				goal_seg = escort_get_goal_segment(objp, OBJ_POWERUP, -1, powerup_flags);
				break;
			case ESCORT_GOAL_ROBOT:
				goal_seg = escort_get_goal_segment(objp, OBJ_ROBOT, -1, powerup_flags);
				break;
			case ESCORT_GOAL_HOSTAGE:
				goal_seg = escort_get_goal_segment(objp, OBJ_HOSTAGE, -1, powerup_flags);
				break;
			case ESCORT_GOAL_PLAYER_SPEW:
				Escort_goal_index = exists_in_mine(objp->segnum, -1, -1, ESCORT_GOAL_PLAYER_SPEW, powerup_flags);
				if (Escort_goal_index != object_none) goal_seg = Objects[Escort_goal_index].segnum;
				break;
			case ESCORT_GOAL_SCRAM:
				Escort_goal_index = object_none;
				break;
			case ESCORT_GOAL_BOSS: {
				int	boss_id;
	
				boss_id = get_boss_id();
				Assert(boss_id != -1);
				goal_seg = escort_get_goal_segment(objp, OBJ_ROBOT, boss_id, powerup_flags);
				break;
			}
			default:
				Int3();	//	Oops, Illegal value in Escort_goal_object.
				goal_seg = 0;
				Escort_goal_index = object_none;
				break;
		}
	}

		if (Escort_goal_index == object_none) {
			escort_goal_does_not_exist(Escort_goal_object);
		} else if (Escort_goal_index == object_guidebot_cannot_reach) {
			escort_goal_unreachable(Escort_goal_object);

	} else {
		if (Escort_goal_object == ESCORT_GOAL_SCRAM) {
			create_n_segment_path(objp, 16 + d_rand() * 16, segment_none);
			aip->path_length = polish_path(objp, &Point_segs[aip->hide_index], aip->path_length);
		} else {
			escort_go_to_goal(objp, aip, goal_seg);
		}

		ailp->mode = ai_mode::AIM_GOTO_OBJECT;

		say_escort_goal(Escort_goal_object);
	}

}

//	-----------------------------------------------------------------------------
//	Escort robot chooses goal object based on player's keys, location.
//	Returns goal object.
static escort_goal_t escort_set_goal_object(const player_flags pl_flags)
{
	if (Escort_special_goal != ESCORT_GOAL_UNSPECIFIED)
		return ESCORT_GOAL_UNSPECIFIED;
	if (!(pl_flags & PLAYER_FLAGS_BLUE_KEY) && exists_in_mine(ConsoleObject->segnum, OBJ_POWERUP, POW_KEY_BLUE, -1, pl_flags) != object_none)
		return ESCORT_GOAL_BLUE_KEY;
	else if (!(pl_flags & PLAYER_FLAGS_GOLD_KEY) && exists_in_mine(ConsoleObject->segnum, OBJ_POWERUP, POW_KEY_GOLD, -1, pl_flags) != object_none)
		return ESCORT_GOAL_GOLD_KEY;
	else if (!(pl_flags & PLAYER_FLAGS_RED_KEY) && exists_in_mine(ConsoleObject->segnum, OBJ_POWERUP, POW_KEY_RED, -1, pl_flags) != object_none)
		return ESCORT_GOAL_RED_KEY;
	else if (Control_center_destroyed == 0)
	{
		if (!Boss_teleport_segs.empty())
			return ESCORT_GOAL_BOSS;
		else
			return ESCORT_GOAL_CONTROLCEN;
	} else
		return ESCORT_GOAL_EXIT;
}

#define	MAX_ESCORT_TIME_AWAY		(F1_0*4)

fix64	Buddy_last_seen_player = 0, Buddy_last_player_path_created;

//	-----------------------------------------------------------------------------
static int time_to_visit_player(const vmobjptr_t objp, ai_local *ailp, ai_static *aip)
{
	//	Note: This one has highest priority because, even if already going towards player,
	//	might be necessary to create a new path, as player can move.
	if (GameTime64 - Buddy_last_seen_player > MAX_ESCORT_TIME_AWAY)
		if (GameTime64 - Buddy_last_player_path_created > F1_0)
			return 1;

	if (ailp->mode == ai_mode::AIM_GOTO_PLAYER)
		return 0;

	if (objp->segnum == ConsoleObject->segnum)
		return 0;

	if (aip->cur_path_index < aip->path_length/2)
		return 0;
	
	return 1;
}

//	-----------------------------------------------------------------------------
static void bash_buddy_weapon_info(const vmobjptridx_t objp)
{
	auto &laser_info = objp->ctype.laser_info;
	const auto console = ConsoleObject;
	laser_info.parent_num = vmobjptridx(console);
	laser_info.parent_type = OBJ_PLAYER;
	laser_info.parent_signature = console->signature;
}

//	-----------------------------------------------------------------------------
static int maybe_buddy_fire_mega(const vmobjptridx_t objp)
{
	const auto &&buddy_objp = objp.absolute_sibling(Buddy_objnum);
	fix		dist, dot;
	auto vec_to_robot = vm_vec_sub(buddy_objp->pos, objp->pos);
	dist = vm_vec_normalize_quick(vec_to_robot);

	if (dist > F1_0*100)
		return 0;

	dot = vm_vec_dot(vec_to_robot, buddy_objp->orient.fvec);

	if (dot < F1_0/2)
		return 0;

	if (!object_to_object_visibility(buddy_objp, objp, FQ_TRANSWALL))
		return 0;

	if (Weapon_info[weapon_id_type::MEGA_ID].render_type == 0) {
		con_puts(CON_VERBOSE, "Buddy can't fire mega (shareware)");
		buddy_message("CLICK!");
		return 0;
	}

	buddy_message("GAHOOGA!");

	const imobjptridx_t weapon_objnum = Laser_create_new_easy( buddy_objp->orient.fvec, buddy_objp->pos, objp, weapon_id_type::MEGA_ID, 1);

	if (weapon_objnum != object_none)
		bash_buddy_weapon_info(weapon_objnum);

	return 1;
}

//-----------------------------------------------------------------------------
static int maybe_buddy_fire_smart(const vmobjptridx_t objp)
{
	const auto &&buddy_objp = objp.absolute_sibling(Buddy_objnum);
	fix		dist;

	dist = vm_vec_dist_quick(buddy_objp->pos, objp->pos);

	if (dist > F1_0*80)
		return 0;

	if (!object_to_object_visibility(buddy_objp, objp, FQ_TRANSWALL))
		return 0;

	buddy_message("WHAMMO!");

	const imobjptridx_t weapon_objnum = Laser_create_new_easy( buddy_objp->orient.fvec, buddy_objp->pos, objp, weapon_id_type::SMART_ID, 1);

	if (weapon_objnum != object_none)
		bash_buddy_weapon_info(weapon_objnum);

	return 1;
}

//	-----------------------------------------------------------------------------
static void do_buddy_dude_stuff(void)
{
	if (!ok_for_buddy_to_talk())
		return;

	if (Buddy_last_missile_time > GameTime64)
		Buddy_last_missile_time = 0;

	if (Buddy_last_missile_time + F1_0*2 < GameTime64) {
		//	See if a robot potentially in view cone
		auto &&rh = make_range(vmobjptridx);
		range_for (const auto &&objp, rh)
		{
			if ((objp->type == OBJ_ROBOT) && !Robot_info[get_robot_id(objp)].companion)
				if (maybe_buddy_fire_mega(objp)) {
					Buddy_last_missile_time = GameTime64;
					return;
				}
		}

		//	See if a robot near enough that buddy should fire smart missile
		range_for (const auto &&objp, rh)
		{
			if ((objp->type == OBJ_ROBOT) && !Robot_info[get_robot_id(objp)].companion)
				if (maybe_buddy_fire_smart(objp)) {
					Buddy_last_missile_time = GameTime64;
					return;
				}
		}
	}
}

//	-----------------------------------------------------------------------------
//	Called every frame (or something).
void do_escort_frame(const vmobjptridx_t objp, const object &plrobj, fix dist_to_player, int player_visibility)
{
	ai_static	*aip = &objp->ctype.ai_info;
	ai_local		*ailp = &objp->ctype.ai_info.ail;

	Buddy_objnum = objp;

	auto &player_info = plrobj.ctype.player_info;
	if (player_visibility) {
		Buddy_last_seen_player = GameTime64;
		if (player_info.powerup_flags & PLAYER_FLAGS_HEADLIGHT_ON)	//	DAMN! MK, stupid bug, fixed 12/08/95, changed PLAYER_FLAGS_HEADLIGHT to PLAYER_FLAGS_HEADLIGHT_ON
		{
			const auto energy = player_info.energy;
			const auto ienergy = f2i(energy);
			if (ienergy < 40)
				if (ienergy & 4)
						buddy_message("Hey, your headlight's on!");
		}
	}

	if (cheats.buddyangry)
		do_buddy_dude_stuff();

	{
		const auto buddy_sorry_time = Buddy_sorry_time;
		if (buddy_sorry_time + F1_0 > GameTime64)
		{
			Buddy_sorry_time = -F1_0*2;
			if (buddy_sorry_time < GameTime64 + F1_0*2)
			{
				buddy_message_ignore_time("Oops, sorry 'bout that...");
			}
		}
	}

	//	If buddy not allowed to talk, then he is locked in his room.  Make him mostly do nothing unless you're nearby.
	if (!Buddy_allowed_to_talk)
		if (dist_to_player > F1_0*100)
			aip->SKIP_AI_COUNT = (F1_0/4)/FrameTime;

	//	ai_mode::AIM_WANDER has been co-opted for buddy behavior (didn't want to modify aistruct.h)
	//	It means the object has been told to get lost and has come to the end of its path.
	//	If the player is now visible, then create a path.
	if (ailp->mode == ai_mode::AIM_WANDER)
		if (player_visibility) {
			create_n_segment_path(objp, 16 + d_rand() * 16, segment_none);
			aip->path_length = polish_path(objp, &Point_segs[aip->hide_index], aip->path_length);
		}

	if (Escort_special_goal == ESCORT_GOAL_SCRAM) {
		if (player_visibility)
			if (Escort_last_path_created + F1_0*3 < GameTime64) {
				create_n_segment_path(objp, 10 + d_rand() * 16, ConsoleObject->segnum);
				Escort_last_path_created = GameTime64;
			}

		return;
	}

	//	Force checking for new goal every 5 seconds, and create new path, if necessary.
	if (((Escort_special_goal != ESCORT_GOAL_SCRAM) && ((Escort_last_path_created + F1_0*5) < GameTime64)) ||
		((Escort_special_goal == ESCORT_GOAL_SCRAM) && ((Escort_last_path_created + F1_0*15) < GameTime64))) {
		Escort_goal_object = ESCORT_GOAL_UNSPECIFIED;
		Escort_last_path_created = GameTime64;
	}

	if ((Escort_special_goal != ESCORT_GOAL_SCRAM) && time_to_visit_player(objp, ailp, aip)) {
		unsigned max_len;

		Buddy_last_player_path_created = GameTime64;
		ailp->mode = ai_mode::AIM_GOTO_PLAYER;
		if (!player_visibility) {
			if ((Last_come_back_message_time + F1_0 < GameTime64) || (Last_come_back_message_time > GameTime64)) {
				buddy_message("Coming back to get you.");
				Last_come_back_message_time = GameTime64;
			}
		}
		//	No point in Buddy creating very long path if he's not allowed to talk.  Really kills framerate.
		max_len = Max_escort_length;
		if (!Buddy_allowed_to_talk)
			max_len = 3;
		create_path_to_player(objp, max_len, create_path_safety_flag::safe);	//	MK!: Last parm used to be 1!
		aip->path_length = polish_path(objp, &Point_segs[aip->hide_index], aip->path_length);
		ailp->mode = ai_mode::AIM_GOTO_PLAYER;
	}	else if (GameTime64 - Buddy_last_seen_player > MAX_ESCORT_TIME_AWAY) {
		//	This is to prevent buddy from looking for a goal, which he will do because we only allow path creation once/second.
		return;
	} else if ((ailp->mode == ai_mode::AIM_GOTO_PLAYER) && (dist_to_player < MIN_ESCORT_DISTANCE)) {
		Escort_goal_object = escort_set_goal_object(player_info.powerup_flags);
		ailp->mode = ai_mode::AIM_GOTO_OBJECT;		//	May look stupid to be before path creation, but ai_door_is_openable uses mode to determine what doors can be got through
		escort_create_path_to_goal(objp, player_info);
		aip->path_length = polish_path(objp, &Point_segs[aip->hide_index], aip->path_length);
		if (aip->path_length < 3) {
			create_n_segment_path(objp, 5, Believed_player_seg);
		}
		ailp->mode = ai_mode::AIM_GOTO_OBJECT;
	} else if (Escort_goal_object == ESCORT_GOAL_UNSPECIFIED) {
		if ((ailp->mode != ai_mode::AIM_GOTO_PLAYER) || (dist_to_player < MIN_ESCORT_DISTANCE)) {
			Escort_goal_object = escort_set_goal_object(player_info.powerup_flags);
			ailp->mode = ai_mode::AIM_GOTO_OBJECT;		//	May look stupid to be before path creation, but ai_door_is_openable uses mode to determine what doors can be got through
			escort_create_path_to_goal(objp, player_info);
			aip->path_length = polish_path(objp, &Point_segs[aip->hide_index], aip->path_length);
			if (aip->path_length < 3) {
				create_n_segment_path(objp, 5, Believed_player_seg);
			}
			ailp->mode = ai_mode::AIM_GOTO_OBJECT;
		}
	}

}

void invalidate_escort_goal(void)
{
	Escort_goal_object = ESCORT_GOAL_UNSPECIFIED;
}

//	-------------------------------------------------------------------------------------------------
void do_snipe_frame(const vmobjptridx_t objp, fix dist_to_player, int player_visibility, const vms_vector &vec_to_player)
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

			connected_distance = find_connected_distance(objp->pos, vmsegptridx(objp->segnum), Believed_player_pos, vmsegptridx(Believed_player_seg), 30, WID_FLY_FLAG);
			if (connected_distance < F1_0*500) {
				create_path_to_player(objp, 30, create_path_safety_flag::safe);
				ailp->mode = ai_mode::AIM_SNIPE_ATTACK;
				ailp->next_action_time = SNIPE_ATTACK_TIME;	//	have up to 10 seconds to find player.
			}
			break;

		case ai_mode::AIM_SNIPE_RETREAT:
		case ai_mode::AIM_SNIPE_RETREAT_BACKWARDS:
			if (ailp->next_action_time < 0) {
				ailp->mode = ai_mode::AIM_SNIPE_WAIT;
				ailp->next_action_time = SNIPE_WAIT_TIME;
			} else if ((player_visibility == 0) || (ailp->next_action_time > SNIPE_ABORT_RETREAT_TIME)) {
				ai_follow_path(objp, player_visibility, &vec_to_player);
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
				ai_follow_path(objp, player_visibility, &vec_to_player);
				if (player_visibility) {
					ailp->mode = ai_mode::AIM_SNIPE_FIRE;
					ailp->next_action_time = SNIPE_FIRE_TIME;
				} else
					ailp->mode = ai_mode::AIM_SNIPE_ATTACK;
			}
			break;

		case ai_mode::AIM_SNIPE_FIRE:
			if (ailp->next_action_time < 0) {
				ai_static	*aip = &objp->ctype.ai_info;
				create_n_segment_path(objp, 10 + d_rand()/2048, ConsoleObject->segnum);
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

#define	THIEF_DEPTH	20

//	------------------------------------------------------------------------------------------------------
//	Choose segment to recreate thief in.
static vmsegidx_t choose_thief_recreation_segment(const vcsegidx_t plrseg)
{
	segnum_t	segnum = segment_none;
	int	cur_drop_depth;

	cur_drop_depth = THIEF_DEPTH;

	while ((segnum == segment_none) && (cur_drop_depth > THIEF_DEPTH/2)) {
		segnum = pick_connected_segment(plrseg, cur_drop_depth);
		if (segnum != segment_none && vcsegptr(segnum)->special == SEGMENT_IS_CONTROLCEN)
			segnum = segment_none;
		cur_drop_depth--;
	}

	if (segnum == segment_none) {
		return (d_rand() * Highest_segment_index) >> 15;
	} else
		return segnum;

}

static fix64	Re_init_thief_time = 0x3f000000;

//	----------------------------------------------------------------------
void recreate_thief(const uint8_t thief_id)
{
	const auto segnum = choose_thief_recreation_segment(ConsoleObject->segnum);
	const auto &&segp = vmsegptridx(segnum);
	const auto &&center_point = compute_segment_center(vcvertptr, segp);

	const auto &&new_obj = create_morph_robot(segp, center_point, thief_id);
	if (new_obj == object_none)
		return;
	Re_init_thief_time = GameTime64 + F1_0*10;		//	In 10 seconds, re-initialize thief.
}

//	----------------------------------------------------------------------------
#define	THIEF_ATTACK_TIME		(F1_0*10)

constexpr array<fix, NDL> Thief_wait_times = {{
	F1_0*30, F1_0*25, F1_0*20, F1_0*15, F1_0*10
}};

//	-------------------------------------------------------------------------------------------------
void do_thief_frame(const vmobjptridx_t objp, fix dist_to_player, int player_visibility, const vms_vector &vec_to_player)
{
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
				create_path_to_player(objp, 30, create_path_safety_flag::safe);
				ailp->mode = ai_mode::AIM_THIEF_ATTACK;
				ailp->next_action_time = THIEF_ATTACK_TIME/2;
				return;
			} else if (player_visibility) {
				create_n_segment_path(objp, 15, ConsoleObject->segnum);
				ailp->mode = ai_mode::AIM_THIEF_RETREAT;
				return;
			}

			if ((dist_to_player > F1_0*50) && (ailp->next_action_time > 0))
				return;

			ailp->next_action_time = Thief_wait_times[Difficulty_level]/2;

			connected_distance = find_connected_distance(objp->pos, vmsegptridx(objp->segnum), Believed_player_pos, vmsegptridx(Believed_player_seg), 30, WID_FLY_FLAG);
			if (connected_distance < F1_0*500) {
				create_path_to_player(objp, 30, create_path_safety_flag::safe);
				ailp->mode = ai_mode::AIM_THIEF_ATTACK;
				ailp->next_action_time = THIEF_ATTACK_TIME;	//	have up to 10 seconds to find player.
			}

			break;

		case ai_mode::AIM_THIEF_RETREAT:
			if (ailp->next_action_time < 0) {
				ailp->mode = ai_mode::AIM_THIEF_WAIT;
				ailp->next_action_time = Thief_wait_times[Difficulty_level];
			} else if ((dist_to_player < F1_0*100) || player_visibility || (ailp->player_awareness_type >= player_awareness_type_t::PA_PLAYER_COLLISION)) {
				ai_follow_path(objp, player_visibility, &vec_to_player);
				if ((dist_to_player < F1_0*100) || (ailp->player_awareness_type >= player_awareness_type_t::PA_PLAYER_COLLISION)) {
					ai_static	*aip = &objp->ctype.ai_info;
					if (((aip->cur_path_index <=1) && (aip->PATH_DIR == -1)) || ((aip->cur_path_index >= aip->path_length-1) && (aip->PATH_DIR == 1))) {
						ailp->player_awareness_type = player_awareness_type_t::PA_NONE;
						create_n_segment_path(objp, 10, ConsoleObject->segnum);

						//	If path is real short, try again, allowing to go through player's segment
						if (aip->path_length < 4) {
							create_n_segment_path(objp, 10, segment_none);
						} else if (objp->shields* 4 < Robot_info[get_robot_id(objp)].strength) {
							//	If robot really low on hits, will run through player with even longer path
							if (aip->path_length < 8) {
								create_n_segment_path(objp, 10, segment_none);
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
					create_n_segment_path(objp, 10, ConsoleObject->segnum);
					ailp->next_action_time = Thief_wait_times[Difficulty_level]/2;
					ailp->mode = ai_mode::AIM_THIEF_RETREAT;
				}
			} else if (ailp->next_action_time < 0) {
				//	This forces him to create a new path every second.
				ailp->next_action_time = F1_0;
				create_path_to_player(objp, 100, create_path_safety_flag::unsafe);
				ailp->mode = ai_mode::AIM_THIEF_ATTACK;
			} else {
				if (player_visibility && (dist_to_player < F1_0*100)) {
					//	If the player is close to looking at the thief, thief shall run away.
					//	No more stupid thief trying to sneak up on you when you're looking right at him!
					if (dist_to_player > F1_0*60) {
						fix	dot = vm_vec_dot(vec_to_player, ConsoleObject->orient.fvec);
						if (dot < -F1_0/2) {	//	Looking at least towards thief, so thief will run!
							create_n_segment_path(objp, 10, ConsoleObject->segnum);
							ailp->next_action_time = Thief_wait_times[Difficulty_level]/2;
							ailp->mode = ai_mode::AIM_THIEF_RETREAT;
						}
					} 
					ai_turn_towards_vector(vec_to_player, objp, F1_0/4);
					move_towards_player(objp, vec_to_player);
				} else {
					ai_static	*aip = &objp->ctype.ai_info;
					//	If path length == 0, then he will keep trying to create path, but he is probably stuck in his closet.
					if ((aip->path_length > 1) || ((d_tick_count & 0x0f) == 0)) {
						ai_follow_path(objp, player_visibility, &vec_to_player);
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

//	----------------------------------------------------------------------------
//	Return true if this item (whose presence is indicated by Players[player_num].flags) gets stolen.
static int maybe_steal_flag_item(const vmobjptr_t playerobjp, const PLAYER_FLAG flagval)
{
	auto &plr_flags = playerobjp->ctype.player_info.powerup_flags;
	if (plr_flags & flagval)
	{
		if (d_rand() < THIEF_PROBABILITY) {
			int	powerup_index;
			const char *msg;
			plr_flags &= (~flagval);
			switch (flagval) {
				case PLAYER_FLAGS_INVULNERABLE:
					powerup_index = POW_INVULNERABILITY;
					msg = "Invulnerability stolen!";
					break;
				case PLAYER_FLAGS_CLOAKED:
					powerup_index = POW_CLOAK;
					msg = "Cloak stolen!";
					break;
				case PLAYER_FLAGS_MAP_ALL:
					powerup_index = POW_FULL_MAP;
					msg = "Full map stolen!";
					break;
				case PLAYER_FLAGS_QUAD_LASERS:
					powerup_index = POW_QUAD_FIRE;
					msg = "Quad lasers stolen!";
					break;
				case PLAYER_FLAGS_AFTERBURNER:
					powerup_index = POW_AFTERBURNER;
					msg = "Afterburner stolen!";
					break;
				case PLAYER_FLAGS_CONVERTER:
					powerup_index = POW_CONVERTER;
					msg = "Converter stolen!";
					break;
				case PLAYER_FLAG::HEADLIGHT_PRESENT_AND_ON:
					powerup_index = POW_HEADLIGHT;
					msg = "Headlight stolen!";
					break;
				default:
					assert(false);
					return 0;
			}
			Stolen_items[Stolen_item_index] = powerup_index;
			thief_message_str(msg);
			return 1;
		}
	}

	return 0;
}

//	----------------------------------------------------------------------------
static int maybe_steal_secondary_weapon(const vmobjptr_t playerobjp, int weapon_num)
{
	auto &player_info = playerobjp->ctype.player_info;
	if (auto &secondary_ammo = player_info.secondary_ammo[weapon_num])
		if (d_rand() < THIEF_PROBABILITY) {
			if (weapon_index_is_player_bomb(weapon_num))
			{
				if (d_rand() > 8192)		//	Come in groups of 4, only add 1/4 of time.
					return 0;
			}
			//	Smart mines and proxbombs don't get dropped because they only come in 4 packs.
			else
				Stolen_items[Stolen_item_index] = Secondary_weapon_to_powerup[weapon_num];
			thief_message("%s stolen!", SECONDARY_WEAPON_NAMES(weapon_num));		//	Danger! Danger! Use of literal!  Danger!
			if (-- secondary_ammo == 0)
				auto_select_secondary_weapon(player_info);

			// -- compress_stolen_items();
			return 1;
		}

	return 0;
}

//	----------------------------------------------------------------------------
static int maybe_steal_primary_weapon(const vmobjptr_t playerobjp, int weapon_num)
{
	auto &player_info = playerobjp->ctype.player_info;
	bool is_energy_weapon = true;
	switch (static_cast<primary_weapon_index_t>(weapon_num))
	{
		case primary_weapon_index_t::LASER_INDEX:
			if (!player_info.laser_level)
				return 0;
			break;
		case primary_weapon_index_t::VULCAN_INDEX:
		case primary_weapon_index_t::GAUSS_INDEX:
			if (!player_info.vulcan_ammo)
				return 0;
			is_energy_weapon = false;
			//-fallthrough
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
			if (weapon_num == primary_weapon_index_t::LASER_INDEX)
			{
				auto &laser_level = player_info.laser_level;
					if (laser_level > MAX_LASER_LEVEL)
					{
						Stolen_items[Stolen_item_index] = POW_SUPER_LASER;
					} else {
						Stolen_items[Stolen_item_index] = Primary_weapon_to_powerup[weapon_num];
					}
					/* Laser levels are zero-based, so print the old
					 * level, then decrement it.  Decrementing first
					 * would produce confusing output, particularly when
					 * the user loses the final laser powerup and drops
					 * to level 0 lasers.
					 */
					const laser_level_t l = laser_level;
					-- laser_level;
					thief_message("%s level decreased to %u!", PRIMARY_WEAPON_NAMES(weapon_num), l);
			}
			else
			{
				player_info.primary_weapon_flags &= ~HAS_PRIMARY_FLAG(weapon_num);
				Stolen_items[Stolen_item_index] = Primary_weapon_to_powerup[weapon_num];

				thief_message("%s stolen!", PRIMARY_WEAPON_NAMES(weapon_num));		//	Danger! Danger! Use of literal!  Danger!
				auto_select_primary_weapon(player_info);
			}
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
static int attempt_to_steal_item_3(const vmobjptr_t objp, const vmobjptr_t player_num)
{
	ai_local		*ailp = &objp->ctype.ai_info.ail;
	if (ailp->mode != ai_mode::AIM_THIEF_ATTACK)
		return 0;

	//	First, try to steal equipped items.

	if (auto r = maybe_steal_flag_item(player_num, PLAYER_FLAGS_INVULNERABLE))
		return r;

	//	If primary weapon = laser, first try to rip away those nasty quad lasers!
	const auto Primary_weapon = player_num->ctype.player_info.Primary_weapon;
	if (Primary_weapon == primary_weapon_index_t::LASER_INDEX)
		if (auto r = maybe_steal_flag_item(player_num, PLAYER_FLAGS_QUAD_LASERS))
			return r;

	//	Makes it more likely to steal primary than secondary.
	for (int i=0; i<2; i++)
		if (auto r = maybe_steal_primary_weapon(player_num, Primary_weapon))
			return r;

	if (auto r = maybe_steal_secondary_weapon(player_num, player_num->ctype.player_info.Secondary_weapon))
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
		if (auto r = maybe_steal_primary_weapon(player_num, i))
			return r;
		if (auto r = maybe_steal_secondary_weapon(player_num, i))
			return r;
	}

	return 0;
}

//	----------------------------------------------------------------------------
static int attempt_to_steal_item_2(const vmobjptr_t objp, const vmobjptr_t player_num)
{
	const auto rval = attempt_to_steal_item_3(objp, player_num);
	if (rval) {
		digi_play_sample_once(SOUND_WEAPON_STOLEN, F1_0);
		auto i = Stolen_item_index;
		if (d_rand() > 20000)	//	Occasionally, boost the value again
			++i;
		constexpr auto size = Stolen_items.size();
		if (++ i >= size)
			i -= size;
		Stolen_item_index = i;
	}
	return rval;
}

//	----------------------------------------------------------------------------
//	Called for a thief-type robot.
//	If a item successfully stolen, returns true, else returns false.
//	If a wapon successfully stolen, do everything, removing it from player,
//	updating Stolen_items information, deselecting, etc.
int attempt_to_steal_item(const vmobjptridx_t objp, const vmobjptr_t player_num)
{
	int	rval = 0;

	if (objp->ctype.ai_info.dying_start_time)
		return 0;

	rval += attempt_to_steal_item_2(objp, player_num);

	for (int i=0; i<3; i++) {
		if (!rval || (d_rand() < 11000)) {	//	about 1/3 of time, steal another item
			rval += attempt_to_steal_item_2(objp, player_num);
		} else
			break;
	}
	create_n_segment_path(objp, 10, ConsoleObject->segnum);
	ai_local		*ailp = &objp->ctype.ai_info.ail;
	ailp->next_action_time = Thief_wait_times[Difficulty_level]/2;
	ailp->mode = ai_mode::AIM_THIEF_RETREAT;
	if (rval) {
		PALETTE_FLASH_ADD(30, 15, -20);
		update_laser_weapon_info();
//		digi_link_sound_to_pos( SOUND_NASTY_ROBOT_HIT_1, objp->segnum, 0, &objp->pos, 0 , DEFAULT_ROBOT_SOUND_VOLUME);
//	I removed this to make the "steal sound" more obvious -AP
                if (Game_mode & GM_NETWORK)
                 multi_send_stolen_items();
	}
	return rval;
}

// --------------------------------------------------------------------------------------------------------------
//	Indicate no items have been stolen.
void init_thief_for_level(void)
{
	Stolen_items.fill(255);

	constexpr unsigned iterations = 3;
	static_assert (Stolen_items.size() >= iterations * 2, "Stolen_items too small");	//	Oops!  Loop below will overwrite memory!
   if (!(Game_mode & GM_MULTI))    
		for (unsigned i = 0; i < iterations; i++)
		{
			Stolen_items[2*i] = POW_SHIELD_BOOST;
			Stolen_items[2*i+1] = POW_ENERGY;
		}

	Stolen_item_index = 0;
}

// --------------------------------------------------------------------------------------------------------------
void drop_stolen_items(const vcobjptr_t objp)
{
	const auto &&segp = vmsegptridx(objp->segnum);
	range_for (auto &i, Stolen_items)
	{
		if (i != 255)
		{
			drop_powerup(exchange(i, 255), 1, objp->mtype.phys_info.velocity, objp->pos, segp, true);
		}
	}

}

// --------------------------------------------------------------------------------------------------------------
namespace {

struct escort_menu : ignore_window_pointer_t
{
	array<char, 300> msg;
	static window_event_result event_handler(window *wind, const d_event &event, escort_menu *menu);
	static window_event_result event_key_command(const d_event &event);
};

}

window_event_result escort_menu::event_key_command(const d_event &event)
{
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
			Looking_for_marker = -1;
			Last_buddy_key = -1;
			set_escort_special_goal(key);
			Last_buddy_key = -1;
			return window_event_result::close;
		case KEY_ESC:
		case KEY_ENTER:
			return window_event_result::close;
		case KEY_T: {
			auto temp = exchange(Buddy_messages_suppressed, 0);
			buddy_message("Messages %s.", temp ? "enabled" : "suppressed");
			Buddy_messages_suppressed = ~temp;
			return window_event_result::close;
		}
			
		default:
			break;
	}
	return window_event_result::ignored;
}

window_event_result escort_menu::event_handler(window *, const d_event &event, escort_menu *menu)
{
	switch (event.type)
	{
		case EVENT_WINDOW_ACTIVATED:
			game_flush_inputs();
			break;
			
		case EVENT_KEY_COMMAND:
			return event_key_command(event);
		case EVENT_IDLE:
			timer_delay2(50);
			break;
			
		case EVENT_WINDOW_DRAW:
			show_escort_menu(menu->msg);		//TXT_PAUSE);
			break;
			
		case EVENT_WINDOW_CLOSE:
			d_free(menu);
			return window_event_result::ignored;	// continue closing
		default:
			return window_event_result::ignored;
	}
	return window_event_result::handled;
}

void do_escort_menu(void)
{
	int	next_goal;
	char	goal_str[32];
	const char *goal_txt;
	const char *tstr;
	escort_menu *menu;

	if (Game_mode & GM_MULTI) {
		HUD_init_message_literal(HM_DEFAULT, "No Guide-Bot in Multiplayer!");
		return;
	}

	auto buddy = find_escort();
	if (buddy == object_none)
	{
		HUD_init_message_literal(HM_DEFAULT, "No Guide-Bot present in mine!");
		return;
	}
	ok_for_buddy_to_talk();	//	Needed here or we might not know buddy can talk when he can.

	if (!Buddy_allowed_to_talk) {
		HUD_init_message(HM_DEFAULT, "%s has not been released.", static_cast<const char *>(PlayerCfg.GuidebotName));
		return;
	}

	MALLOC(menu, escort_menu, 1);
	if (!menu)
		return;
	
	// Just make it the full screen size and let show_escort_menu figure it out
	const auto wind = window_create(grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT, &escort_menu::event_handler, menu);
	if (!wind)
	{
		d_free(menu);
		return;
	}
	
	auto &plrobj = get_local_plrobj();
	//	This prevents the buddy from coming back if you've told him to scram.
	//	If we don't set next_goal, we get garbage there.
	if (Escort_special_goal == ESCORT_GOAL_SCRAM) {
		Escort_special_goal = ESCORT_GOAL_UNSPECIFIED;	//	Else setting next goal might fail.
		next_goal = escort_set_goal_object(plrobj.ctype.player_info.powerup_flags);
		Escort_special_goal = ESCORT_GOAL_SCRAM;
	} else {
		Escort_special_goal = ESCORT_GOAL_UNSPECIFIED;	//	Else setting next goal might fail.
		next_goal = escort_set_goal_object(plrobj.ctype.player_info.powerup_flags);
	}

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
			goal_txt = goal_str;
			snprintf(goal_str, sizeof(goal_str), "marker %i", next_goal-ESCORT_GOAL_MARKER1+1);
			break;

	}
			
	if (!Buddy_messages_suppressed)
		tstr =  "Suppress";
	else
		tstr =  "Enable";

	snprintf(menu->msg.data(), menu->msg.size(), "Select Guide-Bot Command:\n\n\n"
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
				, goal_txt, tstr);
}

//	-------------------------------------------------------------------------------
//	Show the Buddy menu!
void show_escort_menu(const array<char, 300> &amsg)
{	
	const auto msg = amsg.data();
	int	w,h;
	int	x,y;


	gr_set_default_canvas();

	auto &canvas = *grd_curcanv;
	const auto &game_font = *GAME_FONT;
	gr_get_string_size(game_font, msg, &w, &h, nullptr);

	x = (SWIDTH-w)/2;
	y = (SHEIGHT-h)/2;

	gr_set_fontcolor(canvas, BM_XRGB(0, 28, 0), -1);

	nm_draw_background(canvas, x - BORDERX, y - BORDERY, x + w + BORDERX, y + h + BORDERY);

	gr_ustring(canvas, game_font, x, y, msg);

	reset_cockpit();
}

}
