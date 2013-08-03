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
 * Functions to save/restore game state.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "pstypes.h"
#include "inferno.h"
#include "segment.h"
#include "textures.h"
#include "wall.h"
#include "object.h"
#include "gamemine.h"
#include "dxxerror.h"
#include "gamefont.h"
#include "gameseg.h"
#include "switch.h"
#include "game.h"
#include "newmenu.h"
#include "fuelcen.h"
#include "hash.h"
#include "key.h"
#include "piggy.h"
#include "player.h"
#include "cntrlcen.h"
#include "morph.h"
#include "weapon.h"
#include "render.h"
#include "gameseq.h"
#include "gauges.h"
#include "newdemo.h"
#include "automap.h"
#include "piggy.h"
#include "paging.h"
#include "titles.h"
#include "text.h"
#include "mission.h"
#include "pcx.h"
#include "u_mem.h"
#include "args.h"
#include "ai.h"
#include "fireball.h"
#include "controls.h"
#include "laser.h"
#include "state.h"
#include "multi.h"
#include "gr.h"
#ifdef OGL
#include "ogl_init.h"
#endif
#include "physfsx.h"

#define STATE_VERSION 22
#define STATE_COMPATIBLE_VERSION 20
// 0 - Put DGSS (Descent Game State Save) id at tof.
// 1 - Added Difficulty level save
// 2 - Added cheats.enabled flag
// 3 - Added between levels save.
// 4 - Added mission support
// 5 - Mike changed ai and object structure.
// 6 - Added buggin' cheat save
// 7 - Added other cheat saves and game_id.
// 8 - Added AI stuff for escort and thief.
// 9 - Save palette with screen shot
// 12- Saved last_was_super array
// 13- Saved palette flash stuff
// 14- Save cloaking wall stuff
// 15- Save additional ai info
// 16- Save Light_subtracted
// 17- New marker save
// 18- Took out saving of old cheat status
// 19- Saved cheats.enabled flag
// 20- First_secret_visit
// 22- Omega_charge

#define NUM_SAVES 10
#define THUMBNAIL_W 100
#define THUMBNAIL_H 50
#define DESC_LENGTH 20

extern void apply_all_changed_light(void);

extern int Do_appearance_effect;

extern int Physics_cheat_flag;

int state_save_all_sub(char *filename, char *desc);
int state_restore_all_sub(char *filename, int secret_restore);

extern int First_secret_visit;

int sc_last_item= 0;

char dgss_id[4] = "DGSS";

uint state_game_id;

// Following functions convert object to object_rw and back to be written to/read from Savegames. Mostly object differs to object_rw in terms of timer values (fix/fix64). as we reset GameTime64 for writing so it can fit into fix it's not necessary to increment savegame version. But if we once store something else into object which might be useful after restoring, it might be handy to increment Savegame version and actually store these new infos.
// turn object to object_rw to be saved to Savegame.
void state_object_to_object_rw(object *obj, object_rw *obj_rw)
{
	obj_rw->signature     = obj->signature;
	obj_rw->type          = obj->type;
	obj_rw->id            = obj->id;
	obj_rw->next          = obj->next;
	obj_rw->prev          = obj->prev;
	obj_rw->control_type  = obj->control_type;
	obj_rw->movement_type = obj->movement_type;
	obj_rw->render_type   = obj->render_type;
	obj_rw->flags         = obj->flags;
	obj_rw->segnum        = obj->segnum;
	obj_rw->attached_obj  = obj->attached_obj;
	obj_rw->pos.x         = obj->pos.x;
	obj_rw->pos.y         = obj->pos.y;
	obj_rw->pos.z         = obj->pos.z;
	obj_rw->orient.rvec.x = obj->orient.rvec.x;
	obj_rw->orient.rvec.y = obj->orient.rvec.y;
	obj_rw->orient.rvec.z = obj->orient.rvec.z;
	obj_rw->orient.fvec.x = obj->orient.fvec.x;
	obj_rw->orient.fvec.y = obj->orient.fvec.y;
	obj_rw->orient.fvec.z = obj->orient.fvec.z;
	obj_rw->orient.uvec.x = obj->orient.uvec.x;
	obj_rw->orient.uvec.y = obj->orient.uvec.y;
	obj_rw->orient.uvec.z = obj->orient.uvec.z;
	obj_rw->size          = obj->size;
	obj_rw->shields       = obj->shields;
	obj_rw->last_pos.x    = obj->last_pos.x;
	obj_rw->last_pos.y    = obj->last_pos.y;
	obj_rw->last_pos.z    = obj->last_pos.z;
	obj_rw->contains_type = obj->contains_type;
	obj_rw->contains_id   = obj->contains_id;
	obj_rw->contains_count= obj->contains_count;
	obj_rw->matcen_creator= obj->matcen_creator;
	obj_rw->lifeleft      = obj->lifeleft;
	
	switch (obj_rw->movement_type)
	{
		case MT_PHYSICS:
			obj_rw->mtype.phys_info.velocity.x  = obj->mtype.phys_info.velocity.x;
			obj_rw->mtype.phys_info.velocity.y  = obj->mtype.phys_info.velocity.y;
			obj_rw->mtype.phys_info.velocity.z  = obj->mtype.phys_info.velocity.z;
			obj_rw->mtype.phys_info.thrust.x    = obj->mtype.phys_info.thrust.x;
			obj_rw->mtype.phys_info.thrust.y    = obj->mtype.phys_info.thrust.y;
			obj_rw->mtype.phys_info.thrust.z    = obj->mtype.phys_info.thrust.z;
			obj_rw->mtype.phys_info.mass        = obj->mtype.phys_info.mass;
			obj_rw->mtype.phys_info.drag        = obj->mtype.phys_info.drag;
			obj_rw->mtype.phys_info.brakes      = obj->mtype.phys_info.brakes;
			obj_rw->mtype.phys_info.rotvel.x    = obj->mtype.phys_info.rotvel.x;
			obj_rw->mtype.phys_info.rotvel.y    = obj->mtype.phys_info.rotvel.y;
			obj_rw->mtype.phys_info.rotvel.z    = obj->mtype.phys_info.rotvel.z;
			obj_rw->mtype.phys_info.rotthrust.x = obj->mtype.phys_info.rotthrust.x;
			obj_rw->mtype.phys_info.rotthrust.y = obj->mtype.phys_info.rotthrust.y;
			obj_rw->mtype.phys_info.rotthrust.z = obj->mtype.phys_info.rotthrust.z;
			obj_rw->mtype.phys_info.turnroll    = obj->mtype.phys_info.turnroll;
			obj_rw->mtype.phys_info.flags       = obj->mtype.phys_info.flags;
			break;
			
		case MT_SPINNING:
			obj_rw->mtype.spin_rate.x = obj->mtype.spin_rate.x;
			obj_rw->mtype.spin_rate.y = obj->mtype.spin_rate.y;
			obj_rw->mtype.spin_rate.z = obj->mtype.spin_rate.z;
			break;
	}
	
	switch (obj_rw->control_type)
	{
		case CT_WEAPON:
			obj_rw->ctype.laser_info.parent_type      = obj->ctype.laser_info.parent_type;
			obj_rw->ctype.laser_info.parent_num       = obj->ctype.laser_info.parent_num;
			obj_rw->ctype.laser_info.parent_signature = obj->ctype.laser_info.parent_signature;
			if (obj->ctype.laser_info.creation_time - GameTime64 < F1_0*(-18000))
				obj_rw->ctype.laser_info.creation_time = F1_0*(-18000);
			else
				obj_rw->ctype.laser_info.creation_time = obj->ctype.laser_info.creation_time - GameTime64;
			obj_rw->ctype.laser_info.last_hitobj      = obj->ctype.laser_info.last_hitobj;
			obj_rw->ctype.laser_info.track_goal       = obj->ctype.laser_info.track_goal;
			obj_rw->ctype.laser_info.multiplier       = obj->ctype.laser_info.multiplier;
			break;
			
		case CT_EXPLOSION:
			obj_rw->ctype.expl_info.spawn_time    = obj->ctype.expl_info.spawn_time;
			obj_rw->ctype.expl_info.delete_time   = obj->ctype.expl_info.delete_time;
			obj_rw->ctype.expl_info.delete_objnum = obj->ctype.expl_info.delete_objnum;
			obj_rw->ctype.expl_info.attach_parent = obj->ctype.expl_info.attach_parent;
			obj_rw->ctype.expl_info.prev_attach   = obj->ctype.expl_info.prev_attach;
			obj_rw->ctype.expl_info.next_attach   = obj->ctype.expl_info.next_attach;
			break;
			
		case CT_AI:
		{
			int i;
			obj_rw->ctype.ai_info.behavior               = obj->ctype.ai_info.behavior; 
			for (i = 0; i < MAX_AI_FLAGS; i++)
				obj_rw->ctype.ai_info.flags[i]       = obj->ctype.ai_info.flags[i]; 
			obj_rw->ctype.ai_info.hide_segment           = obj->ctype.ai_info.hide_segment;
			obj_rw->ctype.ai_info.hide_index             = obj->ctype.ai_info.hide_index;
			obj_rw->ctype.ai_info.path_length            = obj->ctype.ai_info.path_length;
			obj_rw->ctype.ai_info.cur_path_index         = obj->ctype.ai_info.cur_path_index;
			obj_rw->ctype.ai_info.dying_sound_playing    = obj->ctype.ai_info.dying_sound_playing;
			obj_rw->ctype.ai_info.danger_laser_num       = obj->ctype.ai_info.danger_laser_num;
			obj_rw->ctype.ai_info.danger_laser_signature = obj->ctype.ai_info.danger_laser_signature;
			if (obj->ctype.ai_info.dying_start_time == 0) // if bot not dead, anything but 0 will kill it
				obj_rw->ctype.ai_info.dying_start_time = 0;
			else
				obj_rw->ctype.ai_info.dying_start_time = obj->ctype.ai_info.dying_start_time - GameTime64;
			break;
		}
			
		case CT_LIGHT:
			obj_rw->ctype.light_info.intensity = obj->ctype.light_info.intensity;
			break;
			
		case CT_POWERUP:
			obj_rw->ctype.powerup_info.count         = obj->ctype.powerup_info.count;
			if (obj->ctype.powerup_info.creation_time - GameTime64 < F1_0*(-18000))
				obj_rw->ctype.powerup_info.creation_time = F1_0*(-18000);
			else
				obj_rw->ctype.powerup_info.creation_time = obj->ctype.powerup_info.creation_time - GameTime64;
			obj_rw->ctype.powerup_info.flags         = obj->ctype.powerup_info.flags;
			break;
	}
	
	switch (obj_rw->render_type)
	{
		case RT_MORPH:
		case RT_POLYOBJ:
		case RT_NONE: // HACK below
		{
			int i;
			if (obj->render_type == RT_NONE && obj->type != OBJ_GHOST) // HACK: when a player is dead or not connected yet, clients still expect to get polyobj data - even if render_type == RT_NONE at this time. Here it's not important, but it might be for Multiplayer Savegames.
				break;
			obj_rw->rtype.pobj_info.model_num                = obj->rtype.pobj_info.model_num;
			for (i=0;i<MAX_SUBMODELS;i++)
			{
				obj_rw->rtype.pobj_info.anim_angles[i].p = obj->rtype.pobj_info.anim_angles[i].p;
				obj_rw->rtype.pobj_info.anim_angles[i].b = obj->rtype.pobj_info.anim_angles[i].b;
				obj_rw->rtype.pobj_info.anim_angles[i].h = obj->rtype.pobj_info.anim_angles[i].h;
			}
			obj_rw->rtype.pobj_info.subobj_flags             = obj->rtype.pobj_info.subobj_flags;
			obj_rw->rtype.pobj_info.tmap_override            = obj->rtype.pobj_info.tmap_override;
			obj_rw->rtype.pobj_info.alt_textures             = obj->rtype.pobj_info.alt_textures;
			break;
		}
			
		case RT_WEAPON_VCLIP:
		case RT_HOSTAGE:
		case RT_POWERUP:
		case RT_FIREBALL:
			obj_rw->rtype.vclip_info.vclip_num = obj->rtype.vclip_info.vclip_num;
			obj_rw->rtype.vclip_info.frametime = obj->rtype.vclip_info.frametime;
			obj_rw->rtype.vclip_info.framenum  = obj->rtype.vclip_info.framenum;
			break;
			
		case RT_LASER:
			break;
			
	}
}

