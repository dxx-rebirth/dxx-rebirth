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

/*
 *
 * Header file for user interface
 *
 */

#pragma once

#include <type_traits>
#include "dxxsconf.h"
#include "dsx-ns.h"
#include "fwd-event.h"
#include "fmtcheck.h"
#include "u_mem.h"

#ifdef __cplusplus
#include <cstdint>
#include <string>
#include "fwd-gr.h"
#include "varutil.h"
#include "window.h"
#include "ntstring.h"
#include <array>
#include <memory>

namespace dcx {

struct UI_DIALOG;

struct UI_KEYPAD {
	using buttontext_element_t = std::array<char, 100>;
	using buttontext_t = std::array<buttontext_element_t, 17>;
	UI_KEYPAD();
	unsigned numkeys;
	ntstring<99> description;
	std::array<short, 100> keycode;
	std::array<int, 100> function_number;
	buttontext_t buttontext;
};

struct UI_EVENT
{
	unsigned int frame;
	int type;
	int data;
};

struct UI_GADGET
{
	virtual window_event_result event_handler(UI_DIALOG &dlg, const d_event &) = 0;
	uint8_t           kind;
	short           x1,y1,x2,y2;
	int             hotkey;
	struct UI_GADGET  * prev;     \
	struct UI_GADGET  * next;     \
	struct UI_GADGET  * when_tab;  \
	struct UI_GADGET  * when_btab; \
	struct UI_GADGET  * when_up;    \
	struct UI_GADGET  * when_down;   \
	struct UI_GADGET  * when_left;   \
	struct UI_GADGET  * when_right;  \
	struct UI_GADGET  * parent;    \
	int             status;     \
	int             oldstatus;  \
	grs_subcanvas_ptr canvas;     \
protected:
	~UI_GADGET() = default;
};

struct UI_GADGET_USERBOX final : UI_GADGET
{
	static constexpr auto s_kind = std::integral_constant<uint8_t, 7>{};
	virtual window_event_result event_handler(UI_DIALOG &dlg, const d_event &event) override;
	short           width, height;
	short           b1_held_down;
	short           b1_clicked;
	short           b1_double_clicked;
	short           b1_dragging;
	short           b1_drag_x1, b1_drag_y1;
	short           b1_drag_x2, b1_drag_y2;
	short           b1_done_dragging;
	short           mouse_onme;
	short           mouse_x, mouse_y;
	int             keypress;
	grs_bitmap *    bitmap;
};

struct UI_GADGET_BUTTON final : UI_GADGET
{
	static constexpr auto s_kind = std::integral_constant<uint8_t, 1>{};
	virtual window_event_result event_handler(UI_DIALOG &dlg, const d_event &event) override;
	std::string  text;
	short           width, height;
	short           position;
	short           oldposition;
	short           pressed;
	uint8_t dim_if_no_function;
	int				 hotkey1;
	int          	 (*user_function)(void);
	int          	 (*user_function1)(void);
};

struct UI_GADGET_INPUTBOX final : UI_GADGET
{
	static constexpr auto s_kind = std::integral_constant<uint8_t, 6>{};
	virtual window_event_result event_handler(UI_DIALOG &dlg, const d_event &event) override;
	std::unique_ptr<char[]> text;
	short           width, height;
	short           length;
	short           position;
	short           oldposition;
	short           pressed;
	short           first_time;
};

struct UI_GADGET_RADIO final : UI_GADGET
{
	static constexpr auto s_kind = std::integral_constant<uint8_t, 4>{};
	virtual window_event_result event_handler(UI_DIALOG &dlg, const d_event &event) override;
	RAIIdmem<char[]>  text;
	short           width, height;
	short           position;
	short           oldposition;
	short           pressed;
	short           group;
	short           flag;
};

struct UI_GADGET_ICON final : UI_GADGET
{
	static constexpr auto s_kind = std::integral_constant<uint8_t, 9>{};
	virtual window_event_result event_handler(UI_DIALOG &dlg, const d_event &event) override;
	RAIIdmem<char[]>  text;
	short 		    width, height;
	sbyte           flag;
	sbyte           pressed;
	sbyte           position;
	sbyte           oldposition;
	int             trap_key;
	int          	(*user_function)(void);
};

struct UI_GADGET_CHECKBOX final : UI_GADGET
{
	static constexpr auto s_kind = std::integral_constant<uint8_t, 5>{};
	virtual window_event_result event_handler(UI_DIALOG &dlg, const d_event &event) override;
	RAIIdmem<char[]>  text;
	short           width, height;
	short           position;
	short           oldposition;
	short           pressed;
	short           group;
	short           flag;
};

struct UI_GADGET_SCROLLBAR final : UI_GADGET
{
	static constexpr auto s_kind = std::integral_constant<uint8_t, 3>{};
	virtual window_event_result event_handler(UI_DIALOG &dlg, const d_event &event) override;
	short           horz;
	short           width, height;
	int             start;
	int             stop;
	int             position;
	int             window_size;
	int             fake_length;
	int             fake_position;
	int             fake_size;
	std::unique_ptr<UI_GADGET_BUTTON> up_button, down_button;
	fix64           last_scrolled;
	short           drag_x, drag_y;
	int             drag_starting;
	int             dragging;
	int             moved;
};

struct UI_GADGET_LISTBOX final : UI_GADGET
{
	static constexpr auto s_kind = std::integral_constant<uint8_t, 2>{};
	virtual window_event_result event_handler(UI_DIALOG &dlg, const d_event &event) override;
	const char            *const *list;
	short           width, height;
	int             num_items;
	int             num_items_displayed;
	int             first_item;
	int             old_first_item;
	int             current_item;
	int             selected_item;
	int             old_current_item;
	int             dragging;
	int             textheight;
	int             moved;
	std::unique_ptr<UI_GADGET_SCROLLBAR> scrollbar;
	fix64           last_scrolled;
};

enum dialog_flags
{
	DF_FILLED = 2,
	DF_SAVE_BG = 4,
	DF_DIALOG = (4+2+1),
	DF_MODAL = 8		// modal = accept all user input exclusively
};

template <typename T>
using ui_subfunction_t = window_event_result (*)(struct UI_DIALOG *,const d_event &, T *);

struct UI_DIALOG : window
{
	// TODO: Make these private
	UI_GADGET *gadget = nullptr;
	UI_GADGET *keyboard_focus_gadget = nullptr;
	short           d_width, d_height;
	enum dialog_flags d_flags;

public:
	explicit UI_DIALOG(short x, short y, short w, short h, enum dialog_flags flags);

