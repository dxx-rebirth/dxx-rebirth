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
