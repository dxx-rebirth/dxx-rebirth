/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include <cstdint>
#include <iterator>
#include <tuple>
#include <type_traits>
#include "dxxsconf.h"
#include <utility>
#include <ranges>
#include <span>

enum class zip_sequence_length_selector : uint32_t;

namespace d_zip {

namespace detail {

/* `range_static_extent` stores the extent of a range.  If the extent is not
 * available as an integer constant expression (such as happens for
 * `std::vector<int>`, which is a range, but the extent of the range can only
 * be determined at runtime), then `dynamic_extent` is used as a placeholder.
 */
enum class range_static_extent : std::size_t
{
	dynamic_extent = std::dynamic_extent,
};

template <std::ranges::range Range>
/* By default, the static size is unknown. */
inline constexpr range_static_extent range_static_size{range_static_extent::dynamic_extent};

template <std::ranges::range Range>
/* If the range can be used to construct a `std::span`, then consider the
 * extent of the range to be the extent of the resulting span.  This allows
 * measuring any range for which `std::span` has an overload, including
 * C arrays, `std::array`, and `std::span`.
 *
 * `std::span` only accepts contiguous ranges.  If there are non-contiguous
 * ranges that can be measured at compile-time, a specialization to cover those
 * types can be added below.
 */
requires(
	requires(Range &r) {
		decltype(std::span(r))::extent;
	}
)
inline constexpr range_static_extent range_static_size<Range>{decltype(std::span(std::declval<Range &>()))::extent};

template <zip_sequence_length_selector examine_end_range, typename... range_sentinel_type>
requires(
	examine_end_range != zip_sequence_length_selector{} &&
	sizeof...(range_sentinel_type) > 0
)
struct zip_sentinel : public std::tuple<range_sentinel_type...>
{
	using std::tuple<range_sentinel_type...>::tuple;
};

template <std::size_t... range_index>
struct for_index_sequence
{
	/* This class is never instantiated, but a constructor is declared to
	 * support Class Template Argument Deduction.
	 */
	for_index_sequence(std::index_sequence<range_index...>);
	/* Run-time functions.  These are defined and called. */
	template <std::input_or_output_iterator... range_iterator_type>
		static void increment_iterator(std::tuple<range_iterator_type...> &iterator)
		{
			(static_cast<void>(++(std::get<range_index>(iterator))), ...);
		}
	template <std::bidirectional_iterator... range_iterator_type>
		static void decrement_iterator(std::tuple<range_iterator_type...> &iterator)
		{
			(static_cast<void>(--(std::get<range_index>(iterator))), ...);
		}
	template <std::input_or_output_iterator... range_iterator_type>
		static auto dereference_iterator(const std::tuple<range_iterator_type...> &iterator);
	template <typename zip_iterator, zip_sequence_length_selector examine_end_range, typename... zip_sentinel_elements>
		static constexpr bool compare_zip_iterators(const zip_iterator &iterator, const zip_sentinel<examine_end_range, zip_sentinel_elements...> &sentinel);
};

template <std::size_t... range_index>
template <std::input_or_output_iterator... range_iterator_type>
auto for_index_sequence<range_index...>::dereference_iterator(const std::tuple<range_iterator_type...> &iterator)
{
	/* std::make_tuple is not appropriate here, because the result of
	 * dereferencing the iterator may be a reference, and the resulting
	 * tuple should store the reference, not the underlying object.
	 * Calling std::make_tuple would decay such references into the
	 * underlying type.
	 *
	 * std::tie is not appropriate here, because it captures all
	 * arguments as `T &`, so it fails to compile if the result of
	 * dereferencing the iterator is not a reference.
	 */
	return std::tuple<
		decltype(*(std::get<range_index>(iterator)))...
		>(*(std::get<range_index>(iterator))...);
}

/* Given a zip_sequence_length_selector and an index N in a sequence, evaluate
 * to true if the element at index N is examined, and otherwise false.
 *
 * This is a convenience variable to keep the test in one place, rather than
 * duplicating the cast+mask everywhere it is needed.
 */
template <zip_sequence_length_selector examine_end_range, std::size_t N>
inline constexpr bool examine_zip_element{(N < 32 && (static_cast<uint32_t>(examine_end_range) & (1u << N)))};

/* `std::integer_sequence<range_static_extent, N...>` would work here, except
 * that `range_static_extent` is an `enum class` and gcc-14 requires
 * `std::is_integral_v<T>` be true for the integer type `T`.  `enum class` does
 * not satisfy `std::is_integral_v`.  Therefore, define a custom type to serve
 * an equivalent purpose to `std::integer_sequence`.
 */
template <range_static_extent... N>
struct range_extent_sequence
{
	static constexpr std::size_t size{sizeof...(N)};
};

/* Given a zip_sequence_length_selector and a sequence of static sizes In,
 * return a new sequence of static sizes `On` where:
 * - If index N is selected, `On`=`In`.
 * - Otherwise, `On`=`range_static_extent::dynamic_extent`.
 *
 * The output sequence discards static size information for elements that will
 * not be selected at runtime.
 */
template <zip_sequence_length_selector examine_end_range, std::size_t... indexN, range_static_extent... sizeN>
range_extent_sequence<(examine_zip_element<examine_end_range, indexN> ? sizeN : range_static_extent::dynamic_extent) ...> get_examined_static_size(std::index_sequence<indexN...>, range_extent_sequence<sizeN...>);

/* Yield the lesser of the first size or the next iteration of the template.
 * Subsequent iterations of the template will fold down the remaining values
 * until the base case is reached.
 */
template <range_static_extent size0, range_static_extent... sizeN>
consteval range_static_extent minimum_static_size(range_extent_sequence<size0, sizeN...>)
{
	if constexpr (sizeof...(sizeN) > 0)
		return std::min(size0, minimum_static_size(range_extent_sequence<sizeN...>()));
	else
		/* A sequence of one element is trivially the size from that element, regardless
		 * of the value of size.
		 */
		return size0;
}

void range_index_type(...);

template <
	typename... range_type,
	/* If any `range_type::index_type` is not defined, fail.
	 * If there is no common_type among all the
	 * `range_type::index_type`, fail.
	 */
	typename index_type = typename std::common_type<typename std::remove_reference<typename std::remove_reference<range_type>::type::index_type &>::type...>::type,
	/* If the common_type `index_type` is not a suitable argument to all
	 * `range_type::operator[]()`, fail.
	 */
	typename = std::void_t<decltype(std::declval<range_type &>().operator[](std::declval<index_type>()))...>
	>
index_type range_index_type(std::tuple<range_type...> *);

/* Given as inputs:
 * - a selector mask
 * - a tuple of iterator types <In>
 * - a std::index_sequence of the same length as the tuple
 *
 * Produce a type defined as zip_sentinel<On> where On=In if the iterator is
 * selected in by the mask, and On=std::ignore if the iterator is not selected.
 * The resulting tuple will then collapse down, avoiding the need to save
 * non-selected types In in the end iterator.
 */
template <zip_sequence_length_selector examine_end_range, std::ranges::range... range_type, std::size_t... range_index>
requires(examine_end_range != zip_sequence_length_selector{} && sizeof...(range_index) == sizeof...(range_type))
auto iterator_end_type(std::tuple<range_type...>, std::index_sequence<range_index...>) -> zip_sentinel<
	examine_end_range,
	typename std::conditional<
		examine_zip_element<examine_end_range, range_index>,
		decltype(std::ranges::end(std::declval<range_type &&>())),
		decltype(std::ignore)
	>::type ...
>;

template <typename end_iterator_type, std::ranges::input_range range>
static constexpr auto capture_end_iterator(range &&r)
{
	if constexpr (std::is_same<const end_iterator_type &, const decltype(std::ignore) &>::value)
		return std::ignore;
	else
		return std::ranges::end(r);
}

template <typename sentinel_type, typename... range_sentinel_types, std::ranges::input_range... rangeN>
static constexpr auto capture_end_iterators(rangeN &&...r)
{
	return sentinel_type(capture_end_iterator<range_sentinel_types>(r)...);
}

template <bool mask, std::size_t N, typename zip_iterator, typename zip_sentinel>
static constexpr auto compare_zip_iterator(const zip_iterator &l, const zip_sentinel &r)
{
	if constexpr (mask)
		/* For each included member of the iterator, check if the member compares
		 * equal between the two instances.  Excluded members use the else
		 * block.
		 */
		return std::get<N>(l) == std::get<N>(r);
	else
	{
		/* For each excluded member of the iterator, do nothing.  Excluded members
		 * are not checked for equality, because excluded members are omitted from
		 * the sentinel, instead of being set to the value of `rangeN.end()`,
		 * so examining such members is not possible.
		 */
		(void)l;
		(void)r;
		return std::false_type{};
	}
}

template <std::size_t... N>
template <typename zip_iterator, zip_sequence_length_selector examine_end_range, typename... zip_sentinel_elements>
constexpr bool for_index_sequence<N...>::compare_zip_iterators(const zip_iterator &iterator, const zip_sentinel<examine_end_range, zip_sentinel_elements...> &sentinel)
{
	/* Note the use of `||`, which is atypical for an equality comparison.  By
	 * design, a zip iterator should terminate when any of the component
	 * sequences reaches its end, so use of `||` is correct for that purpose.
	 */
	return (compare_zip_iterator<examine_zip_element<examine_end_range, N>, N>(iterator, sentinel) || ... || false);
}

}

}

