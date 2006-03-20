/**********************************************************************/
/**                        Microsoft Windows                         **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    wsock.h

    WSOCK.386 VxD service definitions.


    FILE HISTORY:
        DavidKa     ??-???-???? Created.
        KeithMo     30-Dec-1993 Cleaned up a bit, made H2INC-able.

*/


#ifndef __wsock_h__
#define __wsock_h__


//
//  Service table.
//

#ifndef Not_VxD

/*XLATOFF*/
#define WSOCK_Service   Declare_Service
#pragma warning ( disable : 4003 )          // disable not enough params warning
/*XLATON*/

/*MACROS*/
Begin_Service_Table(WSOCK)

WSOCK_Service (WSOCK_Get_Version, LOCAL)
WSOCK_Service (WSOCK_Register, LOCAL)
WSOCK_Service (WSOCK_Deregister, LOCAL)
WSOCK_Service (WSOCK_SignalNotify, LOCAL)
WSOCK_Service (WSOCK_SignalAllNotify, LOCAL)

End_Service_Table(WSOCK)
/*ENDMACROS*/

/*XLATOFF*/
#pragma warning ( default : 4003 )          // restore not enough params warning
/*XLATON*/

#endif  // Not_VxD


//
//  Version numbers.
//

#define WSOCK_Ver_Major         1
#define WSOCK_Ver_Minor         0


//
//  The current provider interface version number.  Increment
//  this constant after any change that effects the provider
//  interface.
//

#define WSOCK_INTERFACE_VERSION 0x00000001


//
//  A locally-defined error code, indicating the underlying
//  provider returned WSAEWOULDBLOCK for an operation invoked
//  on a blocking socket.
//

#define WSOCK_WILL_BLOCK        0xFFFF


//
//  Infinite wait time for send/recv timeout.
//

#define SOCK_IO_TIME            (DWORD)-1L


//
//  Incomplete types.
//

#ifndef LPSOCK_INFO_DEFINED
#define LPSOCK_INFO_DEFINED
typedef struct _SOCK_INFO FAR * LPSOCK_INFO;
#endif  // LPSOCK_INFO_DEFINED


#ifdef MASM

//
//  Stolen simplified definitions so we don't force
//  H2INC to parse WINNT.H, WINSOCK.H, et al.
//

typedef DWORD       LIST_ENTRY[2];
typedef WORD        LINGER[2];
typedef VOID FAR  * LPVOID;

#endif


//
//  All FD_* events.
//

#define FD_ALL  (FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE)


//
//  This "special" FD_ event is used in select so that we may
//  synthesize proper exceptfds for failed connection attempts.
//

#define FD_FAILED_CONNECT   0x0100


//
//  A notification object.  One of these objects is created
//  and attached to a socket for every thread that is blocked
//  in an API.
//

typedef struct _WSNOTIFY {
    LIST_ENTRY  PerSocketList;      // per-socket list of notify objects
    LIST_ENTRY  GlobalList;         // global list of all notify objects
    LPSOCK_INFO OwningSocket;       // the socket that "owns" this object
    DWORD       Flags;              // private notification flags (see below)
    DWORD       EventMask;          // events the client is interested in
    DWORD       Status;             // the completion status
    DWORD       OwningThread;       // either ring 0 thread id or VM handle
    LPVOID      ApcRoutine;         // the user-mode APC to schedule
    DWORD       ApcContext;         // a user-supplied context value

} WSNOTIFY;

//
// NOTE: For overlapped IO (asend and arecv), OwningThread field is used to
// store the pointer to the overlapped structure.  ApcRoutine contains a pointer
// to the IO buffer and ApcContext is the IO buffer length.
//
// Field name       Use
// --------------------
// OwningThread     pointer to overlapped structure
// ApcRoutine       pointer to IO buffer
// ApcContext       length of IO buffer
//

#ifndef LPWSNOTIFY_DEFINED
#define LPWSNOTIFY_DEFINED
typedef struct _WSNOTIFY FAR * LPWSNOTIFY;
#endif  // LPWSNOTIFY_DEFINED


