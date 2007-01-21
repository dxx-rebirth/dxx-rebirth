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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/ui/mouse.c,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:39:11 $
 *
 * Mouse interface routines.
 *
 * $Log: mouse.c,v $
 * Revision 1.1.1.1  2006/03/17 19:39:11  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 22:14:39  donut
 * Import of d1x 1.37 source.
 *
 * Revision 1.8  1995/03/06  17:54:17  john
 * fixed bug with mouse color being wrong.
 * 
 * Revision 1.7  1994/09/23  14:57:52  john
 * Took out calls to init_mouse and init_keyboard.
 * 
 * Revision 1.6  1994/04/22  11:10:06  john
 * *** empty log message ***
 * 
 * Revision 1.5  1993/12/07  12:30:44  john
 * new version.
 * 
 * Revision 1.4  1993/10/26  13:46:04  john
 * *** empty log message ***
 * 
 * Revision 1.3  1993/10/05  17:55:27  matt
 * Changed default pointer
 * 
 * Revision 1.2  1993/10/05  17:31:33  john
 * *** empty log message ***
 * 
 * Revision 1.1  1993/09/20  10:35:25  john
 * Initial revision
 * 
 *
 */

#ifdef RCS
static char rcsid[] = "$Id: mouse.c,v 1.1.1.1 2006/03/17 19:39:11 zicodxx Exp $";
#endif

#include <stdlib.h>
#ifdef __SDL__
#include <SDL/SDL.h>
#endif

#include "u_mem.h"
#include "fix.h"
#include "types.h"
#include "gr.h"
#include "mouse.h"

#include "ui.h"
#include "timer.h"

// 16x16

#define PTR_W 11
#define PTR_H 19

char ui_converted_mouse_pointer[PTR_W*PTR_H];
char ui_mouse_pointer[] =  \
/*"1111100000000000"\
//"1111111111000000"\
//"1144411111111111"\
//"1144444444111111"\
//"1144444444444110"\
//"0114444444441100"\
//"0114444444411000"\
//"0114444444411000"\
//"0114444444441100"\
//"0114444444444110"\
//"0011444444444411"\
//"0011444444444411"\
//"0011411144444411"\
//"0011111111444111"\
//"0011100011111110"\
//"0011000000111100";
*/

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
#ifdef __LINUX__
  SDL_ShowCursor(1);
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
#ifndef __LINUX__
	if (Mouse.hidden==0 )   {
		Mouse.hidden = 1;
		if (Mouse.bg_saved==1)  {
			gr_bm_ubitblt( Mouse.background->bm_w, Mouse.background->bm_h, Mouse.bg_x, Mouse.bg_y, 0, 0, Mouse.background,&(grd_curscreen->sc_canvas.cv_bitmap) );
			Mouse.bg_saved = 0;
		}
	}
#endif
}

void ui_mouse_process()
{   
        int buttons,w,h;
#ifdef __LINUX__
	int new_x, new_y;
	buttons = SDL_GetMouseState(&new_x,&new_y);
	Mouse.dx = new_x - Mouse.x;
	Mouse.dy = new_y - Mouse.y;
#else

	Mouse.dx = Mouse.new_dx;
	Mouse.dy = Mouse.new_dy;
	buttons = Mouse.new_buttons;
#endif

	Mouse.x += Mouse.dx;
	Mouse.y += Mouse.dy;

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
#ifndef __LINUX__
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
#endif /* !__LINUX__*/
	}

	Mouse.b1_last_status = Mouse.b1_status;
	Mouse.b2_last_status = Mouse.b2_status;

	if ( Mouse.backwards== 0 )
	{
		if (buttons & MOUSE_LBTN )
			Mouse.b1_status = BUTTON_PRESSED;
		else
			Mouse.b1_status = BUTTON_RELEASED;
		if (buttons & MOUSE_RBTN )
			Mouse.b2_status = BUTTON_PRESSED;
		else
			Mouse.b2_status = BUTTON_RELEASED;
	} else {
		if (buttons & MOUSE_LBTN )
			Mouse.b2_status = BUTTON_PRESSED;
		else
			Mouse.b2_status = BUTTON_RELEASED;
		if (buttons & MOUSE_RBTN )
			Mouse.b1_status = BUTTON_PRESSED;
		else
			Mouse.b1_status = BUTTON_RELEASED;
	}

	if ((Mouse.b1_status & BUTTON_PRESSED) && (Mouse.b1_last_status & BUTTON_RELEASED) )
	{
		if ( (TICKER <= Mouse.time_lastpressed+5)  )  //&& (Mouse.moved==0)
			Mouse.b1_status |= BUTTON_DOUBLE_CLICKED;

		Mouse.moved = 0;
		Mouse.time_lastpressed = TICKER;
		Mouse.b1_status |= BUTTON_JUST_PRESSED;

	}
	else if ((Mouse.b1_status & BUTTON_RELEASED) && (Mouse.b1_last_status & BUTTON_PRESSED) )
		Mouse.b1_status |= BUTTON_JUST_RELEASED;

	if ((Mouse.b2_status & BUTTON_PRESSED) && (Mouse.b2_last_status & BUTTON_RELEASED) )
		Mouse.b2_status |= BUTTON_JUST_PRESSED;
	else if ((Mouse.b2_status & BUTTON_RELEASED) && (Mouse.b2_last_status & BUTTON_PRESSED) )
		Mouse.b2_status |= BUTTON_JUST_RELEASED;
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

#ifdef __LINUX__
        SDL_ShowCursor(1);
#endif
	//mouse_init();

	w = grd_curscreen->sc_w;
	h = grd_curscreen->sc_h;

// 	mouse_set_limits( 0,0, w-1, h-1 );

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
#ifndef __LINUX__
	Mouse.pointer = default_pointer;
	Mouse.background = gr_create_bitmap( Mouse.pointer->bm_w, Mouse.pointer->bm_h );
#endif
	Mouse.time_lastpressed = 0;
	Mouse.moved = 0;

}

#ifndef __LINUX__
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
#ifndef __LINUX__
	gr_free_sub_bitmap(default_pointer);

	gr_free_bitmap(Mouse.background);
#endif
}

