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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/
/*
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/ui/userbox.c,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:39:14 $
 *
 * Routines for user-boxes.
 *
 * $Log: userbox.c,v $
 * Revision 1.1.1.1  2006/03/17 19:39:14  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 22:14:44  donut
 * Import of d1x 1.37 source.
 *
 * Revision 1.4  1993/12/07  12:30:05  john
 * new version.
 * 
 * Revision 1.3  1993/10/26  13:46:13  john
 * *** empty log message ***
 * 
 * Revision 1.2  1993/10/05  17:32:01  john
 * *** empty log message ***
 * 
 * Revision 1.1  1993/09/20  10:35:53  john
 * Initial revision
 * 
 *
 */

#ifdef RCS
static char rcsid[] = "$Id: userbox.c,v 1.1.1.1 2006/03/17 19:39:14 zicodxx Exp $";
#endif

#include <stdlib.h>
#include <string.h>

#include "fix.h"
#include "pstypes.h"
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

		gr_rect( -1, -1, userbox->width, userbox->height );

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





