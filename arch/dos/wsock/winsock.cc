#include "winsock.h"


ERROR WSA_ERRORS [] = {
{0,                   "NONE",               "None"},
{WSAEINTR,            "WSAEINTR",           "Interrupted"},
{WSAEBADF,            "WSAEBADF",           "Bad file number"},
{WSAEFAULT,           "WSAEFAULT",          "Bad address"},
{WSAEINVAL,           "WSAEINVAL",          "Invalid argument"},
{WSAEMFILE,           "WSAEMFILE",          "Too many open files"},
/* 
* Windows Sockets definitions of regular Berkeley error constants 
*/ 
{WSAEWOULDBLOCK,      "WSAEWOULDBLOCK",     "Socket marked as non-blocking"},
{WSAEINPROGRESS,      "WSAEINPROGRESS",     "Blocking call in progress"},
{WSAEALREADY,         "WSAEALREADY",        "Command already completed"},
{WSAENOTSOCK,         "WSAENOTSOCK",        "Descriptor is not a socket"},
{WSAEDESTADDRREQ,     "WSAEDESTADDRREQ",    "Destination address required"},
{WSAEMSGSIZE,         "WSAEMSGSIZE",        "Data size too large"},
{WSAEPROTOTYPE,       "WSAEPROTOTYPE",      "Protocol is of wrong type for this socket"},
{WSAENOPROTOOPT,      "WSAENOPROTOOPT",     "Protocol option not supported for this socket type"},
{WSAEPROTONOSUPPORT,  "WSAEPROTONOSUPPORT", "Protocol is not supported"},
{WSAESOCKTNOSUPPORT,  "WSAESOCKTNOSUPPORT", "Socket type not supported by this address family"},
{WSAEOPNOTSUPP,       "WSAEOPNOTSUPP",      "Option not supported"},
{WSAEPFNOSUPPORT,     "WSAEPFNOSUPPORT",    ""},
{WSAEAFNOSUPPORT,     "WSAEAFNOSUPPORT",    "Address family not supported by this protocol"},
{WSAEADDRINUSE,       "WSAEADDRINUSE",      "Address is in use"},
{WSAEADDRNOTAVAIL,    "WSAEADDRNOTAVAIL",   "Address not available from local machine"},
{WSAENETDOWN,         "WSAENETDOWN",        "Network subsystem is down"},
{WSAENETUNREACH,      "WSAENETUNREACH",     "Network cannot be reached"},
{WSAENETRESET,        "WSAENETRESET",       "Connection has been dropped"},
{WSAECONNABORTED,     "WSAECONNABORTED",    ""},
{WSAECONNRESET,       "WSAECONNRESET",      ""},
{WSAENOBUFS,          "WSAENOBUFS",         "No buffer space available"},
{WSAEISCONN,          "WSAEISCONN",         "Socket is already connected"},
{WSAENOTCONN,         "WSAENOTCONN",        "Socket is not connected"},
{WSAESHUTDOWN,        "WSAESHUTDOWN",       "Socket has been shut down"},
{WSAETOOMANYREFS,     "WSAETOOMANYREFS",    ""},
{WSAETIMEDOUT,        "WSAETIMEDOUT",       "Command timed out"},
{WSAECONNREFUSED,     "WSAECONNREFUSED",    "Connection refused"},
{WSAELOOP,            "WSAELOOP",           ""},
{WSAENAMETOOLONG,     "WSAENAMETOOLONG",    ""},
{WSAEHOSTDOWN,        "WSAEHOSTDOWN",       ""},
{WSAEHOSTUNREACH,     "WSAEHOSTUNREACH",    ""},
{WSAENOTEMPTY,        "WSAENOTEMPTY",       ""},
{WSAEPROCLIM,         "WSAEPROCLIM",        ""},
{WSAEUSERS,           "WSAEUSERS",          ""},
{WSAEDQUOT,           "WSAEDQUOT",          ""},
{WSAESTALE,           "WSAESTALE",          ""},
{WSAEREMOTE,          "WSAEREMOTE",         ""},
/* 
* Extended Windows Sockets error constant definitions 
*/ 
{WSASYSNOTREADY,      "WSASYSNOTREADY",     "Network subsystem not ready"},
{WSAVERNOTSUPPORTED,  "WSAVERNOTSUPPORTED", "Version not supported"},
{WSANOTINITIALISED,   "WSANOTINITIALISED",  "WSAStartup() has not been successfully called"},
/* 
* Other error constants. 
*/ 
{WSAHOST_NOT_FOUND,   "WSAHOST_NOT_FOUND",  "Host not found"},
{WSATRY_AGAIN,        "WSATRY_AGAIN",       "Host not found or SERVERFAIL"},
{WSANO_RECOVERY,      "WSANO_RECOVERY",     "Non-recoverable error"},
{WSANO_DATA,          "WSANO_DATA(WSANO_ADDRESS)", "No data record of requested type"},
{WSOCK_WILL_BLOCK,    "WSOCK_WILL_BLOCK",   ""},
{-1,                  "UNK",                "Unknown Error"}};
