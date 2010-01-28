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

#include "pstypes.h"
#include "error.h"
#include "gr.h"
#include "grdef.h"
#include "window.h"
#include "songs.h"
#include "key.h"
#include "palette.h"
#include "game.h"
#include "text.h"
#include "menu.h"
#include "newmenu.h"
#include "gamefont.h"
#include "iff.h"
#include "pcx.h"
#include "u_mem.h"
#include "mouse.h"
#include "joy.h"
#include "digi.h"
#include "multi.h"
#include "endlevel.h"
#include "screens.h"
#include "config.h"
#include "player.h"
#include "state.h"
#include "newdemo.h"
#include "kconfig.h"
#include "strutil.h"
#include "vers_id.h"
#include "timer.h"
#include "playsave.h"
#include "automap.h"
#include "rbaudio.h"

#ifdef OGL
#include "ogl_init.h"
#endif


#define MAXDISPLAYABLEITEMS 14
#define MESSAGEBOX_TEXT_SIZE 2176  // How many characters in messagebox
#define MAX_TEXT_WIDTH FSPACX(120) // How many pixels wide a input box can be

struct newmenu
{
	int				x,y,w,h;
	char			*title;
	char			*subtitle;
	int				nitems;
	newmenu_item	*items;
	int				(*subfunction)(newmenu *menu, d_event *event, void *userdata);
	int				citem;
	char			*filename;
	int				tiny_mode;
	int				scroll_offset, last_scroll_check, max_displayable;
	int				all_text;		//set true if all text items
	int				sound_stopped,time_stopped;
	int				is_scroll_box;   // Is this a scrolling box? Set to false at init
	int				max_on_menu;
	int				mouse_state, omouse_state, dblclick_flag;
	int				done;
	void			*userdata;		// For whatever - like with window system
};

grs_bitmap nm_background, nm_background1;
grs_bitmap *nm_background_sub = NULL;
ubyte MenuReordering=0;
ubyte SurfingNet=0;
static int draw_copyright=0;
extern ubyte Version_major,Version_minor;
extern char last_palette_loaded[];

int newmenu_do4( char * title, char * subtitle, int nitems, newmenu_item * item, int (*subfunction)(newmenu *menu, d_event *event, void *userdata), void *userdata, int citem, char * filename, int width, int height, int TinyMode );

void newmenu_close()	{
	if (nm_background.bm_data)
	{
		if (nm_background_sub)
		{
			gr_free_sub_bitmap(nm_background_sub);
			nm_background_sub = NULL;
		}
		gr_free_bitmap_data (&nm_background);
	}
	if (nm_background1.bm_data)
		gr_free_bitmap_data (&nm_background1);
}

// Draw Copyright and Version strings
void nm_draw_copyright()
{
	int w,h,aw;

	gr_set_curfont(GAME_FONT);
	gr_set_fontcolor(BM_XRGB(6,6,6),-1);
	gr_printf(0x8000,SHEIGHT-LINE_SPACING,TXT_COPYRIGHT);

	gr_get_string_size("V2.2", &w, &h, &aw );
	gr_printf(SWIDTH-w-FSPACX(1),SHEIGHT-LINE_SPACING,"V%d.%d",Version_major,Version_minor);

	gr_set_fontcolor( BM_XRGB(25,0,0), -1);
	gr_printf(0x8000,SHEIGHT-(LINE_SPACING*2),DESCENT_VERSION);
}

// Draws the background of menus (i.e. Descent Logo screen)
void nm_draw_background1(char * filename)
{
	int pcx_error;

	if (filename == NULL && Function_mode == FMODE_MENU)
		filename = Menu_pcx_name;

	if (filename != NULL)
	{
		if (nm_background1.bm_data == NULL)
		{
			gr_init_bitmap_data (&nm_background1);
			pcx_error = pcx_read_bitmap( filename, &nm_background1, BM_LINEAR, gr_palette );
			Assert(pcx_error == PCX_ERROR_NONE);
			if (!strcmp(filename,Menu_pcx_name))
				draw_copyright=1;
			else
				draw_copyright=0;
		}
		gr_palette_load( gr_palette );
		show_fullscr(&nm_background1);

		if (draw_copyright)
			nm_draw_copyright();
	}

	strcpy(last_palette_loaded,"");		//force palette load next time
}

#define MENU_BACKGROUND_BITMAP_HIRES (cfexist("scoresb.pcx")?"scoresb.pcx":"scores.pcx")
#define MENU_BACKGROUND_BITMAP_LORES (cfexist("scores.pcx")?"scores.pcx":"scoresb.pcx")
#define MENU_BACKGROUND_BITMAP (HIRESMODE?MENU_BACKGROUND_BITMAP_HIRES:MENU_BACKGROUND_BITMAP_LORES)

