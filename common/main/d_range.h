/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include <type_traits>
#include <utility>
#include "ephemeral_range.h"

template <typename T, bool begin>
class xrange_endpoint
{
public:
	const T value;
	constexpr xrange_endpoint(T v) :
		value(v)
	{
	}
	constexpr operator T() const
	{
		return value;
	}
};

template <typename T, T V, bool begin>
class xrange_endpoint<std::integral_constant<T, V>, begin> : public std::integral_constant<T, V>
{
public:
	constexpr xrange_endpoint() = default;
	constexpr xrange_endpoint(const std::integral_constant<T, V> &)
	{
	}
};

template <typename index_type>
class xrange_iterator
{
	index_type m_idx;
public:
	constexpr xrange_iterator(const index_type i) :
		m_idx(i)
	{
	}
	index_type operator*() const
	{
		return m_idx;
	}
	xrange_iterator &operator++()
	{
		++ m_idx;
		return *this;
	}
	constexpr bool operator!=(const xrange_iterator &i) const
	{
		return m_idx != i.m_idx;
	}
};

/* This provides an approximation of the functionality of the Python2
 * xrange object.  Python3 renamed it to `range`.  The older name is
 * kept because it is easier to find with grep.
 */
template <typename index_type, typename B = index_type, typename E = index_type>
class xrange_extent :
	public xrange_endpoint<B, true>,
	public xrange_endpoint<E, false>
{
	using begin_type = xrange_endpoint<B, true>;
	using end_type = xrange_endpoint<E, false>;
	using iterator = xrange_iterator<index_type>;
	static begin_type init_begin(B b, E e, std::true_type /* is_same<B, E> */)
	{
		return (
#ifdef DXX_CONSTANT_TRUE
			/* Compile-time only check.  Runtime handles (b > e)
			 * correctly, and it can happen in a correct program.  If it
			 * is guaranteed to happen, then the range is always empty,
			 * which likely indicates a bug.
			 */
			(DXX_CONSTANT_TRUE(!(b < e)) && (DXX_ALWAYS_ERROR_FUNCTION(xrange_is_always_empty, "begin never less than end"), 0)),
#endif
			b < e ? std::move(b) : e
		);
	}
	static begin_type init_begin(B b, E, std::false_type /* is_same<B, E> */)
	{
		return begin_type(b);
	}
public:
	using range_owns_iterated_storage = std::false_type;
	xrange_extent(B b, E e) :
		begin_type(init_begin(std::move(b), e, std::is_same<B, E>())), end_type(std::move(e))
	{
	}
	iterator begin() const
	{
		return iterator(static_cast<const begin_type &>(*this));
	}
	iterator end() const
	{
		return iterator(static_cast<const end_type &>(*this));
	}
};

/* Disallow building an `xrange` with a reference to mutable `e` as the
 * end term.  When `e` is mutable, the loop might change `e` during
 * iteration, so using `xrange` could be wrong.  If the loop does not
 * change `e`, store it in a const qualified variable, which will select
 * the next overload down instead.
 */
template <typename Tb, typename Te>
typename std::enable_if<!std::is_const<Te>::value, xrange_extent<Tb>>::type xrange(Tb &&, Te &) = delete;	// mutable `e` might change during range

template <typename Tb, typename Te>
static xrange_extent<
	typename std::common_type<Tb, Te>::type,
	typename std::remove_const<typename std::remove_reference<Tb>::type>::type,
	typename std::remove_const<typename std::remove_reference<Te>::type>::type
	> xrange(Tb &&b, Te &&e)
{
	return {std::forward<Tb>(b), std::forward<Te>(e)};
}

/* Disallow building an `xrange` with a default-constructed start term
 * and a non-unsigned end term.  This overload only restricts the
 * one-argument form of `xrange`.  Callers that need a non-unsigned end
 * term can use the two-argument form to bypass this check.
 */
template <
	typename Te,
	typename Te_rr_rc = typename std::remove_const<
		typename std::remove_reference<Te>::type
	>::type,
	typename B = typename std::enable_if<
		std::is_unsigned<Te_rr_rc>::value,
		std::integral_constant<typename std::common_type<Te_rr_rc, unsigned>::type, 0u>
	>::type
>
static typename std::enable_if<std::is_unsigned<Te_rr_rc>::value, xrange_extent<Te_rr_rc, B, Te>>::type xrange(Te &&e)
{
	return {B(), std::forward<Te>(e)};
}

template <typename Te, Te e>
typename std::enable_if<!(Te() < e), xrange_extent<Te>>::type xrange(std::integral_constant<Te, e>) = delete;	// range is invalid or always empty due to value of `e`

template <typename Te, Te e, typename B = std::integral_constant<Te, Te(0)>, typename E = std::integral_constant<Te, e>>
static typename std::enable_if<(Te(0) < e), xrange_extent<Te, B, E>>::type xrange(std::integral_constant<Te, e>)
{
	return {B(), E()};
}
