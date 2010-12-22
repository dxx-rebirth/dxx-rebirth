/*
 *
 * Prototypes for UDP-protocol network management functions.
 *
 */

#include "multi.h"

// Exported functions
int net_udp_setup_game(void);
void net_udp_manual_join_game();
void net_udp_list_join_game();
int net_udp_objnum_is_past(int objnum);
void net_udp_do_frame(int force, int listen);
void net_udp_send_data( ubyte * ptr, int len, int priority );
void net_udp_leave_game();
int net_udp_endlevel(int *secret);
int net_udp_kmatrix_poll1( newmenu *menu, d_event *event, void *userdata );
int net_udp_kmatrix_poll2( newmenu *menu, d_event *event, void *userdata );
void net_udp_send_endlevel_packet();
void net_udp_dump_player(struct _sockaddr dump_addr, int why);
void net_udp_disconnect_player(int playernum);
int net_udp_level_sync();
void net_udp_send_mdata_direct(ubyte *data, int data_len, int pnum, int priority);
void net_udp_send_netgame_update();

// Some defines
#ifdef IPv6
#define UDP_MCASTv6_ADDR "ff02::1"
#endif
#define UDP_BCAST_ADDR "255.255.255.255"
#define UDP_PORT_DEFAULT 42424 // Our default port - easy to remember: D = 4, X = 24, X = 24
#define UDP_REQ_ID "D2XR" // ID string for a request packet
#define UDP_MAX_NETGAMES 600
#define UDP_NETGAMES_PPAGE 12 // Netgames on one page of Netlist
#define UDP_NETGAMES_PAGES 50 // Pages available on Netlist (UDP_MAX_NETGAMES/UDP_NETGAMES_PPAGE)
#define UDP_TIMEOUT (10*F1_0) // 10 seconds disconnect timeout
#define UDP_MDATA_STOR_QUEUE_SIZE	500 // Store up to 500 MDATA packets
#define UDP_OBJ_PACKETS_PER_FRAME 1

// Following are static defines for the buffer size of various packets. IF you change the packets, you must change the size, too.
#define UPKT_MAX_SIZE 1024 // Max size for a packet - just for the buffers
#define UPKT_GAME_INFO_REQ_SIZE 11 
#define UPKT_SEQUENCE_SIZE 14
#define UPKT_PING_SIZE 37
#define UPKT_PONG_SIZE 10
#define UPKT_MBUF_SIZE 454

// UDP-Packet identificators (ubyte).
#define UPID_VERSION_DENY			 1 // Netgame join or info has been denied due to version difference.
#define UPID_GAME_INFO_REQ			 2 // Requesting all info about a netgame.
#define UPID_GAME_INFO				 3 // Packet containing all info about a netgame.
#define UPID_GAME_INFO_LITE_REQ		 4 // Requesting lite info about a netgame. Used for discovering games.
#define UPID_GAME_INFO_LITE			 5 // Packet containing lite netgame info.
#define UPID_DUMP					 6 // Packet containing why player cannot join this game.
#define UPID_ADDPLAYER				 7 // Packet from Host containing info about a new player.
#define UPID_REQUEST				 8 // New player says: "I want to be inside of you!" (haha, sorry I could not resist) / Packet containing request to join the game actually.
#define UPID_QUIT_JOINING			 9 // Packet from a player who suddenly quits joining.
#define UPID_SYNC					10 // Packet from host containing full netgame info to sync players up.
#define UPID_OBJECT_DATA			11 // Packet from host containing object buffer.
#define UPID_PING					12 // Packet from host containing his GameTime and the Ping list. Client returns this time to host as UPID_PONG and adapts the ping list.
#define UPID_PONG					13 // Packet answer from client to UPID_PING. Contains the time the initial ping packet was sent.
#define UPID_ENDLEVEL_H				14 // Packet from Host to all Clients containing connect-states and kills information about everyone in the game.
#define UPID_ENDLEVEL_C				15 // Packet from Client to Host containing connect-state and kills information from this Client.
#define UPID_PDATA_H				16 // Packet from Host to all Clients containing all players movement data.
#define UPID_PDATA_C				17 // Packet from Client to Host containing his movement data.
#define UPID_MDATA_P0				18 // Packet containing multi buffer from a player. Priority 0 - no ACK needed.
#define UPID_MDATA_P1				19 // Packet containing multi buffer from a player. Priority 1 - ACK needed. Also contains pkt_num
#define UPID_MDATA_ACK				20 // ACK packet for UPID_MDATA_P1.

// Structure keeping lite game infos (for netlist, etc.)
typedef struct UDP_netgame_info_lite
{
	struct _sockaddr                game_addr;
	short                           program_iver[3];
	fix                             GameID;
	char                            game_name[NETGAME_NAME_LEN+1];
	char                            mission_title[MISSION_NAME_LEN+1];
	char                            mission_name[9];
	int                             levelnum;
	ubyte                           gamemode;
	ubyte                           RefusePlayers;
	ubyte                           difficulty;
	ubyte                           game_status;
	ubyte                           numconnected;
	ubyte                           max_numplayers;
	ubyte                           game_flags;
	ubyte                           team_vector;
} __pack__ UDP_netgame_info_lite;

typedef struct UDP_sequence_packet
{
	ubyte           				type;
	netplayer_info  				player;
} __pack__ UDP_sequence_packet;

// player position packet structure
typedef struct UDP_frame_info
{
	ubyte			type;
	ubyte			Player_num;
	ubyte			connected;
	ubyte			obj_render_type;
	shortpos		pos;
} __pack__ UDP_frame_info;

// packet structure for multi-buffer
typedef struct UDP_mdata_info
{
	ubyte			type;
	ubyte			Player_num;
	uint32_t		pkt_num;
	ushort			mbuf_size;
	ubyte			mbuf[UPKT_MBUF_SIZE];
} __pack__ UDP_mdata_info;

// structure to store MDATA to maybe resend
typedef struct UDP_mdata_store
{
	int 		used;
	fix64			pkt_initial_timestamp;		// initial timestamp to see if packet is outdated
	fix64			pkt_timestamp[MAX_PLAYERS];	// Packet timestamp
	int			pkt_num;					// Packet number
	ubyte		Player_num;					// sender of this packet
	ubyte		player_ack[MAX_PLAYERS]; 	// 0 if player has not ACK'd this packet, 1 if ACK'd or not connected
	ubyte       data[UPKT_MBUF_SIZE];		// extra data of a packet - contains all multibuf data we don't want to loose
	ushort		data_size;
} __pack__ UDP_mdata_store;

// structure to keep track of MDATA packets we've already got
typedef struct UDP_mdata_recv
{
	int pkt_num[UDP_MDATA_STOR_QUEUE_SIZE];
	int cur_slot; // index we can use for a new pkt_num
} __pack__ UDP_mdata_recv;
	
