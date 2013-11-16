/* No include guard - this file is safe to reuse */

#if defined(DXX_WANT_ARRAY) && !defined(DXX_INCLUDED_ARRAY)
#define DXX_INCLUDED_ARRAY
#ifdef DXX_HAVE_CXX_ARRAY
#include <array>
using std::array;
#endif

#ifdef DXX_HAVE_CXX_TR1_ARRAY
#include <tr1/array>
using std::tr1::array;
#endif

#ifdef DXX_HAVE_BOOST_ARRAY
#include <boost/array.hpp>
using boost::array;
#endif
#endif

#if defined(DXX_WANT_LENGTHOF) && !defined(DXX_INCLUDED_LENGTHOF)
#define DXX_INCLUDED_LENGTHOF
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
#endif
