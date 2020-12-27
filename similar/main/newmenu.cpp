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

#include <stdio.h>
#include <cstdlib>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <functional>

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
#include "text.h"
#include "menu.h"
#include "newmenu.h"
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
#if defined(DXX_BUILD_DESCENT_II)
#include "gamepal.h"
#endif

#if DXX_USE_OGL
#include "ogl_init.h"
#endif

#include "compiler-range_for.h"
#include "d_zip.h"
#include "partial_range.h"

#define MESSAGEBOX_TEXT_SIZE 2176  // How many characters in messagebox
#define MAX_TEXT_WIDTH FSPACX(120) // How many pixels wide a input box can be

namespace dcx {

int passive_newmenu::subfunction_handler(const d_event &)
{
	return 0;
}

namespace {

struct callback_newmenu : newmenu
{
	callback_newmenu(const menu_title title, const menu_subtitle subtitle, const menu_filename filename, const tiny_mode_flag tiny_mode, const tab_processing_flag tabs_flag, const adjusted_citem citem_init, grs_canvas &src, subfunction_type subfunction, void *userdata) :
		newmenu(title, subtitle, filename, tiny_mode, tabs_flag, citem_init, src, filename ? draw_box_flag::none : draw_box_flag::menu_background),
		subfunction(subfunction), userdata(userdata)
	{
	}
	const subfunction_type subfunction;
	void *const userdata;		// For whatever - like with window system
	virtual int subfunction_handler(const d_event &event) override;
};

struct step_down
{
	template <typename T>
		T operator()(T &t) const
		{
			return ++t;
		}
};

struct step_up
{
	template <typename T>
		T operator()(T &t) const
		{
			return --t;
		}
};

static grs_main_bitmap nm_background, nm_background1;
static grs_subbitmap_ptr nm_background_sub;

static void prepare_slider_text(std::array<char, NM_MAX_TEXT_LEN + 1> &text, const std::size_t offset, const std::size_t steps)
{
	/* 3 = (1 for SLIDER_LEFT) + (1 for SLIDER_RIGHT) + (1 null) */
	constexpr std::size_t reserved_space = 3;
	const std::size_t unreserved_space = text.size() - reserved_space;
	if (offset > unreserved_space ||
		steps > unreserved_space ||
		offset + steps > unreserved_space)
	{
		text[0] = 0;
		return;
	}
	text[offset] = SLIDER_LEFT[0];
	text[offset + 1 + steps] = SLIDER_RIGHT[0];
	text[offset + 2 + steps] = 0;
	std::fill_n(&text[offset + 1], steps, SLIDER_MIDDLE[0]);
}

}

void newmenu_free_background()	{
	if (nm_background.bm_data)
	{
		nm_background_sub.reset();
	}
	nm_background.reset();
	nm_background1.reset();
}

newmenu_layout::adjusted_citem newmenu_layout::adjusted_citem::create(const partial_range_t<newmenu_item *> items, int citem)
{
	if (citem < 0)
		citem = 0;
	const std::size_t nitems = items.size();
	if (citem > nitems - 1)
		citem = nitems - 1;
	uint_fast32_t i = 0;
	const auto begin = items.begin();
	uint8_t all_text = 0;
	while (std::next(begin, citem)->type == nm_type::text)
	{
		if (++ citem >= nitems)
			citem = 0;
		if (++ i > nitems)
		{
			citem = 0;
			all_text = 1;
			break;
		}
	}
	return adjusted_citem{items, citem, all_text};
}

int callback_newmenu::subfunction_handler(const d_event &event)
{
	if (!subfunction)
		return 0;
	return (*subfunction)(this, event, userdata);
}

}

namespace dsx {

namespace {

newmenu *newmenu_do4(menu_title title, menu_subtitle subtitle, partial_range_t<newmenu_item *> items, newmenu_subfunction subfunction, void *userdata, int citem, menu_filename filename, tiny_mode_flag TinyMode = tiny_mode_flag::normal, tab_processing_flag TabsFlag = tab_processing_flag::ignore);

#if defined(DXX_BUILD_DESCENT_I)
static const char *UP_ARROW_MARKER(const grs_font &, const grs_font &)
{
	return "+";  // 135
}

static const char *DOWN_ARROW_MARKER(const grs_font &, const grs_font &)
{
	return "+";  // 136
}
#elif defined(DXX_BUILD_DESCENT_II)
static const char *UP_ARROW_MARKER(const grs_font &cv_font, const grs_font &game_font)
{
	return &cv_font == &game_font ? "\202" : "\207";  // 135
}

static const char *DOWN_ARROW_MARKER(const grs_font &cv_font, const grs_font &game_font)
{
	return &cv_font == &game_font ? "\200" : "\210";  // 136
}
#endif

// Draws the custom menu background pcx, if available
static void nm_draw_background1(grs_canvas &canvas, const char * filename)
{
	if (filename != NULL)
	{
		if (nm_background1.bm_data == NULL)
		{
			pcx_read_bitmap_or_default(filename, nm_background1, gr_palette);
		}
		gr_palette_load( gr_palette );
		show_fullscr(canvas, nm_background1);
	}
#if defined(DXX_BUILD_DESCENT_II)
	strcpy(last_palette_loaded,"");		//force palette load next time
#endif
}

}

}

#define MENU_BACKGROUND_BITMAP_HIRES (PHYSFSX_exists("scoresb.pcx",1)?"scoresb.pcx":"scores.pcx")
#define MENU_BACKGROUND_BITMAP_LORES (PHYSFSX_exists("scores.pcx",1)?"scores.pcx":"scoresb.pcx")
#define MENU_BACKGROUND_BITMAP (HIRESMODE?MENU_BACKGROUND_BITMAP_HIRES:MENU_BACKGROUND_BITMAP_LORES)

