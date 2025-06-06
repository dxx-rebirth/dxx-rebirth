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
 * Save game information
 *
 */

#include <stdexcept>
#include <stdio.h>
#include <string.h>
#include <ranges>
#include "pstypes.h"
#include "strutil.h"
#include "console.h"
#include "gr.h"
#include "palette.h"
#include "newmenu.h"
#include "inferno.h"
#if DXX_USE_EDITOR
#include "editor/editor.h"
#include "editor/esegment.h"
#include "editor/eswitch.h"
#endif
#include "dxxerror.h"
#include "object.h"
#include "game.h"
#include "gameseg.h"
#include "wall.h"
#include "gamemine.h"
#include "robot.h"
#include "bm.h"
#include "fireball.h"
#include "switch.h"
#include "fuelcen.h"
#include "cntrlcen.h"
#include "powerup.h"
#include "weapon.h"
#include "player.h"
#include "newdemo.h"
#include "gameseq.h"
#include "polyobj.h"
#include "text.h"
#include "gamefont.h"
#include "gamesave.h"
#include "gamepal.h"
#include "physics.h"
#include "laser.h"
#include "multi.h"
#include "makesig.h"
#include "textures.h"
#include "d_enumerate.h"
#include "d_range.h"
#include "vclip.h"
#include "compiler-range_for.h"
#include "d_construct.h"
#include "d_levelstate.h"
#include "d_underlying_value.h"
#include "d_zip.h"
#include "partial_range.h"

namespace dcx {

namespace {

using savegame_pof_names_type = enumerated_array<char[FILENAME_LEN], MAX_POLYGON_MODELS, polygon_model_index>;
constexpr uint16_t GAME_COMPATIBLE_VERSION{22};

}

}

int Gamesave_current_version;

namespace dsx {

namespace {

#if DXX_BUILD_DESCENT == 1
constexpr unsigned GAME_VERSION{25};
#if DXX_USE_EDITOR
constexpr unsigned LEVEL_FILE_VERSION{1};
#endif
#elif DXX_BUILD_DESCENT == 2
constexpr unsigned GAME_VERSION{32};
#if DXX_USE_EDITOR
constexpr unsigned LEVEL_FILE_VERSION{8};
#endif
#endif

}

}

//version 28->29  add delta light support
//version 27->28  controlcen id now is reactor number, not model number
//version 28->29  ??
//version 29->30  changed trigger structure
//version 30->31  changed trigger structure some more
//version 31->32  change segment structure, make it 512 bytes w/o editor, add Segment2s array.

int Gamesave_num_org_robots{0};

#if DXX_USE_EDITOR
namespace {
// Return true if this level has a name of the form "level??"
// Note that a pathspec can appear at the beginning of the filename.
static int is_real_level(const char *filename)
{
	int len = strlen(filename);

	if (len < 6)
		return 0;

	return !d_strnicmp(&filename[len-11], "level");
}
}
#endif

//--unused-- vms_angvec zero_angles={0,0,0};

int Gamesave_num_players{0};

