#ifndef __bootp_h__
#define __bootp_h__

#include "winsock.h"

// From "RFC 951 : Bootstrap Protocol"

extern unsigned int BOOTP_SubnetMask;
extern unsigned int BOOTP_Gateway;
extern unsigned int BOOTP_DomainNameServer;

class BOOTP
{
  public:

  unsigned char op;             // packet op code / message type.
                                  // 1 = BootRequest, 2 = BootReply
  unsigned char htype;          // hardware address type
                                  // '1' = 10mb ethernet
  unsigned char hlen;           // hardware address length
                                  // (eg '6' for 10mb ethernet).
  unsigned char hops;           // client sets to zero, optionally used by
                                  // gateways in cross-gateway booting.
  unsigned int xid;             // transaction ID, a random number, used to
                                  // match this boot request with the
                                  // responce it generates
  unsigned int secs;            // filled in by client, seconds elapsed since
                                  // client started trying to boot.
  unsigned int ciaddr;          // client IP address; filled in by client
                                  // in bootrequest if known.
  unsigned int yiaddr;          // 'your' (client) IP address; filled in by
                                  // server if client doesn't know it's own
                                  // address (ciaddr was 0).
  unsigned int siaddr;          // server IP address;
                                  // returned in bootreply by server
  unsigned int giaddr;          // gateway IP address, used in optional
                                  // cross-gateway booting.
  unsigned char chaddr [16];    // client hardware address,
                                  // filled in by client.
  unsigned char sname [64];     // optional server host name,
                                  // null terminated string.
  unsigned char file [128];     // boot file name, null terminated string;
                                  // 'generic' name or null in bootrequest,
                                  // fully qualified directory-path name in
                                  // bootreply.
  unsigned char vend [64];      // optional vernor-specific area, e.g. could
                                  // be hardware type/serial on request, or
                                  // 'capability' / remote file system handle
                                  // on reply.  This info me be set aside for
                                  // use by a third phase bootstrap or kernel.

  BOOTP (void);
};

int Resolve (char name [], int address);
int Resolve (char name []);

#endif
