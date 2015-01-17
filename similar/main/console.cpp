/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 *
 * Game console
 *
 */

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <SDL.h>
#include "window.h"
#include "console.h"
#include "args.h"
#include "gr.h"
#include "physfsx.h"
#include "gamefont.h"
#include "game.h"
#include "key.h"
#include "vers_id.h"
#include "timer.h"

#include "dxxsconf.h"
#include "compiler-array.h"

static PHYSFS_file *gamelog_fp=NULL;
static array<console_buffer, CON_LINES_MAX> con_buffer;
static int con_state = CON_STATE_CLOSED, con_scroll_offset = 0, con_size = 0;

static void con_add_buffer_line(int priority, const char *buffer, size_t len)
{
	/* shift con_buffer for one line */
	std::move(std::next(con_buffer.begin()), con_buffer.end(), con_buffer.begin());
	console_buffer &c = con_buffer.back();
	c.priority=priority;

	size_t copy = std::min(len, CON_LINE_LENGTH - 1);
	memcpy(&c.line,buffer, copy);
	c.line[copy] = 0;
}

void (con_printf)(int priority, const char *fmt, ...)
{
	va_list arglist;
	char buffer[CON_LINE_LENGTH];

	if (priority <= ((int)GameArg.DbgVerbose))
	{
		va_start (arglist, fmt);
		size_t len = vsnprintf (buffer, sizeof(buffer), fmt, arglist);
		va_end (arglist);
		con_puts(priority, buffer, len);
	}
}

static void con_scrub_markup(char *buffer)
{
	char *p1 = buffer, *p2 = p1;
	do
		switch (*p1)
		{
			case CC_COLOR:
			case CC_LSPACING:
				if (!*p1++)
					break;
			case CC_UNDERLINE:
				p1++;
				break;
			default:
				*p2++ = *p1++;
		}
	while (*p1);
	*p2 = 0;
}

static void con_print_file(const char *buffer)
{
	/* Print output to stdout */
	puts(buffer);

	/* Print output to gamelog.txt */
	if (gamelog_fp)
	{
		struct tm *lt;
		time_t t;
		t=time(NULL);
		lt=localtime(&t);
		PHYSFSX_printf(gamelog_fp,"%02i:%02i:%02i ",lt->tm_hour,lt->tm_min,lt->tm_sec);
#ifdef _WIN32 // stupid hack to force DOS-style newlines
#define DXX_LF	"\r\n"
#else
#define DXX_LF	"\n"
#endif
		PHYSFSX_printf(gamelog_fp,"%s" DXX_LF,buffer);
	}
}

void con_puts(int priority, char *buffer, size_t len)
{
	if (priority <= ((int)GameArg.DbgVerbose))
	{
		con_add_buffer_line(priority, buffer, len);
		con_scrub_markup(buffer);
		/* Produce a sanitised version and send it to the console */
		con_print_file(buffer);
	}
}

void con_puts(int priority, const char *buffer, size_t len)
{
	if (priority <= ((int)GameArg.DbgVerbose))
	{
		/* add given string to con_buffer */
		con_add_buffer_line(priority, buffer, len);
		con_print_file(buffer);
	}
}

static void con_draw(void)
{
	int i = 0, y = 0, done = 0;

	if (con_size <= 0)
		return;

	gr_set_current_canvas(NULL);
	gr_set_curfont(GAME_FONT);
	gr_setcolor(BM_XRGB(0,0,0));
	gr_settransblend(7, GR_BLEND_NORMAL);
	gr_rect(0,0,SWIDTH,(LINE_SPACING*(con_size))+FSPACY(1));
	gr_settransblend(GR_FADE_OFF, GR_BLEND_NORMAL);
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
				gr_set_fontcolor(BM_XRGB(255,255,255),-1);
				break;
		}
		gr_get_string_size(con_buffer[CON_LINES_MAX-1-i].line,&w,&h,&aw);
		y-=h+FSPACY(1);
		gr_string(FSPACX(1),y,con_buffer[CON_LINES_MAX-1-i].line);
		i++;

		if (y<=0 || CON_LINES_MAX-1-i <= 0 || i < 0)
			done=1;
	}
	gr_setcolor(BM_XRGB(0,0,0));
	gr_rect(0,0,SWIDTH,LINE_SPACING);
	gr_set_fontcolor(BM_XRGB(255,255,255),-1);
	gr_printf(FSPACX(1),FSPACY(1),"%s LOG", DESCENT_VERSION);
	gr_string(SWIDTH-FSPACX(110),FSPACY(1),"PAGE-UP/DOWN TO SCROLL");
}

