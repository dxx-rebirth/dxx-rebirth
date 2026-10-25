/*
 * This file is part of the DXX-Rebirth project <https://github.com/dxx-rebirth/dxx-rebirth/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

/* `mve_audio.cpp` is not part of the common library that the tests link
 * against.  Listing it as a source of this test would build the same object
 * that the D2X program builds, in a different environment, which SCons
 * rejects.  Include it instead.
 */
#include "d2x-rebirth/libmve/mve_audio.cpp"

#include "byteutil.h"

#include <algorithm>
#include <array>
#include <cstdint>

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Rebirth MVE audio
#include <boost/test/unit_test.hpp>

namespace {

/* `mveaudio_uncompress` skips 4 unused bytes and the 2 byte swath length,
 * reads the initial left and right samples, then reads one delta table index
 * for each remaining output sample.
 */
constexpr std::size_t header_size = 10;

template <std::size_t N>
std::array<uint8_t, header_size + N> make_segment(const int16_t left, const int16_t right, const std::array<uint8_t, N> &deltas)
{
	std::array<uint8_t, header_size + N> segment{};
	PUT_INTEL_SHORT(&segment[6], left);
	PUT_INTEL_SHORT(&segment[8], right);
	std::ranges::copy(deltas, segment.begin() + header_size);
	return segment;
}

/* Delta table index 0 adds nothing, so every output sample must equal the
 * initial sample of its channel.
 */
void check_initial_samples(const int16_t left, const int16_t right)
{
	const auto segment = make_segment(left, right, std::array<uint8_t, 8>{});
	std::array<int16_t, 10> output{};
	mveaudio_uncompress(output, segment.data());
	for (std::size_t i = 0; i < output.size(); i += 2)
	{
		BOOST_TEST(output[i] == left);
		BOOST_TEST(output[i + 1] == right);
	}
}

}

/* Test that a negative initial left sample is not read as a large positive
 * value.
 */
BOOST_AUTO_TEST_CASE(negative_initial_left_sample_is_preserved)
{
	check_initial_samples(-256, 256);
}

/* Test that a negative initial right sample is not read as a large positive
 * value.
 */
BOOST_AUTO_TEST_CASE(negative_initial_right_sample_is_preserved)
{
	check_initial_samples(256, -256);
}
