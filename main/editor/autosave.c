/*
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
 * $Source: /cvs/cvsroot/d2x/main/editor/autosave.c,v $
 * $Revision: 1.1 $
 * $Author: btb $
 * $Date: 2004-12-19 13:54:27 $
 * 
 * Autosave system: 
 * Saves current mine to disk to prevent loss of work, and support undo.
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  1999/06/14 22:02:47  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:34:53  john
 * Version 2.0! No anonymous unions, Watcom 10.0, with no need
 * for bitmaps.tbl.
 * 
 * Revision 1.25  1994/11/19  00:04:40  john
 * Changed some shorts to ints.
 * 
 * Revision 1.24  1994/11/17  11:38:59  matt
 * Ripped out code to load old mines
 * 
 * Revision 1.23  1994/07/28  17:00:01  mike
 * fix diagnostic_message erasing.
 * 
 * Revision 1.22  1994/07/21  12:48:28  mike
 * Make time of day a global, fix clock so it doesn't show 10:2 instead of 10:02
 * 
 * Revision 1.21  1994/05/14  17:17:58  matt
 * Got rid of externs in source (non-header) files
 * 
 * Revision 1.20  1994/05/02  18:04:14  yuan
 * Fixed warning.
 * 
 * Revision 1.19  1994/05/02  17:59:04  yuan
 * Changed undo_status into an array rather than malloced pointers.
 * 
 * Revision 1.18  1994/03/16  09:55:48  mike
 * Flashing : in time.
 * 
 * Revision 1.17  1994/02/11  10:27:36  matt
 * Changed 'if !DEMO' to 'ifndef DEMO'
 * 
 * Revision 1.16  1994/02/08  12:43:18  yuan
 * Crippled save game function from demo version
 * 
 * Revision 1.15  1994/02/01  13:27:26  yuan
 * autosave default off.
 * 
 * Revision 1.14  1994/01/05  09:57:37  yuan
 * Fixed calendar/clock problem.
 * 
 * Revision 1.13  1993/12/17  16:09:59  yuan
 * Changed clock font from Red to Black.
 * 
 * Revision 1.12  1993/12/15  13:08:38  yuan
 * Fixed :0x times, so that the 0 shows up.
 * 
 * Revision 1.11  1993/12/15  11:19:52  yuan
 * Added code to display clock in upper right.
 * 
 * Revision 1.10  1993/12/14  21:18:51  yuan
 * Added diagnostic message to display
 * 
 * Revision 1.9  1993/12/14  18:32:59  yuan
 * Added timed autosave code
 * 
 * Revision 1.8  1993/12/13  17:23:25  yuan
 * Fixed bugs with undo.
 * They were caused by badly changed extensions.
 * 
 * Revision 1.7  1993/12/09  16:42:32  yuan
 * Changed extension of temp mines from .mi? -> .mn? 
 * and now to .m? (So it doesn't interfere with .mnu)
 * 
 * Revision 1.6  1993/12/09  16:27:06  yuan
 * Added toggle for autosave
 * 
 * Revision 1.5  1993/11/29  19:46:32  matt
 * Changed includes
 * 
 * Revision 1.4  1993/11/11  15:54:11  yuan
 * Added display message for Undo...
 * Eg. Attach Segment UNDONE.
 * 
 * Revision 1.3  1993/11/09  18:53:11  yuan
 * Autosave/Undo works up to 10 moves.
 * 
 * Revision 1.2  1993/11/08  19:14:03  yuan
 * Added Undo command (not working yet)
 * 
 * Revision 1.1  1993/11/08  16:57:59  yuan
 * Initial revision
 * 
 * 
 */


#ifdef RCS
static char rcsid[] = "$Id: autosave.c,v 1.1 2004-12-19 13:54:27 btb Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#ifndef __LINUX__
#include <process.h>
#endif
#include <time.h>

#include "error.h"

#include "inferno.h"
#include "editor.h"
#include "mono.h"
#include "u_mem.h"
#include "ui.h"

#define AUTOSAVE_PERIOD 5			// Number of minutes for timed autosave

int		Autosave_count;
int		Autosave_numfiles;
int		Autosave_total;
int		undo_count;
int		original;

int		Timer_save_flag=0;
int		Autosave_flag;
int		save_second=-1;

char		undo_status[10][100];

void init_autosave(void) {
//    int i;

    Autosave_count = 0;
    Autosave_numfiles = 0;
	 Autosave_flag = 0;
    undo_count = 0;
    //MALLOC( undo_status, char *, 10 );
    //for (i=0; i<10; i++)
    //    MALLOC( undo_status[i], char, 100 );
    autosave_mine(mine_filename);
}

