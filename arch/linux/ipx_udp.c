/* IPX driver for native Linux TCP/IP networking (UDP implementation)
 *   Version 0.99.2
 * Contact Jan [Lace] Kratochvil <short@ucw.cz> for assistance
 * (no "It somehow doesn't work! What should I do?" complaints, please)
 * Special thanks to Vojtech Pavlik <vojtech@ucw.cz> for testing.
 *
 * Also you may see KIX - KIX kix out KaliNix (in Linux-Linux only):
 *   http://atrey.karlin.mff.cuni.cz/~short/sw/kix.c.gz
 *
 * Primarily based on ipx_kali.c - "IPX driver for KaliNix interface"
 *   which is probably mostly by Jay Cotton <jay@kali.net>.
 * Parts shamelessly stolen from my KIX v0.99.2 and GGN v0.100
 *
 * Changes:
 * --------
 * 0.99.1 - now the default broadcast list also contains all point-to-point
 *          links with their destination peer addresses
 * 0.99.2 - commented a bit :-) 
 *        - now adds to broadcast list each host it gets some packet from
 *          which is already not covered by local physical ethernet broadcast
 *        - implemented short-signature packet format
 *        - compatibility mode for old D1X releases due to the previous bullet
 *
 * Configuration:
 * --------------
 * No network server software is needed, neither KIX nor KaliNIX.
 *
 * Add command line argument "-udp". In default operation D1X will send
 * broadcasts too all the local interfaces found. But you may also use
 * additional parameter specified after "-udp" to modify the default
 * broadcasting style and UDP port numbers binding:
 *
 * ./d1x -udp [@SHIFT]=HOST_LIST   Broadcast ONLY to HOST_LIST
 * ./d1x -udp [@SHIFT]+HOST_LIST   Broadcast both to local ifaces & to HOST_LIST
 *
 * HOST_LIST is a comma (',') separated list of HOSTs:
 * HOST is an IPv4 address (so-called quad like 192.168.1.2) or regular hostname
 * HOST can also be in form 'address:SHIFT'
 * SHIFT sets the UDP port base offset (e.g. +2), can be used to run multiple
 *       clients on one host simultaneously. This SHIFT has nothing to do
 *       with the dynamic-sockets (PgUP/PgDOWN) option in Descent, it's another
 *       higher-level differentiation option.
 *
 * Examples:
 * ---------
 * ./d1x -udp
 *  - Run D1X to participate in normal local network (Linux only, of course)
 *
 * ./d1x -udp @1=localhost:2 & ./d1x -udp @2=localhost:1
 *  - Run two clients simultaneously fighting each other (only each other)
 *
 * ./d1x -udp =192.168.4.255
 *  - Run distant Descent which will participate with remote network
 *    192.168.4.0 with netmask 255.255.255.0 (broadcast has 192.168.4.255)
 *  - You'll have to also setup hosts in that network accordingly:
 * ./d1x -udp +UPPER_DISTANT_MACHINE_NAME
 *
 * Have fun!
 */

#include <stdio.h>
#include <string.h>
#include <netinet/in.h> /* for htons & co. */
#include <unistd.h>
#include <stdarg.h>
#include <netdb.h>
#include <stdlib.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <ctype.h>

#include "ipx_drv.h"
#include "args.h"

extern unsigned char ipx_MyAddress[10];

// #define UDPDEBUG

#define UDP_BASEPORT 28342
#define PORTSHIFT_TOLERANCE 0x100
#define MAX_PACKETSIZE 8192

/* Packet format: first is the signature { 0xD1,'X' } which can be also
 * { 'D','1','X','u','d','p'} for old-fashioned packets.
 * Then follows virtual socket number (as changed during PgDOWN/PgUP)
 * in network-byte-order as 2 bytes (u_short). After such 4/8 byte header
 * follows raw data as communicated with D1X network core functions.
 */

// Length HAS TO BE 6!
#define D1Xudp "D1Xudp"
// Length HAS TO BE 2!
#define D1Xid "\xD1X"

static int open_sockets = 0;
static int dynamic_socket = 0x401;
static const int val_one=1;

/* OUR port. Can be changed by "@X[+=]..." argument (X is the shift value)
 */

static int baseport=UDP_BASEPORT;

