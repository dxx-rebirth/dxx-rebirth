/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include <inttypes.h>
#include <iterator>
#include "dxxsconf.h"
#include "partial_range.h"
#include "ephemeral_range.h"
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
 * use `uint_fast32_t` as a fallback.
 */
template <typename>
uint_fast32_t array_index_type(...);

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
typename std::remove_reference<typename std::remove_reference<T>::type::index_type &>::type array_index_type(std::nullptr_t);

}

}

template <typename range_index_type, typename range_iterator_type, typename adjust_iterator_dereference_type>
class enumerated_iterator
{
	range_iterator_type m_iter;
	range_index_type m_idx;
public:
	using index_type = range_index_type;
	using iterator_category = std::forward_iterator_tag;
	using value_type = typename adjust_iterator_dereference_type::value_type;
	using difference_type = std::ptrdiff_t;
	using pointer = value_type *;
	using reference = value_type &;
	enumerated_iterator(range_iterator_type &&iter, const index_type idx) :
		m_iter(std::move(iter)), m_idx(idx)
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
	bool operator!=(const enumerated_iterator &i) const
	{
		return m_iter != i.m_iter;
	}
	bool operator==(const enumerated_iterator &i) const
	{
		return m_iter == i.m_iter;
	}
};

template <typename range_iterator_type, typename range_index_type>
class enumerate : partial_range_t<range_iterator_type, range_index_type>
{
	using base_type = partial_range_t<range_iterator_type, range_index_type>;
	using iterator_dereference_type = decltype(*std::declval<range_iterator_type>());
	using enumerated_iterator_type = enumerated_iterator<
		range_index_type,
		range_iterator_type,
		d_enumerate::detail::adjust_iterator_dereference_type<range_index_type, typename std::remove_cv<iterator_dereference_type>::type>>;
	const range_index_type m_idx;
public:
	using range_owns_iterated_storage = std::false_type;
	using typename base_type::index_type;
	enumerate(const range_iterator_type b, const range_iterator_type e, const index_type i) :
		base_type(b, e), m_idx(i)
	{
	}
	template <typename range_type>
		enumerate(range_type &&t, const index_type i = index_type{}) :
			base_type(std::forward<range_type>(t)), m_idx(i)
	{
		/* Block using `enumerate` on an ephemeral range, since the storage
		 * owned by the range must exist until the `enumerate` object is
		 * fully consumed.  If `range_type &&` is an ephemeral range, then its
		 * storage may cease to exist after this constructor returns.
		 */
		static_assert(!any_ephemeral_range<range_type &&>::value, "cannot enumerate storage of ephemeral ranges");
		static_assert(std::is_rvalue_reference<range_type &&>::value || !std::is_rvalue_reference<iterator_dereference_type>::value, "lvalue range must not produce rvalue reference enumerated_value");
	}
	enumerated_iterator_type begin() const
	{
		return {this->base_type::begin(), m_idx};
	}
	enumerated_iterator_type end() const
	{
		return {this->base_type::end(), index_type{} /* unused */};
	}
};

template <typename range_type, typename index_type = decltype(d_enumerate::detail::array_index_type<range_type>(nullptr)), typename range_iterator_type = decltype(std::begin(std::declval<range_type &>()))>
enumerate(range_type &&r, const index_type start = index_type(/* value ignored */)) -> enumerate<range_iterator_type, index_type>;
