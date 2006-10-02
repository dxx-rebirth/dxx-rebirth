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
#include "types.h"
#include "gr.h"
#include "mono.h"
#include "key.h"
#include "palette.h"
#include "game.h"
#include "timer.h"
#include "newmenu.h"
#include "gamefont.h"
#include "network.h"
#include "iff.h"
#include "pcx.h"
#include "u_mem.h"
#include "mouse.h"
#include "joy.h"
#include "screens.h"
#include "digi.h"
#include "cfile.h"
#include "compbit.h"
#include "songs.h"

#define ROW_SPACING			(GHEIGHT/17)
#define NUM_LINES			20 //14
#define CREDITS_BACKGROUND_FILENAME	"stars.pcx"
#define CREDITS_FILE 			"credits.tex"

#ifdef OGL
ubyte fade_values_hires[480] = {
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,8,9,9,10,10,11,11,12,12,
12,13,13,14,14,15,15,15,16,16,17,17,17,18,18,19,19,19,20,20,20,21,21,22,
22,22,23,23,23,24,24,24,24,25,25,25,26,26,26,26,27,27,27,27,28,28,28,28,
28,29,29,29,29,29,29,30,30,30,30,30,30,30,30,30,31,31,31,31,31,31,31,31,
31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,
31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,
31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,
31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,
31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,
31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,30,30,30,30,30,30,30,30,30,
29,29,29,29,29,29,28,28,28,28,28,27,27,27,27,26,26,26,26,25,25,25,24,24,
24,24,23,23,23,22,22,22,21,21,20,20,20,19,19,19,18,18,17,17,17,16,16,15,
15,15,14,14,13,13,12,12,12,11,11,10,10,9,9,8,8,8,7,7,6,6,5,5,4,4,3,3,2,
2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0 };
#else
ubyte fade_values_hires[480] = { 1,1,1,2,2,2,2,2,3,3,3,3,3,4,4,4,4,4,5,5,5,
5,5,5,6,6,6,6,6,7,7,7,7,7,8,8,8,8,8,9,9,9,9,9,10,10,10,10,10,10,11,11,11,11,11,12,12,12,12,12,12,
13,13,13,13,13,14,14,14,14,14,14,15,15,15,15,15,15,16,16,16,16,16,17,17,17,17,17,17,18,18,
18,18,18,18,18,19,19,19,19,19,19,20,20,20,20,20,20,20,21,21,21,21,21,21,22,22,22,22,22,22,
22,22,23,23,23,23,23,23,23,24,24,24,24,24,24,24,24,25,25,25,25,25,25,25,25,25,26,26,26,26,
26,26,26,26,26,27,27,27,27,27,27,27,27,27,27,28,28,28,28,28,28,28,28,28,28,28,28,29,29,29,
29,29,29,29,29,29,29,29,29,29,29,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,
30,30,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,
31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,30,30,30,
30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,30,29,29,29,29,29,29,29,29,29,29,29,29,
29,29,28,28,28,28,28,28,28,28,28,28,28,28,27,27,27,27,27,27,27,27,27,27,26,26,26,26,26,26,
26,26,26,25,25,25,25,25,25,25,25,25,24,24,24,24,24,24,24,24,23,23,23,23,23,23,23,22,22,22,
22,22,22,22,22,21,21,21,21,21,21,20,20,20,20,20,20,20,19,19,19,19,19,19,18,18,18,18,18,18,
18,17,17,17,17,17,17,16,16,16,16,16,15,15,15,15,15,15,14,14,14,14,14,14,13,13,13,13,13,12,
12,12,12,12,12,11,11,11,11,11,10,10,10,10,10,10,9,9,9,9,9,8,8,8,8,8,7,7,7,7,7,6,6,6,6,6,5,5,5,5,
5,5,4,4,4,4,4,3,3,3,3,3,2,2,2,2,2,1,1};
#endif

extern ubyte *gr_bitblt_fade_table;

grs_font * header_font;
grs_font * title_font;
grs_font * names_font;

typedef struct box {
	int left, top, width, height;
} box;

extern inline void scale_line(unsigned char *in, unsigned char *out, int ilen, int olen);

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
	ubyte *fade_values_scalled;
	fix last_time;
	fix time_delay = 2800;
	grs_canvas *CreditsOffscreenBuf=NULL;
	grs_bitmap backdrop;
	box dirty_box[NUM_LINES];

	// Clear out all tex buffer lines.
	for (i=0; i<NUM_LINES; i++ )
	{
		buffer[i][0] = 0;
		dirty_box[i].left = dirty_box[i].top = dirty_box[i].width = dirty_box[i].height = 0;
	}

	fade_values_scalled = malloc(SHEIGHT);
	scale_line(fade_values_hires, fade_values_scalled, 480, GHEIGHT);

	sprintf(filename, "%s", CREDITS_FILE);
	have_bin_file = 0;
	file = cfopen( "credits.tex", "rb" );
	if (file == NULL) {
		file = cfopen("credits.txb", "rb");
		if (file == NULL)
			Error("Missing CREDITS.TEX and CREDITS.TXB file\n");
		have_bin_file = 1;
	}

	set_screen_mode(SCREEN_MENU);

	gr_use_palette_table( "credits.256" );
#ifdef OGL
	gr_palette_load(gr_palette);
#endif
	header_font = gr_init_font( gamefont_curfontname(0));
	title_font = gr_init_font( gamefont_curfontname(3));
	names_font = gr_init_font( gamefont_curfontname(2));
	backdrop.bm_data=NULL;

