/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 *
 *  Based on an early version of SDL_Console
 *  Written By: Garrett Banuk <mongoose@mongeese.org>
 *  Code Cleanup and heavily extended by: Clemens Wacha <reflex-2000@gmx.net>
 *  Ported to use native Descent interfaces by: Bradley Bell <btb@icculus.org>
 *
 *  This is free, just be sure to give us credit when using it
 *  in any of your programs.
 *
 */
/*
 *
 * Command-line interface for the console
 *
 */

#pragma once

#include <cstdint>

enum class CLI_insert_type : uint8_t
{
	insert,
	overwrite,
};

// Insert or Overwrite characters?
void cli_toggle_overwrite_mode();

void cli_init(void);
/* executes the command typed in at the console (called if you press ENTER)*/
void cli_execute();
/* Gets called when TAB was pressed */
void cli_autocomplete(void);
/* draws the commandline the user is typing in to the screen. called by update? */
unsigned cli_draw(unsigned y, unsigned line_spacing);
/* Gets called if you press the LEFT key (move cursor left) */
void cli_cursor_left(void);
/* Gets called if you press the RIGHT key (move cursor right) */
void cli_cursor_right(void);
/* Gets called if you press the HOME key (move cursor to the beginning of the line */
void cli_cursor_home(void);
/* Gets called if you press the END key (move cursor to the end of the line*/
void cli_cursor_end(void);
/* Called if you press DELETE (deletes character under the cursor) */
void cli_cursor_del(void);
/* Called if you press BACKSPACE (deletes character left of cursor) */
void cli_cursor_backspace(void);
/* Called if you type in a character (add the char to the command) */
void cli_add_character(char character);
/* Called if you press Ctrl-C (deletes the commandline) */
void cli_clear(void);
/* Called if you press UP key (switches through recent typed in commands */
void cli_history_prev(void);
/* Called if you press DOWN key (switches through recent typed in commands */
void cli_history_next(void);
