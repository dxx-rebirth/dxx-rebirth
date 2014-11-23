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
 * Header for collide.c
 *
 */


#ifndef _COLLIDE_H
#define _COLLIDE_H

#include "playsave.h"

#ifdef __cplusplus

void collide_two_objects(vobjptridx_t A, vobjptridx_t B, vms_vector &collision_point);
void collide_object_with_wall(vobjptridx_t A, fix hitspeed, vsegptridx_t hitseg, short hitwall, const vms_vector &hitpt);
void apply_damage_to_player(vobjptr_t player, cobjptridx_t killer, fix damage, ubyte possibly_friendly);

// Returns 1 if robot died, else 0.
int apply_damage_to_robot(vobjptridx_t robot, fix damage, objnum_t killer_objnum);

#define PERSISTENT_DEBRIS (PlayerCfg.PersistentDebris && !(Game_mode & GM_MULTI)) // no persistent debris in multi

void collide_player_and_materialization_center(vobjptridx_t objp);
void collide_robot_and_materialization_center(vobjptridx_t objp);

void scrape_player_on_wall(vobjptridx_t obj, vsegptridx_t hitseg, short hitwall, const vms_vector &hitpt);
int maybe_detonate_weapon(vobjptridx_t obj0p, vobjptr_t obj, const vms_vector &pos);

void collide_player_and_nasty_robot(vobjptridx_t player, vobjptridx_t robot, const vms_vector &collision_point);

void net_destroy_controlcen(objptridx_t controlcen);
void collide_player_and_powerup(vobjptr_t player, vobjptridx_t powerup, const vms_vector &collision_point);
#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
#if defined(DXX_BUILD_DESCENT_I)
typedef cobjptr_t _check_effect_blowup_objptr;
#elif defined(DXX_BUILD_DESCENT_II)
typedef vcobjptr_t _check_effect_blowup_objptr;
#endif
int check_effect_blowup(vsegptridx_t seg,int side,const vms_vector &pnt, _check_effect_blowup_objptr blower, int force_blowup_flag, int remote);
#endif
void apply_damage_to_controlcen(vobjptridx_t controlcen, fix damage, objnum_t who);
void bump_one_object(vobjptr_t obj0, const vms_vector &hit_dir, fix damage);
void drop_player_eggs(vobjptridx_t playerobj);

#if defined(DXX_BUILD_DESCENT_II)
void do_final_boss_frame(void);
void do_final_boss_hacks(void);
int check_volatile_wall(vobjptridx_t obj,vsegptridx_t seg,int sidenum);
extern int	Final_boss_is_dead;
#endif

#endif

#endif /* _COLLIDE_H */
