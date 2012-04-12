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
 *
 * Routines to handle collisions
 *
 */

#ifdef RCS
static char rcsid[] = "$Id: collide.c,v 1.1.1.1 2006/03/17 19:41:32 zicodxx Exp $";
#endif

#include <string.h>	// for memset
#include <stdlib.h>
#include <stdio.h>

#include "rle.h"
#include "inferno.h"
#include "game.h"
#include "gr.h"
#include "stdlib.h"
#include "bm.h"
#include "3d.h"
#include "segment.h"
#include "texmap.h"
#include "laser.h"
#include "key.h"
#include "gameseg.h"
#include "object.h"
#include "physics.h"
#include "slew.h"
#include "wall.h"
#include "vclip.h"
#include "polyobj.h"
#include "fireball.h"
#include "laser.h"
#include "error.h"
#include "ai.h"
#include "hostage.h"
#include "fuelcen.h"
#include "sounds.h"
#include "robot.h"
#include "weapon.h"
#include "player.h"
#include "gauges.h"
#include "powerup.h"
#include "newmenu.h"
#include "scores.h"
#include "effects.h"
#include "textures.h"
#include "multi.h"
#include "cntrlcen.h"
#include "newdemo.h"
#include "endlevel.h"
#include "multibot.h"
#include "piggy.h"
#include "text.h"
#include "maths.h"

#ifdef EDITOR
#include "editor/editor.h"
#endif

#include "collide.h"

#ifdef SCRIPT
#include "script.h"
#endif

#define STANDARD_EXPL_DELAY (f1_0/4)

int check_collision_delayfunc_exec()
{
	static fix64 last_play_time=0;
	if (last_play_time + (F1_0/3) < GameTime64 || last_play_time > GameTime64)
	{
		last_play_time = GameTime64;
		last_play_time -= (d_rand()/2); // add some randomness
		return 1;
	}
	return 0;
}

//	-------------------------------------------------------------------------------------------------------------
//	The only reason this routine is called (as of 10/12/94) is so Brain guys can open doors.
void collide_robot_and_wall( object * robot, fix hitspeed, short hitseg, short hitwall, vms_vector * hitpt)
{
	if ((robot->id == ROBOT_BRAIN) || (robot->ctype.ai_info.behavior == AIB_RUN_FROM)) {
		int	wall_num = Segments[hitseg].sides[hitwall].wall_num;
		if (wall_num != -1) {
			if ((Walls[wall_num].type == WALL_DOOR) && (Walls[wall_num].keys == KEY_NONE) && (Walls[wall_num].state == WALL_DOOR_CLOSED) && !(Walls[wall_num].flags & WALL_DOOR_LOCKED)) {
				wall_open_door(&Segments[hitseg], hitwall);
			}
		}
	}

	return;
}

//##void collide_hostage_and_wall( object * hostage, fix hitspeed, short hitseg, short hitwall, vms_vector * hitpt)	{
//##	return;
//##}

//	-------------------------------------------------------------------------------------------------------------

int apply_damage_to_clutter(object *clutter, fix damage)
{
	if ( clutter->flags&OF_EXPLODING) return 0;

	if (clutter->shields < 0 ) return 0;	//clutter already dead...

	clutter->shields -= damage;

	if (clutter->shields < 0) {
		explode_object(clutter,0);
		return 1;
	} else
		return 0;
}


//given the specified force, apply damage from that force to an object
void apply_force_damage(object *obj,fix force,object *other_obj)
{
	int	result;
	fix damage;

	if (obj->flags & (OF_EXPLODING|OF_SHOULD_BE_DEAD))
		return;		//already exploding or dead

	damage = fixdiv(force,obj->mtype.phys_info.mass) / 8;

	switch (obj->type) {

		case OBJ_ROBOT:

			if (Robot_info[obj->id].attack_type == 1) {
				if (other_obj->type == OBJ_WEAPON)
					result = apply_damage_to_robot(obj,damage/4, other_obj->ctype.laser_info.parent_num);
				else
					result = apply_damage_to_robot(obj,damage/4, other_obj-Objects);
			}
			else {
				if (other_obj->type == OBJ_WEAPON)
					result = apply_damage_to_robot(obj,damage/2, other_obj->ctype.laser_info.parent_num);
				else
					result = apply_damage_to_robot(obj,damage/2, other_obj-Objects);
			}

			if (result && (other_obj->ctype.laser_info.parent_signature == ConsoleObject->signature))
				add_points_to_score(Robot_info[obj->id].score_value);
			break;

		case OBJ_PLAYER:

			//	If colliding with a claw type robot, do damage proportional to FrameTime because you can collide with those
			//	bots every frame since they don't move.
			if ( (other_obj->type == OBJ_ROBOT) && (Robot_info[other_obj->id].attack_type) )
				damage = fixmul(damage, FrameTime*2);

			apply_damage_to_player(obj,other_obj,damage,0);
			break;

		case OBJ_CLUTTER:

			apply_damage_to_clutter(obj,damage);
			break;

		case OBJ_CNTRLCEN:

			apply_damage_to_controlcen(obj,damage, other_obj-Objects);
			break;

		case OBJ_WEAPON:

			break;		//weapons don't take damage

		default:

			Int3();

	}
}

//	-----------------------------------------------------------------------------
void bump_this_object(object *objp, object *other_objp, vms_vector *force, int damage_flag)
{
	fix force_mag;

        if (! (objp->mtype.phys_info.flags & PF_PERSISTENT)) {
		if (objp->type == OBJ_PLAYER) {
			vms_vector force2;
			force2.x = force->x/4;
			force2.y = force->y/4;
			force2.z = force->z/4;
			phys_apply_force(objp,&force2);
			if (damage_flag) {
				force_mag = vm_vec_mag_quick(&force2);
				apply_force_damage(objp, force_mag, other_objp);
			}
		} else if ((objp->type == OBJ_ROBOT) || (objp->type == OBJ_CLUTTER) || (objp->type == OBJ_CNTRLCEN)) {
			if (!Robot_info[objp->id].boss_flag) {
				vms_vector force2;
				force2.x = force->x/(4 + Difficulty_level);
				force2.y = force->y/(4 + Difficulty_level);
				force2.z = force->z/(4 + Difficulty_level);

				phys_apply_force(objp, force);
				phys_apply_rot(objp, &force2);
				if (damage_flag) {
					force_mag = vm_vec_mag_quick(force);
					apply_force_damage(objp, force_mag, other_objp);
				}
			}
		}
        }
}

//	-----------------------------------------------------------------------------
//deal with two objects bumping into each other.  Apply force from collision
//to each robot.  The flags tells whether the objects should take damage from
//the collision.
void bump_two_objects(object *obj0,object *obj1,int damage_flag)
{
	vms_vector	force;
	object		*t=NULL;

	vm_vec_zero(&force);

	if (obj0->movement_type != MT_PHYSICS)
		t=obj1;
	else if (obj1->movement_type != MT_PHYSICS)
		t=obj0;

	if (t) {
		Assert(t->movement_type == MT_PHYSICS);
		vm_vec_copy_scale(&force,&t->mtype.phys_info.velocity,-t->mtype.phys_info.mass);
		phys_apply_force(t,&force);
		return;
	}

	vm_vec_sub(&force,&obj0->mtype.phys_info.velocity,&obj1->mtype.phys_info.velocity);
	vm_vec_scale2(&force,2*fixmul(obj0->mtype.phys_info.mass,obj1->mtype.phys_info.mass),(obj0->mtype.phys_info.mass+obj1->mtype.phys_info.mass));

	bump_this_object(obj1, obj0, &force, damage_flag);
	vm_vec_negate(&force);
	bump_this_object(obj0, obj1, &force, damage_flag);

}

void bump_one_object(object *obj0, vms_vector *hit_dir, fix damage)
{
	vms_vector	hit_vec;

	hit_vec = *hit_dir;
	vm_vec_scale(&hit_vec, damage);

	phys_apply_force(obj0,&hit_vec);

}

#define DAMAGE_SCALE 		128	//	Was 32 before 8:55 am on Thursday, September 15, changed by MK, walls were hurting me more than robots!
#define DAMAGE_THRESHOLD 	(F1_0/3)
#define WALL_LOUDNESS_SCALE (20)

