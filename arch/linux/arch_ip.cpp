/*
 * $Source: /cvs/cvsroot/d2x/arch/linux/arch_ip.cpp,v $
 * $Revision: 1.2 $
 * $Author: bradleyb $
 * $Date: 2002-02-15 06:41:41 $
 *
 * arch_ip.cpp - arch specific udp/ip code.  (linux ver)
 * added 2000/02/07 Matt Mueller (some code borrowed from ipx_udp.c)
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2002/02/06 09:22:41  bradleyb
 * Adding d1x network code
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <string.h>
#include <netinet/in.h> /* for htons & co. */
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <ctype.h>
extern "C" {
#include "ipx_drv.h"
#include "pstypes.h"
#include "mono.h"
//#include "multi.h"
}

#include "ip_base.h"
#include "ipclient.h"

static int mysock=-1;

/* Do hostname resolve on name "buf" and return the address in buffer "qhbuf".
 */
int arch_ip_queryhost(ip_addr *addr,char *buf,u_short baseport)
{
	struct hostent *he;
	char *s;
	char c=0;
	u_short p;

	if ((s=strrchr(buf,':'))) {
		c=*s;
		*s='\0';
		p=ip_portshift(baseport,s+1);
	}
	else p=ip_portshift(baseport,NULL);//memset(qhbuf+4,0,2);
	he=gethostbyname((char *)buf);
	if (s) *s=c;
	if (!he) {
		msg("Error resolving my hostname \"%s\"",buf);
		return -1; //(NULL);
	}
	if (he->h_addrtype!=AF_INET || he->h_length!=4) {
		msg("Error parsing resolved my hostname \"%s\"",buf);
		return -1;//(NULL);
	}
	if (!*he->h_addr_list) {
		msg("My resolved hostname \"%s\" address list empty",buf);
		return -1;//(NULL);
	}
	addr->set(4,(u_char*)*he->h_addr_list,p);
//	memcpy(qhbuf,(*he->h_addr_list),4);
//	return(qhbuf);
	return 0;
}

#ifdef UDPDEBUG
/* Like dumpraddr() but for structure "sockaddr_in"
 */

static void dumpaddr(struct sockaddr_in *sin)
{
	unsigned short ports;
	u_char qhbuf[6];

	memcpy(qhbuf+0,&sin->sin_addr,4);
	//ports=htons(((short)ntohs(sin->sin_port))-UDP_BASEPORT);
	ports=sin->sin_port;
	memcpy(qhbuf+4,&ports,2);
	dumpraddr(qhbuf);
}
#endif


int ip_sendtoca(ip_addr addr,const void *buf,int len){
	struct sockaddr_in toaddr;
	if (addr.addrlen()!=4) return 0;//not handled yet
	toaddr.sin_family=AF_INET;
	memcpy(&toaddr.sin_addr,addr.addr,4);
	toaddr.sin_port=*(unsigned short *)(addr.addr+4);
#ifdef UDPDEBUG
	con_printf(CON_DEBUG, MSGHDR "sendtoca((%d) ",len);
	//dumpraddr(addr.addr);
	addr.dump();
	con_printf(CON_DEBUG, ").\n");
#endif
	return sendto(mysock,buf,len,0,(struct sockaddr *)&toaddr,sizeof(toaddr));
}

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

int arch_ip_get_my_addr(u_short myport){
	unsigned cnt=MAX_BRDINTERFACES,i,j;
	struct ifconf ifconf;
	int sock;
	struct sockaddr_in *sinp;
	ip_addr addr;

	if ((sock=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP))<0)
		FAIL("Creating socket() failure during broadcast detection: %m");

#ifdef SIOCGIFCOUNT
	if (ioctl(sock,SIOCGIFCOUNT,&cnt))
	{ /* msg("Getting iterface count error: %m"); */ }
	else
		cnt=cnt*2+2;