// Draws the frame background for menus
void nm_draw_background(int x1, int y1, int x2, int y2 )
{
	int w,h,init_sub=0;
	static float BGScaleX=1,BGScaleY=1;
	grs_canvas *tmp,*old;

	if (nm_background.bm_data == NULL)
	{
		int pcx_error;
		ubyte background_palette[768];
		gr_init_bitmap_data (&nm_background);
		pcx_error = pcx_read_bitmap(MENU_BACKGROUND_BITMAP,&nm_background,BM_LINEAR,background_palette);
		Assert(pcx_error == PCX_ERROR_NONE);
		gr_remap_bitmap_good( &nm_background, background_palette, -1, -1 );
		BGScaleX=((float)SWIDTH/nm_background.bm_w);
		BGScaleY=((float)SHEIGHT/nm_background.bm_h);
		init_sub=1;
	}

	if ( x1 < 0 ) x1 = 0;
	if ( y1 < 0 ) y1 = 0;
	if ( x2 > SWIDTH - 1) x2 = SWIDTH - 1;
	if ( y2 > SHEIGHT - 1) y2 = SHEIGHT - 1;

	w = x2-x1;
	h = y2-y1;

	if (w > SWIDTH) w = SWIDTH;
	if (h > SHEIGHT) h = SHEIGHT;

	old=grd_curcanv;
	tmp=gr_create_sub_canvas(old,x1,y1,w,h);
	gr_set_current_canvas(tmp);
	gr_palette_load( gr_palette );

	show_fullscr( &nm_background ); // show so we load all necessary data for the sub-bitmap
	if (!init_sub && ((nm_background_sub->bm_w != w*(((float) nm_background.bm_w)/SWIDTH)) || (nm_background_sub->bm_h != h*(((float) nm_background.bm_h)/SHEIGHT))))
	{
		init_sub=1;
		gr_free_sub_bitmap(nm_background_sub);
		nm_background_sub = NULL;
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
void nm_string( int w1,int x, int y, char * s)
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
	char *p,*s1;

	s1=NULL;

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
	fix time = timer_get_fixed_seconds();
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
		case NM_TYPE_SLIDER:
		{
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
			if ( item->group==0 )
			{
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
		case NM_TYPE_NUMBER:
		{
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

int newmenu_do( char * title, char * subtitle, int nitems, newmenu_item * item, int (*subfunction)(newmenu *menu, d_event *event, void *userdata), void *userdata )
{
	return newmenu_do3( title, subtitle, nitems, item, subfunction, userdata, 0, NULL, -1, -1 );
}

int newmenu_dotiny( char * title, char * subtitle, int nitems, newmenu_item * item, int (*subfunction)(newmenu *menu, d_event *event, void *userdata), void *userdata )
{
        return newmenu_do4( title, subtitle, nitems, item, subfunction, userdata, 0, NULL, -1, -1, 1 );
}


int newmenu_do1( char * title, char * subtitle, int nitems, newmenu_item * item, int (*subfunction)(newmenu *menu, d_event *event, void *userdata), void *userdata, int citem )
{
	return newmenu_do3( title, subtitle, nitems, item, subfunction, userdata, citem, userdata, -1, -1 );
}


int newmenu_do2( char * title, char * subtitle, int nitems, newmenu_item * item, int (*subfunction)(newmenu *menu, d_event *event, void *userdata), void *userdata, int citem, char * filename )
{
	return newmenu_do3( title, subtitle, nitems, item, subfunction, userdata, citem, filename, -1, -1 );
}
int newmenu_do3( char * title, char * subtitle, int nitems, newmenu_item * item, int (*subfunction)(newmenu *menu, d_event *event, void *userdata), void *userdata, int citem, char * filename, int width, int height )
{
	set_screen_mode(SCREEN_MENU);//hafta set the screen mode before calling or fonts might get changed/freed up if screen res changes
	return newmenu_do4( title, subtitle, nitems, item, subfunction, userdata, citem, filename, width, height, 0 );
}

int newmenu_do_fixedfont( char * title, char * subtitle, int nitems, newmenu_item * item, int (*subfunction)(newmenu *menu, d_event *event, void *userdata), void *userdata, int citem, char * filename, int width, int height){
	set_screen_mode(SCREEN_MENU);//hafta set the screen mode before calling or fonts might get changed/freed up if screen res changes
	return newmenu_do4( title, subtitle, nitems, item, subfunction, userdata, citem, filename, width, height, 0);
}


#ifdef NEWMENU_MOUSE
ubyte Hack_DblClick_MenuMode=0;
#endif

newmenu_item *newmenu_get_items(newmenu *menu)
{
	return menu->items;
}

int newmenu_get_nitems(newmenu *menu)
{
	return menu->nitems;
}

int newmenu_get_citem(newmenu *menu)
{
	return menu->citem;
}

int newmenu_idle(window *wind, d_event *event, newmenu *menu)
{
	int old_choice, i, k = -1;
	grs_canvas *menu_canvas = window_get_canvas(wind), *save_canvas = grd_curcanv;
	char *Temp,TempVal;
#ifdef NEWMENU_MOUSE
	int mx=0, my=0, mz=0, x1 = 0, x2, y1, y2;
#endif
	int rval = 0;
	
	timer_delay2(50);
	
#ifdef NEWMENU_MOUSE
	newmenu_show_cursor();      // possibly hidden
	menu->omouse_state = menu->mouse_state;
	menu->mouse_state = mouse_button_state(0);
#endif
	
	//see if redbook song needs to be restarted
	RBACheckFinishedHook();
	
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
	
	gr_set_current_canvas(menu_canvas);
	
	if (menu->tiny_mode)
		gr_set_curfont(GAME_FONT);
	else 
		gr_set_curfont(MEDIUM1_FONT);
	
	if (menu->subfunction)
	{
		if (k)
		{
			d_event_keycommand key_event;	// for now - will be passed through newmenu_handler later
			
			key_event.type = EVENT_KEY_COMMAND;
			key_event.keycode = k;
			
			rval = (*menu->subfunction)(menu, (d_event *)&key_event, menu->userdata);
			k = key_event.keycode;
		}
		else
			rval = (*menu->subfunction)(menu, event, menu->userdata);
	}
	
#ifdef NETWORK
	if ((rval >= -1) && !menu->time_stopped)	{
		// Save current menu box
		if (multi_menu_poll() == -1)
			rval = -2;
	}
#endif
	
	if (rval < -1)
	{
		menu->citem = rval;
		menu->done = 1;
		gr_set_current_canvas(save_canvas);
		return 1;
	}
	else if (rval)
		return 1;	// event handled already, but stay in newmenu
	
	old_choice = menu->citem;
	
	switch( k )	{
		case KEY_TAB + KEY_SHIFTED:
		case KEY_UP:
		case KEY_PAD8:
			if (menu->all_text) break;
			do {
				menu->citem--;
				
				if (menu->is_scroll_box)
				{
					menu->last_scroll_check=-1;
					if (menu->citem<0)
					{
						menu->citem=menu->nitems-1;
						menu->scroll_offset = menu->nitems-menu->max_on_menu;
						break;
					}
					
					if (menu->citem-4<menu->scroll_offset && menu->scroll_offset > 0)
					{
						menu->scroll_offset--;
					}
				}
				else
				{
					if (menu->citem >= menu->nitems ) menu->citem=0;
					if (menu->citem < 0 ) menu->citem=menu->nitems-1;
				}
			} while ( menu->items[menu->citem].type==NM_TYPE_TEXT );
			
			if ((menu->items[menu->citem].type==NM_TYPE_INPUT) && (menu->citem!=old_choice))	
				menu->items[menu->citem].value = -1;
			if ((old_choice>-1) && (menu->items[old_choice].type==NM_TYPE_INPUT_MENU) && (old_choice!=menu->citem))	{
				menu->items[old_choice].group=0;
				strcpy(menu->items[old_choice].text, menu->items[old_choice].saved_text );
				menu->items[old_choice].value = -1;
			}
			break;
			case KEY_TAB:
			case KEY_DOWN:
			case KEY_PAD2:
			if (menu->all_text) break;
			do {
				menu->citem++;
				
				if (menu->is_scroll_box)
				{
					menu->last_scroll_check=-1;
					
					if (menu->citem==menu->nitems)
					{ 
						menu->citem=0;
						menu->scroll_offset=0;
						break;
					}
					
					if (menu->citem+4>=menu->max_on_menu+menu->scroll_offset && menu->scroll_offset < menu->nitems-menu->max_on_menu)
					{
						menu->scroll_offset++;
					}
				}
				else
				{
					if (menu->citem < 0 ) menu->citem=menu->nitems-1;
					if (menu->citem >= menu->nitems ) menu->citem=0;
				}
				
			} while ( menu->items[menu->citem].type==NM_TYPE_TEXT );
			
			if ((menu->items[menu->citem].type==NM_TYPE_INPUT) && (menu->citem!=old_choice))	
				menu->items[menu->citem].value = -1;
			if ( (old_choice>-1) && (menu->items[old_choice].type==NM_TYPE_INPUT_MENU) && (old_choice!=menu->citem))	{
				menu->items[old_choice].group=0;
				strcpy(menu->items[old_choice].text, menu->items[old_choice].saved_text );	
				menu->items[old_choice].value = -1;
			}
			break;
			case KEY_SPACEBAR:
			if ( menu->citem > -1 )	{
				switch( menu->items[menu->citem].type )	{
					case NM_TYPE_MENU:
					case NM_TYPE_INPUT:
					case NM_TYPE_INPUT_MENU:
						break;
					case NM_TYPE_CHECK:
						if ( menu->items[menu->citem].value )
							menu->items[menu->citem].value = 0;
						else
							menu->items[menu->citem].value = 1;
						if (menu->is_scroll_box)
						{
							if (menu->citem==(menu->max_on_menu+menu->scroll_offset-1) || menu->citem==menu->scroll_offset)
							{
								menu->last_scroll_check=-1;
							}
						}
						break;
						case NM_TYPE_RADIO:
						for (i=0; i<menu->nitems; i++ )	{
							if ((i!=menu->citem) && (menu->items[i].type==NM_TYPE_RADIO) && (menu->items[i].group==menu->items[menu->citem].group) && (menu->items[i].value) )	{
								menu->items[i].value = 0;
							}
						}
						menu->items[menu->citem].value = 1;
						break;
				}	
			}
			break;
			
			case KEY_SHIFTED+KEY_UP:
			if (MenuReordering && menu->citem!=0)
			{
				Temp=menu->items[menu->citem].text;
				TempVal=menu->items[menu->citem].value;
				menu->items[menu->citem].text=menu->items[menu->citem-1].text;
				menu->items[menu->citem].value=menu->items[menu->citem-1].value;
				menu->items[menu->citem-1].text=Temp;
				menu->items[menu->citem-1].value=TempVal;
				menu->citem--;
			}
			break;
			case KEY_SHIFTED+KEY_DOWN:
			if (MenuReordering && menu->citem!=(menu->nitems-1))
			{
				Temp=menu->items[menu->citem].text;
				TempVal=menu->items[menu->citem].value;
				menu->items[menu->citem].text=menu->items[menu->citem+1].text;
				menu->items[menu->citem].value=menu->items[menu->citem+1].value;
				menu->items[menu->citem+1].text=Temp;
				menu->items[menu->citem+1].value=TempVal;
				menu->citem++;
			}
			break;
			case KEY_ENTER:
			case KEY_PADENTER:
			if ( (menu->citem>-1) && (menu->items[menu->citem].type==NM_TYPE_INPUT_MENU) && (menu->items[menu->citem].group==0))	{
				menu->items[menu->citem].group = 1;
				if ( !strnicmp( menu->items[menu->citem].saved_text, TXT_EMPTY, strlen(TXT_EMPTY) ) )	{
					menu->items[menu->citem].text[0] = 0;
					menu->items[menu->citem].value = -1;
				} else {	
					strip_end_whitespace(menu->items[menu->citem].text);
				}
			} else
			{
				// Tell callback, allow staying in menu
				event->type = EVENT_NEWMENU_SELECTED;
				if (menu->subfunction && (*menu->subfunction)(menu, event, menu->userdata))
					return 1;
				
				menu->done = 1;
				gr_set_current_canvas(save_canvas);
				return 1;
			}
			break;
			
			case KEY_ALTED+KEY_ENTER:
			case KEY_ALTED+KEY_PADENTER:
			gr_toggle_fullscreen();
			break;
			
			case KEY_ESC:
			if ( (menu->citem>-1) && (menu->items[menu->citem].type==NM_TYPE_INPUT_MENU) && (menu->items[menu->citem].group==1))	{
				menu->items[menu->citem].group=0;
				strcpy(menu->items[menu->citem].text, menu->items[menu->citem].saved_text );	
				menu->items[menu->citem].value = -1;
			} else {
				menu->done = 1;
				menu->citem = -1;
				gr_set_current_canvas(save_canvas);
				return 1;
			}
			break;
			
#ifdef macintosh
			case KEY_COMMAND+KEY_SHIFTED+KEY_3:
#endif
			case KEY_PRINT_SCREEN:
			save_screen_shot(0);
			break;
			
#ifndef NDEBUG
			case KEY_BACKSP:	
			if ( (menu->citem>-1) && (menu->items[menu->citem].type!=NM_TYPE_INPUT)&&(menu->items[menu->citem].type!=NM_TYPE_INPUT_MENU))
				Int3(); 
			break;
#endif
			
	}
	
#ifdef NEWMENU_MOUSE // for mouse selection of menu's etc.
	if ( menu->mouse_state && !menu->omouse_state && !menu->all_text ) {
		mouse_get_pos(&mx, &my, &mz);
		for (i=0; i<menu->max_on_menu; i++ )	{
			x1 = grd_curcanv->cv_bitmap.bm_x + menu->items[i].x-FSPACX(13) /*- menu->items[i].right_offset - 6*/;
			x2 = x1 + menu->items[i].w+FSPACX(13);
			y1 = grd_curcanv->cv_bitmap.bm_y + menu->items[i].y;
			y2 = y1 + menu->items[i].h;
			if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2))) {
				if (i+menu->scroll_offset != menu->citem) {
					if(Hack_DblClick_MenuMode) menu->dblclick_flag = 0; 
				}
				
				menu->citem = i + menu->scroll_offset;
				
				switch( menu->items[menu->citem].type )	{
					case NM_TYPE_CHECK:
						if ( menu->items[menu->citem].value )
							menu->items[menu->citem].value = 0;
						else
							menu->items[menu->citem].value = 1;
						
						if (menu->is_scroll_box)
							menu->last_scroll_check=-1;
						break;
						case NM_TYPE_RADIO:
						for (i=0; i<menu->nitems; i++ )	{
							if ((i!=menu->citem) && (menu->items[i].type==NM_TYPE_RADIO) && (menu->items[i].group==menu->items[menu->citem].group) && (menu->items[i].value) )	{
								menu->items[i].value = 0;
							}
						}
						menu->items[menu->citem].value = 1;
						break;
						case NM_TYPE_TEXT:
						menu->citem=old_choice;
						menu->mouse_state=0;
						break;
				}
				break;
			}
		}
	}
	
	if (menu->mouse_state && menu->all_text)
	{
		menu->done = 1;
		gr_set_current_canvas(save_canvas);
		return 1;
	}
	
	if ( menu->mouse_state && !menu->all_text ) {
		mouse_get_pos(&mx, &my, &mz);
		
		// check possible scrollbar stuff first
		if (menu->is_scroll_box) {
			int arrow_width, arrow_height, aw, ScrollAllow=0, time=timer_get_fixed_seconds();
			static fix ScrollTime=0;
			if (ScrollTime + F1_0/5 < time || time < ScrollTime)
			{
				ScrollTime = time;
				ScrollAllow = 1;
			}
			
			if (menu->scroll_offset != 0) {
				gr_get_string_size(UP_ARROW_MARKER, &arrow_width, &arrow_height, &aw);
				x2 = grd_curcanv->cv_bitmap.bm_x + menu->items[menu->scroll_offset].x-FSPACX(13);
				y1 = grd_curcanv->cv_bitmap.bm_y + menu->items[menu->scroll_offset].y-(((int)LINE_SPACING)*menu->scroll_offset);
				x1 = x2 - arrow_width;
				y2 = y1 + arrow_height;
				if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) && ScrollAllow) {
					menu->citem--;
					menu->last_scroll_check=-1;
					if (menu->citem-4<menu->scroll_offset && menu->scroll_offset > 0)
					{
						menu->scroll_offset--;
					}
				}
			}
			if (menu->scroll_offset+menu->max_displayable<menu->nitems) {
				gr_get_string_size(DOWN_ARROW_MARKER, &arrow_width, &arrow_height, &aw);
				x2 = grd_curcanv->cv_bitmap.bm_x + menu->items[menu->scroll_offset+menu->max_displayable-1].x-FSPACX(13);
				y1 = grd_curcanv->cv_bitmap.bm_y + menu->items[menu->scroll_offset+menu->max_displayable-1].y-(((int)LINE_SPACING)*menu->scroll_offset);
				x1 = x2 - arrow_width;
				y2 = y1 + arrow_height;
				if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) && ScrollAllow) {
					menu->citem++;
					menu->last_scroll_check=-1;
					if (menu->citem+4>=menu->max_on_menu+menu->scroll_offset && menu->scroll_offset < menu->nitems-menu->max_on_menu)
					{
						menu->scroll_offset++;
					}
				}
			}
		}
		
		for (i=0; i<menu->max_on_menu; i++ )	{
			x1 = grd_curcanv->cv_bitmap.bm_x + menu->items[i].x-FSPACX(13);
			x2 = x1 + menu->items[i].w+FSPACX(13);
			y1 = grd_curcanv->cv_bitmap.bm_y + menu->items[i].y;
			y2 = y1 + menu->items[i].h;
			
			if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) && (menu->items[i].type != NM_TYPE_TEXT) ) {
				if (i+menu->scroll_offset != menu->citem) {
					if(Hack_DblClick_MenuMode) menu->dblclick_flag = 0; 
				}
				
				menu->citem = i + menu->scroll_offset;
				
				if ( menu->items[menu->citem].type == NM_TYPE_SLIDER ) {
					char slider_text[NM_MAX_TEXT_LEN+1], *p, *s1;
					int slider_width, height, aw, sleft_width, sright_width, smiddle_width;
					
					strcpy(slider_text, menu->items[menu->citem].saved_text);
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
						
						x1 = grd_curcanv->cv_bitmap.bm_x + menu->items[menu->citem].x + menu->items[menu->citem].w - slider_width;
						x2 = x1 + slider_width + sright_width;
						if ( (mx > x1) && (mx < (x1 + sleft_width)) && (menu->items[menu->citem].value != menu->items[menu->citem].min_value) ) {
							menu->items[menu->citem].value = menu->items[menu->citem].min_value;
						} else if ( (mx < x2) && (mx > (x2 - sright_width)) && (menu->items[menu->citem].value != menu->items[menu->citem].max_value) ) {
							menu->items[menu->citem].value = menu->items[menu->citem].max_value;
						} else if ( (mx > (x1 + sleft_width)) && (mx < (x2 - sright_width)) ) {
							int num_values, value_width, new_value;
							
							num_values = menu->items[menu->citem].max_value - menu->items[menu->citem].min_value + 1;
							value_width = (slider_width - sleft_width - sright_width) / num_values;
							new_value = (mx - x1 - sleft_width) / value_width;
							if ( menu->items[menu->citem].value != new_value ) {
								menu->items[menu->citem].value = new_value;
							}
						}
						*p = '\t';
					}
				}
				if (menu->citem == old_choice)
					break;
				if ((menu->items[menu->citem].type==NM_TYPE_INPUT) && (menu->citem!=old_choice))
					menu->items[menu->citem].value = -1;
				if ((old_choice>-1) && (menu->items[old_choice].type==NM_TYPE_INPUT_MENU) && (old_choice!=menu->citem))	{
					menu->items[old_choice].group=0;
					strcpy(menu->items[old_choice].text, menu->items[old_choice].saved_text );
					menu->items[old_choice].value = -1;
				}
				break;
			}
		}
	}
	
	if ( !menu->mouse_state && menu->omouse_state && !menu->all_text && (menu->citem != -1) && (menu->items[menu->citem].type == NM_TYPE_MENU) ) {
		mouse_get_pos(&mx, &my, &mz);
		for (i=0; i<menu->nitems; i++ )	{
			x1 = grd_curcanv->cv_bitmap.bm_x + menu->items[i].x-FSPACX(13);
			x2 = x1 + menu->items[i].w+FSPACX(13);
			y1 = grd_curcanv->cv_bitmap.bm_y + menu->items[i].y;
			y2 = y1 + menu->items[i].h;
			if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2))) {
				if (Hack_DblClick_MenuMode) {
					if (menu->dblclick_flag)
					{
						// Tell callback, allow staying in menu
						event->type = EVENT_NEWMENU_SELECTED;
						if (menu->subfunction && (*menu->subfunction)(menu, event, menu->userdata))
							return 1;
						
						menu->done = 1;
						gr_set_current_canvas(save_canvas);
						return 1;
					}
					else menu->dblclick_flag = 1;
				}
				else
				{
					// Tell callback, allow staying in menu
					event->type = EVENT_NEWMENU_SELECTED;
					if (menu->subfunction && (*menu->subfunction)(menu, event, menu->userdata))
						return 1;
					
					menu->done = 1;
					gr_set_current_canvas(save_canvas);
					return 1;
				}
			}
		}
	}
	
	if ( !menu->mouse_state && menu->omouse_state && (menu->citem>-1) && (menu->items[menu->citem].type==NM_TYPE_INPUT_MENU) && (menu->items[menu->citem].group==0))	{
		menu->items[menu->citem].group = 1;
		if ( !strnicmp( menu->items[menu->citem].saved_text, TXT_EMPTY, strlen(TXT_EMPTY) ) )	{
			menu->items[menu->citem].text[0] = 0;
			menu->items[menu->citem].value = -1;
		} else {
			strip_end_whitespace(menu->items[menu->citem].text);
		}
	}
	
