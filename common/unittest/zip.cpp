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
	std::array<int, 2> a{};
	short b[4]{};
	for (auto &&[va, vb] : zip(a, b))
	{
		asm("" : "+rm" (va), "+rm" (vb));
		++ count;
	}
	BOOST_TEST(count == a.size());
}

BOOST_AUTO_TEST_CASE(zip_length_selection_01s)
{
	unsigned count = 0;
	std::array<int, 2> b{};
	short a[4]{};
	for (auto &&[va, vb] : zip(zip_sequence_selector<zip_sequence_length_selector{2}>(), a, b))
	{
		asm("" : "+rm" (va), "+rm" (vb));
		++ count;
	}
	BOOST_TEST(count == b.size());
}

BOOST_AUTO_TEST_CASE(zip_length_selection_01v)
{
	unsigned count = 0;
	std::vector<int> b{1, 2};
	short a[3]{};
	for (auto &&[va, vb] : zip(zip_sequence_selector<zip_sequence_length_selector{2}>(), a, b))
	{
		asm("" : "+rm" (va), "+rm" (vb));
		++ count;
	}
	BOOST_TEST(count == b.size());
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

BOOST_AUTO_TEST_CASE(zip_selection_11_short_first)
{
	unsigned count = 0;
	std::vector<int> b{1, 2};
	short a[3]{4, 5, 6};
	for (auto &&[vb, va] : zip(zip_sequence_selector<zip_sequence_length_selector{3}>(), b, a))
	{
		++ count;
		va += 10 + count;
		vb += 20 + count;
	}
	BOOST_TEST(count == b.size());
	BOOST_TEST(a[0] == 15);
	BOOST_TEST(a[1] == 17);
	BOOST_TEST(a[2] == 6);
	BOOST_TEST(b[0] == 22);
	BOOST_TEST(b[1] == 24);
}

BOOST_AUTO_TEST_CASE(zip_selection_11_short_second)
{
	unsigned count = 0;
	std::vector<int> b{1, 2};
	short a[3]{4, 5, 6};
	for (auto &&[va, vb] : zip(zip_sequence_selector<zip_sequence_length_selector{3}>(), a, b))
	{
		++ count;
		va += 10 + count;
		vb += 20 + count;
	}
	BOOST_TEST(count == b.size());
	BOOST_TEST(a[0] == 15);
	BOOST_TEST(a[1] == 17);
	BOOST_TEST(a[2] == 6);
	BOOST_TEST(b[0] == 22);
	BOOST_TEST(b[1] == 24);
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

/* Type system tests can be done at compile-time.  Applying them as
 * static_assert can produce a better error message than letting it fail
 * at runtime.
 */
template <typename Expected, typename zip_type>
concept assert_index_type = std::same_as<Expected, typename zip_type::index_type>;

static_assert(assert_index_type<void, decltype(zip(std::declval<std::array<int, 1> &>(), std::declval<std::array<float, 1> &>()))>);

template <typename T>
struct custom_index_type : std::array<int, 1>
{
	using index_type = T;
	void operator[](typename std::remove_reference<T>::type);
};
enum class e1 : unsigned char;
enum class e2 : unsigned char;

static_assert(assert_index_type<e1, decltype(zip(std::declval<custom_index_type<e1>&>(), std::declval<custom_index_type<e1>&>()))>);
static_assert(assert_index_type<void, decltype(zip(std::declval<custom_index_type<e1>&>(), std::declval<custom_index_type<e2>&>()))>);

static_assert(std::same_as<std::nullptr_t, decltype(d_zip::detail::get_static_size(std::declval<int *>()))>);
static_assert(std::same_as<std::nullptr_t, decltype(d_zip::detail::get_static_size(std::declval<std::vector<int>::iterator>()))>);
static_assert(std::same_as<std::integral_constant<std::size_t, 5>, decltype(d_zip::detail::get_static_size(std::declval<int (&)[5]>()))>);
static_assert(std::same_as<std::integral_constant<std::size_t, 6>, decltype(d_zip::detail::get_static_size(std::declval<std::array<int, 6> &>()))>);

static_assert(std::ranges::borrowed_range<decltype(zip(std::declval<int (&)[1]>()))>);

struct difference_range
{
	struct iterator {
		iterator &operator++();
		iterator operator++(int);
		constexpr auto operator<=>(const iterator &) const = default;
		using difference_type = short;
		using value_type = int;
		using pointer = value_type *;
		using reference = value_type &;
		using iterator_category = std::forward_iterator_tag;
		reference operator *() const;
		difference_type operator-(const iterator &) const;
	};
	struct index_type {};
	iterator begin();
	iterator end();
};

static_assert(std::ranges::range<difference_range &>);
static_assert(std::ranges::input_range<difference_range &>);
static_assert(std::ranges::borrowed_range<difference_range &>);
static_assert(std::same_as<difference_range::iterator::difference_type, decltype(zip(std::declval<difference_range &>()).begin())::difference_type>);

static_assert(std::same_as<std::nullptr_t, d_zip::detail::minimum_static_size<std::tuple<std::nullptr_t>>::type>);
static_assert(std::same_as<std::integral_constant<std::size_t, 1>, d_zip::detail::minimum_static_size<std::tuple<std::integral_constant<std::size_t, 1>, std::nullptr_t>>::type>);
static_assert(std::same_as<std::integral_constant<std::size_t, 1>, d_zip::detail::minimum_static_size<std::tuple<std::nullptr_t, std::integral_constant<std::size_t, 1>>>::type>);
static_assert(std::same_as<std::integral_constant<std::size_t, 2>, d_zip::detail::minimum_static_size<std::tuple<std::nullptr_t, std::integral_constant<std::size_t, 2>, std::integral_constant<std::size_t, 3>>>::type>);
static_assert(std::same_as<std::integral_constant<std::size_t, 2>, d_zip::detail::minimum_static_size<std::tuple<std::nullptr_t, std::integral_constant<std::size_t, 3>, std::nullptr_t, std::integral_constant<std::size_t, 2>>>::type>);
