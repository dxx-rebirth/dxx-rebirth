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

#ifndef _UI_H
#define _UI_H

typedef struct {
	char	description[100];
	char 	* buttontext[17];
	int	numkeys;
	short keycode[100];
	int 	function_number[100];
} UI_KEYPAD;

typedef struct
{
	unsigned int frame;
	int type;
	int data;
} UI_EVENT;

#define BASE_GADGET             \
	short           kind;       \
	struct _gadget  * prev;     \
	struct _gadget  * next;     \
	struct _gadget  * when_tab;  \
	struct _gadget  * when_btab; \
	struct _gadget  * when_up;    \
	struct _gadget  * when_down;   \
	struct _gadget  * when_left;   \
	struct _gadget  * when_right;  \
	struct _gadget  * parent;    \
	int             status;     \
	int             oldstatus;  \
	grs_canvas *    canvas;     \
	int             hotkey;     \
	short           x1,y1,x2,y2;


typedef struct _gadget {
	BASE_GADGET
	unsigned char rsvd[256];
} UI_GADGET;


typedef struct  {
	BASE_GADGET
	int         trap_key;
	int      (*user_function)(void);
} UI_GADGET_KEYTRAP;

typedef struct  {
	BASE_GADGET
	short           width, height;
	short           b1_held_down;
	short           b1_clicked;
	short           b1_double_clicked;
	short           b1_dragging;
	short           b1_drag_x1, b1_drag_y1;
	short           b1_drag_x2, b1_drag_y2;
	short           b1_done_dragging;
	int             keypress;
	short           mouse_onme;
	short           mouse_x, mouse_y;
	grs_bitmap *    bitmap;
} UI_GADGET_USERBOX;

typedef struct  {
	BASE_GADGET
	short           width, height;
	char            * text;
	short           position;
	short           oldposition;
	short           pressed;
	int          	 (*user_function)(void);
	int          	 (*user_function1)(void);
	int				 hotkey1;
	int				 dim_if_no_function;
} UI_GADGET_BUTTON;


typedef struct  {
	BASE_GADGET
	short           width, height;
	char            * text;
	short           length;
	short           slength;
	short           position;
	short           oldposition;
	short           pressed;
	short           first_time;
} UI_GADGET_INPUTBOX;

typedef struct  {
	BASE_GADGET
	short           width, height;
	char            * text;
	short           position;
	short           oldposition;
	short           pressed;
	short           group;
	short           flag;
} UI_GADGET_RADIO;


typedef struct  {
	BASE_GADGET
	char 				 *text;
	short 		    width, height;
	byte            flag;
	byte            pressed;
	byte            position;
	byte            oldposition;
	int             trap_key;
	int          	(*user_function)(void);
} UI_GADGET_ICON;


typedef struct  {
	BASE_GADGET
	short           width, height;
   char            * text;
	short           position;
	short           oldposition;
	short           pressed;
	short           group;
	short           flag;
} UI_GADGET_CHECKBOX;


typedef struct  {
	BASE_GADGET
	short           horz;
	short           width, height;
	int             start;
	int             stop;
	int             position;
	int             window_size;
	int             fake_length;
	int             fake_position;
	int             fake_size;
	UI_GADGET_BUTTON * up_button;
	UI_GADGET_BUTTON * down_button;
	unsigned int    last_scrolled;
	short           drag_x, drag_y;
	int             drag_starting;
	int             dragging;
	int             moved;
} UI_GADGET_SCROLLBAR;

typedef struct  {
	BASE_GADGET
	short           width, height;
	char            * list;
	int             text_width;
	int             num_items;
	int             num_items_displayed;
	int             first_item;
	int             old_first_item;
	int             current_item;
	int             selected_item;
	int             old_current_item;
	unsigned int    last_scrolled;
	int             dragging;
	int             textheight;
	UI_GADGET_SCROLLBAR * scrollbar;
	int             moved;
} UI_GADGET_LISTBOX;

typedef struct _ui_window {
	short           x, y;
	short           width, height;
	short           text_x, text_y;
	grs_canvas *    canvas;
	grs_canvas *    oldcanvas;
	grs_bitmap *    background;
	UI_GADGET *     gadget;
	UI_GADGET *     keyboard_focus_gadget;
	struct _ui_window * next;
	struct _ui_window * prev;
} UI_WINDOW;

