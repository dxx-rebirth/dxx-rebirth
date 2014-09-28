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
#include "multiinternal.h"

#include <string.h>
#include <stdlib.h>

#include "vecmat.h"
#include "object.h"
#include "multibot.h"
#include "game.h"
#include "multi.h"
#include "laser.h"
#include "dxxerror.h"
#include "timer.h"
#include "text.h"
#include "ai.h"
#include "fireball.h"
#include "aistruct.h"
#include "gameseg.h"
#include "robot.h"
#include "powerup.h"
#include "scores.h"
#include "gauges.h"
#include "fuelcen.h"
#include "morph.h"
#include "digi.h"
#include "sounds.h"
#include "effects.h"
#include "physics.h" 
#include "byteutil.h"
#include "escort.h"

#include "compiler-range_for.h"

static int multi_add_controlled_robot(vobjptridx_t objnum, int agitation);
static void multi_send_robot_position_sub(vobjptridx_t objnum, int now);
static void multi_send_release_robot(vobjptridx_t objnum);
static void multi_delete_controlled_robot(vobjptridx_t objnum);

//
// Code for controlling robots in multiplayer games
//

#define STANDARD_EXPL_DELAY (F1_0/4)
#if defined(DXX_BUILD_DESCENT_I)
#define MIN_CONTROL_TIME	F1_0*2
#define ROBOT_TIMEOUT		F1_0*3

static inline int multi_powerup_is_allowed(int id)
{
	(void)id;
	return 1;
}
#define MAX_TO_DELETE	67
#elif defined(DXX_BUILD_DESCENT_II)
#define MIN_CONTROL_TIME	F1_0*1
#define ROBOT_TIMEOUT		F1_0*2
#endif

#define MIN_TO_ADD	60

objnum_t robot_controlled[MAX_ROBOTS_CONTROLLED];
int robot_agitation[MAX_ROBOTS_CONTROLLED];
fix64 robot_controlled_time[MAX_ROBOTS_CONTROLLED];
fix64 robot_last_send_time[MAX_ROBOTS_CONTROLLED];
fix64 robot_last_message_time[MAX_ROBOTS_CONTROLLED];
int robot_send_pending[MAX_ROBOTS_CONTROLLED];
int robot_fired[MAX_ROBOTS_CONTROLLED];
ubyte robot_fire_buf[MAX_ROBOTS_CONTROLLED][18+3];

#define MULTI_ROBOT_PRIORITY(objnum, pnum) (((objnum % 4) + pnum) % N_players)

//#define MULTI_ROBOT_PRIORITY(objnum, pnum) multi_robot_priority(objnum, pnum)
//int multi_robot_priority(int objnum, int pnum)
//{
//	return( ((objnum % 4) + pnum) % N_players);
//}

int multi_can_move_robot(vobjptridx_t objnum, int agitation)
{
	// Determine whether or not I am allowed to move this robot.
	int rval;

	// Claim robot if necessary.

	if (Player_exploded)
		return 0;

#ifndef NDEBUG
	if (objnum->type != OBJ_ROBOT)
	{
		Int3();
		rval = 0;
	}
#endif

	else if ((Robot_info[get_robot_id(objnum)].boss_flag) && (Boss_dying == 1))
		return 0;

	else if (objnum->ctype.ai_info.REMOTE_OWNER == Player_num) // Already my robot!
	{
		int slot_num = objnum->ctype.ai_info.REMOTE_SLOT_NUM;

      if ((slot_num < 0) || (slot_num >= MAX_ROBOTS_CONTROLLED))
		 {
		  return 0;
		 }

		if (robot_fired[slot_num]) {
			rval = 0;
		}
		else {
			robot_agitation[slot_num] = agitation;
			robot_last_message_time[slot_num] = GameTime64;
			rval = 1;
		}
	}

	else if ((objnum->ctype.ai_info.REMOTE_OWNER != -1) || (agitation < MIN_TO_ADD))
	{
		if (agitation == ROBOT_FIRE_AGITATION) // Special case for firing at non-player
		{
			rval = 1; // Try to fire at player even tho we're not in control!
		}
		else
			rval = 0;
	}
	else
		rval = multi_add_controlled_robot(objnum, agitation);

	return(rval);
}

void
multi_check_robot_timeout(void)
{
	static fix64 lastcheck = 0;
	int i;

	if (GameTime64 > lastcheck + F1_0 || lastcheck > GameTime64)
	{
		lastcheck = GameTime64;
		for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++) 
		{
			if (robot_controlled[i] != object_none && robot_last_send_time[i] + ROBOT_TIMEOUT < GameTime64)
			{
				if (Objects[robot_controlled[i]].ctype.ai_info.REMOTE_OWNER != Player_num)
				{		
					robot_controlled[i] = object_none;
					Int3(); // Non-terminal but Rob is interesting, step over please...
					return;
				}
				if (robot_send_pending[i])
					multi_send_robot_position(robot_controlled[i], 1);
				multi_send_release_robot(robot_controlled[i]);
//				multi_delete_controlled_robot(robot_controlled[i]);
//				robot_controlled[i] = -1;
			}
		}
	}			
}

