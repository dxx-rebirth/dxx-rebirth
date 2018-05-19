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

namespace dcx {

#define AUTOSAVE_PERIOD 5			// Number of minutes for timed autosave

int		Autosave_count;
static int Autosave_numfiles;
static int Autosave_total;
static int undo_count;

static int Timer_save_flag;
int		Autosave_flag;

array<const char *, 10> undo_status;

void init_autosave(void) {
//    int i;

    Autosave_count = 0;
    Autosave_numfiles = 0;
	 Autosave_flag = 0;
    undo_count = 0;
    autosave_mine(mine_filename);
}

void close_autosave(void) {
    char *ext;

    for (int i=0;i<Autosave_total;i++) {

		char delname[PATH_MAX];

        strcpy ( delname, mine_filename );
        d_strupr( delname );
	if ( !strcmp(delname, "*.MIN") ) strcpy(delname, "TEMP.MIN");

        ext = strstr(delname, ".MIN");
        snprintf(ext + 2, 3, "%d", i);

        remove( delname );
    }
}

void autosave_mine(const char *name) {
    char *ext;

	if (Autosave_flag) {
	
		char savename[PATH_MAX];

	
	    strcpy ( savename, name );
	    d_strupr( savename );
	    if ( !strcmp(savename, "*.MIN") ) strcpy(savename, "TEMP.MIN");
	
	    ext = strstr(savename, ".MIN");
	    snprintf(ext + 2, 3, "%d", Autosave_count);
	
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

static void print_clock()
{
	int w, h;
	gr_set_default_canvas();
	auto &canvas = *grd_curcanv;
	gr_set_fontcolor(canvas, CBLACK, CGREY);
	array<char, 20> message;
	if (!strftime(message.data(), message.size(), "%m-%d %H:%M:%S", &Editor_time_of_day))
		message[0] = 0;
	gr_get_string_size(*canvas.cv_font, message.data(), &w, &h, nullptr);
	const uint8_t color = CGREY;
	gr_rect(canvas, 700, 0, 799, h + 1, color);
	gr_string(canvas, *canvas.cv_font, 700, 0, message.data(), w, h);
	gr_set_fontcolor(canvas, CBLACK, CWHITE);
}

void set_editor_time_of_day()
{
	time_t	 ltime;

	time( &ltime );
	Editor_time_of_day = *localtime( &ltime );
}

void TimedAutosave(const char *name) 
{
	{
		print_clock();
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
