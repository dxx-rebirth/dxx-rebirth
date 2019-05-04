/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include <inttypes.h>
#include "dxxsconf.h"
#include "partial_range.h"
#include "ephemeral_range.h"

/*
 * This could have been done using a std::pair, but using a custom type
 * allows for more descriptive member names.
 */
template <typename range_value_type, typename index_type>
struct enumerated_value
{
	range_value_type value;
	const index_type idx;
};

template <typename range_iterator_type, typename index_type, typename result_type>
class enumerated_iterator
{
	range_iterator_type m_iter;
	index_type m_idx;
public:
	enumerated_iterator(const range_iterator_type &iter, const index_type idx) :
		m_iter(iter), m_idx(idx)
	{
	}
	result_type operator*() const
	{
		return result_type{*m_iter, m_idx};
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
};

template <typename range_iterator_type, typename index_type>
class enumerated_range : partial_range_t<range_iterator_type>
{
	using base_type = partial_range_t<range_iterator_type>;
	using iterator_dereference_type = decltype(*std::declval<range_iterator_type>());
	using enumerated_iterator_type = enumerated_iterator<range_iterator_type, index_type, enumerated_value<iterator_dereference_type, index_type>>;
	const index_type m_idx;
public:
	using range_owns_iterated_storage = std::false_type;
	enumerated_range(const range_iterator_type b, const range_iterator_type e, const index_type i) :
		base_type(b, e), m_idx(i)
	{
	}
	template <typename range_type>
		enumerated_range(range_type &&t, const index_type i) :
			base_type(t), m_idx(i)
	{
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

template <typename range_type, typename index_type = uint_fast32_t, typename range_iterator_type = decltype(begin(std::declval<range_type &>()))>
static auto enumerate(range_type &&r, const index_type start = 0)
{
	static_assert(!any_ephemeral_range<range_type &&>::value, "cannot enumerate storage of ephemeral ranges");
	return enumerated_range<range_iterator_type, index_type>(std::forward<range_type>(r), start);
}
