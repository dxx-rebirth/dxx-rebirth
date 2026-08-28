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

BOOST_AUTO_TEST_CASE(motion_is_independent_of_event_partitioning)
{
	dcx::mouse_delta_state single_event;
	single_event.update({{8, -12, 3}});

	dcx::mouse_delta_state partitioned_events;
	partitioned_events.update({{1, -2, 0}});
	partitioned_events.update({{2, -3, 1}});
	partitioned_events.update({{5, -7, 2}});

	BOOST_TEST(consume(single_event) == consume(partitioned_events), boost::test_tools::per_element());
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

BOOST_AUTO_TEST_CASE(trailing_zero_event_does_not_discard_motion)
{
	dcx::mouse_delta_state state;
	state.update({{4, -6, 2}});
	state.update({{0, 0, 0}});
	const std::array<int64_t, 3> expected{{4, -6, 2}};
	BOOST_TEST(consume(state) == expected, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(opposing_events_produce_net_motion)
{
	dcx::mouse_delta_state state;
	state.update({{7, -3, 1}});
	state.update({{-5, 8, -1}});
	const std::array<int64_t, 3> expected{{2, 5, 0}};
	BOOST_TEST(consume(state) == expected, boost::test_tools::per_element());
}
