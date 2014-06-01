/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 *
 * Header for escort.c
 *
 */

#ifndef _ESCORT_H
#define _ESCORT_H

#include "maths.h"

#ifdef __cplusplus
struct objptridx_t;

#if defined(DXX_BUILD_DESCENT_I)
static inline void invalidate_escort_goal(void)
{
}
static inline void detect_escort_goal_accomplished(const objptridx_t &)
{
}
#elif defined(DXX_BUILD_DESCENT_II)
#define GUIDEBOT_NAME_LEN 9
struct object;
extern void change_guidebot_name(void);
extern void do_escort_menu(void);
void detect_escort_goal_accomplished(objptridx_t index);
void detect_escort_goal_fuelcen_accomplished();
extern void set_escort_special_goal(int key);
void recreate_thief(struct object *objp);
void init_buddy_for_level(void);
void invalidate_escort_goal(void);
void drop_stolen_items (struct object *);
extern fix64	Buddy_sorry_time;
#endif

#endif

#endif // _ESCORT_H
