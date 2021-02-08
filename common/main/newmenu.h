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

#include <cstdint>
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include "varutil.h"
#include "dxxsconf.h"
#include "dsx-ns.h"
#include "fmtcheck.h"
#include "ntstring.h"
#include "partial_range.h"
#ifdef dsx
#include "gamefont.h"
#include "window.h"

namespace dcx {

struct listbox;

enum class nm_type : uint8_t
{
	menu = 0,   // A menu item... when enter is hit on this, newmenu_do returns this item number
	input = 1,   // An input box... fills the text field in, and you need to fill in text_len field.
	check = 2,   // A check box. Set and get its status by looking at flags field (1=on, 0=off)
	radio = 3,   // Same as check box, but only 1 in a group can be set at a time. Set group fields.
	text = 4,   // A line of text that does nothing.
	number = 5,   // A numeric entry counter.  Changes value from min_value to max_value;
	input_menu = 6,   // A inputbox that you hit Enter to edit, when done, hit enter and menu leaves.
	slider = 7,   // A slider from min_value to max_value. Draws with text_len chars.
};

#define NM_MAX_TEXT_LEN     255

class newmenu_item
{
	struct input_common_type
	{
		const char *allowed_chars;
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
		static constexpr std::integral_constant<nm_type, nm_type::input> static_type{};
	};
	struct radio_specific_type
	{
		static constexpr std::integral_constant<nm_type, nm_type::radio> static_type{};
		int group;          // What group this belongs to for radio buttons.
	};
	struct number_slider_common_type
	{
		int min_value;
		int max_value;
	};
	struct number_specific_type : number_slider_common_type
	{
		static constexpr std::integral_constant<nm_type, nm_type::number> static_type{};
	};
	struct imenu_specific_type : input_common_type
	{
		static constexpr std::integral_constant<nm_type, nm_type::input_menu> static_type{};
		ntstring<NM_MAX_TEXT_LEN> saved_text;
	};
	struct slider_specific_type : number_slider_common_type
	{
		static constexpr std::integral_constant<nm_type, nm_type::slider> static_type{};
		ntstring<NM_MAX_TEXT_LEN> saved_text;
	};
	static void check_union_type(const nm_type current_type, const nm_type static_type)
	{
#ifdef DXX_CONSTANT_TRUE
		if (DXX_CONSTANT_TRUE(current_type != static_type))
			DXX_ALWAYS_ERROR_FUNCTION(dxx_newmenu_trap_invalid_type, "invalid type access");
#endif
		if (current_type != static_type)
			throw std::runtime_error("invalid type access");
	}
	template <typename T, nm_type static_type = T::static_type>
		T &get_union_member(T &v) const
		{
			check_union_type(type, static_type);
			return v;
		}
public:
	input_specific_type &input() {
		return get_union_member(nm_private_input);
	}
	const radio_specific_type &radio() const
	{
		return get_union_member(nm_private_radio);
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
		return (type == nm_private_number.static_type || type == nm_private_slider.static_type)
			? &nm_private_number
			: nullptr;
	}
	input_common_type *input_or_menu() {
		return (type == nm_private_input.static_type || type == nm_private_imenu.static_type)
			? &nm_private_input
			: nullptr;
	}
	char    *text;          // The text associated with this item.
	int     value;          // For checkboxes and radio buttons, this is 1 if marked initially, else 0
	// The rest of these are used internally by by the menu system, so don't set 'em!!
	short   x, y;
	short   w, h;
	uint8_t right_offset;
	nm_type type;           // What kind of item this is, see NM_TYPE_????? defines
	union {
		input_specific_type nm_private_input;
		radio_specific_type nm_private_radio;
		number_specific_type nm_private_number;
		imenu_specific_type nm_private_imenu = {};
		slider_specific_type nm_private_slider;
	};
};

enum class tab_processing_flag : uint8_t
{
	ignore,
	process,
};

enum class tiny_mode_flag : uint8_t
{
	normal,
	tiny,
};

enum class draw_box_flag : uint8_t
{
	none,
	menu_background,
};

template <typename>
struct menu_tagged_string
{
	const char *const p;
	operator const char *() const
	{
		return p;
	}
};

struct menu_title_tag;
struct menu_subtitle_tag;
struct menu_filename_tag;
using menu_title = menu_tagged_string<menu_title_tag>;
using menu_subtitle = menu_tagged_string<menu_subtitle_tag>;
using menu_filename = menu_tagged_string<menu_filename_tag>;

struct newmenu_layout
{
	struct adjusted_citem
	{
		const partial_range_t<newmenu_item *> items;
		const int citem;
		const uint8_t all_text;
		static adjusted_citem create(partial_range_t<newmenu_item *> items, int citem);
	};
	int             x,y,w,h;
	short			swidth, sheight;
	// with these we check if resolution or fonts have changed so menu structure can be recreated
	font_x_scale_proportion fntscalex;
	font_y_scale_proportion fntscaley;
	int				citem;
	const menu_title title;
	const menu_subtitle subtitle;
	const menu_filename filename;
	const tiny_mode_flag tiny_mode;
	const tab_processing_flag tabs_flag;
	const uint8_t max_on_menu;
	const uint8_t all_text;		//set true if all text items
	const uint8_t is_scroll_box;   // Is this a scrolling box? Set to false at init
	const uint8_t max_displayable;
	const draw_box_flag draw_box;
	uint8_t mouse_state;
	const partial_range_t<newmenu_item *> items;
	int	scroll_offset = 0;
	newmenu_layout(const menu_title title, const menu_subtitle subtitle, const menu_filename filename, const tiny_mode_flag tiny_mode, const tab_processing_flag tabs_flag, const adjusted_citem citem_init, const draw_box_flag draw_box) :
		citem(citem_init.citem),
		title(title), subtitle(subtitle), filename(filename),
		tiny_mode(tiny_mode), tabs_flag(tabs_flag),
		max_on_menu(std::min<uint8_t>(citem_init.items.size(), tiny_mode != tiny_mode_flag::normal ? 21u : 14u)),
		all_text(citem_init.all_text),
		is_scroll_box(max_on_menu < citem_init.items.size()),
		max_displayable(std::min<uint8_t>(max_on_menu, citem_init.items.size())),
		draw_box(draw_box),
		items(citem_init.items)
	{
		create_structure();
	}
	newmenu_layout(newmenu_layout &&) = default;
	/* gcc can implement this as requested.  clang implicitly deletes
	 * the move-assignment operator= due to the presence of
	 * const-qualified member variables.
	 */
	newmenu_layout &operator=(newmenu_layout &&) = delete;
	void create_structure();
};

struct newmenu : newmenu_layout, window
{
	using subfunction_type = int(*)(newmenu *menu, const d_event &event, void *userdata);
	newmenu(const menu_title title, const menu_subtitle subtitle, const menu_filename filename, const tiny_mode_flag tiny_mode, const tab_processing_flag tabs_flag, const adjusted_citem citem_init, grs_canvas &src, const draw_box_flag draw_box = draw_box_flag::menu_background) :
		newmenu_layout(title, subtitle, filename, tiny_mode, tabs_flag, citem_init, draw_box), window(src, x, y, w, h)
	{
	}
	int *rval = nullptr;			// Pointer to return value (for polling newmenus)
	virtual window_event_result event_handler(const d_event &) override;
	virtual int subfunction_handler(const d_event &event) = 0;
	static int process_until_closed(newmenu *);
};

template <typename T1, typename... ConstructionArgs>
int run_blocking_newmenu(ConstructionArgs &&... args)
{
	return T1::process_until_closed(window_create<T1>(std::forward<ConstructionArgs>(args)...));
}

struct passive_newmenu : newmenu
{
	using newmenu::newmenu;
	/* Ignores all calls. */
	virtual int subfunction_handler(const d_event &event) final override;
};

struct reorder_newmenu : newmenu
{
	using newmenu::newmenu;
	void event_key_command(const d_event &event);
	/* subfunction_handler is not overridden here, because users of
	 * reorder_newmenu will need to handle more than just the key event,
	 * so requiring them to have a handler for the key event produces
	 * better code than having them call back to
	 * reorder_newmenu::subfunction_handler just for that event.
	 */
};

struct listbox_layout
{
	struct marquee
	{
		class deleter : std::default_delete<fix64[]>
		{
		public:
			void operator()(marquee *const m) const
			{
				static_assert(std::is_trivially_destructible<marquee>::value, "marquee destructor not called");
				std::default_delete<fix64[]>::operator()(reinterpret_cast<fix64 *>(m));
			}
		};
		using ptr = std::unique_ptr<marquee, deleter>;
		static ptr allocate(const unsigned maxchars)
		{
			const unsigned max_bytes = maxchars + 1 + sizeof(marquee);
			auto pf = std::make_unique<fix64[]>(1 + (max_bytes / sizeof(fix64)));
			auto pm = ptr(new(pf.get()) marquee(maxchars));
			pf.release();
			return pm;
		}
		marquee(const unsigned mc) :
			maxchars(mc)
		{
		}
		fix64 lasttime; // to scroll text if string does not fit in box
		const unsigned maxchars;
		int pos = 0, scrollback = 0;
		char text[0];	/* must be last */
	};
	listbox_layout(int citem, unsigned nitems, const char **item, menu_title title) :
		citem(citem), nitems(nitems), item(item), title(title)
	{
		create_structure();
	}
	void create_structure();
	unsigned items_on_screen;
	int box_x, box_y;
	int box_w, height, title_height;
	int citem, first_item;
	unsigned nitems;
	const char **const item;
	const menu_title title;
	marquee::ptr marquee;
	short swidth, sheight;
	// with these we check if resolution or fonts have changed so listbox structure can be recreated
	font_x_scale_proportion fntscalex;
	font_y_scale_proportion fntscaley;
};

template <typename T>
using newmenu_subfunction_t = int(*)(newmenu *menu,const d_event &event, T *userdata);
using newmenu_subfunction = newmenu_subfunction_t<void>;

class unused_newmenu_userdata_t;
constexpr newmenu_subfunction_t<const unused_newmenu_userdata_t> unused_newmenu_subfunction = nullptr;
constexpr const unused_newmenu_userdata_t *unused_newmenu_userdata = nullptr;

//should be called whenever the palette changes
void newmenu_free_background();

int newmenu_do2(menu_title title, menu_subtitle subtitle, partial_range_t<newmenu_item *> items, newmenu_subfunction subfunction, void *userdata, int citem, menu_filename filename);

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
int newmenu_do2(const menu_title title, const menu_subtitle subtitle, partial_range_t<newmenu_item *> items, const newmenu_subfunction_t<T> subfunction, T *const userdata, const int citem = 0, const menu_filename filename = {})
{
	return newmenu_do2(title, subtitle, std::move(items), reinterpret_cast<newmenu_subfunction>(subfunction), static_cast<void *>(userdata), citem, filename);
}

template <typename T>
int newmenu_do2(const menu_title title, const menu_subtitle subtitle, partial_range_t<newmenu_item *> items, const newmenu_subfunction_t<const T> subfunction, const T *const userdata, const int citem = 0, const menu_filename filename = {})
{
	return newmenu_do2(title, subtitle, std::move(items), reinterpret_cast<newmenu_subfunction>(subfunction), static_cast<void *>(const_cast<T *>(userdata)), citem, filename);
}

enum class mission_filter_mode
{
	exclude_anarchy,
	include_anarchy,
};

}

