#include <sys/farptr.h>
#include <sys/segments.h>
#include <stdio.h>
#include <dpmi.h>
#include <pc.h>
#include <string.h>
#include <fcntl.h>

#include "Debug.h"
#include "winsock.h"
#include "vxdldr.h"
#include "vxd.h"
#include "bootp.h"
#include "farptrx.h"

struct
{
  int ID;
  char Name [256];
} Wsock_Calls [] = {
{0x100, "ACCEPT"},
{0X101, "BIND"},
{0X102, "CLOSESOCKET"},
{0x103, "CONNECT"},
{0x104, "GETPEERNAME"},
{0x105, "GETSOCKNAME"},
{0x106, "GETSOCKOPT"},
{0x107, "IOCTLSOCKET"},
{0x108, "LISTEN"},
{0x109, "RECV"},
{0x10A, "SELECT_SETUP"},
{0x10B, "SELECT_CLEANUP"},
{0x10C, "ASYNC_SELECT"},
{0x10D, "SEND"},
{0x10E, "SETSOCKOPT"},
{0x10F, "SHUTDOWN"},
{0x110, "SOCKET"},
{0x111, "CREATE"},
{0x112, "CREATE_MULTIPLE"},
{0x113, "DESTROY"},
{0x114, "DESTROY_BY_SOCKET"},
{0x115, "DESTROY_BY_THREAD"},
{0x116, "SIGNAL"},
{0x117, "SIGNAL_ALL"},
{0x118, "CONTROL"},
{0x119, "REGISTER_POSTMSG"},
{0x11A, "ARECV"},
{0x11B, "ASEND"},
{-1,    "UNKNOWN"}};

int _Debug_Wsock = 0;

int WSockDataSel = 0;

int WSockEntry [2];  // wsock.vxd 16 bit PM entry

__dpmi_meminfo _SocketP;
int SocketP;         // 64k selector for parameters and stack
__dpmi_meminfo _SocketD;
int SocketD;         // 64k selector for Data
__dpmi_meminfo _SocketC;
int SocketC;         // code for callbacks

void WSock::CallVxD (int func)
{
  asm volatile ("pushl   %%es                    \n\
                 pushl   %%ecx                   \n\
                 popl    %%es                    \n\
                 lcall   _WSockEntry             \n\
                 andl    %%eax, 0x0000ffff       \n\
                 popl    %%es"
                 : "=a" (_Error)
                 : "a" (func), "b" (0), "c" (SocketP));

  if (((_Debug || _Debug_Wsock) || (_ErrorDisplay && (_Error != 0))) && (_Error != WSOCK_WILL_BLOCK) && (_Error != WSAEWOULDBLOCK))
  {
    int a;
    for (a = 0; (Wsock_Calls [a].ID != func) && (Wsock_Calls [a].ID != -1); ++ a);

    int b;
    for (b = 0; (WSA_ERRORS [b].ID != _Error) && (WSA_ERRORS [b].ID != -1); ++ b);

    fprintf (stderr, "Wsock: Function %s(%x) returned %s - %s(%x)\r\n", Wsock_Calls [a].Name, func, WSA_ERRORS [b].Name, WSA_ERRORS [b].Desc, _Error);
  }
}

void PrintIP (InetAddress &a);

Socket::Socket (Socket&s, void *Address, int &AddressLength)
{
  _farpokel (SocketP, 0 * 4, (SocketP << 16) + (7 * 4));      // Address
  _farpokel (SocketP, 1 * 4, s._Socket);                      // Listening Socket
  _farpokel (SocketP, 2 * 4, 0);                              // Connected Socket
  _farpokel (SocketP, 3 * 4, AddressLength);                  // Address Length
  _farpokel (SocketP, 4 * 4, 0);                              // Connected Socket Handle
  _farpokel (SocketP, 5 * 4, 0);                              // Apc Routing
  _farpokel (SocketP, 6 * 4, 0);                              // Apc Context

  _farpokex (SocketP, 7 * 4, Address, AddressLength);

  CallVxD (0x0100);

  AddressLength = _farpeekl (SocketP, 3 * 4);
  _farpeekx (SocketP, 7 * 4, Address, AddressLength);

  _AddressLength = AddressLength;
  memcpy (_Address, Address, AddressLength);

  _Socket = _farpeekl (SocketP, 2 * 4);
  _SocketHandle = _farpeekl (SocketP, 4 * 4);

  _Error = 0;
  _ErrorDisplay = 0;
}

