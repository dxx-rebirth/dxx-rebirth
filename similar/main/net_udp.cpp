/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 * 
 * Routines for managing UDP-protocol network play.
 * 
 */

#include "dxxsconf.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <random>
#include <ranges>

#include "pstypes.h"
#include "window.h"
#include "strutil.h"
#include "args.h"
#include "timer.h"
#include "newmenu.h"
#include "key.h"
#include "object.h"
#include "dxxerror.h"
#include "laser.h"
#include "player.h"
#include "gameseq.h"
#include "net_udp.h"
#include "game.h"
#include "gauges.h"
#include "multi.h"
#include "palette.h"
#include "powerup.h"
#include "menu.h"
#include "gameseg.h"
#include "sounds.h"
#include "text.h"
#include "newdemo.h"
#include "multibot.h"
#include "state.h"
#include "wall.h"
#include "bm.h"
#include "effects.h"
#include "physics.h"
#include "hudmsg.h"
#include "switch.h"
#include "textures.h"
#include "event.h"
#include "playsave.h"
#include "gamefont.h"
#include "vers_id.h"
#include "u_mem.h"
#include "weapon.h"

#include "compiler-cf_assert.h"
#include "compiler-range_for.h"
#include "d_enumerate.h"
#include "d_levelstate.h"
#include "d_range.h"
#include "d_zip.h"
#include "partial_range.h"
#include <array>
#include <utility>

#if defined(DXX_BUILD_DESCENT_I)
#define UDP_REQ_ID "D1XR" // ID string for a request packet
#elif defined(DXX_BUILD_DESCENT_II)
#define UDP_REQ_ID "D2XR" // ID string for a request packet
#endif

namespace dcx {

namespace {

enum class upid : uint8_t
{
	/* upid codes are part of the network ABI, and must be compatible with the
	 * game running on the peer system.
	 */
	version_deny = 1,	// Netgame join or info has been denied due to version difference.
	game_info_req,	// Requesting all info about a netgame.
	game_info,	// Packet containing all info about a netgame.
	game_info_lite_req,	// Requesting lite info about a netgame. Used for discovering games.
	game_info_lite,	// Packet containing lite netgame info.
	dump,	// Packet containing why player cannot join this game.
	addplayer,	// Packet from Host containing info about a new player.
	request,	// Packet containing request to join the game
	quit_joining,	// Packet from a player who suddenly quits joining.
	sync,	// Packet from host containing full netgame info to sync players up.
	object_data,	// Packet from host containing object buffer.
	ping,	// Packet from host containing his GameTime and the Ping list. Client returns this time to host as UPID_PONG and adapts the ping list.
	pong,	// Packet answer from client to UPID_PING. Contains the time the initial ping packet was sent.
	endlevel_h,	// Packet from Host to all Clients containing connect-states and kills information about everyone in the game.
	endlevel_c,	// Packet from Client to Host containing connect-state and kills information from this Client.
	pdata,	// Packet from player containing his movement data.
	mdata_pnorm,	// Packet containing multi buffer from a player. Priority 0,1 - no ACK needed.
	mdata_pneedack,	// Packet containing multi buffer from a player. Priority 2 - ACK needed. Also contains pkt_num
	mdata_ack,	// ACK packet for UPID_MDATA_P1.
#if DXX_USE_TRACKER
	/* Tracker upid codes are special.  They must be compatible with the
	 * tracker, which is a separate program maintained in a different
	 * repository[1], with its own private copy of these numbers[2].  Changing
	 * the value of tracker codes will break communications with the tracker
	 * program.
	 *
	 * [1]: https://github.com/Mako88/dxx-tracker/
	 * [2]: https://github.com/Mako88/dxx-tracker/blob/master/RebirthTracker/RebirthTracker/PacketHandlers/ - see `[Opcode(NN)]` in the *.cs files
	 */
	tracker_gameinfo = 24,	// Packet containing info about a game
	tracker_ack = 25,	// An ACK packet from the tracker
	tracker_holepunch = 26,	// Hole punching process. Sent from client to tracker to request hole punching from game host and received by host from tracker to initiate hole punching to requesting client
#endif
};

template <upid>
[[deprecated("only explicit specializations can be used")]]
constexpr std::size_t upid_length
#ifdef __clang__
/* clang raises an error if upid_length is not initialized, even if the generic
 * case is never referenced.
 *
 * gcc permits the generic case to be uninitialized if it is never referenced.
 * Therefore, leave it uninitialized when possible, so that any erroneous use
 * triggers an error.
 */
= std::dynamic_extent
#endif
;

template <>
constexpr std::size_t upid_length<upid::version_deny> = 9;

template <>
constexpr std::size_t upid_length<upid::game_info_req> = 13;

template <>
constexpr std::size_t upid_length<upid::game_info> = sizeof(netgame_info);

template <>
constexpr std::size_t upid_length<upid::game_info_lite_req> = 13;

template <>
constexpr std::size_t upid_length<upid::game_info_lite> = sizeof(UDP_netgame_info_lite);

template <>
constexpr std::size_t upid_length<upid::dump> = 2;

template <>
constexpr std::size_t upid_length<upid::addplayer> = 3 + (CALLSIGN_LEN + 1);

template <>
constexpr std::size_t upid_length<upid::request> = upid_length<upid::addplayer>;

template <>
constexpr std::size_t upid_length<upid::ping> = 37;

template <>
constexpr std::size_t upid_length<upid::pong> = 10;

template <>
constexpr std::size_t upid_length<upid::pdata> = 49;

template <>
constexpr std::size_t upid_length<upid::mdata_ack> = 7;

template <upid id>
using upid_rspan = std::span<const uint8_t, upid_length<id>>;

template <upid id, typename R = upid_rspan<id>>
static std::optional<R> build_upid_rspan(const std::span<const uint8_t> buf)
{
	if (buf.size() != R::extent)
		return std::nullopt;
	return buf.template first<R::extent>();
}

enum class Network_player_added : bool
{
	_0,
	_1,
};

Network_player_added network_player_added;

}

}

namespace {

void net_udp_send_netgame_update();

// player position packet structure
struct UDP_frame_info : prohibit_void_ptr<UDP_frame_info>
{
	ubyte				type;
	ubyte				Player_num;
	player_connection_status connected;
	quaternionpos			qpp;
};

enum class join_netgame_status_code : uint8_t
{
	game_in_disallowed_state,
	game_has_capacity,
	game_is_full,
	game_refuses_players,
};

struct net_udp_select_teams_menu_items
{
	static constexpr std::integral_constant<std::size_t, 0> idx_label_blue_team{};
	unsigned team_vector;
	unsigned idx_label_red_team;
	unsigned idx_item_accept;
	const color_palette_index blue_team_color;
	const color_palette_index red_team_color;
	per_team_array<callsign_t> team_names;
	/* Labels plus slots for every player */
	std::array<newmenu_item, std::size(Netgame.players) + 4> m;
	net_udp_select_teams_menu_items(unsigned num_players);
	unsigned setup_team_sensitive_entries(unsigned num_players);
};

struct net_udp_select_teams_menu : net_udp_select_teams_menu_items, newmenu
{
	net_udp_select_teams_menu(const unsigned num_players, grs_canvas &src) :
		net_udp_select_teams_menu_items(num_players),
		newmenu(menu_title{nullptr}, menu_subtitle{TXT_TEAM_SELECTION}, menu_filename{nullptr}, tiny_mode_flag::normal, tab_processing_flag::ignore, adjusted_citem::create(partial_range(m, idx_item_accept + 1), 0), src)
	{
	}
	virtual window_event_result event_handler(const d_event &event) override;
};

net_udp_select_teams_menu_items::net_udp_select_teams_menu_items(const unsigned num_players) :
	blue_team_color(BM_XRGB(player_rgb[0].r, player_rgb[0].g, player_rgb[0].b)),
	red_team_color(BM_XRGB(player_rgb[1].r, player_rgb[1].g, player_rgb[1].b))
{
	const auto set_team_name = [](callsign_t &team_name, const auto &&s) {
		if constexpr (std::is_array<typename std::remove_cvref<decltype(s)>::type>::value)
			/* When the input is an array (due to
			 * -D'USE_BUILTIN_ENGLISH_TEXT_STRINGS'), construct a std::span
			 *  implicitly with the correct length.
			 */
			team_name.copy(s);
		else
			/* Otherwise, construct one explicitly with the dynamically
			 * detected length. */
			team_name.copy(std::span(s, strlen(s)));
	};
	set_team_name(team_names[team_number::blue], TXT_BLUE);
	set_team_name(team_names[team_number::red], TXT_RED);
	/* Round blue team up.  Round red team down. */
	const unsigned num_blue_players = (num_players + 1) >> 1;
	// Put first half of players on team A
	team_vector = ((1 << num_players) - 1) & ~((1 << num_blue_players) - 1);
	/* Blue team label is always in the same position.  Red team label
	 * varies based on how many players are on the blue team, so the red
	 * team label is set by setup_team_sensitive_entries.
	 */
	nm_set_item_input(m[idx_label_blue_team], team_names[team_number::blue].a);
	const unsigned idx_label_blank0 = setup_team_sensitive_entries(num_players);
	idx_item_accept = idx_label_blank0 + 1;
	nm_set_item_text(m[idx_label_blank0], "");
	nm_set_item_menu(m[idx_item_accept], TXT_ACCEPT);
}

unsigned net_udp_select_teams_menu_items::setup_team_sensitive_entries(const unsigned num_players)
{
	const unsigned blue_team_first_player = idx_label_blue_team + 1;
	const auto tv = team_vector;
	auto mi = std::next(m.begin(), blue_team_first_player);
	unsigned ir = blue_team_first_player;
	for (auto &&[i, ngp] : enumerate(partial_range(Netgame.players, num_players)))
	{
		if (tv & (1 << i))
			continue;
		nm_set_item_menu(*mi, ngp.callsign);
		mi->value = i;
		++ mi;
		++ ir;
	}
	idx_label_red_team = ir;
	nm_set_item_input(*mi, team_names[team_number::red].a);
	++ mi;
	++ ir;
	for (auto &&[i, ngp] : enumerate(partial_range(Netgame.players, num_players)))
	{
		if (!(tv & (1 << i)))
			continue;
		nm_set_item_menu(*mi, ngp.callsign);
		mi->value = i;
		++ mi;
		++ ir;
	}
	return ir;
}

window_event_result net_udp_select_teams_menu::event_handler(const d_event &event)
{
	switch (event.type)
	{
		case event_type::newmenu_selected:
			{
				const auto citem = static_cast<const d_select_event &>(event).citem;
				if (citem == idx_item_accept)
				{
					Netgame.team_vector = team_vector;
					Netgame.team_name = team_names;
					return window_event_result::close;
				}
				else if (citem == idx_label_blue_team || citem == idx_label_red_team)
					/* No handling required, but use this return code to
					 * reuse the `handled` exit path.
					 */
					return window_event_result::handled;
				const auto player_team_mask = 1 << m[citem].value;
				enum class prior_team_color : unsigned
				{
					blue,
				} prior_team = static_cast<prior_team_color>(team_vector & player_team_mask);
				team_vector ^= player_team_mask;
				/* Rebuild the menu entries to reflect that a player has
				 * changed teams.
				 */
				setup_team_sensitive_entries(N_players);
				/* idx_label_red_team is changed by the call to
				 * setup_team_sensitive_entries, so this test is not
				 * redundant relative to the label handling above.
				 */
				if (citem != idx_label_red_team)
					/* If the selection after the change is not pointing
					 * at the label for "Red", then no special handling
					 * is needed.  The selection will now point at a
					 * player who was a teammate of the player who was
					 * moved by this handler.
					 */
					return window_event_result::handled;
				/* If the selection is now on the label "Red" ... */
				if (citem < idx_item_accept - 1 && (citem == idx_label_blue_team + 1 || prior_team != prior_team_color::blue))
					/* If the red team is not empty, and either the blue
					 * team is now empty or the previous team was not
					 * blue, increment the position to point to
					 * red-player-1.
					 *
					 * Otherwise (red is empty) or (blue is not empty
					 * and previous team was blue), decrement the
					 * position, so that the selection points to the
					 * now-last blue player.
					 *
					 * This logic allows the host to quickly transfer
					 * multiple adjacent players to the other team.
					 */
					++ this->citem;
				else
					-- this->citem;
			}
			return window_event_result::handled;
		case event_type::newmenu_draw:
			{
				const auto draw_team_color_box = [&canv = this->w_canv](const newmenu_item &mi, const color_palette_index cpi) {
					const unsigned height = mi.h - 8;
					/* Yes, height is used in an X term.  The box should
					 * be square.
					 */
					gr_urect(canv, mi.x - 12 - height, mi.y, mi.x - 12, mi.y + height, cpi);
				};
				draw_team_color_box(m[idx_label_blue_team], blue_team_color);
				draw_team_color_box(m[idx_label_red_team], red_team_color);
#ifndef NDEBUG
				/* Non-developers probably do not care about the player
				 * index, so hide this from them.
				 */
				const auto draw_player_number = [&canv = this->w_canv, &game_font = *GAME_FONT, team_vector = this->team_vector, blue_team_color = this->blue_team_color, red_team_color = this->red_team_color](const newmenu_item &mi) {
					const unsigned height = mi.h - 8;
					gr_set_fontcolor(canv, (1 << mi.value) & team_vector ? red_team_color : blue_team_color, -1);
					gr_uprintf(canv, game_font, mi.x - 12 - height, mi.y, "%d", 1 + mi.value);
				};
				for (auto &mi : partial_range(m, idx_label_blue_team + 1, idx_label_red_team))
					draw_player_number(mi);
				for (auto &mi : partial_range(m, idx_label_red_team + 1, idx_item_accept - 1))
					draw_player_number(mi);
#endif
			}
			break;
		default:
			break;
	}
	return newmenu::event_handler(event);
}

struct UDP_sequence_packet
{
	const upid type;
	const netplayer_info::player_rank rank;
	const callsign_t callsign;
protected:
	UDP_sequence_packet(const upid type, const netplayer_info::player_rank rank, const callsign_t &callsign) :
		type(type), rank(rank), callsign(callsign)
	{
	}
};

struct UDP_sequence_request_packet : UDP_sequence_packet
{
	const int8_t Current_level_num;
	UDP_sequence_request_packet(const netplayer_info::player_rank rank, const callsign_t &callsign, const int8_t Current_level_num) :
		UDP_sequence_packet(upid::request, rank, callsign), Current_level_num(Current_level_num)
	{
	}
	static UDP_sequence_request_packet build_from_untrusted(upid_rspan<upid::request> untrusted);
};

struct UDP_sequence_addplayer_packet : UDP_sequence_packet
{
	const uint8_t player_num;
	UDP_sequence_addplayer_packet(const netplayer_info::player_rank rank, const callsign_t &callsign, const int8_t player_num) :
		UDP_sequence_packet(upid::addplayer, rank, callsign), player_num(player_num)
	{
	}
	static UDP_sequence_addplayer_packet build_from_untrusted(upid_rspan<upid::addplayer>);
};

struct UDP_sequence_syncplayer_packet
{
	uint8_t player_num;
	netplayer_info::player_rank rank;
	callsign_t callsign;
	struct _sockaddr udp_addr;
	UDP_sequence_syncplayer_packet() = default;
	UDP_sequence_syncplayer_packet(const int8_t player_num, const netplayer_info::player_rank rank, const callsign_t &callsign, const struct _sockaddr &peer_addr) :
		player_num(player_num), rank(rank), callsign(callsign), udp_addr(peer_addr)
	{
	}
};

// Prototypes
static void net_udp_init();
static void net_udp_close();
static void net_udp_listen();
}
namespace dsx {
namespace multi {
namespace udp {
const dispatch_table dispatch{};
}
}
namespace {
static int net_udp_do_join_game();
static void net_udp_update_netgame();
static void net_udp_send_objects(Network_player_added network_player_added);
static void net_udp_send_rejoin_sync(Network_player_added network_player_added, unsigned player_num);
static void net_udp_do_refuse_stuff(const UDP_sequence_request_packet &their, const struct _sockaddr &peer_addr);
static void net_udp_read_sync_packet(const uint8_t *data, uint_fast32_t data_len, const _sockaddr &sender_addr);
static void net_udp_send_extras();
#if DXX_USE_TRACKER
static int udp_tracker_register();
static int udp_tracker_reqgames();
#endif

template <upid id>
constexpr std::array<uint8_t, upid_length<upid::game_info_req>> udp_request_game_info_template{{
	static_cast<uint8_t>(id),
	UDP_REQ_ID[0],
	UDP_REQ_ID[1],
	UDP_REQ_ID[2],
	UDP_REQ_ID[3],
#define DXX_CONST_INIT_LE16(V)	\
	static_cast<uint8_t>(V),	\
	static_cast<uint8_t>(V >> 8)
	DXX_CONST_INIT_LE16(DXX_VERSION_MAJORi),
	DXX_CONST_INIT_LE16(DXX_VERSION_MINORi),
	DXX_CONST_INIT_LE16(DXX_VERSION_MICROi),
	DXX_CONST_INIT_LE16(MULTI_PROTO_VERSION),
}};

}
}
namespace {
static void net_udp_ping_frame(fix64 time);
static void net_udp_process_ping(upid_rspan<upid::ping>, const _sockaddr &sender_addr);
static void net_udp_process_pong(upid_rspan<upid::pong>, const _sockaddr &sender_addr);
static void net_udp_read_endlevel_packet(const uint8_t *data, const _sockaddr &sender_addr);
static void net_udp_send_mdata(int needack, fix64 time);
static void net_udp_process_mdata(const d_level_shared_robot_info_state &LevelSharedRobotInfoState, std::span<uint8_t> data, const _sockaddr &sender_addr, int needack);
static void net_udp_send_pdata();
static void net_udp_process_pdata (std::span<const uint8_t> data, const _sockaddr &sender_addr);
static void net_udp_read_pdata_packet(UDP_frame_info *pd);
static void net_udp_timeout_check(fix64 time);
static int net_udp_get_new_player_num ();
static void net_udp_noloss_got_ack(std::span<const uint8_t>);
static void net_udp_noloss_init_mdata_queue();
static void net_udp_noloss_clear_mdata_trace(ubyte player_num);
static void net_udp_noloss_process_queue(fix64 time);
static int net_udp_start_game();

// Variables
static int UDP_num_sendto, UDP_len_sendto, UDP_num_recvfrom, UDP_len_recvfrom;
static UDP_mdata_info		UDP_MData;
static unsigned UDP_mdata_queue_highest;
static std::array<UDP_mdata_store, UDP_MDATA_STOR_QUEUE_SIZE> UDP_mdata_queue;
static per_player_array<UDP_mdata_check> UDP_mdata_trace;
static UDP_sequence_syncplayer_packet UDP_sync_player; // For rejoin object syncing
static uint16_t UDP_MyPort;
#if DXX_USE_TRACKER
static _sockaddr TrackerSocket;
enum class TrackerAckState : uint8_t
{
	TACK_NOCONNECTION,   // No connection with tracker (yet);
	TACK_INTERNAL	= 1, // Got ACK on TrackerSocket
	TACK_EXTERNAL	= 2, // Got ACK on our game sopcket
	TACK_SEQCOMPL	= 3, // We had enough time to get all acks. If we missed something now, tell the user
};
static TrackerAckState TrackerAckStatus;
static fix64 TrackerAckTime;
static int udp_tracker_init();
static int udp_tracker_unregister();
static int udp_tracker_process_game(std::span<const uint8_t> data, const _sockaddr &sender_addr);
static void udp_tracker_process_ack(const uint8_t *data, int data_len, const _sockaddr &sender_addr );
static void udp_tracker_verify_ack_timeout();
static void udp_tracker_request_holepunch(tracker_game_id TrackerGameID);
static void udp_tracker_process_holepunch(uint8_t *data, unsigned data_len, const _sockaddr &sender_addr);
#endif
}

#ifndef _WIN32
constexpr std::integral_constant<int, -1> INVALID_SOCKET{};
#endif

namespace dcx {

namespace {

constexpr std::integral_constant<uint32_t, 0xfffffffe> network_checksum_marker_object{};

constexpr sockaddr_in GBcast = { // global Broadcast address clients and hosts will use for lite_info exchange over LAN
	.sin_family = AF_INET,
	.sin_port = words_bigendian ? UDP_PORT_DEFAULT : SWAPSHORT(UDP_PORT_DEFAULT),
	.sin_addr = {
#ifdef _WIN32
		.S_un = {
			.S_addr = 0xffffffff
		}
#else
		.s_addr = 0xffffffff
#endif
	},
	.sin_zero = {}
};

#if DXX_USE_IPv6
constexpr sockaddr_in6 GMcast_v6 = { // same for IPv6-only
	.sin6_family = AF_INET6,
	.sin6_port = GBcast.sin_port,
	.sin6_flowinfo = 0,
	.sin6_addr = {
		{{0xff, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}}
	},
	.sin6_scope_id = 0,
};
#endif

enum class game_info_request_result : uint8_t
{
	// Valid request
	accept,
	/* Game type header does not match.  The request is for D1 while this
	 * program is D2, or vice versa, or the request is not a Rebirth request at
	 * all.
	 */
	id_mismatch,
	/* Game version mismatch.  The peer is running an older or newer
	 * multiplayer protocol than this program.
	 */
	version_mismatch,
};

class RAIIsocket
{
#ifndef _WIN32
	typedef int SOCKET;
	static int closesocket(SOCKET fd)
	{
		return close(fd);
	}
#endif
	SOCKET s{INVALID_SOCKET};
public:
	constexpr RAIIsocket() = default;
	RAIIsocket(int domain, int type, int protocol) : s(socket(domain, type, protocol))
	{
	}
	RAIIsocket(const RAIIsocket &) = delete;
	RAIIsocket &operator=(const RAIIsocket &) = delete;
	RAIIsocket(RAIIsocket &&r) :
		s{std::exchange(r.s, INVALID_SOCKET)}
	{
	}
	~RAIIsocket()
	{
		reset();
	}
	RAIIsocket &operator=(RAIIsocket &&r)
	{
		std::swap(s, r.s);
		return *this;
	}
	void reset()
	{
		if (s != INVALID_SOCKET)
			closesocket(std::exchange(s, INVALID_SOCKET));
	}
	[[nodiscard]]
	explicit operator bool() const { return s != INVALID_SOCKET; }
	[[nodiscard]]
	explicit operator bool() { return static_cast<bool>(*const_cast<const RAIIsocket *>(this)); }
	[[nodiscard]]
	operator SOCKET() { return s; }
	bool operator<(auto &&) const = delete;
	bool operator<=(auto &&) const = delete;
	bool operator>(auto &&) const = delete;
	bool operator>=(auto &&) const = delete;
	bool operator==(auto &&) const = delete;
	bool operator!=(auto &&) const = delete;
};

#ifdef DXX_HAVE_GETADDRINFO
struct addrinfo_deleter
{
	void operator()(addrinfo *p) const
	{
		freeaddrinfo(p);
	}
};

class RAIIaddrinfo : std::unique_ptr<addrinfo, addrinfo_deleter>
{
	using base_type = std::unique_ptr<addrinfo, addrinfo_deleter>;
public:
	int getaddrinfo(const char *node, const char *service, const addrinfo *hints)
	{
		addrinfo *p = nullptr;
		int r = ::getaddrinfo(node, service, hints, &p);
		reset(p);
		return r;
	}
	using base_type::get;
	using base_type::operator->;
};
#endif

struct sockaddr_ref
{
	sockaddr &sa;
	socklen_t len;
	/* For bug compatibility with earlier versions, the returned socklen_t from
	 * the kernel is ignored, and the data structure is assumed to be filled
	 * with a value of the correct size.
	 */
#if DXX_USE_IPv6
	sockaddr_ref(sockaddr_in6 &sai6) :
		sa(reinterpret_cast<sockaddr &>(sai6)), len(sizeof(sai6))
	{
	}
#else
	sockaddr_ref(sockaddr_in &sai) :
		sa(reinterpret_cast<sockaddr &>(sai)), len(sizeof(sai))
	{
	}
#endif
	sockaddr_ref(_sockaddr &sai_) :
#if DXX_USE_IPv6
		sockaddr_ref(sai_.sin6)
#else
		sockaddr_ref(sai_.sin)
#endif
	{
	}
};

struct csockaddr_ref
{
	const sockaddr &sa;
	const socklen_t len;
	csockaddr_ref(const sockaddr_in &sai) :
		sa(reinterpret_cast<const sockaddr &>(sai)), len(sizeof(sai))
	{
	}
#if DXX_USE_IPv6
	csockaddr_ref(const sockaddr_in6 &sai6) :
		sa(reinterpret_cast<const sockaddr &>(sai6)),
		len(
			/* If the kernel allows specifying sizeof(sockaddr_in6) for a
			 * sockaddr_in6 with sa.family == AF_INET, then always use
			 * sizeof(sockaddr_in6).  This saves a compare+jump in application
			 * code to pass sizeof(sockaddr_in6) and let the kernel sort it
			 * out.
			 */
#if defined(__linux__)
			/* Known to work */
#else
			/* Default case: not known.  Add a runtime check. */
			sai6.sin6_family == AF_INET ? sizeof(sockaddr_in) :
#endif
			sizeof(sai6))
	{
	}
#endif
	csockaddr_ref(const _sockaddr &sai_) :
#if DXX_USE_IPv6
		csockaddr_ref(sai_.sin6)
#else
		csockaddr_ref(sai_.sin)
#endif
	{
	}
};

using csocket_data_buffer = std::span<const uint8_t>;
using socket_data_buffer = std::span<uint8_t>;

class start_poll_menu_items
{
	/* The host must play */
	unsigned playercount = 1;
public:
	std::array<newmenu_item, MAX_PLAYERS + 4> m;
	unsigned get_player_count() const
	{
		return playercount;
	}
	void set_player_count(const unsigned c)
	{
		playercount = c;
	}
};

constexpr std::array<uint8_t, upid_length<upid::version_deny>> udp_response_version_deny{{
	underlying_value(upid::version_deny),
	DXX_CONST_INIT_LE16(DXX_VERSION_MAJORi),
	DXX_CONST_INIT_LE16(DXX_VERSION_MINORi),
	DXX_CONST_INIT_LE16(DXX_VERSION_MICROi),
	DXX_CONST_INIT_LE16(MULTI_PROTO_VERSION),
}};

static const char *dxx_ntop(const _sockaddr &sa, typename _sockaddr::presentation_buffer &dbuf)
{
#ifdef WIN32
#ifdef DXX_HAVE_INET_NTOP
	/*
	 * Windows and inet_ntop: copy the in_addr/in6_addr to local
	 * variables because the Microsoft prototype lacks a const
	 * qualifier.
	 */
	union {
		in_addr ia;
#if DXX_USE_IPv6
		in6_addr ia6;
#endif
	};
	const auto addr =
#if DXX_USE_IPv6
		(sa.sa.sa_family == AF_INET6)
		? &(ia6 = sa.sin6.sin6_addr)
		:
#endif
		static_cast<void *>(&(ia = sa.sin.sin_addr));
#else
	/*
	 * Windows and not inet_ntop: only inet_ntoa available.
	 *
	 * SConf check_inet_ntop_present enforces that Windows without
	 * inet_ntop cannot enable IPv6, so the IPv4 branch must be correct
	 * here.
	 *
	 * The reverse is not true.  Windows with inet_ntop might not enable
	 * IPv6.
	 */
#if DXX_USE_IPv6
#error "IPv6 requires inet_ntop; SConf should prevent this path"
#endif
	dbuf.back() = 0;
	/*
	 * Copy the formatted string to the local buffer `dbuf` to guard
	 * against concurrent uses of `dxx_ntop`.
	 */
	return reinterpret_cast<const char *>(memcpy(dbuf.data(), inet_ntoa(sa.sin.sin_addr), dbuf.size() - 1));
#endif
#else
	/*
	 * Not Windows; assume inet_ntop present.  Non-Windows platforms
	 * declare inet_ntop with a const qualifier, so take a pointer to
	 * the underlying data.
	 */
	const auto addr =
#if DXX_USE_IPv6
		(sa.sa.sa_family == AF_INET6)
		? &sa.sin6.sin6_addr
		:
#endif
		static_cast<const void *>(&sa.sin.sin_addr);
#endif
#if !defined(WIN32) || defined(DXX_HAVE_INET_NTOP)
	if (const auto r = inet_ntop(sa.sa.sa_family, addr, dbuf.data(), dbuf.size()))
		return r;
	return "address";
#endif
}

network_state get_effective_netgame_status(const d_level_unique_control_center_state &LevelUniqueControlCenterState)
{
	if (Network_status == network_state::endlevel)
		return network_state::endlevel;
	if (LevelUniqueControlCenterState.Control_center_destroyed)
		return network_state::endlevel;
	if (Netgame.PlayTimeAllowed.count())
	{
		const auto TicksPlayTimeRemaining = Netgame.PlayTimeAllowed - ThisLevelTime;
		if (TicksPlayTimeRemaining.count() < i2f(30))
			return network_state::endlevel;
	}
	return Netgame.game_status;
}

static std::array<RAIIsocket, 2> UDP_Socket;

void net_udp_flush(std::array<RAIIsocket, 2> &UDP_Socket);

static bool operator==(const _sockaddr &l, const _sockaddr &r)
{
	return !memcmp(&l, &r, sizeof(l));
}

static bool operator!=(const _sockaddr &l, const _sockaddr &r)
{
	return !(l == r);
}

template <std::size_t Extent, std::size_t N>
[[nodiscard]]
static std::size_t copy_from_ntstring(const std::span<uint8_t, Extent> buf, const std::size_t len, const ntstring<N> &in)
{
	return in.copy_out(buf.subspan(len, N));
}

template <std::size_t N>
static void copy_to_ntstring(const uint8_t *const buf, uint_fast32_t &len, ntstring<N> &out)
{
	uint_fast32_t c = out.copy_if(reinterpret_cast<const char *>(&buf[len]), N);
	if (c < N)
		++ c;
	len += c;
}

static void reset_UDP_MyPort()
{
	UDP_MyPort = CGameArg.MplUdpMyPort >= 1024 ? CGameArg.MplUdpMyPort : UDP_PORT_DEFAULT;
}

static bool convert_text_portstring(const std::array<char, 6> &portstring, uint16_t &outport, bool allow_privileged, bool silent)
{
	char *porterror;
	unsigned long myport = strtoul(portstring.data(), &porterror, 10);
	if (*porterror || static_cast<uint16_t>(myport) != myport || (!allow_privileged && myport < 1024))
	{
		if (!silent)
			nm_messagebox(menu_title{TXT_ERROR}, {TXT_OK}, "Illegal port \"%s\"", portstring.data());
		return false;
	}
	else
		outport = myport;
	return true;
}

/* General UDP functions - START */
ssize_t dxx_sendto(const int sockfd, const csocket_data_buffer msg, const int flags, const csockaddr_ref to)
{
	ssize_t rv = sendto(sockfd, reinterpret_cast<const char *>(msg.data()), msg.size(), flags, &to.sa, to.len);

	UDP_num_sendto++;
	if (rv > 0)
		UDP_len_sendto += rv;

	return rv;
}

ssize_t dxx_recvfrom(const int sockfd, const socket_data_buffer msg, const int flags, sockaddr_ref from)
{
	ssize_t rv = recvfrom(sockfd, reinterpret_cast<char *>(msg.data()), msg.size(), flags, &from.sa, &from.len);

	UDP_num_recvfrom++;
	UDP_len_recvfrom += rv;

	return rv;
}

static game_info_request_result net_udp_check_game_info_request(const upid_rspan<upid::game_info_lite_req> data, std::integral_constant<upid, upid::game_info_lite_req>)
{
	if (const auto sender_major_version = GET_INTEL_SHORT(&(data[5])); sender_major_version != DXX_VERSION_MAJORi)
		return game_info_request_result::version_mismatch;
	if (const auto sender_minor_version = GET_INTEL_SHORT(&(data[7])); sender_minor_version != DXX_VERSION_MINORi)
		return game_info_request_result::version_mismatch;
	if (const auto sender_micro_version = GET_INTEL_SHORT(&(data[9])); sender_micro_version != DXX_VERSION_MICROi)
		return game_info_request_result::version_mismatch;
	return game_info_request_result::accept;
}

[[nodiscard]]
static game_info_request_result net_udp_check_game_info_request(const upid_rspan<upid::game_info_req> data, std::integral_constant<upid, upid::game_info_req>)
{
	{
		if (const auto sender_proto_version = GET_INTEL_SHORT(&data[11]); sender_proto_version != MULTI_PROTO_VERSION)
			return game_info_request_result::version_mismatch;
	}
	return net_udp_check_game_info_request(data.template first<upid_length<upid::game_info_lite_req>>(), std::integral_constant<upid, upid::game_info_lite_req>());
}

static void net_udp_send_version_deny(const _sockaddr &sender_addr)
{
	dxx_sendto(UDP_Socket[0], udp_response_version_deny, 0, sender_addr);
}

[[nodiscard]]
static std::optional<kick_player_reason> build_kick_player_reason_from_untrusted(const uint8_t why)
{
	switch (why)
	{
		case static_cast<uint8_t>(kick_player_reason::closed):
		case static_cast<uint8_t>(kick_player_reason::full):
		case static_cast<uint8_t>(kick_player_reason::endlevel):
		case static_cast<uint8_t>(kick_player_reason::dork):
		case static_cast<uint8_t>(kick_player_reason::aborted):
		case static_cast<uint8_t>(kick_player_reason::connected):
		case static_cast<uint8_t>(kick_player_reason::level):
		case static_cast<uint8_t>(kick_player_reason::kicked):
		case static_cast<uint8_t>(kick_player_reason::pkttimeout):
			return static_cast<kick_player_reason>(why);
		default:
			return std::nullopt;
	}
}

static std::optional<upid> build_upid_from_untrusted(const uint8_t cmd)
{
	switch (cmd)
	{
		case static_cast<uint8_t>(upid::version_deny):
		case static_cast<uint8_t>(upid::game_info_req):
		case static_cast<uint8_t>(upid::game_info):
		case static_cast<uint8_t>(upid::game_info_lite_req):
		case static_cast<uint8_t>(upid::game_info_lite):
		case static_cast<uint8_t>(upid::dump):
		case static_cast<uint8_t>(upid::addplayer):
		case static_cast<uint8_t>(upid::request):
		case static_cast<uint8_t>(upid::quit_joining):
		case static_cast<uint8_t>(upid::sync):
		case static_cast<uint8_t>(upid::object_data):
		case static_cast<uint8_t>(upid::ping):
		case static_cast<uint8_t>(upid::pong):
		case static_cast<uint8_t>(upid::endlevel_h):
		case static_cast<uint8_t>(upid::endlevel_c):
		case static_cast<uint8_t>(upid::pdata):
		case static_cast<uint8_t>(upid::mdata_pnorm):
		case static_cast<uint8_t>(upid::mdata_pneedack):
		case static_cast<uint8_t>(upid::mdata_ack):
#if DXX_USE_TRACKER
		case static_cast<uint8_t>(upid::tracker_gameinfo):
		case static_cast<uint8_t>(upid::tracker_ack):
		case static_cast<uint8_t>(upid::tracker_holepunch):
#endif
			return static_cast<upid>(cmd);
		default:
			return std::nullopt;
	}
}

static void net_udp_process_version_deny(const upid_rspan<upid::version_deny> data, const _sockaddr &)
{
	Netgame.protocol.udp.program_iver[0] = GET_INTEL_SHORT(&data[1]);
	Netgame.protocol.udp.program_iver[1] = GET_INTEL_SHORT(&data[3]);
	Netgame.protocol.udp.program_iver[2] = GET_INTEL_SHORT(&data[5]);
	Netgame.protocol.udp.program_iver[3] = GET_INTEL_SHORT(&data[7]);
	Netgame.protocol.udp.valid = -1;
}

}
}

