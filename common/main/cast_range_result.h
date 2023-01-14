/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include <iterator>
#include <utility>
#include "partial_range.h"

template <typename result_type, typename iterator_type>
/* Declare the requirements on the class itself, rather than the one method which needs them, because:
 * - the class is useless if that one method fails its requirements
 * - gcc produces better error messages if the class is rejected, than if the
 *   class is accepted and `operator*()` is rejected.
 */
requires(
	requires(iterator_type i) {
		/* If the base type's result cannot be converted to this type's result,
		 * then `operator*()` will raise an error when it attempts an implicit
		 * conversion.
		 */
		{*i} -> std::convertible_to<result_type>;
		/* If `result_type` and the base type's `operator*` are the same, then
		 * this adapter has no effect.
		 */
		requires(!std::is_same<result_type, decltype(*i)>::value);
	}
)
class iterator_result_converter : public iterator_type
{
public:
	constexpr iterator_result_converter() = default;
	iterator_result_converter(iterator_type &&iter) :
		iterator_type(std::move(iter))
	{
	}
	iterator_type base() const
	{
		return *this;
	}
	/* `using iterator_type::operator*()` is not viable here because this
	 * `operator*()` has a different return type than
	 * `iterator_type::operator*()`.
	 */
	result_type operator*() const
	{
		return this->iterator_type::operator*();
	}
	iterator_result_converter &operator++()
	{
		return static_cast<iterator_result_converter &>(this->iterator_type::operator++());
	}
	iterator_result_converter operator++(int)
	{
		auto r = *this;
		this->iterator_type::operator++();
		return r;
	}
};

template <typename result_type, typename range_type, typename iterator_type = iterator_result_converter<result_type, decltype(std::begin(std::declval<range_type &>()))>>
static inline partial_range_t<iterator_type> cast_range_result(range_type &range)
{
	return range;
}
