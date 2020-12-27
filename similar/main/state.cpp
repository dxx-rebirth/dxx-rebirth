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
#include "console.h"
#include "config.h"
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
#include "playsave.h"
#include "cntrlcen.h"
#include "morph.h"
#include "weapon.h"
#include "render.h"
#include "gameseq.h"
#include "event.h"
#include "robot.h"
#include "gauges.h"
#include "newdemo.h"
#include "automap.h"
#include "piggy.h"
#include "paging.h"
#include "titles.h"
#include "text.h"
#include "mission.h"
#include "pcx.h"
#include "collide.h"
#include "u_mem.h"
#include "args.h"
#include "ai.h"
#include "fireball.h"
#include "controls.h"
#include "laser.h"
#include "hudmsg.h"
#include "state.h"
#include "multi.h"
#include "gr.h"
#if DXX_USE_OGL
#include "ogl_init.h"
#endif

#if DXX_USE_EDITOR
#include "editor/editor.h"
#endif

#include "compiler-range_for.h"
#include "d_enumerate.h"
#include "d_levelstate.h"
#include "d_range.h"
#include "partial_range.h"
#include "d_zip.h"
#include <utility>

#if defined(DXX_BUILD_DESCENT_I)
#define STATE_VERSION 7
#define STATE_MATCEN_VERSION 25 // specific version of metcen info written into D1 savegames. Currenlty equal to GAME_VERSION (see gamesave.cpp). If changed, then only along with STATE_VERSION.
#define STATE_COMPATIBLE_VERSION 6
#elif defined(DXX_BUILD_DESCENT_II)
#define STATE_VERSION 22
#define STATE_COMPATIBLE_VERSION 20
#endif
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

#define THUMBNAIL_W 100
#define THUMBNAIL_H 50

constexpr char dgss_id[4] = {'D', 'G', 'S', 'S'};

unsigned state_game_id;

namespace dcx {
namespace {
constexpr unsigned NUM_SAVES = d_game_unique_state::MAXIMUM_SAVE_SLOTS.value;

struct relocated_player_data
{
	fix shields;
	int16_t num_robots_level;
	int16_t num_robots_total;
	uint16_t hostages_total;
	uint8_t hostages_level;
};

struct savegame_mission_path {
	std::array<char, 9> original;
	std::array<char, DXX_MAX_MISSION_PATH_LENGTH> full;
};

enum class savegame_mission_name_abi : uint8_t
{
	original,
	pathname,
};

static_assert(sizeof(savegame_mission_path) == sizeof(savegame_mission_path::original) + sizeof(savegame_mission_path::full), "padding error");

}
}