// turn object_rw to object after reading from Savegame
void state_object_rw_to_object(object_rw *obj_rw, object *obj)
{
	obj->signature     = obj_rw->signature;
	obj->type          = obj_rw->type;
	obj->id            = obj_rw->id;
	obj->next          = obj_rw->next;
	obj->prev          = obj_rw->prev;
	obj->control_type  = obj_rw->control_type;
	obj->movement_type = obj_rw->movement_type;
	obj->render_type   = obj_rw->render_type;
	obj->flags         = obj_rw->flags;
	obj->segnum        = obj_rw->segnum;
	obj->attached_obj  = obj_rw->attached_obj;
	obj->pos.x         = obj_rw->pos.x;
	obj->pos.y         = obj_rw->pos.y;
	obj->pos.z         = obj_rw->pos.z;
	obj->orient.rvec.x = obj_rw->orient.rvec.x;
	obj->orient.rvec.y = obj_rw->orient.rvec.y;
	obj->orient.rvec.z = obj_rw->orient.rvec.z;
	obj->orient.fvec.x = obj_rw->orient.fvec.x;
	obj->orient.fvec.y = obj_rw->orient.fvec.y;
	obj->orient.fvec.z = obj_rw->orient.fvec.z;
	obj->orient.uvec.x = obj_rw->orient.uvec.x;
	obj->orient.uvec.y = obj_rw->orient.uvec.y;
	obj->orient.uvec.z = obj_rw->orient.uvec.z;
	obj->size          = obj_rw->size;
	obj->shields       = obj_rw->shields;
	obj->last_pos.x    = obj_rw->last_pos.x;
	obj->last_pos.y    = obj_rw->last_pos.y;
	obj->last_pos.z    = obj_rw->last_pos.z;
	obj->contains_type = obj_rw->contains_type;
	obj->contains_id   = obj_rw->contains_id;
	obj->contains_count= obj_rw->contains_count;
	obj->matcen_creator= obj_rw->matcen_creator;
	obj->lifeleft      = obj_rw->lifeleft;
	
	switch (obj->movement_type)
	{
		case MT_PHYSICS:
			obj->mtype.phys_info.velocity.x  = obj_rw->mtype.phys_info.velocity.x;
			obj->mtype.phys_info.velocity.y  = obj_rw->mtype.phys_info.velocity.y;
			obj->mtype.phys_info.velocity.z  = obj_rw->mtype.phys_info.velocity.z;
			obj->mtype.phys_info.thrust.x    = obj_rw->mtype.phys_info.thrust.x;
			obj->mtype.phys_info.thrust.y    = obj_rw->mtype.phys_info.thrust.y;
			obj->mtype.phys_info.thrust.z    = obj_rw->mtype.phys_info.thrust.z;
			obj->mtype.phys_info.mass        = obj_rw->mtype.phys_info.mass;
			obj->mtype.phys_info.drag        = obj_rw->mtype.phys_info.drag;
			obj->mtype.phys_info.brakes      = obj_rw->mtype.phys_info.brakes;
			obj->mtype.phys_info.rotvel.x    = obj_rw->mtype.phys_info.rotvel.x;
			obj->mtype.phys_info.rotvel.y    = obj_rw->mtype.phys_info.rotvel.y;
			obj->mtype.phys_info.rotvel.z    = obj_rw->mtype.phys_info.rotvel.z;
			obj->mtype.phys_info.rotthrust.x = obj_rw->mtype.phys_info.rotthrust.x;
			obj->mtype.phys_info.rotthrust.y = obj_rw->mtype.phys_info.rotthrust.y;
			obj->mtype.phys_info.rotthrust.z = obj_rw->mtype.phys_info.rotthrust.z;
			obj->mtype.phys_info.turnroll    = obj_rw->mtype.phys_info.turnroll;
			obj->mtype.phys_info.flags       = obj_rw->mtype.phys_info.flags;
			break;
			
		case MT_SPINNING:
			obj->mtype.spin_rate.x = obj_rw->mtype.spin_rate.x;
			obj->mtype.spin_rate.y = obj_rw->mtype.spin_rate.y;
			obj->mtype.spin_rate.z = obj_rw->mtype.spin_rate.z;
			break;
	}
	
	switch (obj->control_type)
	{
		case CT_WEAPON:
			obj->ctype.laser_info.parent_type      = obj_rw->ctype.laser_info.parent_type;
			obj->ctype.laser_info.parent_num       = obj_rw->ctype.laser_info.parent_num;
			obj->ctype.laser_info.parent_signature = obj_rw->ctype.laser_info.parent_signature;
			obj->ctype.laser_info.creation_time    = obj_rw->ctype.laser_info.creation_time;
			obj->ctype.laser_info.last_hitobj      = obj_rw->ctype.laser_info.last_hitobj;
			obj->ctype.laser_info.hitobj_list[obj->ctype.laser_info.last_hitobj] = 1; // restore most recent hitobj to hitobj_list
			obj->ctype.laser_info.track_goal       = obj_rw->ctype.laser_info.track_goal;
			obj->ctype.laser_info.multiplier       = obj_rw->ctype.laser_info.multiplier;
			break;
			
		case CT_EXPLOSION:
			obj->ctype.expl_info.spawn_time    = obj_rw->ctype.expl_info.spawn_time;
			obj->ctype.expl_info.delete_time   = obj_rw->ctype.expl_info.delete_time;
			obj->ctype.expl_info.delete_objnum = obj_rw->ctype.expl_info.delete_objnum;
			obj->ctype.expl_info.attach_parent = obj_rw->ctype.expl_info.attach_parent;
			obj->ctype.expl_info.prev_attach   = obj_rw->ctype.expl_info.prev_attach;
			obj->ctype.expl_info.next_attach   = obj_rw->ctype.expl_info.next_attach;
			break;
			
		case CT_AI:
		{
			int i;
			obj->ctype.ai_info.behavior               = obj_rw->ctype.ai_info.behavior; 
			for (i = 0; i < MAX_AI_FLAGS; i++)
				obj->ctype.ai_info.flags[i]       = obj_rw->ctype.ai_info.flags[i]; 
			obj->ctype.ai_info.hide_segment           = obj_rw->ctype.ai_info.hide_segment;
			obj->ctype.ai_info.hide_index             = obj_rw->ctype.ai_info.hide_index;
			obj->ctype.ai_info.path_length            = obj_rw->ctype.ai_info.path_length;
			obj->ctype.ai_info.cur_path_index         = obj_rw->ctype.ai_info.cur_path_index;
			obj->ctype.ai_info.dying_sound_playing    = obj_rw->ctype.ai_info.dying_sound_playing;
			obj->ctype.ai_info.danger_laser_num       = obj_rw->ctype.ai_info.danger_laser_num;
			obj->ctype.ai_info.danger_laser_signature = obj_rw->ctype.ai_info.danger_laser_signature;
			obj->ctype.ai_info.dying_start_time       = obj_rw->ctype.ai_info.dying_start_time;
			break;
		}
			
		case CT_LIGHT:
			obj->ctype.light_info.intensity = obj_rw->ctype.light_info.intensity;
			break;
			
		case CT_POWERUP:
			obj->ctype.powerup_info.count         = obj_rw->ctype.powerup_info.count;
			obj->ctype.powerup_info.creation_time = obj_rw->ctype.powerup_info.creation_time;
			obj->ctype.powerup_info.flags         = obj_rw->ctype.powerup_info.flags;
			break;
		case CT_CNTRLCEN:
		{
			// gun points of reactor now part of the object but of course not saved in object_rw and overwritten due to reset_objects(). Let's just recompute them.
			int i = 0;
			reactor *reactor = get_reactor_definition(obj->id);
			for (i=0; i<reactor->n_guns; i++)
				calc_controlcen_gun_point(reactor, obj, i);
			break;
		}
	}
	
	switch (obj->render_type)
	{
		case RT_MORPH:
		case RT_POLYOBJ:
		case RT_NONE: // HACK below
		{
			int i;
			if (obj->render_type == RT_NONE && obj->type != OBJ_GHOST) // HACK: when a player is dead or not connected yet, clients still expect to get polyobj data - even if render_type == RT_NONE at this time. Here it's not important, but it might be for Multiplayer Savegames.
				break;
			obj->rtype.pobj_info.model_num                = obj_rw->rtype.pobj_info.model_num;
			for (i=0;i<MAX_SUBMODELS;i++)
			{
				obj->rtype.pobj_info.anim_angles[i].p = obj_rw->rtype.pobj_info.anim_angles[i].p;
				obj->rtype.pobj_info.anim_angles[i].b = obj_rw->rtype.pobj_info.anim_angles[i].b;
				obj->rtype.pobj_info.anim_angles[i].h = obj_rw->rtype.pobj_info.anim_angles[i].h;
			}
			obj->rtype.pobj_info.subobj_flags             = obj_rw->rtype.pobj_info.subobj_flags;
			obj->rtype.pobj_info.tmap_override            = obj_rw->rtype.pobj_info.tmap_override;
			obj->rtype.pobj_info.alt_textures             = obj_rw->rtype.pobj_info.alt_textures;
			break;
		}
			
		case RT_WEAPON_VCLIP:
		case RT_HOSTAGE:
		case RT_POWERUP:
		case RT_FIREBALL:
			obj->rtype.vclip_info.vclip_num = obj_rw->rtype.vclip_info.vclip_num;
			obj->rtype.vclip_info.frametime = obj_rw->rtype.vclip_info.frametime;
			obj->rtype.vclip_info.framenum  = obj_rw->rtype.vclip_info.framenum;
			break;
			
		case RT_LASER:
			break;
			
	}
}

