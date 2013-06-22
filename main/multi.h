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
#include "powerup.h"
#include "newmenu.h"
// Need these for non network builds too -Chris
#define MAX_MESSAGE_LEN 35

#ifdef USE_UDP
#ifdef _WIN32
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0501 // for get/freeaddrinfo()
#include <winsock2.h>
#include <ws2tcpip.h>
#include <io.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>
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
#endif


// PROTOCOL VARIABLES AND DEFINES
extern int multi_protocol; // set and determinate used protocol
#define MULTI_PROTO_UDP 1 // UDP protocol

// What version of the multiplayer protocol is this? Increment each time something drastic changes in Multiplayer without the version number changes. Can be reset to 0 each time the version of the game changes
#define MULTI_PROTO_VERSION 6
// PROTOCOL VARIABLES AND DEFINES - END


#define define_multiplayer_command(NAME,SIZE)	NAME,

#define for_each_multiplayer_command(BEFORE,VALUE,AFTER)	\
	BEFORE	\
	VALUE(MULTI_POSITION             , 25)	\
	VALUE(MULTI_REAPPEAR             , 4)	\
	VALUE(MULTI_FIRE                 , 8)	\
	VALUE(MULTI_KILL                 , 5)	\
	VALUE(MULTI_REMOVE_OBJECT        , 4)	\
	VALUE(MULTI_MESSAGE              , 37)	/* (MAX_MESSAGE_LENGTH = 40) */	\
	VALUE(MULTI_QUIT                 , 2)	\
	VALUE(MULTI_PLAY_SOUND           , 4)	\
	VALUE(MULTI_CONTROLCEN           , 4)	\
	VALUE(MULTI_ROBOT_CLAIM          , 5)	\
	VALUE(MULTI_END_SYNC             , 4)	\
	VALUE(MULTI_CLOAK                , 2)	\
	VALUE(MULTI_ENDLEVEL_START       , 3)	\
	VALUE(MULTI_CREATE_EXPLOSION     , 2)	\
	VALUE(MULTI_CONTROLCEN_FIRE      , 16)	\
	VALUE(MULTI_CREATE_POWERUP       , 19)	\
	VALUE(MULTI_DECLOAK              , 2)	\
	VALUE(MULTI_MENU_CHOICE          , 2)	\
	VALUE(MULTI_ROBOT_POSITION       , 5+sizeof(shortpos))	\
	VALUE(MULTI_PLAYER_EXPLODE       , 57)	\
	VALUE(MULTI_BEGIN_SYNC           , 37)	\
	VALUE(MULTI_DOOR_OPEN            , 4)	\
	VALUE(MULTI_PLAYER_DROP          , 57)	\
	VALUE(MULTI_ROBOT_EXPLODE        , 8)	\
	VALUE(MULTI_ROBOT_RELEASE        , 5)	\
	VALUE(MULTI_ROBOT_FIRE           , 18)	\
	VALUE(MULTI_SCORE                , 6)	\
	VALUE(MULTI_CREATE_ROBOT         , 6)	\
	VALUE(MULTI_TRIGGER              , 3)	\
	VALUE(MULTI_BOSS_ACTIONS         , 10)	\
	VALUE(MULTI_CREATE_ROBOT_POWERUPS, 27)	\
	VALUE(MULTI_HOSTAGE_DOOR         , 7)	\
	VALUE(MULTI_SAVE_GAME            , 2+24)	/* (ubyte slot, uint id, char name[20]) */	\
	VALUE(MULTI_RESTORE_GAME         , 2+4)	/* (ubyte slot, uint id) */	\
	VALUE(MULTI_HEARTBEAT            , 5)	\
	VALUE(MULTI_KILLGOALS            , 9)	\
	VALUE(MULTI_POWCAP_UPDATE        , MAX_POWERUP_TYPES+1)	\
	VALUE(MULTI_DO_BOUNTY            , 2)	\
	VALUE(MULTI_TYPING_STATE         , 3)	\
	VALUE(MULTI_GMODE_UPDATE         , 3)	\
	VALUE(MULTI_KILL_HOST            , 7)	\
	VALUE(MULTI_KILL_CLIENT          , 5)	\
	VALUE(MULTI_RANK                 , 3)	\
	AFTER
