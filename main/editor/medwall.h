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
 * $Source: /cvs/cvsroot/d2x/main/editor/medwall.h,v $
 * $Revision: 1.1 $
 * $Author: bradleyb $
 * $Date: 2001-10-25 02:27:17 $
 * 
 * Created from version 1.6 of main\wall.h
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  1999/06/14 22:02:40  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:35:10  john
 * Version 2.0! No anonymous unions, Watcom 10.0, with no need
 * for bitmaps.tbl.
 * 
 * Revision 1.9  1994/09/28  17:31:51  mike
 * Prototype copy_group_walls().
 * 
 * Revision 1.8  1994/08/05  21:18:16  matt
 * Allow two doors to be linked together
 * 
 * Revision 1.7  1994/06/20  22:30:10  yuan
 * Fixed crazy runaway trigger bug that Adam found
 * 
 * Revision 1.6  1994/05/30  20:22:58  yuan
 * New triggers.
 * 
 * Revision 1.5  1994/05/25  18:08:37  yuan
 * Revamping walls and triggers interface.
 * Wall interface complete, but triggers are still in progress.
 * 
 * Revision 1.4  1994/05/18  18:22:04  yuan
 * Fixed delete segment and walls bug.
 * 
 * Revision 1.3  1994/03/17  18:08:41  yuan
 * New wall stuff... Cut out switches....
 * 
 * Revision 1.2  1994/03/15  16:34:10  yuan
 * Fixed bm loader (might have some changes in walls and switches)
 * 
 * Revision 1.1  1994/02/10  17:52:01  matt
 * Initial revision
 * 
 * 
 * 
 */

#ifndef _MEDWALL_H
#define _MEDWALL_H

#include "wall.h"
#include "inferno.h"
#include "segment.h"

extern int wall_add_removable(); 

// Restores all the walls to original status
extern int wall_restore_all();

// Reset a wall.
extern void wall_reset(segment *seg, short side);

// Adds a removable wall (medwall.c)
extern int wall_add_removable();

// Adds a door (medwall.c)
extern int wall_add_door();

// Adds an illusory wall (medwall.c)
extern int wall_add_illusion();

// Removes a removable wall (medwall.c) 
extern int wall_remove_blastable(); 

// Adds a wall. (visually)
extern int wall_add_to_curside();
extern int wall_add_to_markedside(byte type);
 
// Removes a wall. (visually)
extern int wall_remove();

// Removes a specific side.
int wall_remove_side(segment *seg, short side);

extern int bind_wall_to_control_center();

extern void close_wall_window();

extern void do_wall_window();

extern int wall_link_doors();
extern int wall_unlink_door();
extern void copy_group_walls(int old_group, int new_group);

#endif

