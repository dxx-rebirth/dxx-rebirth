/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

/*
 *
 * Header for timer functions
 *
 */

#pragma once

#include "maths.h"

#ifdef __cplusplus
namespace dcx {

fix64 timer_update();

[[nodiscard]]
fix64 timer_query();

void timer_delay_ms(unsigned milliseconds);

}
#endif
