/* Version defines */

#ifndef _VERS_ID
#define _VERS_ID

#define VERSION_TYPE	"Full Version"

#ifdef D2XMICRO
#define VERSION D2XMAJOR "." D2XMINOR "." D2XMICRO
#else
#define VERSION D2XMAJOR "." D2XMINOR
#endif

#define D2X_IVER (atoi(D2XMAJOR)*10000+atoi(D2XMINOR)*100)
#define DESCENT_VERSION "D2X-Rebirth v" VERSION

#endif /* _VERS_ID */
