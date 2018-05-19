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
#include <sys/time.h>
#include <SDL.h>
#include "window.h"
#include "event.h"
#include "console.h"
#include "args.h"
#include "gr.h"
#include "physfsx.h"
#include "gamefont.h"
#include "game.h"
#include "key.h"
#include "vers_id.h"
#include "timer.h"
#include "cli.h"
#include "cvar.h"

#include "dxxsconf.h"
#include "compiler-array.h"

#ifdef _WIN32
#include <windows.h>
#endif

#ifndef DXX_CONSOLE_TIME_SHOW_YMD
#define DXX_CONSOLE_TIME_SHOW_YMD	0
#endif

#ifndef DXX_CONSOLE_TIME_SHOW_MSEC
#define DXX_CONSOLE_TIME_SHOW_MSEC	1
#endif

#ifndef DXX_CONSOLE_SHOW_TIME_STDOUT
#define DXX_CONSOLE_SHOW_TIME_STDOUT	0
#endif

constexpr unsigned CON_LINES_ONSCREEN = 18;
constexpr auto CON_SCROLL_OFFSET = CON_LINES_ONSCREEN - 3;
constexpr unsigned CON_LINES_MAX = 128;

enum con_state {
	CON_STATE_CLOSING = -1,
	CON_STATE_CLOSED = 0,
	CON_STATE_OPENING = 1,
	CON_STATE_OPEN = 2
};

static RAIIPHYSFS_File gamelog_fp;
static array<console_buffer, CON_LINES_MAX> con_buffer;
static con_state con_state;
static int con_scroll_offset, con_size;
static void con_force_puts(con_priority priority, char *buffer, size_t len);

static void con_add_buffer_line(const con_priority priority, const char *const buffer, const size_t len)
{
	/* shift con_buffer for one line */
	std::move(std::next(con_buffer.begin()), con_buffer.end(), con_buffer.begin());
	console_buffer &c = con_buffer.back();
	c.priority=priority;

	size_t copy = std::min(len, CON_LINE_LENGTH - 1);
	c.line[copy] = 0;
	memcpy(&c.line,buffer, copy);
}

void (con_printf)(const con_priority_wrapper priority, const char *const fmt, ...)
{
	va_list arglist;
	char buffer[CON_LINE_LENGTH];

	if (priority <= CGameArg.DbgVerbose)
	{
		va_start (arglist, fmt);
		auto &&leader = priority.insert_location_leader(buffer);
		size_t len = vsnprintf (leader.first, leader.second, fmt, arglist);
		va_end (arglist);
		con_force_puts(priority, buffer, len);
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
				if (!*++p1)
					break;
				/*-fallthrough*/
			case CC_UNDERLINE:
				p1++;
				break;
			default:
				*p2++ = *p1++;
		}
	while (*p1);
	*p2 = 0;
}