/* This iterator terminates when the selected zipped range(s) terminate.  The
 * caller is responsible for ensuring that use of the zip_iterator does
 * not increment past the end of any zipped range.  This can be done by
 * ensuring that the selected zipped range(s) are not longer than any ignored
 * zipped range, or by ensuring that external logic stops the traversal
 * before the zip_iterator increments past the end.
 *
 * There is no runtime check that the below loop would be
 * safe, since a check external to the zip_iterator could stop before
 * undefined behaviour occurs.
 *
 * However, if any selected range is convertible to a C array of known
 * length or to a C++ std::array, then there is a compile-time check
 * that the shortest selected range is not longer than any other range that is
 * likewise convertible.  If a range cannot be converted to an array,
 * then its length is unknown and is not checked.  If the first range is
 * not convertible, then no ranges are checked.

	for (auto i = zip_range.begin(), e = zip_range.end(); i != e; ++i)
	{
		if (condition())
			break;
	}

 */
template <typename range_iterator_tuple, typename range_sentinel_type>
class zip_iterator;

template <std::input_or_output_iterator... range_iterator_types, typename range_sentinel_type>
class zip_iterator<std::tuple<range_iterator_types...>, range_sentinel_type> : std::tuple<range_iterator_types...>
{
	using range_iterator_tuple = std::tuple<range_iterator_types...>;
protected:
	using for_index_sequence = decltype(d_zip::detail::for_index_sequence(std::make_index_sequence<sizeof...(range_iterator_types)>()));
public:
	using range_iterator_tuple::range_iterator_tuple;
	using difference_type = typename std::incrementable_traits<
		typename std::remove_reference_t<
			std::tuple_element_t<0, range_iterator_tuple>
		>>::difference_type;
	using value_type = decltype(for_index_sequence::dereference_iterator(std::declval<range_iterator_tuple>()));
	using pointer = value_type *;
	using reference = value_type &;
	/*
	 * Given a tuple of iterator types, return the most-derived iterator_category
	 * that all the iterator types satisfy.
	 */
	using iterator_category = typename std::common_type<typename std::iterator_traits<range_iterator_types>::iterator_category...>::type;
	auto operator*() const
	{
		return for_index_sequence::dereference_iterator(static_cast<const range_iterator_tuple &>(*this));
	}
	zip_iterator &operator++()
	{
		for_index_sequence::increment_iterator(static_cast<range_iterator_tuple &>(*this));
		return *this;
	}
	/* operator++(int) is currently unused, but is required to satisfy
	 * the concept check on forward iterator.
	 */
	zip_iterator operator++(int)
	{
		auto result{*this};
		++ *this;
		return result;
	}
	zip_iterator &operator--()
		requires(std::derived_from<std::bidirectional_iterator_tag, iterator_category>)
	{
		for_index_sequence::decrement_iterator(static_cast<range_iterator_tuple &>(*this));
		return *this;
	}
	zip_iterator operator--(int)
		requires(std::derived_from<std::bidirectional_iterator_tag, iterator_category>)
	{
		auto result{*this};
		-- *this;
		return result;
	}
	auto operator-(const zip_iterator &i) const
		requires(std::derived_from<std::random_access_iterator_tag, iterator_category>)
	{
		return std::get<0>(static_cast<const range_iterator_tuple &>(*this)) - std::get<0>(static_cast<const range_iterator_tuple &>(i));
	}
	constexpr bool operator==(const zip_iterator &) const = default;
	constexpr bool operator==(const range_sentinel_type &i) const
	{
		return for_index_sequence::template compare_zip_iterators<range_iterator_tuple>(*this, i);
	}
};

