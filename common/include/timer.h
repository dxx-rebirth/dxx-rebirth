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


#ifndef _TIMER_H
#define _TIMER_H

#include "pstypes.h"
#include "maths.h"

#ifdef __cplusplus

void timer_update();
fix64 timer_query();
void timer_delay(fix seconds);
void timer_delay2(int fps);

#endif

#endif
