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
#include <ranges>

#include "pstypes.h"
#include "inferno.h"
#include "segment.h"
#include "textures.h"
#include "wall.h"
#include "object.h"
#include "dxxerror.h"
#include "console.h"
#include "gamefont.h"
#include "gameseg.h"
#include "switch.h"
#include "game.h"
#include "newmenu.h"
#include "fuelcen.h"
#include "hash.h"
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
#include "newdemo.h"
#include "automap.h"
#include "piggy.h"
#include "text.h"
#include "mission.h"
#include "u_mem.h"
#include "args.h"
#include "ai.h"
#include "fireball.h"
#include "controls.h"
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
#include "d_levelstate.h"
#include "d_range.h"
#include "d_enumerate.h"
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

namespace dsx {

namespace {

struct savegame_newmenu_items
{
	struct error_no_saves_found
	{
	};
	static constexpr unsigned decorative_item_count = 1;
	using imenu_description_buffers_array = std::array<ntstring<NM_MAX_TEXT_LEN>, NUM_SAVES>;
	d_game_unique_state::savegame_description *const caller_savegame_description;
	d_game_unique_state::savegame_file_path &savegame_file_path;
	enumerated_array<d_game_unique_state::savegame_file_path, NUM_SAVES, d_game_unique_state::save_slot> savegame_file_paths;
	enumerated_array<d_game_unique_state::savegame_description, NUM_SAVES, d_game_unique_state::save_slot> savegame_descriptions;
	enumerated_array<grs_bitmap_ptr, NUM_SAVES, d_game_unique_state::save_slot> sc_bmp;
	std::array<newmenu_item, NUM_SAVES + decorative_item_count> m;
	/* For saving a game, savegame_description is a
	 * caller-supplied buffer into which the user's text is placed, so
	 * that the caller can write that text into the file.
	 *
	 * For loading a game, savegame_description is nullptr.
	 */
	savegame_newmenu_items(d_game_unique_state::savegame_description *savegame_description, d_game_unique_state::savegame_file_path &savegame_file_path, imenu_description_buffers_array *);
	/* Test whether `selection` is an index that could be a valid
	 * choice.  If `selection` can never be a valid choice, return
	 * false.  If `selection` would be valid for a user who has every
	 * savegame slot in use, return true.  This does not test whether
	 * there currently exists a savegame in the specified slot.
	 */
	static unsigned valid_savegame_index(const d_game_unique_state::savegame_description *const user_entered_savegame_descriptions, const d_game_unique_state::save_slot selection)
	{
		return user_entered_savegame_descriptions
			? GameUniqueState.valid_save_slot(selection)
			: GameUniqueState.valid_load_slot(selection);
	}
	unsigned valid_savegame_index(const d_game_unique_state::save_slot selection) const
	{
		return valid_savegame_index(caller_savegame_description, selection);
	}
	std::size_t get_count_valid_menuitem_entries(d_game_unique_state::savegame_description *const savegame_description) const
	{
		return savegame_description ? m.size() - 1 : m.size();
	}
};

struct savegame_chooser_newmenu final : savegame_newmenu_items, newmenu
{
	enum class trailing_storage_size : std::size_t
	{
	};
	static void *operator new(std::size_t bytes) = delete;
	static void *operator new(std::size_t bytes, const trailing_storage_size extra_bytes)
	{
		return ::operator new(bytes + static_cast<std::size_t>(extra_bytes));
	}
	static void operator delete(void *p)
	{
		::operator delete(p);
	}
	static void operator delete(void *p, trailing_storage_size)
	{
		::operator delete(p);
	}
	virtual window_event_result event_handler(const d_event &) override;
	static std::unique_ptr<savegame_chooser_newmenu> create(menu_subtitle caption, grs_canvas &src, d_game_unique_state::savegame_description *const savegame_description, d_game_unique_state::savegame_file_path &savegame_file_path);
private:
	void draw_handler(grs_canvas &canvas, const grs_bitmap &bmp);
	void draw_handler();
	savegame_chooser_newmenu(menu_subtitle caption, grs_canvas &src, d_game_unique_state::savegame_description *, d_game_unique_state::savegame_file_path &);
	d_game_unique_state::save_slot build_save_slot_from_citem() const
	{
		if (citem < decorative_item_count)
			return d_game_unique_state::save_slot::None;
		return static_cast<d_game_unique_state::save_slot>(citem - decorative_item_count);
	}
};

std::unique_ptr<savegame_chooser_newmenu> savegame_chooser_newmenu::create(menu_subtitle caption, grs_canvas &src, d_game_unique_state::savegame_description *const savegame_description, d_game_unique_state::savegame_file_path &savegame_file_path)
{
	/* If savegame_description is not nullptr, then this is a request to
	 * create a "Save game" menu.  Otherwise, it is a request to create a
	 * "Load game" menu.
	 */
	const savegame_chooser_newmenu::trailing_storage_size extra_bytes{
		savegame_description ? sizeof(savegame_chooser_newmenu::imenu_description_buffers_array) : 0
	};
	return std::unique_ptr<savegame_chooser_newmenu>(new (extra_bytes) savegame_chooser_newmenu(caption, src, savegame_description, savegame_file_path));
}

savegame_chooser_newmenu::savegame_chooser_newmenu(menu_subtitle subtitle, grs_canvas &src, d_game_unique_state::savegame_description *const savegame_description, d_game_unique_state::savegame_file_path &savegame_file_path) :
	savegame_newmenu_items(savegame_description, savegame_file_path, savegame_description ? reinterpret_cast<savegame_newmenu_items::imenu_description_buffers_array *>(this + 1) : nullptr),
	newmenu(menu_title{nullptr}, subtitle, menu_filename{nullptr}, tiny_mode_flag::normal, tab_processing_flag::ignore, adjusted_citem::create(unchecked_partial_range(m, get_count_valid_menuitem_entries(savegame_description)), decorative_item_count), src)
{
}

void savegame_chooser_newmenu::draw_handler(grs_canvas &canvas, const grs_bitmap &bmp)
{
	const auto &&fspacx = FSPACX();
	const auto &&fspacy = FSPACY();
#if DXX_USE_OGL
	auto temp_canv = gr_create_canvas(THUMBNAIL_W * 2, THUMBNAIL_H * 24 / 10);
#else
	auto temp_canv = gr_create_canvas(fspacx(THUMBNAIL_W), fspacy(THUMBNAIL_H));
#endif
	const std::array<grs_point, 3> vertbuf{{
		{0, 0},
		{0, 0},
		{i2f(THUMBNAIL_W * 2), i2f(THUMBNAIL_H * 24 / 10)}
	}};
	scale_bitmap(bmp, vertbuf, 0, temp_canv->cv_bitmap);
	const auto bx = (canvas.cv_bitmap.bm_w / 2) - fspacx(THUMBNAIL_W / 2);
#if DXX_USE_OGL
	ogl_ubitmapm_cs(canvas, bx, m[0].y - fspacy(3), fspacx(THUMBNAIL_W), fspacy(THUMBNAIL_H), temp_canv->cv_bitmap, ogl_colors::white);
#else
	gr_bitmap(canvas, bx, m[0].y - 3, temp_canv->cv_bitmap);
#endif
}

void savegame_chooser_newmenu::draw_handler()
{
	const auto choice = build_save_slot_from_citem();
	if (!sc_bmp.valid_index(choice))
		return;
	if (auto &bmp = sc_bmp[choice])
		draw_handler(w_canv, *bmp);
}

window_event_result savegame_chooser_newmenu::event_handler(const d_event &event)
{
	switch (event.type)
	{
		case event_type::newmenu_draw:
			draw_handler();
			return window_event_result::handled;
		case event_type::newmenu_selected:
			{
				const auto citem = static_cast<const d_select_event &>(event).citem;
				const auto choice = static_cast<d_game_unique_state::save_slot>(citem - decorative_item_count);
				if (!valid_savegame_index(choice))
					return window_event_result::close;
				GameUniqueState.quicksave_selection = choice;
				savegame_file_path = savegame_file_paths[choice];
				if (const auto desc = caller_savegame_description)
				{
					auto &d = savegame_descriptions[choice];
					if (d.front())
						*desc = d;
					else
					{
						const time_t t = time(nullptr);
						if (const struct tm *ptm = (t == -1) ? nullptr : localtime(&t))
							strftime(desc->data(), desc->size(), "%m-%d %H:%M:%S", ptm);
						else
							strcpy(desc->data(), "-no title-");
					}
				}
			}
			return window_event_result::close;
		default:
			break;
	}
	return newmenu::event_handler(event);
}

// Following functions convert object to object_rw and back to be written to/read from Savegames. Mostly object differs to object_rw in terms of timer values (fix/fix64). as we reset GameTime64 for writing so it can fit into fix it's not necessary to increment savegame version. But if we once store something else into object which might be useful after restoring, it might be handy to increment Savegame version and actually store these new infos.
// turn object to object_rw to be saved to Savegame.

static void state_object_to_object_rw(const object &obj, object_rw *const obj_rw)
{
	const auto otype = obj.type;
	obj_rw->type = otype;
	obj_rw->signature     = static_cast<uint16_t>(obj.signature);
	obj_rw->id            = obj.id;
	obj_rw->next          = obj.next;
	obj_rw->prev          = obj.prev;
	obj_rw->control_source  = underlying_value(obj.control_source);
	obj_rw->movement_source = underlying_value(obj.movement_source);
	const auto rtype = obj.render_type;
	obj_rw->render_type   = underlying_value(rtype);
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
	obj_rw->contains_type = underlying_value(obj.contains.type);
	obj_rw->contains_id   = underlying_value(obj.contains.id.robot);
	obj_rw->contains_count= obj.contains.count;
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
			obj_rw->ctype.ai_info.behavior               = static_cast<uint8_t>(obj.ctype.ai_info.behavior);
			obj_rw->ctype.ai_info.flags[0] = underlying_value(obj.ctype.ai_info.CURRENT_GUN);
			obj_rw->ctype.ai_info.flags[1] = obj.ctype.ai_info.CURRENT_STATE;
			obj_rw->ctype.ai_info.flags[2] = obj.ctype.ai_info.GOAL_STATE;
			obj_rw->ctype.ai_info.flags[3] = obj.ctype.ai_info.PATH_DIR;
#if defined(DXX_BUILD_DESCENT_I)
			obj_rw->ctype.ai_info.flags[4] = obj.ctype.ai_info.SUBMODE;
#elif defined(DXX_BUILD_DESCENT_II)
			obj_rw->ctype.ai_info.flags[4] = obj.ctype.ai_info.SUB_FLAGS;
#endif
			obj_rw->ctype.ai_info.flags[5] = underlying_value(obj.ctype.ai_info.GOALSIDE);
			obj_rw->ctype.ai_info.flags[6] = obj.ctype.ai_info.CLOAKED;
			obj_rw->ctype.ai_info.flags[7] = obj.ctype.ai_info.SKIP_AI_COUNT;
			obj_rw->ctype.ai_info.flags[8] = obj.ctype.ai_info.REMOTE_OWNER;
			obj_rw->ctype.ai_info.flags[9] = obj.ctype.ai_info.REMOTE_SLOT_NUM;
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

	switch (rtype)
	{
		case render_type::RT_NONE:
			if (obj.type != OBJ_GHOST) // HACK: when a player is dead or not connected yet, clients still expect to get polyobj data - even if render_type == RT_NONE at this time. Here it's not important, but it might be for Multiplayer Savegames.
				break;
			[[fallthrough]];
		case render_type::RT_MORPH:
		case render_type::RT_POLYOBJ:
		{
			int i;
			obj_rw->rtype.pobj_info.model_num                = underlying_value(obj.rtype.pobj_info.model_num);
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
			
		case render_type::RT_WEAPON_VCLIP:
		case render_type::RT_HOSTAGE:
		case render_type::RT_POWERUP:
		case render_type::RT_FIREBALL:
			obj_rw->rtype.vclip_info.vclip_num = underlying_value(obj.rtype.vclip_info.vclip_num);
			obj_rw->rtype.vclip_info.frametime = obj.rtype.vclip_info.frametime;
			obj_rw->rtype.vclip_info.framenum  = obj.rtype.vclip_info.framenum;
			break;
			
		case render_type::RT_LASER:
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
	if (const auto rtype = obj_rw->render_type; valid_render_type(rtype))
		obj.render_type = render_type{rtype};
	else
	{
		con_printf(CON_URGENT, "save file used bogus render type %#x for object %p; using none instead", rtype, &obj);
		obj.render_type = render_type::RT_NONE;
	}
	obj.flags         = obj_rw->flags;
	obj.segnum    = vmsegidx_t::check_nothrow_index(obj_rw->segnum).value_or(segment_none);
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
	obj.contains = build_contained_object_parameters_from_untrusted(obj_rw->contains_type, obj_rw->contains_id, obj_rw->contains_count);
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
			/* Use vcobjidx_t so that `object_none` fails the test and uses the
			 * `else` path.  `reset_hitobj` can accept `object_none`, but it
			 * cannot accept invalid indexes, such as the corrupted value
			 * `0x1ff` produced by certain <0.60 builds, when they ran
			 * `laser_info.hitobj_list[-1] = 1;` due to using `object_none`
			 * (`-1` back then) as if it were a valid index.  That bug was
			 * fixed in 14cdf1b3521ff82701c58c04e47a6c1deefe8e43, but some old
			 * save games were corrupted by it, so trap that error here.
			 */
			if (const auto last_hitobj{vcobjidx_t::check_nothrow_index(obj_rw->ctype.laser_info.last_hitobj)})
				obj.ctype.laser_info.reset_hitobj(*last_hitobj);
			else
				obj.ctype.laser_info.clear_hitobj();
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
			obj.ctype.ai_info.behavior               = static_cast<ai_behavior>(obj_rw->ctype.ai_info.behavior);
			{
				const uint8_t gun_num = obj_rw->ctype.ai_info.flags[0];
				obj.ctype.ai_info.CURRENT_GUN = (gun_num < MAX_GUNS) ? robot_gun_number{gun_num} : robot_gun_number{};
			}
			obj.ctype.ai_info.CURRENT_STATE = build_ai_state_from_untrusted(obj_rw->ctype.ai_info.flags[1]).value();
			obj.ctype.ai_info.GOAL_STATE = build_ai_state_from_untrusted(obj_rw->ctype.ai_info.flags[2]).value();
			obj.ctype.ai_info.PATH_DIR = obj_rw->ctype.ai_info.flags[3];
#if defined(DXX_BUILD_DESCENT_I)
			obj.ctype.ai_info.SUBMODE = obj_rw->ctype.ai_info.flags[4];
#elif defined(DXX_BUILD_DESCENT_II)
			obj.ctype.ai_info.SUB_FLAGS = obj_rw->ctype.ai_info.flags[4];
#endif
			obj.ctype.ai_info.GOALSIDE = build_sidenum_from_untrusted(obj_rw->ctype.ai_info.flags[5]).value();
			obj.ctype.ai_info.CLOAKED = obj_rw->ctype.ai_info.flags[6];
			obj.ctype.ai_info.SKIP_AI_COUNT = obj_rw->ctype.ai_info.flags[7];
			obj.ctype.ai_info.REMOTE_OWNER = obj_rw->ctype.ai_info.flags[8];
			obj.ctype.ai_info.REMOTE_SLOT_NUM = obj_rw->ctype.ai_info.flags[9];
			obj.ctype.ai_info.hide_segment = vmsegidx_t::check_nothrow_index(obj_rw->ctype.ai_info.hide_segment).value_or(segment_none);
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
		case render_type::RT_NONE:
			if (obj.type != OBJ_GHOST) // HACK: when a player is dead or not connected yet, clients still expect to get polyobj data - even if render_type == RT_NONE at this time. Here it's not important, but it might be for Multiplayer Savegames.
				break;
			[[fallthrough]];
		case render_type::RT_MORPH:
		case render_type::RT_POLYOBJ:
		{
			int i;
			obj.rtype.pobj_info.model_num                = build_polygon_model_index_from_untrusted(obj_rw->rtype.pobj_info.model_num);
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
			
		case render_type::RT_WEAPON_VCLIP:
		case render_type::RT_HOSTAGE:
		case render_type::RT_POWERUP:
		case render_type::RT_FIREBALL:
			obj.rtype.vclip_info.vclip_num = build_vclip_index_from_untrusted(obj_rw->rtype.vclip_info.vclip_num);
			obj.rtype.vclip_info.frametime = obj_rw->rtype.vclip_info.frametime;
			obj.rtype.vclip_info.framenum  = obj_rw->rtype.vclip_info.framenum;
			break;
			
		case render_type::RT_LASER:
			break;
			
	}
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
	pl_rw->callsign = pl->callsign;
	memset(pl_rw->net_address, 0, 6);
	pl_rw->connected                 = underlying_value(pl->connected);
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
	pl_rw->secondary_weapon_flags    = 0x0f | (pl_info.secondary_ammo[secondary_weapon_index_t::MEGA_INDEX] > 0) << underlying_value(secondary_weapon_index_t::MEGA_INDEX);
#elif defined(DXX_BUILD_DESCENT_II)
	// make sure no side effects for PC demo
	pl_rw->secondary_weapon_flags    = 0xef | (pl_info.secondary_ammo[secondary_weapon_index_t::MEGA_INDEX] > 0) << underlying_value(secondary_weapon_index_t::MEGA_INDEX)
											| (pl_info.secondary_ammo[secondary_weapon_index_t::SMISSILE4_INDEX] > 0) << underlying_value(secondary_weapon_index_t::SMISSILE4_INDEX)	// mercury missile
											| (pl_info.secondary_ammo[secondary_weapon_index_t::SMISSILE5_INDEX] > 0) << underlying_value(secondary_weapon_index_t::SMISSILE5_INDEX);	// earthshaker missile
#endif
	pl_rw->obsolete_primary_ammo = {};
	pl_rw->vulcan_ammo   = pl_info.vulcan_ammo;
	for (const auto &&[iw, r] : enumerate(pl_info.secondary_ammo))
		pl_rw->secondary_ammo[underlying_value(iw)] = r;
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
	pl->callsign = pl_rw->callsign;
	pl->connected                 = player_connection_status{pl_rw->connected};
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
	for (const auto &&[ir, w] : enumerate(pl_info.secondary_ammo))
		w = pl_rw->secondary_ammo[underlying_value(ir)];
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
	PHYSFSX_writeBytes(fp, &pl_rw, sizeof(pl_rw));
}

static void state_read_player(PHYSFS_File *fp, player &pl, const physfsx_endian swap, player_info &pl_info, relocated_player_data &rpd)
{
	player_rw pl_rw;
	PHYSFSX_readBytes(fp, &pl_rw, sizeof(pl_rw));
	player_rw_swap(&pl_rw, swap);
	state_player_rw_to_player(&pl_rw, &pl, pl_info, rpd);
}

}

}

namespace dcx {

namespace {

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

}

namespace dsx {

namespace {

uint8_t read_savegame_properties(const std::size_t savegame_index, d_game_unique_state::savegame_file_path &filename, d_game_unique_state::savegame_description *const dsc, grs_bitmap_ptr *const sc_bmp)
{
	state_format_savegame_filename(filename, savegame_index);
	const auto fp = PHYSFSX_openReadBuffered(filename.data()).first;
	if (!fp)
		return 0;
	//Read id
	char id[4]{};
	if (PHYSFSX_readBytes(fp, id, sizeof(id)) != sizeof(id))
		return 0;
	if (memcmp(id, dgss_id, 4))
		return 0;
	//Read version
	unsigned version;
	if (PHYSFSX_readBytes(fp, &version, sizeof(version)) != sizeof(version))
		return 0;
	if (!(version >= STATE_COMPATIBLE_VERSION || SWAPINT(version) >= STATE_COMPATIBLE_VERSION))
		return 0;
	// In case it's Coop, handle state_game_id & callsign as well
	if (Game_mode & GM_MULTI_COOP)
	{
		PHYSFS_seek(fp, PHYSFS_tell(fp) + sizeof(PHYSFS_sint32) + sizeof(char)*CALLSIGN_LEN+1); // skip state_game_id, callsign
	}
	d_game_unique_state::savegame_description desc_storage;
	d_game_unique_state::savegame_description &desc = dsc ? *dsc : desc_storage;
	// Read description
	if (const std::size_t buffer_size{std::size(desc)}; PHYSFSX_readBytes(fp, std::data(desc), buffer_size) != buffer_size)
		return 0;
	desc.back() = 0;
	if (sc_bmp)
	{
		// Read thumbnail
		grs_bitmap_ptr bmp = gr_create_bitmap(THUMBNAIL_W, THUMBNAIL_H);
		if (constexpr std::size_t buffer_size{THUMBNAIL_W * THUMBNAIL_H}; PHYSFSX_readBytes(fp, bmp->get_bitmap_data(), buffer_size) != buffer_size)
			return 0;
#if defined(DXX_BUILD_DESCENT_II)
		if (version >= 9)
		{
			palette_array_t pal;
			if (constexpr std::size_t buffer_size{sizeof(pal[0]) * std::size(pal)}; PHYSFSX_readBytes(fp, pal, buffer_size) != buffer_size)
				return 0;
			gr_remap_bitmap_good(*bmp.get(), pal, -1, -1);
		}
#endif
		*sc_bmp = std::move(bmp);
	}
	return 1;
}

savegame_newmenu_items::savegame_newmenu_items(d_game_unique_state::savegame_description *const savegame_description, d_game_unique_state::savegame_file_path &savegame_file_path, imenu_description_buffers_array *const user_entered_savegame_descriptions) :
	caller_savegame_description(savegame_description),
	savegame_file_path(savegame_file_path)
{
	unsigned nsaves = 0;
	/* Always start at offset `decorative_item_count` to skip the fixed
	 * text leader.  Conditionally subtract 1 if the call is for saving,
	 * since interactive saves should not access the last slot.  The
	 * last slot is reserved for autosaves.
	 */
	const unsigned max_slots_shown = get_count_valid_menuitem_entries(savegame_description);
	for (const auto &&[savegame_index, mi, filename, desc, sc_bmp] : enumerate(zip(partial_range(m, decorative_item_count, max_slots_shown), savegame_file_paths, savegame_descriptions, sc_bmp)))
	{
		const auto existing_savegame_found = read_savegame_properties(savegame_index, filename, &desc, &sc_bmp);
		if (existing_savegame_found)
			++nsaves;
		else
			/* Defer setting a default value to here.  This allows the
			 * value to be written only if a better one was not
			 * retrieved from a save game file.
			 */
			strcpy(desc.data(), TXT_EMPTY);
		mi.text = desc.data();
		mi.type = savegame_description
			/* If saving, use input_menu so that the user can pick an
			 * element and convert it into a text entry field to receive
			 * the save game title.
			 */
			? nm_type::input_menu
			/* If restoring, use text.  Valid save games will switch the
			 * type.  Invalid save slots will remain set as text.
			 */
			: (existing_savegame_found
				? nm_type::menu
				: nm_type::text);
		if (user_entered_savegame_descriptions)
			mi.initialize_imenu(desc, (*user_entered_savegame_descriptions)[savegame_index]);
	}
	if (!savegame_description && nsaves < 1)
		throw error_no_saves_found();
	nm_set_item_text(m[0], "\n\n\n\n");
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
	if (Newdemo_state != ND_STATE_NORMAL && Newdemo_state != ND_STATE_RECORDING)
		return;
	const auto now = std::chrono::steady_clock::now();
	if (now < GameUniqueState.Next_autosave)
		return;
	state_set_next_autosave(GameUniqueState, now, interval);
	state_autosave_game(multiplayer);
}

namespace {

/* Present a menu for selection of a savegame filename.
 * For saving, dsc should be a pre-allocated buffer into which the new
 * savegame description will be stored.
 * For restoring, dsc should be NULL, in which case empty slots will not be
 * selectable and savagames descriptions will not be editable.
 */
static d_game_unique_state::save_slot state_get_savegame_filename(grs_canvas &canvas, d_game_unique_state::savegame_file_path &fname, d_game_unique_state::savegame_description *const dsc, const menu_subtitle caption, const blind_save entry_blind)
{
	const auto quicksave_selection = GameUniqueState.quicksave_selection;
	if (entry_blind != blind_save::no &&
		/* The user requested a non-blind save.  This block only handles
		 * blind saves, so skip down to the general handler.
		 */
		savegame_chooser_newmenu::valid_savegame_index(dsc, quicksave_selection))
	{
		/* The cached slot is valid, so a save/load might work.
		 */
		if (read_savegame_properties(static_cast<std::size_t>(quicksave_selection), fname, dsc, nullptr))
			/* The data was read from the savegame.  Return early and
			 * skip the dialog.
			 */
			return quicksave_selection;
		/* Fall through and run the interactive dialog, whether the user
		 * requested one or not.
		 */
	}
	std::unique_ptr<savegame_chooser_newmenu> win;
	try {
		win = savegame_chooser_newmenu::create(caption, canvas, dsc, fname);
	}
	catch (const savegame_chooser_newmenu::error_no_saves_found &)
	{
		struct error_no_saves_found : passive_messagebox
		{
			error_no_saves_found(grs_canvas &canvas) :
				passive_messagebox(menu_title{nullptr}, menu_subtitle{"No saved games were found!"}, TXT_OK, canvas)
			{
			}
		};
		run_blocking_newmenu<error_no_saves_found>(canvas);
		return d_game_unique_state::save_slot::None;
	}
	win->send_creation_events();
	const auto citem = savegame_chooser_newmenu::process_until_closed(win.release());
	/* win is now invalid */
	if (citem <= 0)
		return d_game_unique_state::save_slot::None;
	const auto choice = static_cast<d_game_unique_state::save_slot>(citem - savegame_chooser_newmenu::decorative_item_count);
	assert(savegame_chooser_newmenu::valid_savegame_index(dsc, choice));
	return choice;
}

}

d_game_unique_state::save_slot state_get_save_file(grs_canvas &canvas, d_game_unique_state::savegame_file_path &fname, d_game_unique_state::savegame_description *const dsc, const blind_save blind_save)
{
	return state_get_savegame_filename(canvas, fname, dsc, menu_subtitle{"Save Game"}, blind_save);
}

d_game_unique_state::save_slot state_get_restore_file(grs_canvas &canvas, d_game_unique_state::savegame_file_path &fname, blind_save blind_save)
{
	return state_get_savegame_filename(canvas, fname, nullptr, menu_subtitle{"Select Game to Restore"}, blind_save);
}

#if defined(DXX_BUILD_DESCENT_I)
#elif defined(DXX_BUILD_DESCENT_II)
namespace {

//	-----------------------------------------------------------------------------------
//	Imagine if C had a function to copy a file...
static int copy_file(const char *old_file, const char *new_file)
{
	RAIIPHYSFS_File in_file{PHYSFS_openRead(old_file)};
	if (!in_file)
		return -2;
	RAIIPHYSFS_File out_file{PHYSFS_openWrite(new_file)};
	if (!out_file)
		return -1;

	constexpr std::size_t buf_size = 512 * 1024;
	const auto buf = std::make_unique<uint8_t[]>(buf_size);
	const auto pbuf = buf.get();
	while (!PHYSFS_eof(in_file))
	{
		const auto bytes_read{PHYSFSX_readBytes(in_file, pbuf, buf_size)};
		if (bytes_read < 0)
			Error("Cannot read from file <%s>: %s", old_file, PHYSFS_getLastError());

		if (PHYSFSX_writeBytes(out_file, pbuf, bytes_read) < bytes_read)
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
		filenum = state_get_save_file(*grd_curcanv, filename_storage, &desc, blind_save);
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
			if (PHYSFS_exists(temp_fname))
			{
				if (!PHYSFS_delete(temp_fname))
					Error("Cannot delete file <%s>: %s", temp_fname, PHYSFS_getLastError());
			}

			if (PHYSFS_exists(SECRETC_FILENAME))
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

	auto &&[fp, physfserr] = PHYSFSX_openWriteBuffered(filename);
	if (!fp)
	{
		const auto errstr = PHYSFS_getErrorByCode(physfserr);
		con_printf(CON_URGENT, "Failed to open savegame %s: %s", filename, errstr);
		struct error_writing_savegame :
			std::array<char, 96>,
			passive_messagebox
		{
			error_writing_savegame(const char *filename, const char *errstr) :
				passive_messagebox(menu_title{TXT_ERROR}, menu_subtitle{prepare_subtitle(*this, filename, errstr)}, "Return to unsaved game", grd_curscreen->sc_canvas)
			{
			}
			static const char *prepare_subtitle(std::array<char, 96> &b, const char *filename, const char *errstr)
			{
				auto r = b.data();
				std::snprintf(r, b.size(), "Failed to write savegame\n%s\n\n%s", filename, errstr);
				return r;
			}
		};
		run_blocking_newmenu<error_writing_savegame>(filename, errstr);
		return 0;
	}

	pause_game_world_time p;

//Save id
	PHYSFSX_writeBytes(fp, dgss_id, sizeof(char) * 4);

//Save version
	{
		const int i = STATE_VERSION;
		PHYSFSX_writeBytes(fp, &i, sizeof(int));
	}

// Save Coop state_game_id and this Player's callsign. Oh the redundancy... we have this one later on but Coop games want to read this before loading a state so for easy access save this here, too
	if (Game_mode & GM_MULTI_COOP)
	{
		PHYSFSX_writeBytes(fp, &state_game_id, sizeof(unsigned));
		PHYSFSX_writeBytes(fp, &get_local_player().callsign, sizeof(char) * CALLSIGN_LEN + 1);
	}

//Save description
	PHYSFSX_writeBytes(fp, desc, 20);

// Save the current screen shot...

	auto cnv = gr_create_canvas( THUMBNAIL_W, THUMBNAIL_H );
	{
		{
			window_rendered_data window;
			render_frame(*cnv, 0, window);
		}

		{
#if DXX_USE_OGL
			const auto buf = std::make_unique<uint8_t[]>(THUMBNAIL_W * THUMBNAIL_H * 4);
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

		PHYSFSX_writeBytes(fp, cnv->cv_bitmap.bm_data, THUMBNAIL_W * THUMBNAIL_H);
#if defined(DXX_BUILD_DESCENT_II)
		PHYSFSX_writeBytes(fp, gr_palette.data(), sizeof(gr_palette[0]) * gr_palette.size());
#endif
	}

// Save the Between levels flag...
	{
		const int i = 0;
		PHYSFSX_writeBytes(fp, &i, sizeof(int));
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
	PHYSFSX_writeBytes(fp, &mission_pathname, sizeof(mission_pathname));

//Save level info
	PHYSFSX_writeBytes(fp, &Current_level_num, sizeof(int));
	PHYSFS_writeULE32(fp, 0);

//Save GameTime
// NOTE: GameTime now is GameTime64 with fix64 since GameTime could only last 9 hrs. To even help old Savegames, we do not increment Savegame version but rather RESET GameTime64 to 0 on every save! ALL variables based on GameTime64 now will get the current GameTime64 value substracted and saved to fix size as well.
	tmptime32 = 0;
	PHYSFSX_writeBytes(fp, &tmptime32, sizeof(fix));

//Save player info
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
		const int8_t v = static_cast<int8_t>(player_info.Primary_weapon.get_active());
		PHYSFSX_writeBytes(fp, &v, sizeof(int8_t));
	}
	{
		const int8_t v = static_cast<int8_t>(player_info.Secondary_weapon.get_active());
		PHYSFSX_writeBytes(fp, &v, sizeof(int8_t));
	}

// Save the difficulty level
	{
		const int Difficulty_level = underlying_value(GameUniqueState.Difficulty_level);
		PHYSFSX_writeBytes(fp, &Difficulty_level, sizeof(int));
	}

// Save cheats enabled
	PHYSFS_writeULE32(fp, cheats.enabled ? UINT32_MAX : 0);
#if defined(DXX_BUILD_DESCENT_I)
	PHYSFS_writeULE32(fp, cheats.turbo ? UINT32_MAX : 0);
#endif

//Finish all morph objects
	for (auto &obj : vmobjptr)
	{
		if (obj.type != OBJ_NONE && obj.render_type == render_type::RT_MORPH)
		{
			if (const auto umd = find_morph_data(LevelUniqueMorphObjectState, obj))
			{
				const auto md = umd->get();
				md->obj->control_source = md->morph_save_control_type;
				md->obj->movement_source = md->morph_save_movement_type;
				md->obj->render_type = render_type::RT_POLYOBJ;
				md->obj->mtype.phys_info = md->morph_save_phys_info;
				umd->reset();
			} else {						//maybe loaded half-morphed from disk
				obj.flags |= OF_SHOULD_BE_DEAD;
				obj.render_type = render_type::RT_POLYOBJ;
				obj.control_source = object::control_type::None;
				obj.movement_source = object::movement_type::None;
			}
		}
	}

//Save object info
	{
		const int i = Highest_object_index+1;
		PHYSFSX_writeBytes(fp, &i, sizeof(int));
	}
	{
		object_rw None{};
		None.type = OBJ_NONE;
		for (auto &obj : vcobjptr)
		{
			object_rw obj_rw;
			PHYSFSX_writeBytes(fp, obj.type == OBJ_NONE ? &None : (state_object_to_object_rw(obj, &obj_rw), &obj_rw), sizeof(obj_rw));
		}
	}
	
//Save wall info
	{
		auto &Walls = LevelUniqueWallSubsystemState.Walls;
		auto &vcwallptr = Walls.vcptr;
	{
		const int i = Walls.get_count();
		PHYSFSX_writeBytes(fp, &i, sizeof(int));
	}
	for (auto &w : vcwallptr)
		wall_write(fp, w, 0x7fff);

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
		PHYSFSX_writeBytes(fp, &i, sizeof(int));
	}
		for (auto &ad : ActiveDoors.vcptr)
			active_door_write(fp, ad);
	}

#if defined(DXX_BUILD_DESCENT_II)
//Save cloaking wall info
	{
		auto &CloakingWalls = LevelUniqueWallSubsystemState.CloakingWalls;
	{
		const int i = CloakingWalls.get_count();
		PHYSFSX_writeBytes(fp, &i, sizeof(int));
	}
		for (auto &w : CloakingWalls.vcptr)
			cloaking_wall_write(w, fp);
	}
#endif

//Save trigger info
	{
		auto &Triggers = LevelUniqueWallSubsystemState.Triggers;
	{
		unsigned num_triggers = Triggers.get_count();
		PHYSFSX_writeBytes(fp, &num_triggers, sizeof(int));
	}
		for (auto &vt : Triggers.vcptr)
			trigger_write(fp, vt);
	}

//Save tmap info
	for (auto &seg : vcsegptr)
	{
		for (auto &&[ss, us] : zip(seg.shared_segment::sides, seg.unique_segment::sides))	// d_zip
		{
			segment_side_wall_tmap_write(fp, ss, us);
		}
	}

// Save the fuelcen info
	{
		const int Control_center_destroyed = LevelUniqueControlCenterState.Control_center_destroyed;
		PHYSFSX_writeBytes(fp, &Control_center_destroyed, sizeof(int));
	}
#if defined(DXX_BUILD_DESCENT_I)
	PHYSFSX_writeBytes(fp, &LevelUniqueControlCenterState.Countdown_seconds_left, sizeof(int));
#elif defined(DXX_BUILD_DESCENT_II)
	PHYSFSX_writeBytes(fp, &LevelUniqueControlCenterState.Countdown_timer, sizeof(int));
#endif
	const unsigned Num_robot_centers = LevelSharedRobotcenterState.Num_robot_centers;
	PHYSFSX_writeBytes(fp, &Num_robot_centers, sizeof(int));
	range_for (auto &r, partial_const_range(RobotCenters, Num_robot_centers))
#if defined(DXX_BUILD_DESCENT_I)
		matcen_info_write(fp, r, STATE_MATCEN_VERSION);
#elif defined(DXX_BUILD_DESCENT_II)
		matcen_info_write(fp, r, 0x7f);
#endif
	control_center_triggers_write(ControlCenterTriggers, fp);
	const auto Num_fuelcenters = LevelUniqueFuelcenterState.Num_fuelcenters;
	PHYSFSX_writeBytes(fp, &Num_fuelcenters, sizeof(int));
	range_for (auto &s, partial_range(Station, Num_fuelcenters))
	{
#if defined(DXX_BUILD_DESCENT_I)
		// NOTE: Usually Descent1 handles countdown by Timer value of the Reactor Station. Since we now use Descent2 code to handle countdown (which we do in case there IS NO Reactor Station which causes potential trouble in Multiplayer), let's find the Reactor here and store the timer in it.
		if (s.Type == segment_special::controlcen)
			s.Timer = LevelUniqueControlCenterState.Countdown_timer;
#endif
		fuelcen_write(fp, s);
	}

// Save the control cen info
	{
		const int Control_center_been_hit = LevelUniqueControlCenterState.Control_center_been_hit;
		PHYSFSX_writeBytes(fp, &Control_center_been_hit, sizeof(int));
	}
	{
		const auto cc = static_cast<int>(LevelUniqueControlCenterState.Control_center_player_been_seen);
		PHYSFSX_writeBytes(fp, &cc, sizeof(int));
	}
	PHYSFSX_writeBytes(fp, &LevelUniqueControlCenterState.Frametime_until_next_fire, sizeof(int));
	{
		const int Control_center_present = LevelUniqueControlCenterState.Control_center_present;
		PHYSFSX_writeBytes(fp, &Control_center_present, sizeof(int));
	}
	{
		const auto Dead_controlcen_object_num = LevelUniqueControlCenterState.Dead_controlcen_object_num;
	int dead_controlcen_object_num = Dead_controlcen_object_num == object_none ? -1 : Dead_controlcen_object_num;
		PHYSFSX_writeBytes(fp, &dead_controlcen_object_num, sizeof(int));
	}

// Save the AI state
	ai_save_state( fp );

// Save the automap visited info
	PHYSFSX_writeBytes(fp, LevelUniqueAutomapState.Automap_visited.data(), std::max<std::size_t>(Highest_segment_index + 1, MAX_SEGMENTS_ORIGINAL));

	PHYSFSX_writeBytes(fp, &state_game_id, sizeof(unsigned));
	{
	PHYSFS_writeULE32(fp, cheats.rapidfire ? UINT32_MAX : 0);
#if defined(DXX_BUILD_DESCENT_I)
	PHYSFS_writeULE32(fp, 0); // was Ugly_robot_cheat
	PHYSFS_writeULE32(fp, 0); // was Ugly_robot_texture
	PHYSFS_writeULE32(fp, cheats.ghostphysics ? UINT32_MAX : 0);
#endif
	PHYSFS_writeULE32(fp, 0); // was Lunacy
#if defined(DXX_BUILD_DESCENT_II)
	PHYSFS_writeULE32(fp, 0); // was Lunacy, too... and one was Ugly robot stuff a long time ago...

	// Save automap marker info

	range_for (int m, MarkerState.imobjidx)
	{
		if (m == object_none)
			m = -1;
		PHYSFSX_writeBytes(fp, &m, sizeof(m));
	}
	PHYSFS_seek(fp, PHYSFS_tell(fp) + (NUM_MARKERS)*(CALLSIGN_LEN+1)); // MarkerOwner is obsolete
	range_for (const auto &m, MarkerState.message)
		PHYSFSX_writeBytes(fp, m.data(), m.size());

	PHYSFSX_writeBytes(fp, &Afterburner_charge, sizeof(fix));

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
		for (uint8_t j = static_cast<uint8_t>(primary_weapon_index_t::VULCAN_INDEX); j != static_cast<uint8_t>(primary_weapon_index_t::SUPER_LASER_INDEX); ++j)
		{
			if (Primary_last_was_super & HAS_PRIMARY_FLAG(primary_weapon_index_t{j}))
				last_was_super[j] = 1;
		}
		PHYSFSX_writeBytes(fp, &last_was_super, MAX_PRIMARY_WEAPONS);
		auto &Secondary_last_was_super = player_info.Secondary_last_was_super;
		for (uint8_t j = static_cast<uint8_t>(secondary_weapon_index_t::CONCUSSION_INDEX); j != static_cast<uint8_t>(secondary_weapon_index_t::SMISSILE1_INDEX); ++j)
		{
			if (Secondary_last_was_super & HAS_SECONDARY_FLAG(secondary_weapon_index_t{j}))
				last_was_super[j] = 1;
		}
		PHYSFSX_writeBytes(fp, &last_was_super, MAX_SECONDARY_WEAPONS);
	}

	//	Save flash effect stuff
	PHYSFSX_writeBytes(fp, &Flash_effect, sizeof(int));
	if (Time_flash_last_played - GameTime64 < F1_0*(-18000))
		tmptime32 = F1_0*(-18000);
	else
		tmptime32 = Time_flash_last_played - GameTime64;
	PHYSFSX_writeBytes(fp, &tmptime32, sizeof(fix));
	PHYSFSX_writeBytes(fp, &PaletteRedAdd, sizeof(int));
	PHYSFSX_writeBytes(fp, &PaletteGreenAdd, sizeof(int));
	PHYSFSX_writeBytes(fp, &PaletteBlueAdd, sizeof(int));
	{
		union {
			std::array<uint8_t, MAX_SEGMENTS> light_subtracted;
			std::array<uint8_t, MAX_SEGMENTS_ORIGINAL> light_subtracted_original;
		};
		const std::ranges::subrange r{vcsegptr};
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
			*j++ = underlying_value(useg.light_subtracted);
		PHYSFSX_writeBytes(fp, light_subtracted.data(), count);
	}
	PHYSFSX_writeBytes(fp, &First_secret_visit, sizeof(First_secret_visit));
	auto &Omega_charge = player_info.Omega_charge;
	PHYSFSX_writeBytes(fp, &Omega_charge, sizeof(Omega_charge));
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
		PHYSFSX_writeBytes(fp, Netgame.mission_title.data(), Netgame.mission_title.size());
		PHYSFSX_writeBytes(fp, Netgame.mission_name.data(), Netgame.mission_name.size());
		PHYSFSX_writeBytes(fp, &Netgame.levelnum, sizeof(int));
		PHYSFSX_writeBytes(fp, &Netgame.difficulty, sizeof(ubyte));
		PHYSFSX_writeBytes(fp, &Netgame.game_status, sizeof(ubyte));
		PHYSFSX_writeBytes(fp, &Netgame.numplayers, sizeof(ubyte));
		PHYSFSX_writeBytes(fp, &Netgame.max_numplayers, sizeof(ubyte));
		PHYSFSX_writeBytes(fp, &Netgame.numconnected, sizeof(ubyte));
		PHYSFSX_writeBytes(fp, &Netgame.level_time, sizeof(int));
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
	plobjnum->pos = compute_segment_center(vcvertptr, segp);
	obj_relink(vmobjptr, vmsegptr, plobjnum, segp);
	plobjnum->orient = LevelSharedSegmentState.Secret_return_orient;
	reset_player_object(*plobjnum);
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
		filenum = state_get_restore_file(*grd_curcanv, filename_storage, blind);
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
			if (PHYSFS_exists(temp_fname))
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
	int coop_player_got[MAX_PLAYERS], coop_org_objnum = get_local_player().objnum;
	std::array<per_side_array<texture1_value>, MAX_SEGMENTS> TempTmapNum;
	std::array<per_side_array<texture2_value>, MAX_SEGMENTS> TempTmapNum2;

#if defined(DXX_BUILD_DESCENT_I)
	static constexpr std::integral_constant<secret_restore, secret_restore::none> secret{};
#elif defined(DXX_BUILD_DESCENT_II)
	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	const auto old_gametime{GameTime64};
#endif

	#ifndef NDEBUG
	if (CGameArg.SysUsePlayersDir && strncmp(filename, PLAYER_DIRECTORY_TEXT, sizeof(PLAYER_DIRECTORY_TEXT) - 1))
		Int3();
	#endif

	auto fp = PHYSFSX_openReadBuffered(filename).first;
	if ( !fp ) return 0;

//Read id
	char id[5]{};
	PHYSFSX_readBytes(fp, id, sizeof(char) * 4);
	if ( memcmp( id, dgss_id, 4 )) {
		return 0;
	}

//Read version
	//Check for swapped file here, as dgss_id is written as a string (i.e. endian independent)
	int version{};
	PHYSFSX_readBytes(fp, &version, sizeof(int));
	const physfsx_endian swap{
		(version & 0xffff0000)
			? (version = SWAPINT(version), physfsx_endian::foreign)
			: physfsx_endian::native
	};	// if file is not endian native, have to swap all shorts and ints

	if (version < STATE_COMPATIBLE_VERSION)	{
		return 0;
	}

// Read Coop state_game_id. Oh the redundancy... we have this one later on but Coop games want to read this before loading a state so for easy access we have this here
	if (Game_mode & GM_MULTI_COOP)
	{
		callsign_t saved_callsign;
		state_game_id = PHYSFSX_readSXE32(fp, swap);
		PHYSFSX_readBytes(fp, &saved_callsign, sizeof(char) * CALLSIGN_LEN + 1);
		if (!(saved_callsign == get_local_player().callsign)) // check the callsign of the palyer who saved this state. It MUST match. If we transferred this savegame from pilot A to pilot B, others won't be able to restore us. So bail out here if this is the case.
		{
			return 0;
		}
	}

// Read description
	d_game_unique_state::savegame_description desc;
	PHYSFSX_readBytes(fp, desc.data(), 20);
	desc.back() = 0;

// Skip the current screen shot...
	PHYSFS_seek(fp, PHYSFS_tell(fp) + THUMBNAIL_W * THUMBNAIL_H);
#if defined(DXX_BUILD_DESCENT_II)
// And now...skip the goddamn palette stuff that somebody forgot to add
	PHYSFS_seek(fp, PHYSFS_tell(fp) + 768);
#endif
// Read the Between levels flag...
	PHYSFSX_skipBytes<4>(fp);

// Read the mission info...
	savegame_mission_path mission_pathname{};
	PHYSFSX_readBytes(fp, mission_pathname.original.data(), mission_pathname.original.size());
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
				PHYSFSX_readBytes(fp, mission_pathname.full.data(), mission_pathname.full.size());
				if (mission_pathname.full.back())
				{
					struct error_unknown_mission_format : passive_messagebox
					{
						error_unknown_mission_format() :
							passive_messagebox(menu_title{TXT_ERROR}, menu_subtitle{"Unable to load game\nUnrecognized mission name format"}, TXT_OK, grd_curscreen->sc_canvas)
						{
						}
					};
					run_blocking_newmenu<error_unknown_mission_format>();
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
			{
				struct error_unknown_save_format : passive_messagebox
				{
					error_unknown_save_format() :
						passive_messagebox(menu_title{TXT_ERROR}, menu_subtitle{"Unable to load game\nUnrecognized save game format"}, TXT_OK, grd_curscreen->sc_canvas)
						{
						}
				};
				run_blocking_newmenu<error_unknown_save_format>();
			}
			return 0;
	}

	if (const auto errstr = load_mission_by_name(mission_predicate, name_match_mode))
	{
		nm_messagebox(menu_title{TXT_ERROR}, {TXT_OK}, "Unable to load mission\n'%s'\n\n%s", mission_pathname.full.data(), errstr);
		return 0;
	}

//Read level info
	const auto current_level{PHYSFSX_readSXE32(fp, swap)};
	PHYSFSX_skipBytes<4>(fp);	// skip Next_level_num

//Restore GameTime
	GameTime64 = fix64{fix{PHYSFSX_readSXE32(fp, swap)}};

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

	if (const auto g = Game_wind)
		g->set_visible(0);

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
			StartNewLevelSub(LevelSharedRobotInfoState.Robot_info, current_level, 1, secret);

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
		uint8_t v{};
		PHYSFSX_readBytes(fp, &v, sizeof(v));
		Primary_weapon = primary_weapon_index_t{v};
	}
	auto &Secondary_weapon = pl_info.Secondary_weapon;
	{
		uint8_t v{};
		PHYSFSX_readBytes(fp, &v, sizeof(v));
		Secondary_weapon = secondary_weapon_index_t{v};
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
	cheats.enabled = !!PHYSFSX_readULE32(fp);
#if defined(DXX_BUILD_DESCENT_I)
	cheats.turbo = !!PHYSFSX_readULE32(fp);
#endif

	Do_appearance_effect = 0;			// Don't do this for middle o' game stuff.

	//Clear out all the objects from the lvl file
	for (unique_segment &useg : vmsegptr)
		useg.objects = object_none;
	reset_objects(LevelUniqueObjectState, 1);

	//Read objects, and pop 'em into their respective segments.
	{
		const auto i{PHYSFSX_readSXE32(fp, swap)};
	Objects.set_count(i);
	}
	for (auto &obj : vmobjptr)
	{
		object_rw obj_rw;
		PHYSFSX_readBytes(fp, &obj_rw, sizeof(obj_rw));
		object_rw_swap(&obj_rw, swap);
		state_object_rw_to_object(&obj_rw, obj);
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
		if (obj->type == OBJ_ROBOT && Robot_info[get_robot_id(obj)].boss_flag != boss_robot_id::None)
		{
			fix save_shields = obj->shields;

			copy_defaults_to_robot(Robot_info, obj);		//calculate starting shields

			//if in valid range, use loaded shield value
			if (save_shields > 0 && save_shields <= obj->shields)
				obj->shields = save_shields;
			else
				obj->shields /= 2;  //give player a break
		}
#endif
	}
	special_reset_objects(LevelUniqueObjectState, LevelSharedRobotInfoState.Robot_info);
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
		for (auto &w : Walls.vmptr)
			wall_read(fp, w);

#if defined(DXX_BUILD_DESCENT_II)
	//now that we have the walls, check if any sounds are linked to
	//walls that are now open
	for (const auto &w : Walls.vcptr)
	{
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
		for (auto &ad : ActiveDoors.vmptr)
			active_door_read(fp, ad);
	}

#if defined(DXX_BUILD_DESCENT_II)
	if (version >= 14) {		//Restore cloaking wall info
		unsigned num_cloaking_walls = PHYSFSX_readSXE32(fp, swap);
		auto &CloakingWalls = LevelUniqueWallSubsystemState.CloakingWalls;
		CloakingWalls.set_count(num_cloaking_walls);
		for (auto &w : CloakingWalls.vmptr)
			cloaking_wall_read(w, fp);
	}
#endif

	//Restore trigger info
	{
		auto &Triggers = LevelUniqueWallSubsystemState.Triggers;
	Triggers.set_count(PHYSFSX_readSXE32(fp, swap));
		for (auto &t : Triggers.vmptr)
			trigger_read(fp, t);
	}

	//Restore tmap info (to temp values so we can use compiled-in tmap info to compute static_light
	range_for (const auto &&segp, vmsegptridx)
	{
		for (const auto j : MAX_SIDES_PER_SEGMENT)
		{
			segp->shared_segment::sides[j].wall_num = wallnum_t{PHYSFSX_readUXE16(fp, swap)};
			TempTmapNum[segp][j] = texture1_value{PHYSFSX_readUXE16(fp, swap)};
			TempTmapNum2[segp][j] = texture2_value{PHYSFSX_readUXE16(fp, swap)};
		}
	}

	//Restore the fuelcen info
	LevelUniqueControlCenterState.Control_center_destroyed = PHYSFSX_readSXE32(fp, swap);
#if defined(DXX_BUILD_DESCENT_I)
	LevelUniqueControlCenterState.Countdown_seconds_left = {PHYSFSX_readSXE32(fp, swap)};
	LevelUniqueControlCenterState.Countdown_timer = 0;
#elif defined(DXX_BUILD_DESCENT_II)
	LevelUniqueControlCenterState.Countdown_timer = {PHYSFSX_readSXE32(fp, swap)};
#endif
	const unsigned Num_robot_centers = PHYSFSX_readSXE32(fp, swap);
	LevelSharedRobotcenterState.Num_robot_centers = Num_robot_centers;
	range_for (auto &r, partial_range(RobotCenters, Num_robot_centers))
#if defined(DXX_BUILD_DESCENT_I)
		matcen_info_read(fp, r, STATE_MATCEN_VERSION);
#elif defined(DXX_BUILD_DESCENT_II)
		matcen_info_read(fp, r);
#endif
	control_center_triggers_read(ControlCenterTriggers, fp);
	const unsigned Num_fuelcenters = PHYSFSX_readSXE32(fp, swap);
	LevelUniqueFuelcenterState.Num_fuelcenters = Num_fuelcenters;
	range_for (auto &s, partial_range(Station, Num_fuelcenters))
	{
		fuelcen_read(fp, s);
#if defined(DXX_BUILD_DESCENT_I)
		// NOTE: Usually Descent1 handles countdown by Timer value of the Reactor Station. Since we now use Descent2 code to handle countdown (which we do in case there IS NO Reactor Station which causes potential trouble in Multiplayer), let's find the Reactor here and read the timer from it.
		if (s.Type == segment_special::controlcen)
			LevelUniqueControlCenterState.Countdown_timer = s.Timer;
#endif
	}

	// Restore the control cen info
	LevelUniqueControlCenterState.Control_center_been_hit = PHYSFSX_readSXE32(fp, swap);
	LevelUniqueControlCenterState.Control_center_player_been_seen = static_cast<player_visibility_state>(PHYSFSX_readSXE32(fp, swap));
	LevelUniqueControlCenterState.Frametime_until_next_fire = {PHYSFSX_readSXE32(fp, swap)};
	LevelUniqueControlCenterState.Control_center_present = PHYSFSX_readSXE32(fp, swap);
	LevelUniqueControlCenterState.Dead_controlcen_object_num = PHYSFSX_readSXE32(fp, swap);
	if (LevelUniqueControlCenterState.Control_center_destroyed)
		LevelUniqueControlCenterState.Total_countdown_time = LevelUniqueControlCenterState.Countdown_timer / F0_5; // we do not need to know this, but it should not be 0 either...

	// Restore the AI state
	ai_restore_state(LevelSharedRobotInfoState.Robot_info, fp, version, swap);

	{
		auto &Automap_visited = LevelUniqueAutomapState.Automap_visited;
	// Restore the automap visited info
		Automap_visited = {};
		DXX_MAKE_MEM_UNDEFINED(std::span(Automap_visited));
		PHYSFSX_readBytes(fp, Automap_visited.data(), std::max<std::size_t>(Highest_segment_index + 1, MAX_SEGMENTS_ORIGINAL));
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
	player_info.Next_flare_fire_time = {GameTime64};
	Next_laser_fire_time = {GameTime64};
	Next_missile_fire_time = {GameTime64};

	state_game_id = 0;

	if ( version >= 7 )	{
		state_game_id = PHYSFSX_readSXE32(fp, swap);
		cheats.rapidfire = !!PHYSFSX_readULE32(fp);
		// PHYSFSX_readSXE32(fp, swap); // was Lunacy
		// PHYSFSX_readSXE32(fp, swap); // was Lunacy, too... and one was Ugly robot stuff a long time ago...
		PHYSFSX_skipBytes<8>(fp);
#if defined(DXX_BUILD_DESCENT_I)
		cheats.ghostphysics = !!PHYSFSX_readULE32(fp);
		PHYSFSX_skipBytes<4>(fp);
#endif
	}

#if defined(DXX_BUILD_DESCENT_II)
	if (version >= 17) {
		range_for (auto &i, MarkerState.imobjidx)
			i = vcobjidx_t::check_nothrow_index(PHYSFSX_readUXE32(fp, swap)).value_or(object_none);
		PHYSFS_seek(fp, PHYSFS_tell(fp) + (NUM_MARKERS)*(CALLSIGN_LEN+1)); // skip obsolete MarkerOwner
		range_for (auto &i, MarkerState.message)
		{
			std::array<char, MARKER_MESSAGE_LEN> a;
			PHYSFSX_readBytes(fp, a, a.size());
			i.copy_if(a);
		}
	}
	else {
		// skip dummy info
		const auto num{PHYSFSX_readSXE32(fp, swap)};           // was NumOfMarkers
		PHYSFSX_skipBytes<4>(fp);	// was CurMarker

		PHYSFS_seek(fp, PHYSFS_tell(fp) + num * (sizeof(vms_vector) + 40));

		range_for (auto &i, MarkerState.imobjidx)
			i = object_none;
	}

	if (version>=11) {
		if (secret != secret_restore::survived)
			Afterburner_charge = {PHYSFSX_readSXE32(fp, swap)};
		else {
			PHYSFSX_skipBytes<4>(fp);
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
		PHYSFSX_readBytes(fp, &last_was_super, MAX_PRIMARY_WEAPONS);
		Primary_last_was_super = 0;
		for (uint8_t j = static_cast<uint8_t>(primary_weapon_index_t::VULCAN_INDEX); j != static_cast<uint8_t>(primary_weapon_index_t::SUPER_LASER_INDEX); ++j)
		{
			if (last_was_super[j])
				Primary_last_was_super |= HAS_PRIMARY_FLAG(primary_weapon_index_t{j});
		}
		PHYSFSX_readBytes(fp, &last_was_super, MAX_SECONDARY_WEAPONS);
		auto &Secondary_last_was_super = player_info.Secondary_last_was_super;
		Secondary_last_was_super = 0;
		for (uint8_t j = static_cast<uint8_t>(secondary_weapon_index_t::CONCUSSION_INDEX); j != static_cast<uint8_t>(secondary_weapon_index_t::SMISSILE1_INDEX); ++j)
		{
			if (last_was_super[j])
				Secondary_last_was_super |= HAS_SECONDARY_FLAG(secondary_weapon_index_t{j});
		}
	}

	if (version >= 12) {
		Flash_effect = {PHYSFSX_readSXE32(fp, swap)};
		Time_flash_last_played = fix64{fix{PHYSFSX_readSXE32(fp, swap)}};
		PaletteRedAdd = {PHYSFSX_readSXE32(fp, swap)};
		PaletteGreenAdd = {PHYSFSX_readSXE32(fp, swap)};
		PaletteBlueAdd = {PHYSFSX_readSXE32(fp, swap)};
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
				PHYSFSX_readBytes(fp, &useg.light_subtracted, sizeof(useg.light_subtracted));
			}
		}
		else
		{
			range_for (unique_segment &i, partial_range(Segments, MAX_SEGMENTS_ORIGINAL))
			{
				PHYSFSX_readBytes(fp, &i.light_subtracted, sizeof(i.light_subtracted));
			}
		}
		apply_all_changed_light(LevelSharedDestructibleLightState, Segments.vmptridx);
	} else {
		for (unique_segment &useg : vmsegptr)
			useg.light_subtracted = {};
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
		const auto i{PHYSFSX_readSXE32(fp, swap)};
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
		for (auto &&[uside, t1, t2] : zip(segp->unique_segment::sides, TempTmapNum[segp], TempTmapNum2[segp]))
		{
			uside.tmap_num = t1;
			uside.tmap_num2 = t2;
		}
	}

// Read Coop Info
	if (Game_mode & GM_MULTI_COOP)
	{
		player restore_players[MAX_PLAYERS];
		object restore_objects[MAX_PLAYERS];

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
			if (restore_players[i].connected == player_connection_status::playing && obj->type == OBJ_PLAYER)
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
				if (vcplayerptr(i)->connected == player_connection_status::playing && restore_players[j].connected == player_connection_status::playing && vcplayerptr(i)->callsign == restore_players[j].callsign)
				{
					auto &p = *vmplayerptr(i);
					p = restore_players[j];
					coop_player_got[i] = 1;

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
					assert(obj->type == OBJ_GHOST);
					obj->type = OBJ_PLAYER;
					set_player_id(obj, i); // assign player object id to player number
					multi_reset_player_object(obj);
					update_object_seg(vmobjptr, LevelSharedSegmentState, LevelUniqueSegmentState, obj);
				}
			}
		}
		{
			std::array<char, MISSION_NAME_LEN + 1> a;
			PHYSFSX_readBytes(fp, a, a.size());
			Netgame.mission_title.copy_if(a);
		}
		{
			std::array<char, 9> a;
			PHYSFSX_readBytes(fp, a, a.size());
			Netgame.mission_name.copy_if(a);
		}
		Netgame.levelnum = {PHYSFSX_readSXE32(fp, swap)};
		PHYSFSX_readBytes(fp, &Netgame.difficulty, sizeof(ubyte));
		PHYSFSX_readBytes(fp, &Netgame.game_status, sizeof(ubyte));
		PHYSFSX_readBytes(fp, &Netgame.numplayers, sizeof(ubyte));
		PHYSFSX_readBytes(fp, &Netgame.max_numplayers, sizeof(ubyte));
		PHYSFSX_readBytes(fp, &Netgame.numconnected, sizeof(ubyte));
		Netgame.level_time = {PHYSFSX_readSXE32(fp, swap)};
		for (playernum_t i = 0; i < MAX_PLAYERS; i++)
		{
			const auto &&objp = vmobjptr(vcplayerptr(i)->objnum);
			auto &pi = objp->ctype.player_info;
			Netgame.killed[i] = pi.net_killed_total;
			Netgame.player_score[i] = pi.mission.score;
			Netgame.net_player_flags[i] = pi.powerup_flags;
		}
		for (playernum_t i = 0; i < MAX_PLAYERS; i++) // Disconnect connected players not available in this Savegame
			if (!coop_player_got[i] && vcplayerptr(i)->connected == player_connection_status::playing)
				multi_disconnect_player(i);
		Viewer = ConsoleObject = &get_local_plrobj(); // make sure Viewer and ConsoleObject are set up (which we skipped by not using InitPlayerObject but we need since objects changed while loading)
		special_reset_objects(LevelUniqueObjectState, LevelSharedRobotInfoState.Robot_info); // since we juggled around with objects to remap coop players rebuild the index of free objects
		state_set_next_autosave(GameUniqueState, Netgame.MPGameplayOptions.AutosaveInterval);
	}
	else
		state_set_next_autosave(GameUniqueState, PlayerCfg.SPGameplayOptions.AutosaveInterval);
	if (const auto g = Game_wind)
		if (!g->is_visible())
			g->set_visible(1);
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
	callsign_t saved_callsign;