#endif

	chk(ifconf.ifc_req=(struct ifreq *)alloca((ifconf.ifc_len=cnt*sizeof(struct ifreq))));
	if (ioctl(sock,SIOCGIFCONF,&ifconf)||ifconf.ifc_len%sizeof(struct ifreq)) {
		close(sock);
		FAIL("ioctl(SIOCGIFCONF) failure during broadcast detection: %m");
	}
	cnt=ifconf.ifc_len/sizeof(struct ifreq);
	//	chk(broads=malloc(cnt*sizeof(*broads)));
	//	broadsize=cnt;
	for (i=j=0;i<cnt;i++) {
		if (ioctl(sock,SIOCGIFFLAGS,ifconf.ifc_req+i)) {
			close(sock);
			FAIL("ioctl(udp,\"%s\",SIOCGIFFLAGS) error: %m",ifconf.ifc_req[i].ifr_name);
		}
		if (((ifconf.ifc_req[i].ifr_flags&IF_REQFLAGS)!=IF_REQFLAGS)||
				(ifconf.ifc_req[i].ifr_flags&IF_NOTFLAGS))
			continue;
		if (ioctl(sock,SIOCGIFDSTADDR,ifconf.ifc_req+i)) {
			close(sock);
			FAIL("ioctl(udp,\"%s\",SIOCGIFDSTADDR) error: %m",ifconf.ifc_req[i].ifr_name);
		}

		sinp=(struct sockaddr_in *)&ifconf.ifc_req[i].ifr_netmask  ;
		if (sinp->sin_family!=AF_INET) continue;
		addr.set(4,(ubyte*)&sinp->sin_addr,htons(myport));
#ifdef UDPDEBUG
		con_printf(CON_DEBUG, MSGHDR"added if ");
		addr.dump();
		con_printf(CON_DEBUG, "\n");
#endif
		ip_my_addrs.add(addr);
	}
	return(0);
}

/*int arch_ip_get_my_addr(u_short myport){
	char buf[256];
	if (gethostname(buf,sizeof(buf))) FAIL("Error getting my hostname");

	ip_addr ip;
	if (ip.dns(buf,myport)) FAIL("Error querying my own hostname \"%s\"",buf);
	ip_my_addrs.add(ip);
	return 0;
}*/
int arch_ip_open_socket(int port){
	struct sockaddr_in sin;
	int val_one=1;
	if ((mysock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP)) < 0) {
		mysock = -1;
		FAIL("socket() creation failed on port %d: %m",port);
	}
	if (setsockopt(mysock,SOL_SOCKET,SO_BROADCAST,&val_one,sizeof(val_one))) {
		if (close(mysock)) msg("close() failed during error recovery: %m");
		mysock=-1;
		FAIL("setsockopt(SO_BROADCAST) failed: %m");
	}
	sin.sin_family=AF_INET;
	sin.sin_addr.s_addr=htonl(INADDR_ANY);
	sin.sin_port=htons(port);

	if (bind(mysock,(struct sockaddr *)&sin,sizeof(sin))) {
		if (close(mysock)) msg("close() failed during error recovery: %m");
		mysock=-1;
		FAIL("bind() to UDP port %d failed: %m",port);
	}

	return 0;
}
void arch_ip_close_socket(void) {
	if (mysock<0) {
		msg("close w/o open");
		return;
	}
	if (close(mysock))
		msg("close() failed on CloseSocket D1X ip socket %m");
	mysock=-1;
}
int arch_ip_recvfrom(char*outbuf,int outbufsize,struct ipx_recv_data *rd){
	struct sockaddr_in fromaddr;
	socklen_t fromaddrsize=sizeof(fromaddr);
	int size;
	static ip_addr ip_fromaddr;
	ip_addr *vptr=&ip_fromaddr;

	if ((size=recvfrom(mysock,outbuf,outbufsize,0,(struct sockaddr *)&fromaddr,&fromaddrsize))<0)
		return -1;

	if (fromaddr.sin_family!=AF_INET) return -1;

#ifdef UDPDEBUG
	con_printf(CON_DEBUG, MSGHDR "recvfrom((%d-2=%d),",size,size-2);
	dumpaddr(&fromaddr);
	con_printf(CON_DEBUG, ").\n");
#endif

	ip_fromaddr.set(4,(u_char*)&fromaddr.sin_addr,fromaddr.sin_port);

	memcpy(rd->src_node,&vptr,sizeof(ip_addr*));
	return size;
}
int arch_ip_PacketReady(void) {
	return ipx_general_PacketReady(mysock);
}
