/* $Id: button.c,v 1.3 2004-12-19 15:21:11 btb Exp $ */
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

#ifdef RCS
static char rcsid[] = "$Id: button.c,v 1.3 2004-12-19 15:21:11 btb Exp $";
#endif

#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "mem.h"
#include "fix.h"
#include "types.h"
#include "gr.h"
#include "ui.h"
#include "key.h"
#include "mono.h"

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


void ui_draw_button( UI_GADGET_BUTTON * button )
{
	int color;

	if ((button->status==1) || (button->position != button->oldposition))
	{
		ui_button_any_drawn = 1;
		ui_mouse_hide();
		gr_set_current_canvas( button->canvas );
		color = button->canvas->cv_color;

		if (CurWindow->keyboard_focus_gadget == (UI_GADGET *)button)
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
		ui_mouse_show();
	}
}


UI_GADGET_BUTTON * ui_add_gadget_button( UI_WINDOW * wnd, short x, short y, short w, short h, char * text, int (*function_to_call)(void) )
{
	UI_GADGET_BUTTON * button;

	button = (UI_GADGET_BUTTON *)ui_gadget_add( wnd, 1, x, y, x+w-1, y+h-1 );

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


void ui_button_do( UI_GADGET_BUTTON * button, int keypress )
{
	int result;
	int OnMe, ButtonLastSelected;

	OnMe = ui_mouse_on_gadget( (UI_GADGET *)button );

	button->oldposition = button->position;

	if (selected_gadget != NULL)
	{
		if (selected_gadget->kind==1)
			ButtonLastSelected = 1;
		else
			ButtonLastSelected = 0;
	} else
		ButtonLastSelected = 1;


	if ( B1_PRESSED && OnMe && ButtonLastSelected )
	{

		button->position = 1;
	} else  {
		button->position = 0;
	}

	if (keypress == button->hotkey )
	{
		button->position = 2;
		last_keypress = 0;
	}

	if ((keypress == button->hotkey1) && button->user_function1 )
	{
		result = button->user_function1();
		last_keypress = 0;
	}

	
	//if ((CurWindow->keyboard_focus_gadget==(UI_GADGET *)button) && (keyd_pressed[KEY_SPACEBAR] || keyd_pressed[KEY_ENTER] ) )
	//	button->position = 2;

	if ((CurWindow->keyboard_focus_gadget==(UI_GADGET *)button) && ((keypress==KEY_SPACEBAR) || (keypress==KEY_ENTER)) )
		button->position = 2;

	if (CurWindow->keyboard_focus_gadget==(UI_GADGET *)button)	
		if ((button->oldposition==2) && (keyd_pressed[KEY_SPACEBAR] || keyd_pressed[KEY_ENTER] )  )
			button->position = 2;

	button->pressed = 0;

	if (button->position==0) {
		if ( (button->oldposition==1) && OnMe )
			button->pressed = 1;
		if ( (button->oldposition==2) && (CurWindow->keyboard_focus_gadget==(UI_GADGET *)button) )
			button->pressed = 1;
	}

	ui_draw_button( button );

	if (button->pressed && button->user_function )
	{
		result = button->user_function();
	}
}





