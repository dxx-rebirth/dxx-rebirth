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
 * Routines for menus.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#if !(defined(__APPLE__) && defined(__MACH__))
#include <physfs.h>
#else
#include <physfs/physfs.h>
#endif

#include "error.h"
#include "pstypes.h"
#include "gr.h"
#include "songs.h"
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
#include "timer.h"
#include "vers_id.h"
#include "automap.h"
#include "menu.h"
#include "playsave.h"

#ifdef OGL
#include "ogl_init.h"
#endif

grs_bitmap nm_background, nm_background1;
grs_bitmap *nm_background_sub;
ubyte SurfingNet=0;
ubyte MenuReordering=0;

#define MAXDISPLAYABLEITEMS 14
#define MESSAGEBOX_TEXT_SIZE 2176   // How many characters in messagebox
#define MAX_TEXT_WIDTH 	FSPACX(120) // How many pixels wide a input box can be

extern void game_do_render_frame(int flip);

void newmenu_close()	{
	if (nm_background.bm_data)
	{
		gr_free_sub_bitmap(nm_background_sub);
		gr_free_bitmap_data (&nm_background);
	}
	if (nm_background1.bm_data)
		gr_free_bitmap_data (&nm_background1);
}

// Draw Copyright and Version strings
void nm_draw_copyright()
{
	gr_set_curfont(GAME_FONT);
	gr_set_fontcolor(BM_XRGB(6,6,6),-1);
	gr_printf(0x8000,SHEIGHT-LINE_SPACING,TXT_COPYRIGHT);
	gr_set_fontcolor( BM_XRGB(25,0,0), -1);
	gr_printf(0x8000,SHEIGHT-(LINE_SPACING*2),DESCENT_VERSION);
}

// Draws the background of menus (i.e. Descent Logo screen)
void nm_draw_background1(char * filename)
{
	int pcx_error;

	if (filename == NULL && Function_mode == FMODE_MENU)
		filename = Menu_pcx_name;
	else if (filename == NULL && Function_mode == FMODE_GAME)
	{
		if (Automap_active)
			draw_automap(0);
		else
			game_do_render_frame(0);
		gr_set_current_canvas(NULL);
	}

	if (filename != NULL)
	{
		if (nm_background1.bm_data == NULL) {
			gr_init_bitmap_data (&nm_background1);
			pcx_error = pcx_read_bitmap( filename, &nm_background1, BM_LINEAR, gr_palette );
			Assert(pcx_error == PCX_ERROR_NONE);
		}
		gr_palette_load( gr_palette );
		show_fullscr(&nm_background1);

		if (!strcmp(filename,Menu_pcx_name))
			nm_draw_copyright();
	}
}

#define MENU_BACKGROUND_BITMAP_HIRES (cfexist("scoresb.pcx")?"scoresb.pcx":"scores.pcx")
#define MENU_BACKGROUND_BITMAP_LORES (cfexist("scores.pcx")?"scores.pcx":"scoresb.pcx")
#define MENU_BACKGROUND_BITMAP (SWIDTH>=640&&SHEIGHT>=480?MENU_BACKGROUND_BITMAP_HIRES:MENU_BACKGROUND_BITMAP_LORES)

// Draws the frame background for menus
void nm_draw_background(int x1, int y1, int x2, int y2 )
{
	int w,h,init_sub=0;
	float BGScaleX=((float)SWIDTH/320),BGScaleY=((float)SHEIGHT/200);
	grs_canvas *tmp,*old;

	if (nm_background.bm_data == NULL)
	{
		int pcx_error;
		ubyte background_palette[768];
		gr_init_bitmap_data (&nm_background);
		pcx_error = pcx_read_bitmap(MENU_BACKGROUND_BITMAP,&nm_background,BM_LINEAR,background_palette);
		Assert(pcx_error == PCX_ERROR_NONE);
		gr_remap_bitmap_good( &nm_background, background_palette, -1, -1 );
		init_sub=1;
	}

	if ( x1 < 0 ) x1 = 0;
	if ( y1 < 0 ) y1 = 0;
	if ( x2 > SWIDTH - 1) x2 = SWIDTH - 1;
	if ( y2 > SHEIGHT - 1) y2 = SHEIGHT - 1;

	w = x2-x1;
	h = y2-y1;

	old=grd_curcanv;
	tmp=gr_create_sub_canvas(old,x1,y1,w,h);
	gr_set_current_canvas(tmp);
	gr_palette_load( gr_palette );

	show_fullscr( &nm_background ); // show so we load all necessary data for the sub-bitmap
	if (!init_sub && ((nm_background_sub->bm_w != w*(((float) nm_background.bm_w)/SWIDTH)) || (nm_background_sub->bm_h != h*(((float) nm_background.bm_h)/SHEIGHT))))
	{
		init_sub=1;
		gr_free_sub_bitmap(nm_background_sub);
	}
	if (init_sub)
		nm_background_sub = gr_create_sub_bitmap(&nm_background,0,0,w*(((float) nm_background.bm_w)/SWIDTH),h*(((float) nm_background.bm_h)/SHEIGHT));
	show_fullscr( nm_background_sub );

	gr_set_current_canvas(old);
	gr_free_sub_canvas(tmp);

	Gr_scanline_darkening_level = 2*7;
	gr_setcolor( BM_XRGB(1,1,1) );
	for (w=5*BGScaleX;w>0;w--)
		gr_urect( x2-w, y1+w*(BGScaleY/BGScaleX), x2-w, y2-w*(BGScaleY/BGScaleX) );//right edge
	gr_setcolor( BM_XRGB(0,0,0) );
	for (h=5*BGScaleY;h>0;h--)
		gr_urect( x1+h*(BGScaleX/BGScaleY), y2-h, x2-h*(BGScaleX/BGScaleY), y2-h );//bottom edge
	Gr_scanline_darkening_level = GR_FADE_LEVELS;
}