#endif // NEWMENU_MOUSE
	
	if ( menu->citem > -1 )	{
		int ascii;
		
		if ( ((menu->items[menu->citem].type==NM_TYPE_INPUT)||((menu->items[menu->citem].type==NM_TYPE_INPUT_MENU)&&(menu->items[menu->citem].group==1)) )&& (old_choice==menu->citem) )	{
			if ( k==KEY_LEFT || k==KEY_BACKSP || k==KEY_PAD4 )	{
				if (menu->items[menu->citem].value==-1) menu->items[menu->citem].value = strlen(menu->items[menu->citem].text);
				if (menu->items[menu->citem].value > 0)
					menu->items[menu->citem].value--;
				menu->items[menu->citem].text[menu->items[menu->citem].value] = 0;
			} else {
				ascii = key_ascii();
				if ((ascii < 255 ) && (menu->items[menu->citem].value < menu->items[menu->citem].text_len ))
				{
					int allowed;
					
					if (menu->items[menu->citem].value==-1) {
						menu->items[menu->citem].value = 0;
					}
					
					allowed = char_allowed(ascii);
					
					if (!allowed && ascii==' ' && char_allowed('_')) {
						ascii = '_';
						allowed=1;
					}
					
					if (allowed) {
						menu->items[menu->citem].text[menu->items[menu->citem].value++] = ascii;
						menu->items[menu->citem].text[menu->items[menu->citem].value] = 0;
					}
				}
			}
		}
		else if ((menu->items[menu->citem].type!=NM_TYPE_INPUT) && (menu->items[menu->citem].type!=NM_TYPE_INPUT_MENU) )
		{
			ascii = key_ascii();
			if (ascii < 255 ) {
				int choice1 = menu->citem;
				ascii = toupper(ascii);
				do {
					int i,ch;
					choice1++;
					if (choice1 >= menu->nitems )
						choice1=0;
					
					for (i=0;(ch=menu->items[choice1].text[i])!=0 && ch==' ';i++);
					
					if ( ( (menu->items[choice1].type==NM_TYPE_MENU) ||
						  (menu->items[choice1].type==NM_TYPE_CHECK) ||
						  (menu->items[choice1].type==NM_TYPE_RADIO) ||
						  (menu->items[choice1].type==NM_TYPE_NUMBER) ||
						  (menu->items[choice1].type==NM_TYPE_SLIDER) )
						&& (ascii==toupper(ch)) )
					{
						k = 0;
						menu->citem = choice1;
					}
					
					while (menu->citem > menu->scroll_offset+menu->max_displayable-1)
					{
						menu->scroll_offset++;
					}
					while (menu->citem < menu->scroll_offset)
					{
						menu->scroll_offset--;
					}
					
				} while (choice1 != menu->citem );
			}
		}
		
		if ( (menu->items[menu->citem].type==NM_TYPE_NUMBER) || (menu->items[menu->citem].type==NM_TYPE_SLIDER))
		{
			switch( k ) {
				case KEY_PAD4:
				case KEY_LEFT:
				case KEY_MINUS:
				case KEY_MINUS+KEY_SHIFTED:
				case KEY_PADMINUS:
					menu->items[menu->citem].value -= 1;
					break;
				case KEY_RIGHT:
				case KEY_PAD6:
				case KEY_EQUAL:
				case KEY_EQUAL+KEY_SHIFTED:
				case KEY_PADPLUS:
					menu->items[menu->citem].value++;
					break;
				case KEY_PAGEUP:
				case KEY_PAD9:
				case KEY_SPACEBAR:
					menu->items[menu->citem].value += 10;
					break;
				case KEY_PAGEDOWN:
				case KEY_BACKSP:
				case KEY_PAD3:
					menu->items[menu->citem].value -= 10;
					break;
			}
		}
		
	}
	