typedef struct  {
	short           new_dx, new_dy;
	short           new_buttons;
	short           x, y;
	short           dx, dy;
	short           hidden;
	short           backwards;
	short           b1_status;
	short           b1_last_status;
	short           b2_status;
	short           b2_last_status;
	short           b3_status;
	short           b3_last_status;
	short           bg_x, bg_y;
	short           bg_saved;
	grs_bitmap *    background;
	grs_bitmap *    pointer;
	unsigned int    time_lastpressed;
	short           moved;
} UI_MOUSE;

#define BUTTON_PRESSED          1
#define BUTTON_RELEASED         2
#define BUTTON_JUST_PRESSED     4
#define BUTTON_JUST_RELEASED    8
#define BUTTON_DOUBLE_CLICKED   16

#define B1_PRESSED          (Mouse.b1_status & BUTTON_PRESSED)
#define B1_RELEASED         (Mouse.b1_status & BUTTON_RELEASED)
#define B1_JUST_PRESSED     (Mouse.b1_status & BUTTON_JUST_PRESSED)
#define B1_JUST_RELEASED    (Mouse.b1_status & BUTTON_JUST_RELEASED)
#define B1_DOUBLE_CLICKED   (Mouse.b1_status & BUTTON_DOUBLE_CLICKED)

extern grs_font * ui_small_font;

extern UI_MOUSE Mouse;
extern UI_WINDOW * CurWindow;
extern UI_WINDOW * FirstWindow;
extern UI_WINDOW * LastWindow;

extern unsigned char CBLACK,CGREY,CWHITE,CBRIGHT,CRED;
extern UI_GADGET * selected_gadget;
extern int last_keypress;

extern void Hline(short x1, short x2, short y );
extern void Vline(short y1, short y2, short x );
extern void ui_string_centered( short x, short y, char * s );
extern void ui_draw_box_out( short x1, short y1, short x2, short y2 );
extern void ui_draw_box_in( short x1, short y1, short x2, short y2 );
extern void ui_draw_line_in( short x1, short y1, short x2, short y2 );


void ui_init();
void ui_close();
int MessageBox( short x, short y, int NumButtons, char * text, ... );
void ui_string_centered( short x, short y, char * s );
int PopupMenu( int NumItems, char * text[] );

extern void ui_mouse_init();
extern grs_bitmap * ui_mouse_set_pointer( grs_bitmap * new );

extern void ui_mouse_process();
extern void ui_mouse_hide();
extern void ui_mouse_show();

#define WIN_BORDER 1
#define WIN_FILLED 2
#define WIN_SAVE_BG 4
#define WIN_DIALOG (4+2+1)

extern UI_WINDOW * ui_open_window( short x, short y, short w, short h, int flags );
extern void ui_close_window( UI_WINDOW * wnd );

extern UI_GADGET * ui_gadget_add( UI_WINDOW * wnd, short kind, short x1, short y1, short x2, short y2 );
extern UI_GADGET_BUTTON * ui_add_gadget_button( UI_WINDOW * wnd, short x, short y, short w, short h, char * text, int (*function_to_call)(void) );
extern void ui_gadget_delete_all( UI_WINDOW * wnd );
extern void ui_window_do_gadgets( UI_WINDOW * wnd );
extern void ui_draw_button( UI_GADGET_BUTTON * button );

extern int ui_mouse_on_gadget( UI_GADGET * gadget );

extern void ui_button_do( UI_GADGET_BUTTON * button, int keypress );

extern void ui_listbox_do( UI_GADGET_LISTBOX * listbox, int keypress );
extern void ui_draw_listbox( UI_GADGET_LISTBOX * listbox );
extern UI_GADGET_LISTBOX * ui_add_gadget_listbox( UI_WINDOW * wnd, short x, short y, short w, short h, short numitems, char * list, int text_width );

extern void ui_mega_process();

extern void ui_get_button_size( char * text, int * width, int * height );

extern UI_GADGET_SCROLLBAR * ui_add_gadget_scrollbar( UI_WINDOW * wnd, short x, short y, short w, short h, int start, int stop, int position, int window_size  );
extern void ui_scrollbar_do( UI_GADGET_SCROLLBAR * scrollbar, int keypress );
extern void ui_draw_scrollbar( UI_GADGET_SCROLLBAR * scrollbar );


extern void ui_wprintf( UI_WINDOW * wnd, char * format, ... );
extern void ui_wprintf_at( UI_WINDOW * wnd, short x, short y, char * format, ... );

extern void ui_draw_radio( UI_GADGET_RADIO * radio );
extern UI_GADGET_RADIO * ui_add_gadget_radio( UI_WINDOW * wnd, short x, short y, short w, short h, short group, char * text );
extern void ui_radio_do( UI_GADGET_RADIO * radio, int keypress );

