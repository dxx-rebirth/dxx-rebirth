/* $Id: object.c,v 1.11 2004-05-15 17:24:17 schaffner Exp $ */
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
 * object rendering
 *
 * Old Log:
 * Revision 1.5  1995/10/26  14:08:03  allender
 * optimization to do physics on objects that didn't render last
 * frame only every so often
 *
 * Revision 1.4  1995/10/20  00:50:57  allender
 * make alt texture for player ship work correctly when cloaked
 *
 * Revision 1.3  1995/09/14  14:11:32  allender
 * fix_object_segs returns void
 *
 * Revision 1.2  1995/08/12  11:31:01  allender
 * removed #ifdef NEWDEMO -- always in
 *
 * Revision 1.1  1995/05/16  15:29:23  allender
 * Initial revision
 *
 * Revision 2.3  1995/06/15  12:30:51  john
 * Fixed bug with multiplayer ships cloaking out wrongly.
 *
 * Revision 2.2  1995/05/15  11:34:53  john
 * Fixed bug as Matt directed that fixed problems with the exit
 * triggers being missed on slow frame rate computer.
 *
 * Revision 2.1  1995/03/21  14:38:51  john
 * Ifdef'd out the NETWORK code.
 *
 * Revision 2.0  1995/02/27  11:28:14  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.335  1995/02/22  12:57:30  allender
 * remove anonymous unions from object structure
 *
 * Revision 1.334  1995/02/09  22:04:40  mike
 * fix lifeleft on badass weapons.
 *
 * Revision 1.333  1995/02/08  12:54:00  matt
 * Fixed object freeing code which was deleting some explosions it shouldn't
 *
 * Revision 1.332  1995/02/08  11:37:26  mike
 * Check for failures in call to obj_create.
 *
 * Revision 1.331  1995/02/05  17:48:52  rob
 * Changed assert in obj_relink, more robust.
 *
 * Revision 1.330  1995/02/05  13:39:48  mike
 * remove invulnerability effect code (actually, comment out).
 *
 * Revision 1.329  1995/02/04  12:29:52  rob
 * Get rid of potential assert error for explosion detachment.
 *
 * Revision 1.328  1995/02/01  18:15:57  rob
 * Removed debugging output from engine glow change.
 *
 * Revision 1.327  1995/02/01  16:20:12  matt
 * Made engine glow vary over a wider range, and made the glow be based
 * on thrust/speed, without regard to direction.
 *
 * Revision 1.326  1995/01/29  14:46:24  rob
 * Fixed invul. vclip to only appear on player who is invul.
 *
 * Revision 1.325  1995/01/29  13:48:16  mike
 * Add invulnerability graphical effect viewable by other players.
 *
 * Revision 1.324  1995/01/29  11:39:25  mike
 * Add invulnerability effect.
 *
 * Revision 1.323  1995/01/27  17:02:41  mike
 * add more information to an Error call.
 *
 * Revision 1.322  1995/01/26  22:11:30  mike
 * Purple chromo-blaster (ie, fusion cannon) spruce up (chromification)
 *
 * Revision 1.321  1995/01/25  20:04:10  matt
 * Moved matrix check to avoid orthogonalizing an uninitialize matrix
 *
 * Revision 1.320  1995/01/25  12:11:35  matt
 * Make sure orient matrix is orthogonal when resetting player object
 *
 * Revision 1.319  1995/01/21  21:46:22  mike
 * Optimize code in Assert (and prevent warning message).
 *
 * Revision 1.318  1995/01/21  21:22:16  rob
 * Removed HUD clear messages.
 * Added more Asserts to try and find illegal control type bug.
 *
 * Revision 1.317  1995/01/15  15:34:30  matt
 * When freeing object slots, don't free fireballs that will be deleting
 * other objects.
 *
 * Revision 1.316  1995/01/14  19:16:48  john
 * First version of new bitmap paging code.
 *
 * Revision 1.315  1995/01/12  18:53:37  john
 * Fixed parameter passing error.
 *
 * Revision 1.314  1995/01/12  12:09:47  yuan
 * Added coop object capability.
 *
 * Revision 1.313  1994/12/15  16:45:44  matt
 * Took out slew stuff for release version
 *
 * Revision 1.312  1994/12/15  13:04:25  mike
 * Replace Players[Player_num].time_total references with GameTime.
 *
 * Revision 1.311  1994/12/15  11:01:04  mike
 * add Object_minus_one for debugging.
 *
 * Revision 1.310  1994/12/15  03:03:33  matt
 * Added error checking for NULL return from object_create_explosion()
 *
 * Revision 1.309  1994/12/14  17:25:31  matt
 * Made next viewer func based on release not ndebug
 *
 * Revision 1.308  1994/12/13  12:55:42  mike
 * hostages on board messages for when you die.
 *
 * Revision 1.307  1994/12/12  17:18:11  mike
 * make boss cloak/teleport when get hit, make quad laser 3/4 as powerful.
 *
 * Revision 1.306  1994/12/12  00:27:11  matt
 * Added support for no-levelling option
 *
 * Revision 1.305  1994/12/11  22:41:14  matt
 * Added command-line option, -nolevel, which turns off player ship levelling
 *
 * Revision 1.304  1994/12/11  22:03:23  mike
 * free up object slots as necessary.
 *
 * Revision 1.303  1994/12/11  14:09:31  mike
 * make boss explosion sounds softer.
 *
 * Revision 1.302  1994/12/11  13:25:11  matt
 * Restored calls to fix_object_segs() when debugging is turned off, since
 * it's not a big routine, and could fix some possibly bad problems.
 *
 * Revision 1.301  1994/12/11  12:38:25  mike
 * make boss explosion sounds louder in create_small_fireball.
 *
 * Revision 1.300  1994/12/10  15:28:37  matt
 * Added asserts for debugging
 *
 * Revision 1.299  1994/12/09  16:18:51  matt
 * Fixed init_player_object, for editor
 *
 * Revision 1.298  1994/12/09  15:03:10  matt
 * Two changes for Mike:
 *   1.  Do better placement of camera during death sequence (prevents hang)
 *   2.  Only record dodging information if the player fired in a frame
 *
 * Revision 1.297  1994/12/09  14:59:12  matt
 * Added system to attach a fireball to another object for rendering purposes,
 * so the fireball always renders on top of (after) the object.
 *
 * Revision 1.296  1994/12/08  20:05:07  matt
 * Removed unneeded debug message
 *
 * Revision 1.295  1994/12/08  12:36:02  matt
 * Added new object allocation & deallocation functions so other code
 * could stop messing around with internal object data structures.
 *
 * Revision 1.294  1994/12/07  20:13:37  matt
 * Added debris object limiter
 *
 * Revision 1.293  1994/12/06  16:58:38  matt
 * Killed warnings
 *
 * Revision 1.292  1994/12/05  22:34:35  matt
 * Make tmap_override objects use override texture as alt texture.  This
 * should have the effect of making simpler models use the override texture.
 *
 * Revision 1.291  1994/12/05  12:23:53  mike
 * make camera start closer, but move away from player in death sequence.
 *
 * Revision 1.290  1994/12/02  11:11:18  mike
 * hook sound effect to player small explosions (ctrlcen, too).
 *
 * Revision 1.289  1994/11/28  21:50:52  mike
 * optimizations.
 *
 * Revision 1.288  1994/11/27  23:12:28  matt
 * Made changes for new mprintf calling convention
 *
 * Revision 1.287  1994/11/27  20:35:50  matt
 * Fixed dumb mistake
 *
 * Revision 1.286  1994/11/27  20:30:52  matt
 * Got rid of warning
 *
 * Revision 1.285  1994/11/21  11:43:21  mike
 * ndebug stuff.
 *
 * Revision 1.284  1994/11/19  15:19:37  mike
 * rip out unused code and data.
 *
 * Revision 1.283  1994/11/18  23:41:59  john
 * Changed some shorts to ints.
 *
 * Revision 1.282  1994/11/18  16:16:17  mike
 * Separate depth on objects vs. walls.
 *
 * Revision 1.281  1994/11/18  12:05:35  rob
 * Removed unnecessary invulnerability flag set in player death.
 * (I hope its unnecessary.. its commented out if it proves crucial)
 * fixes powerup dropping bug for net play.
 *
 * Revision 1.280  1994/11/16  20:36:34  rob
 * Changed player explosion (small) code.
 *
 * Revision 1.279  1994/11/16  18:26:04  matt
 * Clear tmap override on player, to fix "rock ship" bug
 *
 * Revision 1.278  1994/11/16  14:54:12  rob
 * Moved hook for network explosions.
 *
 * Revision 1.277  1994/11/14  11:40:42  mike
 * plot inner polygon on laser based on detail level.
 *
 * Revision 1.276  1994/11/10  14:02:59  matt
 * Hacked in support for player ships with different textures
 *
 * Revision 1.275  1994/11/08  12:19:08  mike
 * Make a generally useful function for putting small explosions on any object.
 *
 * Revision 1.274  1994/11/04  19:55:54  rob
 * Changed calls to player_explode to accomodate new parameter.
 *
 * Revision 1.273  1994/11/02  21:54:27  matt
 * Delete the camera when the death sequence is done
 *
 * Revision 1.272  1994/11/02  11:36:35  rob
 * Added player-in-process-of-dying explosions to network play.
 *
 * Revision 1.271  1994/10/31  17:25:33  matt
 * Fixed cloaked bug
 *
 * Revision 1.270  1994/10/31  16:11:19  allender
 * on demo recording, store letterbox mode in demo.
 *
 * Revision 1.269  1994/10/31  10:36:18  mike
 * Make cloak effect fadein/fadeout different for robots versus player.
 *
 * Revision 1.268  1994/10/30  14:11:44  mike
 * rip out repair center stuff.
 *
 * Revision 1.267  1994/10/28  19:43:52  mike
 * Boss cloaking effect.
 *
 * Revision 1.266  1994/10/27  11:33:42  mike
 * Add Highest_ever_object_index -- high water mark in object creation.
 *
 * Revision 1.265  1994/10/25  10:51:12  matt
 * Vulcan cannon powerups now contain ammo count
 *
 * Revision 1.264  1994/10/24  20:49:24  matt
 * Made cloaked objects pulse
 *
 * Revision 1.263  1994/10/21  12:19:45  matt
 * Clear transient objects when saving (& loading) games
 *
 * Revision 1.262  1994/10/21  11:25:23  mike
 * Use new constant IMMORTAL_TIME.
 *
 * Revision 1.261  1994/10/19  16:50:35  matt
 * If out of segment, put player in center of segment when checking objects
 *
 *
 * Revision 1.260  1994/10/17  23:21:55  mike
 * Clean up robot cloaking, move more to ai.c
 *
 * Revision 1.259  1994/10/17  21:34:49  matt
 * Added support for new Control Center/Main Reactor
 *
 * Revision 1.258  1994/10/17  21:18:04  mike
 * robot cloaking.
 *
 * Revision 1.257  1994/10/17  14:12:23  matt
 * Cleaned up problems with player dying from mine explosion
 *
 * Revision 1.256  1994/10/15  19:04:31  mike
 * Don't remove proximity bombs after you die.
 *
 * Revision 1.255  1994/10/14  15:57:00  mike
 * Don't show ids in network mode.
 * Fix, I hope, but in death sequence.
 *
 * Revision 1.254  1994/10/12  08:04:29  mike
 * Don't decloak player on death.
 *
 * Revision 1.253  1994/10/11  20:36:16  matt
 * Clear "transient" objects (weapons,explosions,etc.) when starting a level
 *
 * Revision 1.252  1994/10/11  12:24:09  matt
 * Cleaned up/change badass explosion calls
 *
 * Revision 1.251  1994/10/08  19:30:20  matt
 * Fixed (I hope) a bug in cloaking of multiplayer objects
 *
 * Revision 1.250  1994/10/08  14:03:15  rob
 * Changed cloaking routine.
 *
 * Revision 1.249  1994/10/07  22:17:27  mike
 * Asserts on valid segnum.
 *
 * Revision 1.248  1994/10/07  19:11:14  matt
 * Added cool cloak transition effect
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef WINDOWS
#include "desw.h"
#endif

