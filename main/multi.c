/* $Id: multi.c,v 1.17 2004-10-23 18:59:02 schaffner Exp $ */
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
 * Multiplayer code shared by serial and network play.
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "u_mem.h"
#include "strutil.h"
#include "game.h"
#include "modem.h"
#include "network.h"
#include "multi.h"
#include "object.h"
#include "laser.h"
#include "fuelcen.h"
#include "scores.h"
#include "gauges.h"
#include "collide.h"
#include "error.h"
#include "fireball.h"
#include "newmenu.h"
#include "mono.h"
#include "wall.h"
#include "cntrlcen.h"
#include "powerup.h"
#include "polyobj.h"
#include "bm.h"
#include "endlevel.h"
#include "key.h"
#include "playsave.h"
#include "timer.h"
#include "digi.h"
#include "sounds.h"
#include "kconfig.h"
#include "newdemo.h"
#include "text.h"
#include "kmatrix.h"
#include "multibot.h"
#include "gameseq.h"
#include "physics.h"
#include "config.h"
#include "state.h"
#include "ai.h"
#include "switch.h"
#include "textures.h"
#include "byteswap.h"
#include "sounds.h"
#include "args.h"
#include "cfile.h"
#include "effects.h"

void multi_reset_player_object(object *objp);
void multi_reset_object_texture(object *objp);
void multi_add_lifetime_killed();
void multi_add_lifetime_kills();
void multi_send_play_by_play(int num,int spnum,int dpnum);
void multi_send_heartbeat();
void multi_send_modem_ping();
void multi_cap_objects();
void multi_adjust_remote_cap(int pnum);
void multi_save_game(ubyte slot, uint id, char *desc);
void multi_restore_game(ubyte slot, uint id);
void multi_set_robot_ai(void);
void multi_send_powerup_update();
void bash_to_shield(int i,char *s);
void init_hoard_data();
void multi_apply_goal_textures();
int  find_goal_texture(ubyte t);
void multi_bad_restore();
void multi_do_capture_bonus(char *buf);
void multi_do_orb_bonus(char *buf);
void multi_send_drop_flag(int objnum,int seed);
void multi_send_ranking();
void multi_do_play_by_play(char *buf);

//
// Local macros and prototypes
//

// LOCALIZE ME!!

#define vm_angvec_zero(v) (v)->p=(v)->b=(v)->h=0

void drop_player_eggs(object *player); // from collide.c
void GameLoop(int, int); // From game.c

//
// Global variables
//

extern vms_vector MarkerPoint[];
extern char MarkerMessage[16][40];
extern char MarkerOwner[16][40];
extern int MarkerObject[];

int control_invul_time = 0;
int who_killed_controlcen = -1;  // -1 = noone

//do we draw the kill list on the HUD?
int Show_kill_list = 1;
int Show_reticle_name = 1;
fix Show_kill_list_timer = 0;

char Multi_is_guided=0;
char PKilledFlags[MAX_NUM_NET_PLAYERS];

int multi_sending_message = 0;
int multi_defining_message = 0;
int multi_message_index = 0;

char multibuf[MAX_MULTI_MESSAGE_LEN+4];            // This is where multiplayer message are built

short remote_to_local[MAX_NUM_NET_PLAYERS][MAX_OBJECTS];  // Remote object number for each local object
short local_to_remote[MAX_OBJECTS];
sbyte object_owner[MAX_OBJECTS];   // Who created each object in my universe, -1 = loaded at start

int   Net_create_objnums[MAX_NET_CREATE_OBJECTS]; // For tracking object creation that will be sent to remote
int   Net_create_loc = 0;       // pointer into previous array
int   Network_laser_fired = 0;  // How many times we shot
int   Network_laser_gun;        // Which gun number we shot
int   Network_laser_flags;      // Special flags for the shot
int   Network_laser_level;      // What level
short Network_laser_track;      // Who is it tracking?
char  Network_message[MAX_MESSAGE_LEN];
char  Network_message_macro[4][MAX_MESSAGE_LEN];
int   Network_message_reciever=-1;
int   sorted_kills[MAX_NUM_NET_PLAYERS];
short kill_matrix[MAX_NUM_NET_PLAYERS][MAX_NUM_NET_PLAYERS];
int   multi_goto_secret = 0;
short team_kills[2];
int   multi_in_menu = 0;
int   multi_leave_menu = 0;
int   multi_quit_game = 0;

netgame_info Netgame;
AllNetPlayers_info NetPlayers;

bitmap_index multi_player_textures[MAX_NUM_NET_PLAYERS][N_PLAYER_SHIP_TEXTURES];

typedef struct netplayer_stats {
	ubyte  message_type;
	ubyte  Player_num;              // Who am i?
	uint   flags;                   // Powerup flags, see below...
	fix    energy;                  // Amount of energy remaining.
	fix    shields;                 // shields remaining (protection)
	ubyte  lives;                   // Lives remaining, 0 = game over.
	ubyte  laser_level;             // Current level of the laser.
	ubyte  primary_weapon_flags;    // bit set indicates the player has this weapon.
	ubyte  secondary_weapon_flags;  // bit set indicates the player has this weapon.
	ushort primary_ammo[MAX_PRIMARY_WEAPONS];     // How much ammo of each type.
	ushort secondary_ammo[MAX_SECONDARY_WEAPONS]; // How much ammo of each type.
	int    last_score;              // Score at beginning of current level.
	int    score;                   // Current score.
	fix    cloak_time;              // Time cloaked
	fix    invulnerable_time;       // Time invulnerable
	fix    homing_object_dist;      // Distance of nearest homing object.
	short  KillGoalCount;
	short  net_killed_total;        // Number of times killed total
	short  net_kills_total;         // Number of net kills total
	short  num_kills_level;         // Number of kills this level
	short  num_kills_total;         // Number of kills total
	short  num_robots_level;        // Number of initial robots this level
	short  num_robots_total;        // Number of robots total
	ushort hostages_rescued_total;  // Total number of hostages rescued.
	ushort hostages_total;          // Total number of hostages.
	ubyte  hostages_on_board;       // Number of hostages on ship.
	ubyte  unused[16];
} netplayer_stats;

int message_length[MULTI_MAX_TYPE+1] = {
	24, // POSITION
	3,  // REAPPEAR
	8,  // FIRE
	5,  // KILL
	4,  // REMOVE_OBJECT
	97+9, // PLAYER_EXPLODE
	37, // MESSAGE (MAX_MESSAGE_LENGTH = 40)
	2,  // QUIT
	4,  // PLAY_SOUND
	41, // BEGIN_SYNC
	4,  // CONTROLCEN
	5,  // CLAIM ROBOT
	4,  // END_SYNC
	2,  // CLOAK
	3,  // ENDLEVEL_START
	5,  // DOOR_OPEN
	2,  // CREATE_EXPLOSION
	16, // CONTROLCEN_FIRE
	97+9, // PLAYER_DROP
	19, // CREATE_POWERUP
	9,  // MISSILE_TRACK
	2,  // DE-CLOAK
	2,  // MENU_CHOICE
	28, // ROBOT_POSITION  (shortpos_length (23) + 5 = 28)
	9,  // ROBOT_EXPLODE
	5,  // ROBOT_RELEASE
	18, // ROBOT_FIRE
	6,  // SCORE
	6,  // CREATE_ROBOT
	3,  // TRIGGER
	10, // BOSS_ACTIONS
	27, // ROBOT_POWERUPS
	7,  // HOSTAGE_DOOR
	2+24, // SAVE_GAME      (ubyte slot, uint id, char name[20])
	2+4,  // RESTORE_GAME   (ubyte slot, uint id)
	1+1,  // MULTI_REQ_PLAYER
	sizeof(netplayer_stats), // MULTI_SEND_PLAYER
	55, // MULTI_MARKER
	12, // MULTI_DROP_WEAPON
#ifndef MACINTOSH
	3+sizeof(shortpos), // MULTI_GUIDED, IF SHORTPOS CHANGES, CHANGE MAC VALUE BELOW
#else
	26, // MULTI_GUIDED IF SIZE OF SHORTPOS CHANGES, CHANGE THIS VALUE AS WELL!!!!!!
#endif
	11, // MULTI_STOLEN_ITEMS
	6,  // MULTI_WALL_STATUS
	5,  // MULTI_HEARTBEAT
	9,  // MULTI_KILLGOALS
	9,  // MULTI_SEISMIC
	18, // MULTI_LIGHT
	2,  // MULTI_START_TRIGGER
	6,  // MULTI_FLAGS
	2,  // MULTI_DROP_BLOB
	MAX_POWERUP_TYPES+1, // MULTI_POWERUP_UPDATE
	sizeof(active_door)+3, // MULTI_ACTIVE_DOOR
	4,  // MULTI_SOUND_FUNCTION
	2,  // MULTI_CAPTURE_BONUS
	2,  // MULTI_GOT_FLAG
	12, // MULTI_DROP_FLAG
	142, // MULTI_ROBOT_CONTROLS
	2,  // MULTI_FINISH_GAME
	3,  // MULTI_RANK
	1,  // MULTI_MODEM_PING
	1,  // MULTI_MODEM_PING_RETURN
	3,  // MULTI_ORB_BONUS
	2,  // MULTI_GOT_ORB
	12, // MULTI_DROP_ORB
	4,  // MULTI_PLAY_BY_PLAY
};

void extract_netplayer_stats( netplayer_stats *ps, player * pd );
void use_netplayer_stats( player * ps, netplayer_stats *pd );
char PowerupsInMine[MAX_POWERUP_TYPES],MaxPowerupsAllowed[MAX_POWERUP_TYPES];
extern fix ThisLevelTime;

//
//  Functions that replace what used to be macros
//

int objnum_remote_to_local(int remote_objnum, int owner)
{
	// Map a remote object number from owner to a local object number

	int result;

	if ((owner >= N_players) || (owner < -1)) {
		Int3(); // Illegal!
		return(remote_objnum);
	}

	if (owner == -1)
		return(remote_objnum);

	if ((remote_objnum < 0) || (remote_objnum >= MAX_OBJECTS))
		return(-1);

	result = remote_to_local[owner][remote_objnum];

	if (result < 0)
	{
		mprintf((1, "Remote object owner %d number %d mapped to -1!\n", owner, remote_objnum));
		return(-1);
	}

#ifndef NDEBUG
	if (object_owner[result] != owner)
	{
		mprintf((1, "Remote object owner %d number %d doesn't match owner %d.\n", owner, remote_objnum, object_owner[result]));
	}
#endif
	//Assert(object_owner[result] == owner);

	return(result);
}

int objnum_local_to_remote(int local_objnum, sbyte *owner)
{
	// Map a local object number to a remote + owner

	int result;

	if ((local_objnum < 0) || (local_objnum > Highest_object_index)) {
		*owner = -1;
		return(-1);
	}

	*owner = object_owner[local_objnum];

	if (*owner == -1)
		return(local_objnum);

	if ((*owner >= N_players) || (*owner < -1)) {
		Int3(); // Illegal!
		*owner = -1;
		return local_objnum;
	}

	result = local_to_remote[local_objnum];

	//mprintf((0, "Local object %d mapped to owner %d objnum %d.\n", local_objnum,
	//	*owner, result));

	if (result < 0)
	{
		Int3(); // See Rob, object has no remote number!
	}

	return(result);
}

void
map_objnum_local_to_remote(int local_objnum, int remote_objnum, int owner)
{
	// Add a mapping from a network remote object number to a local one

	Assert(local_objnum > -1);
	Assert(local_objnum < MAX_OBJECTS);
	Assert(remote_objnum > -1);
	Assert(remote_objnum < MAX_OBJECTS);
	Assert(owner > -1);
	Assert(owner != Player_num);

	object_owner[local_objnum] = owner;

	remote_to_local[owner][remote_objnum] = local_objnum;
	local_to_remote[local_objnum] = remote_objnum;

	return;
}

void
map_objnum_local_to_local(int local_objnum)
{
	// Add a mapping for our locally created objects

	Assert(local_objnum > -1);
	Assert(local_objnum < MAX_OBJECTS);

	object_owner[local_objnum] = Player_num;
	remote_to_local[Player_num][local_objnum] = local_objnum;
	local_to_remote[local_objnum] = local_objnum;

	return;
}

void reset_network_objects()
{
	memset(local_to_remote, -1, MAX_OBJECTS*sizeof(short));
	memset(remote_to_local, -1, MAX_NUM_NET_PLAYERS*MAX_OBJECTS*sizeof(short));
	memset(object_owner, -1, MAX_OBJECTS);
}

//
// Part 1 : functions whose main purpose in life is to divert the flow
//          of execution to either network or serial specific code based
//          on the curretn Game_mode value.
//

void
multi_endlevel_score(void)
{
	int old_connect=0;
	int i;

	// Show a score list to end of net players

	// Save connect state and change to new connect state
#ifdef NETWORK
	if (Game_mode & GM_NETWORK)
	{
		old_connect = Players[Player_num].connected;
		if (Players[Player_num].connected!=3)
			Players[Player_num].connected = CONNECT_END_MENU;
		Network_status = NETSTAT_ENDLEVEL;

	}
#endif

	// Do the actual screen we wish to show

	Function_mode = FMODE_MENU;

	kmatrix_view(Game_mode & GM_NETWORK);

	Function_mode = FMODE_GAME;

	// Restore connect state

	if (Game_mode & GM_NETWORK)
	{
		Players[Player_num].connected = old_connect;
	}

#ifndef SHAREWARE
	if (Game_mode & GM_MULTI_COOP)
	{
		int i;
		for (i = 0; i < MaxNumNetPlayers; i++)
			// Reset keys
			Players[i].flags &= ~(PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_GOLD_KEY);
	}
	for (i = 0; i < MaxNumNetPlayers; i++)
		Players[i].flags &= ~(PLAYER_FLAGS_FLAG);  // Clear capture flag

#endif

	for (i=0;i<MAX_PLAYERS;i++)
		Players[i].KillGoalCount=0;

	for (i=0;i<MAX_POWERUP_TYPES;i++)
	{
		MaxPowerupsAllowed[i]=0;
		PowerupsInMine[i]=0;
	}

}

int
get_team(int pnum)
{
	if ((Game_mode & GM_CAPTURE) && ((Game_mode & GM_SERIAL) || (Game_mode & GM_MODEM)))
		return pnum;

	if (Netgame.team_vector & (1 << pnum))
		return 1;
	else
		return 0;
}

extern void game_disable_cheats();

void
multi_new_game(void)
{
	int i;

	// Reset variables for a new net game

	memset(kill_matrix, 0, MAX_NUM_NET_PLAYERS*MAX_NUM_NET_PLAYERS*2); // Clear kill matrix

	for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	{
		sorted_kills[i] = i;
		Players[i].net_killed_total = 0;
		Players[i].net_kills_total = 0;
		Players[i].flags = 0;
		Players[i].KillGoalCount=0;
	}

#ifndef SHAREWARE
	for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++)
	{
		robot_controlled[i] = -1;
		robot_agitation[i] = 0;
		robot_fired[i] = 0;
	}
#endif

	team_kills[0] = team_kills[1] = 0;
	Endlevel_sequence = 0;
	Player_is_dead = 0;
	multi_leave_menu = 0;
	multi_quit_game = 0;
	Show_kill_list = 1;
	game_disable_cheats();
	Player_exploded = 0;
	Dead_player_camera = 0;
}

void
multi_make_player_ghost(int playernum)
{
	object *obj;

	//Assert(playernum != Player_num);
	//Assert(playernum < MAX_NUM_NET_PLAYERS);

	if ((playernum == Player_num) || (playernum >= MAX_NUM_NET_PLAYERS) || (playernum < 0))
	{
		Int3(); // Non-terminal, see Rob
		return;
	}

//      if (Objects[Players[playernum].objnum].type != OBJ_PLAYER)
//              mprintf((1, "Warning: Player %d is not currently a player.\n", playernum));

	obj = &Objects[Players[playernum].objnum];

	obj->type = OBJ_GHOST;
	obj->render_type = RT_NONE;
	obj->movement_type = MT_NONE;
	multi_reset_player_object(obj);

	if (Game_mode & GM_MULTI_ROBOTS)
		multi_strip_robots(playernum);
}

void
multi_make_ghost_player(int playernum)
{
	object *obj;

// Assert(playernum != Player_num);
// Assert(playernum < MAX_NUM_NET_PLAYERS);

	if ((playernum == Player_num) || (playernum >= MAX_NUM_NET_PLAYERS))
	{
		Int3(); // Non-terminal, see rob
		return;
	}

//      if(Objects[Players[playernum].objnum].type != OBJ_GHOST)
//              mprintf((1, "Warning: Player %d is not currently a ghost.\n", playernum));

	obj = &Objects[Players[playernum].objnum];

	obj->type = OBJ_PLAYER;
	obj->movement_type = MT_PHYSICS;
	multi_reset_player_object(obj);
}

int multi_get_kill_list(int *plist)
{
	// Returns the number of active net players and their
	// sorted order of kills
	int i;
	int n = 0;

	for (i = 0; i < N_players; i++)
		//if (Players[sorted_kills[i]].connected)
		plist[n++] = sorted_kills[i];

	if (n == 0)
		Int3(); // SEE ROB OR MATT

	//memcpy(plist, sorted_kills, N_players*sizeof(int));

	return(n);
}

void
multi_sort_kill_list(void)
{
	// Sort the kills list each time a new kill is added

	int kills[MAX_NUM_NET_PLAYERS];
	int i;
	int changed = 1;

	for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	{
#ifndef SHAREWARE
		if (Game_mode & GM_MULTI_COOP)
			kills[i] = Players[i].score;
		else
#endif
		if (Show_kill_list==2)
		{
			if (Players[i].net_killed_total+Players[i].net_kills_total==0)
				kills[i]=-1;  // always draw the ones without any ratio last
			else
				kills[i]=(int)((float)((float)Players[i].net_kills_total/((float)Players[i].net_killed_total+(float)Players[i].net_kills_total))*100.0);
		}
		else
			kills[i] = Players[i].net_kills_total;
	}

	while (changed)
	{
		changed = 0;
		for (i = 0; i < N_players-1; i++)
		{
			if (kills[sorted_kills[i]] < kills[sorted_kills[i+1]])
			{
				changed = sorted_kills[i];
				sorted_kills[i] = sorted_kills[i+1];
				sorted_kills[i+1] = changed;
				changed = 1;
			}
		}
	}
	mprintf((0, "Sorted kills %d %d.\n", sorted_kills[0], sorted_kills[1]));
}

extern object *obj_find_first_of_type (int);
char Multi_killed_yourself=0;

void multi_compute_kill(int killer, int killed)
{
	// Figure out the results of a network kills and add it to the
	// appropriate player's tally.

	int killed_pnum, killed_type;
	int killer_pnum, killer_type,killer_id;
	int TheGoal;
	char killed_name[(CALLSIGN_LEN*2)+4];
	char killer_name[(CALLSIGN_LEN*2)+4];

	kmatrix_kills_changed = 1;
	Multi_killed_yourself=0;

	// Both object numbers are localized already!

	mprintf((0, "compute_kill passed: object %d killed object %d.\n", killer, killed));

	if ((killed < 0) || (killed > Highest_object_index) || (killer < 0) || (killer > Highest_object_index))
	{
		Int3(); // See Rob, illegal value passed to compute_kill;
		return;
	}

	killed_type = Objects[killed].type;
	killer_type = Objects[killer].type;
	killer_id = Objects[killer].id;

	if ((killed_type != OBJ_PLAYER) && (killed_type != OBJ_GHOST))
	{
		Int3(); // compute_kill passed non-player object!
		return;
	}

	killed_pnum = Objects[killed].id;

	PKilledFlags[killed_pnum]=1;

	Assert ((killed_pnum >= 0) && (killed_pnum < N_players));

	if (Game_mode & GM_TEAM)
		sprintf(killed_name, "%s (%s)", Players[killed_pnum].callsign, Netgame.team_name[get_team(killed_pnum)]);
	else
		sprintf(killed_name, "%s", Players[killed_pnum].callsign);

	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_multi_death(killed_pnum);

	digi_play_sample( SOUND_HUD_KILL, F3_0 );

	if (Control_center_destroyed)
		Players[killed_pnum].connected=3;

	if (killer_type == OBJ_CNTRLCEN)
	{
		Players[killed_pnum].net_killed_total++;
		Players[killed_pnum].net_kills_total--;

		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_multi_kill(killed_pnum, -1);

		if (killed_pnum == Player_num)
		{
			HUD_init_message("%s %s.", TXT_YOU_WERE, TXT_KILLED_BY_NONPLAY);
			multi_add_lifetime_killed ();
		}
		else
			HUD_init_message("%s %s %s.", killed_name, TXT_WAS, TXT_KILLED_BY_NONPLAY );
		return;
	}

#ifndef SHAREWARE
	else if ((killer_type != OBJ_PLAYER) && (killer_type != OBJ_GHOST))
	{
		if (killer_id==PMINE_ID && killer_type!=OBJ_ROBOT)
		{
			if (killed_pnum == Player_num)
				HUD_init_message("You were killed by a mine!");
			else
				HUD_init_message("%s was killed by a mine!",killed_name);
		}
		else
		{
			if (killed_pnum == Player_num)
			{
				HUD_init_message("%s %s.", TXT_YOU_WERE, TXT_KILLED_BY_ROBOT);
				multi_add_lifetime_killed();
			}
			else
				HUD_init_message("%s %s %s.", killed_name, TXT_WAS, TXT_KILLED_BY_ROBOT );
		}
		Players[killed_pnum].net_killed_total++;
		return;
	}
#else
	else if ((killer_type != OBJ_PLAYER) && (killer_type != OBJ_GHOST) && (killer_id!=PMINE_ID))
	{
		Int3(); // Illegal killer type?
		return;
	}
	if (killer_id==PMINE_ID)
	{
		if (killed_pnum==Player_num)
			HUD_init_message("You were killed by a mine!");
		else
			HUD_init_message("%s was killed by a mine!",killed_name);

		Players[killed_pnum].net_killed_total++;

		return;
	}
#endif

	killer_pnum = Objects[killer].id;

	if (Game_mode & GM_TEAM)
		sprintf(killer_name, "%s (%s)", Players[killer_pnum].callsign, Netgame.team_name[get_team(killer_pnum)]);
	else
		sprintf(killer_name, "%s", Players[killer_pnum].callsign);

	// Beyond this point, it was definitely a player-player kill situation

	if ((killer_pnum < 0) || (killer_pnum >= N_players))
		Int3(); // See rob, tracking down bug with kill HUD messages
	if ((killed_pnum < 0) || (killed_pnum >= N_players))
		Int3(); // See rob, tracking down bug with kill HUD messages

	if (killer_pnum == killed_pnum)
	{
		if (!(Game_mode & GM_HOARD))
		{
			if (Game_mode & GM_TEAM)
				team_kills[get_team(killed_pnum)] -= 1;

			Players[killed_pnum].net_killed_total += 1;
			Players[killed_pnum].net_kills_total -= 1;

			if (Newdemo_state == ND_STATE_RECORDING)
				newdemo_record_multi_kill(killed_pnum, -1);
		}
		kill_matrix[killed_pnum][killed_pnum] += 1; // # of suicides

		if (killer_pnum == Player_num)
		{
			HUD_init_message("%s %s %s!", TXT_YOU, TXT_KILLED, TXT_YOURSELF );
			Multi_killed_yourself=1;
			multi_add_lifetime_killed();
		}
		else
			HUD_init_message("%s %s", killed_name, TXT_SUICIDE);
	}

	else
	{
		if (!(Game_mode & GM_HOARD))
		{
			if (Game_mode & GM_TEAM)
			{
				if (get_team(killed_pnum) == get_team(killer_pnum))
					team_kills[get_team(killed_pnum)] -= 1;
				else
					team_kills[get_team(killer_pnum)] += 1;
			}

			Players[killer_pnum].net_kills_total += 1;
			Players[killer_pnum].KillGoalCount+=1;

			if (Newdemo_state == ND_STATE_RECORDING)
				newdemo_record_multi_kill(killer_pnum, 1);
		}
		else
		{
			if (Game_mode & GM_TEAM)
			{
				if (killed_pnum==Player_num && get_team(killed_pnum) == get_team(killer_pnum))
					Multi_killed_yourself=1;
			}
		}

		kill_matrix[killer_pnum][killed_pnum] += 1;
		Players[killed_pnum].net_killed_total += 1;
		if (killer_pnum == Player_num) {
			HUD_init_message("%s %s %s!", TXT_YOU, TXT_KILLED, killed_name);
			multi_add_lifetime_kills();
			if ((Game_mode & GM_MULTI_COOP) && (Players[Player_num].score >= 1000))
				add_points_to_score(-1000);
		}
		else if (killed_pnum == Player_num)
		{
			HUD_init_message("%s %s %s!", killer_name, TXT_KILLED, TXT_YOU);
			multi_add_lifetime_killed();
			if (Game_mode & GM_HOARD)
			{
				if (Players[Player_num].secondary_ammo[PROXIMITY_INDEX]>3)
					multi_send_play_by_play (1,killer_pnum,Player_num);
				else if (Players[Player_num].secondary_ammo[PROXIMITY_INDEX]>0)
					multi_send_play_by_play (0,killer_pnum,Player_num);
			}
		}
		else
			HUD_init_message("%s %s %s!", killer_name, TXT_KILLED, killed_name);
	}

	TheGoal=Netgame.KillGoal*5;

	if (Netgame.KillGoal>0)
	{
		if (Players[killer_pnum].KillGoalCount>=TheGoal)
		{
			if (killer_pnum==Player_num)
			{
				HUD_init_message("You reached the kill goal!");
				Players[Player_num].shields=i2f(200);
			}
			else
				HUD_init_message ("%s has reached the kill goal!",Players[killer_pnum].callsign);

			HUD_init_message ("The control center has been destroyed!");
			net_destroy_controlcen (obj_find_first_of_type (OBJ_CNTRLCEN));
		}
	}

	multi_sort_kill_list();
	multi_show_player_list();
	Players[killed_pnum].flags&=(~(PLAYER_FLAGS_HEADLIGHT_ON));  // clear the killed guys flags/headlights
}

