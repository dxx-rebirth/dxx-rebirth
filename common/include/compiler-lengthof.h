/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include <cstddef>

template <typename T, std::size_t N>
static auto array_identity(T (&t)[N]) -> T(&)[N] { return t; }
#define lengthof(A)	(sizeof((array_identity)(A)) / sizeof((A)[0]))