/* Have we some old D1X in network and have we to maintain compatibility?
 * FIXME: Following scenario is braindead:
 *                                           A  (NEW) , B  (OLD) , C  (NEW)
 *   host A) We start new D1X.               A-newcomm, B-none   , C-none
 *   host B) We start OLD D1X.               A-newcomm, B-oldcomm, C-none
 *   Now host A hears host B and switches:   A-oldcomm, B-oldcomm, C-none
 *   host C) We start new D1X.               A-oldcomm, B-oldcomm, C-newcomm
 *   Now host C hears host A/B and switches: A-oldcomm, B-oldcomm, C-oldcomm
 *   Now host B finishes:                    A-oldcomm, B-none   , C-oldcomm
 *
 * But right now we have hosts A and C, both new code equipped but
 * communicating wastefully by the OLD protocol! Bummer.
 */

static char compatibility=0;

static int have_empty_address() {
	int i;
	for (i = 0; i < 10 && !ipx_MyAddress[i]; i++) ;
	return i == 10;
}

#define MSGHDR "IPX_udp: "

static void msg(const char *fmt,...)
{
va_list ap;
	fputs(MSGHDR,stdout);
	va_start(ap,fmt);
	vprintf(fmt,ap);
	va_end(ap);
	putchar('\n');
}

static void chk(void *p)
{
	if (p) return;
	msg("FATAL: Virtual memory exhausted!");
	exit(EXIT_FAILURE);
}

#define FAIL(m...) do { msg(#m); return -1; } while (0)

/* Find as much as MAX_BRDINTERFACES during local iface autoconfiguration.
 * Note that more interfaces can be added during manual configuration
 * or host-received-packet autoconfiguration
 */

#define MAX_BRDINTERFACES 16

/* We require the interface to be UP and RUNNING to accept it.
 */

#define IF_REQFLAGS (IFF_UP|IFF_RUNNING)

/* We reject any interfaces declared as LOOPBACK type.
 */
#define IF_NOTFLAGS (IFF_LOOPBACK)

static struct sockaddr_in *broads,broadmasks[MAX_BRDINTERFACES];
static int broadnum,masksnum,broadsize;

/* We'll check whether the "broads" array of destination addresses is now
 * full and so needs expanding.
 */

static void chkbroadsize(void)
{
	if (broadnum<broadsize) return;
	broadsize=broadsize?broadsize*2:8;
	chk(broads=realloc(broads,sizeof(*broads)*broadsize));
}

/* This function is called during init and has to grab all system interfaces
 * and collect their broadcast-destination addresses (and their netmasks).
 * Typically it founds only one ethernet card and so returns address in
 * the style "192.168.1.255" with netmask "255.255.255.0".
 * Broadcast addresses are filled into "broads", netmasks to "broadmasks".
 */

/* Stolen from my GGN */
static int addiflist(void)
{
unsigned cnt=MAX_BRDINTERFACES,i,j;
struct ifconf ifconf;
int sock;
struct sockaddr_in *sinp,*sinmp;

	free(broads);
	if ((sock=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP))<0)
		FAIL("Creating socket() failure during broadcast detection: %m");

#ifdef SIOCGIFCOUNT
	if (ioctl(sock,SIOCGIFCOUNT,&cnt))
		{ /* msg("Getting iterface count error: %m"); */ }
	else
		cnt=cnt*2+2;
#endif

	chk(ifconf.ifc_req=alloca((ifconf.ifc_len=cnt*sizeof(struct ifreq))));
	if (ioctl(sock,SIOCGIFCONF,&ifconf)||ifconf.ifc_len%sizeof(struct ifreq)) {
		close(sock);
		FAIL("ioctl(SIOCGIFCONF) failure during broadcast detection: %m");
		}
	cnt=ifconf.ifc_len/sizeof(struct ifreq);
	chk(broads=malloc(cnt*sizeof(*broads)));
	broadsize=cnt;
	for (i=j=0;i<cnt;i++) {
		if (ioctl(sock,SIOCGIFFLAGS,ifconf.ifc_req+i)) {
			close(sock);
			FAIL("ioctl(udp,\"%s\",SIOCGIFFLAGS) error: %m",ifconf.ifc_req[i].ifr_name);
			}
		if (((ifconf.ifc_req[i].ifr_flags&IF_REQFLAGS)!=IF_REQFLAGS)||
				 (ifconf.ifc_req[i].ifr_flags&IF_NOTFLAGS))
			continue;
		if (ioctl(sock,(ifconf.ifc_req[i].ifr_flags&IFF_BROADCAST?SIOCGIFBRDADDR:SIOCGIFDSTADDR),ifconf.ifc_req+i)) {
			close(sock);
			FAIL("ioctl(udp,\"%s\",SIOCGIF{DST/BRD}ADDR) error: %m",ifconf.ifc_req[i].ifr_name);
			}

		sinp =(struct sockaddr_in *)&ifconf.ifc_req[i].ifr_broadaddr;
		sinmp=(struct sockaddr_in *)&ifconf.ifc_req[i].ifr_netmask  ;
		if (sinp->sin_family!=AF_INET || sinmp->sin_family!=AF_INET) continue;
		broads[j]=*sinp;
		broads[j].sin_port=UDP_BASEPORT; //FIXME: No possibility to override from cmdline
		broadmasks[j]=*sinmp;
		j++;
		}
	broadnum=j;
	masksnum=j;
	return(0);
}

