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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/collide.c,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:41:32 $
 * 
 * $Log: collide.c,v $
 * Revision 1.1.1.1  2006/03/17 19:41:32  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 22:05:46  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.5  1995/07/26  12:07:46  john
 * Made code that pages in weapon_info->robot_hit_vclip not
 * page in unless it is a badass weapon.  Took out old functionallity
 * of using this if no robot exp1_vclip, since all robots have these.
 * 
 * Revision 2.4  1995/03/30  16:36:09  mike
 * text localization.
 * 
 * Revision 2.3  1995/03/24  15:11:13  john
 * Added ugly robot cheat.
 * 
 * Revision 2.2  1995/03/21  14:41:04  john
 * Ifdef'd out the NETWORK code.
 * 
 * Revision 2.1  1995/03/20  18:16:02  john
 * Added code to not store the normals in the segment structure.
 * 
 * Revision 2.0  1995/02/27  11:32:20  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.289  1995/02/22  13:56:06  allender
 * remove anonymous unions from object structure
 * 
 * Revision 1.288  1995/02/11  15:52:45  rob
 * Included text.h.
 * 
 * Revision 1.287  1995/02/11  15:04:11  rob
 * Localized a string.
 * 
 * Revision 1.286  1995/02/11  14:25:41  rob
 * Added invul. controlcen option.
 * 
 * Revision 1.285  1995/02/06  15:53:00  mike
 * create awareness event for player:wall collision.
 * 
 * Revision 1.284  1995/02/05  23:18:17  matt
 * Deal with objects (such as fusion blobs) that get created already
 * poking through a wall
 * 
 * Revision 1.283  1995/02/01  17:51:33  mike
 * fusion bolt can now toast multiple proximity bombs.
 * 
 * Revision 1.282  1995/02/01  17:29:20  john
 * Lintized
 * 
 * Revision 1.281  1995/02/01  15:04:00  rob
 * Changed sound of weapons hitting invulnerable players.
 * 
 * Revision 1.280  1995/01/31  16:16:35  mike
 * Separate smart blobs for robot and player.
 * 
 * Revision 1.279  1995/01/29  15:57:10  rob
 * Fixed another bug with robot_request_change calls.
 * 
 * Revision 1.278  1995/01/28  18:15:06  rob
 * Fixed a bug in multi_request_robot_change.
 * 
 * Revision 1.277  1995/01/27  15:15:44  rob
 * Fixing problems with controlcen damage.
 * 
 * Revision 1.276  1995/01/27  15:13:10  mike
 * comment out mprintf.
 * 
 * Revision 1.275  1995/01/26  22:11:51  mike
 * Purple chromo-blaster (ie, fusion cannon) spruce up (chromification)
 * 
 * Revision 1.274  1995/01/26  18:57:55  rob
 * Changed two uses of digi_play_sample to digi_link_sound_to_pos which
 * made more sense.
 * 
 * Revision 1.273  1995/01/25  23:37:58  mike
 * make persistent objects not hit player more than once.
 * Also, make them hit player before degrading them, else they often did 0 damage.
 * 
 * Revision 1.272  1995/01/25  18:23:54  rob
 * Don't let players pick up powerups in exit tunnel.
 * 
 * Revision 1.271  1995/01/25  13:43:18  rob
 * Added robot transfer for player collisions.
 * Removed mprintf from collide.c on Mike's request.
 * 
 * Revision 1.270  1995/01/25  10:24:01  mike
 * Make sizzle and rock happen in lava even if you're invulnerable.
 * 
 * Revision 1.269  1995/01/22  17:05:33  mike
 * Call multi_robot_request_change when a robot gets whacked by a player or
 * player weapon, if player_num != Player_num
 * 
 * Revision 1.268  1995/01/21  21:20:28  matt
 * Fixed stupid bug
 * 
 * Revision 1.267  1995/01/21  18:47:47  rob
 * Fixed a really dumb bug with player keys.
 * 
 * Revision 1.266  1995/01/21  17:39:30  matt
 * Cleaned up laser/player hit wall confusions
 * 
 * Revision 1.265  1995/01/19  17:44:42  mike
 * damage_force removed, that information coming from strength field.
 * 
 * Revision 1.264  1995/01/18  17:12:56  rob
 * Fixed control stuff for multiplayer.
 * 
 * Revision 1.263  1995/01/18  16:12:33  mike
 * Make control center aware of a cloaked playerr when he fires.
 * 
 * Revision 1.262  1995/01/17  17:48:42  rob
 * Added key syncing for coop players.
 * 
 * Revision 1.261  1995/01/16  19:30:28  rob
 * Fixed an assert error in fireball.c
 * 
 * Revision 1.260  1995/01/16  19:23:51  mike
 * Say Boss_been_hit if he been hit so he gates appropriately.
 * 
 * Revision 1.259  1995/01/16  11:55:16  mike
 * make enemies become aware of player if he damages control center.
 * 
 * Revision 1.258  1995/01/15  16:42:00  rob
 * Fixed problem with robot bumping damage.
 * 
 * Revision 1.257  1995/01/14  19:16:36  john
 * First version of new bitmap paging code.
 * 
 * Revision 1.256  1995/01/03  17:58:37  rob
 * Fixed scoring problems.
 * 
 * Revision 1.255  1994/12/29  12:41:11  rob
 * Tweaking robot exploding in coop.
 * 
 * Revision 1.254  1994/12/28  10:37:59  rob
 * Fixed ifdef of multibot stuff.
 * 
 * Revision 1.253  1994/12/21  19:03:14  rob
 * Fixing score accounting for multiplayer robots
 * 
 * Revision 1.252  1994/12/21  17:36:31  rob
 * Fix hostage pickup problem in network.
 * tweaking robot powerup drops.
 * 
 * Revision 1.251  1994/12/19  20:32:34  rob
 * Remove awareness events from player collisions and lasers that are not the console player.
 * 
 * Revision 1.250  1994/12/19  20:01:22  rob
 * Added multibot.h include.
 * 
 * Revision 1.249  1994/12/19  16:36:41  rob
 * Patches damaging of multiplayer robots.
 * 
 * Revision 1.248  1994/12/14  21:15:18  rob
 * play lava hiss across network.
 * 
 * Revision 1.247  1994/12/14  17:09:09  matt
 * Fixed problem with no sound when lasers hit closed walls, like grates.
 * 
 * Revision 1.246  1994/12/14  09:51:49  mike
 * make any weapon cause proximity bomb detonation.
 * 
 * Revision 1.245  1994/12/13  12:55:25  mike
 * change number of proximity bomb powerups which get dropped.
 * 
 * Revision 1.244  1994/12/12  17:17:53  mike
 * make boss cloak/teleport when get hit, make quad laser 3/4 as powerful.
 * 
 * Revision 1.243  1994/12/12  12:07:51  rob
 * Don't take damage if we're in endlevel sequence.
 * 
 * Revision 1.242  1994/12/11  23:44:52  mike
 * less phys_apply_rot() at higher skill levels.
 * 
 * Revision 1.241  1994/12/11  12:37:02  mike
 * remove stupid robot spinning code.  it was really stupid.  (actually, call here, code in ai.c).
 * 
 * Revision 1.240  1994/12/10  16:44:51  matt
 * Added debugging code to track down door that turns into rock
 * 
 * Revision 1.239  1994/12/09  14:59:19  matt
 * Added system to attach a fireball to another object for rendering purposes,
 * so the fireball always renders on top of (after) the object.
 * 
 * Revision 1.238  1994/12/09  09:57:02  mike
 * Don't allow robots or their weapons to pass through control center.
 * 
 * Revision 1.237  1994/12/08  15:46:03  mike
 * better robot behavior.
 * 
 * Revision 1.236  1994/12/08  12:32:56  mike
 * make boss dying more interesting.
 * 
 * Revision 1.235  1994/12/07  22:49:15  mike
 * tweak rotation due to collision.
 * 
 * Revision 1.234  1994/12/07  16:44:50  mike
 * make bump sound if supposed to, even if not taking damage.
 * 
 * Revision 1.233  1994/12/07  12:55:08  mike
 * tweak rotvel applied from collisions.
 * 
 * Revision 1.232  1994/12/05  19:30:48  matt
 * Fixed horrible segment over-dereferencing
 * 
 * Revision 1.231  1994/12/05  00:32:15  mike
 * do rotvel on badass and bump collisions.
 * 
 * Revision 1.230  1994/12/03  12:49:22  mike
 * don't play bonk sound when you collide with a volatile wall (like lava).
 * 
 * Revision 1.229  1994/12/02  16:51:09  mike
 * make lava sound only happen at 4 Hz.
 * 
 * Revision 1.228  1994/11/30  23:55:27  rob
 * Fixed a bug where a laser hitting a wall was making 2 sounds.
 * 
 * Revision 1.227  1994/11/30  20:11:00  rob
 * Fixed # of dropped laser powerups.
 * 
 * Revision 1.226  1994/11/30  19:19:03  rob
 * Transmit collission sounds for net games.
 * 
 * Revision 1.225  1994/11/30  16:33:01  mike
 * new boss behavior.
 * 
 * Revision 1.224  1994/11/30  15:44:17  mike
 * /2 on boss smart children damage.
 * 
 * Revision 1.223  1994/11/30  14:03:03  mike
 * hook for claw sounds
 * 
 * Revision 1.222  1994/11/29  20:41:09  matt
 * Deleted a bunch of commented-out lines
 * 
 * Revision 1.221  1994/11/27  23:15:08  matt
 * Made changes for new mprintf calling convention
 * 
 * Revision 1.220  1994/11/19  16:11:28  rob
 * Collision damage with walls or lava is counted as suicides in net games
 * 
 * Revision 1.219  1994/11/19  15:20:41  mike
 * rip out unused code and data
 * 
 * Revision 1.218  1994/11/17  18:44:27  rob
 * Added OBJ_GHOST to list of valid player types to create eggs.
 * 
 * Revision 1.217  1994/11/17  14:57:59  mike
 * moved segment validation functions from editor to main.
 * 
 * Revision 1.216  1994/11/16  23:38:36  mike
 * new improved boss teleportation behavior.
 * 
 * Revision 1.215  1994/11/16  12:16:29  mike
 * Enable collisions between robots.  A hack in fvi.c only does this for robots which lunge to attack (eg, green guy)
 * 
 * Revision 1.214  1994/11/15  16:51:50  mike
 * bump player when he hits a volatile wall.
 * 
 * Revision 1.213  1994/11/12  16:38:44  mike
 * allow flares to open doors.
 * 
 * Revision 1.212  1994/11/10  13:09:19  matt
 * Added support for new run-length-encoded bitmaps
 * 
 * Revision 1.211  1994/11/09  17:05:43  matt
 * Fixed problem with volatile walls
 * 
 * Revision 1.210  1994/11/09  12:11:46  mike
 * only award points if ConsoleObject killed robot.
 * 
 * Revision 1.209  1994/11/09  11:11:03  yuan
 * Made wall volatile if either tmap_num1 or tmap_num2 is a volatile wall.
 * 
 * Revision 1.208  1994/11/08  12:20:15  mike
 * moved do_controlcen_destroyed_stuff from here to cntrlcen.c
 * 
 * Revision 1.207  1994/11/02  23:22:08  mike
 * Make ` (backquote, KEY_LAPOSTRO) tell what wall was hit by laser.
 * 
 * Revision 1.206  1994/11/02  18:03:00  rob
 * Fix control_center_been_hit logic so it only cares about the local player.
 * Other players take care of their own control center 'ai'.
 * 
 * Revision 1.205  1994/11/01  19:37:33  rob
 * Changed the max # of consussion missiles to 4.
 * (cause they're lame and clutter things up)
 * 
 * Revision 1.204  1994/11/01  18:06:35  john
 * Tweaked wall banging sound constant.
 * 
 * Revision 1.203  1994/11/01  18:01:40  john
 * Made wall bang less obnoxious, but volume based.
 * 
 * Revision 1.202  1994/11/01  17:11:05  rob
 * Changed some stuff in drop_player_eggs.
 * 
 * Revision 1.201  1994/11/01  12:18:23  john
 * Added sound volume support. Made wall collisions be louder/softer.
 * 
 * Revision 1.200  1994/10/31  13:48:44  rob
 * Fixed bug in opening doors over network/modem.  Added a new message
 * type to multi.c that communicates door openings across the net. 
 * Changed includes in multi.c and wall.c to accomplish this.
 * 
 * Revision 1.199  1994/10/28  14:42:52  john
 * Added sound volumes to all sound calls.
 * 
 * Revision 1.198  1994/10/27  16:58:37  allender
 * added demo recording of monitors blowing up
 * 
 * Revision 1.197  1994/10/26  23:20:52  matt
 * Tone down flash even more
 * 
 * Revision 1.196  1994/10/26  23:01:50  matt
 * Toned down red flash when damaged
 * 
 * Revision 1.195  1994/10/26  15:56:29  yuan
 * Tweaked some palette flashes.
 * 
 * Revision 1.194  1994/10/25  11:32:26  matt
 * Fixed bugs with vulcan powerups in mutliplayer
 * 
 * Revision 1.193  1994/10/25  10:51:18  matt
 * Vulcan cannon powerups now contain ammo count
 * 
 * Revision 1.192  1994/10/24  14:14:05  matt
 * Fixed bug in bump_two_objects()
 * 
 * Revision 1.191  1994/10/23  19:17:04  matt
 * Fixed bug with "no key" messages
 * 
 * Revision 1.190  1994/10/22  00:08:46  matt
 * Fixed up problems with bonus & game sequencing
 * Player doesn't get credit for hostages unless he gets them out alive
 * 
 * Revision 1.189  1994/10/21  20:42:34  mike
 * Clear number of hostages on board between levels.
 * 
 * Revision 1.188  1994/10/20  15:17:43  mike
 * control center in boss handling.
 * 
 * Revision 1.187  1994/10/20  10:09:47  mike
 * Only ever drop 1 shield powerup in multiplayer (as an egg).
 * 
 * Revision 1.186  1994/10/20  09:47:11  mike
 * Fix bug in dropping vulcan ammo in multiplayer.
 * Also control center stuff.
 * 
 * Revision 1.185  1994/10/19  15:14:32  john
 * Took % hits out of player structure, made %kills work properly.
 * 
 * Revision 1.184  1994/10/19  11:33:16  john
 * Fixed hostage rescued percent.
 * 
 * Revision 1.183  1994/10/19  11:16:49  mike
 * Don't allow crazy josh to open locked doors.
 * Don't allow weapons to harm parent.
 * 
 * Revision 1.182  1994/10/18  18:37:01  mike
 * No more hostage killing.  Too much stuff to do to integrate into game.
 * 
 * Revision 1.181  1994/10/18  16:37:35  mike
 * Debug function for Yuan: Show seg:side when hit by puny laser if Show_seg_and_side != 0.
 * 
 * Revision 1.180  1994/10/18  10:53:17  mike
 * Support attack type as a property of a robot, not of being == GREEN_GUY.
 * 
 * Revision 1.179  1994/10/17  21:18:36  mike
 * diminish damage player does to robot due to collision, only took 2-3 hits to kill a josh.
 * 
 * Revision 1.178  1994/10/17  20:30:40  john
 * Made player_hostages_rescued or whatever count properly.
 * 
 * Revision 1.177  1994/10/16  12:42:56  mike
 * Trap bogus amount of vulcan ammo dropping.
 * 
 * Revision 1.176  1994/10/15  19:06:51  mike
 * Drop vulcan ammo if player has it, but no vulcan cannon (when he dies).
 * 
 * Revision 1.175  1994/10/13  15:42:06  mike
 * Remove afterburner.
 * 
 * Revision 1.174  1994/10/13  11:12:57  mike
 * Apply damage to robots.  I hosed it a couple weeks ago when I made the green guy special.
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
//#include "error.h"
#include "mono.h"
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
#include "network.h"
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
#include "multipow.h"

//added on 04/19/99 by Victor Rachels for alt vulcan fire
#include "vlcnfire.h"
//end this section addition - VR

#ifdef EDITOR
#include "editor/editor.h"
#endif

#include "collide.h"

#ifdef SCRIPT
#include "script.h"
#endif

int Ugly_robot_cheat = 0;
int Ugly_robot_texture = 0;

#define STANDARD_EXPL_DELAY (f1_0/4)

//##void collide_fireball_and_wall(object *fireball,fix hitspeed, short hitseg, short hitwall, vms_vector * hitpt)	{
//##	return; 
//##}

//	-------------------------------------------------------------------------------------------------------------
//	The only reason this routine is called (as of 10/12/94) is so Brain guys can open doors.
void collide_robot_and_wall( object * robot, fix hitspeed, short hitseg, short hitwall, vms_vector * hitpt)
{
	if ((robot->id == ROBOT_BRAIN) || (robot->ctype.ai_info.behavior == AIB_RUN_FROM)) {
		int	wall_num = Segments[hitseg].sides[hitwall].wall_num;
		if (wall_num != -1) {
			if ((Walls[wall_num].type == WALL_DOOR) && (Walls[wall_num].keys == KEY_NONE) && (Walls[wall_num].state == WALL_DOOR_CLOSED) && !(Walls[wall_num].flags & WALL_DOOR_LOCKED)) {
				mprintf((0, "Trying to open door at segment %i, side %i\n", hitseg, hitwall));
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

//mprintf((0,"obj %d, damage=%x\n",obj-Objects,damage));

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

			apply_damage_to_player(obj,other_obj,damage);
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
			  	apply_damage_to_player( player, player, damage );
	}

	return;
}

fix	Last_volatile_scrape_sound_time = 0;

void collide_weapon_and_wall( object * weapon, fix hitspeed, short hitseg, short hitwall, vms_vector * hitpt);
void collide_debris_and_wall( object * debris, fix hitspeed, short hitseg, short hitwall, vms_vector * hitpt);

//this gets called when an object is scraping along the wall
void scrape_object_on_wall(object *obj, short hitseg, short hitside, vms_vector * hitpt )
{
	switch (obj->type) {

		case OBJ_PLAYER:

			if (obj->id==Player_num) {
				fix d;
				//mprintf((0, "Scraped segment #%3i, side #%i\n", hitseg, hitside));
				if ((d=TmapInfo[Segments[hitseg].sides[hitside].tmap_num].damage) > 0) {
					vms_vector	hit_dir, rand_vec;
					fix damage = fixmul(d,FrameTime);
		
					if (!(Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE))
						apply_damage_to_player( obj, obj, damage );
		
					PALETTE_FLASH_ADD(f2i(damage*4), 0, 0);	//flash red
					if ((GameTime > Last_volatile_scrape_sound_time + F1_0/4) || (GameTime < Last_volatile_scrape_sound_time)) {
						Last_volatile_scrape_sound_time = GameTime;
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
		
				} else {
					//what scrape sound
					//PLAY_SOUND( SOUND_PLAYER_SCRAPE_WALL );
				}
		
			}

			break;

		//these two kinds of objects below shouldn't really slide, so
		//if this scrape routine gets called (which it might if the
		//object (such as a fusion blob) was created already poking
		//through the wall) call the collide routine.

		case OBJ_WEAPON:
			collide_weapon_and_wall(obj,0,hitseg,hitside,hitpt); 
			break;

		case OBJ_DEBRIS:		
			collide_debris_and_wall(obj,0,hitseg,hitside,hitpt); 
			break;
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

				//mprintf((0,"u,v = %x,%x   x,y=%x,%x",u,v,x,y));

				
				if (bm->bm_flags & BM_FLAG_RLE)
					bm = rle_expand_texture(bm);

				if (gr_gpixel (bm, x, y) != 255) {		//not trans, thus on effect
					int vc,sound_num;

					//mprintf((0,"  HIT!\n"));

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

						mprintf((0,"bm_num = %d\n",bm_num));

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
				//else
				//	mprintf((0,"\n"));

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

	#ifndef NDEBUG
	if (keyd_pressed[KEY_LAPOSTRO])
		if (weapon->ctype.laser_info.parent_num == Players[Player_num].objnum)
			if (weapon->id < 4)
				mprintf((0, "Your laser hit at segment = %i, side = %i\n", hitseg, hitwall));
	#endif

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
				//mprintf((0, "Weapon %i hits wall, but has silent bit set.\n", weapon-Objects));
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
//	mprintf((0, "Coll: [%2i %4i %4i %4i] [%2i %4i %4i %4i] at [%4i %4i %4i]", 
//		robot1-Objects, f2i(robot1->pos.x), f2i(robot1->pos.y), f2i(robot1->pos.z),
//		robot2-Objects, f2i(robot2->pos.x), f2i(robot2->pos.y), f2i(robot2->pos.z),
//		f2i(collision_point->x), f2i(collision_point->y), f2i(collision_point->z)));

	bump_two_objects(robot1, robot2, 1);
	return; 
}

void collide_robot_and_controlcen( object * obj1, object * obj2, vms_vector *collision_point )
{

	if (obj1->type == OBJ_ROBOT) {
		vms_vector	hitvec;
		vm_vec_normalize_quick(vm_vec_sub(&hitvec, &obj2->pos, &obj1->pos));
		bump_one_object(obj1, &hitvec, 0);
	} else {
		vms_vector	hitvec;
		vm_vec_normalize_quick(vm_vec_sub(&hitvec, &obj1->pos, &obj2->pos));
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
		multi_robot_request_change(robot, player->id);
#endif
#endif

	digi_link_sound_to_pos( SOUND_ROBOT_HIT_PLAYER, player->segnum, 0, collision_point, 0, F1_0 );
	bump_two_objects(robot, player, 1);
	return; 
}

// Provide a way for network message to instantly destroy the control center
// without awarding points or anything.

//	if controlcen == NULL, that means don't do the explosion because the control center
//	was actually in another object.
void net_destroy_controlcen(object *controlcen)
{
	if (Fuelcen_control_center_destroyed != 1) {
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
		mprintf((0, "Damage to control center by object of type %i prevented by MK!\n", whotype));
		return;
	}

	#ifdef NETWORK
	#ifndef SHAREWARE
	if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP) && (Players[Player_num].time_level < Netgame.control_invul_time))
	{
		if (Objects[who].id == Player_num) {
			int secs = f2i(Netgame.control_invul_time-Players[Player_num].time_level) % 60;
			int mins = f2i(Netgame.control_invul_time-Players[Player_num].time_level) / 60;
			hud_message(MSGC_MINE_FEEDBACK, "%s %d:%02d.", TXT_CNTRLCEN_INVUL, mins, secs);
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

	if ( Ugly_robot_cheat == 0xBADa55 )	{
		robot->rtype.pobj_info.tmap_override = Ugly_robot_texture % NumTextures;
	}

	//	If a persistent weapon hit robot most recently, quick abort, else we cream the same robot many times,
	//	depending on frame rate.
	if (weapon->mtype.phys_info.flags & PF_PERSISTENT) {
		if (weapon->ctype.laser_info.last_hitobj == robot-Objects)
			return;
		else
			weapon->ctype.laser_info.last_hitobj = robot-Objects;

		// mprintf((0, "weapon #%i with power %i hits robot #%i.\n", weapon - Objects, f2i(weapon->shields), robot - Objects));
	}

//	if (weapon->ctype.laser_info.multiplier)
//		mprintf((0, "Weapon #%3i hit object %3i in frame %4i: multiplier = %7.3f, power = %i\n", weapon-Objects, robot-Objects, FrameCount, f2fl(weapon->ctype.laser_info.multiplier), f2i(weapon->shields)));

//mprintf((0, "weapon #%i hits robot #%i.\n", weapon - Objects, robot - Objects));

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
	digi_link_sound_to_pos( SOUND_ROBOT_HIT_PLAYER, player1->segnum, 0, collision_point, 0, F1_0 );
	bump_two_objects(player1, player2, 1);
	return;
}

// drop powerups in pow_count around object obj.
// special behavior:
// -  POW_MISSILE_1 and POW_HOMING_AMMO_1 are split into
//	  POW_MISSILE_1/4 and POW_HOMING_AMMO_1/4 respectivily.
// -  for single player:
//    when dropping POW_VULCAN_WEAPON it gets all ammo (POW_VULCAN_AMMO),
//	  and no vulcan ammo is dropped.
//    for multi player:
//    when dropping POW_VULCAN_WEAPON it gets VULCAN_WEAPON_AMMO_AMOUNT
//    ammo, and that's subtracted from the to be dropped ammo packages.
// -  when dropping vulcan ammo w/o cannon, ammo is limited to 200 max
void drop_pow_count(object *obj, int *pow_count)
{
	int i, j;
	int count;
	int objnum;
	int powerup_in_level[MAX_POWERUP_TYPES];

	pow_count_level(powerup_in_level);

	for (j = 0; j < NUM_PLAYER_DROP_POWERUPS; j++)
         {
		count = pow_count[(i = player_drop_powerups[j])];

		// limit powerups in D1X network games
#ifdef NETWORK
		if ((Game_mode & GM_NETWORK) &&
                    (Netgame.protocol_version == MULTI_PROTO_D1X_VER))
                 {
                   mprintf((1, "want %d start %d now %d\n", count, powerup_start_level[i], powerup_in_level[i]));
                    if (multi_allow_powerup_mask[i])
                     { // only check 'important' powerups (no shield,energy,conc)
                       int pow_max = MAX(powerup_start_level[i] - powerup_in_level[i], 0);
             
//-killed-                     #ifdef NETWORK
                        while (count > pow_max)
                         {
                           // create dummy Net_create_objnums entries to keep objnums in sync
                            if (Net_create_loc >= MAX_NET_CREATE_OBJECTS)
                             {
                               mprintf( (0, "Not enough slots to drop all powerups!\n" ));
                               return;
                             }
                           Net_create_objnums[Net_create_loc++] = -1;
                           count--;
                         }
//-killed-                    #else
//-killed-                    if (count > pow_max) count = pow_max;
//-killed-                    #endif
                    }
                 }
#endif

		if (count > 0)
			switch (i)
                         {
				case POW_MISSILE_1:
				case POW_HOMING_AMMO_1:
					call_object_create_egg(obj, count / 4, OBJ_POWERUP, i + 1);
					call_object_create_egg(obj, count % 4, OBJ_POWERUP, i);
					break;
				case POW_VULCAN_WEAPON:
					if ((objnum = call_object_create_egg(obj, count, OBJ_POWERUP, POW_VULCAN_WEAPON)) != -1)
                                         {
                                                //added/changed on 9/5/98 by adb (from Matthew Mueller) to drop ammo in multi even with cannon
						if (Game_mode & GM_MULTI) {
							Objects[objnum].ctype.powerup_info.count = VULCAN_WEAPON_AMMO_AMOUNT;
						} else
							Objects[objnum].ctype.powerup_info.count = pow_count[POW_VULCAN_AMMO];
                                                //end change - adb (from Matthew Mueller)
                                         }
					break;
				case POW_VULCAN_AMMO:
					//changed by adb on 980905 to drop ammo in multi even with cannon
					if (pow_count[POW_VULCAN_WEAPON])
						count -= VULCAN_WEAPON_AMMO_AMOUNT;
					if (count > 0 && (pow_count[POW_VULCAN_WEAPON] == 0 || (Game_mode & GM_MULTI))) {
						if (count > 200) {
							mprintf((0, "Surprising amount of vulcan ammo: %d bullets.\n", count));
							count = 200;
						}
						call_object_create_egg(obj, (count + VULCAN_AMMO_AMOUNT - 1) / VULCAN_AMMO_AMOUNT, OBJ_POWERUP, POW_VULCAN_AMMO);
					}
					//end changes by adb
					break;
				default:
						call_object_create_egg(obj, count, OBJ_POWERUP, i);
                         }
         }
}

void drop_player_eggs(object *player)
{
	if ((player->type == OBJ_PLAYER) || (player->type == OBJ_GHOST)) {
		int	pnum = player->id;
		int pow_count[MAX_POWERUP_TYPES];

		// Seed the random number generator so in net play the eggs will always
		// drop the same way
		#ifdef NETWORK
		if (Game_mode & GM_MULTI) 
		{
			Net_create_loc = 0;
			d_srand(5483L);
		}
		#endif

		player_to_pow_count(&Players[pnum], pow_count);
		clip_player_pow_count(pow_count);
		drop_pow_count(player, pow_count);
	}
}
//added/killed on 10/1/98 by Victor Rachels to get rid of that stupid #if 1
//-killed-int maybe_drop_primary_weapon_egg(object *player, int weapon_flag, int powerup_num)
//-killed-{
//-killed-        if (Players[player->id].primary_weapon_flags & weapon_flag)
//-killed-                return call_object_create_egg(player, 1, OBJ_POWERUP, powerup_num);
//-killed-        else
//-killed-                return -1;
//-killed-}
//-killed-
//-killed-void maybe_drop_secondary_weapon_egg(object *player, int weapon_flag, int powerup_num, int count)
//-killed-{
//-killed-        if (Players[player->id].secondary_weapon_flags & weapon_flag) {
//-killed-                int     i, max_count;
//-killed-
//-killed-                max_count = min(count, 3);
//-killed-                for (i=0; i<max_count; i++)
//-killed-                        call_object_create_egg(player, 1, OBJ_POWERUP, powerup_num);
//-killed-        }
//-killed-}
//-killed-
//-killed-void drop_player_eggs(object *player)
//-killed-{
//-killed-//      mprintf((0, "In drop_player_eggs...\n"));
//-killed-
//-killed-        if ((player->type == OBJ_PLAYER) || (player->type == OBJ_GHOST)) {
//-killed-                int     num_missiles = 1;
//-killed-                int     pnum = player->id;
//-killed-                int     objnum;
//-killed-
//-killed-                // Seed the random number generator so in net play the eggs will always
//-killed-                // drop the same way
//-killed-                #ifdef NETWORK
//-killed-                if (Game_mode & GM_MULTI) 
//-killed-                {
//-killed-                        Net_create_loc = 0;
//-killed-                        d_srand(5483L);
//-killed-                }
//-killed-                #endif
//-killed-
//-killed-                //      If the player dies and he has powerful lasers, create the powerups here.
//-killed-
//-killed-                if (Players[pnum].laser_level >= 1)
//-killed-                        call_object_create_egg(player, (Players[pnum].laser_level), OBJ_POWERUP, POW_LASER);
//-killed-
//-killed-                //      Drop quad laser if appropos
//-killed-                if (Players[pnum].flags & PLAYER_FLAGS_QUAD_LASERS)
//-killed-                        call_object_create_egg(player, 1, OBJ_POWERUP, POW_QUAD_FIRE);
//-killed-
//-killed-                if (Players[pnum].flags & PLAYER_FLAGS_CLOAKED)
//-killed-                        call_object_create_egg(player, 1, OBJ_POWERUP, POW_CLOAK);
//-killed-
//-killed-                //      Drop the primary weapons
//-killed-                objnum = maybe_drop_primary_weapon_egg(player, HAS_VULCAN_FLAG, POW_VULCAN_WEAPON);
//-killed-                if (objnum!=-1)
//-killed-                 {
//-killed-//added/changed on 8/27/98 by Victor Rachels from Matt Mueller
//-killed-//                        Objects[objnum].ctype.powerup_info.count = Players[pnum].primary_ammo[VULCAN_INDEX];
//-killed-                 if (Game_mode & GM_MULTI)
//-killed-                  {
//-killed-                   Objects[objnum].ctype.powerup_info.count = VULCAN_WEAPON_AMMO_AMOUNT;
//-killed-                   Players[pnum].primary_ammo[VULCAN_INDEX]-= VULCAN_WEAPON_AMMO_AMOUNT;
//-killed-                  } else
//-killed-                     Objects[objnum].ctype.powerup_info.count = Players[pnum].primary_ammo[VULCAN_INDEX];
//-killed-//end this section edit - Victor Rachels from Matt Mueller
//-killed-                 }
//-killed-                maybe_drop_primary_weapon_egg(player, HAS_SPREADFIRE_FLAG, POW_SPREADFIRE_WEAPON);
//-killed-                maybe_drop_primary_weapon_egg(player, HAS_PLASMA_FLAG, POW_PLASMA_WEAPON);
//-killed-                maybe_drop_primary_weapon_egg(player, HAS_FUSION_FLAG, POW_FUSION_WEAPON);
//-killed-
//-killed-                //      Drop the secondary weapons
//-killed-                //      Note, proximity weapon only comes in packets of 4.  So drop n/2, but a max of 3 (handled inside maybe_drop..)  Make sense?
//-killed-                maybe_drop_secondary_weapon_egg(player, HAS_PROXIMITY_FLAG, POW_PROXIMITY_WEAPON, (Players[player->id].secondary_ammo[PROXIMITY_INDEX]+2)/4);
//-killed-                maybe_drop_secondary_weapon_egg(player, HAS_SMART_FLAG, POW_SMARTBOMB_WEAPON, Players[player->id].secondary_ammo[SMART_INDEX]);
//-killed-                maybe_drop_secondary_weapon_egg(player, HAS_MEGA_FLAG, POW_MEGA_WEAPON, Players[player->id].secondary_ammo[MEGA_INDEX]);
//-killed-
//-killed-                num_missiles = Players[pnum].secondary_ammo[HOMING_INDEX];
//-killed-                if (num_missiles > 6)
//-killed-                        num_missiles = 6;
//-killed-                call_object_create_egg(player, num_missiles/4, OBJ_POWERUP, POW_HOMING_AMMO_4);
//-killed-                call_object_create_egg(player, num_missiles%4, OBJ_POWERUP, POW_HOMING_AMMO_1);
//-killed-
//-killed-                //      If player has vulcan ammo, but no vulcan cannon, drop the ammo.
//-killed-//added/changed on 8/27/98 by Victor Rachels from Matt Mueller
//-killed-//              if (!(Players[player->id].primary_weapon_flags & HAS_VULCAN_FLAG)) {
//-killed-                if (!(Players[player->id].primary_weapon_flags & HAS_VULCAN_FLAG)||(Game_mode & GM_MULTI)) {
//-killed-//end change - Victor Rachels from Matt Mueller
//-killed-                        int     amount = Players[player->id].primary_ammo[VULCAN_INDEX];
//-killed-                        if (amount > 200) {
//-killed-                                mprintf((0, "Surprising amount of vulcan ammo: %i bullets.\n", amount));
//-killed-                                amount = 200;
//-killed-                        }
//-killed-                        while (amount > 0) {
//-killed-                                call_object_create_egg(player, 1, OBJ_POWERUP, POW_VULCAN_AMMO);
//-killed-                                amount -= VULCAN_AMMO_AMOUNT;
//-killed-                        }
//-killed-                }
//-killed-
//-killed-                //      Drop the player's missiles.
//-killed-                num_missiles = Players[pnum].secondary_ammo[CONCUSSION_INDEX];
//-killed-                if (num_missiles > 4)
//-killed-                        num_missiles = 4;
//-killed-
//-killed-                call_object_create_egg(player, num_missiles/4, OBJ_POWERUP, POW_MISSILE_4);
//-killed-                call_object_create_egg(player, num_missiles%4, OBJ_POWERUP, POW_MISSILE_1);
//-killed-
//-killed-                //      Always drop a shield and energy powerup.
//-killed-                if (Game_mode & GM_MULTI) {
//-killed-                        call_object_create_egg(player, 1, OBJ_POWERUP, POW_SHIELD_BOOST);
//-killed-                        call_object_create_egg(player, 1, OBJ_POWERUP, POW_ENERGY);
//-killed-                }
//-killed-
//-killed-//--            //      Drop all the keys.
//-killed-//--            if (Players[Player_num].flags & PLAYER_FLAGS_BLUE_KEY) {
//-killed-//--                    player->contains_count = 1;
//-killed-//--                    player->contains_type = OBJ_POWERUP;
//-killed-//--                    player->contains_id = POW_KEY_BLUE;
//-killed-//--                    object_create_egg(player);
//-killed-//--            }
//-killed-//--            if (Players[Player_num].flags & PLAYER_FLAGS_RED_KEY) {
//-killed-//--                    player->contains_count = 1;
//-killed-//--                    player->contains_type = OBJ_POWERUP;
//-killed-//--                    player->contains_id = POW_KEY_RED;
//-killed-//--                    object_create_egg(player);
//-killed-//--            }
//-killed-//--            if (Players[Player_num].flags & PLAYER_FLAGS_GOLD_KEY) {
//-killed-//--                    player->contains_count = 1;
//-killed-//--                    player->contains_type = OBJ_POWERUP;
//-killed-//--                    player->contains_id = POW_KEY_GOLD;
//-killed-//--                    object_create_egg(player);
//-killed-//--            }
//-killed-        }
//-killed-}


void apply_damage_to_player(object *player, object *killer, fix damage)
{
	if (Player_is_dead)
		return;

	if (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE)
		return;

	if (Endlevel_sequence)
		return;

	//for the player, the 'real' shields are maintained in the Players[]
	//array.  The shields value in the player's object are, I think, not
	//used anywhere.  This routine, however, sets the objects shields to
	//be a mirror of the value in the Player structure. 

	if (player->id == Player_num) {		//is this the local player?

		if (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE) {

			//invincible, so just do blue flash

			PALETTE_FLASH_ADD(0,0,f2i(damage)*4);	//flash blue

		} 
		else {		//take damage, do red flash

			Players[Player_num].shields -= damage;

			PALETTE_FLASH_ADD(f2i(damage)*4,-f2i(damage/2),-f2i(damage/2));	//flash red

		}

		if (Players[Player_num].shields < 0)	{

  			Players[Player_num].killer_objnum = killer-Objects;
			
//			if ( killer && (killer->type == OBJ_PLAYER))
//				Players[Player_num].killer_objnum = killer-Objects;

			player->flags |= OF_SHOULD_BE_DEAD;

//added on 04/19/99 by Victor Rachels for alt vulcan fire
                 #ifdef NETWORK
                         if(use_alt_vulcanfire)
                          do_vulcan_fire(0);
                 #endif
//end this section addition - VR
		}

		player->shields = Players[Player_num].shields;		//mirror

	}
}

void collide_player_and_weapon( object * player, object * weapon, vms_vector *collision_point )
{
	fix		damage = weapon->shields;
	object * killer=NULL;

	damage = fixmul(damage, weapon->ctype.laser_info.multiplier);

        if (weapon->mtype.phys_info.flags & PF_PERSISTENT) {
		if (weapon->ctype.laser_info.last_hitobj == player-Objects)
			return;
		else
			weapon->ctype.laser_info.last_hitobj = player-Objects;
        }

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
			apply_damage_to_player( player, killer, damage);
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

	apply_damage_to_player( player, robot, F1_0*(Difficulty_level+1));

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

	apply_damage_to_player( objp, NULL, 4*F1_0);

	return; 

}

void collide_robot_and_materialization_center(object *objp)
{
	int	side;
	vms_vector	exit_dir;
	segment *segp=&Segments[objp->segnum];

	digi_link_sound_to_pos(SOUND_ROBOT_HIT, objp->segnum, 0, &objp->pos, 0, F1_0);
//	digi_play_sample( SOUND_ROBOT_HIT, F1_0 );

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

	//mprintf( (0, "Object %d of type %d collided with object %d of type %d\n", A-Objects,A->type, B-Objects, B->type ));

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



