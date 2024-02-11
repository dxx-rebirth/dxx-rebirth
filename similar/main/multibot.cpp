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
 * Multiplayer robot code
 *
 */
#include <type_traits>
#include "multiinternal.h"

#include <string.h>
#include <stdlib.h>
#include <stdexcept>

#include "vecmat.h"
#include "object.h"
#include "multibot.h"
#include "game.h"
#include "laser.h"
#include "dxxerror.h"
#include "timer.h"
#include "player.h"
#include "ai.h"
#include "fireball.h"
#include "aistruct.h"
#include "gameseg.h"
#include "robot.h"
#include "powerup.h"
#include "gauges.h"
#include "fuelcen.h"
#include "morph.h"
#include "digi.h"
#include "sounds.h"
#include "effects.h"
#include "automap.h"
#include "physics.h" 
#include "byteutil.h"
#include "escort.h"

#include "compiler-range_for.h"
#include "d_levelstate.h"
#include "partial_range.h"

namespace dcx {
namespace {
std::array<multi_send_robot_position_priority, MAX_ROBOTS_CONTROLLED> robot_send_pending;
std::array<multi_command<multiplayer_command_t::MULTI_ROBOT_FIRE>, MAX_ROBOTS_CONTROLLED> robot_fire_buf;
}
std::array<objnum_t, MAX_ROBOTS_CONTROLLED> robot_controlled;
std::array<int, MAX_ROBOTS_CONTROLLED> robot_agitation;
std::bitset<MAX_ROBOTS_CONTROLLED> robot_fired;
}

namespace dsx {
namespace {
static int multi_add_controlled_robot(vmobjptridx_t objnum, int agitation);
void multi_drop_robot_powerups(object &objnum);
static void multi_send_robot_position_sub(const vmobjptridx_t objnum, multiplayer_data_priority now);
static void multi_send_release_robot(vmobjptridx_t objnum);
static void multi_delete_controlled_robot(const vmobjptridx_t objnum);
}
}

//
// Code for controlling robots in multiplayer games
//

#define STANDARD_EXPL_DELAY (F1_0/4)
#if defined(DXX_BUILD_DESCENT_I)
#define MIN_CONTROL_TIME	F1_0*2
#define ROBOT_TIMEOUT		F1_0*3

#define MAX_TO_DELETE	67
#elif defined(DXX_BUILD_DESCENT_II)
#define MIN_CONTROL_TIME	F1_0*1
#define ROBOT_TIMEOUT		F1_0*2
#endif

#define MIN_TO_ADD	60

std::array<fix64, MAX_ROBOTS_CONTROLLED> robot_controlled_time,
	robot_last_send_time,
	robot_last_message_time;

#define MULTI_ROBOT_PRIORITY(objnum, pnum) (((objnum % 4) + pnum) % N_players)

//#define MULTI_ROBOT_PRIORITY(objnum, pnum) multi_robot_priority(objnum, pnum)
//int multi_robot_priority(int objnum, int pnum)
//{
//	return( ((objnum % 4) + pnum) % N_players);
//}

int multi_can_move_robot(const vmobjptridx_t objnum, int agitation)
{
	auto &BossUniqueState = LevelUniqueObjectState.BossState;
	// Determine whether or not I am allowed to move this robot.
	// Claim robot if necessary.

	if (Player_dead_state == player_dead_state::exploded)
		return 0;

	auto &objrobot = *objnum;
	if (objrobot.type != OBJ_ROBOT)
	{
#ifndef NDEBUG
		Int3();
#endif
		return 0;
	}

	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	if (Robot_info[get_robot_id(objrobot)].boss_flag != boss_robot_id::None && BossUniqueState.Boss_dying)
		return 0;
	else if (objrobot.ctype.ai_info.REMOTE_OWNER == Player_num) // Already my robot!
	{
		const auto slot_num = objrobot.ctype.ai_info.REMOTE_SLOT_NUM;

      if ((slot_num < 0) || (slot_num >= MAX_ROBOTS_CONTROLLED))
		 {
			return 0;
		 }

		if (robot_fired[slot_num]) {
			return 0;
		}
		else {
			robot_agitation[slot_num] = agitation;
			robot_last_message_time[slot_num] = {GameTime64};
			return 1;
		}
	}
	else if (objrobot.ctype.ai_info.REMOTE_OWNER != -1 || agitation < MIN_TO_ADD)
	{
		if (agitation == ROBOT_FIRE_AGITATION) // Special case for firing at non-player
		{
			return 1; // Try to fire at player even tho we're not in control!
		}
		else
			return 0;
	}
	else
		return multi_add_controlled_robot(objnum, agitation);
}

void multi_check_robot_timeout()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	static fix64 lastcheck = 0;
	int i;

	if (GameTime64 > lastcheck + F1_0 || lastcheck > GameTime64)
	{
		lastcheck = {GameTime64};
		for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++) 
		{
			if (robot_controlled[i] != object_none && robot_last_send_time[i] + ROBOT_TIMEOUT < GameTime64)
			{
				const auto &&robot_objp = vmobjptridx(robot_controlled[i]);
				if (robot_objp->ctype.ai_info.REMOTE_OWNER != Player_num)
				{		
					robot_controlled[i] = object_none;
					Int3(); // Non-terminal but Rob is interesting, step over please...
					return;
				}
				if (robot_send_pending[i] != multi_send_robot_position_priority::_0)
					multi_send_robot_position(robot_objp, multi_send_robot_position_priority::_2);
				multi_send_release_robot(robot_objp);
//				multi_delete_controlled_robot(robot_controlled[i]);
//				robot_controlled[i] = -1;
			}
		}
	}			
}

