/* Version defines */

#ifndef _VERS_ID
#define _VERS_ID

#define __stringize2(X)	#X
#define __stringize(X)	__stringize2(X)

#define D2XMAJOR __stringize(D2XMAJORi)
#define D2XMINOR __stringize(D2XMINORi)
#define D2XMICRO __stringize(D2XMICROi)

#define BASED_VERSION "Full Version v1.2"
#define VERSION D2XMAJOR "." D2XMINOR "." D2XMICRO
#define DESCENT_VERSION "D2X-Rebirth v" VERSION

#endif /* _VERS_ID */
