/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include <cstdint>
#include <cstddef>
#include "d_array.h"
#include "dsx-ns.h"

namespace dcx {
enum class vclip_index : uint8_t;
struct vclip;
extern unsigned Num_vclips;

vclip_index build_vclip_index_from_untrusted(std::size_t i);
}

#ifdef dsx
namespace dsx {
#if DXX_BUILD_DESCENT == 1
#define VCLIP_MAXNUM	70
#elif defined(DXX_BUILD_DESCENT_II)
#define VCLIP_MAXNUM	110
#endif

using d_vclip_array = enumerated_array<vclip, VCLIP_MAXNUM, vclip_index>;
extern d_vclip_array Vclip;
#undef VCLIP_MAXNUM
}
#endif
