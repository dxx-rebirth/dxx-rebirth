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
 * Routines for menus.
 */



#ifdef RCS
#pragma off (unreferenced)
static char rcsid[] = "$Id: newmenu.c,v 1.1.1.1 2006/03/17 19:44:42 zicodxx Exp $";
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
#include "text.h"
#include "newdemo.h"
#include "newmenu.h"
#include "gamefont.h"
#include "network.h"
#include "iff.h"
#include "pcx.h"
#include "u_mem.h"
#include "mouse.h"
#include "joy.h"
#include "digi.h"
#include "multi.h"
#include "endlevel.h"
#include "screens.h"
#include "kconfig.h"
#include "player.h"
#include "d_glob.h"
#include "d_io.h"
#include "timer.h"
#include "vers_id.h"

#ifdef OGL
#include "ogl_init.h"
#endif

#ifdef GP2X
#include "gp2x.h"
#endif

#define TITLE_FONT  	(Gamefonts[GFONT_BIG_1])
#define SUBTITLE_FONT	(Gamefonts[GFONT_MEDIUM_3])
#define CURRENT_FONT  	(Gamefonts[GFONT_MEDIUM_2])
#define NORMAL_FONT  	(Gamefonts[GFONT_MEDIUM_1])
#define TEXT_FONT  	(Gamefonts[GFONT_MEDIUM_3])

typedef struct bkg {
	grs_canvas * menu_canvas;
	grs_bitmap * saved;			// The background under the menu.
	grs_bitmap * background;
	int background_is_sub;
} bkg;

grs_bitmap nm_background;
grs_bitmap nm_background1;

#define MESSAGEBOX_TEXT_SIZE 2176		// How many characters in messagebox (changed form 300 (fixes crash from show_game_score and friends) - 2000/01/18 Matt Mueller)
#define MAX_TEXT_WIDTH 	FONTSCALE_X((GameArg.GfxUseHiresFont)?240:120)	// How many pixels wide a input box can be

// ZICO - since the background is rescaled the bevels do the same. because of this we need bigger borders or the fonts would be printed inside the bevels...
#define MENSCALE_X ((double)(SWIDTH/320))
#define MENSCALE_Y ((double)(SHEIGHT/200))

extern void gr_bm_bitblt(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest);

void newmenu_close()	{
#ifdef OGL
	ogl_freebmtexture(&nm_background);
	ogl_freebmtexture(&nm_background1);
#endif
	if (nm_background.bm_data)
		free(nm_background.bm_data);
	if (nm_background1.bm_data)
		free(nm_background1.bm_data);
}

// Draw Copyright and Version strings
void nm_draw_copyright()
{
	grd_curcanv->cv_font = GAME_FONT;
	gr_set_fontcolor(BM_XRGB(6,6,6),-1);
	gr_printf(0x8000,grd_curcanv->cv_bitmap.bm_h-FONTSCALE_Y(GAME_FONT->ft_h)-2,TXT_COPYRIGHT);
	gr_set_fontcolor( GR_GETCOLOR(25,0,0), -1);
	gr_printf(0x8000,GHEIGHT-FONTSCALE_Y(grd_curcanv->cv_font->ft_h*3),DESCENT_VERSION);
}

// Draws the background of menus (i.e. Descent Logo screen)
void nm_draw_background1(char * filename)
{
	int pcx_error;

#ifndef OGL
	if (nm_background1.bm_data)
		free(nm_background1.bm_data);

#else
	if (filename == NULL && Function_mode == FMODE_MENU)
		filename = Menu_pcx_name;
	if (filename != NULL)
#endif
	{
		if (nm_background1.bm_data == NULL) {
			ubyte newpal[768];
			atexit( newmenu_close );
			gr_init_bitmap_data (&nm_background1);
			pcx_error = pcx_read_bitmap( filename, &nm_background1, BM_LINEAR, newpal );
			Assert(pcx_error == PCX_ERROR_NONE);
// 			gr_remap_bitmap_good( &nm_background, newpal, -1, -1 );
		}
		gr_palette_load( gr_palette );
#ifndef OGL
		show_fullscr(&nm_background1);
#else
		ogl_ubitmapm_cs(0,0,-1,-1,&nm_background1,-1,F1_0);
#endif

		if (!strcmp(filename,Menu_pcx_name) && Function_mode != FMODE_GAME)
			nm_draw_copyright();
	}
}

// Draws the frame background for menus
void nm_draw_background(int x1, int y1, int x2, int y2 )
{
	int w,h;

#ifndef OGL
	if (nm_background.bm_data)
		free(nm_background.bm_data);
#else
	if (nm_background.bm_data == NULL)
#endif
	{
		int pcx_error;
		ubyte newpal[768];
		atexit( newmenu_close );
		gr_init_bitmap_data (&nm_background);		
		pcx_error = pcx_read_bitmap("SCORES.PCX",&nm_background,BM_LINEAR,newpal);
		Assert(pcx_error == PCX_ERROR_NONE);
		gr_remap_bitmap_good( &nm_background, newpal, -1, -1 );
	}

	if ( x1 < 0 ) x1 = 0;
	if ( y1 < 0 ) y1 = 0;

	w = x2-x1+1;
	h = y2-y1+1;

	grs_canvas *tmp,*old;
	grs_bitmap bg;
	old=grd_curcanv;
	tmp=gr_create_sub_canvas(old,x1,y1,w,h);
	gr_init_sub_bitmap(&bg,&nm_background,0,0,w*(320.0/SWIDTH),h*(200.0/SHEIGHT));//note that we haven't replaced current_canvas yet, so these macros are still ok.
	gr_set_current_canvas(tmp);

	gr_palette_load( gr_palette );

#ifdef OGL
	if (GameArg.OglScissorOk) {
		glEnable(GL_SCISSOR_TEST);
		glScissor(0,y1,x2,SHEIGHT);
		ogl_ubitmapm_cs(0,0,SWIDTH,SHEIGHT,&nm_background,-1,F1_0);
		glDisable(GL_SCISSOR_TEST);
	} else
#endif
		show_fullscr( &bg );
	gr_set_current_canvas(old);
	gr_free_sub_canvas(tmp);

	Gr_scanline_darkening_level = 2*7;

	//scale the bevels to the res.
	gr_setcolor( BM_XRGB(0,0,0) );

	for (w=5*(SWIDTH/320.0);w>=0;w--)
		gr_urect( x2-w, y1+w*((SHEIGHT/200.0)/(SWIDTH/320.0)), x2-w, y2 );//right edge
	for (h=5*(SHEIGHT/200.0);h>=0;h--)
		gr_urect( x1+h*((SWIDTH/320.0)/(SHEIGHT/200.0)), y2-h, x2, y2-h );//bottom edge

	Gr_scanline_darkening_level = GR_FADE_LEVELS;
}

// Draw a left justfied string
void nm_string( bkg * b, int w1,int x, int y, char * _s )
{
	int w,h,aw;
        char *s, *p,*s1=0;

	s = strdup(_s);
	if (!s)
		return;

	p = strchr( s, '\t' );
	if (p && (w1>0) )	{
		*p = '\0';
		s1 = p+1;
	}

	gr_get_string_size(s, &w, &h, &aw  );

	if (w1 > 0)
		w = w1;
#ifndef OGL
	gr_bm_bitblt(b->background->bm_w-(15*MENSCALE_X), h, 5, y, 5, y, b->background, &(grd_curcanv->cv_bitmap) );
#endif
	gr_string( x, y, s );

	if (s1)	{
		gr_get_string_size(s1, &w, &h, &aw  );
		gr_string( x+w1-w, y, s1 );
		*p = '\t';
	}
	free(s);
}


// Draw a slider and it's string
void nm_string_slider( bkg * b, int w1,int x, int y, char * s )
{
	int w,h,aw;
        char *p,*s1=0;

	p = strchr( s, '\t' );
	if (p)	{
		*p = '\0';
		s1 = p+1;
	}

	gr_get_string_size(s, &w, &h, &aw  );
#ifndef OGL
	gr_bm_bitblt(b->background->bm_w-(15*MENSCALE_X), h, 5, y, 5, y, b->background, &(grd_curcanv->cv_bitmap) );
#endif
	gr_string( x, y, s );

	if (p)	{
		gr_get_string_size(s1, &w, &h, &aw  );
#ifndef OGL
		gr_bm_bitblt(w, 1, x+w1-w, y, x+w1-w, y, b->background, &(grd_curcanv->cv_bitmap) );
		gr_bm_bitblt(w, 1, x+w1-w, y+h-1, x+w1-w, y, b->background, &(grd_curcanv->cv_bitmap) );
#endif
		gr_string( x+w1-w, y, s1 );
		*p = '\t';
	}

}

// Draw a left justfied string with black background.
void nm_string_black( bkg * b, int w1,int x, int y, char * s )
{
	int w,h,aw;
	gr_get_string_size(s, &w, &h, &aw  );
	b = b;
	if (w1 == 0) w1 = w;

	gr_setcolor( BM_XRGB(0,0,0) );
	gr_rect( x, y, x+w1-1, y+h-1 );
	gr_string( x, y, s );
}

// Draw a right justfied string
void nm_rstring( bkg * b,int w1,int x, int y, char * s )
{
	int w,h,aw;
	gr_get_string_size(s, &w, &h, &aw  );
	x -= 3;

	if (w1 == 0) w1 = w;
#ifndef OGL
	gr_bm_bitblt(w1, h, x-w1, y, x-w1, y, b->background, &(grd_curcanv->cv_bitmap) );
#endif
	gr_string( x-w, y, s );
}

//for text items, constantly redraw cursor (to achieve flash)
void update_cursor( newmenu_item *item)
{
	int w,h,aw;
	fix time = timer_get_approx_seconds();
	int x,y;
	char * text = item->text;

	Assert(item->type==NM_TYPE_INPUT_MENU || item->type==NM_TYPE_INPUT);

	while( *text )	{
		gr_get_string_size(text, &w, &h, &aw  );
		if ( w > item->w-10 )
			text++;
		else
			break;
	}
	if (*text==0) 
		w = 0;
	x = item->x+w; y = item->y;

	if (time & 0x8000)
		gr_string( x, y, CURSOR_STRING );
	else {
		gr_setcolor( BM_XRGB(0,0,0) );
		gr_rect( x, y, x+FONTSCALE_X(grd_curcanv->cv_font->ft_w-1), y+FONTSCALE_Y(grd_curcanv->cv_font->ft_h-1) );
	}

}

