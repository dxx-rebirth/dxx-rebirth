/*
 * $Source: /cvs/cvsroot/d2x/main/ip_base.h,v $
 * $Revision: 1.4 $
 * $Author: bradleyb $
 * $Date: 2002-02-15 06:41:42 $
 *
 * ip_base.h - base for NAT-compatible udp/ip code.
 * added 2000/02/07 Matt Mueller
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.3  2002/02/14 09:24:19  bradleyb
 * d1x->d2x
 *
 * Revision 1.2  2002/02/13 10:45:49  bradleyb
 * Lotsa networking stuff from d1x
 *
 * Revision 1.1  2002/02/06 09:22:41  bradleyb
 * Adding d1x network code
 *
 *
 */

#ifndef ___IP_BASE_H
#define ___IP_BASE_H

#ifndef NDEBUG
#define UDPDEBUG
#endif

#include <map.h>
#include <list.h>
#include <algo.h>
extern "C" {
#include "ip_basec.h"
#include "fix.h"
#include "mono.h"
#include "vers_id.h"
//#include "inetc.h"
#include "timer.h"
}

class ip_addr;//prototype for arch_ip_queryhost

#include "ipclient.h"
#include <stdio.h>


//inline u_int32_t d_rand32(void) { return d_rand() + (d_rand()<<16);}
inline u_int32_t d_rand32(void) { return d_rand() + (d_rand()<<15) + (d_rand()<<30);}//d_rand() only returns 15 bits


void ip_receive_cfg(ubyte *buf,int buflen,ip_addr fromaddr);

extern unsigned char ipx_MyAddress[10];

unsigned short ip_portshift(unsigned short baseport, const char *cs);

#define UDP_BASEPORT 31017
#define UDP_SERV_BASEPORT 30817
#define PORTSHIFT_TOLERANCE 0x100
#define MAX_PACKETSIZE 8192

// Length HAS TO BE 2!
#define D2Xid "\xd2x"
// Length HAS TO BE 4!
#define D2Xcfgid "\xcfg\xd2x"


//cfg packet types
#define IP_CFG_BASE 0
#define IP_CFG_HANDSHAKE 1
#define IP_CFG_RELAY 2
//someone giving us an error (ie, outdated version, etc.) ... not handled yet.
#define IP_CFG_SORRY 3


#define STATE_INEEDINFO		1<<0
#define STATE_SENDINGINFO	1<<1
//#define STATE_NEED_RESEND	(STATE_INEEDINFO | STATE_SENDINGINFO)
#define STATE_NEED_RESEND	(STATE_INEEDINFO)
#define STATE_VALID_STATES	(STATE_INEEDINFO | STATE_SENDINGINFO)
#define STATE_RELAYREPLY	1<<6
#define STATE_ERR			1<<7
/* Dump raw form of IP address/port by fancy output to user
 */

#ifdef UDPDEBUG
#include "console.h"

inline char *ip_hs_statetoa(int state){
	if (!state)
		return "NOSTATE";
	if (state&STATE_ERR)
		return "ERR";
	if ((state&(STATE_INEEDINFO|STATE_SENDINGINFO))==(STATE_INEEDINFO|STATE_SENDINGINFO))
		return "NEED+SEND";
	if (state&STATE_INEEDINFO)
		return "NEED";
	if (state&STATE_SENDINGINFO)
		return "SEND";
	return "huh?";
}
inline void dumprid(const unsigned char *a)
{
    con_printf(CON_DEBUG, "<%u.%u.%u.%u.%u.%u>",a[0],a[1],a[2],a[3],a[4],a[5]);
}
inline void dumpraddr(const unsigned char *addr)
{
	ushort port;
	con_printf(CON_DEBUG, "[%u.%u.%u.%u]",addr[0],addr[1],addr[2],addr[3]);
	port=(unsigned short)ntohs(*(unsigned short *)(addr+4));
	con_printf(CON_DEBUG, ":%d",port);
}
#endif


class ip_id {
	public:
		ubyte id[6];

		int fillbuf(ubyte *buf)const {memcpy(buf,id,6);return 6;};
		int readbuf(const ubyte *buf){memcpy(id,buf,6);return 6;};
		bool operator < (const ip_id &a_id)const {return memcmp(id,a_id.id,6)<0;}
		bool operator == (const ip_id &a_id)const {return memcmp(id,a_id.id,6)==0;}
		bool operator > (const ip_id &a_id)const {return memcmp(id,a_id.id,6)>0;}
		ip_id& operator = (const ip_id &a_id){readbuf(a_id.id);return *this;}
#ifdef UDPDEBUG
		void dump(void){dumprid(id);}
#endif
		ip_id(ubyte*nid){readbuf(nid);}
		ip_id(void){};
};

class ip_addr {
	protected:
		ubyte alen;//alen stuff is to maybe make ipv6 support easier (if the transition ever actually happens)
	public:
		ubyte addr[6];

