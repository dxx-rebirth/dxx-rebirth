/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license below.
 * See COPYING.txt for license details.

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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "physfsx.h"
#include "u_mem.h"
#include "maths.h"
#include "pstypes.h"
#include "gr.h"
#include "strutil.h"
#include "event.h"
#include "window.h"
#include "ui.h"
#include "mouse.h"
#include "key.h"
#include "func.h"
#include "dxxerror.h"

#include "compiler-exchange.h"
#include "compiler-range_for.h"
#include "partial_range.h"

#define MAXMENUS 30
#define MAXITEMS 32

namespace {

struct ITEM {
	short 			x, y, w, h;
	RAIIdmem<char[]> Text;
	RAIIdmem<char[]> InactiveText;
	short 			Hotkey;
	int   			(*user_function)(void);
};

struct MENU : embed_window_pointer_t {
	short 			x, y, w, h;
	short				ShowBar;
	short				CurrentItem;
	uint16_t			NumItems;
	short				Displayed;
	short				Active;
	array<ITEM, MAXITEMS> Item;
};

}

static array<MENU, MAXMENUS> Menu;

static unsigned num_menus;
static int state;

#define CMENU (Menu[0].CurrentItem+1)

static window_event_result menubar_handler(window *wind,const d_event &event, MENU *menu);
static window_event_result menu_handler(window *wind,const d_event &event, MENU *menu);

//------------------------- Show a menu item -------------------

static void item_show( MENU * menu, int n )
{
	ITEM * item = &menu->Item[n];
	
	gr_set_current_canvas(NULL);
	// If this is a seperator, then draw it.
	if ( item->Text[0] == '-'  )
	{
		gr_setcolor( CBLACK );
		gr_urect( item->x, item->y+item->h/2, item->x+item->w-1, item->y+item->h/2 );
		return;
	}	

	if ( menu->CurrentItem==n && menu->ShowBar )
	{
		if ( menu != &Menu[0] )
		{
			gr_setcolor( CBLACK );
			gr_urect( item->x+1, item->y+1, item->x+menu->w-2, item->y+item->h-2 );
		}
	 	gr_set_fontcolor( CWHITE, CBLACK );
	}else {
		if ( menu != &Menu[0] )
		{
			gr_setcolor( CGREY );
			gr_urect( item->x+1, item->y+1, item->x+menu->w-2, item->y+item->h-2 );
		}
		gr_set_fontcolor( CBLACK, CGREY );
	}

	if ( menu != &Menu[0] )
	{
		if ( menu->Active)
			gr_ustring(item->x+1, item->y+1, item->Text.get());
		else
			gr_ustring(item->x+1, item->y+1, item->InactiveText.get());
	} else {
		if ( menu->Active)
			gr_ustring(item->x, item->y, item->Text.get());
		else
			gr_ustring(item->x, item->y, item->InactiveText.get());
	}
}

static void menu_draw(MENU *menu)
{
	int i;
	
	gr_set_current_canvas(NULL);

	// Draw the menu background
	gr_setcolor( CGREY );
	gr_urect( menu->x, menu->y, menu->x + menu->w - 1, menu->y + menu->h - 1 );
	if ( menu != &Menu[0] )
	{
		gr_setcolor( CBLACK );
		gr_ubox( menu->x, menu->y, menu->x + menu->w - 1, menu->y + menu->h - 1 );
	}
	
	// Draw the items
	
	for (i=0; i< menu->NumItems; i++ )
		item_show( menu, i );
}

//---------------------------- Show a menu ---------------------

static void menu_show( MENU * menu )
{
	if (!menu->wind)
	{
		menu->wind = window_create(&grd_curscreen->sc_canvas, menu->x, menu->y, menu->w, menu->h, 
								   ((menu == &Menu[0]) ? menubar_handler : menu_handler), menu);
		if (!menu->wind)
			return;
		
		if (menu == &Menu[0])
			window_set_modal(Menu[0].wind, 0);	// allow windows behind the menubar to accept events (e.g. the keypad dialogs)
	}

	window_set_visible(menu->wind, 1);
	
	// Mark as displayed.
	menu->Displayed = 1;
}