#include <string.h>	// for memset
#include <stdio.h>

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
#include "textures.h"

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
#include "pa_enabl.h"
#include "ai.h"
#include "hostage.h"
#include "morph.h"
#include "cntrlcen.h"
#include "powerup.h"
#include "fuelcen.h"
#include "endlevel.h"

#include "sounds.h"
#include "collide.h"

#include "lighting.h"
#include "newdemo.h"
#include "player.h"
#include "weapon.h"
#ifdef NETWORK
#include "network.h"
#endif
#include "newmenu.h"
#include "gauges.h"
#include "multi.h"
#include "menu.h"
#include "args.h"
#include "text.h"
#include "piggy.h"
#include "switch.h"
#include "gameseq.h"

#ifdef TACTILE
#include "tactile.h"
#endif

#ifdef EDITOR
#include "editor/editor.h"
#endif

#ifdef _3DFX
#include "3dfx_des.h"
#endif

void obj_detach_all(object *parent);
void obj_detach_one(object *sub);
int free_object_slots(int num_used);

/*
 *  Global variables
 */

extern sbyte WasRecorded[MAX_OBJECTS];

ubyte CollisionResult[MAX_OBJECT_TYPES][MAX_OBJECT_TYPES];

object *ConsoleObject;					//the object that is the player

static short free_obj_list[MAX_OBJECTS];

//Data for objects

// -- Object stuff

//info on the various types of objects
#ifndef NDEBUG
object	Object_minus_one;
#endif

object Objects[MAX_OBJECTS];
int num_objects=0;
int Highest_object_index=0;
int Highest_ever_object_index=0;

// grs_bitmap *robot_bms[MAX_ROBOT_BITMAPS];	//all bitmaps for all robots

// int robot_bm_nums[MAX_ROBOT_TYPES];		//starting bitmap num for each robot
// int robot_n_bitmaps[MAX_ROBOT_TYPES];		//how many bitmaps for each robot

// char *robot_names[MAX_ROBOT_TYPES];		//name of each robot

//--unused-- int Num_robot_types=0;

int print_object_info = 0;
//@@int Object_viewer = 0;

//object * Slew_object = NULL;	// Object containing slew object info.

//--unused-- int Player_controller_type = 0;

window_rendered_data Window_rendered_data[MAX_RENDERED_WINDOWS];

#ifndef NDEBUG
char	Object_type_names[MAX_OBJECT_TYPES][9] = {
	"WALL    ",
	"FIREBALL",
	"ROBOT   ",
	"HOSTAGE ",
	"PLAYER  ",
	"WEAPON  ",
	"CAMERA  ",
	"POWERUP ",
	"DEBRIS  ",
	"CNTRLCEN",
	"FLARE   ",
	"CLUTTER ",
	"GHOST   ",
	"LIGHT   ",
	"COOP    ",
	"MARKER  ",
};
#endif

#ifndef RELEASE
//set viewer object to next object in array
void object_goto_next_viewer()
{
	int i, start_obj = 0;

	start_obj = Viewer - Objects;		//get viewer object number
	
	for (i=0;i<=Highest_object_index;i++) {

		start_obj++;
		if (start_obj > Highest_object_index ) start_obj = 0;

		if (Objects[start_obj].type != OBJ_NONE )	{
			Viewer = &Objects[start_obj];
			return;
		}
	}

	Error( "Couldn't find a viewer object!" );

}

//set viewer object to next object in array
void object_goto_prev_viewer()
{
	int i, start_obj = 0;

	start_obj = Viewer - Objects;		//get viewer object number
	
	for (i=0; i<=Highest_object_index; i++) {

		start_obj--;
		if (start_obj < 0 ) start_obj = Highest_object_index;

		if (Objects[start_obj].type != OBJ_NONE )	{
			Viewer = &Objects[start_obj];
			return;
		}
	}

	Error( "Couldn't find a viewer object!" );

}
#endif

object *obj_find_first_of_type (int type)
 {
  int i;

  for (i=0;i<=Highest_object_index;i++)
	if (Objects[i].type==type)
	 return (&Objects[i]);
  return ((object *)NULL);
 }

int obj_return_num_of_type (int type)
 {
  int i,count=0;

  for (i=0;i<=Highest_object_index;i++)
	if (Objects[i].type==type)
	 count++;
  return (count);
 }
int obj_return_num_of_typeid (int type,int id)
 {
  int i,count=0;

  for (i=0;i<=Highest_object_index;i++)
	if (Objects[i].type==type && Objects[i].id==id)
	 count++;
  return (count);
 }

int global_orientation = 0;

//draw an object that has one bitmap & doesn't rotate
void draw_object_blob(object *obj,bitmap_index bmi)
{
	int	orientation=0;
	grs_bitmap * bm = &GameBitmaps[bmi.index];


	if (obj->type == OBJ_FIREBALL)
		orientation = (obj-Objects) & 7;

	orientation = global_orientation;

//@@#ifdef WINDOWS
//@@	if (obj->type == OBJ_POWERUP) {
//@@		if ( GameBitmaps[(bmi).index].bm_flags & BM_FLAG_PAGED_OUT)
//@@			piggy_bitmap_page_in_w( bmi,1 );
//@@	}
//@@	if (bm->bm_handle) {
//@@		DDGRUNLOCK(dd_grd_curcanv);
//@@	}
//@@#else
	PIGGY_PAGE_IN( bmi );
//@@#endif	

	if (bm->bm_w > bm->bm_h)

		g3_draw_bitmap(&obj->pos,obj->size,fixmuldiv(obj->size,bm->bm_h,bm->bm_w),bm, orientation);

	else

		g3_draw_bitmap(&obj->pos,fixmuldiv(obj->size,bm->bm_w,bm->bm_h),obj->size,bm, orientation);

//@@#ifdef WINDOWS
//@@	if (bm->bm_handle) {
//@@		DDGRLOCK(dd_grd_curcanv);
//@@	}
//@@#endif
}

//draw an object that is a texture-mapped rod
void draw_object_tmap_rod(object *obj,bitmap_index bitmapi,int lighted)
{
	grs_bitmap * bitmap = &GameBitmaps[bitmapi.index];
	fix light;

	vms_vector delta,top_v,bot_v;
	g3s_point top_p,bot_p;

	PIGGY_PAGE_IN(bitmapi);

   bitmap->bm_handle = bitmapi.index;

	vm_vec_copy_scale(&delta,&obj->orient.uvec,obj->size);

	vm_vec_add(&top_v,&obj->pos,&delta);
	vm_vec_sub(&bot_v,&obj->pos,&delta);

	g3_rotate_point(&top_p,&top_v);
	g3_rotate_point(&bot_p,&bot_v);

	if (lighted)
		light = compute_object_light(obj,&top_p.p3_vec);
	else
		light = f1_0;

   #ifdef _3DFX
   _3dfx_rendering_poly_obj = 1;
   #endif

	#ifdef PA_3DFX_VOODOO
	light = f1_0;
	#endif

	g3_draw_rod_tmap(bitmap,&bot_p,obj->size,&top_p,obj->size,light);

   #ifdef _3DFX
   _3dfx_rendering_poly_obj = 0;
   #endif

}

int	Linear_tmap_polygon_objects = 1;

extern fix Max_thrust;

//used for robot engine glow
#define MAX_VELOCITY i2f(50)

//function that takes the same parms as draw_tmap, but renders as flat poly
//we need this to do the cloaked effect
extern void draw_tmap_flat();

//what darkening level to use when cloaked
#define CLOAKED_FADE_LEVEL		28

#define	CLOAK_FADEIN_DURATION_PLAYER	F2_0
#define	CLOAK_FADEOUT_DURATION_PLAYER	F2_0

#define	CLOAK_FADEIN_DURATION_ROBOT	F1_0
#define	CLOAK_FADEOUT_DURATION_ROBOT	F1_0

//do special cloaked render
void draw_cloaked_object(object *obj,fix light,fix *glow,fix cloak_start_time,fix cloak_end_time)
{
	fix cloak_delta_time,total_cloaked_time;
	fix light_scale=F1_0;
	int cloak_value=0;
	int fading=0;		//if true, fading, else cloaking
	fix	Cloak_fadein_duration=F1_0;
	fix	Cloak_fadeout_duration=F1_0;


	total_cloaked_time = cloak_end_time-cloak_start_time;

	switch (obj->type) {
		case OBJ_PLAYER:
			Cloak_fadein_duration = CLOAK_FADEIN_DURATION_PLAYER;
			Cloak_fadeout_duration = CLOAK_FADEOUT_DURATION_PLAYER;
			break;
		case OBJ_ROBOT:
			Cloak_fadein_duration = CLOAK_FADEIN_DURATION_ROBOT;
			Cloak_fadeout_duration = CLOAK_FADEOUT_DURATION_ROBOT;
			break;
		default:
			Int3();		//	Contact Mike: Unexpected object type in draw_cloaked_object.
	}

	cloak_delta_time = GameTime - cloak_start_time;

	if (cloak_delta_time < Cloak_fadein_duration/2) {

		light_scale = fixdiv(Cloak_fadein_duration/2 - cloak_delta_time,Cloak_fadein_duration/2);
		fading = 1;

	}
	else if (cloak_delta_time < Cloak_fadein_duration) {

		cloak_value = f2i(fixdiv(cloak_delta_time - Cloak_fadein_duration/2,Cloak_fadein_duration/2) * CLOAKED_FADE_LEVEL);

	} else if (GameTime < cloak_end_time-Cloak_fadeout_duration) {
		static int cloak_delta=0,cloak_dir=1;
		static fix cloak_timer=0;

		//note, if more than one cloaked object is visible at once, the
		//pulse rate will change!

		cloak_timer -= FrameTime;
		while (cloak_timer < 0) {

			cloak_timer += Cloak_fadeout_duration/12;

			cloak_delta += cloak_dir;

			if (cloak_delta==0 || cloak_delta==4)
				cloak_dir = -cloak_dir;
		}

		cloak_value = CLOAKED_FADE_LEVEL - cloak_delta;
	
	} else if (GameTime < cloak_end_time-Cloak_fadeout_duration/2) {

		cloak_value = f2i(fixdiv(total_cloaked_time - Cloak_fadeout_duration/2 - cloak_delta_time,Cloak_fadeout_duration/2) * CLOAKED_FADE_LEVEL);

	} else {

		light_scale = fixdiv(Cloak_fadeout_duration/2 - (total_cloaked_time - cloak_delta_time),Cloak_fadeout_duration/2);
		fading = 1;
	}


	if (fading) {
		fix new_light,save_glow;
		bitmap_index * alt_textures = NULL;

#ifdef NETWORK
		if ( obj->rtype.pobj_info.alt_textures > 0 )
			alt_textures = multi_player_textures[obj->rtype.pobj_info.alt_textures-1];
#endif

		new_light = fixmul(light,light_scale);
		save_glow = glow[0];
		glow[0] = fixmul(glow[0],light_scale);
		draw_polygon_model(&obj->pos,
				   &obj->orient,
				   (vms_angvec *)&obj->rtype.pobj_info.anim_angles,
				   obj->rtype.pobj_info.model_num,obj->rtype.pobj_info.subobj_flags,
				   new_light,
				   glow,
				   alt_textures );
		glow[0] = save_glow;
	}
	else {
		Gr_scanline_darkening_level = cloak_value;
		gr_setcolor(BM_XRGB(0,0,0));	//set to black (matters for s3)
		g3_set_special_render(draw_tmap_flat,NULL,NULL);		//use special flat drawer
		draw_polygon_model(&obj->pos,
				   &obj->orient,
				   (vms_angvec *)&obj->rtype.pobj_info.anim_angles,
				   obj->rtype.pobj_info.model_num,obj->rtype.pobj_info.subobj_flags,
				   light,
				   glow,
				   NULL );
		g3_set_special_render(NULL,NULL,NULL);
		Gr_scanline_darkening_level = GR_FADE_LEVELS;
	}

}

