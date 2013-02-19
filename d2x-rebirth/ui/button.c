/*
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

#include "u_mem.h"
#include "fix.h"
#include "pstypes.h"
#include "event.h"
#include "gr.h"
#include "ui.h"
#include "mouse.h"
#include "key.h"

#define Middle(x) ((2*(x)+1)/4)

#define BUTTON_EXTRA_WIDTH  15
#define BUTTON_EXTRA_HEIGHT 2

int ui_button_any_drawn = 0;

void ui_get_button_size( char * text, int * width, int * height )
{
	int avg;

	gr_get_string_size(text, width, height, &avg  );

	*width += BUTTON_EXTRA_WIDTH*2;
	*width += 6;

	*height += BUTTON_EXTRA_HEIGHT*2;
	*height += 6;

}


void ui_draw_button(UI_DIALOG *dlg, UI_GADGET_BUTTON * button)
{
	int color;

#if 0  //ndef OGL
	if ((button->status==1) || (button->position != button->oldposition))
#endif
	{
		ui_button_any_drawn = 1;
		gr_set_current_canvas( button->canvas );
		color = button->canvas->cv_color;

		if (dlg->keyboard_focus_gadget == (UI_GADGET *)button)
			gr_set_fontcolor( CRED, -1 );
		else
		{
			if ((button->user_function==NULL) && button->dim_if_no_function )
				gr_set_fontcolor( CGREY, -1 );
			else 
				gr_set_fontcolor( CBLACK, -1 );
		}

		button->status = 0;
		if (button->position == 0 )
		{
			if (button->text )	{
				ui_draw_box_out( 0, 0, button->width-1, button->height-1 );
				ui_string_centered(  Middle(button->width), Middle(button->height), button->text );
			} else	{
				gr_setcolor( CBLACK );
				gr_rect( 0, 0, button->width, button->height );
				gr_setcolor( color );
				gr_rect( 1, 1, button->width-1, button->height-1 );
			}				
		} else {
			if (button->text )	{
				ui_draw_box_in( 0, 0, button->width-1, button->height-1 );
				ui_string_centered(  Middle(button->width)+1, Middle(button->height)+1, button->text );
			} else	{
				gr_setcolor( CBLACK );
				gr_rect( 0, 0, button->width, button->height );
				gr_setcolor( color );
				gr_rect( 2, 2, button->width, button->height );
			}			
		}
		button->canvas->cv_color = color;
	}
}


UI_GADGET_BUTTON * ui_add_gadget_button( UI_DIALOG * dlg, short x, short y, short w, short h, char * text, int (*function_to_call)(void) )
{
	UI_GADGET_BUTTON * button;

	button = (UI_GADGET_BUTTON *)ui_gadget_add( dlg, 1, x, y, x+w-1, y+h-1 );

	if ( text )
	{
		MALLOC( button->text, char, strlen(text)+1 );
		strcpy( button->text, text );
	} else {
		button->text = NULL;
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


int ui_button_do(UI_DIALOG *dlg, UI_GADGET_BUTTON * button, d_event *event)
{
	int rval = 0;
	
	button->oldposition = button->position;
	button->pressed = 0;

	if (event->type == EVENT_MOUSE_BUTTON_DOWN || event->type == EVENT_MOUSE_BUTTON_UP)
	{
		int OnMe;

		OnMe = ui_mouse_on_gadget( (UI_GADGET *)button );

		if (B1_JUST_PRESSED && OnMe)
		{
			button->position = 1;
			rval = 1;
		}
		else if (B1_JUST_RELEASED)
		{
			if ((button->position == 1) && OnMe)
				button->pressed = 1;

			button->position = 0;
		}
	}

	
	if (event->type == EVENT_KEY_COMMAND)
	{
		int keypress;
		
		keypress = event_key_get(event);

		if	((keypress == button->hotkey) ||
			((keypress == button->hotkey1) && button->user_function1) || 
			((dlg->keyboard_focus_gadget==(UI_GADGET *)button) && ((keypress==KEY_SPACEBAR) || (keypress==KEY_ENTER)) ))
		{
			button->position = 2;
			rval = 1;
		}
	}
	else if (event->type == EVENT_KEY_RELEASE)
	{
		int keypress;
		
		keypress = event_key_get(event);
		
		button->position = 0;

		if	((keypress == button->hotkey) ||
			((dlg->keyboard_focus_gadget==(UI_GADGET *)button) && ((keypress==KEY_SPACEBAR) || (keypress==KEY_ENTER)) ))
			button->pressed = 1;

		if ((keypress == button->hotkey1) && button->user_function1)
		{
			button->user_function1();
			rval = 1;
		}
	}

	if (event->type == EVENT_WINDOW_DRAW)
		ui_draw_button( dlg, button );

	if (button->pressed && button->user_function )
	{
		button->user_function();
		rval = 1;
	}
	else if (button->pressed)
	{
		ui_gadget_send_event(dlg, EVENT_UI_GADGET_PRESSED, (UI_GADGET *)button);
		rval = 1;
	}
	
	return rval;
}
