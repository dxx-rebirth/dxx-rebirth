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
 * Multiplayer code for network play.
 *
 */

#ifdef NETWORK

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "game.h"
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
#include "console.h"
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
#include "newdemo.h"
#include "text.h"
#include "kmatrix.h"
#include "multibot.h"
#include "gameseq.h"
#include "physics.h"
#include "config.h"
#include "multipow.h"
#include "hudmsg.h"
#include "ctype.h"  // for isalpha
#include "vers_id.h"
#include "byteswap.h"
#include "pstypes.h"
#include "strutil.h"


//
// Local macros and prototypes
//

// LOCALIZE ME!!

#define vm_angvec_zero(v) (v)->p=(v)->b=(v)->h=0

void reset_player_object(void); // In object.c but not in object.h
void drop_player_eggs(object *player); // from collide.c
void StartLevel(void); // From gameseq.c
void GameLoop(int, int); // From game.c

//
// Global variables
//

int control_invul_time = 0;
int who_killed_controlcen = -1;  // -1 = noone

//do we draw the kill list on the HUD?
int Show_kill_list = 1;
int Show_reticle_name = 1;
fix Show_kill_list_timer = 0;

int multi_sending_message = 0;
int multi_defining_message = 0;
int multi_message_index = 0;

unsigned char multibuf[MAX_MULTI_MESSAGE_LEN+4];		// This is where multiplayer message are built
unsigned char multibuf2[MAX_MULTI_MESSAGE_LEN+4];

short remote_to_local[MAX_NUM_NET_PLAYERS][MAX_OBJECTS];  // Remote object number for each local object
short local_to_remote[MAX_OBJECTS]; 
sbyte  object_owner[MAX_OBJECTS];   // Who created each object in my universe, -1 = loaded at start

int 	Net_create_objnums[MAX_NET_CREATE_OBJECTS]; // For tracking object creation that will be sent to remote
int 	Net_create_loc = 0;  // pointer into previous array
int	Network_laser_fired = 0; // How many times we shot
int	Network_laser_gun; // Which gun number we shot
int   Network_laser_flags; // Special flags for the shot
int   Network_laser_level; // What level
short	Network_laser_track; // Who is it tracking?
char	Network_message[MAX_MESSAGE_LEN];
int	Network_message_reciever=-1;
int	sorted_kills[MAX_NUM_NET_PLAYERS];
short kill_matrix[MAX_NUM_NET_PLAYERS][MAX_NUM_NET_PLAYERS];
int 	multi_goto_secret = 0;
short	team_kills[2];
int 	multi_in_menu = 0;
int 	multi_leave_menu = 0;
int 	multi_quit_game = 0;

//added 02/26/99 Matt Mueller - reactor kill stats
short reactor_kills[MAX_NUM_NET_PLAYERS];
int reactor_kills_total;
//end addition -MM

uint	multi_allow_powerup;
uint	multi_got_pow_count;

netgame_info Netgame;

bitmap_index multi_player_textures[MAX_NUM_NET_PLAYERS][N_PLAYER_SHIP_TEXTURES];

typedef struct netplayer_stats {
	ubyte		message_type;
	ubyte		Player_num;						// Who am i?
	uint		flags;							// Powerup flags, see below...
	fix		energy;							// Amount of energy remaining.
	fix		shields;							// shields remaining (protection) 
	ubyte		lives;							// Lives remaining, 0 = game over.
 	ubyte		laser_level;					//	Current level of the laser.
	ubyte		primary_weapon_flags;					//	bit set indicates the player has this weapon.
	ubyte		secondary_weapon_flags;					//	bit set indicates the player has this weapon.
	ushort	primary_ammo[MAX_PRIMARY_WEAPONS];	// How much ammo of each type.
	ushort	secondary_ammo[MAX_SECONDARY_WEAPONS]; // How much ammo of each type.
	int		last_score;					// Score at beginning of current level.
	int		score;							// Current score.
	fix		cloak_time;						// Time cloaked
	fix		invulnerable_time;			// Time invulnerable
	fix		homing_object_dist;			//	Distance of nearest homing object.
	short		net_killed_total;				// Number of times killed total
 	short		net_kills_total;				// Number of net kills total
	short		num_kills_level;				// Number of kills this level
 	short		num_kills_total;				// Number of kills total
	short		num_robots_level; 			// Number of initial robots this level
	short		num_robots_total; 			// Number of robots total
	ushort 	hostages_rescued_total;		// Total number of hostages rescued.
	ushort	hostages_total;				// Total number of hostages.
	ubyte		hostages_on_board;			//	Number of hostages on ship.
	ubyte		unused[16];
} netplayer_stats;						  	

int message_length[MULTI_MAX_TYPE+1] = {
        24, // POSITION
        3,  // REAPPEAR
        8,  // FIRE
#ifdef SHAREWARE
        7,  // KILL
#else
        5,  // KILL
#endif
        4,  // REMOVE_OBJECT
#ifdef SHAREWARE
        56, // PLAYER_EXPLODE
#else
        57, // PLAYER_EXPLODE
#endif
#ifdef SHAREWARE
        28, // MESSAGE (MAX_MESSAGE_LENGTH = 25)
#else
        37, // MESSAGE (MAX_MESSAGE_LENGTH = 40)
#endif
        2,  // QUIT
#ifdef SHAREWARE
        10, // PLAY_SOUND
        24, // BEGIN_SYNC
#else
        4,  // PLAY_SOUND
	37, // BEGIN_SYNC
#endif
	4,  // CONTROLCEN
	5,  // CLAIM ROBOT
#ifdef SHAREWARE
	3,  // END_SYNC
#else
	4,  // END_SYNC
#endif
   2,  // CLOAK
	3,  // ENDLEVEL_START
#ifdef SHAREWARE
	7,  // DOOR_OPEN
#else
        4,  // DOOR_OPEN
#endif
	2,  // CREATE_EXPLOSION
	16, // CONTROLCEN_FIRE
#ifdef SHAREWARE
	56, // PLAYER_DROP
	7,  // CREATE_POWERUP
#else
	57, // PLAYER_DROP
	19, // CREATE_POWERUP
#endif
	9,  // MISSILE_TRACK
	2,  // DE-CLOAK
#ifndef SHAREWARE
	2,	 // MENU_CHOICE
	28, // ROBOT_POSITION  (shortpos_length (23) + 5 = 28)
	8,  // ROBOT_EXPLODE
	5,	 // ROBOT_RELEASE
	18, // ROBOT_FIRE
	6,  // SCORE
	6,  // CREATE_ROBOT
	3,  // TRIGGER
	10, // BOSS_ACTIONS	
	27, // ROBOT_POWERUPS
	7,  // HOSTAGE_DOOR
	2+24,	//SAVE_GAME		(ubyte slot, uint id, char name[20]) // obsolete
	2+4,	//RESTORE_GAME   (ubyte slot, uint id) // obsolete
	1+1,	// MULTI_REQ_PLAYER
	sizeof(netplayer_stats),								// MULTI_SEND_PLAYER
	19, // PLAYER_POWERUP_COUNT
	19, // START_POWERUP_COUNT
#else
	2,	 // MENU_CHOICE
#endif
};

void multi_reset_player_object(object *objp);
void multi_set_robot_ai(void);
void extract_netplayer_stats( netplayer_stats *ps, player * pd );

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
		return(-1);
	}

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
	Assert(remote_objnum > -1);
	Assert(owner > -1);
	Assert(owner != Player_num);
	Assert(local_objnum < MAX_OBJECTS);
	Assert(remote_objnum < MAX_OBJECTS);

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