namespace dsx {
#if DXX_BUILD_DESCENT == 1
namespace {


static vclip_index convert_vclip(const d_vclip_array &Vclip, const int vc)
{
	if (vc < 0)
		return vclip_index::None;
	if (const auto o = Vclip.valid_index(vc))
	{
		if (const auto vci{*o}; Vclip[vci].num_frames != ~0u)
			return vci;
	}
	return vclip_index{0};
}

static int convert_wclip(int wc) {
	return (wc < Num_wall_anims) ? wc : wc % Num_wall_anims;
}

}

int convert_tmap(int tmap)
{
	if (tmap == -1)
		return tmap;
    return (tmap >= NumTextures) ? tmap % NumTextures : tmap;
}

namespace {
static unsigned convert_polymod(const unsigned N_polygon_models, const unsigned polymod)
{
    return (polymod >= N_polygon_models) ? polymod % N_polygon_models : polymod;
}
}
#endif

namespace {

static void verify_object(const d_level_shared_robot_info_state &LevelSharedRobotInfoState, const d_vclip_array &Vclip, object &obj, const std::span<char[FILENAME_LEN]> Save_pof_names)
{
	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	obj.lifeleft = IMMORTAL_TIME;		//all loaded object are immortal, for now

	auto &Polygon_models = LevelSharedPolygonModelState.Polygon_models;
	if (obj.type == OBJ_ROBOT)
	{
		Gamesave_num_org_robots++;

		// Make sure valid id...
		const auto N_robot_types = LevelSharedRobotInfoState.N_robot_types;
		if (const auto id = underlying_value(get_robot_id(obj)); id >= N_robot_types)
			set_robot_id(obj, static_cast<robot_id>(id % N_robot_types));

		// Make sure model number & size are correct...
		if (obj.render_type == render_type::RT_POLYOBJ)
		{
			auto &ri = Robot_info[get_robot_id(obj)];
#if DXX_BUILD_DESCENT == 2
			assert(ri.model_num != polygon_model_index::None);
				//if you fail this assert, it means that a robot in this level
				//hasn't been loaded, possibly because he's marked as
				//non-shareware.  To see what robot number, print obj.id.

			assert(ri.always_0xabcd == 0xabcd);
				//if you fail this assert, it means that the robot_ai for
				//a robot in this level hasn't been loaded, possibly because
				//it's marked as non-shareware.  To see what robot number,
				//print obj.id.
#endif

			obj.rtype.pobj_info.model_num = ri.model_num;
			obj.size = Polygon_models[obj.rtype.pobj_info.model_num].rad;

			//@@if (obj.control_source==CT_AI && Robot_info[obj.id].attack_type)
			//@@	obj.size = obj.size*3/4;
		}

#if DXX_BUILD_DESCENT == 2
		if (get_robot_id(obj) == robot_id::special_reactor)						//special "reactor" robots
			obj.movement_source = object::movement_type::None;
#endif

		if (obj.movement_source == object::movement_type::physics)
		{
			auto &ri = Robot_info[get_robot_id(obj)];
			obj.mtype.phys_info.mass = ri.mass;
			obj.mtype.phys_info.drag = ri.drag;
		}
	}
	else {		//Robots taken care of above
		if (obj.render_type == render_type::RT_POLYOBJ)
		{
			if (const auto model_num{underlying_value(obj.rtype.pobj_info.model_num)}; model_num < Save_pof_names.size())
			{
				const auto name{Save_pof_names[model_num]};
			for (auto &&[i, candidate_name] : enumerate(partial_range(LevelSharedPolygonModelState.Pof_names, LevelSharedPolygonModelState.N_polygon_models)))
				if (!d_stricmp(candidate_name, name)) {		//found it!	
					obj.rtype.pobj_info.model_num = i;
					break;
				}
			}
		}
	}

	if (obj.type == OBJ_POWERUP)
	{
		if (underlying_value(get_powerup_id(obj)) >= N_powerup_types)
		{
			set_powerup_id(Powerup_info, Vclip, obj, powerup_type_t::POW_SHIELD_BOOST);
			assert(obj.render_type != render_type::RT_POLYOBJ);
		}
		obj.control_source = object::control_type::powerup;
		obj.size = Powerup_info[get_powerup_id(obj)].size;
		obj.ctype.powerup_info.creation_time = 0;
	}

	if (obj.type == OBJ_WEAPON)
	{
		if ( get_weapon_id(obj) >= N_weapon_types )	{
			set_weapon_id(obj, weapon_id_type::LASER_ID_L1);
			assert(obj.render_type != render_type::RT_POLYOBJ);
		}

#if DXX_BUILD_DESCENT == 2
		const auto weapon_id = get_weapon_id(obj);
		if (weapon_id == weapon_id_type::PMINE_ID)
		{		//make sure pmines have correct values
			obj.mtype.phys_info.mass = Weapon_info[weapon_id].mass;
			obj.mtype.phys_info.drag = Weapon_info[weapon_id].drag;
			obj.mtype.phys_info.flags |= PF_FREE_SPINNING;

			// Make sure model number & size are correct...		
			assert(obj.render_type == render_type::RT_POLYOBJ);

			obj.rtype.pobj_info.model_num = Weapon_info[weapon_id].model_num;
			obj.size = Polygon_models[obj.rtype.pobj_info.model_num].rad;
		}
#endif
	}

	if (obj.type == OBJ_CNTRLCEN)
	{
		obj.render_type = render_type::RT_POLYOBJ;
		obj.control_source = object::control_type::cntrlcen;

#if DXX_BUILD_DESCENT == 1
		// Make model number is correct...	
		for (int i=0; i<Num_total_object_types; i++ )	
			if ( ObjType[i] == OL_CONTROL_CENTER )		{
				obj.rtype.pobj_info.model_num = ObjId[i];
				obj.shields = ObjStrength[i];
				break;		
			}
#elif DXX_BUILD_DESCENT == 2
		if (Gamesave_current_version <= 1) { // descent 1 reactor
			set_reactor_id(obj, 0);                         // used to be only one kind of reactor
			obj.rtype.pobj_info.model_num = Reactors[0].model_num;// descent 1 reactor
		}

		// Make sure model number is correct...
		//obj.rtype.pobj_info.model_num = Reactors[obj.id].model_num;
#endif
	}

	if (obj.type == OBJ_PLAYER)
	{
		//int i;

		//Assert(obj == Player);

		if (&obj == ConsoleObject)
			init_player_object(LevelSharedPolygonModelState, obj);
		else
			if (obj.render_type == render_type::RT_POLYOBJ)	//recover from Matt's pof file matchup bug
				obj.rtype.pobj_info.model_num = Player_ship->model_num;

		//Make sure orient matrix is orthogonal
		check_and_fix_matrix(obj.orient);

		set_player_id(obj, Gamesave_num_players++);
	}

	if (obj.type == OBJ_HOSTAGE)
	{
		obj.render_type = render_type::RT_HOSTAGE;
		obj.control_source = object::control_type::powerup;
	}
}

//reads one object of the given version from the given file
static void read_object(const vmobjptr_t obj, const NamedPHYSFS_File f, int version)
{
	DXX_POISON_MEMORY(std::span<object>(&*obj, 1), 0xfd);
	obj->signature = object_signature_t{0};
	set_object_type(*obj, PHYSFSX_readByte(f));
	obj->id             = PHYSFSX_readByte(f);

	if (obj->type == OBJ_ROBOT)
	{
#if DXX_BUILD_DESCENT == 1
		if (const auto id = underlying_value(get_robot_id(obj)); id >= 24)
			set_robot_id(obj, static_cast<robot_id>(id % 24));
#endif
		obj->matcen_creator = 0;
	}
	{
		uint8_t ctype = PHYSFSX_readByte(f);
		switch (typename object::control_type{ctype})
		{
			case object::control_type::None:
			case object::control_type::ai:
			case object::control_type::explosion:
			case object::control_type::flying:
			case object::control_type::slew:
			case object::control_type::flythrough:
			case object::control_type::weapon:
			case object::control_type::repaircen:
			case object::control_type::morph:
			case object::control_type::debris:
			case object::control_type::powerup:
			case object::control_type::light:
			case object::control_type::remote:
			case object::control_type::cntrlcen:
				break;
			default:
				ctype = static_cast<uint8_t>(object::control_type::None);
				break;
		}
		obj->control_source = typename object::control_type{ctype};
	}
	{
		uint8_t mtype = PHYSFSX_readByte(f);
		switch (typename object::movement_type{mtype})
		{
			case object::movement_type::None:
			case object::movement_type::physics:
			case object::movement_type::spinning:
				break;
			default:
				mtype = static_cast<uint8_t>(object::movement_type::None);
				break;
		}
		obj->movement_source = typename object::movement_type{mtype};
	}
	if (const uint8_t rtype = PHYSFSX_readByte(f); valid_render_type(rtype))
		obj->render_type = render_type{rtype};
	else
	{
		LevelError("Level contains bogus render type %#x for object %p; using none instead", rtype, &*obj);
		obj->render_type = render_type::RT_NONE;
	}
	obj->flags          = PHYSFSX_readByte(f);

	obj->segnum = read_untrusted_segnum_le16(f);
	obj->attached_obj   = object_none;

	PHYSFSX_readVector(f, obj->pos);
	PHYSFSX_readVector(f, obj->orient.rvec);
	PHYSFSX_readVector(f, obj->orient.uvec);
	PHYSFSX_readVector(f, obj->orient.fvec);

	obj->size           = PHYSFSX_readFix(f);
	obj->shields        = PHYSFSX_readFix(f);

	{
		vms_vector last_pos;
		PHYSFSX_readVector(f, last_pos);
	}

	const uint8_t untrusted_contains_type = PHYSFSX_readByte(f);
	const uint8_t untrusted_contains_id = PHYSFSX_readByte(f);
	const uint8_t untrusted_contains_count = PHYSFSX_readByte(f);
	obj->contains = build_contained_object_parameters_from_untrusted(untrusted_contains_type, untrusted_contains_id, untrusted_contains_count);

	switch (obj->movement_source) {

		case object::movement_type::physics:

			PHYSFSX_readVector(f, obj->mtype.phys_info.velocity);
			PHYSFSX_readVector(f, obj->mtype.phys_info.thrust);

			obj->mtype.phys_info.mass		= PHYSFSX_readFix(f);
			obj->mtype.phys_info.drag		= PHYSFSX_readFix(f);
			PHYSFSX_skipBytes<4>(f);	/* brakes */

			PHYSFSX_readVector(f, obj->mtype.phys_info.rotvel);
			PHYSFSX_readVector(f, obj->mtype.phys_info.rotthrust);

			obj->mtype.phys_info.turnroll	= PHYSFSX_readFixAng(f);
			obj->mtype.phys_info.flags		= PHYSFSX_readSLE16(f);

			break;

		case object::movement_type::spinning:

			PHYSFSX_readVector(f, obj->mtype.spin_rate);
			break;

		case object::movement_type::None:
			break;

		default:
			Int3();
	}

	switch (obj->control_source) {

		case object::control_type::ai: {
			obj->ctype.ai_info.behavior				= static_cast<ai_behavior>(PHYSFSX_readByte(f));

			std::array<int8_t, 11> ai_info_flags{};
			PHYSFSX_readBytes(f, ai_info_flags.data(), 11);
			{
				const uint8_t gun_num = ai_info_flags[0];
				obj->ctype.ai_info.CURRENT_GUN = (gun_num < MAX_GUNS) ? robot_gun_number{gun_num} : robot_gun_number{};
			}
			obj->ctype.ai_info.CURRENT_STATE = build_ai_state_from_untrusted(ai_info_flags[1]).value();
			obj->ctype.ai_info.GOAL_STATE = build_ai_state_from_untrusted(ai_info_flags[2]).value();
			obj->ctype.ai_info.PATH_DIR = ai_info_flags[3];
#if DXX_BUILD_DESCENT == 1
			obj->ctype.ai_info.SUBMODE = ai_info_flags[4];
#elif DXX_BUILD_DESCENT == 2
			obj->ctype.ai_info.SUB_FLAGS = ai_info_flags[4];
#endif
			obj->ctype.ai_info.GOALSIDE = build_sidenum_from_untrusted(ai_info_flags[5]).value();
			obj->ctype.ai_info.CLOAKED = ai_info_flags[6];
			obj->ctype.ai_info.SKIP_AI_COUNT = ai_info_flags[7];
			obj->ctype.ai_info.REMOTE_OWNER = ai_info_flags[8];
			obj->ctype.ai_info.REMOTE_SLOT_NUM = ai_info_flags[9];
			obj->ctype.ai_info.hide_segment = read_untrusted_segnum_le16(f);
			obj->ctype.ai_info.hide_index			= PHYSFSX_readSLE16(f);
			obj->ctype.ai_info.path_length			= PHYSFSX_readSLE16(f);
			obj->ctype.ai_info.cur_path_index		= PHYSFSX_readSLE16(f);

			if (version <= 25) {
				//				obj->ctype.ai_info.follow_path_start_seg	= 
				//				obj->ctype.ai_info.follow_path_end_seg		= 
				PHYSFSX_skipBytes<4>(f);
			}

			break;
		}

		case object::control_type::explosion:

			obj->ctype.expl_info.spawn_time		= PHYSFSX_readFix(f);
			obj->ctype.expl_info.delete_time		= PHYSFSX_readFix(f);
			obj->ctype.expl_info.delete_objnum	= PHYSFSX_readSLE16(f);
			obj->ctype.expl_info.next_attach = obj->ctype.expl_info.prev_attach = obj->ctype.expl_info.attach_parent = object_none;

			break;

		case object::control_type::weapon:

			//do I really need to read these?  Are they even saved to disk?

			obj->ctype.laser_info.parent_type		= PHYSFSX_readSLE16(f);
			obj->ctype.laser_info.parent_num		= PHYSFSX_readSLE16(f);
			obj->ctype.laser_info.parent_signature	= object_signature_t{static_cast<uint16_t>(PHYSFSX_readInt(f))};
#if DXX_BUILD_DESCENT == 2
			obj->ctype.laser_info.last_afterburner_time = 0;
#endif
			obj->ctype.laser_info.clear_hitobj();

			break;

		case object::control_type::light:

			obj->ctype.light_info.intensity = PHYSFSX_readFix(f);
			break;

		case object::control_type::powerup:

			if (version >= 25)
				obj->ctype.powerup_info.count = PHYSFSX_readInt(f);
			else
				obj->ctype.powerup_info.count = 1;

			if (obj->type == OBJ_POWERUP)
			{
				/* Objects loaded from a level file were not ejected by
				 * the player.
				 */
				obj->ctype.powerup_info.flags = 0;
				/* Hostages have control type object::control_type::powerup, but object
				 * type OBJ_HOSTAGE.  Hostages are never weapons, so
				 * prevent checking their IDs.
				 */
			if (get_powerup_id(obj) == powerup_type_t::POW_VULCAN_WEAPON)
					obj->ctype.powerup_info.count = VULCAN_WEAPON_AMMO_AMOUNT;

#if DXX_BUILD_DESCENT == 2
			else if (get_powerup_id(obj) == powerup_type_t::POW_GAUSS_WEAPON)
					obj->ctype.powerup_info.count = VULCAN_WEAPON_AMMO_AMOUNT;

			else if (get_powerup_id(obj) == powerup_type_t::POW_OMEGA_WEAPON)
					obj->ctype.powerup_info.count = MAX_OMEGA_CHARGE;
#endif
			}

			break;


		case object::control_type::None:
		case object::control_type::flying:
		case object::control_type::debris:
			break;

		case object::control_type::slew:		//the player is generally saved as slew
			break;

		case object::control_type::cntrlcen:
			break;

		case object::control_type::morph:
		case object::control_type::flythrough:
		case object::control_type::repaircen:
		default:
			Int3();
	
	}

	switch (obj->render_type) {

		case render_type::RT_NONE:
			break;

		case render_type::RT_MORPH:
		case render_type::RT_POLYOBJ: {
			int tmo;

			obj->rtype.pobj_info.model_num = build_polygon_model_index_from_untrusted(
#if DXX_BUILD_DESCENT == 1
				convert_polymod(LevelSharedPolygonModelState.N_polygon_models, PHYSFSX_readInt(f))
#elif DXX_BUILD_DESCENT == 2
				PHYSFSX_readInt(f)
#endif
			);

			range_for (auto &i, obj->rtype.pobj_info.anim_angles)
				PHYSFSX_readAngleVec(f, i);

			obj->rtype.pobj_info.subobj_flags	= PHYSFSX_readInt(f);

			tmo = PHYSFSX_readInt(f);

#if !DXX_USE_EDITOR
#if DXX_BUILD_DESCENT == 1
			obj->rtype.pobj_info.tmap_override	= convert_tmap(tmo);
#elif DXX_BUILD_DESCENT == 2
			obj->rtype.pobj_info.tmap_override	= tmo;
#endif
			#else
			if (tmo==-1)
				obj->rtype.pobj_info.tmap_override	= -1;
			else {
				int xlated_tmo = tmap_xlate_table[tmo];
				if (xlated_tmo < 0)	{
					Int3();
					xlated_tmo = 0;
				}
				obj->rtype.pobj_info.tmap_override	= xlated_tmo;
			}
			#endif

			obj->rtype.pobj_info.alt_textures	= 0;

			break;
		}

		case render_type::RT_WEAPON_VCLIP:
		case render_type::RT_HOSTAGE:
		case render_type::RT_POWERUP:
		case render_type::RT_FIREBALL:

#if DXX_BUILD_DESCENT == 1
			obj->rtype.vclip_info.vclip_num	= convert_vclip(Vclip, PHYSFSX_readInt(f));
#elif DXX_BUILD_DESCENT == 2
			obj->rtype.vclip_info.vclip_num	= build_vclip_index_from_untrusted(PHYSFSX_readInt(f));
#endif
			obj->rtype.vclip_info.frametime	= PHYSFSX_readFix(f);
			obj->rtype.vclip_info.framenum	= PHYSFSX_readByte(f);

			break;

		case render_type::RT_LASER:
			break;

		default:
			Int3();

	}

}
}
}

