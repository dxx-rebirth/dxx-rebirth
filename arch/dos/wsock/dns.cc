#include <stdio.h>
#include <string.h>
#include <pc.h>
#include <iostream.h>

#include "winsock.h"
#include "bootp.h"
#include "debug.h"

int _Debug_DNS;

// From "RFC 1035 : Donaim Names - Implementation and Specification"

#define Type_A 1        // Host Address
#define Type_NS 2       // An Authoritative Name Server
#define Type_MD 3       // A Mail Destination (Obsolete - use MX)
#define Type_MF 4       // A Mail Forwarder (Obsolete - use MX)
#define Type_CNAME 5    // The Canonical Name For An Alias
#define Type_SOA 6      // Marks The Start Of A Zone Of Authority
#define Type_MB 7       // A Mailbox Domain Name (Experimental)
#define Type_MG 8       // A Mail Group Member (Experimental)
#define Type_MR 9       // A Mail Rename Domain Name (Experimental)
#define Type_NULL 10    // A Null RR (Experimental)
#define Type_WKS 11     // A Well Known Service Description
#define Type_PTR 12     // A Domain Name Pointer
#define Type_HINFO 13   // Host Information
#define Type_MX 15      // Mail Exchange
#define Type_TXT 16     // Text Strings

struct
{
  int ID;
  char name [255];
  char description [255];
} DNS_Types [] = {
{1,   "A",        "a host address"},
{2,   "NS",       "an authoritative name server"},
{3,   "MD",       "a mail destination"},
{4,   "MF",       "a mail forwarder"},
{5,   "CNAME",    "the canonical name for an alias"},
{6,   "SOA",      "marks the start of a zone of authority"},
{7,   "MB",       "a mailbox domain name"},
{8,   "MG",       "a mail group member"},
{9,   "MR",       "a mail rename domain name"},
{10,  "NULL",     "a null RR"},
{11,  "WKS",      "a well known service description"},
{12,  "PTR",      "a domain name pointer"},
{13,  "HINFO",    "host information"},
{14,  "MINFO",    "mailbox or mail list information"},
{15,  "MX",       "mail exchange"},
{16,  "TXT",      "text strings"},
{252, "AXFR",     "a request for a transver of an entire zone"},
{253, "MAILB",    "a request for a mail-related records"},
{254, "MAILA",    "a request for mail agent RRs"},
{255, "*",        "a request for all records"},
{-1,  "UNK",      "unknown"}};



#define QType_A 1       // Host Address
#define QType_NS 2      // An Authoritative Name Server
#define QType_MD 3      // A Mail Destination (Obsolete - use MX)
#define QType_MF 4      // A Mail Forwarder (Obsolete - use MX)
#define QType_CNAME 5   // The Canonical Name For An Alias
#define QType_SOA 6     // Marks The Start Of A Zone Of Authority
#define QType_MB 7      // A Mailbox Domain Name (Experimental)
#define QType_MG 8      // A Mail Group Member (Experimental)
#define QType_MR 9      // A Mail Rename Domain Name (Experimental)
#define QType_NULL 10   // A Null RR (Experimental)
#define QType_WKS 11    // A Well Known Service Description
#define QType_PTR 12    // A Domain Name Pointer
#define QType_HINFO 13  // Host Information
#define QType_MX 15     // Mail Exchange
#define QType_TXT 16    // Text Strings
#define QType_AXFR 252  // A Request For A Transfer Of An Entire Zone
#define QType_MAILB 253 // A Request For Mailbox-Related Records (MB, MG, or MR)
#define QType_MAILA 254 // A Request For Mail Agent RRs (Obsolete - See MX)
#define QType_255       // A Request For All Records

#define Class_IN 1      // The Internet
#define Class_CS 2      // The CSNET Class (Obsolete)
#define Class_CH 3      // The CHAOS Class
#define Class_HS 4      // Hesiod [Dyer 87]

#define QClass_IN 1     // The Internet
#define QClass_CS 2     // The CSNET Class (Obsolete)
#define QClass_CH 3     // The CHAOS Class
#define QClass_HS 4     // Hesiod [Dyer 87]
#define QClass_ 255     // All Classes

struct
{
  int ID;
  char name [255];
  char description [255];
} DNS_Classes [] = {
{1,   "IN",       "the internet class"},
{2,   "CS",       "the CSNET class"},
{3,   "CH",       "the CHAOS class"},
{4,   "HS",       "the Hesiod class"},
{255, "*",        "any class"},
{-1,  "UNK",      "unknown"}};