namespace {

static void udp_traffic_stat()
{
	static fix64 last_traf_time = 0;

	if (timer_query() >= last_traf_time + F1_0)
	{
		last_traf_time = timer_query();
		con_printf(CON_DEBUG, "P#%u TRAFFIC - OUT: %fKB/s %iPPS IN: %fKB/s %iPPS",Player_num, static_cast<float>(UDP_len_sendto)/1024, UDP_num_sendto, static_cast<float>(UDP_len_recvfrom)/1024, UDP_num_recvfrom);
		UDP_num_sendto = UDP_len_sendto = UDP_num_recvfrom = UDP_len_recvfrom = 0;
	}
}

// Resolve address
int udp_dns_filladdr(sockaddr_ref addr, const char *host, uint16_t port, bool numeric_only, bool silent)
{
#ifdef DXX_HAVE_GETADDRINFO
	// Variables
	addrinfo hints{};
	char sPort[6];

	// Build the port
	snprintf(sPort, 6, "%hu", port);
	
	// Uncomment the following if we want ONLY what we compile for
	hints.ai_family = _sockaddr::address_family;
	// We are always UDP
	hints.ai_socktype = SOCK_DGRAM;
#ifdef AI_NUMERICSERV
	hints.ai_flags |= AI_NUMERICSERV;
#endif
#if DXX_USE_IPv6
	hints.ai_flags |= AI_V4MAPPED | AI_ALL;
#endif
	// Numeric address only?
	if (numeric_only)
		hints.ai_flags |= AI_NUMERICHOST;
	
	// Resolve the domain name
	RAIIaddrinfo result;
	if (result.getaddrinfo(host, sPort, &hints) != 0)
	{
		con_printf( CON_URGENT, "udp_dns_filladdr (getaddrinfo) failed for host %s", host );
		if (!silent)
			nm_messagebox(menu_title{TXT_ERROR}, {TXT_OK}, "Could not resolve address\n%s", host);
		addr.sa.sa_family = AF_UNSPEC;
		return -1;
	}
	
	if (result->ai_addrlen > addr.len)
	{
		con_printf(CON_URGENT, "Address too big for host %s", host);
		if (!silent)
			nm_messagebox(menu_title{TXT_ERROR}, {TXT_OK}, "Address too big for host\n%s", host);
		addr.sa.sa_family = AF_UNSPEC;
		return -1;
	}
	// Now copy it over
	memcpy(&addr.sa, result->ai_addr, addr.len = result->ai_addrlen);
	
	/* WARNING:  NERDY CONTENT
	 *
	 * The above works, since result->ai_addr contains the socket family,
	 * which is copied into our struct.  Our struct will be read for sendto
	 * and recvfrom, using the sockaddr.sa_family member.  If we are IPv6,
	 * this already has enough space to read into.  If we are IPv4, we will
	 * not be able to get any IPv6 connections anyway, so we will be safe
	 * from an overflow.  The more you know, 'cause knowledge is power!
	 *
	 * -- Matt
	 */
	
	// Free memory
#else
	(void)numeric_only;
	sockaddr_in &sai = reinterpret_cast<sockaddr_in &>(addr.sa);
	if (addr.len < sizeof(sai))
		return -1;
	const auto he = gethostbyname(host);
	if (!he)
	{
		con_printf(CON_URGENT, "udp_dns_filladdr (gethostbyname) failed for host %s", host);
		if (!silent)
			nm_messagebox(menu_title{TXT_ERROR}, {TXT_OK}, "Could not resolve IPv4 address\n%s", host);
		addr.sa.sa_family = AF_UNSPEC;
		return -1;
	}
	sai = {};
	sai.sin_family = ai_family;
	sai.sin_port = htons(port);
	sai.sin_addr = *reinterpret_cast<const in_addr *>(he->h_addr);
#endif
	return 0;
}

// Open socket
static int udp_open_socket(RAIIsocket &sock, int port)
{
	int bcast = 1;

	// close stale socket
	struct _sockaddr sAddr;   // my address information

	sock = RAIIsocket(sAddr.address_family, SOCK_DGRAM, 0);
	if (!sock)
	{
		con_printf(CON_URGENT,"udp_open_socket: socket creation failed (port %i)", port);
		nm_messagebox(menu_title{TXT_ERROR}, {TXT_OK}, "Port: %i\nCould not create socket.", port);
		return -1;
	}
	sAddr = {};
	sAddr.sa.sa_family = sAddr.address_family;
#if DXX_USE_IPv6
	sAddr.sin6.sin6_port = htons (port); // short, network byte order
	sAddr.sin6.sin6_addr = IN6ADDR_ANY_INIT; // automatically fill with my IP
#else
	sAddr.sin.sin_port = htons (port); // short, network byte order
	sAddr.sin.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
#endif
	
	if (bind(sock, &sAddr.sa, sizeof(sAddr)) < 0)
	{      
		con_printf(CON_URGENT,"udp_open_socket: bind name to socket failed (port %i)", port);
		nm_messagebox(menu_title{TXT_ERROR}, {TXT_OK}, "Port: %i\nCould not bind name to socket.", port);
		sock.reset();
		return -1;
	}
#ifdef _WIN32
	setsockopt(sock, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char *>(&bcast), sizeof(bcast));
#else
	setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &bcast, sizeof(bcast));
#endif
	return 0;
}

#ifndef MSG_DONTWAIT
static int udp_general_packet_ready(const SOCKET sock)
{
	fd_set set;
	struct timeval tv;

	FD_ZERO(&set);
	FD_SET(sock, &set);
	tv.tv_sec = tv.tv_usec = 0;
	if (select(sock + 1, &set, NULL, NULL, &tv) > 0)
		return 1;
	else
		return 0;
}
#endif

// Gets some text. Returns 0 if nothing on there.
static ssize_t udp_receive_packet(RAIIsocket &sock, const socket_data_buffer msg, const sockaddr_ref sender_addr)
{
	if (!sock)
		return -1;
#ifndef MSG_DONTWAIT
	if (!udp_general_packet_ready(sock))
		return 0;
#endif
	ssize_t msglen;
		int flags = 0;
#ifdef MSG_DONTWAIT
		flags |= MSG_DONTWAIT;
#endif
		msglen = dxx_recvfrom(sock, msg, flags, sender_addr);

		if (msglen < 0)
			return 0;

	if (msglen < msg.size())
		msg[msglen] = 0;
	return msglen;
}
/* General UDP functions - END */

struct direct_join
{
	enum class connect_type : uint8_t
	{
		idle,
		connecting,
		request_join,
	};
	struct _sockaddr host_addr;
	fix64 start_time, last_time;
	connect_type connecting = connect_type::idle;
#if DXX_USE_TRACKER
	tracker_game_id gameid;
#endif
};

struct manual_join_user_inputs
{
	std::array<char, 6> hostportbuf, guestportbuf;
	std::array<char, 128> hostaddrbuf;
};

struct manual_join_menu_items : direct_join, manual_join_user_inputs
{
	enum {
		label_host_address,
		input_host_address,
		label_host_port,
		input_host_port,
		label_guest_port,
		input_guest_port,
		label_status_text,
	};
	static manual_join_user_inputs s_last_inputs;
	std::array<newmenu_item, 7> m;
	manual_join_menu_items()
	{
		if (s_last_inputs.hostaddrbuf[0])
			hostaddrbuf = s_last_inputs.hostaddrbuf;
		else
			snprintf(hostaddrbuf.data(), hostaddrbuf.size(), "%s", CGameArg.MplUdpHostAddr.c_str());
		if (s_last_inputs.hostportbuf[0])
			hostportbuf = s_last_inputs.hostportbuf;
		else
			snprintf(hostportbuf.data(), hostportbuf.size(), "%hu", CGameArg.MplUdpHostPort ? CGameArg.MplUdpHostPort : UDP_PORT_DEFAULT);
		if (s_last_inputs.guestportbuf[0])
			guestportbuf = s_last_inputs.guestportbuf;
		else
			snprintf(guestportbuf.data(), guestportbuf.size(), "%hu", UDP_MyPort);
		nm_set_item_text(m[label_host_address], "GAME ADDRESS OR HOSTNAME:");
		nm_set_item_text(m[label_host_port], "GAME PORT:");
		nm_set_item_text(m[label_guest_port], "MY PORT:");
		nm_set_item_text(m[label_status_text], "");
		nm_set_item_input(m[input_host_address], hostaddrbuf);
		nm_set_item_input(m[input_host_port], hostportbuf);
		nm_set_item_input(m[input_guest_port], guestportbuf);
	}
};

struct manual_join_menu : manual_join_menu_items, newmenu
{
	manual_join_menu(grs_canvas &src) :
		newmenu(menu_title{nullptr}, menu_subtitle{"ENTER GAME ADDRESS"}, menu_filename{nullptr}, tiny_mode_flag::normal, tab_processing_flag::ignore, adjusted_citem::create(m, input_host_address), src)
	{
	}
	virtual window_event_result event_handler(const d_event &event) override;
};

struct netgame_list_game_menu_items
{
	enum
	{
		header_rows = 4,
		non_game_rows = header_rows + 1,
		menuitem_count = UDP_NETGAMES_PPAGE + non_game_rows,
	};
	std::array<newmenu_item, menuitem_count> menus;
	std::array<std::array<char, 92>, menuitem_count - non_game_rows> ljtext;
	netgame_list_game_menu_items()
	{
#if DXX_USE_TRACKER
#define DXX_NETGAME_LIST_SCAN_STRING	"\tF4/F5/F6: (Re)Scan for all/LAN/Tracker Games."
#else
#define DXX_NETGAME_LIST_SCAN_STRING	"\tF4: (Re)Scan for LAN Games."
#endif
		nm_set_item_text(menus[0], DXX_NETGAME_LIST_SCAN_STRING);
#undef DXX_NETGAME_LIST_SCAN_STRING
		nm_set_item_text(menus[1], "\tPgUp/PgDn: Flip Pages.");
		nm_set_item_text(menus[2], "");
		nm_set_item_text(menus[3], "\tGAME \tMODE \t#PLYRS \tMISSION \tLEV \tSTATUS");

		for (auto &&[i, lj, mi] : enumerate(zip(ljtext, unchecked_partial_range(menus, header_rows + 0u, menus.size() - 1)), 1u))
		{
			snprintf(lj.data(), lj.size(), "%u.                                                                      ", i);
			nm_set_item_menu(mi, lj.data());
		}
		nm_set_item_text(menus.back(), "\t");
	}
};

struct netgame_list_game_menu;

netgame_list_game_menu *netgame_list_menu;

struct netgame_list_game_menu : netgame_list_game_menu_items, direct_join, newmenu
{
	unsigned num_active_udp_games = 0;
	uint8_t num_active_udp_changed = 1;
	std::array<UDP_netgame_info_lite, UDP_MAX_NETGAMES> Active_udp_games{};
	netgame_list_game_menu(grs_canvas &src) :
		newmenu(menu_title{"NETGAMES"}, menu_subtitle{nullptr}, menu_filename{nullptr}, tiny_mode_flag::tiny, tab_processing_flag::process, adjusted_citem::create(menus, 0), src)
	{
		assert(!netgame_list_menu);
		netgame_list_menu = this;
	}
	~netgame_list_game_menu()
	{
		assert(netgame_list_menu == this);
		netgame_list_menu = nullptr;
	}
	virtual window_event_result event_handler(const d_event &event) override;
};

manual_join_user_inputs manual_join_menu_items::s_last_inputs;

void net_udp_send_game_info(csockaddr_ref sender_addr, const _sockaddr *player_address, const upid info_upid);

}

namespace dsx {
namespace {

void net_udp_request_game_info(const csockaddr_ref game_addr, const std::array<uint8_t, upid_length<upid::game_info_req>> &buf = udp_request_game_info_template<upid::game_info_lite_req>)
{
	dxx_sendto(UDP_Socket[0], buf, 0, game_addr);
}

direct_join::connect_type net_udp_show_game_info(const netgame_info &Netgame);

// Connect to a game host and get full info. Eventually we join!
static int net_udp_game_connect(direct_join *const dj)
{
	// Get full game info so we can show it.

	// Timeout after 10 seconds
	if (timer_query() >= dj->start_time + (F1_0*10))
	{
		dj->connecting = direct_join::connect_type::idle;
		typename _sockaddr::presentation_buffer dbuf;
		const auto port =
#if DXX_USE_IPv6
			dj->host_addr.sa.sa_family == AF_INET6
			? dj->host_addr.sin6.sin6_port
			:
#endif
			dj->host_addr.sin.sin_port;
		nm_messagebox(menu_title{TXT_ERROR}, {TXT_OK},
"No response by host.\n\n\
Possible reasons:\n\
* No game on %s (anymore)\n\
* Host port %hu is not open\n\
* Game is hosted on a different port\n\
* Host uses a game version\n\
  I do not understand", dxx_ntop(dj->host_addr, dbuf), ntohs(port));
		return 0;
	}
	
	if (Netgame.protocol.udp.valid == -1)
	{
		nm_messagebox(menu_title{TXT_ERROR}, {TXT_OK}, "Version mismatch! Cannot join Game.\n\nHost game version: %i.%i.%i\nHost game protocol: %i\n(%s)\n\nYour game version: " DXX_VERSION_STR "\nYour game protocol: %i\n(%s)", Netgame.protocol.udp.program_iver[0], Netgame.protocol.udp.program_iver[1], Netgame.protocol.udp.program_iver[2], Netgame.protocol.udp.program_iver[3], (Netgame.protocol.udp.program_iver[3]==0?"RELEASE VERSION":"DEVELOPMENT BUILD, BETA, etc."), MULTI_PROTO_VERSION, (MULTI_PROTO_VERSION==0?"RELEASE VERSION":"DEVELOPMENT BUILD, BETA, etc."));
		dj->connecting = direct_join::connect_type::idle;
		return 0;
	}
	
	if (timer_query() >= dj->last_time + F1_0)
	{
		net_udp_request_game_info(dj->host_addr, udp_request_game_info_template<upid::game_info_req>);
#if DXX_USE_TRACKER
		if (const auto g = dj->gameid; g != tracker_game_id{})
			if (timer_query() >= dj->start_time + (F1_0*4))
				udp_tracker_request_holepunch(g);
#endif
		dj->last_time = timer_query();
	}
	timer_delay2(5);
	net_udp_listen();

	if (Netgame.protocol.udp.valid != 1)
		return 0;		// still trying to connect

	if (dj->connecting == direct_join::connect_type::connecting)
	{
		// show info menu and check if we join
		const auto connecting = net_udp_show_game_info(Netgame);
		dj->connecting = connecting;
		if (connecting == direct_join::connect_type::request_join)
		{
			// Get full game info again as it could have changed since we entered the info menu.
			Netgame.protocol.udp.valid = 0;
			dj->start_time = timer_query();
		}
		return 0;
	}
	dj->connecting = direct_join::connect_type::idle;

	return net_udp_do_join_game();
}

}

}

window_event_result manual_join_menu::event_handler(const d_event &event)
{
	switch (event.type)
	{
		case event_type::key_command:
			if (connecting != direct_join::connect_type::idle && event_key_get(event) == KEY_ESC)
			{
				connecting = direct_join::connect_type::idle;
				nm_set_item_text(m[label_status_text], "");
				return window_event_result::handled;
			}
			break;
		case event_type::idle:
			if (connecting != direct_join::connect_type::idle)
			{
				if (net_udp_game_connect(this))
					return window_event_result::close;	// Success!
				else if (connecting == direct_join::connect_type::idle)
					nm_set_item_text(m[label_status_text], "");
			}
			break;

		case event_type::newmenu_selected:
		{
			int sockres = -1;

			net_udp_init(); // yes, redundant call but since the menu does not know any better it would allow any IP entry as long as Netgame-entry looks okay... my head hurts...
			if (!convert_text_portstring(guestportbuf, UDP_MyPort, false, false))
				return window_event_result::handled;
			sockres = udp_open_socket(UDP_Socket[0], UDP_MyPort);
			if (sockres != 0)
				return window_event_result::handled;
			uint16_t hostport;
			if (!convert_text_portstring(hostportbuf, hostport, true, false))
				return window_event_result::handled;
			// Resolve address
			if (udp_dns_filladdr(host_addr, hostaddrbuf.data(), hostport, false, false) < 0)
				return window_event_result::handled;
			else
			{
				s_last_inputs = *this;
				multi_new_game();
				N_players = 0;
				change_playernum_to(1);
				start_time = timer_query();
				last_time = 0;
				
				Netgame.players[0].protocol.udp.addr = host_addr;
				connecting = direct_join::connect_type::connecting;
				nm_set_item_text(m[label_status_text], "Connecting...");
				return window_event_result::handled;
			}
		}
		case event_type::window_close:
			if (!Game_wind) // they cancelled
				net_udp_close();
			break;
		default:
			break;
	}
	return newmenu::event_handler(event);
}

void net_udp_manual_join_game()
{
	net_udp_init();

	reset_UDP_MyPort();

	auto menu = window_create<manual_join_menu>(grd_curscreen->sc_canvas);
	(void)menu;
}

namespace {

static void copy_truncate_string(const grs_font &cv_font, const font_x_scaled_float strbound, std::array<char, 25> &out, const ntstring<25> &in)
{
	size_t k = 0, x = 0;
	char thold[2];
	thold[1] = 0;
	const std::size_t outsize = out.size();
	range_for (const char c, in)
	{
		if (unlikely(c == '\t'))
			continue;
		if (unlikely(!c))
			break;
		thold[0] = c;
		const auto tx = gr_get_string_size(cv_font, thold).width;
		if ((x += tx) >= strbound)
		{
			const std::size_t outbound = outsize - 4;
			if (k > outbound)
				k = outbound;
			out[k] = out[k + 1] = out[k + 2] = '.';
			k += 3;
			break;
		}
		out[k++] = c;
		if (k >= outsize - 1)
			break;
	}
	out[k] = 0;
}

window_event_result netgame_list_game_menu::event_handler(const d_event &event)
{
	// Polling loop for Join Game menu
	int newpage = 0;
	static int NLPage = 0;
	switch (event.type)
	{
		case event_type::window_activated:
		{
			Netgame.protocol.udp.valid = 0;
			Active_udp_games = {};
			num_active_udp_changed = 1;
			num_active_udp_games = 0;
			net_udp_request_game_info(GBcast);
#if DXX_USE_IPv6
			net_udp_request_game_info(GMcast_v6);
#endif
#if DXX_USE_TRACKER
			udp_tracker_reqgames();
#endif
			if (connecting == direct_join::connect_type::idle) // fallback/failsafe!
				nm_set_item_text(menus[UDP_NETGAMES_PPAGE+4], "\t");
			break;
		}
		case event_type::idle:
			if (connecting != direct_join::connect_type::idle)
			{
				if (net_udp_game_connect(this))
					return window_event_result::close;	// Success!
				if (connecting == direct_join::connect_type::idle) // connect wasn't successful - get rid of the message.
					nm_set_item_text(menus[UDP_NETGAMES_PPAGE+4], "\t");
			}
			break;
		case event_type::key_command:
		{
			int key = event_key_get(event);
			if (key == KEY_PAGEUP)
			{
				NLPage--;
				newpage++;
				if (NLPage < 0)
					NLPage = UDP_NETGAMES_PAGES-1;
				key = 0;
				break;
			}
			if (key == KEY_PAGEDOWN)
			{
				NLPage++;
				newpage++;
				if (NLPage >= UDP_NETGAMES_PAGES)
					NLPage = 0;
				key = 0;
				break;
			}
			if( key == KEY_F4 )
			{
				// Empty the list
				Active_udp_games = {};
				num_active_udp_changed = 1;
				num_active_udp_games = 0;
				
				// Request LAN games
				net_udp_request_game_info(GBcast);
#if DXX_USE_IPv6
				net_udp_request_game_info(GMcast_v6);
#endif
#if DXX_USE_TRACKER
				udp_tracker_reqgames();
#endif
				// All done
				break;
			}
#if DXX_USE_TRACKER
			if (key == KEY_F5)
			{
				Active_udp_games = {};
				num_active_udp_changed = 1;
				num_active_udp_games = 0;
				net_udp_request_game_info(GBcast);

#if DXX_USE_IPv6
				net_udp_request_game_info(GMcast_v6);
#endif
				break;
			}

			if( key == KEY_F6 )
			{
				// Zero the list
				Active_udp_games = {};
				num_active_udp_changed = 1;
				num_active_udp_games = 0;
				
				// Request from the tracker
				udp_tracker_reqgames();
				
				// Break off
				break;
			}
#endif
			if (key == KEY_ESC)
			{
				if (connecting != direct_join::connect_type::idle)
				{
					connecting = direct_join::connect_type::idle;
					nm_set_item_text(menus[UDP_NETGAMES_PPAGE+4], "\t");
					return window_event_result::handled;
				}
				break;
			}
			break;
		}
		case event_type::newmenu_selected:
		{
			auto &citem = static_cast<const d_select_event &>(event).citem;
			if (((citem+(NLPage*UDP_NETGAMES_PPAGE)) >= 4) && (((citem+(NLPage*UDP_NETGAMES_PPAGE))-3) <= num_active_udp_games))
			{
				multi_new_game();
				N_players = 0;
				change_playernum_to(1);
				start_time = timer_query();
				last_time = 0;
				host_addr = Active_udp_games[(citem+(NLPage*UDP_NETGAMES_PPAGE))-4].game_addr;
				Netgame.players[0].protocol.udp.addr = host_addr;
				connecting = direct_join::connect_type::connecting;
#if DXX_USE_TRACKER
				gameid = Active_udp_games[(citem+(NLPage*UDP_NETGAMES_PPAGE))-4].TrackerGameID;
#endif
				nm_set_item_text(menus[UDP_NETGAMES_PPAGE+4], "\tConnecting. Please wait...");
			}
			else
			{
				window_create<passive_messagebox>(menu_title{TXT_SORRY}, menu_subtitle{TXT_INVALID_CHOICE}, TXT_OK, grd_curscreen->sc_canvas);
				// invalid game selected - stay in the menu
			}
			return window_event_result::handled;
		}
		case event_type::window_close:
		{
			if (!Game_wind)
			{
				net_udp_close();
				Network_status = network_state::menu;	// they cancelled
			}
			return window_event_result::ignored;
		}
		default:
			break;
	}

	net_udp_listen();

	if (!num_active_udp_changed && !newpage)
		return newmenu::event_handler(event);

	num_active_udp_changed = 0;

	// Copy the active games data into the menu options
	for (int i = 0; i < UDP_NETGAMES_PPAGE; i++)
	{
		const auto &augi = Active_udp_games[(i + (NLPage * UDP_NETGAMES_PPAGE))];
		int nplayers = 0;
		char levelname[8];

		if ((i+(NLPage*UDP_NETGAMES_PPAGE)) >= num_active_udp_games)
		{
			auto &p = ljtext[i];
			snprintf(p.data(), p.size(), "%d.                                                                      ", (i + (NLPage * UDP_NETGAMES_PPAGE)) + 1);
			continue;
		}

		// These next two loops protect against menu skewing
		// if missiontitle or gamename contain a tab

		const auto &&fspacx = FSPACX();
		const auto &cv_font = *grd_curcanv->cv_font;
		std::array<char, 25> MissName, GameName;
		const auto &&fspacx55 = fspacx(55);
		copy_truncate_string(cv_font, fspacx55, MissName, augi.mission_title);
		copy_truncate_string(cv_font, fspacx55, GameName, augi.game_name);

		nplayers = augi.numconnected;

		const int levelnum = augi.levelnum;
		if (levelnum < 0)
		{
			cf_assert(-levelnum < MAX_SECRET_LEVELS_PER_MISSION);
			snprintf(levelname, sizeof(levelname), "S%d", -levelnum);
		}
		else
		{
			cf_assert(levelnum < MAX_LEVELS_PER_MISSION);
			snprintf(levelname, sizeof(levelname), "%d", levelnum);
		}

		const char *status;
		if (const auto game_status = augi.game_status; game_status == network_state::starting)
			status = "FORMING ";
		else if (game_status == network_state::playing)
		{
			if (augi.RefusePlayers)
				status = "RESTRICT";
			else if ((augi.game_flag & netgame_rule_flags::closed) != netgame_rule_flags::None)
				status = "CLOSED  ";
			else
				status = "OPEN    ";
		}
		else
			status = "BETWEEN ";
		
		const auto gamemode = underlying_value(augi.gamemode);
		auto &p = ljtext[i];
		snprintf(p.data(), p.size(), "%d.\t%.24s \t%.7s \t%3u/%u \t%.24s \t %s \t%s", (i + (NLPage * UDP_NETGAMES_PPAGE)) + 1, GameName.data(), (gamemode < std::size(GMNamesShrt)) ? GMNamesShrt[gamemode] : "INVALID", nplayers, augi.max_numplayers, MissName.data(), levelname, status);
	}
	return newmenu::event_handler(event);
}

}

void net_udp_list_join_game(grs_canvas &canvas)
{
	net_udp_init();
	const auto gamemyport = CGameArg.MplUdpMyPort;
	if (udp_open_socket(UDP_Socket[0], gamemyport >= 1024 ? gamemyport : UDP_PORT_DEFAULT) < 0)
		return;

	if (gamemyport >= 1024 && gamemyport != UDP_PORT_DEFAULT)
		if (udp_open_socket(UDP_Socket[1], UDP_PORT_DEFAULT) < 0)
			nm_messagebox_str(menu_title{TXT_WARNING}, nm_messagebox_tie(TXT_OK), menu_subtitle{"Cannot open default port!\nYou can only scan for games\nmanually."});

	change_playernum_to(1);
	N_players = 0;
	Network_send_objects = 0;
	Network_sending_extras=0;
	Network_rejoined=0;

	Network_status = network_state::browsing; // We are looking at a game menu

	net_udp_flush(UDP_Socket);
	net_udp_listen();  // Throw out old info

	gr_set_fontcolor(canvas, BM_XRGB(15, 15, 23),-1);

	auto menu = window_create<netgame_list_game_menu>(canvas);
	(void)menu;
}