//
//  Notification object flags.
//

#define NOTIFY_FLAG_16BIT_CLIENT    0x00000001
#define NOTIFY_FLAG_ASYNC_SEND      0x00000002
#define NOTIFY_FLAG_ASYNC_RECV      0x00000004
#define NOTIFY_FLAG_VALID_MASK      0x00000007


//
//  A list of socket/event mask pairs.  A pointer to an array
//  of these structures is passed to WsCreateMultipleNotify to
//  create multiple notification objects.
//

typedef struct _SOCK_LIST {
    LPSOCK_INFO Socket;             // the target socket
    DWORD       EventMask;          // events the client is interested in
    DWORD       Context;            // user-defined context value (handle?)

} SOCK_LIST;

#ifndef LPSOCK_LIST_DEFINED
#define LPSOCK_LIST_DEFINED
typedef struct _SOCK_LIST FAR * LPSOCK_LIST;
#endif  // LPSOCK_LIST_DEFINED


//
//  A Winsock I/O Status Block.  This structure contains all information
//  about completing/cancelling a blocking socket operation.  Whenever an
//  APC is scheduled against a thread, the APC Context value points to
//  the thread's WSIOSTATUS structure.  This is especially useful for
//  16-bit applications, since it enables the WSOCK VxD to "unblock" a
//  16-bit thread without calling "up" into user mode.
//

typedef struct _WSIOSTATUS {
    DWORD   IoStatus;               // completion status
    char    IoCompleted;            // i/o has completed
    char    IoCancelled;            // i/o has been cancelled
    char    IoTimedOut;             // i/o has timed out
    char    IoSpare1;               // spare (for dword alignment)

} WSIOSTATUS;
typedef struct _WSIOSTATUS FAR * LPWSIOSTATUS;


//
//  This is is a special APC Routine value that may be passed into the
//  various CreateNotify services.  If this value is specified as the
//  APC Routine, then the APC is not actually invoked, and the APC
//  Context is assumed to point to the thread's WSIOSTATUS block.
//
//  Note that this is used for 16-bit applications only!
//

#define SPECIAL_16BIT_APC ((LPVOID)-1L)


//
//  This section defines the constants and structures necessary for
//  communication between the WinSock DLLs and WSOCK.386.  For each
//  command, there is defined a unique opcode and a structure defining
//  the command parameters.
//
//  Also, for each command, a 16-bit constants, *_MAPIN is defined.  This
//  is used by the 16-bit interface to the provider VxDs to control the
//  mapping of segmented 16:16 pointers to flat 0:32 pointers within the
//  command parameter structures.  Each structure is considered to be a
//  sequence of one or more DWORDS.  All pointers that must be mapped
//  MUST appear FIRST in the command structures.  The *_MAPIN constant
//  specifies how many parameters should be mapped for each command.
//
//  Note also that the LPSOCK_INFO pointers are not mapped, since they are
//  opaque at the application level.  ApcRoutine fields are not mapped either,
//  since 32-bit APCs are scheduled via VWIN32, and 16-bit APCs are called
//  directly.
//
//        D A N G E R ! !       W A R N I N G ! !       D A N G E R ! !
//
//  IF YOU CHANGE ANY FIELDS IN ANY OF THESE STRUCTURES, ENSURE THE *_MAPIN
//  CONSTANT REMAINS ACCURATE!  IF YOU DON'T, YOU'LL BREAK 16-BIT WINSOCK!!
//
//        D A N G E R ! !       W A R N I N G ! !       D A N G E R ! !
//

#define WSOCK_FIRST_CMD             0x00000100