void collide_player_and_wall( object * player, fix hitspeed, short hitseg, short hitwall, vms_vector * hitpt)
{
	fix damage;

	if (player->id != Player_num) // Execute only for local player
		return;

	//	If this wall does damage, don't make *BONK* sound, we'll be making another sound.
	if (TmapInfo[Segments[hitseg].sides[hitwall].tmap_num].damage > 0)
		return;

	wall_hit_process( &Segments[hitseg], hitwall, 20, player->id, player );

	//	** Damage from hitting wall **
	//	If the player has less than 10% shields, don't take damage from bump
	damage = hitspeed / DAMAGE_SCALE;

	if (damage >= DAMAGE_THRESHOLD) {
		int	volume;
		volume = (hitspeed-(DAMAGE_SCALE*DAMAGE_THRESHOLD)) / WALL_LOUDNESS_SCALE ;

		create_awareness_event(player, PA_WEAPON_WALL_COLLISION);

		if ( volume > F1_0 )
			volume = F1_0;
		if (volume > 0 ) {
			digi_link_sound_to_pos( SOUND_PLAYER_HIT_WALL, hitseg, 0, hitpt, 0, volume );
			#ifdef NETWORK
			if (Game_mode & GM_MULTI)
				multi_send_play_sound(SOUND_PLAYER_HIT_WALL, volume);
			#endif
		}

		if (!(Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE))
			if ( Players[Player_num].shields > f1_0*10 )
			  	apply_damage_to_player( player, player, damage, 0 );
	}

	return;
}

fix64	Last_volatile_scrape_sound_time = 0;

void collide_weapon_and_wall( object * weapon, fix hitspeed, short hitseg, short hitwall, vms_vector * hitpt);
void collide_debris_and_wall( object * debris, fix hitspeed, short hitseg, short hitwall, vms_vector * hitpt);

//this gets called when an object is scraping along the wall
void scrape_player_on_wall(object *obj, short hitseg, short hitside, vms_vector * hitpt )
{
	fix d;

	if (obj->type != OBJ_PLAYER || obj->id != Player_num)
		return;

	if ((d=TmapInfo[Segments[hitseg].sides[hitside].tmap_num].damage) > 0) {
		vms_vector	hit_dir, rand_vec;
		fix damage = fixmul(d,FrameTime);

		if (!(Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE))
			apply_damage_to_player( obj, obj, damage, 0 );

		PALETTE_FLASH_ADD(f2i(damage*4), 0, 0);	//flash red
		if ((GameTime64 > Last_volatile_scrape_sound_time + F1_0/4) || (GameTime64 < Last_volatile_scrape_sound_time)) {
			Last_volatile_scrape_sound_time = GameTime64;
			digi_link_sound_to_pos( SOUND_VOLATILE_WALL_HISS,hitseg, 0, hitpt, 0, F1_0 );
			#ifdef NETWORK
			if (Game_mode & GM_MULTI)
				multi_send_play_sound(SOUND_VOLATILE_WALL_HISS, F1_0);
			#endif
		}
		#ifdef COMPACT_SEGS
		get_side_normal(&Segments[hitseg], hitside, 0, &hit_dir );
		#else
		hit_dir = Segments[hitseg].sides[hitside].normals[0];
		#endif
		make_random_vector(&rand_vec);
		vm_vec_scale_add2(&hit_dir, &rand_vec, F1_0/8);
		vm_vec_normalize_quick(&hit_dir);
		bump_one_object(obj, &hit_dir, F1_0*8);

		obj->mtype.phys_info.rotvel.x = (d_rand() - 16384)/2;
		obj->mtype.phys_info.rotvel.z = (d_rand() - 16384)/2;

	}
}

//if an effect is hit, and it can blow up, then blow it up
//returns true if it blew up
int check_effect_blowup(segment *seg,int side,vms_vector *pnt)
{
	int tm,ec,db;

	if ((tm=seg->sides[side].tmap_num2) != 0)
		if ((ec=TmapInfo[tm&0x3fff].eclip_num)!=-1)
   		if ((db=Effects[ec].dest_bm_num)!=-1 && !(Effects[ec].flags&EF_ONE_SHOT)) {
				fix u,v;
				grs_bitmap *bm = &GameBitmaps[Textures[tm&0x3fff].index];
				int x,y,t;

				PIGGY_PAGE_IN(Textures[tm&0x3fff]);

				//this can be blown up...did we hit it?

				find_hitpoint_uv(&u,&v,pnt,seg,side,0);	//evil: always say face zero

				x = ((unsigned) f2i(u*bm->bm_w)) % bm->bm_w;
				y = ((unsigned) f2i(v*bm->bm_h)) % bm->bm_h;

				switch (tm&0xc000) {		//adjust for orientation of paste-on

					case 0x0000:	break;
					case 0x4000:	t=y; y=x; x=bm->bm_w-t-1; break;
					case 0x8000:	y=bm->bm_h-y-1; x=bm->bm_w-x-1; break;
					case 0xc000:	t=x; x=y; y=bm->bm_h-t-1; break;

				}


				if (bm->bm_flags & BM_FLAG_RLE)
					bm = rle_expand_texture(bm);

				if (gr_gpixel (bm, x, y) != 255) {		//not trans, thus on effect
					int vc,sound_num;

					if (Newdemo_state == ND_STATE_RECORDING)
						newdemo_record_effect_blowup( seg-Segments, side, pnt);

					vc = Effects[ec].dest_vclip;

					object_create_explosion( seg-Segments, pnt, Effects[ec].dest_size, vc );

					if ((sound_num = Vclip[vc].sound_num) != -1)
			  			digi_link_sound_to_pos( sound_num, seg-Segments, 0, pnt,  0, F1_0 );

					if ((sound_num=Effects[ec].sound_num)!=-1)		//kill sound
						digi_kill_sound_linked_to_segment(seg-Segments,side,sound_num);

					if (Effects[ec].dest_eclip!=-1 && Effects[Effects[ec].dest_eclip].segnum==-1) {
						int bm_num;
						eclip *new_ec;

						new_ec = &Effects[Effects[ec].dest_eclip];
						bm_num = new_ec->changing_wall_texture;

						new_ec->time_left = new_ec->vc.frame_time;
						new_ec->frame_count = 0;
						new_ec->segnum = seg-Segments;
						new_ec->sidenum = side;
						new_ec->flags |= EF_ONE_SHOT;
						new_ec->dest_bm_num = Effects[ec].dest_bm_num;

						Assert(bm_num!=0 && seg->sides[side].tmap_num2!=0);
						seg->sides[side].tmap_num2 = bm_num | (tm&0xc000);		//replace with destoyed

					}
					else {
						Assert(db!=0 && seg->sides[side].tmap_num2!=0);
						seg->sides[side].tmap_num2 = db | (tm&0xc000);		//replace with destoyed
					}

					return 1;		//blew up!
				}
			}

	return 0;		//didn't blow up
}

//these gets added to the weapon's values when the weapon hits a volitle wall
#define VOLATILE_WALL_EXPL_STRENGTH i2f(10)
#define VOLATILE_WALL_IMPACT_SIZE	i2f(3)
#define VOLATILE_WALL_DAMAGE_FORCE	i2f(5)
#define VOLATILE_WALL_DAMAGE_RADIUS	i2f(30)

// int Show_seg_and_side = 0;

