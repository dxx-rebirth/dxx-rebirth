/*
 * This file is part of the DXX-Rebirth project <https://github.com/dxx-rebirth/dxx-rebirth/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

#pragma once

#include "vecmat.h"

namespace dcx {

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