class DNS_Packet
{
  public:

  struct
  {
    unsigned short id;         // Unique Number for ID

    unsigned int RD : 1;       // Recursion Desired
    unsigned int TC : 1;       // Truncation
    unsigned int AA : 1;       // Authoritative Answer
    unsigned int Opcode : 4;   // Opcode
                                 // 0 = Standard Query
                                 // 1 = Inverse Query
                                 // 2 = Sever Status Request
    unsigned int QR : 1;       // 0 = Query, 1 = Responce

    unsigned int RCode : 4;    // Responce Code
                                 // 0 = No Error Condition
                                 // 1 = Format Error
                                 // 2 = Sever Failure
                                 // 3 = Name Error
                                 // 4 = Not Implemented
                                 // 5 = Refused
    unsigned int Z : 3;        // Reserved
    unsigned int RA : 1;       // Recursion Availiable

    unsigned short QdCount;    // Number of Questions
    unsigned short AnCount;    // Number of Answers
    unsigned short NsCount;    // Number of Server RR's in authority section
    unsigned short ArCount;    // Number of Server RR's in additional section
  } hddr;

  char x [500];

  int i, j;

  DNS_Packet (void);
  void Clear (void);

  void PutChar (unsigned char c);
  void PutShort (unsigned short a);
  void PutInt (unsigned int a);
  void PutString (char s []);
  void PutString_ (char s [], int len);
  void PutIP (char s []);

  unsigned char GetChar (void);
  unsigned short GetShort (void);
  unsigned int GetInt (void);
  unsigned int GetString (char s []);
  void GetString_ (char s [], int len);
  int GetIP (char s []);
};

DNS_Packet::DNS_Packet (void)
{
  Clear ();
}

void DNS_Packet::Clear (void)
{
  memset (this, 0, sizeof (DNS_Packet));
  j = -1;
}


void DNS_Packet::PutChar (unsigned char c)
{
  x [i ++] = c;
}

void DNS_Packet::PutShort (unsigned short a)
{
  PutChar (a >> 8);
  PutChar (a >> 0);
}

void DNS_Packet::PutInt (unsigned int a)
{
  PutChar (a >> 24);
  PutChar (a >> 16);
  PutChar (a >> 8);
  PutChar (a >> 0);
}

void DNS_Packet::PutString (char s [])
{
  int a;

  for (a = 0; s [a]; ++ a);

  PutString_ (s, a);
}

void DNS_Packet::PutString_ (char s [], int len)
{
  PutChar (len);
  for (int a = 0; a < len; ++ a) PutChar (s [a]);
}

void DNS_Packet::PutIP (char s [])
{
  int a = 0, b;

  while (s [a])
  {
    for (b = a; (s [b] != '.') && s [b]; ++ b);

    PutString_ (&s [a], b - a);

    if (s [b]) a += b + 1;
      else a = b;
  }

  PutChar (0);
}


unsigned char DNS_Packet::GetChar (void)
{
  if (j == (-1)) return x [i ++];
    else return (x [j ++]);
}

unsigned short DNS_Packet::GetShort (void)
{
  return (GetChar () << 8) +
         (GetChar () << 0);
}

unsigned int DNS_Packet::GetInt (void)
{
  return (GetChar () << 24) +
         (GetChar () << 16) +
         (GetChar () << 8) +
         (GetChar () << 0);
}

unsigned int DNS_Packet::GetString (char s [])
{
  int len = GetChar ();

  if ((len & 0xc0) == 0xc0)
  {
    j = ((len & 0x3f) << 8) + GetChar () - sizeof (hddr);
    return GetString (s);
  }

  GetString_ (s, len);

  return len;
}

void DNS_Packet::GetString_ (char s [], int len)
{
  int a;

  for (a = 0; a < len; ++ a) s [a] = GetChar ();

  s [a] = 0;
}

int DNS_Packet::GetIP (char s [])
{
  int a = 0;

  for (int b = GetString (&s [a]); b; b = GetString (&s [a]))
  {
    a += b;
    s [a ++] = '.';
  }

  j = -1;
  s [a - 1] = 0;

  return a - 1;
}


