/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include <type_traits>
#include <iterator>
#include <ranges>
#include <utility>

/* These could be empty tag types, but defining them from
 * std::integral_constant<bool, V> allows the deduction guide for
 * std::integral_constant<$integer_type, V> to handle them.  If these
 * were done as tag types, a separate deduction guide would be required.
 */
using xrange_ascending = std::true_type;
using xrange_descending = std::false_type;

namespace detail {

template <typename B, typename E, typename step_type>
struct xrange_check_constant_endpoints : std::true_type
{
	/* By default, endpoints are not constant and are not checked. */
};

template <typename Tb, Tb b, typename Te, Te e>
struct xrange_check_constant_endpoints<std::integral_constant<Tb, b>, std::integral_constant<Te, e>, xrange_ascending> : std::true_type
{
	/* If both endpoints are constant, a static_assert can validate that
	 * the range is not empty.
	 */
	static_assert(b < e, "range is always empty due to value of `b` versus value of `e`");
};

template <typename Tb, Tb b, typename Te, Te e>
struct xrange_check_constant_endpoints<std::integral_constant<Tb, b>, std::integral_constant<Te, e>, xrange_descending> : std::true_type
{
	static_assert(e < b, "range is always empty due to value of `b` versus value of `e`");
};

template <typename Tb, Tb b, typename Te, Te e, typename Tstep, Tstep step>
struct xrange_check_constant_endpoints<std::integral_constant<Tb, b>, std::integral_constant<Te, e>, std::integral_constant<Tstep, step>> :
	xrange_check_constant_endpoints<
		std::integral_constant<Tb, b>,
		std::integral_constant<Te, e>,
		typename std::conditional<(step > 0), xrange_ascending, xrange_descending>::type
		>
{
	/* The implementation of xrange_iterator::operator++ moves the
	 * current position by exactly the step value, every time.  If
	 * std::abs(step) is greater than 1, then some values will be
	 * stepped over.  Assert that skipping values will not cause
	 * operator++ to skip over the end value.  If it did, the range
	 * would continue until wraparound terminated it, the program
	 * abandoned reading the range, or the program crashed due to out of
	 * range values.
	 */
	static_assert((step > 0 ? ((e - b) % step) : ((b - e) % -step)) == 0, "step size will overstep end value");
};

}

/* For the general case, store a copy of the value,
 * and provide an implicit conversion.
 */
