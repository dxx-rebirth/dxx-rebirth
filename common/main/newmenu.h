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


#ifndef _NEWMENU_H
#define _NEWMENU_H

#include "event.h"

typedef struct newmenu newmenu;
typedef struct listbox listbox;

#define NM_TYPE_MENU        0   // A menu item... when enter is hit on this, newmenu_do returns this item number
#define NM_TYPE_INPUT       1   // An input box... fills the text field in, and you need to fill in text_len field.
#define NM_TYPE_CHECK       2   // A check box. Set and get its status by looking at flags field (1=on, 0=off)
#define NM_TYPE_RADIO       3   // Same as check box, but only 1 in a group can be set at a time. Set group fields.
#define NM_TYPE_TEXT        4   // A line of text that does nothing.
#define NM_TYPE_NUMBER      5   // A numeric entry counter.  Changes value from min_value to max_value;
#define NM_TYPE_INPUT_MENU  6   // A inputbox that you hit Enter to edit, when done, hit enter and menu leaves.
#define NM_TYPE_SLIDER      7   // A slider from min_value to max_value. Draws with text_len chars.

#define NM_MAX_TEXT_LEN     255

typedef struct newmenu_item {
	int     type;           // What kind of item this is, see NM_TYPE_????? defines
	int     value;          // For checkboxes and radio buttons, this is 1 if marked initially, else 0
	int     min_value, max_value;   // For sliders and number bars.
	int     group;          // What group this belongs to for radio buttons.
	int     text_len;       // The maximum length of characters that can be entered by this inputboxes
	char    *text;          // The text associated with this item.
	// The rest of these are used internally by by the menu system, so don't set 'em!!
	short   x, y;
	short   w, h;
	short   right_offset;
	char    saved_text[NM_MAX_TEXT_LEN+1];
} newmenu_item;

// Pass an array of newmenu_items and it processes the menu. It will
// return a -1 if Esc is pressed, otherwise, it returns the index of
// the item that was current when Enter was was selected.
// The subfunction function accepts standard events, plus additional
// NEWMENU events in future.  Just pass NULL if you don't want this,
// or return 0 where you don't want to override the default behaviour.
// Title draws big, Subtitle draw medium sized.  You can pass NULL for
// either/both of these if you don't want them.
extern int newmenu_do(const char * title, const char * subtitle, int nitems, newmenu_item *item, int (*subfunction)(newmenu *menu, d_event *event, void *userdata), void *userdata);

// Same as above, only you can pass through what item is initially selected.
extern int newmenu_do1(const char *title, const char *subtitle, int nitems, newmenu_item *item, int (*subfunction)(newmenu *menu, d_event *event, void *userdata), void *userdata, int citem);

// Same as above, only you can pass through what background bitmap to use.
extern int newmenu_do2(const char *title, const char *subtitle, int nitems, newmenu_item *item, int (*subfunction)(newmenu *menu, d_event *event, void *userdata), void *userdata, int citem, const char *filename);

// Same as above, but returns menu instead of citem
extern newmenu *newmenu_do3(const char *title, const char *subtitle, int nitems, newmenu_item *item, int (*subfunction)(newmenu *menu, d_event *event, void *userdata), void *userdata, int citem, const char *filename);

// Tiny menu with GAME_FONT
extern newmenu *newmenu_dotiny(const char * title, const char * subtitle, int nitems, newmenu_item * item, int TabsFlag, int (*subfunction)(newmenu *menu, d_event *event, void *userdata), void *userdata);

// Basically the same as do2 but sets reorderitems flag for weapon priority menu a bit redundant to get lose of a global variable but oh well...
extern int newmenu_doreorder(const char * title, const char * subtitle, int nitems, newmenu_item *item, int (*subfunction)(newmenu *menu, d_event *event, void *userdata), void *userdata);