//draw an object which renders as a polygon model
void draw_polygon_object(object *obj)
{
	fix light;
	int	imsave;
	fix engine_glow_value[2];		//element 0 is for engine glow, 1 for headlight

	light = compute_object_light(obj,NULL);

	//	If option set for bright players in netgame, brighten them!
#ifdef NETWORK
	if (Game_mode & GM_MULTI)
		if (Netgame.BrightPlayers)
			light = F1_0;
#endif

	//make robots brighter according to robot glow field
	if (obj->type == OBJ_ROBOT)
		light += (Robot_info[obj->id].glow<<12);		//convert 4:4 to 16:16

	if (obj->type == OBJ_WEAPON)
		if (obj->id == FLARE_ID)
			light += F1_0*2;

	if (obj->type == OBJ_MARKER)
 		light += F1_0*2;


	imsave = Interpolation_method;
	if (Linear_tmap_polygon_objects)
		Interpolation_method = 1;

	//set engine glow value
	engine_glow_value[0] = f1_0/5;
	if (obj->movement_type == MT_PHYSICS) {

		if (obj->mtype.phys_info.flags & PF_USES_THRUST && obj->type==OBJ_PLAYER && obj->id==Player_num) {
			fix thrust_mag = vm_vec_mag_quick(&obj->mtype.phys_info.thrust);
			engine_glow_value[0] += (fixdiv(thrust_mag,Player_ship->max_thrust)*4)/5;
		}
		else {
			fix speed = vm_vec_mag_quick(&obj->mtype.phys_info.velocity);
			engine_glow_value[0] += (fixdiv(speed,MAX_VELOCITY)*3)/5;
		}
	}

	//set value for player headlight
	if (obj->type == OBJ_PLAYER) {
		if (Players[obj->id].flags & PLAYER_FLAGS_HEADLIGHT && !Endlevel_sequence)
			if (Players[obj->id].flags & PLAYER_FLAGS_HEADLIGHT_ON)
				engine_glow_value[1] = -2;		//draw white!
			else
				engine_glow_value[1] = -1;		//draw normal color (grey)
		else
			engine_glow_value[1] = -3;			//don't draw
	}

	if (obj->rtype.pobj_info.tmap_override != -1) {
#ifndef NDEBUG
		polymodel *pm = &Polygon_models[obj->rtype.pobj_info.model_num];
#endif
		bitmap_index bm_ptrs[12];

		int i;

		Assert(pm->n_textures<=12);

		for (i=0;i<12;i++)		//fill whole array, in case simple model needs more
			bm_ptrs[i] = Textures[obj->rtype.pobj_info.tmap_override];

		draw_polygon_model(&obj->pos,
				   &obj->orient,
				   (vms_angvec *)&obj->rtype.pobj_info.anim_angles,
				   obj->rtype.pobj_info.model_num,
				   obj->rtype.pobj_info.subobj_flags,
				   light,
				   engine_glow_value,
				   bm_ptrs);
	}
	else {

		if (obj->type==OBJ_PLAYER && (Players[obj->id].flags&PLAYER_FLAGS_CLOAKED))
			draw_cloaked_object(obj,light,engine_glow_value,Players[obj->id].cloak_time,Players[obj->id].cloak_time+CLOAK_TIME_MAX);
		else if ((obj->type == OBJ_ROBOT) && (obj->ctype.ai_info.CLOAKED)) {
			if (Robot_info[obj->id].boss_flag)
				draw_cloaked_object(obj,light,engine_glow_value, Boss_cloak_start_time, Boss_cloak_end_time);
			else
				draw_cloaked_object(obj,light,engine_glow_value, GameTime-F1_0*10, GameTime+F1_0*10);
		} else {
			bitmap_index * alt_textures = NULL;
	
			#ifdef NETWORK
			if ( obj->rtype.pobj_info.alt_textures > 0 )
				alt_textures = multi_player_textures[obj->rtype.pobj_info.alt_textures-1];
			#endif

			//	Snipers get bright when they fire.
			if (Ai_local_info[obj-Objects].next_fire < F1_0/8) {
				if (obj->ctype.ai_info.behavior == AIB_SNIPE)
					light = 2*light + F1_0;
			}

			draw_polygon_model(&obj->pos,
					   &obj->orient,
					   (vms_angvec *)&obj->rtype.pobj_info.anim_angles,obj->rtype.pobj_info.model_num,
					   obj->rtype.pobj_info.subobj_flags,
					   light,
					   engine_glow_value,
					   alt_textures);
			if (obj->type == OBJ_WEAPON && (Weapon_info[obj->id].model_num_inner > -1 )) {
				fix dist_to_eye = vm_vec_dist_quick(&Viewer->pos, &obj->pos);
				if (dist_to_eye < Simple_model_threshhold_scale * F1_0*2)
					draw_polygon_model(&obj->pos,
							   &obj->orient,
							   (vms_angvec *)&obj->rtype.pobj_info.anim_angles,
							   Weapon_info[obj->id].model_num_inner,
							   obj->rtype.pobj_info.subobj_flags,
							   light,
							   engine_glow_value,
							   alt_textures);
			}
		}
	}

	Interpolation_method = imsave;

}

//------------------------------------------------------------------------------
// These variables are used to keep a list of the 3 closest robots to the viewer.
// The code works like this: Every time render object is called with a polygon model,
// it finds the distance of that robot to the viewer.  If this distance if within 10
// segments of the viewer, it does the following: If there aren't already 3 robots in
// the closet-robots list, it just sticks that object into the list along with its distance.
// If the list already contains 3 robots, then it finds the robot in that list that is
// farthest from the viewer. If that object is farther than the object currently being
// rendered, then the new object takes over that far object's slot.  *Then* after all
// objects are rendered, object_render_targets is called an it draws a target on top
// of all the objects.

//091494: #define MAX_CLOSE_ROBOTS 3
//--unused-- static int Object_draw_lock_boxes = 0;
//091494: static int Object_num_close = 0;
//091494: static object * Object_close_ones[MAX_CLOSE_ROBOTS];
//091494: static fix Object_close_distance[MAX_CLOSE_ROBOTS];

//091494: set_close_objects(object *obj)
//091494: {
//091494: 	fix dist;
//091494:
//091494: 	if ( (obj->type != OBJ_ROBOT) || (Object_draw_lock_boxes==0) )	
//091494: 		return;
//091494:
//091494: 	// The following code keeps a list of the 10 closest robots to the
//091494: 	// viewer.  See comments in front of this function for how this works.
//091494: 	dist = vm_vec_dist( &obj->pos, &Viewer->pos );
//091494: 	if ( dist < i2f(20*10) )	{				
//091494: 		if ( Object_num_close < MAX_CLOSE_ROBOTS )	{
//091494: 			Object_close_ones[Object_num_close] = obj;
//091494: 			Object_close_distance[Object_num_close] = dist;
//091494: 			Object_num_close++;
//091494: 		} else {
//091494: 			int i, farthest_robot;
//091494: 			fix farthest_distance;
//091494: 			// Find the farthest robot in the list
//091494: 			farthest_robot = 0;
//091494: 			farthest_distance = Object_close_distance[0];
//091494: 			for (i=1; i<Object_num_close; i++ )	{
//091494: 				if ( Object_close_distance[i] > farthest_distance )	{
//091494: 					farthest_distance = Object_close_distance[i];
//091494: 					farthest_robot = i;
//091494: 				}
//091494: 			}
//091494: 			// If this object is closer to the viewer than
//091494: 			// the farthest in the list, replace the farthest with this object.
//091494: 			if ( farthest_distance > dist )	{
//091494: 				Object_close_ones[farthest_robot] = obj;
//091494: 				Object_close_distance[farthest_robot] = dist;
//091494: 			}
//091494: 		}
//091494: 	}
//091494: }

int	Player_fired_laser_this_frame=-1;



// -----------------------------------------------------------------------------
//this routine checks to see if an robot rendered near the middle of
//the screen, and if so and the player had fired, "warns" the robot
void set_robot_location_info(object *objp)
{
	if (Player_fired_laser_this_frame != -1) {
		g3s_point temp;

		g3_rotate_point(&temp,&objp->pos);

		if (temp.p3_codes & CC_BEHIND)		//robot behind the screen
			return;

		//the code below to check for object near the center of the screen
		//completely ignores z, which may not be good

		if ((abs(temp.p3_x) < F1_0*4) && (abs(temp.p3_y) < F1_0*4)) {
			objp->ctype.ai_info.danger_laser_num = Player_fired_laser_this_frame;
			objp->ctype.ai_info.danger_laser_signature = Objects[Player_fired_laser_this_frame].signature;
		}
	}


}

//	------------------------------------------------------------------------------------------------------------------
void create_small_fireball_on_object(object *objp, fix size_scale, int sound_flag)
{
	fix			size;
	vms_vector	pos, rand_vec;
	int			segnum;

	pos = objp->pos;
	make_random_vector(&rand_vec);

	vm_vec_scale(&rand_vec, objp->size/2);

	vm_vec_add2(&pos, &rand_vec);

	size = fixmul(size_scale, F1_0/2 + d_rand()*4/2);

	segnum = find_point_seg(&pos, objp->segnum);
	if (segnum != -1) {
		object *expl_obj;
		expl_obj = object_create_explosion(segnum, &pos, size, VCLIP_SMALL_EXPLOSION);
		if (!expl_obj)
			return;
		obj_attach(objp,expl_obj);
		if (d_rand() < 8192) {
			fix	vol = F1_0/2;
			if (objp->type == OBJ_ROBOT)
				vol *= 2;
			else if (sound_flag)
				digi_link_sound_to_object(SOUND_EXPLODING_WALL, objp-Objects, 0, vol);
		}
	}
}

//	------------------------------------------------------------------------------------------------------------------
void create_vclip_on_object(object *objp, fix size_scale, int vclip_num)
{
	fix			size;
	vms_vector	pos, rand_vec;
	int			segnum;

	pos = objp->pos;
	make_random_vector(&rand_vec);

	vm_vec_scale(&rand_vec, objp->size/2);

	vm_vec_add2(&pos, &rand_vec);

	size = fixmul(size_scale, F1_0 + d_rand()*4);

	segnum = find_point_seg(&pos, objp->segnum);
	if (segnum != -1) {
		object *expl_obj;
		expl_obj = object_create_explosion(segnum, &pos, size, vclip_num);
		if (!expl_obj)
			return;

		expl_obj->movement_type = MT_PHYSICS;
		expl_obj->mtype.phys_info.velocity.x = objp->mtype.phys_info.velocity.x/2;
		expl_obj->mtype.phys_info.velocity.y = objp->mtype.phys_info.velocity.y/2;
		expl_obj->mtype.phys_info.velocity.z = objp->mtype.phys_info.velocity.z/2;
	}
}

// -- mk, 02/05/95 -- #define	VCLIP_INVULNERABILITY_EFFECT	VCLIP_SMALL_EXPLOSION
// -- mk, 02/05/95 --
// -- mk, 02/05/95 -- // -----------------------------------------------------------------------------
// -- mk, 02/05/95 -- void do_player_invulnerability_effect(object *objp)
// -- mk, 02/05/95 -- {
// -- mk, 02/05/95 -- 	if (d_rand() < FrameTime*8) {
// -- mk, 02/05/95 -- 		create_vclip_on_object(objp, F1_0, VCLIP_INVULNERABILITY_EFFECT);
// -- mk, 02/05/95 -- 	}
// -- mk, 02/05/95 -- }

