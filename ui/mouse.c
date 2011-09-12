/* $Id: mouse.c,v 1.1.1.1 2006/03/17 19:52:18 zicodxx Exp $ */
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
 * Mouse interface routines.
 *
 */

#include <stdlib.h>
#include <SDL/SDL.h>

#include "event.h"
#include "u_mem.h"
#include "error.h"
#include "console.h"
#include "fix.h"
#include "pstypes.h"
#include "gr.h"
#include "mouse.h"
#include "ui.h"
#include "timer.h"

// 16x16

#define PTR_W 11
#define PTR_H 19

unsigned char ui_converted_mouse_pointer[PTR_W*PTR_H];
#if 0
char ui_mouse_pointer[] =  \
"1111100000000000"\
"1111111111000000"\
"1144411111111111"\
"1144444444111111"\
"1144444444444110"\
"0114444444441100"\
"0114444444411000"\
"0114444444411000"\
"0114444444441100"\
"0114444444444110"\
"0011444444444411"\
"0011444444444411"\
"0011411144444411"\
"0011111111444111"\
"0011100011111110"\
"0011000000111100";
#else

char ui_mouse_pointer[] =  \
"10000000000"\
"11000000000"\
"14100000000"\
"14410000000"\
"14441000000"\
"14444100000"\
"14444410000"\
"14444441000"\
"14444444100"\
"14444444410"\
"14444411111"\
"14114410000"\
"11014410000"\
"10001441000"\
"00001441000"\
"00001441000"\
"00000144100"\
"00000144100"\
"00000111100";
#endif

static grs_bitmap * default_pointer;


UI_MOUSE Mouse;


/*
int ui_mouse_find_gadget(short n)
{
	int i;

	for (i=1; i<=Window[n].NumItems; i++ )  {
		if ((Mouse.x >= (Window[n].Item[i].x1+Window[n].x)) &&
			(Mouse.x <= (Window[n].Item[i].x2+Window[n].x)) &&
			(Mouse.y >= (Window[n].Item[i].y1+Window[n].y)) &&
			(Mouse.y <= (Window[n].Item[i].y2+Window[n].y)) )
			return i;
	}
	return 0;
}
*/

void ui_mouse_show()
{
#ifndef __MSDOS__
	event_toggle_focus(0);
#else
	if (Mouse.hidden==1 )   {
		Mouse.hidden = 0;
		// Save the background under new pointer
		Mouse.bg_saved=1;
		Mouse.bg_x = Mouse.x; Mouse.bg_y = Mouse.y;
		gr_bm_ubitblt( Mouse.background->bm_w, Mouse.background->bm_h, 0, 0, Mouse.bg_x, Mouse.bg_y, &(grd_curscreen->sc_canvas.cv_bitmap),Mouse.background );
		// Draw the new pointer
		gr_bm_ubitbltm( Mouse.pointer->bm_w, Mouse.pointer->bm_h, Mouse.x, Mouse.y, 0, 0, Mouse.pointer, &(grd_curscreen->sc_canvas.cv_bitmap)   );
	}
#endif
}

void ui_mouse_hide()
{
#ifndef __MSDOS__
//	event_toggle_focus(1);		// causes cursor to recenter
#else
	if (Mouse.hidden==0 )   {
		Mouse.hidden = 1;
		if (Mouse.bg_saved==1)  {
			gr_bm_ubitblt( Mouse.background->bm_w, Mouse.background->bm_h, Mouse.bg_x, Mouse.bg_y, 0, 0, Mouse.background,&(grd_curscreen->sc_canvas.cv_bitmap) );
			Mouse.bg_saved = 0;
		}
	}
#endif
}

