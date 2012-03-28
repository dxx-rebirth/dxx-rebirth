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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/
/*
 * Header for collide.c
 *
 */



#ifndef _COLLIDE_H
#define _COLLIDE_H

#include "playsave.h"

void collide_init();
void collide_two_objects( object * A, object * B, vms_vector *collision_point );
void collide_object_with_wall( object * A, fix hitspeed, short hitseg, short hitwall, vms_vector * hitpt );
extern void apply_damage_to_player(object *player, object *killer, fix damage, ubyte possibly_friendly);

//	Returns 1 if robot died, else 0.
extern int apply_damage_to_robot(object *robot, fix damage, int killer_objnum);

extern int Immaterial;

#define PERSISTENT_DEBRIS (PlayerCfg.PersistentDebris && !(Game_mode & GM_MULTI)) // no persistent debris in multi

extern void collide_player_and_weapon( object * player, object * weapon, vms_vector *collision_point );
extern void collide_player_and_materialization_center(object *objp);
extern void collide_robot_and_materialization_center(object *objp);

extern void scrape_player_on_wall(object *obj, short hitseg, short hitwall, vms_vector * hitpt );
extern int maybe_detonate_weapon(object *obj0p, object *obj, vms_vector *pos);

extern void collide_player_and_nasty_robot( object * player, object * robot, vms_vector *collision_point );

extern void net_destroy_controlcen(object *controlcen);
extern void collide_player_and_powerup( object * player, object * powerup, vms_vector *collision_point );
extern int check_effect_blowup(segment *seg,int side,vms_vector *pnt);
extern void apply_damage_to_controlcen(object *controlcen, fix damage, short who);
extern void bump_one_object(object *obj0, vms_vector *hit_dir, fix damage);


#endif