#if DXX_USE_EDITOR
namespace {
static int PHYSFSX_writeMatrix(PHYSFS_File *file, const vms_matrix &m)
{
	if (PHYSFSX_writeVector(file, m.rvec) < 1 ||
		PHYSFSX_writeVector(file, m.uvec) < 1 ||
		PHYSFSX_writeVector(file, m.fvec) < 1)
		return 0;
	return 1;
}

static int PHYSFSX_writeAngleVec(PHYSFS_File *file, const vms_angvec &v)
{
	if (PHYSFSX_writeFixAng(file, v.p) < 1 ||
		PHYSFSX_writeFixAng(file, v.b) < 1 ||
		PHYSFSX_writeFixAng(file, v.h) < 1)
		return 0;
	return 1;
}
}

//writes one object to the given file
namespace dsx {
namespace {
static void write_object(const object &obj, short version, PHYSFS_File *f)
{
#if DXX_BUILD_DESCENT == 1
	(void)version;
#endif
	PHYSFSX_writeU8(f, obj.type);
	PHYSFSX_writeU8(f, obj.id);

	PHYSFSX_writeU8(f, underlying_value(obj.control_source));
	PHYSFSX_writeU8(f, underlying_value(obj.movement_source));
	PHYSFSX_writeU8(f, underlying_value(obj.render_type));
	PHYSFSX_writeU8(f, obj.flags);

	PHYSFS_writeSLE16(f, obj.segnum);

	PHYSFSX_writeVector(f, obj.pos);
	PHYSFSX_writeMatrix(f, obj.orient);

	PHYSFSX_writeFix(f, obj.size);
	PHYSFSX_writeFix(f, obj.shields);

	PHYSFSX_writeVector(f, obj.pos);

	PHYSFSX_writeU8(f, underlying_value(obj.contains.type));
	PHYSFSX_writeU8(f, underlying_value(obj.contains.id.robot));
	PHYSFSX_writeU8(f, obj.contains.count);

	switch (obj.movement_source) {

		case object::movement_type::physics:

	 		PHYSFSX_writeVector(f, obj.mtype.phys_info.velocity);
			PHYSFSX_writeVector(f, obj.mtype.phys_info.thrust);

			PHYSFSX_writeFix(f, obj.mtype.phys_info.mass);
			PHYSFSX_writeFix(f, obj.mtype.phys_info.drag);
			PHYSFSX_writeFix(f, 0);	/* brakes */

			PHYSFSX_writeVector(f, obj.mtype.phys_info.rotvel);
			PHYSFSX_writeVector(f, obj.mtype.phys_info.rotthrust);

			PHYSFSX_writeFixAng(f, obj.mtype.phys_info.turnroll);
			PHYSFS_writeSLE16(f, obj.mtype.phys_info.flags);

			break;

		case object::movement_type::spinning:

			PHYSFSX_writeVector(f, obj.mtype.spin_rate);
			break;

		case object::movement_type::None:
			break;

		default:
			Int3();
	}

	switch (obj.control_source) {

		case object::control_type::ai: {
			PHYSFSX_writeU8(f, static_cast<uint8_t>(obj.ctype.ai_info.behavior));

			std::array<int8_t, 11> ai_info_flags{};
			ai_info_flags[0] = underlying_value(obj.ctype.ai_info.CURRENT_GUN);
			ai_info_flags[1] = obj.ctype.ai_info.CURRENT_STATE;
			ai_info_flags[2] = obj.ctype.ai_info.GOAL_STATE;
			ai_info_flags[3] = obj.ctype.ai_info.PATH_DIR;
#if DXX_BUILD_DESCENT == 1
			ai_info_flags[4] = obj.ctype.ai_info.SUBMODE;
#elif DXX_BUILD_DESCENT == 2
			ai_info_flags[4] = obj.ctype.ai_info.SUB_FLAGS;
#endif
			ai_info_flags[5] = underlying_value(obj.ctype.ai_info.GOALSIDE);
			ai_info_flags[6] = obj.ctype.ai_info.CLOAKED;
			ai_info_flags[7] = obj.ctype.ai_info.SKIP_AI_COUNT;
			ai_info_flags[8] = obj.ctype.ai_info.REMOTE_OWNER;
			ai_info_flags[9] = obj.ctype.ai_info.REMOTE_SLOT_NUM;
			PHYSFSX_writeBytes(f, ai_info_flags.data(), 11);
			PHYSFS_writeSLE16(f, obj.ctype.ai_info.hide_segment);
			PHYSFS_writeSLE16(f, obj.ctype.ai_info.hide_index);
			PHYSFS_writeSLE16(f, obj.ctype.ai_info.path_length);
			PHYSFS_writeSLE16(f, obj.ctype.ai_info.cur_path_index);

#if DXX_BUILD_DESCENT == 1
			PHYSFS_writeSLE16(f, segment_none);
			PHYSFS_writeSLE16(f, segment_none);
#elif DXX_BUILD_DESCENT == 2
			if (version <= 25)
			{
				PHYSFS_writeSLE16(f, -1);	//obj.ctype.ai_info.follow_path_start_seg
				PHYSFS_writeSLE16(f, -1);	//obj.ctype.ai_info.follow_path_end_seg
			}
#endif

			break;
		}

		case object::control_type::explosion:

			PHYSFSX_writeFix(f, obj.ctype.expl_info.spawn_time);
			PHYSFSX_writeFix(f, obj.ctype.expl_info.delete_time);
			PHYSFS_writeSLE16(f, obj.ctype.expl_info.delete_objnum);

			break;

		case object::control_type::weapon:

			//do I really need to write these objects?

			PHYSFS_writeSLE16(f, obj.ctype.laser_info.parent_type);
			PHYSFS_writeSLE16(f, obj.ctype.laser_info.parent_num);
			PHYSFS_writeSLE32(f, static_cast<uint16_t>(obj.ctype.laser_info.parent_signature));

			break;

		case object::control_type::light:

			PHYSFSX_writeFix(f, obj.ctype.light_info.intensity);
			break;

		case object::control_type::powerup:

#if DXX_BUILD_DESCENT == 1
			PHYSFS_writeSLE32(f, obj.ctype.powerup_info.count);
#elif DXX_BUILD_DESCENT == 2
			if (version >= 25)
				PHYSFS_writeSLE32(f, obj.ctype.powerup_info.count);
#endif
			break;

		case object::control_type::None:
		case object::control_type::flying:
		case object::control_type::debris:
			break;

		case object::control_type::slew:		//the player is generally saved as slew
			break;

		case object::control_type::cntrlcen:
			break;			//control center object.

		case object::control_type::morph:
		case object::control_type::repaircen:
		case object::control_type::flythrough:
		default:
			Int3();
	
	}

	switch (obj.render_type) {

		case render_type::RT_NONE:
			break;

		case render_type::RT_MORPH:
		case render_type::RT_POLYOBJ: {
			PHYSFS_writeSLE32(f, obj.rtype.pobj_info.model_num == polygon_model_index::None ? -1 : underlying_value(obj.rtype.pobj_info.model_num));

			range_for (auto &i, obj.rtype.pobj_info.anim_angles)
				PHYSFSX_writeAngleVec(f, i);

			PHYSFS_writeSLE32(f, obj.rtype.pobj_info.subobj_flags);

			PHYSFS_writeSLE32(f, obj.rtype.pobj_info.tmap_override);

			break;
		}

		case render_type::RT_WEAPON_VCLIP:
		case render_type::RT_HOSTAGE:
		case render_type::RT_POWERUP:
		case render_type::RT_FIREBALL:

			PHYSFS_writeSLE32(f, underlying_value(obj.rtype.vclip_info.vclip_num));
			PHYSFSX_writeFix(f, obj.rtype.vclip_info.frametime);
			PHYSFSX_writeU8(f, obj.rtype.vclip_info.framenum);

			break;

		case render_type::RT_LASER:
			break;

		default:
			Int3();

	}

}
}
}
#endif

