/*
 * This file is part of the DXX-Rebirth project <https://github.com/dxx-rebirth/dxx-rebirth/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

#define DXX_BUILD_DESCENT 2
#include "thief_state.h"

#include <algorithm>
#include <array>
#include <limits>

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Rebirth thief state
#include <boost/test/unit_test.hpp>

namespace {

using namespace dsx;

constexpr std::array<ai_mode, 3> thief_modes{{
	ai_mode::AIM_THIEF_ATTACK,
	ai_mode::AIM_THIEF_RETREAT,
	ai_mode::AIM_THIEF_WAIT,
}};

thief_state round_trip(const thief_state &state)
{
	std::array<uint8_t, thief_state_size> buf;
	write_thief_state(state, buf);
	const auto result = read_thief_state(buf);
	BOOST_REQUIRE(result.has_value());
	return *result;
}

}

BOOST_AUTO_TEST_CASE(snapshot_preserves_each_phase_and_remaining_cooldown)
{
	for (const auto mode : thief_modes) {
		for (const fix remaining : {0, 1, F1_0 * 7 + F1_0 / 3, std::numeric_limits<fix>::max()}) {
			const auto result = round_trip({mode, remaining});
			BOOST_CHECK(result.mode == mode);
			BOOST_TEST(result.next_action_time == remaining);
		}
	}
}

BOOST_AUTO_TEST_CASE(snapshot_matches_protocol_bytes)
{
	constexpr fix remaining = 0x12345678;
	constexpr std::array<std::array<uint8_t, 5>, 3> fixtures{{
		{{15, 0x78, 0x56, 0x34, 0x12}},
		{{16, 0x78, 0x56, 0x34, 0x12}},
		{{17, 0x78, 0x56, 0x34, 0x12}},
	}};
	for (std::size_t i = 0; i != fixtures.size(); ++i) {
		std::array<uint8_t, thief_state_size> buf;
		write_thief_state({thief_modes[i], remaining}, buf);
		BOOST_CHECK_EQUAL_COLLECTIONS(buf.begin(), buf.end(), fixtures[i].begin(), fixtures[i].end());
		const auto decoded = read_thief_state(fixtures[i]);
		BOOST_REQUIRE(decoded.has_value());
		BOOST_CHECK(decoded->mode == thief_modes[i]);
		BOOST_TEST(decoded->next_action_time == remaining);
	}
}

BOOST_AUTO_TEST_CASE(expired_action_time_is_sent_as_zero_without_changing_phase)
{
	for (const auto mode : thief_modes) {
		for (const fix expired : {-1, -F1_0, std::numeric_limits<fix>::min()}) {
			const auto result = round_trip({mode, expired});
			BOOST_CHECK(result.mode == mode);
			BOOST_TEST(result.next_action_time == 0);
		}
	}
}

BOOST_AUTO_TEST_CASE(non_thief_modes_are_rejected)
{
	for (unsigned raw_mode = 0; raw_mode != 256; ++raw_mode) {
		std::array<uint8_t, thief_state_size> buf;
		write_thief_state({ai_mode::AIM_THIEF_WAIT, F1_0}, buf);
		buf.front() = static_cast<uint8_t>(raw_mode);
		const auto result = read_thief_state(buf);
		const auto valid = std::ranges::any_of(thief_modes, [raw_mode](const auto mode) {
			return static_cast<unsigned>(mode) == raw_mode;
		});
		BOOST_TEST(result.has_value() == valid);
	}
}

BOOST_AUTO_TEST_CASE(negative_received_cooldowns_are_rejected)
{
	for (const auto mode : thief_modes) {
		std::array<uint8_t, thief_state_size> buf;
		write_thief_state({mode, 0}, buf);
		buf.back() = 0x80;
		BOOST_TEST(!read_thief_state(buf).has_value());
		std::fill(buf.begin() + 1, buf.end(), 0xff);
		BOOST_TEST(!read_thief_state(buf).has_value());
	}
}