void nm_string_inputbox( bkg *b, int w, int x, int y, char * text, int current )
{
	int w1,h1,aw;

	while( *text )	{
		gr_get_string_size(text, &w1, &h1, &aw  );
		if ( w1 > w-10 )
			text++;
		else
			break;
	}
	if ( *text == 0 )
		w1 = 0;
	nm_string_black( b, w, x, y, text );

	if ( current )	{
		gr_string( x+w1, y, CURSOR_STRING );
	}
}

void draw_item( bkg * b, newmenu_item *item, int is_current, int tiny )
{
	if (tiny)
	{
		if (is_current)
			gr_set_fontcolor(gr_find_closest_color_current(57,49,20),-1);
		else
			gr_set_fontcolor(gr_find_closest_color_current(29,29,47),-1);
	
		if (item->text[0]=='\t')
			gr_set_fontcolor (gr_find_closest_color_current(63,63,63),-1);
	}
	else
	{
		if (is_current)
			grd_curcanv->cv_font = CURRENT_FONT;
		else
			grd_curcanv->cv_font = NORMAL_FONT;
        }
	
	switch( item->type )	{
		case NM_TYPE_TEXT:
		case NM_TYPE_MENU:
			nm_string( b, item->w, item->x, item->y, item->text );
			break;
		case NM_TYPE_SLIDER:	{
			int i,j;
			if (item->value < item->min_value) item->value=item->min_value;
			if (item->value > item->max_value) item->value=item->max_value;
			i = sprintf( item->saved_text, "%s\t%s", item->text, SLIDER_LEFT );
			for (j=0; j<(item->max_value-item->min_value+1); j++ )	{
				i += sprintf( item->saved_text + i, "%s", SLIDER_MIDDLE );
			}
			sprintf( item->saved_text + i, "%s", SLIDER_RIGHT );
			
			item->saved_text[item->value+1+strlen(item->text)+1] = SLIDER_MARKER[0];
			
			nm_string_slider( b, item->w, item->x, item->y, item->saved_text );
			}
			break;
		case NM_TYPE_INPUT_MENU:
			if ( item->group==0 )		{
				nm_string( b, item->w, item->x, item->y, item->text );
			} else {
				nm_string_inputbox( b, item->w, item->x, item->y, item->text, is_current );
			}
			break;
		case NM_TYPE_INPUT:
			nm_string_inputbox( b, item->w, item->x, item->y, item->text, is_current );
			break;
		case NM_TYPE_CHECK:
			nm_string( b, item->w, item->x, item->y, item->text );
			if (item->value)
				nm_rstring( b,item->right_offset,item->x, item->y, CHECKED_CHECK_BOX );
			else														  
				nm_rstring( b,item->right_offset,item->x, item->y, NORMAL_CHECK_BOX );
			break;
		case NM_TYPE_RADIO:
			nm_string( b, item->w, item->x, item->y, item->text );
			if (item->value)
				nm_rstring( b,item->right_offset, item->x, item->y, CHECKED_RADIO_BOX );
			else
				nm_rstring( b,item->right_offset, item->x, item->y, NORMAL_RADIO_BOX );
			break;
		case NM_TYPE_NUMBER:	{
			char text[10];
			if (item->value < item->min_value) item->value=item->min_value;
			if (item->value > item->max_value) item->value=item->max_value;
			nm_string( b, item->w, item->x, item->y, item->text );
			sprintf( text, "%d", item->value );
			nm_rstring( b,item->right_offset,item->x, item->y, text );
			}
			break;
	}

}

char *Newmenu_allowed_chars=NULL;

//returns true if char is allowed
int char_allowed(char c)
{
	char *p = Newmenu_allowed_chars;

	if (!p)
		return 1;

	while (*p) {
		Assert(p[1]);

		if (c>=p[0] && c<=p[1])
			return 1;

		p += 2;
	}

	return 0;
}

void strip_end_whitespace( char * text )
{
	int i,l;
	l = strlen( text );
	for (i=l-1; i>=0; i-- )	{
		if ( isspace(text[i]) )
			text[i] = 0;
		else
			return;
	}
}

//returns 1 if a control device button has been pressed
int check_button_press()
{
	int i;

	switch (Config_control_type) {
	case	CONTROL_JOYSTICK:
		for (i=0; i<4; i++ )	
	 		if (joy_get_button_down_cnt(i)>0) return 1;
		break;
	case	CONTROL_MOUSE:
	case	CONTROL_JOYMOUSE:
#ifndef NEWMENU_MOUSE   // don't allow mouse to continue from menu
		for (i=0; i<3; i++ )	
			if (mouse_button_down_count(i)>0) return 1;
		break;
#endif
	case	CONTROL_NONE: //keyboard only
		break;
	default:
		Error("Bad control type (Config_control_type):%i",Config_control_type);
	}

	return 0;
}

#ifdef NEWMENU_MOUSE
ubyte Hack_DblClick_MenuMode=0;
#endif

#define CLOSE_X     (7*MENSCALE_X)
#define CLOSE_Y     (7*MENSCALE_Y)
#define CLOSE_SIZE  FONTSCALE_X(GameArg.GfxUseHiresFont&&grd_curscreen->sc_w>=640&&grd_curscreen->sc_h>=480?10:5)

void draw_close_box(int x,int y)
{
	gr_setcolor( BM_XRGB(0, 0, 0) );
	gr_rect(x + CLOSE_X, y + CLOSE_Y, x + CLOSE_X + CLOSE_SIZE, y + CLOSE_Y + CLOSE_SIZE);
	gr_setcolor( BM_XRGB(21, 21, 21) );
	gr_rect(x + CLOSE_X + (1), y + CLOSE_Y + (1), x + CLOSE_X + CLOSE_SIZE - (1), y + CLOSE_Y + CLOSE_SIZE - (1));
}

int newmenu_do3_real( char * title, char * subtitle, int nitems, newmenu_item * item, void (*subfunction)(int nitems,newmenu_item * items, int * last_key, int citem), int citem, char * filename, int width, int height, int TinyMode);

int newmenu_do( char * title, char * subtitle, int nitems, newmenu_item * item, void (*subfunction)(int nitems,newmenu_item * items, int * last_key, int citem) )
{
	return newmenu_do3( title, subtitle, nitems, item, subfunction, 0, NULL, -1, -1 );
}

int newmenu_do1( char * title, char * subtitle, int nitems, newmenu_item * item, void (*subfunction)(int nitems,newmenu_item * items, int * last_key, int citem), int citem )
{
	return newmenu_do3( title, subtitle, nitems, item, subfunction, citem, NULL, -1, -1 );
}


int newmenu_do2( char * title, char * subtitle, int nitems, newmenu_item * item, void (*subfunction)(int nitems,newmenu_item * items, int * last_key, int citem), int citem, char * filename )
{
	return newmenu_do3( title, subtitle, nitems, item, subfunction, citem, filename, -1, -1 );
}

int newmenu_do_fixedfont( char * title, char * subtitle, int nitems, newmenu_item * item, void (*subfunction)(int nitems,newmenu_item * items, int * last_key, int citem), int citem, char * filename, int width, int height){
	set_screen_mode(SCREEN_MENU);//hafta set the screen mode before calling or fonts might get changed/freed up if screen res changes
	return newmenu_do3_real( title, subtitle, nitems, item, subfunction, citem, filename, width,height, 0);
}

int newmenu_do3( char * title, char * subtitle, int nitems, newmenu_item * item, void (*subfunction)(int nitems,newmenu_item * items, int * last_key, int citem), int citem, char * filename, int width, int height){
	set_screen_mode(SCREEN_MENU);//hafta set the screen mode before calling or fonts might get changed/freed up if screen res changes
	return newmenu_do3_real( title, subtitle, nitems, item, subfunction, citem, filename, width,height, 0);
}

int newmenu_dotiny( char * title, char * subtitle, int nitems, newmenu_item * item, void (*subfunction)(int nitems,newmenu_item * items, int * last_key, int citem) )
{
        return newmenu_do3_real( title, subtitle, nitems, item, subfunction, 0, NULL, -1, -1, 1 );
}