// -----------------------------------------------------------------------------
//	Render an object.  Calls one of several routines based on type
void render_object(object *obj)
{
	int	mld_save;

	if ( obj == Viewer ) return;		

	if ( obj->type==OBJ_NONE )	{
		#ifndef NDEBUG
		mprintf( (1, "ERROR!!!! Bogus obj %d in seg %d is rendering!\n", obj-Objects, obj->segnum ));
		Int3();
		#endif
		return;
	}

	mld_save = Max_linear_depth;
	Max_linear_depth = Max_linear_depth_objects;

	switch (obj->render_type) {

		case RT_NONE:	break;		//doesn't render, like the player

		case RT_POLYOBJ:

			draw_polygon_object(obj);

			//"warn" robot if being shot at
			if (obj->type == OBJ_ROBOT)
				set_robot_location_info(obj);

//JOHN SAID TO:			if ( (obj->type==OBJ_PLAYER) && ((keyd_pressed[KEY_W]) || (keyd_pressed[KEY_I])))
//JOHN SAID TO:				object_render_id(obj);

// -- mk, 02/05/95 -- 			if (obj->type == OBJ_PLAYER)
// -- mk, 02/05/95 -- 				if (Players[obj->id].flags & PLAYER_FLAGS_INVULNERABLE)
// -- mk, 02/05/95 -- 					do_player_invulnerability_effect(obj);

			break;

		case RT_MORPH:	draw_morph_object(obj); break;

		case RT_FIREBALL: draw_fireball(obj); break;

		case RT_WEAPON_VCLIP: draw_weapon_vclip(obj); break;

		case RT_HOSTAGE: draw_hostage(obj); break;

		case RT_POWERUP: draw_powerup(obj); break;

		case RT_LASER: Laser_render(obj); break;

		default: Error("Unknown render_type <%d>",obj->render_type);
 	}

	#ifdef NEWDEMO
	if ( obj->render_type != RT_NONE )
		if ( Newdemo_state == ND_STATE_RECORDING ) {
			if (!WasRecorded[obj-Objects]) {
				newdemo_record_render_object(obj);
				WasRecorded[obj-Objects]=1;
			}
		}
	#endif

	Max_linear_depth = mld_save;

}

//--unused-- void object_toggle_lock_targets()	{
//--unused-- 	Object_draw_lock_boxes ^= 1;
//--unused-- }

//091494: //draw target boxes for nearby robots
//091494: void object_render_targets()
//091494: {
//091494: 	g3s_point pt;
//091494: 	ubyte codes;
//091494: 	int i;
//091494: 	int radius,x,y;
//091494:
//091494: 	if (Object_draw_lock_boxes==0)
//091494: 		return;
//091494:
//091494: 	for (i=0; i<Object_num_close; i++ )	{
//091494: 			
//091494: 		codes = g3_rotate_point(&pt, &Object_close_ones[i]->pos );
//091494: 		if ( !(codes & CC_BEHIND) )	{
//091494: 			g3_project_point(&pt);
//091494: 			if (pt.p3_flags & PF_PROJECTED)	{
//091494: 				x = f2i(pt.p3_sx);
//091494: 				y = f2i(pt.p3_sy);
//091494: 				radius = f2i(fixdiv((grd_curcanv->cv_bitmap.bm_w*Object_close_ones[i]->size)/8,pt.z));
//091494: 				gr_setcolor( BM_XRGB(0,31,0) );
//091494: 				gr_box(x-radius,y-radius,x+radius,y+radius);
//091494: 			}
//091494: 		}
//091494: 	}
//091494: 	Object_num_close=0;
//091494: }
//--unused-- //draw target boxes for nearby robots
//--unused-- void object_render_id(object * obj)
//--unused-- {
//--unused-- 	g3s_point pt;
//--unused-- 	ubyte codes;
//--unused-- 	int x,y;
//--unused-- 	int w, h, aw;
//--unused-- 	char s[20], *s1;
//--unused--
//--unused-- 	s1 = network_get_player_name( obj-Objects );
//--unused--
//--unused-- 	if (s1)
//--unused-- 		sprintf( s, "%s", s1 );
//--unused-- 	else
//--unused-- 		sprintf( s, "<%d>", obj->id );
//--unused--
//--unused-- 	codes = g3_rotate_point(&pt, &obj->pos );
//--unused-- 	if ( !(codes & CC_BEHIND) )	{
//--unused-- 		g3_project_point(&pt);
//--unused-- 		if (pt.p3_flags & PF_PROJECTED)	{
//--unused-- 			gr_get_string_size( s, &w, &h, &aw );
//--unused-- 			x = f2i(pt.p3_sx) - w/2;
//--unused-- 			y = f2i(pt.p3_sy) - h/2;
//--unused-- 			if ( x>= 0 && y>=0 && (x+w)<=grd_curcanv->cv_bitmap.bm_w && (y+h)<grd_curcanv->cv_bitmap.bm_h )	{
//--unused-- 				gr_set_fontcolor( BM_XRGB(0,31,0), -1 );
//--unused-- 				gr_string( x, y, s );
//--unused-- 			}
//--unused-- 		}
//--unused-- 	}
//--unused-- }

void check_and_fix_matrix(vms_matrix *m);

#define vm_angvec_zero(v) (v)->p=(v)->b=(v)->h=0

void reset_player_object()
{
	int i;

	//Init physics

	vm_vec_zero(&ConsoleObject->mtype.phys_info.velocity);
	vm_vec_zero(&ConsoleObject->mtype.phys_info.thrust);
	vm_vec_zero(&ConsoleObject->mtype.phys_info.rotvel);
	vm_vec_zero(&ConsoleObject->mtype.phys_info.rotthrust);
	ConsoleObject->mtype.phys_info.brakes = ConsoleObject->mtype.phys_info.turnroll = 0;
	ConsoleObject->mtype.phys_info.mass = Player_ship->mass;
	ConsoleObject->mtype.phys_info.drag = Player_ship->drag;
	ConsoleObject->mtype.phys_info.flags |= PF_TURNROLL | PF_LEVELLING | PF_WIGGLE | PF_USES_THRUST;

	//Init render info

	ConsoleObject->render_type = RT_POLYOBJ;
	ConsoleObject->rtype.pobj_info.model_num = Player_ship->model_num;		//what model is this?
	ConsoleObject->rtype.pobj_info.subobj_flags = 0;		//zero the flags
	ConsoleObject->rtype.pobj_info.tmap_override = -1;		//no tmap override!

	for (i=0;i<MAX_SUBMODELS;i++)
		vm_angvec_zero(&ConsoleObject->rtype.pobj_info.anim_angles[i]);

	// Clear misc

	ConsoleObject->flags = 0;

}


//make object0 the player, setting all relevant fields
void init_player_object()
{
	ConsoleObject->type = OBJ_PLAYER;
	ConsoleObject->id = 0;					//no sub-types for player

	ConsoleObject->signature = 0;			//player has zero, others start at 1

	ConsoleObject->size = Polygon_models[Player_ship->model_num].rad;

	ConsoleObject->control_type = CT_SLEW;			//default is player slewing
	ConsoleObject->movement_type = MT_PHYSICS;		//change this sometime

	ConsoleObject->lifeleft = IMMORTAL_TIME;

	ConsoleObject->attached_obj = -1;

	reset_player_object();

}

//sets up the free list & init player & whatever else
void init_objects()
{
	int i;

	collide_init();

	for (i=0;i<MAX_OBJECTS;i++) {
		free_obj_list[i] = i;
		Objects[i].type = OBJ_NONE;
		Objects[i].segnum = -1;
	}

	for (i=0;i<MAX_SEGMENTS;i++)
		Segments[i].objects = -1;

	ConsoleObject = Viewer = &Objects[0];

	init_player_object();
	obj_link(ConsoleObject-Objects,0);	//put in the world in segment 0

	num_objects = 1;						//just the player
	Highest_object_index = 0;

	
}

//after calling init_object(), the network code has grabbed specific
//object slots without allocating them.  Go though the objects & build
//the free list, then set the apporpriate globals
void special_reset_objects(void)
{
	int i;

	num_objects=MAX_OBJECTS;

	Highest_object_index = 0;
	Assert(Objects[0].type != OBJ_NONE);		//0 should be used

	for (i=MAX_OBJECTS;i--;)
		if (Objects[i].type == OBJ_NONE)
			free_obj_list[--num_objects] = i;
		else
			if (i > Highest_object_index)
				Highest_object_index = i;
}

#ifndef NDEBUG
int is_object_in_seg( int segnum, int objn )
{
	int objnum, count = 0;

	for (objnum=Segments[segnum].objects;objnum!=-1;objnum=Objects[objnum].next)	{
		if ( count > MAX_OBJECTS ) 	{
			Int3();
			return count;
		}
		if ( objnum==objn ) count++;
	}
	 return count;
}

int search_all_segments_for_object( int objnum )
{
	int i;
	int count = 0;

	for (i=0; i<=Highest_segment_index; i++) {
		count += is_object_in_seg( i, objnum );
	}
	return count;
}

void johns_obj_unlink(int segnum, int objnum)
{
	object  *obj = &Objects[objnum];
	segment *seg = &Segments[segnum];

	Assert(objnum != -1);

	if (obj->prev == -1)
		seg->objects = obj->next;
	else
		Objects[obj->prev].next = obj->next;

	if (obj->next != -1) Objects[obj->next].prev = obj->prev;
}

void remove_incorrect_objects()
{
	int segnum, objnum, count;

	for (segnum=0; segnum <= Highest_segment_index; segnum++) {
		count = 0;
		for (objnum=Segments[segnum].objects;objnum!=-1;objnum=Objects[objnum].next)	{
			count++;
			#ifndef NDEBUG
			if ( count > MAX_OBJECTS )	{
				mprintf((1, "Object list in segment %d is circular.\n", segnum ));
				Int3();
			}
			#endif
			if (Objects[objnum].segnum != segnum )	{
				#ifndef NDEBUG
				mprintf((0, "Removing object %d from segment %d.\n", objnum, segnum ));
				Int3();
				#endif
				johns_obj_unlink(segnum,objnum);
			}
		}
	}
}

void remove_all_objects_but( int segnum, int objnum )
{
	int i;

	for (i=0; i<=Highest_segment_index; i++) {
		if (segnum != i )	{
			if (is_object_in_seg( i, objnum ))	{
				johns_obj_unlink( i, objnum );
			}
		}
	}
}

int check_duplicate_objects()
{
	int i, count=0;
	
	for (i=0;i<=Highest_object_index;i++) {
		if ( Objects[i].type != OBJ_NONE )	{
			count = search_all_segments_for_object( i );
			if ( count > 1 )	{
				#ifndef NDEBUG
				mprintf((1, "Object %d is in %d segments!\n", i, count ));
				Int3();
				#endif
				remove_all_objects_but( Objects[i].segnum,  i );
				return count;
			}
		}
	}
	return count;
}

void list_seg_objects( int segnum )
{
	int objnum, count = 0;

	for (objnum=Segments[segnum].objects;objnum!=-1;objnum=Objects[objnum].next)	{
		count++;
		if ( count > MAX_OBJECTS ) 	{
			Int3();
			return;
		}
	}
	return;

}
#endif

//link the object into the list for its segment
void obj_link(int objnum,int segnum)
{
	object *obj = &Objects[objnum];

	Assert(objnum != -1);

	Assert(obj->segnum == -1);

	Assert(segnum>=0 && segnum<=Highest_segment_index);

	obj->segnum = segnum;
	
	obj->next = Segments[segnum].objects;
	obj->prev = -1;

	Segments[segnum].objects = objnum;

	if (obj->next != -1) Objects[obj->next].prev = objnum;
	
	//list_seg_objects( segnum );
	//check_duplicate_objects();

	Assert(Objects[0].next != 0);
	if (Objects[0].next == 0)
		Objects[0].next = -1;

	Assert(Objects[0].prev != 0);
	if (Objects[0].prev == 0)
		Objects[0].prev = -1;
}

