/* $Id: cntrlcen.h,v 1.6 2003-10-10 09:36:34 btb Exp $ */
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
 * Header for cntrlcen.c
 *
 * Old Log:
 * Revision 2.0  1995/02/27  11:30:40  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.7  1995/02/01  16:34:14  john
 * Linted.
 *
 * Revision 1.6  1995/01/03  20:19:14  john
 * Pretty good working version of game save.
 *
 * Revision 1.5  1994/11/08  12:18:28  mike
 * small explosions on control center.
 *
 * Revision 1.4  1994/11/02  18:00:12  rob
 * Added extern for Gun_pos array for network hooks.
 *
 * Revision 1.3  1994/10/20  09:47:37  mike
 * *** empty log message ***
 *
 * Revision 1.2  1994/10/17  21:35:10  matt
 * Added support for new Control Center/Main Reactor
 *
 * Revision 1.1  1994/10/17  20:24:28  matt
 * Initial revision
 *
 *
 */

#ifndef _CNTRLCEN_H
#define _CNTRLCEN_H

#include "vecmat.h"
#include "object.h"
#include "wall.h"
//#include "switch.h"

#define MAX_CONTROLCEN_GUNS     8

#define CONTROLCEN_WEAPON_NUM   6

#define MAX_CONTROLCEN_LINKS    10

typedef struct control_center_triggers {
	short   num_links;
	short   seg[MAX_CONTROLCEN_LINKS];
	short   side[MAX_CONTROLCEN_LINKS];
} __pack__ control_center_triggers;

extern control_center_triggers ControlCenterTriggers;

typedef struct reactor {
	int model_num;
	int n_guns;
	vms_vector gun_points[MAX_CONTROLCEN_GUNS];
	vms_vector gun_dirs[MAX_CONTROLCEN_GUNS];
} __pack__ reactor;

#define MAX_REACTORS 7

extern int Num_reactors;

extern reactor Reactors[MAX_REACTORS];

//@@extern int N_controlcen_guns;
extern int Control_center_been_hit;
extern int Control_center_player_been_seen;
extern int Control_center_next_fire_time;
extern int Control_center_present;
extern int Dead_controlcen_object_num;

//@@extern vms_vector controlcen_gun_points[MAX_CONTROLCEN_GUNS];
//@@extern vms_vector controlcen_gun_dirs[MAX_CONTROLCEN_GUNS];
extern vms_vector Gun_pos[MAX_CONTROLCEN_GUNS];

// do whatever this thing does in a frame
extern void do_controlcen_frame(object *obj);

// Initialize control center for a level.
// Call when a new level is started.
extern void init_controlcen_for_level(void);

extern void do_controlcen_destroyed_stuff(object *objp);
extern void do_controlcen_dead_frame(void);

#define DEFAULT_CONTROL_CENTER_EXPLOSION_TIME 30    // Note: Usually uses Alan_pavlish_reactor_times, but can be overridden in editor.

extern fix Countdown_timer;
extern int Control_center_destroyed,Countdown_seconds_left;
extern int Base_control_center_explosion_time;      // how long to blow up on insane
extern int Reactor_strength;

#ifdef FAST_FILE_IO
#define reactor_read_n(r, n, fp) cfread(r, sizeof(reactor), n, fp)
#define control_center_triggers_read_n(cct, n, fp) cfread(cct, sizeof(control_center_triggers), n, fp)
#else
/*
 * reads n reactor structs from a CFILE
 */
extern int reactor_read_n(reactor *r, int n, CFILE *fp);

/*
 * reads n control_center_triggers structs from a CFILE
 */
extern int control_center_triggers_read_n(control_center_triggers *cct, int n, CFILE *fp);
#endif

#endif /* _CNTRLCEN_H */