int ui_mouse_motion_process(d_event *event)
{   
	int w,h;
	int mx, my, mz;

	Assert(event->type == EVENT_MOUSE_MOVED);
	event_mouse_get_delta(event, &mx, &my, &mz);
	
	Mouse.dx = mx;
	Mouse.dy = my;

	//Mouse.x += Mouse.dx;
	//Mouse.y += Mouse.dy;
	mouse_get_pos(&mx, &my, &mz);
	Mouse.x = mx;
	Mouse.y = my;

	w = grd_curscreen->sc_w;
	h = grd_curscreen->sc_h;

	if (Mouse.x < 0 ) Mouse.x = 0;
	if (Mouse.y < 0 ) Mouse.y = 0;
//	if (Mouse.x > w-Mouse.pointer->bm_w ) Mouse.x = w - Mouse.pointer->bm_w;
//	if (Mouse.y > h-Mouse.pointer->bm_h ) Mouse.y = h - Mouse.pointer->bm_h;

	if (Mouse.x > w-3 ) Mouse.x = w - 3;
	if (Mouse.y > h-3 ) Mouse.y = h - 3;

	if ( (Mouse.bg_x!=Mouse.x) || (Mouse.bg_y!=Mouse.y) )
		Mouse.moved = 1;

	if ( (Mouse.bg_x!=Mouse.x) || (Mouse.bg_y!=Mouse.y) || (Mouse.bg_saved==0) )
	{
#ifdef __MSDOS__
		// Restore the background under old pointer
		if (Mouse.bg_saved==1)  {
			gr_bm_ubitblt( Mouse.background->bm_w, Mouse.background->bm_h, Mouse.bg_x, Mouse.bg_y, 0, 0, Mouse.background, &(grd_curscreen->sc_canvas.cv_bitmap)   );
		}
		Mouse.bg_saved = 0;

		if (!Mouse.hidden)
		{
			// Save the background under new pointer
			Mouse.bg_saved=1;
			Mouse.bg_x = Mouse.x; Mouse.bg_y = Mouse.y;

			gr_bm_ubitblt( Mouse.background->bm_w, Mouse.background->bm_h, 0, 0, Mouse.bg_x, Mouse.bg_y, &(grd_curscreen->sc_canvas.cv_bitmap),Mouse.background   );

			// Draw the new pointer
			gr_bm_ubitbltm( Mouse.pointer->bm_w, Mouse.pointer->bm_h, Mouse.x, Mouse.y, 0, 0, Mouse.pointer, &(grd_curscreen->sc_canvas.cv_bitmap)   );
		}
#endif /* __MSDOS__*/
	}
	
	return 1;
}

// straight from mouse.c's counterpart in arch/sdl
typedef struct d_event_mousebutton
{
	event_type type;
	int button;
} d_event_mousebutton;

int ui_mouse_button_process(d_event *event)
{
//	int mx, my, mz;
	int button = -1;
	int pressed;

	Assert((event->type == EVENT_MOUSE_BUTTON_DOWN) || (event->type == EVENT_MOUSE_BUTTON_UP) || (event->type == EVENT_IDLE));
	
	if (event->type != EVENT_IDLE)
		button = event_mouse_get_button(event);

	// Get the mouse's position
	//mouse_get_pos(&mx, &my, &mz);
	//Mouse.x = mx;
	//Mouse.y = my;

	pressed = event->type == EVENT_MOUSE_BUTTON_DOWN;

	Mouse.b1_last_status = Mouse.b1_status;
	Mouse.b2_last_status = Mouse.b2_status;
	Mouse.b1_status &= (BUTTON_PRESSED | BUTTON_RELEASED);
	Mouse.b2_status &= (BUTTON_PRESSED | BUTTON_RELEASED);
	
	if (event->type == EVENT_IDLE)
		return 0;

	if ( Mouse.backwards== 0 )
	{
		if (button == MBTN_LEFT)
			Mouse.b1_status = pressed ? BUTTON_PRESSED : BUTTON_RELEASED;
		else if (button == MBTN_RIGHT)
			Mouse.b2_status = pressed ? BUTTON_PRESSED : BUTTON_RELEASED;
	} else {
		if (button == MBTN_LEFT)
			Mouse.b2_status = pressed ? BUTTON_PRESSED : BUTTON_RELEASED;
		else if (button == MBTN_RIGHT)
			Mouse.b1_status = pressed ? BUTTON_PRESSED : BUTTON_RELEASED;
	}

	if ((Mouse.b1_status & BUTTON_PRESSED) && (Mouse.b1_last_status & BUTTON_RELEASED) )
	{
		if ((timer_query() <= Mouse.time_lastpressed + F1_0/5))  //&& (Mouse.moved==0)
		{
			Mouse.b1_status |= BUTTON_DOUBLE_CLICKED;
		}

		Mouse.moved = 0;
		Mouse.time_lastpressed = timer_query();
		Mouse.b1_status |= BUTTON_JUST_PRESSED;

	}
	else if ((Mouse.b1_status & BUTTON_RELEASED) && (Mouse.b1_last_status & BUTTON_PRESSED) )
		Mouse.b1_status |= BUTTON_JUST_RELEASED;

	if ((Mouse.b2_status & BUTTON_PRESSED) && (Mouse.b2_last_status & BUTTON_RELEASED) )
		Mouse.b2_status |= BUTTON_JUST_PRESSED;
	else if ((Mouse.b2_status & BUTTON_RELEASED) && (Mouse.b2_last_status & BUTTON_PRESSED) )
		Mouse.b2_status |= BUTTON_JUST_RELEASED;
	
	return 1;
}