		int fillbuf(ubyte *buf)const {buf[0]=alen;memcpy(buf+1,addr,alen);return alen+1;};
		int readbuf(const ubyte *buf){
			int l=buf[0];
			if (l==6){
				memcpy(addr,buf+1,6);alen=l;
			}else{
				mprintf((0,"ip_addr readbuf bad len %i\n",l));
				memset(addr,0,6);alen=0;
			}
			return l+1;
		};
		bool operator < (const ip_addr &a_addr)const {
			if (alen!=a_addr.alen) return alen<a_addr.alen;
			return memcmp(addr,a_addr.addr,alen)<0;
		}
		bool operator == (const ip_addr &a_addr)const {return ((alen==a_addr.alen) && (memcmp(addr,a_addr.addr,alen)==0));}
		bool operator > (const ip_addr &a_addr)const {
			if (alen!=a_addr.alen) return alen>a_addr.alen;
			return memcmp(addr,a_addr.addr,alen)>0;
		}
		ip_addr& operator = (const ip_addr &a_addr){alen=a_addr.alen;memcpy(addr,a_addr.addr,6);return *this;}
#ifdef UDPDEBUG
		void dump(void)const{dumpraddr(addr);}
#endif
		void set(int addrsize,ubyte *naddr,ushort port){
			if (addrsize!=4)return;//not handled yet.
			alen=addrsize+2;
			memcpy(addr,naddr,addrsize);
//			port=htons(port);
			memcpy(addr+addrsize,&port,2);
		}
		int dns(char *address,ushort baseport){
			return arch_ip_queryhost(this,address,baseport);
		}
		bool ok(void)const{return alen>0;}
		int addrlen(void)const{return alen-2;}
//		ip_addr(ubyte*naddr){readbuf(naddr);}
		ip_addr(void){alen=0;};
};

class ip_addr_list {
	protected:
		list<ip_addr> addrs;
	public:
		typedef list<ip_addr>::iterator iterator;
		typedef list<ip_addr>::const_iterator const_iterator;
		int naddr;
		ip_addr *goodaddr;

		iterator begin(void){return addrs.begin();}
		iterator end(void){return addrs.end();}
		const_iterator begin(void)const{return addrs.begin();}
		const_iterator end(void)const{return addrs.end();}
		bool hasaddr(ip_addr addr){
			iterator i=find(begin(),end(),addr);
			return (i!=addrs.end());
		}
		int add(ip_addr addr){
			if (hasaddr(addr))
				return 0;
			addrs.push_back(addr);
			naddr++;
			return 1;
		}
		int add(const ip_addr_list &na){
			int j=0;
			const_iterator i=na.begin();
			for (;i!=na.end();++i)
				j+=add(*i);
			return j;
		}
		void setgoodaddr(ip_addr addr){
			iterator i=find(begin(),end(),addr);
			if (i!=end()){
				goodaddr=&(*i);
				return;
			}
			addrs.push_back(addr);
			goodaddr=&(*addrs.end());
		}
		int fillbuf(ubyte *buf)const {
			int s=0;
			const_iterator i;
			buf[s++]=naddr;
			for (i=begin();i!=end();++i)
				s+=(*i).fillbuf(buf+s);
			return s;
		}
		int readbuf(const ubyte *buf){
			int s=1,n;
			ip_addr a;
			for (n=buf[0];n;--n){
				s+=a.readbuf(buf+s);
//				addrs.push_back(a);
				if (a.ok())
					add(a);
			}
			return s;
		}
		void clear(void){
			naddr=0;
			goodaddr=NULL;
			addrs.erase(begin(),end());
		}

		ip_addr_list(const ip_addr_list &nl):addrs(nl.addrs),naddr(nl.naddr){setgoodaddr(*nl.goodaddr);}
		ip_addr_list(void):naddr(0),goodaddr(NULL){}
//		ip_addr_list(const ubyte * buf):naddr(0),goodaddr(NULL){readbuf(buf);}
};

extern ip_addr_list ip_my_addrs;

extern int ip_sendtoca(ip_addr addr,const void *buf,int len);

class ip_handshake_base {
	public:
		ubyte type;
		ubyte state;
		ip_id id;
		u_int16_t iver;
		u_int32_t tryid;

		fix nextsend;
		int attempts;
		int nopend;

		virtual int fillbuf(ubyte *buf);
		virtual int readbuf(ubyte *buf);
		void setrandid(void){tryid=d_rand32() ^ timer_get_approx_seconds();}
		void setstate(int newstate);
		int addstate(int newstate);

//		ip_handshake_base(void):type(IP_CFG_BASE){setstate(IPNOSTATE);}
		ip_handshake_base(bool initme=0):type(IP_CFG_BASE){
			nopend=0;
			state=0;
			setstate(0);
			if(initme){
		//		id=ip_my_id;
				id=ipx_MyAddress+4;
				iver=D2X_IVER;
				setrandid();
			}
		}
		virtual ~ip_handshake_base(){setstate(0);}//decrement pendinghandshakes if needed.
};
class ip_handshake_info : public ip_handshake_base {
	public:
		ip_addr_list addr;