void close_autosave(void) {
    int i;
    char *delname, *ext;

    for (i=0;i<Autosave_total;i++) {

	MALLOC(delname, char, 128);

        strcpy ( delname, mine_filename );
        strupr( delname );
	if ( !strcmp(delname, "*.MIN") ) strcpy(delname, "TEMP.MIN");

        ext = strstr(delname, ".MIN");
        sprintf( ext, ".M%d", i );

        remove( delname );
        free( delname );
    }
    //for (i=0;i<10;i++) free( undo_status[i] );
    //free( undo_status );

}

void autosave_mine(char *name) {
    char *savename, *ext;

	if (Autosave_flag) {
	
	    MALLOC(savename, char, 128);

	
	    strcpy ( savename, name );
	    strupr( savename );
	    if ( !strcmp(savename, "*.MIN") ) strcpy(savename, "TEMP.MIN");
	
	    ext = strstr(savename, ".MIN");
	    sprintf( ext, ".M%d", Autosave_count );
	
	    //mprintf( 0, "Autosave: %s\n", savename );
	    med_save_mine( savename );
	    Autosave_count++;
	    if (undo_count > 0) undo_count--;
	    if (Autosave_count > 9) Autosave_count -= 10;
	    if (Autosave_numfiles < 10)
	        Autosave_numfiles++;
	    if (Autosave_total < 10)
	        Autosave_total++;
	
	    free(savename);

	}

}


void print_clock( int seconds, char message[10] ) {
	int w,h,aw;
	char	*p;

	//	Make colon flash
	if (seconds & 1)
		if ((p = strchr(message, ':')) != NULL)
			*p = ' ';

	gr_set_current_canvas( NULL );
	gr_set_fontcolor( CBLACK, CGREY );
	gr_get_string_size( message, &w, &h, &aw );
	gr_setcolor( CGREY );
	gr_rect( 700, 0, 799, h+1 );
	gr_string( 700, 0, message );
	gr_set_fontcolor( CBLACK, CWHITE );
}

static char the_time[14];	// changed from 10, I don't think that was long enough

void clock_message( int seconds, char *format, ... ) {
	va_list ap;

	va_start(ap, format);
  	vsprintf(the_time, format, ap);
	va_end(ap);

  	print_clock(seconds, the_time);
}


struct tm Editor_time_of_day;

void set_editor_time_of_day()
{
	time_t	 ltime;

	time( &ltime );
	Editor_time_of_day = *localtime( &ltime );
}

void TimedAutosave(char *name) 
{
	int 		 month,day,hour,minute,second;

	month = (Editor_time_of_day.tm_mon) + 1;
	day =	Editor_time_of_day.tm_mday;
	minute = Editor_time_of_day.tm_min;
	hour = Editor_time_of_day.tm_hour;
	second = Editor_time_of_day.tm_sec;

	if (hour > 12) hour-=12;

	if (second!=save_second) {
		save_second = second;
		clock_message(second, "%d/%d %d:%02d", month, day, hour, minute);
	}
	

#ifndef DEMO
	if (minute%AUTOSAVE_PERIOD != 0)
		Timer_save_flag = 1;

	if ((minute%AUTOSAVE_PERIOD == 0) && (Timer_save_flag)) {
		time_t	 ltime;

		autosave_mine(name);
		Timer_save_flag = 0;
		time( &ltime );
   	diagnostic_message("Mine Autosaved at %s\n", ctime(&ltime));
	}
#endif

}


int undo( void ) {
	Int3();
	return 2;

//@@    char *loadname, *ext;
//@@    if (undo_count == 0) original = Autosave_count;
//@@
//@@	 if (!Autosave_flag) 
//@@		 return 2;
//@@
//@@    if (Autosave_numfiles > 1) {
//@@
//@@        MALLOC(loadname, char, 128);
//@@
//@@        strcpy ( loadname, mine_filename );
//@@        strupr( loadname );
//@@        if ( !strcmp(loadname, "*.MIN") ) strcpy(loadname, "TEMP.MIN");
//@@
//@@        undo_count++;
//@@        Autosave_count = original - undo_count;
//@@        if (Autosave_count < 0) Autosave_count = Autosave_count+10;
//@@        Autosave_numfiles--;
//@@        //mprintf(0, "u=%d  a=%d  o=%d  num=%d\n", undo_count, Autosave_count, original, Autosave_numfiles);
//@@
//@@        ext = strstr(loadname, ".MIN");
//@@        if (Autosave_count == 0) sprintf( ext, ".M9" );
//@@            else sprintf( ext, ".M%d", Autosave_count-1 );
//@@        //mprintf( 0, "Loading: %s\n", loadname );
//@@        med_load_mine( loadname );
//@@
//@@        free(loadname);
//@@        return 0;
//@@     }
//@@     else return 1;
//@@     //diagnostic_message("Can't undo\n");

}