// --------------------------------------------------------------------
// Load game 
// Loads all the relevant data for a level.
// If level != -1, it loads the filename with extension changed to .min
// Otherwise it loads the appropriate level mine.
// returns 0=everything ok, 1=old version, -1=error
namespace dsx {
namespace {

static void validate_segment_wall(const vcsegptridx_t seg, shared_side &side, const sidenum_t sidenum)
{
	auto &rwn0 = side.wall_num;
	const auto wn0{rwn0};
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcwallptr = Walls.vcptr;
	auto &w0 = *vcwallptr(wn0);
	switch (w0.type)
	{
		case WALL_DOOR:
			{
				const auto connected_seg = seg->shared_segment::children[sidenum];
				if (connected_seg == segment_none)
				{
					rwn0 = wall_none;
					LevelError("segment %hu side %u wall %hu has no child segment; removing orphan wall.", seg.get_unchecked_index(), underlying_value(sidenum), underlying_value(wn0));
					return;
				}
				const shared_segment &vcseg = *vcsegptr(connected_seg);
				const auto connected_side = find_connect_side(seg, vcseg);
				if (connected_side == side_none)
				{
					rwn0 = wall_none;
					LevelError("segment %hu side %u wall %u has child segment %hu side %u, but child segment does not link back; removing orphan wall.", seg.get_unchecked_index(), underlying_value(sidenum), underlying_value(wn0), connected_seg, underlying_value(connected_side));
					return;
				}
				const auto wn1 = vcseg.sides[connected_side].wall_num;
				if (wn1 == wall_none)
				{
					rwn0 = wall_none;
					LevelError("segment %hu side %u wall %u has child segment %hu side %u, but no wall; removing orphan wall.", seg.get_unchecked_index(), underlying_value(sidenum), static_cast<unsigned>(wn0), connected_seg, underlying_value(connected_side));
					return;
				}
			}
			break;
		default:
			break;
	}
}

static int load_game_data(
#if DXX_BUILD_DESCENT == 2
	d_level_shared_destructible_light_state &LevelSharedDestructibleLightState,
#endif
	fvmobjptridx &vmobjptridx, fvmsegptridx &vmsegptridx, const NamedPHYSFS_File LoadFile)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	auto &WallAnims = GameSharedState.WallAnims;
	auto &RobotCenters = LevelSharedRobotcenterState.RobotCenters;
	const auto &vcsegptridx = vmsegptridx;
	short game_top_fileinfo_version;
	int object_offset;
	unsigned gs_num_objects;
	int trig_size;

	//===================== READ FILE INFO ========================

	// Check signature
	if (PHYSFSX_readSLE16(LoadFile) != 0x6705)
		return -1;

	// Read and check version number
	game_top_fileinfo_version = PHYSFSX_readSLE16(LoadFile);
	if (game_top_fileinfo_version < GAME_COMPATIBLE_VERSION )
		return -1;

	// We skip some parts of the former game_top_fileinfo
	PHYSFSX_fseek(LoadFile, 31, SEEK_CUR);

	object_offset = PHYSFSX_readInt(LoadFile);
	gs_num_objects = PHYSFSX_readInt(LoadFile);
	PHYSFSX_fseek(LoadFile, 8, SEEK_CUR);

	init_exploding_walls();
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	Walls.set_count(PHYSFSX_readInt(LoadFile));
	PHYSFSX_fseek(LoadFile, 20, SEEK_CUR);

	auto &Triggers = LevelUniqueWallSubsystemState.Triggers;
	Triggers.set_count(PHYSFSX_readInt(LoadFile));
	PHYSFSX_fseek(LoadFile, 24, SEEK_CUR);

	trig_size = PHYSFSX_readInt(LoadFile);
	if (trig_size != sizeof(v1_control_center_triggers))
		throw std::runtime_error("wrong size for v1_control_center_triggers");
	PHYSFSX_fseek(LoadFile, 4, SEEK_CUR);

	const unsigned Num_robot_centers = PHYSFSX_readInt(LoadFile);
	LevelSharedRobotcenterState.Num_robot_centers = Num_robot_centers;
	PHYSFSX_fseek(LoadFile, 4, SEEK_CUR);

#if DXX_BUILD_DESCENT == 1
#elif DXX_BUILD_DESCENT == 2
	unsigned num_delta_lights;
	unsigned Num_static_lights;
	if (game_top_fileinfo_version >= 29) {
		PHYSFSX_fseek(LoadFile, 4, SEEK_CUR);
		Num_static_lights = PHYSFSX_readInt(LoadFile);
		PHYSFSX_fseek(LoadFile, 8, SEEK_CUR);
		num_delta_lights = PHYSFSX_readInt(LoadFile);
		PHYSFSX_fseek(LoadFile, 4, SEEK_CUR);
	} else {
		Num_static_lights = 0;
		num_delta_lights = 0;
	}
#endif

	if (game_top_fileinfo_version >= 14) //load mine filename
	{
		// read newline-terminated string, not sure what version this changed.
		if (!PHYSFSX_fgets(Current_level_name, LoadFile))
			Current_level_name.next()[0]=0;
	}
	else
		Current_level_name.next()[0]=0;

	savegame_pof_names_type Save_pof_name_storage;
	std::span<char[FILENAME_LEN]> Save_pof_names;
	if (game_top_fileinfo_version >= 19) {	//load pof names
		const unsigned N_save_pof_names = PHYSFSX_readSLE16(LoadFile);
		if (N_save_pof_names < MAX_POLYGON_MODELS)
		{
			Save_pof_names = std::span(static_cast<savegame_pof_names_type::base_type &>(Save_pof_name_storage)).first(N_save_pof_names);
			PHYSFSX_readBytes(LoadFile, Save_pof_name_storage, N_save_pof_names * FILENAME_LEN);
		}
		else
			LevelError("Level contains bogus N_save_pof_names %#x; ignoring", N_save_pof_names);
	}

	//===================== READ PLAYER INFO ==========================


	//===================== READ OBJECT INFO ==========================

	Gamesave_num_org_robots = 0;
	Gamesave_num_players = 0;

	if (object_offset > -1) {
		if (!PHYSFS_seek(LoadFile, object_offset))
			Error( "Error seeking to object_offset in gamesave.c" );

		range_for (auto &i, partial_range(Objects, gs_num_objects))
		{
			const auto &&o = vmobjptr(&i);
			read_object(o, LoadFile, game_top_fileinfo_version);
			verify_object(LevelSharedRobotInfoState, Vclip, o, Save_pof_names);
		}
	}

	//===================== READ WALL INFO ============================

	auto &vmwallptr = Walls.vmptr;
	for (auto &nw : vmwallptr)
	{
		if (game_top_fileinfo_version >= 20)
			wall_read(LoadFile, nw); // v20 walls and up.
		else if (game_top_fileinfo_version >= 17) {
			v19_wall w;
			v19_wall_read(LoadFile, w);
			nw.segnum	        = w.segnum;
			nw.sidenum	= build_sidenum_from_untrusted(w.sidenum).value();
			nw.linked_wall	= w.linked_wall;
			nw.type		= w.type;
			auto wf = static_cast<wall_flags>(w.flags);
			wf &= ~wall_flag::exploding;
			nw.flags = wf;
			nw.hps		= w.hps;
			nw.trigger	= static_cast<trgnum_t>(w.trigger);
#if DXX_BUILD_DESCENT == 1
			nw.clip_num	= convert_wclip(w.clip_num);
#elif DXX_BUILD_DESCENT == 2
			nw.clip_num	= w.clip_num;
#endif
			nw.keys		= static_cast<wall_key>(w.keys);
			nw.state		= wall_state::closed;
		} else {
			v16_wall w;
			v16_wall_read(LoadFile, w);
			nw.segnum = segment_none;
			nw.sidenum = {};
			nw.linked_wall = wall_none;
			nw.type		= w.type;
			auto wf = static_cast<wall_flags>(w.flags);
			wf &= ~wall_flag::exploding;
			nw.flags = wf;
			nw.hps		= w.hps;
			nw.trigger	= static_cast<trgnum_t>(w.trigger);
#if DXX_BUILD_DESCENT == 1
			nw.clip_num	= convert_wclip(w.clip_num);
#elif DXX_BUILD_DESCENT == 2
			nw.clip_num	= w.clip_num;
#endif
			nw.keys		= static_cast<wall_key>(w.keys);
		}
	}

