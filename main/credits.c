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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
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

#include "dxxerror.h"
#include "pstypes.h"
#include "gr.h"
#include "window.h"
#include "key.h"
#include "mouse.h"
#include "palette.h"
#include "game.h"
#include "gamepal.h"
#include "timer.h"
#include "gamefont.h"
#include "pcx.h"
#include "u_mem.h"
#include "screens.h"
#include "digi.h"
#include "rbaudio.h"
#include "text.h"
#include "songs.h"
#include "menu.h"
#include "mission.h"
#include "config.h"
#include "args.h"

#define ROW_SPACING			(SHEIGHT / 17)
#define NUM_LINES			20 //14
#define CREDITS_FILE    		(PHYSFSX_exists("mcredits.tex",1)?"mcredits.tex":PHYSFSX_exists("ocredits.tex",1)?"ocredits.tex":"credits.tex")
#define ALLOWED_CHAR			( Current_mission==NULL ? 'R' : (is_SHAREWARE ? 'S' : 'R'))

typedef struct credits
{
	PHYSFS_file * file;
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
	int j, l, y;
	char * tempp;
	
	switch (event->type)
	{
		case EVENT_KEY_COMMAND:
			if (!call_default_handler(event))	// if not print screen, debug etc
				window_close(wind);
			return 1;

		case EVENT_MOUSE_BUTTON_DOWN:
		case EVENT_MOUSE_BUTTON_UP:
			if (event_mouse_get_button(event) == MBTN_LEFT || event_mouse_get_button(event) == MBTN_RIGHT)
			{
				window_close(wind);
				return 1;
			}
			break;

		case EVENT_IDLE:
			if (cr->done>NUM_LINES)
			{
				window_close(wind);
				return 0;
			}
			break;
			
		case EVENT_WINDOW_DRAW:
			timer_delay(F1_0/28);
			
			if (cr->row == 0)
			{
				do {
					cr->buffer_line = (cr->buffer_line+1) % NUM_LINES;
				get_line:;
					if (PHYSFSX_fgets( cr->buffer[cr->buffer_line], 80, cr->file ))	{
						char *p;
						if (cr->have_bin_file) // is this a binary tbl file
							decode_text_line (cr->buffer[cr->buffer_line]);
						p = cr->buffer[cr->buffer_line];
						if (p[0] == ';')
							goto get_line;
						
						if (p[0] == '%')
						{
							if (p[1] == ALLOWED_CHAR)
							{
								int i = 0, len = strlen(p);
								for (i = 0; i < len; i++)
									p[i] = p[i+2];
							}
							else
								goto get_line;
						}
						
					} else	{
						//fseek( file, 0, SEEK_SET);
						cr->buffer[cr->buffer_line][0] = 0;
						cr->done++;
					}
				} while (cr->extra_inc--);
				cr->extra_inc = 0;
			}

			// cheap but effective: towards end of credits sequence, fade out the music volume
			if (cr->done >= NUM_LINES-16)
			{
				static int curvol = -10; 
				if (curvol == -10) 
					curvol = GameCfg.MusicVolume;
				if (curvol > (NUM_LINES-cr->done)/2)
				{
					curvol = (NUM_LINES-cr->done)/2;
					songs_set_volume(curvol);
				}
			}
			
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
					gr_string( 0x8000, y, s );
				}
				y += ROW_SPACING;
			}
			
			cr->row += SHEIGHT/200;
			if (cr->row >= ROW_SPACING)
				cr->row = 0;
			break;

		case EVENT_WINDOW_CLOSE:
			gr_free_bitmap_data (&cr->backdrop);
			PHYSFS_close(cr->file);
			songs_set_volume(GameCfg.MusicVolume);
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
	cr->file = PHYSFSX_openReadBuffered( filename );
	if (cr->file == NULL) {
		char nfile[32];
		
		if (credits_filename)
		{
			d_free(cr);
			return;		//ok to not find special filename
		}

		tempp = strchr(filename, '.');
		*tempp = '\0';
		sprintf(nfile, "%s.txb", filename);
		cr->file = PHYSFSX_openReadBuffered(nfile);
		if (cr->file == NULL)
			Error("Missing CREDITS.TEX and CREDITS.TXB file\n");
		cr->have_bin_file = 1;
	}

	set_screen_mode(SCREEN_MENU);
	gr_use_palette_table( "credits.256" );
	cr->backdrop.bm_data=NULL;

	pcx_error = pcx_read_bitmap(STARS_BACKGROUND,&cr->backdrop, BM_LINEAR,backdrop_palette);
	if (pcx_error != PCX_ERROR_NONE)		{
		PHYSFS_close(cr->file);
		d_free(cr);
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
