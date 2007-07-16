/*
 * ip_base.cpp - base for NAT-compatible udp/ip code.
 * added 2000/02/07 Matt Mueller
 */
#ifdef HAVE_CONFIG_H
# include "conf.h"
#endif

extern "C" {
#include "timer.h"
#include "mono.h"
#include "args.h"
}
#include <string.h>
#include "ip_base.h"
#include <assert.h>
#ifndef _WIN32
#include <netinet/in.h>
#endif

int myport=-1;

int baseport=UDP_BASEPORT;

ip_addr_list ip_my_addrs;
ip_peer_list peer_list;

int ip_handshake_base::fillbuf(ubyte *buf){
	u_int32_t tmp32;
	u_int16_t tmp16;
	int pos=0;
	buf[pos++]=type;
	buf[pos++]=state;
	pos+=id.fillbuf(buf+pos);
	tmp16=htons(iver);
	memcpy(buf+pos,&tmp16,2);pos+=2;
	tmp32=htonl(tryid);
	memcpy(buf+pos,&tmp32,4);pos+=4;
	return pos;
}

int ip_handshake_base::readbuf(ubyte *buf){
	int pos=0;
	nopend=1;
	type=buf[pos++];
	state=buf[pos++];
	pos+=id.readbuf(buf+pos);
	memcpy(&iver,buf+pos,2);pos+=2;
	iver=htons(iver);
	memcpy(&tryid,buf+pos,4);pos+=4;
	tryid=htonl(tryid);
	return pos;
}

void ip_handshake_base::setstate(int newstate){
	if (!nopend){
		if (state&STATE_NEED_RESEND){
			if (!(newstate&STATE_NEED_RESEND))
				peer_list.pendinghandshakes--;
		}else{
			if (newstate&STATE_NEED_RESEND)
				peer_list.pendinghandshakes++;
		}
		mprintf((0,"peer_list.pendinghandshakes=%i\n",peer_list.pendinghandshakes));
	}
	state=newstate;attempts=0;nextsend=0;
}

int ip_handshake_base::addstate(int newstate){
	if (state & STATE_ERR){
		setstate((state | newstate) & ~STATE_ERR);
		return 2;
	}
	if ((state | newstate) != state){
		setstate(state | newstate);
		return 1;
	}
	return 0;
}

int ip_handshake_info::fillbuf(ubyte *buf){
	int pos=ip_handshake_base::fillbuf(buf);
	pos+=addr.fillbuf(buf+pos);
	return pos;
}

int ip_handshake_info::readbuf(ubyte *buf){
	int pos=ip_handshake_base::readbuf(buf);
	pos+=addr.readbuf(buf+pos);
	return pos;
}

int ip_handshake_relay::fillbuf(ubyte *buf){
	u_int16_t tmp16;
	int pos=ip_handshake_base::fillbuf(buf);
	pos+=r_id.fillbuf(buf+pos);
	tmp16=htons(r_iver);
	memcpy(buf+pos,&tmp16,2);pos+=2;
	pos+=r_addr.fillbuf(buf+pos);
	return pos;
}

int ip_handshake_relay::readbuf(ubyte *buf){
	int pos=ip_handshake_base::readbuf(buf);
	pos+=r_id.readbuf(buf+pos);
	memcpy(&r_iver,buf+pos,2);pos+=2;
	r_iver=htons(r_iver);
	pos+=r_addr.readbuf(buf+pos);
	return pos;

}
ip_handshake_relay::ip_handshake_relay(ip_peer *torelay):ip_handshake_base(1){
	type=IP_CFG_RELAY;
	r_id=torelay->id;
	r_iver=torelay->iver;
	r_addr=torelay->addr;
	setstate(STATE_INEEDINFO);
}

void ip_peer::send_handshake(ip_handshake_base*hsb){
	ubyte buf[256];
	int s=0;
	memcpy(buf+s,D1Xcfgid,4);s+=4;
	s+=hsb->fillbuf(buf+s);
	assert(s<256);
	if (addr.goodaddr==NULL){
		ip_addr_list::iterator i;
		for (i=addr.begin();i!=addr.end();i++)
			ip_sendtoca(*i,buf,s);
	}else
		ip_sendtoca(*addr.goodaddr,buf,s);
	hsb->nextsend=timer_get_approx_seconds()+IP_HS_RETRYTIME;
	hsb->attempts++;
}

bool ip_peer::verify_addr(ip_addr_list &fraddrs){
	int a1=addr.add(fraddrs);
	if (a1)
		addr.goodaddr=NULL;

	return a1>0;
}

ip_peer * ip_peer_list::add_1(ip_addr addr){
	ip_peer*n=add_unk();
	n->addr.add(addr);

	return n;
}

ip_peer * ip_peer_list::add_full(ip_id id, u_int16_t iver,ip_addr_list &addrs){
	ip_peer*n=add_id(id);
	n->addr.add(addrs);
	n->iver=iver;
	mprintf((0,"addfull %i addrs\n",n->addr.naddr));
	return n;
}