template <
	zip_sequence_length_selector examine_end_range,
	/* Values in `range_all_static_sizes` are either a `range_static_extent`
	 * reporting the static size of the corresponding range, or
	 * `range_static_extent::dynamic_extent` if the range has no known
	 * static size.
	 */
	typename range_all_static_sizes,	// range_extent_sequence<Extent...>
	/* Value `minimum_all_static_size` is the `range_static_extent`
	 * reporting the static size of the shortest static range, or
	 * `range_static_extent::dynamic_extent` if no ranges have a known
	 * static size.
	 */
	d_zip::detail::range_static_extent minimum_all_static_size = d_zip::detail::minimum_static_size(range_all_static_sizes()),
	/* Filter `range_all_static_sizes` to discard static sizes for elements
	 * that will not be examined at runtime.
	 */
	typename range_examined_static_sizes = decltype(d_zip::detail::get_examined_static_size<examine_end_range>(std::make_index_sequence<range_all_static_sizes::size>(), range_all_static_sizes())),
	/* Value `minimum_examined_static_size` reports the static size of the
	 * shortest examined static range, or `range_static_extent::dynamic_extent`
	 * if no examined ranges have a known static size.
	 */
	d_zip::detail::range_static_extent minimum_examined_static_size = d_zip::detail::minimum_static_size(range_examined_static_sizes())
	>
	concept zip_static_size_bounds_check = (
		/* If no examined range has a static size, then
		 * `minimum_examined_static_size` is
		 * `range_static_extent::dynamic_extent` and no compile-time check is
		 * possible.
		 *
		 * Otherwise, at least one examined range has a static size, so check
		 * that it is not longer than an unexamined static range.
		 */
		(minimum_examined_static_size == d_zip::detail::range_static_extent::dynamic_extent || minimum_examined_static_size <= minimum_all_static_size)
	);