void
multi_do_frame(void)
{
	static int lasttime=0;
	int i;

	if (!(Game_mode & GM_MULTI))
	{
		Int3();
		return;
	}

	if ((Game_mode & GM_NETWORK) && Netgame.PlayTimeAllowed && lasttime!=f2i (ThisLevelTime))
	{
		for (i=0;i<N_players;i++)
			if (Players[i].connected)
			{
				if (i==Player_num)
				{
					multi_send_heartbeat();
					lasttime=f2i(ThisLevelTime);
				}
				break;
			}
	}

	multi_send_message(); // Send any waiting messages

	if (!multi_in_menu)
		multi_leave_menu = 0;

#ifndef SHAREWARE
	if (Game_mode & GM_MULTI_ROBOTS)
	{
		multi_check_robot_timeout();
	}
#endif

	if ((Game_mode & GM_SERIAL) || (Game_mode & GM_MODEM))
	{
		com_do_frame();
	}
	else
	{
		network_do_frame(0, 1);
	}

	if (multi_quit_game && !multi_in_menu)
	{
		multi_quit_game = 0;
		longjmp(LeaveGame, 0);
	}
}

void
multi_send_data(char *buf, int len, int repeat)
{
	Assert(len == message_length[(int)buf[0]]);
	Assert(buf[0] <= MULTI_MAX_TYPE);
	//      Assert(buf[0] >= 0);

	if (Game_mode & GM_NETWORK)
		Assert(buf[0] > 0);

	if ((Game_mode & GM_SERIAL) || (Game_mode & GM_MODEM))
		com_send_data(buf, len, repeat);
	else if (Game_mode & GM_NETWORK)
		network_send_data((unsigned char *)buf, len, repeat);
}

void
multi_leave_game(void)
{

	//      if (Function_mode != FMODE_GAME)
	//              return;

	if (!(Game_mode & GM_MULTI))
		return;

	if (Game_mode & GM_NETWORK)
	{
		mprintf((0, "Sending explosion message.\n"));

		Net_create_loc = 0;
		AdjustMineSpawn();
		multi_cap_objects();
		drop_player_eggs(ConsoleObject);
		multi_send_position(Players[Player_num].objnum);
		multi_send_player_explode(MULTI_PLAYER_DROP);
	}

	mprintf((1, "Sending leave game.\n"));
	multi_send_quit(MULTI_QUIT);

	if ((Game_mode & GM_SERIAL) || (Game_mode & GM_MODEM))
		serial_leave_game();
	if (Game_mode & GM_NETWORK)
		network_leave_game();

	Game_mode |= GM_GAME_OVER;

	if (Function_mode!=FMODE_EXIT)
		Function_mode = FMODE_MENU;

	//      N_players = 0;

	//      change_playernum_to(0);
	//      Viewer = ConsoleObject = &Objects[0];

}

void
multi_show_player_list()
{
	if (!(Game_mode & GM_MULTI) || (Game_mode & GM_MULTI_COOP))
		return;

	if (Show_kill_list)
		return;

	Show_kill_list_timer = F1_0*5; // 5 second timer
	Show_kill_list = 1;
}

int
multi_endlevel(int *secret)
{
	int result = 0;

	if ((Game_mode & GM_SERIAL) || (Game_mode & GM_MODEM))
		com_endlevel(secret);          // an opportunity to re-sync or whatever
	else if (Game_mode & GM_NETWORK)
		result = network_endlevel(secret);

	return(result);
}

//
// Part 2 : functions that act on network/serial messages and change the
//          the state of the game in some way.
//

#ifndef MACINTOSH
//extern PORT *com_port;
#endif

int
multi_menu_poll(void)
{
	fix old_shields;
	int was_fuelcen_alive;
	int player_was_dead;

	was_fuelcen_alive = Control_center_destroyed;

	// Special polling function for in-game menus for multiplayer and serial

	if (! ((Game_mode & GM_MULTI) && (Function_mode == FMODE_GAME)) )
		return(0);

	if (multi_leave_menu)
		return(-1);

	old_shields = Players[Player_num].shields;
	player_was_dead = Player_is_dead;

	multi_in_menu++; // Track level of menu nesting

	GameLoop( 0, 0 );

	multi_in_menu--;

	timer_delay(f0_1);   // delay 100 milliseconds

	if (Endlevel_sequence || (Control_center_destroyed && !was_fuelcen_alive) || (Player_is_dead != player_was_dead) || (Players[Player_num].shields < old_shields))
	{
		multi_leave_menu = 1;
		return(-1);
	}
	if ((Control_center_destroyed) && (Countdown_seconds_left < 10))
	{
		multi_leave_menu = 1;
		return(-1);
	}

#if 0
	if ((Game_mode & GM_MODEM) && (!GetCd(com_port)))
	{
		multi_leave_menu = 1;
		return(-1);
	}
#endif

	return(0);
}

void
multi_define_macro(int key)
{
	if (!(Game_mode & GM_MULTI))
		return;

	key &= (~KEY_SHIFTED);

	switch(key)
	{
	case KEY_F9:
		multi_defining_message = 1; break;
	case KEY_F10:
		multi_defining_message = 2; break;
	case KEY_F11:
		multi_defining_message = 3; break;
	case KEY_F12:
		multi_defining_message = 4; break;
	default:
		Int3();
	}

	if (multi_defining_message)     {
		multi_message_index = 0;
		Network_message[multi_message_index] = 0;
	}

}

char feedback_result[200];

void
multi_message_feedback(void)
{
	char *colon;
	int found = 0;
	int i;

	if (!( ((colon = strrchr(Network_message, ':')) == NULL) || (colon-Network_message < 1) || (colon-Network_message > CALLSIGN_LEN) ))
	{
		sprintf(feedback_result, "%s ", TXT_MESSAGE_SENT_TO);
		if ((Game_mode & GM_TEAM) && (atoi(Network_message) > 0) && (atoi(Network_message) < 3))
		{
			sprintf(feedback_result+strlen(feedback_result), "%s '%s'", TXT_TEAM, Netgame.team_name[atoi(Network_message)-1]);
			found = 1;
		}
		if (Game_mode & GM_TEAM)
		{
			for (i = 0; i < N_players; i++)
			{
				if (!strnicmp(Netgame.team_name[i], Network_message, colon-Network_message))
				{
					if (found)
						strcat(feedback_result, ", ");
					found++;
					if (!(found % 4))
						strcat(feedback_result, "\n");
					sprintf(feedback_result+strlen(feedback_result), "%s '%s'", TXT_TEAM, Netgame.team_name[i]);
				}
			}
		}
		for (i = 0; i < N_players; i++)
		{
			if ((!strnicmp(Players[i].callsign, Network_message, colon-Network_message)) && (i != Player_num) && (Players[i].connected))
			{
				if (found)
					strcat(feedback_result, ", ");
				found++;
				if (!(found % 4))
					strcat(feedback_result, "\n");
				sprintf(feedback_result+strlen(feedback_result), "%s", Players[i].callsign);
			}
		}
		if (!found)
			strcat(feedback_result, TXT_NOBODY);
		else
			strcat(feedback_result, ".");

		digi_play_sample(SOUND_HUD_MESSAGE, F1_0);

		Assert(strlen(feedback_result) < 200);

		HUD_init_message(feedback_result);
		//sprintf (temp,"%s",colon);
		//sprintf (Network_message,"%s",temp);

	}
}

void
multi_send_macro(int key)
{
	if (! (Game_mode & GM_MULTI) )
		return;

	switch(key)
	{
	case KEY_F9:
		key = 0; break;
	case KEY_F10:
		key = 1; break;
	case KEY_F11:
		key = 2; break;
	case KEY_F12:
		key = 3; break;
	default:
		Int3();
	}

	if (!Network_message_macro[key][0])
	{
		HUD_init_message(TXT_NO_MACRO);
		return;
	}

	strcpy(Network_message, Network_message_macro[key]);
	Network_message_reciever = 100;

	HUD_init_message("%s '%s'", TXT_SENDING, Network_message);
	multi_message_feedback();
}


void
multi_send_message_start()
{
	if (Game_mode&GM_MULTI) {
		multi_sending_message = 1;
		multi_message_index = 0;
		Network_message[multi_message_index] = 0;
	}
}

extern fix StartingShields;
fix PingLaunchTime,PingReturnTime;

extern void network_send_ping (ubyte);
extern int network_who_is_master();
extern char NameReturning;
extern int force_cockpit_redraw;

void network_dump_appletalk_player(ubyte node, ushort net, ubyte socket, int why);

void multi_send_message_end()
{
	char *mytempbuf;
	int i,t;

	Network_message_reciever = 100;

	if (!strnicmp (Network_message,"!Names",6))
	{
		NameReturning=1-NameReturning;
		HUD_init_message ("Name returning is now %s.",NameReturning?"active":"disabled");
	}
	else if (!strnicmp (Network_message,"Handicap:",9))
	{
		mytempbuf=&Network_message[9];
		mprintf ((0,"Networkhandi=%s\n",mytempbuf));
		StartingShields=atol (mytempbuf);
		if (StartingShields<10)
			StartingShields=10;
		if (StartingShields>100)
		{
			sprintf (Network_message,"%s has tried to cheat!",Players[Player_num].callsign);
			StartingShields=100;
		}
		else
			sprintf (Network_message,"%s handicap is now %d",Players[Player_num].callsign,StartingShields);

		HUD_init_message ("Telling others of your handicap of %d!",StartingShields);
		StartingShields=i2f(StartingShields);
	}
	else if (!strnicmp (Network_message,"NoBombs",7))
		Netgame.DoSmartMine=0;
	else if (!strnicmp (Network_message,"Ping:",5))
	{
		if (Game_mode & GM_NETWORK)
		{
			int name_index=5;
			if (strlen(Network_message) > 5)
				while (Network_message[name_index] == ' ')
					name_index++;

			if (strlen(Network_message)<=name_index)
			{
				HUD_init_message ("You must specify a name to ping");
				return;
			}

			for (i = 0; i < N_players; i++)
				if ((!strnicmp(Players[i].callsign, &Network_message[name_index], strlen(Network_message)-name_index)) && (i != Player_num) && (Players[i].connected))
				{
					PingLaunchTime=timer_get_fixed_seconds();
					network_send_ping (i);
					HUD_init_message("Pinging %s...",Players[i].callsign);
					multi_message_index = 0;
					multi_sending_message = 0;
					return;
				}
		}
		else // Modem/Serial ping
		{
			PingLaunchTime=timer_get_fixed_seconds();
			multi_send_modem_ping ();
			HUD_init_message("Pinging opponent...");
		    multi_message_index = 0;
			multi_sending_message = 0;
			return;
		}
	}
	else if (!strnicmp (Network_message,"move:",5))
	{
		mprintf ((0,"moving?\n"));

		if ((Game_mode & GM_NETWORK) && (Game_mode & GM_TEAM))
		{
			int name_index=5;
			if (strlen(Network_message) > 5)
				while (Network_message[name_index] == ' ')
					name_index++;

			if (!network_i_am_master())
			{
				HUD_init_message ("Only %s can move players!",Players[network_who_is_master()].callsign);
				return;
			}

			if (strlen(Network_message)<=name_index)
			{
				HUD_init_message ("You must specify a name to move");
				return;
			}

			for (i = 0; i < N_players; i++)
				if ((!strnicmp(Players[i].callsign, &Network_message[name_index], strlen(Network_message)-name_index)) && (Players[i].connected))
				{
					if ((Game_mode & GM_CAPTURE) && (Players[i].flags & PLAYER_FLAGS_FLAG))
					{
						HUD_init_message ("Can't move player because s/he has a flag!");
						return;
					}

					if (Netgame.team_vector & (1<<i))
						Netgame.team_vector&=(~(1<<i));
					else
						Netgame.team_vector|=(1<<i);

					for (t=0;t<N_players;t++)
						if (Players[t].connected)
							multi_reset_object_texture (&Objects[Players[t].objnum]);

					network_send_netgame_update ();
					sprintf (Network_message,"%s has changed teams!",Players[i].callsign);
					if (i==Player_num)
					{
						HUD_init_message ("You have changed teams!");
						reset_cockpit();
					}
					else
						HUD_init_message ("Moving %s to other team.",Players[i].callsign);
					break;
				}
		}
	}

	else if (!strnicmp (Network_message,"kick:",5) && (Game_mode & GM_NETWORK))
	{
		int name_index=5;
		if (strlen(Network_message) > 5)
			while (Network_message[name_index] == ' ')
				name_index++;

		if (!network_i_am_master())
		{
			HUD_init_message ("Only %s can kick others out!",Players[network_who_is_master()].callsign);
			multi_message_index = 0;
			multi_sending_message = 0;
			return;
		}
		if (strlen(Network_message)<=name_index)
		{
			HUD_init_message ("You must specify a name to kick");
			multi_message_index = 0;
			multi_sending_message = 0;
			return;
		}

		if (Network_message[name_index] == '#' && isdigit(Network_message[name_index+1])) {
			int players[MAX_NUM_NET_PLAYERS];
			int listpos = Network_message[name_index+1] - '0';

			mprintf ((0,"Trying to kick %d , show_kill_list=%d\n",listpos,Show_kill_list));

			if (Show_kill_list==1 || Show_kill_list==2) {
				if (listpos == 0 || listpos >= N_players) {
					HUD_init_message ("Invalid player number for kick.");
					multi_message_index = 0;
					multi_sending_message = 0;
					return;
				}
				multi_get_kill_list(players);
				i = players[listpos];
				if ((i != Player_num) && (Players[i].connected))
					goto kick_player;
			}
			else HUD_init_message ("You cannot use # kicking with in team display.");


		    multi_message_index = 0;
		    multi_sending_message = 0;
			return;
		}


		for (i = 0; i < N_players; i++)
		if ((!strnicmp(Players[i].callsign, &Network_message[name_index], strlen(Network_message)-name_index)) && (i != Player_num) && (Players[i].connected)) {
			kick_player:;
				if (Network_game_type == IPX_GAME)
					network_dump_player(NetPlayers.players[i].network.ipx.server,NetPlayers.players[i].network.ipx.node, 7);
				else
					network_dump_appletalk_player(NetPlayers.players[i].network.appletalk.node,NetPlayers.players[i].network.appletalk.net, NetPlayers.players[i].network.appletalk.socket, 7);

				HUD_init_message("Dumping %s...",Players[i].callsign);
				multi_message_index = 0;
				multi_sending_message = 0;
				return;
			}
	}

	else
		HUD_init_message("%s '%s'", TXT_SENDING, Network_message);

	multi_send_message();
	multi_message_feedback();

	multi_message_index = 0;
	multi_sending_message = 0;
}

void multi_define_macro_end()
{
	Assert( multi_defining_message > 0 );

	strcpy( Network_message_macro[multi_defining_message-1], Network_message );
	write_player_file();

	multi_message_index = 0;
	multi_defining_message = 0;
}

void multi_message_input_sub( int key )
{
	switch( key )   {
	case KEY_F8:
	case KEY_ESC:
		multi_sending_message = 0;
		multi_defining_message = 0;
		game_flush_inputs();
		break;
	case KEY_LEFT:
	case KEY_BACKSP:
	case KEY_PAD4:
		if (multi_message_index > 0)
			multi_message_index--;
		Network_message[multi_message_index] = 0;
		break;
	case KEY_ENTER:
		if ( multi_sending_message )
			multi_send_message_end();
		else if ( multi_defining_message )
			multi_define_macro_end();
		game_flush_inputs();
		break;
	default:
		if ( key > 0 )  {
			int ascii = key_to_ascii(key);
			if ((ascii < 255 ))     {
				if (multi_message_index < MAX_MESSAGE_LEN-2 )   {
					Network_message[multi_message_index++] = ascii;
					Network_message[multi_message_index] = 0;
				} else if ( multi_sending_message )     {
					int i;
					char * ptext, * pcolon;
					ptext = NULL;
					Network_message[multi_message_index++] = ascii;
					Network_message[multi_message_index] = 0;
					for (i=multi_message_index-1; i>=0; i-- )       {
						if ( Network_message[i]==32 )   {
							ptext = &Network_message[i+1];
							Network_message[i] = 0;
							break;
						}
					}
					multi_send_message_end();
					if ( ptext )    {
						multi_sending_message = 1;
						pcolon = strchr( Network_message, ':' );
						if ( pcolon )
							strcpy( pcolon+1, ptext );
						else
							strcpy( Network_message, ptext );
						multi_message_index = strlen( Network_message );
					}
				}
			}
		}
	}
}

void
multi_send_message_dialog(void)
{
	newmenu_item m[1];
	int choice;

	if (!(Game_mode&GM_MULTI))
		return;

	Network_message[0] = 0;             // Get rid of old contents

	m[0].type=NM_TYPE_INPUT; m[0].text = Network_message; m[0].text_len = MAX_MESSAGE_LEN-1;
	choice = newmenu_do( NULL, TXT_SEND_MESSAGE, 1, m, NULL );

	if ((choice > -1) && (strlen(Network_message) > 0)) {
		Network_message_reciever = 100;
		HUD_init_message("%s '%s'", TXT_SENDING, Network_message);
		multi_message_feedback();
	}
}



void
multi_do_death(int objnum)
{
	// Do any miscellaneous stuff for a new network player after death

	objnum = objnum;

	if (!(Game_mode & GM_MULTI_COOP))
	{
		mprintf((0, "Setting all keys for player %d.\n", Player_num));
		Players[Player_num].flags |= (PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_GOLD_KEY);
	}
}

void
multi_do_fire(char *buf)
{
	ubyte weapon;
	char pnum;
	sbyte flags;
	//static dum=0;

	// Act out the actual shooting
	pnum = buf[1];

#ifndef MACINTOSH
	weapon = (int)buf[2];
#else
	weapon = buf[2];
#endif
	flags = buf[4];
	Network_laser_track = GET_INTEL_SHORT(buf + 6);

	Assert (pnum < N_players);

	if (Objects[Players[(int)pnum].objnum].type == OBJ_GHOST)
		multi_make_ghost_player(pnum);

	//  mprintf((0,"multi_do_fire, weapon = %d\n",weapon));

	if (weapon == FLARE_ADJUST)
		Laser_player_fire( Objects+Players[(int)pnum].objnum, FLARE_ID, 6, 1, 0);
	else if (weapon >= MISSILE_ADJUST) {
		int weapon_id,weapon_gun;

		weapon_id = Secondary_weapon_to_weapon_info[weapon-MISSILE_ADJUST];
		weapon_gun = Secondary_weapon_to_gun_num[weapon-MISSILE_ADJUST] + (flags & 1);
		mprintf((0,"missile id = %d, gun = %d\n",weapon_id,weapon_gun));

		if (weapon-MISSILE_ADJUST==GUIDED_INDEX)
		{
			mprintf ((0,"Missile is guided!!!\n"));
			Multi_is_guided=1;
		}

		Laser_player_fire( Objects+Players[(int)pnum].objnum, weapon_id, weapon_gun, 1, 0 );
	}
	else {
		fix save_charge = Fusion_charge;

		if (weapon == FUSION_INDEX) {
			Fusion_charge = flags << 12;
			mprintf((0, "Fusion charge X%f.\n", f2fl(Fusion_charge)));
		}
		if (weapon == LASER_ID) {
			if (flags & LASER_QUAD)
				Players[(int)pnum].flags |= PLAYER_FLAGS_QUAD_LASERS;
			else
				Players[(int)pnum].flags &= ~PLAYER_FLAGS_QUAD_LASERS;
		}

		do_laser_firing(Players[(int)pnum].objnum, weapon, (int)buf[3], flags, (int)buf[5]);

		if (weapon == FUSION_INDEX)
			Fusion_charge = save_charge;
	}
}

