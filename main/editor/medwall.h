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
 * $Revision: 1.2 $
 * $Author: schaffner $
 * $Date: 2004-08-28 23:17:45 $
 * 
 * Created from version 1.6 of main\wall.h
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2001/10/25 02:27:17  bradleyb
 * attempt at support for editor, makefile changes, etc
 *
 * Revision 1.1.1.1  1999/06/14 22:02:40  donut
 * Import of d1x 1.37 source.
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