void multi_strip_robots(const int playernum)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	auto &vmobjptridx = Objects.vmptridx;
	// Grab all robots away from a player 
	// (player died or exited the game)
	if (Game_mode & GM_MULTI_ROBOTS) {
	
		if (playernum == Player_num)
		{
			range_for (const auto r, robot_controlled)
				if (r != object_none)
					multi_delete_controlled_robot(vmobjptridx(r));
		}

		/* clang-3.7 crashes with a segmentation fault if asked to
		 * implicitly convert 1u to std::size_t in partial_range inline
		 * chain.  Add a conversion up front, which seems to avoid the
		 * bug.  The value must not be cast on non-clang targets because
		 * some non-clang targets issue a `-Wuseless-cast` diagnostic
		 * for the cast.  Fortunately, clang does not understand
		 * `-Wuseless-cast`, so the cast can be used on all clang
		 * targets, even ones where it is useless.
		 *
		 * This crash was first seen in x86_64-pc-linux-gnu-clang-3.7,
		 * then later reported by kreator in `Apple LLVM version 7.3.0`
		 * on `x86_64-apple-darwin15.6.0`.
		 *
		 * Comment from kreator regarding the clang crash bug:
		 *	This has been reported with Apple, bug report 28247284
		 */
#ifdef __clang__
#define DXX_partial_range_vobjptr_skip_distance	static_cast<std::size_t>
#else
#define DXX_partial_range_vobjptr_skip_distance
#endif
		range_for (const auto &&objp, partial_range(vmobjptr, DXX_partial_range_vobjptr_skip_distance(1u), vmobjptr.count()))
#undef DXX_partial_range_vobjptr_skip_distance
		{
			auto &obj = *objp;
			if (obj.type == OBJ_ROBOT && obj.ctype.ai_info.REMOTE_OWNER == playernum)
			{
				assert(obj.control_source == object::control_type::ai || obj.control_source == object::control_type::None || obj.control_source == object::control_type::morph);
				obj.ctype.ai_info.REMOTE_OWNER = -1;
				if (playernum == Player_num)
					obj.ctype.ai_info.REMOTE_SLOT_NUM = HANDS_OFF_PERIOD;
				else
					obj.ctype.ai_info.REMOTE_SLOT_NUM = 0;
	  		}
		}
	}
	// Note -- only call this with playernum == Player_num if all other players
	// already know that we are clearing house.  This does not send a release
	// message for each controlled robot!!

}

namespace dsx {
namespace {
int multi_add_controlled_robot(const vmobjptridx_t objnum, int agitation)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	auto &vmobjptridx = Objects.vmptridx;
	int i;
	int lowest_agitation = INT32_MAX; // MAX POSITIVE INT
	int lowest_agitated_bot = -1;
	int first_free_robot = -1;

	// Try to add a new robot to the controlled list, return 1 if added, 0 if not.

	auto &objrobot = *objnum;
#if defined(DXX_BUILD_DESCENT_II)
	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	if (Robot_info[get_robot_id(objrobot)].boss_flag != boss_robot_id::None) // this is a boss, so make sure he gets a slot
		agitation=(agitation*3)+Player_num;  
#endif
	if (objrobot.ctype.ai_info.REMOTE_SLOT_NUM > 0)
	{
		objrobot.ctype.ai_info.REMOTE_SLOT_NUM -= 1;
		return 0;
	}

	for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++)
	{
		if (robot_controlled[i] == object_none || vcobjptr(robot_controlled[i])->type != OBJ_ROBOT) {
			first_free_robot = i;
			break;
		}

		if (robot_last_message_time[i] + ROBOT_TIMEOUT < GameTime64) {
			const auto &&robot_objp = vmobjptridx(robot_controlled[i]);
			if (robot_send_pending[i] != multi_send_robot_position_priority::_0)
				multi_send_robot_position(robot_objp, multi_send_robot_position_priority::_2);
			multi_send_release_robot(robot_objp);
			first_free_robot = i;
			break;
		}

		if (robot_agitation[i] < lowest_agitation && robot_controlled_time[i] + MIN_CONTROL_TIME < GameTime64)
		{
			lowest_agitation = robot_agitation[i];
			lowest_agitated_bot = i;
		}
	}

	if (first_free_robot != -1)  // Room left for new robots
		i = first_free_robot;

	else if ((agitation > lowest_agitation)
#if defined(DXX_BUILD_DESCENT_I)
			 && (lowest_agitation <= MAX_TO_DELETE)
#endif
	) // Replace some old robot with a more agitated one
	{
		const auto &&robot_objp = vmobjptridx(robot_controlled[lowest_agitated_bot]);
		if (robot_send_pending[lowest_agitated_bot] != multi_send_robot_position_priority::_0)
			multi_send_robot_position(robot_objp, multi_send_robot_position_priority::_2);
		multi_send_release_robot(robot_objp);

		i = lowest_agitated_bot;
	}
	else {
		return(0); // Sorry, can't squeeze him in!
	}

	multi_send_claim_robot(objnum);
	robot_controlled[i] = objnum;
	robot_agitation[i] = agitation;
	objrobot.ctype.ai_info.REMOTE_OWNER = Player_num;
	objrobot.ctype.ai_info.REMOTE_SLOT_NUM = i;
	robot_controlled_time[i] = {GameTime64};
	robot_last_send_time[i] = robot_last_message_time[i] = {GameTime64};
	return(1);
}	

void multi_delete_controlled_robot(const vmobjptridx_t objnum)
{
	int i;

	// Delete robot object number objnum from list of controlled robots because it is dead

	for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++)
		if (robot_controlled[i] == objnum)
			break;

	if (i == MAX_ROBOTS_CONTROLLED)
		return;

	auto &objrobot = *objnum;
	if (objrobot.ctype.ai_info.REMOTE_SLOT_NUM != i)
	 {
	  Int3();  // can't release this bot!
	  return;
	 }

	objrobot.ctype.ai_info.REMOTE_OWNER = -1;
	objrobot.ctype.ai_info.REMOTE_SLOT_NUM = 0;
	robot_controlled[i] = object_none;
	robot_send_pending[i] = multi_send_robot_position_priority::_0;
	robot_fired[i] = 0;
}
}
}

struct multi_claim_robot
{
	uint8_t pnum;
	int8_t owner;
	uint16_t robjnum;
};
DEFINE_MULTIPLAYER_SERIAL_MESSAGE(multiplayer_command_t::MULTI_ROBOT_CLAIM, multi_claim_robot, b, (b.pnum, b.owner, b.robjnum));

void multi_send_claim_robot(const vmobjptridx_t objnum)
{
	if (objnum->type != OBJ_ROBOT)
		throw std::runtime_error("claiming non-robot"); // See rob
	// The AI tells us we should take control of this robot. 
	auto r = objnum_local_to_remote(objnum);
	multi_serialize_write(multiplayer_data_priority::_2, multi_claim_robot{static_cast<uint8_t>(Player_num), r.owner, r.objnum});
}

namespace dsx {
namespace {

void multi_send_release_robot(const vmobjptridx_t objnum)
{
	multi_command<multiplayer_command_t::MULTI_ROBOT_RELEASE> multibuf;
	if (objnum->type != OBJ_ROBOT)
	{
		Int3(); // See rob
		return;
	}

	multi_delete_controlled_robot(objnum);

	multibuf[1] = Player_num;
	const auto &&[obj_owner, remote_objnum] = objnum_local_to_remote(objnum);
	multibuf[4] = obj_owner;
	PUT_INTEL_SHORT(&multibuf[2], remote_objnum);

	multi_send_data(multibuf, multiplayer_data_priority::_2);
}

}
}

