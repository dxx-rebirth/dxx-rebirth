/*
 *
 * Game console
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <SDL/SDL.h>
#include "console.h"
#include "args.h"
#include "gr.h"
#include "physfsx.h"
#include "gamefont.h"
#include "key.h"
#include "vers_id.h"
#include "game.h"

PHYSFS_file *gamelog_fp=NULL;
struct console_buffer con_buffer[CON_LINES_MAX];
int con_render=0;
static int con_scroll_offset=0;

void con_add_buffer_line(int priority, char *buffer)
{
	int i=0;

	/* shift con_buffer for one line */
	for (i=1; i<CON_LINES_MAX; i++)
	{
		con_buffer[i-1].priority=con_buffer[i].priority;
		memcpy(&con_buffer[i-1].line,&con_buffer[i].line,CON_LINE_LENGTH);
	}
	memset(con_buffer[CON_LINES_MAX-1].line,'\0',sizeof(CON_LINE_LENGTH));
	con_buffer[CON_LINES_MAX-1].priority=priority;

	memcpy(&con_buffer[CON_LINES_MAX-1].line,buffer,CON_LINE_LENGTH);
	for (i=0; i<CON_LINE_LENGTH; i++)
	{
		con_buffer[CON_LINES_MAX-1].line[i]=buffer[i];
		if (buffer[strlen(buffer)-1] == '\n')
			con_buffer[CON_LINES_MAX-1].line[strlen(buffer)-1]='\0';
	}
}

void con_printf(int priority, char *fmt, ...)
{
	va_list arglist;
	char buffer[CON_LINE_LENGTH];

	memset(buffer,'\0',CON_LINE_LENGTH);

	if (priority <= ((int)GameArg.DbgVerbose))
	{
		char *p1, *p2;

		va_start (arglist, fmt);
		vsprintf (buffer,  fmt, arglist);
		va_end (arglist);

		/* Produce a sanitised version and send it to the console */
		p1 = p2 = buffer;
		do
			switch (*p1)
			{
				case CC_COLOR:
				case CC_LSPACING:
					p1++;
				case CC_UNDERLINE:
					p1++;
					break;
				default:
					*p2++ = *p1++;
			}
		while (*p1);
		*p2 = 0;

		/* add given string to con_buffer */
		con_add_buffer_line(priority, buffer);

		/* Print output to stdout */
		printf(buffer);

		/* Print output to gamelog.txt */
		if (gamelog_fp)
		{
			struct tm *lt;
			time_t t;
			t=time(NULL);
			lt=localtime(&t);
			PHYSFSX_printf(gamelog_fp,"%02i:%02i:%02i ",lt->tm_hour,lt->tm_min,lt->tm_sec);
#ifdef _WIN32 // stupid hack to force DOS-style newlines
			if (buffer[strlen(buffer)-1] == '\n' && strlen(buffer) <= CON_LINE_LENGTH)
			{
				buffer[strlen(buffer)-1]='\r';
				buffer[strlen(buffer)]='\n';
			}
#endif
			PHYSFSX_printf(gamelog_fp,"%s",buffer);
		}
	}
}

void con_show(void)
{
	int i=0, y;
	static float con_size=0;
	int done=0;

	if (con_render)
	{
		if (con_size < CON_LINES_ONSCREEN && FixedStep & EPS30)
		{
			con_size++;
		}
	}
	else
	{
		if (con_size > 0 && FixedStep & EPS30)
		{
			con_size--;
		}
	}

	if (!con_size)
		return;

	gr_set_current_canvas(NULL);
	gr_set_curfont(GAME_FONT);
	gr_setcolor(0);
	Gr_scanline_darkening_level = 1*7;
	gr_rect(0,0,SWIDTH,(LINE_SPACING*(con_size))+FSPACY(1));
	Gr_scanline_darkening_level = GR_FADE_LEVELS;
	y=FSPACY(1)+(LINE_SPACING*con_size);
	i+=con_scroll_offset;
	while (!done)
	{
		int w,h,aw;

		switch (con_buffer[CON_LINES_MAX-1-i].priority)
		{
			case CON_CRITICAL:
				gr_set_fontcolor(BM_XRGB(28,0,0),-1);
				break;
			case CON_URGENT:
				gr_set_fontcolor(BM_XRGB(54,54,0),-1);
				break;
			case CON_DEBUG:
			case CON_VERBOSE:
				gr_set_fontcolor(BM_XRGB(14,14,14),-1);
				break;
			case CON_HUD:
				gr_set_fontcolor(BM_XRGB(0,28,0),-1);
				break;
			default:
				gr_set_fontcolor(255,-1);
				break;
		}
		gr_get_string_size(con_buffer[CON_LINES_MAX-1-i].line,&w,&h,&aw);
		y-=h+FSPACY(1);
		gr_printf(FSPACX(1),y,"%s",con_buffer[CON_LINES_MAX-1-i].line);
		i++;

		if (y<=0 || CON_LINES_MAX-1-i <= 0 || i < 0)
			done=1;
	}
	gr_setcolor(0);
	gr_rect(0,0,SWIDTH,LINE_SPACING);
	gr_set_fontcolor(255,-1);
	gr_printf(FSPACX(1),FSPACY(1),"%s LOG", DESCENT_VERSION);
	gr_printf(SWIDTH-FSPACX(110),FSPACY(1),"PAGE-UP/DOWN TO SCROLL");
}

int con_events(int key)
{
	switch (key)
	{
		case KEY_PAGEUP:
			con_scroll_offset+=CON_SCROLL_OFFSET;
			if (con_scroll_offset >= CON_LINES_MAX-1)
				con_scroll_offset = CON_LINES_MAX-1;
			while (con_buffer[CON_LINES_MAX-1-con_scroll_offset].line[0]=='\0')
				con_scroll_offset--;
			return 1;
		case KEY_PAGEDOWN:
			con_scroll_offset-=CON_SCROLL_OFFSET;
			if (con_scroll_offset<0)
				con_scroll_offset=0;
			return 1;
		case KEY_SHIFTED + KEY_ESC:
			con_render=!con_render;
			return 1;
	}
	return 0;
}

void con_close(void)
{
	if (gamelog_fp)
		PHYSFS_close(gamelog_fp);
	
	gamelog_fp = NULL;
}

void con_init(void)
{
	memset(con_buffer,0,sizeof(con_buffer));

	gamelog_fp = PHYSFS_openWrite("gamelog.txt");
	atexit(con_close);
}

