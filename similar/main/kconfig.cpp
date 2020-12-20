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
 * Routines to configure keyboard, joystick, etc..
 *
 */

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <cstddef>
#include <stdexcept>
#include <functional>

#include "dxxerror.h"
#include "pstypes.h"
#include "gr.h"
#include "window.h"
#include "console.h"
#include "palette.h"
#include "physfsx.h"
#include "game.h"
#include "gamefont.h"
#include "iff.h"
#include "u_mem.h"
#include "kconfig.h"
#include "gauges.h"
#include "rbaudio.h"
#include "render.h"
#include "digi.h"
#include "key.h"
#include "mouse.h"
#include "newmenu.h"
#include "endlevel.h"
#include "multi.h"
#include "timer.h"
#include "text.h"
#include "player.h"
#include "menu.h"
#include "automap.h"
#include "args.h"
#include "lighting.h"
#include "ai.h"
#include "cntrlcen.h"
#include "collide.h"
#include "playsave.h"
#include "screens.h"

#if DXX_USE_OGL
#include "ogl_init.h"
#endif

#include "compiler-range_for.h"
#include "d_array.h"
#include "d_range.h"
#include "d_zip.h"
#include "partial_range.h"

using std::min;
using std::max;
using std::plus;
using std::minus;

namespace dcx {

// Array used to 'blink' the cursor while waiting for a keypress.
const std::array<int8_t, 64> fades{{
	1,1,1,2,2,3,4,4,5,6,8,9,10,12,13,15,
	16,17,19,20,22,23,24,26,27,28,28,29,30,30,31,31,
	31,31,31,30,30,29,28,28,27,26,24,23,22,20,19,17,
	16,15,13,12,10,9,8,6,5,4,4,3,2,2,1,1
}};

const std::array<char[2], 2> invert_text{{"N", "Y"}};
#if DXX_MAX_JOYSTICKS
joybutton_text_t joybutton_text;
#endif
#if DXX_MAX_AXES_PER_JOYSTICK
joyaxis_text_t joyaxis_text;
#endif
constexpr char mouseaxis_text[][8] = { "L/R", "F/B", "WHEEL" };
constexpr char mousebutton_text[][8] = { "LEFT", "RIGHT", "MID", "M4", "M5", "M6", "M7", "M8", "M9", "M10","M11","M12","M13","M14","M15","M16" };

const std::array<uint8_t, 19> system_keys{{
	KEY_ESC, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12, KEY_MINUS, KEY_EQUAL, KEY_PRINT_SCREEN,
	// KEY_*LOCK should always be last since we wanna skip these if -nostickykeys
	KEY_CAPSLOCK, KEY_SCROLLOCK, KEY_NUMLOCK
}};

fix Cruise_speed=0;

#define INFO_Y (188)

const std::array<uint8_t, MAX_DXX_REBIRTH_CONTROLS> DefaultKeySettingsRebirth{{ 0x2,0xff,0xff,0x3,0xff,0xff,0x4,0xff,0xff,0x5,0xff,0xff,0x6,0xff,0xff,0x7,0xff,0xff,0x8,0xff,0xff,0x9,0xff,0xff,0xa,0xff,0xff,0xb,0xff,0xff }};

void kconfig_begin_loop(control_info &Controls)
{
	Controls.pitch_time = Controls.vertical_thrust_time = Controls.heading_time = Controls.sideways_thrust_time = Controls.bank_time = Controls.forward_thrust_time = 0;
}

namespace {

struct kc_mitem {
	uint8_t oldvalue;
	uint8_t value;		// what key,button,etc
};

enum kc_type : uint8_t
{
	BT_KEY = 0,
	BT_MOUSE_BUTTON = 1,
	BT_MOUSE_AXIS = 2,
	BT_JOY_BUTTON = 3,
	BT_JOY_AXIS = 4,
	BT_INVERT = 5,
};

enum kc_state : uint8_t
{
	STATE_NONE = 0,
	STATE_BIT1 = 1,
	STATE_BIT2 = 2,
	STATE_BIT3 = 4,
	STATE_BIT4 = 8,
	STATE_BIT5 = 16,
};

static void kc_drawquestion(grs_canvas &canvas, const grs_font &cv_font, uint8_t &menu_fade_index, const short item_xinput, const short item_y, const int8_t item_w2, const color_palette_index color)
{
	if (++ menu_fade_index > 63)
		menu_fade_index = 0;
	int w, h;
	gr_get_string_size(cv_font, "?", &w, &h, nullptr);
	const auto &&fspacx = FSPACX();
	const auto &&fspacy = FSPACY();
	const auto &&fspacx_item_xinput = fspacx(item_xinput);
	const auto &&fspacy_item_y = fspacy(item_y);
	gr_urect(canvas, fspacx_item_xinput, fspacy(item_y - 1), fspacx(item_xinput + item_w2), fspacy_item_y + h, color);
	gr_set_fontcolor(canvas, BM_XRGB(28, 28, 28), -1);
	const auto x = fspacx_item_xinput + ((fspacx(item_w2) - w) / 2);
	gr_string(canvas, cv_font, x, fspacy_item_y, "?", w, h);
}

}

}

