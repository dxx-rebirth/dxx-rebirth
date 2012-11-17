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


#include <stdio.h>
#include <stdlib.h>

#include "u_mem.h"
#include "fix.h"
#include "pstypes.h"
#include "gr.h"
#include "ui.h"
#include "event.h"
#include "mouse.h"
#include "window.h"
#include "dxxerror.h"

#include "key.h"

UI_GADGET * selected_gadget;

typedef struct event_gadget
{
	event_type type;
	UI_GADGET *gadget;
} event_gadget;

UI_GADGET * ui_gadget_add( UI_DIALOG * dlg, short kind, short x1, short y1, short x2, short y2 )
{
	UI_GADGET * gadget;

	gadget = (UI_GADGET *) d_malloc(sizeof(UI_GADGET));
	if (gadget==NULL) Error("Could not create gadget: Out of memory");

	if (dlg->gadget == NULL )
	{
		dlg->gadget = gadget;
		gadget->prev = gadget;
		gadget->next = gadget;
	} else {
		dlg->gadget->prev->next = gadget;
		gadget->next = dlg->gadget;
		gadget->prev = dlg->gadget->prev;
		dlg->gadget->prev = gadget;
	}

	gadget->when_tab = NULL;
	gadget->when_btab = NULL;
	gadget->when_up = NULL;
	gadget->when_down = NULL;
	gadget->when_left = NULL;
	gadget->when_right = NULL;
	gadget->kind = kind;
	gadget->status = 1;
	gadget->oldstatus = 0;
	if ( x1==0 && x2==0 && y1==0 && y2== 0 )
		gadget->canvas = NULL;
	else
		gadget->canvas = gr_create_sub_canvas( window_get_canvas(ui_dialog_get_window( dlg )), x1, y1, x2-x1+1, y2-y1+1 );
	gadget->x1 = gadget->canvas->cv_bitmap.bm_x;
	gadget->y1 = gadget->canvas->cv_bitmap.bm_y;
	gadget->x2 = gadget->canvas->cv_bitmap.bm_x+x2-x1+1;
	gadget->y2 = gadget->canvas->cv_bitmap.bm_y+y2-y1+1;
	gadget->parent = NULL;
	gadget->hotkey = -1;
	return gadget;

}

void ui_gadget_delete_all( UI_DIALOG * dlg )
{
	UI_GADGET * tmp;

	while( dlg->gadget != NULL )
	{
		tmp = dlg->gadget;
		if (tmp->next == tmp )
		{
			dlg->gadget = NULL;
		} else {
			tmp->next->prev = tmp->prev;
			tmp->prev->next = tmp->next;
			dlg->gadget = tmp->next;
		}
		if (tmp->canvas)
			gr_free_sub_canvas( tmp->canvas );


		if (tmp->kind == 1 )    // Button
		{
			UI_GADGET_BUTTON * but1 = (UI_GADGET_BUTTON *)tmp;
			if (but1->text)
				d_free( but1->text );
		}

		if (tmp->kind == 4 )    // Radio
		{
			UI_GADGET_RADIO * but1 = (UI_GADGET_RADIO *)tmp;
			if (but1->text)
				d_free( but1->text );
		}
		
		if (tmp->kind == 6 )    // Inputbox
		{
			UI_GADGET_INPUTBOX * but1 = (UI_GADGET_INPUTBOX *)tmp;
			d_free( but1->text );
		}

		if (tmp->kind == 5 )    // Checkbox
		{
			UI_GADGET_CHECKBOX * but1 = (UI_GADGET_CHECKBOX *)tmp;
			d_free( but1->text );
		}
		
		if (tmp->kind == 9 )    // Icon
		{
			UI_GADGET_ICON * but1 = (UI_GADGET_ICON *)tmp;
			d_free( but1->text );
		}


		d_free( tmp );
	}
}


#if 0
int is_under_another_window( UI_DIALOG * dlg, UI_GADGET * gadget )
{
	UI_DIALOG * temp;

	temp = dlg->next;

	while( temp != NULL )	{
		if (	( gadget->x1 > temp->x)						&&
				( gadget->x1 < (temp->x+temp->width) )	&&
				( gadget->y1 > temp->y)						&& 
				( gadget->y1 < (temp->y+temp->height) )
			)	
		{
				//gadget->status =1;
				return 1;
		}
		

		if (	( gadget->x2 > temp->x)						&&
				( gadget->x2 < (temp->x+temp->width) )	&&
				( gadget->y2 > temp->y)						&& 
				( gadget->y2 < (temp->y+temp->height) )
			)
		{
				//gadget->status =1;
				return 1;
		}
		

		temp = temp->next;
	}
	return 0;
}
#endif


int ui_mouse_on_gadget( UI_GADGET * gadget )
{
	int x, y, z;
	
	mouse_get_pos(&x, &y, &z);
	if ((x >= gadget->x1) && (x <= gadget->x2-1) &&	(y >= gadget->y1) &&	(y <= gadget->y2-1) )
	{
#if 0	// check is no longer required - if it is under another window, that dialog's handler would have returned 1
		if (is_under_another_window(dlg, gadget))
			return 0;
#endif
		return 1;
	} else
		return 0;
}