void
multi_do_message(char *buf)
{
	char *colon;
	char *tilde,mesbuf[100];
	int tloc,t;

	int loc = 2;

	if ((tilde=strchr (buf+loc,'$')))  // do that stupid name stuff
	{											// why'd I put this in?  Probably for the
		tloc=tilde-(buf+loc);				// same reason you can name your guidebot
		mprintf ((0,"Tloc=%d\n",tloc));

		if (tloc>0)
			strncpy (mesbuf,buf+loc,tloc);
		strcpy (mesbuf+tloc,Players[Player_num].callsign);
		strcpy (mesbuf+strlen(Players[Player_num].callsign)+tloc,buf+loc+tloc+1);
		strcpy (buf+loc,mesbuf);
	}

	if (((colon = strrchr(buf+loc, ':')) == NULL) || (colon-(buf+loc) < 1) || (colon-(buf+loc) > CALLSIGN_LEN))
	{
		mesbuf[0] = 1;
		mesbuf[1] = BM_XRGB(28, 0, 0);
		strcpy(&mesbuf[2], Players[(int)buf[1]].callsign);
		t = strlen(mesbuf);
		mesbuf[t] = ':';
		mesbuf[t+1] = 1;
		mesbuf[t+2] = BM_XRGB(0, 31, 0);
		mesbuf[t+3] = 0;

		digi_play_sample(SOUND_HUD_MESSAGE, F1_0);
		HUD_init_message("%s %s", mesbuf, buf+2);
	}
	else
	{
		if ( (!strnicmp(Players[Player_num].callsign, buf+loc, colon-(buf+loc))) ||
			 ((Game_mode & GM_TEAM) && ( (get_team(Player_num) == atoi(buf+loc)-1) || !strnicmp(Netgame.team_name[get_team(Player_num)], buf+loc, colon-(buf+loc)))) )
		{
			mesbuf[0] = 1;
			mesbuf[1] = BM_XRGB(0, 32, 32);
			strcpy(&mesbuf[2], Players[(int)buf[1]].callsign);
			t = strlen(mesbuf);
			mesbuf[t] = ':';
			mesbuf[t+1] = 1;
			mesbuf[t+2] = BM_XRGB(0, 31, 0);
			mesbuf[t+3] = 0;

			digi_play_sample(SOUND_HUD_MESSAGE, F1_0);
			HUD_init_message("%s %s", mesbuf, colon+1);
		}
	}
}

void
multi_do_position(char *buf)
{
#ifdef WORDS_BIGENDIAN
	shortpos sp;
#endif

	// This routine does only player positions, mode game only
	//      mprintf((0, "Got position packet.\n"));

	int pnum = (Player_num+1)%2;

	Assert(&Objects[Players[pnum].objnum] != ConsoleObject);

	if (Game_mode & GM_NETWORK)
	{
		Int3(); // Get Jason, what the hell are we doing here?
		return;
    }


#ifndef WORDS_BIGENDIAN
	extract_shortpos(&Objects[Players[pnum].objnum], (shortpos *)(buf+1),0);
#else
	memcpy((ubyte *)(sp.bytemat), (ubyte *)(buf + 1), 9);
	memcpy((ubyte *)&(sp.xo), (ubyte *)(buf + 10), 14);
	extract_shortpos(&Objects[Players[pnum].objnum], &sp, 1);
#endif

	if (Objects[Players[pnum].objnum].movement_type == MT_PHYSICS)
		set_thrust_from_velocity(&Objects[Players[pnum].objnum]);
}

void
multi_do_reappear(char *buf)
{
	short objnum;

	objnum = GET_INTEL_SHORT(buf + 1);

	Assert(objnum >= 0);
	//      Assert(Players[Objects[objnum].id]].objnum == objnum);

	// mprintf((0, "Switching rendering back on for object %d.\n", objnum));

	multi_make_ghost_player(Objects[objnum].id);
	create_player_appearance_effect(&Objects[objnum]);
	PKilledFlags[Objects[objnum].id]=0;
}

void
multi_do_player_explode(char *buf)
{
	// Only call this for players, not robots.  pnum is player number, not
	// Object number.

	object *objp;
	int count;
	int pnum;
	int i;
	char remote_created;

	pnum = buf[1];

#ifdef NDEBUG
	if ((pnum < 0) || (pnum >= N_players))
		return;
#else
	Assert(pnum >= 0);
	Assert(pnum < N_players);
#endif

#ifdef NETWORK
	// If we are in the process of sending objects to a new player, reset that process
	if (Network_send_objects)
	{
		mprintf((0, "Resetting object sync due to player explosion.\n"));
		Network_send_objnum = -1;
	}
#endif

	// Stuff the Players structure to prepare for the explosion

	count = 2;
	Players[pnum].primary_weapon_flags = GET_INTEL_SHORT(buf + count); count += 2;
	Players[pnum].secondary_weapon_flags = GET_INTEL_SHORT(buf + count); count += 2;
	Players[pnum].laser_level = buf[count];                                                 count++;
	Players[pnum].secondary_ammo[HOMING_INDEX] = buf[count];                count++;
	Players[pnum].secondary_ammo[CONCUSSION_INDEX] = buf[count];count++;
	Players[pnum].secondary_ammo[SMART_INDEX] = buf[count];         count++;
	Players[pnum].secondary_ammo[MEGA_INDEX] = buf[count];          count++;
	Players[pnum].secondary_ammo[PROXIMITY_INDEX] = buf[count]; count++;

	Players[pnum].secondary_ammo[SMISSILE1_INDEX] = buf[count]; count++;
	Players[pnum].secondary_ammo[GUIDED_INDEX]    = buf[count]; count++;
	Players[pnum].secondary_ammo[SMART_MINE_INDEX]= buf[count]; count++;
	Players[pnum].secondary_ammo[SMISSILE4_INDEX] = buf[count]; count++;
	Players[pnum].secondary_ammo[SMISSILE5_INDEX] = buf[count]; count++;

	Players[pnum].primary_ammo[VULCAN_INDEX] = GET_INTEL_SHORT(buf + count); count += 2;
	Players[pnum].primary_ammo[GAUSS_INDEX] = GET_INTEL_SHORT(buf + count); count += 2;
	Players[pnum].flags = GET_INTEL_INT(buf + count);               count += 4;

	multi_adjust_remote_cap (pnum);

	objp = Objects+Players[pnum].objnum;

	//      objp->phys_info.velocity = *(vms_vector *)(buf+16); // 12 bytes
	//      objp->pos = *(vms_vector *)(buf+28);                // 12 bytes

	remote_created = buf[count++]; // How many did the other guy create?

	Net_create_loc = 0;

	drop_player_eggs(objp);

	// Create mapping from remote to local numbering system

	mprintf((0, "I Created %d powerups, remote created %d.\n", Net_create_loc, remote_created));

	// We now handle this situation gracefully, Int3 not required
	//      if (Net_create_loc != remote_created)
	//              Int3(); // Probably out of object array space, see Rob

	for (i = 0; i < remote_created; i++)
	{
		short s;

		s = GET_INTEL_SHORT(buf + count);

		if ((i < Net_create_loc) && (s > 0))
			map_objnum_local_to_remote((short)Net_create_objnums[i], s, pnum);
		else if (s <= 0)
		{
			mprintf((0, "WARNING: Remote created object has non-valid number %d (player %d)", s, pnum));
		}
		else
		{
			mprintf((0, "WARNING: Could not create all powerups created by player %d.\n", pnum));
		}
		//              Assert(s > 0);
		count += 2;
	}
	for (i = remote_created; i < Net_create_loc; i++) {
		mprintf((0, "WARNING: I Created more powerups than player %d, deleting.\n", pnum));
		Objects[Net_create_objnums[i]].flags |= OF_SHOULD_BE_DEAD;
	}

	if (buf[0] == MULTI_PLAYER_EXPLODE)
	{
		explode_badass_player(objp);

		objp->flags &= ~OF_SHOULD_BE_DEAD;              //don't really kill player
		multi_make_player_ghost(pnum);
	}
	else
	{
		create_player_appearance_effect(objp);
	}

	Players[pnum].flags &= ~(PLAYER_FLAGS_CLOAKED | PLAYER_FLAGS_INVULNERABLE | PLAYER_FLAGS_FLAG);
	Players[pnum].cloak_time = 0;
}

void
multi_do_kill(char *buf)
{
	int killer, killed;
	int count = 1;
	int pnum;

	pnum = (int)(buf[count]);
	if ((pnum < 0) || (pnum >= N_players))
	{
		Int3(); // Invalid player number killed
		return;
	}
	killed = Players[pnum].objnum;
	count += 1;

	killer = GET_INTEL_SHORT(buf + count);
	if (killer > 0)
		killer = objnum_remote_to_local(killer, (sbyte)buf[count+2]);

#ifdef SHAREWARE
	if ((Objects[killed].type != OBJ_PLAYER) && (Objects[killed].type != OBJ_GHOST))
	{
		Int3();
		mprintf( (1, "SOFT INT3: MULTI.C Non-player object %d of type %d killed! (JOHN)\n", killed, Objects[killed].type ));
		return;
	}
#endif

	multi_compute_kill(killer, killed);

}


//      Changed by MK on 10/20/94 to send NULL as object to net_destroy_controlcen if it got -1
// which means not a controlcen object, but contained in another object
void multi_do_controlcen_destroy(char *buf)
{
	sbyte who;
	short objnum;

	objnum = GET_INTEL_SHORT(buf + 1);
	who = buf[3];

	if (Control_center_destroyed != 1)
	{
		if ((who < N_players) && (who != Player_num)) {
			HUD_init_message("%s %s", Players[who].callsign, TXT_HAS_DEST_CONTROL);
		}
		else if (who == Player_num)
			HUD_init_message(TXT_YOU_DEST_CONTROL);
		else
			HUD_init_message(TXT_CONTROL_DESTROYED);

		if (objnum != -1)
			net_destroy_controlcen(Objects+objnum);
		else
			net_destroy_controlcen(NULL);
	}
}

void
multi_do_escape(char *buf)
{
	int objnum;

	objnum = Players[(int)buf[1]].objnum;

	digi_play_sample(SOUND_HUD_MESSAGE, F1_0);
	digi_kill_sound_linked_to_object (objnum);

	if (buf[2] == 0)
	{
		HUD_init_message("%s %s", Players[(int)buf[1]].callsign, TXT_HAS_ESCAPED);
		if (Game_mode & GM_NETWORK)
			Players[(int)buf[1]].connected = CONNECT_ESCAPE_TUNNEL;
		if (!multi_goto_secret)
			multi_goto_secret = 2;
	}
	else if (buf[2] == 1)
	{
		HUD_init_message("%s %s", Players[(int)buf[1]].callsign, TXT_HAS_FOUND_SECRET);
		if (Game_mode & GM_NETWORK)
			Players[(int)buf[1]].connected = CONNECT_FOUND_SECRET;
		if (!multi_goto_secret)
			multi_goto_secret = 1;
	}
	create_player_appearance_effect(&Objects[objnum]);
	multi_make_player_ghost(buf[1]);
}

void
multi_do_remobj(char *buf)
{
	short objnum; // which object to remove
	short local_objnum;
	sbyte obj_owner; // which remote list is it entered in

	objnum = GET_INTEL_SHORT(buf + 1);
	obj_owner = buf[3];

	Assert(objnum >= 0);

	if (objnum < 1)
		return;

	local_objnum = objnum_remote_to_local(objnum, obj_owner); // translate to local objnum

	//      mprintf((0, "multi_do_remobj: %d owner %d = %d.\n", objnum, obj_owner, local_objnum));

	if (local_objnum < 0)
	{
		mprintf((1, "multi_do_remobj: Could not remove referenced object.\n"));
		return;
	}

	if ((Objects[local_objnum].type != OBJ_POWERUP) && (Objects[local_objnum].type != OBJ_HOSTAGE))
	{
		mprintf((1, "multi_get_remobj: tried to remove invalid type %d.\n", Objects[local_objnum].type));
		return;
	}

	if (Network_send_objects && network_objnum_is_past(local_objnum))
	{
		mprintf((0, "Resetting object sync due to object removal.\n"));
		Network_send_objnum = -1;
	}
	if (Objects[local_objnum].type==OBJ_POWERUP)
		if (Game_mode & GM_NETWORK)
		{
			if (PowerupsInMine[Objects[local_objnum].id]>0)
				PowerupsInMine[Objects[local_objnum].id]--;

			if (multi_powerup_is_4pack (Objects[local_objnum].id))
			{
				mprintf ((0,"Hey babe! Doing that wacky 4 pack stuff."));

				if (PowerupsInMine[Objects[local_objnum].id-1]-4<0)
					PowerupsInMine[Objects[local_objnum].id-1]=0;
				else
					PowerupsInMine[Objects[local_objnum].id-1]-=4;
			}

			mprintf ((0,"Decrementing powerups! %d\n",PowerupsInMine[Objects[local_objnum].id]));
		}

	Objects[local_objnum].flags |= OF_SHOULD_BE_DEAD; // quick and painless

}

void
multi_do_quit(char *buf)
{

	if (Game_mode & GM_NETWORK)
	{
		int i, n = 0;

		digi_play_sample( SOUND_HUD_MESSAGE, F1_0 );

		HUD_init_message( "%s %s", Players[(int)buf[1]].callsign, TXT_HAS_LEFT_THE_GAME);

		network_disconnect_player(buf[1]);

		if (multi_in_menu)
			return;

		for (i = 0; i < N_players; i++)
			if (Players[i].connected) n++;
		if (n == 1)
		{
			nm_messagebox(NULL, 1, TXT_OK, TXT_YOU_ARE_ONLY);
		}
	}

	if ((Game_mode & GM_SERIAL) || (Game_mode & GM_MODEM))
	{
		Function_mode = FMODE_MENU;
		multi_quit_game = 1;
		multi_leave_menu = 1;
		nm_messagebox(NULL, 1, TXT_OK, TXT_OPPONENT_LEFT);
		Function_mode = FMODE_GAME;
		multi_reset_stuff();
	}
	return;
}

void
multi_do_cloak(char *buf)
{
	int pnum;

	pnum = (int)(buf[1]);

	Assert(pnum < N_players);

	mprintf((0, "Cloaking player %d\n", pnum));

	Players[pnum].flags |= PLAYER_FLAGS_CLOAKED;
	Players[pnum].cloak_time = GameTime;
	ai_do_cloak_stuff();

#ifndef SHAREWARE
	if (Game_mode & GM_MULTI_ROBOTS)
		multi_strip_robots(pnum);
#endif

	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_multi_cloak(pnum);
}

void
multi_do_decloak(char *buf)
{
	int pnum;

	pnum = (int)(buf[1]);

	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_multi_decloak(pnum);

}

void
multi_do_door_open(char *buf)
{
	int segnum;
	sbyte side;
	segment *seg;
	wall *w;
	ubyte flag;

	segnum = GET_INTEL_SHORT(buf + 1);
	side = buf[3];
	flag= buf[4];

	//      mprintf((0, "Opening door on side %d of segment # %d.\n", side, segnum));

	if ((segnum < 0) || (segnum > Highest_segment_index) || (side < 0) || (side > 5))
	{
		Int3();
		return;
	}

	seg = &Segments[segnum];

	if (seg->sides[side].wall_num == -1) {  //Opening door on illegal wall
		Int3();
		return;
	}

	w = &Walls[seg->sides[side].wall_num];

	if (w->type == WALL_BLASTABLE)
	{
		if (!(w->flags & WALL_BLASTED))
		{
			mprintf((0, "Blasting wall by remote command.\n"));
			wall_destroy(seg, side);
		}
		return;
	}
	else if (w->state != WALL_DOOR_OPENING)
	{
		wall_open_door(seg, side);
		w->flags=flag;
	}
	else
		w->flags=flag;

	//      else
	//              mprintf((0, "Door already opening!\n"));
}

void
multi_do_create_explosion(char *buf)
{
	int pnum;
	int count = 1;

	pnum = buf[count++];

	//      mprintf((0, "Creating small fireball.\n"));
	create_small_fireball_on_object(&Objects[Players[pnum].objnum], F1_0, 1);
}

void
multi_do_controlcen_fire(char *buf)
{
	vms_vector to_target;
	char gun_num;
	short objnum;
	int count = 1;

	memcpy(&to_target, buf+count, 12);          count += 12;
#ifdef WORDS_BIGENDIAN  // swap the vector to_target
	to_target.x = (fix)INTEL_INT((int)to_target.x);
	to_target.y = (fix)INTEL_INT((int)to_target.y);
	to_target.z = (fix)INTEL_INT((int)to_target.z);
#endif
	gun_num = buf[count];                       count += 1;
	objnum = GET_INTEL_SHORT(buf + count);      count += 2;

	Laser_create_new_easy(&to_target, &Gun_pos[(int)gun_num], objnum, CONTROLCEN_WEAPON_NUM, 1);
}

void
multi_do_create_powerup(char *buf)
{
	short segnum;
	short objnum;
	int my_objnum;
	char pnum;
	int count = 1;
	vms_vector new_pos;
	char powerup_type;

	if (Endlevel_sequence || Control_center_destroyed)
		return;

	pnum = buf[count++];
	powerup_type = buf[count++];
	segnum = GET_INTEL_SHORT(buf + count); count += 2;
	objnum = GET_INTEL_SHORT(buf + count); count += 2;

	if ((segnum < 0) || (segnum > Highest_segment_index)) {
		Int3();
		return;
	}

	memcpy(&new_pos, buf+count, sizeof(vms_vector)); count+=sizeof(vms_vector);
#ifdef WORDS_BIGENDIAN
	new_pos.x = (fix)SWAPINT((int)new_pos.x);
	new_pos.y = (fix)SWAPINT((int)new_pos.y);
	new_pos.z = (fix)SWAPINT((int)new_pos.z);
#endif

	Net_create_loc = 0;
	my_objnum = call_object_create_egg(&Objects[Players[(int)pnum].objnum], 1, OBJ_POWERUP, powerup_type);

	if (my_objnum < 0) {
		mprintf((0, "Could not create new powerup!\n"));
		return;
	}

	if (Network_send_objects && network_objnum_is_past(my_objnum))
	{
		mprintf((0, "Resetting object sync due to powerup creation.\n"));
		Network_send_objnum = -1;
	}

	Objects[my_objnum].pos = new_pos;

	vm_vec_zero(&Objects[my_objnum].mtype.phys_info.velocity);

	obj_relink(my_objnum, segnum);

	map_objnum_local_to_remote(my_objnum, objnum, pnum);

	object_create_explosion(segnum, &new_pos, i2f(5), VCLIP_POWERUP_DISAPPEARANCE);
	mprintf((0, "Creating powerup type %d in segment %i.\n", powerup_type, segnum));

	if (Game_mode & GM_NETWORK)
		PowerupsInMine[(int)powerup_type]++;
}

void
multi_do_play_sound(char *buf)
{
	int pnum = (int)(buf[1]);
	int sound_num = (int)(buf[2]);
	fix volume = (int)(buf[3]) << 12;

	if (!Players[pnum].connected)
		return;

	Assert(Players[pnum].objnum >= 0);
	Assert(Players[pnum].objnum <= Highest_object_index);

	digi_link_sound_to_object( sound_num, Players[pnum].objnum, 0, volume);
}

void
multi_do_score(char *buf)
{
	int pnum = (int)(buf[1]);

	if ((pnum < 0) || (pnum >= N_players))
	{
		Int3(); // Non-terminal, see rob
		return;
	}

	if (Newdemo_state == ND_STATE_RECORDING) {
		int score;
		score = GET_INTEL_INT(buf + 2);
		newdemo_record_multi_score(pnum, score);
	}

	Players[pnum].score = GET_INTEL_INT(buf + 2);

	multi_sort_kill_list();
}

void
multi_do_trigger(char *buf)
{
	int pnum = (int)(buf[1]);
	int trigger = (int)(buf[2]);

	mprintf ((0,"MULTI doing trigger!\n"));

	if ((pnum < 0) || (pnum >= N_players) || (pnum == Player_num))
	{
		Int3(); // Got trigger from illegal playernum
		return;
	}
	if ((trigger < 0) || (trigger >= Num_triggers))
	{
		Int3(); // Illegal trigger number in multiplayer
		return;
	}
	check_trigger_sub(trigger, pnum,0);
}

void multi_do_drop_marker (char *buf)
{
	int i;
	int pnum=(int)(buf[1]);
	int mesnum=(int)(buf[2]);
	vms_vector position;

	if (pnum==Player_num)  // my marker? don't set it down cuz it might screw up the orientation
		return;

	position.x = GET_INTEL_INT(buf + 3);
	position.y = GET_INTEL_INT(buf + 7);
	position.z = GET_INTEL_INT(buf + 11);

	for (i=0;i<40;i++)
		MarkerMessage[(pnum*2)+mesnum][i]=buf[15+i];

	MarkerPoint[(pnum*2)+mesnum]=position;

	if (MarkerObject[(pnum*2)+mesnum] !=-1 && Objects[MarkerObject[(pnum*2)+mesnum]].type!=OBJ_NONE && MarkerObject[(pnum*2)+mesnum] !=0)
		obj_delete(MarkerObject[(pnum*2)+mesnum]);

	MarkerObject[(pnum*2)+mesnum] = drop_marker_object(&position,Objects[Players[Player_num].objnum].segnum,&Objects[Players[Player_num].objnum].orient,(pnum*2)+mesnum);
	strcpy (MarkerOwner[(pnum*2)+mesnum],Players[pnum].callsign);
	mprintf ((0,"Dropped player %d message: %s\n",pnum,MarkerMessage[(pnum*2)+mesnum]));
}


void multi_do_hostage_door_status(char *buf)
{
	// Update hit point status of a door

	int count = 1;
	int wallnum;
	fix hps;

	wallnum = GET_INTEL_SHORT(buf + count);     count += 2;
	hps = GET_INTEL_INT(buf + count);           count += 4;

	if ((wallnum < 0) || (wallnum > Num_walls) || (hps < 0) || (Walls[wallnum].type != WALL_BLASTABLE))
	{
		Int3(); // Non-terminal, see Rob
		return;
	}

	//      mprintf((0, "Damaging wall number %d to %f points.\n", wallnum, f2fl(hps)));

	if (hps < Walls[wallnum].hps)
		wall_damage(&Segments[Walls[wallnum].segnum], Walls[wallnum].sidenum, Walls[wallnum].hps - hps);
}