namespace dsx {

//Handles creating and selecting from the mission list.
//Returns 1 if a mission was loaded.
int select_mission (mission_filter_mode anarchy_mode, menu_title message, window_event_result (*when_selected)(void));

}

newmenu_item *newmenu_get_items(newmenu *menu);
int newmenu_get_citem(newmenu *menu);

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

int nm_messagebox_str(menu_title title, const nm_messagebox_tie &tie, menu_subtitle str);
int vnm_messagebox_aN(menu_title title, const nm_messagebox_tie &tie, const char *format, ...) __attribute_format_printf(3, 4);

void nm_draw_background(grs_canvas &, int x1, int y1, int x2, int y2);

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

namespace dcx {

struct listbox : listbox_layout, window
{
	listbox(int citem, unsigned nitems, const char **item, menu_title title, grs_canvas &canvas, uint8_t allow_abort_flag);
	const uint8_t allow_abort_flag;
	uint8_t mouse_state = 0;
	marquee::ptr marquee;
	virtual window_event_result event_handler(const d_event &) override;
	virtual window_event_result callback_handler(const d_event &, window_event_result default_return_value) = 0;
};

const char **listbox_get_items(listbox &lb);
int listbox_get_citem(listbox &lb);
void listbox_delete_item(listbox &lb, int item);
}

