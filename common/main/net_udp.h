/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 *
 * Prototypes for UDP-protocol network management functions.
 *
 */

#pragma once
#include "multi.h"
#include "newmenu.h"

#ifdef __cplusplus
#include "pack.h"
#include "ntstring.h"
#include "fwd-window.h"
#include <array>

// Exported functions
#ifdef dsx
namespace dsx {
namespace multi {
namespace udp {
struct dispatch_table final : multi::dispatch_table
{
	virtual int objnum_is_past(objnum_t objnum) const override;
	virtual void do_protocol_frame(int force, int listen) const override;
	virtual window_event_result level_sync() const override;
	virtual void send_endlevel_packet() const override;
	virtual void kick_player(const _sockaddr &dump_addr, int why) const override;
	virtual void disconnect_player(int playernum) const override;
	virtual int end_current_level(int *secret) const override;
	virtual void leave_game() const override;
};

extern const dispatch_table dispatch;
int kmatrix_poll2(newmenu *menu, const d_event &event, const unused_newmenu_userdata_t *);
void leave_game();
}
using udp::dispatch;
}

window_event_result net_udp_setup_game(void);
}
#endif
void net_udp_manual_join_game();
void net_udp_list_join_game();
#ifdef dsx
namespace dsx {
}
#endif
void net_udp_send_data(const uint8_t *ptr, unsigned len, int priority);
#ifdef dsx
namespace dsx {
}
#endif
window_event_result net_udp_level_sync();
void net_udp_send_mdata_direct(const ubyte *data, int data_len, int pnum, int priority);
void net_udp_send_netgame_update();

// Some defines
// Our default port - easy to remember: D = 4, X = 24, X = 24
constexpr uint16_t UDP_PORT_DEFAULT = 42424;
#define UDP_MANUAL_ADDR_DEFAULT "localhost"
#if DXX_USE_TRACKER
#ifndef TRACKER_ADDR_DEFAULT
/* Allow an alternate default at compile time */
#define TRACKER_ADDR_DEFAULT "tracker.dxx-rebirth.com"
#endif
constexpr uint16_t TRACKER_PORT_DEFAULT = 9999;
#endif
#define UDP_MAX_NETGAMES 900
namespace dcx {
constexpr std::integral_constant<unsigned, 12> UDP_NETGAMES_PPAGE{}; // Netgames on one page of Netlist
}
#define UDP_NETGAMES_PAGES 75 // Pages available on Netlist (UDP_MAX_NETGAMES/UDP_NETGAMES_PPAGE)
#define UDP_TIMEOUT (5*F1_0) // 5 seconds disconnect timeout
#define UDP_MDATA_STOR_QUEUE_SIZE 1024 // Store up to 1024 MDATA packets
#define UDP_MDATA_STOR_MIN_FREE_2JOIN 384 // have at least this many free packet slots before we let someone join the game
#define UDP_MDATA_PKT_NUM_MIN 1 // start from pkt_num 1 (0 is used to initialize the trace list)
#define UDP_MDATA_PKT_NUM_MAX (UDP_MDATA_STOR_QUEUE_SIZE*100) // the max value for pkt_num. roll over when we go any higher. this should be smaller than INT_MAX