void ui_mouse_flip_buttons()
{   short x;

	x = Mouse.b1_status;
	Mouse.b1_status = Mouse.b2_status;
	Mouse.b2_status = x;

	x = Mouse.b1_last_status;
	Mouse.b1_last_status = Mouse.b2_last_status;
	Mouse.b2_last_status = x;

	Mouse.backwards ^= 1;

}


void ui_mouse_init()
{
	int i, w,h;

#ifndef __MSDOS__
        SDL_ShowCursor(1);
#endif
	//mouse_init();

	w = grd_curscreen->sc_w;
	h = grd_curscreen->sc_h;

	//mouse_set_limits(0, 0, w - 1, h - 1);

	Mouse.x = w/2;
	Mouse.y = h/2;

	//mouse_set_pos( w/2, h/2 );

	for (i=0; i < PTR_W*PTR_H; i++ )   {
		switch (ui_mouse_pointer[i]) {
		case '0':
			ui_converted_mouse_pointer[i]=255;
			break;
		case '1':
			ui_converted_mouse_pointer[i]=CBLACK;
			break;
		case '2':
			ui_converted_mouse_pointer[i]=CGREY;
			break;
		case '3':
			ui_converted_mouse_pointer[i]=CWHITE;
			break;
		case '4':
			ui_converted_mouse_pointer[i]=CBRIGHT;
			break;
		case '5':
			ui_converted_mouse_pointer[i]=CRED;
			break;
		}
	}

	default_pointer = gr_create_bitmap_raw( PTR_W, PTR_H, ui_converted_mouse_pointer );
	Mouse.x = Mouse.y = 0;
	Mouse.backwards = 0;
	Mouse.hidden = 1;
	Mouse.b1_status = Mouse.b1_last_status = BUTTON_RELEASED;
	Mouse.b2_status = Mouse.b2_last_status = BUTTON_RELEASED;
	Mouse.b3_status = Mouse.b3_last_status = BUTTON_RELEASED;
	Mouse.bg_x = Mouse.bg_y = 0;
	Mouse.bg_saved = 0;
#ifdef __MSDOS__
	Mouse.pointer = default_pointer;
	Mouse.background = gr_create_bitmap( Mouse.pointer->bm_w, Mouse.pointer->bm_h );
#endif
	Mouse.time_lastpressed = 0;
	Mouse.moved = 0;

}

#ifdef __MSDOS__
grs_bitmap * ui_mouse_set_pointer( grs_bitmap * new )
{
	grs_bitmap * temp = Mouse.pointer;

	gr_free_bitmap( Mouse.background );

	if (new == NULL ) {
		Mouse.pointer = default_pointer;
	} else {
		Mouse.pointer = new;
	}
	Mouse.background = gr_create_bitmap( Mouse.pointer->bm_w, Mouse.pointer->bm_h );

	return temp;

}

#endif
void ui_mouse_close()
{
#ifdef __MSDOS__
	gr_free_sub_bitmap(default_pointer);

	gr_free_bitmap(Mouse.background);
#endif
}

