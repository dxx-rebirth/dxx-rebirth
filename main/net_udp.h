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

// Some defines
#ifdef IPv6
#define UDP_MCASTv6_ADDR "ff02::1"
#endif
#define UDP_BCAST_ADDR "255.255.255.255"
#define UDP_PORT_DEFAULT 42424 // Our default port - easy to remember: D = 4, X = 24, X = 24
#define UDP_MANUAL_ADDR_DEFAULT "localhost"
#ifdef USE_TRACKER
#define TRACKER_ADDR_DEFAULT "dxxtracker.reenigne.net"
#define TRACKER_PORT_DEFAULT 42420
#endif
#define UDP_REQ_ID "D1XR" // ID string for a request packet
#define UDP_MAX_NETGAMES 900
#define UDP_NETGAMES_PPAGE 12 // Netgames on one page of Netlist
#define UDP_NETGAMES_PAGES 75 // Pages available on Netlist (UDP_MAX_NETGAMES/UDP_NETGAMES_PPAGE)
#define UDP_TIMEOUT (5*F1_0) // 5 seconds disconnect timeout
#define UDP_MDATA_STOR_QUEUE_SIZE 500 // Store up to 500 MDATA packets

// UDP-Packet identificators (ubyte) and their (max. sizes).
#define UPID_VERSION_DENY			  1 // Netgame join or info has been denied due to version difference.
#define UPID_VERSION_DENY_SIZE			  9
#define UPID_GAME_INFO_REQ			  2 // Requesting all info about a netgame.
#define UPID_GAME_INFO_REQ_SIZE			 13
#define UPID_GAME_INFO_LITE_REQ_SIZE		 11
#define UPID_GAME_INFO				  3 // Packet containing all info about a netgame.
#define UPID_GAME_INFO_SIZE			505
#define UPID_GAME_INFO_LITE_REQ			  4 // Requesting lite info about a netgame. Used for discovering games.
#define UPID_GAME_INFO_LITE			  5 // Packet containing lite netgame info.
#define UPID_GAME_INFO_LITE_SIZE		 69
#define UPID_DUMP				  6 // Packet containing why player cannot join this game.
#define UPID_DUMP_SIZE				  2
#define UPID_ADDPLAYER				  7 // Packet from Host containing info about a new player.
#define UPID_REQUEST				  8 // New player says: "I want to be inside of you!" (haha, sorry I could not resist) / Packet containing request to join the game actually.
#define UPID_QUIT_JOINING			  9 // Packet from a player who suddenly quits joining.
#define UPID_SEQUENCE_SIZE			 12
#define UPID_SYNC				 10 // Packet from host containing full netgame info to sync players up.
#define UPID_OBJECT_DATA			 11 // Packet from host containing object buffer.
#define UPID_PING				 12 // Packet from host containing his GameTime and the Ping list. Client returns this time to host as UPID_PONG and adapts the ping list.
#define UPID_PING_SIZE				 37
#define UPID_PONG				 13 // Packet answer from client to UPID_PING. Contains the time the initial ping packet was sent.
#define UPID_PONG_SIZE				 10
#define UPID_ENDLEVEL_H				 14 // Packet from Host to all Clients containing connect-states and kills information about everyone in the game.
#define UPID_ENDLEVEL_C				 15 // Packet from Client to Host containing connect-state and kills information from this Client.
#define UPID_PDATA				 16 // Packet from player containing his movement data.
#define UPID_MDATA_PNORM			 17 // Packet containing multi buffer from a player. Priority 0,1 - no ACK needed.
#define UPID_MDATA_PNEEDACK			 18 // Packet containing multi buffer from a player. Priority 2 - ACK needed. Also contains pkt_num
#define UPID_MDATA_ACK				 19 // ACK packet for UPID_MDATA_P1.
#define UPID_MAX_SIZE			       1024 // Max size for a packet
#define UPID_MDATA_BUF_SIZE			454
#ifdef USE_TRACKER
#  define UPID_TRACKER_VERIFY			 21 // The tracker has successfully gotten a hold of us
#  define UPID_TRACKER_INCGAME			 22 // The tracker is sending us some game info
#endif

// Structure keeping lite game infos (for netlist, etc.)
typedef struct UDP_netgame_info_lite
{
	struct _sockaddr                game_addr;
	short                           program_iver[3];
	fix                             GameID;
	char                            game_name[NETGAME_NAME_LEN+1];
	char                            mission_title[MISSION_NAME_LEN+1];
	char                            mission_name[9];
	int32_t                         levelnum;
	ubyte                           gamemode;
	ubyte                           RefusePlayers;
	ubyte                           difficulty;
	ubyte                           game_status;
	ubyte                           numconnected;
	ubyte                           max_numplayers;
	ubyte                           game_flags;
} __pack__ UDP_netgame_info_lite;

typedef struct UDP_sequence_packet
{
	ubyte           		type;
	netplayer_info  		player;
} __pack__ UDP_sequence_packet;

// player position packet structure
typedef struct UDP_frame_info
{
	ubyte				type;
	ubyte				Player_num;
	ubyte				connected;
	ubyte				obj_render_type;
	shortpos			pos;
} __pack__ UDP_frame_info;

// packet structure for multi-buffer
typedef struct UDP_mdata_info
{
	ubyte				type;
	ubyte				Player_num;
	uint32_t			pkt_num;
	ushort				mbuf_size;
	ubyte				mbuf[UPID_MDATA_BUF_SIZE];
} __pack__ UDP_mdata_info;

// structure to store MDATA to maybe resend
typedef struct UDP_mdata_store
{
	int 				used;
	fix64				pkt_initial_timestamp;		// initial timestamp to see if packet is outdated
	fix64				pkt_timestamp[MAX_PLAYERS];	// Packet timestamp
	int				pkt_num;			// Packet number
	ubyte				Player_num;			// sender of this packet
	ubyte				player_ack[MAX_PLAYERS]; 	// 0 if player has not ACK'd this packet, 1 if ACK'd or not connected
	ubyte				data[UPID_MDATA_BUF_SIZE];	// extra data of a packet - contains all multibuf data we don't want to loose
	ushort				data_size;
} __pack__ UDP_mdata_store;

// structure to keep track of MDATA packets we've already got
typedef struct UDP_mdata_recv
{
	int				pkt_num[UDP_MDATA_STOR_QUEUE_SIZE];
	int				cur_slot; // index we can use for a new pkt_num
} __pack__ UDP_mdata_recv;
	