//-------------------------- Hide a menu -----------------------

static void menu_hide( MENU * menu )
{

	// Can't hide if it's not already drawn
	if (!menu->Displayed) return;
	
	if ((menu != &Menu[0]) && menu->wind)
		window_set_visible(menu->wind, 0);	// don't draw or receive events

	menu->Active = 0;
	if (Menu[0].wind && menu == &Menu[0])
		window_set_modal(Menu[0].wind, 0);
		
	// Mark as hidden.
	menu->Displayed = 0;
}


//------------------------- Move the menu bar ------------------

static void menu_move_bar_to( MENU * menu, int number )
{
	int old_item;

	old_item = menu->CurrentItem;
	menu->CurrentItem = number;
	
	if (menu->Displayed && (number != old_item))
	{
		item_show( menu, old_item );
		item_show( menu, number );
	}
}

//------------------------ Match keypress to item ------------------
static int menu_match_keypress( MENU * menu, int keypress )
{
	int i;
	char c;
	if ((keypress & KEY_CTRLED) || (keypress & KEY_SHIFTED))
		return -1;
	
	keypress &= 0xFF;

	c = key_ascii();
			
	for (i=0; i< menu->NumItems; i++ )
	{
		auto letter = strrchr(menu->Item[i].Text.get(), CC_UNDERLINE);
		if (letter)
		{
			letter++;
			if (c==tolower(*letter))
				return i;
		}
	}
	return -1;
}


static int menu_is_mouse_on( ITEM * item )
{
	int x, y, z;
	
	mouse_get_pos(&x, &y, &z);

	if ((x >= item->x) &&
		(x < item->x + item->w ) &&
		(y >= item->y) &&
		(y <= item->y + item->h ) )
		return 1;
	else
		return 0;
}

static int menu_check_mouse_item( MENU * menu )
{
	int i;
	
	for (i=0; i<menu->NumItems; i++ )
	{
		if (menu_is_mouse_on( &menu->Item[i] ))
		{
			if (menu->Item[i].Text[0] == '-')
				return -1;
			else
				return i;
		}
	}
	return -1;
}


static void menu_hide_all()
{
	range_for (auto &i, partial_range(Menu, 1u, num_menus))
		menu_hide(&i);
	
	Menu[0].ShowBar = 0;
	Menu[0].Active = 0;
	if (Menu[0].wind)
		window_set_modal(Menu[0].wind, 0);
	menu_show( &Menu[0] );

}


static int state2_alt_down;

static window_event_result do_state_0(const d_event &event)
{
	int i;
	int keypress = 0;
	
	if (event.type == EVENT_KEY_COMMAND)
		keypress = event_key_get(event);
	
	Menu[0].Active = 0;
	if (Menu[0].wind)
		window_set_modal(Menu[0].wind, 0);
	Menu[0].ShowBar = 0;

	if ( keypress & KEY_ALTED )	{
		i = menu_match_keypress( &Menu[0], keypress );
		if (i > -1 )
		{
			Menu[0].CurrentItem = i;
			Menu[0].Active = 0;
			if (Menu[0].wind)
				window_set_modal(Menu[0].wind, 0);

			state = 3;	
			Menu[ CMENU ].ShowBar = 1;
			Menu[ CMENU ].Active = 1;
			Menu[0].ShowBar = 1;
	
			menu_show( &Menu[ CMENU ] );
			menu_show( &Menu[0] );
			return window_event_result::handled;
		}
	}
	
	range_for (auto &i, partial_range(Menu, num_menus))
		range_for (auto &j, partial_range(i.Item, i.NumItems))
		{
			if ( j.Hotkey == keypress )
			{
				if (j.user_function)
					j.user_function();
				return window_event_result::handled;
			}
		}
		
	if (keypress & KEY_ALTED)
	//if ( (keypress & 0xFF) == KEY_LALT )
	{
		// Make sure the menubar receives events exclusively
		state = 1;
		Menu[0].Active = 1;
		
		// Put the menubar in front - hope this doesn't mess anything up by leaving it there
		// If it does, will need to remember the previous front window and restore it.
		// (Personally, I just use either the mouse or 'hotkeys' for menus)
		window_select(*Menu[0].wind);

		window_set_modal(Menu[0].wind, 1);
		menu_show( &Menu[0] );
		return window_event_result::handled;
	}

	i = menu_check_mouse_item( &Menu[0] );

	if ( B1_JUST_PRESSED && (i > -1))
	{
		Menu[0].CurrentItem = i;
		state = 3;	
		Menu[ CMENU ].ShowBar = 1;
		Menu[0].ShowBar = 1;
		Menu[ CMENU ].Active = 1;
		Menu[0].Active = 0;
		window_set_modal(Menu[0].wind, 0);
		menu_show( &Menu[ CMENU ] );
		menu_show( &Menu[0] );
		return window_event_result::handled;
	}
	return window_event_result::ignored;
}