//Edited 2000/10/27 Matthew Mueller - made newmenu_do3 allow you to set the fonts used, thus allowing newmenu_do_fixedfont to be removed, saving a lot of duplication.
int newmenu_do3_real( char * title, char * subtitle, int nitems, newmenu_item * item, void (*subfunction)(int nitems,newmenu_item * items, int * last_key, int citem), int citem, char * filename, int width, int height, int TinyMode)
{
	int old_keyd_repeat, done;
	int  choice,old_choice,i,j,x,y,w,h,aw, tw, th, twidth,fm,right_offset;
	int k, nmenus, nothers;
	grs_canvas * save_canvas;
	grs_font * save_font;
	int string_width, string_height, average_width;
	int ty;
	bkg bg;
	int all_text=0; //set true if all text items
	int sound_stopped=0, time_stopped=0;
#ifdef NEWMENU_MOUSE
	int mouse_state, omouse_state, dblclick_flag=0;
	int mx=0, my=0, mz=0, x1 = 0, x2, y1, y2;
	int close_box=0;
#endif

	if (nitems < 1 )
		return -1;

	if ( Function_mode == FMODE_GAME )	{
		digi_pause_all();
		sound_stopped = 1;
	}

	if (!((Game_mode & GM_MULTI) && (Function_mode == FMODE_GAME) && (!Endlevel_sequence)) )
	{
		time_stopped = 1;
		stop_time();
	}

	save_canvas = grd_curcanv;
	gr_set_current_canvas( NULL );
	save_font = grd_curcanv->cv_font;

	tw = th = 0;

	if ( title )	{
		grd_curcanv->cv_font = TITLE_FONT;
		gr_get_string_size(title,&string_width,&string_height,&average_width );
		tw = string_width;
		th = string_height;
	}
	if ( subtitle )	{
		grd_curcanv->cv_font = SUBTITLE_FONT;
		gr_get_string_size(subtitle,&string_width,&string_height,&average_width );
		if (string_width > tw )
			tw = string_width;
		th += string_height;
	}


	th += 8*MENSCALE_Y; //put some space between titles & body

	if (TinyMode)
		grd_curcanv->cv_font = GAME_FONT;
	else 
		grd_curcanv->cv_font = NORMAL_FONT;

	w = aw = 0;
	h = th;
	nmenus = nothers = 0;

	// Find menu height & width (store in w,h)
	for (i=0; i<nitems; i++ )	{
		item[i].redraw=1;
		item[i].y = h;
		gr_get_string_size(item[i].text,&string_width,&string_height,&average_width );
		item[i].right_offset = 0;

		item[i].saved_text[0] = '\0';

		if ( item[i].type == NM_TYPE_SLIDER )	{
			int index,w1,h1,aw1;
			nothers++;
			index = sprintf( item[i].saved_text, "%s", SLIDER_LEFT );
			for (j=0; j<(item[i].max_value-item[i].min_value+1); j++ )	{
				index+= sprintf( item[i].saved_text + index, "%s", SLIDER_MIDDLE );
			}
			sprintf( item[i].saved_text + index, "%s", SLIDER_RIGHT );
			gr_get_string_size(item[i].saved_text,&w1,&h1,&aw1 );
			string_width += w1 + aw;
		}

		if ( item[i].type == NM_TYPE_MENU )	{
			nmenus++;
		}

		if ( item[i].type == NM_TYPE_CHECK )	{
			int w1,h1,aw1;
			nothers++;
			gr_get_string_size(NORMAL_CHECK_BOX, &w1, &h1, &aw1  );
			item[i].right_offset = w1;
			gr_get_string_size(CHECKED_CHECK_BOX, &w1, &h1, &aw1  );
			if (w1 > item[i].right_offset)
				item[i].right_offset = w1;
		}
		
		if (item[i].type == NM_TYPE_RADIO ) {
			int w1,h1,aw1;
			nothers++;
			gr_get_string_size(NORMAL_RADIO_BOX, &w1, &h1, &aw1  );
			item[i].right_offset = w1;
			gr_get_string_size(CHECKED_RADIO_BOX, &w1, &h1, &aw1  );
			if (w1 > item[i].right_offset)
				item[i].right_offset = w1;
		}

		if  (item[i].type==NM_TYPE_NUMBER )	{
			int w1,h1,aw1;
			char test_text[20];
			nothers++;
			sprintf( test_text, "%d", item[i].max_value );
			gr_get_string_size( test_text, &w1, &h1, &aw1 );
			item[i].right_offset = w1;
			sprintf( test_text, "%d", item[i].min_value );
			gr_get_string_size( test_text, &w1, &h1, &aw1 );
			if ( w1 > item[i].right_offset)
				item[i].right_offset = w1;
		}

		if ( item[i].type == NM_TYPE_INPUT )	{
			Assert( strlen(item[i].text) < NM_MAX_TEXT_LEN );
			strcpy(item[i].saved_text, item[i].text );
			nothers++;
			string_width = item[i].text_len*FONTSCALE_X(grd_curcanv->cv_font->ft_w)+item[i].text_len;
			if ( string_width > MAX_TEXT_WIDTH ) 
				string_width = MAX_TEXT_WIDTH;
			item[i].value = -1;
		}

		if ( item[i].type == NM_TYPE_INPUT_MENU )	{
			Assert( strlen(item[i].text) < NM_MAX_TEXT_LEN );
			strcpy(item[i].saved_text, item[i].text );
			nmenus++;
			string_width = item[i].text_len*FONTSCALE_X(grd_curcanv->cv_font->ft_w)+item[i].text_len;
			item[i].value = -1;
			item[i].group = 0;
		}

		item[i].w = string_width;
		item[i].h = string_height;

		if ( string_width > w )
			w = string_width;		// Save maximum width
		if ( average_width > aw )
			aw = average_width;
		h += string_height+(FONTSCALE_Y(1));		// Find the height of all strings
	}

	right_offset=0;

	if ( width > -1 )
		w = width;

	if ( height > -1 )
		h = height;

	for (i=0; i<nitems; i++ )	{
		item[i].w = w;
		if (item[i].right_offset > right_offset )
			right_offset = item[i].right_offset;
	}
	if (right_offset > 0 )
		right_offset += 3;
	
	w += right_offset;

	twidth = 0;
	if ( tw > w )	{
		twidth = ( tw - w )/2;
		w = tw;
	}
			
	// Find min point of menu border
	w += 30*MENSCALE_X;
	h += 30*MENSCALE_Y;

	/* If window is as or almost as big as screen define hard size so it fits (with borders and stuff).
	   Also make use of MENSCALE_* so we are sure it does scale correct if font does scale or not */
	if (w >= 320*MENSCALE_X-3)
		w=318*MENSCALE_X;
	// when using GameArg.GfxUseHiresFont we need to resize from a base of 240 since the font is made for 640x480
	if (h >= ((GameArg.GfxUseHiresFont&&SHEIGHT>=480)?240:200)*MENSCALE_Y-3)
		h=((GameArg.GfxUseHiresFont&&SHEIGHT>=480)?236:198)*MENSCALE_Y;

	x = (GWIDTH-w)/2;
	y = (GHEIGHT-h)/2;

	if ( x < 0 ) x = 0;
	if ( y < 0 ) y = 0;

#ifndef OGL
	if ( filename != NULL )	{
		nm_draw_background1( filename );
		gr_palette_load(gr_palette);
	}
#endif

	// Save the background of the display
	bg.menu_canvas = gr_create_sub_canvas( &grd_curscreen->sc_canvas, x, y, w+MENSCALE_X, h+MENSCALE_Y );
	gr_set_current_canvas( bg.menu_canvas );

	ty = (15*MENSCALE_Y);

#ifndef OGL
	if ( filename == NULL )	{
		// Save the background under the menu...
		bg.saved = gr_create_bitmap( w+MENSCALE_X+10, h+MENSCALE_Y );
		Assert( bg.saved != NULL );
		gr_bm_bitblt(w+MENSCALE_X, h+MENSCALE_Y, 0, 0, 0, 0, &grd_curcanv->cv_bitmap, bg.saved );
		gr_set_current_canvas( NULL );
		nm_draw_background(x,y,x+w,y+h);

		if ( GWIDTH > nm_background.bm_w || GHEIGHT > nm_background.bm_h ) {
			grs_bitmap sbg;
			gr_init_sub_bitmap(&sbg,&nm_background,0,0,w*(320.0/GWIDTH),h*(200.0/GHEIGHT));//use the correctly resized portion of the background instead of the whole thing -MPM
			bg.background=gr_create_bitmap(w,h);
			gr_bitmap_scale_to(&sbg,bg.background);
			bg.background_is_sub=0;
		}
		else
		{
			bg.background = gr_create_sub_bitmap(&nm_background,0,0,w,h);
			bg.background_is_sub=1;
		}
		gr_set_current_canvas( bg.menu_canvas );
	} else {
		bg.saved = NULL;
		bg.background = gr_create_bitmap( w, h );
		Assert( bg.background != NULL );
		gr_bm_bitblt(w, h, 0, 0, 0, 0, &grd_curcanv->cv_bitmap, bg.background );
	}

	if ( title )	{
		grd_curcanv->cv_font = TITLE_FONT;
		gr_set_fontcolor( GR_GETCOLOR(31,31,31), -1 );
		gr_get_string_size(title,&string_width,&string_height,&average_width );
		tw = string_width;
		th = string_height;
		gr_printf( 0x8000, ty, title );
	}
	
	if ( subtitle )	{
		grd_curcanv->cv_font = SUBTITLE_FONT;
		gr_set_fontcolor( GR_GETCOLOR(21,21,21), -1 );
		gr_get_string_size(subtitle,&string_width,&string_height,&average_width );
		tw = string_width;
		th = (title?th:0);
		gr_printf( 0x8000, ty+th, subtitle );
	}

	if (TinyMode)
		grd_curcanv->cv_font = GAME_FONT;
	else 
		grd_curcanv->cv_font = NORMAL_FONT;
#endif

	// Update all item's x & y values.
	for (i=0; i<nitems; i++ )	{
		item[i].x = (15*MENSCALE_X) + twidth + right_offset;
		item[i].y += (15*MENSCALE_Y);
		if ( item[i].type==NM_TYPE_RADIO )	{
			fm = -1;	// find first marked one
			for ( j=0; j<nitems; j++ )	{
				if ( item[j].type==NM_TYPE_RADIO && item[j].group==item[i].group )	{
					if (fm==-1 && item[j].value)
						fm = j;
					item[j].value = 0;
				}
			}
			if ( fm>=0 )	
				item[fm].value=1;
			else
				item[i].value=1;
		}
	}

	old_keyd_repeat = keyd_repeat;
	keyd_repeat = 1;

	if (citem==-1)	{
		choice = -1;
	} else {
		if (citem < 0 ) citem = 0;
		if (citem > nitems-1 ) citem = nitems-1;
		choice = citem;

#ifdef NEWMENU_MOUSE
		dblclick_flag = 1;
#endif

		while ( item[choice].type==NM_TYPE_TEXT )	{
			choice++;
			if (choice >= nitems ) {
				choice=0; 
			}
			if (choice == citem ) {
				choice=0; 
				all_text=1;
				break; 
			}
		}
	}
	done = 0;
        gr_update();
	// Clear mouse, joystick to clear button presses.
	game_flush_inputs();

#ifdef NEWMENU_MOUSE
	mouse_state = omouse_state = 0;
	newmenu_show_cursor();
#endif

	while(!done)	{
		timer_delay(400);
#ifdef OGL
		gr_flip();

		gr_set_current_canvas( NULL );
		nm_draw_background1(filename);
		if (filename == NULL)
			nm_draw_background(x,y,x+w,y+h);

		gr_set_current_canvas( bg.menu_canvas );

		if ( title )	{
			grd_curcanv->cv_font = TITLE_FONT;
			gr_set_fontcolor( GR_GETCOLOR(31,31,31), -1 );
			gr_get_string_size(title,&string_width,&string_height,&average_width );
			tw = string_width;
			th = string_height;
			gr_printf( 0x8000, ty, title );
		}
	
		if ( subtitle )	{
			grd_curcanv->cv_font = SUBTITLE_FONT;
			gr_set_fontcolor( GR_GETCOLOR(21,21,21), -1 );
			gr_get_string_size(subtitle,&string_width,&string_height,&average_width );
			tw = string_width;
			th = (title?th:0);
			gr_printf( 0x8000, ty+th, subtitle );
		}

		if (TinyMode)
			grd_curcanv->cv_font = GAME_FONT;
		else 
			grd_curcanv->cv_font = NORMAL_FONT;
#endif

		if (filename == NULL) {
			draw_close_box(0,0);
			close_box = 1;
		}

#ifdef NEWMENU_MOUSE
		newmenu_show_cursor();      // possibly hidden
		omouse_state = mouse_state;
		mouse_state = mouse_button_state(0);
#endif

		k = key_inkey();

		switch( k )
		{
			case KEY_PAD0: k = KEY_0;  break;
			case KEY_PAD1: k = KEY_1;  break;
			case KEY_PAD2: k = KEY_2;  break;
			case KEY_PAD3: k = KEY_3;  break;
			case KEY_PAD4: k = KEY_4;  break;
			case KEY_PAD5: k = KEY_5;  break;
			case KEY_PAD6: k = KEY_6;  break;
			case KEY_PAD7: k = KEY_7;  break;
			case KEY_PAD8: k = KEY_8;  break;
			case KEY_PAD9: k = KEY_9;  break;
			case KEY_PADPERIOD: k = KEY_PERIOD; break;
		}

		if (subfunction)
			(*subfunction)(nitems,item,&k,choice);

		if (!time_stopped)	{
			// Save current menu box
			#ifdef NETWORK
			if (multi_menu_poll() == -1)
				k = -2;
			#endif
		}

		if ( k<-1 ) {
			choice = k;
			k = -1;
			done = 1;
		}
		if (check_button_press())
			done = 1;
				
		old_choice = choice;
	
		switch( k )	{
		case KEY_TAB + KEY_SHIFTED:
		case KEY_UP:
		case KEY_PAD8:
			if (all_text) break;
			do {
				choice--;
				if (choice >= nitems ) choice=0;
				if (choice < 0 ) choice=nitems-1;
			} while ( item[choice].type==NM_TYPE_TEXT );
			if ((item[choice].type==NM_TYPE_INPUT) && (choice!=old_choice))	
				item[choice].value = -1;
			if ((old_choice>-1) && (item[old_choice].type==NM_TYPE_INPUT_MENU) && (old_choice!=choice))	{
				item[old_choice].group=0;
				strcpy(item[old_choice].text, item[old_choice].saved_text );
				item[old_choice].value = -1;
			}
			if (old_choice>-1) 
				item[old_choice].redraw = 1;
			item[choice].redraw=1;
			break;
		case KEY_TAB:
		case KEY_DOWN:
		case KEY_PAD2:
			if (all_text) break;
			do {
				choice++;
				if (choice < 0 ) choice=nitems-1;
				if (choice >= nitems ) choice=0;
			} while ( item[choice].type==NM_TYPE_TEXT );
			if ((item[choice].type==NM_TYPE_INPUT) && (choice!=old_choice))	
				item[choice].value = -1;
			if ( (old_choice>-1) && (item[old_choice].type==NM_TYPE_INPUT_MENU) && (old_choice!=choice))	{
				item[old_choice].group=0;
				strcpy(item[old_choice].text, item[old_choice].saved_text );	
				item[old_choice].value = -1;
			}
			if (old_choice>-1)
				item[old_choice].redraw=1;
			item[choice].redraw=1;
			break;
		case KEY_SPACEBAR:
			if ( choice > -1 )	{
				switch( item[choice].type )	{
				case NM_TYPE_MENU:
				case NM_TYPE_INPUT:
				case NM_TYPE_INPUT_MENU:
					break;
				case NM_TYPE_CHECK:
					if ( item[choice].value )
						item[choice].value = 0;
					else
						item[choice].value = 1;
					item[choice].redraw=1;
					break;
				case NM_TYPE_RADIO:
					for (i=0; i<nitems; i++ )	{
						if ((i!=choice) && (item[i].type==NM_TYPE_RADIO) && (item[i].group==item[choice].group) && (item[i].value) )	{
							item[i].value = 0;
							item[i].redraw = 1;
						}
					}
					item[choice].value = 1;
					item[choice].redraw = 1;
					break;
				}	
			}
			break;

		case KEY_ENTER:
		case KEY_PADENTER:
#ifdef GP2X
		case GP2X_BUTTON_B:
#endif
			if ( (choice>-1) && (item[choice].type==NM_TYPE_INPUT_MENU) && (item[choice].group==0))	{
				item[choice].group = 1;
				item[choice].redraw = 1;
				if ( !strncasecmp( item[choice].saved_text, TXT_EMPTY, strlen(TXT_EMPTY) ) )	{
					item[choice].text[0] = 0;
					item[choice].value = -1;
				} else {	
					strip_end_whitespace(item[choice].text);
				}
			} else
				done = 1;
			break;

		case KEYS_GR_TOGGLE_FULLSCREEN:
			gr_toggle_fullscreen();
			break;

		case KEY_ESC:
			if ( (choice>-1) && (item[choice].type==NM_TYPE_INPUT_MENU) && (item[choice].group==1))	{
				item[choice].group=0;
				strcpy(item[choice].text, item[choice].saved_text );	
				item[choice].redraw=1;
				item[choice].value = -1;
			} else {
				done = 1;
				choice = -1;
			}
			break;

		case KEY_PRINT_SCREEN: save_screen_shot(0); break;

		#ifndef NDEBUG
		case KEY_BACKSP:	
			if ( (choice>-1) && (item[choice].type!=NM_TYPE_INPUT)&&(item[choice].type!=NM_TYPE_INPUT_MENU))
				Int3(); 
			break;
		#endif

		}

#ifdef NEWMENU_MOUSE // for mouse selection of menu's etc.
		if ( !done && mouse_state && !omouse_state && !all_text ) {
			mouse_get_pos(&mx, &my, &mz);
			for (i=0; i<nitems; i++ )	{
				x1 = grd_curcanv->cv_bitmap.bm_x + item[i].x - item[i].right_offset - 6;
				x2 = x1 + item[i].w;
				y1 = grd_curcanv->cv_bitmap.bm_y + item[i].y;
				y2 = y1 + item[i].h;
				if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2))) {
					if (i != choice) {
						if(Hack_DblClick_MenuMode) dblclick_flag = 0; 
					}
					
					choice = i;

					switch( item[choice].type )	{
					case NM_TYPE_CHECK:
						if ( item[choice].value )
							item[choice].value = 0;
						else
							item[choice].value = 1;
						item[choice].redraw=1;
						break;
					case NM_TYPE_RADIO:
						for (i=0; i<nitems; i++ )	{
							if ((i!=choice) && (item[i].type==NM_TYPE_RADIO) && (item[i].group==item[choice].group) && (item[i].value) )	{
								item[i].value = 0;
								item[i].redraw = 1;
							}
						}
						item[choice].value = 1;
						item[choice].redraw = 1;
						break;
					}
					item[old_choice].redraw=1;
					break;
				}
			}
		}

		if (mouse_state && all_text)
			done = 1;
		
		if ( !done && mouse_state && !all_text ) {
			mouse_get_pos(&mx, &my, &mz);

			for (i=0; i<nitems; i++ )	{
				x1 = grd_curcanv->cv_bitmap.bm_x + item[i].x - item[i].right_offset - 6;
				x2 = x1 + item[i].w;
				y1 = grd_curcanv->cv_bitmap.bm_y + item[i].y;
				y2 = y1 + item[i].h;
				if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) && (item[i].type != NM_TYPE_TEXT) ) {
					if (i != choice) {
						if(Hack_DblClick_MenuMode) dblclick_flag = 0; 
					}

					choice = i;

					if ( item[choice].type == NM_TYPE_SLIDER ) {
						char slider_text[NM_MAX_TEXT_LEN+1], *p, *s1;
						int slider_width, height, aw, sleft_width, sright_width, smiddle_width;
						
						strcpy(slider_text, item[choice].saved_text);
						p = strchr(slider_text, '\t');
						if (p) {
							*p = '\0';
							s1 = p+1;
						}
						if (p) {
							gr_get_string_size(s1, &slider_width, &height, &aw);
							gr_get_string_size(SLIDER_LEFT, &sleft_width, &height, &aw);
							gr_get_string_size(SLIDER_RIGHT, &sright_width, &height, &aw);
							gr_get_string_size(SLIDER_MIDDLE, &smiddle_width, &height, &aw);

							x1 = grd_curcanv->cv_bitmap.bm_x + item[choice].x + item[choice].w - slider_width;
							x2 = x1 + slider_width + sright_width;
							if ( (mx > x1) && (mx < (x1 + sleft_width)) && (item[choice].value != item[choice].min_value) ) {
								item[choice].value = item[choice].min_value;
								item[choice].redraw = 2;
							} else if ( (mx < x2) && (mx > (x2 - sright_width)) && (item[choice].value != item[choice].max_value) ) {
								item[choice].value = item[choice].max_value;
								item[choice].redraw = 2;
							} else if ( (mx > (x1 + sleft_width)) && (mx < (x2 - sright_width)) ) {
								int num_values, value_width, new_value;
								
								num_values = item[choice].max_value - item[choice].min_value + 1;
								value_width = (slider_width - sleft_width - sright_width) / num_values;
								new_value = (mx - x1 - sleft_width) / value_width;
								if ( item[choice].value != new_value ) {
									item[choice].value = new_value;
									item[choice].redraw = 2;
								}
							}
							*p = '\t';
						}
					}
					if (choice == old_choice)
						break;
					if ((item[choice].type==NM_TYPE_INPUT) && (choice!=old_choice))	
						item[choice].value = -1;
					if ((old_choice>-1) && (item[old_choice].type==NM_TYPE_INPUT_MENU) && (old_choice!=choice))	{
						item[old_choice].group=0;
						strcpy(item[old_choice].text, item[old_choice].saved_text );
						item[old_choice].value = -1;
					}
					if (old_choice>-1) 
						item[old_choice].redraw = 1;
					item[choice].redraw=1;
					break;
				}
			}
		}
		
		if ( !done && !mouse_state && omouse_state && !all_text && (choice != -1) && (item[choice].type == NM_TYPE_MENU) ) {
			mouse_get_pos(&mx, &my, &mz);
			x1 = grd_curcanv->cv_bitmap.bm_x + item[choice].x;
			x2 = x1 + item[choice].w;
			y1 = grd_curcanv->cv_bitmap.bm_y + item[choice].y;
			y2 = y1 + item[choice].h;
			if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2))) {
				if (Hack_DblClick_MenuMode) {
					if (dblclick_flag) done = 1;
					else dblclick_flag = 1;
				}
				else done = 1;
			}
		}
		
		if ( !done && !mouse_state && omouse_state && (choice>-1) && (item[choice].type==NM_TYPE_INPUT_MENU) && (item[choice].group==0))	{
			item[choice].group = 1;
			item[choice].redraw = 1;
			if ( !/*strnicmp*/strncasecmp( item[choice].saved_text, TXT_EMPTY, strlen(TXT_EMPTY) ) )	{
				item[choice].text[0] = 0;
				item[choice].value = -1;
			} else {
				strip_end_whitespace(item[choice].text);
			}
		}
		
		if ( !done && !mouse_state && omouse_state && close_box ) {
			mouse_get_pos(&mx, &my, &mz);
			x1 = grd_curcanv->cv_bitmap.bm_x + CLOSE_X;
			x2 = x1 + CLOSE_SIZE;
			y1 = grd_curcanv->cv_bitmap.bm_y + CLOSE_Y;
			y2 = y1 + CLOSE_SIZE;
			if ( ((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) ) {
				choice = -1;
				done = 1;
			}
		}
#endif // NEWMENU_MOUSE

		if ( (choice > -1) && (k != -1))	{
			int ascii;

			if ( ((item[choice].type==NM_TYPE_INPUT)||((item[choice].type==NM_TYPE_INPUT_MENU)&&(item[choice].group==1)) )&& (old_choice==choice) )	{
				if ( k==KEY_LEFT || k==KEY_BACKSP || k==KEY_PAD4 )	{
					if (item[choice].value==-1) item[choice].value = strlen(item[choice].text);
					if (item[choice].value > 0)
						item[choice].value--;
					item[choice].text[item[choice].value] = 0;
					item[choice].redraw = 1;	
				} else {
					ascii = key_to_ascii(k);
					if ((ascii < 255 ) && (item[choice].value < item[choice].text_len ))
					{
						int allowed;

						if (item[choice].value==-1) {
							item[choice].value = 0;
						}

						allowed = char_allowed(ascii);
						if (!allowed && ascii==' ' && char_allowed('_')) {
							ascii = '_';
							allowed=1;
						}

						if (allowed) {
							item[choice].text[item[choice].value++] = ascii;
							item[choice].text[item[choice].value] = 0;
							item[choice].redraw=1;	
						}
					}
				}
			} else if ((item[choice].type!=NM_TYPE_INPUT) && (item[choice].type!=NM_TYPE_INPUT_MENU) ) {
				ascii = key_to_ascii(k);
				if (ascii < 255 ) {
					int choice1 = choice;
					ascii = toupper(ascii);
					do {
						int i,ch;
						choice1++;
						if (choice1 >= nitems ) choice1=0;
						for (i=0;(ch=item[choice1].text[i])!=0 && ch==' ';i++);
						if ( ( (item[choice1].type==NM_TYPE_MENU) ||
								 (item[choice1].type==NM_TYPE_CHECK) ||
								 (item[choice1].type==NM_TYPE_RADIO) ||
								 (item[choice1].type==NM_TYPE_NUMBER) ||
								 (item[choice1].type==NM_TYPE_SLIDER) )
								&& (ascii==toupper(ch)) )	{
							k = 0;
							choice = choice1;
							if (old_choice>-1)
								item[old_choice].redraw=1;
							item[choice].redraw=1;
						}
					} while (choice1 != choice );
				}	
			}

			if ( (item[choice].type==NM_TYPE_NUMBER) || (item[choice].type==NM_TYPE_SLIDER)) 	{
				int ov=item[choice].value;
				switch( k ) {
				case KEY_PAD4:
				case KEY_LEFT:
				case KEY_MINUS:
				case KEY_MINUS+KEY_SHIFTED:
				case KEY_PADMINUS:
					item[choice].value -= 1;
					break;
				case KEY_RIGHT:
				case KEY_PAD6:
				case KEY_EQUAL:
				case KEY_EQUAL+KEY_SHIFTED:
				case KEY_PADPLUS:
					item[choice].value++;
					break;
				case KEY_PAGEUP:
				case KEY_PAD9:
				case KEY_SPACEBAR:
					item[choice].value += 10;
					break;
				case KEY_PAGEDOWN:
				case KEY_BACKSP:
				case KEY_PAD3:
					item[choice].value -= 10;
					break;
				}
				if (ov!=item[choice].value)
					item[choice].redraw=1;
			}
	
		}

		gr_set_current_canvas(bg.menu_canvas);
#ifndef OGL
		// Redraw everything...
		for (i=0; i<nitems; i++ )	{
			if (item[i].redraw)	{
                                draw_item( &bg, &item[i], (i==choice && !all_text), TinyMode );
				item[i].redraw=0;
#ifdef NEWMENU_MOUSE
				newmenu_show_cursor();
#endif
			}
			else if (i==choice && (item[i].type==NM_TYPE_INPUT || (item[i].type==NM_TYPE_INPUT_MENU && item[i].group)))
				update_cursor( &item[i]);
		}
#else
		// Redraw everything...
		for (i=0; i<nitems; i++ )	{
			draw_item( &bg, &item[i], (i==choice && !all_text), TinyMode );
#ifdef NEWMENU_MOUSE
			newmenu_show_cursor();
#endif
			if (i==choice && (item[i].type==NM_TYPE_INPUT || (item[i].type==NM_TYPE_INPUT_MENU && item[i].group)))
				update_cursor( &item[i]);
		}
#endif
		gr_update();

		if ( gr_palette_faded_out )	{
			gr_palette_fade_in( gr_palette, 32, 0 );
		}
	}

	newmenu_hide_cursor();

