/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 * 
 * Routines for managing UDP-protocol network play.
 * 
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "pstypes.h"
#include "window.h"
#include "strutil.h"
#include "args.h"
#include "timer.h"
#include "newmenu.h"
#include "key.h"
#include "gauges.h"
#include "object.h"
#include "dxxerror.h"
#include "laser.h"
#include "gamesave.h"
#include "gamemine.h"
#include "player.h"
#include "gameseq.h"
#include "fireball.h"
#include "net_udp.h"
#include "game.h"
#include "multi.h"
#include "endlevel.h"
#include "palette.h"
#include "cntrlcen.h"
#include "powerup.h"
#include "menu.h"
#include "gameseg.h"
#include "sounds.h"
#include "text.h"
#include "kmatrix.h"
#include "newdemo.h"
#include "multibot.h"
#include "wall.h"
#include "bm.h"
#include "effects.h"
#include "physics.h"
#include "hudmsg.h"
#include "switch.h"
#include "automap.h"
#include "byteutil.h"
#include "playsave.h"
#include "gamefont.h"
#include "rbaudio.h"
#include "config.h"
#include "vers_id.h"
#include "u_mem.h"

#include "dxxsconf.h"
#include "compiler-array.h"
#include "compiler-exchange.h"
#include "compiler-range_for.h"
#include "compiler-lengthof.h"
#include "compiler-static_assert.h"
#include "highest_valid.h"
#include "partial_range.h"

// player position packet structure
struct UDP_frame_info : prohibit_void_ptr<UDP_frame_info>
{
	ubyte				type;
	ubyte				Player_num;
	ubyte				connected;
	quaternionpos			qpp;
};

// Prototypes
static void net_udp_init();
static void net_udp_close();
static void net_udp_listen();
static int net_udp_show_game_info();
static int net_udp_do_join_game();
static void net_udp_flush();
static void net_udp_update_netgame(void);
static void net_udp_send_objects(void);
static void net_udp_send_rejoin_sync(int player_num);
static void net_udp_do_refuse_stuff (UDP_sequence_packet *their);
static void net_udp_read_sync_packet(const uint8_t *data, uint_fast32_t data_len, const _sockaddr &sender_addr);
static void net_udp_ping_frame(fix64 time);
static void net_udp_process_ping(const uint8_t *data, const _sockaddr &sender_addr);
static void net_udp_process_pong(const uint8_t *data, const _sockaddr &sender_addr);
static void net_udp_read_endlevel_packet(const uint8_t *data, const _sockaddr &sender_addr);
static void net_udp_send_mdata(int needack, fix64 time);
static void net_udp_process_mdata (uint8_t *data, uint_fast32_t data_len, const _sockaddr &sender_addr, int needack);
static void net_udp_send_pdata();
static void net_udp_process_pdata (const uint8_t *data, uint_fast32_t data_len, const _sockaddr &sender_addr);
static void net_udp_read_pdata_packet(UDP_frame_info *pd);
static void net_udp_timeout_check(fix64 time);
static int net_udp_get_new_player_num ();
static void net_udp_noloss_got_ack(const uint8_t *data, uint_fast32_t data_len);
static void net_udp_noloss_init_mdata_queue(void);
static void net_udp_noloss_clear_mdata_trace(ubyte player_num);
static void net_udp_noloss_process_queue(fix64 time);
static void net_udp_send_extras ();
static void net_udp_broadcast_game_info(ubyte info_upid);
static void net_udp_process_game_info(const uint8_t *data, uint_fast32_t data_len, const _sockaddr &game_addr, int lite_info);
static int net_udp_more_options_handler( newmenu *menu,const d_event &event, const unused_newmenu_userdata_t *);
static int net_udp_start_game(void);

// Variables
static int UDP_num_sendto, UDP_len_sendto, UDP_num_recvfrom, UDP_len_recvfrom;
static UDP_mdata_info		UDP_MData;
static UDP_sequence_packet UDP_Seq;
static unsigned UDP_mdata_queue_highest;
static array<UDP_mdata_store, UDP_MDATA_STOR_QUEUE_SIZE> UDP_mdata_queue;
static array<UDP_mdata_check, MAX_PLAYERS> UDP_mdata_trace;
static UDP_sequence_packet UDP_sync_player; // For rejoin object syncing
static array<UDP_netgame_info_lite, UDP_MAX_NETGAMES> Active_udp_games;
static unsigned num_active_udp_games;
static int num_active_udp_changed;
static uint16_t UDP_MyPort;
static sockaddr_in GBcast; // global Broadcast address clients and hosts will use for lite_info exchange over LAN
#ifdef IPv6
static sockaddr_in6 GMcast_v6; // same for IPv6-only
#define dispatch_sockaddr_from	from.sin6
#else
#define dispatch_sockaddr_from	from.sin
#endif
#ifdef USE_TRACKER
static _sockaddr TrackerSocket;
static int iTrackerVerified;
static const int require_tracker_socket = 1;
#else
static const int require_tracker_socket = 0;
#endif

static fix64 StartAbortMenuTime;

#ifndef _WIN32
const int INVALID_SOCKET = -1;
#endif

namespace {

struct RAIIsocket
{
#ifndef _WIN32
	typedef int SOCKET;
	int closesocket(SOCKET s)
	{
		return close(s);
	}
#endif
	SOCKET s;
	constexpr RAIIsocket() : s(INVALID_SOCKET)
	{
	}
	RAIIsocket(int domain, int type, int protocol) : s(socket(domain, type, protocol))
	{
	}
	RAIIsocket(const RAIIsocket &) = delete;
	RAIIsocket &operator=(const RAIIsocket &) = delete;
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
		SOCKET c = exchange(s, INVALID_SOCKET);
		if (c != INVALID_SOCKET)
			closesocket(c);
	}
	explicit operator bool() const { return s != INVALID_SOCKET; }
	explicit operator bool() { return static_cast<bool>(*const_cast<const RAIIsocket *>(this)); }
	operator SOCKET() { return s; }
	template <typename T> bool operator<(T) const = delete;
	template <typename T> bool operator<=(T) const = delete;
	template <typename T> bool operator>(T) const = delete;
	template <typename T> bool operator>=(T) const = delete;
	template <typename T> bool operator==(T) const = delete;
	template <typename T> bool operator!=(T) const = delete;
};

class RAIIaddrinfo
{
	struct deleter
	{
		void operator()(addrinfo *p) const
		{
			freeaddrinfo(p);
		}
	};
	std::unique_ptr<addrinfo, deleter> result;
public:
	int getaddrinfo(const char *node, const char *service, const addrinfo *hints)
	{
		addrinfo *p = nullptr;
		int r = ::getaddrinfo(node, service, hints, &p);
		result.reset(p);
		return r;
	}
	addrinfo *get() { return result.get(); }
	addrinfo *operator->() { return result.operator->(); }
};

}

static array<RAIIsocket, 2 + require_tracker_socket> UDP_Socket;

static bool operator==(const _sockaddr &l, const _sockaddr &r)
{
	return !memcmp(&l, &r, sizeof(l));
}

static bool operator!=(const _sockaddr &l, const _sockaddr &r)
{
	return !(l == r);
}

template <std::size_t N>
static void copy_from_ntstring(uint8_t *const buf, uint_fast32_t &len, const ntstring<N> &in)
{
	len += in.copy_out(0, reinterpret_cast<char *>(&buf[len]), N);
}

template <std::size_t N>
static void copy_to_ntstring(const uint8_t *const buf, uint_fast32_t &len, ntstring<N> &out)
{
	uint_fast32_t c = out.copy_if(reinterpret_cast<const char *>(&buf[len]), N);
	if (c < N)
		++ c;
	len += c;
}

static void net_udp_prepare_request_game_info(array<uint8_t, UPID_GAME_INFO_REQ_SIZE> &buf, int lite)
{
	buf[0] = lite ? UPID_GAME_INFO_LITE_REQ : UPID_GAME_INFO_REQ;
	memcpy(&buf[1], UDP_REQ_ID, 4);
	PUT_INTEL_SHORT(&buf[5], DXX_VERSION_MAJORi);
	PUT_INTEL_SHORT(&buf[7], DXX_VERSION_MINORi);
	PUT_INTEL_SHORT(&buf[9], DXX_VERSION_MICROi);
	PUT_INTEL_SHORT(&buf[11], MULTI_PROTO_VERSION);
}

static void reset_UDP_MyPort()
{
	UDP_MyPort = GameArg.MplUdpMyPort >= 1024 ? GameArg.MplUdpMyPort : UDP_PORT_DEFAULT;
}

static bool convert_text_portstring(const char *portstring, uint16_t &outport, bool allow_privileged)
{
	char *porterror;
	unsigned long myport = strtoul(portstring, &porterror, 10);
	if (*porterror || static_cast<uint16_t>(myport) != myport || (!allow_privileged && myport < 1024))
	{
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "Illegal port \"%s\"", portstring);
		return false;
	}
	else
		outport = myport;
	return true;
}

namespace {

#ifdef IPv6
/* Returns true if kernel allows specifying sizeof(sockaddr_in6) for
 * size of a sockaddr_in.  Saves a compare+jump in application code to
 * pass sizeof(sockaddr_in6) and let kernel sort it out.
 */
static constexpr bool kernel_accepts_extra_sockaddr_bytes()
{
#if defined(__linux__)
	/* Known to work */
	return true;
#else
	/* Default case: not known */
	return false;
#endif
}
#endif

	/* Forward to static function to eliminate this pointer */
template <typename F>
class passthrough_static_apply : F
{
public:
#define apply_passthrough()	this->F::apply(std::forward<Args>(args)...)
	template <typename... Args>
	__attribute_always_inline()
		auto operator()(Args &&... args) const -> decltype(apply_passthrough())
		{
			return apply_passthrough();
		}
#undef apply_passthrough
};

template <typename F>
class sockaddr_dispatch_t : F
{
public:
#define apply_sockaddr()	this->F::operator()(reinterpret_cast<sockaddr &>(from), fromlen, std::forward<Args>(args)...)
	template <typename... Args>
		auto operator()(sockaddr_in &from, socklen_t &fromlen, Args &&... args) const -> decltype(apply_sockaddr())
		{
			fromlen = sizeof(from);
			return apply_sockaddr();
		}
#ifdef IPv6
	template <typename... Args>
		auto operator()(sockaddr_in6 &from, socklen_t &fromlen, Args &&... args) const -> decltype(apply_sockaddr())
		{
			fromlen = sizeof(from);
			return apply_sockaddr();
		}
#endif
	template <typename... Args>
		auto operator()(_sockaddr &from, socklen_t &fromlen, Args &&... args) const -> decltype(apply_sockaddr())
		{
			return this->operator()(dispatch_sockaddr_from, fromlen, std::forward<Args>(args)...);
		}
#undef apply_sockaddr
};

template <typename F>
class csockaddr_dispatch_t : F
{
public:
#define apply_sockaddr()	this->F::operator()(to, tolen, std::forward<Args>(args)...)
	template <typename... Args>
		auto operator()(const sockaddr &to, socklen_t tolen, Args &&... args) const -> decltype(apply_sockaddr())
		{
			return apply_sockaddr();
		}
#undef apply_sockaddr
#define apply_sockaddr()	this->F::operator()(reinterpret_cast<const sockaddr &>(to), sizeof(to), std::forward<Args>(args)...)
	template <typename... Args>
		auto operator()(const sockaddr_in &to, Args &&... args) const -> decltype(apply_sockaddr())
		{
			return apply_sockaddr();
		}
#ifdef IPv6
	template <typename... Args>
		auto operator()(const sockaddr_in6 &to, Args &&... args) const -> decltype(apply_sockaddr())
		{
			return apply_sockaddr();
		}
#endif
	template <typename... Args>
		auto operator()(const _sockaddr &to, Args &&... args) const -> decltype(apply_sockaddr())
#undef apply_sockaddr
		{
#ifdef IPv6
			if (kernel_accepts_extra_sockaddr_bytes() || to.sin6.sin6_family == AF_INET6)
				return operator()(to.sin6, std::forward<Args>(args)...);
#endif
			return operator()(to.sin, std::forward<Args>(args)...);
		}
};

template <typename F>
class socket_array_dispatch_t : F
{
public:
#define apply_array(B,L)	this->F::operator()(to, tolen, sock, B, L, std::forward<Args>(args)...)
	template <typename T, typename... Args>
		auto operator()(const sockaddr &to, socklen_t tolen, int sock, T *buf, uint_fast32_t buflen, Args &&... args) const -> decltype(apply_array(buf, buflen))
		{
			return apply_array(buf, buflen);
		}
	template <typename A, typename... Args>
		auto operator()(const sockaddr &to, socklen_t tolen, int sock, A &buf, Args &&... args) const -> decltype(apply_array(buf.data(), buf.size()))
		{
			return apply_array(buf.data(), buf.size());
		}
#undef apply_array
};

/* General UDP functions - START */
class dxx_sendto_t
{
public:
	__attribute_always_inline()
	ssize_t operator()(const sockaddr &to, socklen_t tolen, int sockfd, const void *msg, size_t len, int flags) const
	{
		/* Fix argument order */
		return apply(sockfd, msg, len, flags, to, tolen);
	}
	static ssize_t apply(int sockfd, const void *msg, size_t len, int flags, const sockaddr &to, socklen_t tolen);
};

class dxx_recvfrom_t
{
public:
	__attribute_always_inline()
	ssize_t operator()(sockaddr &from, socklen_t &fromlen, int sockfd, void *msg, size_t len, int flags) const
	{
		/* Fix argument order */
		return apply(sockfd, msg, len, flags, from, fromlen);
	}
	static ssize_t apply(int sockfd, void *msg, size_t len, int flags, sockaddr &from, socklen_t &fromlen);
};

ssize_t dxx_sendto_t::apply(int sockfd, const void *msg, size_t len, int flags, const sockaddr &to, socklen_t tolen)
{
	ssize_t rv = sendto(sockfd, reinterpret_cast<const char *>(msg), len, flags, &to, tolen);

	UDP_num_sendto++;
	if (rv > 0)
		UDP_len_sendto += rv;

	return rv;
}

ssize_t dxx_recvfrom_t::apply(int sockfd, void *buf, size_t len, int flags, sockaddr &from, socklen_t &fromlen)
{
	ssize_t rv = recvfrom(sockfd, reinterpret_cast<char *>(buf), len, flags, &from, &fromlen);

	UDP_num_recvfrom++;
	UDP_len_recvfrom += rv;

	return rv;
}

const csockaddr_dispatch_t<socket_array_dispatch_t<dxx_sendto_t>> dxx_sendto{};
const sockaddr_dispatch_t<dxx_recvfrom_t> dxx_recvfrom{};

}

static void udp_traffic_stat()
{
	static fix64 last_traf_time = 0;

	if (timer_query() >= last_traf_time + F1_0)
	{
		last_traf_time = timer_query();
		con_printf(CON_DEBUG, "P#%u TRAFFIC - OUT: %fKB/s %iPPS IN: %fKB/s %iPPS",Player_num, (float)UDP_len_sendto/1024, UDP_num_sendto, (float)UDP_len_recvfrom/1024, UDP_num_recvfrom);
		UDP_num_sendto = UDP_len_sendto = UDP_num_recvfrom = UDP_len_recvfrom = 0;
	}
}

namespace {

class udp_dns_filladdr_t
{
public:
	static int apply(sockaddr &addr, socklen_t addrlen, int ai_family, const char *host, uint16_t port);
};

// Resolve address
int udp_dns_filladdr_t::apply(sockaddr &addr, socklen_t addrlen, int ai_family, const char *host, uint16_t port)
{
	// Variables
	addrinfo hints{};
	char sPort[6];

	// Build the port
	snprintf(sPort, 6, "%hu", port);
	
	// Uncomment the following if we want ONLY what we compile for
	hints.ai_family = ai_family;
	// We are always UDP
	hints.ai_socktype = SOCK_DGRAM;
	
	// Resolve the domain name
	RAIIaddrinfo result;
	if (result.getaddrinfo(host, sPort, &hints) != 0)
	{
		con_printf( CON_URGENT, "udp_dns_filladdr (getaddrinfo) failed for host %s", host );
		nm_messagebox( TXT_ERROR, 1, TXT_OK, "Could not resolve address\n%s", host );
		addr.sa_family = AF_UNSPEC;
		return -1;
	}
	
	if (result->ai_addrlen > addrlen)
	{
		con_printf(CON_URGENT, "Address too big for host %s", host);
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "Address too big for host\n%s", host);
		addr.sa_family = AF_UNSPEC;
		return -1;
	}
	// Now copy it over
	memcpy(&addr, result->ai_addr, addrlen = result->ai_addrlen);
	
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
	return 0;
}

template <typename F>
class sockaddr_resolve_family_dispatch_t : sockaddr_dispatch_t<F>
{
public:
#define apply_sockaddr(fromlen,family)	this->sockaddr_dispatch_t<F>::operator()(from, fromlen, family, std::forward<Args>(args)...)
	template <typename... Args>
		auto operator()(sockaddr_in &from, Args &&... args) const -> decltype(apply_sockaddr(std::declval<socklen_t &>(), AF_INET))
		{
			socklen_t fromlen;
			return apply_sockaddr(fromlen, AF_INET);
		}
#ifdef IPv6
	template <typename... Args>
		auto operator()(sockaddr_in6 &from, Args &&... args) const -> decltype(apply_sockaddr(std::declval<socklen_t &>(), AF_UNSPEC))
		{
			socklen_t fromlen;
			return apply_sockaddr(fromlen, AF_UNSPEC);
		}
#endif
	template <typename... Args>
		auto operator()(_sockaddr &from, Args &&... args) const -> decltype(apply_sockaddr(std::declval<socklen_t &>(), AF_UNSPEC))
		{
			return this->operator()(dispatch_sockaddr_from, std::forward<Args>(args)...);
		}
#undef apply_sockaddr
};

const sockaddr_resolve_family_dispatch_t<passthrough_static_apply<udp_dns_filladdr_t>> udp_dns_filladdr{};

}

static void udp_init_broadcast_addresses()
{
	udp_dns_filladdr(GBcast, UDP_BCAST_ADDR, UDP_PORT_DEFAULT);
#ifdef IPv6
	udp_dns_filladdr(GMcast_v6, UDP_MCASTv6_ADDR, UDP_PORT_DEFAULT);
#endif
}

// Open socket
static int udp_open_socket(RAIIsocket &sock, int port)
{
	int bcast = 1;

	// close stale socket
	sock.reset();
	struct _sockaddr sAddr{};   // my address information

	sock = RAIIsocket(sAddr.address_family(), SOCK_DGRAM, 0);
	if (!sock)
	{
		con_printf(CON_URGENT,"udp_open_socket: socket creation failed (port %i)", port);
		nm_messagebox(TXT_ERROR,1,TXT_OK,"Port: %i\nCould not create socket.", port);
		return -1;
	}
	sAddr.sa.sa_family = sAddr.address_family();
#ifdef IPv6
	sAddr.sin6.sin6_port = htons (port); // short, network byte order
	sAddr.sin6.sin6_addr = in6addr_any; // automatically fill with my IP
#else
	sAddr.sin.sin_port = htons (port); // short, network byte order
	sAddr.sin.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
#endif
	
	if (bind(sock, &sAddr.sa, sizeof(sAddr)) < 0)
	{      
		con_printf(CON_URGENT,"udp_open_socket: bind name to socket failed (port %i)", port);
		nm_messagebox(TXT_ERROR,1,TXT_OK,"Port: %i\nCould not bind name to socket.", port);
		sock.reset();
		return -1;
	}
#ifdef _WIN32
	(void)setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char *) &bcast, sizeof(bcast) );
#else
	setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &bcast, sizeof(bcast));
#endif
	return 0;
}

#ifndef MSG_DONTWAIT
static int udp_general_packet_ready(RAIIsocket &sock)
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
static int udp_receive_packet(RAIIsocket &sock, ubyte *text, int len, struct _sockaddr *sender_addr)
{
	if (!sock)
		return -1;
#ifndef MSG_DONTWAIT
	if (!udp_general_packet_ready(sock))
		return 0;
#endif
	ssize_t msglen;
		socklen_t clen;
		int flags = 0;
#ifdef MSG_DONTWAIT
		flags |= MSG_DONTWAIT;
#endif
		msglen = dxx_recvfrom(*sender_addr, clen, sock, text, len, flags);

		if (msglen < 0)
			return 0;

		if ((msglen >= 0) && (msglen < len))
			text[msglen] = 0;
	return msglen;
}
/* General UDP functions - END */

/* Tracker stuff, begin! */
#ifdef USE_TRACKER

/* Tracker defines:  System stuff */
#define TRACKER_SYS_VERSION		0x00	/* Tracker protocol version */

/* Tracker defines:  Packet stuff */
#define TRACKER_PKT_REGISTER		0	/* Register a game */
#define TRACKER_PKT_UNREGISTER		1	/* Unregister our game */
#define TRACKER_PKT_GAMELIST		2	/* Request the game list */

/* Tracker initialization */
static int udp_tracker_init()
{
	if (GameArg.MplTrackerAddr.empty())
		return 0;
	const char *tracker_addr = GameArg.MplTrackerAddr.c_str();
	int tracker_port = d_rand() % 0xffff;

	while (tracker_port <= 1024)
		tracker_port = d_rand() % 0xffff;

	// Open the socket
	udp_open_socket(UDP_Socket[2], tracker_port );
	
	// Fill the address
	if(udp_dns_filladdr(TrackerSocket, tracker_addr, GameArg.MplTrackerPort) < 0)
		return -1;
	
	// Yay
	return 0;
}

/* Unregister from the tracker */
static int udp_tracker_unregister()
{
	// Variables
	array<uint8_t, 5> pBuf;
	// Put the opcode
	pBuf[0] = TRACKER_PKT_UNREGISTER;
	// Put the GameID
	PUT_INTEL_INT(&pBuf[1], Netgame.protocol.udp.GameID );
	// Send it off
	return dxx_sendto(TrackerSocket, UDP_Socket[2], pBuf, 0);
}

/* Tell the tracker we're starting a game */
static int udp_tracker_register()
{
	// Variables
	array<uint8_t, 15> pBuf;
	// Reset the last tracker message
	iTrackerVerified = 0;
	// Put the opcode
	pBuf[0] = TRACKER_PKT_REGISTER;
	// Put the protocol version
	pBuf[1] = TRACKER_SYS_VERSION;
	// Write the game version (d1 = 1, d2 = 2, x = oshiii)
#if defined(DXX_BUILD_DESCENT_I)
	pBuf[2] = 0x01;
#elif defined(DXX_BUILD_DESCENT_II)
	pBuf[2] = 0x02;
#endif
	// Write the port we're running on
	PUT_INTEL_SHORT(&pBuf[3], UDP_MyPort );
	// Put the GameID
	PUT_INTEL_INT(&pBuf[5], Netgame.protocol.udp.GameID );
	// Now, put the game version
	PUT_INTEL_SHORT(&pBuf[9], DXX_VERSION_MAJORi );
	PUT_INTEL_SHORT(&pBuf[11], DXX_VERSION_MINORi );
	PUT_INTEL_SHORT(&pBuf[13], DXX_VERSION_MICROi );
	// Send it off
	return dxx_sendto(TrackerSocket, UDP_Socket[2], pBuf, 0);
}

/* Ask the tracker to send us a list of games */
static int udp_tracker_reqgames()
{
	// Variables
	array<uint8_t, 3> pBuf;
	// Put the opcode
	pBuf[0] = TRACKER_PKT_GAMELIST;
#if defined(DXX_BUILD_DESCENT_I)
// 	// Put the game version (d1)
	pBuf[1] = 0x01;
#elif defined(DXX_BUILD_DESCENT_II)
	// Put the game version (d2)
	pBuf[1] = 0x02;
#endif
	// If we're IPv6 ready, send that too
#ifdef IPv6
	pBuf[2] = 1;
#else
	pBuf[2] = 0;
#endif
	// Send it off
	return dxx_sendto(TrackerSocket, UDP_Socket[2], pBuf, 0);
}

/* The tracker has sent us a game.  Let's list it. */
static int udp_tracker_process_game( ubyte *data, int data_len )
{
	// All our variables
	int iPos = 1;
	int iPort = 0;
	int bIPv6 = 0;
	char *sIP = NULL;
	// Get the IPv6 flag from the tracker
	bIPv6 = data[iPos++];
	(void)bIPv6; // currently unused
	
	// Get the IP
	sIP = (char *)&data[iPos];
	iPos += strlen( sIP ) + 1;
	
	// Get the port
	iPort = GET_INTEL_SHORT( &data[iPos] );
	iPos += 2;
	
	// Get the DNS stuff
	struct _sockaddr sAddr;
	if(udp_dns_filladdr(sAddr, sIP, iPort) < 0)
		return -1;
	
	// Now move on to BIGGER AND BETTER THINGS!
	net_udp_process_game_info( &data[iPos - 1], data_len - iPos, sAddr, 1 );
	return 0;
}

#else
static int udp_tracker_init()
{
	return -1;
}

static void udp_tracker_reqgames()
{
}
#endif /* USE_TRACKER */

