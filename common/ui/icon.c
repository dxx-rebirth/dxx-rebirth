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

void ui_draw_box_in1( short x1, short y1, short x2, short y2 )
{

	gr_setcolor( CWHITE );
	gr_urect( x1+1, y1+1, x2-1, y2-1 );

	ui_draw_shad( x1+0, y1+0, x2-0, y2-0, CGREY, CBRIGHT );
}


void ui_draw_icon( UI_GADGET_ICON * icon )
{
	int height, width, avg;
	int x, y;
	
#if 0  //ndef OGL
	if ((icon->status==1) || (icon->position != icon->oldposition))
#endif
	{
		icon->status = 0;

		gr_set_current_canvas( icon->canvas );
		gr_get_string_size(icon->text, &width, &height, &avg );
	
		x = ((icon->width-1)/2)-((width-1)/2);
		y = ((icon->height-1)/2)-((height-1)/2);

		if (icon->position==1 )
		{
			// Draw pressed
			ui_draw_box_in( 0, 0, icon->width, icon->height );
			x += 2; y += 2;
		}
		else if (icon->flag)
		{
			// Draw part out
			ui_draw_box_in1( 0, 0, icon->width, icon->height );
			x += 1; y += 1;	
		}
		else
		{
			// Draw released!
			ui_draw_box_out( 0, 0, icon->width, icon->height );
		}
	
		gr_set_fontcolor( CBLACK, -1 );		
		gr_ustring( x, y, icon->text );
	}
}


UI_GADGET_ICON * ui_add_gadget_icon( UI_DIALOG * dlg, char * text, short x, short y, short w, short h, int k,int (*f)(void) )
{
	UI_GADGET_ICON * icon;

	icon = (UI_GADGET_ICON *)ui_gadget_add( dlg, 9, x, y, x+w-1, y+h-1 );

	icon->width = w;
	icon->height = h;
	MALLOC( icon->text, char, strlen( text )+2);//Hack by KRB
	strcpy( icon->text, text );
	icon->trap_key = k;
	icon->user_function = f;
	icon->oldposition = 0;
	icon->position = 0;
	icon->pressed = 0;
	icon->canvas->cv_font = ui_small_font;

	// Call twice to get original;
	if (f)
	{
		icon->flag = (sbyte)f();
		icon->flag = (sbyte)f();
	} else {
		icon->flag = 0;
	}


	return icon;

}

int ui_icon_do( UI_DIALOG *dlg, UI_GADGET_ICON * icon, d_event *event )
{
	int rval = 0;
	
	icon->oldposition = icon->position;
	icon->pressed = 0;

	if (event->type == EVENT_MOUSE_BUTTON_DOWN || event->type == EVENT_MOUSE_BUTTON_UP)
	{
		int OnMe;
		
		OnMe = ui_mouse_on_gadget( (UI_GADGET *)icon );

		if (B1_JUST_PRESSED && OnMe)
		{
			icon->position = 1;
			rval = 1;
		}
		else if (B1_JUST_RELEASED)
		{
			if ((icon->position == 1) && OnMe)
				icon->pressed = 1;
				
			icon->position = 0;
		}
	}


	if (event->type == EVENT_KEY_COMMAND)
	{
		int key;
		
		key = event_key_get(event);
		
		if (key == icon->trap_key)
		{
			icon->position = 1;
			rval = 1;
		}
	}
	else if (event->type == EVENT_KEY_RELEASE)
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
		icon->flag = (sbyte)icon->user_function();
		ui_gadget_send_event(dlg, EVENT_UI_GADGET_PRESSED, (UI_GADGET *)icon);
		rval = 1;
	}

	if (event->type == EVENT_WINDOW_DRAW)
		ui_draw_icon( icon );

	return rval;
}