#define addreq(a,b) ((a).sin_port==(b).sin_port&&(a).sin_addr.s_addr==(b).sin_addr.s_addr)

/* Previous function addiflist() can (and probably will) report multiple
 * same addresses. On some Linux boxes is present both device "eth0" and
 * "dummy0" with the same IP addreesses - we'll filter it here.
 */

static void unifyiflist(void)
{
int d=0,s,i;

	for (s=0;s<broadnum;s++) {
		for (i=0;i<s;i++)
			if (addreq(broads[s],broads[i])) break;
		if (i>=s) broads[d++]=broads[s];
		}
	broadnum=d;
}

static unsigned char qhbuf[6];

/* Parse PORTSHIFT numeric parameter
 */

static void portshift(const char *cs)
{
long port;
unsigned short ports=0;

	port=atol(cs);
	if (port<-PORTSHIFT_TOLERANCE || port>+PORTSHIFT_TOLERANCE)
		msg("Invalid portshift in \"%s\", tolerance is +/-%d",cs,PORTSHIFT_TOLERANCE);
	else ports=htons(port);
	memcpy(qhbuf+4,&ports,2);
}

/* Do hostname resolve on name "buf" and return the address in buffer "qhbuf".
 */
static unsigned char *queryhost(char *buf)
{
struct hostent *he;
char *s;
char c=0;

	if ((s=strrchr(buf,':'))) {
	  c=*s;
	  *s='\0';
	  portshift(s+1);
	  }
	else memset(qhbuf+4,0,2);
	he=gethostbyname((char *)buf);
	if (s) *s=c;
	if (!he) {
		msg("Error resolving my hostname \"%s\"",buf);
		return(NULL);
		}
	if (he->h_addrtype!=AF_INET || he->h_length!=4) {
	  msg("Error parsing resolved my hostname \"%s\"",buf);
		return(NULL);
		}
	if (!*he->h_addr_list) {
	  msg("My resolved hostname \"%s\" address list empty",buf);
		return(NULL);
		}
	memcpy(qhbuf,(*he->h_addr_list),4);
	return(qhbuf);
}

/* Dump raw form of IP address/port by fancy output to user
 */
static void dumpraddr(unsigned char *a)
{
short port;
	printf("[%u.%u.%u.%u]",a[0],a[1],a[2],a[3]);
	port=(signed short)ntohs(*(unsigned short *)(a+4));
	if (port) printf(":%+d",port);
}

/* Like dumpraddr() but for structure "sockaddr_in"
 */

static void dumpaddr(struct sockaddr_in *sin)
{
unsigned short ports;

	memcpy(qhbuf+0,&sin->sin_addr,4);
	ports=htons(((short)ntohs(sin->sin_port))-UDP_BASEPORT);
	memcpy(qhbuf+4,&ports,2);
	dumpraddr(qhbuf);
}

/* Startup... Uninteresting parsing...
 */

static int ipx_udp_GetMyAddress(void) {

char buf[256];
int i;
char *s,*s2,*ns;

	if (!have_empty_address())
		return 0;

	if (!((i=FindArg("-udp")) && (s=Args[i+1]) && (*s=='=' || *s=='+' || *s=='@'))) s=NULL;

	if (gethostname(buf,sizeof(buf))) FAIL("Error getting my hostname");
	if (!(queryhost(buf))) FAIL("Querying my own hostname \"%s\"",buf);

	if (s) while (*s=='@') {
		portshift(++s);
		while (isdigit(*s)) s++;
		}

	memset(ipx_MyAddress+0,0,4);
	memcpy(ipx_MyAddress+4,qhbuf,6);
	baseport+=(short)ntohs(*(unsigned short *)(qhbuf+4));

	if (!s || (s && !*s)) addiflist();
	else {
		if (*s=='+') addiflist();
		s++;
		for (;;) {
struct sockaddr_in *sin;
			while (isspace(*s)) s++;
			if (!*s) break;
			for (s2=s;*s2 && *s2!=',';s2++);
			chk(ns=malloc(s2-s+1));
			memcpy(ns,s,s2-s);
			ns[s2-s]='\0';
			if (!queryhost(ns)) msg("Ignored broadcast-destination \"%s\" as being invalid",ns);
			free(ns);
			chkbroadsize();
			sin=broads+(broadnum++);
			sin->sin_family=AF_INET;
			memcpy(&sin->sin_addr,qhbuf+0,4);
			sin->sin_port=htons(((short)ntohs(*(unsigned short *)(qhbuf+4)))+UDP_BASEPORT);
			s=s2+(*s2==',');
			}
		}

	unifyiflist();

	printf(MSGHDR "Using TCP/IP address ");
	dumpraddr(ipx_MyAddress+4);
	putchar('\n');
	if (broadnum) {
		printf(MSGHDR "Using %u broadcast-dest%s:",broadnum,(broadnum==1?"":"s"));
		for (i=0;i<broadnum;i++) {
			putchar(' ');
			dumpaddr(broads+i);
			}
		putchar('\n');
		}

	return 0;
}

