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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: credits.c,v 1.6 2003-03-15 14:17:52 btb Exp $";
#endif

#ifdef WINDOWS
#include "desw.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "pa_enabl.h"                   //$$POLY_ACC
#include "error.h"
#include "pstypes.h"
#include "gr.h"
#include "mono.h"
#include "key.h"
#include "palette.h"
#include "game.h"
#include "gamepal.h"
#include "timer.h"

#include "newmenu.h"
#include "gamefont.h"
#ifdef NETWORK
#include "network.h"
#endif
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
#include "menu.h"			// for MenuHires

#if defined(POLY_ACC)
#include "poly_acc.h"
#endif

#define ROW_SPACING (MenuHires?26:11)
#define NUM_LINES_HIRES 21
#define NUM_LINES (MenuHires?NUM_LINES_HIRES:20)

ubyte fade_values[200] = { 1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,8,9,9,10,10,
11,11,12,12,12,13,13,14,14,15,15,15,16,16,17,17,17,18,18,19,19,19,20,20,
20,21,21,22,22,22,23,23,23,24,24,24,24,25,25,25,26,26,26,26,27,27,27,27,
28,28,28,28,28,29,29,29,29,29,29,30,30,30,30,30,30,30,30,30,31,31,31,31,
31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,30,30,30,30,30,
30,30,30,30,29,29,29,29,29,29,28,28,28,28,28,27,27,27,27,26,26,26,26,25,
25,25,24,24,24,24,23,23,23,22,22,22,21,21,20,20,20,19,19,19,18,18,17,17,
17,16,16,15,15,15,14,14,13,13,12,12,12,11,11,10,10,9,9,8,8,8,7,7,6,6,5,
5,4,4,3,3,2,2,1 };

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

extern ubyte *gr_bitblt_fade_table;
extern void gr_bm_bitblt(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest);

grs_font * header_font;
grs_font * title_font;
grs_font * names_font;

#ifdef SHAREWARE
#define ALLOWED_CHAR 'S'
#else
#define ALLOWED_CHAR 'R'
#endif

#ifdef RELEASE
#define CREDITS_BACKGROUND_FILENAME (MenuHires?"\x01starsb.pcx":"\x01stars.pcx")	//only read from hog file
#else
#define CREDITS_BACKGROUND_FILENAME (MenuHires?"starsb.pcx":"stars.pcx")
#endif

typedef struct box {
	int left, top, width, height;
} box;

#define CREDITS_FILE    (cfexist("mcredits.tex")?"mcredits.tex":cfexist("ocredits.tex")?"ocredits.tex":"credits.tex")