template <
	zip_sequence_length_selector examine_end_range,
	typename... rangeN
	>
	concept zip_input_constraints = (
		/* Require at least two ranges.  A zip of a single range adds complexity for no reason. */
		sizeof...(rangeN) > 1 &&
		(std::ranges::input_range<rangeN> && ...) &&
		/* Require borrowed ranges, because zip will return iterators that point into the ranges. */
		(std::ranges::borrowed_range<rangeN> && ...) &&
		zip_static_size_bounds_check<
			examine_end_range,
			d_zip::detail::range_extent_sequence<d_zip::detail::range_static_size<rangeN>...>
		>
	);

/* Class Template Argument Deduction cannot implicitly deduce the template type
 * arguments for `zip` because none of the types are used directly as an
 * argument to any constructor, but instead are derived from properties of the
 * constructor arguments.
 *
 * Explicit deduction guides are provided below to handle constructing `zip`
 * without specifying template argument types.
 */
template <typename range_index_type, typename range_iterator_type, typename zip_sentinel_type>
class zip;

template <typename range_index_type, typename range_iterator_type, zip_sequence_length_selector examine_end_range, typename... range_sentinel_types>
class zip<range_index_type, range_iterator_type, d_zip::detail::zip_sentinel<examine_end_range, range_sentinel_types...>> : zip_iterator<range_iterator_type, d_zip::detail::zip_sentinel<examine_end_range, range_sentinel_types...>>
{
	using sentinel_type = d_zip::detail::zip_sentinel<examine_end_range, range_sentinel_types...>;
	[[no_unique_address]]
	sentinel_type m_end;
public:
	using index_type = range_index_type;
	using iterator = zip_iterator<range_iterator_type, sentinel_type>;
	template <std::ranges::input_range... rangeN>
		requires(
			sizeof...(rangeN) > 0 &&
			(std::ranges::borrowed_range<rangeN> && ...)
		)
		constexpr zip(rangeN &&... rN) :
			iterator{std::ranges::begin(rN)...},
			m_end{d_zip::detail::capture_end_iterators<sentinel_type, range_sentinel_types...>(rN...)}
	{
	}
	template <std::ranges::input_range... rangeN>
		/* Delegate to the no-selector constructor, since the selector is not
		 * needed by the constructor.  The selector affected the type of
		 * `zip_sentinel_type`, which in turn affects how `m_end` is
		 * constructed.
		 */
		constexpr zip(std::integral_constant<zip_sequence_length_selector, examine_end_range>, rangeN &&... rN) :
			zip{std::forward<rangeN>(rN)...}
	{
	}
	[[nodiscard]]
	constexpr iterator begin() const { return *this; }
	[[nodiscard]]
	constexpr auto end() const
	{
		return m_end;
	}
};

