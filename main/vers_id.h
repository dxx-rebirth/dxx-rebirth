/* Version defines */

#ifndef _VERS_ID
#define _VERS_ID

#define VERSION_NAME	"Registered v1.5 Jan 5, 1996"
#define VERSION_TYPE	"REGISTERED"

#ifdef D1XMICRO
#define VERSION D1XMAJOR "." D1XMINOR "." D1XMICRO
#define D1X_IVER (atoi(D1XMAJOR)*10000+atoi(D1XMINOR)*100+atoi(D1XMICRO)
#else
#define VERSION D1XMAJOR "." D1XMINOR
#define D1X_IVER (atoi(D1XMAJOR)*10000+atoi(D1XMINOR)*100)
#endif

#define DESCENT_VERSION "D1X-Rebirth v" VERSION

#endif /* _VERS_ID */