// Draws the frame background for menus
void nm_draw_background(grs_canvas &canvas, int x1, int y1, int x2, int y2)
{
	int w,h,init_sub=0;
	static float BGScaleX=1,BGScaleY=1;
	if (nm_background.bm_data == NULL)
	{
		palette_array_t background_palette;
		pcx_read_bitmap_or_default(MENU_BACKGROUND_BITMAP, nm_background, background_palette);
		gr_remap_bitmap_good(nm_background, background_palette, -1, -1);
		BGScaleX=(static_cast<float>(SWIDTH)/nm_background.bm_w);
		BGScaleY=(static_cast<float>(SHEIGHT)/nm_background.bm_h);
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

	gr_palette_load( gr_palette );
	{
		const auto &&tmp = gr_create_sub_canvas(canvas, x1, y1, w, h);
		show_fullscr(*tmp, nm_background); // show so we load all necessary data for the sub-bitmap
	if (!init_sub && ((nm_background_sub->bm_w != w*((static_cast<float>(nm_background.bm_w))/SWIDTH)) || (nm_background_sub->bm_h != h*((static_cast<float>(nm_background.bm_h))/SHEIGHT))))
	{
		init_sub=1;
	}
	if (init_sub)
		nm_background_sub = gr_create_sub_bitmap(nm_background,0,0,w*((static_cast<float>(nm_background.bm_w))/SWIDTH),h*((static_cast<float>(nm_background.bm_h))/SHEIGHT));
	show_fullscr(*tmp, *nm_background_sub.get());
	}

	gr_settransblend(canvas, 14, gr_blend::normal);
	{
		const auto color = BM_XRGB(1, 1, 1);
	for (w=5*BGScaleX;w>0;w--)
			gr_urect(canvas, x2 - w, y1 + w * (BGScaleY / BGScaleX), x2 - w, y2 - w * (BGScaleY / BGScaleX), color);//right edge
	}
	{
		const auto color = BM_XRGB(0, 0, 0);
	for (h=5*BGScaleY;h>0;h--)
			gr_urect(canvas, x1 + h * (BGScaleX / BGScaleY), y2 - h, x2 - h * (BGScaleX / BGScaleY), y2 - h, color);//bottom edge
	}
	gr_settransblend(canvas, GR_FADE_OFF, gr_blend::normal);
}

namespace dcx {

namespace {

// Draw a left justfied string
static void nm_string(grs_canvas &canvas, const grs_font &cv_font, const int w1, int x, const int y, const char *const s, const tab_processing_flag tabs_flag)
{
	if (tabs_flag == tab_processing_flag::ignore)
	{
		const char *s1 = s;
		const char *p = nullptr;
		RAIIdmem<char[]> s2;
		if (w1 > 0 && (p = strchr(s, '\t')))
		{
			s2.reset(d_strdup(s));
			s1 = s2.get();
			*std::next(s2.get(), std::distance(s, p)) = '\0';
		}
		gr_string(canvas, cv_font, x, y, s1);
		if (p)
		{
			int w, h;
			++ p;
			gr_get_string_size(cv_font, p, &w, &h, nullptr);
			gr_string(canvas, cv_font, x + w1 - w, y, p, w, h);
		}
		return;
	}
	std::array<int, 6> XTabs = {{18, 90, 127, 165, 231, 256}};
	const auto &&fspacx = FSPACX();
	range_for (auto &i, XTabs)
	{
		i = fspacx(i) + x;
	}
	unsigned t = 0;
	char measure[2];
	measure[1] = 0;
	for (unsigned i = 0; const char c = s[i]; ++i)
	{
		if (c == '\t')
		{
			x=XTabs[t];
			t++;
			continue;
		}
		measure[0] = c;
		int tx, th;
		gr_get_string_size(cv_font, measure, &tx, &th, nullptr);
		gr_string(canvas, cv_font, x, y, measure, tx, th);
		x+=tx;
	}
}

// Draw a slider and it's string
static void nm_string_slider(grs_canvas &canvas, const grs_font &cv_font, const int w1, const int x, const int y, char *const s)
{
	char *p,*s1;

	s1=NULL;

	p = strchr( s, '\t' );
	if (p)	{
		*p = '\0';
		s1 = p+1;
	}

	gr_string(canvas, cv_font, x, y, s);

	if (p)	{
		int w, h;
		gr_get_string_size(cv_font, s1, &w, &h, nullptr);
		gr_string(canvas, cv_font, x + w1 - w, y, s1, w, h);

		*p = '\t';
	}
}


// Draw a left justfied string with black background.
static void nm_string_black(grs_canvas &canvas, int w1, const int x, const int y, const char *const s)
{
	int w,h;
	gr_get_string_size(*canvas.cv_font, s, &w, &h, nullptr);

	if (w1 == 0) w1 = w;

	const auto &&fspacx = FSPACX();
	const auto &&fspacy = FSPACY();
	{
		const uint8_t color = BM_XRGB(5, 5, 5);
		gr_rect(canvas, x - fspacx(2), y - fspacy(1), x + w1, y + h, color);
	}
	{
		const uint8_t color = BM_XRGB(2, 2, 2);
		gr_rect(canvas, x - fspacx(2), y - fspacy(1), x, y + h, color);
	}
	{
		const uint8_t color = BM_XRGB(0, 0, 0);
		gr_rect(canvas, x - fspacx(1), y - fspacy(1), x + w1 - fspacx(1), y + h, color);
	}
	gr_string(canvas, *canvas.cv_font, x, y, s, w, h);
}


// Draw a right justfied string
static void nm_rstring(grs_canvas &canvas, const grs_font &cv_font, int w1, int x, const int y, const char *const s)
{
	int w, h;
	gr_get_string_size(cv_font, s, &w, &h, nullptr);
	x -= FSPACX(3);

	if (w1 == 0) w1 = w;
	gr_string(canvas, cv_font, x - w, y, s, w, h);
}

static void nm_string_inputbox(grs_canvas &canvas, const grs_font &cv_font, const int w, const int x, const int y, const char *text, const int current)
{
	int w1;

	// even with variable char widths and a box that goes over the whole screen, we maybe never get more than 75 chars on the line
	if (strlen(text)>75)
		text+=strlen(text)-75;
	while( *text )	{
		gr_get_string_size(cv_font, text, &w1, nullptr, nullptr);
		if ( w1 > w-FSPACX(10) )
			text++;
		else
			break;
	}
	if ( *text == 0 )
		w1 = 0;

	nm_string_black(canvas, w, x, y, text);

	if ( current && timer_query() & 0x8000 )
		gr_string(canvas, cv_font, x + w1, y, CURSOR_STRING);
}

static void draw_item(grs_canvas &canvas, const grs_font &cv_font, newmenu_item &item, int is_current, const tiny_mode_flag tiny, const tab_processing_flag tabs_flag, int scroll_offset)
{
	if (tiny != tiny_mode_flag::normal)
	{
		int r, g, b;
		if (item.text[0] == '\t')
			r = g = b = 63;
		else if (is_current)
			r = 57, g = 49, b = 20;
		else
			r = g = 29, b = 47;
		gr_set_fontcolor(canvas, gr_find_closest_color_current(r, g, b), -1);
	}

	const int line_spacing = static_cast<int>(LINE_SPACING(cv_font, *GAME_FONT));
	switch(item.type)
	{
		case nm_type::slider:
		{
			int i;
			auto &slider = item.slider();
			if (item.value < slider.min_value)
				item.value = slider.min_value;
			if (item.value > slider.max_value)
				item.value = slider.max_value;
			auto &saved_text = slider.saved_text;
			i = snprintf(saved_text.data(), saved_text.size(), "%s\t", item.text);
			prepare_slider_text(saved_text, i, slider.max_value - slider.min_value + 1);
			saved_text[item.value + 1 + strlen(item.text) + 1] = SLIDER_MARKER[0];
			nm_string_slider(canvas, cv_font, item.w, item.x, item.y - (line_spacing * scroll_offset), saved_text.data());
		}
			break;
		case nm_type::input_menu:
			if (item.imenu().group == 0)
			{
				case nm_type::text:
				case nm_type::menu:
				nm_string(canvas, cv_font, item.w, item.x, item.y - (line_spacing * scroll_offset), item.text, tabs_flag);
				break;
			}
			DXX_BOOST_FALLTHROUGH;
		case nm_type::input:
			nm_string_inputbox(canvas, cv_font, item.w, item.x, item.y - (line_spacing * scroll_offset), item.text, is_current);
			break;
		case nm_type::check:
			nm_string(canvas, cv_font, item.w, item.x, item.y - (line_spacing * scroll_offset), item.text, tabs_flag);
			nm_rstring(canvas, cv_font, item.right_offset, item.x, item.y - (line_spacing * scroll_offset), item.value ? CHECKED_CHECK_BOX : NORMAL_CHECK_BOX);
			break;
		case nm_type::radio:
			nm_string(canvas, cv_font, item.w, item.x, item.y - (line_spacing * scroll_offset), item.text, tabs_flag);
			nm_rstring(canvas, cv_font, item.right_offset, item.x, item.y - (line_spacing * scroll_offset), item.value ? CHECKED_RADIO_BOX : NORMAL_RADIO_BOX);
			break;
		case nm_type::number:
		{
			char text[sizeof("-2147483647")];
			auto &number = item.number();
			if (item.value < number.min_value)
				item.value = number.min_value;
			if (item.value > number.max_value)
				item.value = number.max_value;
			nm_string(canvas, cv_font, item.w, item.x, item.y - (line_spacing * scroll_offset), item.text, tabs_flag);
			snprintf(text, sizeof(text), "%d", item.value );
			nm_rstring(canvas, cv_font, item.right_offset, item.x, item.y - (line_spacing * scroll_offset), text);
		}
			break;
	}
}

//returns true if char is allowed
static bool char_disallowed(char c, const char *const allowed_chars)
{
	const char *p = allowed_chars;
	if (!p)
		return false;
	for (uint8_t a, b; (a = p[0]) && (b = p[1]); p += 2)
	{
		if (likely(c >= a && c <= b))
			return false;
	}
	return true;
}

static bool char_allowed(char c, const char *const allowed_chars)
{
	return !char_disallowed(c, allowed_chars);
}

static void strip_end_whitespace( char * text )
{
	char *ns = text;
	for (char c; (c = *text);)
	{
		++ text;
		if (!isspace(static_cast<unsigned>(c)))
			ns = text;
	}
	*ns = 0;
}

}

int newmenu_do2(const menu_title title, const menu_subtitle subtitle, const partial_range_t<newmenu_item *> items, const newmenu_subfunction subfunction, void *const userdata, const int citem, const menu_filename filename)
{
	newmenu *menu;
	menu = newmenu_do4(title, subtitle, items, subfunction, userdata, citem, filename);

	if (!menu)
		return -1;
	return newmenu::process_until_closed(menu);
}

int newmenu::process_until_closed(newmenu *const menu)
{
	bool exists = true;
	int rval = -1;
	menu->rval = &rval;
	// Track to see when the window is freed
	// Doing this way in case another window is opened on top without its own polling loop
	menu->track(&exists);

	// newmenu_do2 and simpler get their own event loop
	// This is so the caller doesn't have to provide a callback that responds to EVENT_NEWMENU_SELECTED
	while (exists)
		event_process();

	/* menu is now a pointer to freed memory, and cannot be accessed
	 * further
	 */
	return rval;
}

namespace {

static void swap_menu_item_entries(newmenu_item &a, newmenu_item &b)
{
	using std::swap;
	swap(a.text, b.text);
	swap(a.value, b.value);
}

template <typename F>
static inline void rotate_menu_item_subrange(F &&step_function, newmenu_item *const range_begin, const std::size_t start_index, const newmenu_item *const subrange_stop)
{
	const auto subrange_start = std::next(range_begin, start_index);
	/* The terminating pointer is named `subrange_stop`, rather than `end`,
	 * because traditional semantics hold that `end` cannot be
	 * dereferenced.  In this function, the terminating pointer points
	 * to the last element that is legal to dereference, rather than the
	 * first element that is not legal.
	 */
	auto iter = subrange_start;
	if (iter == subrange_stop)
		/* If iter == subrange_stop, then this function is an elaborate
		 * no-op.  Return immediately if there is no work to do.
		 */
		return;
	auto &&selected = std::pair(iter->text, iter->value);
	for (; iter != subrange_stop;)
	{
		auto &a = *iter;
		iter = step_function(iter, 1);
		auto &b = *iter;
		a.text = b.text;
		a.value = b.value;
	}
	iter->text = selected.first;
	iter->value = selected.second;
}

}

void reorder_newmenu::event_key_command(const d_event &event)
{
	switch(event_key_get(event))
	{
		case KEY_SHIFTED+KEY_UP:
			if (const auto ci = citem; ci > 0)
			{
				const auto ni = -- citem;
				const auto begin = items.begin();
				auto &a = *std::next(begin, ci);
				auto &b = *std::next(begin, ni);
				swap_menu_item_entries(a, b);
			}
			break;
		case KEY_SHIFTED+KEY_DOWN:
			if (const auto ci = citem; ci < items.size() - 1)
			{
				const auto ni = ++ citem;
				const auto begin = items.begin();
				auto &a = *std::next(begin, ci);
				auto &b = *std::next(begin, ni);
				swap_menu_item_entries(a, b);
			}
			break;
		case KEY_PAGEUP + KEY_SHIFTED:
			{
				const auto begin = items.begin();
				const auto stop = begin;
				rotate_menu_item_subrange(std::minus<void>(), begin, citem, stop);
			}
			break;
		case KEY_PAGEDOWN + KEY_SHIFTED:
			{
				const auto begin = items.begin();
				const auto stop = std::prev(items.end());
				rotate_menu_item_subrange(std::plus<void>(), begin, citem, stop);
			}
			break;
	}
}

}

newmenu_item *newmenu_get_items(newmenu *menu)
{
	return menu->items.begin();
}

int newmenu_get_citem(newmenu *menu)
{
	return menu->citem;
}

namespace {

template <typename S, typename O>
static void update_menu_position(newmenu &menu, newmenu_item *const stop, int_fast32_t amount, S step, O overflow)
{
	auto icitem = menu.citem;
	auto pcitem = std::next(menu.items.begin(), icitem);
	do // count until we reached a non NM_TYPE_TEXT item and reached our amount
	{
		if (pcitem == stop)
			break;
		step(icitem);
		step(pcitem);
		if (menu.is_scroll_box) // update scroll_offset as we go
		{
			if (overflow(icitem))
				step(menu.scroll_offset);
		}
	} while (-- amount > 0 || pcitem->type == nm_type::text);
	menu.citem = icitem;
}

static void newmenu_scroll(newmenu *const menu, const int amount)
{
	if (amount == 0) // nothing to do for us
		return;

	if (menu->all_text)
	{
		menu->scroll_offset += amount;
		if (menu->scroll_offset < 0)
			menu->scroll_offset = 0;
		const auto nitems = menu->items.size();
		if (menu->max_on_menu+menu->scroll_offset > nitems)
			menu->scroll_offset = nitems - menu->max_on_menu;
		return;
	}
	const auto &range = menu->items;
	const auto predicate = [](const newmenu_item &n) {
		return n.type != nm_type::text;
	};
	const auto first = std::find_if(range.begin(), range.end(), predicate);
	if (first == range.end())
		return;
	const auto rlast = std::find_if(range.rbegin(), std::reverse_iterator<newmenu_item *>(first), predicate).base();
	/* `first == rlast` should not happen, since that would mean that
	 * there are no elements in `range` for which `predicate` is true.
	 * If there are no such elements, then `first == range.end()` should
	 * be true and the function would have returned above.
	 */
	assert(first != rlast);
	if (first == rlast) // nothing to do for us
		return;
	const auto last = std::prev(rlast);
	/* Exactly one element that satisfies `predicate` exists in `range`.
	 * Only elements that satisfy `predicate` can be selected, so the
	 * selection cannot be changed.
	 */
	if (first == last)
		return;
	auto citem = std::next(menu->items.begin(), menu->citem);
	if (citem == last && amount == 1) // if citem == last item and we want to go down one step, go to first item
	{
		newmenu_scroll(menu, -menu->items.size());
		return;
	}
	if (citem == first && amount == -1) // if citem == first item and we want to go up one step, go to last item
	{
		newmenu_scroll(menu, menu->items.size());
		return;
	}

	if (amount > 0) // down the list
	{
		const auto nitems = menu->items.size();
		const auto overflow = [menu, nitems](int icitem) {
			return icitem + 4 >= menu->max_on_menu + menu->scroll_offset && menu->scroll_offset < nitems - menu->max_on_menu;
		};
		update_menu_position(*menu, last, amount, step_down(), overflow);
	}
	else if (amount < 0) // up the list
	{
		const auto overflow = [menu](int icitem) {
			return icitem - 4 < menu->scroll_offset && menu->scroll_offset > 0;
		};
		update_menu_position(*menu, first, std::abs(amount), step_up(), overflow);
	}
}

static int nm_trigger_radio_button(newmenu &menu, newmenu_item &citem)
{
	citem.value = 1;
	const auto cg = citem.radio().group;
	range_for (auto &r, menu.items)
	{
		if (&r != &citem && r.type == nm_type::radio && r.radio().group == cg)
		{
			if (r.value)
			{
				r.value = 0;
				return 1;
			}
		}
	}
	return 0;
}

static window_event_result newmenu_mouse(const d_event &event, newmenu *menu, int button)
{
	int old_choice, mx=0, my=0, mz=0, x1 = 0, x2, y1, y2, changed = 0;
	grs_canvas &menu_canvas = menu->w_canv;
	grs_canvas &save_canvas = *grd_curcanv;

	switch (button)
	{
		case MBTN_LEFT:
		{
			gr_set_current_canvas(menu_canvas);
			auto &canvas = *grd_curcanv;

			old_choice = menu->citem;

			const auto &&fspacx = FSPACX();
			if ((event.type == EVENT_MOUSE_BUTTON_DOWN) && !menu->all_text)
			{
				mouse_get_pos(&mx, &my, &mz);
				const int line_spacing = static_cast<int>(LINE_SPACING(*grd_curcanv->cv_font, *GAME_FONT));
				for (int i = menu->scroll_offset; i < menu->max_on_menu + menu->scroll_offset; ++i)
				{
					auto &iitem = *std::next(menu->items.begin(), i);
					x1 = canvas.cv_bitmap.bm_x + iitem.x - fspacx(13) /*- menu->items[i].right_offset - 6*/;
					x2 = x1 + iitem.w + fspacx(13);
					y1 = canvas.cv_bitmap.bm_y + iitem.y - (line_spacing * menu->scroll_offset);
					y2 = y1 + iitem.h;
					if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2))) {
						menu->citem = i;
						auto &citem = iitem;
						switch (citem.type)
						{
							case nm_type::check:
								citem.value = !citem.value;
								changed = 1;
								break;
							case nm_type::radio:
								changed = nm_trigger_radio_button(*menu, citem);
								break;
							case nm_type::text:
								menu->citem=old_choice;
								menu->mouse_state=0;
								break;
							case nm_type::menu:
							case nm_type::input:
							case nm_type::number:
							case nm_type::input_menu:
							case nm_type::slider:
								break;
						}
						break;
					}
				}
			}

			if ( menu->mouse_state ) {
				mouse_get_pos(&mx, &my, &mz);

				// check possible scrollbar stuff first
				const int line_spacing = static_cast<int>(LINE_SPACING(*grd_curcanv->cv_font, *GAME_FONT));
				if (menu->is_scroll_box) {
					int ScrollAllow=0;
					static fix64 ScrollTime=0;
					if (ScrollTime + F1_0/5 < timer_query())
					{
						ScrollTime = timer_query();
						ScrollAllow = 1;
					}

					if (menu->scroll_offset != 0) {
						int arrow_width, arrow_height;
						gr_get_string_size(*canvas.cv_font, UP_ARROW_MARKER(*canvas.cv_font, *GAME_FONT), &arrow_width, &arrow_height, nullptr);
						x1 = canvas.cv_bitmap.bm_x + BORDERX - fspacx(12);
						y1 = canvas.cv_bitmap.bm_y + std::next(menu->items.begin(), menu->scroll_offset)->y - (line_spacing * menu->scroll_offset);
						x2 = x1 + arrow_width;
						y2 = y1 + arrow_height;
						if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) && ScrollAllow) {
							newmenu_scroll(menu, -1);
						}
					}
					if (menu->scroll_offset + menu->max_displayable < menu->items.size())
					{
						int arrow_width, arrow_height;
						gr_get_string_size(*canvas.cv_font, DOWN_ARROW_MARKER(*canvas.cv_font, *GAME_FONT), &arrow_width, &arrow_height, nullptr);
						x1 = canvas.cv_bitmap.bm_x + BORDERX - fspacx(12);
						y1 = canvas.cv_bitmap.bm_y + std::next(menu->items.begin(), menu->scroll_offset + menu->max_displayable - 1)->y - (line_spacing * menu->scroll_offset);
						x2 = x1 + arrow_width;
						y2 = y1 + arrow_height;
						if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) && ScrollAllow) {
							newmenu_scroll(menu, 1);
						}
					}
				}

				for (int i = menu->scroll_offset; i < menu->max_on_menu + menu->scroll_offset; ++i)
				{
					auto &iitem = *std::next(menu->items.begin(), i);
					x1 = canvas.cv_bitmap.bm_x + iitem.x - fspacx(13);
					x2 = x1 + iitem.w + fspacx(13);
					y1 = canvas.cv_bitmap.bm_y + iitem.y - (line_spacing * menu->scroll_offset);
					y2 = y1 + iitem.h;

					if (mx > x1 && mx < x2 && my > y1 && my < y2 && iitem.type != nm_type::text)
					{
						menu->citem = i;
						auto &citem = iitem;
						if (citem.type == nm_type::slider)
						{
							auto &slider = citem.slider();
							auto &saved_text = slider.saved_text;
							char slider_text[NM_MAX_TEXT_LEN+1], *p, *s1;

							strcpy(slider_text, saved_text);
							p = strchr(slider_text, '\t');
							if (p) {
								*p = '\0';
								s1 = p+1;
							}
							if (p) {
								int slider_width, sleft_width, sright_width, smiddle_width;
								gr_get_string_size(*canvas.cv_font, s1, &slider_width, nullptr, nullptr);
								gr_get_string_size(*canvas.cv_font, SLIDER_LEFT, &sleft_width, nullptr, nullptr);
								gr_get_string_size(*canvas.cv_font, SLIDER_RIGHT, &sright_width, nullptr, nullptr);
								gr_get_string_size(*canvas.cv_font, SLIDER_MIDDLE, &smiddle_width, nullptr, nullptr);

								x1 = canvas.cv_bitmap.bm_x + citem.x + citem.w - slider_width;
								x2 = x1 + slider_width + sright_width;
								int new_value;
								auto &slider = citem.slider();
								if (
									(mx > x1 && mx < x1 + sleft_width && (new_value = slider.min_value, true)) ||
									(mx < x2 && mx > x2 - sright_width && (new_value = slider.max_value, true)) ||
									(mx > x1 + sleft_width && mx < x2 - sright_width - sleft_width && (new_value = (mx - x1 - sleft_width) / ((slider_width - sleft_width - sright_width) / (slider.max_value - slider.min_value + 1)), true))
								)
									if (citem.value != new_value)
									{
										citem.value = new_value;
										changed = 1;
									}
								*p = '\t';
							}
						}
						if (menu->citem == old_choice)
							break;
						if (citem.type == nm_type::input)
							citem.value = -1;
						if (!(old_choice > -1))
							break;
						auto &oitem = *std::next(menu->items.begin(), old_choice);
						if (oitem.type == nm_type::input_menu)
						{
							auto &im = oitem.imenu();
							im.group = 0;
							oitem.value = -1;
							strcpy(oitem.text, im.saved_text);
						}
						break;
					}
				}
			}

			if (event.type == EVENT_MOUSE_BUTTON_UP &&
				!menu->all_text &&
				menu->citem != -1 &&
				std::next(menu->items.begin(), menu->citem)->type == nm_type::menu)
			{
				mouse_get_pos(&mx, &my, &mz);
				const int line_spacing = static_cast<int>(LINE_SPACING(*grd_curcanv->cv_font, *GAME_FONT));
				for (int i = menu->scroll_offset; i < menu->max_on_menu + menu->scroll_offset; ++i)
				{
					auto &iitem = *std::next(menu->items.begin(), i);
					x1 = canvas.cv_bitmap.bm_x + iitem.x - fspacx(13);
					x2 = x1 + iitem.w + fspacx(13);
					y1 = canvas.cv_bitmap.bm_y + iitem.y - (line_spacing * menu->scroll_offset);
					y2 = y1 + iitem.h;
					if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2))) {
							// Tell callback, allow staying in menu
							const d_select_event selected{menu->citem};
							if (menu->subfunction_handler(selected))
								return window_event_result::handled;

							if (menu->rval)
								*menu->rval = menu->citem;
							gr_set_current_canvas(save_canvas);
							return window_event_result::close;
					}
				}
			}

			if (event.type == EVENT_MOUSE_BUTTON_UP && menu->citem > -1)
			{
				auto &citem = *std::next(menu->items.begin(), menu->citem);
				if (citem.type == nm_type::input_menu && citem.imenu().group == 0)
				{
					auto &im = citem.imenu();
					im.group = 1;
					if (!d_stricmp(im.saved_text, TXT_EMPTY))
					{
						citem.text[0] = 0;
						citem.value = -1;
					} else {
						strip_end_whitespace(citem.text);
					}
				}
			}

			gr_set_current_canvas(save_canvas);

			if (changed)
			{
				menu->subfunction_handler(d_change_event{menu->citem});
			}
			break;
		}
		case MBTN_RIGHT:
			if (menu->mouse_state)
			{
				if (!(menu->citem > -1))
					return window_event_result::close;
				auto &citem = *std::next(menu->items.begin(), menu->citem);
				if (citem.type != nm_type::input_menu)
					return window_event_result::close;
				if (auto &im = citem.imenu(); im.group == 1)
				{
					im.group = 0;
					citem.value = -1;
					strcpy(citem.text, im.saved_text);
				} else {
					return window_event_result::close;
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

	return window_event_result::ignored;
}

static window_event_result newmenu_key_command(const d_event &event, newmenu *const menu)
{
	int k = event_key_get(event);
	int old_choice;
	int changed = 0;
	window_event_result rval = window_event_result::handled;

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
	auto &citem = *std::next(menu->items.begin(), menu->citem);

	switch( k )	{
		case KEY_HOME:
		case KEY_PAD7:
			newmenu_scroll(menu, -menu->items.size());
			break;
		case KEY_END:
		case KEY_PAD1:
			newmenu_scroll(menu, menu->items.size());
			break;
		case KEY_TAB + KEY_SHIFTED:
		case KEY_UP:
		case KEY_PAGEUP:
		case KEY_PAD8:
			newmenu_scroll(menu, k == KEY_PAGEUP ? -10 : -1);
			if (citem.type == nm_type::input && menu->citem != old_choice)
				citem.value = -1;
			if (old_choice > -1 && old_choice != menu->citem)
			{
				auto &oitem = *std::next(menu->items.begin(), old_choice);
				if (oitem.type == nm_type::input_menu)
				{
					auto &im = oitem.imenu();
					im.group = 0;
					oitem.value = -1;
					strcpy(oitem.text, im.saved_text);
				}
			}
			break;
		case KEY_TAB:
		case KEY_DOWN:
		case KEY_PAGEDOWN:
		case KEY_PAD2:
			newmenu_scroll(menu, k == KEY_PAGEDOWN ? 10 : 1);
			if (citem.type == nm_type::input && menu->citem != old_choice)
				citem.value = -1;
			if (old_choice > -1 && old_choice != menu->citem)
			{
				auto &oitem = *std::next(menu->items.begin(), old_choice);
				if (oitem.type == nm_type::input_menu)
				{
					auto &im = oitem.imenu();
					im.group = 0;
					oitem.value = -1;
					strcpy(oitem.text, im.saved_text);
				}
			}
			break;
		case KEY_SPACEBAR:
			if ( menu->citem > -1 )	{
				switch (citem.type)
				{
					case nm_type::text:
					case nm_type::number:
					case nm_type::slider:
					case nm_type::menu:
					case nm_type::input:
					case nm_type::input_menu:
						break;
					case nm_type::check:
						citem.value = !citem.value;
						changed = 1;
						break;
					case nm_type::radio:
						changed = nm_trigger_radio_button(*menu, citem);
						break;
				}
			}
			break;

		case KEY_ENTER:
		case KEY_PADENTER:
			if (menu->citem > -1 && citem.type == nm_type::input_menu && citem.imenu().group == 0)
			{
				auto &im = citem.imenu();
				im.group = 1;
				if (!d_stricmp(im.saved_text, TXT_EMPTY))
				{
					citem.text[0] = 0;
					citem.value = -1;
				} else {
					strip_end_whitespace(citem.text);
				}
			} else
			{
				if (citem.type == nm_type::input_menu)
					citem.imenu().group = 0;	// go out of editing mode

				// Tell callback, allow staying in menu
				const d_select_event selected{menu->citem};
				if (menu->subfunction_handler(selected))
					return window_event_result::handled;

				if (menu->rval)
					*menu->rval = menu->citem;
				return window_event_result::close;
			}
			break;

		case KEY_ESC:
			if (menu->citem > -1 && citem.type == nm_type::input_menu && citem.imenu().group == 1)
			{
				auto &im = citem.imenu();
				im.group = 0;
				citem.value = -1;
				strcpy(citem.text, im.saved_text);
			} else {
				return window_event_result::close;
			}
			break;
		default:
			rval = window_event_result::ignored;
			break;
	}

	if ( menu->citem > -1 )	{
		// Alerting callback of every keypress for NM_TYPE_INPUT. Alternatively, just respond to EVENT_NEWMENU_SELECTED
		if ((citem.type == nm_type::input || (citem.type == nm_type::input_menu && citem.imenu().group == 1)) && old_choice == menu->citem)
		{
			if ( k==KEY_LEFT || k==KEY_BACKSP || k==KEY_PAD4 )	{
				if (citem.value == -1) citem.value = strlen(citem.text);
				if (citem.value > 0)
					citem.value--;
				citem.text[citem.value] = 0;

				if (citem.type == nm_type::input)
					changed = 1;
				rval = window_event_result::handled;
			}
			else if (const auto im = citem.input_or_menu(); citem.value < im->text_len)
			{
				auto ascii = key_ascii();
				if (ascii < 255)
				{
					if (citem.value == -1) {
						citem.value = 0;
					}
					const auto allowed_chars = im->allowed_chars;
					if (char_allowed(ascii, allowed_chars) || (ascii == ' ' && char_allowed(ascii = '_', allowed_chars)))
					{
						citem.text[citem.value++] = ascii;
						citem.text[citem.value] = 0;

						if (citem.type == nm_type::input)
							changed = 1;
					}
				}
			}
		}
		else if (citem.type != nm_type::input && citem.type != nm_type::input_menu)
		{
			auto ascii = key_ascii();
			if (ascii < 255 ) {
				int choice1 = menu->citem;
				ascii = toupper(ascii);
				do {
					choice1++;
					if (choice1 >= menu->items.size())
						choice1=0;

					unsigned ch;
					auto &choice_item = *std::next(menu->items.begin(), choice1);
					for (int i = 0; (ch = choice_item.text[i]) != 0 && ch == ' ' ; ++i)
					{
					}

					if ((choice_item.type == nm_type::menu ||
						  choice_item.type == nm_type::check ||
						  choice_item.type == nm_type::radio ||
						  choice_item.type == nm_type::number ||
						  choice_item.type == nm_type::slider)
						&& (ascii==toupper(ch)) )
					{
						k = 0;
						menu->citem = choice1;
					}

					while (menu->citem + 4 >= menu->max_on_menu + menu->scroll_offset && menu->scroll_offset < menu->items.size() - menu->max_on_menu)
						menu->scroll_offset++;
					while (menu->citem-4<menu->scroll_offset && menu->scroll_offset > 0)
						menu->scroll_offset--;

				} while (choice1 != menu->citem );
			}
		}

		if (const auto ns = citem.number_or_slider())
		{
			switch( k ) {
				case KEY_LEFT:
				case KEY_PAD4:
					citem.value -= 1;
					changed = 1;
					rval = window_event_result::handled;
					break;
				case KEY_RIGHT:
				case KEY_PAD6:
					citem.value++;
					changed = 1;
					rval = window_event_result::handled;
					break;
				case KEY_SPACEBAR:
					citem.value += 10;
					changed = 1;
					rval = window_event_result::handled;
					break;
				case KEY_BACKSP:
					citem.value -= 10;
					changed = 1;
					rval = window_event_result::handled;
					break;
			}

			auto &min_value = ns->min_value;
			if (citem.value < min_value)
				citem.value = min_value;
			auto &max_value = ns->max_value;
			if (citem.value > max_value)
				citem.value = max_value;
		}

	}

	if (changed)
	{
		menu->subfunction_handler(d_change_event{menu->citem});
	}

	return rval;
}

}