namespace {

static void net_udp_send_sequence_packet(const UDP_sequence_request_packet &seq, const _sockaddr &recv_addr)
{
	std::array<uint8_t, upid_length<upid::request>> buf;
	int len = 0;
	buf[0] = underlying_value(seq.type);						len++;
	memcpy(&buf[len], seq.callsign.operator const char *(), CALLSIGN_LEN+1);		len += CALLSIGN_LEN+1;
	buf[len] = seq.Current_level_num;				len++;
	buf[len] = underlying_value(seq.rank);					len++;
	dxx_sendto(UDP_Socket[0], buf, 0, recv_addr);
}

static void net_udp_send_sequence_packet(const UDP_sequence_addplayer_packet &seq, const _sockaddr &recv_addr)
{
	std::array<uint8_t, upid_length<upid::addplayer>> buf;
	int len = 0;
	buf[0] = underlying_value(seq.type);						len++;
	memcpy(&buf[len], seq.callsign.operator const char *(), CALLSIGN_LEN+1);		len += CALLSIGN_LEN+1;
	buf[len] = seq.player_num;				len++;
	buf[len] = underlying_value(seq.rank);					len++;
	dxx_sendto(UDP_Socket[0], buf, 0, recv_addr);
}

static auto build_udp_sequence_from_untrusted(const std::span<const uint8_t, upid_length<upid::request>> untrusted)
{
	callsign_t callsign;
	/* Skip over the first byte, since it is the UPID code */
	std::size_t position = 1;
	callsign.copy_lower(std::span<const char, CALLSIGN_LEN>(reinterpret_cast<const char *>(&untrusted[position]), CALLSIGN_LEN));
	position += CALLSIGN_LEN + 1;
	const netplayer_info::player_rank rank = build_rank_from_untrusted(untrusted[++position]);
	return std::tuple(callsign, rank);
}

UDP_sequence_request_packet UDP_sequence_request_packet::build_from_untrusted(const upid_rspan<upid::request> untrusted)
{
	auto &&[callsign, rank] = build_udp_sequence_from_untrusted(untrusted.template first<upid_length<upid::request>>());
	const int8_t Current_level_num = untrusted[1 + CALLSIGN_LEN + 1];
	return {rank, callsign, Current_level_num};
}

UDP_sequence_addplayer_packet UDP_sequence_addplayer_packet::build_from_untrusted(const upid_rspan<upid::addplayer> untrusted)
{
	auto &&[callsign, rank] = build_udp_sequence_from_untrusted(untrusted.template first<upid_length<upid::addplayer>>());
	const int8_t player_num = untrusted[1 + CALLSIGN_LEN + 1];
	return {rank, callsign, player_num};
}

void net_udp_init()
{
	// So you want to play a netgame, eh?  Let's a get a few things straight

#ifdef _WIN32
{
	WORD wVersionRequested;
	WSADATA wsaData;
	wVersionRequested = MAKEWORD(2, 2);
	WSACleanup();
	if (WSAStartup( wVersionRequested, &wsaData))
		nm_messagebox_str(menu_title{TXT_ERROR}, nm_messagebox_tie(TXT_OK), menu_subtitle{"Cannot init Winsock!"}); // no break here... game will fail at socket creation anyways...
}
#endif

	Netgame = {};
	UDP_MData = {};
	net_udp_noloss_init_mdata_queue();
	UDP_sequence_request_packet UDP_Seq{GetMyNetRanking(), InterfaceUniqueState.PilotName, 0};

	multi_new_game();
	net_udp_flush(UDP_Socket);

#if DXX_USE_TRACKER
	// Initialize the tracker info
	udp_tracker_init();
#endif
}

void net_udp_close()
{
	UDP_Socket = {};
#ifdef _WIN32
	WSACleanup();
#endif
}

}

namespace dsx {
namespace multi {
namespace udp {

int dispatch_table::end_current_level(
#if defined(DXX_BUILD_DESCENT_I)
	next_level_request_secret_flag *const secret
#endif
	) const
{
	// Do whatever needs to be done between levels
#if defined(DXX_BUILD_DESCENT_I)
	{
		// We do not really check if a player has actually found a secret level... yeah, I am too lazy! So just go there and pretend we did!
		range_for (const auto i, unchecked_partial_range(Current_mission->secret_level_table.get(), Current_mission->n_secret_levels))
		{
			if (Current_level_num == i)
			{
				*secret = next_level_request_secret_flag::use_secret;
				break;
			}
		}
	}
#endif

	Network_status = network_state::endlevel; // We are between levels
	net_udp_listen();
	dispatch->send_endlevel_packet();

	range_for (auto &i, partial_range(Netgame.players, N_players)) 
	{
		i.LastPacketTime = timer_query();
	}
   
	dispatch->send_endlevel_packet();
	dispatch->send_endlevel_packet();

	net_udp_update_netgame();

	return(0);
}
}
}
}

namespace {
static join_netgame_status_code net_udp_can_join_netgame(const netgame_info *const game)
{
	// Can this player rejoin a netgame in progress?
	if (game->game_status == network_state::starting)
		return join_netgame_status_code::game_has_capacity;

	if (game->game_status != network_state::playing)
		return join_netgame_status_code::game_in_disallowed_state;

	// Game is in progress, figure out if this guy can re-join it

	const unsigned num_players = game->numplayers;

	if ((game->game_flag & netgame_rule_flags::closed) == netgame_rule_flags::None)
	{
		// Look for player that is not connected
		
		if (game->numconnected==game->max_numplayers)
			return join_netgame_status_code::game_is_full;
		if (game->RefusePlayers)
			return join_netgame_status_code::game_refuses_players;
		if (num_players < game->max_numplayers)
			return join_netgame_status_code::game_has_capacity;
		if (game->numconnected<num_players)
			return join_netgame_status_code::game_has_capacity;
	}

	// Search to see if we were already in this closed netgame in progress

	auto &plr = get_local_player();
	for (const auto i : xrange(num_players))
	{
		if (plr.callsign == game->players[i].callsign && i == game->protocol.udp.your_index)
			return join_netgame_status_code::game_has_capacity;
	}
	return join_netgame_status_code::game_in_disallowed_state;
}
}

namespace dsx {
namespace multi {
namespace udp {
// do UDP stuff to disconnect a player. Should ONLY be called from multi_disconnect_player()
void dispatch_table::disconnect_player(int playernum) const
{
	// A player has disconnected from the net game, take whatever steps are
	// necessary 

	if (playernum == Player_num) 
	{
		Int3(); // Weird, see Rob
		return;
	}

	if (VerifyPlayerJoined==playernum)
		VerifyPlayerJoined=-1;

	net_udp_noloss_clear_mdata_trace(playernum);
}
}
}

namespace {

static void net_udp_new_player(const UDP_sequence_addplayer_packet &their, const struct _sockaddr &peer_addr)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	const unsigned pnum = their.player_num;

	Assert(pnum < Netgame.max_numplayers);
	
	if (Newdemo_state == ND_STATE_RECORDING) {
		int new_player;

		if (pnum == N_players)
			new_player = 1;
		else
			new_player = 0;
		newdemo_record_multi_connect(pnum, new_player, their.callsign);
	}

	auto &plr = *vmplayerptr(pnum);
	plr.callsign = their.callsign;
	Netgame.players[pnum].callsign = their.callsign;
	Netgame.players[pnum].protocol.udp.addr = peer_addr;

	Netgame.players[pnum].rank = their.rank;

	plr.connected = player_connection_status::playing;
	kill_matrix[pnum] = {};
	auto &objp = *vmobjptr(plr.objnum);
	auto &player_info = objp.ctype.player_info;
	player_info.net_killed_total = 0;
	player_info.net_kills_total = 0;
	player_info.mission.score = 0;
	player_info.powerup_flags = {};
	player_info.KillGoalCount = 0;

	if (pnum == N_players)
	{
		N_players++;
		Netgame.numplayers = N_players;
	}

	digi_play_sample(SOUND_HUD_MESSAGE, F1_0);

	const auto &&rankstr = GetRankStringWithSpace(their.rank);
	HUD_init_message(HM_MULTI, "%s%s'%s' %s", rankstr.first, rankstr.second, their.callsign.operator const char *(), TXT_JOINING);
	
	multi_make_ghost_player(pnum);

	multi_send_score();
#if defined(DXX_BUILD_DESCENT_II)
	multi_sort_kill_list();
#endif

	net_udp_noloss_clear_mdata_trace(pnum);
}
}
}

namespace {

static void net_udp_welcome_player(const UDP_sequence_request_packet &their, const struct _sockaddr &udp_addr)
{
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	// Add a player to a game already in progress
	WaitForRefuseAnswer=0;

	// Don't accept new players if we're ending this level.  Its safe to
	// ignore since they'll request again later

	if (Network_status == network_state::endlevel || LevelUniqueControlCenterState.Control_center_destroyed)
	{
		multi::udp::dispatch->kick_player(udp_addr, kick_player_reason::endlevel);
		return; 
	}

	if (Network_send_objects || Network_sending_extras)
	{
		// Ignore silently, we're already responding to someone and we can't
		// do more than one person at a time.  If we don't dump them they will
		// re-request in a few seconds.
		return;
	}

	// Joining a running game will need quite a few packets on the mdata-queue, so let players only join if we have enough space.
	if (Netgame.PacketLossPrevention)
		if ((UDP_MDATA_STOR_QUEUE_SIZE - UDP_mdata_queue_highest) < UDP_MDATA_STOR_MIN_FREE_2JOIN)
			return;

	if (their.Current_level_num != Current_level_num)
	{
		multi::udp::dispatch->kick_player(udp_addr, kick_player_reason::level);
		return;
	}

	unsigned player_num = UINT_MAX;
	UDP_sync_player = {};
	network_player_added = Network_player_added::_0;

	for (unsigned i = 0; i < N_players; i++)
	{
		if (vcplayerptr(i)->callsign == their.callsign &&
			udp_addr == Netgame.players[i].protocol.udp.addr)
		{
			player_num = i;
			break;
		}
	}

	if (player_num == UINT_MAX)
	{
		// Player is new to this game

		if ((Netgame.game_flag & netgame_rule_flags::closed) != netgame_rule_flags::None)
		{
			// Slots are open but game is closed
			multi::udp::dispatch->kick_player(udp_addr, kick_player_reason::closed);
			return;
		}
		if (N_players < Netgame.max_numplayers)
		{
			// Add player in an open slot, game not full yet

			player_num = N_players;
			network_player_added = Network_player_added::_1;
		}
		else
		{
			// Slots are full but game is open, see if anyone is
			// disconnected and replace the oldest player with this new one
		
			int oldest_player = -1;
			fix64 oldest_time = timer_query();
			int activeplayers = 0;

			Assert(N_players == Netgame.max_numplayers);

			range_for (auto &i, partial_const_range(Netgame.players, Netgame.numplayers))
				if (i.connected != player_connection_status::disconnected)
					activeplayers++;

			if (activeplayers == Netgame.max_numplayers)
			{
				// Game is full.
				multi::udp::dispatch->kick_player(udp_addr, kick_player_reason::full);
				return;
			}

			for (unsigned i = 0; i < N_players; i++)
			{
				if (vcplayerptr(i)->connected == player_connection_status::disconnected && Netgame.players[i].LastPacketTime < oldest_time)
				{
					oldest_time = Netgame.players[i].LastPacketTime;
					oldest_player = i;
				}
			}

			if (oldest_player == -1)
			{
				// Everyone is still connected 
				multi::udp::dispatch->kick_player(udp_addr, kick_player_reason::full);
				return;
			}
			else
			{
				// Found a slot!

				player_num = oldest_player;
				network_player_added = Network_player_added::_1;
			}
		}
	}
	else
	{
		// Player is reconnecting
		
		auto &plr = *vcplayerptr(player_num);
		if (plr.connected != player_connection_status::disconnected)
		{
			return;
		}

		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_multi_reconnect(player_num);

		digi_play_sample(SOUND_HUD_MESSAGE, F1_0);

		const auto &&rankstr = GetRankStringWithSpace(Netgame.players[player_num].rank);
		HUD_init_message(HM_MULTI, "%s%s'%s' %s", rankstr.first, rankstr.second, static_cast<const char *>(plr.callsign), TXT_REJOIN);

		multi_send_score();

		net_udp_noloss_clear_mdata_trace(player_num);
	}

	auto &obj = *vmobjptr(vcplayerptr(player_num)->objnum);
	auto &player_info = obj.ctype.player_info;
	player_info.KillGoalCount = 0;

	// Send updated Objects data to the new/returning player

	UDP_sync_player = UDP_sequence_syncplayer_packet(player_num, their.rank, their.callsign, udp_addr);
	Network_send_objects = 1;
	Network_send_objnum = -1;
	Netgame.players[player_num].LastPacketTime = timer_query();

	net_udp_send_objects(network_player_added);
}
}

namespace dsx {
namespace multi {
namespace udp {
int dispatch_table::objnum_is_past(const objnum_t objnum) const
{
	// determine whether or not a given object number has already been sent
	// to a re-joining player.
	
	int player_num = UDP_sync_player.player_num;
	int obj_mode = !((object_owner[objnum] == -1) || (object_owner[objnum] == player_num));

	if (!Network_send_objects)
		return 0; // We're not sending objects to a new player

	if (obj_mode > Network_send_object_mode)
		return 0;
	else if (obj_mode < Network_send_object_mode)
		return 1;
	else if (objnum < Network_send_objnum)
		return 1;
	else
		return 0;
}

}
}

namespace {
#if defined(DXX_BUILD_DESCENT_I)
static void net_udp_send_door_updates(void)
{
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcwallptridx = Walls.vcptridx;
	// Send door status when new player joins
	range_for (const auto &&p, vcwallptridx)
	{
		auto &w = *p;
		if ((w.type == WALL_DOOR && (w.state == wall_state::opening || w.state == wall_state::waiting)) || (w.type == WALL_BLASTABLE && (w.flags & wall_flag::blasted)))
			multi_send_door_open(w.segnum, w.sidenum, {});
		else if (w.type == WALL_BLASTABLE && w.hps != WALL_HPS)
			multi_send_hostage_door_status(p);
	}

}
#elif defined(DXX_BUILD_DESCENT_II)
static void net_udp_send_door_updates(const playernum_t pnum)
{
	// Send door status when new player joins
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcwallptridx = Walls.vcptridx;
	range_for (const auto &&p, vcwallptridx)
	{
		auto &w = *p;
		if ((w.type == WALL_DOOR && (w.state == wall_state::opening || w.state == wall_state::waiting || w.state == wall_state::open)) || (w.type == WALL_BLASTABLE && (w.flags & wall_flag::blasted)))
			multi_send_door_open_specific(pnum,w.segnum, w.sidenum,w.flags);
		else if (w.type == WALL_BLASTABLE && w.hps != WALL_HPS)
			multi_send_hostage_door_status(p);
		else
			multi_send_wall_status_specific(pnum,p,w.type,w.flags,w.state);
	}
}
#endif
}

}

namespace {
static void net_udp_process_monitor_vector(uint32_t vector)
{
	auto &Effects = LevelUniqueEffectsClipState.Effects;
	auto &TmapInfo = LevelUniqueTmapInfoState.TmapInfo;
	if (!vector)
		return;
	range_for (unique_segment &seg, vmsegptr)
	{
		range_for (auto &j, seg.sides)
		{
			const auto tm = j.tmap_num2;
			if (tm == texture2_value::None)
				continue;
			const auto ec = TmapInfo[get_texture_index(tm)].eclip_num;
			if (ec == eclip_none)
				continue;
			const int bm = Effects[ec].dest_bm_num;
			if (bm == ~0u)
				continue;
			{
				if (vector & 1)
				{
					j.tmap_num2 = build_texture2_value(bm, get_texture_rotation_high(tm));
				}
				if (!(vector >>= 1))
					return;
			}
		}
	}
}

class blown_bitmap_array
{
	typedef int T;
	using array_t = std::array<T, 32>;
	typedef array_t::const_iterator const_iterator;
	array_t a;
	array_t::iterator e = a.begin();
public:
	bool exists(T t) const
	{
		const_iterator ce = e;
		return std::find(a.begin(), ce, t) != ce;
	}
	void insert_unique(T t)
	{
		if (exists(t))
			return;
		if (e == a.end())
		{
			LevelError("too many blown bitmaps; ignoring bitmap %i.", t);
			return;
		}
		*e = t;
		++e;
	}
};

static unsigned net_udp_create_monitor_vector(void)
{
	auto &Effects = LevelUniqueEffectsClipState.Effects;
	auto &TmapInfo = LevelUniqueTmapInfoState.TmapInfo;
	blown_bitmap_array blown_bitmaps;
	constexpr size_t max_textures = Textures.size();
	range_for (auto &i, partial_const_range(Effects, Num_effects))
	{
		if (i.dest_bm_num < max_textures)
		{
			blown_bitmaps.insert_unique(i.dest_bm_num);
		}
	}
	unsigned monitor_num = 0;
	unsigned vector = 0;
	range_for (const auto &&seg, vcsegptridx)
	{
		range_for (auto &j, seg->unique_segment::sides)
		{
			const auto tm2 = j.tmap_num2;
			if (tm2 == texture2_value::None)
				continue;
			const auto masked_tm2 = get_texture_index(tm2);
			const unsigned ec = TmapInfo[masked_tm2].eclip_num;
			{
				if (ec != eclip_none &&
					Effects[ec].dest_bm_num != ~0u)
				{
				}
				else if (blown_bitmaps.exists(masked_tm2))
				{
					if (monitor_num >= 8 * sizeof(vector))
					{
						LevelError("too many blown monitors; ignoring segment %hu.", seg.get_unchecked_index());
						return vector;
					}
							vector |= (1 << monitor_num);
				}
				else
					continue;
				monitor_num++;
			}
		}
	}
	return(vector);
}

static void net_udp_stop_resync(const struct _sockaddr &udp_addr)
{
	if (UDP_sync_player.udp_addr == udp_addr)
	{
		Network_send_objects = 0;
		Network_sending_extras=0;
		Network_rejoined=0;
		Player_joining_extras=-1;
		Network_send_objnum = -1;
	}
}

}

namespace dsx {
namespace {

void net_udp_send_objects(const Network_player_added network_player_added)
{
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	uint8_t player_num = UDP_sync_player.player_num;
	static int obj_count = 0;
	unsigned loc = 0, obj_count_frame = 0;
	static fix64 last_send_time = 0;
	
	if (last_send_time + (F1_0/50) > timer_query())
		return;
	last_send_time = timer_query();

	// Send clear objects array trigger and send player num

	Assert(Network_send_objects != 0);
	Assert(player_num >= 0);
	Assert(player_num < Netgame.max_numplayers);

	if (Network_status == network_state::endlevel || LevelUniqueControlCenterState.Control_center_destroyed)
	{
		// Endlevel started before we finished sending the goods, we'll
		// have to stop and try again after the level.
		multi::udp::dispatch->kick_player(UDP_sync_player.udp_addr, kick_player_reason::endlevel);
		Network_send_objects = 0; 
		return;
	}

	std::array<uint8_t, UPID_MAX_SIZE> object_buffer{{
		underlying_value(upid::object_data)
	}};
	loc = 5;

	if (Network_send_objnum == -1)
	{
		obj_count = 0;
		Network_send_object_mode = 0;
		PUT_INTEL_INT(&object_buffer[loc], -1);                       loc += 4;
		object_buffer[loc] = player_num;                            loc += 1;
		/* Placeholder for remote_objnum, not used here */          loc += 4;
		Network_send_objnum = 0;
		obj_count_frame = 1;
	}
	
	objnum_t i;
	for (i = Network_send_objnum; i <= Highest_object_index; i++)
	{
		const auto &&objp = vmobjptr(i);
		if ((objp->type != OBJ_POWERUP) && (objp->type != OBJ_PLAYER) &&
				(objp->type != OBJ_CNTRLCEN) && (objp->type != OBJ_GHOST) &&
				(objp->type != OBJ_ROBOT) && (objp->type != OBJ_HOSTAGE)
#if defined(DXX_BUILD_DESCENT_II)
				&& !(objp->type == OBJ_WEAPON && get_weapon_id(objp) == weapon_id_type::PMINE_ID)
#endif
				)
			continue;
		if ((Network_send_object_mode == 0) && ((object_owner[i] != -1) && (object_owner[i] != player_num)))
			continue;
		if ((Network_send_object_mode == 1) && ((object_owner[i] == -1) || (object_owner[i] == player_num)))
			continue;

		if ( loc + sizeof(object_rw) + 9 > UPID_MAX_SIZE-1 )
			break; // Not enough room for another object

		obj_count_frame++;
		obj_count++;

		const auto &&[owner, remote_objnum] = objnum_local_to_remote(i);
		Assert(owner == object_owner[i]);

		PUT_INTEL_INT(&object_buffer[loc], i);                        loc += 4;
		object_buffer[loc] = owner;                                 loc += 1;
		PUT_INTEL_INT(&object_buffer[loc], remote_objnum);            loc += 4;
		// use object_rw to send objects for now. if object sometime contains some day contains something useful the client should know about, we should use it. but by now it's also easier to use object_rw because then we also do not need fix64 timer values.
		multi_object_to_object_rw(vmobjptr(i), reinterpret_cast<object_rw *>(&object_buffer[loc]));
		loc += sizeof(object_rw);
	}

	if (obj_count_frame) // Send any objects we've buffered
	{
		Network_send_objnum = i;
		PUT_INTEL_INT(&object_buffer[1], obj_count_frame);

		Assert(loc <= UPID_MAX_SIZE);

		dxx_sendto(UDP_Socket[0], std::span(object_buffer).first(loc), 0, UDP_sync_player.udp_addr);
	}

	if (i > Highest_object_index)
	{
		if (Network_send_object_mode == 0)
		{
			Network_send_objnum = 0;
			Network_send_object_mode = 1; // go to next mode
		}
		else 
		{
			Assert(Network_send_object_mode == 1); 

			// Send count so other side can make sure he got them all
			object_buffer[0] = underlying_value(upid::object_data);
			PUT_INTEL_INT(&object_buffer[1], 1);
			PUT_INTEL_INT(&object_buffer[5], network_checksum_marker_object);
			object_buffer[9] = player_num;
			PUT_INTEL_INT(&object_buffer[10], obj_count);
			dxx_sendto(UDP_Socket[0], std::span(object_buffer).first<14>(), 0, UDP_sync_player.udp_addr);

			// Send sync packet which tells the player who he is and to start!
			net_udp_send_rejoin_sync(network_player_added, player_num);

			// Turn off send object mode
			Network_send_objnum = -1;
			Network_send_objects = 0;
			obj_count = 0;

#if defined(DXX_BUILD_DESCENT_I)
			Network_sending_extras=3; // start to send extras
#elif defined(DXX_BUILD_DESCENT_II)
			Network_sending_extras=9; // start to send extras
#endif
			VerifyPlayerJoined = Player_joining_extras = player_num;

			return;
		} // mode == 1;
	} // i > Highest_object_index
}

}
}

namespace {

static int net_udp_verify_objects(int remote, int local)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	int nplayers = 0;

	if ((remote-local) > 10)
		return(2);

	for (auto &obj : vcobjptr)
	{
		if (obj.type == OBJ_PLAYER || obj.type == OBJ_GHOST)
			nplayers++;
	}

	if (Netgame.max_numplayers<=nplayers)
		return(0);

	return(1);
}

static void net_udp_read_object_packet(const d_level_shared_robot_info_state &LevelSharedRobotInfoState, const uint8_t *const data)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	auto &vmobjptridx = Objects.vmptridx;
	// Object from another net player we need to sync with
	sbyte obj_owner;
	static int mode = 0, object_count = 0, my_pnum = 0;
	unsigned loc{5};
	const int nobj{GET_INTEL_INT<int32_t>(&data[1])};

	for (int i = 0; i < nobj; i++)
	{
		const unsigned uobjnum{GET_INTEL_INT(&data[loc])};
		objnum_t objnum = uobjnum;                         loc += 4;
		obj_owner = data[loc];                                      loc += 1;
		const int remote_objnum{GET_INTEL_INT<int32_t>(&data[loc])};                  loc += 4;

		if (objnum == object_none) 
		{
			// Clear object array
			init_objects();
			Network_rejoined = 1;
			my_pnum = obj_owner;
			change_playernum_to(my_pnum);
			mode = 1;
			object_count = 0;
		}
		else if (uobjnum == network_checksum_marker_object)
		{
			// Special debug checksum marker for entire send
			if (mode == 1)
			{
				special_reset_objects(LevelUniqueObjectState, Robot_info);
				mode = 0;
			}
			if (remote_objnum != object_count) {
				Int3();
			}
			if (net_udp_verify_objects(remote_objnum, object_count))
			{
				// Failed to sync up 
				nm_messagebox_str(menu_title{nullptr}, nm_messagebox_tie(TXT_OK), menu_subtitle{TXT_NET_SYNC_FAILED});
				Network_status = network_state::menu;                          
				return;
			}
		}
		else 
		{
			object_count++;
			if ((obj_owner == my_pnum) || (obj_owner == -1)) 
			{
				if (mode != 1)
					Int3(); // SEE ROB
				objnum = remote_objnum;
			}
			else {
				if (mode == 1)
				{
					special_reset_objects(LevelUniqueObjectState, Robot_info);
					mode = 0;
				}
				objnum = obj_allocate(LevelUniqueObjectState);
			}
			if (objnum != object_none) {
				auto obj = vmobjptridx(objnum);
				if (obj->type != OBJ_NONE)
				{
					obj_unlink(Objects.vmptr, Segments.vmptr, obj);
					Assert(obj->segnum == segment_none);
				}
				Assert(objnum < MAX_OBJECTS);
				multi_object_rw_to_object(reinterpret_cast<const object_rw *>(&data[loc]), obj);
				loc += sizeof(object_rw);
				auto segnum = obj->segnum;
				if (segnum != segment_none)
				{
					obj_link_unchecked(Objects.vmptr, obj, Segments.vmptridx(segnum));
				}
				if (obj_owner == my_pnum) 
					map_objnum_local_to_local(objnum);
				else if (obj_owner != -1)
					map_objnum_local_to_remote(objnum, remote_objnum, obj_owner);
				else
					object_owner[objnum] = -1;
			}
		} // For a standard onbject
	} // For each object in packet
}

}

namespace dsx {
namespace {

void net_udp_send_rejoin_sync(const Network_player_added network_player_added, const unsigned player_num)
{
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	vmplayerptr(player_num)->connected = player_connection_status::playing;	// connect the new guy
	Netgame.players[player_num].LastPacketTime = timer_query();

	if (Network_status == network_state::endlevel || LevelUniqueControlCenterState.Control_center_destroyed)
	{
		// Endlevel started before we finished sending the goods, we'll
		// have to stop and try again after the level.

		multi::udp::dispatch->kick_player(UDP_sync_player.udp_addr, kick_player_reason::endlevel);

		Network_send_objects = 0; 
		Network_sending_extras=0;
		return;
	}

	if (network_player_added != Network_player_added::_0)
	{
		const UDP_sequence_addplayer_packet add(UDP_sync_player.rank, UDP_sync_player.callsign, UDP_sync_player.player_num);
		net_udp_new_player(add, UDP_sync_player.udp_addr);

		for (unsigned i = 0; i < N_players; ++i)
		{
			if (i != player_num && i != Player_num && vcplayerptr(i)->connected != player_connection_status::disconnected)
				net_udp_send_sequence_packet(add, Netgame.players[i].protocol.udp.addr);
		}
	}

	// Send sync packet to the new guy

	net_udp_update_netgame();

	// Fill in the kill list
	Netgame.kills = kill_matrix;
	for (unsigned j = 0; j < MAX_PLAYERS; ++j)
	{
		auto &objp = *vcobjptr(vcplayerptr(j)->objnum);
		auto &player_info = objp.ctype.player_info;
		Netgame.killed[j] = player_info.net_killed_total;
		Netgame.player_kills[j] = player_info.net_kills_total;
		Netgame.player_score[j] = player_info.mission.score;
	}

	Netgame.level_time = get_local_player().time_level;
	Netgame.monitor_vector = net_udp_create_monitor_vector();

	net_udp_send_game_info(UDP_sync_player.udp_addr, &UDP_sync_player.udp_addr, upid::sync);
#if defined(DXX_BUILD_DESCENT_I)
	net_udp_send_door_updates();
#endif

	return;
}

template <upid id>
[[nodiscard]]
static game_info_request_result net_udp_check_game_info_request(const upid_rspan<id> data)
{
	if (memcmp(&data[1], UDP_REQ_ID, 4))
		return game_info_request_result::id_mismatch;
	return net_udp_check_game_info_request(data, std::integral_constant<upid, id>());
}

}
}

namespace {

static void net_udp_resend_sync_due_to_packet_loss()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	if (!multi_i_am_master())
		return;

	net_udp_update_netgame();

	// Fill in the kill list
	Netgame.kills = kill_matrix;
	for (unsigned j = 0; j < MAX_PLAYERS; ++j)
	{
		auto &objp = *vcobjptr(vcplayerptr(j)->objnum);
		auto &player_info = objp.ctype.player_info;
		Netgame.killed[j] = player_info.net_killed_total;
		Netgame.player_kills[j] = player_info.net_kills_total;
		Netgame.player_score[j] = player_info.mission.score;
	}

	Netgame.level_time = get_local_player().time_level;
	Netgame.monitor_vector = net_udp_create_monitor_vector();

	net_udp_send_game_info(UDP_sync_player.udp_addr, &UDP_sync_player.udp_addr, upid::sync);
}

static void net_udp_add_player(const UDP_sequence_request_packet &p, const struct _sockaddr &udp_addr)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	range_for (auto &i, partial_range(Netgame.players, N_players))
	{
		if (i.protocol.udp.addr == udp_addr)
		{
			i.LastPacketTime = timer_query();
			return;		// already got them
		}
	}

	if ( N_players >= MAX_PLAYERS )
	{
		return;		// too many of em
	}

	Netgame.players[N_players].callsign = p.callsign;
	Netgame.players[N_players].protocol.udp.addr = udp_addr;
	Netgame.players[N_players].rank = p.rank;
	Netgame.players[N_players].connected = player_connection_status::playing;
	auto &obj = *vmobjptr(vcplayerptr(N_players)->objnum);
	auto &player_info = obj.ctype.player_info;
	player_info.KillGoalCount = 0;
	vmplayerptr(N_players)->connected = player_connection_status::playing;
	Netgame.players[N_players].LastPacketTime = timer_query();
	N_players++;
	Netgame.numplayers = N_players;

	net_udp_send_netgame_update();
}

// One of the players decided not to join the game

static void net_udp_remove_player(const struct _sockaddr &udp_addr)
{
	const auto &&ngp_range = partial_range(Netgame.players, N_players);
	for (auto &&iter = ngp_range.begin(), &&end = ngp_range.end(); iter != end; ++iter)
	{
		auto &ngp = *iter;
		if (ngp.protocol.udp.addr != udp_addr)
			continue;
		std::move(std::next(iter), end, iter);
		--N_players;
		Netgame.numplayers = N_players;
		net_udp_send_netgame_update();
		return;
	}
}

}

namespace dsx {
namespace multi {
namespace udp {
void dispatch_table::kick_player(const _sockaddr &dump_addr, kick_player_reason why) const
{
	// Inform player that he was not chosen for the netgame
	const std::array<uint8_t, upid_length<upid::dump>> buf{{
		underlying_value(upid::dump),
		underlying_value(why)
	}};
	dxx_sendto(UDP_Socket[0], buf, 0, dump_addr);
	if (multi_i_am_master())
		for (playernum_t i = 1; i < N_players; i++)
			if (dump_addr == Netgame.players[i].protocol.udp.addr)
				multi_disconnect_player(i);
}
}
}

namespace {

void net_udp_update_netgame()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	// Update the netgame struct with current game variables
	Netgame.numconnected=0;
	range_for (auto &i, partial_const_range(Players, N_players))
		if (i.connected != player_connection_status::disconnected)
			Netgame.numconnected++;

#if defined(DXX_BUILD_DESCENT_II)
// This is great: D2 1.0 and 1.1 ignore upper part of the game_flags field of
//	the lite_info struct when you're sitting on the join netgame screen.  We can
//	"sneak" Hoard information into this field.  This is better than sending 
//	another packet that could be lost in transit.

	if (HoardEquipped())
	{
		const auto is_hoard_game{game_mode_hoard(Game_mode)};
		const auto game_flag_no_hoard{Netgame.game_flag & ~(netgame_rule_flags::hoard | netgame_rule_flags::team_hoard)};
		Netgame.game_flag = is_hoard_game
			? (
				(Game_mode & GM_TEAM)
				? (game_flag_no_hoard | netgame_rule_flags::hoard | netgame_rule_flags::team_hoard)
				: (game_flag_no_hoard | netgame_rule_flags::hoard)
			)
			: game_flag_no_hoard;
	}
#endif
	if (Network_status == network_state::starting)
		return;

	Netgame.numplayers = N_players;
	Netgame.game_status = Network_status;

