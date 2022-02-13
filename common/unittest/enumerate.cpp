#include "d_enumerate.h"
#include <array>

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Rebirth enumerate
#include <boost/test/unit_test.hpp>

/* Test that an enumerated range is empty when the component range is empty.
 */
BOOST_AUTO_TEST_CASE(enumerate_empty)
{
	bool empty = true;
	std::array<int, 0> a;
	for (auto &&[idx, v] : enumerate(a))
	{
		(void)idx;
		(void)v;
		empty = false;
	}
	BOOST_TEST(empty);
}

/* Test that an enumerated range is the length of the underlying sequence.
 */
BOOST_AUTO_TEST_CASE(enumerate_length)
{
	unsigned count = 0;
	std::array<int, 2> a;
	for (auto &&[idx, v] : enumerate(a))
	{
		(void)idx;
		(void)v;
		++ count;
	}
	BOOST_TEST(count == a.size());
}

/* Test that an enumerated value references the underlying storage.
 */
BOOST_AUTO_TEST_CASE(enumerate_passthrough_modifications)
{
	std::array<int, 1> a{{1}};
	for (auto &&[idx, v] : enumerate(a))
	{
		(void)idx;
		++ v;
	}
	BOOST_TEST(a[0] == 2);
}

/* Test that an enumerated index tracks properly.
 */
BOOST_AUTO_TEST_CASE(enumerate_starting_value)
{
	const std::array<int, 4> a{{1, 2, 3, 4}};
	for (auto &&[idx, v] : enumerate(a))
		BOOST_TEST(idx + 1 == v);
	for (auto &&[idx, v] : enumerate(a, 1))
		BOOST_TEST(idx == v);
}

/* Type system tests can be done at compile-time.  Applying them as
 * static_assert can produce a better error message than letting it fail
 * at runtime.
 */
template <typename Expected, typename enumerate_type, typename index_type = typename enumerate_type::index_type>
struct assert_index_type
{
	static_assert(std::is_same<Expected, index_type>::value);
};

template struct assert_index_type<uint_fast32_t, decltype(enumerate(std::declval<std::array<int, 1>>()))>;

template <typename T>
struct custom_index_type : std::array<int, 1>
{
	using index_type = T;
	void operator[](typename std::remove_reference<T>::type);
};
enum class e1 : unsigned char;

template struct assert_index_type<e1, decltype(enumerate(std::declval<custom_index_type<e1>&>()))>;