	~UI_DIALOG();
	virtual window_event_result event_handler(const d_event &) override;
	virtual window_event_result callback_handler(const d_event &) = 0;
};

#define B1_JUST_PRESSED     (event.type == EVENT_MOUSE_BUTTON_DOWN && event_mouse_get_button(event) == 0)
#define B1_JUST_RELEASED    (event.type == EVENT_MOUSE_BUTTON_UP && event_mouse_get_button(event) == 0)
#define B1_DOUBLE_CLICKED   (event.type == EVENT_MOUSE_DOUBLE_CLICKED && event_mouse_get_button(event) == 0)

extern grs_font_ptr ui_small_font;

extern unsigned char CBLACK,CGREY,CWHITE,CBRIGHT,CRED;
extern UI_GADGET * selected_gadget;

#define Hline(C,x1,x2,y,c)	Hline(C,x1,y,x2,c)
#define Vline(C,y1,y2,x,c)	Vline(C,x,y1,y2,c)
void Hline(grs_canvas &, fix x1, fix x2, fix y, color_palette_index color);
void Vline(grs_canvas &, fix y1, fix y2, fix x, color_palette_index color);
void ui_string_centered(grs_canvas &, unsigned x, unsigned y, const char *s);
void ui_draw_box_out(grs_canvas &, unsigned x1, unsigned y1, unsigned x2, unsigned y2);
void ui_draw_box_in(grs_canvas &, unsigned x1, unsigned y1, unsigned x2, unsigned y2);
void ui_draw_shad(grs_canvas &, unsigned x1, unsigned y1, unsigned x2, unsigned y2, color_palette_index c1, color_palette_index c2);

int ui_init();
void ui_close();

typedef cstring_tie<10> ui_messagebox_tie;
int ui_messagebox( short xc, short yc, const char * text, const ui_messagebox_tie &Button );
#define ui_messagebox(X,Y,N,T,...)	((ui_messagebox)((X),(Y),(T), ui_messagebox_tie(__VA_ARGS__)))

class unused_ui_userdata_t;
constexpr unused_ui_userdata_t *unused_ui_userdata = nullptr;

void ui_dialog_set_current_canvas(UI_DIALOG &dlg);
void ui_close_dialog(UI_DIALOG &dlg);

#define GADGET_PRESSED(g) (event.type == EVENT_UI_GADGET_PRESSED && &ui_event_get_gadget(event) == g)

void ui_gadget_add(UI_DIALOG &dlg, short x1, short y1, short x2, short y2, UI_GADGET &);
template <typename T>
__attribute_warn_unused_result
static std::unique_ptr<T> ui_gadget_add(UI_DIALOG &dlg, short x1, short y1, short x2, short y2)
{
	auto t = std::make_unique<T>();
	t->kind = T::s_kind;
	ui_gadget_add(dlg, x1, y1, x2, y2, *t);
	return t;
}
__attribute_warn_unused_result
std::unique_ptr<UI_GADGET_BUTTON> ui_add_gadget_button(UI_DIALOG &dlg, short x, short y, short w, short h, const char *text, int (*function_to_call)());
window_event_result ui_gadget_send_event(UI_DIALOG &dlg, enum event_type type, UI_GADGET &gadget);
UI_GADGET &ui_event_get_gadget(const d_event &event);
window_event_result ui_dialog_do_gadgets(UI_DIALOG &dlg, const d_event &event);

int ui_mouse_on_gadget(UI_GADGET &gadget);

std::unique_ptr<UI_GADGET_LISTBOX> ui_add_gadget_listbox(UI_DIALOG &dlg, short x, short y, short w, short h, short numitems, char **list);

extern void ui_mega_process();

void ui_get_button_size(const grs_font &, const char *text, int &width, int &height);

__attribute_warn_unused_result
std::unique_ptr<UI_GADGET_SCROLLBAR> ui_add_gadget_scrollbar(UI_DIALOG &dlg, short x, short y, short w, short h, int start, int stop, int position, int window_size);


void ui_dputs_at( UI_DIALOG * dlg, short x, short y, const char * str );
extern void ui_dprintf_at( UI_DIALOG * dlg, short x, short y, const char * format, ... ) __attribute_format_printf(4, 5);
#define ui_dprintf_at(A1,A2,A3,F,...)	dxx_call_printf_checked(ui_dprintf_at,ui_dputs_at,(A1,A2,A3),(F),##__VA_ARGS__)

__attribute_warn_unused_result
std::unique_ptr<UI_GADGET_RADIO> ui_add_gadget_radio(UI_DIALOG &dlg, short x, short y, short w, short h, short group, const char *text);
void ui_radio_set_value(UI_GADGET_RADIO &radio, int value);

__attribute_warn_unused_result
std::unique_ptr<UI_GADGET_CHECKBOX> ui_add_gadget_checkbox(UI_DIALOG &dlg, short x, short y, short w, short h, short group, const char *text);
extern void ui_checkbox_check(UI_GADGET_CHECKBOX * checkbox, int check);

UI_GADGET &ui_gadget_get_next(UI_GADGET &gadget);
void ui_gadget_calc_keys(UI_DIALOG &dlg);

void ui_listbox_change(UI_DIALOG *dlg, UI_GADGET_LISTBOX *listbox, uint_fast32_t numitems, const char *const *list);

__attribute_warn_unused_result
std::unique_ptr<UI_GADGET_INPUTBOX> ui_add_gadget_inputbox(UI_DIALOG &dlg, short x, short y, uint_fast32_t length_of_initial_text, uint_fast32_t maximum_allowed_text_length, const char *text);

template <std::size_t SL, std::size_t L>
__attribute_warn_unused_result
static inline std::unique_ptr<UI_GADGET_INPUTBOX> ui_add_gadget_inputbox(UI_DIALOG &dlg, short x, short y, const char (&text)[L])
{
	static_assert(SL <= L, "SL too large");
	return ui_add_gadget_inputbox(dlg, x, y, L, SL, text);
}

extern void ui_inputbox_set_text(UI_GADGET_INPUTBOX *inputbox, const char *text);

__attribute_warn_unused_result
std::unique_ptr<UI_GADGET_USERBOX> ui_add_gadget_userbox(UI_DIALOG &dlg, short x, short y, short w, short h);

int MenuX( int x, int y, int NumButtons, const char *const text[] );

#if DXX_USE_EDITOR
int ui_get_filename(std::array<char, PATH_MAX> &filename, const char *filespec, const char *message);
#endif


void * ui_malloc( int size );
void ui_free( void * buffer );

#define UI_RECORD_MOUSE     1
#define UI_RECORD_KEYS      2
#define UI_STATUS_NORMAL    0
#define UI_STATUS_RECORDING 1
#define UI_STATUS_PLAYING   2
#define UI_STATUS_FASTPLAY  3

int ui_get_file( char * filename, const char * Filespec  );

__attribute_warn_unused_result
std::unique_ptr<UI_GADGET_ICON> ui_add_gadget_icon(UI_DIALOG &dlg, const char *text, short x, short y, short w, short h, int k,int (*f)());

int DecodeKeyText( const char * text );
void GetKeyDescription(char (&text)[100], uint_fast32_t keypress);

int menubar_init(grs_canvas &canvas, const char * filename );
extern void menubar_close();
extern void menubar_hide();
extern void menubar_show();

void ui_pad_init();
void ui_pad_close();
void ui_pad_activate(UI_DIALOG &dlg, uint_fast32_t x, uint_fast32_t y);
void ui_pad_deactivate();
void ui_pad_goto(int n);
void ui_pad_goto_next();
void ui_pad_goto_prev();
int ui_pad_read( int n, const char * filename );
int ui_pad_get_current();
void ui_pad_draw(UI_DIALOG *dlg, int x, int y);

void ui_barbox_open( const char * text, int length );
void ui_barbox_update( int position );
void ui_barbox_close();

extern int ui_button_any_drawn;

}

#endif