extern void ui_draw_checkbox( UI_GADGET_CHECKBOX * checkbox );
extern UI_GADGET_CHECKBOX * ui_add_gadget_checkbox( UI_WINDOW * wnd, short x, short y, short w, short h, short group, char * text );
extern void ui_checkbox_do( UI_GADGET_CHECKBOX * checkbox, int keypress );

extern UI_GADGET * ui_gadget_get_prev( UI_GADGET * gadget );
extern UI_GADGET * ui_gadget_get_next( UI_GADGET * gadget );
extern void ui_gadget_calc_keys( UI_WINDOW * wnd);

extern void ui_listbox_change( UI_WINDOW * wnd, UI_GADGET_LISTBOX * listbox, short numitems, char * list, int text_width );


extern void ui_draw_inputbox( UI_GADGET_INPUTBOX * inputbox );
extern UI_GADGET_INPUTBOX * ui_add_gadget_inputbox( UI_WINDOW * wnd, short x, short y, short w, short h, char * text );
extern void ui_inputbox_do( UI_GADGET_INPUTBOX * inputbox, int keypress );


extern void ui_userbox_do( UI_GADGET_USERBOX * userbox, int keypress );
extern UI_GADGET_USERBOX * ui_add_gadget_userbox( UI_WINDOW * wnd, short x, short y, short w, short h );
extern void ui_draw_userbox( UI_GADGET_USERBOX * userbox );


extern int MenuX( int x, int y, int NumButtons, char * text[] );

// Changes to a drive if valid.. 1=A, 2=B, etc
// If flag, then changes to it.
// Returns 0 if not-valid, 1 if valid.
int file_chdrive( int DriveNum, int flag );

// Changes to directory in dir.  Even drive is changed.
// Returns 1 if failed.
//  0 = Changed ok.
//  1 = Invalid disk drive.
//  2 = Invalid directory.

int file_chdir( char * dir );

int file_getdirlist( int MaxNum, char list[][13] );
int file_getfilelist( int MaxNum, char list[][13], char * filespec );
int ui_get_filename( char * filename, char * Filespec, char * message  );


void * ui_malloc( int size );
void ui_free( void * buffer );

UI_GADGET_KEYTRAP * ui_add_gadget_keytrap( UI_WINDOW * wnd, int key_to_trap, int (*function_to_call)(void)  );
void ui_keytrap_do( UI_GADGET_KEYTRAP * keytrap, int keypress );

void ui_mouse_close();

#define UI_RECORD_MOUSE     1
#define UI_RECORD_KEYS      2
#define UI_STATUS_NORMAL    0
#define UI_STATUS_RECORDING 1
#define UI_STATUS_PLAYING   2
#define UI_STATUS_FASTPLAY  3

int ui_record_events( int NumberOfEvents, UI_EVENT * buffer, int Flags );
int ui_play_events_realtime( int NumberOfEvents, UI_EVENT * buffer );
int ui_play_events_fast( int NumberOfEvents, UI_EVENT * buffer );
int ui_recorder_status();
void ui_set_playback_speed( int speed );

extern unsigned int ui_number_of_events;
extern unsigned int ui_event_counter;


int ui_get_file( char * filename, char * Filespec  );

int MessageBoxN( short xc, short yc, int NumButtons, char * text, char * Button[] );

void ui_draw_icon( UI_GADGET_ICON * icon );
void ui_icon_do( UI_GADGET_ICON * icon, int keypress );
UI_GADGET_ICON * ui_add_gadget_icon( UI_WINDOW * wnd, char * text, short x, short y, short w, short h, int k,int (*f)(void) );

int GetKeyCode(char * text);
int DecodeKeyText( char * text );
void GetKeyDescription( char * text, int keypress );

extern void menubar_init(char * filename );
extern void menubar_do( int keypress );
extern void menubar_close();
extern void menubar_hide();
extern void menubar_show();

void ui_pad_init();
void ui_pad_close();
void ui_pad_activate( UI_WINDOW * wnd, int x, int y );
void ui_pad_deactivate();
void ui_pad_goto(int n);
void ui_pad_goto_next();
void ui_pad_goto_prev();
void ui_pad_read( int n, char * filename );
int ui_pad_get_current();

void ui_barbox_open( char * text, int length );
int ui_barbox_update( int position );
void ui_barbox_close();

void ui_reset_idle_seconds(void);
int ui_get_idle_seconds(void);

extern char filename_list[300][13];
extern char directory_list[100][13];

extern int ui_button_any_drawn;

#endif

