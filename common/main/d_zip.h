/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include <iterator>
#include <tuple>
#include <type_traits>
#include "dxxsconf.h"
#include <utility>
#include "backports-ranges.h"

enum class zip_sequence_length_selector : uint32_t;

namespace d_zip {

namespace detail {

template <typename... T>
void discard_arguments(T &&...)
{
}

template <std::size_t... N, typename... range_iterator_type>
void increment_iterator(std::tuple<range_iterator_type...> &iterator, std::index_sequence<N...>)
{
	/* Order of evaluation is irrelevant, so pass the results to a
	 * discarding function.  This permits the compiler to evaluate the
	 * expression elements in any order.
	 */
	discard_arguments(++(std::get<N>(iterator))...);
}

template <std::size_t... N, typename... range_iterator_type>
auto dereference_iterator(const std::tuple<range_iterator_type...> &iterator, std::index_sequence<N...>)
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
		decltype(*(std::get<N>(iterator)))...
		>(*(std::get<N>(iterator))...);
}

/* The overloads of get_static_size are declared, but never defined.  They
 * exist only for use in decltype() expressions.
 */
template <typename T, std::size_t N>
std::integral_constant<std::size_t, N> get_static_size(const T (&)[N]);

template <typename T>
requires(requires {
	typename std::tuple_size<T>::type;
	})
typename std::tuple_size<T>::type get_static_size(const T &);

std::nullptr_t get_static_size(...);

/* Given a zip_sequence_length_selector and an index N in a sequence, evaluate
 * to std::true_type if the element at index N is examined, and otherwise
 * std::false_type.
 *
 * This is a convenience type to keep the test in one place, rather than
 * duplicating the cast+mask everywhere it is needed.
 */
template <zip_sequence_length_selector examine_end_range, std::size_t N>
using examine_zip_element = std::integral_constant<bool, (N < 32 && (static_cast<uint32_t>(examine_end_range) & (1u << N)))>;

/* Given a zip_sequence_length_selector and a sequence of static sizes In,
 * return a new sequence of static sizes On where:
 * - If index N is selected, On=In.
 * - Otherwise, On=std::nullptr_t.
 *
 * The output sequence discards static size information for elements that will
 * not be selected at runtime.
 */
template <zip_sequence_length_selector examine_end_range, std::size_t... indexN, typename... sizeN>
std::tuple<typename std::conditional<examine_zip_element<examine_end_range, indexN>::value, sizeN, std::nullptr_t>::type ...> get_examined_static_size(std::index_sequence<indexN...>, std::tuple<sizeN...>);

template <typename>
struct minimum_static_size;

/* A tuple of one element is trivially the size from that element, regardless
 * of the value of size.
 */
template <typename size>
struct minimum_static_size<std::tuple<size>>
{
	using type = size;
};

/* If the first element is indeterminate, drop it and use the remainder of the
 * sequence.
 */
template <typename... sizeN>
requires(sizeof...(sizeN) > 0)
struct minimum_static_size<std::tuple<std::nullptr_t, sizeN...>> : minimum_static_size<std::tuple<sizeN...>>
{
};

/* If the second element is indeterminate, drop it and use the remainder of the
 * sequence.
 */
template <typename size0, typename... sizeN>
requires(requires {
	/* Disable size0 == std::nullptr_t, since that would make
	 * minimum_static_size<std::nullptr_t, std::nullptr_t> ambiguous between
	 * this definition and the preceding one.
	 */
	size0::value;
})
struct minimum_static_size<std::tuple<size0, std::nullptr_t, sizeN...>> : minimum_static_size<std::tuple<size0, sizeN...>>
{
};

/* If both of the first two elements are determinate, use a sequence consisting
 * of the lesser of those two, and all the other values passed through without
 * change.  Subsequent iterations of the template will fold down the remaining
 * values until the base case is reached.
 */
template <typename size0, typename size1, typename... sizeN>
requires(requires {
	size0::value < size1::value;
})
struct minimum_static_size<std::tuple<size0, size1, sizeN...>> : minimum_static_size<std::tuple<typename std::conditional<(size0::value < size1::value), size0, size1>::type, sizeN...>>
{
};

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

template <zip_sequence_length_selector mask, typename range_iterator, typename range_index>
struct iterator_end_type;

/* Given as inputs:
 * - a selector mask
 * - a tuple of iterator types <In>
 * - a std::index_sequence of the same length as the tuple
 *
 * Produce a nested type named `type` defined as tuple<On> where On=In if the
 * iterator is selected in by the mask, and On=std::ignore if the iterator is
 * not selected.  The resulting tuple will then collapse down, avoiding the
 * need to save non-selected types In in the end iterator.
 */
template <zip_sequence_length_selector mask, typename... range_iterator, std::size_t... range_index>
requires(mask != zip_sequence_length_selector{} && sizeof...(range_index) == sizeof...(range_iterator))
struct iterator_end_type<mask, std::tuple<range_iterator...>, std::index_sequence<range_index...>>
{
	using type = std::tuple<typename std::conditional<examine_zip_element<mask, range_index>::value, range_iterator, decltype(std::ignore)>::type...>;
};

template <typename end_iterator_type, typename range>
static constexpr auto capture_end_iterator(range &&r)
{
	if constexpr (std::is_same<end_iterator_type, decltype(std::ignore)>::value)
		return std::ignore;
	else
		return std::ranges::end(r);
}

template <typename... end_iterator_type, typename... range>
static constexpr auto capture_end_iterators(const std::tuple<end_iterator_type...> *, range &&...r)
{
	return std::tuple<end_iterator_type...>(capture_end_iterator<end_iterator_type>(r)...);
}

template <zip_sequence_length_selector mask, typename zip_iterator, std::size_t... N>
bool compare_zip_iterators(const zip_iterator &l, const zip_iterator &r, std::index_sequence<N...>)
{
	/* For each member of the iterator, check if the member is excluded by the mask or
	 * if the member compares equal between the two instances.  Excluded
	 * members are not checked for equality.  When one iterator is `end()`,
	 * excluded members of that iterator may be default-constructed instead of
	 * set to the value of `rangeN.end()`, so examining such members would lead
	 * to incorrect results.
	 *
	 * Note the use of `||`, which is atypical for an equality comparison.  By
	 * design, a zip iterator should terminate when any of the component
	 * sequences reaches its end, so use of `||` is correct for that purpose.
	 */
	return ((examine_zip_element<mask, N>::value && std::get<N>(l) == std::get<N>(r)) || ... || false);
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
template <typename range_index_type, zip_sequence_length_selector examine_end_range, typename range0_iterator_type, typename... rangeN_iterator_type>
requires(
	requires {
		typename std::iterator_traits<range0_iterator_type>::difference_type;
	}
)
class zip_iterator : std::tuple<range0_iterator_type, rangeN_iterator_type...>
{
	using base_type = std::tuple<range0_iterator_type, rangeN_iterator_type...>;
	/* Prior to C++17, range-based for insisted on the same type for
	 * `begin` and `end`, so method `end_internal` must return a full iterator,
	 * even though most of it is a waste.  To save some work, values that are
	 * used for ignored fields are default-constructed (if possible)
	 * instead of copy-constructed from the begin iterator.
	 *
	 * Even in C++20, many STL algorithms assume that `.begin()` and `.end()`
	 * produce the same type, so this class continues to produce a full-sized
	 * end iterator.
	 */
	template <std::size_t, typename begin_element_type, typename end_element_type, typename end_tuple_type>
		requires(std::is_same<end_element_type, decltype(std::ignore)>::value && std::is_default_constructible<begin_element_type>::value)
		static begin_element_type end_construct_element(const end_tuple_type &)
		{
			/* The type `begin_element_type` can be default-constructed,
			 * and the value will be ignored later.  Use any valid instance of
			 * `begin_element_type`.  The easiest such instance to get is a
			 * default-constructed value.
			 */
			return begin_element_type();
		}
	template <std::size_t I, typename begin_element_type, typename end_element_type, typename end_tuple_type>
		requires(std::is_same<end_element_type, decltype(std::ignore)>::value && !std::is_default_constructible<begin_element_type>::value)
		begin_element_type end_construct_element(const end_tuple_type &) const
		{
			/* The type cannot be default-constructed, but the value will be
			 * ignored.  Return a copy from the begin iterator, because the end
			 * iterator did not retain a value for this index.
			 */
			return std::get<I>(*this);
		}
	template <std::size_t I, typename begin_element_type, typename end_element_type, typename end_tuple_type>
		requires(!std::is_same<end_element_type, decltype(std::ignore)>::value)
		static begin_element_type end_construct_element(const end_tuple_type &end_iter)
		{
			/* The value will not be ignored, so a valid value must be provided
			 * from the end iterator.
			 */
			return std::get<I>(end_iter);
		}
protected:
	template <typename end_tuple_type, std::size_t... N>
		zip_iterator end_internal(const end_tuple_type &end_iter, std::index_sequence<N...>) const
		{
			return zip_iterator(this->template end_construct_element<N, typename std::tuple_element<N, base_type>::type, typename std::tuple_element<N, end_tuple_type>::type, end_tuple_type>(end_iter)...);
		}
	using index_sequence_type = std::make_index_sequence<1 + sizeof...(rangeN_iterator_type)>;
public:
	using difference_type = typename std::iterator_traits<range0_iterator_type>::difference_type;
	using index_type = range_index_type;
	using value_type = decltype(d_zip::detail::dereference_iterator(std::declval<base_type>(), index_sequence_type()));
	using pointer = value_type *;
	using reference = value_type &;
	using iterator_category = std::forward_iterator_tag;
	using base_type::base_type;
	auto operator*() const
	{
		return d_zip::detail::dereference_iterator(*this, index_sequence_type());
	}
	zip_iterator &operator++()
	{
		d_zip::detail::increment_iterator(*this, index_sequence_type());
		return *this;
	}
	/* operator++(int) is currently unused, but is required to satisfy
	 * the concept check on forward iterator.
	 */
	zip_iterator operator++(int)
	{
		auto result = *this;
		d_zip::detail::increment_iterator(*this, index_sequence_type());
		return result;
	}
	difference_type operator-(const zip_iterator &i) const
	{
		return std::get<0>(*this) - std::get<0>(i);
	}
	bool operator==(const zip_iterator &i) const
	{
		return d_zip::detail::compare_zip_iterators<examine_end_range, base_type>(*this, i, index_sequence_type());
	}
};

template <typename range_index_type, zip_sequence_length_selector examine_end_range, typename... rangeN_iterator_type>
requires(examine_end_range != zip_sequence_length_selector{} && sizeof...(rangeN_iterator_type) > 0)
class zip : zip_iterator<range_index_type, examine_end_range, rangeN_iterator_type...>
{
	typename d_zip::detail::iterator_end_type<examine_end_range, std::tuple<rangeN_iterator_type...>, std::make_index_sequence<sizeof...(rangeN_iterator_type)>>::type m_end;
public:
	using iterator = zip_iterator<range_index_type, examine_end_range, rangeN_iterator_type...>;
	using typename iterator::index_type;
	template <typename... rangeN,
		/* Tuple types in range_all_static_sizes are either an
		 * integral_constant reporting the static size of the corresponding
		 * range, or std::nullptr_t if the range has no known static size.
		 */
		typename range_all_static_sizes = std::tuple<decltype(d_zip::detail::get_static_size(std::declval<rangeN>()))...>,
		/* Type minimum_all_static_size is an integral_constant reporting the
		 * static size of the shortest static range, or std::nullptr_t if no
		 * ranges have a known static size.
		 */
		typename minimum_all_static_size = typename d_zip::detail::minimum_static_size<range_all_static_sizes>::type,
		/* Filter range_all_static_sizes to discard static sizes for elements
		 * that will not be examined at runtime.
		 */
		typename range_examined_static_sizes = decltype(d_zip::detail::get_examined_static_size<examine_end_range>(std::make_index_sequence<std::tuple_size<range_all_static_sizes>::value>(), std::declval<range_all_static_sizes>())),
		/* Type minimum_examined_static_size is an integral_constant reporting
		 * the static size of the shortest examined static range, or
		 * std::nullptr_t if no examined ranges have a known static size.
		 */
		typename minimum_examined_static_size = typename d_zip::detail::minimum_static_size<range_examined_static_sizes>::type
		>
		requires(
			sizeof...(rangeN) == sizeof...(rangeN_iterator_type) &&
			/* If no range has a static size, then minimum_examined_static_size
			 * is nullptr_t and this requirement is skipped.  Otherwise, at
			 * least one examined range has a static size, so check that it is
			 * not longer than an unexamined static range.
			 */
			(std::is_same<minimum_examined_static_size, std::nullptr_t>::value || (minimum_examined_static_size::value <= minimum_all_static_size::value))
			)
		constexpr zip(rangeN &&... rN) :
			iterator(std::begin(rN)...), m_end(d_zip::detail::capture_end_iterators(static_cast<decltype(m_end) *>(nullptr), rN...))
	{
	}
	template <zip_sequence_length_selector selector, typename... rangeN>
		constexpr zip(std::integral_constant<zip_sequence_length_selector, selector>, rangeN &&... rN) :
			zip(std::forward<rangeN>(rN)...)
	{
	}
	[[nodiscard]]
	iterator begin() const { return *this; }
	[[nodiscard]]
	iterator end() const
	{
		return this->end_internal(m_end, typename iterator::index_sequence_type());
	}
};

template <typename range_index_type, zip_sequence_length_selector examine_end_range, typename... rangeN_iterator_type>
inline constexpr bool std::ranges::enable_borrowed_range<zip<range_index_type, examine_end_range, rangeN_iterator_type...>> = true;

template <typename... rangeN>
requires(... && std::ranges::borrowed_range<rangeN>)
zip(rangeN &&... rN) -> zip<decltype(d_zip::detail::range_index_type(static_cast<std::tuple<rangeN...> *>(nullptr))), zip_sequence_length_selector{1}, decltype(std::begin(rN))...>;

template <zip_sequence_length_selector selector>
using zip_sequence_selector = std::integral_constant<zip_sequence_length_selector, selector>;

template <zip_sequence_length_selector selector, typename... rangeN>
requires(... && std::ranges::borrowed_range<rangeN>)
zip(zip_sequence_selector<selector>, rangeN &&... rN) -> zip<decltype(d_zip::detail::range_index_type(static_cast<std::tuple<rangeN...> *>(nullptr))), selector, decltype(std::begin(rN))...>;
