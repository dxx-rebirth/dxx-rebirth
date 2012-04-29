/*
 * 
 * Routines for managing UDP-protocol network play.
 * 
 */

#ifdef NETWORK

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
#include "error.h"
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
#include "menu.h"
#include "sounds.h"
#include "text.h"
#include "kmatrix.h"
#include "newdemo.h"
#include "multibot.h"
#include "wall.h"
#include "bm.h"
#include "effects.h"
#include "physics.h"
#include "vers_id.h"
#include "gamefont.h"
#include "playsave.h"
#include "rbaudio.h"
#include "byteswap.h"
#include "config.h"

// Prototypes
void net_udp_init();
void net_udp_close();
void net_udp_request_game_info(struct _sockaddr game_addr, int lite);
void net_udp_listen();
int net_udp_show_game_info();
int net_udp_do_join_game();
int net_udp_can_join_netgame(netgame_info *game);
void net_udp_flush();
void net_udp_update_netgame(void);
void net_udp_send_objects(void);
void net_udp_send_rejoin_sync(int player_num);
void net_udp_send_game_info(struct _sockaddr sender_addr, ubyte info_upid);
void net_udp_send_netgame_update();
void net_udp_do_refuse_stuff (UDP_sequence_packet *their);
void net_udp_read_sync_packet( ubyte * data, int data_len, struct _sockaddr sender_addr );
void net_udp_read_object_packet( ubyte *data );
void net_udp_ping_frame(fix64 time);
void net_udp_process_ping(ubyte *data, int data_len, struct _sockaddr sender_addr);
void net_udp_process_pong(ubyte *data, int data_len, struct _sockaddr sender_addr);
void net_udp_process_game_info(ubyte *data, int data_len, struct _sockaddr game_addr, int lite_info);
void net_udp_read_endlevel_packet( ubyte *data, int data_len, struct _sockaddr sender_addr );
void net_udp_send_mdata(int needack, fix64 time);
void net_udp_process_mdata (ubyte *data, int data_len, struct _sockaddr sender_addr, int needack);
void net_udp_send_pdata();
void net_udp_process_pdata ( ubyte *data, int data_len, struct _sockaddr sender_addr );
void net_udp_read_pdata_short_packet(UDP_frame_info *pd);
void net_udp_timeout_check(fix64 time);
int net_udp_get_new_player_num (UDP_sequence_packet *their);
void net_udp_noloss_add_queue_pkt(uint32_t pkt_num, fix64 time, ubyte *data, ushort data_size, ubyte pnum, ubyte player_ack[MAX_PLAYERS]);
int net_udp_noloss_validate_mdata(uint32_t pkt_num, ubyte sender_pnum, struct _sockaddr sender_addr);
void net_udp_noloss_got_ack(ubyte *data, int data_len);
void net_udp_noloss_init_mdata_queue(void);
void net_udp_noloss_clear_mdata_got(ubyte player_num);
void net_udp_noloss_process_queue(fix64 time);
void net_udp_send_extras ();
extern void multi_send_kill_goal_counts();

// Variables
int UDP_num_sendto = 0, UDP_len_sendto = 0, UDP_num_recvfrom = 0, UDP_len_recvfrom = 0;
UDP_mdata_info		UDP_MData;
UDP_sequence_packet UDP_Seq;
UDP_mdata_store UDP_mdata_queue[UDP_MDATA_STOR_QUEUE_SIZE];
UDP_mdata_recv UDP_mdata_got[MAX_PLAYERS];
UDP_sequence_packet UDP_sync_player; // For rejoin object syncing
UDP_netgame_info_lite Active_udp_games[UDP_MAX_NETGAMES];
int num_active_udp_games = 0;
int num_active_udp_changed = 0;
static int UDP_Socket[3] = { -1, -1, -1 };
static char UDP_MyPort[6] = "";
struct _sockaddr GBcast; // global Broadcast address clients and hosts will use for lite_info exchange over LAN
#ifdef IPv6
struct _sockaddr GMcast_v6; // same for IPv6-only
#endif
#ifdef USE_TRACKER
struct _sockaddr TrackerSocket;
int iTrackerVerified = 0;
#endif
extern obj_position Player_init[MAX_PLAYERS];

/* General UDP functions - START */
ssize_t dxx_sendto(int sockfd, const void *msg, int len, unsigned int flags, const struct sockaddr *to, socklen_t tolen)
{
	ssize_t rv = sendto(sockfd, msg, len, flags, to, tolen);

	UDP_num_sendto++;
	if (rv > 0)
		UDP_len_sendto += rv;

	return rv;
}

ssize_t dxx_recvfrom(int sockfd, void *buf, int len, unsigned int flags, struct sockaddr *from, unsigned int *fromlen)
{
	ssize_t rv = recvfrom(sockfd, buf, len, flags, from, fromlen);

	UDP_num_recvfrom++;
	UDP_len_recvfrom += rv;

	return rv;
}

void udp_traffic_stat()
{
	static fix64 last_traf_time = 0;

	if (timer_query() >= last_traf_time + F1_0)
	{
		last_traf_time = timer_query();
		con_printf(CON_VERBOSE, "P#%i TRAFFIC - OUT: %fKB/s %iPPS IN: %fKB/s %iPPS\n",Player_num, (float)UDP_len_sendto/1024, UDP_num_sendto, (float)UDP_len_recvfrom/1024, UDP_num_recvfrom);
		UDP_num_sendto = UDP_len_sendto = UDP_num_recvfrom = UDP_len_recvfrom = 0;
	}
}

// Resolve address
int udp_dns_filladdr( char *host, int port, struct _sockaddr *sAddr )
{
	// Variables
	struct addrinfo *result, hints;
	char sPort[6];
	
	// Build the port
	snprintf( sPort, 6, "%d", port );
	
	// Set up hints
	memset( &hints, 0, sizeof( hints ) );
	
	// Uncomment the following if we want ONLY what we compile for
	// hints.ai_family = _af;
	
	// We are always UDP
	hints.ai_socktype = SOCK_DGRAM;
	
	// Resolve the domain name
	if( getaddrinfo( host, sPort, &hints, &result ) != 0 )
	{
		con_printf( CON_URGENT, "udp_dns_filladdr (getaddrinfo) failed\n" );
		nm_messagebox( TXT_ERROR, 1, TXT_OK, "Could not resolve address" );
		return -1;
	}
	
	// Zero out the target first
	memset( sAddr, 0, sizeof( struct _sockaddr ) );
	
	// Now copy it over
	memcpy( sAddr, result->ai_addr, result->ai_addrlen );
	
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
	freeaddrinfo( result );
	return 0;
}

// Closes an existing udp socket
void udp_close_socket(int socknum)
{
	if (UDP_Socket[socknum] != -1)
	{
#ifdef _WIN32
		closesocket(UDP_Socket[socknum]);
#else
		close (UDP_Socket[socknum]);
#endif
	}
	UDP_Socket[socknum] = -1;
}

// Open socket
int udp_open_socket(int socknum, int port)
{
	int bcast = 1;

	// close stale socket
	if( UDP_Socket[socknum] != -1 )
		udp_close_socket(socknum);

	{
#ifdef _WIN32
	struct _sockaddr sAddr;   // my address information

	memset( &sAddr, '\0', sizeof( sAddr ) );

	if ((UDP_Socket[socknum] = socket (_af, SOCK_DGRAM, 0)) < 0) {
		con_printf(CON_URGENT,"udp_open_socket: socket creation failed\n");
		nm_messagebox(TXT_ERROR,1,TXT_OK,"Could not create socket");
		return -1;
	}

#ifdef IPv6
	sAddr.sin6_family = _pf; // host byte order
	sAddr.sin6_port = htons (port); // short, network byte order
	sAddr.sin6_flowinfo = 0;
	sAddr.sin6_addr = in6addr_any; // automatically fill with my IP
	sAddr.sin6_scope_id = 0;
#else
	sAddr.sin_family = _pf; // host byte order
	sAddr.sin_port = htons (port); // short, network byte order
	sAddr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
	memset (&(sAddr.sin_zero), '\0', 8); // zero the rest of the struct
#endif
	
	if (bind (UDP_Socket[socknum], (struct sockaddr *) &sAddr, sizeof (struct sockaddr)) < 0) 
	{      
		con_printf(CON_URGENT,"udp_open_socket: bind name to socket failed\n");
		nm_messagebox(TXT_ERROR,1,TXT_OK,"Could not bind name to socket");
		udp_close_socket(socknum);
		return -1;
	}
	(void)setsockopt( UDP_Socket[socknum], SOL_SOCKET, SO_BROADCAST, (const char *) &bcast, sizeof(bcast) );
#else
	struct addrinfo hints,*res,*sres;
	int err,ai_family_;
	char cport[6];
	
	memset (&hints, '\0', sizeof (struct addrinfo));
	memset(cport,'\0',sizeof(char)*6);
	
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = _pf;
	hints.ai_socktype = SOCK_DGRAM;
	
	ai_family_ = 0;

	sprintf(cport,"%i",port);

	if ((err = getaddrinfo (NULL, cport, &hints, &res)) == 0)
	{
		sres = res;
		while ((ai_family_ == 0) && (sres))
		{
			if (sres->ai_family == _pf || _pf == PF_UNSPEC)
				ai_family_ = sres->ai_family;
			else
				sres = sres->ai_next;
		}
	
		if (sres == NULL)
			sres = res;
	
		ai_family_ = sres->ai_family;
		if (ai_family_ != _pf && _pf != PF_UNSPEC)
		{
			// ai_family is not identic
			freeaddrinfo (res);
			return -1;
		}
	
		if ((UDP_Socket[socknum] = socket (sres->ai_family, SOCK_DGRAM, 0)) < 0)
		{
			con_printf(CON_URGENT,"udp_open_socket: socket creation failed\n");
			nm_messagebox(TXT_ERROR,1,TXT_OK,"Could not create socket");
			freeaddrinfo (res);
			return -1;
		}
	
		if ((err = bind (UDP_Socket[socknum], sres->ai_addr, sres->ai_addrlen)) < 0)
		{
			con_printf(CON_URGENT,"udp_open_socket: bind name to socket failed\n");
			nm_messagebox(TXT_ERROR,1,TXT_OK,"Could not bind name to socket");
			udp_close_socket(socknum);
			freeaddrinfo (res);
			return -1;
		}
	
		freeaddrinfo (res);
	}
	else {
		UDP_Socket[socknum] = -1;
		con_printf(CON_URGENT,"udp_open_socket (getaddrinfo):%s\n", gai_strerror (err));
		nm_messagebox(TXT_ERROR,1,TXT_OK,"Could not get address information:\n%s",gai_strerror (err));
	}
	setsockopt( UDP_Socket[socknum], SOL_SOCKET, SO_BROADCAST, &bcast, sizeof(bcast) );
#endif

	return 0;
	}
}

int udp_general_packet_ready(int socknum)
{
	fd_set set;
	struct timeval tv;

	FD_ZERO(&set);
	FD_SET(UDP_Socket[socknum], &set);
	tv.tv_sec = tv.tv_usec = 0;
	if (select(UDP_Socket[socknum] + 1, &set, NULL, NULL, &tv) > 0)
		return 1;
	else
		return 0;
}

