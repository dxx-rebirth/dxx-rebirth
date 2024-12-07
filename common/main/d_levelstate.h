/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

#pragma once
#include "dxxsconf.h"
#include "object.h"
#include "morph.h"

#ifdef DXX_BUILD_DESCENT
namespace dcx {

struct d_level_unique_morph_object_state
{
	std::array<morph_data::ptr, 5> morph_objects;
	~d_level_unique_morph_object_state();
};

}

namespace dsx {

struct d_level_unique_object_state
{
	unsigned num_objects{0};
	/* `accumulated_robots` counts robots present at level entry and
	 * robots added later via materialization centers / boss gating.  It
	 * never decreases, so it is not a shortcut for counting the number
	 * of currently live objects with type OBJ_ROBOT.
	 */
	unsigned accumulated_robots;
	unsigned total_hostages;
	unsigned Debris_object_count{0};
#if DXX_BUILD_DESCENT == 2
	d_unique_buddy_state BuddyState;
	d_thief_unique_state ThiefState;
	d_guided_missile_indices Guided_missile;
#endif
	std::array<imobjidx_t, MAX_OBJECTS> free_obj_list = init_object_number_array<imobjidx_t>(std::make_index_sequence<MAX_OBJECTS>());
	object_array Objects;
	d_level_unique_boss_state BossState;
	d_level_unique_control_center_state ControlCenterState;
	vms_vector last_console_player_position;
	d_level_unique_morph_object_state MorphObjectState;
	uint8_t Level_path_created{0};
	auto &get_objects()
	{
		return Objects;
	}
	const auto &get_objects() const
	{
		return Objects;
	}
};

extern d_level_unique_object_state LevelUniqueObjectState;

}
#endif
