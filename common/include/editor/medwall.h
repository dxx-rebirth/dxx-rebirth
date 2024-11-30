/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license below.
 * See COPYING.txt for license details.

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
 *
 * Created from version 1.6 of main\wall.h
 *
 */

#pragma once

#include <cstdint>
#include "fwd-segment.h"

#ifdef __cplusplus
#include "fwd-wall.h"

// Restores all the walls to original status
extern int wall_restore_all();

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
#ifdef DXX_BUILD_DESCENT
int wall_add_to_markedside(fvcvertptr &vcvertptr, wall_array &Walls, int8_t type);
#endif
 
// Removes a wall. (visually)
extern int wall_remove();

// Removes a specific side.
#ifdef DXX_BUILD_DESCENT
int wall_remove_side(vmsegptridx_t seg, sidenum_t side);
#endif

extern int bind_wall_to_control_center();

extern void close_wall_window();

extern void do_wall_window();

extern int wall_link_doors();
extern int wall_unlink_door();
extern void copy_group_walls(int old_group, int new_group);
void check_wall_validity(void);

#endif