namespace {

class net_udp_request_game_info_t
{
public:
	static void apply(const sockaddr &to, socklen_t tolen, int lite);
};

class net_udp_send_game_info_t
{
public:
	static void apply(const sockaddr &target_addr, socklen_t targetlen, const _sockaddr *sender_addr, ubyte info_upid);
};

void net_udp_request_game_info_t::apply(const sockaddr &game_addr, socklen_t addrlen, int lite)
{
	array<uint8_t, UPID_GAME_INFO_REQ_SIZE> buf;
	net_udp_prepare_request_game_info(buf, lite);
	dxx_sendto(game_addr, addrlen, UDP_Socket[0], buf, 0);
}

const csockaddr_dispatch_t<passthrough_static_apply<net_udp_request_game_info_t>> net_udp_request_game_info{};
const csockaddr_dispatch_t<passthrough_static_apply<net_udp_send_game_info_t>> net_udp_send_game_info{};

}

struct direct_join
{
	struct _sockaddr host_addr;
	int connecting;
	fix64 start_time, last_time;
	char addrbuf[128];
	char hostportbuf[6], myportbuf[6];
};

// Connect to a game host and get full info. Eventually we join!
static int net_udp_game_connect(direct_join *dj)
{
	// Get full game info so we can show it.

	// Timeout after 10 seconds
	if (timer_query() >= dj->start_time + (F1_0*10))
	{
		nm_messagebox(TXT_ERROR,1,TXT_OK,"No response by host.\n\nPossible reasons:\n* No game on this IP (anymore)\n* Port of Host not open\n  or different\n* Host uses a game version\n  I do not understand");
		dj->connecting = 0;
		return 0;
	}
	
	if (Netgame.protocol.udp.valid == -1)
	{
		nm_messagebox(TXT_ERROR,1,TXT_OK,"Version mismatch! Cannot join Game.\n\nHost game version: %i.%i.%i\nHost game protocol: %i\n(%s)\n\nYour game version: %s\nYour game protocol: %i\n(%s)",Netgame.protocol.udp.program_iver[0],Netgame.protocol.udp.program_iver[1],Netgame.protocol.udp.program_iver[2],Netgame.protocol.udp.program_iver[3],(Netgame.protocol.udp.program_iver[3]==0?"RELEASE VERSION":"DEVELOPMENT BUILD, BETA, etc."),VERSION, MULTI_PROTO_VERSION, (MULTI_PROTO_VERSION==0?"RELEASE VERSION":"DEVELOPMENT BUILD, BETA, etc."));
		dj->connecting = 0;
		return 0;
	}
	
	if (timer_query() >= dj->last_time + F1_0)
	{
		net_udp_request_game_info(dj->host_addr, 0);
		dj->last_time = timer_query();
	}
	timer_delay2(5);
	net_udp_listen();

	if (Netgame.protocol.udp.valid != 1)
		return 0;		// still trying to connect

	if (dj->connecting == 1)
	{
		if (!net_udp_show_game_info()) // show info menu and check if we join
		{
			dj->connecting = 0;
			return 0;
		}
		else
		{
			// Get full game info again as it could have changed since we entered the info menu.
			dj->connecting = 2;
			Netgame.protocol.udp.valid = 0;
			dj->start_time = timer_query();

			return 0;
		}
	}
		
	dj->connecting = 0;

	return net_udp_do_join_game();
}

static int manual_join_game_handler(newmenu *menu,const d_event &event, direct_join *dj)
{
	newmenu_item *items = newmenu_get_items(menu);

	switch (event.type)
	{
		case EVENT_KEY_COMMAND:
			if (dj->connecting && event_key_get(event) == KEY_ESC)
			{
				dj->connecting = 0;
				nm_set_item_text(items[6], "");
				return 1;
			}
			break;
			
		case EVENT_IDLE:
			if (dj->connecting)
			{
				if (net_udp_game_connect(dj))
					return -2;	// Success!
				else if (!dj->connecting)
					nm_set_item_text(items[6], "");
			}
			break;

		case EVENT_NEWMENU_SELECTED:
		{
			int sockres = -1;

			net_udp_init(); // yes, redundant call but since the menu does not know any better it would allow any IP entry as long as Netgame-entry looks okay... my head hurts...
			if (!convert_text_portstring(dj->myportbuf, UDP_MyPort, false))
				return 1;
			sockres = udp_open_socket(UDP_Socket[0], UDP_MyPort);
			if (sockres != 0)
			{
				return 1;
			}
			uint16_t hostport;
			if (!convert_text_portstring(dj->hostportbuf, hostport, true))
				return 1;
			// Resolve address
			if (udp_dns_filladdr(dj->host_addr, dj->addrbuf, hostport) < 0)
			{
				return 1;
			}
			else
			{
				multi_new_game();
				N_players = 0;
				change_playernum_to(1);
				dj->start_time = timer_query();
				dj->last_time = 0;
				
				Netgame.players[0].protocol.udp.addr = dj->host_addr;
				
				dj->connecting = 1;
				nm_set_item_text(items[6], "Connecting...");
				return 1;
			}

			break;
		}
			
		case EVENT_WINDOW_CLOSE:
			if (!Game_wind) // they cancelled
				net_udp_close();
			d_free(dj);
			break;
			
		default:
			break;
	}
	
	return 0;
}

void net_udp_manual_join_game()
{
	direct_join *dj;
	newmenu_item m[7];
	int nitems = 0;

	CALLOC(dj, direct_join, 1);
	if (!dj)
		return;
	
	net_udp_init();

	snprintf(dj->addrbuf, sizeof(dj->addrbuf), "%s", GameArg.MplUdpHostAddr.c_str());
	snprintf(dj->hostportbuf, sizeof(dj->hostportbuf), "%hu", GameArg.MplUdpHostPort ? GameArg.MplUdpHostPort : UDP_PORT_DEFAULT);

	reset_UDP_MyPort();

	nitems = 0;
	nm_set_item_text(m[nitems++],"GAME ADDRESS OR HOSTNAME:");
	nm_set_item_input(m[nitems++],dj->addrbuf);
	nm_set_item_text(m[nitems++],"GAME PORT:");
	nm_set_item_input(m[nitems++], dj->hostportbuf);
	nm_set_item_text(m[nitems++],"MY PORT:");
	snprintf(dj->myportbuf, sizeof(dj->myportbuf), "%hu", UDP_MyPort);
	nm_set_item_input(m[nitems++], dj->myportbuf);
	nm_set_item_text(m[nitems++],"");

	newmenu_do1( NULL, "ENTER GAME ADDRESS", nitems, m, manual_join_game_handler, dj, 0 );
}

static char *ljtext;

static int net_udp_list_join_poll( newmenu *menu,const d_event &event, direct_join *dj )
{
	// Polling loop for Join Game menu
	int newpage = 0;
	static int NLPage = 0;
	newmenu_item *menus = newmenu_get_items(menu);
	switch (event.type)
	{
		case EVENT_WINDOW_ACTIVATED:
		{
			Netgame.protocol.udp.valid = 0;
			Active_udp_games = {};
			num_active_udp_changed = 1;
			num_active_udp_games = 0;
			net_udp_request_game_info(GBcast, 1);
#ifdef IPv6
			net_udp_request_game_info(GMcast_v6, 1);
#endif
			udp_tracker_reqgames();
			break;
		}
		case EVENT_IDLE:
			if (dj->connecting)
			{
				if (net_udp_game_connect(dj))
					return -2;	// Success!
			}
			break;
		case EVENT_KEY_COMMAND:
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
				net_udp_request_game_info(GBcast, 1);
#ifdef IPv6
				net_udp_request_game_info(GMcast_v6, 1);
#endif
				
				udp_tracker_reqgames();
				// All done
				break;
			}
			if (key == KEY_F5)
			{
				Active_udp_games = {};
				num_active_udp_changed = 1;
				num_active_udp_games = 0;
				net_udp_request_game_info(GBcast, 1);

#ifdef IPv6
				net_udp_request_game_info(GMcast_v6, 1);
#endif
				break;
			}
#ifdef USE_TRACKER
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
				if (dj->connecting)
				{
					dj->connecting = 0;
					return 1;
				}
				break;
			}
			break;
		}
		case EVENT_NEWMENU_SELECTED:
		{
			auto &citem = static_cast<const d_select_event &>(event).citem;
			if (((citem+(NLPage*UDP_NETGAMES_PPAGE)) >= 4) && (((citem+(NLPage*UDP_NETGAMES_PPAGE))-4) <= num_active_udp_games-1))
			{
				multi_new_game();
				N_players = 0;
				change_playernum_to(1);
				dj->start_time = timer_query();
				dj->last_time = 0;
				dj->host_addr = Active_udp_games[(citem+(NLPage*UDP_NETGAMES_PPAGE))-4].game_addr;
				Netgame.players[0].protocol.udp.addr = dj->host_addr;
				dj->connecting = 1;
				return 1;
			}
			else
			{
				nm_messagebox(TXT_SORRY, 1, TXT_OK, TXT_INVALID_CHOICE);
				return -1; // invalid game selected - stay in the menu
			}
			break;
		}
		case EVENT_WINDOW_CLOSE:
		{
			d_free(ljtext);
			d_free(menus);
			d_free(dj);
			if (!Game_wind)
			{
				net_udp_close();
				Network_status = NETSTAT_MENU;	// they cancelled
			}
			return 0;
		}
		default:
			break;
	}

	net_udp_listen();

	if (!num_active_udp_changed && !newpage)
		return 0;

	num_active_udp_changed = 0;

	// Copy the active games data into the menu options
	for (int i = 0; i < UDP_NETGAMES_PPAGE; i++)
	{
		int game_status = Active_udp_games[(i+(NLPage*UDP_NETGAMES_PPAGE))].game_status;
		int j,x, k,tx,ty,ta,nplayers = 0;
		char levelname[8],MissName[25],GameName[25],thold[2];
		thold[1]=0;

		if ((i+(NLPage*UDP_NETGAMES_PPAGE)) >= num_active_udp_games)
		{
			snprintf(menus[i+4].text, sizeof(char)*74, "%d.                                                                      ",(i+(NLPage*UDP_NETGAMES_PPAGE))+1);
			continue;
		}

		// These next two loops protect against menu skewing
		// if missiontitle or gamename contain a tab

		const auto &&fspacx = FSPACX();
		for (x=0,tx=0,k=0,j=0;j<15;j++)
		{
			if (Active_udp_games[(i+(NLPage*UDP_NETGAMES_PPAGE))].mission_title[j]=='\t')
				continue;
			thold[0]=Active_udp_games[(i+(NLPage*UDP_NETGAMES_PPAGE))].mission_title[j];
			gr_get_string_size (thold,&tx,&ty,&ta);

			if ((x += tx) >= fspacx(55))
			{
				MissName[k]=MissName[k+1]=MissName[k+2]='.';
				k+=3;
				break;
			}

			MissName[k++]=Active_udp_games[(i+(NLPage*UDP_NETGAMES_PPAGE))].mission_title[j];
		}
		MissName[k]=0;

		for (x=0,tx=0,k=0,j=0;j<15;j++)
		{
			if (Active_udp_games[(i+(NLPage*UDP_NETGAMES_PPAGE))].game_name[j]=='\t')
				continue;
			thold[0]=Active_udp_games[(i+(NLPage*UDP_NETGAMES_PPAGE))].game_name[j];
			gr_get_string_size (thold,&tx,&ty,&ta);

			if ((x += tx) >= fspacx(55))
			{
				GameName[k]=GameName[k+1]=GameName[k+2]='.';
				k+=3;
				break;
			}
			GameName[k++]=Active_udp_games[(i+(NLPage*UDP_NETGAMES_PPAGE))].game_name[j];
		}
		GameName[k]=0;

		nplayers = Active_udp_games[(i+(NLPage*UDP_NETGAMES_PPAGE))].numconnected;

		if (Active_udp_games[(i+(NLPage*UDP_NETGAMES_PPAGE))].levelnum < 0)
			snprintf(levelname, sizeof(levelname), "S%d", -Active_udp_games[(i+(NLPage*UDP_NETGAMES_PPAGE))].levelnum);
		else
			snprintf(levelname, sizeof(levelname), "%d", Active_udp_games[(i+(NLPage*UDP_NETGAMES_PPAGE))].levelnum);

		const char *status;
		if (game_status == NETSTAT_STARTING)
			status = "FORMING ";
		else if (game_status == NETSTAT_PLAYING)
		{
			if (Active_udp_games[(i+(NLPage*UDP_NETGAMES_PPAGE))].RefusePlayers)
				status = "RESTRICT";
			else if (Active_udp_games[(i+(NLPage*UDP_NETGAMES_PPAGE))].game_flag.closed)
				status = "CLOSED  ";
			else
				status = "OPEN    ";
		}
		else
			status = "BETWEEN ";
		
		unsigned gamemode = Active_udp_games[(i+(NLPage*UDP_NETGAMES_PPAGE))].gamemode;
		snprintf (menus[i+4].text,sizeof(char)*74,"%d.\t%s \t%s \t  %d/%d \t%s \t %s \t%s",(i+(NLPage*UDP_NETGAMES_PPAGE))+1,GameName,(gamemode < sizeof(GMNamesShrt) / sizeof(GMNamesShrt[0])) ? GMNamesShrt[gamemode] : "INVALID",nplayers, Active_udp_games[(i+(NLPage*UDP_NETGAMES_PPAGE))].max_numplayers,MissName,levelname,status);
			
		Assert(strlen(menus[i+4].text) < 75);
	}
	return 0;
}

void net_udp_list_join_game()
{
	newmenu_item *m;
	direct_join *dj;

	CALLOC(m, newmenu_item, ((UDP_NETGAMES_PPAGE+4)*2)+1);
	if (!m)
		return;
	MALLOC(ljtext, char, (((UDP_NETGAMES_PPAGE+4)*2)+1)*74);
	if (!ljtext)
	{
		d_free(m);
		return;
	}
	CALLOC(dj, direct_join, 1);
	if (!dj)
		return;

	net_udp_init();
	auto gamemyport = GameArg.MplUdpMyPort;
	if (udp_open_socket(UDP_Socket[0], gamemyport >= 1024 ? gamemyport : UDP_PORT_DEFAULT) < 0)
		return;

	if (gamemyport >= 1024 && gamemyport != UDP_PORT_DEFAULT)
		if (udp_open_socket(UDP_Socket[1], UDP_PORT_DEFAULT) < 0)
			nm_messagebox(TXT_WARNING, 1, TXT_OK, "Cannot open default port!\nYou can only scan for games\nmanually.");

	// prepare broadcast address to discover games
	udp_init_broadcast_addresses();

	change_playernum_to(1);
	N_players = 0;
	Network_send_objects = 0;
	Network_sending_extras=0;
	Network_rejoined=0;

	Network_status = NETSTAT_BROWSING; // We are looking at a game menu

	net_udp_flush();
	net_udp_listen();  // Throw out old info

	num_active_udp_games = 0;

	Active_udp_games = {};

	gr_set_fontcolor(BM_XRGB(15,15,23),-1);

	nm_set_item_text(m[0], "\tF4/F5/F6: (Re)Scan for all/LAN/Tracker Games." );
	nm_set_item_text(m[1], "\tPgUp/PgDn: Flip Pages." );
	nm_set_item_text(m[2], " " );
	nm_set_item_text(m[3],  "\tGAME \tMODE \t#PLYRS \tMISSION \tLEV \tSTATUS");

	for (int i = 0; i < UDP_NETGAMES_PPAGE; i++) {
		nm_set_item_menu(m[i+4], ljtext + 74 * i);
		snprintf(m[i+4].text,sizeof(char)*74,"%d.                                                                      ", i+1);
	}

	num_active_udp_changed = 1;
	newmenu_dotiny("NETGAMES", NULL,(UDP_NETGAMES_PPAGE+4), m, 1, net_udp_list_join_poll, dj);
}

static void net_udp_send_sequence_packet(UDP_sequence_packet seq, const _sockaddr &recv_addr)
{
	array<uint8_t, UPID_SEQUENCE_SIZE> buf;
	int len = 0;
	buf[0] = seq.type;						len++;
	memcpy(&buf[len], seq.player.callsign.buffer(), CALLSIGN_LEN+1);		len += CALLSIGN_LEN+1;
	buf[len] = seq.player.connected;				len++;
	buf[len] = seq.player.rank;					len++;
	dxx_sendto(recv_addr, UDP_Socket[0], buf, 0);
}

static void net_udp_receive_sequence_packet(ubyte *data, UDP_sequence_packet *seq, const _sockaddr &sender_addr)
{
	int len = 0;
	
	seq->type = data[0];						len++;
	memcpy(seq->player.callsign.buffer(), &(data[len]), CALLSIGN_LEN+1);	len += CALLSIGN_LEN+1;
	seq->player.connected = data[len];				len++;
	memcpy (&(seq->player.rank),&(data[len]),1);			len++;
	
	if (multi_i_am_master())
		seq->player.protocol.udp.addr = sender_addr;
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
		nm_messagebox( TXT_ERROR, 1, TXT_OK, "Cannot init Winsock!"); // no break here... game will fail at socket creation anyways...
}
#endif

	UDP_Socket[0].reset();
	UDP_Socket[1].reset();

	Netgame = {};
	UDP_Seq = {};
	UDP_MData = {};
	net_udp_noloss_init_mdata_queue();
	UDP_Seq.type = UPID_REQUEST;
	UDP_Seq.player.callsign = Players[Player_num].callsign;

	UDP_Seq.player.rank=GetMyNetRanking();	

	multi_new_game();
	net_udp_flush();
	
	// Initialize the tracker info
	udp_tracker_init();
}

void net_udp_close()
{
	range_for (auto &i, UDP_Socket)
		i.reset();
#ifdef _WIN32
	WSACleanup();
#endif
}

// Send PID_ENDLEVEL in regular intervals and listen for them (host also does the packets for playing clients)
int net_udp_kmatrix_poll1( newmenu *,const d_event &event, const unused_newmenu_userdata_t *)
{
	// Polling loop for End-of-level menu
	if (event.type != EVENT_WINDOW_DRAW)
		return 0;
	net_udp_do_frame(0, 1);
	
	return 0;
}

// Same as above but used when player pressed ESC during kmatrix (host also does the packets for playing clients)
int net_udp_kmatrix_poll2( newmenu *,const d_event &event, const unused_newmenu_userdata_t *)
{
	int rval = 0;

	// Polling loop for End-of-level menu
	if (event.type == EVENT_WINDOW_CREATED)
	{
		StartAbortMenuTime=timer_query();
		return 0;
	}
	if (event.type != EVENT_WINDOW_DRAW)
		return 0;
	if (timer_query() > (StartAbortMenuTime+(F1_0*3)))
		rval = -2;

	net_udp_do_frame(0, 1);
	
	return rval;
}

int net_udp_endlevel(int *secret)
{
	// Do whatever needs to be done between levels
#if defined(DXX_BUILD_DESCENT_II)
	if (EMULATING_D1)
#endif
	{
		// We do not really check if a player has actually found a secret level... yeah, I am too lazy! So just go there and pretend we did!
		range_for (const auto i, unchecked_partial_range(Secret_level_table.get(), N_secret_levels))
		{
			if (Current_level_num == i)
			{
				*secret = 1;
				break;
			}
		}
	}
#if defined(DXX_BUILD_DESCENT_II)
	else
		*secret = 0;
#endif

	Network_status = NETSTAT_ENDLEVEL; // We are between levels
	net_udp_listen();
	net_udp_send_endlevel_packet();

	range_for (auto &i, partial_range(Netgame.players, N_players)) 
	{
		i.LastPacketTime = timer_query();
	}
   
	net_udp_send_endlevel_packet();
	net_udp_send_endlevel_packet();

	net_udp_update_netgame();

	return(0);
}

int 
static net_udp_can_join_netgame(netgame_info *game)
{
	// Can this player rejoin a netgame in progress?

	int num_players;

	if (game->game_status == NETSTAT_STARTING)
		return 1;

	if (game->game_status != NETSTAT_PLAYING)
	{
		return 0;
	}

	// Game is in progress, figure out if this guy can re-join it

	num_players = game->numplayers;

	if (!(game->game_flag.closed)) {
		// Look for player that is not connected
		
		if (game->numconnected==game->max_numplayers)
			return (2);
		
		if (game->RefusePlayers)
			return (3);
		
		if (game->numplayers < game->max_numplayers)
			return 1;

		if (game->numconnected<num_players)
			return 1;
	}

	// Search to see if we were already in this closed netgame in progress

	for (int i = 0; i < num_players; i++) {
		if (Players[Player_num].callsign == game->players[i].callsign && i == game->protocol.udp.your_index)
			return 1;
	}
	return 0;
}

// do UDP stuff to disconnect a player. Should ONLY be called from multi_disconnect_player()
void net_udp_disconnect_player(int playernum)
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

void
static net_udp_new_player(UDP_sequence_packet *their)
{
	int pnum;

	pnum = their->player.connected;

	Assert(pnum >= 0);
	Assert(pnum < Netgame.max_numplayers);
	
	if (Newdemo_state == ND_STATE_RECORDING) {
		int new_player;

		if (pnum == N_players)
			new_player = 1;
		else
			new_player = 0;
		newdemo_record_multi_connect(pnum, new_player, their->player.callsign);
	}

	Players[pnum].callsign = their->player.callsign;
	Netgame.players[pnum].callsign = their->player.callsign;
	Netgame.players[pnum].protocol.udp.addr = their->player.protocol.udp.addr;

	ClipRank (&their->player.rank);
	Netgame.players[pnum].rank=their->player.rank;

	Players[pnum].connected = CONNECT_PLAYING;
	Players[pnum].net_kills_total = 0;
	Players[pnum].net_killed_total = 0;
	kill_matrix[pnum] = {};
	Players[pnum].score = 0;
	Players[pnum].flags = 0;
	Players[pnum].KillGoalCount=0;

	if (pnum == N_players)
	{
		N_players++;
		Netgame.numplayers = N_players;
	}

	digi_play_sample(SOUND_HUD_MESSAGE, F1_0);

	ClipRank (&their->player.rank);

	if (PlayerCfg.NoRankings)
		HUD_init_message(HM_MULTI, "'%s' %s\n", static_cast<const char *>(their->player.callsign), TXT_JOINING);
	else
		HUD_init_message(HM_MULTI, "%s'%s' %s\n",RankStrings[their->player.rank],static_cast<const char *>(their->player.callsign), TXT_JOINING);
	
	multi_make_ghost_player(pnum);

	multi_send_score();
#if defined(DXX_BUILD_DESCENT_II)
	multi_sort_kill_list();
#endif

	net_udp_noloss_clear_mdata_trace(pnum);
}

