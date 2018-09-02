/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#include "dxxsconf.h"
#include "vers_id.h"

#if defined(DXX_BUILD_DESCENT_I)
#define DXX_NAME_NUMBER	"1"
#elif defined(DXX_BUILD_DESCENT_II)
#define DXX_NAME_NUMBER	"2"
#else
#error "Must set DXX_BUILD_DESCENT_I or DXX_BUILD_DESCENT_II"
#endif

#ifndef DXX_VERSID_BUILD_DATE
#define DXX_VERSID_BUILD_DATE	__DATE__
#endif

#ifndef DXX_VERSID_BUILD_TIME
#define DXX_VERSID_BUILD_TIME	__TIME__
#endif

constexpr char g_descent_version[] = "D" DXX_NAME_NUMBER "X-Rebirth " DESCENT_VERSION_EXTRA;
constexpr char g_descent_build_datetime[21] = DXX_VERSID_BUILD_DATE " " DXX_VERSID_BUILD_TIME;

#ifdef DXX_RBE
#define RECORD_BUILD_VARIABLE(X)	extern const char g_descent_##X[];	\
	const char g_descent_##X[] __attribute_used = #X "=" DESCENT_##X;

DXX_RBE(RECORD_BUILD_VARIABLE);
#endif
