/*
 * $Source: /cvs/cvsroot/d2x/main/console.c,v $
 * $Revision: 1.6 $
 * $Author: btb $
 * $Date: 2002-07-22 02:17:10 $
 *
 * FIXME: put description here
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.5  2001/10/19 09:47:34  bradleyb
 * RCS headers added
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include "pstypes.h"
#include "u_mem.h"
#include "error.h"
#include "console.h"
#include "cmd.h"

#ifndef __MSDOS__
int text_console_enabled = 1;
#else
int isvga();
#define text_console_enabled (!isvga())
#endif

cvar_t *cvar_vars = NULL;

/* Console specific cvars */
/* How discriminating we are about which messages are displayed */
cvar_t con_threshold = {"con_threshold", "0",};

/* Private console stuff */
#define CON_NUM_LINES 40
#define CON_LINE_LEN 40
static char con_display[40][40];
static int  con_line; /* Current display line */

/* ======
 * con_init - Initialise the console.
 * ======
 */
int con_init(void)
{
	/* Make sure the output is unbuffered */
	if (text_console_enabled) {
		setbuf (stdout, NULL);
		setbuf (stderr, NULL);
	}

	memset(con_display, ' ', sizeof(con_display));
	con_line = 0;

	/* Initialise the cvars */
	cvar_registervariable (&con_threshold);
	return 0;	
}

/* ======
 * con_printf - Print a message to the console.
 * ======
 */
void con_printf(int priority, char *fmt, ...)
{
	va_list arglist;
	char buffer[2048];

	if (priority <= ((int)con_threshold.value))
	{
		va_start (arglist, fmt);
                vsprintf (buffer,  fmt, arglist);
		if (text_console_enabled) vprintf(fmt, arglist);
		va_end (arglist);

/*		for (i=0; i<l; i+=CON_LINE_LEN,con_line++)
		{
			memcpy(con_display, &buffer[i], min(80, l-i));	
		}*/
	}
}

/* ======
 * con_update - Check for new console input. If it's there, use it.
 * ======
 */
void con_update(void)
{
//	char buffer[CMD_MAX_LENGTH], *t;

	/* Check for new input */
/*	t = fgets(buffer, sizeof(buffer), stdin);
	if (t == NULL) return;

	cmd_parse(buffer);*/
//	con_draw();
}

/* ======
 * cvar_registervariable - Register a CVar
 * ======
 */
void cvar_registervariable (cvar_t *cvar)
{
	cvar_t *ptr;
	
	Assert(cvar != NULL);

	cvar->next = NULL;
	cvar->value = strtod(cvar->string, (char **) NULL);

	if (cvar_vars == NULL)
	{
		cvar_vars = cvar;
	} else
	{
		for (ptr = cvar_vars; ptr->next != NULL; ptr = ptr->next) ;
		ptr->next = cvar;
	}
}

/* ======
 * cvar_set - Set a CVar's value
 * ======
 */
void cvar_set (char *cvar_name, char *value)
{
	cvar_t *ptr;

	for (ptr = cvar_vars; ptr != NULL; ptr = ptr->next)
		if (!strcmp(cvar_name, ptr->name)) break;

	if (ptr == NULL) return; // If we didn't find the cvar, give up

	ptr->value = strtod(value, (char **) NULL);
}

/* ======
 * cvar() - Get a CVar's value
 * ======
 */
float cvar (char *cvar_name)
{
	cvar_t *ptr;

	for (ptr = cvar_vars; ptr != NULL; ptr = ptr->next)
		if (!strcmp(cvar_name, ptr->name)) break;

	if (ptr == NULL) return 0.0; // If we didn't find the cvar, give up

	return ptr->value;
}


/* ==========================================================================
 * DRAWING
 * ==========================================================================
 */
void con_draw(void)
{
/*	char buffer[CON_LINE_LEN+1];
	int i,j; */
/*	for (i = con_line, j=0; j < 20; i = (i+1) % CON_NUM_LINES, j++)
	{
		memcpy(buffer, con_display[i], CON_LINE_LEN);
		buffer[CON_LINE_LEN] = 0;
		gr_string(1,j*10,buffer);
	}*/
}