void obj_unlink(int objnum)
{
	object  *obj = &Objects[objnum];
	segment *seg = &Segments[obj->segnum];

	Assert(objnum != -1);

	if (obj->prev == -1)
		seg->objects = obj->next;
	else
		Objects[obj->prev].next = obj->next;

	if (obj->next != -1) Objects[obj->next].prev = obj->prev;

	obj->segnum = -1;

	Assert(Objects[0].next != 0);
	Assert(Objects[0].prev != 0);
}

int Object_next_signature = 1;	//player gets 0, others start at 1

int Debris_object_count=0;

int	Unused_object_slots;

//returns the number of a free object, updating Highest_object_index.
//Generally, obj_create() should be called to get an object, since it
//fills in important fields and does the linking.
//returns -1 if no free objects
int obj_allocate(void)
{
	int objnum;

	if ( num_objects >= MAX_OBJECTS-2 ) {
		int	num_freed;

		num_freed = free_object_slots(MAX_OBJECTS-10);
		mprintf((0, " *** Freed %i objects in frame %i\n", num_freed, FrameCount));
	}

	if ( num_objects >= MAX_OBJECTS ) {
		#ifndef NDEBUG
		mprintf((1, "Object creation failed - too many objects!\n" ));
		#endif
		return -1;
	}

	objnum = free_obj_list[num_objects++];

	if (objnum > Highest_object_index) {
		Highest_object_index = objnum;
		if (Highest_object_index > Highest_ever_object_index)
			Highest_ever_object_index = Highest_object_index;
	}

{
int	i;
Unused_object_slots=0;
for (i=0; i<=Highest_object_index; i++)
	if (Objects[i].type == OBJ_NONE)
		Unused_object_slots++;
}
	return objnum;
}

//frees up an object.  Generally, obj_delete() should be called to get
//rid of an object.  This function deallocates the object entry after
//the object has been unlinked
void obj_free(int objnum)
{
	free_obj_list[--num_objects] = objnum;
	Assert(num_objects >= 0);

	if (objnum == Highest_object_index)
		while (Objects[--Highest_object_index].type == OBJ_NONE);
}

//-----------------------------------------------------------------------------
//	Scan the object list, freeing down to num_used objects
//	Returns number of slots freed.
int free_object_slots(int num_used)
{
	int	i, olind;
	int	obj_list[MAX_OBJECTS];
	int	num_already_free, num_to_free, original_num_to_free;

	olind = 0;
	num_already_free = MAX_OBJECTS - Highest_object_index - 1;

	if (MAX_OBJECTS - num_already_free < num_used)
		return 0;

	for (i=0; i<=Highest_object_index; i++) {
		if (Objects[i].flags & OF_SHOULD_BE_DEAD) {
			num_already_free++;
			if (MAX_OBJECTS - num_already_free < num_used)
				return num_already_free;
		} else
			switch (Objects[i].type) {
				case OBJ_NONE:
					num_already_free++;
					if (MAX_OBJECTS - num_already_free < num_used)
						return 0;
					break;
				case OBJ_WALL:
				case OBJ_FLARE:
					Int3();		//	This is curious.  What is an object that is a wall?
					break;
				case OBJ_FIREBALL:
				case OBJ_WEAPON:
				case OBJ_DEBRIS:
					obj_list[olind++] = i;
					break;
				case OBJ_ROBOT:
				case OBJ_HOSTAGE:
				case OBJ_PLAYER:
				case OBJ_CNTRLCEN:
				case OBJ_CLUTTER:
				case OBJ_GHOST:
				case OBJ_LIGHT:
				case OBJ_CAMERA:
				case OBJ_POWERUP:
					break;
			}

	}

	num_to_free = MAX_OBJECTS - num_used - num_already_free;
	original_num_to_free = num_to_free;

	if (num_to_free > olind) {
		mprintf((1, "Warning: Asked to free %i objects, but can only free %i.\n", num_to_free, olind));
		num_to_free = olind;
	}

	for (i=0; i<num_to_free; i++)
		if (Objects[obj_list[i]].type == OBJ_DEBRIS) {
			num_to_free--;
			mprintf((0, "Freeing   DEBRIS object %3i\n", obj_list[i]));
			Objects[obj_list[i]].flags |= OF_SHOULD_BE_DEAD;
		}

	if (!num_to_free)
		return original_num_to_free;

	for (i=0; i<num_to_free; i++)
		if (Objects[obj_list[i]].type == OBJ_FIREBALL  &&  Objects[obj_list[i]].ctype.expl_info.delete_objnum==-1) {
			num_to_free--;
			mprintf((0, "Freeing FIREBALL object %3i\n", obj_list[i]));
			Objects[obj_list[i]].flags |= OF_SHOULD_BE_DEAD;
		}

	if (!num_to_free)
		return original_num_to_free;

	for (i=0; i<num_to_free; i++)
		if ((Objects[obj_list[i]].type == OBJ_WEAPON) && (Objects[obj_list[i]].id == FLARE_ID)) {
			num_to_free--;
			Objects[obj_list[i]].flags |= OF_SHOULD_BE_DEAD;
		}

	if (!num_to_free)
		return original_num_to_free;

	for (i=0; i<num_to_free; i++)
		if ((Objects[obj_list[i]].type == OBJ_WEAPON) && (Objects[obj_list[i]].id != FLARE_ID)) {
			num_to_free--;
			mprintf((0, "Freeing   WEAPON object %3i\n", obj_list[i]));
			Objects[obj_list[i]].flags |= OF_SHOULD_BE_DEAD;
		}

	return original_num_to_free - num_to_free;
}

//-----------------------------------------------------------------------------
//initialize a new object.  adds to the list for the given segment
//note that segnum is really just a suggestion, since this routine actually
//searches for the correct segment
//returns the object number
int obj_create(ubyte type,ubyte id,int segnum,vms_vector *pos,
				vms_matrix *orient,fix size,ubyte ctype,ubyte mtype,ubyte rtype)
{
	int objnum;
	object *obj;

	Assert(segnum <= Highest_segment_index);
	Assert (segnum >= 0);
	Assert(ctype <= CT_CNTRLCEN);

	if (type==OBJ_DEBRIS && Debris_object_count>=Max_debris_objects)
		return -1;

	if (get_seg_masks(pos,segnum,0).centermask!=0)
		if ((segnum=find_point_seg(pos,segnum))==-1) {
			#ifndef NDEBUG
			mprintf((0,"Bad segnum in obj_create (type=%d)\n",type));
			#endif
			return -1;		//don't create this object
		}

	// Find next free object
	objnum = obj_allocate();

	if (objnum == -1)		//no free objects
		return -1;

	Assert(Objects[objnum].type == OBJ_NONE);		//make sure unused

	obj = &Objects[objnum];

	Assert(obj->segnum == -1);

	// Zero out object structure to keep weird bugs from happening
	// in uninitialized fields.
	memset( obj, 0, sizeof(object) );

	obj->signature				= Object_next_signature++;
	obj->type 					= type;
	obj->id 						= id;
	obj->last_pos				= *pos;
	obj->pos 					= *pos;
	obj->size 					= size;
	obj->flags 					= 0;
	//@@if (orient != NULL)
	//@@	obj->orient 			= *orient;

	obj->orient 				= orient?*orient:vmd_identity_matrix;

	obj->control_type 		= ctype;
	obj->movement_type 		= mtype;
	obj->render_type 			= rtype;
	obj->contains_type		= -1;

	obj->lifeleft 				= IMMORTAL_TIME;		//assume immortal
	obj->attached_obj			= -1;

	if (obj->control_type == CT_POWERUP)
		obj->ctype.powerup_info.count = 1;

	// Init physics info for this object
	if (obj->movement_type == MT_PHYSICS) {

		vm_vec_zero(&obj->mtype.phys_info.velocity);
		vm_vec_zero(&obj->mtype.phys_info.thrust);
		vm_vec_zero(&obj->mtype.phys_info.rotvel);
		vm_vec_zero(&obj->mtype.phys_info.rotthrust);

		obj->mtype.phys_info.mass		= 0;
		obj->mtype.phys_info.drag 		= 0;
		obj->mtype.phys_info.brakes	= 0;
		obj->mtype.phys_info.turnroll	= 0;
		obj->mtype.phys_info.flags		= 0;
	}

	if (obj->render_type == RT_POLYOBJ)
		obj->rtype.pobj_info.tmap_override = -1;

	obj->shields 				= 20*F1_0;

	segnum = find_point_seg(pos,segnum);		//find correct segment

	Assert(segnum!=-1);

	obj->segnum = -1;					//set to zero by memset, above
	obj_link(objnum,segnum);

	//	Set (or not) persistent bit in phys_info.
	if (obj->type == OBJ_WEAPON) {
		Assert(obj->control_type == CT_WEAPON);
		obj->mtype.phys_info.flags |= (Weapon_info[obj->id].persistent*PF_PERSISTENT);
		obj->ctype.laser_info.creation_time = GameTime;
		obj->ctype.laser_info.last_hitobj = -1;
		obj->ctype.laser_info.multiplier = F1_0;
	}

	if (obj->control_type == CT_POWERUP)
		obj->ctype.powerup_info.creation_time = GameTime;

	if (obj->control_type == CT_EXPLOSION)
		obj->ctype.expl_info.next_attach = obj->ctype.expl_info.prev_attach = obj->ctype.expl_info.attach_parent = -1;

	#ifndef NDEBUG
	if (print_object_info)	
		mprintf( (0, "Created object %d of type %d\n", objnum, obj->type ));
	#endif

	if (obj->type == OBJ_DEBRIS)
		Debris_object_count++;

	return objnum;
}

#ifdef EDITOR
//create a copy of an object. returns new object number
int obj_create_copy(int objnum, vms_vector *new_pos, int newsegnum)
{
	int newobjnum;
	object *obj;

	// Find next free object
	newobjnum = obj_allocate();

	if (newobjnum == -1)
		return -1;

	obj = &Objects[newobjnum];

	*obj = Objects[objnum];

	obj->pos = obj->last_pos = *new_pos;

	obj->next = obj->prev = obj->segnum = -1;

	obj_link(newobjnum,newsegnum);

	obj->signature				= Object_next_signature++;

	//we probably should initialize sub-structures here

	return newobjnum;

}
#endif

extern void newdemo_record_guided_end();

//remove object from the world
void obj_delete(int objnum)
{
	int pnum;
	object *obj = &Objects[objnum];

	Assert(objnum != -1);
	Assert(objnum != 0 );
	Assert(obj->type != OBJ_NONE);
	Assert(obj != ConsoleObject);

	if (obj->type==OBJ_WEAPON && obj->id==GUIDEDMISS_ID) {
		pnum=Objects[obj->ctype.laser_info.parent_num].id;
		mprintf ((0,"Deleting a guided missile! Player %d\n\n",pnum));

		if (pnum!=Player_num) {
			mprintf ((0,"deleting missile that belongs to %d (%s)!\n",pnum,Players[pnum].callsign));
			Guided_missile[pnum]=NULL;
		}
		else if (Newdemo_state==ND_STATE_RECORDING)
			newdemo_record_guided_end();
		
	}

	if (obj == Viewer)		//deleting the viewer?
		Viewer = ConsoleObject;						//..make the player the viewer

	if (obj->flags & OF_ATTACHED)		//detach this from object
		obj_detach_one(obj);

	if (obj->attached_obj != -1)		//detach all objects from this
		obj_detach_all(obj);

	#if !defined(NDEBUG) && !defined(NMONO)
	if (print_object_info) mprintf( (0, "Deleting object %d of type %d\n", objnum, Objects[objnum].type ));
	#endif

	if (obj->type == OBJ_DEBRIS)
		Debris_object_count--;

	obj_unlink(objnum);

	Assert(Objects[0].next != 0);

	obj->type = OBJ_NONE;		//unused!
	obj->signature = -1;
	obj->segnum=-1;				// zero it!

	obj_free(objnum);
}

