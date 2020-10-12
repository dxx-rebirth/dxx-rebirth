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


#include <stdlib.h>
#include <string.h>

#include "u_mem.h"
#include "maths.h"
#include "pstypes.h"
#include "event.h"
#include "gr.h"
#include "ui.h"
#include "key.h"
#include "mouse.h"

namespace dcx {

void ui_draw_inputbox( UI_DIALOG *dlg, UI_GADGET_INPUTBOX * inputbox )
{
#if 0  //ndef OGL
	if ((inputbox->status==1) || (inputbox->position != inputbox->oldposition))
#endif
	{
		gr_set_current_canvas( inputbox->canvas );
		auto &canvas = *grd_curcanv;

		gr_rect(canvas, 0, 0, inputbox->width-1, inputbox->height-1, CBLACK);
		
		int w, h;
		gr_get_string_size(*canvas.cv_font, inputbox->text.get(), &w, &h, nullptr);
		if (dlg->keyboard_focus_gadget == inputbox)
		{
			if (inputbox->first_time)
			{
				gr_set_fontcolor(canvas, CBLACK, -1);
				gr_rect(canvas, 2, 2, 2 + w, 2 + h, CRED);
			}
			else
				gr_set_fontcolor(canvas, CRED, -1);
		}
		else
			gr_set_fontcolor(canvas, CWHITE, -1);

		inputbox->status = 0;

		gr_string(canvas, *canvas.cv_font, 2, 2, inputbox->text.get(), w, h);

		if (dlg->keyboard_focus_gadget == inputbox  && !inputbox->first_time )
		{
			const uint8_t cred = CRED;
			Vline(canvas, 2, inputbox->height - 3, 3 + w, cred);
			Vline(canvas, 2, inputbox->height - 3, 4 + w, cred);
		}
	}
}

std::unique_ptr<UI_GADGET_INPUTBOX> ui_add_gadget_inputbox(UI_DIALOG *const dlg, const short x, const short y, const uint_fast32_t length_of_initial_text, const uint_fast32_t maximum_allowed_text_length, const char *const initial_text)
{
	int h, aw;
	gr_get_string_size(*grd_curcanv->cv_font, nullptr, nullptr, &h, &aw);
	auto inputbox = ui_gadget_add<UI_GADGET_INPUTBOX>(*dlg, x, y, x + aw * maximum_allowed_text_length - 1, y + h - 1 + 4);
	inputbox->text = std::make_unique<char[]>(length_of_initial_text + 1);
	const auto allocated_text = inputbox->text.get();
	allocated_text[length_of_initial_text] = 0;
	memcpy(allocated_text, initial_text, length_of_initial_text);
	inputbox->position = length_of_initial_text;
	inputbox->oldposition = inputbox->position;
	inputbox->width = aw * maximum_allowed_text_length;
	inputbox->height = h+4;
	inputbox->length = length_of_initial_text;
	inputbox->pressed = 0;
	inputbox->first_time = 1;
	return inputbox;
}

window_event_result UI_GADGET_INPUTBOX::event_handler(UI_DIALOG &dlg, const d_event &event)
{
	const auto keypress = (event.type == EVENT_KEY_COMMAND)
		? event_key_get(event)
		: 0u;

	oldposition = position;
	pressed=0;

	window_event_result rval = window_event_result::ignored;
	if (dlg.keyboard_focus_gadget == this)
	{
		switch( keypress )
		{
		case 0:
			break;
		case (KEY_LEFT):
		case (KEY_BACKSP):
			if (position > 0)
				position--;
			text[position] = 0;
			status = 1;
			if (first_time) first_time = 0;
			rval = window_event_result::handled;
			break;
		case (KEY_ENTER):
			pressed=1;
			status = 1;
			if (first_time) first_time = 0;
			rval = window_event_result::handled;
			break;
		default:
			{
			const uint8_t ascii = key_ascii();
			if ((ascii < 255 ) && (position < length-2))
			{
				if (first_time) {
					first_time = 0;
					position = 0;
				}
				text[position++] = ascii;
				text[position] = 0;
				rval = window_event_result::handled;
			}
			status = 1;
			break;
			}
		}
	} else {
		first_time = 1;
	}
	
	if (pressed)
	{
		rval = ui_gadget_send_event(dlg, EVENT_UI_GADGET_PRESSED, *this);
		if (rval == window_event_result::ignored)
			rval = window_event_result::handled;
	}
		
	if (event.type == EVENT_WINDOW_DRAW)
		ui_draw_inputbox(&dlg, this);

	return rval;
}

void ui_inputbox_set_text(UI_GADGET_INPUTBOX *inputbox, const char *text)
{
	auto ltext = std::min(static_cast<std::size_t>(inputbox->length), strlen(text));
	memcpy(inputbox->text.get(), text, ltext);
	inputbox->text[ltext] = 0;
	inputbox->position = ltext;
	inputbox->oldposition = inputbox->position;
	inputbox->status = 1;		// redraw
	inputbox->first_time = 1;	// select all
}

}
