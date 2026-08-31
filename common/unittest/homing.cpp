#include "homing.h"

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Rebirth homing
#include <boost/test/unit_test.hpp>

namespace {

using namespace dcx;

static constexpr fix homing_turn_time{F1_0 / 30};
static constexpr fix d2_retention_dot{7 * F1_0 / 8 + F1_0 / 64 - F1_0 / 16 - homing_turn_time};

struct trajectory_result
{
	vms_vector position;
	vms_vector velocity;
	vms_matrix orientation;
	unsigned turns;
	int stored_track_goal;
};

static trajectory_result run_open_space_robot_homer(int stored_track_goal, const bool retain_updated_goal, const bool target_available, const unsigned initial_target_death_tick = 0, const bool replacement_available = false)
{
	static constexpr int player_object{17};
	static constexpr int replacement_player_object{18};
	static constexpr int player_type{4};
	static constexpr int ghost_type{12};
	static constexpr unsigned tracker_object{6};
	vms_vector position{};
	vms_vector velocity{.x = i2f(20), .y = 0, .z = 0};
	vms_matrix orientation{
		.rvec = {.x = 0, .y = -F1_0, .z = 0},
		.uvec = {.x = 0, .y = 0, .z = F1_0},
		.fvec = {.x = F1_0, .y = 0, .z = 0},
	};
	unsigned turns{};
	for (unsigned tick = 1; tick <= 60; ++tick)
	{
		const vms_vector target_position{
			.x = i2f(100 + static_cast<int>(tick)),
			.y = i2f(static_cast<int>(tick)) / 2,
			.z = 0,
		};
		const auto target_vector{vm_vec_build_sub(target_position, position)};
		auto target_direction{vm_vec_normalized_quick(target_vector)};
		const auto target_dot{vm_vec_build_dot(target_direction, orientation.fvec)};
		const bool initial_target_is_dead{initial_target_death_tick && tick >= initial_target_death_tick};
		const int stored_target_type{stored_track_goal == player_object && initial_target_is_dead ? ghost_type : player_type};
		const bool retained{stored_track_goal != -1 && homing_target_is_alive_and_visible(stored_target_type, player_type, ghost_type, false) && target_dot >= d2_retention_dot && ((tracker_object ^ tick) % 8)};
		const bool live_target_available{target_available && (!initial_target_is_dead || replacement_available)};
		const bool acquired{live_target_available && !retained && !((tracker_object ^ tick) % 4) && target_dot > 7 * F1_0 / 8};
		const int acquired_track_goal{initial_target_is_dead ? replacement_player_object : player_object};
		const int updated_track_goal{retained ? stored_track_goal : acquired ? acquired_track_goal : -1};
		if (retain_updated_goal)
			stored_track_goal = updated_track_goal;
		if (updated_track_goal != -1)
		{
			const auto turn{homing_turn_velocity(velocity, target_vector, i2f(40), homing_turn_time, false)};
			velocity = turn.velocity;
			orientation = homing_turn_orientation(orientation, turn.normalized_velocity, homing_turn_time, 16);
			++turns;
		}
		vm_vec_scale_add2(position, velocity, homing_turn_time);
	}
	return {position, velocity, orientation, turns, stored_track_goal};
}

BOOST_AUTO_TEST_CASE(target_types_follow_firing_object_and_game_mode)
{
	static constexpr int player_type{4};
	static constexpr int robot_type{2};
	static constexpr int ghost_type{12};
	const auto check = [](const homing_target_types &types, const int primary, const int secondary) {
		BOOST_CHECK_EQUAL(types.primary, primary);
		BOOST_CHECK_EQUAL(types.secondary, secondary);
	};
	BOOST_CHECK(homing_target_is_alive_and_visible(player_type, player_type, ghost_type, false));
	BOOST_CHECK(!homing_target_is_alive_and_visible(player_type, player_type, ghost_type, true));
	BOOST_CHECK(homing_target_is_alive_and_visible(robot_type, player_type, ghost_type, false));
	BOOST_CHECK(!homing_target_is_alive_and_visible(ghost_type, player_type, ghost_type, false));

	check(get_homing_target_types(false, false, false, true, false, player_type, robot_type), robot_type, -1);
	check(get_homing_target_types(false, false, false, false, false, player_type, robot_type), player_type, -1);
	check(get_homing_target_types(true, true, true, true, false, player_type, robot_type), robot_type, -1);
	check(get_homing_target_types(true, false, false, true, false, player_type, robot_type), player_type, -1);
	check(get_homing_target_types(true, false, true, true, false, player_type, robot_type), player_type, robot_type);
	check(get_homing_target_types(true, false, true, false, false, player_type, robot_type), player_type, -1);
	check(get_homing_target_types(true, false, true, false, true, player_type, robot_type), player_type, robot_type);
}

BOOST_AUTO_TEST_CASE(only_direct_robot_fire_needs_an_initial_target_scan)
{
	static constexpr int player_type{4};
	static constexpr int robot_type{2};
	static constexpr int weapon_type{5};
	BOOST_CHECK(homing_weapon_needs_initial_target(true, robot_type, robot_type));
	BOOST_CHECK(!homing_weapon_needs_initial_target(true, player_type, robot_type));
	BOOST_CHECK(!homing_weapon_needs_initial_target(true, weapon_type, robot_type));
	BOOST_CHECK(!homing_weapon_needs_initial_target(false, robot_type, robot_type));
}

BOOST_AUTO_TEST_CASE(recorded_parent_type_survives_shooter_becoming_ghost)
{
	static constexpr int player_type{4};
	static constexpr int robot_type{2};
	static constexpr int ghost_type{12};
	static constexpr int recorded_parent_type{player_type};
	static constexpr int current_parent_object_type{ghost_type};
	const auto recorded_types{get_homing_target_types(true, true, true, recorded_parent_type == player_type, false, player_type, robot_type)};
	const auto current_types{get_homing_target_types(true, true, true, current_parent_object_type == player_type, false, player_type, robot_type)};
	BOOST_CHECK_EQUAL(recorded_types.primary, robot_type);
	BOOST_CHECK_EQUAL(current_types.primary, player_type);
}

BOOST_AUTO_TEST_CASE(initial_robot_target_matches_persistent_target_oracle)
{
	const auto result{run_open_space_robot_homer(17, true, true)};
	BOOST_CHECK_EQUAL(result.turns, 60);
	BOOST_CHECK_EQUAL(result.stored_track_goal, 17);
	BOOST_CHECK_EQUAL(result.position.x, 4348568);
	BOOST_CHECK_EQUAL(result.position.y, 568252);
	BOOST_CHECK_EQUAL(result.velocity.x, 2376199);
	BOOST_CHECK_EQUAL(result.velocity.y, 533998);
	BOOST_CHECK_EQUAL(result.orientation.fvec.x, 64022);
	BOOST_CHECK_EQUAL(result.orientation.fvec.y, 14002);
}

BOOST_AUTO_TEST_CASE(reacquired_robot_target_is_retained)
{
	const auto result{run_open_space_robot_homer(-1, true, true)};
	const auto unretained{run_open_space_robot_homer(-1, false, true)};
	BOOST_CHECK_EQUAL(result.turns, 59);
	BOOST_CHECK_EQUAL(result.stored_track_goal, 17);
	BOOST_CHECK_EQUAL(result.position.x, 4308017);
	BOOST_CHECK_EQUAL(result.position.y, 563940);
	BOOST_CHECK_EQUAL(result.velocity.x, 2376891);
	BOOST_CHECK_EQUAL(result.velocity.y, 532321);
	BOOST_CHECK_EQUAL(result.orientation.fvec.x, 64033);
	BOOST_CHECK_EQUAL(result.orientation.fvec.y, 13955);
	BOOST_CHECK_EQUAL(unretained.turns, 15);
	BOOST_CHECK_EQUAL(unretained.position.x, 3165625);
	BOOST_CHECK_EQUAL(unretained.position.y, 351497);
	BOOST_CHECK_GT(static_cast<fix>(vm_vec_dist(result.position, unretained.position)), i2f(10));
}

BOOST_AUTO_TEST_CASE(robot_homer_without_valid_target_remains_untracked)
{
	const auto result{run_open_space_robot_homer(-1, true, false)};
	BOOST_CHECK_EQUAL(result.turns, 0);
	BOOST_CHECK_EQUAL(result.stored_track_goal, -1);
}

BOOST_AUTO_TEST_CASE(dead_player_target_is_replaced_by_live_player)
{
	const auto result{run_open_space_robot_homer(17, true, true, 20, true)};
	BOOST_CHECK_EQUAL(result.turns, 58);
	BOOST_CHECK_EQUAL(result.stored_track_goal, 18);
}

BOOST_AUTO_TEST_CASE(d2_polygon_turn_matches_original_30hz_kernel)
{
	const auto result{homing_turn_velocity(
		{.x = i2f(20), .y = 0, .z = 0},
		{.x = i2f(30), .y = i2f(40), .z = 0},
		i2f(40), homing_turn_time, false)};
	BOOST_CHECK_EQUAL(result.velocity.x, 1143332);
	BOOST_CHECK_EQUAL(result.velocity.y, 562872);
	BOOST_CHECK_EQUAL(result.velocity.z, 0);
	BOOST_CHECK_EQUAL(result.normalized_velocity.x, 55323);
	BOOST_CHECK_EQUAL(result.normalized_velocity.y, 27236);
	BOOST_CHECK_EQUAL(result.normalized_velocity.z, 0);
	BOOST_CHECK_EQUAL(result.velocity_target_dot, 38362);
}

BOOST_AUTO_TEST_CASE(d2_polygon_orientation_matches_original_30hz_kernel)
{
	const vms_matrix orientation{
		.rvec = {.x = F1_0, .y = 0, .z = 0},
		.uvec = {.x = 0, .y = F1_0, .z = 0},
		.fvec = {.x = F1_0, .y = 0, .z = 0},
	};
	const auto result{homing_turn_orientation(orientation, {.x = 55323, .y = 27236, .z = 0}, homing_turn_time, 16)};
	BOOST_CHECK_EQUAL(result.fvec.x, 64784);
	BOOST_CHECK_EQUAL(result.fvec.y, 9898);
	BOOST_CHECK_EQUAL(result.fvec.z, 0);
}

BOOST_AUTO_TEST_CASE(d1_polygon_orientation_matches_original_30hz_kernel)
{
	const vms_matrix orientation{
		.rvec = {.x = F1_0, .y = 0, .z = 0},
		.uvec = {.x = 0, .y = F1_0, .z = 0},
		.fvec = {.x = F1_0, .y = 0, .z = 0},
	};
	const auto result{homing_turn_orientation(orientation, {.x = 1143332, .y = 562872, .z = 0}, homing_turn_time, 8)};
	BOOST_CHECK_EQUAL(result.fvec.x, 60739);
	BOOST_CHECK_EQUAL(result.fvec.y, 24610);
	BOOST_CHECK_EQUAL(result.fvec.z, 0);
}

}