// Following functions convert player to player_rw and back to be written to/read from Savegames. player only differ to player_rw in terms of timer values (fix/fix64). as we reset GameTime64 for writing so it can fit into fix it's not necessary to increment savegame version. But if we once store something else into object which might be useful after restoring, it might be handy to increment Savegame version and actually store these new infos.
// turn player to player_rw to be saved to Savegame.
void state_player_to_player_rw(player *pl, player_rw *pl_rw)
{
	int i=0;
	memcpy(pl_rw->callsign, pl->callsign, CALLSIGN_LEN+1);
	memcpy(pl_rw->net_address, pl->net_address, 6);
	pl_rw->connected                 = pl->connected;
	pl_rw->objnum                    = pl->objnum;
	pl_rw->n_packets_got             = pl->n_packets_got;
	pl_rw->n_packets_sent            = pl->n_packets_sent;
	pl_rw->flags                     = pl->flags;
	pl_rw->energy                    = pl->energy;
	pl_rw->shields                   = pl->shields;
	pl_rw->lives                     = pl->lives;
	pl_rw->level                     = pl->level;
	pl_rw->laser_level               = pl->laser_level;
	pl_rw->starting_level            = pl->starting_level;
	pl_rw->killer_objnum             = pl->killer_objnum;
	pl_rw->primary_weapon_flags      = pl->primary_weapon_flags;
	pl_rw->secondary_weapon_flags    = pl->secondary_weapon_flags;
	for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
		pl_rw->primary_ammo[i]   = pl->primary_ammo[i];
	for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
		pl_rw->secondary_ammo[i] = pl->secondary_ammo[i];
	pl_rw->last_score                = pl->last_score;
	pl_rw->score                     = pl->score;
	pl_rw->time_level                = pl->time_level;
	pl_rw->time_total                = pl->time_total;
	if (pl->cloak_time - GameTime64 < F1_0*(-18000))
		pl_rw->cloak_time        = F1_0*(-18000);
	else
		pl_rw->cloak_time        = pl->cloak_time - GameTime64;
	if (pl->invulnerable_time - GameTime64 < F1_0*(-18000))
		pl_rw->invulnerable_time = F1_0*(-18000);
	else
		pl_rw->invulnerable_time = pl->invulnerable_time - GameTime64;
	pl_rw->KillGoalCount             = pl->KillGoalCount;
	pl_rw->net_killed_total          = pl->net_killed_total;
	pl_rw->net_kills_total           = pl->net_kills_total;
	pl_rw->num_kills_level           = pl->num_kills_level;
	pl_rw->num_kills_total           = pl->num_kills_total;
	pl_rw->num_robots_level          = pl->num_robots_level;
	pl_rw->num_robots_total          = pl->num_robots_total;
	pl_rw->hostages_rescued_total    = pl->hostages_rescued_total;
	pl_rw->hostages_total            = pl->hostages_total;
	pl_rw->hostages_on_board         = pl->hostages_on_board;
	pl_rw->hostages_level            = pl->hostages_level;
	pl_rw->homing_object_dist        = pl->homing_object_dist;
	pl_rw->hours_level               = pl->hours_level;
	pl_rw->hours_total               = pl->hours_total;
}

// turn player_rw to player after reading from Savegame
void state_player_rw_to_player(player_rw *pl_rw, player *pl)
{
	int i=0;
	memcpy(pl->callsign, pl_rw->callsign, CALLSIGN_LEN+1);
	memcpy(pl->net_address, pl_rw->net_address, 6);
	pl->connected                 = pl_rw->connected;
	pl->objnum                    = pl_rw->objnum;
	pl->n_packets_got             = pl_rw->n_packets_got;
	pl->n_packets_sent            = pl_rw->n_packets_sent;
	pl->flags                     = pl_rw->flags;
	pl->energy                    = pl_rw->energy;
	pl->shields                   = pl_rw->shields;
	pl->lives                     = pl_rw->lives;
	pl->level                     = pl_rw->level;
	pl->laser_level               = pl_rw->laser_level;
	pl->starting_level            = pl_rw->starting_level;
	pl->killer_objnum             = pl_rw->killer_objnum;
	pl->primary_weapon_flags      = pl_rw->primary_weapon_flags;
	pl->secondary_weapon_flags    = pl_rw->secondary_weapon_flags;
	for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
		pl->primary_ammo[i]   = pl_rw->primary_ammo[i];
	for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
		pl->secondary_ammo[i] = pl_rw->secondary_ammo[i];
	pl->last_score                = pl_rw->last_score;
	pl->score                     = pl_rw->score;
	pl->time_level                = pl_rw->time_level;
	pl->time_total                = pl_rw->time_total;
	pl->cloak_time                = pl_rw->cloak_time;
	pl->invulnerable_time         = pl_rw->invulnerable_time;
	pl->KillGoalCount             = pl_rw->KillGoalCount;
	pl->net_killed_total          = pl_rw->net_killed_total;
	pl->net_kills_total           = pl_rw->net_kills_total;
	pl->num_kills_level           = pl_rw->num_kills_level;
	pl->num_kills_total           = pl_rw->num_kills_total;
	pl->num_robots_level          = pl_rw->num_robots_level;
	pl->num_robots_total          = pl_rw->num_robots_total;
	pl->hostages_rescued_total    = pl_rw->hostages_rescued_total;
	pl->hostages_total            = pl_rw->hostages_total;
	pl->hostages_on_board         = pl_rw->hostages_on_board;
	pl->hostages_level            = pl_rw->hostages_level;
	pl->homing_object_dist        = pl_rw->homing_object_dist;
	pl->hours_level               = pl_rw->hours_level;
	pl->hours_total               = pl_rw->hours_total;
}


//-------------------------------------------------------------------
int state_callback(newmenu *menu, d_event *event, grs_bitmap *sc_bmp[])
{
	newmenu_item *items = newmenu_get_items(menu);
	int citem = newmenu_get_citem(menu);
	
	if ( (citem > 0) && (event->type == EVENT_NEWMENU_DRAW) )
	{
		if ( sc_bmp[citem-1] )	{
			grs_canvas *save_canv = grd_curcanv;
#ifndef OGL
			grs_canvas *temp_canv = gr_create_canvas(FSPACX(THUMBNAIL_W),FSPACY(THUMBNAIL_H));
#else
			grs_canvas *temp_canv = gr_create_canvas(THUMBNAIL_W*2,(THUMBNAIL_H*24/10));
#endif
			grs_point vertbuf[3] = {{0,0}, {0,0}, {i2f(THUMBNAIL_W*2),i2f(THUMBNAIL_H*24/10)} };
			gr_set_current_canvas(temp_canv);
			scale_bitmap(sc_bmp[citem-1], vertbuf, 0);
			gr_set_current_canvas( save_canv );
#ifndef OGL
			gr_bitmap( (grd_curcanv->cv_bitmap.bm_w/2)-FSPACX(THUMBNAIL_W/2),items[0].y-3, &temp_canv->cv_bitmap);
#else
			ogl_ubitmapm_cs((grd_curcanv->cv_bitmap.bm_w/2)-FSPACX(THUMBNAIL_W/2),items[0].y-FSPACY(3),FSPACX(THUMBNAIL_W),FSPACY(THUMBNAIL_H),&temp_canv->cv_bitmap,-1,F1_0);
#endif
			gr_free_canvas(temp_canv);
		}
		
		return 1;
	}
	
	return 0;
}