void collide_weapon_and_wall( object * weapon, fix hitspeed, short hitseg, short hitwall, vms_vector * hitpt)
{
	segment *seg = &Segments[hitseg];
	int blew_up;
	int wall_type;
	int playernum;

	if (weapon->mtype.phys_info.flags & PF_BOUNCE)
		return;

	if ((weapon->mtype.phys_info.velocity.x == 0) && (weapon->mtype.phys_info.velocity.y == 0) && (weapon->mtype.phys_info.velocity.z == 0)) {
		Int3();	//	Contact Matt: This is impossible.  A weapon with 0 velocity hit a wall, which doesn't move.
		return;
	}

	blew_up = check_effect_blowup(seg,hitwall,hitpt);

	//if ((seg->sides[hitwall].tmap_num2==0) && (TmapInfo[seg->sides[hitwall].tmap_num].flags & TMI_VOLATILE)) {

	if (Objects[weapon->ctype.laser_info.parent_num].type == OBJ_PLAYER)
		playernum = Objects[weapon->ctype.laser_info.parent_num].id;
	else
		playernum = -1;		//not a player

	wall_type = wall_hit_process( seg, hitwall, weapon->shields, playernum, weapon );

	// Wall is volatile if either tmap 1 or 2 is volatile
	if ((TmapInfo[seg->sides[hitwall].tmap_num].flags & TMI_VOLATILE) || (seg->sides[hitwall].tmap_num2 && (TmapInfo[seg->sides[hitwall].tmap_num2&0x3fff].flags & TMI_VOLATILE))) {
		weapon_info *wi = &Weapon_info[weapon->id];

		//we've hit a volatile wall

		digi_link_sound_to_pos( SOUND_VOLATILE_WALL_HIT,hitseg, 0, hitpt, 0, F1_0 );

		object_create_badass_explosion( weapon, hitseg, hitpt,
			wi->impact_size + VOLATILE_WALL_IMPACT_SIZE,
			VCLIP_VOLATILE_WALL_HIT,
			wi->strength[Difficulty_level]/4+VOLATILE_WALL_EXPL_STRENGTH,	//	diminished by mk on 12/08/94, i was doing 70 damage hitting lava on lvl 1.
			wi->damage_radius+VOLATILE_WALL_DAMAGE_RADIUS,
			wi->strength[Difficulty_level]/2+VOLATILE_WALL_DAMAGE_FORCE,
			weapon->ctype.laser_info.parent_num );

		weapon->flags |= OF_SHOULD_BE_DEAD;		//make flares die in lava
	}
	else {

		//if it's not the player's weapon, or it is the player's and there
		//is no wall, and no blowing up monitor, then play sound
		if ((weapon->ctype.laser_info.parent_type != OBJ_PLAYER) ||	((seg->sides[hitwall].wall_num == -1 || wall_type==WHP_NOT_SPECIAL) && !blew_up))
			if ((Weapon_info[weapon->id].wall_hit_sound > -1 ) && (!(weapon->flags & OF_SILENT)))
				digi_link_sound_to_pos( Weapon_info[weapon->id].wall_hit_sound,weapon->segnum, 0, &weapon->pos, 0, F1_0 );

		if ( Weapon_info[weapon->id].wall_hit_vclip > -1 )	{
			if ( Weapon_info[weapon->id].damage_radius )
				explode_badass_weapon(weapon);
			else
				object_create_explosion( weapon->segnum, &weapon->pos, Weapon_info[weapon->id].impact_size, Weapon_info[weapon->id].wall_hit_vclip );
		}
	}

	if ( weapon->ctype.laser_info.parent_type== OBJ_PLAYER )	{

		if (!(weapon->flags & OF_SILENT) && (weapon->ctype.laser_info.parent_num == Players[Player_num].objnum))
			create_awareness_event(weapon, PA_WEAPON_WALL_COLLISION);			// object "weapon" can attract attention to player

//		if (weapon->id != FLARE_ID) {
//	We now allow flares to open doors.
		{
			if (weapon->id != FLARE_ID)
				weapon->flags |= OF_SHOULD_BE_DEAD;

			if (!(weapon->flags & OF_SILENT)) {
				switch (wall_type) {

					case WHP_NOT_SPECIAL:
						//should be handled above
//						digi_link_sound_to_pos( Weapon_info[weapon->id].wall_hit_sound, weapon->segnum, 0, &weapon->pos, 0, F1_0 );
						break;

					case WHP_NO_KEY:
						//play special hit door sound (if/when we get it)
						digi_link_sound_to_pos( SOUND_WEAPON_HIT_DOOR, weapon->segnum, 0, &weapon->pos, 0, F1_0 );
						break;

					case WHP_BLASTABLE:
						//play special blastable wall sound (if/when we get it)
						digi_link_sound_to_pos( SOUND_WEAPON_HIT_BLASTABLE, weapon->segnum, 0, &weapon->pos, 0, F1_0 );
						break;

					case WHP_DOOR:
						//don't play anything, since door open sound will play
						break;
				}
			} // else
		} // else {
			//	if (weapon->lifeleft <= 0)
			//	weapon->flags |= OF_SHOULD_BE_DEAD;
		// }

	} else {
		// This is a robot's laser
		weapon->flags |= OF_SHOULD_BE_DEAD;
	}

	return;
}

//##void collide_camera_and_wall( object * camera, fix hitspeed, short hitseg, short hitwall, vms_vector * hitpt)	{
//##	return;
//##}

//##void collide_powerup_and_wall( object * powerup, fix hitspeed, short hitseg, short hitwall, vms_vector * hitpt)	{
//##	return;
//##}

void collide_debris_and_wall( object * debris, fix hitspeed, short hitseg, short hitwall, vms_vector * hitpt)	{
	if (!PERSISTENT_DEBRIS || TmapInfo[Segments[hitseg].sides[hitwall].tmap_num].damage)
		explode_object(debris,0);
	return;
}

//##void collide_fireball_and_fireball( object * fireball1, object * fireball2, vms_vector *collision_point ) {
//##	return;
//##}

//##void collide_fireball_and_robot( object * fireball, object * robot, vms_vector *collision_point ) {
//##	return;
//##}

//##void collide_fireball_and_hostage( object * fireball, object * hostage, vms_vector *collision_point ) {
//##	return;
//##}

//##void collide_fireball_and_player( object * fireball, object * player, vms_vector *collision_point ) {
//##	return;
//##}

//##void collide_fireball_and_weapon( object * fireball, object * weapon, vms_vector *collision_point ) {
//##	//weapon->flags |= OF_SHOULD_BE_DEAD;
//##	return;
//##}

//##void collide_fireball_and_camera( object * fireball, object * camera, vms_vector *collision_point ) {
//##	return;
//##}

//##void collide_fireball_and_powerup( object * fireball, object * powerup, vms_vector *collision_point ) {
//##	return;
//##}

//##void collide_fireball_and_debris( object * fireball, object * debris, vms_vector *collision_point ) {
//##	return;
//##}

//	-------------------------------------------------------------------------------------------------------------------
void collide_robot_and_robot( object * robot1, object * robot2, vms_vector *collision_point ) {
	bump_two_objects(robot1, robot2, 1);
	return;
}

void collide_robot_and_controlcen( object * obj1, object * obj2, vms_vector *collision_point )
{
	if (obj1->type == OBJ_ROBOT) {
		vms_vector	hitvec;
		vm_vec_normalize(vm_vec_sub(&hitvec, &obj2->pos, &obj1->pos));
		bump_one_object(obj1, &hitvec, 0);
	} else {
		vms_vector	hitvec;
		vm_vec_normalize(vm_vec_sub(&hitvec, &obj1->pos, &obj2->pos));
		bump_one_object(obj2, &hitvec, 0);
	}
}

//##void collide_robot_and_hostage( object * robot, object * hostage, vms_vector *collision_point ) {
//##	return;
//##}

void collide_robot_and_player( object * robot, object * player, vms_vector *collision_point ) {
	if (player->id == Player_num) {
		create_awareness_event(player, PA_PLAYER_COLLISION);			// object robot can attract attention to player
		do_ai_robot_hit_attack(robot, player, collision_point);
		do_ai_robot_hit(robot, PA_WEAPON_ROBOT_COLLISION);
	}
#ifdef NETWORK
#ifndef SHAREWARE
	else
	{
		multi_robot_request_change(robot, player->id);
		return; // only controlling player should make damage otherwise we might juggle robot back and forth, killing it instantly
	}
#endif
#endif

	if (check_collision_delayfunc_exec())
	{
		int	collision_seg = find_point_seg(collision_point, player->segnum);

		digi_link_sound_to_pos( SOUND_ROBOT_HIT_PLAYER, player->segnum, 0, collision_point, 0, F1_0 );

		if (collision_seg != -1)
			object_create_explosion( collision_seg, collision_point, Weapon_info[0].impact_size, Weapon_info[0].wall_hit_vclip );
	}

	bump_two_objects(robot, player, 1);

	return;
}

// Provide a way for network message to instantly destroy the control center
// without awarding points or anything.

//	if controlcen == NULL, that means don't do the explosion because the control center
//	was actually in another object.
void net_destroy_controlcen(object *controlcen)
{
	if (Control_center_destroyed != 1) {
		do_controlcen_destroyed_stuff(controlcen);

		if ((controlcen != NULL) && !(controlcen->flags&(OF_EXPLODING|OF_DESTROYED))) {
			digi_link_sound_to_pos( SOUND_CONTROL_CENTER_DESTROYED, controlcen->segnum, 0, &controlcen->pos, 0, F1_0 );
			explode_object(controlcen,0);
		}
	}

}

//	-----------------------------------------------------------------------------
void apply_damage_to_controlcen(object *controlcen, fix damage, short who)
{
	int	whotype;

	//	Only allow a player to damage the control center.

	if ((who < 0) || (who > Highest_object_index))
		return;

	whotype = Objects[who].type;
	if (whotype != OBJ_PLAYER) {
		return;
	}

	#ifdef NETWORK
	#ifndef SHAREWARE
	if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP) && ((i2f(Players[Player_num].hours_level*3600)+Players[Player_num].time_level) < Netgame.control_invul_time))
	{
		if (Objects[who].id == Player_num) {
			int secs = f2i(Netgame.control_invul_time-(i2f(Players[Player_num].hours_level*3600)+Players[Player_num].time_level)) % 60;
			int mins = f2i(Netgame.control_invul_time-(i2f(Players[Player_num].hours_level*3600)+Players[Player_num].time_level)) / 60;
			HUD_init_message(HM_DEFAULT, "%s %d:%02d.", TXT_CNTRLCEN_INVUL, mins, secs);
		}
		return;
	}
	#endif
	#endif

	if (Objects[who].id == Player_num) {
		Control_center_been_hit = 1;
		ai_do_cloak_stuff();
	}

	if ( controlcen->shields >= 0 )
		controlcen->shields -= damage;

	if ( (controlcen->shields < 0) && !(controlcen->flags&(OF_EXPLODING|OF_DESTROYED)) ) {
		do_controlcen_destroyed_stuff(controlcen);

		#ifdef NETWORK
		if (Game_mode & GM_MULTI) {
			if (who == Players[Player_num].objnum)
				add_points_to_score(CONTROL_CEN_SCORE);
			multi_send_destroy_controlcen((ushort)(controlcen-Objects), Objects[who].id );
		}
		#endif

		if (!(Game_mode & GM_MULTI))
			add_points_to_score(CONTROL_CEN_SCORE);

		digi_link_sound_to_pos( SOUND_CONTROL_CENTER_DESTROYED, controlcen->segnum, 0, &controlcen->pos, 0, F1_0 );

		explode_object(controlcen,0);
	}
}