namespace dsx {

control_info Controls;

namespace {

#define kc_item kc_item
struct kc_item
{
	const short x, y;              // x, y pos of label
	const short xinput;                // x pos of input field
	const int8_t w2;                // length of input field
	const uint8_t u,d,l,r;           // neighboring field ids for cursor navigation
	const kc_type type;
	const kc_state state_bit;
	union {
		uint8_t control_info::state_controls_t::*const ci_state_ptr;
		uint8_t control_info::state_controls_t::*const ci_count_ptr;
	};
};

struct kc_menu : window
{
	using window::window;
	const char *litems;
	const kc_item	*items;
	kc_mitem	*mitems;
	const char	*title;
	unsigned	nitems;
	unsigned	citem = 0;
	uint8_t changing = 0;
	ubyte	q_fade_i;	// for flashing the question mark
	uint8_t mouse_state = 0;
	std::array<int, 3>	old_maxis;
#if DXX_MAX_AXES_PER_JOYSTICK
	std::array<int, JOY_MAX_AXES>	old_jaxis;
#endif
	virtual window_event_result event_handler(const d_event &) override;
};

}

const struct player_config::KeySettings DefaultKeySettings{
	/* Keyboard */ {{{
#if defined(DXX_BUILD_DESCENT_I)
		KEY_UP, KEY_PAD8, KEY_DOWN, KEY_PAD2, KEY_LEFT, KEY_PAD4, KEY_RIGHT, KEY_PAD6, KEY_LALT, 0xff, KEY_A, KEY_PAD1, KEY_D, KEY_PAD3, KEY_C, KEY_PADMINUS, KEY_X, KEY_PADPLUS, 0xff, 0xff, KEY_Q, KEY_PAD7, KEY_E, KEY_PAD9, KEY_LCTRL, KEY_RCTRL, KEY_SPACEBAR, 0xff, KEY_F, 0xff, KEY_W, 0xff, KEY_S, 0xff, KEY_B, 0xff, KEY_R, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, KEY_TAB, 0xff, KEY_COMMA, 0x0, KEY_PERIOD, 0x0
#elif defined(DXX_BUILD_DESCENT_II)
		KEY_UP, KEY_PAD8, KEY_DOWN, KEY_PAD2, KEY_LEFT, KEY_PAD4, KEY_RIGHT, KEY_PAD6, KEY_LALT, 0xff, KEY_A, KEY_PAD1, KEY_D, KEY_PAD3, KEY_C, KEY_PADMINUS, KEY_X, KEY_PADPLUS, 0xff, 0xff, KEY_Q, KEY_PAD7, KEY_E, KEY_PAD9, KEY_LCTRL, KEY_RCTRL, KEY_SPACEBAR, 0xff, KEY_F, 0xff, KEY_W, 0xff, KEY_S, 0xff, KEY_B, 0xff, KEY_R, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, KEY_TAB, 0xff, KEY_LSHIFT, 0xff, KEY_COMMA, 0xff, KEY_PERIOD, 0xff, KEY_H, 0xff, KEY_T, 0xff, 0xff, 0xff, 0x0, 0x0
#endif
	}}},
#if DXX_MAX_JOYSTICKS
	{{{
#if defined(DXX_BUILD_DESCENT_I)
		0x0, 0x1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1, 0x0, 0x0, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
#elif defined(DXX_BUILD_DESCENT_II)
		0x0, 0x1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1, 0x0, 0x0, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
#endif
	}}},
#endif
	/* Mouse */ {{{
#if defined(DXX_BUILD_DESCENT_I)
	0x0, 0x1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1, 0x0, 0x0, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
#elif defined(DXX_BUILD_DESCENT_II)
	0x0, 0x1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1, 0x0, 0x0, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0
#endif
	}}}
};

#if defined(DXX_BUILD_DESCENT_I)
#include /* generated by similar/main/generate-kconfig-udlr.py */ "d1x-rebirth/kconfig.udlr.h"
#elif defined(DXX_BUILD_DESCENT_II)
#include /* generated by similar/main/generate-kconfig-udlr.py */ "d2x-rebirth/kconfig.udlr.h"
#endif

namespace {

#if defined(DXX_BUILD_DESCENT_I)
#define D2X_EXTENDED_WEAPON_STRING(X)
#elif defined(DXX_BUILD_DESCENT_II)
#define D2X_EXTENDED_WEAPON_STRING(X)	X
#endif

#define WEAPON_STRING_LASER	D2X_EXTENDED_WEAPON_STRING("(SUPER)") "LASER CANNON"
#define WEAPON_STRING_VULCAN	"VULCAN" D2X_EXTENDED_WEAPON_STRING("/GAUSS") " CANNON"
#define WEAPON_STRING_SPREADFIRE	"SPREADFIRE" D2X_EXTENDED_WEAPON_STRING("/HELIX") " CANNON"
#define WEAPON_STRING_PLASMA	"PLASMA" D2X_EXTENDED_WEAPON_STRING("/PHOENIX") " CANNON"
#define WEAPON_STRING_FUSION	"FUSION" D2X_EXTENDED_WEAPON_STRING("/OMEGA") " CANNON"
#define WEAPON_STRING_CONCUSSION	"CONCUSSION" D2X_EXTENDED_WEAPON_STRING("/FLASH") " MISSILE"
#define WEAPON_STRING_HOMING	"HOMING" D2X_EXTENDED_WEAPON_STRING("/GUIDED") " MISSILE"
#define WEAPON_STRING_PROXIMITY	"PROXIMITY BOMB" D2X_EXTENDED_WEAPON_STRING("/SMART MINE")
#define WEAPON_STRING_SMART	"SMART" D2X_EXTENDED_WEAPON_STRING("/MERCURY") " MISSILE"
#define WEAPON_STRING_MEGA	"MEGA" D2X_EXTENDED_WEAPON_STRING("/EARTHSHAKER") " MISSILE"

#if DXX_MAX_BUTTONS_PER_JOYSTICK || DXX_MAX_HATS_PER_JOYSTICK
#define DXX_KCONFIG_ITEM_JOY_WIDTH(I)	I
#else
#define DXX_KCONFIG_ITEM_JOY_WIDTH(I)	(static_cast<void>(I), 0)
#endif

#include "kconfig.ui-table.cpp"

static enumerated_array<kc_mitem, std::size(kc_keyboard), dxx_kconfig_ui_kc_keyboard> kcm_keyboard;
#if DXX_MAX_JOYSTICKS
static enumerated_array<kc_mitem, std::size(kc_joystick), dxx_kconfig_ui_kc_joystick> kcm_joystick;
#endif
static enumerated_array<kc_mitem, std::size(kc_mouse), dxx_kconfig_ui_kc_mouse> kcm_mouse;
static std::array<kc_mitem, std::size(kc_rebirth)> kcm_rebirth;

static void kconfig_start_changing(kc_menu &menu)
{
	const auto citem = menu.citem;
	if (menu.items[citem].type == BT_INVERT)
	{
		menu.changing = 0;		// in case we were changing something else
		auto &value = menu.mitems[citem].value;
		value = value ? 0 : 1;
		return;
	}
	menu.q_fade_i = 0;	// start question mark flasher
	menu.changing = 1;
}

static void kc_set_exclusive_binding(kc_menu &menu, kc_mitem &mitem, const unsigned type, const unsigned value)
{
	const auto nitems = menu.nitems;
	for (auto &&[iterate_mitem, iterate_item] : zip(unchecked_partial_range(menu.mitems, nitems), unchecked_partial_range(menu.items, nitems)))
	{
		if (&iterate_mitem != &mitem && iterate_mitem.value == value && iterate_item.type == type)
			iterate_mitem.value = 255;
	}
	mitem.value = value;
	menu.changing = 0;
}

}
}

static void kc_drawinput(grs_canvas &, const grs_font &, const kc_item &item, kc_mitem &mitem, int is_current, const char *label);
static void kc_change_key( kc_menu &menu,const d_event &event, kc_mitem& mitem );
#if DXX_MAX_BUTTONS_PER_JOYSTICK || DXX_MAX_HATS_PER_JOYSTICK
static void kc_change_joybutton( kc_menu &menu,const d_event &event, kc_mitem& mitem );
#endif
static void kc_change_mousebutton( kc_menu &menu,const d_event &event, kc_mitem& mitem );
#if DXX_MAX_AXES_PER_JOYSTICK
static void kc_change_joyaxis( kc_menu &menu,const d_event &event, kc_mitem& mitem );
#endif
static void kc_change_mouseaxis( kc_menu &menu,const d_event &event, kc_mitem& mitem );

static const char *get_item_text(const kc_item &item, const kc_mitem &mitem, char (&buf)[10])
{
	if (mitem.value==255) {
		return "";
	} else {
		switch( item.type )	{
			case BT_KEY:
				return key_properties[mitem.value].key_text;
			case BT_MOUSE_BUTTON:
				return mousebutton_text[mitem.value];
			case BT_MOUSE_AXIS:
				return mouseaxis_text[mitem.value];
#if DXX_MAX_BUTTONS_PER_JOYSTICK || DXX_MAX_HATS_PER_JOYSTICK
			case BT_JOY_BUTTON:
				if (joybutton_text.size() > mitem.value)
					return &joybutton_text[mitem.value][0];
				else
				{
					snprintf(buf, sizeof(buf), "BTN%2d", mitem.value + 1);
					return buf;
				}
				break;
#else
				(void)buf;
#endif
#if DXX_MAX_AXES_PER_JOYSTICK
			case BT_JOY_AXIS:
				if (joyaxis_text.size() > mitem.value)
					return &joyaxis_text[mitem.value][0];
				else
				{
					snprintf(buf, sizeof(buf), "AXIS%2d", mitem.value + 1);
					return buf;
				}
				break;
#else
				(void)buf;
#endif
			case BT_INVERT:
				return invert_text[mitem.value];
			default:
				return NULL;
		}
	}
}

static int get_item_height(const grs_font &cv_font, const kc_item &item, const kc_mitem &mitem)
{
	int h;
	char buf[10];
	const char *btext;

	btext = get_item_text(item, mitem, buf);
	if (!btext)
		return 0;
	gr_get_string_size(cv_font, btext, nullptr, &h, nullptr);
	return h;
}

static void kc_gr_2y_string(grs_canvas &canvas, const grs_font &cv_font, const char *const s, const font_y_scaled_float y, const font_x_scaled_float x0, const font_x_scaled_float x1)
{
	int w, h;
	gr_get_string_size(cv_font, s, &w, &h, nullptr);
	gr_string(canvas, cv_font, x0, y, s, w, h);
	gr_string(canvas, cv_font, x1, y, s, w, h);
}

