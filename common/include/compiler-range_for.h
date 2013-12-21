#pragma once

#if defined(DXX_HAVE_CXX11_RANGE_FOR)
#define range_for(A,B)	for(A:B)
#elif defined(DXX_HAVE_BOOST_FOREACH)
#include <boost/foreach.hpp>
#define range_for	BOOST_FOREACH
#else
#error "No range-based for implementation found."
#endif
