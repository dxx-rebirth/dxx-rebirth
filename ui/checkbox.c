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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/ui/checkbox.c,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:39:13 $
 *
 * Routines for doing checkbox gadgets.
 *
 * $Log: checkbox.c,v $
 * Revision 1.1.1.1  2006/03/17 19:39:13  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 22:14:20  donut
 * Import of d1x 1.37 source.
 *
 * Revision 1.4  1993/12/07  12:30:47  john
 * new version.
 * 
 * Revision 1.3  1993/10/26  13:46:20  john
 * *** empty log message ***
 * 
 * Revision 1.2  1993/10/05  17:30:42  john
 * *** empty log message ***
 * 
 * Revision 1.1  1993/09/20  10:35:07  john
 * Initial revision
 * 
 *
 */

#ifdef RCS
static char rcsid[] = "$Id: checkbox.c,v 1.1.1.1 2006/03/17 19:39:13 zicodxx Exp $";
#endif


#include <stdlib.h>
#include <string.h>

#include "u_mem.h"
#include "fix.h"
#include "pstypes.h"
#include "gr.h"
#include "ui.h"
#include "key.h"

#define Middle(x) ((2*(x)+1)/4)

void ui_draw_checkbox( UI_GADGET_CHECKBOX * checkbox )
{

	if ((checkbox->status==1) || (checkbox->position != checkbox->oldposition))
	{
		checkbox->status = 0;

		ui_mouse_hide();
		gr_set_current_canvas( checkbox->canvas );

		if (CurWindow->keyboard_focus_gadget == (UI_GADGET *)checkbox)
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

		ui_mouse_show();
	}
}


UI_GADGET_CHECKBOX * ui_add_gadget_checkbox( UI_WINDOW * wnd, short x, short y, short w, short h, short group, char * text )
{
	UI_GADGET_CHECKBOX * checkbox;

	checkbox = (UI_GADGET_CHECKBOX *)ui_gadget_add( wnd, 5, x, y, x+w-1, y+h-1 );

	checkbox->text = d_malloc(strlen(text)+5);
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


void ui_checkbox_do( UI_GADGET_CHECKBOX * checkbox, int keypress )
{
	int OnMe, ButtonLastSelected;

	keypress = keypress;

	OnMe = ui_mouse_on_gadget( (UI_GADGET *)checkbox );

	checkbox->oldposition = checkbox->position;

	if ((selected_gadget != NULL) && (selected_gadget->kind !=5))
		ButtonLastSelected = 0;
	else
		ButtonLastSelected = 1;

	if ( B1_PRESSED && OnMe && ButtonLastSelected )
	{
		checkbox->position = 1;
	} else  {
		checkbox->position = 0;
	}


	if ((CurWindow->keyboard_focus_gadget==(UI_GADGET *)checkbox) && (keyd_pressed[KEY_SPACEBAR] || keyd_pressed[KEY_ENTER] ) )
		checkbox->position = 2;

	if ((checkbox->position==0) && (checkbox->oldposition==1) && OnMe )
		checkbox->pressed = 1;
	else if ((checkbox->position==0) && (checkbox->oldposition==2) && (CurWindow->keyboard_focus_gadget==(UI_GADGET *)checkbox) )
		checkbox->pressed = 1;
	else
		checkbox->pressed = 0;

	if (checkbox->pressed == 1)
		checkbox->flag ^= 1;

	ui_draw_checkbox( checkbox );

}