for_each_multiplayer_command(enum {, define_multiplayer_command, });

#define MAX_MULTI_MESSAGE_LEN  90 //didn't change it, just moved it up

#define MAX_NET_CREATE_OBJECTS 20

#define MISSILE_ADJUST 6

#define NETGAME_ANARCHY         0
#define NETGAME_TEAM_ANARCHY    1
#define NETGAME_ROBOT_ANARCHY   2
#define NETGAME_COOPERATIVE     3
#define NETGAME_BOUNTY		7

#define NETSTAT_MENU                0
#define NETSTAT_PLAYING             1
#define NETSTAT_BROWSING            2
#define NETSTAT_WAITING             3
#define NETSTAT_STARTING            4
#define NETSTAT_ENDLEVEL            5

#define CONNECT_DISCONNECTED        0
#define CONNECT_PLAYING             1
#define CONNECT_WAITING             2
#define CONNECT_DIED_IN_MINE        3
#define CONNECT_FOUND_SECRET        4
#define CONNECT_ESCAPE_TUNNEL       5
#define CONNECT_END_MENU            6

// reasons for a packet with type PID_DUMP
#define DUMP_CLOSED     0 // no new players allowed after game started
#define DUMP_FULL       1 // player cound maxed out
#define DUMP_ENDLEVEL   2
#define DUMP_DORK       3
#define DUMP_ABORTED    4
#define DUMP_CONNECTED  5 // never used
#define DUMP_LEVEL      6
#define DUMP_KICKED     7
#define DUMP_PKTTIMEOUT 8

#define for_each_netflag_value(VALUE)	\
	VALUE(NETFLAG_DOLASER)	\
	VALUE(NETFLAG_DOQUAD)	\
	VALUE(NETFLAG_DOVULCAN)	\
	VALUE(NETFLAG_DOSPREAD)	\
	VALUE(NETFLAG_DOPLASMA)	\
	VALUE(NETFLAG_DOFUSION)	\
	VALUE(NETFLAG_DOHOMING)	\
	VALUE(NETFLAG_DOPROXIM)	\
	VALUE(NETFLAG_DOSMART)	\
	VALUE(NETFLAG_DOMEGA)	\
	VALUE(NETFLAG_DOCLOAK)	\
	VALUE(NETFLAG_DOINVUL)	\

#define define_netflag_bit_enum(NAME)	BIT_##NAME,
#define define_netflag_bit_mask(NAME)	NAME = (1 << BIT_##NAME),
#define define_netflag_powerup_mask(NAME)	| (NAME)
enum { for_each_netflag_value(define_netflag_bit_enum) };
// Bitmask for netgame_info->AllowedItems to set allowed items in Netgame
enum { for_each_netflag_value(define_netflag_bit_mask) };
enum { NETFLAG_DOPOWERUP = 0 for_each_netflag_value(define_netflag_powerup_mask) };

#define MULTI_GAME_TYPE_COUNT	8
#define MULTI_GAME_NAME_LENGTH	13
#define MULTI_ALLOW_POWERUP_MAX 12
int multi_allow_powerup_mask[MAX_POWERUP_TYPES];
extern char *multi_allow_powerup_text[MULTI_ALLOW_POWERUP_MAX];
extern const char GMNames[MULTI_GAME_TYPE_COUNT][MULTI_GAME_NAME_LENGTH];
extern const char GMNamesShrt[MULTI_GAME_TYPE_COUNT][8];

// Exported functions

