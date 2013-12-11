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

#if defined(DXX_WANT_STATIC_ASSERT) && !defined(DXX_INCLUDED_STATIC_ASSERT)
#define DXX_INCLUDED_STATIC_ASSERT
#if defined(DXX_HAVE_BOOST_STATIC_ASSERT)
#include <boost/static_assert.hpp>
#define static_assert(C,M)	BOOST_STATIC_ASSERT_MSG((C),M)
#elif defined(DXX_HAVE_C_TYPEDEF_STATIC_ASSERT)
#define static_assert(C,M)	typedef int static_assertion_check[(C) ? 1 : -1];
#endif
#endif

#if defined(DXX_WANT_RANGE_FOR) && !defined(DXX_INCLUDED_RANGE_FOR)
#define DXX_INCLUDED_RANGE_FOR
#if defined(DXX_HAVE_CXX11_RANGE_FOR)
#define range_for(A,B)	for(A:B)
#elif defined(DXX_HAVE_BOOST_FOREACH)
#include <boost/foreach.hpp>
#define range_for	BOOST_FOREACH
#endif
#endif