void collide_player_and_controlcen( object * controlcen, object * player, vms_vector *collision_point )
{
	if (player->id == Player_num) {
		Control_center_been_hit = 1;
		ai_do_cloak_stuff();				//	In case player cloaked, make control center know where he is.
	}

	if (check_collision_delayfunc_exec())
		digi_link_sound_to_pos( SOUND_ROBOT_HIT_PLAYER, player->segnum, 0, collision_point, 0, F1_0 );

	bump_two_objects(controlcen, player, 1);

	return;
}

//	If a persistent weapon and other object is not a weapon, weaken it, else kill it.
//	If both objects are weapons, weaken the weapon.
void maybe_kill_weapon(object *weapon, object *other_obj)
{
	if (weapon->id == PROXIMITY_ID) {
		weapon->flags |= OF_SHOULD_BE_DEAD;
		return;
	}

	if ((weapon->mtype.phys_info.flags & PF_PERSISTENT) || (other_obj->type == OBJ_WEAPON)) {
		//	Weapons do a lot of damage to weapons, other objects do much less.
		if (!(weapon->mtype.phys_info.flags & PF_PERSISTENT)) {
			if (other_obj->type == OBJ_WEAPON)
				weapon->shields -= other_obj->shields/2;
			else
				weapon->shields -= other_obj->shields/4;

			if (weapon->shields <= 0) {
				weapon->shields = 0;
				weapon->flags |= OF_SHOULD_BE_DEAD;
			}
		}
	} else
		weapon->flags |= OF_SHOULD_BE_DEAD;
}

void collide_weapon_and_controlcen( object * weapon, object *controlcen, vms_vector *collision_point  )
{

	if (weapon->ctype.laser_info.parent_type == OBJ_PLAYER) {
		fix	damage = weapon->shields;

		/*
		* Check if persistent weapon already hit this object. If yes, abort.
		* If no, add this object to hitobj_list and do it's damage.
		*/
		if (weapon->mtype.phys_info.flags & PF_PERSISTENT)
		{
			damage = weapon->shields*2; // to not alter Gameplay too much, multiply damage by 2.
			if (!weapon->ctype.laser_info.hitobj_list[controlcen-Objects])
			{
				weapon->ctype.laser_info.hitobj_list[controlcen-Objects] = 1;
				weapon->ctype.laser_info.last_hitobj = controlcen-Objects;
			}
			else
			{
				return;
			}
		}

		if (Objects[weapon->ctype.laser_info.parent_num].id == Player_num)
			Control_center_been_hit = 1;

		if ( Weapon_info[weapon->id].damage_radius )
			explode_badass_weapon(weapon);
		else
			object_create_explosion( controlcen->segnum, collision_point, ((controlcen->size/3)*3)/4, VCLIP_SMALL_EXPLOSION );

		digi_link_sound_to_pos( SOUND_CONTROL_CENTER_HIT, controlcen->segnum, 0, collision_point, 0, F1_0 );

		damage = fixmul(damage, weapon->ctype.laser_info.multiplier);

		apply_damage_to_controlcen(controlcen, damage, weapon->ctype.laser_info.parent_num);

		maybe_kill_weapon(weapon,controlcen);
	} else {	//	If robot weapon hits control center, blow it up, make it go away, but do no damage to control center.
		object_create_explosion( controlcen->segnum, collision_point, ((controlcen->size/3)*3)/4, VCLIP_SMALL_EXPLOSION );
		maybe_kill_weapon(weapon,controlcen);
	}

}

void collide_weapon_and_clutter( object * weapon, object *clutter, vms_vector *collision_point  )	{
	short exp_vclip = VCLIP_SMALL_EXPLOSION;

	if ( clutter->shields >= 0 )
		clutter->shields -= weapon->shields;

	digi_link_sound_to_pos( SOUND_LASER_HIT_CLUTTER, weapon->segnum, 0, collision_point, 0, F1_0 );

	object_create_explosion( clutter->segnum, collision_point, ((clutter->size/3)*3)/4, exp_vclip );

	if ( (clutter->shields < 0) && !(clutter->flags&(OF_EXPLODING|OF_DESTROYED)))
		explode_object(clutter,STANDARD_EXPL_DELAY);

	maybe_kill_weapon(weapon,clutter);
}

//--mk, 121094 -- extern void spin_robot(object *robot, vms_vector *collision_point);

//	------------------------------------------------------------------------------------------------------
//	Return 1 if robot died, else return 0
int apply_damage_to_robot(object *robot, fix damage, int killer_objnum)
{
	if ( robot->flags&OF_EXPLODING) return 0;

	if (robot->shields < 0 ) return 0;	//robot already dead...

//	if (robot->control_type == CT_REMOTE)
//		return 0; // Can't damange a robot controlled by another player

	if (Robot_info[robot->id].boss_flag)
		Boss_been_hit = 1;

	robot->shields -= damage;

	if (robot->shields < 0) {

#ifdef SCRIPT
		script_notify(NT_ROBOT_DIED, robot - Objects);
#endif

#ifndef SHAREWARE
#ifdef NETWORK
		if (Game_mode & GM_MULTI) {
			if (multi_explode_robot_sub(robot-Objects, killer_objnum))
			{
				multi_send_robot_explode(robot-Objects, killer_objnum);
				return 1;
			}
			else
				return 0;
		}
#endif
#endif

		Players[Player_num].num_kills_level++;
		Players[Player_num].num_kills_total++;

		if (Robot_info[robot->id].boss_flag) {
			start_boss_death_sequence(robot);	//do_controlcen_destroyed_stuff(NULL);
		} else
			explode_object(robot,STANDARD_EXPL_DELAY);
		return 1;
	} else
		return 0;
}

//	------------------------------------------------------------------------------------------------------
void collide_robot_and_weapon( object * robot, object * weapon, vms_vector *collision_point )
{

	if (Robot_info[robot->id].boss_flag)
		Boss_hit_this_frame = 1;

#if 1
	/*
	 * Check if persistent weapon already hit this object. If yes, abort.
	 * If no, add this object to hitobj_list and do it's damage.
	 */
	if (weapon->mtype.phys_info.flags & PF_PERSISTENT)
	{
		if (!weapon->ctype.laser_info.hitobj_list[robot-Objects])
		{
			weapon->ctype.laser_info.hitobj_list[robot-Objects] = 1;
			weapon->ctype.laser_info.last_hitobj = robot-Objects;
		}
		else
		{
			return;
		}
	}
#else
	//	If a persistent weapon hit robot most recently, quick abort, else we cream the same robot many times,
	//	depending on frame rate.
	if (weapon->mtype.phys_info.flags & PF_PERSISTENT) {
		if (weapon->ctype.laser_info.last_hitobj == robot-Objects)
			return;
		else
			weapon->ctype.laser_info.last_hitobj = robot-Objects;
	}
#endif

	if (weapon->ctype.laser_info.parent_signature == robot->signature)
		return;

	if ( Weapon_info[weapon->id].damage_radius )
		explode_badass_weapon(weapon);

	if ( (weapon->ctype.laser_info.parent_type==OBJ_PLAYER) && !(robot->flags & OF_EXPLODING) )	{
		object *expl_obj=NULL;

		if (weapon->ctype.laser_info.parent_num == Players[Player_num].objnum) {
			create_awareness_event(weapon, PA_WEAPON_ROBOT_COLLISION);			// object "weapon" can attract attention to player
			do_ai_robot_hit(robot, PA_WEAPON_ROBOT_COLLISION);
		}
#ifdef NETWORK
#ifndef SHAREWARE
		else
			multi_robot_request_change(robot, Objects[weapon->ctype.laser_info.parent_num].id);
#endif
#endif

//--mk, 121094 -- 		spin_robot(robot, collision_point);

		if ( Robot_info[robot->id].exp1_vclip_num > -1 )
			expl_obj = object_create_explosion( weapon->segnum, collision_point, (robot->size/2*3)/4, Robot_info[robot->id].exp1_vclip_num );
//NOT_USED		else if ( Weapon_info[weapon->id].robot_hit_vclip > -1 )
//NOT_USED			expl_obj = object_create_explosion( weapon->segnum, collision_point, Weapon_info[weapon->id].impact_size, Weapon_info[weapon->id].robot_hit_vclip );

		if (expl_obj)
			obj_attach(robot,expl_obj);

		if ( Robot_info[robot->id].exp1_sound_num > -1 )
			digi_link_sound_to_pos( Robot_info[robot->id].exp1_sound_num, robot->segnum, 0, collision_point, 0, F1_0 );

		if (!(weapon->flags & OF_HARMLESS)) {
			fix	damage = weapon->shields;

			damage = fixmul(damage, weapon->ctype.laser_info.multiplier);

			if (! apply_damage_to_robot(robot, damage, weapon->ctype.laser_info.parent_num))
				bump_two_objects(robot, weapon, 0);		//only bump if not dead. no damage from bump
			else if (weapon->ctype.laser_info.parent_signature == ConsoleObject->signature)
				add_points_to_score(Robot_info[robot->id].score_value);
		}

	}

	maybe_kill_weapon(weapon,robot);

	return;
}