#ifdef NEWMENU_MOUSE
	newmenu_show_cursor();
#endif
	for (i=menu->scroll_offset; i<menu->max_displayable+menu->scroll_offset; i++ )
		if (i==menu->citem && (menu->items[i].type==NM_TYPE_INPUT || (menu->items[i].type==NM_TYPE_INPUT_MENU && menu->items[i].group)))
			update_cursor( &menu->items[i],menu->scroll_offset);
	
	gr_set_current_canvas(save_canvas);

	return 0;
}

int newmenu_draw(window *wind, d_event *event, newmenu *menu)
{
	grs_canvas *menu_canvas = window_get_canvas(wind), *save_canvas = grd_curcanv;
	int tw, th, ty, sx, sy;
	int i;
	int string_width, string_height, average_width;
	
	gr_set_current_canvas( NULL );
#ifdef OGL
	nm_draw_background1(menu->filename);
#endif
	if (menu->filename == NULL)
		nm_draw_background(menu->x-(menu->is_scroll_box?FSPACX(5):0),menu->y,menu->x+menu->w,menu->y+menu->h);
	
	gr_set_current_canvas( menu_canvas );
	
	ty = BORDERY;
	
	if ( menu->title )	{
		gr_set_curfont(HUGE_FONT);
		gr_set_fontcolor( BM_XRGB(31,31,31), -1 );
		gr_get_string_size(menu->title,&string_width,&string_height,&average_width );
		tw = string_width;
		th = string_height;
		gr_printf( 0x8000, ty, menu->title );
	}
	
	if ( menu->subtitle )	{
		gr_set_curfont(MEDIUM3_FONT);
		gr_set_fontcolor( BM_XRGB(21,21,21), -1 );
		gr_get_string_size(menu->subtitle,&string_width,&string_height,&average_width );
		tw = string_width;
		th = (menu->title?th:0);
		gr_printf( 0x8000, ty+th, menu->subtitle );
	}
	
	if (menu->tiny_mode)
		gr_set_curfont(GAME_FONT);
	else 
		gr_set_curfont(MEDIUM1_FONT);
	
	// Redraw everything...
	for (i=menu->scroll_offset; i<menu->max_displayable+menu->scroll_offset; i++ )
	{
		menu->items[i].y-=(((int)LINE_SPACING)*menu->scroll_offset);
		draw_item( &menu->items[i], (i==menu->citem && !menu->all_text),menu->tiny_mode );
		menu->items[i].y+=(((int)LINE_SPACING)*menu->scroll_offset);
		
	}
	
	if (menu->is_scroll_box)
	{
		menu->last_scroll_check=menu->scroll_offset;
		gr_set_curfont(MEDIUM2_FONT);
		
		sy=menu->items[menu->scroll_offset].y-(((int)LINE_SPACING)*menu->scroll_offset);
		sx=menu->items[menu->scroll_offset].x-FSPACX(11);
		
		
		if (menu->scroll_offset!=0)
			nm_rstring( FSPACX(11), sx, sy, UP_ARROW_MARKER );
		else
			nm_rstring( FSPACX(11), sx, sy, "  " );
		
		sy=menu->items[menu->scroll_offset+menu->max_displayable-1].y-(((int)LINE_SPACING)*menu->scroll_offset);
		sx=menu->items[menu->scroll_offset+menu->max_displayable-1].x-FSPACX(11);
		
		if (menu->scroll_offset+menu->max_displayable<menu->nitems)
			nm_rstring( FSPACX(11), sx, sy, DOWN_ARROW_MARKER );
		else
			nm_rstring( FSPACX(11), sx, sy, "  " );
		
	}
	
	// Only allow drawing over the top of the default stuff
	if (menu->subfunction)
		(*menu->subfunction)(menu, event, menu->userdata);
	
	gr_set_current_canvas(save_canvas);
	
	return 1;
}

