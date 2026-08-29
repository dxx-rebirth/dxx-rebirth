#include "homing.h"

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Rebirth homing
#include <boost/test/unit_test.hpp>

namespace {

using namespace dcx;

static constexpr fix homing_turn_time{F1_0 / 30};
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
