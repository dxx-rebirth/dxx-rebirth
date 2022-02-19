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

/* In the general case, a range is ephemeral if `T` is an rvalue reference,
 * since an rvalue may be moved or destroyed by the end of the statement.
 */
template <typename T>
std::is_rvalue_reference<T> is_ephemeral_range(...);

/* As a special exception to the general case, if the type T defines an inner
 * type `range_owns_iterated_storage`, delegate the test to that type.  This
 * allows a type T to declare that it is only a view on longer-lived storage,
 * and therefore the storage is not ephemeral even when `T` is an rvalue
 * reference.
 */
template <typename T>
typename std::remove_reference<T>::type::range_owns_iterated_storage is_ephemeral_range(std::nullptr_t);

}

/* Define a convenience helper that applies the `is_ephemeral_range` test to
 * any number of types.
 */
template <typename... Tn>
using any_ephemeral_range = std::disjunction<decltype(detail::is_ephemeral_range<Tn>(nullptr))...>;
