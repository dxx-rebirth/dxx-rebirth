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
 * Routines for menus.
 *
 */

#pragma once

#include "fwd-event.h"

#ifdef __cplusplus
#include <cstdint>
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include "fwd-window.h"
#include "varutil.h"
#include "dxxsconf.h"
#include "dsx-ns.h"
#include "fmtcheck.h"
#include "ntstring.h"

struct newmenu;
struct listbox;

enum nm_type : uint8_t
{
	NM_TYPE_MENU = 0,   // A menu item... when enter is hit on this, newmenu_do returns this item number
	NM_TYPE_INPUT = 1,   // An input box... fills the text field in, and you need to fill in text_len field.
	NM_TYPE_CHECK = 2,   // A check box. Set and get its status by looking at flags field (1=on, 0=off)
	NM_TYPE_RADIO = 3,   // Same as check box, but only 1 in a group can be set at a time. Set group fields.
	NM_TYPE_TEXT = 4,   // A line of text that does nothing.
	NM_TYPE_NUMBER = 5,   // A numeric entry counter.  Changes value from min_value to max_value;
	NM_TYPE_INPUT_MENU = 6,   // A inputbox that you hit Enter to edit, when done, hit enter and menu leaves.
	NM_TYPE_SLIDER = 7,   // A slider from min_value to max_value. Draws with text_len chars.
};

#define NM_MAX_TEXT_LEN     255

class newmenu_item
{
	struct input_common_type
	{
		int text_len;
		/* Only used by imenu, but placing it in imenu_specific_type
		 * makes newmenu_item non-POD.  Some users expect newmenu_item
		 * to be POD.  Placing group here does not increase overall size
		 * since number_slider_common_type also has two int members.
		 */
		int group;
	};
	struct input_specific_type : input_common_type
	{
		static constexpr std::integral_constant<unsigned, NM_TYPE_INPUT> nm_type{};
	};
	struct radio_specific_type
	{
		static constexpr std::integral_constant<unsigned, NM_TYPE_RADIO> nm_type{};
		int group;          // What group this belongs to for radio buttons.
	};
	struct number_slider_common_type
	{
		int min_value;
		int max_value;
	};
	struct number_specific_type : number_slider_common_type
	{
		static constexpr std::integral_constant<unsigned, NM_TYPE_NUMBER> nm_type{};
	};
	struct imenu_specific_type : input_common_type
	{
		static constexpr std::integral_constant<unsigned, NM_TYPE_INPUT_MENU> nm_type{};
	};
	struct slider_specific_type : number_slider_common_type
	{
		static constexpr std::integral_constant<unsigned, NM_TYPE_SLIDER> nm_type{};
	};
	template <typename T, unsigned expected_type = T::nm_type>
		T &get_union_member(T &v)
		{
#ifdef DXX_CONSTANT_TRUE
			if (DXX_CONSTANT_TRUE(type != expected_type))
				DXX_ALWAYS_ERROR_FUNCTION(dxx_newmenu_trap_invalid_type, "invalid type access");
#endif
			if (type != expected_type)
				throw std::runtime_error("invalid type access");
			return v;
		}
public:
	int     value;          // For checkboxes and radio buttons, this is 1 if marked initially, else 0
	union {
		input_specific_type nm_private_input;
		radio_specific_type nm_private_radio;
		number_specific_type nm_private_number;
		imenu_specific_type nm_private_imenu;
		slider_specific_type nm_private_slider;
	};
	input_specific_type &input() {
		return get_union_member(nm_private_input);
	}
	radio_specific_type &radio() {
		return get_union_member(nm_private_radio);
	}
	number_specific_type &number() {
		return get_union_member(nm_private_number);
	}
	imenu_specific_type &imenu() {
		return get_union_member(nm_private_imenu);
	}
	slider_specific_type &slider() {
		return get_union_member(nm_private_slider);
	}
	number_slider_common_type *number_or_slider() {
		return (type == nm_private_number.nm_type || type == nm_private_slider.nm_type)
			? &nm_private_number
			: nullptr;
	}
	input_common_type *input_or_menu() {
		return (type == nm_private_input.nm_type || type == nm_private_imenu.nm_type)
			? &nm_private_input
			: nullptr;
	}
	char    *text;          // The text associated with this item.
	// The rest of these are used internally by by the menu system, so don't set 'em!!
	short   x, y;
	short   w, h;
	short   right_offset;
	nm_type type;           // What kind of item this is, see NM_TYPE_????? defines
	ntstring<NM_MAX_TEXT_LEN> saved_text;
};

