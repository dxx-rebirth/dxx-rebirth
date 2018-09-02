/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 *
 */
/*
 *
 * Console variables
 *
 */

#pragma once

#include <cstdint>
#include <string>
#include <physfs.h>
#include "maths.h"
#include "dxxsconf.h"

// cvar flags
#define CVAR_NONE           0
#define CVAR_ARCHIVE        1   // save to descent.cfg
#define CVAR_CHEAT          512 // can not be changed if cheats are disabled

// Other examples we could implement (from quake 3, see also iconvar.h from the Source sdk.)
//#define CVAR_USERINFO       2   // sent to server on connect or change
//#define CVAR_SERVERINFO     4   // sent in response to front end requests
//#define CVAR_SYSTEMINFO     8   // these cvars will be duplicated on all clients
//#define CVAR_INIT           16  // don't allow change from console at all, but can be set from the command line
//#define CVAR_LATCH          32  // will only change when C code next does a Cvar_Get(), so it can't be changed
//                                // without proper initialization.  modified will be set, even though the value hasn't changed yet
//#define CVAR_ROM            64  // display only, cannot be set by user at all
//#define CVAR_USER_CREATED   128 // created by a set command
//#define CVAR_TEMP           256 // can be set even when cheats are disabled, but is not archived
//#define CVAR_NORESTART     1024 // do not clear when a cvar_restart is issued

struct cvar_t
{
	const char *name;
	std::string string;
	uint16_t flags;
	fix value;
	int intval;

	operator    int() const { return intval; }
	const char  *operator=(const char *s);
	int          operator=(int i);
	unsigned int operator=(unsigned int i) { return *this = static_cast<int>(i); }
};

void cvar_init(void);
void cvar_cmd_set(unsigned long, const char *const *const);

/* Register a CVar with the name and string and optionally archive elements set */
void cvar_registervariable (cvar_t &cvar);

/* Set a CVar's value */
void cvar_set_cvar(cvar_t *cvar, const char *value);
__attribute_format_printf(2, 3)
void cvar_set_cvarf(cvar_t *cvar, const char *fmt, ...);

/* Equivalent to typing <var_name> <value> at the console */
void cvar_set(const char *cvar_name, char *value);

/* Get the pointer to a cvar by name */
cvar_t *cvar_find(const char *cvar_name);

/* Try to autocomplete a cvar name */
const char *cvar_complete(const char *text);

/* Write archive cvars to file */
void cvar_write(PHYSFS_File *file);
