/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include <array>
#include <type_traits>

#include "dxxsconf.h"
#include "dsx-ns.h"

using std::array;

namespace dcx {
struct vclip;
extern unsigned Num_vclips;
}

#ifdef dsx
namespace dsx {
#if defined(DXX_BUILD_DESCENT_I)
#define VCLIP_MAXNUM	70
#elif defined(DXX_BUILD_DESCENT_II)
#define VCLIP_MAXNUM	110
#endif

using d_vclip_array = array<vclip, VCLIP_MAXNUM>;
extern d_vclip_array Vclip;
#undef VCLIP_MAXNUM
}
#endif
