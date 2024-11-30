/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/* Version defines */

#pragma once

#define DXX_STRINGIZE2(X)	#X
#define DXX_STRINGIZE(X)	DXX_STRINGIZE2(X)

#define VERSID_EXTRACT_0(A,...)	A
#define VERSID_EXTRACT_1(A,B,...)	B
#define VERSID_EXTRACT_2(A,B,C,...)	C
#define VERSID_EXTRACT_SEQ(F,S)	F(S)

#define DXX_VERSION_MAJORiu	VERSID_EXTRACT_SEQ(VERSID_EXTRACT_0, DXX_VERSION_SEQ)
#define DXX_VERSION_MINORiu	VERSID_EXTRACT_SEQ(VERSID_EXTRACT_1, DXX_VERSION_SEQ)
#define DXX_VERSION_MICROiu	VERSID_EXTRACT_SEQ(VERSID_EXTRACT_2, DXX_VERSION_SEQ)

#define DXX_VERSION_MAJORi static_cast<uint16_t>(DXX_VERSION_MAJORiu)
#define DXX_VERSION_MINORi static_cast<uint16_t>(DXX_VERSION_MINORiu)
#define DXX_VERSION_MICROi static_cast<uint16_t>(DXX_VERSION_MICROiu)

#define DXX_VERSION_STR	\
	DXX_STRINGIZE(DXX_VERSION_MAJORiu) "."	\
	DXX_STRINGIZE(DXX_VERSION_MINORiu) "."	\
	DXX_STRINGIZE(DXX_VERSION_MICROiu)

#ifdef DXX_BUILD_DESCENT
#if DXX_BUILD_DESCENT == 1
#define BASED_VERSION "Registered v1.5 Jan 5, 1996"
#elif defined(DXX_BUILD_DESCENT_II)
#define BASED_VERSION "Full Version v1.2"
#endif
#endif

#define DESCENT_VERSION g_descent_version

#ifndef RC_INVOKED
extern const char g_descent_version[];
extern const char g_descent_build_datetime[];
#endif