static window_event_result do_state_1(const d_event &event)
{
	int i;
	int keypress = 0;
	window_event_result rval = window_event_result::ignored;
	
	if (event.type == EVENT_KEY_COMMAND)
		keypress = event_key_get(event);
	
	if ((event.type == EVENT_KEY_RELEASE) && !(event_key_get(event) & KEY_ALTED))
	{
		state = 2;
		state2_alt_down = 0;
		Menu[0].ShowBar = 1;
		Menu[0].Active = 1;
		menu_show( &Menu[0] );
		rval = window_event_result::handled;
  	}

	i = menu_match_keypress( &Menu[0], keypress );
	
	if (i > -1 )
	{
		Menu[0].CurrentItem = i;
		Menu[0].Active = 0;
		window_set_modal(Menu[0].wind, 0);
		state = 3;	
		Menu[ CMENU ].ShowBar = 1;
		Menu[ CMENU ].Active = 1;
		Menu[0].ShowBar = 1;

		menu_show( &Menu[ CMENU ] );
		menu_show( &Menu[0] );
		rval = window_event_result::handled;
	}

	i = menu_check_mouse_item( &Menu[0] );

	if ( (i == -1) && B1_JUST_RELEASED )
	{
		state = 0;
		menu_hide_all();
		return window_event_result::handled;
	}

	if ( B1_JUST_PRESSED && (i > -1))
	{
		Menu[0].CurrentItem = i;
		state = 3;	
		Menu[ CMENU ].ShowBar = 1;
		Menu[ CMENU ].Active = 1;
		Menu[0].ShowBar = 1;
		Menu[0].Active = 0;
		window_set_modal(Menu[0].wind, 0);
		menu_show( &Menu[ CMENU ] );
		menu_show( &Menu[0] );
		return window_event_result::handled;
	}
	
	return rval;
}

static window_event_result do_state_2(const d_event &event)
{
	int i;
	int keypress = 0;
	window_event_result rval = window_event_result::ignored;
	if (event.type == EVENT_KEY_COMMAND)
		keypress = event_key_get(event);
		
	if (keypress & KEY_ALTED)
	{
 		state2_alt_down = 1;
		rval = window_event_result::handled;
	}

	if ((event.type == EVENT_KEY_RELEASE) && !(event_key_get(event) & KEY_ALTED) && state2_alt_down)
	{
		state = 0;
		menu_hide_all();
		rval = window_event_result::handled;
	}			

	switch( keypress )
	{
	case KEY_ESC:
		state = 0;
		menu_hide_all();
		return window_event_result::handled;
	case KEY_LEFT:
	case KEY_PAD4:
		i = Menu[0].CurrentItem-1;
		if (i < 0 ) i = Menu[0].NumItems-1;
		menu_move_bar_to( &Menu[0], i );
		return window_event_result::handled;
	case KEY_RIGHT:
	case KEY_PAD6:
		i = Menu[0].CurrentItem+1;
		if (i >= Menu[0].NumItems ) i = 0;
		menu_move_bar_to( &Menu[0], i );
		return window_event_result::handled;
	case KEY_ENTER:
	case KEY_PADENTER:
	case KEY_DOWN:
	case KEY_PAD2:
		state = 3;	
		Menu[ CMENU ].ShowBar = 1;
		Menu[ CMENU ].Active = 1;
		Menu[0].Active = 0;
		window_set_modal(Menu[0].wind, 0);
		menu_show( &Menu[ 0 ] );
		menu_show( &Menu[ CMENU ] );
		return window_event_result::handled;
	
	default:
		i = menu_match_keypress( &Menu[0], keypress );
	
		if (i > -1 )
		{
			Menu[0].CurrentItem = i;
			Menu[0].Active = 0;
			window_set_modal(Menu[0].wind, 0);
			state = 3;	
			Menu[ CMENU ].ShowBar = 1;
			Menu[ CMENU ].Active = 1;
			Menu[0].ShowBar = 1;
			menu_show( &Menu[ CMENU ] );
			menu_show( &Menu[0] );
			return window_event_result::handled;
		}

		i = menu_check_mouse_item( &Menu[0] );

		if ( (i == -1) && B1_JUST_RELEASED )
		{
			state = 0;
			menu_hide_all();
			return window_event_result::handled;
		}

		if (i > -1)
		{
			Menu[0].CurrentItem = i;
			Menu[0].Active = 0;
			window_set_modal(Menu[0].wind, 0);
			state = 3;	
			Menu[ CMENU ].ShowBar = 1;
			Menu[ CMENU ].Active = 1;
			Menu[0].ShowBar = 1;
			menu_show( &Menu[ CMENU ] );
			menu_show( &Menu[0] );
			return window_event_result::handled;
		}
	}
	
	return rval;
}



