/* $Id: controls.h,v 1.2 2003-10-10 09:36:34 btb Exp $ */
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
 * Header for controls.c
 *
 * Old Log:
 * Revision 1.1  1995/05/16  15:55:31  allender
 * Initial revision
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

extern fix Afterburner_charge;

#endif
