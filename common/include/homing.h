/*
 * This file is part of the DXX-Rebirth project <https://github.com/dxx-rebirth/dxx-rebirth/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

#pragma once

#include "vecmat.h"

namespace dcx {

struct homing_target_types
{
	int primary;
	int secondary;
};

[[nodiscard]]
constexpr bool homing_target_is_alive_and_visible(const int target_type, const int player_type, const int ghost_type, const bool player_cloaked)
{
	return target_type != ghost_type && (target_type != player_type || !player_cloaked);
}

[[nodiscard]]
constexpr bool homing_weapon_needs_initial_target(const bool homing, const int direct_parent_type, const int robot_type)
{
	return homing && direct_parent_type == robot_type;
}

[[nodiscard]]
constexpr homing_target_types get_homing_target_types(const bool multiplayer, const bool cooperative, const bool multiplayer_robots, const bool fired_by_player, const bool robots_kill_robots, const int player_type, const int robot_type)
{
	if (!fired_by_player)
		return {player_type, robots_kill_robots ? robot_type : -1};
	if (!multiplayer || cooperative)
		return {robot_type, -1};
	return {player_type, multiplayer_robots ? robot_type : -1};
}

struct homing_turn_result
{
	vms_vector velocity;
	vms_vector normalized_velocity;
	fix velocity_target_dot;
};

/* One turn of the original velocity blend.  Rebirth invokes this at its
 * selected fixed homing rate rather than once per rendered frame.
 */
[[nodiscard]]
homing_turn_result homing_turn_velocity(vms_vector velocity, vms_vector vector_to_object, fix max_speed, fix turn_time, bool double_target_vector);

[[nodiscard]]
vms_matrix homing_turn_orientation(const vms_matrix &orientation, vms_vector normalized_velocity, fix turn_time, fix orientation_scale);

}
