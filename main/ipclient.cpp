/*
 * $Source: /cvs/cvsroot/d2x/main/ipclient.cpp,v $
 * $Revision: 1.1 $
 * $Author: bradleyb $
 * $Date: 2002-02-06 09:22:42 $
 *
 * ipclient.cpp - udp/ip client code
 * added 2000/02/07 Matt Mueller
 *
 * $Log: not supported by cvs2svn $
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

extern "C"{
#include "mono.h"
#include "key.h"
#include "text.h"
#include "newmenu.h"
#include "args.h"
void network_init(void);
int get_and_show_netgame_info(ubyte *server, ubyte *node, ubyte *net_address);
}

#include "ip_base.h"
#include "ipclient.h"


extern int Player_num;
extern int Network_send_objects;

extern int N_players;

int ip_connect_manual(char *textaddr) {
//  ip_handshake_info handshake;
//  ip_init_handshake_info(&handshake,0);
    int r;
    ubyte buf[1500];
    ip_peer *p;
	ip_addr addr;

    if (addr.dns(textaddr,UDP_BASEPORT)) {
		nm_messagebox(TXT_ERROR,1,TXT_OK,"Could not resolve address");
		return -1;
	}
#ifdef UDPDEBUG
    printf("connecting to ");
    addr.dump();
    printf("\n");
#endif

    network_init();
    N_players = 0;

    if (!(p=peer_list.find_by_addr(addr))){
        p=peer_list.add_1(addr);
    }
    ip_handshake_info *hsi=p->find_handshake();
	hsi->setstate(STATE_INEEDINFO);
   // p->newhandshake(hsi);
//	p->handshakes.push_back(hsi);
//  set_hs_state(&hsr->reply,IPHELLO);
//  ip_send_handshake(hsr,&hsr->reply);
    while(hsi->state&STATE_VALID_STATES){
        r=ipx_get_packet_data(buf);
        if (r>0)
            mprintf((0,MSGHDR "ip_connect_manual: weird, someone sent us normal data\n"));
        if (key_inkey()==KEY_ESC)
            return 0;
    }
    if (hsi->state&STATE_ERR) {
		nm_messagebox(TXT_ERROR,1,TXT_OK,"handshake timeout");
		return -1;
	}

//  join the Netgame.

    return get_and_show_netgame_info(null_addr,p->id.id,NULL);
}


static int ipx_ip_OpenSocket(int oport) {
	int i;
	if ((i=FindArg("-ip_baseport"))){
		baseport=UDP_BASEPORT+atoi(Args[i+1]);
	}

	myport=baseport+(oport - IPX_DEFAULT_SOCKET);
	if (arch_ip_open_socket(myport)) return -1;

	if (ipx_ip_GetMyAddress() < 0) FAIL("Error getting my address");

	//	if (!port)
	//		port = dynamic_socket++;
#ifdef UDPDEBUG
	msg("OpenSocket on D1X socket port %d (%d : %+d) : %+d",myport,oport,oport-IPX_DEFAULT_SOCKET,baseport-UDP_BASEPORT);
#endif
	//sk->socket = UDP_BASEPORT+(oport - IPX_DEFAULT_SOCKET);
//	sk->socket = port;
	return 0;
}
static void ipx_ip_CloseSocket(void) {
#ifdef UDPDEBUG
        msg("CloseSocket on D1X socket port %d",myport);
#endif
	arch_ip_close_socket();
	peer_list.clear();
}



/* Here we'll send the packet to our host. If it is unicast packet, send
 * it to IP address/port as retrieved from IPX address. Otherwise (broadcast)
 * we'll repeat the same data to each host in our broadcasting list.
 */

static int ipx_ip_SendPacket(IPXPacket_t *IPXHeader,
		ubyte *data, int dataLen) {
//	struct sockaddr_in toaddr;
	int i=dataLen;
	//int bcast;
	char *buf;

	if (dataLen<0 || dataLen>MAX_PACKETSIZE) {
#ifdef UDPDEBUG
		msg("SendPacket enter, dataLen=%d out of range",dataLen);
#endif
		return -1;
	}
	chk(buf=(char*)alloca(2+dataLen));
	memcpy(buf+0,D1Xid ,2);
	memcpy(buf+2,data,dataLen);

#ifdef UDPDEBUG
	printf(MSGHDR "sendto((%d),Node=[4] %02X %02X,Socket=%02X %02X,s_port=%u,",
			dataLen,
			IPXHeader->Destination.Node [4],IPXHeader->Destination.Node  [5],
			IPXHeader->Destination.Socket[0],IPXHeader->Destination.Socket[1],
			ntohs(*(unsigned short *)(IPXHeader->Destination.Socket)));
	dumprid(IPXHeader->Destination.Node);
	puts(").");
#endif

	//toaddr.sin_family=AF_INET;
	if (memcmp(broadcast_addr,IPXHeader->Destination.Node,6)==0){
//		ubyte brbuf[6];
		ip_addr brbuf;
//		ushort port=htons(myport);
//		//int j;
//		memcpy(brbuf+0,IPXHeader->Destination.Node+0,4);
//		//memcpy(brbuf+4,IPXHeader->Destination.Socket,2);
//		memcpy(brbuf+4,&port,2);
		brbuf.set(4,IPXHeader->Destination.Node,htons(myport));
		i=ip_sendtoca(brbuf,buf,2+dataLen);
//		memcpy(&toaddr.sin_addr,IPXHeader->Destination.Node+0,4);
//		toaddr.sin_port=*(unsigned short *)(IPXHeader->Destination.Socket);
		ip_sendtoall(buf,2+dataLen);
	}else {
		i=ip_sendtoid(IPXHeader->Destination.Node,buf,2+dataLen);
//		memcpy(&toaddr.sin_addr,hsr->addr[hsr->goodaddr],4);
//		toaddr.sin_port=*(unsigned short *)(hsr->addr[hsr->goodaddr]+4);
////		toaddr.sin_port=*(unsigned short *)(IPXHeader->Destination.Node+4);
	}
		//toaddr.sin_port=htons(((short)ntohs(*(unsigned short *)(IPXHeader->Destination.Node+4)))+UDP_BASEPORT);
//	toaddr.sin_port=*(unsigned short *)(IPXHeader->Destination.Socket);

//	i=sendto(mysock->fd,buf,2+dataLen,
//			0,(struct sockaddr *)&toaddr,sizeof(toaddr));
	return(i<1?-1:i-1);
}