namespace dcx {
template <typename T>
using newmenu_subfunction_t = int(*)(newmenu *menu,const d_event &event, T *userdata);
using newmenu_subfunction = newmenu_subfunction_t<void>;

class unused_newmenu_userdata_t;
constexpr newmenu_subfunction_t<const unused_newmenu_userdata_t> unused_newmenu_subfunction = nullptr;
constexpr const unused_newmenu_userdata_t *unused_newmenu_userdata = nullptr;
}

int newmenu_do2(const char *title, const char *subtitle, uint_fast32_t nitems, newmenu_item *item, newmenu_subfunction subfunction, void *userdata, int citem, const char *filename);

// Pass an array of newmenu_items and it processes the menu. It will
// return a -1 if Esc is pressed, otherwise, it returns the index of
// the item that was current when Enter was was selected.
// The subfunction function accepts standard events, plus additional
// NEWMENU events in future.  Just pass NULL if you don't want this,
// or return 0 where you don't want to override the default behaviour.
// Title draws big, Subtitle draw medium sized.  You can pass NULL for
// either/both of these if you don't want them.
// Same as above, only you can pass through what background bitmap to use.
template <typename T>
int newmenu_do2(const char *const title, const char *const subtitle, const uint_fast32_t nitems, newmenu_item *const item, const newmenu_subfunction_t<T> subfunction, T *const userdata, const int citem, const char *const filename)
{
	return newmenu_do2(title, subtitle, nitems, item, reinterpret_cast<newmenu_subfunction>(subfunction), static_cast<void *>(userdata), citem, filename);
}

template <typename T>
int newmenu_do2(const char *const title, const char *const subtitle, const uint_fast32_t nitems, newmenu_item *const item, const newmenu_subfunction_t<const T> subfunction, const T *const userdata, const int citem, const char *const filename)
{
	return newmenu_do2(title, subtitle, nitems, item, reinterpret_cast<newmenu_subfunction>(subfunction), static_cast<void *>(const_cast<T *>(userdata)), citem, filename);
}

template <typename T>
static inline int newmenu_do(const char *const title, const char *const subtitle, const uint_fast32_t nitems, newmenu_item *const item, const newmenu_subfunction_t<T> subfunction, T *const userdata)
{
	return newmenu_do2( title, subtitle, nitems, item, subfunction, userdata, 0, NULL );
}

template <std::size_t N, typename T>
static inline int newmenu_do(const char *const title, const char *const subtitle, array<newmenu_item, N> &items, const newmenu_subfunction_t<T> subfunction, T *const userdata)
{
	return newmenu_do(title, subtitle, items.size(), &items.front(), subfunction, userdata);
}

// Same as above, only you can pass through what item is initially selected.
template <typename T>
static inline int newmenu_do1(const char *const title, const char *const subtitle, const uint_fast32_t nitems, newmenu_item *const item, const newmenu_subfunction_t<T> subfunction, T *const userdata, const int citem)
{
	return newmenu_do2( title, subtitle, nitems, item, subfunction, userdata, citem, NULL );
}