	//==================== READ TRIGGER INFO ==========================

	auto &vmtrgptr = Triggers.vmptr;
	for (auto &i : vmtrgptr)
	{
#if DXX_BUILD_DESCENT == 1
		if (game_top_fileinfo_version <= 25)
			v25_trigger_read(LoadFile, &i);
		else {
			v26_trigger_read(LoadFile, i);
		}
#elif DXX_BUILD_DESCENT == 2
		if (game_top_fileinfo_version < 31)
		{
			if (game_top_fileinfo_version < 30) {
				v29_trigger_read_as_v31(LoadFile, i);
			}
			else
				v30_trigger_read_as_v31(LoadFile, i);
		}
		else
			trigger_read(LoadFile, i);
#endif
	}

	//================ READ CONTROL CENTER TRIGGER INFO ===============

	reconstruct_at(ControlCenterTriggers, control_center_triggers_read, LoadFile);

	//================ READ MATERIALOGRIFIZATIONATORS INFO ===============

	for (auto &&[i, r] : enumerate(partial_range(RobotCenters, Num_robot_centers)))
	{
#if DXX_BUILD_DESCENT == 1
		matcen_info_read(LoadFile, r, game_top_fileinfo_version);
#elif DXX_BUILD_DESCENT == 2
		if (game_top_fileinfo_version < 27) {
			d1_matcen_info_read(LoadFile, r);
		}
		else
			matcen_info_read(LoadFile, r);
#endif
			//	Set links in RobotCenters to Station array
		for (const shared_segment &seg : partial_const_range(Segments, Segments.get_count()))
			if (seg.special == segment_special::robotmaker)
				if (seg.matcen_num == i)
					r.fuelcen_num = seg.station_idx;
	}

#if DXX_BUILD_DESCENT == 2
	//================ READ DL_INDICES INFO ===============

	{
	auto &Dl_indices = LevelSharedDestructibleLightState.Dl_indices;
	Dl_indices.set_count(Num_static_lights);
	if (game_top_fileinfo_version < 29)
	{
		if (Num_static_lights)
			throw std::logic_error("Static lights in old file");
	}
	else
	{
		const auto &&lr = partial_range(Dl_indices, Num_static_lights);
		range_for (auto &i, lr)
			dl_index_read(&i, LoadFile);
		std::sort(lr.begin(), lr.end());
	}
	}

	//	Indicate that no light has been subtracted from any vertices.
	clear_light_subtracted();

	//================ READ DELTA LIGHT INFO ===============

		if (game_top_fileinfo_version < 29) {
			;
		} else
	{
		auto &Delta_lights = LevelSharedDestructibleLightState.Delta_lights;
		range_for (auto &i, partial_range(Delta_lights, num_delta_lights))
			delta_light_read(&i, LoadFile);
	}
#endif

	//========================= UPDATE VARIABLES ======================

	reset_objects(LevelUniqueObjectState, gs_num_objects);

	range_for (auto &i, Objects)
	{
		if (i.type != OBJ_NONE) {
			auto objsegnum = i.segnum;
			if (objsegnum > Highest_segment_index)		//bogus object
			{
				Warning("Object %p is in non-existent segment %hu, highest=%hu", &i, objsegnum, Highest_segment_index);
				i.type = OBJ_NONE;
			}
			else {
				obj_link_unchecked(Objects.vmptr, vmobjptridx(&i), vmsegptridx(objsegnum));
			}
		}
	}

	clear_transient_objects(1);		//1 means clear proximity bombs

	// Make sure non-transparent doors are set correctly.
	range_for (auto &&i, vmsegptridx)
		for (auto &&[side_idx, sside, uside] : enumerate(zip(i->shared_segment::sides, i->unique_segment::sides)))
		{
			if (sside.wall_num == wall_none)
				continue;
			auto &w = *vmwallptr(sside.wall_num);
			if (w.clip_num != -1)
			{
				auto &wa = WallAnims[w.clip_num];
				if (wa.flags & WCF_TMAP1)
				{
					uside.tmap_num = build_texture1_value(wa.frames[0]);
					uside.tmap_num2 = texture2_value();
				}
			}
			validate_segment_wall(i, sside, side_idx);
		}

	auto &ActiveDoors = LevelUniqueWallSubsystemState.ActiveDoors;
	ActiveDoors.set_count(0);

	//go through all walls, killing references to invalid triggers
	for (auto &w : vmwallptr)
	{
		if (underlying_value(w.trigger) >= Triggers.get_count())
		{
			w.trigger = trigger_none;	//kill trigger
		}
	}

#if DXX_USE_EDITOR
	//go through all triggers, killing unused ones
	{
		const std::ranges::subrange wr{vmwallptr};
		for (auto iter = Triggers.vmptridx.begin(); iter != Triggers.vmptridx.end();)
		{
			const auto i = (*iter).get_unchecked_index();
		//	Find which wall this trigger is connected to.
			const auto &&w{std::ranges::find(wr, i, &wall::trigger)};
		if (w == wr.end())
		{
				remove_trigger_num(Triggers, Walls.vmptr, i);
		}
		else
			++iter;
	}
	}
#endif

	//	MK, 10/17/95: Make walls point back at the triggers that control them.
	//	Go through all triggers, stuffing controlling_trigger field in Walls.
	{
#if DXX_BUILD_DESCENT == 2
		for (auto &w : vmwallptr)
			w.controlling_trigger = trigger_none;
#endif
		for (const auto &&t : Triggers.vcptridx)
		{
			auto &tr = *t;
			for (unsigned l = 0; l < tr.num_links; ++l)
			{
				//check to see that if a trigger requires a wall that it has one,
				//and if it requires a matcen that it has one
				const auto seg_num = tr.seg[l];
				if (trigger_is_matcen(tr))
				{
					if (Segments[seg_num].special != segment_special::robotmaker)
						con_printf(CON_URGENT, "matcen %u triggers non-matcen segment %hu", underlying_value(t.get_unchecked_index()), seg_num);
				}
#if DXX_BUILD_DESCENT == 2
				else if (tr.type != trigger_action::light_off && tr.type != trigger_action::light_on)
				{	//light triggers don't require walls
					const auto side_num = tr.side[l];
					auto wall_num = vmsegptr(seg_num)->shared_segment::sides[side_num].wall_num;
					if (const auto &&uwall = vmwallptr.check_untrusted(wall_num))
						(*uwall)->controlling_trigger = t;
					else
					{
						LevelError("trigger %u link %u type %u references segment %hu, side %u which is an invalid wall; ignoring.", underlying_value(t.get_unchecked_index()), l, static_cast<unsigned>(tr.type), seg_num, underlying_value(side_num));
					}
				}
#endif
			}
		}
	}

	//fix old wall structs
	if (game_top_fileinfo_version < 17) {
		range_for (const auto &&segp, vcsegptridx)
		{
			for (const auto &&[sidenum, side] : enumerate(segp->shared_segment::sides))
			{
				const auto wallnum = side.wall_num;
				if (wallnum != wall_none)
				{
					auto &w = *vmwallptr(wallnum);
					w.segnum = segp;
					w.sidenum = static_cast<sidenum_t>(sidenum);
				}
			}
		}
	}
	fix_object_segs();

	if (game_top_fileinfo_version < GAME_VERSION
#if DXX_BUILD_DESCENT == 2
	    && !(game_top_fileinfo_version == 25 && GAME_VERSION == 26)
#endif
		)
		return 1;		//means old version
	else
		return 0;
}
}
}

// ----------------------------------------------------------------------------

//1 -> 2  add palette name
//2 -> 3  add control center explosion time
//3 -> 4  add reactor strength
//4 -> 5  killed hostage text stuff
//5 -> 6  added Secret_return_segment and Secret_return_orient
//6 -> 7  added flickering lights
//7 -> 8  made version 8 to be not compatible with D2 1.0 & 1.1

#ifndef RELEASE
const char *Level_being_loaded=NULL;
#endif

#if DXX_BUILD_DESCENT == 2
int no_old_level_file_error{0};
#endif

//loads a level (.LVL) file from disk
//returns 0 if success, else error code
namespace dsx {
int load_level(
#if DXX_BUILD_DESCENT == 2
	d_level_shared_destructible_light_state &LevelSharedDestructibleLightState,
#endif
	const char * filename_passed)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vmobjptridx = Objects.vmptridx;
	int sig, minedata_offset, gamedata_offset;
	int mine_err, game_err;

	#ifndef RELEASE
	Level_being_loaded = filename_passed;
	#endif

