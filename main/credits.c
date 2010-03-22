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
 *
 * Routines to display the credits.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "error.h"
#include "pstypes.h"
#include "gr.h"
#include "window.h"
#include "key.h"
#include "palette.h"
#include "game.h"
#include "timer.h"
#include "gamefont.h"
#include "pcx.h"
#include "u_mem.h"
#include "screens.h"
#include "digi.h"
#include "cfile.h"
#include "rbaudio.h"
#include "text.h"
#include "songs.h"
#include "menu.h"

#define ROW_SPACING			(SHEIGHT / 17)
#define NUM_LINES			20 //14
#define CREDITS_FILE 			"credits.tex"

typedef struct credits
{
	CFILE * file;
	int have_bin_file;
	char buffer[NUM_LINES][80];
	int buffer_line;
	int first_line_offset;
	int extra_inc;
	int done;
	int row;
	grs_bitmap backdrop;
} credits;

int credits_handler(window *wind, d_event *event, credits *cr)
{
	int j, k, l;
	char * tempp;
	int y;
	
	switch (event->type)
	{
		case EVENT_KEY_COMMAND:
			k = ((d_event_keycommand *)event)->keycode;
			switch (k)
			{
				case KEY_PRINT_SCREEN:
					save_screen_shot(0);
					return 1;
					
				default:
					window_close(wind);
					return 1;
			}
			break;

		case EVENT_IDLE:
			timer_delay(F1_0/25);
			
			//see if redbook song needs to be restarted
			RBACheckFinishedHook();

			if (cr->row == 0)
			{
				do {
					cr->buffer_line = (cr->buffer_line+1) % NUM_LINES;
					if (cfgets( cr->buffer[cr->buffer_line], 80, cr->file ))	{
						char *p;
						if (cr->have_bin_file) // is this a binary tbl file
							decode_text_line (cr->buffer[cr->buffer_line]);
						p = strchr(&cr->buffer[cr->buffer_line][0],'\n');
						if (p) *p = '\0';
					} else	{
						//fseek( file, 0, SEEK_SET);
						cr->buffer[cr->buffer_line][0] = 0;
						cr->done++;
					}
				} while (cr->extra_inc--);
				cr->extra_inc = 0;
			}
			
			if (cr->done>NUM_LINES)
			{
				window_close(wind);
				return 0;
			}
			break;
			
		case EVENT_WINDOW_DRAW:
			y = cr->first_line_offset - cr->row;
			show_fullscr(&cr->backdrop);
			for (j=0; j<NUM_LINES; j++ )	{
				char *s;
				
				l = (cr->buffer_line + j + 1 ) %  NUM_LINES;
				s = cr->buffer[l];
				
				if ( s[0] == '!' ) {
					s++;
				} else if ( s[0] == '$' )	{
					gr_set_curfont( HUGE_FONT );
					s++;
				} else if ( s[0] == '*' )	{
					gr_set_curfont( MEDIUM3_FONT );
					s++;
				} else
					gr_set_curfont( MEDIUM2_FONT );
				
				tempp = strchr( s, '\t' );
				if ( !tempp )	{
					// Wacky Fast Credits thing
					int w, h, aw;
					
					gr_get_string_size( s, &w, &h, &aw);
					gr_printf( 0x8000, y, s );
				}
				y += ROW_SPACING;
			}
			
			cr->row += SHEIGHT/200;
			if (cr->row >= ROW_SPACING)
				cr->row = 0;
			break;

		case EVENT_WINDOW_CLOSE:
			gr_free_bitmap_data (&cr->backdrop);
			cfclose(cr->file);
			songs_play_song( SONG_TITLE, 1 );
			d_free(cr);
			break;
			
		default:
			break;
	}
	
	return 0;
}

//if filename passed is NULL, show normal credits
void credits_show(char *credits_filename)
{
	credits *cr;
	window *wind;
	int i;
	int pcx_error;
	char * tempp;
	char filename[32];
	ubyte backdrop_palette[768];
	
	MALLOC(cr, credits, 1);
	if (!cr)
		return;
	
	cr->have_bin_file = 0;
	cr->buffer_line = 0;
	cr->first_line_offset = 0;
	cr->extra_inc = 0;
	cr->done = 0;
	cr->row = 0;

	// Clear out all tex buffer lines.
	for (i=0; i<NUM_LINES; i++ )
		cr->buffer[i][0] = 0;

	sprintf(filename, "%s", CREDITS_FILE);
	cr->have_bin_file = 0;
	if (credits_filename) {
		strcpy(filename,credits_filename);
		cr->have_bin_file = 1;
	}
	cr->file = cfopen( filename, "rb" );
	if (cr->file == NULL) {
		char nfile[32];
		
		if (credits_filename)
			return;		//ok to not find special filename

		tempp = strchr(filename, '.');
		*tempp = '\0';
		sprintf(nfile, "%s.txb", filename);
		cr->file = cfopen(nfile, "rb");
		if (cr->file == NULL)
			Error("Missing CREDITS.TEX and CREDITS.TXB file\n");
		cr->have_bin_file = 1;
	}

	set_screen_mode(SCREEN_MENU);
	cr->backdrop.bm_data=NULL;

	pcx_error = pcx_read_bitmap(STARS_BACKGROUND,&cr->backdrop, BM_LINEAR,backdrop_palette);
	if (pcx_error != PCX_ERROR_NONE)		{
		cfclose(cr->file);
		return;
	}

	songs_play_song( SONG_CREDITS, 1 );

	gr_remap_bitmap_good( &cr->backdrop,backdrop_palette, -1, -1 );

	gr_set_current_canvas(NULL);
	show_fullscr(&cr->backdrop);
	gr_palette_load( gr_palette );

	key_flush();

	wind = window_create(&grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT, (int (*)(window *, d_event *, void *))credits_handler, cr);
	if (!wind)
	{
		d_event event = { EVENT_WINDOW_CLOSE };
		credits_handler(NULL, &event, cr);
		return;
	}

	while (window_exists(wind))
		event_process();
}