static void con_print_file(const char *const buffer)
{
	char buf[1024];
#if !DXX_CONSOLE_SHOW_TIME_STDOUT
#ifndef _WIN32
	/* Print output to stdout */
	puts(buffer);
#endif

	/* Print output to gamelog.txt */
	if (gamelog_fp)
#endif
	{
#if DXX_CONSOLE_TIME_SHOW_YMD
#define DXX_CONSOLE_TIME_FORMAT_YMD	"%04i-%02i-%02i "
#define DXX_CONSOLE_TIME_ARG_YMD	tm_year, tm_month, tm_day,
#else
#define DXX_CONSOLE_TIME_FORMAT_YMD	""
#define DXX_CONSOLE_TIME_ARG_YMD
#endif
#if DXX_CONSOLE_TIME_SHOW_MSEC
#ifdef _WIN32
#define DXX_CONSOLE_TIME_FORMAT_MSEC	".%03i"
#else
#define DXX_CONSOLE_TIME_FORMAT_MSEC	".%06i"
#endif
#define DXX_CONSOLE_TIME_ARG_MSEC	tm_msec,
#else
#define DXX_CONSOLE_TIME_FORMAT_MSEC	""
#define DXX_CONSOLE_TIME_ARG_MSEC
#endif
		int
			DXX_CONSOLE_TIME_ARG_YMD
			DXX_CONSOLE_TIME_ARG_MSEC
			tm_hour, tm_min, tm_sec;
#ifdef _WIN32
#define DXX_LF	"\r\n"
		SYSTEMTIME st = {};
		GetLocalTime(&st);
#if DXX_CONSOLE_TIME_SHOW_YMD
		tm_year = st.wYear;
		tm_month = st.wMonth;
		tm_day = st.wDay;
#endif
		tm_hour = st.wHour;
		tm_min = st.wMinute;
		tm_sec = st.wSecond;
#if DXX_CONSOLE_TIME_SHOW_MSEC
		tm_msec = st.wMilliseconds;
#endif
#else
#define DXX_LF	"\n"
		struct timeval tv;
		if (gettimeofday(&tv, nullptr))
			tv = {};
		if (const auto lt = localtime(&tv.tv_sec))
		{
#if DXX_CONSOLE_TIME_SHOW_YMD
			tm_year = lt->tm_year;
			tm_month = lt->tm_mon;
			tm_day = lt->tm_mday;
#endif
			tm_hour = lt->tm_hour;
			tm_min = lt->tm_min;
			tm_sec = lt->tm_sec;
#if DXX_CONSOLE_TIME_SHOW_MSEC
			tm_msec = tv.tv_usec;
#endif
		}
		else
		{
#if DXX_CONSOLE_TIME_SHOW_YMD
			tm_year = tm_month = tm_day =
#endif
#if DXX_CONSOLE_TIME_SHOW_MSEC
			tm_msec =
#endif
			tm_hour = tm_min = tm_sec = -1;
		}
#endif
		const size_t len = snprintf(buf, sizeof(buf), DXX_CONSOLE_TIME_FORMAT_YMD "%02i:%02i:%02i" DXX_CONSOLE_TIME_FORMAT_MSEC " %s" DXX_LF, DXX_CONSOLE_TIME_ARG_YMD tm_hour, tm_min, tm_sec, DXX_CONSOLE_TIME_ARG_MSEC buffer);
#if DXX_CONSOLE_SHOW_TIME_STDOUT
#ifndef _WIN32
		fputs(buf, stdout);
#endif
		if (gamelog_fp)
#endif
		{
			PHYSFS_write(gamelog_fp, buf, 1, len);
		}
#undef DXX_LF
#undef DXX_CONSOLE_TIME_ARG_MSEC
#undef DXX_CONSOLE_TIME_FORMAT_MSEC
#undef DXX_CONSOLE_TIME_ARG_YMD
#undef DXX_CONSOLE_TIME_FORMAT_YMD
	}
}

/*
 * The caller is assumed to have checked that the priority allows this
 * entry to be logged.
 */
static void con_force_puts(const con_priority priority, char *const buffer, const size_t len)
{
	con_add_buffer_line(priority, buffer, len);
	con_scrub_markup(buffer);
	/* Produce a sanitised version and send it to the console */
	con_print_file(buffer);
}

void con_puts(const con_priority_wrapper priority, char *const buffer, const size_t len)
{
	if (priority <= CGameArg.DbgVerbose)
	{
		typename con_priority_wrapper::scratch_buffer<CON_LINE_LENGTH> scratch_buffer;
		auto &&b = priority.prepare_buffer(scratch_buffer, buffer, len);
		con_force_puts(priority, b.first, b.second);
	}
}

void con_puts(const con_priority_wrapper priority, const char *const buffer, const size_t len)
{
	if (priority <= CGameArg.DbgVerbose)
	{
		typename con_priority_wrapper::scratch_buffer<CON_LINE_LENGTH> scratch_buffer;
		auto &&b = priority.prepare_buffer(scratch_buffer, buffer, len);
		/* add given string to con_buffer */
		con_add_buffer_line(priority, b.first, b.second);
		con_print_file(b.first);
	}
}

static color_t get_console_color_by_priority(int priority)
{
	int r, g, b;
	switch (priority)
	{
		case CON_CRITICAL:
			r = 28 * 2, g = 0 * 2, b = 0 * 2;
			break;
		case CON_URGENT:
			r = 54 * 2, g = 54 * 2, b = 0 * 2;
			break;
		case CON_DEBUG:
		case CON_VERBOSE:
			r = 14 * 2, g = 14 * 2, b = 14 * 2;
			break;
		case CON_HUD:
			r = 0 * 2, g = 28 * 2, b = 0 * 2;
			break;
		default:
			r = 255 * 2, g = 255 * 2, b = 255 * 2;
			break;
	}
	return gr_find_closest_color(r, g, b);
}

