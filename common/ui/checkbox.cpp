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
#include "mouse.h"
#include "key.h"

namespace dcx {

#define Middle(x) ((2*(x)+1)/4)

void ui_draw_checkbox( UI_DIALOG *dlg, UI_GADGET_CHECKBOX * checkbox )
{
#if 0  //ndef OGL
	if ((checkbox->status==1) || (checkbox->position != checkbox->oldposition))
#endif
	{
		checkbox->status = 0;

		gr_set_current_canvas( checkbox->canvas );
		auto &canvas = *grd_curcanv;
		gr_set_fontcolor(canvas, dlg->keyboard_focus_gadget == checkbox
			? CRED
			: CBLACK, -1);

		unsigned offset;
		if (checkbox->position == 0 )
		{
			ui_draw_box_out(canvas, 0, 0, checkbox->width-1, checkbox->height-1);
			offset = 0;
		} else {
			ui_draw_box_in(canvas, 0, 0, checkbox->width-1, checkbox->height-1);
			offset = 1;
		}
		ui_string_centered(canvas, Middle(checkbox->width) + offset, Middle(checkbox->height) + offset, checkbox->flag ? "X" : " ");
		gr_ustring(canvas, *canvas.cv_font, checkbox->width + 4, 2, checkbox->text.get());
	}
}


std::unique_ptr<UI_GADGET_CHECKBOX> ui_add_gadget_checkbox(UI_DIALOG * dlg, short x, short y, short w, short h, short group, const char * text)
{
	std::unique_ptr<UI_GADGET_CHECKBOX> checkbox{ui_gadget_add<UI_GADGET_CHECKBOX>(dlg, x, y, x+w-1, y+h-1)};
	auto ltext = strlen(text) + 1;
	MALLOC(checkbox->text, char[], ltext + 4);
	memcpy(checkbox->text.get(), text, ltext);
	checkbox->width = w;
	checkbox->height = h;
	checkbox->position = 0;
	checkbox->oldposition = 0;
	checkbox->pressed = 0;
	checkbox->flag = 0;
	checkbox->group = group;
	return checkbox;
}

window_event_result ui_checkbox_do( UI_DIALOG *dlg, UI_GADGET_CHECKBOX * checkbox,const d_event &event )
{
	checkbox->oldposition = checkbox->position;
	checkbox->pressed = 0;

	if (event.type == EVENT_MOUSE_BUTTON_DOWN || event.type == EVENT_MOUSE_BUTTON_UP)
	{
		int OnMe;
		
		OnMe = ui_mouse_on_gadget( checkbox );
		
		if (B1_JUST_PRESSED && OnMe)
		{
			checkbox->position = 1;
			return window_event_result::handled;
		}
		else if (B1_JUST_RELEASED)
		{
			if ((checkbox->position==1) && OnMe)
				checkbox->pressed = 1;

			checkbox->position = 0;
		}
	}


	if (event.type == EVENT_KEY_COMMAND)
	{
		int key;
		
		key = event_key_get(event);
		
		if ((dlg->keyboard_focus_gadget==checkbox) && ((key==KEY_SPACEBAR) || (key==KEY_ENTER)) )
		{
			checkbox->position = 2;
			return window_event_result::handled;
		}
	}
	else if (event.type == EVENT_KEY_RELEASE)
	{
		int key;
		
		key = event_key_get(event);
		
		checkbox->position = 0;
		
		if ((dlg->keyboard_focus_gadget==checkbox) && ((key==KEY_SPACEBAR) || (key==KEY_ENTER)) )
			checkbox->pressed = 1;
	}
		
	if (checkbox->pressed == 1)
	{
		checkbox->flag ^= 1;
		auto rval = ui_gadget_send_event(dlg, EVENT_UI_GADGET_PRESSED, checkbox);
		if (rval == window_event_result::ignored)
			rval = window_event_result::handled;
		return rval;
	}

	if (event.type == EVENT_WINDOW_DRAW)
		ui_draw_checkbox( dlg, checkbox );

	return window_event_result::ignored;
}

void ui_checkbox_check(UI_GADGET_CHECKBOX * checkbox, int check)
{
	check = check != 0;
	if (checkbox->flag == check)
		return;
	
	checkbox->flag = check;
	checkbox->status = 1;	// redraw
}

}
