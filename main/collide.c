/* $Id: collide.c,v 1.10 2003-01-02 23:13:21 btb Exp $ */
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
 * Routines to handle collisions
 *
 * Old Log:
 * Revision 1.3  1995/11/08  17:15:21  allender
 * make collide_player_and_weapon play player_hit_sound if
 * shareware and not my playernum
 *
 * Revision 1.2  1995/10/31  10:24:37  allender
 * shareware stuff
 *
 * Revision 1.1  1995/05/16  15:23:34  allender
 * Initial revision
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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

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
#include "render.h"
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
#ifdef NETWORK
#include "network.h"
#endif
#include "newmenu.h"
#include "scores.h"
#include "effects.h"
#include "textures.h"
#ifdef NETWORK
#include "multi.h"
#endif
#include "cntrlcen.h"
#include "newdemo.h"
#include "endlevel.h"
#include "multibot.h"
#include "piggy.h"
#include "text.h"
#include "automap.h"
#include "switch.h"
#include "palette.h"

#ifdef TACTILE
#include "tactile.h"
#endif 

#ifdef EDITOR
#include "editor/editor.h"
#endif

#include "collide.h"

#define STANDARD_EXPL_DELAY (f1_0/4)

//##void collide_fireball_and_wall(object *fireball,fix hitspeed, short hitseg, short hitwall, vms_vector * hitpt)	{
//##	return; 
//##}

