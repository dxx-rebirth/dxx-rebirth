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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Header for cntrlcen.c
 *
 */

#pragma once

#include <physfs.h>

#ifdef __cplusplus
#include "fwd-object.h"
#include "pack.h"
#include "fwd-segment.h"
#include "fwd-window.h"

#include "fwd-partial_range.h"

struct control_center_triggers : public prohibit_void_ptr<control_center_triggers>
{
	enum {
		max_links = 10
	};
	uint16_t num_links;
	array<segnum_t, max_links>   seg;
	array<uint16_t, max_links>   side;
};

extern control_center_triggers ControlCenterTriggers;

#ifdef dsx
#include "vecmat.h"
struct reactor {
#if defined(DXX_BUILD_DESCENT_II)
	int model_num;
#endif
	int n_guns;
	/* Location of the gun on the reactor model */
	array<vms_vector, MAX_CONTROLCEN_GUNS> gun_points;
	/* Orientation of the gun on the reactor model */
	array<vms_vector, MAX_CONTROLCEN_GUNS> gun_dirs;
};

// fills in arrays gun_points & gun_dirs, returns the number of guns read
void read_model_guns(const char *filename, reactor &);

namespace dsx {
#if defined(DXX_BUILD_DESCENT_I)
constexpr std::integral_constant<unsigned, 1> MAX_REACTORS{};
constexpr std::integral_constant<unsigned, 1> Num_reactors{};
#elif defined(DXX_BUILD_DESCENT_II)
constexpr std::integral_constant<unsigned, 7> MAX_REACTORS{};
#define DEFAULT_CONTROL_CENTER_EXPLOSION_TIME 30    // Note: Usually uses Alan_pavlish_reactor_times, but can be overridden in editor.

extern unsigned Num_reactors;
extern int Base_control_center_explosion_time;      // how long to blow up on insane
extern int Reactor_strength;

/*
 * reads n reactor structs from a PHYSFS_File
 */
void reactor_read_n(PHYSFS_File *fp, partial_range_t<reactor *> r);
#endif

extern array<reactor, MAX_REACTORS> Reactors;

static inline int get_reactor_model_number(int id)
{
#if defined(DXX_BUILD_DESCENT_I)
	return id;
#elif defined(DXX_BUILD_DESCENT_II)
	return Reactors[id].model_num;
#endif
}

static inline reactor &get_reactor_definition(int id)
{
#if defined(DXX_BUILD_DESCENT_I)
	(void)id;
	return Reactors[0];
#elif defined(DXX_BUILD_DESCENT_II)
	return Reactors[id];
#endif
}
}

namespace dcx {
//@@extern int N_controlcen_guns;
extern int Control_center_been_hit;
extern int Control_center_next_fire_time;
extern int Control_center_present;
extern objnum_t Dead_controlcen_object_num;
}

namespace dsx {
// do whatever this thing does in a frame
void do_controlcen_frame(vmobjptridx_t obj);

// Initialize control center for a level.
// Call when a new level is started.
void init_controlcen_for_level();
void calc_controlcen_gun_point(object &obj);

void do_controlcen_destroyed_stuff(imobjptridx_t objp);
window_event_result do_controlcen_dead_frame();
}
#endif

namespace dcx {
extern fix Countdown_timer;
extern int Total_countdown_time;

}

/*
 * reads n control_center_triggers structs from a PHYSFS_File
 */
void control_center_triggers_read(control_center_triggers *cct, PHYSFS_File *fp);
void control_center_triggers_write(const control_center_triggers *cct, PHYSFS_File *fp);

#endif