void
multi_strip_robots(int playernum)
{
	// Grab all robots away from a player 
	// (player died or exited the game)

	int i;

	if (Game_mode & GM_MULTI_ROBOTS) {
	
		if (playernum == Player_num)
		{
			range_for (auto r, robot_controlled)
				if (r != object_none)
					multi_delete_controlled_robot(r);
		}

		for (i = 1; i <= Highest_object_index; i++)
			if ((Objects[i].type == OBJ_ROBOT) && (Objects[i].ctype.ai_info.REMOTE_OWNER == playernum)) {
				Assert((Objects[i].control_type == CT_AI) || (Objects[i].control_type == CT_NONE) || (Objects[i].control_type == CT_MORPH));
				Objects[i].ctype.ai_info.REMOTE_OWNER = -1;
				if (playernum == Player_num)
					Objects[i].ctype.ai_info.REMOTE_SLOT_NUM = 4;
				else
					Objects[i].ctype.ai_info.REMOTE_SLOT_NUM = 0;
	  		}
	}
	// Note -- only call this with playernum == Player_num if all other players
	// already know that we are clearing house.  This does not send a release
	// message for each controlled robot!!

}

int multi_add_controlled_robot(vobjptridx_t objnum, int agitation)
{
	int i;
	int lowest_agitation = 0x7fffffff; // MAX POSITIVE INT
	int lowest_agitated_bot = -1;
	int first_free_robot = -1;

	// Try to add a new robot to the controlled list, return 1 if added, 0 if not.

#if defined(DXX_BUILD_DESCENT_II)
   if (Robot_info[get_robot_id(objnum)].boss_flag) // this is a boss, so make sure he gets a slot
		agitation=(agitation*3)+Player_num;  
#endif
	if (objnum->ctype.ai_info.REMOTE_SLOT_NUM > 0)
	{
		objnum->ctype.ai_info.REMOTE_SLOT_NUM -= 1;
		return 0;
	}

	for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++)
	{
		if (robot_controlled[i] == object_none || Objects[robot_controlled[i]].type != OBJ_ROBOT) {
			first_free_robot = i;
			break;
		}

		if (robot_last_message_time[i] + ROBOT_TIMEOUT < GameTime64) {
			if (robot_send_pending[i])
				multi_send_robot_position(robot_controlled[i], 1);
			multi_send_release_robot(robot_controlled[i]);
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
		if (robot_send_pending[lowest_agitated_bot])
			multi_send_robot_position(robot_controlled[lowest_agitated_bot], 1);
		multi_send_release_robot(robot_controlled[lowest_agitated_bot]);

		i = lowest_agitated_bot;
	}
	else {
		return(0); // Sorry, can't squeeze him in!
	}

	multi_send_claim_robot(objnum);
	robot_controlled[i] = objnum;
	robot_agitation[i] = agitation;
	objnum->ctype.ai_info.REMOTE_OWNER = Player_num;
	objnum->ctype.ai_info.REMOTE_SLOT_NUM = i;
	robot_controlled_time[i] = GameTime64;
	robot_last_send_time[i] = robot_last_message_time[i] = GameTime64;
	return(1);
}	

void multi_delete_controlled_robot(vobjptridx_t objnum)
{
	int i;

	// Delete robot object number objnum from list of controlled robots because it is dead

	for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++)
		if (robot_controlled[i] == objnum)
			break;

	if (i == MAX_ROBOTS_CONTROLLED)
		return;

	if (objnum->ctype.ai_info.REMOTE_SLOT_NUM != i)
	 {
	  Int3();  // can't release this bot!
	  return;
	 }

	objnum->ctype.ai_info.REMOTE_OWNER = -1;
	objnum->ctype.ai_info.REMOTE_SLOT_NUM = 0;
	robot_controlled[i] = object_none;
	robot_send_pending[i] = 0;
	robot_fired[i] = 0;
}

struct multi_claim_robot
{
	uint8_t pnum;
	int8_t owner;
	int16_t robjnum;
};
DEFINE_MULTIPLAYER_SERIAL_MESSAGE(MULTI_ROBOT_CLAIM, multi_claim_robot, b, (b.pnum, b.owner, b.robjnum));

void multi_send_claim_robot(vobjptridx_t objnum)
{
	if (objnum->type != OBJ_ROBOT)
		throw std::runtime_error("claiming non-robot"); // See rob
	// The AI tells us we should take control of this robot. 
	auto r = objnum_local_to_remote(objnum);
	multi_serialize_write(2, multi_claim_robot{static_cast<uint8_t>(Player_num), r.owner, r.objnum});
}

void multi_send_release_robot(vobjptridx_t objnum)
{
	short s;
	if (objnum->type != OBJ_ROBOT)
	{
		Int3(); // See rob
		return;
	}

	multi_delete_controlled_robot(objnum);

	multibuf[1] = Player_num;
	s = objnum_local_to_remote(objnum, (sbyte *)&multibuf[4]);
	PUT_INTEL_SHORT(multibuf+2, s);

	multi_send_data<MULTI_ROBOT_RELEASE>(multibuf, 5, 2);
}

#define MIN_ROBOT_COM_GAP F1_0/12