int newmenu_handler(window *wind, d_event *event, newmenu *menu)
{
	switch (event->type)
	{
		case EVENT_IDLE:
			return newmenu_idle(wind, event, menu);
			break;
			
		case EVENT_WINDOW_DRAW:
			return newmenu_draw(wind, event, menu);
			break;
			
		case EVENT_WINDOW_CLOSE:
			// Don't allow cancel here - handle item selected events / key events instead
			if (menu->subfunction)
				(*menu->subfunction)(menu, event, menu->userdata);
			
			newmenu_hide_cursor();
			game_flush_inputs();
			
			if (menu->time_stopped)
				start_time();
			
			if ( menu->sound_stopped )
				digi_resume_digi_sounds();
			
			// d_free(menu);	// have to wait until newmenus use a separate event loop
			return 0;	// continue closing
			break;
			
		default:
			if (menu->subfunction)
				return (*menu->subfunction)(menu, event, menu->userdata);
			else
				return 0;
			break;
	}
	
	return 1;
}

int newmenu_do4( char * title, char * subtitle, int nitems, newmenu_item * item, int (*subfunction)(newmenu *menu, d_event *event, void *userdata), void *userdata, int citem, char * filename, int width, int height, int TinyMode )
{
	window *wind = NULL;
	newmenu *menu;
	int i,j,aw, tw, th, twidth,fm,right_offset;
	int nmenus, nothers;
	grs_font * save_font;
	int string_width, string_height, average_width;
	grs_canvas *menu_canvas, *save_canvas;
	int rval = -1;

	MALLOC(menu, newmenu, 1);
	if (!menu)
		return -1;
	
	menu->scroll_offset = 0;
	menu->last_scroll_check = -1;
	menu->all_text = 0;
	menu->is_scroll_box = 0;
	menu->sound_stopped = menu->time_stopped = 0;
	menu->max_on_menu = MAXDISPLAYABLEITEMS;
	menu->dblclick_flag = 0;
	menu->title = title;
	menu->subtitle = subtitle;
	menu->nitems = nitems;
	menu->subfunction = subfunction;
	menu->items = item;
	menu->filename = filename;
	menu->tiny_mode = TinyMode;
	menu->done = 0;
	menu->userdata = userdata;
	
	newmenu_close();
	newmenu_hide_cursor();

	if (nitems < 1 )
	{
		d_free(menu);
		return -1;
	}

	menu->max_displayable=nitems;

	if ( Function_mode == FMODE_GAME && !((Game_mode & GM_MULTI) && (Newdemo_state != ND_STATE_PLAYBACK))) {
		digi_pause_digi_sounds();
		menu->sound_stopped = 1;
	}

	if (!(((Game_mode & GM_MULTI) && (Newdemo_state != ND_STATE_PLAYBACK)) && (Function_mode == FMODE_GAME) && (!Endlevel_sequence)) )
	{
		menu->time_stopped = 1;
		stop_time();
	}

	save_canvas = grd_curcanv;

	gr_set_current_canvas(NULL);

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

	th += FSPACY(5);		//put some space between titles & body

	if (TinyMode)
		gr_set_curfont(GAME_FONT);
	else 
		gr_set_curfont(MEDIUM1_FONT);

	menu->w = aw = 0;
	menu->h = th;
	nmenus = nothers = 0;

	// Find menu height & width (store in w,h)
	for (i=0; i<nitems; i++ )	{
		item[i].y = menu->h;
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

		if ( string_width > menu->w )
			menu->w = string_width;		// Save maximum width
		if ( average_width > aw )
			aw = average_width;
		menu->h += string_height+FSPACY(1);		// Find the height of all strings
	}

	if (!TinyMode && i > menu->max_on_menu)
	{
		menu->is_scroll_box=1;
		menu->h=((menu->max_on_menu+(subtitle?1:0))*LINE_SPACING);
		menu->max_displayable=menu->max_on_menu;
		
		// if our last citem was > menu->max_on_menu, make sure we re-scroll when we call this menu again
		if (citem > menu->max_on_menu-4)
		{
			menu->scroll_offset = citem - (menu->max_on_menu-4);
			if (menu->scroll_offset + menu->max_on_menu > nitems)
				menu->scroll_offset = nitems - menu->max_on_menu;
		}
	}
	else
	{
		menu->is_scroll_box=0;
		menu->max_on_menu=i;
	}

	right_offset=0;

	if ( width > -1 )
		menu->w = width;

	if ( height > -1 )
		menu->h = height;

	for (i=0; i<nitems; i++ )	{
		item[i].w = menu->w;
		if (item[i].right_offset > right_offset )
			right_offset = item[i].right_offset;
	}

	menu->w += right_offset;

	twidth = 0;
	if ( tw > menu->w )	{
		twidth = ( tw - menu->w )/2;
		menu->w = tw;
	}

	// Find min point of menu border
	menu->w += BORDERX*2;
	menu->h += BORDERY*2;

	menu->x = (GWIDTH-menu->w)/2;
	menu->y = (GHEIGHT-menu->h)/2;

	if ( menu->x < 0 ) menu->x = 0;
	if ( menu->y < 0 ) menu->y = 0;

	nm_draw_background1( menu->filename );

	// Create the basic window
	if (menu)
		wind = window_create(&grd_curscreen->sc_canvas, menu->x, menu->y, menu->w, menu->h, (int (*)(window *, d_event *, void *))newmenu_handler, menu);
	if (!wind)
	{
		if (menu->time_stopped)
			start_time();
		
		if ( menu->sound_stopped )
			digi_resume_digi_sounds();
		
		d_free(menu);
		newmenu_hide_cursor();

		return -1;
	}

	menu_canvas = window_get_canvas(wind);
	gr_set_curfont(save_font);
	gr_set_current_canvas(menu_canvas);

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

	if (citem==-1)	{
		menu->citem = -1;
	} else {
		if (citem < 0 ) citem = 0;
		if (citem > nitems-1 ) citem = nitems-1;
		menu->citem = citem;

#ifdef NEWMENU_MOUSE
		menu->dblclick_flag = 1;
#endif

		while ( item[menu->citem].type==NM_TYPE_TEXT )	{
			menu->citem++;
			if (menu->citem >= nitems ) {
				menu->citem=0; 
			}
			if (menu->citem == citem ) {
				menu->citem=0; 
				menu->all_text=1;
				break; 
			}
		}
	}

	// Clear mouse, joystick to clear button presses.
	game_flush_inputs();

#ifdef NEWMENU_MOUSE
	menu->mouse_state = menu->omouse_state = 0;
	newmenu_show_cursor();
#endif

	gr_set_current_canvas(save_canvas);

	// All newmenus get their own event loop, for now.
	while (!menu->done)
		event_process();

	window_close(wind);

	rval = menu->citem;
	d_free(menu);
	return rval;
}


