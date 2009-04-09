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

typedef struct IPX_netplayer_info {
	char		callsign[CALLSIGN_LEN+1];
	ubyte		server[4];
	ubyte		node[6];
	ushort	socket;
	sbyte 		connected;
} __pack__ IPX_netplayer_info;

typedef struct IPX_netgame_info {
	ubyte					type;
	char					game_name[NETGAME_NAME_LEN+1];
	char					team_name[2][CALLSIGN_LEN+1];
	ubyte					gamemode;
        ubyte                                   difficulty;
        ubyte                                   game_status;
	ubyte					numplayers;
	ubyte					max_numplayers;
	ubyte					game_flags;
        IPX_netplayer_info                          players[MAX_PLAYERS];
	int					locations[MAX_PLAYERS];
	short					kills[MAX_PLAYERS][MAX_PLAYERS];
	int					levelnum;
	ubyte					protocol_version;
	ubyte					team_vector;
        ushort                                  segments_checksum;
	short					team_kills[2];
	short					killed[MAX_PLAYERS];
	short					player_kills[MAX_PLAYERS];
	fix					level_time;
	int					control_invul_time;
	int 					monitor_vector;
	int					player_score[MAX_PLAYERS];
	ubyte					player_flags[MAX_PLAYERS];
	char					mission_name[9];
	char					mission_title[MISSION_NAME_LEN+1];
} __pack__ IPX_netgame_info;

void net_ipx_start_game();
void net_ipx_join_game();
void net_ipx_leave_game();
int net_ipx_endlevel(int *secret);
void net_ipx_endlevel_poll2( int nitems, struct newmenu_item * menus, int * key, int citem );
int net_ipx_level_sync();
void net_ipx_send_endlevel_packet();
int net_ipx_objnum_is_past(int objnum);
char * net_ipx_get_player_name( int objnum );
void net_ipx_disconnect_player(int playernum);

//extern int Network_allow_socket_changes;

//added on 8/4/98 by Matt Mueller for global short packets declaration
extern int Network_short_packets; // short packets or not
//end modified section - Matt Mueller
//added on 8/5/98 by Victor Rachels for global pps editing
extern int Network_pps; // packets per second
//end edit - Victor Rachels

extern void net_ipx_ping_all();
extern void net_ipx_handle_ping_return (ubyte pnum);

// By putting an up-to-20-char-message into Network_message and 
// setting Network_message_reciever to the player num you want to
// send it to (100 for broadcast) the next frame the player will
// get your message.

// Call once at the beginning of a frame
void net_ipx_do_frame(int force, int listen);

// Tacks data of length 'len' onto the end of the next
// packet that we're transmitting.
void net_ipx_send_data( ubyte * ptr, int len, int urgent );

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

void net_ipx_send_objects(void);
void net_ipx_dump_player(ubyte * server, ubyte *node, int why);
void net_ipx_send_game_info(sequence_packet *their);
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
 
