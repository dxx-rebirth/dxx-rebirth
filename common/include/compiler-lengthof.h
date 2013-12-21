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