// Gets some text. Returns 0 if nothing on there.
int udp_receive_packet(int socknum, ubyte *text, int len, struct _sockaddr *sender_addr)
{
	unsigned int clen = sizeof (struct _sockaddr), msglen = 0;

	if (UDP_Socket[socknum] == -1)
		return -1;

	if (udp_general_packet_ready(socknum))
	{
		msglen = dxx_recvfrom (UDP_Socket[socknum], text, len, 0, (struct sockaddr *)sender_addr, &clen);

		if (msglen < 0)
			return 0;

		if ((msglen >= 0) && (msglen < len))
			text[msglen] = 0;
	}

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
int udp_tracker_init()
{
	// Zero it out
	memset( &TrackerSocket, 0, sizeof( TrackerSocket ) );
	
	// Open the socket
	udp_open_socket( 2, d_rand() % 0xffff );
	
	// Fill the address
	if( udp_dns_filladdr( (char *)GameArg.MplTrackerAddr, GameArg.MplTrackerPort, &TrackerSocket ) < 0 )
		return -1;
	
	// Yay
	return 0;
}

/* Unregister from the tracker */
int udp_tracker_unregister()
{
	// Variables
	int iLen = 5;
	ubyte pBuf[iLen];
	
	// Put the opcode
	pBuf[0] = TRACKER_PKT_UNREGISTER;
	
	// Put the GameID
	PUT_INTEL_INT( &pBuf[1], Netgame.protocol.udp.GameID );
	
	// Send it off
	return dxx_sendto( UDP_Socket[2], pBuf, iLen, 0, (struct sockaddr *)&TrackerSocket, sizeof( TrackerSocket ) );
}

/* Tell the tracker we're starting a game */
int udp_tracker_register()
{
	// Variables
	int iLen = 14;
	ubyte pBuf[iLen];
	
	// Reset the last tracker message
	iTrackerVerified = 0;
	
	// Put the opcode
	pBuf[0] = TRACKER_PKT_REGISTER;
	
	// Put the protocol version
	pBuf[1] = TRACKER_SYS_VERSION;
	
	// Write the game version (d1 = 1, d2 = 2, x = oshiii)
	pBuf[2] = 0x01;
	
	// Write the port we're running on
	PUT_INTEL_SHORT( &pBuf[3], atoi( UDP_MyPort ) );
	
	// Put the GameID
	PUT_INTEL_INT( &pBuf[5], Netgame.protocol.udp.GameID );
	
	// Now, put the game version
	PUT_INTEL_SHORT( &pBuf[9], D1XMAJORi );
	PUT_INTEL_SHORT( &pBuf[11], D1XMINORi );
	PUT_INTEL_SHORT( &pBuf[13], D1XMICROi );
	
	// Send it off
	return dxx_sendto( UDP_Socket[2], pBuf, iLen, 0, (struct sockaddr *)&TrackerSocket, sizeof( TrackerSocket ) );
}

/* Ask the tracker to send us a list of games */
int udp_tracker_reqgames()
{
	// Variables
	int iLen = 3;
	ubyte pBuf[iLen];
	
	// Put the opcode
	pBuf[0] = TRACKER_PKT_GAMELIST;
	
// 	// Put the game version (d1)
	pBuf[1] = 1;
	
	// If we're IPv6 ready, send that too
#ifdef IPv6
	pBuf[2] = 1;
#else
	pBuf[2] = 0;
#endif
	
	// Send it off
	return dxx_sendto( UDP_Socket[2], pBuf, iLen, 0, (struct sockaddr *)&TrackerSocket, sizeof( TrackerSocket ) );
}

/* The tracker has sent us a game.  Let's list it. */
int udp_tracker_process_game( ubyte *data, int data_len )
{
	// All our variables
	struct _sockaddr sAddr;
	int iPos = 1;
	int iPort = 0;
	int bIPv6 = 0;
	char *sIP = NULL;
	
	// Zero it out
	memset( &sAddr, 0, sizeof( sAddr ) );
	
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
	if( udp_dns_filladdr( sIP, iPort, &sAddr ) < 0 )
		return -1;
	
	// Now move on to BIGGER AND BETTER THINGS!
	net_udp_process_game_info( &data[iPos - 1], data_len - iPos, sAddr, 1 );
	return 0;
}
#endif /* USE_TRACKER */

typedef struct direct_join
{
	struct _sockaddr host_addr;
	int connecting;
	fix64 start_time, last_time;
	char addrbuf[128];
	char portbuf[6];
} direct_join;

// Connect to a game host and get full info. Eventually we join!
int net_udp_game_connect(direct_join *dj)
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
		nm_messagebox(TXT_ERROR,1,TXT_OK,"Version mismatch! Cannot join Game.\nHost game version: %i.%i.%i\nHost game protocol: %i\nYour game version: %s\nYour game protocol: %i",Netgame.protocol.udp.program_iver[0],Netgame.protocol.udp.program_iver[1],Netgame.protocol.udp.program_iver[2],Netgame.protocol.udp.program_iver[3],VERSION, MULTI_PROTO_VERSION);
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

static char *connecting_txt = "Connecting...";
static char *blank = "";

static int manual_join_game_handler(newmenu *menu, d_event *event, direct_join *dj)
{
	newmenu_item *items = newmenu_get_items(menu);

	switch (event->type)
	{
		case EVENT_KEY_COMMAND:
			if (dj->connecting && event_key_get(event) == KEY_ESC)
			{
				dj->connecting = 0;
				items[6].text = blank;
				return 1;
			}
			break;
			
		case EVENT_IDLE:
			if (dj->connecting)
			{
				if (net_udp_game_connect(dj))
					return -2;	// Success!
				else if (!dj->connecting)
					items[6].text = blank;
			}
			break;

		case EVENT_NEWMENU_SELECTED:
		{
			int sockres = -1;
			
			if ((atoi(UDP_MyPort)) < 0 ||(atoi(UDP_MyPort)) > 65535)
			{
				snprintf (UDP_MyPort, sizeof(UDP_MyPort), "%d", UDP_PORT_DEFAULT);
				nm_messagebox(TXT_ERROR, 1, TXT_OK, "Illegal port");
				return 1;
			}
			
			sockres = udp_open_socket(0, atoi(UDP_MyPort));
			
			if (sockres != 0)
			{
				return 1;
			}
			
			// Resolve address
			if (udp_dns_filladdr(dj->addrbuf, atoi(dj->portbuf), &dj->host_addr) < 0)
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
				
				memcpy((struct _sockaddr *)&Netgame.players[0].protocol.udp.addr, (struct _sockaddr *)&dj->host_addr, sizeof(struct _sockaddr));
				
				dj->connecting = 1;
				items[6].text = connecting_txt;
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

	MALLOC(dj, direct_join, 1);
	if (!dj)
		return;
	dj->connecting = 0;
	dj->addrbuf[0] = '\0';
	dj->portbuf[0] = '\0';
	
	net_udp_init();

	memset(&dj->addrbuf,'\0', sizeof(char)*128);
	snprintf(dj->addrbuf, sizeof(char)*(strlen(GameArg.MplUdpHostAddr)+1), "%s", GameArg.MplUdpHostAddr);

	if (GameArg.MplUdpHostPort != 0)
		snprintf(dj->portbuf, sizeof(dj->portbuf), "%d", GameArg.MplUdpHostPort);
	else
		snprintf(dj->portbuf, sizeof(dj->portbuf), "%d", UDP_PORT_DEFAULT);

	if (GameArg.MplUdpMyPort != 0)
		snprintf (UDP_MyPort, sizeof(UDP_MyPort), "%d", GameArg.MplUdpMyPort);
	else
		snprintf (UDP_MyPort, sizeof(UDP_MyPort), "%d", UDP_PORT_DEFAULT);

	nitems = 0;
	
	m[nitems].type = NM_TYPE_TEXT;  m[nitems].text="GAME ADDRESS OR HOSTNAME:";     	nitems++;
	m[nitems].type = NM_TYPE_INPUT; m[nitems].text=dj->addrbuf; m[nitems].text_len=128; 	nitems++;
	m[nitems].type = NM_TYPE_TEXT;  m[nitems].text="GAME PORT:";                    	nitems++;
	m[nitems].type = NM_TYPE_INPUT; m[nitems].text=dj->portbuf; m[nitems].text_len=5;   	nitems++;
	m[nitems].type = NM_TYPE_TEXT;  m[nitems].text="MY PORT:";	                    	nitems++;
	m[nitems].type = NM_TYPE_INPUT; m[nitems].text=UDP_MyPort; m[nitems].text_len=5;	nitems++;
	m[nitems].type = NM_TYPE_TEXT;  m[nitems].text=blank;								nitems++;	// for connecting_txt

	newmenu_do1( NULL, "ENTER GAME ADDRESS", nitems, m, (int (*)(newmenu *, d_event *, void *))manual_join_game_handler, dj, 0 );
}

static char *ljtext;

int net_udp_list_join_poll( newmenu *menu, d_event *event, direct_join *dj )
{
	// Polling loop for Join Game menu
	int i, newpage = 0;
	static int NLPage = 0;
	newmenu_item *menus = newmenu_get_items(menu);
	int citem = newmenu_get_citem(menu);

	switch (event->type)
	{
		case EVENT_WINDOW_ACTIVATED:
		{
			Netgame.protocol.udp.valid = 0;
			memset(Active_udp_games, 0, sizeof(UDP_netgame_info_lite)*UDP_MAX_NETGAMES);
			num_active_udp_changed = 1;
			num_active_udp_games = 0;
			net_udp_request_game_info(GBcast, 1);
#ifdef IPv6
			net_udp_request_game_info(GMcast_v6, 1);
#endif
#ifdef USE_TRACKER
			udp_tracker_reqgames();
#endif
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
				memset(Active_udp_games, 0, sizeof(UDP_netgame_info_lite)*UDP_MAX_NETGAMES);
				num_active_udp_changed = 1;
				num_active_udp_games = 0;
				
				// Request LAN games
				net_udp_request_game_info(GBcast, 1);
#ifdef IPv6
				net_udp_request_game_info(GMcast_v6, 1);
#endif
				
#ifdef USE_TRACKER
				udp_tracker_reqgames();
#endif
				// All done
				break;
			}
			if (key == KEY_F5)
			{
				memset(Active_udp_games, 0, sizeof(UDP_netgame_info_lite)*UDP_MAX_NETGAMES);
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
				memset( Active_udp_games, 0, sizeof( UDP_netgame_info_lite ) * UDP_MAX_NETGAMES );
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
			if (((citem+(NLPage*UDP_NETGAMES_PPAGE)) >= 4) && (((citem+(NLPage*UDP_NETGAMES_PPAGE))-4) <= num_active_udp_games-1))
			{
				multi_new_game();
				N_players = 0;
				change_playernum_to(1);
				dj->start_time = timer_query();
				dj->last_time = 0;
				memcpy((struct _sockaddr *)&dj->host_addr, (struct _sockaddr *)&Active_udp_games[(citem+(NLPage*UDP_NETGAMES_PPAGE))-4].game_addr, sizeof(struct _sockaddr));
				memcpy((struct _sockaddr *)&Netgame.players[0].protocol.udp.addr, (struct _sockaddr *)&dj->host_addr, sizeof(struct _sockaddr));
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
	for (i = 0; i < UDP_NETGAMES_PPAGE; i++)
	{
		int game_status = Active_udp_games[(i+(NLPage*UDP_NETGAMES_PPAGE))].game_status;
		int j,x, k,tx,ty,ta,nplayers = 0;
		char levelname[8],MissName[25],GameName[25],thold[2],status[8];
		thold[1]=0;

		if ((i+(NLPage*UDP_NETGAMES_PPAGE)) >= num_active_udp_games)
		{
			snprintf(menus[i+4].text, sizeof(char)*74, "%d.                                                                      ",(i+(NLPage*UDP_NETGAMES_PPAGE))+1);
			continue;
		}

		// These next two loops protect against menu skewing
		// if missiontitle or gamename contain a tab

		for (x=0,tx=0,k=0,j=0;j<15;j++)
		{
			if (Active_udp_games[(i+(NLPage*UDP_NETGAMES_PPAGE))].mission_title[j]=='\t')
				continue;
			thold[0]=Active_udp_games[(i+(NLPage*UDP_NETGAMES_PPAGE))].mission_title[j];
			gr_get_string_size (thold,&tx,&ty,&ta);

			if ((x+=tx)>=FSPACX(55))
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

			if ((x+=tx)>=FSPACX(55))
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

		if (game_status == NETSTAT_STARTING)
			snprintf(status, sizeof(status), "FORMING ");
		else if (game_status == NETSTAT_PLAYING)
		{
			if (Active_udp_games[(i+(NLPage*UDP_NETGAMES_PPAGE))].RefusePlayers)
				snprintf(status, sizeof(status), "RESTRICT");
			else if (Active_udp_games[(i+(NLPage*UDP_NETGAMES_PPAGE))].game_flags & NETGAME_FLAG_CLOSED)
				snprintf(status, sizeof(status), "CLOSED  ");
			else
				snprintf(status, sizeof(status), "OPEN    ");
		}
		else
			snprintf(status, sizeof(status), "BETWEEN ");
		
		snprintf (menus[i+4].text,sizeof(char)*74,"%d.\t%s \t%s \t  %d/%d \t%s \t %s \t%s",(i+(NLPage*UDP_NETGAMES_PPAGE))+1,GameName,GMNamesShrt[Active_udp_games[(i+(NLPage*UDP_NETGAMES_PPAGE))].gamemode],nplayers, Active_udp_games[(i+(NLPage*UDP_NETGAMES_PPAGE))].max_numplayers,MissName,levelname,status);
			
		Assert(strlen(menus[i+4].text) < 75);
	}
	return 0;
}

void net_udp_list_join_game()
{
	int i = 0;
	newmenu_item *m;
	direct_join *dj;

	MALLOC(m, newmenu_item, ((UDP_NETGAMES_PPAGE+4)*2)+1);
	if (!m)
		return;
	MALLOC(ljtext, char, (((UDP_NETGAMES_PPAGE+4)*2)+1)*74);
	if (!ljtext)
	{
		d_free(m);
		return;
	}
	MALLOC(dj, direct_join, 1);
	if (!dj)
		return;
	dj->connecting = 0;
	dj->addrbuf[0] = '\0';
	dj->portbuf[0] = '\0';

	net_udp_init();
	if (udp_open_socket(0, GameArg.MplUdpMyPort != 0?GameArg.MplUdpMyPort:UDP_PORT_DEFAULT) < 0)
		return;

	if (GameArg.MplUdpMyPort != 0)
		if (udp_open_socket(1, UDP_PORT_DEFAULT) < 0)
			nm_messagebox(TXT_WARNING, 1, TXT_OK, "Cannot open default port!\nYou can only scan for games\nmanually.");

	// prepare broadcast address to discover games
	memset(&GBcast, '\0', sizeof(struct _sockaddr));
	udp_dns_filladdr(UDP_BCAST_ADDR, UDP_PORT_DEFAULT, &GBcast);
#ifdef IPv6
	memset(&GMcast_v6, '\0', sizeof(struct _sockaddr));
	udp_dns_filladdr(UDP_MCASTv6_ADDR, UDP_PORT_DEFAULT, &GMcast_v6);
#endif

	change_playernum_to(1);
	N_players = 0;
	Network_send_objects = 0;
	Network_sending_extras=0;
	Network_rejoined=0;

	Network_status = NETSTAT_BROWSING; // We are looking at a game menu

	net_udp_flush();
	net_udp_listen();  // Throw out old info

	num_active_udp_games = 0;

	memset(m, 0, sizeof(newmenu_item)*(UDP_NETGAMES_PPAGE+2));
	memset(Active_udp_games, 0, sizeof(UDP_netgame_info_lite)*UDP_MAX_NETGAMES);

	gr_set_fontcolor(BM_XRGB(15,15,23),-1);

	m[0].text = ljtext;
	m[0].type = NM_TYPE_TEXT;
	snprintf( m[0].text, sizeof(char)*74, "\tF4/F5/F6: (Re)Scan for all/LAN/Tracker Games." );
	m[1].text = ljtext + 74*1;
	m[1].type = NM_TYPE_TEXT;
	snprintf( m[1].text, sizeof(char)*74, "\tPgUp/PgDn: Flip Pages." );
	m[2].text = ljtext + 74*2;
	m[2].type = NM_TYPE_TEXT;
	snprintf( m[2].text, sizeof(char)*74, " " );
	m[3].text = ljtext + 74*3;
	m[3].type = NM_TYPE_TEXT;
	snprintf (m[3].text, sizeof(char)*74, "\tGAME \tMODE \t#PLYRS \tMISSION \tLEV \tSTATUS");

	for (i = 0; i < UDP_NETGAMES_PPAGE; i++) {
		m[i+4].text = ljtext + 74 * (i+4);
		m[i+4].type = NM_TYPE_MENU;
		snprintf(m[i+4].text,sizeof(char)*74,"%d.                                                                      ", i+1);
	}

	num_active_udp_changed = 1;
	newmenu_dotiny("NETGAMES", NULL,(UDP_NETGAMES_PPAGE+4), m, 1, (int (*)(newmenu *, d_event *, void *))net_udp_list_join_poll, dj);
}

void net_udp_send_sequence_packet(UDP_sequence_packet seq, struct _sockaddr recv_addr)
{
	int len = 0;
	ubyte buf[UPID_SEQUENCE_SIZE];

	len = 0;
	memset(buf, 0, sizeof(buf));
	buf[0] = seq.type;						len++;
	memcpy(&buf[len], seq.player.callsign, CALLSIGN_LEN+1);		len += CALLSIGN_LEN+1;
	buf[len] = seq.player.connected;				len++;
	buf[len] = seq.player.rank;					len++;
	
	dxx_sendto (UDP_Socket[0], buf, len, 0, (struct sockaddr *)&recv_addr, sizeof(struct _sockaddr));
}

void net_udp_receive_sequence_packet(ubyte *data, UDP_sequence_packet *seq, struct _sockaddr sender_addr)
{
	int len = 0;
	
	seq->type = data[0];						len++;
	memcpy(seq->player.callsign, &(data[len]), CALLSIGN_LEN+1);	len += CALLSIGN_LEN+1;
	seq->player.connected = data[len];				len++;
	memcpy (&(seq->player.rank),&(data[len]),1);			len++;
	
	if (multi_i_am_master())
		memcpy(&seq->player.protocol.udp.addr, (struct _sockaddr *)&sender_addr, sizeof(struct _sockaddr));
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

	if( UDP_Socket[0] != -1 )
		udp_close_socket(0);
	if( UDP_Socket[1] != -1 )
		udp_close_socket(1);

	memset(&Netgame, 0, sizeof(netgame_info));
	memset(&UDP_Seq, 0, sizeof(UDP_sequence_packet));
	memset(&UDP_MData, 0, sizeof(UDP_mdata_info));
	net_udp_noloss_init_mdata_queue();
	UDP_Seq.type = UPID_REQUEST;
	memcpy(UDP_Seq.player.callsign, Players[Player_num].callsign, CALLSIGN_LEN+1);

	multi_new_game();
	net_udp_flush();
	
#ifdef USE_TRACKER
	// Initialize the tracker info
	udp_tracker_init();
#endif
}

void net_udp_close()
{
#ifdef _WIN32
	WSACleanup();
#endif

	if( UDP_Socket[0] != -1 )
		udp_close_socket(0);
	if( UDP_Socket[1] != -1 )
		udp_close_socket(1);
	if( UDP_Socket[2] != -1 )
		udp_close_socket(2);
}

// Send PID_ENDLEVEL in regular intervals and listen for them (host also does the packets for playing clients)
int net_udp_kmatrix_poll1( newmenu *menu, d_event *event, void *userdata )
{
	// Polling loop for End-of-level menu
	if (event->type != EVENT_WINDOW_DRAW)
		return 0;
	
	menu = menu;
	userdata = userdata;

	net_udp_do_frame(0, 1);
	
	return 0;
}

// Same as above but used when player pressed ESC during kmatrix (host also does the packets for playing clients)
extern fix64 StartAbortMenuTime;
int net_udp_kmatrix_poll2( newmenu *menu, d_event *event, void *userdata )
{
	int rval = 0;

	// Polling loop for End-of-level menu
	if (event->type != EVENT_WINDOW_DRAW)
		return 0;
	
	menu = menu;
	userdata = userdata;
	
	if (timer_query() > (StartAbortMenuTime+(F1_0*3)))
		rval = -2;

	net_udp_do_frame(0, 1);
	
	return rval;
}

int net_udp_endlevel(int *secret)
{
	// Do whatever needs to be done between levels

	int i;

	// We do not really check if a player has actually found a secret level... yeah, I am too lazy! So just go there and pretend we did!
	for (i = 0; i < N_secret_levels; i++)
	{
		if (Current_level_num == Secret_level_table[i])
		{
			*secret = 1;
			break;
		}
	}

	Network_status = NETSTAT_ENDLEVEL; // We are between levels
	net_udp_listen();
	net_udp_send_endlevel_packet();

	for (i=0; i<N_players; i++) 
	{
		Netgame.players[i].LastPacketTime = timer_query();
	}
   
	net_udp_send_endlevel_packet();
	net_udp_send_endlevel_packet();

	net_udp_update_netgame();

	return(0);
}

int 
net_udp_can_join_netgame(netgame_info *game)
{
	// Can this player rejoin a netgame in progress?

	int i, num_players;

	if (game->game_status == NETSTAT_STARTING)
		return 1;

	if (game->game_status != NETSTAT_PLAYING)
		return 0;

	// Game is in progress, figure out if this guy can re-join it

	num_players = game->numplayers;

	if (!(game->game_flags & NETGAME_FLAG_CLOSED)) {
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

	for (i = 0; i < num_players; i++)
		if ( (!strcasecmp(Players[Player_num].callsign, game->players[i].callsign)) && game->players[i].protocol.udp.isyou )
			break;

	if (i != num_players)
		return 1;

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

	net_udp_noloss_clear_mdata_got(playernum);
}

void
net_udp_new_player(UDP_sequence_packet *their)
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

	memcpy(Players[pnum].callsign, their->player.callsign, CALLSIGN_LEN+1);
	memcpy(Netgame.players[pnum].callsign, their->player.callsign, CALLSIGN_LEN+1);
	memcpy(&Netgame.players[pnum].protocol.udp.addr, &their->player.protocol.udp.addr, sizeof(struct _sockaddr));

	Players[pnum].connected = CONNECT_PLAYING;
	Players[pnum].net_kills_total = 0;
	Players[pnum].net_killed_total = 0;
	memset(kill_matrix[pnum], 0, MAX_PLAYERS*sizeof(short)); 
	Players[pnum].score = 0;
	Players[pnum].flags = 0;
	Players[pnum].KillGoalCount=0;

	if (pnum == N_players)
	{
		N_players++;
		Netgame.numplayers = N_players;
	}

	digi_play_sample(SOUND_HUD_MESSAGE, F1_0);

	HUD_init_message(HM_MULTI, "'%s' %s",their->player.callsign, TXT_JOINING);
	
	multi_make_ghost_player(pnum);

	multi_send_score();

	net_udp_noloss_clear_mdata_got(pnum);
}

void net_udp_welcome_player(UDP_sequence_packet *their)
{
 	// Add a player to a game already in progress
	int player_num;
	int i;

	// Don't accept new players if we're ending this level.  Its safe to
	// ignore since they'll request again later

	if ((Endlevel_sequence) || (Control_center_destroyed))
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

	if (their->player.connected != Current_level_num)
	{
		net_udp_dump_player(their->player.protocol.udp.addr, DUMP_LEVEL);
		return;
	}

	player_num = -1;
	memset(&UDP_sync_player, 0, sizeof(UDP_sequence_packet));
	Network_player_added = 0;

	for (i = 0; i < N_players; i++)
	{
		if ((!strcasecmp(Players[i].callsign, their->player.callsign )) && !memcmp((struct _sockaddr *)&their->player.protocol.udp.addr, (struct _sockaddr *)&Netgame.players[i].protocol.udp.addr, sizeof(struct _sockaddr)))
		{
			player_num = i;
			break;
		}
	}

	if (player_num == -1)
	{
		// Player is new to this game

		if ( !(Netgame.game_flags & NETGAME_FLAG_CLOSED) && (N_players < Netgame.max_numplayers))
		{
			// Add player in an open slot, game not full yet

			player_num = N_players;
			Network_player_added = 1;
		}
		else if (Netgame.game_flags & NETGAME_FLAG_CLOSED)
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

			for (i = 0; i < Netgame.numplayers; i++)
				if (Netgame.players[i].connected)
					activeplayers++;

			if (activeplayers == Netgame.max_numplayers)
			{
				// Game is full.
				net_udp_dump_player(their->player.protocol.udp.addr, DUMP_FULL);
				return;
			}

			for (i = 0; i < N_players; i++)
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

		HUD_init_message(HM_MULTI, "'%s' %s", Players[player_num].callsign, TXT_REJOIN);

		multi_send_score();

		net_udp_noloss_clear_mdata_got(player_num);
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

int net_udp_objnum_is_past(int objnum)
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

void net_udp_send_door_updates(void)
{
	// Send door status when new player joins
	
	int i;

	for (i = 0; i < Num_walls; i++)
	{
		if ((Walls[i].type == WALL_DOOR) && ((Walls[i].state == WALL_DOOR_OPENING) || (Walls[i].state == WALL_DOOR_WAITING)))
			multi_send_door_open(Walls[i].segnum, Walls[i].sidenum);
		else if ((Walls[i].type == WALL_BLASTABLE) && (Walls[i].flags & WALL_BLASTED))
			multi_send_door_open(Walls[i].segnum, Walls[i].sidenum);
		else if ((Walls[i].type == WALL_BLASTABLE) && (Walls[i].hps != WALL_HPS))
			multi_send_hostage_door_status(i);
	}

}	

void net_udp_process_monitor_vector(int vector)
{
	int i, j;
	int count = 0;
	segment *seg;
	
	for (i=0; i <= Highest_segment_index; i++)
	{
		int tm, ec, bm;
		seg = &Segments[i];
		for (j = 0; j < 6; j++)
		{
			if ( ((tm = seg->sides[j].tmap_num2) != 0) &&
				  ((ec = TmapInfo[tm&0x3fff].eclip_num) != -1) &&
 				  ((bm = Effects[ec].dest_bm_num) != -1) )
			{
				if (vector & (1 << count))
				{
					seg->sides[j].tmap_num2 = bm | (tm&0xc000);
				}
				count++;
				Assert(count < 32);
			}
		}
	}
}

int net_udp_create_monitor_vector(void)
{
	int i, j, k;
	int num_blown_bitmaps = 0;
	int monitor_num = 0;
	int blown_bitmaps[7];
	int vector = 0;
	segment *seg;

	for (i=0; i < Num_effects; i++)
	{
		if (Effects[i].dest_bm_num > 0) {
			for (j = 0; j < num_blown_bitmaps; j++)
				if (blown_bitmaps[j] == Effects[i].dest_bm_num)
					break;
			if (j == num_blown_bitmaps)
				blown_bitmaps[num_blown_bitmaps++] = Effects[i].dest_bm_num;
		}
	}		
		

	Assert(num_blown_bitmaps <= 7);

	for (i=0; i <= Highest_segment_index; i++)
	{
		int tm, ec;
		seg = &Segments[i];
		for (j = 0; j < 6; j++)
		{
			if ((tm = seg->sides[j].tmap_num2) != 0) 
			{
				if ( ((ec = TmapInfo[tm&0x3fff].eclip_num) != -1) &&
 					  (Effects[ec].dest_bm_num != -1) )
				{
					monitor_num++;
					Assert(monitor_num < 32);
				}
				else
				{
					for (k = 0; k < num_blown_bitmaps; k++)
					{
						if ((tm&0x3fff) == blown_bitmaps[k])
						{
							vector |= (1 << monitor_num);
							monitor_num++;
							Assert(monitor_num < 32);
							break;
						}
					}
				}
			}
		}
	}
	return(vector);
}

void net_udp_stop_resync(UDP_sequence_packet *their)
{
	if ( (!memcmp((struct _sockaddr *)&UDP_sync_player.player.protocol.udp.addr, (struct _sockaddr *)&their->player.protocol.udp.addr, sizeof(struct _sockaddr))) &&
		(!stricmp(UDP_sync_player.player.callsign, their->player.callsign)) )
	{
		Network_send_objects = 0;
		Network_sending_extras=0;
		Network_rejoined=0;
		Player_joining_extras=-1;
		Network_send_objnum = -1;
	}
}

ubyte object_buffer[UPID_MAX_SIZE];

void net_udp_send_objects(void)
{
	sbyte owner, player_num = UDP_sync_player.player.connected;
	static int obj_count = 0;
	int loc = 0, i = 0, remote_objnum = 0, obj_count_frame = 0;
	static fix64 last_send_time = 0;
	
	if (last_send_time + (F1_0/50) > timer_query())
		return;
	last_send_time = timer_query();

	// Send clear objects array trigger and send player num

	Assert(Network_send_objects != 0);
	Assert(player_num >= 0);
	Assert(player_num < Netgame.max_numplayers);

	if (Endlevel_sequence || Control_center_destroyed)
	{
		// Endlevel started before we finished sending the goods, we'll
		// have to stop and try again after the level.
		net_udp_dump_player(UDP_sync_player.player.protocol.udp.addr, DUMP_ENDLEVEL);
		Network_send_objects = 0; 
		return;
	}

	memset(object_buffer, 0, UPID_MAX_SIZE);
	object_buffer[0] = UPID_OBJECT_DATA;
	loc = 5;

	if (Network_send_objnum == -1)
	{
		obj_count = 0;
		Network_send_object_mode = 0;
		PUT_INTEL_INT(object_buffer+loc, -1);                       loc += 4;
		object_buffer[loc] = player_num;                            loc += 1;
		/* Placeholder for remote_objnum, not used here */          loc += 4;
		Network_send_objnum = 0;
		obj_count_frame = 1;
	}
	
	for (i = Network_send_objnum; i <= Highest_object_index; i++)
	{
		if ((Objects[i].type != OBJ_POWERUP) && (Objects[i].type != OBJ_PLAYER) &&
				(Objects[i].type != OBJ_CNTRLCEN) && (Objects[i].type != OBJ_GHOST) &&
				(Objects[i].type != OBJ_ROBOT) && (Objects[i].type != OBJ_HOSTAGE))
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

		PUT_INTEL_INT(object_buffer+loc, i);                        loc += 4;
		object_buffer[loc] = owner;                                 loc += 1;
		PUT_INTEL_INT(object_buffer+loc, remote_objnum);            loc += 4;
		// use object_rw to send objects for now. if object sometime contains some day contains something useful the client should know about, we should use it. but by now it's also easier to use object_rw because then we also do not need fix64 timer values.
		multi_object_to_object_rw(&Objects[i], (object_rw *)&object_buffer[loc]);
#ifdef WORDS_BIGENDIAN
		object_rw_swap((object_rw *)&object_buffer[loc], 1);
#endif
		loc += sizeof(object_rw);
	}

	if (obj_count_frame) // Send any objects we've buffered
	{
		Network_send_objnum = i;
		PUT_INTEL_INT(object_buffer+1, obj_count_frame);

		Assert(loc <= UPID_MAX_SIZE);

		dxx_sendto (UDP_Socket[0], object_buffer, loc, 0, (struct sockaddr *)&UDP_sync_player.player.protocol.udp.addr, sizeof(struct _sockaddr));
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
			PUT_INTEL_INT(object_buffer+1, 1);
			PUT_INTEL_INT(object_buffer+5, -2);
			object_buffer[9] = player_num;
			PUT_INTEL_INT(object_buffer+10, obj_count);
			dxx_sendto (UDP_Socket[0], object_buffer, 14, 0, (struct sockaddr *)&UDP_sync_player.player.protocol.udp.addr, sizeof(struct _sockaddr));

			// Send sync packet which tells the player who he is and to start!
			net_udp_send_rejoin_sync(player_num);

			// Turn off send object mode
			Network_send_objnum = -1;
			Network_send_objects = 0;
			obj_count = 0;

			Network_sending_extras=3; // start to send extras
			VerifyPlayerJoined = Player_joining_extras = player_num;

			return;
		} // mode == 1;
	} // i > Highest_object_index
}

int net_udp_verify_objects(int remote, int local)
{
	int i, nplayers = 0;

	if ((remote-local) > 10)
		return(2);

	for (i = 0; i <= Highest_object_index; i++)
	{
		if ((Objects[i].type == OBJ_PLAYER) || (Objects[i].type == OBJ_GHOST))
			nplayers++;
	}

	if (Netgame.max_numplayers<=nplayers)
		return(0);

	return(1);
}

void net_udp_read_object_packet( ubyte *data )
{
	// Object from another net player we need to sync with
	object *obj;
	sbyte obj_owner;
	static int mode = 0, object_count = 0, my_pnum = 0;
	int i = 0, segnum = 0, objnum = 0, remote_objnum = 0, nobj = 0, loc = 5;
	
	nobj = GET_INTEL_INT(data + 1);

	for (i = 0; i < nobj; i++)
	{
		objnum = GET_INTEL_INT(data + loc);                         loc += 4;
		obj_owner = data[loc];                                      loc += 1;
		remote_objnum = GET_INTEL_INT(data + loc);                  loc += 4;

		if (objnum == -1) 
		{
			// Clear object array
			init_objects();
			Network_rejoined = 1;
			my_pnum = obj_owner;
			change_playernum_to(my_pnum);
			mode = 1;
			object_count = 0;
		}
		else if (objnum == -2)
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
			if (objnum != -1) {
				obj = &Objects[objnum];
				if (obj->segnum != -1)
					obj_unlink(objnum);
				Assert(obj->segnum == -1);
				Assert(objnum < MAX_OBJECTS);
#ifdef WORDS_BIGENDIAN
				object_rw_swap((object_rw *)&data[loc], 1);
#endif
				multi_object_rw_to_object((object_rw *)&data[loc], obj);
				loc += sizeof(object_rw);
				segnum = obj->segnum;
				obj->next = obj->prev = obj->segnum = -1;
				obj->attached_obj = -1;
				if (segnum > -1)
					obj_link(obj-Objects,segnum);
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
	int i, j;

	Players[player_num].connected = CONNECT_PLAYING; // connect the new guy
	Netgame.players[player_num].LastPacketTime = timer_query();

	if (Endlevel_sequence || Control_center_destroyed)
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

		for (i = 0; i < N_players; i++)
		{
			if ((i != player_num) && (i != Player_num) && (Players[i].connected))
				net_udp_send_sequence_packet( UDP_sync_player, Netgame.players[i].protocol.udp.addr);
		}
	}

	// Send sync packet to the new guy

	net_udp_update_netgame();

	// Fill in the kill list
	for (j=0; j<MAX_PLAYERS; j++)
	{
		for (i=0; i<MAX_PLAYERS;i++)
			Netgame.kills[j][i] = kill_matrix[j][i];
		Netgame.killed[j] = Players[j].net_killed_total;
		Netgame.player_kills[j] = Players[j].net_kills_total;
		Netgame.player_score[j] = Players[j].score;
	}	

	Netgame.level_time = Players[Player_num].time_level;
	Netgame.monitor_vector = net_udp_create_monitor_vector();

	net_udp_send_game_info(UDP_sync_player.player.protocol.udp.addr, UPID_SYNC);
	net_udp_send_door_updates();

	return;
}

void net_udp_resend_sync_due_to_packet_loss()
{
	int i,j;

	if (!multi_i_am_master())
		return;

	net_udp_update_netgame();

	// Fill in the kill list
	for (j=0; j<MAX_PLAYERS; j++)
	{
		for (i=0; i<MAX_PLAYERS;i++)
			Netgame.kills[j][i] = kill_matrix[j][i];
		Netgame.killed[j] = Players[j].net_killed_total;
		Netgame.player_kills[j] = Players[j].net_kills_total;
		Netgame.player_score[j] = Players[j].score;
	}

	Netgame.level_time = Players[Player_num].time_level;
	Netgame.monitor_vector = net_udp_create_monitor_vector();

	net_udp_send_game_info(UDP_sync_player.player.protocol.udp.addr, UPID_SYNC);
}

char * net_udp_get_player_name( int objnum )
{
	if ( objnum < 0 ) return NULL; 
	if ( Objects[objnum].type != OBJ_PLAYER ) return NULL;
	if ( Objects[objnum].id >= MAX_PLAYERS ) return NULL;
	if ( Objects[objnum].id >= N_players ) return NULL;
	
	return Players[Objects[objnum].id].callsign;
}


void net_udp_add_player(UDP_sequence_packet *p)
{
	int i;

	for (i=0; i<N_players; i++ )
	{
		if ( !memcmp( (struct _sockaddr *)&Netgame.players[i].protocol.udp.addr, (struct _sockaddr *)&p->player.protocol.udp.addr, sizeof(struct _sockaddr)))
		{
			Netgame.players[i].LastPacketTime = timer_query();
			return;		// already got them
		}
	}

	if ( N_players >= MAX_PLAYERS )
	{
		return;		// too many of em
	}

	memcpy( Netgame.players[N_players].callsign, p->player.callsign, CALLSIGN_LEN+1 );
	memcpy( (struct _sockaddr *)&Netgame.players[N_players].protocol.udp.addr, (struct _sockaddr *)&p->player.protocol.udp.addr, sizeof(struct _sockaddr) );
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

void net_udp_remove_player(UDP_sequence_packet *p)
{
	int i,pn;
	
	pn = -1;
	for (i=0; i<N_players; i++ )
	{
		if (!memcmp((struct _sockaddr *)&Netgame.players[i].protocol.udp.addr, (struct _sockaddr *)&p->player.protocol.udp.addr, sizeof(struct _sockaddr)))
		{
			pn = i;
			break;
		}
	}
	
	if (pn < 0 )
		return;

	for (i=pn; i<N_players-1; i++ )
	{
		memcpy( Netgame.players[i].callsign, Netgame.players[i+1].callsign, CALLSIGN_LEN+1 );
		memcpy( (struct _sockaddr *)&Netgame.players[i].protocol.udp.addr, (struct _sockaddr *)&Netgame.players[i+1].protocol.udp.addr, sizeof(struct _sockaddr) );
		Netgame.players[i].rank=Netgame.players[i+1].rank;
	}
		
	N_players--;
	Netgame.numplayers = N_players;

	net_udp_send_netgame_update();
}

void net_udp_dump_player(struct _sockaddr dump_addr, int why)
{
	// Inform player that he was not chosen for the netgame

	ubyte buf[UPID_DUMP_SIZE];
	int i;
	
	buf[0] = UPID_DUMP;
	buf[1] = why;
	
	dxx_sendto (UDP_Socket[0], buf, sizeof(buf), 0, (struct sockaddr *)&dump_addr, sizeof(struct _sockaddr));

	if (multi_i_am_master())
		for (i = 1; i < N_players; i++)
			if (!memcmp((struct _sockaddr *)&dump_addr, (struct _sockaddr *)&Netgame.players[i].protocol.udp.addr, sizeof(struct _sockaddr)))
				multi_disconnect_player(i);
}

void net_udp_update_netgame(void)
{
	// Update the netgame struct with current game variables

	int i, j;
	
	Netgame.numconnected=0;
	for (i=0;i<N_players;i++)
		if (Players[i].connected)
			Netgame.numconnected++;
	
	if (Network_status == NETSTAT_STARTING)
		return;

	Netgame.numplayers = N_players;
	Netgame.game_status = Network_status;

	for (i = 0; i < MAX_PLAYERS; i++) 
	{
		Netgame.players[i].connected = Players[i].connected;
		for(j = 0; j < MAX_PLAYERS; j++)
			Netgame.kills[i][j] = kill_matrix[i][j];
		Netgame.killed[i] = Players[i].net_killed_total;
		Netgame.player_kills[i] = Players[i].net_kills_total;
	}

	Netgame.team_kills[0] = team_kills[0];
	Netgame.team_kills[1] = team_kills[1];
	Netgame.levelnum = Current_level_num;
}

/* Send an updated endlevel status to everyone (if we are host) or host (if we are client)  */
void net_udp_send_endlevel_packet(void)
{
	int i = 0, j = 0, len = 0;

	if (multi_i_am_master())
	{
		ubyte buf[UPID_MAX_SIZE];

		memset(buf, 0, sizeof(buf));

		buf[len] = UPID_ENDLEVEL_H;											len++;
		buf[len] = Countdown_seconds_left;									len++;

		for (i = 0; i < MAX_PLAYERS; i++)
		{
			buf[len] = Players[i].connected;								len++;
			PUT_INTEL_SHORT(buf + len, Players[i].net_kills_total);			len += 2;
			PUT_INTEL_SHORT(buf + len, Players[i].net_killed_total);		len += 2;
		}

		for (i = 0; i < MAX_PLAYERS; i++)
		{
			for (j = 0; j < MAX_PLAYERS; j++)
			{
				PUT_INTEL_SHORT(buf + len, kill_matrix[i][j]);				len += 2;
			}
		}

		for (i = 1; i < MAX_PLAYERS; i++)
			dxx_sendto (UDP_Socket[0], buf, len, 0, (struct sockaddr *)&Netgame.players[i].protocol.udp.addr, sizeof(struct _sockaddr));
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

		for (i = 0; i < MAX_PLAYERS; i++)
		{
			PUT_INTEL_SHORT(buf + len, kill_matrix[Player_num][i]);			len += 2;
		}

		dxx_sendto (UDP_Socket[0], buf, len, 0, (struct sockaddr *)&Netgame.players[0].protocol.udp.addr, sizeof(struct _sockaddr));
	}
}

void net_udp_send_version_deny(struct _sockaddr sender_addr)
{
	ubyte buf[UPID_VERSION_DENY_SIZE];
	
	buf[0] = UPID_VERSION_DENY;
	PUT_INTEL_SHORT(buf + 1, D1XMAJORi);
	PUT_INTEL_SHORT(buf + 3, D1XMINORi);
	PUT_INTEL_SHORT(buf + 5, D1XMICROi);
	PUT_INTEL_SHORT(buf + 7, MULTI_PROTO_VERSION);
	
	dxx_sendto (UDP_Socket[0], buf, sizeof(buf), 0, (struct sockaddr *)&sender_addr, sizeof(struct _sockaddr));
}

void net_udp_process_version_deny(ubyte *data, struct _sockaddr sender_addr)
{
	Netgame.protocol.udp.program_iver[0] = GET_INTEL_SHORT(&data[1]);
	Netgame.protocol.udp.program_iver[1] = GET_INTEL_SHORT(&data[3]);
	Netgame.protocol.udp.program_iver[2] = GET_INTEL_SHORT(&data[5]);
	Netgame.protocol.udp.program_iver[3] = GET_INTEL_SHORT(&data[7]);
	Netgame.protocol.udp.valid = -1;
}

void net_udp_request_game_info(struct _sockaddr game_addr, int lite)
{
	ubyte buf[UPID_GAME_INFO_REQ_SIZE];
	
	buf[0] = (lite?UPID_GAME_INFO_LITE_REQ:UPID_GAME_INFO_REQ);
	memcpy(&(buf[1]), UDP_REQ_ID, 4);
	PUT_INTEL_SHORT(buf + 5, D1XMAJORi);
	PUT_INTEL_SHORT(buf + 7, D1XMINORi);
	PUT_INTEL_SHORT(buf + 9, D1XMICROi);
	if (!lite)
		PUT_INTEL_SHORT(buf + 11, MULTI_PROTO_VERSION);
	
	dxx_sendto (UDP_Socket[0], buf, sizeof(buf), 0, (struct sockaddr *)&game_addr, sizeof(struct _sockaddr));
}

// Check request for game info. Return 1 if sucessful; -1 if version mismatch; 0 if wrong game or some other error - do not process
int net_udp_check_game_info_request(ubyte *data, int lite)
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
	
	if ((sender_iver[0] != D1XMAJORi) || (sender_iver[1] != D1XMINORi) || (sender_iver[2] != D1XMICROi) || (!lite && sender_iver[3] != MULTI_PROTO_VERSION))
		return -1;
		
	return 1;
}

extern fix ThisLevelTime;

void net_udp_send_game_info(struct _sockaddr sender_addr, ubyte info_upid)
{
	// Send game info to someone who requested it

	int len = 0;
	
	net_udp_update_netgame(); // Update the values in the netgame struct
	
	if (info_upid == UPID_GAME_INFO_LITE)
	{
		ubyte buf[UPID_GAME_INFO_LITE_SIZE];
		int tmpvar = 0;

		memset(buf, 0, sizeof(buf));
		
		buf[0] = info_upid;								len++;
		PUT_INTEL_SHORT(buf + len, D1XMAJORi); 						len += 2;
		PUT_INTEL_SHORT(buf + len, D1XMINORi); 						len += 2;
		PUT_INTEL_SHORT(buf + len, D1XMICROi); 						len += 2;
		PUT_INTEL_INT(buf + len, Netgame.protocol.udp.GameID);				len += 4;
		memcpy(&(buf[len]), Netgame.game_name, NETGAME_NAME_LEN+1);			len += (NETGAME_NAME_LEN+1);
		memcpy(&(buf[len]), Netgame.mission_title, MISSION_NAME_LEN+1);			len += (MISSION_NAME_LEN+1);
		memcpy(&(buf[len]), Netgame.mission_name, 9);					len += 9;
		PUT_INTEL_INT(buf + len, Netgame.levelnum);					len += 4;
		buf[len] = Netgame.gamemode;							len++;
		buf[len] = Netgame.RefusePlayers;						len++;
		buf[len] = Netgame.difficulty;							len++;
		tmpvar = Netgame.game_status;
		if (Endlevel_sequence || Control_center_destroyed)
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
		buf[len] = Netgame.game_flags;							len++;
		
		dxx_sendto (UDP_Socket[0], buf, len, 0, (struct sockaddr *)&sender_addr, sizeof(struct _sockaddr));
	}
	else
	{
		ubyte buf[UPID_GAME_INFO_SIZE];
		int i = 0, j = 0, tmpvar = 0;
		
		memset(buf, 0, sizeof(buf));
	
		buf[0] = info_upid;								len++;
		PUT_INTEL_SHORT(buf + len, D1XMAJORi); 						len += 2;
		PUT_INTEL_SHORT(buf + len, D1XMINORi); 						len += 2;
		PUT_INTEL_SHORT(buf + len, D1XMICROi); 						len += 2;
		for (i = 0; i < MAX_PLAYERS+4; i++)
		{
			memcpy(&buf[len], Netgame.players[i].callsign, CALLSIGN_LEN+1); 	len += CALLSIGN_LEN+1;
			buf[len] = Netgame.players[i].connected;				len++;
			buf[len] = Netgame.players[i].rank;					len++;
			if (!memcmp((struct _sockaddr *)&sender_addr, (struct _sockaddr *)&Netgame.players[i].protocol.udp.addr, sizeof(struct _sockaddr)))
				buf[len] = 1;
			else
				buf[len] = 0;							len++;
		}
		memcpy(&(buf[len]), Netgame.game_name, NETGAME_NAME_LEN+1);			len += (NETGAME_NAME_LEN+1);
		memcpy(&(buf[len]), Netgame.mission_title, MISSION_NAME_LEN+1);			len += (MISSION_NAME_LEN+1);
		memcpy(&(buf[len]), Netgame.mission_name, 9);		            		len += 9;
		PUT_INTEL_INT(buf + len, Netgame.levelnum);					len += 4;
		buf[len] = Netgame.gamemode;							len++;
		buf[len] = Netgame.RefusePlayers;						len++;
		buf[len] = Netgame.difficulty;							len++;
		tmpvar = Netgame.game_status;
		if (Endlevel_sequence || Control_center_destroyed)
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
		buf[len] = Netgame.game_flags;							len++;
		buf[len] = Netgame.team_vector;							len++;
		PUT_INTEL_INT(buf + len, Netgame.AllowedItems);					len += 4;
		PUT_INTEL_SHORT(buf + len, Netgame.Allow_marker_view);				len += 2;
		PUT_INTEL_SHORT(buf + len, Netgame.AlwaysLighting);				len += 2;
		PUT_INTEL_SHORT(buf + len, Netgame.ShowEnemyNames);				len += 2;
		PUT_INTEL_SHORT(buf + len, Netgame.BrightPlayers);				len += 2;
		PUT_INTEL_SHORT(buf + len, Netgame.InvulAppear);				len += 2;
		memcpy(&buf[len], Netgame.team_name, 2*(CALLSIGN_LEN+1));			len += 2*(CALLSIGN_LEN+1);
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			PUT_INTEL_INT(buf + len, Netgame.locations[i]);				len += 4;
		}
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			for (j = 0; j < MAX_PLAYERS; j++)
			{
				PUT_INTEL_SHORT(buf + len, Netgame.kills[i][j]);		len += 2;
			}
		}
		PUT_INTEL_SHORT(buf + len, Netgame.segments_checksum);				len += 2;
		PUT_INTEL_SHORT(buf + len, Netgame.team_kills[0]);				len += 2;
		PUT_INTEL_SHORT(buf + len, Netgame.team_kills[1]);				len += 2;
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			PUT_INTEL_SHORT(buf + len, Netgame.killed[i]);				len += 2;
		}
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			PUT_INTEL_SHORT(buf + len, Netgame.player_kills[i]);			len += 2;
		}
		PUT_INTEL_INT(buf + len, Netgame.KillGoal);					len += 4;
		PUT_INTEL_INT(buf + len, Netgame.PlayTimeAllowed);				len += 4;
		PUT_INTEL_INT(buf + len, Netgame.level_time);					len += 4;
		PUT_INTEL_INT(buf + len, Netgame.control_invul_time);				len += 4;
		PUT_INTEL_INT(buf + len, Netgame.monitor_vector);				len += 4;
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			PUT_INTEL_INT(buf + len, Netgame.player_score[i]);			len += 4;
		}
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			buf[len] = Netgame.player_flags[i];					len++;
		}
		PUT_INTEL_SHORT(buf + len, Netgame.PacketsPerSec);				len += 2;
		buf[len] = Netgame.PacketLossPrevention;					len++;
		buf[len] = Netgame.NoFriendlyFire;						len++;

		dxx_sendto (UDP_Socket[0], buf, len, 0, (struct sockaddr *)&sender_addr, sizeof(struct _sockaddr));
	}
}

/* Send game info to all players in this game. Also send lite_info for people watching the netlist */
void net_udp_send_netgame_update()
{
	int i = 0;
	
	for (i=1; i<N_players; i++ )
	{
		net_udp_send_game_info(Netgame.players[i].protocol.udp.addr, UPID_GAME_INFO);
	}
	net_udp_send_game_info(GBcast, UPID_GAME_INFO_LITE);
#ifdef IPv6
	net_udp_send_game_info(GMcast_v6, UPID_GAME_INFO_LITE);
#endif
}

int net_udp_send_request(void)
{
	// Send a request to join a game 'Netgame'.  Returns 0 if we can join this
	// game, non-zero if there is some problem.
	int i;

	if (Netgame.numplayers < 1)
	 return 1;

	for (i = 0; i < MAX_PLAYERS; i++)
		if (Netgame.players[i].connected)
			break;

	Assert(i < MAX_PLAYERS);

	UDP_Seq.type = UPID_REQUEST;
	UDP_Seq.player.connected = Current_level_num;

	net_udp_send_sequence_packet(UDP_Seq, Netgame.players[0].protocol.udp.addr);

	return i;
}
	
void net_udp_process_game_info(ubyte *data, int data_len, struct _sockaddr game_addr, int lite_info)
{
	int len = 0, i = 0, j = 0;
	
	if (lite_info)
	{
		UDP_netgame_info_lite recv_game;
		
		memcpy(&recv_game, &game_addr, sizeof(struct _sockaddr));
												len++; // skip UPID byte
		recv_game.program_iver[0] = GET_INTEL_SHORT(&(data[len]));			len += 2;
		recv_game.program_iver[1] = GET_INTEL_SHORT(&(data[len]));			len += 2;
		recv_game.program_iver[2] = GET_INTEL_SHORT(&(data[len]));			len += 2;
		
		if ((recv_game.program_iver[0] != D1XMAJORi) || (recv_game.program_iver[1] != D1XMINORi) || (recv_game.program_iver[2] != D1XMICROi))
			return;

		recv_game.GameID = GET_INTEL_INT(&(data[len]));					len += 4;
		memcpy(&recv_game.game_name, &(data[len]), NETGAME_NAME_LEN+1);			len += (NETGAME_NAME_LEN+1);
		memcpy(&recv_game.mission_title, &(data[len]), MISSION_NAME_LEN+1);		len += (MISSION_NAME_LEN+1);
		memcpy(&recv_game.mission_name, &(data[len]), 9);				len += 9;
		recv_game.levelnum = GET_INTEL_INT(&(data[len]));				len += 4;
		recv_game.gamemode = data[len];							len++;
		recv_game.RefusePlayers = data[len];						len++;
		recv_game.difficulty = data[len];						len++;
		recv_game.game_status = data[len];						len++;
		recv_game.numconnected = data[len];						len++;
		recv_game.max_numplayers = data[len];						len++;
		recv_game.game_flags = data[len];						len++;
	
		num_active_udp_changed = 1;
		
		for (i = 0; i < num_active_udp_games; i++)
			if (!strcasecmp(Active_udp_games[i].game_name, recv_game.game_name) && Active_udp_games[i].GameID == recv_game.GameID)
				break;

		if (i == UDP_MAX_NETGAMES)
		{
			return;
		}
		
		memcpy(&Active_udp_games[i], &recv_game, sizeof(UDP_netgame_info_lite));
		
		if (i == num_active_udp_games)
			num_active_udp_games++;

		if (Active_udp_games[i].numconnected == 0)
		{
			// Delete this game
			for (j = i; j < num_active_udp_games-1; j++)
				memcpy(&Active_udp_games[j], &Active_udp_games[j+1], sizeof(UDP_netgame_info_lite));
			num_active_udp_games--;
		}
	}
	else
	{
		memcpy((struct _sockaddr *)&Netgame.players[0].protocol.udp.addr, (struct _sockaddr *)&game_addr, sizeof(struct _sockaddr));

												len++; // skip UPID byte
		Netgame.protocol.udp.program_iver[0] = GET_INTEL_SHORT(&(data[len]));		len += 2;
		Netgame.protocol.udp.program_iver[1] = GET_INTEL_SHORT(&(data[len]));		len += 2;
		Netgame.protocol.udp.program_iver[2] = GET_INTEL_SHORT(&(data[len]));		len += 2;
		for (i = 0; i < MAX_PLAYERS+4; i++)
		{
			memcpy(&Netgame.players[i].callsign, &(data[len]), CALLSIGN_LEN+1);	len += CALLSIGN_LEN+1;
			Netgame.players[i].connected = data[len];				len++;
			Netgame.players[i].rank = data[len];					len++;
			Netgame.players[i].protocol.udp.isyou = data[len];			len++;
		}
		memcpy(&Netgame.game_name, &(data[len]), NETGAME_NAME_LEN+1);			len += (NETGAME_NAME_LEN+1);
		memcpy(&Netgame.mission_title, &(data[len]), MISSION_NAME_LEN+1);		len += (MISSION_NAME_LEN+1);
		memcpy(&Netgame.mission_name, &(data[len]), 9);					len += 9;
		Netgame.levelnum = GET_INTEL_INT(&(data[len]));					len += 4;
		Netgame.gamemode = data[len];							len++;
		Netgame.RefusePlayers = data[len];						len++;
		Netgame.difficulty = data[len];							len++;
		Netgame.game_status = data[len];						len++;
		Netgame.numplayers = data[len];							len++;
		Netgame.max_numplayers = data[len];						len++;
		Netgame.numconnected = data[len];						len++;
		Netgame.game_flags = data[len];							len++;
		Netgame.team_vector = data[len];						len++;
		Netgame.AllowedItems = GET_INTEL_INT(&(data[len]));				len += 4;
		Netgame.Allow_marker_view = GET_INTEL_SHORT(&(data[len]));			len += 2;
		Netgame.AlwaysLighting = GET_INTEL_SHORT(&(data[len]));				len += 2;
		Netgame.ShowEnemyNames = GET_INTEL_SHORT(&(data[len]));				len += 2;
		Netgame.BrightPlayers = GET_INTEL_SHORT(&(data[len]));				len += 2;
		Netgame.InvulAppear = GET_INTEL_SHORT(&(data[len]));				len += 2;
		memcpy(Netgame.team_name, &(data[len]), 2*(CALLSIGN_LEN+1));			len += 2*(CALLSIGN_LEN+1);
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			Netgame.locations[i] = GET_INTEL_INT(&(data[len]));			len += 4;
		}
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			for (j = 0; j < MAX_PLAYERS; j++)
			{
				Netgame.kills[i][j] = GET_INTEL_SHORT(&(data[len]));		len += 2;
			}
		}
		Netgame.segments_checksum = GET_INTEL_SHORT(&(data[len]));			len += 2;
		Netgame.team_kills[0] = GET_INTEL_SHORT(&(data[len]));				len += 2;	
		Netgame.team_kills[1] = GET_INTEL_SHORT(&(data[len]));				len += 2;
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			Netgame.killed[i] = GET_INTEL_SHORT(&(data[len]));			len += 2;
		}
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			Netgame.player_kills[i] = GET_INTEL_SHORT(&(data[len]));		len += 2;
		}
		Netgame.KillGoal = GET_INTEL_INT(&(data[len]));					len += 4;
		Netgame.PlayTimeAllowed = GET_INTEL_INT(&(data[len]));				len += 4;
		Netgame.level_time = GET_INTEL_INT(&(data[len]));				len += 4;
		Netgame.control_invul_time = GET_INTEL_INT(&(data[len]));			len += 4;
		Netgame.monitor_vector = GET_INTEL_INT(&(data[len]));				len += 4;
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			Netgame.player_score[i] = GET_INTEL_INT(&(data[len]));			len += 4;
		}
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			Netgame.player_flags[i] = data[len];					len++;
		}
		Netgame.PacketsPerSec = GET_INTEL_SHORT(&(data[len]));				len += 2;
		Netgame.PacketLossPrevention = data[len];					len++;
		Netgame.NoFriendlyFire = data[len];						len++;

		Netgame.protocol.udp.valid = 1; // This game is valid! YAY!
	}
}

