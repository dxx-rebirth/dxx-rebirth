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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/controls.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:43:28 $
 * 
 * Header for controls.c
 * 
 * $Log: controls.h,v $
 * Revision 1.1.1.1  2006/03/17 19:43:28  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 22:12:14  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:27:17  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.4  1994/07/21  18:15:33  matt
 * Ripped out a bunch of unused stuff
 * 
 * Revision 1.3  1994/07/15  09:32:08  john
 * Changes player movement.
 * 
 * Revision 1.2  1994/06/30  19:02:22  matt
 * Moved flying controls code from physics.c to controls.c
 * 
 * Revision 1.1  1994/06/30  18:41:36  matt
 * Initial revision
 * 
 * 
 */



#ifndef _CONTROLS_H
#define _CONTROLS_H

extern int Cyberman_installed;	//SWIFT device present

void read_flying_controls( object * obj );

extern ubyte Controls_stopped;
extern ubyte Controls_always_move;

#endif
