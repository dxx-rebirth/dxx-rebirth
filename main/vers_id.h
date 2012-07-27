/* Version defines */

#ifndef _VERS_ID
#define _VERS_ID

+#define __stringize2(X)	#X
+#define __stringize(X)	__stringize2(X)
+
+#define D1XMAJOR __stringize(D1XMAJORi)
+#define D1XMINOR __stringize(D1XMINORi)
+#define D1XMICRO __stringize(D1XMICROi)

#define BASED_VERSION "Registered v1.5 Jan 5, 1996"
#define VERSION D1XMAJOR "." D1XMINOR "." D1XMICRO
#define DESCENT_VERSION "D1X-Rebirth v" VERSION

#endif /* _VERS_ID */