void ip_peer_list::make_full(ip_peer*p,ip_id id, u_int16_t iver, ip_addr_list &addrs){
	list<ip_peer*>::iterator unki=find(unknown_peers.begin(),unknown_peers.end(),p);
	if (unki!=unknown_peers.end())
		unknown_peers.erase(unki);
	p->id=id;
	p->iver=iver;
	p->addr.add(addrs);
	peers.insert(peer_map::value_type(p->id,p));
}

ip_peer * ip_peer_list::find_byid(ip_id id){
	peer_map::iterator i=peers.find(id);
	if (i!=peers.end())
		return (*i).second;
	return NULL;
}

struct do_find_by_addr : public unary_function<ip_peer*,bool> {
	int j;
	ip_addr addr;
	bool operator()(ip_peer * p) {
		if (p->addr.hasaddr(addr))
			return 1;
		return 0;
	}
};

template <class ret,class p1,class p2, class fo>
struct pairkiller {
	fo *o;
	ret operator()(pair<p1,p2> &aoeuaoeu) {
		return (*o)(aoeuaoeu.second);
	}
	pairkiller(fo *no):o(no){}
};

template <class p1, class fo>
pairkiller<typename fo::result_type,p1,typename fo::argument_type,fo> pairkill(fo * no){
	return pairkiller<typename fo::result_type,p1,typename fo::argument_type,fo>(no);
}

ip_peer * ip_peer_list::find_by_addr(ip_addr addr){
	do_find_by_addr fba;
	fba.addr=addr;
	peer_map::iterator i=find_if(peers.begin(),peers.end(),pairkill<const ip_id>(&fba));
	if (i!=peers.end())
		return (*i).second;
	list<ip_peer*>::iterator j=find_if(unknown_peers.begin(),unknown_peers.end(),fba);
	if (j!=unknown_peers.end())
		return (*j);
	return NULL;
}

ip_peer * ip_peer_list::find_unk_by_addr(ip_addr addr){
	do_find_by_addr fba;
	fba.addr=addr;
	list<ip_peer*>::iterator j=find_if(unknown_peers.begin(),unknown_peers.end(),fba);
	if (j!=unknown_peers.end())
		return (*j);
	return NULL;
}

struct do_peer_handshake : public unary_function<ip_peer *, void>{
	fix mintime;
	list<ip_handshake_base*>::iterator i;
	ip_handshake_base *hsb;
	void operator() (ip_peer* peer) {
		for(i=peer->handshakes.begin();i!=peer->handshakes.end();++i){
			hsb=(*i);
			//if (hsb->state<IPNOSTATE && hsb->nextsend<mintime){
			if ((hsb->state & STATE_NEED_RESEND) && hsb->nextsend<mintime){
				if(hsb->attempts>IP_MAX_HS_ATTEMPTS){
					hsb->setstate(STATE_ERR);
				}else{
					peer->send_handshake(hsb);
				}
			}
		}
	}
};

void ip_peer_list::handshake_frame(void){
	if(pendinghandshakes){
		fix mintime=timer_get_approx_seconds();//try every X seconds
		if (pendinghandshake_lasttime<mintime-IP_HS_FRAME_RETRYTIME){
			do_peer_handshake doph;
			doph.mintime=mintime;
			for_each(peers.begin(),peers.end(),pairkill<const ip_id>(&doph));
			for_each(unknown_peers.begin(),unknown_peers.end(),doph);
		}
	}
}

struct do_peer_delete : public unary_function<ip_peer *, void>{
	void operator() (ip_peer* peer) {
		delete peer;
	}
};

void ip_peer_list::clear(void){
	do_peer_delete dopd;
	for_each(peers.begin(),peers.end(),pairkill<const ip_id>(&dopd));
	peers.erase(peers.begin(),peers.end());
	for_each(unknown_peers.begin(),unknown_peers.end(),dopd);
	unknown_peers.erase(unknown_peers.begin(),unknown_peers.end());
}

ip_peer_list::~ip_peer_list(){
	clear();
}


#ifdef UDPDEBUG
//000a0oeuaoeu

static void dumpid(unsigned char *a)
{
	printf("<%u.%u.%u.%u.%u.%u>",a[0],a[1],a[2],a[3],a[4],a[5]);
}
#endif

int ip_sendtoid(ubyte *id,const void *buf,int len){
	ip_peer *p=peer_list.find_byid(id);
	if (!p || p->addr.goodaddr==NULL){
#ifdef UDPDEBUG
		printf(MSGHDR"send to invalid id(");
		dumpid(id);
		printf(") %p.",p);
#endif
		return -1;
	}
	return ip_sendtoca(*p->addr.goodaddr,buf,len);
}

