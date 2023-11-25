/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include <iterator>
#include "dxxsconf.h"
#include "backports-ranges.h"
#include <tuple>
#include <type_traits>

namespace d_enumerate {

namespace detail {

/* In the general case, `value_type` is a tuple of the index and the
 * result of the underlying sequence.
 *
 * In the special case that `result_type` is a tuple, construct a
 * modified tuple instead of using a tuple of (index, inner tuple).
 * This allows simpler structured bindings.
 */
template <typename index_type, typename result_type>
struct adjust_iterator_dereference_type : std::false_type
{
	using value_type = std::tuple<index_type, result_type>;
};

template <typename index_type, typename... T>
struct adjust_iterator_dereference_type<index_type, std::tuple<T...>> : std::true_type
{
	using value_type = std::tuple<index_type, T...>;
};

/* Retrieve the member typedef `index_type` if one exists.  Otherwise,
 * use `std::size_t` as a fallback.
 */
std::size_t array_index_type(...);

/* Add, then remove, a reference to the `index_type`.  If `index_type`
 * is an integer or enum type, this produces `index_type` again.  If
 * `index_type` is `void`, then this overload refers to
 * `std::remove_reference<void &>::type`, which is ill-formed, and the
 * overload is removed by SFINAE.  This allows target types to use
 * `using index_type = void` to decline to specify an `index_type`,
 * which may be convenient in templates that conditionally define an
 * `index_type` based on their template parameters.
 */
template <typename T>
typename std::remove_reference<typename T::index_type &>::type array_index_type(T *);

}

}

template <typename sentinel_type>
class enumerated_sentinel
{
public:
	sentinel_type m_sentinel;
	constexpr enumerated_sentinel() = default;
	constexpr enumerated_sentinel(sentinel_type &&iter) :
		m_sentinel(std::move(iter))
	{
	}
};

template <typename range_index_type, typename range_iterator_type, typename sentinel_type, typename adjust_iterator_dereference_type>
class enumerated_iterator
{
	range_iterator_type m_iter;
	range_index_type m_idx;
public:
	using index_type = range_index_type;
	using iterator_category = typename std::iterator_traits<range_iterator_type>::iterator_category;
	using value_type = typename adjust_iterator_dereference_type::value_type;
	using difference_type = std::ptrdiff_t;
	using pointer = value_type *;
	using reference = value_type &;
	constexpr enumerated_iterator(range_iterator_type &&iter, index_type idx) :
		m_iter{std::move(iter)}, m_idx{std::move(idx)}
	{
	}
	value_type operator*() const
	{
		if constexpr (adjust_iterator_dereference_type::value)
			return std::tuple_cat(std::tuple<index_type>(m_idx), *m_iter);
		else
			return {m_idx, *m_iter};
	}
	enumerated_iterator &operator++()
	{
		++ m_iter;
		if constexpr (std::is_enum<index_type>::value)
			/* An enum type is not required to have operator++()
			 * defined, and may deliberately omit it to avoid allowing
			 * the value to be incremented in the typical use case.  For
			 * this situation, an increment is desired, so emulate
			 * operator++() by casting to the underlying integer
			 * type, adding 1, and casting back.
			 */
			m_idx = static_cast<index_type>(1u + static_cast<typename std::underlying_type<index_type>::type>(m_idx));
		else
			++ m_idx;
		return *this;
	}
	enumerated_iterator operator++(int)
	{
		auto result = *this;
		++ * this;
		return result;
	}
	enumerated_iterator &operator--()
		requires(std::bidirectional_iterator<range_iterator_type>)
	{
		-- m_iter;
		if constexpr (std::is_enum<index_type>::value)
			m_idx = static_cast<index_type>(static_cast<typename std::underlying_type<index_type>::type>(m_idx) - 1u);
		else
			-- m_idx;
		return *this;
	}
	enumerated_iterator operator--(int)
		requires(std::bidirectional_iterator<range_iterator_type>)
	{
		auto result = *this;
		-- * this;
		return result;
	}
	constexpr bool operator==(const enumerated_sentinel<sentinel_type> &i) const
	{
		return m_iter == i.m_sentinel;
	}
};

template <typename range_iterator_type, typename range_sentinel_type, typename range_index_type>
class enumerate : ranges::subrange<range_iterator_type, range_sentinel_type>
{
	using base_type = ranges::subrange<range_iterator_type, range_sentinel_type>;
	using iterator_dereference_type = decltype(*std::declval<range_iterator_type>());
	using enumerated_iterator_type = enumerated_iterator<
		range_index_type,
		range_iterator_type,
		range_sentinel_type,
		d_enumerate::detail::adjust_iterator_dereference_type<range_index_type, typename std::remove_cv<iterator_dereference_type>::type>>;
	[[no_unique_address]]
	range_index_type m_idx;
public:
	using base_type::size;
	using index_type = range_index_type;
	template <typename range_type>
		/* Block using `enumerate` on an ephemeral range, since the storage
		 * owned by the range must exist until the `enumerate` object is
		 * fully consumed.  If `range_type &&` is an ephemeral range, then its
		 * storage may cease to exist after this constructor returns.
		 */
		requires(ranges::borrowed_range<range_type>)
		enumerate(range_type &&t, index_type i = index_type{}) :
			base_type{std::forward<range_type>(t)}, m_idx{std::move(i)}
	{
		static_assert(std::is_rvalue_reference<range_type &&>::value || !std::is_rvalue_reference<iterator_dereference_type>::value, "lvalue range must not produce rvalue reference enumerated_value");
	}
	enumerated_iterator_type begin() const
	{
		return {this->base_type::begin(), m_idx};
	}
	enumerated_sentinel<range_sentinel_type> end() const
	{
		return {this->base_type::end()};
	}
};

template <typename range_iterator_type, typename range_sentinel_type, typename index_type>
inline constexpr bool std::ranges::enable_borrowed_range<enumerate<range_iterator_type, range_sentinel_type, index_type>> = true;

template <typename range_type, typename index_type = decltype(d_enumerate::detail::array_index_type(static_cast<typename std::remove_reference<range_type>::type *>(nullptr)))>
requires(ranges::borrowed_range<range_type>)
enumerate(range_type &&r, index_type start = {/* value ignored for deduction guide */}) -> enumerate</* range_iterator_type = */ decltype(std::ranges::begin(std::declval<range_type &>())), /* range_sentinel_type = */ decltype(std::ranges::end(std::declval<range_type &>())), index_type>;
