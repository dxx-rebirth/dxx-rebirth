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
 * Defines and exported variables for multi.c
 *
 */

#ifndef _MULTI_H
#define _MULTI_H

#include "gameseq.h"
#include "piggy.h"
#include "newmenu.h"
#include "powerup.h"

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

#ifdef __cplusplus

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
#define MULTI_PROTO_VERSION 11
// PROTOCOL VARIABLES AND DEFINES - END

// limits for Packets (i.e. positional updates) per sec
#define MIN_PPS 5
#define MAX_PPS 40

#define MAX_MESSAGE_LEN 35

#define define_multiplayer_command(NAME,SIZE)	NAME,

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
#define for_each_multiplayer_command(BEFORE,VALUE,AFTER)	\
	BEFORE	\
	VALUE(MULTI_POSITION              , 25)	\
	VALUE(MULTI_REAPPEAR              , 4)	\
	VALUE(MULTI_FIRE                  , 18)	\
	VALUE(MULTI_FIRE_TRACK            , 21)	\
	VALUE(MULTI_FIRE_BOMB             , 20)	\
	VALUE(MULTI_KILL                  , 5)	\
	VALUE(MULTI_REMOVE_OBJECT         , 4)	\
	VALUE(MULTI_MESSAGE               , 37)	/* (MAX_MESSAGE_LENGTH = 40) */	\
	VALUE(MULTI_QUIT                  , 2)	\
	VALUE(MULTI_PLAY_SOUND            , 4)	\
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
	VALUE(MULTI_PLAYER_EXPLODE       , DXX_MP_SIZE_PLAYER_RELATED)	\
	VALUE(MULTI_BEGIN_SYNC           , DXX_MP_SIZE_BEGIN_SYNC)	\
	VALUE(MULTI_DOOR_OPEN            , DXX_MP_SIZE_DOOR_OPEN)	\
	VALUE(MULTI_PLAYER_DROP          , DXX_MP_SIZE_PLAYER_RELATED)	\
	VALUE(MULTI_ROBOT_EXPLODE        , DXX_MP_SIZE_ROBOT_EXPLODE)	\
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
	D2X_MP_COMMANDS(BEFORE,VALUE,AFTER)	\
	AFTER

#if defined(DXX_BUILD_DESCENT_I)
#define MAX_NET_CREATE_OBJECTS 20
#define MAX_MULTI_MESSAGE_LEN  90 //didn't change it, just moved it up
#define DXX_MP_SIZE_PLAYER_RELATED	57
#define DXX_MP_SIZE_BEGIN_SYNC	37
#define DXX_MP_SIZE_DOOR_OPEN	4
#define DXX_MP_SIZE_ROBOT_EXPLODE	8
#define D2X_MP_COMMANDS(BEFORE,VALUE,AFTER)
#elif defined(DXX_BUILD_DESCENT_II)
#define MAX_NET_CREATE_OBJECTS  40
#define MAX_MULTI_MESSAGE_LEN   120
#define DXX_MP_SIZE_PLAYER_RELATED	(97+9)
#define DXX_MP_SIZE_BEGIN_SYNC	41
#define DXX_MP_SIZE_DOOR_OPEN	5
#define DXX_MP_SIZE_ROBOT_EXPLODE	9
#define D2X_MP_COMMANDS(BEFORE,VALUE,AFTER)	\
	VALUE(MULTI_MARKER               , 55)	\
	VALUE(MULTI_DROP_WEAPON          , 12)	\
	VALUE(MULTI_GUIDED               , 3+sizeof(shortpos))	\
	VALUE(MULTI_STOLEN_ITEMS         , 11)	\
	VALUE(MULTI_WALL_STATUS          , 6)	/* send to new players */	\
	VALUE(MULTI_SEISMIC              , 9)	\
	VALUE(MULTI_LIGHT                , 18)	\
	VALUE(MULTI_START_TRIGGER        , 2)	\
	VALUE(MULTI_FLAGS                , 6)	\
	VALUE(MULTI_DROP_BLOB            , 2)	\
	VALUE(MULTI_SOUND_FUNCTION       , 4)	\
	VALUE(MULTI_CAPTURE_BONUS        , 2)	\
	VALUE(MULTI_GOT_FLAG             , 2)	\
	VALUE(MULTI_DROP_FLAG            , 12)	\
	VALUE(MULTI_FINISH_GAME          , 2)	\
	VALUE(MULTI_MODEM_PING           , 1)	\
	VALUE(MULTI_MODEM_PING_RETURN    , 1)	\
	VALUE(MULTI_ORB_BONUS            , 3)	\
	VALUE(MULTI_GOT_ORB              , 2)	\
	VALUE(MULTI_EFFECT_BLOWUP        , 17)	\