void Socket::Bind (void *Address, int AddressLength)
{
  _farpokel (SocketP, 0 * 4, (SocketP << 16) + (5 * 4));      // Address
  _farpokel (SocketP, 1 * 4, _Socket);                        // Socket
  _farpokel (SocketP, 2 * 4, AddressLength);                  // Address Length
  _farpokel (SocketP, 3 * 4, 0);                              // Apc Routine
  _farpokel (SocketP, 4 * 4, 0);                              // Apc Context

  _farpokex (SocketP, 5 * 4, Address, AddressLength);

  CallVxD (0x0101);

  _farpeekx (SocketP, 5 * 4, Address, AddressLength);
}

Socket::~Socket (void)
{
  if (_Socket)
  {
    _farpokel (SocketP, 0 * 4, _Socket);

    CallVxD (0x0102);
  }
}

void Socket::Connect (void *Address, int AddressLength)
{
  _farpokel (SocketP, 0 * 4, (SocketP << 16) + (5 * 4));      // Address
  _farpokel (SocketP, 1 * 4, _Socket);                        // Socket
  _farpokel (SocketP, 2 * 4, AddressLength);                  // Address Length
  _farpokel (SocketP, 3 * 4, 0);                              // Apc Routine
  _farpokel (SocketP, 4 * 4, 0);                              // Apc Context

  _farpokex (SocketP, 5 * 4, Address, AddressLength);

  CallVxD (0x0103);

  _farpeekx (SocketP, 5 * 4, Address, AddressLength);

  _AddressLength = AddressLength;
  memcpy (_Address, Address, AddressLength);
}

void Socket::GetPeerName (void *Address, int &AddressLength)
{
  _farpokel (SocketP, 0 * 4, (SocketP << 16) + (3 * 4));      // Address
  _farpokel (SocketP, 1 * 4, _Socket);                        // Socket
  _farpokel (SocketP, 2 * 4, AddressLength);                  // Address Length

  _farpokex (SocketP, 3 * 4, Address, AddressLength);

  CallVxD (0x0104);

  AddressLength = _farpeekl (SocketP, 2 * 4);
  _farpeekx (SocketP, 3 * 4, Address, AddressLength);
}

void Socket::GetSockName (void *Address, int &AddressLength)
{
  _farpokel (SocketP, 0 * 4, (SocketP << 16) + (3 * 4));      // Address
  _farpokel (SocketP, 1 * 4, _Socket);                        // Socket
  _farpokel (SocketP, 2 * 4, AddressLength);                  // Address Length

  _farpokex (SocketP, 3 * 4, Address, AddressLength);

  CallVxD (0x0105);

  AddressLength = _farpeekl (SocketP, 2 * 4);
  _farpeekx (SocketP, 3 * 4, Address, AddressLength);
}

void Socket::GetOption (void *Value, int OptionLevel, int OptionName, int &ValueLength)
{
  _farpokel (SocketP, 0 * 4, (SocketP << 16) + (6 * 4));      // Value
  _farpokel (SocketP, 1 * 4, _Socket);                        // Socket
  _farpokel (SocketP, 2 * 4, OptionLevel);                    // Option Level
  _farpokel (SocketP, 3 * 4, OptionName);                     // Option Name
  _farpokel (SocketP, 4 * 4, ValueLength);                    // Value Length
  _farpokel (SocketP, 5 * 4, *((int *) Value));               // Int Value

  ValueLength = _farpeekl (SocketP, 4 * 4);
  _farpokex (SocketP, 6 * 4, Value, ValueLength);

  CallVxD (0x0106);

  ValueLength = _farpeekl (SocketP, 4 * 4);
  if (ValueLength == 4) *((int *) Value) = _farpeekl (SocketP, 5 * 4);
    else _farpeekx (SocketP, 6 * 4, Value, ValueLength);
}

