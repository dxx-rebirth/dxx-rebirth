/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#if defined(DXX_HAVE_CXX11_ADDRESSOF)
#include <memory>
using std::addressof;
#elif defined(DXX_HAVE_BOOST_ADDRESSOF)
#include <boost/utility.hpp>
using boost::addressof;
#else
#error "No addressof() implementation found."
#endif