#define WSOCK_ACCEPT_CMD            (WSOCK_FIRST_CMD + 0x0000)
#define WSOCK_BIND_CMD              (WSOCK_FIRST_CMD + 0x0001)
#define WSOCK_CLOSESOCKET_CMD       (WSOCK_FIRST_CMD + 0x0002)
#define WSOCK_CONNECT_CMD           (WSOCK_FIRST_CMD + 0x0003)
#define WSOCK_GETPEERNAME_CMD       (WSOCK_FIRST_CMD + 0x0004)
#define WSOCK_GETSOCKNAME_CMD       (WSOCK_FIRST_CMD + 0x0005)
#define WSOCK_GETSOCKOPT_CMD        (WSOCK_FIRST_CMD + 0x0006)
#define WSOCK_IOCTLSOCKET_CMD       (WSOCK_FIRST_CMD + 0x0007)
#define WSOCK_LISTEN_CMD            (WSOCK_FIRST_CMD + 0x0008)
#define WSOCK_RECV_CMD              (WSOCK_FIRST_CMD + 0x0009)
#define WSOCK_SELECT_SETUP_CMD      (WSOCK_FIRST_CMD + 0x000a)
#define WSOCK_SELECT_CLEANUP_CMD    (WSOCK_FIRST_CMD + 0x000b)
#define WSOCK_ASYNC_SELECT_CMD      (WSOCK_FIRST_CMD + 0x000c)
#define WSOCK_SEND_CMD              (WSOCK_FIRST_CMD + 0x000d)
#define WSOCK_SETSOCKOPT_CMD        (WSOCK_FIRST_CMD + 0x000e)
#define WSOCK_SHUTDOWN_CMD          (WSOCK_FIRST_CMD + 0x000f)
#define WSOCK_SOCKET_CMD            (WSOCK_FIRST_CMD + 0x0010)

#define WSOCK_CREATE_CMD            (WSOCK_FIRST_CMD + 0x0011)
#define WSOCK_CREATE_MULTIPLE_CMD   (WSOCK_FIRST_CMD + 0x0012)
#define WSOCK_DESTROY_CMD           (WSOCK_FIRST_CMD + 0x0013)
#define WSOCK_DESTROY_BY_SOCKET_CMD (WSOCK_FIRST_CMD + 0x0014)
#define WSOCK_DESTROY_BY_THREAD_CMD (WSOCK_FIRST_CMD + 0x0015)
#define WSOCK_SIGNAL_CMD            (WSOCK_FIRST_CMD + 0x0016)
#define WSOCK_SIGNAL_ALL_CMD        (WSOCK_FIRST_CMD + 0x0017)

#define WSOCK_CONTROL_CMD           (WSOCK_FIRST_CMD + 0x0018)

#define WSOCK_REGISTER_POSTMSG_CMD  (WSOCK_FIRST_CMD + 0x0019)

#define WSOCK_ARECV_CMD             (WSOCK_FIRST_CMD + 0x001a)
#define WSOCK_ASEND_CMD             (WSOCK_FIRST_CMD + 0x001b)

#ifdef CHICAGO
#define WSOCK_LAST_CMD              WSOCK_ASEND_CMD
#else
#define WSOCK_LAST_CMD              WSOCK_REGISTER_POSTMSG_CMD
#endif


//
//  Socket APIs.
//

typedef struct _WSOCK_ACCEPT_PARAMS {
    LPVOID      Address;
    LPSOCK_INFO ListeningSocket;
    LPSOCK_INFO ConnectedSocket;
    DWORD       AddressLength;
    DWORD       ConnectedSocketHandle;
    LPVOID      ApcRoutine;
    DWORD       ApcContext;

} WSOCK_ACCEPT_PARAMS;
typedef struct _WSOCK_ACCEPT_PARAMS FAR * LPWSOCK_ACCEPT_PARAMS;
#define WSOCK_ACCEPT_MAPIN                  1


typedef struct _WSOCK_BIND_PARAMS {
    LPVOID      Address;
    LPSOCK_INFO Socket;
    DWORD       AddressLength;
    LPVOID      ApcRoutine;
    DWORD       ApcContext;

} WSOCK_BIND_PARAMS;
typedef struct _WSOCK_BIND_PARAMS FAR * LPWSOCK_BIND_PARAMS;
#define WSOCK_BIND_MAPIN                    1