namespace dsx {

namespace {

static void newmenu_create_structure(newmenu_layout &menu, const grs_font &cv_font)
{
	int nmenus;
	grs_canvas &save_canvas = *grd_curcanv;
	gr_set_default_canvas();
	auto &canvas = *grd_curcanv;

	auto iterative_layout_max_width = 0u;
	auto iterative_layout_max_height = 0u;

	if (menu.title)
	{
		int string_width, string_height;
		auto &huge_font = *HUGE_FONT;
		gr_get_string_size(huge_font, menu.title, &string_width, &string_height, nullptr);
		iterative_layout_max_width = string_width;
		iterative_layout_max_height = string_height;
	}
	if (menu.subtitle)
	{
		int string_width, string_height;
		auto &medium3_font = *MEDIUM3_FONT;
		gr_get_string_size(medium3_font, menu.subtitle, &string_width, &string_height, nullptr);
		if (iterative_layout_max_width < string_width)
			iterative_layout_max_width = string_width;
		iterative_layout_max_height += string_height;
	}

	iterative_layout_max_height += FSPACY(5);		//put some space between titles & body

	int aw = 0;
	auto iterative_layout_body_width = 0u;
	const auto initial_layout_height = iterative_layout_max_height;
	nmenus = 0;

	const auto &&fspacx = FSPACX();
	const auto &&fspacy = FSPACY();
	const auto &&fspacx8 = fspacx(8);
	const auto &&fspacy1 = fspacy(1);
	// Find menu height & width (store in iterative_layout_max_width,
	// iterative_layout_max_height)
	range_for (auto &i, menu.items)
	{
		i.y = iterative_layout_max_height;
		int string_width, string_height, average_width;
		gr_get_string_size(cv_font, i.text, &string_width, &string_height, &average_width);
		i.right_offset = 0;

		if (i.type == nm_type::menu)
			nmenus++;

		if (i.type == nm_type::slider)
		{
			int w1;
			auto &slider = i.slider();
			auto &saved_text = slider.saved_text;
			prepare_slider_text(saved_text, 0, slider.max_value - slider.min_value + 1);
			gr_get_string_size(cv_font, saved_text.data(), &w1, nullptr, nullptr);
			string_width += w1 + aw;
		}

		else if (i.type == nm_type::check)
		{
			int w1;
			gr_get_string_size(cv_font, NORMAL_CHECK_BOX, &w1, nullptr, nullptr);
			i.right_offset = w1;
			gr_get_string_size(cv_font, CHECKED_CHECK_BOX, &w1, nullptr, nullptr);
			if (w1 > i.right_offset)
				i.right_offset = w1;
		}

		else if (i.type == nm_type::radio)
		{
			int w1;
			gr_get_string_size(cv_font, NORMAL_RADIO_BOX, &w1, nullptr, nullptr);
			i.right_offset = w1;
			gr_get_string_size(cv_font, CHECKED_RADIO_BOX, &w1, nullptr, nullptr);
			if (w1 > i.right_offset)
				i.right_offset = w1;
		}

		else if (i.type == nm_type::number)
		{
			int w1;
			char test_text[20];
			auto &number = i.number();
			snprintf(test_text, sizeof(test_text), "%d", number.max_value);
			gr_get_string_size(cv_font, test_text, &w1, nullptr, nullptr);
			i.right_offset = w1;
			snprintf(test_text, sizeof(test_text), "%d", number.min_value);
			gr_get_string_size(cv_font, test_text, &w1, nullptr, nullptr);
			if (w1 > i.right_offset)
				i.right_offset = w1;
		}

		if (const auto input_or_menu = i.input_or_menu())
		{
			const auto text_len = input_or_menu->text_len;
			string_width = text_len * fspacx8 + text_len;
			if (i.type == nm_type::input && string_width > MAX_TEXT_WIDTH)
				string_width = MAX_TEXT_WIDTH;

			i.value = -1;
			if (i.type == nm_type::input_menu)
			{
				auto &im = i.imenu();
				im.group = 0;
				im.saved_text.copy_if(i.text);
				nmenus++;
			}
		}

		i.w = string_width;
		i.h = string_height;

		if (iterative_layout_body_width < string_width)
			iterative_layout_body_width = string_width;		// Save maximum width
		if ( average_width > aw )
			aw = average_width;
		iterative_layout_max_height += string_height + fspacy1;		// Find the height of all strings
	}

	if (menu.items.size() > menu.max_on_menu)
	{
		iterative_layout_max_height = initial_layout_height + (LINE_SPACING(cv_font, *GAME_FONT) * menu.max_on_menu);

		// if our last citem was > menu.max_on_menu, make sure we re-scroll when we call this menu again
		if (menu.citem > menu.max_on_menu - 4)
		{
			menu.scroll_offset = menu.citem - (menu.max_on_menu - 4);
			if (menu.scroll_offset + menu.max_on_menu > menu.items.size())
				menu.scroll_offset = menu.items.size() - menu.max_on_menu;
		}
	}
	menu.h = iterative_layout_max_height;

	int right_offset = 0;

	range_for (auto &i, menu.items)
	{
		i.w = iterative_layout_body_width;
		if (right_offset < i.right_offset)
			right_offset = i.right_offset;
	}

	menu.w = iterative_layout_body_width + right_offset;

	int twidth = 0;
	if (menu.w < iterative_layout_max_width)
	{
		twidth = (iterative_layout_max_width - menu.w) / 2;
		menu.w = iterative_layout_max_width;
	}

	// Find min point of menu border
	menu.w += BORDERX*2;
	menu.h += BORDERY*2;

	menu.x = (canvas.cv_bitmap.bm_w - menu.w) / 2;
	menu.y = (canvas.cv_bitmap.bm_h - menu.h) / 2;

	if (menu.x < 0)
		menu.x = 0;
	if (menu.y < 0)
		menu.y = 0;

	nm_draw_background1(canvas, menu.filename);

	// Update all item's x & y values.
	range_for (auto &i, menu.items)
	{
		i.x = BORDERX + twidth + right_offset;
		i.y += BORDERY;
		if (i.type == nm_type::radio)
		{
			// find first marked one
			newmenu_item *fm = nullptr;
			range_for (auto &j, menu.items)
			{
				if (j.type == nm_type::radio && j.radio().group == i.radio().group)
				{
					if (!fm && j.value)
						fm = &j;
					j.value = 0;
				}
			}
			(fm ? *fm : i).value = 1;
		}
	}

	menu.mouse_state = 0;
	menu.swidth = SWIDTH;
	menu.sheight = SHEIGHT;
	menu.fntscalex = FNTScaleX;
	menu.fntscaley = FNTScaleY;
	gr_set_current_canvas(save_canvas);
}

static window_event_result newmenu_draw(newmenu *menu)
{
	grs_canvas &menu_canvas = menu->w_canv;
	grs_canvas &save_canvas = *grd_curcanv;
	int th = 0, ty, sx, sy;

	if (menu->swidth != SWIDTH || menu->sheight != SHEIGHT || menu->fntscalex != FNTScaleX || menu->fntscaley != FNTScaleY)
	{
		menu->create_structure();
		{
			gr_init_sub_canvas(menu_canvas, grd_curscreen->sc_canvas, menu->x, menu->y, menu->w, menu->h);
		}
	}

	gr_set_default_canvas();
	nm_draw_background1(*grd_curcanv, menu->filename);
	if (menu->draw_box != draw_box_flag::none)
	{
		const auto mx = menu->x;
		const auto my = menu->y;
		auto ex = mx;
		if (menu->is_scroll_box)
			ex -= FSPACX(5);
		nm_draw_background(*grd_curcanv, ex, my, mx + menu->w, my + menu->h);
	}

	gr_set_current_canvas( menu_canvas );

	ty = BORDERY;

	if ( menu->title )	{
		gr_set_fontcolor(*grd_curcanv, BM_XRGB(31, 31, 31), -1);
		int string_width, string_height;
		auto &huge_font = *HUGE_FONT;
		gr_get_string_size(huge_font, menu->title, &string_width, &string_height, nullptr);
		th = string_height;
		gr_string(*grd_curcanv, huge_font, 0x8000, ty, menu->title, string_width, string_height);
	}

	if ( menu->subtitle )	{
		gr_set_fontcolor(*grd_curcanv, BM_XRGB(21, 21, 21), -1);
		auto &medium3_font = *MEDIUM3_FONT;
		gr_string(*grd_curcanv, medium3_font, 0x8000, ty + th, menu->subtitle);
	}

	const auto tiny_mode = menu->tiny_mode;
	const auto &game_font = *GAME_FONT.get();
	const grs_font &noncurrent_item_cv_font = tiny_mode != tiny_mode_flag::normal
		? game_font
		: *MEDIUM1_FONT.get();
	// Redraw everything...
	{
		const auto begin = menu->items.begin();
		/* If the menu is all text, no item is current.  Set the
		 * "current" index to an impossible value so that the all_text
		 * test does not need to be run at every step.
		 */
		const auto current_index = menu->all_text ? ~0u : menu->citem;
		const auto tabs_flag = menu->tabs_flag;
		const auto scroll_offset = menu->scroll_offset;
		for (auto i = scroll_offset; i < menu->max_displayable + scroll_offset; ++i)
		{
			const auto is_current = (i == current_index);
			const grs_font &cv_font = (tiny_mode == tiny_mode_flag::normal && is_current)
				? *MEDIUM2_FONT.get()
				: noncurrent_item_cv_font;
			gr_set_curfont(*grd_curcanv, cv_font);
			draw_item(*grd_curcanv, cv_font, *std::next(begin, i), is_current, tiny_mode, tabs_flag, scroll_offset);
		}
	}

	if (menu->is_scroll_box)
	{
		auto &cv_font = (menu->tiny_mode != tiny_mode_flag::normal ? noncurrent_item_cv_font : *MEDIUM2_FONT);

		const int line_spacing = static_cast<int>(LINE_SPACING(cv_font, game_font));
		const auto scroll_offset = menu->scroll_offset;
		const auto begin = menu->items.begin();
		sy = std::next(begin, scroll_offset)->y - (line_spacing * scroll_offset);
		const auto &&fspacx = FSPACX();
		sx = BORDERX - fspacx(12);

		gr_string(*grd_curcanv, cv_font, sx, sy, scroll_offset ? UP_ARROW_MARKER(cv_font, game_font) : "  ");

		sy = std::next(begin, scroll_offset + menu->max_displayable - 1)->y - (line_spacing * scroll_offset);
		sx = BORDERX - fspacx(12);

		gr_string(*grd_curcanv, cv_font, sx, sy, (scroll_offset + menu->max_displayable < menu->items.size()) ? DOWN_ARROW_MARKER(*grd_curcanv->cv_font, game_font) : "  ");
	}
	menu->subfunction_handler(d_event{EVENT_NEWMENU_DRAW});
	gr_set_current_canvas(save_canvas);
	return window_event_result::handled;
}

}

}

