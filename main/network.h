/* $Id: network.h,v 1.12 2003-10-10 09:36:35 btb Exp $ */
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
 * Old Log:
 * Revision 1.7  1995/10/31  10:20:04  allender
 * shareware stuff
 *
 * Revision 1.6  1995/09/21  14:31:18  allender
 * new appltalk type packet which contains shortpos
 *
 * Revision 1.5  1995/09/18  08:07:08  allender
 * added function prototype to remove netgame NBP
 *
 * Revision 1.4  1995/08/31  15:51:55  allender
 * new prototypes for join and start games
 *
 * Revision 1.3  1995/07/26  17:02:29  allender
 * implemented and working on mac
 *
 * Revision 1.2  1995/06/02  07:42:34  allender
 * fixed prototype for network_endlevel_poll2
 *
 * Revision 1.1  1995/05/16  16:00:15  allender
 * Initial revision
 *
 * Revision 2.2  1995/03/21  14:58:09  john
 * *** empty log message ***
 *
 * Revision 2.1  1995/03/21  14:39:43  john
 * Ifdef'd out the NETWORK code.
 *
 * Revision 2.0  1995/02/27  11:29:48  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.72  1995/02/11  14:24:39  rob
 * Added suppor for invul. cntrlcen.
 *
 * Revision 1.71  1995/02/08  19:18:43  rob
 * Added extern for segments checksum.
 *
 * Revision 1.70  1995/02/08  11:01:16  rob
 * Fixed a bug.
 *
 * Revision 1.69  1995/02/07  21:16:49  rob
 * Added flag for automap.
 *
 * Revision 1.68  1995/02/06  18:18:33  rob
 * Extern'ed Packet Urgent.
 *
 * Revision 1.67  1995/02/05  14:36:29  rob
 * Moved defines to network.h.
 *
 * Revision 1.66  1995/02/01  16:34:09  john
 * Linted.
 *
 * Revision 1.65  1995/01/30  21:14:35  rob
 * Fixed a bug in join menu.
 * Simplified mission load support.
 *
 * Revision 1.64  1995/01/30  18:19:40  rob
 * Added mission title to gameinfo packet.
 *
 * Revision 1.63  1995/01/28  17:02:39  rob
 * Fixed monitor syncing bug (minor 19).
 *
 * Revision 1.62  1995/01/27  11:15:25  rob
 * removed extern of variable no longer in network.c
 *
 * Revision 1.61  1995/01/26  12:17:34  rob
 * Added new param to network_do_frame.
 *
 * Revision 1.60  1995/01/22  17:32:11  rob
 * Added mission support for network games.
 *
 * Revision 1.59  1995/01/17  17:10:33  rob
 * Added some flags to netgame struct.
 *
 * Revision 1.58  1995/01/12  18:57:15  rob
 * Added score resync stuff.
 *
 * Revision 1.57  1995/01/12  16:42:18  rob
 * Added new prototypes.
 *
 * Revision 1.56  1995/01/05  12:12:10  rob
 * Fixed a problem with packet size.
 *
 * Revision 1.55  1995/01/05  11:12:36  rob
 * Reduced packet size by 1 byte just to be safe.
 *
 * Revision 1.54  1995/01/04  21:39:32  rob
 * New framedata packet for registered.
 *
 * Revision 1.53  1995/01/04  08:47:04  rob
 * JOHN CHECKED IN FOR ROB !!!
 *
 * Revision 1.52  1994/12/30  20:09:07  rob
 * ADded a toggle for showing player names on HUD.
 *
 * Revision 1.51  1994/12/29  15:59:52  rob
 * Centralized network disconnects.
 *
 * Revision 1.50  1994/12/09  21:21:57  rob
 * rolled back changes.
 *
 * Revision 1.48  1994/12/05  22:59:27  rob
 * added prototype for send_endlevel_packet.
 *
 * Revision 1.47  1994/12/05  21:47:34  rob
 * Added a new field to netgame struct.
 *
 * Revision 1.46  1994/12/05  14:39:16  rob
 * Added new variable to indicate new net game starting.
 *
 * Revision 1.45  1994/12/04  15:37:18  rob
 * Fixed a typo.
 *
 * Revision 1.44  1994/12/04  15:30:42  rob
 * Added new fields to netgame struct.
 *
 * Revision 1.43  1994/12/03  18:03:19  rob
 * ADded team kill syncing.
 *
 * Revision 1.42  1994/12/01  21:21:27  rob
 * Added new system for object sync on rejoin.
 *
 * Revision 1.41  1994/11/29  13:07:33  rob
 * Changed structure defs to .h files.
 *
 * Revision 1.40  1994/11/22  17:10:48  rob
 * Fix for secret levels in network play mode.
 *
 * Revision 1.39  1994/11/18  16:36:31  rob
 * Added variable network_rejoined to enable random placement
 * of rejoining palyers.
 *
 * Revision 1.38  1994/11/12  19:51:13  rob
 * Changed prototype for network_send_data to pass
 * urgent flag.
 *
 * Revision 1.37  1994/11/10  21:48:26  rob
 * Changed net_endlevel to return an int.
 *
 * Revision 1.36  1994/11/10  20:32:49  rob
 * Added extern for LastPacketTime.
 *
 * Revision 1.35  1994/11/09  11:36:34  rob
 * ADded return value to network_level_sync for success/fail.
 *
 * Revision 1.34  1994/11/08  21:22:31  rob
 * Added proto for network_endlevel_snyc
 *
 * Revision 1.33  1994/11/08  15:20:00  rob
 * added proto for change_playernum_to
 *
 * Revision 1.32  1994/11/07  17:45:40  rob
 * Added prototype for network_endlevel (called from multi.c)
 *
 * Revision 1.31  1994/11/04  19:52:37  rob
 * Removed a function from network.h (network_show_player_list)
 *
 * Revision 1.30  1994/11/01  19:39:26  rob
 * Removed a couple of variables that should be externed from multi.c
 * (remnants of the changover)
 *
 * Revision 1.29  1994/10/31  19:18:24  rob
 * Changed prototype for network_do_frame to add a parameter if you wish
 * to force the frame to be sent.  Important if this is a leave_game frame.
 *
 * Revision 1.28  1994/10/08  19:59:20  rob
 * Removed network message variables.
 *
 * Revision 1.27  1994/10/08  11:08:38  john
 * Moved network_delete_unused objects into multi.c,
 * took out network callsign stuff and used Matt's net
 * players[.].callsign stuff.
 *
 * Revision 1.26  1994/10/07  11:26:20  rob
 * Changed network_start_frame to network_do_frame,.
 *
 *
 * Revision 1.25  1994/10/05  13:58:10  rob
 * Added a new function net_endlevel that is called after each level is
 * completed.  Currently it only does anything if SERIAL game is in effect.
 *
 * Revision 1.24  1994/10/04  19:34:57  rob
 * export network_find_max_net_players.
 *
 * Revision 1.23  1994/10/04  17:31:10  rob
 * Exported MaxNumNetPlayers.
 *
 * Revision 1.22  1994/10/03  22:57:30  matt
 * Took out redundant definition of callsign_len
 *
 * Revision 1.21  1994/10/03  15:12:48  rob
 * Boosted MAX_CREATE_OBJECTS to 15.
 *
 * Revision 1.20  1994/09/30  18:19:51  rob
 * Added two new variables for tracking object creation.
 *
 * Revision 1.19  1994/09/27  15:03:18  rob
 * Added prototype for network_delete_extra_objects used by modem.c
 *
 * Revision 1.18  1994/09/27  14:36:45  rob
 * Added two new varaibles for network/serial weapon firing.
 *
 * Revision 1.17  1994/09/07  17:10:57  john
 * Started adding code to sync powerups.
 *
 * Revision 1.16  1994/09/06  19:29:05  john
 * Added trial version of rejoin function.
 *
 * Revision 1.15  1994/08/26  13:01:54  john
 * Put high score system in.
 *
 * Revision 1.14  1994/08/25  18:12:04  matt
 * Made player's weapons and flares fire from the positions on the 3d model.
 * Also added support for quad lasers.
 *
 * Revision 1.13  1994/08/17  16:50:00  john
 * Added damaging fireballs, missiles.
 *
 * Revision 1.12  1994/08/16  12:25:22  john
 * Added hooks for sending messages.
 *
 * Revision 1.11  1994/08/13  12:20:18  john
 * Made the networking uise the Players array.
 *
 * Revision 1.10  1994/08/12  22:41:27  john
 * Took away Player_stats; add Players array.
 *
 * Revision 1.9  1994/08/12  03:10:22  john
 * Made network be default off; Moved network options into
 * main menu.  Made starting net game check that mines are the
 * same.
 *
 * Revision 1.8  1994/08/11  21:57:08  john
 * Moved network options to main menu.
 *
 * Revision 1.7  1994/08/10  11:29:20  john
 * *** empty log message ***
 *
 * Revision 1.6  1994/08/10  10:44:12  john
 * Made net players fire..
 *
 * Revision 1.5  1994/08/09  19:31:46  john
 * Networking changes.
 *
 * Revision 1.4  1994/08/05  16:30:26  john
 * Added capability to turn off network.
 *
 * Revision 1.3  1994/08/05  16:11:43  john
 * Psuedo working version of networking.
 *
 * Revision 1.2  1994/07/25  12:33:34  john
 * Network "pinging" in.
 *
 * Revision 1.1  1994/07/20  16:09:05  john
 * Initial revision
 *
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
	ubyte       data[NET_XDATA_SIZE];   // extra data to be tacked on the end
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
	ubyte       data[NET_XDATA_SIZE];   // extra data to be tacked on the end
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

extern fix LastPacketTime[MAX_PLAYERS];

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

#endif /* _NETWORK_H */
