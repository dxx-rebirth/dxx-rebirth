/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#if defined(DXX_HAVE_CXX11_STATIC_ASSERT)
/* native */
#elif defined(DXX_HAVE_BOOST_STATIC_ASSERT)
#include <boost/static_assert.hpp>
#define static_assert(C,M)	BOOST_STATIC_ASSERT_MSG((C),M)
#elif defined(DXX_HAVE_C_TYPEDEF_STATIC_ASSERT)
#define static_assert(C,M)	typedef int static_assertion_check[(C) ? 1 : -1];
#else
#error "No static_assert implementation found."
#endif