static window_event_result con_handler(window *wind,const d_event &event, const unused_window_userdata_t *)
{
	int key;
	static fix64 last_scroll_time = 0;
	
	switch (event.type)
	{
		case EVENT_WINDOW_ACTIVATED:
			break;

		case EVENT_WINDOW_DEACTIVATED:
			con_size = 0;
			con_state = CON_STATE_CLOSED;
			break;

		case EVENT_KEY_COMMAND:
			key = event_key_get(event);
			switch (key)
			{
				case KEY_SHIFTED + KEY_ESC:
					switch (con_state)
					{
						case CON_STATE_OPEN:
						case CON_STATE_OPENING:
							con_state = CON_STATE_CLOSING;
							break;
						case CON_STATE_CLOSED:
						case CON_STATE_CLOSING:
							con_state = CON_STATE_OPENING;
						default:
							break;
					}
					break;
				case KEY_PAGEUP:
					con_scroll_offset+=CON_SCROLL_OFFSET;
					if (con_scroll_offset >= CON_LINES_MAX-1)
						con_scroll_offset = CON_LINES_MAX-1;
					while (con_buffer[CON_LINES_MAX-1-con_scroll_offset].line[0]=='\0')
						con_scroll_offset--;
					break;
				case KEY_PAGEDOWN:
					con_scroll_offset-=CON_SCROLL_OFFSET;
					if (con_scroll_offset<0)
						con_scroll_offset=0;
					break;
				default:
					break;
			}
			return window_event_result::handled;

		case EVENT_WINDOW_DRAW:
			timer_delay2(50);
			if (con_state == CON_STATE_OPENING)
			{
				if (con_size < CON_LINES_ONSCREEN && timer_query() >= last_scroll_time+(F1_0/30))
				{
					last_scroll_time = timer_query();
					con_size++;
				}
			}
			else if (con_state == CON_STATE_CLOSING)
			{
				if (con_size > 0 && timer_query() >= last_scroll_time+(F1_0/30))
				{
					last_scroll_time = timer_query();
					con_size--;
				}
			}
			if (con_size >= CON_LINES_ONSCREEN)
				con_state = CON_STATE_OPEN;
			else if (con_size <= 0)
				con_state = CON_STATE_CLOSED;
			con_draw();
			if (con_state == CON_STATE_CLOSED && wind)
			{
				return window_event_result::close;
			}
			break;
		case EVENT_WINDOW_CLOSE:
			break;
		default:
			break;
	}
	
	return window_event_result::ignored;
}

void con_showup(void)
{
	window *wind;

	game_flush_inputs();
	con_state = CON_STATE_OPENING;
	wind = window_create(&grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT, con_handler, unused_window_userdata);
	
	if (!wind)
	{
		d_event event = { EVENT_WINDOW_CLOSE };
		con_handler(NULL, event, NULL);
		return;
	}
}

static void con_close(void)
{
	if (gamelog_fp)
		PHYSFS_close(gamelog_fp);
	
	gamelog_fp = NULL;
}

void con_init(void)
{
	con_buffer = {};
	if (GameArg.DbgSafelog)
		gamelog_fp = PHYSFS_openWrite("gamelog.txt");
	else
		gamelog_fp = PHYSFSX_openWriteBuffered("gamelog.txt");
	atexit(con_close);
}