#if 0
void rpad_string( char * string, int max_chars )
{
	int i, end_found;

	end_found = 0;
	for( i=0; i<max_chars; i++ )	{
		if ( *string == 0 )
			end_found = 1;
		if ( end_found )
			*string = ' ';
		string++;
	}
	*string = 0;		// NULL terminate
}
#endif

static int state_default_item = 0;
//Since state_default_item should ALWAYS point to a valid savegame slot, we use this to check if we once already actually SAVED a game. If yes, state_quick_item will be equal state_default_item, otherwise it should be -1 on every new mission and tell us we need to select a slot for quicksave.
int state_quick_item = -1;

/* Present a menu for selection of a savegame filename.
 * For saving, dsc should be a pre-allocated buffer into which the new
 * savegame description will be stored.
 * For restoring, dsc should be NULL, in which case empty slots will not be
 * selectable and savagames descriptions will not be editable.
 */
int state_get_savegame_filename(char * fname, char * dsc, char * caption, int blind_save)
{
	PHYSFS_file * fp;
	int i, choice, version, nsaves;
	newmenu_item m[NUM_SAVES+1];
	char filename[NUM_SAVES][PATH_MAX];
	char desc[NUM_SAVES][DESC_LENGTH + 16];
	grs_bitmap *sc_bmp[NUM_SAVES];
	char id[5], dummy_callsign[CALLSIGN_LEN+1];
	int valid;

	nsaves=0;
	m[0].type = NM_TYPE_TEXT; m[0].text = "\n\n\n\n";
	for (i=0;i<NUM_SAVES; i++ )	{
		sc_bmp[i] = NULL;
		snprintf( filename[i], PATH_MAX, GameArg.SysUsePlayersDir? "Players/%s.%sg%x" : "%s.%sg%x", Players[Player_num].callsign, (Game_mode & GM_MULTI_COOP)?"m":"s", i );
		valid = 0;
		fp = PHYSFSX_openReadBuffered(filename[i]);
		if ( fp ) {
			//Read id
			PHYSFS_read(fp, id, sizeof(char) * 4, 1);
			if ( !memcmp( id, dgss_id, 4 )) {
				//Read version
				PHYSFS_read(fp, &version, sizeof(int), 1);
				// In case it's Coop, read state_game_id & callsign as well
				if (Game_mode & GM_MULTI_COOP)
				{
					PHYSFS_seek(fp, PHYSFS_tell(fp) + sizeof(PHYSFS_sint32)); // skip state_game_id
					PHYSFS_read(fp, &dummy_callsign, sizeof(char)*CALLSIGN_LEN+1, 1);
				}
				if ((version >= STATE_COMPATIBLE_VERSION) || (SWAPINT(version) >= STATE_COMPATIBLE_VERSION)) {
					// Read description
					PHYSFS_read(fp, desc[i], sizeof(char) * DESC_LENGTH, 1);
					//rpad_string( desc[i], DESC_LENGTH-1 );
					if (dsc == NULL) m[i+1].type = NM_TYPE_MENU;
					// Read thumbnail
					sc_bmp[i] = gr_create_bitmap(THUMBNAIL_W,THUMBNAIL_H );
					PHYSFS_read(fp, sc_bmp[i]->bm_data, THUMBNAIL_W * THUMBNAIL_H, 1);
					if (version >= 9) {
						ubyte pal[256*3];
						PHYSFS_read(fp, pal, 3, 256);
						gr_remap_bitmap_good( sc_bmp[i], pal, -1, -1 );
					}
					nsaves++;
					valid = 1;
				}
			}
			PHYSFS_close(fp);
		} 
		if (!valid) {
			strcpy( desc[i], TXT_EMPTY );
			//rpad_string( desc[i], DESC_LENGTH-1 );
			if (dsc == NULL) m[i+1].type = NM_TYPE_TEXT;
		}
		if (dsc != NULL) {
			m[i+1].type = NM_TYPE_INPUT_MENU;
		}
		m[i+1].text_len = DESC_LENGTH-1;
		m[i+1].text = desc[i];
	}

	if ( dsc == NULL && nsaves < 1 )	{
		nm_messagebox( NULL, 1, "Ok", "No saved games were found!" );
		return 0;
	}

	sc_last_item = -1;

	if (blind_save && state_quick_item < 0)
		blind_save = 0;		// haven't picked a slot yet

	if (blind_save)
		choice = state_default_item + 1;
	else
		choice = newmenu_do2( NULL, caption, NUM_SAVES+1, m, (int (*)(newmenu *, d_event *, void *))state_callback, sc_bmp, state_default_item + 1, NULL );

	for (i=0; i<NUM_SAVES; i++ )	{
		if ( sc_bmp[i] )
			gr_free_bitmap( sc_bmp[i] );
	}

	if (choice > 0) {
		strcpy( fname, filename[choice-1] );
		if ( dsc != NULL ) strcpy( dsc, desc[choice-1] );
		state_quick_item = state_default_item = choice - 1;
		return choice;
	}
	return 0;
}

int state_get_save_file(char * fname, char * dsc, int blind_save)
{
	return state_get_savegame_filename(fname, dsc, "Save Game", blind_save);
}

int state_get_restore_file(char * fname)
{
	return state_get_savegame_filename(fname, NULL, "Select Game to Restore", 0);
}

#define	DESC_OFFSET	8

//	-----------------------------------------------------------------------------------
//	Imagine if C had a function to copy a file...
int copy_file(char *old_file, char *new_file)
{
	sbyte	*buf;
	int		buf_size;
	PHYSFS_file *in_file, *out_file;

	out_file = PHYSFS_openWrite(new_file);

	if (out_file == NULL)
		return -1;

	in_file = PHYSFS_openRead(old_file);

	if (in_file == NULL)
		return -2;

	buf_size = PHYSFS_fileLength(in_file);
	while (buf_size && !(buf = d_malloc(buf_size)))
		buf_size /= 2;
	if (buf_size == 0)
		return -5;	// likely to be an empty file

	while (!PHYSFS_eof(in_file))
	{
		int bytes_read;

		bytes_read = PHYSFS_read(in_file, buf, 1, buf_size);
		if (bytes_read < 0)
			Error("Cannot read from file <%s>: %s", old_file, PHYSFS_getLastError());

		Assert(bytes_read == buf_size || PHYSFS_eof(in_file));

		if (PHYSFS_write(out_file, buf, 1, bytes_read) < bytes_read)
			Error("Cannot write to file <%s>: %s", new_file, PHYSFS_getLastError());
	}

	d_free(buf);

	if (!PHYSFS_close(in_file))
	{
		PHYSFS_close(out_file);
		return -3;
	}

	if (!PHYSFS_close(out_file))
		return -4;

	return 0;
}

extern int Final_boss_is_dead;

//	-----------------------------------------------------------------------------------
int state_save_all(int secret_save, char *filename_override, int blind_save)
{
	int	rval, filenum = -1;
	char	filename[PATH_MAX], desc[DESC_LENGTH+1];

	if ((Current_level_num < 0) && (secret_save == 0)) {
		HUD_init_message_literal(HM_DEFAULT,  "Can't save in secret level!" );
		return 0;
	}

	if (Final_boss_is_dead)		//don't allow save while final boss is dying
		return 0;

	if ( Game_mode & GM_MULTI )
	{
		if (Game_mode & GM_MULTI_COOP)
			multi_initiate_save_game();
		return 0;
	}

	//	If this is a secret save and the control center has been destroyed, don't allow
	//	return to the base level.
	if (secret_save && (Control_center_destroyed)) {
		PHYSFS_delete(SECRETB_FILENAME);
		return 0;
	}

	stop_time();

	memset(&filename, '\0', PATH_MAX);
	memset(&desc, '\0', DESC_LENGTH+1);
	if (secret_save == 1) {
		filename_override = filename;
		sprintf(filename_override, SECRETB_FILENAME);
	} else if (secret_save == 2) {
		filename_override = filename;
		sprintf(filename_override, SECRETC_FILENAME);
	} else {
		if (!(filenum = state_get_save_file(filename, desc, blind_save)))
		{
			start_time();
			return 0;
		}
	}
		
	//	MK, 1/1/96
	//	Do special secret level stuff.
	//	If secret.sgc exists, then copy it to Nsecret.sgc (where N = filenum).
	//	If it doesn't exist, then delete Nsecret.sgc
	if (!secret_save && !(Game_mode & GM_MULTI_COOP)) {
		int	rval;
		char	temp_fname[PATH_MAX], fc;

		if (filenum != -1) {

			if (filenum >= 10)
				fc = (filenum-10) + 'a';
			else
				fc = '0' + filenum;

			sprintf(temp_fname, GameArg.SysUsePlayersDir? "Players/%csecret.sgc" : "%csecret.sgc", fc);

			if (PHYSFSX_exists(temp_fname,0))
			{
				if (!PHYSFS_delete(temp_fname))
					Error("Cannot delete file <%s>: %s", temp_fname, PHYSFS_getLastError());
			}

			if (PHYSFSX_exists(SECRETC_FILENAME,0))
			{
				rval = copy_file(SECRETC_FILENAME, temp_fname);
				Assert(rval == 0);	//	Oops, error copying secret.sgc to temp_fname!
				(void)rval;
			}
		}
	}

	rval = state_save_all_sub(filename, desc);

	if (rval && !secret_save)
		HUD_init_message_literal(HM_DEFAULT, "Game saved");

	return rval;
}

extern	fix	Flash_effect;
extern fix64 Time_flash_last_played;


