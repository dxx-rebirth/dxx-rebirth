/*
 * This file is part of the DXX-Rebirth project <https://github.com/dxx-rebirth/dxx-rebirth/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#include "byteutil.h"
#include <array>
#include <limits>

#undef main
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Rebirth byte order
#include <boost/test/unit_test.hpp>

static_assert(std::same_as<decltype(SWAPSHORT(int16_t{})), int16_t>);
static_assert(std::same_as<decltype(SWAPSHORT(uint16_t{})), uint16_t>);
static_assert(std::same_as<decltype(SWAPINT(int32_t{})), int32_t>);
static_assert(std::same_as<decltype(SWAPINT(uint32_t{})), uint32_t>);
static_assert(SWAPSHORT(int16_t{128}) == std::numeric_limits<int16_t>::min());
static_assert(SWAPINT(int32_t{128}) == std::numeric_limits<int32_t>::min());
static_assert(SWAPSHORT(int16_t{-1}) == -1 && SWAPINT(int32_t{-1}) == -1);
static_assert(SWAPSHORT(uint16_t{0x1234}) == 0x3412);
static_assert(SWAPINT(uint32_t{0x12345678}) == 0x78563412);

namespace {
template <typename T>
void check_unaligned_roundtrip(const T value)
{
	alignas(uint32_t) std::array<uint8_t, 6> bytes;
	bytes.fill(0xa5);
	if constexpr (sizeof(T) == sizeof(uint16_t))
	{
		PUT_INTEL_SHORT(bytes.data() + 1, value);
		BOOST_TEST(GET_INTEL_SHORT<T>(bytes.data() + 1) == value);
	}
	else
	{
		PUT_INTEL_INT(bytes.data() + 1, value);
		BOOST_TEST(GET_INTEL_INT<T>(bytes.data() + 1) == value);
	}
	BOOST_TEST(bytes.front() == 0xa5);
	BOOST_TEST(bytes[1 + sizeof(T)] == 0xa5);
}
}

BOOST_AUTO_TEST_CASE(native_signedness_and_unaligned_storage)
{
	for (const auto value : std::array<int16_t, 5>{std::numeric_limits<int16_t>::min(), -1, 0, 1, std::numeric_limits<int16_t>::max()})
		check_unaligned_roundtrip(value);
	for (const auto value : std::array<uint16_t, 4>{0, 128, 0x8000, 0xffff})
		check_unaligned_roundtrip(value);
	for (const auto value : std::array<int32_t, 5>{std::numeric_limits<int32_t>::min(), -1, 0, 1, std::numeric_limits<int32_t>::max()})
		check_unaligned_roundtrip(value);
	for (const auto value : std::array<uint32_t, 4>{0, 128, 0x80000000, 0xffffffff})
		check_unaligned_roundtrip(value);
}

BOOST_AUTO_TEST_CASE(little_endian_bytes_and_word_promotion)
{
	std::array<uint8_t, 4> bytes{};
	PUT_INTEL_INT(bytes.data(), int16_t{-2});
	const std::array<uint8_t, 4> negative{{0xfe, 0xff, 0xff, 0xff}};
	BOOST_TEST(bytes == negative, boost::test_tools::per_element());
	BOOST_TEST(GET_INTEL_SHORT<int16_t>(negative.data()) == -2);
	BOOST_TEST(GET_INTEL_INT<int32_t>(negative.data()) == -2);
	PUT_INTEL_INT(bytes.data(), uint16_t{0xabcd});
	const std::array<uint8_t, 4> positive{{0xcd, 0xab, 0, 0}};
	BOOST_TEST(bytes == positive, boost::test_tools::per_element());
	PUT_INTEL_SHORT(bytes.data(), int16_t{-2});
	BOOST_TEST(bytes[0] == 0xfe);
	BOOST_TEST(bytes[1] == 0xff);
	uint16_t short_word{};
	PUT_INTEL_SHORT(&short_word, int16_t{-2});
	BOOST_TEST(GET_INTEL_SHORT<int16_t>(&short_word) == -2);
	uint32_t long_word{};
	PUT_INTEL_INT(&long_word, int32_t{-2});
	BOOST_TEST(GET_INTEL_INT<int32_t>(&long_word) == -2);
}
