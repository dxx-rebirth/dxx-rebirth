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
static char rcsid[] = "$Id: userbox.c,v 1.2 2004-12-19 14:10:33 btb Exp $";
#endif
#include <stdlib.h>
#include <string.h>

#include "fix.h"
#include "types.h"
#include "gr.h"
#include "ui.h"
#include "key.h"

void ui_draw_userbox( UI_GADGET_USERBOX * userbox )
{

	if ( userbox->status==1 )
	{
		userbox->status = 0;

		ui_mouse_hide();
		gr_set_current_canvas( userbox->canvas );

		if (CurWindow->keyboard_focus_gadget == (UI_GADGET *)userbox)
			gr_setcolor( CRED );
		else
			gr_setcolor( CBRIGHT );

		gr_box( -1, -1, userbox->width, userbox->height );

		ui_mouse_show();
	}
}


UI_GADGET_USERBOX * ui_add_gadget_userbox( UI_WINDOW * wnd, short x, short y, short w, short h )
{
	UI_GADGET_USERBOX * userbox;

	userbox = (UI_GADGET_USERBOX *)ui_gadget_add( wnd, 7, x, y, x+w-1, y+h-1 );

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

void ui_userbox_do( UI_GADGET_USERBOX * userbox, int keypress )
{
	int OnMe, olddrag;

	OnMe = ui_mouse_on_gadget( (UI_GADGET *)userbox );

	olddrag  = userbox->b1_dragging;

	userbox->mouse_onme = OnMe;
	userbox->mouse_x = Mouse.x - userbox->x1;
	userbox->mouse_y = Mouse.y - userbox->y1;

	userbox->b1_clicked = 0;

	if (OnMe)
	{
		if ( B1_JUST_PRESSED )
		{
			userbox->b1_dragging = 1;
			userbox->b1_drag_x1 = Mouse.x - userbox->x1;
			userbox->b1_drag_y1 = Mouse.y - userbox->y1;
			userbox->b1_clicked = 1;
		}

		if ( B1_PRESSED )
		{
			userbox->b1_held_down = 1;
			userbox->b1_drag_x2 = Mouse.x - userbox->x1;
			userbox->b1_drag_y2 = Mouse.y - userbox->y1;
		}
		else    {
			userbox->b1_held_down = 0;
			userbox->b1_dragging = 0;
		}

		if ( B1_DOUBLE_CLICKED )
			userbox->b1_double_clicked = 1;
		else
			userbox->b1_double_clicked = 0;

	}

	if (!B1_PRESSED)
		userbox->b1_dragging = 0;

	userbox->b1_done_dragging = 0;

	if (olddrag==1 && userbox->b1_dragging==0 )
	{
		if ((userbox->b1_drag_x1 !=  userbox->b1_drag_x2) || (userbox->b1_drag_y1 !=  userbox->b1_drag_y2) )
			userbox->b1_done_dragging = 1;
	}

	if (CurWindow->keyboard_focus_gadget==(UI_GADGET *)userbox)
		userbox->keypress = keypress;

	ui_draw_userbox( userbox );

}