int Resolve (char name [])
{
  return Resolve (name, BOOTP_DomainNameServer);
}

int GetDNS (DNS_Packet &p, int count)
{
  char s [256];
  int a, b, c, type, size, Class;
  InetAddress Temp = {0, 0, 0};
  int AddrReturn = 0;

  for (a = 0; a < count; ++ a)
  {
    p.GetIP (s);
//    if (_Debug || _Debug_DNS) cerr << "DNS: NAME     : " << s << endl;
    type = p.GetShort ();
    for (c = 0; (DNS_Types [c].ID != -1) && (DNS_Types [c].ID != type); ++ c);
//    if (_Debug || _Debug_DNS)
//      cerr << "DNS: TYPE     : " << type << " "
//                                 << DNS_Types [c].name << " "
//                                 << DNS_Types [c].description << endl;
    Class = p.GetShort ();
    for (c = 0; (DNS_Classes [c].ID != -1) && (DNS_Classes [c].ID != Class); ++ c);
//    if (_Debug || _Debug_DNS)
//      cerr << "DNS: CLASS    : " << Class << " "
//                                 << DNS_Classes [c].name << " "
//                                 << DNS_Classes [c].description << endl;
    b = p.GetInt ();
//    if (_Debug || _Debug_DNS) cerr << "DNS: TTL      : " << b << endl;
    size = p.GetShort ();
//    if (_Debug || _Debug_DNS) cerr << "DNS: RDLENGTH : " << size << endl;

    switch (type)
    {
      case Type_A :
        while (size)
        {
          b = p.GetInt ();
          b = htonl (b);
          if (!AddrReturn) AddrReturn = b;
          Temp.address = htonl (b);
//          if (_Debug || _Debug_DNS)
//            cout << "DNS:   " << (*((_InetAddress *) (&b))) << endl;
          size -= 4;
        }
        break;
      case Type_CNAME :
        p.GetIP (s);
        if (_Debug || _Debug_DNS) fprintf (stderr, "DNS:   %s\r\n", s);
        break;
      case Type_NS :
        p.GetIP (s);
        if (_Debug || _Debug_DNS) fprintf (stderr, "DNS:   %s\r\n", s);
        break;
      default:
        while (size)
        {
          p.GetChar ();
          -- size;
        }
    }
  }

  return AddrReturn;
}

int Resolve (char name [], int address)
{
  InetAddress DNS_ = {AF_INET, htons (53), address};
  Socket s (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  DNS_Packet p;
  int a;

  if (_Debug || _Debug_DNS) fprintf (stderr, "DNS: Using server        : ");
  if (_Debug || _Debug_DNS) fPrintIP (stderr, DNS_);
  fprintf (stderr, "\r\n");

  p.hddr.id = 0x1234;
  p.hddr.QdCount = htons (1);

  p.PutIP (name);
  p.PutShort (QType_A);
  p.PutShort (QClass_IN);

  s.Send (&p, sizeof (p.hddr) + p.i, 0, &DNS_, sizeof (InetAddress));

  a = 0;
  int b = sizeof (InetAddress);
//  while ((!a))
  a = s.Recv (&p, 512, 0, &DNS_, b);

  a -= sizeof (p.hddr);

  if (_Debug || _Debug_DNS) fprintf (stderr, "DNS: Number of Questions : %u\r\n", ntohs (p.hddr.QdCount));
  if (_Debug || _Debug_DNS) fprintf (stderr, "DNS: Number of Answers   : %u\r\n", ntohs (p.hddr.AnCount));
  if (_Debug || _Debug_DNS) fprintf (stderr, "DNS: Number of NS        : %u\r\n", ntohs (p.hddr.NsCount));
  if (_Debug || _Debug_DNS) fprintf (stderr, "DNS: Number of AR        : %u\r\n", ntohs (p.hddr.ArCount));

  if ((a = GetDNS (p, ntohs (p.hddr.AnCount)))) return a;
  else
  {
     if (_Debug || _Debug_DNS)
      fprintf (stderr, "DNS: Processing Name Servers.\r\n");
    GetDNS (p, ntohs (p.hddr.NsCount));
     if (_Debug || _Debug_DNS)
      fprintf (stderr, "DNS: Processing Answers.\r\n");
    a = GetDNS (p, ntohs (p.hddr.ArCount));
     if (a == 0)
      return 0;
    else return Resolve (name, a);
  }
}
