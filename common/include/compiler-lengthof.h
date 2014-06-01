/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#ifdef DXX_HAVE_CXX11_FUNCTION_AUTO
#include <cstddef>

#ifdef DXX_HAVE_CXX11_EXPLICIT_DELETE
template <typename T>
static void array_identity(T) = delete;
#endif

template <typename T, std::size_t N>
static auto array_identity(T (&t)[N]) -> T(&)[N] { return t; }
#define lengthof(A)	(sizeof((array_identity)(A)) / sizeof((A)[0]))

#else
#define lengthof(A)	(sizeof((A)) / sizeof((A)[0]))
#endif
