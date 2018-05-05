/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
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

#include <cstdarg>
#include <map>
#include <stdlib.h>
#include <physfs.h>

#include "console.h"
#include "cvar.h"
#include "dxxerror.h"
#include "strutil.h"
#include "u_mem.h"
#include "game.h"
#include "physfsx.h"

#include "dxxsconf.h"
#include "compiler-range_for.h"

#define CVAR_MAX_LENGTH 1024

/* The list of cvars */
typedef std::map<const char *, std::reference_wrapper<cvar_t>> cvar_list_type;
static cvar_list_type cvar_list;

const char *cvar_t::operator=(const char *s)
{
	cvar_set_cvar(this, s);
	return s;
}

int         cvar_t::operator=(int i)         { cvar_set_cvarf(this, "%d", i);  return this->intval; }

void cvar_cmd_set(unsigned long argc, const char *const *const argv)
{
	char buf[CVAR_MAX_LENGTH];
	int ret;

	if (argc == 2) {
		cvar_t *ptr;
		
		if ((ptr = cvar_find(argv[1])))
			con_printf(CON_NORMAL, "%s: %s", ptr->name, ptr->string.c_str());
		else
			con_printf(CON_NORMAL, "set: variable %s not found", argv[1]);
		return;
	}
	
	if (argc == 1) {
		range_for (const auto &i, cvar_list)
			con_printf(CON_NORMAL, "%s: %s", i.first, i.second.get().string.c_str());
		return;
	}
	
	ret = snprintf(buf, sizeof(buf), "%s", argv[2]);
	if (ret >= CVAR_MAX_LENGTH) {
		con_printf(CON_CRITICAL, "set: value too long (max %d characters)", CVAR_MAX_LENGTH);
		return;
	}
	
	size_t position = ret;
	for (int i = 3; i < argc; i++) {
		ret = snprintf(&buf[position], CVAR_MAX_LENGTH - position, " %s", argv[i]);
		position += ret;
		if (position >= CVAR_MAX_LENGTH)
		{
			con_printf(CON_CRITICAL, "set: value too long (max %d characters)", CVAR_MAX_LENGTH);
			return;
		}
	}
	cvar_set(argv[1], buf);
}


void cvar_init(void)
{
	cmd_addcommand("set", cvar_cmd_set, "set <name> <value>\n"  "    set variable <name> equal to <value>\n"
	                                    "set <name>\n"          "    show value of <name>\n"
	                                    "set\n"                 "    show value of all variables");
}


cvar_t *cvar_find(const char *cvar_name)
{
	const auto i = cvar_list.find(cvar_name);
	return i == cvar_list.end() ? nullptr : &i->second.get();
}


const char *cvar_complete(const char *text)
{
	uint_fast32_t len = strlen(text);
	if (!len)
		return NULL;
	range_for (const auto &i, cvar_list)
		if (!d_strnicmp(text, i.first, len))
			return i.first;
	return NULL;
}


/* Register a cvar */
void cvar_registervariable (cvar_t &cvar)
{
	cvar.value = fl2f(strtod(cvar.string.c_str(), NULL));
	cvar.intval = static_cast<int>(strtol(cvar.string.c_str(), NULL, 10));

	const auto i = cvar_list.insert(cvar_list_type::value_type(cvar.name, cvar));
	if (!i.second)
	{
		Int3();
		con_printf(CON_URGENT, "cvar %s already exists!", cvar.name);
		return;
	}
	/* insert at end of list */
}


/* Set a CVar's value */
void cvar_set_cvar(cvar_t *cvar, const char *value)
{
	if (!cvar)
		return;
	
	cvar->string = value;
	cvar->value = fl2f(strtod(value, NULL));
	cvar->intval = static_cast<int>(strtol(value, NULL, 10));
	con_printf(CON_VERBOSE, "%s: %s", cvar->name, value);
}


void cvar_set_cvarf(cvar_t *cvar, const char *fmt, ...)
{
	va_list arglist;
	char buf[CVAR_MAX_LENGTH];
	int n;
	
	va_start (arglist, fmt);
	n = vsnprintf(buf, sizeof(buf), fmt, arglist);
	va_end (arglist);
	
	if (n < 0 || n > CVAR_MAX_LENGTH) {
		Int3();
		con_printf(CON_CRITICAL, "error setting cvar %s", cvar->name);
		return;
	}

	cvar_set_cvar(cvar, buf);
}


void cvar_set(const char *cvar_name, char *value)
{
	cvar_t *cvar;
	
	cvar = cvar_find(cvar_name);
	if (!cvar) {
		Int3();
		con_printf(CON_NORMAL, "cvar %s not found", cvar_name);
		return;
	}

	if (cvar->flags & CVAR_CHEAT && !cheats_enabled())
	{
		con_printf(CON_NORMAL, "cvar %s is cheat protected.", cvar_name);
		return;
	}

	cvar_set_cvar(cvar, value);
}


/* Write archive cvars to file */
void cvar_write(PHYSFS_File *file)
{
	range_for (const auto &i, cvar_list)
		if (i.second.get().flags & CVAR_ARCHIVE)
			PHYSFSX_printf(file, "%s=%s\n", i.first, i.second.get().string.c_str());
}