void multi_do_save_game(char *buf)
{
	int count = 1;
	ubyte slot;
	uint id;
	char desc[25];

	slot = *(ubyte *)(buf+count);           count += 1;
	id = GET_INTEL_INT(buf + count);        count += 4;
	memcpy( desc, &buf[count], 20 );        count += 20;

	multi_save_game( slot, id, desc );
}

void multi_do_restore_game(char *buf)
{
	int count = 1;
	ubyte slot;
	uint id;

	slot = *(ubyte *)(buf+count);           count += 1;
	id = GET_INTEL_INT(buf + count);        count += 4;

	multi_restore_game( slot, id );
}


void multi_do_req_player(char *buf)
{
	netplayer_stats ps;
	ubyte player_n;
	// Send my netplayer_stats to everyone!
	player_n = *(ubyte *)(buf+1);
	if ( (player_n == Player_num) || (player_n == 255)  )   {
		extract_netplayer_stats( &ps, &Players[Player_num] );
		ps.Player_num = Player_num;
		ps.message_type = MULTI_SEND_PLAYER;            // SET
		multi_send_data((char*)&ps, sizeof(netplayer_stats), 0);
	}
}

void multi_do_send_player(char *buf)
{
	// Got a player packet from someone!!!
	netplayer_stats * p;
	p = (netplayer_stats *)buf;

	Assert( p->Player_num <= N_players );

	mprintf(( 0, "Got netplayer_stats for player %d (I'm %d)\n", p->Player_num, Player_num ));
	mprintf(( 0, "Their shields are: %d\n", f2i(p->shields) ));

	use_netplayer_stats( &Players[p->Player_num], p );
}

void
multi_reset_stuff(void)
{
	// A generic, emergency function to solve problems that crop up
	// when a player exits quick-out from the game because of a
	// serial connection loss.  Fixes several weird bugs!

	dead_player_end();

	Players[Player_num].homing_object_dist = -F1_0; // Turn off homing sound.

	Dead_player_camera = 0;
	Endlevel_sequence = 0;
	reset_rear_view();
}

void
multi_reset_player_object(object *objp)
{
	int i;

	//Init physics for a non-console player

	Assert(objp >= Objects);
	Assert(objp <= Objects+Highest_object_index);
	Assert((objp->type == OBJ_PLAYER) || (objp->type == OBJ_GHOST));

	vm_vec_zero(&objp->mtype.phys_info.velocity);
	vm_vec_zero(&objp->mtype.phys_info.thrust);
	vm_vec_zero(&objp->mtype.phys_info.rotvel);
	vm_vec_zero(&objp->mtype.phys_info.rotthrust);
	objp->mtype.phys_info.brakes = objp->mtype.phys_info.turnroll = 0;
	objp->mtype.phys_info.mass = Player_ship->mass;
	objp->mtype.phys_info.drag = Player_ship->drag;
	//      objp->mtype.phys_info.flags &= ~(PF_TURNROLL | PF_LEVELLING | PF_WIGGLE | PF_USES_THRUST);
	objp->mtype.phys_info.flags &= ~(PF_TURNROLL | PF_LEVELLING | PF_WIGGLE);

	//Init render info

	objp->render_type = RT_POLYOBJ;
	objp->rtype.pobj_info.model_num = Player_ship->model_num;               //what model is this?
	objp->rtype.pobj_info.subobj_flags = 0;         //zero the flags
	for (i=0;i<MAX_SUBMODELS;i++)
		vm_angvec_zero(&objp->rtype.pobj_info.anim_angles[i]);

	//reset textures for this, if not player 0

	multi_reset_object_texture (objp);

	// Clear misc

	objp->flags = 0;

	if (objp->type == OBJ_GHOST)
		objp->render_type = RT_NONE;

}

void multi_reset_object_texture (object *objp)
{
	int id,i;

	if (Game_mode & GM_TEAM)
		id = get_team(objp->id);
	else
		id = objp->id;

	if (id == 0)
		objp->rtype.pobj_info.alt_textures=0;
	else {
		Assert(N_PLAYER_SHIP_TEXTURES == Polygon_models[objp->rtype.pobj_info.model_num].n_textures);

		for (i=0;i<N_PLAYER_SHIP_TEXTURES;i++)
			multi_player_textures[id-1][i] = ObjBitmaps[ObjBitmapPtrs[Polygon_models[objp->rtype.pobj_info.model_num].first_texture+i]];

		multi_player_textures[id-1][4] = ObjBitmaps[ObjBitmapPtrs[First_multi_bitmap_num+(id-1)*2]];
		multi_player_textures[id-1][5] = ObjBitmaps[ObjBitmapPtrs[First_multi_bitmap_num+(id-1)*2+1]];

		objp->rtype.pobj_info.alt_textures = id;
	}
}




#ifdef NETPROFILING
extern int TTRecv[];
extern FILE *RecieveLogFile;
#endif

void
multi_process_bigdata(char *buf, int len)
{
	// Takes a bunch of messages, check them for validity,
	// and pass them to multi_process_data.

	int type, sub_len, bytes_processed = 0;

	while( bytes_processed < len )  {
		type = buf[bytes_processed];

		if ( (type<0) || (type>MULTI_MAX_TYPE)) {
			mprintf( (1, "multi_process_bigdata: Invalid packet type %d!\n", type ));
			return;
		}
		sub_len = message_length[type];

		Assert(sub_len > 0);

		if ( (bytes_processed+sub_len) > len )  {
			mprintf( (1, "multi_process_bigdata: packet type %d too short (%d>%d)!\n", type, (bytes_processed+sub_len), len ));
			Int3();
			return;
		}

		multi_process_data(&buf[bytes_processed], sub_len);
		bytes_processed += sub_len;
	}
}

//
// Part 2 : Functions that send communication messages to inform the other
//          players of something we did.
//

void
multi_send_fire(void)
{
	if (!Network_laser_fired)
		return;

	multibuf[0] = (char)MULTI_FIRE;
	multibuf[1] = (char)Player_num;
	multibuf[2] = (char)Network_laser_gun;
	multibuf[3] = (char)Network_laser_level;
	multibuf[4] = (char)Network_laser_flags;
	multibuf[5] = (char)Network_laser_fired;

	PUT_INTEL_SHORT(multibuf+6, Network_laser_track);

	multi_send_data(multibuf, 8, 0);

	Network_laser_fired = 0;
}

void
multi_send_destroy_controlcen(int objnum, int player)
{
	if (player == Player_num)
		HUD_init_message(TXT_YOU_DEST_CONTROL);
	else if ((player > 0) && (player < N_players))
		HUD_init_message("%s %s", Players[player].callsign, TXT_HAS_DEST_CONTROL);
	else
		HUD_init_message(TXT_CONTROL_DESTROYED);

	multibuf[0] = (char)MULTI_CONTROLCEN;
	PUT_INTEL_SHORT(multibuf+1, objnum);
	multibuf[3] = player;
	multi_send_data(multibuf, 4, 2);
}

void multi_send_drop_marker (int player,vms_vector position,char messagenum,char text[])
{
	int i;

	if (player<N_players)
	{
		mprintf ((0,"Sending MARKER drop!\n"));
		multibuf[0]=(char)MULTI_MARKER;
		multibuf[1]=(char)player;
		multibuf[2]=messagenum;
		PUT_INTEL_INT(multibuf+3, position.x);
		PUT_INTEL_INT(multibuf+7, position.y);
		PUT_INTEL_INT(multibuf+11, position.z);
		for (i=0;i<40;i++)
			multibuf[15+i]=text[i];
	}
	multi_send_data(multibuf, 55, 1);
}

void
multi_send_endlevel_start(int secret)
{
	multibuf[0] = (char)MULTI_ENDLEVEL_START;
	multibuf[1] = Player_num;
	multibuf[2] = (char)secret;

	if ((secret) && !multi_goto_secret)
		multi_goto_secret = 1;
	else if (!multi_goto_secret)
		multi_goto_secret = 2;

	multi_send_data(multibuf, 3, 1);
	if (Game_mode & GM_NETWORK)
	{
		Players[Player_num].connected = 5;
		network_send_endlevel_packet();
	}
}

void
multi_send_player_explode(char type)
{
	int count = 0;
	int i;

	Assert( (type == MULTI_PLAYER_DROP) || (type == MULTI_PLAYER_EXPLODE) );

	multi_send_position(Players[Player_num].objnum);

	if (Network_send_objects)
	{
		mprintf((0, "Resetting object sync due to player explosion.\n"));
		Network_send_objnum = -1;
	}

	multibuf[count++] = type;
	multibuf[count++] = Player_num;

	PUT_INTEL_SHORT(multibuf+count, Players[Player_num].primary_weapon_flags);
	count += 2;
	PUT_INTEL_SHORT(multibuf+count, Players[Player_num].secondary_weapon_flags);
	count += 2;
	multibuf[count++] = (char)Players[Player_num].laser_level;

	multibuf[count++] = (char)Players[Player_num].secondary_ammo[HOMING_INDEX];
	multibuf[count++] = (char)Players[Player_num].secondary_ammo[CONCUSSION_INDEX];
	multibuf[count++] = (char)Players[Player_num].secondary_ammo[SMART_INDEX];
	multibuf[count++] = (char)Players[Player_num].secondary_ammo[MEGA_INDEX];
	multibuf[count++] = (char)Players[Player_num].secondary_ammo[PROXIMITY_INDEX];

	multibuf[count++] = (char)Players[Player_num].secondary_ammo[SMISSILE1_INDEX];
	multibuf[count++] = (char)Players[Player_num].secondary_ammo[GUIDED_INDEX];
	multibuf[count++] = (char)Players[Player_num].secondary_ammo[SMART_MINE_INDEX];
	multibuf[count++] = (char)Players[Player_num].secondary_ammo[SMISSILE4_INDEX];
	multibuf[count++] = (char)Players[Player_num].secondary_ammo[SMISSILE5_INDEX];

	PUT_INTEL_SHORT(multibuf+count, Players[Player_num].primary_ammo[VULCAN_INDEX] );
	count += 2;
	PUT_INTEL_SHORT(multibuf+count, Players[Player_num].primary_ammo[GAUSS_INDEX] );
	count += 2;
	PUT_INTEL_INT(multibuf+count, Players[Player_num].flags );
	count += 4;

	multibuf[count++] = Net_create_loc;

	Assert(Net_create_loc <= MAX_NET_CREATE_OBJECTS);

	memset(multibuf+count, -1, MAX_NET_CREATE_OBJECTS*sizeof(short));

	mprintf((0, "Created %d explosion objects.\n", Net_create_loc));

	for (i = 0; i < Net_create_loc; i++)
	{
		if (Net_create_objnums[i] <= 0) {
			Int3(); // Illegal value in created egg object numbers
			count +=2;
			continue;
		}

		PUT_INTEL_SHORT(multibuf+count, Net_create_objnums[i]); count += 2;

		// We created these objs so our local number = the network number
		map_objnum_local_to_local((short)Net_create_objnums[i]);
	}

	Net_create_loc = 0;

	//      mprintf((1, "explode message size = %d, max = %d.\n", count, message_length[MULTI_PLAYER_EXPLODE]));

	if (count > message_length[MULTI_PLAYER_EXPLODE])
	{
		Int3(); // See Rob
	}

	multi_send_data(multibuf, message_length[MULTI_PLAYER_EXPLODE], 2);
	if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED)
		multi_send_decloak();
	if (Game_mode & GM_MULTI_ROBOTS)
		multi_strip_robots(Player_num);
}

extern ubyte Secondary_weapon_to_powerup[];
extern ubyte Primary_weapon_to_powerup[];

// put a lid on how many objects will be spewed by an exploding player
// to prevent rampant powerups in netgames

void multi_cap_objects ()
{
	char type,flagtype;
	int index;

	if (!(Game_mode & GM_NETWORK))
		return;

	for (index=0;index<MAX_PRIMARY_WEAPONS;index++)
	{
		type=Primary_weapon_to_powerup[index];
		if (PowerupsInMine[(int)type]>=MaxPowerupsAllowed[(int)type])
			if(Players[Player_num].primary_weapon_flags & (1 << index))
			{
				mprintf ((0,"PIM=%d MPA=%d\n",PowerupsInMine[(int)type],MaxPowerupsAllowed[(int)type]));
				mprintf ((0,"Killing a primary cuz there's too many! (%d)\n",(int)type));
				Players[Player_num].primary_weapon_flags&=(~(1 << index));
			}
	}


	// Don't do the adjustment stuff for Hoard mode
	if (!(Game_mode & GM_HOARD))
		Players[Player_num].secondary_ammo[2]/=4;

	Players[Player_num].secondary_ammo[7]/=4;

	for (index=0;index<MAX_SECONDARY_WEAPONS;index++)
	{
		if ((Game_mode & GM_HOARD) && index==PROXIMITY_INDEX)
			continue;

		type=Secondary_weapon_to_powerup[index];

		if ((Players[Player_num].secondary_ammo[index]+PowerupsInMine[(int)type])>MaxPowerupsAllowed[(int)type])
		{
			if (MaxPowerupsAllowed[(int)type]-PowerupsInMine[(int)type]<0)
				Players[Player_num].secondary_ammo[index]=0;
			else
				Players[Player_num].secondary_ammo[index]=(MaxPowerupsAllowed[(int)type]-PowerupsInMine[(int)type]);

			mprintf ((0,"Hey! I killed secondary type %d because PIM=%d MPA=%d\n",(int)type,PowerupsInMine[(int)type],MaxPowerupsAllowed[(int)type]));
		}
	}

	if (!(Game_mode & GM_HOARD))
		Players[Player_num].secondary_ammo[2]*=4;
	Players[Player_num].secondary_ammo[7]*=4;

	if (Players[Player_num].laser_level > MAX_LASER_LEVEL)
		if (PowerupsInMine[POW_SUPER_LASER]+1 > MaxPowerupsAllowed[POW_SUPER_LASER])
			Players[Player_num].laser_level=0;

	if (Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS)
		if (PowerupsInMine[POW_QUAD_FIRE]+1 > MaxPowerupsAllowed[POW_QUAD_FIRE])
			Players[Player_num].flags&=(~PLAYER_FLAGS_QUAD_LASERS);

	if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED)
		if (PowerupsInMine[POW_CLOAK]+1 > MaxPowerupsAllowed[POW_CLOAK])
			Players[Player_num].flags&=(~PLAYER_FLAGS_CLOAKED);

	if (Players[Player_num].flags & PLAYER_FLAGS_MAP_ALL)
		if (PowerupsInMine[POW_FULL_MAP]+1 > MaxPowerupsAllowed[POW_FULL_MAP])
			Players[Player_num].flags&=(~PLAYER_FLAGS_MAP_ALL);

	if (Players[Player_num].flags & PLAYER_FLAGS_AFTERBURNER)
		if (PowerupsInMine[POW_AFTERBURNER]+1 > MaxPowerupsAllowed[POW_AFTERBURNER])
			Players[Player_num].flags&=(~PLAYER_FLAGS_AFTERBURNER);

	if (Players[Player_num].flags & PLAYER_FLAGS_AMMO_RACK)
		if (PowerupsInMine[POW_AMMO_RACK]+1 > MaxPowerupsAllowed[POW_AMMO_RACK])
			Players[Player_num].flags&=(~PLAYER_FLAGS_AMMO_RACK);

	if (Players[Player_num].flags & PLAYER_FLAGS_CONVERTER)
		if (PowerupsInMine[POW_CONVERTER]+1 > MaxPowerupsAllowed[POW_CONVERTER])
			Players[Player_num].flags&=(~PLAYER_FLAGS_CONVERTER);

	if (Players[Player_num].flags & PLAYER_FLAGS_HEADLIGHT)
		if (PowerupsInMine[POW_HEADLIGHT]+1 > MaxPowerupsAllowed[POW_HEADLIGHT])
			Players[Player_num].flags&=(~PLAYER_FLAGS_HEADLIGHT);

	if (Game_mode & GM_CAPTURE)
	{
		if (Players[Player_num].flags & PLAYER_FLAGS_FLAG)
		{
			if (get_team(Player_num)==TEAM_RED)
				flagtype=POW_FLAG_BLUE;
			else
				flagtype=POW_FLAG_RED;

			if (PowerupsInMine[(int)flagtype]+1 > MaxPowerupsAllowed[(int)flagtype])
				Players[Player_num].flags&=(~PLAYER_FLAGS_FLAG);
		}
	}

}

// adds players inventory to multi cap

void multi_adjust_cap_for_player (int pnum)
{
	char type;

	int index;

	if (!(Game_mode & GM_NETWORK))
		return;

	for (index=0;index<MAX_PRIMARY_WEAPONS;index++)
	{
		type=Primary_weapon_to_powerup[index];
		if (Players[pnum].primary_weapon_flags & (1 << index))
		    MaxPowerupsAllowed[(int)type]++;
	}

	for (index=0;index<MAX_SECONDARY_WEAPONS;index++)
	{
		type=Secondary_weapon_to_powerup[index];
		MaxPowerupsAllowed[(int)type]+=Players[pnum].secondary_ammo[index];
	}

	if (Players[pnum].laser_level > MAX_LASER_LEVEL)
		MaxPowerupsAllowed[POW_SUPER_LASER]++;

	if (Players[pnum].flags & PLAYER_FLAGS_QUAD_LASERS)
		MaxPowerupsAllowed[POW_QUAD_FIRE]++;

	if (Players[pnum].flags & PLAYER_FLAGS_CLOAKED)
		MaxPowerupsAllowed[POW_CLOAK]++;

	if (Players[pnum].flags & PLAYER_FLAGS_MAP_ALL)
		MaxPowerupsAllowed[POW_FULL_MAP]++;

	if (Players[pnum].flags & PLAYER_FLAGS_AFTERBURNER)
		MaxPowerupsAllowed[POW_AFTERBURNER]++;

	if (Players[pnum].flags & PLAYER_FLAGS_AMMO_RACK)
		MaxPowerupsAllowed[POW_AMMO_RACK]++;

	if (Players[pnum].flags & PLAYER_FLAGS_CONVERTER)
		MaxPowerupsAllowed[POW_CONVERTER]++;

	if (Players[pnum].flags & PLAYER_FLAGS_HEADLIGHT)
		MaxPowerupsAllowed[POW_HEADLIGHT]++;
}

void multi_adjust_remote_cap (int pnum)
{
	char type;

	int index;

	if (!(Game_mode & GM_NETWORK))
		return;

	for (index=0;index<MAX_PRIMARY_WEAPONS;index++)
	{
		type=Primary_weapon_to_powerup[index];
		if (Players[pnum].primary_weapon_flags & (1 << index))
		    PowerupsInMine[(int)type]++;
	}

	for (index=0;index<MAX_SECONDARY_WEAPONS;index++)
	{
		type=Secondary_weapon_to_powerup[index];

		if ((Game_mode & GM_HOARD) && index==2)
			continue;

		if (index==2 || index==7) // PROX or SMARTMINES? Those bastards...
			PowerupsInMine[(int)type]+=(Players[pnum].secondary_ammo[index]/4);
		else
			PowerupsInMine[(int)type]+=Players[pnum].secondary_ammo[index];

	}

	if (Players[pnum].laser_level > MAX_LASER_LEVEL)
		PowerupsInMine[POW_SUPER_LASER]++;

	if (Players[pnum].flags & PLAYER_FLAGS_QUAD_LASERS)
		PowerupsInMine[POW_QUAD_FIRE]++;

	if (Players[pnum].flags & PLAYER_FLAGS_CLOAKED)
		PowerupsInMine[POW_CLOAK]++;

	if (Players[pnum].flags & PLAYER_FLAGS_MAP_ALL)
		PowerupsInMine[POW_FULL_MAP]++;

	if (Players[pnum].flags & PLAYER_FLAGS_AFTERBURNER)
		PowerupsInMine[POW_AFTERBURNER]++;

	if (Players[pnum].flags & PLAYER_FLAGS_AMMO_RACK)
		PowerupsInMine[POW_AMMO_RACK]++;

	if (Players[pnum].flags & PLAYER_FLAGS_CONVERTER)
		PowerupsInMine[POW_CONVERTER]++;

	if (Players[pnum].flags & PLAYER_FLAGS_HEADLIGHT)
		PowerupsInMine[POW_HEADLIGHT]++;

}

void
multi_send_message(void)
{
	int loc = 0;
	if (Network_message_reciever != -1)
	{
		multibuf[loc] = (char)MULTI_MESSAGE;            loc += 1;
		multibuf[loc] = (char)Player_num;                       loc += 1;
		strncpy(multibuf+loc, Network_message, MAX_MESSAGE_LEN); loc += MAX_MESSAGE_LEN;
		multibuf[loc-1] = '\0';
		multi_send_data(multibuf, loc, 0);
		Network_message_reciever = -1;
	}
}

void
multi_send_reappear()
{
	multibuf[0] = (char)MULTI_REAPPEAR;
	PUT_INTEL_SHORT(multibuf+1, Players[Player_num].objnum);

	multi_send_data(multibuf, 3, 2);
	PKilledFlags[Player_num]=0;
}

void
multi_send_position(int objnum)
{
#ifdef WORDS_BIGENDIAN
	shortpos sp;
#endif
	int count=0;

	if (Game_mode & GM_NETWORK) {
		return;
	}

	multibuf[count++] = (char)MULTI_POSITION;
#ifndef WORDS_BIGENDIAN
	create_shortpos((shortpos *)(multibuf+count), Objects+objnum,0);
	count += sizeof(shortpos);
#else
	create_shortpos(&sp, Objects+objnum, 1);
	memcpy(&(multibuf[count]), (ubyte *)(sp.bytemat), 9);
	count += 9;
	memcpy(&(multibuf[count]), (ubyte *)&(sp.xo), 14);
	count += 14;
#endif

	multi_send_data(multibuf, count, 0);
}

void
multi_send_kill(int objnum)
{
	// I died, tell the world.

	int killer_objnum;
	int count = 0;

	Assert(Objects[objnum].id == Player_num);
	killer_objnum = Players[Player_num].killer_objnum;

	multi_compute_kill(killer_objnum, objnum);

	multibuf[0] = (char)MULTI_KILL;     count += 1;
	multibuf[1] = Player_num;           count += 1;
	if (killer_objnum > -1) {
		short s;		// do it with variable since INTEL_SHORT won't work on return val from function.

		s = (short)objnum_local_to_remote(killer_objnum, (sbyte *)&multibuf[count+2]);
		PUT_INTEL_SHORT(multibuf+count, s);
	}
	else
	{
		PUT_INTEL_SHORT(multibuf+count, -1);
		multibuf[count+2] = (char)-1;
	}
	count += 3;
	multi_send_data(multibuf, count, 1);

#ifndef SHAREWARE
	if (Game_mode & GM_MULTI_ROBOTS)
		multi_strip_robots(Player_num);
#endif
}