int
multi_send_robot_frame(int sent)
{
	static int last_sent = 0;

	int i;
	int rval = 0;

	for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++)
	{
		int sending = (last_sent+1+i)%MAX_ROBOTS_CONTROLLED;
		if ( (robot_controlled[sending] != -1) && ((robot_send_pending[sending] > sent) || (robot_fired[sending] > sent)) )
		{
			if (robot_send_pending[sending])
			{
				multi_send_robot_position_sub(robot_controlled[sending], (robot_send_pending[sending]>1)?1:0);
				robot_send_pending[sending] = 0;
			}

			if (robot_fired[sending])
			{
				robot_fired[sending] = 0;
				multi_send_data<MULTI_ROBOT_FIRE>(robot_fire_buf[sending], 18, 1);
			}

			if (!(Game_mode & GM_NETWORK))
				sent += 1;

			last_sent = sending;
			rval++;
		}
	}
	Assert((last_sent >= 0) && (last_sent <= MAX_ROBOTS_CONTROLLED));
	return(rval);
}

void multi_send_robot_position_sub(vobjptridx_t objnum, int now)
{
	int loc = 0;
	short s;
#ifdef WORDS_BIGENDIAN
	shortpos sp;
#endif

	loc += 1;
	multibuf[loc] = Player_num;											loc += 1;
	s = objnum_local_to_remote(objnum, (sbyte *)&multibuf[loc+2]);
	PUT_INTEL_SHORT(multibuf+loc, s);

																		loc += 3;
#ifndef WORDS_BIGENDIAN
	create_shortpos((shortpos *)(multibuf+loc), objnum,0);		loc += sizeof(shortpos);
#else
	create_shortpos(&sp, objnum, 1);
	memcpy(&(multibuf[loc]), (ubyte *)(sp.bytemat), 9);
	loc += 9;
	memcpy(&(multibuf[loc]), (ubyte *)&(sp.xo), 14);
	loc += 14;
#endif
	multi_send_data<MULTI_ROBOT_POSITION>(multibuf, loc, now?1:0);
}

void multi_send_robot_position(vobjptridx_t objnum, int force)
{
	// Send robot position to other player(s).  Includes a byte
	// value describing whether or not they fired a weapon

	if (!(Game_mode & GM_MULTI))
		return;

	if (objnum->type != OBJ_ROBOT)
	{
		Int3(); // See rob
		return;
	}

	if (objnum->ctype.ai_info.REMOTE_OWNER != Player_num)
		return;

//	Objects[objnum].phys_info.drag = Robot_info[Objects[objnum].id].drag; // Set drag to normal

	robot_last_send_time[objnum->ctype.ai_info.REMOTE_SLOT_NUM] = GameTime64;
	robot_send_pending[objnum->ctype.ai_info.REMOTE_SLOT_NUM] = 1+force;
	return;
}

void multi_send_robot_fire(vobjptridx_t obj, int gun_num, vms_vector *fire)
{
	// Send robot fire event
	int loc = 0;
	short s;
#ifdef WORDS_BIGENDIAN
	vms_vector swapped_vec;
#endif

	loc += 1;
	multibuf[loc] = Player_num;								loc += 1;
	s = objnum_local_to_remote(obj, (sbyte *)&multibuf[loc+2]);
	PUT_INTEL_SHORT(multibuf+loc, s);
																		loc += 3;
	multibuf[loc] = gun_num;									loc += 1;
#ifndef WORDS_BIGENDIAN
	memcpy(multibuf+loc, fire, sizeof(vms_vector));         loc += sizeof(vms_vector); // 12
	// --------------------------
	//      Total = 18
#else
	swapped_vec.x = (fix)INTEL_INT((int)fire->x);
	swapped_vec.y = (fix)INTEL_INT((int)fire->y);
	swapped_vec.z = (fix)INTEL_INT((int)fire->z);
	memcpy(multibuf+loc, &swapped_vec, sizeof(vms_vector)); loc += sizeof(vms_vector);
#endif

	if (obj->ctype.ai_info.REMOTE_OWNER == Player_num)
	{
		int slot = obj->ctype.ai_info.REMOTE_SLOT_NUM;
		if (slot<0 || slot>=MAX_ROBOTS_CONTROLLED)
		 {
			return;
		 }
		if (robot_fired[slot] != 0)
		 {
		  //	Int3(); // ROB!
			return;
	    }
		memcpy(robot_fire_buf[slot], multibuf, loc);
		robot_fired[slot] = 1;
	}
	else
		multi_send_data<MULTI_ROBOT_FIRE>(multibuf, loc, 0); // Not our robot, send ASAP
}

struct multi_explode_robot
{
	int16_t robj_killer, robj_killed;
	int8_t owner_killer, owner_killed;
};
DEFINE_MULTIPLAYER_SERIAL_MESSAGE(MULTI_ROBOT_EXPLODE, multi_explode_robot, b, (b.robj_killer, b.robj_killed, b.owner_killer, b.owner_killed));

void multi_send_robot_explode(objptridx_t objnum, objnum_t killer)
{
	// Send robot explosion event to the other players
	const auto k = objnum_local_to_remote(killer);
	multi_explode_robot e;
	e.robj_killer = k.objnum;
	e.owner_killer = k.owner;
	const auto d = objnum_local_to_remote(objnum);
	e.robj_killed = d.objnum;
	e.owner_killed = d.owner;
	multi_serialize_write(2, e);

	multi_delete_controlled_robot(objnum);
}