// Sample Code:
/*
{
	int mmn;
	newmenu_item mm[8];
	char xtext[21];

	strcpy( xtext, "John" );

	mm[0].type=NM_TYPE_MENU; mm[0].text="Play game";
	mm[1].type=NM_TYPE_INPUT; mm[1].text=xtext; mm[1].text_len=20;
	mm[2].type=NM_TYPE_CHECK; mm[2].value=0; mm[2].text="check box";
	mm[3].type=NM_TYPE_TEXT; mm[3].text="-pickone-";
	mm[4].type=NM_TYPE_RADIO; mm[4].value=1; mm[4].group=0; mm[4].text="Radio #1";
	mm[5].type=NM_TYPE_RADIO; mm[5].value=1; mm[5].group=0; mm[5].text="Radio #2";
	mm[6].type=NM_TYPE_RADIO; mm[6].value=1; mm[6].group=0; mm[6].text="Radio #3";
	mm[7].type=NM_TYPE_PERCENT; mm[7].value=50; mm[7].text="Volume";

	mmn = newmenu_do("Descent", "Sample Menu", 8, mm, NULL );
}

*/

// This function pops up a messagebox and returns which choice was selected...
// Example:
// nm_messagebox( "Title", "Subtitle", 2, "Ok", "Cancel", "There are %d objects", nobjects );
// Returns 0 through nchoices-1.
int nm_messagebox(const char *title, int nchoices, ...);
// Same as above, but you can pass a function
int nm_messagebox1(const char *title, int (*subfunction)(newmenu *menu, d_event *event, void *userdata), void *userdata, int nchoices, ...);

newmenu_item *newmenu_get_items(newmenu *menu);
int newmenu_get_nitems(newmenu *menu);
int newmenu_get_citem(newmenu *menu);
struct window *newmenu_get_window(newmenu *menu);
void nm_draw_background(int x1, int y1, int x2, int y2);
void nm_restore_background(int x, int y, int w, int h);

extern const char *Newmenu_allowed_chars;

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

extern char **listbox_get_items(listbox *lb);
extern int listbox_get_nitems(listbox *lb);
extern int listbox_get_citem(listbox *lb);
struct window *listbox_get_window(listbox *lb);
extern void listbox_delete_item(listbox *lb, int item);

extern listbox *newmenu_listbox(const char *title, int nitems, char *items[], int allow_abort_flag, int (*listbox_callback)(listbox *lb, d_event *event, void *userdata), void *userdata);
extern listbox *newmenu_listbox1(const char *title, int nitems, char *items[], int allow_abort_flag, int default_item, int (*listbox_callback)(listbox *lb, d_event *event, void *userdata), void *userdata);

//added on 10/14/98 by Victor Rachels to attempt a fixedwidth font messagebox
newmenu *nm_messagebox_fixedfont(const char *title, int nchoices, ...);
//end this section addition

//should be called whenever the palette changes
extern void newmenu_free_background();

static inline void nm_set_item_menu(newmenu_item *ni, const char *text)
{
	ni->type = NM_TYPE_MENU;
	ni->text = (char *)text;
}

static inline void nm_set_item_text(newmenu_item *ni, const char *text)
{
	ni->type = NM_TYPE_TEXT;
	ni->text = (char *)text;
}

#define NEWMENU_MOUSE

// #define NORMAL_CHECK_BOX    ""
// #define CHECKED_CHECK_BOX   "‚"
// 
// #define NORMAL_RADIO_BOX    ""
// #define CHECKED_RADIO_BOX   "€"
// #define CURSOR_STRING       "_"
// #define SLIDER_LEFT         "ƒ"  // 131
// #define SLIDER_RIGHT        "„"  // 132
// #define SLIDER_MIDDLE       "…"  // 133
// #define SLIDER_MARKER       "†"  // 134
// #define UP_ARROW_MARKER     "‡"  // 135
// #define DOWN_ARROW_MARKER   "ˆ"  // 136
#define NORMAL_CHECK_BOX    "\201"
#define CHECKED_CHECK_BOX   "\202"

#define NORMAL_RADIO_BOX    "\177"
#define CHECKED_RADIO_BOX   "\200"
#define CURSOR_STRING       "_"
#define SLIDER_LEFT         "\203"  // 131
#define SLIDER_RIGHT        "\204"  // 132
#define SLIDER_MIDDLE       "\205"  // 133
#define SLIDER_MARKER       "\206"  // 134
#if defined(DXX_BUILD_DESCENT_I)
#define UP_ARROW_MARKER     "+"  // 135
#define DOWN_ARROW_MARKER   "+"  // 136
#elif defined(DXX_BUILD_DESCENT_II)
#define UP_ARROW_MARKER     ((grd_curcanv->cv_font==GAME_FONT)?"\202":"\207")  // 135
#define DOWN_ARROW_MARKER   ((grd_curcanv->cv_font==GAME_FONT)?"\200":"\210")  // 136
#endif

