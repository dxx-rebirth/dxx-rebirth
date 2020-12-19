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


#include <stdio.h>
#include <stdarg.h>

#include "maths.h"
#include "pstypes.h"
#include "gr.h"
#include "event.h"
#include "ui.h"
#include "mouse.h"
#include "key.h"
#include "u_mem.h"

#include "dxxsconf.h"
#include "dsx-ns.h"
#include <array>
#include <memory>

namespace dcx {

// ts = total span
// w = width of each item
// n = number of items
// i = item number (0 based)
#define EVEN_DIVIDE(ts,w,n,i) ((((ts)-((w)*(n)))*((i)+1))/((n)+1))+((w)*(i))

#define BUTTON_HORZ_SPACING 20
#define TEXT_EXTRA_HEIGHT 5

namespace {

struct messagebox : UI_DIALOG
{
	using UI_DIALOG::UI_DIALOG;
	const ui_messagebox_tie	*button = nullptr;
	std::array<std::unique_ptr<UI_GADGET_BUTTON>, 10> button_g;
	const char *text = nullptr;
	int *choice = nullptr;
	int					width;
	int					text_y;
	int					line_y;
	virtual window_event_result callback_handler(const d_event &) override;
};

window_event_result messagebox::callback_handler(const d_event &event)
{
	if (event.type == EVENT_UI_DIALOG_DRAW)
	{
		const grs_font * temp_font;

		gr_set_current_canvas(grd_curscreen->sc_canvas);
		auto &canvas = *grd_curcanv;
		temp_font = grd_curscreen->sc_canvas.cv_font;
		
		if (grd_curscreen->get_screen_width() < 640) {
			grd_curscreen->sc_canvas.cv_font = ui_small_font.get();
		}
		
		ui_dialog_set_current_canvas(*this);
		ui_string_centered(canvas, width / 2, text_y, text);
		
		Hline(canvas, 1, width - 2, line_y + 1, CGREY);
		Hline(canvas, 2, width - 2, line_y + 2, CBRIGHT);

		grd_curscreen->sc_canvas.cv_font = temp_font;
		
		return window_event_result::handled;
	}

	for (uint_fast32_t i=0; i < button->count(); i++ )
	{
		if (GADGET_PRESSED(button_g[i].get()))
		{
			*(choice) = i+1;
			return window_event_result::handled;
		}
	}
	
	return window_event_result::ignored;
}

}

int (ui_messagebox)( short xc, short yc, const char * text, const ui_messagebox_tie &Button )
{
	int avg, x, y;
	int button_width, button_height, text_height, text_width;
	int w, h;

	int choice;

	button_width = button_height = 0;

	gr_set_current_canvas(grd_curscreen->sc_canvas);
	auto &canvas = *grd_curcanv;
	auto &cv_font = *canvas.cv_font;
	w = grd_curscreen->get_screen_width();
	h = grd_curscreen->get_screen_height();

	for (uint_fast32_t i=0; i < Button.count(); i++ )
	{
		int width, height;
		ui_get_button_size(cv_font, Button.string(i), width, height);

		if ( width > button_width ) button_width = width;
		if ( height > button_height ) button_height = height;
	}

	gr_get_string_size(cv_font, text, &text_width, &text_height, &avg);

	unsigned width, height;
	width = button_width*Button.count();
	width += BUTTON_HORZ_SPACING*(Button.count()+1);
	width ++;

	text_width += avg*6;
	text_width += 10;

	if (text_width > width )
		width = text_width;

	height = text_height;
	height += button_height;
	height += 4*TEXT_EXTRA_HEIGHT;
	height += 2;  // For line in middle

	{
		int mx, my, mz;
		
		mouse_get_pos(&mx, &my, &mz);

		if ( xc == -1 )
			xc = mx;

		if ( yc == -1 )
			yc = my - button_height/2;
	}

	if ( xc == -2 )
		xc = w/2;

	if ( yc == -2 )
		yc = h/2;

	x = xc - width/2;
	y = yc - height/2;

	if (x < 0 ) {
		x = 0;
	}

	if ( (x+width-1) >= w ) {
		x = w - width;
	}

	if (y < 0 ) {
		y = 0;
	}

	if ( (y+height-1) >= h ) {
		y = h - height;
	}

	const auto dlg = window_create<messagebox>(x, y, width, height, static_cast<dialog_flags>(DF_DIALOG | DF_MODAL));
	dlg->button = &Button;
	dlg->text = text;
	dlg->choice = &choice;

	y = TEXT_EXTRA_HEIGHT + text_height/2 - 1;

	dlg->width = width;
	dlg->text_y = y;

	y = 2*TEXT_EXTRA_HEIGHT + text_height;
	
	dlg->line_y = y;

	y = height - TEXT_EXTRA_HEIGHT - button_height;

	for (uint_fast32_t i=0; i < Button.count(); i++ )
	{

		x = EVEN_DIVIDE(width,button_width,Button.count(),i);

		dlg->button_g[i] = ui_add_gadget_button(*dlg, x, y, button_width, button_height, Button.string(i), nullptr);
	}

	ui_gadget_calc_keys(*dlg);

	//key_flush();

	dlg->keyboard_focus_gadget = dlg->button_g[0].get();

	choice = 0;

	while(choice==0)
		event_process();

	ui_close_dialog(*dlg);
	return choice;
}

}
