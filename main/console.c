/* $Id: console.c,v 1.15 2003-06-08 01:33:27 btb Exp $ */
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
#ifdef CONSOLE
#include "CON_console.h"
#endif

#include "pstypes.h"
#include "u_mem.h"
#include "error.h"
#include "console.h"
#include "cmd.h"
#include "gr.h"
#include "gamefont.h"
#include "pcx.h"
#include "cfile.h"

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
#if 0
#define CON_LINE_LEN 40
static char con_display[40][40];
static int  con_line; /* Current display line */
#endif

#ifdef CONSOLE
static int con_initialized;

ConsoleInformation *Console;

void con_parse(ConsoleInformation *console, char *command);


/* ======
 * con_free - Free the console.
 * ======
 */
void con_free(void)
{
	if (con_initialized)
		CON_Free(Console);
	con_initialized = 0;
}
#endif


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

#ifdef CONSOLE

#define CON_BG_HIRES (cfexist("scoresb.pcx")?"scoresb.pcx":"scores.pcx")
#define CON_BG_LORES (cfexist("scores.pcx")?"scores.pcx":"scoresb.pcx") // Mac datafiles only have scoresb.pcx
#define CON_BG ((SWIDTH>=640)?CON_BG_HIRES:CON_BG_LORES)

void con_background(char *filename)
{
	int pcx_error;
	grs_bitmap bmp;
	ubyte pal[256*3];

	gr_init_bitmap_data(&bmp);
	pcx_error = pcx_read_bitmap(filename, &bmp, BM_LINEAR, pal);
	Assert(pcx_error == PCX_ERROR_NONE);
	gr_remap_bitmap_good(&bmp, pal, -1, -1);
	CON_Background(Console, &bmp);
	gr_free_bitmap_data(&bmp);
}


void con_init_real(void)
{
	Console = CON_Init(SMALL_FONT, grd_curscreen, CON_NUM_LINES, 0, 0, SWIDTH, SHEIGHT / 2);

	Assert(Console);

	CON_SetExecuteFunction(Console, con_parse);

	con_background(CON_BG);

	con_initialized = 1;

	atexit(con_free);
}
#endif


void con_resize(void)
{
#ifdef CONSOLE
	if (!con_initialized)
		con_init_real();

	CON_Font(Console, SMALL_FONT, gr_getcolor(63, 63, 63), -1);
	CON_Resize(Console, 0, 0, SWIDTH, SHEIGHT / 2);
	con_background(CON_BG);
#endif
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
		if (text_console_enabled)
			printf(buffer);

#ifdef CONSOLE
		if (con_initialized)
			CON_Out(Console, buffer);
#endif

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


int con_events(int key)
{
#ifdef CONSOLE
	return CON_Events(key);
#else
	return key;
#endif
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
#ifdef CONSOLE
	CON_DrawConsole(Console);
#else
#if 0
	char buffer[CON_LINE_LEN+1];
	int i,j;
	for (i = con_line, j=0; j < 20; i = (i+1) % CON_NUM_LINES, j++)
	{
		memcpy(buffer, con_display[i], CON_LINE_LEN);
		buffer[CON_LINE_LEN] = 0;
		gr_string(1,j*10,buffer);
	}
#endif
#endif
}

void con_show(void)
{
#ifdef CONSOLE
	if (!con_initialized)
		con_init_real();

	CON_Show(Console);
	CON_Topmost(Console);
#endif
}

#ifdef CONSOLE
void con_parse(ConsoleInformation *console, char *command)
{
	cmd_parse(command);
}
#endif