#endif
for_each_multiplayer_command(enum {, define_multiplayer_command, });
#endif

#define NETGAME_ANARCHY         0
#define NETGAME_TEAM_ANARCHY    1
#define NETGAME_ROBOT_ANARCHY   2
#define NETGAME_COOPERATIVE     3
#if defined(DXX_BUILD_DESCENT_II)
#define NETGAME_CAPTURE_FLAG    4
#define NETGAME_HOARD           5
#define NETGAME_TEAM_HOARD      6
#endif
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
#if defined(DXX_BUILD_DESCENT_II)
#define CONNECT_KMATRIX_WAITING     7 // Like CONNECT_WAITING but used especially in kmatrix.c to seperate "escaped" and "waiting"
#endif

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

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
#define for_each_netflag_value(VALUE)	\
	VALUE(NETFLAG_DOLASER, "Laser upgrade")	\
	VALUE(NETFLAG_DOQUAD, "Quad Lasers")	\
	VALUE(NETFLAG_DOVULCAN, "Vulcan cannon")	\
	VALUE(NETFLAG_DOSPREAD, "Spreadfire cannon")	\
	VALUE(NETFLAG_DOPLASMA, "Plasma cannon")	\
	VALUE(NETFLAG_DOFUSION, "Fusion cannon")	\
	VALUE(NETFLAG_DOHOMING, "Homing Missiles")	\
	VALUE(NETFLAG_DOPROXIM, "Proximity Bombs")	\
	VALUE(NETFLAG_DOSMART, "Smart Missiles")	\
	VALUE(NETFLAG_DOMEGA, "Mega Missiles")	\
	VALUE(NETFLAG_DOCLOAK, "Cloaking")	\
	VALUE(NETFLAG_DOINVUL, "Invulnerability")	\
	D2X_MP_NETFLAGS(VALUE)	\

#define MULTI_GAME_TYPE_COUNT	8
#if defined(DXX_BUILD_DESCENT_I)
#define MULTI_GAME_NAME_LENGTH	13
#define MULTI_ALLOW_POWERUP_TEXT_LENGTH	18
#define MULTI_ALLOW_POWERUP_MAX 12
#define D2X_MP_NETFLAGS(VALUE)
#elif defined(DXX_BUILD_DESCENT_II)
#define MULTI_GAME_NAME_LENGTH	17
#define MULTI_ALLOW_POWERUP_TEXT_LENGTH	21
#define MULTI_ALLOW_POWERUP_MAX 26
#define D2X_MP_NETFLAGS(VALUE)	\
	VALUE(NETFLAG_DOSUPERLASER, "Super lasers")	\
	VALUE(NETFLAG_DOGAUSS, "Gauss cannon")	\
	VALUE(NETFLAG_DOHELIX, "Helix cannon")	\
	VALUE(NETFLAG_DOPHOENIX, "Phoenix cannon")	\
	VALUE(NETFLAG_DOOMEGA, "Omega cannon")	\
	VALUE(NETFLAG_DOFLASH, "Flash Missiles")	\
	VALUE(NETFLAG_DOGUIDED, "Guided Missiles")	\
	VALUE(NETFLAG_DOSMARTMINE, "Smart Mines")	\
	VALUE(NETFLAG_DOMERCURY, "Mercury Missiles")	\
	VALUE(NETFLAG_DOSHAKER, "EarthShaker Missiles")	\
	VALUE(NETFLAG_DOAFTERBURNER, "Afterburners")	\
	VALUE(NETFLAG_DOAMMORACK, "Ammo rack")	\
	VALUE(NETFLAG_DOCONVERTER, "Energy Converter")	\
	VALUE(NETFLAG_DOHEADLIGHT, "Headlight")	\

#endif
#endif

