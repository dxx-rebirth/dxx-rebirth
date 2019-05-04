/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include <tuple>
#include <type_traits>
#include "dxxsconf.h"
#include "compiler-integer_sequence.h"
#include "ephemeral_range.h"

/* This iterator terminates when the first zipped range terminates.  The caller
 * is responsible for ensuring that the first zipped range is not longer than
 * any other zipped range.
 */
template <typename... range_iterator_type>
class zip_iterator : std::tuple<range_iterator_type...>
{
	using base_type = std::tuple<range_iterator_type...>;
	template <typename... T>
		void discard_arguments(T &&...)
		{
		}
	template <std::size_t... N>
		void increment(std::index_sequence<N...>)
		{
			/* Order of evaluation is irrelevant, so pass the results to
			 * a discarding function.  This permits the compiler to
			 * evaluate the expression elements in any order.
			 */
			discard_arguments(++(std::get<N>(*this))...);
		}
	template <std::size_t... N>
		auto dereference(std::index_sequence<N...>) const
		{
			return std::tie(*(std::get<N>(*this))...);
		}
protected:
	/* Prior to C++17, range-based for insisted on the same type for
	 * `begin` and `end`, so method `end_internal` must return a full iterator,
	 * even though most of it is a waste.  To save some work, values that are
	 * used for ignored fields are default-constructed (if possible)
	 * instead of copy-constructed from the begin iterator.
	 */
	template <std::size_t I, typename T>
		static typename std::enable_if<std::is_default_constructible<T>::value, T>::type end_construct_ignored_element()
		{
			return T();
		}
	template <std::size_t I, typename T>
		typename std::enable_if<!std::is_default_constructible<T>::value, T>::type end_construct_ignored_element() const
		{
			return std::get<I>(*this);
		}
	template <typename E0, std::size_t... N>
		zip_iterator end_internal(const E0 &e0, std::index_sequence<0, N...>) const
		{
			return zip_iterator(e0, this->template end_construct_ignored_element<N, typename std::tuple_element<N, base_type>::type>()...);
		}
	using index_type = make_tree_index_sequence<sizeof...(range_iterator_type)>;
public:
	using base_type::base_type;
	auto operator*() const
	{
		return dereference(index_type());
	}
	zip_iterator &operator++()
	{
		increment(index_type());
		return *this;
	}
	bool operator!=(const zip_iterator &i) const
	{
		return std::get<0>(*this) != std::get<0>(i);
	}
};

template <typename range0_iterator_type, typename... rangeN_iterator_type>
class zipped_range : zip_iterator<range0_iterator_type, rangeN_iterator_type...>
{
	range0_iterator_type m_end;
public:
	using range_owns_iterated_storage = std::false_type;
	using iterator = zip_iterator<range0_iterator_type, rangeN_iterator_type...>;
	zipped_range(iterator &&i, range0_iterator_type &&e) :
		iterator(std::move(i)), m_end(std::move(e))
	{
	}
	__attribute_warn_unused_result
	iterator begin() const { return *this; }
	__attribute_warn_unused_result
	iterator end() const
	{
		return this->end_internal(m_end, typename iterator::index_type());
	}
};

template <typename range0, typename... rangeN>
static auto zip(range0 &&r0, rangeN &&... rN)
{
	using R = zipped_range<decltype(begin(r0)), decltype(begin(rN))...>;
	static_assert((!any_ephemeral_range<range0 &&, rangeN &&...>::value), "cannot zip storage of ephemeral ranges");
	return R(typename R::iterator(begin(r0), begin(rN)...), end(r0));
}