void
multi_send_remobj(int objnum)
{
	// Tell the other guy to remove an object from his list

	sbyte obj_owner;
	short remote_objnum;

	if (Objects[objnum].type==OBJ_POWERUP && (Game_mode & GM_NETWORK))
    {
		if (PowerupsInMine[Objects[objnum].id] > 0)
		{
			PowerupsInMine[Objects[objnum].id]--;
			if (multi_powerup_is_4pack (Objects[objnum].id))
			{
				mprintf ((0,"Hey babe! Doing that wacky 4 pack stuff."));

				if (PowerupsInMine[Objects[objnum].id-1]-4<0)
					PowerupsInMine[Objects[objnum].id-1]=0;
				else
					PowerupsInMine[Objects[objnum].id-1]-=4;
			}
		}

	}

	multibuf[0] = (char)MULTI_REMOVE_OBJECT;

	remote_objnum = objnum_local_to_remote((short)objnum, &obj_owner);

	PUT_INTEL_SHORT(multibuf+1, remote_objnum); // Map to network objnums

	multibuf[3] = obj_owner;

	//      mprintf((0, "multi_send_remobj: %d = %d owner %d.\n", objnum, remote_objnum, obj_owner));

	multi_send_data(multibuf, 4, 0);

	if (Network_send_objects && network_objnum_is_past(objnum))
	{
		mprintf((0, "Resetting object sync due to object removal.\n"));
		Network_send_objnum = -1;
	}
}

void
multi_send_quit(int why)
{
	// I am quitting the game, tell the other guy the bad news.

	Assert (why == MULTI_QUIT);

	multibuf[0] = (char)why;
	multibuf[1] = Player_num;
	multi_send_data(multibuf, 2, 1);

}

void
multi_send_cloak(void)
{
	// Broadcast a change in our pflags (made to support cloaking)

	multibuf[0] = MULTI_CLOAK;
	multibuf[1] = (char)Player_num;

	multi_send_data(multibuf, 2, 1);

#ifndef SHAREWARE
	if (Game_mode & GM_MULTI_ROBOTS)
		multi_strip_robots(Player_num);
#endif
}

void
multi_send_decloak(void)
{
	// Broadcast a change in our pflags (made to support cloaking)

	multibuf[0] = MULTI_DECLOAK;
	multibuf[1] = (char)Player_num;

	multi_send_data(multibuf, 2, 1);
}

void
multi_send_door_open(int segnum, int side,ubyte flag)
{
	// When we open a door make sure everyone else opens that door

	multibuf[0] = MULTI_DOOR_OPEN;
	PUT_INTEL_SHORT(multibuf+1, segnum );
	multibuf[3] = (sbyte)side;
	multibuf[4] = flag;

	multi_send_data(multibuf, 5, 2);
}

extern void network_send_naked_packet (char *,short,int);

void
multi_send_door_open_specific(int pnum,int segnum, int side,ubyte flag)
{
	// For sending doors only to a specific person (usually when they're joining)

	Assert (Game_mode & GM_NETWORK);
	//   Assert (pnum>-1 && pnum<N_players);

	multibuf[0] = MULTI_DOOR_OPEN;
	PUT_INTEL_SHORT(multibuf+1, segnum);
	multibuf[3] = (sbyte)side;
	multibuf[4] = flag;

	network_send_naked_packet(multibuf, 5, pnum);
}

//
// Part 3 : Functions that change or prepare the game for multiplayer use.
//          Not including functions needed to syncronize or start the
//          particular type of multiplayer game.  Includes preparing the
//                      mines, player structures, etc.

void
multi_send_create_explosion(int pnum)
{
	// Send all data needed to create a remote explosion

	int count = 0;

	multibuf[count] = MULTI_CREATE_EXPLOSION;       count += 1;
	multibuf[count] = (sbyte)pnum;                  count += 1;
	//                                                                                                      -----------
	//                                                                                                      Total size = 2

	multi_send_data(multibuf, count, 0);
}

void
multi_send_controlcen_fire(vms_vector *to_goal, int best_gun_num, int objnum)
{
#ifdef WORDS_BIGENDIAN
	vms_vector swapped_vec;
#endif
	int count = 0;

	multibuf[count] = MULTI_CONTROLCEN_FIRE;                count +=  1;
#ifndef WORDS_BIGENDIAN
	memcpy(multibuf+count, to_goal, 12);                    count += 12;
#else
	swapped_vec.x = (fix)INTEL_INT( (int)to_goal->x );
	swapped_vec.y = (fix)INTEL_INT( (int)to_goal->y );
	swapped_vec.z = (fix)INTEL_INT( (int)to_goal->z );
	memcpy(multibuf+count, &swapped_vec, 12);				count += 12;
#endif
	multibuf[count] = (char)best_gun_num;                   count +=  1;
	PUT_INTEL_SHORT(multibuf+count, objnum );     count +=  2;
	//                                                                                                                      ------------
	//                                                                                                                      Total  = 16
	multi_send_data(multibuf, count, 0);
}

void
multi_send_create_powerup(int powerup_type, int segnum, int objnum, vms_vector *pos)
{
	// Create a powerup on a remote machine, used for remote
	// placement of used powerups like missiles and cloaking
	// powerups.

#ifdef WORDS_BIGENDIAN
	vms_vector swapped_vec;
#endif
	int count = 0;

	if (Game_mode & GM_NETWORK)
		PowerupsInMine[powerup_type]++;

	multibuf[count] = MULTI_CREATE_POWERUP;         count += 1;
	multibuf[count] = Player_num;                                      count += 1;
	multibuf[count] = powerup_type;                                 count += 1;
	PUT_INTEL_SHORT(multibuf+count, segnum );     count += 2;
	PUT_INTEL_SHORT(multibuf+count, objnum );     count += 2;
#ifndef WORDS_BIGENDIAN
	memcpy(multibuf+count, pos, sizeof(vms_vector));  count += sizeof(vms_vector);
#else
	swapped_vec.x = (fix)INTEL_INT( (int)pos->x );
	swapped_vec.y = (fix)INTEL_INT( (int)pos->y );
	swapped_vec.z = (fix)INTEL_INT( (int)pos->z );
	memcpy(multibuf+count, &swapped_vec, 12);				count += 12;
#endif
	//                                                                                                            -----------
	//                                                                                                            Total =  19
	multi_send_data(multibuf, count, 2);

	if (Network_send_objects && network_objnum_is_past(objnum))
	{
		mprintf((0, "Resetting object sync due to powerup creation.\n"));
		Network_send_objnum = -1;
	}

	mprintf((0, "Creating powerup type %d in segment %i.\n", powerup_type, segnum));
	map_objnum_local_to_local(objnum);
}

void
multi_send_play_sound(int sound_num, fix volume)
{
	int count = 0;
	multibuf[count] = MULTI_PLAY_SOUND;                     count += 1;
	multibuf[count] = Player_num;                                   count += 1;
	multibuf[count] = (char)sound_num;                      count += 1;
	multibuf[count] = (char)(volume >> 12); count += 1;
	//                                                                                                         -----------
	//                                                                                                         Total = 4
	multi_send_data(multibuf, count, 0);
}

void
multi_send_audio_taunt(int taunt_num)
{
	return; // Taken out, awaiting sounds..

#if 0
	int audio_taunts[4] = {
		SOUND_CONTROL_CENTER_WARNING_SIREN,
		SOUND_HOSTAGE_RESCUED,
		SOUND_REFUEL_STATION_GIVING_FUEL,
		SOUND_BAD_SELECTION
	};


	Assert(taunt_num >= 0);
	Assert(taunt_num < 4);

	digi_play_sample( audio_taunts[taunt_num], F1_0 );
	multi_send_play_sound(audio_taunts[taunt_num], F1_0);
#endif
}

void
multi_send_score(void)
{
	// Send my current score to all other players so it will remain
	// synced.
	int count = 0;

	if (Game_mode & GM_MULTI_COOP) {
		multi_sort_kill_list();
		multibuf[count] = MULTI_SCORE;                  count += 1;
		multibuf[count] = Player_num;                           count += 1;
		PUT_INTEL_INT(multibuf+count, Players[Player_num].score);  count += 4;
		multi_send_data(multibuf, count, 0);
	}
}


void
multi_send_save_game(ubyte slot, uint id, char * desc)
{
	int count = 0;

	multibuf[count] = MULTI_SAVE_GAME;              count += 1;
	multibuf[count] = slot;                         count += 1;    // Save slot=0
	PUT_INTEL_INT(multibuf+count, id );           count += 4;             // Save id
	memcpy( &multibuf[count], desc, 20 ); count += 20;

	multi_send_data(multibuf, count, 2);
}

void
multi_send_restore_game(ubyte slot, uint id)
{
	int count = 0;

	multibuf[count] = MULTI_RESTORE_GAME;   count += 1;
	multibuf[count] = slot;                                                 count += 1;             // Save slot=0
	PUT_INTEL_INT(multibuf+count, id);         count += 4;             // Save id

	multi_send_data(multibuf, count, 2);
}

void
multi_send_netplayer_stats_request(ubyte player_num)
{
	int count = 0;

	multibuf[count] = MULTI_REQ_PLAYER;     count += 1;
	multibuf[count] = player_num;                   count += 1;

	multi_send_data(multibuf, count, 0 );
}

void
multi_send_trigger(int triggernum)
{
	// Send an even to trigger something in the mine

	int count = 0;

	multibuf[count] = MULTI_TRIGGER;                                count += 1;
	multibuf[count] = Player_num;                                   count += 1;
	multibuf[count] = (ubyte)triggernum;            count += 1;

	mprintf ((0,"Sending trigger %d\n",triggernum));

	multi_send_data(multibuf, count, 1);
	//multi_send_data(multibuf, count, 1); // twice?
}

void
multi_send_hostage_door_status(int wallnum)
{
	// Tell the other player what the hit point status of a hostage door
	// should be

	int count = 0;

	Assert(Walls[wallnum].type == WALL_BLASTABLE);

	multibuf[count] = MULTI_HOSTAGE_DOOR;           count += 1;
	PUT_INTEL_SHORT(multibuf+count, wallnum );           count += 2;
	PUT_INTEL_INT(multibuf+count, Walls[wallnum].hps );  count += 4;

	//      mprintf((0, "Door %d damaged by %f points.\n", wallnum, f2fl(Walls[wallnum].hps)));

	multi_send_data(multibuf, count, 0);
}

extern int ConsistencyCount;
extern int Drop_afterburner_blob_flag;
int PhallicLimit=0;
int PhallicMan=-1;

void multi_prep_level(void)
{
	// Do any special stuff to the level required for serial games
	// before we begin playing in it.

	// Player_num MUST be set before calling this procedure.

	// This function must be called before checksuming the Object array,
	// since the resulting checksum with depend on the value of Player_num
	// at the time this is called.

	int i,ng=0;
	int     cloak_count, inv_count;

	Assert(Game_mode & GM_MULTI);

	Assert(NumNetPlayerPositions > 0);

	PhallicLimit=0;
	PhallicMan=-1;
	Drop_afterburner_blob_flag=0;
	ConsistencyCount=0;

	for (i=0;i<MAX_NUM_NET_PLAYERS;i++)
		PKilledFlags[i]=0;

	for (i = 0; i < NumNetPlayerPositions; i++)
	{
		if (i != Player_num)
			Objects[Players[i].objnum].control_type = CT_REMOTE;
		Objects[Players[i].objnum].movement_type = MT_PHYSICS;
		multi_reset_player_object(&Objects[Players[i].objnum]);
		LastPacketTime[i] = 0;
	}

#ifndef SHAREWARE
	for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++)
	{
		robot_controlled[i] = -1;
		robot_agitation[i] = 0;
		robot_fired[i] = 0;
	}
#endif

	Viewer = ConsoleObject = &Objects[Players[Player_num].objnum];

	if (!(Game_mode & GM_MULTI_COOP))
	{
		multi_delete_extra_objects(); // Removes monsters from level
	}

	if (Game_mode & GM_MULTI_ROBOTS)
	{
		multi_set_robot_ai(); // Set all Robot AI to types we can cope with
	}

	if (Game_mode & GM_NETWORK)
	{
		multi_adjust_cap_for_player(Player_num);
		multi_send_powerup_update();
		ng=1;  // ng means network game
	}
	ng=1;

	inv_count = 0;
	cloak_count = 0;
	for (i=0; i<=Highest_object_index; i++)
	{
		int objnum;

		if ((Objects[i].type == OBJ_HOSTAGE) && !(Game_mode & GM_MULTI_COOP))
		{
			objnum = obj_create(OBJ_POWERUP, POW_SHIELD_BOOST, Objects[i].segnum, &Objects[i].pos, &vmd_identity_matrix, Powerup_info[POW_SHIELD_BOOST].size, CT_POWERUP, MT_PHYSICS, RT_POWERUP);
			obj_delete(i);
			if (objnum != -1)
			{
				Objects[objnum].rtype.vclip_info.vclip_num = Powerup_info[POW_SHIELD_BOOST].vclip_num;
				Objects[objnum].rtype.vclip_info.frametime = Vclip[Objects[objnum].rtype.vclip_info.vclip_num].frame_time;
				Objects[objnum].rtype.vclip_info.framenum = 0;
				Objects[objnum].mtype.phys_info.drag = 512;     //1024;
				Objects[objnum].mtype.phys_info.mass = F1_0;
				vm_vec_zero(&Objects[objnum].mtype.phys_info.velocity);
			}
			continue;
		}

		if (Objects[i].type == OBJ_POWERUP)
		{
			if (Objects[i].id == POW_EXTRA_LIFE)
			{
				if (ng && !Netgame.DoInvulnerability)
				{
					Objects[i].id = POW_SHIELD_BOOST;
					Objects[i].rtype.vclip_info.vclip_num = Powerup_info[Objects[i].id].vclip_num;
					Objects[i].rtype.vclip_info.frametime = Vclip[Objects[i].rtype.vclip_info.vclip_num].frame_time;
				}
				else
				{
					Objects[i].id = POW_INVULNERABILITY;
					Objects[i].rtype.vclip_info.vclip_num = Powerup_info[Objects[i].id].vclip_num;
					Objects[i].rtype.vclip_info.frametime = Vclip[Objects[i].rtype.vclip_info.vclip_num].frame_time;
				}

			}

			if (!(Game_mode & GM_MULTI_COOP))
				if ((Objects[i].id >= POW_KEY_BLUE) && (Objects[i].id <= POW_KEY_GOLD))
				{
					Objects[i].id = POW_SHIELD_BOOST;
					Objects[i].rtype.vclip_info.vclip_num = Powerup_info[Objects[i].id].vclip_num;
					Objects[i].rtype.vclip_info.frametime = Vclip[Objects[i].rtype.vclip_info.vclip_num].frame_time;
				}

			if (Objects[i].id == POW_INVULNERABILITY) {
				if (inv_count >= 3 || (ng && !Netgame.DoInvulnerability)) {
					mprintf((0, "Bashing Invulnerability object #%i to shield.\n", i));
					Objects[i].id = POW_SHIELD_BOOST;
					Objects[i].rtype.vclip_info.vclip_num = Powerup_info[Objects[i].id].vclip_num;
					Objects[i].rtype.vclip_info.frametime = Vclip[Objects[i].rtype.vclip_info.vclip_num].frame_time;
				} else
					inv_count++;
			}

			if (Objects[i].id == POW_CLOAK) {
				if (cloak_count >= 3 || (ng && !Netgame.DoCloak)) {
					mprintf((0, "Bashing Cloak object #%i to shield.\n", i));
					Objects[i].id = POW_SHIELD_BOOST;
					Objects[i].rtype.vclip_info.vclip_num = Powerup_info[Objects[i].id].vclip_num;
					Objects[i].rtype.vclip_info.frametime = Vclip[Objects[i].rtype.vclip_info.vclip_num].frame_time;
				} else
					cloak_count++;
			}

			if (Objects[i].id == POW_AFTERBURNER && ng && !Netgame.DoAfterburner)
				bash_to_shield (i,"afterburner");
			if (Objects[i].id == POW_FUSION_WEAPON && ng &&  !Netgame.DoFusions)
				bash_to_shield (i,"fusion");
			if (Objects[i].id == POW_PHOENIX_WEAPON && ng && !Netgame.DoPhoenix)
				bash_to_shield (i,"phoenix");

			if (Objects[i].id == POW_HELIX_WEAPON && ng && !Netgame.DoHelix)
				bash_to_shield (i,"helix");

			if (Objects[i].id == POW_MEGA_WEAPON && ng && !Netgame.DoMegas)
				bash_to_shield (i,"mega");

			if (Objects[i].id == POW_SMARTBOMB_WEAPON && ng && !Netgame.DoSmarts)
				bash_to_shield (i,"smartmissile");

			if (Objects[i].id == POW_GAUSS_WEAPON && ng && !Netgame.DoGauss)
				bash_to_shield (i,"gauss");

			if (Objects[i].id == POW_VULCAN_WEAPON && ng && !Netgame.DoVulcan)
				bash_to_shield (i,"vulcan");

			if (Objects[i].id == POW_PLASMA_WEAPON && ng && !Netgame.DoPlasma)
				bash_to_shield (i,"plasma");

			if (Objects[i].id == POW_OMEGA_WEAPON && ng && !Netgame.DoOmega)
				bash_to_shield (i,"omega");

			if (Objects[i].id == POW_SUPER_LASER && ng && !Netgame.DoSuperLaser)
				bash_to_shield (i,"superlaser");

			if (Objects[i].id == POW_PROXIMITY_WEAPON && ng && !Netgame.DoProximity)
				bash_to_shield (i,"proximity");

			// Special: Make all proximity bombs into shields if in
			// hoard mode because we use the proximity slot in the
			// player struct to signify how many orbs the player has.

			if (Objects[i].id == POW_PROXIMITY_WEAPON && ng && (Game_mode & GM_HOARD))
				bash_to_shield (i,"proximity");

			if (Objects[i].id==POW_VULCAN_AMMO && ng && (!Netgame.DoVulcan && !Netgame.DoGauss))
				bash_to_shield(i,"vulcan ammo");

			if (Objects[i].id == POW_SPREADFIRE_WEAPON && ng && !Netgame.DoSpread)
				bash_to_shield (i,"spread");
			if (Objects[i].id == POW_SMART_MINE && ng && !Netgame.DoSmartMine)
				bash_to_shield (i,"smartmine");
			if (Objects[i].id == POW_SMISSILE1_1 && ng &&  !Netgame.DoFlash)
				bash_to_shield (i,"flash");
			if (Objects[i].id == POW_SMISSILE1_4 && ng &&  !Netgame.DoFlash)
				bash_to_shield (i,"flash");
			if (Objects[i].id == POW_GUIDED_MISSILE_1 && ng &&  !Netgame.DoGuided)
				bash_to_shield (i,"guided");
			if (Objects[i].id == POW_GUIDED_MISSILE_4 && ng &&  !Netgame.DoGuided)
				bash_to_shield (i,"guided");
			if (Objects[i].id == POW_EARTHSHAKER_MISSILE && ng &&  !Netgame.DoEarthShaker)
				bash_to_shield (i,"earth");
			if (Objects[i].id == POW_MERCURY_MISSILE_1 && ng &&  !Netgame.DoMercury)
				bash_to_shield (i,"Mercury");
			if (Objects[i].id == POW_MERCURY_MISSILE_4 && ng &&  !Netgame.DoMercury)
				bash_to_shield (i,"Mercury");
			if (Objects[i].id == POW_CONVERTER && ng &&  !Netgame.DoConverter)
				bash_to_shield (i,"Converter");
			if (Objects[i].id == POW_AMMO_RACK && ng &&  !Netgame.DoAmmoRack)
				bash_to_shield (i,"Ammo rack");
			if (Objects[i].id == POW_HEADLIGHT && ng &&  !Netgame.DoHeadlight)
				bash_to_shield (i,"Headlight");
			if (Objects[i].id == POW_LASER && ng &&  !Netgame.DoLaserUpgrade)
				bash_to_shield (i,"Laser powerup");
			if (Objects[i].id == POW_HOMING_AMMO_1 && ng &&  !Netgame.DoHoming)
				bash_to_shield (i,"Homing");
			if (Objects[i].id == POW_HOMING_AMMO_4 && ng &&  !Netgame.DoHoming)
				bash_to_shield (i,"Homing");
			if (Objects[i].id == POW_QUAD_FIRE && ng &&  !Netgame.DoQuadLasers)
				bash_to_shield (i,"Quad Lasers");
			if (Objects[i].id == POW_FLAG_BLUE && !(Game_mode & GM_CAPTURE))
				bash_to_shield (i,"Blue flag");
			if (Objects[i].id == POW_FLAG_RED && !(Game_mode & GM_CAPTURE))
				bash_to_shield (i,"Red flag");
		}
	}

	if (Game_mode & GM_HOARD)
		init_hoard_data();

	if ((Game_mode & GM_CAPTURE) || (Game_mode & GM_HOARD))
		multi_apply_goal_textures();

	multi_sort_kill_list();

	multi_show_player_list();

	ConsoleObject->control_type = CT_FLYING;

	reset_player_object();

}

int Goal_blue_segnum,Goal_red_segnum;

void multi_apply_goal_textures()
{
	int		i,j,tex;
	segment	*seg;
	segment2	*seg2;

	for (i=0; i <= Highest_segment_index; i++)
	{
		seg = &Segments[i];
		seg2 = &Segment2s[i];

		if (seg2->special==SEGMENT_IS_GOAL_BLUE)
		{

			Goal_blue_segnum = i;

			if (Game_mode & GM_HOARD)
				tex=find_goal_texture (TMI_GOAL_HOARD);
			else
				tex=find_goal_texture (TMI_GOAL_BLUE);

			if (tex>-1)
				for (j = 0; j < 6; j++) {
					int v;
					seg->sides[j].tmap_num=tex;
					for (v=0;v<4;v++)
						seg->sides[j].uvls[v].l = i2f(100);		//max out
				}

			seg2->static_light = i2f(100);	//make static light bright

		}

		if (seg2->special==SEGMENT_IS_GOAL_RED)
		{
			Goal_red_segnum = i;

			// Make both textures the same if Hoard mode

			if (Game_mode & GM_HOARD)
				tex=find_goal_texture (TMI_GOAL_HOARD);
			else
				tex=find_goal_texture (TMI_GOAL_RED);

			if (tex>-1)
				for (j = 0; j < 6; j++) {
					int v;
					seg->sides[j].tmap_num=tex;
					for (v=0;v<4;v++)
						seg->sides[j].uvls[v].l = i2f(1000);		//max out
				}

			seg2->static_light = i2f(100);	//make static light bright
		}
	}
}
int find_goal_texture (ubyte t)
{
	int i;

	for (i=0;i<NumTextures;i++)
		if (TmapInfo[i].flags & t)
			return i;

	Int3(); // Hey, there is no goal texture for this PIG!!!!
	// Edit bitmaps.tbl and designate two textures to be RED and BLUE
	// goal textures
	return (-1);
}