#ifndef OGL
	// Restore everything...
	gr_set_current_canvas(bg.menu_canvas);
	if ( filename == NULL )	{
		// Save the background under the menu...
		gr_bitmap(0, 0, bg.saved); 	
		gr_free_bitmap(bg.saved);
		if (bg.background_is_sub)
			gr_free_sub_bitmap( bg.background );
		else
			gr_free_bitmap( bg.background );
	} else {
		gr_bitmap(0, 0, bg.background); 	
		gr_free_bitmap(bg.background);
	}
#endif

	gr_free_sub_canvas( bg.menu_canvas );
	gr_set_current_canvas( NULL );
	grd_curcanv->cv_font	= save_font;
	gr_set_current_canvas( save_canvas );
	keyd_repeat = old_keyd_repeat;

	newmenu_close();

	game_flush_inputs();

	if (time_stopped)
		start_time();

	if ( sound_stopped )
		digi_resume_all();

	return choice;

}

int nm_messagebox1( char *title, void (*subfunction)(int nitems,newmenu_item * items, int * last_key, int citem), int nchoices, ... )
{
	int i;
	char * format;
	va_list args;
	char *s;
	char nm_text[MESSAGEBOX_TEXT_SIZE];
	newmenu_item nm_message_items[5];

	va_start(args, nchoices );

	Assert( nchoices <= 5 );

	for (i=0; i<nchoices; i++ )	{
		s = va_arg( args, char * );
		nm_message_items[i].type = NM_TYPE_MENU; nm_message_items[i].text = s;
	}
	format = va_arg( args, char * );
	//sprintf(	  nm_text, "" ); // adb: ?
	vsprintf(nm_text,format,args);
	va_end(args);

	Assert(strlen(nm_text) < MESSAGEBOX_TEXT_SIZE);

	return newmenu_do( title, nm_text, nchoices, nm_message_items, subfunction );
}

