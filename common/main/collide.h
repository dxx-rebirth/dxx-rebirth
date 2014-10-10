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
void collide_object_with_wall(vobjptridx_t A, fix hitspeed, segnum_t hitseg, short hitwall, vms_vector * hitpt);
void apply_damage_to_player(object *player, objptridx_t killer, fix damage, ubyte possibly_friendly);

// Returns 1 if robot died, else 0.
int apply_damage_to_robot(vobjptridx_t robot, fix damage, objnum_t killer_objnum);

#define PERSISTENT_DEBRIS (PlayerCfg.PersistentDebris && !(Game_mode & GM_MULTI)) // no persistent debris in multi

extern void collide_player_and_materialization_center(object *objp);
void collide_robot_and_materialization_center(vobjptridx_t objp);

void scrape_player_on_wall(vobjptridx_t obj, segnum_t hitseg, short hitwall, vms_vector * hitpt);
int maybe_detonate_weapon(vobjptridx_t obj0p, object *obj, vms_vector *pos);

void collide_player_and_nasty_robot(vobjptridx_t player, vobjptridx_t robot, vms_vector *collision_point);

void net_destroy_controlcen(objptridx_t controlcen);
void collide_player_and_powerup(object *player, vobjptridx_t powerup, vms_vector *collision_point);
extern int check_effect_blowup(segment *seg,int side,vms_vector *pnt, object *blower, int force_blowup_flag, int remote);
void apply_damage_to_controlcen(vobjptridx_t controlcen, fix damage, objnum_t who);
extern void bump_one_object(object *obj0, vms_vector *hit_dir, fix damage);
void drop_player_eggs(vobjptridx_t playerobj);

#if defined(DXX_BUILD_DESCENT_II)
void do_final_boss_frame(void);
void do_final_boss_hacks(void);
int check_volatile_wall(object *obj,segnum_t segnum,int sidenum);
extern int	Final_boss_is_dead;
#endif

#endif

#endif /* _COLLIDE_H */