//
// Part 1 : functions whose main purpose in life is to divert the flow
//          of execution to either network specific code based
//          on the curretn Game_mode value.
//

void
multi_endlevel_score(void)
{
#ifdef SHAREWARE
	return; // DEBUG
#endif

	// Show a score list to end of net players

	// Change to new connect state
#ifdef NETWORK
	if (Game_mode & GM_NETWORK)
	{
		Players[Player_num].connected = CONNECT_END_MENU;
	}
#endif

	// Do the actual screen we wish to show

	Function_mode = FMODE_MENU;
#ifdef NETWORK
	Network_status = NETSTAT_ENDLEVEL;
#endif

	if (Game_mode & GM_MULTI_COOP)
		DoEndLevelScoreGlitz(1);
	else
		kmatrix_view(1);

	Function_mode = FMODE_GAME;

#ifndef SHAREWARE
	if (Game_mode & GM_MULTI_COOP)
	{
         int i;
		for (i = 0; i < MaxNumNetPlayers; i++)
		// Reset keys
			Players[i].flags &= ~(PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_GOLD_KEY);
	}
#endif
}

int
get_team(int pnum)
{
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

//edited 02/26/99 Matt Mueller - reactor kill stuff, and fix this other memset that hardcoded short=2bytes
	memset(kill_matrix, 0, sizeof(kill_matrix)); // Clear kill matrix
	memset(reactor_kills, 0, sizeof(reactor_kills)); // Clear kill matrix
	reactor_kills_total=0;
//end edit -MM

	for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	{
		sorted_kills[i] = i;
		Players[i].net_killed_total = 0;
		Players[i].net_kills_total = 0;
		Players[i].flags = 0;
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
	multi_allow_powerup = NETFLAG_DOPOWERUP;
	multi_got_pow_count = 0;
}
	
void
multi_make_player_ghost(int playernum)
{
	object *obj;

//	Assert(playernum != Player_num);
//	Assert(playernum < MAX_NUM_NET_PLAYERS);

	if ((playernum == Player_num) || (playernum >= MAX_NUM_NET_PLAYERS) || (playernum < 0))
	{
		Int3(); // Non-terminal, see Rob
		return;
	}

	obj = &Objects[Players[playernum].objnum];

	obj->type = OBJ_GHOST;
	obj->render_type = RT_NONE;
	obj->movement_type = MT_NONE;
	multi_reset_player_object(obj);

#ifndef SHAREWARE
	if (Game_mode & GM_MULTI_ROBOTS)
		multi_strip_robots(playernum);
#endif
}

void
multi_make_ghost_player(int playernum)
{
	object *obj;

//	Assert(playernum != Player_num);
// Assert(playernum < MAX_NUM_NET_PLAYERS);

	if ((playernum == Player_num) || (playernum >= MAX_NUM_NET_PLAYERS))
	{
		Int3(); // Non-terminal, see rob
		return;
	}

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
//		if (Players[sorted_kills[i]].connected)
			plist[n++] = sorted_kills[i];

	if (n == 0)
		Int3(); // SEE ROB OR MATT

//	memcpy(plist, sorted_kills, N_players*sizeof(int));	

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
}

void multi_compute_kill(int killer, int killed)
{
	// Figure out the results of a network kills and add it to the
	// appropriate player's tally.

	int killed_pnum, killed_type;
	int killer_pnum, killer_type;
	char killed_name[(CALLSIGN_LEN*2)+4];
	char killer_name[(CALLSIGN_LEN*2)+4];

	kmatrix_kills_changed = 1;

	// Both object numbers are localized already!

	if ((killed < 0) || (killed > Highest_object_index) || (killer < 0) || (killer > Highest_object_index))
	{
		Int3(); // See Rob, illegal value passed to compute_kill;
		return;
	}

	killed_type = Objects[killed].type;
	killer_type = Objects[killer].type;

	if ((killed_type != OBJ_PLAYER) && (killed_type != OBJ_GHOST)) 
	{
		Int3(); // compute_kill passed non-player object!
		return;
	}

	killed_pnum = Objects[killed].id;

	Assert ((killed_pnum >= 0) && (killed_pnum < N_players));

	if (Game_mode & GM_TEAM)
		sprintf(killed_name, "%s (%s)", Players[killed_pnum].callsign, Netgame.team_name[get_team(killed_pnum)]);
	else
		sprintf(killed_name, "%s", Players[killed_pnum].callsign);

#ifndef SHAREWARE
	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_multi_death(killed_pnum);
#endif

	digi_play_sample( SOUND_HUD_KILL, F3_0 );

	if (killer_type == OBJ_CNTRLCEN)
	{
		Players[killed_pnum].net_killed_total++;
		Players[killed_pnum].net_kills_total--;

#ifndef SHAREWARE
		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_multi_kill(killed_pnum, -1);
#endif
	    
//edited 02/26/99 Matt Mueller - add kill stats to messages
		reactor_kills[killed_pnum]++;
		reactor_kills_total++;
		if (killed_pnum == Player_num)
			HUD_init_message("%s %s.", TXT_YOU_WERE, TXT_KILLED_BY_NONPLAY);
		else
			HUD_init_message("%s %s %s.", killed_name, TXT_WAS, TXT_KILLED_BY_NONPLAY );
		return;
	}

#ifndef SHAREWARE
	else if ((killer_type != OBJ_PLAYER) && (killer_type != OBJ_GHOST))
	{
		if (killed_pnum == Player_num)
			hud_message(MSGC_MULTI_KILL, "%s %s.", TXT_YOU_WERE, TXT_KILLED_BY_ROBOT);
		else
			hud_message(MSGC_MULTI_KILL, "%s %s %s.", killed_name, TXT_WAS, TXT_KILLED_BY_ROBOT );
		Players[killed_pnum].net_killed_total++;
		return;		
	}
#else
	else if ((killer_type != OBJ_PLAYER) && (killer_type != OBJ_GHOST))
	{
		Int3(); // Illegal killer type?
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
		if (Game_mode & GM_TEAM)
		{
			team_kills[get_team(killed_pnum)] -= 1;
		}
		Players[killed_pnum].net_killed_total += 1;
		Players[killed_pnum].net_kills_total -= 1;

#ifndef SHAREWARE
		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_multi_kill(killed_pnum, -1);
#endif

		kill_matrix[killed_pnum][killed_pnum] += 1; // # of suicides
		if (killer_pnum == Player_num)
			HUD_init_message("%s %s %s!", TXT_YOU, TXT_KILLED, TXT_YOURSELF );
		else
			HUD_init_message("%s %s", killed_name, TXT_SUICIDE);
	}

	else
	{
		if (Game_mode & GM_TEAM)
		{
			if (get_team(killed_pnum) == get_team(killer_pnum))
				team_kills[get_team(killed_pnum)] -= 1;
			else
				team_kills[get_team(killer_pnum)] += 1;
		}
		Players[killer_pnum].net_kills_total += 1;

#ifndef SHAREWARE
		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_multi_kill(killer_pnum, 1);
#endif

		Players[killed_pnum].net_killed_total += 1;
		kill_matrix[killer_pnum][killed_pnum] += 1;

		if (killer_pnum == Player_num) {
			HUD_init_message("%s %s %s!", TXT_YOU, TXT_KILLED, killed_name);
			if ((Game_mode & GM_MULTI_COOP) && (Players[Player_num].score >= 1000))
				add_points_to_score(-1000);
		}
		else if (killed_pnum == Player_num)
			HUD_init_message("%s %s %s!", killer_name, TXT_KILLED, TXT_YOU);
		else
			HUD_init_message("%s %s %s!", killer_name, TXT_KILLED, killed_name);
	}
	multi_sort_kill_list();
	multi_show_player_list();
}

void multi_do_frame(void)
{
	if (!(Game_mode & GM_MULTI) || Newdemo_state == ND_STATE_PLAYBACK)
	{
		Int3();
		return;
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

	network_do_frame(0, 1);

	if (multi_quit_game && !multi_in_menu)
	{
		multi_quit_game = 0;
		longjmp(LeaveGame, 0);
	}
}

//edit 03/04/99 Matt Mueller - some debug code.. ignore if you wish.
void
multi_send_data_real(unsigned char *buf, int len, int repeat,char *file,char *func,int line)
//end edit -MM
{
	Assert(buf[0] <= MULTI_MAX_TYPE);

    if (buf[0] >MULTI_MAX_TYPE)
	{
	    return;
	}
	if (Game_mode & GM_NETWORK)
		Assert(buf[0] > 0);

        Assert(len == message_length[(int)buf[0]]);

	if (Game_mode & GM_NETWORK)
	{
           network_send_data(buf, len, repeat);
	}
}

void
multi_leave_game(void)
{

//	if (Function_mode != FMODE_GAME)
//		return;

	if (!(Game_mode & GM_MULTI))
		return;

	if (Game_mode & GM_NETWORK)
	{
		Net_create_loc = 0;
		drop_player_eggs(ConsoleObject);
		multi_send_position(Players[Player_num].objnum);
		multi_send_player_explode(MULTI_PLAYER_DROP);
	}

	multi_send_quit(MULTI_QUIT);

	if (Game_mode & GM_NETWORK)
		network_leave_game();

	Game_mode |= GM_GAME_OVER;

//added on 9/20/98 by Geoff Coovert to fix Shift-ESC in multi
        if (Function_mode != FMODE_EXIT)
//end this addition - Geoff Coovert
         Function_mode = FMODE_MENU;

//	N_players = 0;

//	change_playernum_to(0);
//	Viewer = ConsoleObject = &Objects[0];

        plyr_save_stats();
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

	if (Game_mode & GM_NETWORK)
		result = network_endlevel(secret);
	
	return(result);		
}

//
// Part 2 : functions that act on network messages and change the
//          the state of the game in some way.
//

int
multi_menu_poll(void)
{
	fix old_shields;
	int was_fuelcen_alive;

	was_fuelcen_alive = Fuelcen_control_center_destroyed;

	// Special polling function for in-game menus for multiplayer

	if (! ((Game_mode & GM_MULTI) && (Function_mode == FMODE_GAME)) )
		return(0);

	if (multi_leave_menu)
		return(-1);

	old_shields = Players[Player_num].shields;

	multi_in_menu++; // Track level of menu nesting

	GameLoop( 0, 0 );			

	multi_in_menu--;

	if (Endlevel_sequence || (Fuelcen_control_center_destroyed && !was_fuelcen_alive) || (Player_is_dead && !Player_exploded) || (Players[Player_num].shields < old_shields))
	{
		multi_leave_menu = 1;
		return(-1);
	}
	if ((Fuelcen_control_center_destroyed) && (Fuelcen_seconds_left < 10))
	{
		multi_leave_menu = 1;
		return(-1);
	}

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

	if (multi_defining_message)	{
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
				if (!strncasecmp(Netgame.team_name[i], Network_message, colon-Network_message))
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
			if ((!strncasecmp(Players[i].callsign, Network_message, colon-Network_message)) && (i != Player_num) && (Players[i].connected))
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

		hud_message(MSGC_GAME_FEEDBACK, feedback_result);
	}
}

//added/moved on 11/10/98 by Victor Rachels to declare before this function
void multi_send_message_end();
//end this section change - VR

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

	if (!PlayerCfg.NetworkMessageMacro[key][0])
	{
		hud_message(MSGC_GAME_FEEDBACK, TXT_NO_MACRO);
		return;
	}

	strcpy(Network_message, PlayerCfg.NetworkMessageMacro[key]);
	Network_message_reciever = 100;

	HUD_init_message("%s '%s'", TXT_SENDING, Network_message);
	multi_message_feedback();

}


void 
multi_send_message_start()
{
	if (Game_mode&GM_MULTI)	{
		multi_sending_message = 1;
		multi_message_index = 0;
		Network_message[multi_message_index] = 0;
	}
}

// compare s1 with s2 ignoring case and only considering the
// first strlen(s2) characters of s1
int strcasecmpbegin(const char *s1, const char *s2)
{
	return strncasecmp(s1, s2, strlen(s2));
}

void multi_send_message_end()
{
	int i;

  multi_message_index = 0;
  multi_sending_message = 0;

	if (!strnicmp (Network_message,"kick:",5) && (Game_mode & GM_NETWORK))
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
				network_dump_player(Netgame.players[i].server,Netgame.players[i].node, 7);

				HUD_init_message("Dumping %s...",Players[i].callsign);
				multi_message_index = 0;
				multi_sending_message = 0;
				return;
			}
	}
	
	else if (!strnicmp (Network_message,"KillReactor",11) && (Game_mode & GM_NETWORK) && !Fuelcen_control_center_destroyed)
	{	
		if (!network_i_am_master())
			HUD_init_message ("Only %s can kill the reactor this way!",Players[network_who_is_master()].callsign);
		else
		{
			net_destroy_controlcen(NULL);
			multi_send_destroy_controlcen(-1,Player_num);
		}
		multi_message_index = 0;
		multi_sending_message = 0;
		return;
	}

	Network_message_reciever = 100;
	hud_message(MSGC_GAME_FEEDBACK, "%s '%s'", TXT_SENDING, Network_message);
	multi_send_message();
        multi_message_feedback();
	game_flush_inputs();
}