int nm_messagebox( char *title, int nchoices, ... )
{
	int i;
	char * format;
	va_list args;
	char *s;
	char nm_text[MESSAGEBOX_TEXT_SIZE];
	newmenu_item nm_message_items[5];

	va_start(args, nchoices );

	Assert( nchoices <= 5 );

	for (i=0; i<nchoices; i++ )	{
		s = va_arg( args, char * );
		nm_message_items[i].type = NM_TYPE_MENU; nm_message_items[i].text = s;
	}
	format = va_arg( args, char * );
	//sprintf(	  nm_text, "" ); // adb: ?
	vsprintf(nm_text,format,args);
	va_end(args);

	Assert(strlen(nm_text) < MESSAGEBOX_TEXT_SIZE );

	return newmenu_do( title, nm_text, nchoices, nm_message_items, NULL );
}

void newmenu_file_sort( int n, char *list )
{
	int i, j, incr;
	char t[14];

	incr = n / 2;
	while( incr > 0 ) {
		for (i=incr; i<n; i++ ) {
			j = i - incr;
			while (j>=0 ) {
				if (strncmp(&list[j*14], &list[(j+incr)*14], 12) > 0) {
					memcpy( t, &list[j*14], 13 );
					memcpy( &list[j*14], &list[(j+incr)*14], 13 );
					memcpy( &list[(j+incr)*14], t, 13 );
					j -= incr;
				}
				else
					break;
			}
		}
		incr = incr / 2;
	}
}

void delete_player_saved_games(char * name)
{
	int i;
	char filename[16];
	
	for (i=0;i<10; i++)	{
		sprintf( filename, "%s.sg%d", name, i );
		remove( filename );
	}
}

#define MAX_FILES 100

int MakeNewPlayerFile(int allow_abort);