namespace dcx {

void newmenu_layout::create_structure()
{
	newmenu_create_structure(*this, *(tiny_mode != tiny_mode_flag::normal ? GAME_FONT : MEDIUM1_FONT));
}

}

window_event_result newmenu::event_handler(const d_event &event)
{
#if DXX_MAX_BUTTONS_PER_JOYSTICK
	if (joy_translate_menu_key(event))
		return window_event_result::handled;
#endif

	{
		const auto rval = subfunction_handler(event);
#if 0	// No current instances of the subfunction closing the window itself (which is preferred)
		// Enable when all subfunctions return a window_event_result
		if (rval == window_event_result::deleted)
			return rval;	// some subfunction closed the window: bail!
#endif

		if (rval)
		{
			if (rval < -1)
			{
				if (this->rval)
					*this->rval = rval;
				return window_event_result::close;
			}

			return window_event_result::handled;		// event handled
		}
	}

	switch (event.type)
	{
		case EVENT_WINDOW_ACTIVATED:
			game_flush_inputs(Controls);
			event_toggle_focus(0);
			key_toggle_repeat(1);
			break;

		case EVENT_WINDOW_DEACTIVATED:
			//event_toggle_focus(1);	// No cursor recentering
			key_toggle_repeat(1);
			mouse_state = 0;
			break;

		case EVENT_MOUSE_BUTTON_DOWN:
		case EVENT_MOUSE_BUTTON_UP:
		{
			int button = event_mouse_get_button(event);
			mouse_state = event.type == EVENT_MOUSE_BUTTON_DOWN;
			return newmenu_mouse(event, this, button);
		}

		case EVENT_KEY_COMMAND:
			return newmenu_key_command(event, this);
		case EVENT_IDLE:
			if (!(Game_mode & GM_MULTI && Game_wind))
				timer_delay2(CGameArg.SysMaxFPS);
			break;
		case EVENT_WINDOW_DRAW:
			return newmenu_draw(this);
		case EVENT_WINDOW_CLOSE:
			break;
		default:
			break;
	}
	return window_event_result::ignored;
}