void net_udp_process_dump(ubyte *data, int len, struct _sockaddr sender_addr)
{
	// Our request for join was denied.  Tell the user why.
	if (memcmp((struct _sockaddr *)&sender_addr,(struct _sockaddr *)&Netgame.players[0].protocol.udp.addr,sizeof(struct _sockaddr)))
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
			nm_messagebox(NULL, 1, TXT_OK, NET_DUMP_STRINGS(data[1]));
			Network_status = NETSTAT_MENU;
			multi_reset_stuff();
			break;
	}
}
	
void net_udp_process_request(UDP_sequence_packet *their)
{
	// Player is ready to receieve a sync packet
	int i;

	for (i = 0; i < N_players; i++)
		if (!memcmp((struct _sockaddr *)&their->player.protocol.udp.addr, (struct _sockaddr *)&Netgame.players[i].protocol.udp.addr, sizeof(struct _sockaddr)) && (!strcasecmp(their->player.callsign, Netgame.players[i].callsign)))
		{
			Players[i].connected = CONNECT_PLAYING;
			Netgame.players[i].LastPacketTime = timer_query();
			break;
		}
}

void net_udp_process_packet(ubyte *data, struct _sockaddr sender_addr, int length )
{
	UDP_sequence_packet their;
	memset(&their, 0, sizeof(UDP_sequence_packet));

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
				net_udp_send_game_info(sender_addr, UPID_GAME_INFO);
			break;
		}
		case UPID_GAME_INFO:
			if (multi_i_am_master() || length != UPID_GAME_INFO_SIZE)
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
				net_udp_send_game_info(sender_addr, UPID_GAME_INFO_LITE);
			break;
		}
		case UPID_GAME_INFO_LITE:
			if (multi_i_am_master() || length != UPID_GAME_INFO_LITE_SIZE)
				break;
			net_udp_process_game_info(data, length, sender_addr, 1);
			break;
		case UPID_DUMP:
			if (multi_i_am_master() || memcmp((struct _sockaddr *)&Netgame.players[0].protocol.udp.addr, (struct _sockaddr *)&sender_addr, sizeof(struct _sockaddr)) || length != UPID_DUMP_SIZE)
				break;
			if ((Network_status == NETSTAT_WAITING) || (Network_status == NETSTAT_PLAYING))
				net_udp_process_dump(data, length, sender_addr);
			break;
		case UPID_ADDPLAYER:
			if (multi_i_am_master() || memcmp((struct _sockaddr *)&Netgame.players[0].protocol.udp.addr, (struct _sockaddr *)&sender_addr, sizeof(struct _sockaddr)) || length != UPID_SEQUENCE_SIZE)
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
			if (multi_i_am_master() || length != UPID_GAME_INFO_SIZE || Network_status != NETSTAT_WAITING)
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
			net_udp_process_ping(data, length, sender_addr);
			break;
		case UPID_PONG:
			if (!multi_i_am_master() || length != UPID_PONG_SIZE)
				break;
			net_udp_process_pong(data, length, sender_addr);
			break;
		case UPID_ENDLEVEL_H:
			if ((!multi_i_am_master()) && ((Network_status == NETSTAT_ENDLEVEL) || (Network_status == NETSTAT_PLAYING)))
				net_udp_read_endlevel_packet( data, length, sender_addr );
			break;
		case UPID_ENDLEVEL_C:
			if ((multi_i_am_master()) && ((Network_status == NETSTAT_ENDLEVEL) || (Network_status == NETSTAT_PLAYING)))
				net_udp_read_endlevel_packet( data, length, sender_addr );
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
			con_printf(CON_DEBUG, "unknown packet type received - type %i\n", data[0]);
			break;
	}
}

