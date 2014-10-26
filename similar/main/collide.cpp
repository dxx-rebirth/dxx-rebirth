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
 * Routines to handle collisions
 *
 */

#include <algorithm>
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
#include "render.h"
#include "wall.h"
#include "vclip.h"
#include "polyobj.h"
#include "fireball.h"
#include "laser.h"
#include "dxxerror.h"
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
#include "automap.h"
#include "switch.h"
#include "palette.h"
#include "gameseq.h"
#ifdef EDITOR
#include "editor/editor.h"
#endif
#include "collide.h"
#include "escort.h"

#include "dxxsconf.h"
#include "compiler-integer_sequence.h"
#include "compiler-static_assert.h"
#include "compiler-type_traits.h"

using std::min;

#define WALL_DAMAGE_SCALE (128) // Was 32 before 8:55 am on Thursday, September 15, changed by MK, walls were hurting me more than robots!
#define WALL_DAMAGE_THRESHOLD (F1_0/3)
#define WALL_LOUDNESS_SCALE (20)
#define FORCE_DAMAGE_THRESHOLD (F1_0/3)
#define STANDARD_EXPL_DELAY (F1_0/4)

static int check_collision_delayfunc_exec()
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
static void collide_robot_and_wall( object * robot, fix hitspeed, short hitseg, short hitwall, vms_vector * hitpt)
{
	const ubyte robot_id = get_robot_id(robot);
#if defined(DXX_BUILD_DESCENT_I)
	if ((robot_id == ROBOT_BRAIN) || (robot->ctype.ai_info.behavior == AIB_RUN_FROM))
#elif defined(DXX_BUILD_DESCENT_II)
	const robot_info *robptr = &Robot_info[robot_id];
	if ((robot_id == ROBOT_BRAIN) || (robot->ctype.ai_info.behavior == AIB_RUN_FROM) || (robot_is_companion(robptr) == 1) || (robot->ctype.ai_info.behavior == AIB_SNIPE))
#endif
	{
		auto	wall_num = Segments[hitseg].sides[hitwall].wall_num;
		if (wall_num != wall_none) {
			if ((Walls[wall_num].type == WALL_DOOR) && (Walls[wall_num].keys == KEY_NONE) && (Walls[wall_num].state == WALL_DOOR_CLOSED) && !(Walls[wall_num].flags & WALL_DOOR_LOCKED)) {
				wall_open_door(&Segments[hitseg], hitwall);
			// -- Changed from this, 10/19/95, MK: Don't want buddy getting stranded from player
			//-- } else if ((Robot_info[robot->id].companion == 1) && (Walls[wall_num].type == WALL_DOOR) && (Walls[wall_num].keys != KEY_NONE) && (Walls[wall_num].state == WALL_DOOR_CLOSED) && !(Walls[wall_num].flags & WALL_DOOR_LOCKED)) {
			}
#if defined(DXX_BUILD_DESCENT_II)
			else if ((robot_is_companion(robptr) == 1) && (Walls[wall_num].type == WALL_DOOR)) {
				ai_local		*ailp = &robot->ctype.ai_info.ail;
				if ((ailp->mode == AIM_GOTO_PLAYER) || (Escort_special_goal == ESCORT_GOAL_SCRAM)) {
					if (Walls[wall_num].keys != KEY_NONE) {
						if (Walls[wall_num].keys & Players[Player_num].flags)
							wall_open_door(&Segments[hitseg], hitwall);
					} else if (!(Walls[wall_num].flags & WALL_DOOR_LOCKED))
						wall_open_door(&Segments[hitseg], hitwall);
				}
			} else if (Robot_info[get_robot_id(robot)].thief) {		//	Thief allowed to go through doors to which player has key.
				if (Walls[wall_num].keys != KEY_NONE)
					if (Walls[wall_num].keys & Players[Player_num].flags)
						wall_open_door(&Segments[hitseg], hitwall);
			}
#endif
		}
	}

	return;
}

//	-------------------------------------------------------------------------------------------------------------

static int apply_damage_to_clutter(vobjptridx_t clutter, fix damage)
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
static void apply_force_damage(vobjptridx_t obj,fix force,vobjptridx_t other_obj)
{
	int	result;
	fix damage;

	if (obj->flags & (OF_EXPLODING|OF_SHOULD_BE_DEAD))
		return;		//already exploding or dead

	damage = fixdiv(force,obj->mtype.phys_info.mass) / 8;

#if defined(DXX_BUILD_DESCENT_II)
	if ((other_obj->type == OBJ_PLAYER) && cheats.monsterdamage)
		damage = 0x7fffffff;
#endif

	if (damage < FORCE_DAMAGE_THRESHOLD)
		return;

	switch (obj->type) {

		case OBJ_ROBOT:
		{
			const robot_info *robptr = &Robot_info[get_robot_id(obj)];
			if (robptr->attack_type == 1) {
				if (other_obj->type == OBJ_WEAPON)
					result = apply_damage_to_robot(obj,damage/4, other_obj->ctype.laser_info.parent_num);
				else
					result = apply_damage_to_robot(obj,damage/4, other_obj);
			}
			else {
				if (other_obj->type == OBJ_WEAPON)
					result = apply_damage_to_robot(obj,damage/2, other_obj->ctype.laser_info.parent_num);
				else
					result = apply_damage_to_robot(obj,damage/2, other_obj);
			}

			if (result && (other_obj->ctype.laser_info.parent_signature == ConsoleObject->signature))
				add_points_to_score(robptr->score_value);
			break;
		}

		case OBJ_PLAYER:

			//	If colliding with a claw type robot, do damage proportional to FrameTime because you can collide with those
			//	bots every frame since they don't move.
			if ( (other_obj->type == OBJ_ROBOT) && (Robot_info[get_robot_id(other_obj)].attack_type) )
				damage = fixmul(damage, FrameTime*2);

#if defined(DXX_BUILD_DESCENT_II)
			//	Make trainee easier.
			if (Difficulty_level == 0)
				damage /= 2;
#endif

			apply_damage_to_player(obj,other_obj,damage,0);
			break;

		case OBJ_CLUTTER:

			apply_damage_to_clutter(obj,damage);
			break;

		case OBJ_CNTRLCEN: // Never hits! Reactor does not have MT_PHYSICS - it's stationary! So no force damage here.

			apply_damage_to_controlcen(obj,damage, other_obj);
			break;

		case OBJ_WEAPON:

			break; //weapons don't take damage

		default:

			Int3();

	}
}

