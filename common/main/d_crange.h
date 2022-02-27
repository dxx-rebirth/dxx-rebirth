/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include "d_range.h"

template <typename index_type, index_type begin_value, index_type end_value>
using constant_xrange = xrange<index_type, std::integral_constant<index_type, begin_value>, std::integral_constant<index_type, end_value>>;