int ui_gadget_do(UI_DIALOG *dlg, UI_GADGET *g, d_event *event)
{
	switch( g->kind )
	{
		case 1:
			return ui_button_do(dlg, (UI_GADGET_BUTTON *)g, event);
			break;
		case 2:
			return ui_listbox_do(dlg, (UI_GADGET_LISTBOX *)g, event);
			break;
		case 3:
			return ui_scrollbar_do(dlg, (UI_GADGET_SCROLLBAR *)g, event);
			break;
		case 4:
			return ui_radio_do(dlg, (UI_GADGET_RADIO *)g, event);
			break;
		case 5:
			return ui_checkbox_do(dlg, (UI_GADGET_CHECKBOX *)g, event);
			break;
		case 6:
			return ui_inputbox_do(dlg, (UI_GADGET_INPUTBOX *)g, event);
			break;
		case 7:
			return ui_userbox_do(dlg, (UI_GADGET_USERBOX *)g, event);
			break;
		case 8:
			return ui_keytrap_do((UI_GADGET_KEYTRAP *)g, event);
			break;
		case 9:
			return ui_icon_do(dlg, (UI_GADGET_ICON *)g, event);
			break;
	}
	
	return 0;
}

int ui_gadget_send_event(UI_DIALOG *dlg, event_type type, UI_GADGET *gadget)
{
	event_gadget event;
	
	event.type = type;
	event.gadget = gadget;
	
	if (gadget->parent)
		return ui_gadget_do(dlg, gadget->parent, (d_event *) &event);

	return window_send_event(ui_dialog_get_window(dlg), (d_event *) &event);
}

UI_GADGET *ui_event_get_gadget(d_event *event)
{
	Assert(event->type >= EVENT_UI_GADGET_PRESSED);	// Any UI event
	return ((event_gadget *) event)->gadget;
}

int ui_dialog_do_gadgets(UI_DIALOG * dlg, d_event *event)
{
	int keypress = 0;
	UI_GADGET * tmp, * tmp1;
	window *wind;
	int rval = 0;

	if (event->type == EVENT_KEY_COMMAND)
		keypress = event_key_get(event);

	tmp = dlg->gadget;

	if (tmp == NULL) return 0;

	if (selected_gadget==NULL)
		selected_gadget = tmp;

	tmp1 = dlg->keyboard_focus_gadget;

	do
	{
		if (ui_mouse_on_gadget(tmp) && B1_JUST_PRESSED )
		{
			selected_gadget = tmp;
			if (tmp->parent!=NULL)
			{
				while (tmp->parent != NULL )
					tmp = tmp->parent;
				dlg->keyboard_focus_gadget = tmp;
				break;
			}
			else
			{
				dlg->keyboard_focus_gadget = tmp;
				break;
			}
		}
		if ( tmp->hotkey == keypress )
		{
			dlg->keyboard_focus_gadget = tmp;
			break;
		}
		tmp = tmp->next;
	} while( tmp != dlg->gadget );

	if (dlg->keyboard_focus_gadget != NULL)
	{
		switch (keypress )
		{
			case (KEY_TAB):
				if ( dlg->keyboard_focus_gadget->when_tab != NULL )
					dlg->keyboard_focus_gadget = dlg->keyboard_focus_gadget->when_tab;
				break;
			case (KEY_TAB+KEY_SHIFTED):
				if ( dlg->keyboard_focus_gadget->when_btab != NULL )
					dlg->keyboard_focus_gadget = dlg->keyboard_focus_gadget->when_btab;
				break;
			case (KEY_UP):
				if ( dlg->keyboard_focus_gadget->when_up != NULL )
					dlg->keyboard_focus_gadget = dlg->keyboard_focus_gadget->when_up;
	  			break;
			case (KEY_DOWN):
				if ( dlg->keyboard_focus_gadget->when_down != NULL )
					dlg->keyboard_focus_gadget = dlg->keyboard_focus_gadget->when_down;
				break;
			case (KEY_LEFT):
				if ( dlg->keyboard_focus_gadget->when_left != NULL )
					dlg->keyboard_focus_gadget = dlg->keyboard_focus_gadget->when_left;
				break;
			case (KEY_RIGHT):
				if ( dlg->keyboard_focus_gadget->when_right != NULL )
					dlg->keyboard_focus_gadget = dlg->keyboard_focus_gadget->when_right;
				break;
		}
	}

	if (dlg->keyboard_focus_gadget != tmp1)
	{
		if (dlg->keyboard_focus_gadget != NULL )
			dlg->keyboard_focus_gadget->status = 1;
		if (tmp1 != NULL )
			tmp1->status = 1;
		rval = 1;
		
		if (keypress)
			return rval;
	}

	tmp = dlg->gadget;
	wind = ui_dialog_get_window(dlg);
	do
	{
		// If it is under another dialog, that dialog's handler would have returned 1 for mouse events.
		// Key events are handled in a priority depending on the window ordering.
		//if (!is_under_another_window( dlg, tmp ))
			rval = ui_gadget_do(dlg, tmp, event);
		
		if (!window_exists(wind))
			break;

		tmp = tmp->next;
	} while(/*!rval &&*/ tmp != dlg->gadget);	// have to look for pesky scrollbars in case an arrow button or arrow key are held down
	
	return rval;
}



UI_GADGET * ui_gadget_get_next( UI_GADGET * gadget )
{
	UI_GADGET * tmp;

	tmp = gadget->next;

	while( tmp != gadget && (tmp->parent!=NULL) )
		tmp = tmp->next;

	return tmp;
}

UI_GADGET * ui_gadget_get_prev( UI_GADGET * gadget )
{
	UI_GADGET * tmp;

	tmp = gadget->prev;

	while( tmp != gadget && (tmp->parent!=NULL) )
		tmp = tmp->prev;

	return tmp;
}

void ui_gadget_calc_keys( UI_DIALOG * dlg)
{
	UI_GADGET * tmp;

	tmp = dlg->gadget;

	if (tmp==NULL) return;

	do
	{
		tmp->when_tab = ui_gadget_get_next(tmp);
		tmp->when_btab = ui_gadget_get_prev(tmp);

		tmp = tmp->next;
	} while( tmp != dlg->gadget );

}