void multi_send_robot_frame()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	static int last_sent = 0;

	int i;

	const unsigned sent = 0;
	for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++)
	{
		int sending = (last_sent+1+i)%MAX_ROBOTS_CONTROLLED;
		if (robot_controlled[sending] != object_none && (underlying_value(robot_send_pending[sending]) > sent || robot_fired[sending] > sent))
		{
			if (auto &pending = robot_send_pending[sending]; pending != multi_send_robot_position_priority::_0)
			{
				const auto p = std::exchange(pending, multi_send_robot_position_priority::_0);
				multi_send_robot_position_sub(vmobjptridx(robot_controlled[sending]), underlying_value(p) > 1 ? multiplayer_data_priority::_1 : multiplayer_data_priority::_0);
			}

			if (auto &&b = robot_fired[sending])
			{
				b = 0;
				multi_send_data(robot_fire_buf[sending], multiplayer_data_priority::_1);
			}
			last_sent = sending;
		}
	}
	Assert((last_sent >= 0) && (last_sent <= MAX_ROBOTS_CONTROLLED));
}

namespace dsx {
#if defined(DXX_BUILD_DESCENT_II)
/*
 * The thief bot moves around even when not controlled by a player. Due to its erratic and random behaviour, it's movement will diverge heavily between players and cause it to teleport when a player takes over.
 * To counter this, let host update positions when no one controls it OR the client which does.
 * Seperated this function to allow the positions being updated more frequently then multi_send_robot_frame (see dispatch_table::do_protocol_frame()).
 */
void multi_send_thief_frame()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
        if (!(Game_mode & GM_MULTI_ROBOTS))
                return;

        range_for (const auto &&objp, vmobjptridx)
        {
		if (objp->type == OBJ_ROBOT)
                {
			if (robot_is_thief(Robot_info[get_robot_id(objp)]))
                        {
				if ((multi_i_am_master() && objp->ctype.ai_info.REMOTE_OWNER == -1) || objp->ctype.ai_info.REMOTE_OWNER == Player_num)
					multi_send_robot_position_sub(objp, multiplayer_data_priority::_1);
                                return;
                        }
                }
        }
}

#endif

namespace {
void multi_send_robot_position_sub(const vmobjptridx_t objnum, const multiplayer_data_priority priority)
{
	multi_command<multiplayer_command_t::MULTI_ROBOT_POSITION> multibuf;
	int loc = 0;

	loc += 1;
	multibuf[loc] = Player_num;                                            loc += 1;
	const auto &&[obj_owner, remote_objnum] = objnum_local_to_remote(objnum);
	multibuf[loc+2] = obj_owner;
	PUT_INTEL_SHORT(&multibuf[loc], remote_objnum);                                      loc += 3; // 5

	const auto qpp{build_quaternionpos(objnum)};
	PUT_INTEL_SHORT(&multibuf[loc], qpp.orient.w);			loc += 2;
	PUT_INTEL_SHORT(&multibuf[loc], qpp.orient.x);			loc += 2;
	PUT_INTEL_SHORT(&multibuf[loc], qpp.orient.y);			loc += 2;
	PUT_INTEL_SHORT(&multibuf[loc], qpp.orient.z);			loc += 2;
	multi_put_vector(&multibuf[loc], qpp.pos);
	loc += 12;
	PUT_INTEL_SEGNUM(&multibuf[loc], qpp.segment);			loc += 2;
	multi_put_vector(&multibuf[loc], qpp.vel);
	loc += 12;
	multi_put_vector(&multibuf[loc], qpp.rotvel);
	// 46 + 5 = 51

	multi_send_data(multibuf, priority);
}

}
}

void multi_send_robot_position(object &obj, const multi_send_robot_position_priority force)
{
	// Send robot position to other player(s).  Includes a byte
	// value describing whether or not they fired a weapon

	if (!(Game_mode & GM_MULTI))
		return;

	if (obj.type != OBJ_ROBOT)
	{
		Int3(); // See rob
		return;
	}

	if (obj.ctype.ai_info.REMOTE_OWNER != Player_num)
		return;

//	Objects[objnum].phys_info.drag = Robot_info[Objects[objnum].id].drag; // Set drag to normal

	const auto slot = obj.ctype.ai_info.REMOTE_SLOT_NUM;
	robot_last_send_time[slot] = {GameTime64};
	robot_send_pending[slot] = force;
	return;
}

void multi_send_robot_fire(const vmobjptridx_t obj, const robot_gun_number gun_num, const vms_vector &fire)
{
	multi_command<multiplayer_command_t::MULTI_ROBOT_FIRE> multibuf;
        // Send robot fire event
        int loc = 0;

                                                                        loc += 1;
        multibuf[loc] = Player_num;                                     loc += 1;
	const auto &&[obj_owner, remote_objnum] = objnum_local_to_remote(obj);
	multibuf[loc+2] = obj_owner;
	PUT_INTEL_SHORT(&multibuf[loc], remote_objnum);
                                                                        loc += 3;
        multibuf[loc] = underlying_value(gun_num);                                        loc += 1;
		multi_put_vector(&multibuf[loc], fire);
                                                                        loc += sizeof(vms_vector); // 12
                                                                        // --------------------------
                                                                        //      Total = 18

        if (obj->ctype.ai_info.REMOTE_OWNER == Player_num)
        {
                const auto slot = obj->ctype.ai_info.REMOTE_SLOT_NUM;
                if (slot<0 || slot>=MAX_ROBOTS_CONTROLLED)
                {
                        return;
                }
		if (auto &&b = robot_fired[slot])
                {
                        // Int3(); // ROB!
                        return;
                }
		else
		{
			b = 1;
		}
			robot_fire_buf[slot] = multibuf;
        }
        else
                multi_send_data(multibuf, multiplayer_data_priority::_1); // Not our robot, send ASAP
}

struct multi_explode_robot
{
	int16_t robj_killer, robj_killed;
	int8_t owner_killer, owner_killed;
};
DEFINE_MULTIPLAYER_SERIAL_MESSAGE(multiplayer_command_t::MULTI_ROBOT_EXPLODE, multi_explode_robot, b, (b.robj_killer, b.robj_killed, b.owner_killer, b.owner_killed));

void multi_send_robot_explode(const imobjptridx_t objnum, objnum_t killer)
{
	// Send robot explosion event to the other players
	const auto k = objnum_local_to_remote(killer);
	multi_explode_robot e;
	e.robj_killer = k.objnum;
	e.owner_killer = k.owner;
	const auto d = objnum_local_to_remote(objnum);
	e.robj_killed = d.objnum;
	e.owner_killed = d.owner;
	multi_serialize_write(multiplayer_data_priority::_2, e);

	multi_delete_controlled_robot(objnum);
}