static void con_draw(void)
{
	int i = 0, y = 0;

	if (con_size <= 0)
		return;

	gr_set_default_canvas();
	auto &canvas = *grd_curcanv;
	auto &game_font = *GAME_FONT;
	gr_set_curfont(canvas, GAME_FONT);
	const uint8_t color = BM_XRGB(0, 0, 0);
	gr_settransblend(canvas, 7, GR_BLEND_NORMAL);
	const auto &&fspacy1 = FSPACY(1);
	const auto &&line_spacing = LINE_SPACING(*canvas.cv_font, *GAME_FONT);
	y = fspacy1 + (line_spacing * con_size);
	gr_rect(canvas, 0, 0, SWIDTH, y, color);
	gr_settransblend(canvas, GR_FADE_OFF, GR_BLEND_NORMAL);
	i+=con_scroll_offset;

	gr_set_fontcolor(canvas, BM_XRGB(255, 255, 255), -1);
	y = cli_draw(y, line_spacing);

	const auto &&fspacx = FSPACX();
	const auto &&fspacx1 = fspacx(1);
	for (;;)
	{
		auto &b = con_buffer[CON_LINES_MAX - 1 - i];
		gr_set_fontcolor(canvas, get_console_color_by_priority(b.priority), -1);
		int w,h;
		gr_get_string_size(game_font, b.line, &w, &h, nullptr);
		y -= h + fspacy1;
		gr_string(canvas, game_font, fspacx1, y, b.line, w, h);
		i++;

		if (y<=0 || CON_LINES_MAX-1-i <= 0 || i < 0)
			break;
	}
	gr_rect(canvas, 0, 0, SWIDTH, line_spacing, color);
	gr_set_fontcolor(canvas, BM_XRGB(255, 255, 255),-1);
	gr_printf(canvas, game_font, fspacx1, fspacy1, "%s LOG", DESCENT_VERSION);
	gr_string(canvas, game_font, SWIDTH - fspacx(110), fspacy1, "PAGE-UP/DOWN TO SCROLL");
}

static window_event_result con_handler(window *wind,const d_event &event, const unused_window_userdata_t *)
{
	int key;
	static fix64 last_scroll_time = 0;
	
	switch (event.type)
	{
		case EVENT_WINDOW_ACTIVATED:
			key_toggle_repeat(1);
			break;

		case EVENT_WINDOW_DEACTIVATED:
			key_toggle_repeat(0);
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
				case KEY_CTRLED + KEY_A:
				case KEY_HOME:              cli_cursor_home();      break;
				case KEY_END:
				case KEY_CTRLED + KEY_E:    cli_cursor_end();       break;
				case KEY_CTRLED + KEY_C:    cli_clear();            break;
				case KEY_LEFT:              cli_cursor_left();      break;
				case KEY_RIGHT:             cli_cursor_right();     break;
				case KEY_BACKSP:            cli_cursor_backspace(); break;
				case KEY_CTRLED + KEY_D:
				case KEY_DELETE:            cli_cursor_del();       break;
				case KEY_UP:                cli_history_prev();     break;
				case KEY_DOWN:              cli_history_next();     break;
				case KEY_TAB:               cli_autocomplete();     break;
				case KEY_ENTER:             cli_execute();          break;
				case KEY_INSERT:
					cli_toggle_overwrite_mode();
					break;
				default:
					int character = key_ascii();
					if (character == 255)
						break;
					cli_add_character(character);
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
					if (++ con_size >= CON_LINES_ONSCREEN)
						con_state = CON_STATE_OPEN;
				}
			}
			else if (con_state == CON_STATE_CLOSING)
			{
				if (con_size > 0 && timer_query() >= last_scroll_time+(F1_0/30))
				{
					last_scroll_time = timer_query();
					if (! -- con_size)
						con_state = CON_STATE_CLOSED;
				}
			}
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
	game_flush_inputs();
	con_state = CON_STATE_OPENING;
	const auto wind = window_create(grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT, con_handler, unused_window_userdata);
	
	if (!wind)
	{
		d_event event = { EVENT_WINDOW_CLOSE };
		con_handler(NULL, event, NULL);
		return;
	}
}

void con_init(void)
{
	con_buffer = {};
	if (CGameArg.DbgSafelog)
		gamelog_fp.reset(PHYSFS_openWrite("gamelog.txt"));
	else
		gamelog_fp = PHYSFSX_openWriteBuffered("gamelog.txt");

	cli_init();
	cmd_init();
	cvar_init();

}
