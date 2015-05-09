/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
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

fix64 timer_update();
__attribute_warn_unused_result
fix64 timer_query();
void timer_delay(fix seconds);
void timer_delay_bound(unsigned bound);
static inline void timer_delay2(int fps)
{
	timer_delay_bound(1000u / fps);
}

#endif