//	-------------------------------------------------------------------------------------------------------------
//	The only reason this routine is called (as of 10/12/94) is so Brain guys can open doors.
void collide_robot_and_wall( object * robot, fix hitspeed, short hitseg, short hitwall, vms_vector * hitpt)
{
	ai_local		*ailp = &Ai_local_info[robot-Objects];

	if ((robot->id == ROBOT_BRAIN) || (robot->ctype.ai_info.behavior == AIB_RUN_FROM) || (Robot_info[robot->id].companion == 1) || (robot->ctype.ai_info.behavior == AIB_SNIPE)) {
		int	wall_num = Segments[hitseg].sides[hitwall].wall_num;
		if (wall_num != -1) {
			if ((Walls[wall_num].type == WALL_DOOR) && (Walls[wall_num].keys == KEY_NONE) && (Walls[wall_num].state == WALL_DOOR_CLOSED) && !(Walls[wall_num].flags & WALL_DOOR_LOCKED)) {
				// -- mprintf((0, "Trying to open door at segment %i, side %i\n", hitseg, hitwall));
				wall_open_door(&Segments[hitseg], hitwall);
			// -- Changed from this, 10/19/95, MK: Don't want buddy getting stranded from player
			//-- } else if ((Robot_info[robot->id].companion == 1) && (Walls[wall_num].type == WALL_DOOR) && (Walls[wall_num].keys != KEY_NONE) && (Walls[wall_num].state == WALL_DOOR_CLOSED) && !(Walls[wall_num].flags & WALL_DOOR_LOCKED)) {
			} else if ((Robot_info[robot->id].companion == 1) && (Walls[wall_num].type == WALL_DOOR)) {
				if ((ailp->mode == AIM_GOTO_PLAYER) || (Escort_special_goal == ESCORT_GOAL_SCRAM)) {
					if (Walls[wall_num].keys != KEY_NONE) {
						if (Walls[wall_num].keys & Players[Player_num].flags)
							wall_open_door(&Segments[hitseg], hitwall);
					} else if (!(Walls[wall_num].flags & WALL_DOOR_LOCKED))
						wall_open_door(&Segments[hitseg], hitwall);
				}
			} else if (Robot_info[robot->id].thief) {		//	Thief allowed to go through doors to which player has key.
				if (Walls[wall_num].keys != KEY_NONE)
					if (Walls[wall_num].keys & Players[Player_num].flags)
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

char	Monster_mode = 0;		//	A cheat.  Do massive damage when collide.

//given the specified force, apply damage from that force to an object
void apply_force_damage(object *obj,fix force,object *other_obj)
{
	int	result;
	fix damage;

	if (obj->flags & (OF_EXPLODING|OF_SHOULD_BE_DEAD))
		return;		//already exploding or dead

	damage = fixdiv(force,obj->mtype.phys_info.mass) / 8;

	if ((other_obj->type == OBJ_PLAYER) && Monster_mode)
		damage = 0x7fffffff;

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

			//	If colliding with a claw type robot, do damage proportional to FrameTime because you can collide with those
			//	bots every frame since they don't move.
			if ( (other_obj->type == OBJ_ROBOT) && (Robot_info[other_obj->id].attack_type) )
				damage = fixmul(damage, FrameTime*2);

			//	Make trainee easier.
			if (Difficulty_level == 0)
				damage /= 2;

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

	if (! (objp->mtype.phys_info.flags & PF_PERSISTENT))
	{
		if (objp->type == OBJ_PLAYER) {
			vms_vector force2;
			force2.x = force->x/4;
			force2.y = force->y/4;
			force2.z = force->z/4;
			phys_apply_force(objp,&force2);
			if (damage_flag && ((other_objp->type != OBJ_ROBOT) || !Robot_info[other_objp->id].companion)) {
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

fix force_force = i2f(50);

void collide_player_and_wall( object * playerobj, fix hitspeed, short hitseg, short hitwall, vms_vector * hitpt)
{
	fix damage;
	char ForceFieldHit=0;
	int tmap_num,tmap_num2;

	if (playerobj->id != Player_num) // Execute only for local player
		return;

	tmap_num = Segments[hitseg].sides[hitwall].tmap_num;

	//	If this wall does damage, don't make *BONK* sound, we'll be making another sound.
	if (TmapInfo[tmap_num].damage > 0)
		return;

	if (TmapInfo[tmap_num].flags & TMI_FORCE_FIELD) {
		vms_vector force;

		PALETTE_FLASH_ADD(0, 0, 60);	//flash blue

		//knock player around
		force.x = 40*(d_rand() - 16384);
		force.y = 40*(d_rand() - 16384);
		force.z = 40*(d_rand() - 16384);
		phys_apply_rot(playerobj, &force);

#ifdef TACTILE
		if (TactileStick)
		 Tactile_apply_force (&force,&playerobj->orient);
#endif

		//make sound
		digi_link_sound_to_pos( SOUND_FORCEFIELD_BOUNCE_PLAYER, hitseg, 0, hitpt, 0, f1_0 );
#ifdef NETWORK
		if (Game_mode & GM_MULTI)
			multi_send_play_sound(SOUND_FORCEFIELD_BOUNCE_PLAYER, f1_0);
#endif
		ForceFieldHit=1;
	} 
	else {

	#ifdef TACTILE
		vms_vector force;
		if (TactileStick) {
			force.x = -playerobj->mtype.phys_info.velocity.x;
			force.y = -playerobj->mtype.phys_info.velocity.y;
			force.z = -playerobj->mtype.phys_info.velocity.z;
			Tactile_do_collide(&force, &playerobj->orient);
		}
	#endif

    	wall_hit_process( &Segments[hitseg], hitwall, 20, playerobj->id, playerobj );
	}

	//	** Damage from hitting wall **
	//	If the player has less than 10% shields, don't take damage from bump
	// Note: Does quad damage if hit a force field - JL
	damage = (hitspeed / DAMAGE_SCALE) * (ForceFieldHit*8 + 1);

	tmap_num2 = Segments[hitseg].sides[hitwall].tmap_num2;

	//don't do wall damage and sound if hit lava or water
	if ((TmapInfo[tmap_num].flags & (TMI_WATER|TMI_VOLATILE)) || (tmap_num2 && (TmapInfo[tmap_num2&0x3fff].flags & (TMI_WATER|TMI_VOLATILE))))
		damage = 0;

	if (damage >= DAMAGE_THRESHOLD) {
		int	volume;
		volume = (hitspeed-(DAMAGE_SCALE*DAMAGE_THRESHOLD)) / WALL_LOUDNESS_SCALE ;

		create_awareness_event(playerobj, PA_WEAPON_WALL_COLLISION);

		if ( volume > F1_0 )
			volume = F1_0;
		if (volume > 0 && !ForceFieldHit) {  // uhhhgly hack
			digi_link_sound_to_pos( SOUND_PLAYER_HIT_WALL, hitseg, 0, hitpt, 0, volume );
			#ifdef NETWORK
			if (Game_mode & GM_MULTI)
				multi_send_play_sound(SOUND_PLAYER_HIT_WALL, volume);	
			#endif
		}

		if (!(Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE))
			if ( Players[Player_num].shields > f1_0*10 || ForceFieldHit)
			  	apply_damage_to_player( playerobj, playerobj, damage );

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

fix	Last_volatile_scrape_sound_time = 0;

void collide_weapon_and_wall( object * weapon, fix hitspeed, short hitseg, short hitwall, vms_vector * hitpt);
void collide_debris_and_wall( object * debris, fix hitspeed, short hitseg, short hitwall, vms_vector * hitpt);

//see if wall is volatile or water
//if volatile, cause damage to player  
//returns 1=lava, 2=water
int check_volatile_wall(object *obj,int segnum,int sidenum,vms_vector *hitpt)
{
	fix tmap_num,d,water;

	Assert(obj->type==OBJ_PLAYER);

	tmap_num = Segments[segnum].sides[sidenum].tmap_num;

	d = TmapInfo[tmap_num].damage;
	water = (TmapInfo[tmap_num].flags & TMI_WATER);

	if (d > 0 || water) {

		if (obj->id == Player_num) {

			if (d > 0) {
				fix damage = fixmul(d,FrameTime);

				if (Difficulty_level == 0)
					damage /= 2;

				if (!(Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE))
					apply_damage_to_player( obj, obj, damage );

#ifdef TACTILE
				if (TactileStick)
				 Tactile_Xvibrate (50,25);
#endif

				PALETTE_FLASH_ADD(f2i(damage*4), 0, 0);	//flash red
			}

			obj->mtype.phys_info.rotvel.x = (d_rand() - 16384)/2;
			obj->mtype.phys_info.rotvel.z = (d_rand() - 16384)/2;
		}

		return (d>0)?1:2;
	}
	else
	 {
#ifdef TACTILE
		if (TactileStick && !(FrameCount & 15))
		 Tactile_Xvibrate_clear ();
#endif

		return 0;
	 }
}

//this gets called when an object is scraping along the wall
void scrape_object_on_wall(object *obj, short hitseg, short hitside, vms_vector * hitpt )
{
	switch (obj->type) {

		case OBJ_PLAYER:

			if (obj->id==Player_num) {
				int type;

				//mprintf((0, "Scraped segment #%3i, side #%i\n", hitseg, hitside));

				if ((type=check_volatile_wall(obj,hitseg,hitside,hitpt))!=0) {
					vms_vector	hit_dir, rand_vec;

					if ((GameTime > Last_volatile_scrape_sound_time + F1_0/4) || (GameTime < Last_volatile_scrape_sound_time)) {
						int sound = (type==1)?SOUND_VOLATILE_WALL_HISS:SOUND_SHIP_IN_WATER;

						Last_volatile_scrape_sound_time = GameTime;

						digi_link_sound_to_pos( sound, hitseg, 0, hitpt, 0, F1_0 );
#ifdef NETWORK
						if (Game_mode & GM_MULTI)
							multi_send_play_sound(sound, F1_0);
#endif
					}

					#ifdef COMPACT_SEGS
						get_side_normal(&Segments[hitseg], higside, 0, &hit_dir );	
					#else
						hit_dir = Segments[hitseg].sides[hitside].normals[0];
					#endif
			
					make_random_vector(&rand_vec);
					vm_vec_scale_add2(&hit_dir, &rand_vec, F1_0/8);
					vm_vec_normalize_quick(&hit_dir);
					bump_one_object(obj, &hit_dir, F1_0*8);
				}

				//@@} else {
				//@@	//what scrape sound
				//@@	//PLAY_SOUND( SOUND_PLAYER_SCRAPE_WALL );
				//@@}
		
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
int check_effect_blowup(segment *seg,int side,vms_vector *pnt, object *blower, int force_blowup_flag)
{
	int tm,tmf,ec,db;

	db=0;

	//	If this wall has a trigger and the blower-upper is not the player or the buddy, abort!
	{
		int	ok_to_blow = 0;

		if (blower->ctype.laser_info.parent_type == OBJ_ROBOT)
			if (Robot_info[Objects[blower->ctype.laser_info.parent_num].id].companion)
				ok_to_blow = 1;

		if (!(ok_to_blow || (blower->ctype.laser_info.parent_type == OBJ_PLAYER))) {
			int	trigger_num, wall_num;

			wall_num = seg->sides[side].wall_num;
			if ( wall_num != -1 ) {
				trigger_num = Walls[wall_num].trigger;

				if (trigger_num != -1)
					return 0;
			}
		}
	}


	if ((tm=seg->sides[side].tmap_num2) != 0) {

		tmf = tm&0xc000;		//tm flags
		tm &= 0x3fff;			//tm without flags

		//check if it's an animation (monitor) or casts light
		if ((((ec=TmapInfo[tm].eclip_num)!=-1) && ((db=Effects[ec].dest_bm_num)!=-1 && !(Effects[ec].flags&EF_ONE_SHOT))) ||	(ec==-1 && (TmapInfo[tm].destroyed!=-1))) {
			fix u,v;
			grs_bitmap *bm = &GameBitmaps[Textures[tm].index];
			int x=0,y=0,t;

			PIGGY_PAGE_IN(Textures[tm]);

			//this can be blown up...did we hit it?

			if (!force_blowup_flag) {
				find_hitpoint_uv(&u,&v,NULL,pnt,seg,side,0);	//evil: always say face zero

				x = ((unsigned) f2i(u*bm->bm_w)) % bm->bm_w;
				y = ((unsigned) f2i(v*bm->bm_h)) % bm->bm_h;

				switch (tmf) {		//adjust for orientation of paste-on
					case 0x0000:	break;
					case 0x4000:	t=y; y=x; x=bm->bm_w-t-1; break;
					case 0x8000:	y=bm->bm_h-y-1; x=bm->bm_w-x-1; break;
					case 0xc000:	t=x; x=y; y=bm->bm_h-t-1; break;
				}

				//mprintf((0,"u,v = %x,%x   x,y=%x,%x",u,v,x,y));
			
				if (bm->bm_flags & BM_FLAG_RLE)
					bm = rle_expand_texture(bm);
			}

			if (force_blowup_flag || (bm->bm_data[y*bm->bm_w+x] != TRANSPARENCY_COLOR)) {		//not trans, thus on effect
				int vc,sound_num;
				fix dest_size;


#ifdef NETWORK
				if ((Game_mode & GM_MULTI) && Netgame.AlwaysLighting)
			   	if (!(ec!=-1 && db!=-1 && !(Effects[ec].flags&EF_ONE_SHOT)))
				   	return(0);
#endif

				//mprintf((0,"  HIT!\n"));

				//note: this must get called before the texture changes, 
				//because we use the light value of the texture to change
				//the static light in the segment
				subtract_light(seg-Segments,side);

				if (Newdemo_state == ND_STATE_RECORDING)
					newdemo_record_effect_blowup( seg-Segments, side, pnt);

				if (ec!=-1) {
					dest_size = Effects[ec].dest_size;
					vc = Effects[ec].dest_vclip;
				} else {
					dest_size = i2f(20);
					vc = 3;
				}

				object_create_explosion( seg-Segments, pnt, dest_size, vc );

				if (ec!=-1 && db!=-1 && !(Effects[ec].flags&EF_ONE_SHOT)) {

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
						seg->sides[side].tmap_num2 = bm_num | tmf;		//replace with destoyed

					}
					else {
						Assert(db!=0 && seg->sides[side].tmap_num2!=0);
						seg->sides[side].tmap_num2 = db | tmf;		//replace with destoyed
					}
				}
				else {
					seg->sides[side].tmap_num2 = TmapInfo[tm].destroyed | tmf;

					//assume this is a light, and play light sound
		  			digi_link_sound_to_pos( SOUND_LIGHT_BLOWNUP, seg-Segments, 0, pnt,  0, F1_0 );
				}


				return 1;		//blew up!
			}
		}
	}

	return 0;		//didn't blow up
}

//	Copied from laser.c!
#define	MIN_OMEGA_BLOBS		3				//	No matter how close the obstruction, at this many blobs created.
#define	MIN_OMEGA_DIST			(F1_0*3)		//	At least this distance between blobs, unless doing so would violate MIN_OMEGA_BLOBS
#define	DESIRED_OMEGA_DIST	(F1_0*5)		//	This is the desired distance between blobs.  For distances > MIN_OMEGA_BLOBS*DESIRED_OMEGA_DIST, but not very large, this will apply.
#define	MAX_OMEGA_BLOBS		16				//	No matter how far away the obstruction, this is the maximum number of blobs.
#define	MAX_OMEGA_DIST			(MAX_OMEGA_BLOBS * DESIRED_OMEGA_DIST)		//	Maximum extent of lightning blobs.

//	-------------------------------------------------
//	Return true if ok to do Omega damage.
int ok_to_do_omega_damage(object *weapon)
{
	int	parent_sig = weapon->ctype.laser_info.parent_signature;
	int	parent_num = weapon->ctype.laser_info.parent_num;

	if (!(Game_mode & GM_MULTI))
		return 1;

	if (Objects[parent_num].signature != parent_sig)
		mprintf((0, "Parent of omega blob not consistent with object information.\n"));
	else {
		fix	dist = vm_vec_dist_quick(&Objects[parent_num].pos, &weapon->pos);

		if (dist > MAX_OMEGA_DIST) {
			// -- mprintf((0, "Not doing damage in frame %i, too far away.\n", FrameCount));
			return 0;
		} else
			; // -- mprintf((0, "*** Doing damage in frame %i ***\n", FrameCount));
	}

	return 1;
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
	int	robot_escort;

	if (weapon->id == OMEGA_ID)
		if (!ok_to_do_omega_damage(weapon))
			return;

	//	If this is a guided missile and it strikes fairly directly, clear bounce flag.
	if (weapon->id == GUIDEDMISS_ID) {
		fix	dot;

		dot = vm_vec_dot(&weapon->orient.fvec, &Segments[hitseg].sides[hitwall].normals[0]);
		mprintf((0, "Guided missile dot = %7.3f\n", f2fl(dot)));
		if (dot < -F1_0/6) {
			mprintf((0, "Guided missile loses bounciness.\n"));
			weapon->mtype.phys_info.flags &= ~PF_BOUNCE;
		}
	}

	//if an energy weapon hits a forcefield, let it bounce
	if ((TmapInfo[seg->sides[hitwall].tmap_num].flags & TMI_FORCE_FIELD) &&
		 !(weapon->type == OBJ_WEAPON && Weapon_info[weapon->id].energy_usage==0)) {

		//make sound
		digi_link_sound_to_pos( SOUND_FORCEFIELD_BOUNCE_WEAPON, hitseg, 0, hitpt, 0, f1_0 );
#ifdef NETWORK
		if (Game_mode & GM_MULTI)
			multi_send_play_sound(SOUND_FORCEFIELD_BOUNCE_WEAPON, f1_0);
#endif

		return;	//bail here. physics code will bounce this object
	}

	#ifndef NDEBUG
	if (keyd_pressed[KEY_LAPOSTRO])
		if (weapon->ctype.laser_info.parent_num == Players[Player_num].objnum) {
			//	MK: Real pain when you need to know a seg:side and you've got quad lasers.
			mprintf((0, "Your laser hit at segment = %i, side = %i\n", hitseg, hitwall));
			HUD_init_message("Hit at segment = %i, side = %i", hitseg, hitwall);
			if (weapon->id < 4)
				subtract_light(hitseg, hitwall);
			else if (weapon->id == FLARE_ID)
				add_light(hitseg, hitwall);
		}

		//@@#ifdef EDITOR
		//@@Cursegp = &Segments[hitseg];
		//@@Curside = hitwall;
		//@@#endif
	#endif

	if ((weapon->mtype.phys_info.velocity.x == 0) && (weapon->mtype.phys_info.velocity.y == 0) && (weapon->mtype.phys_info.velocity.z == 0)) {
		Int3();	//	Contact Matt: This is impossible.  A weapon with 0 velocity hit a wall, which doesn't move.
		return;
	}

	blew_up = check_effect_blowup(seg,hitwall,hitpt, weapon, 0);

	//if ((seg->sides[hitwall].tmap_num2==0) && (TmapInfo[seg->sides[hitwall].tmap_num].flags & TMI_VOLATILE)) {

	if ((weapon->ctype.laser_info.parent_type == OBJ_ROBOT) && (Robot_info[Objects[weapon->ctype.laser_info.parent_num].id].companion==1)) {
		robot_escort = 1;

		if (Game_mode & GM_MULTI)
		 {
			 Int3();  // Get Jason!
		    return;
	    }	


		playernum = Player_num;		//if single player, he's the player's buddy
	}
	else {
		robot_escort = 0;

		if (Objects[weapon->ctype.laser_info.parent_num].type == OBJ_PLAYER)
			playernum = Objects[weapon->ctype.laser_info.parent_num].id;
		else
			playernum = -1;		//not a player (thus a robot)
	}

	if (blew_up) {		//could be a wall switch
		//for wall triggers, always say that the player shot it out.  This is
		//because robots can shoot out wall triggers, and so the trigger better
		//take effect  
		//	NO -- Changed by MK, 10/18/95.  We don't want robots blowing puzzles.  Only player or buddy can open!
		check_trigger(seg,hitwall,weapon->ctype.laser_info.parent_num,1);
	}

	if (weapon->id == EARTHSHAKER_ID)
		smega_rock_stuff();

	wall_type = wall_hit_process( seg, hitwall, weapon->shields, playernum, weapon );

	// Wall is volatile if either tmap 1 or 2 is volatile
	if ((TmapInfo[seg->sides[hitwall].tmap_num].flags & TMI_VOLATILE) || (seg->sides[hitwall].tmap_num2 && (TmapInfo[seg->sides[hitwall].tmap_num2&0x3fff].flags & TMI_VOLATILE))) {
		weapon_info *wi = &Weapon_info[weapon->id];
		int vclip;

		//we've hit a volatile wall

		digi_link_sound_to_pos( SOUND_VOLATILE_WALL_HIT,hitseg, 0, hitpt, 0, F1_0 );

		//for most weapons, use volatile wall hit.  For mega, use its special vclip
		vclip = (weapon->id == MEGA_ID)?Weapon_info[weapon->id].robot_hit_vclip:VCLIP_VOLATILE_WALL_HIT;

		//	New by MK: If powerful badass, explode as badass, not due to lava, fixes megas being wimpy in lava.
		if (wi->damage_radius >= VOLATILE_WALL_DAMAGE_RADIUS/2) {
			// -- mprintf((0, "Big weapon doing badass in lava instead.\n"));
			explode_badass_weapon(weapon,hitpt);
		} else {
			object_create_badass_explosion( weapon, hitseg, hitpt, 
				wi->impact_size + VOLATILE_WALL_IMPACT_SIZE,
				vclip,
				wi->strength[Difficulty_level]/4+VOLATILE_WALL_EXPL_STRENGTH,	//	diminished by mk on 12/08/94, i was doing 70 damage hitting lava on lvl 1.
				wi->damage_radius+VOLATILE_WALL_DAMAGE_RADIUS,
				wi->strength[Difficulty_level]/2+VOLATILE_WALL_DAMAGE_FORCE,
				weapon->ctype.laser_info.parent_num );
		}

		weapon->flags |= OF_SHOULD_BE_DEAD;		//make flares die in lava

	}
	else if ((TmapInfo[seg->sides[hitwall].tmap_num].flags & TMI_WATER) || (seg->sides[hitwall].tmap_num2 && (TmapInfo[seg->sides[hitwall].tmap_num2&0x3fff].flags & TMI_WATER))) {
		weapon_info *wi = &Weapon_info[weapon->id];

		//we've hit water

		//	MK: 09/13/95: Badass in water is 1/2 normal intensity.
		if ( Weapon_info[weapon->id].matter ) {

			digi_link_sound_to_pos( SOUND_MISSILE_HIT_WATER,hitseg, 0, hitpt, 0, F1_0 );

			if ( Weapon_info[weapon->id].damage_radius ) {

				digi_link_sound_to_object(SOUND_BADASS_EXPLOSION, weapon-Objects, 0, F1_0);

				//	MK: 09/13/95: Badass in water is 1/2 normal intensity.
				object_create_badass_explosion( weapon, hitseg, hitpt, 
					wi->impact_size/2,
					wi->robot_hit_vclip, 
					wi->strength[Difficulty_level]/4,
					wi->damage_radius,
					wi->strength[Difficulty_level]/2,
					weapon->ctype.laser_info.parent_num );
			}
			else
				object_create_explosion( weapon->segnum, &weapon->pos, Weapon_info[weapon->id].impact_size, Weapon_info[weapon->id].wall_hit_vclip );

		} else {
			digi_link_sound_to_pos( SOUND_LASER_HIT_WATER,hitseg, 0, hitpt, 0, F1_0 );
			object_create_explosion( weapon->segnum, &weapon->pos, Weapon_info[weapon->id].impact_size, VCLIP_WATER_HIT );
		}

		weapon->flags |= OF_SHOULD_BE_DEAD;		//make flares die in water

	}
	else {

		if (weapon->mtype.phys_info.flags & PF_BOUNCE) {

			//do special bound sound & effect

		}
		else {

			//if it's not the player's weapon, or it is the player's and there
			//is no wall, and no blowing up monitor, then play sound
			if ((weapon->ctype.laser_info.parent_type != OBJ_PLAYER) ||	((seg->sides[hitwall].wall_num == -1 || wall_type==WHP_NOT_SPECIAL) && !blew_up))
				if ((Weapon_info[weapon->id].wall_hit_sound > -1 ) && (!(weapon->flags & OF_SILENT)))
				digi_link_sound_to_pos( Weapon_info[weapon->id].wall_hit_sound,weapon->segnum, 0, &weapon->pos, 0, F1_0 );
		
			if ( Weapon_info[weapon->id].wall_hit_vclip > -1 )	{
				if ( Weapon_info[weapon->id].damage_radius )
					explode_badass_weapon(weapon,hitpt);
				else
					object_create_explosion( weapon->segnum, &weapon->pos, Weapon_info[weapon->id].impact_size, Weapon_info[weapon->id].wall_hit_vclip );
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

			if (((weapon->id != FLARE_ID) || (weapon->ctype.laser_info.parent_type != OBJ_PLAYER)) && !(weapon->mtype.phys_info.flags & PF_BOUNCE))
				weapon->flags |= OF_SHOULD_BE_DEAD;

			//don't let flares stick in force fields
			if ((weapon->id == FLARE_ID) && (TmapInfo[seg->sides[hitwall].tmap_num].flags & TMI_FORCE_FIELD))
				weapon->flags |= OF_SHOULD_BE_DEAD;

			if (!(weapon->flags & OF_SILENT)) {
				switch (wall_type) {

					case WHP_NOT_SPECIAL:
						//should be handled above
						//digi_link_sound_to_pos( Weapon_info[weapon->id].wall_hit_sound, weapon->segnum, 0, &weapon->pos, 0, F1_0 );
						break;

					case WHP_NO_KEY:
						//play special hit door sound (if/when we get it)
						digi_link_sound_to_pos( SOUND_WEAPON_HIT_DOOR, weapon->segnum, 0, &weapon->pos, 0, F1_0 );
#ifdef NETWORK
			         if (Game_mode & GM_MULTI)
							multi_send_play_sound( SOUND_WEAPON_HIT_DOOR, F1_0 );
#endif

						break;

					case WHP_BLASTABLE:
						//play special blastable wall sound (if/when we get it)
						if ((Weapon_info[weapon->id].wall_hit_sound > -1 ) && (!(weapon->flags & OF_SILENT)))
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
		if (!(weapon->mtype.phys_info.flags & PF_BOUNCE))
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

fix Last_thief_hit_time;

void collide_robot_and_player( object * robot, object * playerobj, vms_vector *collision_point )
{ 
	int	steal_attempt = 0;
	int	collision_seg;

	if (robot->flags&OF_EXPLODING)
		return;

	collision_seg = find_point_seg(collision_point, playerobj->segnum);
	if (collision_seg != -1)
		object_create_explosion( collision_seg, collision_point, Weapon_info[0].impact_size, Weapon_info[0].wall_hit_vclip );

	if (playerobj->id == Player_num) {
		if (Robot_info[robot->id].companion)	//	Player and companion don't collide.
			return;
		if (Robot_info[robot->id].kamikaze) {
			apply_damage_to_robot(robot, robot->shields+1, playerobj-Objects);
			if (playerobj == ConsoleObject)
				add_points_to_score(Robot_info[robot->id].score_value);
		}

		if (Robot_info[robot->id].thief) {
			if (Ai_local_info[robot-Objects].mode == AIM_THIEF_ATTACK) {
				Last_thief_hit_time = GameTime;
				attempt_to_steal_item(robot, playerobj->id);
				steal_attempt = 1;
			} else if (GameTime - Last_thief_hit_time < F1_0*2)
				return;		//	ZOUNDS!  BRILLIANT!  Thief not collide with player if not stealing!
								// NO!  VERY DUMB!  makes thief look very stupid if player hits him while cloaked! -AP
			else
				Last_thief_hit_time = GameTime;
		}

		create_awareness_event(playerobj, PA_PLAYER_COLLISION);			// object robot can attract attention to player
		do_ai_robot_hit_attack(robot, playerobj, collision_point);
		do_ai_robot_hit(robot, PA_WEAPON_ROBOT_COLLISION);
	} 
#ifdef NETWORK
	else
		multi_robot_request_change(robot, playerobj->id);
#endif

	// added this if to remove the bump sound if it's the thief.
	// A "steal" sound was added and it was getting obscured by the bump. -AP 10/3/95
	//	Changed by MK to make this sound unless the robot stole.
	if ((!steal_attempt) && !Robot_info[robot->id].energy_drain)
		digi_link_sound_to_pos( SOUND_ROBOT_HIT_PLAYER, playerobj->segnum, 0, collision_point, 0, F1_0 );

	bump_two_objects(robot, playerobj, 1);
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
		mprintf((0, "Damage to control center by object of type %i prevented by MK!\n", whotype));
		return;
	}

	#ifdef NETWORK
	if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP) && (Players[Player_num].time_level < Netgame.control_invul_time))
	{
		if (Objects[who].id == Player_num) {
			int secs = f2i(Netgame.control_invul_time-Players[Player_num].time_level) % 60;
			int mins = f2i(Netgame.control_invul_time-Players[Player_num].time_level) / 60;
			HUD_init_message("%s %d:%02d.", TXT_CNTRLCEN_INVUL, mins, secs);
		}
		return;
	}
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

void collide_player_and_controlcen( object * controlcen, object * playerobj, vms_vector *collision_point )
{ 
	if (playerobj->id == Player_num) {
		Control_center_been_hit = 1;
		ai_do_cloak_stuff();				//	In case player cloaked, make control center know where he is.
	}

	digi_link_sound_to_pos( SOUND_ROBOT_HIT_PLAYER, playerobj->segnum, 0, collision_point, 0, F1_0 );
	bump_two_objects(controlcen, playerobj, 1);

	return; 
}

void collide_player_and_marker( object * marker, object * playerobj, vms_vector *collision_point )
{ 
   mprintf ((0,"Collided with marker %d!\n",marker->id));

	if (playerobj->id==Player_num) {
		int drawn;

		if (Game_mode & GM_MULTI)
		{
			drawn = HUD_init_message ("MARKER %s: %s",Players[marker->id/2].callsign,MarkerMessage[marker->id]);
		}
		else
		{
			if (MarkerMessage[marker->id][0])
				drawn = HUD_init_message("MARKER %d: %s", marker->id+1,MarkerMessage[marker->id]);
			else
				drawn = HUD_init_message("MARKER %d", marker->id+1);
	   }

		if (drawn)
			digi_play_sample( SOUND_MARKER_HIT, F1_0 );

		detect_escort_goal_accomplished(marker-Objects);
   }
}

//	If a persistent weapon and other object is not a weapon, weaken it, else kill it.
//	If both objects are weapons, weaken the weapon.
void maybe_kill_weapon(object *weapon, object *other_obj)
{
	if ((weapon->id == PROXIMITY_ID) || (weapon->id == SUPERPROX_ID) || (weapon->id == PMINE_ID)) {
		weapon->flags |= OF_SHOULD_BE_DEAD;
		return;
	}

	//	Changed, 10/12/95, MK: Make weapon-weapon collisions always kill both weapons if not persistent.
	//	Reason: Otherwise you can't use proxbombs to detonate incoming homing missiles (or mega missiles).
	if (weapon->mtype.phys_info.flags & PF_PERSISTENT) {
		//	Weapons do a lot of damage to weapons, other objects do much less.
		if (!(weapon->mtype.phys_info.flags & PF_PERSISTENT)) {
			if (other_obj->type == OBJ_WEAPON)
				weapon->shields -= other_obj->shields/2;
			else
				weapon->shields -= other_obj->shields/4;

			if (weapon->shields <= 0) {
				weapon->shields = 0;
				weapon->flags |= OF_SHOULD_BE_DEAD;	// weapon->lifeleft = 1;
			}
		}
	} else
		weapon->flags |= OF_SHOULD_BE_DEAD;	// weapon->lifeleft = 1;

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

void collide_weapon_and_controlcen( object * weapon, object *controlcen, vms_vector *collision_point  )
{

	if (weapon->id == OMEGA_ID)
		if (!ok_to_do_omega_damage(weapon))
			return;

	if (weapon->ctype.laser_info.parent_type == OBJ_PLAYER) {
		fix	damage = weapon->shields;

		if (Objects[weapon->ctype.laser_info.parent_num].id == Player_num)
			Control_center_been_hit = 1;

		if ( Weapon_info[weapon->id].damage_radius )
			explode_badass_weapon(weapon,collision_point);
		else
			object_create_explosion( controlcen->segnum, collision_point, controlcen->size*3/20, VCLIP_SMALL_EXPLOSION );

		digi_link_sound_to_pos( SOUND_CONTROL_CENTER_HIT, controlcen->segnum, 0, collision_point, 0, F1_0 );

		damage = fixmul(damage, weapon->ctype.laser_info.multiplier);

		apply_damage_to_controlcen(controlcen, damage, weapon->ctype.laser_info.parent_num);

		maybe_kill_weapon(weapon,controlcen);
	} else {	//	If robot weapon hits control center, blow it up, make it go away, but do no damage to control center.
		object_create_explosion( controlcen->segnum, collision_point, controlcen->size*3/20, VCLIP_SMALL_EXPLOSION );
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

extern object *explode_badass_object(object *objp, fix damage, fix distance, fix force);

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

	gr_palette_fade_out( gr_palette, 256, 0 );
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
		Players[Player_num].invulnerable_time = GameTime;
		Players[Player_num].flags |= PLAYER_FLAGS_INVULNERABLE;
	}
	if (!(Game_mode & GM_MULTI))
		buddy_message("Nice job, %s!", Players[Player_num].callsign);

	Final_boss_is_dead = 1;
}

extern int Buddy_dude_cheat;
extern int multi_all_players_alive();
void multi_send_finish_game ();

//	------------------------------------------------------------------------------------------------------
//	Return 1 if robot died, else return 0
int apply_damage_to_robot(object *robot, fix damage, int killer_objnum)
{
#ifdef NETWORK
   char isthief;
	char i,temp_stolen[MAX_STOLEN_ITEMS];	
#endif
	
	if ( robot->flags&OF_EXPLODING) return 0;

	if (robot->shields < 0 ) return 0;	//robot already dead...

	if (Robot_info[robot->id].boss_flag)
		Boss_hit_time = GameTime;

	//	Buddy invulnerable on level 24 so he can give you his important messages.  Bah.
	//	Also invulnerable if his cheat for firing weapons is in effect.
	if (Robot_info[robot->id].companion) {
//		if ((Current_mission_num == 0 && Current_level_num == Last_level) || Buddy_dude_cheat)
#ifdef NETWORK
		if ((Current_mission_num == 0 && Current_level_num == Last_level) )
			return 0;
#endif
	}

//	if (robot->control_type == CT_REMOTE)
//		return 0; // Can't damange a robot controlled by another player

// -- MK, 10/21/95, unused! --	if (Robot_info[robot->id].boss_flag)
//		Boss_been_hit = 1;

	robot->shields -= damage;

	//	Do unspeakable hacks to make sure player doesn't die after killing boss.  Or before, sort of.
	if (Robot_info[robot->id].boss_flag)
#ifdef NETWORK
		if ((Current_mission_num == 0) && Current_level_num == Last_level)
#endif
			if (robot->shields < 0)
			 {
#ifdef NETWORK
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
#endif
				  {	// NOTE LINK TO ABOVE!!!
					if ((Players[Player_num].shields < 0) || Player_is_dead)
						robot->shields = 1;		//	Sorry, we can't allow you to kill the final boss after you've died.  Rough luck.
					else
						do_final_boss_hacks();
				  }
			  }

	if (robot->shields < 0) {
#ifdef NETWORK
		if (Game_mode & GM_MULTI) {
		 if (Robot_info[robot->id].thief)	
			isthief=1;
		 else
			isthief=0;

		 if (isthief)
			for (i=0;i<MAX_STOLEN_ITEMS;i++)
			 temp_stolen[(int)i]=Stolen_items[(int)i];
		  		
			if (multi_explode_robot_sub(robot-Objects, killer_objnum,Robot_info[robot->id].thief))
			{
			 if (isthief)	
   			for (i=0;i<MAX_STOLEN_ITEMS;i++)
				  Stolen_items[(int)i]=temp_stolen[(int)i];
					
				multi_send_robot_explode(robot-Objects, killer_objnum,Robot_info[robot->id].thief);

	     	   if (isthief)	
   				for (i=0;i<MAX_STOLEN_ITEMS;i++)
					  Stolen_items[(int)i]=255;

				return 1;
			}
			else
				return 0;
		}
#endif

		Players[Player_num].num_kills_level++;
		Players[Player_num].num_kills_total++;

		if (Robot_info[robot->id].boss_flag) {
			start_boss_death_sequence(robot);	//do_controlcen_destroyed_stuff(NULL);
		} else if (Robot_info[robot->id].death_roll) {
			start_robot_death_sequence(robot);	//do_controlcen_destroyed_stuff(NULL);
		} else {
			if (robot->id == SPECIAL_REACTOR_ROBOT)
				special_reactor_stuff();
			//if (Robot_info[robot->id].smart_blobs)
			//	create_smart_children(robot, Robot_info[robot->id].smart_blobs);
			//if (Robot_info[robot->id].badass)
			//	explode_badass_object(robot, F1_0*Robot_info[robot->id].badass, F1_0*40, F1_0*150);
			if (Robot_info[robot->id].kamikaze)
				explode_object(robot,1);		//	Kamikaze, explode right away, IN YOUR FACE!
			else
				explode_object(robot,STANDARD_EXPL_DELAY);
		}
		return 1;
	} else
		return 0;
}

extern int boss_spew_robot(object *objp, vms_vector *pos);

//--ubyte	Boss_teleports[NUM_D2_BOSSES] = 				{1,1,1,1,1,1};		// Set byte if this boss can teleport
//--ubyte	Boss_cloaks[NUM_D2_BOSSES] = 					{1,1,1,1,1,1};		// Set byte if this boss can cloak
//--ubyte	Boss_spews_bots_energy[NUM_D2_BOSSES] = 	{1,1,0,0,1,1};		//	Set byte if boss spews bots when hit by energy weapon.
//--ubyte	Boss_spews_bots_matter[NUM_D2_BOSSES] = 	{0,0,1,0,1,1};		//	Set byte if boss spews bots when hit by matter weapon.
//--ubyte	Boss_invulnerable_energy[NUM_D2_BOSSES] = {0,0,1,1,0,0};		//	Set byte if boss is invulnerable to energy weapons.
//--ubyte	Boss_invulnerable_matter[NUM_D2_BOSSES] = {0,0,0,1,0,0};		//	Set byte if boss is invulnerable to matter weapons.
//--ubyte	Boss_invulnerable_spot[NUM_D2_BOSSES] = 	{0,0,0,0,1,1};		//	Set byte if boss is invulnerable in all but a certain spot.  (Dot product fvec|vec_to_collision < BOSS_INVULNERABLE_DOT)

//#define	BOSS_INVULNERABLE_DOT	0		//	If a boss is invulnerable over most of his body, fvec(dot)vec_to_collision must be less than this for damage to occur.
int	Boss_invulnerable_dot = 0;

int	Buddy_gave_hint_count = 5;
fix	Last_time_buddy_gave_hint = 0;

//	------------------------------------------------------------------------------------------------------
//	Return true if damage done to boss, else return false.
int do_boss_weapon_collision(object *robot, object *weapon, vms_vector *collision_point)
{
	int	d2_boss_index;
	int	damage_flag;

	damage_flag = 1;

	d2_boss_index = Robot_info[robot->id].boss_flag - BOSS_D2;

	Assert((d2_boss_index >= 0) && (d2_boss_index < NUM_D2_BOSSES));

	//	See if should spew a bot.
	if (weapon->ctype.laser_info.parent_type == OBJ_PLAYER)
		if ((Weapon_info[weapon->id].matter && Boss_spews_bots_matter[d2_boss_index]) || (!Weapon_info[weapon->id].matter && Boss_spews_bots_energy[d2_boss_index])) {
			if (Boss_spew_more[d2_boss_index])
				if (d_rand() > 16384) {
					if (boss_spew_robot(robot, collision_point) != -1)
						Last_gate_time = GameTime - Gate_interval - 1;	//	Force allowing spew of another bot.
				}
			boss_spew_robot(robot, collision_point);
		}

	if (Boss_invulnerable_spot[d2_boss_index]) {
		fix			dot;
		vms_vector	tvec1;

		//	Boss only vulnerable in back.  See if hit there.
		vm_vec_sub(&tvec1, collision_point, &robot->pos);
		vm_vec_normalize_quick(&tvec1);	//	Note, if BOSS_INVULNERABLE_DOT is close to F1_0 (in magnitude), then should probably use non-quick version.
		dot = vm_vec_dot(&tvec1, &robot->orient.fvec);
		mprintf((0, "Boss hit vec dot = %7.3f\n", f2fl(dot)));

		if (dot > Boss_invulnerable_dot) {
			int	new_obj;
			int	segnum;

			segnum = find_point_seg(collision_point, robot->segnum);
			digi_link_sound_to_pos( SOUND_WEAPON_HIT_DOOR, segnum, 0, collision_point, 0, F1_0);
			damage_flag = 0;

			if (Last_time_buddy_gave_hint == 0)
				Last_time_buddy_gave_hint = d_rand()*32 + F1_0*16;

			if (Buddy_gave_hint_count) {
				if (Last_time_buddy_gave_hint + F1_0*20 < GameTime) {
					int	sval;

					Buddy_gave_hint_count--;
					Last_time_buddy_gave_hint = GameTime;
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

			//	Cause weapon to bounce.
			//	Make a copy of this weapon, because the physics wants to destroy it.
			if (!Weapon_info[weapon->id].matter) {
				new_obj = obj_create(weapon->type, weapon->id, weapon->segnum, &weapon->pos,
					&weapon->orient, weapon->size, weapon->control_type, weapon->movement_type, weapon->render_type);

				if (new_obj != -1) {
					vms_vector	vec_to_point;
					vms_vector	weap_vec;
					fix			speed;

					if (weapon->render_type == RT_POLYOBJ) {
						Objects[new_obj].rtype.pobj_info.model_num = Weapon_info[Objects[new_obj].id].model_num;
						Objects[new_obj].size = fixdiv(Polygon_models[Objects[new_obj].rtype.pobj_info.model_num].rad,Weapon_info[Objects[new_obj].id].po_len_to_width_ratio);
					}

					Objects[new_obj].mtype.phys_info.mass = Weapon_info[weapon->type].mass;
					Objects[new_obj].mtype.phys_info.drag = Weapon_info[weapon->type].drag;
					vm_vec_zero(&Objects[new_obj].mtype.phys_info.thrust);

					vm_vec_sub(&vec_to_point, collision_point, &robot->pos);
					vm_vec_normalize_quick(&vec_to_point);
					weap_vec = weapon->mtype.phys_info.velocity;
					speed = vm_vec_normalize_quick(&weap_vec);
					vm_vec_scale_add2(&vec_to_point, &weap_vec, -F1_0*2);
					vm_vec_scale(&vec_to_point, speed/4);
					Objects[new_obj].mtype.phys_info.velocity = vec_to_point;
				}
			}
		}
	} else if ((Weapon_info[weapon->id].matter && Boss_invulnerable_matter[d2_boss_index]) || (!Weapon_info[weapon->id].matter && Boss_invulnerable_energy[d2_boss_index])) {
		int	segnum;

		segnum = find_point_seg(collision_point, robot->segnum);
		digi_link_sound_to_pos( SOUND_WEAPON_HIT_DOOR, segnum, 0, collision_point, 0, F1_0);
		damage_flag = 0;
	}

	return damage_flag;
}

extern int Robots_kill_robots_cheat;

//	------------------------------------------------------------------------------------------------------
void collide_robot_and_weapon( object * robot, object * weapon, vms_vector *collision_point )
{ 
	int	damage_flag=1;
	int	boss_invul_flag=0;

	if (weapon->id == OMEGA_ID)
		if (!ok_to_do_omega_damage(weapon))
			return;

	if (Robot_info[robot->id].boss_flag) {
		Boss_hit_time = GameTime;
		if (Robot_info[robot->id].boss_flag >= BOSS_D2) {
			damage_flag = do_boss_weapon_collision(robot, weapon, collision_point);
			boss_invul_flag = !damage_flag;
		}
	}

	//	Put in at request of Jasen (and Adam) because the Buddy-Bot gets in their way.
	//	MK has so much fun whacking his butt around the mine he never cared...
	if ((Robot_info[robot->id].companion) && ((weapon->ctype.laser_info.parent_type != OBJ_ROBOT) && !Robots_kill_robots_cheat))
		return;

	if (weapon->id == EARTHSHAKER_ID)
		smega_rock_stuff();

	//	If a persistent weapon hit robot most recently, quick abort, else we cream the same robot many times,
	//	depending on frame rate.
	if (weapon->mtype.phys_info.flags & PF_PERSISTENT) {
		if (weapon->ctype.laser_info.last_hitobj == robot-Objects)
			return;
		else
			weapon->ctype.laser_info.last_hitobj = robot-Objects;

		// mprintf((0, "weapon #%i with power %i hits robot #%i.\n", weapon - Objects, f2i(weapon->shields), robot - Objects));
	}

	if (weapon->ctype.laser_info.parent_signature == robot->signature)
		return;

	//	Changed, 10/04/95, put out blobs based on skill level and power of weapon doing damage.
	//	Also, only a weapon hit from a player weapon causes smart blobs.
	if ((weapon->ctype.laser_info.parent_type == OBJ_PLAYER) && (Robot_info[robot->id].energy_blobs))
		if ((robot->shields > 0) && Weapon_is_energy[weapon->id]) {
			fix	probval;
			int	num_blobs;

			probval = (Difficulty_level+2) * min(weapon->shields, robot->shields);
			probval = Robot_info[robot->id].energy_blobs * probval/(NDL*32);

			num_blobs = probval >> 16;
			if (2*d_rand() < (probval & 0xffff))
				num_blobs++;

			if (num_blobs)
				create_smart_children(robot, num_blobs);
		}

	//	Note: If weapon hits an invulnerable boss, it will still do badass damage, including to the boss,
	//	unless this is trapped elsewhere.
	if ( Weapon_info[weapon->id].damage_radius )
	{
		if (boss_invul_flag) {			//don't make badass sound
			weapon_info *wi = &Weapon_info[weapon->id];

			//this code copied from explode_badass_weapon()
		
			object_create_badass_explosion( weapon, weapon->segnum, collision_point, 
							wi->impact_size, 
							wi->robot_hit_vclip, 
							wi->strength[Difficulty_level], 
							wi->damage_radius,wi->strength[Difficulty_level],
							weapon->ctype.laser_info.parent_num );
		
		}
		else		//normal badass explosion
			explode_badass_weapon(weapon,collision_point);
	}

	if ( ((weapon->ctype.laser_info.parent_type==OBJ_PLAYER) || Robots_kill_robots_cheat) && !(robot->flags & OF_EXPLODING) )	{	
		object *expl_obj=NULL;

		if (weapon->ctype.laser_info.parent_num == Players[Player_num].objnum) {
			create_awareness_event(weapon, PA_WEAPON_ROBOT_COLLISION);			// object "weapon" can attract attention to player
			do_ai_robot_hit(robot, PA_WEAPON_ROBOT_COLLISION);
		}
#ifdef NETWORK
	       	else
			multi_robot_request_change(robot, Objects[weapon->ctype.laser_info.parent_num].id);
#endif

		if ( Robot_info[robot->id].exp1_vclip_num > -1 )
			expl_obj = object_create_explosion( weapon->segnum, collision_point, (robot->size/2*3)/4, Robot_info[robot->id].exp1_vclip_num );
		else if ( Weapon_info[weapon->id].robot_hit_vclip > -1 )
			expl_obj = object_create_explosion( weapon->segnum, collision_point, Weapon_info[weapon->id].impact_size, Weapon_info[weapon->id].robot_hit_vclip );

		if (expl_obj)
			obj_attach(robot,expl_obj);

		if ( damage_flag && (Robot_info[robot->id].exp1_sound_num > -1 ))
			digi_link_sound_to_pos( Robot_info[robot->id].exp1_sound_num, robot->segnum, 0, collision_point, 0, F1_0 );

		if (!(weapon->flags & OF_HARMLESS)) {
			fix	damage = weapon->shields;

			if (damage_flag)
				damage = fixmul(damage, weapon->ctype.laser_info.multiplier);
			else
				damage = 0;

			//	Cut Gauss damage on bosses because it just breaks the game.  Bosses are so easy to
			//	hit, and missing a robot is what prevents the Gauss from being game-breaking.
			if (weapon->id == GAUSS_ID)
				if (Robot_info[robot->id].boss_flag)
					damage = damage * (2*NDL-Difficulty_level)/(2*NDL);

			if (! apply_damage_to_robot(robot, damage, weapon->ctype.laser_info.parent_num))
				bump_two_objects(robot, weapon, 0);		//only bump if not dead. no damage from bump
			else if (weapon->ctype.laser_info.parent_signature == ConsoleObject->signature) {
				add_points_to_score(Robot_info[robot->id].score_value);
				detect_escort_goal_accomplished(robot-Objects);
			}
		}


		//	If Gauss Cannon, spin robot.
		if ((robot != NULL) && (!Robot_info[robot->id].companion) && (!Robot_info[robot->id].boss_flag) && (weapon->id == GAUSS_ID)) {
			ai_static	*aip = &robot->ctype.ai_info;

			if (aip->SKIP_AI_COUNT * FrameTime < F1_0) {
				aip->SKIP_AI_COUNT++;
				robot->mtype.phys_info.rotthrust.x = fixmul((d_rand() - 16384), FrameTime * aip->SKIP_AI_COUNT);
				robot->mtype.phys_info.rotthrust.y = fixmul((d_rand() - 16384), FrameTime * aip->SKIP_AI_COUNT);
				robot->mtype.phys_info.rotthrust.z = fixmul((d_rand() - 16384), FrameTime * aip->SKIP_AI_COUNT);
				robot->mtype.phys_info.flags |= PF_USES_THRUST;

			}
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
		detect_escort_goal_accomplished(hostage-Objects);
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

// -- int	Items_destroyed = 0;

void drop_player_eggs(object *playerobj)
{
//	mprintf((0, "In drop_player_eggs...\n"));

	if ((playerobj->type == OBJ_PLAYER) || (playerobj->type == OBJ_GHOST)) {
		int	rthresh;
		int	pnum = playerobj->id;
		int	objnum;
		int	vulcan_ammo=0;
		vms_vector	randvec;

		// -- Items_destroyed = 0;

		// Seed the random number generator so in net play the eggs will always
		// drop the same way
		#ifdef NETWORK
		if (Game_mode & GM_MULTI) 
		{
			Net_create_loc = 0;
			d_srand(5483L);
		}
		#endif

		//	If the player had smart mines, maybe arm one of them.
		rthresh = 30000;
		while ((Players[playerobj->id].secondary_ammo[SMART_MINE_INDEX]%4==1) && (d_rand() < rthresh)) {
			int			newseg;
			vms_vector	tvec;

			make_random_vector(&randvec);
			rthresh /= 2;
			vm_vec_add(&tvec, &playerobj->pos, &randvec);
			newseg = find_point_seg(&tvec, playerobj->segnum);
			if (newseg != -1)
				Laser_create_new(&randvec, &tvec, newseg, playerobj-Objects, SUPERPROX_ID, 0);
	  	}

		//	If the player had proximity bombs, maybe arm one of them.

		if ((Game_mode & GM_MULTI) && !(Game_mode & GM_HOARD))
		{
			rthresh = 30000;
			while ((Players[playerobj->id].secondary_ammo[PROXIMITY_INDEX]%4==1) && (d_rand() < rthresh)) {
				int			newseg;
				vms_vector	tvec;
	
				make_random_vector(&randvec);
				rthresh /= 2;
				vm_vec_add(&tvec, &playerobj->pos, &randvec);
				newseg = find_point_seg(&tvec, playerobj->segnum);
				if (newseg != -1)
					Laser_create_new(&randvec, &tvec, newseg, playerobj-Objects, PROXIMITY_ID, 0);
	
			}
		}

		//	If the player dies and he has powerful lasers, create the powerups here.

		if (Players[pnum].laser_level > MAX_LASER_LEVEL)
			call_object_create_egg(playerobj, Players[pnum].laser_level-MAX_LASER_LEVEL, OBJ_POWERUP, POW_SUPER_LASER);
		else if (Players[pnum].laser_level >= 1)
			call_object_create_egg(playerobj, Players[pnum].laser_level, OBJ_POWERUP, POW_LASER);	// Note: laser_level = 0 for laser level 1.

		//	Drop quad laser if appropos
		if (Players[pnum].flags & PLAYER_FLAGS_QUAD_LASERS)
			call_object_create_egg(playerobj, 1, OBJ_POWERUP, POW_QUAD_FIRE);

		if (Players[pnum].flags & PLAYER_FLAGS_CLOAKED)
			call_object_create_egg(playerobj, 1, OBJ_POWERUP, POW_CLOAK);

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

#ifdef NETWORK
		if ((Game_mode & GM_CAPTURE) && (Players[pnum].flags & PLAYER_FLAGS_FLAG))
		{
		 if ((get_team (pnum)==TEAM_RED))
			call_object_create_egg(playerobj, 1, OBJ_POWERUP, POW_FLAG_BLUE);
		 else
			call_object_create_egg(playerobj, 1, OBJ_POWERUP, POW_FLAG_RED);
		}

	
		if (Game_mode & GM_HOARD)
		{
			// Drop hoard orbs
			
			int max_count,i;

			mprintf ((0,"HOARD MODE: Dropping %d orbs\n",Players[pnum].secondary_ammo[PROXIMITY_INDEX]));
	
			max_count = min(Players[pnum].secondary_ammo[PROXIMITY_INDEX], 12);
			for (i=0; i<max_count; i++)
				call_object_create_egg(playerobj, 1, OBJ_POWERUP, POW_HOARD_ORB);
		}
#endif

		//Drop the vulcan, gauss, and ammo
		vulcan_ammo = Players[pnum].primary_ammo[VULCAN_INDEX];
		if ((Players[pnum].primary_weapon_flags & HAS_FLAG(VULCAN_INDEX)) && (Players[pnum].primary_weapon_flags & HAS_FLAG(GAUSS_INDEX)))
			vulcan_ammo /= 2;		//if both vulcan & gauss, each gets half
		if (vulcan_ammo < VULCAN_AMMO_AMOUNT)
			vulcan_ammo = VULCAN_AMMO_AMOUNT;	//make sure gun has at least as much as a powerup
		objnum = maybe_drop_primary_weapon_egg(playerobj, VULCAN_INDEX);
		if (objnum!=-1)
			Objects[objnum].ctype.powerup_info.count = vulcan_ammo;
		objnum = maybe_drop_primary_weapon_egg(playerobj, GAUSS_INDEX);
		if (objnum!=-1)
			Objects[objnum].ctype.powerup_info.count = vulcan_ammo;

		//	Drop the rest of the primary weapons
		maybe_drop_primary_weapon_egg(playerobj, SPREADFIRE_INDEX);
		maybe_drop_primary_weapon_egg(playerobj, PLASMA_INDEX);
		maybe_drop_primary_weapon_egg(playerobj, FUSION_INDEX);

		maybe_drop_primary_weapon_egg(playerobj, HELIX_INDEX);
		maybe_drop_primary_weapon_egg(playerobj, PHOENIX_INDEX);

		objnum = maybe_drop_primary_weapon_egg(playerobj, OMEGA_INDEX);
		if (objnum!=-1)
			Objects[objnum].ctype.powerup_info.count = (playerobj->id==Player_num)?Omega_charge:MAX_OMEGA_CHARGE;

		//	Drop the secondary weapons
		//	Note, proximity weapon only comes in packets of 4.  So drop n/2, but a max of 3 (handled inside maybe_drop..)  Make sense?
		
		if (!(Game_mode & GM_HOARD))
			maybe_drop_secondary_weapon_egg(playerobj, PROXIMITY_INDEX, (Players[playerobj->id].secondary_ammo[PROXIMITY_INDEX])/4);

		maybe_drop_secondary_weapon_egg(playerobj, SMART_INDEX, Players[playerobj->id].secondary_ammo[SMART_INDEX]);
		maybe_drop_secondary_weapon_egg(playerobj, MEGA_INDEX, Players[playerobj->id].secondary_ammo[MEGA_INDEX]);

		maybe_drop_secondary_weapon_egg(playerobj, SMART_MINE_INDEX,(Players[playerobj->id].secondary_ammo[SMART_MINE_INDEX])/4);
		maybe_drop_secondary_weapon_egg(playerobj, SMISSILE5_INDEX, Players[playerobj->id].secondary_ammo[SMISSILE5_INDEX]);

		//	Drop the player's missiles in packs of 1 and/or 4
		drop_missile_1_or_4(playerobj,HOMING_INDEX);
		drop_missile_1_or_4(playerobj,GUIDED_INDEX);
		drop_missile_1_or_4(playerobj,CONCUSSION_INDEX);
		drop_missile_1_or_4(playerobj,SMISSILE1_INDEX);
		drop_missile_1_or_4(playerobj,SMISSILE4_INDEX);

		//	If player has vulcan ammo, but no vulcan cannon, drop the ammo.
		if (!(Players[playerobj->id].primary_weapon_flags & HAS_VULCAN_FLAG)) {
			int	amount = Players[playerobj->id].primary_ammo[VULCAN_INDEX];
			if (amount > 200) {
				mprintf((0, "Surprising amount of vulcan ammo: %i bullets.\n", amount));
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

//--		//	Drop all the keys.
//--		if (Players[Player_num].flags & PLAYER_FLAGS_BLUE_KEY) {
//--			playerobj->contains_count = 1;
//--			playerobj->contains_type = OBJ_POWERUP;
//--			playerobj->contains_id = POW_KEY_BLUE;
//--			object_create_egg(playerobj);
//--		}
//--		if (Players[Player_num].flags & PLAYER_FLAGS_RED_KEY) {
//--			playerobj->contains_count = 1;
//--			playerobj->contains_type = OBJ_POWERUP;
//--			playerobj->contains_id = POW_KEY_RED;
//--			object_create_egg(playerobj);
//--		}
//--		if (Players[Player_num].flags & PLAYER_FLAGS_GOLD_KEY) {
//--			playerobj->contains_count = 1;
//--			playerobj->contains_type = OBJ_POWERUP;
//--			playerobj->contains_id = POW_KEY_GOLD;
//--			object_create_egg(playerobj);
//--		}

// -- 		if (Items_destroyed) {
// -- 			if (Items_destroyed == 1)
// -- 				HUD_init_message("%i item was destroyed.", Items_destroyed);
// -- 			else
// -- 				HUD_init_message("%i items were destroyed.", Items_destroyed);
// -- 			Items_destroyed = 0;
// -- 		}
	}

}

// -- removed, 09/06/95, MK -- void destroy_primary_weapon(int weapon_index)
// -- removed, 09/06/95, MK -- {
// -- removed, 09/06/95, MK -- 	if (weapon_index == MAX_PRIMARY_WEAPONS) {
// -- removed, 09/06/95, MK -- 		HUD_init_message("Quad lasers destroyed!");
// -- removed, 09/06/95, MK -- 		Players[Player_num].flags &= ~PLAYER_FLAGS_QUAD_LASERS;
// -- removed, 09/06/95, MK -- 		update_laser_weapon_info();
// -- removed, 09/06/95, MK -- 	} else if (weapon_index == 0) {
// -- removed, 09/06/95, MK -- 		Assert(Players[Player_num].laser_level > 0);
// -- removed, 09/06/95, MK -- 		HUD_init_message("%s degraded!", Text_string[104+weapon_index]);		//	Danger! Danger! Use of literal!  Danger!
// -- removed, 09/06/95, MK -- 		Players[Player_num].laser_level--;
// -- removed, 09/06/95, MK -- 		update_laser_weapon_info();
// -- removed, 09/06/95, MK -- 	} else {
// -- removed, 09/06/95, MK -- 		HUD_init_message("%s destroyed!", Text_string[104+weapon_index]);		//	Danger! Danger! Use of literal!  Danger!
// -- removed, 09/06/95, MK -- 		Players[Player_num].primary_weapon_flags &= ~(1 << weapon_index);
// -- removed, 09/06/95, MK -- 		auto_select_weapon(0);
// -- removed, 09/06/95, MK -- 	}
// -- removed, 09/06/95, MK -- 
// -- removed, 09/06/95, MK -- }
// -- removed, 09/06/95, MK -- 
// -- removed, 09/06/95, MK -- void destroy_secondary_weapon(int weapon_index)
// -- removed, 09/06/95, MK -- {
// -- removed, 09/06/95, MK -- 	if (Players[Player_num].secondary_ammo <= 0)
// -- removed, 09/06/95, MK -- 		return;
// -- removed, 09/06/95, MK -- 
// -- removed, 09/06/95, MK -- 	HUD_init_message("%s destroyed!", Text_string[114+weapon_index]);		//	Danger! Danger! Use of literal!  Danger!
// -- removed, 09/06/95, MK -- 	if (--Players[Player_num].secondary_ammo[weapon_index] == 0)
// -- removed, 09/06/95, MK -- 		auto_select_weapon(1);
// -- removed, 09/06/95, MK -- 
// -- removed, 09/06/95, MK -- }
// -- removed, 09/06/95, MK -- 
// -- removed, 09/06/95, MK -- #define	LOSE_WEAPON_THRESHOLD	(F1_0*30)

extern fix Buddy_sorry_time;

void apply_damage_to_player(object *playerobj, object *killer, fix damage)
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

	if (playerobj->id == Player_num) {		//is this the local player?

		//	MK: 08/14/95: This code can never be reached.  See the return about 12 lines up.
// -- 		if (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE) {
// -- 
// -- 			//invincible, so just do blue flash
// -- 
// -- 			PALETTE_FLASH_ADD(0,0,f2i(damage)*4);	//flash blue
// -- 
// -- 		} 
// -- 		else {		//take damage, do red flash

			Players[Player_num].shields -= damage;

			PALETTE_FLASH_ADD(f2i(damage)*4,-f2i(damage/2),-f2i(damage/2));	//flash red

// -- 		}

		if (Players[Player_num].shields < 0)	{

  			Players[Player_num].killer_objnum = killer-Objects;
			
//			if ( killer && (killer->type == OBJ_PLAYER))
//				Players[Player_num].killer_objnum = killer-Objects;

			playerobj->flags |= OF_SHOULD_BE_DEAD;

			if (Buddy_objnum != -1)
				if (killer && (killer->type == OBJ_ROBOT) && (Robot_info[killer->id].companion))
					Buddy_sorry_time = GameTime;
		}
// -- removed, 09/06/95, MK --  else if (Players[Player_num].shields < LOSE_WEAPON_THRESHOLD) {
// -- removed, 09/06/95, MK -- 			int	randnum = d_rand();
// -- removed, 09/06/95, MK -- 
// -- removed, 09/06/95, MK -- 			if (fixmul(Players[Player_num].shields, randnum) < damage/4) {
// -- removed, 09/06/95, MK -- 				if (d_rand() > 20000) {
// -- removed, 09/06/95, MK -- 					destroy_secondary_weapon(Secondary_weapon);
// -- removed, 09/06/95, MK -- 				} else if (Primary_weapon == 0) {
// -- removed, 09/06/95, MK -- 					if (Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS)
// -- removed, 09/06/95, MK -- 						destroy_primary_weapon(MAX_PRIMARY_WEAPONS);	//	This means to destroy quad laser.
// -- removed, 09/06/95, MK -- 					else if (Players[Player_num].laser_level > 0)
// -- removed, 09/06/95, MK -- 						destroy_primary_weapon(Primary_weapon);
// -- removed, 09/06/95, MK -- 				} else
// -- removed, 09/06/95, MK -- 					destroy_primary_weapon(Primary_weapon);
// -- removed, 09/06/95, MK -- 			} else
// -- removed, 09/06/95, MK -- 				; // mprintf((0, "%8x > %8x, so don't lose weapon.\n", fixmul(Players[Player_num].shields, randnum), damage/4));
// -- removed, 09/06/95, MK -- 		}

		playerobj->shields = Players[Player_num].shields;		//mirror

	}
}

void collide_player_and_weapon( object * playerobj, object * weapon, vms_vector *collision_point )
{
	fix		damage = weapon->shields;
	object * killer=NULL;

	//	In multiplayer games, only do damage to another player if in first frame.
	//	This is necessary because in multiplayer, due to varying framerates, omega blobs actually
	//	have a bit of a lifetime.  But they start out with a lifetime of ONE_FRAME_TIME, and this
	//	gets bashed to 1/4 second in laser_do_weapon_sequence.  This bashing occurs for visual purposes only.
	if (weapon->id == OMEGA_ID)
		if (!ok_to_do_omega_damage(weapon))
			return;

	//	Don't collide own smart mines unless direct hit.
	if (weapon->id == SUPERPROX_ID)
		if (playerobj-Objects == weapon->ctype.laser_info.parent_num)
			if (vm_vec_dist_quick(collision_point, &playerobj->pos) > playerobj->size)
				return;

	if (weapon->id == EARTHSHAKER_ID)
		smega_rock_stuff();

	damage = fixmul(damage, weapon->ctype.laser_info.multiplier);
#ifndef SHAREWARE
	if (Game_mode & GM_MULTI)
		damage = fixmul(damage, Weapon_info[weapon->id].multi_damage_scale);
#endif

	if (weapon->mtype.phys_info.flags & PF_PERSISTENT)
	{
		if (weapon->ctype.laser_info.last_hitobj == playerobj-Objects)
			return;
		else
			weapon->ctype.laser_info.last_hitobj = playerobj-Objects;
	}

	if (playerobj->id == Player_num)
	{
		if (!(Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE))
		{
			digi_link_sound_to_pos( SOUND_PLAYER_GOT_HIT, playerobj->segnum, 0, collision_point, 0, F1_0 );
			#ifdef NETWORK
			if (Game_mode & GM_MULTI)
				multi_send_play_sound(SOUND_PLAYER_GOT_HIT, F1_0);
			#endif
		}
		else
		{
			digi_link_sound_to_pos( SOUND_WEAPON_HIT_DOOR, playerobj->segnum, 0, collision_point, 0, F1_0);
			#ifdef NETWORK
			if (Game_mode & GM_MULTI)
				multi_send_play_sound(SOUND_WEAPON_HIT_DOOR, F1_0);
			#endif
		}
	}

	object_create_explosion( playerobj->segnum, collision_point, i2f(10)/2, VCLIP_PLAYER_HIT );
	if ( Weapon_info[weapon->id].damage_radius )
		explode_badass_weapon(weapon,collision_point);

	maybe_kill_weapon(weapon,playerobj);

	bump_two_objects(playerobj, weapon, 0);	//no damage from bump

	if ( !Weapon_info[weapon->id].damage_radius ) {
		if ( weapon->ctype.laser_info.parent_num > -1 )
			killer = &Objects[weapon->ctype.laser_info.parent_num];

//		if (weapon->id == SMART_HOMING_ID)
//			damage /= 4;

		if (!(weapon->flags & OF_HARMLESS))
			apply_damage_to_player( playerobj, killer, damage);
	}

	//	Robots become aware of you if you get hit.
	ai_do_cloak_stuff();

	return; 
}

//	Nasty robots are the ones that attack you by running into you and doing lots of damage.
void collide_player_and_nasty_robot( object * playerobj, object * robot, vms_vector *collision_point )
{
//	if (!(Robot_info[robot->id].energy_drain && Players[playerobj->id].energy))
		digi_link_sound_to_pos( Robot_info[robot->id].claw_sound, playerobj->segnum, 0, collision_point, 0, F1_0 );

	object_create_explosion( playerobj->segnum, collision_point, i2f(10)/2, VCLIP_PLAYER_HIT );

	bump_two_objects(playerobj, robot, 0);	//no damage from bump

	apply_damage_to_player( playerobj, robot, F1_0*(Difficulty_level+1));

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

	apply_damage_to_player( objp, objp, 4*F1_0);	//	Changed, MK, 2/19/96, make killer the player, so if you die in matcen, will say you killed yourself

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

//##void collide_player_and_camera( object * playerobj, object * camera, vms_vector *collision_point ) { 
//##	return; 
//##}

extern int Network_got_powerup; // HACK!!!

void collide_player_and_powerup( object * playerobj, object * powerup, vms_vector *collision_point ) { 
	if (!Endlevel_sequence && !Player_is_dead && (playerobj->id == Player_num )) {
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
	else if ((Game_mode & GM_MULTI_COOP) && (playerobj->id != Player_num))
	{
		switch (powerup->id) {
			case POW_KEY_BLUE:	
				Players[playerobj->id].flags |= PLAYER_FLAGS_BLUE_KEY;
				break;
			case POW_KEY_RED:	
				Players[playerobj->id].flags |= PLAYER_FLAGS_RED_KEY;
				break;
			case POW_KEY_GOLD:	
				Players[playerobj->id].flags |= PLAYER_FLAGS_GOLD_KEY;
				break;
			default:
				break;
		}
	}
#endif
	return; 
}

//##void collide_player_and_debris( object * playerobj, object * debris, vms_vector *collision_point ) { 
//##	return; 
//##}

void collide_player_and_clutter( object * playerobj, object * clutter, vms_vector *collision_point ) { 
	digi_link_sound_to_pos( SOUND_ROBOT_HIT_PLAYER, playerobj->segnum, 0, collision_point, 0, F1_0 );
	bump_two_objects(clutter, playerobj, 1);
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
				explode_badass_weapon(weapon1,collision_point);
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
	// -- Does this look buggy??:  if (weapon1->id == PMINE_ID && weapon1->id == PMINE_ID)
	if (weapon1->id == PMINE_ID && weapon2->id == PMINE_ID)
		return;		//these can't blow each other up  

	if (weapon1->id == OMEGA_ID) {
		if (!ok_to_do_omega_damage(weapon1))
			return;
	} else if (weapon2->id == OMEGA_ID) {
		if (!ok_to_do_omega_damage(weapon2))
			return;
	}

	if ((Weapon_info[weapon1->id].destroyable) || (Weapon_info[weapon2->id].destroyable)) {

		//	Bug reported by Adam Q. Pletcher on September 9, 1994, smart bomb homing missiles were toasting each other.
		if ((weapon1->id == weapon2->id) && (weapon1->ctype.laser_info.parent_num == weapon2->ctype.laser_info.parent_num))
			return;

		if (Weapon_info[weapon1->id].destroyable)
			if (maybe_detonate_weapon(weapon1, weapon2, collision_point))
				maybe_detonate_weapon(weapon2,weapon1, collision_point);

		if (Weapon_info[weapon2->id].destroyable)
			if (maybe_detonate_weapon(weapon2, weapon1, collision_point))
				maybe_detonate_weapon(weapon1,weapon2, collision_point);

	}

}

//##void collide_weapon_and_camera( object * weapon, object * camera, vms_vector *collision_point ) { 
//##	return; 
//##}

//##void collide_weapon_and_powerup( object * weapon, object * powerup, vms_vector *collision_point ) { 
//##	return; 
//##}

void collide_weapon_and_debris( object * weapon, object * debris, vms_vector *collision_point ) { 

	//	Hack!  Prevent debris from causing bombs spewed at player death to detonate!
	if ((weapon->id == PROXIMITY_ID) || (weapon->id == SUPERPROX_ID)) {
		if (weapon->ctype.laser_info.creation_time + F1_0/2 > GameTime)
			return;
	}

	if ( (weapon->ctype.laser_info.parent_type==OBJ_PLAYER) && !(debris->flags & OF_EXPLODING) )	{	
		digi_link_sound_to_pos( SOUND_ROBOT_HIT, weapon->segnum , 0, collision_point, 0, F1_0 );

		explode_object(debris,0);
		if ( Weapon_info[weapon->id].damage_radius )
			explode_badass_weapon(weapon,collision_point);
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


/* DPH: Put these macros on one long line to avoid CR/LF problems on linux */
#define COLLISION_OF(a,b) (((a)<<8) + (b))

#define DO_COLLISION(type1,type2,collision_function)	case COLLISION_OF( (type1), (type2) ):	(collision_function)( (A), (B), collision_point ); break;   case COLLISION_OF( (type2), (type1) ):  (collision_function)( (B), (A), collision_point );  break;

#define DO_SAME_COLLISION(type1,type2,collision_function)    case COLLISION_OF( (type1), (type1) ):  (collision_function)( (A), (B), collision_point ); break;

//these next two macros define a case that does nothing
#define NO_COLLISION(type1,type2,collision_function)	case COLLISION_OF( (type1), (type2) ):	break;	case COLLISION_OF( (type2), (type1) ):	break;

#define NO_SAME_COLLISION(type1,type2,collision_function)    case COLLISION_OF( (type1), (type1) ):    break;

/* DPH: These ones are never used so I'm not going to bother */
#ifndef __GNUC__
#define IGNORE_COLLISION(type1,type2,collision_function)					\
	case COLLISION_OF( (type1), (type2) ):										\
		break;																			\
	case COLLISION_OF( (type2), (type1) ):										\
		break;

#define ERROR_COLLISION(type1,type2,collision_function)					\
	case COLLISION_OF( (type1), (type2) ):										\
		Error( "Error in collision type!" );									\
		break;																			\
	case COLLISION_OF( (type2), (type1) ):										\
		Error( "Error in collision type!" );									\
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
	NO_SAME_COLLISION( OBJ_MARKER, OBJ_MARKER,  NULL )
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

	DO_COLLISION( OBJ_MARKER, OBJ_PLAYER,  collide_player_and_marker)
	NO_COLLISION( OBJ_MARKER, OBJ_ROBOT,   NULL)
	NO_COLLISION( OBJ_MARKER, OBJ_HOSTAGE, NULL)
	NO_COLLISION( OBJ_MARKER, OBJ_WEAPON,  NULL)
	NO_COLLISION( OBJ_MARKER, OBJ_CAMERA,  NULL)
	NO_COLLISION( OBJ_MARKER, OBJ_POWERUP, NULL)
	NO_COLLISION( OBJ_MARKER, OBJ_DEBRIS,  NULL)

	default:
		Int3();	//Error( "Unhandled collision_type in collide.c!\n" );
	}
}

#define ENABLE_COLLISION(type1,type2)					\
	CollisionResult[type1][type2] = RESULT_CHECK;	\
	CollisionResult[type2][type1] = RESULT_CHECK;

#define DISABLE_COLLISION(type1,type2)					\
	CollisionResult[type1][type2] = RESULT_NOTHING;	\
	CollisionResult[type2][type1] = RESULT_NOTHING;

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

	ENABLE_COLLISION( OBJ_PLAYER, OBJ_MARKER );

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


