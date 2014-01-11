/*
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

void collide_init();
void collide_two_objects(objptridx_t  A, objptridx_t  B, vms_vector *collision_point);
void collide_object_with_wall(objptridx_t A, fix hitspeed, short hitseg, short hitwall, vms_vector * hitpt);
extern void apply_damage_to_player(object *player, object *killer, fix damage, ubyte possibly_friendly);

// Returns 1 if robot died, else 0.
extern int apply_damage_to_robot(object *robot, fix damage, int killer_objnum);

extern int Immaterial;

#define PERSISTENT_DEBRIS (PlayerCfg.PersistentDebris && !(Game_mode & GM_MULTI)) // no persistent debris in multi

extern void collide_player_and_materialization_center(object *objp);
void collide_robot_and_materialization_center(objptridx_t objp);

extern void scrape_player_on_wall(object *obj, short hitseg, short hitwall, vms_vector * hitpt);
extern int maybe_detonate_weapon(object *obj0p, object *obj, vms_vector *pos);

void collide_player_and_nasty_robot(objptridx_t player, objptridx_t robot, vms_vector *collision_point);

void net_destroy_controlcen(objptridx_t controlcen);
extern void collide_player_and_powerup(object * player, object * powerup, vms_vector *collision_point);
extern int check_effect_blowup(segment *seg,int side,vms_vector *pnt, object *blower, int force_blowup_flag, int remote);
extern void apply_damage_to_controlcen(object *controlcen, fix damage, short who);
extern void bump_one_object(object *obj0, vms_vector *hit_dir, fix damage);
void drop_player_eggs(struct object *playerobj);

#if defined(DXX_BUILD_DESCENT_II)
void do_final_boss_frame(void);
void do_final_boss_hacks(void);
int check_volatile_wall(object *obj,int segnum,int sidenum,vms_vector *hitpt);
extern int	Final_boss_is_dead;
#endif

#endif

#endif /* _COLLIDE_H */
