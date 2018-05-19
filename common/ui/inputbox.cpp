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

std::unique_ptr<UI_GADGET_INPUTBOX> ui_add_gadget_inputbox(UI_DIALOG * dlg, short x, short y, short length, short slength, const char * text)
{
	int h, aw;
	gr_get_string_size(*grd_curcanv->cv_font, nullptr, nullptr, &h, &aw);
	std::unique_ptr<UI_GADGET_INPUTBOX> inputbox{ui_gadget_add<UI_GADGET_INPUTBOX>(dlg, x, y, x+aw*slength-1, y+h-1+4)};
	MALLOC(inputbox->text, char[], length + 1);
	auto ltext = strlen(text) + 1;
	memcpy(inputbox->text.get(), text, ltext);
	inputbox->position = ltext - 1;
	inputbox->oldposition = inputbox->position;
	inputbox->width = aw*slength;
	inputbox->height = h+4;
	inputbox->length = length;
	inputbox->slength = slength;
	inputbox->pressed = 0;
	inputbox->first_time = 1;
	return inputbox;
}

window_event_result ui_inputbox_do( UI_DIALOG *dlg, UI_GADGET_INPUTBOX * inputbox,const d_event &event )
{
	unsigned char ascii;
	int keypress = 0;
	
	if (event.type == EVENT_KEY_COMMAND)
		keypress = event_key_get(event);

	inputbox->oldposition = inputbox->position;
	inputbox->pressed=0;

	window_event_result rval = window_event_result::ignored;
	if (dlg->keyboard_focus_gadget==inputbox)
	{
		switch( keypress )
		{
		case 0:
			break;
		case (KEY_LEFT):
		case (KEY_BACKSP):
			if (inputbox->position > 0)
				inputbox->position--;
			inputbox->text[inputbox->position] = 0;
			inputbox->status = 1;
			if (inputbox->first_time) inputbox->first_time = 0;
			rval = window_event_result::handled;
			break;
		case (KEY_ENTER):
			inputbox->pressed=1;
			inputbox->status = 1;
			if (inputbox->first_time) inputbox->first_time = 0;
			rval = window_event_result::handled;
			break;
		default:
			ascii = key_ascii();
			if ((ascii < 255 ) && (inputbox->position < inputbox->length-2))
			{
				if (inputbox->first_time) {
					inputbox->first_time = 0;
					inputbox->position = 0;
				}
				inputbox->text[inputbox->position++] = ascii;
				inputbox->text[inputbox->position] = 0;
				rval = window_event_result::handled;
			}
			inputbox->status = 1;
			break;
		}
	} else {
		inputbox->first_time = 1;
	}
	
	if (inputbox->pressed)
	{
		rval = ui_gadget_send_event(dlg, EVENT_UI_GADGET_PRESSED, inputbox);
		if (rval == window_event_result::ignored)
			rval = window_event_result::handled;
	}
		
	if (event.type == EVENT_WINDOW_DRAW)
		ui_draw_inputbox( dlg, inputbox );

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