#define define_netflag_bit_enum(NAME,STR)	BIT_##NAME,
#define define_netflag_bit_mask(NAME,STR)	NAME = (1 << BIT_##NAME),
#define define_netflag_powerup_mask(NAME,STR)	| (NAME)
enum { for_each_netflag_value(define_netflag_bit_enum) };
// Bitmask for netgame_info->AllowedItems to set allowed items in Netgame
enum { for_each_netflag_value(define_netflag_bit_mask) };
enum { NETFLAG_DOPOWERUP = 0 for_each_netflag_value(define_netflag_powerup_mask) };

enum msgsend_state_t {
	msgsend_none,
	msgsend_typing,
	msgsend_automap,
};

extern const char multi_allow_powerup_text[MULTI_ALLOW_POWERUP_MAX][MULTI_ALLOW_POWERUP_TEXT_LENGTH];
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

void multi_send_fire(int laser_gun, int laser_level, int laser_flags, int laser_fired, short laser_track, int is_bomb_objnum);
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
void multi_send_score(void);
void multi_send_trigger(int trigger);
void multi_send_hostage_door_status(int wallnum);
#if defined(DXX_BUILD_DESCENT_II)
extern char Multi_is_guided;
void multi_send_flags(char);
void multi_send_drop_weapon (int objnum,int seed);
void multi_send_drop_marker (int player,vms_vector position,char messagenum,char text[]);
void multi_send_markers();
void multi_send_guided_info (object *miss,char);
void multi_send_orb_bonus( char pnum );
void multi_send_got_orb( char pnum );
void multi_send_effect_blowup(short segnum, int side, vms_vector *pnt);
#endif
void multi_add_lifetime_kills(void);
void multi_send_bounty( void );

void multi_endlevel_score(void);
void multi_consistency_error(int reset);
void multi_prep_level(void);
int multi_level_sync(void);
int multi_endlevel(int *secret);
int multi_endlevel_poll1(newmenu *menu, d_event *event, unused_newmenu_userdata_t *userdata);
int multi_endlevel_poll2( newmenu *menu, d_event *event, unused_newmenu_userdata_t *userdata );
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
void multi_send_data(const ubyte *buf, int len, int priority);
int get_team(int pnum);
int multi_maybe_disable_friendly_fire(object *killer);
void multi_initiate_save_game();
void multi_initiate_restore_game();
void multi_disconnect_player(int pnum);
void multi_object_to_object_rw(object *obj, object_rw *obj_rw);
void multi_object_rw_to_object(object_rw *obj_rw, object *obj);

#if defined(DXX_BUILD_DESCENT_I)
static inline void multi_send_got_flag (char a) { (void)a; }
#elif defined(DXX_BUILD_DESCENT_II)
void multi_send_got_flag (char);
#endif

// Exported variables

extern int Network_status;
extern grs_bitmap Orb_icons[2];

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

// Which player 'owns' each local object for network purposes
extern sbyte object_owner[MAX_OBJECTS];

extern int multi_quit_game;

extern msgsend_state_t multi_sending_message[MAX_PLAYERS];
extern int multi_defining_message;
extern int multi_message_input_sub(int key);
extern void multi_send_message_start();
void multi_send_msgsend_state(msgsend_state_t state);
extern int multi_powerup_is_4pack(int);

extern int PhallicLimit,PhallicMan;
extern int Bounty_target;

extern bitmap_index multi_player_textures[MAX_PLAYERS][N_PLAYER_SHIP_TEXTURES];

extern const char RankStrings[10][14];
extern char PowerupsInMine[MAX_POWERUP_TYPES],
	MaxPowerupsAllowed[MAX_POWERUP_TYPES];

// Globals for protocol-bound Refuse-functions
extern char RefuseThisPlayer,WaitForRefuseAnswer,RefuseTeam,RefusePlayerName[12];
extern fix64 RefuseTimeLimit;
#define REFUSE_INTERVAL (F1_0*8)