/* Here we will receive new packet to the given buffer. Both formats of packets
 * are supported, we fallback to old format when first obsolete packet is seen.
 * If the (valid) packet is received from unknown host, we will add it to our
 * broadcasting list. FIXME: For now such autoconfigured hosts are NEVER removed.
 */

static int ipx_ip_ReceivePacket(char *outbuf, int outbufsize, 
 struct ipx_recv_data *rd) {
	int size;
	size_t offs;
	ip_addr *fromaddr;
	//int i;

	if ((size=arch_ip_recvfrom(outbuf,outbufsize,rd))<0)
		return -1;

	if (size<2) return -1;

	memcpy(&fromaddr,rd->src_node,sizeof(ip_addr*));

	if (memcmp(outbuf+0,D1Xid,2)) {
		if (memcmp(outbuf+0,D1Xcfgid,4)) {
			mprintf((0,MSGHDR"no valid header\n"));
			return -1;
		}
		{
/*			ubyte buf[6];
			memcpy(buf,&fromaddr.sin_addr,4);
			*(unsigned short *)(buf+4)=fromaddr.sin_port;*/
			//do stuff.
//			fromaddr.set(4,rd->src_node,*(unsigned short*)(rd->src_node+4));
			ip_receive_cfg((ubyte*)outbuf+4,size-4,*fromaddr);
		}
		return 0;
	}
	else offs=2;

	/* Lace: (dst_socket & src_socket) should be network-byte-order by comment in include/ipx_drv.h */
	/*       This behaviour presented here is broken. It is not used anywhere, so why bother? */
/*	rd->src_socket = ntohs(*(unsigned short *)(outbuf+offs));
	if (rd->src_socket != s->socket) {
#ifdef UDPDEBUG
		msg(" - pkt was dropped (dst=%d,my=%d)",rd->src_socket,s->socket);
#endif
		return -1;
		}*/
//	rd->src_socket = s->socket;
//	rd->dst_socket = s->socket;

	memmove(outbuf,outbuf+offs,size-(offs));
	size-=offs;

	rd->src_socket = myport;
    rd->dst_socket = myport;

	rd->pkt_type = 0;
#ifdef UDPDEBUG
	printf(MSGHDR "ReceivePacket: size=%d,from=",size);
//	dumpraddr(rd->src_node);
	fromaddr->dump();
	putchar('\n');
#endif
/*	ip_peer *p=peer_list.find_by_addr(*fromaddr);
	if(p)
		p->id.fillbuf(rd->src_node);
	else
		memset(rd->src_node,0,6);*/
	if (ip_my_addrs.hasaddr(*fromaddr))
		memcpy(rd->src_node,ipx_MyAddress+4,6);
	else
		memset(rd->src_node,0,6);

	return size;
}

static int ipx_ip_general_PacketReady(void) {
	//ip_handshake_frame();//handle handshake resending
	peer_list.handshake_frame();
	return arch_ip_PacketReady();
}

static int ipx_ip_CheckReadyToJoin(ubyte *server, ubyte *node){
	if (Network_send_objects) return 0;//if we are currently letting someone else join, we don't know if this person can join ok.

	ip_peer *p=peer_list.find_byid(node);
	if (!p || p->addr.goodaddr==NULL)
		return 0;
	ip_peer *np;
	ip_handshake_relay *hsr;
	int j;
	int ok,nope=0;

	for (j=0;j<get_MAX_PLAYERS();j++){
		if (get_Netgame_player_connected(j) && j!=Player_num){
			np=peer_list.find_byid(get_Netgame_player_node(j));
			if (!np)continue;//huh??
			if (np->id==node)continue;//don't tell them to talk to themselves.

			ok=0;

			hsr=p->find_relay(np->id);
			if (!hsr){
				hsr=new ip_handshake_relay(np);
				p->add_hs(hsr);
			}
			if (hsr->state==0)
				ok++;

			hsr=np->find_relay(p->id);
			if (!hsr){
				hsr=new ip_handshake_relay(p);
				np->add_hs(hsr);
			}
			if (hsr->state==0)
				ok++;
			if (ok!=2)
				nope++;
		}
	}
	if (nope)
		return 0;

	return 1;
}

struct ipx_driver ipx_ip = {
//	ipx_ip_GetMyAddress,
	ipx_ip_OpenSocket,
	ipx_ip_CloseSocket,
	ipx_ip_SendPacket,
	ipx_ip_ReceivePacket,
	ipx_ip_general_PacketReady,
	ipx_ip_CheckReadyToJoin,
	0, //save 4 bytes.  udp/ip is completely inaccessable by the other methods, so we don't need to worry about compatibility.
	NULL, //use the easier ones
	NULL, //use the easier ones
	NULL  //use the easier ones
};
