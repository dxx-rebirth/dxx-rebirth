/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license below.
 * See COPYING.txt for license details.

THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Autosave system:
 * Saves current mine to disk to prevent loss of work, and support undo.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include <time.h>

#include "dxxerror.h"

#include "inferno.h"
#include "editor.h"
#include "u_mem.h"
#include "ui.h"
#include "strutil.h"

#include "compiler-cf_assert.h"

namespace dcx {

#define AUTOSAVE_PERIOD 5			// Number of minutes for timed autosave

int		Autosave_count;

namespace {

static int Autosave_numfiles;
static int Autosave_total;
static int undo_count;

const char *set_autosave_name(mine_filename_type &out, const std::array<char, PATH_MAX> &in, const unsigned i)
{
	d_strupr(out, in);
	char *ext;
#define DXX_AUTOSAVE_MINE_EXTENSION	".MIN"
	if (!strcmp(out.data(), "*" DXX_AUTOSAVE_MINE_EXTENSION))
	{
#define DXX_DEFAULT_AUTOSAVE_MINE_NAME	"TEMP" DXX_AUTOSAVE_MINE_EXTENSION
		strcpy(out.data(), DXX_DEFAULT_AUTOSAVE_MINE_NAME);
		ext = &out[sizeof(DXX_DEFAULT_AUTOSAVE_MINE_NAME) - sizeof(DXX_AUTOSAVE_MINE_EXTENSION)];
	}
	else
	{
		ext = strstr(out.data(), DXX_AUTOSAVE_MINE_EXTENSION);
		if (!ext)
			return ext;
	}
#undef DXX_AUTOSAVE_MINE_EXTENSION
	cf_assert(i < 10);
	snprintf(ext + 2, 3, "%u", i);
	return ext;
}

static int Timer_save_flag;

}
int		Autosave_flag;

std::array<const char *, 10> undo_status;

void init_autosave(void) {
//    int i;

    Autosave_count = 0;
    Autosave_numfiles = 0;
	 Autosave_flag = 0;
    undo_count = 0;
    autosave_mine(mine_filename);
}

void close_autosave(void) {
	const unsigned t = Autosave_total;
	cf_assert(t < 10);
	for (unsigned i = 0; i < t; ++i)
	{
		mine_filename_type delname;
		if (set_autosave_name(delname, mine_filename, i))
			PHYSFS_delete(delname.data());
    }
}

void autosave_mine(const std::array<char, PATH_MAX> &name)
{
	if (Autosave_flag) {
		mine_filename_type savename;
		if (!set_autosave_name(savename, name, Autosave_count))
			return;
	
	    med_save_mine( savename );
	    Autosave_count++;
	    if (undo_count > 0) undo_count--;
	    if (Autosave_count > 9) Autosave_count -= 10;
	    if (Autosave_numfiles < 10)
	        Autosave_numfiles++;
	    if (Autosave_total < 10)
	        Autosave_total++;
	}
}

tm Editor_time_of_day;

static void print_clock(grs_canvas &canvas, const grs_font &cv_font)
{
	int w, h;
	gr_set_fontcolor(canvas, CBLACK, CGREY);
	std::array<char, 20> message;
	if (!strftime(message.data(), message.size(), "%m-%d %H:%M:%S", &Editor_time_of_day))
		message[0] = 0;
	gr_get_string_size(cv_font, message.data(), &w, &h, nullptr);
	const auto color = CGREY;
	gr_rect(canvas, 700, 0, 799, h + 1, color);
	gr_string(canvas, cv_font, 700, 0, message.data(), w, h);
	gr_set_fontcolor(canvas, CBLACK, CWHITE);
}

void set_editor_time_of_day()
{
	time_t	 ltime;

	time( &ltime );
	Editor_time_of_day = *localtime( &ltime );
}

void TimedAutosave(const std::array<char, PATH_MAX> &name)
{
	auto &canvas = grd_curscreen->sc_canvas;
	gr_set_current_canvas(canvas);
	{
		print_clock(canvas, *canvas.cv_font);
	}
	

#ifndef DEMO
	const auto &minute = Editor_time_of_day.tm_min;
	if (minute%AUTOSAVE_PERIOD != 0)
		Timer_save_flag = 1;

	if ((minute%AUTOSAVE_PERIOD == 0) && (Timer_save_flag)) {
		time_t	 ltime;

		autosave_mine(name);
		Timer_save_flag = 0;
		time( &ltime );
   	diagnostic_message_fmt("Mine Autosaved at %s\n", ctime(&ltime));
	}
#endif

}


int undo( void ) {
	Int3();
	return 2;
}


}