/* We should probably avoid such insanity here as during PgUP/PgDOWN
 * (virtual port number change) we wastefully destroy/create the same
 * socket here (binded always to "baseport"). FIXME.
 * "open_sockets" can be only 0 or 1 anyway.
 */

static int ipx_udp_OpenSocket(ipx_socket_t *sk, int port) {
struct sockaddr_in sin;

	if (!open_sockets)
		if (have_empty_address())
		if (ipx_udp_GetMyAddress() < 0) FAIL("Error getting my address");

	msg("OpenSocket on D1X socket port %d",port);

	if (!port)
		port = dynamic_socket++;

	if ((sk->fd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP)) < 0) {
		sk->fd = -1;
		FAIL("socket() creation failed on port %d: %m",port);
		}
  if (setsockopt(sk->fd,SOL_SOCKET,SO_BROADCAST,&val_one,sizeof(val_one))) {
		if (close(sk->fd)) msg("close() failed during error recovery: %m");
		sk->fd=-1;
		FAIL("setsockopt(SO_BROADCAST) failed: %m");
		}
	sin.sin_family=AF_INET;
	sin.sin_addr.s_addr=htonl(INADDR_ANY);
	sin.sin_port=htons(baseport);
	if (bind(sk->fd,(struct sockaddr *)&sin,sizeof(sin))) {
		if (close(sk->fd)) msg("close() failed during error recovery: %m");
		sk->fd=-1;
		FAIL("bind() to UDP port %d failed: %m",baseport);
		}

	open_sockets++;
	sk->socket = port;
	return 0;
}

/* The same comment as in previous "ipx_udp_OpenSocket"...
 */

static void ipx_udp_CloseSocket(ipx_socket_t *mysock) {
	if (!open_sockets) {
		msg("close w/o open");
		return;
	}
	msg("CloseSocket on D1X socket port %d",mysock->socket);
	if (close(mysock->fd))
		msg("close() failed on CloseSocket D1X socket port %d: %m",mysock->socket);
	mysock->fd=-1;
	if (--open_sockets) {
		msg("(closesocket) %d sockets left", open_sockets);
		return;
	}
}

/* Here we'll send the packet to our host. If it is unicast packet, send
 * it to IP address/port as retrieved from IPX address. Otherwise (broadcast)
 * we'll repeat the same data to each host in our broadcasting list.
 */

static int ipx_udp_SendPacket(ipx_socket_t *mysock, IPXPacket_t *IPXHeader,
 u_char *data, int dataLen) {
 	struct sockaddr_in toaddr,*dest;
	int i=dataLen;
	int bcast;
	char *buf;

#ifdef UDPDEBUG
	msg("SendPacket enter, dataLen=%d",dataLen);
#endif
	if (dataLen<0 || dataLen>MAX_PACKETSIZE) return -1;
	chk(buf=alloca(8+dataLen));
	if (compatibility) memcpy(buf+0,D1Xudp,6),buf+=6;
	else               memcpy(buf+0,D1Xid ,2),buf+=2;
	memcpy(buf+0,IPXHeader->Destination.Socket,2);
	memcpy(buf+2,data,dataLen);
 
 	toaddr.sin_family=AF_INET;
	memcpy(&toaddr.sin_addr,IPXHeader->Destination.Node+0,4);
	toaddr.sin_port=htons(((short)ntohs(*(unsigned short *)(IPXHeader->Destination.Node+4)))+UDP_BASEPORT);

	for (bcast=(toaddr.sin_addr.s_addr==htonl(INADDR_BROADCAST)?0:-1);bcast<broadnum;bcast++) {
		if (bcast>=0) dest=broads+bcast;
		else dest=&toaddr;
	
#ifdef UDPDEBUG
		printf(MSGHDR "sendto((%d),Node=[4] %02X %02X,Socket=%02X %02X,s_port=%u,",
			dataLen,
			IPXHeader->Destination.Node  [4],IPXHeader->Destination.Node  [5],
			IPXHeader->Destination.Socket[0],IPXHeader->Destination.Socket[1],
			ntohs(dest->sin_port));
		dumpaddr(dest);
		puts(").");
#endif
		i=sendto(mysock->fd,buf-(compatibility?6:2),(compatibility?8:4)+dataLen,
			0,(struct sockaddr *)dest,sizeof(*dest));
		if (bcast==-1) return (i<8?-1:i-8);
		}
	return(dataLen);
}