void ip_receive_cfg(ubyte *buf,int buflen,ip_addr fromaddr){
	ip_peer *p;
	switch(buf[0]){
		case IP_CFG_SORRY:
			{
#ifdef UDPDEBUG
	printf("ip_receive_cfg: %i %i ",buf[0],buf[1]);
	dumprid(buf+2);
        printf(" v%i tryid%u\n",ntohs(*(unsigned short*)(buf+8)),(unsigned int)ntohl(*(u_int32_t*)(buf+10)));
#endif

			}break;
		case IP_CFG_HANDSHAKE:
#ifdef UDPDEBUG
	printf("ip_receive_cfg: %i %i ",buf[0],buf[1]);
	dumprid(buf+2);
        printf(" v%i tryid%u\n",ntohs(*(unsigned short*)(buf+8)),(unsigned int)ntohl(*(u_int32_t*)(buf+10)));
#endif
			{
				ip_handshake_info *hsi=new ip_handshake_info(buf);
				hsi->addr.add(fromaddr);
				ip_handshake_info *lhsi=NULL;
				p=peer_list.find_byid(hsi->id);
				if (!p){
					p=peer_list.find_unk_by_addr(fromaddr);
					if (p){
						peer_list.make_full(p,hsi->id,hsi->iver,hsi->addr);
					}
				}
				else
					p->verify_addr(hsi->addr);
				if (!p){
					p=peer_list.add_hsi(hsi);
				}
				lhsi=p->find_handshake();
				lhsi->tryid=hsi->tryid;
				if (p->addr.goodaddr==NULL){
					p->addr.setgoodaddr(fromaddr);
				}
				if (hsi->state&STATE_INEEDINFO)
					lhsi->setstate(STATE_SENDINGINFO);
				else
					lhsi->setstate(0);

				if (lhsi->state)
					p->send_handshake(lhsi);

				delete hsi;
			}break;
		case IP_CFG_RELAY:
			{
				ip_peer *rp;
				ip_handshake_relay hsr(buf);
#ifdef UDPDEBUG
				printf("ip_receive_cfg: %i %i ",buf[0],buf[1]);
				dumprid(buf+2);
				printf(" v%i r_id ",ntohs(*(unsigned short*)(buf+8)));
				hsr.r_id.dump();
				printf(" r_iv%i\n",hsr.r_iver);
#endif
				p=peer_list.find_byid(hsr.id);
				if (!p) {
					mprintf((0,"relay from unknown peer\n"));
					break;//hrm.
				}
				rp=peer_list.find_byid(hsr.r_id);
				if (hsr.state&STATE_RELAYREPLY){
					if (!rp) {
						mprintf((0,"relay reply for unknown peer\n"));
						break;//hrm.
					}
					ip_handshake_relay *rhsr=p->find_relay(rp->id);
					if (!rhsr)
						break;
					rhsr->setstate(0);
#ifdef UDPDEBUG
					printf("**** ");
					p->id.dump();
					printf(" is ok with ");
					rp->id.dump();
					printf("\n");
#endif
				}else{
					if (!rp)
						rp=peer_list.add_full(hsr.r_id,hsr.r_iver,hsr.r_addr);
					else
						rp->verify_addr(hsr.r_addr);

					if (rp->addr.goodaddr==NULL){
						mprintf((0,"sending relayed handshake\n"));
						//handshake with relayed peer
						ip_handshake_info *lhsi=rp->find_handshake();
						if (lhsi->addstate(STATE_INEEDINFO));
							rp->send_handshake(lhsi);
					}else{
						mprintf((0,"sending relayed reply\n"));
						//reply to relayer
						ip_handshake_relay rhsr(rp);
						rhsr.setstate(STATE_RELAYREPLY);
						p->send_handshake(&rhsr);
					}
				}
			}break;
	}
}



int ipx_ip_GetMyAddress(void) {

	u_int32_t myhandshakeid;

	d_srand( timer_get_approx_seconds() );

	ip_my_addrs.clear();

	if (!GameArg.MplIpNoGetMyAddr)
		arch_ip_get_my_addr(myport);//if we can't get an addr, then we can still probably play, just some NAT configs might not work.

	if (GameArg.MplIpMyAddr){
		ip_addr ip;
		if (!ip.dns(GameArg.MplIpMyAddr,myport)){
			ip_my_addrs.add(ip);
		}
	}

	myhandshakeid=htonl(d_rand32());
	if (ip_my_addrs.naddr){
		ip_addr ip=*ip_my_addrs.begin();
		ipx_MyAddress[4]=ip.addr[2];
		ipx_MyAddress[5]=ip.addr[3];
	}else{
		ipx_MyAddress[4]=d_rand()%255;
		ipx_MyAddress[5]=d_rand()%255;
	}
	memcpy(ipx_MyAddress+6,&myhandshakeid,4);

#ifdef UDPDEBUG
	printf(MSGHDR "Using TCP/IP id ");
	dumprid(ipx_MyAddress+4);
	putchar('\n');
#endif
	return 0;
}
/* Parse PORTSHIFT numeric parameter
 */

unsigned short ip_portshift(unsigned short baseport, const char *cs)
{
	long port;
	unsigned short ports=htons(baseport);
	if (cs){
		port=atol(cs);
		if (cs[0]=='-' || cs[0]=='+')//relative port
			ports=htons(port+baseport);
		else
			ports=htons(port);//absolute port
	}
	return ports;
}
