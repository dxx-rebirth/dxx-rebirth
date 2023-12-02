#include "partial_range.h"
#include <type_traits>
#include <vector>

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Rebirth partial_range
#include <boost/test/unit_test.hpp>

#define DXX_TEST_IGNORE_RETURN(EXPR)	({	auto &&r = EXPR; static_cast<void>(r); })
#define OPTIMIZER_HIDE_VARIABLE(V)	asm("" : "=rm" (V) : "0" (V) : "memory")

BOOST_TEST_SPECIALIZED_COLLECTION_COMPARE(std::vector<int>);

BOOST_AUTO_TEST_CASE(exception_past_end)
{
	std::vector<int> vec;
	using partial_range_type = decltype(partial_range(vec, 1u));
	/* Test that trying to create a range which exceeds the end of the
	 * container raises an exception.
	 */
	BOOST_CHECK_THROW(
		DXX_TEST_IGNORE_RETURN(partial_range(vec, 1u)),
		partial_range_type::partial_range_error);
	/* Even if the range is empty, if it is beyond the end, then it is
	 * an error.
	 */
	BOOST_CHECK_THROW(
		DXX_TEST_IGNORE_RETURN(partial_range(vec, 1u, 1u)),
		partial_range_type::partial_range_error);
}

BOOST_AUTO_TEST_CASE(empty_range_end_0)
{
	std::vector<int> vec;
	bool empty = true;
	auto &&r = partial_range(vec, 0u);
	for (auto &&v : r)
	{
		(void)v;
		empty = false;
	}
	BOOST_TEST(empty);
	BOOST_TEST(r.empty());
}

BOOST_AUTO_TEST_CASE(empty_range_begin_1_end_1)
{
	std::vector<int> vec{1};
	bool empty = true;
	auto &&r = partial_range(vec, 1u, 1u);
	for (auto &&v : r)
	{
		(void)v;
		empty = false;
	}
	BOOST_TEST(empty);
	BOOST_TEST(r.empty());
}

BOOST_AUTO_TEST_CASE(empty_range_begin_2_past_end_1)
{
	std::vector<int> vec{1, 2, 3};
	bool empty = true;
	auto &&r = partial_range(vec, 2u, 1u);
	for (auto &&v : r)
	{
		(void)v;
		empty = false;
	}
	BOOST_TEST(empty);
	BOOST_TEST(r.empty());
}

BOOST_AUTO_TEST_CASE(range_slice_end_2)
{
	std::vector<int> vec{1, 2, 3};
	std::vector<int> out;
	for (auto &&v : partial_range(vec, 2u))
		out.emplace_back(v);
	std::vector<int> expected{1, 2};
	BOOST_TEST(out == expected);
}

BOOST_AUTO_TEST_CASE(range_slice_begin_1_end_2)
{
	std::vector<int> vec{1, 2, 3};
	std::vector<int> out;
	for (auto &&v : partial_range(vec, 1u, 2u))
		out.emplace_back(v);
	std::vector<int> expected{2};
	BOOST_TEST(out == expected);
}

BOOST_AUTO_TEST_CASE(range_slice_reversed_begin_1_end_3)
{
	std::vector<int> vec{1, 2, 3, 4};
	std::vector<int> out;
	for (auto &&v : partial_range(vec, 1u, 3u).reversed())
		out.emplace_back(v);
	std::vector<int> expected{3, 2};
	BOOST_TEST(out == expected);
}

/* Type system tests can be done at compile-time.  Applying them as
 * static_assert can produce a better error message than letting it fail
 * at runtime.
 */
template <typename Expected, typename partial_range_type>
concept assert_index_type = std::same_as<Expected, typename partial_range_type::index_type>;

static_assert(assert_index_type<void, decltype(partial_range(std::declval<std::vector<int>&>(), 0u, 1u))>);
template <typename T>
struct custom_index_type_only : std::array<int, 1>
{
	using index_type = T;
};

template <typename T>
struct custom_index_type : std::array<int, 1>
{
	using index_type = T;
	void operator[](typename std::remove_reference<T>::type);
};
enum class e1 : unsigned char;

/* The type is `void` because resolving index_type fails since `int *`
 * is not a valid argument type to operator[].
 */
static_assert(assert_index_type<void, decltype(partial_range(std::declval<custom_index_type_only<int *>&>(), 0u, 1u))>);
static_assert(assert_index_type<std::size_t, decltype(partial_range(std::declval<custom_index_type<std::size_t>&>(), 0u, 1u))>);
static_assert(assert_index_type<e1, decltype(partial_range(std::declval<custom_index_type<e1>&>(), 0u, 1u))>);