		virtual int fillbuf(ubyte *buf);
		virtual int readbuf(ubyte *buf);

		ip_handshake_info(ubyte *buf){
			readbuf(buf);
		}
		ip_handshake_info(void):ip_handshake_base(1){
			type=IP_CFG_HANDSHAKE;
			addr=ip_my_addrs;
//			setstate(STATE_INEEDINFO);
		}
};
class ip_peer;
class ip_handshake_relay : public ip_handshake_base {
	public:
		ip_id r_id;
		u_int16_t r_iver;
		ip_addr_list r_addr;

		virtual int fillbuf(ubyte *buf);
		virtual int readbuf(ubyte *buf);

		ip_handshake_relay(ubyte *buf){
			readbuf(buf);
		}
		ip_handshake_relay(ip_peer *torelay);
};

//max number of times to try to handshake a host
#define IP_MAX_HS_ATTEMPTS 10
//how often to resend a hand-shake (3 seconds)
#define IP_HS_RETRYTIME (F1_0*3)
//how often (max) to search through the resend loop (1 second)
#define IP_HS_FRAME_RETRYTIME (F1_0)

class ip_peer {
	public:
		ip_addr_list addr;
//		int goodaddr;
		ip_id id;
		int iver;
//		ip_hs_state query;
//		ip_hs_state reply;
		list<ip_handshake_base*> handshakes;

//		void set_good_addr(ubyte *fraddr);
		void add_hs(ip_handshake_base* hs){
//			mprintf((0,"########## add_hs %p: adding a hs %p %i %i\n",this,hs,hs->type,hs->state));
			handshakes.push_back(hs);
		}
		ip_handshake_relay* find_relay(ip_id r_id){
			list<ip_handshake_base*>::iterator i;
			ip_handshake_base* hsb;
			ip_handshake_relay* hsr;
			for (i=handshakes.begin();i!=handshakes.end();++i){
				hsb=(*i);
				if (hsb->type==IP_CFG_RELAY){
					hsr=(ip_handshake_relay*)hsb;
					if (hsr->r_id==r_id)
						return hsr;
				}
			}
			return NULL;
		}
		ip_handshake_info* find_handshake(void){
			list<ip_handshake_base*>::iterator i;
			ip_handshake_base* hsb;
			for (i=handshakes.begin();i!=handshakes.end();++i){
				hsb=(*i);
				if (hsb->type==IP_CFG_HANDSHAKE)
					return (ip_handshake_info*) hsb;
			}
			ip_handshake_info *ni=new ip_handshake_info();
//			handshakes.push_back(ni);
			add_hs(ni);
			return ni;
		}
/*		ip_handshake_base* find_hs(u_int32_t tryid, ubyte type){
			list<ip_handshake_base*>::iterator i;
			ip_handshake_base* hsb;
			for (i=handshakes.begin();i!=handshakes.end();++i){
				hsb=(*i);
				if (hsb->tryid==tryid && hsb->type==type)
					return hsb;
			}
			return NULL;
		}*/
		void send_handshake(ip_handshake_base*hsb);
		bool verify_addr(ip_addr_list &fraddrs);
		ip_peer(void){iver=0;}
		~ip_peer(){
			list<ip_handshake_base*>::iterator i;
			for (i=handshakes.begin();i!=handshakes.end();++i){
/*				mprintf((0,"###### ~ip_peer %p: deleting a hs %p %i %i\n",this,(*i),(*i)->type,(*i)->state));
				if (!(*i))
					mprintf((0,"~ip_peer null hs?\n"));
				else*/
					delete (*i);
			}
		}
};


class ip_peer_list {
	public:
		typedef map<ip_id,ip_peer *,less<ip_id> > peer_map;
		peer_map peers;
		list<ip_peer*> unknown_peers;
		int pendinghandshakes;
		fix pendinghandshake_lasttime;

		ip_peer * add_id(ip_id id){
			ip_peer*n=new ip_peer();
			n->id=id;
			peers.insert(peer_map::value_type(n->id,n));
			return n;
		}
		ip_peer * add_unk(void){
			ip_peer*n=new ip_peer();
			unknown_peers.push_back(n);
			return n;
		}
		//ip_peer * add_1(ubyte *addr,ip_id id);
		ip_peer * add_1(ip_addr addr);
		ip_peer * add_full(ip_id id, u_int16_t iver, ip_addr_list &addrs);
		void make_full(ip_peer*p,ip_id id, u_int16_t iver, ip_addr_list &addrs);
		ip_peer * add_hsi(ip_handshake_info *hsi){return add_full(hsi->id,hsi->iver,hsi->addr);}
		ip_peer * find_byid(ip_id id);
		ip_peer * find_by_addr(ip_addr addr);
		ip_peer * find_unk_by_addr(ip_addr addr);
		void handshake_frame(void);
		void clear(void);
		ip_peer_list(){
			pendinghandshakes=0;
			pendinghandshake_lasttime=0;
		}
		~ip_peer_list();
};
extern ip_peer_list peer_list;

#endif