template <typename T, bool begin>
class xrange_endpoint
{
public:
	using value_type = T;
	/* The value is never mutated, but must be mutable to satisfy
	 * std::ranges::range<xrange<...>>.
	 */
	value_type value{};
	constexpr operator value_type() const
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

template <typename index_type, typename step_type>
class xrange_iterator
{
	index_type m_idx{};
public:
	using difference_type = std::ptrdiff_t;
	using iterator_category = std::forward_iterator_tag;
	using value_type = index_type;
	using pointer = value_type *;
	using reference = value_type &;
	constexpr xrange_iterator() = default;	// default constructible required by std::semiregular
	constexpr xrange_iterator(const index_type i) :
		m_idx(i)
	{
	}
	[[nodiscard]]
	difference_type operator-(const xrange_iterator &i) const
		requires(
			requires(index_type i) {
				i - i;
			}
		)
	{
		return m_idx - i.m_idx;
	}
	[[nodiscard]]
	index_type operator*() const
	{
		return m_idx;
	}
	xrange_iterator &operator++()
		requires(
			std::same_as<step_type, xrange_ascending> &&
			requires(index_type i) {
				++ i;
			}
		)
	{
		++ m_idx;
		return *this;
	}
	xrange_iterator &operator++()
		requires(
			std::same_as<step_type, xrange_descending> &&
			requires(index_type i) {
				-- i;
			}
		)
	{
		-- m_idx;
		return *this;
	}
	xrange_iterator &operator++()
		requires(
			!std::same_as<step_type, xrange_ascending> &&
			!std::same_as<step_type, xrange_descending> &&
			requires(index_type i) {
				i += step_type::value;
			}
		)
	{
		m_idx += step_type::value;
		return *this;
	}
	[[nodiscard]]
	xrange_iterator operator++(int)
		requires(
			requires(xrange_iterator i) {
				++ i;
			}
		)
	{
		auto r = *this;
		++ *this;
		return r;
	}
	[[nodiscard]]
	constexpr bool operator==(const xrange_iterator &i) const = default;
	template <typename end_index_type>
		[[nodiscard]]
		constexpr bool operator==(const xrange_endpoint<end_index_type, false> &i) const
		{
			return m_idx == i.value;
		}
};

/* This provides an approximation of the functionality of the Python2
 * xrange object.  Python3 renamed it to `range`.  The older name is
 * kept because it is easier to find with grep.
 */
template <typename index_type, typename B = index_type, typename E = index_type, typename step_type = xrange_ascending>
class xrange :
	protected xrange_endpoint<B, true>,
	protected xrange_endpoint<E, false>
{
protected:
	using begin_type = xrange_endpoint<B, true>;
	using end_type = xrange_endpoint<E, false>;
	using iterator = xrange_iterator<index_type, step_type>;
	/* This static_assert has no message, since the value is always
	 * true.  Use of static_assert forces instantiation of the type,
	 * which has a static_assert that checks the values and displays a
	 * message on failure.
	 */
	static_assert(detail::xrange_check_constant_endpoints<B, E, step_type>::value);
	static_assert(!std::is_reference<E>::value, "xrange<E> must be a value, not a reference");
	static constexpr B init_begin(B b, E e)
	{
		if constexpr (std::is_convertible<E, B>::value)
		{
			if constexpr (std::is_same<step_type, xrange_ascending>::value || (!std::is_same<step_type, xrange_descending>::value && step_type::value > 0))
			{
#ifdef DXX_CONSTANT_TRUE
				(DXX_CONSTANT_TRUE(!(b < e)) && (DXX_ALWAYS_ERROR_FUNCTION("begin never less than end"), 0));
#endif
				if (!(b < e))
					return e;
			}
			else
			{
#ifdef DXX_CONSTANT_TRUE
				(DXX_CONSTANT_TRUE(!(e < b)) && (DXX_ALWAYS_ERROR_FUNCTION("end never less than begin"), 0));
#endif
				if (!(e < b))
					return e;
			}
		}
		else
			(void)e;
		return b;
	}
private:
	/* gcc-15 rejects `using end_type::operator typename end_type::value_type;`
	 * Use `using end_type::operator end_value_type` with a local type as an
	 * indirection in, rather than looking up the type from the base.
	 *
	 * - compiles in gcc-12, gcc-13, gcc-14
	 * - compiles in clang-15, clang-16, clang-17
	 * - fails to compile in gcc-15
	 */
	using end_value_type = typename end_type::value_type;
public:
	using end_type::value;
	using end_type::operator end_value_type;
	constexpr xrange() = default;
	constexpr xrange(B b, E e) :
		begin_type{init_begin(std::move(b), e)}, end_type{std::move(e)}
	{
	}
	/* Allow, but ignore, a third argument with the step size.  The
	 * argument must be here for class template argument deduction to
	 * work.
	 */
	constexpr xrange(B b, E e, step_type) :
		xrange{std::move(b), std::move(e)}
	{
	}
	constexpr xrange(E e) :
		begin_type{}, end_type{std::move(e)}
	{
		static_assert(
			/* For the general case, delegate to `std::is_unsigned_v`.
			 * xrange requires that the type not be a reference, so there is no
			 * need to use `std::remove_reference<E>` here.
			 */
			requires { requires std::is_unsigned_v<E>; } or
			(
				/* For the special case that the value is a
				 * `std::integral_constant`, examine the underlying integer
				 * type.
				 * `std::is_unsigned_v<std::integral_constant<unsigned, N>>` is
				 * false, but this requires expression should yield true in
				 * that case.
				 */
				requires { E::value; typename E::value_type; } and
				requires { requires std::is_same_v<std::integral_constant<typename E::value_type, E::value>, E>; } and
				requires { requires std::is_unsigned_v<typename E::value_type>; }
			), "xrange(E) requires unsigned E; use xrange(B, E) if E must be signed");
	}
	iterator begin() const
	{
		return iterator(static_cast<const begin_type &>(*this));
	}
	end_type end() const
	{
		return *this;
	}
};

template <typename index_type, typename B, typename E, typename step_type>
inline constexpr bool std::ranges::enable_borrowed_range<xrange<index_type, B, E, step_type>> = true;

/* Disallow building an `xrange` with a reference to mutable `e` as the
 * end term.  When `e` is mutable, the loop might change `e` during
 * iteration, so using `xrange` could be wrong.  If the loop does not
 * change `e`, store it in a const qualified variable, which will select
 * the next overload down instead.
 */
template <typename Tb, typename Te>
requires(!std::is_const<Te>::value)
xrange(Tb &&b, Te &e) -> xrange<Tb, Tb, Te &>;	// provokes a static_assert failure in the constructor

template <typename Tb, typename Te>
xrange(Tb &&b, Te &&e) -> xrange<
	typename std::common_type<Tb, Te>::type,
	typename std::remove_const<typename std::remove_reference<Tb>::type>::type,
	typename std::remove_const<typename std::remove_reference<Te>::type>::type
	>;

template <typename Tb, typename Te, typename Tstep, Tstep step>
xrange(Tb &&b, Te &&e, std::integral_constant<Tstep, step>) -> xrange<
	typename std::common_type<Tb, Te>::type,
	typename std::remove_const<typename std::remove_reference<Tb>::type>::type,
	typename std::remove_const<typename std::remove_reference<Te>::type>::type,
	std::integral_constant<Tstep, step>
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