static inline void nm_set_item_menu(newmenu_item &ni, const char *text)
{
	ni.type = nm_type::menu;
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
static inline void nm_set_item_input(newmenu_item &ni, unsigned len, char *text, const char *const allowed_chars)
{
	ni.type = nm_type::input;
	ni.text = text;
	auto &i = ni.input();
	i.text_len = len - 1;
	i.allowed_chars = allowed_chars;
}

template <std::size_t len>
static inline void nm_set_item_input(newmenu_item &ni, char (&text)[len], const char *const allowed_chars = nullptr)
{
	nm_set_item_input(ni, len, text, allowed_chars);
}

template <std::size_t len>
static inline void nm_set_item_input(newmenu_item &ni, std::array<char, len> &text, const char *const allowed_chars = nullptr)
{
	nm_set_item_input(ni, len, text.data(), allowed_chars);
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
	ni.type = nm_type::check;
	ni.text = const_cast<char *>(text);
	ni.value = checked;
}

__attribute_nonnull()
static inline void nm_set_item_text(newmenu_item &ni, const char *text)
{
	ni.type = nm_type::text;
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
	ni.type = nm_type::radio;
	ni.text = const_cast<char *>(text);
	ni.value = checked;
	auto &radio = ni.radio();
	radio.group = grp;
}

__attribute_nonnull()
static inline void nm_set_item_number(newmenu_item &ni, const char *text, unsigned now, unsigned low, unsigned high)
{
	ni.type = nm_type::number;
	ni.text = const_cast<char *>(text);
	ni.value = now;
	auto &number = ni.number();
	number.min_value = low;
	number.max_value = high;
}

__attribute_nonnull()
static inline void nm_set_item_slider(newmenu_item &ni, const char *text, unsigned now, unsigned low, unsigned high)
{
	ni.type = nm_type::slider;
	ni.text = const_cast<char *>(text);
	ni.value = now;
	auto &slider = ni.slider();
	slider.min_value = low;
	slider.max_value = high;
}

struct passive_messagebox_item
{
	std::array<newmenu_item, 1> m;
	passive_messagebox_item(const char *const item) :
		m{{nm_item_menu(item)}}
	{
	}
};

struct passive_messagebox : passive_messagebox_item, passive_newmenu
{
	passive_messagebox(const menu_title title, const menu_subtitle subtitle, const char *const item, grs_canvas &src) :
		passive_messagebox_item(item),
		passive_newmenu(title, subtitle, menu_filename{nullptr}, tiny_mode_flag::normal, tab_processing_flag::ignore, adjusted_citem::create(m, 0), src)
	{
	}
};

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