void Socket::Ioctl (int Command, int &Param)    // Not Tested Yet
{
  _farpokel (SocketP, 0 * 4, _Socket);                        // Socket
  _farpokel (SocketP, 1 * 4, Command);                        // Command
  _farpokel (SocketP, 2 * 4, Param);                          // Parameter

  CallVxD (0x0107);

  Param = _farpeekl (SocketP, 2 * 4);
}

void Socket::Listen (int Backlog)
{
  _farpokel (SocketP, 0 * 4, _Socket);
  _farpokel (SocketP, 1 * 4, Backlog);

  CallVxD (0x0108);
}

int Socket::Recv (void *Buffer, int BufferLength, int Flags, void *Address, int &AddressLength)
{
  _farpokel (SocketP, 0 * 4, SocketD << 16);                  // Buffer
  _farpokel (SocketP, 1 * 4, (SocketP << 16) + (10 * 4));     // Address
  _farpokel (SocketP, 2 * 4, _Socket);                        // Socket
  _farpokel (SocketP, 3 * 4, BufferLength);                   // Buffer Length
  _farpokel (SocketP, 4 * 4, Flags);                          // Flags
  _farpokel (SocketP, 5 * 4, AddressLength);                  // Address Length
  _farpokel (SocketP, 6 * 4, 0);                              // Bytes Received
  _farpokel (SocketP, 7 * 4, 0);                              // Apc Routine
  _farpokel (SocketP, 8 * 4, 0);                              // Apc Context
  _farpokel (SocketP, 9 * 4, 0);                              // Timeout

  _farpokex (SocketP, 10 * 4, Address, AddressLength);

  CallVxD (0x0109);

  _farpeekx (SocketP, 10 * 4, Address, AddressLength);
  _farpeekx (SocketD, 0, Buffer, _farpeekl (SocketP, 6 * 4));

  return _farpeekl (SocketP, 6 * 4);
}

int Socket::Recv (void *Buffer, int BufferLength, int Flags)
{
  _farpokel (SocketP, 0 * 4, SocketD << 16);                  // Buffer
  _farpokel (SocketP, 1 * 4, (SocketP << 16) + (10 * 4));     // Address
  _farpokel (SocketP, 2 * 4, _Socket);                        // Socket
  _farpokel (SocketP, 3 * 4, BufferLength);                   // Buffer Length
  _farpokel (SocketP, 4 * 4, Flags);                          // Flags
  _farpokel (SocketP, 5 * 4, _AddressLength);                 // Address Length
  _farpokel (SocketP, 6 * 4, 0);                              // Bytes Received
  _farpokel (SocketP, 7 * 4, 0);                              // Apc Routine
  _farpokel (SocketP, 8 * 4, 0);                              // Apc Context
  _farpokel (SocketP, 9 * 4, 0);                              // Timeout

  _farpokex (SocketP, 10 * 4, _Address, _AddressLength);

  CallVxD (0x0109);

  _farpeekx (SocketD, 0, Buffer, _farpeekl (SocketP, 6 * 4));

  return _farpeekl (SocketP, 6 * 4);
}

int Socket::Select (int FD)
{
  _farpokel (SocketP, 0 * 4, (SocketP << 16) + (6 * 4));      // ReadList
  _farpokel (SocketP, 1 * 4, 0);                              // WriteList
  _farpokel (SocketP, 2 * 4, 0);                              // ExceptList
  _farpokel (SocketP, 3 * 4, 1);                              // ReadCount
  _farpokel (SocketP, 4 * 4, 0);                              // WriteCount
  _farpokel (SocketP, 5 * 4, 0);                              // ExceptCount

  _farpokel (SocketP, 6 * 4, _Socket);
  _farpokel (SocketP, 7 * 4, FD);
  _farpokel (SocketP, 8 * 4, 0);

  CallVxD (0x010b);

  return _farpeekl (SocketP, 6 * 4);
}

