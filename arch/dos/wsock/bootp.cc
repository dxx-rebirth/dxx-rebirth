#include <stdio.h>
#include <string.h>
#include <pc.h>
#include <time.h>
#include <iostream.h>

#include "Debug.h"
#include "winsock.h"
#include "bootp.h"

// from "RFC 1497 : BOOTP Vendor Information Extenrions"

#define BOOTP_Server 67
#define BOOTP_Client 68
#define BOOTP_MagicCookie (dotaddr (99,130,83,99))

#define BOOTP_Field_Pad 0
#define BOOTP_Field_SubnetMask 1
#define BOOTP_Field_TimeOffset 2
#define BOOTP_Field_Gateway 3
#define BOOTP_Field_TimeServer 4
#define BOOTP_Field_IEN166_NameServer 5
#define BOOTP_Field_DomainNameServer 6
#define BOOTP_Field_LogServerField 7
#define BOOTP_Field_Cookie_QuoteServerField 8
#define BOOTP_Field_LPR_Server 9
#define BOOTP_Field_ImpressServer 10
#define BOOTP_Field_RLP_Server 11
#define BOOTP_Field_Hostname 12
#define BOOTP_Field_BootFileSize 13
#define BOOTP_Field_MerritDumpFile 14
#define BOOTP_Field_DomainName 15
#define BOOTP_Field_SwapServer 16
#define BOOTP_Field_RootPath 17
#define BOOTP_Field_ExtensionsPath 18
#define BOOTP_Field_End 255

int _Debug_Bootp = 0;

unsigned int BOOTP_SubnetMask = 0;
unsigned int BOOTP_Gateway = 0;
unsigned int BOOTP_DomainNameServer = 0;

#define BOOTP_Tries 3                     // Number of times to query the BOOTP server
#define BOOTP_Timeout (CLOCKS_PER_SEC*10) // Timeout in CLOCKS_PER_SEC

BOOTP::BOOTP (void)
{
  unsigned int a;
  int z = sizeof (InetAddress);
  Socket s (AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  InetAddress Bind = (InetAddress) {AF_INET, htons (BOOTP_Client), 0};
  s.Bind (&Bind, sizeof (InetAddress));

  a = 1;
  s.SetOption (&a, SOL_SOCKET, SO_BROADCAST, 4);

  for (unsigned int t = 0; t < BOOTP_Tries; ++ t)
  {
    memset (this, 0, sizeof (BOOTP));

    op = 1;
    secs = 100;

    xid = 0x12345678 + t;

    InetAddress Connect = (InetAddress) {AF_INET, htons (BOOTP_Server), dotaddr (255,255,255,255)};
    s.Send (this, sizeof (BOOTP), 0, &Connect, sizeof (InetAddress));
//    if (_Debug || _Debug_Bootp) cerr << "BootP: Boot Request Sent" << endl;

    clock_t t0;
    t0 = clock ();

    while (
            (
              (!s.Recv (this, sizeof (BOOTP), 0, &Connect, z))
              ||
              (xid != 0x12345678 + t)
              ||
              (op != 2)
            )
            && (!kbhit ()
            && ((clock () - t0) < BOOTP_Timeout))
          );

    if ((((int *) vend) [0] == BOOTP_MagicCookie) && (xid == 0x12345678 + t) && (op == 2))
    {
      t = BOOTP_Tries;
//      if (_Debug || _Debug_Bootp)
//      {
//        cerr << "BootP: Boot Reply Received\r\n"
//             << "BootP: Hardware Address Type    - " << htype << endl
//             << "BootP: Hardware Address Length  - " << hlen << endl
//             << "BootP: Hops                     - " << hops << endl
//             << "BootP: Your IP Address          - " << (*((_InetAddress *) (&yiaddr))) << endl
//             << "BootP: Server IP Address        - " << (*((_InetAddress *) (&siaddr))) << endl
//             << "BootP: Gateway IP Address       - " << (*((_InetAddress *) (&giaddr))) << endl
//             << "BootP: Sever Host Name          - " << sname << endl
//             << "BootP: Boot File                - " << file << endl;
//      }

      a = 4;
      int done = 0;
      while ((!done) && (a < 64))
      {
        switch (vend [a])
        {
          case BOOTP_Field_Pad :
            break;
          case BOOTP_Field_SubnetMask :
            BOOTP_SubnetMask = *((int *) &vend [a + 2]);
//            if (_Debug || _Debug_Bootp)
//              cerr << "BootP: Subnet Mask        - " << (*((_InetAddress *) (&BOOTP_SubnetMask))) << endl;
            break;
          case BOOTP_Field_TimeOffset :
//            if (_Debug || _Debug_Bootp)
//              cerr << "BootP: Time Offset        - " << (*((int *) (&vend [a + 2]))) << endl;
            break;
          case BOOTP_Field_Gateway :
            BOOTP_Gateway = *((int *) &vend [a + 2]);
//            if (_Debug || _Debug_Bootp)
//              cerr << "BootP: Gateway            - " << (*((_InetAddress *) (&BOOTP_Gateway))) << endl;
            break;
          case BOOTP_Field_DomainNameServer :
            BOOTP_DomainNameServer = *((int *) &vend [a + 2]);
//            if (_Debug || _Debug_Bootp)
//              cerr << "BootP: Domain Name Server - " << (*((_InetAddress *) (&BOOTP_DomainNameServer))) << endl;
            break;
          case BOOTP_Field_DomainName :
            if (_Debug || _Debug_Bootp)
            {
              char s [255];
              int z;

              for (z = 0; z < vend [a + 2]; ++ z) s [z] = vend [a + 2 + z];
              s [z] = 0;
//              cerr << "BootP: Donain Name        - " << s << endl;
            }
            break;
          case BOOTP_Field_End :
            done = 1;
            break;
          default:
//            if (_Debug || _Debug_Bootp) cerr << "BootP: Not Implemented    - " << vend [a] << "[" << vend [a + 1] << "]" << endl;
            break;
        }
        if (vend [a] == 0) ++ a; else a += 2 + vend [a + 1];
      }
    } //else if (_Debug || _Debug_Bootp) cerr << "BootP: Boot Reply Not Received." << endl;
  }
}
