#include "vers_id.h"

#ifndef DESCENT_VERSION_EXTRA
#define DESCENT_VERSION_EXTRA	"v" VERSION
#endif

const char g_descent_version[40] = "D2X-Rebirth " DESCENT_VERSION_EXTRA;
const char g_descent_build_datetime[20] = __DATE__ " " __TIME__;
