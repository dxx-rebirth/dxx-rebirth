/*
 * $Source: /cvs/cvsroot/d2x/arch/svgalib/event.c,v $
 * $Revision: 1.1 $
 * $Author: bradleyb $
 * $Date: 2001-10-24 09:25:05 $
 *
 * SVGALib Event related stuff
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2001/01/29 14:03:57  bradleyb
 * Fixed build, minor fixes
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include <vgakeyboard.h>
#include <vgamouse.h>

void key_handler(int scancode, int press);
//added on 10/17/98 by Hans de Goede for mouse functionality
//extern void mouse_button_handler(SDL_MouseButtonEvent *mbe);
//extern void mouse_motion_handler(SDL_MouseMotionEvent *mme);
//end this section addition - Hans

void event_poll()
{
 keyboard_update();
 mouse_update();
}

/*int event_init()
{
 initialised = 1;
 return 0;
}*/