int Socket::Send (void *Buffer, int BufferLength, int Flags, void *Address, int AddressLength)
{
  _farpokel (SocketP, 0 * 4, SocketD << 16);                  // Buffer
  _farpokel (SocketP, 1 * 4, (SocketP << 16) + (10 * 4));     // Address
  _farpokel (SocketP, 2 * 4, _Socket);                        // Socket
  _farpokel (SocketP, 3 * 4, BufferLength);                   // Buffer Length
  _farpokel (SocketP, 4 * 4, Flags);                          // Flags
  _farpokel (SocketP, 5 * 4, AddressLength);                  // Address Length
  _farpokel (SocketP, 6 * 4, 0);                              // Bytes Sent
  _farpokel (SocketP, 7 * 4, 0);                              // Apc Routine
  _farpokel (SocketP, 8 * 4, 0);                              // Apc Context
  _farpokel (SocketP, 9 * 4, 0);                              // Timeout

  _farpokex (SocketP, 10 * 4, Address, AddressLength);
  _farpokex (SocketD, 0, Buffer, BufferLength);

  CallVxD (0x010d);

  return _farpeekl (SocketP, 6 * 4);
}

int Socket::Send (void *Buffer, int BufferLength, int Flags)
{
  _farpokel (SocketP, 0 * 4, SocketD << 16);                  // Buffer
  _farpokel (SocketP, 1 * 4, (SocketP << 16) + (10 * 4));     // Address
  _farpokel (SocketP, 2 * 4, _Socket);                        // Socket
  _farpokel (SocketP, 3 * 4, BufferLength);                   // Buffer Length
  _farpokel (SocketP, 4 * 4, Flags);                          // Flags
  _farpokel (SocketP, 5 * 4, _AddressLength);                 // Address Length
  _farpokel (SocketP, 6 * 4, 0);                              // Bytes Sent
  _farpokel (SocketP, 7 * 4, 0);                              // Apc Routine
  _farpokel (SocketP, 8 * 4, 0);                              // Apc Context
  _farpokel (SocketP, 9 * 4, 0);                              // Timeout

  _farpokex (SocketP, 10 * 4, _Address, _AddressLength);
  _farpokex (SocketD, 0, Buffer, BufferLength);

  CallVxD (0x010d);

  return _farpeekl (SocketP, 6 * 4);
}

void Socket::SetOption (void *Value, int OptionLevel, int OptionName, int ValueLength)
{
  _farpokel (SocketP, 0 * 4, (SocketP << 16) + (6 * 4));      // Value
  _farpokel (SocketP, 1 * 4, _Socket);                        // Socket
  _farpokel (SocketP, 2 * 4, OptionLevel);                    // Option Level
  _farpokel (SocketP, 3 * 4, OptionName);                     // Option Name
  _farpokel (SocketP, 4 * 4, ValueLength);                    // Value Length
  _farpokel (SocketP, 5 * 4, *((int *) Value));               // Int Value

  _farpokex (SocketP, 6 * 4, Value, ValueLength);

  CallVxD (0x010e);
}

void Socket::ShutDown (int How)
{
  _farpokel (SocketP, 0 * 4, _Socket);                        // Socket
  _farpokel (SocketP, 1 * 4, How);                            // How

  CallVxD (0x010f);
}

Socket::Socket (int AddressFamily, int SocketType, int Protocol)
{
  _ErrorDisplay = 0;

  _farpokel (SocketP, 0 * 4, AddressFamily);                  // Address Family
  _farpokel (SocketP, 1 * 4, SocketType);                     // Socket Type
  _farpokel (SocketP, 2 * 4, Protocol);                       // Protocol
  _farpokel (SocketP, 3 * 4, 0);                              // New Socket
  _farpokel (SocketP, 4 * 4, 0);                              // New Socket Handle

  CallVxD (0x0110);

  _Socket = _farpeekl (SocketP, 3 * 4);
  _SocketHandle = _farpeekl (SocketP, 4 * 4);

  _Error = 0;
  _ErrorDisplay = 0;
}