void multi_send_create_robot(int station, objnum_t objnum, int type)
{
	// Send create robot information

	int loc = 0;

	loc += 1;
	multibuf[loc] = Player_num;								loc += 1;
	multibuf[loc] = (sbyte)station;                         loc += 1;
	PUT_INTEL_SHORT(multibuf+loc, objnum);                  loc += 2;
	multibuf[loc] = type;									loc += 1;

	map_objnum_local_to_local(objnum);

	multi_send_data<MULTI_CREATE_ROBOT>(multibuf, loc, 2);
}

struct boss_teleport
{
	objnum_t objnum;
	segnum_t where;
};
DEFINE_MULTIPLAYER_SERIAL_MESSAGE(MULTI_BOSS_TELEPORT, boss_teleport, b, (b.objnum, b.where));

struct boss_cloak
{
	objnum_t objnum;
};
DEFINE_MULTIPLAYER_SERIAL_MESSAGE(MULTI_BOSS_CLOAK, boss_cloak, b, (b.objnum));

struct boss_start_gate
{
	objnum_t objnum;
};
DEFINE_MULTIPLAYER_SERIAL_MESSAGE(MULTI_BOSS_START_GATE, boss_start_gate, b, (b.objnum));

struct boss_stop_gate
{
	objnum_t objnum;
};
DEFINE_MULTIPLAYER_SERIAL_MESSAGE(MULTI_BOSS_STOP_GATE, boss_stop_gate, b, (b.objnum));

struct boss_create_robot
{
	objnum_t objnum;
	objnum_t objrobot;
	segnum_t where;
	uint8_t robot_type;
};
DEFINE_MULTIPLAYER_SERIAL_MESSAGE(MULTI_BOSS_CREATE_ROBOT, boss_create_robot, b, (b.objnum, b.objrobot, b.where, b.robot_type));

template <typename T, typename... Args>
static inline void multi_send_boss_action(objnum_t bossobjnum, Args&&... args)
{
	multi_serialize_write(2, T{bossobjnum, std::forward<Args>(args)...});
}