//	-----------------------------------------------------------------------------
static void bump_this_object(vobjptridx_t objp, vobjptridx_t other_objp, vms_vector *force, int damage_flag)
{
	fix force_mag;

	if (! (objp->mtype.phys_info.flags & PF_PERSISTENT))
	{
		if (objp->type == OBJ_PLAYER) {
			vms_vector force2;
			force2.x = force->x/4;
			force2.y = force->y/4;
			force2.z = force->z/4;
			phys_apply_force(objp,&force2);
			if (damage_flag && ((other_objp->type != OBJ_ROBOT) || !robot_is_companion(&Robot_info[get_robot_id(other_objp)])))
			{
				force_mag = vm_vec_mag_quick(force2);
				apply_force_damage(objp, force_mag, other_objp);
			}
		} else if ((objp->type == OBJ_ROBOT) || (objp->type == OBJ_CLUTTER) || (objp->type == OBJ_CNTRLCEN)) {
			if (!Robot_info[get_robot_id(objp)].boss_flag) {
				vms_vector force2;
				force2.x = force->x/(4 + Difficulty_level);
				force2.y = force->y/(4 + Difficulty_level);
				force2.z = force->z/(4 + Difficulty_level);

				phys_apply_force(objp, force);
				phys_apply_rot(objp, &force2);
				if (damage_flag) {
					force_mag = vm_vec_mag_quick(*force);
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
static void bump_two_objects(vobjptridx_t obj0,vobjptridx_t obj1,int damage_flag)
{
	vms_vector	force;
	object		*t=NULL;

	vm_vec_zero(force);

	if (obj0->movement_type != MT_PHYSICS)
		t=obj1;
	else if (obj1->movement_type != MT_PHYSICS)
		t=obj0;

	if (t) {
		Assert(t->movement_type == MT_PHYSICS);
		vm_vec_copy_scale(force,t->mtype.phys_info.velocity,-t->mtype.phys_info.mass);
		phys_apply_force(t,&force);
		return;
	}

	vm_vec_sub(force,obj0->mtype.phys_info.velocity,obj1->mtype.phys_info.velocity);
	vm_vec_scale2(force,2*fixmul(obj0->mtype.phys_info.mass,obj1->mtype.phys_info.mass),(obj0->mtype.phys_info.mass+obj1->mtype.phys_info.mass));

	bump_this_object(obj1, obj0, &force, damage_flag);
	vm_vec_negate(force);
	bump_this_object(obj0, obj1, &force, damage_flag);

}

void bump_one_object(object *obj0, vms_vector *hit_dir, fix damage)
{
	vms_vector	hit_vec;

	hit_vec = *hit_dir;
	vm_vec_scale(hit_vec, damage);

	phys_apply_force(obj0,&hit_vec);

}

static void collide_player_and_wall(vobjptridx_t playerobj, fix hitspeed, segnum_t hitseg, short hitwall, const vms_vector *hitpt)
{
	fix damage;

	if (get_player_id(playerobj) != Player_num) // Execute only for local player
		return;

	int tmap_num = Segments[hitseg].sides[hitwall].tmap_num;

	//	If this wall does damage, don't make *BONK* sound, we'll be making another sound.
	if (TmapInfo[tmap_num].damage > 0)
		return;

#if defined(DXX_BUILD_DESCENT_I)
	const int ForceFieldHit = 0;
#elif defined(DXX_BUILD_DESCENT_II)
	int ForceFieldHit=0;
	if (TmapInfo[tmap_num].flags & TMI_FORCE_FIELD) {
		vms_vector force;

		PALETTE_FLASH_ADD(0, 0, 60);	//flash blue

		//knock player around
		force.x = 40*(d_rand() - 16384);
		force.y = 40*(d_rand() - 16384);
		force.z = 40*(d_rand() - 16384);
		phys_apply_rot(playerobj, &force);

		//make sound
		digi_link_sound_to_pos( SOUND_FORCEFIELD_BOUNCE_PLAYER, hitseg, 0, *hitpt, 0, f1_0 );
		if (Game_mode & GM_MULTI)
			multi_send_play_sound(SOUND_FORCEFIELD_BOUNCE_PLAYER, f1_0);
		ForceFieldHit=1;
	}
	else
#endif
	{
		wall_hit_process( &Segments[hitseg], hitwall, 20, get_player_id(playerobj), playerobj );
	}

	//	** Damage from hitting wall **
	//	If the player has less than 10% shields, don't take damage from bump
#if defined(DXX_BUILD_DESCENT_I)
	damage = hitspeed / WALL_DAMAGE_SCALE;
#elif defined(DXX_BUILD_DESCENT_II)
	// Note: Does quad damage if hit a force field - JL
	damage = (hitspeed / WALL_DAMAGE_SCALE) * (ForceFieldHit*8 + 1);

	int tmap_num2 = Segments[hitseg].sides[hitwall].tmap_num2;

	//don't do wall damage and sound if hit lava or water
	if ((TmapInfo[tmap_num].flags & (TMI_WATER|TMI_VOLATILE)) || (tmap_num2 && (TmapInfo[tmap_num2&0x3fff].flags & (TMI_WATER|TMI_VOLATILE))))
		damage = 0;
#endif

	if (damage >= WALL_DAMAGE_THRESHOLD) {
		int	volume;
		volume = (hitspeed-(WALL_DAMAGE_SCALE*WALL_DAMAGE_THRESHOLD)) / WALL_LOUDNESS_SCALE ;

		create_awareness_event(playerobj, PA_WEAPON_WALL_COLLISION);

		if ( volume > F1_0 )
			volume = F1_0;
		if (volume > 0 && !ForceFieldHit) {  // uhhhgly hack
			digi_link_sound_to_pos( SOUND_PLAYER_HIT_WALL, hitseg, 0, *hitpt, 0, volume );
			if (Game_mode & GM_MULTI)
				multi_send_play_sound(SOUND_PLAYER_HIT_WALL, volume);
		}

		if (!(Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE))
			if ( Players[Player_num].shields > f1_0*10 || ForceFieldHit)
			  	apply_damage_to_player( playerobj, playerobj, damage, 0 );

		// -- No point in doing this unless we compute a reasonable hitpt.  Currently it is just the player's position. --MK, 01/18/96
		// -- if (!(TmapInfo[Segments[hitseg].sides[hitwall].tmap_num].flags & TMI_FORCE_FIELD)) {
		// -- 	vms_vector	hitpt1;
		// -- 	int			hitseg1;
		// --
		// -- 		vm_vec_avg(&hitpt1, hitpt, &Objects[Players[Player_num].objnum].pos);
		// -- 	hitseg1 = find_point_seg(&hitpt1, Objects[Players[Player_num].objnum].segnum);
		// -- 	if (hitseg1 != -1)
		// -- 		object_create_explosion( hitseg, hitpt, Weapon_info[0].impact_size, Weapon_info[0].wall_hit_vclip );
		// -- }

	}

	return;
}

static fix64	Last_volatile_scrape_sound_time = 0;

#if defined(DXX_BUILD_DESCENT_I)
//this gets called when an object is scraping along the wall
void scrape_player_on_wall(vobjptridx_t obj, segnum_t hitseg, short hitside, const vms_vector &hitpt)
{
	fix d;

	if (obj->type != OBJ_PLAYER || get_player_id(obj) != Player_num)
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
			if (Game_mode & GM_MULTI)
				multi_send_play_sound(SOUND_VOLATILE_WALL_HISS, F1_0);
		}
		hit_dir = Segments[hitseg].sides[hitside].normals[0];
		make_random_vector(rand_vec);
		vm_vec_scale_add2(hit_dir, rand_vec, F1_0/8);
		vm_vec_normalize_quick(hit_dir);
		bump_one_object(obj, &hit_dir, F1_0*8);

		obj->mtype.phys_info.rotvel.x = (d_rand() - 16384)/2;
		obj->mtype.phys_info.rotvel.z = (d_rand() - 16384)/2;

	}
}
#elif defined(DXX_BUILD_DESCENT_II)
//see if wall is volatile or water
//if volatile, cause damage to player
//returns 1=lava, 2=water
int check_volatile_wall(object *obj,segnum_t segnum,int sidenum)
{
	fix tmap_num,d,water;

	Assert(obj->type==OBJ_PLAYER);

	tmap_num = Segments[segnum].sides[sidenum].tmap_num;

	d = TmapInfo[tmap_num].damage;
	water = (TmapInfo[tmap_num].flags & TMI_WATER);

	if (d > 0 || water) {

		if (get_player_id(obj) == Player_num) {

			if (d > 0) {
				fix damage = fixmul(d,FrameTime);

				if (Difficulty_level == 0)
					damage /= 2;

				if (!(Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE))
					apply_damage_to_player( obj, obj, damage, 0 );

				PALETTE_FLASH_ADD(f2i(damage*4), 0, 0);	//flash red
			}

			obj->mtype.phys_info.rotvel.x = (d_rand() - 16384)/2;
			obj->mtype.phys_info.rotvel.z = (d_rand() - 16384)/2;
		}

		return (d>0)?1:2;
	}
	else
	 {
		return 0;
	 }
}

//this gets called when an object is scraping along the wall
void scrape_player_on_wall(vobjptridx_t obj, segnum_t hitseg, short hitside, const vms_vector &hitpt)
{
	int type;

	if (obj->type != OBJ_PLAYER || get_player_id(obj) != Player_num)
		return;

	if ((type=check_volatile_wall(obj,hitseg,hitside))!=0) {
		vms_vector	hit_dir, rand_vec;

		if ((GameTime64 > Last_volatile_scrape_sound_time + F1_0/4) || (GameTime64 < Last_volatile_scrape_sound_time)) {
			int sound = (type==1)?SOUND_VOLATILE_WALL_HISS:SOUND_SHIP_IN_WATER;

			Last_volatile_scrape_sound_time = GameTime64;

			digi_link_sound_to_pos( sound, hitseg, 0, hitpt, 0, F1_0 );
			if (Game_mode & GM_MULTI)
				multi_send_play_sound(sound, F1_0);
		}

			hit_dir = Segments[hitseg].sides[hitside].normals[0];

		make_random_vector(rand_vec);
		vm_vec_scale_add2(hit_dir, rand_vec, F1_0/8);
		vm_vec_normalize_quick(hit_dir);
		bump_one_object(obj, &hit_dir, F1_0*8);
	}
}

static int effect_parent_is_guidebot(const object *effect)
{
	if (effect->ctype.laser_info.parent_type != OBJ_ROBOT)
		return 0;
	const object *robot = &Objects[effect->ctype.laser_info.parent_num];
	if (robot->signature != effect->ctype.laser_info.parent_signature)
		/* parent replaced, no idea what it once was */
		return 0;
	const ubyte robot_id = get_robot_id(robot);
	const robot_info *robptr = &Robot_info[robot_id];
	return robot_is_companion(robptr);
}
#endif

//if an effect is hit, and it can blow up, then blow it up
//returns true if it blew up
int check_effect_blowup(segment *seg,int side,vms_vector *pnt, object *blower, int force_blowup_flag, int remote)
{
	int tm,db;

#if defined(DXX_BUILD_DESCENT_I)
	(void)blower;
	force_blowup_flag = 0;
	(void)remote;
#elif defined(DXX_BUILD_DESCENT_II)
	int trigger_check = 0, is_trigger = 0;
	auto wall_num = seg->sides[side].wall_num;
	db=0;

	// If this wall has a trigger and the blower-upper is not the player or the buddy, abort!
	trigger_check = (!(effect_parent_is_guidebot(blower) || blower->ctype.laser_info.parent_type == OBJ_PLAYER));
	// For Multiplayer perform an additional check to see if it's a local-player hit. If a remote player hits, a packet is expected (remote 1) which would be followed by MULTI_TRIGGER to ensure sync with the switch and the actual trigger.
	if (Game_mode & GM_MULTI)
		trigger_check = (!(blower->ctype.laser_info.parent_type == OBJ_PLAYER && (blower->ctype.laser_info.parent_num == Players[Player_num].objnum || remote)));
	if ( wall_num != wall_none )
		if (Walls[wall_num].trigger != trigger_none)
			is_trigger = 1;
	if (trigger_check && is_trigger)
		return(0);
#endif

	if ((tm=seg->sides[side].tmap_num2) != 0) {
		int tmf = tm&0xc000;		//tm flags
		tm &= 0x3fff;			//tm without flags

		const int ec = TmapInfo[tm].eclip_num;
#if defined(DXX_BUILD_DESCENT_I)
   		if (ec!=-1 && (db=Effects[ec].dest_bm_num)!=-1 && !(Effects[ec].flags&EF_ONE_SHOT))
#elif defined(DXX_BUILD_DESCENT_II)
		//check if it's an animation (monitor) or casts light
		if ((ec!=-1 && ((db=Effects[ec].dest_bm_num)!=-1 && !(Effects[ec].flags&EF_ONE_SHOT))) ||	(ec==-1 && (TmapInfo[tm].destroyed!=-1)))
#endif
		{
			fix u,v;
			grs_bitmap *bm = &GameBitmaps[Textures[tm].index];
			int x=0,y=0,t;

			PIGGY_PAGE_IN(Textures[tm]);

			//this can be blown up...did we hit it?

			if (!force_blowup_flag) {
				find_hitpoint_uv(&u,&v,pnt,seg,side,0);	//evil: always say face zero

				x = ((unsigned) f2i(u*bm->bm_w)) % bm->bm_w;
				y = ((unsigned) f2i(v*bm->bm_h)) % bm->bm_h;

				switch (tmf) {		//adjust for orientation of paste-on
					case 0x0000:	break;
					case 0x4000:	t=y; y=x; x=bm->bm_w-t-1; break;
					case 0x8000:	y=bm->bm_h-y-1; x=bm->bm_w-x-1; break;
					case 0xc000:	t=x; x=y; y=bm->bm_h-t-1; break;
				}

				if (bm->bm_flags & BM_FLAG_RLE)
					bm = rle_expand_texture(bm);
			}

#if defined(DXX_BUILD_DESCENT_I)
			if (gr_gpixel (bm, x, y) != 255)
#elif defined(DXX_BUILD_DESCENT_II)
			if (force_blowup_flag || (bm->bm_data[y*bm->bm_w+x] != TRANSPARENCY_COLOR))
#endif
			{		//not trans, thus on effect
				int vc,sound_num;

#if defined(DXX_BUILD_DESCENT_II)
				if ((Game_mode & GM_MULTI) && Netgame.AlwaysLighting)
			   	if (!(ec!=-1 && db!=-1 && !(Effects[ec].flags&EF_ONE_SHOT)))
				   	return(0);

				//note: this must get called before the texture changes,
				//because we use the light value of the texture to change
				//the static light in the segment
				subtract_light(seg-Segments,side);

				// we blew up something connected to a trigger. Send it to others!
				if ((Game_mode & GM_MULTI) && is_trigger && !remote && !force_blowup_flag)
					multi_send_effect_blowup(seg-Segments, side, pnt);
#endif
				if (Newdemo_state == ND_STATE_RECORDING)
					newdemo_record_effect_blowup( seg-Segments, side, pnt);

				fix dest_size;
				if (ec!=-1) {
					dest_size = Effects[ec].dest_size;
					vc = Effects[ec].dest_vclip;
				}
				else
				{
					dest_size = i2f(20);
					vc = 3;
				}

				object_create_explosion( seg-Segments, *pnt, dest_size, vc );

#if defined(DXX_BUILD_DESCENT_II)
				if (ec!=-1 && db!=-1 && !(Effects[ec].flags&EF_ONE_SHOT))
#endif
				{

					if ((sound_num = Vclip[vc].sound_num) != -1)
		  				digi_link_sound_to_pos( sound_num, seg-Segments, 0, *pnt,  0, F1_0 );

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
						seg->sides[side].tmap_num2 = bm_num | tmf;		//replace with destoyed

					}
					else {
						Assert(db!=0 && seg->sides[side].tmap_num2!=0);
						seg->sides[side].tmap_num2 = db | tmf;		//replace with destoyed
					}
				}
#if defined(DXX_BUILD_DESCENT_II)
				else {
					seg->sides[side].tmap_num2 = TmapInfo[tm].destroyed | tmf;

					//assume this is a light, and play light sound
		  			digi_link_sound_to_pos( SOUND_LIGHT_BLOWNUP, seg-Segments, 0, *pnt,  0, F1_0 );
				}
#endif

				return 1;		//blew up!
			}
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

static void collide_weapon_and_wall(vobjptridx_t weapon, fix hitspeed, segnum_t hitseg, short hitwall, vms_vector * hitpt)
{
	segment *seg = &Segments[hitseg];
	int blew_up;
	int wall_type;
	int playernum;

#if defined(DXX_BUILD_DESCENT_I)
	if (weapon->mtype.phys_info.flags & PF_BOUNCE)
		return;
#elif defined(DXX_BUILD_DESCENT_II)
	if (get_weapon_id(weapon) == OMEGA_ID)
		if (!ok_to_do_omega_damage(weapon)) // see comment in laser.c
			return;

	//	If this is a guided missile and it strikes fairly directly, clear bounce flag.
	if (get_weapon_id(weapon) == GUIDEDMISS_ID) {
		fix	dot;

		dot = vm_vec_dot(weapon->orient.fvec, Segments[hitseg].sides[hitwall].normals[0]);
		if (dot < -F1_0/6) {
			weapon->mtype.phys_info.flags &= ~PF_BOUNCE;
		}
	}

	//if an energy weapon hits a forcefield, let it bounce
	if ((TmapInfo[seg->sides[hitwall].tmap_num].flags & TMI_FORCE_FIELD) &&
		 !(weapon->type == OBJ_WEAPON && Weapon_info[get_weapon_id(weapon)].energy_usage==0)) {

		//make sound
		digi_link_sound_to_pos( SOUND_FORCEFIELD_BOUNCE_WEAPON, hitseg, 0, *hitpt, 0, f1_0 );
		if (Game_mode & GM_MULTI)
			multi_send_play_sound(SOUND_FORCEFIELD_BOUNCE_WEAPON, f1_0);

		return;	//bail here. physics code will bounce this object
	}

	#ifndef NDEBUG
	if (keyd_pressed[KEY_LAPOSTRO])
		if (weapon->ctype.laser_info.parent_num == Players[Player_num].objnum) {
			//	MK: Real pain when you need to know a seg:side and you've got quad lasers.
			HUD_init_message(HM_DEFAULT, "Hit at segment = %i, side = %i", hitseg, hitwall);
			if (get_weapon_id(weapon) < 4)
				subtract_light(hitseg, hitwall);
			else if (get_weapon_id(weapon) == FLARE_ID)
				add_light(hitseg, hitwall);
		}

		//@@#ifdef EDITOR
		//@@Cursegp = &Segments[hitseg];
		//@@Curside = hitwall;
		//@@#endif
	#endif
#endif

	if ((weapon->mtype.phys_info.velocity.x == 0) && (weapon->mtype.phys_info.velocity.y == 0) && (weapon->mtype.phys_info.velocity.z == 0)) {
		Int3();	//	Contact Matt: This is impossible.  A weapon with 0 velocity hit a wall, which doesn't move.
		return;
	}

	blew_up = check_effect_blowup(seg,hitwall,hitpt, weapon, 0, 0);

	//if ((seg->sides[hitwall].tmap_num2==0) && (TmapInfo[seg->sides[hitwall].tmap_num].flags & TMI_VOLATILE)) {

	int	robot_escort;
#if defined(DXX_BUILD_DESCENT_II)
	robot_escort = effect_parent_is_guidebot(weapon);
	if (robot_escort) {

		if (Game_mode & GM_MULTI)
		 {
			 Int3();  // Get Jason!
		    return;
	    }


		playernum = Player_num;		//if single player, he's the player's buddy
	}
	else
#endif
	{
		robot_escort = 0;

		if (Objects[weapon->ctype.laser_info.parent_num].type == OBJ_PLAYER)
			playernum = get_player_id(&Objects[weapon->ctype.laser_info.parent_num]);
		else
			playernum = -1;		//not a player (thus a robot)
	}

#if defined(DXX_BUILD_DESCENT_II)
	if (blew_up) {		//could be a wall switch
		//for wall triggers, always say that the player shot it out.  This is
		//because robots can shoot out wall triggers, and so the trigger better
		//take effect
		//	NO -- Changed by MK, 10/18/95.  We don't want robots blowing puzzles.  Only player or buddy can open!
		check_trigger(seg,hitwall,weapon->ctype.laser_info.parent_num,1);
	}

	if (get_weapon_id(weapon) == EARTHSHAKER_ID)
		smega_rock_stuff();
#endif

	wall_type = wall_hit_process( seg, hitwall, weapon->shields, playernum, weapon );

	// Wall is volatile if either tmap 1 or 2 is volatile
	if ((TmapInfo[seg->sides[hitwall].tmap_num].flags & TMI_VOLATILE) || (seg->sides[hitwall].tmap_num2 && (TmapInfo[seg->sides[hitwall].tmap_num2&0x3fff].flags & TMI_VOLATILE))) {
		weapon_info *wi = &Weapon_info[get_weapon_id(weapon)];

		//we've hit a volatile wall

		digi_link_sound_to_pos( SOUND_VOLATILE_WALL_HIT,hitseg, 0, *hitpt, 0, F1_0 );

		int vclip;
#if defined(DXX_BUILD_DESCENT_I)
		vclip = VCLIP_VOLATILE_WALL_HIT;
#elif defined(DXX_BUILD_DESCENT_II)
		//for most weapons, use volatile wall hit.  For mega, use its special vclip
		vclip = (get_weapon_id(weapon) == MEGA_ID)?wi->robot_hit_vclip:VCLIP_VOLATILE_WALL_HIT;

		//	New by MK: If powerful badass, explode as badass, not due to lava, fixes megas being wimpy in lava.
		if (wi->damage_radius >= VOLATILE_WALL_DAMAGE_RADIUS/2) {
			explode_badass_weapon(weapon,*hitpt);
		} else
#endif
		{
			object_create_badass_explosion( weapon, hitseg, *hitpt,
				wi->impact_size + VOLATILE_WALL_IMPACT_SIZE,
				vclip,
				wi->strength[Difficulty_level]/4+VOLATILE_WALL_EXPL_STRENGTH,	//	diminished by mk on 12/08/94, i was doing 70 damage hitting lava on lvl 1.
				wi->damage_radius+VOLATILE_WALL_DAMAGE_RADIUS,
				wi->strength[Difficulty_level]/2+VOLATILE_WALL_DAMAGE_FORCE,
				weapon->ctype.laser_info.parent_num );
		}

		weapon->flags |= OF_SHOULD_BE_DEAD;		//make flares die in lava

	}
#if defined(DXX_BUILD_DESCENT_II)
	else if ((TmapInfo[seg->sides[hitwall].tmap_num].flags & TMI_WATER) || (seg->sides[hitwall].tmap_num2 && (TmapInfo[seg->sides[hitwall].tmap_num2&0x3fff].flags & TMI_WATER))) {
		weapon_info *wi = &Weapon_info[get_weapon_id(weapon)];

		//we've hit water

		//	MK: 09/13/95: Badass in water is 1/2 normal intensity.
		if ( Weapon_info[get_weapon_id(weapon)].matter ) {

			digi_link_sound_to_pos( SOUND_MISSILE_HIT_WATER,hitseg, 0, *hitpt, 0, F1_0 );

			if ( Weapon_info[get_weapon_id(weapon)].damage_radius ) {

				digi_link_sound_to_object(SOUND_BADASS_EXPLOSION, weapon, 0, F1_0);

				//	MK: 09/13/95: Badass in water is 1/2 normal intensity.
				object_create_badass_explosion( weapon, hitseg, *hitpt,
					wi->impact_size/2,
					wi->robot_hit_vclip,
					wi->strength[Difficulty_level]/4,
					wi->damage_radius,
					wi->strength[Difficulty_level]/2,
					weapon->ctype.laser_info.parent_num );
			}
			else
				object_create_explosion( weapon->segnum, weapon->pos, Weapon_info[get_weapon_id(weapon)].impact_size, Weapon_info[get_weapon_id(weapon)].wall_hit_vclip );

		} else {
			digi_link_sound_to_pos( SOUND_LASER_HIT_WATER,hitseg, 0, *hitpt, 0, F1_0 );
			object_create_explosion( weapon->segnum, weapon->pos, Weapon_info[get_weapon_id(weapon)].impact_size, VCLIP_WATER_HIT );
		}

		weapon->flags |= OF_SHOULD_BE_DEAD;		//make flares die in water

	}
#endif
	else {

#if defined(DXX_BUILD_DESCENT_II)
		if (weapon->mtype.phys_info.flags & PF_BOUNCE) {

			//do special bound sound & effect

		}
		else
#endif
		{

			//if it's not the player's weapon, or it is the player's and there
			//is no wall, and no blowing up monitor, then play sound
			if ((weapon->ctype.laser_info.parent_type != OBJ_PLAYER) ||	((seg->sides[hitwall].wall_num == wall_none || wall_type==WHP_NOT_SPECIAL) && !blew_up))
				if ((Weapon_info[get_weapon_id(weapon)].wall_hit_sound > -1 ) && (!(weapon->flags & OF_SILENT)))
				digi_link_sound_to_pos( Weapon_info[get_weapon_id(weapon)].wall_hit_sound,weapon->segnum, 0, weapon->pos, 0, F1_0 );

			if ( Weapon_info[get_weapon_id(weapon)].wall_hit_vclip > -1 )	{
				if ( Weapon_info[get_weapon_id(weapon)].damage_radius )
#if defined(DXX_BUILD_DESCENT_I)
					explode_badass_weapon(weapon, weapon->pos);
#elif defined(DXX_BUILD_DESCENT_II)
					explode_badass_weapon(weapon, *hitpt);
#endif
				else
					object_create_explosion( weapon->segnum, weapon->pos, Weapon_info[get_weapon_id(weapon)].impact_size, Weapon_info[get_weapon_id(weapon)].wall_hit_vclip );
			}
		}
	}

	//	If weapon fired by player or companion...
	if (( weapon->ctype.laser_info.parent_type== OBJ_PLAYER ) || robot_escort) {

		if (!(weapon->flags & OF_SILENT) && (weapon->ctype.laser_info.parent_num == Players[Player_num].objnum))
			create_awareness_event(weapon, PA_WEAPON_WALL_COLLISION);			// object "weapon" can attract attention to player

//		if (weapon->id != FLARE_ID) {
//	We now allow flares to open doors.
		{
#if defined(DXX_BUILD_DESCENT_I)
			if (get_weapon_id(weapon) != FLARE_ID)
				weapon->flags |= OF_SHOULD_BE_DEAD;
#elif defined(DXX_BUILD_DESCENT_II)
			if (((get_weapon_id(weapon) != FLARE_ID) || (weapon->ctype.laser_info.parent_type != OBJ_PLAYER)) && !(weapon->mtype.phys_info.flags & PF_BOUNCE))
				weapon->flags |= OF_SHOULD_BE_DEAD;

			//don't let flares stick in force fields
			if ((get_weapon_id(weapon) == FLARE_ID) && (TmapInfo[seg->sides[hitwall].tmap_num].flags & TMI_FORCE_FIELD))
				weapon->flags |= OF_SHOULD_BE_DEAD;
#endif

			if (!(weapon->flags & OF_SILENT)) {
				switch (wall_type) {

					case WHP_NOT_SPECIAL:
						//should be handled above
						//digi_link_sound_to_pos( Weapon_info[weapon->id].wall_hit_sound, weapon->segnum, 0, &weapon->pos, 0, F1_0 );
						break;

					case WHP_NO_KEY:
						//play special hit door sound (if/when we get it)
						digi_link_sound_to_pos( SOUND_WEAPON_HIT_DOOR, weapon->segnum, 0, weapon->pos, 0, F1_0 );
			         if (Game_mode & GM_MULTI)
							multi_send_play_sound( SOUND_WEAPON_HIT_DOOR, F1_0 );

						break;

					case WHP_BLASTABLE:
						//play special blastable wall sound (if/when we get it)
#if defined(DXX_BUILD_DESCENT_II)
						if ((Weapon_info[get_weapon_id(weapon)].wall_hit_sound > -1 ) && (!(weapon->flags & OF_SILENT)))
#endif
							digi_link_sound_to_pos( SOUND_WEAPON_HIT_BLASTABLE, weapon->segnum, 0, weapon->pos, 0, F1_0 );
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
#if defined(DXX_BUILD_DESCENT_II)
		if (!(weapon->mtype.phys_info.flags & PF_BOUNCE))
#endif
			weapon->flags |= OF_SHOULD_BE_DEAD;
	}

	return;
}

static void collide_debris_and_wall(vobjptridx_t debris, fix hitspeed, segnum_t hitseg, short hitwall, vms_vector * hitpt)
{
	if (!PERSISTENT_DEBRIS || TmapInfo[Segments[hitseg].sides[hitwall].tmap_num].damage)
		explode_object(debris,0);
	return;
}

//	-------------------------------------------------------------------------------------------------------------------
static void collide_robot_and_robot(vobjptridx_t robot1, vobjptridx_t robot2, vms_vector *collision_point)
{
	bump_two_objects(robot1, robot2, 1);
	return;
}

static void collide_robot_and_controlcen( object * obj1, object * obj2, vms_vector *collision_point )
{
	if (obj1->type == OBJ_ROBOT) {
		std::swap(obj1, obj2);
	}
	vms_vector	hitvec;
	vm_vec_normalize(vm_vec_sub(hitvec, obj1->pos, obj2->pos));
	bump_one_object(obj2, &hitvec, 0);
}

static void collide_robot_and_player(vobjptridx_t robot, vobjptridx_t playerobj, vms_vector *collision_point)
{
#if defined(DXX_BUILD_DESCENT_II)
	int	steal_attempt = 0;

	if (robot->flags&OF_EXPLODING)
		return;
#endif

	if (get_player_id(playerobj) == Player_num) {
#if defined(DXX_BUILD_DESCENT_II)
		const robot_info *robptr = &Robot_info[get_robot_id(robot)];
		if (robot_is_companion(robptr))	//	Player and companion don't collide.
			return;
		if (robptr->kamikaze) {
			apply_damage_to_robot(robot, robot->shields+1, playerobj);
			if (playerobj == ConsoleObject)
				add_points_to_score(robptr->score_value);
		}

		if (robot_is_thief(robptr)) {
			static fix64 Last_thief_hit_time;
			ai_local		*ailp = &robot->ctype.ai_info.ail;
			if (ailp->mode == AIM_THIEF_ATTACK) {
				Last_thief_hit_time = GameTime64;
				attempt_to_steal_item(robot, get_player_id(playerobj));
				steal_attempt = 1;
			} else if (GameTime64 - Last_thief_hit_time < F1_0*2)
				return;		//	ZOUNDS!  BRILLIANT!  Thief not collide with player if not stealing!
								// NO!  VERY DUMB!  makes thief look very stupid if player hits him while cloaked! -AP
			else
				Last_thief_hit_time = GameTime64;
		}
#endif

		create_awareness_event(playerobj, PA_PLAYER_COLLISION);			// object robot can attract attention to player
		do_ai_robot_hit_attack(robot, playerobj, collision_point);
		do_ai_robot_hit(robot, PA_WEAPON_ROBOT_COLLISION);
	}
	else
	{
		multi_robot_request_change(robot, get_player_id(playerobj));
		return; // only controlling player should make damage otherwise we might juggle robot back and forth, killing it instantly
	}

	if (check_collision_delayfunc_exec())
	{
		auto collision_seg = find_point_seg(*collision_point, playerobj->segnum);

#if defined(DXX_BUILD_DESCENT_II)
		// added this if to remove the bump sound if it's the thief.
		// A "steal" sound was added and it was getting obscured by the bump. -AP 10/3/95
		//	Changed by MK to make this sound unless the robot stole.
		if ((!steal_attempt) && !Robot_info[get_robot_id(robot)].energy_drain)
#endif
			digi_link_sound_to_pos( SOUND_ROBOT_HIT_PLAYER, playerobj->segnum, 0, *collision_point, 0, F1_0 );

		if (collision_seg != segment_none)
			object_create_explosion( collision_seg, *collision_point, Weapon_info[0].impact_size, Weapon_info[0].wall_hit_vclip );
	}

	bump_two_objects(robot, playerobj, 1);
	return;
}

// Provide a way for network message to instantly destroy the control center
// without awarding points or anything.

//	if controlcen == NULL, that means don't do the explosion because the control center
//	was actually in another object.
void net_destroy_controlcen(objptridx_t controlcen)
{
	if (Control_center_destroyed != 1) {
		do_controlcen_destroyed_stuff(controlcen);

		if ((controlcen != object_none) && !(controlcen->flags&(OF_EXPLODING|OF_DESTROYED))) {
			digi_link_sound_to_pos( SOUND_CONTROL_CENTER_DESTROYED, controlcen->segnum, 0, controlcen->pos, 0, F1_0 );
			explode_object(controlcen,0);
		}
	}

}

//	-----------------------------------------------------------------------------
void apply_damage_to_controlcen(vobjptridx_t controlcen, fix damage, objnum_t who)
{
	int	whotype;

	//	Only allow a player to damage the control center.

	if ((who < 0) || (who > Highest_object_index))
		return;

	whotype = Objects[who].type;
	if (whotype != OBJ_PLAYER) {
		return;
	}

	if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP) && ((i2f(Players[Player_num].hours_level*3600)+Players[Player_num].time_level) < Netgame.control_invul_time))
	{
		if (get_player_id(&Objects[who]) == Player_num) {
			int secs = f2i(Netgame.control_invul_time-(i2f(Players[Player_num].hours_level*3600)+Players[Player_num].time_level)) % 60;
			int mins = f2i(Netgame.control_invul_time-(i2f(Players[Player_num].hours_level*3600)+Players[Player_num].time_level)) / 60;
			HUD_init_message(HM_DEFAULT, "%s %d:%02d.", TXT_CNTRLCEN_INVUL, mins, secs);
		}
		return;
	}

	if (get_player_id(&Objects[who]) == Player_num) {
		Control_center_been_hit = 1;
		ai_do_cloak_stuff();
	}

	if ( controlcen->shields >= 0 )
		controlcen->shields -= damage;

	if ( (controlcen->shields < 0) && !(controlcen->flags&(OF_EXPLODING|OF_DESTROYED)) ) {
		do_controlcen_destroyed_stuff(controlcen);

		if (Game_mode & GM_MULTI) {
			if (who == Players[Player_num].objnum)
				add_points_to_score(CONTROL_CEN_SCORE);
			multi_send_destroy_controlcen(controlcen, get_player_id(&Objects[who]) );
		}

		if (!(Game_mode & GM_MULTI))
			add_points_to_score(CONTROL_CEN_SCORE);

		digi_link_sound_to_pos( SOUND_CONTROL_CENTER_DESTROYED, controlcen->segnum, 0, controlcen->pos, 0, F1_0 );

		explode_object(controlcen,0);
	}
}

static void collide_player_and_controlcen(vobjptridx_t controlcen, vobjptridx_t playerobj, vms_vector *collision_point)
{
	if (get_player_id(playerobj) == Player_num) {
		Control_center_been_hit = 1;
		ai_do_cloak_stuff();				//	In case player cloaked, make control center know where he is.
	}

	if (check_collision_delayfunc_exec())
		digi_link_sound_to_pos( SOUND_ROBOT_HIT_PLAYER, playerobj->segnum, 0, *collision_point, 0, F1_0 );

	bump_two_objects(controlcen, playerobj, 1);

	return;
}

#if defined(DXX_BUILD_DESCENT_II)
static void collide_player_and_marker( vobjptridx_t  marker, object * playerobj, vms_vector *collision_point )
{
	if (get_player_id(playerobj)==Player_num) {
		int drawn;

		if (Game_mode & GM_MULTI)
		{
			drawn = HUD_init_message(HM_DEFAULT|HM_MAYDUPL, "MARKER %s: %s", static_cast<const char *>(Players[marker->id/2].callsign), &MarkerMessage[marker->id][0]);
		}
		else
		{
			if (MarkerMessage[marker->id][0])
				drawn = HUD_init_message(HM_DEFAULT|HM_MAYDUPL, "MARKER %d: %s", marker->id+1,&MarkerMessage[marker->id][0]);
			else
				drawn = HUD_init_message(HM_DEFAULT|HM_MAYDUPL, "MARKER %d", marker->id+1);
	   }

		if (drawn)
			digi_play_sample( SOUND_MARKER_HIT, F1_0 );

		detect_escort_goal_accomplished(marker);
   }
}
#endif

//	If a persistent weapon and other object is not a weapon, weaken it, else kill it.
//	If both objects are weapons, weaken the weapon.
static void maybe_kill_weapon(object *weapon, object *other_obj)
{
	if (is_proximity_bomb_or_smart_mine_or_placed_mine(get_weapon_id(weapon))) {
		weapon->flags |= OF_SHOULD_BE_DEAD;
		return;
	}

#if defined(DXX_BUILD_DESCENT_I)
	if ((weapon->mtype.phys_info.flags & PF_PERSISTENT) || (other_obj->type == OBJ_WEAPON))
#elif defined(DXX_BUILD_DESCENT_II)
	//	Changed, 10/12/95, MK: Make weapon-weapon collisions always kill both weapons if not persistent.
	//	Reason: Otherwise you can't use proxbombs to detonate incoming homing missiles (or mega missiles).
	if (weapon->mtype.phys_info.flags & PF_PERSISTENT)
#endif
	{
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

// -- 	if ((weapon->mtype.phys_info.flags & PF_PERSISTENT) || (other_obj->type == OBJ_WEAPON)) {
// -- 		//	Weapons do a lot of damage to weapons, other objects do much less.
// -- 		if (!(weapon->mtype.phys_info.flags & PF_PERSISTENT)) {
// -- 			if (other_obj->type == OBJ_WEAPON)
// -- 				weapon->shields -= other_obj->shields/2;
// -- 			else
// -- 				weapon->shields -= other_obj->shields/4;
// --
// -- 			if (weapon->shields <= 0) {
// -- 				weapon->shields = 0;
// -- 				weapon->flags |= OF_SHOULD_BE_DEAD;
// -- 			}
// -- 		}
// -- 	} else
// -- 		weapon->flags |= OF_SHOULD_BE_DEAD;
}

static void collide_weapon_and_controlcen(vobjptridx_t weapon, vobjptridx_t controlcen, vms_vector *collision_point)
{

#if defined(DXX_BUILD_DESCENT_I)
	fix explosion_size = ((controlcen->size/3)*3)/4;
#elif defined(DXX_BUILD_DESCENT_II)
	if (get_weapon_id(weapon) == OMEGA_ID)
		if (!ok_to_do_omega_damage(weapon)) // see comment in laser.c
			return;

	fix explosion_size = controlcen->size*3/20;
#endif
	if (weapon->ctype.laser_info.parent_type == OBJ_PLAYER) {
		fix	damage = weapon->shields;

		/*
		* Check if persistent weapon already hit this object. If yes, abort.
		* If no, add this object to hitobj_list and do it's damage.
		*/
		if (weapon->mtype.phys_info.flags & PF_PERSISTENT)
		{
			damage = weapon->shields*2; // to not alter Gameplay too much, multiply damage by 2.
			if (!weapon->ctype.laser_info.hitobj_list[controlcen])
			{
				weapon->ctype.laser_info.hitobj_list[controlcen] = true;
				weapon->ctype.laser_info.last_hitobj = controlcen;
			}
			else
			{
				return;
			}
		}

		if (get_player_id(&Objects[weapon->ctype.laser_info.parent_num]) == Player_num)
			Control_center_been_hit = 1;

		if ( Weapon_info[get_weapon_id(weapon)].damage_radius )
		{
			vms_vector obj2weapon;
			vm_vec_sub(obj2weapon, *collision_point, controlcen->pos);
			fix mag = vm_vec_mag(obj2weapon);
			if(mag < controlcen->size && mag > 0) // FVI code does not necessarily update the collision point for object2object collisions. Do that now.
			{
				vm_vec_scale_add(*collision_point, controlcen->pos, obj2weapon, fixdiv(controlcen->size, mag)); 
				weapon->pos = *collision_point;
			}
#if defined(DXX_BUILD_DESCENT_I)
			explode_badass_weapon(weapon, weapon->pos);
#elif defined(DXX_BUILD_DESCENT_II)
			explode_badass_weapon(weapon, *collision_point);
#endif
		}
		else
			object_create_explosion( controlcen->segnum, *collision_point, explosion_size, VCLIP_SMALL_EXPLOSION );

		digi_link_sound_to_pos( SOUND_CONTROL_CENTER_HIT, controlcen->segnum, 0, *collision_point, 0, F1_0 );

		damage = fixmul(damage, weapon->ctype.laser_info.multiplier);

		apply_damage_to_controlcen(controlcen, damage, weapon->ctype.laser_info.parent_num);

		maybe_kill_weapon(weapon,controlcen);
	} else {	//	If robot weapon hits control center, blow it up, make it go away, but do no damage to control center.
		object_create_explosion( controlcen->segnum, *collision_point, explosion_size, VCLIP_SMALL_EXPLOSION );
		maybe_kill_weapon(weapon,controlcen);
	}

}

static void collide_weapon_and_clutter(object * weapon, vobjptridx_t clutter, vms_vector *collision_point)
{
	short exp_vclip = VCLIP_SMALL_EXPLOSION;

	if ( clutter->shields >= 0 )
		clutter->shields -= weapon->shields;

	digi_link_sound_to_pos( SOUND_LASER_HIT_CLUTTER, weapon->segnum, 0, *collision_point, 0, F1_0 );

	object_create_explosion( clutter->segnum, *collision_point, ((clutter->size/3)*3)/4, exp_vclip );

	if ( (clutter->shields < 0) && !(clutter->flags&(OF_EXPLODING|OF_DESTROYED)))
		explode_object(clutter,STANDARD_EXPL_DELAY);

	maybe_kill_weapon(weapon,clutter);
}

//--mk, 121094 -- extern void spin_robot(object *robot, vms_vector *collision_point);

#if defined(DXX_BUILD_DESCENT_II)
int	Final_boss_is_dead = 0;
fix	Final_boss_countdown_time = 0;

//	------------------------------------------------------------------------------------------------------
void do_final_boss_frame(void)
{

	if (!Final_boss_is_dead)
		return;

	if (!Control_center_destroyed)
		return;

	if (Final_boss_countdown_time == 0)
		Final_boss_countdown_time = F1_0*2;

	Final_boss_countdown_time -= FrameTime;
	if (Final_boss_countdown_time > 0)
		return;

	start_endlevel_sequence();		//pretend we hit the exit trigger

}

//	------------------------------------------------------------------------------------------------------
//	This is all the ugly stuff we do when you kill the final boss so that you don't die or something
//	which would ruin the logic of the cut sequence.
void do_final_boss_hacks(void)
{
	if (Player_is_dead) {
		Int3();		//	Uh-oh, player is dead.  Try to rescue him.
		Player_is_dead = 0;
	}

	if (Players[Player_num].shields <= 0)
		Players[Player_num].shields = 1;

	//	If you're not invulnerable, get invulnerable!
	if (!(Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE)) {
		Players[Player_num].invulnerable_time = GameTime64;
		Players[Player_num].flags |= PLAYER_FLAGS_INVULNERABLE;
	}
	if (!(Game_mode & GM_MULTI))
		buddy_message("Nice job, %s!", static_cast<const char *>(Players[Player_num].callsign));

	Final_boss_is_dead = 1;
}
#endif

//	------------------------------------------------------------------------------------------------------
//	Return 1 if robot died, else return 0
int apply_damage_to_robot(vobjptridx_t robot, fix damage, objnum_t killer_objnum)
{
	if ( robot->flags&OF_EXPLODING) return 0;

	if (robot->shields < 0 ) return 0;	//robot already dead...

	const robot_info *robptr = &Robot_info[get_robot_id(robot)];
	if (robptr->boss_flag)
#if defined(DXX_BUILD_DESCENT_I)
		Boss_been_hit = 1;
#elif defined(DXX_BUILD_DESCENT_II)
		Boss_hit_time = GameTime64;

	//	Buddy invulnerable on level 24 so he can give you his important messages.  Bah.
	//	Also invulnerable if his cheat for firing weapons is in effect.
	if (robot_is_companion(robptr)) {
		if (PLAYING_BUILTIN_MISSION && Current_level_num == Last_level)
			return 0;
	}
#endif

	robot->shields -= damage;

#if defined(DXX_BUILD_DESCENT_II)
	//	Do unspeakable hacks to make sure player doesn't die after killing boss.  Or before, sort of.
	if (robptr->boss_flag)
		if (PLAYING_BUILTIN_MISSION && Current_level_num == Last_level)
			if (robot->shields < 0)
			 {
				if (Game_mode & GM_MULTI)
				  {
					 if (!multi_all_players_alive()) // everyones gotta be alive
					   robot->shields=1;
					 else
					  {
					    multi_send_finish_game();
					    do_final_boss_hacks();
					  }

					}
				else
				  {	// NOTE LINK TO ABOVE!!!
					if ((Players[Player_num].shields < 0) || Player_is_dead)
						robot->shields = 1;		//	Sorry, we can't allow you to kill the final boss after you've died.  Rough luck.
					else
						do_final_boss_hacks();
				  }
			  }
#endif

	if (robot->shields < 0) {
		if (Game_mode & GM_MULTI) {
#if defined(DXX_BUILD_DESCENT_II)
			char isthief;
			stolen_items_t temp_stolen;

		 if (robot_is_thief(robptr))
			{
			isthief=1;
				temp_stolen = Stolen_items;
			}
		 else
			isthief=0;
#endif

			if (multi_explode_robot_sub(robot))
			{
#if defined(DXX_BUILD_DESCENT_II)
			 if (isthief)
				Stolen_items = temp_stolen;
#endif

				multi_send_robot_explode(robot, killer_objnum);

#if defined(DXX_BUILD_DESCENT_II)
	     	   if (isthief)
					Stolen_items.fill(255);
#endif

				return 1;
			}
			else
				return 0;
		}

		Players[Player_num].num_kills_level++;
		Players[Player_num].num_kills_total++;

		if (robptr->boss_flag) {
			start_boss_death_sequence(robot);	//do_controlcen_destroyed_stuff(NULL);
		}
#if defined(DXX_BUILD_DESCENT_II)
		else if (robptr->death_roll) {
			start_robot_death_sequence(robot);	//do_controlcen_destroyed_stuff(NULL);
		}
#endif
		else {
#if defined(DXX_BUILD_DESCENT_II)
			if (get_robot_id(robot) == SPECIAL_REACTOR_ROBOT)
				special_reactor_stuff();
			//if (Robot_info[robot->id].smart_blobs)
			//	create_smart_children(robot, Robot_info[robot->id].smart_blobs);
			//if (Robot_info[robot->id].badass)
			//	explode_badass_object(robot, F1_0*Robot_info[robot->id].badass, F1_0*40, F1_0*150);
			if (robptr->kamikaze)
				explode_object(robot,1);		//	Kamikaze, explode right away, IN YOUR FACE!
			else
#endif
				explode_object(robot,STANDARD_EXPL_DELAY);
		}
		return 1;
	} else
		return 0;
}

#if defined(DXX_BUILD_DESCENT_II)
static inline int Boss_invulnerable_dot()
{
	return F1_0/4 - i2f(Difficulty_level)/8;
}

int	Buddy_gave_hint_count = 5;
fix64	Last_time_buddy_gave_hint = 0;

//	------------------------------------------------------------------------------------------------------
//	Return true if damage done to boss, else return false.
static int do_boss_weapon_collision(object *robot, object *weapon, vms_vector *collision_point)
{
	int	d2_boss_index;
	int	damage_flag;

	damage_flag = 1;

	d2_boss_index = Robot_info[get_robot_id(robot)].boss_flag - BOSS_D2;

	Assert((d2_boss_index >= 0) && (d2_boss_index < NUM_D2_BOSSES));

	//	See if should spew a bot.
	if (weapon->ctype.laser_info.parent_type == OBJ_PLAYER)
		if ((Weapon_info[get_weapon_id(weapon)].matter && Boss_spews_bots_matter[d2_boss_index]) || (!Weapon_info[get_weapon_id(weapon)].matter && Boss_spews_bots_energy[d2_boss_index])) {
			if (Boss_spew_more[d2_boss_index])
				if (d_rand() > 16384) {
					if (boss_spew_robot(robot, *collision_point) != object_none)
						Last_gate_time = GameTime64 - Gate_interval - 1;	//	Force allowing spew of another bot.
				}
			boss_spew_robot(robot, *collision_point);
		}

	if (Boss_invulnerable_spot[d2_boss_index]) {
		fix			dot;
		vms_vector	tvec1;

		//	Boss only vulnerable in back.  See if hit there.
		vm_vec_sub(tvec1, *collision_point, robot->pos);
		vm_vec_normalize_quick(tvec1);	//	Note, if BOSS_INVULNERABLE_DOT is close to F1_0 (in magnitude), then should probably use non-quick version.
		dot = vm_vec_dot(tvec1, robot->orient.fvec);
		if (dot > Boss_invulnerable_dot()) {
			auto segnum = find_point_seg(*collision_point, robot->segnum);
			digi_link_sound_to_pos( SOUND_WEAPON_HIT_DOOR, segnum, 0, *collision_point, 0, F1_0);
			damage_flag = 0;

			if (Buddy_objnum != object_none)
			{
				if (Last_time_buddy_gave_hint == 0)
					Last_time_buddy_gave_hint = d_rand()*32 + F1_0*16;

				if (Buddy_gave_hint_count) {
					if (Last_time_buddy_gave_hint + F1_0*20 < GameTime64) {
						int	sval;

						Buddy_gave_hint_count--;
						Last_time_buddy_gave_hint = GameTime64;
						sval = (d_rand()*4) >> 15;
						switch (sval) {
							case 0:	buddy_message("Hit him in the back!");	break;
							case 1:	buddy_message("He's invulnerable there!");	break;
							case 2:	buddy_message("Get behind him and fire!");	break;
							case 3:
							default:
										buddy_message("Hit the glowing spot!");	break;
						}
					}
				}
			}

			//	Cause weapon to bounce.
			//	Make a copy of this weapon, because the physics wants to destroy it.
			if (!Weapon_info[get_weapon_id(weapon)].matter) {
				auto new_obj = obj_create(OBJ_WEAPON, get_weapon_id(weapon), weapon->segnum, weapon->pos,
					&weapon->orient, weapon->size, weapon->control_type, weapon->movement_type, weapon->render_type);

				if (new_obj != object_none) {
					vms_vector	vec_to_point;
					vms_vector	weap_vec;
					fix			speed;

					if (weapon->render_type == RT_POLYOBJ) {
						new_obj->rtype.pobj_info.model_num = Weapon_info[get_weapon_id(new_obj)].model_num;
						new_obj->size = fixdiv(Polygon_models[new_obj->rtype.pobj_info.model_num].rad,Weapon_info[get_weapon_id(new_obj)].po_len_to_width_ratio);
					}

					new_obj->mtype.phys_info.mass = Weapon_info[get_weapon_id(weapon)].mass;
					new_obj->mtype.phys_info.drag = Weapon_info[get_weapon_id(weapon)].drag;
					vm_vec_zero(new_obj->mtype.phys_info.thrust);

					vm_vec_sub(vec_to_point, *collision_point, robot->pos);
					vm_vec_normalize_quick(vec_to_point);
					weap_vec = weapon->mtype.phys_info.velocity;
					speed = vm_vec_normalize_quick(weap_vec);
					vm_vec_scale_add2(vec_to_point, weap_vec, -F1_0*2);
					vm_vec_scale(vec_to_point, speed/4);
					new_obj->mtype.phys_info.velocity = vec_to_point;
				}
			}
		}
	} else if ((Weapon_info[get_weapon_id(weapon)].matter && Boss_invulnerable_matter[d2_boss_index]) || (!Weapon_info[get_weapon_id(weapon)].matter && Boss_invulnerable_energy[d2_boss_index])) {
		auto segnum = find_point_seg(*collision_point, robot->segnum);
		digi_link_sound_to_pos( SOUND_WEAPON_HIT_DOOR, segnum, 0, *collision_point, 0, F1_0);
		damage_flag = 0;
	}

	return damage_flag;
}
#endif

//	------------------------------------------------------------------------------------------------------
static void collide_robot_and_weapon(vobjptridx_t  robot, vobjptridx_t  weapon, vms_vector *collision_point )
{
	int	damage_flag=1;
#if defined(DXX_BUILD_DESCENT_II)
	int	boss_invul_flag=0;

	if (get_weapon_id(weapon) == OMEGA_ID)
		if (!ok_to_do_omega_damage(weapon)) // see comment in laser.c
			return;
#endif

	const robot_info *robptr = &Robot_info[get_robot_id(robot)];
	if (robptr->boss_flag)
	{
#if defined(DXX_BUILD_DESCENT_I)
		Boss_hit_this_frame = 1;
#elif defined(DXX_BUILD_DESCENT_II)
		Boss_hit_time = GameTime64;
		if (robptr->boss_flag >= BOSS_D2) {
			damage_flag = do_boss_weapon_collision(robot, weapon, collision_point);
			boss_invul_flag = !damage_flag;
		}
#endif
	}

#if defined(DXX_BUILD_DESCENT_II)
	//	Put in at request of Jasen (and Adam) because the Buddy-Bot gets in their way.
	//	MK has so much fun whacking his butt around the mine he never cared...
	if ((robot_is_companion(robptr)) && ((weapon->ctype.laser_info.parent_type != OBJ_ROBOT) && !cheats.robotskillrobots))
		return;

	if (get_weapon_id(weapon) == EARTHSHAKER_ID)
		smega_rock_stuff();
#endif

#if 1
	/*
	 * Check if persistent weapon already hit this object. If yes, abort.
	 * If no, add this object to hitobj_list and do it's damage.
	 */
	if (weapon->mtype.phys_info.flags & PF_PERSISTENT)
	{
		if (!weapon->ctype.laser_info.hitobj_list[robot])
		{
			weapon->ctype.laser_info.hitobj_list[robot] = true;
			weapon->ctype.laser_info.last_hitobj = robot;
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

#if defined(DXX_BUILD_DESCENT_II)
	//	Changed, 10/04/95, put out blobs based on skill level and power of weapon doing damage.
	//	Also, only a weapon hit from a player weapon causes smart blobs.
	if ((weapon->ctype.laser_info.parent_type == OBJ_PLAYER) && (robptr->energy_blobs))
		if ((robot->shields > 0) && Weapon_is_energy[get_weapon_id(weapon)]) {
			fix	probval;
			int	num_blobs;

			probval = (Difficulty_level+2) * min(weapon->shields, robot->shields);
			probval = robptr->energy_blobs * probval/(NDL*32);

			num_blobs = probval >> 16;
			if (2*d_rand() < (probval & 0xffff))
				num_blobs++;

			if (num_blobs)
				create_smart_children(robot, num_blobs);
		}

	//	Note: If weapon hits an invulnerable boss, it will still do badass damage, including to the boss,
	//	unless this is trapped elsewhere.
#endif
	weapon_info *wi = &Weapon_info[get_weapon_id(weapon)];
	if ( wi->damage_radius )
	{
		vms_vector obj2weapon;
		vm_vec_sub(obj2weapon, *collision_point, robot->pos);
		fix mag = vm_vec_mag(obj2weapon);
		if(mag < robot->size && mag > 0) // FVI code does not necessarily update the collision point for object2object collisions. Do that now.
		{
			vm_vec_scale_add(*collision_point, robot->pos, obj2weapon, fixdiv(robot->size, mag)); 
			weapon->pos = *collision_point;
		}
#if defined(DXX_BUILD_DESCENT_I)
		explode_badass_weapon(weapon, weapon->pos);
#elif defined(DXX_BUILD_DESCENT_II)
		if (boss_invul_flag) {			//don't make badass sound

			//this code copied from explode_badass_weapon()

			object_create_badass_explosion( weapon, weapon->segnum, *collision_point,
							wi->impact_size,
							wi->robot_hit_vclip,
							wi->strength[Difficulty_level],
							wi->damage_radius,wi->strength[Difficulty_level],
							weapon->ctype.laser_info.parent_num );

		}
		else		//normal badass explosion
			explode_badass_weapon(weapon, *collision_point);
#endif
	}

#if defined(DXX_BUILD_DESCENT_I)
	if ( (weapon->ctype.laser_info.parent_type==OBJ_PLAYER) && !(robot->flags & OF_EXPLODING) )
#elif defined(DXX_BUILD_DESCENT_II)
	if ( ((weapon->ctype.laser_info.parent_type==OBJ_PLAYER) || cheats.robotskillrobots) && !(robot->flags & OF_EXPLODING) )
#endif
	{
		if (weapon->ctype.laser_info.parent_num == Players[Player_num].objnum) {
			create_awareness_event(weapon, PA_WEAPON_ROBOT_COLLISION);			// object "weapon" can attract attention to player
			do_ai_robot_hit(robot, PA_WEAPON_ROBOT_COLLISION);
		}
	       	else
			multi_robot_request_change(robot, get_robot_id(&Objects[weapon->ctype.laser_info.parent_num]));

		objptridx_t expl_obj = object_none;
		if ( robptr->exp1_vclip_num > -1 )
			expl_obj = object_create_explosion( weapon->segnum, *collision_point, (robot->size/2*3)/4, robptr->exp1_vclip_num );
#if defined(DXX_BUILD_DESCENT_II)
		else if ( wi->robot_hit_vclip > -1 )
			expl_obj = object_create_explosion( weapon->segnum, *collision_point, wi->impact_size, wi->robot_hit_vclip );
#endif

		if (expl_obj != object_none)
			obj_attach(robot,expl_obj);

		if ( damage_flag && (robptr->exp1_sound_num > -1 ))
			digi_link_sound_to_pos( robptr->exp1_sound_num, robot->segnum, 0, *collision_point, 0, F1_0 );

		{
			fix	damage = weapon->shields;

			if (damage_flag)
				damage = fixmul(damage, weapon->ctype.laser_info.multiplier);
			else
				damage = 0;

#if defined(DXX_BUILD_DESCENT_II)
			//	Cut Gauss damage on bosses because it just breaks the game.  Bosses are so easy to
			//	hit, and missing a robot is what prevents the Gauss from being game-breaking.
			if (get_weapon_id(weapon) == GAUSS_ID)
				if (robptr->boss_flag)
					damage = damage * (2*NDL-Difficulty_level)/(2*NDL);
#endif

			if (! apply_damage_to_robot(robot, damage, weapon->ctype.laser_info.parent_num))
				bump_two_objects(robot, weapon, 0);		//only bump if not dead. no damage from bump
			else if (weapon->ctype.laser_info.parent_signature == ConsoleObject->signature) {
				add_points_to_score(robptr->score_value);
				detect_escort_goal_accomplished(robot);
			}
		}

#if defined(DXX_BUILD_DESCENT_II)
		//	If Gauss Cannon, spin robot.
		if (!robot_is_companion(robptr) && (!robptr->boss_flag) && (get_weapon_id(weapon) == GAUSS_ID)) {
			ai_static	*aip = &robot->ctype.ai_info;

			if (aip->SKIP_AI_COUNT * FrameTime < F1_0) {
				aip->SKIP_AI_COUNT++;
				robot->mtype.phys_info.rotthrust.x = fixmul((d_rand() - 16384), FrameTime * aip->SKIP_AI_COUNT);
				robot->mtype.phys_info.rotthrust.y = fixmul((d_rand() - 16384), FrameTime * aip->SKIP_AI_COUNT);
				robot->mtype.phys_info.rotthrust.z = fixmul((d_rand() - 16384), FrameTime * aip->SKIP_AI_COUNT);
				robot->mtype.phys_info.flags |= PF_USES_THRUST;

			}
		}
#endif

	}

	maybe_kill_weapon(weapon,robot);

	return;
}

static void collide_hostage_and_player(vobjptridx_t  hostage, object * player, vms_vector *collision_point)
{
	// Give player points, etc.
	if ( player == ConsoleObject )	{
		detect_escort_goal_accomplished(hostage);
		add_points_to_score(HOSTAGE_SCORE);

		// Do effect
		hostage_rescue();

		// Remove the hostage object.
		hostage->flags |= OF_SHOULD_BE_DEAD;

		if (Game_mode & GM_MULTI)
			multi_send_remobj(hostage);
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

static void collide_player_and_player(vobjptridx_t player1, vobjptridx_t player2, vms_vector *collision_point)
{
	static fix64 last_player_bump[MAX_PLAYERS] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	int damage_flag = 1, otherpl = -1;

	if (check_collision_delayfunc_exec())
		digi_link_sound_to_pos( SOUND_ROBOT_HIT_PLAYER, player1->segnum, 0, *collision_point, 0, F1_0 );

	// Multi is special - as always. Clients do the bump damage locally but the remote players do their collision result (change velocity) on their own. So after our initial collision, ignore further collision damage till remote players (hopefully) react.
	if (Game_mode & GM_MULTI)
	{
		damage_flag = 0;
		if (!(get_player_id(player1) == Player_num || get_player_id(player2) == Player_num))
			return;
		if (get_player_id(player1) > MAX_PLAYERS || get_player_id(player2) > MAX_PLAYERS)
			return;
		otherpl = (get_player_id(player1)==Player_num)?get_player_id(player2):get_player_id(player1);
		if (last_player_bump[otherpl] + (F1_0/Netgame.PacketsPerSec) < GameTime64 || last_player_bump[otherpl] > GameTime64)
		{
			last_player_bump[otherpl] = GameTime64;
			damage_flag = 1;
		}
	}

	bump_two_objects(player1, player2, damage_flag);
	return;
}

static objnum_t maybe_drop_primary_weapon_egg(object *playerobj, int weapon_index)
{
	int weapon_flag = HAS_PRIMARY_FLAG(weapon_index);
	int powerup_num;

	powerup_num = Primary_weapon_to_powerup[weapon_index];

	if (Players[get_player_id(playerobj)].primary_weapon_flags & weapon_flag)
		return call_object_create_egg(playerobj, 1, OBJ_POWERUP, powerup_num);
	else
		return object_none;
}

static void maybe_drop_secondary_weapon_egg(object *playerobj, int weapon_index, int count)
{
	int weapon_flag = HAS_SECONDARY_FLAG(weapon_index);
	int powerup_num;

	powerup_num = Secondary_weapon_to_powerup[weapon_index];

	if (Players[get_player_id(playerobj)].secondary_weapon_flags & weapon_flag) {
		int	max_count;

		max_count = min(count, 3);
		for (int i=0; i<max_count; i++)
			call_object_create_egg(playerobj, 1, OBJ_POWERUP, powerup_num);
	}
}

static void drop_missile_1_or_4(object *playerobj,int missile_index)
{
	int num_missiles,powerup_id;

	num_missiles = Players[get_player_id(playerobj)].secondary_ammo[missile_index];
	powerup_id = Secondary_weapon_to_powerup[missile_index];

	if (num_missiles > 10)
		num_missiles = 10;

	call_object_create_egg(playerobj, num_missiles/4, OBJ_POWERUP, powerup_id+1);
	call_object_create_egg(playerobj, num_missiles%4, OBJ_POWERUP, powerup_id);
}

void drop_player_eggs(vobjptridx_t playerobj)
{
	if ((playerobj->type == OBJ_PLAYER) || (playerobj->type == OBJ_GHOST)) {
		playernum_t	pnum = get_player_id(playerobj);
		int	vulcan_ammo=0;

		// Seed the random number generator so in net play the eggs will always
		// drop the same way
		if (Game_mode & GM_MULTI)
		{
			Net_create_loc = 0;
			d_srand(5483L);
		}

#if defined(DXX_BUILD_DESCENT_II)
		//	If the player had smart mines, maybe arm one of them.
		int	rthresh;
		rthresh = 30000;
		while ((Players[get_player_id(playerobj)].secondary_ammo[SMART_MINE_INDEX]%4==1) && (d_rand() < rthresh)) {
			vms_vector	tvec;

			vms_vector	randvec;
			make_random_vector(randvec);
			rthresh /= 2;
			vm_vec_add(tvec, playerobj->pos, randvec);
			auto newseg = find_point_seg(tvec, playerobj->segnum);
			if (newseg != segment_none)
				Laser_create_new(&randvec, &tvec, newseg, playerobj, SUPERPROX_ID, 0);
	  	}

		//	If the player had proximity bombs, maybe arm one of them.

		if ((Game_mode & GM_MULTI) && !game_mode_hoard())
		{
			rthresh = 30000;
			while ((Players[get_player_id(playerobj)].secondary_ammo[PROXIMITY_INDEX]%4==1) && (d_rand() < rthresh)) {
				vms_vector	tvec;

				vms_vector	randvec;
				make_random_vector(randvec);
				rthresh /= 2;
				vm_vec_add(tvec, playerobj->pos, randvec);
				auto newseg = find_point_seg(tvec, playerobj->segnum);
				if (newseg != segment_none)
					Laser_create_new(&randvec, &tvec, newseg, playerobj, PROXIMITY_ID, 0);

			}
		}
#endif

		//	If the player dies and he has powerful lasers, create the powerups here.

#if defined(DXX_BUILD_DESCENT_II)
		if (Players[pnum].laser_level > MAX_LASER_LEVEL)
			call_object_create_egg(playerobj, Players[pnum].laser_level-MAX_LASER_LEVEL, OBJ_POWERUP, POW_SUPER_LASER);
		else
#endif
			if (Players[pnum].laser_level >= 1)
			call_object_create_egg(playerobj, Players[pnum].laser_level, OBJ_POWERUP, POW_LASER);	// Note: laser_level = 0 for laser level 1.

		//	Drop quad laser if appropos
		if (Players[pnum].flags & PLAYER_FLAGS_QUAD_LASERS)
			call_object_create_egg(playerobj, 1, OBJ_POWERUP, POW_QUAD_FIRE);

		if (Players[pnum].flags & PLAYER_FLAGS_CLOAKED)
			call_object_create_egg(playerobj, 1, OBJ_POWERUP, POW_CLOAK);

#if defined(DXX_BUILD_DESCENT_II)
		if (Players[pnum].flags & PLAYER_FLAGS_MAP_ALL)
			call_object_create_egg(playerobj, 1, OBJ_POWERUP, POW_FULL_MAP);

		if (Players[pnum].flags & PLAYER_FLAGS_AFTERBURNER)
			call_object_create_egg(playerobj, 1, OBJ_POWERUP, POW_AFTERBURNER);

		if (Players[pnum].flags & PLAYER_FLAGS_AMMO_RACK)
			call_object_create_egg(playerobj, 1, OBJ_POWERUP, POW_AMMO_RACK);

		if (Players[pnum].flags & PLAYER_FLAGS_CONVERTER)
			call_object_create_egg(playerobj, 1, OBJ_POWERUP, POW_CONVERTER);

		if (Players[pnum].flags & PLAYER_FLAGS_HEADLIGHT)
			call_object_create_egg(playerobj, 1, OBJ_POWERUP, POW_HEADLIGHT);

		// drop the other enemies flag if you have it

		if (game_mode_capture_flag() && (Players[pnum].flags & PLAYER_FLAGS_FLAG))
		{
		 if ((get_team (pnum)==TEAM_RED))
			call_object_create_egg(playerobj, 1, OBJ_POWERUP, POW_FLAG_BLUE);
		 else
			call_object_create_egg(playerobj, 1, OBJ_POWERUP, POW_FLAG_RED);
		}


		if (game_mode_hoard())
		{
			// Drop hoard orbs

			int max_count;

			max_count = min(Players[pnum].secondary_ammo[PROXIMITY_INDEX], (unsigned short) 12);
			for (int i=0; i<max_count; i++)
				call_object_create_egg(playerobj, 1, OBJ_POWERUP, POW_HOARD_ORB);
		}
#endif

		//Drop the vulcan, gauss, and ammo
		vulcan_ammo = Players[pnum].vulcan_ammo;
#if defined(DXX_BUILD_DESCENT_II)
		if ((Players[pnum].primary_weapon_flags & HAS_VULCAN_FLAG) && (Players[pnum].primary_weapon_flags & HAS_GAUSS_FLAG))
			vulcan_ammo /= 2;		//if both vulcan & gauss, each gets half
#endif
		if (vulcan_ammo < VULCAN_AMMO_AMOUNT)
			vulcan_ammo = VULCAN_AMMO_AMOUNT;	//make sure gun has at least as much as a powerup
		objnum_t objnum = maybe_drop_primary_weapon_egg(playerobj, VULCAN_INDEX);
		if (objnum!=object_none)
			Objects[objnum].ctype.powerup_info.count = vulcan_ammo;
#if defined(DXX_BUILD_DESCENT_II)
		objnum = maybe_drop_primary_weapon_egg(playerobj, GAUSS_INDEX);
		if (objnum!=object_none)
			Objects[objnum].ctype.powerup_info.count = vulcan_ammo;
#endif

		//	Drop the rest of the primary weapons
		maybe_drop_primary_weapon_egg(playerobj, SPREADFIRE_INDEX);
		maybe_drop_primary_weapon_egg(playerobj, PLASMA_INDEX);
		maybe_drop_primary_weapon_egg(playerobj, FUSION_INDEX);

#if defined(DXX_BUILD_DESCENT_II)
		maybe_drop_primary_weapon_egg(playerobj, HELIX_INDEX);
		maybe_drop_primary_weapon_egg(playerobj, PHOENIX_INDEX);

		objnum = maybe_drop_primary_weapon_egg(playerobj, OMEGA_INDEX);
		if (objnum!=object_none)
			Objects[objnum].ctype.powerup_info.count = (get_player_id(playerobj)==Player_num)?Omega_charge:MAX_OMEGA_CHARGE;
#endif

		//	Drop the secondary weapons
		//	Note, proximity weapon only comes in packets of 4.  So drop n/2, but a max of 3 (handled inside maybe_drop..)  Make sense?

#if defined(DXX_BUILD_DESCENT_II)
		if (!game_mode_hoard())
#endif
			maybe_drop_secondary_weapon_egg(playerobj, PROXIMITY_INDEX, (Players[get_player_id(playerobj)].secondary_ammo[PROXIMITY_INDEX])/4);

		maybe_drop_secondary_weapon_egg(playerobj, SMART_INDEX, Players[get_player_id(playerobj)].secondary_ammo[SMART_INDEX]);
		maybe_drop_secondary_weapon_egg(playerobj, MEGA_INDEX, Players[get_player_id(playerobj)].secondary_ammo[MEGA_INDEX]);

#if defined(DXX_BUILD_DESCENT_II)
		maybe_drop_secondary_weapon_egg(playerobj, SMART_MINE_INDEX,(Players[get_player_id(playerobj)].secondary_ammo[SMART_MINE_INDEX])/4);
		maybe_drop_secondary_weapon_egg(playerobj, SMISSILE5_INDEX, Players[get_player_id(playerobj)].secondary_ammo[SMISSILE5_INDEX]);
#endif

		//	Drop the player's missiles in packs of 1 and/or 4
		drop_missile_1_or_4(playerobj,HOMING_INDEX);
#if defined(DXX_BUILD_DESCENT_II)
		drop_missile_1_or_4(playerobj,GUIDED_INDEX);
#endif
		drop_missile_1_or_4(playerobj,CONCUSSION_INDEX);
#if defined(DXX_BUILD_DESCENT_II)
		drop_missile_1_or_4(playerobj,SMISSILE1_INDEX);
		drop_missile_1_or_4(playerobj,SMISSILE4_INDEX);
#endif

		//	If player has vulcan ammo, but no vulcan cannon, drop the ammo.
		if (!(Players[get_player_id(playerobj)].primary_weapon_flags & HAS_VULCAN_FLAG)) {
			int	amount = Players[get_player_id(playerobj)].vulcan_ammo;
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

void apply_damage_to_player(object *playerobj, objptridx_t killer, fix damage, ubyte possibly_friendly)
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

	if (get_player_id(playerobj) == Player_num) {		//is this the local player?
		Players[Player_num].shields -= damage;
		PALETTE_FLASH_ADD(f2i(damage)*4,-f2i(damage/2),-f2i(damage/2));	//flash red

		if (Players[Player_num].shields < 0)	{

  			Players[Player_num].killer_objnum = killer;
			playerobj->flags |= OF_SHOULD_BE_DEAD;

#if defined(DXX_BUILD_DESCENT_II)
			if (Buddy_objnum != object_none)
				if (killer && (killer->type == OBJ_ROBOT) && robot_is_companion(&Robot_info[get_robot_id(killer)]))
					Buddy_sorry_time = GameTime64;
#endif
		}

		playerobj->shields = Players[Player_num].shields;		//mirror

	}
}

static void collide_player_and_weapon(vobjptridx_t playerobj, vobjptridx_t weapon, vms_vector *collision_point)
{
	fix		damage = weapon->shields;
	objptridx_t killer = object_none;

#if defined(DXX_BUILD_DESCENT_II)
	if (get_weapon_id(weapon) == OMEGA_ID)
		if (!ok_to_do_omega_damage(weapon)) // see comment in laser.c
			return;

	//	Don't collide own smart mines unless direct hit.
	if (get_weapon_id(weapon) == SUPERPROX_ID)
		if (playerobj == weapon->ctype.laser_info.parent_num)
			if (vm_vec_dist_quick(*collision_point, playerobj->pos) > playerobj->size)
				return;

	if (get_weapon_id(weapon) == EARTHSHAKER_ID)
		smega_rock_stuff();
#endif

	damage = fixmul(damage, weapon->ctype.laser_info.multiplier);
#if defined(DXX_BUILD_DESCENT_II)
	if (Game_mode & GM_MULTI)
		damage = fixmul(damage, Weapon_info[get_weapon_id(weapon)].multi_damage_scale);
#endif

#if 1
	/*
	 * Check if persistent weapon already hit this object. If yes, abort.
	 * If no, add this object to hitobj_list and do it's damage.
	 */
	if (weapon->mtype.phys_info.flags & PF_PERSISTENT)
	{
		if (!weapon->ctype.laser_info.hitobj_list[playerobj])
		{
			weapon->ctype.laser_info.hitobj_list[playerobj] = true;
			weapon->ctype.laser_info.last_hitobj = playerobj;
		}
		else
		{
			return;
		}
	}
#endif

	if (get_player_id(playerobj) == Player_num)
	{
		if (!(Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE))
		{
			digi_link_sound_to_pos( SOUND_PLAYER_GOT_HIT, playerobj->segnum, 0, *collision_point, 0, F1_0 );
			if (Game_mode & GM_MULTI)
				multi_send_play_sound(SOUND_PLAYER_GOT_HIT, F1_0);
		}
		else
		{
			digi_link_sound_to_pos( SOUND_WEAPON_HIT_DOOR, playerobj->segnum, 0, *collision_point, 0, F1_0);
			if (Game_mode & GM_MULTI)
				multi_send_play_sound(SOUND_WEAPON_HIT_DOOR, F1_0);
		}
	}

	object_create_explosion( playerobj->segnum, *collision_point, i2f(10)/2, VCLIP_PLAYER_HIT );
	if ( Weapon_info[get_weapon_id(weapon)].damage_radius )
	{
		vms_vector obj2weapon;
		vm_vec_sub(obj2weapon, *collision_point, playerobj->pos);
		fix mag = vm_vec_mag(obj2weapon);
		if(mag < playerobj->size && mag > 0) // FVI code does not necessarily update the collision point for object2object collisions. Do that now.
		{
			vm_vec_scale_add(*collision_point, playerobj->pos, obj2weapon, fixdiv(playerobj->size, mag)); 
			weapon->pos = *collision_point;
		}
#if defined(DXX_BUILD_DESCENT_I)
		explode_badass_weapon(weapon, weapon->pos);
#elif defined(DXX_BUILD_DESCENT_II)
		explode_badass_weapon(weapon, *collision_point);
#endif
	}

	maybe_kill_weapon(weapon,playerobj);

	bump_two_objects(playerobj, weapon, 0);	//no damage from bump

	if ( !Weapon_info[get_weapon_id(weapon)].damage_radius ) {
		if ( weapon->ctype.laser_info.parent_num != object_none )
			killer = objptridx(weapon->ctype.laser_info.parent_num);

//		if (weapon->id == SMART_HOMING_ID)
//			damage /= 4;

			apply_damage_to_player( playerobj, killer, damage, 1);
	}

	//	Robots become aware of you if you get hit.
	ai_do_cloak_stuff();

	return;
}

//	Nasty robots are the ones that attack you by running into you and doing lots of damage.
void collide_player_and_nasty_robot(vobjptridx_t playerobj, vobjptridx_t robot, vms_vector *collision_point)
{
		digi_link_sound_to_pos( Robot_info[get_robot_id(robot)].claw_sound, playerobj->segnum, 0, *collision_point, 0, F1_0 );

	object_create_explosion( playerobj->segnum, *collision_point, i2f(10)/2, VCLIP_PLAYER_HIT );

	bump_two_objects(playerobj, robot, 0);	//no damage from bump

	apply_damage_to_player( playerobj, robot, F1_0*(Difficulty_level+1), 0);

	return;
}

void collide_player_and_materialization_center(object *objp)
{
	vms_vector	exit_dir;
	segment	*segp = &Segments[objp->segnum];

	digi_link_sound_to_pos(SOUND_PLAYER_GOT_HIT, objp->segnum, 0, objp->pos, 0, F1_0);
//	digi_play_sample( SOUND_PLAYER_GOT_HIT, F1_0 );

	object_create_explosion( objp->segnum, objp->pos, i2f(10)/2, VCLIP_PLAYER_HIT );

	if (get_player_id(objp) != Player_num)
		return;

	for (int side=0; side<MAX_SIDES_PER_SEGMENT; side++)
		if (WALL_IS_DOORWAY(segp, side) & WID_FLY_FLAG) {
			vms_vector	exit_point, rand_vec;

			compute_center_point_on_side(&exit_point, segp, side);
			vm_vec_sub(exit_dir, exit_point, objp->pos);
			vm_vec_normalize_quick(exit_dir);
			make_random_vector(rand_vec);
			rand_vec.x /= 4;	rand_vec.y /= 4;	rand_vec.z /= 4;
			vm_vec_add2(exit_dir, rand_vec);
			vm_vec_normalize_quick(exit_dir);
		}

	bump_one_object(objp, &exit_dir, 64*F1_0);

#if defined(DXX_BUILD_DESCENT_I)
	apply_damage_to_player( objp, object_none, 4*F1_0, 0);
#elif defined(DXX_BUILD_DESCENT_II)
	apply_damage_to_player( objp, objp, 4*F1_0, 0);	//	Changed, MK, 2/19/96, make killer the player, so if you die in matcen, will say you killed yourself
#endif

	return;

}

void collide_robot_and_materialization_center(vobjptridx_t objp)
{
	vms_vector	exit_dir;
	segment *segp=&Segments[objp->segnum];

	vm_vec_zero(exit_dir);
	digi_link_sound_to_pos(SOUND_ROBOT_HIT, objp->segnum, 0, objp->pos, 0, F1_0);

	if ( Robot_info[get_robot_id(objp)].exp1_vclip_num > -1 )
		object_create_explosion( objp->segnum, objp->pos, (objp->size/2*3)/4, Robot_info[get_robot_id(objp)].exp1_vclip_num );

	for (int side=0; side<MAX_SIDES_PER_SEGMENT; side++)
		if (WALL_IS_DOORWAY(segp, side) & WID_FLY_FLAG) {
			vms_vector	exit_point;

			compute_center_point_on_side(&exit_point, segp, side);
			vm_vec_sub(exit_dir, exit_point, objp->pos);
			vm_vec_normalize_quick(exit_dir);
		}

	bump_one_object(objp, &exit_dir, 8*F1_0);

	apply_damage_to_robot( objp, F1_0, object_none);

	return;

}

void collide_player_and_powerup(object * playerobj, vobjptridx_t powerup, vms_vector *collision_point)
{
	if (!Endlevel_sequence && !Player_is_dead && (get_player_id(playerobj) == Player_num )) {
		int powerup_used;

		powerup_used = do_powerup(powerup);

		if (powerup_used)	{
			powerup->flags |= OF_SHOULD_BE_DEAD;
			if (Game_mode & GM_MULTI)
				multi_send_remobj(powerup);
		}
	}
	else if ((Game_mode & GM_MULTI_COOP) && (get_player_id(playerobj) != Player_num))
	{
		switch (get_powerup_id(powerup)) {
			case POW_KEY_BLUE:
				Players[get_player_id(playerobj)].flags |= PLAYER_FLAGS_BLUE_KEY;
				break;
			case POW_KEY_RED:
				Players[get_player_id(playerobj)].flags |= PLAYER_FLAGS_RED_KEY;
				break;
			case POW_KEY_GOLD:
				Players[get_player_id(playerobj)].flags |= PLAYER_FLAGS_GOLD_KEY;
				break;
			default:
				break;
		}
	}
	return;
}

static void collide_player_and_clutter(vobjptridx_t  playerobj, vobjptridx_t  clutter, vms_vector *collision_point)
{
	if (check_collision_delayfunc_exec())
		digi_link_sound_to_pos( SOUND_ROBOT_HIT_PLAYER, playerobj->segnum, 0, *collision_point, 0, F1_0 );
	bump_two_objects(clutter, playerobj, 1);
	return;
}

//	See if weapon1 creates a badass explosion.  If so, create the explosion
//	Return true if weapon does proximity (as opposed to only contact) damage when it explodes.
int maybe_detonate_weapon(vobjptridx_t weapon1, object *weapon2, vms_vector *collision_point)
{
	if ( Weapon_info[get_weapon_id(weapon1)].damage_radius ) {
		fix	dist;

		dist = vm_vec_dist_quick(weapon1->pos, weapon2->pos);
		if (dist < F1_0*5) {
			maybe_kill_weapon(weapon1,weapon2);
			if (weapon1->flags & OF_SHOULD_BE_DEAD) {
#if defined(DXX_BUILD_DESCENT_I)
				explode_badass_weapon(weapon1, weapon1->pos);
#elif defined(DXX_BUILD_DESCENT_II)
				explode_badass_weapon(weapon1, *collision_point);
#endif
				digi_link_sound_to_pos( Weapon_info[get_weapon_id(weapon1)].robot_hit_sound, weapon1->segnum , 0, *collision_point, 0, F1_0 );
			}
			return 1;
		} else {
			weapon1->lifeleft = min(dist/64, F1_0);
			return 1;
		}
	} else
		return 0;
}

static void collide_weapon_and_weapon(vobjptridx_t weapon1, vobjptridx_t weapon2, vms_vector *collision_point)
{
#if defined(DXX_BUILD_DESCENT_II)
	// -- Does this look buggy??:  if (weapon1->id == PMINE_ID && weapon1->id == PMINE_ID)
	if (get_weapon_id(weapon1) == PMINE_ID && get_weapon_id(weapon2) == PMINE_ID)
		return;		//these can't blow each other up

	if (get_weapon_id(weapon1) == OMEGA_ID) {
		if (!ok_to_do_omega_damage(weapon1)) // see comment in laser.c
			return;
	} else if (get_weapon_id(weapon2) == OMEGA_ID) {
		if (!ok_to_do_omega_damage(weapon2)) // see comment in laser.c
			return;
	}
#endif

	if ((Weapon_info[get_weapon_id(weapon1)].destroyable) || (Weapon_info[get_weapon_id(weapon2)].destroyable)) {
		// shooting Plasma will make bombs explode one drops at the same time since hitboxes overlap. Small HACK to get around this issue. if the player moves away from the bomb at least...
		if ((GameTime64 < weapon1->ctype.laser_info.creation_time + (F1_0/5)) && (GameTime64 < weapon2->ctype.laser_info.creation_time + (F1_0/5)) && (weapon1->ctype.laser_info.parent_num == weapon2->ctype.laser_info.parent_num))
			return;
		//	Bug reported by Adam Q. Pletcher on September 9, 1994, smart bomb homing missiles were toasting each other.
		if ((get_weapon_id(weapon1) == get_weapon_id(weapon2)) && (weapon1->ctype.laser_info.parent_num == weapon2->ctype.laser_info.parent_num))
			return;

#if defined(DXX_BUILD_DESCENT_I)
		if (Weapon_info[get_weapon_id(weapon1)].destroyable)
			if (maybe_detonate_weapon(weapon1, weapon2, collision_point))
				maybe_kill_weapon(weapon2,weapon1);

		if (Weapon_info[get_weapon_id(weapon2)].destroyable)
			if (maybe_detonate_weapon(weapon2, weapon1, collision_point))
				maybe_kill_weapon(weapon1,weapon2);
#elif defined(DXX_BUILD_DESCENT_II)
		if (Weapon_info[get_weapon_id(weapon1)].destroyable)
			if (maybe_detonate_weapon(weapon1, weapon2, collision_point))
				maybe_detonate_weapon(weapon2,weapon1, collision_point);

		if (Weapon_info[get_weapon_id(weapon2)].destroyable)
			if (maybe_detonate_weapon(weapon2, weapon1, collision_point))
				maybe_detonate_weapon(weapon1,weapon2, collision_point);
#endif
	}

}

static void collide_weapon_and_debris(vobjptridx_t weapon, vobjptridx_t debris, vms_vector *collision_point)
{
#if defined(DXX_BUILD_DESCENT_II)
	//	Hack!  Prevent debris from causing bombs spewed at player death to detonate!
	if ((get_weapon_id(weapon) == PROXIMITY_ID) || (get_weapon_id(weapon) == SUPERPROX_ID)) {
		if (weapon->ctype.laser_info.creation_time + F1_0/2 > GameTime64)
			return;
	}
#endif
	if ( (weapon->ctype.laser_info.parent_type==OBJ_PLAYER) && !(debris->flags & OF_EXPLODING) )	{
		digi_link_sound_to_pos( SOUND_ROBOT_HIT, weapon->segnum , 0, *collision_point, 0, F1_0 );

		explode_object(debris,0);
		if ( Weapon_info[get_weapon_id(weapon)].damage_radius )
			explode_badass_weapon(weapon, *collision_point);
		maybe_kill_weapon(weapon,debris);
		if (!(weapon->mtype.phys_info.flags & PF_PERSISTENT))
			weapon->flags |= OF_SHOULD_BE_DEAD;
	}
	return;
}

#if defined(DXX_BUILD_DESCENT_I)
#define DXX_COLLISION_TABLE(NO,DO)	\

#elif defined(DXX_BUILD_DESCENT_II)
#define DXX_COLLISION_TABLE(NO,DO)	\
	NO##_SAME_COLLISION(OBJ_MARKER)	\
	DO##_COLLISION( OBJ_MARKER, OBJ_PLAYER,  collide_player_and_marker)	\
	NO##_COLLISION( OBJ_MARKER, OBJ_ROBOT)	\
	NO##_COLLISION( OBJ_MARKER, OBJ_HOSTAGE)	\
	NO##_COLLISION( OBJ_MARKER, OBJ_WEAPON)	\
	NO##_COLLISION( OBJ_MARKER, OBJ_CAMERA)	\
	NO##_COLLISION( OBJ_MARKER, OBJ_POWERUP)	\
	NO##_COLLISION( OBJ_MARKER, OBJ_DEBRIS)	\

#endif

#define COLLIDE_IGNORE_COLLISION(O1,O2,C)

#define COLLISION_TABLE(NO,DO)	\
	NO##_SAME_COLLISION( OBJ_FIREBALL)	\
	DO##_SAME_COLLISION( OBJ_ROBOT, collide_robot_and_robot )	\
	NO##_SAME_COLLISION( OBJ_HOSTAGE)	\
	DO##_SAME_COLLISION( OBJ_PLAYER, collide_player_and_player )	\
	DO##_SAME_COLLISION( OBJ_WEAPON, collide_weapon_and_weapon )	\
	NO##_SAME_COLLISION( OBJ_CAMERA)	\
	NO##_SAME_COLLISION( OBJ_POWERUP)	\
	NO##_SAME_COLLISION( OBJ_DEBRIS)	\
	DO##_COLLISION( OBJ_WALL, OBJ_ROBOT, COLLIDE_IGNORE_COLLISION)	\
	DO##_COLLISION( OBJ_WALL, OBJ_WEAPON, COLLIDE_IGNORE_COLLISION)	\
	DO##_COLLISION( OBJ_WALL, OBJ_PLAYER, COLLIDE_IGNORE_COLLISION)	\
	DO##_COLLISION( OBJ_WALL, OBJ_POWERUP, COLLIDE_IGNORE_COLLISION)	\
	DO##_COLLISION( OBJ_WALL, OBJ_DEBRIS, COLLIDE_IGNORE_COLLISION)	\
	NO##_COLLISION( OBJ_FIREBALL, OBJ_ROBOT)	\
	NO##_COLLISION( OBJ_FIREBALL, OBJ_HOSTAGE)	\
	NO##_COLLISION( OBJ_FIREBALL, OBJ_PLAYER)	\
	NO##_COLLISION( OBJ_FIREBALL, OBJ_WEAPON)	\
	NO##_COLLISION( OBJ_FIREBALL, OBJ_CAMERA)	\
	NO##_COLLISION( OBJ_FIREBALL, OBJ_POWERUP)	\
	NO##_COLLISION( OBJ_FIREBALL, OBJ_DEBRIS)	\
	NO##_COLLISION( OBJ_ROBOT, OBJ_HOSTAGE)	\
	DO##_COLLISION( OBJ_ROBOT, OBJ_PLAYER,  collide_robot_and_player )	\
	DO##_COLLISION( OBJ_ROBOT, OBJ_WEAPON,  collide_robot_and_weapon )	\
	NO##_COLLISION( OBJ_ROBOT, OBJ_CAMERA)	\
	NO##_COLLISION( OBJ_ROBOT, OBJ_POWERUP)	\
	NO##_COLLISION( OBJ_ROBOT, OBJ_DEBRIS)	\
	DO##_COLLISION( OBJ_HOSTAGE, OBJ_PLAYER,  collide_hostage_and_player )	\
	DO##_COLLISION( OBJ_HOSTAGE, OBJ_WEAPON, COLLIDE_IGNORE_COLLISION)	\
	NO##_COLLISION( OBJ_HOSTAGE, OBJ_CAMERA)	\
	NO##_COLLISION( OBJ_HOSTAGE, OBJ_POWERUP)	\
	NO##_COLLISION( OBJ_HOSTAGE, OBJ_DEBRIS)	\
	DO##_COLLISION( OBJ_PLAYER, OBJ_WEAPON,  collide_player_and_weapon )	\
	NO##_COLLISION( OBJ_PLAYER, OBJ_CAMERA)	\
	DO##_COLLISION( OBJ_PLAYER, OBJ_POWERUP, collide_player_and_powerup )	\
	NO##_COLLISION( OBJ_PLAYER, OBJ_DEBRIS)	\
	DO##_COLLISION( OBJ_PLAYER, OBJ_CNTRLCEN, collide_player_and_controlcen )	\
	DO##_COLLISION( OBJ_PLAYER, OBJ_CLUTTER, collide_player_and_clutter )	\
	NO##_COLLISION( OBJ_WEAPON, OBJ_CAMERA)	\
	NO##_COLLISION( OBJ_WEAPON, OBJ_POWERUP)	\
	DO##_COLLISION( OBJ_WEAPON, OBJ_DEBRIS,  collide_weapon_and_debris )	\
	NO##_COLLISION( OBJ_CAMERA, OBJ_POWERUP)	\
	NO##_COLLISION( OBJ_CAMERA, OBJ_DEBRIS)	\
	NO##_COLLISION( OBJ_POWERUP, OBJ_DEBRIS)	\
	DO##_COLLISION( OBJ_WEAPON, OBJ_CNTRLCEN, collide_weapon_and_controlcen )	\
	DO##_COLLISION( OBJ_ROBOT, OBJ_CNTRLCEN, collide_robot_and_controlcen )	\
	DO##_COLLISION( OBJ_WEAPON, OBJ_CLUTTER, collide_weapon_and_clutter )	\
	DXX_COLLISION_TABLE(NO,DO)	\


/* DPH: Put these macros on one long line to avoid CR/LF problems on linux */
#define COLLISION_OF(a,b) (((a)<<4) + (b))

#define DO_COLLISION(type1,type2,collision_function)	\
	case COLLISION_OF( (type1), (type2) ):	\
		collision_function( (A), (B), &collision_point );	\
		break;	\
	case COLLISION_OF( (type2), (type1) ):	\
		collision_function( (B), (A), &collision_point );	\
		break;
#define DO_SAME_COLLISION(type1,collision_function)	\
	case COLLISION_OF( (type1), (type1) ):	\
		collision_function( (A), (B), &collision_point );	\
		break;

//these next two macros define a case that does nothing
#define NO_COLLISION(type1,type2)	\
	case COLLISION_OF( (type1), (type2) ):	\
		break;	\
	case COLLISION_OF( (type2), (type1) ):	\
		break;

#define NO_SAME_COLLISION(type1)	\
	case COLLISION_OF( (type1), (type1) ):	\
		break;

template <typename T, std::size_t V>
struct assert_no_truncation
{
	static_assert(static_cast<T>(V) == V, "truncation error");
};

void collide_two_objects(vobjptridx_t A, vobjptridx_t B, vms_vector &collision_point)
{
	if (B->type < A->type)
	{
		using std::swap;
		swap(A, B);
	}
	uint_fast8_t at = A->type;
	if (at >= MAX_OBJECT_TYPES)
		throw std::runtime_error("illegal object type");
	uint_fast8_t bt = B->type;
	if (bt >= MAX_OBJECT_TYPES)
		throw std::runtime_error("illegal object type");
	uint_fast8_t collision_type = COLLISION_OF(at, bt);
	struct assert_object_type_not_truncated : std::pair<assert_no_truncation<decltype(at), MAX_OBJECT_TYPES>, assert_no_truncation<decltype(bt), MAX_OBJECT_TYPES>> {};
	struct assert_collision_of_not_truncated : assert_no_truncation<decltype(collision_type), COLLISION_OF(MAX_OBJECT_TYPES - 1, MAX_OBJECT_TYPES - 1)> {};
	switch( collision_type )	{
		COLLISION_TABLE(NO,DO)
	default:
		Int3();	//Error( "Unhandled collision_type in collide.c!\n" );
	}
}

#define ENABLE_COLLISION(type1,type2,f)	\
	COLLISION_RESULT(type1,type2,RESULT_CHECK);	\
	COLLISION_RESULT(type2,type1,RESULT_CHECK);

#define DISABLE_COLLISION(type1,type2)	\
	COLLISION_RESULT(type1,type2,RESULT_NOTHING);	\
	COLLISION_RESULT(type2,type1,RESULT_NOTHING);

#define ENABLE_SAME_COLLISION(type,f)	COLLISION_RESULT(type,type,RESULT_CHECK);
#define DISABLE_SAME_COLLISION(type)	COLLISION_RESULT(type,type,RESULT_NOTHING);

template <object_type_t A, object_type_t B>
struct collision_result_t : public tt::conditional<(B < A), collision_result_t<B, A>, tt::integral_constant<ubyte, RESULT_NOTHING>>::type {};

#define COLLISION_RESULT(type1,type2,result)	\
	template <>	\
	struct collision_result_t<type1, type2> : public tt::integral_constant<ubyte, result> {}

COLLISION_TABLE(DISABLE, ENABLE);

template <std::size_t R, std::size_t... C>
static inline constexpr collision_inner_array_t collide_init(index_sequence<C...> c)
{
	static_assert(COLLISION_OF(R, 0) < COLLISION_OF(R, sizeof...(C) - 1), "ambiguous collision");
	static_assert(COLLISION_OF(R, sizeof...(C) - 1) < COLLISION_OF(R + 1, 0), "ambiguous collision");
	return collision_inner_array_t{{
		collision_result_t<static_cast<object_type_t>(R), static_cast<object_type_t>(C)>::value...
	}};
}

template <std::size_t... R, std::size_t... C>
static inline constexpr collision_outer_array_t collide_init(index_sequence<R...>, index_sequence<C...> c)
{
	return collision_outer_array_t{{collide_init<R>(c)...}};
}

const collision_outer_array_t CollisionResult = collide_init(make_tree_index_sequence<MAX_OBJECT_TYPES>(), make_tree_index_sequence<MAX_OBJECT_TYPES>());

#undef DISABLE_COLLISION
#undef ENABLE_COLLISION

#define ENABLE_COLLISION(T1,T2)	static_assert(collision_result_t<T1, T2>::value && collision_result_t<T2, T1>::value, #T1 " " #T2);
#define DISABLE_COLLISION(T1,T2)	static_assert(!collision_result_t<T1, T2>::value && !collision_result_t<T2, T1>::value, #T1 " " #T2);

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
#if defined(DXX_BUILD_DESCENT_II)
	ENABLE_COLLISION( OBJ_PLAYER, OBJ_MARKER );
#endif
	ENABLE_COLLISION( OBJ_DEBRIS, OBJ_WALL );


void collide_object_with_wall(vobjptridx_t A, fix hitspeed, segnum_t hitseg, short hitwall, vms_vector * hitpt)
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