int state_save_all_sub(char *filename, char *desc)
{
	int i,j;
	PHYSFS_file *fp;
	grs_canvas * cnv;
	ubyte *pal;
	char mission_filename[9];
#ifdef OGL
	GLint gl_draw_buffer;
#endif
	fix tmptime32 = 0;

	#ifndef NDEBUG
	if (GameArg.SysUsePlayersDir && strncmp(filename, "Players/", 8))
		Int3();
	#endif

	fp = PHYSFSX_openWriteBuffered(filename);
	if ( !fp ) {
		nm_messagebox(NULL, 1, TXT_OK, "Error writing savegame.\nPossibly out of disk\nspace.");
		start_time();
		return 0;
	}

//Save id
	PHYSFS_write(fp, dgss_id, sizeof(char) * 4, 1);

//Save version
	i = STATE_VERSION;
	PHYSFS_write(fp, &i, sizeof(int), 1);

// Save Coop state_game_id and this Player's callsign. Oh the redundancy... we have this one later on but Coop games want to read this before loading a state so for easy access save this here, too
	if (Game_mode & GM_MULTI_COOP)
	{
		PHYSFS_write(fp, &state_game_id, sizeof(uint), 1);
		PHYSFS_write(fp, &Players[Player_num].callsign, sizeof(char)*CALLSIGN_LEN+1, 1);
	}

//Save description
	PHYSFS_write(fp, desc, sizeof(char) * DESC_LENGTH, 1);

// Save the current screen shot...

	cnv = gr_create_canvas( THUMBNAIL_W, THUMBNAIL_H );
	if ( cnv )
	{
#ifdef OGL
		ubyte *buf;
		int k;
#endif
		grs_canvas * cnv_save;
		cnv_save = grd_curcanv;

		gr_set_current_canvas( cnv );

		render_frame(0, 0);

#if defined(OGL)
		buf = d_malloc(THUMBNAIL_W * THUMBNAIL_H * 4);
#ifndef OGLES
 		glGetIntegerv(GL_DRAW_BUFFER, &gl_draw_buffer);
 		glReadBuffer(gl_draw_buffer);
#endif
		glReadPixels(0, SHEIGHT - THUMBNAIL_H, THUMBNAIL_W, THUMBNAIL_H, GL_RGBA, GL_UNSIGNED_BYTE, buf);
		k = THUMBNAIL_H;
		for (i = 0; i < THUMBNAIL_W * THUMBNAIL_H; i++) {
			if (!(j = i % THUMBNAIL_W))
				k--;
			cnv->cv_bitmap.bm_data[THUMBNAIL_W * k + j] =
				gr_find_closest_color(buf[4*i]/4, buf[4*i+1]/4, buf[4*i+2]/4);
		}
		d_free(buf);
#endif
		pal = gr_palette;

		PHYSFS_write(fp, cnv->cv_bitmap.bm_data, THUMBNAIL_W * THUMBNAIL_H, 1);

		gr_set_current_canvas(cnv_save);
		gr_free_canvas( cnv );
		PHYSFS_write(fp, pal, 3, 256);
	}
	else
	{
	 	ubyte color = 0;
	 	for ( i=0; i<THUMBNAIL_W*THUMBNAIL_H; i++ )
			PHYSFS_write(fp, &color, sizeof(ubyte), 1);		
	} 

// Save the Between levels flag...
	i = 0;
	PHYSFS_write(fp, &i, sizeof(int), 1);

// Save the mission info...
	memset(&mission_filename, '\0', 9);
	snprintf(mission_filename, 9, "%s", Current_mission_filename); // Current_mission_filename is not necessarily 9 bytes long so for saving we use a proper string - preventing corruptions
	PHYSFS_write(fp, &mission_filename, 9 * sizeof(char), 1);

//Save level info
	PHYSFS_write(fp, &Current_level_num, sizeof(int), 1);
	PHYSFS_write(fp, &Next_level_num, sizeof(int), 1);

//Save GameTime
// NOTE: GameTime now is GameTime64 with fix64 since GameTime could only last 9 hrs. To even help old Savegames, we do not increment Savegame version but rather RESET GameTime64 to 0 on every save! ALL variables based on GameTime64 now will get the current GameTime64 value substracted and saved to fix size as well.
	tmptime32 = 0;
	PHYSFS_write(fp, &tmptime32, sizeof(fix), 1);

//Save player info
	//PHYSFS_write(fp, &Players[Player_num], sizeof(player), 1);
	{
		player_rw *pl_rw;
		CALLOC(pl_rw, player_rw, 1);
		state_player_to_player_rw(&Players[Player_num], pl_rw);
		PHYSFS_write(fp, pl_rw, sizeof(player_rw), 1);
		d_free(pl_rw);
	}

// Save the current weapon info
	PHYSFS_write(fp, &Primary_weapon, sizeof(sbyte), 1);
	PHYSFS_write(fp, &Secondary_weapon, sizeof(sbyte), 1);

// Save the difficulty level
	PHYSFS_write(fp, &Difficulty_level, sizeof(int), 1);

// Save cheats enabled
	PHYSFS_write(fp, &cheats.enabled, sizeof(int), 1);

//Finish all morph objects
	for (i=0; i<=Highest_object_index; i++ )	{
		if ( (Objects[i].type != OBJ_NONE) && (Objects[i].render_type==RT_MORPH))	{
			morph_data *md;
			md = find_morph_data(&Objects[i]);
			if (md) {					
				md->obj->control_type = md->morph_save_control_type;
				md->obj->movement_type = md->morph_save_movement_type;
				md->obj->render_type = RT_POLYOBJ;
				md->obj->mtype.phys_info = md->morph_save_phys_info;
				md->obj = NULL;
			} else {						//maybe loaded half-morphed from disk
				Objects[i].flags |= OF_SHOULD_BE_DEAD;
				Objects[i].render_type = RT_POLYOBJ;
				Objects[i].control_type = CT_NONE;
				Objects[i].movement_type = MT_NONE;
			}
		}
	}

//Save object info
	i = Highest_object_index+1;
	PHYSFS_write(fp, &i, sizeof(int), 1);
	//PHYSFS_write(fp, Objects, sizeof(object), i);
	for (i = 0; i <= Highest_object_index; i++)
	{
		object_rw *obj_rw;
		CALLOC(obj_rw, object_rw, 1);
		state_object_to_object_rw(&Objects[i], obj_rw);
		PHYSFS_write(fp, obj_rw, sizeof(object_rw), 1);
		d_free(obj_rw);
	}
	
//Save wall info
	i = Num_walls;
	PHYSFS_write(fp, &i, sizeof(int), 1);
	PHYSFS_write(fp, Walls, sizeof(wall), i);

//Save exploding wall info
	i = MAX_EXPLODING_WALLS;
	PHYSFS_write(fp, &i, sizeof(int), 1);
	PHYSFS_write(fp, expl_wall_list, sizeof(*expl_wall_list), i);

//Save door info
	i = Num_open_doors;
	PHYSFS_write(fp, &i, sizeof(int), 1);
	PHYSFS_write(fp, ActiveDoors, sizeof(active_door), i);

//Save cloaking wall info
	i = Num_cloaking_walls;
	PHYSFS_write(fp, &i, sizeof(int), 1);
	PHYSFS_write(fp, CloakingWalls, sizeof(cloaking_wall), i);

//Save trigger info
	PHYSFS_write(fp, &Num_triggers, sizeof(int), 1);
	PHYSFS_write(fp, Triggers, sizeof(trigger), Num_triggers);

//Save tmap info
	for (i = 0; i <= Highest_segment_index; i++)
	{
		for (j = 0; j < 6; j++)
		{
			PHYSFS_write(fp, &Segments[i].sides[j].wall_num, sizeof(short), 1);
			PHYSFS_write(fp, &Segments[i].sides[j].tmap_num, sizeof(short), 1);
			PHYSFS_write(fp, &Segments[i].sides[j].tmap_num2, sizeof(short), 1);
		}
	}

// Save the fuelcen info
	PHYSFS_write(fp, &Control_center_destroyed, sizeof(int), 1);
	PHYSFS_write(fp, &Countdown_timer, sizeof(int), 1);
	PHYSFS_write(fp, &Num_robot_centers, sizeof(int), 1);
	PHYSFS_write(fp, RobotCenters, sizeof(matcen_info), Num_robot_centers);
	PHYSFS_write(fp, &ControlCenterTriggers, sizeof(control_center_triggers), 1);
	PHYSFS_write(fp, &Num_fuelcenters, sizeof(int), 1);
	PHYSFS_write(fp, Station, sizeof(FuelCenter), Num_fuelcenters);

// Save the control cen info
	PHYSFS_write(fp, &Control_center_been_hit, sizeof(int), 1);
	PHYSFS_write(fp, &Control_center_player_been_seen, sizeof(int), 1);
	PHYSFS_write(fp, &Control_center_next_fire_time, sizeof(int), 1);
	PHYSFS_write(fp, &Control_center_present, sizeof(int), 1);
	PHYSFS_write(fp, &Dead_controlcen_object_num, sizeof(int), 1);

// Save the AI state
	ai_save_state( fp );

// Save the automap visited info
	if ( Highest_segment_index+1 > MAX_SEGMENTS_ORIGINAL )
	{
		PHYSFS_write(fp, Automap_visited, sizeof(ubyte), Highest_segment_index + 1);
	}
	else
		PHYSFS_write(fp, Automap_visited, sizeof(ubyte), MAX_SEGMENTS_ORIGINAL);

	PHYSFS_write(fp, &state_game_id, sizeof(uint), 1);
	i = 0;
	PHYSFS_write(fp, &cheats.rapidfire, sizeof(int), 1);
	PHYSFS_write(fp, &i, sizeof(int), 1); // was Lunacy
	PHYSFS_write(fp, &i, sizeof(int), 1); // was Lunacy, too... and one was Ugly robot stuff a long time ago...

	// Save automap marker info

	PHYSFS_write(fp, MarkerObject, sizeof(MarkerObject) ,1);
	PHYSFS_write(fp, &Players[0].callsign[0], sizeof(char), (NUM_MARKERS)*(CALLSIGN_LEN+1)); // PHYSFS_write(fp, MarkerOwner, sizeof(MarkerOwner), 1); MarkerOwner is obsolete
	PHYSFS_write(fp, MarkerMessage, sizeof(MarkerMessage), 1);

	PHYSFS_write(fp, &Afterburner_charge, sizeof(fix), 1);

	//save last was super information
	PHYSFS_write(fp, &Primary_last_was_super, sizeof(Primary_last_was_super), 1);
	PHYSFS_write(fp, &Secondary_last_was_super, sizeof(Secondary_last_was_super), 1);

	//	Save flash effect stuff
	PHYSFS_write(fp, &Flash_effect, sizeof(int), 1);
	if (Time_flash_last_played - GameTime64 < F1_0*(-18000))
		tmptime32 = F1_0*(-18000);
	else
		tmptime32 = Time_flash_last_played - GameTime64;
	PHYSFS_write(fp, &tmptime32, sizeof(fix), 1);
	PHYSFS_write(fp, &PaletteRedAdd, sizeof(int), 1);
	PHYSFS_write(fp, &PaletteGreenAdd, sizeof(int), 1);
	PHYSFS_write(fp, &PaletteBlueAdd, sizeof(int), 1);
	if ( Highest_segment_index+1 > MAX_SEGMENTS_ORIGINAL )
	{
		PHYSFS_write(fp, Light_subtracted, sizeof(Light_subtracted[0]), Highest_segment_index + 1);
	}
	else
		PHYSFS_write(fp, Light_subtracted, sizeof(Light_subtracted[0]), MAX_SEGMENTS_ORIGINAL);
	PHYSFS_write(fp, &First_secret_visit, sizeof(First_secret_visit), 1);
	PHYSFS_write(fp, &Omega_charge, sizeof(Omega_charge), 1);

// Save Coop Info
	if (Game_mode & GM_MULTI_COOP)
	{
		for (i = 0; i < MAX_PLAYERS; i++) // I know, I know we only allow 4 players in coop. I screwed that up. But if we ever allow 8 players in coop, who's gonna laugh then?
		{
			player_rw *pl_rw;
			CALLOC(pl_rw, player_rw, 1);
			state_player_to_player_rw(&Players[i], pl_rw);
			PHYSFS_write(fp, pl_rw, sizeof(player_rw), 1);
			d_free(pl_rw);
		}
		PHYSFS_write(fp, &Netgame.mission_title, sizeof(char), MISSION_NAME_LEN+1);
		PHYSFS_write(fp, &Netgame.mission_name, sizeof(char), 9);
		PHYSFS_write(fp, &Netgame.levelnum, sizeof(int), 1);
		PHYSFS_write(fp, &Netgame.difficulty, sizeof(ubyte), 1);
		PHYSFS_write(fp, &Netgame.game_status, sizeof(ubyte), 1);
		PHYSFS_write(fp, &Netgame.numplayers, sizeof(ubyte), 1);
		PHYSFS_write(fp, &Netgame.max_numplayers, sizeof(ubyte), 1);
		PHYSFS_write(fp, &Netgame.numconnected, sizeof(ubyte), 1);
		PHYSFS_write(fp, &Netgame.level_time, sizeof(int), 1);
	}

	PHYSFS_close(fp);
	
	start_time();

	return 1;
}