extern int GetMyNetRanking();
extern void ClipRank (ubyte *rank);
int objnum_remote_to_local(int remote_obj, int owner);
int objnum_local_to_remote(int local_obj, sbyte *owner);
void map_objnum_local_to_remote(int local, int remote, int owner);
void map_objnum_local_to_local(int objnum);
void reset_network_objects();
int multi_objnum_is_past(int objnum);
void multi_do_ping_frame();

void multi_init_objects(void);
void multi_show_player_list(void);
void multi_do_protocol_frame(int force, int listen);
void multi_do_frame(void);

void multi_send_fire(int laser_gun, int laser_level, int laser_flags, int laser_fired, short laser_track);
void multi_send_destroy_controlcen(int objnum, int player);
void multi_send_endlevel_start(int);
void multi_send_player_explode(char type);
void multi_send_message(void);
void multi_send_position(int objnum);
void multi_send_reappear();
void multi_send_kill(int objnum);
void multi_send_remobj(int objnum);
void multi_send_quit(int why);
void multi_send_door_open(int segnum, int side,ubyte flag);
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

void multi_send_bounty( void );
void multi_endlevel_score(void);
void multi_consistency_error(int reset);
void multi_prep_level(void);
int multi_level_sync(void);
int multi_endlevel(int *secret);
int multi_endlevel_poll1(newmenu *menu, d_event *event, void *userdata);
int multi_endlevel_poll2( newmenu *menu, d_event *event, void *userdata );
void multi_send_endlevel_packet();
void multi_leave_game(void);
void multi_process_data(const ubyte *dat, int len);
void multi_process_bigdata(const ubyte *buf, unsigned len);
void multi_do_death(int objnum);
void multi_send_message_dialog(void);
int multi_delete_extra_objects(void);
void multi_make_ghost_player(int objnum);
void multi_make_player_ghost(int objnum);
void multi_reset_player_object(object *objp);
void multi_define_macro(int key);
void multi_send_macro(int key);
int multi_get_kill_list(int *plist);
void multi_new_game(void);
void multi_sort_kill_list(void);
void multi_reset_stuff(void);
void multi_send_data(unsigned char *buf, int len, int priority);
int get_team(int pnum);
int multi_maybe_disable_friendly_fire(object *killer);
void multi_initiate_save_game();
void multi_initiate_restore_game();
void multi_disconnect_player(int pnum);
void multi_object_to_object_rw(object *obj, object_rw *obj_rw);
void multi_object_rw_to_object(object_rw *obj_rw, object *obj);

// Exported variables

extern int Network_status;

// IMPORTANT: These variables needed for player rejoining done by protocol-specific code
extern int Network_send_objects;
extern int Network_send_object_mode;
extern int Network_send_objnum;
extern int Network_rejoined;
extern int Network_sending_extras;
extern int VerifyPlayerJoined;
extern int Player_joining_extras;
extern int Network_player_added;

extern ubyte multibuf[MAX_MULTI_MESSAGE_LEN+4];

extern int who_killed_controlcen;

extern int Net_create_objnums[MAX_NET_CREATE_OBJECTS];
extern int Net_create_loc;

extern short kill_matrix[MAX_PLAYERS][MAX_PLAYERS];
extern short team_kills[2];

extern int multi_goto_secret;

extern ushort my_segments_checksum;

//do we draw the kill list on the HUD?
extern int Show_kill_list;
extern int Show_reticle_name;
extern fix Show_kill_list_timer;

// Used to send network messages

extern char Network_message[MAX_MESSAGE_LEN];
extern int Network_message_reciever;

// Used to map network to local object numbers

extern short remote_to_local[MAX_PLAYERS][MAX_OBJECTS];  // Network object num for each 
extern short local_to_remote[MAX_OBJECTS];   // Local object num for each network objnum
extern sbyte object_owner[MAX_OBJECTS]; // Who 'owns' each local object for network purposes

extern int multi_quit_game;

extern int multi_sending_message[MAX_PLAYERS];
extern int multi_defining_message;
extern int multi_message_input_sub(int key);
extern void multi_send_message_start();
extern int multi_powerup_is_4pack(int);
extern void multi_message_feedback();