void Socket::Create (int Event, int ApcRoutine, int ApcContext, int Notify)
{
  _farpokel (SocketP, 0 * 4, _Socket);                        // Socket
  _farpokel (SocketP, 1 * 4, Event);                          // Event
  _farpokel (SocketP, 2 * 4, ApcRoutine);                     // Apc Routine
  _farpokel (SocketP, 3 * 4, ApcContext);                     // Apc Contect
  _farpokel (SocketP, 4 * 4, Notify);                         // Notify

  CallVxD (0x0111);
}

void Socket::_Create (int Event, CB &ApcRoutine)
{
  Create (Event, SocketC << 16, (int) &ApcRoutine, 0);
}

void Socket::Destroy (void)
{
  _farpokel (SocketP, 0 * 4, _Socket);

  CallVxD (0x0114);
}


void PrintIP (InetAddress &a)
{
  printf ("%u.%u.%u.%u:%u\r\n", a.address & 0xff,
                               (a.address >> 8) & 0xff,
                               (a.address >> 16) & 0xff,
                               (a.address >> 24) & 0xff,
                                ((a.port >> 8) & 0x00ff) + ((a.port << 8) & 0xff00));
}

void PrintIP (unsigned int a)
{
  printf ("%u.%u.%u.%u\r\n", a & 0xff,
                               (a >> 8) & 0xff,
                               (a >> 16) & 0xff,
                               (a >> 24) & 0xff);
}


void fPrintIP (FILE *file, InetAddress &a)
{
  fprintf (file, "%u.%u.%u.%u:%u", a.address & 0xff,
                                  (a.address >> 8) & 0xff,
                                  (a.address >> 16) & 0xff,
                                  (a.address >> 24) & 0xff,
                                 ((a.port >> 8) & 0x00ff) + ((a.port << 8) & 0xff00));
}

void fPrintIP (FILE *file, unsigned int a)
{
  fprintf (file, "%u.%u.%u.%u", a & 0xff,
                               (a >> 8) & 0xff,
                               (a >> 16) & 0xff,
                               (a >> 24) & 0xff);
}

extern int CallBack;

void SocketInit (void)
{
  VxdLdrLoadDevice ("WSOCK.VXD");
  VxdLdrLoadDevice ("WSOCK.386");

  VxdGetEntry (WSockEntry, 0x003e);

  _SocketP.handle = 0;
  _SocketP.size = 65536;
  _SocketP.address = 0;
  __dpmi_allocate_memory (&_SocketP);

  _SocketD.handle = 0;
  _SocketD.size = 65536;
  _SocketD.address = 0;
  __dpmi_allocate_memory (&_SocketD);

  _SocketC.handle = 0;
  _SocketC.size = 65536;
  _SocketC.address = 0;
  __dpmi_allocate_memory (&_SocketC);

  SocketP = __dpmi_allocate_ldt_descriptors (1);
  SocketD = __dpmi_allocate_ldt_descriptors (1);
  SocketC = __dpmi_allocate_ldt_descriptors (1);

  __dpmi_set_segment_base_address (SocketP, _SocketP.address);
  __dpmi_set_segment_base_address (SocketD, _SocketD.address);
  __dpmi_set_segment_base_address (SocketC, _SocketC.address);

  __dpmi_set_segment_limit (SocketP, _SocketP.size);
  __dpmi_set_segment_limit (SocketD, _SocketD.size);
  __dpmi_set_segment_limit (SocketC, _SocketC.size);

  _farpokel (SocketC, 0, 0xea676690);           // nop / jmp long:long
  _farpokel (SocketC, 4, (int) (&CallBack));    // Offset
  _farpokel (SocketC, 8, _my_cs ());            // Segment

  __dpmi_set_descriptor_access_rights (SocketC, 0x00fb);

  WSockDataSel = _my_ds ();
}
