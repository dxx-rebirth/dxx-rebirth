/* $Id: multibot.c,v 1.4 2003-10-03 08:21:28 btb Exp $ */
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
 * Multiplayer robot code
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "vecmat.h"
#include "object.h"
#include "multibot.h"
#include "game.h"
#include "modem.h"
#include "network.h"
#include "multi.h"
#include "laser.h"
#include "error.h"
#include "mono.h"
#include "timer.h"
#include "text.h"
#include "ai.h"
#include "fireball.h"
#include "aistruct.h"
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
#include "byteswap.h"



int multi_add_controlled_robot(int objnum, int agitation);
void multi_send_release_robot(int objnum);
void multi_delete_controlled_robot(int objnum);
void multi_send_robot_position_sub(int objnum);

//
// Code for controlling robots in multiplayer games
//


#define STANDARD_EXPL_DELAY (F1_0/4)
#define MIN_CONTROL_TIME	F1_0*1
#define ROBOT_TIMEOUT		F1_0*2

#define MIN_TO_ADD	60
#define MAX_TO_DELETE	61

int robot_controlled[MAX_ROBOTS_CONTROLLED];
int robot_agitation[MAX_ROBOTS_CONTROLLED];
fix robot_controlled_time[MAX_ROBOTS_CONTROLLED];
fix robot_last_send_time[MAX_ROBOTS_CONTROLLED];
fix robot_last_message_time[MAX_ROBOTS_CONTROLLED];
int robot_send_pending[MAX_ROBOTS_CONTROLLED];
int robot_fired[MAX_ROBOTS_CONTROLLED];
byte robot_fire_buf[MAX_ROBOTS_CONTROLLED][18+3];

#define MULTI_ROBOT_PRIORITY(objnum, pnum) (((objnum % 4) + pnum) % N_players)

extern void multi_send_stolen_items();
extern int multi_powerup_is_allowed (int);

//#define MULTI_ROBOT_PRIORITY(objnum, pnum) multi_robot_priority(objnum, pnum)
//int multi_robot_priority(int objnum, int pnum)
//{
//	return( ((objnum % 4) + pnum) % N_players);
//}