/* DPH: Moved to gameseq.c
   void bash_to_shield (int i,char *s)
   {
   int type=Objects[i].id;

   mprintf((0, "Bashing %s object #%i to shield.\n",s, i));

   PowerupsInMine[type]=MaxPowerupsAllowed[type]=0;

   Objects[i].id = POW_SHIELD_BOOST;
   Objects[i].rtype.vclip_info.vclip_num = Powerup_info[Objects[i].id].vclip_num;
   Objects[i].rtype.vclip_info.frametime = Vclip[Objects[i].rtype.vclip_info.vclip_num].frame_time;
   }
*/

void multi_set_robot_ai(void)
{
	// Go through the objects array looking for robots and setting
	// them to certain supported types of NET AI behavior.

	//      int i;
	//
	//      for (i = 0; i <= Highest_object_index; i++)
	//      {
	//              if (Objects[i].type == OBJ_ROBOT) {
	//                      Objects[i].ai_info.REMOTE_OWNER = -1;
	//                      if (Objects[i].ai_info.behavior == AIB_STATION)
	//                              Objects[i].ai_info.behavior = AIB_NORMAL;
	//              }
	//      }
}

int multi_delete_extra_objects()
{
	int i;
	int nnp=0;
	object *objp;

	// Go through the object list and remove any objects not used in
	// 'Anarchy!' games.

	// This function also prints the total number of available multiplayer
	// positions in this level, even though this should always be 8 or more!

	objp = Objects;
	for (i=0;i<=Highest_object_index;i++) {
		if ((objp->type==OBJ_PLAYER) || (objp->type==OBJ_GHOST))
			nnp++;
		else if ((objp->type==OBJ_ROBOT) && (Game_mode & GM_MULTI_ROBOTS))
			;
		else if ( (objp->type!=OBJ_NONE) && (objp->type!=OBJ_PLAYER) && (objp->type!=OBJ_POWERUP) && (objp->type!=OBJ_CNTRLCEN) && (objp->type!=OBJ_HOSTAGE) && !(objp->type==OBJ_WEAPON && objp->id==PMINE_ID) ) {
			// Before deleting object, if it's a robot, drop it's special powerup, if any
			if (objp->type == OBJ_ROBOT)
				if (objp->contains_count && (objp->contains_type == OBJ_POWERUP))
					object_create_egg(objp);
			obj_delete(i);
		}
		objp++;
	}

	return nnp;
}

void change_playernum_to( int new_Player_num )
{
	if (Player_num > -1)
		memcpy( Players[new_Player_num].callsign, Players[Player_num].callsign, CALLSIGN_LEN+1 );
	Player_num = new_Player_num;
}

int multi_all_players_alive()
{
	int i;
	for (i=0;i<N_players;i++)
	{
		if (PKilledFlags[i] && Players[i].connected)
			return (0);
	}
	return (1);
}

void multi_initiate_save_game()
{
	uint game_id;
	int i, slot;
	char filename[128];
	char desc[24];

	if ((Endlevel_sequence) || (Control_center_destroyed))
		return;

	if (!multi_all_players_alive())
	{
		HUD_init_message ("Can't save...all players must be alive!");
		return;
	}

	//multi_send_netplayer_stats_request(255);
	//return;

	//stop_time();

	slot = state_get_save_file(filename, desc, 1 );
	if (!slot)      {
		//start_time();
		return;
	}
	slot--;

	//start_time();

	// Make a unique game id
	game_id = timer_get_fixed_seconds();
	game_id ^= N_players<<4;
	for (i=0; i<N_players; i++ )
		game_id ^= *(uint *)Players[i].callsign;
	if ( game_id == 0 ) game_id = 1;                // 0 is invalid

	mprintf(( 1, "Game_id = %8x\n", game_id));
	multi_send_save_game(slot, game_id, desc );
	multi_do_frame();
	multi_save_game(slot,game_id, desc );
}

extern int state_get_game_id(char *);

void multi_initiate_restore_game()
{
	int slot;
	char filename[128];

	if ((Endlevel_sequence) || (Control_center_destroyed))
		return;

	if (!multi_all_players_alive())
	{
		HUD_init_message ("Can't restore...all players must be alive!");
		return;
	}

	//stop_time();
	slot = state_get_restore_file(filename,1);
	if (!slot)      {
		//start_time();
		return;
	}
	state_game_id=state_get_game_id (filename);
	if (!state_game_id)
		return;

	slot--;
	//start_time();
	multi_send_restore_game(slot,state_game_id);
	multi_do_frame();
	multi_restore_game(slot,state_game_id);
}

void multi_save_game(ubyte slot, uint id, char *desc)
{
	char filename[128];

	if ((Endlevel_sequence) || (Control_center_destroyed))
		return;

#ifndef MACINTOSH
	sprintf( filename, "%s.mg%d", Players[Player_num].callsign, slot );
#else
	sprintf( filename, ":Players:%s.mg%d", Players[Player_num].callsign, slot );
#endif
	mprintf(( 0, "Save game %x on slot %d\n", id, slot ));
	HUD_init_message( "Saving game #%d, '%s'", slot, desc );
	stop_time();
	state_game_id = id;
	state_save_all_sub(filename, desc, 0 );
}

void multi_restore_game(ubyte slot, uint id)
{
	char filename[128];
	player saved_player;
	int pnum,i;
	int thisid;

	if ((Endlevel_sequence) || (Control_center_destroyed))
		return;

	mprintf(( 0, "Restore game %x from slot %d\n", id, slot ));
	saved_player = Players[Player_num];
#ifndef MACINTOSH
	sprintf( filename, "%s.mg%d", Players[Player_num].callsign, slot );
#else
	sprintf( filename, ":Players:%s.mg%d", Players[Player_num].callsign, slot );
#endif

	for (i=0;i<N_players;i++)
		multi_strip_robots(i);

	thisid=state_get_game_id (filename);
	if (thisid!=id)
	{
		multi_bad_restore ();
		return;
	}

	pnum=state_restore_all_sub( filename, 1, 0 );

	mprintf ((0,"StateId=%d ThisID=%d\n",state_game_id,id));

#if 0
	if (state_game_id != id )       {
		// Game doesn't match!!!
		nm_messagebox( "Error", 1, "Ok", "Cannot restore saved game" );
		Game_mode |= GM_GAME_OVER;
		Function_mode = FMODE_MENU;
		longjmp(LeaveGame, 0);
	}

	change_playernum_to(pnum-1);
	memcpy( Players[Player_num].callsign, saved_player.callsign, CALLSIGN_LEN+1 );
	memcpy( Players[Player_num].net_address, saved_player.net_address, 6 );
	Players[Player_num].connected = saved_player.connected;
	Players[Player_num].n_packets_got  = saved_player.n_packets_got;
	Players[Player_num].n_packets_sent = saved_player.n_packets_sent;
	Viewer = ConsoleObject = &Objects[pnum-1];
#endif

}


void extract_netplayer_stats( netplayer_stats *ps, player * pd )
{
	int i;

	ps->flags = INTEL_INT(pd->flags);                                   // Powerup flags, see below...
	ps->energy = (fix)INTEL_INT(pd->energy);                            // Amount of energy remaining.
	ps->shields = (fix)INTEL_INT(pd->shields);                          // shields remaining (protection)
	ps->lives = pd->lives;                                              // Lives remaining, 0 = game over.
	ps->laser_level = pd->laser_level;                                  // Current level of the laser.
	ps->primary_weapon_flags=pd->primary_weapon_flags;                  // bit set indicates the player has this weapon.
	ps->secondary_weapon_flags=pd->secondary_weapon_flags;              // bit set indicates the player has this weapon.
	for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
		ps->primary_ammo[i] = INTEL_SHORT(pd->primary_ammo[i]);
	for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
		ps->secondary_ammo[i] = INTEL_SHORT(pd->secondary_ammo[i]);

	//memcpy( ps->primary_ammo, pd->primary_ammo, MAX_PRIMARY_WEAPONS*sizeof(short) );        // How much ammo of each type.
	//memcpy( ps->secondary_ammo, pd->secondary_ammo, MAX_SECONDARY_WEAPONS*sizeof(short) ); // How much ammo of each type.

	ps->last_score=INTEL_INT(pd->last_score);                           // Score at beginning of current level.
	ps->score=INTEL_INT(pd->score);                                     // Current score.
	ps->cloak_time=(fix)INTEL_INT(pd->cloak_time);                      // Time cloaked
	ps->homing_object_dist=(fix)INTEL_INT(pd->homing_object_dist);      // Distance of nearest homing object.
	ps->invulnerable_time=(fix)INTEL_INT(pd->invulnerable_time);        // Time invulnerable
	ps->KillGoalCount=INTEL_SHORT(pd->KillGoalCount);
	ps->net_killed_total=INTEL_SHORT(pd->net_killed_total);             // Number of times killed total
	ps->net_kills_total=INTEL_SHORT(pd->net_kills_total);               // Number of net kills total
	ps->num_kills_level=INTEL_SHORT(pd->num_kills_level);               // Number of kills this level
	ps->num_kills_total=INTEL_SHORT(pd->num_kills_total);               // Number of kills total
	ps->num_robots_level=INTEL_SHORT(pd->num_robots_level);             // Number of initial robots this level
	ps->num_robots_total=INTEL_SHORT(pd->num_robots_total);             // Number of robots total
	ps->hostages_rescued_total=INTEL_SHORT(pd->hostages_rescued_total); // Total number of hostages rescued.
	ps->hostages_total=INTEL_SHORT(pd->hostages_total);                 // Total number of hostages.
	ps->hostages_on_board=pd->hostages_on_board;                        // Number of hostages on ship.
}

void use_netplayer_stats( player * ps, netplayer_stats *pd )
{
	int i;

	ps->flags = INTEL_INT(pd->flags);                       // Powerup flags, see below...
	ps->energy = (fix)INTEL_INT((int)pd->energy);           // Amount of energy remaining.
	ps->shields = (fix)INTEL_INT((int)pd->shields);         // shields remaining (protection)
	ps->lives = pd->lives;                                  // Lives remaining, 0 = game over.
	ps->laser_level = pd->laser_level;                      // Current level of the laser.
	ps->primary_weapon_flags=pd->primary_weapon_flags;      // bit set indicates the player has this weapon.
	ps->secondary_weapon_flags=pd->secondary_weapon_flags;  // bit set indicates the player has this weapon.
	for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
		ps->primary_ammo[i] = INTEL_SHORT(pd->primary_ammo[i]);
	for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
		ps->secondary_ammo[i] = INTEL_SHORT(pd->secondary_ammo[i]);
	//memcpy( ps->primary_ammo, pd->primary_ammo, MAX_PRIMARY_WEAPONS*sizeof(short) );  // How much ammo of each type.
	//memcpy( ps->secondary_ammo, pd->secondary_ammo, MAX_SECONDARY_WEAPONS*sizeof(short) ); // How much ammo of each type.
	ps->last_score = INTEL_INT(pd->last_score);             // Score at beginning of current level.
	ps->score = INTEL_INT(pd->score);                       // Current score.
	ps->cloak_time = (fix)INTEL_INT((int)pd->cloak_time);   // Time cloaked
	ps->homing_object_dist = (fix)INTEL_INT((int)pd->homing_object_dist); // Distance of nearest homing object.
	ps->invulnerable_time = (fix)INTEL_INT((int)pd->invulnerable_time); // Time invulnerable
	ps->KillGoalCount=INTEL_SHORT(pd->KillGoalCount);
	ps->net_killed_total = INTEL_SHORT(pd->net_killed_total); // Number of times killed total
	ps->net_kills_total = INTEL_SHORT(pd->net_kills_total); // Number of net kills total
	ps->num_kills_level = INTEL_SHORT(pd->num_kills_level); // Number of kills this level
	ps->num_kills_total = INTEL_SHORT(pd->num_kills_total); // Number of kills total
	ps->num_robots_level = INTEL_SHORT(pd->num_robots_level); // Number of initial robots this level
	ps->num_robots_total = INTEL_SHORT(pd->num_robots_total); // Number of robots total
	ps->hostages_rescued_total = INTEL_SHORT(pd->hostages_rescued_total); // Total number of hostages rescued.
	ps->hostages_total = INTEL_SHORT(pd->hostages_total);   // Total number of hostages.
	ps->hostages_on_board=pd->hostages_on_board;            // Number of hostages on ship.
}

void multi_send_drop_weapon (int objnum,int seed)
{
	object *objp;
	int count=0;
	int ammo_count;

	objp = &Objects[objnum];

	ammo_count = objp->ctype.powerup_info.count;

	if (objp->id == POW_OMEGA_WEAPON && ammo_count == F1_0)
		ammo_count = F1_0 - 1; //make fit in short

	Assert(ammo_count < F1_0); //make sure fits in short

	multibuf[count++]=(char)MULTI_DROP_WEAPON;
	multibuf[count++]=(char)objp->id;

	PUT_INTEL_SHORT(multibuf+count, Player_num); count += 2;
	PUT_INTEL_SHORT(multibuf+count, objnum); count += 2;
	PUT_INTEL_SHORT(multibuf+count, ammo_count); count += 2;
	PUT_INTEL_INT(multibuf+count, seed);

	map_objnum_local_to_local(objnum);

	if (Game_mode & GM_NETWORK)
		PowerupsInMine[objp->id]++;

	multi_send_data(multibuf, 12, 2);
}

void multi_do_drop_weapon (char *buf)
{
	int pnum,ammo,objnum,remote_objnum,seed;
	object *objp;
	int powerup_id;

	powerup_id=(int)(buf[1]);
	pnum = GET_INTEL_SHORT(buf + 2);
	remote_objnum = GET_INTEL_SHORT(buf + 4);
	ammo = GET_INTEL_SHORT(buf + 6);
	seed = GET_INTEL_INT(buf + 8);

	objp = &Objects[Players[pnum].objnum];

	objnum = spit_powerup(objp, powerup_id, seed);

	map_objnum_local_to_remote(objnum, remote_objnum, pnum);

	if (objnum!=-1)
		Objects[objnum].ctype.powerup_info.count = ammo;

	if (Game_mode & GM_NETWORK)
		PowerupsInMine[powerup_id]++;

	mprintf ((0,"Dropped weapon %d!\n"));

}

void multi_send_guided_info (object *miss,char done)
{
#ifdef WORDS_BIGENDIAN
	shortpos sp;
#endif
	int count=0;

	mprintf ((0,"Sending guided info!\n"));

	multibuf[count++]=(char)MULTI_GUIDED;
	multibuf[count++]=(char)Player_num;
	multibuf[count++]=done;

#ifndef WORDS_BIGENDIAN
	create_shortpos((shortpos *)(multibuf+count), miss,0);
	count+=sizeof(shortpos);
#else
	create_shortpos(&sp, miss, 1);
	memcpy(&(multibuf[count]), (ubyte *)(sp.bytemat), 9);
	count += 9;
	memcpy(&(multibuf[count]), (ubyte *)&(sp.xo), 14);
	count += 14;
#endif

	multi_send_data(multibuf, count, 0);
}

void multi_do_guided (char *buf)
{
	char pnum=buf[1];
	int count=3;
	static int fun=200;
#ifdef WORDS_BIGENDIAN
	shortpos sp;
#endif

	if (Guided_missile[(int)pnum]==NULL)
	{
		if (++fun>=50)
		{
			mprintf ((0,"Guided missile for %s is NULL!\n",Players[(int)pnum].callsign));
			fun=0;
		}
		return;
	}
	else if (++fun>=50)
	{
		mprintf ((0,"Got guided info for %d (%s)\n",pnum,Players[(int)pnum].callsign));
		fun=0;
	}

	if (buf[2])
	{
		release_guided_missile(pnum);
		return;
	}


	if (Guided_missile[(int)pnum]-Objects<0 || Guided_missile[(int)pnum]-Objects > Highest_object_index)
	{
		Int3();  // Get Jason immediately!
		return;
	}

#ifndef WORDS_BIGENDIAN
	extract_shortpos(Guided_missile[(int)pnum], (shortpos *)(buf+count),0);
#else
	memcpy((ubyte *)(sp.bytemat), (ubyte *)(buf + count), 9);
	memcpy((ubyte *)&(sp.xo), (ubyte *)(buf + count + 9), 14);
	extract_shortpos(Guided_missile[(int)pnum], &sp, 1);
#endif

	count+=sizeof (shortpos);

	update_object_seg(Guided_missile[(int)pnum]);
}

void multi_send_stolen_items ()
{
	int i,count=1;
	multibuf[0]=MULTI_STOLEN_ITEMS;

	for (i=0;i<MAX_STOLEN_ITEMS;i++)
	{
		multibuf[i+1]=Stolen_items[i];
		mprintf ((0,"[%d]=%d ",i,Stolen_items[i]));
		count++;      // So I like to break my stuff into smaller chunks, so what?
	}
	mprintf ((0,"\n"));
	multi_send_data(multibuf, count, 1);
}

void multi_do_stolen_items (char *buf)
{
	int i;

	mprintf ((0,"Recieved a stolen item packet...\n"));

	for (i=0;i<MAX_STOLEN_ITEMS;i++)
	{
		Stolen_items[i]=buf[i+1];
		mprintf ((0,"[%d]=%d ",i,Stolen_items[i]));
	}
	mprintf ((0,"\n"));
}

extern void network_send_important_packet (char *,int);

void multi_send_wall_status (int wallnum,ubyte type,ubyte flags,ubyte state)
{
	int count=0;
	multibuf[count]=MULTI_WALL_STATUS;        count++;
	PUT_INTEL_SHORT(multibuf+count, wallnum);   count+=2;
	multibuf[count]=type;                 count++;
	multibuf[count]=flags;                count++;
	multibuf[count]=state;                count++;

#if 0
	if (Game_mode & GM_NETWORK)
	{
		network_send_important_packet (multibuf,count);
		network_send_important_packet (multibuf,count);
	}
	else
#endif
	{
		multi_send_data(multibuf, count, 1); // twice, just to be sure
		multi_send_data(multibuf, count, 1);
	}
}
void multi_send_wall_status_specific (int pnum,int wallnum,ubyte type,ubyte flags,ubyte state)
{
	// Send wall states a specific rejoining player

	int count=0;

	Assert (Game_mode & GM_NETWORK);
	//Assert (pnum>-1 && pnum<N_players);

	multibuf[count]=MULTI_WALL_STATUS;        count++;
	PUT_INTEL_SHORT(multibuf+count, wallnum);  count+=2;
	multibuf[count]=type;                 count++;
	multibuf[count]=flags;                count++;
	multibuf[count]=state;                count++;

	network_send_naked_packet(multibuf, count,pnum); // twice, just to be sure
	network_send_naked_packet(multibuf, count,pnum);
}

void multi_do_wall_status (char *buf)
{
	short wallnum;
	ubyte flag,type,state;

	wallnum = GET_INTEL_SHORT(buf + 1);
	type=buf[3];
	flag=buf[4];
	state=buf[5];

	Assert (wallnum>=0);
	Walls[wallnum].type=type;
	Walls[wallnum].flags=flag;
	//Assert(state <= 4);
	Walls[wallnum].state=state;

	if (Walls[wallnum].type==WALL_OPEN)
	{
		digi_kill_sound_linked_to_segment(Walls[wallnum].segnum,Walls[wallnum].sidenum,SOUND_FORCEFIELD_HUM);
		//digi_kill_sound_linked_to_segment(csegp-Segments,cside,SOUND_FORCEFIELD_HUM);
	}


	//mprintf ((0,"Got a walls packet.\n"));
}

void multi_send_jason_cheat (int num)
{
	num=num;
	return;
}

void multi_send_kill_goal_counts()
{
	int i,count=1;
	multibuf[0]=MULTI_KILLGOALS;

	for (i=0;i<MAX_PLAYERS;i++)
	{
		*(char *)(multibuf+count)=(char)Players[i].KillGoalCount;
		count++;
	}

	mprintf ((0,"MULTI: Sending KillGoalCounts...\n"));
	multi_send_data(multibuf, count, 1);
}

void multi_do_kill_goal_counts(char *buf)
{
	int i,count=1;

	for (i=0;i<MAX_PLAYERS;i++)
	{
		Players[i].KillGoalCount=*(char *)(buf+count);
		mprintf ((0,"KGC: %s has %d kills!\n",Players[i].callsign,Players[i].KillGoalCount));
		count++;
	}

}

void multi_send_heartbeat ()
{
	if (!Netgame.PlayTimeAllowed)
		return;

	multibuf[0]=MULTI_HEARTBEAT;
	PUT_INTEL_INT(multibuf+1, ThisLevelTime);
	multi_send_data(multibuf, 5, 0);
}

void multi_do_heartbeat (char *buf)
{
	fix num;

	num = GET_INTEL_INT(buf + 1);

	ThisLevelTime=num;
}

void multi_check_for_killgoal_winner ()
{
	int i,best=0,bestnum=0;
	object *objp;

	if (Control_center_destroyed)
		return;

	for (i=0;i<N_players;i++)
	{
		if (Players[i].KillGoalCount>best)
		{
			best=Players[i].KillGoalCount;
			bestnum=i;
		}
	}

	if (bestnum==Player_num)
	{
		HUD_init_message("You have the best score at %d kills!",best);
		//Players[Player_num].shields=i2f(200);
	}
	else

		HUD_init_message ("%s has the best score with %d kills!",Players[bestnum].callsign,best);

	HUD_init_message ("The control center has been destroyed!");

	objp=obj_find_first_of_type (OBJ_CNTRLCEN);
	net_destroy_controlcen (objp);
}

void multi_send_seismic (fix start,fix end)
{
	int count=1;

	multibuf[0]=MULTI_SEISMIC;
	PUT_INTEL_INT(multibuf+count, start); count+=(sizeof(fix));
	PUT_INTEL_INT(multibuf+count, end); count+=(sizeof(fix));

	multi_send_data(multibuf, count, 1);
}

extern fix Seismic_disturbance_start_time;
extern fix Seismic_disturbance_end_time;

void multi_do_seismic (char *buf)
{
	Seismic_disturbance_start_time = GET_INTEL_INT(buf + 1);
	Seismic_disturbance_end_time = GET_INTEL_INT(buf + 5);
	digi_play_sample (SOUND_SEISMIC_DISTURBANCE_START, F1_0);
}

