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

#include "pstypes.h"
#include "dxxerror.h"
#include "gr.h"
#include "grdef.h"
#include "window.h"
#include "songs.h"
#include "key.h"
#include "mouse.h"
#include "palette.h"
#include "game.h"
#include "gamepal.h"
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
#include "args.h"
#include "gamepal.h"

#ifdef OGL
#include "ogl_init.h"
#endif


#define MAXDISPLAYABLEITEMS 14
#define MAXDISPLAYABLEITEMSTINY 21
#define MESSAGEBOX_TEXT_SIZE 2176  // How many characters in messagebox
#define MAX_TEXT_WIDTH FSPACX(120) // How many pixels wide a input box can be

struct newmenu
{
	window			*wind;
	int				x,y,w,h;
	short			swidth, sheight; float fntscalex, fntscaley; // with these we check if resolution or fonts have changed so menu structure can be recreated
	char			*title;
	char			*subtitle;
	int				nitems;
	newmenu_item	*items;
	int				(*subfunction)(newmenu *menu, d_event *event, void *userdata);
	int				citem;
	char			*filename;
	int				tiny_mode;
	int			tabs_flag;
	int			reorderitems;
	int				scroll_offset, last_scroll_check, max_displayable;
	int				all_text;		//set true if all text items
	int				is_scroll_box;   // Is this a scrolling box? Set to false at init
	int				max_on_menu;
	int				mouse_state, dblclick_flag;
	int				*rval;			// Pointer to return value (for polling newmenus)
	void			*userdata;		// For whatever - like with window system
};

grs_bitmap nm_background, nm_background1;
grs_bitmap *nm_background_sub = NULL;

newmenu *newmenu_do4( char * title, char * subtitle, int nitems, newmenu_item * item, int (*subfunction)(newmenu *menu, d_event *event, void *userdata), void *userdata, int citem, char * filename, int TinyMode, int TabsFlag );

void newmenu_free_background()	{
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

// Draws the custom menu background pcx, if available
void nm_draw_background1(char * filename)
{
	int pcx_error;

	if (filename != NULL)
	{
		if (nm_background1.bm_data == NULL)
		{
			gr_init_bitmap_data (&nm_background1);
			pcx_error = pcx_read_bitmap( filename, &nm_background1, BM_LINEAR, gr_palette );
			Assert(pcx_error == PCX_ERROR_NONE);
			(void)pcx_error;
		}
		gr_palette_load( gr_palette );
		show_fullscr(&nm_background1);
	}

	strcpy(last_palette_loaded,"");		//force palette load next time
}

#define MENU_BACKGROUND_BITMAP_HIRES (PHYSFSX_exists("scoresb.pcx",1)?"scoresb.pcx":"scores.pcx")
#define MENU_BACKGROUND_BITMAP_LORES (PHYSFSX_exists("scores.pcx",1)?"scores.pcx":"scoresb.pcx")
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
		(void)pcx_error;
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

	gr_settransblend(14, GR_BLEND_NORMAL);
	gr_setcolor( BM_XRGB(1,1,1) );
	for (w=5*BGScaleX;w>0;w--)
		gr_urect( x2-w, y1+w*(BGScaleY/BGScaleX), x2-w, y2-w*(BGScaleY/BGScaleX) );//right edge
	gr_setcolor( BM_XRGB(0,0,0) );
	for (h=5*BGScaleY;h>0;h--)
		gr_urect( x1+h*(BGScaleX/BGScaleY), y2-h, x2-h*(BGScaleX/BGScaleY), y2-h );//bottom edge
	gr_settransblend(GR_FADE_OFF, GR_BLEND_NORMAL);
}