void multi_send_create_robot(const station_number station, const objnum_t objnum, const robot_id type)
{
	multi_command<multiplayer_command_t::MULTI_CREATE_ROBOT> multibuf;
	// Send create robot information

	int loc = 0;

	loc += 1;
	multibuf[loc] = Player_num;								loc += 1;
	static_assert(sizeof(underlying_value(station)) == 1);
	multibuf[loc] = underlying_value(station);                         loc += 1;
	PUT_INTEL_SHORT(&multibuf[loc], objnum);                  loc += 2;
	multibuf[loc] = underlying_value(type);					loc += 1;

	map_objnum_local_to_local(objnum);

	multi_send_data(multibuf, multiplayer_data_priority::_2);
}

namespace {

struct boss_teleport
{
	objnum_t objnum;
	segnum_t where;
};
DEFINE_MULTIPLAYER_SERIAL_MESSAGE(multiplayer_command_t::MULTI_BOSS_TELEPORT, boss_teleport, b, (b.objnum, b.where));

struct boss_cloak
{
	objnum_t objnum;
};
DEFINE_MULTIPLAYER_SERIAL_MESSAGE(multiplayer_command_t::MULTI_BOSS_CLOAK, boss_cloak, b, (b.objnum));

struct boss_start_gate
{
	objnum_t objnum;
};
DEFINE_MULTIPLAYER_SERIAL_MESSAGE(multiplayer_command_t::MULTI_BOSS_START_GATE, boss_start_gate, b, (b.objnum));

struct boss_stop_gate
{
	objnum_t objnum;
};
DEFINE_MULTIPLAYER_SERIAL_MESSAGE(multiplayer_command_t::MULTI_BOSS_STOP_GATE, boss_stop_gate, b, (b.objnum));

struct boss_create_robot
{
	objnum_t objnum;
	objnum_t objrobot;
	segnum_t where;
	uint8_t robot_type;
};
DEFINE_MULTIPLAYER_SERIAL_MESSAGE(multiplayer_command_t::MULTI_BOSS_CREATE_ROBOT, boss_create_robot, b, (b.objnum, b.objrobot, b.where, b.robot_type));

#if defined(DXX_BUILD_DESCENT_II)
struct update_buddy_state
{
	uint8_t Looking_for_marker;
	escort_goal_t Escort_special_goal;
	int Last_buddy_key;
};
DEFINE_MULTIPLAYER_SERIAL_MESSAGE(multiplayer_command_t::MULTI_UPDATE_BUDDY_STATE, update_buddy_state, b, (b.Looking_for_marker, b.Escort_special_goal, b.Last_buddy_key));
#endif

}

namespace {

template <typename T, typename... Args>
static inline void multi_send_boss_action(objnum_t bossobjnum, Args&&... args)
{
	multi_serialize_write(multiplayer_data_priority::_2, T{bossobjnum, std::forward<Args>(args)...});
}

}

namespace dsx {
void multi_send_boss_teleport(const vmobjptridx_t bossobj, const vcsegidx_t where)
{
	// Boss is up for grabs after teleporting
	Assert((bossobj->ctype.ai_info.REMOTE_SLOT_NUM >= 0) && (bossobj->ctype.ai_info.REMOTE_SLOT_NUM < MAX_ROBOTS_CONTROLLED));
	multi_delete_controlled_robot(bossobj);
#if defined(DXX_BUILD_DESCENT_I)
	bossobj->ctype.ai_info.REMOTE_SLOT_NUM = HANDS_OFF_PERIOD; // Hands-off period!
#endif
	multi_send_boss_action<boss_teleport>(bossobj, where);
}
}

void multi_send_boss_cloak(objnum_t bossobjnum)
{
	multi_send_boss_action<boss_cloak>(bossobjnum);
}

void multi_send_boss_start_gate(objnum_t bossobjnum)
{
	multi_send_boss_action<boss_start_gate>(bossobjnum);
}

void multi_send_boss_stop_gate(objnum_t bossobjnum)
{
	multi_send_boss_action<boss_stop_gate>(bossobjnum);
}

void multi_send_boss_create_robot(vmobjidx_t bossobjnum, const vmobjptridx_t objrobot)
{
	map_objnum_local_to_local(objrobot);
	multi_send_boss_action<boss_create_robot>(bossobjnum, objrobot, objrobot->segnum, underlying_value(get_robot_id(objrobot)));
}

#define MAX_ROBOT_POWERUPS 4

namespace dsx {
namespace {

static void multi_send_create_robot_powerups(const object_base &del_obj)
{
	multi_command<multiplayer_command_t::MULTI_CREATE_ROBOT_POWERUPS> multibuf;
	// Send create robot information

	int loc = 0;

	loc += 1;
	multibuf[loc] = Player_num;									loc += 1;
	multibuf[loc] = del_obj.contains.count;					loc += 1;
	multibuf[loc] = underlying_value(del_obj.contains.type);	loc += 1;
	multibuf[loc] = underlying_value(del_obj.contains.id.robot);	loc += 1;
	PUT_INTEL_SEGNUM(&multibuf[loc], del_obj.segnum);		        loc += 2;
	multi_put_vector(&multibuf[loc], del_obj.pos);
	loc += 12;

	memset(&multibuf[loc], -1, MAX_ROBOT_POWERUPS*sizeof(short));
#if defined(DXX_BUILD_DESCENT_II)
   if (del_obj.contains.count != Net_create_loc)
	  Int3();  //Get Jason, a bad thing happened
#endif

	if ((Net_create_loc > MAX_ROBOT_POWERUPS) || (Net_create_loc < 1))
	{
		Int3(); // See Rob
	}
	range_for (const auto i, partial_const_range(Net_create_objnums, Net_create_loc))
	{
		PUT_INTEL_SHORT(&multibuf[loc], i);
		loc += 2;
		map_objnum_local_to_local(i);
	}

	Net_create_loc = 0;

	multi_send_data(multibuf, multiplayer_data_priority::_2);
}
}

void multi_do_claim_robot(const playernum_t pnum, const multiplayer_rspan<multiplayer_command_t::MULTI_ROBOT_CLAIM> buf)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	multi_claim_robot b;
	multi_serialize_read(buf, b);
	auto botnum = objnum_remote_to_local(b.robjnum, b.owner);
	if (botnum > Highest_object_index)
	{
		return;
	}

	const auto &&botp = vmobjptridx(botnum);
	if (botp->type != OBJ_ROBOT)
	{
		return;
	}
	
	if (botp->ctype.ai_info.REMOTE_OWNER != -1)
	{
		if (MULTI_ROBOT_PRIORITY(b.robjnum, pnum) <= MULTI_ROBOT_PRIORITY(b.robjnum, botp->ctype.ai_info.REMOTE_OWNER))
			return;
	}
	