#ifdef dsx
namespace dsx {
newmenu *newmenu_do4( const char * title, const char * subtitle, uint_fast32_t nitems, newmenu_item * item, newmenu_subfunction subfunction, void *userdata, int citem, const char * filename, int TinyMode, int TabsFlag );

static inline newmenu *newmenu_do3( const char * title, const char * subtitle, uint_fast32_t nitems, newmenu_item * item, newmenu_subfunction subfunction, void *userdata, int citem, const char * filename )
{
	return newmenu_do4( title, subtitle, nitems, item, subfunction, userdata, citem, filename, 0, 0 );
}

// Same as above, but returns menu instead of citem
template <typename T>
static newmenu *newmenu_do3(const char *const title, const char *const subtitle, const uint_fast32_t nitems, newmenu_item *const item, const newmenu_subfunction_t<T> subfunction, T *const userdata, const int citem, const char *const filename)
{
	return newmenu_do3(title, subtitle, nitems, item, reinterpret_cast<newmenu_subfunction>(subfunction), static_cast<void *>(userdata), citem, filename);
}

template <typename T>
static newmenu *newmenu_do3(const char *const title, const char *const subtitle, const uint_fast32_t nitems, newmenu_item *const item, const newmenu_subfunction_t<const T> subfunction, const T *const userdata, const int citem, const char *const filename)
{
	return newmenu_do3(title, subtitle, nitems, item, reinterpret_cast<newmenu_subfunction>(subfunction), static_cast<void *>(const_cast<T *>(userdata)), citem, filename);
}

static inline newmenu *newmenu_dotiny( const char * title, const char * subtitle, uint_fast32_t nitems, newmenu_item * item, int TabsFlag, newmenu_subfunction subfunction, void *userdata )
{
	return newmenu_do4( title, subtitle, nitems, item, subfunction, userdata, 0, NULL, 1, TabsFlag );
}

// Tiny menu with GAME_FONT
template <typename T>
static newmenu *newmenu_dotiny(const char *const title, const char *const subtitle, const uint_fast32_t nitems, newmenu_item *const item, const int TabsFlag, const newmenu_subfunction_t<T> subfunction, T *const userdata)
{
	return newmenu_dotiny(title, subtitle, nitems, item, TabsFlag, reinterpret_cast<newmenu_subfunction>(subfunction), static_cast<void *>(userdata));
}
}
#endif

// Basically the same as do2 but sets reorderitems flag for weapon priority menu a bit redundant to get lose of a global variable but oh well...
void newmenu_doreorder(const char * title, const char * subtitle, uint_fast32_t nitems, newmenu_item *item);

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
//int nm_messagebox(const char *title, int nchoices, ...);
#define nm_messagebox(T,N,...)	nm_messagebox_a##N((T), ##__VA_ARGS__)
#define nm_messagebox_a1(T,A1				,F,...)	vnm_messagebox_aN(T,nm_messagebox_tie(A1			),F,##__VA_ARGS__)
#define nm_messagebox_a2(T,A1,A2			,F,...)	vnm_messagebox_aN(T,nm_messagebox_tie(A1,A2			),F,##__VA_ARGS__)
#define nm_messagebox_a3(T,A1,A2,A3			,F,...)	vnm_messagebox_aN(T,nm_messagebox_tie(A1,A2,A3		),F,##__VA_ARGS__)
#define nm_messagebox_a4(T,A1,A2,A3,A4		,F,...)	vnm_messagebox_aN(T,nm_messagebox_tie(A1,A2,A3,A4	),F,##__VA_ARGS__)
#define nm_messagebox_a5(T,A1,A2,A3,A4,A5	,F,...)	vnm_messagebox_aN(T,nm_messagebox_tie(A1,A2,A3,A4,A5),F,##__VA_ARGS__)

typedef cstring_tie<5> nm_messagebox_tie;

int nm_messagebox_str(const char *title, const nm_messagebox_tie &tie, const char *str) __attribute_nonnull((3));
int vnm_messagebox_aN(const char *title, const nm_messagebox_tie &tie, const char *format, ...) __attribute_format_printf(3, 4);
#define vnm_messagebox_aN(A1,A2,F,...)	dxx_call_printf_checked(vnm_messagebox_aN,nm_messagebox_str,(A1,A2),(F),##__VA_ARGS__)

newmenu_item *newmenu_get_items(newmenu *menu);
int newmenu_get_nitems(newmenu *menu);
int newmenu_get_citem(newmenu *menu);
void nm_draw_background(grs_canvas &, int x1, int y1, int x2, int y2);
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