// Draw a left justfied string
void nm_string( int w1,int x, int y, char * s, int tabs_flag)
{
	int w,h,aw,tx=0,t=0,i;
	char *p,*s1,*s2,measure[2];
	int XTabs[]={18,90,127,165,231,256};

	p=s1=NULL;
	s2 = d_strdup(s);

	for (i=0;i<6;i++) {
		XTabs[i]=FSPACX(XTabs[i]);
		XTabs[i]+=x;
	}

	measure[1]=0;

	if (!tabs_flag) {
		p = strchr( s2, '\t' );
		if (p && (w1>0) ) {
			*p = '\0';
			s1 = p+1;
		}
	}

	gr_get_string_size(s2, &w, &h, &aw  );

	if (w1 > 0)
		w = w1;

	if (tabs_flag) {
		for (i=0;s2[i];i++) {
			if (s2[i]=='\t' && tabs_flag) {
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

	if (!tabs_flag && p && (w1>0) ) {
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
	x -= FSPACX(3);

	if (w1 == 0) w1 = w;
	gr_string( x-w, y, s );
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

	if ( current && timer_query() & 0x8000 )
		gr_string( x+w1, y, CURSOR_STRING );
}

void draw_item( newmenu_item *item, int is_current, int tiny, int tabs_flag, int scroll_offset )
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
		gr_set_curfont(is_current?MEDIUM2_FONT:MEDIUM1_FONT);
        }

	switch( item->type )	{
		case NM_TYPE_TEXT:
		case NM_TYPE_MENU:
			nm_string( item->w, item->x, item->y-(((int)LINE_SPACING)*scroll_offset), item->text, tabs_flag );
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

			nm_string_slider( item->w, item->x, item->y-(((int)LINE_SPACING)*scroll_offset), item->saved_text );
		}
			break;
		case NM_TYPE_INPUT_MENU:
			if ( item->group==0 )
			{
				nm_string( item->w, item->x, item->y-(((int)LINE_SPACING)*scroll_offset), item->text, tabs_flag );
			} else {
				nm_string_inputbox( item->w, item->x, item->y-(((int)LINE_SPACING)*scroll_offset), item->text, is_current );
			}
			break;
		case NM_TYPE_INPUT:
			nm_string_inputbox( item->w, item->x, item->y-(((int)LINE_SPACING)*scroll_offset), item->text, is_current );
			break;
		case NM_TYPE_CHECK:
			nm_string( item->w, item->x, item->y-(((int)LINE_SPACING)*scroll_offset), item->text, tabs_flag );
			if (item->value)
				nm_rstring( item->right_offset,item->x, item->y-(((int)LINE_SPACING)*scroll_offset), CHECKED_CHECK_BOX );
			else
				nm_rstring( item->right_offset,item->x, item->y-(((int)LINE_SPACING)*scroll_offset), NORMAL_CHECK_BOX );
			break;
		case NM_TYPE_RADIO:
			nm_string( item->w, item->x, item->y-(((int)LINE_SPACING)*scroll_offset), item->text, tabs_flag );
			if (item->value)
				nm_rstring( item->right_offset, item->x, item->y-(((int)LINE_SPACING)*scroll_offset), CHECKED_RADIO_BOX );
			else
				nm_rstring( item->right_offset, item->x, item->y-(((int)LINE_SPACING)*scroll_offset), NORMAL_RADIO_BOX );
			break;
		case NM_TYPE_NUMBER:
		{
			char text[10];
			if (item->value < item->min_value) item->value=item->min_value;
			if (item->value > item->max_value) item->value=item->max_value;
			nm_string( item->w, item->x, item->y-(((int)LINE_SPACING)*scroll_offset), item->text, tabs_flag );
			sprintf( text, "%d", item->value );
			nm_rstring( item->right_offset,item->x, item->y-(((int)LINE_SPACING)*scroll_offset), text );
		}
			break;
	}
}

const char *Newmenu_allowed_chars=NULL;

//returns true if char is allowed
int char_allowed(char c)
{
	const char *p = Newmenu_allowed_chars;

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
	return newmenu_do2( title, subtitle, nitems, item, subfunction, userdata, 0, NULL );
}

newmenu *newmenu_dotiny( char * title, char * subtitle, int nitems, newmenu_item * item, int TabsFlag, int (*subfunction)(newmenu *menu, d_event *event, void *userdata), void *userdata )
{
        return newmenu_do4( title, subtitle, nitems, item, subfunction, userdata, 0, NULL, 1, TabsFlag );
}


int newmenu_do1( char * title, char * subtitle, int nitems, newmenu_item * item, int (*subfunction)(newmenu *menu, d_event *event, void *userdata), void *userdata, int citem )
{
	return newmenu_do2( title, subtitle, nitems, item, subfunction, userdata, citem, NULL );
}


int newmenu_do2( char * title, char * subtitle, int nitems, newmenu_item * item, int (*subfunction)(newmenu *menu, d_event *event, void *userdata), void *userdata, int citem, char * filename )
{
	newmenu *menu;
	window *wind;
	int rval = -1;

	menu = newmenu_do3( title, subtitle, nitems, item, subfunction, userdata, citem, filename );

	if (!menu)
		return -1;
	menu->rval = &rval;
	wind = menu->wind;	// avoid dereferencing a freed 'menu'

	// newmenu_do2 and simpler get their own event loop
	// This is so the caller doesn't have to provide a callback that responds to EVENT_NEWMENU_SELECTED
	while (window_exists(wind))
		event_process();

	return rval;
}

// Basically the same as do2 but sets reorderitems flag for weapon priority menu a bit redundant to get lose of a global variable but oh well...
int newmenu_doreorder( char * title, char * subtitle, int nitems, newmenu_item * item, int (*subfunction)(newmenu *menu, d_event *event, void *userdata), void *userdata )
{
	newmenu *menu;
	window *wind;
	int rval = -1;

	menu = newmenu_do3( title, subtitle, nitems, item, subfunction, userdata, 0, NULL );

	if (!menu)
		return -1;
	menu->reorderitems = 1;
	menu->rval = &rval;
	wind = menu->wind;	// avoid dereferencing a freed 'menu'

	// newmenu_do2 and simpler get their own event loop
	// This is so the caller doesn't have to provide a callback that responds to EVENT_NEWMENU_SELECTED
	while (window_exists(wind))
		event_process();

	return rval;
}

newmenu *newmenu_do3( char * title, char * subtitle, int nitems, newmenu_item * item, int (*subfunction)(newmenu *menu, d_event *event, void *userdata), void *userdata, int citem, char * filename )
{
	return newmenu_do4( title, subtitle, nitems, item, subfunction, userdata, citem, filename, 0, 0 );
}

newmenu *newmenu_do_fixedfont( char * title, char * subtitle, int nitems, newmenu_item * item, int (*subfunction)(newmenu *menu, d_event *event, void *userdata), void *userdata, int citem, char * filename){
	return newmenu_do4( title, subtitle, nitems, item, subfunction, userdata, citem, filename, 0, 0);
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

window *newmenu_get_window(newmenu *menu)
{
	return menu->wind;
}

void newmenu_scroll(newmenu *menu, int amount)
{
	int i = 0, first = 0, last = 0;

	if (amount == 0) // nothing to do for us
		return;

	if (menu->all_text)
	{
		menu->scroll_offset += amount;
		if (menu->scroll_offset < 0)
			menu->scroll_offset = 0;
		if (menu->max_on_menu+menu->scroll_offset > menu->nitems)
			menu->scroll_offset = menu->nitems-menu->max_on_menu;
		return;
	}

	for (i = 0; i < menu->nitems; i++) // find first "usable" item
	{
		if (menu->items[i].type != NM_TYPE_TEXT)
		{
			first = i;
			break;
		}
	}
	for (i = menu->nitems-1; i >= first; i--) // find last "usable" item
	{
		if (menu->items[i].type != NM_TYPE_TEXT)
		{
			last = i;
			break;
		}
	}

	if (first == last) // nothing to do for us
		return;

	if (menu->citem == last && amount == 1) // if citem == last item and we want to go down one step, go to first item
	{
		newmenu_scroll(menu, -menu->nitems);
		return;
	}
	if (menu->citem == first && amount == -1) // if citem == first item and we want to go up one step, go to last item
	{
		newmenu_scroll(menu, menu->nitems);
		return;
	}

	i = 0;
	if (amount > 0) // down the list
	{
		do // count down until we reached a non NM_TYPE_TEXT item and reached our amount
		{
			if (menu->citem == last) // stop if we reached the last item
				return;
			i++;
			menu->citem++;
			if (menu->is_scroll_box) // update scroll_offset as we go down the menu
			{
				menu->last_scroll_check=-1;
				if (menu->citem+4>=menu->max_on_menu+menu->scroll_offset && menu->scroll_offset < menu->nitems-menu->max_on_menu)
					menu->scroll_offset++;
			}
		} while (menu->items[menu->citem].type == NM_TYPE_TEXT || i < amount);
	}
	else if (amount < 0) // up the list
	{
		do // count up until we reached a non NM_TYPE_TEXT item and reached our amount
		{
			if (menu->citem == first)  // stop if we reached the first item
				return;
			i--;
			menu->citem--;
			if (menu->is_scroll_box) // update scroll_offset as we go up the menu
			{
				menu->last_scroll_check=-1;
				if (menu->citem-4<menu->scroll_offset && menu->scroll_offset > 0)
					menu->scroll_offset--;
			}
		} while (menu->items[menu->citem].type == NM_TYPE_TEXT || i > amount);
	}
}

int newmenu_mouse(window *wind, d_event *event, newmenu *menu, int button)
{
	int old_choice, i, mx=0, my=0, mz=0, x1 = 0, x2, y1, y2, changed = 0;
	grs_canvas *menu_canvas = window_get_canvas(wind), *save_canvas = grd_curcanv;

	switch (button)
	{
		case MBTN_LEFT:
		{
			gr_set_current_canvas(menu_canvas);

			old_choice = menu->citem;

			if ((event->type == EVENT_MOUSE_BUTTON_DOWN) && !menu->all_text)
			{
				mouse_get_pos(&mx, &my, &mz);
				for (i=menu->scroll_offset; i<menu->max_on_menu+menu->scroll_offset; i++ )	{
					x1 = grd_curcanv->cv_bitmap.bm_x + menu->items[i].x-FSPACX(13) /*- menu->items[i].right_offset - 6*/;
					x2 = x1 + menu->items[i].w+FSPACX(13);
					y1 = grd_curcanv->cv_bitmap.bm_y + menu->items[i].y - (((int)LINE_SPACING)*menu->scroll_offset);
					y2 = y1 + menu->items[i].h;
					if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2))) {
						if (i != menu->citem) {
							if(Hack_DblClick_MenuMode) menu->dblclick_flag = 0;
						}

						menu->citem = i;

						switch( menu->items[menu->citem].type )	{
							case NM_TYPE_CHECK:
								if ( menu->items[menu->citem].value )
									menu->items[menu->citem].value = 0;
								else
									menu->items[menu->citem].value = 1;

								if (menu->is_scroll_box)
									menu->last_scroll_check=-1;
								changed = 1;
								break;
							case NM_TYPE_RADIO:
								for (i=0; i<menu->nitems; i++ )	{
									if ((i!=menu->citem) && (menu->items[i].type==NM_TYPE_RADIO) && (menu->items[i].group==menu->items[menu->citem].group) && (menu->items[i].value) )	{
										menu->items[i].value = 0;
										changed = 1;
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

			if ( menu->mouse_state ) {
				mouse_get_pos(&mx, &my, &mz);

				// check possible scrollbar stuff first
				if (menu->is_scroll_box) {
					int arrow_width, arrow_height, aw, ScrollAllow=0;
					static fix64 ScrollTime=0;
					if (ScrollTime + F1_0/5 < timer_query())
					{
						ScrollTime = timer_query();
						ScrollAllow = 1;
					}

					if (menu->scroll_offset != 0) {
						gr_get_string_size(UP_ARROW_MARKER, &arrow_width, &arrow_height, &aw);
						x1 = grd_curcanv->cv_bitmap.bm_x+BORDERX-FSPACX(12);
						y1 = grd_curcanv->cv_bitmap.bm_y + menu->items[menu->scroll_offset].y-(((int)LINE_SPACING)*menu->scroll_offset);
						x2 = x1 + arrow_width;
						y2 = y1 + arrow_height;
						if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) && ScrollAllow) {
							newmenu_scroll(menu, -1);
						}
					}
					if (menu->scroll_offset+menu->max_displayable<menu->nitems) {
						gr_get_string_size(DOWN_ARROW_MARKER, &arrow_width, &arrow_height, &aw);
						x1 = grd_curcanv->cv_bitmap.bm_x+BORDERX-FSPACX(12);
						y1 = grd_curcanv->cv_bitmap.bm_y + menu->items[menu->scroll_offset+menu->max_displayable-1].y-(((int)LINE_SPACING)*menu->scroll_offset);
						x2 = x1 + arrow_width;
						y2 = y1 + arrow_height;
						if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) && ScrollAllow) {
							newmenu_scroll(menu, 1);
						}
					}
				}

				for (i=menu->scroll_offset; i<menu->max_on_menu+menu->scroll_offset; i++ )	{
					x1 = grd_curcanv->cv_bitmap.bm_x + menu->items[i].x-FSPACX(13);
					x2 = x1 + menu->items[i].w+FSPACX(13);
					y1 = grd_curcanv->cv_bitmap.bm_y + menu->items[i].y - (((int)LINE_SPACING)*menu->scroll_offset);
					y2 = y1 + menu->items[i].h;

					if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) && (menu->items[i].type != NM_TYPE_TEXT) ) {
						if (i != menu->citem) {
							if(Hack_DblClick_MenuMode) menu->dblclick_flag = 0;
						}

						menu->citem = i;

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
									changed = 1;
								} else if ( (mx < x2) && (mx > (x2 - sright_width)) && (menu->items[menu->citem].value != menu->items[menu->citem].max_value) ) {
									menu->items[menu->citem].value = menu->items[menu->citem].max_value;
									changed = 1;
								} else if ( (mx > (x1 + sleft_width)) && (mx < (x2 - sright_width)) ) {
									int num_values, value_width, new_value;

									num_values = menu->items[menu->citem].max_value - menu->items[menu->citem].min_value + 1;
									value_width = (slider_width - sleft_width - sright_width) / num_values;
									new_value = (mx - x1 - sleft_width) / value_width;
									if ( menu->items[menu->citem].value != new_value ) {
										menu->items[menu->citem].value = new_value;
										changed = 1;
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

			if ((event->type == EVENT_MOUSE_BUTTON_UP) && !menu->all_text && (menu->citem != -1) && (menu->items[menu->citem].type == NM_TYPE_MENU) )
			{
				mouse_get_pos(&mx, &my, &mz);
				for (i=menu->scroll_offset; i<menu->max_on_menu+menu->scroll_offset; i++ )	{
					x1 = grd_curcanv->cv_bitmap.bm_x + menu->items[i].x-FSPACX(13);
					x2 = x1 + menu->items[i].w+FSPACX(13);
					y1 = grd_curcanv->cv_bitmap.bm_y + menu->items[i].y - (((int)LINE_SPACING)*menu->scroll_offset);
					y2 = y1 + menu->items[i].h;
					if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2))) {
						if (Hack_DblClick_MenuMode) {
							if (menu->dblclick_flag)
							{
								// Tell callback, allow staying in menu
								event->type = EVENT_NEWMENU_SELECTED;
								if (menu->subfunction && (*menu->subfunction)(menu, event, menu->userdata))
									return 1;

								if (menu->rval)
									*menu->rval = menu->citem;
								window_close(menu->wind);
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

							if (menu->rval)
								*menu->rval = menu->citem;
							window_close(menu->wind);
							gr_set_current_canvas(save_canvas);
							return 1;
						}
					}
				}
			}

			if ((event->type == EVENT_MOUSE_BUTTON_UP) && (menu->citem>-1) && (menu->items[menu->citem].type==NM_TYPE_INPUT_MENU) && (menu->items[menu->citem].group==0))
			{
				menu->items[menu->citem].group = 1;
				if ( !d_strnicmp( menu->items[menu->citem].saved_text, TXT_EMPTY, strlen(TXT_EMPTY) ) )	{
					menu->items[menu->citem].text[0] = 0;
					menu->items[menu->citem].value = -1;
				} else {
					strip_end_whitespace(menu->items[menu->citem].text);
				}
			}

			gr_set_current_canvas(save_canvas);

			if (changed && menu->subfunction)
			{
				event->type = EVENT_NEWMENU_CHANGED;
				(*menu->subfunction)(menu, event, menu->userdata);
			}
			break;
		}
		case MBTN_RIGHT:
			if (menu->mouse_state)
			{
				if ( (menu->citem>-1) && (menu->items[menu->citem].type==NM_TYPE_INPUT_MENU) && (menu->items[menu->citem].group==1))	{
					menu->items[menu->citem].group=0;
					strcpy(menu->items[menu->citem].text, menu->items[menu->citem].saved_text );
					menu->items[menu->citem].value = -1;
				} else {
					window_close(menu->wind);
					return 1;
				}
			}
			break;
		case MBTN_Z_UP:
			if (menu->mouse_state)
				newmenu_scroll(menu, -1);
			break;
		case MBTN_Z_DOWN:
			if (menu->mouse_state)
				newmenu_scroll(menu, 1);
			break;
	}

	return 0;
}

int newmenu_key_command(window *wind, d_event *event, newmenu *menu)
{
	newmenu_item *item = &menu->items[menu->citem];
	int k = event_key_get(event);
	int old_choice, i;
	char *Temp,TempVal;
	int changed = 0;
	int rval = 1;

	if (keyd_pressed[KEY_NUMLOCK])
	{
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
	}

	old_choice = menu->citem;

	switch( k )	{
		case KEY_HOME:
		case KEY_PAD7:
			newmenu_scroll(menu, -menu->nitems);
			break;
		case KEY_END:
		case KEY_PAD1:
			newmenu_scroll(menu, menu->nitems);
			break;
		case KEY_TAB + KEY_SHIFTED:
		case KEY_UP:
		case KEY_PAGEUP:
		case KEY_PAD8:
			if (k == KEY_PAGEUP)
				newmenu_scroll(menu, -10);
			else
				newmenu_scroll(menu, -1);

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
		case KEY_PAGEDOWN:
		case KEY_PAD2:
			if (k == KEY_PAGEDOWN)
				newmenu_scroll(menu, 10);
			else
				newmenu_scroll(menu, 1);

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

				switch( item->type )	{
					case NM_TYPE_MENU:
					case NM_TYPE_INPUT:
					case NM_TYPE_INPUT_MENU:
						break;
					case NM_TYPE_CHECK:
						if ( item->value )
							item->value = 0;
						else
							item->value = 1;
						if (menu->is_scroll_box)
						{
							if (menu->citem==(menu->max_on_menu+menu->scroll_offset-1) || menu->citem==menu->scroll_offset)
							{
								menu->last_scroll_check=-1;
							}
						}

						changed = 1;
						break;
					case NM_TYPE_RADIO:
						for (i=0; i<menu->nitems; i++ )	{
							if ((i!=menu->citem) && (menu->items[i].type==NM_TYPE_RADIO) && (menu->items[i].group==item->group) && (menu->items[i].value) )	{
								menu->items[i].value = 0;
								changed = 1;
							}
						}
						item->value = 1;
						changed = 1;
						break;
				}
			}
			break;

		case KEY_SHIFTED+KEY_UP:
			if (menu->reorderitems && menu->citem!=0)
			{
				Temp=menu->items[menu->citem].text;
				TempVal=menu->items[menu->citem].value;
				menu->items[menu->citem].text=menu->items[menu->citem-1].text;
				menu->items[menu->citem].value=menu->items[menu->citem-1].value;
				menu->items[menu->citem-1].text=Temp;
				menu->items[menu->citem-1].value=TempVal;
				menu->citem--;
				changed = 1;
			}
			break;
		case KEY_SHIFTED+KEY_DOWN:
			if (menu->reorderitems && menu->citem!=(menu->nitems-1))
			{
				Temp=menu->items[menu->citem].text;
				TempVal=menu->items[menu->citem].value;
				menu->items[menu->citem].text=menu->items[menu->citem+1].text;
				menu->items[menu->citem].value=menu->items[menu->citem+1].value;
				menu->items[menu->citem+1].text=Temp;
				menu->items[menu->citem+1].value=TempVal;
				menu->citem++;
				changed = 1;
			}
			break;
		case KEY_ENTER:
		case KEY_PADENTER:
			if ( (menu->citem>-1) && (item->type==NM_TYPE_INPUT_MENU) && (item->group==0))	{
				item->group = 1;
				if ( !d_strnicmp( item->saved_text, TXT_EMPTY, strlen(TXT_EMPTY) ) )	{
					item->text[0] = 0;
					item->value = -1;
				} else {
					strip_end_whitespace(item->text);
				}
			} else
			{
				if (item->type==NM_TYPE_INPUT_MENU)
					item->group = 0;	// go out of editing mode

				// Tell callback, allow staying in menu
				event->type = EVENT_NEWMENU_SELECTED;
				if (menu->subfunction && (*menu->subfunction)(menu, event, menu->userdata))
					return 1;

				if (menu->rval)
					*menu->rval = menu->citem;
				window_close(menu->wind);
				return 1;
			}
			break;

		case KEY_ESC:
			if ( (menu->citem>-1) && (item->type==NM_TYPE_INPUT_MENU) && (item->group==1))	{
				item->group=0;
				strcpy(item->text, item->saved_text );
				item->value = -1;
			} else {
				window_close(menu->wind);
				return 1;
			}
			break;

#ifndef NDEBUG
		case KEY_BACKSP:
			if ( (menu->citem>-1) && (item->type!=NM_TYPE_INPUT)&&(item->type!=NM_TYPE_INPUT_MENU))
				Int3();
			break;
#endif

		default:
			rval = 0;
			break;
	}

	if ( menu->citem > -1 )	{
		int ascii;

		// Alerting callback of every keypress for NM_TYPE_INPUT. Alternatively, just respond to EVENT_NEWMENU_SELECTED
		if ( ((item->type==NM_TYPE_INPUT)||((item->type==NM_TYPE_INPUT_MENU)&&(item->group==1)) )&& (old_choice==menu->citem) )	{
			if ( k==KEY_LEFT || k==KEY_BACKSP || k==KEY_PAD4 )	{
				if (item->value==-1) item->value = strlen(item->text);
				if (item->value > 0)
					item->value--;
				item->text[item->value] = 0;

				if (item->type==NM_TYPE_INPUT)
					changed = 1;
				rval = 1;
			} else {
				ascii = key_ascii();
				if ((ascii < 255 ) && (item->value < item->text_len ))
				{
					int allowed;

					if (item->value==-1) {
						item->value = 0;
					}

					allowed = char_allowed(ascii);

					if (!allowed && ascii==' ' && char_allowed('_')) {
						ascii = '_';
						allowed=1;
					}

					if (allowed) {
						item->text[item->value++] = ascii;
						item->text[item->value] = 0;

						if (item->type==NM_TYPE_INPUT)
							changed = 1;
					}
				}
			}
		}
		else if ((item->type!=NM_TYPE_INPUT) && (item->type!=NM_TYPE_INPUT_MENU) )
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

					while (menu->citem+4>=menu->max_on_menu+menu->scroll_offset && menu->scroll_offset < menu->nitems-menu->max_on_menu)
						menu->scroll_offset++;
					while (menu->citem-4<menu->scroll_offset && menu->scroll_offset > 0)
						menu->scroll_offset--;

				} while (choice1 != menu->citem );
			}
		}

		if ( (item->type==NM_TYPE_NUMBER) || (item->type==NM_TYPE_SLIDER))
		{
			switch( k ) {
				case KEY_LEFT:
				case KEY_PAD4:
					item->value -= 1;
					changed = 1;
					rval = 1;
					break;
				case KEY_RIGHT:
				case KEY_PAD6:
					item->value++;
					changed = 1;
					rval = 1;
					break;
				case KEY_SPACEBAR:
					item->value += 10;
					changed = 1;
					rval = 1;
					break;
				case KEY_BACKSP:
					item->value -= 10;
					changed = 1;
					rval = 1;
					break;
			}

			if (item->value < item->min_value) item->value=item->min_value;
			if (item->value > item->max_value) item->value=item->max_value;
		}

	}

	if (changed && menu->subfunction)
	{
		event->type = EVENT_NEWMENU_CHANGED;
		(*menu->subfunction)(menu, event, menu->userdata);
	}

	return rval;
}

