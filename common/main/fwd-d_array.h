/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

#pragma once
#include <cstddef>
#include <type_traits>

namespace dcx {

template <typename T, std::size_t N, typename E>
requires(!std::is_same<unsigned, E>::value && !std::is_same<unsigned long, E>::value)
struct enumerated_array;

}
