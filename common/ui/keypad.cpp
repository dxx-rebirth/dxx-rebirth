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

#include <memory>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ranges>
#include <utility>
#include "physfsx.h"
#include "maths.h"
#include "pstypes.h"
#include "gr.h"
#include "key.h"
#include "ui.h"
#include "u_mem.h"
#include "func.h"
#include "dxxerror.h"

#include "compiler-range_for.h"
#include "d_range.h"
#include "d_enumerate.h"
#include <memory>

namespace dcx {
namespace {

#define MAX_NUM_PADS 20

static std::array<std::unique_ptr<UI_GADGET_BUTTON>, 17> Pad;
static std::array<std::unique_ptr<UI_KEYPAD>, MAX_NUM_PADS> KeyPad;
static std::size_t active_pad;

static int desc_x, desc_y;

static std::array<int, 17> HotKey, HotKey1;

}

std::size_t ui_pad_get_current()
{
	return active_pad;
}

void ui_pad_init()
{
	KeyPad = {};
	active_pad = 0;
}

void ui_pad_close()
{
	KeyPad = {};
}

namespace {

typedef PHYSFSX_gets_line_t<100>::line_t keypad_input_line_t;

static keypad_input_line_t::const_iterator find_fake_comma(keypad_input_line_t::const_iterator i, keypad_input_line_t::const_iterator e)
{
	const auto is_fake_comma = [](const char c) {
		return !c || static_cast<uint8_t>(c) == 179;
	};
	return std::ranges::find_if(i, e, is_fake_comma);
}

template <bool append, char eor>
static keypad_input_line_t::const_iterator set_row(keypad_input_line_t::const_iterator i, const keypad_input_line_t::const_iterator e, UI_KEYPAD::buttontext_element_t &r)
{
	const auto oe = r.end();
	auto ob = r.begin();
	if (append)
		ob = std::ranges::find(ob, oe, 0);
	auto comma0 = find_fake_comma(i, e);
	if (comma0 == e)
		/* Start not found */
		return comma0;
	const auto comma1 = find_fake_comma(++ comma0, e);
	std::size_t id = std::distance(comma0, comma1);
	std::size_t od = std::distance(ob, oe);
	if (!od)
		/* Output buffer full */
		return comma1;
	-- od;
	std::size_t md = std::min(id, od);
	std::copy_n(comma0, md, ob);
	std::advance(ob, md);
	assert(ob != oe);
	if (ob == oe)
		-- ob;
	if (eor)
	{
		/* Add EOR if room */
		auto on = std::next(ob);
		if (on != oe)
			*ob++ = eor;
	}
	*ob = 0;
	return comma1;
}

template <bool append, char eor, typename... T>
static keypad_input_line_t::const_iterator set_row(keypad_input_line_t::const_iterator i, const keypad_input_line_t::const_iterator e, UI_KEYPAD::buttontext_element_t &r, T &... t)
{
	return set_row<append, eor>(set_row<append, eor>(i, e, r), e, t...);
}

static void set_short_row(keypad_input_line_t::const_iterator i, const keypad_input_line_t::const_iterator e, UI_KEYPAD::buttontext_element_t &r)
{
	typedef std::reverse_iterator<keypad_input_line_t::const_iterator> reverse_iterator;
	const auto oe = r.end();
	auto ob = std::find(r.begin(), oe, 0);
	std::size_t od = std::distance(ob, oe);
	if (!od)
		return;
	-- od;
	auto ie = std::find(i, e, 0);
	auto ri = reverse_iterator(i);
	auto comma0 = std::find(reverse_iterator(ie), ri, 179);
	if (comma0 == ri)
		return;
	auto comma1 = std::find(++ comma0, ri, 180);
	if (comma1 == ri)
		return;
	auto bcomma1 = comma1.base();
	std::size_t id = std::distance(comma0.base(), bcomma1);
	std::size_t md = std::min(id, od);
	std::copy_n(bcomma1, md, ob);
	std::advance(ob, md);
	assert(ob != oe);
	if (ob == oe)
		-- ob;
	auto on = std::next(ob);
	if (on != oe)
		*ob++ = '\n';
	*ob = 0;
}

struct pad_button_layout
{
	short col, row, width, height;
};

constexpr std::array<pad_button_layout, 17> button_layout{{
	{0, 0, 1, 1},
	{1, 0, 1, 1},
	{2, 0, 1, 1},
	{3, 0, 1, 1},
	{0, 1, 1, 1},
	{1, 1, 1, 1},
	{2, 1, 1, 1},
	{3, 1, 1, 2},
	{0, 2, 1, 1},
	{1, 2, 1, 1},
	{2, 2, 1, 1},
	{0, 3, 1, 1},
	{1, 3, 1, 1},
	{2, 3, 1, 1},
	{3, 3, 1, 2},
	{0, 4, 2, 1},
	{2, 4, 1, 1},
}};

}

void ui_pad_activate(UI_DIALOG &dlg, const int x, const int y)
{
	constexpr int bw = 56;
	constexpr int bh = 30;
	const auto x5 = x + 5;
	const auto y20 = y + 20;
	const auto &font = *ui_small_font.get();

	desc_x = x + 2;
	desc_y = y - 17;

	for (auto &&[n, button] : enumerate(button_layout))
	{
		auto gadget = ui_add_gadget_button(dlg, x5 + bw * button.col, y20 + bh * button.row, bw * button.width, bh * button.height, nullptr, nullptr);
		gadget->canvas->cv_font = &font;
		Pad[n] = std::move(gadget);
	}

	HotKey[0] = KEY_CTRLED + KEY_NUMLOCK;
	HotKey[1] = KEY_CTRLED + KEY_PADDIVIDE;
	HotKey[2] = KEY_CTRLED + KEY_PADMULTIPLY;
	HotKey[3] = KEY_CTRLED + KEY_PADMINUS;
	HotKey[4] = KEY_CTRLED + KEY_PAD7;
	HotKey[5] = KEY_CTRLED + KEY_PAD8;
	HotKey[6] = KEY_CTRLED + KEY_PAD9;
	HotKey[7] = KEY_CTRLED + KEY_PADPLUS;
	HotKey[8] = KEY_CTRLED + KEY_PAD4;
	HotKey[9] = KEY_CTRLED + KEY_PAD5;
	HotKey[10] = KEY_CTRLED + KEY_PAD6;
	HotKey[11] = KEY_CTRLED + KEY_PAD1;
	HotKey[12] = KEY_CTRLED + KEY_PAD2;
	HotKey[13] = KEY_CTRLED + KEY_PAD3;
	HotKey[14] = KEY_CTRLED + KEY_PADENTER;
	HotKey[15] = KEY_CTRLED + KEY_PAD0;
	HotKey[16] = KEY_CTRLED + KEY_PADPERIOD;

	HotKey1[0] = KEY_SHIFTED + KEY_CTRLED + KEY_NUMLOCK;
	HotKey1[1] = KEY_SHIFTED + KEY_CTRLED + KEY_PADDIVIDE;
	HotKey1[2] = KEY_SHIFTED + KEY_CTRLED + KEY_PADMULTIPLY;
	HotKey1[3] = KEY_SHIFTED + KEY_CTRLED + KEY_PADMINUS;
	HotKey1[4] = KEY_SHIFTED + KEY_CTRLED + KEY_PAD7;
	HotKey1[5] = KEY_SHIFTED + KEY_CTRLED + KEY_PAD8;
	HotKey1[6] = KEY_SHIFTED + KEY_CTRLED + KEY_PAD9;
	HotKey1[7] = KEY_SHIFTED + KEY_CTRLED + KEY_PADPLUS;
	HotKey1[8] = KEY_SHIFTED + KEY_CTRLED + KEY_PAD4;
	HotKey1[9] = KEY_SHIFTED + KEY_CTRLED + KEY_PAD5;
	HotKey1[10] = KEY_SHIFTED + KEY_CTRLED + KEY_PAD6;
	HotKey1[11] = KEY_SHIFTED + KEY_CTRLED + KEY_PAD1;
	HotKey1[12] = KEY_SHIFTED + KEY_CTRLED + KEY_PAD2;
	HotKey1[13] = KEY_SHIFTED + KEY_CTRLED + KEY_PAD3;
	HotKey1[14] = KEY_SHIFTED + KEY_CTRLED + KEY_PADENTER;
	HotKey1[15] = KEY_SHIFTED + KEY_CTRLED + KEY_PAD0;
	HotKey1[16] = KEY_SHIFTED + KEY_CTRLED + KEY_PADPERIOD;

	active_pad = 0;

}


void ui_pad_deactivate()
{
	range_for (auto &i, Pad)
	{
		i->text.clear();
	}
}

void ui_pad_draw(UI_DIALOG *dlg, int x, int y)
{
	int bh, bw;
	
	bw = 56; bh = 30;
	
	ui_dialog_set_current_canvas(*dlg);
	ui_draw_box_in(*grd_curcanv, x, y, x+(bw * 4)+10 + 200, y+(bh * 5)+45);

	gr_set_default_canvas();
	auto &canvas = *grd_curcanv;
	const auto color{CWHITE};
	gr_urect(canvas, desc_x, desc_y, desc_x+ 56*4-1, desc_y+15, color);
	gr_set_fontcolor(canvas, CBLACK, CWHITE);
	gr_ustring(canvas, *canvas.cv_font, desc_x, desc_y, KeyPad[active_pad]->description.data());
}

namespace {

static void ui_pad_set_active(const std::size_t n)
{
	int np;
	const char * name;
	

	for (const auto i : xrange(Pad.size()))
	{
		Pad[i]->text = KeyPad[n]->buttontext[i].data();
		Pad[i]->user_function = NULL;
		Pad[i]->dim_if_no_function = 1;
		Pad[i]->hotkey = -1;
				
		for (std::size_t j = 0; j < KeyPad[n]->numkeys; ++j)
		{
			if (std::cmp_equal(HotKey[i], KeyPad[n]->keycode[j]))
			{
				Pad[i]->hotkey =  HotKey[i];
				Pad[i]->user_function = func_nget( KeyPad[n]->function_number[j], &np, &name );
			}
			if (std::cmp_equal(HotKey1[i], KeyPad[n]->keycode[j]))
			{
				Pad[i]->hotkey1 =  HotKey1[i];
				Pad[i]->user_function1 = func_nget( KeyPad[n]->function_number[j], &np, &name );
			}
		}
	}

	active_pad = n;
}

}

void ui_pad_goto(const std::size_t n)
{
	if (n < KeyPad.size() && KeyPad[n])
		ui_pad_set_active(n);
}

void ui_pad_goto_next()
{
	auto i = active_pad;
	do
	{
		if (++i == KeyPad.size())
			i = 0;
		if (KeyPad[i])
		{
			ui_pad_set_active(i);
			return;
		}
	} while (i != active_pad);
}

void ui_pad_goto_prev()
{
	auto i = active_pad;
	do
	{
		i = i ? i - 1 : KeyPad.size() - 1;
		if (KeyPad[i])
		{
			ui_pad_set_active(i);
			return;
		}
	} while (i != active_pad);
}

UI_KEYPAD::UI_KEYPAD() :
	numkeys(0)
{
	description.front() = 0;
	range_for (auto &i, buttontext)
		i[0] = 0;
}

int ui_pad_read(const std::size_t n, const char *filename)
{
	int linenumber{0};
	int keycode;
	if (n >= KeyPad.size())
		return 0;

	auto &&[infile, physfserr] = PHYSFSX_openReadBuffered(filename);
	if (!infile) {
		Warning("UI pad: failed to open \"%s\": %s", filename, PHYSFS_getErrorByCode(physfserr));
		return 0;
	}
	auto &kpn = *(KeyPad[n] = std::make_unique<UI_KEYPAD>());

	PHYSFSX_gets_line_t<100> buffer;
	while (linenumber < 22 && PHYSFSX_fgets(buffer, infile))
	{
		auto &line = buffer.line();
		const auto lb = line.begin();
		const auto le = line.end();
		switch( linenumber+1 )
		{
		case 1:
			kpn.description.copy_if(line);
			break;
		//===================== ROW 0 ==============================
		case 3:
			set_row<false, '\n'>(lb, le, kpn.buttontext[0], kpn.buttontext[1], kpn.buttontext[2], kpn.buttontext[3]);
			break;
		case 4:
			set_row<true, '\n'>(lb, le, kpn.buttontext[0], kpn.buttontext[1], kpn.buttontext[2], kpn.buttontext[3]);
			break;
		case 5:
			set_row<true, 0>(lb, le, kpn.buttontext[0], kpn.buttontext[1], kpn.buttontext[2], kpn.buttontext[3]);
			break;
		//===================== ROW 1 ==============================
		case 7:
			set_row<false, '\n'>(lb, le, kpn.buttontext[4], kpn.buttontext[5], kpn.buttontext[6], kpn.buttontext[7]);
			break;
		case 8:
			set_row<true, '\n'>(lb, le, kpn.buttontext[4], kpn.buttontext[5], kpn.buttontext[6], kpn.buttontext[7]);
			break;
		case 9:
			set_row<true, '\n'>(set_row<true, 0>(lb, le, kpn.buttontext[4], kpn.buttontext[5], kpn.buttontext[6]), le, kpn.buttontext[7]);
			break;
		case 10:
			set_short_row(lb, le, kpn.buttontext[7]);
			break;
		//======================= ROW 2 ==============================
		case 11:
			set_row<true, '\n'>(set_row<false, '\n'>(lb, le, kpn.buttontext[8], kpn.buttontext[9], kpn.buttontext[10]), le, kpn.buttontext[7]);
			break;
		case 12:
			set_row<true, '\n'>(lb, le, kpn.buttontext[8], kpn.buttontext[9], kpn.buttontext[10], kpn.buttontext[7]);
			break;
		case 13:
			set_row<true, 0>(lb, le, kpn.buttontext[8], kpn.buttontext[9], kpn.buttontext[10], kpn.buttontext[7]);
			break;
		// ====================== ROW 3 =========================
		case 15:
			set_row<false, '\n'>(lb, le, kpn.buttontext[11], kpn.buttontext[12], kpn.buttontext[13], kpn.buttontext[14]);
			break;
		case 16:
			set_row<true, '\n'>(lb, le, kpn.buttontext[11], kpn.buttontext[12], kpn.buttontext[13], kpn.buttontext[14]);
			break;
		case 17:
			set_row<true, '\n'>(set_row<true, 0>(lb, le, kpn.buttontext[11], kpn.buttontext[12], kpn.buttontext[13]), le, kpn.buttontext[14]);
			break;
		case 18:
			set_short_row(lb, le, kpn.buttontext[14]);
			break;
		//======================= ROW 4 =========================
		case 19:
			set_row<true, '\n'>(set_row<false, '\n'>(lb, le, kpn.buttontext[15], kpn.buttontext[16]), le, kpn.buttontext[14]);
			break;
		case 20:
			set_row<true, '\n'>(lb, le, kpn.buttontext[15], kpn.buttontext[16], kpn.buttontext[14]);
			break;
		case 21:
			set_row<true, 0>(lb, le, kpn.buttontext[15], kpn.buttontext[16], kpn.buttontext[14]);
			break;
		}
										
		linenumber++;	
	}

	// Get the keycodes...

	for (PHYSFSX_gets_line_t<200> line_buffer; PHYSFSX_fgets(line_buffer, infile);)
	{
		if (!line_buffer[0])
			continue;
		PHYSFSX_gets_line_t<100> text;
		sscanf(line_buffer, " %99s %99s ", text.next().data(), buffer.next().data());
		keycode = DecodeKeyText(text);
		const auto functionnumber = func_get_index(buffer);
		if (!functionnumber)
		{
			UserError( "Unknown function, %s, in %s\n", static_cast<const char *>(buffer), filename );
		} else if (keycode==-1)
		{
			UserError( "Unknown keystroke, %s, in %s\n", static_cast<const char *>(text), filename );
			//ui_messagebox( -2, -2, 1, buffer, "Ok" );

		} else {
			if (kpn.numkeys == kpn.keycode.size())
				UserError("Too many key bindings in %s", filename);
			kpn.keycode[kpn.numkeys] = keycode;
			kpn.function_number[kpn.numkeys] = *functionnumber;
			kpn.numkeys++;
		}
	}

	return 1;
}

}