void newmenu_create_structure( newmenu *menu )
{
	int i,j,aw, tw, th, twidth,fm,right_offset;
	int nmenus, nothers;
	grs_font *save_font;
	grs_canvas *save_canvas;
	int string_width, string_height, average_width;

	save_canvas = grd_curcanv;

	gr_set_current_canvas(NULL);

	save_font = grd_curcanv->cv_font;

	tw = th = 0;

	if ( menu->title )	{
		gr_set_curfont(HUGE_FONT);
		gr_get_string_size(menu->title,&string_width,&string_height,&average_width );
		tw = string_width;
		th = string_height;
	}
	if ( menu->subtitle )	{
		gr_set_curfont(MEDIUM3_FONT);
		gr_get_string_size(menu->subtitle,&string_width,&string_height,&average_width );
		if (string_width > tw )
			tw = string_width;
		th += string_height;
	}

	th += FSPACY(5);		//put some space between titles & body

	gr_set_curfont(menu->tiny_mode?GAME_FONT:MEDIUM1_FONT);

	menu->w = aw = 0;
	menu->h = th;
	nmenus = nothers = 0;

	// Find menu height & width (store in w,h)
	for (i=0; i<menu->nitems; i++ )	{
		menu->items[i].y = menu->h;
		gr_get_string_size(menu->items[i].text,&string_width,&string_height,&average_width );
		menu->items[i].right_offset = 0;

		menu->items[i].saved_text[0] = '\0';

		if ( menu->items[i].type == NM_TYPE_SLIDER )	{
			int index,w1,h1,aw1;
			nothers++;
			index = sprintf( menu->items[i].saved_text, "%s", SLIDER_LEFT );
			for (j=0; j<(menu->items[i].max_value-menu->items[i].min_value+1); j++ )	{
				index+= sprintf( menu->items[i].saved_text + index, "%s", SLIDER_MIDDLE );
			}
			sprintf( menu->items[i].saved_text + index, "%s", SLIDER_RIGHT );
			gr_get_string_size(menu->items[i].saved_text,&w1,&h1,&aw1 );
			string_width += w1 + aw;
		}

		if ( menu->items[i].type == NM_TYPE_MENU )	{
			nmenus++;
		}

		if ( menu->items[i].type == NM_TYPE_CHECK )	{
			int w1,h1,aw1;
			nothers++;
			gr_get_string_size(NORMAL_CHECK_BOX, &w1, &h1, &aw1  );
			menu->items[i].right_offset = w1;
			gr_get_string_size(CHECKED_CHECK_BOX, &w1, &h1, &aw1  );
			if (w1 > menu->items[i].right_offset)
				menu->items[i].right_offset = w1;
		}

		if (menu->items[i].type == NM_TYPE_RADIO ) {
			int w1,h1,aw1;
			nothers++;
			gr_get_string_size(NORMAL_RADIO_BOX, &w1, &h1, &aw1  );
			menu->items[i].right_offset = w1;
			gr_get_string_size(CHECKED_RADIO_BOX, &w1, &h1, &aw1  );
			if (w1 > menu->items[i].right_offset)
				menu->items[i].right_offset = w1;
		}

		if  (menu->items[i].type==NM_TYPE_NUMBER )	{
			int w1,h1,aw1;
			char test_text[20];
			nothers++;
			sprintf( test_text, "%d", menu->items[i].max_value );
			gr_get_string_size( test_text, &w1, &h1, &aw1 );
			menu->items[i].right_offset = w1;
			sprintf( test_text, "%d", menu->items[i].min_value );
			gr_get_string_size( test_text, &w1, &h1, &aw1 );
			if ( w1 > menu->items[i].right_offset)
				menu->items[i].right_offset = w1;
		}

		if ((menu->items[i].type == NM_TYPE_INPUT) || (menu->items[i].type == NM_TYPE_INPUT_MENU))
		{
			Assert( strlen(menu->items[i].text) < NM_MAX_TEXT_LEN );
			strcpy(menu->items[i].saved_text, menu->items[i].text );

			string_width = menu->items[i].text_len*FSPACX(8)+menu->items[i].text_len;
			if ( menu->items[i].type == NM_TYPE_INPUT && string_width > MAX_TEXT_WIDTH )
				string_width = MAX_TEXT_WIDTH;

			menu->items[i].value = -1;
			menu->items[i].group = 0;
			if (menu->items[i].type == NM_TYPE_INPUT_MENU)
				nmenus++;
			else
				nothers++;
		}

		menu->items[i].w = string_width;
		menu->items[i].h = string_height;

		if ( string_width > menu->w )
			menu->w = string_width;		// Save maximum width
		if ( average_width > aw )
			aw = average_width;
		menu->h += string_height+FSPACY(1);		// Find the height of all strings
	}

	if (i > menu->max_on_menu)
	{
		menu->is_scroll_box=1;
		menu->h = th+(LINE_SPACING*menu->max_on_menu);
		menu->max_displayable=menu->max_on_menu;

		// if our last citem was > menu->max_on_menu, make sure we re-scroll when we call this menu again
		if (menu->citem > menu->max_on_menu-4)
		{
			menu->scroll_offset = menu->citem - (menu->max_on_menu-4);
			if (menu->scroll_offset + menu->max_on_menu > menu->nitems)
				menu->scroll_offset = menu->nitems - menu->max_on_menu;
		}
	}
	else
	{
		menu->is_scroll_box=0;
		menu->max_on_menu=i;
	}

	right_offset=0;

	for (i=0; i<menu->nitems; i++ )	{
		menu->items[i].w = menu->w;
		if (menu->items[i].right_offset > right_offset )
			right_offset = menu->items[i].right_offset;
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

	// Update all item's x & y values.
	for (i=0; i<menu->nitems; i++ )	{
		menu->items[i].x = BORDERX + twidth + right_offset;
		menu->items[i].y += BORDERY;
		if ( menu->items[i].type==NM_TYPE_RADIO )	{
			fm = -1;	// find first marked one
			for ( j=0; j<menu->nitems; j++ )	{
				if ( menu->items[j].type==NM_TYPE_RADIO && menu->items[j].group==menu->items[i].group )	{
					if (fm==-1 && menu->items[j].value)
						fm = j;
					menu->items[j].value = 0;
				}
			}
			if ( fm>=0 )
				menu->items[fm].value=1;
			else
				menu->items[i].value=1;
		}
	}

	if (menu->citem != -1)
	{
		if (menu->citem < 0 ) menu->citem = 0;
		if (menu->citem > menu->nitems-1 ) menu->citem = menu->nitems-1;

#ifdef NEWMENU_MOUSE
		menu->dblclick_flag = 1;
#endif
		i = 0;
		while ( menu->items[menu->citem].type==NM_TYPE_TEXT )	{
			menu->citem++;
			i++;
			if (menu->citem >= menu->nitems ) {
				menu->citem=0;
			}
			if (i > menu->nitems ) {
				menu->citem=0;
				menu->all_text=1;
				break;
			}
		}
	}

	menu->mouse_state = 0;
	menu->swidth = SWIDTH;
	menu->sheight = SHEIGHT;
	menu->fntscalex = FNTScaleX;
	menu->fntscaley = FNTScaleY;
	gr_set_curfont(save_font);
	gr_set_current_canvas(save_canvas);
}

int newmenu_draw(window *wind, newmenu *menu)
{
	grs_canvas *menu_canvas = window_get_canvas(wind), *save_canvas = grd_curcanv;
	int th = 0, ty, sx, sy;
	int i;
	int string_width, string_height, average_width;

	if (menu->swidth != SWIDTH || menu->sheight != SHEIGHT || menu->fntscalex != FNTScaleX || menu->fntscalex != FNTScaleY)
	{
		newmenu_create_structure ( menu );
		if (menu_canvas)
		{
			gr_init_sub_canvas(menu_canvas, &grd_curscreen->sc_canvas, menu->x, menu->y, menu->w, menu->h);
		}
	}

	gr_set_current_canvas( NULL );
	nm_draw_background1(menu->filename);
	if (menu->filename == NULL)
		nm_draw_background(menu->x-(menu->is_scroll_box?FSPACX(5):0),menu->y,menu->x+menu->w,menu->y+menu->h);

	gr_set_current_canvas( menu_canvas );

	ty = BORDERY;

	if ( menu->title )	{
		gr_set_curfont(HUGE_FONT);
		gr_set_fontcolor( BM_XRGB(31,31,31), -1 );
		gr_get_string_size(menu->title,&string_width,&string_height,&average_width );
		th = string_height;
		gr_string( 0x8000, ty, menu->title );
	}

	if ( menu->subtitle )	{
		gr_set_curfont(MEDIUM3_FONT);
		gr_set_fontcolor( BM_XRGB(21,21,21), -1 );
		gr_get_string_size(menu->subtitle,&string_width,&string_height,&average_width );
		gr_string( 0x8000, ty+th, menu->subtitle );
	}

	gr_set_curfont(menu->tiny_mode?GAME_FONT:MEDIUM1_FONT);

	// Redraw everything...
	for (i=menu->scroll_offset; i<menu->max_displayable+menu->scroll_offset; i++ )
	{
		draw_item( &menu->items[i], (i==menu->citem && !menu->all_text),menu->tiny_mode, menu->tabs_flag, menu->scroll_offset );

	}

	if (menu->is_scroll_box)
	{
		menu->last_scroll_check=menu->scroll_offset;
		gr_set_curfont(menu->tiny_mode?GAME_FONT:MEDIUM2_FONT);

		sy=menu->items[menu->scroll_offset].y-(((int)LINE_SPACING)*menu->scroll_offset);
		sx=BORDERX-FSPACX(12);

		if (menu->scroll_offset!=0)
			gr_printf( sx, sy, UP_ARROW_MARKER );
		else
			gr_printf( sx, sy, "  " );

		sy=menu->items[menu->scroll_offset+menu->max_displayable-1].y-(((int)LINE_SPACING)*menu->scroll_offset);
		sx=BORDERX-FSPACX(12);

		if (menu->scroll_offset+menu->max_displayable<menu->nitems)
			gr_printf( sx, sy, DOWN_ARROW_MARKER );
		else
			gr_printf( sx, sy, "  " );

	}

	{
		d_event event;

		event.type = EVENT_NEWMENU_DRAW;
		if (menu->subfunction)
			(*menu->subfunction)(menu, &event, menu->userdata);
	}

	gr_set_current_canvas(save_canvas);

	return 1;
}

int newmenu_handler(window *wind, d_event *event, newmenu *menu)
{
	if (event->type == EVENT_WINDOW_CLOSED)
		return 0;

	if (menu->subfunction)
	{
		int rval = (*menu->subfunction)(menu, event, menu->userdata);

		if (!window_exists(wind))
			return 1;	// some subfunction closed the window: bail!

		if (rval)
		{
			if (rval < -1)
			{
				if (menu->rval)
					*menu->rval = rval;
				window_close(wind);
			}

			return 1;		// event handled
		}
	}

	switch (event->type)
	{
		case EVENT_WINDOW_ACTIVATED:
			game_flush_inputs();
			event_toggle_focus(0);
			key_toggle_repeat(1);
			break;

		case EVENT_WINDOW_DEACTIVATED:
			//event_toggle_focus(1);	// No cursor recentering
			key_toggle_repeat(1);
			menu->mouse_state = 0;
			break;

		case EVENT_MOUSE_BUTTON_DOWN:
		case EVENT_MOUSE_BUTTON_UP:
		{
			int button = event_mouse_get_button(event);
			menu->mouse_state = event->type == EVENT_MOUSE_BUTTON_DOWN;
			return newmenu_mouse(wind, event, menu, button);
		}

		case EVENT_KEY_COMMAND:
			return newmenu_key_command(wind, event, menu);
			break;

		case EVENT_IDLE:
			timer_delay2(50);

			return newmenu_mouse(wind, event, menu, -1);
			break;

		case EVENT_WINDOW_DRAW:
			return newmenu_draw(wind, menu);
			break;

		case EVENT_WINDOW_CLOSE:
			d_free(menu);
			break;

		default:
			break;
	}

	return 0;
}

newmenu *newmenu_do4( char * title, char * subtitle, int nitems, newmenu_item * item, int (*subfunction)(newmenu *menu, d_event *event, void *userdata), void *userdata, int citem, char * filename, int TinyMode, int TabsFlag )
{
	window *wind = NULL;
	newmenu *menu;

	CALLOC(menu, newmenu, 1);

	if (!menu)
		return NULL;

	menu->citem = citem;
	menu->scroll_offset = 0;
	menu->last_scroll_check = -1;
	menu->all_text = 0;
	menu->is_scroll_box = 0;
	menu->max_on_menu = TinyMode?MAXDISPLAYABLEITEMSTINY:MAXDISPLAYABLEITEMS;
	menu->dblclick_flag = 0;
	menu->title = title;
	menu->subtitle = subtitle;
	menu->nitems = nitems;
	menu->subfunction = subfunction;
	menu->items = item;
	menu->filename = filename;
	menu->tiny_mode = TinyMode;
	menu->tabs_flag = TabsFlag;
	menu->reorderitems = 0; // will be set if needed
	menu->rval = NULL;		// Default to not returning a value - respond to EVENT_NEWMENU_SELECTED instead
	menu->userdata = userdata;

	newmenu_free_background();

	if (nitems < 1 )
	{
		d_free(menu);
		return NULL;
	}

	menu->max_displayable=nitems;

	//set_screen_mode(SCREEN_MENU);	//hafta set the screen mode here or fonts might get changed/freed up if screen res changes

	newmenu_create_structure(menu);

	// Create the basic window
	if (menu)
		wind = window_create(&grd_curscreen->sc_canvas, menu->x, menu->y, menu->w, menu->h, (int (*)(window *, d_event *, void *))newmenu_handler, menu);
	if (!wind)
	{
		d_free(menu);

		return NULL;
	}
	menu->wind = wind;

	return menu;
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
	window *wind;
	char *title;
	int nitems;
	char **item;
	int allow_abort_flag;
	int (*listbox_callback)(listbox *lb, d_event *event, void *userdata);
	int citem, first_item;
	int marquee_maxchars, marquee_charpos, marquee_scrollback;
	fix64 marquee_lasttime; // to scroll text if string does not fit in box
	int box_w, height, box_x, box_y, title_height;
	short swidth, sheight; float fntscalex, fntscaley; // with these we check if resolution or fonts have changed so listbox structure can be recreated
	int mouse_state;
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

window *listbox_get_window(listbox *lb)
{
	return lb->wind;
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

void update_scroll_position(listbox *lb)
{
	if (lb->citem<0)
		lb->citem = 0;

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
}

int listbox_mouse(window *wind, d_event *event, listbox *lb, int button)
{
	int i, mx, my, mz, x1, x2, y1, y2;

	switch (button)
	{
		case MBTN_LEFT:
		{
			if (lb->mouse_state)
			{
				int w, h, aw;

				mouse_get_pos(&mx, &my, &mz);
				for (i=lb->first_item; i<lb->first_item+LB_ITEMS_ON_SCREEN; i++ )	{
					if (i >= lb->nitems)
						break;
					gr_get_string_size(lb->item[i], &w, &h, &aw  );
					x1 = lb->box_x;
					x2 = lb->box_x + lb->box_w;
					y1 = (i-lb->first_item)*LINE_SPACING+lb->box_y;
					y2 = y1+h;
					if ( ((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) ) {
						lb->citem = i;
						return 1;
					}
				}
			}
			else if (event->type == EVENT_MOUSE_BUTTON_UP)
			{
				int w, h, aw;

				if (lb->citem < 0)
					return 0;

				mouse_get_pos(&mx, &my, &mz);
				gr_get_string_size(lb->item[lb->citem], &w, &h, &aw  );
				x1 = lb->box_x;
				x2 = lb->box_x + lb->box_w;
				y1 = (lb->citem-lb->first_item)*LINE_SPACING+lb->box_y;
				y2 = y1+h;
				if ( ((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) )
				{
					// Tell callback, allow staying in menu
					event->type = EVENT_NEWMENU_SELECTED;
					if (lb->listbox_callback && (*lb->listbox_callback)(lb, event, lb->userdata))
						return 1;

					window_close(wind);
					return 1;
				}
			}
			break;
		}
		case MBTN_RIGHT:
		{
			if (lb->allow_abort_flag && lb->mouse_state) {
				lb->citem = -1;
				window_close(wind);
				return 1;
			}
			break;
		}
		case MBTN_Z_UP:
		{
			if (lb->mouse_state)
			{
				lb->citem--;
				update_scroll_position(lb);
			}
			break;
		}
		case MBTN_Z_DOWN:
		{
			if (lb->mouse_state)
			{
				lb->citem++;
				update_scroll_position(lb);
			}
			break;
		}
		default:
			break;
	}

	return 0;
}

int listbox_key_command(window *wind, d_event *event, listbox *lb)
{
	int key = event_key_get(event);
	int rval = 1;

	switch(key)	{
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
				window_close(wind);
				return 1;
			}
			break;
		case KEY_ENTER:
		case KEY_PADENTER:
			// Tell callback, allow staying in menu
			event->type = EVENT_NEWMENU_SELECTED;
			if (lb->listbox_callback && (*lb->listbox_callback)(lb, event, lb->userdata))
				return 1;

			window_close(wind);
			return 1;
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
			rval = 0;
		}
	}

	update_scroll_position(lb);

	return rval;
}

void listbox_create_structure( listbox *lb)
{
	int i = 0;

	gr_set_current_canvas(NULL);

	gr_set_curfont(MEDIUM3_FONT);

	lb->box_w = 0;
	for (i=0; i<lb->nitems; i++ )	{
		int w, h, aw;
		gr_get_string_size( lb->item[i], &w, &h, &aw );
		if ( w > lb->box_w )
			lb->box_w = w+FSPACX(10);
	}
	lb->height = LINE_SPACING * LB_ITEMS_ON_SCREEN;

	{
		int w, h, aw;
		gr_get_string_size( lb->title, &w, &h, &aw );
		if ( w > lb->box_w )
			lb->box_w = w;
		lb->title_height = h+FSPACY(5);
	}

	lb->marquee_maxchars = lb->marquee_charpos = lb->marquee_scrollback = lb->marquee_lasttime = 0;
	// The box is bigger than we can fit on the screen since at least one string is too long. Check how many chars we can fit on the screen (at least only - MEDIUM*_FONT is variable font!) so we can make a marquee-like effect.
	if (lb->box_w + (BORDERX*2) > SWIDTH)
	{
		int w = 0, h = 0, aw = 0;

		lb->box_w = SWIDTH - (BORDERX*2);
		gr_get_string_size("O", &w, &h, &aw);
		lb->marquee_maxchars = lb->box_w/w;
		lb->marquee_lasttime = timer_query();
	}

	lb->box_x = (grd_curcanv->cv_bitmap.bm_w-lb->box_w)/2;
	lb->box_y = (grd_curcanv->cv_bitmap.bm_h-(lb->height+lb->title_height))/2 + lb->title_height;
	if ( lb->box_y < lb->title_height )
		lb->box_y = lb->title_height;

	if ( lb->citem < 0 ) lb->citem = 0;
	if ( lb->citem >= lb->nitems ) lb->citem = 0;

	lb->first_item = 0;
	update_scroll_position(lb);

	lb->mouse_state = 0;	//dblclick_flag = 0;
	lb->swidth = SWIDTH;
	lb->sheight = SHEIGHT;
	lb->fntscalex = FNTScaleX;
	lb->fntscaley = FNTScaleY;
}

int listbox_draw(window *wind, listbox *lb)
{
	int i;

	if (lb->swidth != SWIDTH || lb->sheight != SHEIGHT || lb->fntscalex != FNTScaleX || lb->fntscalex != FNTScaleY)
		listbox_create_structure ( lb );

	gr_set_current_canvas(NULL);
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
			gr_set_curfont(( i == lb->citem )?MEDIUM2_FONT:MEDIUM1_FONT);
			gr_setcolor( BM_XRGB(5,5,5));
			gr_rect( lb->box_x + lb->box_w - FSPACX(1), y-FSPACY(1), lb->box_x + lb->box_w, y + LINE_SPACING);
			gr_setcolor( BM_XRGB(2,2,2));
			gr_rect( lb->box_x - FSPACX(1), y - FSPACY(1), lb->box_x, y + LINE_SPACING );
			gr_setcolor( BM_XRGB(0,0,0));
			gr_rect( lb->box_x, y - FSPACY(1), lb->box_x + lb->box_w - FSPACX(1), y + LINE_SPACING);

			if (lb->marquee_maxchars && strlen(lb->item[i]) > lb->marquee_maxchars)
			{
				char *shrtstr = d_malloc(lb->marquee_maxchars+1);
				static int prev_citem = -1;
				
				if (prev_citem != lb->citem)
				{
					lb->marquee_charpos = lb->marquee_scrollback = 0;
					lb->marquee_lasttime = timer_query();
					prev_citem = lb->citem;
				}

				memset(shrtstr, '\0', lb->marquee_maxchars+1);
				
				if (i == lb->citem)
				{
					if (lb->marquee_lasttime + (F1_0/3) < timer_query())
					{
						lb->marquee_charpos = lb->marquee_charpos+(lb->marquee_scrollback?-1:+1);
						lb->marquee_lasttime = timer_query();
					}
					if (lb->marquee_charpos < 0) // reached beginning of string -> scroll forward
					{
						lb->marquee_charpos = 0;
						lb->marquee_scrollback = 0;
					}
					if (lb->marquee_charpos + lb->marquee_maxchars - 1 > strlen(lb->item[i])) // reached end of string -> scroll backward
					{
						lb->marquee_charpos = strlen(lb->item[i]) - lb->marquee_maxchars + 1;
						lb->marquee_scrollback = 1;
					}
					snprintf(shrtstr, lb->marquee_maxchars, "%s", lb->item[i]+lb->marquee_charpos);
				}
				else
				{
					snprintf(shrtstr, lb->marquee_maxchars, "%s", lb->item[i]);
				}
				gr_string( lb->box_x+FSPACX(5), y, shrtstr );
				d_free(shrtstr);
			}
			else
			{
				gr_string( lb->box_x+FSPACX(5), y, lb->item[i]  );
			}
		}
	}

	{
		d_event event;

		event.type = EVENT_NEWMENU_DRAW;
		if ( lb->listbox_callback )
			(*lb->listbox_callback)(lb, &event, lb->userdata);
	}

	return 1;
}

int listbox_handler(window *wind, d_event *event, listbox *lb)
{
	if (event->type == EVENT_WINDOW_CLOSED)
		return 0;

	if (lb->listbox_callback)
	{
		int rval = (*lb->listbox_callback)(lb, event, lb->userdata);
		if (rval)
			return 1;		// event handled
	}

	switch (event->type)
	{
		case EVENT_WINDOW_ACTIVATED:
			game_flush_inputs();
			event_toggle_focus(0);
			key_toggle_repeat(1);
			break;

		case EVENT_WINDOW_DEACTIVATED:
			//event_toggle_focus(1);	// No cursor recentering
			key_toggle_repeat(0);
			break;

		case EVENT_MOUSE_BUTTON_DOWN:
		case EVENT_MOUSE_BUTTON_UP:
		{
			int button = event_mouse_get_button(event);
			lb->mouse_state = event->type == EVENT_MOUSE_BUTTON_DOWN;
			return listbox_mouse(wind, event, lb, button);
		}

		case EVENT_KEY_COMMAND:
			return listbox_key_command(wind, event, lb);
			break;

		case EVENT_IDLE:
			timer_delay2(50);

			return listbox_mouse(wind, event, lb, -1);
			break;

		case EVENT_WINDOW_DRAW:
			return listbox_draw(wind, lb);
			break;

		case EVENT_WINDOW_CLOSE:
			d_free(lb);
			break;

		default:
			break;
	}

	return 0;
}

listbox *newmenu_listbox( char * title, int nitems, char * items[], int allow_abort_flag, int (*listbox_callback)(listbox *lb, d_event *event, void *userdata), void *userdata )
{
	return newmenu_listbox1( title, nitems, items, allow_abort_flag, 0, listbox_callback, userdata );
}

listbox *newmenu_listbox1( char * title, int nitems, char * items[], int allow_abort_flag, int default_item, int (*listbox_callback)(listbox *lb, d_event *event, void *userdata), void *userdata )
{
	listbox *lb;
	window *wind;

	CALLOC(lb, listbox, 1);

	if (!lb)
		return NULL;

	newmenu_free_background();

	lb->title = title;
	lb->nitems = nitems;
	lb->item = items;
	lb->citem = default_item;
	lb->allow_abort_flag = allow_abort_flag;
	lb->listbox_callback = listbox_callback;
	lb->userdata = userdata;

	set_screen_mode(SCREEN_MENU);	//hafta set the screen mode here or fonts might get changed/freed up if screen res changes
	
	listbox_create_structure(lb);

	wind = window_create(&grd_curscreen->sc_canvas, lb->box_x-BORDERX, lb->box_y-lb->title_height-BORDERY, lb->box_w+2*BORDERX, lb->height+2*BORDERY, (int (*)(window *, d_event *, void *))listbox_handler, lb);
	if (!wind)
	{
		d_free(lb);

		return NULL;
	}
	lb->wind = wind;

	return lb;
}

//added on 10/14/98 by Victor Rachels to attempt a fixedwidth font messagebox
newmenu *nm_messagebox_fixedfont( char *title, int nchoices, ... )
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

        return newmenu_do_fixedfont( title, nm_text, nchoices, nm_message_items, NULL, NULL, 0, NULL );
}
//end this section addition - Victor Rachels