	std::array<char, PATH_MAX> filename_storage;
	auto filename = filename_passed;
	auto LoadFile = PHYSFSX_openReadBuffered(filename).first;
	if (!LoadFile)
	{
		filename = filename_storage.data(); 
		snprintf(filename_storage.data(), filename_storage.size(), "%.*s%s", DXX_ptrdiff_cast_int(std::distance(Current_mission->path.cbegin(), Current_mission->filename)), Current_mission->path.c_str(), filename_passed);
		auto &&[fp, physfserr] = PHYSFSX_openReadBuffered_updateCase(filename_storage.data());
		if (!fp)
		{
#if DXX_USE_EDITOR
			con_printf(CON_VERBOSE, "Failed to open file <%s>: %s", filename, PHYSFS_getErrorByCode(physfserr));
			return 1;
		#else
			Error("Failed to open file <%s>: %s", filename, PHYSFS_getErrorByCode(physfserr));
		#endif
		}
		LoadFile = std::move(fp);
	}

	sig                      = PHYSFSX_readInt(LoadFile);
	Gamesave_current_version = PHYSFSX_readInt(LoadFile);
	minedata_offset          = PHYSFSX_readInt(LoadFile);
	gamedata_offset          = PHYSFSX_readInt(LoadFile);

	Assert(sig == MAKE_SIG('P','L','V','L'));
	(void)sig;

	if (Gamesave_current_version < 5)
		PHYSFSX_skipBytes<4>(LoadFile);		//was hostagetext_offset
	init_exploding_walls();
#if DXX_BUILD_DESCENT == 2
	if (Gamesave_current_version >= 8) {    //read dummy data
		PHYSFSX_skipBytes<4 + 2 + 1>(LoadFile);
	}

	if (Gamesave_current_version > 1)
	{
		if (!PHYSFSX_fgets(Current_level_palette, LoadFile))
			Current_level_palette[0u] = 0;
	}
	if (Gamesave_current_version <= 1 || Current_level_palette[0u]==0) // descent 1 level
		strcpy(Current_level_palette.next().data(), DEFAULT_LEVEL_PALETTE);

	if (Gamesave_current_version >= 3)
		LevelSharedControlCenterState.Base_control_center_explosion_time = PHYSFSX_readInt(LoadFile);
	else
		LevelSharedControlCenterState.Base_control_center_explosion_time = DEFAULT_CONTROL_CENTER_EXPLOSION_TIME;

	if (Gamesave_current_version >= 4)
		LevelSharedControlCenterState.Reactor_strength = PHYSFSX_readInt(LoadFile);
	else
		LevelSharedControlCenterState.Reactor_strength = -1;  //use old defaults

	if (Gamesave_current_version >= 7) {
		Flickering_light_state.Num_flickering_lights = PHYSFSX_readInt(LoadFile);
		range_for (auto &i, partial_range(Flickering_light_state.Flickering_lights, Flickering_light_state.Num_flickering_lights))
			flickering_light_read(i, LoadFile);
	}
	else
		Flickering_light_state.Num_flickering_lights = 0;

	{
		auto &Secret_return_orient = LevelSharedSegmentState.Secret_return_orient;
	if (Gamesave_current_version < 6) {
		LevelSharedSegmentState.Secret_return_segment = segment_first;
		Secret_return_orient.rvec.x = F1_0;
		Secret_return_orient.rvec.y = 0;
		Secret_return_orient.rvec.z = 0;
		Secret_return_orient.fvec.x = 0;
		Secret_return_orient.fvec.y = F1_0;
		Secret_return_orient.fvec.z = 0;
		Secret_return_orient.uvec.x = 0;
		Secret_return_orient.uvec.y = 0;
		Secret_return_orient.uvec.z = F1_0;
	} else {
		LevelSharedSegmentState.Secret_return_segment = read_untrusted_segnum_le32(LoadFile);
		Secret_return_orient.rvec.x = PHYSFSX_readInt(LoadFile);
		Secret_return_orient.rvec.y = PHYSFSX_readInt(LoadFile);
		Secret_return_orient.rvec.z = PHYSFSX_readInt(LoadFile);
		Secret_return_orient.fvec.x = PHYSFSX_readInt(LoadFile);
		Secret_return_orient.fvec.y = PHYSFSX_readInt(LoadFile);
		Secret_return_orient.fvec.z = PHYSFSX_readInt(LoadFile);
		Secret_return_orient.uvec.x = PHYSFSX_readInt(LoadFile);
		Secret_return_orient.uvec.y = PHYSFSX_readInt(LoadFile);
		Secret_return_orient.uvec.z = PHYSFSX_readInt(LoadFile);
	}
	}
#endif

	PHYSFS_seek(LoadFile, minedata_offset);
		//NOTE LINK TO ABOVE!!
		mine_err = load_mine_data_compiled(LoadFile, filename);

	/* !!!HACK!!!
	 * Descent 1 - Level 19: OBERON MINE has some ugly overlapping rooms (segment 484).
	 * HACK to make this issue less visible by moving one vertex a little.
	 */
	auto &vmvertptr = Vertices.vmptr;
	if (Current_mission && !d_stricmp("Descent: First Strike", Current_mission->mission_name) && !d_stricmp("level19.rdl", filename) && PHYSFS_fileLength(LoadFile) == 136706)
		vmvertptr(vertnum_t{1905u})->z = -385 * F1_0;
#if DXX_BUILD_DESCENT == 2
	/* !!!HACK!!!
	 * Descent 2 - Level 12: MAGNACORE STATION has a segment (104) with illegal dimensions.
	 * HACK to fix this by moving the Vertex and fixing the associated Normals.
	 * NOTE: This only fixes the normals of segment 104, not the other ones connected to this Vertex but this is unsignificant.
	 */
	if (Current_mission && !d_stricmp("Descent 2: Counterstrike!", Current_mission->mission_name) && !d_stricmp("d2levc-4.rl2", filename))
	{
		shared_segment &s104 = *vmsegptr(segnum_t{104});
		auto &s104v0 = *vmvertptr(s104.verts[segment_relative_vertnum::_0]);
		auto &s104s1 = s104.sides[sidenum_t::WTOP];
		auto &s104s1n0 = s104s1.normals[0];
		auto &s104s1n1 = s104s1.normals[1];
		auto &s104s2 = s104.sides[sidenum_t::WRIGHT];
		auto &s104s2n0 = s104s2.normals[0];
		auto &s104s2n1 = s104s2.normals[1];
		if (
			(s104v0.x == -53990800 && s104v0.y == -59927741 && s104v0.z == 23034584) &&
			(s104s1n0.x == 56775 && s104s1n0.y == -27796 && s104s1n0.z == -17288 && s104s1n1.x == 50157 && s104s1n1.y == -34561 && s104s1n1.z == -24180) &&
			(s104s2n0.x == 60867 && s104s2n0.y == -19485 && s104s2n0.z == -14507 && s104s2n1.x == 55485 && s104s2n1.y == -29668 && s104s2n1.z == -18332)
		)
		{
			s104v0.x = -53859726;
			s104v0.y = -59927743;
			s104v0.z = 23034586;
			s104s1n0.x = 56123;
			s104s1n0.y = -27725;
			s104s1n0.z = -19401;
			s104s1n1.x = 49910;
			s104s1n1.y = -33946;
			s104s1n1.z = -25525;
			s104s2n0.x = 60903;
			s104s2n0.y = -18371;
			s104s2n0.z = -15753;
			s104s2n1.x = 57004;
			s104s2n1.y = -26385;
			s104s2n1.z = -18688;
			// I feel so dirty now ...
		}
	}
#endif

	if (mine_err == -1) {   //error!!
		return 2;
	}

	PHYSFS_seek(LoadFile, gamedata_offset);
	game_err = load_game_data(
#if DXX_BUILD_DESCENT == 2
		LevelSharedDestructibleLightState,
#endif
		vmobjptridx, vmsegptridx, LoadFile);

	if (game_err == -1) {   //error!!
		return 3;
	}

	//======================== CLOSE FILE =============================
	LoadFile.reset();
#if DXX_BUILD_DESCENT == 2
	set_ambient_sound_flags();
#endif

#if DXX_USE_EDITOR
#if DXX_BUILD_DESCENT == 1
	//If an old version, ask the use if he wants to save as new version
	if (((LEVEL_FILE_VERSION>1) && Gamesave_current_version < LEVEL_FILE_VERSION) || mine_err==1 || game_err==1) {
		gr_palette_load(gr_palette);
		if (nm_messagebox_str(menu_title{nullptr}, nm_messagebox_tie("Don't Save", "Save"), menu_subtitle{"You just loaded a old version level.  Would\n"
						"you like to save it as a current version level?"}) == 1)
			save_level(filename);
	}
#elif DXX_BUILD_DESCENT == 2
	//If a Descent 1 level and the Descent 1 pig isn't present, pretend it's a Descent 2 level.
	if (EditorWindow && (Gamesave_current_version <= 3) && !d1_pig_present)
	{
		if (!no_old_level_file_error)
			Warning_puts("A Descent 1 level was loaded,\n"
					"and there is no Descent 1 texture\n"
					"set available. Saving it will\n"
					"convert it to a Descent 2 level.");

		Gamesave_current_version = LEVEL_FILE_VERSION;
	}
#endif
	#endif

#if DXX_USE_EDITOR
	if (EditorWindow)
		editor_status_fmt("Loaded NEW mine %s, \"%s\"", filename, static_cast<const char *>(Current_level_name));
	#endif

#ifdef NDEBUG
	if (!PLAYING_BUILTIN_MISSION)
#endif
	if (check_segment_connections())
	{
#ifndef NDEBUG
		nm_messagebox_str(menu_title{TXT_ERROR}, nm_messagebox_tie(TXT_OK),
				menu_subtitle{"Connectivity errors detected in\n"
				"mine.  See monochrome screen for\n"
				"details, and contact Matt or Mike."});
	#endif
	}


#if DXX_BUILD_DESCENT == 2
	compute_slide_segs();
#endif
	return 0;
}
}