// Following functions convert object to object_rw and back to be written to/read from Savegames. Mostly object differs to object_rw in terms of timer values (fix/fix64). as we reset GameTime64 for writing so it can fit into fix it's not necessary to increment savegame version. But if we once store something else into object which might be useful after restoring, it might be handy to increment Savegame version and actually store these new infos.
// turn object to object_rw to be saved to Savegame.
namespace dsx {

static void state_object_to_object_rw(const object &obj, object_rw *const obj_rw)
{
	const auto otype = obj.type;
	obj_rw->type = otype;
	obj_rw->signature     = static_cast<uint16_t>(obj.signature);
	obj_rw->id            = obj.id;
	obj_rw->next          = obj.next;
	obj_rw->prev          = obj.prev;
	obj_rw->control_source  = static_cast<uint8_t>(obj.control_source);
	obj_rw->movement_source = static_cast<uint8_t>(obj.movement_source);
	obj_rw->render_type   = obj.render_type;
	obj_rw->flags         = obj.flags;
	obj_rw->segnum        = obj.segnum;
	obj_rw->attached_obj  = obj.attached_obj;
	obj_rw->pos.x         = obj.pos.x;
	obj_rw->pos.y         = obj.pos.y;
	obj_rw->pos.z         = obj.pos.z;
	obj_rw->orient.rvec.x = obj.orient.rvec.x;
	obj_rw->orient.rvec.y = obj.orient.rvec.y;
	obj_rw->orient.rvec.z = obj.orient.rvec.z;
	obj_rw->orient.fvec.x = obj.orient.fvec.x;
	obj_rw->orient.fvec.y = obj.orient.fvec.y;
	obj_rw->orient.fvec.z = obj.orient.fvec.z;
	obj_rw->orient.uvec.x = obj.orient.uvec.x;
	obj_rw->orient.uvec.y = obj.orient.uvec.y;
	obj_rw->orient.uvec.z = obj.orient.uvec.z;
	obj_rw->size          = obj.size;
	obj_rw->shields       = obj.shields;
	obj_rw->last_pos    = obj.pos;
	obj_rw->contains_type = obj.contains_type;
	obj_rw->contains_id   = obj.contains_id;
	obj_rw->contains_count= obj.contains_count;
	obj_rw->matcen_creator= obj.matcen_creator;
	obj_rw->lifeleft      = obj.lifeleft;

	switch (typename object::movement_type{obj_rw->movement_source})
	{
		case object::movement_type::None:
			obj_rw->mtype = {};
			break;
		case object::movement_type::physics:
			obj_rw->mtype.phys_info.velocity.x  = obj.mtype.phys_info.velocity.x;
			obj_rw->mtype.phys_info.velocity.y  = obj.mtype.phys_info.velocity.y;
			obj_rw->mtype.phys_info.velocity.z  = obj.mtype.phys_info.velocity.z;
			obj_rw->mtype.phys_info.thrust.x    = obj.mtype.phys_info.thrust.x;
			obj_rw->mtype.phys_info.thrust.y    = obj.mtype.phys_info.thrust.y;
			obj_rw->mtype.phys_info.thrust.z    = obj.mtype.phys_info.thrust.z;
			obj_rw->mtype.phys_info.mass        = obj.mtype.phys_info.mass;
			obj_rw->mtype.phys_info.drag        = obj.mtype.phys_info.drag;
			obj_rw->mtype.phys_info.obsolete_brakes = 0;
			obj_rw->mtype.phys_info.rotvel.x    = obj.mtype.phys_info.rotvel.x;
			obj_rw->mtype.phys_info.rotvel.y    = obj.mtype.phys_info.rotvel.y;
			obj_rw->mtype.phys_info.rotvel.z    = obj.mtype.phys_info.rotvel.z;
			obj_rw->mtype.phys_info.rotthrust.x = obj.mtype.phys_info.rotthrust.x;
			obj_rw->mtype.phys_info.rotthrust.y = obj.mtype.phys_info.rotthrust.y;
			obj_rw->mtype.phys_info.rotthrust.z = obj.mtype.phys_info.rotthrust.z;
			obj_rw->mtype.phys_info.turnroll    = obj.mtype.phys_info.turnroll;
			obj_rw->mtype.phys_info.flags       = obj.mtype.phys_info.flags;
			break;
			
		case object::movement_type::spinning:
			obj_rw->mtype.spin_rate.x = obj.mtype.spin_rate.x;
			obj_rw->mtype.spin_rate.y = obj.mtype.spin_rate.y;
			obj_rw->mtype.spin_rate.z = obj.mtype.spin_rate.z;
			break;
	}
	
	switch (typename object::control_type{obj_rw->control_source})
	{
		case object::control_type::weapon:
			obj_rw->ctype.laser_info.parent_type      = obj.ctype.laser_info.parent_type;
			obj_rw->ctype.laser_info.parent_num       = obj.ctype.laser_info.parent_num;
			obj_rw->ctype.laser_info.parent_signature = static_cast<uint16_t>(obj.ctype.laser_info.parent_signature);
			if (obj.ctype.laser_info.creation_time - GameTime64 < F1_0*(-18000))
				obj_rw->ctype.laser_info.creation_time = F1_0*(-18000);
			else
				obj_rw->ctype.laser_info.creation_time = obj.ctype.laser_info.creation_time - GameTime64;
			obj_rw->ctype.laser_info.last_hitobj      = obj.ctype.laser_info.get_last_hitobj();
			obj_rw->ctype.laser_info.track_goal       = obj.ctype.laser_info.track_goal;
			obj_rw->ctype.laser_info.multiplier       = obj.ctype.laser_info.multiplier;
			break;
			
		case object::control_type::explosion:
			obj_rw->ctype.expl_info.spawn_time    = obj.ctype.expl_info.spawn_time;
			obj_rw->ctype.expl_info.delete_time   = obj.ctype.expl_info.delete_time;
			obj_rw->ctype.expl_info.delete_objnum = obj.ctype.expl_info.delete_objnum;
			obj_rw->ctype.expl_info.attach_parent = obj.ctype.expl_info.attach_parent;
			obj_rw->ctype.expl_info.prev_attach   = obj.ctype.expl_info.prev_attach;
			obj_rw->ctype.expl_info.next_attach   = obj.ctype.expl_info.next_attach;
			break;
			
		case object::control_type::ai:
		{
			int i;
			obj_rw->ctype.ai_info.behavior               = static_cast<uint8_t>(obj.ctype.ai_info.behavior);
			for (i = 0; i < MAX_AI_FLAGS; i++)
				obj_rw->ctype.ai_info.flags[i]       = obj.ctype.ai_info.flags[i]; 
			obj_rw->ctype.ai_info.hide_segment           = obj.ctype.ai_info.hide_segment;
			obj_rw->ctype.ai_info.hide_index             = obj.ctype.ai_info.hide_index;
			obj_rw->ctype.ai_info.path_length            = obj.ctype.ai_info.path_length;
			obj_rw->ctype.ai_info.cur_path_index         = obj.ctype.ai_info.cur_path_index;
			obj_rw->ctype.ai_info.danger_laser_num       = obj.ctype.ai_info.danger_laser_num;
			if (obj.ctype.ai_info.danger_laser_num != object_none)
				obj_rw->ctype.ai_info.danger_laser_signature = static_cast<uint16_t>(obj.ctype.ai_info.danger_laser_signature);
			else
				obj_rw->ctype.ai_info.danger_laser_signature = 0;
#if defined(DXX_BUILD_DESCENT_I)
			obj_rw->ctype.ai_info.follow_path_start_seg  = segment_none;
			obj_rw->ctype.ai_info.follow_path_end_seg    = segment_none;
#elif defined(DXX_BUILD_DESCENT_II)
			obj_rw->ctype.ai_info.dying_sound_playing    = obj.ctype.ai_info.dying_sound_playing;
			if (obj.ctype.ai_info.dying_start_time == 0) // if bot not dead, anything but 0 will kill it
				obj_rw->ctype.ai_info.dying_start_time = 0;
			else
				obj_rw->ctype.ai_info.dying_start_time = obj.ctype.ai_info.dying_start_time - GameTime64;
#endif
			break;
		}
			
		case object::control_type::light:
			obj_rw->ctype.light_info.intensity = obj.ctype.light_info.intensity;
			break;
			
		case object::control_type::powerup:
			obj_rw->ctype.powerup_info.count         = obj.ctype.powerup_info.count;
#if defined(DXX_BUILD_DESCENT_II)
			if (obj.ctype.powerup_info.creation_time - GameTime64 < F1_0*(-18000))
				obj_rw->ctype.powerup_info.creation_time = F1_0*(-18000);
			else
				obj_rw->ctype.powerup_info.creation_time = obj.ctype.powerup_info.creation_time - GameTime64;
			obj_rw->ctype.powerup_info.flags         = obj.ctype.powerup_info.flags;
#endif
			break;
		case object::control_type::None:
		case object::control_type::flying:
		case object::control_type::slew:
		case object::control_type::flythrough:
		case object::control_type::repaircen:
		case object::control_type::morph:
		case object::control_type::debris:
		case object::control_type::remote:
		default:
			break;
	}

	switch (obj_rw->render_type)
	{
		case RT_MORPH:
		case RT_POLYOBJ:
		case RT_NONE: // HACK below
		{
			int i;
			if (obj.render_type == RT_NONE && obj.type != OBJ_GHOST) // HACK: when a player is dead or not connected yet, clients still expect to get polyobj data - even if render_type == RT_NONE at this time. Here it's not important, but it might be for Multiplayer Savegames.
				break;
			obj_rw->rtype.pobj_info.model_num                = obj.rtype.pobj_info.model_num;
			for (i=0;i<MAX_SUBMODELS;i++)
			{
				obj_rw->rtype.pobj_info.anim_angles[i].p = obj.rtype.pobj_info.anim_angles[i].p;
				obj_rw->rtype.pobj_info.anim_angles[i].b = obj.rtype.pobj_info.anim_angles[i].b;
				obj_rw->rtype.pobj_info.anim_angles[i].h = obj.rtype.pobj_info.anim_angles[i].h;
			}
			obj_rw->rtype.pobj_info.subobj_flags             = obj.rtype.pobj_info.subobj_flags;
			obj_rw->rtype.pobj_info.tmap_override            = obj.rtype.pobj_info.tmap_override;
			obj_rw->rtype.pobj_info.alt_textures             = obj.rtype.pobj_info.alt_textures;
			break;
		}
			
		case RT_WEAPON_VCLIP:
		case RT_HOSTAGE:
		case RT_POWERUP:
		case RT_FIREBALL:
			obj_rw->rtype.vclip_info.vclip_num = obj.rtype.vclip_info.vclip_num;
			obj_rw->rtype.vclip_info.frametime = obj.rtype.vclip_info.frametime;
			obj_rw->rtype.vclip_info.framenum  = obj.rtype.vclip_info.framenum;
			break;
			
		case RT_LASER:
			break;
			
	}
}

// turn object_rw to object after reading from Savegame
static void state_object_rw_to_object(const object_rw *const obj_rw, object &obj)
{
	obj = {};
	DXX_POISON_VAR(obj, 0xfd);
	set_object_type(obj, obj_rw->type);
	if (obj.type == OBJ_NONE)
	{
		obj.signature = object_signature_t{0};
		return;
	}

	obj.signature     = object_signature_t{static_cast<uint16_t>(obj_rw->signature)};
	obj.id            = obj_rw->id;
	obj.next          = obj_rw->next;
	obj.prev          = obj_rw->prev;
	obj.control_source  = typename object::control_type{obj_rw->control_source};
	obj.movement_source  = typename object::movement_type{obj_rw->movement_source};
	const auto render_type = obj_rw->render_type;
	if (valid_render_type(render_type))
		obj.render_type = render_type_t{render_type};
	else
	{
		con_printf(CON_URGENT, "save file used bogus render type %#x for object %p; using none instead", render_type, &obj);
		obj.render_type = RT_NONE;
	}
	obj.flags         = obj_rw->flags;
	obj.segnum        = obj_rw->segnum;
	obj.attached_obj  = obj_rw->attached_obj;
	obj.pos.x         = obj_rw->pos.x;
	obj.pos.y         = obj_rw->pos.y;
	obj.pos.z         = obj_rw->pos.z;
	obj.orient.rvec.x = obj_rw->orient.rvec.x;
	obj.orient.rvec.y = obj_rw->orient.rvec.y;
	obj.orient.rvec.z = obj_rw->orient.rvec.z;
	obj.orient.fvec.x = obj_rw->orient.fvec.x;
	obj.orient.fvec.y = obj_rw->orient.fvec.y;
	obj.orient.fvec.z = obj_rw->orient.fvec.z;
	obj.orient.uvec.x = obj_rw->orient.uvec.x;
	obj.orient.uvec.y = obj_rw->orient.uvec.y;
	obj.orient.uvec.z = obj_rw->orient.uvec.z;
	obj.size          = obj_rw->size;
	obj.shields       = obj_rw->shields;
	obj.contains_type = obj_rw->contains_type;
	obj.contains_id   = obj_rw->contains_id;
	obj.contains_count= obj_rw->contains_count;
	obj.matcen_creator= obj_rw->matcen_creator;
	obj.lifeleft      = obj_rw->lifeleft;
	
	switch (obj.movement_source)
	{
		case object::movement_type::None:
			break;
		case object::movement_type::physics:
			obj.mtype.phys_info.velocity.x  = obj_rw->mtype.phys_info.velocity.x;
			obj.mtype.phys_info.velocity.y  = obj_rw->mtype.phys_info.velocity.y;
			obj.mtype.phys_info.velocity.z  = obj_rw->mtype.phys_info.velocity.z;
			obj.mtype.phys_info.thrust.x    = obj_rw->mtype.phys_info.thrust.x;
			obj.mtype.phys_info.thrust.y    = obj_rw->mtype.phys_info.thrust.y;
			obj.mtype.phys_info.thrust.z    = obj_rw->mtype.phys_info.thrust.z;
			obj.mtype.phys_info.mass        = obj_rw->mtype.phys_info.mass;
			obj.mtype.phys_info.drag        = obj_rw->mtype.phys_info.drag;
			obj.mtype.phys_info.rotvel.x    = obj_rw->mtype.phys_info.rotvel.x;
			obj.mtype.phys_info.rotvel.y    = obj_rw->mtype.phys_info.rotvel.y;
			obj.mtype.phys_info.rotvel.z    = obj_rw->mtype.phys_info.rotvel.z;
			obj.mtype.phys_info.rotthrust.x = obj_rw->mtype.phys_info.rotthrust.x;
			obj.mtype.phys_info.rotthrust.y = obj_rw->mtype.phys_info.rotthrust.y;
			obj.mtype.phys_info.rotthrust.z = obj_rw->mtype.phys_info.rotthrust.z;
			obj.mtype.phys_info.turnroll    = obj_rw->mtype.phys_info.turnroll;
			obj.mtype.phys_info.flags       = obj_rw->mtype.phys_info.flags;
			break;
			
		case object::movement_type::spinning:
			obj.mtype.spin_rate.x = obj_rw->mtype.spin_rate.x;
			obj.mtype.spin_rate.y = obj_rw->mtype.spin_rate.y;
			obj.mtype.spin_rate.z = obj_rw->mtype.spin_rate.z;
			break;
	}
	
	switch (obj.control_source)
	{
		case object::control_type::weapon:
			obj.ctype.laser_info.parent_type      = obj_rw->ctype.laser_info.parent_type;
			obj.ctype.laser_info.parent_num       = obj_rw->ctype.laser_info.parent_num;
			obj.ctype.laser_info.parent_signature = object_signature_t{static_cast<uint16_t>(obj_rw->ctype.laser_info.parent_signature)};
			obj.ctype.laser_info.creation_time    = obj_rw->ctype.laser_info.creation_time;
			obj.ctype.laser_info.reset_hitobj(obj_rw->ctype.laser_info.last_hitobj);
			obj.ctype.laser_info.track_goal       = obj_rw->ctype.laser_info.track_goal;
			obj.ctype.laser_info.multiplier       = obj_rw->ctype.laser_info.multiplier;
#if defined(DXX_BUILD_DESCENT_II)
			obj.ctype.laser_info.last_afterburner_time = 0;
#endif
			break;
			
		case object::control_type::explosion:
			obj.ctype.expl_info.spawn_time    = obj_rw->ctype.expl_info.spawn_time;
			obj.ctype.expl_info.delete_time   = obj_rw->ctype.expl_info.delete_time;
			obj.ctype.expl_info.delete_objnum = obj_rw->ctype.expl_info.delete_objnum;
			obj.ctype.expl_info.attach_parent = obj_rw->ctype.expl_info.attach_parent;
			obj.ctype.expl_info.prev_attach   = obj_rw->ctype.expl_info.prev_attach;
			obj.ctype.expl_info.next_attach   = obj_rw->ctype.expl_info.next_attach;
			break;
			
		case object::control_type::ai:
		{
			int i;
			obj.ctype.ai_info.behavior               = static_cast<ai_behavior>(obj_rw->ctype.ai_info.behavior);
			for (i = 0; i < MAX_AI_FLAGS; i++)
				obj.ctype.ai_info.flags[i]       = obj_rw->ctype.ai_info.flags[i]; 
			obj.ctype.ai_info.hide_segment           = obj_rw->ctype.ai_info.hide_segment;
			obj.ctype.ai_info.hide_index             = obj_rw->ctype.ai_info.hide_index;
			obj.ctype.ai_info.path_length            = obj_rw->ctype.ai_info.path_length;
			obj.ctype.ai_info.cur_path_index         = obj_rw->ctype.ai_info.cur_path_index;
			obj.ctype.ai_info.danger_laser_num       = obj_rw->ctype.ai_info.danger_laser_num;
			if (obj.ctype.ai_info.danger_laser_num != object_none)
				obj.ctype.ai_info.danger_laser_signature = object_signature_t{static_cast<uint16_t>(obj_rw->ctype.ai_info.danger_laser_signature)};
#if defined(DXX_BUILD_DESCENT_I)
#elif defined(DXX_BUILD_DESCENT_II)
			obj.ctype.ai_info.dying_sound_playing    = obj_rw->ctype.ai_info.dying_sound_playing;
			obj.ctype.ai_info.dying_start_time       = obj_rw->ctype.ai_info.dying_start_time;
#endif
			break;
		}
			
		case object::control_type::light:
			obj.ctype.light_info.intensity = obj_rw->ctype.light_info.intensity;
			break;
			
		case object::control_type::powerup:
			obj.ctype.powerup_info.count         = obj_rw->ctype.powerup_info.count;
#if defined(DXX_BUILD_DESCENT_I)
			obj.ctype.powerup_info.creation_time = 0;
			obj.ctype.powerup_info.flags         = 0;
#elif defined(DXX_BUILD_DESCENT_II)
			obj.ctype.powerup_info.creation_time = obj_rw->ctype.powerup_info.creation_time;
			obj.ctype.powerup_info.flags         = obj_rw->ctype.powerup_info.flags;
#endif
			break;
		case object::control_type::cntrlcen:
		{
			if (obj.type == OBJ_GHOST)
			{
				/* Boss missions convert the reactor into OBJ_GHOST
				 * instead of freeing it.  Old releases (before
				 * ed46a05296f9d480f934d8c951c4755ebac1d5e7 ("Update
				 * control_type when ghosting reactor")) did not update
				 * `control_type`, so games saved by those releases have an
				 * object with obj->type == OBJ_GHOST and obj->control_source ==
				 * object::control_type::cntrlcen.  That inconsistency triggers an assertion down
				 * in `calc_controlcen_gun_point` because obj->type !=
				 * OBJ_CNTRLCEN.
				 *
				 * Add a special case here to correct this
				 * inconsistency.
				 */
				obj.control_source = object::control_type::None;
				break;
			}
			// gun points of reactor now part of the object but of course not saved in object_rw and overwritten due to reset_objects(). Let's just recompute them.
			calc_controlcen_gun_point(obj);
			break;
		}
		case object::control_type::None:
		case object::control_type::flying:
		case object::control_type::slew:
		case object::control_type::flythrough:
		case object::control_type::repaircen:
		case object::control_type::morph:
		case object::control_type::debris:
		case object::control_type::remote:
		default:
			break;
	}
	
	switch (obj.render_type)
	{
		case RT_MORPH:
		case RT_POLYOBJ:
		case RT_NONE: // HACK below
		{
			int i;
			if (obj.render_type == RT_NONE && obj.type != OBJ_GHOST) // HACK: when a player is dead or not connected yet, clients still expect to get polyobj data - even if render_type == RT_NONE at this time. Here it's not important, but it might be for Multiplayer Savegames.
				break;
			obj.rtype.pobj_info.model_num                = obj_rw->rtype.pobj_info.model_num;
			for (i=0;i<MAX_SUBMODELS;i++)
			{
				obj.rtype.pobj_info.anim_angles[i].p = obj_rw->rtype.pobj_info.anim_angles[i].p;
				obj.rtype.pobj_info.anim_angles[i].b = obj_rw->rtype.pobj_info.anim_angles[i].b;
				obj.rtype.pobj_info.anim_angles[i].h = obj_rw->rtype.pobj_info.anim_angles[i].h;
			}
			obj.rtype.pobj_info.subobj_flags             = obj_rw->rtype.pobj_info.subobj_flags;
			obj.rtype.pobj_info.tmap_override            = obj_rw->rtype.pobj_info.tmap_override;
			obj.rtype.pobj_info.alt_textures             = obj_rw->rtype.pobj_info.alt_textures;
			break;
		}
			
		case RT_WEAPON_VCLIP:
		case RT_HOSTAGE:
		case RT_POWERUP:
		case RT_FIREBALL:
			obj.rtype.vclip_info.vclip_num = obj_rw->rtype.vclip_info.vclip_num;
			obj.rtype.vclip_info.frametime = obj_rw->rtype.vclip_info.frametime;
			obj.rtype.vclip_info.framenum  = obj_rw->rtype.vclip_info.framenum;
			break;
			
		case RT_LASER:
			break;
			
	}
}

deny_save_result deny_save_game(fvcobjptr &vcobjptr, const d_level_unique_control_center_state &LevelUniqueControlCenterState, const d_game_unique_state &GameUniqueState)
{
#if defined(DXX_BUILD_DESCENT_I)
	(void)GameUniqueState;
#elif defined(DXX_BUILD_DESCENT_II)
	if (Current_level_num < 0)
		return deny_save_result::denied;
	if (GameUniqueState.Final_boss_countdown_time)		//don't allow save while final boss is dying
		return deny_save_result::denied;
#endif
	return deny_save_game(vcobjptr, LevelUniqueControlCenterState);
}

namespace {

// Following functions convert player to player_rw and back to be written to/read from Savegames. player only differ to player_rw in terms of timer values (fix/fix64). as we reset GameTime64 for writing so it can fit into fix it's not necessary to increment savegame version. But if we once store something else into object which might be useful after restoring, it might be handy to increment Savegame version and actually store these new infos.
// turn player to player_rw to be saved to Savegame.
static void state_player_to_player_rw(const relocated_player_data &rpd, const player *pl, player_rw *pl_rw, const player_info &pl_info)
{
	int i=0;
	pl_rw->callsign = pl->callsign;
	memset(pl_rw->net_address, 0, 6);
	pl_rw->connected                 = pl->connected;
	pl_rw->objnum                    = pl->objnum;
	pl_rw->n_packets_got             = 0;
	pl_rw->n_packets_sent            = 0;
	pl_rw->flags                     = pl_info.powerup_flags.get_player_flags();
	pl_rw->energy                    = pl_info.energy;
	pl_rw->shields                   = rpd.shields;
	/*
	 * The savegame only allocates a uint8_t for this value.  If the
	 * player has exceeded the maximum representable value, cap at that
	 * value.  This is better than truncating the value, since the
	 * player will get to keep more lives this way than with truncation.
	 */
	pl_rw->lives                     = std::min<unsigned>(pl->lives, std::numeric_limits<uint8_t>::max());
	pl_rw->level                     = pl->level;
	pl_rw->laser_level               = static_cast<uint8_t>(pl_info.laser_level);
	pl_rw->starting_level            = pl->starting_level;
	pl_rw->killer_objnum             = pl_info.killer_objnum;
	pl_rw->primary_weapon_flags      = pl_info.primary_weapon_flags;
#if defined(DXX_BUILD_DESCENT_I)
	// make sure no side effects for Mac demo
	pl_rw->secondary_weapon_flags    = 0x0f | (pl_info.secondary_ammo[MEGA_INDEX] > 0) << MEGA_INDEX;
#elif defined(DXX_BUILD_DESCENT_II)
	// make sure no side effects for PC demo
	pl_rw->secondary_weapon_flags    = 0xef | (pl_info.secondary_ammo[MEGA_INDEX] > 0) << MEGA_INDEX
											| (pl_info.secondary_ammo[SMISSILE4_INDEX] > 0) << SMISSILE4_INDEX	// mercury missile
											| (pl_info.secondary_ammo[SMISSILE5_INDEX] > 0) << SMISSILE5_INDEX;	// earthshaker missile
#endif
	pl_rw->obsolete_primary_ammo = {};
	pl_rw->vulcan_ammo   = pl_info.vulcan_ammo;
	for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
		pl_rw->secondary_ammo[i] = pl_info.secondary_ammo[i];
#if defined(DXX_BUILD_DESCENT_II)
	pl_rw->pad = 0;
#endif
	pl_rw->last_score                = pl_info.mission.last_score;
	pl_rw->score                     = pl_info.mission.score;
	pl_rw->time_level                = pl->time_level;
	pl_rw->time_total                = pl->time_total;
	if (!(pl_info.powerup_flags & PLAYER_FLAGS_CLOAKED) || pl_info.cloak_time - GameTime64 < F1_0*(-18000))
		pl_rw->cloak_time        = F1_0*(-18000);
	else
		pl_rw->cloak_time        = pl_info.cloak_time - GameTime64;
	if (!(pl_info.powerup_flags & PLAYER_FLAGS_INVULNERABLE) || pl_info.invulnerable_time - GameTime64 < F1_0*(-18000))
		pl_rw->invulnerable_time = F1_0*(-18000);
	else
		pl_rw->invulnerable_time = pl_info.invulnerable_time - GameTime64;
#if defined(DXX_BUILD_DESCENT_II)
	pl_rw->KillGoalCount             = pl_info.KillGoalCount;
#endif
	pl_rw->net_killed_total          = pl_info.net_killed_total;
	pl_rw->net_kills_total           = pl_info.net_kills_total;
	pl_rw->num_kills_level           = pl->num_kills_level;
	pl_rw->num_kills_total           = pl->num_kills_total;
	pl_rw->num_robots_level          = LevelUniqueObjectState.accumulated_robots;
	pl_rw->num_robots_total          = GameUniqueState.accumulated_robots;
	pl_rw->hostages_rescued_total    = pl_info.mission.hostages_rescued_total;
	pl_rw->hostages_total            = GameUniqueState.total_hostages;
	pl_rw->hostages_on_board         = pl_info.mission.hostages_on_board;
	pl_rw->hostages_level            = LevelUniqueObjectState.total_hostages;
	pl_rw->homing_object_dist        = pl_info.homing_object_dist;
	pl_rw->hours_level               = pl->hours_level;
	pl_rw->hours_total               = pl->hours_total;
}

// turn player_rw to player after reading from Savegame

static void state_player_rw_to_player(const player_rw *pl_rw, player *pl, player_info &pl_info, relocated_player_data &rpd)
{
	int i=0;
	pl->callsign = pl_rw->callsign;
	pl->connected                 = pl_rw->connected;
	pl->objnum                    = pl_rw->objnum;
	pl_info.powerup_flags         = player_flags(pl_rw->flags);
	pl_info.energy                = pl_rw->energy;
	rpd.shields                    = pl_rw->shields;
	pl->lives                     = pl_rw->lives;
	pl->level                     = pl_rw->level;
	pl_info.laser_level           = laser_level{pl_rw->laser_level};
	pl->starting_level            = pl_rw->starting_level;
	pl_info.killer_objnum         = pl_rw->killer_objnum;
	pl_info.primary_weapon_flags  = pl_rw->primary_weapon_flags;
	pl_info.vulcan_ammo   = pl_rw->vulcan_ammo;
	for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
		pl_info.secondary_ammo[i] = pl_rw->secondary_ammo[i];
	pl_info.mission.last_score                = pl_rw->last_score;
	pl_info.mission.score                     = pl_rw->score;
	pl->time_level                = pl_rw->time_level;
	pl->time_total                = pl_rw->time_total;
	pl_info.cloak_time                = pl_rw->cloak_time;
	pl_info.invulnerable_time         = pl_rw->invulnerable_time;
#if defined(DXX_BUILD_DESCENT_I)
	pl_info.KillGoalCount = 0;
#elif defined(DXX_BUILD_DESCENT_II)
	pl_info.KillGoalCount             = pl_rw->KillGoalCount;
#endif
	pl_info.net_killed_total          = pl_rw->net_killed_total;
	pl_info.net_kills_total           = pl_rw->net_kills_total;
	pl->num_kills_level           = pl_rw->num_kills_level;
	pl->num_kills_total           = pl_rw->num_kills_total;
	rpd.num_robots_level = pl_rw->num_robots_level;
	rpd.num_robots_total = pl_rw->num_robots_total;
	pl_info.mission.hostages_rescued_total    = pl_rw->hostages_rescued_total;
	rpd.hostages_total            = pl_rw->hostages_total;
	pl_info.mission.hostages_on_board         = pl_rw->hostages_on_board;
	rpd.hostages_level            = pl_rw->hostages_level;
	pl_info.homing_object_dist        = pl_rw->homing_object_dist;
	pl->hours_level               = pl_rw->hours_level;
	pl->hours_total               = pl_rw->hours_total;
}

static void state_write_player(PHYSFS_File *fp, const player &pl, const relocated_player_data &rpd, const player_info &pl_info)
{
	player_rw pl_rw;
	state_player_to_player_rw(rpd, &pl, &pl_rw, pl_info);
	PHYSFS_write(fp, &pl_rw, sizeof(pl_rw), 1);
}

static void state_read_player(PHYSFS_File *fp, player &pl, int swap, player_info &pl_info, relocated_player_data &rpd)
{
	player_rw pl_rw;
	PHYSFS_read(fp, &pl_rw, sizeof(pl_rw), 1);
	player_rw_swap(&pl_rw, swap);
	state_player_rw_to_player(&pl_rw, &pl, pl_info, rpd);
}

void state_format_savegame_filename(d_game_unique_state::savegame_file_path &filename, const unsigned i)
{
	snprintf(filename.data(), filename.size(), PLAYER_DIRECTORY_STRING("%.8s.%cg%x"), static_cast<const char *>(InterfaceUniqueState.PilotName), (Game_mode & GM_MULTI_COOP) ? 'm' : 's', i);
}

void state_autosave_game(const int multiplayer)
{
	d_game_unique_state::savegame_description desc;
	const char *p;
	time_t t = time(nullptr);
	if (struct tm *ptm = (t == -1) ? nullptr : localtime(&t))
	{
		p = desc.data();
		strftime(desc.data(), desc.size(), "auto %m-%d %H:%M:%S", ptm);
	}
	else
		p = "<autosave>";
	if (multiplayer)
	{
		const auto &&player_range = partial_const_range(Players, N_players);
		multi_execute_save_game(d_game_unique_state::save_slot::_autosave, desc, player_range);
	}
	else
	{
		d_game_unique_state::savegame_file_path filename;
		state_format_savegame_filename(filename, NUM_SAVES - 1);
		if (state_save_all_sub(filename.data(), p))
			con_printf(CON_NORMAL, "Autosave written to \"%s\"", filename.data());
	}
}

}

void state_set_immediate_autosave(d_game_unique_state &GameUniqueState)
{
	GameUniqueState.Next_autosave = {};
}

void state_set_next_autosave(d_game_unique_state &GameUniqueState, const std::chrono::steady_clock::time_point now, const autosave_interval_type interval)
{
	GameUniqueState.Next_autosave = now + interval;
}

void state_set_next_autosave(d_game_unique_state &GameUniqueState, const autosave_interval_type interval)
{
	const auto now = std::chrono::steady_clock::now();
	state_set_next_autosave(GameUniqueState, now, interval);
}

void state_poll_autosave_game(d_game_unique_state &GameUniqueState, const d_level_unique_object_state &LevelUniqueObjectState)
{
	const auto multiplayer = Game_mode & GM_MULTI;
	if (multiplayer && !multi_i_am_master())
		return;
	const auto interval = (multiplayer ? static_cast<const d_gameplay_options &>(Netgame.MPGameplayOptions) : PlayerCfg.SPGameplayOptions).AutosaveInterval;
	if (interval.count() <= 0)
		/* Autosave is disabled */
		return;
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;
	auto &Objects = LevelUniqueObjectState.Objects;
	if (deny_save_game(Objects.vcptr, LevelUniqueControlCenterState, GameUniqueState) != deny_save_result::allowed)
		return;
	const auto now = std::chrono::steady_clock::now();
	if (now < GameUniqueState.Next_autosave)
		return;
	state_set_next_autosave(GameUniqueState, now, interval);
	state_autosave_game(multiplayer);
}

}