// Draw a left justfied string
void nm_string( int w1,int x, int y, char * s )
{
	int w,h,aw,tx=0,t=0,i;
	char *p,*s1,*s2,measure[2];
	int XTabs[]={15,87,124,162,228,253};

	p=s1=NULL;
	s2 = d_strdup(s);

	for (i=0;i<6;i++) {
		XTabs[i]=FSPACX(XTabs[i]);
		XTabs[i]+=x;
	}
 
	measure[1]=0;

	if (!SurfingNet) {
		p = strchr( s2, '\t' );
		if (p && (w1>0) ) {
			*p = '\0';
			s1 = p+1;
		}
	}

	gr_get_string_size(s2, &w, &h, &aw  );

	if (w1 > 0)
		w = w1;

	if (SurfingNet) {
		for (i=0;i<strlen(s2);i++) {
			if (s2[i]=='\t' && SurfingNet) {
				x=XTabs[t];
				t++;
				continue;
			}
			measure[0]=s2[i];
			gr_get_string_size(measure,&tx,&h,&aw);
			gr_string(x,y,measure);
			x+=tx;
		}
	}
	else
		gr_string (x,y,s2);

	if (!SurfingNet && p && (w1>0) ) {
		gr_get_string_size(s1, &w, &h, &aw  );

		gr_string( x+w1-w, y, s1 );

		*p = '\t';
	}
	d_free(s2);
}


// Draw a slider and it's string
void nm_string_slider( int w1,int x, int y, char * s )
{
	int w,h,aw;
        char *p,*s1=0;

	p = strchr( s, '\t' );
	if (p)	{
		*p = '\0';
		s1 = p+1;
	}

	gr_get_string_size(s, &w, &h, &aw  );

	gr_string( x, y, s );

	if (p)	{
		gr_get_string_size(s1, &w, &h, &aw  );
		gr_string( x+w1-w, y, s1 );
		*p = '\t';
	}

}

// Draw a left justfied string with black background.
void nm_string_black( int w1,int x, int y, char * s )
{
	int w,h,aw;
	gr_get_string_size(s, &w, &h, &aw  );
	if (w1 == 0) w1 = w;

	gr_setcolor( BM_XRGB(5,5,5));
	gr_rect( x - FSPACX(2), y-FSPACY(1), x+w1, y + h);
	gr_setcolor( BM_XRGB(2,2,2));
	gr_rect( x - FSPACX(2), y - FSPACY(1), x, y + h );
	gr_setcolor( BM_XRGB(0,0,0));
	gr_rect( x - FSPACX(1), y - FSPACY(1), x+w1 - FSPACX(1), y + h);
	
	gr_string( x, y, s );
}

// Draw a right justfied string
void nm_rstring( int w1,int x, int y, char * s )
{
	int w,h,aw;
	gr_get_string_size(s, &w, &h, &aw  );
	x -= 3;

	if (w1 == 0) w1 = w;

	gr_string( x-w, y, s );
}

//for text items, constantly redraw cursor (to achieve flash)
void update_cursor( newmenu_item *item, int ScrollOffset)
{
	int w,h,aw;
	fix time = timer_get_approx_seconds();
	int x,y;
	char * text = item->text;

	Assert(item->type==NM_TYPE_INPUT_MENU || item->type==NM_TYPE_INPUT);
	gr_get_string_size(" ", &w, &h, &aw  );
	// even with variable char widths and a box that goes over the whole screen, we maybe never get more than 75 chars on the line
	if (strlen(text)>75)
		text+=strlen(text)-75;
	while( *text )	{
		gr_get_string_size(text, &w, &h, &aw  );
		if ( w > item->w-FSPACX(10) )
			text++;
		else
			break;
	}
	if (*text==0) 
		w = 0;
	x = item->x+w; y = item->y - LINE_SPACING*ScrollOffset;

	if (time & 0x8000)
		gr_string( x, y, CURSOR_STRING );
	else {
		gr_setcolor( BM_XRGB(0,0,0) );
		gr_rect( x, y, x+FSPACX(7), y+h );
	}
}

void nm_string_inputbox( int w, int x, int y, char * text, int current )
{
	int w1,h1,aw;

	// even with variable char widths and a box that goes over the whole screen, we maybe never get more than 75 chars on the line
	if (strlen(text)>75)
		text+=strlen(text)-75;
	while( *text )	{
		gr_get_string_size(text, &w1, &h1, &aw  );
		if ( w1 > w-FSPACX(10) )
			text++;
		else
			break;
	}
	if ( *text == 0 )
		w1 = 0;
	nm_string_black( w, x, y, text );

	if ( current )	{
		gr_string( x+w1, y, CURSOR_STRING );
	}
}