static void kconfig_draw(kc_menu &menu)
{
	grs_canvas &save_canvas = *grd_curcanv;
	const auto &&fspacx = FSPACX();
	const auto &&fspacy = FSPACY();
	int w = fspacx(290), h = fspacy(170);

	gr_set_default_canvas();
	nm_draw_background(*grd_curcanv, ((SWIDTH - w) / 2) - BORDERX, ((SHEIGHT - h) / 2) - BORDERY, ((SWIDTH - w) / 2) + w + BORDERX, ((SHEIGHT - h) / 2) + h + BORDERY);

	gr_set_current_canvas(menu.w_canv);
	auto &canvas = *grd_curcanv;

	const grs_font *save_font = canvas.cv_font;

	auto &medium3_font = *MEDIUM3_FONT;
	gr_string(canvas, medium3_font, 0x8000, fspacy(8), menu.title);

	auto &game_font = *GAME_FONT;
	gr_set_fontcolor(canvas, BM_XRGB(28, 28, 28), -1);
	gr_string(canvas, game_font, 0x8000, fspacy(21), "Enter changes, ctrl-d deletes, ctrl-r resets defaults, ESC exits");

	if (menu.items == kc_keyboard)
	{
		gr_set_fontcolor(canvas, BM_XRGB(31, 27, 6), -1);
		const uint8_t color = BM_XRGB(31,27,6);
		const auto &&fspacx98 = fspacx(98);
		const auto &&fspacx128 = fspacx(128);
		const auto &&fspacy42 = fspacy(42);
		gr_rect(canvas, fspacx98, fspacy42, fspacx(106), fspacy42, color); // horiz/left
		gr_rect(canvas, fspacx(120), fspacy42, fspacx128, fspacy42, color); // horiz/right
		const auto &&fspacy44 = fspacy(44);
		gr_rect(canvas, fspacx98, fspacy42, fspacx98, fspacy44, color); // vert/left
		gr_rect(canvas, fspacx128, fspacy42, fspacx128, fspacy44, color); // vert/right

		const auto &&fspacx253 = fspacx(253);
		const auto &&fspacx283 = fspacx(283);
		gr_rect(canvas, fspacx253, fspacy42, fspacx(261), fspacy42, color); // horiz/left
		gr_rect(canvas, fspacx(275), fspacy42, fspacx283, fspacy42, color); // horiz/right
		gr_rect(canvas, fspacx253, fspacy42, fspacx253, fspacy44, color); // vert/left
		gr_rect(canvas, fspacx283, fspacy42, fspacx283, fspacy44, color); // vert/right

		kc_gr_2y_string(canvas, game_font, "OR", fspacy(40), fspacx(109), fspacx(264));
	}
#if DXX_MAX_JOYSTICKS
	else if (menu.items == kc_joystick)
	{
		const uint8_t color = BM_XRGB(31, 27, 6);
		gr_set_fontcolor(canvas, color, -1);
#if DXX_MAX_AXES_PER_JOYSTICK
		constexpr auto kconfig_axis_labels_top_y = DXX_KCONFIG_UI_JOYSTICK_AXES_TOP_Y + 8;
		const auto &&fspacy_lower_label2 = fspacy(kconfig_axis_labels_top_y);
		gr_string(canvas, game_font, 0x8000, fspacy(DXX_KCONFIG_UI_JOYSTICK_AXES_TOP_Y), TXT_AXES);
		gr_set_fontcolor(canvas, BM_XRGB(28, 28, 28), -1);
		kc_gr_2y_string(canvas, game_font, TXT_AXIS, fspacy_lower_label2, fspacx(81), fspacx(230));
		kc_gr_2y_string(canvas, game_font, TXT_INVERT, fspacy_lower_label2, fspacx(111), fspacx(260));
		gr_set_fontcolor(canvas, color, -1);
#endif

#if DXX_MAX_BUTTONS_PER_JOYSTICK || DXX_MAX_HATS_PER_JOYSTICK
		constexpr auto kconfig_buttons_top_y = DXX_KCONFIG_UI_JOYSTICK_TOP_Y + 6;
		constexpr auto kconfig_buttons_labels_top_y = kconfig_buttons_top_y + 4;
		gr_string(canvas, game_font, 0x8000, fspacy(DXX_KCONFIG_UI_JOYSTICK_TOP_Y), TXT_BUTTONS);
		const auto &&fspacx115 = fspacx(115);
		const auto &&fspacy_horiz_OR_line = fspacy(kconfig_buttons_labels_top_y);
		gr_rect(canvas, fspacx115, fspacy_horiz_OR_line, fspacx(123), fspacy_horiz_OR_line, color); // horiz/left
		const auto &&fspacx145 = fspacx(145);
		gr_rect(canvas, fspacx(137), fspacy_horiz_OR_line, fspacx145, fspacy_horiz_OR_line, color); // horiz/right
		const auto &&fspacx261 = fspacx(261);
		gr_rect(canvas, fspacx261, fspacy_horiz_OR_line, fspacx(269), fspacy_horiz_OR_line, color); // horiz/left
		const auto &&fspacx291 = fspacx(291);
		gr_rect(canvas, fspacx(283), fspacy_horiz_OR_line, fspacx291, fspacy_horiz_OR_line, color); // horiz/right

		const auto &&fspacy_vert_OR_line = fspacy(kconfig_buttons_labels_top_y + 2);
		gr_rect(canvas, fspacx115, fspacy_horiz_OR_line, fspacx115, fspacy_vert_OR_line, color); // vert/left
		gr_rect(canvas, fspacx145, fspacy_horiz_OR_line, fspacx145, fspacy_vert_OR_line, color); // vert/right
		gr_rect(canvas, fspacx261, fspacy_horiz_OR_line, fspacx261, fspacy_vert_OR_line, color); // vert/left
		gr_rect(canvas, fspacx291, fspacy_horiz_OR_line, fspacx291, fspacy_vert_OR_line, color); // vert/right

		kc_gr_2y_string(canvas, game_font, "OR", fspacy(kconfig_buttons_top_y + 2), fspacx(126), fspacx(272));
#endif
	}
#endif
	else if (menu.items == kc_mouse)
	{
		gr_set_fontcolor(canvas, BM_XRGB(31, 27, 6), -1);
		gr_string(canvas, game_font, 0x8000, fspacy(37), TXT_BUTTONS);
		gr_string(canvas, game_font, 0x8000, fspacy(137), TXT_AXES);
		gr_set_fontcolor(canvas, BM_XRGB(28, 28, 28), -1);
		const auto &&fspacy145 = fspacy(145);
		kc_gr_2y_string(canvas, game_font, TXT_AXIS, fspacy145, fspacx( 87), fspacx(242));
		kc_gr_2y_string(canvas, game_font, TXT_INVERT, fspacy145, fspacx(120), fspacx(274));
	}
	else if (menu.items == kc_rebirth)
	{
		gr_set_fontcolor(canvas, BM_XRGB(31, 27, 6), -1);

		const auto &&fspacy60 = fspacy(60);
		gr_string(canvas, game_font, fspacx(152), fspacy60, "KEYBOARD");
#if DXX_MAX_BUTTONS_PER_JOYSTICK || DXX_MAX_HATS_PER_JOYSTICK
		gr_string(canvas, game_font, fspacx(210), fspacy60, "JOYSTICK");
#endif
		gr_string(canvas, game_font, fspacx(273), fspacy60, "MOUSE");
	}
	
	unsigned citem = menu.citem;
	const char *current_label = NULL;
	const char *litem = menu.litems;
	const auto nitems = menu.nitems;
	for (const auto i : xrange(nitems))
	{
		auto next_label = (i + 1 >= menu.nitems || menu.items[i + 1].y != menu.items[i].y) ? litem : nullptr;
		if (i == citem)
			current_label = litem;
		else if (menu.items[i].w2)
			kc_drawinput(canvas, game_font, menu.items[i], menu.mitems[i], 0, next_label);
		if (next_label)
			litem += strlen(litem) + 1;
	}
	kc_drawinput(canvas, game_font, menu.items[citem], menu.mitems[citem], 1, current_label);
	
	gr_set_fontcolor(canvas, BM_XRGB(28, 28, 28), -1);
	if (menu.changing)
	{
		const char *s;
		switch(menu.items[menu.citem].type)
		{
			case BT_KEY:
				s = TXT_PRESS_NEW_KEY;
				break;
			case BT_MOUSE_BUTTON:
				s = TXT_PRESS_NEW_MBUTTON;
				break;
			case BT_MOUSE_AXIS:
				s = TXT_MOVE_NEW_MSE_AXIS;
				break;
#if DXX_MAX_BUTTONS_PER_JOYSTICK || DXX_MAX_HATS_PER_JOYSTICK
			case BT_JOY_BUTTON:
				s = TXT_PRESS_NEW_JBUTTON;
				break;
#endif
#if DXX_MAX_AXES_PER_JOYSTICK
			case BT_JOY_AXIS:
				s = TXT_MOVE_NEW_JOY_AXIS;
				break;
#endif
			default:
				s = nullptr;
				break;
		}
		if (s)
			gr_string(canvas, game_font, 0x8000, fspacy(INFO_Y), s);
		auto &item = menu.items[menu.citem];
		auto &menu_fade_index = menu.q_fade_i;
		const auto fade_element = fades[menu_fade_index];
#if defined(DXX_BUILD_DESCENT_I)
		const auto color = gr_fade_table[fade_element][BM_XRGB(21, 0, 24)];
#elif defined(DXX_BUILD_DESCENT_II)
		const auto color = BM_XRGB(21 * fade_element / 31, 0, 24 * fade_element / 31);
#endif
		kc_drawquestion(canvas, game_font, menu_fade_index, item.xinput, item.y, item.w2, color);
	}
	canvas.cv_font = save_font;
	gr_set_current_canvas( save_canvas );
}

