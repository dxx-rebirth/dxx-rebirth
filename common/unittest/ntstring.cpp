/*
 * This file is part of the DXX-Rebirth project <https://github.com/dxx-rebirth/dxx-rebirth/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

#include "ntstring.h"
#include <algorithm>
#include <limits>

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Rebirth fixed-capacity strings
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(copy_preserves_prefix_and_clears_unused_suffix)
{
	ntstring<4> s{};
	BOOST_CHECK_EQUAL(s.copy_if("abcd"), 4);
	BOOST_CHECK_EQUAL(s.copy_if(2, "x"), 1);
	BOOST_CHECK(std::ranges::equal(s, std::array{'a', 'b', 'x', '\0', '\0'}));
	BOOST_CHECK_EQUAL(s.copy_if(2, "xyz"), 0);
	BOOST_CHECK(std::ranges::equal(s, std::array{'a', 'b', '\0', '\0', '\0'}));
	const auto saved = s;
	BOOST_CHECK_EQUAL(s.copy_if(s.size(), "z"), 0);
	BOOST_CHECK(s == saved);
}

BOOST_AUTO_TEST_CASE(bounded_input_and_empty_copies)
{
	ntstring<4> s{};
	const std::array input{'a', '\0', 'b', 'c'};
	BOOST_CHECK_EQUAL(s.copy_if(0, input.data(), input.size()), 1);
	BOOST_CHECK(std::ranges::equal(s, std::array{'a', '\0', '\0', '\0', '\0'}));
	BOOST_CHECK_EQUAL(s.copy_if(0, input.data() + 2, 2), 2);
	BOOST_CHECK(std::ranges::equal(s, std::array{'b', 'c', '\0', '\0', '\0'}));
	BOOST_CHECK_EQUAL(s.copy_if(0, input.data(), 0), 0);
	BOOST_CHECK(std::ranges::equal(s, std::array<char, 5>{}));
	ntstring<0> empty{};
	BOOST_CHECK_EQUAL(empty.copy_if(""), 0);
	BOOST_CHECK_EQUAL(empty[0], '\0');
}

BOOST_AUTO_TEST_CASE(explicit_copy_count_cannot_wrap_capacity_check)
{
	ntstring<4> s{};
	s.copy_if("abcd");
	BOOST_CHECK_EQUAL(s._copy_n(1, "x", std::numeric_limits<std::size_t>::max()), 0);
	BOOST_CHECK(std::ranges::equal(s, std::array{'a', '\0', '\0', '\0', '\0'}));
}

BOOST_AUTO_TEST_CASE(export_preserves_short_and_full_string_encoding)
{
	ntstring<4> s{};
	s.copy_if("ab");
	std::array<uint8_t, 5> output{{255, 255, 255, 255, 255}};
	BOOST_CHECK_EQUAL(s.copy_out(output), 3);
	BOOST_CHECK(std::ranges::equal(output, std::array<uint8_t, 5>{{'a', 'b', 0, 255, 255}}));
	s.copy_if("abcd");
	BOOST_CHECK_EQUAL(s.copy_out(output), 4);
	BOOST_CHECK(std::ranges::equal(output, std::array<uint8_t, 5>{{'a', 'b', 'c', 'd', 255}}));
	BOOST_CHECK_EQUAL(s.copy_out({}), 0);
	std::array<uint8_t, 2> short_output{{255, 255}};
	BOOST_CHECK_EQUAL(s.copy_out(short_output), 1);
	BOOST_CHECK(std::ranges::equal(short_output, std::array<uint8_t, 2>{{0, 255}}));
	ntstring<0> empty{};
	BOOST_CHECK_EQUAL(empty.copy_out(output), 0);
	BOOST_CHECK_EQUAL(output[0], 'a');
}