//##void collide_robot_and_camera( object * robot, object * camera, vms_vector *collision_point ) {
//##	return;
//##}

//##void collide_robot_and_powerup( object * robot, object * powerup, vms_vector *collision_point ) {
//##	return;
//##}

//##void collide_robot_and_debris( object * robot, object * debris, vms_vector *collision_point ) {
//##	return;
//##}

//##void collide_hostage_and_hostage( object * hostage1, object * hostage2, vms_vector *collision_point ) {
//##	return;
//##}

void collide_hostage_and_player( object * hostage, object * player, vms_vector *collision_point ) {
	// Give player points, etc.
	if ( player == ConsoleObject )	{
		add_points_to_score(HOSTAGE_SCORE);

		// Do effect
		hostage_rescue(hostage->id);

		// Remove the hostage object.
		hostage->flags |= OF_SHOULD_BE_DEAD;

		#ifdef NETWORK
		if (Game_mode & GM_MULTI)
			multi_send_remobj(hostage-Objects);
		#endif
	}
	return;
}

//--unused-- void collide_hostage_and_weapon( object * hostage, object * weapon, vms_vector *collision_point )
//--unused-- {
//--unused-- 	//	Cannot kill hostages, as per Matt's edict!
//--unused-- 	//	(A fine edict, but in contradiction to the milestone: "Robots attack hostages.")
//--unused-- 	hostage->shields -= weapon->shields/2;
//--unused--
//--unused-- 	create_awareness_event(weapon, PA_WEAPON_ROBOT_COLLISION);			// object "weapon" can attract attention to player
//--unused--
//--unused-- 	//PLAY_SOUND_3D( SOUND_HOSTAGE_KILLED, collision_point, hostage->segnum );
//--unused-- 	digi_link_sound_to_pos( SOUND_HOSTAGE_KILLED, hostage->segnum , 0, collision_point, 0, F1_0 );
//--unused--
//--unused--
//--unused-- 	if (hostage->shields <= 0) {
//--unused-- 		explode_object(hostage,0);
//--unused-- 		hostage->flags |= OF_SHOULD_BE_DEAD;
//--unused-- 	}
//--unused--
//--unused-- 	if ( Weapon_info[weapon->id].damage_radius )
//--unused-- 		explode_badass_weapon(weapon);
//--unused--
//--unused-- 	maybe_kill_weapon(weapon,hostage);
//--unused--
//--unused-- }

//##void collide_hostage_and_camera( object * hostage, object * camera, vms_vector *collision_point ) {
//##	return;
//##}

//##void collide_hostage_and_powerup( object * hostage, object * powerup, vms_vector *collision_point ) {
//##	return;
//##}

//##void collide_hostage_and_debris( object * hostage, object * debris, vms_vector *collision_point ) {
//##	return;
//##}

void collide_player_and_player( object * player1, object * player2, vms_vector *collision_point ) {
	if (check_collision_delayfunc_exec())
		digi_link_sound_to_pos( SOUND_ROBOT_HIT_PLAYER, player1->segnum, 0, collision_point, 0, F1_0 );

	bump_two_objects(player1, player2, 1);
	return;
}

int maybe_drop_primary_weapon_egg(object *playerobj, int weapon_index)
{
	int weapon_flag = HAS_FLAG(weapon_index);
	int powerup_num;

	powerup_num = Primary_weapon_to_powerup[weapon_index];

	if (Players[playerobj->id].primary_weapon_flags & weapon_flag)
		return call_object_create_egg(playerobj, 1, OBJ_POWERUP, powerup_num);
	else
		return -1;
}

void maybe_drop_secondary_weapon_egg(object *playerobj, int weapon_index, int count)
{
	int weapon_flag = HAS_FLAG(weapon_index);
	int powerup_num;

	powerup_num = Secondary_weapon_to_powerup[weapon_index];

	if (Players[playerobj->id].secondary_weapon_flags & weapon_flag) {
		int	i, max_count;

		max_count = min(count, 3);
		for (i=0; i<max_count; i++)
			call_object_create_egg(playerobj, 1, OBJ_POWERUP, powerup_num);
	}
}

void drop_missile_1_or_4(object *playerobj,int missile_index)
{
	int num_missiles,powerup_id;

	num_missiles = Players[playerobj->id].secondary_ammo[missile_index];
	powerup_id = Secondary_weapon_to_powerup[missile_index];

	if (num_missiles > 10)
		num_missiles = 10;

	call_object_create_egg(playerobj, num_missiles/4, OBJ_POWERUP, powerup_id+1);
	call_object_create_egg(playerobj, num_missiles%4, OBJ_POWERUP, powerup_id);
}

void drop_player_eggs(object *playerobj)
{
	if ((playerobj->type == OBJ_PLAYER) || (playerobj->type == OBJ_GHOST)) {
		int	pnum = playerobj->id;
		int	objnum;
		int	vulcan_ammo=0;

		// Seed the random number generator so in net play the eggs will always
		// drop the same way
		#ifdef NETWORK
		if (Game_mode & GM_MULTI)
		{
			Net_create_loc = 0;
			d_srand(5483L);
		}
		#endif

		//	If the player dies and he has powerful lasers, create the powerups here.

		if (Players[pnum].laser_level >= 1)
			call_object_create_egg(playerobj, Players[pnum].laser_level, OBJ_POWERUP, POW_LASER);	// Note: laser_level = 0 for laser level 1.

		//	Drop quad laser if appropos
		if (Players[pnum].flags & PLAYER_FLAGS_QUAD_LASERS)
			call_object_create_egg(playerobj, 1, OBJ_POWERUP, POW_QUAD_FIRE);

		if (Players[pnum].flags & PLAYER_FLAGS_CLOAKED)
			call_object_create_egg(playerobj, 1, OBJ_POWERUP, POW_CLOAK);

		//Drop the vulcan, gauss, and ammo
		vulcan_ammo = Players[pnum].primary_ammo[VULCAN_INDEX];
		if (vulcan_ammo < VULCAN_AMMO_AMOUNT)
			vulcan_ammo = VULCAN_AMMO_AMOUNT;	//make sure gun has at least as much as a powerup
		objnum = maybe_drop_primary_weapon_egg(playerobj, VULCAN_INDEX);
		if (objnum!=-1)
			Objects[objnum].ctype.powerup_info.count = vulcan_ammo;

		//	Drop the rest of the primary weapons
		maybe_drop_primary_weapon_egg(playerobj, SPREADFIRE_INDEX);
		maybe_drop_primary_weapon_egg(playerobj, PLASMA_INDEX);
		maybe_drop_primary_weapon_egg(playerobj, FUSION_INDEX);

		//	Drop the secondary weapons
		//	Note, proximity weapon only comes in packets of 4.  So drop n/2, but a max of 3 (handled inside maybe_drop..)  Make sense?

		maybe_drop_secondary_weapon_egg(playerobj, PROXIMITY_INDEX, (Players[playerobj->id].secondary_ammo[PROXIMITY_INDEX])/4);

		maybe_drop_secondary_weapon_egg(playerobj, SMART_INDEX, Players[playerobj->id].secondary_ammo[SMART_INDEX]);
		maybe_drop_secondary_weapon_egg(playerobj, MEGA_INDEX, Players[playerobj->id].secondary_ammo[MEGA_INDEX]);

		//	Drop the player's missiles in packs of 1 and/or 4
		drop_missile_1_or_4(playerobj,HOMING_INDEX);
		drop_missile_1_or_4(playerobj,CONCUSSION_INDEX);

		//	If player has vulcan ammo, but no vulcan cannon, drop the ammo.
		if (!(Players[playerobj->id].primary_weapon_flags & HAS_VULCAN_FLAG)) {
			int	amount = Players[playerobj->id].primary_ammo[VULCAN_INDEX];
			if (amount > 200) {
				amount = 200;
			}
			while (amount > 0) {
				call_object_create_egg(playerobj, 1, OBJ_POWERUP, POW_VULCAN_AMMO);
				amount -= VULCAN_AMMO_AMOUNT;
			}
		}

		//	Always drop a shield and energy powerup.
		if (Game_mode & GM_MULTI) {
			call_object_create_egg(playerobj, 1, OBJ_POWERUP, POW_SHIELD_BOOST);
			call_object_create_egg(playerobj, 1, OBJ_POWERUP, POW_ENERGY);
		}
	}
}