#define	DEATH_SEQUENCE_LENGTH			(F1_0*5)
#define	DEATH_SEQUENCE_EXPLODE_TIME	(F1_0*2)

int		Player_is_dead = 0;			//	If !0, then player is dead, but game continues so he can watch.
object	*Dead_player_camera = NULL;	//	Object index of object watching deader.
fix		Player_time_of_death;		//	Time at which player died.
object	*Viewer_save;
int		Player_flags_save;
int		Player_exploded = 0;
int		Death_sequence_aborted=0;
int		Player_eggs_dropped=0;
fix		Camera_to_player_dist_goal=F1_0*4;

ubyte		Control_type_save, Render_type_save;
extern	int Cockpit_mode_save;	//set while in letterbox or rear view, or -1

//	------------------------------------------------------------------------------------------------------------------
void dead_player_end(void)
{
	if (!Player_is_dead)
		return;

	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_restore_cockpit();

	Player_is_dead = 0;
	Player_exploded = 0;
	obj_delete(Dead_player_camera-Objects);
	Dead_player_camera = NULL;
	select_cockpit(Cockpit_mode_save);
	Cockpit_mode_save = -1;
	Viewer = Viewer_save;
	ConsoleObject->type = OBJ_PLAYER;
	ConsoleObject->flags = Player_flags_save;

	Assert((Control_type_save == CT_FLYING) || (Control_type_save == CT_SLEW));

	ConsoleObject->control_type = Control_type_save;
	ConsoleObject->render_type = Render_type_save;
	Players[Player_num].flags &= ~PLAYER_FLAGS_INVULNERABLE;
	Player_eggs_dropped = 0;

}

//	------------------------------------------------------------------------------------------------------------------
//	Camera is less than size of player away from
void set_camera_pos(vms_vector *camera_pos, object *objp)
{
	int	count = 0;
	fix	camera_player_dist;
	fix	far_scale;

	camera_player_dist = vm_vec_dist_quick(camera_pos, &objp->pos);

	if (camera_player_dist < Camera_to_player_dist_goal) { //2*objp->size) {
		//	Camera is too close to player object, so move it away.
		vms_vector	player_camera_vec;
		fvi_query	fq;
		fvi_info		hit_data;
		vms_vector	local_p1;

		vm_vec_sub(&player_camera_vec, camera_pos, &objp->pos);
		if ((player_camera_vec.x == 0) && (player_camera_vec.y == 0) && (player_camera_vec.z == 0))
			player_camera_vec.x += F1_0/16;

		hit_data.hit_type = HIT_WALL;
		far_scale = F1_0;

		while ((hit_data.hit_type != HIT_NONE) && (count++ < 6)) {
			vms_vector	closer_p1;
			vm_vec_normalize_quick(&player_camera_vec);
			vm_vec_scale(&player_camera_vec, Camera_to_player_dist_goal);

			fq.p0 = &objp->pos;
			vm_vec_add(&closer_p1, &objp->pos, &player_camera_vec);		//	This is the actual point we want to put the camera at.
			vm_vec_scale(&player_camera_vec, far_scale);						//	...but find a point 50% further away...
			vm_vec_add(&local_p1, &objp->pos, &player_camera_vec);		//	...so we won't have to do as many cuts.

			fq.p1 = &local_p1;
			fq.startseg = objp->segnum;
			fq.rad = 0;
			fq.thisobjnum = objp-Objects;
			fq.ignore_obj_list = NULL;
			fq.flags = 0;
			find_vector_intersection( &fq, &hit_data);

			if (hit_data.hit_type == HIT_NONE) {
				*camera_pos = closer_p1;
			} else {
				make_random_vector(&player_camera_vec);
				far_scale = 3*F1_0/2;
			}
		}
	}
}

extern void drop_player_eggs(object *objp);
extern int get_explosion_vclip(object *obj,int stage);
extern void multi_cap_objects();
extern int Proximity_dropped,Smartmines_dropped;

//	------------------------------------------------------------------------------------------------------------------
void dead_player_frame(void)
{
	fix	time_dead;
	vms_vector	fvec;

	if (Player_is_dead) {
		time_dead = GameTime - Player_time_of_death;

		//	If unable to create camera at time of death, create now.
		if (Dead_player_camera == Viewer_save) {
			int		objnum;
			object	*player = &Objects[Players[Player_num].objnum];

			objnum = obj_create(OBJ_CAMERA, 0, player->segnum, &player->pos, &player->orient, 0, CT_NONE, MT_NONE, RT_NONE);

			mprintf((0, "Creating new dead player camera.\n"));
			if (objnum != -1)
				Viewer = Dead_player_camera = &Objects[objnum];
			else {
				mprintf((1, "Can't create dead player camera.\n"));
				Int3();
			}
		}		

		ConsoleObject->mtype.phys_info.rotvel.x = max(0, DEATH_SEQUENCE_EXPLODE_TIME - time_dead)/4;
		ConsoleObject->mtype.phys_info.rotvel.y = max(0, DEATH_SEQUENCE_EXPLODE_TIME - time_dead)/2;
		ConsoleObject->mtype.phys_info.rotvel.z = max(0, DEATH_SEQUENCE_EXPLODE_TIME - time_dead)/3;

		Camera_to_player_dist_goal = min(time_dead*8, F1_0*20) + ConsoleObject->size;

		set_camera_pos(&Dead_player_camera->pos, ConsoleObject);

//		if (time_dead < DEATH_SEQUENCE_EXPLODE_TIME+F1_0*2) {
			vm_vec_sub(&fvec, &ConsoleObject->pos, &Dead_player_camera->pos);
			vm_vector_2_matrix(&Dead_player_camera->orient, &fvec, NULL, NULL);
//		} else {
//			Dead_player_camera->movement_type = MT_PHYSICS;
//			Dead_player_camera->mtype.phys_info.rotvel.y = F1_0/8;
//		}

		if (time_dead > DEATH_SEQUENCE_EXPLODE_TIME) {
			if (!Player_exploded) {

			if (Players[Player_num].hostages_on_board > 1)
				HUD_init_message(TXT_SHIP_DESTROYED_2, Players[Player_num].hostages_on_board);
			else if (Players[Player_num].hostages_on_board == 1)
				HUD_init_message(TXT_SHIP_DESTROYED_1);
			else
				HUD_init_message(TXT_SHIP_DESTROYED_0);

				#ifdef TACTILE
					if (TactileStick)
					 {
					  ClearForces();
					 }
				#endif
				Player_exploded = 1;
#ifdef NETWORK
				if (Game_mode & GM_NETWORK)
				 {
					AdjustMineSpawn ();
					multi_cap_objects();
				 }
#endif
				
				drop_player_eggs(ConsoleObject);
				Player_eggs_dropped = 1;
				#ifdef NETWORK
				if (Game_mode & GM_MULTI)
				{
					//multi_send_position(Players[Player_num].objnum);
					multi_send_player_explode(MULTI_PLAYER_EXPLODE);
				}
				#endif

				explode_badass_player(ConsoleObject);

				//is this next line needed, given the badass call above?
				explode_object(ConsoleObject,0);
				ConsoleObject->flags &= ~OF_SHOULD_BE_DEAD;		//don't really kill player
				ConsoleObject->render_type = RT_NONE;				//..just make him disappear
				ConsoleObject->type = OBJ_GHOST;						//..and kill intersections
				Players[Player_num].flags &= ~PLAYER_FLAGS_HEADLIGHT_ON;
			}
		} else {
			if (d_rand() < FrameTime*4) {
				#ifdef NETWORK
				if (Game_mode & GM_MULTI)
					multi_send_create_explosion(Player_num);
				#endif
				create_small_fireball_on_object(ConsoleObject, F1_0, 1);
			}
		}


		if (Death_sequence_aborted) { //time_dead > DEATH_SEQUENCE_LENGTH) {
			if (!Player_eggs_dropped) {
			
#ifdef NETWORK
				if (Game_mode & GM_NETWORK)
				 {
					AdjustMineSpawn();
					multi_cap_objects();
				 }
#endif
				
				drop_player_eggs(ConsoleObject);
				Player_eggs_dropped = 1;
				#ifdef NETWORK
				if (Game_mode & GM_MULTI)
				{
					//multi_send_position(Players[Player_num].objnum);
					multi_send_player_explode(MULTI_PLAYER_EXPLODE);
				}
				#endif
			}

			DoPlayerDead();		//kill_player();
		}
	}
}


void AdjustMineSpawn()
 {
   if (!(Game_mode & GM_NETWORK))
		return;  // No need for this function in any other mode

   if (!(Game_mode & GM_HOARD))
	  	Players[Player_num].secondary_ammo[PROXIMITY_INDEX]+=Proximity_dropped;
   Players[Player_num].secondary_ammo[SMART_MINE_INDEX]+=Smartmines_dropped;
	Proximity_dropped=0;
	Smartmines_dropped=0;
 }



int Killed_in_frame = -1;
short Killed_objnum = -1;
extern char Multi_killed_yourself;

//	------------------------------------------------------------------------------------------------------------------
void start_player_death_sequence(object *player)
{
	int	objnum;

	Assert(player == ConsoleObject);
	if ((Player_is_dead != 0) || (Dead_player_camera != NULL))
		return;

	//Assert(Player_is_dead == 0);
	//Assert(Dead_player_camera == NULL);

	reset_rear_view();

	if (!(Game_mode & GM_MULTI))
		HUD_clear_messages();

	Killed_in_frame = FrameCount;
	Killed_objnum = player-Objects;
	Death_sequence_aborted = 0;

	#ifdef NETWORK
	if (Game_mode & GM_MULTI)
	{
		multi_send_kill(Players[Player_num].objnum);

//		If Hoard, increase number of orbs by 1
//    Only if you haven't killed yourself
//		This prevents cheating

		if (Game_mode & GM_HOARD)
		 if (Players[Player_num].secondary_ammo[PROXIMITY_INDEX]<12)
			if (!Multi_killed_yourself)
				Players[Player_num].secondary_ammo[PROXIMITY_INDEX]++;
	
	}
	#endif
	
	PaletteRedAdd = 40;
	Player_is_dead = 1;
   #ifdef TACTILE
    if (TactileStick)
	  Buffeting (70);
	#endif

	//Players[Player_num].flags &= ~(PLAYER_FLAGS_AFTERBURNER);

	vm_vec_zero(&player->mtype.phys_info.rotthrust);
	vm_vec_zero(&player->mtype.phys_info.thrust);

	Player_time_of_death = GameTime;

	objnum = obj_create(OBJ_CAMERA, 0, player->segnum, &player->pos, &player->orient, 0, CT_NONE, MT_NONE, RT_NONE);
	Viewer_save = Viewer;
	if (objnum != -1)
		Viewer = Dead_player_camera = &Objects[objnum];
	else {
		mprintf((1, "Can't create dead player camera.\n"));
		Int3();
		Dead_player_camera = Viewer;
	}

	if (Cockpit_mode_save == -1)		//if not already saved
		Cockpit_mode_save = Cockpit_mode;
	select_cockpit(CM_LETTERBOX);
	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_letterbox();

	Player_flags_save = player->flags;
	Control_type_save = player->control_type;
	Render_type_save = player->render_type;

	player->flags &= ~OF_SHOULD_BE_DEAD;
//	Players[Player_num].flags |= PLAYER_FLAGS_INVULNERABLE;
	player->control_type = CT_NONE;
	player->shields = F1_0*1000;

	PALETTE_FLASH_SET(0,0,0);
}

