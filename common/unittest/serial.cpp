#include "dxxsconf.h"

#include "serial.h"

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Rebirth serial
#include <boost/test/unit_test.hpp>

static_assert(!serial::is_message<int>::value, "");
static_assert(serial::is_message<serial::message<int>>::value, "");

assert_equal(serial::detail::size_base<4>::maximum_size, 4, "");
assert_equal((serial::detail::size_base<4, 2>::minimum_size), 2, "");

static_assert(!serial::is_cxx_array<int>::value, "");
static_assert(!serial::is_cxx_array<int[1]>::value, "");
static_assert(!serial::is_cxx_array<int *>::value, "");
static_assert(!serial::is_cxx_array<std::array<int, 1> *>::value, "");
static_assert(!serial::is_cxx_array<std::array<int, 1> &>::value, "");
static_assert(serial::is_cxx_array<std::array<int, 1>>::value, "");
static_assert(serial::is_cxx_array<const std::array<int, 1>>::value, "");

static_assert(serial::udt_message_compatible_same_type<const int, int>::value, "");
static_assert(serial::udt_message_compatible_same_type<const int &, int>::value, "");
static_assert(serial::udt_message_compatible_same_type<const int &&, int>::value, "");

namespace {

struct simple_serial_test_struct
{
	uint16_t a, b;
};

DEFINE_SERIAL_MUTABLE_UDT_TO_MESSAGE(simple_serial_test_struct, s, (s.a, s.b));

}

BOOST_AUTO_TEST_CASE(read_uint16_10)
{
	constexpr uint8_t buf[2]{1, 0};
	serial::reader::bytebuffer_t b(buf);
	uint16_t u = 0;
	process_integer(b, u);
	BOOST_TEST(u == 1);
}

BOOST_AUTO_TEST_CASE(read_uint16_01)
{
	constexpr uint8_t buf[2]{0, 1};
	serial::reader::bytebuffer_t b(buf);
	uint16_t u = 0;
	process_integer(b, u);
	BOOST_TEST(u == 0x100);
}

BOOST_AUTO_TEST_CASE(read_uint32_0010)
{
	constexpr uint8_t buf[4]{0, 0, 1, 0};
	serial::reader::bytebuffer_t b(buf);
	uint32_t u = 0;
	process_integer(b, u);
	BOOST_TEST(u == 0x10000);
}

BOOST_AUTO_TEST_CASE(read_uint8_array_to_std_array)
{
	constexpr uint8_t buf[4]{1, 2, 3, 4};
	serial::reader::bytebuffer_t b(buf);
	std::array<uint8_t, 3> u{};
	process_array(b, u);
	BOOST_TEST(u[0] == 1);
	BOOST_TEST(u[1] == 2);
	BOOST_TEST(u[2] == 3);
}

BOOST_AUTO_TEST_CASE(read_uint8_array_to_uint16_array)
{
	constexpr uint8_t buf[]{1, 2, 3, 4, 5, 6, 7, 8};
	serial::reader::bytebuffer_t b(buf);
	std::array<uint16_t, 4> u{};
	process_buffer(b, u[0], u[1], u[3], u[2]);
	BOOST_TEST(u[0] == 0x201);
	BOOST_TEST(u[1] == 0x403);
	BOOST_TEST(u[2] == 0x807);
	BOOST_TEST(u[3] == 0x605);
}

BOOST_AUTO_TEST_CASE(read_uint8_array_to_uint32_array)
{
	constexpr uint8_t buf[]{1, 2, 3, 4, 5, 6, 7, 8};
	serial::reader::bytebuffer_t b(buf);
	std::array<uint32_t, 2> u{};
	process_buffer(b, u);
	BOOST_TEST(u[0] == 0x4030201);
	BOOST_TEST(u[1] == 0x8070605);
}

BOOST_AUTO_TEST_CASE(read_struct)
{
	constexpr uint8_t buf[]{1, 2, 3, 4};
	serial::reader::bytebuffer_t b(buf);
	simple_serial_test_struct s{0x1234, 0x5678};
	process_buffer(b, s);
	BOOST_TEST(s.a == 0x201);
	BOOST_TEST(s.b == 0x403);
}

BOOST_AUTO_TEST_CASE(read_sign_extend)
{
	constexpr int8_t expected = -5;
	constexpr uint8_t buf[2]{static_cast<uint8_t>(expected), 0xff};
	serial::reader::bytebuffer_t b(buf);
	int8_t value = 1;
	process_buffer(b, serial::sign_extend<int16_t>(value));
	BOOST_TEST(value == expected);
}

BOOST_AUTO_TEST_CASE(read_pad)
{
	constexpr uint8_t buf[2]{0, 0xff};
	serial::reader::bytebuffer_t b(buf);
	process_buffer(b, serial::pad<2>());
}

BOOST_AUTO_TEST_CASE(write_uint16)
{
	uint8_t buf[2]{};
	serial::writer::bytebuffer_t b(buf);
	constexpr uint16_t u = 0x100;
	process_integer(b, u);
	constexpr uint8_t expected[] = {0, 1};
	BOOST_TEST(buf == expected);
}

BOOST_AUTO_TEST_CASE(write_uint32)
{
	uint8_t buf[4]{};
	serial::writer::bytebuffer_t b(buf);
	constexpr uint32_t u = 0x12345678;
	process_integer(b, u);
	BOOST_TEST(buf[0] == 0x78);
	BOOST_TEST(buf[1] == 0x56);
	BOOST_TEST(buf[2] == 0x34);
	BOOST_TEST(buf[3] == 0x12);
}

BOOST_AUTO_TEST_CASE(write_uint8_array)
{
	uint8_t buf[4]{0xff, 0xfe, 0xfd, 0xfc};
	serial::writer::bytebuffer_t b(buf);
	constexpr std::array<uint8_t, 3> u{{1, 2, 3}};
	process_array(b, u);
	BOOST_TEST(buf[0] == 1);
	BOOST_TEST(buf[1] == 2);
	BOOST_TEST(buf[2] == 3);
	BOOST_TEST(buf[3] == 0xfc);	// check that the field was not modified
}

BOOST_AUTO_TEST_CASE(write_uint16_array)
{
	uint8_t buf[]{81, 82, 83, 84, 85, 86, 87, 88};
	serial::writer::bytebuffer_t b(buf);
	constexpr std::array<uint16_t, 4> u{{0x201, 0x403, 0x605, 0x807}};
	process_buffer(b, u[0], u[1], u[3], u[2]);
	BOOST_TEST(buf[0] == 1);
	BOOST_TEST(buf[1] == 2);
	BOOST_TEST(buf[2] == 3);
	BOOST_TEST(buf[3] == 4);
	BOOST_TEST(buf[4] == 7);
	BOOST_TEST(buf[5] == 8);
	BOOST_TEST(buf[6] == 5);
	BOOST_TEST(buf[7] == 6);
}

BOOST_AUTO_TEST_CASE(write_sign_extend)
{
	uint8_t buf[2]{};
	serial::writer::bytebuffer_t b(buf);
	constexpr int8_t value = 0x100 - 5;
	process_buffer(b, serial::sign_extend<int16_t>(value));
	BOOST_TEST(buf[0] == 0xfb);
	BOOST_TEST(buf[1] == 0xff);
}

BOOST_AUTO_TEST_CASE(write_pad)
{
	uint8_t buf[2]{};
	serial::writer::bytebuffer_t b(buf);
	process_buffer(b, serial::pad<2, 0x85>());
	BOOST_TEST(buf[0] == 0x85);
	BOOST_TEST(buf[1] == 0x85);
}