static void net_udp_welcome_player(UDP_sequence_packet *their)
{
	// Add a player to a game already in progress
	int player_num;
	WaitForRefuseAnswer=0;

	// Don't accept new players if we're ending this level.  Its safe to
	// ignore since they'll request again later

	if ((Network_status == NETSTAT_ENDLEVEL) || (Control_center_destroyed))
	{
		net_udp_dump_player(their->player.protocol.udp.addr, DUMP_ENDLEVEL);
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

	if (their->player.connected != Current_level_num)
	{
		net_udp_dump_player(their->player.protocol.udp.addr, DUMP_LEVEL);
		return;
	}

	player_num = -1;
	UDP_sync_player = {};
	Network_player_added = 0;

	for (int i = 0; i < N_players; i++)
	{
		if (Players[i].callsign == their->player.callsign &&
			their->player.protocol.udp.addr == Netgame.players[i].protocol.udp.addr)
		{
			player_num = i;
			break;
		}
	}

	if (player_num == -1)
	{
		// Player is new to this game

		if ( !(Netgame.game_flag.closed) && (N_players < Netgame.max_numplayers))
		{
			// Add player in an open slot, game not full yet

			player_num = N_players;
			Network_player_added = 1;
		}
		else if (Netgame.game_flag.closed)
		{
			// Slots are open but game is closed
			net_udp_dump_player(their->player.protocol.udp.addr, DUMP_CLOSED);
			return;
		}
		else
		{
			// Slots are full but game is open, see if anyone is
			// disconnected and replace the oldest player with this new one
		
			int oldest_player = -1;
			fix64 oldest_time = timer_query();
			int activeplayers = 0;

			Assert(N_players == Netgame.max_numplayers);

			range_for (auto &i, partial_range(Netgame.players, Netgame.numplayers))
				if (i.connected)
					activeplayers++;

			if (activeplayers == Netgame.max_numplayers)
			{
				// Game is full.
				net_udp_dump_player(their->player.protocol.udp.addr, DUMP_FULL);
				return;
			}

			for (int i = 0; i < N_players; i++)
			{
				if ( (!Players[i].connected) && (Netgame.players[i].LastPacketTime < oldest_time))
				{
					oldest_time = Netgame.players[i].LastPacketTime;
					oldest_player = i;
				}
			}

			if (oldest_player == -1)
			{
				// Everyone is still connected 
				net_udp_dump_player(their->player.protocol.udp.addr, DUMP_FULL);
				return;
			}
			else
			{
				// Found a slot!

				player_num = oldest_player;
				Network_player_added = 1;
			}
		}
	}
	else
	{
		// Player is reconnecting
		
		if (Players[player_num].connected)
		{
			return;
		}

		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_multi_reconnect(player_num);

		Network_player_added = 0;

		digi_play_sample(SOUND_HUD_MESSAGE, F1_0);

		if (PlayerCfg.NoRankings)
			HUD_init_message(HM_MULTI, "'%s' %s", static_cast<const char *>(Players[player_num].callsign), TXT_REJOIN);
		else
			HUD_init_message(HM_MULTI, "%s'%s' %s", RankStrings[Netgame.players[player_num].rank],static_cast<const char *>(Players[player_num].callsign), TXT_REJOIN);

		multi_send_score();

		net_udp_noloss_clear_mdata_trace(player_num);
	}

	Players[player_num].KillGoalCount=0;

	// Send updated Objects data to the new/returning player

	
	UDP_sync_player = *their;
	UDP_sync_player.player.connected = player_num;
	Network_send_objects = 1;
	Network_send_objnum = -1;
	Netgame.players[player_num].LastPacketTime = timer_query();

	net_udp_send_objects();
}

int net_udp_objnum_is_past(objnum_t objnum)
{
	// determine whether or not a given object number has already been sent
	// to a re-joining player.
	
	int player_num = UDP_sync_player.player.connected;
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

#if defined(DXX_BUILD_DESCENT_I)
static void net_udp_send_door_updates(void)
{
	// Send door status when new player joins
	for (int i = 0; i < Num_walls; i++)
	{
		if ((Walls[i].type == WALL_DOOR) && ((Walls[i].state == WALL_DOOR_OPENING) || (Walls[i].state == WALL_DOOR_WAITING)))
			multi_send_door_open(Walls[i].segnum, Walls[i].sidenum,0);
		else if ((Walls[i].type == WALL_BLASTABLE) && (Walls[i].flags & WALL_BLASTED))
			multi_send_door_open(Walls[i].segnum, Walls[i].sidenum,0);
		else if ((Walls[i].type == WALL_BLASTABLE) && (Walls[i].hps != WALL_HPS))
			multi_send_hostage_door_status(i);
	}

}
#elif defined(DXX_BUILD_DESCENT_II)
static void net_udp_send_door_updates(const playernum_t pnum)
{
	// Send door status when new player joins
	for (int i = 0; i < Num_walls; i++)
	{
      if ((Walls[i].type == WALL_DOOR) && ((Walls[i].state == WALL_DOOR_OPENING) || (Walls[i].state == WALL_DOOR_WAITING) || (Walls[i].state == WALL_DOOR_OPEN)))
			multi_send_door_open_specific(pnum,Walls[i].segnum, Walls[i].sidenum,Walls[i].flags);
		else if ((Walls[i].type == WALL_BLASTABLE) && (Walls[i].flags & WALL_BLASTED))
			multi_send_door_open_specific(pnum,Walls[i].segnum, Walls[i].sidenum,Walls[i].flags);
		else if ((Walls[i].type == WALL_BLASTABLE) && (Walls[i].hps != WALL_HPS))
			multi_send_hostage_door_status(i);
		else
			multi_send_wall_status_specific(pnum,i,Walls[i].type,Walls[i].flags,Walls[i].state);
	}
}
#endif

static void net_udp_process_monitor_vector(uint32_t vector)
{
	if (!vector)
		return;
	range_for (const auto i, highest_valid(Segments))
	{
		const auto &&seg = vsegptr(static_cast<segnum_t>(i));
		int tm, ec, bm;
		range_for (auto &j, seg->sides)
		{
			if ( ((tm = j.tmap_num2) != 0) &&
				  ((ec = TmapInfo[tm&0x3fff].eclip_num) != -1) &&
				  ((bm = Effects[ec].dest_bm_num) != -1) )
			{
				if (vector & 1)
				{
					j.tmap_num2 = bm | (tm&0xc000);
				}
				if (!(vector >>= 1))
					return;
			}
		}
	}
}

namespace {

class blown_bitmap_array
{
#if defined(DXX_BUILD_DESCENT_I)
#define NUM_BLOWN_BITMAPS 7
#elif defined(DXX_BUILD_DESCENT_II)
#define NUM_BLOWN_BITMAPS 20
#endif
	typedef int T;
	typedef array<T, NUM_BLOWN_BITMAPS> array_t;
	typedef array_t::const_iterator const_iterator;
	array_t a;
	array_t::iterator e;
public:
	blown_bitmap_array() :
		e(a.begin())
	{
	}
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
			throw std::length_error("too many blown bitmaps");
		*e = t;
		++e;
	}
};

}

static int net_udp_create_monitor_vector(void)
{
	int monitor_num = 0;
	int vector = 0;
	blown_bitmap_array blown_bitmaps;
	range_for (auto &i, partial_range(Effects, Num_effects))
	{
		if (i.dest_bm_num > 0) {
			blown_bitmaps.insert_unique(i.dest_bm_num);
		}
	}
		
	range_for (const auto i, highest_valid(Segments))
	{
		const auto &&seg = vsegptr(static_cast<segnum_t>(i));
		int tm, ec;
		range_for (auto &j, seg->sides)
		{
			if ((tm = j.tmap_num2) != 0)
			{
				if ( ((ec = TmapInfo[tm&0x3fff].eclip_num) != -1) &&
					  (Effects[ec].dest_bm_num != -1) )
				{
					monitor_num++;
					Assert(monitor_num < 32);
				}
				else
				{
					if (blown_bitmaps.exists(tm&0x3fff))
					{
							vector |= (1 << monitor_num);
							monitor_num++;
							Assert(monitor_num < 32);
					}
				}
			}
		}
	}
	return(vector);
}

static void net_udp_stop_resync(UDP_sequence_packet *their)
{
	if (UDP_sync_player.player.protocol.udp.addr == their->player.protocol.udp.addr &&
		(!d_stricmp(UDP_sync_player.player.callsign, their->player.callsign)) )
	{
		Network_send_objects = 0;
		Network_sending_extras=0;
		Network_rejoined=0;
		Player_joining_extras=-1;
		Network_send_objnum = -1;
	}
}

void net_udp_send_objects(void)
{
	sbyte owner, player_num = UDP_sync_player.player.connected;
	static int obj_count = 0;
	int loc = 0, remote_objnum = 0, obj_count_frame = 0;
	static fix64 last_send_time = 0;
	
	if (last_send_time + (F1_0/50) > timer_query())
		return;
	last_send_time = timer_query();

	// Send clear objects array trigger and send player num

	Assert(Network_send_objects != 0);
	Assert(player_num >= 0);
	Assert(player_num < Netgame.max_numplayers);

	if ((Network_status == NETSTAT_ENDLEVEL) || Control_center_destroyed)
	{
		// Endlevel started before we finished sending the goods, we'll
		// have to stop and try again after the level.
		net_udp_dump_player(UDP_sync_player.player.protocol.udp.addr, DUMP_ENDLEVEL);
		Network_send_objects = 0; 
		return;
	}

	array<uint8_t, UPID_MAX_SIZE> object_buffer;
	object_buffer.fill(0);
	object_buffer[0] = UPID_OBJECT_DATA;
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
		if ((Objects[i].type != OBJ_POWERUP) && (Objects[i].type != OBJ_PLAYER) &&
				(Objects[i].type != OBJ_CNTRLCEN) && (Objects[i].type != OBJ_GHOST) &&
				(Objects[i].type != OBJ_ROBOT) && (Objects[i].type != OBJ_HOSTAGE)
#if defined(DXX_BUILD_DESCENT_II)
				&& !(Objects[i].type==OBJ_WEAPON && get_weapon_id(&Objects[i])==PMINE_ID)
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

		remote_objnum = objnum_local_to_remote(i, &owner);
		Assert(owner == object_owner[i]);

		PUT_INTEL_INT(&object_buffer[loc], i);                        loc += 4;
		object_buffer[loc] = owner;                                 loc += 1;
		PUT_INTEL_INT(&object_buffer[loc], remote_objnum);            loc += 4;
		// use object_rw to send objects for now. if object sometime contains some day contains something useful the client should know about, we should use it. but by now it's also easier to use object_rw because then we also do not need fix64 timer values.
		multi_object_to_object_rw(&Objects[i], (object_rw *)&object_buffer[loc]);
		if (words_bigendian)
			object_rw_swap(reinterpret_cast<object_rw *>(&object_buffer[loc]), 1);
		loc += sizeof(object_rw);
	}

	if (obj_count_frame) // Send any objects we've buffered
	{
		Network_send_objnum = i;
		PUT_INTEL_INT(&object_buffer[1], obj_count_frame);

		Assert(loc <= UPID_MAX_SIZE);

		dxx_sendto(UDP_sync_player.player.protocol.udp.addr, UDP_Socket[0], &object_buffer[0], loc, 0);
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
			object_buffer[0] = UPID_OBJECT_DATA;
			PUT_INTEL_INT(&object_buffer[1], 1);
			PUT_INTEL_INT(&object_buffer[5], -2);
			object_buffer[9] = player_num;
			PUT_INTEL_INT(&object_buffer[10], obj_count);
			dxx_sendto(UDP_sync_player.player.protocol.udp.addr, UDP_Socket[0], &object_buffer[0], 14, 0);

			// Send sync packet which tells the player who he is and to start!
			net_udp_send_rejoin_sync(player_num);

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

static int net_udp_verify_objects(int remote, int local)
{
	int nplayers = 0;

	if ((remote-local) > 10)
		return(2);

	range_for (const auto i, highest_valid(Objects))
	{
		const auto &&objp = vobjptr(static_cast<objnum_t>(i));
		if (objp->type == OBJ_PLAYER || objp->type == OBJ_GHOST)
			nplayers++;
	}

	if (Netgame.max_numplayers<=nplayers)
		return(0);

	return(1);
}

static void net_udp_read_object_packet( ubyte *data )
{
	// Object from another net player we need to sync with
	sbyte obj_owner;
	static int mode = 0, object_count = 0, my_pnum = 0;
	int remote_objnum = 0, nobj = 0, loc = 5;
	
	nobj = GET_INTEL_INT(data + 1);

	for (int i = 0; i < nobj; i++)
	{
		objnum_t objnum = GET_INTEL_INT(data + loc);                         loc += 4;
		obj_owner = data[loc];                                      loc += 1;
		remote_objnum = GET_INTEL_INT(data + loc);                  loc += 4;

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
		else if (objnum == object_guidebot_cannot_reach)
		{
			// Special debug checksum marker for entire send
			if (mode == 1)
			{
				special_reset_objects();
				mode = 0;
			}
			if (remote_objnum != object_count) {
				Int3();
			}
			if (net_udp_verify_objects(remote_objnum, object_count))
			{
				// Failed to sync up 
				nm_messagebox(NULL, 1, TXT_OK, TXT_NET_SYNC_FAILED);
				Network_status = NETSTAT_MENU;                          
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
					special_reset_objects();
					mode = 0;
				}
				objnum = obj_allocate();
			}
			if (objnum != object_none) {
				auto obj = vobjptridx(objnum);
				if (obj->segnum != segment_none)
				{
					obj_unlink(obj);
					Assert(obj->segnum == segment_none);
				}
				Assert(objnum < MAX_OBJECTS);
				if (words_bigendian)
					object_rw_swap(reinterpret_cast<object_rw *>(&data[loc]), 1);
				multi_object_rw_to_object((object_rw *)&data[loc], obj);
				loc += sizeof(object_rw);
				auto segnum = obj->segnum;
				obj->next = obj->prev = object_none;
				obj->segnum = segment_none;
				obj->attached_obj = object_none;
				if (segnum != segment_none)
					obj_link(obj,segnum);
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

void net_udp_send_rejoin_sync(int player_num)
{
	Players[player_num].connected = CONNECT_PLAYING; // connect the new guy
	Netgame.players[player_num].LastPacketTime = timer_query();

	if ((Network_status == NETSTAT_ENDLEVEL) || Control_center_destroyed)
	{
		// Endlevel started before we finished sending the goods, we'll
		// have to stop and try again after the level.

		net_udp_dump_player(UDP_sync_player.player.protocol.udp.addr, DUMP_ENDLEVEL);

		Network_send_objects = 0; 
		Network_sending_extras=0;
		return;
	}

	if (Network_player_added)
	{
		UDP_sync_player.type = UPID_ADDPLAYER;
		UDP_sync_player.player.connected = player_num;
		net_udp_new_player(&UDP_sync_player);

		for (int i = 0; i < N_players; i++)
		{
			if ((i != player_num) && (i != Player_num) && (Players[i].connected))
				net_udp_send_sequence_packet( UDP_sync_player, Netgame.players[i].protocol.udp.addr);
		}
	}

	// Send sync packet to the new guy

	net_udp_update_netgame();

	// Fill in the kill list
	Netgame.kills = kill_matrix;
	for (int j=0; j<MAX_PLAYERS; j++)
	{
		Netgame.killed[j] = Players[j].net_killed_total;
		Netgame.player_kills[j] = Players[j].net_kills_total;
		Netgame.player_score[j] = Players[j].score;
	}

	Netgame.level_time = Players[Player_num].time_level;
	Netgame.monitor_vector = net_udp_create_monitor_vector();

	net_udp_send_game_info(UDP_sync_player.player.protocol.udp.addr, &UDP_sync_player.player.protocol.udp.addr, UPID_SYNC);
#if defined(DXX_BUILD_DESCENT_I)
	net_udp_send_door_updates();
#endif

	return;
}

static void net_udp_resend_sync_due_to_packet_loss()
{
	if (!multi_i_am_master())
		return;

	net_udp_update_netgame();

	// Fill in the kill list
	Netgame.kills = kill_matrix;
	for (int j=0; j<MAX_PLAYERS; j++)
	{
		Netgame.killed[j] = Players[j].net_killed_total;
		Netgame.player_kills[j] = Players[j].net_kills_total;
		Netgame.player_score[j] = Players[j].score;
	}

	Netgame.level_time = Players[Player_num].time_level;
	Netgame.monitor_vector = net_udp_create_monitor_vector();

	net_udp_send_game_info(UDP_sync_player.player.protocol.udp.addr, &UDP_sync_player.player.protocol.udp.addr, UPID_SYNC);
}

static void net_udp_add_player(UDP_sequence_packet *p)
{
	range_for (auto &i, partial_range(Netgame.players, N_players))
	{
		if (i.protocol.udp.addr == p->player.protocol.udp.addr)
		{
			i.LastPacketTime = timer_query();
			return;		// already got them
		}
	}

	if ( N_players >= MAX_PLAYERS )
	{
		return;		// too many of em
	}

	ClipRank (&p->player.rank);
	Netgame.players[N_players].callsign = p->player.callsign;
	Netgame.players[N_players].protocol.udp.addr = p->player.protocol.udp.addr;
	Netgame.players[N_players].rank=p->player.rank;
	Netgame.players[N_players].connected = CONNECT_PLAYING;
	Players[N_players].KillGoalCount=0;
	Players[N_players].connected = CONNECT_PLAYING;
	Netgame.players[N_players].LastPacketTime = timer_query();
	N_players++;
	Netgame.numplayers = N_players;

	net_udp_send_netgame_update();
}

// One of the players decided not to join the game

static void net_udp_remove_player(UDP_sequence_packet *p)
{
	int pn;
	
	pn = -1;
	for (int i=0; i<N_players; i++ )
	{
		if (Netgame.players[i].protocol.udp.addr == p->player.protocol.udp.addr)
		{
			pn = i;
			break;
		}
	}
	
	if (pn < 0 )
		return;

	for (int i=pn; i<N_players-1; i++ )
	{
		Netgame.players[i].callsign = Netgame.players[i+1].callsign;
		Netgame.players[i].protocol.udp.addr = Netgame.players[i+1].protocol.udp.addr;
		Netgame.players[i].rank=Netgame.players[i+1].rank;
		ClipRank (&Netgame.players[i].rank);
	}
		
	N_players--;
	Netgame.numplayers = N_players;

	net_udp_send_netgame_update();
}

void net_udp_dump_player(const _sockaddr &dump_addr, int why)
{
	// Inform player that he was not chosen for the netgame
	array<uint8_t, UPID_DUMP_SIZE> buf;
	buf[0] = UPID_DUMP;
	buf[1] = why;
	dxx_sendto(dump_addr, UDP_Socket[0], buf, 0);
	if (multi_i_am_master())
		for (playernum_t i = 1; i < N_players; i++)
			if (dump_addr == Netgame.players[i].protocol.udp.addr)
				multi_disconnect_player(i);
}

void net_udp_update_netgame(void)
{
	// Update the netgame struct with current game variables
	Netgame.numconnected=0;
	range_for (auto &i, partial_range(Players, N_players))
		if (i.connected)
			Netgame.numconnected++;

#if defined(DXX_BUILD_DESCENT_II)
// This is great: D2 1.0 and 1.1 ignore upper part of the game_flags field of
//	the lite_info struct when you're sitting on the join netgame screen.  We can
//	"sneak" Hoard information into this field.  This is better than sending 
//	another packet that could be lost in transit.

	if (HoardEquipped())
	{
		if (game_mode_hoard())
		{
			Netgame.game_flag.hoard = 1;
			Netgame.game_flag.team_hoard = !!(Game_mode & GM_TEAM);
		}
		else
		{
			Netgame.game_flag.hoard = 0;
			Netgame.game_flag.team_hoard = 0;
		}
	}
#endif
	if (Network_status == NETSTAT_STARTING)
		return;

	Netgame.numplayers = N_players;
	Netgame.game_status = Network_status;

	Netgame.kills = kill_matrix;
	for (int i = 0; i < MAX_PLAYERS; i++) 
	{
		Netgame.players[i].connected = Players[i].connected;
		Netgame.killed[i] = Players[i].net_killed_total;
		Netgame.player_kills[i] = Players[i].net_kills_total;
#if defined(DXX_BUILD_DESCENT_II)
		Netgame.player_score[i] = Players[i].score;
		Netgame.player_flags[i] = (Players[i].flags & (PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_GOLD_KEY));
#endif
	}
	Netgame.team_kills = team_kills;
	Netgame.levelnum = Current_level_num;
}

/* Send an updated endlevel status to everyone (if we are host) or host (if we are client)  */
void net_udp_send_endlevel_packet(void)
{
	int len = 0;

	if (multi_i_am_master())
	{
		ubyte buf[UPID_MAX_SIZE];

		memset(buf, 0, sizeof(buf));

		buf[len] = UPID_ENDLEVEL_H;											len++;
		buf[len] = Countdown_seconds_left;									len++;

		range_for (auto &i, Players)
		{
			buf[len] = i.connected;								len++;
			PUT_INTEL_SHORT(buf + len, i.net_kills_total);			len += 2;
			PUT_INTEL_SHORT(buf + len, i.net_killed_total);		len += 2;
		}

		range_for (auto &i, kill_matrix)
		{
			range_for (auto &j, i)
			{
				PUT_INTEL_SHORT(buf + len, j);				len += 2;
			}
		}

		for (int i = 1; i < MAX_PLAYERS; i++)
			if (Players[i].connected != CONNECT_DISCONNECTED)
				dxx_sendto(Netgame.players[i].protocol.udp.addr, UDP_Socket[0], buf, len, 0);
	}
	else
	{
		ubyte buf[UPID_MAX_SIZE];

		memset(buf, 0,  sizeof(buf));

		buf[len] = UPID_ENDLEVEL_C;											len++;
		buf[len] = Player_num;												len++;
		buf[len] = Players[Player_num].connected;							len++;
		buf[len] = Countdown_seconds_left;									len++;
		PUT_INTEL_SHORT(buf + len, Players[Player_num].net_kills_total);	len += 2;
		PUT_INTEL_SHORT(buf + len, Players[Player_num].net_killed_total);	len += 2;

		range_for (auto &i, kill_matrix[Player_num])
		{
			PUT_INTEL_SHORT(buf + len, i);			len += 2;
		}

		dxx_sendto(Netgame.players[0].protocol.udp.addr, UDP_Socket[0], buf, len, 0);
	}
}

static void net_udp_send_version_deny(const _sockaddr &sender_addr)
{
	array<uint8_t, UPID_VERSION_DENY_SIZE> buf;
	buf[0] = UPID_VERSION_DENY;
	PUT_INTEL_SHORT(&buf[1], DXX_VERSION_MAJORi);
	PUT_INTEL_SHORT(&buf[3], DXX_VERSION_MINORi);
	PUT_INTEL_SHORT(&buf[5], DXX_VERSION_MICROi);
	PUT_INTEL_SHORT(&buf[7], MULTI_PROTO_VERSION);
	dxx_sendto(sender_addr, UDP_Socket[0], buf, 0);
}

static void net_udp_process_version_deny(ubyte *data, const _sockaddr &)
{
	Netgame.protocol.udp.program_iver[0] = GET_INTEL_SHORT(&data[1]);
	Netgame.protocol.udp.program_iver[1] = GET_INTEL_SHORT(&data[3]);
	Netgame.protocol.udp.program_iver[2] = GET_INTEL_SHORT(&data[5]);
	Netgame.protocol.udp.program_iver[3] = GET_INTEL_SHORT(&data[7]);
	Netgame.protocol.udp.valid = -1;
}

// Check request for game info. Return 1 if sucessful; -1 if version mismatch; 0 if wrong game or some other error - do not process
static int net_udp_check_game_info_request(ubyte *data, int lite)
{
	short sender_iver[4] = { 0, 0, 0, 0 };
	char sender_id[4] = "";

	memcpy(&sender_id, &(data[1]), 4);
	sender_iver[0] = GET_INTEL_SHORT(&(data[5]));
	sender_iver[1] = GET_INTEL_SHORT(&(data[7]));
	sender_iver[2] = GET_INTEL_SHORT(&(data[9]));
	if (!lite)
		sender_iver[3] = GET_INTEL_SHORT(&(data[11]));
	
	if (memcmp(&sender_id, UDP_REQ_ID, 4))
		return 0;
	
	if ((sender_iver[0] != DXX_VERSION_MAJORi) || (sender_iver[1] != DXX_VERSION_MINORi) || (sender_iver[2] != DXX_VERSION_MICROi) || (!lite && sender_iver[3] != MULTI_PROTO_VERSION))
		return -1;
		
	return 1;
}

namespace {

struct game_info_light
{
	array<uint8_t, UPID_GAME_INFO_LITE_SIZE_MAX> buf;
};

struct game_info_heavy
{
	array<uint8_t, UPID_GAME_INFO_SIZE_MAX> buf;
};

static uint_fast32_t net_udp_prepare_light_game_info(game_info_light &info)
{
	uint_fast32_t len = 0;
	uint8_t *const buf = info.buf.data();
		buf[0] = UPID_GAME_INFO_LITE;								len++;				// 1
		PUT_INTEL_SHORT(buf + len, DXX_VERSION_MAJORi); 						len += 2;			// 3
		PUT_INTEL_SHORT(buf + len, DXX_VERSION_MINORi); 						len += 2;			// 5
		PUT_INTEL_SHORT(buf + len, DXX_VERSION_MICROi); 						len += 2;			// 7
		PUT_INTEL_INT(buf + len, Netgame.protocol.udp.GameID);				len += 4;			// 11
		PUT_INTEL_INT(buf + len, Netgame.levelnum);					len += 4;
		buf[len] = Netgame.gamemode;							len++;
		buf[len] = Netgame.RefusePlayers;						len++;
		buf[len] = Netgame.difficulty;							len++;
		int tmpvar;
		tmpvar = Netgame.game_status;
		if ((Network_status == NETSTAT_ENDLEVEL) || Control_center_destroyed)
			tmpvar = NETSTAT_ENDLEVEL;
		if (Netgame.PlayTimeAllowed)
		{
			if ( (f2i((i2f (Netgame.PlayTimeAllowed*5*60))-ThisLevelTime)) < 30 )
			{
				tmpvar = NETSTAT_ENDLEVEL;
			}
		}
		buf[len] = tmpvar;								len++;
		buf[len] = Netgame.numconnected;						len++;
		buf[len] = Netgame.max_numplayers;						len++;
		buf[len] = pack_game_flags(&Netgame.game_flag).value;							len++;
		copy_from_ntstring(buf, len, Netgame.game_name);
		copy_from_ntstring(buf, len, Netgame.mission_title);
		copy_from_ntstring(buf, len, Netgame.mission_name);
	return len;
}

static uint_fast32_t net_udp_prepare_heavy_game_info(const _sockaddr *addr, ubyte info_upid, game_info_heavy &info)
{
	uint8_t *const buf = info.buf.data();
	uint_fast32_t len = 0;

		buf[0] = info_upid;								len++;
		PUT_INTEL_SHORT(buf + len, DXX_VERSION_MAJORi); 						len += 2;
		PUT_INTEL_SHORT(buf + len, DXX_VERSION_MINORi); 						len += 2;
		PUT_INTEL_SHORT(buf + len, DXX_VERSION_MICROi); 						len += 2;
		ubyte &your_index = buf[len++];
		your_index = MULTI_PNUM_UNDEF;
		for (int i = 0; i < Netgame.players.size(); i++)
		{
			memcpy(&buf[len], Netgame.players[i].callsign.buffer(), CALLSIGN_LEN+1); 	len += CALLSIGN_LEN+1;
			buf[len] = Netgame.players[i].connected;				len++;
			buf[len] = Netgame.players[i].rank;					len++;
			if (addr && *addr == Netgame.players[i].protocol.udp.addr)
				your_index = i;
		}
		PUT_INTEL_INT(buf + len, Netgame.levelnum);					len += 4;
		buf[len] = Netgame.gamemode;							len++;
		buf[len] = Netgame.RefusePlayers;						len++;
		buf[len] = Netgame.difficulty;							len++;
		int tmpvar;
		tmpvar = Netgame.game_status;
		if ((Network_status == NETSTAT_ENDLEVEL) || Control_center_destroyed)
			tmpvar = NETSTAT_ENDLEVEL;
		if (Netgame.PlayTimeAllowed)
		{
			if ( (f2i((i2f (Netgame.PlayTimeAllowed*5*60))-ThisLevelTime)) < 30 )
			{
				tmpvar = NETSTAT_ENDLEVEL;
			}
		}
		buf[len] = tmpvar;								len++;
		buf[len] = Netgame.numplayers;							len++;
		buf[len] = Netgame.max_numplayers;						len++;
		buf[len] = Netgame.numconnected;						len++;
		buf[len] = pack_game_flags(&Netgame.game_flag).value;							len++;
		buf[len] = Netgame.team_vector;							len++;
		PUT_INTEL_INT(buf + len, Netgame.AllowedItems);					len += 4;
#if defined(DXX_BUILD_DESCENT_I)
		buf[len] = Netgame.SpawnGrantedItems.mask;			len += 1;
		buf[len] = Netgame.DuplicatePowerups.get_packed_field();			len += 1;
#elif defined(DXX_BUILD_DESCENT_II)
		PUT_INTEL_SHORT(buf + len, Netgame.SpawnGrantedItems.mask);			len += 2;
		PUT_INTEL_SHORT(buf + len, Netgame.DuplicatePowerups.get_packed_field());			len += 2;
		PUT_INTEL_SHORT(buf + len, Netgame.Allow_marker_view);				len += 2;
		PUT_INTEL_SHORT(buf + len, Netgame.AlwaysLighting);				len += 2;
#endif
		PUT_INTEL_SHORT(buf + len, Netgame.ShowEnemyNames);				len += 2;
		PUT_INTEL_SHORT(buf + len, Netgame.BrightPlayers);				len += 2;
		PUT_INTEL_SHORT(buf + len, Netgame.InvulAppear);				len += 2;
		range_for (const auto &i, Netgame.team_name)
		{
			memcpy(&buf[len], static_cast<const char *>(i), (CALLSIGN_LEN+1));
			len += CALLSIGN_LEN + 1;
		}
		range_for (auto &i, Netgame.locations)
		{
			PUT_INTEL_INT(buf + len, i);				len += 4;
		}
		range_for (auto &i, Netgame.kills)
		{
			range_for (auto &j, i)
			{
				PUT_INTEL_SHORT(buf + len, j);		len += 2;
			}
		}
		PUT_INTEL_SHORT(buf + len, Netgame.segments_checksum);			len += 2;
		PUT_INTEL_SHORT(buf + len, Netgame.team_kills[0]);				len += 2;
		PUT_INTEL_SHORT(buf + len, Netgame.team_kills[1]);				len += 2;
		range_for (auto &i, Netgame.killed)
		{
			PUT_INTEL_SHORT(buf + len, i);				len += 2;
		}
		range_for (auto &i, Netgame.player_kills)
		{
			PUT_INTEL_SHORT(buf + len, i);			len += 2;
		}
		PUT_INTEL_INT(buf + len, Netgame.KillGoal);					len += 4;
		PUT_INTEL_INT(buf + len, Netgame.PlayTimeAllowed);				len += 4;
		PUT_INTEL_INT(buf + len, Netgame.level_time);					len += 4;
		PUT_INTEL_INT(buf + len, Netgame.control_invul_time);				len += 4;
		PUT_INTEL_INT(buf + len, Netgame.monitor_vector);				len += 4;
		range_for (auto &i, Netgame.player_score)
		{
			PUT_INTEL_INT(buf + len, i);			len += 4;
		}
		range_for (auto &i, Netgame.player_flags)
		{
			buf[len] = i;					len++;
		}
		PUT_INTEL_SHORT(buf + len, Netgame.PacketsPerSec);				len += 2;
		buf[len] = Netgame.PacketLossPrevention;					len++;
		buf[len] = Netgame.NoFriendlyFire;						len++;
		copy_from_ntstring(buf, len, Netgame.game_name);
		copy_from_ntstring(buf, len, Netgame.mission_title);
		copy_from_ntstring(buf, len, Netgame.mission_name);
	return len;
}

void net_udp_send_game_info_t::apply(const sockaddr &sender_addr, socklen_t senderlen, const _sockaddr *player_address, ubyte info_upid)
{
	// Send game info to someone who requested it
	net_udp_update_netgame(); // Update the values in the netgame struct
	union {
		game_info_light light;
		game_info_heavy heavy;
	};
	std::size_t len;
	const uint8_t *info;
	if (info_upid == UPID_GAME_INFO_LITE)
	{
		len = net_udp_prepare_light_game_info(light);
		info = light.buf.data();
	}
	else
	{
		len = net_udp_prepare_heavy_game_info(player_address, info_upid, heavy);
		info = heavy.buf.data();
	}
	dxx_sendto(sender_addr, senderlen, UDP_Socket[0], info, len, 0);
}

}

static void net_udp_broadcast_game_info(ubyte info_upid)
{
	net_udp_send_game_info(GBcast, nullptr, info_upid);
#ifdef IPv6
	net_udp_send_game_info(GMcast_v6, nullptr, info_upid);
#endif
}

/* Send game info to all players in this game. Also send lite_info for people watching the netlist */
void net_udp_send_netgame_update()
{
	for (int i=1; i<N_players; i++ )
	{
		if (Players[i].connected == CONNECT_DISCONNECTED)
			continue;
		const auto &addr = Netgame.players[i].protocol.udp.addr;
		net_udp_send_game_info(addr, &addr, UPID_GAME_INFO);
	}
	net_udp_broadcast_game_info(UPID_GAME_INFO_LITE);
}

static unsigned net_udp_send_request(void)
{
	// Send a request to join a game 'Netgame'.  Returns 0 if we can join this
	// game, non-zero if there is some problem.
	auto b = Netgame.players.begin();
	auto e = Netgame.players.end();
	auto i = std::find_if(b, e, [](const netplayer_info &ni) { return ni.connected != 0; });
	if (i == e)
	{
		Assert(false);
		return std::distance(b, i);
	}
	UDP_Seq.type = UPID_REQUEST;
	UDP_Seq.player.connected = Current_level_num;

	net_udp_send_sequence_packet(UDP_Seq, Netgame.players[0].protocol.udp.addr);
	return std::distance(b, i);
}

static void net_udp_process_game_info(const uint8_t *data, uint_fast32_t, const _sockaddr &game_addr, int lite_info)
{
	uint_fast32_t len = 0;
	if (lite_info)
	{
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
		recv_game.gamemode = data[len];							len++;
		recv_game.RefusePlayers = data[len];						len++;
		recv_game.difficulty = data[len];						len++;
		recv_game.game_status = data[len];						len++;
		recv_game.numconnected = data[len];						len++;
		recv_game.max_numplayers = data[len];						len++;
		packed_game_flags p;
		p.value = data[len];
		recv_game.game_flag = unpack_game_flags(&p);						len++;
		copy_to_ntstring(data, len, recv_game.game_name);
		copy_to_ntstring(data, len, recv_game.mission_title);
		copy_to_ntstring(data, len, recv_game.mission_name);
	
		num_active_udp_changed = 1;
		
		auto r = partial_range(Active_udp_games, num_active_udp_games);
		auto i = std::find_if(r.begin(), r.end(), [&recv_game](const UDP_netgame_info_lite &g) { return !d_stricmp(g.game_name.data(), recv_game.game_name.data()) && g.GameID == recv_game.GameID; });
		if (i == Active_udp_games.end())
		{
			return;
		}
		
		*i = std::move(recv_game);
#if defined(DXX_BUILD_DESCENT_II)
		// See if this is really a Hoard game
		// If so, adjust all the data accordingly
		if (HoardEquipped())
		{
			if (i->game_flag.hoard)
			{
				i->gamemode=NETGAME_HOARD;
				i->game_status=NETSTAT_PLAYING;
				
				if (i->game_flag.team_hoard)
					i->gamemode=NETGAME_TEAM_HOARD;
				if (i->game_flag.endlevel)
					i->game_status=NETSTAT_ENDLEVEL;
				if (i->game_flag.forming)
					i->game_status=NETSTAT_STARTING;
			}
		}
#endif
		if (i == r.end())
		{
			if (i->numconnected)
				num_active_udp_games++;
		}
		else if (!i->numconnected)
		{
			// Delete this game
			std::move(std::next(i), r.end(), i);
			num_active_udp_games--;
		}
	}
	else
	{
		Netgame.players[0].protocol.udp.addr = game_addr;

												len++; // skip UPID byte
		Netgame.protocol.udp.program_iver[0] = GET_INTEL_SHORT(&(data[len]));		len += 2;
		Netgame.protocol.udp.program_iver[1] = GET_INTEL_SHORT(&(data[len]));		len += 2;
		Netgame.protocol.udp.program_iver[2] = GET_INTEL_SHORT(&(data[len]));		len += 2;
		Netgame.protocol.udp.your_index = data[len]; ++len;
		range_for (auto &i, Netgame.players)
		{
			i.callsign.copy_lower(reinterpret_cast<const char *>(&data[len]), CALLSIGN_LEN);
			len += CALLSIGN_LEN+1;
			i.connected = data[len];				len++;
			i.rank = data[len];					len++;
		}
		Netgame.levelnum = GET_INTEL_INT(&(data[len]));					len += 4;
		Netgame.gamemode = data[len];							len++;
		Netgame.RefusePlayers = data[len];						len++;
		Netgame.difficulty = data[len];							len++;
		Netgame.game_status = data[len];						len++;
		Netgame.numplayers = data[len];							len++;
		Netgame.max_numplayers = data[len];						len++;
		Netgame.numconnected = data[len];						len++;
		packed_game_flags p;
		p.value = data[len];
		Netgame.game_flag = unpack_game_flags(&p);						len++;
		Netgame.team_vector = data[len];						len++;
		Netgame.AllowedItems = GET_INTEL_INT(&(data[len]));				len += 4;
#if defined(DXX_BUILD_DESCENT_I)
		Netgame.SpawnGrantedItems = data[len];		len += 1;
		Netgame.DuplicatePowerups.set_packed_field(data[len]);			len += 1;
#elif defined(DXX_BUILD_DESCENT_II)
		Netgame.SpawnGrantedItems = GET_INTEL_SHORT(&(data[len]));		len += 2;
		Netgame.DuplicatePowerups.set_packed_field(GET_INTEL_SHORT(&data[len])); len += 2;
		if (unlikely(map_granted_flags_to_laser_level(Netgame.SpawnGrantedItems) > MAX_SUPER_LASER_LEVEL))
			/* Bogus input - reject whole entry */
			Netgame.SpawnGrantedItems = 0;
		Netgame.Allow_marker_view = GET_INTEL_SHORT(&(data[len]));			len += 2;
		Netgame.AlwaysLighting = GET_INTEL_SHORT(&(data[len]));				len += 2;
#endif
		Netgame.ShowEnemyNames = GET_INTEL_SHORT(&(data[len]));				len += 2;
		Netgame.BrightPlayers = GET_INTEL_SHORT(&(data[len]));				len += 2;
		Netgame.InvulAppear = GET_INTEL_SHORT(&(data[len]));				len += 2;
		range_for (auto &i, Netgame.team_name)
		{
			i.copy(reinterpret_cast<const char *>(&data[len]), (CALLSIGN_LEN+1));
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
		Netgame.team_kills[0] = GET_INTEL_SHORT(&(data[len]));				len += 2;	
		Netgame.team_kills[1] = GET_INTEL_SHORT(&(data[len]));				len += 2;
		range_for (auto &i, Netgame.killed)
		{
			i = GET_INTEL_SHORT(&(data[len]));			len += 2;
		}
		range_for (auto &i, Netgame.player_kills)
		{
			i = GET_INTEL_SHORT(&(data[len]));		len += 2;
		}
		Netgame.KillGoal = GET_INTEL_INT(&(data[len]));					len += 4;
		Netgame.PlayTimeAllowed = GET_INTEL_INT(&(data[len]));				len += 4;
		Netgame.level_time = GET_INTEL_INT(&(data[len]));				len += 4;
		Netgame.control_invul_time = GET_INTEL_INT(&(data[len]));			len += 4;
		Netgame.monitor_vector = GET_INTEL_INT(&(data[len]));				len += 4;
		range_for (auto &i, Netgame.player_score)
		{
			i = GET_INTEL_INT(&(data[len]));			len += 4;
		}
		range_for (auto &i, Netgame.player_flags)
		{
			i = data[len];					len++;
		}
		Netgame.PacketsPerSec = GET_INTEL_SHORT(&(data[len]));				len += 2;
		Netgame.PacketLossPrevention = data[len];					len++;
		Netgame.NoFriendlyFire = data[len];						len++;
		copy_to_ntstring(data, len, Netgame.game_name);
		copy_to_ntstring(data, len, Netgame.mission_title);
		copy_to_ntstring(data, len, Netgame.mission_name);

		Netgame.protocol.udp.valid = 1; // This game is valid! YAY!
	}
}

static void net_udp_process_dump(ubyte *data, int, const _sockaddr &sender_addr)
{
	// Our request for join was denied.  Tell the user why.
	if (sender_addr != Netgame.players[0].protocol.udp.addr)
		return;

	switch (data[1])
	{
		case DUMP_PKTTIMEOUT:
		case DUMP_KICKED:
			if (Network_status==NETSTAT_PLAYING)
				multi_leave_game();
			if (Game_wind)
				window_set_visible(Game_wind, 0);
			if (data[1] == DUMP_PKTTIMEOUT)
				nm_messagebox(NULL, 1, TXT_OK, "You were removed from the game.\nYou failed receiving important\npackets. Sorry.");
			if (data[1] == DUMP_KICKED)
				nm_messagebox(NULL, 1, TXT_OK, "You were kicked by Host!");
			if (Game_wind)
				window_set_visible(Game_wind, 1);
			multi_quit_game = 1;
			game_leave_menus();
			multi_reset_stuff();
			break;
		default:
			if (data[1] > DUMP_LEVEL) // invalid dump... heh
				break;
			Network_status = NETSTAT_MENU; // stop us from sending before message
			nm_messagebox_str(NULL, TXT_OK, NET_DUMP_STRINGS(data[1]));
			Network_status = NETSTAT_MENU;
			multi_reset_stuff();
			break;
	}
}

static void net_udp_process_request(UDP_sequence_packet *their)
{
	// Player is ready to receieve a sync packet
	for (int i = 0; i < N_players; i++)
		if (their->player.protocol.udp.addr == Netgame.players[i].protocol.udp.addr && !d_stricmp(their->player.callsign, Netgame.players[i].callsign))
		{
			Players[i].connected = CONNECT_PLAYING;
			Netgame.players[i].LastPacketTime = timer_query();
			break;
		}
}

static void net_udp_process_packet(ubyte *data, const _sockaddr &sender_addr, int length )
{
	UDP_sequence_packet their{};

	switch (data[0])
	{
		case UPID_VERSION_DENY:
			if (multi_i_am_master() || length != UPID_VERSION_DENY_SIZE)
				break;
			net_udp_process_version_deny(data, sender_addr);
			break;
		case UPID_GAME_INFO_REQ:
		{
			int result = 0;
			static fix64 last_full_req_time = 0;
			if (!multi_i_am_master() || length != UPID_GAME_INFO_REQ_SIZE)
				break;
			if (timer_query() < last_full_req_time+(F1_0/2)) // answer 2 times per second max
				break;
			last_full_req_time = timer_query();
			result = net_udp_check_game_info_request(data, 0);
			if (result == -1)
				net_udp_send_version_deny(sender_addr);
			else if (result == 1)
				net_udp_send_game_info(sender_addr, &sender_addr, UPID_GAME_INFO);
			break;
		}
		case UPID_GAME_INFO:
			if (multi_i_am_master() || length > UPID_GAME_INFO_SIZE_MAX)
				break;
			net_udp_process_game_info(data, length, sender_addr, 0);
			break;
		case UPID_GAME_INFO_LITE_REQ:
		{
			static fix64 last_lite_req_time = 0;
			if (!multi_i_am_master() || length != UPID_GAME_INFO_LITE_REQ_SIZE)
				break;
			if (timer_query() < last_lite_req_time+(F1_0/8))// answer 8 times per second max
				break;
			last_lite_req_time = timer_query();
			if (net_udp_check_game_info_request(data, 1) == 1)
				net_udp_send_game_info(sender_addr, &sender_addr, UPID_GAME_INFO_LITE);
			break;
		}
		case UPID_GAME_INFO_LITE:
			if (multi_i_am_master() || length > UPID_GAME_INFO_LITE_SIZE_MAX)
				break;
			net_udp_process_game_info(data, length, sender_addr, 1);
			break;
		case UPID_DUMP:
			if (multi_i_am_master() || Netgame.players[0].protocol.udp.addr != sender_addr || length != UPID_DUMP_SIZE)
				break;
			if ((Network_status == NETSTAT_WAITING) || (Network_status == NETSTAT_PLAYING))
				net_udp_process_dump(data, length, sender_addr);
			break;
		case UPID_ADDPLAYER:
			if (multi_i_am_master() || Netgame.players[0].protocol.udp.addr != sender_addr || length != UPID_SEQUENCE_SIZE)
				break;
			net_udp_receive_sequence_packet(data, &their, sender_addr);
			net_udp_new_player(&their);
			break;
		case UPID_REQUEST:
			if (!multi_i_am_master() || length != UPID_SEQUENCE_SIZE)
				break;
			net_udp_receive_sequence_packet(data, &their, sender_addr);
			if (Network_status == NETSTAT_STARTING) 
			{
				// Someone wants to join our game!
				net_udp_add_player(&their);
			}
			else if (Network_status == NETSTAT_WAITING)
			{
				// Someone is ready to recieve a sync packet
				net_udp_process_request(&their);
			}
			else if (Network_status == NETSTAT_PLAYING)
			{
				// Someone wants to join a game in progress!
				if (Netgame.RefusePlayers)
					net_udp_do_refuse_stuff (&their);
				else
					net_udp_welcome_player(&their);
			}
			break;
		case UPID_QUIT_JOINING:
			if (!multi_i_am_master() || length != UPID_SEQUENCE_SIZE)
				break;
			net_udp_receive_sequence_packet(data, &their, sender_addr);
			if (Network_status == NETSTAT_STARTING)
				net_udp_remove_player( &their );
			else if ((Network_status == NETSTAT_PLAYING) && (Network_send_objects))
				net_udp_stop_resync( &their );
			break;
		case UPID_SYNC:
			if (multi_i_am_master() || length > UPID_GAME_INFO_SIZE_MAX || Network_status != NETSTAT_WAITING)
				break;
			net_udp_read_sync_packet(data, length, sender_addr);
			break;
		case UPID_OBJECT_DATA:
			if (multi_i_am_master() || length > UPID_MAX_SIZE || Network_status != NETSTAT_WAITING)
				break;
			net_udp_read_object_packet(data);
			break;
		case UPID_PING:
			if (multi_i_am_master() || length != UPID_PING_SIZE)
				break;
			net_udp_process_ping(data, sender_addr);
			break;
		case UPID_PONG:
			if (!multi_i_am_master() || length != UPID_PONG_SIZE)
				break;
			net_udp_process_pong(data, sender_addr);
			break;
		case UPID_ENDLEVEL_H:
			if ((!multi_i_am_master()) && ((Network_status == NETSTAT_ENDLEVEL) || (Network_status == NETSTAT_PLAYING)))
				net_udp_read_endlevel_packet(data, sender_addr);
			break;
		case UPID_ENDLEVEL_C:
			if ((multi_i_am_master()) && ((Network_status == NETSTAT_ENDLEVEL) || (Network_status == NETSTAT_PLAYING)))
				net_udp_read_endlevel_packet(data, sender_addr);
			break;
		case UPID_PDATA:
			net_udp_process_pdata( data, length, sender_addr );
			break;
		case UPID_MDATA_PNORM:
			net_udp_process_mdata( data, length, sender_addr, 0 );
			break;
		case UPID_MDATA_PNEEDACK:
			net_udp_process_mdata( data, length, sender_addr, 1 );
			break;
		case UPID_MDATA_ACK:
			net_udp_noloss_got_ack(data, length);
			break;
#ifdef USE_TRACKER
		case UPID_TRACKER_VERIFY:
			iTrackerVerified = 1;
			break;
		case UPID_TRACKER_INCGAME:
			udp_tracker_process_game( data, length );
			break;
#endif
		default:
			con_printf(CON_DEBUG, "unknown packet type received - type %i", data[0]);
			break;
	}
}

// Packet for end of level syncing
void net_udp_read_endlevel_packet(const uint8_t *data, const _sockaddr &sender_addr)
{
	int len = 0;
	ubyte tmpvar = 0;
	
	if (multi_i_am_master())
	{
		playernum_t pnum = data[1];

		if (Netgame.players[pnum].protocol.udp.addr != sender_addr)
			return;

		len += 2;

		if ((int)data[len] == CONNECT_DISCONNECTED)
			multi_disconnect_player(pnum);
		Players[pnum].connected = data[len];					len++;
		tmpvar = data[len];							len++;
		if ((Network_status != NETSTAT_PLAYING) && (Players[pnum].connected == CONNECT_PLAYING) && (tmpvar < Countdown_seconds_left))
			Countdown_seconds_left = tmpvar;
		Players[pnum].net_kills_total = GET_INTEL_SHORT(&(data[len]));		len += 2;
		Players[pnum].net_killed_total = GET_INTEL_SHORT(&(data[len]));		len += 2;

		range_for (auto &i, kill_matrix[pnum])
		{
			i = GET_INTEL_SHORT(&(data[len]));		len += 2;
		}
		if (Players[pnum].connected)
			Netgame.players[pnum].LastPacketTime = timer_query();
	}
	else
	{
		if (Netgame.players[0].protocol.udp.addr != sender_addr)
			return;

		len++;

		tmpvar = data[len];							len++;
		if ((Network_status != NETSTAT_PLAYING) && (tmpvar < Countdown_seconds_left))
			Countdown_seconds_left = tmpvar;

		for (playernum_t i = 0; i < MAX_PLAYERS; i++)
		{
			if (i == Player_num)
			{
				len += 5;
				continue;
			}

			if ((int)data[len] == CONNECT_DISCONNECTED)
				multi_disconnect_player(i);
			Players[i].connected = data[len];				len++;
			Players[i].net_kills_total = GET_INTEL_SHORT(&(data[len]));	len += 2;
			Players[i].net_killed_total = GET_INTEL_SHORT(&(data[len]));	len += 2;

			if (Players[i].connected)
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

	if (event.type != EVENT_WINDOW_DRAW)
		return 0;
	net_udp_listen();

	// Leave if Host disconnects
	if (Netgame.players[0].connected == CONNECT_DISCONNECTED)
		rval = -2;

	if (Network_status != NETSTAT_WAITING)	// Status changed to playing, exit the menu
		rval = -2;

	if (Network_status != NETSTAT_MENU && !Network_rejoined && (timer_query() > t1+F1_0*2))
	{
		// Poll time expired, re-send request
		
		t1 = timer_query();

		auto i = net_udp_send_request();
		if (i >= MAX_PLAYERS)
			rval = -2;
	}
	
	return rval;
}

struct start_poll_data
{
	int playercount;
};

static int net_udp_start_poll( newmenu *menu,const d_event &event, start_poll_data *spd)
{
	newmenu_item *menus = newmenu_get_items(menu);
	int nitems = newmenu_get_nitems(menu);
	int nm;

	if (event.type != EVENT_WINDOW_DRAW)
		return 0;
	Assert(Network_status == NETSTAT_STARTING);

	if (!menus[0].value)
		menus[0].value = 1;

	for (int i=1; i<nitems; i++ ) {
		
		if ( (i>= N_players) && (menus[i].value) ) {
			menus[i].value = 0;
		}
	}

	nm = 0;
	for (int i=0; i<nitems; i++ ) {
		if ( menus[i].value ) {
			nm++;
			if ( nm > N_players ) {
				menus[i].value = 0;
			}
		}
	}

	if ( nm > Netgame.max_numplayers ) {
		nm_messagebox( TXT_ERROR, 1, TXT_OK, "%s %d %s", TXT_SORRY_ONLY, Netgame.max_numplayers, TXT_NETPLAYERS_IN );
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
		if (PlayerCfg.NoRankings)	
			snprintf( menus[i].text, 45, "%d. %-20s", i+1, static_cast<const char *>(Netgame.players[i].callsign));
		else
			snprintf( menus[i].text, 45, "%d. %s%-20s", i+1, RankStrings[Netgame.players[i].rank],static_cast<const char *>(Netgame.players[i].callsign));
	}

	if (spd->playercount < Netgame.numplayers ) // A new player
	{
		digi_play_sample (SOUND_HUD_MESSAGE,F1_0);
		if (N_players <= Netgame.max_numplayers)
			menus[N_players-1].value = 1;
	} 
	else if ( spd->playercount > Netgame.numplayers ) // One got removed...
	{
		digi_play_sample (SOUND_HUD_KILL,F1_0);
  
		for (int i=0; i<N_players; i++ )
		{
			if (i < Netgame.max_numplayers)
				menus[i].value = 1;
			else
				menus[i].value = 0;
		}
		for (int i=N_players; i<spd->playercount; i++ )
		{
			sprintf( menus[i].text, "%d. ", i+1 );          // Clear out the deleted entries...
			menus[i].value = 0;
		}
	}
	spd->playercount = Netgame.numplayers;

	return 0;
}

#ifdef USE_TRACKER
#define DXX_UDP_MENU_TRACKER_OPTION(VERB)	\
	DXX_##VERB##_CHECK(tracker, opt_tracker, Netgame.Tracker)
#else
#define DXX_UDP_MENU_TRACKER_OPTION(VERB)
#endif

#if defined(DXX_BUILD_DESCENT_I)
#define D2X_UDP_MENU_OPTIONS(VERB)	\

#elif defined(DXX_BUILD_DESCENT_II)
#define D2X_UDP_MENU_OPTIONS(VERB)	\
	DXX_##VERB##_CHECK("Allow Marker camera views", opt_marker_view, Netgame.Allow_marker_view)	\
	DXX_##VERB##_CHECK("Indestructible lights", opt_light, Netgame.AlwaysLighting)	\

#endif

#define DXX_UDP_MENU_OPTIONS(VERB)	\
	DXX_##VERB##_SLIDER(TXT_DIFFICULTY, opt_difficulty, Netgame.difficulty, 0, (NDL-1))	\
	DXX_##VERB##_SCALE_SLIDER(srinvul, opt_cinvul, Netgame.control_invul_time, 0, 10, 5*F1_0*60)	\
	DXX_##VERB##_SLIDER(PlayText, opt_playtime, Netgame.PlayTimeAllowed, 0, 10)	\
	DXX_##VERB##_SLIDER(KillText, opt_killgoal, Netgame.KillGoal, 0, 20)	\
	DXX_##VERB##_CHECK(TXT_SHOW_ON_MAP, opt_show_on_map, Netgame.game_flag.show_on_map)	\
	D2X_UDP_MENU_OPTIONS(VERB)	\
	DXX_##VERB##_SLIDER(SpawnInvulnerableText, opt_start_invul, Netgame.InvulAppear, 0, 8)	\
	DXX_##VERB##_CHECK("Bright player ships", opt_bright, Netgame.BrightPlayers)	\
	DXX_##VERB##_CHECK("Show enemy names on HUD", opt_show_names, Netgame.ShowEnemyNames)	\
	DXX_##VERB##_CHECK("No friendly fire (Team, Coop)", opt_ffire, Netgame.NoFriendlyFire)	\
	DXX_##VERB##_MENU("Set Objects allowed...", opt_setpower)	\
	DXX_##VERB##_MENU("Set Objects granted at spawn...", opt_setgrant)	\
	DXX_##VERB##_MENU("Duplicate powerups at level start", opt_set_powerup_duplicates)	\
	DXX_##VERB##_TEXT("Packets per second (" DXX_STRINGIZE_PPS(MIN_PPS) " - " DXX_STRINGIZE_PPS(MAX_PPS) ")", opt_label_pps)	\
	DXX_##VERB##_INPUT(packstring, opt_packets)	\
	DXX_##VERB##_TEXT("Network port", opt_label_port)	\
	DXX_##VERB##_INPUT(portstring, opt_port)	\
	DXX_UDP_MENU_TRACKER_OPTION(VERB)

#define DXX_STRINGIZE_PPS2(X)	#X
#define DXX_STRINGIZE_PPS(X)	DXX_STRINGIZE_PPS2(X)

enum {
	DXX_UDP_MENU_OPTIONS(ENUM)
};

static void net_udp_set_power (void)
{
	newmenu_item m[MULTI_ALLOW_POWERUP_MAX];
	for (int i = 0; i < MULTI_ALLOW_POWERUP_MAX; i++)
	{
		nm_set_item_checkbox(m[i], multi_allow_powerup_text[i], (Netgame.AllowedItems >> i) & 1);
	}

	newmenu_do1( NULL, "Objects to allow", MULTI_ALLOW_POWERUP_MAX, m, unused_newmenu_subfunction, unused_newmenu_userdata, 0 );

	Netgame.AllowedItems &= ~NETFLAG_DOPOWERUP;
	for (int i = 0; i < MULTI_ALLOW_POWERUP_MAX; i++)
		if (m[i].value)
			Netgame.AllowedItems |= (1 << i);
}

#if defined(DXX_BUILD_DESCENT_I)
#define D2X_GRANT_POWERUP_MENU(VERB)
#define D2X_DUPLICATE_POWERUP_MENU(VERB)
#elif defined(DXX_BUILD_DESCENT_II)
#define D2X_GRANT_POWERUP_MENU(VERB)	\
	DXX_##VERB##_CHECK(NETFLAG_LABEL_GAUSS, opt_gauss, menu_bit_wrapper(flags, NETGRANT_GAUSS))	\
	DXX_##VERB##_CHECK(NETFLAG_LABEL_HELIX, opt_helix, menu_bit_wrapper(flags, NETGRANT_HELIX))	\
	DXX_##VERB##_CHECK(NETFLAG_LABEL_PHOENIX, opt_phoenix, menu_bit_wrapper(flags, NETGRANT_PHOENIX))	\
	DXX_##VERB##_CHECK(NETFLAG_LABEL_OMEGA, opt_omega, menu_bit_wrapper(flags, NETGRANT_OMEGA))	\
	DXX_##VERB##_CHECK(NETFLAG_LABEL_AFTERBURNER, opt_afterburner, menu_bit_wrapper(flags, NETGRANT_AFTERBURNER))	\
	DXX_##VERB##_CHECK(NETFLAG_LABEL_AMMORACK, opt_ammo_rack, menu_bit_wrapper(flags, NETGRANT_AMMORACK))	\
	DXX_##VERB##_CHECK(NETFLAG_LABEL_CONVERTER, opt_converter, menu_bit_wrapper(flags, NETGRANT_CONVERTER))	\
	DXX_##VERB##_CHECK(NETFLAG_LABEL_HEADLIGHT, opt_headlight, menu_bit_wrapper(flags, NETGRANT_HEADLIGHT))	\

#define D2X_DUPLICATE_POWERUP_MENU(VERB)	\
	DXX_##VERB##_NUMBER("duplicate accessories", opt_accessory, accessory, 0, (1 << packed_netduplicate_items::accessory_width) - 1)	\

#endif

#define DXX_GRANT_POWERUP_MENU(VERB)	\
	DXX_##VERB##_NUMBER("Laser level", opt_laser_level, menu_number_bias_wrapper(laser_level, 1), LASER_LEVEL_1 + 1, DXX_MAXIMUM_LASER_LEVEL + 1)	\
	DXX_##VERB##_CHECK(NETFLAG_LABEL_QUAD, opt_quad_lasers, menu_bit_wrapper(flags, NETGRANT_QUAD))	\
	DXX_##VERB##_CHECK(NETFLAG_LABEL_VULCAN, opt_vulcan, menu_bit_wrapper(flags, NETGRANT_VULCAN))	\
	DXX_##VERB##_CHECK(NETFLAG_LABEL_SPREAD, opt_spreadfire, menu_bit_wrapper(flags, NETGRANT_SPREAD))	\
	DXX_##VERB##_CHECK(NETFLAG_LABEL_PLASMA, opt_plasma, menu_bit_wrapper(flags, NETGRANT_PLASMA))	\
	DXX_##VERB##_CHECK(NETFLAG_LABEL_FUSION, opt_fusion, menu_bit_wrapper(flags, NETGRANT_FUSION))	\
	D2X_GRANT_POWERUP_MENU(VERB)

#define DXX_DUPLICATE_POWERUP_MENU(VERB)	\
	DXX_##VERB##_NUMBER("duplicate primaries", opt_primary, primary, 0, (1 << packed_netduplicate_items::primary_width) - 1)	\
	DXX_##VERB##_NUMBER("duplicate secondaries", opt_secondary, secondary, 0, (1 << packed_netduplicate_items::secondary_width) - 1)	\
	D2X_DUPLICATE_POWERUP_MENU(VERB)

#define FORMAT_KILL_GOAL_STRING(BUF)	\
	snprintf(BUF, 80, "Kill Goal: %3d kills", Netgame.KillGoal*5);

#define FORMAT_SPAWN_INVULNERABLE_STRING(BUF)	\
	snprintf(BUF, 50, "Spawn invulnerability: %1.1f sec", static_cast<float>(Netgame.InvulAppear) / 2);

namespace {

class grant_powerup_menu_items
{
public:
	enum
	{
		DXX_GRANT_POWERUP_MENU(ENUM)
	};
	array<newmenu_item, DXX_GRANT_POWERUP_MENU(COUNT)> m;
	grant_powerup_menu_items(const unsigned laser_level, const packed_spawn_granted_items p)
	{
		auto &flags = p.mask;
		DXX_GRANT_POWERUP_MENU(ADD);
	}
	void read(packed_spawn_granted_items &p) const
	{
		unsigned laser_level, flags = 0;
		DXX_GRANT_POWERUP_MENU(READ);
		p.mask = laser_level | flags;
	}
};

class duplicate_powerup_menu_items
{
public:
	enum
	{
		DXX_DUPLICATE_POWERUP_MENU(ENUM)
	};
	array<newmenu_item, DXX_DUPLICATE_POWERUP_MENU(COUNT)> m;
	duplicate_powerup_menu_items(const packed_netduplicate_items items)
	{
		const auto primary = items.get_primary_count();
		const auto secondary = items.get_secondary_count();
#if defined(DXX_BUILD_DESCENT_II)
		const auto accessory = items.get_accessory_count();
#endif
		DXX_DUPLICATE_POWERUP_MENU(ADD);
	}
	void read(packed_netduplicate_items &items) const
	{
		unsigned primary, secondary;
#if defined(DXX_BUILD_DESCENT_II)
		unsigned accessory;
#endif
		DXX_DUPLICATE_POWERUP_MENU(READ);
		items.set_primary_count(primary);
		items.set_secondary_count(secondary);
#if defined(DXX_BUILD_DESCENT_II)
		items.set_accessory_count(accessory);
#endif
	}
};

}

static void net_udp_set_grant_power()
{
	const auto SpawnGrantedItems = Netgame.SpawnGrantedItems;
	grant_powerup_menu_items menu{map_granted_flags_to_laser_level(SpawnGrantedItems), SpawnGrantedItems};
	newmenu_do(nullptr, "Powerups granted at player spawn", menu.m, unused_newmenu_subfunction, unused_newmenu_userdata);
	menu.read(Netgame.SpawnGrantedItems);
}

static void net_udp_set_duplicate_powerups()
{
	duplicate_powerup_menu_items menu{Netgame.DuplicatePowerups};
	newmenu_do(nullptr, "Duplicate powerups at level start", menu.m, unused_newmenu_subfunction, unused_newmenu_userdata);
	menu.read(Netgame.DuplicatePowerups);
}

static void net_udp_more_game_options ()
{
	int i;
	char PlayText[80],KillText[80],srinvul[50],packstring[3];
	char SpawnInvulnerableText[50];
	char portstring[6];
	newmenu_item m[DXX_UDP_MENU_OPTIONS(COUNT)];

	snprintf(packstring,sizeof(packstring),"%d",Netgame.PacketsPerSec);
	snprintf(portstring,sizeof(portstring),"%hu",UDP_MyPort);
	snprintf(srinvul, sizeof(srinvul), "%s: %d %s", TXT_REACTOR_LIFE, Netgame.control_invul_time/F1_0/60, TXT_MINUTES_ABBREV );
	snprintf(PlayText, sizeof(PlayText), "Max time: %d %s", Netgame.PlayTimeAllowed*5, TXT_MINUTES_ABBREV );
	FORMAT_SPAWN_INVULNERABLE_STRING(SpawnInvulnerableText);
	FORMAT_KILL_GOAL_STRING(KillText);
#ifdef USE_TRACKER
	char tracker[52];
#endif

	DXX_UDP_MENU_OPTIONS(ADD);

#ifdef USE_TRACKER
	const auto &tracker_addr = GameArg.MplTrackerAddr;
	if (tracker_addr.empty())
		nm_set_item_text(m[opt_tracker], "Tracker use disabled by -no-tracker");
	else
		snprintf(tracker, sizeof(tracker), "Track this game on\n%s:%u", tracker_addr.c_str(), GameArg.MplTrackerPort);
#endif

	for (;;)
	{
		i = newmenu_do(nullptr, "Advanced netgame options", sizeof(m) / sizeof(m[0]), m, net_udp_more_options_handler, unused_newmenu_userdata);
		switch (i)
		{
			case opt_setpower:
				net_udp_set_power();
				continue;
			case opt_setgrant:
				net_udp_set_grant_power();
				continue;
			case opt_set_powerup_duplicates:
				net_udp_set_duplicate_powerups();
				continue;
			default:
				break;
		}
		break;
	}

	DXX_UDP_MENU_OPTIONS(READ);

	Netgame.PacketsPerSec=atoi(packstring);
	
	if (Netgame.PacketsPerSec>MAX_PPS)
	{
		Netgame.PacketsPerSec=MAX_PPS;
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "Packet value out of range\nSetting value to %i",MAX_PPS);
	}

	if (Netgame.PacketsPerSec<MIN_PPS)
	{
		Netgame.PacketsPerSec=MIN_PPS;
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "Packet value out of range\nSetting value to %i", MIN_PPS);
	}
	convert_text_portstring(portstring, UDP_MyPort, false);
	Difficulty_level = Netgame.difficulty;
}

static int net_udp_more_options_handler( newmenu *menu,const d_event &event, const unused_newmenu_userdata_t *)
{
	newmenu_item *menus = newmenu_get_items(menu);
	switch (event.type)
	{
		case EVENT_NEWMENU_CHANGED:
		{
			auto &citem = static_cast<const d_change_event &>(event).citem;
			if (citem == opt_cinvul)
				sprintf( menus[opt_cinvul].text, "%s: %d %s", TXT_REACTOR_LIFE, menus[opt_cinvul].value*5, TXT_MINUTES_ABBREV );
			else if (citem == opt_playtime)
			{
				if (Game_mode & GM_MULTI_COOP)
				{
					nm_messagebox ("Sorry",1,TXT_OK,"You can't change those for coop!");
					menus[opt_playtime].value=0;
					return 0;
				}
				
				Netgame.PlayTimeAllowed=menus[opt_playtime].value;
				sprintf( menus[opt_playtime].text, "Max Time: %d %s", Netgame.PlayTimeAllowed*5, TXT_MINUTES_ABBREV );
			}
			else if (citem == opt_killgoal)
			{
				if (Game_mode & GM_MULTI_COOP)
				{
					nm_messagebox ("Sorry",1,TXT_OK,"You can't change those for coop!");
					menus[opt_killgoal].value=0;
					return 0;
				}
				
				Netgame.KillGoal=menus[opt_killgoal].value;
				FORMAT_KILL_GOAL_STRING(menus[opt_killgoal].text);
			}
			else if (citem == opt_start_invul)
			{
				Netgame.InvulAppear = menus[opt_start_invul].value;
				FORMAT_SPAWN_INVULNERABLE_STRING(menus[opt_start_invul].text);
			}
			break;
		}
		default:
			break;
	}
	return 0;
}

struct param_opt
{
	int start_game, name, level, mode, mode_end, moreopts;
	int closed, refuse, maxnet, anarchy, team_anarchy, robot_anarchy, coop, bounty;
#if defined(DXX_BUILD_DESCENT_II)
	int capture, hoard, team_hoard;
#endif
};

static int net_udp_game_param_handler( newmenu *menu,const d_event &event, param_opt *opt )
{
	newmenu_item *menus = newmenu_get_items(menu);
	switch (event.type)
	{
		case EVENT_NEWMENU_CHANGED:
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
				if (menus[opt->maxnet].value>2) 
				{
					menus[opt->maxnet].value=2;
				}
				
				if (menus[opt->maxnet].max_value>2)
				{
					menus[opt->maxnet].max_value=2;
				}
				sprintf( menus[opt->maxnet].text, "Maximum players: %d", menus[opt->maxnet].value+2 );
				Netgame.max_numplayers = menus[opt->maxnet].value+2;
				
				Netgame.game_flag.show_on_map = 1;

				if (Netgame.PlayTimeAllowed || Netgame.KillGoal)
				{
					Netgame.PlayTimeAllowed=0;
					Netgame.KillGoal=0;
				}

			}
			else // if !Coop game
			{
				if (menus[opt->maxnet].max_value<6)
				{
					menus[opt->maxnet].value=6;
					menus[opt->maxnet].max_value=6;
					sprintf( menus[opt->maxnet].text, "Maximum players: %d", menus[opt->maxnet].value+2 );
					Netgame.max_numplayers = menus[opt->maxnet].value+2;
				}
			}
			
			if (citem == opt->level)
			{
				char *slevel = menus[opt->level].text;
#if defined(DXX_BUILD_DESCENT_I)
				if (tolower(static_cast<unsigned>(*slevel)) == 's')
					Netgame.levelnum = -atoi(slevel+1);
				else
#endif
					Netgame.levelnum = atoi(slevel);
			}
			
			if (citem == opt->maxnet)
			{
				sprintf( menus[opt->maxnet].text, "Maximum players: %d", menus[opt->maxnet].value+2 );
				Netgame.max_numplayers = menus[opt->maxnet].value+2;
			}

			if ((citem >= opt->mode) && (citem <= opt->mode_end))
			{
				if ( menus[opt->anarchy].value )
					Netgame.gamemode = NETGAME_ANARCHY;
				
				else if (menus[opt->team_anarchy].value) {
					Netgame.gamemode = NETGAME_TEAM_ANARCHY;
				}
#if defined(DXX_BUILD_DESCENT_II)
				else if (menus[opt->capture].value)
					Netgame.gamemode = NETGAME_CAPTURE_FLAG;
				else if (HoardEquipped() && menus[opt->hoard].value)
					Netgame.gamemode = NETGAME_HOARD;
				else if (HoardEquipped() && menus[opt->team_hoard].value)
					Netgame.gamemode = NETGAME_TEAM_HOARD;
#endif
				else if( menus[opt->bounty].value )
					Netgame.gamemode = NETGAME_BOUNTY;
#if defined(DXX_BUILD_DESCENT_II)
		 		else if (ANARCHY_ONLY_MISSION) {
					int i = 0;
		 			nm_messagebox(NULL, 1, TXT_OK, TXT_ANARCHY_ONLY_MISSION);
					for (i = opt->mode; i <= opt->mode_end; i++)
						menus[i].value = 0;
					menus[opt->anarchy].value = 1;
		 			return 0;
		 		}
#endif
				else if ( menus[opt->robot_anarchy].value ) 
					Netgame.gamemode = NETGAME_ROBOT_ANARCHY;
				else if ( menus[opt->coop].value ) 
					Netgame.gamemode = NETGAME_COOPERATIVE;
				else Int3(); // Invalid mode -- see Rob
			}

			Netgame.game_flag.closed = menus[opt->closed].value;
			Netgame.RefusePlayers=menus[opt->refuse].value;
			break;
		}
		case EVENT_NEWMENU_SELECTED:
		{
			auto &citem = static_cast<const d_select_event &>(event).citem;
#if defined(DXX_BUILD_DESCENT_I)
			if ((Netgame.levelnum < Last_secret_level) || (Netgame.levelnum > Last_level) || (Netgame.levelnum == 0))
#elif defined(DXX_BUILD_DESCENT_II)
			if ((Netgame.levelnum < 1) || (Netgame.levelnum > Last_level))
#endif
			{
				char *slevel = menus[opt->level].text;
				nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_LEVEL_OUT_RANGE );
				sprintf(slevel, "1");
				return 1;
			}

			if (citem==opt->moreopts)
			{
				if ( menus[opt->coop].value )
					Game_mode=GM_MULTI_COOP;
				net_udp_more_game_options();
				Game_mode=0;
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

int net_udp_setup_game()
{
	int i;
	int optnum;
	param_opt opt;
	newmenu_item m[22];
	char slevel[5];
	char level_text[32];
	char srmaxnet[50];

	net_udp_init();

	multi_new_game();

	change_playernum_to(0);

	{
		auto self = &Players[Player_num];
		range_for (auto &i, Players)
			if (&i != self)
				i.callsign.fill(0);
	}

	Netgame.max_numplayers = MAX_PLAYERS;
	Netgame.KillGoal=0;
	Netgame.PlayTimeAllowed=0;
#if defined(DXX_BUILD_DESCENT_I)
	Netgame.RefusePlayers=0;
#elif defined(DXX_BUILD_DESCENT_II)
	Netgame.Allow_marker_view=1;
#endif
	Netgame.difficulty=PlayerCfg.DefaultDifficulty;
	Netgame.PacketsPerSec=10;
	snprintf(Netgame.game_name.data(), Netgame.game_name.size(), "%s%s", static_cast<const char *>(Players[Player_num].callsign), TXT_S_GAME );
	reset_UDP_MyPort();
	Netgame.BrightPlayers = 1;
	Netgame.InvulAppear = 4;
	Netgame.AllowedItems = NETFLAG_DOPOWERUP;
	Netgame.PacketLossPrevention = 1;
	Netgame.NoFriendlyFire = 0;

#ifdef USE_TRACKER
	Netgame.Tracker = 1;
#endif

	read_netgame_profile(&Netgame);

	if (Netgame.gamemode == NETGAME_COOPERATIVE) // did we restore Coop as default? then fix max players right now!
		Netgame.max_numplayers = 4;
#if defined(DXX_BUILD_DESCENT_II)
	if (!HoardEquipped() && (Netgame.gamemode == NETGAME_HOARD || Netgame.gamemode == NETGAME_TEAM_HOARD)) // did we restore a hoard mode but don't have hoard installed right now? then fall back to anarchy!
		Netgame.gamemode = NETGAME_ANARCHY;
#endif

	Netgame.mission_name.copy_if(Current_mission_filename, Netgame.mission_name.size());
	Netgame.mission_title = Current_mission_longname;

	sprintf( slevel, "1" ); Netgame.levelnum = 1;

	optnum = 0;
	opt.start_game=optnum;
	nm_set_item_menu(  m[optnum], "Start Game"); optnum++;
	nm_set_item_text(m[optnum], TXT_DESCRIPTION); optnum++;

	opt.name = optnum;
	nm_set_item_input(m[optnum], Netgame.game_name); optnum++;

#if defined(DXX_BUILD_DESCENT_I)
#define DXX_SECRET_LEVEL_FORMAT_STRING	", S1-S%d)"
#define DXX_SECRET_LEVEL_FORMAT_ARGUMENT	, -Last_secret_level
#elif defined(DXX_BUILD_DESCENT_II)
#define DXX_SECRET_LEVEL_FORMAT_STRING
#define DXX_SECRET_LEVEL_FORMAT_ARGUMENT
#endif
	snprintf(level_text, sizeof(level_text), "%s (1-%d)" DXX_SECRET_LEVEL_FORMAT_STRING, TXT_LEVEL_, Last_level DXX_SECRET_LEVEL_FORMAT_ARGUMENT);

	Assert(strlen(level_text) < 32);

	nm_set_item_text(m[optnum], level_text); optnum++;

	opt.level = optnum;
	nm_set_item_input(m[optnum], slevel); optnum++;
	nm_set_item_text(m[optnum], TXT_OPTIONS); optnum++;

	opt.mode = optnum;
	nm_set_item_radio(m[optnum], TXT_ANARCHY,(Netgame.gamemode == NETGAME_ANARCHY),0); opt.anarchy=optnum; optnum++;
	nm_set_item_radio(m[optnum], TXT_TEAM_ANARCHY,(Netgame.gamemode == NETGAME_TEAM_ANARCHY),0); opt.team_anarchy=optnum; optnum++;
	nm_set_item_radio(m[optnum], TXT_ANARCHY_W_ROBOTS,(Netgame.gamemode == NETGAME_ROBOT_ANARCHY),0); opt.robot_anarchy=optnum; optnum++;
	nm_set_item_radio(m[optnum], TXT_COOPERATIVE,(Netgame.gamemode == NETGAME_COOPERATIVE),0); opt.coop=optnum; optnum++;
#if defined(DXX_BUILD_DESCENT_II)
	nm_set_item_radio(m[optnum], "Capture the flag",(Netgame.gamemode == NETGAME_CAPTURE_FLAG),0); opt.capture=optnum; optnum++;

	if (HoardEquipped())
	{
		nm_set_item_radio(m[optnum], "Hoard",(Netgame.gamemode == NETGAME_HOARD),0); opt.hoard=optnum; optnum++;
		nm_set_item_radio(m[optnum], "Team Hoard",(Netgame.gamemode == NETGAME_TEAM_HOARD),0); opt.team_hoard=optnum; optnum++;
	}
	else
	{
		opt.hoard = opt.team_hoard = 0; // NOTE: Make sure if you use these, use them in connection with HoardEquipped() only!
	}
#endif
	nm_set_item_radio(m[optnum], "Bounty", ( Netgame.gamemode & NETGAME_BOUNTY ), 0); opt.mode_end=opt.bounty=optnum; optnum++;

	nm_set_item_text(m[optnum], ""); optnum++;

	nm_set_item_radio(m[optnum], "Open game",(!Netgame.RefusePlayers && !Netgame.game_flag.closed),1); optnum++;
	opt.closed = optnum;
	nm_set_item_radio(m[optnum], TXT_CLOSED_GAME,Netgame.game_flag.closed,1); optnum++;
	opt.refuse = optnum;
	nm_set_item_radio(m[optnum], "Restricted Game              ",Netgame.RefusePlayers,1); optnum++;

	opt.maxnet = optnum;
	sprintf( srmaxnet, "Maximum players: %d", Netgame.max_numplayers);
	nm_set_item_slider(m[optnum], srmaxnet,Netgame.max_numplayers-2,0,Netgame.max_numplayers-2); optnum++;
	
	opt.moreopts=optnum;
	nm_set_item_menu(  m[optnum], "Advanced Options"); optnum++;

	Assert(optnum <= 20);

	i = newmenu_do1( NULL, TXT_NETGAME_SETUP, optnum, m, net_udp_game_param_handler, &opt, opt.start_game );

	if (i < 0)
		net_udp_close();

	write_netgame_profile(&Netgame);
#ifdef USE_TRACKER
	/* Force off _after_ writing profile, so that command line does not
	 * change ngp file.
	 */
	if (GameArg.MplTrackerAddr.empty())
		Netgame.Tracker = 0;
#endif

	return i >= 0;
}

void
static net_udp_set_game_mode(int gamemode)
{
	Show_kill_list = 1;

	if ( gamemode == NETGAME_ANARCHY )
		Game_mode = GM_NETWORK;
	else if ( gamemode == NETGAME_ROBOT_ANARCHY )
		Game_mode = GM_NETWORK | GM_MULTI_ROBOTS;
	else if ( gamemode == NETGAME_COOPERATIVE ) 
		Game_mode = GM_NETWORK | GM_MULTI_COOP | GM_MULTI_ROBOTS;
#if defined(DXX_BUILD_DESCENT_II)
	else if (gamemode == NETGAME_CAPTURE_FLAG)
		{
		 Game_mode = GM_NETWORK | GM_TEAM | GM_CAPTURE;
		 Show_kill_list=3;
		}

	else if (HoardEquipped() && gamemode == NETGAME_HOARD)
		 Game_mode = GM_NETWORK | GM_HOARD;
	else if (HoardEquipped() && gamemode == NETGAME_TEAM_HOARD)
		 {
		  Game_mode = GM_NETWORK | GM_HOARD | GM_TEAM;
 		  Show_kill_list=3;
		 }
#endif
	else if( gamemode == NETGAME_BOUNTY )
		Game_mode = GM_NETWORK | GM_BOUNTY;
	else if ( gamemode == NETGAME_TEAM_ANARCHY )
	{
		Game_mode = GM_NETWORK | GM_TEAM;
		Show_kill_list = 3;
	}
	else
		Int3();
}

void net_udp_read_sync_packet(const uint8_t * data, uint_fast32_t data_len, const _sockaddr &sender_addr)
{
	if (data)
	{
		net_udp_process_game_info(data, data_len, sender_addr, 0);
	}

	N_players = Netgame.numplayers;
	Difficulty_level = Netgame.difficulty;
	Network_status = Netgame.game_status;

	if (Netgame.segments_checksum != my_segments_checksum)
	{
		Network_status = NETSTAT_MENU;
		net_udp_close();
		nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_NETLEVEL_NMATCH);
		throw multi::level_checksum_mismatch();
	}

	// Discover my player number

	callsign_t temp_callsign = Players[Player_num].callsign;
	
	Player_num = MULTI_PNUM_UNDEF;

	range_for (auto &i, Players)
	{
		i.net_kills_total = 0;
		i.net_killed_total = 0;
	}

	for (int i=0; i<N_players; i++ ) {
		if (i == Netgame.protocol.udp.your_index && Netgame.players[i].callsign == temp_callsign)
		{
			if (Player_num!=MULTI_PNUM_UNDEF) {
				Int3(); // Hey, we've found ourselves twice
				Network_status = NETSTAT_MENU;
				return; 
			}
			change_playernum_to(i);
		}
		Players[i].callsign = Netgame.players[i].callsign;
		Players[i].connected = Netgame.players[i].connected;
		Players[i].net_kills_total = Netgame.player_kills[i];
		Players[i].net_killed_total = Netgame.killed[i];
		if ((Network_rejoined) || (i != Player_num))
			Players[i].score = Netgame.player_score[i];
	}
	kill_matrix = Netgame.kills;

	if (Player_num >= MAX_PLAYERS)
	{
		Network_status = NETSTAT_MENU;
		throw multi::local_player_not_playing();
	}

	if (Network_rejoined)
		for (int i=0; i<N_players;i++)
			Players[i].net_killed_total = Netgame.killed[i];

#if defined(DXX_BUILD_DESCENT_I)
	PlayerCfg.NetlifeKills -= Players[Player_num].net_kills_total;
	PlayerCfg.NetlifeKilled -= Players[Player_num].net_killed_total;
#endif

	if (Network_rejoined)
	{
		net_udp_process_monitor_vector(Netgame.monitor_vector);
		Players[Player_num].time_level = Netgame.level_time;
	}

	team_kills = Netgame.team_kills;
	Players[Player_num].connected = CONNECT_PLAYING;
	Netgame.players[Player_num].connected = CONNECT_PLAYING;
	Netgame.players[Player_num].rank=GetMyNetRanking();

	if (!Network_rejoined)
	{
		for (int i=0; i<NumNetPlayerPositions; i++)
		{
			const auto o = vobjptridx(Players[i].objnum);
			const auto &p = Player_init[Netgame.locations[i]];
			o->pos = p.pos;
			o->orient = p.orient;
			obj_relink(o, p.segnum);
		}
	}

	Objects[Players[Player_num].objnum].type = OBJ_PLAYER;

	Network_status = NETSTAT_PLAYING;
	multi_sort_kill_list();
}

static int net_udp_send_sync(void)
{
	int np;

	// Check if there are enough starting positions
	if (NumNetPlayerPositions < Netgame.max_numplayers)
	{
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "Not enough start positions\n(set %d got %d)\nNetgame aborted", Netgame.max_numplayers, NumNetPlayerPositions);
		// Tell everyone we're bailing
		Netgame.numplayers = 0;
		for (int i=1; i<N_players; i++)
		{
			if (Players[i].connected == CONNECT_DISCONNECTED)
				continue;
			const auto &addr = Netgame.players[i].protocol.udp.addr;
			net_udp_dump_player(addr, DUMP_ABORTED);
			net_udp_send_game_info(addr, &addr, UPID_GAME_INFO);
		}
		net_udp_broadcast_game_info(UPID_GAME_INFO_LITE);
		return -1;
	}

	// Randomize their starting locations...
	d_srand( (fix)timer_query() );
	for (unsigned i=0; i<NumNetPlayerPositions; i++ ) 
	{
		if (Players[i].connected)
			Players[i].connected = CONNECT_PLAYING; // Get rid of endlevel connect statuses

		if (Game_mode & GM_MULTI_COOP)
			Netgame.locations[i] = i;
		else {
			do 
			{
				np = d_rand() % NumNetPlayerPositions;
				range_for (auto &j, partial_range(Netgame.locations, i)) 
				{
					if (j==np)   
					{
						np =-1;
						break;
					}
				}
			} while (np<0);
			// np is a location that is not used anywhere else..
			Netgame.locations[i]=np;
		}
	}

	// Push current data into the sync packet

	net_udp_update_netgame();
	Netgame.game_status = NETSTAT_PLAYING;
	Netgame.segments_checksum = my_segments_checksum;

	for (int i=0; i<N_players; i++ )
	{
		if ((!Players[i].connected) || (i == Player_num))
			continue;
		const auto &addr = Netgame.players[i].protocol.udp.addr;
		net_udp_send_game_info(addr, &addr, UPID_SYNC);
	}

	net_udp_read_sync_packet(NULL, 0, Netgame.players[0].protocol.udp.addr); // Read it myself, as if I had sent it
	return 0;
}

int
static net_udp_select_teams(void)
{
	newmenu_item m[MAX_PLAYERS+4];
	int choice, opt, opt_team_b;
	ubyte team_vector = 0;
	int pnums[MAX_PLAYERS+2];

	// One-time initialization

	for (int i = N_players/2; i < N_players; i++) // Put first half of players on team A
	{
		team_vector |= (1 << i);
	}

	array<callsign_t, 2> team_names;
	team_names[0].copy(TXT_BLUE, ~0ul);
	team_names[1].copy(TXT_RED, ~0ul);

	// Here comes da menu
menu:
	nm_set_item_input(m[0], team_names[0].buffer());

	opt = 1;
	for (int i = 0; i < N_players; i++)
	{
		if (!(team_vector & (1 << i)))
		{
			nm_set_item_menu( m[opt], Netgame.players[i].callsign); pnums[opt] = i; opt++;
		}
	}
	opt_team_b = opt;
	nm_set_item_input(m[opt], team_names[1].buffer());
	opt++;
	for (int i = 0; i < N_players; i++)
	{
		if (team_vector & (1 << i))
		{
			nm_set_item_menu( m[opt], Netgame.players[i].callsign); pnums[opt] = i; opt++;
		}
	}
	nm_set_item_text(m[opt], ""); opt++;
	nm_set_item_menu( m[opt], TXT_ACCEPT); opt++;

	Assert(opt <= MAX_PLAYERS+4);
	
	choice = newmenu_do(NULL, TXT_TEAM_SELECTION, opt, m, unused_newmenu_subfunction, unused_newmenu_userdata);

	if (choice == opt-1)
	{
#if 0 // no need to wait for other players
		if ((opt-2-opt_team_b < 2) || (opt_team_b == 1)) 
		{
			nm_messagebox(NULL, 1, TXT_OK, TXT_TEAM_MUST_ONE);
			#ifdef RELEASE
			goto menu;
			#endif
		}
#endif
		Netgame.team_vector = team_vector;
		Netgame.team_name[0] = team_names[0];
		Netgame.team_name[1] = team_names[1];
		return 1;
	}

	else if ((choice > 0) && (choice < opt_team_b)) {
		team_vector |= (1 << pnums[choice]);
	}
	else if ((choice > opt_team_b) && (choice < opt-2)) {
		team_vector &= ~(1 << pnums[choice]);
	}
	else if (choice == -1)
		return 0;
	goto menu;
}

int
static net_udp_select_players(void)
{
	int j;
	newmenu_item m[MAX_PLAYERS+4];
	char text[MAX_PLAYERS+4][45];
	char title[50];
	unsigned save_nplayers;              //how may people would like to join

	net_udp_add_player( &UDP_Seq );
	std::unique_ptr<start_poll_data> spd(new start_poll_data{});
	if (!spd)
		return 0;
	spd->playercount=1;
		
	for (int i=0; i< MAX_PLAYERS+4; i++ ) {
		sprintf( text[i], "%d.  %-20s", i+1, "" );
		nm_set_item_checkbox(m[i], text[i], 0);
	}

	m[0].value = 1;                         // Assume server will play...

	if (PlayerCfg.NoRankings)
		snprintf( text[0], sizeof(text[0]), "%d. %-20s", 1, static_cast<const char *>(Players[Player_num].callsign));
	else
		snprintf( text[0], sizeof(text[0]), "%d. %s%-20s", 1, RankStrings[Netgame.players[Player_num].rank],static_cast<const char *>(Players[Player_num].callsign));

	sprintf( title, "%s %d %s", TXT_TEAM_SELECT, Netgame.max_numplayers, TXT_TEAM_PRESS_ENTER );

GetPlayersAgain:
#ifdef USE_TRACKER
	if( Netgame.Tracker )
		udp_tracker_register();
#endif

	j=newmenu_do1( NULL, title, MAX_PLAYERS+4, m, net_udp_start_poll, spd.get(), 1 );

	save_nplayers = N_players;

	if (j<0) 
	{
		// Aborted!
		// Dump all players and go back to menu mode
#ifdef USE_TRACKER
		if( Netgame.Tracker )
			udp_tracker_unregister();
#endif

abort:
		// Tell everyone we're bailing
		Netgame.numplayers = 0;
		for (int i=1; i<save_nplayers; i++) {
			if (Players[i].connected == CONNECT_DISCONNECTED)
				continue;
			const auto &addr = Netgame.players[i].protocol.udp.addr;
			net_udp_dump_player(addr, DUMP_ABORTED);
			net_udp_send_game_info(addr, &addr, UPID_GAME_INFO);
		}
		net_udp_broadcast_game_info(UPID_GAME_INFO_LITE);
		Netgame.numplayers = save_nplayers;

		Network_status = NETSTAT_MENU;
		return(0);
	}
	// Count number of players chosen

	N_players = 0;
	range_for (auto &i, partial_range(m, save_nplayers))
	{
		if (i.value)
			N_players++;
	}
	
	if ( N_players > Netgame.max_numplayers) {
		nm_messagebox( TXT_ERROR, 1, TXT_OK, "%s %d %s", TXT_SORRY_ONLY, Netgame.max_numplayers, TXT_NETPLAYERS_IN );
		N_players = save_nplayers;
		goto GetPlayersAgain;
	}

// Let host join without Client available. Let's see if our players like that
#if 0 //def RELEASE
	if ( N_players < 2 )    {
		nm_messagebox( TXT_ERROR, 1, TXT_OK, TXT_TEAM_ATLEAST_TWO );
		N_players = save_nplayers;
		goto GetPlayersAgain;
	}
#endif

#if 0 //def RELEASE
	if ( (Netgame.gamemode == NETGAME_TEAM_ANARCHY || Netgame.gamemode == NETGAME_CAPTURE_FLAG || Netgame.gamemode == NETGAME_TEAM_HOARD) && (N_players < 2) )
	{
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "You must select at least two\nplayers to start a team game" );
		N_players = save_nplayers;
		goto GetPlayersAgain;
	}
#endif

	// Remove players that aren't marked.
	N_players = 0;
	for (int i=0; i<save_nplayers; i++ )
	{
		if (m[i].value) 
		{
			if (i > N_players)
			{
				Netgame.players[N_players].callsign = Netgame.players[i].callsign;
				Netgame.players[N_players].rank=Netgame.players[i].rank;
				ClipRank (&Netgame.players[N_players].rank);
			}
			Players[N_players].connected = CONNECT_PLAYING;
			N_players++;
		}
		else
		{
			net_udp_dump_player(Netgame.players[i].protocol.udp.addr, DUMP_DORK);
		}
	}

	range_for (auto &i, partial_range(Netgame.players, N_players, static_cast<unsigned>(Netgame.players.size())))
	{
		i.callsign.fill(0);
		i.rank=0;
	}

#if defined(DXX_BUILD_DESCENT_I)
	if (Netgame.gamemode == NETGAME_TEAM_ANARCHY)
#elif defined(DXX_BUILD_DESCENT_II)
	if (Netgame.gamemode == NETGAME_TEAM_ANARCHY ||
	    Netgame.gamemode == NETGAME_CAPTURE_FLAG ||
		 Netgame.gamemode == NETGAME_TEAM_HOARD)
#endif
		 if (!net_udp_select_teams())
			goto abort;
	return(1);
}

static int net_udp_start_game(void)
{
	int i;

	i = udp_open_socket(UDP_Socket[0], UDP_MyPort);

	if (i != 0)
		return 0;
	
	if (UDP_MyPort != UDP_PORT_DEFAULT)
		i = udp_open_socket(UDP_Socket[1], UDP_PORT_DEFAULT); // Default port open for Broadcasts

	if (i != 0)
		return 0;

	// prepare broadcast address to announce our game
	udp_init_broadcast_addresses();
	d_srand( (fix)timer_query() );
	Netgame.protocol.udp.GameID=d_rand();

	N_players = 0;
	Netgame.game_status = NETSTAT_STARTING;
	Netgame.numplayers = 0;

	Network_status = NETSTAT_STARTING;

	net_udp_set_game_mode(Netgame.gamemode);

	Netgame.protocol.udp.your_index = 0; // I am Host. I need to know that y'know? For syncing later.
	
	if(net_udp_select_players())
	{
		StartNewLevel(Netgame.levelnum);
	}
	else
	{
		Game_mode = GM_GAME_OVER;
		return 0;	// see if we want to tweak the game we setup
	}
	net_udp_broadcast_game_info(UPID_GAME_INFO_LITE); // game started. broadcast our current status to everyone who wants to know

	return 1;	// don't keep params menu or mission listbox (may want to join a game next time)
}

static int net_udp_wait_for_sync(void)
{
	char text[60];
	int choice=0;
	
	Network_status = NETSTAT_WAITING;

	array<newmenu_item, 2> m{{
		nm_item_text(text),
		nm_item_text(TXT_NET_LEAVE),
	}};
	auto i = net_udp_send_request();

	if (i >= MAX_PLAYERS)
		return(-1);

	snprintf(text, sizeof(text), "%s\n'%s' %s", TXT_NET_WAITING, static_cast<const char *>(Netgame.players[i].callsign), TXT_NET_TO_ENTER );

	while (choice > -1)
	{
		timer_update();
		choice=newmenu_do( NULL, TXT_WAIT, m, net_udp_sync_poll, unused_newmenu_userdata );
	}

	if (Network_status != NETSTAT_PLAYING)
	{
		UDP_sequence_packet me{};
		me.type = UPID_QUIT_JOINING;
		me.player.callsign = Players[Player_num].callsign;
		net_udp_send_sequence_packet(me, Netgame.players[0].protocol.udp.addr);
		N_players = 0;
		Game_mode = GM_GAME_OVER;
		return(-1);     // they cancelled
	}
	return(0);
}

static int net_udp_request_poll( newmenu *,const d_event &event, const unused_newmenu_userdata_t *)
{
	// Polling loop for waiting-for-requests menu
	int num_ready = 0;

	if (event.type != EVENT_WINDOW_DRAW)
		return 0;
	net_udp_listen();
	net_udp_timeout_check(timer_query());

	range_for (auto &i, partial_range(Players, N_players))
	{
		if ((i.connected == CONNECT_PLAYING) || (i.connected == CONNECT_DISCONNECTED))
			num_ready++;
	}

	if (num_ready == N_players) // All players have checked in or are disconnected
	{
		return -2;
	}
	
	return 0;
}

static int net_udp_wait_for_requests(void)
{
	// Wait for other players to load the level before we send the sync
	int choice;
	array<newmenu_item, 1> m{{
		nm_item_text(TXT_NET_LEAVE),
	}};
	Network_status = NETSTAT_WAITING;
	net_udp_flush();

	Players[Player_num].connected = CONNECT_PLAYING;

menu:
	choice = newmenu_do(NULL, TXT_WAIT, m, net_udp_request_poll, unused_newmenu_userdata);

	if (choice == -1)
	{
		// User aborted
		choice = nm_messagebox(NULL, 3, TXT_YES, TXT_NO, TXT_START_NOWAIT, TXT_QUITTING_NOW);
		if (choice == 2) {
			N_players = 1;
			return 0;
		}
		if (choice != 0)
			goto menu;
		
		// User confirmed abort
		
		for (int i=0; i < N_players; i++) {
			if ((Players[i].connected != CONNECT_DISCONNECTED) && (i != Player_num)) {
				net_udp_dump_player(Netgame.players[i].protocol.udp.addr, DUMP_ABORTED);
			}
		}
		return -1;
	}
	else if (choice != -2)
		goto menu;

	return 0;
}

/* Do required syncing after each level, before starting new one */
int
net_udp_level_sync(void)
{
	int result = 0;

	UDP_MData = {};
	net_udp_noloss_init_mdata_queue();

	net_udp_flush(); // Flush any old packets

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

	multi_powcap_count_powerups_in_mine();

	if (result)
	{
		Players[Player_num].connected = CONNECT_DISCONNECTED;
		net_udp_send_endlevel_packet();
		if (Game_wind)
			window_close(Game_wind);
		show_menus();
		net_udp_close();
		return -1;
	}
	return(0);
}

int net_udp_do_join_game()
{

	if (Netgame.game_status == NETSTAT_ENDLEVEL)
	{
		nm_messagebox(TXT_SORRY, 1, TXT_OK, TXT_NET_GAME_BETWEEN2);
		return 0;
	}

	// Check for valid mission name
	if (!load_mission_by_name(Netgame.mission_name))
	{
		nm_messagebox(NULL, 1, TXT_OK, TXT_MISSION_NOT_FOUND);
		return 0;
	}

#if defined(DXX_BUILD_DESCENT_II)
	if (is_D2_OEM)
	{
		if (Netgame.levelnum>8)
		{
			nm_messagebox(NULL, 1, TXT_OK, "This OEM version only supports\nthe first 8 levels!");
			return 0;
		}
	}

	if (is_MAC_SHARE)
	{
		if (Netgame.levelnum > 4)
		{
			nm_messagebox(NULL, 1, TXT_OK, "This SHAREWARE version only supports\nthe first 4 levels!");
			return 0;
		}
	}

	if ( !HoardEquipped() && (Netgame.gamemode == NETGAME_HOARD || Netgame.gamemode == NETGAME_TEAM_HOARD) )
	{
		nm_messagebox(TXT_SORRY, 1, TXT_OK, "HOARD(.ham) not installed. You can't join.");
		return 0;
	}

	Network_status = NETSTAT_BROWSING; // We are looking at a game menu
#endif

	if (!net_udp_can_join_netgame(&Netgame))
	{
		if (Netgame.numplayers == Netgame.max_numplayers)
			nm_messagebox(TXT_SORRY, 1, TXT_OK, TXT_GAME_FULL);
		else
			nm_messagebox(TXT_SORRY, 1, TXT_OK, TXT_IN_PROGRESS);
		return 0;
	}

	// Choice is valid, prepare to join in
	Difficulty_level = Netgame.difficulty;
	change_playernum_to(1);

	net_udp_set_game_mode(Netgame.gamemode);

	StartNewLevel(Netgame.levelnum);
	
	return 1;     // look ma, we're in a game!!!
}

void net_udp_leave_game()
{
	int nsave;

	net_udp_do_frame(1, 1);

	if ((multi_i_am_master()))
	{
		while (Network_sending_extras>1 && Player_joining_extras!=-1)
		{
			timer_update();
			net_udp_send_extras();
		}

		Netgame.numplayers = 0;
		nsave=N_players;
		N_players=0;
		for (int i=1; i<nsave; i++ )
		{
			if (Players[i].connected == CONNECT_DISCONNECTED)
				continue;
			const auto &addr = Netgame.players[i].protocol.udp.addr;
			net_udp_send_game_info(addr, &addr, UPID_GAME_INFO);
		}
		net_udp_broadcast_game_info(UPID_GAME_INFO_LITE);
		N_players=nsave;
#ifdef USE_TRACKER
		if( Netgame.Tracker )
			udp_tracker_unregister();
#endif
	}

	Players[Player_num].connected = CONNECT_DISCONNECTED;
	change_playernum_to(0);
#if defined(DXX_BUILD_DESCENT_II)
	write_player_file();
#endif

	net_udp_flush();
	net_udp_close();
}

void net_udp_flush()
{
	ubyte packet[UPID_MAX_SIZE];
	struct _sockaddr sender_addr; 

	if (UDP_Socket[0])
		while (udp_receive_packet(UDP_Socket[0], packet, UPID_MAX_SIZE, &sender_addr) > 0);

	if (UDP_Socket[1])
		while (udp_receive_packet(UDP_Socket[1], packet, UPID_MAX_SIZE, &sender_addr) > 0);
}

void net_udp_listen()
{
	int size;
	ubyte packet[UPID_MAX_SIZE];
	struct _sockaddr sender_addr;

	if (UDP_Socket[0])
	{
		size = udp_receive_packet(UDP_Socket[0], packet, UPID_MAX_SIZE, &sender_addr );
		while ( size > 0 )	{
			net_udp_process_packet( packet, sender_addr, size );
			size = udp_receive_packet(UDP_Socket[0], packet, UPID_MAX_SIZE, &sender_addr );
		}
	}

	if (UDP_Socket[1])
	{
		size = udp_receive_packet(UDP_Socket[1], packet, UPID_MAX_SIZE, &sender_addr );
		while ( size > 0 )	{
			net_udp_process_packet( packet, sender_addr, size );
			size = udp_receive_packet(UDP_Socket[1], packet, UPID_MAX_SIZE, &sender_addr );
		}
	}
	
#ifdef USE_TRACKER
	if (UDP_Socket[2])
	{
		size = udp_receive_packet(UDP_Socket[2], packet, UPID_MAX_SIZE, &sender_addr );
		while ( size > 0 )	{
			net_udp_process_packet( packet, sender_addr, size );
			size = udp_receive_packet(UDP_Socket[2], packet, UPID_MAX_SIZE, &sender_addr );
		}
	}
#endif
}

void net_udp_send_data(const ubyte * ptr, int len, int priority )
{
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

	if (priority)
		net_udp_send_mdata((priority==2)?1:0, timer_query());
}

void net_udp_timeout_check(fix64 time)
{
	static fix64 last_timeout_time = 0;
	if (time>=last_timeout_time+F1_0)
	{
		// Check for player timeouts
		for (playernum_t i = 0; i < N_players; i++)
		{
			if ((i != Player_num) && (Players[i].connected != CONNECT_DISCONNECTED))
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

void net_udp_do_frame(int force, int listen)
{
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
	}
	
	if (force || (time >= (last_mdata_time+(F1_0/10))))
	{
		last_mdata_time = time;
		multi_send_robot_frame(0);
		net_udp_send_mdata(0, time);
	}

	net_udp_noloss_process_queue(time);

	if (VerifyPlayerJoined!=-1 && time >= last_resync_time+F1_0)
	{
		last_resync_time = time;
		net_udp_resend_sync_due_to_packet_loss(); // This will resend to UDP_sync_player
	}

	if ((time>=last_endlevel_time+F1_0) && Control_center_destroyed)
	{
		last_endlevel_time = time;
		net_udp_send_endlevel_packet();
	}

	// broadcast lite_info every 10 seconds
	if (multi_i_am_master() && time>=last_bcast_time+(F1_0*10))
	{
		last_bcast_time = time;
		net_udp_broadcast_game_info(UPID_GAME_INFO_LITE);
	}

#ifdef USE_TRACKER
	// If we use the tracker, tell the tracker about us every 10 seconds
	if( Netgame.Tracker )
	{
		// Static variable... the last time we sent to the tracker
		static fix64 iLastQuery = 0;
		static int iAttempts = 0;
		fix64 iNow = timer_query();
		
		// Set the last query to now if we must
		if( iLastQuery == 0 )
			iLastQuery = iNow;
		
		// Test it
		if( iTrackerVerified == 0 && iNow >= iLastQuery + ( F1_0 * 3 ) )
		{
			// Update it
			iLastQuery = iNow;
			iAttempts++;
		}
		
		// Have we had all our attempts?
		if( iTrackerVerified == 0 && iAttempts > 3 )
		{
			// Turn off tracker
			Netgame.Tracker = 0;
			
			// Reset the static variables for next time
			iLastQuery = 0;
			iAttempts = 0;
			
			// Warn
			nm_messagebox( TXT_WARNING, 1, TXT_OK, "No response from tracker!\nPossible causes:\nTracker is down\nYour port is likely not open!\n\nTracker: %s:%hu\nGame port: %hu", GameArg.MplTrackerAddr.c_str(), GameArg.MplTrackerPort, UDP_MyPort );
		}
	}
#endif

	net_udp_ping_frame(time);

	if (listen)
	{
		net_udp_timeout_check(time);
		net_udp_listen();
		if (Network_send_objects)
			net_udp_send_objects();
		if (Network_sending_extras && VerifyPlayerJoined==-1)
			net_udp_send_extras();
	}

	udp_traffic_stat();
}

/* CODE FOR PACKET LOSS PREVENTION - START */
/* This code tries to make sure that packets with opcode UPID_MDATA_PNEEDACK aren't lost and sent and received in order. */
/*
 * Adds a packet to our queue. Should be called when an IMPORTANT mdata packet is created.
 * player_ack is an array which should contain 0 for each player that needs to send an ACK signal.
 */
static void net_udp_noloss_add_queue_pkt(fix64 time, const ubyte *data, ushort data_size, ubyte pnum, ubyte player_ack[MAX_PLAYERS])
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
					net_udp_dump_player(Netgame.players[i].protocol.udp.addr, DUMP_PKTTIMEOUT);
			std::move(std::next(UDP_mdata_queue.begin()), UDP_mdata_queue.end(), UDP_mdata_queue.begin());
			UDP_mdata_queue[UDP_MDATA_STOR_QUEUE_SIZE - 1] = {};
			UDP_mdata_queue_highest--;
		}
		else // I am just a client. I gotta go.
		{
			Netgame.PacketLossPrevention = 0; // Disable PLP - otherwise we get stuck in an infinite loop here. NOTE: We could as well clean the whole queue to continue protect our disconnect signal bit it's not that important - we just wanna leave.
			if (Network_status==NETSTAT_PLAYING)
				multi_leave_game();
			if (Game_wind)
				window_set_visible(Game_wind, 0);
			nm_messagebox(NULL, 1, TXT_OK, "You left the game. You failed\nsending important packets.\nSorry.");
			if (Game_wind)
				window_set_visible(Game_wind, 1);
			multi_quit_game = 1;
			game_leave_menus();
			multi_reset_stuff();
		}
		Assert(UDP_mdata_queue_highest == (UDP_MDATA_STOR_QUEUE_SIZE - 1));
	}

	con_printf(CON_VERBOSE, "P#%u: Adding MData pkt_num [%i,%i,%i,%i,%i,%i,%i,%i], type %i from P#%i to MData store list", Player_num, UDP_mdata_trace[0].pkt_num_tosend,UDP_mdata_trace[1].pkt_num_tosend,UDP_mdata_trace[2].pkt_num_tosend,UDP_mdata_trace[3].pkt_num_tosend,UDP_mdata_trace[4].pkt_num_tosend,UDP_mdata_trace[5].pkt_num_tosend,UDP_mdata_trace[6].pkt_num_tosend,UDP_mdata_trace[7].pkt_num_tosend, data[0], pnum);
	UDP_mdata_queue[UDP_mdata_queue_highest].used = 1;
	UDP_mdata_queue[UDP_mdata_queue_highest].pkt_initial_timestamp = time;
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (i == Player_num || player_ack[i] || Players[i].connected == CONNECT_DISCONNECTED) // if player me, is not playing or does not require an ACK, do not add timestamp or increment pkt_num
			continue;
		
		UDP_mdata_queue[UDP_mdata_queue_highest].pkt_timestamp[i] = time;
		UDP_mdata_queue[UDP_mdata_queue_highest].pkt_num[i] = UDP_mdata_trace[i].pkt_num_tosend;
		UDP_mdata_trace[i].pkt_num_tosend++;
		if (UDP_mdata_trace[i].pkt_num_tosend > UDP_MDATA_PKT_NUM_MAX)
			UDP_mdata_trace[i].pkt_num_tosend = UDP_MDATA_PKT_NUM_MIN;
	}
	UDP_mdata_queue[UDP_mdata_queue_highest].Player_num = pnum;
	memcpy( &UDP_mdata_queue[UDP_mdata_queue_highest].player_ack, player_ack, sizeof(ubyte)*MAX_PLAYERS); 
	memcpy( &UDP_mdata_queue[UDP_mdata_queue_highest].data, data, sizeof(char)*data_size );
	UDP_mdata_queue[UDP_mdata_queue_highest].data_size = data_size;
	UDP_mdata_queue_highest++;
}

/*
 * We have received a MDATA packet. Send ACK response to sender!
 * Make sure this packet has the expected packet number so we get them all in order. If not, reject it and await further packets.
 * Also check in our UDP_mdata_trace list, if we got this packet already. If yes, return 0 so do not process it!
 */
static int net_udp_noloss_validate_mdata(uint32_t pkt_num, ubyte sender_pnum, const _sockaddr &sender_addr)
{
	ubyte buf[7], pkt_sender_pnum = sender_pnum;
	int len = 0;

	// If we are a client, we get all our packets from the host.
	if (!multi_i_am_master())
		sender_pnum = 0;

	// Check if this comes from a valid IP
	if (sender_addr != Netgame.players[sender_pnum].protocol.udp.addr)
		return 0;

        // Prepare the ACK (but do not send, yet)
        memset(&buf,0,sizeof(buf));
        buf[len] = UPID_MDATA_ACK;											len++;
        buf[len] = Player_num;												len++;
        buf[len] = pkt_sender_pnum;											len++;
        PUT_INTEL_INT(buf + len, pkt_num);										len += 4;

        // Make sure this is the packet we are expecting!
        if (UDP_mdata_trace[sender_pnum].pkt_num_torecv != pkt_num)
        {
                range_for (auto &i, partial_range(UDP_mdata_trace[sender_pnum].pkt_num, (uint32_t)UDP_MDATA_STOR_QUEUE_SIZE))
                {
                        if (pkt_num == i) // We got this packet already - need to REsend ACK
                        {
                                con_printf(CON_VERBOSE, "P#%u: Resending MData ACK for pkt %i we already got by pnum %i",Player_num, pkt_num, sender_pnum);
                                dxx_sendto(sender_addr, UDP_Socket[0], buf, len, 0);
                                return 0;
                        }
                }
                con_printf(CON_VERBOSE, "P#%u: Rejecting MData pkt %i - expected %i by pnum %i",Player_num, pkt_num, UDP_mdata_trace[sender_pnum].pkt_num_torecv, sender_pnum);
                return 0; // Not the right packet and we haven't gotten it, yet either. So bail out and wait for the right one.
        }

	con_printf(CON_VERBOSE, "P#%u: Sending MData ACK for pkt %i by pnum %i",Player_num, pkt_num, sender_pnum);
	dxx_sendto(sender_addr, UDP_Socket[0], buf, len, 0);

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
void net_udp_noloss_got_ack(const uint8_t *data, uint_fast32_t data_len)
{
	int len = 0;
	uint32_t pkt_num = 0;
	ubyte sender_pnum = 0, dest_pnum = 0;

	if (data_len != 7)
		return;

															len++;
	sender_pnum = data[len];											len++;
	dest_pnum = data[len];												len++;
	pkt_num = GET_INTEL_INT(&data[len]);										len += 4;

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
		for (int plc = 0; plc < MAX_PLAYERS; plc++)
		{
			// If player is not playing anymore, we can remove him from list. Also remove *me* (even if that should have been done already). Also make sure Clients do not send to anyone else than Host
			if ((Players[plc].connected != CONNECT_PLAYING || plc == Player_num) || (!multi_i_am_master() && plc > 0))
				UDP_mdata_queue[queuec].player_ack[plc] = 1;

			if (!UDP_mdata_queue[queuec].player_ack[plc])
			{
				// Resend if enough time has passed.
				if (UDP_mdata_queue[queuec].pkt_timestamp[plc] + (F1_0/4) <= time)
				{
					ubyte buf[sizeof(UDP_mdata_info)];
					int len = 0;
					
					con_printf(CON_VERBOSE, "P#%u: Resending pkt_num %i from pnum %i to pnum %i",Player_num, UDP_mdata_queue[queuec].pkt_num[plc], UDP_mdata_queue[queuec].Player_num, plc);
					
					UDP_mdata_queue[queuec].pkt_timestamp[plc] = time;
					memset(&buf, 0, sizeof(UDP_mdata_info));
					
					// Prepare the packet and send it
					buf[len] = UPID_MDATA_PNEEDACK;													len++;
					buf[len] = UDP_mdata_queue[queuec].Player_num;								len++;
					PUT_INTEL_INT(buf + len, UDP_mdata_queue[queuec].pkt_num[plc]);					len += 4;
					memcpy(&buf[len], UDP_mdata_queue[queuec].data.data(), sizeof(char)*UDP_mdata_queue[queuec].data_size);
																								len += UDP_mdata_queue[queuec].data_size;
					dxx_sendto(Netgame.players[plc].protocol.udp.addr, UDP_Socket[0], buf, len, 0);
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
							net_udp_dump_player(Netgame.players[plc].protocol.udp.addr, DUMP_PKTTIMEOUT);
				}
				else // We are client, so we gotta go.
				{
					Netgame.PacketLossPrevention = 0; // Disable PLP - otherwise we get stuck in an infinite loop here. NOTE: We could as well clean the whole queue to continue protect our disconnect signal bit it's not that important - we just wanna leave.
					if (Network_status==NETSTAT_PLAYING)
						multi_leave_game();
					if (Game_wind)
						window_set_visible(Game_wind, 0);
					nm_messagebox(NULL, 1, TXT_OK, "You left the game. You failed\nsending important packets.\nSorry.");
					if (Game_wind)
						window_set_visible(Game_wind, 1);
					multi_quit_game = 1;
					game_leave_menus();
					multi_reset_stuff();
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
	while (!UDP_mdata_queue[0].used && UDP_mdata_queue_highest > 0)
	{
		std::move(std::next(UDP_mdata_queue.begin()), UDP_mdata_queue.end(), UDP_mdata_queue.begin());
		UDP_mdata_queue[UDP_MDATA_STOR_QUEUE_SIZE - 1] = {};
		UDP_mdata_queue_highest--;
	}
}
/* CODE FOR PACKET LOSS PREVENTION - END */

void net_udp_send_mdata_direct(const ubyte *data, int data_len, int pnum, int needack)
{
	ubyte buf[sizeof(UDP_mdata_info)];
	ubyte pack[MAX_PLAYERS];
	int len = 0;
	
	if (!(Game_mode&GM_NETWORK) || !UDP_Socket[0])
		return;

	if (!(data_len > 0))
		return;

	if (!multi_i_am_master() && pnum != 0)
		Error("Client sent direct data to non-Host in net_udp_send_mdata_direct()!\n");

	if (!Netgame.PacketLossPrevention)
		needack = 0;

	memset(&buf, 0, sizeof(UDP_mdata_info));
	memset(&pack, 1, sizeof(ubyte)*MAX_PLAYERS);

	pack[pnum] = 0;

	if (needack)
		buf[len] = UPID_MDATA_PNEEDACK;
	else
		buf[len] = UPID_MDATA_PNORM;
														len++;
	buf[len] = Player_num;											len++;
	if (needack)
	{
		PUT_INTEL_INT(buf + len, UDP_mdata_trace[pnum].pkt_num_tosend);					len += 4;
	}
	memcpy(&buf[len], data, sizeof(char)*data_len);								len += data_len;

	dxx_sendto(Netgame.players[pnum].protocol.udp.addr, UDP_Socket[0], buf, len, 0);

	if (needack)
		net_udp_noloss_add_queue_pkt(timer_query(), data, data_len, Player_num, pack);
}

void net_udp_send_mdata(int needack, fix64 time)
{
	ubyte buf[sizeof(UDP_mdata_info)];
	ubyte pack[MAX_PLAYERS];
	int len = 0;
	
	if (!(Game_mode&GM_NETWORK) || !UDP_Socket[0])
		return;

	if (!(UDP_MData.mbuf_size > 0))
		return;

	if (!Netgame.PacketLossPrevention)
		needack = 0;

	memset(&buf, 0, sizeof(UDP_mdata_info));
	memset(&pack, 1, sizeof(ubyte)*MAX_PLAYERS);

	if (needack)
		buf[len] = UPID_MDATA_PNEEDACK;
	else
		buf[len] = UPID_MDATA_PNORM;
														len++;
	buf[len] = Player_num;											len++;
	if (needack)												len += 4; // we place the pkt_num later since it changes per player
	memcpy(&buf[len], UDP_MData.mbuf.data(), sizeof(char)*UDP_MData.mbuf_size);
	len += UDP_MData.mbuf_size;

	if (multi_i_am_master())
	{
		for (int i = 1; i < MAX_PLAYERS; i++)
		{
			if (Players[i].connected == CONNECT_PLAYING)
			{
				if (needack) // assign pkt_num
					PUT_INTEL_INT(buf + 2, UDP_mdata_trace[i].pkt_num_tosend);
				dxx_sendto(Netgame.players[i].protocol.udp.addr, UDP_Socket[0], buf, len, 0);
				pack[i] = 0;
			}
		}
	}
	else
	{
		if (needack) // assign pkt_num
			PUT_INTEL_INT(buf + 2, UDP_mdata_trace[0].pkt_num_tosend);
		dxx_sendto(Netgame.players[0].protocol.udp.addr, UDP_Socket[0], buf, len, 0);
		pack[0] = 0;
	}
	
	if (needack)
		net_udp_noloss_add_queue_pkt(time, UDP_MData.mbuf.data(), UDP_MData.mbuf_size, Player_num, pack);

	// Clear UDP_MData except pkt_num. That one must not be deleted so we can clearly keep track of important packets.
	UDP_MData.type = 0;
	UDP_MData.Player_num = 0;
	UDP_MData.mbuf_size = 0;
	UDP_MData.mbuf = {};
}

void net_udp_process_mdata(uint8_t *data, uint_fast32_t data_len, const _sockaddr &sender_addr, int needack)
{
	int pnum = data[1], dataoffset = (needack?6:2);

	// Check if packet might be bogus
	if ((pnum < 0) || (data_len > sizeof(UDP_mdata_info)))
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

	// send this to everyone else (if master)
	if (multi_i_am_master())
	{
		ubyte pack[MAX_PLAYERS];
		memset(&pack, 1, sizeof(ubyte)*MAX_PLAYERS);
		
		for (int i = 1; i < MAX_PLAYERS; i++)
		{
			if ((i != pnum) && Players[i].connected == CONNECT_PLAYING)
			{
				if (needack)
				{
					pack[i] = 0;
					PUT_INTEL_INT(data + 2, UDP_mdata_trace[i].pkt_num_tosend);
				}
				dxx_sendto(Netgame.players[i].protocol.udp.addr, UDP_Socket[0], data, data_len, 0);
				
			}
		}

		if (needack)
		{
			net_udp_noloss_add_queue_pkt(timer_query(), data+dataoffset, data_len-dataoffset, pnum, pack);
		}
	}

	// Check if we are in correct state to process the packet
	if (!((Network_status == NETSTAT_PLAYING)||(Network_status == NETSTAT_ENDLEVEL) || Network_status==NETSTAT_WAITING))
		return;

	// Process

	multi_process_bigdata(pnum, data+dataoffset, data_len-dataoffset );
}

void net_udp_send_pdata()
{
	ubyte buf[sizeof(UDP_frame_info)];
	int len = 0;

	if (!(Game_mode&GM_NETWORK) || !UDP_Socket[0])
		return;
	if (Players[Player_num].connected != CONNECT_PLAYING)
		return;
	if ( !( Network_status == NETSTAT_PLAYING || Network_status == NETSTAT_ENDLEVEL ) )
		return;

	memset(&buf, 0, sizeof(UDP_frame_info));
	
	buf[len] = UPID_PDATA;									len++;
	buf[len] = Player_num;									len++;
	buf[len] = Players[Player_num].connected;						len++;

	quaternionpos qpp{};
	create_quaternionpos(&qpp, &Objects[Players[Player_num].objnum], 0);
	PUT_INTEL_SHORT(buf+len, qpp.orient.w);							len += 2;
	PUT_INTEL_SHORT(buf+len, qpp.orient.x);							len += 2;
	PUT_INTEL_SHORT(buf+len, qpp.orient.y);							len += 2;
	PUT_INTEL_SHORT(buf+len, qpp.orient.z);							len += 2;
	PUT_INTEL_INT(buf+len, qpp.pos.x);							len += 4;
	PUT_INTEL_INT(buf+len, qpp.pos.y);							len += 4;
	PUT_INTEL_INT(buf+len, qpp.pos.z);							len += 4;
	PUT_INTEL_SHORT(buf+len, qpp.segment);							len += 2;
	PUT_INTEL_INT(buf+len, qpp.vel.x);							len += 4;
	PUT_INTEL_INT(buf+len, qpp.vel.y);							len += 4;
	PUT_INTEL_INT(buf+len, qpp.vel.z);							len += 4;
	PUT_INTEL_INT(buf+len, qpp.rotvel.x);							len += 4;
	PUT_INTEL_INT(buf+len, qpp.rotvel.y);							len += 4;
	PUT_INTEL_INT(buf+len, qpp.rotvel.z);							len += 4; // 46 + 3 = 49

	if (multi_i_am_master())
	{
		for (int i = 1; i < MAX_PLAYERS; i++)
			if (Players[i].connected != CONNECT_DISCONNECTED)
				dxx_sendto(Netgame.players[i].protocol.udp.addr, UDP_Socket[0], buf, len, 0);
	}
	else
	{
		dxx_sendto(Netgame.players[0].protocol.udp.addr, UDP_Socket[0], buf, len, 0);
	}
}

void net_udp_process_pdata(const uint8_t *data, uint_fast32_t data_len, const _sockaddr &sender_addr)
{
	UDP_frame_info pd;
	int len = 0;

	if ( !( Game_mode & GM_NETWORK && ( Network_status == NETSTAT_PLAYING || Network_status == NETSTAT_ENDLEVEL ) ) )
		return;

	len++;

	pd = {};
	
	if (data_len > sizeof(UDP_frame_info))
		return;
	if (data_len != UPID_PDATA_SIZE)
		return;

	if (sender_addr != Netgame.players[((multi_i_am_master())?(data[len]):(0))].protocol.udp.addr)
		return;

	pd.Player_num = data[len];								len++;
	pd.connected = data[len];								len++;
	pd.qpp.orient.w = GET_INTEL_SHORT(&data[len]);					len += 2;
	pd.qpp.orient.x = GET_INTEL_SHORT(&data[len]);					len += 2;
	pd.qpp.orient.y = GET_INTEL_SHORT(&data[len]);					len += 2;
	pd.qpp.orient.z = GET_INTEL_SHORT(&data[len]);					len += 2;
	pd.qpp.pos.x = GET_INTEL_INT(&data[len]);						len += 4;
	pd.qpp.pos.y = GET_INTEL_INT(&data[len]);						len += 4;
	pd.qpp.pos.z = GET_INTEL_INT(&data[len]);						len += 4;
	pd.qpp.segment = GET_INTEL_SHORT(&data[len]);					len += 2;
	pd.qpp.vel.x = GET_INTEL_INT(&data[len]);						len += 4;
	pd.qpp.vel.y = GET_INTEL_INT(&data[len]);						len += 4;
	pd.qpp.vel.z = GET_INTEL_INT(&data[len]);						len += 4;
	pd.qpp.rotvel.x = GET_INTEL_INT(&data[len]);					len += 4;
	pd.qpp.rotvel.y = GET_INTEL_INT(&data[len]);					len += 4;
	pd.qpp.rotvel.z = GET_INTEL_INT(&data[len]);					len += 4;
	
	if (multi_i_am_master()) // I am host - must relay this packet to others!
	{
		if (pd.Player_num > 0 && pd.Player_num <= N_players && Players[pd.Player_num].connected == CONNECT_PLAYING) // some checking wether this packet is legal
		{
			for (int i = 1; i < MAX_PLAYERS; i++)
			{
				if (i != pd.Player_num && Players[i].connected != CONNECT_DISCONNECTED) // not to sender or disconnected players - right.
					dxx_sendto(Netgame.players[i].protocol.udp.addr, UDP_Socket[0], data, data_len, 0);
			}
		}
	}

	net_udp_read_pdata_packet (&pd);
}

void net_udp_read_pdata_packet(UDP_frame_info *pd)
{
	int TheirPlayernum;
	int TheirObjnum;

	TheirPlayernum = pd->Player_num;
	TheirObjnum = Players[pd->Player_num].objnum;

	if (multi_i_am_master())
	{
		// latecoming player seems to successfully have synced
		if ( VerifyPlayerJoined != -1 && TheirPlayernum == VerifyPlayerJoined )
			VerifyPlayerJoined=-1;
		// we say that guy is disconnected so we do not want him/her in game
		if ( Players[TheirPlayernum].connected == CONNECT_DISCONNECTED )
			return;
	}
	else
	{
		// only by reading pdata a client can know if a player reconnected. So do that here.
		// NOTE: we might do this somewhere else - maybe with a sync packet like when adding a fresh player.
		if ( Players[TheirPlayernum].connected == CONNECT_DISCONNECTED && pd->connected == CONNECT_PLAYING )
		{
			Players[TheirPlayernum].connected = CONNECT_PLAYING;

			if (Newdemo_state == ND_STATE_RECORDING)
				newdemo_record_multi_reconnect(TheirPlayernum);

			digi_play_sample( SOUND_HUD_MESSAGE, F1_0);
			ClipRank (&Netgame.players[TheirPlayernum].rank);
			
			if (PlayerCfg.NoRankings)
				HUD_init_message(HM_MULTI, "'%s' %s", static_cast<const char *>(Players[TheirPlayernum].callsign), TXT_REJOIN );
			else
				HUD_init_message(HM_MULTI,  "%s'%s' %s", RankStrings[Netgame.players[TheirPlayernum].rank], static_cast<const char *>(Players[TheirPlayernum].callsign), TXT_REJOIN );

			multi_send_score();

			net_udp_noloss_clear_mdata_trace(TheirPlayernum);
		}
	}

	if (Players[TheirPlayernum].connected != CONNECT_PLAYING || TheirPlayernum == Player_num)
		return;

	if (!multi_quit_game && (TheirPlayernum >= N_players))
	{
		if (Network_status!=NETSTAT_WAITING)
		{
			Int3(); // We missed an important packet!
			multi_consistency_error(0);
			return;
		}
		else
			return;
	}

	const auto TheirObj = vobjptridx(TheirObjnum);
	Netgame.players[TheirPlayernum].LastPacketTime = timer_query();

	//------------ Read the player's ship's object info ----------------------

	extract_quaternionpos(TheirObj, &pd->qpp, 0);
	if (TheirObj->movement_type == MT_PHYSICS)
		set_thrust_from_velocity(TheirObj);
}

#if defined(DXX_BUILD_DESCENT_II)
static void net_udp_send_smash_lights (const playernum_t pnum)
 {
  // send the lights that have been blown out
	range_for (const auto i, highest_valid(Segments))
	{
		const auto &&segp = vsegptr(static_cast<segnum_t>(i));
		if (segp->light_subtracted)
			multi_send_light_specific(pnum, i, segp->light_subtracted);
	}
 }

static void net_udp_send_fly_thru_triggers (const playernum_t pnum)
 {
  // send the fly thru triggers that have been disabled
  for (int i=0;i<Num_triggers;i++)
   if (Triggers[i].flags & TF_DISABLED)
    multi_send_trigger_specific(pnum,i);
 }

static void net_udp_send_player_flags()
 {
	for (playernum_t i=0;i<N_players;i++)
	multi_send_flags(i);
 }
#endif

// Send the ping list in regular intervals
void net_udp_ping_frame(fix64 time)
{
	static fix64 PingTime = 0;
	
	if ((PingTime + F1_0) < time)
	{
		array<uint8_t, UPID_PING_SIZE> buf;
		int len = 0;
		
		memset(&buf, 0, sizeof(ubyte)*UPID_PING_SIZE);
		buf[len] = UPID_PING;							len++;
		memcpy(&buf[len], &time, 8);						len += 8;
		range_for (auto &i, partial_range(Netgame.players, 1u, MAX_PLAYERS))
		{
			PUT_INTEL_INT(&buf[len], i.ping);		len += 4;
		}
		
		for (int i = 1; i < MAX_PLAYERS; i++)
		{
			if (Players[i].connected == CONNECT_DISCONNECTED)
				continue;
			dxx_sendto(Netgame.players[i].protocol.udp.addr, UDP_Socket[0], buf, 0);
		}
		PingTime = time;
	}
}

// Got a PING from host. Apply the pings to our players and respond to host.
void net_udp_process_ping(const uint8_t *data, const _sockaddr &sender_addr)
{
	fix64 host_ping_time = 0;
	array<uint8_t, UPID_PONG_SIZE> buf;
	int len = 0;

	if (Netgame.players[0].protocol.udp.addr != sender_addr)
		return;

										len++; // Skip UPID byte;
	memcpy(&host_ping_time, &data[len], 8);					len += 8;
	range_for (auto &i, partial_range(Netgame.players, 1u, MAX_PLAYERS))
	{
		i.ping = GET_INTEL_INT(&(data[len]));		len += 4;
	}
	
	buf[0] = UPID_PONG;
	buf[1] = Player_num;
	memcpy(&buf[2], &host_ping_time, 8);
	
	dxx_sendto(sender_addr, UDP_Socket[0], buf, 0);
}

// Got a PONG from a client. Check the time and add it to our players.
void net_udp_process_pong(const uint8_t *data, const _sockaddr &sender_addr)
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

void net_udp_do_refuse_stuff (UDP_sequence_packet *their)
{
	int new_player_num;
	
	ClipRank (&their->player.rank);
		
	for (int i=0;i<MAX_PLAYERS;i++)
	{
		if (!d_stricmp(Players[i].callsign, their->player.callsign) && their->player.protocol.udp.addr == Netgame.players[i].protocol.udp.addr)
		{
			net_udp_welcome_player(their);
			return;
		}
	}

	if (!WaitForRefuseAnswer)
	{
		for (int i=0;i<MAX_PLAYERS;i++)
		{
			if (!d_stricmp(Players[i].callsign, their->player.callsign) && their->player.protocol.udp.addr == Netgame.players[i].protocol.udp.addr)
			{
				net_udp_welcome_player(their);
				return;
			}
		}
	
#if defined(DXX_BUILD_DESCENT_I)
		digi_play_sample (SOUND_CONTROL_CENTER_WARNING_SIREN,F1_0*2);
#elif defined(DXX_BUILD_DESCENT_II)
		digi_play_sample (SOUND_HUD_JOIN_REQUEST,F1_0*2);
#endif
	
		if (Game_mode & GM_TEAM)
		{
			if (!PlayerCfg.NoRankings)
			{
				HUD_init_message(HM_MULTI, "%s %s wants to join",RankStrings[their->player.rank],static_cast<const char *>(their->player.callsign));
			}
			else
			{
				HUD_init_message(HM_MULTI, "%s wants to join",static_cast<const char *>(their->player.callsign));
			}
			HUD_init_message(HM_MULTI, "Alt-1 assigns to team %s. Alt-2 to team %s", static_cast<const char *>(Netgame.team_name[0]), static_cast<const char *>(Netgame.team_name[1]));
		}
		else
		{
			HUD_init_message(HM_MULTI, "%s wants to join (accept: F6)", static_cast<const char *>(their->player.callsign));
		}
	
		strcpy (RefusePlayerName,their->player.callsign);
		RefuseTimeLimit=timer_query();
		RefuseThisPlayer=0;
		WaitForRefuseAnswer=1;
	}
	else
	{
		for (int i=0;i<MAX_PLAYERS;i++)
		{
			if (!d_stricmp(Players[i].callsign, their->player.callsign) && their->player.protocol.udp.addr == Netgame.players[i].protocol.udp.addr)
			{
				net_udp_welcome_player(their);
				return;
			}
		}
	
		if (strcmp(their->player.callsign,RefusePlayerName))
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
				net_udp_welcome_player(their);
				net_udp_send_netgame_update();
			}
			else
			{
				net_udp_welcome_player(their);
			}
			return;
		}

		if ((timer_query()) > RefuseTimeLimit+REFUSE_INTERVAL)
		{
			RefuseTimeLimit=0;
			RefuseThisPlayer=0;
			WaitForRefuseAnswer=0;
			if (!strcmp (their->player.callsign,RefusePlayerName))
			{
				net_udp_dump_player(their->player.protocol.udp.addr, DUMP_DORK);
			}
			return;
		}
	}
}

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

			for (int i = 0; i < N_players; i++)
			{
				if ( (!Players[i].connected) && (Netgame.players[i].LastPacketTime < oldest_time))
				{
					oldest_time = Netgame.players[i].LastPacketTime;
					oldest_player = i;
				}
			}
	    return (oldest_player);
	  }
  }

void net_udp_send_extras ()
{
	static fix64 last_send_time = 0;
	
	if (last_send_time + (F1_0/50) > timer_query())
		return;
	last_send_time = timer_query();

	Assert (Player_joining_extras>-1);

#if defined(DXX_BUILD_DESCENT_I)
	if (Network_sending_extras==3 && (Netgame.PlayTimeAllowed || Netgame.KillGoal))
#elif defined(DXX_BUILD_DESCENT_II)
	if (Network_sending_extras==9)
		net_udp_send_fly_thru_triggers(Player_joining_extras);
	if (Network_sending_extras==8)
		net_udp_send_door_updates(Player_joining_extras);
	if (Network_sending_extras==7)
		multi_send_markers();
	if (Network_sending_extras==6 && (Game_mode & GM_MULTI_ROBOTS))
		multi_send_stolen_items();
	if (Network_sending_extras==5 && (Netgame.PlayTimeAllowed || Netgame.KillGoal))
#endif
		multi_send_kill_goal_counts();
#if defined(DXX_BUILD_DESCENT_II)
	if (Network_sending_extras==4)
		net_udp_send_smash_lights(Player_joining_extras);
	if (Network_sending_extras==3)
		net_udp_send_player_flags();    
#endif
	if (Network_sending_extras==2)
		multi_send_powcap_update();
	if (Network_sending_extras==1 && Game_mode & GM_BOUNTY)
		multi_send_bounty();

	Network_sending_extras--;
	if (!Network_sending_extras)
		Player_joining_extras=-1;
}

static window_event_result show_game_rules_handler(window *wind,const d_event &event, netgame_info *netgame)
{
	int k;
	const auto &&fspacx = FSPACX();
	const auto &&fspacy = FSPACY();
#if defined(DXX_BUILD_DESCENT_I)
	int w = fspacx(280), h = fspacy(130);
#elif defined(DXX_BUILD_DESCENT_II)
	int w = fspacx(280), h = fspacy(170);
#endif
	
	switch (event.type)
	{
		case EVENT_WINDOW_ACTIVATED:
			game_flush_inputs();
			break;
			
		case EVENT_KEY_COMMAND:
			k = event_key_get(event);
			switch (k)
			{
				case KEY_ENTER:
				case KEY_SPACEBAR:
				case KEY_ESC:
					return window_event_result::close;
			}
			break;
			
		case EVENT_WINDOW_DRAW:
			{
			timer_delay2(50);

			gr_set_current_canvas(NULL);
			nm_draw_background(((SWIDTH-w)/2)-BORDERX,((SHEIGHT-h)/2)-BORDERY,((SWIDTH-w)/2)+w+BORDERX,((SHEIGHT-h)/2)+h+BORDERY);

#define SHOW_INVULNERABLE_APPEAR_STRING(X,Y)	\
			if (netgame->InvulAppear)	\
				gr_printf(X, Y, "%1.1f sec", static_cast<float>(netgame->InvulAppear) / 2);	\
			else	\
				gr_string(X, Y, "OFF");

			gr_set_current_canvas(window_get_canvas(*wind));
			gr_set_curfont(MEDIUM3_FONT);
			gr_set_fontcolor(gr_find_closest_color_current(29,29,47),-1);
			const auto &&fspacx25 = fspacx(25);
			const auto &&fspacx115 = fspacx(115);
			const auto &&fspacx130 = fspacx(130);
			const auto &&fspacx155 = fspacx(155);
			const auto &&fspacx170 = fspacx(170);
			const auto &&fspacx275 = fspacx(275);
#if defined(DXX_BUILD_DESCENT_I)
			gr_string(0x8000, fspacy(35), "NETGAME INFO");
			
			gr_set_curfont(GAME_FONT);
			gr_string(fspacx25, fspacy( 55), "Reactor Life:");
			gr_string(fspacx25, fspacy( 61), "Max Time:");
			gr_string(fspacx25, fspacy( 67), "Kill Goal:");
			gr_string(fspacx25, fspacy( 73), "Packets per sec.:");
			gr_string(fspacx155, fspacy( 55), "Invul when reappearing:");
			gr_string(fspacx155, fspacy( 61), "Bright player ships:");
			gr_string(fspacx155, fspacy( 67), "Show enemy names on hud:");
			gr_string(fspacx155, fspacy( 73), "Show players on automap:");
			gr_string(fspacx155, fspacy( 79), "No friendly Fire:");
			gr_string(fspacx25, fspacy(100), "Allowed Objects");
			gr_string(fspacx25, fspacy(110), "Laser Upgrade:");
			gr_string(fspacx25, fspacy(116), "Quad Laser:");
			gr_string(fspacx25, fspacy(122), "Vulcan Cannon:");
			gr_string(fspacx25, fspacy(128), "Spreadfire Cannon:");
			gr_string(fspacx25, fspacy(134), "Plasma Cannon:");
			gr_string(fspacx25, fspacy(140), "Fusion Cannon:");
			gr_string(fspacx170, fspacy(110), "Homing Missile:");
			gr_string(fspacx170, fspacy(116), "Proximity Bomb:");
			gr_string(fspacx170, fspacy(122), "Smart Missile:");
			gr_string(fspacx170, fspacy(128), "Mega Missile:");
			gr_string(fspacx25, fspacy(150), "Invulnerability:");
			gr_string(fspacx25, fspacy(156), "Cloak:");
			
			gr_set_fontcolor(gr_find_closest_color_current(255,255,255),-1);
			gr_printf(fspacx115, fspacy( 55), "%i Min", netgame->control_invul_time / F1_0 / 60);
			gr_printf(fspacx115, fspacy( 61), "%i Min", netgame->PlayTimeAllowed * 5);
			gr_printf(fspacx115, fspacy( 67), "%i", netgame->KillGoal * 5);
			gr_printf(fspacx115, fspacy( 73), "%i", netgame->PacketsPerSec);
			SHOW_INVULNERABLE_APPEAR_STRING(fspacx275, fspacy( 55));
			gr_string(fspacx275, fspacy( 61), netgame->BrightPlayers?"ON":"OFF");
			gr_string(fspacx275, fspacy( 67), netgame->ShowEnemyNames?"ON":"OFF");
			gr_string(fspacx275, fspacy( 73), netgame->game_flag.show_on_map?"ON":"OFF");
			gr_string(fspacx275, fspacy( 79), netgame->NoFriendlyFire?"ON":"OFF");
			gr_string(fspacx130, fspacy(110), netgame->AllowedItems&NETFLAG_DOLASER?"YES":"NO");
			gr_string(fspacx130, fspacy(116), netgame->AllowedItems&NETFLAG_DOQUAD?"YES":"NO");
			gr_string(fspacx130, fspacy(122), netgame->AllowedItems&NETFLAG_DOVULCAN?"YES":"NO");
			gr_string(fspacx130, fspacy(128), netgame->AllowedItems&NETFLAG_DOSPREAD?"YES":"NO");
			gr_string(fspacx130, fspacy(134), netgame->AllowedItems&NETFLAG_DOPLASMA?"YES":"NO");
			gr_string(fspacx130, fspacy(140), netgame->AllowedItems&NETFLAG_DOFUSION?"YES":"NO");
			gr_string(fspacx275, fspacy(110), netgame->AllowedItems&NETFLAG_DOHOMING?"YES":"NO");
			gr_string(fspacx275, fspacy(116), netgame->AllowedItems&NETFLAG_DOPROXIM?"YES":"NO");
			gr_string(fspacx275, fspacy(122), netgame->AllowedItems&NETFLAG_DOSMART?"YES":"NO");
			gr_string(fspacx275, fspacy(128), netgame->AllowedItems&NETFLAG_DOMEGA?"YES":"NO");
			gr_string(fspacx130, fspacy(150), netgame->AllowedItems&NETFLAG_DOINVUL?"YES":"NO");
			gr_string(fspacx130, fspacy(156), netgame->AllowedItems&NETFLAG_DOCLOAK?"YES":"NO");
#elif defined(DXX_BUILD_DESCENT_II)
			gr_string(0x8000, fspacy(15), "NETGAME INFO");
			
			gr_set_curfont(GAME_FONT);
			gr_string(fspacx25, fspacy( 35), "Reactor Life:");
			gr_string(fspacx25, fspacy( 41), "Max Time:");
			gr_string(fspacx25, fspacy( 47), "Kill Goal:");
			gr_string(fspacx25, fspacy( 53), "Packets per sec.:");
			gr_string(fspacx155, fspacy( 35), "Invul when reappearing:");
			gr_string(fspacx155, fspacy( 41), "Marker camera views:");
			gr_string(fspacx155, fspacy( 47), "Indestructible lights:");
			gr_string(fspacx155, fspacy( 53), "Bright player ships:");
			gr_string(fspacx155, fspacy( 59), "Show enemy names on hud:");
			gr_string(fspacx155, fspacy( 65), "Show players on automap:");
			gr_string(fspacx155, fspacy( 71), "No friendly Fire:");
			gr_string(fspacx25, fspacy( 80), "Allowed Objects");
			gr_string(fspacx25, fspacy( 90), "Laser Upgrade:");
			gr_string(fspacx25, fspacy( 96), "Super Laser:");
			gr_string(fspacx25, fspacy(102), "Quad Laser:");
			gr_string(fspacx25, fspacy(108), "Vulcan Cannon:");
			gr_string(fspacx25, fspacy(114), "Gauss Cannon:");
			gr_string(fspacx25, fspacy(120), "Spreadfire Cannon:");
			gr_string(fspacx25, fspacy(126), "Helix Cannon:");
			gr_string(fspacx25, fspacy(132), "Plasma Cannon:");
			gr_string(fspacx25, fspacy(138), "Phoenix Cannon:");
			gr_string(fspacx25, fspacy(144), "Fusion Cannon:");
			gr_string(fspacx25, fspacy(150), "Omega Cannon:");
			gr_string(fspacx170, fspacy( 90), "Flash Missile:");
			gr_string(fspacx170, fspacy( 96), "Homing Missile:");
			gr_string(fspacx170, fspacy(102), "Guided Missile:");
			gr_string(fspacx170, fspacy(108), "Proximity Bomb:");
			gr_string(fspacx170, fspacy(114), "Smart Mine:");
			gr_string(fspacx170, fspacy(120), "Smart Missile:");
			gr_string(fspacx170, fspacy(126), "Mercury Missile:");
			gr_string(fspacx170, fspacy(132), "Mega Missile:");
			gr_string(fspacx170, fspacy(138), "Earthshaker Missile:");
			gr_string(fspacx25, fspacy(160), "Afterburner:");
			gr_string(fspacx25, fspacy(166), "Headlight:");
			gr_string(fspacx25, fspacy(172), "Energy->Shield Conv:");
			gr_string(fspacx170, fspacy(160), "Invulnerability:");
			gr_string(fspacx170, fspacy(166), "Cloaking Device:");
			gr_string(fspacx170, fspacy(172), "Ammo Rack:");
			gr_set_fontcolor(BM_XRGB(255,255,255),-1);
			gr_printf(fspacx115, fspacy( 35), "%i Min", netgame->control_invul_time/F1_0/60);
			gr_printf(fspacx115, fspacy( 41), "%i Min", netgame->PlayTimeAllowed*5);
			gr_printf(fspacx115, fspacy( 47), "%i", netgame->KillGoal*5);
			gr_printf(fspacx115, fspacy( 53), "%i", netgame->PacketsPerSec);
			SHOW_INVULNERABLE_APPEAR_STRING(fspacx275, fspacy( 35));
			gr_string(fspacx275, fspacy( 41), netgame->Allow_marker_view?"ON":"OFF");
			gr_string(fspacx275, fspacy( 47), netgame->AlwaysLighting?"ON":"OFF");
			gr_string(fspacx275, fspacy( 53), netgame->BrightPlayers?"ON":"OFF");
			gr_string(fspacx275, fspacy( 59), netgame->ShowEnemyNames?"ON":"OFF");
			gr_string(fspacx275, fspacy( 65), netgame->game_flag.show_on_map?"ON":"OFF");
			gr_string(fspacx275, fspacy( 71), netgame->NoFriendlyFire?"ON":"OFF");
			gr_string(fspacx130, fspacy( 90), netgame->AllowedItems & NETFLAG_DOLASER?"YES":"NO");
			gr_string(fspacx130, fspacy( 96), netgame->AllowedItems & NETFLAG_DOSUPERLASER?"YES":"NO");
			gr_string(fspacx130, fspacy(102), netgame->AllowedItems & NETFLAG_DOQUAD?"YES":"NO");
			gr_string(fspacx130, fspacy(108), netgame->AllowedItems & NETFLAG_DOVULCAN?"YES":"NO");
			gr_string(fspacx130, fspacy(114), netgame->AllowedItems & NETFLAG_DOGAUSS?"YES":"NO");
			gr_string(fspacx130, fspacy(120), netgame->AllowedItems & NETFLAG_DOSPREAD?"YES":"NO");
			gr_string(fspacx130, fspacy(126), netgame->AllowedItems & NETFLAG_DOHELIX?"YES":"NO");
			gr_string(fspacx130, fspacy(132), netgame->AllowedItems & NETFLAG_DOPLASMA?"YES":"NO");
			gr_string(fspacx130, fspacy(138), netgame->AllowedItems & NETFLAG_DOPHOENIX?"YES":"NO");
			gr_string(fspacx130, fspacy(144), netgame->AllowedItems & NETFLAG_DOFUSION?"YES":"NO");
			gr_string(fspacx130, fspacy(150), netgame->AllowedItems & NETFLAG_DOOMEGA?"YES":"NO");
			gr_string(fspacx275, fspacy( 90), netgame->AllowedItems & NETFLAG_DOFLASH?"YES":"NO");
			gr_string(fspacx275, fspacy( 96), netgame->AllowedItems & NETFLAG_DOHOMING?"YES":"NO");
			gr_string(fspacx275, fspacy(102), netgame->AllowedItems & NETFLAG_DOGUIDED?"YES":"NO");
			gr_string(fspacx275, fspacy(108), netgame->AllowedItems & NETFLAG_DOPROXIM?"YES":"NO");
			gr_string(fspacx275, fspacy(114), netgame->AllowedItems & NETFLAG_DOSMARTMINE?"YES":"NO");
			gr_string(fspacx275, fspacy(120), netgame->AllowedItems & NETFLAG_DOSMART?"YES":"NO");
			gr_string(fspacx275, fspacy(126), netgame->AllowedItems & NETFLAG_DOMERCURY?"YES":"NO");
			gr_string(fspacx275, fspacy(132), netgame->AllowedItems & NETFLAG_DOMEGA?"YES":"NO");
			gr_string(fspacx275, fspacy(138), netgame->AllowedItems & NETFLAG_DOSHAKER?"YES":"NO");
			gr_string(fspacx130, fspacy(160), netgame->AllowedItems & NETFLAG_DOAFTERBURNER?"YES":"NO");
			gr_string(fspacx130, fspacy(166), netgame->AllowedItems & NETFLAG_DOHEADLIGHT?"YES":"NO");
			gr_string(fspacx130, fspacy(172), netgame->AllowedItems & NETFLAG_DOCONVERTER?"YES":"NO");
			gr_string(fspacx275, fspacy(160), netgame->AllowedItems & NETFLAG_DOINVUL?"YES":"NO");
			gr_string(fspacx275, fspacy(166), netgame->AllowedItems & NETFLAG_DOCLOAK?"YES":"NO");
			gr_string(fspacx275, fspacy(172), netgame->AllowedItems & NETFLAG_DOAMMORACK?"YES":"NO");
#endif
			gr_set_current_canvas(NULL);
			break;
			}

		default:
			break;
	}
	return window_event_result::ignored;
}

static void net_udp_show_game_rules(netgame_info *netgame)
{
	gr_set_current_canvas(NULL);

	const auto &&fspacx = FSPACX();
	const auto &&fspacy = FSPACY();
	window_create(&grd_curscreen->sc_canvas, (SWIDTH - fspacx(320)) / 2, (SHEIGHT - fspacy(200)) / 2, fspacx(320), fspacy(200), 
				  show_game_rules_handler, netgame);
}

static int show_game_info_handler(newmenu *, const d_event &event, netgame_info *netgame)
{
	switch (event.type)
	{
		case EVENT_NEWMENU_SELECTED:
		{
			auto &citem = static_cast<const d_select_event &>(event).citem;
			if (citem != 1)
				return 0;
			net_udp_show_game_rules(netgame);
			return 1;
		}
		default:
			return 0;
	}
}

int net_udp_show_game_info()
{
	char rinfo[512];
	int c;
	netgame_info *netgame = &Netgame;

#if defined(DXX_BUILD_DESCENT_I)
#define DXX_DEFAULT_MISSION_TITLE	"Descent: First Strike"
#define DXX_SECRET_LEVEL_FORMAT	"%s"
#define DXX_SECRET_LEVEL_PARAMETER	(netgame->levelnum >= 0 ? "" : "S"), \
	netgame->levelnum < 0 ? -netgame->levelnum :	/* else portion provided by invoker */
#elif defined(DXX_BUILD_DESCENT_II)
#define DXX_DEFAULT_MISSION_TITLE	"Descent2: CounterStrike"
#define DXX_SECRET_LEVEL_FORMAT
#define DXX_SECRET_LEVEL_PARAMETER
#endif
	unsigned gamemode = netgame->gamemode;
	unsigned players;
#if defined(DXX_BUILD_DESCENT_I)
	players = netgame->numplayers;
#elif defined(DXX_BUILD_DESCENT_II)
	players = netgame->numconnected;
#endif
#define GAME_INFO_FORMAT_TEXT(F)	\
	F("\nConnected to\n\"%s\"\n", netgame->game_name.data())	\
	F("%s", netgame->mission_title.data())	\
	F(" - Lvl " DXX_SECRET_LEVEL_FORMAT "%i", DXX_SECRET_LEVEL_PARAMETER netgame->levelnum)	\
	F("\n\nDifficulty: %s", MENU_DIFFICULTY_TEXT(netgame->difficulty))	\
	F("\nGame Mode: %s", gamemode < lengthof(GMNames) ? GMNames[gamemode] : "INVALID")	\
	F("\nPlayers: %u/%i", players, netgame->max_numplayers)
#define EXPAND_FORMAT(A,B,...)	A
#define EXPAND_ARGUMENT(A,B,...)	, B, ## __VA_ARGS__
	snprintf(rinfo, lengthof(rinfo), GAME_INFO_FORMAT_TEXT(EXPAND_FORMAT) GAME_INFO_FORMAT_TEXT(EXPAND_ARGUMENT));

	array<newmenu_item, 2> nm_message_items{{
		nm_item_menu("JOIN GAME"),
		nm_item_menu("GAME INFO"),
	}};
	c = newmenu_do("WELCOME", rinfo, nm_message_items, show_game_info_handler, netgame);
	if (c==0)
		return 1;
	//else if (c==1)
	// handled in above callback
	else
		return 0;
}