void multi_send_boss_teleport(vobjptridx_t bossobj, segnum_t where)
{
	// Boss is up for grabs after teleporting
	Assert((bossobj->ctype.ai_info.REMOTE_SLOT_NUM >= 0) && (bossobj->ctype.ai_info.REMOTE_SLOT_NUM < MAX_ROBOTS_CONTROLLED));
	multi_delete_controlled_robot(bossobj);
#if defined(DXX_BUILD_DESCENT_I)
	bossobj->ctype.ai_info.REMOTE_SLOT_NUM = 5; // Hands-off period!
#endif
	multi_send_boss_action<boss_teleport>(bossobj, where);
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

void multi_send_boss_create_robot(objnum_t bossobjnum, int robot_type, vobjptridx_t objrobot)
{
	multi_send_boss_action<boss_create_robot>(bossobjnum, objrobot, objrobot->segnum, static_cast<uint8_t>(robot_type));
}

#define MAX_ROBOT_POWERUPS 4

void
static multi_send_create_robot_powerups(object *del_obj)
{
	// Send create robot information

	int loc = 0;
	int i;
#ifdef WORDS_BIGENDIAN
	vms_vector swapped_vec;
#endif

	loc += 1;
	multibuf[loc] = Player_num;									loc += 1;
	multibuf[loc] = del_obj->contains_count;					loc += 1;
	multibuf[loc] = del_obj->contains_type; 					loc += 1;
	multibuf[loc] = del_obj->contains_id;						loc += 1;
	PUT_INTEL_SHORT(multibuf+loc, del_obj->segnum);		        loc += 2;
#ifndef WORDS_BIGENDIAN
	memcpy(multibuf+loc, &del_obj->pos, sizeof(vms_vector));	loc += 12;
#else
	swapped_vec.x = (fix)INTEL_INT((int)del_obj->pos.x);
	swapped_vec.y = (fix)INTEL_INT((int)del_obj->pos.y);
	swapped_vec.z = (fix)INTEL_INT((int)del_obj->pos.z);
	memcpy(multibuf+loc, &swapped_vec, sizeof(vms_vector));     loc += 12;
#endif

	memset(multibuf+loc, -1, MAX_ROBOT_POWERUPS*sizeof(short));
#if defined(DXX_BUILD_DESCENT_II)
   if (del_obj->contains_count!=Net_create_loc)
	  Int3();  //Get Jason, a bad thing happened
#endif

	if ((Net_create_loc > MAX_ROBOT_POWERUPS) || (Net_create_loc < 1))
	{
		Int3(); // See Rob
	}
	for (i = 0; i < Net_create_loc; i++)
	{
		PUT_INTEL_SHORT(multibuf+loc, Net_create_objnums[i]);
		loc += 2;
		map_objnum_local_to_local(Net_create_objnums[i]);
	}

	Net_create_loc = 0;

	multi_send_data<MULTI_CREATE_ROBOT_POWERUPS>(multibuf, 27, 2);
}

void multi_do_claim_robot(const playernum_t pnum, const ubyte *buf)
{
	multi_claim_robot b;
	multi_serialize_read(buf, b);
	objnum_t botnum = objnum_remote_to_local(b.robjnum, b.owner);
	if ((botnum > Highest_object_index) || (botnum < 0)) {
		return;
	}

	if (Objects[botnum].type != OBJ_ROBOT) {
		return;
	}
	
	if (Objects[botnum].ctype.ai_info.REMOTE_OWNER != -1)
	{
		if (MULTI_ROBOT_PRIORITY(b.robjnum, pnum) <= MULTI_ROBOT_PRIORITY(b.robjnum, Objects[botnum].ctype.ai_info.REMOTE_OWNER))
			return;
	}
	
	// Perform the requested change

	if (Objects[botnum].ctype.ai_info.REMOTE_OWNER == Player_num)
	{
		multi_delete_controlled_robot(botnum);
	}

	Objects[botnum].ctype.ai_info.REMOTE_OWNER = pnum;
	Objects[botnum].ctype.ai_info.REMOTE_SLOT_NUM = 0;
}

void multi_do_release_robot(const playernum_t pnum, const ubyte *buf)
{
	short remote_botnum;

	remote_botnum = GET_INTEL_SHORT(buf + 2);
	objnum_t botnum = objnum_remote_to_local(remote_botnum, (sbyte)buf[4]);

	if ((botnum < 0) || (botnum > Highest_object_index)) {
		return;
	}

	if (Objects[botnum].type != OBJ_ROBOT) {
		return;
	}
	
	if (Objects[botnum].ctype.ai_info.REMOTE_OWNER != pnum)
	{
		return;
	}
	
	// Perform the requested change

	Objects[botnum].ctype.ai_info.REMOTE_OWNER = -1;
	Objects[botnum].ctype.ai_info.REMOTE_SLOT_NUM = 0;
}

void multi_do_robot_position(const playernum_t pnum, const ubyte *buf)
{
	// Process robot movement sent by another player

	short remote_botnum;
	int loc = 1;
#ifdef WORDS_BIGENDIAN
	shortpos sp;
#endif

	;										loc += 1;

	remote_botnum = GET_INTEL_SHORT(buf + loc);
	objnum_t botnum = objnum_remote_to_local(remote_botnum, (sbyte)buf[loc+2]); loc += 3;

	if ((botnum < 0) || (botnum > Highest_object_index)) {
		return;
	}

	if ((Objects[botnum].type != OBJ_ROBOT) || (Objects[botnum].flags & OF_EXPLODING)) {
		return;
	}
		
	if (Objects[botnum].ctype.ai_info.REMOTE_OWNER != pnum)
	{	
		if (Objects[botnum].ctype.ai_info.REMOTE_OWNER == -1)
		{
			// Robot claim packet must have gotten lost, let this player claim it.
			if (Objects[botnum].ctype.ai_info.REMOTE_SLOT_NUM > 3) {
				Objects[botnum].ctype.ai_info.REMOTE_OWNER = pnum;
				Objects[botnum].ctype.ai_info.REMOTE_SLOT_NUM = 0;
			}
			else
				Objects[botnum].ctype.ai_info.REMOTE_SLOT_NUM++;
		}
		else
		{
			return;
		}
	}

	set_thrust_from_velocity(&Objects[botnum]); // Try to smooth out movement
//	Objects[botnum].phys_info.drag = Robot_info[Objects[botnum].id].drag >> 4; // Set drag to low

#ifndef WORDS_BIGENDIAN
	extract_shortpos(&Objects[botnum], (shortpos *)(buf+loc), 0);
#else
	memcpy((ubyte *)(sp.bytemat), (ubyte *)(buf + loc), 9);		loc += 9;
	memcpy((ubyte *)&(sp.xo), (ubyte *)(buf + loc), 14);
	extract_shortpos(&Objects[botnum], &sp, 1);
#endif
}

void
multi_do_robot_fire(const ubyte *buf)
{
	// Send robot fire event
	int loc = 1;
	short remote_botnum;
	int gun_num;
	vms_vector fire, gun_point;
	robot_info *robptr;

	loc += 1; // pnum
	remote_botnum = GET_INTEL_SHORT(buf + loc);
	objnum_t botnum = objnum_remote_to_local(remote_botnum, (sbyte)buf[loc+2]); loc += 3;
	gun_num = (sbyte)buf[loc];                                      loc += 1;
	memcpy(&fire, buf+loc, sizeof(vms_vector));
	fire.x = (fix)INTEL_INT((int)fire.x);
	fire.y = (fix)INTEL_INT((int)fire.y);
	fire.z = (fix)INTEL_INT((int)fire.z);

	if ((botnum < 0) || (botnum > Highest_object_index) || (Objects[botnum].type != OBJ_ROBOT) || (Objects[botnum].flags & OF_EXPLODING))
	{
		return;
	}
	auto botp = vobjptridx(botnum);
	// Do the firing
	if (gun_num == -1
#if defined(DXX_BUILD_DESCENT_II)
		|| gun_num==-2
#endif
		)
	{
		// Drop proximity bombs
		vm_vec_add(&gun_point, &botp->pos, &fire);
		if (gun_num == -1)
			Laser_create_new_easy( &fire, &gun_point, botp, PROXIMITY_ID, 1);
#if defined(DXX_BUILD_DESCENT_II)
		else
			Laser_create_new_easy( &fire, &gun_point, botp, SUPERPROX_ID, 1);
#endif
	}
	else
	{
		calc_gun_point(&gun_point, botp, gun_num);
		robptr = &Robot_info[get_robot_id(botp)];
		Laser_create_new_easy( &fire, &gun_point, botp, (enum weapon_type_t) robptr->weapon_type, 1);
	}
}

int multi_explode_robot_sub(vobjptridx_t robot)
{
	if (robot->type != OBJ_ROBOT) { // Object is robot?
		return 0;
	}

	if (robot->flags & OF_EXPLODING) { // Object not already exploding
		return 0;
	}

	// Data seems valid, explode the sucker

	if (Network_send_objects && multi_objnum_is_past(robot))
	{
		Network_send_objnum = -1;
	}

	// Drop non-random KEY powerups locally only!
	if ((robot->contains_count > 0) && (robot->contains_type == OBJ_POWERUP) && (Game_mode & GM_MULTI_COOP) && (robot->contains_id >= POW_KEY_BLUE) && (robot->contains_id <= POW_KEY_GOLD))
	{
		object_create_egg(robot);
	}
	else if (robot->ctype.ai_info.REMOTE_OWNER == Player_num) 
	{
		multi_drop_robot_powerups(robot);
		multi_delete_controlled_robot(robot);
	}
	else if (robot->ctype.ai_info.REMOTE_OWNER == -1 && multi_i_am_master()) 
	{
		multi_drop_robot_powerups(robot);
	}
	if (robot_is_thief(&Robot_info[get_robot_id(robot)]))
		drop_stolen_items(robot);

	if (Robot_info[get_robot_id(robot)].boss_flag) {
		if (!Boss_dying)
			start_boss_death_sequence(robot);	
		else
			return (0);
	}
#if defined(DXX_BUILD_DESCENT_II)
	else if (Robot_info[get_robot_id(robot)].death_roll) {
		start_robot_death_sequence(robot);
	}
#endif
	else
	{
#if defined(DXX_BUILD_DESCENT_II)
		if (robot->id == SPECIAL_REACTOR_ROBOT)
			special_reactor_stuff();
		if (Robot_info[get_robot_id(robot)].kamikaze)
			explode_object(robot,1);	//	Kamikaze, explode right away, IN YOUR FACE!
		else
#endif
			explode_object(robot,STANDARD_EXPL_DELAY);
   }
   
	return 1;
}

void
multi_do_robot_explode(const ubyte *buf)
{
	multi_explode_robot b;
	multi_serialize_read(buf, b);
	objnum_t killer = objnum_remote_to_local(b.robj_killer, b.owner_killer);
	objnum_t botnum = objnum_remote_to_local(b.robj_killed, b.owner_killed);
	// Explode robot controlled by other player
	int rval;
	if ((botnum < 0) || (botnum > Highest_object_index)) {
		return;
	}

	rval = multi_explode_robot_sub(botnum);

	if (rval && (killer == Players[Player_num].objnum))
		add_points_to_score(Robot_info[get_robot_id(&Objects[botnum])].score_value);
}

void multi_do_create_robot(const playernum_t pnum, const ubyte *buf)
{
	uint_fast32_t fuelcen_num = buf[2];
	int type = buf[5];

	FuelCenter *robotcen;
	vms_vector cur_object_loc, direction;

	objnum_t objnum;
	objnum = GET_INTEL_SHORT(buf + 3);

	if ((objnum < 0) || (fuelcen_num >= Num_fuelcenters) || (pnum >= N_players))
	{
		Int3(); // Bogus data
		return;
	}

	robotcen = &Station[fuelcen_num];

	// Play effect and sound

	compute_segment_center(&cur_object_loc, &Segments[robotcen->segnum]);
	objptridx_t obj = object_create_explosion(robotcen->segnum, &cur_object_loc, i2f(10), VCLIP_MORPHING_ROBOT);
	if (obj)
		extract_orient_from_segment(&obj->orient, &Segments[robotcen->segnum]);
	if (Vclip[VCLIP_MORPHING_ROBOT].sound_num > -1)
		digi_link_sound_to_pos( Vclip[VCLIP_MORPHING_ROBOT].sound_num, robotcen->segnum, 0, &cur_object_loc, 0, F1_0 );

	// Set robot center flags, in case we become the master for the next one

	robotcen->Flag = 0;
	robotcen->Capacity -= EnergyToCreateOneRobot;
	robotcen->Timer = 0;

	obj = create_morph_robot(&Segments[robotcen->segnum], &cur_object_loc, type);
	if (obj == object_none)
		return; // Cannot create object!
	
	obj->matcen_creator = (robotcen-Station) | 0x80;
//	extract_orient_from_segment(&obj->orient, &Segments[robotcen->segnum]);
	vm_vec_sub( &direction, &ConsoleObject->pos, &obj->pos );
	vm_vector_2_matrix( &obj->orient, &direction, &obj->orient.uvec, NULL);
	morph_start( obj );

	map_objnum_local_to_remote(obj, objnum, pnum);

	Assert(obj->ctype.ai_info.REMOTE_OWNER == -1);
}

void multi_do_boss_teleport(const playernum_t pnum, const ubyte *buf)
{
	boss_teleport b;
	multi_serialize_read(buf, b);
	if ((b.objnum < 0) || (b.objnum > Highest_object_index))
	{
		Int3();  // See Rob
		return;
	}
	vobjptridx_t boss_obj = vobjptridx(b.objnum);
	if ((boss_obj->type != OBJ_ROBOT) || !(Robot_info[get_robot_id(boss_obj)].boss_flag))
	{
		Int3(); // Got boss actions for a robot who's not a boss?
		return;
	}
	segnum_t teleport_segnum = b.where;
	if ((teleport_segnum < 0) || (teleport_segnum > Highest_segment_index))
	{
		Int3();  // See Rob
		return;
	}
	vms_vector boss_dir;
	compute_segment_center(&boss_obj->pos, &Segments[teleport_segnum]);
	obj_relink(boss_obj, teleport_segnum);
	Last_teleport_time = GameTime64;

	vm_vec_sub(&boss_dir, &Objects[Players[pnum].objnum].pos, &boss_obj->pos);
	vm_vector_2_matrix(&boss_obj->orient, &boss_dir, NULL, NULL);

	digi_link_sound_to_pos( Vclip[VCLIP_MORPHING_ROBOT].sound_num, teleport_segnum, 0, &boss_obj->pos, 0 , F1_0);
	digi_kill_sound_linked_to_object( boss_obj);
	digi_link_sound_to_object2( SOUND_BOSS_SHARE_SEE, boss_obj, 1, F1_0, F1_0*512 );	//	F1_0*512 means play twice as loud
	ai_local		*ailp = &boss_obj->ctype.ai_info.ail;
	ailp->next_fire = 0;

	if (boss_obj->ctype.ai_info.REMOTE_OWNER == Player_num)
	{
		multi_delete_controlled_robot(boss_obj);
	}

	boss_obj->ctype.ai_info.REMOTE_OWNER = -1; // Boss is up for grabs again!
	boss_obj->ctype.ai_info.REMOTE_SLOT_NUM = 0; // Available immediately!
}

void multi_do_boss_cloak(const playernum_t pnum, const ubyte *buf)
{
	boss_cloak b;
	multi_serialize_read(buf, b);
	if ((b.objnum < 0) || (b.objnum > Highest_object_index))
	{
		Int3();  // See Rob
		return;
	}
	vobjptridx_t boss_obj = vobjptridx(b.objnum);
	if ((boss_obj->type != OBJ_ROBOT) || !(Robot_info[get_robot_id(boss_obj)].boss_flag))
	{
		Int3(); // Got boss actions for a robot who's not a boss?
		return;
	}
#if defined(DXX_BUILD_DESCENT_I)
	Boss_hit_this_frame = 0;
#elif defined(DXX_BUILD_DESCENT_II)
	Boss_hit_time = -F1_0*10;
#endif
	Boss_cloak_start_time = GameTime64;
	Boss_cloak_end_time = GameTime64 + Boss_cloak_duration;
	boss_obj->ctype.ai_info.CLOAKED = 1;
}

void multi_do_boss_start_gate(const playernum_t pnum, const ubyte *buf)
{
	boss_start_gate b;
	multi_serialize_read(buf, b);
	if ((b.objnum < 0) || (b.objnum > Highest_object_index))
	{
		Int3();  // See Rob
		return;
	}
	vobjptridx_t boss_obj = vobjptridx(b.objnum);
	if ((boss_obj->type != OBJ_ROBOT) || !(Robot_info[get_robot_id(boss_obj)].boss_flag))
	{
		Int3(); // Got boss actions for a robot who's not a boss?
		return;
	}
	restart_effect(ECLIP_NUM_BOSS);
}

void multi_do_boss_stop_gate(const playernum_t pnum, const ubyte *buf)
{
	boss_start_gate b;
	multi_serialize_read(buf, b);
	if ((b.objnum < 0) || (b.objnum > Highest_object_index))
	{
		Int3();  // See Rob
		return;
	}
	vobjptridx_t boss_obj = vobjptridx(b.objnum);
	if ((boss_obj->type != OBJ_ROBOT) || !(Robot_info[get_robot_id(boss_obj)].boss_flag))
	{
		Int3(); // Got boss actions for a robot who's not a boss?
		return;
	}
	stop_effect(ECLIP_NUM_BOSS);
}

void multi_do_boss_create_robot(const playernum_t pnum, const ubyte *buf)
{
	boss_create_robot b;
	multi_serialize_read(buf, b);
	if ((b.objnum < 0) || (b.objnum > Highest_object_index))
	{
		Int3();  // See Rob
		return;
	}
	vobjptridx_t boss_obj = vobjptridx(b.objnum);
	if ((boss_obj->type != OBJ_ROBOT) || !(Robot_info[get_robot_id(boss_obj)].boss_flag))
	{
		Int3(); // Got boss actions for a robot who's not a boss?
		return;
	}
	// Do some validity checking
	if ( (b.objrobot >= MAX_OBJECTS) || (b.objrobot < 0) || (b.where < 0) || (b.where > Highest_segment_index) )
	{
		Int3(); // See Rob, bad data in boss gate action message
		return;
	}
	// Gate one in!
	if (gate_in_robot(b.robot_type, b.where) != object_none)
		map_objnum_local_to_remote(Net_create_objnums[0], b.objrobot, pnum);
}

void multi_do_create_robot_powerups(const playernum_t pnum, const ubyte *buf)
{
	// Code to drop remote-controlled robot powerups

	int loc = 1;
	object del_obj{};
	int i;

	del_obj.type = OBJ_ROBOT;

	;					loc += 1;
	del_obj.contains_count = buf[loc];			loc += 1;
	del_obj.contains_type = buf[loc];			loc += 1;
	del_obj.contains_id = buf[loc]; 			loc += 1;
	del_obj.segnum = GET_INTEL_SHORT(buf + loc);            loc += 2;
	memcpy(&del_obj.pos, buf+loc, sizeof(vms_vector));      loc += 12;
	
	vm_vec_zero(del_obj.mtype.phys_info.velocity);

	del_obj.pos.x = (fix)INTEL_INT((int)del_obj.pos.x);
	del_obj.pos.y = (fix)INTEL_INT((int)del_obj.pos.y);
	del_obj.pos.z = (fix)INTEL_INT((int)del_obj.pos.z);

	Assert(pnum < N_players);
	Assert (pnum!=Player_num); // What? How'd we send ourselves this?

	Net_create_loc = 0;
	d_srand(1245L);

	objnum_t egg_objnum = object_create_egg(&del_obj);

	if (egg_objnum == object_none)
		return; // Object buffer full

//	Assert(egg_objnum > -1);
	Assert((Net_create_loc > 0) && (Net_create_loc <= MAX_ROBOT_POWERUPS));

	for (i = 0; i < Net_create_loc; i++)
	{
		short s;
		
		s = GET_INTEL_SHORT(buf + loc);
		if ( s != -1)
			map_objnum_local_to_remote((short)Net_create_objnums[i], s, pnum);
		else
			Objects[Net_create_objnums[i]].flags |= OF_SHOULD_BE_DEAD; // Delete objects other guy didn't create one of
		loc += 2;
	}
}

void multi_drop_robot_powerups(vobjptridx_t del_obj)
{
	// Code to handle dropped robot powerups in network mode ONLY!

	objnum_t egg_objnum = object_none;
	robot_info	*robptr; 

	if (del_obj->type != OBJ_ROBOT)
	{
		Int3(); // dropping powerups for non-robot, Rob's fault
		return;
	}
	robptr = &Robot_info[get_robot_id(del_obj)];

	Net_create_loc = 0;

	if (del_obj->contains_count > 0) { 
		//	If dropping a weapon that the player has, drop energy instead, unless it's vulcan, in which case drop vulcan ammo.
		if (del_obj->contains_type == OBJ_POWERUP) {
			maybe_replace_powerup_with_energy(del_obj);
			if (!multi_powerup_is_allowed(del_obj->contains_id))
				del_obj->contains_id=POW_SHIELD_BOOST;

			// No key drops in non-coop games!
			if (!(Game_mode & GM_MULTI_COOP)) {
				if ((del_obj->contains_id >= POW_KEY_BLUE) && (del_obj->contains_id <= POW_KEY_GOLD))
					del_obj->contains_count = 0;
			}
		}
		d_srand(1245L);
		if (del_obj->contains_count > 0)
			egg_objnum = object_create_egg(del_obj);
	}
		
	else if (del_obj->ctype.ai_info.REMOTE_OWNER == -1) // No random goodies for robots we weren't in control of
		return;

	else if (robptr->contains_count) {
		d_srand((fix)timer_query());
		if (((d_rand() * 16) >> 15) < robptr->contains_prob) {
			del_obj->contains_count = ((d_rand() * robptr->contains_count) >> 15) + 1;
			del_obj->contains_type = robptr->contains_type;
			del_obj->contains_id = robptr->contains_id;
			if (del_obj->contains_type == OBJ_POWERUP)
			 {
				maybe_replace_powerup_with_energy(del_obj);
				if (!multi_powerup_is_allowed(del_obj->contains_id))
					del_obj->contains_id=POW_SHIELD_BOOST;
			 }
		
			d_srand(1245L);
			if (del_obj->contains_count > 0)
				egg_objnum = object_create_egg(del_obj);
		}
	}

	if (egg_objnum != object_none) {
		// Transmit the object creation to the other players	 	
		multi_send_create_robot_powerups(del_obj);
	}
}

//	-----------------------------------------------------------------------------
//	Robot *robot got whacked by player player_num and requests permission to do something about it.
//	Note: This function will be called regardless of whether Game_mode is a multiplayer mode, so it
//	should quick-out if not in a multiplayer mode.  On the other hand, it only gets called when a
//	player or player weapon whacks a robot, so it happens rarely.
void multi_robot_request_change(vobjptridx_t robot, int player_num)
{
	int slot, remote_objnum;
	sbyte dummy;

	if (!(Game_mode & GM_MULTI_ROBOTS))
		return;
#if defined(DXX_BUILD_DESCENT_I)
	if (robot->ctype.ai_info.REMOTE_OWNER != Player_num)
		return;
#endif

	slot = robot->ctype.ai_info.REMOTE_SLOT_NUM;

	if ((slot < 0) || (slot >= MAX_ROBOTS_CONTROLLED)) {
		Int3();
		return;
	}

	remote_objnum = objnum_local_to_remote(robot, &dummy);
	if (remote_objnum < 0)
		return;

	if ( (robot_agitation[slot] < 70) || (MULTI_ROBOT_PRIORITY(remote_objnum, player_num) > MULTI_ROBOT_PRIORITY(remote_objnum, Player_num)) || (d_rand() > 0x4400))
	{
		if (robot_send_pending[slot])
			multi_send_robot_position(robot_controlled[slot], -1);
		multi_send_release_robot(robot_controlled[slot]);
		robot->ctype.ai_info.REMOTE_SLOT_NUM = 5;  // Hands-off period
	}
}