int nm_messagebox1( char *title, int (*subfunction)(newmenu *menu, d_event *event, void *userdata), void *userdata, int nchoices, ... )
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
	strcpy( nm_text, "" );
	vsprintf(nm_text,format,args);
	va_end(args);

	Assert(strlen(nm_text) < MESSAGEBOX_TEXT_SIZE);

	return newmenu_do( title, nm_text, nchoices, nm_message_items, subfunction, userdata );
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
	strcpy( nm_text, "" );
	vsprintf(nm_text,format,args);
	va_end(args);

	Assert(strlen(nm_text) < MESSAGEBOX_TEXT_SIZE );

	return newmenu_do( title, nm_text, nchoices, nm_message_items, NULL, NULL );
}

// Example listbox callback function...
// int lb_callback( int * citem, int *nitems, char * items[], int *keypress )
// {
// 	int i;
// 
// 	if ( *keypress = KEY_CTRLED+KEY_D )	{
// 		if ( *nitems > 1 )	{
// 			PHYSFS_delete(items[*citem]);     // Delete the file
// 			for (i=*citem; i<*nitems-1; i++ )	{
// 				items[i] = items[i+1];
// 			}
// 			*nitems = *nitems - 1;
// 			d_free( items[*nitems] );
// 			items[*nitems] = NULL;
// 			return 1;	// redraw;
// 		}
//			*keypress = 0;
// 	}			
// 	return 0;
// }

