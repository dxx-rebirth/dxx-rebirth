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

#ifdef RCS
#pragma off (unreferenced)
static char rcsid[] = "$Id: credits.c,v 1.1.1.1 2006/03/17 19:44:11 zicodxx Exp $";
#pragma on (unreferenced)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "error.h"
#include "pstypes.h"
#include "gr.h"
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
#include "songs.h"
#include "text.h"
#include "menu.h"

#define ROW_SPACING			(SHEIGHT/17)
#define NUM_LINES			20 //14
#define CREDITS_FILE 			"credits.tex"

//if filename passed is NULL, show normal credits
void credits_show(char *credits_filename)
{
	CFILE * file;
	int i, j, l, done;
	int pcx_error;
	int buffer_line = 0;
	int first_line_offset;
	int extra_inc=0;
	int have_bin_file = 0;
	char * tempp;
	char filename[32];
	char buffer[NUM_LINES][80];
	ubyte backdrop_palette[768];
	grs_canvas *CreditsOffscreenBuf=NULL;
	grs_bitmap backdrop;

	// Clear out all tex buffer lines.
	for (i=0; i<NUM_LINES; i++ )
		buffer[i][0] = 0;

	sprintf(filename, "%s", CREDITS_FILE);
	have_bin_file = 0;
	if (credits_filename) {
		strcpy(filename,credits_filename);
		have_bin_file = 1;
	}
	file = cfopen( filename, "rb" );
	if (file == NULL) {
		char nfile[32];
		
		if (credits_filename)
			return;		//ok to not find special filename

		tempp = strchr(filename, '.');
		*tempp = '\0';
		sprintf(nfile, "%s.txb", filename);
		file = cfopen(nfile, "rb");
		if (file == NULL)
			Error("Missing CREDITS.TEX and CREDITS.TXB file\n");
		have_bin_file = 1;
	}

	set_screen_mode(SCREEN_MENU);
	backdrop.bm_data=NULL;

	pcx_error = pcx_read_bitmap(STARS_BACKGROUND,&backdrop, BM_LINEAR,backdrop_palette);
	if (pcx_error != PCX_ERROR_NONE)		{
		cfclose(file);
		return;
	}

	songs_play_song( SONG_CREDITS, 1 );

	gr_remap_bitmap_good( &backdrop,backdrop_palette, -1, -1 );

	gr_set_current_canvas(NULL);
	show_fullscr(&backdrop);
	gr_palette_load( gr_palette );
	CreditsOffscreenBuf = gr_create_sub_canvas(grd_curcanv,0,0,SWIDTH,SHEIGHT);

	if (!CreditsOffscreenBuf)
		Error("Not enough memory to allocate Credits Buffer.");

	key_flush();

	done = 0;

	first_line_offset = 0;

	while( 1 ) {
		int k;

		do {
			buffer_line = (buffer_line+1) % NUM_LINES;
			if (cfgets( buffer[buffer_line], 80, file ))	{
				char *p;
				if (have_bin_file) // is this a binary tbl file
					decode_text_line (buffer[buffer_line]);
				p = strchr(&buffer[buffer_line][0],'\n');
				if (p) *p = '\0';
			} else	{
				buffer[buffer_line][0] = 0;
				done++;
			}
		} while (extra_inc--);
		extra_inc = 0;

		for (i=0; i<ROW_SPACING; i += (SHEIGHT/200) )	{
			int y;

			y = first_line_offset - i;
			gr_flip();
			gr_set_current_canvas(CreditsOffscreenBuf);
			show_fullscr(&backdrop);

			for (j=0; j<NUM_LINES; j++ )	{
				char *s;

				l = (buffer_line + j + 1 ) %  NUM_LINES;
				s = buffer[l];

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

			timer_delay(F1_0/25);
		
			//see if redbook song needs to be restarted
			songs_check_redbook_repeat();
			
			k = key_inkey();

			if (k == KEY_PRINT_SCREEN) {
				save_screen_shot(0);
				k = 0;
			}

			if ((k>0)||(done>NUM_LINES))
			{
				gr_free_bitmap_data (&backdrop);
				cfclose(file);
				songs_play_song( SONG_TITLE, 1 );
				gr_free_sub_canvas(CreditsOffscreenBuf);
				return;
			}
		}
	}
}