namespace dsx {

namespace {

newmenu *newmenu_do4(const menu_title title, const menu_subtitle subtitle, const partial_range_t<newmenu_item *> items, const newmenu_subfunction subfunction, void *const userdata, const int citem, const menu_filename filename, const tiny_mode_flag TinyMode, const tab_processing_flag TabsFlag)
{
	if (items.size() < 1)
		return nullptr;
	auto menu = window_create<callback_newmenu>(title, subtitle, filename, TinyMode, TabsFlag, newmenu_layout::adjusted_citem::create(items, citem), grd_curscreen->sc_canvas, subfunction, userdata);

	newmenu_free_background();

	//set_screen_mode(SCREEN_MENU);	//hafta set the screen mode here or fonts might get changed/freed up if screen res changes

	// Create the basic window
	return menu;
}

}
}

int (vnm_messagebox_aN)(const menu_title title, const nm_messagebox_tie &tie, const char *format, ...)
{
	va_list args;
	char nm_text[MESSAGEBOX_TEXT_SIZE];
	va_start(args, format);
	vsnprintf(nm_text,sizeof(nm_text),format,args);
	va_end(args);
	return nm_messagebox_str(title, tie, menu_subtitle{nm_text});
}

int nm_messagebox_str(const menu_title title, const nm_messagebox_tie &tie, const menu_subtitle subtitle)
{
	std::array<newmenu_item, nm_messagebox_tie::maximum_arity> items;
	auto &&item_range = partial_range(items, tie.count());
	for (auto &&[i, s] : zip(item_range, tie))
		nm_set_item_menu(i, s);
	return newmenu_do2(title, subtitle, item_range, unused_newmenu_subfunction, unused_newmenu_userdata);
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

namespace dcx {

listbox::listbox(int citem, unsigned nitems, const char **item, menu_title title, grs_canvas &canvas, uint8_t allow_abort_flag) :
	listbox_layout(citem, nitems, item, title), window(canvas, box_x - BORDERX, box_y - title_height - BORDERY, box_w + 2 * BORDERX, height + 2 * BORDERY),
	allow_abort_flag(allow_abort_flag)
{
}

const char **listbox_get_items(listbox &lb)
{
	return lb.item;
}

int listbox_get_citem(listbox &lb)
{
	return lb.citem;
}

void listbox_delete_item(listbox &lb, int item)
{
	Assert(item >= 0);

	const auto nitems = lb.nitems;
	if (nitems <= 0)
		return;
	if (item < nitems - 1)
	{
		auto &items = lb.item;
		std::rotate(&items[item], &items[item + 1], &items[nitems]);
	}
	-- lb.nitems;
	if (lb.citem >= lb.nitems)
		lb.citem = lb.nitems ? lb.nitems - 1 : 0;
}

}

namespace {

static void update_scroll_position(listbox_layout &lb)
{
	if (lb.citem < 0)
		lb.citem = 0;

	if (lb.citem >= lb.nitems)
		lb.citem = lb.nitems-1;

	if (lb.citem < lb.first_item)
		lb.first_item = lb.citem;

	if (lb.citem >= lb.items_on_screen)
	{
		if (lb.first_item < lb.citem - lb.items_on_screen + 1)
			lb.first_item = lb.citem - lb.items_on_screen + 1;
	}

	if (lb.nitems <= lb.items_on_screen)
		lb.first_item = 0;

	if (lb.nitems >= lb.items_on_screen)
	{
		if (lb.first_item > lb.nitems - lb.items_on_screen)
			lb.first_item = lb.nitems - lb.items_on_screen;
	}
	if (lb.first_item < 0)
		lb.first_item = 0;
}

static window_event_result listbox_mouse(const d_event &event, listbox *lb, int button)
{
	switch (button)
	{
		case MBTN_LEFT:
		{
			if (lb->mouse_state)
			{
				int mx, my, mz;
				mouse_get_pos(&mx, &my, &mz);
				const int x1 = lb->box_x;
				if (mx <= x1)
					break;
				const int x2 = x1 + lb->box_w;
				if (mx >= x2)
					break;
				const int by = lb->box_y;
				if (my <= by)
					break;
				const int my_relative_by = my - by;
				const auto &&line_spacing = LINE_SPACING(*MEDIUM3_FONT, *GAME_FONT);
				if (line_spacing < 1)
					break;
				const int idx_relative_first_visible_item = my_relative_by / line_spacing;
				const auto first_visible_item = lb->first_item;
				const auto nitems = lb->nitems;
				const auto last_visible_item = std::min(first_visible_item + lb->items_on_screen, nitems);
				if (idx_relative_first_visible_item >= last_visible_item)
					break;
				const int px_within_item = my_relative_by % static_cast<int>(line_spacing);
				const int h = gr_get_string_height(*MEDIUM3_FONT, 1);
				if (px_within_item >= h)
					break;
				const int idx_absolute_item = idx_relative_first_visible_item + first_visible_item;
				if (idx_absolute_item >= nitems)
					break;
				lb->citem = idx_absolute_item;
			}
			else if (event.type == EVENT_MOUSE_BUTTON_UP)
			{
				int h;

				if (lb->citem < 0)
					return window_event_result::ignored;

				int mx, my, mz;
				mouse_get_pos(&mx, &my, &mz);
				gr_get_string_size(*MEDIUM3_FONT, lb->item[lb->citem], nullptr, &h, nullptr);
				const int x1 = lb->box_x;
				const int x2 = lb->box_x + lb->box_w;
				const int y1 = (lb->citem - lb->first_item) * LINE_SPACING(*MEDIUM3_FONT, *GAME_FONT) + lb->box_y;
				const int y2 = y1 + h;
				if ( ((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) )
				{
					// Tell callback, if it wants to close it will return window_event_result::close
					const d_select_event selected{lb->citem};
					return lb->callback_handler(selected, window_event_result::close);
				}
			}
			break;
		}
		case MBTN_RIGHT:
		{
			if (lb->allow_abort_flag && lb->mouse_state) {
				lb->citem = -1;
				return window_event_result::close;
			}
			break;
		}
		case MBTN_Z_UP:
		{
			if (lb->mouse_state)
			{
				lb->citem--;
				update_scroll_position(*lb);
			}
			break;
		}
		case MBTN_Z_DOWN:
		{
			if (lb->mouse_state)
			{
				lb->citem++;
				update_scroll_position(*lb);
			}
			break;
		}
		default:
			break;
	}

	return window_event_result::ignored;
}

static window_event_result listbox_key_command(const d_event &event, listbox *lb)
{
	int key = event_key_get(event);
	window_event_result rval = window_event_result::handled;

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
			lb->citem += lb->items_on_screen;
			break;
		case KEY_PAGEUP:
		case KEY_PAD9:
			lb->citem -= lb->items_on_screen;
			break;
		case KEY_ESC:
			if (lb->allow_abort_flag) {
				lb->citem = -1;
				return window_event_result::close;
			}
			break;
		case KEY_ENTER:
		case KEY_PADENTER:
			// Tell callback, if it wants to close it will return window_event_result::close
			{
				const d_select_event selected{lb->citem};
				return lb->callback_handler(selected, window_event_result::close);
			}
		default:
		{
			const unsigned ascii = key_ascii();
			if ( ascii < 255 )	{
				const unsigned upper_ascii = toupper(ascii);
				const auto nitems = lb->nitems;
				for (unsigned cc = lb->citem + 1, steps = 0; steps < nitems; ++steps, ++cc)
				{
					if (cc >= nitems)
						cc = 0;
					if (toupper(static_cast<unsigned>(lb->item[cc][0])) == upper_ascii)
					{
						lb->citem = cc;
						break;
					}
				}
			}
			rval = window_event_result::ignored;
		}
	}
	update_scroll_position(*lb);
	return rval;
}

}

namespace dcx {

void listbox_layout::create_structure()
{
	gr_set_default_canvas();
	auto &canvas = *grd_curcanv;

	auto &medium3_font = *MEDIUM3_FONT;

	box_w = 0;
	const auto &&fspacx = FSPACX();
	const auto &&fspacx10 = fspacx(10);
	const unsigned max_box_width = SWIDTH - (BORDERX * 2);
	unsigned marquee_maxchars = UINT_MAX;
	for (const auto i : unchecked_partial_range(item, nitems))
	{
		int w;
		gr_get_string_size(medium3_font, i, &w, nullptr, nullptr);
		w += fspacx10;
		if (w > max_box_width)
		{
			unsigned mmc = 1;
			for (;; ++mmc)
			{
				int w2;
				gr_get_string_size(medium3_font, i, &w2, nullptr, nullptr, mmc);
				if (w2 > max_box_width - fspacx10 || mmc > 128)
					break;
			}
			/* mmc is now the shortest initial subsequence that is wider
			 * than max_box_width.
			 *
			 * Next, search for whether any internal subsequences of
			 * lesser length are also too wide.  This can happen if all
			 * the initial characters are narrow, then characters
			 * outside the initial subsequence are wide.
			 */
			for (auto j = i;;)
			{
				int w2;
				gr_get_string_size(medium3_font, j, &w2, nullptr, nullptr, mmc);
				if (w2 > max_box_width - fspacx10)
				{
					/* This subsequence is too long.  Reduce the length
					 * and retry.
					 */
					if (!--mmc)
						break;
				}
				else
				{
					/* This subsequence fits.  Move to the next
					 * character.
					 */
					if (!*++j)
						break;
				}
			}
			w = max_box_width;
			if (marquee_maxchars > mmc)
				marquee_maxchars = mmc;
		}
		if (box_w < w)
			box_w = w;
	}

	{
		int w, h;
		gr_get_string_size(medium3_font, title, &w, &h, nullptr);
		if (box_w < w)
			box_w = w;
		title_height = h+FSPACY(5);
	}

	// The box is bigger than we can fit on the screen since at least one string is too long. Check how many chars we can fit on the screen (at least only - MEDIUM*_FONT is variable font!) so we can make a marquee-like effect.
	if (marquee_maxchars != UINT_MAX)
	{
		box_w = max_box_width;
		marquee = listbox::marquee::allocate(marquee_maxchars);
		marquee->lasttime = timer_query();
	}

	const auto &&line_spacing = LINE_SPACING(medium3_font, *GAME_FONT);
	const unsigned bordery2 = BORDERY * 2;
	const auto items_on_screen = std::max<unsigned>(
		std::min<unsigned>(((canvas.cv_bitmap.bm_h - bordery2 - title_height) / line_spacing) - 2, nitems),
		LB_ITEMS_ON_SCREEN);
	this->items_on_screen = items_on_screen;
	height = line_spacing * items_on_screen;
	box_x = (canvas.cv_bitmap.bm_w - box_w) / 2;
	box_y = (canvas.cv_bitmap.bm_h - (height + title_height)) / 2 + title_height;
	if (box_y < bordery2)
		box_y = bordery2;

	if (citem < 0)
		citem = 0;
	else if (citem >= nitems)
		citem = 0;

	first_item = 0;
	update_scroll_position(*this);

	swidth = SWIDTH;
	sheight = SHEIGHT;
	fntscalex = FNTScaleX;
	fntscaley = FNTScaleY;
}

}

namespace {

static window_event_result listbox_draw(listbox *lb)
{
	if (lb->swidth != SWIDTH || lb->sheight != SHEIGHT || lb->fntscalex != FNTScaleX || lb->fntscaley != FNTScaleY)
		lb->create_structure();

	gr_set_default_canvas();
	auto &canvas = *grd_curcanv;
	nm_draw_background(canvas, lb->box_x - BORDERX, lb->box_y - lb->title_height - BORDERY,lb->box_x + lb->box_w + BORDERX, lb->box_y + lb->height + BORDERY);
	auto &medium3_font = *MEDIUM3_FONT;
	gr_string(canvas, medium3_font, 0x8000, lb->box_y - lb->title_height, lb->title);

	const auto &&line_spacing = LINE_SPACING(medium3_font, *GAME_FONT);
	for (int i = lb->first_item; i < lb->first_item + lb->items_on_screen; ++i)
	{
		int y = (i - lb->first_item) * line_spacing + lb->box_y;
		const auto &&fspacx = FSPACX();
		const auto &&fspacy = FSPACY();
		const uint8_t color5 = BM_XRGB(5, 5, 5);
		const uint8_t color2 = BM_XRGB(2, 2, 2);
		const uint8_t color0 = BM_XRGB(0, 0, 0);
		if ( i >= lb->nitems )	{
			gr_rect(canvas, lb->box_x + lb->box_w - fspacx(1), y - fspacy(1), lb->box_x + lb->box_w, y + line_spacing, color5);
			gr_rect(canvas, lb->box_x - fspacx(1), y - fspacy(1), lb->box_x, y + line_spacing, color2);
			gr_rect(canvas, lb->box_x, y - fspacy(1), lb->box_x + lb->box_w - fspacx(1), y + line_spacing, color0);
		} else {
			auto &mediumX_font = *(i == lb->citem ? MEDIUM2_FONT : MEDIUM1_FONT);
			gr_rect(canvas, lb->box_x + lb->box_w - fspacx(1), y - fspacy(1), lb->box_x + lb->box_w, y + line_spacing, color5);
			gr_rect(canvas, lb->box_x - fspacx(1), y - fspacy(1), lb->box_x, y + line_spacing, color2);
			gr_rect(canvas, lb->box_x, y - fspacy(1), lb->box_x + lb->box_w - fspacx(1), y + line_spacing, color0);

			const char *showstr;
			std::size_t item_len;
			if (lb->marquee && (item_len = strlen(lb->item[i])) > lb->marquee->maxchars)
			{
				showstr = lb->marquee->text;
				static int prev_citem = -1;
				
				if (prev_citem != lb->citem)
				{
					lb->marquee->pos = lb->marquee->scrollback = 0;
					lb->marquee->lasttime = timer_query();
					prev_citem = lb->citem;
				}

				unsigned srcoffset = 0;
				if (i == lb->citem)
				{
					const auto tq = timer_query();
					if (lb->marquee->lasttime + (F1_0 / 3) < tq)
					{
						lb->marquee->pos += lb->marquee->scrollback ? -1 : 1;
						lb->marquee->lasttime = tq;
					}
					if (lb->marquee->pos < 0) // reached beginning of string -> scroll forward
					{
						lb->marquee->pos = 0;
						lb->marquee->scrollback = 0;
					}
					if (lb->marquee->pos + lb->marquee->maxchars - 1 > item_len) // reached end of string -> scroll backward
					{
						lb->marquee->pos = item_len - lb->marquee->maxchars + 1;
						lb->marquee->scrollback = 1;
					}
					srcoffset = lb->marquee->pos;
				}
				snprintf(lb->marquee->text, lb->marquee->maxchars, "%s", lb->item[i] + srcoffset);
			}
			else
			{
				showstr = lb->item[i];
			}
			gr_string(canvas, mediumX_font, lb->box_x + fspacx(5), y, showstr);
		}
	}

	return lb->callback_handler(d_event{EVENT_NEWMENU_DRAW}, window_event_result::handled);
}

}

window_event_result listbox::event_handler(const d_event &event)
{
	if (const auto rval = callback_handler(event, window_event_result::ignored); rval != window_event_result::ignored)
		return rval;		// event handled

#if DXX_MAX_BUTTONS_PER_JOYSTICK
	if (joy_translate_menu_key(event))
		return window_event_result::handled;
#endif

	switch (event.type)
	{
		case EVENT_WINDOW_ACTIVATED:
			game_flush_inputs(Controls);
			event_toggle_focus(0);
			key_toggle_repeat(1);
			break;

		case EVENT_WINDOW_DEACTIVATED:
			//event_toggle_focus(1);	// No cursor recentering
			key_toggle_repeat(0);
			break;

		case EVENT_MOUSE_BUTTON_DOWN:
		case EVENT_MOUSE_BUTTON_UP:
			mouse_state = event.type == EVENT_MOUSE_BUTTON_DOWN;
			return listbox_mouse(event, this, event_mouse_get_button(event));
		case EVENT_KEY_COMMAND:
			return listbox_key_command(event, this);
		case EVENT_IDLE:
			if (!(Game_mode & GM_MULTI && Game_wind))
				timer_delay2(CGameArg.SysMaxFPS);
			return listbox_mouse(event, this, -1);
		case EVENT_WINDOW_DRAW:
			return listbox_draw(this);
		case EVENT_WINDOW_CLOSE:
			break;
		default:
			break;
	}
	return window_event_result::ignored;
}
