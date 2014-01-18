#pragma once

#if defined(DXX_HAVE_CXX11_BEGIN)
#include <iterator>
using std::begin;
using std::end;
#elif defined(DXX_HAVE_BOOST_BEGIN)
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
using boost::begin;
using boost::end;
#else
#error "No begin()/end() implementation found."
#endif