int newmenu_get_filename( char * title, char * filespec, char * filename, int allow_abort_flag )
{
	int i;
	d_glob_t glob_ret;
	int NumFiles=0, key,done, citem, ocitem;
	char * filenames = NULL;
	int NumFiles_displayed = 8;
	int first_item = -1, ofirst_item;
	int old_keyd_repeat = keyd_repeat;
	int player_mode=0;
	int demo_mode=0;
	int demos_deleted=0;
	int initialized = 0;
	int exit_value = 0;
	int w_x, w_y, w_w, w_h, title_height;
	int box_x, box_y, box_w, box_h;
#ifndef OGL
	bkg bg; // background under listbox
#endif
#ifdef NEWMENU_MOUSE
	int mx, my, mz, x1, x2, y1, y2, mouse_state, omouse_state;
	int mouse2_state, omouse2_state;
	int dblclick_flag=0;
#endif

	filenames = malloc( MAX_FILES * (14+sizeof(DEMO_DIR)) );
	if (filenames==NULL) return 0;

	citem = 0;
	keyd_repeat = 1;
	w_x=w_y=w_w=w_h=title_height=0;
	box_x=box_y=box_w=box_h=0;

	if (!strcasecmp( filespec, "*.plr" ))
		player_mode = 1;
	else if (!strcasecmp( filespec, DEMO_DIR "*.dem" ))
		demo_mode = 1;

ReadFileNames:
	done = 0;
	NumFiles=0;

	if (player_mode)	{
		strncpy( &filenames[NumFiles*14], TXT_CREATE_NEW, 13 );
		NumFiles++;
	}

	if (!d_glob(filespec, &glob_ret)) {
		int j;
		for (j = 0; j < glob_ret.gl_pathc; j++) {
			if (NumFiles >= MAX_FILES)
				break;
			if (demo_mode) { // special stuff for demos
#ifndef __WINDOWS__
				char * buf;
				buf = malloc( MAX_FILES * (14+sizeof(DEMO_DIR)) );
				// increase our buffer so we have enough space for DEMO_DIR
				strncpy(buf, glob_ret.gl_pathv[j], 13+sizeof(DEMO_DIR));
				// now substract DEMO_DIR string so it does not display in menu and does not return. However DEMO_DIR passed later before calling newdemo_start_playback in menu.c
				strncpy(filenames+(NumFiles*14),buf+sizeof(DEMO_DIR)-1, 13+sizeof(DEMO_DIR));
				free(buf);
#else				// while on Windows d_glob does not return the path, so we don't need that
				strncpy(&filenames[NumFiles*14], glob_ret.gl_pathv[j], 13);
#endif
			}
			else if (player_mode) {
				char *p;
				strncpy(&filenames[NumFiles*14], glob_ret.gl_pathv[j], 13);
				p = strrchr(&filenames[NumFiles*14], '.');
				if (p) *p = '\0';
			}
			NumFiles++;
		}
		d_globfree(&glob_ret);
        }

	if ( (NumFiles < 1) && demos_deleted )	{
		exit_value = 0;
		goto ExitFileMenu;
	}
	if ( (NumFiles < 1) && demo_mode ) {
		nm_messagebox( NULL, 1, TXT_OK, "%s %s\n%s", TXT_NO_DEMO_FILES, TXT_USE_F5, TXT_TO_CREATE_ONE);
		exit_value = 0;
		goto ExitFileMenu;
	}

	if ( (NumFiles < 2) && player_mode ) {
		citem = 0;
		goto ExitFileMenuEarly;
	}

	if ( NumFiles<1 )	{
		nm_messagebox( NULL, 1, "Ok", "%s\n '%s' %s", TXT_NO_FILES_MATCHING, filespec, TXT_WERE_FOUND);
		exit_value = 0;
		goto ExitFileMenu;
	}

	if (!initialized) {	
		set_screen_mode(SCREEN_MENU);

		gr_set_current_canvas(NULL);

		grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_3];

		w_w = 0;
		w_h = 0;

		for (i=0; i<NumFiles; i++ ) {
			int w, h, aw;
			gr_get_string_size( &filenames[i*14], &w, &h, &aw );		
			if ( w > w_w )
				w_w = w;
		}
		if ( title ) {
			int w, h, aw;
			gr_get_string_size( title, &w, &h, &aw );		
			if ( w > w_w )
				w_w = w;
#ifdef OGL
			if (GameArg.OglFixedFont)
#endif
				h += 10*MENSCALE_Y;
			title_height = h + FONTSCALE_Y(grd_curcanv->cv_font->ft_h*2);		// add a little space at the bottom of the title
		}

		box_w = w_w;
		box_h = ((FONTSCALE_Y(grd_curcanv->cv_font->ft_h + 2)) * NumFiles_displayed);

#ifdef OGL
		if (GameArg.OglFixedFont)
#endif
			w_w += (grd_curcanv->cv_font->ft_w * 2)*MENSCALE_X;
#ifdef OGL
		else
			w_w += FONTSCALE_X(grd_curcanv->cv_font->ft_w * 4);
#endif
		w_h = title_height + box_h + FONTSCALE_Y(grd_curcanv->cv_font->ft_h * 2)+(5*MENSCALE_Y);		// more space at bottom

		if ( w_w > GWIDTH ) w_w = GWIDTH;
		if ( w_h > GHEIGHT ) w_h = GHEIGHT;
	
		w_x = (GWIDTH-w_w)/2;
		w_y = (GHEIGHT-w_h)/2;
	
		if ( w_x < 0 ) w_x = 0;
		if ( w_y < 0 ) w_y = 0;

#ifdef OGL
		if (GameArg.OglFixedFont)
#endif
			box_x = w_x + (grd_curcanv->cv_font->ft_w)*MENSCALE_X;
#ifdef OGL
		else
			box_x = w_x + FONTSCALE_X(grd_curcanv->cv_font->ft_w)*2;			// must be in sync with w_w!!!
#endif
		box_y = w_y + title_height;


#ifndef OGL
		// save the screen behind the menu.
		bg.saved = NULL;

		bg.background = gr_create_bitmap( w_w, w_h );

		Assert( bg.background != NULL );

		gr_bm_bitblt(w_w, w_h, 0, 0, w_x, w_y, &grd_curcanv->cv_bitmap, bg.background );

		nm_draw_background( w_x,w_y,w_x+w_w-1,w_y+w_h );
		
		gr_string( 0x8000, w_y+(10*MENSCALE_Y), title );
#endif
		initialized = 1;
	}

	if ( !player_mode )	{
		newmenu_file_sort( NumFiles, filenames );
	} else {
		newmenu_file_sort( NumFiles-1, &filenames[14] );		// Don't sort first one!
		for ( i=0; i<NumFiles; i++ )	{
			if (!strcasecmp(Players[Player_num].callsign, &filenames[i*14]) ) {
#ifdef NEWMENU_MOUSE
				dblclick_flag = 1;
#endif
				citem = i;
			}
		}
	}

#ifdef NEWMENU_MOUSE
	mouse_state = omouse_state = 0;
	mouse2_state = omouse2_state = 0;
	newmenu_show_cursor();
#endif

	while(!done)	{
		timer_delay(400);
		ocitem = citem;
		ofirst_item = first_item;
                gr_update();

#ifdef OGL
		gr_flip();
		nm_draw_background1(NULL);
		nm_draw_background( w_x,w_y,w_x+w_w-1,w_y+w_h );
		grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_3];
		gr_string( 0x8000, w_y+(10*MENSCALE_Y), title );
#endif

#ifdef NEWMENU_MOUSE
		draw_close_box(w_x,w_y);
		omouse_state = mouse_state;
		omouse2_state = mouse2_state;
		mouse_state = mouse_button_state(0);
		mouse2_state = mouse_button_state(1);
#endif

		key = key_inkey();

		switch(key) {
		case KEY_PRINT_SCREEN: 		save_screen_shot(0); break;
		case KEY_CTRLED+KEY_D:
			if ( ((player_mode)&&(citem>0)) || ((demo_mode)&&(citem>=0)) )	{
				int x = 1;
				newmenu_hide_cursor();
				if (player_mode)
					x = nm_messagebox( NULL, 2, TXT_YES, TXT_NO, "%s %s?", TXT_DELETE_PILOT, &filenames[citem*14]+((player_mode && filenames[citem*14]=='$')?1:0) );
				else if (demo_mode)
					x = nm_messagebox( NULL, 2, TXT_YES, TXT_NO, "%s %s?", TXT_DELETE_DEMO, &filenames[citem*14]+((demo_mode && filenames[citem*14]=='$')?1:0) );
				newmenu_show_cursor();
 				if (x==0)	{
					char * p;
					char plxfile[256], efffile[256];
					int ret=0;
					p = &filenames[(citem*14)+strlen(&filenames[citem*14])];
					if (player_mode) {
						*p = '.';
						ret = unlink( &filenames[citem*14] );
					} else if (demo_mode) {
						char demo_full[16 + sizeof(DEMO_DIR)];
						strcpy(demo_full,DEMO_DIR);
						strcat(demo_full,filenames+(citem*14));
						ret = unlink( demo_full );
					}
					if (player_mode)
						*p = 0;

					if ((!ret) && player_mode)	{
						delete_player_saved_games( &filenames[citem*14] );
						// delete PLX and EFF files
						sprintf(plxfile,"%.8s.plx",&filenames[citem*14]);
						if (cfexist(plxfile))
							remove(plxfile);
						sprintf(efffile,"%.8s.eff",&filenames[citem*14]);
						if (cfexist(efffile))
							remove(efffile);
					}

					if (ret) {
						if (player_mode)
							nm_messagebox( NULL, 1, TXT_OK, "%s %s %s", TXT_COULDNT, TXT_DELETE_PILOT, &filenames[citem*14]+((player_mode && filenames[citem*14]=='$')?1:0) );
						else if (demo_mode)
							nm_messagebox( NULL, 1, TXT_OK, "%s %s %s", TXT_COULDNT, TXT_DELETE_DEMO, &filenames[citem*14]+((demo_mode && filenames[citem*14]=='$')?1:0) );
					} else if (demo_mode)
						demos_deleted = 1;
					first_item = -1;
					goto ReadFileNames;
				}
			}
			break;
		case KEY_HOME:
		case KEY_PAD7:
			citem = 0;
			break;
		case KEY_END:
		case KEY_PAD1:
			citem = NumFiles-1;
			break;
		case KEY_UP:
		case KEY_PAD8:
			citem--;
			break;
		case KEY_DOWN:
		case KEY_PAD2:
			citem++;
			break;
 		case KEY_PAGEDOWN:
		case KEY_PAD3:
			citem += NumFiles_displayed;
			break;
		case KEY_PAGEUP:
		case KEY_PAD9:
			citem -= NumFiles_displayed;
			break;
		case KEY_ESC:
			if (allow_abort_flag) {
				citem = -1;
				done = 1;
			}
			break;
		case KEY_ENTER:
		case KEY_PADENTER:
#ifdef GP2X
		case GP2X_BUTTON_B:
#endif
			done = 1;
			break;

		case KEYS_GR_TOGGLE_FULLSCREEN:
			gr_toggle_fullscreen();
			break;

		default:	
			{
				int ascii = key_to_ascii(key);
				if ( ascii < 255 )	{
					int cc,cc1;
					cc=cc1=citem+1;
					if (cc1 < 0 )  cc1 = 0;
					if (cc1 >= NumFiles )  cc1 = 0;
					while(1) {
						if ( cc < 0 ) cc = 0;
						if ( cc >= NumFiles ) cc = 0;
						if ( citem == cc ) break;
	
						if ( toupper(filenames[cc*14]) == toupper(ascii) )	{
							citem = cc;
							break;
						}
						cc++;
					}
				}
			}
		}
		if ( done ) break;


		if (citem<0)
			citem=0;

		if (citem>=NumFiles)
			citem = NumFiles-1;

		if (citem< first_item)
			first_item = citem;

		if (citem>=( first_item+NumFiles_displayed))
			first_item = citem-NumFiles_displayed+1;

		if (NumFiles <= NumFiles_displayed )
			 first_item = 0;

		if (first_item>NumFiles-NumFiles_displayed)
			first_item = NumFiles-NumFiles_displayed;
		if (first_item < 0 ) first_item = 0;

