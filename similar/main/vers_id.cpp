/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#include "dxxsconf.h"
#include "vers_id.h"

#ifndef DXX_VERSID_BUILD_DATE
#define DXX_VERSID_BUILD_DATE	__DATE__
#endif

#ifndef DXX_VERSID_BUILD_TIME
#define DXX_VERSID_BUILD_TIME	__TIME__
#endif

// "D1X-Rebirth" or "D2X-Rebirth"
constexpr char g_descent_version[]{'D', DXX_BUILD_DESCENT + '0', 'X', '-', 'R', 'e', 'b', 'i', 'r', 't', 'h',
#ifdef DESCENT_VERSION_EXTRA
	' ', DESCENT_VERSION_EXTRA,
#endif
	0};
constexpr char g_descent_build_datetime[21] = DXX_VERSID_BUILD_DATE " " DXX_VERSID_BUILD_TIME;

#ifdef DXX_RBE
#define RECORD_BUILD_VARIABLE(X)	extern const char g_descent_##X[];	\
	constexpr char g_descent_##X[] __attribute_used = {DESCENT_##X};

DXX_RBE(RECORD_BUILD_VARIABLE);
#endif
