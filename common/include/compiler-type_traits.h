/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
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
