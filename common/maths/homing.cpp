/*
 * This file is part of the DXX-Rebirth project <https://github.com/dxx-rebirth/dxx-rebirth/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

#include "homing.h"

namespace dcx {

homing_turn_result homing_turn_velocity(vms_vector velocity, vms_vector vector_to_object, const fix max_speed, const fix turn_time, const bool double_target_vector)
{
	vm_vec_normalize_quick(vector_to_object);
	auto &&[speed_magnitude, normalized_velocity] = vm_vec_normalize_quick_with_magnitude(velocity);
	fix speed{speed_magnitude};
	if (speed + F1_0 < max_speed)
	{
		speed += fixmul(max_speed, turn_time / 2);
		if (speed > max_speed)
			speed = max_speed;
	}
	const auto velocity_target_dot{vm_vec_build_dot(normalized_velocity, vector_to_object)};
	vm_vec_add2(normalized_velocity, vector_to_object);
	if (double_target_vector)
		vm_vec_add2(normalized_velocity, vector_to_object);
	vm_vec_normalize_quick(normalized_velocity);
	velocity = normalized_velocity;
	vm_vec_scale(velocity, speed);
	return {velocity, normalized_velocity, velocity_target_dot};
}

vms_matrix homing_turn_orientation(const vms_matrix &orientation, vms_vector normalized_velocity, const fix turn_time, const fix orientation_scale)
{
	vm_vec_scale(normalized_velocity, turn_time * orientation_scale);
	vm_vec_add2(normalized_velocity, orientation.fvec);
	return vm_vector_to_matrix(vm_vec_normalized_quick(normalized_velocity));
}

}