	Netgame.kills = kill_matrix;
	for (unsigned i = 0; i < MAX_PLAYERS; ++i) 
	{
		auto &plr = *vcplayerptr(i);
		Netgame.players[i].connected = plr.connected;
		auto &objp = *vcobjptr(plr.objnum);
		auto &player_info = objp.ctype.player_info;
		Netgame.killed[i] = player_info.net_killed_total;
		Netgame.player_kills[i] = player_info.net_kills_total;
#if defined(DXX_BUILD_DESCENT_II)
		Netgame.player_score[i] = player_info.mission.score;
#endif
		Netgame.net_player_flags[i] = player_info.powerup_flags;
	}
	Netgame.team_kills = team_kills;
	Netgame.levelnum = Current_level_num;
}

}

namespace multi {
namespace udp {
/* Send an updated endlevel status to everyone (if we are host) or host (if we are client)  */
void dispatch_table::send_endlevel_packet() const
{
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	auto &vmobjptr = Objects.vmptr;
	int len = 0;

	if (multi_i_am_master())
	{
		std::array<uint8_t, 2 + (5 * Players.size()) + sizeof(kill_matrix)> buf;
		buf[len] = underlying_value(upid::endlevel_h);											len++;
		buf[len] = LevelUniqueControlCenterState.Countdown_seconds_left;				len++;

		range_for (auto &i, Players)
		{
			buf[len] = underlying_value(i.connected);								len++;
			auto &objp = *vcobjptr(i.objnum);
			auto &player_info = objp.ctype.player_info;
			PUT_INTEL_SHORT(&buf[len], player_info.net_kills_total);
			len += 2;
			PUT_INTEL_SHORT(&buf[len], player_info.net_killed_total);
			len += 2;
		}

		range_for (auto &i, kill_matrix)
		{
			range_for (auto &j, i)
			{
				PUT_INTEL_SHORT(&buf[len], j);				len += 2;
			}
		}

		for (unsigned i = 1; i < MAX_PLAYERS; ++i)
			if (vcplayerptr(i)->connected != player_connection_status::disconnected)
				dxx_sendto(UDP_Socket[0], buf, 0, Netgame.players[i].protocol.udp.addr);
	}
	else
	{
		std::array<uint8_t, 8 + sizeof(kill_matrix[0])> buf;
		buf[len] = underlying_value(upid::endlevel_c);											len++;
		buf[len] = Player_num;												len++;
		buf[len] = underlying_value(get_local_player().connected);							len++;
		buf[len] = LevelUniqueControlCenterState.Countdown_seconds_left;				len++;
		auto &player_info = get_local_plrobj().ctype.player_info;
		PUT_INTEL_SHORT(&buf[len], player_info.net_kills_total);
		len += 2;
		PUT_INTEL_SHORT(&buf[len], player_info.net_killed_total);
		len += 2;

		range_for (auto &i, kill_matrix[Player_num])
		{
			PUT_INTEL_SHORT(&buf[len], i);
			len += 2;
		}

		dxx_sendto(UDP_Socket[0], buf, 0, Netgame.players[0].protocol.udp.addr);
	}
}
}
}

namespace {

struct game_info_light
{
	std::array<uint8_t, upid_length<upid::game_info_lite>> buf;
};

struct game_info_heavy
{
	std::array<uint8_t, upid_length<upid::game_info>> buf;
};

[[nodiscard]]
static std::span<const uint8_t> net_udp_prepare_light_game_info(const d_level_unique_control_center_state &LevelUniqueControlCenterState, const netgame_info &Netgame, const std::span<uint8_t, upid_length<upid::game_info_lite>> buf)
{
	std::size_t len = 0;
	buf[0] = underlying_value(upid::game_info_lite);								len++;				// 1
	PUT_INTEL_SHORT(&buf[len], DXX_VERSION_MAJORi); 						len += 2;			// 3
	PUT_INTEL_SHORT(&buf[len], DXX_VERSION_MINORi); 						len += 2;			// 5
	PUT_INTEL_SHORT(&buf[len], DXX_VERSION_MICROi); 						len += 2;			// 7
	PUT_INTEL_INT(&buf[len], Netgame.protocol.udp.GameID);				len += 4;			// 11
	PUT_INTEL_INT(&buf[len], Netgame.levelnum);					len += 4;
	buf[len] = underlying_value(Netgame.gamemode);							len++;
	buf[len] = Netgame.RefusePlayers;						len++;
	buf[len] = underlying_value(Netgame.difficulty);							len++;
	const auto tmpvar = get_effective_netgame_status(LevelUniqueControlCenterState);
	buf[len] = underlying_value(tmpvar);								len++;
	buf[len] = Netgame.numconnected;						len++;
	buf[len] = Netgame.max_numplayers;						len++;
	buf[len] = underlying_value(Netgame.game_flag);							len++;
	len += copy_from_ntstring(buf, len, Netgame.game_name);
	len += copy_from_ntstring(buf, len, Netgame.mission_title);
	len += copy_from_ntstring(buf, len, Netgame.mission_name);
	return buf.first(len);
}

[[nodiscard]]
static std::span<const uint8_t> net_udp_prepare_heavy_game_info(const d_level_unique_control_center_state &LevelUniqueControlCenterState, const netgame_info &Netgame, const _sockaddr *const entry_addr, const upid info_upid, game_info_heavy &info)
{
	const std::span buf = info.buf;
	std::size_t len = 0;

	buf[0] = underlying_value(info_upid);								len++;
	PUT_INTEL_SHORT(&buf[len], DXX_VERSION_MAJORi); 						len += 2;
	PUT_INTEL_SHORT(&buf[len], DXX_VERSION_MINORi); 						len += 2;
	PUT_INTEL_SHORT(&buf[len], DXX_VERSION_MICROi); 						len += 2;
	auto &your_index = buf[len++];
	your_index = MULTI_PNUM_UNDEF;
	auto addr = entry_addr;
	for (const auto &&[i, np] : enumerate(Netgame.players))
	{
		memcpy(&buf[len], &np.callsign[0u], CALLSIGN_LEN+1); 	len += CALLSIGN_LEN+1;
		buf[len] = underlying_value(np.connected);				len++;
		buf[len] = underlying_value(np.rank);					len++;
		if (addr && *addr == np.protocol.udp.addr)
		{
			addr = nullptr;
			your_index = i;
		}
	}
	PUT_INTEL_INT(&buf[len], Netgame.levelnum);					len += 4;
	buf[len] = underlying_value(Netgame.gamemode);							len++;
	buf[len] = Netgame.RefusePlayers;						len++;
	buf[len] = underlying_value(Netgame.difficulty);							len++;
	const auto tmpvar = get_effective_netgame_status(LevelUniqueControlCenterState);
	buf[len] = underlying_value(tmpvar);								len++;
	buf[len] = Netgame.numplayers;							len++;
	buf[len] = Netgame.max_numplayers;						len++;
	buf[len] = Netgame.numconnected;						len++;
	buf[len] = underlying_value(Netgame.game_flag);							len++;
	buf[len] = Netgame.team_vector;							len++;
	PUT_INTEL_INT(&buf[len], underlying_value(Netgame.AllowedItems));					len += 4;
	/* In cooperative games, never shuffle. */
	PUT_INTEL_INT(&buf[len], (Game_mode & GM_MULTI_COOP) ? 0 : Netgame.ShufflePowerupSeed);			len += 4;
	buf[len] = Netgame.SecludedSpawns;			len += 1;
#if defined(DXX_BUILD_DESCENT_I)
	buf[len] = Netgame.SpawnGrantedItems.mask;			len += 1;
	buf[len] = Netgame.DuplicatePowerups.get_packed_field();			len += 1;
#elif defined(DXX_BUILD_DESCENT_II)
	PUT_INTEL_SHORT(&buf[len], Netgame.SpawnGrantedItems.mask);			len += 2;
	PUT_INTEL_SHORT(&buf[len], Netgame.DuplicatePowerups.get_packed_field());			len += 2;
	buf[len++] = Netgame.Allow_marker_view;
	buf[len++] = Netgame.AlwaysLighting;
	buf[len++] = Netgame.ThiefModifierFlags;
	buf[len++] = Netgame.AllowGuidebot;
#endif
	buf[len++] = Netgame.ShowEnemyNames;
	buf[len++] = Netgame.BrightPlayers;
	buf[len++] = Netgame.InvulAppear;
	for (auto &i : Netgame.team_name)
	{
		memcpy(&buf[len], i.operator const char *(), (CALLSIGN_LEN+1));
		len += CALLSIGN_LEN + 1;
	}
	for (auto &i : Netgame.locations)
	{
		PUT_INTEL_INT(&buf[len], i);				len += 4;
	}
	for (auto &i : Netgame.kills)
	{
		for (auto &j : i)
		{
			PUT_INTEL_SHORT(&buf[len], j);		len += 2;
		}
	}
	PUT_INTEL_SHORT(&buf[len], Netgame.segments_checksum);			len += 2;
	PUT_INTEL_SHORT(&buf[len], Netgame.team_kills[team_number::blue]);				len += 2;
	PUT_INTEL_SHORT(&buf[len], Netgame.team_kills[team_number::red]);				len += 2;
	for (auto &i : Netgame.killed)
	{
		PUT_INTEL_SHORT(&buf[len], i);				len += 2;
	}
	for (auto &i : Netgame.player_kills)
	{
		PUT_INTEL_SHORT(&buf[len], i);			len += 2;
	}
	PUT_INTEL_INT(&buf[len], Netgame.KillGoal);					len += 4;
	PUT_INTEL_INT(&buf[len], Netgame.PlayTimeAllowed.count());				len += 4;
	PUT_INTEL_INT(&buf[len], Netgame.level_time);					len += 4;
	PUT_INTEL_INT(&buf[len], Netgame.control_invul_time);				len += 4;
	PUT_INTEL_INT(&buf[len], Netgame.monitor_vector);				len += 4;
	for (auto &i : Netgame.player_score)
	{
		PUT_INTEL_INT(&buf[len], i);			len += 4;
	}
	for (auto &i : Netgame.net_player_flags)
	{
		buf[len] = static_cast<uint8_t>(i.get_player_flags());
		len++;
	}
	PUT_INTEL_SHORT(&buf[len], Netgame.PacketsPerSec);				len += 2;
	buf[len] = Netgame.PacketLossPrevention;					len++;
	buf[len] = Netgame.NoFriendlyFire;						len++;
	buf[len] = Netgame.MouselookFlags;						len++;
	buf[len] = Netgame.PitchLockFlags;                      len++;
	len += copy_from_ntstring(buf, len, Netgame.game_name);
	len += copy_from_ntstring(buf, len, Netgame.mission_title);
	len += copy_from_ntstring(buf, len, Netgame.mission_name);
	return buf.first(len);
}

}
}

namespace {

void net_udp_send_game_info(csockaddr_ref sender_addr, const _sockaddr *player_address, const upid info_upid)
{
	// Send game info to someone who requested it
	net_udp_update_netgame(); // Update the values in the netgame struct
	union {
		game_info_light light;
		game_info_heavy heavy;
	};
	const auto buf = (info_upid == upid::game_info_lite)
		? net_udp_prepare_light_game_info(LevelUniqueObjectState.ControlCenterState, Netgame, light.buf)
		: net_udp_prepare_heavy_game_info(LevelUniqueObjectState.ControlCenterState, Netgame, player_address, info_upid, heavy);
	dxx_sendto(UDP_Socket[0], buf, 0, sender_addr);
}

static unsigned MouselookMPFlag(const unsigned game_is_cooperative)
{
	return game_is_cooperative ? MouselookMode::MPCoop : MouselookMode::MPAnarchy;
}

static void net_udp_broadcast_game_info(const upid info_upid)
{
	net_udp_send_game_info(GBcast, nullptr, info_upid);
#if DXX_USE_IPv6
	net_udp_send_game_info(GMcast_v6, nullptr, info_upid);
#endif
}

/* Send game info to all players in this game. Also send lite_info for people watching the netlist */
void net_udp_send_netgame_update()
{
	for (unsigned i = 1; i < N_players; ++i)
	{
		if (vcplayerptr(i)->connected == player_connection_status::disconnected)
			continue;
		const auto &addr = Netgame.players[i].protocol.udp.addr;
		net_udp_send_game_info(addr, &addr, upid::game_info);
	}
	net_udp_broadcast_game_info(upid::game_info_lite);
}

static unsigned net_udp_send_request(void)
{
	// Send a request to join a game 'Netgame'.  Returns 0 if we can join this
	// game, non-zero if there is some problem.
	auto b = Netgame.players.begin();
	auto e = Netgame.players.end();
	const auto &&i{std::ranges::find_if(b, e, [](const netplayer_info &ni) { return ni.connected != player_connection_status::disconnected; })};
	if (i == e)
	{
		Assert(false);
		return std::distance(b, i);
	}
	int8_t l = Current_level_num;
	const UDP_sequence_request_packet UDP_Seq{GetMyNetRanking(), InterfaceUniqueState.PilotName, l};
	net_udp_send_sequence_packet(UDP_Seq, Netgame.players[0].protocol.udp.addr);
	return std::distance(b, i);
}

}

namespace dsx {
namespace {

static void net_udp_process_game_info_light(const std::span<const uint8_t> buf, const _sockaddr &game_addr
#if DXX_USE_TRACKER
									  , const tracker_game_id TrackerGameID = {}
#endif
									  )
{
	const auto data = buf.data();
	uint_fast32_t len = 0;
	{
		const auto menu = netgame_list_menu;
		if (!menu)
			return;
		UDP_netgame_info_lite recv_game;
		
		recv_game.game_addr = game_addr;
												len++; // skip UPID byte
		recv_game.program_iver[0] = GET_INTEL_SHORT(&(data[len]));			len += 2;
		recv_game.program_iver[1] = GET_INTEL_SHORT(&(data[len]));			len += 2;
		recv_game.program_iver[2] = GET_INTEL_SHORT(&(data[len]));			len += 2;
		
		if ((recv_game.program_iver[0] != DXX_VERSION_MAJORi) || (recv_game.program_iver[1] != DXX_VERSION_MINORi) || (recv_game.program_iver[2] != DXX_VERSION_MICROi))
			return;

		recv_game.GameID = GET_INTEL_INT(&(data[len]));					len += 4;
		recv_game.levelnum = GET_INTEL_INT(&(data[len]));				len += 4;
		recv_game.gamemode = network_game_type{data[len]};							len++;
		recv_game.RefusePlayers = data[len];						len++;
		recv_game.difficulty = data[len];						len++;
		const auto game_status = build_network_state_from_untrusted(data[len]);
		if (!game_status)
			return;
		recv_game.game_status = *game_status;						len++;
		recv_game.numconnected = data[len];						len++;
		recv_game.max_numplayers = data[len];						len++;
		recv_game.game_flag = netgame_rule_flags{data[len]};						len++;
		copy_to_ntstring(data, len, recv_game.game_name);
		copy_to_ntstring(data, len, recv_game.mission_title);
		copy_to_ntstring(data, len, recv_game.mission_name);
#if DXX_USE_TRACKER
		recv_game.TrackerGameID = TrackerGameID;
#endif
		menu->num_active_udp_changed = 1;
		auto r = partial_range(menu->Active_udp_games, menu->num_active_udp_games);
		const auto &&i{std::ranges::find_if(r, [&recv_game](const UDP_netgame_info_lite &g) { return !d_stricmp(g.game_name.data(), recv_game.game_name.data()) && g.GameID == recv_game.GameID; })};
		if (i == menu->Active_udp_games.end())
		{
			return;
		}
		*i = std::move(recv_game);
#if defined(DXX_BUILD_DESCENT_II)
		// See if this is really a Hoard game
		// If so, adjust all the data accordingly
		if (HoardEquipped())
		{
			if (const auto game_flag{i->game_flag}; (game_flag & netgame_rule_flags::hoard) != netgame_rule_flags::None)
			{
				i->gamemode = (game_flag & netgame_rule_flags::team_hoard) != netgame_rule_flags::None
					? network_game_type::team_hoard
					: network_game_type::hoard;
				i->game_status = (game_flag & netgame_rule_flags::really_endlevel) != netgame_rule_flags::None
					? network_state::endlevel
					: (
						(game_flag & netgame_rule_flags::really_forming) != netgame_rule_flags::None
						? network_state::starting
						: network_state::playing
					);
			}
		}
#endif
		if (i == r.end())
		{
			if (i->numconnected)
				++ menu->num_active_udp_games;
		}
		else if (!i->numconnected)
		{
			// Delete this game
			std::move(std::next(i), r.end(), i);
			-- menu->num_active_udp_games;
		}
	}
}

static void net_udp_process_game_info_heavy(const uint8_t *data, uint_fast32_t, const _sockaddr &game_addr)
{
	uint_fast32_t len = 0;
	{
		Netgame.players[0].protocol.udp.addr = game_addr;

												len++; // skip UPID byte
		Netgame.protocol.udp.program_iver[0] = GET_INTEL_SHORT(&(data[len]));		len += 2;
		Netgame.protocol.udp.program_iver[1] = GET_INTEL_SHORT(&(data[len]));		len += 2;
		Netgame.protocol.udp.program_iver[2] = GET_INTEL_SHORT(&(data[len]));		len += 2;
		Netgame.protocol.udp.your_index = data[len]; ++len;
		range_for (auto &i, Netgame.players)
		{
			i.callsign.copy_lower(std::span<const char, CALLSIGN_LEN>(reinterpret_cast<const char *>(&data[len]), CALLSIGN_LEN));
			len += CALLSIGN_LEN+1;
			i.connected = player_connection_status{data[len]};				len++;
			i.rank = build_rank_from_untrusted(data[len]);					len++;
		}
		Netgame.levelnum = GET_INTEL_INT(&(data[len]));					len += 4;
		Netgame.gamemode = network_game_type{data[len]};							len++;
		Netgame.RefusePlayers = data[len];						len++;
		Netgame.difficulty = cast_clamp_difficulty(data[len]);
		len++;
		const auto game_status = build_network_state_from_untrusted(data[len]);
		if (!game_status)
			return;
		Netgame.game_status = *game_status;						len++;
		Netgame.numplayers = data[len];							len++;
		Netgame.max_numplayers = data[len];						len++;
		Netgame.numconnected = data[len];						len++;
		Netgame.game_flag = netgame_rule_flags{data[len]};						len++;
		Netgame.team_vector = data[len];						len++;
		Netgame.AllowedItems = static_cast<netflag_flag>(GET_INTEL_INT(&data[len]));				len += 4;
		Netgame.ShufflePowerupSeed = GET_INTEL_INT(&(data[len]));		len += 4;
		Netgame.SecludedSpawns = data[len];		len += 1;
#if defined(DXX_BUILD_DESCENT_I)
		Netgame.SpawnGrantedItems = netgrant_flag{data[len]};		len += 1;
		Netgame.DuplicatePowerups.set_packed_field(data[len]);			len += 1;
#elif defined(DXX_BUILD_DESCENT_II)
		Netgame.SpawnGrantedItems = netgrant_flag{GET_INTEL_SHORT(&(data[len]))};		len += 2;
		Netgame.DuplicatePowerups.set_packed_field(GET_INTEL_SHORT(&data[len])); len += 2;
		if (unlikely(map_granted_flags_to_laser_level(Netgame.SpawnGrantedItems) > MAX_SUPER_LASER_LEVEL))
			/* Bogus input - reject whole entry */
			Netgame.SpawnGrantedItems = netgrant_flag::None;
		Netgame.Allow_marker_view = data[len++];
		Netgame.AlwaysLighting = data[len++];
		Netgame.ThiefModifierFlags = data[len++];
		Netgame.AllowGuidebot = data[len++];
#endif
		Netgame.ShowEnemyNames = data[len];				len += 1;
		Netgame.BrightPlayers = data[len];				len += 1;
		Netgame.InvulAppear = data[len];				len += 1;
		range_for (auto &i, Netgame.team_name)
		{
			i.copy(std::span<const char, CALLSIGN_LEN + 1>(reinterpret_cast<const char *>(&data[len]), CALLSIGN_LEN + 1));
			len += CALLSIGN_LEN + 1;
		}
		range_for (auto &i, Netgame.locations)
		{
			i = GET_INTEL_INT(&(data[len]));			len += 4;
		}
		range_for (auto &i, Netgame.kills)
		{
			range_for (auto &j, i)
			{
				j = GET_INTEL_SHORT(&(data[len]));		len += 2;
			}
		}
		Netgame.segments_checksum = GET_INTEL_SHORT(&(data[len]));			len += 2;
		Netgame.team_kills[team_number::blue] = GET_INTEL_SHORT(&(data[len]));				len += 2;
		Netgame.team_kills[team_number::red] = GET_INTEL_SHORT(&(data[len]));				len += 2;
		range_for (auto &i, Netgame.killed)
		{
			i = GET_INTEL_SHORT(&(data[len]));			len += 2;
		}
		range_for (auto &i, Netgame.player_kills)
		{
			i = GET_INTEL_SHORT(&(data[len]));		len += 2;
		}
		Netgame.KillGoal = GET_INTEL_INT(&(data[len]));					len += 4;
		Netgame.PlayTimeAllowed = d_time_fix(GET_INTEL_INT(&data[len]));
		len += 4;
		Netgame.level_time = GET_INTEL_INT(&(data[len]));				len += 4;
		Netgame.control_invul_time = GET_INTEL_INT(&(data[len]));			len += 4;
		Netgame.monitor_vector = GET_INTEL_INT(&(data[len]));				len += 4;
		range_for (auto &i, Netgame.player_score)
		{
			i = GET_INTEL_INT(&(data[len]));			len += 4;
		}
		range_for (auto &i, Netgame.net_player_flags)
		{
			i = player_flags(data[len]);
			len++;
		}
		Netgame.PacketsPerSec = GET_INTEL_SHORT(&(data[len]));				len += 2;
		Netgame.PacketLossPrevention = data[len];					len++;
		Netgame.NoFriendlyFire = data[len];						len++;
		Netgame.MouselookFlags = data[len];						len++;
		Netgame.PitchLockFlags = data[len];                     len++;
		copy_to_ntstring(data, len, Netgame.game_name);
		copy_to_ntstring(data, len, Netgame.mission_title);
		copy_to_ntstring(data, len, Netgame.mission_name);

		Netgame.protocol.udp.valid = 1; // This game is valid! YAY!
	}
}
}
}

namespace {

static void net_udp_process_dump(const upid_rspan<upid::dump> data, const _sockaddr &sender_addr)
{
	// Our request for join was denied.  Tell the user why.
	if (sender_addr != Netgame.players[0].protocol.udp.addr)
		return;
	const auto untrusted_why = build_kick_player_reason_from_untrusted(data[1]);
	if (!untrusted_why)
		/* If the peer sent an invalid kick_player_reason, ignore the kick. */
		return;
	switch (const auto why = *untrusted_why)
	{
		case kick_player_reason::pkttimeout:
		case kick_player_reason::kicked:
			{
				const auto g = Game_wind;
			if (g)
				g->set_visible(0);
			nm_messagebox_str(menu_title{nullptr}, nm_messagebox_tie(TXT_OK), menu_subtitle{
							why == kick_player_reason::pkttimeout
							? "You were removed from the game.\nYou failed receiving important\npackets. Sorry."
							: "You were kicked by Host!"}
			);
			if (g)
				g->set_visible(1);
			multi_quit_game = 1;
			game_leave_menus();
			break;
			}
		case kick_player_reason::closed:
		case kick_player_reason::full:
		case kick_player_reason::endlevel:
		case kick_player_reason::dork:
		case kick_player_reason::aborted:
		case kick_player_reason::connected:
		case kick_player_reason::level:
			Network_status = network_state::menu; // stop us from sending before message
			{
			const char *dump_string;
			switch (why)
			{
				case kick_player_reason::kicked:	// unreachable
				case kick_player_reason::pkttimeout:	// unreachable
				case kick_player_reason::closed:	// reachable
					dump_string = TXT_NET_GAME_CLOSED;
					break;
				case kick_player_reason::full:
					dump_string = TXT_NET_GAME_FULL;
					break;
				case kick_player_reason::endlevel:
					dump_string = TXT_NET_GAME_BETWEEN;
					break;
				case kick_player_reason::dork:
					dump_string = TXT_NET_GAME_NSELECT;
					break;
				case kick_player_reason::aborted:
					dump_string = TXT_NET_GAME_NSTART;
					break;
				case kick_player_reason::connected:
					dump_string = TXT_NET_GAME_CONNECT;
					break;
				case kick_player_reason::level:
					dump_string = TXT_NET_GAME_WRONGLEV;
					break;
			}
			nm_messagebox_str(menu_title{nullptr}, TXT_OK, menu_subtitle{dump_string});
			}
			Network_status = network_state::menu;
			multi_reset_stuff();
			break;
	}
}

static void net_udp_process_request(const UDP_sequence_request_packet &their, const struct _sockaddr &udp_addr)
{
	// Player is ready to receieve a sync packet
	for (unsigned i = 0; i < N_players; ++i)
		if (udp_addr == Netgame.players[i].protocol.udp.addr && !d_stricmp(their.callsign, Netgame.players[i].callsign))
		{
			vmplayerptr(i)->connected = player_connection_status::playing;
			Netgame.players[i].LastPacketTime = timer_query();
			break;
		}
}

}

namespace dsx {
namespace {

static void net_udp_process_packet(const d_level_shared_robot_info_state &LevelSharedRobotInfoState, const std::span<uint8_t> buf, const _sockaddr &sender_addr)
{
	const auto data = buf.data();
	const auto length = buf.size();
	const auto dcmd = data[0];
	const auto cmd = build_upid_from_untrusted(dcmd);
	if (!cmd)
	{
		con_printf(CON_DEBUG, "unknown packet type received - type %i", dcmd);
		return;
	}
	switch (*cmd)
	{
		case upid::version_deny:
			if (multi_i_am_master())
				break;
			if (const auto s = build_upid_rspan<upid::version_deny>(buf))
				net_udp_process_version_deny(*s, sender_addr);
			break;
		case upid::game_info_req:
		{
			if (!multi_i_am_master())
				break;
			const auto s = build_upid_rspan<upid::game_info_req>(buf);
			if (!s)
				break;
			static fix64 last_full_req_time = 0;
			if (timer_query() < last_full_req_time+(F1_0/2)) // answer 2 times per second max
				break;
			last_full_req_time = timer_query();
			switch (net_udp_check_game_info_request(*s, std::integral_constant<upid, upid::game_info_req>()))
			{
				case game_info_request_result::accept:
					net_udp_send_game_info(sender_addr, &sender_addr, upid::game_info);
					break;
				case game_info_request_result::id_mismatch:
					/* No response - peer is not the same type of Descent, and
					 * may not be Descent at all.
					 */
					break;
				case game_info_request_result::version_mismatch:
					net_udp_send_version_deny(sender_addr);
					break;
			}
			break;
		}
		case upid::game_info:
			if (multi_i_am_master() || length > upid_length<upid::game_info>)
				break;
			net_udp_process_game_info_heavy(data, length, sender_addr);
			break;
		case upid::game_info_lite_req:
		{
			if (!multi_i_am_master())
				break;
			const auto s = build_upid_rspan<upid::game_info_lite_req>(buf);
			if (!s)
				break;
			static fix64 last_lite_req_time = 0;
			if (timer_query() < last_lite_req_time+(F1_0/8))// answer 8 times per second max
				break;
			last_lite_req_time = timer_query();
			switch (net_udp_check_game_info_request(*s, std::integral_constant<upid, upid::game_info_lite_req>()))
			{
				case game_info_request_result::accept:
					net_udp_send_game_info(sender_addr, &sender_addr, upid::game_info_lite);
					break;
				case game_info_request_result::id_mismatch:
					/* No response - peer is not the same type of Descent, and
					 * may not be Descent at all.
					 */
				case game_info_request_result::version_mismatch:
					/* No response - peer was speculatively searching for games
					 * of the same version, and does not want to receive
					 * responses from version-incompatible programs.
					 */
					break;
			}
			break;
		}
		case upid::game_info_lite:
			if (multi_i_am_master() || length > upid_length<upid::game_info_lite>)
				break;
			net_udp_process_game_info_light(buf, sender_addr);
			break;
		case upid::dump:
			if (multi_i_am_master() || Netgame.players[0].protocol.udp.addr != sender_addr)
				break;
			if (const auto s = build_upid_rspan<upid::dump>(buf))
				if (Network_status == network_state::waiting || Network_status == network_state::playing)
					net_udp_process_dump(*s, sender_addr);
			break;
		case upid::addplayer:
			if (multi_i_am_master() || Netgame.players[0].protocol.udp.addr != sender_addr)
				break;
			if (const auto s = build_upid_rspan<upid::addplayer>(buf))
				net_udp_new_player(UDP_sequence_addplayer_packet::build_from_untrusted(*s), sender_addr);
			break;
		case upid::request:
			if (!multi_i_am_master())
				break;
			if (const auto s = build_upid_rspan<upid::request>(buf))
			{
				const auto their = UDP_sequence_request_packet::build_from_untrusted(*s);
				if (Network_status == network_state::starting) 
			{
				// Someone wants to join our game!
				net_udp_add_player(their, sender_addr);
			}
				else if (Network_status == network_state::waiting)
			{
				// Someone is ready to recieve a sync packet
				net_udp_process_request(their, sender_addr);
			}
				else if (Network_status == network_state::playing)
			{
				// Someone wants to join a game in progress!
				if (Netgame.RefusePlayers)
					net_udp_do_refuse_stuff(their, sender_addr);
				else
					net_udp_welcome_player(their, sender_addr);
			}
			}
			break;
		case upid::quit_joining:
			if (!multi_i_am_master() || length != 1)
				break;
			if (Network_status == network_state::starting)
				net_udp_remove_player(sender_addr);
			else if (Network_status == network_state::playing && Network_send_objects)
				net_udp_stop_resync(sender_addr);
			break;
		case upid::sync:
			if (multi_i_am_master() || length > upid_length<upid::game_info> || Network_status != network_state::waiting)
				break;
			net_udp_read_sync_packet(data, length, sender_addr);
			break;
		case upid::object_data:
			if (multi_i_am_master() || length > UPID_MAX_SIZE || Network_status != network_state::waiting)
				break;
			net_udp_read_object_packet(LevelSharedRobotInfoState, data);
			break;
		case upid::ping:
			if (multi_i_am_master())
				break;
			if (const auto s = build_upid_rspan<upid::ping>(buf))
				net_udp_process_ping(*s, sender_addr);
			break;
		case upid::pong:
			if (!multi_i_am_master())
				break;
			if (const auto s = build_upid_rspan<upid::pong>(buf))
				net_udp_process_pong(*s, sender_addr);
			break;
		case upid::endlevel_h:
			if ((!multi_i_am_master()) && (Network_status == network_state::endlevel || Network_status == network_state::playing))
				net_udp_read_endlevel_packet(data, sender_addr);
			break;
		case upid::endlevel_c:
			if ((multi_i_am_master()) && (Network_status == network_state::endlevel || Network_status == network_state::playing))
				net_udp_read_endlevel_packet(data, sender_addr);
			break;
		case upid::pdata:
			if (const auto s = build_upid_rspan<upid::pdata>(buf))
				net_udp_process_pdata(*s, sender_addr);
			break;
		case upid::mdata_pnorm:
			net_udp_process_mdata(LevelSharedRobotInfoState, buf, sender_addr, 0);
			break;
		case upid::mdata_pneedack:
			net_udp_process_mdata(LevelSharedRobotInfoState, buf, sender_addr, 1);
			break;
		case upid::mdata_ack:
			if (const auto s = build_upid_rspan<upid::mdata_ack>(buf))
				net_udp_noloss_got_ack(*s);
			break;
#if DXX_USE_TRACKER
		case upid::tracker_gameinfo:
			udp_tracker_process_game(buf, sender_addr);
			break;
		case upid::tracker_ack:
			if (!multi_i_am_master())
				break;
			udp_tracker_process_ack( data, length, sender_addr );
			break;
		case upid::tracker_holepunch:
			udp_tracker_process_holepunch( data, length, sender_addr );
			break;
#endif
	}
}

}
}