#ifdef NEWMENU_MOUSE
		if (mouse_state || mouse2_state) {
			int w, h, aw;

			mouse_get_pos(&mx, &my, &mz);
			for (i=first_item; i<first_item+NumFiles_displayed; i++ )	{
				gr_get_string_size(&filenames[i*14], &w, &h, &aw  );
				x1 = box_x;
				x2 = box_x + box_w - 1;
				y1 = (i-first_item)*FONTSCALE_Y(grd_curcanv->cv_font->ft_h + 2) + box_y;
				y2 = y1+h+1;
				if ( ((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) ) {
					if (i == citem && !mouse2_state) {
						break;
					}
					citem = i;
					dblclick_flag = 0;
					break;
				}
			}
		}
		
		if (!mouse_state && omouse_state) {
			int w, h, aw;

			gr_get_string_size(&filenames[citem*14], &w, &h, &aw  );
			mouse_get_pos(&mx, &my, &mz);
			x1 = box_x;
			x2 = box_x + box_w - 1;
			y1 = (citem-first_item)*FONTSCALE_Y(grd_curcanv->cv_font->ft_h + 2) + box_y;
			y2 = y1+h+1;
			if ( ((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) ) {
				if (dblclick_flag) done = 1;
				else dblclick_flag = 1;
			}
		}

		if ( !mouse_state && omouse_state ) {
			mouse_get_pos(&mx, &my, &mz);
			x1 = w_x + CLOSE_X + 2;
			x2 = x1 + CLOSE_SIZE - 2;
			y1 = w_y + CLOSE_Y + 2;
			y2 = y1 + CLOSE_SIZE - 2;
			if ( ((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) ) {
				citem = -1;
				done = 1;
			}
		}

#endif

#ifndef OGL
		gr_setcolor( BM_XRGB( 0,0,0)  );

		if (ofirst_item != first_item)	{
			newmenu_hide_cursor();
#endif
			gr_setcolor( BM_XRGB( 0,0,0)  );
			for (i=first_item; i<first_item+NumFiles_displayed; i++ )	{
				int w, h, aw, y;
				y = (i-first_item)*FONTSCALE_Y(grd_curcanv->cv_font->ft_h + 2) + box_y;
			
				if ( i >= NumFiles )	{

					gr_setcolor( BM_XRGB(5,5,5));
					gr_rect( box_x + box_w, y-1, box_x + box_w, y + FONTSCALE_Y(grd_curcanv->cv_font->ft_h + 2));
					
					gr_setcolor( BM_XRGB(2,2,2));
					gr_rect( box_x - 1, y - 1, box_x - 1, y + FONTSCALE_Y(grd_curcanv->cv_font->ft_h + 2) );
					
					gr_setcolor( BM_XRGB(0,0,0));
					gr_rect( box_x, y - 1, box_x + box_w - 1, y + FONTSCALE_Y(grd_curcanv->cv_font->ft_h + 2));
					
				} else {
					if ( i == citem )	
						grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_2];
					else	
						grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_1];
					gr_get_string_size(&filenames[i*14], &w, &h, &aw  );

					gr_setcolor( BM_XRGB(5,5,5));
					gr_rect( box_x + box_w, y - 1, box_x + box_w, y + h + FONTSCALE_Y(2));
					
					gr_setcolor( BM_XRGB(2,2,2));
					gr_rect( box_x - 1, y - 1, box_x - 1, y + h + FONTSCALE_Y(2));
					gr_setcolor( BM_XRGB(0,0,0));
							
					gr_rect( box_x, y-1, box_x + box_w - 1, y + h + FONTSCALE_Y(2) );
					gr_string( box_x + 5, y, (&filenames[i*14])+((player_mode && filenames[i*14]=='$')?1:0)  );
				}
			}
#ifndef OGL
			newmenu_show_cursor();
		} else if ( citem != ocitem )	{
			int w, h, aw, y;

			newmenu_hide_cursor();
			i = ocitem;
			if ( (i>=0) && (i<NumFiles) )	{
				y = (i-first_item)*FONTSCALE_Y(grd_curcanv->cv_font->ft_h+2)+box_y;
				if ( i == citem )	
					grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_2];
				else	
					grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_1];
				gr_get_string_size(&filenames[i*14], &w, &h, &aw  );
				gr_rect( box_x, y-1, box_x + box_w - 1, y + h + FONTSCALE_Y(2) );
				gr_string( box_x + 5, y, (&filenames[i*14])+((player_mode && filenames[i*14]=='$')?1:0)  );
			}
			i = citem;
			if ( (i>=0) && (i<NumFiles) )	{
				y = (i-first_item)*FONTSCALE_Y(grd_curcanv->cv_font->ft_h+2)+box_y;
				if ( i == citem )	
					grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_2];
				else	
					grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_1];
				gr_get_string_size(&filenames[i*14], &w, &h, &aw  );
				gr_string( box_x + 5, y, (&filenames[i*14])+((player_mode && filenames[i*14]=='$')?1:0)  );
			}
			newmenu_show_cursor();
		}
#endif
		if ( gr_palette_faded_out )	{
			gr_palette_fade_in( gr_palette, 32, 0 );
		}
        }

	newmenu_close();

#ifdef NEWMENU_MOUSE
	newmenu_hide_cursor();
#endif

ExitFileMenuEarly:
	if ( citem > -1 )	{
		strncpy( filename, (&filenames[citem*14])+((player_mode && filenames[citem*14]=='$')?1:0), 13 );
		exit_value = 1;
	} else {
		exit_value = 0;
	}

ExitFileMenu:
	keyd_repeat = old_keyd_repeat;

	if ( filenames )
		free(filenames);

	return exit_value;

}


// Example listbox callback function...
// int lb_callback( int * citem, int *nitems, char * items[], int *keypress )
// {
// 	int i;
// 
// 	if ( *keypress = KEY_CTRLED+KEY_D )	{
// 		if ( *nitems > 1 )	{
// 			unlink( items[*citem] );		// Delete the file
// 			for (i=*citem; i<*nitems-1; i++ )	{
// 				items[i] = items[i+1];
// 			}
// 			*nitems = *nitems - 1;
// 			free( items[*nitems] );
// 			items[*nitems] = NULL;
// 			return 1;	// redraw;
// 		}
//			*keypress = 0;
// 	}			
// 	return 0;
// }

#define LB_ITEMS_ON_SCREEN 8

int newmenu_listbox( char * title, int nitems, char * items[], int allow_abort_flag, int (*listbox_callback)( int * citem, int *nitems, char * items[], int *keypress ) )
{
	return newmenu_listbox1( title, nitems, items, allow_abort_flag, 0, listbox_callback );
}

int newmenu_listbox1( char * title, int nitems, char * items[], int allow_abort_flag, int default_item, int (*listbox_callback)( int * citem, int *nitems, char * items[], int *keypress ) )
{
	int i;
	int done, ocitem,citem, ofirst_item, first_item, key, redraw;
	int old_keyd_repeat = keyd_repeat;
	int width, height, wx, wy, title_height;
	int font_height,font_height1;
	keyd_repeat = 1;
	bkg bg;
#ifdef NEWMENU_MOUSE
	int mx, my, mz, x1, x2, y1, y2, mouse_state, omouse_state;	//, dblclick_flag;
	int close_x,close_y;
#endif

	set_screen_mode(SCREEN_MENU);
	gr_set_current_canvas(NULL);
	grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_3];

	width = 0;
	for (i=0; i<nitems; i++ )	{
		int w, h, aw;
		gr_get_string_size( items[i], &w, &h, &aw );
		if ( w > width )
			width = w;
	}
	font_height=FONTSCALE_Y(Gamefonts[GFONT_MEDIUM_1]->ft_h)+2;
	font_height1=font_height-1;
	height = font_height * LB_ITEMS_ON_SCREEN; // was 12*

	{
		int w, h, aw;
		gr_get_string_size( title, &w, &h, &aw );
		if ( w > width )
			width = w;
		title_height = h + (5*MENSCALE_Y);
	}
		
	width += 10;
	if ( width > grd_curcanv->cv_bitmap.bm_w - 30 )//was 320 -
		width = grd_curcanv->cv_bitmap.bm_w - 30;
	
	wx = (grd_curcanv->cv_bitmap.bm_w-width)/2;
	wy = (grd_curcanv->cv_bitmap.bm_h-(height+title_height))/2 + title_height;
	if ( wy < title_height )
		wy = title_height;

	bg.saved = NULL;
	bg.background = gr_create_bitmap(grd_curcanv->cv_bitmap.bm_w, grd_curcanv->cv_bitmap.bm_h);