#if DXX_USE_EDITOR
int get_level_name()
{
	using items_type = std::array<newmenu_item, 2>;
	struct request_menu : items_type, passive_newmenu
	{
		request_menu(grs_canvas &canvas) :
			items_type{{
				newmenu_item::nm_item_text{"Please enter a name for this mine:"},
				newmenu_item::nm_item_input(Current_level_name.next()),
			}},
			passive_newmenu(menu_title{nullptr}, menu_subtitle{"Enter mine name"}, menu_filename{nullptr}, tiny_mode_flag::normal, tab_processing_flag::ignore, adjusted_citem::create(*static_cast<items_type *>(this), 1), canvas, draw_box_flag::none)
		{
		}
	};
	return run_blocking_newmenu<request_menu>(*grd_curcanv);
}

// --------------------------------------------------------------------------------------
//	Create a new mine, set global variables.
namespace dsx {
int create_new_mine(void)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	vms_matrix	m1 = IDENTITY_MATRIX;
	
	// initialize_mine_arrays();
	
	//	gamestate_not_restored = 1;
	
	// Clear refueling center code
	fuelcen_reset();
	init_all_vertices();
	
	Current_level_num = 1;		// make level 1 (for now)
	Current_level_name.next()[0] = 0;
#if DXX_BUILD_DESCENT == 1
	Gamesave_current_version = LEVEL_FILE_VERSION;
#elif DXX_BUILD_DESCENT == 2
	Gamesave_current_version = GAME_VERSION;
	
	strcpy(Current_level_palette.next().data(), DEFAULT_LEVEL_PALETTE);
#endif
	
	Cur_object_index = -1;
	reset_objects(LevelUniqueObjectState, 1);		//just one object, the player
	
	num_groups = 0;
	current_group = -1;
	
	
	LevelSharedVertexState.Num_vertices = 0;		// Number of vertices in global array.
	Vertices.set_count(1);
	LevelSharedSegmentState.Num_segments = 0;		// Number of segments in global array, will get increased in med_create_segment
	Segments.set_count(1);
	Cursegp = imsegptridx(segment_first);	// Say current segment is the only segment.
	Curside = sidenum_t::WBACK;		// The active side is the back side
	Markedsegp = segment_none;		// Say there is no marked segment.
	Markedside = sidenum_t::WBACK;	//	Shouldn't matter since Markedsegp == 0, but just in case...
	for (int s=0;s<MAX_GROUPS+1;s++) {
		GroupList[s].clear();
		Groupsegp[s] = NULL;
	}
	Groupside = {};
	
	LevelSharedRobotcenterState.Num_robot_centers = 0;
	wall_init(LevelUniqueWallSubsystemState);
	auto &Triggers = LevelUniqueWallSubsystemState.Triggers;
	Triggers.set_count(0);
	
	// Create New_segment, which is the segment we will be adding at each instance.
	med_create_new_segment({DEFAULT_X_SIZE, DEFAULT_Y_SIZE, DEFAULT_Z_SIZE});		// New_segment = Segments[0];
	//	med_create_segment(Segments,0,0,0,DEFAULT_X_SIZE,DEFAULT_Y_SIZE,DEFAULT_Z_SIZE,vm_mat_make(&m1,F1_0,0,0,0,F1_0,0,0,0,F1_0));
	med_create_segment(vmsegptridx(segment_first), 0, 0, 0, DEFAULT_X_SIZE, DEFAULT_Y_SIZE, DEFAULT_Z_SIZE, m1);
	
	Found_segs.clear();
	Selected_segs.clear();
	Warning_segs.clear();
	
	//--repair-- create_local_segment_data();
	
	ControlCenterTriggers.num_links = 0;
	
	create_new_mission();
	
    //editor_status("New mine created.");
	return	0;			// say no error
}
}

int	Errors_in_mine;

namespace dsx {
namespace {
// -----------------------------------------------------------------------------
#if DXX_BUILD_DESCENT == 2
static unsigned compute_num_delta_light_records(fvcdlindexptr &vcdlindexptr)
{
	unsigned total{0};
	for (auto &i : vcdlindexptr)
		total += i.count;
	return total;

}
#endif

// -----------------------------------------------------------------------------
// Save game
static int save_game_data(
#if DXX_BUILD_DESCENT == 2
	const d_level_shared_destructible_light_state &LevelSharedDestructibleLightState,
#endif
	PHYSFS_File *SaveFile)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	auto &RobotCenters = LevelSharedRobotcenterState.RobotCenters;
#if DXX_BUILD_DESCENT == 1
	short game_top_fileinfo_version = Gamesave_current_version >= 5 ? 31 : GAME_VERSION;
#elif DXX_BUILD_DESCENT == 2
	short game_top_fileinfo_version = Gamesave_current_version >= 5 ? 31 : 25;
	int dl_indices_offset{0}, delta_light_offset=0;
#endif
	int player_offset{0}, object_offset=0, walls_offset=0, doors_offset=0, triggers_offset=0, control_offset=0, matcen_offset=0; //, links_offset;
	int offset_offset{0}, end_offset=0;
	//===================== SAVE FILE INFO ========================

	PHYSFS_writeSLE16(SaveFile, 0x6705);	// signature
	PHYSFS_writeSLE16(SaveFile, game_top_fileinfo_version);
	PHYSFS_writeSLE32(SaveFile, 0);
	PHYSFSX_writeBytes(SaveFile, Current_level_name.line(), 15);
	PHYSFS_writeSLE32(SaveFile, Current_level_num);
	offset_offset = PHYSFS_tell(SaveFile);	// write the offsets later
	PHYSFS_writeSLE32(SaveFile, -1);
	PHYSFS_writeSLE32(SaveFile, 0);

#define WRITE_HEADER_ENTRY(t, n) do { PHYSFS_writeSLE32(SaveFile, -1); PHYSFS_writeSLE32(SaveFile, n); PHYSFS_writeSLE32(SaveFile, sizeof(t)); } while(0)

	WRITE_HEADER_ENTRY(object, Objects.get_count());
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	WRITE_HEADER_ENTRY(wall, Walls.get_count());
	auto &ActiveDoors = LevelUniqueWallSubsystemState.ActiveDoors;
	WRITE_HEADER_ENTRY(active_door, ActiveDoors.get_count());
	auto &Triggers = LevelUniqueWallSubsystemState.Triggers;
	WRITE_HEADER_ENTRY(trigger, Triggers.get_count());
	WRITE_HEADER_ENTRY(0, 0);		// links (removed by Parallax)
	WRITE_HEADER_ENTRY(control_center_triggers, 1);
	const auto Num_robot_centers = LevelSharedRobotcenterState.Num_robot_centers;
	WRITE_HEADER_ENTRY(matcen_info, Num_robot_centers);

#if DXX_BUILD_DESCENT == 2
	unsigned num_delta_lights{0};
	if (game_top_fileinfo_version >= 29)
	{
		auto &Dl_indices = LevelSharedDestructibleLightState.Dl_indices;
		const unsigned Num_static_lights = Dl_indices.get_count();
		WRITE_HEADER_ENTRY(dl_index, Num_static_lights);
		WRITE_HEADER_ENTRY(delta_light, num_delta_lights = compute_num_delta_light_records(Dl_indices.vcptr));
	}

	// Write the mine name
	if (game_top_fileinfo_version >= 31)
#endif
		PHYSFSX_printf(SaveFile, "%s\n", static_cast<const char *>(Current_level_name));
#if DXX_BUILD_DESCENT == 2
	else if (game_top_fileinfo_version >= 14)
		PHYSFSX_writeString(SaveFile, Current_level_name);

	if (game_top_fileinfo_version >= 19)
#endif
	{
		const auto N_polygon_models = LevelSharedPolygonModelState.N_polygon_models;
		PHYSFS_writeSLE16(SaveFile, N_polygon_models);
		range_for (auto &i, partial_const_range(LevelSharedPolygonModelState.Pof_names, N_polygon_models))
			PHYSFSX_writeBytes(SaveFile, &i, sizeof(i));
	}

	//==================== SAVE PLAYER INFO ===========================

	player_offset = PHYSFS_tell(SaveFile);

	//==================== SAVE OBJECT INFO ===========================

	object_offset = PHYSFS_tell(SaveFile);
	for (auto &objp : vcobjptr)
	{
		write_object(objp, game_top_fileinfo_version, SaveFile);
	}

	//==================== SAVE WALL INFO =============================

	walls_offset = PHYSFS_tell(SaveFile);
	auto &vcwallptr = Walls.vcptr;
	for (auto &w : vcwallptr)
		wall_write(SaveFile, w, game_top_fileinfo_version);

	//==================== SAVE TRIGGER INFO =============================

