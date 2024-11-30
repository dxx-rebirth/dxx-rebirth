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

#include <ranges>
#include <physfs.h>
#include <ranges>

#include "fwd-object.h"
#include "pack.h"
#include "fwd-robot.h"
#include "fwd-segment.h"
#include "fwd-window.h"

#include "fwd-partial_range.h"
#include "physfsx.h"

namespace dcx {

struct control_center_triggers : public prohibit_void_ptr<control_center_triggers>
{
	static constexpr std::integral_constant<std::size_t, 10> max_links{};
	uint8_t num_links;
	std::array<segnum_t, max_links>   seg;
	std::array<sidenum_t, max_links>  side;
};

struct v1_control_center_triggers
{
	v1_control_center_triggers(NamedPHYSFS_File);
	v1_control_center_triggers(const control_center_triggers &);
	static constexpr std::integral_constant<std::size_t, 10> max_links{};
	uint16_t num_links;
	std::array<segnum_t, max_links>   seg;
	std::array<uint16_t, max_links>   side;
};
static_assert(sizeof(v1_control_center_triggers) == (2 + (2 * 10) + (2 * 10)));
extern control_center_triggers ControlCenterTriggers;

/*
 * reads 1 control_center_triggers struct from a PHYSFS_File
 */
void control_center_triggers_read(control_center_triggers &cct, NamedPHYSFS_File fp);
void control_center_triggers_write(const control_center_triggers &cct, PHYSFS_File *fp);

}

#ifdef dsx
#include "vecmat.h"

namespace dsx {

struct reactor {
#if DXX_BUILD_DESCENT == 2
	polygon_model_index model_num;
#endif
	int n_guns;
	/* Location of the gun on the reactor model */
	std::array<vms_vector, MAX_CONTROLCEN_GUNS> gun_points;
	/* Orientation of the gun on the reactor model */
	std::array<vms_vector, MAX_CONTROLCEN_GUNS> gun_dirs;
};

// fills in arrays gun_points & gun_dirs, returns the number of guns read
void read_model_guns(const char *filename, reactor &);

#if DXX_BUILD_DESCENT == 1
constexpr std::integral_constant<unsigned, 1> MAX_REACTORS{};
constexpr std::integral_constant<unsigned, 1> Num_reactors{};
#elif defined(DXX_BUILD_DESCENT_II)
constexpr std::integral_constant<unsigned, 7> MAX_REACTORS{};
#define DEFAULT_CONTROL_CENTER_EXPLOSION_TIME 30    // Note: Usually uses Alan_pavlish_reactor_times, but can be overridden in editor.

struct d_level_shared_control_center_state
{
	int Base_control_center_explosion_time;      // how long to blow up on insane
	int Reactor_strength;
};

extern d_level_shared_control_center_state LevelSharedControlCenterState;

extern unsigned Num_reactors;

/*
 * reads n reactor structs from a PHYSFS_File
 */
void reactor_read_n(NamedPHYSFS_File fp, std::ranges::subrange<reactor *> r);
#endif

extern std::array<reactor, MAX_REACTORS> Reactors;

static inline polygon_model_index get_reactor_model_number(const uint8_t id)
{
#if DXX_BUILD_DESCENT == 1
	return polygon_model_index{id};
#elif defined(DXX_BUILD_DESCENT_II)
	return Reactors[id].model_num;
#endif
}

static inline reactor &get_reactor_definition(int id)
{
#if DXX_BUILD_DESCENT == 1
	(void)id;
	return Reactors[0];
#elif defined(DXX_BUILD_DESCENT_II)
	return Reactors[id];
#endif
}

// Initialize control center for a level.
// Call when a new level is started.
void init_controlcen_for_level(const d_robot_info_array &Robot_info);
void calc_controlcen_gun_point(object &obj);

void do_controlcen_destroyed_stuff(imobjidx_t objp);
window_event_result do_controlcen_dead_frame();
}
#endif
