/*
 * $Source: /cvs/cvsroot/d2x/main/ip_base.cpp,v $
 * $Revision: 1.4 $
 * $Author: bradleyb $
 * $Date: 2002-02-15 06:41:42 $
 *
 * ip_base.cpp - base for NAT-compatible udp/ip code.
 * added 2000/02/07 Matt Mueller
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.3  2002/02/14 09:24:19  bradleyb
 * d1x->d2x
 *
 * Revision 1.2  2002/02/13 10:39:21  bradleyb
 * Lotsa networking stuff from d1x
 *
 * Revision 1.1  2002/02/06 09:22:41  bradleyb
 * Adding d1x network code
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

extern "C" {
#include "timer.h"
#include "mono.h"
#include "args.h"
}
#include <string.h>
#include "ip_base.h"

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
//	memcpy(buf+pos,id,6);pos+=6;
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
//	memcpy(id,buf+pos,6);pos+=6;
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
//	memcpy(buf+pos,addr,6);pos+=6;
	return pos;
}
int ip_handshake_info::readbuf(ubyte *buf){
	int pos=ip_handshake_base::readbuf(buf);
	pos+=addr.readbuf(buf+pos);
//	memcpy(addr,buf+pos,6);pos+=6;
	return pos;
}

int ip_handshake_relay::fillbuf(ubyte *buf){
	u_int16_t tmp16;
	int pos=ip_handshake_base::fillbuf(buf);
//	memcpy(buf+pos,r_addr[0],6);pos+=6;
//	memcpy(buf+pos,r_addr[1],6);pos+=6;
//	memcpy(buf+pos,r_id,6);pos+=6;
	pos+=r_id.fillbuf(buf+pos);
	tmp16=htons(r_iver);
	memcpy(buf+pos,&tmp16,2);pos+=2;
	pos+=r_addr.fillbuf(buf+pos);
//	pos+=r_addr[1].fillbuf(buf+pos);
	return pos;
}
int ip_handshake_relay::readbuf(ubyte *buf){
	int pos=ip_handshake_base::readbuf(buf);
//	memcpy(r_addr[0],buf+pos,6);pos+=6;
//	memcpy(r_addr[1],buf+pos,6);pos+=6;
//	memcpy(r_id,buf+pos,6);pos+=6;
	pos+=r_id.readbuf(buf+pos);
	memcpy(&r_iver,buf+pos,2);pos+=2;
	r_iver=htons(r_iver);
	pos+=r_addr.readbuf(buf+pos);
//	pos+=r_addr[1].readbuf(buf+pos);
	return pos;
}
ip_handshake_relay::ip_handshake_relay(ip_peer *torelay):ip_handshake_base(1){
	type=IP_CFG_RELAY;
	r_id=torelay->id;
	r_iver=torelay->iver;
	r_addr=torelay->addr;
//	r_addr[1]=torelay->addr[1];
	setstate(STATE_INEEDINFO);
}

/*void ip_peer::set_good_addr(ubyte *fraddr){
	int i;
	for (i=0;i<numaddr;i++)
		if (addr[i]==fraddr){
			goodaddr=i;
			return;
		}
	//######hrm.
	addr[1]=fraddr;
	numaddr=2;
	goodaddr=1;
}*/
void ip_peer::send_handshake(ip_handshake_base*hsb){
	ubyte buf[256];
	int s=0;
	memcpy(buf+s,D2Xcfgid,4);s+=4;
	s+=hsb->fillbuf(buf+s);
	assert(s<256);
	if (addr.goodaddr==NULL){
		ip_addr_list::iterator i;
		for (i=addr.begin();i!=addr.end();i++)
			ip_sendtoca(*i,buf,s);
//		int i;
//		for (i=0;i<addr.naddr;i++)
//			ip_sendtoca(addr[i],buf,s);
	}else
		//ip_sendtoca(addr[goodaddr],buf,s);
		ip_sendtoca(*addr.goodaddr,buf,s);
	hsb->nextsend=timer_get_approx_seconds()+IP_HS_RETRYTIME;
	hsb->attempts++;
/*#ifdef UDPDEBUG
	{
		int hj;
		con_printf(CON_DEBUG, MSGHDR"sending handshake %i (t%i state %i(%s)) for (%i)",hsb->attempts,hsb->type,hsb->state,ip_hs_statetoa(hsb->state),addr.goodaddr);
		for (hj=0;hj<numaddr;hj++){
			if (hj>0)
				con_printf(CON_DEBUG, ", ");
			dumpraddr(addr[hj].addr);
		}
		con_printf(CON_DEBUG, " v%i\n",iver);
	}
#endif*/
}
bool ip_peer::verify_addr(ip_addr_list &fraddrs){
	int a1=addr.add(fraddrs);
	if (a1)
		addr.goodaddr=NULL;
/*	a1=(addr[0]!=fraddr0 && addr[1]!=fraddr0);
	a2=(addr[0]!=fraddr1 && addr[1]!=fraddr1);
	if (a1 || a2 || (numaddr<2 && fraddr0!=fraddr1))
	{
		mprintf((0,"verify_peer_addr_hsi %i %i\n",a1,a2));
		addr[0]=fraddr1;
		addr[1]=fraddr0;
		goodaddr=-1;
		numaddr=2;
		return 1;
	}*/
	return a1>0;
}
ip_peer * ip_peer_list::add_1(ip_addr addr/*,ip_id id*/){
	ip_peer*n=add_unk();
	n->addr.add(addr);
//	n->goodaddr=-1;
//	n->numaddr=1;
//	n->id=id;
	return n;
}
ip_peer * ip_peer_list::add_full(ip_id id, u_int16_t iver,ip_addr_list &addrs){
	ip_peer*n=add_id(id);
/*	n->addr[0]=addr0;
	n->addr[1]=addr1;
	n->goodaddr=-1;
	if (addr0==addr1){
		n->numaddr=1;
	}else{
		n->numaddr=2;
	}*/
	n->addr.add(addrs);
	n->iver=iver;
	mprintf((0,"addfull %i addrs\n",n->addr.naddr));
//	n->id=id;
	return n;
}
void ip_peer_list::make_full(ip_peer*p,ip_id id, u_int16_t iver, ip_addr_list &addrs){
	list<ip_peer*>::iterator unki=find(unknown_peers.begin(),unknown_peers.end(),p);
	if (unki!=unknown_peers.end())
		unknown_peers.erase(unki);
	p->id=id;
	p->iver=iver;
//	p->addr[0]=addr0;
//	p->addr[1]=addr1;
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
////	bool operator()(pair<const ip_id,ip_peer *> &) {
//		for(j=0;j<p->numaddr;j++)
//			if (addr==p->addr[j])
//				return 1;
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
//template <class ret,class p1,class p2, class fo>
template <class p1, class fo>
pairkiller<typename fo::result_type,p1,typename fo::argument_type,fo> pairkill(fo * no){
	return pairkiller<typename fo::result_type,p1,typename fo::argument_type,fo>(no);
}

ip_peer * ip_peer_list::find_by_addr(ip_addr addr){
	do_find_by_addr fba;
//	pairkiller<const ip_id,ip_peer*,do_find_by_addr> fbap(&fba);
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
/*#ifdef UDPDEBUG
					int hj;
					con_printf(CON_DEBUG, MSGHDR"handshake timeout (state %i(%s)) for (%i)",hsb->state,ip_hs_statetoa(hsb->state),peer->goodaddr);
					for (hj=0;hj<peer->numaddr;hj++){
						if (hj>0)
							con_printf(CON_DEBUG, ", ");
						dumpraddr(peer->addr[hj].addr);
					}
					con_printf(CON_DEBUG, " v%i\n",peer->iver);
#endif*/
					hsb->setstate(STATE_ERR);
				}else{
			//					handshake_buf.state=handshakers[i].state;
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
//			pairkiller<const ip_id,ip_peer*,do_peer_handshake> dophp(&doph);
			doph.mintime=mintime;
			for_each(peers.begin(),peers.end(),pairkill<const ip_id>(&doph));
			for_each(unknown_peers.begin(),unknown_peers.end(),doph);
//			int i;
//			mintime-=IP_HS_RETRYTIME;//try every X seconds
/*			for (i=0;i<numpeers;i++){
				ip_handshake_do_hs(&peers[i],&peers[i].query,mintime);
				ip_handshake_do_hs(&peers[i],&peers[i].reply,mintime);
			}*/
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
	con_printf(CON_DEBUG, "<%u.%u.%u.%u.%u.%u>",a[0],a[1],a[2],a[3],a[4],a[5]);
}
#endif

int ip_sendtoid(ubyte *id,const void *buf,int len){
//	ip_handshaker *hsr=find_handshaker_by_id(id);
	ip_peer *p=peer_list.find_byid(id);
	if (!p || p->addr.goodaddr==NULL){
#ifdef UDPDEBUG
		con_printf(CON_DEBUG, MSGHDR"send to invalid id(");
		dumpid(id);
		con_printf(CON_DEBUG, ") %p.",p);
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
	con_printf(CON_DEBUG, "ip_receive_cfg: %i %i ",buf[0],buf[1]);
	dumprid(buf+2);
        con_printf(CON_DEBUG, " v%i tryid%u\n",ntohs(*(unsigned short*)(buf+8)),(unsigned int)ntohl(*(u_int32_t*)(buf+10)));
#endif

			}break;
		case IP_CFG_HANDSHAKE:
#ifdef UDPDEBUG
	con_printf(CON_DEBUG, "ip_receive_cfg: %i %i ",buf[0],buf[1]);
	dumprid(buf+2);
        con_printf(CON_DEBUG, " v%i tryid%u\n",ntohs(*(unsigned short*)(buf+8)),(unsigned int)ntohl(*(u_int32_t*)(buf+10)));
#endif
			{
				ip_handshake_info *hsi=new ip_handshake_info(buf);
				hsi->addr.add(fromaddr);
				ip_handshake_info *lhsi=NULL;
				p=peer_list.find_byid(hsi->id);
/*#ifdef UDPDEBUG
				con_printf(CON_DEBUG, "hsi %i %i id=",hsi->type,hsi->state);
				hsi->id.dump();
				con_printf(CON_DEBUG, " ver=%i tryid=%u\n",hsi->iver,hsi->tryid);
				mprintf((0,"peer_list.find_byid=%p\n",p));
#endif*/
				if (!p){
					p=peer_list.find_unk_by_addr(fromaddr);
//					mprintf((0,"peer_list.find_by_addr=%p\n",p));
					if (p){
						peer_list.make_full(p,hsi->id,hsi->iver,hsi->addr);
					}
				}
				else
					p->verify_addr(hsi->addr);
				if (!p){
					p=peer_list.add_hsi(hsi);
//					mprintf((0,"peer_list.add_hsi=%p\n",p));
				}
				lhsi=p->find_handshake();
				lhsi->tryid=hsi->tryid;
				if (p->addr.goodaddr==NULL){
					p->addr.setgoodaddr(fromaddr);
//					lhsi->state&=STATE_INEEDINFO;
				}
				if (hsi->state&STATE_INEEDINFO)
					lhsi->setstate(STATE_SENDINGINFO);
					//lhsi->state&=STATE_SENDINGINFO;
				else
					lhsi->setstate(0);

//				mprintf((0,"lhsi->state=%i\n",lhsi->state));
//				mprintf((0,"hsi->state=%i\n",hsi->state));
				if (lhsi->state)
					p->send_handshake(lhsi);

				delete hsi;
			}break;
		case IP_CFG_RELAY:
			{
				ip_peer *rp;
				ip_handshake_relay hsr(buf);
#ifdef UDPDEBUG
				con_printf(CON_DEBUG, "ip_receive_cfg: %i %i ",buf[0],buf[1]);
				dumprid(buf+2);
				con_printf(CON_DEBUG, " v%i r_id ",ntohs(*(unsigned short*)(buf+8)));
				hsr.r_id.dump();
				con_printf(CON_DEBUG, " r_iv%i\n",hsr.r_iver);
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
					con_printf(CON_DEBUG, "**** ");
					p->id.dump();
					con_printf(CON_DEBUG, " is ok with ");
					rp->id.dump();
					con_printf(CON_DEBUG, "\n");
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
//						lhsi->setstate(STATE_INEEDINFO);
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

	int i;
	u_int32_t myhandshakeid;

	d_srand( timer_get_approx_seconds() );
//	con_printf(CON_DEBUG, "set my id to %u\n",myhandshakeid);

	ip_my_addrs.clear();

	if (!FindArg("-ip_nogetmyaddr"))
		arch_ip_get_my_addr(myport);//if we can't get an addr, then we can still probably play, just some NAT configs might not work.
//		if (arch_ip_get_my_addr(myport)) return -1;

	if ((i=FindArg("-ip_myaddr"))){
		ip_addr ip;
		if (!ip.dns(Args[i+1],myport)){
			ip_my_addrs.add(ip);
		}
	}

	myhandshakeid=htonl(d_rand32());
	//ipx_MyAddress[4]=qhbuf[2];
	//ipx_MyAddress[5]=qhbuf[3];
	if (ip_my_addrs.naddr){
		ip_addr ip=*ip_my_addrs.begin();
		ipx_MyAddress[4]=ip.addr[2];
		ipx_MyAddress[5]=ip.addr[3];
	}else{
		ipx_MyAddress[4]=d_rand()%255;
		ipx_MyAddress[5]=d_rand()%255;
	}
	//memcpy(ipx_MyAddress+6,&handshake_buf.id,4);
	memcpy(ipx_MyAddress+6,&myhandshakeid,4);

#ifdef UDPDEBUG
	con_printf(CON_DEBUG, MSGHDR "Using TCP/IP id ");
	dumprid(ipx_MyAddress+4);
	con_printf(CON_DEBUG, "\n");
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
//		if (port<-PORTSHIFT_TOLERANCE || port>+PORTSHIFT_TOLERANCE)
//			msg("Invalid portshift in \"%s\", tolerance is +/-%d",cs,PORTSHIFT_TOLERANCE);
//		else
			ports=htons(port+baseport);
		else
			ports=htons(port);//absolute port
	}
//	memcpy(qhbuf+4,&ports,2);
	return ports;
}