extern int Bounty_target;

extern bitmap_index multi_player_textures[MAX_PLAYERS][N_PLAYER_SHIP_TEXTURES];

extern char *RankStrings[];

#define NETGAME_FLAG_CLOSED             1
#define NETGAME_FLAG_SHOW_ID            2
#define NETGAME_FLAG_SHOW_MAP           4

#define NETGAME_NAME_LEN                15

#define NETPLAYER_ORIG_SIZE	22
#define NETPLAYER_D1X_SIZE	22 /* D1X version removes last char from callsign */

int multi_i_am_master(void);
int multi_who_is_master(void);
void change_playernum_to(int new_pnum);

// Multiplayer powerup capping
extern void multi_powcap_count_powerups_in_mine(void);
extern void multi_powcap_cap_objects();
extern void multi_do_powcap_update();
extern void multi_send_powcap_update();
extern void multi_send_kill_goal_counts();

// Globals for protocol-bound Refuse-functions
extern char RefuseThisPlayer,WaitForRefuseAnswer,RefuseTeam,RefusePlayerName[12];
extern fix64 RefuseTimeLimit;
#define REFUSE_INTERVAL (F1_0*8)

extern struct netgame_info Netgame;

/*
 * The Network Players structure
 * Contains protocol-specific data with designated prefixes and general player-related data.
 * Note that not all of these infos will be sent to other users - some are used and/or set locally, only.
 */
typedef struct netplayer_info
{
#if defined(USE_UDP)
	union
	{
#ifdef USE_UDP
		struct
		{
			struct _sockaddr	addr; // IP address of this peer
			ubyte				isyou; // This flag is set true while sending info to tell player his designated (re)join position
		} udp;
#endif
	} protocol;	
#endif
	char						callsign[CALLSIGN_LEN+1];
	sbyte						connected;
	ubyte						rank;
	fix							ping;
	fix64							LastPacketTime;
} __pack__ netplayer_info;

/*
 * The Network Game structure
 * Contains protocol-specific data with designated prefixes and general game-related data.
 * Note that not all of these infos will be sent to clients - some are used and/or set locally, only.
 */
typedef struct netgame_info
{
#if defined(USE_UDP)
	union
	{
#ifdef USE_UDP
		struct
		{
			struct _sockaddr		addr; // IP address of this netgame's host
			short				program_iver[4]; // IVER of program for version checking
			sbyte				valid; // Status of Netgame info: -1 = Failed, Wrong version; 0 = No info, yet; 1 = Success
			fix				GameID;
		} udp;
#endif
	} protocol;	
#endif
	struct netplayer_info 				players[MAX_PLAYERS+4];
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
	ubyte   					numconnected;
	ubyte   					game_flags;
	ubyte   					team_vector;
	u_int32_t					AllowedItems;
	short						Allow_marker_view; // (unused in D1 - no markers in game)
	short						AlwaysLighting; // (unused in D1 - cannot destroy lights after all)
	short						ShowEnemyNames;
	short						BrightPlayers;
	short						InvulAppear;
	char						team_name[2][CALLSIGN_LEN+1];
	int						locations[MAX_PLAYERS];
	short						kills[MAX_PLAYERS][MAX_PLAYERS];
	ushort						segments_checksum;
	short						team_kills[2];
	short						killed[MAX_PLAYERS];
	short						player_kills[MAX_PLAYERS];
	int						KillGoal;
	fix						PlayTimeAllowed;
	fix						level_time;
	int						control_invul_time;
	int						monitor_vector;
	int						player_score[MAX_PLAYERS];
	ubyte						player_flags[MAX_PLAYERS];
	short						PacketsPerSec;
	ubyte						ShortPackets;
	ubyte						PacketLossPrevention;
	ubyte						NoFriendlyFire;
#ifdef USE_TRACKER
	ubyte						Tracker;
#endif
} __pack__ netgame_info;
#endif /* _MULTI_H */