//	-----------------------------------------------------------------------------------
//	Set the player's position from the globals Secret_return_segment and Secret_return_orient.
void set_pos_from_return_segment(void)
{
	int	plobjnum = Players[Player_num].objnum;

	compute_segment_center(&Objects[plobjnum].pos, &Segments[Secret_return_segment]);
	obj_relink(plobjnum, Secret_return_segment);
	reset_player_object();
	Objects[plobjnum].orient = Secret_return_orient;
}

//	-----------------------------------------------------------------------------------
int state_restore_all(int in_game, int secret_restore, char *filename_override)
{
	char filename[PATH_MAX];
	int	filenum = -1;

	if (in_game && (Current_level_num < 0) && (secret_restore == 0)) {
		HUD_init_message_literal(HM_DEFAULT,  "Can't restore in secret level!" );
		return 0;
	}

	if ( Newdemo_state == ND_STATE_RECORDING )
		newdemo_stop_recording();

	if ( Newdemo_state != ND_STATE_NORMAL )
		return 0;

	if ( Game_mode & GM_MULTI )
	{
		if (Game_mode & GM_MULTI_COOP)
			multi_initiate_restore_game();
		return 0;
	}

	stop_time();

	if (filename_override) {
		strcpy(filename, filename_override);
		filenum = NUM_SAVES+1; // place outside of save slots
	} else if (!(filenum = state_get_restore_file(filename)))	{
		start_time();
		return 0;
	}
	
	//	MK, 1/1/96
	//	Do special secret level stuff.
	//	If Nsecret.sgc (where N = filenum) exists, then copy it to secret.sgc.
	//	If it doesn't exist, then delete secret.sgc
	if (!secret_restore) {
		int	rval;
		char	temp_fname[PATH_MAX], fc;

		if (filenum != -1) {
			if (filenum >= 10)
				fc = (filenum-10) + 'a';
			else
				fc = '0' + filenum;
			
			snprintf(temp_fname, PATH_MAX, GameArg.SysUsePlayersDir? "Players/%csecret.sgc" : "%csecret.sgc", fc);

			if (PHYSFSX_exists(temp_fname,0))
			{
				rval = copy_file(temp_fname, SECRETC_FILENAME);
				Assert(rval == 0);	//	Oops, error copying temp_fname to secret.sgc!
				(void)rval;
			} else
				PHYSFS_delete(SECRETC_FILENAME);
		}
	}

	if ( !secret_restore && in_game ) {
		int choice;
		choice =  nm_messagebox( NULL, 2, "Yes", "No", "Restore Game?" );
		if ( choice != 0 )	{
			start_time();
			return 0;
		}
	}

	start_time();

	return state_restore_all_sub(filename, secret_restore);
}

extern void init_player_stats_new_ship(ubyte pnum);

void ShowLevelIntro(int level_num);

extern void do_cloak_invul_secret_stuff(fix64 old_gametime);
extern void copy_defaults_to_robot(object *objp);

