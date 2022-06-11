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
template <typename T, typename R = typename std::enable_if<std::is_enum<T>::value, typename std::underlying_type<T>::type>::type>
static constexpr R underlying_value(const T e)
{
	return static_cast<R>(e);
}

template <typename T>
typename std::enable_if<!std::is_enum<T>::value, T>::type underlying_value(T) = delete;