static window_event_result menu_handler(window *, const d_event &event, MENU *menu)
{
	int i;
	int keypress = 0;
	
	if (state != 3)
		return window_event_result::ignored;
	
	if (event.type == EVENT_KEY_COMMAND)
		keypress = event_key_get(event);
	else if (event.type == EVENT_WINDOW_CLOSE)	// quitting
	{
		state = 0;
		menu_hide_all();
		menu->wind = NULL;
		return window_event_result::ignored;
	}
	window_event_result rval = window_event_result::ignored;
	switch( keypress )
	{
		case 0:
			break;
		case KEY_ESC:
			state = 0;
			menu_hide_all();
			rval = window_event_result::handled;
			break;
		case KEY_DOWN:
		case KEY_PAD2:
			i = Menu[ CMENU ].CurrentItem;
			do {
				i++;		
				if ( i >= Menu[ CMENU ].NumItems )
					i = 0;
			} while( Menu[CMENU].Item[i].Text[0] == '-');
			menu_move_bar_to( &Menu[ CMENU ], i );
			rval = window_event_result::handled;
			break;
		case KEY_UP:
		case KEY_PAD8:
			i = Menu[ CMENU ].CurrentItem;
			do 
			{
				i--;
				if ( i < 0 )
					i = Menu[ CMENU ].NumItems-1;
			} while( Menu[CMENU].Item[i].Text[0] == '-');
			menu_move_bar_to( &Menu[ CMENU ], i );
			rval = window_event_result::handled;
			break;
		case KEY_RIGHT:
		case KEY_PAD6:
			menu_hide( &Menu[ CMENU ] );
			i = Menu[0].CurrentItem+1;
			if (i >= Menu[0].NumItems ) i = 0;
			menu_move_bar_to( &Menu[0], i );
			Menu[CMENU].ShowBar = 1;
			Menu[CMENU].Active = 1;
			menu_show( &Menu[CMENU] );
			rval = window_event_result::handled;
			break;
		case KEY_LEFT:
		case KEY_PAD4:
			menu_hide( &Menu[ CMENU ] );
			i = Menu[0].CurrentItem-1;
			if (i < 0 ) i = Menu[0].NumItems-1;
			menu_move_bar_to( &Menu[0], i );
			Menu[ CMENU ].ShowBar = 1;
			Menu[CMENU].Active = 1;
			menu_show( &Menu[ CMENU ] );
			rval = window_event_result::handled;
			break;
		case KEY_ENTER:
		case KEY_PADENTER:
			state = 0;
			menu_hide_all();

			if (Menu[CMENU].Item[ Menu[CMENU].CurrentItem ].user_function)
				Menu[CMENU].Item[ Menu[CMENU].CurrentItem ].user_function();
			
			rval = window_event_result::handled;
			break;
			
		default:
			i = menu_match_keypress( &Menu[ CMENU ], keypress );

			if (i > -1 )
			{
				menu_move_bar_to( &Menu[ CMENU ], i );
				state = 0;
				menu_hide_all();
								
				if (Menu[CMENU].Item[ Menu[CMENU].CurrentItem ].user_function)
					Menu[CMENU].Item[ Menu[CMENU].CurrentItem ].user_function();
				rval = window_event_result::handled;
				break;
			}
	}
	
	if (event.type == EVENT_MOUSE_MOVED || B1_JUST_RELEASED)
	{
		i = menu_check_mouse_item( &Menu[CMENU] );
			
		if (i > -1 )
		{
			if ( B1_JUST_RELEASED )
			{
				menu_move_bar_to( &Menu[ CMENU ], i );
				state = 0;
				menu_hide_all();
								
				if (Menu[CMENU].Item[ Menu[CMENU].CurrentItem ].user_function)
					Menu[CMENU].Item[ Menu[CMENU].CurrentItem ].user_function();
				rval = window_event_result::handled;
			}
			else
			{
				menu_move_bar_to( &Menu[ CMENU ], i );
				rval = window_event_result::handled;
			}
		} else {
			i = menu_check_mouse_item( &Menu[0] );

			if (i > -1)
			{
				if ( Menu[0].CurrentItem != i)	{
					menu_hide( &Menu[ CMENU ] );
					menu_move_bar_to( &Menu[0], i );
					Menu[ CMENU ].ShowBar = 1;
					Menu[CMENU].Active = 1;
					menu_show( &Menu[ CMENU ] );
				}

				rval = window_event_result::handled;
			}

			if ( B1_JUST_RELEASED )
			{
				state = 0;
				menu_hide_all();
				rval = window_event_result::handled;
			}
		}
	}
	
	if (event.type == EVENT_WINDOW_DRAW)
	{
		menu_draw(&Menu[CMENU]);
		return window_event_result::handled;
	}
	
	return rval;
}