int state_restore_all_sub(char *filename, int secret_restore)
{
	int version,i, j, segnum, coop_player_got[MAX_PLAYERS], coop_org_objnum = Players[Player_num].objnum;
	object * obj;
	PHYSFS_file *fp;
	int swap = 0;	// if file is not endian native, have to swap all shorts and ints
	int current_level;
	char mission[16];
	char desc[DESC_LENGTH+1];
	char id[5];
	char org_callsign[CALLSIGN_LEN+16];
	fix tmptime32 = 0;
	fix64	old_gametime = GameTime64;
	short TempTmapNum[MAX_SEGMENTS][MAX_SIDES_PER_SEGMENT];
	short TempTmapNum2[MAX_SEGMENTS][MAX_SIDES_PER_SEGMENT];

	#ifndef NDEBUG
	if (GameArg.SysUsePlayersDir && strncmp(filename, "Players/", 8))
		Int3();
	#endif

	fp = PHYSFSX_openReadBuffered(filename);
	if ( !fp ) return 0;

//Read id
	PHYSFS_read(fp, id, sizeof(char) * 4, 1);
	if ( memcmp( id, dgss_id, 4 )) {
		PHYSFS_close(fp);
		return 0;
	}

//Read version
	//Check for swapped file here, as dgss_id is written as a string (i.e. endian independent)
	PHYSFS_read(fp, &version, sizeof(int), 1);
	if (version & 0xffff0000)
	{
		swap = 1;
		version = SWAPINT(version);
	}

	if (version < STATE_COMPATIBLE_VERSION)	{
		PHYSFS_close(fp);
		return 0;
	}

// Read Coop state_game_id. Oh the redundancy... we have this one later on but Coop games want to read this before loading a state so for easy access we have this here
	if (Game_mode & GM_MULTI_COOP)
	{
		char saved_callsign[CALLSIGN_LEN+1];
		state_game_id = PHYSFSX_readSXE32(fp, swap);
		PHYSFS_read(fp, &saved_callsign, sizeof(char)*CALLSIGN_LEN+1, 1);
		if (strcmp(saved_callsign, Players[Player_num].callsign)) // check the callsign of the palyer who saved this state. It MUST match. If we transferred this savegame from pilot A to pilot B, others won't be able to restore us. So bail out here if this is the case.
		{
			PHYSFS_close(fp);
			return 0;
		}
	}

// Read description
	PHYSFS_read(fp, desc, sizeof(char) * DESC_LENGTH, 1);

// Skip the current screen shot...
	PHYSFS_seek(fp, PHYSFS_tell(fp) + THUMBNAIL_W * THUMBNAIL_H);

// And now...skip the goddamn palette stuff that somebody forgot to add
	PHYSFS_seek(fp, PHYSFS_tell(fp) + 768);

// Read the Between levels flag...
	i = PHYSFSX_readSXE32(fp, swap);
	i = 0;

// Read the mission info...
	PHYSFS_read(fp, mission, sizeof(char) * 9, 1);

	if (!load_mission_by_name( mission ))	{
		nm_messagebox( NULL, 1, "Ok", "Error!\nUnable to load mission\n'%s'\n", mission );
		PHYSFS_close(fp);
		return 0;
	}

//Read level info
	current_level = PHYSFSX_readSXE32(fp, swap);
	PHYSFS_seek(fp, PHYSFS_tell(fp) + sizeof(PHYSFS_sint32)); // skip Next_level_num

//Restore GameTime
	tmptime32 = PHYSFSX_readSXE32(fp, swap);
	GameTime64 = (fix64)tmptime32;

// Start new game....
	if (!(Game_mode & GM_MULTI_COOP))
	{
		Game_mode = GM_NORMAL;
#ifdef NETWORK
		change_playernum_to(0);
#endif
		N_players = 1;
		strcpy( org_callsign, Players[0].callsign );
		if (!secret_restore) {
			InitPlayerObject();				//make sure player's object set up
			init_player_stats_game(0);		//clear all stats
		}
	}
	else // in coop we want to stay the player we are already.
	{
		strcpy( org_callsign, Players[Player_num].callsign );
		if (!secret_restore)
			init_player_stats_game(Player_num);
	}

	if (Game_wind)
		window_set_visible(Game_wind, 0);

//Read player info

	{
		StartNewLevelSub(current_level, 1, secret_restore);

		if (secret_restore) {
			player	dummy_player;
			player_rw *pl_rw;
			MALLOC(pl_rw, player_rw, 1);
			PHYSFS_read(fp, pl_rw, sizeof(player_rw), 1);
			player_rw_swap(pl_rw, swap);
			state_player_rw_to_player(pl_rw, &dummy_player);
			d_free(pl_rw);
			if (secret_restore == 1) {		//	This means he didn't die, so he keeps what he got in the secret level.
				Players[Player_num].level = dummy_player.level;
				Players[Player_num].last_score = dummy_player.last_score;
				Players[Player_num].time_level = dummy_player.time_level;

				Players[Player_num].num_robots_level = dummy_player.num_robots_level;
				Players[Player_num].num_robots_total = dummy_player.num_robots_total;
				Players[Player_num].hostages_rescued_total = dummy_player.hostages_rescued_total;
				Players[Player_num].hostages_total = dummy_player.hostages_total;
				Players[Player_num].hostages_on_board = dummy_player.hostages_on_board;
				Players[Player_num].hostages_level = dummy_player.hostages_level;
				Players[Player_num].homing_object_dist = dummy_player.homing_object_dist;
				Players[Player_num].hours_level = dummy_player.hours_level;
				Players[Player_num].hours_total = dummy_player.hours_total;
				do_cloak_invul_secret_stuff(old_gametime);
			} else {
				Players[Player_num] = dummy_player;
			}
		} else {
			player_rw *pl_rw;
			MALLOC(pl_rw, player_rw, 1);
			PHYSFS_read(fp, pl_rw, sizeof(player_rw), 1);
			player_rw_swap(pl_rw, swap);
			state_player_rw_to_player(pl_rw, &Players[Player_num]);
			d_free(pl_rw);
		}
	}
	strcpy( Players[Player_num].callsign, org_callsign );
	if (Game_mode & GM_MULTI_COOP)
		Players[Player_num].objnum = coop_org_objnum;

// Restore the weapon states
	PHYSFS_read(fp, &Primary_weapon, sizeof(sbyte), 1);
	PHYSFS_read(fp, &Secondary_weapon, sizeof(sbyte), 1);

	select_weapon(Primary_weapon, 0, 0, 0);
	select_weapon(Secondary_weapon, 1, 0, 0);

// Restore the difficulty level
	Difficulty_level = PHYSFSX_readSXE32(fp, swap);

// Restore the cheats enabled flag
	game_disable_cheats(); // disable cheats first
	cheats.enabled = PHYSFSX_readSXE32(fp, swap);

	Do_appearance_effect = 0;			// Don't do this for middle o' game stuff.

	//Clear out all the objects from the lvl file
	for (segnum=0; segnum <= Highest_segment_index; segnum++)
		Segments[segnum].objects = -1;
	reset_objects(1);

	//Read objects, and pop 'em into their respective segments.
	i = PHYSFSX_readSXE32(fp, swap);
	Highest_object_index = i-1;
	//object_read_n_swap(Objects, i, swap, fp);
	for (i=0; i<=Highest_object_index; i++ )
	{
		object_rw *obj_rw;
		MALLOC(obj_rw, object_rw, 1);
		PHYSFS_read(fp, obj_rw, sizeof(object_rw), 1);
		object_rw_swap(obj_rw, swap);
		state_object_rw_to_object(obj_rw, &Objects[i]);
		d_free(obj_rw);
	}

	for (i=0; i<=Highest_object_index; i++ )	{
		obj = &Objects[i];
		obj->rtype.pobj_info.alt_textures = -1;
		segnum = obj->segnum;
		obj->next = obj->prev = obj->segnum = -1;
		if ( obj->type != OBJ_NONE )	{
			obj_link(i,segnum);
		}

		//look for, and fix, boss with bogus shields
		if (obj->type == OBJ_ROBOT && Robot_info[obj->id].boss_flag) {
			fix save_shields = obj->shields;

			copy_defaults_to_robot(obj);		//calculate starting shields

			//if in valid range, use loaded shield value
			if (save_shields > 0 && save_shields <= obj->shields)
				obj->shields = save_shields;
			else
				obj->shields /= 2;  //give player a break
		}
	}
	special_reset_objects();

	//	1 = Didn't die on secret level.
	//	2 = Died on secret level.
	if (secret_restore && (Current_level_num >= 0)) {
		set_pos_from_return_segment();
		if (secret_restore == 2)
			init_player_stats_new_ship(Player_num);
	}

	//Restore wall info
	Num_walls = PHYSFSX_readSXE32(fp, swap);
	wall_read_n_swap(Walls, Num_walls, swap, fp);

	//now that we have the walls, check if any sounds are linked to
	//walls that are now open
	for (i=0;i<Num_walls;i++) {
		if (Walls[i].type == WALL_OPEN)
			digi_kill_sound_linked_to_segment(Walls[i].segnum,Walls[i].sidenum,-1);	//-1 means kill any sound
	}

	//Restore exploding wall info
	if (version >= 10) {
		i = PHYSFSX_readSXE32(fp, swap);
		expl_wall_read_n_swap(expl_wall_list, i, swap, fp);
	}

	//Restore door info
	Num_open_doors = PHYSFSX_readSXE32(fp, swap);
	active_door_read_n_swap(ActiveDoors, Num_open_doors, swap, fp);

	if (version >= 14) {		//Restore cloaking wall info
		Num_cloaking_walls = PHYSFSX_readSXE32(fp, swap);
		cloaking_wall_read_n_swap(CloakingWalls, Num_cloaking_walls, swap, fp);
	}

	//Restore trigger info
	Num_triggers = PHYSFSX_readSXE32(fp, swap);
	trigger_read_n_swap(Triggers, Num_triggers, swap, fp);

	//Restore tmap info (to temp values so we can use compiled-in tmap info to compute static_light
	for (i=0; i<=Highest_segment_index; i++ )	{
		for (j=0; j<6; j++ )	{
			Segments[i].sides[j].wall_num = PHYSFSX_readSXE16(fp, swap);
			TempTmapNum[i][j] = PHYSFSX_readSXE16(fp, swap);
			TempTmapNum2[i][j] = PHYSFSX_readSXE16(fp, swap);
		}
	}

	//Restore the fuelcen info
	Control_center_destroyed = PHYSFSX_readSXE32(fp, swap);
	Countdown_timer = PHYSFSX_readSXE32(fp, swap);
	Num_robot_centers = PHYSFSX_readSXE32(fp, swap);
	matcen_info_read_n_swap(RobotCenters, Num_robot_centers, swap, fp);
	control_center_triggers_read_n_swap(&ControlCenterTriggers, 1, swap, fp);
	Num_fuelcenters = PHYSFSX_readSXE32(fp, swap);
	fuelcen_read_n_swap(Station, Num_fuelcenters, swap, fp);

	// Restore the control cen info
	Control_center_been_hit = PHYSFSX_readSXE32(fp, swap);
	Control_center_player_been_seen = PHYSFSX_readSXE32(fp, swap);
	Control_center_next_fire_time = PHYSFSX_readSXE32(fp, swap);
	Control_center_present = PHYSFSX_readSXE32(fp, swap);
	Dead_controlcen_object_num = PHYSFSX_readSXE32(fp, swap);
	if (Control_center_destroyed)
		Total_countdown_time = Countdown_timer/F0_5; // we do not need to know this, but it should not be 0 either...
		

	// Restore the AI state
	ai_restore_state( fp, version, swap );

	// Restore the automap visited info
	if ( Highest_segment_index+1 > MAX_SEGMENTS_ORIGINAL )
	{
		memset(&Automap_visited, 0, MAX_SEGMENTS);
		PHYSFS_read(fp, Automap_visited, sizeof(ubyte), Highest_segment_index + 1);
	}
	else
		PHYSFS_read(fp, Automap_visited, sizeof(ubyte), MAX_SEGMENTS_ORIGINAL);

	//	Restore hacked up weapon system stuff.
	Auto_fire_fusion_cannon_time = 0;
	Next_laser_fire_time = GameTime64;
	Next_missile_fire_time = GameTime64;
	Last_laser_fired_time = GameTime64;

	state_game_id = 0;

	if ( version >= 7 )	{
		state_game_id = PHYSFSX_readSXE32(fp, swap);
		cheats.rapidfire = PHYSFSX_readSXE32(fp, swap);
		PHYSFS_seek(fp, PHYSFS_tell(fp) + sizeof(PHYSFS_sint32)); // PHYSFSX_readSXE32(fp, swap); // was Lunacy
		PHYSFS_seek(fp, PHYSFS_tell(fp) + sizeof(PHYSFS_sint32)); // PHYSFSX_readSXE32(fp, swap); // was Lunacy, too... and one was Ugly robot stuff a long time ago...
	}

	if (version >= 17) {
		for (i = 0; i < NUM_MARKERS; i++)
			MarkerObject[i] = PHYSFSX_readSXE32(fp, swap);
		PHYSFS_seek(fp, PHYSFS_tell(fp) + (NUM_MARKERS)*(CALLSIGN_LEN+1)); // PHYSFS_read(fp, MarkerOwner, sizeof(MarkerOwner), 1); // skip obsolete MarkerOwner
		PHYSFS_read(fp, MarkerMessage, sizeof(MarkerMessage), 1);
	}
	else {
		int num;

		// skip dummy info

		num = PHYSFSX_readSXE32(fp, swap);           // was NumOfMarkers
		PHYSFS_seek(fp, PHYSFS_tell(fp) + sizeof(PHYSFS_sint32)); // PHYSFSX_readSXE32(fp, swap); // was CurMarker

		PHYSFS_seek(fp, PHYSFS_tell(fp) + num * (sizeof(vms_vector) + 40));

		for (num=0;num<NUM_MARKERS;num++)
			MarkerObject[num] = -1;
	}

	if (version>=11) {
		if (secret_restore != 1)
			Afterburner_charge = PHYSFSX_readSXE32(fp, swap);
		else {
			PHYSFSX_readSXE32(fp, swap);
		}
	}
	if (version>=12) {
		//read last was super information
		PHYSFS_read(fp, &Primary_last_was_super, sizeof(Primary_last_was_super), 1);
		PHYSFS_read(fp, &Secondary_last_was_super, sizeof(Secondary_last_was_super), 1);
	}

	if (version >= 12) {
		Flash_effect = PHYSFSX_readSXE32(fp, swap);
		tmptime32 = PHYSFSX_readSXE32(fp, swap);
		Time_flash_last_played = (fix64)tmptime32;
		PaletteRedAdd = PHYSFSX_readSXE32(fp, swap);
		PaletteGreenAdd = PHYSFSX_readSXE32(fp, swap);
		PaletteBlueAdd = PHYSFSX_readSXE32(fp, swap);
	} else {
		Flash_effect = 0;
		Time_flash_last_played = 0;
		PaletteRedAdd = 0;
		PaletteGreenAdd = 0;
		PaletteBlueAdd = 0;
	}

	//	Load Light_subtracted
	if (version >= 16) {
		if ( Highest_segment_index+1 > MAX_SEGMENTS_ORIGINAL )
		{
			memset(&Light_subtracted, 0, sizeof(Light_subtracted[0])*MAX_SEGMENTS);
			PHYSFS_read(fp, Light_subtracted, sizeof(Light_subtracted[0]), Highest_segment_index + 1);
		}
		else
			PHYSFS_read(fp, Light_subtracted, sizeof(Light_subtracted[0]), MAX_SEGMENTS_ORIGINAL);
		apply_all_changed_light();
	} else {
		int	i;
		for (i=0; i<=Highest_segment_index; i++)
			Light_subtracted[i] = 0;
	}

	// static_light should now be computed - now actually set tmap info
	for (i=0; i<=Highest_segment_index; i++ )	{
		for (j=0; j<6; j++ )	{
			Segments[i].sides[j].tmap_num=TempTmapNum[i][j];
			Segments[i].sides[j].tmap_num2=TempTmapNum2[i][j];
		}
	}

	if (!secret_restore) {
		if (version >= 20) {
			First_secret_visit = PHYSFSX_readSXE32(fp, swap);
		} else
			First_secret_visit = 1;
	} else
		First_secret_visit = 0;

	if (version >= 22)
	{
		if (secret_restore != 1)
			Omega_charge = PHYSFSX_readSXE32(fp, swap);
		else
			PHYSFSX_readSXE32(fp, swap);
	}

// Read Coop Info
	if (Game_mode & GM_MULTI_COOP)
	{
		player restore_players[MAX_PLAYERS];
		object restore_objects[MAX_PLAYERS];
		int coop_got_nplayers = 0;

		for (i = 0; i < MAX_PLAYERS; i++) 
		{
			player_rw *pl_rw;
			object *obj;

			// prepare arrays for mapping our players below
			coop_player_got[i] = 0;

			// read the stored players
			MALLOC(pl_rw, player_rw, 1);
			PHYSFS_read(fp, pl_rw, sizeof(player_rw), 1);
			player_rw_swap(pl_rw, swap);
			state_player_rw_to_player(pl_rw, &restore_players[i]);
			d_free(pl_rw);
			
			// make all (previous) player objects to ghosts but store them first for later remapping
			obj = &Objects[restore_players[i].objnum];
			if (restore_players[i].connected == CONNECT_PLAYING && obj->type == OBJ_PLAYER)
			{
				memcpy(&restore_objects[i], obj, sizeof(object));
				obj->type = OBJ_GHOST;
				multi_reset_player_object(obj);
			}
		}
		for (i = 0; i < MAX_PLAYERS; i++) // copy restored players to the current slots
		{
			for (j = 0; j < MAX_PLAYERS; j++)
			{
				// map stored players to current players depending on their unique (which we made sure) callsign
				if (Players[i].connected == CONNECT_PLAYING && restore_players[j].connected == CONNECT_PLAYING && !strcmp(Players[i].callsign, restore_players[j].callsign))
				{
					object *obj;
					int sav_objnum = Players[i].objnum;
					
					memcpy(&Players[i], &restore_players[j], sizeof(player));
					Players[i].objnum = sav_objnum;
					
					coop_player_got[i] = 1;
					coop_got_nplayers++;

					obj = &Objects[Players[i].objnum];
					// since a player always uses the same object, we just have to copy the saved object properties to the existing one. i hate you...
					obj->id = i; // assign player object id to player number
					obj->control_type = restore_objects[j].control_type;
					obj->movement_type = restore_objects[j].movement_type;
					obj->render_type = restore_objects[j].render_type;
					obj->flags = restore_objects[j].flags;
					obj->pos = restore_objects[j].pos;
					obj->orient = restore_objects[j].orient;
					obj->size = restore_objects[j].size;
					obj->shields = restore_objects[j].shields;
					obj->lifeleft = restore_objects[j].lifeleft;
					obj->mtype.phys_info = restore_objects[j].mtype.phys_info;
					obj->rtype.pobj_info = restore_objects[j].rtype.pobj_info;
					// make this restored player object an actual player again
					obj->type = OBJ_PLAYER;
					multi_reset_player_object(obj);
					update_object_seg(obj);
				}
			}
		}
		PHYSFS_read(fp, &Netgame.mission_title, sizeof(char), MISSION_NAME_LEN+1);
		PHYSFS_read(fp, &Netgame.mission_name, sizeof(char), 9);
		Netgame.levelnum = PHYSFSX_readSXE32(fp, swap);
		PHYSFS_read(fp, &Netgame.difficulty, sizeof(ubyte), 1);
		PHYSFS_read(fp, &Netgame.game_status, sizeof(ubyte), 1);
		PHYSFS_read(fp, &Netgame.numplayers, sizeof(ubyte), 1);
		PHYSFS_read(fp, &Netgame.max_numplayers, sizeof(ubyte), 1);
		PHYSFS_read(fp, &Netgame.numconnected, sizeof(ubyte), 1);
		Netgame.level_time = PHYSFSX_readSXE32(fp, swap);
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			Netgame.killed[i] = Players[i].net_killed_total;
			Netgame.player_score[i] = Players[i].score;
			Netgame.player_flags[i] = Players[i].flags;
		}
		for (i = 0; i < MAX_PLAYERS; i++) // Disconnect connected players not available in this Savegame
			if (!coop_player_got[i] && Players[i].connected == CONNECT_PLAYING)
				multi_disconnect_player(i);
		Viewer = ConsoleObject = &Objects[Players[Player_num].objnum]; // make sure Viewer and ConsoleObject are set up (which we skipped by not using InitPlayerObject but we need since objects changed while loading)
		special_reset_objects(); // since we juggeled around with objects to remap coop players rebuild the index of free objects
	}

	PHYSFS_close(fp);

	if (Game_wind)
		if (!window_is_visible(Game_wind))
			window_set_visible(Game_wind, 1);
	reset_time();

	return 1;
}

