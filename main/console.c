/* $Id: console.c,v 1.10 2003-06-02 01:55:03 btb Exp $ */
/*
 *
 * FIXME: put description here
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

#include <SDL.h>
#include "CON_console.h"

#include "pstypes.h"
#include "u_mem.h"
#include "error.h"
#include "console.h"
#include "cmd.h"
#include "gr.h"

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
#if 0
static char con_display[40][40];
static int  con_line; /* Current display line */
#endif

static int con_initialized;

ConsoleInformation *Console;
extern SDL_Surface *screen;

void con_parse(ConsoleInformation *console, char *command);


/* ======
 * con_init - Initialise the console.
 * ======
 */
int con_init(void)
{
	/* Initialise the cvars */
	cvar_registervariable (&con_threshold);
	return 0;
}

void real_con_init(void)
{
	SDL_Rect Con_rect;

	Con_rect.x = Con_rect.y = 0;
	Con_rect.w = Con_rect.h = 300;

	Console = CON_Init("ConsoleFont.png", screen, CON_NUM_LINES, Con_rect);

	Assert(Console);

	CON_SetExecuteFunction(Console, con_parse);

	con_initialized = 1;
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
		va_end (arglist);
		if (text_console_enabled) {
			va_start (arglist, fmt);
			vprintf(fmt, arglist);
			va_end (arglist);
		}

		CON_Out(Console, buffer);

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
#if 0
	char buffer[CMD_MAX_LENGTH], *t;

	/* Check for new input */
	t = fgets(buffer, sizeof(buffer), stdin);
	if (t == NULL) return;

	cmd_parse(buffer);
#endif
	con_draw();
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
#if 1
	CON_DrawConsole(Console);
#else
	char buffer[CON_LINE_LEN+1];
	int i,j;
	for (i = con_line, j=0; j < 20; i = (i+1) % CON_NUM_LINES, j++)
	{
		memcpy(buffer, con_display[i], CON_LINE_LEN);
		buffer[CON_LINE_LEN] = 0;
		gr_string(1,j*10,buffer);
	}
#endif
}

void con_show(void)
{
	if (!con_initialized)
		real_con_init();

	CON_Show(Console);
	CON_Topmost(Console);
}

void con_parse(ConsoleInformation *console, char *command)
{
	cmd_parse(command);
}