#define BORDERX (15*(SWIDTH/320))
#define BORDERY (15*(SHEIGHT/200))

#define DXX_NEWMENU_VARIABLE	m
#define DXX_ENUM_CHECK(S,OPT,V)	OPT,
#define DXX_ENUM_SLIDER(S,OPT,V,MIN,MAX)	OPT,
#define DXX_ENUM_SCALE_SLIDER(S,OPT,V,MIN,MAX,SCALE)	OPT,
#define DXX_ENUM_MENU(S,OPT)	OPT,
#define DXX_ENUM_TEXT(S,OPT)	OPT,
#define DXX_ENUM_INPUT(S,OPT,MAX_TEXT_LEN)	OPT,
#define DXX_COUNT_CHECK(S,OPT,V)	+1
#define DXX_COUNT_SLIDER(S,OPT,V,MIN,MAX)	+1
#define DXX_COUNT_SCALE_SLIDER(S,OPT,V,MIN,MAX,SCALE)	+1
#define DXX_COUNT_MENU(S,OPT)	+1
#define DXX_COUNT_TEXT(S,OPT)	+1
#define DXX_COUNT_INPUT(S,OPT,MAX_TEXT_LEN)	+1
#define DXX_ADD_CHECK(S,OPT,V)	\
	(DXX_NEWMENU_VARIABLE)[(OPT)].type=NM_TYPE_CHECK;	\
	(DXX_NEWMENU_VARIABLE)[(OPT)].text=(S);	\
	(DXX_NEWMENU_VARIABLE)[(OPT)].value=V;
#define DXX_ADD_SLIDER(S,OPT,V,MIN,MAX)	\
	(DXX_NEWMENU_VARIABLE)[(OPT)].type=NM_TYPE_SLIDER;	\
	(DXX_NEWMENU_VARIABLE)[(OPT)].value=V;	\
	(DXX_NEWMENU_VARIABLE)[(OPT)].text=(S);	\
	(DXX_NEWMENU_VARIABLE)[(OPT)].min_value=(MIN);	\
	(DXX_NEWMENU_VARIABLE)[(OPT)].max_value=(MAX);
#define DXX_ADD_SCALE_SLIDER(S,OPT,V,MIN,MAX,SCALE)	\
	DXX_ADD_SLIDER((S),(OPT),(V) / (SCALE),(MIN),(MAX))
#define DXX_ADD_MENU(S,OPT)	\
	nm_set_item_menu(&((DXX_NEWMENU_VARIABLE)[(OPT)]), (S));
#define DXX_ADD_TEXT(S,OPT)	\
	nm_set_item_text(&((DXX_NEWMENU_VARIABLE)[(OPT)]), (S));
#define DXX_ADD_INPUT(S,OPT,MAX_TEXT_LEN)	\
	(DXX_NEWMENU_VARIABLE)[(OPT)].type = NM_TYPE_INPUT;	\
	(DXX_NEWMENU_VARIABLE)[(OPT)].text=(S);	\
	(DXX_NEWMENU_VARIABLE)[(OPT)].text_len=MAX_TEXT_LEN;
#define DXX_READ_CHECK(S,OPT,V)	\
	V = (DXX_NEWMENU_VARIABLE)[(OPT)].value;
#define DXX_READ_SLIDER(S,OPT,V,MIN,MAX)	\
	V = (DXX_NEWMENU_VARIABLE)[(OPT)].value;
#define DXX_READ_SCALE_SLIDER(S,OPT,V,MIN,MAX,SCALE)	\
	V = (DXX_NEWMENU_VARIABLE)[(OPT)].value * (SCALE);
#define DXX_READ_MENU(S,OPT)	/* handled specially */
#define DXX_READ_TEXT(S,OPT)	/* handled specially */
#define DXX_READ_INPUT(S,OPT,MAX_TEXT_LEN)	/* handled specially */

#endif /* _NEWMENU_H */

