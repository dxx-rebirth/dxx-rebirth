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
 * Defines and exported variables for multi.c
 *
 */



#ifndef _MULTI_H
#define _MULTI_H

#include "gameseq.h"
#include "piggy.h"
#include "vers_id.h"
#include "powerup.h"
#include "newmenu.h"

// Need these for non network builds too -Chris
#define MAX_MESSAGE_LEN 35
#define SHAREWARE_MAX_MESSAGE_LEN 25
#define MAX_NUM_NET_PLAYERS 8 // How many simultaneous network players do we support?

#ifdef NETWORK

#ifdef _WIN32
	#include <winsock.h>
	#include <io.h>
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#include <arpa/inet.h>
	#include <unistd.h>
	#include <sys/time.h>
#endif

#ifdef IPv6
	#define _sockaddr sockaddr_in6
	#define _af AF_INET6
	#define _pf PF_INET6
#else
	#define _sockaddr sockaddr_in
	#define _af AF_INET
	#define _pf PF_INET
#endif


// PROTOCOL VARIABLES AND DEFINES
extern int multi_protocol; // set and determinate used protocol
#define MULTI_PROTO_IPX 1 // IPX-type protocol (IPX, KALI)
#define MULTI_PROTO_UDP 2 // UDP protocol

// What version of the multiplayer protocol is this?
// Protocol versions:
//   1 Descent Shareware
//   2 Descent Registered/Commercial
//   3 Descent II Shareware
//   4 Descent II Commercial
#define MULTI_PROTO_VERSION	2
// these two defines should become obsolete due to strict version checking in UDP
#define MULTI_PROTO_D1X_VER	7  // Increment everytime we change networking features - based on ubyte, must never be > 255
#define MULTI_PROTO_D1X_MINOR	1 //Incrementing this seems the only way possible.  Still stays backwards compitible.
// PROTOCOL VARIABLES AND DEFINES - END


#define MULTI_POSITION			0
#define MULTI_REAPPEAR   		1
#define MULTI_FIRE                      2
#define MULTI_KILL                      3
#define MULTI_REMOVE_OBJECT             4
#define MULTI_PLAYER_EXPLODE            5
#define MULTI_MESSAGE			6
#define MULTI_QUIT                      7
#define MULTI_PLAY_SOUND		8
#define MULTI_BEGIN_SYNC		9
#define MULTI_CONTROLCEN		10
#define MULTI_ROBOT_CLAIM		11
#define MULTI_END_SYNC			12
#define MULTI_CLOAK                     13
#define MULTI_ENDLEVEL_START            14
#define MULTI_DOOR_OPEN			15
#define MULTI_CREATE_EXPLOSION          16
#define MULTI_CONTROLCEN_FIRE           17
#define MULTI_PLAYER_DROP		18
#define MULTI_CREATE_POWERUP            19
#define MULTI_CONSISTENCY		20
#define MULTI_DECLOAK			21
#define MULTI_MENU_CHOICE		22
#ifndef SHAREWARE
#define MULTI_ROBOT_POSITION            23
#define MULTI_ROBOT_EXPLODE             24
#define MULTI_ROBOT_RELEASE             25
#define MULTI_ROBOT_FIRE                26
#define MULTI_SCORE                     27
#define MULTI_CREATE_ROBOT		28
#define MULTI_TRIGGER			29
#define MULTI_BOSS_ACTIONS		30
#define MULTI_CREATE_ROBOT_POWERUPS	31
#define MULTI_HOSTAGE_DOOR		32
#endif

#define MULTI_SAVE_GAME			33 // obsolete
#define MULTI_RESTORE_GAME		34 // obsolete

#define MULTI_REQ_PLAYER		35		// Someone requests my player structure
#define MULTI_SEND_PLAYER		36		// Sending someone my player structure

#define MULTI_PLAYER_POWERUP_COUNT	37
#define MULTI_START_POWERUP_COUNT	38

#ifndef SHAREWARE
#define MULTI_MAX_TYPE                  38
#else
#define MULTI_MAX_TYPE                  25 //22
#endif

#define MAX_MULTI_MESSAGE_LEN  90 //didn't change it, just moved it up

#ifdef SHAREWARE
#define MAX_NET_CREATE_OBJECTS 19 
#else
#define MAX_NET_CREATE_OBJECTS 20
#endif

#define MISSILE_ADJUST 6


#define NETSTAT_MENU                            0
#define NETSTAT_PLAYING				1
#define NETSTAT_BROWSING			2
#define NETSTAT_WAITING				3
#define NETSTAT_STARTING			4
#define NETSTAT_ENDLEVEL			5

#define CONNECT_DISCONNECTED                    0
#define CONNECT_PLAYING				1
#define CONNECT_WAITING				2
#define CONNECT_DIED_IN_MINE                    3
#define CONNECT_FOUND_SECRET                    4
#define CONNECT_ESCAPE_TUNNEL                   5
#define CONNECT_END_MENU			6