static inline int in_bounds(unsigned mx, unsigned my, unsigned x1, unsigned xw, unsigned y1, unsigned yh)
{
	if (mx <= x1)
		return 0;
	if (my <= y1)
		return 0;
	if (mx >= x1 + xw)
		return 0;
	if (my >= y1 + yh)
		return 0;
	return 1;
}

namespace dsx {

namespace {

static window_event_result kconfig_mouse(kc_menu &menu, const d_event &event)
{
	grs_canvas &save_canvas = *grd_curcanv;
	int mx, my, mz, x1, y1;
	window_event_result rval = window_event_result::ignored;

	gr_set_current_canvas(menu.w_canv);
	auto &canvas = *grd_curcanv;
	
	if (menu.mouse_state)
	{
		int item_height;
		
		mouse_get_pos(&mx, &my, &mz);
		const auto &&fspacx = FSPACX();
		const auto &&fspacy = FSPACY();
		const auto nitems = menu.nitems;
		for (const auto i : xrange(nitems))
		{
			item_height = get_item_height(*canvas.cv_font, menu.items[i], menu.mitems[i]);
			x1 = canvas.cv_bitmap.bm_x + fspacx(menu.items[i].xinput);
			y1 = canvas.cv_bitmap.bm_y + fspacy(menu.items[i].y);
			if (in_bounds(mx, my, x1, fspacx(menu.items[i].w2), y1, item_height)) {
				menu.citem = i;
				rval = window_event_result::handled;
				break;
			}
		}
	}
	else if (event.type == EVENT_MOUSE_BUTTON_UP)
	{
		int item_height;
		
		mouse_get_pos(&mx, &my, &mz);
		item_height = get_item_height(*canvas.cv_font, menu.items[menu.citem], menu.mitems[menu.citem]);
		const auto &&fspacx = FSPACX();
		x1 = canvas.cv_bitmap.bm_x + fspacx(menu.items[menu.citem].xinput);
		y1 = canvas.cv_bitmap.bm_y + FSPACY(menu.items[menu.citem].y);
		if (in_bounds(mx, my, x1, fspacx(menu.items[menu.citem].w2), y1, item_height)) {
			kconfig_start_changing(menu);
			rval = window_event_result::handled;
		}
		else
		{
			// Click out of changing mode - kreatordxx
			menu.changing = 0;
			rval = window_event_result::handled;
		}
	}
	
	gr_set_current_canvas(save_canvas);
	
	return rval;
}

}

}

namespace dcx {

namespace {

template <std::size_t M, std::size_t C>
static void reset_mitem_values(std::array<kc_mitem, M> &m, const std::array<uint8_t, C> &c)
{
	for (std::size_t i = 0; i != min(M, C); ++i)
		m[i].value = c[i];
}

static void step_citem_past_empty_cell(unsigned &citem, const kc_item *const items, const uint8_t kc_item::*const next)
{
	do {
		citem = items[citem].*next;
	} while (!items[citem].w2);
}

static window_event_result kconfig_key_command(kc_menu &menu, const d_event &event)
{
	auto k = event_key_get(event);

	// when changing, process no keys instead of ESC
	if (menu.changing && (k != -2 && k != KEY_ESC))
		return window_event_result::ignored;
	switch (k)
	{
		case KEY_CTRLED+KEY_D:
			menu.mitems[menu.citem].value = 255;
			return window_event_result::handled;
		case KEY_CTRLED+KEY_R:	
			if (menu.items==kc_keyboard)
				reset_mitem_values(kcm_keyboard, DefaultKeySettings.Keyboard);
#if DXX_MAX_JOYSTICKS
			else if (menu.items == kc_joystick)
				reset_mitem_values(kcm_joystick, DefaultKeySettings.Joystick);
#endif
			else if (menu.items == kc_mouse)
				reset_mitem_values(kcm_mouse, DefaultKeySettings.Mouse);
			else if (menu.items == kc_rebirth)
				reset_mitem_values(kcm_rebirth, DefaultKeySettingsRebirth);
			return window_event_result::handled;
		case KEY_DELETE:
			menu.mitems[menu.citem].value = 255;
			return window_event_result::handled;
		case KEY_UP: 		
		case KEY_PAD8:
			step_citem_past_empty_cell(menu.citem, menu.items, &kc_item::u);
			return window_event_result::handled;
		case KEY_DOWN:
		case KEY_PAD2:
			step_citem_past_empty_cell(menu.citem, menu.items, &kc_item::d);
			return window_event_result::handled;
		case KEY_LEFT:
		case KEY_PAD4:
			step_citem_past_empty_cell(menu.citem, menu.items, &kc_item::l);
			return window_event_result::handled;
		case KEY_RIGHT:
		case KEY_PAD6:
			step_citem_past_empty_cell(menu.citem, menu.items, &kc_item::r);
			return window_event_result::handled;
		case KEY_ENTER:
		case KEY_PADENTER:
			kconfig_start_changing(menu);
			return window_event_result::handled;
		case KEY_ESC:
			if (menu.changing)
				menu.changing = 0;
			else
			{
				return window_event_result::close;
			}
			return window_event_result::handled;
		case 0:		// some other event
			break;
			
		default:
			break;
	}
	return window_event_result::ignored;
}

}

}

namespace dsx {

namespace {

window_event_result kc_menu::event_handler(const d_event &event)
{
#if DXX_MAX_BUTTONS_PER_JOYSTICK
	if (!changing && joy_translate_menu_key(event))
		return window_event_result::handled;
#endif

	switch (event.type)
	{
		case EVENT_WINDOW_ACTIVATED:
			game_flush_inputs(Controls);
			break;
			
		case EVENT_WINDOW_DEACTIVATED:
			mouse_state = 0;
			break;
			
		case EVENT_MOUSE_BUTTON_DOWN:
		case EVENT_MOUSE_BUTTON_UP:
			if (changing && (items[citem].type == BT_MOUSE_BUTTON) && (event.type == EVENT_MOUSE_BUTTON_UP))
			{
				kc_change_mousebutton(*this, event, mitems[citem]);
				mouse_state = (event.type == EVENT_MOUSE_BUTTON_DOWN);
				return window_event_result::handled;
			}

			if (event_mouse_get_button(event) == MBTN_RIGHT)
			{
				if (!changing)
				{
					return window_event_result::close;
				}
				return window_event_result::handled;
			}
			else if (event_mouse_get_button(event) != MBTN_LEFT)
				return window_event_result::ignored;

			mouse_state = (event.type == EVENT_MOUSE_BUTTON_DOWN);
			return kconfig_mouse(*this, event);

		case EVENT_MOUSE_MOVED:
			if (changing && items[citem].type == BT_MOUSE_AXIS)
				kc_change_mouseaxis(*this, event, mitems[citem]);
			else
				event_mouse_get_delta(event, &old_maxis[0], &old_maxis[1], &old_maxis[2]);
			break;

#if DXX_MAX_BUTTONS_PER_JOYSTICK || DXX_MAX_HATS_PER_JOYSTICK
		case EVENT_JOYSTICK_BUTTON_DOWN:
			if (changing && items[citem].type == BT_JOY_BUTTON)
				kc_change_joybutton(*this, event, mitems[citem]);
			break;
#endif

#if DXX_MAX_AXES_PER_JOYSTICK
		case EVENT_JOYSTICK_MOVED:
			if (changing && items[citem].type == BT_JOY_AXIS)
				kc_change_joyaxis(*this, event, mitems[citem]);
			else
			{
				const auto &av = event_joystick_get_axis(event);
				const auto &axis = av.axis;
				const auto &value = av.value;
				old_jaxis[axis] = value;
			}
			break;
#endif

		case EVENT_KEY_COMMAND:
		{
			window_event_result rval = kconfig_key_command(*this, event);
			if (rval != window_event_result::ignored)
				return rval;
			if (changing && items[citem].type == BT_KEY)
				kc_change_key(*this, event, mitems[citem]);
			return window_event_result::ignored;
		}

		case EVENT_IDLE:
			kconfig_mouse(*this, event);
			break;
			
		case EVENT_WINDOW_DRAW:
			if (changing)
				timer_delay(f0_1/10);
			else
				timer_delay2(50);
			kconfig_draw(*this);
			break;
			
		case EVENT_WINDOW_CLOSE:
			// Update save values...
			for (auto &&[kcm, setting] : zip(kcm_keyboard, PlayerCfg.KeySettings.Keyboard))
				setting = kcm.value;
#if DXX_MAX_JOYSTICKS
			for (auto &&[kcm, setting] : zip(kcm_joystick, PlayerCfg.KeySettings.Joystick))
				setting = kcm.value;
#endif

			for (auto &&[kcm, setting] : zip(kcm_mouse, PlayerCfg.KeySettings.Mouse))
				setting = kcm.value;
			for (auto &&[kcm, setting] : zip(kcm_rebirth, PlayerCfg.KeySettingsRebirth))
				setting = kcm.value;
			return window_event_result::ignored;	// continue closing
		default:
			return window_event_result::ignored;
	}
	return window_event_result::handled;
}

static void kconfig_sub(const char *litems, const kc_item * items,kc_mitem *mitems,int nitems, const char *title)
{
	set_screen_mode(SCREEN_MENU);
	kc_set_controls();

	const auto &&fspacx = FSPACX();
	const auto &&fspacy = FSPACY();
	const auto &&window_width = fspacx(320);
	const auto &&window_height = fspacy(220);
	auto menu = window_create<kc_menu>(grd_curscreen->sc_canvas, (SWIDTH - window_width) / 2, (SHEIGHT - window_height) / 2, window_width, window_height);
	menu->items = items;
	menu->litems = litems;
	menu->mitems = mitems;
	menu->nitems = nitems;
	menu->title = title;
	if (!items[0].w2)
		step_citem_past_empty_cell(menu->citem, items, &kc_item::r);
}

template <std::size_t N>
static void kconfig_sub(const char *litems, const kc_item (&items)[N], std::array<kc_mitem, N> &mitems, const char *title)
{
	kconfig_sub(litems, items, mitems.data(), N, title);
}

}

}

