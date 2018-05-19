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
#include "physfsx.h"
#include "maths.h"
#include "pstypes.h"
#include "gr.h"
#include "key.h"
#include "ui.h"
#include "u_mem.h"
#include "func.h"
#include "dxxerror.h"

#include "compiler-make_unique.h"
#include "compiler-range_for.h"

namespace dcx {

#define MAX_NUM_PADS 20

static array<std::unique_ptr<UI_GADGET_BUTTON>, 17> Pad;
static array<std::unique_ptr<UI_KEYPAD>, MAX_NUM_PADS> KeyPad;
static int active_pad;

static int desc_x, desc_y;

static array<int, 17> HotKey, HotKey1;

int ui_pad_get_current()
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

typedef PHYSFSX_gets_line_t<100>::line_t keypad_input_line_t;

static keypad_input_line_t::const_iterator find_fake_comma(keypad_input_line_t::const_iterator i, keypad_input_line_t::const_iterator e)
{
	auto is_fake_comma = [](char c) {
		return !c || static_cast<uint8_t>(c) == 179;
	};
	return std::find_if(i, e, is_fake_comma);
}

template <bool append, char eor>
static keypad_input_line_t::const_iterator set_row(keypad_input_line_t::const_iterator i, const keypad_input_line_t::const_iterator e, UI_KEYPAD::buttontext_element_t &r)
{
	const auto oe = r.end();
	auto ob = r.begin();
	if (append)
		ob = std::find(ob, oe, 0);
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

static std::unique_ptr<UI_GADGET_BUTTON> ui_create_pad_gadget(UI_DIALOG &dlg, uint_fast32_t x, uint_fast32_t y, uint_fast32_t w, uint_fast32_t h, const grs_font &font)
{
	auto r = ui_add_gadget_button(&dlg, x, y, w, h, nullptr, nullptr);
	r->canvas->cv_font = &font;
	return r;
}

void ui_pad_activate(UI_DIALOG &dlg, uint_fast32_t x, uint_fast32_t y)
{
	const uint_fast32_t bw = 56;
	const uint_fast32_t bh = 30;
	const uint_fast32_t x5 = x + 5;
	const uint_fast32_t y20 = y + 20;
	const auto &font = *ui_small_font.get();
	const auto fx = [=](uint_fast32_t col) {
		return x5 + (bw * col);
	};
	const auto fy = [=](uint_fast32_t row) {
		return y20 + (bh * row);
	};
	const auto fw = [=](uint_fast32_t w) {
		return bw * w;
	};
	const auto fh = [=](uint_fast32_t h) {
		return bh * h;
	};
	const auto ui_add_pad_gadget = [&dlg, &font](uint_fast32_t n, uint_fast32_t gx, uint_fast32_t gy, uint_fast32_t w, uint_fast32_t h) {
		Pad[n] = ui_create_pad_gadget(dlg, gx, gy, w, h, font);
	};

	int w,h,row,col, n;

	desc_x = x+2;
	desc_y = y-17;

	n=0; row = 0; col = 0; w = 1; h = 1;
	ui_add_pad_gadget(n, fx(col), fy(row), fw(w), fh(h));
	n=1; row = 0; col = 1; w = 1; h = 1;
	ui_add_pad_gadget(n, fx(col), fy(row), fw(w), fh(h));
	n=2; row = 0; col = 2; w = 1; h = 1;
	ui_add_pad_gadget(n, fx(col), fy(row), fw(w), fh(h));
	n=3; row = 0; col = 3; w = 1; h = 1;
	ui_add_pad_gadget(n, fx(col), fy(row), fw(w), fh(h));
	n=4; row = 1; col = 0; w = 1; h = 1; 
	ui_add_pad_gadget(n, fx(col), fy(row), fw(w), fh(h));
	n=5; row = 1; col = 1; w = 1; h = 1; 
	ui_add_pad_gadget(n, fx(col), fy(row), fw(w), fh(h));
	n=6; row = 1; col = 2; w = 1; h = 1; 
	ui_add_pad_gadget(n, fx(col), fy(row), fw(w), fh(h));
	n=7; row = 1; col = 3; w = 1; h = 2; 
	ui_add_pad_gadget(n, fx(col), fy(row), fw(w), fh(h));
	n=8; row = 2; col = 0; w = 1; h = 1; 
	ui_add_pad_gadget(n, fx(col), fy(row), fw(w), fh(h));
	n=9; row = 2; col = 1; w = 1; h = 1; 
	ui_add_pad_gadget(n, fx(col), fy(row), fw(w), fh(h));
	n=10; row = 2; col = 2; w = 1; h = 1;
	ui_add_pad_gadget(n, fx(col), fy(row), fw(w), fh(h));
	n=11; row = 3; col = 0; w = 1; h = 1;
	ui_add_pad_gadget(n, fx(col), fy(row), fw(w), fh(h));
	n=12; row = 3; col = 1; w = 1; h = 1;
	ui_add_pad_gadget(n, fx(col), fy(row), fw(w), fh(h));
	n=13; row = 3; col = 2; w = 1; h = 1;
	ui_add_pad_gadget(n, fx(col), fy(row), fw(w), fh(h));
	n=14; row = 3; col = 3; w = 1; h = 2; 
	ui_add_pad_gadget(n, fx(col), fy(row), fw(w), fh(h));
	n=15; row = 4; col = 0; w = 2; h = 1;
	ui_add_pad_gadget(n, fx(col), fy(row), fw(w), fh(h));
	n=16; row = 4; col = 2; w = 1; h = 1; 
	ui_add_pad_gadget(n, fx(col), fy(row), fw(w), fh(h));

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
	
	ui_dialog_set_current_canvas( dlg );
	ui_draw_box_in(*grd_curcanv, x, y, x+(bw * 4)+10 + 200, y+(bh * 5)+45);

	gr_set_default_canvas();
	auto &canvas = *grd_curcanv;
	const uint8_t color = CWHITE;
	gr_urect(canvas, desc_x, desc_y, desc_x+ 56*4-1, desc_y+15, color);
	gr_set_fontcolor(canvas, CBLACK, CWHITE);
	gr_ustring(canvas, *canvas.cv_font, desc_x, desc_y, KeyPad[active_pad]->description.data());
}

static void ui_pad_set_active( int n )
{
	int np;
	const char * name;
	

	for (int i=0; i<17; i++ )
	{
		Pad[i]->text = KeyPad[n]->buttontext[i].data();
		Pad[i]->status = 1;
		Pad[i]->user_function = NULL;
		Pad[i]->dim_if_no_function = 1;
		Pad[i]->hotkey = -1;
				
		for (int j=0; j< KeyPad[n]->numkeys; j++ )
		{
			if (HotKey[i] == KeyPad[n]->keycode[j] )
			{
				Pad[i]->hotkey =  HotKey[i];
				Pad[i]->user_function = func_nget( KeyPad[n]->function_number[j], &np, &name );
			}
			if (HotKey1[i] == KeyPad[n]->keycode[j] )
			{
				Pad[i]->hotkey1 =  HotKey1[i];
				Pad[i]->user_function1 = func_nget( KeyPad[n]->function_number[j], &np, &name );
			}
		}
	}

	active_pad = n;
}

void ui_pad_goto(int n)
{
	if ( KeyPad[n] != NULL )
		ui_pad_set_active(n);
}

void ui_pad_goto_next()
{
	int i, si;

	i = active_pad + 1;
	si = i;
	
	while( KeyPad[i]==NULL )
	{
		i++;
		if (i >= MAX_NUM_PADS)
			i = 0;
		if (i == si )
			break;
	}
	ui_pad_set_active(i);
}

void ui_pad_goto_prev()
{
	int i;

	if (active_pad == -1 ) 
		active_pad = MAX_NUM_PADS;
	
	i = active_pad - 1;
	if (i<0) i= MAX_NUM_PADS - 1;
	
	while( KeyPad[i]==NULL )
	{
		i--;
		if (i < 0)
			i = MAX_NUM_PADS-1;
		if (i == active_pad )
			break;
	}
	ui_pad_set_active(i);
}

UI_KEYPAD::UI_KEYPAD() :
	numkeys(0)
{
	description.front() = 0;
	range_for (auto &i, buttontext)
		i[0] = 0;
}

int ui_pad_read( int n, const char * filename )
{
	int linenumber = 0;
	int keycode, functionnumber;

	auto infile = PHYSFSX_openReadBuffered(filename);
	if (!infile) {
		Warning( "Could not find %s\n", filename );
		return 0;
	}
	auto &kpn = *(KeyPad[n] = make_unique<UI_KEYPAD>());

	PHYSFSX_gets_line_t<100> buffer;
	while ( linenumber < 22)
	{
		PHYSFSX_fgets( buffer, infile );

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

	PHYSFSX_gets_line_t<200> line_buffer;
	while (PHYSFSX_fgets(line_buffer, infile))
	{
		if (!line_buffer[0])
			continue;
		PHYSFSX_gets_line_t<100> text;
		sscanf(line_buffer, " %99s %99s ", text.next().data(), buffer.next().data());
		keycode = DecodeKeyText(text);
		functionnumber = func_get_index(buffer);
		if (functionnumber==-1)
		{
			UserError( "Unknown function, %s, in %s\n", static_cast<const char *>(buffer), filename );
		} else if (keycode==-1)
		{
			UserError( "Unknown keystroke, %s, in %s\n", static_cast<const char *>(text), filename );
			//ui_messagebox( -2, -2, 1, buffer, "Ok" );

		} else {
			kpn.keycode[kpn.numkeys] = keycode;
			kpn.function_number[kpn.numkeys] = functionnumber;
			kpn.numkeys++;
		}
	}

	return 1;
}

}