// UDP-Packet identificators (ubyte) and their (max. sizes).
#define UPID_VERSION_DENY			  1 // Netgame join or info has been denied due to version difference.
#define UPID_VERSION_DENY_SIZE			  9
#define UPID_GAME_INFO_REQ			  2 // Requesting all info about a netgame.
#define UPID_GAME_INFO_REQ_SIZE			 13
#define UPID_GAME_INFO_LITE_REQ_SIZE		 13
#define UPID_GAME_INFO				  3 // Packet containing all info about a netgame.
#define UPID_GAME_INFO_LITE_REQ			  4 // Requesting lite info about a netgame. Used for discovering games.
#define UPID_GAME_INFO_LITE			  5 // Packet containing lite netgame info.
#define UPID_GAME_INFO_SIZE_MAX			 (sizeof(netgame_info))
#define UPID_GAME_INFO_LITE_SIZE_MAX		 (sizeof(UDP_netgame_info_lite))
#define UPID_DUMP				  6 // Packet containing why player cannot join this game.
#define UPID_DUMP_SIZE				  2
#define UPID_ADDPLAYER				  7 // Packet from Host containing info about a new player.
#define UPID_REQUEST				  8 // New player says: "I want to be inside of you!" (haha, sorry I could not resist) / Packet containing request to join the game actually.
#define UPID_QUIT_JOINING			  9 // Packet from a player who suddenly quits joining.
#define UPID_SEQUENCE_SIZE			 (3 + (CALLSIGN_LEN+1))
#define UPID_SYNC				 10 // Packet from host containing full netgame info to sync players up.
#define UPID_OBJECT_DATA			 11 // Packet from host containing object buffer.
#define UPID_PING				 12 // Packet from host containing his GameTime and the Ping list. Client returns this time to host as UPID_PONG and adapts the ping list.
#define UPID_PING_SIZE				 37
#define UPID_PONG				 13 // Packet answer from client to UPID_PING. Contains the time the initial ping packet was sent.
#define UPID_PONG_SIZE				 10
#define UPID_ENDLEVEL_H				 14 // Packet from Host to all Clients containing connect-states and kills information about everyone in the game.
#define UPID_ENDLEVEL_C				 15 // Packet from Client to Host containing connect-state and kills information from this Client.
#define UPID_PDATA				 16 // Packet from player containing his movement data.
#define UPID_PDATA_SIZE				 49
#define UPID_MDATA_PNORM			 17 // Packet containing multi buffer from a player. Priority 0,1 - no ACK needed.
#define UPID_MDATA_PNEEDACK			 18 // Packet containing multi buffer from a player. Priority 2 - ACK needed. Also contains pkt_num
#define UPID_MDATA_ACK				 19 // ACK packet for UPID_MDATA_P1.
#define UPID_MAX_SIZE			       1024 // Max size for a packet
#define UPID_MDATA_BUF_SIZE			454
#if DXX_USE_TRACKER
#define UPID_TRACKER_REGISTER			 21 // Register or update a game on the tracker.
#define UPID_TRACKER_REMOVE			 22 // Remove our game from the tracker.
#define UPID_TRACKER_REQGAMES			 23 // Request a list of all games stored on the tracker.
#define UPID_TRACKER_GAMEINFO			 24 // Packet containing info about a game
#define UPID_TRACKER_ACK			 25 // An ACK packet from the tracker
#define UPID_TRACKER_HOLEPUNCH			 26 // Hole punching process. Sent from client to tracker to request hole punching from game host and received by host from tracker to initiate hole punching to requesting client
#endif

// Structure keeping lite game infos (for netlist, etc.)
#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
struct UDP_netgame_info_lite : public prohibit_void_ptr<UDP_netgame_info_lite>
{
	struct _sockaddr                game_addr;
	std::array<short, 3>                 program_iver;
	fix                             GameID;
	uint16_t			TrackerGameID;
	ntstring<NETGAME_NAME_LEN> game_name;
	ntstring<MISSION_NAME_LEN> mission_title;
	ntstring<8> mission_name;
	int32_t                         levelnum;
	ubyte                           gamemode;
	ubyte                           RefusePlayers;
	ubyte                           difficulty;
	ubyte                           game_status;
	ubyte                           numconnected;
	ubyte                           max_numplayers;
	bit_game_flags game_flag;
};
#endif

struct UDP_sequence_packet : prohibit_void_ptr<UDP_sequence_packet>
{
	ubyte           		type;
	netplayer_info  		player;
};

// packet structure for multi-buffer
struct UDP_mdata_info : prohibit_void_ptr<UDP_mdata_info>
{
	ubyte				type;
	ubyte				Player_num;
	uint16_t			mbuf_size;
	uint32_t			pkt_num;
	std::array<uint8_t, UPID_MDATA_BUF_SIZE> mbuf;
};

// structure to store MDATA to maybe resend
struct UDP_mdata_store : prohibit_void_ptr<UDP_mdata_store>
{
	fix64				pkt_initial_timestamp;			// initial timestamp to see if packet is outdated
	std::array<fix64, MAX_PLAYERS>		pkt_timestamp;		// Packet timestamp
	std::array<uint32_t, MAX_PLAYERS>	pkt_num;			// Packet number
	sbyte				used;
	ubyte				Player_num;				// sender of this packet
	uint16_t			data_size;
	std::array<uint8_t, MAX_PLAYERS>		player_ack; 		// 0 if player has not ACK'd this packet, 1 if ACK'd or not connected
	std::array<uint8_t, UPID_MDATA_BUF_SIZE> data;		// extra data of a packet - contains all multibuf data we don't want to loose
};

// structure to keep track of MDATA packets we already got, which we expect from another player and the pkt_num for the next packet we want to send to another player
struct UDP_mdata_check : public prohibit_void_ptr<UDP_mdata_check>
{
	std::array<uint32_t, UDP_MDATA_STOR_QUEUE_SIZE>			pkt_num; 	// all those we got just recently, so we can ignore them if we get them again
	int				cur_slot; 				// index we can use for a new pkt_num
	uint32_t			pkt_num_torecv; 			// the next pkt_num we await for this player
	uint32_t			pkt_num_tosend; 			// the next pkt_num we want to send to another player
};

#endif