/* Here we will receive new packet to the given buffer. Both formats of packets
 * are supported, we fallback to old format when first obsolete packet is seen.
 * If the (valid) packet is received from unknown host, we will add it to our
 * broadcasting list. FIXME: For now such autoconfigured hosts are NEVER removed.
 */

static int ipx_udp_ReceivePacket(ipx_socket_t *s, char *outbuf, int outbufsize, 
 struct ipx_recv_data *rd) {
	int size;
	struct sockaddr_in fromaddr;
	int fromaddrsize=sizeof(fromaddr);
	unsigned short ports;
	size_t offs;
	int i;

	if ((size=recvfrom(s->fd,outbuf,outbufsize,0,(struct sockaddr *)&fromaddr,&fromaddrsize))<0)
		return -1;
#ifdef UDPDEBUG
	printf(MSGHDR "recvfrom((%d-8=%d),",size,size-8);
	dumpaddr(&fromaddr);
	puts(").");
#endif
	if (fromaddr.sin_family!=AF_INET) return -1;
	if (size<4) return -1;
	if (memcmp(outbuf+0,D1Xid,2)) {
		if (size<8 || memcmp(outbuf+0,D1Xudp,6)) return -1;
		if (!compatibility) {
			compatibility=1;
			fputs(MSGHDR "Received obsolete packet from ",stdout);
			dumpaddr(&fromaddr);
			puts(", upgrade that machine.\n" MSGHDR "Turning on compatibility mode...");
			}
		offs=6;
		}
	else offs=2;

	/* Lace: (dst_socket & src_socket) should be network-byte-order by comment in include/ipx_drv.h */
	/*       This behaviour presented here is broken. It is not used anywhere, so why bother? */
	rd->src_socket = ntohs(*(unsigned short *)(outbuf+offs));
	if (rd->src_socket != s->socket) {
#ifdef UDPDEBUG
		msg(" - pkt was dropped (dst=%d,my=%d)",rd->src_socket,s->socket);
#endif
		return -1;
		}
	rd->dst_socket = s->socket;

	for (i=0;i<broadnum;i++) {
		if (i>=masksnum) {
			if (addreq(fromaddr,broads[i])) break; 
			}
		else {
			if (fromaddr.sin_port==broads[i].sin_port
			&&( fromaddr.sin_addr.s_addr & broadmasks[i].sin_addr.s_addr)
			==(broads[i].sin_addr.s_addr & broadmasks[i].sin_addr.s_addr)) break;
			}
		}
	if (i>=broadnum) {
		chkbroadsize();
		broads[broadnum++]=fromaddr;
		fputs(MSGHDR "Adding host ",stdout);
		dumpaddr(&fromaddr);
		puts(" to broadcasting address list");
		}

	memmove(outbuf,outbuf+offs+2,size-(offs+2));
	size-=offs+2;

	memcpy(rd->src_node+0,&fromaddr.sin_addr,4);
	ports=htons(ntohs(fromaddr.sin_port)-UDP_BASEPORT);
	memcpy(rd->src_node+4,&ports,2);
	memset(rd->src_network, 0, 4);
	rd->pkt_type = 0;
#ifdef UDPDEBUG
	printf(MSGHDR "ReceivePacket: size=%d,from=",size);
	dumpraddr(rd->src_node);
	putchar('\n');
#endif

	return size;
}

struct ipx_driver ipx_udp = {
	ipx_udp_GetMyAddress,
	ipx_udp_OpenSocket,
	ipx_udp_CloseSocket,
	ipx_udp_SendPacket,
	ipx_udp_ReceivePacket,
	ipx_general_PacketReady
};