static void kc_drawinput(grs_canvas &canvas, const grs_font &cv_font, const kc_item &item, kc_mitem &mitem, const int is_current, const char *const label)
{
	char buf[10];
	const char *btext;
	const auto &&fspacx = FSPACX();
	const auto &&fspacy = FSPACY();
	unsigned r, g, b;
	if (label)
	{
		if (is_current)
			r = 20 * 2, g = 20 * 2, b = 29 * 2;
		else
			r = 15 * 2, g = 15 * 2, b = 24 * 2;
		gr_set_fontcolor(canvas, gr_find_closest_color(r, g, b), -1);
		gr_string(canvas, cv_font, fspacx(item.x), fspacy(item.y), label);
	}

	btext = get_item_text(item, mitem, buf);
	if (!btext)
		return;
	{
		if (is_current)
			r = 21 * 2, g = 0, b = 24 * 2;
		else
			r = 16 * 2, g = 0, b = 19 * 2;

		int x, w, h;
		const auto color = gr_find_closest_color(r, g, b);
		gr_get_string_size(cv_font, btext, &w, &h, nullptr);
		const auto &&fspacx_item_xinput = fspacx(item.xinput);
		const auto &&fspacy_item_y = fspacy(item.y);
		gr_urect(canvas, fspacx_item_xinput, fspacy(item.y - 1), fspacx(item.xinput + item.w2), fspacy_item_y + h, color);
		
		if (mitem.oldvalue == mitem.value)
			r = b = 28 * 2;
		else
			r = b = 0;
		gr_set_fontcolor(canvas, gr_find_closest_color(r, 28 * 2, b), -1);

		x = fspacx_item_xinput + ((fspacx(item.w2) - w) / 2);
	
		gr_string(canvas, cv_font, x, fspacy_item_y, btext, w, h);
	}
}

static void kc_change_key( kc_menu &menu,const d_event &event, kc_mitem &mitem )
{
	ubyte keycode = 255;

	Assert(event.type == EVENT_KEY_COMMAND);
	keycode = event_key_get_raw(event);

	auto e = end(system_keys);
	if (unlikely(CGameArg.CtlNoStickyKeys))
		e = std::prev(e, 3);
	const auto predicate = [keycode](uint8_t k) { return keycode == k; };
	if (std::any_of(begin(system_keys), e, predicate))
		return;

	kc_set_exclusive_binding(menu, mitem, BT_KEY, keycode);
}

#if DXX_MAX_BUTTONS_PER_JOYSTICK
static void kc_change_joybutton( kc_menu &menu,const d_event &event, kc_mitem &mitem )
{
	int button = 255;

	Assert(event.type == EVENT_JOYSTICK_BUTTON_DOWN);
	button = event_joystick_get_button(event);

	kc_set_exclusive_binding(menu, mitem, BT_JOY_BUTTON, button);
}
#endif

static void kc_change_mousebutton( kc_menu &menu,const d_event &event, kc_mitem &mitem)
{
	int button;

	Assert(event.type == EVENT_MOUSE_BUTTON_DOWN || event.type == EVENT_MOUSE_BUTTON_UP);
	button = event_mouse_get_button(event);

	kc_set_exclusive_binding(menu, mitem, BT_MOUSE_BUTTON, button);
}

#if DXX_MAX_AXES_PER_JOYSTICK
static void kc_change_joyaxis( kc_menu &menu,const d_event &event, kc_mitem &mitem )
{
	const auto &av = event_joystick_get_axis(event);
	const auto &axis = av.axis;
	const auto &value = av.value;

	if ( abs(value-menu.old_jaxis[axis])<32 )
		return;
	con_printf(CON_DEBUG, "Axis Movement detected: Axis %i", axis);

	kc_set_exclusive_binding(menu, mitem, BT_JOY_AXIS, axis);
}
#endif

static void kc_change_mouseaxis( kc_menu &menu,const d_event &event, kc_mitem &mitem )
{
	int dx, dy, dz;

	Assert(event.type == EVENT_MOUSE_MOVED);
	event_mouse_get_delta( event, &dx, &dy, &dz );
	uint8_t code;
	if (abs(dz) > 5)
		code = 2;
	else if (abs(dy) > 5)
		code = 1;
	else if (abs(dx) > 5)
		code = 0;
	else
		return;
	kc_set_exclusive_binding(menu, mitem, BT_MOUSE_AXIS, code);
}

