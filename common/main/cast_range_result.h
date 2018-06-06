/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include <iterator>
#include <utility>
#include "partial_range.h"

template <typename result_type, typename iterator_type>
class iterator_result_converter : public iterator_type
{
public:
	iterator_result_converter(iterator_type &&iter) :
		iterator_type(std::move(iter))
	{
	}
	iterator_type base() const
	{
		return *this;
	}
	result_type operator*() const
	{
		return this->iterator_type::operator*();
	}
	iterator_result_converter &operator++()
	{
		return static_cast<iterator_result_converter &>(this->iterator_type::operator++());
	}
};

template <typename result_type, typename range_type, typename iterator_type = iterator_result_converter<result_type, decltype(std::begin(std::declval<range_type &>()))>>
partial_range_t<iterator_type> cast_range_result(range_type &range)
{
	return range;
}
