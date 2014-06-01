/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
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
