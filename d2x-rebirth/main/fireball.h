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
 * Header for fireball.c
 *
 */


#ifndef _FIREBALL_H
#define _FIREBALL_H

// explosion types
#define ET_SPARKS       0   //little sparks, like when laser hits wall
#define ET_MULTI_START  1   //first part of multi-part explosion
#define ET_MULTI_SECOND 2   //second part of multi-part explosion

// data for exploding walls (such as hostage door)
typedef struct expl_wall {
	int segnum,sidenum;
	fix time;
} expl_wall;

#define MAX_EXPLODING_WALLS     10

extern expl_wall expl_wall_list[MAX_EXPLODING_WALLS];

object *object_create_explosion(short segnum, vms_vector *position, fix size, int vclip_type);
object *object_create_muzzle_flash(short segnum, vms_vector *position, fix size, int vclip_type);

object *object_create_badass_explosion(object *objp, short segnum,
		vms_vector *position, fix size, int vclip_type,
		fix maxdamage, fix maxdistance, fix maxforce, int parent);

// blows up a badass weapon, creating the badass explosion
// return the explosion object
object *explode_badass_weapon(object *obj,vms_vector *pos);

// blows up the player with a badass explosion
// return the explosion object
object *explode_badass_player(object *obj);

void explode_object(object *obj,fix delay_time);
void do_explosion_sequence(object *obj);
void do_debris_frame(object *obj);      // deal with debris for this frame

void draw_fireball(object *obj);

void explode_wall(int segnum, int sidenum);
void do_exploding_wall_frame(void);
void init_exploding_walls(void);
extern void maybe_drop_net_powerup(int powerup_type);
extern void maybe_replace_powerup_with_energy(object *del_obj);

extern int get_explosion_vclip(object *obj, int stage);
extern int drop_powerup(int type, int id, int num, vms_vector *init_vel, vms_vector *pos, int segnum);

// creates afterburner blobs behind the specified object
void drop_afterburner_blobs(object *obj, int count, fix size_scale, fix lifetime);

/*
 * reads n expl_wall structs from a PHYSFS_file and swaps if specified
 */
extern void expl_wall_read_n_swap(expl_wall *ew, int n, int swap, PHYSFS_file *fp);

#endif /* _FIREBALL_H */
