/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once
#include <cstddef>
#include <type_traits>

namespace detail {

template <typename T>
std::is_rvalue_reference<T> is_ephemeral_range(...);

template <typename T>
typename std::remove_reference<T>::type::range_owns_iterated_storage is_ephemeral_range(std::nullptr_t);

}

template <typename...>
struct any_ephemeral_range : std::false_type
{
};

template <typename T1>
struct any_ephemeral_range<T1> : decltype(detail::is_ephemeral_range<T1>(nullptr))
{
};

template <typename T1, typename... Tn>
struct any_ephemeral_range<T1, Tn...> : std::conditional<any_ephemeral_range<T1>::value, std::true_type, any_ephemeral_range<Tn...>>::type
{
};