void apply_damage_to_player(object *player, object *killer, fix damage, ubyte possibly_friendly)
{
	if (Player_is_dead)
		return;

	if (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE)
		return;

	if (multi_maybe_disable_friendly_fire(killer) && possibly_friendly)
		return;

	if (Endlevel_sequence)
		return;

	//for the player, the 'real' shields are maintained in the Players[]
	//array.  The shields value in the player's object are, I think, not
	//used anywhere.  This routine, however, sets the objects shields to
	//be a mirror of the value in the Player structure.

	if (player->id == Player_num) {		//is this the local player?
		Players[Player_num].shields -= damage;
		PALETTE_FLASH_ADD(f2i(damage)*4,-f2i(damage/2),-f2i(damage/2));	//flash red

		if (Players[Player_num].shields < 0)	{

  			Players[Player_num].killer_objnum = killer-Objects;

//			if ( killer && (killer->type == OBJ_PLAYER))
//				Players[Player_num].killer_objnum = killer-Objects;

			player->flags |= OF_SHOULD_BE_DEAD;
		}

		player->shields = Players[Player_num].shields;		//mirror

	}
}

void collide_player_and_weapon( object * player, object * weapon, vms_vector *collision_point )
{
	fix		damage = weapon->shields;
	object * killer=NULL;

	damage = fixmul(damage, weapon->ctype.laser_info.multiplier);

#if 1
	/*
	 * Check if persistent weapon already hit this object. If yes, abort.
	 * If no, add this object to hitobj_list and do it's damage.
	 */
	if (weapon->mtype.phys_info.flags & PF_PERSISTENT)
	{
		if (!weapon->ctype.laser_info.hitobj_list[player-Objects])
		{
			weapon->ctype.laser_info.hitobj_list[player-Objects] = 1;
			weapon->ctype.laser_info.last_hitobj = player-Objects;
		}
		else
		{
			return;
		}
	}
#else
        if (weapon->mtype.phys_info.flags & PF_PERSISTENT) {
		if (weapon->ctype.laser_info.last_hitobj == player-Objects)
			return;
		else
			weapon->ctype.laser_info.last_hitobj = player-Objects;
        }
#endif

	if (player->id == Player_num)
	{
		if (!(Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE))
		{
			digi_link_sound_to_pos( SOUND_PLAYER_GOT_HIT, player->segnum, 0, collision_point, 0, F1_0 );
			#ifdef NETWORK
			if (Game_mode & GM_MULTI)
				multi_send_play_sound(SOUND_PLAYER_GOT_HIT, F1_0);
			#endif
		}
		else
		{
			digi_link_sound_to_pos( SOUND_WEAPON_HIT_DOOR, player->segnum, 0, collision_point, 0, F1_0);
			#ifdef NETWORK
			if (Game_mode & GM_MULTI)
				multi_send_play_sound(SOUND_WEAPON_HIT_DOOR, F1_0);
			#endif
		}
	}

	object_create_explosion( player->segnum, collision_point, i2f(10)/2, VCLIP_PLAYER_HIT );
	if ( Weapon_info[weapon->id].damage_radius )
		explode_badass_weapon(weapon);

	maybe_kill_weapon(weapon,player);

	bump_two_objects(player, weapon, 0);	//no damage from bump

	if ( !Weapon_info[weapon->id].damage_radius ) {
		if ( weapon->ctype.laser_info.parent_num > -1 )
			killer = &Objects[weapon->ctype.laser_info.parent_num];

//		if (weapon->id == SMART_HOMING_ID)
//			damage /= 4;

		if (!(weapon->flags & OF_HARMLESS))
			apply_damage_to_player( player, killer, damage, 1);
	}

	//	Robots become aware of you if you get hit.
	ai_do_cloak_stuff();

	return;
}

//	Nasty robots are the ones that attack you by running into you and doing lots of damage.
void collide_player_and_nasty_robot( object * player, object * robot, vms_vector *collision_point )
{
	digi_link_sound_to_pos( Robot_info[robot->id].claw_sound, player->segnum, 0, collision_point, 0, F1_0 );

	object_create_explosion( player->segnum, collision_point, i2f(10)/2, VCLIP_PLAYER_HIT );

	bump_two_objects(player, robot, 0);	//no damage from bump

	apply_damage_to_player( player, robot, F1_0*(Difficulty_level+1), 0);

	return;
}

void collide_player_and_materialization_center(object *objp)
{
	int	side;
	vms_vector	exit_dir;
	segment	*segp = &Segments[objp->segnum];

	digi_link_sound_to_pos(SOUND_PLAYER_GOT_HIT, objp->segnum, 0, &objp->pos, 0, F1_0);
//	digi_play_sample( SOUND_PLAYER_GOT_HIT, F1_0 );

	object_create_explosion( objp->segnum, &objp->pos, i2f(10)/2, VCLIP_PLAYER_HIT );

	if (objp->id != Player_num)
		return;

	for (side=0; side<MAX_SIDES_PER_SEGMENT; side++)
		if (WALL_IS_DOORWAY(segp, side) & WID_FLY_FLAG) {
			vms_vector	exit_point, rand_vec;

			compute_center_point_on_side(&exit_point, segp, side);
			vm_vec_sub(&exit_dir, &exit_point, &objp->pos);
			vm_vec_normalize_quick(&exit_dir);
			make_random_vector(&rand_vec);
			rand_vec.x /= 4;	rand_vec.y /= 4;	rand_vec.z /= 4;
			vm_vec_add2(&exit_dir, &rand_vec);
			vm_vec_normalize_quick(&exit_dir);
		}

	bump_one_object(objp, &exit_dir, 64*F1_0);

	apply_damage_to_player( objp, NULL, 4*F1_0, 0);

	return;

}

void collide_robot_and_materialization_center(object *objp)
{
	int	side;
	vms_vector	exit_dir;
	segment *segp=&Segments[objp->segnum];

	vm_vec_zero(&exit_dir);
	digi_link_sound_to_pos(SOUND_ROBOT_HIT, objp->segnum, 0, &objp->pos, 0, F1_0);

	if ( Robot_info[objp->id].exp1_vclip_num > -1 )
		object_create_explosion( objp->segnum, &objp->pos, (objp->size/2*3)/4, Robot_info[objp->id].exp1_vclip_num );

	for (side=0; side<MAX_SIDES_PER_SEGMENT; side++)
		if (WALL_IS_DOORWAY(segp, side) & WID_FLY_FLAG) {
			vms_vector	exit_point;

			compute_center_point_on_side(&exit_point, segp, side);
			vm_vec_sub(&exit_dir, &exit_point, &objp->pos);
			vm_vec_normalize_quick(&exit_dir);
		}

	bump_one_object(objp, &exit_dir, 8*F1_0);

	apply_damage_to_robot( objp, F1_0, -1);

	return;

}

//##void collide_player_and_camera( object * player, object * camera, vms_vector *collision_point ) {
//##	return;
//##}

extern int Network_got_powerup; // HACK!!!

void collide_player_and_powerup( object * player, object * powerup, vms_vector *collision_point ) {
	if (!Endlevel_sequence && !Player_is_dead && (player->id == Player_num )) {
		int powerup_used;

		powerup_used = do_powerup(powerup);

		if (powerup_used)	{
			powerup->flags |= OF_SHOULD_BE_DEAD;
			#ifdef NETWORK
			if (Game_mode & GM_MULTI)
				multi_send_remobj(powerup-Objects);
			#endif
		}
	}
#ifndef SHAREWARE
	else if ((Game_mode & GM_MULTI_COOP) && (player->id != Player_num))
	{
		switch (powerup->id) {
			case POW_KEY_BLUE:
				Players[player->id].flags |= PLAYER_FLAGS_BLUE_KEY;
				break;
			case POW_KEY_RED:
				Players[player->id].flags |= PLAYER_FLAGS_RED_KEY;
				break;
			case POW_KEY_GOLD:
				Players[player->id].flags |= PLAYER_FLAGS_GOLD_KEY;
				break;
			default:
				break;
		}
	}
#endif
	return;
}

