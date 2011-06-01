/* $Id: cntrlcen.h,v 1.1.1.1 2006/03/17 19:55:32 zicodxx Exp $ */
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
 *
 * Header for cntrlcen.c
 *
 */

#ifndef _CNTRLCEN_H
#define _CNTRLCEN_H

#include "vecmat.h"
#include "object.h"
#include "wall.h"
#include "switch.h"

#define MAX_CONTROLCEN_GUNS     4

#define CONTROLCEN_WEAPON_NUM   6

#define MAX_CONTROLCEN_LINKS    10

typedef struct control_center_triggers {
	short   num_links;
	short   seg[MAX_CONTROLCEN_LINKS];
	short   side[MAX_CONTROLCEN_LINKS];
} __pack__ control_center_triggers;

extern control_center_triggers ControlCenterTriggers;

extern int N_controlcen_guns;
extern int Control_center_been_hit;
extern int Control_center_player_been_seen;
extern int Control_center_next_fire_time;
extern int Control_center_present;
extern int Dead_controlcen_object_num;

extern vms_vector controlcen_gun_points[MAX_CONTROLCEN_GUNS];
extern vms_vector controlcen_gun_dirs[MAX_CONTROLCEN_GUNS];
extern vms_vector Gun_pos[MAX_CONTROLCEN_GUNS];

//return the position & orientation of a gun on the control center object 
extern void calc_controlcen_gun_point(vms_vector *gun_point,vms_vector *gun_dir,object *obj,int gun_num);

// do whatever this thing does in a frame
extern void do_controlcen_frame(object *obj);

// Initialize control center for a level.
// Call when a new level is started.
extern void init_controlcen_for_level(void);

extern void do_controlcen_destroyed_stuff(object *objp);
extern void do_controlcen_dead_frame(void);

extern fix Countdown_timer;
extern int Control_center_destroyed, Countdown_seconds_left, Total_countdown_time;

/*
 * reads n control_center_triggers structs from a PHYSFS_file
 */
extern int control_center_triggers_read_n(control_center_triggers *cct, int n, PHYSFS_file *fp);

/*
 * reads n control_center_triggers structs from a PHYSFS_file and swaps if specified
 */
void control_center_triggers_read_n_swap(control_center_triggers *cct, int n, int swap, PHYSFS_file *fp);

extern int control_center_triggers_write(control_center_triggers *cct, PHYSFS_file *fp);

#endif
 
