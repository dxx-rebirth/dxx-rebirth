#include "d_range.h"
#include "d_zip.h"
#include <array>
#include <vector>

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Rebirth zip
#include <boost/test/unit_test.hpp>

/* Test that a zipped range is empty when the component range is empty.
 */
BOOST_AUTO_TEST_CASE(zip_empty)
{
	bool empty = true;
	std::array<int, 0> a;
	for (auto &&v : zip(a))
	{
		(void)v;
		empty = false;
	}
	BOOST_TEST(empty);
}

/* Test that a zipped range is the length of the first sequence.
 *
 * Note that there is no test for when the first sequence is longer than
 * the later ones, since such a test would increment some iterators past
 * their end.
 */
BOOST_AUTO_TEST_CASE(zip_length)
{
	unsigned count = 0;
	std::array<int, 2> a;
	short b[4];
	for (auto &&v : zip(a, b))
	{
		(void)v;
		++ count;
	}
	BOOST_TEST(count == a.size());
}

/* Test that a zipped value references the underlying storage.
 */
BOOST_AUTO_TEST_CASE(zip_passthrough_modifications)
{
	std::array<int, 1> a{{1}};
	short b[1]{2};
	for (auto &&v : zip(a, b))
	{
		++ std::get<0>(v);
		++ std::get<1>(v);
	}
	BOOST_TEST(a[0] == 2);
	BOOST_TEST(b[0] == 3);
}

/* Test that a zipped range can zip an xrange, and produce the correct
 * underlying value.
 */
BOOST_AUTO_TEST_CASE(zip_xrange)
{
	for (auto &&v : zip(xrange(1u)))
	{
		BOOST_TEST(std::get<0>(v) == 0);
	}
}