//##void collide_player_and_debris( object * player, object * debris, vms_vector *collision_point ) {
//##	return;
//##}

void collide_player_and_clutter( object * player, object * clutter, vms_vector *collision_point ) {
	if (check_collision_delayfunc_exec())
		digi_link_sound_to_pos( SOUND_ROBOT_HIT_PLAYER, player->segnum, 0, collision_point, 0, F1_0 );

	bump_two_objects(clutter, player, 1);
	return;
}

//	See if weapon1 creates a badass explosion.  If so, create the explosion
//	Return true if weapon does proximity (as opposed to only contact) damage when it explodes.
int maybe_detonate_weapon(object *weapon1, object *weapon2, vms_vector *collision_point)
{
	if ( Weapon_info[weapon1->id].damage_radius ) {
		fix	dist;

		dist = vm_vec_dist_quick(&weapon1->pos, &weapon2->pos);
		if (dist < F1_0*5) {
			maybe_kill_weapon(weapon1,weapon2);
			if (weapon1->flags & OF_SHOULD_BE_DEAD) {
				explode_badass_weapon(weapon1);
				digi_link_sound_to_pos( Weapon_info[weapon1->id].robot_hit_sound, weapon1->segnum , 0, collision_point, 0, F1_0 );
			}
			return 1;
		} else {
			weapon1->lifeleft = min(dist/64, F1_0);
			return 1;
		}
	} else
		return 0;
}

void collide_weapon_and_weapon( object * weapon1, object * weapon2, vms_vector *collision_point )
{
	if ((Weapon_info[weapon1->id].destroyable) || (Weapon_info[weapon2->id].destroyable)) {
		// shooting Plasma will make bombs explode one drops at the same time since hitboxes overlap. Small HACK to get around this issue. if the player moves away from the bomb at least...
		if ((GameTime64 < weapon1->ctype.laser_info.creation_time + (F1_0/5)) && (GameTime64 < weapon2->ctype.laser_info.creation_time + (F1_0/5)) && (weapon1->ctype.laser_info.parent_num == weapon2->ctype.laser_info.parent_num))
			return;
		//	Bug reported by Adam Q. Pletcher on September 9, 1994, smart bomb homing missiles were toasting each other.
		if ((weapon1->id == weapon2->id) && (weapon1->ctype.laser_info.parent_num == weapon2->ctype.laser_info.parent_num))
			return;

		if (Weapon_info[weapon1->id].destroyable)
			if (maybe_detonate_weapon(weapon1, weapon2, collision_point))
				maybe_kill_weapon(weapon2,weapon1);

		if (Weapon_info[weapon2->id].destroyable)
			if (maybe_detonate_weapon(weapon2, weapon1, collision_point))
				maybe_kill_weapon(weapon1,weapon2);

	}

}

//##void collide_weapon_and_camera( object * weapon, object * camera, vms_vector *collision_point ) {
//##	return;
//##}

//##void collide_weapon_and_powerup( object * weapon, object * powerup, vms_vector *collision_point ) {
//##	return;
//##}

void collide_weapon_and_debris( object * weapon, object * debris, vms_vector *collision_point ) {

	if ( (weapon->ctype.laser_info.parent_type==OBJ_PLAYER) && !(debris->flags & OF_EXPLODING) )	{
		digi_link_sound_to_pos( SOUND_ROBOT_HIT, weapon->segnum , 0, collision_point, 0, F1_0 );

		explode_object(debris,0);
		if ( Weapon_info[weapon->id].damage_radius )
			explode_badass_weapon(weapon);
		maybe_kill_weapon(weapon,debris);
		if (!(weapon->mtype.phys_info.flags & PF_PERSISTENT))
			weapon->flags |= OF_SHOULD_BE_DEAD;
	}
	return;
}

//##void collide_camera_and_camera( object * camera1, object * camera2, vms_vector *collision_point ) {
//##	return;
//##}

//##void collide_camera_and_powerup( object * camera, object * powerup, vms_vector *collision_point ) {
//##	return;
//##}

//##void collide_camera_and_debris( object * camera, object * debris, vms_vector *collision_point ) {
//##	return;
//##}

//##void collide_powerup_and_powerup( object * powerup1, object * powerup2, vms_vector *collision_point ) {
//##	return;
//##}

//##void collide_powerup_and_debris( object * powerup, object * debris, vms_vector *collision_point ) {
//##	return;
//##}

//##void collide_debris_and_debris( object * debris1, object * debris2, vms_vector *collision_point ) {
//##	return;
//##}

#define COLLISION_OF(a,b) (((a)<<8) + (b))

// DPH: Put these macros on one long line to avoid CR/LF problems under Linux.
#define DO_COLLISION(type1,type2,collision_function) case COLLISION_OF( (type1), (type2) ): (collision_function)( (A), (B), collision_point ); break; case COLLISION_OF( (type2), (type1) ): (collision_function)( (B), (A), collision_point ); break;
#define DO_SAME_COLLISION(type1,type2,collision_function) case COLLISION_OF( (type1), (type1) ): (collision_function)( (A), (B), collision_point ); break;

//these next two macros define a case that does nothing
#define NO_COLLISION(type1,type2,collision_function) case COLLISION_OF( (type1), (type2) ): case COLLISION_OF( (type2), (type1) ): break;
#define NO_SAME_COLLISION(type1,type2,collision_function) case COLLISION_OF( (type1), (type1) ): break;

// DPH: These ones are never used so I'm not going to bother.
#ifndef __LINUX__
#define IGNORE_COLLISION(type1,type2,collision_function)                                \
	case COLLISION_OF( (type1), (type2) ):                                          \
		break;                                                                  \
	case COLLISION_OF( (type2), (type1) ):                                          \
		break;

#define ERROR_COLLISION(type1,type2,collision_function)                                 \
	case COLLISION_OF( (type1), (type2) ):                                          \
		Error( "Error in collision type!" );                                    \
		break;                                                                  \
	case COLLISION_OF( (type2), (type1) ):                                          \
		Error( "Error in collision type!" );                                    \
		break;
#endif

void collide_two_objects( object * A, object * B, vms_vector *collision_point )
{
	int collision_type;

	collision_type = COLLISION_OF(A->type,B->type);

	switch( collision_type )	{
	NO_SAME_COLLISION( OBJ_FIREBALL, OBJ_FIREBALL,   collide_fireball_and_fireball )
	DO_SAME_COLLISION( OBJ_ROBOT, OBJ_ROBOT, collide_robot_and_robot )
	NO_SAME_COLLISION( OBJ_HOSTAGE, OBJ_HOSTAGE,  collide_hostage_and_hostage )
	DO_SAME_COLLISION( OBJ_PLAYER, OBJ_PLAYER,  collide_player_and_player )
	DO_SAME_COLLISION( OBJ_WEAPON, OBJ_WEAPON,  collide_weapon_and_weapon )
	NO_SAME_COLLISION( OBJ_CAMERA, OBJ_CAMERA, collide_camera_and_camera )
	NO_SAME_COLLISION( OBJ_POWERUP, OBJ_POWERUP,  collide_powerup_and_powerup )
	NO_SAME_COLLISION( OBJ_DEBRIS, OBJ_DEBRIS,  collide_debris_and_debris )
	NO_COLLISION( OBJ_FIREBALL, OBJ_ROBOT,   collide_fireball_and_robot )
	NO_COLLISION( OBJ_FIREBALL, OBJ_HOSTAGE, collide_fireball_and_hostage )
	NO_COLLISION( OBJ_FIREBALL, OBJ_PLAYER,  collide_fireball_and_player )
	NO_COLLISION( OBJ_FIREBALL, OBJ_WEAPON,  collide_fireball_and_weapon )
	NO_COLLISION( OBJ_FIREBALL, OBJ_CAMERA,  collide_fireball_and_camera )
	NO_COLLISION( OBJ_FIREBALL, OBJ_POWERUP, collide_fireball_and_powerup )
	NO_COLLISION( OBJ_FIREBALL, OBJ_DEBRIS,  collide_fireball_and_debris )
	NO_COLLISION( OBJ_ROBOT, OBJ_HOSTAGE, collide_robot_and_hostage )
	DO_COLLISION( OBJ_ROBOT, OBJ_PLAYER,  collide_robot_and_player )
	DO_COLLISION( OBJ_ROBOT, OBJ_WEAPON,  collide_robot_and_weapon )
	NO_COLLISION( OBJ_ROBOT, OBJ_CAMERA,  collide_robot_and_camera )
	NO_COLLISION( OBJ_ROBOT, OBJ_POWERUP, collide_robot_and_powerup )
	NO_COLLISION( OBJ_ROBOT, OBJ_DEBRIS,  collide_robot_and_debris )
	DO_COLLISION( OBJ_HOSTAGE, OBJ_PLAYER,  collide_hostage_and_player )
	NO_COLLISION( OBJ_HOSTAGE, OBJ_WEAPON,  collide_hostage_and_weapon )
	NO_COLLISION( OBJ_HOSTAGE, OBJ_CAMERA,  collide_hostage_and_camera )
	NO_COLLISION( OBJ_HOSTAGE, OBJ_POWERUP, collide_hostage_and_powerup )
	NO_COLLISION( OBJ_HOSTAGE, OBJ_DEBRIS,  collide_hostage_and_debris )
	DO_COLLISION( OBJ_PLAYER, OBJ_WEAPON,  collide_player_and_weapon )
	NO_COLLISION( OBJ_PLAYER, OBJ_CAMERA,  collide_player_and_camera )
	DO_COLLISION( OBJ_PLAYER, OBJ_POWERUP, collide_player_and_powerup )
	NO_COLLISION( OBJ_PLAYER, OBJ_DEBRIS,  collide_player_and_debris )
	DO_COLLISION( OBJ_PLAYER, OBJ_CNTRLCEN, collide_player_and_controlcen )
	DO_COLLISION( OBJ_PLAYER, OBJ_CLUTTER, collide_player_and_clutter )
	NO_COLLISION( OBJ_WEAPON, OBJ_CAMERA,  collide_weapon_and_camera )
	NO_COLLISION( OBJ_WEAPON, OBJ_POWERUP, collide_weapon_and_powerup )
	DO_COLLISION( OBJ_WEAPON, OBJ_DEBRIS,  collide_weapon_and_debris )
	NO_COLLISION( OBJ_CAMERA, OBJ_POWERUP, collide_camera_and_powerup )
	NO_COLLISION( OBJ_CAMERA, OBJ_DEBRIS,  collide_camera_and_debris )
	NO_COLLISION( OBJ_POWERUP, OBJ_DEBRIS,  collide_powerup_and_debris )
	DO_COLLISION( OBJ_WEAPON, OBJ_CNTRLCEN, collide_weapon_and_controlcen )
	DO_COLLISION( OBJ_ROBOT, OBJ_CNTRLCEN, collide_robot_and_controlcen )
	DO_COLLISION( OBJ_WEAPON, OBJ_CLUTTER, collide_weapon_and_clutter )
	default:
		Int3();	//Error( "Unhandled collision_type in collide.c!\n" );
	}
}