	// Perform the requested change

	if (botp->ctype.ai_info.REMOTE_OWNER == Player_num)
	{
		multi_delete_controlled_robot(botp);
	}

	botp->ctype.ai_info.REMOTE_OWNER = pnum;
	botp->ctype.ai_info.REMOTE_SLOT_NUM = 0;
}

void multi_do_release_robot(const playernum_t pnum, const multiplayer_rspan<multiplayer_command_t::MULTI_ROBOT_RELEASE> buf)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	short remote_botnum;

	remote_botnum = GET_INTEL_SHORT(&buf[2]);
	auto botnum = objnum_remote_to_local(remote_botnum, buf[4]);

	if (botnum > Highest_object_index)
	{
		return;
	}

	const auto &&botp = vmobjptr(botnum);
	if (botp->type != OBJ_ROBOT)
	{
		return;
	}
	
	if (botp->ctype.ai_info.REMOTE_OWNER != pnum)
	{
		return;
	}
	
	// Perform the requested change

	botp->ctype.ai_info.REMOTE_OWNER = -1;
	botp->ctype.ai_info.REMOTE_SLOT_NUM = 0;
}

void multi_do_robot_position(const playernum_t pnum, const multiplayer_rspan<multiplayer_command_t::MULTI_ROBOT_POSITION> buf)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	// Process robot movement sent by another player

	short remote_botnum;
	int loc = 1;

	;										loc += 1;

	remote_botnum = GET_INTEL_SHORT(&buf[loc]);
	auto botnum = objnum_remote_to_local(remote_botnum, buf[loc+2]); loc += 3;

	if (botnum > Highest_object_index)
	{
		return;
	}

	const auto robot = vmobjptridx(botnum);

	if ((robot->type != OBJ_ROBOT) || (robot->flags & OF_EXPLODING)) {
		return;
	}

	if (robot->ctype.ai_info.REMOTE_OWNER != pnum)
	{	
		if (robot->ctype.ai_info.REMOTE_OWNER == -1)
		{
			// Robot claim packet must have gotten lost, let this player claim it.
			if (robot->ctype.ai_info.REMOTE_SLOT_NUM >= MAX_ROBOTS_CONTROLLED) { // == HANDS_OFF_PERIOD should do the same trick
				robot->ctype.ai_info.REMOTE_OWNER = pnum;
				robot->ctype.ai_info.REMOTE_SLOT_NUM = 0;
			}
			else
				robot->ctype.ai_info.REMOTE_SLOT_NUM++;
		}
		else
		{
			return;
		}
	}

	set_thrust_from_velocity(robot); // Try to smooth out movement
//	Objects[botnum].phys_info.drag = Robot_info[Objects[botnum].id].drag >> 4; // Set drag to low

	quaternionpos qpp{};
	qpp.orient.w = GET_INTEL_SHORT(&buf[loc]);					loc += 2;
	qpp.orient.x = GET_INTEL_SHORT(&buf[loc]);					loc += 2;
	qpp.orient.y = GET_INTEL_SHORT(&buf[loc]);					loc += 2;
	qpp.orient.z = GET_INTEL_SHORT(&buf[loc]);					loc += 2;
	qpp.pos = multi_get_vector(buf.subspan<5 + 8, 12>());
	loc += 12;
	if (const auto s{vmsegidx_t::check_nothrow_index(GET_INTEL_SHORT(&buf[loc]))})
	{
		qpp.segment = *s;
		loc += 2;
	}
	else
		return;
	qpp.vel = multi_get_vector(buf.subspan<5 + 8 + 12 + 2, 12>());
	loc += 12;
	qpp.rotvel = multi_get_vector(buf.subspan<5 + 8 + 12 + 2 + 12, 12>());
	extract_quaternionpos(robot, qpp);
}

}

static inline vms_vector calc_gun_point(const robot_info &robptr, const object_base &obj, const robot_gun_number gun_num)
{
	vms_vector v;
	return calc_gun_point(robptr, v, obj, gun_num), v;
}

namespace dsx {
void multi_do_robot_fire(const multiplayer_rspan<multiplayer_command_t::MULTI_ROBOT_FIRE> buf)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	// Send robot fire event
	int loc = 1;
                                                                                        loc += 1; // pnum
	const short remote_botnum = GET_INTEL_SHORT(&buf[loc]);
	auto botnum = objnum_remote_to_local(remote_botnum, buf[loc+2]);                loc += 3;
	const auto gun_num = buf[loc];                                                      loc += 1;
	const auto fire = multi_get_vector(buf.subspan<6, 12>());

	if (botnum > Highest_object_index)
		return;

	auto botp = vmobjptridx(botnum);
	if (botp->type != OBJ_ROBOT || botp->flags & OF_EXPLODING)
		return;

	using pt_weapon = std::pair<vms_vector, weapon_id_type>;
	auto &robptr = Robot_info[get_robot_id(botp)];
	switch (gun_num)
	{
		case underlying_value(robot_gun_number::proximity):
#if defined(DXX_BUILD_DESCENT_II)
		case underlying_value(robot_gun_number::smart_mine):
#endif
			break;
		default:
			if (gun_num >= robptr.n_guns)
				return;
			break;
	}
	const pt_weapon pw =
	// Do the firing
		(gun_num == underlying_value(robot_gun_number::proximity)
#if defined(DXX_BUILD_DESCENT_II)
		|| gun_num == underlying_value(robot_gun_number::smart_mine)
#endif
		)
		? pt_weapon(vm_vec_add(botp->pos, fire), 
#if defined(DXX_BUILD_DESCENT_II)
			gun_num != underlying_value(robot_gun_number::proximity) ? weapon_id_type::SUPERPROX_ID :
#endif
			weapon_id_type::PROXIMITY_ID)
		: pt_weapon(calc_gun_point(robptr, botp, robot_gun_number{gun_num}), get_robot_weapon(robptr, robot_gun_number::_1));
	Laser_create_new_easy(Robot_info, fire, pw.first, botp, pw.second, weapon_sound_flag::audible);
}

