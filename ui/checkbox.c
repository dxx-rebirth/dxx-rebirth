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

void ui_draw_checkbox( UI_DIALOG *dlg, UI_GADGET_CHECKBOX * checkbox )
{
#if 0  //ndef OGL
	if ((checkbox->status==1) || (checkbox->position != checkbox->oldposition))
#endif
	{
		checkbox->status = 0;

		gr_set_current_canvas( checkbox->canvas );

		if (dlg->keyboard_focus_gadget == (UI_GADGET *)checkbox)
			gr_set_fontcolor( CRED, -1 );
		else
			gr_set_fontcolor( CBLACK, -1 );

		if (checkbox->position == 0 )
		{
			ui_draw_box_out( 0, 0, checkbox->width-1, checkbox->height-1 );
			if (checkbox->flag)
				ui_string_centered(  Middle(checkbox->width), Middle(checkbox->height), "X" );
			else
				ui_string_centered(  Middle(checkbox->width), Middle(checkbox->height), " " );
		} else {
			ui_draw_box_in( 0, 0, checkbox->width-1, checkbox->height-1 );
			if (checkbox->flag)
				ui_string_centered(  Middle(checkbox->width)+1, Middle(checkbox->height)+1, "X" );
			else
				ui_string_centered(  Middle(checkbox->width)+1, Middle(checkbox->height)+1, " " );
		}

		gr_ustring( checkbox->width+4, 2, checkbox->text );

	}
}


UI_GADGET_CHECKBOX * ui_add_gadget_checkbox( UI_DIALOG * dlg, short x, short y, short w, short h, short group, char * text )
{
	UI_GADGET_CHECKBOX * checkbox;

	checkbox = (UI_GADGET_CHECKBOX *)ui_gadget_add( dlg, 5, x, y, x+w-1, y+h-1 );

	MALLOC(checkbox->text, char, strlen(text) + 5);
	strcpy(checkbox->text,text);
	checkbox->width = w;
	checkbox->height = h;
	checkbox->position = 0;
	checkbox->oldposition = 0;
	checkbox->pressed = 0;
	checkbox->flag = 0;
	checkbox->group = group;

	return checkbox;

}


int ui_checkbox_do( UI_DIALOG *dlg, UI_GADGET_CHECKBOX * checkbox, d_event *event )
{
	int rval = 0;
	
	checkbox->oldposition = checkbox->position;
	checkbox->pressed = 0;

	if (event->type == EVENT_MOUSE_BUTTON_DOWN || event->type == EVENT_MOUSE_BUTTON_UP)
	{
		int OnMe;
		
		OnMe = ui_mouse_on_gadget( (UI_GADGET *)checkbox );
		
		if (B1_JUST_PRESSED && OnMe)
		{
			checkbox->position = 1;
			rval = 1;
		}
		else if (B1_JUST_RELEASED)
		{
			if ((checkbox->position==1) && OnMe)
				checkbox->pressed = 1;

			checkbox->position = 0;
		}
	}


	if (event->type == EVENT_KEY_COMMAND)
	{
		int key;
		
		key = event_key_get(event);
		
		if ((dlg->keyboard_focus_gadget==(UI_GADGET *)checkbox) && ((key==KEY_SPACEBAR) || (key==KEY_ENTER)) )
		{
			checkbox->position = 2;
			rval = 1;
		}
	}
	else if (event->type == EVENT_KEY_RELEASE)
	{
		int key;
		
		key = event_key_get(event);
		
		checkbox->position = 0;
		
		if ((dlg->keyboard_focus_gadget==(UI_GADGET *)checkbox) && ((key==KEY_SPACEBAR) || (key==KEY_ENTER)) )
			checkbox->pressed = 1;
	}
		
	if (checkbox->pressed == 1)
	{
		checkbox->flag ^= 1;
		ui_gadget_send_event(dlg, EVENT_UI_GADGET_PRESSED, (UI_GADGET *)checkbox);
		rval = 1;
	}

	if (event->type == EVENT_WINDOW_DRAW)
		ui_draw_checkbox( dlg, checkbox );

	return rval;
}

void ui_checkbox_check(UI_GADGET_CHECKBOX * checkbox, int check)
{
	check = check != 0;
	if (checkbox->flag == check)
		return;
	
	checkbox->flag = check;
	checkbox->status = 1;	// redraw
}