int state_get_game_id(char *filename)
{
	int version;
	PHYSFS_file *fp;
	int swap = 0;	// if file is not endian native, have to swap all shorts and ints
	char id[5], saved_callsign[CALLSIGN_LEN+1];

	#ifndef NDEBUG
	if (GameArg.SysUsePlayersDir && strncmp(filename, "Players/", 8))
		Int3();
	#endif

	if (!(Game_mode & GM_MULTI_COOP))
		return 0;

	fp = PHYSFSX_openReadBuffered(filename);
	if ( !fp ) return 0;

//Read id
	PHYSFS_read(fp, id, sizeof(char) * 4, 1);
	if ( memcmp( id, dgss_id, 4 )) {
		PHYSFS_close(fp);
		return 0;
	}

//Read version
	//Check for swapped file here, as dgss_id is written as a string (i.e. endian independent)
	PHYSFS_read(fp, &version, sizeof(int), 1);
	if (version & 0xffff0000)
	{
		swap = 1;
		version = SWAPINT(version);
	}

	if (version < STATE_COMPATIBLE_VERSION)	{
		PHYSFS_close(fp);
		return 0;
	}

// Read Coop state_game_id to validate the savegame we are about to load matches the others
	state_game_id = PHYSFSX_readSXE32(fp, swap);
	PHYSFS_read(fp, &saved_callsign, sizeof(char)*CALLSIGN_LEN+1, 1);
	if (strcmp(saved_callsign, Players[Player_num].callsign)) // check the callsign of the palyer who saved this state. It MUST match. If we transferred this savegame from pilot A to pilot B, others won't be able to restore us. So bail out here if this is the case.
		return 0;

	return state_game_id;
}