typedef struct _WSOCK_CLOSESOCKET_PARAMS {
    LPSOCK_INFO Socket;

} WSOCK_CLOSESOCKET_PARAMS;
typedef struct _WSOCK_CLOSESOCKET_PARAMS FAR * LPWSOCK_CLOSESOCKET_PARAMS;
#define WSOCK_CLOSESOCKET_MAPIN             0


typedef struct _WSOCK_CONNECT_PARAMS {
    LPVOID      Address;
    LPSOCK_INFO Socket;
    DWORD       AddressLength;
    LPVOID      ApcRoutine;
    DWORD       ApcContext;

} WSOCK_CONNECT_PARAMS;
typedef struct _WSOCK_CONNECT_PARAMS FAR * LPWSOCK_CONNECT_PARAMS;
#define WSOCK_CONNECT_MAPIN                 1


typedef struct _WSOCK_GETPEERNAME_PARAMS {
    LPVOID      Address;
    LPSOCK_INFO Socket;
    DWORD       AddressLength;

} WSOCK_GETPEERNAME_PARAMS;
typedef struct _WSOCK_GETPEERNAME_PARAMS FAR * LPWSOCK_GETPEERNAME_PARAMS;
#define WSOCK_GETPEERNAME_MAPIN             1


typedef struct _WSOCK_GETSOCKNAME_PARAMS {
    LPVOID      Address;
    LPSOCK_INFO Socket;
    DWORD       AddressLength;

} WSOCK_GETSOCKNAME_PARAMS;
typedef struct _WSOCK_GETSOCKNAME_PARAMS FAR * LPWSOCK_GETSOCKNAME_PARAMS;
#define WSOCK_GETSOCKNAME_MAPIN             1


typedef struct _WSOCK_GETSOCKOPT_PARAMS {
    LPVOID      Value;
    LPSOCK_INFO Socket;
    DWORD       OptionLevel;
    DWORD       OptionName;
    DWORD       ValueLength;
    DWORD       IntValue;

} WSOCK_GETSOCKOPT_PARAMS;
typedef struct _WSOCK_GETSOCKOPT_PARAMS FAR * LPWSOCK_GETSOCKOPT_PARAMS;
#define WSOCK_GETSOCKOPT_MAPIN              1


typedef struct _WSOCK_IOCTLSOCKET_PARAMS {
    LPSOCK_INFO Socket;
    DWORD       Command;
    DWORD       Param;

} WSOCK_IOCTLSOCKET_PARAMS;
typedef struct _WSOCK_IOCTLSOCKET_PARAMS FAR * LPWSOCK_IOCTLSOCKET_PARAMS;
#define WSOCK_IOCTLSOCKET_MAPIN             0


typedef struct _WSOCK_LISTEN_PARAMS {
    LPSOCK_INFO Socket;
    DWORD       Backlog;

} WSOCK_LISTEN_PARAMS;
typedef struct _WSOCK_LISTEN_PARAMS FAR * LPWSOCK_LISTEN_PARAMS;
#define WSOCK_LISTEN_MAPIN                  0


typedef struct _WSOCK_RECV_PARAMS {
    LPVOID      Buffer;
    LPVOID      Address;
    LPSOCK_INFO Socket;
    DWORD       BufferLength;
    DWORD       Flags;
    DWORD       AddressLength;
    DWORD       BytesReceived;
    LPVOID      ApcRoutine;
    DWORD       ApcContext;
    DWORD       Timeout;

} WSOCK_RECV_PARAMS;
typedef struct _WSOCK_RECV_PARAMS FAR * LPWSOCK_RECV_PARAMS;
#define WSOCK_RECV_MAPIN                    2


typedef struct _WSOCK_SELECT_SETUP_PARAMS {
    LPSOCK_LIST ReadList;
    LPSOCK_LIST WriteList;
    LPSOCK_LIST ExceptList;
    DWORD       ReadCount;
    DWORD       WriteCount;
    DWORD       ExceptCount;
    LPVOID      ApcRoutine;
    DWORD       ApcContext;

} WSOCK_SELECT_SETUP_PARAMS;
typedef struct _WSOCK_SELECT_SETUP_PARAMS FAR * LPWSOCK_SELECT_SETUP_PARAMS;
#define WSOCK_SELECT_SETUP_MAPIN            3