void draw_item( newmenu_item *item, int is_current, int tiny )
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
			gr_set_curfont(MEDIUM2_FONT);
		else
			gr_set_curfont(MEDIUM1_FONT);
        }
	
	switch( item->type )	{
		case NM_TYPE_TEXT:
		case NM_TYPE_MENU:
			nm_string( item->w, item->x, item->y, item->text );
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
			
			nm_string_slider( item->w, item->x, item->y, item->saved_text );
			}
			break;
		case NM_TYPE_INPUT_MENU:
			if ( item->group==0 )		{
				nm_string( item->w, item->x, item->y, item->text );
			} else {
				nm_string_inputbox( item->w, item->x, item->y, item->text, is_current );
			}
			break;
		case NM_TYPE_INPUT:
			nm_string_inputbox( item->w, item->x, item->y, item->text, is_current );
			break;
		case NM_TYPE_CHECK:
			nm_string( item->w, item->x, item->y, item->text );
			if (item->value)
				nm_rstring( item->right_offset,item->x, item->y, CHECKED_CHECK_BOX );
			else														  
				nm_rstring( item->right_offset,item->x, item->y, NORMAL_CHECK_BOX );
			break;
		case NM_TYPE_RADIO:
			nm_string( item->w, item->x, item->y, item->text );
			if (item->value)
				nm_rstring( item->right_offset, item->x, item->y, CHECKED_RADIO_BOX );
			else
				nm_rstring( item->right_offset, item->x, item->y, NORMAL_RADIO_BOX );
			break;
		case NM_TYPE_NUMBER:	{
			char text[10];
			if (item->value < item->min_value) item->value=item->min_value;
			if (item->value > item->max_value) item->value=item->max_value;
			nm_string( item->w, item->x, item->y, item->text );
			sprintf( text, "%d", item->value );
			nm_rstring( item->right_offset,item->x, item->y, text );
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

#ifdef NEWMENU_MOUSE
ubyte Hack_DblClick_MenuMode=0;
#endif

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
	int k, nmenus, nothers,ScrollOffset=0,LastScrollCheck=-1,MaxDisplayable,sx,sy;
	grs_canvas *menu_canvas, *save_canvas;
	grs_font * save_font;
	int string_width, string_height, average_width;
	int ty;
	int all_text=0; //set true if all text items
	int sound_stopped=0, time_stopped=0;
	int MaxOnMenu=MAXDISPLAYABLEITEMS;
	int IsScrollBox=0;   // Is this a scrolling box? Set to false at init
	char *Temp,TempVal;
#ifdef NEWMENU_MOUSE
	int mouse_state, omouse_state, dblclick_flag=0;
	int mx=0, my=0, mz=0, x1 = 0, x2, y1, y2;
#endif

	if (nitems < 1 )
		return -1;

	newmenu_close();

        MaxDisplayable=nitems;

	if ( Function_mode == FMODE_GAME && !((Game_mode & GM_MULTI) && (Newdemo_state != ND_STATE_PLAYBACK))) {
		digi_pause_all();
		sound_stopped = 1;
	}

	if (!(((Game_mode & GM_MULTI) && (Newdemo_state != ND_STATE_PLAYBACK)) && (Function_mode == FMODE_GAME) && (!Endlevel_sequence)) )
	{
		time_stopped = 1;
		stop_time();
	}

	save_canvas = grd_curcanv;
	gr_set_current_canvas( NULL );
	save_font = grd_curcanv->cv_font;

	tw = th = 0;

	if ( title )	{
		gr_set_curfont(HUGE_FONT);
		gr_get_string_size(title,&string_width,&string_height,&average_width );
		tw = string_width;
		th = string_height;
	}
	if ( subtitle )	{
		gr_set_curfont(MEDIUM3_FONT);
		gr_get_string_size(subtitle,&string_width,&string_height,&average_width );
		if (string_width > tw )
			tw = string_width;
		th += string_height;
	}


	th += FSPACY(5); //put some space between titles & body

	if (TinyMode)
		gr_set_curfont(GAME_FONT);
	else 
		gr_set_curfont(MEDIUM1_FONT);

	w = aw = 0;
	h = th;
	nmenus = nothers = 0;

	// Find menu height & width (store in w,h)
	for (i=0; i<nitems; i++ )	{
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
			string_width = item[i].text_len*FSPACX(8)+item[i].text_len;
			if ( string_width > MAX_TEXT_WIDTH ) 
				string_width = MAX_TEXT_WIDTH;
			item[i].value = -1;
		}

		if ( item[i].type == NM_TYPE_INPUT_MENU )	{
			Assert( strlen(item[i].text) < NM_MAX_TEXT_LEN );
			strcpy(item[i].saved_text, item[i].text );
			nmenus++;
			string_width = item[i].text_len*FSPACX(8)+item[i].text_len;
			item[i].value = -1;
			item[i].group = 0;
		}

		item[i].w = string_width;
		item[i].h = string_height;

		if ( string_width > w )
			w = string_width;		// Save maximum width
		if ( average_width > aw )
			aw = average_width;
		h += string_height+FSPACY(1);		// Find the height of all strings
	}

	right_offset=0;

	if ( width > -1 )
		w = width;

	if ( height > -1 )
		h = height;

	if (!TinyMode && i > MaxOnMenu)
	{
		IsScrollBox=1;
		h=((MaxOnMenu+(subtitle?1:0))*LINE_SPACING);
		MaxDisplayable=MaxOnMenu;
		
		// if our last citem was > MaxOnMenu, make sure we re-scroll when we call this menu again
		if (citem > MaxOnMenu-4)
		{
			ScrollOffset = citem - (MaxOnMenu-4);
			if (ScrollOffset + MaxOnMenu > nitems)
				ScrollOffset = nitems - MaxOnMenu;
		}
	}
	else
	{
		IsScrollBox=0;
		MaxOnMenu=i;
	}

	for (i=0; i<nitems; i++ )	{
		item[i].w = w;
		if (item[i].right_offset > right_offset )
			right_offset = item[i].right_offset;
	}
	
	w += right_offset;

	twidth = 0;
	if ( tw > w )	{
		twidth = ( tw - w )/2;
		w = tw;
	}
			
	// Find min point of menu border
	w += BORDERX*2;
	h += BORDERY*2;

	x = (GWIDTH-w)/2;
	y = (GHEIGHT-h)/2;

	if ( x < 0 ) x = 0;
	if ( y < 0 ) y = 0;

	nm_draw_background1( filename );

	// Save the background of the display
	menu_canvas = gr_create_sub_canvas( &grd_curscreen->sc_canvas, x, y, w, h );
	gr_set_current_canvas( menu_canvas );

	ty = BORDERY;

	// Update all item's x & y values.
	for (i=0; i<nitems; i++ )	{
		item[i].x = BORDERX + twidth + right_offset;
		item[i].y += BORDERY;
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

	// Clear mouse, joystick to clear button presses.
	game_flush_inputs();

#ifdef NEWMENU_MOUSE
	mouse_state = omouse_state = 0;
	newmenu_show_cursor();
#endif

	while(!done)	{
		timer_delay2(50);

		gr_flip();

		gr_set_current_canvas( NULL );
#ifdef OGL
		nm_draw_background1(filename);
#endif
		if (filename == NULL)
			nm_draw_background(x-(IsScrollBox?FSPACX(5):0),y,x+w,y+h);

		gr_set_current_canvas( menu_canvas );

		if ( title )	{
			gr_set_curfont(HUGE_FONT);
			gr_set_fontcolor( BM_XRGB(31,31,31), -1 );
			gr_get_string_size(title,&string_width,&string_height,&average_width );
			tw = string_width;
			th = string_height;
			gr_printf( 0x8000, ty, title );
		}
	
		if ( subtitle )	{
			gr_set_curfont(MEDIUM3_FONT);
			gr_set_fontcolor( BM_XRGB(21,21,21), -1 );
			gr_get_string_size(subtitle,&string_width,&string_height,&average_width );
			tw = string_width;
			th = (title?th:0);
			gr_printf( 0x8000, ty+th, subtitle );
		}

		if (TinyMode)
			gr_set_curfont(GAME_FONT);
		else 
			gr_set_curfont(MEDIUM1_FONT);

#ifdef NEWMENU_MOUSE
		newmenu_show_cursor();      // possibly hidden
		omouse_state = mouse_state;
		mouse_state = mouse_button_state(0);
#endif

		//see if redbook song needs to be restarted
		songs_check_redbook_repeat();
		
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
				
		old_choice = choice;
	
		switch( k )	{
		case KEY_TAB + KEY_SHIFTED:
		case KEY_UP:
		case KEY_PAD8:
			if (all_text) break;
			do {
				choice--;

				if (IsScrollBox)
				{
					LastScrollCheck=-1;
						
					if (choice<0)
					{
						choice=nitems-1;
						ScrollOffset = nitems-MaxOnMenu;
						break;
					}
			
					if (choice-4<ScrollOffset && ScrollOffset > 0)
					{
						ScrollOffset--;
					}
				}
				else
				{
					if (choice >= nitems ) choice=0;
					if (choice < 0 ) choice=nitems-1;
				}
			} while ( item[choice].type==NM_TYPE_TEXT );
			if ((item[choice].type==NM_TYPE_INPUT) && (choice!=old_choice))	
				item[choice].value = -1;
			if ((old_choice>-1) && (item[old_choice].type==NM_TYPE_INPUT_MENU) && (old_choice!=choice))	{
				item[old_choice].group=0;
				strcpy(item[old_choice].text, item[old_choice].saved_text );
				item[old_choice].value = -1;
			}
			break;
		case KEY_TAB:
		case KEY_DOWN:
		case KEY_PAD2:
			if (all_text) break;
			do {
				choice++;
	
				if (IsScrollBox)
				{
					LastScrollCheck=-1;
						
					if (choice==nitems)
					{ 
						choice=0;
						ScrollOffset=0;
						break;
					}
		
					if (choice+4>=MaxOnMenu+ScrollOffset && ScrollOffset < nitems-MaxOnMenu)
					{
						ScrollOffset++;
					}
				}
				else
				{
					if (choice < 0 ) choice=nitems-1;
					if (choice >= nitems ) choice=0;
				}

			} while ( item[choice].type==NM_TYPE_TEXT );
			if ((item[choice].type==NM_TYPE_INPUT) && (choice!=old_choice))	
				item[choice].value = -1;
			if ( (old_choice>-1) && (item[old_choice].type==NM_TYPE_INPUT_MENU) && (old_choice!=choice))	{
				item[old_choice].group=0;
				strcpy(item[old_choice].text, item[old_choice].saved_text );	
				item[old_choice].value = -1;
			}
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
					if (IsScrollBox)
					{
						if (choice==(MaxOnMenu+ScrollOffset-1) || choice==ScrollOffset)
						{
							LastScrollCheck=-1;
						}
					}
				
					break;
				case NM_TYPE_RADIO:
					for (i=0; i<nitems; i++ )	{
						if ((i!=choice) && (item[i].type==NM_TYPE_RADIO) && (item[i].group==item[choice].group) && (item[i].value) )	{
							item[i].value = 0;
						}
					}
					item[choice].value = 1;
					break;
				}	
			}
			break;
		case KEY_SHIFTED+KEY_UP:
			if (MenuReordering && choice!=0)
			{
				Temp=item[choice].text;
				TempVal=item[choice].value;
				item[choice].text=item[choice-1].text;
				item[choice].value=item[choice-1].value;
				item[choice-1].text=Temp;
				item[choice-1].value=TempVal;
				choice--;
			}
			break;
		case KEY_SHIFTED+KEY_DOWN:
			if (MenuReordering && choice!=(nitems-1))
			{
				Temp=item[choice].text;
				TempVal=item[choice].value;
				item[choice].text=item[choice+1].text;
				item[choice].value=item[choice+1].value;
				item[choice+1].text=Temp;
				item[choice+1].value=TempVal;
				choice++;
			}
			break;
		case KEY_ENTER:
		case KEY_PADENTER:
			if ( (choice>-1) && (item[choice].type==NM_TYPE_INPUT_MENU) && (item[choice].group==0))	{
				item[choice].group = 1;
				if ( !strnicmp( item[choice].saved_text, TXT_EMPTY, strlen(TXT_EMPTY) ) )	{
					item[choice].text[0] = 0;
					item[choice].value = -1;
				} else {	
					strip_end_whitespace(item[choice].text);
				}
			} else
				done = 1;
			break;

		case KEY_ALTED+KEY_ENTER:
		case KEY_ALTED+KEY_PADENTER:
			gr_toggle_fullscreen();
			break;

		case KEY_ESC:
			if ( (choice>-1) && (item[choice].type==NM_TYPE_INPUT_MENU) && (item[choice].group==1))	{
				item[choice].group=0;
				strcpy(item[choice].text, item[choice].saved_text );	
				item[choice].value = -1;
			} else {
				done = 1;
				choice = -1;
			}
			break;

#ifdef macintosh
		case KEY_COMMAND+KEY_SHIFTED+KEY_3:
#endif
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
			for (i=0; i<MaxOnMenu; i++ )	{
				x1 = grd_curcanv->cv_bitmap.bm_x + item[i].x-FSPACX(13);
				x2 = x1 + item[i].w+FSPACX(13);
				y1 = grd_curcanv->cv_bitmap.bm_y + item[i].y;
				y2 = y1 + item[i].h;
				if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2))) {
					if (i+ScrollOffset != choice) {
						if(Hack_DblClick_MenuMode) dblclick_flag = 0; 
					}
					
					choice = i + ScrollOffset;

					switch( item[choice].type )	{
						case NM_TYPE_CHECK:
							if ( item[choice].value )
								item[choice].value = 0;
							else
								item[choice].value = 1;
	
							if (IsScrollBox)
								LastScrollCheck=-1;
							break;
						case NM_TYPE_RADIO:
							for (i=0; i<nitems; i++ )	{
								if ((i!=choice) && (item[i].type==NM_TYPE_RADIO) && (item[i].group==item[choice].group) && (item[i].value) )	{
									item[i].value = 0;
								}
							}
							item[choice].value = 1;
							break;
						case NM_TYPE_TEXT:
							choice=old_choice;
							mouse_state=0;
							break;
					}
					break;
				}
			}
		}

		if (mouse_state && all_text)
			done = 1;
		
		if ( !done && mouse_state && !all_text ) {
			mouse_get_pos(&mx, &my, &mz);
			
			// check possible scrollbar stuff first
			if (IsScrollBox) {
				int arrow_width, arrow_height, aw, ScrollAllow=0, time=timer_get_fixed_seconds();
				static fix ScrollTime=0;
				if (ScrollTime + F1_0/5 < time || time < ScrollTime)
				{
					ScrollTime = time;
					ScrollAllow = 1;
				}

				if (ScrollOffset != 0) {
					gr_get_string_size(UP_ARROW_MARKER, &arrow_width, &arrow_height, &aw);
					x2 = grd_curcanv->cv_bitmap.bm_x + item[ScrollOffset].x-FSPACX(13);
		          		y1 = grd_curcanv->cv_bitmap.bm_y + item[ScrollOffset].y-(((int)LINE_SPACING)*ScrollOffset);
					x1 = x2 - arrow_width;
					y2 = y1 + arrow_height;
					if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) && ScrollAllow) {
						choice--;
						LastScrollCheck=-1;
						if (choice-4<ScrollOffset && ScrollOffset > 0)
						{
							ScrollOffset--;
						}
					}
				}
				if (ScrollOffset+MaxDisplayable<nitems) {
					gr_get_string_size(DOWN_ARROW_MARKER, &arrow_width, &arrow_height, &aw);
					x2 = grd_curcanv->cv_bitmap.bm_x + item[ScrollOffset+MaxDisplayable-1].x-FSPACX(13);
					y1 = grd_curcanv->cv_bitmap.bm_y + item[ScrollOffset+MaxDisplayable-1].y-(((int)LINE_SPACING)*ScrollOffset);
					x1 = x2 - arrow_width;
					y2 = y1 + arrow_height;
					if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) && ScrollAllow) {
						choice++;
						LastScrollCheck=-1;
						if (choice+4>=MaxOnMenu+ScrollOffset && ScrollOffset < nitems-MaxOnMenu)
						{
							ScrollOffset++;
						}
					}
				}
			}
			
			for (i=0; i<MaxOnMenu; i++ )	{
				x1 = grd_curcanv->cv_bitmap.bm_x + item[i].x-FSPACX(13);
				x2 = x1 + item[i].w+FSPACX(13);
				y1 = grd_curcanv->cv_bitmap.bm_y + item[i].y;
				y2 = y1 + item[i].h;

				if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) && (item[i].type != NM_TYPE_TEXT) ) {
					if (i+ScrollOffset != choice) {
						if(Hack_DblClick_MenuMode) dblclick_flag = 0; 
					}

					choice = i + ScrollOffset;

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
							} else if ( (mx < x2) && (mx > (x2 - sright_width)) && (item[choice].value != item[choice].max_value) ) {
								item[choice].value = item[choice].max_value;
							} else if ( (mx > (x1 + sleft_width)) && (mx < (x2 - sright_width)) ) {
								int num_values, value_width, new_value;
								
								num_values = item[choice].max_value - item[choice].min_value + 1;
								value_width = (slider_width - sleft_width - sright_width) / num_values;
								new_value = (mx - x1 - sleft_width) / value_width;
								if ( item[choice].value != new_value ) {
									item[choice].value = new_value;
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
					break;
				}
			}
		}
		
		if ( !done && !mouse_state && omouse_state && !all_text && (choice != -1) && (item[choice].type == NM_TYPE_MENU) ) {
			mouse_get_pos(&mx, &my, &mz);
			for (i=0; i<nitems; i++ )	{
				x1 = grd_curcanv->cv_bitmap.bm_x + item[i].x-FSPACX(13);
				x2 = x1 + item[i].w+FSPACX(13);
				y1 = grd_curcanv->cv_bitmap.bm_y + item[i].y;
				y2 = y1 + item[i].h;
				if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2))) {
					if (Hack_DblClick_MenuMode) {
						if (dblclick_flag) done = 1;
						else dblclick_flag = 1;
					}
					else done = 1;
				}
			}
		}

		if ( !done && !mouse_state && omouse_state && (choice>-1) && (item[choice].type==NM_TYPE_INPUT_MENU) && (item[choice].group==0))	{
			item[choice].group = 1;
			if ( !strnicmp( item[choice].saved_text, TXT_EMPTY, strlen(TXT_EMPTY) ) )	{
				item[choice].text[0] = 0;
				item[choice].value = -1;
			} else {
				strip_end_whitespace(item[choice].text);
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
				} else {
					ascii = key_ascii();
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
						}
					}
				}
			}
			else if ((item[choice].type!=NM_TYPE_INPUT) && (item[choice].type!=NM_TYPE_INPUT_MENU) )
			{
				ascii = key_ascii();
				if (ascii < 255 ) {
					int choice1 = choice;
					ascii = toupper(ascii);
					do {
						int i,ch;
						choice1++;
						if (choice1 >= nitems )
							choice1=0;

						for (i=0;(ch=item[choice1].text[i])!=0 && ch==' ';i++);

						if ( ( (item[choice1].type==NM_TYPE_MENU) ||
								(item[choice1].type==NM_TYPE_CHECK) ||
								(item[choice1].type==NM_TYPE_RADIO) ||
								(item[choice1].type==NM_TYPE_NUMBER) ||
								(item[choice1].type==NM_TYPE_SLIDER) )
								&& (ascii==toupper(ch)) )
						{
							k = 0;
							choice = choice1;
						}

						while (choice > ScrollOffset+MaxDisplayable-1)
						{
							ScrollOffset++;
						}
						while (choice < ScrollOffset)
						{
							ScrollOffset--;
						}

					} while (choice1 != choice );
				}
			}

			if ( (item[choice].type==NM_TYPE_NUMBER) || (item[choice].type==NM_TYPE_SLIDER)) 	{
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
			}
	
		}

		gr_set_current_canvas(menu_canvas);
		for (i=ScrollOffset; i<MaxDisplayable+ScrollOffset; i++ )
		{
			item[i].y-=(((int)LINE_SPACING)*ScrollOffset);
			draw_item( &item[i], (i==choice && !all_text),TinyMode );
#ifdef NEWMENU_MOUSE
			newmenu_show_cursor();
#endif
			item[i].y+=(((int)LINE_SPACING)*ScrollOffset);

			if (i==choice && (item[i].type==NM_TYPE_INPUT || (item[i].type==NM_TYPE_INPUT_MENU && item[i].group)))
				update_cursor( &item[i],ScrollOffset);
		}

		if (IsScrollBox)
		{
			{
				LastScrollCheck=ScrollOffset;
				gr_set_curfont(MEDIUM2_FONT);
						
				sy=item[ScrollOffset].y-(((int)LINE_SPACING)*ScrollOffset);
				sx=item[ScrollOffset].x-FSPACX(11);
						
			
				if (ScrollOffset!=0)
					nm_rstring( FSPACX(11), sx, sy, UP_ARROW_MARKER );
				else
					nm_rstring( FSPACX(11), sx, sy, "  " );
		
				sy=item[ScrollOffset+MaxDisplayable-1].y-(((int)LINE_SPACING)*ScrollOffset);
				sx=item[ScrollOffset+MaxDisplayable-1].x-FSPACX(11);
			
				if (ScrollOffset+MaxDisplayable<nitems)
					nm_rstring( FSPACX(11), sx, sy, DOWN_ARROW_MARKER );
				else
					nm_rstring( FSPACX(11), sx, sy, "  " );
		
			}
		
		}
	}

	newmenu_hide_cursor();

	gr_free_sub_canvas( menu_canvas );
	gr_set_current_canvas( NULL );
	grd_curcanv->cv_font	= save_font;
	gr_set_current_canvas( save_canvas );
	keyd_repeat = old_keyd_repeat;

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