void kconfig(const kconfig_type n)
{
	switch (n)
	{
#define kconfig_case(TYPE,TITLE)	\
		case kconfig_type::TYPE:	\
			kconfig_sub(DXX_KCONFIG_UI_LABEL_kc_##TYPE, kc_##TYPE, kcm_##TYPE, TITLE);	\
			break;
		kconfig_case(keyboard, "KEYBOARD");
#if DXX_MAX_JOYSTICKS
		kconfig_case(joystick, "JOYSTICK");
#endif
		kconfig_case(mouse, "MOUSE");
		kconfig_case(rebirth, "WEAPON KEYS");
#undef kconfig_case
		default:
			Int3();
			return;
	}
}

namespace dsx {

static void input_button_matched(control_info &Controls, const kc_item& item, int down)
{
	if (item.state_bit)
	{
		if (!item.ci_state_ptr)
			throw std::logic_error("NULL state pointer with non-zero state bit");
		if (down)
			Controls.state.*item.ci_state_ptr |= item.state_bit;
		else
			Controls.state.*item.ci_state_ptr &= ~item.state_bit;
	}
	else
	{
		if (item.ci_count_ptr != NULL && down)
			Controls.state.*item.ci_count_ptr += 1;
	}
}

}

namespace dcx {

namespace {

template <template<typename> class F>
static void adjust_ramped_keyboard_field(float& keydown_time, ubyte& state, fix& time, const float& sensitivity, const int& speed_factor, const int& speed_divisor = 1)
#define adjust_ramped_keyboard_field(F, M, ...)	\
	(adjust_ramped_keyboard_field<F>(Controls.down_time.M, Controls.state.M, __VA_ARGS__))
{
	if (state)
	{
                if (keydown_time < F1_0)
			keydown_time += (static_cast<float>(FrameTime) * 6.66) * ((sensitivity + 1) / 17); // values based on observation that the original game uses a keyboard ramp of 8 frames. Full sensitivity should reflect 60FPS behaviour, half sensitivity reflects 30FPS behaviour (give or take a frame).
                time = F<fix>()(time, speed_factor / speed_divisor * (keydown_time / F1_0));
	}
	else
		keydown_time = 0;
}

template <std::size_t N>
static void adjust_axis_field(fix& time, const std::array<fix, N> &axes, unsigned value, unsigned invert, const int& sensitivity)
{
	if (value == 255)
		return;
	fix amount = (axes.at(value) * sensitivity) / 8;
	if ( !invert ) // If not inverted...
		time -= amount;
	else
		time += amount;
}

static void clamp_value(fix& value, const fix& lower, const fix& upper)
{
	value = min(max(value, lower), upper);
}

static void clamp_symmetric_value(fix& value, const fix& bound)
{
	clamp_value(value, -bound, bound);
}

#if DXX_MAX_AXES_PER_JOYSTICK
static void convert_raw_joy_axis(control_info::joystick_axis_values &Controls, const uint_fast32_t player_cfg_index, const uint_fast32_t i)
{
	const auto raw_joy_axis = Controls.raw_joy_axis[i];
	const auto joy_axis = (abs(raw_joy_axis) <= (128 * PlayerCfg.JoystickLinear[player_cfg_index]) / 16)
		? (raw_joy_axis * (FrameTime * PlayerCfg.JoystickSpeed[player_cfg_index]) / 16)
		: (raw_joy_axis * FrameTime);
	Controls.joy_axis[i] = joy_axis / 128;
}

static void convert_raw_joy_axis(control_info::joystick_axis_values &Controls, const dxx_kconfig_ui_kc_joystick kcm_index, const uint_fast32_t player_cfg_index, const uint_fast32_t i)
{
	if (i != kcm_joystick[kcm_index].value)
		return;
	convert_raw_joy_axis(Controls, player_cfg_index, i);
}
#endif

static inline void adjust_button_time(fix &o, uint8_t add, uint8_t sub, fix v)
{
	if (add)
	{
		if (sub)
			return;
		o += v;
	}
	else if (sub)
		o -= v;
}

static void clamp_kconfig_control_with_overrun(fix &value, const fix &bound, fix &excess, const fix &ebound)
{
	/* Assume no integer overflow here */
	value += excess;
	const auto ivalue = value;
	clamp_symmetric_value(value, bound);
	excess = ivalue - value;
	clamp_symmetric_value(excess, ebound);
}

static unsigned allow_uncapped_turning()
{
	const auto game_mode = Game_mode;
	if (!(game_mode & GM_MULTI))
		return PlayerCfg.MouselookFlags & MouselookMode::Singleplayer;
	return PlayerCfg.MouselookFlags &
		Netgame.MouselookFlags &
		((game_mode & GM_MULTI_COOP)
		? MouselookMode::MPCoop
		: MouselookMode::MPAnarchy);
}

}

}

namespace dsx {

void kconfig_read_controls(control_info &Controls, const d_event &event, int automap_flag)
{
	static fix64 mouse_delta_time = 0;

#ifndef NDEBUG
	// --- Don't do anything if in debug mode ---
	if ( keyd_pressed[KEY_DELETE] )
	{
		Controls = {};
		return;
	}
#endif

	const auto frametime = FrameTime;

	switch (event.type)
	{
		case EVENT_KEY_COMMAND:
		case EVENT_KEY_RELEASE:
			{
				const auto &&key = event_key_get_raw(event);
				if (key < 255)
				{
					for (auto &&[kc, kcm] : zip(kc_keyboard, kcm_keyboard))
					{
						if (kcm.value == key)
							input_button_matched(Controls, kc, event.type == EVENT_KEY_COMMAND);
					}
			if (!automap_flag && event.type == EVENT_KEY_COMMAND)
				for (uint_fast32_t i = 0, j = 0; i < 28; i += 3, j++)
					if (kcm_rebirth[i].value == key)
					{
						Controls.state.select_weapon = j+1;
						break;
					}
				}
			}
			break;
#if DXX_MAX_BUTTONS_PER_JOYSTICK
		case EVENT_JOYSTICK_BUTTON_DOWN:
		case EVENT_JOYSTICK_BUTTON_UP:
			if (!(PlayerCfg.ControlType & CONTROL_USING_JOYSTICK))
				break;
			{
				const auto &&button = event_joystick_get_button(event);
				if (button < 255)
				{
					for (auto &&[kc, kcm] : zip(kc_joystick, kcm_joystick))
					{
						if (kc.type == BT_JOY_BUTTON && kcm.value == button)
							input_button_matched(Controls, kc, event.type == EVENT_JOYSTICK_BUTTON_DOWN);
					}
			if (!automap_flag && event.type == EVENT_JOYSTICK_BUTTON_DOWN)
				for (uint_fast32_t i = 1, j = 0; i < 29; i += 3, j++)
					if (kcm_rebirth[i].value == button)
					{
						Controls.state.select_weapon = j+1;
						break;
					}
				}
			break;
			}
#endif
		case EVENT_MOUSE_BUTTON_DOWN:
		case EVENT_MOUSE_BUTTON_UP:
			if (!(PlayerCfg.ControlType & CONTROL_USING_MOUSE))
				break;
			{
				const auto &&button = event_mouse_get_button(event);
				if (button < 255)
				{
					for (auto &&[kc, kcm] : zip(kc_mouse, kcm_mouse))
					{
						if (kc.type == BT_MOUSE_BUTTON && kcm.value == button)
							input_button_matched(Controls, kc, event.type == EVENT_MOUSE_BUTTON_DOWN);
					}
			if (!automap_flag && event.type == EVENT_MOUSE_BUTTON_DOWN)
				for (uint_fast32_t i = 2, j = 0; i < 30; i += 3, j++)
					if (kcm_rebirth[i].value == button)
					{
						Controls.state.select_weapon = j+1;
						break;
					}
				}
			}
			break;
#if DXX_MAX_AXES_PER_JOYSTICK
		case EVENT_JOYSTICK_MOVED:
		{
			int joy_null_value = 0;
			if (!(PlayerCfg.ControlType & CONTROL_USING_JOYSTICK))
				break;
			const auto &av = event_joystick_get_axis(event);
			const auto &axis = av.axis;
			auto value = av.value;

			if (axis == PlayerCfg.KeySettings.Joystick[dxx_kconfig_ui_kc_joystick_pitch]) // Pitch U/D Deadzone
				joy_null_value = PlayerCfg.JoystickDead[1]*8;
			else if (axis == PlayerCfg.KeySettings.Joystick[dxx_kconfig_ui_kc_joystick_turn]) // Turn L/R Deadzone
				joy_null_value = PlayerCfg.JoystickDead[0]*8;
			else if (axis == PlayerCfg.KeySettings.Joystick[dxx_kconfig_ui_kc_joystick_slide_lr]) // Slide L/R Deadzone
				joy_null_value = PlayerCfg.JoystickDead[2]*8;
			else if (axis == PlayerCfg.KeySettings.Joystick[dxx_kconfig_ui_kc_joystick_slide_ud]) // Slide U/D Deadzone
				joy_null_value = PlayerCfg.JoystickDead[3]*8;
#ifdef dxx_kconfig_ui_kc_joystick_bank
			else if (axis == PlayerCfg.KeySettings.Joystick[dxx_kconfig_ui_kc_joystick_bank]) // Bank Deadzone
				joy_null_value = PlayerCfg.JoystickDead[4]*8;
#endif
#ifdef dxx_kconfig_ui_kc_joystick_throttle
			else if (axis == PlayerCfg.KeySettings.Joystick[dxx_kconfig_ui_kc_joystick_throttle]) // Throttle - default deadzone
				joy_null_value = PlayerCfg.JoystickDead[5]*3;
#endif

			Controls.raw_joy_axis[axis] = apply_deadzone(value, joy_null_value);
			break;
		}
#endif
		case EVENT_MOUSE_MOVED:
		{
			if (!(PlayerCfg.ControlType & CONTROL_USING_MOUSE))
				break;
			if (PlayerCfg.MouseFlightSim)
			{
				int ax[3];
				event_mouse_get_delta( event, &ax[0], &ax[1], &ax[2] );
				for (uint_fast32_t i = 0; i <= 2; i++)
				{
					int mouse_null_value = (i==2?16:PlayerCfg.MouseFSDead*8);
					Controls.raw_mouse_axis[i] += ax[i];
					clamp_symmetric_value(Controls.raw_mouse_axis[i], MOUSEFS_DELTA_RANGE);
					if (Controls.raw_mouse_axis[i] > mouse_null_value) 
						Controls.mouse_axis[i] = (((Controls.raw_mouse_axis[i] - mouse_null_value) * MOUSEFS_DELTA_RANGE) / (MOUSEFS_DELTA_RANGE - mouse_null_value) * frametime) / MOUSEFS_DELTA_RANGE;
					else if (Controls.raw_mouse_axis[i] < -mouse_null_value)
						Controls.mouse_axis[i] = (((Controls.raw_mouse_axis[i] + mouse_null_value) * MOUSEFS_DELTA_RANGE) / (MOUSEFS_DELTA_RANGE - mouse_null_value) * frametime) / MOUSEFS_DELTA_RANGE;
					else
						Controls.mouse_axis[i] = 0;
				}
			}
			else
			{
				event_mouse_get_delta( event, &Controls.raw_mouse_axis[0], &Controls.raw_mouse_axis[1], &Controls.raw_mouse_axis[2] );
				Controls.mouse_axis[0] = (Controls.raw_mouse_axis[0] * frametime) / 8;
				Controls.mouse_axis[1] = (Controls.raw_mouse_axis[1] * frametime) / 8;
				Controls.mouse_axis[2] = (Controls.raw_mouse_axis[2] * frametime);
				mouse_delta_time = timer_query() + DESIGNATED_GAME_FRAMETIME;
			}
			break;
		}
		case EVENT_IDLE:
		default:
			if (!PlayerCfg.MouseFlightSim && mouse_delta_time < timer_query())
			{
				Controls.mouse_axis[0] = Controls.mouse_axis[1] = Controls.mouse_axis[2] = 0;
				mouse_delta_time = timer_query() + DESIGNATED_GAME_FRAMETIME;
			}
			break;
	}
	
#if DXX_MAX_AXES_PER_JOYSTICK
	for (int i = 0; i < JOY_MAX_AXES; i++) {
		convert_raw_joy_axis(Controls, dxx_kconfig_ui_kc_joystick_turn, 0, i); // Turn L/R
		convert_raw_joy_axis(Controls, dxx_kconfig_ui_kc_joystick_pitch, 1, i); // Pitch U/D
		convert_raw_joy_axis(Controls, dxx_kconfig_ui_kc_joystick_slide_lr, 2, i); // Slide L/R
		convert_raw_joy_axis(Controls, dxx_kconfig_ui_kc_joystick_slide_ud, 3, i); // Slide U/D
		convert_raw_joy_axis(Controls, dxx_kconfig_ui_kc_joystick_bank, 4, i); // Bank
		convert_raw_joy_axis(Controls, dxx_kconfig_ui_kc_joystick_throttle, 5, i); // Throttle
	}
#endif

	const auto speed_factor = (cheats.turbo ? 2 : 1) * frametime;

	//------------ Read pitch_time -----------
	if ( !Controls.state.slide_on )
	{
		// From keyboard...
		adjust_ramped_keyboard_field(plus, key_pitch_forward, Controls.pitch_time, PlayerCfg.KeyboardSens[1], speed_factor, 2);
		adjust_ramped_keyboard_field(minus, key_pitch_backward, Controls.pitch_time, PlayerCfg.KeyboardSens[1], speed_factor, 2);
		// From joystick...
#ifdef dxx_kconfig_ui_kc_joystick_pitch
		adjust_axis_field(Controls.pitch_time, Controls.joy_axis, kcm_joystick[dxx_kconfig_ui_kc_joystick_pitch].value, kcm_joystick[dxx_kconfig_ui_kc_joystick_invert_pitch].value, PlayerCfg.JoystickSens[1]);
#endif
		// From mouse...
#ifdef dxx_kconfig_ui_kc_mouse_pitch
		adjust_axis_field(Controls.pitch_time, Controls.mouse_axis, kcm_mouse[dxx_kconfig_ui_kc_mouse_pitch].value, kcm_mouse[dxx_kconfig_ui_kc_mouse_invert_pitch].value, PlayerCfg.MouseSens[1]);
#endif
	}
	else Controls.pitch_time = 0;


	//----------- Read vertical_thrust_time -----------------
	if ( Controls.state.slide_on )
	{
		// From keyboard...
		adjust_ramped_keyboard_field(plus, key_pitch_forward, Controls.vertical_thrust_time, PlayerCfg.KeyboardSens[3], speed_factor);
		adjust_ramped_keyboard_field(minus, key_pitch_backward, Controls.vertical_thrust_time, PlayerCfg.KeyboardSens[3], speed_factor);
		// From joystick...
		// NOTE: Use Slide U/D invert setting
#if defined(dxx_kconfig_ui_kc_joystick_pitch) && defined(dxx_kconfig_ui_kc_joystick_slide_ud)
		adjust_axis_field(Controls.vertical_thrust_time, Controls.joy_axis, kcm_joystick[dxx_kconfig_ui_kc_joystick_pitch].value, !kcm_joystick[dxx_kconfig_ui_kc_joystick_invert_slide_ud].value, PlayerCfg.JoystickSens[3]);
#endif
		// From mouse...
		adjust_axis_field(Controls.vertical_thrust_time, Controls.mouse_axis, kcm_mouse[dxx_kconfig_ui_kc_mouse_pitch].value, kcm_mouse[dxx_kconfig_ui_kc_mouse_invert_slide_ud].value, PlayerCfg.MouseSens[3]);
	}
	// From keyboard...
	adjust_ramped_keyboard_field(plus, key_slide_up, Controls.vertical_thrust_time, PlayerCfg.KeyboardSens[3], speed_factor);
	adjust_ramped_keyboard_field(minus, key_slide_down, Controls.vertical_thrust_time, PlayerCfg.KeyboardSens[3], speed_factor);
	// From buttons...
	adjust_button_time(Controls.vertical_thrust_time, Controls.state.btn_slide_up, Controls.state.btn_slide_down, speed_factor);
	// From joystick...
#ifdef dxx_kconfig_ui_kc_joystick_slide_ud
	adjust_axis_field(Controls.vertical_thrust_time, Controls.joy_axis, kcm_joystick[dxx_kconfig_ui_kc_joystick_slide_ud].value, !kcm_joystick[dxx_kconfig_ui_kc_joystick_invert_slide_ud].value, PlayerCfg.JoystickSens[3]);
#endif
	// From mouse...
	adjust_axis_field(Controls.vertical_thrust_time, Controls.mouse_axis, kcm_mouse[dxx_kconfig_ui_kc_mouse_slide_ud].value, !kcm_mouse[dxx_kconfig_ui_kc_mouse_invert_slide_ud].value, PlayerCfg.MouseSens[3]);

	//---------- Read heading_time -----------
	if (!Controls.state.slide_on && !Controls.state.bank_on)
	{
		// From keyboard...
		adjust_ramped_keyboard_field(plus, key_heading_right, Controls.heading_time, PlayerCfg.KeyboardSens[0], speed_factor);
		adjust_ramped_keyboard_field(minus, key_heading_left, Controls.heading_time, PlayerCfg.KeyboardSens[0], speed_factor);
		// From joystick...
#ifdef dxx_kconfig_ui_kc_joystick_turn
		adjust_axis_field(Controls.heading_time, Controls.joy_axis, kcm_joystick[dxx_kconfig_ui_kc_joystick_turn].value, !kcm_joystick[dxx_kconfig_ui_kc_joystick_invert_turn].value, PlayerCfg.JoystickSens[0]);
#endif
		// From mouse...
		adjust_axis_field(Controls.heading_time, Controls.mouse_axis, kcm_mouse[dxx_kconfig_ui_kc_mouse_turn].value, !kcm_mouse[dxx_kconfig_ui_kc_mouse_invert_turn].value, PlayerCfg.MouseSens[0]);
	}
	else Controls.heading_time = 0;

	//----------- Read sideways_thrust_time -----------------
	if ( Controls.state.slide_on )
	{
		// From keyboard...
		adjust_ramped_keyboard_field(plus, key_heading_right, Controls.sideways_thrust_time, PlayerCfg.KeyboardSens[2], speed_factor);
		adjust_ramped_keyboard_field(minus, key_heading_left, Controls.sideways_thrust_time, PlayerCfg.KeyboardSens[2], speed_factor);
		// From joystick...
#if defined(dxx_kconfig_ui_kc_joystick_turn) && defined(dxx_kconfig_ui_kc_joystick_slide_lr)
		adjust_axis_field(Controls.sideways_thrust_time, Controls.joy_axis, kcm_joystick[dxx_kconfig_ui_kc_joystick_turn].value, !kcm_joystick[dxx_kconfig_ui_kc_joystick_invert_slide_lr].value, PlayerCfg.JoystickSens[2]);
#endif
		// From mouse...
		adjust_axis_field(Controls.sideways_thrust_time, Controls.mouse_axis, kcm_mouse[dxx_kconfig_ui_kc_mouse_turn].value, !kcm_mouse[dxx_kconfig_ui_kc_mouse_invert_slide_lr].value, PlayerCfg.MouseSens[2]);
	}
	// From keyboard...
	adjust_ramped_keyboard_field(plus, key_slide_right, Controls.sideways_thrust_time, PlayerCfg.KeyboardSens[2], speed_factor);
	adjust_ramped_keyboard_field(minus, key_slide_left, Controls.sideways_thrust_time, PlayerCfg.KeyboardSens[2], speed_factor);
	// From buttons...
	adjust_button_time(Controls.sideways_thrust_time, Controls.state.btn_slide_right, Controls.state.btn_slide_left, speed_factor);
	// From joystick...
#ifdef dxx_kconfig_ui_kc_joystick_slide_lr
	adjust_axis_field(Controls.sideways_thrust_time, Controls.joy_axis, kcm_joystick[dxx_kconfig_ui_kc_joystick_slide_lr].value, !kcm_joystick[dxx_kconfig_ui_kc_joystick_invert_slide_lr].value, PlayerCfg.JoystickSens[2]);
#endif
	// From mouse...
	adjust_axis_field(Controls.sideways_thrust_time, Controls.mouse_axis, kcm_mouse[dxx_kconfig_ui_kc_mouse_slide_lr].value, !kcm_mouse[dxx_kconfig_ui_kc_mouse_invert_slide_lr].value, PlayerCfg.MouseSens[2]);

	//----------- Read bank_time -----------------
	if ( Controls.state.bank_on )
	{
		// From keyboard...
		adjust_ramped_keyboard_field(plus, key_heading_left, Controls.bank_time, PlayerCfg.KeyboardSens[4], speed_factor);
		adjust_ramped_keyboard_field(minus, key_heading_right, Controls.bank_time, PlayerCfg.KeyboardSens[4], speed_factor);
		// From joystick...
#if defined(dxx_kconfig_ui_kc_joystick_turn) && defined(dxx_kconfig_ui_kc_joystick_bank)
		adjust_axis_field(Controls.bank_time, Controls.joy_axis, kcm_joystick[dxx_kconfig_ui_kc_joystick_turn].value, kcm_joystick[dxx_kconfig_ui_kc_joystick_invert_bank].value, PlayerCfg.JoystickSens[4]);
#endif
		// From mouse...
		adjust_axis_field(Controls.bank_time, Controls.mouse_axis, kcm_mouse[dxx_kconfig_ui_kc_mouse_turn].value, !kcm_mouse[dxx_kconfig_ui_kc_mouse_invert_bank].value, PlayerCfg.MouseSens[4]);
	}
	// From keyboard...
	adjust_ramped_keyboard_field(plus, key_bank_left, Controls.bank_time, PlayerCfg.KeyboardSens[4], speed_factor);
	adjust_ramped_keyboard_field(minus, key_bank_right, Controls.bank_time, PlayerCfg.KeyboardSens[4], speed_factor);
	// From buttons...
	adjust_button_time(Controls.bank_time, Controls.state.btn_bank_left, Controls.state.btn_bank_right, speed_factor);
	// From joystick...
#ifdef dxx_kconfig_ui_kc_joystick_bank
	adjust_axis_field(Controls.bank_time, Controls.joy_axis, kcm_joystick[dxx_kconfig_ui_kc_joystick_bank].value, kcm_joystick[dxx_kconfig_ui_kc_joystick_invert_bank].value, PlayerCfg.JoystickSens[4]);
#endif
	// From mouse...
	adjust_axis_field(Controls.bank_time, Controls.mouse_axis, kcm_mouse[dxx_kconfig_ui_kc_mouse_bank].value, !kcm_mouse[dxx_kconfig_ui_kc_mouse_invert_bank].value, PlayerCfg.MouseSens[4]);

	//----------- Read forward_thrust_time -------------
	// From keyboard/buttons...
	adjust_button_time(Controls.forward_thrust_time, Controls.state.accelerate, Controls.state.reverse, speed_factor);
	// From joystick...
#ifdef dxx_kconfig_ui_kc_joystick_throttle
	adjust_axis_field(Controls.forward_thrust_time, Controls.joy_axis, kcm_joystick[dxx_kconfig_ui_kc_joystick_throttle].value, kcm_joystick[dxx_kconfig_ui_kc_joystick_invert_throttle].value, PlayerCfg.JoystickSens[5]);
#endif
	// From mouse...
	adjust_axis_field(Controls.forward_thrust_time, Controls.mouse_axis, kcm_mouse[dxx_kconfig_ui_kc_mouse_throttle].value, kcm_mouse[dxx_kconfig_ui_kc_mouse_invert_throttle].value, PlayerCfg.MouseSens[5]);

	//----------- Read cruise-control-type of throttle.
	if ( Controls.state.cruise_off > 0 ) Controls.state.cruise_off = Cruise_speed = 0;
	else
	{
		adjust_button_time(Cruise_speed, Controls.state.cruise_plus, Controls.state.cruise_minus, speed_factor * 80);
		clamp_value(Cruise_speed, 0, i2f(100));
		if (Controls.forward_thrust_time == 0)
			Controls.forward_thrust_time = fixmul(Cruise_speed, frametime) / 100;
	}

	//----------- Clamp values between -FrameTime and FrameTime
	clamp_kconfig_control_with_overrun(Controls.vertical_thrust_time, frametime, Controls.excess_vertical_thrust_time, frametime * PlayerCfg.MouseOverrun[3]);
	clamp_kconfig_control_with_overrun(Controls.sideways_thrust_time, frametime, Controls.excess_sideways_thrust_time, frametime * PlayerCfg.MouseOverrun[2]);
	clamp_kconfig_control_with_overrun(Controls.forward_thrust_time, frametime, Controls.excess_forward_thrust_time, frametime * PlayerCfg.MouseOverrun[5]);
	if (!allow_uncapped_turning())
	{
		clamp_kconfig_control_with_overrun(Controls.pitch_time, frametime / 2, Controls.excess_pitch_time, frametime * PlayerCfg.MouseOverrun[1]);
		clamp_kconfig_control_with_overrun(Controls.heading_time, frametime, Controls.excess_heading_time, frametime * PlayerCfg.MouseOverrun[0]);
		clamp_kconfig_control_with_overrun(Controls.bank_time, frametime, Controls.excess_bank_time, frametime * PlayerCfg.MouseOverrun[4]);
	}
}

}

void reset_cruise(void)
{
	Cruise_speed=0;
}


void kc_set_controls()
{
	for (auto &&[kcm, setting] : zip(kcm_keyboard, PlayerCfg.KeySettings.Keyboard))
		kcm.oldvalue = kcm.value = setting;

#if DXX_MAX_JOYSTICKS
	for (auto &&[kcm, kc, setting] : zip(kcm_joystick, kc_joystick, PlayerCfg.KeySettings.Joystick))
	{
		uint8_t value = setting;
		if (kc.type == BT_INVERT)
		{
			if (value != 1)
				value = 0;
			setting = value;
		}
		kcm.oldvalue = kcm.value = value;
	}
#endif

	for (auto &&[kcm, kc, setting] : zip(kcm_mouse, kc_mouse, PlayerCfg.KeySettings.Mouse))
	{
		uint8_t value = setting;
		if (kc.type == BT_INVERT)
		{
			if (value != 1)
				value = 0;
			setting = value;
		}
		kcm.oldvalue = kcm.value = value;
	}

	for (auto &&[kcm, setting] : zip(kcm_rebirth, PlayerCfg.KeySettingsRebirth))
		kcm.oldvalue = kcm.value = setting;
}