static window_event_result menubar_handler(window *, const d_event &event, MENU *)
{
	if (event.type == EVENT_WINDOW_DRAW)
	{
		menu_draw(&Menu[0]);
		return window_event_result::handled;
	}
	else if (event.type == EVENT_WINDOW_CLOSE)
	{
		//menu_hide_all();
		//menu_hide( &Menu[0] );
		
		range_for (auto &i, partial_range(Menu, 1u, num_menus))
		{
			if (i.wind)
			{
				window_close(exchange(i.wind, nullptr));
			}
		}
		
		Menu[0].wind = NULL;
	}

	switch (state)
	{
		case 0:
			return do_state_0(event);
		case 1:
			return do_state_1(event);
		case 2:
			return do_state_2(event);
		case 3:
			break;
		default:
			state = 0;
			menu_hide_all();
	}
	return window_event_result::ignored;
}

static void CommaParse(uint_fast32_t n, char * dest, const PHYSFSX_gets_line_t<200>::line_t &source)
{
	int i = 0, j=0, cn = 0;

	// Go to the n'th comma
	while (cn < n )
		if (source[i++] == ',' )
			cn++;
	// Read all the whitespace
	while ( source[i]==' ' || source[i]=='\t' || source[i]==13 || source[i]==10 )
		i++;

	// Read up until the next comma
	while ( source[i] != ',' )
	{
		dest[j] = source[i++];
		j++;		
	}

	// Null-terminate	
	dest[j++] = 0;
}

//translate '&' characters to the underline character
static void ul_xlate(char *s)
{
	while ((s=strchr(s,'&'))!=NULL)
		*s = CC_UNDERLINE;
}