namespace {

// Packet for end of level syncing
void net_udp_read_endlevel_packet(const uint8_t *data, const _sockaddr &sender_addr)
{
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	int len = 0;
	ubyte tmpvar = 0;
	
	if (multi_i_am_master())
	{
		playernum_t pnum = data[1];

		if (Netgame.players[pnum].protocol.udp.addr != sender_addr)
			return;

		len += 2;

		if (player_connection_status{data[len]} == player_connection_status::disconnected)
			multi_disconnect_player(pnum);
		vmplayerptr(pnum)->connected = player_connection_status{data[len]};					len++;
		tmpvar = data[len];							len++;
		if (Network_status != network_state::playing && vcplayerptr(pnum)->connected == player_connection_status::playing && tmpvar < LevelUniqueControlCenterState.Countdown_seconds_left)
			LevelUniqueControlCenterState.Countdown_seconds_left = tmpvar;
		auto &objp = *vmobjptr(vcplayerptr(pnum)->objnum);
		auto &player_info = objp.ctype.player_info;
		player_info.net_kills_total = GET_INTEL_SHORT(&data[len]);
		len += 2;
		player_info.net_killed_total = GET_INTEL_SHORT(&data[len]);
		len += 2;

		range_for (auto &i, kill_matrix[pnum])
		{
			i = GET_INTEL_SHORT(&(data[len]));		len += 2;
		}
		if (vcplayerptr(pnum)->connected != player_connection_status::disconnected)
			Netgame.players[pnum].LastPacketTime = timer_query();
	}
	else
	{
		if (Netgame.players[0].protocol.udp.addr != sender_addr)
			return;

		len++;

		tmpvar = data[len];							len++;
		if (Network_status != network_state::playing && tmpvar < LevelUniqueControlCenterState.Countdown_seconds_left)
			LevelUniqueControlCenterState.Countdown_seconds_left = tmpvar;

		for (playernum_t i = 0; i < MAX_PLAYERS; i++)
		{
			if (i == Player_num)
			{
				len += 5;
				continue;
			}

			if (player_connection_status{data[len]} == player_connection_status::disconnected)
				multi_disconnect_player(i);
			auto &objp = *vmobjptr(vcplayerptr(i)->objnum);
			auto &player_info = objp.ctype.player_info;
			vmplayerptr(i)->connected = player_connection_status{data[len]};				len++;
			player_info.net_kills_total = GET_INTEL_SHORT(&data[len]);
			len += 2;
			player_info.net_killed_total = GET_INTEL_SHORT(&data[len]);
			len += 2;

			if (vcplayerptr(i)->connected != player_connection_status::disconnected)
				Netgame.players[i].LastPacketTime = timer_query();
		}

		for (playernum_t i = 0; i < MAX_PLAYERS; i++)
		{
			for (playernum_t j = 0; j < MAX_PLAYERS; j++)
			{
				if (i != Player_num)
				{
					kill_matrix[i][j] = GET_INTEL_SHORT(&(data[len]));
				}
											len += 2;
			}
		}
	}
}

/*
 * Polling loop waiting for sync packet to start game after having sent request
 */
static int net_udp_sync_poll( newmenu *,const d_event &event, const unused_newmenu_userdata_t *)
{
	static fix64 t1 = 0;
	int rval = 0;

	if (event.type != event_type::window_draw)
		return 0;
	net_udp_listen();

	// Leave if Host disconnects
	if (Netgame.players[0].connected == player_connection_status::disconnected)
		rval = -2;

	if (Network_status != network_state::waiting)	// Status changed to playing, exit the menu
		rval = -2;

	if (Network_status != network_state::menu && !Network_rejoined && (timer_query() > t1+F1_0*2))
	{
		// Poll time expired, re-send request
		
		t1 = timer_query();

		auto i = net_udp_send_request();
		if (i >= MAX_PLAYERS)
			rval = -2;
	}
	
	return rval;
}

static int net_udp_start_poll(newmenu *, const d_event &event, start_poll_menu_items *const items)
{
	if (event.type != event_type::window_draw)
		return 0;
	assert(Network_status == network_state::starting);

	auto &menus = items->m;
	const unsigned nitems = menus.size();
	menus[0].value = 1;
	range_for (auto &i, partial_range(menus, N_players, nitems))
		i.value = 0;

	const auto predicate = [](const newmenu_item &i) {
		return i.value;
	};
	const auto nm = std::count_if(menus.begin(), std::next(menus.begin(), nitems), predicate);
	if ( nm > Netgame.max_numplayers ) {
		nm_messagebox(menu_title{TXT_ERROR}, {TXT_OK}, "%s %d %s", TXT_SORRY_ONLY, Netgame.max_numplayers, TXT_NETPLAYERS_IN);
		// Turn off the last player highlighted
		for (int i = N_players; i > 0; i--)
			if (menus[i].value == 1) 
			{
				menus[i].value = 0;
				break;
			}
	}

	net_udp_listen();

	for (int i=0; i<N_players; i++ ) // fill this in always in case players change but not their numbers
	{
		const auto &&rankstr = GetRankStringWithSpace(Netgame.players[i].rank);
		snprintf(menus[i].text, 45, "%d. %s%s%-20s", i+1, rankstr.first, rankstr.second, static_cast<const char *>(Netgame.players[i].callsign));
	}

	const unsigned players_last_poll = items->get_player_count();
	if (players_last_poll == Netgame.numplayers)
		return 0;
	items->set_player_count(Netgame.numplayers);
	// A new player
	if (players_last_poll < Netgame.numplayers)
	{
		digi_play_sample (SOUND_HUD_MESSAGE,F1_0);
		if (N_players <= Netgame.max_numplayers)
			menus[N_players-1].value = 1;
	} 
	else	// One got removed...
	{
		digi_play_sample (SOUND_HUD_KILL,F1_0);
  
		const auto j = std::min(N_players, static_cast<unsigned>(Netgame.max_numplayers));
		/* Reset all the user's choices, since there is insufficient
		 * integration to move the checks based on the position of the
		 * departed player(s).  Without this reset, names would move up
		 * one line, but checkboxes would not.
		 */
		range_for (auto &i, partial_range(menus, j))
			i.value = 1;
		range_for (auto &i, partial_range(menus, j, N_players))
			i.value = 0;
		range_for (auto &i, partial_range(menus, N_players, players_last_poll))
		{
			/* The default format string is "%d. "
			 * For single digit numbers, [3] is the first character
			 * after the space.  For double digit numbers, [3] is the
			 * space.  Users cannot see the trailing space or its
			 * absence, so always overwrite [3].  This would break if
			 * the menu allowed more than 99 players.
			 */
			i.text[3] = 0;
			i.value = 0;
		}
	}
	return 0;
}

#if DXX_USE_TRACKER
#define DXX_UDP_MENU_TRACKER_OPTION(VERB)	\
	DXX_MENUITEM(VERB, CHECK, "Track this game on", opt_tracker, Netgame.Tracker) \
	DXX_MENUITEM(VERB, TEXT, tracker_addr_txt, opt_tracker_addr)	\
	DXX_MENUITEM(VERB, CHECK, "Enable tracker NAT hole punch", opt_tracker_nathp, TrackerNATWarned)	\

#else
#define DXX_UDP_MENU_TRACKER_OPTION(VERB)
#endif

#if defined(DXX_BUILD_DESCENT_I)
#define D2X_UDP_MENU_OPTIONS(VERB)	\

#elif defined(DXX_BUILD_DESCENT_II)
#define D2X_UDP_MENU_OPTIONS(VERB)	\
	DXX_MENUITEM(VERB, CHECK, "Allow Marker camera views", opt_marker_view, Netgame.Allow_marker_view)	\
	DXX_MENUITEM(VERB, CHECK, "Indestructible lights", opt_light, Netgame.AlwaysLighting)	\
	DXX_MENUITEM(VERB, CHECK, "Remove Thief at level start", opt_thief_presence, thief_absent)	\
	DXX_MENUITEM(VERB, CHECK, "Prevent Thief Stealing Energy Weapons", opt_thief_steal_energy, thief_cannot_steal_energy_weapons)	\
	DXX_MENUITEM(VERB, CHECK, "Allow Guidebot (coop only; experimental)", opt_guidebot_enabled, Netgame.AllowGuidebot)	\

#endif

constexpr std::integral_constant<unsigned, F1_0 * 60> reactor_invul_time_mini_scale{};
constexpr std::integral_constant<unsigned, 5 * reactor_invul_time_mini_scale> reactor_invul_time_scale{};

#if defined(DXX_BUILD_DESCENT_I)
#define D2X_DUPLICATE_POWERUP_OPTIONS(VERB)	                           \

#elif defined(DXX_BUILD_DESCENT_II)
#define D2X_DUPLICATE_POWERUP_OPTIONS(VERB)	                           \
	DXX_MENUITEM(VERB, SLIDER, extraAccessory, opt_extra_accessory, accessory, 0, (1 << packed_netduplicate_items::accessory_width) - 1)	\

#endif

#define DXX_DUPLICATE_POWERUP_OPTIONS(VERB)	                           \
	DXX_MENUITEM(VERB, SLIDER, extraPrimary, opt_extra_primary, primary, 0, (1 << packed_netduplicate_items::primary_width) - 1)	\
	DXX_MENUITEM(VERB, SLIDER, extraSecondary, opt_extra_secondary, secondary, 0, (1 << packed_netduplicate_items::secondary_width) - 1)	\
	D2X_DUPLICATE_POWERUP_OPTIONS(VERB)                                

#define DXX_UDP_MENU_OPTIONS(VERB)	                                    \
	DXX_MENUITEM(VERB, TEXT, "Game Options", game_label)	                     \
	DXX_MENUITEM(VERB, SLIDER, get_annotated_difficulty_string(Netgame.difficulty), opt_difficulty, difficulty, underlying_value(Difficulty_level_type::_0), underlying_value(Difficulty_level_type::_4))	\
	DXX_MENUITEM(VERB, SCALE_SLIDER, srinvul, opt_cinvul, Netgame.control_invul_time, 0, 10, reactor_invul_time_scale)	\
	DXX_MENUITEM(VERB, SLIDER, PlayText, opt_playtime, PlayTimeAllowed, 0, 12)	\
	DXX_MENUITEM(VERB, SLIDER, KillText, opt_killgoal, Netgame.KillGoal, 0, 20)	\
	DXX_MENUITEM(VERB, TEXT, "", blank_1)                                     \
	DXX_MENUITEM(VERB, TEXT, "Duplicate Powerups", duplicate_label)	          \
	DXX_DUPLICATE_POWERUP_OPTIONS(VERB)		                              \
	DXX_MENUITEM(VERB, TEXT, "", blank_5)                                     \
	DXX_MENUITEM(VERB, TEXT, "Spawn Options", spawn_label)	                   \
	DXX_MENUITEM(VERB, SLIDER, SecludedSpawnText, opt_secluded_spawns, Netgame.SecludedSpawns, 0, MAX_PLAYERS - 1)	\
	DXX_MENUITEM(VERB, SLIDER, SpawnInvulnerableText, opt_start_invul, Netgame.InvulAppear, 0, 8)	\
	DXX_MENUITEM(VERB, TEXT, "", blank_2)                                     \
	DXX_MENUITEM(VERB, TEXT, "Object Options", powerup_label)	                \
	DXX_MENUITEM(VERB, CHECK, "Shuffle powerups in anarchy games", opt_shuffle_powerups, Netgame.ShufflePowerupSeed)	\
	DXX_MENUITEM(VERB, MENU, "Set Objects allowed...", opt_setpower)	         \
	DXX_MENUITEM(VERB, MENU, "Set Objects granted at spawn...", opt_setgrant)	\
	DXX_MENUITEM(VERB, TEXT, "", blank_3)                                     \
	DXX_MENUITEM(VERB, TEXT, "Misc. Options", misc_label)	                    \
	DXX_MENUITEM(VERB, CHECK, TXT_SHOW_ON_MAP, opt_show_on_map, game_flag_show_all_players_on_automap)	\
	D2X_UDP_MENU_OPTIONS(VERB)	                                        \
	DXX_MENUITEM(VERB, CHECK, "Bright player ships", opt_bright, Netgame.BrightPlayers)	\
	DXX_MENUITEM(VERB, CHECK, "Show enemy names on HUD", opt_show_names, Netgame.ShowEnemyNames)	\
	DXX_MENUITEM(VERB, CHECK, "No friendly fire (Team, Coop)", opt_ffire, Netgame.NoFriendlyFire)	\
	DXX_MENUITEM(VERB, FCHECK, game_is_cooperative ? "Allow coop mouselook" : "Allow anarchy mouselook", opt_mouselook, Netgame.MouselookFlags, MouselookMPFlag(game_is_cooperative))	\
	DXX_MENUITEM(VERB, FCHECK, game_is_cooperative ? "Release coop pitch lock" : "Release anarchy pitch lock", opt_pitch_lock, Netgame.PitchLockFlags, MouselookMPFlag(game_is_cooperative))  \
	DXX_MENUITEM(VERB, TEXT, "", blank_4)                                     \
	DXX_MENUITEM_AUTOSAVE_LABEL_INPUT(VERB)	\
	DXX_MENUITEM(VERB, TEXT, "", blank_6)                                     \
	DXX_MENUITEM(VERB, TEXT, "Network Options", network_label)	               \
	DXX_MENUITEM(VERB, TEXT, "Packets per second (" DXX_STRINGIZE_PPS(MIN_PPS) " - " DXX_STRINGIZE_PPS(MAX_PPS) ")", opt_label_pps)	\
	DXX_MENUITEM(VERB, INPUT, packstring, opt_packets)	\
	DXX_MENUITEM(VERB, TEXT, "Network port", opt_label_port)	\
	DXX_MENUITEM(VERB, INPUT, portstring, opt_port)	\
	DXX_UDP_MENU_TRACKER_OPTION(VERB)

#define DXX_STRINGIZE_PPS2(X)	#X
#define DXX_STRINGIZE_PPS(X)	DXX_STRINGIZE_PPS2(X)

struct netgame_powerups_allowed_menu_items
{
	std::array<newmenu_item, multi_allow_powerup_text.size()> m;
	netgame_powerups_allowed_menu_items()
	{
		const auto AllowedItems{underlying_value(Netgame.AllowedItems)};
		for (auto &&[i, t, mi] : enumerate(zip(multi_allow_powerup_text, m)))
			nm_set_item_checkbox(mi, t, AllowedItems & (1u << i));
	}
};

struct netgame_powerups_allowed_menu : netgame_powerups_allowed_menu_items, newmenu
{
	netgame_powerups_allowed_menu(grs_canvas &src) :
		newmenu(menu_title{nullptr}, menu_subtitle{"Objects to allow"}, menu_filename{nullptr}, tiny_mode_flag::normal, tab_processing_flag::ignore, adjusted_citem::create(m, 0), src)
	{
	}
	virtual window_event_result event_handler(const d_event &event) override;
};

window_event_result netgame_powerups_allowed_menu::event_handler(const d_event &event)
{
	switch (event.type)
	{
		case event_type::window_close:
			{
				typename std::underlying_type<netflag_flag>::type AllowedItems = 0;
				for (auto &&[i, mi] : enumerate(m))
					if (mi.value)
						AllowedItems |= (1 << i);
				Netgame.AllowedItems = netflag_flag{AllowedItems};
				break;
			}
		default:
			break;
	}
	return newmenu::event_handler(event);
}

static void net_udp_set_power (void)
{
	auto menu = window_create<netgame_powerups_allowed_menu>(grd_curscreen->sc_canvas);
	(void)menu;
}

#if defined(DXX_BUILD_DESCENT_I)
#define D2X_GRANT_POWERUP_MENU(VERB)
#elif defined(DXX_BUILD_DESCENT_II)
#define D2X_GRANT_POWERUP_MENU(VERB)	\
	DXX_MENUITEM(VERB, CHECK, NETFLAG_LABEL_GAUSS, opt_gauss, menu_bit_wrapper(flags, netgrant_flag::NETGRANT_GAUSS))	\
	DXX_MENUITEM(VERB, CHECK, NETFLAG_LABEL_HELIX, opt_helix, menu_bit_wrapper(flags, netgrant_flag::NETGRANT_HELIX))	\
	DXX_MENUITEM(VERB, CHECK, NETFLAG_LABEL_PHOENIX, opt_phoenix, menu_bit_wrapper(flags, netgrant_flag::NETGRANT_PHOENIX))	\
	DXX_MENUITEM(VERB, CHECK, NETFLAG_LABEL_OMEGA, opt_omega, menu_bit_wrapper(flags, netgrant_flag::NETGRANT_OMEGA))	\
	DXX_MENUITEM(VERB, CHECK, NETFLAG_LABEL_AFTERBURNER, opt_afterburner, menu_bit_wrapper(flags, netgrant_flag::NETGRANT_AFTERBURNER))	\
	DXX_MENUITEM(VERB, CHECK, NETFLAG_LABEL_AMMORACK, opt_ammo_rack, menu_bit_wrapper(flags, netgrant_flag::NETGRANT_AMMORACK))	\
	DXX_MENUITEM(VERB, CHECK, NETFLAG_LABEL_CONVERTER, opt_converter, menu_bit_wrapper(flags, netgrant_flag::NETGRANT_CONVERTER))	\
	DXX_MENUITEM(VERB, CHECK, NETFLAG_LABEL_HEADLIGHT, opt_headlight, menu_bit_wrapper(flags, netgrant_flag::NETGRANT_HEADLIGHT))	\

#endif

#define DXX_GRANT_POWERUP_MENU(VERB)	\
	DXX_MENUITEM(VERB, NUMBER, "Laser level", opt_laser_level, menu_number_bias_wrapper<1>(laser_level), static_cast<unsigned>(laser_level::_1) + 1, static_cast<unsigned>(DXX_MAXIMUM_LASER_LEVEL) + 1)	\
	DXX_MENUITEM(VERB, CHECK, NETFLAG_LABEL_QUAD, opt_quad_lasers, menu_bit_wrapper(flags, netgrant_flag::NETGRANT_QUAD))	\
	DXX_MENUITEM(VERB, CHECK, NETFLAG_LABEL_VULCAN, opt_vulcan, menu_bit_wrapper(flags, netgrant_flag::NETGRANT_VULCAN))	\
	DXX_MENUITEM(VERB, CHECK, NETFLAG_LABEL_SPREAD, opt_spreadfire, menu_bit_wrapper(flags, netgrant_flag::NETGRANT_SPREAD))	\
	DXX_MENUITEM(VERB, CHECK, NETFLAG_LABEL_PLASMA, opt_plasma, menu_bit_wrapper(flags, netgrant_flag::NETGRANT_PLASMA))	\
	DXX_MENUITEM(VERB, CHECK, NETFLAG_LABEL_FUSION, opt_fusion, menu_bit_wrapper(flags, netgrant_flag::NETGRANT_FUSION))	\
	D2X_GRANT_POWERUP_MENU(VERB)

class more_game_options_menu_items
{
protected:
	const unsigned game_is_cooperative;
	std::array<char, sizeof("99")> packstring;
	std::array<char, sizeof("65535")> portstring;
	/* Reactor life and Maximum time are stored in a uint32_t, and have
	 * a theoretical maximum of 1092 after converting from internal game
	 * time (seconds in fixed point) to minutes.  User input limitations
	 * prevent setting a value higher than 50 minutes.  Even if the code
	 * is modified to verify a maximum value of 50 immediately before
	 * formatting it, the gcc value range propagation pass fails to
	 * detect the reduced range and issues a warning as if the value
	 * could be 1092.  Eliminate the bogus warning by using a buffer
	 * large enough for the theoretical maximum.
	 */
	char srinvul[sizeof("Reactor life: 1092 min")];
	char PlayText[sizeof("Max time: 1092 min")];
	char SpawnInvulnerableText[sizeof("Invul. Time: 0.0 sec")];
	char SecludedSpawnText[sizeof("Use 0 Furthest Sites")];
	char KillText[sizeof("Kill goal: 000 kills")];
	char extraPrimary[sizeof("Primaries: 0")];
	char extraSecondary[sizeof("Secondaries: 0")];
#if defined(DXX_BUILD_DESCENT_II)
	char extraAccessory[sizeof("Accessories: 0")];
#endif
#if DXX_USE_TRACKER
        char tracker_addr_txt[sizeof("65535") + 28];
#endif
	human_readable_mmss_time<decltype(d_gameplay_options::AutosaveInterval)::rep> AutosaveInterval;
	using menu_array = std::array<newmenu_item, DXX_UDP_MENU_OPTIONS(COUNT)>;
	DXX_UDP_MENU_OPTIONS(DECL);
	menu_array m;
	static const char *get_annotated_difficulty_string(const Difficulty_level_type d)
	{
		static constexpr enumerated_array<char[20], 5, Difficulty_level_type> text{{{
			"Difficulty: Trainee",
			"Difficulty: Rookie",
			"Difficulty: Hotshot",
			"Difficulty: Ace",
			"Difficulty: Insane"
		}}};
		switch (d)
		{
			case Difficulty_level_type::_0:
			case Difficulty_level_type::_1:
			case Difficulty_level_type::_2:
			case Difficulty_level_type::_3:
			case Difficulty_level_type::_4:
				return text[d];
			default:
				// Empty string at the end of `Ace`
				return &text[Difficulty_level_type::_3][16];
		}
	}
	static int handler(newmenu *, const d_event &event, more_game_options_menu_items *items);
public:
	menu_array &get_menu_items()
	{
		return m;
	}
	void update_difficulty_string(const Difficulty_level_type difficulty)
	{
		/* Cast away const because newmenu_item uses `char *text` even
		 * for fields where text is treated as `const char *`.
		 */
		m[opt_difficulty].text = const_cast<char *>(get_annotated_difficulty_string(difficulty));
	}
	void update_extra_primary_string(unsigned primary)
	{
		snprintf(extraPrimary, sizeof(extraPrimary), "Primaries: %u", primary);
	}
	void update_extra_secondary_string(unsigned secondary)
	{
		snprintf(extraSecondary, sizeof(extraSecondary), "Secondaries: %u", secondary);
	}
#if defined(DXX_BUILD_DESCENT_II)
	void update_extra_accessory_string(unsigned accessory)
	{
		snprintf(extraAccessory, sizeof(extraAccessory), "Accessories: %u", accessory);
	}
#endif
	void update_packstring()
	{
		snprintf(packstring.data(), packstring.size(), "%u", Netgame.PacketsPerSec);
	}
	void update_portstring()
	{
		snprintf(portstring.data(), portstring.size(), "%hu", UDP_MyPort);
	}
	void update_reactor_life_string(unsigned t)
	{
		snprintf(srinvul, sizeof(srinvul), "%s: %u %s", TXT_REACTOR_LIFE, t, TXT_MINUTES_ABBREV);
	}
	void update_max_play_time_string()
	{
		snprintf(PlayText, sizeof(PlayText), "Max time: %d %s", Netgame.PlayTimeAllowed.count() / (F1_0 * 60), TXT_MINUTES_ABBREV);
	}
	void update_spawn_invuln_string()
	{
		snprintf(SpawnInvulnerableText, sizeof(SpawnInvulnerableText), "Invul. Time: %1.1f sec", static_cast<float>(Netgame.InvulAppear) / 2);
	}
	void update_secluded_spawn_string()
	{
		const unsigned SecludedSpawns = Netgame.SecludedSpawns;
		cf_assert(SecludedSpawns < MAX_PLAYERS);
		snprintf(SecludedSpawnText, sizeof(SecludedSpawnText), "Use %u Furthest Sites", SecludedSpawns + 1);
	}
	void update_kill_goal_string()
	{
		snprintf(KillText, sizeof(KillText), "Kill Goal: %3d", Netgame.KillGoal * 5);
	}
	enum
	{
		DXX_UDP_MENU_OPTIONS(ENUM)
	};
	more_game_options_menu_items(const unsigned game_is_cooperative) :
		game_is_cooperative(game_is_cooperative),
		AutosaveInterval{build_human_readable_time(Netgame.MPGameplayOptions.AutosaveInterval)}
	{
		const auto edifficulty = Netgame.difficulty;
		const auto difficulty = underlying_value(edifficulty);
		update_difficulty_string(edifficulty);
		update_packstring();
		update_portstring();
		update_reactor_life_string(Netgame.control_invul_time / reactor_invul_time_mini_scale);
		update_max_play_time_string();
		update_spawn_invuln_string();
		update_secluded_spawn_string();
		update_kill_goal_string();
		auto primary = Netgame.DuplicatePowerups.get_primary_count();
		auto secondary = Netgame.DuplicatePowerups.get_secondary_count();
#if defined(DXX_BUILD_DESCENT_II)
		auto accessory = Netgame.DuplicatePowerups.get_accessory_count();
		const auto thief_absent = Netgame.ThiefModifierFlags & ThiefModifier::Absent;
		const auto thief_cannot_steal_energy_weapons = Netgame.ThiefModifierFlags & ThiefModifier::NoEnergyWeapons;
		update_extra_accessory_string(accessory);
#endif
		update_extra_primary_string(primary);
		update_extra_secondary_string(secondary);
#if DXX_USE_TRACKER
		const unsigned TrackerNATWarned = Netgame.TrackerNATWarned == TrackerNATHolePunchWarn::UserEnabledHP;
#endif
		const unsigned PlayTimeAllowed = std::chrono::duration_cast<std::chrono::duration<int, netgame_info::play_time_allowed_abi_ratio>>(Netgame.PlayTimeAllowed).count();
		const auto game_flag_show_all_players_on_automap{underlying_value(Netgame.game_flag & netgame_rule_flags::show_all_players_on_automap)};
		DXX_UDP_MENU_OPTIONS(ADD);
#if DXX_USE_TRACKER
		const auto &tracker_addr = CGameArg.MplTrackerAddr;
		if (tracker_addr.empty())
                {
			nm_set_item_text(m[opt_tracker], "Tracker use disabled");
                        nm_set_item_text(m[opt_tracker_addr], "<Tracker address not set>");
                }
		else
                {
			snprintf(tracker_addr_txt, sizeof(tracker_addr_txt), "%s:%u", tracker_addr.c_str(), CGameArg.MplTrackerPort);
                }
#endif
	}
	void read() const
	{
		unsigned primary, secondary;
#if defined(DXX_BUILD_DESCENT_II)
		unsigned accessory;
		uint8_t thief_absent;
		uint8_t thief_cannot_steal_energy_weapons;
#endif
		uint8_t difficulty;
#if DXX_USE_TRACKER
		unsigned TrackerNATWarned;
#endif
		unsigned PlayTimeAllowed;
		uint8_t game_flag_show_all_players_on_automap;
		DXX_UDP_MENU_OPTIONS(READ);
		Netgame.difficulty = cast_clamp_difficulty(difficulty);
		Netgame.PlayTimeAllowed = std::chrono::duration<int, netgame_info::play_time_allowed_abi_ratio>(PlayTimeAllowed);
		if (game_flag_show_all_players_on_automap)
			Netgame.game_flag |= netgame_rule_flags::show_all_players_on_automap;
		else
			Netgame.game_flag &= ~netgame_rule_flags::show_all_players_on_automap;
		auto &items = Netgame.DuplicatePowerups;
		items.set_primary_count(primary);
		items.set_secondary_count(secondary);
#if defined(DXX_BUILD_DESCENT_II)
		items.set_accessory_count(accessory);
		Netgame.ThiefModifierFlags =
			(thief_absent ? ThiefModifier::Absent : 0) |
			(thief_cannot_steal_energy_weapons ? ThiefModifier::NoEnergyWeapons : 0);
#endif
		char *p;
		auto pps = strtol(packstring.data(), &p, 10);
		if (!*p)
			Netgame.PacketsPerSec = pps;
#if DXX_USE_TRACKER
		Netgame.TrackerNATWarned = TrackerNATWarned ? TrackerNATHolePunchWarn::UserEnabledHP : TrackerNATHolePunchWarn::UserRejectedHP;
#endif
		convert_text_portstring(portstring, UDP_MyPort, false, false);
		parse_human_readable_time(Netgame.MPGameplayOptions.AutosaveInterval, AutosaveInterval);
	}
};

struct more_game_options_menu : more_game_options_menu_items, newmenu
{
	more_game_options_menu(unsigned game_is_cooperative, grs_canvas &);
	static void net_udp_more_game_options(unsigned game_is_cooperative);
	virtual window_event_result event_handler(const d_event &event) override;
};

more_game_options_menu::more_game_options_menu(const unsigned game_is_cooperative, grs_canvas &canvas) :
	more_game_options_menu_items(game_is_cooperative),
	newmenu(menu_title{nullptr}, menu_subtitle{"Advanced netgame options"}, menu_filename{nullptr}, tiny_mode_flag::normal, tab_processing_flag::ignore, adjusted_citem::create(m, 0), canvas)
{
}

class grant_powerup_menu_items
{
public:
	enum
	{
		DXX_GRANT_POWERUP_MENU(ENUM)
	};
	std::array<newmenu_item, DXX_GRANT_POWERUP_MENU(COUNT)> m;
	grant_powerup_menu_items(const laser_level level, const packed_spawn_granted_items p)
	{
		auto &flags = p.mask;
		auto laser_level{static_cast<uint8_t>(level)};
		DXX_GRANT_POWERUP_MENU(ADD);
	}
	void read(packed_spawn_granted_items &p) const
	{
		uint8_t laser_level{};
		typename std::underlying_type<netgrant_flag>::type flags{};
		DXX_GRANT_POWERUP_MENU(READ);
		flags |= laser_level;
		p.mask = netgrant_flag{flags};
	}
};

struct grant_powerup_menu : grant_powerup_menu_items, newmenu
{
	grant_powerup_menu(const laser_level level, const packed_spawn_granted_items p, grs_canvas &src) :
		grant_powerup_menu_items(level, p),
		newmenu(menu_title{nullptr}, menu_subtitle{"Powerups granted at player spawn"}, menu_filename{nullptr}, tiny_mode_flag::normal, tab_processing_flag::ignore, adjusted_citem::create(m, 0), src)
	{
	}
	virtual window_event_result event_handler(const d_event &event) override;
};

window_event_result grant_powerup_menu::event_handler(const d_event &event)
{
	switch (event.type)
	{
		case event_type::window_close:
			read(Netgame.SpawnGrantedItems);
			break;
		default:
			break;
	}
	return newmenu::event_handler(event);
}

static void net_udp_set_grant_power()
{
	const auto SpawnGrantedItems = Netgame.SpawnGrantedItems;
	auto menu = window_create<grant_powerup_menu>(map_granted_flags_to_laser_level(SpawnGrantedItems), SpawnGrantedItems, grd_curscreen->sc_canvas);
	(void)menu;
}

void more_game_options_menu::net_udp_more_game_options(const unsigned game_is_cooperative)
{
	auto menu = window_create<more_game_options_menu>(game_is_cooperative, grd_curscreen->sc_canvas);
	(void)menu;
}

window_event_result more_game_options_menu::event_handler(const d_event &event)
{
	switch (event.type)
	{
		case event_type::newmenu_changed:
		{
			auto &citem = static_cast<const d_change_event &>(event).citem;
			auto &menus = m;
			if (citem == opt_difficulty)
			{
				Netgame.difficulty = cast_clamp_difficulty(menus[opt_difficulty].value);
				update_difficulty_string(Netgame.difficulty);
			}
			else if (citem == opt_cinvul)
				update_reactor_life_string(menus[opt_cinvul].value * (reactor_invul_time_scale / reactor_invul_time_mini_scale));
			else if (citem == opt_playtime)
			{
				if (game_is_cooperative)
				{
					nm_messagebox_str(menu_title{TXT_SORRY}, nm_messagebox_tie(TXT_OK), menu_subtitle{"You can't change those for coop!"});
					menus[opt_playtime].value=0;
					return window_event_result::ignored;
				}
				Netgame.PlayTimeAllowed = std::chrono::duration<int, netgame_info::play_time_allowed_abi_ratio>(menus[opt_playtime].value);
				update_max_play_time_string();
			}
			else if (citem == opt_killgoal)
			{
				if (game_is_cooperative)
				{
					nm_messagebox_str(menu_title{TXT_SORRY}, nm_messagebox_tie(TXT_OK), menu_subtitle{"You can't change those for coop!"});
					menus[opt_killgoal].value=0;
					return window_event_result::ignored;
				}
				Netgame.KillGoal=menus[opt_killgoal].value;
				update_kill_goal_string();
			}
			else if(citem == opt_extra_primary)
			{
				auto primary = menus[opt_extra_primary].value;
				update_extra_primary_string(primary);
			}
			else if(citem == opt_extra_secondary)
			{
				auto secondary = menus[opt_extra_secondary].value;
				update_extra_secondary_string(secondary);
			}
#if defined(DXX_BUILD_DESCENT_II)
			else if(citem == opt_extra_accessory)
			{
				auto accessory = menus[opt_extra_accessory].value;
				update_extra_accessory_string(accessory);
			}
#endif
			else if (citem == opt_start_invul)
			{
				Netgame.InvulAppear = menus[opt_start_invul].value;
				update_spawn_invuln_string();
			}
			else if (citem == opt_secluded_spawns)
			{
				Netgame.SecludedSpawns = menus[opt_secluded_spawns].value;
				update_secluded_spawn_string();
			}
			break;
		}
		case event_type::newmenu_selected:
		{
			auto &citem = static_cast<const d_select_event &>(event).citem;
			if (citem == opt_setpower)
				net_udp_set_power();
			else if (citem == opt_setgrant)
				net_udp_set_grant_power();
			else
				break;
			return window_event_result::handled;
		}
		case event_type::window_close:
			read();
			if (Netgame.PacketsPerSec > MAX_PPS)
			{
				Netgame.PacketsPerSec = MAX_PPS;
				nm_messagebox(menu_title{TXT_ERROR}, {TXT_OK}, "Packet value out of range\nSetting value to %i", MAX_PPS);
			}
			else if (Netgame.PacketsPerSec < MIN_PPS)
			{
				Netgame.PacketsPerSec = MIN_PPS;
				nm_messagebox(menu_title{TXT_ERROR}, {TXT_OK}, "Packet value out of range\nSetting value to %i", MIN_PPS);
			}
			GameUniqueState.Difficulty_level = Netgame.difficulty;
			break;
		default:
			break;
	}
	return newmenu::event_handler(event);
}

struct param_opt
{
	enum {
		name = 2,
		label_level,
		level,
	};
	int start_game, mode, mode_end, moreopts;
	int closed, refuse, maxnet, anarchy, team_anarchy, robot_anarchy, coop, bounty;
#if defined(DXX_BUILD_DESCENT_II)
	int capture, hoard, team_hoard;
#endif
	std::array<char, sizeof("S100")> slevel{{"1"}};
	char srmaxnet[sizeof("Maximum players: 99")];
	ntstring<NM_MAX_TEXT_LEN> max_numplayers_saved_text;
	std::array<newmenu_item, 22> m;
	void update_netgame_max_players()
	{
		Netgame.max_numplayers = m[maxnet].value + 2;
		update_max_players_string();
	}
	void update_max_players_string()
	{
		const unsigned max_numplayers = Netgame.max_numplayers;
		cf_assert(max_numplayers < MAX_PLAYERS);
		snprintf(srmaxnet, sizeof(srmaxnet), "Maximum players: %u", max_numplayers);
	}
};

static int net_udp_game_param_handler( newmenu *menu,const d_event &event, param_opt *opt )
{
	newmenu_item *menus = newmenu_get_items(menu);
	switch (event.type)
	{
		case event_type::newmenu_changed:
		{
			auto &citem = static_cast<const d_change_event &>(event).citem;
#if defined(DXX_BUILD_DESCENT_I)
			if (citem == opt->team_anarchy)
			{
				menus[opt->closed].value = 1;
				menus[opt->closed-1].value = 0;
				menus[opt->closed+1].value = 0;
			}
#elif defined(DXX_BUILD_DESCENT_II)
			if (((HoardEquipped() && (citem == opt->team_hoard)) || ((citem == opt->team_anarchy) || (citem == opt->capture))) && !menus[opt->closed].value && !menus[opt->refuse].value) 
			{
				menus[opt->refuse].value = 1;
				menus[opt->refuse-1].value = 0;
				menus[opt->refuse-2].value = 0;
			}
#endif
			
			if (menus[opt->coop].value)
			{
				/* In cooperative games, always show all players on the map,
				 * regardless of what the host requested.
				 */
				Netgame.game_flag |= netgame_rule_flags::show_all_players_on_automap;

				Netgame.PlayTimeAllowed = {};
				Netgame.KillGoal = 0;
			}
			if (citem == opt->level)
			{
				auto &slevel = opt->slevel;
#if defined(DXX_BUILD_DESCENT_I)
				if (tolower(static_cast<unsigned>(slevel[0])) == 's')
					Netgame.levelnum = -strtol(&slevel[1], 0, 0);
				else
#endif
					Netgame.levelnum = strtol(slevel.data(), 0, 0);
			}
			
			if (citem == opt->maxnet)
			{
				opt->update_netgame_max_players();
			}

			if ((citem >= opt->mode) && (citem <= opt->mode_end))
			{
				if ( menus[opt->anarchy].value )
					Netgame.gamemode = network_game_type::anarchy;
				
				else if (menus[opt->team_anarchy].value) {
					Netgame.gamemode = network_game_type::team_anarchy;
				}
#if defined(DXX_BUILD_DESCENT_II)
				else if (menus[opt->capture].value)
					Netgame.gamemode = network_game_type::capture_flag;
				else if (HoardEquipped() && menus[opt->hoard].value)
					Netgame.gamemode = network_game_type::hoard;
				else if (HoardEquipped() && menus[opt->team_hoard].value)
					Netgame.gamemode = network_game_type::team_hoard;
#endif
				else if( menus[opt->bounty].value )
					Netgame.gamemode = network_game_type::bounty;
		 		else if (ANARCHY_ONLY_MISSION) {
					int i = 0;
		 			nm_messagebox_str(menu_title{nullptr}, nm_messagebox_tie(TXT_OK), menu_subtitle{TXT_ANARCHY_ONLY_MISSION});
					for (i = opt->mode; i <= opt->mode_end; i++)
						menus[i].value = 0;
					menus[opt->anarchy].value = 1;
		 			return 0;
		 		}
				else if ( menus[opt->robot_anarchy].value ) 
					Netgame.gamemode = network_game_type::robot_anarchy;
				else if ( menus[opt->coop].value ) 
					Netgame.gamemode = network_game_type::cooperative;
				else Int3(); // Invalid mode -- see Rob
			}

			if (menus[opt->closed].value)
				Netgame.game_flag |= netgame_rule_flags::closed;
			else
				Netgame.game_flag &= ~netgame_rule_flags::closed;
			Netgame.RefusePlayers=menus[opt->refuse].value;
			break;
		}
		case event_type::newmenu_selected:
		{
			auto &citem = static_cast<const d_select_event &>(event).citem;
#if defined(DXX_BUILD_DESCENT_I)
			if (Netgame.levelnum < Current_mission->last_secret_level || Netgame.levelnum > Current_mission->last_level || Netgame.levelnum == 0)
#elif defined(DXX_BUILD_DESCENT_II)
			if (Netgame.levelnum < 1 || Netgame.levelnum > Current_mission->last_level)
#endif
			{
				auto &slevel = opt->slevel;
				strcpy(slevel.data(), "1");
				window_create<passive_messagebox>(menu_title{TXT_ERROR}, menu_subtitle{TXT_LEVEL_OUT_RANGE}, TXT_OK, grd_curscreen->sc_canvas);
				return 1;
			}

			if (citem==opt->moreopts)
			{
				more_game_options_menu::net_udp_more_game_options(menus[opt->coop].value);
				return 1;
			}
			if (citem==opt->start_game)
				return !net_udp_start_game();
			return 1;
		}
		default:
			break;
	}
	
	return 0;
}

}