typedef struct _WSOCK_SELECT_CLEANUP_PARAMS {
    LPSOCK_LIST ReadList;
    LPSOCK_LIST WriteList;
    LPSOCK_LIST ExceptList;
    DWORD       ReadCount;
    DWORD       WriteCount;
    DWORD       ExceptCount;

} WSOCK_SELECT_CLEANUP_PARAMS;
typedef struct _WSOCK_SELECT_CLEANUP_PARAMS FAR * LPWSOCK_SELECT_CLEANUP_PARAMS;
#define WSOCK_SELECT_CLEANUP_MAPIN          3


typedef struct _WSOCK_ASYNC_SELECT_PARAMS {
    LPSOCK_INFO Socket;
    DWORD       Window;
    DWORD       Message;
    DWORD       Events;

} WSOCK_ASYNC_SELECT_PARAMS;
typedef struct _WSOCK_ASYNC_SELECT_PARAMS FAR * LPWSOCK_ASYNC_SELECT_PARAMS;
#define WSOCK_ASYNC_SELECT_MAPIN            0


typedef struct _WSOCK_SEND_PARAMS {
    LPVOID      Buffer;
    LPVOID      Address;
    LPSOCK_INFO Socket;
    DWORD       BufferLength;
    DWORD       Flags;
    DWORD       AddressLength;
    DWORD       BytesSent;
    LPVOID      ApcRoutine;
    DWORD       ApcContext;
    DWORD       Timeout;

} WSOCK_SEND_PARAMS;
typedef struct _WSOCK_SEND_PARAMS FAR * LPWSOCK_SEND_PARAMS;
#define WSOCK_SEND_MAPIN                    2


typedef struct _WSOCK_SETSOCKOPT_PARAMS {
    LPVOID      Value;
    LPSOCK_INFO Socket;
    DWORD       OptionLevel;
    DWORD       OptionName;
    DWORD       ValueLength;
    DWORD       IntValue;

} WSOCK_SETSOCKOPT_PARAMS;
typedef struct _WSOCK_SETSOCKOPT_PARAMS FAR * LPWSOCK_SETSOCKOPT_PARAMS;
#define WSOCK_SETSOCKOPT_MAPIN              1


typedef struct _WSOCK_SOCKET_PARAMS {
    DWORD       AddressFamily;
    DWORD       SocketType;
    DWORD       Protocol;
    LPSOCK_INFO NewSocket;
    DWORD       NewSocketHandle;

} WSOCK_SOCKET_PARAMS;
typedef struct _WSOCK_SOCKET_PARAMS FAR * LPWSOCK_SOCKET_PARAMS;
#define WSOCK_SOCKET_MAPIN                  0


typedef struct _WSOCK_SHUTDOWN_PARAMS {
    LPSOCK_INFO Socket;
    DWORD       How;

} WSOCK_SHUTDOWN_PARAMS;
typedef struct _WSOCK_SHUTDOWN_PARAMS FAR * LPWSOCK_SHUTDOWN_PARAMS;
#define WSOCK_SHUTDOWN_MAPIN                0


//
//  Notification APIs.
//

typedef struct _WSOCK_CREATE_PARAMS {
    LPSOCK_INFO Socket;
    DWORD       Event;
    LPVOID      ApcRoutine;
    DWORD       ApcContext;
    LPWSNOTIFY  Notify;

} WSOCK_CREATE_PARAMS;
typedef struct _WSOCK_CREATE_PARAMS FAR * LPWSOCK_CREATE_PARAMS;
#define WSOCK_CREATE_MAPIN                  0


