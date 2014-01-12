#pragma once

#if defined(DXX_HAVE_CXX11_ADDRESSOF)
#include <utility>
using std::addressof;
#elif defined(DXX_HAVE_BOOST_ADDRESSOF)
#include <boost/utility.hpp>
using boost::addressof;
#else
#error "No addressof() implementation found."
#endif