int multi_explode_robot_sub(const d_robot_info_array &Robot_info, const vmobjptridx_t robot)
{
	auto &BossUniqueState = LevelUniqueObjectState.BossState;
	auto &objrobot = *robot;
	if (objrobot.type != OBJ_ROBOT) { // Object is robot?
		return 0;
	}

	if (objrobot.flags & OF_EXPLODING) { // Object not already exploding
		return 0;
	}

	// Data seems valid, explode the sucker

	if (Network_send_objects && multi::dispatch->objnum_is_past(robot))
	{
		Network_send_objnum = -1;
	}

	// Drop non-random KEY powerups locally only!
	if (objrobot.contains.count > 0 && objrobot.contains.type == contained_object_type::powerup && (Game_mode & GM_MULTI_COOP) && objrobot.contains.id.powerup >= powerup_type_t::POW_KEY_BLUE && objrobot.contains.id.powerup <= powerup_type_t::POW_KEY_GOLD)
	{
		object_create_robot_egg(Robot_info, objrobot);
	}
	else if (objrobot.ctype.ai_info.REMOTE_OWNER == Player_num)
	{
		multi_drop_robot_powerups(robot);
		multi_delete_controlled_robot(robot);
	}
	else if (objrobot.ctype.ai_info.REMOTE_OWNER == -1 && multi_i_am_master())
	{
		multi_drop_robot_powerups(robot);
	}
	const auto robot_id = get_robot_id(objrobot);
#if defined(DXX_BUILD_DESCENT_II)
	if (robot_is_thief(Robot_info[robot_id]))
		drop_stolen_items_local(LevelUniqueObjectState, LevelSharedSegmentState, LevelUniqueSegmentState, Vclip, vmsegptridx(objrobot.segnum), objrobot.mtype.phys_info.velocity, objrobot.pos, LevelUniqueObjectState.ThiefState.Stolen_items);
#endif
	if (Robot_info[robot_id].boss_flag != boss_robot_id::None)
	{
		if (!BossUniqueState.Boss_dying)
			start_boss_death_sequence(LevelUniqueObjectState.BossState, Robot_info, robot);
		else
			return (0);
	}
#if defined(DXX_BUILD_DESCENT_II)
	else if (Robot_info[robot_id].death_roll) {
		start_robot_death_sequence(robot);
	}
#endif
	else
	{
#if defined(DXX_BUILD_DESCENT_II)
		if (robot_id == robot_id::special_reactor)
			special_reactor_stuff();
#endif
		// Kamikaze, explode right away, IN YOUR FACE!
		explode_object(LevelUniqueObjectState, Robot_info, LevelSharedSegmentState, LevelUniqueSegmentState, robot,
#if defined(DXX_BUILD_DESCENT_II)
					   Robot_info[robot_id].kamikaze ? 1 :
#endif
					   STANDARD_EXPL_DELAY);
   }
   
	return 1;
}

void multi_do_robot_explode(const d_robot_info_array &Robot_info, const multiplayer_rspan<multiplayer_command_t::MULTI_ROBOT_EXPLODE> buf)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	multi_explode_robot b;
	multi_serialize_read(buf, b);
	auto killer = objnum_remote_to_local(b.robj_killer, b.owner_killer);
	auto botnum = objnum_remote_to_local(b.robj_killed, b.owner_killed);
	// Explode robot controlled by other player
	if (botnum > Highest_object_index)
	{
		return;
	}

	const auto robot = vmobjptridx(botnum);
	const auto rval = multi_explode_robot_sub(Robot_info, robot);
	if (!rval)
		return;

	++ Players[0u].num_kills_level;
	++ Players[0u].num_kills_total;
	if (killer == get_local_player().objnum)
		add_points_to_score(ConsoleObject->ctype.player_info, Robot_info[get_robot_id(robot)].score_value, Game_mode);
}

void multi_do_create_robot(const d_robot_info_array &Robot_info, const d_vclip_array &Vclip, const playernum_t pnum, const multiplayer_rspan<multiplayer_command_t::MULTI_CREATE_ROBOT> buf)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &LevelUniqueMorphObjectState = LevelUniqueObjectState.MorphObjectState;
	auto &Station = LevelUniqueFuelcenterState.Station;
	auto &Vertices = LevelSharedVertexState.get_vertices();
	const auto untrusted_fuelcen_num = buf[2];
	const auto untrusted_robot_type = buf[5];

	objnum_t objnum;
	objnum = GET_INTEL_SHORT(&buf[3]);

	const auto trusted_fuelcen_num = ({
		const auto o = LevelUniqueFuelcenterState.Station.valid_index(untrusted_fuelcen_num);
		if (!o)
			return;
		if (untrusted_fuelcen_num >= LevelUniqueFuelcenterState.Num_fuelcenters || pnum >= N_players)
		{
			Int3(); // Bogus data
			return;
		}
		*o;
	});
	const auto trusted_robot_type = ({
		const auto o = LevelSharedRobotInfoState.Robot_info.valid_index(untrusted_robot_type);
		if (!o)
			return;
		*o;
	});
	auto &robotcen = Station[trusted_fuelcen_num];

	// Play effect and sound

	// Set robot center flags, in case we become the master for the next one

	robotcen.Flag = 0;
	robotcen.Capacity -= EnergyToCreateOneRobot;
	robotcen.Timer = 0;

	const auto &&robotcen_segp = vmsegptridx(robotcen.segnum);
	auto &vcvertptr = Vertices.vcptr;
	const auto cur_object_loc{compute_segment_center(vcvertptr, robotcen_segp)};
	if (const auto &&obj = object_create_explosion_without_damage(Vclip, robotcen_segp, cur_object_loc, i2f(10), vclip_index::morphing_robot))
		extract_orient_from_segment(vcvertptr, obj->orient, robotcen_segp);
	digi_link_sound_to_pos(Vclip[vclip_index::morphing_robot].sound_num, robotcen_segp, sidenum_t::WLEFT, cur_object_loc, 0, F1_0);

	const auto &&obj = create_morph_robot(Robot_info, robotcen_segp, cur_object_loc, trusted_robot_type);
	if (obj == object_none)
		return; // Cannot create object!
	
	obj->matcen_creator = untrusted_fuelcen_num | 0x80;
//	extract_orient_from_segment(&obj->orient, &Segments[robotcen->segnum]);
	const auto direction = vm_vec_sub(ConsoleObject->pos, obj->pos );
	vm_vector_2_matrix( obj->orient, direction, &obj->orient.uvec, nullptr);
	morph_start(LevelUniqueMorphObjectState, LevelSharedPolygonModelState, obj);

	map_objnum_local_to_remote(obj, objnum, pnum);

	Assert(obj->ctype.ai_info.REMOTE_OWNER == -1);
}

#if defined(DXX_BUILD_DESCENT_II)
void multi_send_escort_goal(const d_unique_buddy_state &BuddyState)
{
	update_buddy_state b;
	b.Looking_for_marker = static_cast<uint8_t>(BuddyState.Looking_for_marker);
	b.Escort_special_goal = BuddyState.Escort_special_goal;
	b.Last_buddy_key = BuddyState.Last_buddy_key;
	multi_serialize_write(multiplayer_data_priority::_2, b);
}

