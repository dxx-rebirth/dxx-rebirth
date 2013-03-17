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
 * Radio box gadget stuff.
 *
 */

#include <stdlib.h>
#include <string.h>
#include "fix.h"
#include "pstypes.h"
#include "event.h"
#include "gr.h"
#include "ui.h"
#include "mouse.h"
#include "key.h"
#include "u_mem.h"
#include "strutil.h"

#define Middle(x) ((2*(x)+1)/4)

void ui_draw_radio( UI_DIALOG *dlg, UI_GADGET_RADIO * radio )
{
#if 0  //ndef OGL
	if ((radio->status==1) || (radio->position != radio->oldposition))
#endif
	{
		radio->status = 0;

		gr_set_current_canvas( radio->canvas );

		if (dlg->keyboard_focus_gadget == (UI_GADGET *) radio)
			gr_set_fontcolor(CRED, -1);
		else
			gr_set_fontcolor(CBLACK, -1);

		if (radio->position == 0 )
		{
			ui_draw_box_out( 0, 0, radio->width-1, radio->height-1 );
			if (radio->flag)
				ui_string_centered(Middle(radio->width), Middle(radio->height), "O");
			else
				ui_string_centered(Middle(radio->width), Middle(radio->height), " ");
		} else {
			ui_draw_box_in( 0, 0, radio->width-1, radio->height-1 );
			if (radio->flag)
				ui_string_centered(Middle(radio->width) + 1, Middle(radio->height) + 1, "O");
			else
				ui_string_centered(Middle(radio->width) + 1, Middle(radio->height) + 1, " ");
		}

		gr_ustring( radio->width+4, 2, radio->text );
	}
}


UI_GADGET_RADIO * ui_add_gadget_radio( UI_DIALOG * dlg, short x, short y, short w, short h, short group, char * text )
{
	UI_GADGET_RADIO * radio;

	radio = (UI_GADGET_RADIO *)ui_gadget_add( dlg, 4, x, y, x+w-1, y+h-1 );

	radio->text = d_strdup(text);
	radio->width = w;
	radio->height = h;
	radio->position = 0;
	radio->oldposition = 0;
	radio->pressed = 0;
	radio->flag = 0;
	radio->group = group;

	return radio;

}


int ui_radio_do( UI_DIALOG *dlg, UI_GADGET_RADIO * radio, d_event *event )
{
	UI_GADGET * tmp;
	UI_GADGET_RADIO * tmpr;
	int rval = 0;
	
	radio->oldposition = radio->position;
	radio->pressed = 0;

	if (event->type == EVENT_MOUSE_BUTTON_DOWN || event->type == EVENT_MOUSE_BUTTON_UP)
	{
		int OnMe;
		
		OnMe = ui_mouse_on_gadget( (UI_GADGET *)radio );

		if ( B1_JUST_PRESSED && OnMe)
		{
			radio->position = 1;
			rval = 1;
		} 
		else if (B1_JUST_RELEASED)
		{
			if ((radio->position==1) && OnMe)
				radio->pressed = 1;
			
			radio->position = 0;
		}
	}

	
	if (event->type == EVENT_KEY_COMMAND)
	{
		int key;
		
		key = event_key_get(event);
		
		if ((dlg->keyboard_focus_gadget==(UI_GADGET *)radio) && ((key==KEY_SPACEBAR) || (key==KEY_ENTER)) )
		{
			radio->position = 2;
			rval = 1;
		}
	}
	else if (event->type == EVENT_KEY_RELEASE)
	{
		int key;
		
		key = event_key_get(event);
		
		radio->position = 0;
		
		if ((dlg->keyboard_focus_gadget==(UI_GADGET *)radio) && ((key==KEY_SPACEBAR) || (key==KEY_ENTER)) )
			radio->pressed = 1;
	}
		
	if ((radio->pressed == 1) && (radio->flag==0))
	{
		tmp = (UI_GADGET *)radio->next;

		while (tmp != (UI_GADGET *)radio )
		{
			if (tmp->kind==4)
			{
				tmpr = (UI_GADGET_RADIO *)tmp;
				if ((tmpr->group == radio->group ) && (tmpr->flag) )
				{
					tmpr->flag = 0;
					tmpr->status = 1;
					tmpr->pressed = 0;
				}
			}
			tmp = tmp->next;
		}
		radio->flag = 1;
		ui_gadget_send_event(dlg, EVENT_UI_GADGET_PRESSED, (UI_GADGET *)radio);
		rval = 1;
	}

	if (event->type == EVENT_WINDOW_DRAW)
		ui_draw_radio( dlg, radio );

	return rval;
}

void ui_radio_set_value(UI_GADGET_RADIO *radio, int value)
{
	UI_GADGET_RADIO *tmp;

	value = value != 0;
	if (radio->flag == value)
		return;

	radio->flag = value;
	radio->status = 1;	// redraw

	tmp = (UI_GADGET_RADIO *) radio->next;

	while (tmp != radio)
	{
		if ((tmp->kind == 4) && (tmp->group == radio->group) && tmp->flag)
		{
			tmp->flag = 0;
			tmp->status = 1;
		}
		tmp = (UI_GADGET_RADIO *) tmp->next;
	}
}