void multi_send_light (int segnum,ubyte val)
{
	int count=1,i;
	multibuf[0]=MULTI_LIGHT;
	PUT_INTEL_INT(multibuf+count, segnum); count+=(sizeof(int));
	*(char *)(multibuf+count)=val; count++;
	for (i=0;i<6;i++)
	{
		//mprintf ((0,"Sending %d!\n",Segments[segnum].sides[i].tmap_num2));
		PUT_INTEL_SHORT(multibuf+count, Segments[segnum].sides[i].tmap_num2); count+=2;
	}
	multi_send_data(multibuf, count, 1);
}
void multi_send_light_specific (int pnum,int segnum,ubyte val)
{
	int count=1,i;

	Assert (Game_mode & GM_NETWORK);
	//  Assert (pnum>-1 && pnum<N_players);

	multibuf[0]=MULTI_LIGHT;
	PUT_INTEL_INT(multibuf+count, segnum); count+=(sizeof(int));
	*(char *)(multibuf+count)=val; count++;

	for (i=0;i<6;i++)
	{
		//mprintf ((0,"Sending %d!\n",Segments[segnum].sides[i].tmap_num2));
		PUT_INTEL_SHORT(multibuf+count, Segments[segnum].sides[i].tmap_num2); count+=2;
	}
	network_send_naked_packet(multibuf, count, pnum);
}

void multi_do_light (char *buf)
{
	int i, seg;
	ubyte sides=*(char *)(buf+5);

	seg = GET_INTEL_INT(buf + 1);
	for (i=0;i<6;i++)
	{
		if ((sides & (1<<i)))
		{
			subtract_light (seg,i);
			Segments[seg].sides[i].tmap_num2 = GET_INTEL_SHORT(buf + (6 + (2 * i)));
			//mprintf ((0,"Got %d!\n",Segments[seg].sides[i].tmap_num2));
		}
	}
}

//@@void multi_send_start_trigger(int triggernum)
//@@{
//@@	// Send an even to trigger something in the mine
//@@
//@@	int count = 0;
//@@
//@@	multibuf[count] = MULTI_START_TRIGGER;          count += 1;
//@@	multibuf[count] = Player_num;                   count += 1;
//@@	multibuf[count] = (ubyte)triggernum;            count += 1;
//@@
//@@	//mprintf ((0,"Sending start trigger %d\n",triggernum));
//@@	multi_send_data(multibuf, count, 2);
//@@}
//@@void multi_do_start_trigger(char *buf)
//@@{
//@@	int pnum = buf[1];
//@@	int trigger = buf[2];
//@@
//@@	//mprintf ((0,"MULTI doing start trigger!\n"));
//@@
//@@	if ((pnum < 0) || (pnum >= N_players) || (pnum == Player_num))
//@@	{
//@@		Int3(); // Got trigger from illegal playernum
//@@		return;
//@@	}
//@@	if ((trigger < 0) || (trigger >= Num_triggers))
//@@	{
//@@		Int3(); // Illegal trigger number in multiplayer
//@@		return;
//@@	}
//@@
//@@	if (!(Triggers[trigger].flags & TF_SPRUNG))
//@@		check_trigger_sub(trigger, pnum,0);
//@@}


void multi_do_flags (char *buf)
{
	char pnum=buf[1];
	uint flags;

	flags = GET_INTEL_INT(buf + 2);
	if (pnum!=Player_num)
		Players[(int)pnum].flags=flags;
}

void multi_send_flags (char pnum)
{
	multibuf[0]=MULTI_FLAGS;
	multibuf[1]=pnum;
	PUT_INTEL_INT(multibuf+2, Players[(int)pnum].flags);
 
	multi_send_data(multibuf, 6, 1);
}

void multi_send_drop_blobs (char pnum)
{
	multibuf[0]=MULTI_DROP_BLOB;
	multibuf[1]=pnum;

	multi_send_data(multibuf, 2, 0);
}

void multi_do_drop_blob (char *buf)
{
	char pnum=buf[1];
	drop_afterburner_blobs (&Objects[Players[(int)pnum].objnum], 2, i2f(5)/2, -1);
}

void multi_send_powerup_update ()
{
	int i;


	multibuf[0]=MULTI_POWERUP_UPDATE;
	for (i=0;i<MAX_POWERUP_TYPES;i++)
		multibuf[i+1]=MaxPowerupsAllowed[i];

	multi_send_data(multibuf, MAX_POWERUP_TYPES+1, 1);
}
void multi_do_powerup_update (char *buf)
{
	int i;

	for (i=0;i<MAX_POWERUP_TYPES;i++)
		if (buf[i+1]>MaxPowerupsAllowed[i])
			MaxPowerupsAllowed[i]=buf[i+1];
}

extern active_door ActiveDoors[];
extern int Num_open_doors;          // Number of open doors


#if 0 // never used...
void multi_send_active_door (int i)
{
	int count;

	multibuf[0]=MULTI_ACTIVE_DOOR;
	multibuf[1]=i;
	multibuf[2]=Num_open_doors;
	count = 3;
#ifndef WORDS_BIGENDIAN
	memcpy ((char *)(&multibuf[3]),&ActiveDoors[(int)i],sizeof(struct active_door));
	count += sizeof(active_door);
#else
	*(int *)(multibuf + count) = INTEL_INT(ActiveDoors[i].n_parts);                 count += 4;
	*(short *)(multibuf + count) = INTEL_SHORT(ActiveDoors[i].front_wallnum[0]);    count += 2;
	*(short *)(multibuf + count) = INTEL_SHORT(ActiveDoors[i].front_wallnum[1]);    count += 2;
	*(short *)(multibuf + count) = INTEL_SHORT(ActiveDoors[i].back_wallnum[0]);     count += 2;
	*(short *)(multibuf + count) = INTEL_SHORT(ActiveDoors[i].back_wallnum[1]);     count += 2;
	*(int *)(multibuf + count) = INTEL_INT(ActiveDoors[i].time);                    count += 4;
#endif
	//multi_send_data (multibuf,sizeof(struct active_door)+3,1);
	multi_send_data (multibuf,count,1);
}
#endif // 0 (never used)

void multi_do_active_door (char *buf)
{
	char i = multibuf[1];
	Num_open_doors = buf[2];

	memcpy(&ActiveDoors[(int)i], buf+3, sizeof(struct active_door));
#ifdef WORDS_BIGENDIAN
	{
		active_door *ad = &ActiveDoors[(int)i];
		ad->n_parts = INTEL_INT(ad->n_parts);
		ad->front_wallnum[0] = INTEL_SHORT(ad->front_wallnum[0]);
		ad->front_wallnum[1] = INTEL_SHORT(ad->front_wallnum[1]);
		ad->back_wallnum[0] = INTEL_SHORT(ad->back_wallnum[0]);
		ad->back_wallnum[1] = INTEL_SHORT(ad->back_wallnum[1]);
		ad->time = INTEL_INT(ad->time);
	}
#endif //WORDS_BIGENDIAN
}

void multi_send_sound_function (char whichfunc, char sound)
{
	int count=0;

	multibuf[0]=MULTI_SOUND_FUNCTION;   count++;
	multibuf[1]=Player_num;             count++;
	multibuf[2]=whichfunc;              count++;
#ifndef WORDS_BIGENDIAN
	*(uint *)(multibuf+count)=sound;    count++;
#else
	multibuf[3] = sound; count++;       // this would probably work on the PC as well.  Jason?
#endif
	multi_send_data (multibuf,4,0);
}

#define AFTERBURNER_LOOP_START  20098
#define AFTERBURNER_LOOP_END    25776

void multi_do_sound_function (char *buf)
{
	// for afterburner

	char pnum,whichfunc;
	int sound;

	if (Players[Player_num].connected!=1)
		return;

	pnum=buf[1];
	whichfunc=buf[2];
	sound=buf[3];

	if (whichfunc==0)
		digi_kill_sound_linked_to_object (Players[(int)pnum].objnum);
	else if (whichfunc==3)
		digi_link_sound_to_object3( sound, Players[(int)pnum].objnum, 1,F1_0, i2f(256), AFTERBURNER_LOOP_START, AFTERBURNER_LOOP_END);
}

void multi_send_capture_bonus (char pnum)
{
	Assert (Game_mode & GM_CAPTURE);

	multibuf[0]=MULTI_CAPTURE_BONUS;
	multibuf[1]=pnum;

	multi_send_data (multibuf,2,1);
	multi_do_capture_bonus (multibuf);
}
void multi_send_orb_bonus (char pnum)
{
	Assert (Game_mode & GM_HOARD);

	multibuf[0]=MULTI_ORB_BONUS;
	multibuf[1]=pnum;
	multibuf[2]=Players[Player_num].secondary_ammo[PROXIMITY_INDEX];

	multi_send_data (multibuf,3,1);
	multi_do_orb_bonus (multibuf);
}
void multi_do_capture_bonus(char *buf)
{
	// Figure out the results of a network kills and add it to the
	// appropriate player's tally.

	char pnum=buf[1];
	int TheGoal;

	kmatrix_kills_changed = 1;

	if (pnum==Player_num)
		HUD_init_message("You have Scored!");
	else
		HUD_init_message("%s has Scored!",Players[(int)pnum].callsign);

	if (pnum==Player_num)
		digi_play_sample (SOUND_HUD_YOU_GOT_GOAL,F1_0*2);
	else if (get_team(pnum)==TEAM_RED)
		digi_play_sample (SOUND_HUD_RED_GOT_GOAL,F1_0*2);
	else
		digi_play_sample (SOUND_HUD_BLUE_GOT_GOAL,F1_0*2);

	Players[(int)pnum].flags &= ~(PLAYER_FLAGS_FLAG);  // Clear capture flag

	team_kills[get_team(pnum)] += 5;
	Players[(int)pnum].net_kills_total += 5;
	Players[(int)pnum].KillGoalCount+=5;

	if (Netgame.KillGoal>0)
	{
		TheGoal=Netgame.KillGoal*5;

		if (Players[(int)pnum].KillGoalCount>=TheGoal)
		{
			if (pnum==Player_num)
			{
				HUD_init_message("You reached the kill goal!");
				Players[Player_num].shields=i2f(200);
			}
			else
				HUD_init_message ("%s has reached the kill goal!",Players[(int)pnum].callsign);

			HUD_init_message ("The control center has been destroyed!");
			net_destroy_controlcen (obj_find_first_of_type (OBJ_CNTRLCEN));
		}
	}

	multi_sort_kill_list();
	multi_show_player_list();
}

int GetOrbBonus (char num)
{
	int bonus;

	bonus=num*(num+1)/2;
	return (bonus);
}

void multi_do_orb_bonus(char *buf)
{
	// Figure out the results of a network kills and add it to the
	// appropriate player's tally.

	char pnum=buf[1];
	int TheGoal;
	int bonus=GetOrbBonus (buf[2]);

	kmatrix_kills_changed = 1;

	if (pnum==Player_num)
		HUD_init_message("You have scored %d points!",bonus);
	else
		HUD_init_message("%s has scored with %d orbs!",Players[(int)pnum].callsign,buf[2]);

	if (pnum==Player_num)
		digi_start_sound_queued (SOUND_HUD_YOU_GOT_GOAL,F1_0*2);
	else if (Game_mode & GM_TEAM)
	{
		if (get_team(pnum)==TEAM_RED)
			digi_play_sample (SOUND_HUD_RED_GOT_GOAL,F1_0*2);
		else
			digi_play_sample (SOUND_HUD_BLUE_GOT_GOAL,F1_0*2);
	}
	else
		digi_play_sample (SOUND_OPPONENT_HAS_SCORED,F1_0*2);

	if (bonus>PhallicLimit)
	{
		if (pnum==Player_num)
			HUD_init_message ("You have the record with %d points!",bonus);
		else
			HUD_init_message ("%s has the record with %d points!",Players[(int)pnum].callsign,bonus);
		digi_play_sample (SOUND_BUDDY_MET_GOAL,F1_0*2);
		PhallicMan=pnum;
		PhallicLimit=bonus;
	}

	Players[(int)pnum].flags &= ~(PLAYER_FLAGS_FLAG);  // Clear orb flag

	team_kills[get_team(pnum)] += bonus;
	Players[(int)pnum].net_kills_total += bonus;
	Players[(int)pnum].KillGoalCount+=bonus;

	team_kills[get_team(pnum)]%=1000;
	Players[(int)pnum].net_kills_total%=1000;
	Players[(int)pnum].KillGoalCount%=1000;

	if (Netgame.KillGoal>0)
	{
		TheGoal=Netgame.KillGoal*5;

		if (Players[(int)pnum].KillGoalCount>=TheGoal)
		{
			if (pnum==Player_num)
			{
				HUD_init_message("You reached the kill goal!");
				Players[Player_num].shields=i2f(200);
			}
			else
				HUD_init_message ("%s has reached the kill goal!",Players[(int)pnum].callsign);

			HUD_init_message ("The control center has been destroyed!");
			net_destroy_controlcen (obj_find_first_of_type (OBJ_CNTRLCEN));
		}
	}
	multi_sort_kill_list();
	multi_show_player_list();
}

void multi_send_got_flag (char pnum)
{
	multibuf[0]=MULTI_GOT_FLAG;
	multibuf[1]=pnum;

	digi_start_sound_queued (SOUND_HUD_YOU_GOT_FLAG,F1_0*2);

	multi_send_data (multibuf,2,1);
	multi_send_flags (Player_num);
}

int SoundHacked=0;
digi_sound ReversedSound;

void multi_send_got_orb (char pnum)
{
	multibuf[0]=MULTI_GOT_ORB;
	multibuf[1]=pnum;

	digi_play_sample (SOUND_YOU_GOT_ORB,F1_0*2);

	multi_send_data (multibuf,2,1);
	multi_send_flags (Player_num);
}

void multi_do_got_flag (char *buf)
{
	char pnum=buf[1];

	if (pnum==Player_num)
		digi_start_sound_queued (SOUND_HUD_YOU_GOT_FLAG,F1_0*2);
	else if (get_team(pnum)==TEAM_RED)
		digi_start_sound_queued (SOUND_HUD_RED_GOT_FLAG,F1_0*2);
	else
		digi_start_sound_queued (SOUND_HUD_BLUE_GOT_FLAG,F1_0*2);
	Players[(int)pnum].flags|=PLAYER_FLAGS_FLAG;
	HUD_init_message ("%s picked up a flag!",Players[(int)pnum].callsign);
}
void multi_do_got_orb (char *buf)
{
	char pnum=buf[1];

	Assert (Game_mode & GM_HOARD);

	if (Game_mode & GM_TEAM)
	{
		if (get_team(pnum)==get_team(Player_num))
			digi_play_sample (SOUND_FRIEND_GOT_ORB,F1_0*2);
		else
			digi_play_sample (SOUND_OPPONENT_GOT_ORB,F1_0*2);
    }
	else
		digi_play_sample (SOUND_OPPONENT_GOT_ORB,F1_0*2);

	Players[(int)pnum].flags|=PLAYER_FLAGS_FLAG;
	HUD_init_message ("%s picked up an orb!",Players[(int)pnum].callsign);
}


void DropOrb ()
{
	int objnum,seed;

	if (!(Game_mode & GM_HOARD))
		Int3(); // How did we get here? Get Leighton!

	if (!Players[Player_num].secondary_ammo[PROXIMITY_INDEX])
	{
		HUD_init_message("No orbs to drop!");
		return;
	}

	seed = d_rand();

	objnum = spit_powerup(ConsoleObject,POW_HOARD_ORB,seed);

	if (objnum<0)
		return;

	HUD_init_message("Orb dropped!");
	digi_play_sample (SOUND_DROP_WEAPON,F1_0);

	if ((Game_mode & GM_HOARD) && objnum>-1)
		multi_send_drop_flag(objnum,seed);

	Players[Player_num].secondary_ammo[PROXIMITY_INDEX]--;

	// If empty, tell everyone to stop drawing the box around me
	if (Players[Player_num].secondary_ammo[PROXIMITY_INDEX]==0)
		multi_send_flags (Player_num);
}

void DropFlag ()
{
	int objnum,seed;

	if (!(Game_mode & GM_CAPTURE) && !(Game_mode & GM_HOARD))
		return;
	if (Game_mode & GM_HOARD)
	{
		DropOrb();
		return;
	}

	if (!(Players[Player_num].flags & PLAYER_FLAGS_FLAG))
	{
		HUD_init_message("No flag to drop!");
		return;
	}


	HUD_init_message("Flag dropped!");
	digi_play_sample (SOUND_DROP_WEAPON,F1_0);

	seed = d_rand();

	if (get_team (Player_num)==TEAM_RED)
		objnum = spit_powerup(ConsoleObject,POW_FLAG_BLUE,seed);
	else
		objnum = spit_powerup(ConsoleObject,POW_FLAG_RED,seed);

	if (objnum<0)
		return;

	if ((Game_mode & GM_CAPTURE) && objnum>-1)
		multi_send_drop_flag(objnum,seed);

	Players[Player_num].flags &=~(PLAYER_FLAGS_FLAG);
}


void multi_send_drop_flag (int objnum,int seed)
{
	object *objp;
	int count=0;

	objp = &Objects[objnum];

	multibuf[count++]=(char)MULTI_DROP_FLAG;
	multibuf[count++]=(char)objp->id;

	PUT_INTEL_SHORT(multibuf+count, Player_num); count += 2;
	PUT_INTEL_SHORT(multibuf+count, objnum); count += 2;
	PUT_INTEL_SHORT(multibuf+count, objp->ctype.powerup_info.count); count += 2;
	PUT_INTEL_INT(multibuf+count, seed);

	map_objnum_local_to_local(objnum);

	if (!(Game_mode & GM_HOARD))
		if (Game_mode & GM_NETWORK)
			PowerupsInMine[objp->id]++;

	multi_send_data(multibuf, 12, 2);
}

void multi_do_drop_flag (char *buf)
{
	int pnum,ammo,objnum,remote_objnum,seed;
	object *objp;
	int powerup_id;

	powerup_id=buf[1];
	pnum = GET_INTEL_SHORT(buf + 2);
	remote_objnum = GET_INTEL_SHORT(buf + 4);
	ammo = GET_INTEL_SHORT(buf + 6);
	seed = GET_INTEL_INT(buf + 8);

	objp = &Objects[Players[pnum].objnum];

	objnum = spit_powerup(objp, powerup_id, seed);

	map_objnum_local_to_remote(objnum, remote_objnum, pnum);

	if (objnum!=-1)
		Objects[objnum].ctype.powerup_info.count = ammo;

	if (!(Game_mode & GM_HOARD))
	{
		if (Game_mode & GM_NETWORK)
			PowerupsInMine[powerup_id]++;
		Players[pnum].flags &= ~(PLAYER_FLAGS_FLAG);
	}
	mprintf ((0,"Dropped flag %d!\n"));

}

void multi_bad_restore ()
{
	Function_mode = FMODE_MENU;
	nm_messagebox(NULL, 1, TXT_OK,
	              "A multi-save game was restored\nthat you are missing or does not\nmatch that of the others.\nYou must rejoin if you wish to\ncontinue.");
	Function_mode = FMODE_GAME;
	multi_quit_game = 1;
	multi_leave_menu = 1;
	multi_reset_stuff();
}

extern int robot_controlled[MAX_ROBOTS_CONTROLLED];
extern int robot_agitation[MAX_ROBOTS_CONTROLLED];
extern fix robot_controlled_time[MAX_ROBOTS_CONTROLLED];
extern fix robot_last_send_time[MAX_ROBOTS_CONTROLLED];
extern fix robot_last_message_time[MAX_ROBOTS_CONTROLLED];
extern int robot_send_pending[MAX_ROBOTS_CONTROLLED];
extern int robot_fired[MAX_ROBOTS_CONTROLLED];
extern sbyte robot_fire_buf[MAX_ROBOTS_CONTROLLED][18+3];


void multi_send_robot_controls (char pnum)
{
	int count=2;

	mprintf ((0,"Sending ROBOT_CONTROLS!!!\n"));

	multibuf[0]=MULTI_ROBOT_CONTROLS;
	multibuf[1]=pnum;
	memcpy (&(multibuf[count]),&robot_controlled,MAX_ROBOTS_CONTROLLED*4);
	count+=(MAX_ROBOTS_CONTROLLED*4);
	memcpy (&(multibuf[count]),&robot_agitation,MAX_ROBOTS_CONTROLLED*4);
	count+=(MAX_ROBOTS_CONTROLLED*4);
	memcpy (&(multibuf[count]),&robot_controlled_time,MAX_ROBOTS_CONTROLLED*4);
	count+=(MAX_ROBOTS_CONTROLLED*4);
	memcpy (&(multibuf[count]),&robot_last_send_time,MAX_ROBOTS_CONTROLLED*4);
	count+=(MAX_ROBOTS_CONTROLLED*4);
	memcpy (&(multibuf[count]),&robot_last_message_time,MAX_ROBOTS_CONTROLLED*4);
	count+=(MAX_ROBOTS_CONTROLLED*4);
	memcpy (&(multibuf[count]),&robot_send_pending,MAX_ROBOTS_CONTROLLED*4);
	count+=(MAX_ROBOTS_CONTROLLED*4);
	memcpy (&(multibuf[count]),&robot_fired,MAX_ROBOTS_CONTROLLED*4);
	count+=(MAX_ROBOTS_CONTROLLED*4);

	network_send_naked_packet (multibuf,142,pnum);
}
void multi_do_robot_controls(char *buf)
{
	int count=2;

	mprintf ((0,"Recieved ROBOT_CONTROLS!!!\n"));

	if (buf[1]!=Player_num)
	{
		Int3(); // Get Jason!  Recieved a coop_sync that wasn't ours!
		return;
	}

	memcpy (&robot_controlled,&(buf[count]),MAX_ROBOTS_CONTROLLED*4);
	count+=(MAX_ROBOTS_CONTROLLED*4);
	memcpy (&robot_agitation,&(buf[count]),MAX_ROBOTS_CONTROLLED*4);
	count+=(MAX_ROBOTS_CONTROLLED*4);
	memcpy (&robot_controlled_time,&(buf[count]),MAX_ROBOTS_CONTROLLED*4);
	count+=(MAX_ROBOTS_CONTROLLED*4);
	memcpy (&robot_last_send_time,&(buf[count]),MAX_ROBOTS_CONTROLLED*4);
	count+=(MAX_ROBOTS_CONTROLLED*4);
	memcpy (&robot_last_message_time,&(buf[count]),MAX_ROBOTS_CONTROLLED*4);
	count+=(MAX_ROBOTS_CONTROLLED*4);
	memcpy (&robot_send_pending,&(buf[count]),MAX_ROBOTS_CONTROLLED*4);
	count+=(MAX_ROBOTS_CONTROLLED*4);
	memcpy (&robot_fired,&(buf[count]),MAX_ROBOTS_CONTROLLED*4);
	count+=(MAX_ROBOTS_CONTROLLED*4);
}

#define POWERUPADJUSTS 5
int PowerupAdjustMapping[]={11,19,39,41,44};

int multi_powerup_is_4pack (int id)
{
	int i;

	for (i=0;i<POWERUPADJUSTS;i++)
		if (id==PowerupAdjustMapping[i])
			return (1);
	return (0);
}

