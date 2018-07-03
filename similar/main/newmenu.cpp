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
#if defined(DXX_BUILD_DESCENT_II)
#include "args.h"
#include "gamepal.h"
#endif

#if DXX_USE_OGL
#include "ogl_init.h"
#endif

#include "compiler-range_for.h"
#include "partial_range.h"

#define MAXDISPLAYABLEITEMS 14
#define MAXDISPLAYABLEITEMSTINY 21
#define MESSAGEBOX_TEXT_SIZE 2176  // How many characters in messagebox
#define MAX_TEXT_WIDTH FSPACX(120) // How many pixels wide a input box can be

struct newmenu : embed_window_pointer_t
{
	int				x,y,w,h;
	short			swidth, sheight;
	// with these we check if resolution or fonts have changed so menu structure can be recreated
	font_x_scale_proportion fntscalex;
	font_y_scale_proportion fntscaley;
	const char			*title;
	const char			*subtitle;
	unsigned		nitems;
	int				citem;
	newmenu_item	*items;
	int				(*subfunction)(newmenu *menu,const d_event &event, void *userdata);
	const char			*filename;
	int				tiny_mode;
	int			tabs_flag;
	int				scroll_offset, max_displayable;
	int				all_text;		//set true if all text items
	int				is_scroll_box;   // Is this a scrolling box? Set to false at init
	int				max_on_menu;
	int				mouse_state, dblclick_flag;
	int				*rval;			// Pointer to return value (for polling newmenus)
	void			*userdata;		// For whatever - like with window system
	partial_range_t<newmenu_item *> item_range()
	{
		return unchecked_partial_range(items, nitems);
	}
};

constexpr std::integral_constant<unsigned, NM_TYPE_INPUT> newmenu_item::input_specific_type::nm_type;
constexpr std::integral_constant<unsigned, NM_TYPE_RADIO> newmenu_item::radio_specific_type::nm_type;
constexpr std::integral_constant<unsigned, NM_TYPE_NUMBER> newmenu_item::number_specific_type::nm_type;
constexpr std::integral_constant<unsigned, NM_TYPE_INPUT_MENU> newmenu_item::imenu_specific_type::nm_type;
constexpr std::integral_constant<unsigned, NM_TYPE_SLIDER> newmenu_item::slider_specific_type::nm_type;

static grs_main_bitmap nm_background, nm_background1;
static grs_subbitmap_ptr nm_background_sub;

void newmenu_free_background()	{
	if (nm_background.bm_data)
	{
		nm_background_sub.reset();
	}
	nm_background.reset();
	nm_background1.reset();
}

namespace dsx {

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
	int pcx_error;

	if (filename != NULL)
	{
		if (nm_background1.bm_data == NULL)
		{
			pcx_error = pcx_read_bitmap( filename, nm_background1, gr_palette );
			Assert(pcx_error == PCX_ERROR_NONE);
			(void)pcx_error;
		}
		gr_palette_load( gr_palette );
		show_fullscr(canvas, nm_background1);
	}
#if defined(DXX_BUILD_DESCENT_II)
	strcpy(last_palette_loaded,"");		//force palette load next time
#endif
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
		int pcx_error;
		palette_array_t background_palette;
		pcx_error = pcx_read_bitmap(MENU_BACKGROUND_BITMAP, nm_background,background_palette);
		Assert(pcx_error == PCX_ERROR_NONE);
		(void)pcx_error;
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

	gr_settransblend(canvas, 14, GR_BLEND_NORMAL);
	{
		const uint8_t color = BM_XRGB(1, 1, 1);
	for (w=5*BGScaleX;w>0;w--)
			gr_urect(canvas, x2 - w, y1 + w * (BGScaleY / BGScaleX), x2 - w, y2 - w * (BGScaleY / BGScaleX), color);//right edge
	}
	{
		const uint8_t color = BM_XRGB(0, 0, 0);
	for (h=5*BGScaleY;h>0;h--)
			gr_urect(canvas, x1 + h * (BGScaleX / BGScaleY), y2 - h, x2 - h * (BGScaleX / BGScaleY), y2 - h, color);//bottom edge
	}
	gr_settransblend(canvas, GR_FADE_OFF, GR_BLEND_NORMAL);
}

// Draw a left justfied string
static void nm_string(grs_canvas &canvas, const int w1, int x, const int y, const char *const s, const int tabs_flag)
{
	if (!tabs_flag)
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
		gr_string(canvas, *canvas.cv_font, x, y, s1);
		if (p)
		{
			int w, h;
			++ p;
			gr_get_string_size(*canvas.cv_font, p, &w, &h, nullptr);
			gr_string(canvas, *canvas.cv_font, x + w1 - w, y, p, w, h);
		}
		return;
	}
	array<int, 6> XTabs = {{18, 90, 127, 165, 231, 256}};
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
		gr_get_string_size(*canvas.cv_font, measure, &tx, &th, nullptr);
		gr_string(canvas, *canvas.cv_font, x, y, measure, tx, th);
		x+=tx;
	}
}

