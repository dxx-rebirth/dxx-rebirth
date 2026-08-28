/*
 * This file is part of the DXX-Rebirth project <https://github.com/dxx-rebirth/dxx-rebirth/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

#include "mouse_delta.h"

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Rebirth mouse delta
#include <boost/test/unit_test.hpp>

namespace {

std::array<int64_t, 3> consume(dcx::mouse_delta_state &state)
{
	const auto result = state.consume();
	BOOST_REQUIRE(result.has_value());
	return *result;
}

}

BOOST_AUTO_TEST_CASE(latest_motion_replaces_previous_motion)
{
	dcx::mouse_delta_state state;
	state.update({{1, -2, 0}});
	state.update({{5, -7, 2}});
	const std::array<int64_t, 3> expected{{5, -7, 2}};
	BOOST_TEST(consume(state) == expected, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(consume_clears_pending_motion)
{
	dcx::mouse_delta_state state;
	BOOST_TEST(!state.consume().has_value());
	state.update({{1, 2, 3}});
	BOOST_TEST(state.consume().has_value());
	BOOST_TEST(!state.consume().has_value());
}

BOOST_AUTO_TEST_CASE(reset_discards_pending_motion)
{
	dcx::mouse_delta_state state;
	state.update({{1, 2, 3}});
	state.reset();
	BOOST_TEST(!state.consume().has_value());
}

BOOST_AUTO_TEST_CASE(zero_motion_is_a_pending_event)
{
	dcx::mouse_delta_state state;
	state.update({{0, 0, 0}});
	const std::array<int64_t, 3> expected{};
	BOOST_TEST(consume(state) == expected, boost::test_tools::per_element());
}
