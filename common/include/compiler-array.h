#pragma once

#if defined(DXX_HAVE_CXX_ARRAY)
#include <array>
using std::array;
#elif defined(DXX_HAVE_CXX_TR1_ARRAY)
#include <tr1/array>
using std::tr1::array;
#elif defined(DXX_HAVE_BOOST_ARRAY)
#include <boost/array.hpp>
using boost::array;
#else
#error "No <array> implementation found."
#endif
