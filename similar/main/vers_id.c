#include "vers_id.h"

#ifndef DESCENT_VERSION_EXTRA
#define DESCENT_VERSION_EXTRA	"v" VERSION
#endif

#if defined(DXX_BUILD_DESCENT_I)
#define DXX_NAME_NUMBER	"1"
#elif defined(DXX_BUILD_DESCENT_II)
#define DXX_NAME_NUMBER	"2"
#endif

const char g_descent_version[40] = "D" DXX_NAME_NUMBER "X-Rebirth " DESCENT_VERSION_EXTRA;
const char g_descent_build_datetime[21] = __DATE__ " " __TIME__;
