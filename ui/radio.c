/* $Id: radio.c,v 1.6 2005-07-03 13:12:47 chris Exp $ */
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
static char rcsid[] = "$Id: radio.c,v 1.6 2005-07-03 13:12:47 chris Exp $";
#endif

#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "fix.h"
#include "pstypes.h"
#include "gr.h"
#include "ui.h"
#include "key.h"
#include "u_mem.h"

#include "mono.h"

#define Middle(x) ((2*(x)+1)/4)

void ui_draw_radio( UI_GADGET_RADIO * radio )
{

	if ((radio->status==1) || (radio->position != radio->oldposition))
	{
		radio->status = 0;

		ui_mouse_hide();
		gr_set_current_canvas( radio->canvas );

		if (CurWindow->keyboard_focus_gadget == (UI_GADGET *) radio)
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

		ui_mouse_show();
	}
}


UI_GADGET_RADIO * ui_add_gadget_radio( UI_WINDOW * wnd, short x, short y, short w, short h, short group, char * text )
{
	UI_GADGET_RADIO * radio;

	radio = (UI_GADGET_RADIO *)ui_gadget_add( wnd, 4, x, y, x+w-1, y+h-1 );

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


void ui_radio_do( UI_GADGET_RADIO * radio, int keypress )
{
	UI_GADGET * tmp;
	UI_GADGET_RADIO * tmpr;
	int OnMe, ButtonLastSelected;
	keypress  = keypress;

	OnMe = ui_mouse_on_gadget( (UI_GADGET *)radio );

	radio->oldposition = radio->position;

	if (selected_gadget != NULL)
	{
		if (selected_gadget->kind==4)
			ButtonLastSelected = 1;
		else
			ButtonLastSelected = 0;
	} else
		ButtonLastSelected = 1;


	if ( B1_PRESSED && OnMe && ButtonLastSelected )
	{

		radio->position = 1;
	} else  {
		radio->position = 0;
	}


	if ((CurWindow->keyboard_focus_gadget==(UI_GADGET *)radio) && (keyd_pressed[KEY_SPACEBAR] || keyd_pressed[KEY_ENTER] ) )
		radio->position = 2;

	if ((radio->position==0) && (radio->oldposition==1) && OnMe )
		radio->pressed = 1;
	else if ((radio->position==0) && (radio->oldposition==2) && (CurWindow->keyboard_focus_gadget==(UI_GADGET *)radio) )
		radio->pressed = 1;
	else
		radio->pressed = 0;

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
	}

	ui_draw_radio( radio );

}

void ui_radio_set_value(UI_GADGET_RADIO *radio, sbyte value)
{
	UI_GADGET_RADIO *tmp;

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