/* Honestly I think we don't ened this function anymore since PhysFS already takes care about 
   sorting files. However I keep it since:
   a) it's just one function
   b) works with FILENAME_LEN so it doesn'T break if we decide to support more than 8+3 filename length
   c) I am not sure if PhysFS sorty correctly on all platforms
*/
void newmenu_file_sort( int n, char *list )
{
	int i, j, incr;
	char t[(FILENAME_LEN+1)];

	incr = n / 2;
	while( incr > 0 ) {
		for (i=incr; i<n; i++ ) {
			j = i - incr;
			while (j>=0 ) {
				if (strncmp(&list[j*(FILENAME_LEN+1)], &list[(j+incr)*(FILENAME_LEN+1)], FILENAME_LEN-1) > 0) {
					memcpy( t, &list[j*(FILENAME_LEN+1)], FILENAME_LEN );
					memcpy( &list[j*(FILENAME_LEN+1)], &list[(j+incr)*(FILENAME_LEN+1)], FILENAME_LEN );
					memcpy( &list[(j+incr)*(FILENAME_LEN+1)], t, FILENAME_LEN );
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
	char filename[FILENAME_LEN + 9];

	for (i=0;i<10; i++)
	{
		sprintf( filename, GameArg.SysUsePlayersDir? "Players/%s.sg%x" : "%s.sg%x", name, i );

		PHYSFS_delete(filename);
	}
}

#define MAX_FILES 300

int MakeNewPlayerFile(int allow_abort);

int newmenu_get_filename( char * title, char * type, char * filename, int allow_abort_flag )
{
	int i;
	char **find;
	char **f;
	char *ext;
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
	int w_x, w_y, w_w, w_h, title_height, height=0;
	int box_x, box_y, box_w, box_h;
#ifdef NEWMENU_MOUSE
	int mx, my, mz, x1, x2, y1, y2, mouse_state, omouse_state;
	int mouse2_state, omouse2_state;
	int dblclick_flag=0;
#endif

	w_x=w_y=w_w=w_h=title_height=0;
	box_x=box_y=box_w=box_h=0;

	filenames = d_malloc( MAX_FILES * (FILENAME_LEN+1) );
	if (filenames==NULL) return 0;

	newmenu_close();
	citem = 0;
	keyd_repeat = 1;

	if (!stricmp(type, ".plr"))
		player_mode = 1;
	else if (!stricmp(type, ".dem"))
		demo_mode = 1;

ReadFileNames:
	done = 0;
	NumFiles=0;
	
	if (player_mode)	{
		strncpy( &filenames[NumFiles*(FILENAME_LEN+1)], TXT_CREATE_NEW, FILENAME_LEN );
		NumFiles++;
	}

	find = PHYSFS_enumerateFiles(demo_mode ? DEMO_DIR : ((player_mode && GameArg.SysUsePlayersDir) ? "Players/" : ""));
	for (f = find; *f != NULL; f++)
	{
		ext = strrchr(*f, '.');
		if (!ext || strnicmp(ext, type, 4))
			continue;

		if (NumFiles < MAX_FILES)
		{
			strncpy(&filenames[NumFiles*(FILENAME_LEN+1)],*f,FILENAME_LEN);
			if (player_mode)
			{
				char *p;

				p = strchr(&filenames[NumFiles*(FILENAME_LEN+1)], '.');
				if (p)
					*p = '\0';
			}
			NumFiles++;
		}
		else
			break;
	}

	PHYSFS_freeList(find);

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
		nm_messagebox(NULL, 1, "Ok", "%s\n '%s' %s", TXT_NO_FILES_MATCHING, type, TXT_WERE_FOUND);
		exit_value = 0;
		goto ExitFileMenu;
	}

	if (!initialized) {	
		gr_set_current_canvas(NULL);

		gr_set_curfont(MEDIUM3_FONT);

		box_w = 0;
		for (i=0; i<NumFiles; i++ ) {
			int w, h, aw;
			gr_get_string_size( &filenames[i*(FILENAME_LEN+1)], &w, &h, &aw );
			if ( w > box_w )
				box_w = w+FSPACX(10);
		}

		height = (LINE_SPACING * NumFiles_displayed);

		{
			int w, h, aw;
			gr_get_string_size( title, &w, &h, &aw );
			if ( w > box_w )
				box_w = w;
			title_height = h+FSPACY(5);
		}
	
		box_x = (grd_curcanv->cv_bitmap.bm_w-box_w)/2;
		box_y = (grd_curcanv->cv_bitmap.bm_h-(height+title_height))/2 + title_height;
		if ( box_y < title_height )
			box_y = title_height;

		initialized = 1;
	}

	if ( !player_mode )	{
		newmenu_file_sort( NumFiles, filenames );
	} else {
		newmenu_file_sort( NumFiles-1, &filenames[(FILENAME_LEN+1)] );		// Don't sort first one!
		for ( i=0; i<NumFiles; i++ )	{
			if (!stricmp(Players[Player_num].callsign, &filenames[i*(FILENAME_LEN+1)]) )	{
#ifdef NEWMENU_MOUSE
				dblclick_flag = 1;
#endif
				citem = i;
			}
	 	}
	}

	nm_draw_background1(NULL);

#ifdef NEWMENU_MOUSE
	mouse_state = omouse_state = 0;
	mouse2_state = omouse2_state = 0;
	newmenu_show_cursor();
#endif

	while(!done)	{
		timer_delay2(50);

		ocitem = citem;
		ofirst_item = first_item;

		gr_flip();
#ifdef OGL
		nm_draw_background1(NULL);
#endif
		nm_draw_background( box_x-BORDERX,box_y-title_height-BORDERY,box_x+box_w+BORDERX,box_y+height+BORDERY );
		gr_set_curfont(MEDIUM3_FONT);
		gr_string( 0x8000, box_y - title_height, title );

#ifdef NEWMENU_MOUSE
		omouse_state = mouse_state;
		omouse2_state = mouse2_state;
		mouse_state = mouse_button_state(0);
		mouse2_state = mouse_button_state(1);
#endif

		//see if redbook song needs to be restarted
		songs_check_redbook_repeat();
		
		key = key_inkey();

		switch(key)	{
#ifdef macintosh
			case KEY_COMMAND+KEY_SHIFTED+KEY_3:
#endif
			case KEY_PRINT_SCREEN:
			save_screen_shot(0);
			
			break;

		case KEY_CTRLED+KEY_D:
			if ( ((player_mode)&&(citem>0)) || ((demo_mode)&&(citem>=0)) )	{
				int x = 1;
				newmenu_hide_cursor();
				if (player_mode)
					x = nm_messagebox( NULL, 2, TXT_YES, TXT_NO, "%s %s?", TXT_DELETE_PILOT, &filenames[citem*(FILENAME_LEN+1)]+((player_mode && filenames[citem*(FILENAME_LEN+1)]=='$')?1:0) );
				else if (demo_mode)
					x = nm_messagebox( NULL, 2, TXT_YES, TXT_NO, "%s %s?", TXT_DELETE_DEMO, &filenames[citem*(FILENAME_LEN+1)]+((demo_mode && filenames[citem*(FILENAME_LEN+1)]=='$')?1:0) );
				newmenu_show_cursor();
 				if (x==0)	{
					char * p;
					char plxfile[PATH_MAX];
					int ret;
					char name[PATH_MAX];

					p = &filenames[(citem*(FILENAME_LEN+1))+strlen(&filenames[citem*(FILENAME_LEN+1)])];
					if (player_mode)
						*p = '.';

					strcpy(name, demo_mode ? DEMO_DIR : ((player_mode && GameArg.SysUsePlayersDir) ? "Players/" : ""));
					strcat(name,&filenames[citem*(FILENAME_LEN+1)]);
					
					ret = !PHYSFS_delete(name);
					if (player_mode)
						*p = 0;

					if ((!ret) && player_mode)	{
						delete_player_saved_games( &filenames[citem*(FILENAME_LEN+1)] );
						// delete PLX file
						sprintf(plxfile, GameArg.SysUsePlayersDir? "Players/%.8s.plx" : "%.8s.plx", &filenames[citem*(FILENAME_LEN+1)]);
						if (cfexist(plxfile))
							PHYSFS_delete(plxfile);
					}

					if (ret) {
						if (player_mode)
							nm_messagebox( NULL, 1, TXT_OK, "%s %s %s", TXT_COULDNT, TXT_DELETE_PILOT, &filenames[citem*(FILENAME_LEN+1)]+((player_mode && filenames[citem*(FILENAME_LEN+1)]=='$')?1:0) );
						else if (demo_mode)
							nm_messagebox( NULL, 1, TXT_OK, "%s %s %s", TXT_COULDNT, TXT_DELETE_DEMO, &filenames[citem*(FILENAME_LEN+1)]+((demo_mode && filenames[citem*(FILENAME_LEN+1)]=='$')?1:0) );
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
			done = 1;
			break;
			
		case KEY_ALTED+KEY_ENTER:
		case KEY_ALTED+KEY_PADENTER:
			gr_toggle_fullscreen();
			break;
		
		default:	
			{
				int ascii = key_ascii();
				if ( ascii < 255 )	{
					int cc,cc1;
					cc=cc1=citem+1;
					if (cc1 < 0 )  cc1 = 0;
					if (cc1 >= NumFiles )  cc1 = 0;
					while(1) {
						if ( cc < 0 ) cc = 0;
						if ( cc >= NumFiles ) cc = 0;
						if ( citem == cc ) break;
	
						if ( toupper(filenames[cc*(FILENAME_LEN+1)]) == toupper(ascii) )	{
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
		{
			first_item = citem-NumFiles_displayed+1;
		}

		if (NumFiles <= NumFiles_displayed )
			 first_item = 0;

		if (first_item>NumFiles-NumFiles_displayed)
		{
			first_item = NumFiles-NumFiles_displayed;
		}

		if (first_item < 0 ) first_item = 0;

#ifdef NEWMENU_MOUSE
		if (mouse_state || mouse2_state) {
			int w, h, aw;

			mouse_get_pos(&mx, &my, &mz);
			for (i=first_item; i<first_item+NumFiles_displayed; i++ )	{
				gr_get_string_size(&filenames[i*(FILENAME_LEN+1)], &w, &h, &aw  );
				x1 = box_x;
				x2 = box_x + box_w;
				y1 = (i-first_item)*LINE_SPACING + box_y;
				y2 = y1+h;
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

			gr_get_string_size(&filenames[citem*(FILENAME_LEN+1)], &w, &h, &aw  );
			mouse_get_pos(&mx, &my, &mz);
			x1 = box_x;
			x2 = box_x + box_w;
			y1 = (citem-first_item)*LINE_SPACING + box_y;
			y2 = y1+h;
			if ( ((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) ) {
				if (dblclick_flag) done = 1;
				else dblclick_flag = 1;
			}
		}
#endif

		gr_setcolor( BM_XRGB( 0,0,0)  );
		for (i=first_item; i<first_item+NumFiles_displayed; i++ )	{
			int w, h, aw, y;
			y = (i-first_item)*LINE_SPACING + box_y;
		
			if ( i >= NumFiles )	{
				gr_setcolor( BM_XRGB(5,5,5));
				gr_rect( box_x + box_w - FSPACX(1), y-FSPACY(1), box_x + box_w, y + LINE_SPACING);
				gr_setcolor( BM_XRGB(2,2,2));
				gr_rect( box_x - FSPACX(1), y - FSPACY(1), box_x, y + LINE_SPACING );
				gr_setcolor( BM_XRGB(0,0,0));
				gr_rect( box_x, y - FSPACY(1), box_x + box_w - FSPACX(1), y + LINE_SPACING);
				
			} else {
				if ( i == citem )	
					gr_set_curfont(MEDIUM2_FONT);
				else	
					gr_set_curfont(MEDIUM1_FONT);
				gr_get_string_size(&filenames[i*(FILENAME_LEN+1)], &w, &h, &aw  );

				gr_setcolor( BM_XRGB(5,5,5));
				gr_rect( box_x + box_w - FSPACX(1), y-FSPACY(1), box_x + box_w, y + LINE_SPACING);
				gr_setcolor( BM_XRGB(2,2,2));
				gr_rect( box_x - FSPACX(1), y - FSPACY(1), box_x, y + LINE_SPACING );
				gr_setcolor( BM_XRGB(0,0,0));
				gr_rect( box_x, y - FSPACY(1), box_x + box_w - FSPACX(1), y + LINE_SPACING);

				gr_string( box_x + FSPACX(5), y, (&filenames[i*(FILENAME_LEN+1)])+((player_mode && filenames[i*(FILENAME_LEN+1)]=='$')?1:0)  );
			}
		}
	}

#ifdef NEWMENU_MOUSE
	newmenu_hide_cursor();
#endif

ExitFileMenuEarly:
	if ( citem > -1 )	{
		strncpy( filename, (&filenames[citem*(FILENAME_LEN+1)])+((player_mode && filenames[citem*(FILENAME_LEN+1)]=='$')?1:0), FILENAME_LEN );
		exit_value = 1;
	} else {
		exit_value = 0;
	}

ExitFileMenu:
	keyd_repeat = old_keyd_repeat;

	if ( filenames )
		d_free(filenames);

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
	int box_w, height, box_x, box_y, title_height;
#ifdef NEWMENU_MOUSE
	int mx, my, mz, x1, x2, y1, y2, mouse_state, omouse_state;	//, dblclick_flag;
#endif

	keyd_repeat = 1;

	newmenu_close();

	gr_set_current_canvas(NULL);

	gr_set_curfont(MEDIUM3_FONT);

	box_w = 0;
	for (i=0; i<nitems; i++ )	{
		int w, h, aw;
		gr_get_string_size( items[i], &w, &h, &aw );		
		if ( w > box_w )
			box_w = w+FSPACX(10);
	}
	height = LINE_SPACING * LB_ITEMS_ON_SCREEN;

	{
		int w, h, aw;
		gr_get_string_size( title, &w, &h, &aw );		
		if ( w > box_w )
			box_w = w;
		title_height = h+FSPACY(5);
	}

	box_x = (grd_curcanv->cv_bitmap.bm_w-box_w)/2;
	box_y = (grd_curcanv->cv_bitmap.bm_h-(height+title_height))/2 + title_height;
	if ( box_y < title_height )
		box_y = title_height;

	done = 0;
	citem = default_item;
	if ( citem < 0 ) citem = 0;
	if ( citem >= nitems ) citem = 0;

	first_item = -1;

#ifdef NEWMENU_MOUSE
	mouse_state = omouse_state = 0;
	newmenu_show_cursor();
#endif

	while(!done)	{
		timer_delay2(50);
		gr_flip();
#ifdef OGL
		nm_draw_background1(NULL);
#endif
		nm_draw_background( box_x-BORDERX,box_y-title_height-BORDERY,box_x+box_w+BORDERX,box_y+height+BORDERY );
		gr_set_curfont(MEDIUM3_FONT);
		gr_string( 0x8000, box_y - title_height, title );

		ocitem = citem;
		ofirst_item = first_item;

#ifdef NEWMENU_MOUSE
		omouse_state = mouse_state;
		mouse_state = mouse_button_state(0);
#endif

		//see if redbook song needs to be restarted
		songs_check_redbook_repeat();
		
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
#ifdef macintosh
			case KEY_COMMAND+KEY_SHIFTED+KEY_3:
#endif
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
			done = 1;
			break;

		case KEY_ALTED+KEY_ENTER:
		case KEY_ALTED+KEY_PADENTER:
			gr_toggle_fullscreen();
			break;

		default:	
			{
				int ascii = key_ascii();
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
				x1 = box_x;
				x2 = box_x + box_w;
				y1 = (i-first_item)*LINE_SPACING+box_y;
				y2 = y1+h;
				if ( ((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) ) {
					citem = i;
					done = 1;
					break;
				}
			}
		}
#endif

		for (i=first_item; i<first_item+LB_ITEMS_ON_SCREEN; i++ )	{
			int y = (i-first_item)*LINE_SPACING+box_y;
			if ( i >= nitems )	{
				gr_setcolor( BM_XRGB(5,5,5));
				gr_rect( box_x + box_w - FSPACX(1), y-FSPACY(1), box_x + box_w, y + LINE_SPACING);
				gr_setcolor( BM_XRGB(2,2,2));
				gr_rect( box_x - FSPACX(1), y - FSPACY(1), box_x, y + LINE_SPACING );
				gr_setcolor( BM_XRGB(0,0,0));
				gr_rect( box_x, y - FSPACY(1), box_x + box_w - FSPACX(1), y + LINE_SPACING);
			} else {
				if ( i == citem )	
					gr_set_curfont(MEDIUM2_FONT);
				else	
					gr_set_curfont(MEDIUM1_FONT);
				gr_setcolor( BM_XRGB(5,5,5));
				gr_rect( box_x + box_w - FSPACX(1), y-FSPACY(1), box_x + box_w, y + LINE_SPACING);
				gr_setcolor( BM_XRGB(2,2,2));
				gr_rect( box_x - FSPACX(1), y - FSPACY(1), box_x, y + LINE_SPACING );
				gr_setcolor( BM_XRGB(0,0,0));
				gr_rect( box_x, y - FSPACY(1), box_x + box_w - FSPACX(1), y + LINE_SPACING);
				gr_string( box_x+FSPACX(5), y, items[i]  );
			}
		}
	}
	newmenu_hide_cursor();
	keyd_repeat = old_keyd_repeat;

	return citem;
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