namespace dsx {

window_event_result net_udp_setup_game()
{
	param_opt opt;
	auto &m = opt.m;
	char level_text[32];

	net_udp_init();

	multi_new_game();

	change_playernum_to(0);

	{
		const player *const self = &get_local_player();
		range_for (auto &i, Players)
			if (&i != self)
				i.callsign = {};
	}

	Netgame.max_numplayers = MAX_PLAYERS;
	Netgame.KillGoal=0;
	Netgame.PlayTimeAllowed = {};
#if defined(DXX_BUILD_DESCENT_I)
	Netgame.RefusePlayers=0;
#elif defined(DXX_BUILD_DESCENT_II)
	Netgame.Allow_marker_view=1;
	Netgame.ThiefModifierFlags = 0;
#endif
	Netgame.difficulty=PlayerCfg.DefaultDifficulty;
	Netgame.PacketsPerSec=DEFAULT_PPS;
	snprintf(Netgame.game_name.data(), Netgame.game_name.size(), "%s%s", static_cast<const char *>(InterfaceUniqueState.PilotName), TXT_S_GAME);
	reset_UDP_MyPort();
	Netgame.ShufflePowerupSeed = 0;
	Netgame.BrightPlayers = 1;
	Netgame.InvulAppear = 4;
	Netgame.SecludedSpawns = MAX_PLAYERS - 1;
	Netgame.AllowedItems = Netgame.MaskAllKnownAllowedItems;
	Netgame.PacketLossPrevention = 1;
	Netgame.NoFriendlyFire = 0;
	Netgame.MouselookFlags = 0;
	Netgame.PitchLockFlags = 0;

#if DXX_USE_TRACKER
	Netgame.Tracker = 1;
#endif

	read_netgame_profile(&Netgame);

#if defined(DXX_BUILD_DESCENT_II)
	if (!HoardEquipped() && (Netgame.gamemode == network_game_type::hoard || Netgame.gamemode == network_game_type::team_hoard)) // did we restore a hoard mode but don't have hoard installed right now? then fall back to anarchy!
		Netgame.gamemode = network_game_type::anarchy;
#endif

	Netgame.mission_name.copy_if(&*Current_mission->filename, Netgame.mission_name.size());
	Netgame.mission_title = Current_mission->mission_name;

	Netgame.levelnum = 1;

	unsigned optnum = 0;
	opt.start_game=optnum;
	nm_set_item_menu(  m[optnum], "Start Game"); optnum++;
	nm_set_item_text(m[optnum], TXT_DESCRIPTION); optnum++;

	nm_set_item_input(m[optnum], Netgame.game_name); optnum++;

#define DXX_LEVEL_FORMAT_LEADER	"%s (1-%d"
#define DXX_LEVEL_FORMAT_TRAILER	")"
#if defined(DXX_BUILD_DESCENT_I)
	if (Current_mission->last_secret_level == -1)
		/* Exactly one secret level */
		snprintf(level_text, sizeof(level_text), DXX_LEVEL_FORMAT_LEADER ", S1" DXX_LEVEL_FORMAT_TRAILER, TXT_LEVEL_, Current_mission->last_level);
	else if (Current_mission->last_secret_level)
		/* More than one secret level */
		snprintf(level_text, sizeof(level_text), DXX_LEVEL_FORMAT_LEADER ", S1-S%d" DXX_LEVEL_FORMAT_TRAILER, TXT_LEVEL_, Current_mission->last_level, -Current_mission->last_secret_level);
	else
		/* No secret levels */
#endif
		snprintf(level_text, sizeof(level_text), DXX_LEVEL_FORMAT_LEADER DXX_LEVEL_FORMAT_TRAILER, TXT_LEVEL_, Current_mission->last_level);
#undef DXX_LEVEL_FORMAT_TRAILER
#undef DXX_LEVEL_FORMAT_LEADER

	nm_set_item_text(m[optnum], level_text); optnum++;

	nm_set_item_input(m[optnum], opt.slevel); optnum++;
	nm_set_item_text(m[optnum], TXT_OPTIONS); optnum++;

	opt.mode = optnum;
	nm_set_item_radio(m[optnum], TXT_ANARCHY, Netgame.gamemode == network_game_type::anarchy, 0); opt.anarchy=optnum; optnum++;
	nm_set_item_radio(m[optnum], TXT_TEAM_ANARCHY, Netgame.gamemode == network_game_type::team_anarchy, 0); opt.team_anarchy=optnum; optnum++;
	nm_set_item_radio(m[optnum], TXT_ANARCHY_W_ROBOTS, Netgame.gamemode == network_game_type::robot_anarchy, 0); opt.robot_anarchy=optnum; optnum++;
	nm_set_item_radio(m[optnum], TXT_COOPERATIVE, Netgame.gamemode == network_game_type::cooperative, 0); opt.coop=optnum; optnum++;
#if defined(DXX_BUILD_DESCENT_II)
	nm_set_item_radio(m[optnum], "Capture the flag", Netgame.gamemode == network_game_type::capture_flag, 0); opt.capture=optnum; optnum++;

	if (HoardEquipped())
	{
		nm_set_item_radio(m[optnum], "Hoard", Netgame.gamemode == network_game_type::hoard, 0); opt.hoard=optnum; optnum++;
		nm_set_item_radio(m[optnum], "Team Hoard", Netgame.gamemode == network_game_type::team_hoard, 0); opt.team_hoard=optnum; optnum++;
	}
	else
	{
		opt.hoard = opt.team_hoard = 0; // NOTE: Make sure if you use these, use them in connection with HoardEquipped() only!
	}
#endif
	nm_set_item_radio(m[optnum], "Bounty", Netgame.gamemode == network_game_type::bounty, 0); opt.mode_end=opt.bounty=optnum; optnum++;

	nm_set_item_text(m[optnum], ""); optnum++;

	const auto closed{underlying_value(Netgame.game_flag & netgame_rule_flags::closed)};
	nm_set_item_radio(m[optnum], "Open game", !Netgame.RefusePlayers && closed, 1); optnum++;
	opt.closed = optnum;
	nm_set_item_radio(m[optnum], TXT_CLOSED_GAME, closed, 1); optnum++;
	opt.refuse = optnum;
	nm_set_item_radio(m[optnum], "Restricted Game              ",Netgame.RefusePlayers,1); optnum++;

	opt.maxnet = optnum;
	opt.update_max_players_string();
	nm_set_item_slider(m[optnum], opt.srmaxnet, Netgame.max_numplayers - 2, 0, Netgame.max_numplayers - 2, opt.max_numplayers_saved_text); optnum++;
	
	opt.moreopts=optnum;
	nm_set_item_menu(  m[optnum], "Advanced Options"); optnum++;

	Assert(optnum <= 20);

#if DXX_USE_TRACKER
	if (Netgame.TrackerNATWarned == TrackerNATHolePunchWarn::Unset)
	{
		const unsigned choice = nm_messagebox_str(menu_title{"NAT Hole Punch"}, nm_messagebox_tie("Yes, let Internet users join", "No, I will configure my router"),
menu_subtitle{"Rebirth now supports automatic\n"
"NAT hole punch through the\n"
"tracker.\n\n"
"This allows Internet users to\n"
"join your game, even if you do\n"
"not configure your router for\n"
"hosting.\n\n"
"Do you want to use this feature?"});
		if (choice <= 1)
			Netgame.TrackerNATWarned = static_cast<TrackerNATHolePunchWarn>(choice + 1);
	}
#endif

	const int i = newmenu_do2(menu_title{nullptr}, menu_subtitle{TXT_NETGAME_SETUP}, unchecked_partial_range(m, optnum), net_udp_game_param_handler, &opt, opt.start_game);

	if (i < 0)
		net_udp_close();

	write_netgame_profile(&Netgame);
#if DXX_USE_TRACKER
	/* Force off _after_ writing profile, so that command line does not
	 * change ngp file.
	 */
	if (CGameArg.MplTrackerAddr.empty())
		Netgame.Tracker = 0;
#endif

	return (i >= 0) ? window_event_result::close : window_event_result::handled;
}

namespace {

static void net_udp_set_game_mode(const network_game_type gamemode)
{
	Show_kill_list = show_kill_list_mode::_1;

	if (gamemode == network_game_type::anarchy)
		Game_mode = game_mode_flags::anarchy_no_robots;
	else if (gamemode == network_game_type::robot_anarchy)
		Game_mode = game_mode_flags::anarchy_with_robots;
	else if (gamemode == network_game_type::cooperative) 
		Game_mode = game_mode_flags::cooperative;
#if defined(DXX_BUILD_DESCENT_II)
	else if (gamemode == network_game_type::capture_flag)
		{
		 Game_mode = game_mode_flags::capture_flag;
		Show_kill_list = show_kill_list_mode::team_kills;
		}
	else if (HoardEquipped() && gamemode == network_game_type::hoard)
		Game_mode = game_mode_flags::hoard;
	else if (HoardEquipped() && gamemode == network_game_type::team_hoard)
		 {
		Game_mode = game_mode_flags::team_hoard;
 		Show_kill_list = show_kill_list_mode::team_kills;
		 }
#endif
	else if (gamemode == network_game_type::bounty)
		Game_mode = game_mode_flags::bounty;
	else if (gamemode == network_game_type::team_anarchy)
	{
		Game_mode = game_mode_flags::team_anarchy_no_robots;
		Show_kill_list = show_kill_list_mode::team_kills;
	}
	else
		Int3();
}

void net_udp_read_sync_packet(const uint8_t *data, uint_fast32_t data_len, const _sockaddr &sender_addr)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	auto &vmobjptridx = Objects.vmptridx;
	if (data)
	{
		net_udp_process_game_info_heavy(data, data_len, sender_addr);
	}

	N_players = Netgame.numplayers;
	GameUniqueState.Difficulty_level = Netgame.difficulty;
	Network_status = Netgame.game_status;

	if (Netgame.segments_checksum != my_segments_checksum)
	{
		Network_status = network_state::menu;
		net_udp_close();
		nm_messagebox_str(menu_title{TXT_ERROR}, nm_messagebox_tie(TXT_OK), menu_subtitle{TXT_NETLEVEL_NMATCH});
		throw multi::level_checksum_mismatch();
	}

	// Discover my player number

	auto &temp_callsign = InterfaceUniqueState.PilotName;
	
	Player_num = MULTI_PNUM_UNDEF;

	for (unsigned i = 0; i < N_players; ++i)
	{
		if (i == Netgame.protocol.udp.your_index && Netgame.players[i].callsign == temp_callsign)
		{
			if (Player_num!=MULTI_PNUM_UNDEF) {
				Int3(); // Hey, we've found ourselves twice
				Network_status = network_state::menu;
				return; 
			}
			change_playernum_to(i);
		}
		auto &plr = *vmplayerptr(i);
		plr.callsign = Netgame.players[i].callsign;
		plr.connected = Netgame.players[i].connected;
		auto &objp = *vmobjptr(plr.objnum);
		auto &player_info = objp.ctype.player_info;
		player_info.net_kills_total = Netgame.player_kills[i];
		player_info.net_killed_total = Netgame.killed[i];
		if ((Network_rejoined) || (i != Player_num))
			player_info.mission.score = Netgame.player_score[i];
	}
	kill_matrix = Netgame.kills;

	if (Player_num >= MAX_PLAYERS)
	{
		Network_status = network_state::menu;
		throw multi::local_player_not_playing();
	}

#if defined(DXX_BUILD_DESCENT_I)
	{
		auto &player_info = get_local_plrobj().ctype.player_info;
		PlayerCfg.NetlifeKills -= player_info.net_kills_total;
		PlayerCfg.NetlifeKilled -= player_info.net_killed_total;
	}
#endif

	auto &plr = get_local_player();
	if (Network_rejoined)
	{
		net_udp_process_monitor_vector(Netgame.monitor_vector);
		plr.time_level = Netgame.level_time;
	}

	team_kills = Netgame.team_kills;
	plr.connected = player_connection_status::playing;
	Netgame.players[Player_num].connected = player_connection_status::playing;
	Netgame.players[Player_num].rank=GetMyNetRanking();

	if (!Network_rejoined)
	{
		for (unsigned i = 0; i < NumNetPlayerPositions; ++i)
		{
			const auto &&o = vmobjptridx(vcplayerptr(i)->objnum);
			const auto &p = Player_init[Netgame.locations[i]];
			o->pos = p.pos;
			o->orient = p.orient;
			obj_relink(vmobjptr, vmsegptr, o, vmsegptridx(p.segnum));
		}
	}

	get_local_plrobj().type = OBJ_PLAYER;

	Network_status = network_state::playing;
	multi_sort_kill_list();
}

}
}

namespace {

static int net_udp_send_sync(void)
{
	const auto supported_start_positions_on_level = NumNetPlayerPositions;
	// Check if there are enough starting positions
	if (supported_start_positions_on_level < Netgame.max_numplayers)
	{
		nm_messagebox(menu_title{TXT_ERROR}, {TXT_OK}, "Not enough start positions\n(set %d got %d)\nNetgame aborted", Netgame.max_numplayers, supported_start_positions_on_level);
		// Tell everyone we're bailing
		Netgame.numplayers = 0;
		for (unsigned i = 1; i < N_players; ++i)
		{
			if (vcplayerptr(i)->connected == player_connection_status::disconnected)
				continue;
			const auto &addr = Netgame.players[i].protocol.udp.addr;
			multi::udp::dispatch->kick_player(addr, kick_player_reason::aborted);
			net_udp_send_game_info(addr, &addr, upid::game_info);
		}
		net_udp_broadcast_game_info(upid::game_info_lite);
		return -1;
	}

	// Randomize their starting locations...
	d_srand(static_cast<fix>(timer_query()));
	for (auto &plr : partial_range(Players, supported_start_positions_on_level))
		if (auto &connected = plr.connected; connected != player_connection_status::disconnected)
			connected = player_connection_status::playing; // Get rid of endlevel connect statuses
	auto &&locations = partial_range(Netgame.locations, supported_start_positions_on_level);
	std::iota(locations.begin(), locations.end(), 0);
	if (!(Game_mode & GM_MULTI_COOP))
	{
		/* In cooperative games, use the locations in sequential order.
		 * In non-cooperative games, shuffle.
		 *
		 * High quality randomness is not required here.  Anything that
		 * seems random to users replaying a level should suffice.
		 */
		std::minstd_rand mrd(timer_query());
		std::shuffle(locations.begin(), locations.end(), mrd);
	}

	// Push current data into the sync packet

	net_udp_update_netgame();
	Netgame.game_status = network_state::playing;
	Netgame.segments_checksum = my_segments_checksum;

	for (unsigned i = 0; i < N_players; ++i)
	{
		if (vcplayerptr(i)->connected == player_connection_status::disconnected || i == Player_num)
			continue;
		const auto &addr = Netgame.players[i].protocol.udp.addr;
		net_udp_send_game_info(addr, &addr, upid::sync);
	}

	net_udp_read_sync_packet(nullptr, 0, Netgame.players[0].protocol.udp.addr); // Read it myself, as if I had sent it
	return 0;
}

}

namespace dsx {
namespace {
static int net_udp_select_players()
{
	int j;
	char text[MAX_PLAYERS+4][45];
	char subtitle[50];
	unsigned save_nplayers;              //how may people would like to join

	if (Netgame.ShufflePowerupSeed)
	{
		unsigned seed = 0;
		try {
			seed = std::random_device()();
			if (!seed)
				/* random_device can return any number, including zero.
				 * Rebirth treats zero specially, interpreting it as a
				 * request not to shuffle.  Prevent a zero from
				 * random_device being interpreted as a request not to
				 * shuffle.
				 */
				seed = 1;
		} catch (const std::exception &e) {
			con_printf(CON_URGENT, "Failed to generate random number: %s", e.what());
			/* Fall out without setting `seed`, so that the option is
			 * disabled until the user notices the message and
			 * resolves the problem.
			 */
		}
		Netgame.ShufflePowerupSeed = seed;
	}

	net_udp_add_player(UDP_sequence_request_packet{GetMyNetRanking(), InterfaceUniqueState.PilotName, 0}, {});
	start_poll_menu_items spd;
		
	for (int i=0; i< MAX_PLAYERS+4; i++ ) {
		snprintf(text[i], sizeof(text[i]), "%d. ", i + 1);
		nm_set_item_checkbox(spd.m[i], text[i], 0);
	}

	spd.m[0].value = 1;                         // Assume server will play...

	const auto &&rankstr = GetRankStringWithSpace(Netgame.players[Player_num].rank);
	snprintf( text[0], sizeof(text[0]), "%d. %s%s%-20s", 1, rankstr.first, rankstr.second, static_cast<const char *>(get_local_player().callsign));

	snprintf(subtitle, sizeof(subtitle), "%s %d %s", TXT_TEAM_SELECT, Netgame.max_numplayers, TXT_TEAM_PRESS_ENTER);

#if DXX_USE_TRACKER
	if( Netgame.Tracker )
	{
		TrackerAckStatus = TrackerAckState::TACK_NOCONNECTION;
		TrackerAckTime = timer_query();
		udp_tracker_register();
	}
#endif

GetPlayersAgain:
	j = newmenu_do2(menu_title{nullptr}, menu_subtitle{subtitle}, spd.m, net_udp_start_poll, &spd, 1);

	save_nplayers = N_players;

	if (j<0) 
	{
		// Aborted!
		// Dump all players and go back to menu mode
abort:
		// Tell everyone we're bailing
		Netgame.numplayers = 0;
		for (unsigned i = 1; i < save_nplayers; ++i)
		{
			if (vcplayerptr(i)->connected == player_connection_status::disconnected)
				continue;
			const auto &addr = Netgame.players[i].protocol.udp.addr;
			multi::udp::dispatch->kick_player(addr, kick_player_reason::aborted);
			net_udp_send_game_info(addr, &addr, upid::game_info);
		}
		net_udp_broadcast_game_info(upid::game_info_lite);
		Netgame.numplayers = save_nplayers;

		Network_status = network_state::menu;
#if DXX_USE_TRACKER
		if( Netgame.Tracker )
			udp_tracker_unregister();
#endif
		return(0);
	}
	// Count number of players chosen

	N_players = 0;
	range_for (auto &i, partial_const_range(spd.m, save_nplayers))
	{
		if (i.value)
			N_players++;
	}
	
	if ( N_players > Netgame.max_numplayers) {
		nm_messagebox(menu_title{TXT_ERROR}, {TXT_OK}, "%s %d %s", TXT_SORRY_ONLY, Netgame.max_numplayers, TXT_NETPLAYERS_IN);
		N_players = save_nplayers;
		goto GetPlayersAgain;
	}

// Let host join without Client available. Let's see if our players like that
	// Remove players that aren't marked.
	N_players = 0;
	for (int i=0; i<save_nplayers; i++ )
	{
		if (spd.m[i].value) 
		{
			if (i > N_players)
			{
				Netgame.players[N_players].callsign = Netgame.players[i].callsign;
				Netgame.players[N_players].rank=Netgame.players[i].rank;
			}
			vmplayerptr(N_players)->connected = player_connection_status::playing;
			N_players++;
		}
		else
		{
			multi::udp::dispatch->kick_player(Netgame.players[i].protocol.udp.addr, kick_player_reason::dork);
		}
	}

	range_for (auto &i, partial_range(Netgame.players, N_players, Netgame.players.size()))
	{
		i.callsign = {};
		i.rank = netplayer_info::player_rank::None;
	}

#if defined(DXX_BUILD_DESCENT_I)
	if (Netgame.gamemode == network_game_type::team_anarchy)
#elif defined(DXX_BUILD_DESCENT_II)
	if (Netgame.gamemode == network_game_type::team_anarchy ||
	    Netgame.gamemode == network_game_type::capture_flag ||
		Netgame.gamemode == network_game_type::team_hoard)
#endif
		 if (run_blocking_newmenu<net_udp_select_teams_menu>(N_players, *grd_curcanv) == -1)
			goto abort;
	return(1);
}
}
}

namespace {

static int net_udp_start_game()
{
	int i;

	i = udp_open_socket(UDP_Socket[0], UDP_MyPort);

	if (i != 0)
		return 0;
	
	if (UDP_MyPort != UDP_PORT_DEFAULT)
		i = udp_open_socket(UDP_Socket[1], UDP_PORT_DEFAULT); // Default port open for Broadcasts

	if (i != 0)
		return 0;

	d_srand(static_cast<fix>(timer_query()));
	Netgame.protocol.udp.GameID=d_rand();

	N_players = 0;
	Netgame.game_status = network_state::starting;
	Netgame.numplayers = 0;

	Network_status = network_state::starting;

	net_udp_set_game_mode(Netgame.gamemode);

	Netgame.protocol.udp.your_index = 0; // I am Host. I need to know that y'know? For syncing later.
	
	if (!net_udp_select_players()
		|| StartNewLevel(Netgame.levelnum) == window_event_result::close)
	{
		Game_mode = {};
		return 0;	// see if we want to tweak the game we setup
	}
	state_set_next_autosave(GameUniqueState, Netgame.MPGameplayOptions.AutosaveInterval);
	net_udp_broadcast_game_info(upid::game_info_lite); // game started. broadcast our current status to everyone who wants to know

	return 1;	// don't keep params menu or mission listbox (may want to join a game next time)
}

static int net_udp_wait_for_sync(void)
{
	char text[60];
	int choice=0;
	
	Network_status = network_state::waiting;

	std::array<newmenu_item, 2> m{{
		newmenu_item::nm_item_text{text},
		newmenu_item::nm_item_text{TXT_NET_LEAVE},
	}};
	auto i = net_udp_send_request();

	if (i >= MAX_PLAYERS)
		return(-1);

	snprintf(text, sizeof(text), "%s\n'%s' %s", TXT_NET_WAITING, static_cast<const char *>(Netgame.players[i].callsign), TXT_NET_TO_ENTER );

	while (choice > -1)
	{
		timer_update();
		choice = newmenu_do2(menu_title{nullptr}, menu_subtitle{TXT_WAIT}, m, net_udp_sync_poll, unused_newmenu_userdata);
	}

	if (Network_status != network_state::playing)
	{
		const std::array<uint8_t, 1> buf{{
			underlying_value(upid::quit_joining)
		}};
		dxx_sendto(UDP_Socket[0], buf, 0, Netgame.players[0].protocol.udp.addr);
		N_players = 0;
		Game_mode = {};
		return(-1);     // they cancelled
	}
	return(0);
}

static int net_udp_request_poll( newmenu *,const d_event &event, const unused_newmenu_userdata_t *)
{
	// Polling loop for waiting-for-requests menu
	int num_ready = 0;

	if (event.type != event_type::window_draw)
		return 0;
	net_udp_listen();
	net_udp_timeout_check(timer_query());

	range_for (auto &i, partial_const_range(Players, N_players))
	{
		if (i.connected == player_connection_status::playing || i.connected == player_connection_status::disconnected)
			num_ready++;
	}

	if (num_ready == N_players) // All players have checked in or are disconnected
	{
		timer_delay_ms(50);
		return -2;
	}
	
	return 0;
}

static int net_udp_wait_for_requests(void)
{
	// Wait for other players to load the level before we send the sync
	int choice;
	std::array<newmenu_item, 1> m{{
		newmenu_item::nm_item_text{TXT_NET_LEAVE},
	}};
	Network_status = network_state::waiting;
	net_udp_flush(UDP_Socket);

	get_local_player().connected = player_connection_status::playing;

menu:
	choice = newmenu_do2(menu_title{nullptr}, menu_subtitle{TXT_WAIT}, m, net_udp_request_poll, unused_newmenu_userdata);

	if (choice == -1)
	{
		// User aborted
		choice = nm_messagebox_str(menu_title{nullptr}, nm_messagebox_tie(TXT_YES, TXT_NO, TXT_START_NOWAIT), menu_subtitle{TXT_QUITTING_NOW});
		if (choice == 2) {
			N_players = 1;
			return 0;
		}
		if (choice != 0)
			goto menu;
		
		// User confirmed abort
		
		for (unsigned i = 0; i < N_players; ++i)
		{
			if (vcplayerptr(i)->connected != player_connection_status::disconnected && i != Player_num)
			{
				multi::udp::dispatch->kick_player(Netgame.players[i].protocol.udp.addr, kick_player_reason::aborted);
			}
		}
		return -1;
	}
	else if (choice != -2)
		goto menu;

	return 0;
}

}

