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
#include "pack.h"
#include "ntstring.h"
#include "fwd-window.h"
#include "d_array.h"
#include "d_bitset.h"
#include <array>

// Exported functions
#ifdef dsx
namespace dsx {
namespace multi {
namespace udp {
struct dispatch_table final : multi::dispatch_table
{
	virtual void send_data(std::span<const uint8_t> data, multiplayer_data_priority priority) const override;
	virtual void send_data_direct(std::span<const uint8_t> data, playernum_t pnum, int needack) const override;
	virtual int objnum_is_past(objnum_t objnum) const override;
	virtual void do_protocol_frame(int force, int listen) const override;
	virtual window_event_result level_sync() const override;
	virtual void send_endlevel_packet() const override;
	virtual void kick_player(const _sockaddr &dump_addr, kick_player_reason why) const override;
	virtual void disconnect_player(int playernum) const override;
	virtual int end_current_level(
#if defined(DXX_BUILD_DESCENT_I)
		next_level_request_secret_flag *secret
#endif
		) const override;
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
void net_udp_list_join_game(grs_canvas &canvas);
window_event_result net_udp_level_sync();

// Some defines
// Our default port - easy to remember: D = 4, X = 24, X = 24
namespace dcx {
constexpr uint16_t UDP_PORT_DEFAULT = 42424;
#define UDP_MANUAL_ADDR_DEFAULT "localhost"
#if DXX_USE_TRACKER
#ifndef TRACKER_ADDR_DEFAULT
/* Allow an alternate default at compile time */
#define TRACKER_ADDR_DEFAULT "tracker.dxx-rebirth.com"
#endif
constexpr uint16_t TRACKER_PORT_DEFAULT = 9999;
enum class tracker_game_id : uint16_t
{
};
#endif
#define UDP_MAX_NETGAMES 900
constexpr std::integral_constant<unsigned, 12> UDP_NETGAMES_PPAGE{}; // Netgames on one page of Netlist
}
#define UDP_NETGAMES_PAGES 75 // Pages available on Netlist (UDP_MAX_NETGAMES/UDP_NETGAMES_PPAGE)
#define UDP_TIMEOUT (5*F1_0) // 5 seconds disconnect timeout
#define UDP_MDATA_STOR_QUEUE_SIZE 1024u // Store up to 1024 MDATA packets
#define UDP_MDATA_STOR_MIN_FREE_2JOIN 384u // have at least this many free packet slots before we let someone join the game
#define UDP_MDATA_PKT_NUM_MIN 1 // start from pkt_num 1 (0 is used to initialize the trace list)
#define UDP_MDATA_PKT_NUM_MAX (UDP_MDATA_STOR_QUEUE_SIZE*100) // the max value for pkt_num. roll over when we go any higher. this should be smaller than INT_MAX

// UDP-Packet identificators (ubyte) and their (max. sizes).
#define UPID_MAX_SIZE			       1024 // Max size for a packet
#define UPID_MDATA_BUF_SIZE			454
#if DXX_USE_TRACKER
#define UPID_TRACKER_REGISTER			 21 // Register or update a game on the tracker.
#define UPID_TRACKER_REMOVE			 22 // Remove our game from the tracker.
#define UPID_TRACKER_REQGAMES			 23 // Request a list of all games stored on the tracker.
#endif

// Structure keeping lite game infos (for netlist, etc.)
#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
struct UDP_netgame_info_lite : public prohibit_void_ptr<UDP_netgame_info_lite>
{
	struct _sockaddr                game_addr;
	std::array<short, 3>                 program_iver;
	fix                             GameID;
#if DXX_USE_TRACKER
	tracker_game_id			TrackerGameID;
#endif
	ntstring<NETGAME_NAME_LEN> game_name;
	ntstring<MISSION_NAME_LEN> mission_title;
	ntstring<8> mission_name;
	int32_t                         levelnum;
	network_game_type               gamemode;
	ubyte                           RefusePlayers;
	ubyte                           difficulty;
	network_state game_status;
	ubyte                           numconnected;
	ubyte                           max_numplayers;
	bit_game_flags game_flag;
};

struct player_acknowledgement_mask : enumerated_bitset<MAX_PLAYERS, std::conditional<std::is_same<std::size_t, unsigned>::value, unsigned long, playernum_t>::type>
{
public:
	constexpr player_acknowledgement_mask() :
		enumerated_bitset{(1u << MAX_PLAYERS) - 1}
	{
	}
};
#endif

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
	per_player_array<fix64>		pkt_timestamp;		// Packet timestamp
	per_player_array<uint32_t>	pkt_num;			// Packet number
	sbyte				used;
	ubyte				Player_num;				// sender of this packet
	uint16_t			data_size;
	player_acknowledgement_mask player_ack;		// 0 if player has not ACK'd this packet, 1 if ACK'd or not connected
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