window *listbox_get_window(listbox *lb);
extern const char **listbox_get_items(listbox *lb);
extern int listbox_get_citem(listbox *lb);
extern void listbox_delete_item(listbox *lb, int item);

namespace dcx {
template <typename T>
using listbox_subfunction_t = window_event_result (*)(listbox *menu,const d_event &event, T *userdata);

class unused_listbox_userdata_t;
constexpr listbox_subfunction_t<const unused_listbox_userdata_t> *unused_listbox_subfunction = nullptr;
constexpr const unused_listbox_userdata_t *unused_listbox_userdata = nullptr;
}

listbox *newmenu_listbox1(const char * title, uint_fast32_t nitems, const char *items[], int allow_abort_flag, int default_item, listbox_subfunction_t<void> listbox_callback, void *userdata);

template <typename T>
listbox *newmenu_listbox1(const char *const title, const uint_fast32_t nitems, const char *items[], const int allow_abort_flag, const int default_item, const listbox_subfunction_t<T> listbox_callback, T *const userdata)
{
	return newmenu_listbox1(title, nitems, items, allow_abort_flag, default_item, reinterpret_cast<listbox_subfunction_t<void>>(listbox_callback), static_cast<void *>(userdata));
}

template <typename T>
listbox *newmenu_listbox1(const char *const title, const uint_fast32_t nitems, const char *items[], const int allow_abort_flag, const int default_item, const listbox_subfunction_t<T> listbox_callback, std::unique_ptr<T> userdata)
{
	auto r = newmenu_listbox1(title, nitems, items, allow_abort_flag, default_item, reinterpret_cast<listbox_subfunction_t<void>>(listbox_callback), static_cast<void *>(userdata.get()));
	userdata.release();
	return r;
}

template <typename T>
listbox *newmenu_listbox(const char *const title, const uint_fast32_t nitems, const char *items[], const int allow_abort_flag, const listbox_subfunction_t<T> listbox_callback, T *const userdata)
{
	return newmenu_listbox1(title, nitems, items, allow_abort_flag, 0, reinterpret_cast<listbox_subfunction_t<void>>(listbox_callback), static_cast<void *>(userdata));
}

//should be called whenever the palette changes
extern void newmenu_free_background();

static inline void nm_set_item_menu(newmenu_item &ni, const char *text)
{
	ni.type = NM_TYPE_MENU;
	ni.text = const_cast<char *>(text);
}

__attribute_nonnull()
__attribute_warn_unused_result
static inline newmenu_item nm_item_menu(const char *text)
{
	newmenu_item i;
	return nm_set_item_menu(i, text), i;
}

__attribute_nonnull()
static inline void nm_set_item_input(newmenu_item &ni, unsigned len, char *text)
{
	ni.type = NM_TYPE_INPUT;
	ni.text = text;
	ni.input().text_len = len - 1;
}

template <std::size_t len>
static inline void nm_set_item_input(newmenu_item &ni, char (&text)[len])
{
	nm_set_item_input(ni, len, text);
}

template <std::size_t len>
static inline void nm_set_item_input(newmenu_item &ni, array<char, len> &text)
{
	nm_set_item_input(ni, len, text.data());
}

template <typename... T>
__attribute_warn_unused_result
static inline newmenu_item nm_item_input(T &&... t)
{
	newmenu_item i;
	return nm_set_item_input(i, std::forward<T>(t)...), i;
}

__attribute_nonnull()
static inline void nm_set_item_checkbox(newmenu_item &ni, const char *text, unsigned checked)
{
	ni.type = NM_TYPE_CHECK;
	ni.text = const_cast<char *>(text);
	ni.value = checked;
}

__attribute_nonnull()
static inline void nm_set_item_text(newmenu_item &ni, const char *text)
{
	ni.type = NM_TYPE_TEXT;
	ni.text = const_cast<char *>(text);
}

__attribute_nonnull()
__attribute_warn_unused_result
static inline newmenu_item nm_item_text(const char *text)
{
	newmenu_item i;
	return nm_set_item_text(i, text), i;
}

