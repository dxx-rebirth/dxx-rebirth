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
 */

#ifndef _CNTRLCEN_H
#define _CNTRLCEN_H

#include "vecmat.h"
#include "object.h"
#include "wall.h"
#include "switch.h"

#ifdef __cplusplus

#define MAX_CONTROLCEN_LINKS    10

struct control_center_triggers
{
	short   num_links;
	short   seg[MAX_CONTROLCEN_LINKS];
	short   side[MAX_CONTROLCEN_LINKS];
};

extern control_center_triggers ControlCenterTriggers;

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
struct reactor {
#if defined(DXX_BUILD_DESCENT_II)
	int model_num;
#endif
	int n_guns;
	/* Location of the gun on the reactor model */
	vms_vector gun_points[MAX_CONTROLCEN_GUNS];
	/* Orientation of the gun on the reactor model */
	vms_vector gun_dirs[MAX_CONTROLCEN_GUNS];
};

#if defined(DXX_BUILD_DESCENT_I)
#define MAX_REACTORS	1
#elif defined(DXX_BUILD_DESCENT_II)
#define MAX_REACTORS 7
#define DEFAULT_CONTROL_CENTER_EXPLOSION_TIME 30    // Note: Usually uses Alan_pavlish_reactor_times, but can be overridden in editor.

extern int Num_reactors;
extern int Base_control_center_explosion_time;      // how long to blow up on insane
extern int Reactor_strength;

/*
 * reads n reactor structs from a PHYSFS_file
 */
extern int reactor_read_n(reactor *r, int n, PHYSFS_file *fp);
#endif

extern reactor Reactors[MAX_REACTORS];

static inline int get_num_reactor_models()
{
#if defined(DXX_BUILD_DESCENT_I)
	return 1;
#elif defined(DXX_BUILD_DESCENT_II)
	return Num_reactors;
#endif
}

static inline int get_reactor_model_number(int id)
{
#if defined(DXX_BUILD_DESCENT_I)
	return id;
#elif defined(DXX_BUILD_DESCENT_II)
	return Reactors[id].model_num;
#endif
}

static inline reactor *get_reactor_definition(int id)
{
#if defined(DXX_BUILD_DESCENT_I)
	(void)id;
	return &Reactors[0];
#elif defined(DXX_BUILD_DESCENT_II)
	return &Reactors[id];
#endif
}
#else
struct reactor;
#endif

//@@extern int N_controlcen_guns;
extern int Control_center_been_hit;
extern int Control_center_player_been_seen;
extern int Control_center_next_fire_time;
extern int Control_center_present;
extern int Dead_controlcen_object_num;

// do whatever this thing does in a frame
extern void do_controlcen_frame(object *obj);

// Initialize control center for a level.
// Call when a new level is started.
extern void init_controlcen_for_level(void);
extern void calc_controlcen_gun_point(reactor *reactor, object *obj,int gun_num);

extern void do_controlcen_destroyed_stuff(object *objp);
extern void do_controlcen_dead_frame(void);

extern fix Countdown_timer;
extern int Control_center_destroyed, Countdown_seconds_left, Total_countdown_time;

/*
 * reads n control_center_triggers structs from a PHYSFS_file and swaps if specified
 */
void control_center_triggers_read_swap(control_center_triggers *cct, int swap, PHYSFS_file *fp);

/*
 * reads n control_center_triggers structs from a PHYSFS_file
 */
static inline void control_center_triggers_read(control_center_triggers *cct, PHYSFS_file *fp)
{
	control_center_triggers_read_swap(cct, 0, fp);
}

void control_center_triggers_write(const control_center_triggers *cct, PHYSFS_file *fp);

#endif

#endif /* _CNTRLCEN_H */