//	------------------------------------------------------------------------------------------------------------------
void obj_delete_all_that_should_be_dead()
{
	int i;
	object *objp;
	int		local_dead_player_object=-1;

	// Move all objects
	objp = Objects;

	for (i=0;i<=Highest_object_index;i++) {
		if ((objp->type!=OBJ_NONE) && (objp->flags&OF_SHOULD_BE_DEAD) )	{
			Assert(!(objp->type==OBJ_FIREBALL && objp->ctype.expl_info.delete_time!=-1));
			if (objp->type==OBJ_PLAYER) {
				if ( objp->id == Player_num ) {
					if (local_dead_player_object == -1) {
						start_player_death_sequence(objp);
						local_dead_player_object = objp-Objects;
					} else
						Int3();	//	Contact Mike: Illegal, killed player twice in this frame!
									// Ok to continue, won't start death sequence again!
					// kill_player();
				}
			} else {					
				obj_delete(i);
			}
		}
		objp++;
	}
}

//when an object has moved into a new segment, this function unlinks it
//from its old segment, and links it into the new segment
void obj_relink(int objnum,int newsegnum)
{

	Assert((objnum >= 0) && (objnum <= Highest_object_index));
	Assert((newsegnum <= Highest_segment_index) && (newsegnum >= 0));

	obj_unlink(objnum);

	obj_link(objnum,newsegnum);
	
#ifndef NDEBUG
	if (get_seg_masks(&Objects[objnum].pos,Objects[objnum].segnum,0).centermask!=0)
		mprintf((1, "obj_relink violates seg masks.\n"));
#endif
}

//process a continuously-spinning object
void
spin_object(object *obj)
{
	vms_angvec rotangs;
	vms_matrix rotmat, new_pm;

	Assert(obj->movement_type == MT_SPINNING);

	rotangs.p = fixmul(obj->mtype.spin_rate.x,FrameTime);
	rotangs.h = fixmul(obj->mtype.spin_rate.y,FrameTime);
	rotangs.b = fixmul(obj->mtype.spin_rate.z,FrameTime);

	vm_angles_2_matrix(&rotmat,&rotangs);

	vm_matrix_x_matrix(&new_pm,&obj->orient,&rotmat);
	obj->orient = new_pm;

	check_and_fix_matrix(&obj->orient);
}

int Drop_afterburner_blob_flag;		//ugly hack
extern void multi_send_drop_blobs(char);
extern void fuelcen_check_for_goal (segment *);

//see if wall is volatile, and if so, cause damage to player
//returns true if player is in lava
int check_volatile_wall(object *obj,int segnum,int sidenum,vms_vector *hitpt);

//	Time at which this object last created afterburner blobs.
fix	Last_afterburner_time[MAX_OBJECTS];

//--------------------------------------------------------------------
//move an object for the current frame
void object_move_one( object * obj )
{

	#ifndef DEMO_ONLY

	int	previous_segment = obj->segnum;

	obj->last_pos = obj->pos;			// Save the current position

	if ((obj->type==OBJ_PLAYER) && (Player_num==obj->id))	{
		fix fuel, shields;
		
#ifdef NETWORK
      if (Game_mode & GM_CAPTURE)
			 fuelcen_check_for_goal (&Segments[obj->segnum]);
      if (Game_mode & GM_HOARD)
			 fuelcen_check_for_hoard_goal (&Segments[obj->segnum]);
#endif

		fuel=fuelcen_give_fuel( &Segments[obj->segnum], INITIAL_ENERGY-Players[Player_num].energy );
		if (fuel > 0 )	{
			Players[Player_num].energy += fuel;
		}

		shields = repaircen_give_shields( &Segments[obj->segnum], INITIAL_ENERGY-Players[Player_num].energy );
		if (shields > 0) {
			Players[Player_num].shields += shields;
		}
	}

	if (obj->lifeleft != IMMORTAL_TIME) {	//if not immortal...
		//	Ok, this is a big hack by MK.
		//	If you want an object to last for exactly one frame, then give it a lifeleft of ONE_FRAME_TIME.
		if (obj->lifeleft != ONE_FRAME_TIME)
			obj->lifeleft -= FrameTime;		//...inevitable countdown towards death
	}

	Drop_afterburner_blob_flag = 0;

	switch (obj->control_type) {

		case CT_NONE: break;

		case CT_FLYING:

			#if !defined(NDEBUG) && !defined(NMONO)
			if (print_object_info>1) mprintf( (0, "Moving player object #%d\n", obj-Objects ));
			#endif

			read_flying_controls( obj );

			break;

		case CT_REPAIRCEN: Int3();	// -- hey! these are no longer supported!! -- do_repair_sequence(obj); break;

		case CT_POWERUP: do_powerup_frame(obj); break;
	
		case CT_MORPH:			//morph implies AI
			do_morph_frame(obj);
			//NOTE: FALLS INTO AI HERE!!!!

		case CT_AI:
			//NOTE LINK TO CT_MORPH ABOVE!!!
			if (Game_suspended & SUSP_ROBOTS) return;
#if !defined(NDEBUG) && !defined(NMONO)
			if (print_object_info>1)
				mprintf( (0, "AI: Moving robot object #%d\n",obj-Objects ));
#endif
			do_ai_frame(obj);
			break;

		case CT_WEAPON:		Laser_do_weapon_sequence(obj); break;
		case CT_EXPLOSION:	do_explosion_sequence(obj); break;

		#ifndef RELEASE
		case CT_SLEW:
			if ( keyd_pressed[KEY_PAD5] ) slew_stop( obj );
			if ( keyd_pressed[KEY_NUMLOCK] ) 		{
				slew_reset_orient( obj );
				* (ubyte *) 0x417 &= ~0x20;		//kill numlock
			}
			slew_frame(0 );		// Does velocity addition for us.
			break;
		#endif


//		case CT_FLYTHROUGH:
//			do_flythrough(obj,0);			// HACK:do_flythrough should operate on an object!!!!
//			//check_object_seg(obj);
//			return;	// DON'T DO THE REST OF OBJECT STUFF SINCE THIS IS A SPECIAL CASE!!!
//			break;

		case CT_DEBRIS: do_debris_frame(obj); break;

		case CT_LIGHT: break;		//doesn't do anything

		case CT_REMOTE: break;		//movement is handled in com_process_input

		case CT_CNTRLCEN: do_controlcen_frame(obj); break;

		default:

#ifdef __DJGPP__
			Error("Unknown control type %d in object %li, sig/type/id = %i/%i/%i",obj->control_type, obj-Objects, obj->signature, obj->type, obj->id);
#else
			Error("Unknown control type %d in object %i, sig/type/id = %i/%i/%i",obj->control_type, obj-Objects, obj->signature, obj->type, obj->id);
#endif

			break;

	}

	if (obj->lifeleft < 0 ) {		// We died of old age
		obj->flags |= OF_SHOULD_BE_DEAD;
		if ( obj->type==OBJ_WEAPON && Weapon_info[obj->id].damage_radius )
			explode_badass_weapon(obj,&obj->pos);
		else if ( obj->type==OBJ_ROBOT)	//make robots explode
			explode_object(obj,0);
	}

	if (obj->type == OBJ_NONE || obj->flags&OF_SHOULD_BE_DEAD)
		return;			//object has been deleted

	switch (obj->movement_type) {

		case MT_NONE:			break;								//this doesn't move

		case MT_PHYSICS:		do_physics_sim(obj);	break;	//move by physics

		case MT_SPINNING:		spin_object(obj); break;

	}

	//	If player and moved to another segment, see if hit any triggers.
	// also check in player under a lavafall
	if (obj->type == OBJ_PLAYER && obj->movement_type==MT_PHYSICS)	{

		if (previous_segment != obj->segnum) {
			int	connect_side,i;
#ifdef NETWORK
			int	old_level = Current_level_num;
#endif
			for (i=0;i<n_phys_segs-1;i++) {
				connect_side = find_connect_side(&Segments[phys_seglist[i+1]], &Segments[phys_seglist[i]]);
				if (connect_side != -1)
					check_trigger(&Segments[phys_seglist[i]], connect_side, obj-Objects,0);
				#ifndef NDEBUG
				else {	// segments are not directly connected, so do binary subdivision until you find connected segments.
					mprintf((1, "UNCONNECTED SEGMENTS %d,%d\n",phys_seglist[i+1],phys_seglist[i]));
					// -- Unnecessary, MK, 09/04/95 -- Int3();
				}
				#endif

				//maybe we've gone on to the next level.  if so, bail!
#ifdef NETWORK
				if (Current_level_num != old_level)
					return;
#endif
			}
		}

		{
			int sidemask,under_lavafall=0;
			static int lavafall_hiss_playing[MAX_PLAYERS]={0};

			sidemask = get_seg_masks(&obj->pos,obj->segnum,obj->size).sidemask;
			if (sidemask) {
				int sidenum,bit,wall_num;
	
				for (sidenum=0,bit=1;sidenum<6;bit<<=1,sidenum++)
					if ((sidemask & bit) && ((wall_num=Segments[obj->segnum].sides[sidenum].wall_num)!=-1) && Walls[wall_num].type==WALL_ILLUSION) {
						int type;
						if ((type=check_volatile_wall(obj,obj->segnum,sidenum,&obj->pos))!=0) {
							int sound = (type==1)?SOUND_LAVAFALL_HISS:SOUND_SHIP_IN_WATERFALL;
							under_lavafall = 1;
							if (!lavafall_hiss_playing[obj->id]) {
								digi_link_sound_to_object3( sound, obj-Objects, 1, F1_0, i2f(256), -1, -1);
								lavafall_hiss_playing[obj->id] = 1;
							}
						}
					}
			}
	
			if (!under_lavafall && lavafall_hiss_playing[obj->id]) {
				digi_kill_sound_linked_to_object( obj-Objects);
				lavafall_hiss_playing[obj->id] = 0;
			}
		}
	}

	//see if guided missile has flown through exit trigger
	if (obj==Guided_missile[Player_num] && obj->signature==Guided_missile_sig[Player_num]) {
		if (previous_segment != obj->segnum) {
			int	connect_side;
			connect_side = find_connect_side(&Segments[obj->segnum], &Segments[previous_segment]);
			if (connect_side != -1) {
				int wall_num,trigger_num;
				wall_num = Segments[previous_segment].sides[connect_side].wall_num;
				if ( wall_num != -1 ) {
					trigger_num = Walls[wall_num].trigger;
					if (trigger_num != -1)
						if (Triggers[trigger_num].type == TT_EXIT)
							Guided_missile[Player_num]->lifeleft = 0;
				}
			}
		}
	}

	if (Drop_afterburner_blob_flag) {
		Assert(obj==ConsoleObject);
		drop_afterburner_blobs(obj, 2, i2f(5)/2, -1);	//	-1 means use default lifetime
#ifdef NETWORK
		if (Game_mode & GM_MULTI)
			multi_send_drop_blobs(Player_num);
#endif
		Drop_afterburner_blob_flag = 0;
	}

	if ((obj->type == OBJ_WEAPON) && (Weapon_info[obj->id].afterburner_size)) {
		int	objnum = obj-Objects;
		fix	vel = vm_vec_mag_quick(&obj->mtype.phys_info.velocity);
		fix	delay, lifetime;

		if (vel > F1_0*200)
			delay = F1_0/16;
		else if (vel > F1_0*40)
			delay = fixdiv(F1_0*13,vel);
		else
			delay = F1_0/4;

		lifetime = (delay * 3)/2;
		if (!(Game_mode & GM_MULTI)) {
			delay /= 2;
			lifetime *= 2;
		}

		if ((Last_afterburner_time[objnum] + delay < GameTime) || (Last_afterburner_time[objnum] > GameTime)) {
			drop_afterburner_blobs(obj, 1, i2f(Weapon_info[obj->id].afterburner_size)/16, lifetime);
			Last_afterburner_time[objnum] = GameTime;
		}
	}

	#else
		obj++;		//kill warning
	#endif		//DEMO_ONLY
}

int	Max_used_objects = MAX_OBJECTS - 20;