__attribute_nonnull()
static inline void nm_set_item_radio(newmenu_item &ni, const char *text, unsigned checked, unsigned grp)
{
	ni.type = NM_TYPE_RADIO;
	ni.text = const_cast<char *>(text);
	ni.value = checked;
	auto &radio = ni.radio();
	radio.group = grp;
}

__attribute_nonnull()
static inline void nm_set_item_number(newmenu_item &ni, const char *text, unsigned now, unsigned low, unsigned high)
{
	ni.type = NM_TYPE_NUMBER;
	ni.text = const_cast<char *>(text);
	ni.value = now;
	auto &number = ni.number();
	number.min_value = low;
	number.max_value = high;
}

__attribute_nonnull()
static inline void nm_set_item_slider(newmenu_item &ni, const char *text, unsigned now, unsigned low, unsigned high)
{
	ni.type = NM_TYPE_SLIDER;
	ni.text = const_cast<char *>(text);
	ni.value = now;
	auto &slider = ni.slider();
	slider.min_value = low;
	slider.max_value = high;
}

#define NORMAL_CHECK_BOX    "\201"
#define CHECKED_CHECK_BOX   "\202"

#define NORMAL_RADIO_BOX    "\177"
#define CHECKED_RADIO_BOX   "\200"
#define CURSOR_STRING       "_"
#define SLIDER_LEFT         "\203"  // 131
#define SLIDER_RIGHT        "\204"  // 132
#define SLIDER_MIDDLE       "\205"  // 133
#define SLIDER_MARKER       "\206"  // 134

#define BORDERX (15*(SWIDTH/320))
#define BORDERY (15*(SHEIGHT/200))

#define DXX_NEWMENU_VARIABLE	m
#define DXX_MENUITEM(VERB, TYPE, ...)	DXX_MENUITEM_V_##VERB(TYPE, ## __VA_ARGS__)
#define DXX_MENUITEM_V_ENUM(TYPE,S,OPT,...)	OPT,
#define DXX_MENUITEM_V_COUNT(TYPE,...)	+1
#define DXX_MENUITEM_V_ADD(TYPE,S,OPT,...)	DXX_MENUITEM_V_ADD_T_##TYPE(S, OPT, ## __VA_ARGS__)
#define DXX_MENUITEM_V_READ(TYPE,S,OPT,...)	DXX_MENUITEM_V_READ_T_##TYPE(S, OPT, ## __VA_ARGS__)
#define DXX_MENUITEM_V_ADD_T_CHECK(S,OPT,V)	\
	nm_set_item_checkbox(((DXX_NEWMENU_VARIABLE)[(OPT)]), (S), (V));
#define DXX_MENUITEM_V_ADD_T_FCHECK(S,OPT,V,F)	DXX_MENUITEM_V_ADD_T_CHECK(S,OPT,(V) & (F))
#define DXX_MENUITEM_V_ADD_T_RADIO(S,OPT,C,G)	\
	nm_set_item_radio(((DXX_NEWMENU_VARIABLE)[(OPT)]), (S), (C), (G));
#define DXX_MENUITEM_V_ADD_T_NUMBER(S,OPT,V,MIN,MAX)	\
	nm_set_item_number(((DXX_NEWMENU_VARIABLE)[(OPT)]), (S), (V), (MIN), (MAX));
#define DXX_MENUITEM_V_ADD_T_SLIDER(S,OPT,V,MIN,MAX)	\
	nm_set_item_slider(((DXX_NEWMENU_VARIABLE)[(OPT)]), (S), (V), (MIN), (MAX));
#define DXX_MENUITEM_V_ADD_T_SCALE_SLIDER(S,OPT,V,MIN,MAX,SCALE)	\
	DXX_MENUITEM_V_ADD_T_SLIDER((S),(OPT),(V) / (SCALE),(MIN),(MAX))
#define DXX_MENUITEM_V_ADD_T_MENU(S,OPT)	\
	nm_set_item_menu(((DXX_NEWMENU_VARIABLE)[(OPT)]), (S));