namespace {

//-------------------------------------------------------------------
struct state_userdata
{
	static constexpr std::integral_constant<unsigned, 1> decorative_item_count = {};
	unsigned citem;
	std::array<grs_bitmap_ptr, NUM_SAVES> sc_bmp;
};

}

static int state_callback(newmenu *menu,const d_event &event, state_userdata *const userdata)
{
	std::array<grs_bitmap_ptr, NUM_SAVES> &sc_bmp = userdata->sc_bmp;
	newmenu_item *items = newmenu_get_items(menu);
	unsigned citem;
	if (event.type == EVENT_NEWMENU_SELECTED)
		userdata->citem = static_cast<const d_select_event &>(event).citem;
	else if (event.type == EVENT_NEWMENU_DRAW && (citem = newmenu_get_citem(menu)) > 0)
	{
		if (sc_bmp[citem - userdata->decorative_item_count])
		{
			const auto &&fspacx = FSPACX();
			const auto &&fspacy = FSPACY();
#if !DXX_USE_OGL
			auto temp_canv = gr_create_canvas(fspacx(THUMBNAIL_W), fspacy(THUMBNAIL_H));
#else
			auto temp_canv = gr_create_canvas(THUMBNAIL_W*2,(THUMBNAIL_H*24/10));
#endif
			const std::array<grs_point, 3> vertbuf{{
				{0,0},
				{0,0},
				{i2f(THUMBNAIL_W*2),i2f(THUMBNAIL_H*24/10)}
			}};
			scale_bitmap(*sc_bmp[citem-1].get(), vertbuf, 0, temp_canv->cv_bitmap);
#if !DXX_USE_OGL
			gr_bitmap(*grd_curcanv, (grd_curcanv->cv_bitmap.bm_w / 2) - fspacx(THUMBNAIL_W / 2), items[0].y - 3, temp_canv->cv_bitmap);
#else
			ogl_ubitmapm_cs(*grd_curcanv, (grd_curcanv->cv_bitmap.bm_w / 2) - fspacx(THUMBNAIL_W / 2), items[0].y - fspacy(3), fspacx(THUMBNAIL_W), fspacy(THUMBNAIL_H), temp_canv->cv_bitmap, ogl_colors::white, F1_0);
#endif
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

namespace dsx {

/* Present a menu for selection of a savegame filename.
 * For saving, dsc should be a pre-allocated buffer into which the new
 * savegame description will be stored.
 * For restoring, dsc should be NULL, in which case empty slots will not be
 * selectable and savagames descriptions will not be editable.
 */
static d_game_unique_state::save_slot state_get_savegame_filename(d_game_unique_state::savegame_file_path &fname, d_game_unique_state::savegame_description *const dsc, const char *const caption, const blind_save entry_blind)
{
	int version, nsaves;
	std::array<d_game_unique_state::savegame_file_path, NUM_SAVES> filename;
	std::array<d_game_unique_state::savegame_description, NUM_SAVES> desc;
	state_userdata userdata;
	constexpr auto decorative_item_count = userdata.decorative_item_count;
	std::array<newmenu_item, NUM_SAVES + decorative_item_count> m;
	auto &sc_bmp = userdata.sc_bmp;
	char id[4];
	int valid;

	nsaves=0;
	nm_set_item_text(m[0], "\n\n\n");
	/* Always subtract 1 for the fixed text leader.  Conditionally
	 * subtract another 1 if the call is for saving, since interactive
	 * saves should not access the last slot.  The last slot is reserved
	 * for autosaves.
	 */
	const unsigned max_slots_shown = (dsc ? m.size() - 1 : m.size()) - decorative_item_count;
	range_for (const unsigned i, xrange(max_slots_shown))
	{
		state_format_savegame_filename(filename[i], i);
		valid = 0;
		auto &mi = m[i + decorative_item_count];
		if (const auto fp = PHYSFSX_openReadBuffered(filename[i].data()))
		{
			//Read id
			PHYSFS_read(fp, id, sizeof(char) * 4, 1);
			if ( !memcmp( id, dgss_id, 4 )) {
				//Read version
				PHYSFS_read(fp, &version, sizeof(int), 1);
				// In case it's Coop, read state_game_id & callsign as well
				if (Game_mode & GM_MULTI_COOP)
				{
					PHYSFS_seek(fp, PHYSFS_tell(fp) + sizeof(PHYSFS_sint32) + sizeof(char)*CALLSIGN_LEN+1); // skip state_game_id, callsign
				}
				if ((version >= STATE_COMPATIBLE_VERSION) || (SWAPINT(version) >= STATE_COMPATIBLE_VERSION)) {
					// Read description
					PHYSFS_read(fp, desc[i].data(), desc[i].size(), 1);
					desc[i].back() = 0;
					if (!dsc)
						mi.type = nm_type::menu;
					// Read thumbnail
					sc_bmp[i] = gr_create_bitmap(THUMBNAIL_W,THUMBNAIL_H );
					PHYSFS_read(fp, sc_bmp[i]->get_bitmap_data(), THUMBNAIL_W * THUMBNAIL_H, 1);
#if defined(DXX_BUILD_DESCENT_II)
					if (version >= 9) {
						palette_array_t pal;
						PHYSFS_read(fp, &pal[0], sizeof(pal[0]), pal.size());
						gr_remap_bitmap_good(*sc_bmp[i].get(), pal, -1, -1);
					}
#endif
					nsaves++;
					valid = 1;
				}
			}
		} 
		mi.text = desc[i].data();
		if (!valid) {
			strcpy(desc[i].data(), TXT_EMPTY);
			if (!dsc)
				mi.type = nm_type::text;
		}
		if (dsc)
		{
			mi.type = nm_type::input_menu;
			auto &im = mi.imenu();
			im.text_len = desc[i].size() - 1;
		}
	}

	if (!dsc && nsaves < 1)
	{
		nm_messagebox_str(menu_title{nullptr}, nm_messagebox_tie(TXT_OK), menu_subtitle{"No saved games were found!"});
		return d_game_unique_state::save_slot::None;
	}

	const auto quicksave_selection = GameUniqueState.quicksave_selection;
	const auto valid_selection = [dsc](d_game_unique_state::save_slot selection) {
		return dsc
			? GameUniqueState.valid_save_slot(selection)
			: GameUniqueState.valid_load_slot(selection);
	};
	const auto blind = (entry_blind == blind_save::no || !valid_selection(quicksave_selection))
		/* If not a blind save, or if is a blind save and no slot picked, force to ::no */
		? blind_save::no
		/* otherwise, user's choice */
		: entry_blind;

	const auto choice = (blind != blind_save::no)
		? quicksave_selection
		: (
			userdata.citem = 0,
			newmenu_do2(menu_title{nullptr}, menu_subtitle{caption}, partial_range(m, max_slots_shown + decorative_item_count), state_callback, &userdata, (GameUniqueState.valid_save_slot(quicksave_selection) ? static_cast<unsigned>(quicksave_selection) : 0) + decorative_item_count, menu_filename{nullptr}),
			userdata.citem == 0
			? d_game_unique_state::save_slot::None
			: static_cast<d_game_unique_state::save_slot>(userdata.citem - decorative_item_count)
		);

	if (valid_selection(choice))
	{
		GameUniqueState.quicksave_selection = choice;
		const auto uchoice = static_cast<std::size_t>(choice);
		fname = filename[uchoice];
		if (dsc)
		{
			auto &d = desc[uchoice];
			if (d.front())
				*dsc = d;
			else
			{
				time_t t = time(nullptr);
				if (struct tm *ptm = (t == -1) ? nullptr : localtime(&t))
					strftime(dsc->data(), dsc->size(), "%m-%d %H:%M:%S", ptm);
				else
					strcpy(dsc->data(), "-no title-");
			}
		}
	}
	return choice;
}

d_game_unique_state::save_slot state_get_save_file(d_game_unique_state::savegame_file_path &fname, d_game_unique_state::savegame_description *const dsc, const blind_save blind_save)
{
	return state_get_savegame_filename(fname, dsc, "Save Game", blind_save);
}

d_game_unique_state::save_slot state_get_restore_file(d_game_unique_state::savegame_file_path &fname, blind_save blind_save)
{
	return state_get_savegame_filename(fname, NULL, "Select Game to Restore", blind_save);
}

#if defined(DXX_BUILD_DESCENT_I)
#elif defined(DXX_BUILD_DESCENT_II)

//	-----------------------------------------------------------------------------------
//	Imagine if C had a function to copy a file...
static int copy_file(const char *old_file, const char *new_file)
{
	int		buf_size;
	RAIIPHYSFS_File in_file{PHYSFS_openRead(old_file)};
	if (!in_file)
		return -2;
	RAIIPHYSFS_File out_file{PHYSFS_openWrite(new_file)};
	if (!out_file)
		return -1;

	buf_size = PHYSFS_fileLength(in_file);
	RAIIdmem<sbyte[]> buf;
	for (;;) {
		if (buf_size == 0)
			return -5;	// likely to be an empty file
		if (MALLOC(buf, sbyte[], buf_size))
			break;
		buf_size /= 2;
	}

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
	if (!out_file.close())
		return -4;

	return 0;
}

static void format_secret_sgc_filename(std::array<char, PATH_MAX> &fname, const d_game_unique_state::save_slot filenum)
{
	snprintf(fname.data(), fname.size(), PLAYER_DIRECTORY_STRING("%xsecret.sgc"), static_cast<unsigned>(filenum));
}
#endif

//	-----------------------------------------------------------------------------------
#if defined(DXX_BUILD_DESCENT_I)
int state_save_all(const blind_save blind_save)
#elif defined(DXX_BUILD_DESCENT_II)
int state_save_all(const secret_save secret, const blind_save blind_save)
#endif
{
#if defined(DXX_BUILD_DESCENT_I)
	static constexpr std::integral_constant<secret_save, secret_save::none> secret{};
#elif defined(DXX_BUILD_DESCENT_II)
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;
	if (Current_level_num < 0 && secret == secret_save::none)
	{
		HUD_init_message_literal(HM_DEFAULT,  "Can't save in secret level!" );
		return 0;
	}

	if (GameUniqueState.Final_boss_countdown_time)		//don't allow save while final boss is dying
		return 0;
#endif

	if ( Game_mode & GM_MULTI )
	{
		if (Game_mode & GM_MULTI_COOP)
			multi_initiate_save_game();
		return 0;
	}

#if defined(DXX_BUILD_DESCENT_II)
	//	If this is a secret save and the control center has been destroyed, don't allow
	//	return to the base level.
	if (secret != secret_save::none && LevelUniqueControlCenterState.Control_center_destroyed)
	{
		PHYSFS_delete(SECRETB_FILENAME);
		return 0;
	}
#endif

	d_game_unique_state::savegame_file_path filename_storage;
	const char *filename;
	d_game_unique_state::savegame_description desc{};
	d_game_unique_state::save_slot filenum = d_game_unique_state::save_slot::None;
	{
		pause_game_world_time p;

#if defined(DXX_BUILD_DESCENT_II)
	if (secret == secret_save::b) {
		filename = SECRETB_FILENAME;
	} else if (secret == secret_save::c) {
		filename = SECRETC_FILENAME;
	} else
#endif
	{
		filenum = state_get_save_file(filename_storage, &desc, blind_save);
		if (!GameUniqueState.valid_save_slot(filenum))
			return 0;
		filename = filename_storage.data();
	}
#if defined(DXX_BUILD_DESCENT_II)
	//	MK, 1/1/96
	//	Do special secret level stuff.
	//	If secret.sgc exists, then copy it to Nsecret.sgc (where N = filenum).
	//	If it doesn't exist, then delete Nsecret.sgc
	if (secret == secret_save::none && !(Game_mode & GM_MULTI_COOP)) {
		if (filenum != d_game_unique_state::save_slot::None)
		{
			std::array<char, PATH_MAX> fname;
			const auto temp_fname = fname.data();
			format_secret_sgc_filename(fname, filenum);
			if (PHYSFSX_exists(temp_fname,0))
			{
				if (!PHYSFS_delete(temp_fname))
					Error("Cannot delete file <%s>: %s", temp_fname, PHYSFS_getLastError());
			}

			if (PHYSFSX_exists(SECRETC_FILENAME,0))
			{
				const int rval = copy_file(SECRETC_FILENAME, temp_fname);
				Assert(rval == 0);	//	Oops, error copying secret.sgc to temp_fname!
				(void)rval;
			}
		}
	}
#endif
	}

	const int rval = state_save_all_sub(filename, desc.data());

	if (rval && secret == secret_save::none)
		HUD_init_message(HM_DEFAULT, "Game saved to \"%s\": \"%s\"", filename, desc.data());

	return rval;
}

int state_save_all_sub(const char *filename, const char *desc)
{
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	auto &vmobjptr = Objects.vmptr;
	auto &LevelUniqueMorphObjectState = LevelUniqueObjectState.MorphObjectState;
	auto &RobotCenters = LevelSharedRobotcenterState.RobotCenters;
	auto &Station = LevelUniqueFuelcenterState.Station;
	fix tmptime32 = 0;

	#ifndef NDEBUG
	if (CGameArg.SysUsePlayersDir && strncmp(filename, PLAYER_DIRECTORY_TEXT, sizeof(PLAYER_DIRECTORY_TEXT) - 1))
		Int3();
	#endif

	auto fp = PHYSFSX_openWriteBuffered(filename);
	if ( !fp ) {
		con_printf(CON_URGENT, "Failed to open %s: %s", filename, PHYSFS_getLastError());
		nm_messagebox_str(menu_title{nullptr}, nm_messagebox_tie(TXT_OK), menu_subtitle{"Error writing savegame.\nPossibly out of disk\nspace."});
		return 0;
	}

	pause_game_world_time p;

//Save id
	PHYSFS_write(fp, dgss_id, sizeof(char) * 4, 1);

//Save version
	{
		const int i = STATE_VERSION;
	PHYSFS_write(fp, &i, sizeof(int), 1);
	}

// Save Coop state_game_id and this Player's callsign. Oh the redundancy... we have this one later on but Coop games want to read this before loading a state so for easy access save this here, too
	if (Game_mode & GM_MULTI_COOP)
	{
		PHYSFS_write(fp, &state_game_id, sizeof(unsigned), 1);
		PHYSFS_write(fp, &get_local_player().callsign, sizeof(char)*CALLSIGN_LEN+1, 1);
	}

//Save description
	PHYSFS_write(fp, desc, 20, 1);

// Save the current screen shot...

	auto cnv = gr_create_canvas( THUMBNAIL_W, THUMBNAIL_H );
	{
		render_frame(*cnv, 0);

		{
#if DXX_USE_OGL
		RAIIdmem<uint8_t[]> buf;
		MALLOC(buf, uint8_t[], THUMBNAIL_W * THUMBNAIL_H * 4);
#if !DXX_USE_OGLES
		GLint gl_draw_buffer;
 		glGetIntegerv(GL_DRAW_BUFFER, &gl_draw_buffer);
 		glReadBuffer(gl_draw_buffer);
#endif
		glReadPixels(0, SHEIGHT - THUMBNAIL_H, THUMBNAIL_W, THUMBNAIL_H, GL_RGBA, GL_UNSIGNED_BYTE, buf.get());
		int k;
		k = THUMBNAIL_H;
		for (unsigned i = 0; i < THUMBNAIL_W * THUMBNAIL_H; i++)
		{
			int j;
			if (!(j = i % THUMBNAIL_W))
				k--;
			cnv->cv_bitmap.get_bitmap_data()[THUMBNAIL_W * k + j] =
				gr_find_closest_color(buf[4*i]/4, buf[4*i+1]/4, buf[4*i+2]/4);
		}
#endif
		}

		PHYSFS_write(fp, cnv->cv_bitmap.bm_data, THUMBNAIL_W * THUMBNAIL_H, 1);
#if defined(DXX_BUILD_DESCENT_II)
		PHYSFS_write(fp, &gr_palette[0], sizeof(gr_palette[0]), gr_palette.size());
#endif
	}

// Save the Between levels flag...
	{
		const int i = 0;
	PHYSFS_write(fp, &i, sizeof(int), 1);
	}

// Save the mission info...
	savegame_mission_path mission_pathname{};
#if defined(DXX_BUILD_DESCENT_II)
	mission_pathname.original[1] = static_cast<uint8_t>(Current_mission->descent_version);
#endif
	mission_pathname.original.back() = static_cast<uint8_t>(savegame_mission_name_abi::pathname);
	auto Current_mission_pathname = Current_mission->path.c_str();
	// Current_mission_filename is not necessarily 9 bytes long so for saving we use a proper string - preventing corruptions
	snprintf(mission_pathname.full.data(), mission_pathname.full.size(), "%s", Current_mission_pathname);
	PHYSFS_write(fp, &mission_pathname, sizeof(mission_pathname), 1);

//Save level info
	PHYSFS_write(fp, &Current_level_num, sizeof(int), 1);
	PHYSFS_write(fp, &Next_level_num, sizeof(int), 1);

//Save GameTime
// NOTE: GameTime now is GameTime64 with fix64 since GameTime could only last 9 hrs. To even help old Savegames, we do not increment Savegame version but rather RESET GameTime64 to 0 on every save! ALL variables based on GameTime64 now will get the current GameTime64 value substracted and saved to fix size as well.
	tmptime32 = 0;
	PHYSFS_write(fp, &tmptime32, sizeof(fix), 1);

//Save player info
	//PHYSFS_write(fp, &Players[Player_num], sizeof(player), 1);
	const auto &plrobj = get_local_plrobj();
	auto &player_info = plrobj.ctype.player_info;
	state_write_player(fp, get_local_player(), relocated_player_data{
		plrobj.shields,
		static_cast<int16_t>(LevelUniqueObjectState.accumulated_robots),
		static_cast<int16_t>(GameUniqueState.accumulated_robots),
		static_cast<uint16_t>(GameUniqueState.total_hostages),
		static_cast<uint8_t>(LevelUniqueObjectState.total_hostages)
		}, player_info);

// Save the current weapon info
	{
		int8_t v = static_cast<int8_t>(static_cast<primary_weapon_index_t>(player_info.Primary_weapon));
		PHYSFS_write(fp, &v, sizeof(int8_t), 1);
	}
	{
		int8_t v = static_cast<int8_t>(static_cast<secondary_weapon_index_t>(player_info.Secondary_weapon));
		PHYSFS_write(fp, &v, sizeof(int8_t), 1);
	}

// Save the difficulty level
	{
		const int Difficulty_level = GameUniqueState.Difficulty_level;
	PHYSFS_write(fp, &Difficulty_level, sizeof(int), 1);
	}

// Save cheats enabled
	PHYSFS_write(fp, &cheats.enabled, sizeof(int), 1);
#if defined(DXX_BUILD_DESCENT_I)
	PHYSFS_write(fp, &cheats.turbo, sizeof(int), 1);
#endif

//Finish all morph objects
	range_for (const auto &&objp, vmobjptr)
	{
		if (objp->type != OBJ_NONE && objp->render_type == RT_MORPH)
		{
			if (const auto umd = find_morph_data(LevelUniqueMorphObjectState, objp))
			{
				const auto md = umd->get();
				md->obj->control_source = md->morph_save_control_type;
				md->obj->movement_source = md->morph_save_movement_type;
				md->obj->render_type = RT_POLYOBJ;
				md->obj->mtype.phys_info = md->morph_save_phys_info;
				umd->reset();
			} else {						//maybe loaded half-morphed from disk
				objp->flags |= OF_SHOULD_BE_DEAD;
				objp->render_type = RT_POLYOBJ;
				objp->control_source = object::control_type::None;
				objp->movement_source = object::movement_type::None;
			}
		}
	}

//Save object info
	{
		const int i = Highest_object_index+1;
	PHYSFS_write(fp, &i, sizeof(int), 1);
	}
	{
		object_rw None{};
		None.type = OBJ_NONE;
		range_for (const auto &&objp, vcobjptr)
		{
			object_rw obj_rw;
			auto &obj = *objp;
			PHYSFS_write(fp, obj.type == OBJ_NONE ? &None : (state_object_to_object_rw(objp, &obj_rw), &obj_rw), sizeof(obj_rw), 1);
		}
	}
	
//Save wall info
	{
		auto &Walls = LevelUniqueWallSubsystemState.Walls;
		auto &vcwallptr = Walls.vcptr;
	{
		const int i = Walls.get_count();
	PHYSFS_write(fp, &i, sizeof(int), 1);
	}
	range_for (const auto &&w, vcwallptr)
		wall_write(fp, *w, 0x7fff);

#if defined(DXX_BUILD_DESCENT_II)
//Save exploding wall info
	expl_wall_write(Walls.vmptr, fp);
#endif
	}

//Save door info
	{
		auto &ActiveDoors = LevelUniqueWallSubsystemState.ActiveDoors;
	{
		const int i = ActiveDoors.get_count();
	PHYSFS_write(fp, &i, sizeof(int), 1);
	}
		range_for (auto &&ad, ActiveDoors.vcptr)
		active_door_write(fp, ad);
	}

#if defined(DXX_BUILD_DESCENT_II)
//Save cloaking wall info
	{
		auto &CloakingWalls = LevelUniqueWallSubsystemState.CloakingWalls;
	{
		const int i = CloakingWalls.get_count();
	PHYSFS_write(fp, &i, sizeof(int), 1);
	}
		range_for (auto &&w, CloakingWalls.vcptr)
		cloaking_wall_write(w, fp);
	}
#endif

//Save trigger info
	{
		auto &Triggers = LevelUniqueWallSubsystemState.Triggers;
	{
		unsigned num_triggers = Triggers.get_count();
		PHYSFS_write(fp, &num_triggers, sizeof(int), 1);
	}
	range_for (const trigger &vt, Triggers.vcptr)
		trigger_write(fp, vt);
	}

//Save tmap info
	range_for (const auto &&segp, vcsegptr)
	{
		for (auto &&[ss, us] : zip(segp->shared_segment::sides, segp->unique_segment::sides))	// d_zip
		{
			segment_side_wall_tmap_write(fp, ss, us);
		}
	}

// Save the fuelcen info
	{
		const int Control_center_destroyed = LevelUniqueControlCenterState.Control_center_destroyed;
	PHYSFS_write(fp, &Control_center_destroyed, sizeof(int), 1);
	}
#if defined(DXX_BUILD_DESCENT_I)
	PHYSFS_write(fp, &LevelUniqueControlCenterState.Countdown_seconds_left, sizeof(int), 1);
#elif defined(DXX_BUILD_DESCENT_II)
	PHYSFS_write(fp, &LevelUniqueControlCenterState.Countdown_timer, sizeof(int), 1);
#endif
	const unsigned Num_robot_centers = LevelSharedRobotcenterState.Num_robot_centers;
	PHYSFS_write(fp, &Num_robot_centers, sizeof(int), 1);
	range_for (auto &r, partial_const_range(RobotCenters, Num_robot_centers))
#if defined(DXX_BUILD_DESCENT_I)
		matcen_info_write(fp, r, STATE_MATCEN_VERSION);
#elif defined(DXX_BUILD_DESCENT_II)
		matcen_info_write(fp, r, 0x7f);
#endif
	control_center_triggers_write(&ControlCenterTriggers, fp);
	const auto Num_fuelcenters = LevelUniqueFuelcenterState.Num_fuelcenters;
	PHYSFS_write(fp, &Num_fuelcenters, sizeof(int), 1);
	range_for (auto &s, partial_range(Station, Num_fuelcenters))
	{
#if defined(DXX_BUILD_DESCENT_I)
		// NOTE: Usually Descent1 handles countdown by Timer value of the Reactor Station. Since we now use Descent2 code to handle countdown (which we do in case there IS NO Reactor Station which causes potential trouble in Multiplayer), let's find the Reactor here and store the timer in it.
		if (s.Type == SEGMENT_IS_CONTROLCEN)
			s.Timer = LevelUniqueControlCenterState.Countdown_timer;
#endif
		fuelcen_write(fp, s);
	}

// Save the control cen info
	{
		const int Control_center_been_hit = LevelUniqueControlCenterState.Control_center_been_hit;
	PHYSFS_write(fp, &Control_center_been_hit, sizeof(int), 1);
	}
	{
		const auto cc = static_cast<int>(LevelUniqueControlCenterState.Control_center_player_been_seen);
		PHYSFS_write(fp, &cc, sizeof(int), 1);
	}
	PHYSFS_write(fp, &LevelUniqueControlCenterState.Frametime_until_next_fire, sizeof(int), 1);
	{
		const int Control_center_present = LevelUniqueControlCenterState.Control_center_present;
	PHYSFS_write(fp, &Control_center_present, sizeof(int), 1);
	}
	{
		const auto Dead_controlcen_object_num = LevelUniqueControlCenterState.Dead_controlcen_object_num;
	int dead_controlcen_object_num = Dead_controlcen_object_num == object_none ? -1 : Dead_controlcen_object_num;
	PHYSFS_write(fp, &dead_controlcen_object_num, sizeof(int), 1);
	}

// Save the AI state
	ai_save_state( fp );

// Save the automap visited info
	PHYSFS_write(fp, LevelUniqueAutomapState.Automap_visited.data(), sizeof(uint8_t), std::max<std::size_t>(Highest_segment_index + 1, MAX_SEGMENTS_ORIGINAL));

	PHYSFS_write(fp, &state_game_id, sizeof(unsigned), 1);
	{
		const int i = 0;
	PHYSFS_write(fp, &cheats.rapidfire, sizeof(int), 1);
#if defined(DXX_BUILD_DESCENT_I)
	PHYSFS_write(fp, &i, sizeof(int), 1); // was Ugly_robot_cheat
	PHYSFS_write(fp, &i, sizeof(int), 1); // was Ugly_robot_texture
	PHYSFS_write(fp, &cheats.ghostphysics, sizeof(int), 1);
#endif
	PHYSFS_write(fp, &i, sizeof(int), 1); // was Lunacy
#if defined(DXX_BUILD_DESCENT_II)
	PHYSFS_write(fp, &i, sizeof(int), 1); // was Lunacy, too... and one was Ugly robot stuff a long time ago...

	// Save automap marker info

	range_for (int m, MarkerState.imobjidx)
	{
		if (m == object_none)
			m = -1;
		PHYSFS_write(fp, &m, sizeof(m), 1);
	}
	PHYSFS_seek(fp, PHYSFS_tell(fp) + (NUM_MARKERS)*(CALLSIGN_LEN+1)); // PHYSFS_write(fp, MarkerOwner, sizeof(MarkerOwner), 1); MarkerOwner is obsolete
	range_for (const auto &m, MarkerState.message)
		PHYSFS_write(fp, m.data(), m.size(), 1);

	PHYSFS_write(fp, &Afterburner_charge, sizeof(fix), 1);

	//save last was super information
	{
		auto &Primary_last_was_super = player_info.Primary_last_was_super;
		std::array<uint8_t, MAX_PRIMARY_WEAPONS> last_was_super{};
		/* Descent 2 shipped with Primary_last_was_super and
		 * Secondary_last_was_super each sized to contain MAX_*_WEAPONS,
		 * but only the first half of those are ever used.
		 * Unfortunately, the save file format is defined as saving
		 * MAX_*_WEAPONS for each.  Copy into a temporary, then write
		 * the temporary to the file.
		 */
		for (uint_fast32_t j = primary_weapon_index_t::VULCAN_INDEX; j != primary_weapon_index_t::SUPER_LASER_INDEX; ++j)
		{
			if (Primary_last_was_super & (1 << j))
				last_was_super[j] = 1;
		}
		PHYSFS_write(fp, &last_was_super, MAX_PRIMARY_WEAPONS, 1);
		auto &Secondary_last_was_super = player_info.Secondary_last_was_super;
		for (uint_fast32_t j = secondary_weapon_index_t::CONCUSSION_INDEX; j != secondary_weapon_index_t::SMISSILE1_INDEX; ++j)
		{
			if (Secondary_last_was_super & (1 << j))
				last_was_super[j] = 1;
		}
		PHYSFS_write(fp, &last_was_super, MAX_SECONDARY_WEAPONS, 1);
	}

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
	{
		union {
			std::array<uint8_t, MAX_SEGMENTS> light_subtracted;
			std::array<uint8_t, MAX_SEGMENTS_ORIGINAL> light_subtracted_original;
		};
		const auto &&r = make_range(vcsegptr);
		/* For compatibility with old game versions, always write at
		 * least MAX_SEGMENTS_ORIGINAL entries.  If the level is larger
		 * than that, then write as many entries as needed for the
		 * level.
		 */
		const unsigned count = (Highest_segment_index + 1 > MAX_SEGMENTS_ORIGINAL)
			/* Every written element will be filled by the loop, so
			 * there is no need to initialize the storage area.
			 */
			? vcsegptr.count()
			/* The loop may fill fewer than MAX_SEGMENTS_ORIGINAL
			 * entries, but MAX_SEGMENTS_ORIGINAL entries will be
			 * written, so zero-initialize the elements first.
			 */
			: (light_subtracted_original = {}, MAX_SEGMENTS_ORIGINAL);
		auto j = light_subtracted.begin();
		for (const unique_segment &useg : r)
			*j++ = useg.light_subtracted;
		PHYSFS_write(fp, light_subtracted.data(), sizeof(uint8_t), count);
	}
	PHYSFS_write(fp, &First_secret_visit, sizeof(First_secret_visit), 1);
	auto &Omega_charge = player_info.Omega_charge;
	PHYSFS_write(fp, &Omega_charge, sizeof(Omega_charge), 1);
#endif
	}

// Save Coop Info
	if (Game_mode & GM_MULTI_COOP)
	{
		/* Write local player's shields for everyone.  Remote players'
		 * shields are ignored, and using local everywhere is cheaper
		 * than using it only for the one slot where it may matter.
		 */
		const auto shields = plrobj.shields;
		const relocated_player_data rpd{shields, 0, 0, 0, 0};
		// I know, I know we only allow 4 players in coop. I screwed that up. But if we ever allow 8 players in coop, who's gonna laugh then?
		range_for (auto &i, partial_const_range(Players, MAX_PLAYERS))
		{
			state_write_player(fp, i, rpd, player_info);
		}
		PHYSFS_write(fp, Netgame.mission_title.data(), Netgame.mission_title.size(), 1);
		PHYSFS_write(fp, Netgame.mission_name.data(), Netgame.mission_name.size(), 1);
		PHYSFS_write(fp, &Netgame.levelnum, sizeof(int), 1);
		PHYSFS_write(fp, &Netgame.difficulty, sizeof(ubyte), 1);
		PHYSFS_write(fp, &Netgame.game_status, sizeof(ubyte), 1);
		PHYSFS_write(fp, &Netgame.numplayers, sizeof(ubyte), 1);
		PHYSFS_write(fp, &Netgame.max_numplayers, sizeof(ubyte), 1);
		PHYSFS_write(fp, &Netgame.numconnected, sizeof(ubyte), 1);
		PHYSFS_write(fp, &Netgame.level_time, sizeof(int), 1);
	}
	return 1;
}

//	-----------------------------------------------------------------------------------
//	Set the player's position from the globals Secret_return_segment and Secret_return_orient.
#if defined(DXX_BUILD_DESCENT_II)
void set_pos_from_return_segment(void)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vmobjptr = Objects.vmptr;
	auto &vmobjptridx = Objects.vmptridx;
	const auto &&plobjnum = vmobjptridx(get_local_player().objnum);
	const auto &&segp = vmsegptridx(LevelSharedSegmentState.Secret_return_segment);
	auto &vcvertptr = Vertices.vcptr;
	compute_segment_center(vcvertptr, plobjnum->pos, segp);
	obj_relink(vmobjptr, vmsegptr, plobjnum, segp);
	reset_player_object();
	plobjnum->orient = LevelSharedSegmentState.Secret_return_orient;
}
#endif

//	-----------------------------------------------------------------------------------
#if defined(DXX_BUILD_DESCENT_I)
int state_restore_all(const int in_game, std::nullptr_t, const blind_save blind)
#elif defined(DXX_BUILD_DESCENT_II)
int state_restore_all(const int in_game, const secret_restore secret, const char *const filename_override, const blind_save blind)
#endif
{

#if defined(DXX_BUILD_DESCENT_I)
	static constexpr std::integral_constant<secret_restore, secret_restore::none> secret{};
#elif defined(DXX_BUILD_DESCENT_II)
	if (in_game && Current_level_num < 0 && secret == secret_restore::none)
	{
		HUD_init_message_literal(HM_DEFAULT,  "Can't restore in secret level!" );
		return 0;
	}
#endif

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

	d_game_unique_state::savegame_file_path filename_storage;
	const char *filename;
	d_game_unique_state::save_slot filenum = d_game_unique_state::save_slot::None;
	{
		pause_game_world_time p;

#if defined(DXX_BUILD_DESCENT_II)
	if (filename_override) {
		filename = filename_override;
		filenum = d_game_unique_state::save_slot::secret_save_filename_override; // place outside of save slots
	} else
#endif
	{
		filenum = state_get_restore_file(filename_storage, blind);
		if (!GameUniqueState.valid_load_slot(filenum))
		{
			return 0;
		}
		filename = filename_storage.data();
	}
#if defined(DXX_BUILD_DESCENT_II)
	//	MK, 1/1/96
	//	Do special secret level stuff.
	//	If Nsecret.sgc (where N = filenum) exists, then copy it to secret.sgc.
	//	If it doesn't exist, then delete secret.sgc
	if (secret == secret_restore::none)
	{
		int	rval;

		if (filenum != d_game_unique_state::save_slot::None)
		{
			std::array<char, PATH_MAX> fname;
			const auto temp_fname = fname.data();
			format_secret_sgc_filename(fname, filenum);
			if (PHYSFSX_exists(temp_fname,0))
			{
				rval = copy_file(temp_fname, SECRETC_FILENAME);
				Assert(rval == 0);	//	Oops, error copying temp_fname to secret.sgc!
				(void)rval;
			} else
				PHYSFS_delete(SECRETC_FILENAME);
		}
	}
#endif
	if (secret == secret_restore::none && in_game && blind == blind_save::no)
	{
		const auto choice = nm_messagebox_str(menu_title{nullptr}, nm_messagebox_tie(TXT_YES, TXT_NO), menu_subtitle{"Restore Game?"});
		if ( choice != 0 )	{
			return 0;
		}
	}
	}
	return state_restore_all_sub(
#if defined(DXX_BUILD_DESCENT_II)
		LevelSharedSegmentState.DestructibleLights, secret,
#endif
		filename);
}

#if defined(DXX_BUILD_DESCENT_I)
int state_restore_all_sub(const char *filename)
#elif defined(DXX_BUILD_DESCENT_II)
int state_restore_all_sub(const d_level_shared_destructible_light_state &LevelSharedDestructibleLightState, const secret_restore secret, const char *const filename)
#endif
{
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	auto &vmobjptridx = Objects.vmptridx;
	auto &RobotCenters = LevelSharedRobotcenterState.RobotCenters;
	auto &Station = LevelUniqueFuelcenterState.Station;
	int version, coop_player_got[MAX_PLAYERS], coop_org_objnum = get_local_player().objnum;
	int swap = 0;	// if file is not endian native, have to swap all shorts and ints
	int current_level;
	char id[5];
	fix tmptime32 = 0;
	std::array<std::array<texture1_value, MAX_SIDES_PER_SEGMENT>, MAX_SEGMENTS> TempTmapNum;
	std::array<std::array<texture2_value, MAX_SIDES_PER_SEGMENT>, MAX_SEGMENTS> TempTmapNum2;

#if defined(DXX_BUILD_DESCENT_I)
	static constexpr std::integral_constant<secret_restore, secret_restore::none> secret{};
#elif defined(DXX_BUILD_DESCENT_II)
	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	fix64	old_gametime = GameTime64;
#endif

	#ifndef NDEBUG
	if (CGameArg.SysUsePlayersDir && strncmp(filename, PLAYER_DIRECTORY_TEXT, sizeof(PLAYER_DIRECTORY_TEXT) - 1))
		Int3();
	#endif

	auto fp = PHYSFSX_openReadBuffered(filename);
	if ( !fp ) return 0;

//Read id
	PHYSFS_read(fp, id, sizeof(char) * 4, 1);
	if ( memcmp( id, dgss_id, 4 )) {
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
		return 0;
	}

// Read Coop state_game_id. Oh the redundancy... we have this one later on but Coop games want to read this before loading a state so for easy access we have this here
	if (Game_mode & GM_MULTI_COOP)
	{
		callsign_t saved_callsign;
		state_game_id = PHYSFSX_readSXE32(fp, swap);
		PHYSFS_read(fp, &saved_callsign, sizeof(char)*CALLSIGN_LEN+1, 1);
		if (!(saved_callsign == get_local_player().callsign)) // check the callsign of the palyer who saved this state. It MUST match. If we transferred this savegame from pilot A to pilot B, others won't be able to restore us. So bail out here if this is the case.
		{
			return 0;
		}
	}

// Read description
	d_game_unique_state::savegame_description desc;
	PHYSFS_read(fp, desc.data(), 20, 1);
	desc.back() = 0;

// Skip the current screen shot...
	PHYSFS_seek(fp, PHYSFS_tell(fp) + THUMBNAIL_W * THUMBNAIL_H);
#if defined(DXX_BUILD_DESCENT_II)
// And now...skip the goddamn palette stuff that somebody forgot to add
	PHYSFS_seek(fp, PHYSFS_tell(fp) + 768);
#endif
// Read the Between levels flag...
	PHYSFSX_readSXE32(fp, swap);

// Read the mission info...
	savegame_mission_path mission_pathname{};
	PHYSFS_read(fp, mission_pathname.original.data(), mission_pathname.original.size(), 1);
	mission_name_type name_match_mode;
	mission_entry_predicate mission_predicate;
	switch (static_cast<savegame_mission_name_abi>(mission_pathname.original.back()))
	{
		case savegame_mission_name_abi::original:	/* Save game without the ability to do extended mission names */
			name_match_mode = mission_name_type::basename;
			mission_predicate.filesystem_name = mission_pathname.original.data();
#if defined(DXX_BUILD_DESCENT_II)
			mission_predicate.check_version = false;
#endif
			break;
		case savegame_mission_name_abi::pathname:	/* Save game with extended mission name */
			{
				PHYSFS_read(fp, mission_pathname.full.data(), mission_pathname.full.size(), 1);
				if (mission_pathname.full.back())
				{
					nm_messagebox_str(menu_title{TXT_ERROR}, nm_messagebox_tie(TXT_OK), menu_subtitle{"Unable to load game\nUnrecognized mission name format"});
					return 0;
				}
			}
			name_match_mode = mission_name_type::pathname;
			mission_predicate.filesystem_name = mission_pathname.full.data();
#if defined(DXX_BUILD_DESCENT_II)
			mission_predicate.check_version = true;
			mission_predicate.descent_version = static_cast<Mission::descent_version_type>(mission_pathname.original[1]);
#endif
			break;
		default:	/* Save game written by a future version of Rebirth.  ABI unknown. */
			nm_messagebox_str(menu_title{TXT_ERROR}, nm_messagebox_tie(TXT_OK), menu_subtitle{"Unable to load game\nUnrecognized save game format"});
			return 0;
	}

	if (const auto errstr = load_mission_by_name(mission_predicate, name_match_mode))
	{
		nm_messagebox(menu_title{TXT_ERROR}, 1, TXT_OK, "Unable to load mission\n'%s'\n\n%s", mission_pathname.full.data(), errstr);
		return 0;
	}

//Read level info
	current_level = PHYSFSX_readSXE32(fp, swap);
	PHYSFS_seek(fp, PHYSFS_tell(fp) + sizeof(PHYSFS_sint32)); // skip Next_level_num

//Restore GameTime
	tmptime32 = PHYSFSX_readSXE32(fp, swap);
	GameTime64 = static_cast<fix64>(tmptime32);

// Start new game....
	callsign_t org_callsign;
	LevelUniqueObjectState.accumulated_robots = 0;
	LevelUniqueObjectState.total_hostages = 0;
	GameUniqueState.accumulated_robots = 0;
	GameUniqueState.total_hostages = 0;
	if (!(Game_mode & GM_MULTI_COOP))
	{
		Game_mode = GM_NORMAL;
		change_playernum_to(0);
		N_players = 1;
		auto &callsign = vmplayerptr(0u)->callsign;
		callsign = InterfaceUniqueState.PilotName;
		org_callsign = callsign;
		if (secret == secret_restore::none)
		{
			InitPlayerObject();				//make sure player's object set up
			init_player_stats_game(0);		//clear all stats
		}
	}
	else // in coop we want to stay the player we are already.
	{
		org_callsign = get_local_player().callsign;
		if (secret == secret_restore::none)
			init_player_stats_game(Player_num);
	}

	if (Game_wind)
		window_set_visible(*Game_wind, 0);

//Read player info

	{
	player_info pl_info;
	relocated_player_data rpd;
#if defined(DXX_BUILD_DESCENT_II)
	player_info ret_pl_info;
	ret_pl_info.mission.hostages_on_board = get_local_plrobj().ctype.player_info.mission.hostages_on_board;
#endif
	{
#if DXX_USE_EDITOR
		// Don't bother with the other game sequence stuff if loading saved game in editor
		if (EditorWindow)
			LoadLevel(current_level, 1);
		else
#endif
			StartNewLevelSub(current_level, 1, secret);

		auto &plr = get_local_player();
#if defined(DXX_BUILD_DESCENT_II)
		auto &plrobj = get_local_plrobj();
		if (secret != secret_restore::none) {
			player	dummy_player;
			state_read_player(fp, dummy_player, swap, pl_info, rpd);
			if (secret == secret_restore::survived) {		//	This means he didn't die, so he keeps what he got in the secret level.
				const auto hostages_on_board = ret_pl_info.mission.hostages_on_board;
				ret_pl_info = plrobj.ctype.player_info;
				ret_pl_info.mission.hostages_on_board = hostages_on_board;
				rpd.shields = plrobj.shields;
				plr.level = dummy_player.level;
				plr.time_level = dummy_player.time_level;

				ret_pl_info.homing_object_dist = -1;
				plr.hours_level = dummy_player.hours_level;
				plr.hours_total = dummy_player.hours_total;
				do_cloak_invul_secret_stuff(old_gametime, ret_pl_info);
			} else {
				plr = dummy_player;
				// Keep keys even if they died on secret level (otherwise game becomes impossible)
				// Example: Cameron 'Stryker' Fultz's Area 51
				pl_info.powerup_flags |= (plrobj.ctype.player_info.powerup_flags &
										  (PLAYER_FLAGS_BLUE_KEY |
										   PLAYER_FLAGS_RED_KEY |
										   PLAYER_FLAGS_GOLD_KEY));
			}
		} else
#endif
		{
			state_read_player(fp, plr, swap, pl_info, rpd);
		}
		LevelUniqueObjectState.accumulated_robots = rpd.num_robots_level;
		LevelUniqueObjectState.total_hostages = rpd.hostages_level;
		GameUniqueState.accumulated_robots = rpd.num_robots_total;
		GameUniqueState.total_hostages = rpd.hostages_total;
	}
	{
		auto &plr = get_local_player();
		plr.callsign = org_callsign;
	if (Game_mode & GM_MULTI_COOP)
			plr.objnum = coop_org_objnum;
	}

	auto &Primary_weapon = pl_info.Primary_weapon;
// Restore the weapon states
	{
		int8_t v;
		PHYSFS_read(fp, &v, sizeof(int8_t), 1);
		Primary_weapon = static_cast<primary_weapon_index_t>(v);
	}
	auto &Secondary_weapon = pl_info.Secondary_weapon;
	{
		int8_t v;
		PHYSFS_read(fp, &v, sizeof(int8_t), 1);
		Secondary_weapon = static_cast<secondary_weapon_index_t>(v);
	}

	select_primary_weapon(pl_info, nullptr, Primary_weapon, 0);
	select_secondary_weapon(pl_info, nullptr, Secondary_weapon, 0);

// Restore the difficulty level
	{
		const unsigned u = PHYSFSX_readSXE32(fp, swap);
		GameUniqueState.Difficulty_level = cast_clamp_difficulty(u);
	}

// Restore the cheats enabled flag
	game_disable_cheats(); // disable cheats first
	cheats.enabled = PHYSFSX_readSXE32(fp, swap);
#if defined(DXX_BUILD_DESCENT_I)
	cheats.turbo = PHYSFSX_readSXE32(fp, swap);
#endif

	Do_appearance_effect = 0;			// Don't do this for middle o' game stuff.

	//Clear out all the objects from the lvl file
	for (unique_segment &useg : vmsegptr)
		useg.objects = object_none;
	reset_objects(LevelUniqueObjectState, 1);

	//Read objects, and pop 'em into their respective segments.
	{
		const int i = PHYSFSX_readSXE32(fp, swap);
	Objects.set_count(i);
	}
	range_for (const auto &&objp, vmobjptr)
	{
		object_rw obj_rw;
		PHYSFS_read(fp, &obj_rw, sizeof(obj_rw), 1);
		object_rw_swap(&obj_rw, swap);
		state_object_rw_to_object(&obj_rw, objp);
	}

	range_for (const auto &&obj, vmobjptridx)
	{
		obj->rtype.pobj_info.alt_textures = -1;
		if ( obj->type != OBJ_NONE )	{
			const auto segnum = obj->segnum;
			obj_link_unchecked(Objects.vmptr, obj, Segments.vmptridx(segnum));
		}
#if defined(DXX_BUILD_DESCENT_II)
		//look for, and fix, boss with bogus shields
		if (obj->type == OBJ_ROBOT && Robot_info[get_robot_id(obj)].boss_flag) {
			fix save_shields = obj->shields;

			copy_defaults_to_robot(obj);		//calculate starting shields

			//if in valid range, use loaded shield value
			if (save_shields > 0 && save_shields <= obj->shields)
				obj->shields = save_shields;
			else
				obj->shields /= 2;  //give player a break
		}
#endif
	}
	special_reset_objects(LevelUniqueObjectState);
	/* Reload plrobj reference.  The player's object number may have
	 * been changed by the state_object_rw_to_object call.
	 */
	auto &plrobj = get_local_plrobj();
	plrobj.shields = rpd.shields;
#if defined(DXX_BUILD_DESCENT_II)
	if (secret == secret_restore::survived)
	{		//	This means he didn't die, so he keeps what he got in the secret level.
		ret_pl_info.mission.last_score = pl_info.mission.last_score;
		plrobj.ctype.player_info = ret_pl_info;
	}
	else
#endif
		plrobj.ctype.player_info = pl_info;
	}
	/* Reload plrobj reference.  This is unnecessary for correctness,
	 * but is required by scoping rules, since the previous correct copy
	 * goes out of scope when pl_info goes out of scope, and that needs
	 * to be removed from scope to avoid a shadow warning when
	 * cooperative players are loaded below.
	 */
	auto &plrobj = get_local_plrobj();

	//	1 = Didn't die on secret level.
	//	2 = Died on secret level.
	if (secret != secret_restore::none && (Current_level_num >= 0)) {
		set_pos_from_return_segment();
#if defined(DXX_BUILD_DESCENT_II)
		if (secret == secret_restore::died)
			init_player_stats_new_ship(Player_num);
#endif
	}

	//Restore wall info
	init_exploding_walls();
	{
		auto &Walls = LevelUniqueWallSubsystemState.Walls;
	Walls.set_count(PHYSFSX_readSXE32(fp, swap));
	range_for (const auto &&w, Walls.vmptr)
		wall_read(fp, *w);

#if defined(DXX_BUILD_DESCENT_II)
	//now that we have the walls, check if any sounds are linked to
	//walls that are now open
	range_for (const auto &&wp, Walls.vcptr)
	{
		auto &w = *wp;
		if (w.type == WALL_OPEN)
			digi_kill_sound_linked_to_segment(w.segnum,w.sidenum,-1);	//-1 means kill any sound
	}

	//Restore exploding wall info
	if (version >= 10) {
		unsigned i = PHYSFSX_readSXE32(fp, swap);
		expl_wall_read_n_swap(Walls.vmptr, fp, swap, i);
	}
#endif
	}

	//Restore door info
	{
		auto &ActiveDoors = LevelUniqueWallSubsystemState.ActiveDoors;
	ActiveDoors.set_count(PHYSFSX_readSXE32(fp, swap));
	range_for (auto &&ad, ActiveDoors.vmptr)
		active_door_read(fp, ad);
	}

#if defined(DXX_BUILD_DESCENT_II)
	if (version >= 14) {		//Restore cloaking wall info
		unsigned num_cloaking_walls = PHYSFSX_readSXE32(fp, swap);
		auto &CloakingWalls = LevelUniqueWallSubsystemState.CloakingWalls;
		CloakingWalls.set_count(num_cloaking_walls);
		range_for (auto &&w, CloakingWalls.vmptr)
			cloaking_wall_read(w, fp);
	}
#endif

	//Restore trigger info
	{
		auto &Triggers = LevelUniqueWallSubsystemState.Triggers;
	Triggers.set_count(PHYSFSX_readSXE32(fp, swap));
	range_for (trigger &t, Triggers.vmptr)
		trigger_read(fp, t);
	}

	//Restore tmap info (to temp values so we can use compiled-in tmap info to compute static_light
	range_for (const auto &&segp, vmsegptridx)
	{
		range_for (const unsigned j, xrange(6u))
		{
			const uint16_t wall_num = PHYSFSX_readSXE16(fp, swap);
			segp->shared_segment::sides[j].wall_num = wallnum_t{wall_num};
			TempTmapNum[segp][j] = static_cast<texture1_value>(PHYSFSX_readSXE16(fp, swap));
			TempTmapNum2[segp][j] = static_cast<texture2_value>(PHYSFSX_readSXE16(fp, swap));
		}
	}

	//Restore the fuelcen info
	LevelUniqueControlCenterState.Control_center_destroyed = PHYSFSX_readSXE32(fp, swap);
#if defined(DXX_BUILD_DESCENT_I)
	LevelUniqueControlCenterState.Countdown_seconds_left = PHYSFSX_readSXE32(fp, swap);
	LevelUniqueControlCenterState.Countdown_timer = 0;
#elif defined(DXX_BUILD_DESCENT_II)
	LevelUniqueControlCenterState.Countdown_timer = PHYSFSX_readSXE32(fp, swap);
#endif
	const unsigned Num_robot_centers = PHYSFSX_readSXE32(fp, swap);
	LevelSharedRobotcenterState.Num_robot_centers = Num_robot_centers;
	range_for (auto &r, partial_range(RobotCenters, Num_robot_centers))
#if defined(DXX_BUILD_DESCENT_I)
		matcen_info_read(fp, r, STATE_MATCEN_VERSION);
#elif defined(DXX_BUILD_DESCENT_II)
		matcen_info_read(fp, r);
#endif
	control_center_triggers_read(&ControlCenterTriggers, fp);
	const unsigned Num_fuelcenters = PHYSFSX_readSXE32(fp, swap);
	LevelUniqueFuelcenterState.Num_fuelcenters = Num_fuelcenters;
	range_for (auto &s, partial_range(Station, Num_fuelcenters))
	{
		fuelcen_read(fp, s);
#if defined(DXX_BUILD_DESCENT_I)
		// NOTE: Usually Descent1 handles countdown by Timer value of the Reactor Station. Since we now use Descent2 code to handle countdown (which we do in case there IS NO Reactor Station which causes potential trouble in Multiplayer), let's find the Reactor here and read the timer from it.
		if (s.Type == SEGMENT_IS_CONTROLCEN)
			LevelUniqueControlCenterState.Countdown_timer = s.Timer;
#endif
	}

	// Restore the control cen info
	LevelUniqueControlCenterState.Control_center_been_hit = PHYSFSX_readSXE32(fp, swap);
	{
		const int cc = PHYSFSX_readSXE32(fp, swap);
		LevelUniqueControlCenterState.Control_center_player_been_seen = static_cast<player_visibility_state>(cc);
	}
	LevelUniqueControlCenterState.Frametime_until_next_fire = PHYSFSX_readSXE32(fp, swap);
	LevelUniqueControlCenterState.Control_center_present = PHYSFSX_readSXE32(fp, swap);
	LevelUniqueControlCenterState.Dead_controlcen_object_num = PHYSFSX_readSXE32(fp, swap);
	if (LevelUniqueControlCenterState.Control_center_destroyed)
		LevelUniqueControlCenterState.Total_countdown_time = LevelUniqueControlCenterState.Countdown_timer / F0_5; // we do not need to know this, but it should not be 0 either...

	// Restore the AI state
	ai_restore_state( fp, version, swap );

	{
		auto &Automap_visited = LevelUniqueAutomapState.Automap_visited;
	// Restore the automap visited info
		Automap_visited = {};
		DXX_MAKE_MEM_UNDEFINED(Automap_visited.begin(), Automap_visited.end());
		PHYSFS_read(fp, Automap_visited.data(), sizeof(uint8_t), std::max<std::size_t>(Highest_segment_index + 1, MAX_SEGMENTS_ORIGINAL));
	}

	{
	//	Restore hacked up weapon system stuff.
	auto &player_info = plrobj.ctype.player_info;
	/* These values were never saved, so coerce them to a sane default.
	 */
	player_info.Fusion_charge = 0;
	player_info.Player_eggs_dropped = false;
	player_info.FakingInvul = false;
	player_info.lavafall_hiss_playing = false;
	player_info.missile_gun = 0;
	player_info.Spreadfire_toggle = 0;
#if defined(DXX_BUILD_DESCENT_II)
	player_info.Helix_orientation = 0;
#endif
	player_info.Last_bumped_local_player = 0;
	auto &Next_laser_fire_time = player_info.Next_laser_fire_time;
	auto &Next_missile_fire_time = player_info.Next_missile_fire_time;
	player_info.Auto_fire_fusion_cannon_time = 0;
	player_info.Next_flare_fire_time = GameTime64;
	Next_laser_fire_time = GameTime64;
	Next_missile_fire_time = GameTime64;

	state_game_id = 0;

	if ( version >= 7 )	{
		state_game_id = PHYSFSX_readSXE32(fp, swap);
		cheats.rapidfire = PHYSFSX_readSXE32(fp, swap);
		PHYSFS_seek(fp, PHYSFS_tell(fp) + sizeof(PHYSFS_sint32)); // PHYSFSX_readSXE32(fp, swap); // was Lunacy
		PHYSFS_seek(fp, PHYSFS_tell(fp) + sizeof(PHYSFS_sint32)); // PHYSFSX_readSXE32(fp, swap); // was Lunacy, too... and one was Ugly robot stuff a long time ago...
#if defined(DXX_BUILD_DESCENT_I)
		cheats.ghostphysics = PHYSFSX_readSXE32(fp, swap);
		PHYSFS_seek(fp, PHYSFS_tell(fp) + sizeof(PHYSFS_sint32)); // PHYSFSX_readSXE32(fp, swap);
#endif
	}

#if defined(DXX_BUILD_DESCENT_II)
	if (version >= 17) {
		range_for (auto &i, MarkerState.imobjidx)
			i = PHYSFSX_readSXE32(fp, swap);
		PHYSFS_seek(fp, PHYSFS_tell(fp) + (NUM_MARKERS)*(CALLSIGN_LEN+1)); // PHYSFS_read(fp, MarkerOwner, sizeof(MarkerOwner), 1); // skip obsolete MarkerOwner
		range_for (auto &i, MarkerState.message)
		{
			std::array<char, MARKER_MESSAGE_LEN> a;
			PHYSFS_read(fp, a.data(), a.size(), 1);
			i.copy_if(a);
		}
	}
	else {
		int num;

		// skip dummy info

		num = PHYSFSX_readSXE32(fp, swap);           // was NumOfMarkers
		PHYSFS_seek(fp, PHYSFS_tell(fp) + sizeof(PHYSFS_sint32)); // PHYSFSX_readSXE32(fp, swap); // was CurMarker

		PHYSFS_seek(fp, PHYSFS_tell(fp) + num * (sizeof(vms_vector) + 40));

		range_for (auto &i, MarkerState.imobjidx)
			i = object_none;
	}

	if (version>=11) {
		if (secret != secret_restore::survived)
			Afterburner_charge = PHYSFSX_readSXE32(fp, swap);
		else {
			PHYSFSX_readSXE32(fp, swap);
		}
	}
	if (version>=12) {
		//read last was super information
		auto &Primary_last_was_super = player_info.Primary_last_was_super;
		std::array<uint8_t, MAX_PRIMARY_WEAPONS> last_was_super;
		/* Descent 2 shipped with Primary_last_was_super and
		 * Secondary_last_was_super each sized to contain MAX_*_WEAPONS,
		 * but only the first half of those are ever used.
		 * Unfortunately, the save file format is defined as saving
		 * MAX_*_WEAPONS for each.  Read into a temporary, then copy the
		 * meaningful elements to the live data.
		 */
		PHYSFS_read(fp, &last_was_super, MAX_PRIMARY_WEAPONS, 1);
		Primary_last_was_super = 0;
		for (uint_fast32_t j = primary_weapon_index_t::VULCAN_INDEX; j != primary_weapon_index_t::SUPER_LASER_INDEX; ++j)
		{
			if (last_was_super[j])
				Primary_last_was_super |= 1 << j;
		}
		PHYSFS_read(fp, &last_was_super, MAX_SECONDARY_WEAPONS, 1);
		auto &Secondary_last_was_super = player_info.Secondary_last_was_super;
		Secondary_last_was_super = 0;
		for (uint_fast32_t j = secondary_weapon_index_t::CONCUSSION_INDEX; j != secondary_weapon_index_t::SMISSILE1_INDEX; ++j)
		{
			if (last_was_super[j])
				Secondary_last_was_super |= 1 << j;
		}
	}

	if (version >= 12) {
		Flash_effect = PHYSFSX_readSXE32(fp, swap);
		tmptime32 = PHYSFSX_readSXE32(fp, swap);
		Time_flash_last_played = static_cast<fix64>(tmptime32);
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
			for (unique_segment &useg : vmsegptr)
			{
				PHYSFS_read(fp, &useg.light_subtracted, sizeof(useg.light_subtracted), 1);
			}
		}
		else
		{
			range_for (unique_segment &i, partial_range(Segments, MAX_SEGMENTS_ORIGINAL))
			{
				PHYSFS_read(fp, &i.light_subtracted, sizeof(i.light_subtracted), 1);
			}
		}
		apply_all_changed_light(LevelSharedDestructibleLightState, Segments.vmptridx);
	} else {
		for (unique_segment &useg : vmsegptr)
			useg.light_subtracted = 0;
	}

	if (secret == secret_restore::none)
	{
		if (version >= 20) {
			First_secret_visit = PHYSFSX_readSXE32(fp, swap);
		} else
			First_secret_visit = 1;
	} else
		First_secret_visit = 0;

	if (secret != secret_restore::survived)
	{
		player_info.Omega_charge = 0;
		/* The savegame does not record this, so pick a value.  Be
		 * nice to the player: let the cannon recharge immediately.
		 */
		player_info.Omega_recharge_delay = 0;
	}
	if (version >= 22)
	{
		auto i = PHYSFSX_readSXE32(fp, swap);
		if (secret != secret_restore::survived)
		{
			player_info.Omega_charge = i;
		}
	}
#endif
	}

	// static_light should now be computed - now actually set tmap info
	range_for (const auto &&segp, vmsegptridx)
	{
		range_for (const unsigned j, xrange(6u))
		{
			auto &uside = segp->unique_segment::sides[j];
			uside.tmap_num = TempTmapNum[segp][j];
			uside.tmap_num2 = TempTmapNum2[segp][j];
		}
	}

// Read Coop Info
	if (Game_mode & GM_MULTI_COOP)
	{
		player restore_players[MAX_PLAYERS];
		object restore_objects[MAX_PLAYERS];
		int coop_got_nplayers = 0;

		for (playernum_t i = 0; i < MAX_PLAYERS; i++) 
		{
			// prepare arrays for mapping our players below
			coop_player_got[i] = 0;

			// read the stored players
			player_info pl_info;
			relocated_player_data rpd;
			/* No need to reload num_robots_level again.  It was already
			 * restored above when the local player was restored.
			 */
			state_read_player(fp, restore_players[i], swap, pl_info, rpd);
			
			// make all (previous) player objects to ghosts but store them first for later remapping
			const auto &&obj = vmobjptr(restore_players[i].objnum);
			if (restore_players[i].connected == CONNECT_PLAYING && obj->type == OBJ_PLAYER)
			{
				obj->ctype.player_info = pl_info;
				obj->shields = rpd.shields;
				restore_objects[i] = *obj;
				obj->type = OBJ_GHOST;
				multi_reset_player_object(obj);
			}
		}
		for (playernum_t i = 0; i < MAX_PLAYERS; i++) // copy restored players to the current slots
		{
			for (unsigned j = 0; j < MAX_PLAYERS; j++)
			{
				// map stored players to current players depending on their unique (which we made sure) callsign
				if (vcplayerptr(i)->connected == CONNECT_PLAYING && restore_players[j].connected == CONNECT_PLAYING && vcplayerptr(i)->callsign == restore_players[j].callsign)
				{
					auto &p = *vmplayerptr(i);
					const auto sav_objnum = p.objnum;
					p = restore_players[j];
					p.objnum = sav_objnum;
					coop_player_got[i] = 1;
					coop_got_nplayers++;

					const auto &&obj = vmobjptridx(vcplayerptr(i)->objnum);
					// since a player always uses the same object, we just have to copy the saved object properties to the existing one. i hate you...
					obj->control_source = restore_objects[j].control_source;
					obj->movement_source = restore_objects[j].movement_source;
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
					set_player_id(obj, i); // assign player object id to player number
					multi_reset_player_object(obj);
					update_object_seg(vmobjptr, LevelSharedSegmentState, LevelUniqueSegmentState, obj);
				}
			}
		}
		{
			std::array<char, MISSION_NAME_LEN + 1> a;
			PHYSFS_read(fp, a.data(), a.size(), 1);
			Netgame.mission_title.copy_if(a);
		}
		{
			std::array<char, 9> a;
			PHYSFS_read(fp, a.data(), a.size(), 1);
			Netgame.mission_name.copy_if(a);
		}
		Netgame.levelnum = PHYSFSX_readSXE32(fp, swap);
		PHYSFS_read(fp, &Netgame.difficulty, sizeof(ubyte), 1);
		PHYSFS_read(fp, &Netgame.game_status, sizeof(ubyte), 1);
		PHYSFS_read(fp, &Netgame.numplayers, sizeof(ubyte), 1);
		PHYSFS_read(fp, &Netgame.max_numplayers, sizeof(ubyte), 1);
		PHYSFS_read(fp, &Netgame.numconnected, sizeof(ubyte), 1);
		Netgame.level_time = PHYSFSX_readSXE32(fp, swap);
		for (playernum_t i = 0; i < MAX_PLAYERS; i++)
		{
			const auto &&objp = vmobjptr(vcplayerptr(i)->objnum);
			auto &pi = objp->ctype.player_info;
			Netgame.killed[i] = pi.net_killed_total;
			Netgame.player_score[i] = pi.mission.score;
			Netgame.net_player_flags[i] = pi.powerup_flags;
		}
		for (playernum_t i = 0; i < MAX_PLAYERS; i++) // Disconnect connected players not available in this Savegame
			if (!coop_player_got[i] && vcplayerptr(i)->connected == CONNECT_PLAYING)
				multi_disconnect_player(i);
		Viewer = ConsoleObject = &get_local_plrobj(); // make sure Viewer and ConsoleObject are set up (which we skipped by not using InitPlayerObject but we need since objects changed while loading)
		special_reset_objects(LevelUniqueObjectState); // since we juggled around with objects to remap coop players rebuild the index of free objects
		state_set_next_autosave(GameUniqueState, Netgame.MPGameplayOptions.AutosaveInterval);
	}
	else
		state_set_next_autosave(GameUniqueState, PlayerCfg.SPGameplayOptions.AutosaveInterval);
	if (Game_wind)
		if (!window_is_visible(*Game_wind))
			window_set_visible(*Game_wind, 1);
	reset_time();

	return 1;
}

deny_save_result deny_save_game(fvcobjptr &vcobjptr, const d_level_unique_control_center_state &LevelUniqueControlCenterState)
{
	if (LevelUniqueControlCenterState.Control_center_destroyed)
		return deny_save_result::denied;
	if (Game_mode & GM_MULTI)
	{
		if (!(Game_mode & GM_MULTI_COOP))
			/* Deathmatch games can never be saved */
			return deny_save_result::denied;
		if (multi_common_deny_save_game(vcobjptr, partial_const_range(Players, N_players)))
			return deny_save_result::denied;
	}
	return deny_save_result::allowed;
}

}

namespace dcx {

int state_get_game_id(const d_game_unique_state::savegame_file_path &filename)
{
	int version;
	int swap = 0;	// if file is not endian native, have to swap all shorts and ints
	char id[5];
	callsign_t saved_callsign;

	#ifndef NDEBUG
	if (CGameArg.SysUsePlayersDir && strncmp(filename.data(), PLAYER_DIRECTORY_TEXT, sizeof(PLAYER_DIRECTORY_TEXT) - 1))
		Int3();
	#endif

	if (!(Game_mode & GM_MULTI_COOP))
		return 0;

	auto fp = PHYSFSX_openReadBuffered(filename.data());
	if ( !fp ) return 0;

//Read id
	PHYSFS_read(fp, id, sizeof(char) * 4, 1);
	if ( memcmp( id, dgss_id, 4 )) {
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
		return 0;
	}

// Read Coop state_game_id to validate the savegame we are about to load matches the others
	state_game_id = PHYSFSX_readSXE32(fp, swap);
	PHYSFS_read(fp, &saved_callsign, sizeof(char)*CALLSIGN_LEN+1, 1);
	if (!(saved_callsign == get_local_player().callsign)) // check the callsign of the palyer who saved this state. It MUST match. If we transferred this savegame from pilot A to pilot B, others won't be able to restore us. So bail out here if this is the case.
		return 0;

	return state_game_id;
}

}