typedef struct _WSOCK_CREATE_MULTIPLE_PARAMS {
    LPSOCK_LIST ReadList;
    LPSOCK_LIST WriteList;
    LPSOCK_LIST ExceptList;
    DWORD       ReadCount;
    DWORD       WriteCount;
    DWORD       ExceptCount;
    LPVOID      ApcRoutine;
    DWORD       ApcContext;

} WSOCK_CREATE_MULTIPLE_PARAMS;
typedef struct _WSOCK_CREATE_MULTIPLE_PARAMS FAR * LPWSOCK_CREATE_MULTIPLE_PARAMS;
#define WSOCK_CREATE_MULTIPLE_MAPIN         3


typedef struct _WSOCK_DESTROY_PARAMS {
    LPWSNOTIFY  Notify;

} WSOCK_DESTROY_PARAMS;
typedef struct _WSOCK_DESTROY_PARAMS FAR * LPWSOCK_DESTROY_PARAMS;
#define WSOCK_DESTROY_MAPIN                 1


typedef struct _WSOCK_DESTROY_BY_SOCKET_PARAMS {
    LPSOCK_INFO Socket;

} WSOCK_DESTROY_BY_SOCKET_PARAMS;
typedef struct _WSOCK_DESTROY_BY_SOCKET_PARAMS FAR * LPWSOCK_DESTROY_BY_SOCKET_PARAMS;
#define WSOCK_DESTROY_BY_SOCKET_MAPIN       0


//
//  Note that there is no structure defined for WSOCK_DESTROY_BY_THREAD,
//  since this function takes no parameters, and C won't allow us to have
//  an empty structure, but if it did, it would look like this:
//
//  typedef struct _WSOCK_DESTROY_BY_THREAD_PARAMS {
//
//  } WSOCK_DESTROY_BY_THREAD_PARAMS;
//
typedef struct _WSOCK_DESTROY_BY_THREAD_PARAMS FAR * LPWSOCK_DESTROY_BY_THREAD_PARAMS;
#define WSOCK_DESTROY_BY_THREAD_MAPIN       0


typedef struct _WSOCK_SIGNAL_PARAMS {
    LPSOCK_INFO Socket;
    DWORD       Event;
    DWORD       Status;

} WSOCK_SIGNAL_PARAMS;
typedef struct _WSOCK_SIGNAL_PARAMS FAR * LPWSOCK_SIGNAL_PARAMS;
#define WSOCK_SIGNAL_MAPIN                  0


typedef struct _WSOCK_SIGNAL_ALL_PARAMS {
    LPSOCK_INFO Socket;
    DWORD       Status;

} WSOCK_SIGNAL_ALL_PARAMS;
typedef struct _WSOCK_SIGNAL_ALL_PARAMS FAR * LPWSOCK_SIGNAL_ALL_PARAMS;
#define WSOCK_SIGNAL_ALL_MAPIN              0


typedef struct _WSOCK_REGISTER_POSTMSG_PARAMS {
    DWORD       PostMessageCallback;

} WSOCK_REGISTER_POSTMSG_PARAMS;
typedef struct _WSOCK_REGISTER_POSTMSG_PARAMS FAR * LPWSOCK_REGISTER_POSTMSG_PARAMS;
#define WSOCK_REGISTER_POSTMSG_MAPIN        0


typedef struct _WSOCK_CONTROL_PARAMS {
    LPVOID  InputBuffer;
    LPVOID  OutputBuffer;
    DWORD   InputBufferLength;
    DWORD   OutputBufferLength;
    DWORD   Protocol;
    DWORD   Action;

} WSOCK_CONTROL_PARAMS;
typedef struct _WSOCK_CONTROL_PARAMS FAR * LPWSOCK_CONTROL_PARAMS;
#define WSOCK_CONTROL_MAPIN                 2



typedef struct _WSOCK_ASYNCIO_PARAMS {
    LPVOID      Buffer;
    LPVOID      Address;
    LPSOCK_INFO Socket;
    DWORD       BufferLength;
    LPVOID      Overlap;

} WSOCK_ASYNCIO_PARAMS;
typedef struct _WSOCK_ASYNCIO_PARAMS FAR * LPWSOCK_ASYNCIO_PARAMS;
#define WSOCK_ASYNCIO_MAPIN                 0

#endif  // _WSOCK_H_