#ifndef OGL
 	gr_bm_bitblt(grd_curcanv->cv_bitmap.bm_w, grd_curcanv->cv_bitmap.bm_h, 0, 0, 0, 0, &grd_curcanv->cv_bitmap, bg.background );
 	nm_draw_background( wx-(15*MENSCALE_X),wy-title_height-(15*MENSCALE_Y),wx+width+(15*MENSCALE_X),wy+height+(15*MENSCALE_Y) );

	gr_string( 0x8000, wy - title_height, title );
#endif

	done = 0;
	citem = default_item;
	if ( citem < 0 ) citem = 0;
	if ( citem >= nitems ) citem = 0;

	first_item = -1;

#ifdef NEWMENU_MOUSE
	mouse_state = omouse_state = 0;
	close_x = wx-(15*MENSCALE_X);
	close_y = wy-title_height-(15*MENSCALE_Y);
	newmenu_show_cursor();
#endif

	while(!done)	{
		timer_delay(400);
#ifdef OGL
		gr_flip();
		nm_draw_background1(NULL);
		nm_draw_background( wx-(15*MENSCALE_X),wy-title_height-(15*MENSCALE_Y),wx+width+(15*MENSCALE_X),wy+height+(15*MENSCALE_Y) );
		grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_3];
		gr_string( 0x8000, wy - title_height, title );
#endif

		ocitem = citem;
		ofirst_item = first_item;

#ifdef NEWMENU_MOUSE
		draw_close_box(close_x,close_y);
		omouse_state = mouse_state;
		mouse_state = mouse_button_state(0);
#endif

		key = key_inkey();

		if ( listbox_callback )
			redraw = (*listbox_callback)(&citem, &nitems, items, &key );
		else
			redraw = 0;

		if ( key<-1 ) {
			citem = key;
			key = -1;
			done = 1;
		}

		switch(key)	{
		case KEY_PRINT_SCREEN: 		
			save_screen_shot(0); 
			break;
		case KEY_HOME:
		case KEY_PAD7:
			citem = 0;
			break;
		case KEY_END:
		case KEY_PAD1:
			citem = nitems-1;
			break;
		case KEY_UP:
		case KEY_PAD8:
			citem--;
			break;
		case KEY_DOWN:
		case KEY_PAD2:
			citem++;
			break;
		case KEY_PAGEDOWN:
		case KEY_PAD3:
			citem += LB_ITEMS_ON_SCREEN;
			break;
		case KEY_PAGEUP:
		case KEY_PAD9:
			citem -= LB_ITEMS_ON_SCREEN;
			break;
		case KEY_ESC:
			if (allow_abort_flag) {
				citem = -1;
				done = 1;
			}
			break;
		case KEY_ENTER:
		case KEY_PADENTER:
#ifdef GP2X
		case GP2X_BUTTON_B:
#endif
			done = 1;
			break;

		case KEYS_GR_TOGGLE_FULLSCREEN:
			gr_toggle_fullscreen();
			break;

		default:	
			if ( key > 0 )	{
				int ascii = key_to_ascii(key);
				if ( ascii < 255 )	{
					int cc,cc1;
					cc=cc1=citem+1;
					if (cc1 < 0 )  cc1 = 0;
					if (cc1 >= nitems )  cc1 = 0;
					while(1) {
						if ( cc < 0 ) cc = 0;
						if ( cc >= nitems ) cc = 0;
						if ( citem == cc ) break;
	
						if ( toupper( items[cc][0] ) == toupper(ascii) )	{
							citem = cc;
							break;
						}
						cc++;
					}
				}
			}
		}
		if ( done ) break;

		if (citem<0)
			citem=0;

		if (citem>=nitems)
			citem = nitems-1;

		if (citem< first_item)
			first_item = citem;

		if (citem>=( first_item+LB_ITEMS_ON_SCREEN))
			first_item = citem-LB_ITEMS_ON_SCREEN+1;

		if (nitems <= LB_ITEMS_ON_SCREEN )
			 first_item = 0;

		if (first_item>nitems-LB_ITEMS_ON_SCREEN)
			first_item = nitems-LB_ITEMS_ON_SCREEN;
		if (first_item < 0 ) first_item = 0;

#ifdef NEWMENU_MOUSE
		if (mouse_state) {
			int w, h, aw;

			mouse_get_pos(&mx, &my, &mz);
			for (i=first_item; i<first_item+LB_ITEMS_ON_SCREEN; i++ )	{
				if (i > nitems)
					break;
				gr_get_string_size(items[i], &w, &h, &aw  );
				x1 = wx;
				x2 = wx + width;
				y1 = (i-first_item)*FONTSCALE_Y(grd_curcanv->cv_font->ft_h+2)+wy;
				y2 = y1+h+1;
				if ( ((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) ) {
					citem = i;
					done = 1;
					break;
				}
			}
		}

		//check for close box clicked
		if ( !mouse_state && omouse_state ) {
			mouse_get_pos(&mx, &my, &mz);
			x1 = close_x + CLOSE_X + 2;
			x2 = x1 + CLOSE_SIZE - 2;
			y1 = close_y + CLOSE_Y + 2;
			y2 = y1 + CLOSE_SIZE - 2;
			if ( ((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) ) {
				citem = -1;
				done = 1;
			}
		}
#endif

#ifndef OGL
		if ((ofirst_item != first_item) || redraw)
		{
			newmenu_hide_cursor();
#endif
			gr_setcolor( BM_XRGB( 0,0,0)  );
			for (i=first_item; i<first_item+LB_ITEMS_ON_SCREEN; i++ )	{
				int w, h, aw, y;
				y = (i-first_item)*font_height+wy;//was *12
				if ( i >= nitems )	{
					gr_setcolor( BM_XRGB(0,0,0));
					gr_rect( wx, y-FONTSCALE_Y(1), wx+width-FONTSCALE_X(1), y+font_height1 );//was +11
				} else {
					if ( i == citem )	
						grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_2];
					else	
						grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_1];
					gr_get_string_size(items[i], &w, &h, &aw  );
					gr_rect( wx, y-FONTSCALE_Y(1), wx+width-FONTSCALE_X(1), y+font_height1 );//was +11
					gr_string( wx+FONTSCALE_X(5), y, items[i]  );
				}
			}		
#ifndef OGL
			newmenu_show_cursor();
			gr_update();
		} else if ( citem != ocitem )	{
			int w, h, aw, y;

			newmenu_hide_cursor();
			i = ocitem;
			gr_setcolor( BM_XRGB(0,0,0));
			if ( (i>=0) && (i<nitems) )	{
				y = (i-first_item)*font_height+wy;//was *12
				if ( i == citem )	
					grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_2];
				else	
					grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_1];
				gr_get_string_size(items[i], &w, &h, &aw  );
				gr_rect( wx, y-1, wx+width-1, y+font_height1 );//was +11
				gr_string( wx+5, y, items[i]  );
			}
			i = citem;
			if ( (i>=0) && (i<nitems) )	{
				y = (i-first_item)*font_height+wy;//was *12
				if ( i == citem )	
					grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_2];
				else	
					grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_1];
				gr_get_string_size( items[i], &w, &h, &aw  );
				gr_rect( wx, y-1, wx+width-1, y+font_height1 );//was +11
				gr_string( wx+5, y, items[i]  );
				newmenu_show_cursor();
				gr_update();
			}
		}
#endif
	}
	newmenu_hide_cursor();
	keyd_repeat = old_keyd_repeat;

#ifndef OGL
	gr_bm_bitblt(grd_curcanv->cv_bitmap.bm_w, grd_curcanv->cv_bitmap.bm_h, 0, 0, 0, 0, bg.background, &grd_curcanv->cv_bitmap );
#endif 
	if ( bg.background != &VR_offscreen_buffer->cv_bitmap )
		gr_free_bitmap(bg.background);

	newmenu_close();

	return citem;
}


int newmenu_filelist( char * title, char * filespec, char * filename )
{
	int i, NumFiles;
	char * Filenames[MAX_FILES];
	char FilenameText[MAX_FILES][14];
	d_glob_t glob_ret;

	NumFiles = 0;
	if ( !d_glob( filespec, &glob_ret ) ) {
		NumFiles = glob_ret.gl_pathc < MAX_FILES ? glob_ret.gl_pathc : MAX_FILES;
		for (i = 0; i < NumFiles; i++) {
			strncpy( FilenameText[i], glob_ret.gl_pathv[i], 13 );
			Filenames[i] = FilenameText[i];
		}
		d_globfree(&glob_ret);
	}

	i = newmenu_listbox( title, NumFiles, Filenames, 1, NULL );
	if ( i > -1 )	{
		strcpy( filename, Filenames[i] );
		return 1;
	} 
	return 0;
}

//added on 10/14/98 by Victor Rachels to attempt a fixedwidth font messagebox
int nm_messagebox_fixedfont( char *title, int nchoices, ... )
{
	int i;
	char * format;
	va_list args;
	char *s;
	char nm_text[MESSAGEBOX_TEXT_SIZE];
	newmenu_item nm_message_items[5];

	va_start(args, nchoices );

	Assert( nchoices <= 5 );

	for (i=0; i<nchoices; i++ )	{
		s = va_arg( args, char * );
		nm_message_items[i].type = NM_TYPE_MENU; nm_message_items[i].text = s;
	}
	format = va_arg( args, char * );
	//sprintf(	  nm_text, "" ); // adb: ?
	vsprintf(nm_text,format,args);
	va_end(args);

	Assert(strlen(nm_text) < MESSAGEBOX_TEXT_SIZE );

        return newmenu_do_fixedfont( title, nm_text, nchoices, nm_message_items, NULL, 0, NULL, -1, -1 );
}

//end this section addition - Victor Rachels