	#ifndef NDEBUG
	if (CGameArg.SysUsePlayersDir && strncmp(filename.data(), PLAYER_DIRECTORY_TEXT, sizeof(PLAYER_DIRECTORY_TEXT) - 1))
		Int3();
	#endif

	if (!(Game_mode & GM_MULTI_COOP))
		return 0;

	auto fp = PHYSFSX_openReadBuffered(filename.data()).first;
	if ( !fp ) return 0;

//Read id
	char id[5]{};
	PHYSFSX_readBytes(fp, id, sizeof(char) * 4);
	if ( memcmp( id, dgss_id, 4 )) {
		return 0;
	}

//Read version
	//Check for swapped file here, as dgss_id is written as a string (i.e. endian independent)
	int version{};
	PHYSFSX_readBytes(fp, &version, sizeof(int));
	const physfsx_endian swap{
		(version & 0xffff0000)
			? (version = SWAPINT(version), physfsx_endian::foreign)
			: physfsx_endian::native
	};	// if file is not endian native, have to swap all shorts and ints

	if (version < STATE_COMPATIBLE_VERSION)	{
		return 0;
	}

// Read Coop state_game_id to validate the savegame we are about to load matches the others
	state_game_id = PHYSFSX_readSXE32(fp, swap);
	PHYSFSX_readBytes(fp, &saved_callsign, sizeof(char) * CALLSIGN_LEN + 1);
	if (!(saved_callsign == get_local_player().callsign)) // check the callsign of the palyer who saved this state. It MUST match. If we transferred this savegame from pilot A to pilot B, others won't be able to restore us. So bail out here if this is the case.
		return 0;

	return state_game_id;
}

}
