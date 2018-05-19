/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 *
 * --
 *  Based on an early version of SDL_Console
 *  Written By: Garrett Banuk <mongoose@mongeese.org>
 *  Code Cleanup and heavily extended by: Clemens Wacha <reflex-2000@gmx.net>
 *  Ported to use native Descent interfaces by: Bradley Bell <btb@icculus.org>
 *
 *  This is free, just be sure to give us credit when using it
 *  in any of your programs.
 * --
 *
 * Rewritten to use C++ utilities by Kp.  Post-Bradley work is under
 * the standard Rebirth terms, which are less permissive than the
 * statement above.
 */
/*
 *
 * Command-line interface for the console
 *
 */

#include <algorithm>
#include <cassert>
#include <cctype>
#include <deque>
#include <string>

#include "gr.h"
#include "gamefont.h"
#include "console.h"
#include "cli.h"
#include "compiler-poison.h"

// Cursor shown if we are in insert mode
#define CLI_INS_CURSOR          "_"
// Cursor shown if we are in overwrite mode
#define CLI_OVR_CURSOR          "|"

namespace {

class CLIState
{
	static const char g_prompt_mode_cmd = ']';
	static const char g_prompt_strings[];
	/* When drawing an underscore as a cursor indicator, shift it down by
	 * this many pixels, to make it easier to see when the underlined
	 * character is itself an underscore.
	 */
	static const unsigned m_cursor_underline_y_shift = 3;
	static const unsigned m_maximum_history_lines = 100;
	unsigned m_history_position, m_line_position;
	std::string m_line;
	std::deque<std::string> m_lines;
	CLI_insert_type m_insert_type;
	void history_move(unsigned position);
public:
	void init();
	unsigned draw(unsigned, unsigned);
	void execute_active_line();
	void insert_completion();
	void cursor_left();
	void cursor_right();
	void cursor_home();
	void cursor_end();
	void cursor_del();
	void cursor_backspace();
	void add_character(char c);
	void clear_active_line();
	void history_prev();
	void history_next();
	void toggle_overwrite_mode();
};

const char CLIState::g_prompt_strings[] = {
	g_prompt_mode_cmd,
};

}

static CLIState g_cli;

/* Initializes the cli */
void cli_init()
{
	g_cli.init();
}

/* Draws the command line the user is typing in to the screen */
unsigned cli_draw(unsigned y, unsigned line_spacing)
{
	return g_cli.draw(y, line_spacing);
}

/* Executes the command entered */
void cli_execute()
{
	g_cli.execute_active_line();
}

void cli_autocomplete(void)
{
	g_cli.insert_completion();
}

void cli_cursor_left()
{
	g_cli.cursor_left();
}

void cli_cursor_right()
{
	g_cli.cursor_right();
}

void cli_cursor_home()
{
	g_cli.cursor_home();
}

void cli_cursor_end()
{
	g_cli.cursor_end();
}

void cli_cursor_del()
{
	g_cli.cursor_del();
}

void cli_cursor_backspace()
{
	g_cli.cursor_backspace();
}

void cli_add_character(char character)
{
	g_cli.add_character(character);
}

void cli_clear()
{
	g_cli.clear_active_line();
}

void cli_history_prev()
{
	g_cli.history_prev();
}

void cli_history_next()
{
	g_cli.history_next();
}

void cli_toggle_overwrite_mode()
{
	g_cli.toggle_overwrite_mode();
}

void CLIState::init()
{
	m_lines.emplace_front();
}

