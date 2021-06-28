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

/*
 * This could have been done using a std::pair, but using a custom type
 * allows for more descriptive member names.
 */
template <typename range_value_type, typename index_type>
struct enumerated_value
{
	const index_type idx;
	range_value_type value;
};

namespace d_enumerate {

namespace detail {

template <typename result_type, typename index_type>
struct adjust_iterator_dereference_type : std::false_type
{
	using value_type = enumerated_value<result_type, index_type>;
};

template <typename... T, typename index_type>
struct adjust_iterator_dereference_type<std::tuple<T...>, index_type> : std::true_type
{
	using value_type = std::tuple<index_type, T...>;
};

}

}

template <typename range_iterator_type, typename index_type, typename iterator_dereference_type>
class enumerated_iterator
{
	range_iterator_type m_iter;
	index_type m_idx;
	using adjust_iterator_dereference_type = d_enumerate::detail::adjust_iterator_dereference_type<typename std::remove_cv<iterator_dereference_type>::type, index_type>;
public:
	using iterator_category = std::forward_iterator_tag;
	using value_type = typename adjust_iterator_dereference_type::value_type;
	using difference_type = std::ptrdiff_t;
	using pointer = value_type *;
	using reference = value_type &;
	enumerated_iterator(const range_iterator_type &iter, const index_type idx) :
		m_iter(iter), m_idx(idx)
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

template <typename range_iterator_type, typename index_type>
class enumerate : partial_range_t<range_iterator_type>
{
	using base_type = partial_range_t<range_iterator_type>;
	using iterator_dereference_type = decltype(*std::declval<range_iterator_type>());
	using enumerated_iterator_type = enumerated_iterator<
		range_iterator_type,
		index_type,
		iterator_dereference_type>;
	const index_type m_idx;
public:
	using range_owns_iterated_storage = std::false_type;
	enumerate(const range_iterator_type b, const range_iterator_type e, const index_type i) :
		base_type(b, e), m_idx(i)
	{
	}
	template <typename range_type>
		enumerate(range_type &&t, const index_type i = 0) :
			base_type(t), m_idx(i)
	{
		static_assert(!any_ephemeral_range<range_type &&>::value, "cannot enumerate storage of ephemeral ranges");
		static_assert(std::is_rvalue_reference<range_type &&>::value || std::is_lvalue_reference<iterator_dereference_type>::value, "lvalue range must produce lvalue reference enumerated_value");
	}
	enumerated_iterator_type begin() const
	{
		return enumerated_iterator_type(this->base_type::begin(), m_idx);
	}
	enumerated_iterator_type end() const
	{
		return enumerated_iterator_type(this->base_type::end(), 0 /* unused */);
	}
};

template <typename range_type, typename index_type = uint_fast32_t, typename range_iterator_type = decltype(std::begin(std::declval<range_type &>()))>
enumerate(range_type &&r, const index_type start = index_type(/* value ignored */)) -> enumerate<range_iterator_type, index_type>;