int
multi_can_move_robot(int objnum, int agitation)
{
	// Determine whether or not I am allowed to move this robot.
	int rval;

	// Claim robot if necessary.

	if (Player_exploded)
		return 0;

#ifndef NDEBUG
	if ((objnum < 0) || (objnum > Highest_object_index))
	{	
		Int3();
		rval = 0;
	}

	else if (Objects[objnum].type != OBJ_ROBOT)
	{
		Int3();
		rval = 0;
	}
#endif

	else if ((Robot_info[Objects[objnum].id].boss_flag) && (Boss_dying == 1))
		return 0;

	else if (Objects[objnum].ctype.ai_info.REMOTE_OWNER == Player_num) // Already my robot!
	{
		int slot_num = Objects[objnum].ctype.ai_info.REMOTE_SLOT_NUM;

      if ((slot_num < 0) || (slot_num >= MAX_ROBOTS_CONTROLLED))
		 {
		  mprintf ((0,"Not asserting in multi_can_move_robot!"));
		  return 0;
		 }

		if (robot_fired[slot_num]) {
//			mprintf((0, "Preventing robot from firing again until firing complete!\n"));
			rval = 0;
		}
		else {
			robot_agitation[slot_num] = agitation;
			robot_last_message_time[slot_num] = GameTime;
			rval = 1;
		}
	}

	else if ((Objects[objnum].ctype.ai_info.REMOTE_OWNER != -1) || (agitation < MIN_TO_ADD))
	{
		if (agitation == ROBOT_FIRE_AGITATION) // Special case for firing at non-player
		{
			mprintf((0, "Shooting anyway.\n"));
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
	static fix lastcheck = 0;
	int i;

	if (GameTime > lastcheck + F1_0)
	{
		lastcheck = GameTime;
		for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++) 
		{
			if ((robot_controlled[i] != -1) && (robot_last_send_time[i] + ROBOT_TIMEOUT < GameTime)) 
			{
				if (Objects[robot_controlled[i]].ctype.ai_info.REMOTE_OWNER != Player_num)
				{		
					robot_controlled[i] = -1;
					Int3(); // Non-terminal but Rob is interesting, step over please...
					return;
				}
 				if (Objects[robot_controlled[i]].ctype.ai_info.REMOTE_OWNER !=Player_num)
				 {
					mprintf ((0,"What? This ain't my bot and I'm trying to time it out!\n"));
					return;
				 }
				mprintf((0, "Robot %d (slot %d) removed, timed out, frame %d.\n", robot_controlled[i], i, GameTime));
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
			for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++)
				multi_delete_controlled_robot(robot_controlled[i]);

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

void
multi_dump_robots(void)

{
	// Dump robot control info for debug purposes

	int i;

	if (!(Game_mode & GM_MULTI_ROBOTS))
		return;

	mprintf((0, "\n"));
	for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++)
	{
		mprintf((0, "Robot %d) objnum=%d, anger=%d, last_message=%d, last move=%d, gametime=%d\n", i, robot_controlled[i], robot_agitation[i], robot_last_message_time[i], robot_last_send_time[i], GameTime));
	}
}

int
multi_add_controlled_robot(int objnum, int agitation)
{
	int i;
	int lowest_agitation = 0x7fffffff; // MAX POSITIVE INT
	int lowest_agitated_bot = -1;
	int first_free_robot = -1;

	// Try to add a new robot to the controlled list, return 1 if added, 0 if not.

   if (Robot_info[Objects[objnum].id].boss_flag) // this is a boss, so make sure he gets a slot
		agitation=(agitation*3)+Player_num;  
 
	if (Objects[objnum].ctype.ai_info.REMOTE_SLOT_NUM > 0)
	{
		mprintf((0, "Robot %d hands-off = %d.\n", objnum, Objects[objnum].ctype.ai_info.REMOTE_SLOT_NUM));
		Objects[objnum].ctype.ai_info.REMOTE_SLOT_NUM -= 1;
		return 0;
	}

	for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++)
	{
		if ((robot_controlled[i] == -1) || (Objects[robot_controlled[i]].type != OBJ_ROBOT)) {
			first_free_robot = i;
			break;
		}

		if (robot_last_message_time[i] + ROBOT_TIMEOUT < GameTime) {
			mprintf((0, "Robot %d replaced (timeout).\n", robot_controlled[i]));
			if (robot_send_pending[i])
				multi_send_robot_position(robot_controlled[i], 1);
			multi_send_release_robot(robot_controlled[i]);
			first_free_robot = i;
			break;
		}

		if ((robot_controlled[i] != -1) && (robot_agitation[i] < lowest_agitation) && (robot_controlled_time[i] + MIN_CONTROL_TIME < GameTime))
		{
			lowest_agitation = robot_agitation[i];
			lowest_agitated_bot = i;
		}
	}

//   mprintf ((0,"first_free_robot=%d lowest_agitated_bot=%d\n",first_free_robot,lowest_agitated_bot));

	if (first_free_robot != -1)  // Room left for new robots
		i = first_free_robot;

	else if ((agitation > lowest_agitation))// && (lowest_agitation <= MAX_TO_DELETE)) // Replace some old robot with a more agitated one
	{
		if (robot_send_pending[lowest_agitated_bot])
			multi_send_robot_position(robot_controlled[lowest_agitated_bot], 1);
		multi_send_release_robot(robot_controlled[lowest_agitated_bot]);

		mprintf((0, "Replaced robot %d (agitation %d) with robot %d (agitation %d).\n", robot_controlled[lowest_agitated_bot], lowest_agitation, objnum, agitation));		

		i = lowest_agitated_bot;
	}
	else {
//		mprintf((0, "Cannot add robot %d agitation %d (min agitation %d)\n", objnum, agitation, lowest_agitation));
		return(0); // Sorry, can't squeeze him in!
	}

	mprintf((0, "Taking control of robot %d, agitation %d.\n", objnum, agitation));

	multi_send_claim_robot(objnum);
	robot_controlled[i] = objnum;
	robot_agitation[i] = agitation;
	Objects[objnum].ctype.ai_info.REMOTE_OWNER = Player_num;
	Objects[objnum].ctype.ai_info.REMOTE_SLOT_NUM = i;
	robot_controlled_time[i] = GameTime;
	robot_last_send_time[i] = robot_last_message_time[i] = GameTime;
	return(1);
}	

void
multi_delete_controlled_robot(int objnum)
{
	int i;

	// Delete robot object number objnum from list of controlled robots because it is dead

	if ( (objnum<0) || (objnum>Highest_object_index))	{
		mprintf((0, "Trying to releasing control of bogus robot %d\n", objnum));
		return;
	}

	for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++)
		if (robot_controlled[i] == objnum)
			break;

	if (i == MAX_ROBOTS_CONTROLLED)
		return;

	mprintf((0, "Releasing control of robot %d\n", objnum));

	if (Objects[objnum].ctype.ai_info.REMOTE_SLOT_NUM != i)
	 {
	  mprintf ((0,"Saved from asserting in multi_delete_controlled_bot!"));
	  Int3();  // can't release this bot!
	  return;
	 }

	Objects[objnum].ctype.ai_info.REMOTE_OWNER = -1;
	Objects[objnum].ctype.ai_info.REMOTE_SLOT_NUM = 0;
	robot_controlled[i] = -1;
	robot_send_pending[i] = 0;
	robot_fired[i] = 0;
}

void
multi_send_claim_robot(int objnum)
{
	short s;
	
	if ((objnum < 0) || (objnum > Highest_object_index))
	{
		Int3(); // See rob
		return;
	}

	if (Objects[objnum].type != OBJ_ROBOT)
	{
		Int3(); // See rob
		return;
	}

	// The AI tells us we should take control of this robot. 

	multibuf[0] = (char)MULTI_ROBOT_CLAIM;
	multibuf[1] = Player_num;
	s = objnum_local_to_remote(objnum, (byte *)&multibuf[4]);
	PUT_INTEL_SHORT(multibuf+2, s);

	multi_send_data(multibuf, 5, 2);
	multi_send_data(multibuf, 5, 2);
	multi_send_data(multibuf, 5, 2);

}

void
multi_send_release_robot(int objnum)
{
	short s;
	
	if ((objnum < 0) || (objnum > Highest_object_index))
	{
		Int3(); // See rob
		return;
	}

	if (Objects[objnum].type != OBJ_ROBOT)
	{
		Int3(); // See rob
		return;
	}

	multi_delete_controlled_robot(objnum);

	multibuf[0] = (char)MULTI_ROBOT_RELEASE;
	multibuf[1] = Player_num;
	s = objnum_local_to_remote(objnum, (byte *)&multibuf[4]);
	PUT_INTEL_SHORT(multibuf+2, s);

	multi_send_data(multibuf, 5, 2);
	multi_send_data(multibuf, 5, 2);
	multi_send_data(multibuf, 5, 2);
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
				robot_send_pending[sending] = 0;	
				multi_send_robot_position_sub(robot_controlled[sending]);
			}

			if (robot_fired[sending])
			{
				robot_fired[sending] = 0;
				multi_send_data(robot_fire_buf[sending], 18, 1);
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

void
multi_send_robot_position_sub(int objnum)
{
	int loc = 0;
	short s;
#ifdef WORDS_BIGENDIAN
	shortpos sp;
#endif

//	mprintf((0, "SENDPOS object %d, Gametime %d.\n", objnum, GameTime));

	multibuf[loc] = MULTI_ROBOT_POSITION;  								loc += 1;
	multibuf[loc] = Player_num;											loc += 1;
	s = objnum_local_to_remote(objnum, (byte *)&multibuf[loc+2]);
	PUT_INTEL_SHORT(multibuf+loc, s);

																		loc += 3;
#ifndef WORDS_BIGENDIAN
	create_shortpos((shortpos *)(multibuf+loc), Objects+objnum,0);		loc += sizeof(shortpos);
#else
	create_shortpos(&sp, Objects+objnum, 1);
	memcpy(&(multibuf[loc]), (ubyte *)(sp.bytemat), 9);
	loc += 9;
	memcpy(&(multibuf[loc]), (ubyte *)&(sp.xo), 14);
	loc += 14;
#endif
	multi_send_data(multibuf, loc, 1);
}

void
multi_send_robot_position(int objnum, int force)
{
	// Send robot position to other player(s).  Includes a byte
	// value describing whether or not they fired a weapon

	if (!(Game_mode & GM_MULTI))
		return;

	if ((objnum < 0) || (objnum > Highest_object_index))
	{
		Int3(); // See rob
		return;
	}
	if (Objects[objnum].type != OBJ_ROBOT)
	{
		Int3(); // See rob
		return;
	}

	if (Objects[objnum].ctype.ai_info.REMOTE_OWNER != Player_num)
		return;

//	Objects[objnum].phys_info.drag = Robot_info[Objects[objnum].id].drag; // Set drag to normal

	robot_last_send_time[Objects[objnum].ctype.ai_info.REMOTE_SLOT_NUM] = GameTime;

	robot_send_pending[Objects[objnum].ctype.ai_info.REMOTE_SLOT_NUM] = 1+force;

	if (force & (Game_mode & GM_NETWORK))
		PacketUrgent = 1;

	return;
}

void
multi_send_robot_fire(int objnum, int gun_num, vms_vector *fire)
{
	// Send robot fire event
	int loc = 0;
	short s;
#ifdef WORDS_BIGENDIAN
	vms_vector swapped_vec;
#endif

	multibuf[loc] = MULTI_ROBOT_FIRE;						loc += 1;
	multibuf[loc] = Player_num;								loc += 1;
	s = objnum_local_to_remote(objnum, (byte *)&multibuf[loc+2]);
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

	if (Objects[objnum].ctype.ai_info.REMOTE_OWNER == Player_num)
	{
		int slot = Objects[objnum].ctype.ai_info.REMOTE_SLOT_NUM;
		if (slot<0 || slot>=MAX_ROBOTS_CONTROLLED)
		 {
			mprintf ((0,"Saved from asserting in robofire\n"));
			return;
		 }
		if (robot_fired[slot] != 0)
		 {
			mprintf ((0,"Couldn't find an open slot for robo firing!\n"));
		  //	Int3(); // ROB!
			return;
	    }
		memcpy(robot_fire_buf[slot], multibuf, loc);
		robot_fired[slot] = 1;
		if (Game_mode & GM_NETWORK)
			PacketUrgent = 1;
	}
	else
		multi_send_data(multibuf, loc, 2); // Not our robot, send ASAP
}

void
multi_send_robot_explode(int objnum, int killer,char isthief)
{
	// Send robot explosion event to the other players

	int loc = 0;
	short s;

	multibuf[loc] = MULTI_ROBOT_EXPLODE;					loc += 1;
	multibuf[loc] = Player_num;								loc += 1;
	s = (short)objnum_local_to_remote(killer, (byte *)&multibuf[loc+2]);
	PUT_INTEL_SHORT(multibuf+loc, s);                       loc += 3;

	s = (short)objnum_local_to_remote(objnum, (byte *)&multibuf[loc+2]);
	PUT_INTEL_SHORT(multibuf+loc, s);                       loc += 3;
	
	multibuf[loc]=isthief;   loc++;
		
	multi_send_data(multibuf, loc, 1);

	multi_delete_controlled_robot(objnum);
}

void
multi_send_create_robot(int station, int objnum, int type)
{
	// Send create robot information

	int loc = 0;

	multibuf[loc] = MULTI_CREATE_ROBOT;						loc += 1;
	multibuf[loc] = Player_num;								loc += 1;
	multibuf[loc] = (byte)station;							loc += 1;
	PUT_INTEL_SHORT(multibuf+loc, objnum);                  loc += 2;
	multibuf[loc] = type;									loc += 1;

	map_objnum_local_to_local((short)objnum);

	multi_send_data(multibuf, loc, 2);
}

void
multi_send_boss_actions(int bossobjnum, int action, int secondary, int objnum)
{
	// Send special boss behavior information

	int loc = 0;
	
	multibuf[loc] = MULTI_BOSS_ACTIONS;						loc += 1;
	multibuf[loc] = Player_num;								loc += 1; // Which player is controlling the boss
	PUT_INTEL_SHORT(multibuf+loc, bossobjnum);              loc += 2; // We won't network map this objnum since it's the boss
	multibuf[loc] = (byte)action;							loc += 1; // What is the boss doing?
	multibuf[loc] = (byte)secondary;						loc += 1; // More info for what he is doing
	PUT_INTEL_SHORT(multibuf+loc, objnum);                  loc += 2; // Objnum of object created by gate-in action
	if (action == 3) {
		PUT_INTEL_SHORT(multibuf+loc, Objects[objnum].segnum); loc += 2; // Segment number object created in (for gate only)
	}
	else
																		loc += 2; // Dummy

	if (action == 1) { // Teleport releases robot
		// Boss is up for grabs after teleporting
		mprintf((0, "Boss teleporting, removed from my list.\n"));
		Assert((Objects[bossobjnum].ctype.ai_info.REMOTE_SLOT_NUM >= 0) && (Objects[bossobjnum].ctype.ai_info.REMOTE_SLOT_NUM < MAX_ROBOTS_CONTROLLED));
		multi_delete_controlled_robot(bossobjnum);
//		robot_controlled[Objects[bossobjnum].ctype.ai_info.REMOTE_SLOT_NUM] = -1;
//		Objects[bossobjnum].ctype.ai_info.REMOTE_OWNER = -1;
//		Objects[bossobjnum].ctype.ai_info.REMOTE_SLOT_NUM = 5; // Hands-off period!
	}
	if (action == 3) {
		mprintf((0, "LOCAL:Boss gating in robot id %d, objnum %d (%d).\n", secondary, objnum, objnum));
	}
	multi_send_data(multibuf, loc, 1);
}
			
#define MAX_ROBOT_POWERUPS 4

void
multi_send_create_robot_powerups(object *del_obj)
{
	// Send create robot information

	int loc = 0;
	int i;
#ifdef WORDS_BIGENDIAN
	vms_vector swapped_vec;
#endif

	multibuf[loc] = MULTI_CREATE_ROBOT_POWERUPS;				loc += 1;
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
  
   if (del_obj->contains_count!=Net_create_loc)
	  Int3();  //Get Jason, a bad thing happened

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

	multi_send_data(multibuf, 27, 2);
}

void
multi_do_claim_robot(char *buf)
{
	short botnum, remote_botnum;
	char pnum;

	pnum = buf[1];

	GET_INTEL_SHORT(remote_botnum, buf+2);
	botnum = objnum_remote_to_local(remote_botnum, (byte)buf[4]);

	if ((botnum > Highest_object_index) || (botnum < 0)) {
		mprintf((1, "Ignoring claim message for object I don't have.\n"));
//		Int3(); // See rob
		return;
	}

	if (Objects[botnum].type != OBJ_ROBOT) {
		mprintf((1, "Got MYBOT message for non-Robot!\n"));
		return;
	}
	
	if (Objects[botnum].ctype.ai_info.REMOTE_OWNER != -1)
	{
		mprintf((0, "Got a MYBOT message bot %d (%d) currently controlled by player %d.\n", botnum, remote_botnum, Objects[botnum].ctype.ai_info.REMOTE_OWNER));
		if (MULTI_ROBOT_PRIORITY(remote_botnum, pnum) <= MULTI_ROBOT_PRIORITY(remote_botnum, Objects[botnum].ctype.ai_info.REMOTE_OWNER))
			return;
	}
	
	// Perform the requested change

	mprintf((0, "Player %d taking control of robot %d (%d).\n", pnum, botnum, remote_botnum));

	if (Objects[botnum].ctype.ai_info.REMOTE_OWNER == Player_num)
	{
		multi_delete_controlled_robot(botnum);
	}

	Objects[botnum].ctype.ai_info.REMOTE_OWNER = pnum;
	Objects[botnum].ctype.ai_info.REMOTE_SLOT_NUM = 0;
}

void
multi_do_release_robot(char *buf)
{
	short botnum, remote_botnum;
	char pnum;

	pnum = buf[1];

	GET_INTEL_SHORT(remote_botnum, buf+2);
	botnum = objnum_remote_to_local(remote_botnum, (byte)buf[4]);

	if ((botnum < 0) || (botnum > Highest_object_index)) {
		mprintf((1, "Ignoring release message for object I don't have.\n"));
//		Int3(); // See rob
		return;
	}

	if (Objects[botnum].type != OBJ_ROBOT) {
		mprintf((1, "Got RELEASE message for non-Robot!\n"));
		return;
	}
	
	if (Objects[botnum].ctype.ai_info.REMOTE_OWNER != pnum)
	{
		mprintf((0, "Got a RELEASE message for bot %d not controlled by player %d.\n", botnum, pnum));
		return;
	}
	
	// Perform the requested change

	mprintf((0, "Player %d releasing control of robot %d (%d).\n", pnum, botnum, remote_botnum));

	Objects[botnum].ctype.ai_info.REMOTE_OWNER = -1;
	Objects[botnum].ctype.ai_info.REMOTE_SLOT_NUM = 0;
}

void
multi_do_robot_position(char *buf)
{
	// Process robot movement sent by another player

	short botnum, remote_botnum;
	char pnum;
	int loc = 1;
#ifdef WORDS_BIGENDIAN
	shortpos sp;
#endif

	pnum = buf[loc];										loc += 1;

	GET_INTEL_SHORT(remote_botnum, buf+loc);
	botnum = objnum_remote_to_local(remote_botnum, (byte)buf[loc+2]); loc += 3;

	if ((botnum < 0) || (botnum > Highest_object_index)) {
		mprintf((1, "Got robot position for object I don't have.\n"));
//		Int3(); // See rob
		return;
	}

	if ((Objects[botnum].type != OBJ_ROBOT) || (Objects[botnum].flags & OF_EXPLODING)) {
//		mprintf((0, "Ignoring position packet for non-robot or exploding robot.\n"));
		return;
	}
		
	if (Objects[botnum].ctype.ai_info.REMOTE_OWNER != pnum)
	{	
		if (Objects[botnum].ctype.ai_info.REMOTE_OWNER == -1)
		{
			// Robot claim packet must have gotten lost, let this player claim it.
			if (Objects[botnum].ctype.ai_info.REMOTE_SLOT_NUM > 3) {
				mprintf((0, "Player %d taking control of robot %d WITHOUT claim message.\n", pnum, botnum));
				Objects[botnum].ctype.ai_info.REMOTE_OWNER = pnum;
				Objects[botnum].ctype.ai_info.REMOTE_SLOT_NUM = 0;
			}
			else
				Objects[botnum].ctype.ai_info.REMOTE_SLOT_NUM++;
		}
		else
		{
			mprintf((0, "Ignoring position for robot %d not controlled by player %d.\n", botnum, pnum));
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
multi_do_robot_fire(char *buf)
{
	// Send robot fire event
	int loc = 1;
	int botnum;
	short remote_botnum;
	int pnum, gun_num;
	vms_vector fire, gun_point;
	robot_info *robptr;

	pnum = buf[loc];												loc += 1;
	GET_INTEL_SHORT(remote_botnum, buf+loc);
	botnum = objnum_remote_to_local(remote_botnum, (byte)buf[loc+2]); loc += 3;
	gun_num = (byte)buf[loc];											loc += 1;
	memcpy(&fire, buf+loc, sizeof(vms_vector));
	fire.x = (fix)INTEL_INT((int)fire.x);
	fire.y = (fix)INTEL_INT((int)fire.y);
	fire.z = (fix)INTEL_INT((int)fire.z);

	if ((botnum < 0) || (botnum > Highest_object_index) || (Objects[botnum].type != OBJ_ROBOT) || (Objects[botnum].flags & OF_EXPLODING))
	{
//		mprintf((1, "Ignored robot fire from deleted robot.\n"));
//		Int3(); // See Rob, probably not serious tho
		return;
	}
	
	// Do the firing
	
	if (gun_num == -1 || gun_num==-2)
	{
		// Drop proximity bombs
		vm_vec_add(&gun_point, &Objects[botnum].pos, &fire);
	}
	else 
	{
		calc_gun_point(&gun_point, &Objects[botnum], gun_num);
	}
	robptr = &Robot_info[Objects[botnum].id];
	
	if (gun_num == -1) 
		Laser_create_new_easy( &fire, &gun_point, botnum, PROXIMITY_ID, 1);
	else if (gun_num == -2)
		Laser_create_new_easy( &fire, &gun_point, botnum, SUPERPROX_ID, 1);
	else	
		Laser_create_new_easy( &fire, &gun_point, botnum, robptr->weapon_type, 1);
}

extern void drop_stolen_items (object *);

int
multi_explode_robot_sub(int botnum, int killer,char isthief)
{
	object *robot;

	killer = killer;

	if ((botnum < 0) || (botnum > Highest_object_index)) { // Objnum in range?
		Int3(); // See rob
		return 0;
	}

	if (Objects[botnum].type != OBJ_ROBOT) { // Object is robot?
		mprintf((1, "Non-robot explosion ignored.\n"));
//		Int3(); // See rob
		return 0;
	}

	if (Objects[botnum].flags & OF_EXPLODING) { // Object not already exploding
		return 0;
	}

	// Data seems valid, explode the sucker

	if (Network_send_objects && network_objnum_is_past(botnum))
	{
		mprintf((0, "Resetting object sync due to object removal.\n"));
		Network_send_objnum = -1;
	}

	robot = &Objects[botnum];

	mprintf((0, "Robot %d controlled by player %d exploded.\n", botnum, robot->ctype.ai_info.REMOTE_OWNER));

	// Drop non-random KEY powerups locally only!
	if ((robot->contains_count > 0) && (robot->contains_type == OBJ_POWERUP) && (Game_mode & GM_MULTI_COOP) && (robot->contains_id >= POW_KEY_BLUE) && (robot->contains_id <= POW_KEY_GOLD))
	{
		object_create_egg(robot);
	}
	else if (robot->ctype.ai_info.REMOTE_OWNER == Player_num) 
	{
		multi_drop_robot_powerups(robot-Objects);
		multi_delete_controlled_robot(robot-Objects);
	}
	else if (robot->ctype.ai_info.REMOTE_OWNER == -1 && network_i_am_master()) 
	{
		multi_drop_robot_powerups(robot-Objects);
		//multi_delete_controlled_robot(robot-Objects);
	}

   if (isthief || Robot_info[robot->id].thief)
	 drop_stolen_items(robot);

	if (Robot_info[robot->id].boss_flag) {
		if (!Boss_dying)
			start_boss_death_sequence(robot);	
		else
			return (0);
	} else if (Robot_info[robot->id].death_roll) {
		start_robot_death_sequence(robot);
	} else {
		if (robot->id == SPECIAL_REACTOR_ROBOT)
			special_reactor_stuff();
		if (Robot_info[robot->id].kamikaze)
			explode_object(robot,1);	//	Kamikaze, explode right away, IN YOUR FACE!
		else
			explode_object(robot,STANDARD_EXPL_DELAY);
   }
   
	return 1;
}

void
multi_do_robot_explode(char *buf)
{
	// Explode robot controlled by other player

	int botnum;
	short remote_botnum;
	int loc = 1;
	short killer, remote_killer;
	int pnum;
	int rval;
	char thief;

	pnum = buf[loc]; 					loc += 1;
	GET_INTEL_SHORT(remote_killer, buf+loc);
	killer = objnum_remote_to_local(remote_killer, (byte)buf[loc+2]); loc += 3;
	GET_INTEL_SHORT(remote_botnum, buf+loc);
	botnum = objnum_remote_to_local(remote_botnum, (byte)buf[loc+2]); loc += 3;
   thief=buf[loc];

	if ((botnum < 0) || (botnum > Highest_object_index)) {
//		Int3();
		mprintf((0, "Ignored explode message for robot I don't have.\n"));
		return;
	}

	rval = multi_explode_robot_sub(botnum, killer,thief);

	if (rval && (killer == Players[Player_num].objnum))
		add_points_to_score(Robot_info[Objects[botnum].id].score_value);
}

extern fix EnergyToCreateOneRobot; // From fuelcen.c 
extern object *create_morph_robot(segment *segp, vms_vector *object_pos, int object_id); // from fuelcen.c

void
multi_do_create_robot(char *buf)
{
	
	int fuelcen_num = buf[2];
	int pnum = buf[1];
	short objnum;
	int type = buf[5];

	FuelCenter *robotcen;
	vms_vector cur_object_loc, direction;
	object *obj;

	GET_INTEL_SHORT(objnum, buf+3);

	if ((pnum < 0) || (objnum < 0) || (fuelcen_num < 0) || (fuelcen_num >= Num_fuelcenters) || (pnum >= N_players))
	{
		Int3(); // Bogus data
		return;
	}

	robotcen = &Station[fuelcen_num];

	mprintf((1, "NETWORK: Creating morph robot from matcen %d.\n", fuelcen_num)); // DEBUG

	// Play effect and sound

	compute_segment_center(&cur_object_loc, &Segments[robotcen->segnum]);
	obj = object_create_explosion(robotcen->segnum, &cur_object_loc, i2f(10), VCLIP_MORPHING_ROBOT);
	if (obj)
		extract_orient_from_segment(&obj->orient, &Segments[robotcen->segnum]);
	if (Vclip[VCLIP_MORPHING_ROBOT].sound_num > -1)
		digi_link_sound_to_pos( Vclip[VCLIP_MORPHING_ROBOT].sound_num, robotcen->segnum, 0, &cur_object_loc, 0, F1_0 );

	// Set robot center flags, in case we become the master for the next one

	robotcen->Flag = 0;
	robotcen->Capacity -= EnergyToCreateOneRobot;
	robotcen->Timer = 0;

	obj = create_morph_robot(&Segments[robotcen->segnum], &cur_object_loc, type);
	if (obj == NULL)
		return; // Cannot create object!
	
	obj->matcen_creator = (robotcen-Station) | 0x80;
//	extract_orient_from_segment(&obj->orient, &Segments[robotcen->segnum]);
	vm_vec_sub( &direction, &ConsoleObject->pos, &obj->pos );
	vm_vector_2_matrix( &obj->orient, &direction, &obj->orient.uvec, NULL);
	morph_start( obj );

	mprintf((1, "matcen created robot %d (remote %d)\n", obj-Objects, objnum));
	map_objnum_local_to_remote(obj-Objects, objnum, pnum);

	Assert(obj->ctype.ai_info.REMOTE_OWNER == -1);
}

void
multi_do_boss_actions(char *buf)
{
	// Code to handle remote-controlled boss actions

	object *boss_obj;
	int boss_objnum;
	int pnum;
	int action, secondary;
	int loc = 1;
	short remote_objnum, segnum;

	pnum = buf[loc]; 									loc += 1;
	GET_INTEL_SHORT(boss_objnum, buf+loc);              loc += 2;
	action = buf[loc];									loc += 1;
	secondary = buf[loc];								loc += 1;
	GET_INTEL_SHORT(remote_objnum, buf+loc);            loc += 2;
	GET_INTEL_SHORT(segnum, buf+loc);                   loc += 2;
	
	if ((boss_objnum < 0) || (boss_objnum > Highest_object_index))
	{
		Int3();  // See Rob
		return;
	}

	boss_obj = &Objects[boss_objnum];

	if ((boss_obj->type != OBJ_ROBOT) || !(Robot_info[boss_obj->id].boss_flag))
	{
		Int3(); // Got boss actions for a robot who's not a boss?
		return;
	}
		
	mprintf((0, "REMOTE: performing boss action %d.\n", action));

	switch(action) 
	{
		case 1: // Teleport
			{	
				int teleport_segnum;
				vms_vector boss_dir;

				if ((secondary < 0) || (secondary > Num_boss_teleport_segs))
				{
					Int3(); // Bad segnum for boss teleport, ROB!!
					return;
				}
				teleport_segnum = Boss_teleport_segs[secondary];
				mprintf((0, "Boss is teleporting, remove from other's list.\n"));
				if ((teleport_segnum < 0) || (teleport_segnum > Highest_segment_index))
				{
					Int3();  // See Rob
					return;
				}
				compute_segment_center(&boss_obj->pos, &Segments[teleport_segnum]);
				obj_relink(boss_obj-Objects, teleport_segnum);
				Last_teleport_time = GameTime;
		
				vm_vec_sub(&boss_dir, &Objects[Players[pnum].objnum].pos, &boss_obj->pos);
				vm_vector_2_matrix(&boss_obj->orient, &boss_dir, NULL, NULL);

				digi_link_sound_to_pos( Vclip[VCLIP_MORPHING_ROBOT].sound_num, teleport_segnum, 0, &boss_obj->pos, 0 , F1_0);
				digi_kill_sound_linked_to_object( boss_obj-Objects);
				digi_link_sound_to_object2( SOUND_BOSS_SHARE_SEE, boss_obj-Objects, 1, F1_0, F1_0*512 );	//	F1_0*512 means play twice as loud
				Ai_local_info[boss_obj-Objects].next_fire = 0;

				if (boss_obj->ctype.ai_info.REMOTE_OWNER == Player_num)
				{
					mprintf((1, "WARNING: Accepted teleport message for boss when I controlled him!\n"));
					multi_delete_controlled_robot(boss_objnum);
//					robot_controlled[boss_obj->ctype.ai_info.REMOTE_SLOT_NUM] = -1;
				}

				boss_obj->ctype.ai_info.REMOTE_OWNER = -1; // Boss is up for grabs again!
				boss_obj->ctype.ai_info.REMOTE_SLOT_NUM = 0; // Available immediately!
			}
			break;
		case 2: // Cloak
			Boss_hit_time = -F1_0*10;
			Boss_cloak_start_time = GameTime;
			Boss_cloak_end_time = GameTime + Boss_cloak_duration;
			boss_obj->ctype.ai_info.CLOAKED = 1;
			break;
		case 3: // Gate in robots!
			{
				// Do some validity checking
				if ( (remote_objnum >= MAX_OBJECTS) || (remote_objnum < 0) || (segnum < 0) || (segnum > Highest_segment_index) )
				{
					Int3(); // See Rob, bad data in boss gate action message
					return;
				}

				// Gate one in!
				if (gate_in_robot(secondary, segnum))
					map_objnum_local_to_remote(Net_create_objnums[0], remote_objnum, pnum);
				mprintf((0, "REMOTE: Boss gating in robot id %d, objnum %d (%d).\n", secondary, Net_create_objnums[0], remote_objnum));
			}
			break;
		case 4: // Start effect
			restart_effect(BOSS_ECLIP_NUM);
			break;
		case 5:	// Stop effect
			stop_effect(BOSS_ECLIP_NUM);
			break;
		default:
			Int3(); // Illegal type to boss actions
	}
}

void
multi_do_create_robot_powerups(char *buf)
{
	// Code to drop remote-controlled robot powerups

	int loc = 1;
	object del_obj;
	int pnum, egg_objnum, i;

	pnum = buf[loc];										loc += 1;
	del_obj.contains_count = buf[loc];						loc += 1;	
	del_obj.contains_type = buf[loc];						loc += 1;
	del_obj.contains_id = buf[loc]; 						loc += 1;
	GET_INTEL_SHORT(del_obj.segnum, buf+loc);               loc += 2;
	memcpy(&del_obj.pos, buf+loc, sizeof(vms_vector));      loc += 12;
	
	vm_vec_zero(&del_obj.mtype.phys_info.velocity);

	del_obj.pos.x = (fix)INTEL_INT((int)del_obj.pos.x);
	del_obj.pos.y = (fix)INTEL_INT((int)del_obj.pos.y);
	del_obj.pos.z = (fix)INTEL_INT((int)del_obj.pos.z);

	Assert((pnum >= 0) && (pnum < N_players));
	Assert (pnum!=Player_num); // What? How'd we send ourselves this?

	Net_create_loc = 0;
	d_srand(1245L);

	egg_objnum = object_create_egg(&del_obj);

	if (egg_objnum == -1)
		return; // Object buffer full

//	Assert(egg_objnum > -1);
	Assert((Net_create_loc > 0) && (Net_create_loc <= MAX_ROBOT_POWERUPS));

	for (i = 0; i < Net_create_loc; i++)
	{
		short s;
		
		GET_INTEL_SHORT(s, buf+loc);
		if ( s != -1)
			map_objnum_local_to_remote((short)Net_create_objnums[i], s, pnum);
		else
			Objects[Net_create_objnums[i]].flags |= OF_SHOULD_BE_DEAD; // Delete objects other guy didn't create one of
		loc += 2;
	}
}

void
multi_drop_robot_powerups(int objnum)
{
	// Code to handle dropped robot powerups in network mode ONLY!

	object *del_obj;
	int egg_objnum = -1;
	robot_info	*robptr; 

	if ((objnum < 0) || (objnum > Highest_object_index))
	{
		Int3();  // See rob
		return;
	}
	
	del_obj = &Objects[objnum];

	if (del_obj->type != OBJ_ROBOT)
	{
		Int3(); // dropping powerups for non-robot, Rob's fault
		return;
	}

	robptr = &Robot_info[del_obj->id];

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
		d_srand(timer_get_approx_seconds());
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

	if (egg_objnum >= 0) {
		// Transmit the object creation to the other players	 	
		mprintf((0, "Dropped %d powerups for robot %d.\n", Net_create_loc, del_obj-Objects));
		multi_send_create_robot_powerups(del_obj);
	}
}

//	-----------------------------------------------------------------------------
//	Robot *robot got whacked by player player_num and requests permission to do something about it.
//	Note: This function will be called regardless of whether Game_mode is a multiplayer mode, so it
//	should quick-out if not in a multiplayer mode.  On the other hand, it only gets called when a
//	player or player weapon whacks a robot, so it happens rarely.
void multi_robot_request_change(object *robot, int player_num)
{
	int slot, remote_objnum;
	byte dummy;

	if (!(Game_mode & GM_MULTI_ROBOTS))
		return;
	
//	if (robot->ctype.ai_info.REMOTE_OWNER == Player_num)
//		return;

	slot = robot->ctype.ai_info.REMOTE_SLOT_NUM;

	if ((slot < 0) || (slot >= MAX_ROBOTS_CONTROLLED)) {
		Int3();
		return;
	}

	remote_objnum = objnum_local_to_remote(robot-Objects, &dummy);
	if (remote_objnum < 0)
		return;

	mprintf((0, "request_change(): my pri %d, player %d's pri %d.\n", MULTI_ROBOT_PRIORITY(remote_objnum, Player_num),
			  player_num, MULTI_ROBOT_PRIORITY(remote_objnum, player_num)));

	if ( (robot_agitation[slot] < 70) || (MULTI_ROBOT_PRIORITY(remote_objnum, player_num) > MULTI_ROBOT_PRIORITY(remote_objnum, Player_num)) || (d_rand() > 0x4400))
	{
		mprintf((0, "Robot %d (%d) released because it got hit by Player %d.\n", robot-Objects, remote_objnum, player_num));
		if (robot_send_pending[slot])
			multi_send_robot_position(robot_controlled[slot], -1);
		multi_send_release_robot(robot_controlled[slot]);
		robot->ctype.ai_info.REMOTE_SLOT_NUM = 5;  // Hands-off period
	}
}