// Packet for end of level syncing
void net_udp_read_endlevel_packet( ubyte *data, int data_len, struct _sockaddr sender_addr )
{
	int len = 0, i = 0, j = 0;
	ubyte tmpvar = 0;
	
	if (multi_i_am_master())
	{
		ubyte pnum = data[1];

		if (memcmp((struct _sockaddr *)&Netgame.players[pnum].protocol.udp.addr, (struct _sockaddr *)&sender_addr, sizeof(struct _sockaddr)))
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

		for (i = 0; i < MAX_PLAYERS; i++)
		{
			kill_matrix[pnum][i] = GET_INTEL_SHORT(&(data[len]));		len += 2;
		}
		if (Players[pnum].connected)
			Netgame.players[pnum].LastPacketTime = timer_query();
	}
	else
	{
		if (memcmp((struct _sockaddr *)&Netgame.players[0].protocol.udp.addr, (struct _sockaddr *)&sender_addr, sizeof(struct _sockaddr)))
			return;

		len++;

		tmpvar = data[len];							len++;
		if ((Network_status != NETSTAT_PLAYING) && (tmpvar < Countdown_seconds_left))
			Countdown_seconds_left = tmpvar;

		for (i = 0; i < MAX_PLAYERS; i++)
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

		for (i = 0; i < MAX_PLAYERS; i++)
		{
			for (j = 0; j < MAX_PLAYERS; j++)
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
int net_udp_sync_poll( newmenu *menu, d_event *event, void *userdata )
{
	static fix64 t1 = 0;
	int rval = 0;

	if (event->type != EVENT_WINDOW_DRAW)
		return 0;
	
	menu = menu;
	userdata = userdata;
	
	net_udp_listen();

	// Leave if Host disconnects
	if (Netgame.players[0].connected == CONNECT_DISCONNECTED)
		rval = -2;

	if (Network_status != NETSTAT_WAITING)	// Status changed to playing, exit the menu
		rval = -2;

	if (Network_status != NETSTAT_MENU && !Network_rejoined && (timer_query() > t1+F1_0*2))
	{
		int i;

		// Poll time expired, re-send request
		
		t1 = timer_query();

		i = net_udp_send_request();
		if (i < 0)
			rval = -2;
	}
	
	return rval;
}

int net_udp_start_poll( newmenu *menu, d_event *event, void *userdata )
{
	newmenu_item *menus = newmenu_get_items(menu);
	int nitems = newmenu_get_nitems(menu);
	int i,n,nm;

	if (event->type != EVENT_WINDOW_DRAW)
		return 0;
	
	userdata = userdata;
	
	Assert(Network_status == NETSTAT_STARTING);

	if (!menus[0].value) {
			menus[0].value = 1;
	}

	for (i=1; i<nitems; i++ )	{
		if ( (i>= N_players) && (menus[i].value) )	{
			menus[i].value = 0;
		}
	}

	nm = 0;
	for (i=0; i<nitems; i++ )	{
		if ( menus[i].value )	{
			nm++;
			if ( nm > N_players )	{
				menus[i].value = 0;
			}
		}
	}

	if ( nm > Netgame.max_numplayers )	{
		nm_messagebox( TXT_ERROR, 1, TXT_OK, "%s %d %s", TXT_SORRY_ONLY, Netgame.max_numplayers, TXT_NETPLAYERS_IN );
		// Turn off the last player highlighted
		for (i = N_players; i > 0; i--)
			if (menus[i].value == 1) 
			{
				menus[i].value = 0;
				break;
			}
	}

   //added/killed by Victor Rachels to eventually add msging
           //since nitems should not be changing, anyway
//        if (nitems > MAX_PLAYERS ) return;
   //end this section kill - VR
	
	n = Netgame.numplayers;
	net_udp_listen();

	if (n < Netgame.numplayers ) 	
	{
		sprintf( menus[N_players-1].text, "%d. %-16s", N_players, Netgame.players[N_players-1].callsign );

		//Begin addition by GF
		digi_play_sample(SOUND_HUD_MESSAGE, F1_0);  //A noise to alert you when someone joins a starting game...
		//End addition by GF

		if (N_players <= Netgame.max_numplayers)
		{
			menus[N_players-1].value = 1;
		}
	} 
	else if ( n > Netgame.numplayers )	
	{
		// One got removed...

		//Begin addition by GF
		// <Taken out for now due to lack of testing> digi_play_sample(SOUND_HUD_KILL, F1_0);  //A noise to alert you when someone leaves a starting game...
		//End addition by GF

		for (i=0; i<N_players; i++ )	
		{
			sprintf( menus[i].text, "%d. %-16s", i+1, Netgame.players[i].callsign );
			if (i < Netgame.max_numplayers)
				menus[i].value = 1;
			else
				menus[i].value = 0;
		}
		for (i=N_players; i<n; i++ )	
		{
			sprintf( menus[i].text, "%d. ", i+1 );		// Clear out the deleted entries...
			menus[i].value = 0;
		}
   }
	
	return 0;
}

static int opt_cinvul, opt_show_on_map;
static int opt_show_on_map, opt_difficulty, opt_setpower, opt_playtime, opt_killgoal, opt_port, opt_packets, opt_show_names, opt_bright, opt_start_invul, opt_ffire;
#ifdef USE_TRACKER
static int opt_tracker;
#endif

void net_udp_set_power (void)
{
	newmenu_item m[MULTI_ALLOW_POWERUP_MAX];
	int i;

	for (i = 0; i < MULTI_ALLOW_POWERUP_MAX; i++)
	{
		m[i].type = NM_TYPE_CHECK; m[i].text = multi_allow_powerup_text[i]; m[i].value = (Netgame.AllowedItems >> i) & 1;
	}

	newmenu_do1( NULL, "Objects to allow", MULTI_ALLOW_POWERUP_MAX, m, NULL, NULL, 0 );

	Netgame.AllowedItems &= ~NETFLAG_DOPOWERUP;
	for (i = 0; i < MULTI_ALLOW_POWERUP_MAX; i++)
		if (m[i].value)
			Netgame.AllowedItems |= (1 << i);
}

int net_udp_more_options_handler( newmenu *menu, d_event *event, void *userdata );

void net_udp_more_game_options ()
{
	int opt=0,i=0;
	char PlayText[80],KillText[80],srinvul[50],packstring[5];
#ifdef USE_TRACKER
	newmenu_item m[15];
#else
 	newmenu_item m[14];
#endif

	snprintf(packstring,sizeof(char)*4,"%d",Netgame.PacketsPerSec);
	
	opt_difficulty = opt;
	m[opt].type = NM_TYPE_SLIDER; m[opt].value=Netgame.difficulty; m[opt].text=TXT_DIFFICULTY; m[opt].min_value=0; m[opt].max_value=(NDL-1); opt++;

	opt_cinvul = opt;
	sprintf( srinvul, "%s: %d %s", TXT_REACTOR_LIFE, Netgame.control_invul_time/F1_0/60, TXT_MINUTES_ABBREV );
	m[opt].type = NM_TYPE_SLIDER; m[opt].value=Netgame.control_invul_time/5/F1_0/60; m[opt].text= srinvul; m[opt].min_value=0; m[opt].max_value=10; opt++;

	opt_playtime=opt;
	sprintf( PlayText, "Max time: %d %s", Netgame.PlayTimeAllowed*5, TXT_MINUTES_ABBREV );
	m[opt].type = NM_TYPE_SLIDER; m[opt].value=Netgame.PlayTimeAllowed; m[opt].text= PlayText; m[opt].min_value=0; m[opt].max_value=10; opt++;

	opt_killgoal=opt;
	sprintf( KillText, "Kill Goal: %d kills", Netgame.KillGoal*5);
	m[opt].type = NM_TYPE_SLIDER; m[opt].value=Netgame.KillGoal; m[opt].text= KillText; m[opt].min_value=0; m[opt].max_value=10; opt++;

	opt_show_on_map=opt;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = TXT_SHOW_ON_MAP; m[opt].value=(Netgame.game_flags & NETGAME_FLAG_SHOW_MAP); opt_show_on_map=opt; opt++;

	opt_start_invul=opt;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Invulnerable when reappearing"; m[opt].value=Netgame.InvulAppear; opt++;

	opt_bright = opt;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Bright player ships"; m[opt].value=Netgame.BrightPlayers; opt++;

	opt_show_names=opt;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Show enemy names on HUD"; m[opt].value=Netgame.ShowEnemyNames; opt++;

	opt_ffire=opt;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "No friendly fire (Team, Coop)"; m[opt].value=Netgame.NoFriendlyFire; opt++;

	opt_setpower = opt;
	m[opt].type = NM_TYPE_MENU;  m[opt].text = "Set Objects allowed..."; opt++;

	m[opt].type = NM_TYPE_TEXT; m[opt].text = "Packets per second (2 - 20)"; opt++;
	opt_packets=opt;
	m[opt].type = NM_TYPE_INPUT; m[opt].text=packstring; m[opt].text_len=2; opt++;

	m[opt].type = NM_TYPE_TEXT; m[opt].text = "Network port"; opt++;
	opt_port = opt;
	m[opt].type = NM_TYPE_INPUT; m[opt].text = UDP_MyPort; m[opt].text_len=5; opt++;
#ifdef USE_TRACKER
	opt_tracker = opt;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Track this game"; m[opt].value = Netgame.Tracker; opt++;
#endif

menu:
	i = newmenu_do1( NULL, "Advanced netgame options", opt, m, net_udp_more_options_handler, NULL, 0 );

	Netgame.control_invul_time = m[opt_cinvul].value*5*F1_0*60;

	if (i==opt_setpower)
	{
		net_udp_set_power ();
		goto menu;
	}

	Netgame.PacketsPerSec=atoi(packstring);
	
	if (Netgame.PacketsPerSec>20)
	{
		Netgame.PacketsPerSec=20;
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "Packet value out of range\nSetting value to 20");
	}

	if (Netgame.PacketsPerSec<2)
	{
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "Packet value out of range\nSetting value to 2");
		Netgame.PacketsPerSec=2;
	}

	if ((atoi(UDP_MyPort)) < 0 ||(atoi(UDP_MyPort)) > 65535)
	{
		snprintf (UDP_MyPort, sizeof(UDP_MyPort), "%d", UDP_PORT_DEFAULT);
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "Illegal port");
	}

	Netgame.InvulAppear=m[opt_start_invul].value;	
	Netgame.BrightPlayers=m[opt_bright].value;
	Netgame.ShowEnemyNames=m[opt_show_names].value;
	Netgame.difficulty=Difficulty_level = m[opt_difficulty].value;
	if (m[opt_show_on_map].value)
		Netgame.game_flags |= NETGAME_FLAG_SHOW_MAP;
	else
		Netgame.game_flags &= ~NETGAME_FLAG_SHOW_MAP;
	Netgame.NoFriendlyFire = m[opt_ffire].value;
#ifdef USE_TRACKER
	Netgame.Tracker = m[opt_tracker].value;
#endif
}

