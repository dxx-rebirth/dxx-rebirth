/* $Id: newmenu.h,v 1.3 2002-08-30 08:04:44 btb Exp $ */
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

#ifndef _NEWMENU_H
#define _NEWMENU_H

#define NM_TYPE_MENU        0   // A menu item... when enter is hit on this, newmenu_do returns this item number
#define NM_TYPE_INPUT       1   // An input box... fills the text field in, and you need to fill in text_len field.
#define NM_TYPE_CHECK       2   // A check box. Set and get its status by looking at flags field (1=on, 0=off)
#define NM_TYPE_RADIO       3   // Same as check box, but only 1 in a group can be set at a time. Set group fields.
#define NM_TYPE_TEXT        4   // A line of text that does nothing.
#define NM_TYPE_NUMBER      5   // A numeric entry counter.  Changes value from min_value to max_value;
#define NM_TYPE_INPUT_MENU  6   // A inputbox that you hit Enter to edit, when done, hit enter and menu leaves.
#define NM_TYPE_SLIDER      7   // A slider from min_value to max_value. Draws with text_len chars.

#define NM_MAX_TEXT_LEN     50

typedef struct newmenu_item {
	int     type;           // What kind of item this is, see NM_TYPE_????? defines
	int     value;          // For checkboxes and radio buttons, this is 1 if marked initially, else 0
	int     min_value, max_value;   // For sliders and number bars.
	int     group;          // What group this belongs to for radio buttons.
	int     text_len;       // The maximum length of characters that can be entered by this inputboxes
	char    *text;          // The text associated with this item.
	// The rest of these are used internally by by the menu system, so don't set 'em!!
	short	x, y;
	short 	w, h;
	short 	right_offset;
	ubyte 	redraw;
	char	saved_text[NM_MAX_TEXT_LEN+1];
} newmenu_item;

// Pass an array of newmenu_items and it processes the menu. It will
// return a -1 if Esc is pressed, otherwise, it returns the index of 
// the item that was current when Enter was was selected.
// The subfunction function gets called constantly, so you can dynamically
// change the text of an item.  Just pass NULL if you don't want this.
// Title draws big, Subtitle draw medium sized.  You can pass NULL for
// either/both of these if you don't want them.
extern int newmenu_do( char * title, char * subtitle, int nitems, newmenu_item * item, void (*subfunction)(int nitems, newmenu_item * items, int *last_key, int citem ) );

// Same as above, only you can pass through what item is initially selected.
extern int newmenu_do1( char * title, char * subtitle, int nitems, newmenu_item * item, void (*subfunction)(int nitems,newmenu_item * items, int * last_key, int citem), int citem );

// Same as above, only you can pass through what background bitmap to use.
extern int newmenu_do2( char * title, char * subtitle, int nitems, newmenu_item * item, void (*subfunction)(int nitems,newmenu_item * items, int * last_key, int citem), int citem, char * filename );

// Same as above, only you can pass through the width & height
extern int newmenu_do3( char * title, char * subtitle, int nitems, newmenu_item * item, void (*subfunction)(int nitems,newmenu_item * items, int * last_key, int citem), int citem, char * filename, int width, int height );


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
			mprintf( 0, "Menu returned: %d\n", mmn );
			}

*/

// This function pops up a messagebox and returns which choice was selected...
// Example:
// nm_messagebox( "Title", "Subtitle", 2, "Ok", "Cancel", "There are %d objects", nobjects );
// Returns 0 through nchoices-1.
int nm_messagebox( char *title, int nchoices, ... );
// Same as above, but you can pass a function
int nm_messagebox1( char *title, void (*subfunction)(int nitems,newmenu_item * items, int * last_key, int citem), int nchoices, ... );

void nm_draw_background(int x1, int y1, int x2, int y2);
void nm_restore_background( int x, int y, int w, int h );

// Returns 0 if no file selected, else filename is filled with selected file.
int newmenu_get_filename( char * title, char * filespec, char * filename, int allow_abort_flag );

//	in menu.c
extern int Max_linear_depth_objects;

extern char *Newmenu_allowed_chars;

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

extern int newmenu_listbox( char * title, int nitems, char * items[], int allow_abort_flag, int (*listbox_callback)( int * citem, int *nitems, char * items[], int *keypress ) );
extern int newmenu_listbox1( char * title, int nitems, char * items[], int allow_abort_flag, int default_item, int (*listbox_callback)( int * citem, int *nitems, char * items[], int *keypress ) );

extern int newmenu_filelist( char * title, char * filespace, char * filename );

//added on 10/14/98 by Victor Rachels to attempt a fixedwidth font messagebox
int nm_messagebox_fixedfont( char *title, int nchoices, ... );
//end this section addition

//should be called whenever the palette changes
extern void nm_remap_background(void);

#endif
