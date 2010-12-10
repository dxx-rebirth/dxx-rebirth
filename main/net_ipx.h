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
 * Prototypes for IPX-protocol network management functions.
 *
 */

#ifdef NETWORK

#ifndef _NETWORK_H
#define _NETWORK_H

#include "gameseq.h"
#include "multi.h"
#include "pstypes.h"
#ifdef _WIN32
#include <winsock.h>
#else
#include <netinet/in.h> /* for htons & co. */
#endif

#define IPX_TIMEOUT (10*F1_0) // 10 seconds disconnect timeout
#define IPX_MAX_NETGAMES                     12


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

typedef struct IPX_endlevel_info {
        ubyte                                   type;
        ubyte                                   player_num;
        sbyte                                    connected;
        short                                   kill_matrix[MAX_PLAYERS][MAX_PLAYERS];
        short                                   kills;
        short                                   killed;
        ubyte                                   seconds_left;
} __pack__ IPX_endlevel_info;

typedef struct IPX_sequence_packet {
	ubyte					type;
	netplayer_info		player;
} __pack__ IPX_sequence_packet;

#define NET_XDATA_SIZE 454

typedef struct IPX_frame_info {
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
} __pack__ IPX_frame_info;

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

int net_ipx_setup_game();
void net_ipx_join_game();
void net_ipx_leave_game();
int net_ipx_endlevel(int *secret);
int net_ipx_endlevel_poll2( newmenu *menu, d_event *event, int *secret );
int net_ipx_kmatrix_poll1( newmenu *menu, d_event *event, void *userdata );
int net_ipx_level_sync();
void net_ipx_send_endlevel_packet();
int net_ipx_objnum_is_past(int objnum);
char * net_ipx_get_player_name( int objnum );
void net_ipx_disconnect_player(int playernum);

void net_ipx_send_objects(void);
void net_ipx_dump_player(ubyte * server, ubyte *node, int why);
void net_ipx_send_game_info(IPX_sequence_packet *their);

// By putting an up-to-20-char-message into Network_message and 
// setting Network_message_reciever to the player num you want to
// send it to (100 for broadcast) the next frame the player will
// get your message.

// Call once at the beginning of a frame
void net_ipx_do_frame(int force, int listen);

// Tacks data of length 'len' onto the end of the next
// packet that we're transmitting.
void net_ipx_send_data( ubyte * ptr, int len, int urgent );

extern int IPX_Socket;
extern int IPX_active;

/* General IPX stuff - START */
#define MAX_PACKET_DATA		1500
#define MAX_DATA_SIZE		542
#define MAX_IPX_DATA		576

#define IPX_DEFAULT_SOCKET 0x5100

#define NETPROTO_IPX		1
#define NETPROTO_KALINIX	2

typedef struct IPXAddressStruct {
	ubyte Network[4];
	ubyte Node[6];
	ubyte Socket[2];
} IPXAddress_t;

typedef struct IPXPacketStructure {
	ushort Checksum;
	ushort Length;
	ubyte TransportControl;
	ubyte PacketType;
	IPXAddress_t Destination;
	IPXAddress_t Source;
} IPXPacket_t;

typedef struct socket_struct {
	ushort socket;
	int fd;
} socket_t;

struct recv_data {
	/* all network order */
	ubyte src_network[4];
	ubyte src_node[6];
	ushort src_socket;
	ushort dst_socket;
	int pkt_type;
};

struct net_driver {
	int (*open_socket)(socket_t *sk, int port);
	void (*close_socket)(socket_t *mysock);
	int (*send_packet)(socket_t *mysock, IPXPacket_t *IPXHeader, ubyte *data, int dataLen);
	int (*receive_packet)(socket_t *s, char *buffer, int bufsize, struct recv_data *rec);
	int (*packet_ready)(socket_t *s);
	int usepacketnum;//we can save 4 bytes
	int type; // type of driver (NETPROTO_*). Can be used to make driver-specific rules in other parts of the multiplayer code.
};

extern int ipxdrv_general_packet_ready(int fd);
extern void ipxdrv_get_local_target( ubyte * server, ubyte * node, ubyte * local_target );
extern int ipxdrv_set(int arg);
extern int ipxdrv_change_default_socket( ushort socket_number );
extern ubyte * ipxdrv_get_my_local_address();
extern ubyte * ipxdrv_get_my_server_address();
extern int ipxdrv_get_packet_data( ubyte * data );
extern void ipxdrv_send_broadcast_packet_data( ubyte * data, int datasize );
extern void ipxdrv_send_packet_data( ubyte * data, int datasize, ubyte *network, ubyte *address, ubyte *immediate_address );
extern void ipxdrv_send_internetwork_packet_data( ubyte * data, int datasize, ubyte * server, ubyte *address );
extern int ipxdrv_type(void);

#ifndef __APPLE__
extern struct net_driver ipxdrv_ipx;
#endif
#ifdef __LINUX__
extern struct net_driver ipxdrv_kali;
#endif

extern unsigned char MyAddress[10];
extern ubyte broadcast_addr[];
extern ubyte null_addr[];
extern u_int32_t ipx_network;
/* General IPX stuff - END */
#endif
#endif
 
