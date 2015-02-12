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

#include <stdlib.h>
#include <float.h>
#include <physfs.h>

#include "console.h"
#include "dxxerror.h"
#include "strutil.h"
#include "u_mem.h"


#define CVAR_MAX_LENGTH 1024


/* The list of cvars */
cvar_t *cvar_list;


static void cvar_free(void)
{
	cvar_t *ptr;
	
	for (ptr = cvar_list; ptr != NULL; ptr = ptr->next)
		d_free(ptr->string);
}


void cvar_cmd_set(int argc, char **argv)
{
	char buf[CVAR_MAX_LENGTH];
	int ret, i;

	if (argc == 2) {
		cvar_t *ptr;
		
		if ((ptr = cvar_find(argv[1])))
			con_printf(CON_NORMAL, "%s: %s", ptr->name, ptr->string);
		else
			con_printf(CON_NORMAL, "set: variable %s not found", argv[1]);
		return;
	}
	
	if (argc == 1) {
		cvar_t *ptr;
		
		for (ptr = cvar_list; ptr != NULL; ptr = ptr->next)
			con_printf(CON_NORMAL, "%s: %s", ptr->name, ptr->string);
		return;
	}
	
	ret = snprintf(buf, CVAR_MAX_LENGTH, "%s", argv[2]);
	if (ret >= CVAR_MAX_LENGTH) {
		con_printf(CON_CRITICAL, "set: value too long (max %d characters)", CVAR_MAX_LENGTH);
		return;
	}
	
	for (i = 3; i < argc; i++) {
		ret = snprintf(buf, CVAR_MAX_LENGTH, "%s %s", buf, argv[i]);
		if (ret >= CVAR_MAX_LENGTH) {
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

	atexit(cvar_free);
}


cvar_t *cvar_find(char *cvar_name)
{
	cvar_t *ptr;
	
	for (ptr = cvar_list; ptr != NULL; ptr = ptr->next)
		if (!d_stricmp(cvar_name, ptr->name))
			return ptr;
	
	return NULL;
}


const char *cvar_complete(char *text)
{
	cvar_t *ptr;
	int len = (int)strlen(text);
	
	if (!len)
		return NULL;
	
	for (ptr = cvar_list; ptr != NULL; ptr = ptr->next)
		if (!d_strnicmp(text, ptr->name, len))
			return ptr->name;
	
	return NULL;
}


/* Register a cvar */
void cvar_registervariable (cvar_t *cvar)
{
	char *stringval;
	cvar_t *ptr;

	Assert(cvar != NULL);
	
	stringval = cvar->string;
	
	cvar->string = d_strdup(stringval);
	cvar->value = fl2f(strtod(cvar->string, NULL));
	cvar->intval = (int)strtol(cvar->string, NULL, 10);
	cvar->next = NULL;
	
	if (cvar_list == NULL) {
		cvar_list = cvar;
		return;
	}
	
	/* insert at end of list */
	for (ptr = cvar_list; ptr->next != NULL; ptr = ptr->next)
		Assert(d_stricmp(cvar->name, ptr->name));
	ptr->next = cvar;
}


/* Set a CVar's value */
void cvar_set_cvar(cvar_t *cvar, char *value)
{
	if (!cvar)
		return;
	
	d_free(cvar->string);
	cvar->string = d_strdup(value);
	cvar->value = fl2f(strtod(cvar->string, (char **) NULL));
	cvar->intval = (int)strtol(cvar->string, NULL, 10);
	con_printf(CON_VERBOSE, "%s: %s", cvar->name, cvar->string);
}


void cvar_set_cvarf(cvar_t *cvar, const char *fmt, ...)
{
	va_list arglist;
	char buf[CVAR_MAX_LENGTH];
	int n;
	
	va_start (arglist, fmt);
	n = vsnprintf (buf, CVAR_MAX_LENGTH, fmt, arglist);
	va_end (arglist);
	
	Assert(!(n < 0 || n > CVAR_MAX_LENGTH));
	
	cvar_set_cvar(cvar, buf);
}


void cvar_set (char *cvar_name, char *value)
{
	cvar_t *cvar;
	
	cvar = cvar_find(cvar_name);
	if (!cvar) {
		Int3();
		con_printf(CON_NORMAL, "cvar %s not found", cvar_name);
		return;
	}
	
	cvar_set_cvar(cvar, value);
}


/* Write archive cvars to file */
void cvar_write(PHYSFS_file *file)
{
	cvar_t *ptr;
	
	for (ptr = cvar_list; ptr != NULL; ptr = ptr->next)
		if (ptr->archive)
			PHYSFSX_printf(file, "%s=%s\n", ptr->name, ptr->string);
}