void multi_define_macro_end()
{
	Assert( multi_defining_message > 0 );

	strcpy( PlayerCfg.NetworkMessageMacro[multi_defining_message-1], Network_message );
	write_player_file();

	multi_message_index = 0;
	multi_defining_message = 0;
	game_flush_inputs();
}

void multi_message_input_sub(int key)
{
	switch( key )
	{
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
		{
			int ascii = key_ascii();
			if ((ascii < 255 ) && (ascii != 37))	{
				if (multi_message_index < MAX_MESSAGE_LEN-2 )	{
					Network_message[multi_message_index++] = ascii;
					Network_message[multi_message_index] = 0;
				} else if ( multi_sending_message )	{		
					int i;
					char * ptext, * pcolon;
					ptext = NULL;
					Network_message[multi_message_index++] = ascii;
					Network_message[multi_message_index] = 0;
					for (i=multi_message_index-1; i>=0; i-- )	{
						if ( Network_message[i]==32 )	{
							ptext = &Network_message[i+1];
							Network_message[i] = 0;
							break;
						}
					}
					multi_send_message_end();
					if ( ptext )	{
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
                //hud_message(MSGC_GAME_FEEDBACK, "%s '%s'", TXT_SENDING, Network_message);
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
		Players[Player_num].flags |= (PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_GOLD_KEY);
	}
}

void
multi_do_fire(char *buf)
{
	ubyte weapon;
	int pnum;
	sbyte flags;
	fix save_charge = Fusion_charge;
    
	// Act out the actual shooting
	pnum = buf[1];
	weapon = (int)buf[2];	
	flags = buf[4];
	Network_laser_track = GET_INTEL_SHORT(buf + 6);

	Assert (pnum < N_players);

//added on 03/05/99 Matt Mueller - add POS_FIRE capability
    if (buf[0]==MULTI_POS_FIRE)
	{
	    extract_shorterpos(&Objects[Players[pnum].objnum],(shorterpos*)(buf+8));
	}
//end addition -MM

	if (Objects[Players[pnum].objnum].type == OBJ_GHOST)
		multi_make_ghost_player(pnum);
		
	if (weapon >= MISSILE_ADJUST) 
		net_missile_firing(pnum, weapon, (int)buf[4]);
	else {
		if (weapon == FUSION_INDEX) {
			Fusion_charge = buf[4] << 12;
		}
		if (weapon == LASER_INDEX) {
			if (flags & LASER_QUAD)
				Players[pnum].flags |= PLAYER_FLAGS_QUAD_LASERS;
			else
				Players[pnum].flags &= ~PLAYER_FLAGS_QUAD_LASERS;
		}

		do_laser_firing(Players[pnum].objnum, weapon, (int)buf[3], flags, (int)buf[5]);

		if (weapon == FUSION_INDEX)
			Fusion_charge = save_charge;
	}
}

void 
multi_do_message(char *buf)
{
	char *colon,mesbuf[100];
	int t;

#ifdef SHAREWARE
	int loc = 3;
#else
	int loc = 2;
#endif

	if (((colon = strrchr(buf+loc, ':')) == NULL) || (colon-(buf+loc) < 1) || (colon-(buf+loc) > CALLSIGN_LEN))
	{
		int color = 0;
		mesbuf[0] = CC_COLOR;
		if (Game_mode & GM_TEAM)
			color = get_team((int)buf[1]);
		else
			color = (int)buf[1];
		mesbuf[1] = BM_XRGB(player_rgb[color].r,player_rgb[color].g,player_rgb[color].b);
		strcpy(&mesbuf[2], Players[(int)buf[1]].callsign);
		t = strlen(mesbuf);
		mesbuf[t] = ':';
		mesbuf[t+1] = CC_COLOR;
		mesbuf[t+2] = BM_XRGB(0, 31, 0);
		mesbuf[t+3] = 0;

		digi_play_sample(SOUND_HUD_MESSAGE, F1_0);
		HUD_init_message("%s %s", mesbuf, buf+2);
	}
	else if ( (!strncasecmp(Players[Player_num].callsign, buf+loc, colon-(buf+loc))) ||
			  ((Game_mode & GM_TEAM) && ( (get_team(Player_num) == atoi(buf+loc)-1) || !strncasecmp(Netgame.team_name[get_team(Player_num)], buf+loc, colon-(buf+loc)))) )
	{
		int color = 0;
		mesbuf[0] = CC_COLOR;
		if (Game_mode & GM_TEAM)
			color = get_team((int)buf[1]);
		else
			color = (int)buf[1];
		mesbuf[1] = BM_XRGB(player_rgb[color].r,player_rgb[color].g,player_rgb[color].b);
		strcpy(&mesbuf[2], Players[(int)buf[1]].callsign);
		t = strlen(mesbuf);
		mesbuf[t] = ':';
		mesbuf[t+1] = CC_COLOR;
		mesbuf[t+2] = BM_XRGB(0, 31, 0);
		mesbuf[t+3] = 0;

		digi_play_sample(SOUND_HUD_MESSAGE, F1_0);
		HUD_init_message("%s %s", mesbuf, colon+1);
	}
}

void
multi_do_position(char *buf)
{
#ifdef WORDS_BIGENDIAN
	shortpos sp;
#endif

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

	multi_make_ghost_player(Objects[objnum].id);

	create_player_appearance_effect(&Objects[objnum]);
}

void
multi_do_powerup_count(char *buf)
{
	int pnum = buf[1];
	int count;
	int pow_count[MAX_POWERUP_TYPES];
	int i, pow;

	memset(pow_count, 0, sizeof(pow_count));
	count = 2;
	for (i = 0; i < NUM_PLAYER_DROP_POWERUPS; i++)
	{
		pow = player_drop_powerups[i];
		if (pow == POW_VULCAN_AMMO)
		{
			pow_count[pow] = *((short *)&buf[count]); count += 2;
		}
		else
			pow_count[pow] = buf[count++];
	}

	if ((multi_got_pow_count & (1 << pnum)))
                return; // already got pow_count from this player
	if (buf[0] == MULTI_START_POWERUP_COUNT)
	{
		memset(powerup_start_level, 0, sizeof(powerup_start_level));
		multi_got_pow_count = -1;
	}
	else
		multi_got_pow_count |= (1 << pnum);
	pow_add_level_pow_count(pow_count);
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
		Network_send_objnum = -1;
	}
#endif

	// Stuff the Players structure to prepare for the explosion
	
	count = 2;
	Players[pnum].primary_weapon_flags = buf[count];				count++;
	Players[pnum].secondary_weapon_flags = buf[count];				count++;
	Players[pnum].laser_level = buf[count]; 						count++;
	Players[pnum].secondary_ammo[HOMING_INDEX] = buf[count];		count++;
	Players[pnum].secondary_ammo[CONCUSSION_INDEX] = buf[count];count++;
	Players[pnum].secondary_ammo[SMART_INDEX] = buf[count]; 	count++;
	Players[pnum].secondary_ammo[MEGA_INDEX] = buf[count];		count++;
	Players[pnum].secondary_ammo[PROXIMITY_INDEX] = buf[count]; count++;
	Players[pnum].primary_ammo[VULCAN_INDEX] = GET_INTEL_SHORT(buf + count); count += 2;
	Players[pnum].flags = GET_INTEL_INT(buf + count);               count += 4;

	objp = Objects+Players[pnum].objnum;

//	objp->phys_info.velocity = *(vms_vector *)(buf+16); // 12 bytes
//	objp->pos = *(vms_vector *)(buf+28);                // 12 bytes

	remote_created = buf[count++]; // How many did the other guy create?

	Net_create_loc = 0;

//added on 03/05/99 Matt Mueller - no more misplaced spew.. might not be the best way to do it though..
//(moving the player to where they died that is)
    if (buf[0]==MULTI_POS_PLAYER_EXPLODE)
	{
	    extract_shorterpos(objp,(shorterpos*)(buf+message_length[MULTI_PLAYER_EXPLODE]));
	}
//end addition -MM


	drop_player_eggs(objp);
 
	// Create mapping from remote to local numbering system

	// We now handle this situation gracefully, Int3 not required
	//	if (Net_create_loc != remote_created)
	//		Int3(); // Probably out of object array space, see Rob

	for (i = 0; i < remote_created; i++)
	{
		short s;

		s = GET_INTEL_SHORT(buf + count);
		if ((i < Net_create_loc) && (s > 0) &&
		    (Net_create_objnums[i] > 0))
			map_objnum_local_to_remote((short)Net_create_objnums[i], s, pnum);
		count += 2;
	}
	for (i = remote_created; i < Net_create_loc; i++) {
		Objects[Net_create_objnums[i]].flags |= OF_SHOULD_BE_DEAD;
	}

//edited on 03/05/99 Matt Mueller - no more misplaced spew
	if (buf[0] == MULTI_PLAYER_EXPLODE || buf[0] == MULTI_POS_PLAYER_EXPLODE)
//end edit -MM
	{
		explode_badass_player(objp);
		
		objp->flags &= ~OF_SHOULD_BE_DEAD;		//don't really kill player
		multi_make_player_ghost(pnum);
	}
	else
	{
		create_player_appearance_effect(objp);
	}

	Players[pnum].flags &= ~(PLAYER_FLAGS_CLOAKED | PLAYER_FLAGS_INVULNERABLE);
	Players[pnum].cloak_time = 0;
}

void
multi_do_kill(char *buf)
{
	int killer, killed;
	int count = 1;
	
#ifndef SHAREWARE
	int pnum;
	pnum = buf[count];
	if ((pnum < 0) || (pnum >= N_players))
	{
		Int3(); // Invalid player number killed
		return;
	}
	killed = Players[pnum].objnum;			
	count += 1;
#else
	killed = objnum_remote_to_local(*(short *)(buf+count), (sbyte)buf[count+2]);
	count += 3;
#endif
	killer = GET_INTEL_SHORT(buf + count);
	if (killer > 0)
		killer = objnum_remote_to_local(killer, (sbyte)buf[count+2]);

#ifdef SHAREWARE
	if ((Objects[killed].type != OBJ_PLAYER) && (Objects[killed].type != OBJ_GHOST))
	{
		Int3();
		return;
	}
#endif		

	multi_compute_kill(killer, killed);

}


//	Changed by MK on 10/20/94 to send NULL as object to net_destroy_controlcen if it got -1
// which means not a controlcen object, but contained in another object
void multi_do_controlcen_destroy(char *buf)
{
	sbyte who;
	short objnum;

	objnum = GET_INTEL_SHORT(buf + 1);
	who = buf[3];

	if (Fuelcen_control_center_destroyed != 1) 
	{
		if ((who < N_players) && (who != Player_num)) {
			hud_message(MSGC_MULTI_INFO, "%s %s", Players[who].callsign, TXT_HAS_DEST_CONTROL);
		}
		else if (who == Player_num)
			hud_message(MSGC_MULTI_INFO, TXT_YOU_DEST_CONTROL);
		else 
			hud_message(MSGC_MULTI_INFO, TXT_CONTROL_DESTROYED);

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

        if (buf[2] == 0)
	{
                digi_play_sample(SOUND_HUD_MESSAGE, F1_0);
		hud_message(MSGC_MULTI_INFO, "%s %s", Players[(int)buf[1]].callsign, TXT_HAS_ESCAPED);
#ifndef SHAREWARE
		if (Game_mode & GM_NETWORK)
			Players[(int)buf[1]].connected = CONNECT_ESCAPE_TUNNEL;
#endif
		if (!multi_goto_secret)
			multi_goto_secret = 2;
	}
        else if (buf[2] == 1) 
	{
                digi_play_sample(SOUND_HUD_MESSAGE, F1_0);
		hud_message(MSGC_MULTI_INFO, "%s %s", Players[(int)buf[1]].callsign, TXT_HAS_FOUND_SECRET);
#ifndef SHAREWARE
		if (Game_mode & GM_NETWORK)
			Players[(int)buf[1]].connected = CONNECT_FOUND_SECRET;
#endif
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

	if (local_objnum < 0)
	{
		return;
	}

	if ((Objects[local_objnum].type != OBJ_POWERUP) && (Objects[local_objnum].type != OBJ_HOSTAGE))
	{
		return;
	}
	
	if (Network_send_objects && network_objnum_is_past(local_objnum))
	{
		Network_send_objnum = -1;
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
//added/changed on 10/11/98 by Victor Rachels cuz this is annoying as a box
//-killed-                        nm_messagebox(NULL, 1, TXT_OK, TXT_YOU_ARE_ONLY);
               hud_message(MSGC_GAME_FEEDBACK, "You are the only person remaining in this netgame");
//end this change - Victor Rachels
                }

	}

	return;
}

void
multi_do_cloak(char *buf)
{
	int pnum;

	pnum = buf[1];

	Assert(pnum < N_players);
	
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

	pnum = buf[1];

	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_multi_decloak(pnum);

}
	
void
multi_do_door_open(char *buf)
{
	int segnum;
	short side;
	segment *seg;
	wall *w;

#ifdef SHAREWARE
	segnum = *(int *)(buf+1);
	side = *(short *)(buf+5);
#else
	segnum = GET_INTEL_SHORT(buf + 1);
	side = buf[3];
	
#endif
	
	if ((segnum < 0) || (segnum > Highest_segment_index) || (side < 0) || (side > 5))
	{
		Int3();
		return;
	}

	seg = &Segments[segnum];

	if (seg->sides[side].wall_num == -1) {	//Opening door on illegal wall
		Int3();
		return;
	}

	w = &Walls[seg->sides[side].wall_num];

	if (w->type == WALL_BLASTABLE)
	{
		if (!(w->flags & WALL_BLASTED))
		{
			wall_destroy(seg, side);
		}
		return;
	}
	else if (w->state != WALL_DOOR_OPENING)
	{
		wall_open_door(seg, side);
	}
}

void
multi_do_create_explosion(char *buf)
{
	int pnum;
	int count = 1;

	pnum = buf[count++];

	create_small_fireball_on_object(&Objects[Players[pnum].objnum], F1_0, 1);
}
	
void
multi_do_controlcen_fire(char *buf)
{
	vms_vector to_target;
	int gun_num;
	short objnum;
	int count = 1;

	memcpy(&to_target, buf+count, 12);	count += 12;
#ifdef WORDS_BIGENDIAN  // swap the vector to_target
	to_target.x = (fix)INTEL_INT((int)to_target.x);
	to_target.y = (fix)INTEL_INT((int)to_target.y);
	to_target.z = (fix)INTEL_INT((int)to_target.z);
#endif
	gun_num = buf[count];                       count += 1;
	objnum = GET_INTEL_SHORT(buf + count);      count += 2;

 	Laser_create_new_easy(&to_target, &Gun_pos[gun_num], objnum, CONTROLCEN_WEAPON_NUM, 1);
}

void
multi_do_create_powerup(char *buf)
{
	short segnum;
	short objnum;
	int my_objnum;
	int pnum;
	int count = 1;
	vms_vector new_pos;
	char powerup_type;

	if (Endlevel_sequence || Fuelcen_control_center_destroyed)
		return;

	pnum = buf[count++];
	powerup_type = buf[count++];
	segnum = GET_INTEL_SHORT(buf + count); count += 2;
	objnum = GET_INTEL_SHORT(buf + count); count += 2;

	if ((segnum < 0) || (segnum > Highest_segment_index)) {
		Int3();
		return;
	}
	
#ifndef SHAREWARE
	new_pos = *(vms_vector *)(buf+count); count+=sizeof(vms_vector);
#else
	compute_segment_center(&new_pos, &Segments[segnum]);
#endif
#ifdef WORDS_BIGENDIAN
	new_pos.x = (fix)SWAPINT((int)new_pos.x);
	new_pos.y = (fix)SWAPINT((int)new_pos.y);
	new_pos.z = (fix)SWAPINT((int)new_pos.z);
#endif

	if (!may_create_powerup(powerup_type))
		return;

	Net_create_loc = 0;
	my_objnum = call_object_create_egg(&Objects[Players[pnum].objnum], 1, OBJ_POWERUP, powerup_type);

	if (my_objnum < 0) {
		return;
	}

	if (Network_send_objects && network_objnum_is_past(my_objnum))
	{
		Network_send_objnum = -1;
	}

	Objects[my_objnum].pos = new_pos;

	vm_vec_zero(&Objects[my_objnum].mtype.phys_info.velocity);
	
	obj_relink(my_objnum, segnum);

	map_objnum_local_to_remote(my_objnum, objnum, pnum);

	object_create_explosion(segnum, &new_pos, i2f(5), VCLIP_POWERUP_DISAPPEARANCE);
}

void
multi_do_play_sound(char *buf)
{
	int pnum = buf[1];
#ifdef SHAREWARE
	int sound_num = *(int *)(buf+2);
	fix volume = *(fix *)(buf+6);
#else
	int sound_num = buf[2];
	fix volume = buf[3] << 12;
#endif

	if (!Players[pnum].connected)
		return;

	Assert(Players[pnum].objnum >= 0);
	Assert(Players[pnum].objnum <= Highest_object_index);

	digi_link_sound_to_object( sound_num, Players[pnum].objnum, 0, volume);	
}

#ifndef SHAREWARE
void
multi_do_score(char *buf)
{
	int pnum = buf[1];
	
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
	int pnum = buf[1];
	int trigger = buf[2];

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
	check_trigger_sub(trigger, pnum);
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

	if (hps < Walls[wallnum].hps)
		wall_damage(&Segments[Walls[wallnum].segnum], Walls[wallnum].sidenum, Walls[wallnum].hps - hps);
}

// 
void multi_do_req_player(char *buf)
{
	netplayer_stats ps;
	ubyte player_n;
	// Send my netplayer_stats to everyone!
	player_n = *(ubyte *)(buf+1);
	if ( (player_n == Player_num) || (player_n == 255)  )	{
		extract_netplayer_stats( &ps, &Players[Player_num] );
		ps.Player_num = Player_num;
		ps.message_type = MULTI_SEND_PLAYER;		// SET
		multi_send_data((ubyte*)&ps, sizeof(netplayer_stats), 1);
	}
}

void multi_do_send_player(char *buf)
{
	// Got a player packet from someone!!!
	netplayer_stats * p;
	p = (netplayer_stats *)buf;	
	Assert( p->Player_num <= N_players );
}
#endif

void
multi_reset_stuff(void)
{
	// A generic, emergency function to solve problems that crop up
	// when a player exits quick-out from the game because of a 
	// connection loss.  Fixes several weird bugs!

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
	int id;

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
//	objp->mtype.phys_info.flags &= ~(PF_TURNROLL | PF_LEVELLING | PF_WIGGLE | PF_USES_THRUST);
	objp->mtype.phys_info.flags &= ~(PF_TURNROLL | PF_LEVELLING | PF_WIGGLE);

	//Init render info

	objp->render_type = RT_POLYOBJ;
	objp->rtype.pobj_info.model_num = Player_ship->model_num;		//what model is this?
	objp->rtype.pobj_info.subobj_flags = 0;		//zero the flags
	for (i=0;i<MAX_SUBMODELS;i++)
		vm_angvec_zero(&objp->rtype.pobj_info.anim_angles[i]);

	//reset textures for this, if not player 0

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

	// Clear misc

	objp->flags = 0;
	
	if (objp->type == OBJ_GHOST)
		objp->render_type = RT_NONE;

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
		Int3();
		return;
	}

	switch(type) 
	{
		case MULTI_POSITION:
			if (!Endlevel_sequence) multi_do_position(buf); break;
		case MULTI_REAPPEAR:
			if (!Endlevel_sequence) multi_do_reappear(buf); break;
		case MULTI_FIRE:
	    //added 03/05/99 Matt Mueller - new shorter fire packets
		case MULTI_POS_FIRE:
	    //end addition -MM
                        if (!Endlevel_sequence) multi_do_fire(buf); break;
		case MULTI_KILL:
			multi_do_kill(buf); break;
		case MULTI_REMOVE_OBJECT:
			if (!Endlevel_sequence) multi_do_remobj(buf); break;
		case MULTI_PLAYER_DROP:
		case MULTI_PLAYER_EXPLODE:
	    //added 03/05/99 Matt Mueller - nomore explosions in the wrong place
		case MULTI_POS_PLAYER_EXPLODE:
	    //end addition -MM
			if (!Endlevel_sequence) multi_do_player_explode(buf); break;
		case MULTI_MESSAGE:
			if (!Endlevel_sequence) multi_do_message(buf); break;
		case MULTI_QUIT:
			if (!Endlevel_sequence) multi_do_quit(buf); break;
		case MULTI_BEGIN_SYNC:
		   break;
		case MULTI_CONTROLCEN:
			if (!Endlevel_sequence) multi_do_controlcen_destroy(buf); break;
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

#ifndef SHAREWARE
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
		case MULTI_SCORE:
			if (!Endlevel_sequence) multi_do_score(buf); break;
		case MULTI_CREATE_ROBOT:
			if (!Endlevel_sequence) multi_do_create_robot(buf); break;
		case MULTI_TRIGGER:
			if (!Endlevel_sequence) multi_do_trigger(buf); break;
		case MULTI_BOSS_ACTIONS:
			if (!Endlevel_sequence) multi_do_boss_actions(buf); break;
		case MULTI_CREATE_ROBOT_POWERUPS:
			if (!Endlevel_sequence) multi_do_create_robot_powerups(buf); break;
		case MULTI_HOSTAGE_DOOR:
			if (!Endlevel_sequence) multi_do_hostage_door_status(buf); break;
		case MULTI_REQ_PLAYER:
			if (!Endlevel_sequence) multi_do_req_player(buf); break;
		case MULTI_SEND_PLAYER:
			if (!Endlevel_sequence) multi_do_send_player(buf); break;
		case MULTI_PLAYER_POWERUP_COUNT:
		case MULTI_START_POWERUP_COUNT:
			if (!Endlevel_sequence) multi_do_powerup_count(buf); break;
#endif
		default:
			Int3();
	}
}

void
multi_process_bigdata(char *buf, int len)
{
	// Takes a bunch of messages, check them for validity,
	// and pass them to multi_process_data. 

	int type, sub_len, bytes_processed = 0;

	while( bytes_processed < len )	{
		type = buf[bytes_processed];

		if ( (type<0) || (type>MULTI_MAX_TYPE))	{
			con_printf( CON_DEBUG,"multi_process_bigdata: Invalid packet type %d!\n", type );
			return;
		}

                sub_len = message_length[type];

		Assert(sub_len > 0);

		if ( (bytes_processed+sub_len) > len )	{
			con_printf(CON_DEBUG, "multi_process_bigdata: packet type %d too short (%d>%d)!\n", type, (bytes_processed+sub_len), len );
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

void multi_send_fire(int pl)
{
   if (!Network_laser_fired)
    return;

  multibuf[1] = (char)Player_num;
  multibuf[2] = (char)Network_laser_gun;
  multibuf[3] = (char)Network_laser_level;
  multibuf[4] = (char)Network_laser_flags;
  multibuf[5] = (char)Network_laser_fired;
  PUT_INTEL_SHORT(multibuf+6, Network_laser_track);
      multibuf[0] = (char)MULTI_FIRE;
      multi_send_data(multibuf, 8, 1);

  Network_laser_fired = 0;
}


void
multi_send_destroy_controlcen(int objnum, int player)
{
	if (player == Player_num)
		hud_message(MSGC_MULTI_INFO, TXT_YOU_DEST_CONTROL);
	else if ((player > 0) && (player < N_players))
		hud_message(MSGC_MULTI_INFO, "%s %s", Players[player].callsign, TXT_HAS_DEST_CONTROL);
	else
		hud_message(MSGC_MULTI_INFO, TXT_CONTROL_DESTROYED);

	multibuf[0] = (char)MULTI_CONTROLCEN;
	PUT_INTEL_SHORT(multibuf+1, objnum);
	multibuf[3] = player;
   multi_send_data(multibuf, 4, 2);
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
multi_send_powerup_count(char type, int *pow_count)
{
	int i, pow;
	int count = 0;

	multibuf[count++] = type;
	multibuf[count++] = Player_num;
	for (i = 0; i < NUM_PLAYER_DROP_POWERUPS; i++)
	{
		pow = player_drop_powerups[i];
		if (pow == POW_VULCAN_AMMO)
		{
			*((short *)&multibuf[count]) = pow_count[pow]; count += 2;
		}
		else
			multibuf[count++] = pow_count[pow];
	}
	multi_send_data(multibuf, count, 2);
}

void
multi_send_player_powerup_count()
{
	int pow_count[MAX_POWERUP_TYPES];

	player_to_pow_count(&Players[Player_num], pow_count);
	multi_send_powerup_count(MULTI_PLAYER_POWERUP_COUNT, pow_count);
}

void
multi_send_start_powerup_count()
{
	multi_send_powerup_count(MULTI_START_POWERUP_COUNT, powerup_start_level);
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
		Network_send_objnum = -1;
	}

	multibuf[count++] = type;
	multibuf[count++] = Player_num;
	multibuf[count++] = (char)Players[Player_num].primary_weapon_flags;
        multibuf[count++] = (char)Players[Player_num].secondary_weapon_flags;
        multibuf[count++] = (char)Players[Player_num].laser_level;
        multibuf[count++] = (char)Players[Player_num].secondary_ammo[HOMING_INDEX];
        multibuf[count++] = (char)Players[Player_num].secondary_ammo[CONCUSSION_INDEX];
        multibuf[count++] = (char)Players[Player_num].secondary_ammo[SMART_INDEX];
        multibuf[count++] = (char)Players[Player_num].secondary_ammo[MEGA_INDEX];
        multibuf[count++] = (char)Players[Player_num].secondary_ammo[PROXIMITY_INDEX];
        PUT_INTEL_SHORT(multibuf+count, Players[Player_num].primary_ammo[VULCAN_INDEX] );
        count += 2;
        PUT_INTEL_INT(multibuf+count, Players[Player_num].flags );
        count += 4;

	multibuf[count++] = Net_create_loc;

	Assert(Net_create_loc <= MAX_NET_CREATE_OBJECTS);

	memset(multibuf+count, -1, MAX_NET_CREATE_OBJECTS*sizeof(short));
	
	for (i = 0; i < Net_create_loc; i++)
	{
		if (Net_create_objnums[i] <= 0) {
			#if 0 // Now legal, happens if there are too much powerups in mine
			Int3(); // Illegal value in created egg object numbers
			#endif
			continue;
		}

		PUT_INTEL_SHORT(multibuf+count, Net_create_objnums[i]); count += 2;

		// We created these objs so our local number = the network number
		map_objnum_local_to_local((short)Net_create_objnums[i]);
	}

	Net_create_loc = 0;

	if (count > message_length[MULTI_PLAYER_EXPLODE])
	{
		Int3(); // See Rob
	}

	multi_send_data(multibuf, message_length[MULTI_PLAYER_EXPLODE], 2);
	if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED)
		multi_send_decloak();
#ifndef SHAREWARE
	multi_strip_robots(Player_num);
#endif
}

void
multi_send_message(void)
{
	int loc = 0;
	if (Network_message_reciever != -1)
	{
		multibuf[loc] = (char)MULTI_MESSAGE; 		loc += 1;
                multibuf[loc] = (char)Player_num;               loc += 1;
#ifdef SHAREWARE
                loc += 1; // Dummy space for reciever (Which isn't used)
#endif
		strncpy((char*)multibuf+loc, Network_message, MAX_MESSAGE_LEN); loc += MAX_MESSAGE_LEN;
		multibuf[loc-1] = '\0';
		multi_send_data(multibuf, loc, 1);
		Network_message_reciever = -1;
	}
}

void
multi_send_reappear()
{
	multibuf[0] = (char)MULTI_REAPPEAR;
	PUT_INTEL_SHORT(multibuf+1, Players[Player_num].objnum);

	multi_send_data(multibuf, 3, 3);
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

	multibuf[count] = (char)MULTI_KILL; 	count += 1;
#ifndef SHAREWARE
	multibuf[1] = Player_num;					count += 1;
#else
	*(short *)(multibuf+count) = (short)objnum_local_to_remote(objnum, (sbyte *)&multibuf[count+2]);
	count += 3;
#endif

	Assert(Objects[objnum].id == Player_num);
	killer_objnum = Players[Player_num].killer_objnum;
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

	multi_compute_kill(killer_objnum, objnum);
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

	multibuf[0] = (char)MULTI_REMOVE_OBJECT;

	remote_objnum = objnum_local_to_remote((short)objnum, &obj_owner);

	PUT_INTEL_SHORT(multibuf+1, remote_objnum); // Map to network objnums

	multibuf[3] = obj_owner;	

	multi_send_data(multibuf, 4, 1);

	if (Network_send_objects && network_objnum_is_past(objnum))
	{
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
multi_send_door_open(int segnum, int side)
{
	multibuf[0] = MULTI_DOOR_OPEN;
#ifdef SHAREWARE
	*(int *)(multibuf+1) = segnum;
	*(short *)(multibuf+5) = (short)side;
	multi_send_data(multibuf, 7, 1);
#else
	PUT_INTEL_SHORT(multibuf+1, segnum );
	multibuf[3] = (sbyte)side;
	multi_send_data(multibuf, 4, 1);
#endif

}

//
// Part 3 : Functions that change or prepare the game for multiplayer use.
//          Not including functions needed to syncronize or start the 
//          particular type of multiplayer game.  Includes preparing the
// 			mines, player structures, etc.

void
multi_send_create_explosion(int pnum)
{
	// Send all data needed to create a remote explosion

	int count = 0;

	multibuf[count] = MULTI_CREATE_EXPLOSION; 	count += 1;
	multibuf[count] = (sbyte)pnum;					count += 1;
	//													-----------
	//													Total size = 2

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
	memcpy(multibuf+count, &swapped_vec, 12);		count += 12;
#endif
	multibuf[count] = (char)best_gun_num;                   count +=  1;
	PUT_INTEL_SHORT(multibuf+count, objnum );		count +=  2;
	//							------------
	//							Total  = 16
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

	multibuf[count] = MULTI_CREATE_POWERUP;			count += 1;
	multibuf[count] = Player_num;				count += 1;
	multibuf[count] = powerup_type;				count += 1;
	PUT_INTEL_SHORT(multibuf+count, segnum );		count += 2;
	PUT_INTEL_SHORT(multibuf+count, objnum );		count += 2;
#if !defined(WORDS_BIGENDIAN) && !defined(SHAREWARE)
	memcpy(multibuf+count, pos, sizeof(vms_vector));	count += sizeof(vms_vector);
#else
	swapped_vec.x = (fix)INTEL_INT( (int)pos->x );
	swapped_vec.y = (fix)INTEL_INT( (int)pos->y );
	swapped_vec.z = (fix)INTEL_INT( (int)pos->z );
	memcpy(multibuf+count, &swapped_vec, 12);		count += 12;
#endif
	//							-----------
	//							Total =  19
	multi_send_data(multibuf, count, 2);

	if (Network_send_objects && network_objnum_is_past(objnum))
	{
		Network_send_objnum = -1;
	}

	map_objnum_local_to_local(objnum);
}

void
multi_send_play_sound(int sound_num, fix volume)
{
	int count = 0;

	multibuf[count] = MULTI_PLAY_SOUND;			count += 1;
        multibuf[count] = Player_num;                           count += 1;
#ifdef SHAREWARE
	*(int *)(multibuf+count) = sound_num;			count += 4;
        *(fix *)(multibuf+count) = volume;                      count += 4;
	//															-----------
	//															Total = 10
#else
	multibuf[count] = (char)sound_num;			count += 1;
	multibuf[count] = (char)(volume >> 12);	count += 1;
	//													   -----------
	//													   Total = 4
#endif
	multi_send_data(multibuf, count, 1);
}

void
multi_send_audio_taunt(int taunt_num)
{
#ifdef AUDIO_TAUNTS
        int audio_taunts[4] = {
		// Begin addition by GF
		SOUND_CONTROL_CENTER_WARNING_SIREN,
		SOUND_HOMING_WARNING,
		SOUND_CONTROL_CENTER_DESTROYED,
		SOUND_MINE_BLEW_UP
		// End addition by GF
	};


	Assert(taunt_num >= 0);
	Assert(taunt_num < 4);

	digi_play_sample( audio_taunts[taunt_num], F1_0 );
	multi_send_play_sound(audio_taunts[taunt_num], F1_0);
#endif
}

#ifndef SHAREWARE
void
multi_send_score(void)
{
	// Send my current score to all other players so it will remain
	// synced.
	int count = 0;

	if (Game_mode & GM_MULTI_COOP) {
		multi_sort_kill_list();
		multibuf[count] = MULTI_SCORE;			count += 1;
		multibuf[count] = Player_num;				count += 1;
		PUT_INTEL_INT(multibuf+count, Players[Player_num].score);  count += 4;
		multi_send_data(multibuf, count, 0);
	}
}	


void
multi_send_netplayer_stats_request(ubyte player_num)
{
	int count = 0;
	
	multibuf[count] = MULTI_REQ_PLAYER;	count += 1;
	multibuf[count] = player_num;			count += 1;

	multi_send_data(multibuf, count, 2 );
}



void
multi_send_trigger(int triggernum)
{
        // Send an event to trigger something in the mine
	
	int count = 0;

	multibuf[count] = MULTI_TRIGGER;				count += 1;
	multibuf[count] = Player_num;					count += 1;
	multibuf[count] = (ubyte)triggernum;		count += 1;

	multi_send_data(multibuf, count, 2);
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

	multi_send_data(multibuf, count, 0);
}
#endif

void
multi_prep_level(void)
{
	// Do any special stuff to the level required for games
	// before we begin playing in it.

	// Player_num MUST be set before calling this procedure.  

	// This function must be called before checksuming the Object array,
	// since the resulting checksum with depend on the value of Player_num
	// at the time this is called.

	int i;
	int	cloak_count, inv_count;

	Assert(Game_mode & GM_MULTI);

	Assert(NumNetPlayerPositions > 0);

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
				Objects[objnum].mtype.phys_info.drag = 512;	//1024;
				Objects[objnum].mtype.phys_info.mass = F1_0;
				vm_vec_zero(&Objects[objnum].mtype.phys_info.velocity);
			}
			continue;
		}

		if (Objects[i].type == OBJ_POWERUP)
		{
			if (Objects[i].id == POW_EXTRA_LIFE) 
			{
				Objects[i].id = POW_INVULNERABILITY;
				Objects[i].rtype.vclip_info.vclip_num = Powerup_info[Objects[i].id].vclip_num;
				Objects[i].rtype.vclip_info.frametime = Vclip[Objects[i].rtype.vclip_info.vclip_num].frame_time;
			}

			if ((multi_allow_powerup & multi_allow_powerup_mask[Objects[i].id]) != multi_allow_powerup_mask[Objects[i].id]) {
				Objects[i].id = POW_SHIELD_BOOST;
				Objects[i].rtype.vclip_info.vclip_num = Powerup_info[Objects[i].id].vclip_num;
                                Objects[i].rtype.vclip_info.frametime = Vclip[Objects[i].rtype.vclip_info.vclip_num].frame_time;
			}

			if (Game_mode & GM_MULTI_COOP)
				continue;

			if ((Objects[i].id >= POW_KEY_BLUE) && (Objects[i].id <= POW_KEY_GOLD))
			{
				Objects[i].id = POW_SHIELD_BOOST;
				Objects[i].rtype.vclip_info.vclip_num = Powerup_info[Objects[i].id].vclip_num;
				Objects[i].rtype.vclip_info.frametime = Vclip[Objects[i].rtype.vclip_info.vclip_num].frame_time;
			}

			if (Objects[i].id == POW_INVULNERABILITY) {
				if (inv_count >= 3) {
					Objects[i].id = POW_SHIELD_BOOST;
					Objects[i].rtype.vclip_info.vclip_num = Powerup_info[Objects[i].id].vclip_num;
					Objects[i].rtype.vclip_info.frametime = Vclip[Objects[i].rtype.vclip_info.vclip_num].frame_time;
				} else
					inv_count++;
			}

			if (Objects[i].id == POW_CLOAK) {
				if (cloak_count >= 3) {
					Objects[i].id = POW_SHIELD_BOOST;
					Objects[i].rtype.vclip_info.vclip_num = Powerup_info[Objects[i].id].vclip_num;
					Objects[i].rtype.vclip_info.frametime = Vclip[Objects[i].rtype.vclip_info.vclip_num].frame_time;
				} else
					cloak_count++;
			}
		}
	}
	
	multi_sort_kill_list();

	multi_show_player_list();

	ConsoleObject->control_type = CT_FLYING;

	reset_player_object();

	multi_got_pow_count = 0;

	// send player powerups (assumes sync already send)
	if ((Game_mode & GM_NETWORK) &&
	    Netgame.protocol_version == MULTI_PROTO_D1X_VER &&
	    !Network_rejoined)
		multi_send_player_powerup_count();

}

void multi_set_robot_ai(void)
{
	// Go through the objects array looking for robots and setting
	// them to certain supported types of NET AI behavior.

//	int i;
//
//	for (i = 0; i <= Highest_object_index; i++)
//	{
//		if (Objects[i].type == OBJ_ROBOT) {
//			Objects[i].ai_info.REMOTE_OWNER = -1;
//			if (Objects[i].ai_info.behavior == AIB_STATION)
//				Objects[i].ai_info.behavior = AIB_NORMAL;
//		}
//	}
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
		else if ( (objp->type!=OBJ_NONE) && (objp->type!=OBJ_PLAYER) && (objp->type!=OBJ_POWERUP) && (objp->type!=OBJ_CNTRLCEN) && (objp->type!=OBJ_HOSTAGE) )
			obj_delete(i);
		objp++;
	}

	return nnp;
}

int
network_i_am_master(void)
{
	// I am the lowest numbered player in this game?

	int i;

	if (!(Game_mode & GM_NETWORK))
		return (Player_num == 0);

	for (i = 0; i < Player_num; i++)
		if (Players[i].connected)
			return 0;
	return 1;
}
#include "u_mem.h"
void change_playernum_to( int new_Player_num )	
{
// 	if (Player_num > -1)
// 		memcpy( Players[new_Player_num].callsign, Players[Player_num].callsign, CALLSIGN_LEN+1 );
	if (Player_num > -1)
	{
		char *buf;
		MALLOC(buf,char,CALLSIGN_LEN+1);
		memcpy( buf, Players[Player_num].callsign, CALLSIGN_LEN+1 );
		strcpy(Players[new_Player_num].callsign,buf);
		d_free(buf);
	}

	Player_num = new_Player_num;
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
#endif