void multi_recv_escort_goal(d_unique_buddy_state &BuddyState, const multiplayer_rspan<multiplayer_command_t::MULTI_UPDATE_BUDDY_STATE> buf)
{
	update_buddy_state b;
	multi_serialize_read(buf, b);
	const auto Looking_for_marker = b.Looking_for_marker;
	BuddyState.Looking_for_marker = ({
		const auto o = MarkerState.imobjidx.valid_index(Looking_for_marker);
		o ? *o : game_marker_index::None;
	});
	BuddyState.Escort_special_goal = b.Escort_special_goal;
	BuddyState.Last_buddy_key = b.Last_buddy_key;
	BuddyState.Buddy_messages_suppressed = 0;
	BuddyState.Last_buddy_message_time = GameTime64 - 2 * F1_0;
	BuddyState.Escort_goal_object = ESCORT_GOAL_UNSPECIFIED;
}
#endif

void multi_do_boss_teleport(const d_robot_info_array &Robot_info, const d_vclip_array &Vclip, const playernum_t pnum, const multiplayer_rspan<multiplayer_command_t::MULTI_BOSS_TELEPORT> buf)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &BossUniqueState = LevelUniqueObjectState.BossState;
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vcobjptr = Objects.vcptr;
	auto &vmobjptr = Objects.vmptr;
	boss_teleport b;
	multi_serialize_read(buf, b);
	const auto &&guarded_boss_obj = Objects.vmptridx.check_untrusted(b.objnum);
	if (!guarded_boss_obj)
		return;
	const auto &&boss_obj = *guarded_boss_obj;
	if (boss_obj->type != OBJ_ROBOT || Robot_info[get_robot_id(boss_obj)].boss_flag == boss_robot_id::None)
	{
		Int3(); // Got boss actions for a robot who's not a boss?
		return;
	}
	const auto &&guarded_teleport_segnum = vmsegptridx.check_untrusted(b.where);
	if (!guarded_teleport_segnum)
		return;
	const auto &&teleport_segnum = *guarded_teleport_segnum;
	auto &vcvertptr = Vertices.vcptr;
	boss_obj->pos = compute_segment_center(vcvertptr, teleport_segnum);
	obj_relink(vmobjptr, vmsegptr, boss_obj, teleport_segnum);
	BossUniqueState.Last_teleport_time = {GameTime64};

	const auto boss_dir = vm_vec_sub(vcobjptr(vcplayerptr(pnum)->objnum)->pos, boss_obj->pos);
	vm_vector_2_matrix(boss_obj->orient, boss_dir, nullptr, nullptr);

	digi_link_sound_to_pos(Vclip[vclip_index::morphing_robot].sound_num, teleport_segnum, sidenum_t::WLEFT, boss_obj->pos, 0, F1_0);
	digi_kill_sound_linked_to_object( boss_obj);
	boss_link_see_sound(Robot_info, boss_obj);
	ai_local		*ailp = &boss_obj->ctype.ai_info.ail;
	ailp->next_fire = 0;

	if (boss_obj->ctype.ai_info.REMOTE_OWNER == Player_num)
	{
		multi_delete_controlled_robot(boss_obj);
	}

	boss_obj->ctype.ai_info.REMOTE_OWNER = -1; // Boss is up for grabs again!
	boss_obj->ctype.ai_info.REMOTE_SLOT_NUM = 0; // Available immediately!
}

void multi_do_boss_cloak(const multiplayer_rspan<multiplayer_command_t::MULTI_BOSS_CLOAK> buf)
{
	auto &BossUniqueState = LevelUniqueObjectState.BossState;
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	boss_cloak b;
	multi_serialize_read(buf, b);
	const auto &&guarded_boss_obj = vmobjptridx.check_untrusted(b.objnum);
	if (!guarded_boss_obj)
		return;
	const auto &&boss_obj = *guarded_boss_obj;
	if (boss_obj->type != OBJ_ROBOT || Robot_info[get_robot_id(boss_obj)].boss_flag == boss_robot_id::None)
	{
		Int3(); // Got boss actions for a robot who's not a boss?
		return;
	}
	BossUniqueState.Boss_hit_this_frame = 0;
#if defined(DXX_BUILD_DESCENT_II)
	BossUniqueState.Boss_hit_time = -F1_0*10;
#endif
	BossUniqueState.Boss_cloak_start_time = {GameTime64};
	boss_obj->ctype.ai_info.CLOAKED = 1;
}

void multi_do_boss_start_gate(const multiplayer_rspan<multiplayer_command_t::MULTI_BOSS_START_GATE> buf)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	boss_start_gate b;
	multi_serialize_read(buf, b);
	const auto &&guarded_boss_obj = vmobjptridx.check_untrusted(b.objnum);
	if (!guarded_boss_obj)
		return;
	const object_base &boss_obj = *guarded_boss_obj;
	if (boss_obj.type != OBJ_ROBOT || Robot_info[get_robot_id(boss_obj)].boss_flag == boss_robot_id::None)
	{
		Int3(); // Got boss actions for a robot who's not a boss?
		return;
	}
	restart_effect(ECLIP_NUM_BOSS);
}

void multi_do_boss_stop_gate(const multiplayer_rspan<multiplayer_command_t::MULTI_BOSS_STOP_GATE> buf)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	boss_start_gate b;
	multi_serialize_read(buf, b);
	const auto &&guarded_boss_obj = vmobjptridx.check_untrusted(b.objnum);
	if (!guarded_boss_obj)
		return;
	const object_base &boss_obj = *guarded_boss_obj;
	if (boss_obj.type != OBJ_ROBOT || Robot_info[get_robot_id(boss_obj)].boss_flag == boss_robot_id::None)
	{
		Int3(); // Got boss actions for a robot who's not a boss?
		return;
	}
	stop_effect(ECLIP_NUM_BOSS);
}

void multi_do_boss_create_robot(const playernum_t pnum, const multiplayer_rspan<multiplayer_command_t::MULTI_BOSS_CREATE_ROBOT> buf)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	boss_create_robot b;
	multi_serialize_read(buf, b);
	const auto &&guarded_boss_obj = vmobjptridx.check_untrusted(b.objnum);
	if (!guarded_boss_obj)
		return;
	const object_base &boss_obj = *guarded_boss_obj;
	if (boss_obj.type != OBJ_ROBOT || Robot_info[get_robot_id(boss_obj)].boss_flag == boss_robot_id::None)
	{
		Int3(); // Got boss actions for a robot who's not a boss?
		return;
	}
	// Do some validity checking
	if (b.objrobot >= MAX_OBJECTS)
	{
		Int3(); // See Rob, bad data in boss gate action message
		return;
	}
	if (const auto trusted_robot_type = LevelSharedRobotInfoState.Robot_info.valid_index(b.robot_type); !trusted_robot_type)
		return;
	// Gate one in!
	else if (const auto &&robot = gate_in_robot(Robot_info, *trusted_robot_type, vmsegptridx(b.where)); robot != object_none)
		map_objnum_local_to_remote(robot, b.objrobot, pnum);
}

