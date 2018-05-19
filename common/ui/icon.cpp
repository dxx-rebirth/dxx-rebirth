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

static void ui_draw_box_in1(grs_canvas &canvas, const unsigned x1, const unsigned y1, const unsigned x2, const unsigned y2)
{
	const uint8_t color = CWHITE;
	gr_urect(canvas, x1 + 1, y1 + 1, x2 - 1, y2 - 1, color);
	ui_draw_shad(canvas, x1, y1, x2, y2, CGREY, CBRIGHT);
}


void ui_draw_icon( UI_GADGET_ICON * icon )
{
	int height, width;
	int x, y;
	
#if 0  //ndef OGL
	if ((icon->status==1) || (icon->position != icon->oldposition))
#endif
	{
		icon->status = 0;

		gr_set_current_canvas( icon->canvas );
		auto &canvas = *grd_curcanv;
		gr_get_string_size(*canvas.cv_font, icon->text.get(), &width, &height, nullptr);
	
		x = ((icon->width-1)/2)-((width-1)/2);
		y = ((icon->height-1)/2)-((height-1)/2);

		if (icon->position==1 )
		{
			// Draw pressed
			ui_draw_box_in(canvas, 0, 0, icon->width, icon->height);
			x += 2; y += 2;
		}
		else if (icon->flag)
		{
			// Draw part out
			ui_draw_box_in1(canvas, 0, 0, icon->width, icon->height);
			x += 1; y += 1;	
		}
		else
		{
			// Draw released!
			ui_draw_box_out(canvas, 0, 0, icon->width, icon->height);
		}
	
		gr_set_fontcolor(canvas, CBLACK, -1);
		gr_ustring(canvas, *canvas.cv_font, x, y, icon->text.get());
	}
}


std::unique_ptr<UI_GADGET_ICON> ui_add_gadget_icon(UI_DIALOG * dlg, const char * text, short x, short y, short w, short h, int k,int (*f)())
{
	std::unique_ptr<UI_GADGET_ICON> icon{ui_gadget_add<UI_GADGET_ICON>(dlg, x, y, x+w-1, y+h-1)};

	icon->width = w;
	icon->height = h;
	auto ltext = strlen(text) + 1;
	MALLOC( icon->text, char[], ltext + 1);//Hack by KRB
	memcpy(icon->text.get(), text, ltext);
	icon->trap_key = k;
	icon->user_function = f;
	icon->oldposition = 0;
	icon->position = 0;
	icon->pressed = 0;
	icon->canvas->cv_font = ui_small_font.get();
	// Call twice to get original;
	if (f)
	{
		icon->flag = static_cast<int8_t>(f());
		icon->flag = static_cast<int8_t>(f());
	} else {
		icon->flag = 0;
	}
	return icon;
}

window_event_result ui_icon_do( UI_DIALOG *dlg, UI_GADGET_ICON * icon,const d_event &event )
{
	icon->oldposition = icon->position;
	icon->pressed = 0;

	window_event_result rval = window_event_result::ignored;
	if (event.type == EVENT_MOUSE_BUTTON_DOWN || event.type == EVENT_MOUSE_BUTTON_UP)
	{
		int OnMe;
		
		OnMe = ui_mouse_on_gadget( icon );

		if (B1_JUST_PRESSED && OnMe)
		{
			icon->position = 1;
			rval = window_event_result::handled;
		}
		else if (B1_JUST_RELEASED)
		{
			if ((icon->position == 1) && OnMe)
				icon->pressed = 1;
				
			icon->position = 0;
		}
	}


	if (event.type == EVENT_KEY_COMMAND)
	{
		int key;
		
		key = event_key_get(event);
		
		if (key == icon->trap_key)
		{
			icon->position = 1;
			rval = window_event_result::handled;
		}
	}
	else if (event.type == EVENT_KEY_RELEASE)
	{
		int key;
		
		key = event_key_get(event);
		
		icon->position = 0;
		
		if (key == icon->trap_key)
			icon->pressed = 1;
	}
		
	if (icon->pressed == 1)
	{
		icon->status = 1;
		icon->flag = static_cast<int8_t>(icon->user_function());
		rval = ui_gadget_send_event(dlg, EVENT_UI_GADGET_PRESSED, icon);
		if (rval == window_event_result::ignored)
			rval = window_event_result::handled;
	}

	if (event.type == EVENT_WINDOW_DRAW)
		ui_draw_icon( icon );

	return rval;
}

}
