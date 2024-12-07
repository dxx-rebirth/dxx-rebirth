#include "d_enumerate.h"
#include "d_zip.h"
#include <list>

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Rebirth enumerate
#include <boost/test/unit_test.hpp>

template <typename range_index_type, std::input_or_output_iterator range_iterator_type, std::sentinel_for<range_iterator_type> sentinel_type>
std::ostream &boost_test_print_type(std::ostream &ostr, const enumerated_iterator<range_index_type, range_iterator_type, sentinel_type> &right)
{
	auto &&[idx, val] = *right;
	return (ostr << "enumerated_iterator(" << idx << ", " << val << ")");
}

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
	unsigned count{0};
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

BOOST_AUTO_TEST_CASE(enumerate_zip)
{
	const std::array<int, 4> a{{1, 2, 3, 4}};
	const std::array<int, 4> b{{5, 6, 7, 8}};
	for (auto &&[idx, va, vb] : enumerate(zip(a, b), 1))
	{
		BOOST_CHECK_EQUAL(idx, va);
		BOOST_CHECK_EQUAL(idx + 4, vb);
	}
}

BOOST_AUTO_TEST_CASE(enumerate_bidirectional)
{
	const std::list<int> a{1, 2};
	const auto &&en = enumerate(a);
	auto i = en.begin();
	static_assert(std::bidirectional_iterator<decltype(i)>);
	static_assert(!std::random_access_iterator<decltype(i)>);
	{
		auto &&[idx, val] = *i;
		BOOST_CHECK_EQUAL(idx, 0);
		BOOST_CHECK_EQUAL(val, 1);
	}
	{
		const auto j{i};
		BOOST_CHECK_EQUAL(j, i);
		++i;
		BOOST_CHECK_EQUAL(j, en.begin());
		BOOST_CHECK_NE(j, i);
	}
	{
		auto &&[idx, val] = *i;
		BOOST_CHECK_EQUAL(idx, 1);
		BOOST_CHECK_EQUAL(val, 2);
	}
	--i;
	{
		auto &&[idx, val] = *i;
		BOOST_CHECK_EQUAL(idx, 0);
		BOOST_CHECK_EQUAL(val, 1);
	}
}

BOOST_AUTO_TEST_CASE(enumerate_random_access)
{
	const std::vector<int> a{4, 5, 6};
	const auto &&en = enumerate(a);
	auto i = en.begin();
	static_assert(std::bidirectional_iterator<decltype(i)>);
	static_assert(std::random_access_iterator<decltype(i)>);
	{
		auto &&[idx, val] = *i;
		BOOST_CHECK_EQUAL(idx, 0);
		BOOST_CHECK_EQUAL(val, 4);
	}
	{
		auto &&[idx, val] = i[2];
		BOOST_CHECK_EQUAL(idx, 2);
		BOOST_CHECK_EQUAL(val, 6);
	}
	{
		const auto j = i + 2;
		BOOST_CHECK_NE(i, j);
		i += 2;
		BOOST_CHECK_EQUAL(i, j);
	}
	{
		auto &&[idx, val] = *i;
		BOOST_CHECK_EQUAL(idx, 2);
		BOOST_CHECK_EQUAL(val, 6);
	}
	{
		const auto j = i - 2;
		BOOST_CHECK_NE(i, j);
		i -= 2;
		BOOST_CHECK_EQUAL(i, j);
	}
	{
		auto &&[idx, val] = *i;
		BOOST_CHECK_EQUAL(idx, 0);
		BOOST_CHECK_EQUAL(val, 4);
	}
}

/* Type system tests can be done at compile-time.  Applying them as
 * static_assert can produce a better error message than letting it fail
 * at runtime.
 */
template <typename Expected, typename enumerate_type>
concept assert_index_type = std::same_as<Expected, typename enumerate_type::index_type>;

static_assert(assert_index_type<std::size_t, decltype(enumerate(std::declval<std::array<int, 1> &>()))>);

template <typename T>
struct custom_index_type : std::array<int, 1>
{
	using index_type = T;
	void operator[](typename std::remove_reference<T>::type);
};
enum class e1 : unsigned char;

static_assert(assert_index_type<e1, decltype(enumerate(std::declval<custom_index_type<e1>&>()))>);
