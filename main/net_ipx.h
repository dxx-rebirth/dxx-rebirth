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
 * Prototypes for IPX-protocol network management functions.
 *
 */


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

#define IPX_TIMEOUT (15*F1_0) // 15 seconds disconnect timeout
#define IPX_MAX_NETGAMES     12

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

/* The following are values for NetSecurityFlag */
#define NETSECURITY_OFF                 0
#define NETSECURITY_WAIT_FOR_PLAYERS    1
#define NETSECURITY_WAIT_FOR_GAMEINFO   2
#define NETSECURITY_WAIT_FOR_SYNC       3
/* The NetSecurityNum and the "Security" field of the network structs
 * identifies a netgame. It is a random number chosen by the network master
 * (the one that did "start netgame").
 */

typedef struct IPX_sequence_packet {
	ubyte           type;
	int             Security;
	ubyte           pad1[3];
	netplayer_info  player;
} __pack__ IPX_sequence_packet;

#define NET_XDATA_SIZE 454


// frame info is aligned -- 01/18/96 -- MWA
// if you change this structure -- be sure to keep
// alignment:
//      bytes on byte boundries
//      shorts on even byte boundries
//      ints on even byte boundries

typedef struct IPX_frame_info {
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
} __pack__ IPX_frame_info;

// IPX_short_frame_info is not aligned -- 01/18/96 -- MWA
// won't align because of shortpos.  Shortpos needs
// to stay in current form.

typedef struct IPX_short_frame_info {
	ubyte       type;                   // What type of packet
	ubyte       pad[3];                 // Pad out length of frame_info packet
	int         numpackets;
	shortpos    thepos;
	ushort      data_size;          // Size of data appended to the net packet
	ubyte       playernum;
	ubyte       obj_render_type;
	ubyte       level_num;
	char        data[NET_XDATA_SIZE];   // extra data to be tacked on the end
} __pack__ IPX_short_frame_info;

typedef struct IPX_netplayer_info {
	char    callsign[CALLSIGN_LEN+1];
	union {
		struct {
			ubyte   server[4];
			ubyte   node[6];
		} ipx;
		struct {
			ushort  net;
			ubyte   node;
			ubyte   socket;
		} appletalk;
	} network;

	ubyte   version_major;
	ubyte   version_minor;
	ubyte computer_type;
	sbyte    connected;

	ushort  socket;

	ubyte   rank;

} __pack__ IPX_netplayer_info;

typedef struct IPX_AllNetPlayers_info
{
	char    type;
	int     Security;
	struct IPX_netplayer_info players[MAX_PLAYERS+4];
} __pack__ IPX_AllNetPlayers_info;

typedef struct IPX_netgame_info {
	ubyte   type;
	int     Security;
	char    game_name[NETGAME_NAME_LEN+1];
	char    mission_title[MISSION_NAME_LEN+1];
	char    mission_name[9];
	int     levelnum;
	ubyte   gamemode;
	ubyte   RefusePlayers;
	ubyte   difficulty;
	ubyte   game_status;
	ubyte   numplayers;
	ubyte   max_numplayers;
	ubyte   numconnected;
	ubyte   game_flags;
	ubyte   protocol_version;
	ubyte   version_major;
	ubyte   version_minor;
	ubyte   team_vector;
	short DoMegas:1;
	short DoSmarts:1;
	short DoFusions:1;
	short DoHelix:1;
	short DoPhoenix:1;
	short DoAfterburner:1;
	short DoInvulnerability:1;
	short DoCloak:1;
	short DoGauss:1;
	short DoVulcan:1;
	short DoPlasma:1;
	short DoOmega:1;
	short DoSuperLaser:1;
	short DoProximity:1;
	short DoSpread:1;
	short DoSmartMine:1;
	short DoFlash:1;
	short DoGuided:1;
	short DoEarthShaker:1;
	short DoMercury:1;
	short Allow_marker_view:1;
	short AlwaysLighting:1;
	short DoAmmoRack:1;
	short DoConverter:1;
	short DoHeadlight:1;
	short DoHoming:1;
	short DoLaserUpgrade:1;
	short DoQuadLasers:1;
	short ShowAllNames:1;
	short BrightPlayers:1;
	short invul:1;
	short bitfield_not_used2:1;
	char    team_name[2][CALLSIGN_LEN+1];
	int     locations[MAX_PLAYERS];
	short   kills[MAX_PLAYERS][MAX_PLAYERS];
	ushort  segments_checksum;
	short   team_kills[2];
	short   killed[MAX_PLAYERS];
	short   player_kills[MAX_PLAYERS];
	int     KillGoal;
	fix     PlayTimeAllowed;
	fix     level_time;
	int     control_invul_time;
	int     monitor_vector;
	int     player_score[MAX_PLAYERS];
	ubyte   player_flags[MAX_PLAYERS];
	short   PacketsPerSec;
	ubyte   ShortPackets;
	ubyte   AuxData[NETGAME_AUX_SIZE];  // Storage for protocol-specific data (e.g., multicast session and port)

} __pack__ IPX_netgame_info;

typedef struct IPX_lite_info {
	ubyte                           type;
	int                             Security;
	char                            game_name[NETGAME_NAME_LEN+1];
	char                            mission_title[MISSION_NAME_LEN+1];
	char                            mission_name[9];
	int                             levelnum;
	ubyte                           gamemode;
	ubyte                           RefusePlayers;
	ubyte                           difficulty;
	ubyte                           game_status;
	ubyte                           numplayers;
	ubyte                           max_numplayers;
	ubyte                           numconnected;
	ubyte                           game_flags;
	ubyte                           protocol_version;
	ubyte                           version_major;
	ubyte                           version_minor;
	ubyte                           team_vector;
} __pack__ IPX_lite_info;

int net_ipx_setup_game();
void net_ipx_join_game();
void network_rejoin_game();
void net_ipx_leave_game();
int net_ipx_endlevel(int *secret);
int net_ipx_kmatrix_poll1( newmenu *menu, d_event *event, void *userdata );
int net_ipx_kmatrix_poll2( newmenu *menu, d_event *event, void *userdata );

int net_ipx_level_sync();
void net_ipx_send_endlevel_packet();

int net_ipx_objnum_is_past(int objnum);
char * net_ipx_get_player_name(int objnum);
void net_ipx_send_endlevel_sub(int player_num);
void net_ipx_disconnect_player(int playernum);

extern void net_ipx_dump_player(ubyte * server, ubyte *node, int why);
extern void net_ipx_send_netgame_update();

// By putting an up-to-20-char-message into Network_message and
// setting Network_message_reciever to the player num you want to
// send it to (100 for broadcast) the next frame the player will
// get your message.

// Call once at the beginning of a frame
void net_ipx_do_frame(int force, int listen);

// Tacks data of length 'len' onto the end of the next
// packet that we're transmitting.
void net_ipx_send_data(ubyte * ptr, int len, int urgent);

// returns 1 if hoard.ham available
extern int HoardEquipped();

extern int IPX_Socket;
extern int IPX_active;

/* General IPX stuff - START */
#define MAX_PACKET_DATA		1500
#define MAX_DATA_SIZE		542
#define MAX_IPX_DATA		576

#define IPX_DEFAULT_SOCKET 0x5130

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

#endif /* _NETWORK_H */