#define LB_ITEMS_ON_SCREEN 8

struct listbox
{
	char *title;
	int nitems;
	char **item;
	int allow_abort_flag;
	int (*listbox_callback)(listbox *lb, d_event *event, void *userdata);
	int done, citem, first_item;
	int box_w, height, box_x, box_y, title_height;
	int mouse_state, omouse_state;
	void *userdata;
};

char **listbox_get_items(listbox *lb)
{
	return lb->item;
}

int listbox_get_nitems(listbox *lb)
{
	return lb->nitems;
}

int listbox_get_citem(listbox *lb)
{
	return lb->citem;
}

void listbox_delete_item(listbox *lb, int item)
{
	int i;
	
	Assert(item >= 0);

	if (lb->nitems)
	{
		for (i=item; i<lb->nitems-1; i++ )
			lb->item[i] = lb->item[i+1];
		lb->nitems--;
		lb->item[lb->nitems] = NULL;
		
		if (lb->citem >= lb->nitems)
			lb->citem = lb->nitems ? lb->nitems - 1 : 0;
	}
}

int listbox_idle(window *wind, d_event *event, listbox *lb)
{
	int i;
	int ocitem, ofirst_item, key;
#ifdef NEWMENU_MOUSE
	int mx, my, mz, x1, x2, y1, y2;	//, dblclick_flag;
#endif
	int rval = 0;
	
	timer_delay2(50);
	ocitem = lb->citem;
	ofirst_item = lb->first_item;
	
#ifdef NEWMENU_MOUSE
	lb->omouse_state = lb->mouse_state;
	lb->mouse_state = mouse_button_state(0);
#endif
	
	//see if redbook song needs to be restarted
	RBACheckFinishedHook();
	
	key = key_inkey();
	
	if ( lb->listbox_callback )
	{
		if (key)
		{
			d_event_keycommand key_event;	// somewhere to put the keycode for now
			
			key_event.type = EVENT_KEY_COMMAND;
			key_event.keycode = key;
			
			rval = (*lb->listbox_callback)(lb, (d_event *)&key_event, lb->userdata);
			key = key_event.keycode;
		}
		else
			rval = (*lb->listbox_callback)(lb, event, lb->userdata);
	}
	
	if (rval < -1) {
		lb->citem = rval;
		lb->done = 1;
		return 1;
	}
	else if (rval)
		return 1;	// event handled already, but stay in listbox
	
	switch(key)	{
#ifdef macintosh
		case KEY_COMMAND+KEY_SHIFTED+KEY_3:
#endif
		case KEY_PRINT_SCREEN: 		
			save_screen_shot(0); 
			break;
		case KEY_HOME:
		case KEY_PAD7:
			lb->citem = 0;
			break;
		case KEY_END:
		case KEY_PAD1:
			lb->citem = lb->nitems-1;
			break;
		case KEY_UP:
		case KEY_PAD8:
			lb->citem--;
			break;
		case KEY_DOWN:
		case KEY_PAD2:
			lb->citem++;
			break;
		case KEY_PAGEDOWN:
		case KEY_PAD3:
			lb->citem += LB_ITEMS_ON_SCREEN;
			break;
		case KEY_PAGEUP:
		case KEY_PAD9:
			lb->citem -= LB_ITEMS_ON_SCREEN;
			break;
		case KEY_ESC:
			if (lb->allow_abort_flag) {
				lb->citem = -1;
				lb->done = 1;
				return 1;
			}
			break;
		case KEY_ENTER:
		case KEY_PADENTER:
			// Tell callback, allow staying in menu
			event->type = EVENT_NEWMENU_SELECTED;
			if (lb->listbox_callback && (*lb->listbox_callback)(lb, event, lb->userdata))
				return 1;
			
			lb->done = 1;
			return 1;
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
				cc=cc1=lb->citem+1;
				if (cc1 < 0 )  cc1 = 0;
				if (cc1 >= lb->nitems )  cc1 = 0;
				while(1) {
					if ( cc < 0 ) cc = 0;
					if ( cc >= lb->nitems ) cc = 0;
					if ( lb->citem == cc ) break;
					
					if ( toupper( lb->item[cc][0] ) == toupper(ascii) )	{
						lb->citem = cc;
						break;
					}
					cc++;
				}
			}
		}
	}
	
	if (lb->citem<0)
		lb->citem=0;
	
	if (lb->citem>=lb->nitems)
		lb->citem = lb->nitems-1;
	
	if (lb->citem< lb->first_item)
		lb->first_item = lb->citem;
	
	if (lb->citem>=( lb->first_item+LB_ITEMS_ON_SCREEN))
		lb->first_item = lb->citem-LB_ITEMS_ON_SCREEN+1;
	
	if (lb->nitems <= LB_ITEMS_ON_SCREEN )
		lb->first_item = 0;
	
	if (lb->first_item>lb->nitems-LB_ITEMS_ON_SCREEN)
		lb->first_item = lb->nitems-LB_ITEMS_ON_SCREEN;
	if (lb->first_item < 0 ) lb->first_item = 0;
	