unsigned CLIState::draw(unsigned y, unsigned line_spacing)
{
	using wrap_result = std::pair<const char *, unsigned>;
	/* At most this many lines of wrapped input can be shown at once.
	 * Any excess lines will be hidden.
	 *
	 * Use a power of 2 to make the modulus optimize into a fast masking
	 * operation.
	 *
	 * Zero-initialize for safety, but also mark it as initially
	 * undefined for Valgrind.  Assuming no bugs, any element of wraps[]
	 * accessed by the second loop will have been initialized by the
	 * first loop.
	 */
	std::array<wrap_result, 8> wraps{};
	DXX_MAKE_VAR_UNDEFINED(wraps);
	const auto margin_width = FSPACX(1);
	const char prompt_string[2] = {g_prompt_strings[0], 0};
	int prompt_width, h;
	gr_get_string_size(*grd_curcanv->cv_font, prompt_string, &prompt_width, &h, nullptr);
	y -= line_spacing;
	const auto canvas_width = grd_curcanv->cv_bitmap.bm_w;
	const unsigned max_pixels_per_line = canvas_width - (margin_width * 2) - prompt_width;
	const unsigned unknown_cursor_line = ~0u;
	const auto line_position = m_line_position;
	const auto line_begin = m_line.c_str();
	std::size_t last_wrap_line = 0;
	unsigned cursor_line = unknown_cursor_line;
	/* Search the text and initialize wraps[] to record where line
	 * breaks will appear.  If the wrapped text is more than
	 * wraps.size() vertical lines, only the most recent wraps.size()
	 * lines are saved and shown.
	 */
	for (const char *p = line_begin;; ++last_wrap_line)
	{
		auto &w = wraps[last_wrap_line % wraps.size()];
		w = gr_get_string_wrap(*grd_curcanv->cv_font, p, max_pixels_per_line);
		/* Record the vertical line on which the cursor will appear as
		 * `cursor_line`.
		 */
		if (cursor_line == unknown_cursor_line)
		{
			const auto unseen_position = w.first - p;
			if (line_position < unseen_position)
				cursor_line = last_wrap_line;
		}
		/* If more text exists than can be shown, then stop at
		 * (wraps.size() / 2) lines past the cursor line.
		 */
		else if (last_wrap_line >= wraps.size() && cursor_line + (wraps.size() / 2) < last_wrap_line)
			break;
		p = w.first;
		if (!*p)
			break;
	}
	const auto line_left = margin_width + prompt_width + 1;
	const auto cursor_string = (m_insert_type == CLI_insert_type::insert ? CLI_INS_CURSOR : CLI_OVR_CURSOR);
	int cursor_width, cursor_height;
	gr_get_string_size(*grd_curcanv->cv_font, cursor_string, &cursor_width, &cursor_height, nullptr);
	if (line_position == m_line.size())
	{
		const auto &w = wraps[last_wrap_line % wraps.size()];
		if (cursor_width + line_left + w.second > max_pixels_per_line)
		{
			auto &w2 = wraps[++last_wrap_line % wraps.size()];
			w2 = {w.first, 0};
			assert(!*w2.first);
		}
		cursor_line = last_wrap_line;
	}
	for (unsigned i = std::min(last_wrap_line + 1, wraps.size());; --last_wrap_line)
	{
		const auto &w = wraps[last_wrap_line % wraps.size()];
		const auto p = w.first;
		if (!p)
		{
			assert(p);
			break;
		}
		std::string::const_pointer q;
		if (last_wrap_line)
		{
			q = wraps[(last_wrap_line - 1) % wraps.size()].first;
			if (!q)
			{
				assert(q);
				break;
			}
		}
		else
			q = line_begin;
		std::string::pointer mc;
		std::string::value_type c;
		/* If the parsing loop exited by the cursor_line test, then this
		 * test is true on every pass through this loop.
		 *
		 * If the parsing loop exited by !*p, then this test is false on
		 * the first pass through this loop and true on every other
		 * pass.
		 *
		 * If the input text requires only one vertical line, then the
		 * parsing loop will have exited through the !*p test and this
		 * loop will only run iteration.
		 */
		if (*p)
		{
			/* Temporarily write a null into the text string for the
			 * benefit of null-terminator based code in the gr_string*
			 * functions.  The original character is saved in `c` and
			 * will be restored later.
			 */
			mc = &m_line[p - q];
			c = *mc;
			*mc = 0;
		}
		else
		{
			/* No need to write to the std::string because a
			 * null-terminator is already present.
			 */
			mc = nullptr;
			c = 0;
		}
		gr_string(*grd_curcanv, *grd_curcanv->cv_font, line_left, y, q, w.second, h);
		if (--i == cursor_line)
		{
			unsigned cx = line_left + w.second, cy = y;
			if (m_insert_type == CLI_insert_type::insert)
				cy += m_cursor_underline_y_shift;
			if (line_position != p - line_begin)
			{
				int cw;
				gr_get_string_size(*grd_curcanv->cv_font, &line_begin[line_position], &cw, nullptr, nullptr);
				cx -= cw;
			}
			gr_string(*grd_curcanv, *grd_curcanv->cv_font, cx, cy, cursor_string, cursor_width, cursor_height);
		}
		/* Restore the original character, if one was overwritten. */
		if (mc)
			*mc = c;
		if (!i)
			break;
		y -= h;
	}
	gr_string(*grd_curcanv, *grd_curcanv->cv_font, margin_width, y, prompt_string, prompt_width, h);
	return y;
}

void CLIState::execute_active_line()
{
	if (m_line.empty())
		return;
	const char *p = m_line.c_str();
	con_printf(CON_NORMAL, "con%c%s", g_prompt_strings[0], p);
	cmd_append(p);
	m_lines[0] = move(m_line);
	m_lines.emplace_front();
	m_history_position = 0;
	if (m_lines.size() > m_maximum_history_lines)
		m_lines.pop_back();
	clear_active_line();
}

void CLIState::insert_completion()
{
	const auto suggestion = cmd_complete(m_line.c_str());
	if (!suggestion)
		return;
	m_line = suggestion;
	m_line += " ";
	m_line_position = m_line.size();
}

void CLIState::cursor_left()
{
	if (m_line_position > 0)
		-- m_line_position;
}

void CLIState::cursor_right()
{
	if (m_line_position < m_line.size())
		++ m_line_position;
}

void CLIState::cursor_home()
{
	m_line_position = 0;
}

void CLIState::cursor_end()
{
	m_line_position = m_line.size();
}

void CLIState::cursor_del()
{
	const auto l = m_line_position;
	if (l >= m_line.size())
		return;
	m_line.erase(next(m_line.begin(), l));
}

void CLIState::cursor_backspace()
{
	if (m_line_position <= 0)
		return;
	m_line.erase(next(m_line.begin(), --m_line_position));
}

void CLIState::add_character(char c)
{
	if (m_insert_type == CLI_insert_type::overwrite && m_line_position < m_line.size())
		m_line[m_line_position] = c;
	else
		m_line.insert(next(m_line.begin(), m_line_position), c);
	++m_line_position;
}

void CLIState::clear_active_line()
{
	m_line_position = 0;
	m_line.clear();
}

void CLIState::history_move(unsigned position)
{
	if (position >= m_lines.size())
		return;
	m_lines[m_history_position] = move(m_line);
	auto &l = m_lines[m_history_position = position];
	m_line_position = l.size();
	m_line = l;
}

void CLIState::history_prev()
{
	history_move(m_history_position + 1);
}

void CLIState::history_next()
{
	const auto max_lines = m_lines.size();
	if (m_history_position > max_lines)
		m_history_position = max_lines;
	history_move(m_history_position - 1);
}

void CLIState::toggle_overwrite_mode()
{
	m_insert_type = m_insert_type == CLI_insert_type::insert
		? CLI_insert_type::overwrite
		: CLI_insert_type::insert;
}
