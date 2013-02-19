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

#include "fix.h"
#include "pstypes.h"
#include "event.h"
#include "gr.h"
#include "ui.h"
#include "mouse.h"
#include "key.h"

void ui_draw_userbox( UI_DIALOG *dlg, UI_GADGET_USERBOX * userbox )
{
#if 0  //ndef OGL
	if ( userbox->status==1 )
#endif
	{
		userbox->status = 0;

		gr_set_current_canvas( userbox->canvas );

		if (dlg->keyboard_focus_gadget == (UI_GADGET *)userbox)
			gr_setcolor( CRED );
		else
			gr_setcolor( CBRIGHT );

		gr_ubox( -1, -1, userbox->width, userbox->height );
	}
}


UI_GADGET_USERBOX * ui_add_gadget_userbox( UI_DIALOG * dlg, short x, short y, short w, short h )
{
	UI_GADGET_USERBOX * userbox;

	userbox = (UI_GADGET_USERBOX *)ui_gadget_add( dlg, 7, x, y, x+w-1, y+h-1 );

	userbox->width = w;
	userbox->height = h;
	userbox->b1_held_down=0;
	userbox->b1_clicked=0;
	userbox->b1_double_clicked=0;
	userbox->b1_dragging=0;
	userbox->b1_drag_x1=0;
	userbox->b1_drag_y1=0;
	userbox->b1_drag_x2=0;
	userbox->b1_drag_y2=0;
	userbox->b1_done_dragging = 0;
	userbox->keypress = 0;
	userbox->mouse_onme = 0;
	userbox->mouse_x = 0;
	userbox->mouse_y = 0;
	userbox->bitmap = &(userbox->canvas->cv_bitmap);

	return userbox;

}

int ui_userbox_do( UI_DIALOG *dlg, UI_GADGET_USERBOX * userbox, d_event *event )
{
	int OnMe, olddrag;
	int x, y, z;
	int keypress = 0;
	int rval = 0;
	
	if (event->type == EVENT_WINDOW_DRAW)
		ui_draw_userbox( dlg, userbox );
	
	if (event->type == EVENT_KEY_COMMAND)
		keypress = event_key_get(event);
		
	mouse_get_pos(&x, &y, &z);
	OnMe = ui_mouse_on_gadget( (UI_GADGET *)userbox );

	olddrag  = userbox->b1_held_down;

	userbox->mouse_onme = OnMe;
	userbox->mouse_x = x - userbox->x1;
	userbox->mouse_y = y - userbox->y1;

	userbox->b1_dragging = 0;
	userbox->b1_clicked = 0;

	if (OnMe)
	{
		if ( B1_JUST_PRESSED )
		{
			userbox->b1_held_down = 1;
			userbox->b1_drag_x1 = x - userbox->x1;
			userbox->b1_drag_y1 = y - userbox->y1;
			rval = 1;
		}
		else if (B1_JUST_RELEASED)
		{
			if (userbox->b1_held_down)
				userbox->b1_clicked = 1;
			userbox->b1_held_down = 0;
			rval = 1;
		}

		if ( (event->type == EVENT_MOUSE_MOVED) && userbox->b1_held_down )
		{
			userbox->b1_dragging = 1;
			userbox->b1_drag_x2 = x - userbox->x1;
			userbox->b1_drag_y2 = y - userbox->y1;
		}

		if ( B1_DOUBLE_CLICKED )
		{
			userbox->b1_double_clicked = 1;
			rval = 1;
		}
		else
			userbox->b1_double_clicked = 0;

	}

	if (B1_JUST_RELEASED)
		userbox->b1_held_down = 0;

	userbox->b1_done_dragging = 0;

	if (olddrag==1 && userbox->b1_held_down==0 )
	{
		if ((userbox->b1_drag_x1 !=  userbox->b1_drag_x2) || (userbox->b1_drag_y1 !=  userbox->b1_drag_y2) )
			userbox->b1_done_dragging = 1;
	}

	if (dlg->keyboard_focus_gadget==(UI_GADGET *)userbox)
	{
		userbox->keypress = keypress;
		rval = 1;
	}
	
	if (userbox->b1_clicked || userbox->b1_dragging)
	{
		ui_gadget_send_event(dlg, userbox->b1_clicked ? EVENT_UI_GADGET_PRESSED : EVENT_UI_USERBOX_DRAGGED, (UI_GADGET *)userbox);
		rval = 1;
	}

	return rval;
}