namespace dsx {
namespace multi {
namespace udp {
/* Do required syncing after each level, before starting new one */
window_event_result dispatch_table::level_sync() const
{
	int result = 0;

	UDP_MData = {};
	net_udp_noloss_init_mdata_queue();

	net_udp_flush(UDP_Socket); // Flush any old packets

	if (N_players == 0)
		result = net_udp_wait_for_sync();
	else if (multi_i_am_master())
	{
		result = net_udp_wait_for_requests();
		if (!result)
			result = net_udp_send_sync();
	}
	else
		result = net_udp_wait_for_sync();

	if (result)
	{
		get_local_player().connected = player_connection_status::disconnected;
		dispatch->send_endlevel_packet();
		show_menus();
		net_udp_close();
		return window_event_result::close;
	}
	return window_event_result::handled;
}
}
}

namespace {

int net_udp_do_join_game()
{
	if (Netgame.game_status == network_state::endlevel)
	{
		struct error_game_between_levels : passive_messagebox
		{
			error_game_between_levels() :
				passive_messagebox(menu_title{TXT_SORRY}, menu_subtitle{TXT_NET_GAME_BETWEEN2}, TXT_OK, grd_curscreen->sc_canvas)
				{
				}
		};
		run_blocking_newmenu<error_game_between_levels>();
		return 0;
	}

	// Check for valid mission name
	{
		mission_entry_predicate mission_predicate;
		mission_predicate.filesystem_name = Netgame.mission_name;
#if defined(DXX_BUILD_DESCENT_II)
		/* FIXME: This should be set to true and the version set
		 * accordingly.  However, currently the host does not provide
		 * the mission version to the guests.
		 */
		mission_predicate.check_version = false;
#endif
	if (const auto errstr = load_mission_by_name(mission_predicate, mission_name_type::guess))
	{
		struct error_mission_not_found :
			std::array<char, 96>,
			passive_messagebox
		{
			error_mission_not_found(const char *errstr) :
				passive_messagebox(menu_title{nullptr}, menu_subtitle{prepare_subtitle(*this, errstr)}, TXT_OK, grd_curscreen->sc_canvas)
				{
				}
			static const char *prepare_subtitle(std::array<char, 96> &b, const char *errstr)
			{
				auto r = b.data();
				std::snprintf(r, b.size(), "%s\n\n%s", TXT_MISSION_NOT_FOUND, errstr);
				return r;
			}
		};
		run_blocking_newmenu<error_mission_not_found>(errstr);
		return 0;
	}
	}

#if defined(DXX_BUILD_DESCENT_II)
	if (is_D2_OEM)
	{
		if (Netgame.levelnum>8)
		{
			struct error_using_oem_data : passive_messagebox
			{
				error_using_oem_data() :
					passive_messagebox(menu_title{nullptr}, menu_subtitle{"You are using OEM game data.  You can only play the first 8 levels."}, TXT_OK, grd_curscreen->sc_canvas)
					{
					}
			};
			run_blocking_newmenu<error_using_oem_data>();
			return 0;
		}
	}

	if (is_MAC_SHARE)
	{
		if (Netgame.levelnum > 4)
		{
			struct error_using_mac_shareware : passive_messagebox
			{
				error_using_mac_shareware() :
					passive_messagebox(menu_title{nullptr}, menu_subtitle{"You are using Mac shareware data.  You can only play the first 4 levels."}, TXT_OK, grd_curscreen->sc_canvas)
					{
					}
			};
			run_blocking_newmenu<error_using_mac_shareware>();
			return 0;
		}
	}

	if (!HoardEquipped() && (Netgame.gamemode == network_game_type::hoard || Netgame.gamemode == network_game_type::team_hoard))
	{
		struct error_hoard_not_available : passive_messagebox
		{
			error_hoard_not_available() :
				passive_messagebox(menu_title{TXT_SORRY}, menu_subtitle{"That is a hoard game.\nYou do not have HOARD.HAM installed.\nYou cannot join."}, TXT_OK, grd_curscreen->sc_canvas)
				{
				}
		};
		run_blocking_newmenu<error_hoard_not_available>();
		return 0;
	}

	Network_status = network_state::browsing; // We are looking at a game menu
#endif

	if (net_udp_can_join_netgame(&Netgame) == join_netgame_status_code::game_in_disallowed_state)
	{
		struct error_cannot_join_game : passive_messagebox
		{
			error_cannot_join_game() :
				passive_messagebox(menu_title{TXT_SORRY}, menu_subtitle{Netgame.numplayers == Netgame.max_numplayers ? TXT_GAME_FULL : TXT_IN_PROGRESS}, TXT_OK, grd_curscreen->sc_canvas)
				{
				}
		};
		run_blocking_newmenu<error_cannot_join_game>();
		return 0;
	}

	// Choice is valid, prepare to join in
	GameUniqueState.Difficulty_level = Netgame.difficulty;
	change_playernum_to(1);

	net_udp_set_game_mode(Netgame.gamemode);

	return StartNewLevel(Netgame.levelnum) == window_event_result::handled;     // look ma, we're in a game!!! (If level syncing didn't fail -kreatordxx)
}

}

namespace multi {
namespace udp {
void dispatch_table::leave_game() const
{
	int nsave;

	dispatch->do_protocol_frame(1, 1);

	if (multi_i_am_master())
	{
		while (Network_sending_extras>1 && Player_joining_extras!=-1)
		{
			timer_update();
			net_udp_send_extras();
		}

		Netgame.numplayers = 0;
		nsave=N_players;
		N_players=0;
		for (unsigned i = 1; i < nsave; ++i)
		{
			if (vcplayerptr(i)->connected == player_connection_status::disconnected)
				continue;
			const auto &addr = Netgame.players[i].protocol.udp.addr;
			net_udp_send_game_info(addr, &addr, upid::game_info);
		}
		net_udp_broadcast_game_info(upid::game_info_lite);
		N_players=nsave;
#if DXX_USE_TRACKER
		if( Netgame.Tracker )
			udp_tracker_unregister();
#endif
	}

	get_local_player().connected = player_connection_status::disconnected;
	change_playernum_to(0);
#if defined(DXX_BUILD_DESCENT_II)
	write_player_file();
#endif

	net_udp_flush(UDP_Socket);
	net_udp_close();
}
}
}
}

namespace dcx {
namespace {

static void net_udp_flush(RAIIsocket &s)
{
	if (!s)
		return;
	unsigned i = 0;
	struct _sockaddr sender_addr;
	std::array<uint8_t, UPID_MAX_SIZE> packet;
	while (udp_receive_packet(s, packet, sender_addr) > 0)
		++i;
	if (i)
		con_printf(CON_VERBOSE, "Flushed %u UDP packets from socket %i", i, static_cast<int>(s));
}

void net_udp_flush(std::array<RAIIsocket, 2> &UDP_Socket)
{
	for (auto &s : UDP_Socket)
		net_udp_flush(s);
}

}
}

namespace {

static void net_udp_listen(RAIIsocket &sock)
{
	if (!sock)
		return;
	struct _sockaddr sender_addr;
	std::array<uint8_t, UPID_MAX_SIZE> packet;
	for (;;)
	{
		const auto size = udp_receive_packet(sock, packet, sender_addr);
		if (!(size > 0))
			break;
		/* Casting from ssize_t to std::size_t is safe here.  Only negative
		 * values would be affected by the narrowing conversion, and a negative
		 * value would have caused the preceding `if` test to `break`.
		 */
		net_udp_process_packet(LevelSharedRobotInfoState, {packet.data(), static_cast<std::size_t>(size)}, sender_addr);
	}
}

void net_udp_listen()
{
	range_for (auto &s, UDP_Socket)
		net_udp_listen(s);
}

}

namespace dsx {
namespace multi {
namespace udp {

void dispatch_table::send_data(const std::span<const uint8_t> buf, const multiplayer_data_priority priority) const
{
	const auto ptr = buf.data();
	const auto len = buf.size();
#if DXX_HAVE_POISON_VALGRIND
	VALGRIND_CHECK_MEM_IS_DEFINED(ptr, len);
#endif
	char check;

	if ((UDP_MData.mbuf_size+len) > UPID_MDATA_BUF_SIZE )
	{
		check = ptr[0];
		net_udp_send_mdata(0, timer_query());
		if (UDP_MData.mbuf_size != 0)
			Int3();
		Assert(check == ptr[0]);
		(void)check;
	}

	Assert(UDP_MData.mbuf_size+len <= UPID_MDATA_BUF_SIZE);

	memcpy( &UDP_MData.mbuf[UDP_MData.mbuf_size], ptr, len );
	UDP_MData.mbuf_size += len;

	if (priority != multiplayer_data_priority::_0)
		net_udp_send_mdata(priority == multiplayer_data_priority::_2 ? 1 : 0, timer_query());
}

}
}
}

namespace {

void net_udp_timeout_check(fix64 time)
{
	static fix64 last_timeout_time = 0;
	if (time>=last_timeout_time+F1_0)
	{
		// Check for player timeouts
		for (playernum_t i = 0; i < N_players; i++)
		{
			if (i != Player_num && vcplayerptr(i)->connected != player_connection_status::disconnected)
			{
				if ((Netgame.players[i].LastPacketTime == 0) || (Netgame.players[i].LastPacketTime > time))
				{
					Netgame.players[i].LastPacketTime = time;
				}
				else if ((time - Netgame.players[i].LastPacketTime) > UDP_TIMEOUT)
				{
					multi_disconnect_player(i);
				}
			}
		}
		last_timeout_time = time;
	}
}

}

namespace dsx {
namespace multi {
namespace udp {
void dispatch_table::do_protocol_frame(int force, int listen) const
{
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;
	static fix64 last_pdata_time = 0, last_mdata_time = 16, last_endlevel_time = 32, last_bcast_time = 48, last_resync_time = 64;

	if (!(Game_mode&GM_NETWORK) || !UDP_Socket[0])
		return;

	const fix64 time = timer_update();

	if (WaitForRefuseAnswer && time>(RefuseTimeLimit+(F1_0*12)))
		WaitForRefuseAnswer=0;

	// Send positional update either in the regular PPS interval OR if forced
	if (force || (time >= (last_pdata_time+(F1_0/Netgame.PacketsPerSec))))
	{
		last_pdata_time = time;
		net_udp_send_pdata();
#if defined(DXX_BUILD_DESCENT_II)
                multi_send_thief_frame();
#endif
	}
	
	if (force || (time >= (last_mdata_time+(F1_0/10))))
	{
		last_mdata_time = time;
		multi_send_robot_frame();
		net_udp_send_mdata(0, time);
	}

	net_udp_noloss_process_queue(time);

	if (VerifyPlayerJoined!=-1 && time >= last_resync_time+F1_0)
	{
		last_resync_time = time;
		net_udp_resend_sync_due_to_packet_loss(); // This will resend to UDP_sync_player
	}

	if (time >= last_endlevel_time + F1_0 && LevelUniqueControlCenterState.Control_center_destroyed)
	{
		last_endlevel_time = time;
		dispatch->send_endlevel_packet();
	}

	// broadcast lite_info every 10 seconds
	if (multi_i_am_master() && time>=last_bcast_time+(F1_0*10))
	{
		last_bcast_time = time;
		net_udp_broadcast_game_info(upid::game_info_lite);
#if DXX_USE_TRACKER
		if ( Netgame.Tracker )
			udp_tracker_register();
#endif
	}

	net_udp_ping_frame(time);
#if DXX_USE_TRACKER
	udp_tracker_verify_ack_timeout();
#endif

	if (listen)
	{
		net_udp_timeout_check(time);
		net_udp_listen();
		if (Network_send_objects)
			net_udp_send_objects(network_player_added);
		if (Network_sending_extras && VerifyPlayerJoined==-1)
			net_udp_send_extras();
	}

	udp_traffic_stat();
}
}
}
}

namespace {

/* CODE FOR PACKET LOSS PREVENTION - START */
/* This code tries to make sure that packets with opcode upid::mdata_pneedack aren't lost and sent and received in order. */
/*
 * Adds a packet to our queue. Should be called when an IMPORTANT mdata packet is created.
 * player_ack is an array which should contain 0 for each player that needs to send an ACK signal.
 */
static void net_udp_noloss_add_queue_pkt(fix64 time, const std::span<const uint8_t> data, ubyte pnum, const player_acknowledgement_mask &player_ack)
{
	if (!(Game_mode&GM_NETWORK) || !UDP_Socket[0])
		return;

	if (!Netgame.PacketLossPrevention)
		return;

	if (UDP_mdata_queue_highest == UDP_MDATA_STOR_QUEUE_SIZE) // The list is full. That should not happen. But if it does, we must do something.
	{
		con_printf(CON_VERBOSE, "P#%u: MData store list is full!", Player_num);
		if (multi_i_am_master()) // I am host. I will kick everyone who did not ACK the first packet and then remove it.
		{
			for ( int i=1; i<N_players; i++ )
				if (UDP_mdata_queue[0].player_ack[i] == 0)
					multi::udp::dispatch->kick_player(Netgame.players[i].protocol.udp.addr, kick_player_reason::pkttimeout);
			std::move(std::next(UDP_mdata_queue.begin()), UDP_mdata_queue.end(), UDP_mdata_queue.begin());
			UDP_mdata_queue[UDP_MDATA_STOR_QUEUE_SIZE - 1] = {};
			UDP_mdata_queue_highest--;
		}
		else // I am just a client. I gotta go.
		{
			Netgame.PacketLossPrevention = 0; // Disable PLP - otherwise we get stuck in an infinite loop here. NOTE: We could as well clean the whole queue to continue protect our disconnect signal bit it's not that important - we just wanna leave.
			const auto g = Game_wind;
			if (g)
				g->set_visible(0);
			nm_messagebox_str(menu_title{nullptr}, nm_messagebox_tie(TXT_OK), menu_subtitle{"You left the game. You failed\nsending important packets.\nSorry."});
			if (g)
				g->set_visible(1);
			multi_quit_game = 1;
			game_leave_menus();
		}
		Assert(UDP_mdata_queue_highest == (UDP_MDATA_STOR_QUEUE_SIZE - 1));
	}

	con_printf(CON_VERBOSE, "P#%u: Adding MData pkt_num [%i,%i,%i,%i,%i,%i,%i,%i], type %i from P#%i to MData store list", Player_num, UDP_mdata_trace[0].pkt_num_tosend,UDP_mdata_trace[1].pkt_num_tosend,UDP_mdata_trace[2].pkt_num_tosend,UDP_mdata_trace[3].pkt_num_tosend,UDP_mdata_trace[4].pkt_num_tosend,UDP_mdata_trace[5].pkt_num_tosend,UDP_mdata_trace[6].pkt_num_tosend,UDP_mdata_trace[7].pkt_num_tosend, data[0], pnum);
	UDP_mdata_queue[UDP_mdata_queue_highest].used = 1;
	UDP_mdata_queue[UDP_mdata_queue_highest].pkt_initial_timestamp = time;
	for (unsigned i = 0; i < MAX_PLAYERS; ++i)
	{
		if (i == Player_num || player_ack[i] || vcplayerptr(i)->connected == player_connection_status::disconnected)	// if player me, is not playing or does not require an ACK, do not add timestamp or increment pkt_num
			continue;
		
		UDP_mdata_queue[UDP_mdata_queue_highest].pkt_timestamp[i] = time;
		UDP_mdata_queue[UDP_mdata_queue_highest].pkt_num[i] = UDP_mdata_trace[i].pkt_num_tosend;
		UDP_mdata_trace[i].pkt_num_tosend++;
		if (UDP_mdata_trace[i].pkt_num_tosend > UDP_MDATA_PKT_NUM_MAX)
			UDP_mdata_trace[i].pkt_num_tosend = UDP_MDATA_PKT_NUM_MIN;
	}
	UDP_mdata_queue[UDP_mdata_queue_highest].Player_num = pnum;
	UDP_mdata_queue[UDP_mdata_queue_highest].player_ack = player_ack;
	memcpy(&UDP_mdata_queue[UDP_mdata_queue_highest].data, data.data(), UDP_mdata_queue[UDP_mdata_queue_highest].data_size = data.size());
	UDP_mdata_queue_highest++;
}

/*
 * We have received a MDATA packet. Send ACK response to sender!
 * Make sure this packet has the expected packet number so we get them all in order. If not, reject it and await further packets.
 * Also check in our UDP_mdata_trace list, if we got this packet already. If yes, return 0 so do not process it!
 */
static int net_udp_noloss_validate_mdata(uint32_t pkt_num, ubyte sender_pnum, const _sockaddr &sender_addr)
{
	ubyte pkt_sender_pnum = sender_pnum;
	int len = 0;

	// If we are a client, we get all our packets from the host.
	if (!multi_i_am_master())
		sender_pnum = 0;

	// Check if this comes from a valid IP
	if (sender_addr != Netgame.players[sender_pnum].protocol.udp.addr)
		return 0;

        // Prepare the ACK (but do not send, yet)
	std::array<uint8_t, 7> buf;
        buf[len] = underlying_value(upid::mdata_ack);											len++;
        buf[len] = Player_num;												len++;
        buf[len] = pkt_sender_pnum;											len++;
	PUT_INTEL_INT(&buf[len], pkt_num);										len += 4;

        // Make sure this is the packet we are expecting!
        if (UDP_mdata_trace[sender_pnum].pkt_num_torecv != pkt_num)
        {
                range_for (auto &i, partial_const_range(UDP_mdata_trace[sender_pnum].pkt_num, UDP_MDATA_STOR_QUEUE_SIZE))
                {
                        if (pkt_num == i) // We got this packet already - need to REsend ACK
                        {
                                con_printf(CON_VERBOSE, "P#%u: Resending MData ACK for pkt %i we already got by pnum %i",Player_num, pkt_num, sender_pnum);
                                dxx_sendto(UDP_Socket[0], buf, 0, sender_addr);
                                return 0;
                        }
                }
                con_printf(CON_VERBOSE, "P#%u: Rejecting MData pkt %i - expected %i by pnum %i",Player_num, pkt_num, UDP_mdata_trace[sender_pnum].pkt_num_torecv, sender_pnum);
                return 0; // Not the right packet and we haven't gotten it, yet either. So bail out and wait for the right one.
        }

	con_printf(CON_VERBOSE, "P#%u: Sending MData ACK for pkt %i by pnum %i",Player_num, pkt_num, sender_pnum);
	dxx_sendto(UDP_Socket[0], buf, 0, sender_addr);

	UDP_mdata_trace[sender_pnum].cur_slot++;
	if (UDP_mdata_trace[sender_pnum].cur_slot >= UDP_MDATA_STOR_QUEUE_SIZE)
		UDP_mdata_trace[sender_pnum].cur_slot = 0;
	UDP_mdata_trace[sender_pnum].pkt_num[UDP_mdata_trace[sender_pnum].cur_slot] = pkt_num;
	UDP_mdata_trace[sender_pnum].pkt_num_torecv++;
	if (UDP_mdata_trace[sender_pnum].pkt_num_torecv > UDP_MDATA_PKT_NUM_MAX)
		UDP_mdata_trace[sender_pnum].pkt_num_torecv = UDP_MDATA_PKT_NUM_MIN;
	return 1;
}

/* We got an ACK by a player. Set this player slot to positive! */
void net_udp_noloss_got_ack(const std::span<const uint8_t> data)
{
	int len = 0;
	ubyte sender_pnum = 0, dest_pnum = 0;

															len++;
	sender_pnum = data[len];											len++;
	dest_pnum = data[len];												len++;
	const uint32_t pkt_num{GET_INTEL_INT(&data[len])};										len += 4;

	for (int i = 0; i < UDP_mdata_queue_highest; i++)
	{
		if ((pkt_num == UDP_mdata_queue[i].pkt_num[sender_pnum]) && (dest_pnum == UDP_mdata_queue[i].Player_num))
		{
			con_printf(CON_VERBOSE, "P#%u: Got MData ACK for pkt_num %i from pnum %i for pnum %i",Player_num, pkt_num, sender_pnum, dest_pnum);
			UDP_mdata_queue[i].player_ack[sender_pnum] = 1;
			break;
		}
	}
}

/* Init/Free the queue. Call at start and end of a game or level. */
void net_udp_noloss_init_mdata_queue(void)
{
	UDP_mdata_queue_highest=0;
	con_printf(CON_VERBOSE, "P#%u: Clearing MData store/trace list",Player_num);
	UDP_mdata_queue = {};
	for (int i = 0; i < MAX_PLAYERS; i++)
		net_udp_noloss_clear_mdata_trace(i);
}

/* Reset the trace list for given player when (dis)connect happens */
void net_udp_noloss_clear_mdata_trace(ubyte player_num)
{
	con_printf(CON_VERBOSE, "P#%u: Clearing trace list for %i",Player_num, player_num);
	UDP_mdata_trace[player_num].pkt_num = {};
	UDP_mdata_trace[player_num].cur_slot = 0;
	UDP_mdata_trace[player_num].pkt_num_torecv = UDP_MDATA_PKT_NUM_MIN;
	UDP_mdata_trace[player_num].pkt_num_tosend = UDP_MDATA_PKT_NUM_MIN;
}

/*
 * The main queue-process function.
 * Check if we can remove a packet from queue, and check if there are packets in queue which we need to re-send
 */
void net_udp_noloss_process_queue(fix64 time)
{
	int total_len = 0;

	if (!(Game_mode&GM_NETWORK) || !UDP_Socket[0])
		return;

	if (!Netgame.PacketLossPrevention)
		return;

	for (int queuec = 0; queuec < UDP_mdata_queue_highest; queuec++)
	{
		int needack = 0;

		// This might happen if we get out ACK's in the wrong order. So ignore that packet for now. It'll resolve itself.
		if (!UDP_mdata_queue[queuec].used)
			continue;

		// Check if at least one connected player has not ACK'd the packet
		for (unsigned plc = 0; plc < MAX_PLAYERS; ++plc)
		{
			// If player is not playing anymore, we can remove him from list. Also remove *me* (even if that should have been done already). Also make sure Clients do not send to anyone else than Host
			if ((vcplayerptr(plc)->connected != player_connection_status::playing || plc == Player_num) || (!multi_i_am_master() && plc > 0))
				UDP_mdata_queue[queuec].player_ack[plc] = 1;

			if (!UDP_mdata_queue[queuec].player_ack[plc])
			{
				// Resend if enough time has passed.
				if (UDP_mdata_queue[queuec].pkt_timestamp[plc] + (F1_0/4) <= time)
				{
					con_printf(CON_VERBOSE, "P#%u: Resending pkt_num %i from pnum %i to pnum %i",Player_num, UDP_mdata_queue[queuec].pkt_num[plc], UDP_mdata_queue[queuec].Player_num, plc);
					UDP_mdata_queue[queuec].pkt_timestamp[plc] = time;
					std::array<uint8_t, sizeof(UDP_mdata_info)> buf{};

					unsigned len = 0;
					// Prepare the packet and send it
					buf[len] = underlying_value(upid::mdata_pneedack);													len++;
					buf[len] = UDP_mdata_queue[queuec].Player_num;								len++;
					PUT_INTEL_INT(&buf[len], UDP_mdata_queue[queuec].pkt_num[plc]);					len += 4;
					memcpy(&buf[len], UDP_mdata_queue[queuec].data.data(), sizeof(char)*UDP_mdata_queue[queuec].data_size);
																								len += UDP_mdata_queue[queuec].data_size;
					dxx_sendto(UDP_Socket[0], std::span(buf).first(len), 0, Netgame.players[plc].protocol.udp.addr);
					total_len += len;
				}
				needack++;
			}
		}

		// Check if we can remove that packet due to to it had no resend's or Timeout
		if (needack==0 || (UDP_mdata_queue[queuec].pkt_initial_timestamp + UDP_TIMEOUT <= time))
		{
			if (needack) // packet timed out but still not all have ack'd.
			{
				if (multi_i_am_master()) // We are host, so we kick the remaining players.
				{
					for ( int plc=1; plc<N_players; plc++ )
						if (UDP_mdata_queue[queuec].player_ack[plc] == 0)
							multi::udp::dispatch->kick_player(Netgame.players[plc].protocol.udp.addr, kick_player_reason::pkttimeout);
				}
				else // We are client, so we gotta go.
				{
					Netgame.PacketLossPrevention = 0; // Disable PLP - otherwise we get stuck in an infinite loop here. NOTE: We could as well clean the whole queue to continue protect our disconnect signal bit it's not that important - we just wanna leave.
					const auto g = Game_wind;
					if (g)
						g->set_visible(0);
					nm_messagebox_str(menu_title{nullptr}, nm_messagebox_tie(TXT_OK), menu_subtitle{"You left the game. You failed\nsending important packets.\nSorry."});
					if (g)
						g->set_visible(1);
					multi_quit_game = 1;
					game_leave_menus();
				}
			}
			con_printf(CON_VERBOSE, "P#%u: Removing stored pkt_num [%i,%i,%i,%i,%i,%i,%i,%i] - missing ACKs: %i",Player_num, UDP_mdata_queue[queuec].pkt_num[0],UDP_mdata_queue[queuec].pkt_num[1],UDP_mdata_queue[queuec].pkt_num[2],UDP_mdata_queue[queuec].pkt_num[3],UDP_mdata_queue[queuec].pkt_num[4],UDP_mdata_queue[queuec].pkt_num[5],UDP_mdata_queue[queuec].pkt_num[6],UDP_mdata_queue[queuec].pkt_num[7], needack); // Just *marked* for removal. The actual process happens further below.
			UDP_mdata_queue[queuec].used = 0;
		}

		// Send up to half our max packet size
		if (total_len >= (UPID_MAX_SIZE/2))
			break;
	}

	// Now that we are done processing the queue, actually remove all unused packets from the top of the list.
	{
		const auto b = UDP_mdata_queue.begin();
		const auto e = std::next(b, UDP_mdata_queue_highest);
		const auto &&first_used_entry{std::ranges::find_if(b, e, [](const UDP_mdata_store &m) { return m.used; })};
		if (first_used_entry != b)
		{
			std::move(first_used_entry, e, b);
			const auto total_used_entries = std::distance(first_used_entry, e);
			UDP_mdata_queue_highest = total_used_entries;
			for (auto first_unused_entry = std::next(b, total_used_entries); first_unused_entry != e; ++first_unused_entry)
				*first_unused_entry = {};
		}
	}
}

}
/* CODE FOR PACKET LOSS PREVENTION - END */

namespace dsx {
namespace multi {
namespace udp {

void dispatch_table::send_data_direct(const std::span<const uint8_t> data, const playernum_t pnum, int needack) const
{
	if (!(Game_mode&GM_NETWORK) || !UDP_Socket[0])
		return;

	if (data.empty())
		return;

	if (!multi_i_am_master() && pnum != 0)
		Error("Client sent direct data to non-Host in net_udp_send_mdata_direct()!\n");

	if (!Netgame.PacketLossPrevention)
		needack = 0;

	std::array<uint8_t, sizeof(UDP_mdata_info)> buf{};
	unsigned len = 0;
	buf[0] = needack
		? underlying_value(upid::mdata_pneedack)
		: underlying_value(upid::mdata_pnorm);
														len++;
	buf[len] = Player_num;											len++;
	if (needack)
	{
		PUT_INTEL_INT(&buf[len], UDP_mdata_trace[pnum].pkt_num_tosend);					len += 4;
	}
	{
		const auto count_to_copy = data.size();
		cf_assert(count_to_copy != std::dynamic_extent);
		memcpy(std::span(buf).subspan(len, count_to_copy).data(), data.data(), count_to_copy);
		len += count_to_copy;
	}

	dxx_sendto(UDP_Socket[0], std::span(buf).first(len), 0, Netgame.players[pnum].protocol.udp.addr);

	if (needack)
	{
		player_acknowledgement_mask player_ack;
		player_ack[pnum] = 0;
		net_udp_noloss_add_queue_pkt(timer_query(), data, Player_num, player_ack);
	}
}

}
}
}

namespace {

void net_udp_send_mdata(int needack, fix64 time)
{
	if (!(Game_mode&GM_NETWORK) || !UDP_Socket[0])
		return;

	if (!(UDP_MData.mbuf_size > 0))
		return;

	if (!Netgame.PacketLossPrevention)
		needack = 0;

	std::array<uint8_t, sizeof(UDP_mdata_info)> buf{};

	unsigned len = 0;
	buf[0] = needack
		? underlying_value(upid::mdata_pneedack)
		: underlying_value(upid::mdata_pnorm);
														len++;
	buf[len] = Player_num;											len++;
	if (needack)												len += 4; // we place the pkt_num later since it changes per player
	memcpy(std::span(buf).subspan(len, UDP_MData.mbuf_size).data(), UDP_MData.mbuf.data(), UDP_MData.mbuf_size);
	len += UDP_MData.mbuf_size;

	player_acknowledgement_mask player_ack;
	if (multi_i_am_master())
	{
		for (unsigned i = 1; i < MAX_PLAYERS; ++i)
		{
			if (vcplayerptr(i)->connected == player_connection_status::playing)
			{
				if (needack) // assign pkt_num
					PUT_INTEL_INT(&buf[2], UDP_mdata_trace[i].pkt_num_tosend);
				dxx_sendto(UDP_Socket[0], std::span(buf).first(len), 0, Netgame.players[i].protocol.udp.addr);
				player_ack[i] = 0;
			}
		}
	}
	else
	{
		if (needack) // assign pkt_num
			PUT_INTEL_INT(&buf[2], UDP_mdata_trace[0].pkt_num_tosend);
		dxx_sendto(UDP_Socket[0], std::span(buf).first(len), 0, Netgame.players[0].protocol.udp.addr);
		player_ack[0] = 0;
	}
	
	if (needack)
		net_udp_noloss_add_queue_pkt(time, std::span{UDP_MData.mbuf.data(), UDP_MData.mbuf_size}, Player_num, player_ack);

	// Clear UDP_MData except pkt_num. That one must not be deleted so we can clearly keep track of important packets.
	UDP_MData.type = 0;
	UDP_MData.Player_num = 0;
	UDP_MData.mbuf_size = 0;
	UDP_MData.mbuf = {};
}

void net_udp_process_mdata(const d_level_shared_robot_info_state &LevelSharedRobotInfoState, const std::span<uint8_t> data, const _sockaddr &sender_addr, int needack)
{
	const unsigned pnum = data[1];
	const unsigned dataoffset = needack ? 6 : 2;

	// Check if packet might be bogus
	if (pnum >= MAX_PLAYERS)
		return;
	if (data.size() > sizeof(UDP_mdata_info))
		return;

	// Check if it came from valid IP
	if (multi_i_am_master())
	{
		if (sender_addr != Netgame.players[pnum].protocol.udp.addr)
		{
			return;
		}
	}
	else
	{
		if (sender_addr != Netgame.players[0].protocol.udp.addr)
		{
			return;
		}
	}

	// Add needack packet and check for possible redundancy
	if (needack)
	{
		if (!net_udp_noloss_validate_mdata(GET_INTEL_INT(&data[2]), pnum, sender_addr))
			return;
	}
	const auto subdata = data.subspan(dataoffset);

	// send this to everyone else (if master)
	if (multi_i_am_master())
	{
		player_acknowledgement_mask player_ack;
		for (unsigned i = 1; i < MAX_PLAYERS; ++i)
		{
			if (i != pnum && vcplayerptr(i)->connected == player_connection_status::playing)
			{
				if (needack)
				{
					player_ack[i] = 0;
					PUT_INTEL_INT(&data[2], UDP_mdata_trace[i].pkt_num_tosend);
				}
				dxx_sendto(UDP_Socket[0], data, 0, Netgame.players[i].protocol.udp.addr);
			}
		}

		if (needack)
			net_udp_noloss_add_queue_pkt(timer_query(), subdata, pnum, player_ack);
	}

	// Check if we are in correct state to process the packet
	if (!(Network_status == network_state::playing || Network_status == network_state::endlevel || Network_status == network_state::waiting))
		return;

	// Process

	multi_process_bigdata(LevelSharedRobotInfoState, pnum, subdata);
}

void net_udp_send_pdata()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	std::array<uint8_t, 3 + quaternionpos::packed_size::value> buf;
	int len = 0;