	triggers_offset = PHYSFS_tell(SaveFile);
	auto &vctrgptr = Triggers.vcptr;
	for (auto &t : vctrgptr)
	{
		if (game_top_fileinfo_version <= 29)
			v29_trigger_write(SaveFile, t);
		else if (game_top_fileinfo_version <= 30)
			v30_trigger_write(SaveFile, t);
		else if (game_top_fileinfo_version >= 31)
			v31_trigger_write(SaveFile, t);
	}

	//================ SAVE CONTROL CENTER TRIGGER INFO ===============

	control_offset = PHYSFS_tell(SaveFile);
	control_center_triggers_write(ControlCenterTriggers, SaveFile);


	//================ SAVE MATERIALIZATION CENTER TRIGGER INFO ===============

	matcen_offset = PHYSFS_tell(SaveFile);
	range_for (auto &r, partial_const_range(RobotCenters, Num_robot_centers))
		matcen_info_write(SaveFile, r, game_top_fileinfo_version);

	//================ SAVE DELTA LIGHT INFO ===============
#if DXX_BUILD_DESCENT == 2
	if (game_top_fileinfo_version >= 29)
	{
		dl_indices_offset = PHYSFS_tell(SaveFile);
		auto &Dl_indices = LevelSharedDestructibleLightState.Dl_indices;
		for (const auto &i : Dl_indices.vcptr)
			dl_index_write(&i, SaveFile);

		delta_light_offset = PHYSFS_tell(SaveFile);
		auto &Delta_lights = LevelSharedDestructibleLightState.Delta_lights;
		range_for (auto &i, partial_const_range(Delta_lights, num_delta_lights))
			delta_light_write(&i, SaveFile);
	}
#endif

	//============= SAVE OFFSETS ===============

	end_offset = PHYSFS_tell(SaveFile);

	// Update the offset fields

#define WRITE_OFFSET(o, n) do { PHYSFS_seek(SaveFile, offset_offset); PHYSFS_writeSLE32(SaveFile, o ## _offset); offset_offset += sizeof(int)*n; } while (0)

	WRITE_OFFSET(player, 2);
	WRITE_OFFSET(object, 3);
	WRITE_OFFSET(walls, 3);
	WRITE_OFFSET(doors, 3);
	WRITE_OFFSET(triggers, 6);
	WRITE_OFFSET(control, 3);
	WRITE_OFFSET(matcen, 3);
#if DXX_BUILD_DESCENT == 2
	if (game_top_fileinfo_version >= 29)
	{
		WRITE_OFFSET(dl_indices, 3);
		WRITE_OFFSET(delta_light, 0);
	}
#endif

	// Go back to end of data
	PHYSFS_seek(SaveFile, end_offset);

	return 0;
}

// -----------------------------------------------------------------------------
// Save game
static int save_level_sub(
#if DXX_BUILD_DESCENT == 2
	const d_level_shared_destructible_light_state &LevelSharedDestructibleLightState,
#endif
	fvmobjptridx &vmobjptridx, const char *const filename)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vmobjptr = Objects.vmptr;
	int minedata_offset{0},gamedata_offset=0;
	std::array<char, PATH_MAX> temp_filename;

//	if ( !compiled_version )
	{
		write_game_text_file(filename);

		if (Errors_in_mine) {
			if (is_real_level(filename)) {
				gr_palette_load(gr_palette);
	 
				if (nm_messagebox(menu_title{nullptr}, {"Cancel Save", "Save"}, "Warning: %i errors in this mine!\n", Errors_in_mine )!=1)	{
					return 1;
				}
			}
		}
	}
//	else
	{
		auto &level_file_extension =
#if DXX_BUILD_DESCENT == 2
			(Gamesave_current_version > 3)
			? D2X_LEVEL_FILE_EXTENSION
			:
#endif
			D1X_LEVEL_FILE_EXTENSION;
		if (!change_filename_extension(temp_filename, filename, level_file_extension))
		{
			con_printf(CON_URGENT, "Failed to generate filename for level data from \"%s\"", filename);
			return 1;
		}
	}

	auto &&[SaveFile, physfserr] = PHYSFSX_openWriteBuffered(temp_filename.data());
	if (!SaveFile)
	{
		gr_palette_load(gr_palette);
		nm_messagebox(menu_title{nullptr}, {TXT_OK}, "ERROR: Cannot write to '%s'.\n%s", temp_filename.data(), PHYSFS_getErrorByCode(physfserr));
		return 1;
	}

	if (Current_level_name[0] == 0)
		strcpy(Current_level_name.next().data(),"Untitled");

	clear_transient_objects(1);		//1 means clear proximity bombs

	compress_objects();		//after this, Highest_object_index == num objects

	//make sure player is in a segment
	{
		const auto &&plr = vmobjptridx(vcplayerptr(0u)->objnum);
		if (update_object_seg(vmobjptr, LevelSharedSegmentState, LevelUniqueSegmentState, plr) == 0)
		{
			if (plr->segnum > Highest_segment_index)
				plr->segnum = segment_first;
			auto &vcvertptr = Vertices.vcptr;
			plr->pos = compute_segment_center(vcvertptr, vcsegptr(plr->segnum));
		}
	}
 
	fix_object_segs();

	//Write the header

	PHYSFS_writeSLE32(SaveFile, MAKE_SIG('P','L','V','L'));
	PHYSFS_writeSLE32(SaveFile, Gamesave_current_version);

	//save placeholders
	PHYSFS_writeSLE32(SaveFile, minedata_offset);
	PHYSFS_writeSLE32(SaveFile, gamedata_offset);
#if DXX_BUILD_DESCENT == 1
	int hostagetext_offset{0};
	PHYSFS_writeSLE32(SaveFile, hostagetext_offset);
#endif

	//Now write the damn data

#if DXX_BUILD_DESCENT == 2
	if (Gamesave_current_version >= 8)
	{
		//write the version 8 data (to make file unreadable by 1.0 & 1.1)
		PHYSFS_writeSLE32(SaveFile, GameTime64);
		PHYSFS_writeSLE16(SaveFile, d_tick_count);
		PHYSFSX_writeU8(SaveFile, FrameTime);
	}

	if (Gamesave_current_version < 5)
		PHYSFS_writeSLE32(SaveFile, -1);       //was hostagetext_offset

	// Write the palette file name
	if (Gamesave_current_version > 1)
		PHYSFSX_printf(SaveFile, "%s\n", static_cast<const char *>(Current_level_palette));

	if (Gamesave_current_version >= 3)
		PHYSFS_writeSLE32(SaveFile, LevelSharedControlCenterState.Base_control_center_explosion_time);
	if (Gamesave_current_version >= 4)
		PHYSFS_writeSLE32(SaveFile, LevelSharedControlCenterState.Reactor_strength);

	if (Gamesave_current_version >= 7)
	{
		const auto Num_flickering_lights = Flickering_light_state.Num_flickering_lights;
		PHYSFS_writeSLE32(SaveFile, Num_flickering_lights);
		range_for (auto &i, partial_const_range(Flickering_light_state.Flickering_lights, Num_flickering_lights))
			flickering_light_write(i, SaveFile);
	}

	if (Gamesave_current_version >= 6)
	{
		PHYSFS_writeSLE32(SaveFile, LevelSharedSegmentState.Secret_return_segment);
		auto &Secret_return_orient = LevelSharedSegmentState.Secret_return_orient;
		PHYSFSX_writeVector(SaveFile, Secret_return_orient.rvec);
		PHYSFSX_writeVector(SaveFile, Secret_return_orient.fvec);
		PHYSFSX_writeVector(SaveFile, Secret_return_orient.uvec);
	}
#endif

	minedata_offset = PHYSFS_tell(SaveFile);
		save_mine_data_compiled(SaveFile);
	gamedata_offset = PHYSFS_tell(SaveFile);
	save_game_data(
#if DXX_BUILD_DESCENT == 2
		LevelSharedDestructibleLightState,
#endif
		SaveFile);
#if DXX_BUILD_DESCENT == 1
	hostagetext_offset = PHYSFS_tell(SaveFile);
#endif

	PHYSFS_seek(SaveFile, sizeof(int) + sizeof(Gamesave_current_version));
	PHYSFS_writeSLE32(SaveFile, minedata_offset);
	PHYSFS_writeSLE32(SaveFile, gamedata_offset);
#if DXX_BUILD_DESCENT == 1
	PHYSFS_writeSLE32(SaveFile, hostagetext_offset);
#elif DXX_BUILD_DESCENT == 2
	if (Gamesave_current_version < 5)
		PHYSFS_writeSLE32(SaveFile, PHYSFS_fileLength(SaveFile));
#endif

	//==================== CLOSE THE FILE =============================

//	if ( !compiled_version )
	{
		if (EditorWindow)
			editor_status_fmt("Saved mine %s, \"%s\"", filename, static_cast<const char *>(Current_level_name));
	}

	return 0;

}
}

int save_level(
#if DXX_BUILD_DESCENT == 2
	const d_level_shared_destructible_light_state &LevelSharedDestructibleLightState,
#endif
	const char * filename)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	int r1;

	// Save compiled version...
	r1 = save_level_sub(
#if DXX_BUILD_DESCENT == 2
		LevelSharedDestructibleLightState,
#endif
		vmobjptridx, filename);

	return r1;
}
}

#endif	//EDITOR