int net_udp_more_options_handler( newmenu *menu, d_event *event, void *userdata )
{
	newmenu_item *menus = newmenu_get_items(menu);
	int citem = newmenu_get_citem(menu);
	
	switch (event->type)
	{
		case EVENT_NEWMENU_CHANGED:
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
				sprintf( menus[opt_killgoal].text, "Kill Goal: %d kills", Netgame.KillGoal*5);
			}
			break;
			
		default:
			break;
	}
	
	userdata = userdata;
	
	return 0;
}

typedef struct param_opt
{
	int start_game, name, level, mode, mode_end, moreopts;
	int closed, refuse, maxnet, anarchy, team_anarchy, robot_anarchy, coop, bounty;
} param_opt;

int net_udp_start_game(void);

int net_udp_game_param_handler( newmenu *menu, d_event *event, param_opt *opt )
{
	newmenu_item *menus = newmenu_get_items(menu);
	int citem = newmenu_get_citem(menu);

	switch (event->type)
	{
		case EVENT_NEWMENU_CHANGED:
			if (citem == opt->team_anarchy)
			{
				menus[opt->closed].value = 1;
				menus[opt->closed-1].value = 0;
				menus[opt->closed+1].value = 0;
			}
			
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
				
				if (!(Netgame.game_flags & NETGAME_FLAG_SHOW_MAP))
					Netgame.game_flags |= NETGAME_FLAG_SHOW_MAP;

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

				Netgame.levelnum = atoi(slevel);
				
				if (!strnicmp(slevel, "s", 1))
					Netgame.levelnum = -atoi(slevel+1);
				else
					Netgame.levelnum = atoi(slevel);
				
// 				if ((Netgame.levelnum < Last_secret_level) || (Netgame.levelnum > Last_level) || (Netgame.levelnum == 0))
// 				{
// 					nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_LEVEL_OUT_RANGE );
// 					sprintf(slevel, "1");
// 					return 0;
// 				}
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
// 		 		else if (ANARCHY_ONLY_MISSION) {
// 					int i = 0;
// 		 			nm_messagebox(NULL, 1, TXT_OK, TXT_ANARCHY_ONLY_MISSION);
// 					for (i = opt->mode; i <= opt->mode_end; i++)
// 						menus[i].value = 0;
// 					menus[opt->anarchy].value = 1;
// 		 			return 0;
// 		 		}
				else if ( menus[opt->robot_anarchy].value ) 
					Netgame.gamemode = NETGAME_ROBOT_ANARCHY;
				else if ( menus[opt->coop].value ) 
					Netgame.gamemode = NETGAME_COOPERATIVE;
				else if ( menus[opt->bounty].value )
					Netgame.gamemode = NETGAME_BOUNTY;
				else Int3(); // Invalid mode -- see Rob
			}

			if (menus[opt->closed].value)
				Netgame.game_flags |= NETGAME_FLAG_CLOSED;
			else
				Netgame.game_flags &= ~NETGAME_FLAG_CLOSED;
			Netgame.RefusePlayers=menus[opt->refuse].value;
			break;
			
		case EVENT_NEWMENU_SELECTED:
			if ((Netgame.levelnum < Last_secret_level) || (Netgame.levelnum > Last_level) || (Netgame.levelnum == 0))
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

	for (i=0;i<MAX_PLAYERS;i++)
		if (i!=Player_num)
			Players[i].callsign[0]=0;

	Netgame.max_numplayers = MAX_PLAYERS;
	Netgame.KillGoal=0;
	Netgame.PlayTimeAllowed=0;
	Netgame.RefusePlayers=0;
	sprintf( Netgame.game_name, "%s%s", Players[Player_num].callsign, TXT_S_GAME );
	Netgame.difficulty=PlayerCfg.DefaultDifficulty;
	Netgame.PacketsPerSec = 10;
	if (GameArg.MplUdpMyPort != 0)
		snprintf (UDP_MyPort, sizeof(UDP_MyPort), "%d", GameArg.MplUdpMyPort);
	else
		snprintf (UDP_MyPort, sizeof(UDP_MyPort), "%d", UDP_PORT_DEFAULT);
	Netgame.BrightPlayers = Netgame.InvulAppear = 0;
	Netgame.AllowedItems=0;
	Netgame.AllowedItems |= NETFLAG_DOPOWERUP;
	Netgame.PacketLossPrevention = 1;
	Netgame.NoFriendlyFire = 0;

#ifdef USE_TRACKER
	Netgame.Tracker = 1;
#endif

	read_netgame_profile(&Netgame);

	if (Netgame.gamemode == NETGAME_COOPERATIVE) // did we restore Coop as default? then fix max players right now!
		Netgame.max_numplayers = 4;

	strcpy(Netgame.mission_name, Current_mission_filename);
	strcpy(Netgame.mission_title, Current_mission_longname);

	sprintf( slevel, "1" ); Netgame.levelnum = 1;

	optnum = 0;
	opt.start_game=optnum;
	m[optnum].type = NM_TYPE_MENU;  m[optnum].text = "Start Game"; optnum++;
	m[optnum].type = NM_TYPE_TEXT; m[optnum].text = TXT_DESCRIPTION; optnum++;

	opt.name = optnum;
	m[optnum].type = NM_TYPE_INPUT; m[optnum].text = Netgame.game_name; m[optnum].text_len = NETGAME_NAME_LEN; optnum++;

	sprintf(level_text, "%s (1-%d)", TXT_LEVEL_, Last_level);
	if (Last_secret_level < -1)
		sprintf(level_text+strlen(level_text)-1, ", S1-S%d)", -Last_secret_level);
	else if (Last_secret_level == -1)
		sprintf(level_text+strlen(level_text)-1, ", S1)");

	Assert(strlen(level_text) < 32);

	m[optnum].type = NM_TYPE_TEXT; m[optnum].text = level_text; optnum++;

	opt.level = optnum;
	m[optnum].type = NM_TYPE_INPUT; m[optnum].text = slevel; m[optnum].text_len=4; optnum++;
	m[optnum].type = NM_TYPE_TEXT; m[optnum].text = TXT_OPTIONS; optnum++;

	opt.mode = optnum;
	m[optnum].type = NM_TYPE_RADIO; m[optnum].text = TXT_ANARCHY; m[optnum].value=(Netgame.gamemode == NETGAME_ANARCHY); m[optnum].group=0; opt.anarchy=optnum; optnum++;
	m[optnum].type = NM_TYPE_RADIO; m[optnum].text = TXT_TEAM_ANARCHY; m[optnum].value=(Netgame.gamemode == NETGAME_TEAM_ANARCHY); m[optnum].group=0; opt.team_anarchy=optnum; optnum++;
	m[optnum].type = NM_TYPE_RADIO; m[optnum].text = TXT_ANARCHY_W_ROBOTS; m[optnum].value=(Netgame.gamemode == NETGAME_ROBOT_ANARCHY); m[optnum].group=0; opt.robot_anarchy=optnum; optnum++;
	m[optnum].type = NM_TYPE_RADIO; m[optnum].text = TXT_COOPERATIVE; m[optnum].value=(Netgame.gamemode == NETGAME_COOPERATIVE); m[optnum].group=0; opt.coop=optnum; optnum++;
	m[optnum].type = NM_TYPE_RADIO; m[optnum].text = "Bounty"; m[optnum].value = ( Netgame.gamemode & NETGAME_BOUNTY ); m[optnum].group = 0; opt.mode_end=opt.bounty=optnum; optnum++;

	m[optnum].type = NM_TYPE_TEXT; m[optnum].text = ""; optnum++;

	m[optnum].type = NM_TYPE_RADIO; m[optnum].text = "Open game"; m[optnum].group=1; m[optnum].value=(!Netgame.RefusePlayers && !Netgame.game_flags & NETGAME_FLAG_CLOSED); optnum++;
	opt.closed = optnum;
	m[optnum].type = NM_TYPE_RADIO; m[optnum].text = TXT_CLOSED_GAME; m[optnum].group=1; m[optnum].value=Netgame.game_flags & NETGAME_FLAG_CLOSED; optnum++;
	opt.refuse = optnum;
	m[optnum].type = NM_TYPE_RADIO; m[optnum].text = "Restricted Game              "; m[optnum].group=1; m[optnum].value=Netgame.RefusePlayers; optnum++;

	opt.maxnet = optnum;
	sprintf( srmaxnet, "Maximum players: %d", Netgame.max_numplayers);
	m[optnum].type = NM_TYPE_SLIDER; m[optnum].value=Netgame.max_numplayers-2; m[optnum].text= srmaxnet; m[optnum].min_value=0; 
	m[optnum].max_value=Netgame.max_numplayers-2; optnum++;
	
	opt.moreopts=optnum;
	m[optnum].type = NM_TYPE_MENU;  m[optnum].text = "Advanced options"; optnum++;

	Assert(optnum <= 20);

	i = newmenu_do1( NULL, TXT_NETGAME_SETUP, optnum, m, (int (*)( newmenu *, d_event *, void * ))net_udp_game_param_handler, &opt, opt.start_game );

	if (i < 0)
		net_udp_close();

	write_netgame_profile(&Netgame);

	return i >= 0;
}

