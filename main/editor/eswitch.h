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
 * $Source: /cvs/cvsroot/d2x/main/editor/eswitch.h,v $
 * $Revision: 1.1 $
 * $Author: btb $
 * $Date: 2004-12-19 13:54:27 $
 * 
 * Headers for switch adding functions
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  1999/06/14 22:02:35  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:35:40  john
 * Version 2.0! No anonymous unions, Watcom 10.0, with no need
 * for bitmaps.tbl.
 * 
 * Revision 1.6  1994/05/30  20:22:35  yuan
 * New triggers.
 * 
 * Revision 1.5  1994/05/27  10:34:35  yuan
 * Added new Dialog boxes for Walls and Triggers.
 * 
 * Revision 1.4  1994/05/25  18:08:39  yuan
 * Revamping walls and triggers interface.
 * Wall interface complete, but triggers are still in progress.
 * 
 * Revision 1.3  1994/04/28  23:46:56  yuan
 * Added prototype for remove_trigger.
 * 
 * Revision 1.2  1994/03/15  16:34:20  yuan
 * Fixed bm loader (might have some changes in walls and switches)
 * 
 * Revision 1.1  1994/03/10  14:49:03  yuan
 * Initial revision
 * 
 * 
 */

#ifndef _ESWITCH_H
#define _ESWITCH_H

#include "inferno.h"
#include "segment.h"
#include "switch.h"

extern int bind_wall_to_trigger();

extern int trigger_remove();

extern int remove_trigger(segment *seg, short side);

extern void close_trigger_window();

extern void do_trigger_window();

#endif