	if (!(Game_mode&GM_NETWORK) || !UDP_Socket[0])
		return;
	auto &plr = get_local_player();
	if (plr.connected != player_connection_status::playing)
		return;
	if (!(Network_status == network_state::playing || Network_status == network_state::endlevel))
		return;

	buf[len] = underlying_value(upid::pdata);									len++;
	buf[len] = Player_num;									len++;
	buf[len] = underlying_value(plr.connected);						len++;

	const auto qpp{build_quaternionpos(vmobjptr(plr.objnum))};
	PUT_INTEL_SHORT(&buf[len], qpp.orient.w);							len += 2;
	PUT_INTEL_SHORT(&buf[len], qpp.orient.x);							len += 2;
	PUT_INTEL_SHORT(&buf[len], qpp.orient.y);							len += 2;
	PUT_INTEL_SHORT(&buf[len], qpp.orient.z);							len += 2;
	multi_put_vector(&buf[len], qpp.pos);
	len += 12;
	PUT_INTEL_SEGNUM(&buf[len], qpp.segment);						len += 2;
	multi_put_vector(&buf[len], qpp.vel);
	len += 12;
	multi_put_vector(&buf[len], qpp.rotvel);
	len += 12;
	// 46 + 3 = 49

	if (multi_i_am_master())
	{
		for (unsigned i = 1; i < MAX_PLAYERS; ++i)
			if (vcplayerptr(i)->connected != player_connection_status::disconnected)
				dxx_sendto(UDP_Socket[0], buf, 0, Netgame.players[i].protocol.udp.addr);
	}
	else
	{
		dxx_sendto(UDP_Socket[0], buf, 0, Netgame.players[0].protocol.udp.addr);
	}
}

void net_udp_process_pdata(const std::span<const uint8_t> data, const _sockaddr &sender_addr)
{
	UDP_frame_info pd;
	int len = 0;

	if (!((Game_mode & GM_NETWORK) && (Network_status == network_state::playing || Network_status == network_state::endlevel)))
		return;

	len++;

	pd = {};
	const playernum_t playernum = data[len];
	if (playernum >= std::size(Netgame.players))
		return;
	if (sender_addr != Netgame.players[multi_i_am_master() ? playernum : 0].protocol.udp.addr)
		return;

	pd.Player_num = playernum;								len++;
	pd.connected = player_connection_status{data[len]};								len++;
	pd.qpp.orient.w = GET_INTEL_SHORT(&data[len]);					len += 2;
	pd.qpp.orient.x = GET_INTEL_SHORT(&data[len]);					len += 2;
	pd.qpp.orient.y = GET_INTEL_SHORT(&data[len]);					len += 2;
	pd.qpp.orient.z = GET_INTEL_SHORT(&data[len]);					len += 2;
	pd.qpp.pos = multi_get_vector(data.subspan<3 + 8, 12>());
	len += 12;
	if (const auto s{vmsegidx_t::check_nothrow_index(GET_INTEL_SHORT(&data[len]))})
	{
		len += 2;
		pd.qpp.segment = *s;
	}
	else
		return;
	pd.qpp.vel = multi_get_vector(data.subspan<3 + 8 + 12 + 2, 12>());
	len += 12;
	pd.qpp.rotvel = multi_get_vector(data.subspan<3 + 8 + 12 + 2 + 12, 12>());
	len += 12;

	if (multi_i_am_master()) // I am host - must relay this packet to others!
	{
		const unsigned ppn = pd.Player_num;
		if (ppn > 0 && ppn <= N_players && vcplayerptr(ppn)->connected == player_connection_status::playing) // some checking whether this packet is legal
		{
			for (unsigned i = 1; i < MAX_PLAYERS; ++i)
			{
				// not to sender or disconnected/waiting players - right.
				if (i == ppn)
					continue;
				auto &iplr = *vcplayerptr(i);
				if (iplr.connected != player_connection_status::disconnected && iplr.connected != player_connection_status::waiting)
					dxx_sendto(UDP_Socket[0], data, 0, Netgame.players[i].protocol.udp.addr);
			}
		}
	}

	net_udp_read_pdata_packet (&pd);
}

void net_udp_read_pdata_packet(UDP_frame_info *pd)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	const unsigned TheirPlayernum = pd->Player_num;
	auto &tplr = *vmplayerptr(TheirPlayernum);
	const auto TheirObjnum = tplr.objnum;

	if (multi_i_am_master())
	{
		// latecoming player seems to successfully have synced
		if ( VerifyPlayerJoined != -1 && TheirPlayernum == VerifyPlayerJoined )
			VerifyPlayerJoined=-1;
		// we say that guy is disconnected so we do not want him/her in game
		if (tplr.connected == player_connection_status::disconnected)
			return;
	}
	else
	{
		// only by reading pdata a client can know if a player reconnected. So do that here.
		// NOTE: we might do this somewhere else - maybe with a sync packet like when adding a fresh player.
		if (tplr.connected == player_connection_status::disconnected && pd->connected == player_connection_status::playing)
		{
			tplr.connected = player_connection_status::playing;

			if (Newdemo_state == ND_STATE_RECORDING)
				newdemo_record_multi_reconnect(TheirPlayernum);

			digi_play_sample( SOUND_HUD_MESSAGE, F1_0);
			const auto &&rankstr = GetRankStringWithSpace(Netgame.players[TheirPlayernum].rank);
			HUD_init_message(HM_MULTI, "%s%s'%s' %s", rankstr.first, rankstr.second, static_cast<const char *>(vcplayerptr(TheirPlayernum)->callsign), TXT_REJOIN);

			multi_send_score();

			net_udp_noloss_clear_mdata_trace(TheirPlayernum);
		}
	}

	if (vcplayerptr(TheirPlayernum)->connected != player_connection_status::playing || TheirPlayernum == Player_num)
		return;

	if (!multi_quit_game && (TheirPlayernum >= N_players))
	{
		if (Network_status != network_state::waiting)
		{
			Int3(); // We missed an important packet!
			multi_consistency_error(0);
			return;
		}
		else
			return;
	}

	const auto TheirObj = vmobjptridx(TheirObjnum);
	Netgame.players[TheirPlayernum].LastPacketTime = timer_query();

	// do not read the packet unless the level is loaded.
	if (vcplayerptr(Player_num)->connected == player_connection_status::disconnected || vcplayerptr(Player_num)->connected == player_connection_status::waiting)
                return;
	//------------ Read the player's ship's object info ----------------------
	extract_quaternionpos(TheirObj, pd->qpp);
	if (TheirObj->movement_source == object::movement_type::physics)
		set_thrust_from_velocity(TheirObj);
}

}

#if defined(DXX_BUILD_DESCENT_II)
namespace dsx {
namespace {

static void net_udp_send_smash_lights (const playernum_t pnum)
 {
  // send the lights that have been blown out
	range_for (const auto &&segp, vmsegptridx)
	{
		unique_segment &useg = segp;
		if (const auto light_subtracted = useg.light_subtracted; light_subtracted != sidemask_t{})
			multi_send_light_specific(pnum, segp, light_subtracted);
	}
 }

static void net_udp_send_fly_thru_triggers (const playernum_t pnum)
 {
  // send the fly thru triggers that have been disabled
	auto &Triggers = LevelUniqueWallSubsystemState.Triggers;
	auto &vctrgptridx = Triggers.vcptridx;
	range_for (const auto &&t, vctrgptridx)
	{
		if (t->flags & trigger_behavior_flags::disabled)
			multi_send_trigger_specific(pnum, t);
	}
 }

static void net_udp_send_player_flags()
 {
	for (playernum_t i=0;i<N_players;i++)
	multi_send_flags(i);
 }

}
}
#endif

namespace {

// Send the ping list in regular intervals
void net_udp_ping_frame(fix64 time)
{
	static fix64 PingTime = 0;
	
	if ((PingTime + F1_0) < time)
	{
		std::array<uint8_t, upid_length<upid::ping>> buf{};
		int len = 0;
		
		buf[0] = underlying_value(upid::ping);							len++;
		memcpy(&buf[len], &time, 8);						len += 8;
		range_for (auto &i, partial_const_range(Netgame.players, 1u, MAX_PLAYERS))
		{
			PUT_INTEL_INT(&buf[len], i.ping);		len += 4;
		}
		
		for (unsigned i = 1; i < MAX_PLAYERS; ++i)
		{
			if (vcplayerptr(i)->connected == player_connection_status::disconnected)
				continue;
			dxx_sendto(UDP_Socket[0], buf, 0, Netgame.players[i].protocol.udp.addr);
		}
		PingTime = time;
	}
}

// Got a PING from host. Apply the pings to our players and respond to host.
void net_udp_process_ping(const upid_rspan<upid::ping> data, const _sockaddr &sender_addr)
{
	std::array<uint8_t, upid_length<upid::pong>> buf;
	int len = 0;

	if (Netgame.players[0].protocol.udp.addr != sender_addr)
		return;

										len++; // Skip UPID byte;
	memcpy(&buf[2], &data[len], 8);					len += 8;
	range_for (auto &i, partial_range(Netgame.players, 1u, MAX_PLAYERS))
	{
		i.ping = GET_INTEL_INT(&(data[len]));		len += 4;
	}
	
	buf[0] = underlying_value(upid::pong);
	buf[1] = Player_num;
	dxx_sendto(UDP_Socket[0], buf, 0, sender_addr);
}

// Got a PONG from a client. Check the time and add it to our players.
void net_udp_process_pong(const upid_rspan<upid::pong> data, const _sockaddr &sender_addr)
{
	const uint_fast32_t playernum = data[1];
	if (playernum >= MAX_PLAYERS || playernum < 1)
		return;
	if (sender_addr != Netgame.players[playernum].protocol.udp.addr)
		return;
	fix64 client_pong_time;
	memcpy(&client_pong_time, &data[2], 8);
	const fix64 delta64 = timer_update() - client_pong_time;
	const fix delta = static_cast<fix>(delta64);
	fix result;
	if (likely(delta64 == static_cast<fix64>(delta)))
	{
		result = f2i(fixmul64(delta, i2f(1000)));
		const fix lower_bound = 0;
		const fix upper_bound = 9999;
		if (unlikely(result < lower_bound))
			result = lower_bound;
		else if (unlikely(result > upper_bound))
			result = upper_bound;
	}
	else
		result = 0;
	Netgame.players[playernum].ping = result;
}

}

namespace dsx {
namespace {

void net_udp_do_refuse_stuff(const UDP_sequence_request_packet &their, const struct _sockaddr &peer_addr)
{
	int new_player_num;
	
	for (unsigned i = 0; i < MAX_PLAYERS; ++i)
	{
		if (!d_stricmp(vcplayerptr(i)->callsign, their.callsign) && peer_addr == Netgame.players[i].protocol.udp.addr)
		{
			net_udp_welcome_player(their, peer_addr);
			return;
		}
	}

	if (!WaitForRefuseAnswer)
	{
#if defined(DXX_BUILD_DESCENT_I)
		digi_play_sample (SOUND_CONTROL_CENTER_WARNING_SIREN,F1_0*2);
#elif defined(DXX_BUILD_DESCENT_II)
		digi_play_sample (SOUND_HUD_JOIN_REQUEST,F1_0*2);
#endif
	
		const auto &&rankstr = GetRankStringWithSpace(their.rank);
		if (Game_mode & GM_TEAM)
		{
			HUD_init_message(HM_MULTI, "%s%s'%s' wants to join", rankstr.first, rankstr.second, their.callsign.operator const char *());
			HUD_init_message(HM_MULTI, "Alt-1 assigns to team %s. Alt-2 to team %s", Netgame.team_name[team_number::blue].operator const char *(), Netgame.team_name[team_number::red].operator const char *());
		}
		else
		{
			HUD_init_message(HM_MULTI, "%s%s'%s' wants to join (accept: F6)", rankstr.first, rankstr.second, their.callsign.operator const char *());
		}
	
		strcpy(RefusePlayerName, their.callsign);
		RefuseTimeLimit=timer_query();
		RefuseThisPlayer=0;
		WaitForRefuseAnswer=1;
	}
	else
	{
		if (strcmp(RefusePlayerName, their.callsign))
			return;
	
		if (RefuseThisPlayer)
		{
			RefuseTimeLimit=0;
			RefuseThisPlayer=0;
			WaitForRefuseAnswer=0;
			if (Game_mode & GM_TEAM)
			{
				new_player_num=net_udp_get_new_player_num ();
	
				Assert (RefuseTeam==1 || RefuseTeam==2);        
			
				if (RefuseTeam==1)      
					Netgame.team_vector &=(~(1<<new_player_num));
				else
					Netgame.team_vector |=(1<<new_player_num);
				net_udp_welcome_player(their, peer_addr);
				net_udp_send_netgame_update();
			}
			else
			{
				net_udp_welcome_player(their, peer_addr);
			}
			return;
		}

		if ((timer_query()) > RefuseTimeLimit+REFUSE_INTERVAL)
		{
			RefuseTimeLimit=0;
			RefuseThisPlayer=0;
			WaitForRefuseAnswer=0;
			if (!strcmp(RefusePlayerName, their.callsign))
				multi::udp::dispatch->kick_player(peer_addr, kick_player_reason::dork);
			return;
		}
	}
}
}
}

namespace {

static int net_udp_get_new_player_num ()
{
	if ( N_players < Netgame.max_numplayers)
		return (N_players);

	else
	{
		// Slots are full but game is open, see if anyone is
		// disconnected and replace the oldest player with this new one

		int oldest_player = -1;
		fix64 oldest_time = timer_query();

		Assert(N_players == Netgame.max_numplayers);

		for (unsigned i = 0; i < N_players; ++i)
		{
			if (vcplayerptr(i)->connected == player_connection_status::disconnected && Netgame.players[i].LastPacketTime < oldest_time)
			{
				oldest_time = Netgame.players[i].LastPacketTime;
				oldest_player = i;
			}
		}
		return (oldest_player);
	}
}

}

namespace dsx {
namespace {

void net_udp_send_extras ()
{
	static fix64 last_send_time = 0;
	
	if (last_send_time + (F1_0/50) > timer_query())
		return;
	last_send_time = timer_query();

	Assert (Player_joining_extras>-1);

#if defined(DXX_BUILD_DESCENT_I)
	if (Network_sending_extras==3 && (Netgame.PlayTimeAllowed.count() || Netgame.KillGoal))
#elif defined(DXX_BUILD_DESCENT_II)
	if (Network_sending_extras==9)
		net_udp_send_fly_thru_triggers(Player_joining_extras);
	if (Network_sending_extras==8)
		net_udp_send_door_updates(Player_joining_extras);
	if (Network_sending_extras==7)
		multi_send_markers();
	if (Network_sending_extras==6 && (Game_mode & GM_MULTI_ROBOTS))
		multi_send_stolen_items();
	if (Network_sending_extras==5 && (Netgame.PlayTimeAllowed.count() || Netgame.KillGoal))
#endif
		multi_send_kill_goal_counts();
#if defined(DXX_BUILD_DESCENT_II)
	if (Network_sending_extras==4)
		net_udp_send_smash_lights(Player_joining_extras);
	if (Network_sending_extras==3)
		net_udp_send_player_flags();    
#endif
	if (Network_sending_extras==2)
		multi_send_player_inventory(multiplayer_data_priority::_1);
	if (Network_sending_extras==1 && Game_mode & GM_BOUNTY)
		multi_send_bounty();

	Network_sending_extras--;
	if (!Network_sending_extras)
		Player_joining_extras=-1;
}

struct show_game_info_menu : std::array<newmenu_item, 2>, std::array<char, 512>, passive_newmenu
{
	const netgame_info &netgame;
	show_game_info_menu(grs_canvas &canvas, const netgame_info &netgame) :
		std::array<newmenu_item, 2>{{
			newmenu_item::nm_item_menu{"JOIN GAME"},
			newmenu_item::nm_item_menu{"GAME INFO"},
		}},
		passive_newmenu(menu_title{"WELCOME"}, menu_subtitle{(setup_subtitle_text(*this, netgame).data())}, menu_filename{nullptr}, tiny_mode_flag::normal, tab_processing_flag::ignore, adjusted_citem::create(static_cast<std::array<newmenu_item, 2> &>(*this), 0), canvas),
		netgame(netgame)
	{
	}
	virtual window_event_result event_handler(const d_event &event) override;
	static const std::array<char, 512> &setup_subtitle_text(std::array<char, 512> &, const netgame_info &);
};

window_event_result show_game_info_menu::event_handler(const d_event &event)
{
	switch (event.type)
	{
		case event_type::newmenu_selected:
		{
			auto &citem = static_cast<const d_select_event &>(event).citem;
			switch (citem)
			{
				case 0:
				default:
					return window_event_result::close;
				case 1:
					show_netgame_info(netgame);
					return window_event_result::handled;
			}
		}
		default:
			return newmenu::event_handler(event);
	}
}

const std::array<char, 512> &show_game_info_menu::setup_subtitle_text(std::array<char, 512> &rinfo, const netgame_info &netgame)
{
#if defined(DXX_BUILD_DESCENT_I)
#define DXX_SECRET_LEVEL_FORMAT	"%s"
#define DXX_SECRET_LEVEL_PARAMETER	(netgame.levelnum >= 0 ? "" : "S"), \
	netgame.levelnum < 0 ? -netgame.levelnum :	/* else portion provided by invoker */
#elif defined(DXX_BUILD_DESCENT_II)
#define DXX_SECRET_LEVEL_FORMAT
#define DXX_SECRET_LEVEL_PARAMETER
#endif
	const auto gamemode = underlying_value(netgame.gamemode);
	const unsigned
#if defined(DXX_BUILD_DESCENT_I)
	players = netgame.numplayers;
#elif defined(DXX_BUILD_DESCENT_II)
	players = netgame.numconnected;
#endif
#define GAME_INFO_FORMAT_TEXT(F)	\
	F("\nConnected to\n\"%." DXX_STRINGIZE(NETGAME_NAME_LEN) "s\"\n", netgame.game_name.data())	\
	F("%." DXX_STRINGIZE(MISSION_NAME_LEN) "s", netgame.mission_title.data())	\
	F(" - Lvl " DXX_SECRET_LEVEL_FORMAT "%i", DXX_SECRET_LEVEL_PARAMETER netgame.levelnum)	\
	F("\n\nDifficulty: %s", MENU_DIFFICULTY_TEXT(netgame.difficulty))	\
	F("\nGame Mode: %s", gamemode < GMNames.size() ? GMNames[gamemode] : "INVALID")	\
	F("\nPlayers: %u/%i", players, netgame.max_numplayers)
#define EXPAND_FORMAT(A,B,...)	A
#define EXPAND_ARGUMENT(A,B,...)	, B, ## __VA_ARGS__
	std::snprintf(rinfo.data(), rinfo.size(), GAME_INFO_FORMAT_TEXT(EXPAND_FORMAT) GAME_INFO_FORMAT_TEXT(EXPAND_ARGUMENT));
#undef GAME_INFO_FORMAT_TEXT
	return rinfo;
}

direct_join::connect_type net_udp_show_game_info(const netgame_info &Netgame)
{
	switch (run_blocking_newmenu<show_game_info_menu>(*grd_curcanv, Netgame))
	{
		case 0:
		default:
			return direct_join::connect_type::request_join;
		case 1:
			return direct_join::connect_type::idle;
	}
}

}
}

/* Tracker stuff, begin! */
#if DXX_USE_TRACKER
namespace {

/* Tracker initialization */
static int udp_tracker_init()
{
	if (CGameArg.MplTrackerAddr.empty())
		return 0;

	TrackerAckStatus = TrackerAckState::TACK_NOCONNECTION;
	TrackerAckTime = timer_query();

	const char *tracker_addr = CGameArg.MplTrackerAddr.c_str();

	// Fill the address
	if (udp_dns_filladdr(TrackerSocket, tracker_addr, CGameArg.MplTrackerPort, false, true) < 0)
		return -1;

	// Yay
	return 0;
}

/* Compares sender to tracker. Returns 1 if address matches, Returns 2 is address and port matches. */
static int sender_is_tracker(const _sockaddr &sender, const _sockaddr &tracker)
{
	uint16_t sf, tf, sp, tp;

	sf = sender.sin.sin_family;
	tf = tracker.sin.sin_family;

#if DXX_USE_IPv6
	if (sf == AF_INET6)
	{
		if (tf == AF_INET)
		{
			if (IN6_IS_ADDR_V4MAPPED(&sender.sin6.sin6_addr))
			{
				if (memcmp(&sender.sin6.sin6_addr.s6_addr[12], &tracker.sin.sin_addr, sizeof(tracker.sin.sin_addr)))
					return 0;
				tp = tracker.sin.sin_port;
			}
			else
				return 0;
		}
		else if (tf == AF_INET6)
		{
			if (memcmp(&sender.sin6.sin6_addr, &tracker.sin6.sin6_addr, sizeof(sender.sin6.sin6_addr)))
				return 0;
			tp = tracker.sin6.sin6_port;
		}
		else
			return 0;
		sp = sender.sin6.sin6_port;
	}
	else
#endif
	if (sf == AF_INET)
	{
		if (sf != tf)
			return 0;
		if (memcmp(&sender.sin.sin_addr, &tracker.sin.sin_addr, sizeof(sender.sin.sin_addr)))
			return 0;
		sp = sender.sin.sin_port;
		tp = tracker.sin.sin_port;
	}
	else
		return 0;

	if (tp == sp)
		return 2;
	else
		return 1;
}

/* Unregister from the tracker */
static int udp_tracker_unregister()
{
	std::array<uint8_t, 1> pBuf;
	pBuf[0] = UPID_TRACKER_REMOVE;
	return dxx_sendto(UDP_Socket[0], pBuf, 0, TrackerSocket);
}

}

namespace dsx {
namespace {
/* Register or update (i.e. keep alive) a game on the tracker */
static int udp_tracker_register()
{
	net_udp_update_netgame();

	game_info_light light;
	std::size_t len = 1;
	const auto light_span = net_udp_prepare_light_game_info(LevelUniqueObjectState.ControlCenterState, Netgame, light.buf);
	std::array<uint8_t, 2 + sizeof("b=") + sizeof(UDP_REQ_ID) + sizeof("00000.00000.00000.00000,z=") + upid_length<upid::game_info_lite>> pBuf{};

	pBuf[0] = UPID_TRACKER_REGISTER;
	len += snprintf(reinterpret_cast<char *>(&pBuf[1]), sizeof(pBuf)-1, "b=" UDP_REQ_ID DXX_VERSION_STR ".%hu,z=", MULTI_PROTO_VERSION );
	memcpy(&pBuf[len], light_span.data(), light_span.size());		len += light_span.size();

	return dxx_sendto(UDP_Socket[0], std::span(pBuf).first(len), 0, TrackerSocket);
}

/* Ask the tracker to send us a list of games */
static int udp_tracker_reqgames()
{
	std::array<uint8_t, 2 + sizeof(UDP_REQ_ID) + sizeof("00000.00000.00000.00000")> pBuf{};
	unsigned len = 1;

	pBuf[0] = UPID_TRACKER_REQGAMES;
	len += snprintf(reinterpret_cast<char *>(&pBuf[1]), sizeof(pBuf)-1, UDP_REQ_ID DXX_VERSION_STR ".%hu", MULTI_PROTO_VERSION );

	return dxx_sendto(UDP_Socket[0], std::span(pBuf).first(len), 0, TrackerSocket);
}
}
}

namespace {

/* The tracker has sent us a game.  Let's list it. */
static int udp_tracker_process_game(const std::span<const uint8_t> buf, const _sockaddr &sender_addr)
{
	// Only accept data from the tracker we specified and only when we look at the netlist (i.e. network_state::browsing)
	if (!sender_is_tracker(sender_addr, TrackerSocket) || Network_status != network_state::browsing)
		return -1;

	const char *p0 = NULL, *p1 = NULL, *p3 = NULL;
	char sIP[47]{};
	std::array<char, 6> sPort{};
	uint16_t iPort = 0;

	const auto data = buf.data();
	const auto data_len = buf.size();
	// Get the IP
	if ((p0 = strstr(reinterpret_cast<const char *>(data), "a=")) == NULL)
		return -1;
	p0 +=2;
	if ((p1 = strstr(p0, "/")) == NULL)
		return -1;
	if (p1-p0 < 1 || p1-p0 > sizeof(sIP))
		return -1;
	memcpy(sIP, p0, p1-p0);

	// Get the port
	p1++;
	const auto p2 = strstr(p1, "c=");
	if (p2 == nullptr)
		return -1;
	if (p2-p1-1 < 1 || p2-p1-1 > sizeof(sPort))
		return -1;
	memcpy(&sPort, p1, p2-p1-1);
	if (!convert_text_portstring(sPort, iPort, true, true))
		return -1;

	// Get the DNS stuff
	struct _sockaddr sAddr;
	if(udp_dns_filladdr(sAddr, sIP, iPort, true, true) < 0)
		return -1;
	if (data_len < p2 - reinterpret_cast<const char *>(data) + 2)
		return -1;
	if ((p3 = strstr(reinterpret_cast<const char *>(data), "z=")) == NULL)
		return -1;

	// Now process the actual lite_game packet contained.
	int iPos = (p3-p0+5);
	const auto TrackerGameID = tracker_game_id{GET_INTEL_SHORT(&p2[2])};
	net_udp_process_game_info_light(buf.subspan(iPos, data_len - iPos), sAddr, TrackerGameID);

	return 0;
}

/* Process ACK's from tracker. We will get up to 5, each internal and external */
static void udp_tracker_process_ack(const uint8_t *const data, int data_len, const _sockaddr &sender_addr)
{
	if(!Netgame.Tracker)
		return;
	if (data_len != 2)
		return;
	int addr_check = sender_is_tracker(sender_addr, TrackerSocket);

	switch (data[1])
	{
		case 0: // ack coming from the same socket we are already talking with the tracker
			if (TrackerAckStatus == TrackerAckState::TACK_NOCONNECTION && addr_check == 2)
			{
				TrackerAckStatus = TrackerAckState::TACK_INTERNAL;
				con_puts(CON_VERBOSE, "[Tracker] Got internal ACK. Your game is hosted!");
			}
			break;
		case 1: // ack from another socket (same IP, different port) to see if we're reachable from the outside
			if (TrackerAckStatus <= TrackerAckState::TACK_INTERNAL && addr_check)
			{
				TrackerAckStatus = TrackerAckState::TACK_EXTERNAL;
				con_puts(CON_VERBOSE, "[Tracker] Got external ACK. Your game is hosted and game port is reachable!");
			}
			break;
	}
}

/* 10 seconds passed since we registered our game. If we have not received all ACK's, yet, tell user about that! */
static void udp_tracker_verify_ack_timeout()
{
	if (!Netgame.Tracker || !multi_i_am_master() || TrackerAckTime + F1_0*10 > timer_query() || TrackerAckStatus == TrackerAckState::TACK_SEQCOMPL)
		return;
	if (TrackerAckStatus == TrackerAckState::TACK_NOCONNECTION)
	{
		TrackerAckStatus = TrackerAckState::TACK_SEQCOMPL; // set this now or we'll run into an endless loop if nm_messagebox triggers.
		if (Network_status == network_state::playing)
			HUD_init_message_literal(HM_MULTI, "No ACK from tracker. Please check game log.");
		else
			nm_messagebox_str(menu_title{TXT_WARNING}, nm_messagebox_tie(TXT_OK), menu_subtitle{"No ACK from tracker.\nPlease check game log."});
		con_puts(CON_URGENT, "[Tracker] No response from game tracker. Tracker address may be invalid or Tracker may be offline or otherwise unreachable.");
	}
	else if (TrackerAckStatus == TrackerAckState::TACK_INTERNAL)
	{
		con_puts(CON_NORMAL, "[Tracker] No external signal from game tracker.  Your game port does not seem to be reachable.");
		con_puts(CON_NORMAL, Netgame.TrackerNATWarned == TrackerNATHolePunchWarn::UserEnabledHP ? std::span<const char>("Clients will attempt hole-punching to join your game.") : std::span<const char>("Clients will only be able to join your game if specifically configured in your router."));
	}
	TrackerAckStatus = TrackerAckState::TACK_SEQCOMPL;
}

/* We don't seem to be able to connect to a game. Ask Tracker to send hole punch request to host. */
static void udp_tracker_request_holepunch(const tracker_game_id id)
{
	std::array<uint8_t, 3> pBuf;

	pBuf[0] = underlying_value(upid::tracker_holepunch);
	const uint16_t TrackerGameID = underlying_value(id);
	PUT_INTEL_SHORT(&pBuf[1], TrackerGameID);

	con_printf(CON_VERBOSE, "[Tracker] Sending hole-punch request for game [%i] to tracker.", TrackerGameID);
	dxx_sendto(UDP_Socket[0], pBuf, 0, TrackerSocket);
}

/* Tracker sent us an address from a client requesting hole punching.
 * We'll simply reply with another hole punch packet and wait for them to request our game info properly. */
static void udp_tracker_process_holepunch(uint8_t *const data, const unsigned data_len, const _sockaddr &sender_addr)
{
	if (data_len == 1 && !multi_i_am_master())
	{
		con_puts(CON_VERBOSE, "[Tracker] Received hole-punch pong from a host.");
		return;
	}
	if (!Netgame.Tracker || !sender_is_tracker(sender_addr, TrackerSocket) || !multi_i_am_master())
		return;
	if (Netgame.TrackerNATWarned != TrackerNATHolePunchWarn::UserEnabledHP)
	{
		con_puts(CON_NORMAL, "Ignoring tracker hole-punch request because user disabled hole punch.");
		return;
	}
	if (!data_len || data[data_len - 1])
		return;

	auto &delimiter = "/";

	const auto p0 = strtok(reinterpret_cast<char *>(data), delimiter);
	if (!p0)
		return;
	const auto sIP = p0 + 1;
	const auto pPort = strtok(NULL, delimiter);
	if (!pPort)
		return;
	char *porterror;
	const auto myport = strtoul(pPort, &porterror, 10);
	if (*porterror)
		return;
	const uint16_t iPort = myport;
	if (iPort != myport)
		return;

	// Get the DNS stuff
	struct _sockaddr sAddr;
	if(udp_dns_filladdr(sAddr, sIP, iPort, true, true) < 0)
		return;

	const std::array<uint8_t, 1> pBuf{{
		underlying_value(upid::tracker_holepunch)
	}};
	dxx_sendto(UDP_Socket[0], pBuf, 0, sAddr);
}

}
#endif /* USE_TRACKER */
