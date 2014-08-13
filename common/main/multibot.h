/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license below.
 * See COPYING.txt for license details.

THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Header file for multiplayer robot support.
 *
 */


#ifndef _MULTIBOT_H
#define _MULTIBOT_H

#include "pstypes.h"

#ifdef __cplusplus

struct object;
struct vms_vector;

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
#if defined(DXX_BUILD_DESCENT_I)
#define MAX_ROBOTS_CONTROLLED 3
#elif defined(DXX_BUILD_DESCENT_II)
#define MAX_ROBOTS_CONTROLLED 5
#endif

extern int robot_controlled[MAX_ROBOTS_CONTROLLED];
extern int robot_agitation[MAX_ROBOTS_CONTROLLED];
extern int robot_fired[MAX_ROBOTS_CONTROLLED];
#endif

int multi_can_move_robot(objptridx_t objnum, int agitation);
void multi_send_robot_position(objptridx_t objnum, int fired);
void multi_send_robot_fire(objptridx_t objnum, int gun_num, vms_vector *fire);
void multi_send_claim_robot(objptridx_t objnum);
void multi_send_robot_explode(objptridx_t objnum, int killer, char unused);
void multi_send_create_robot(int robotcen, int objnum, int type);
void multi_send_boss_actions(objptridx_t bossobjnum, int action, int secondary, int objnum);
int multi_send_robot_frame(int sent);

void multi_do_robot_explode(const ubyte *buf);
void multi_do_robot_position(unsigned pnum, const ubyte *buf);
void multi_do_claim_robot(unsigned pnum, const ubyte *buf);
void multi_do_release_robot(unsigned pnum, const ubyte *buf);
void multi_do_robot_fire(const ubyte *buf);
void multi_do_create_robot(unsigned pnum, const ubyte *buf);
void multi_do_boss_actions(unsigned pnum, const ubyte *buf);
void multi_do_create_robot_powerups(unsigned pnum, const ubyte *buf);

int multi_explode_robot_sub(objptridx_t botnum, char unused);

void multi_drop_robot_powerups(objptridx_t objnum);

void multi_strip_robots(int playernum);
void multi_check_robot_timeout(void);

void multi_robot_request_change(objptridx_t robot, int playernum);

#endif

#endif /* _MULTIBOT_H */