#define ENABLE_COLLISION(type1,type2) CollisionResult[type1][type2] = RESULT_CHECK; CollisionResult[type2][type1] = RESULT_CHECK;
#define DISABLE_COLLISION(type1,type2) CollisionResult[type1][type2] = RESULT_NOTHING; CollisionResult[type2][type1] = RESULT_NOTHING;

void collide_init()	{
	int i, j;

	for (i=0; i < MAX_OBJECT_TYPES; i++ )
		for (j=0; j < MAX_OBJECT_TYPES; j++ )
			CollisionResult[i][j] = RESULT_NOTHING;

	ENABLE_COLLISION( OBJ_WALL, OBJ_ROBOT );
	ENABLE_COLLISION( OBJ_WALL, OBJ_WEAPON );
	ENABLE_COLLISION( OBJ_WALL, OBJ_PLAYER  );
	DISABLE_COLLISION( OBJ_FIREBALL, OBJ_FIREBALL );

	ENABLE_COLLISION( OBJ_ROBOT, OBJ_ROBOT );
//	DISABLE_COLLISION( OBJ_ROBOT, OBJ_ROBOT );	//	ALERT: WARNING: HACK: MK = RESPONSIBLE! TESTING!!
	DISABLE_COLLISION( OBJ_HOSTAGE, OBJ_HOSTAGE );
	ENABLE_COLLISION( OBJ_PLAYER, OBJ_PLAYER );
	ENABLE_COLLISION( OBJ_WEAPON, OBJ_WEAPON );
	DISABLE_COLLISION( OBJ_CAMERA, OBJ_CAMERA );
	DISABLE_COLLISION( OBJ_POWERUP, OBJ_POWERUP );
	DISABLE_COLLISION( OBJ_DEBRIS, OBJ_DEBRIS );
	DISABLE_COLLISION( OBJ_FIREBALL, OBJ_ROBOT );
	DISABLE_COLLISION( OBJ_FIREBALL, OBJ_HOSTAGE );
	DISABLE_COLLISION( OBJ_FIREBALL, OBJ_PLAYER );
	DISABLE_COLLISION( OBJ_FIREBALL, OBJ_WEAPON );
	DISABLE_COLLISION( OBJ_FIREBALL, OBJ_CAMERA );
	DISABLE_COLLISION( OBJ_FIREBALL, OBJ_POWERUP );
	DISABLE_COLLISION( OBJ_FIREBALL, OBJ_DEBRIS );
	DISABLE_COLLISION( OBJ_ROBOT, OBJ_HOSTAGE );
	ENABLE_COLLISION( OBJ_ROBOT, OBJ_PLAYER );
	ENABLE_COLLISION( OBJ_ROBOT, OBJ_WEAPON );
	DISABLE_COLLISION( OBJ_ROBOT, OBJ_CAMERA );
	DISABLE_COLLISION( OBJ_ROBOT, OBJ_POWERUP );
	DISABLE_COLLISION( OBJ_ROBOT, OBJ_DEBRIS );
	ENABLE_COLLISION( OBJ_HOSTAGE, OBJ_PLAYER );
	ENABLE_COLLISION( OBJ_HOSTAGE, OBJ_WEAPON );
	DISABLE_COLLISION( OBJ_HOSTAGE, OBJ_CAMERA );
	DISABLE_COLLISION( OBJ_HOSTAGE, OBJ_POWERUP );
	DISABLE_COLLISION( OBJ_HOSTAGE, OBJ_DEBRIS );
	ENABLE_COLLISION( OBJ_PLAYER, OBJ_WEAPON );
	DISABLE_COLLISION( OBJ_PLAYER, OBJ_CAMERA );
	ENABLE_COLLISION( OBJ_PLAYER, OBJ_POWERUP );
	DISABLE_COLLISION( OBJ_PLAYER, OBJ_DEBRIS );
	DISABLE_COLLISION( OBJ_WEAPON, OBJ_CAMERA );
	DISABLE_COLLISION( OBJ_WEAPON, OBJ_POWERUP );
	ENABLE_COLLISION( OBJ_WEAPON, OBJ_DEBRIS );
	DISABLE_COLLISION( OBJ_CAMERA, OBJ_POWERUP );
	DISABLE_COLLISION( OBJ_CAMERA, OBJ_DEBRIS );
	DISABLE_COLLISION( OBJ_POWERUP, OBJ_DEBRIS );
	ENABLE_COLLISION( OBJ_POWERUP, OBJ_WALL );
	ENABLE_COLLISION( OBJ_WEAPON, OBJ_CNTRLCEN )
	ENABLE_COLLISION( OBJ_WEAPON, OBJ_CLUTTER )
	ENABLE_COLLISION( OBJ_PLAYER, OBJ_CNTRLCEN )
	ENABLE_COLLISION( OBJ_ROBOT, OBJ_CNTRLCEN )
	ENABLE_COLLISION( OBJ_PLAYER, OBJ_CLUTTER )
	ENABLE_COLLISION( OBJ_DEBRIS, OBJ_WALL );
}

void collide_object_with_wall( object * A, fix hitspeed, short hitseg, short hitwall, vms_vector * hitpt )
{

	switch( A->type )	{
	case OBJ_NONE:
		Error( "A object of type NONE hit a wall!\n");
		break;
	case OBJ_PLAYER:		collide_player_and_wall(A,hitspeed,hitseg,hitwall,hitpt); break;
	case OBJ_WEAPON:		collide_weapon_and_wall(A,hitspeed,hitseg,hitwall,hitpt); break;
	case OBJ_DEBRIS:		collide_debris_and_wall(A,hitspeed,hitseg,hitwall,hitpt); break;

	case OBJ_FIREBALL:	break;		//collide_fireball_and_wall(A,hitspeed,hitseg,hitwall,hitpt);
	case OBJ_ROBOT:		collide_robot_and_wall(A,hitspeed,hitseg,hitwall,hitpt); break;
	case OBJ_HOSTAGE:		break;		//collide_hostage_and_wall(A,hitspeed,hitseg,hitwall,hitpt);
	case OBJ_CAMERA:		break;		//collide_camera_and_wall(A,hitspeed,hitseg,hitwall,hitpt);
	case OBJ_POWERUP:		break;		//collide_powerup_and_wall(A,hitspeed,hitseg,hitwall,hitpt);
	case OBJ_GHOST:		break;	//do nothing

	default:
		Error( "Unhandled object type hit wall in collide.c\n" );
	}
}