/* zip() is always a view onto another range, and never owns the storage that
 * its iterators reference. */
template <typename range_index_type, typename range_iterator_type, typename range_sentinel_type>
inline constexpr bool std::ranges::enable_borrowed_range<zip<range_index_type, range_iterator_type, range_sentinel_type>> = true;

template <zip_sequence_length_selector selector>
using zip_sequence_selector = std::integral_constant<zip_sequence_length_selector, selector>;

template <zip_sequence_length_selector examine_end_range, std::ranges::input_range... rangeN>
requires(
	zip_input_constraints<examine_end_range, rangeN...>
)
zip(zip_sequence_selector<examine_end_range>, rangeN &&... rN) -> zip<
	/* range_index_type = */ decltype(d_zip::detail::range_index_type(static_cast<std::tuple<rangeN...> *>(nullptr))),
	/* range_iterator_type = */ std::tuple<decltype(std::ranges::begin(std::declval<rangeN &&>()))...>,
	/* range_sentinel_type = */ decltype(
		d_zip::detail::iterator_end_type<examine_end_range>(
			std::declval<std::tuple<rangeN...>>(),
			std::make_index_sequence<sizeof...(rangeN)>()
		)
	)
	>;

/* Store the default selector as a default template argument, so that the
 * template-id specified by the deduction guide can use the same text in both
 * the explicit-selector and implicit-selector guides.
 */
template <std::ranges::input_range... rangeN, zip_sequence_length_selector examine_end_range = zip_sequence_length_selector{1}>
requires(
	zip_input_constraints<examine_end_range, rangeN...>
)
zip(rangeN &&... rN) -> zip<
	/* range_index_type = */ decltype(d_zip::detail::range_index_type(static_cast<std::tuple<rangeN...> *>(nullptr))),
	/* range_iterator_type = */ std::tuple<decltype(std::ranges::begin(std::declval<rangeN &&>()))...>,
	/* range_sentinel_type = */ decltype(
		d_zip::detail::iterator_end_type<examine_end_range>(
			std::declval<std::tuple<rangeN...>>(),
			std::make_index_sequence<sizeof...(rangeN)>()
		)
	)
	>;