void
net_udp_set_game_mode(int gamemode)
{
	Show_kill_list = 1;

	if ( gamemode == NETGAME_ANARCHY )
		Game_mode = GM_NETWORK;
	else if ( gamemode == NETGAME_ROBOT_ANARCHY )
		Game_mode = GM_NETWORK | GM_MULTI_ROBOTS;
	else if ( gamemode == NETGAME_COOPERATIVE ) 
		Game_mode = GM_NETWORK | GM_MULTI_COOP | GM_MULTI_ROBOTS;
	else if ( gamemode == NETGAME_TEAM_ANARCHY )
	{
		Game_mode = GM_NETWORK | GM_TEAM;
                Show_kill_list = 3;
	}
	else if( gamemode == NETGAME_BOUNTY )
		Game_mode = GM_NETWORK | GM_BOUNTY;
	else
		Int3();
}

void net_udp_read_sync_packet( ubyte * data, int data_len, struct _sockaddr sender_addr )
{	
	int i, j;

	char temp_callsign[CALLSIGN_LEN+1];
	
	// This function is now called by all people entering the netgame.

	if (data)
	{
		net_udp_process_game_info(data, data_len, sender_addr, 0);
	}

	N_players = Netgame.numplayers;
	Difficulty_level = Netgame.difficulty;
	Network_status = Netgame.game_status;

	// New code, 11/27

	if (Netgame.segments_checksum != my_segments_checksum)
	{
		Network_status = NETSTAT_MENU;
		nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_NETLEVEL_NMATCH);
#ifdef NDEBUG
		return;
#endif
	}

	// Discover my player number

	memcpy(temp_callsign, Players[Player_num].callsign, CALLSIGN_LEN+1);
	
	Player_num = -1;

	for (i=0; i<MAX_PLAYERS; i++ )
	{
		Players[i].net_kills_total = 0;
		Players[i].net_killed_total = 0;
	}


	for (i=0; i<N_players; i++ )
	{
		if ( Netgame.players[i].protocol.udp.isyou == 1 && (!strcasecmp( Netgame.players[i].callsign, temp_callsign)) )
		{
			Assert(Player_num == -1); // Make sure we don't find ourselves twice!  Looking for interplay reported bug
			change_playernum_to(i);
		}
		memcpy( Players[i].callsign, Netgame.players[i].callsign, CALLSIGN_LEN+1 );

		Players[i].connected = Netgame.players[i].connected;
		Players[i].net_kills_total = Netgame.player_kills[i];
		Players[i].net_killed_total = Netgame.killed[i];
		if ((Network_rejoined) || (i != Player_num))
			Players[i].score = Netgame.player_score[i];
		for (j = 0; j < MAX_PLAYERS; j++)
		{
			kill_matrix[i][j] = Netgame.kills[i][j];
		}
	}

	if ( Player_num < 0 )	{
		Network_status = NETSTAT_MENU;
		return;
	}

	if (Network_rejoined)
		for (i=0; i<N_players;i++)
			Players[i].net_killed_total = Netgame.killed[i];

	PlayerCfg.NetlifeKills -= Players[Player_num].net_kills_total;
	PlayerCfg.NetlifeKilled -= Players[Player_num].net_killed_total;

	if (Network_rejoined)
	{
		net_udp_process_monitor_vector(Netgame.monitor_vector);
		Players[Player_num].time_level = Netgame.level_time;
	}

	team_kills[0] = Netgame.team_kills[0];
	team_kills[1] = Netgame.team_kills[1];
	
	Players[Player_num].connected = CONNECT_PLAYING;
	Netgame.players[Player_num].connected = CONNECT_PLAYING;

	if (!Network_rejoined)
	{
		for (i=0; i<NumNetPlayerPositions; i++)
		{
			Objects[Players[i].objnum].pos = Player_init[Netgame.locations[i]].pos;
			Objects[Players[i].objnum].orient = Player_init[Netgame.locations[i]].orient;
		 	obj_relink(Players[i].objnum,Player_init[Netgame.locations[i]].segnum);
		}
	}
	
	Objects[Players[Player_num].objnum].type = OBJ_PLAYER;

	Network_status = NETSTAT_PLAYING;
	multi_sort_kill_list();
}

