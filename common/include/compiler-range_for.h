/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#if defined(DXX_HAVE_CXX11_RANGE_FOR)
#define range_for(A,B)	for(A:B)
#elif defined(DXX_HAVE_BOOST_FOREACH)
#include <boost/foreach.hpp>
#define range_for	BOOST_FOREACH
#else
#error "No range-based for implementation found."
#endif
