/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/* Version defines */

#ifndef _VERS_ID
#define _VERS_ID

#define __stringize2(X)	#X
#define __stringize(X)	__stringize2(X)

#define VERSID_EXTRACT_0(A,...)	A
#define VERSID_EXTRACT_1(A,B,...)	B
#define VERSID_EXTRACT_2(A,B,C,...)	C
#define VERSID_EXTRACT_SEQ(F,S)	F(S)

#define DXX_VERSION_MAJORi	VERSID_EXTRACT_SEQ(VERSID_EXTRACT_0, DXX_VERSION_SEQ)
#define DXX_VERSION_MINORi	VERSID_EXTRACT_SEQ(VERSID_EXTRACT_1, DXX_VERSION_SEQ)
#define DXX_VERSION_MICROi	VERSID_EXTRACT_SEQ(VERSID_EXTRACT_2, DXX_VERSION_SEQ)

#define DXX_VERSION_MAJOR __stringize(DXX_VERSION_MAJORi)
#define DXX_VERSION_MINOR __stringize(DXX_VERSION_MINORi)
#define DXX_VERSION_MICRO __stringize(DXX_VERSION_MICROi)

#define VERSION DXX_VERSION_MAJOR "." DXX_VERSION_MINOR "." DXX_VERSION_MICRO
#if defined(DXX_BUILD_DESCENT_I)
#define BASED_VERSION "Registered v1.5 Jan 5, 1996"
#elif defined(DXX_BUILD_DESCENT_II)
#define BASED_VERSION "Full Version v1.2"
#endif

#define DESCENT_VERSION g_descent_version

extern const char g_descent_version[];
extern const char g_descent_build_datetime[21];

#endif /* _VERS_ID */