//--------------------------------------------------------------------
//move all objects for the current frame
void object_move_all()
{
	int i;
	object *objp;

// -- mprintf((0, "Frame %i: %i/%i objects used.\n", FrameCount, num_objects, MAX_OBJECTS));

//	check_duplicate_objects();
//	remove_incorrect_objects();

	if (Highest_object_index > Max_used_objects)
		free_object_slots(Max_used_objects);		//	Free all possible object slots.

	obj_delete_all_that_should_be_dead();

	if (Auto_leveling_on)
		ConsoleObject->mtype.phys_info.flags |= PF_LEVELLING;
	else
		ConsoleObject->mtype.phys_info.flags &= ~PF_LEVELLING;

	// Move all objects
	objp = Objects;

	#ifndef DEMO_ONLY
	for (i=0;i<=Highest_object_index;i++) {
		if ( (objp->type != OBJ_NONE) && (!(objp->flags&OF_SHOULD_BE_DEAD)) )	{
			object_move_one( objp );
		}
		objp++;
	}
	#else
		i=0;	//kill warning
	#endif

//	check_duplicate_objects();
//	remove_incorrect_objects();

}


//--unused-- // -----------------------------------------------------------
//--unused-- //	Moved here from eobject.c on 02/09/94 by MK.
//--unused-- int find_last_obj(int i)
//--unused-- {
//--unused-- 	for (i=MAX_OBJECTS;--i>=0;)
//--unused-- 		if (Objects[i].type != OBJ_NONE) break;
//--unused--
//--unused-- 	return i;
//--unused--
//--unused-- }


//make object array non-sparse
void compress_objects(void)
{
	int start_i;	//,last_i;

	//last_i = find_last_obj(MAX_OBJECTS);

	//	Note: It's proper to do < (rather than <=) Highest_object_index here because we
	//	are just removing gaps, and the last object can't be a gap.
	for (start_i=0;start_i<Highest_object_index;start_i++)

		if (Objects[start_i].type == OBJ_NONE) {

			int	segnum_copy;

			segnum_copy = Objects[Highest_object_index].segnum;

			obj_unlink(Highest_object_index);

			Objects[start_i] = Objects[Highest_object_index];

			#ifdef EDITOR
			if (Cur_object_index == Highest_object_index)
				Cur_object_index = start_i;
			#endif

			Objects[Highest_object_index].type = OBJ_NONE;

			obj_link(start_i,segnum_copy);

			while (Objects[--Highest_object_index].type == OBJ_NONE);

			//last_i = find_last_obj(last_i);
			
		}

	reset_objects(num_objects);

}

//called after load.  Takes number of objects,  and objects should be
//compressed.  resets free list, marks unused objects as unused
void reset_objects(int n_objs)
{
	int i;

	num_objects = n_objs;

	Assert(num_objects>0);

	for (i=num_objects;i<MAX_OBJECTS;i++) {
		free_obj_list[i] = i;
		Objects[i].type = OBJ_NONE;
		Objects[i].segnum = -1;
	}

	Highest_object_index = num_objects-1;

	Debris_object_count = 0;
}

//Tries to find a segment for an object, using find_point_seg()
int find_object_seg(object * obj )
{
	return find_point_seg(&obj->pos,obj->segnum);
}


//If an object is in a segment, set its segnum field and make sure it's
//properly linked.  If not in any segment, returns 0, else 1.
//callers should generally use find_vector_intersection()
int update_object_seg(object * obj )
{
	int newseg;

	newseg = find_object_seg(obj);

	if (newseg == -1)
		return 0;

	if ( newseg != obj->segnum )
		obj_relink(obj-Objects, newseg );

	return 1;
}


//go through all objects and make sure they have the correct segment numbers
void
fix_object_segs()
{
	int i;

	for (i=0;i<=Highest_object_index;i++)
		if (Objects[i].type != OBJ_NONE)
			if (update_object_seg(&Objects[i]) == 0) {
				mprintf((1,"Cannot find segment for object %d in fix_object_segs()\n"));
				Int3();
				compute_segment_center(&Objects[i].pos,&Segments[Objects[i].segnum]);
			}
}


//--unused-- void object_use_new_object_list( object * new_list )
//--unused-- {
//--unused-- 	int i, segnum;
//--unused-- 	object *obj;
//--unused--
//--unused-- 	// First, unlink all the old objects for the segments array
//--unused-- 	for (segnum=0; segnum <= Highest_segment_index; segnum++) {
//--unused-- 		Segments[segnum].objects = -1;
//--unused-- 	}
//--unused-- 	// Then, erase all the objects
//--unused-- 	reset_objects(1);
//--unused--
//--unused-- 	// Fill in the object array
//--unused-- 	memcpy( Objects, new_list, sizeof(object)*MAX_OBJECTS );
//--unused--
//--unused-- 	Highest_object_index=-1;
//--unused--
//--unused-- 	// Relink 'em
//--unused-- 	for (i=0; i<MAX_OBJECTS; i++ )	{
//--unused-- 		obj = &Objects[i];
//--unused-- 		if ( obj->type != OBJ_NONE )	{
//--unused-- 			num_objects++;
//--unused-- 			Highest_object_index = i;
//--unused-- 			segnum = obj->segnum;
//--unused-- 			obj->next = obj->prev = obj->segnum = -1;
//--unused-- 			obj_link(i,segnum);
//--unused-- 		} else {
//--unused-- 			obj->next = obj->prev = obj->segnum = -1;
//--unused-- 		}
//--unused-- 	}
//--unused-- 	
//--unused-- }

//delete objects, such as weapons & explosions, that shouldn't stay between levels
//	Changed by MK on 10/15/94, don't remove proximity bombs.
//if clear_all is set, clear even proximity bombs
void clear_transient_objects(int clear_all)
{
	int objnum;
	object *obj;

	for (objnum=0,obj=&Objects[0];objnum<=Highest_object_index;objnum++,obj++)
		if (((obj->type == OBJ_WEAPON) && !(Weapon_info[obj->id].flags&WIF_PLACABLE) && (clear_all || ((obj->id != PROXIMITY_ID) && (obj->id != SUPERPROX_ID)))) ||
			 obj->type == OBJ_FIREBALL ||
			 obj->type == OBJ_DEBRIS ||
			 obj->type == OBJ_DEBRIS ||
			 (obj->type!=OBJ_NONE && obj->flags & OF_EXPLODING)) {

			#ifndef NDEBUG
			if (Objects[objnum].lifeleft > i2f(2))
				mprintf((0,"Note: Clearing object %d (type=%d, id=%d) with lifeleft=%x\n",objnum,Objects[objnum].type,Objects[objnum].id,Objects[objnum].lifeleft));
			#endif
			obj_delete(objnum);
		}
		#ifndef NDEBUG
		 else if (Objects[objnum].type!=OBJ_NONE && Objects[objnum].lifeleft < i2f(2))
			mprintf((0,"Note: NOT clearing object %d (type=%d, id=%d) with lifeleft=%x\n",objnum,Objects[objnum].type,Objects[objnum].id,Objects[objnum].lifeleft));
		#endif
}

//attaches an object, such as a fireball, to another object, such as a robot
void obj_attach(object *parent,object *sub)
{
	Assert(sub->type == OBJ_FIREBALL);
	Assert(sub->control_type == CT_EXPLOSION);

	Assert(sub->ctype.expl_info.next_attach==-1);
	Assert(sub->ctype.expl_info.prev_attach==-1);

	Assert(parent->attached_obj==-1 || Objects[parent->attached_obj].ctype.expl_info.prev_attach==-1);

	sub->ctype.expl_info.next_attach = parent->attached_obj;

	if (sub->ctype.expl_info.next_attach != -1)
		Objects[sub->ctype.expl_info.next_attach].ctype.expl_info.prev_attach = sub-Objects;

	parent->attached_obj = sub-Objects;

	sub->ctype.expl_info.attach_parent = parent-Objects;
	sub->flags |= OF_ATTACHED;

	Assert(sub->ctype.expl_info.next_attach != sub-Objects);
	Assert(sub->ctype.expl_info.prev_attach != sub-Objects);
}

//dettaches one object
void obj_detach_one(object *sub)
{
	Assert(sub->flags & OF_ATTACHED);
	Assert(sub->ctype.expl_info.attach_parent != -1);

	if ((Objects[sub->ctype.expl_info.attach_parent].type == OBJ_NONE) || (Objects[sub->ctype.expl_info.attach_parent].attached_obj == -1))
	{
		sub->flags &= ~OF_ATTACHED;
		return;
	}

	if (sub->ctype.expl_info.next_attach != -1) {
		Assert(Objects[sub->ctype.expl_info.next_attach].ctype.expl_info.prev_attach=sub-Objects);
		Objects[sub->ctype.expl_info.next_attach].ctype.expl_info.prev_attach = sub->ctype.expl_info.prev_attach;
	}

	if (sub->ctype.expl_info.prev_attach != -1) {
		Assert(Objects[sub->ctype.expl_info.prev_attach].ctype.expl_info.next_attach=sub-Objects);
		Objects[sub->ctype.expl_info.prev_attach].ctype.expl_info.next_attach = sub->ctype.expl_info.next_attach;
	}
	else {
		Assert(Objects[sub->ctype.expl_info.attach_parent].attached_obj=sub-Objects);
		Objects[sub->ctype.expl_info.attach_parent].attached_obj = sub->ctype.expl_info.next_attach;
	}

	sub->ctype.expl_info.next_attach = sub->ctype.expl_info.prev_attach = -1;
	sub->flags &= ~OF_ATTACHED;

}

//dettaches all objects from this object
void obj_detach_all(object *parent)
{
	while (parent->attached_obj != -1)
		obj_detach_one(&Objects[parent->attached_obj]);
}

//creates a marker object in the world.  returns the object number
int drop_marker_object(vms_vector *pos,int segnum,vms_matrix *orient, int marker_num)
{
	int objnum;

	Assert(Marker_model_num != -1);

	objnum = obj_create(OBJ_MARKER, marker_num, segnum, pos, orient, Polygon_models[Marker_model_num].rad, CT_NONE, MT_NONE, RT_POLYOBJ);

	if (objnum >= 0) {
		object *obj = &Objects[objnum];

		obj->rtype.pobj_info.model_num = Marker_model_num;

		vm_vec_copy_scale(&obj->mtype.spin_rate,&obj->orient.uvec,F1_0/2);

		//	MK, 10/16/95: Using lifeleft to make it flash, thus able to trim lightlevel from all objects.
		obj->lifeleft = IMMORTAL_TIME - 1;
	}

	return objnum;	
}

extern int Ai_last_missile_camera;

//	*viewer is a viewer, probably a missile.
//	wake up all robots that were rendered last frame subject to some constraints.
void wake_up_rendered_objects(object *viewer, int window_num)
{
	int	i;

	//	Make sure that we are processing current data.
	if (FrameCount != Window_rendered_data[window_num].frame) {
		mprintf((1, "Warning: Called wake_up_rendered_objects with a bogus window.\n"));
		return;
	}

	Ai_last_missile_camera = viewer-Objects;

	for (i=0; i<Window_rendered_data[window_num].num_objects; i++) {
		int	objnum;
		object *objp;
		int	fcval = FrameCount & 3;

		objnum = Window_rendered_data[window_num].rendered_objects[i];
		if ((objnum & 3) == fcval) {
			objp = &Objects[objnum];
	
			if (objp->type == OBJ_ROBOT) {
				if (vm_vec_dist_quick(&viewer->pos, &objp->pos) < F1_0*100) {
					ai_local		*ailp = &Ai_local_info[objnum];
					if (ailp->player_awareness_type == 0) {
						objp->ctype.ai_info.SUB_FLAGS |= SUB_FLAGS_CAMERA_AWAKE;
						ailp->player_awareness_type = PA_WEAPON_ROBOT_COLLISION;
						ailp->player_awareness_time = F1_0*3;
						ailp->previous_visibility = 2;
					}
				}
			}
		}
	}
}