#ifdef NEWMENU_MOUSE
	if (lb->mouse_state) {
		int w, h, aw;
		
		mouse_get_pos(&mx, &my, &mz);
		for (i=lb->first_item; i<lb->first_item+LB_ITEMS_ON_SCREEN; i++ )	{
			if (i > lb->nitems)
				break;
			gr_get_string_size(lb->item[i], &w, &h, &aw  );
			x1 = lb->box_x;
			x2 = lb->box_x + lb->box_w;
			y1 = (i-lb->first_item)*LINE_SPACING+lb->box_y;
			y2 = y1+h;
			if ( ((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) ) {
				lb->citem = i;
				
				// Tell callback, allow staying in menu
				event->type = EVENT_NEWMENU_SELECTED;
				if (lb->listbox_callback && (*lb->listbox_callback)(lb, event, lb->userdata))
					return 1;
				
				lb->done = 1;
				return 1;
				break;
			}
		}
	}
#endif
	
	return 0;
}

int listbox_draw(window *wind, d_event *event, listbox *lb)
{
	int i;

#ifdef OGL
	nm_draw_background1(NULL);
#endif
	nm_draw_background( lb->box_x-BORDERX,lb->box_y-lb->title_height-BORDERY,lb->box_x+lb->box_w+BORDERX,lb->box_y+lb->height+BORDERY );
	gr_set_curfont(MEDIUM3_FONT);
	gr_string( 0x8000, lb->box_y - lb->title_height, lb->title );
	
	gr_setcolor( BM_XRGB( 0,0,0)  );
	for (i=lb->first_item; i<lb->first_item+LB_ITEMS_ON_SCREEN; i++ )	{
		int y = (i-lb->first_item)*LINE_SPACING+lb->box_y;
		if ( i >= lb->nitems )	{
			gr_setcolor( BM_XRGB(5,5,5));
			gr_rect( lb->box_x + lb->box_w - FSPACX(1), y-FSPACY(1), lb->box_x + lb->box_w, y + LINE_SPACING);
			gr_setcolor( BM_XRGB(2,2,2));
			gr_rect( lb->box_x - FSPACX(1), y - FSPACY(1), lb->box_x, y + LINE_SPACING );
			gr_setcolor( BM_XRGB(0,0,0));
			gr_rect( lb->box_x, y - FSPACY(1), lb->box_x + lb->box_w - FSPACX(1), y + LINE_SPACING);
		} else {
			if ( i == lb->citem )	
				gr_set_curfont(MEDIUM2_FONT);
			else	
				gr_set_curfont(MEDIUM1_FONT);
			gr_setcolor( BM_XRGB(5,5,5));
			gr_rect( lb->box_x + lb->box_w - FSPACX(1), y-FSPACY(1), lb->box_x + lb->box_w, y + LINE_SPACING);
			gr_setcolor( BM_XRGB(2,2,2));
			gr_rect( lb->box_x - FSPACX(1), y - FSPACY(1), lb->box_x, y + LINE_SPACING );
			gr_setcolor( BM_XRGB(0,0,0));
			gr_rect( lb->box_x, y - FSPACY(1), lb->box_x + lb->box_w - FSPACX(1), y + LINE_SPACING);
			gr_string( lb->box_x+FSPACX(5), y, lb->item[i]  );
		}
	}
	
	// Only allow drawing over the top of the default stuff
	if ( lb->listbox_callback )
		(*lb->listbox_callback)(lb, event, lb->userdata);
	
	return 1;
}

int listbox_handler(window *wind, d_event *event, listbox *lb)
{
	switch (event->type)
	{
		case EVENT_IDLE:
			return listbox_idle(wind, event, lb);
			break;
			
		case EVENT_WINDOW_DRAW:
			return listbox_draw(wind, event, lb);
			break;
			
		case EVENT_WINDOW_CLOSE:
			// Don't allow cancel here - handle item selected events / key events instead
			if (lb->listbox_callback)
				(*lb->listbox_callback)(lb, event, lb->userdata);
			
			newmenu_hide_cursor();
			return 0;	// continue closing
			break;
			
		default:
			if (lb->listbox_callback)
				return (*lb->listbox_callback)(lb, event, lb->userdata);
			else
				return 0;
			break;
	}

	return 1;
}

int newmenu_listbox( char * title, int nitems, char * items[], int allow_abort_flag, int (*listbox_callback)(listbox *lb, d_event *event, void *userdata), void *userdata )
{
	return newmenu_listbox1( title, nitems, items, allow_abort_flag, 0, listbox_callback, userdata );
}

int newmenu_listbox1( char * title, int nitems, char * items[], int allow_abort_flag, int default_item, int (*listbox_callback)(listbox *lb, d_event *event, void *userdata), void *userdata )
{
	listbox *lb;
	window *wind;
	int i, rval = -1;

	MALLOC(lb, listbox, 1);
	if (!lb)
		return -1;
	
	newmenu_close();
	
	lb->title = title;
	lb->nitems = nitems;
	lb->item = items;
	lb->allow_abort_flag = allow_abort_flag;
	lb->listbox_callback = listbox_callback;
	lb->userdata = userdata;

	gr_set_current_canvas(NULL);

	gr_set_curfont(MEDIUM3_FONT);

	lb->box_w = 0;
	for (i=0; i<nitems; i++ )	{
		int w, h, aw;
		gr_get_string_size( items[i], &w, &h, &aw );		
		if ( w > lb->box_w )
			lb->box_w = w+FSPACX(10);
	}
	lb->height = LINE_SPACING * LB_ITEMS_ON_SCREEN;

	{
		int w, h, aw;
		gr_get_string_size( title, &w, &h, &aw );		
		if ( w > lb->box_w )
			lb->box_w = w;
		lb->title_height = h+FSPACY(5);
	}

	lb->box_x = (grd_curcanv->cv_bitmap.bm_w-lb->box_w)/2;
	lb->box_y = (grd_curcanv->cv_bitmap.bm_h-(lb->height+lb->title_height))/2 + lb->title_height;
	if ( lb->box_y < lb->title_height )
		lb->box_y = lb->title_height;

	wind = window_create(&grd_curscreen->sc_canvas, lb->box_x-BORDERX, lb->box_y-lb->title_height-BORDERY, lb->box_w+2*BORDERX, lb->height+2*BORDERY, (int (*)(window *, d_event *, void *))listbox_handler, lb);
	if (!wind)
	{
		d_free(lb);
		newmenu_hide_cursor();
		
		return -1;
	}

	lb->done = 0;
	lb->citem = default_item;
	if ( lb->citem < 0 ) lb->citem = 0;
	if ( lb->citem >= nitems ) lb->citem = 0;

	lb->first_item = -1;

	lb->mouse_state = lb->omouse_state = 0;	//dblclick_flag = 0;
	newmenu_show_cursor();

	while(!lb->done)
		event_process();

	window_close(wind);
	
	rval = lb->citem;
	d_free(lb);
	return rval;
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

        return newmenu_do_fixedfont( title, nm_text, nchoices, nm_message_items, NULL, NULL, 0, NULL, -1, -1 );
}
//end this section addition - Victor Rachels