#define DXX_MENUITEM_V_ADD_T_TEXT(S,OPT)	\
	nm_set_item_text(((DXX_NEWMENU_VARIABLE)[(OPT)]), (S));
#define DXX_MENUITEM_V_ADD_T_INPUT(S,OPT)	\
	nm_set_item_input(((DXX_NEWMENU_VARIABLE)[(OPT)]),(S));
#define DXX_MENUITEM_V_READ_T_CHECK(S,OPT,V)	\
	(V) = (DXX_NEWMENU_VARIABLE)[(OPT)].value;
#define DXX_MENUITEM_V_READ_T_FCHECK(S,OPT,V,F)	\
	( DXX_BEGIN_COMPOUND_STATEMENT {	\
		auto &dxx_menuitem_read_fcheck_v = (V);	\
		const auto dxx_menuitem_read_fcheck_f = (F);	\
		if ((DXX_NEWMENU_VARIABLE)[(OPT)].value)	\
			dxx_menuitem_read_fcheck_v |= dxx_menuitem_read_fcheck_f;	\
		else	\
			dxx_menuitem_read_fcheck_v &= ~dxx_menuitem_read_fcheck_f;	\
	} DXX_END_COMPOUND_STATEMENT );
#define DXX_MENUITEM_V_READ_T_RADIO(S,OPT,C,G)	/* handled specially */
#define DXX_MENUITEM_V_READ_T_NUMBER(S,OPT,V,MIN,MAX)	\
	(V) = (DXX_NEWMENU_VARIABLE)[(OPT)].value;
#define DXX_MENUITEM_V_READ_T_SLIDER(S,OPT,V,MIN,MAX)	\
	(V) = (DXX_NEWMENU_VARIABLE)[(OPT)].value;
#define DXX_MENUITEM_V_READ_T_SCALE_SLIDER(S,OPT,V,MIN,MAX,SCALE)	\
	(V) = (DXX_NEWMENU_VARIABLE)[(OPT)].value * (SCALE);
#define DXX_MENUITEM_V_READ_T_MENU(S,OPT)	/* handled specially */
#define DXX_MENUITEM_V_READ_T_TEXT(S,OPT)	/* handled specially */
#define DXX_MENUITEM_V_READ_T_INPUT(S,OPT)	/* handled specially */

template <typename T, typename B>
class menu_bit_wrapper_t
{
	using M = decltype(std::declval<const T &>() & std::declval<B>());
	std::tuple<T &, B> m_data;
	enum
	{
		m_mask = 0,
		m_bit = 1,
	};
	T &get_mask()
	{
		return std::get<m_mask>(m_data);
	}
	const T &get_mask() const
	{
		return std::get<m_mask>(m_data);
	}
	B get_bit() const
	{
		return std::get<m_bit>(m_data);
	}
public:
	constexpr menu_bit_wrapper_t(T &t, B bit) :
		m_data(t, bit)
	{
	}
	constexpr operator M() const
	{
		return get_mask() & get_bit();
	}
	menu_bit_wrapper_t &operator=(const bool n)
	{
		auto &m = get_mask();
		const auto b = get_bit();
		if (n)
			m |= b;
		else
			m &= ~b;
		return *this;
	}
};

template <typename T, typename B>
static constexpr menu_bit_wrapper_t<T, B> menu_bit_wrapper(T &t, B b)
{
	return {t, b};
}

template <unsigned B, typename T>
class menu_number_bias_wrapper_t
{
	std::tuple<T &, std::integral_constant<unsigned, B>> m_data;
#define m_value	std::get<0>(m_data)
#define m_bias	std::get<1>(m_data)
public:
	constexpr menu_number_bias_wrapper_t(T &t) :
		m_data(t, {})
	{
	}
	constexpr operator T() const
	{
		return m_value + m_bias;
	}
	menu_number_bias_wrapper_t &operator=(const T n)
	{
		m_value = n - m_bias;
		return *this;
	}
#undef m_bias
#undef m_value
};

template <unsigned B, typename T>
static constexpr menu_number_bias_wrapper_t<B, T> menu_number_bias_wrapper(T &t)
{
	return t;
}

#endif