//MWA  Made backdrop bitmap linear since it should always be.  the current canvas may not
//MWA  be linear, so we can't rely on grd_curcanv->cv_bitmap->bm_type.

	pcx_error = pcx_read_bitmap(CREDITS_BACKGROUND_FILENAME,&backdrop, BM_LINEAR,backdrop_palette);
	if (pcx_error != PCX_ERROR_NONE)		{
		cfclose(file);
		return;
	}

	songs_play_song( SONG_CREDITS, 1 );

	gr_remap_bitmap_good( &backdrop,backdrop_palette, -1, -1 );

	gr_set_current_canvas(NULL);
	show_fullscr(&backdrop);
	gr_update();
	gr_palette_fade_in( gr_palette, 32, 0 );

#ifndef OGL
	CreditsOffscreenBuf = gr_create_canvas(GWIDTH,GHEIGHT);
#else
	CreditsOffscreenBuf = gr_create_sub_canvas(grd_curcanv,0,0,GWIDTH,GHEIGHT);
#endif

	if (!CreditsOffscreenBuf)
		Error("Not enough memory to allocate Credits Buffer.");

	key_flush();

	last_time = timer_get_fixed_seconds();
	done = 0;

	first_line_offset = 0;

	while( 1 ) {
		int k;

		do {
			buffer_line = (buffer_line+1) % NUM_LINES;
			if (cfgets( buffer[buffer_line], 80, file ))	{
				char *p;
				if (have_bin_file) { // is this a binary tbl file
					for (i = 0; i < strlen(buffer[buffer_line]) - 1; i++) {
						encode_rotate_left(&(buffer[buffer_line][i]));
						buffer[buffer_line][i] ^= BITMAP_TBL_XOR;
						encode_rotate_left(&(buffer[buffer_line][i]));
					}
				}
				p = strchr(&buffer[buffer_line][0],'\n');
				if (p) *p = '\0';
			} else	{
				buffer[buffer_line][0] = 0;
				done++;
			}
		} while (extra_inc--);
		extra_inc = 0;

		for (i=0; i<ROW_SPACING; i += (SWIDTH>=640?FONTSCALE_Y(2):1) )	{
			int y;
			box	*new_box;
			grs_bitmap *tempbmp;

			y = first_line_offset - i;
#ifdef OGL
			ogl_start_offscreen_render(0,-1,GWIDTH,GHEIGHT);
#endif
			gr_set_current_canvas(CreditsOffscreenBuf);
			show_fullscr(&backdrop);

			for (j=0; j<NUM_LINES; j++ )	{
				char *s;

				l = (buffer_line + j + 1 ) %  NUM_LINES;
				s = buffer[l];

				if ( s[0] == '!' ) {
					s++;
				} else if ( s[0] == '$' )	{
					grd_curcanv->cv_font = header_font;
					s++;
				} else if ( s[0] == '*' )	{
					grd_curcanv->cv_font = title_font;
					s++;
				} else
					grd_curcanv->cv_font = names_font;

				gr_bitblt_fade_table = fade_values_scalled;

				tempp = strchr( s, '\t' );
				if ( !tempp )	{
				// Wacky Fast Credits thing
					int w, h, aw;

					gr_get_string_size( s, &w, &h, &aw);
					dirty_box[j].width = w;
        				dirty_box[j].height = h;
        				dirty_box[j].top = y;
        				dirty_box[j].left = (GWIDTH - w) / 2;

					gr_printf( 0x8000, y, s );
				}
				gr_bitblt_fade_table = NULL;
				y += ROW_SPACING;
			}

				// Wacky Fast Credits Thing
				for (j=0; j<NUM_LINES; j++ )
				{
					new_box = &dirty_box[j];

					tempbmp = &(CreditsOffscreenBuf->cv_bitmap);

					gr_bm_bitblt(	new_box->width+1,
							new_box->height+4,
							new_box->left,
							new_box->top,
							new_box->left,
							new_box->top,
							tempbmp,
							&(grd_curscreen->sc_canvas.cv_bitmap) );
				}

#ifndef OGL
				for (j=0; j<NUM_LINES; j++ )
				{
					new_box = &dirty_box[j];

					tempbmp = &(CreditsOffscreenBuf->cv_bitmap);

					gr_bm_bitblt(   new_box->width,
							new_box->height+2,
							new_box->left,
							new_box->top,
							new_box->left,
							new_box->top,
							&backdrop,
							tempbmp );
				}
				
#endif
			gr_update();

			while( timer_get_fixed_seconds() < last_time+time_delay );
			last_time = timer_get_fixed_seconds();
		
			k = key_inkey();

#ifndef NDEBUG
			if (k == KEY_BACKSP) {
				Int3();
				k=0;
			}
#endif

			if (k == KEY_PRINT_SCREEN) {
				save_screen_shot(0);
				k = 0;
			}

			if ((k>0)||(done>NUM_LINES))	{
					gr_close_font(header_font);
					gr_close_font(title_font);
					gr_close_font(names_font);
					gr_palette_fade_out( gr_palette, 32, 0 );
					gr_use_palette_table( "palette.256" );
					free(backdrop.bm_data);
					cfclose(file);
					songs_play_song( SONG_TITLE, 1 );
					gr_palette_load( gr_palette );
#ifdef OGL
					gr_free_sub_canvas(CreditsOffscreenBuf);
					ogl_end_offscreen_render();
#else
					gr_free_canvas(CreditsOffscreenBuf);
#endif
				return;
			}
#ifdef OGL
		ogl_end_offscreen_render();
#endif
		}
	}
}