int multi_powerup_is_allowed(int id)
{
	if (id == POW_INVULNERABILITY && !Netgame.DoInvulnerability)
		return (0);
	if (id == POW_CLOAK && !Netgame.DoCloak)
		return (0);
	if (id == POW_AFTERBURNER && !Netgame.DoAfterburner)
		return (0);
	if (id == POW_FUSION_WEAPON &&  !Netgame.DoFusions)
		return (0);
	if (id == POW_PHOENIX_WEAPON && !Netgame.DoPhoenix)
		return (0);
	if (id == POW_HELIX_WEAPON && !Netgame.DoHelix)
		return (0);
	if (id == POW_MEGA_WEAPON && !Netgame.DoMegas)
		return (0);
	if (id == POW_SMARTBOMB_WEAPON && !Netgame.DoSmarts)
		return (0);
	if (id == POW_GAUSS_WEAPON && !Netgame.DoGauss)
		return (0);
	if (id == POW_VULCAN_WEAPON && !Netgame.DoVulcan)
		return (0);
	if (id == POW_PLASMA_WEAPON && !Netgame.DoPlasma)
		return (0);
	if (id == POW_OMEGA_WEAPON && !Netgame.DoOmega)
		return (0);
	if (id == POW_SUPER_LASER && !Netgame.DoSuperLaser)
		return (0);
	if (id == POW_PROXIMITY_WEAPON && !Netgame.DoProximity)
		return (0);
	if (id==POW_VULCAN_AMMO && (!Netgame.DoVulcan && !Netgame.DoGauss))
		return (0);
	if (id == POW_SPREADFIRE_WEAPON && !Netgame.DoSpread)
		return (0);
	if (id == POW_SMART_MINE && !Netgame.DoSmartMine)
		return (0);
	if (id == POW_SMISSILE1_1 &&  !Netgame.DoFlash)
		return (0);
	if (id == POW_SMISSILE1_4 &&  !Netgame.DoFlash)
		return (0);
	if (id == POW_GUIDED_MISSILE_1 &&  !Netgame.DoGuided)
		return (0);
	if (id == POW_GUIDED_MISSILE_4 &&  !Netgame.DoGuided)
		return (0);
	if (id == POW_EARTHSHAKER_MISSILE &&  !Netgame.DoEarthShaker)
		return (0);
	if (id == POW_MERCURY_MISSILE_1 &&  !Netgame.DoMercury)
		return (0);
	if (id == POW_MERCURY_MISSILE_4 &&  !Netgame.DoMercury)
		return (0);
	if (id == POW_CONVERTER &&  !Netgame.DoConverter)
		return (0);
	if (id == POW_AMMO_RACK &&  !Netgame.DoAmmoRack)
		return (0);
	if (id == POW_HEADLIGHT &&  !Netgame.DoHeadlight)
		return (0);
	if (id == POW_LASER &&  !Netgame.DoLaserUpgrade)
		return (0);
	if (id == POW_HOMING_AMMO_1 &&  !Netgame.DoHoming)
		return (0);
	if (id == POW_HOMING_AMMO_4 &&  !Netgame.DoHoming)
		return (0);
	if (id == POW_QUAD_FIRE &&  !Netgame.DoQuadLasers)
		return (0);
	if (id == POW_FLAG_BLUE && !(Game_mode & GM_CAPTURE))
		return (0);
	if (id == POW_FLAG_RED && !(Game_mode & GM_CAPTURE))
		return (0);

	return (1);
}

void multi_send_finish_game ()
{
	multibuf[0]=MULTI_FINISH_GAME;
	multibuf[1]=Player_num;

	multi_send_data (multibuf,2,1);
}


extern void do_final_boss_hacks();
void multi_do_finish_game (char *buf)
{
	if (buf[0]!=MULTI_FINISH_GAME)
		return;

	if (Current_level_num!=Last_level)
		return;

	do_final_boss_hacks();
}

void multi_send_trigger_specific (char pnum,char trig)
{
	multibuf[0] = MULTI_START_TRIGGER;
	multibuf[1] = trig;

	network_send_naked_packet(multibuf, 2, pnum);
}
void multi_do_start_trigger (char *buf)
{
	Triggers[(int)buf[1]].flags |=TF_DISABLED;
}

extern char *RankStrings[];

void multi_add_lifetime_kills ()
{
	// This function adds a kill to lifetime stats of this player, and possibly
	// gives a promotion.  If so, it will tell everyone else

	int oldrank;

	if (!Game_mode & GM_NETWORK)
		return;

	oldrank=GetMyNetRanking();

	Netlife_kills++;

	if (oldrank!=GetMyNetRanking())
	{
		multi_send_ranking();
		if (!FindArg("-norankings"))
		{
			HUD_init_message ("You have been promoted to %s!",RankStrings[GetMyNetRanking()]);
			digi_play_sample (SOUND_BUDDY_MET_GOAL,F1_0*2);
			NetPlayers.players[Player_num].rank=GetMyNetRanking();
		}
	}
	write_player_file();
}

void multi_add_lifetime_killed ()
{
	// This function adds a "killed" to lifetime stats of this player, and possibly
	// gives a demotion.  If so, it will tell everyone else

	int oldrank;

	if (!Game_mode & GM_NETWORK)
		return;

	oldrank=GetMyNetRanking();

	Netlife_killed++;

	if (oldrank!=GetMyNetRanking())
	{
		multi_send_ranking();
		NetPlayers.players[Player_num].rank=GetMyNetRanking();

		if (!FindArg("-norankings"))
			HUD_init_message ("You have been demoted to %s!",RankStrings[GetMyNetRanking()]);

	}
	write_player_file();

}

void multi_send_ranking ()
{
	multibuf[0]=(char)MULTI_RANK;
	multibuf[1]=(char)Player_num;
	multibuf[2]=(char)GetMyNetRanking();

	multi_send_data (multibuf,3,1);
}

void multi_do_ranking (char *buf)
{
	char rankstr[20];
	char pnum=buf[1];
	char rank=buf[2];

	if (NetPlayers.players[(int)pnum].rank<rank)
		strcpy (rankstr,"promoted");
	else if (NetPlayers.players[(int)pnum].rank>rank)
		strcpy (rankstr,"demoted");
	else
		return;

	NetPlayers.players[(int)pnum].rank=rank;

	if (!FindArg("-norankings"))
		HUD_init_message ("%s has been %s to %s!",Players[(int)pnum].callsign,rankstr,RankStrings[(int)rank]);
}
void multi_send_modem_ping ()
{
	multibuf[0]=MULTI_MODEM_PING;
	multi_send_data (multibuf,1,1);
}
void multi_send_modem_ping_return ()
{
	multibuf[0]=MULTI_MODEM_PING_RETURN;
	multi_send_data (multibuf,1,1);
}

void  multi_do_modem_ping_return ()
{
	if (PingLaunchTime==0)
	{
		mprintf ((0,"Got invalid PING RETURN from opponent!\n"));
		return;
	}

	PingReturnTime=timer_get_fixed_seconds();

	HUD_init_message ("Ping time for opponent is %d ms!",f2i(fixmul(PingReturnTime-PingLaunchTime,i2f(1000))));
	PingLaunchTime=0;
}


void multi_quick_sound_hack (int num)
{
	int length,i;
	num = digi_xlat_sound(num);
	length=GameSounds[num].length;
	ReversedSound.data=(ubyte *)d_malloc (length);
	ReversedSound.length=length;

	for (i=0;i<length;i++)
		ReversedSound.data[i]=GameSounds[num].data[length-i-1];

	SoundHacked=1;
}

void multi_send_play_by_play (int num,int spnum,int dpnum)
{
	if (!(Game_mode & GM_HOARD))
		return;

	return;
	multibuf[0]=MULTI_PLAY_BY_PLAY;
	multibuf[1]=(char)num;
	multibuf[2]=(char)spnum;
	multibuf[3]=(char)dpnum;
	multi_send_data (multibuf,4,1);
	multi_do_play_by_play (multibuf);
}

void multi_do_play_by_play (char *buf)
{
	int whichplay=buf[1];
	int spnum=buf[2];
	int dpnum=buf[3];

	if (!(Game_mode & GM_HOARD))
	{
		Int3(); // Get Leighton, something bad has happened.
		return;
	}

	switch (whichplay)
	{
	case 0: // Smacked!
		HUD_init_message ("Ouch! %s has been smacked by %s!",Players[dpnum].callsign,Players[spnum].callsign);
		break;
	case 1: // Spanked!
		HUD_init_message ("Haha! %s has been spanked by %s!",Players[dpnum].callsign,Players[spnum].callsign);
		break;
	default:
		Int3();
	}
}

///
/// CODE TO LOAD HOARD DATA
///


void init_bitmap(grs_bitmap *bm,int w,int h,int flags,ubyte *data)
{
	bm->bm_x = bm->bm_y = 0;
	bm->bm_w = bm->bm_rowsize = w;
	bm->bm_h = h;
	bm->bm_type = BM_LINEAR;
	bm->bm_flags = flags;
	bm->bm_data = data;
	bm->bm_handle = 0;
	bm->avg_color = 0;
}

grs_bitmap Orb_icons[2];

int Hoard_goal_eclip;

void init_hoard_data()
{
	static int first_time=1;
	static int orb_vclip;
	int n_orb_frames,n_goal_frames;
	int orb_w,orb_h;
	int icon_w,icon_h;
	ubyte palette[256*3];
	CFILE *ifile;
	int i,save_pos;
	extern int Num_bitmap_files,Num_effects,Num_sound_files;

	ifile = cfopen("hoard.ham","rb");
	if (ifile == NULL)
		Error("can't open <hoard.ham>");

	n_orb_frames = cfile_read_short(ifile);
	orb_w = cfile_read_short(ifile);
	orb_h = cfile_read_short(ifile);
	save_pos = cftell(ifile);
	cfseek(ifile,sizeof(palette)+n_orb_frames*orb_w*orb_h,SEEK_CUR);
	n_goal_frames = cfile_read_short(ifile);
	cfseek(ifile,save_pos,SEEK_SET);

	if (first_time) {
		ubyte *bitmap_data;
		int bitmap_num=Num_bitmap_files;

		//Allocate memory for bitmaps
		MALLOC( bitmap_data, ubyte, n_orb_frames*orb_w*orb_h + n_goal_frames*64*64 );

		//Create orb vclip
		orb_vclip = Num_vclips++;
		Assert(Num_vclips <= VCLIP_MAXNUM);
		Vclip[orb_vclip].play_time = F1_0/2;
		Vclip[orb_vclip].num_frames = n_orb_frames;
		Vclip[orb_vclip].frame_time = Vclip[orb_vclip].play_time / Vclip[orb_vclip].num_frames;
		Vclip[orb_vclip].flags = 0;
		Vclip[orb_vclip].sound_num = -1;
		Vclip[orb_vclip].light_value = F1_0;
		for (i=0;i<n_orb_frames;i++) {
			Vclip[orb_vclip].frames[i].index = bitmap_num;
			init_bitmap(&GameBitmaps[bitmap_num],orb_w,orb_h,BM_FLAG_TRANSPARENT,bitmap_data);
			bitmap_data += orb_w*orb_h;
			bitmap_num++;
			Assert(bitmap_num < MAX_BITMAP_FILES);
		}

		//Create obj powerup
		Powerup_info[POW_HOARD_ORB].vclip_num = orb_vclip;
		Powerup_info[POW_HOARD_ORB].hit_sound = -1; //Powerup_info[POW_SHIELD_BOOST].hit_sound;
		Powerup_info[POW_HOARD_ORB].size = Powerup_info[POW_SHIELD_BOOST].size;
		Powerup_info[POW_HOARD_ORB].light = Powerup_info[POW_SHIELD_BOOST].light;

		//Create orb goal wall effect
		Hoard_goal_eclip = Num_effects++;
		Assert(Num_effects < MAX_EFFECTS);
		Effects[Hoard_goal_eclip] = Effects[94];        //copy from blue goal
		Effects[Hoard_goal_eclip].changing_wall_texture = NumTextures;
		Effects[Hoard_goal_eclip].vc.num_frames=n_goal_frames;

		TmapInfo[NumTextures] = TmapInfo[find_goal_texture(TMI_GOAL_BLUE)];
		TmapInfo[NumTextures].eclip_num = Hoard_goal_eclip;
		TmapInfo[NumTextures].flags = TMI_GOAL_HOARD;
		NumTextures++;
		Assert(NumTextures < MAX_TEXTURES);
		for (i=0;i<n_goal_frames;i++) {
			Effects[Hoard_goal_eclip].vc.frames[i].index = bitmap_num;
			init_bitmap(&GameBitmaps[bitmap_num],64,64,0,bitmap_data);
			bitmap_data += 64*64;
			bitmap_num++;
			Assert(bitmap_num < MAX_BITMAP_FILES);
		}

	}

	//Load and remap bitmap data for orb
	cfread(palette,3,256,ifile);
	for (i=0;i<n_orb_frames;i++) {
		grs_bitmap *bm = &GameBitmaps[Vclip[orb_vclip].frames[i].index];
		cfread(bm->bm_data,1,orb_w*orb_h,ifile);
		gr_remap_bitmap_good( bm, palette, 255, -1 );
	}

	//Load and remap bitmap data for goal texture
	cfile_read_short(ifile);        //skip frame count
	cfread(palette,3,256,ifile);
	for (i=0;i<n_goal_frames;i++) {
		grs_bitmap *bm = &GameBitmaps[Effects[Hoard_goal_eclip].vc.frames[i].index];
		cfread(bm->bm_data,1,64*64,ifile);
		gr_remap_bitmap_good( bm, palette, 255, -1 );
	}

	//Load and remap bitmap data for HUD icons
	for (i=0;i<2;i++) {
		icon_w = cfile_read_short(ifile);
		icon_h = cfile_read_short(ifile);
		if (first_time) {
			ubyte *bitmap_data;
			MALLOC( bitmap_data, ubyte, icon_w*icon_h );
			init_bitmap(&Orb_icons[i],icon_w,icon_h,BM_FLAG_TRANSPARENT,bitmap_data);
		}
		cfread(palette,3,256,ifile);
		cfread(Orb_icons[i].bm_data,1,icon_w*icon_h,ifile);
		gr_remap_bitmap_good( &Orb_icons[i], palette, 255, -1 );
	}

	if (first_time) {

		//Load sounds for orb game

		for (i=0;i<4;i++) {
			int len;

			len = cfile_read_int(ifile);        //get 11k len

			if (digi_sample_rate == SAMPLE_RATE_22K) {
				cfseek(ifile,len,SEEK_CUR);     //skip over 11k sample
				len = cfile_read_int(ifile);    //get 22k len
			}

			GameSounds[Num_sound_files+i].length = len;
			GameSounds[Num_sound_files+i].data = d_malloc(len);
			cfread(GameSounds[Num_sound_files+i].data,1,len,ifile);

			if (digi_sample_rate == SAMPLE_RATE_11K) {
				len = cfile_read_int(ifile);    //get 22k len
				cfseek(ifile,len,SEEK_CUR);     //skip over 22k sample
			}

			Sounds[SOUND_YOU_GOT_ORB+i] = Num_sound_files+i;
			AltSounds[SOUND_YOU_GOT_ORB+i] = Sounds[SOUND_YOU_GOT_ORB+i];
		}
	}

	cfclose(ifile);

	first_time = 0;
}

void
multi_process_data(char *buf, int len)
{
	// Take an entire message (that has already been checked for validity,
	// if necessary) and act on it.

	int type;
	len = len;

	type = buf[0];

	if (type > MULTI_MAX_TYPE)
	{
		mprintf((1, "multi_process_data: invalid type %d.\n", type));
		Int3();
		return;
	}


#ifdef NETPROFILING
	TTRecv[type]++;
	fprintf (RecieveLogFile,"Packet type: %d Len:%d TT=%d\n",type,len,TTRecv[type]);
	fflush (RecieveLogFile);
#endif

	switch(type)
	{
	case MULTI_POSITION:
		if (!Endlevel_sequence) multi_do_position(buf); break;
	case MULTI_REAPPEAR:
		if (!Endlevel_sequence) multi_do_reappear(buf); break;
	case MULTI_FIRE:
		if (!Endlevel_sequence) multi_do_fire(buf); break;
	case MULTI_KILL:
		multi_do_kill(buf); break;
	case MULTI_REMOVE_OBJECT:
		if (!Endlevel_sequence) multi_do_remobj(buf); break;
	case MULTI_PLAYER_DROP:
	case MULTI_PLAYER_EXPLODE:
		if (!Endlevel_sequence) multi_do_player_explode(buf); break;
	case MULTI_MESSAGE:
		if (!Endlevel_sequence) multi_do_message(buf); break;
	case MULTI_QUIT:
		if (!Endlevel_sequence) multi_do_quit(buf); break;
	case MULTI_BEGIN_SYNC:
		break;
	case MULTI_CONTROLCEN:
		if (!Endlevel_sequence) multi_do_controlcen_destroy(buf); break;
	case MULTI_POWERUP_UPDATE:
		if (!Endlevel_sequence) multi_do_powerup_update(buf); break;
	case MULTI_SOUND_FUNCTION:
		multi_do_sound_function(buf); break;
	case MULTI_MARKER:
		if (!Endlevel_sequence) multi_do_drop_marker (buf); break;
	case MULTI_DROP_WEAPON:
		if (!Endlevel_sequence) multi_do_drop_weapon(buf); break;
	case MULTI_DROP_FLAG:
		if (!Endlevel_sequence) multi_do_drop_flag(buf); break;
	case MULTI_GUIDED:
		if (!Endlevel_sequence) multi_do_guided (buf); break;
	case MULTI_STOLEN_ITEMS:
		if (!Endlevel_sequence) multi_do_stolen_items(buf); break;
	case MULTI_WALL_STATUS:
		if (!Endlevel_sequence) multi_do_wall_status(buf); break;
	case MULTI_HEARTBEAT:
		if (!Endlevel_sequence) multi_do_heartbeat (buf); break;
	case MULTI_SEISMIC:
		if (!Endlevel_sequence) multi_do_seismic (buf); break;
	case MULTI_LIGHT:
		if (!Endlevel_sequence) multi_do_light (buf); break;
	case MULTI_KILLGOALS:

		if (!Endlevel_sequence) multi_do_kill_goal_counts (buf); break;
	case MULTI_ENDLEVEL_START:
		if (!Endlevel_sequence) multi_do_escape(buf); break;
	case MULTI_END_SYNC:
		break;
	case MULTI_CLOAK:
		if (!Endlevel_sequence) multi_do_cloak(buf); break;
	case MULTI_DECLOAK:
		if (!Endlevel_sequence) multi_do_decloak(buf); break;
	case MULTI_DOOR_OPEN:
		if (!Endlevel_sequence) multi_do_door_open(buf); break;
	case MULTI_CREATE_EXPLOSION:
		if (!Endlevel_sequence) multi_do_create_explosion(buf); break;
	case MULTI_CONTROLCEN_FIRE:
		if (!Endlevel_sequence) multi_do_controlcen_fire(buf); break;
	case MULTI_CREATE_POWERUP:
		if (!Endlevel_sequence) multi_do_create_powerup(buf); break;
	case MULTI_PLAY_SOUND:
		if (!Endlevel_sequence) multi_do_play_sound(buf); break;
	case MULTI_CAPTURE_BONUS:
		if (!Endlevel_sequence) multi_do_capture_bonus(buf); break;
	case MULTI_ORB_BONUS:
		if (!Endlevel_sequence) multi_do_orb_bonus(buf); break;
	case MULTI_GOT_FLAG:
		if (!Endlevel_sequence) multi_do_got_flag(buf); break;
	case MULTI_GOT_ORB:
		if (!Endlevel_sequence) multi_do_got_orb(buf); break;
	case MULTI_PLAY_BY_PLAY:
		if (!Endlevel_sequence) multi_do_play_by_play(buf); break;
	case MULTI_RANK:
		if (!Endlevel_sequence) multi_do_ranking (buf); break;
	case MULTI_MODEM_PING:
		if (!Endlevel_sequence) multi_send_modem_ping_return(); break;
	case MULTI_MODEM_PING_RETURN:
		if (!Endlevel_sequence) multi_do_modem_ping_return(); break;
#ifndef SHAREWARE
	case MULTI_FINISH_GAME:
		multi_do_finish_game(buf); break;  // do this one regardless of endsequence
	case MULTI_ROBOT_CONTROLS:
		if (!Endlevel_sequence) multi_do_robot_controls(buf); break;
	case MULTI_ROBOT_CLAIM:
		if (!Endlevel_sequence) multi_do_claim_robot(buf); break;
	case MULTI_ROBOT_POSITION:
		if (!Endlevel_sequence) multi_do_robot_position(buf); break;
	case MULTI_ROBOT_EXPLODE:
		if (!Endlevel_sequence) multi_do_robot_explode(buf); break;
	case MULTI_ROBOT_RELEASE:
		if (!Endlevel_sequence) multi_do_release_robot(buf); break;
	case MULTI_ROBOT_FIRE:
		if (!Endlevel_sequence) multi_do_robot_fire(buf); break;
#endif
	case MULTI_SCORE:
		if (!Endlevel_sequence) multi_do_score(buf); break;
	case MULTI_CREATE_ROBOT:
		if (!Endlevel_sequence) multi_do_create_robot(buf); break;
	case MULTI_TRIGGER:
		if (!Endlevel_sequence) multi_do_trigger(buf); break;
	case MULTI_START_TRIGGER:
		if (!Endlevel_sequence) multi_do_start_trigger(buf); break;
	case MULTI_FLAGS:
		if (!Endlevel_sequence) multi_do_flags(buf); break;
	case MULTI_DROP_BLOB:
		if (!Endlevel_sequence) multi_do_drop_blob(buf); break;
	case MULTI_ACTIVE_DOOR:
		if (!Endlevel_sequence) multi_do_active_door(buf); break;
	case MULTI_BOSS_ACTIONS:
		if (!Endlevel_sequence) multi_do_boss_actions(buf); break;
	case MULTI_CREATE_ROBOT_POWERUPS:
		if (!Endlevel_sequence) multi_do_create_robot_powerups(buf); break;
	case MULTI_HOSTAGE_DOOR:
		if (!Endlevel_sequence) multi_do_hostage_door_status(buf); break;
	case MULTI_SAVE_GAME:
		if (!Endlevel_sequence) multi_do_save_game(buf); break;
	case MULTI_RESTORE_GAME:
		if (!Endlevel_sequence) multi_do_restore_game(buf); break;
	case MULTI_REQ_PLAYER:
		if (!Endlevel_sequence) multi_do_req_player(buf); break;
	case MULTI_SEND_PLAYER:
		if (!Endlevel_sequence) multi_do_send_player(buf); break;

	default:
		mprintf((1, "Invalid type in multi_process_input().\n"));
		Int3();
	}
}