typedef struct {
	unsigned closed : 1;
	unsigned : 1;
	unsigned show_on_map : 1;
	/*
	 * These #define are written to .NGP files and to the network.
	 * Changing them breaks ABI compatibility.
	 * The bit flags need not match in value, and are converted below in
	 * pack_game_flags / unpack_game_flags.
	 */
#define NETGAME_FLAG_CLOSED             1
#define NETGAME_FLAG_SHOW_MAP           4
#if defined(DXX_BUILD_DESCENT_II)
	unsigned hoard : 1;
	unsigned team_hoard : 1;
	unsigned endlevel : 1;
	unsigned forming : 1;
#define NETGAME_FLAG_HOARD              8
#define NETGAME_FLAG_TEAM_HOARD         16
#define NETGAME_FLAG_REALLY_ENDLEVEL    32
#define NETGAME_FLAG_REALLY_FORMING     64
#endif
} __pack__ bit_game_flags;

typedef struct
{
	unsigned char value;
} packed_game_flags;

static inline bit_game_flags unpack_game_flags(const packed_game_flags *p)
{
	bit_game_flags flags;
	flags.closed = !!(p->value & NETGAME_FLAG_CLOSED);
	flags.show_on_map = !!(p->value & NETGAME_FLAG_SHOW_MAP);
#if defined(DXX_BUILD_DESCENT_II)
	flags.hoard = !!(p->value & NETGAME_FLAG_HOARD);
	flags.team_hoard = !!(p->value & NETGAME_FLAG_TEAM_HOARD);
	flags.endlevel = !!(p->value & NETGAME_FLAG_REALLY_ENDLEVEL);
	flags.forming = !!(p->value & NETGAME_FLAG_REALLY_FORMING);
#endif
	return flags;
}

static inline packed_game_flags pack_game_flags(const bit_game_flags *flags)
{
	packed_game_flags p;
	p.value =
		(flags->closed ? NETGAME_FLAG_CLOSED : 0) |
		(flags->show_on_map ? NETGAME_FLAG_SHOW_MAP : 0) |
#if defined(DXX_BUILD_DESCENT_II)
		(flags->hoard ? NETGAME_FLAG_HOARD : 0) |
		(flags->team_hoard ? NETGAME_FLAG_TEAM_HOARD : 0) |
		(flags->endlevel ? NETGAME_FLAG_REALLY_ENDLEVEL : 0) |
		(flags->forming ? NETGAME_FLAG_REALLY_FORMING : 0) |
#endif
		0;
	return p;
}

#define NETGAME_NAME_LEN                15

extern struct netgame_info Netgame;

int multi_i_am_master(void);
int multi_who_is_master(void);
void change_playernum_to(int new_pnum);

// Multiplayer powerup capping
extern void multi_powcap_count_powerups_in_mine(void);
extern void multi_powcap_cap_objects();
extern void multi_do_powcap_update();
extern void multi_send_powcap_update();
extern void multi_send_kill_goal_counts();
void multi_check_for_killgoal_winner();
#if defined(DXX_BUILD_DESCENT_II)
extern void multi_send_stolen_items();
extern void multi_send_trigger_specific(char pnum,char trig);
extern void multi_send_door_open_specific(int pnum,int segnum, int side,ubyte flag);
extern void multi_send_wall_status_specific (int pnum,int wallnum,ubyte type,ubyte flags,ubyte state);
extern void multi_send_light_specific (int pnum,int segnum,ubyte val);
void multi_send_capture_bonus (char pnum);
int multi_all_players_alive();
void multi_send_seismic (fix64,fix64);
void multi_send_drop_blobs(char);
void multi_send_sound_function (char,char);
void DropFlag();
int multi_powerup_is_allowed (int);
void multi_send_finish_game ();
void init_hoard_data();
void multi_apply_goal_textures();

int HoardEquipped();
#ifdef EDITOR
void save_hoard_data(void);
#endif
#endif

//how to encode missiles & flares in weapon packets
#define MISSILE_ADJUST  100
#define FLARE_ADJUST    127

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
	bit_game_flags game_flag;
	ubyte   					team_vector;
	u_int32_t					AllowedItems;
	/*
	 * Only used in Descent II, but defined in both for historical
	 * reasons
	 */
	short						Allow_marker_view;
	short						AlwaysLighting;
	/* End Descent II */
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
	ubyte						PacketLossPrevention;
	ubyte						NoFriendlyFire;
#ifdef USE_TRACKER
	ubyte						Tracker;
#endif
} __pack__ netgame_info;

#endif
#endif /* _MULTI_H */
