/*
 * $Source: /cvs/cvsroot/d2x/arch/win32/arch_ip.cpp,v $
 * $Revision: 1.2 $
 * $Author: bradleyb $
 * $Date: 2002-02-15 06:41:41 $
 *
 * arch_ip.cpp - arch specific udp/ip code.  (win32 ver)
 * added 2000/02/07 Matt Mueller (some code borrowed from ipx_udp.c)
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2002/02/15 02:29:33  bradleyb
 * copied from d1x
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
extern "C" {
#include "ipx_drv.h"
#include "pstypes.h"
#include "mono.h"
//#include "multi.h"
}

#include "ip_base.h"
#include "ipclient.h"

//static int dynamic_socket=0x402;

static int mysock=-1;

//static unsigned char qhbuf[6];


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
	con_printf(").\n");
#endif
	return sendto(mysock,(char*)buf,len,0,(struct sockaddr *)&toaddr,sizeof(toaddr));
}


int arch_ip_get_my_addr(u_short myport){
	char buf[256];
	if (gethostname(buf,sizeof(buf))) FAIL("Error getting my hostname");

	ip_addr ip;
	if (ip.dns(buf,myport)) FAIL("Error querying my own hostname \"%s\"",buf);
	ip_my_addrs.add(ip);
	return 0;
}
int arch_ip_open_socket(int port){
	struct sockaddr_in sin;
	int val_one=1;
	WORD wVersionRequested;
	WSADATA wsaData;

	wVersionRequested = MAKEWORD(2, 0);
	if (WSAStartup( wVersionRequested, &wsaData))
	{
		return -1;
	}
#if 0
	if ( LOBYTE( wsaData.wVersion ) != 2 ||
			HIBYTE( wsaData.wVersion ) != 0 ) {
		/* We couldn't find a usable WinSock DLL. */
		WSACleanup( );
		return -2;
	}
#endif

	if ((mysock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP)) < 0) {
		mysock = -1;
		FAIL("socket() creation failed on port %d: %m",port);
	}
	if (setsockopt(mysock,SOL_SOCKET,SO_BROADCAST,(char*)&val_one,sizeof(val_one))) {
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
	if (closesocket(mysock))
		msg("close() failed on CloseSocket D1X ip socket %m");
	mysock=-1;
	WSACleanup();
}
int arch_ip_recvfrom(char*outbuf,int outbufsize,struct ipx_recv_data *rd){
	struct sockaddr_in fromaddr;
	int fromaddrsize=sizeof(fromaddr);
	int size;
	static ip_addr ip_fromaddr;
	ip_addr *vptr=&ip_fromaddr;

	if ((size=recvfrom(mysock,outbuf,outbufsize,0,(struct sockaddr *)&fromaddr,&fromaddrsize))<0)
		return -1;

	if (fromaddr.sin_family!=AF_INET) return -1;

#ifdef UDPDEBUG
	con_printf(CON_DEBUG, MSGHDR "recvfrom((%d-2=%d),",size,size-2);
	dumpaddr(&fromaddr);
	con_printf(CON_DEBUG(").\n");
#endif

	ip_fromaddr.set(4,(u_char*)&fromaddr.sin_addr,fromaddr.sin_port);

	memcpy(rd->src_node,&vptr,sizeof(ip_addr*));
	return size;
}
int arch_ip_PacketReady(void) {
	return ipx_general_PacketReady(mysock);
}
