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
 * Routines for manipulating the button gadgets.
 *
 */

#include <stdlib.h>
#include <string.h>

#include "strutil.h"
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

#define BUTTON_EXTRA_WIDTH  15
#define BUTTON_EXTRA_HEIGHT 2

int ui_button_any_drawn = 0;

void ui_get_button_size(const grs_font &cv_font, const char *text, int &width, int &height)
{
	gr_get_string_size(cv_font, text, &width, &height, nullptr);
	width += BUTTON_EXTRA_WIDTH * 2 + 6;
	height += BUTTON_EXTRA_HEIGHT * 2 + 6;
}


void ui_draw_button(UI_DIALOG *dlg, UI_GADGET_BUTTON * button)
{
#if 0  //ndef OGL
	if ((button->status==1) || (button->position != button->oldposition))
#endif
	{
		ui_button_any_drawn = 1;
		gr_set_current_canvas( button->canvas );
		color_t color = 0;

		gr_set_fontcolor(*grd_curcanv, dlg->keyboard_focus_gadget == button
			? CRED
			: (!button->user_function && button->dim_if_no_function
				? CGREY
				: CBLACK
			), -1);

		button->status = 0;
		if (button->position == 0 )
		{
			if (!button->text.empty())
			{
				ui_draw_box_out( 0, 0, button->width-1, button->height-1 );
				ui_string_centered(Middle(button->width), Middle(button->height), button->text.c_str());
			} else	{
				gr_rect(*grd_curcanv, 0, 0, button->width, button->height, CBLACK);
				gr_rect(*grd_curcanv, 1, 1, button->width-1, button->height-1, color);
			}				
		} else {
			if (!button->text.empty())	{
				ui_draw_box_in( 0, 0, button->width-1, button->height-1 );
				ui_string_centered(Middle(button->width)+1, Middle(button->height)+1, button->text.c_str());
			} else	{
				gr_rect(*grd_curcanv, 0, 0, button->width, button->height, CBLACK);
				gr_rect(*grd_curcanv, 2, 2, button->width, button->height, color);
			}			
		}
	}
}

std::unique_ptr<UI_GADGET_BUTTON> ui_add_gadget_button(UI_DIALOG * dlg, short x, short y, short w, short h, const char * text, int (*function_to_call)())
{
	std::unique_ptr<UI_GADGET_BUTTON> button{ui_gadget_add<UI_GADGET_BUTTON>( dlg, x, y, x+w-1, y+h-1)};

	if ( text )
	{
		button->text = text;
	} else {
		button->text.clear();
	}
	button->width = w;
	button->height = h;
	button->position = 0;
	button->oldposition = 0;
	button->pressed = 0;
	button->user_function = function_to_call;
	button->user_function1 = NULL;
	button->hotkey1= -1;
	button->dim_if_no_function = 0;
	return button;
}


window_event_result ui_button_do(UI_DIALOG *dlg, UI_GADGET_BUTTON * button,const d_event &event)
{
	window_event_result rval = window_event_result::ignored;
	
	button->oldposition = button->position;
	button->pressed = 0;

	if (event.type == EVENT_MOUSE_BUTTON_DOWN || event.type == EVENT_MOUSE_BUTTON_UP)
	{
		int OnMe;

		OnMe = ui_mouse_on_gadget( button );

		if (B1_JUST_PRESSED && OnMe)
		{
			button->position = 1;
			rval = window_event_result::handled;
		}
		else if (B1_JUST_RELEASED)
		{
			if ((button->position == 1) && OnMe)
				button->pressed = 1;

			button->position = 0;
		}
	}

	
	if (event.type == EVENT_KEY_COMMAND)
	{
		int keypress;
		
		keypress = event_key_get(event);

		if	((keypress == button->hotkey) ||
			((keypress == button->hotkey1) && button->user_function1) || 
			((dlg->keyboard_focus_gadget==button) && ((keypress==KEY_SPACEBAR) || (keypress==KEY_ENTER)) ))
		{
			button->position = 2;
			rval = window_event_result::handled;
		}
	}
	else if (event.type == EVENT_KEY_RELEASE)
	{
		int keypress;
		
		keypress = event_key_get(event);
		
		button->position = 0;

		if	((keypress == button->hotkey) ||
			((dlg->keyboard_focus_gadget==button) && ((keypress==KEY_SPACEBAR) || (keypress==KEY_ENTER)) ))
			button->pressed = 1;

		if ((keypress == button->hotkey1) && button->user_function1)
		{
			button->user_function1();
			rval = window_event_result::handled;
		}
	}

	if (event.type == EVENT_WINDOW_DRAW)
		ui_draw_button( dlg, button );

	if (button->pressed && button->user_function )
	{
		button->user_function();
		return window_event_result::handled;
	}
	else if (button->pressed)
	{
		rval = ui_gadget_send_event(dlg, EVENT_UI_GADGET_PRESSED, button);
		if (rval == window_event_result::ignored)
			rval = window_event_result::handled;
	}
	
	return rval;
}

}