// Draw a slider and it's string
static void nm_string_slider(grs_canvas &canvas, const int w1, const int x, const int y, char *const s)
{
	char *p,*s1;

	s1=NULL;

	p = strchr( s, '\t' );
	if (p)	{
		*p = '\0';
		s1 = p+1;
	}

	gr_string(canvas, *canvas.cv_font, x, y, s);

	if (p)	{
		int w, h;
		gr_get_string_size(*canvas.cv_font, s1, &w, &h, nullptr);
		gr_string(canvas, *canvas.cv_font, x + w1 - w, y, s1, w, h);

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
static void nm_rstring(grs_canvas &canvas, int w1, int x, const int y, const char *const s)
{
	int w, h;
	gr_get_string_size(*canvas.cv_font, s, &w, &h, nullptr);
	x -= FSPACX(3);

	if (w1 == 0) w1 = w;
	gr_string(canvas, *canvas.cv_font, x - w, y, s, w, h);
}

static void nm_string_inputbox(grs_canvas &canvas, const int w, const int x, const int y, const char *text, const int current)
{
	int w1;

	// even with variable char widths and a box that goes over the whole screen, we maybe never get more than 75 chars on the line
	if (strlen(text)>75)
		text+=strlen(text)-75;
	while( *text )	{
		gr_get_string_size(*canvas.cv_font, text, &w1, nullptr, nullptr);
		if ( w1 > w-FSPACX(10) )
			text++;
		else
			break;
	}
	if ( *text == 0 )
		w1 = 0;

	nm_string_black(canvas, w, x, y, text);

	if ( current && timer_query() & 0x8000 )
		gr_string(canvas, *canvas.cv_font, x + w1, y, CURSOR_STRING);
}

static void draw_item(grs_canvas &canvas, newmenu_item *item, int is_current, int tiny, int tabs_flag, int scroll_offset)
{
	if (tiny)
	{
		int r, g, b;
		if (item->text[0]=='\t')
			r = g = b = 63;
		else if (is_current)
			r = 57, g = 49, b = 20;
		else
			r = g = 29, b = 47;
		gr_set_fontcolor(canvas, gr_find_closest_color_current(r, g, b), -1);
	}
	else
	{
		gr_set_curfont(canvas, is_current?MEDIUM2_FONT:MEDIUM1_FONT);
        }

	const int line_spacing = static_cast<int>(LINE_SPACING(*canvas.cv_font, *GAME_FONT));
	switch( item->type )	{
		case NM_TYPE_SLIDER:
		{
			int i;
			auto &slider = item->slider();
			if (item->value < slider.min_value)
				item->value = slider.min_value;
			if (item->value > slider.max_value)
				item->value = slider.max_value;
			i = snprintf(item->saved_text.data(), item->saved_text.size(), "%s\t%s", item->text, SLIDER_LEFT);
			for (uint_fast32_t j = (slider.max_value - slider.min_value + 1); j--;)
			{
				i += snprintf(item->saved_text.data() + i, item->saved_text.size() - i, "%s", SLIDER_MIDDLE);
			}
			i += snprintf(item->saved_text.data() + i, item->saved_text.size() - i, "%s", SLIDER_RIGHT);
			item->saved_text[item->value+1+strlen(item->text)+1] = SLIDER_MARKER[0];
			nm_string_slider(canvas, item->w, item->x, item->y - (line_spacing * scroll_offset), item->saved_text.data());
		}
			break;
		case NM_TYPE_INPUT_MENU:
			if (item->imenu().group == 0)
			{
			case NM_TYPE_TEXT:
			case NM_TYPE_MENU:
				nm_string(canvas, item->w, item->x, item->y - (line_spacing * scroll_offset), item->text, tabs_flag);
				break;
			}
		case NM_TYPE_INPUT:
			nm_string_inputbox(canvas, item->w, item->x, item->y - (line_spacing * scroll_offset), item->text, is_current);
			break;
		case NM_TYPE_CHECK:
			nm_string(canvas, item->w, item->x, item->y - (line_spacing * scroll_offset), item->text, tabs_flag);
			nm_rstring(canvas, item->right_offset, item->x, item->y - (line_spacing * scroll_offset), item->value ? CHECKED_CHECK_BOX : NORMAL_CHECK_BOX);
			break;
		case NM_TYPE_RADIO:
			nm_string(canvas, item->w, item->x, item->y - (line_spacing * scroll_offset), item->text, tabs_flag);
			nm_rstring(canvas, item->right_offset, item->x, item->y - (line_spacing * scroll_offset), item->value ? CHECKED_RADIO_BOX : NORMAL_RADIO_BOX);
			break;
		case NM_TYPE_NUMBER:
		{
			char text[sizeof("-2147483647")];
			auto &number = item->number();
			if (item->value < number.min_value)
				item->value = number.min_value;
			if (item->value > number.max_value)
				item->value = number.max_value;
			nm_string(canvas, item->w, item->x, item->y - (line_spacing * scroll_offset), item->text, tabs_flag);
			snprintf(text, sizeof(text), "%d", item->value );
			nm_rstring(canvas, item->right_offset, item->x, item->y - (line_spacing * scroll_offset), text);
		}
			break;
	}
}

const char *Newmenu_allowed_chars=NULL;

//returns true if char is allowed
static bool char_disallowed(char c)
{
	const char *p = Newmenu_allowed_chars;
	if (!p)
		return false;
	for (uint8_t a, b; (a = p[0]) && (b = p[1]); p += 2)
	{
		if (likely(c >= a && c <= b))
			return false;
	}
	return true;
}

static bool char_allowed(char c)
{
	return !char_disallowed(c);
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

int newmenu_do2(const char *const title, const char *const subtitle, const uint_fast32_t nitems, newmenu_item *const item, const newmenu_subfunction subfunction, void *const userdata, const int citem, const char *const filename)
{
	newmenu *menu;
	bool exists = true;
	int rval = -1;

	menu = newmenu_do3( title, subtitle, nitems, item, subfunction, userdata, citem, filename );

	if (!menu)
		return -1;
	menu->rval = &rval;

	// Track to see when the window is freed
	// Doing this way in case another window is opened on top without its own polling loop
	menu->wind->track(&exists);

	// newmenu_do2 and simpler get their own event loop
	// This is so the caller doesn't have to provide a callback that responds to EVENT_NEWMENU_SELECTED
	while (exists)
		event_process();

	return rval;
}

static void swap_menu_item_entries(newmenu_item &a, newmenu_item &b)
{
	using std::swap;
	swap(a.text, b.text);
	swap(a.value, b.value);
}

template <typename T>
static void move_menu_item_entry(T &&t, newmenu_item *const items, uint_fast32_t citem, uint_fast32_t boundary)
{
	if (citem == boundary)
		return;
	auto a = &items[citem];
	auto selected = std::make_pair(a->text, a->value);
	for (; citem != boundary;)
	{
		citem = t(citem, 1);
		auto &b = items[citem];
		a->text = b.text;
		a->value = b.value;
		a = &b;
	}
	a->text = selected.first;
	a->value = selected.second;
}

static int newmenu_save_selection_key(newmenu *menu, const d_event &event)
{
	auto k = event_key_get(event);
	switch(k)
	{
		case KEY_SHIFTED+KEY_UP:
			if (menu->citem > 0)
			{
				auto &a = menu->items[menu->citem];
				auto &b = menu->items[-- menu->citem];
				swap_menu_item_entries(a, b);
			}
			break;
		case KEY_SHIFTED+KEY_DOWN:
			if (menu->citem < (menu->nitems - 1))
			{
				auto &a = menu->items[menu->citem];
				auto &b = menu->items[++ menu->citem];
				swap_menu_item_entries(a, b);
			}
			break;
		case KEY_PAGEUP + KEY_SHIFTED:
			move_menu_item_entry(std::minus<uint_fast32_t>(), menu->items, menu->citem, 0);
			break;
		case KEY_PAGEDOWN + KEY_SHIFTED:
			move_menu_item_entry(std::plus<uint_fast32_t>(), menu->items, menu->citem, menu->nitems - 1);
			break;
	}
	return 0;
}

static int newmenu_save_selection_handler(newmenu *menu, const d_event &event, const unused_newmenu_userdata_t *)
{
	switch(event.type)
	{
		case EVENT_KEY_COMMAND:
			return newmenu_save_selection_key(menu, event);
		default:
			break;
	}
	return 0;
}

// Basically the same as do2 but sets reorderitems flag for weapon priority menu a bit redundant to get lose of a global variable but oh well...
void newmenu_doreorder( const char * title, const char * subtitle, uint_fast32_t nitems, newmenu_item * item)
{
	newmenu_do2(title, subtitle, nitems, item, newmenu_save_selection_handler, unused_newmenu_userdata, 0, nullptr);
}

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

namespace {

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

}

template <typename S, typename O>
static void update_menu_position(newmenu &menu, newmenu_item *const stop, int_fast32_t amount, S step, O overflow)
{
	auto icitem = menu.citem;
	auto pcitem = &menu.items[icitem];
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
	} while (-- amount > 0 || pcitem->type == NM_TYPE_TEXT);
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
		if (menu->max_on_menu+menu->scroll_offset > menu->nitems)
			menu->scroll_offset = menu->nitems-menu->max_on_menu;
		return;
	}
	const auto &range = menu->item_range();
	const auto predicate = [](const newmenu_item &n) {
		return n.type != NM_TYPE_TEXT;
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
	auto citem = &menu->items[menu->citem];
	if (citem == last && amount == 1) // if citem == last item and we want to go down one step, go to first item
	{
		newmenu_scroll(menu, -menu->nitems);
		return;
	}
	if (citem == first && amount == -1) // if citem == first item and we want to go up one step, go to last item
	{
		newmenu_scroll(menu, menu->nitems);
		return;
	}

	if (amount > 0) // down the list
	{
		const auto overflow = [menu](int icitem) {
			return icitem + 4 >= menu->max_on_menu + menu->scroll_offset && menu->scroll_offset < menu->nitems - menu->max_on_menu;
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
	range_for (auto &r, menu.item_range())
	{
		if (&r != &citem && r.type == NM_TYPE_RADIO && r.radio().group == cg)
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

static window_event_result newmenu_mouse(window *wind,const d_event &event, newmenu *menu, int button)
{
	int old_choice, mx=0, my=0, mz=0, x1 = 0, x2, y1, y2, changed = 0;
	grs_canvas &menu_canvas = window_get_canvas(*wind);
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
					x1 = canvas.cv_bitmap.bm_x + menu->items[i].x - fspacx(13) /*- menu->items[i].right_offset - 6*/;
					x2 = x1 + menu->items[i].w + fspacx(13);
					y1 = canvas.cv_bitmap.bm_y + menu->items[i].y - (line_spacing * menu->scroll_offset);
					y2 = y1 + menu->items[i].h;
					if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2))) {
						menu->citem = i;
						auto &citem = menu->items[menu->citem];
						switch (citem.type)
						{
							case NM_TYPE_CHECK:
								citem.value = !citem.value;
								changed = 1;
								break;
							case NM_TYPE_RADIO:
								changed = nm_trigger_radio_button(*menu, citem);
								break;
							case NM_TYPE_TEXT:
								menu->citem=old_choice;
								menu->mouse_state=0;
								break;
							case NM_TYPE_MENU:
							case NM_TYPE_INPUT:
							case NM_TYPE_NUMBER:
							case NM_TYPE_INPUT_MENU:
							case NM_TYPE_SLIDER:
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
						y1 = canvas.cv_bitmap.bm_y + menu->items[menu->scroll_offset].y - (line_spacing * menu->scroll_offset);
						x2 = x1 + arrow_width;
						y2 = y1 + arrow_height;
						if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) && ScrollAllow) {
							newmenu_scroll(menu, -1);
						}
					}
					if (menu->scroll_offset+menu->max_displayable<menu->nitems) {
						int arrow_width, arrow_height;
						gr_get_string_size(*canvas.cv_font, DOWN_ARROW_MARKER(*canvas.cv_font, *GAME_FONT), &arrow_width, &arrow_height, nullptr);
						x1 = canvas.cv_bitmap.bm_x + BORDERX - fspacx(12);
						y1 = canvas.cv_bitmap.bm_y + menu->items[menu->scroll_offset + menu->max_displayable - 1].y - (line_spacing * menu->scroll_offset);
						x2 = x1 + arrow_width;
						y2 = y1 + arrow_height;
						if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) && ScrollAllow) {
							newmenu_scroll(menu, 1);
						}
					}
				}

				for (int i = menu->scroll_offset; i < menu->max_on_menu + menu->scroll_offset; ++i)
				{
					x1 = canvas.cv_bitmap.bm_x + menu->items[i].x - fspacx(13);
					x2 = x1 + menu->items[i].w + fspacx(13);
					y1 = canvas.cv_bitmap.bm_y + menu->items[i].y - (line_spacing * menu->scroll_offset);
					y2 = y1 + menu->items[i].h;

					if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) && (menu->items[i].type != NM_TYPE_TEXT) ) {
						menu->citem = i;
						auto &citem = menu->items[menu->citem];
						if (citem.type == NM_TYPE_SLIDER)
						{
							char slider_text[NM_MAX_TEXT_LEN+1], *p, *s1;

							strcpy(slider_text, citem.saved_text);
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
						if (citem.type == NM_TYPE_INPUT && menu->citem != old_choice)
							citem.value = -1;
						if ((old_choice>-1) && (menu->items[old_choice].type==NM_TYPE_INPUT_MENU) && (old_choice!=menu->citem))	{
							menu->items[old_choice].imenu().group = 0;
							strcpy(menu->items[old_choice].text, menu->items[old_choice].saved_text );
							menu->items[old_choice].value = -1;
						}
						break;
					}
				}
			}

			if ((event.type == EVENT_MOUSE_BUTTON_UP) && !menu->all_text && (menu->citem != -1) && (menu->items[menu->citem].type == NM_TYPE_MENU) )
			{
				mouse_get_pos(&mx, &my, &mz);
				const int line_spacing = static_cast<int>(LINE_SPACING(*grd_curcanv->cv_font, *GAME_FONT));
				for (int i = menu->scroll_offset; i < menu->max_on_menu + menu->scroll_offset; ++i)
				{
					x1 = canvas.cv_bitmap.bm_x + menu->items[i].x - fspacx(13);
					x2 = x1 + menu->items[i].w + fspacx(13);
					y1 = canvas.cv_bitmap.bm_y + menu->items[i].y - (line_spacing * menu->scroll_offset);
					y2 = y1 + menu->items[i].h;
					if (((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2))) {
							// Tell callback, allow staying in menu
							const d_select_event selected{menu->citem};
							if (menu->subfunction && (*menu->subfunction)(menu, selected, menu->userdata))
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
				auto &citem = menu->items[menu->citem];
				if (citem.type == NM_TYPE_INPUT_MENU && citem.imenu().group == 0)
				{
					citem.imenu().group = 1;
					if (!d_stricmp(citem.saved_text, TXT_EMPTY))
					{
						citem.text[0] = 0;
						citem.value = -1;
					} else {
						strip_end_whitespace(citem.text);
					}
				}
			}

			gr_set_current_canvas(save_canvas);

			if (changed && menu->subfunction)
			{
				(*menu->subfunction)(menu, d_change_event{menu->citem}, menu->userdata);
			}
			break;
		}
		case MBTN_RIGHT:
			if (menu->mouse_state)
			{
				if (!(menu->citem > -1))
					return window_event_result::close;
				auto &citem = menu->items[menu->citem];
				if (citem.type == NM_TYPE_INPUT_MENU && citem.imenu().group == 1)
				{
					citem.imenu().group = 0;
					strcpy(citem.text, citem.saved_text);
					citem.value = -1;
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

static window_event_result newmenu_key_command(window *, const d_event &event, newmenu *menu)
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
	auto &citem = menu->items[menu->citem];

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
			newmenu_scroll(menu, k == KEY_PAGEUP ? -10 : -1);
			if (citem.type == NM_TYPE_INPUT && menu->citem != old_choice)
				citem.value = -1;
			if ((old_choice>-1) && (menu->items[old_choice].type==NM_TYPE_INPUT_MENU) && (old_choice!=menu->citem))	{
				menu->items[old_choice].imenu().group=0;
				strcpy(menu->items[old_choice].text, menu->items[old_choice].saved_text );
				menu->items[old_choice].value = -1;
			}
			break;
		case KEY_TAB:
		case KEY_DOWN:
		case KEY_PAGEDOWN:
		case KEY_PAD2:
			newmenu_scroll(menu, k == KEY_PAGEDOWN ? 10 : 1);
			if (citem.type == NM_TYPE_INPUT && menu->citem != old_choice)
				citem.value = -1;
			if ( (old_choice>-1) && (menu->items[old_choice].type==NM_TYPE_INPUT_MENU) && (old_choice!=menu->citem))	{
				menu->items[old_choice].imenu().group=0;
				strcpy(menu->items[old_choice].text, menu->items[old_choice].saved_text );
				menu->items[old_choice].value = -1;
			}
			break;
		case KEY_SPACEBAR:
			if ( menu->citem > -1 )	{
				switch (citem.type)
				{
					case NM_TYPE_TEXT:
					case NM_TYPE_NUMBER:
					case NM_TYPE_SLIDER:
					case NM_TYPE_MENU:
					case NM_TYPE_INPUT:
					case NM_TYPE_INPUT_MENU:
						break;
					case NM_TYPE_CHECK:
						citem.value = !citem.value;
						changed = 1;
						break;
					case NM_TYPE_RADIO:
						changed = nm_trigger_radio_button(*menu, citem);
						break;
				}
			}
			break;

		case KEY_ENTER:
		case KEY_PADENTER:
			if (menu->citem > -1 && citem.type == NM_TYPE_INPUT_MENU && citem.imenu().group == 0)
			{
				citem.imenu().group = 1;
				if (!d_stricmp(citem.saved_text, TXT_EMPTY))
				{
					citem.text[0] = 0;
					citem.value = -1;
				} else {
					strip_end_whitespace(citem.text);
				}
			} else
			{
				if (citem.type == NM_TYPE_INPUT_MENU)
					citem.imenu().group = 0;	// go out of editing mode

				// Tell callback, allow staying in menu
				const d_select_event selected{menu->citem};
				if (menu->subfunction && (*menu->subfunction)(menu, selected, menu->userdata))
					return window_event_result::handled;

				if (menu->rval)
					*menu->rval = menu->citem;
				return window_event_result::close;
			}
			break;

		case KEY_ESC:
			if (menu->citem > -1 && citem.type == NM_TYPE_INPUT_MENU && citem.imenu().group == 1)
			{
				citem.imenu().group = 0;
				strcpy(citem.text, citem.saved_text);
				citem.value = -1;
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
		if ((citem.type == NM_TYPE_INPUT || (citem.type == NM_TYPE_INPUT_MENU && citem.imenu().group == 1)) && (old_choice == menu->citem) )	{
			if ( k==KEY_LEFT || k==KEY_BACKSP || k==KEY_PAD4 )	{
				if (citem.value == -1) citem.value = strlen(citem.text);
				if (citem.value > 0)
					citem.value--;
				citem.text[citem.value] = 0;

				if (citem.type == NM_TYPE_INPUT)
					changed = 1;
				rval = window_event_result::handled;
			}
			else if (citem.value < citem.input_or_menu()->text_len)
			{
				auto ascii = key_ascii();
				if (ascii < 255)
				{
					if (citem.value == -1) {
						citem.value = 0;
					}
					if (char_allowed(ascii) || (ascii == ' ' && char_allowed(ascii = '_')))
					{
						citem.text[citem.value++] = ascii;
						citem.text[citem.value] = 0;

						if (citem.type == NM_TYPE_INPUT)
							changed = 1;
					}
				}
			}
		}
		else if ((citem.type != NM_TYPE_INPUT) && (citem.type != NM_TYPE_INPUT_MENU) )
		{
			auto ascii = key_ascii();
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

	if (changed && menu->subfunction)
	{
		(*menu->subfunction)(menu, d_change_event{menu->citem}, menu->userdata);
	}

	return rval;
}

namespace dsx {
static void newmenu_create_structure( newmenu *menu )
{
	int aw, tw, th, twidth,right_offset;
	int nmenus, nothers;
	grs_canvas &save_canvas = *grd_curcanv;
	gr_set_default_canvas();
	auto &canvas = *grd_curcanv;

	tw = th = 0;

	if ( menu->title )	{
		int string_width, string_height;
		auto &huge_font = *HUGE_FONT;
		gr_get_string_size(huge_font, menu->title, &string_width, &string_height, nullptr);
		tw = string_width;
		th = string_height;
	}
	if ( menu->subtitle )	{
		int string_width, string_height;
		auto &medium3_font = *MEDIUM3_FONT;
		gr_get_string_size(medium3_font, menu->subtitle, &string_width, &string_height, nullptr);
		if (string_width > tw )
			tw = string_width;
		th += string_height;
	}

	th += FSPACY(5);		//put some space between titles & body

	auto &cv_font = *(menu->tiny_mode ? GAME_FONT : MEDIUM1_FONT).get();

	menu->w = aw = 0;
	menu->h = th;
	nmenus = nothers = 0;

	const auto &&fspacx = FSPACX();
	const auto &&fspacy = FSPACY();
	// Find menu height & width (store in w,h)
	range_for (auto &i, menu->item_range())
	{
		i.y = menu->h;
		int string_width, string_height, average_width;
		gr_get_string_size(cv_font, i.text, &string_width, &string_height, &average_width);
		i.right_offset = 0;

		i.saved_text[0] = '\0';

		if (i.type == NM_TYPE_SLIDER)
		{
			int index,w1;
			nothers++;
			index = snprintf (i.saved_text.data(), i.saved_text.size(), "%s", SLIDER_LEFT);
			auto &slider = i.slider();
			for (uint_fast32_t j = (slider.max_value - slider.min_value + 1); j--;)
			{
				index += snprintf(i.saved_text.data() + index, i.saved_text.size() - index, "%s", SLIDER_MIDDLE);
			}
			index += snprintf(i.saved_text.data() + index, i.saved_text.size() - index, "%s", SLIDER_RIGHT);
			gr_get_string_size(cv_font, i.saved_text.data(), &w1, nullptr, nullptr);
			string_width += w1 + aw;
		}

		if (i.type == NM_TYPE_MENU)
		{
			nmenus++;
		}

		if (i.type == NM_TYPE_CHECK)
		{
			int w1;
			nothers++;
			gr_get_string_size(cv_font, NORMAL_CHECK_BOX, &w1, nullptr, nullptr);
			i.right_offset = w1;
			gr_get_string_size(cv_font, CHECKED_CHECK_BOX, &w1, nullptr, nullptr);
			if (w1 > i.right_offset)
				i.right_offset = w1;
		}

		if (i.type == NM_TYPE_RADIO)
		{
			int w1;
			nothers++;
			gr_get_string_size(cv_font, NORMAL_RADIO_BOX, &w1, nullptr, nullptr);
			i.right_offset = w1;
			gr_get_string_size(cv_font, CHECKED_RADIO_BOX, &w1, nullptr, nullptr);
			if (w1 > i.right_offset)
				i.right_offset = w1;
		}

		if (i.type == NM_TYPE_NUMBER)
		{
			int w1;
			char test_text[20];
			nothers++;
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
			i.saved_text.copy_if(i.text);
			const auto text_len = input_or_menu->text_len;
			string_width = text_len * fspacx(8) + text_len;
			if (i.type == NM_TYPE_INPUT && string_width > MAX_TEXT_WIDTH)
				string_width = MAX_TEXT_WIDTH;

			i.value = -1;
			if (i.type == NM_TYPE_INPUT_MENU)
			{
				i.imenu().group = 0;
				nmenus++;
			}
			else
			{
				nothers++;
			}
		}

		i.w = string_width;
		i.h = string_height;

		if ( string_width > menu->w )
			menu->w = string_width;		// Save maximum width
		if ( average_width > aw )
			aw = average_width;
		menu->h += string_height + fspacy(1);		// Find the height of all strings
	}

	if (menu->nitems > menu->max_on_menu)
	{
		menu->is_scroll_box=1;
		menu->h = th + (LINE_SPACING(cv_font, *GAME_FONT) * menu->max_on_menu);
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
		menu->max_on_menu = menu->nitems;
	}

	right_offset=0;

	range_for (auto &i, menu->item_range())
	{
		i.w = menu->w;
		if (right_offset < i.right_offset)
			right_offset = i.right_offset;
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

	menu->x = (canvas.cv_bitmap.bm_w - menu->w) / 2;
	menu->y = (canvas.cv_bitmap.bm_h - menu->h) / 2;

	if ( menu->x < 0 ) menu->x = 0;
	if ( menu->y < 0 ) menu->y = 0;

	nm_draw_background1(canvas, menu->filename);

	// Update all item's x & y values.
	range_for (auto &i, menu->item_range())
	{
		i.x = BORDERX + twidth + right_offset;
		i.y += BORDERY;
		if (i.type == NM_TYPE_RADIO) {
			// find first marked one
			newmenu_item *fm = nullptr;
			range_for (auto &j, menu->item_range())
			{
				if (j.type == NM_TYPE_RADIO && j.radio().group == i.radio().group) {
					if (!fm && j.value)
						fm = &j;
					j.value = 0;
				}
			}
			(fm ? *fm : i).value = 1;
		}
	}

	if (menu->citem != -1)
	{
		if (menu->citem < 0 ) menu->citem = 0;
		if (menu->citem > menu->nitems-1 ) menu->citem = menu->nitems-1;

		menu->dblclick_flag = 1;
		uint_fast32_t i = 0;
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
	gr_set_current_canvas(save_canvas);
}

static window_event_result newmenu_draw(window *wind, newmenu *menu)
{
	grs_canvas &menu_canvas = window_get_canvas(*wind);
	grs_canvas &save_canvas = *grd_curcanv;
	int th = 0, ty, sx, sy;
	int i;

	if (menu->swidth != SWIDTH || menu->sheight != SHEIGHT || menu->fntscalex != FNTScaleX || menu->fntscaley != FNTScaleY)
	{
		newmenu_create_structure ( menu );
		{
			gr_init_sub_canvas(menu_canvas, grd_curscreen->sc_canvas, menu->x, menu->y, menu->w, menu->h);
		}
	}

	gr_set_default_canvas();
	nm_draw_background1(*grd_curcanv, menu->filename);
	if (menu->filename == NULL)
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

	gr_set_curfont(*grd_curcanv, menu->tiny_mode?GAME_FONT:MEDIUM1_FONT);

	// Redraw everything...
	for (i=menu->scroll_offset; i<menu->max_displayable+menu->scroll_offset; i++ )
	{
		draw_item(*grd_curcanv, &menu->items[i], (i==menu->citem && !menu->all_text),menu->tiny_mode, menu->tabs_flag, menu->scroll_offset);
	}

	if (menu->is_scroll_box)
	{
		auto &cv_font = *(menu->tiny_mode ? GAME_FONT : MEDIUM2_FONT);

		const int line_spacing = static_cast<int>(LINE_SPACING(cv_font, *GAME_FONT));
		sy = menu->items[menu->scroll_offset].y - (line_spacing * menu->scroll_offset);
		const auto &&fspacx = FSPACX();
		sx = BORDERX - fspacx(12);

		gr_string(*grd_curcanv, cv_font, sx, sy, menu->scroll_offset ? UP_ARROW_MARKER(cv_font, *GAME_FONT) : "  ");

		sy = menu->items[menu->scroll_offset + menu->max_displayable - 1].y - (line_spacing * menu->scroll_offset);
		sx = BORDERX - fspacx(12);

		gr_string(*grd_curcanv, cv_font, sx, sy, (menu->scroll_offset + menu->max_displayable < menu->nitems) ? DOWN_ARROW_MARKER(*grd_curcanv->cv_font, *GAME_FONT) : "  ");
	}

		if (menu->subfunction)
			(*menu->subfunction)(menu, d_event{EVENT_NEWMENU_DRAW}, menu->userdata);

	gr_set_current_canvas(save_canvas);

	return window_event_result::handled;
}

static window_event_result newmenu_handler(window *wind,const d_event &event, newmenu *menu)
{
	if (menu->subfunction)
	{
		int rval = (*menu->subfunction)(menu, event, menu->userdata);

#if 0	// No current instances of the subfunction closing the window itself (which is preferred)
		// Enable when all subfunctions return a window_event_result
		if (rval == window_event_result::deleted)
			return rval;	// some subfunction closed the window: bail!
#endif

		if (rval)
		{
			if (rval < -1)
			{
				if (menu->rval)
					*menu->rval = rval;
				return window_event_result::close;
			}

			return window_event_result::handled;		// event handled
		}
	}

	switch (event.type)
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
			menu->mouse_state = event.type == EVENT_MOUSE_BUTTON_DOWN;
			return newmenu_mouse(wind, event, menu, button);
		}

		case EVENT_KEY_COMMAND:
			return newmenu_key_command(wind, event, menu);
		case EVENT_IDLE:
			if (!(Game_mode & GM_MULTI && Game_wind))
				timer_delay2(CGameArg.SysMaxFPS);
			break;
		case EVENT_WINDOW_DRAW:
			return newmenu_draw(wind, menu);
		case EVENT_WINDOW_CLOSE:
			delete menu;
			break;

		default:
			break;
	}
	return window_event_result::ignored;
}

newmenu *newmenu_do4( const char * title, const char * subtitle, uint_fast32_t nitems, newmenu_item * item, newmenu_subfunction subfunction, void *userdata, int citem, const char * filename, int TinyMode, int TabsFlag )
{
	newmenu *menu = new newmenu{};
	menu->citem = citem;
	menu->scroll_offset = 0;
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
	menu->rval = NULL;		// Default to not returning a value - respond to EVENT_NEWMENU_SELECTED instead
	menu->userdata = userdata;

	newmenu_free_background();

	if (nitems < 1 )
	{
		delete menu;
		return NULL;
	}

	menu->max_displayable=nitems;

	//set_screen_mode(SCREEN_MENU);	//hafta set the screen mode here or fonts might get changed/freed up if screen res changes

	newmenu_create_structure(menu);

	// Create the basic window
	const auto wind = window_create(grd_curscreen->sc_canvas, menu->x, menu->y, menu->w, menu->h, newmenu_handler, menu);
	
	if (!wind)
	{
		delete menu;
		return NULL;
	}
	return menu;
}
}

int (vnm_messagebox_aN)(const char *title, const nm_messagebox_tie &tie, const char *format, ...)
{
	va_list args;
	char nm_text[MESSAGEBOX_TEXT_SIZE];
	va_start(args, format);
	vsnprintf(nm_text,sizeof(nm_text),format,args);
	va_end(args);
	return nm_messagebox_str(title, tie, nm_text);
}

int nm_messagebox_str(const char *title, const nm_messagebox_tie &tie, const char *str)
{
	newmenu_item items[nm_messagebox_tie::maximum_arity];
	for (unsigned i=0; i < tie.count(); ++i) {
		const char *s = tie.string(i);
		nm_set_item_menu(items[i], s);
	}
	return newmenu_do( title, str, tie.count(), items, unused_newmenu_subfunction, unused_newmenu_userdata );
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

struct listbox : embed_window_pointer_t
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
			auto pf = make_unique<fix64[]>(1 + (max_bytes / sizeof(fix64)));
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
	const char *title;
	const char **item;
	int allow_abort_flag;
	listbox_subfunction_t<void> listbox_callback;
	unsigned nitems;
	unsigned items_on_screen;
	int citem, first_item;
	int box_w, height, box_x, box_y, title_height;
	short swidth, sheight;
	// with these we check if resolution or fonts have changed so listbox structure can be recreated
	font_x_scale_proportion fntscalex;
	font_y_scale_proportion fntscaley;
	int mouse_state;
	marquee::ptr marquee;
	void *userdata;
};

window *listbox_get_window(listbox *const lb)
{
	return lb->wind;
}

const char **listbox_get_items(listbox *lb)
{
	return lb->item;
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

static void update_scroll_position(listbox *lb)
{
	if (lb->citem<0)
		lb->citem = 0;

	if (lb->citem>=lb->nitems)
		lb->citem = lb->nitems-1;

	if (lb->citem< lb->first_item)
		lb->first_item = lb->citem;

	if (lb->citem >= lb->items_on_screen)
	{
		if (lb->first_item <= lb->citem - lb->items_on_screen)
			lb->first_item = lb->citem - lb->items_on_screen + 1;
	}

	if (lb->nitems <= lb->items_on_screen)
		lb->first_item = 0;

	if (lb->nitems >= lb->items_on_screen)
	{
		if (lb->first_item > lb->nitems - lb->items_on_screen)
			lb->first_item = lb->nitems - lb->items_on_screen;
	}
	if (lb->first_item < 0 ) lb->first_item = 0;
}

static window_event_result listbox_mouse(window *, const d_event &event, listbox *lb, int button)
{
	int mx, my, mz, x1, x2, y1, y2;

	switch (button)
	{
		case MBTN_LEFT:
		{
			if (lb->mouse_state)
			{
				mouse_get_pos(&mx, &my, &mz);
				const auto &&line_spacing = LINE_SPACING(*grd_curcanv->cv_font, *GAME_FONT);
				for (int i = lb->first_item; i < lb->first_item + lb->items_on_screen; ++i)
				{
					if (i >= lb->nitems)
						break;
					int h;
					gr_get_string_size(*grd_curcanv->cv_font, lb->item[i], nullptr, &h, nullptr);
					x1 = lb->box_x;
					x2 = lb->box_x + lb->box_w;
					y1 = (i - lb->first_item) * line_spacing + lb->box_y;
					y2 = y1+h;
					if ( ((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) ) {
						lb->citem = i;
						return window_event_result::handled;
					}
				}
			}
			else if (event.type == EVENT_MOUSE_BUTTON_UP)
			{
				int h;

				if (lb->citem < 0)
					return window_event_result::ignored;

				mouse_get_pos(&mx, &my, &mz);
				gr_get_string_size(*grd_curcanv->cv_font, lb->item[lb->citem], nullptr, &h, nullptr);
				x1 = lb->box_x;
				x2 = lb->box_x + lb->box_w;
				y1 = (lb->citem - lb->first_item) * LINE_SPACING(*grd_curcanv->cv_font, *GAME_FONT) + lb->box_y;
				y2 = y1+h;
				if ( ((mx > x1) && (mx < x2)) && ((my > y1) && (my < y2)) )
				{
					// Tell callback, if it wants to close it will return window_event_result::close
					const d_select_event selected{lb->citem};
					if (lb->listbox_callback)
						return (*lb->listbox_callback)(lb, selected, lb->userdata);
					return window_event_result::close;
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

	return window_event_result::ignored;
}

static window_event_result listbox_key_command(window *, const d_event &event, listbox *lb)
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
				if (lb->listbox_callback)
					return (*lb->listbox_callback)(lb, selected, lb->userdata);
			}
			return window_event_result::close;
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
	update_scroll_position(lb);
	return rval;
}

static void listbox_create_structure( listbox *lb)
{
	gr_set_default_canvas();
	auto &canvas = *grd_curcanv;

	auto &medium3_font = *MEDIUM3_FONT;

	lb->box_w = 0;
	const auto &&fspacx = FSPACX();
	const auto &&fspacx10 = fspacx(10);
	const unsigned max_box_width = SWIDTH - (BORDERX * 2);
	unsigned marquee_maxchars = UINT_MAX;
	range_for (const auto i, unchecked_partial_range(lb->item, lb->nitems))
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
		if (lb->box_w < w)
			lb->box_w = w;
	}

	{
		int w, h;
		gr_get_string_size(medium3_font, lb->title, &w, &h, nullptr);
		if ( w > lb->box_w )
			lb->box_w = w;
		lb->title_height = h+FSPACY(5);
	}

	// The box is bigger than we can fit on the screen since at least one string is too long. Check how many chars we can fit on the screen (at least only - MEDIUM*_FONT is variable font!) so we can make a marquee-like effect.
	if (marquee_maxchars != UINT_MAX)
	{
		lb->box_w = max_box_width;
		lb->marquee = listbox::marquee::allocate(marquee_maxchars);
		lb->marquee->lasttime = timer_query();
	}

	const auto &&line_spacing = LINE_SPACING(medium3_font, *GAME_FONT);
	const unsigned bordery2 = BORDERY * 2;
	const auto items_on_screen = std::max<unsigned>(
		std::min<unsigned>(((canvas.cv_bitmap.bm_h - bordery2 - lb->title_height) / line_spacing) - 2, lb->nitems),
		LB_ITEMS_ON_SCREEN);
	lb->items_on_screen = items_on_screen;
	lb->height = line_spacing * items_on_screen;
	lb->box_x = (canvas.cv_bitmap.bm_w - lb->box_w) / 2;
	lb->box_y = (canvas.cv_bitmap.bm_h - (lb->height + lb->title_height)) / 2 + lb->title_height;
	if (lb->box_y < bordery2)
		lb->box_y = bordery2;

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

static window_event_result listbox_draw(window *, listbox *lb)
{
	if (lb->swidth != SWIDTH || lb->sheight != SHEIGHT || lb->fntscalex != FNTScaleX || lb->fntscaley != FNTScaleY)
		listbox_create_structure ( lb );

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

		if ( lb->listbox_callback )
			return (*lb->listbox_callback)(lb, d_event{EVENT_NEWMENU_DRAW}, lb->userdata);
	return window_event_result::handled;
}

static window_event_result listbox_handler(window *wind,const d_event &event, listbox *lb)
{
	if (lb->listbox_callback)
	{
		auto rval = (*lb->listbox_callback)(lb, event, lb->userdata);
		if (rval != window_event_result::ignored)
			return rval;		// event handled
	}

	switch (event.type)
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
			lb->mouse_state = event.type == EVENT_MOUSE_BUTTON_DOWN;
			return listbox_mouse(wind, event, lb, event_mouse_get_button(event));
		case EVENT_KEY_COMMAND:
			return listbox_key_command(wind, event, lb);
		case EVENT_IDLE:
			if (!(Game_mode & GM_MULTI && Game_wind))
				timer_delay2(CGameArg.SysMaxFPS);
			return listbox_mouse(wind, event, lb, -1);
		case EVENT_WINDOW_DRAW:
			return listbox_draw(wind, lb);
		case EVENT_WINDOW_CLOSE:
			std::default_delete<listbox>()(lb);
			break;
		default:
			break;
	}
	return window_event_result::ignored;
}

listbox *newmenu_listbox1(const char *const title, const uint_fast32_t nitems, const char *items[], const int allow_abort_flag, const int default_item, const listbox_subfunction_t<void> listbox_callback, void *const userdata)
{
	window *wind;
	newmenu_free_background();

	auto lb = make_unique<listbox>();
	*lb = {};
	lb->title = title;
	lb->nitems = nitems;
	lb->item = items;
	lb->citem = default_item;
	lb->allow_abort_flag = allow_abort_flag;
	lb->listbox_callback = listbox_callback;
	lb->userdata = userdata;

	set_screen_mode(SCREEN_MENU);	//hafta set the screen mode here or fonts might get changed/freed up if screen res changes
	
	listbox_create_structure(lb.get());

	wind = window_create(grd_curscreen->sc_canvas, lb->box_x-BORDERX, lb->box_y-lb->title_height-BORDERY, lb->box_w+2*BORDERX, lb->height+2*BORDERY, listbox_handler, lb.get());
	if (!wind)
	{
		lb.reset();
	}
	return lb.release();
}
