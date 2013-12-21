#pragma once

#if defined(DXX_HAVE_CXX11_TYPE_TRAITS)
#include <type_traits>
namespace tt = std;
#elif defined(DXX_HAVE_BOOST_TYPE_TRAITS)
#include <boost/type_traits.hpp>
namespace tt = boost;
#else
#error "No <type_traits> implementation found."
#endif