void multi_do_create_robot_powerups(const playernum_t pnum, const multiplayer_rspan<multiplayer_command_t::MULTI_CREATE_ROBOT_POWERUPS> buf)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	// Code to drop remote-controlled robot powerups

	int loc = 1;
	;					loc += 1;
	const uint8_t untrusted_contains_count{buf[loc]};			loc += 1;
	if (untrusted_contains_count == 0)
		return;
	const uint8_t untrusted_contains_type = buf[loc];			loc += 1;
	const uint8_t untrusted_contains_id = buf[loc];				loc += 1;
	const auto segnum{vmsegidx_t::check_nothrow_index(GET_INTEL_SHORT(&buf[loc]))};	loc += 2;
	if (!segnum)
		return;
	const auto pos = multi_get_vector(buf.subspan<7, 12>());
	loc += 12;
	vms_vector velocity{};

	Assert(pnum < N_players);
	Assert (pnum!=Player_num); // What? How'd we send ourselves this?

	Net_create_loc = 0;
	d_srand(1245L);

	if (const auto contains_parameters{build_contained_object_parameters_from_untrusted(untrusted_contains_type, untrusted_contains_id, untrusted_contains_count)}; !object_create_robot_egg(LevelSharedRobotInfoState.Robot_info, contains_parameters.type, contains_parameters.id, contains_parameters.count, velocity, pos, vmsegptridx(*segnum)))
		return; // Object buffer full

	Assert((Net_create_loc > 0) && (Net_create_loc <= MAX_ROBOT_POWERUPS));

	range_for (const auto i, partial_const_range(Net_create_objnums, Net_create_loc))
	{
		short s;
		
		s = GET_INTEL_SHORT(&buf[loc]);
		if ( s != -1)
			map_objnum_local_to_remote(i, s, pnum);
		else
			vmobjptr(i)->flags |= OF_SHOULD_BE_DEAD; // Delete objects other guy didn't create one of
		loc += 2;
	}
}

namespace {

void multi_drop_robot_powerups(object &del_obj)
{
	// Code to handle dropped robot powerups in network mode ONLY!
	if (del_obj.type != OBJ_ROBOT)
	{
		Int3(); // dropping powerups for non-robot, Rob's fault
		return;
	}
	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	auto &robptr = Robot_info[get_robot_id(del_obj)];

	Net_create_loc = 0;

	bool created = false;
	if (del_obj.contains.count > 0)
	{
		//	If dropping a weapon that the player has, drop energy instead, unless it's vulcan, in which case drop vulcan ammo.
		if (del_obj.contains.type == contained_object_type::powerup) {
			maybe_replace_powerup_with_energy(del_obj);
			if (auto &powerup = del_obj.contains.id.powerup; multi_powerup_is_allowed(powerup, Netgame.AllowedItems) == netflag_flag::None)
				powerup = powerup_type_t::POW_SHIELD_BOOST;

			// No key drops in non-coop games!
			if (!(Game_mode & GM_MULTI_COOP)) {
				if (del_obj.contains.id.powerup >= powerup_type_t::POW_KEY_BLUE && del_obj.contains.id.powerup <= powerup_type_t::POW_KEY_GOLD)
					return;
			}
		}
		d_srand(1245L);
		if (del_obj.contains.count > 0)
			created = object_create_robot_egg(LevelSharedRobotInfoState.Robot_info, del_obj);
	}
		
	else if (del_obj.ctype.ai_info.REMOTE_OWNER == -1) // No random goodies for robots we weren't in control of
		return;

	else if (robptr.contains.count)
	{
		d_srand(static_cast<fix>(timer_query()));
		if (((d_rand() * 16) >> 15) < robptr.contains_prob) {
			del_obj.contains = robptr.contains;
			del_obj.contains.count = ((d_rand() * robptr.contains.count) >> 15) + 1;
			if (del_obj.contains.type == contained_object_type::powerup)
			 {
				maybe_replace_powerup_with_energy(del_obj);
				if (multi_powerup_is_allowed(del_obj.contains.id.powerup, Netgame.AllowedItems) == netflag_flag::None)
					del_obj.contains.id.powerup = powerup_type_t::POW_SHIELD_BOOST;
			 }
		
			d_srand(1245L);
			if (del_obj.contains.count > 0)
				created = object_create_robot_egg(LevelSharedRobotInfoState.Robot_info, del_obj);
		}
	}
	if (created)
		// Transmit the object creation to the other players	 	
		multi_send_create_robot_powerups(del_obj);
}

}

//	-----------------------------------------------------------------------------
//	Robot *robot got whacked by player player_num and requests permission to do something about it.
//	Note: This function will be called regardless of whether Game_mode is a multiplayer mode, so it
//	should quick-out if not in a multiplayer mode.  On the other hand, it only gets called when a
//	player or player weapon whacks a robot, so it happens rarely.
void multi_robot_request_change(const vmobjptridx_t robot, int player_num)
{
	if (!(Game_mode & GM_MULTI_ROBOTS))
		return;
	auto &objrobot = *robot;
#if defined(DXX_BUILD_DESCENT_I)
	if (objrobot.ctype.ai_info.REMOTE_OWNER != Player_num)
		return;
#endif

	const auto slot = objrobot.ctype.ai_info.REMOTE_SLOT_NUM;

	if (slot == HANDS_OFF_PERIOD)
	{
		con_printf(CON_DEBUG, "Suppressing debugger trap for hands off robot %hu with player %i", robot.get_unchecked_index(), player_num);
		return;
	}
	if ((slot < 0) || (slot >= MAX_ROBOTS_CONTROLLED)) {
		Int3();
		return;
	}
	if (robot_controlled[slot] == object_none)
		return;
	const auto &&rcrobot = robot.absolute_sibling(robot_controlled[slot]);
	const auto remote_objnum = objnum_local_to_remote(robot).objnum;
	if (remote_objnum >= MAX_OBJECTS)
		return;

	if ( (robot_agitation[slot] < 70) || (MULTI_ROBOT_PRIORITY(remote_objnum, player_num) > MULTI_ROBOT_PRIORITY(remote_objnum, Player_num)) || (d_rand() > 0x4400))
	{
		if (robot_send_pending[slot] != multi_send_robot_position_priority::_0)
			multi_send_robot_position(rcrobot, multi_send_robot_position_priority::_0);
		multi_send_release_robot(rcrobot);
		objrobot.ctype.ai_info.REMOTE_SLOT_NUM = HANDS_OFF_PERIOD;  // Hands-off period
	}
}

}
