/* $Id: network.h,v 1.1.1.1 2006/03/17 19:56:24 zicodxx Exp $ */
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
 * Prototypes for network management functions.
 *
 */


#ifndef _NETWORK_H
#define _NETWORK_H

#include "gameseq.h"
#include "multi.h"
#include "newmenu.h"

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

#define NETGAMEIPX                  1
#define NETGAMETCP                  2

// defines and other things for appletalk/ipx games on mac

#define IPX_GAME        1
#define APPLETALK_GAME  2
#ifdef MACINTOSH
extern int Network_game_type;
#else
#define Network_game_type IPX_GAME
#endif

/* the following are the possible packet identificators.
 * they are stored in the "type" field of the packet structs.
 * they are offset 4 bytes from the beginning of the raw IPX data
 * because of the "driver's" ipx_packetnum (see linuxnet.c).
 */
#define PID_LITE_INFO       43 // 0x2B lite game info
#define PID_SEND_ALL_GAMEINFO 44 // 0x2C plz send more than lite only
#define PID_PLAYERSINFO     45 // 0x2D here's my name & personal data
#define PID_REQUEST         46 // 0x2E may i join, plz send sync
#define PID_SYNC            47 // 0x2F master says: enter mine now!
#define PID_PDATA           48 // 0x30
#define PID_ADDPLAYER       49

#define PID_DUMP            51 // 0x33 you can't join this game
#define PID_ENDLEVEL        52

#define PID_QUIT_JOINING    54
#define PID_OBJECT_DATA     55 // array of bots, players, powerups, ...
#define PID_GAME_LIST       56 // 0x38 give me the list of your games
#define PID_GAME_INFO       57 // 0x39 here's a game i've started
#define PID_PING_SEND       58
#define PID_PING_RETURN     59
#define PID_GAME_UPDATE     60 // inform about new player/team change
#define PID_ENDLEVEL_SHORT  61
#define PID_NAKED_PDATA     62
#define PID_GAME_PLAYERS    63
#define PID_NAMES_RETURN    64 // 0x40
// new packet types to get a little bit more information about the netgame so we can show up some rules/flags - uses netgame_info instead of lite_info
#define PID_LITE_INFO_D2X   65 // like PID_LITE_INFO
#define PID_GAME_LIST_D2X   66 // like PID_GAME_LIST

#define NETGAME_ANARCHY         0
#define NETGAME_TEAM_ANARCHY    1
#define NETGAME_ROBOT_ANARCHY   2
#define NETGAME_COOPERATIVE     3
#define NETGAME_CAPTURE_FLAG    4
#define NETGAME_HOARD           5
#define NETGAME_TEAM_HOARD      6

/* The following are values for NetSecurityFlag */
#define NETSECURITY_OFF                 0
#define NETSECURITY_WAIT_FOR_PLAYERS    1
#define NETSECURITY_WAIT_FOR_GAMEINFO   2
#define NETSECURITY_WAIT_FOR_SYNC       3
/* The NetSecurityNum and the "Security" field of the network structs
 * identifies a netgame. It is a random number chosen by the network master
 * (the one that did "start netgame").
 */

typedef struct sequence_packet {
	ubyte           type;
	int             Security;
	ubyte           pad1[3];
	netplayer_info  player;
} __pack__ sequence_packet;

#define NET_XDATA_SIZE 454


// frame info is aligned -- 01/18/96 -- MWA
// if you change this structure -- be sure to keep
// alignment:
//      bytes on byte boundries
//      shorts on even byte boundries
//      ints on even byte boundries

typedef struct frame_info {
	ubyte       type;                   // What type of packet
	ubyte       pad[3];                 // Pad out length of frame_info packet
	int         numpackets;
	vms_vector  obj_pos;
	vms_matrix  obj_orient;
	vms_vector  phys_velocity;
	vms_vector  phys_rotvel;
	short       obj_segnum;
	ushort      data_size;          // Size of data appended to the net packet
	ubyte       playernum;
	ubyte       obj_render_type;
	ubyte       level_num;
	char        data[NET_XDATA_SIZE];   // extra data to be tacked on the end
} __pack__ frame_info;

// short_frame_info is not aligned -- 01/18/96 -- MWA
// won't align because of shortpos.  Shortpos needs
// to stay in current form.

typedef struct short_frame_info {
	ubyte       type;                   // What type of packet
	ubyte       pad[3];                 // Pad out length of frame_info packet
	int         numpackets;
	shortpos    thepos;
	ushort      data_size;          // Size of data appended to the net packet
	ubyte       playernum;
	ubyte       obj_render_type;
	ubyte       level_num;
	char        data[NET_XDATA_SIZE];   // extra data to be tacked on the end
} __pack__ short_frame_info;

void network_start_game();
void network_join_game();
void network_rejoin_game();
void network_leave_game();
int network_endlevel(int *secret);
void network_endlevel_poll2(int nitems, struct newmenu_item * menus, int * key, int citem);


int network_level_sync();
void network_send_endlevel_packet();

int network_delete_extra_objects();
int network_find_max_net_players();
int network_objnum_is_past(int objnum);
char * network_get_player_name(int objnum);
void network_send_endlevel_sub(int player_num);
void network_disconnect_player(int playernum);

extern void network_dump_player(ubyte * server, ubyte *node, int why);
extern void network_send_netgame_update();

extern int GetMyNetRanking();

extern int NetGameType;
extern int Network_send_objects;
extern int Network_send_objnum;
extern int PacketUrgent;
extern int Network_rejoined;

extern int Network_new_game;
extern int Network_status;

extern int PhallicLimit,PhallicMan;

extern fix LastPacketTime[MAX_PLAYERS];

extern char *RankStrings[];

extern ushort my_segments_checksum;
// By putting an up-to-20-char-message into Network_message and
// setting Network_message_reciever to the player num you want to
// send it to (100 for broadcast) the next frame the player will
// get your message.

// Call once at the beginning of a frame
void network_do_frame(int force, int listen);

// Tacks data of length 'len' onto the end of the next
// packet that we're transmitting.
void network_send_data(ubyte * ptr, int len, int urgent);

// returns 1 if hoard.ham available
extern int HoardEquipped();

extern int ping_table[MAX_PLAYERS];
extern void network_ping_all();
extern int network_who_is_master(void);

#endif /* _NETWORK_H */