// Bitmask for netgame_info->AllowedItems to set allowed items in Netgame
#define NETFLAG_DOLASER   1     //  0x0000001
#define NETFLAG_DOQUAD    2     //  0x0000002
#define NETFLAG_DOVULCAN  4     //  0x0000004
#define NETFLAG_DOSPREAD  8     //  0x0000008
#define NETFLAG_DOPLASMA  16    //  0x0000010
#define NETFLAG_DOFUSION  32    //  0x0000020
#define NETFLAG_DOHOMING  64    //  0x0000040
#define NETFLAG_DOSMART   128   //  0x0000080
#define NETFLAG_DOMEGA    256   //  0x0000100
#define NETFLAG_DOPROXIM  512   //  0x0000200
#define NETFLAG_DOCLOAK   1024  //  0x0000400
#define NETFLAG_DOINVUL   2048  //  0x0000800
#define NETFLAG_DOPOWERUP 4095  //  0x0000fff mask for all powerup flags

#define MULTI_ALLOW_POWERUP_MAX 12
int multi_allow_powerup_mask[MAX_POWERUP_TYPES];
extern char *multi_allow_powerup_text[MULTI_ALLOW_POWERUP_MAX];

// Exported functions

int objnum_remote_to_local(int remote_obj, int owner);
int objnum_local_to_remote(int local_obj, sbyte *owner);
void map_objnum_local_to_remote(int local, int remote, int owner);
void map_objnum_local_to_local(int objnum);
int multi_objnum_is_past(int objnum);
void multi_do_ping_frame();

void multi_init_objects(void);
void multi_show_player_list(void);
void multi_do_protocol_frame(int force, int listen);
void multi_do_frame(void);

void multi_send_fire(int pl);
void multi_send_destroy_controlcen(int objnum, int player);
void multi_send_endlevel_start(int);
void multi_send_player_explode(char type);
void multi_send_message(void);
void multi_send_position(int objnum);
void multi_send_reappear();
void multi_send_kill(int objnum);
void multi_send_remobj(int objnum);
void multi_send_quit(int why);
void multi_send_door_open(int segnum, int side);
void multi_send_create_explosion(int player_num);
void multi_send_controlcen_fire(vms_vector *to_target, int gun_num, int objnum);
void multi_send_cloak(void);
void multi_send_decloak(void);
void multi_send_create_powerup(int powerup_type, int segnum, int objnum, vms_vector *pos);
void multi_send_play_sound(int sound_num, fix volume);
void multi_send_audio_taunt(int taunt_num);
void multi_send_score(void);
void multi_send_trigger(int trigger);
void multi_send_hostage_door_status(int wallnum);
void multi_send_netplayer_stats_request(ubyte player_num);
void multi_send_player_powerup_count();
void multi_send_start_powerup_count();

void multi_endlevel_score(void);
void multi_prep_level(void);
int multi_level_sync(void);
int multi_endlevel(int *secret);
void multi_endlevel_poll2( int nitems, struct newmenu_item * menus, int * key, int citem );
int multi_menu_poll(void);
void multi_leave_game(void);
void multi_process_data(char *dat, int len);
void multi_process_bigdata(char *buf, int len);		
void multi_do_death(int objnum);
void multi_send_message_dialog(void);
int multi_delete_extra_objects(void);
void multi_make_ghost_player(int objnum);
void multi_make_player_ghost(int objnum);
void multi_define_macro(int key);
void multi_send_macro(int key);
int multi_get_kill_list(int *plist);
void multi_new_game(void);
void multi_sort_kill_list(void);
void multi_reset_stuff(void);
void multi_send_data(unsigned char *buf, int len, int repeat);
int get_team(int pnum);


// Exported variables

extern int PacketUrgent;
extern int Network_active;
extern int Network_status;
extern int Network_laser_gun;
extern int Network_laser_fired;
extern int Network_laser_level;
extern int Network_laser_flags;

// IMPORTANT: These variables needed for player rejoining done by protocol-specific code
extern int Network_send_objects;
extern int Network_send_objnum;
extern int Network_rejoined;
extern int Network_new_game;

extern int message_length[MULTI_MAX_TYPE+1];

extern unsigned char multibuf[MAX_MULTI_MESSAGE_LEN+4];
extern short Network_laser_track;

extern int who_killed_controlcen;

extern int Net_create_objnums[MAX_NET_CREATE_OBJECTS];
extern int Net_create_loc;

extern short kill_matrix[MAX_NUM_NET_PLAYERS][MAX_NUM_NET_PLAYERS];
extern short team_kills[2];

extern int multi_goto_secret;

extern ushort my_segments_checksum;

//do we draw the kill list on the HUD?
extern int Show_kill_list;
extern int Show_reticle_name;
extern fix Show_kill_list_timer;

// Used to send network messages

extern char	Network_message[MAX_MESSAGE_LEN];
extern int Network_message_reciever;