int net_udp_send_sync(void)
{
	int i, j, np;

	// Check if there are enough starting positions
	if (NumNetPlayerPositions < Netgame.max_numplayers)
	{
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "Not enough start positions\n(set %d got %d)\nNetgame aborted", Netgame.max_numplayers, NumNetPlayerPositions);
		// Tell everyone we're bailing
		Netgame.numplayers = 0;
		for (i=1; i<N_players; i++)
		{
			net_udp_dump_player(Netgame.players[i].protocol.udp.addr, DUMP_ABORTED);
			net_udp_send_game_info(Netgame.players[i].protocol.udp.addr, UPID_GAME_INFO);
		}
		net_udp_send_game_info(GBcast, UPID_GAME_INFO_LITE);
#ifdef IPv6
		net_udp_send_game_info(GMcast_v6, UPID_GAME_INFO_LITE);
#endif
		return -1;
	}

	// Randomize their starting locations...
	d_srand( (fix)timer_query() );
	for (i=0; i<NumNetPlayerPositions; i++ )        
	{
		if (Players[i].connected)
			Players[i].connected = CONNECT_PLAYING; // Get rid of endlevel connect statuses

		if (Game_mode & GM_MULTI_COOP)
			Netgame.locations[i] = i;
		else {
			do 
			{
				np = d_rand() % NumNetPlayerPositions;
				for (j=0; j<i; j++ )    
				{
					if (Netgame.locations[j]==np)   
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

	for (i=0; i<N_players; i++ )
	{
		if ((!Players[i].connected) || (i == Player_num))
			continue;

		net_udp_send_game_info(Netgame.players[i].protocol.udp.addr, UPID_SYNC);
	}

	net_udp_read_sync_packet(NULL, 0, Netgame.players[0].protocol.udp.addr); // Read it myself, as if I had sent it
	return 0;
}

int
net_udp_select_teams(void)
{
	newmenu_item m[MAX_PLAYERS+4];
	int choice, opt, opt_team_b;
	ubyte team_vector = 0;
	char team_names[2][CALLSIGN_LEN+1];
	int i;
	int pnums[MAX_PLAYERS+2];

	// One-time initialization

	for (i = N_players/2; i < N_players; i++) // Put first half of players on team A
	{
		team_vector |= (1 << i);
	}

	sprintf(team_names[0], "%s", TXT_BLUE);
	sprintf(team_names[1], "%s", TXT_RED);

	// Here comes da menu
menu:
	m[0].type = NM_TYPE_INPUT; m[0].text = team_names[0]; m[0].text_len = CALLSIGN_LEN; 

	opt = 1;
	for (i = 0; i < N_players; i++)
	{
		if (!(team_vector & (1 << i)))
		{
			m[opt].type = NM_TYPE_MENU; m[opt].text = Netgame.players[i].callsign; pnums[opt] = i; opt++;
		}
	}
	opt_team_b = opt;
	m[opt].type = NM_TYPE_INPUT; m[opt].text = team_names[1]; m[opt].text_len = CALLSIGN_LEN; opt++;
	for (i = 0; i < N_players; i++)
	{
		if (team_vector & (1 << i))
		{
			m[opt].type = NM_TYPE_MENU; m[opt].text = Netgame.players[i].callsign; pnums[opt] = i; opt++;
		}
	}
	m[opt].type = NM_TYPE_TEXT; m[opt].text = ""; opt++;
	m[opt].type = NM_TYPE_MENU; m[opt].text = TXT_ACCEPT; opt++;

	Assert(opt <= MAX_PLAYERS+4);
	
	choice = newmenu_do(NULL, TXT_TEAM_SELECTION, opt, m, NULL, NULL);

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
		strcpy(Netgame.team_name[0], team_names[0]);
		strcpy(Netgame.team_name[1], team_names[1]);
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
net_udp_select_players(void)
{
        int i, j, opts, opt_msg;
        newmenu_item m[MAX_PLAYERS+1];
	char text[MAX_PLAYERS][25];
	char title[50];
	int save_nplayers;

	net_udp_add_player( &UDP_Seq );
		
	for (i=0; i< MAX_PLAYERS; i++ )	{
		sprintf( text[i], "%d.  %-16s", i+1, "" );
		m[i].type = NM_TYPE_CHECK; m[i].text = text[i]; m[i].value = 0;
	}
//added/edited on 11/7/98 by Victor Rachels in an attempt to get msgs going.
        opts=MAX_PLAYERS;
        opt_msg = opts;
//killed for now to not raise people's hopes - 11/10/98 - VR
//        m[opts].type = NM_TYPE_MENU; m[opts].text = "Send message..."; opts++;

	m[0].value = 1;				// Assume server will play...

	sprintf( text[0], "%d. %-16s", 1, Players[Player_num].callsign );
	sprintf( title, "%s %d %s", TXT_TEAM_SELECT, Netgame.max_numplayers, TXT_TEAM_PRESS_ENTER );

GetPlayersAgain:
#ifdef USE_TRACKER
	if( Netgame.Tracker )
		udp_tracker_register();
#endif

        j=opt_msg;
         while(j==opt_msg)
          {
		  timer_update();
            j=newmenu_do1( NULL, title, opts, m, net_udp_start_poll, NULL, 1 );

            if(j==opt_msg)
             {
              multi_send_message_dialog();
               if (Network_message_reciever != -1)
                multi_send_message();
             }
          }
//end this section addition
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
		for (i=1; i<save_nplayers; i++) {
			net_udp_dump_player(Netgame.players[i].protocol.udp.addr, DUMP_ABORTED);
			net_udp_send_game_info(Netgame.players[i].protocol.udp.addr, UPID_GAME_INFO);
		}
		net_udp_send_game_info(GBcast, UPID_GAME_INFO_LITE);
#ifdef IPv6
		net_udp_send_game_info(GMcast_v6, UPID_GAME_INFO_LITE);
#endif
		Netgame.numplayers = save_nplayers;

		Network_status = NETSTAT_MENU;
		return(0);
	}

	// Count number of players chosen

	N_players = 0;
	for (i=0; i<save_nplayers; i++ )
	{
		if (m[i].value)	
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

// Let host join without Client available. Let's see if our players like that
#if 0 //def RELEASE
	if ( (Netgame.gamemode == NETGAME_TEAM_ANARCHY) && (N_players < 3) ) {
		nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_TEAM_ATLEAST_THREE );
		N_players = save_nplayers;
		goto GetPlayersAgain;
	}
#endif

	// Remove players that aren't marked.
	N_players = 0;
	for (i=0; i<save_nplayers; i++ )	{
		if (m[i].value)	
		{
			if (i > N_players)
			{
				memcpy(Netgame.players[N_players].callsign, Netgame.players[i].callsign, CALLSIGN_LEN+1);
			}
			Players[N_players].connected = CONNECT_PLAYING;
			N_players++;
		}
		else
		{
			net_udp_dump_player(Netgame.players[i].protocol.udp.addr, DUMP_DORK);
		}
	}

	for (i = N_players; i < MAX_PLAYERS; i++) {
		memset(Netgame.players[i].callsign, 0, CALLSIGN_LEN+1);
	}

	if (Netgame.gamemode == NETGAME_TEAM_ANARCHY)
		if (!net_udp_select_teams())
			goto abort;

	return(1); 
}

int net_udp_start_game(void)
{
	int i;

	i = udp_open_socket(0, atoi(UDP_MyPort));

	if (i != 0)
		return 0;
	
	if (atoi(UDP_MyPort) != UDP_PORT_DEFAULT)
		i = udp_open_socket(1, UDP_PORT_DEFAULT); // Default port open for Broadcasts

	if (i != 0)
		return 0;

	// prepare broadcast address to announce our game
	memset(&GBcast, '\0', sizeof(struct _sockaddr));
	udp_dns_filladdr(UDP_BCAST_ADDR, UDP_PORT_DEFAULT, &GBcast);
#ifdef IPv6
	memset(&GMcast_v6, '\0', sizeof(struct _sockaddr));
	udp_dns_filladdr(UDP_MCASTv6_ADDR, UDP_PORT_DEFAULT, &GMcast_v6);
#endif
	d_srand( (fix)timer_query() );
	Netgame.protocol.udp.GameID=d_rand();


	N_players = 0;

    Netgame.game_status = NETSTAT_STARTING;
	Netgame.numplayers = 0;
	net_udp_set_game_mode(Netgame.gamemode);
	Netgame.players[0].protocol.udp.isyou = 1; // I am Host. I need to know that y'know? For syncing later.

	Network_status = NETSTAT_STARTING;

	if(net_udp_select_players())
	{
		StartNewLevel(Netgame.levelnum);
	}
	else
	{
		Game_mode = GM_GAME_OVER;
		return 0;	// see if we want to tweak the game we setup
	}
	net_udp_send_game_info(GBcast, UPID_GAME_INFO_LITE); // game started. broadcast our current status to everyone who wants to know
#ifdef IPv6
	net_udp_send_game_info(GMcast_v6, UPID_GAME_INFO_LITE); // game started. broadcast our current status to everyone who wants to know
#endif

	return 1;	// don't keep params menu or mission listbox (may want to join a game next time)
}

int
net_udp_wait_for_sync(void)
{
	char text[60];
	newmenu_item m[2];
	int i, choice=0;
	
	Network_status = NETSTAT_WAITING;
	m[0].type=NM_TYPE_TEXT; m[0].text = text;
	m[1].type=NM_TYPE_TEXT; m[1].text = TXT_NET_LEAVE;
	
	i = net_udp_send_request();

	if (i < 0)
		return(-1);

	sprintf( m[0].text, "%s\n'%s' %s", TXT_NET_WAITING, Netgame.players[i].callsign, TXT_NET_TO_ENTER );

	while (choice > -1)
	{
		timer_update();
		choice=newmenu_do( NULL, TXT_WAIT, 2, m, net_udp_sync_poll, NULL );
	}


	if (Network_status != NETSTAT_PLAYING)	
	{
		UDP_sequence_packet me;

		memset(&me, 0, sizeof(UDP_sequence_packet));
		me.type = UPID_QUIT_JOINING;
		memcpy( me.player.callsign, Players[Player_num].callsign, CALLSIGN_LEN+1 );
		net_udp_send_sequence_packet( me, Netgame.players[0].protocol.udp.addr );
		N_players = 0;
		Game_mode = GM_GAME_OVER;
		return(-1);	// they cancelled		
	}
	return(0);
}

int net_udp_request_poll( newmenu *menu, d_event *event, void *userdata )
{
	// Polling loop for waiting-for-requests menu

	int i = 0;
	int num_ready = 0;

	if (event->type != EVENT_WINDOW_DRAW)
		return 0;
	
	menu = menu;
	userdata = userdata;
	
	net_udp_listen();
	net_udp_timeout_check(timer_query());

	for (i = 0; i < N_players; i++)
	{
		if ((Players[i].connected == CONNECT_PLAYING) || (Players[i].connected == CONNECT_DISCONNECTED))
			num_ready++;
	}

	if (num_ready == N_players) // All players have checked in or are disconnected
	{
		return -2;
	}
	
	return 0;
}

int net_udp_wait_for_requests(void)
{
	// Wait for other players to load the level before we send the sync
	int choice, i;
	newmenu_item m[1];
	
	Network_status = NETSTAT_WAITING;

	m[0].type=NM_TYPE_TEXT; m[0].text = TXT_NET_LEAVE;

	Network_status = NETSTAT_WAITING;
	net_udp_flush();

	Players[Player_num].connected = CONNECT_PLAYING;

menu:
	choice = newmenu_do(NULL, TXT_WAIT, 1, m, net_udp_request_poll, NULL);	

	if (choice == -1)
	{
		// User aborted
		choice = nm_messagebox(NULL, 3, TXT_YES, TXT_NO, TXT_START_NOWAIT, TXT_QUITTING_NOW);
		if (choice == 2)
			return 0;
		if (choice != 0)
			goto menu;
		
		// User confirmed abort
		
		for (i=0; i < N_players; i++)
			if ((Players[i].connected != CONNECT_DISCONNECTED) && (i != Player_num))
				net_udp_dump_player(Netgame.players[i].protocol.udp.addr, DUMP_ABORTED);

		return -1;
	}
	else if (choice != -2)
		goto menu;

	return 0;
}

int
net_udp_level_sync(void)
{
 	// Do required syncing between (before) levels

	int result = 0;

	memset(&UDP_MData, 0, sizeof(UDP_mdata_info));
	net_udp_noloss_init_mdata_queue();

//	my_segments_checksum = netmisc_calc_checksum(Segments, sizeof(segment)*(Highest_segment_index+1));

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

	if (!load_mission_by_name(Netgame.mission_name))
	{
		nm_messagebox(NULL, 1, TXT_OK, TXT_MISSION_NOT_FOUND);
		return 0;
	}

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
	int nsave, i;

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
		for (i=1; i<nsave; i++ )
		{
			net_udp_send_game_info(Netgame.players[i].protocol.udp.addr, UPID_GAME_INFO);
		}
		net_udp_send_game_info(GBcast, UPID_GAME_INFO_LITE);
#ifdef IPv6
		net_udp_send_game_info(GMcast_v6, UPID_GAME_INFO_LITE);
#endif
		N_players=nsave;
#ifdef USE_TRACKER
		if( Netgame.Tracker )
			udp_tracker_unregister();
#endif
	}

	Players[Player_num].connected = CONNECT_DISCONNECTED;
	change_playernum_to(0);
	net_udp_flush();
	net_udp_close();
}

void net_udp_flush()
{
	ubyte packet[UPID_MAX_SIZE];
	struct _sockaddr sender_addr; 

	if (UDP_Socket[0] != -1)
		while (udp_receive_packet( 0, packet, UPID_MAX_SIZE, &sender_addr) > 0);

	if (UDP_Socket[1] != -1)
		while (udp_receive_packet( 1, packet, UPID_MAX_SIZE, &sender_addr) > 0);
}

void net_udp_listen()
{
	int size;
	ubyte packet[UPID_MAX_SIZE];
	struct _sockaddr sender_addr;

	if (UDP_Socket[0] != -1)
	{
		size = udp_receive_packet( 0, packet, UPID_MAX_SIZE, &sender_addr );
		while ( size > 0 )	{
			net_udp_process_packet( packet, sender_addr, size );
			size = udp_receive_packet( 0, packet, UPID_MAX_SIZE, &sender_addr );
		}
	}

	if (UDP_Socket[1] != -1)
	{
		size = udp_receive_packet( 1, packet, UPID_MAX_SIZE, &sender_addr );
		while ( size > 0 )	{
			net_udp_process_packet( packet, sender_addr, size );
			size = udp_receive_packet( 1, packet, UPID_MAX_SIZE, &sender_addr );
		}
	}
#ifdef USE_TRACKER
	if( UDP_Socket[2] != -1 )
	{
		size = udp_receive_packet( 2, packet, UPID_MAX_SIZE, &sender_addr );
		while ( size > 0 )	{
			net_udp_process_packet( packet, sender_addr, size );
			size = udp_receive_packet( 2, packet, UPID_MAX_SIZE, &sender_addr );
		}
	}
#endif
}

void net_udp_send_data( ubyte * ptr, int len, int priority )
{
	char check;

	if (Endlevel_sequence)
		return;

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
	int i = 0;
	static fix64 last_timeout_time = 0;
	
	if (time>=last_timeout_time+F1_0)
	{
		// Check for player timeouts
		for (i = 0; i < N_players; i++)
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
	fix64 time = 0;
	static fix64 last_pdata_time = 0, last_mdata_time = 16, last_endlevel_time = 32, last_bcast_time = 48, last_resync_time = 64;

	if (!(Game_mode&GM_NETWORK) || UDP_Socket[0] == -1)
		return;

	time = timer_query();

	if (WaitForRefuseAnswer && time>(RefuseTimeLimit+(F1_0*12)))
		WaitForRefuseAnswer=0;

	// Send positional update either in the regular PPS interval OR if forced AND at least every 66.6ms (nice for firing)
	if ((force && time >= (last_pdata_time+(F1_0/15))) || (time >= (last_pdata_time+(F1_0/Netgame.PacketsPerSec))))
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
		net_udp_send_game_info(GBcast, UPID_GAME_INFO_LITE);
#ifdef IPv6
		net_udp_send_game_info(GMcast_v6, UPID_GAME_INFO_LITE);
#endif
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
			nm_messagebox( TXT_WARNING, 1, TXT_OK, "No response from tracker!\nPossible causes:\nTracker is down\nYour port is likely not open!\n\nTracker: %s\nGame port: %s", GameArg.MplTrackerAddr, UDP_MyPort );
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
/*
 * Adds a packet to our queue. Should be called when an IMPORTANT mdata packet is created.
 * player_ack is an array which should contain 0 for each player that needs to send an ACK signal.
 */
void net_udp_noloss_add_queue_pkt(uint32_t pkt_num, fix64 time, ubyte *data, ushort data_size, ubyte pnum, ubyte player_ack[MAX_PLAYERS])
{
	int i, found = 0;

	if (!(Game_mode&GM_NETWORK) || UDP_Socket[0] == -1)
		return;

	if (!Netgame.PacketLossPrevention)
		return;

	for (i = 0; i < UDP_MDATA_STOR_QUEUE_SIZE; i++) // look for unused or oldest slot
	{
		if (UDP_mdata_queue[i].used)
		{
			if (UDP_mdata_queue[i].pkt_initial_timestamp > UDP_mdata_queue[found].pkt_initial_timestamp)
				found = i;
		}
		else
		{
			found = i;
			break;
		}
	}

	if (UDP_mdata_queue[found].used) // seems the slot we found is used (list is full) so screw  those who still need ack's.
	{
		con_printf(CON_VERBOSE, "P#%i: MData store list is full!\n", Player_num);
		if (multi_i_am_master())
		{
			for ( i=1; i<N_players; i++ )
				if (UDP_mdata_queue[found].player_ack[i] == 0)
					net_udp_dump_player(Netgame.players[i].protocol.udp.addr, DUMP_PKTTIMEOUT);
		}
		else
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

	con_printf(CON_VERBOSE, "P#%i: Adding MData pkt_num %i, type %i from P#%i to MData store list\n", Player_num, pkt_num, data[0], pnum);
	UDP_mdata_queue[found].used = 1;
	UDP_mdata_queue[found].pkt_initial_timestamp = time;
	for (i = 0; i < MAX_PLAYERS; i++)
		UDP_mdata_queue[found].pkt_timestamp[i] = time;
	UDP_mdata_queue[found].pkt_num = pkt_num;
	UDP_mdata_queue[found].Player_num = pnum;
	memcpy( &UDP_mdata_queue[found].player_ack, player_ack, sizeof(ubyte)*MAX_PLAYERS); 
	memcpy( &UDP_mdata_queue[found].data, data, sizeof(char)*data_size );
	UDP_mdata_queue[found].data_size = data_size;
}

/*
 * We have received a MDATA packet. Send ACK response to sender!
 * Also check in our UDP_mdata_got list, if we got this packet already. If yes, return 0 so do not process it!
 */
int net_udp_noloss_validate_mdata(uint32_t pkt_num, ubyte sender_pnum, struct _sockaddr sender_addr)
{
	ubyte buf[7];
	int i = 0, len = 0;

	// Check if this comes from a valid IP
	if (multi_i_am_master())
	{
		if (memcmp((struct _sockaddr *)&sender_addr, (struct _sockaddr *)&Netgame.players[sender_pnum].protocol.udp.addr, sizeof(struct _sockaddr)))
			return 0;
	}
	else
	{
		if (memcmp((struct _sockaddr *)&sender_addr, (struct _sockaddr *)&Netgame.players[0].protocol.udp.addr, sizeof(struct _sockaddr)))
			return 0;
	}
	
	con_printf(CON_VERBOSE, "P#%i: Sending MData ACK for pkt %i - pnum %i\n",Player_num, pkt_num, sender_pnum);
	memset(&buf,0,sizeof(buf));
	buf[len] = UPID_MDATA_ACK;													len++;
	buf[len] = Player_num;														len++;
	buf[len] = sender_pnum;														len++;
	PUT_INTEL_INT(buf + len, pkt_num);											len += 4;
	dxx_sendto (UDP_Socket[0], buf, len, 0, (struct sockaddr *)&sender_addr, sizeof(struct _sockaddr));
	
	for (i = 0; i < UDP_MDATA_STOR_QUEUE_SIZE; i++)
	{
		if (pkt_num == UDP_mdata_got[sender_pnum].pkt_num[i])
			return 0; // we got this packet already
	}
	UDP_mdata_got[sender_pnum].cur_slot++;
	if (UDP_mdata_got[sender_pnum].cur_slot >= UDP_MDATA_STOR_QUEUE_SIZE)
		UDP_mdata_got[sender_pnum].cur_slot = 0;
	UDP_mdata_got[sender_pnum].pkt_num[UDP_mdata_got[sender_pnum].cur_slot] = pkt_num;
	return 1;
}

/* We got an ACK by a player. Set this player slot to positive! */
void net_udp_noloss_got_ack(ubyte *data, int data_len)
{
	int i = 0, len = 0;
	uint32_t pkt_num = 0;
	ubyte sender_pnum = 0, dest_pnum = 0;

	if (data_len != 7)
		return;

																				len++;
	sender_pnum = data[len];													len++;
	dest_pnum = data[len];														len++;
	pkt_num = GET_INTEL_INT(&data[len]);										len += 4;

	for (i = 0; i < UDP_MDATA_STOR_QUEUE_SIZE; i++)
	{
		if ((pkt_num == UDP_mdata_queue[i].pkt_num) && (dest_pnum == UDP_mdata_queue[i].Player_num))
		{
			con_printf(CON_VERBOSE, "P#%i: Got MData ACK for pkt_num %i from pnum %i for pnum %i\n",Player_num, pkt_num, sender_pnum, dest_pnum);
			UDP_mdata_queue[i].player_ack[sender_pnum] = 1;
			break;
		}
	}
}

/* Init/Free the queue. Call at start and end of a game or level. */
void net_udp_noloss_init_mdata_queue(void)
{
	con_printf(CON_VERBOSE, "P#%i: Clearing MData store/GOT list\n",Player_num);
	memset(&UDP_mdata_queue,0,sizeof(UDP_mdata_store)*UDP_MDATA_STOR_QUEUE_SIZE);
	memset(&UDP_mdata_got,0,sizeof(UDP_mdata_recv)*MAX_PLAYERS);
}

/* Reset the trace list for given player when (dis)connect happens */
void net_udp_noloss_clear_mdata_got(ubyte player_num)
{
	con_printf(CON_VERBOSE, "P#%i: Clearing GOT list for %i\n",Player_num, player_num);
	memset(&UDP_mdata_got[player_num].pkt_num,0,sizeof(uint32_t)*UDP_MDATA_STOR_QUEUE_SIZE);
	UDP_mdata_got[player_num].cur_slot = 0;
}

/*
 * The main queue-process function.
 * Check if we can remove a packet from queue, and check if there are packets in queue which we need to re-send
 */
void net_udp_noloss_process_queue(fix64 time)
{
	int queuec = 0, plc = 0, total_len = 0;

	if (!(Game_mode&GM_NETWORK) || UDP_Socket[0] == -1)
		return;

	if (!Netgame.PacketLossPrevention)
		return;

	for (queuec = 0; queuec < UDP_MDATA_STOR_QUEUE_SIZE; queuec++)
	{
		int needack = 0;
		
		if (!UDP_mdata_queue[queuec].used)
			continue;

		// Check if at least one connected player has not ACK'd the packet
		for (plc = 0; plc < MAX_PLAYERS; plc++)
		{
			// If player is not playing anymore, we can remove him from list. Also remove *me* (even if that should have been done already). Also make sure Clients do not send to anyone else than Host
			if ((Players[plc].connected != CONNECT_PLAYING || plc == Player_num) || (!multi_i_am_master() && plc > 0))
				UDP_mdata_queue[queuec].player_ack[plc] = 1;

			if (!UDP_mdata_queue[queuec].player_ack[plc])
			{
				// Resend if enough time has passed.
				if (UDP_mdata_queue[queuec].pkt_timestamp[plc] + (F1_0/3) <= time)
				{
					ubyte buf[sizeof(UDP_mdata_info)];
					int len = 0;
					
					con_printf(CON_VERBOSE, "P#%i: Resending pkt_num %i from pnum %i to pnum %i\n",Player_num, UDP_mdata_queue[queuec].pkt_num, UDP_mdata_queue[queuec].Player_num, plc);
					
					UDP_mdata_queue[queuec].pkt_timestamp[plc] = time;
					memset(&buf, 0, sizeof(UDP_mdata_info));
					
					// Prepare the packet and send it
					buf[len] = UPID_MDATA_PNEEDACK;													len++;
					buf[len] = UDP_mdata_queue[queuec].Player_num;								len++;
					PUT_INTEL_INT(buf + len, UDP_mdata_queue[queuec].pkt_num);					len += 4;
					memcpy(&buf[len], UDP_mdata_queue[queuec].data, sizeof(char)*UDP_mdata_queue[queuec].data_size);
																								len += UDP_mdata_queue[queuec].data_size;
					dxx_sendto (UDP_Socket[0], buf, len, 0, (struct sockaddr *)&Netgame.players[plc].protocol.udp.addr, sizeof(struct _sockaddr));
					total_len += len;
				}
				needack++;
			}
		}

		// Check if we can remove that packet due to to it had no resend's or Timeout
		if (needack==0 || (UDP_mdata_queue[queuec].pkt_initial_timestamp + UDP_TIMEOUT <= time))
		{
			if (needack) // packet timed out but still not all have ack'd. SCREW THEM NOW!
			{
				if (multi_i_am_master())
				{
					for ( plc=1; plc<N_players; plc++ )
						if (UDP_mdata_queue[queuec].player_ack[plc] == 0)
							net_udp_dump_player(Netgame.players[plc].protocol.udp.addr, DUMP_PKTTIMEOUT);
				}
				else
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
			con_printf(CON_VERBOSE, "P#%i: Removing stored pkt_num %i - missing ACKs: %i\n",Player_num, UDP_mdata_queue[queuec].pkt_num, needack);
			memset(&UDP_mdata_queue[queuec],0,sizeof(UDP_mdata_store));
		}

		// Send up to half our max packet size
		if (total_len >= (UPID_MAX_SIZE/2))
			break;
	}
}
/* CODE FOR PACKET LOSS PREVENTION - END */

void net_udp_send_mdata_direct(ubyte *data, int data_len, int pnum, int needack)
{
	ubyte buf[sizeof(UDP_mdata_info)];
	ubyte pack[MAX_PLAYERS];
	int len = 0;
	
	if (!(Game_mode&GM_NETWORK) || UDP_Socket[0] == -1)
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
	buf[len] = Player_num;														len++;
	if (needack)
	{
		UDP_MData.pkt_num++;
		Assert(UDP_MDATA_STOR_QUEUE_SIZE*100 < INT_MAX);
		if (UDP_MData.pkt_num > UDP_MDATA_STOR_QUEUE_SIZE*100) // roll over at some point
			UDP_MData.pkt_num = 0;
		PUT_INTEL_INT(buf + len, UDP_MData.pkt_num);							len += 4;
	}
	memcpy(&buf[len], data, sizeof(char)*data_len);								len += data_len;

	dxx_sendto (UDP_Socket[0], buf, len, 0, (struct sockaddr *)&Netgame.players[pnum].protocol.udp.addr, sizeof(struct _sockaddr));

	if (needack)
		net_udp_noloss_add_queue_pkt(UDP_MData.pkt_num, timer_query(), data, data_len, Player_num, pack);
}

void net_udp_send_mdata(int needack, fix64 time)
{
	ubyte buf[sizeof(UDP_mdata_info)];
	ubyte pack[MAX_PLAYERS];
	int len = 0, i = 0;
	
	if (!(Game_mode&GM_NETWORK) || UDP_Socket[0] == -1)
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
	buf[len] = Player_num;														len++;
	if (needack)
	{
		UDP_MData.pkt_num++;
		PUT_INTEL_INT(buf + len, UDP_MData.pkt_num);							len += 4;
	}
	memcpy(&buf[len], UDP_MData.mbuf, sizeof(char)*UDP_MData.mbuf_size);		len += UDP_MData.mbuf_size;

	if (multi_i_am_master())
	{
		for (i = 1; i < MAX_PLAYERS; i++)
		{
			if (Players[i].connected == CONNECT_PLAYING)
			{
				dxx_sendto (UDP_Socket[0], buf, len, 0, (struct sockaddr *)&Netgame.players[i].protocol.udp.addr, sizeof(struct _sockaddr));
				pack[i] = 0;
			}
		}
	}
	else
	{
		dxx_sendto (UDP_Socket[0], buf, len, 0, (struct sockaddr *)&Netgame.players[0].protocol.udp.addr, sizeof(struct _sockaddr));
		pack[0] = 0;
	}
	
	if (needack)
		net_udp_noloss_add_queue_pkt(UDP_MData.pkt_num, time, UDP_MData.mbuf, UDP_MData.mbuf_size, Player_num, pack);

	// Clear UDP_MData except pkt_num. That one must not be deleted so we can clearly keep track of important packets.
	UDP_MData.type = 0;
	UDP_MData.Player_num = 0;
	UDP_MData.mbuf_size = 0;
	memset(&UDP_MData.mbuf, 0, sizeof(ubyte)*UPID_MDATA_BUF_SIZE);
}

void net_udp_process_mdata (ubyte *data, int data_len, struct _sockaddr sender_addr, int needack)
{
	int pnum = data[1], dataoffset = (needack?6:2);

	// Check if packet might be bogus
	if ((pnum < 0) || (data_len > sizeof(UDP_mdata_info)))
		return;

	// Check if it came from valid IP
	if (multi_i_am_master())
	{
		if (memcmp((struct _sockaddr *)&sender_addr, (struct _sockaddr *)&Netgame.players[pnum].protocol.udp.addr, sizeof(struct _sockaddr)))
		{
			return;
		}
	}
	else
	{
		if (memcmp((struct _sockaddr *)&sender_addr, (struct _sockaddr *)&Netgame.players[0].protocol.udp.addr, sizeof(struct _sockaddr)))
		{
			return;
		}
	}

	// Add needack packet and check for possible redundancy
	if (needack)
	{
		if (!net_udp_noloss_validate_mdata(GET_INTEL_SHORT(&data[2]), pnum, sender_addr))
			return;
	}

	// send this to everyone else (if master)
	if (multi_i_am_master())
	{
		ubyte pack[MAX_PLAYERS];
		int i = 0;

		memset(&pack, 1, sizeof(ubyte)*MAX_PLAYERS);
		
		for (i = 1; i < MAX_PLAYERS; i++)
		{
			if ((i != pnum) && Players[i].connected == CONNECT_PLAYING)
			{
				dxx_sendto (UDP_Socket[0], data, data_len, 0, (struct sockaddr *)&Netgame.players[i].protocol.udp.addr, sizeof(struct _sockaddr));
				pack[i] = 0;
			}
		}

		if (needack && N_players > 2)
		{
			net_udp_noloss_add_queue_pkt(GET_INTEL_SHORT(&data[2]), timer_query(), data+dataoffset, data_len-dataoffset, pnum, pack);
		}
	}

	// Check if we are in correct state to process the packet
	if (!((Network_status == NETSTAT_PLAYING)||(Network_status == NETSTAT_ENDLEVEL) || Network_status==NETSTAT_WAITING))
		return;

	// Process
	if (Endlevel_sequence || (Network_status == NETSTAT_ENDLEVEL))
	{
		int old_Endlevel_sequence = Endlevel_sequence;
		Endlevel_sequence = 1;
		multi_process_bigdata( (char*)data+dataoffset, data_len-dataoffset);
		Endlevel_sequence = old_Endlevel_sequence;
		return;
	}

	multi_process_bigdata( (char*)data+dataoffset, data_len-dataoffset );
}

void net_udp_send_pdata()
{
	ubyte buf[sizeof(UDP_frame_info)];
	shortpos pos;
	int len = 0, i = 0;

	if (!(Game_mode&GM_NETWORK) || UDP_Socket[0] == -1)
		return;
	if (Players[Player_num].connected != CONNECT_PLAYING)
		return;

	memset(&buf, 0, sizeof(UDP_frame_info));
	
	buf[len] = UPID_PDATA;										len++;
	buf[len] = Player_num;										len++;
	buf[len] = Players[Player_num].connected;							len++;
	buf[len] = Objects[Players[Player_num].objnum].render_type;					len++;
	memset(&pos, 0, sizeof(shortpos));
	create_shortpos(&pos, Objects+Players[Player_num].objnum, 0);
	memcpy(buf + len, &pos.bytemat, 9);								len += 9;
	PUT_INTEL_SHORT(&buf[len], pos.xo);								len += 2;
	PUT_INTEL_SHORT(&buf[len], pos.yo);								len += 2;
	PUT_INTEL_SHORT(&buf[len], pos.zo);								len += 2;
	PUT_INTEL_SHORT(&buf[len], pos.segment);							len += 2;
	PUT_INTEL_SHORT(&buf[len], pos.velx);								len += 2;
	PUT_INTEL_SHORT(&buf[len], pos.vely);								len += 2;
	PUT_INTEL_SHORT(&buf[len], pos.velz);								len += 2;

	if (multi_i_am_master())
	{
		for (i = 1; i < MAX_PLAYERS; i++)
			if (Players[i].connected != CONNECT_DISCONNECTED)
				dxx_sendto (UDP_Socket[0], buf, len, 0, (struct sockaddr *)&Netgame.players[i].protocol.udp.addr, sizeof(struct _sockaddr));
	}
	else
	{
		dxx_sendto (UDP_Socket[0], buf, len, 0, (struct sockaddr *)&Netgame.players[0].protocol.udp.addr, sizeof(struct _sockaddr));
	}
}

void net_udp_process_pdata ( ubyte *data, int data_len, struct _sockaddr sender_addr )
{
	UDP_frame_info pd;
	int len = 0, i = 0;

	if ( !( Game_mode & GM_NETWORK && ( Network_status == NETSTAT_PLAYING || Network_status == NETSTAT_ENDLEVEL ||  Network_status==NETSTAT_WAITING ) ) )
		return;

	len++;

	memset(&pd, 0, sizeof(UDP_frame_info));
	
	if (data_len > sizeof(UDP_frame_info))
		return;

	if (memcmp((struct _sockaddr *)&sender_addr, (struct _sockaddr *)&Netgame.players[((multi_i_am_master())?(data[len]):(0))].protocol.udp.addr, sizeof(struct _sockaddr)))
		return;

	pd.Player_num = data[len];									len++;
	pd.connected = data[len];									len++;
	pd.obj_render_type = data[len];									len++;
	memcpy(&pd.pos.bytemat, &(data[len]), 9);							len += 9;
	pd.pos.xo = GET_INTEL_SHORT(&data[len]);							len += 2;
	pd.pos.yo = GET_INTEL_SHORT(&data[len]);							len += 2;
	pd.pos.zo = GET_INTEL_SHORT(&data[len]);							len += 2;
	pd.pos.segment = GET_INTEL_SHORT(&data[len]);							len += 2;
	pd.pos.velx = GET_INTEL_SHORT(&data[len]);							len += 2;
	pd.pos.vely = GET_INTEL_SHORT(&data[len]);							len += 2;
	pd.pos.velz = GET_INTEL_SHORT(&data[len]);							len += 2;
	
	if (multi_i_am_master()) // I am host - must relay this packet to others!
	{
		if (pd.Player_num > 0 && pd.Player_num <= N_players && Players[pd.Player_num].connected == CONNECT_PLAYING) // some checking wether this packet is legal
		{
			for (i = 1; i < MAX_PLAYERS; i++)
			{
				if (i != pd.Player_num && Players[pd.Player_num].connected != CONNECT_DISCONNECTED) // not to sender or disconnected players - right.
					dxx_sendto (UDP_Socket[0], data, data_len, 0, (struct sockaddr *)&Netgame.players[i].protocol.udp.addr, sizeof(struct _sockaddr));
			}
		}
	}

	net_udp_read_pdata_short_packet (&pd);
}

void net_udp_read_pdata_short_packet(UDP_frame_info *pd)
{
	int TheirPlayernum;
	int TheirObjnum;
	object * TheirObj = NULL;

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

			HUD_init_message(HM_MULTI,  "'%s' %s", Players[TheirPlayernum].callsign, TXT_REJOIN );

			multi_send_score();

			net_udp_noloss_clear_mdata_got(TheirPlayernum);
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

	TheirObj = &Objects[TheirObjnum];
	Netgame.players[TheirPlayernum].LastPacketTime = timer_query();

	//------------ Read the player's ship's object info ----------------------

	extract_shortpos(TheirObj, &pd->pos, 0);

	if (TheirObj->movement_type == MT_PHYSICS)
		set_thrust_from_velocity(TheirObj);
}

// Send the ping list in regular intervals
void net_udp_ping_frame(fix64 time)
{
	static fix64 PingTime = 0;
	
	if ((PingTime + F1_0) < time)
	{
		ubyte buf[UPID_PING_SIZE];
		int len = 0, i = 0;
		
		memset(&buf, 0, sizeof(ubyte)*UPID_PING_SIZE);
		buf[len] = UPID_PING;							len++;
		memcpy(&buf[len], &time, 8);						len += 8;
		for (i = 1; i < MAX_PLAYERS; i++)
		{
			PUT_INTEL_INT(buf + len, Netgame.players[i].ping);		len += 4;
		}
		
		for (i = 1; i < MAX_PLAYERS; i++)
		{
			if (Players[i].connected == CONNECT_DISCONNECTED)
				continue;
			dxx_sendto (UDP_Socket[0], buf, sizeof(buf), 0, (struct sockaddr *)&Netgame.players[i].protocol.udp.addr, sizeof(struct _sockaddr));
		}
		PingTime = time;
	}
}

// Got a PING from host. Apply the pings to our players and respond to host.
void net_udp_process_ping(ubyte *data, int data_len, struct _sockaddr sender_addr)
{
	fix64 host_ping_time = 0;
	ubyte buf[UPID_PONG_SIZE];
	int i, len = 0;
// 
	if (memcmp((struct _sockaddr *)&Netgame.players[0].protocol.udp.addr, (struct _sockaddr *)&sender_addr, sizeof(struct _sockaddr)))
		return;
	
										len++; // Skip UPID byte;
	memcpy(&host_ping_time, &data[len], 8);					len += 8;
	for (i = 1; i < MAX_PLAYERS; i++)
	{
		Netgame.players[i].ping = GET_INTEL_INT(&(data[len]));		len += 4;
	}
	
	buf[0] = UPID_PONG;
	buf[1] = Player_num;
	memcpy(&buf[2], &host_ping_time, 8);
	
	dxx_sendto (UDP_Socket[0], buf, sizeof(buf), 0, (struct sockaddr *)&sender_addr, sizeof(struct _sockaddr));
}

// Got a PONG from a client. Check the time and add it to our players.
void net_udp_process_pong(ubyte *data, int data_len, struct _sockaddr sender_addr)
{
	fix64 client_pong_time = 0;
	int i = 0;
	
	if (memcmp((struct _sockaddr *)&sender_addr, (struct _sockaddr *)&Netgame.players[data[1]].protocol.udp.addr, sizeof(struct _sockaddr)))
		return;

	if (data[1] >= MAX_PLAYERS || data[1] < 1)
		return;

	if (i == MAX_PLAYERS)
		return;
	
	memcpy(&client_pong_time, &data[2], 8);
	Netgame.players[data[1]].ping = f2i(fixmul(timer_query() - client_pong_time,i2f(1000)));
	
	if (Netgame.players[data[1]].ping < 0)
		Netgame.players[data[1]].ping = 0;
		
	if (Netgame.players[data[1]].ping > 9999)
		Netgame.players[data[1]].ping = 9999;
}

void net_udp_do_refuse_stuff (UDP_sequence_packet *their)
{
	int i,new_player_num;
	
	for (i=0;i<MAX_PLAYERS;i++)
	{
		if (!strcmp (their->player.callsign,Players[i].callsign))
		{
			net_udp_welcome_player(their);
			return;
		}
	}

	if (!WaitForRefuseAnswer)
	{
		for (i=0;i<MAX_PLAYERS;i++)
		{
			if (!strcmp (their->player.callsign,Players[i].callsign))
			{
				net_udp_welcome_player(their);
				return;
			}
		}
	
		digi_play_sample (SOUND_CONTROL_CENTER_WARNING_SIREN,F1_0*2);
	
		if (Game_mode & GM_TEAM)
		{
			HUD_init_message(HM_MULTI, "%s wants to join",their->player.callsign);
			HUD_init_message(HM_MULTI, "Alt-1 assigns to team %s. Alt-2 to team %s",their->player.callsign,Netgame.team_name[0],Netgame.team_name[1]);
		}
		else
		{
			HUD_init_message(HM_MULTI, "%s wants to join (accept: F6)",their->player.callsign);
		}
	
		strcpy (RefusePlayerName,their->player.callsign);
		RefuseTimeLimit=timer_query();
		RefuseThisPlayer=0;
		WaitForRefuseAnswer=1;
	}
	else
	{
		for (i=0;i<MAX_PLAYERS;i++)
		{
			if (!strcmp (their->player.callsign,Players[i].callsign))
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
				new_player_num=net_udp_get_new_player_num (their);
	
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

int net_udp_get_new_player_num (UDP_sequence_packet *their)
  {
	 int i;
	
	 their=their;
	
		if ( N_players < Netgame.max_numplayers)
			return (N_players);
		
		else
		{
			// Slots are full but game is open, see if anyone is
			// disconnected and replace the oldest player with this new one
		
			int oldest_player = -1;
			fix64 oldest_time = timer_query();

			Assert(N_players == Netgame.max_numplayers);

			for (i = 0; i < N_players; i++)
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

	if (Network_sending_extras==3 && (Netgame.PlayTimeAllowed || Netgame.KillGoal))
		multi_send_kill_goal_counts();
	if (Network_sending_extras==2)
		multi_send_powcap_update();
	if (Network_sending_extras==1 && Game_mode & GM_BOUNTY)
		multi_send_bounty();

	Network_sending_extras--;
	if (!Network_sending_extras)
		Player_joining_extras=-1;
}

static int show_game_rules_handler(window *wind, d_event *event, netgame_info *netgame)
{
	int k;
	int w = FSPACX(280), h = FSPACY(130);
	
	switch (event->type)
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
					window_close(wind);
					return 1;
			}
			break;
			
		case EVENT_WINDOW_DRAW:
			timer_delay2(50);

			gr_set_current_canvas(NULL);
			nm_draw_background(((SWIDTH-w)/2)-BORDERX,((SHEIGHT-h)/2)-BORDERY,((SWIDTH-w)/2)+w+BORDERX,((SHEIGHT-h)/2)+h+BORDERY);
			
			gr_set_current_canvas(window_get_canvas(wind));
			
			grd_curcanv->cv_font = MEDIUM3_FONT;
			
			gr_set_fontcolor(gr_find_closest_color_current(29,29,47),-1);
			gr_string( 0x8000, FSPACY(35), "NETGAME INFO" );
			
			grd_curcanv->cv_font = GAME_FONT;
			gr_printf( FSPACX( 25),FSPACY( 55), "Reactor Life:");
			gr_printf( FSPACX( 25),FSPACY( 61), "Max Time:");
			gr_printf( FSPACX( 25),FSPACY( 67), "Kill Goal:");
			gr_printf( FSPACX( 25),FSPACY( 73), "Packets per sec.:");
			gr_printf( FSPACX(155),FSPACY( 55), "Invul when reappearing:");
			gr_printf( FSPACX(155),FSPACY( 61), "Bright player ships:");
			gr_printf( FSPACX(155),FSPACY( 67), "Show enemy names on hud:");
			gr_printf( FSPACX(155),FSPACY( 73), "Show players on automap:");
			gr_printf( FSPACX(155),FSPACY( 79), "No friendly Fire:");
			gr_printf( FSPACX( 25),FSPACY(100), "Allowed Objects");
			gr_printf( FSPACX( 25),FSPACY(110), "Laser Upgrade:");
			gr_printf( FSPACX( 25),FSPACY(116), "Quad Laser:");
			gr_printf( FSPACX( 25),FSPACY(122), "Vulcan Cannon:");
			gr_printf( FSPACX( 25),FSPACY(128), "Spreadfire Cannon:");
			gr_printf( FSPACX( 25),FSPACY(134), "Plasma Cannon:");
			gr_printf( FSPACX( 25),FSPACY(140), "Fusion Cannon:");
			gr_printf( FSPACX(170),FSPACY(110), "Homing Missile:");
			gr_printf( FSPACX(170),FSPACY(116), "Proximity Bomb:");
			gr_printf( FSPACX(170),FSPACY(122), "Smart Missile:");
			gr_printf( FSPACX(170),FSPACY(128), "Mega Missile:");
			gr_printf( FSPACX( 25),FSPACY(150), "Invulnerability:");
			gr_printf( FSPACX( 25),FSPACY(156), "Cloak:");
			
			gr_set_fontcolor(gr_find_closest_color_current(255,255,255),-1);
			gr_printf( FSPACX(115),FSPACY( 55), "%i Min", netgame->control_invul_time/F1_0/60);
			gr_printf( FSPACX(115),FSPACY( 61), "%i Min", netgame->PlayTimeAllowed*5);
			gr_printf( FSPACX(115),FSPACY( 67), "%i", netgame->KillGoal*5);
			gr_printf( FSPACX(115),FSPACY( 73), "%i", netgame->PacketsPerSec);
			gr_printf( FSPACX(275),FSPACY( 55), netgame->InvulAppear?"ON":"OFF");
			gr_printf( FSPACX(275),FSPACY( 61), netgame->BrightPlayers?"ON":"OFF");
			gr_printf( FSPACX(275),FSPACY( 67), netgame->ShowEnemyNames?"ON":"OFF");
			gr_printf( FSPACX(275),FSPACY( 73), netgame->game_flags&NETGAME_FLAG_SHOW_MAP?"ON":"OFF");
			gr_printf( FSPACX(275),FSPACY( 79), netgame->NoFriendlyFire?"ON":"OFF");
			
			gr_printf( FSPACX(130),FSPACY(110), netgame->AllowedItems&NETFLAG_DOLASER?"YES":"NO");
			gr_printf( FSPACX(130),FSPACY(116), netgame->AllowedItems&NETFLAG_DOQUAD?"YES":"NO");
			gr_printf( FSPACX(130),FSPACY(122), netgame->AllowedItems&NETFLAG_DOVULCAN?"YES":"NO");
			gr_printf( FSPACX(130),FSPACY(128), netgame->AllowedItems&NETFLAG_DOSPREAD?"YES":"NO");
			gr_printf( FSPACX(130),FSPACY(134), netgame->AllowedItems&NETFLAG_DOPLASMA?"YES":"NO");
			gr_printf( FSPACX(130),FSPACY(140), netgame->AllowedItems&NETFLAG_DOFUSION?"YES":"NO");
			gr_printf( FSPACX(275),FSPACY(110), netgame->AllowedItems&NETFLAG_DOHOMING?"YES":"NO");
			gr_printf( FSPACX(275),FSPACY(116), netgame->AllowedItems&NETFLAG_DOPROXIM?"YES":"NO");
			gr_printf( FSPACX(275),FSPACY(122), netgame->AllowedItems&NETFLAG_DOSMART?"YES":"NO");
			gr_printf( FSPACX(275),FSPACY(128), netgame->AllowedItems&NETFLAG_DOMEGA?"YES":"NO");
			gr_printf( FSPACX(130),FSPACY(150), netgame->AllowedItems&NETFLAG_DOINVUL?"YES":"NO");
			gr_printf( FSPACX(130),FSPACY(156), netgame->AllowedItems&NETFLAG_DOCLOAK?"YES":"NO");
			gr_set_current_canvas(NULL);
			break;

		default:
			break;
	}
	
	return 0;
}

void net_udp_show_game_rules(netgame_info *netgame)
{
	gr_set_current_canvas(NULL);

	window_create(&grd_curscreen->sc_canvas, (SWIDTH - FSPACX(320))/2, (SHEIGHT - FSPACY(200))/2, FSPACX(320), FSPACY(200), 
				  (int (*)(window *, d_event *, void *))show_game_rules_handler, netgame);
}

static int show_game_info_handler(newmenu *menu, d_event *event, netgame_info *netgame)
{
	if (event->type != EVENT_NEWMENU_SELECTED)
		return 0;
	
	if (newmenu_get_citem(menu) != 1)
		return 0;

	net_udp_show_game_rules(netgame);
	
	return 1;
}

int net_udp_show_game_info()
{
	char rinfo[512],*info=rinfo;
	int c;
	netgame_info *netgame = &Netgame;

	memset(info,0,sizeof(char)*256);

	info+=sprintf(info,"\nConnected to\n\"%s\"\n",netgame->game_name);

	if(!netgame->mission_title)
		info+=sprintf(info,"Descent: First Strike");
	else
		info+=sprintf(info,"%s",netgame->mission_title);

   if( netgame->levelnum >= 0 )
   {
	   info+=sprintf (info," - Lvl %i",netgame->levelnum);
   }
   else
   {
      info+=sprintf (info," - Lvl S%i",(netgame->levelnum*-1));
   }

	info+=sprintf (info,"\n\nDifficulty: %s",MENU_DIFFICULTY_TEXT(netgame->difficulty));
	info+=sprintf (info,"\nGame Mode: %s",GMNames[netgame->gamemode]);
	info+=sprintf (info,"\nPlayers: %i/%i",netgame->numplayers,netgame->max_numplayers);

	c=nm_messagebox1("WELCOME", (int (*)(newmenu *, d_event *, void *))show_game_info_handler, netgame, 2, "JOIN GAME", "GAME INFO", rinfo);
	if (c==0)
		return 1;
	//else if (c==1)
	// handled in above callback
	else
		return 0;
}
#endif