void menubar_init( const char * file )
{
	int i,j, np;
	char buf1[200];
	char buf2[200];
	int menu, item;
		
	num_menus = state = 0;

	// This method should be faster than explicitly setting all the variables (I think)
	Menu = {};

	range_for (auto &i, Menu)
		range_for (auto &j, i.Item)
			j.Hotkey = -1;
	auto infile = PHYSFSX_openReadBuffered(file);

	if (!infile) return;
		
	PHYSFSX_gets_line_t<200> buffer;
	while ( PHYSFSX_fgets( buffer, infile) != NULL )
	{
		if ( buffer[0] == ';' ) continue;
		
		CommaParse( 0, buf1, buffer );
		menu = atoi( buf1 );
		if (menu >= MAXMENUS)
			UserError("Too many menus (%d).",menu);

		CommaParse( 1, buf1, buffer );
		item = atoi(buf1 );
		if (item >= MAXITEMS)
			UserError("Too many items (%d) in menu %d.",item+1,menu);

		CommaParse( 2, buf1, buffer );
		ul_xlate(buf1);

		if (buf1[0] != '-' )
		{
			sprintf( buf2, " %s ", buf1 );
			Menu[menu].Item[item].Text.reset(d_strdup(buf2));
		} else 
			Menu[menu].Item[item].Text.reset(d_strdup(buf1));
		
		Menu[menu].Item[item].InactiveText.reset(d_strdup(Menu[menu].Item[item].Text.get()));
		
		j= 0;
		for (i=0;; i++ )
		{
			np = Menu[menu].Item[item].Text[i];
			if (np != CC_UNDERLINE) 
				Menu[menu].Item[item].InactiveText[j++] = np;
			if (!np)
				break;
		}

		CommaParse( 3, buf1, buffer );
		if (buf1[0]=='{' && buf1[1] =='}')
			Menu[menu].Item[item].Hotkey = -1;
		else			{
			i = DecodeKeyText(buf1);
			if (i<1) {
				UserError("Unknown key, %s, in %s\n", buf1, file );
			} else {
				Menu[menu].Item[item].Hotkey = i;
			}
		}
		CommaParse( 4, buf1, buffer );

		if (buf1[0])
		{
			Menu[menu].Item[item].user_function = func_get(buf1, &np);

			if (Menu[menu].Item[item].user_function==NULL)
			{
				UserError( "Unknown function, %s, in %s\n", buf1, file );
				//ui_messagebox( -2, -2, 1, buffer, "Ok" );
			}
		}
				
		Menu[menu].Item[item].x = Menu[menu].x;
		Menu[menu].Item[item].y = Menu[menu].y;

		int w, h;
		if ( Menu[menu].Item[item].Text[0] == '-' )
		{
			w = 1; h = 3;
		} else {
			gr_get_string_size(Menu[menu].Item[item].Text.get(), &w, &h, nullptr);
			w += 2;
			h += 2;
		}
								
		if (menu==0)	{
			Menu[0].h = h;

			Menu[0].Item[item].x = Menu[0].x + Menu[0].w;

			Menu[0].Item[item].y = Menu[0].y;
			
			Menu[item+1].x = Menu[0].x + Menu[0].w;
			Menu[item+1].y = Menu[0].h - 2;

			Menu[0].Item[item].w = w;
			Menu[0].Item[item].h = h;

			Menu[0].w += w;

		}else	{
			if ( w > Menu[menu].w )
			{
				Menu[menu].w = w;
				range_for (auto &i, partial_range(Menu[menu].Item, Menu[menu].NumItems))
					i.w = Menu[menu].w;
			}
			Menu[menu].Item[item].w = Menu[menu].w;
			Menu[menu].Item[item].x = Menu[menu].x;
			Menu[menu].Item[item].y = Menu[menu].y+Menu[menu].h;
			Menu[menu].Item[item].h = h;
			Menu[menu].h += h;
		}
	
		if ( item >= Menu[menu].NumItems )
		{
			Menu[menu].NumItems = item+1;
		}

		if ( menu >= num_menus )
			num_menus = menu+1;

	}
	Menu[0].w = 700;
}

void menubar_hide()
{
	state = 0;
	menu_hide_all();
	menu_hide( &Menu[0] );
}

void menubar_show()
{
	menu_show( &Menu[0] );
}

void menubar_close()
{
	if (!Menu[0].wind)
		return;
	window_close(exchange(Menu[0].wind, nullptr));
}