//if filename passed is NULL, show normal credits
void credits_show(char *credits_filename)
{
	int i, j, l, done;
	CFILE * file;
	char buffer[NUM_LINES_HIRES][80];
	grs_bitmap backdrop;
	ubyte backdrop_palette[768];
	int pcx_error;
	int buffer_line = 0;
	fix last_time;
//	fix time_delay = 4180;			// ~ F1_0 / 12.9
//	fix time_delay = 1784;
	fix time_delay = 2800;
	int first_line_offset,extra_inc=0;
	int have_bin_file = 0;
	char * tempp;
	char filename[32];

WIN(int credinit = 0;)

	box dirty_box[NUM_LINES_HIRES];
	grs_canvas *CreditsOffscreenBuf=NULL;

	WINDOS(
		dd_grs_canvas *save_canv,
		grs_canvas *save_canv
	);

	WINDOS(
		save_canv = dd_grd_curcanv,
		save_canv = grd_curcanv
	);

	// Clear out all tex buffer lines.
	for (i=0; i<NUM_LINES; i++ ) 
	{
		buffer[i][0] = 0;
		dirty_box[i].left = dirty_box[i].top = dirty_box[i].width = dirty_box[i].height = 0;
	}


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

	WIN(DEFINE_SCREEN(NULL));

#ifdef WINDOWS
CreditsPaint:
#endif
	gr_use_palette_table( "credits.256" );
#ifdef OGL
	gr_palette_load(gr_palette);
#endif
#if defined(POLY_ACC)
	pa_update_clut(gr_palette, 0, 256, 0);
#endif
	header_font = gr_init_font( MenuHires?"font1-1h.fnt":"font1-1.fnt" );
	title_font = gr_init_font( MenuHires?"font2-3h.fnt":"font2-3.fnt" );
	names_font = gr_init_font( MenuHires?"font2-2h.fnt":"font2-2.fnt" );
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

WINDOS(
	dd_gr_set_current_canvas(NULL),	
	gr_set_current_canvas(NULL)
);
WIN(DDGRLOCK(dd_grd_curcanv));
	gr_bitmap(0,0,&backdrop);
WIN(DDGRUNLOCK(dd_grd_curcanv));
        gr_update();
	gr_palette_fade_in( gr_palette, 32, 0 );

//	Create a new offscreen buffer for the credits screen
//MWA  Let's be a little smarter about this and check the VR_offscreen buffer
//MWA  for size to determine if we can use that buffer.  If the game size
//MWA  matches what we need, then lets save memory.

#ifndef PA_3DFX_VOODOO
#ifndef WINDOWS
	if (MenuHires && VR_offscreen_buffer->cv_w == 640)	{
		CreditsOffscreenBuf = VR_offscreen_buffer;
	}
	else if (MenuHires)	{
		CreditsOffscreenBuf = gr_create_canvas(640,480);
	}
	else {
		CreditsOffscreenBuf = gr_create_canvas(320,200);
	}
#else
	CreditsOffscreenBuf = gr_create_canvas(640,480);
#endif				 
#else
	CreditsOffscreenBuf = gr_create_canvas(640,480);
#endif

	if (!CreditsOffscreenBuf) 
		Error("Not enough memory to allocate Credits Buffer.");

	//gr_clear_canvas(BM_XRGB(0,0,0));
	key_flush();

#ifdef WINDOWS
	if (!credinit)	
#endif
	{
		last_time = timer_get_fixed_seconds();
		done = 0;
		first_line_offset = 0;
	}

	WIN(credinit = 1);

	while( 1 )	{
		int k;

		do {
			buffer_line = (buffer_line+1) % NUM_LINES;
get_line:;
			if (cfgets( buffer[buffer_line], 80, file ))	{
				char *p;
				if (have_bin_file) {				// is this a binary tbl file
					for (i = 0; i < strlen(buffer[buffer_line]) - 1; i++) {
						encode_rotate_left(&(buffer[buffer_line][i]));
						buffer[buffer_line][i] ^= BITMAP_TBL_XOR;
						encode_rotate_left(&(buffer[buffer_line][i]));
					}
				}
				p = buffer[buffer_line];
				if (p[0] == ';')
					goto get_line;

				if (p[0] == '%')
				{
					if (p[1] == ALLOWED_CHAR)
						strcpy(p,p+2);
					else
						goto get_line;
				}

				p = strchr(&buffer[buffer_line][0],'\n');
				if (p) *p = '\0';
			} else	{
				//fseek( file, 0, SEEK_SET);
				buffer[buffer_line][0] = 0;
				done++;
			}
		} while (extra_inc--);
		extra_inc = 0;

NO_DFX (for (i=0; i<ROW_SPACING; i += (MenuHires?2:1) )	{)
PA_DFX (for (i=0; i<ROW_SPACING; i += (MenuHires?2:1) )	{)
			int y;

			y = first_line_offset - i;

			gr_set_current_canvas(CreditsOffscreenBuf);
		
			gr_bitmap(0,0,&backdrop);

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

				gr_bitblt_fade_table = (MenuHires?fade_values_hires:fade_values);

				tempp = strchr( s, '\t' );
				if ( tempp )	{
				//	Wacky Credits thing
					int w, h, aw, w2, x1, x2;

					*tempp = 0;
					gr_get_string_size( s, &w, &h, &aw );
					x1 = ((MenuHires?320:160)-w)/2;
					gr_printf( x1 , y, s );
					gr_get_string_size( &tempp[1], &w2, &h, &aw );
					x2 = (MenuHires?320:160)+(((MenuHires?320:160)-w2)/2);
					gr_printf( x2, y, &tempp[1] );

					dirty_box[j].left = ((MenuHires?320:160)-w)/2;
					dirty_box[j].top = y;
					dirty_box[j].width =(x2+w2)-x1;
					dirty_box[j].height = h;

					*tempp = '\t';
		
				} else {
				// Wacky Fast Credits thing
					int w, h, aw;

					gr_get_string_size( s, &w, &h, &aw);
					dirty_box[j].width = w;
        			dirty_box[j].height = h;
        			dirty_box[j].top = y;
        			dirty_box[j].left = ((MenuHires?640:320) - w) / 2;

					gr_printf( 0x8000, y, s );
				}
				gr_bitblt_fade_table = NULL;
				if (buffer[l][0] == '!')
					y += ROW_SPACING/2;
				else
					y += ROW_SPACING;
			}

			{	// Wacky Fast Credits Thing
				box	*new_box;
				grs_bitmap *tempbmp;
				
				for (j=0; j<NUM_LINES; j++ )
				{
					new_box = &dirty_box[j];

					tempbmp = &(CreditsOffscreenBuf->cv_bitmap);
			  
				WIN(DDGRSCREENLOCK);
#if defined(POLY_ACC)
					if(new_box->width != 0)
#endif
						gr_bm_bitblt( new_box->width + 1, new_box->height +4,
									new_box->left, new_box->top, new_box->left, new_box->top,
									tempbmp, &(grd_curscreen->sc_canvas.cv_bitmap) );
				WIN(DDGRSCREENUNLOCK);
				}

#if defined(POLY_ACC)
                pa_flush();
#endif

#if !defined(POLY_ACC) || defined(MACINTOSH)
				MAC( if(!PAEnabled) )			// POLY_ACC always on for the macintosh
				for (j=0; j<NUM_LINES; j++ )
				{
					new_box = &dirty_box[j];

					tempbmp = &(CreditsOffscreenBuf->cv_bitmap);

					gr_bm_bitblt(   new_box->width 
									,new_box->height+2
									,new_box->left
									,new_box->top
									,new_box->left
									,new_box->top
									,&backdrop
									,tempbmp );
				}
				
#endif
			gr_update();
				
			}

//		Wacky Fast Credits thing doesn't need this (it's done above)
//@@		WINDOS(
//@@			dd_gr_blt_notrans(CreditsOffscreenBuf, 0,0,0,0,	dd_grd_screencanv, 0,0,0,0),
//@@			gr_bm_ubitblt(grd_curcanv->cv_w, grd_curcanv->cv_h, 0, 0, 0, 0, &(CreditsOffscreenBuf->cv_bitmap), &(grd_curscreen->sc_canvas.cv_bitmap) );
//@@		);

//			mprintf( ( 0, "Fr = %d", (timer_get_fixed_seconds() - last_time) ));
			while( timer_get_fixed_seconds() < last_time+time_delay );
			last_time = timer_get_fixed_seconds();
		
		#ifdef WINDOWS
			{
				MSG msg;

				DoMessageStuff(&msg);

				if (_RedrawScreen) {
					_RedrawScreen = FALSE;

					gr_close_font(header_font);
					gr_close_font(title_font);
					gr_close_font(names_font);

					d_free(backdrop.bm_data);
					gr_free_canvas(CreditsOffscreenBuf);
		
					goto CreditsPaint;
				}

				DDGRRESTORE;
			}
		#endif

			//see if redbook song needs to be restarted
			songs_check_redbook_repeat();

			k = key_inkey();

			#ifndef NDEBUG
			if (k == KEY_BACKSP) {
				Int3();
				k=0;
			}
			#endif

//			{
//				fix ot = time_delay;
//				time_delay += (keyd_pressed[KEY_X] - keyd_pressed[KEY_Z])*100;
//				if (ot!=time_delay)	{
//					mprintf( (0, "[%x] ", time_delay ));
//				}
//			}

			if (k == KEY_PRINT_SCREEN) {
				save_screen_shot(0);
				k = 0;
			}

			if ((k>0)||(done>NUM_LINES))	{
					gr_close_font(header_font);
					gr_close_font(title_font);
					gr_close_font(names_font);
					gr_palette_fade_out( gr_palette, 32, 0 );
					gr_use_palette_table( DEFAULT_PALETTE );
					d_free(backdrop.bm_data);
					cfclose(file);
				WINDOS(
					dd_gr_set_current_canvas(save_canv),
					gr_set_current_canvas(save_canv)
				);
					songs_play_song( SONG_TITLE, 1 );

				#ifdef WINDOWS
					gr_free_canvas(CreditsOffscreenBuf);
				#else					
					if (CreditsOffscreenBuf != VR_offscreen_buffer)
						gr_free_canvas(CreditsOffscreenBuf);
				#endif

				WIN(DEFINE_SCREEN(Menu_pcx_name));
			
				return;
			}
		}

		if (buffer[(buffer_line + 1 ) %  NUM_LINES][0] == '!') {
			first_line_offset -= ROW_SPACING-ROW_SPACING/2;
			if (first_line_offset <= -ROW_SPACING) {
				first_line_offset += ROW_SPACING;
				extra_inc++;
			}
		}
	}

}
