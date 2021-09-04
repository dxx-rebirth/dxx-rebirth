/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include <type_traits>

/* Convenience function to use std::underlying_type<T> on a given value,
 * and always pick the right T for the enum being converted.
 */
template <typename T>
static constexpr auto underlying_value(const T e)
{
	if constexpr (std::is_enum<T>::value)
		return static_cast<typename std::underlying_type<T>::type>(e);
	else
	{
		/* This path should never be executed, but has a return
		 * statement because deducing a return type of `void` produces
		 * worse error messages than deducing a passthrough of the
		 * original value.  Although this function can work correctly on
		 * a non-enum, it is unnecessary there, so any such use likely
		 * indicates a bug.
		 */
		static_assert(std::is_enum<T>::value);
		return e;
	}
}
