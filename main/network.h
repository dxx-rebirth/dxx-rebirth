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
 * Prototypes for network management functions.
 *
 */

#ifdef NETWORK

#ifndef _NETWORK_H
#define _NETWORK_H

#include "gameseq.h"
#include "multi.h"
#include "newmenu.h"

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

#define MAX_ACTIVE_NETGAMES                     12

typedef struct sequence_packet {
	ubyte					type;
	netplayer_info		player;
} __pack__ sequence_packet;

#ifdef SHAREWARE
#define NET_XDATA_SIZE 256
#else
#define NET_XDATA_SIZE 454
#endif

#ifdef SHAREWARE
typedef struct frame_info {
	ubyte				type;						// What type of p
	int				numpackets;			
	short				objnum;
	ubyte				playernum;
	short				obj_segnum;
	vms_vector		obj_pos;
	vms_matrix		obj_orient;
	physics_info	obj_phys_info;
	ubyte				obj_render_type;
	ubyte				level_num;
	ushort			data_size;		// Size of data appended to the net packet
	ubyte				data[NET_XDATA_SIZE];		// extra data to be tacked on the end
} __pack__ frame_info;
#else
typedef struct frame_info {
	ubyte				type;						// What type of packet
	ubyte				pad[3];					// Pad out length of frame_info packet
	int				numpackets;			
	vms_vector		obj_pos;
	vms_matrix		obj_orient;
	vms_vector		phys_velocity;
	vms_vector		phys_rotvel;
	short				obj_segnum;
	ushort			data_size;		// Size of data appended to the net packet
	ubyte				playernum;
	ubyte				obj_render_type;
	ubyte				level_num;
	char				data[NET_XDATA_SIZE];		// extra data to be tacked on the end
} __pack__ frame_info;
#endif

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
void network_endlevel_poll2( int nitems, struct newmenu_item * menus, int * key, int citem );
int network_level_sync();
void network_send_endlevel_packet();
int network_delete_extra_objects();
int network_find_max_net_players();
int network_objnum_is_past(int objnum);
char * network_get_player_name( int objnum );
void network_disconnect_player(int playernum);
int network_whois_master();

extern int Network_send_objects;
extern int Network_send_objnum;
extern int PacketUrgent;
extern int Network_rejoined;

extern int Network_new_game;
extern int Network_status;

extern int restrict_mode;

extern fix LastPacketTime[MAX_PLAYERS];

extern ushort my_segments_checksum;

//extern int Network_allow_socket_changes;

//added on 8/4/98 by Matt Mueller for global short packets declaration
extern int Network_short_packets; // short packets or not
//end modified section - Matt Mueller
//added on 8/5/98 by Victor Rachels for global pps editing
extern int Network_pps; // packets per second
//end edit - Victor Rachels

extern int network_who_is_master(void);

extern int PingTable[MAX_PLAYERS];
extern void network_ping_all();
extern void network_handle_ping_return (ubyte pnum);

// By putting an up-to-20-char-message into Network_message and 
// setting Network_message_reciever to the player num you want to
// send it to (100 for broadcast) the next frame the player will
// get your message.

// Call once at the beginning of a frame
void network_do_frame(int force, int listen);

// Tacks data of length 'len' onto the end of the next
// packet that we're transmitting.
void network_send_data( ubyte * ptr, int len, int urgent );

//added on 11/16/98 by Victor Rachels for use with mlticntrl
extern sequence_packet Network_player_rejoining; // Who is rejoining now?
extern int Network_player_added;
extern int IPX_Socket;

#define DUMP_CLOSED 0
#define DUMP_FULL 1
#define DUMP_ENDLEVEL 2
#define DUMP_DORK 3
#define DUMP_ABORTED 4
#define DUMP_CONNECTED 5
#define DUMP_LEVEL 6

#define NETWORK_TIMEOUT (10*F1_0) // 10 seconds disconnect timeout
#define REFUSE_INTERVAL F1_0 * 8

void network_send_objects(void);
void network_dump_player(ubyte * server, ubyte *node, int why);
void network_send_game_info(sequence_packet *their, int light);
//end this section change - VR

#ifdef SHAREWARE
#define PID_REQUEST                             11
#define PID_SYNC                                13
#define PID_PDATA                               14
#define PID_ADDPLAYER                           15
#define PID_DUMP                                17
#define PID_ENDLEVEL                            18
#define PID_QUIT_JOINING                        20
#define PID_OBJECT_DATA                         21
#define PID_GAME_LIST                           22
#define PID_GAME_INFO                           24
#else
#define PID_REQUEST                             25
#define PID_SYNC                                27
#define PID_PDATA                               28
#define PID_ADDPLAYER                           29
#define PID_DUMP                                31
#define PID_ENDLEVEL                            32
#define PID_QUIT_JOINING                        34
#define PID_OBJECT_DATA                         35
#define PID_GAME_LIST                           36
#define PID_GAME_INFO                           37
#endif

#define PID_D1X_GAME_INFO_REQ           65
#define PID_D1X_GAME_INFO		66
#define PID_D1X_GAME_LITE_REQ		67
#define PID_D1X_GAME_LITE		68
#define PID_D1X_SYNC			69
#define PID_D1X_REQUEST 		71
//added 03/04/99 Matt Mueller - send multi data, without extra baggage
#define PID_DIRECTDATA			72
//end addition -MM 
#define PID_PING_SEND       73
#define PID_PING_RETURN     74

#define NETGAME_ANARCHY                 0
#define NETGAME_TEAM_ANARCHY            1
#define NETGAME_ROBOT_ANARCHY           2
#define NETGAME_COOPERATIVE             3

typedef struct endlevel_info {
        ubyte                                   type;
        ubyte                                   player_num;
        sbyte                                    connected;
        short                                   kill_matrix[MAX_PLAYERS][MAX_PLAYERS];
        short                                   kills;
        short                                   killed;
        ubyte                                   seconds_left;
} __pack__ endlevel_info;

#endif
#endif
 