// Used to map network to local object numbers

extern short remote_to_local[MAX_NUM_NET_PLAYERS][MAX_OBJECTS];  // Network object num for each 
extern short local_to_remote[MAX_OBJECTS];   // Local object num for each network objnum
extern sbyte object_owner[MAX_OBJECTS]; // Who 'owns' each local object for network purposes

extern int multi_in_menu; // Flag to tell if we're executing GameLoop from within a newmenu.
extern int multi_leave_menu;
extern int multi_quit_game;

extern int multi_sending_message;
extern int multi_defining_message;
extern void multi_message_input_sub(int key);
extern void multi_send_message_start();
extern void multi_message_feedback();

extern int control_invul_time;

#define N_PLAYER_SHIP_TEXTURES 6

extern bitmap_index multi_player_textures[MAX_NUM_NET_PLAYERS][N_PLAYER_SHIP_TEXTURES];

#define NETGAME_FLAG_CLOSED 	1
#define NETGAME_FLAG_SHOW_ID	2
#define NETGAME_FLAG_SHOW_MAP 4

#define NETGAME_NAME_LEN				15

#define NETPLAYER_ORIG_SIZE	22
#define NETPLAYER_D1X_SIZE	22 /* D1X version removes last char from callsign */

int multi_i_am_master(void);
int multi_who_is_master(void);
void change_playernum_to(int new_pnum);

extern uint multi_allow_powerup;

extern struct netgame_info Netgame;

/*
 * The Network Players structure
 * Contains both IPX- and UDP-specific data with designated prefixes and general player-related data.
 * Note that not all of these infos will be sent to other users - some are used and/or set locally, only.
 * Even if some variables are not UDP-specific, they might be used in IPX as well to maintain backwards compability.
 */
typedef struct netplayer_info
{
	union
	{
		struct
		{
			ubyte				server[4];
			ubyte				node[6];
			ushort				socket;
		} ipx;
		struct
		{
			struct _sockaddr	addr; // IP address of this peer
			int					valid; // 1 = client connected / 2 = client ready for handshaking / 3 = client done with handshake and fully joined / 0 between clients = no connection -> relay
			fix					timestamp; // time of received packet - used for timeout
			char				hs_list[MAX_PLAYERS+4]; // list to store all handshake results for this player from already connected clients
			int					hstimeout; // counts the number of tries the client tried to connect - if reached 10, client put to relay if allowed
			int					relay; // relay packets by/to this clients over host
		} udp;
	} protocol;	

	char						callsign[CALLSIGN_LEN+1];
	sbyte						connected;
	fix							ping;
	fix							LastPacketTime;
} __pack__ netplayer_info;

/*
 * The Network Game structure
 * Contains both IPX- and UDP-specific data with designated prefixes and general game-related data.
 * Note that not all of these infos will be sent to clients - some are used and/or set locally, only.
 * Even if some variables are not UDP-specific, they might be used in IPX as well to maintain backwards compability.
 */
typedef struct netgame_info {

	union
	{
		struct
		{
			ubyte				Game_pkt_type;
			ubyte   			protocol_version;
		} ipx;
		struct
		{
			struct _sockaddr	addr; // IP address of this netgame's host
			int					program_iver; // IVER of program for version checking
		} udp;
	} protocol;	

	struct netplayer_info 		players[MAX_PLAYERS+4];
	char    					game_name[NETGAME_NAME_LEN+1];
	char    					mission_title[MISSION_NAME_LEN+1];
	char    					mission_name[9];
	int     					levelnum;
	ubyte   					gamemode;
	ubyte   					RefusePlayers;
	ubyte   					difficulty;
	ubyte   					game_status;
	ubyte   					numplayers;
	ubyte   					max_numplayers;
	ubyte   					numconnected; // FIXME!!!
	ubyte   					game_flags;
	ubyte   					team_vector;
	u_int32_t					AllowedItems;
	short						Allow_marker_view:1; // FIXME!!!
	short						AlwaysLighting:1; // FIXME!!!
	short						ShowAllNames:1; // FIXME!!!
	short						BrightPlayers:1; // FIXME!!!
	short						InvulAppear:1; // FIXME!!!
	char						team_name[2][CALLSIGN_LEN+1];
	int							locations[MAX_PLAYERS];
	short						kills[MAX_PLAYERS][MAX_PLAYERS];
	ushort						segments_checksum;
	short						team_kills[2];
	short						killed[MAX_PLAYERS];
	short						player_kills[MAX_PLAYERS];
	int							KillGoal; // FIXME!!!
	fix							PlayTimeAllowed; // FIXME!!!
	fix							level_time;
	int							control_invul_time;
	int							monitor_vector;
	int							player_score[MAX_PLAYERS];
	ubyte						player_flags[MAX_PLAYERS];
	short						PacketsPerSec;
	ubyte						PacketLossPrevention;
} __pack__ netgame_info;

#endif

#endif
