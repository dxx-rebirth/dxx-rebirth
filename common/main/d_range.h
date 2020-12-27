/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include <type_traits>
#include <iterator>
#include <utility>

namespace detail {

template <typename T>
struct xrange_is_unsigned : std::is_unsigned<T>
{
	/* For the general case, delegate to std::is_unsigned.
	 * xrange requires that the type not be a reference, so there is no
	 * need to use std::remove_reference<T> here.
	 */
};

template <typename T, T v>
struct xrange_is_unsigned<std::integral_constant<T, v>> : std::is_unsigned<T>
{
	/* For the special case that the value is a std::integral_constant,
	 * examine the underlying integer type.
	 * std::is_unsigned<std::integral_constant<unsigned, N>> is
	 * std::false_type, but xrange_is_unsigned should yield
	 * std::true_type in that case.
	 */
};

template <typename B, typename E>
struct xrange_check_constant_endpoints : std::true_type
{
	/* By default, endpoints are not constant and are not checked. */
};

template <typename Tb, Tb b, typename Te, Te e>
struct xrange_check_constant_endpoints<std::integral_constant<Tb, b>, std::integral_constant<Te, e>> : std::true_type
{
	/* If both endpoints are constant, a static_assert can validate that
	 * the range is not empty.
	 */
	static_assert(b < e, "range is always empty due to value of `b` versus value of `e`");
};

}

/* For the general case, store a `const`-qualified copy of the value,
 * and provide an implicit conversion.
 */
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

/* For the special case that the value is a std::integral_constant,
 * inherit from it so that the Empty Base Optimization can apply.
 */
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
	using difference_type = std::ptrdiff_t;
	using iterator_category = std::forward_iterator_tag;
	using value_type = index_type;
	using pointer = value_type *;
	using reference = value_type &;
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
class xrange :
	public xrange_endpoint<B, true>,
	public xrange_endpoint<E, false>
{
	using begin_type = xrange_endpoint<B, true>;
	using end_type = xrange_endpoint<E, false>;
	using iterator = xrange_iterator<index_type>;
	/* This static_assert has no message, since the value is always
	 * true.  Use of static_assert forces instantiation of the type,
	 * which has a static_assert that checks the values and displays a
	 * message on failure.
	 */
	static_assert(detail::xrange_check_constant_endpoints<B, E>::value);
	static_assert(!std::is_reference<E>::value, "xrange<E> must be a value, not a reference");
	static begin_type init_begin(B b, E e)
	{
		if constexpr (std::is_convertible<E, B>::value)
		{
#ifdef DXX_CONSTANT_TRUE
			(DXX_CONSTANT_TRUE(!(b < e)) && (DXX_ALWAYS_ERROR_FUNCTION(xrange_is_always_empty, "begin never less than end"), 0));
#endif
			if (!(b < e))
				return e;
		}
		else
			(void)e;
		return b;
	}
public:
	using range_owns_iterated_storage = std::false_type;
	xrange(B b, E e) :
		begin_type(init_begin(std::move(b), e)), end_type(std::move(e))
	{
	}
	xrange(E e) :
		begin_type(), end_type(e)
	{
		static_assert(detail::xrange_is_unsigned<E>::value, "xrange(E) requires unsigned E; use xrange(B, E) if E must be signed");
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
template <typename Tb, typename Te, typename std::enable_if<!std::is_const<Te>::value, int>::type = 0>
xrange(Tb &&b, Te &e) -> xrange<Tb, Tb, Te &>;	// provokes a static_assert failure in the constructor

template <typename Tb, typename Te>
xrange(Tb &&b, Te &&e) -> xrange<
	typename std::common_type<Tb, Te>::type,
	typename std::remove_const<typename std::remove_reference<Tb>::type>::type,
	typename std::remove_const<typename std::remove_reference<Te>::type>::type
	>;

template <typename Te>
xrange(const Te &) -> xrange<Te, std::integral_constant<typename std::common_type<Te, unsigned>::type, 0u>>;

template <typename Te>
xrange(Te &) -> xrange<
	Te,
	std::integral_constant<typename std::common_type<typename std::remove_const<Te>::type, unsigned>::type, 0u>,
	Te &
	>;

template <typename Te, Te e>
xrange(std::integral_constant<Te, e>) -> xrange<Te, std::integral_constant<Te, Te(0)>, std::integral_constant<Te, e>>;
