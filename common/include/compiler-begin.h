/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
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
