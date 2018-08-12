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
#include "pstypes.h"
#include "strutil.h"
#include "console.h"
#include "key.h"
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
#include "screens.h"
#include "wall.h"
#include "gamemine.h"
#include "robot.h"
#include "bm.h"
#include "menu.h"
#include "fireball.h"
#include "switch.h"
#include "fuelcen.h"
#include "cntrlcen.h"
#include "powerup.h"
#include "hostage.h"
#include "weapon.h"
#include "player.h"
#include "newdemo.h"
#include "gameseq.h"
#include "automap.h"
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

#include "dxxsconf.h"
#include "compiler-range_for.h"
#include "partial_range.h"

#if defined(DXX_BUILD_DESCENT_I)
#if DXX_USE_EDITOR
const char Shareware_level_names[NUM_SHAREWARE_LEVELS][12] = {
	"level01.rdl",
	"level02.rdl",
	"level03.rdl",
	"level04.rdl",
	"level05.rdl",
	"level06.rdl",
	"level07.rdl"
};

const char Registered_level_names[NUM_REGISTERED_LEVELS][12] = {
	"level08.rdl",
	"level09.rdl",
	"level10.rdl",
	"level11.rdl",
	"level12.rdl",
	"level13.rdl",
	"level14.rdl",
	"level15.rdl",
	"level16.rdl",
	"level17.rdl",
	"level18.rdl",
	"level19.rdl",
	"level20.rdl",
	"level21.rdl",
	"level22.rdl",
	"level23.rdl",
	"level24.rdl",
	"level25.rdl",
	"level26.rdl",
	"level27.rdl",
	"levels1.rdl",
	"levels2.rdl",
	"levels3.rdl"
};
#endif
#endif

char Gamesave_current_filename[PATH_MAX];

int Gamesave_current_version;

#if defined(DXX_BUILD_DESCENT_I)
#define GAME_VERSION					25
#elif defined(DXX_BUILD_DESCENT_II)
#define GAME_VERSION            32
#endif
#define GAME_COMPATIBLE_VERSION 22

//version 28->29  add delta light support
//version 27->28  controlcen id now is reactor number, not model number
//version 28->29  ??
//version 29->30  changed trigger structure
//version 30->31  changed trigger structure some more
//version 31->32  change segment structure, make it 512 bytes w/o editor, add Segment2s array.

#define MENU_CURSOR_X_MIN       MENU_X
#define MENU_CURSOR_X_MAX       MENU_X+6

int Gamesave_num_org_robots = 0;
//--unused-- grs_bitmap * Gamesave_saved_bitmap = NULL;

#if DXX_USE_EDITOR
// Return true if this level has a name of the form "level??"
// Note that a pathspec can appear at the beginning of the filename.
static int is_real_level(const char *filename)
{
	int len = strlen(filename);

	if (len < 6)
		return 0;

	return !d_strnicmp(&filename[len-11], "level");

}
#endif

//--unused-- vms_angvec zero_angles={0,0,0};

int Gamesave_num_players=0;

#if defined(DXX_BUILD_DESCENT_I)
#define MAX_POLYGON_MODELS_NEW 167
static array<char[FILENAME_LEN], MAX_POLYGON_MODELS_NEW> Save_pof_names;

static int convert_vclip(int vc) {
	if (vc < 0)
		return vc;
	if ((vc < VCLIP_MAXNUM) && (Vclip[vc].num_frames != ~0u))
		return vc;
	return 0;
}
static int convert_wclip(int wc) {
	return (wc < Num_wall_anims) ? wc : wc % Num_wall_anims;
}
int convert_tmap(int tmap)
{
	if (tmap == -1)
		return tmap;
    return (tmap >= NumTextures) ? tmap % NumTextures : tmap;
}
static int convert_polymod(int polymod) {
    return (polymod >= N_polygon_models) ? polymod % N_polygon_models : polymod;
}
#elif defined(DXX_BUILD_DESCENT_II)
static array<char[FILENAME_LEN], MAX_POLYGON_MODELS> Save_pof_names;
#endif

namespace dsx {

static void verify_object(object &obj)
{
	obj.lifeleft = IMMORTAL_TIME;		//all loaded object are immortal, for now

	if (obj.type == OBJ_ROBOT)
	{
		Gamesave_num_org_robots++;

		// Make sure valid id...
		if (get_robot_id(obj) >= N_robot_types )
			set_robot_id(obj, get_robot_id(obj) % N_robot_types);

		// Make sure model number & size are correct...
		if (obj.render_type == RT_POLYOBJ)
		{
			auto &ri = Robot_info[get_robot_id(obj)];
#if defined(DXX_BUILD_DESCENT_II)
			assert(ri.model_num != -1);
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

			//@@Took out this ugly hack 1/12/96, because Mike has added code
			//@@that should fix it in a better way.
			//@@//this is a super-ugly hack.  Since the baby stripe robots have
			//@@//their firing point on their bounding sphere, the firing points
			//@@//can poke through a wall if the robots are very close to it. So
			//@@//we make their radii bigger so the guns can't get too close to 
			//@@//the walls
			//@@if (Robot_info[obj.id].flags & RIF_BIG_RADIUS)
			//@@	obj.size = (obj.size*3)/2;

			//@@if (obj.control_type==CT_AI && Robot_info[obj.id].attack_type)
			//@@	obj.size = obj.size*3/4;
		}

#if defined(DXX_BUILD_DESCENT_II)
		if (get_robot_id(obj) == 65)						//special "reactor" robots
			obj.movement_type = MT_NONE;
#endif

		if (obj.movement_type == MT_PHYSICS)
		{
			auto &ri = Robot_info[get_robot_id(obj)];
			obj.mtype.phys_info.mass = ri.mass;
			obj.mtype.phys_info.drag = ri.drag;
		}
	}
	else {		//Robots taken care of above
		if (obj.render_type == RT_POLYOBJ)
		{
			char *name = Save_pof_names[obj.rtype.pobj_info.model_num];
			for (uint_fast32_t i = 0;i < N_polygon_models;i++)
				if (!d_stricmp(Pof_names[i],name)) {		//found it!	
					obj.rtype.pobj_info.model_num = i;
					break;
				}
		}
	}

	if (obj.type == OBJ_POWERUP)
	{
		if ( get_powerup_id(obj) >= N_powerup_types )	{
			set_powerup_id(obj, POW_SHIELD_BOOST);
			Assert( obj.render_type != RT_POLYOBJ );
		}
		obj.control_type = CT_POWERUP;
		obj.size = Powerup_info[get_powerup_id(obj)].size;
		obj.ctype.powerup_info.creation_time = 0;
	}

	if (obj.type == OBJ_WEAPON)
	{
		if ( get_weapon_id(obj) >= N_weapon_types )	{
			set_weapon_id(obj, weapon_id_type::LASER_ID_L1);
			Assert( obj.render_type != RT_POLYOBJ );
		}

#if defined(DXX_BUILD_DESCENT_II)
		const auto weapon_id = get_weapon_id(obj);
		if (weapon_id == weapon_id_type::PMINE_ID)
		{		//make sure pmines have correct values
			obj.mtype.phys_info.mass = Weapon_info[weapon_id].mass;
			obj.mtype.phys_info.drag = Weapon_info[weapon_id].drag;
			obj.mtype.phys_info.flags |= PF_FREE_SPINNING;

			// Make sure model number & size are correct...		
			Assert( obj.render_type == RT_POLYOBJ );

			obj.rtype.pobj_info.model_num = Weapon_info[weapon_id].model_num;
			obj.size = Polygon_models[obj.rtype.pobj_info.model_num].rad;
		}
#endif
	}

	if (obj.type == OBJ_CNTRLCEN)
	{
		obj.render_type = RT_POLYOBJ;
		obj.control_type = CT_CNTRLCEN;

#if defined(DXX_BUILD_DESCENT_I)
		// Make model number is correct...	
		for (int i=0; i<Num_total_object_types; i++ )	
			if ( ObjType[i] == OL_CONTROL_CENTER )		{
				obj.rtype.pobj_info.model_num = ObjId[i];
				obj.shields = ObjStrength[i];
				break;		
			}
#elif defined(DXX_BUILD_DESCENT_II)
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
			init_player_object();
		else
			if (obj.render_type == RT_POLYOBJ)	//recover from Matt's pof file matchup bug
				obj.rtype.pobj_info.model_num = Player_ship->model_num;

		//Make sure orient matrix is orthogonal
		check_and_fix_matrix(obj.orient);

		set_player_id(obj, Gamesave_num_players++);
	}

	if (obj.type == OBJ_HOSTAGE)
	{
		obj.render_type = RT_HOSTAGE;
		obj.control_type = CT_POWERUP;
	}
}

}

//static gs_skip(int len,PHYSFS_File *file)
//{
//
//	PHYSFSX_fseek(file,len,SEEK_CUR);
//}

//reads one object of the given version from the given file
namespace dsx {
static void read_object(const vmobjptr_t obj,PHYSFS_File *f,int version)
{
	const auto poison_obj = reinterpret_cast<uint8_t *>(&*obj);
	const auto signature = obj_get_signature();
	DXX_POISON_MEMORY(poison_obj, sizeof(*obj), 0xfd);
	obj->signature = signature;
	set_object_type(*obj, PHYSFSX_readByte(f));
	obj->id             = PHYSFSX_readByte(f);

#if defined(DXX_BUILD_DESCENT_I)
	if (obj->type == OBJ_ROBOT && get_robot_id(obj) > 23) {
		set_robot_id(obj, get_robot_id(obj) % 24);
	}
#endif
	obj->control_type   = PHYSFSX_readByte(f);
	set_object_movement_type(*obj, PHYSFSX_readByte(f));
	obj->render_type    = PHYSFSX_readByte(f);
	obj->flags          = PHYSFSX_readByte(f);

	obj->segnum         = PHYSFSX_readShort(f);
	obj->attached_obj   = object_none;

	PHYSFSX_readVector(f, obj->pos);
	PHYSFSX_readMatrix(&obj->orient,f);

	obj->size           = PHYSFSX_readFix(f);
	obj->shields        = PHYSFSX_readFix(f);

	PHYSFSX_readVector(f, obj->last_pos);

	obj->contains_type  = PHYSFSX_readByte(f);
	obj->contains_id    = PHYSFSX_readByte(f);
	obj->contains_count = PHYSFSX_readByte(f);

	switch (obj->movement_type) {

		case MT_PHYSICS:

			PHYSFSX_readVector(f, obj->mtype.phys_info.velocity);
			PHYSFSX_readVector(f, obj->mtype.phys_info.thrust);

			obj->mtype.phys_info.mass		= PHYSFSX_readFix(f);
			obj->mtype.phys_info.drag		= PHYSFSX_readFix(f);
			PHYSFSX_readFix(f);	/* brakes */

			PHYSFSX_readVector(f, obj->mtype.phys_info.rotvel);
			PHYSFSX_readVector(f, obj->mtype.phys_info.rotthrust);

			obj->mtype.phys_info.turnroll	= PHYSFSX_readFixAng(f);
			obj->mtype.phys_info.flags		= PHYSFSX_readShort(f);

			break;

		case MT_SPINNING:

			PHYSFSX_readVector(f, obj->mtype.spin_rate);
			break;

		case MT_NONE:
			break;

		default:
			Int3();
	}

	switch (obj->control_type) {

		case CT_AI: {
			obj->ctype.ai_info.behavior				= static_cast<ai_behavior>(PHYSFSX_readByte(f));

			range_for (auto &i, obj->ctype.ai_info.flags)
				i = PHYSFSX_readByte(f);

			obj->ctype.ai_info.hide_segment			= PHYSFSX_readShort(f);
			obj->ctype.ai_info.hide_index			= PHYSFSX_readShort(f);
			obj->ctype.ai_info.path_length			= PHYSFSX_readShort(f);
			obj->ctype.ai_info.cur_path_index		= PHYSFSX_readShort(f);

			if (version <= 25) {
				PHYSFSX_readShort(f);	//				obj->ctype.ai_info.follow_path_start_seg	= 
				PHYSFSX_readShort(f);	//				obj->ctype.ai_info.follow_path_end_seg		= 
			}

			break;
		}

		case CT_EXPLOSION:

			obj->ctype.expl_info.spawn_time		= PHYSFSX_readFix(f);
			obj->ctype.expl_info.delete_time		= PHYSFSX_readFix(f);
			obj->ctype.expl_info.delete_objnum	= PHYSFSX_readShort(f);
			obj->ctype.expl_info.next_attach = obj->ctype.expl_info.prev_attach = obj->ctype.expl_info.attach_parent = object_none;

			break;

		case CT_WEAPON:

			//do I really need to read these?  Are they even saved to disk?

			obj->ctype.laser_info.parent_type		= PHYSFSX_readShort(f);
			obj->ctype.laser_info.parent_num		= PHYSFSX_readShort(f);
			obj->ctype.laser_info.parent_signature	= object_signature_t{static_cast<uint16_t>(PHYSFSX_readInt(f))};
#if defined(DXX_BUILD_DESCENT_II)
			obj->ctype.laser_info.last_afterburner_time = 0;
#endif
			obj->ctype.laser_info.clear_hitobj();

			break;

		case CT_LIGHT:

			obj->ctype.light_info.intensity = PHYSFSX_readFix(f);
			break;

		case CT_POWERUP:

			if (version >= 25)
				obj->ctype.powerup_info.count = PHYSFSX_readInt(f);
			else
				obj->ctype.powerup_info.count = 1;

			if (obj->type == OBJ_POWERUP)
			{
				/* Hostages have control type CT_POWERUP, but object
				 * type OBJ_HOSTAGE.  Hostages are never weapons, so
				 * prevent checking their IDs.
				 */
			if (get_powerup_id(obj) == POW_VULCAN_WEAPON)
					obj->ctype.powerup_info.count = VULCAN_WEAPON_AMMO_AMOUNT;

#if defined(DXX_BUILD_DESCENT_II)
			else if (get_powerup_id(obj) == POW_GAUSS_WEAPON)
					obj->ctype.powerup_info.count = VULCAN_WEAPON_AMMO_AMOUNT;

			else if (get_powerup_id(obj) == POW_OMEGA_WEAPON)
					obj->ctype.powerup_info.count = MAX_OMEGA_CHARGE;
#endif
			}

			break;


		case CT_NONE:
		case CT_FLYING:
		case CT_DEBRIS:
			break;

		case CT_SLEW:		//the player is generally saved as slew
			break;

		case CT_CNTRLCEN:
			break;

		case CT_MORPH:
		case CT_FLYTHROUGH:
		case CT_REPAIRCEN:
		default:
			Int3();
	
	}

	switch (obj->render_type) {

		case RT_NONE:
			break;

		case RT_MORPH:
		case RT_POLYOBJ: {
			int tmo;

#if defined(DXX_BUILD_DESCENT_I)
			obj->rtype.pobj_info.model_num		= convert_polymod(PHYSFSX_readInt(f));
#elif defined(DXX_BUILD_DESCENT_II)
			obj->rtype.pobj_info.model_num		= PHYSFSX_readInt(f);
#endif

			range_for (auto &i, obj->rtype.pobj_info.anim_angles)
				PHYSFSX_readAngleVec(&i, f);

			obj->rtype.pobj_info.subobj_flags	= PHYSFSX_readInt(f);

			tmo = PHYSFSX_readInt(f);

#if !DXX_USE_EDITOR
#if defined(DXX_BUILD_DESCENT_I)
			obj->rtype.pobj_info.tmap_override	= convert_tmap(tmo);
#elif defined(DXX_BUILD_DESCENT_II)
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

		case RT_WEAPON_VCLIP:
		case RT_HOSTAGE:
		case RT_POWERUP:
		case RT_FIREBALL:

#if defined(DXX_BUILD_DESCENT_I)
			obj->rtype.vclip_info.vclip_num	= convert_vclip(PHYSFSX_readInt(f));
#elif defined(DXX_BUILD_DESCENT_II)
			obj->rtype.vclip_info.vclip_num	= PHYSFSX_readInt(f);
#endif
			obj->rtype.vclip_info.frametime	= PHYSFSX_readFix(f);
			obj->rtype.vclip_info.framenum	= PHYSFSX_readByte(f);

			break;

		case RT_LASER:
			break;

		default:
			Int3();

	}

}
}

#if DXX_USE_EDITOR
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

//writes one object to the given file
namespace dsx {
static void write_object(const object &obj, short version, PHYSFS_File *f)
{
#if defined(DXX_BUILD_DESCENT_I)
	(void)version;
#endif
	PHYSFSX_writeU8(f, obj.type);
	PHYSFSX_writeU8(f, obj.id);

	PHYSFSX_writeU8(f, obj.control_type);
	PHYSFSX_writeU8(f, obj.movement_type);
	PHYSFSX_writeU8(f, obj.render_type);
	PHYSFSX_writeU8(f, obj.flags);

	PHYSFS_writeSLE16(f, obj.segnum);

	PHYSFSX_writeVector(f, obj.pos);
	PHYSFSX_writeMatrix(f, obj.orient);

	PHYSFSX_writeFix(f, obj.size);
	PHYSFSX_writeFix(f, obj.shields);

	PHYSFSX_writeVector(f, obj.last_pos);

	PHYSFSX_writeU8(f, obj.contains_type);
	PHYSFSX_writeU8(f, obj.contains_id);
	PHYSFSX_writeU8(f, obj.contains_count);

	switch (obj.movement_type) {

		case MT_PHYSICS:

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

		case MT_SPINNING:

			PHYSFSX_writeVector(f, obj.mtype.spin_rate);
			break;

		case MT_NONE:
			break;

		default:
			Int3();
	}

	switch (obj.control_type) {

		case CT_AI: {
			PHYSFSX_writeU8(f, static_cast<uint8_t>(obj.ctype.ai_info.behavior));

			range_for (auto &i, obj.ctype.ai_info.flags)
				PHYSFSX_writeU8(f, i);

			PHYSFS_writeSLE16(f, obj.ctype.ai_info.hide_segment);
			PHYSFS_writeSLE16(f, obj.ctype.ai_info.hide_index);
			PHYSFS_writeSLE16(f, obj.ctype.ai_info.path_length);
			PHYSFS_writeSLE16(f, obj.ctype.ai_info.cur_path_index);

#if defined(DXX_BUILD_DESCENT_I)
			PHYSFS_writeSLE16(f, segment_none);
			PHYSFS_writeSLE16(f, segment_none);
#elif defined(DXX_BUILD_DESCENT_II)
			if (version <= 25)
			{
				PHYSFS_writeSLE16(f, -1);	//obj.ctype.ai_info.follow_path_start_seg
				PHYSFS_writeSLE16(f, -1);	//obj.ctype.ai_info.follow_path_end_seg
			}
#endif

			break;
		}

		case CT_EXPLOSION:

			PHYSFSX_writeFix(f, obj.ctype.expl_info.spawn_time);
			PHYSFSX_writeFix(f, obj.ctype.expl_info.delete_time);
			PHYSFS_writeSLE16(f, obj.ctype.expl_info.delete_objnum);

			break;

		case CT_WEAPON:

			//do I really need to write these objects?

			PHYSFS_writeSLE16(f, obj.ctype.laser_info.parent_type);
			PHYSFS_writeSLE16(f, obj.ctype.laser_info.parent_num);
			PHYSFS_writeSLE32(f, obj.ctype.laser_info.parent_signature.get());

			break;

		case CT_LIGHT:

			PHYSFSX_writeFix(f, obj.ctype.light_info.intensity);
			break;

		case CT_POWERUP:

#if defined(DXX_BUILD_DESCENT_I)
			PHYSFS_writeSLE32(f, obj.ctype.powerup_info.count);
#elif defined(DXX_BUILD_DESCENT_II)
			if (version >= 25)
				PHYSFS_writeSLE32(f, obj.ctype.powerup_info.count);
#endif
			break;

		case CT_NONE:
		case CT_FLYING:
		case CT_DEBRIS:
			break;

		case CT_SLEW:		//the player is generally saved as slew
			break;

		case CT_CNTRLCEN:
			break;			//control center object.

		case CT_MORPH:
		case CT_REPAIRCEN:
		case CT_FLYTHROUGH:
		default:
			Int3();
	
	}

	switch (obj.render_type) {

		case RT_NONE:
			break;

		case RT_MORPH:
		case RT_POLYOBJ: {
			PHYSFS_writeSLE32(f, obj.rtype.pobj_info.model_num);

			range_for (auto &i, obj.rtype.pobj_info.anim_angles)
				PHYSFSX_writeAngleVec(f, i);

			PHYSFS_writeSLE32(f, obj.rtype.pobj_info.subobj_flags);

			PHYSFS_writeSLE32(f, obj.rtype.pobj_info.tmap_override);

			break;
		}

		case RT_WEAPON_VCLIP:
		case RT_HOSTAGE:
		case RT_POWERUP:
		case RT_FIREBALL:

			PHYSFS_writeSLE32(f, obj.rtype.vclip_info.vclip_num);
			PHYSFSX_writeFix(f, obj.rtype.vclip_info.frametime);
			PHYSFSX_writeU8(f, obj.rtype.vclip_info.framenum);

			break;

		case RT_LASER:
			break;

		default:
			Int3();

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

static void validate_segment_wall(const vcsegptridx_t seg, shared_side &side, const unsigned sidenum)
{
	auto &rwn0 = side.wall_num;
	const auto wn0 = rwn0;
	auto &w0 = *vcwallptr(wn0);
	switch (w0.type)
	{
		case WALL_DOOR:
			{
				const auto connected_seg = seg->children[sidenum];
				if (connected_seg == segment_none)
				{
					rwn0 = wall_none;
					LevelError("segment %u side %u wall %u has no child segment; removing orphan wall.", seg.get_unchecked_index(), sidenum, wn0);
					return;
				}
				auto &vcseg = *vcsegptr(connected_seg);
				const unsigned connected_side = find_connect_side(seg, vcseg);
				const auto wn1 = vcseg.sides[connected_side].wall_num;
				if (wn1 == wall_none)
				{
					rwn0 = wall_none;
					LevelError("segment %u side %u wall %u has child segment %u side %u, but no wall; removing orphan wall.", seg.get_unchecked_index(), sidenum, wn0, connected_seg, connected_side);
					return;
				}
			}
			break;
		default:
			break;
	}
}

static int load_game_data(fvmobjptridx &vmobjptridx, fvmsegptridx &vmsegptridx, PHYSFS_File *LoadFile)
{
	const auto &vcsegptridx = vmsegptridx;
	short game_top_fileinfo_version;
	int object_offset;
	unsigned gs_num_objects;
	int trig_size;

	//===================== READ FILE INFO ========================

	// Check signature
	if (PHYSFSX_readShort(LoadFile) != 0x6705)
		return -1;

	// Read and check version number
	game_top_fileinfo_version = PHYSFSX_readShort(LoadFile);
	if (game_top_fileinfo_version < GAME_COMPATIBLE_VERSION )
		return -1;

	// We skip some parts of the former game_top_fileinfo
	PHYSFSX_fseek(LoadFile, 31, SEEK_CUR);

	object_offset = PHYSFSX_readInt(LoadFile);
	gs_num_objects = PHYSFSX_readInt(LoadFile);
	PHYSFSX_fseek(LoadFile, 8, SEEK_CUR);

	init_exploding_walls();
	Walls.set_count(PHYSFSX_readInt(LoadFile));
	PHYSFSX_fseek(LoadFile, 20, SEEK_CUR);

	Triggers.set_count(PHYSFSX_readInt(LoadFile));
	PHYSFSX_fseek(LoadFile, 24, SEEK_CUR);

	trig_size = PHYSFSX_readInt(LoadFile);
	Assert(trig_size == sizeof(ControlCenterTriggers));
	(void)trig_size;
	PHYSFSX_fseek(LoadFile, 4, SEEK_CUR);

	Num_robot_centers = PHYSFSX_readInt(LoadFile);
	PHYSFSX_fseek(LoadFile, 4, SEEK_CUR);

#if defined(DXX_BUILD_DESCENT_I)
#elif defined(DXX_BUILD_DESCENT_II)
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

	if (game_top_fileinfo_version >= 31) //load mine filename
		// read newline-terminated string, not sure what version this changed.
		PHYSFSX_fgets(Current_level_name,LoadFile);
	else if (game_top_fileinfo_version >= 14) { //load mine filename
		// read null-terminated string
		//must do read one char at a time, since no PHYSFSX_fgets()
		for (auto p = Current_level_name.next().begin(); (*p = PHYSFSX_fgetc(LoadFile));)
		{
			if (++p == Current_level_name.line().end())
			{
				p[-1] = 0;
				while (PHYSFSX_fgetc(LoadFile))
					;
				break;
			}
		}
	}
	else
		Current_level_name.next()[0]=0;

	if (game_top_fileinfo_version >= 19) {	//load pof names
		const unsigned N_save_pof_names = PHYSFSX_readShort(LoadFile);
		if (N_save_pof_names < MAX_POLYGON_MODELS)
			PHYSFS_read(LoadFile,Save_pof_names,N_save_pof_names,FILENAME_LEN);
		else
			LevelError("Level contains bogus N_save_pof_names %#x; ignoring", N_save_pof_names);
	}

	//===================== READ PLAYER INFO ==========================


	//===================== READ OBJECT INFO ==========================

	Gamesave_num_org_robots = 0;
	Gamesave_num_players = 0;

	if (object_offset > -1) {
		if (PHYSFSX_fseek( LoadFile, object_offset, SEEK_SET ))
			Error( "Error seeking to object_offset in gamesave.c" );

		range_for (auto &i, partial_range(Objects, gs_num_objects))
		{
			const auto &&o = vmobjptr(&i);
			read_object(o, LoadFile, game_top_fileinfo_version);
			verify_object(o);
		}
	}

	//===================== READ WALL INFO ============================

	range_for (const auto &&vw, vmwallptr)
	{
		auto &nw = *vw;
		if (game_top_fileinfo_version >= 20)
			wall_read(LoadFile, nw); // v20 walls and up.
		else if (game_top_fileinfo_version >= 17) {
			v19_wall w;
			v19_wall_read(LoadFile, w);
			nw.segnum	        = w.segnum;
			nw.sidenum	= w.sidenum;
			nw.linked_wall	= w.linked_wall;
			nw.type		= w.type;
			nw.flags		= w.flags & ~WALL_EXPLODING;
			nw.hps		= w.hps;
			nw.trigger	= w.trigger;
#if defined(DXX_BUILD_DESCENT_I)
			nw.clip_num	= convert_wclip(w.clip_num);
#elif defined(DXX_BUILD_DESCENT_II)
			nw.clip_num	= w.clip_num;
#endif
			nw.keys		= w.keys;
			nw.state		= WALL_DOOR_CLOSED;
		} else {
			v16_wall w;
			v16_wall_read(LoadFile, w);
			nw.segnum = segment_none;
			nw.sidenum = nw.linked_wall = -1;
			nw.type		= w.type;
			nw.flags		= w.flags & ~WALL_EXPLODING;
			nw.hps		= w.hps;
			nw.trigger	= w.trigger;
#if defined(DXX_BUILD_DESCENT_I)
			nw.clip_num	= convert_wclip(w.clip_num);
#elif defined(DXX_BUILD_DESCENT_II)
			nw.clip_num	= w.clip_num;
#endif
			nw.keys		= w.keys;
		}
	}

	//==================== READ TRIGGER INFO ==========================

	range_for (const auto vt, vmtrgptr)
	{
		auto &i = *vt;
#if defined(DXX_BUILD_DESCENT_I)
		if (game_top_fileinfo_version <= 25)
			v25_trigger_read(LoadFile, &i);
		else {
			v26_trigger_read(LoadFile, i);
		}
#elif defined(DXX_BUILD_DESCENT_II)
		if (game_top_fileinfo_version < 31)
		{
			if (game_top_fileinfo_version < 30) {
				v29_trigger_read_as_v31(LoadFile, i);
			}
			else
				v30_trigger_read_as_v31(LoadFile, i);
		}
		else
			trigger_read(&i, LoadFile);
#endif
	}

	//================ READ CONTROL CENTER TRIGGER INFO ===============

	control_center_triggers_read(&ControlCenterTriggers, LoadFile);

	//================ READ MATERIALOGRIFIZATIONATORS INFO ===============

	for (uint_fast32_t i = 0; i < Num_robot_centers; i++) {
#if defined(DXX_BUILD_DESCENT_I)
		matcen_info_read(LoadFile, RobotCenters[i], game_top_fileinfo_version);
#elif defined(DXX_BUILD_DESCENT_II)
		if (game_top_fileinfo_version < 27) {
			d1_matcen_info_read(LoadFile, RobotCenters[i]);
		}
		else
			matcen_info_read(LoadFile, RobotCenters[i]);
#endif
			//	Set links in RobotCenters to Station array
		range_for (auto &seg, partial_const_range(Segments, Highest_segment_index + 1))
			if (seg.special == SEGMENT_IS_ROBOTMAKER)
				if (seg.matcen_num == i)
					RobotCenters[i].fuelcen_num = seg.station_idx;
	}

#if defined(DXX_BUILD_DESCENT_II)
	//================ READ DL_INDICES INFO ===============

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

	//	Indicate that no light has been subtracted from any vertices.
	clear_light_subtracted();

	//================ READ DELTA LIGHT INFO ===============

		if (game_top_fileinfo_version < 29) {
			;
		} else
	{
		range_for (auto &i, partial_range(Delta_lights, num_delta_lights))
			delta_light_read(&i, LoadFile);
	}
#endif

	//========================= UPDATE VARIABLES ======================

	reset_objects(ObjectState, gs_num_objects);

	range_for (auto &i, Objects)
	{
		if (i.type != OBJ_NONE) {
			auto objsegnum = i.segnum;
			if (objsegnum > Highest_segment_index)		//bogus object
			{
				Warning("Object %p is in non-existent segment %i, highest=%i", &i, objsegnum, Highest_segment_index);
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
		range_for (const auto eside, enumerate(i->sides))
		{
			auto &side = eside.value;
			if (side.wall_num == wall_none)
				continue;
			const auto sidep = &side;
			auto &w = *vmwallptr(side.wall_num);
			if (w.clip_num != -1)
			{
				if (WallAnims[w.clip_num].flags & WCF_TMAP1)
				{
					sidep->tmap_num = WallAnims[w.clip_num].frames[0];
					sidep->tmap_num2 = 0;
				}
			}
			validate_segment_wall(i, side, eside.idx);
		}

	ActiveDoors.set_count(0);

	//go through all walls, killing references to invalid triggers
	range_for (const auto &&p, vmwallptr)
	{
		auto &w = *p;
		if (w.trigger >= Num_triggers) {
			w.trigger = trigger_none;	//kill trigger
		}
	}

#if DXX_USE_EDITOR
	//go through all triggers, killing unused ones
	{
		const auto &&wr = make_range(vmwallptr);
	for (uint_fast32_t i = 0;i < Num_triggers;) {
		auto a = [i](const wall &w) { return w.trigger == i; };
		//	Find which wall this trigger is connected to.
		auto w = std::find_if(wr.begin(), wr.end(), a);
		if (w == wr.end())
		{
			remove_trigger_num(i);
		}
		else
			i++;
	}
	}
#endif

	//	MK, 10/17/95: Make walls point back at the triggers that control them.
	//	Go through all triggers, stuffing controlling_trigger field in Walls.
	{
#if defined(DXX_BUILD_DESCENT_II)
		range_for (const auto &&w, vmwallptr)
			w->controlling_trigger = -1;
#endif

		range_for (const auto &&t, vctrgptridx)
		{
			auto &tr = *t;
			for (unsigned l = 0; l < tr.num_links; ++l)
			{
				//check to see that if a trigger requires a wall that it has one,
				//and if it requires a matcen that it has one
				const auto seg_num = tr.seg[l];
				if (trigger_is_matcen(tr))
				{
					if (Segments[seg_num].special != SEGMENT_IS_ROBOTMAKER)
						con_printf(CON_URGENT, "matcen %u triggers non-matcen segment %hu", t.get_unchecked_index(), seg_num);
				}
#if defined(DXX_BUILD_DESCENT_II)
				else if (tr.type != TT_LIGHT_OFF && tr.type != TT_LIGHT_ON)
				{	//light triggers don't require walls
					const auto side_num = tr.side[l];
					auto wall_num = vmsegptr(seg_num)->sides[side_num].wall_num;
					if (const auto &&uwall = vmwallptr.check_untrusted(wall_num))
						(*uwall)->controlling_trigger = t;
					else
					{
						LevelError("trigger %u link %u type %u references segment %hu, side %u which is an invalid wall; ignoring.", static_cast<trgnum_t>(t), l, tr.type, seg_num, side_num);
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
			for (int sidenum=0;sidenum<6;sidenum++)
			{
				const auto wallnum = segp->sides[sidenum].wall_num;
				if (wallnum != wall_none)
				{
					auto &w = *vmwallptr(wallnum);
					w.segnum = segp;
					w.sidenum = sidenum;
				}
			}
		}
	}
	fix_object_segs();

	if (game_top_fileinfo_version < GAME_VERSION
#if defined(DXX_BUILD_DESCENT_II)
	    && !(game_top_fileinfo_version == 25 && GAME_VERSION == 26)
#endif
		)
		return 1;		//means old version
	else
		return 0;
}
}

// ----------------------------------------------------------------------------

#if defined(DXX_BUILD_DESCENT_I)
#define LEVEL_FILE_VERSION		1
#elif defined(DXX_BUILD_DESCENT_II)
#define LEVEL_FILE_VERSION      8
#endif
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

#if defined(DXX_BUILD_DESCENT_II)
int no_old_level_file_error=0;
#endif

//loads a level (.LVL) file from disk
//returns 0 if success, else error code
namespace dsx {
int load_level(const char * filename_passed)
{
#if DXX_USE_EDITOR
	int use_compiled_level=1;
#endif
	char filename[PATH_MAX];
	int sig, minedata_offset, gamedata_offset;
	int mine_err, game_err;

	#ifndef RELEASE
	Level_being_loaded = filename_passed;
	#endif

	strcpy(filename,filename_passed);

#if DXX_USE_EDITOR
	//if we have the editor, try the LVL first, no matter what was passed.
	//if we don't have an LVL, try what was passed or RL2  
	//if we don't have the editor, we just use what was passed

	change_filename_extension(filename,filename_passed,".lvl");
	use_compiled_level = 0;

	if (!PHYSFSX_exists(filename,1))
	{
		const char *p = strrchr(filename_passed, '.');

		if (d_stricmp(p, ".lvl"))
			strcpy(filename, filename_passed);	// set to what was passed
		else
			change_filename_extension(filename, filename, "." DXX_LEVEL_FILE_EXTENSION);
		use_compiled_level = 1;
	}		
#endif

	auto LoadFile = PHYSFSX_openReadBuffered(filename);
	if (!LoadFile)
	{
		snprintf(filename, sizeof(filename), "%.*s%s", DXX_ptrdiff_cast_int(std::distance(Current_mission->path.cbegin(), Current_mission->filename)), Current_mission->path.c_str(), filename_passed);
		LoadFile = PHYSFSX_openReadBuffered(filename);
	}

	if (!LoadFile)	{
#if DXX_USE_EDITOR
			return 1;
		#else
			Error("Can't open file <%s>\n",filename);
		#endif
	}

	strcpy( Gamesave_current_filename, filename );

	sig                      = PHYSFSX_readInt(LoadFile);
	Gamesave_current_version = PHYSFSX_readInt(LoadFile);
	minedata_offset          = PHYSFSX_readInt(LoadFile);
	gamedata_offset          = PHYSFSX_readInt(LoadFile);

	Assert(sig == MAKE_SIG('P','L','V','L'));
	(void)sig;

	if (Gamesave_current_version < 5)
		PHYSFSX_readInt(LoadFile);       //was hostagetext_offset
	init_exploding_walls();
#if defined(DXX_BUILD_DESCENT_II)
	if (Gamesave_current_version >= 8) {    //read dummy data
		PHYSFSX_readInt(LoadFile);
		PHYSFSX_readShort(LoadFile);
		PHYSFSX_readByte(LoadFile);
	}

	if (Gamesave_current_version > 1)
		PHYSFSX_fgets(Current_level_palette,LoadFile);
	if (Gamesave_current_version <= 1 || Current_level_palette[0]==0) // descent 1 level
		strcpy(Current_level_palette.next().data(), DEFAULT_LEVEL_PALETTE);

	if (Gamesave_current_version >= 3)
		Base_control_center_explosion_time = PHYSFSX_readInt(LoadFile);
	else
		Base_control_center_explosion_time = DEFAULT_CONTROL_CENTER_EXPLOSION_TIME;

	if (Gamesave_current_version >= 4)
		Reactor_strength = PHYSFSX_readInt(LoadFile);
	else
		Reactor_strength = -1;  //use old defaults

	if (Gamesave_current_version >= 7) {
		Flickering_light_state.Num_flickering_lights = PHYSFSX_readInt(LoadFile);
		range_for (auto &i, partial_range(Flickering_light_state.Flickering_lights, Flickering_light_state.Num_flickering_lights))
			flickering_light_read(i, LoadFile);
	}
	else
		Flickering_light_state.Num_flickering_lights = 0;

	if (Gamesave_current_version < 6) {
		Secret_return_segment = 0;
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
		Secret_return_segment = PHYSFSX_readInt(LoadFile);
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
#endif

	PHYSFSX_fseek(LoadFile,minedata_offset,SEEK_SET);
#if DXX_USE_EDITOR
	if (!use_compiled_level) {
		mine_err = load_mine_data(LoadFile);
#if 0 // get from d1src if needed
		// Compress all uv coordinates in mine, improves texmap precision. --MK, 02/19/96
		compress_uv_coordinates_all();
#endif
	} else
	#endif
		//NOTE LINK TO ABOVE!!
		mine_err = load_mine_data_compiled(LoadFile);

	/* !!!HACK!!!
	 * Descent 1 - Level 19: OBERON MINE has some ugly overlapping rooms (segment 484).
	 * HACK to make this issue less visible by moving one vertex a little.
	 */
	if (Current_mission && !d_stricmp("Descent: First Strike",Current_mission_longname) && !d_stricmp("level19.rdl",filename) && PHYSFS_fileLength(LoadFile) == 136706)
		vmvertptr(1905u)->z = -385 * F1_0;
#if defined(DXX_BUILD_DESCENT_II)
	/* !!!HACK!!!
	 * Descent 2 - Level 12: MAGNACORE STATION has a segment (104) with illegal dimensions.
	 * HACK to fix this by moving the Vertex and fixing the associated Normals.
	 * NOTE: This only fixes the normals of segment 104, not the other ones connected to this Vertex but this is unsignificant.
	 */
	if (Current_mission && !d_stricmp("Descent 2: Counterstrike!",Current_mission_longname) && !d_stricmp("d2levc-4.rl2",filename))
	{
		auto &s104 = *vmsegptr(vmsegidx_t(104));
		auto &s104v0 = *vmvertptr(s104.verts[0]);
		auto &s104s1 = s104.sides[1];
		auto &s104s1n0 = s104s1.normals[0];
		auto &s104s1n1 = s104s1.normals[1];
		auto &s104s2 = s104.sides[2];
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

	PHYSFSX_fseek(LoadFile,gamedata_offset,SEEK_SET);
	game_err = load_game_data(vmobjptridx, vmsegptridx, LoadFile);

	if (game_err == -1) {   //error!!
		return 3;
	}

	//======================== CLOSE FILE =============================
	LoadFile.reset();
#if defined(DXX_BUILD_DESCENT_II)
	set_ambient_sound_flags();
#endif

#if DXX_USE_EDITOR
#if defined(DXX_BUILD_DESCENT_I)
	//If an old version, ask the use if he wants to save as new version
	if (((LEVEL_FILE_VERSION>1) && Gamesave_current_version < LEVEL_FILE_VERSION) || mine_err==1 || game_err==1) {
		gr_palette_load(gr_palette);
		if (nm_messagebox( NULL, 2, "Don't Save", "Save", "You just loaded a old version level.  Would\n"
						"you like to save it as a current version level?")==1)
			save_level(filename);
	}
#elif defined(DXX_BUILD_DESCENT_II)
	//If a Descent 1 level and the Descent 1 pig isn't present, pretend it's a Descent 2 level.
	if (EditorWindow && (Gamesave_current_version <= 3) && !d1_pig_present)
	{
		if (!no_old_level_file_error)
			Warning("A Descent 1 level was loaded,\n"
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
		nm_messagebox( "ERROR", 1, "Ok", 
				"Connectivity errors detected in\n"
				"mine.  See monochrome screen for\n"
				"details, and contact Matt or Mike." );
	#endif
	}


#if defined(DXX_BUILD_DESCENT_II)
	compute_slide_segs();
#endif
	return 0;
}
}

#if DXX_USE_EDITOR
int get_level_name()
{
	array<newmenu_item, 2> m{{
		nm_item_text("Please enter a name for this mine:"),
		nm_item_input(Current_level_name.next()),
	}};
	return newmenu_do( NULL, "Enter mine name", m, unused_newmenu_subfunction, unused_newmenu_userdata ) >= 0;
}
#endif


#if DXX_USE_EDITOR

// --------------------------------------------------------------------------------------
//	Create a new mine, set global variables.
namespace dsx {
int create_new_mine(void)
{
	vms_matrix	m1 = IDENTITY_MATRIX;
	
	// initialize_mine_arrays();
	
	//	gamestate_not_restored = 1;
	
	// Clear refueling center code
	fuelcen_reset();
	init_all_vertices();
	
	Current_level_num = 1;		// make level 1 (for now)
	Current_level_name.next()[0] = 0;
#if defined(DXX_BUILD_DESCENT_I)
	Gamesave_current_version = LEVEL_FILE_VERSION;
#elif defined(DXX_BUILD_DESCENT_II)
	Gamesave_current_version = GAME_VERSION;
	
	strcpy(Current_level_palette.next().data(), DEFAULT_LEVEL_PALETTE);
#endif
	
	Cur_object_index = -1;
	reset_objects(ObjectState, 1);		//just one object, the player
	
	num_groups = 0;
	current_group = -1;
	
	
	Num_vertices = 0;		// Number of vertices in global array.
	Vertices.set_count(1);
	Num_segments = 0;		// Number of segments in global array, will get increased in med_create_segment
	Segments.set_count(1);
	Cursegp = imsegptridx(segment_first);	// Say current segment is the only segment.
	Curside = WBACK;		// The active side is the back side
	Markedsegp = segment_none;		// Say there is no marked segment.
	Markedside = WBACK;	//	Shouldn't matter since Markedsegp == 0, but just in case...
	for (int s=0;s<MAX_GROUPS+1;s++) {
		GroupList[s].clear();
		Groupsegp[s] = NULL;
		Groupside[s] = 0;
	}
	
	Num_robot_centers = 0;
	ActiveDoors.set_count(0);
	wall_init();
	trigger_init();
	
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

// -----------------------------------------------------------------------------
#if defined(DXX_BUILD_DESCENT_II)
static int compute_num_delta_light_records(void)
{
	int	total = 0;
	range_for (const auto &&i, Dl_indices.vcptr)
		total += i->count;
	return total;

}
#endif

// -----------------------------------------------------------------------------
// Save game
namespace dsx {
static int save_game_data(PHYSFS_File *SaveFile)
{
#if defined(DXX_BUILD_DESCENT_I)
	short game_top_fileinfo_version = Gamesave_current_version >= 5 ? 31 : GAME_VERSION;
#elif defined(DXX_BUILD_DESCENT_II)
	short game_top_fileinfo_version = Gamesave_current_version >= 5 ? 31 : 25;
	int	dl_indices_offset=0, delta_light_offset=0;
#endif
	int  player_offset=0, object_offset=0, walls_offset=0, doors_offset=0, triggers_offset=0, control_offset=0, matcen_offset=0; //, links_offset;
	int offset_offset=0, end_offset=0;
	//===================== SAVE FILE INFO ========================

	PHYSFS_writeSLE16(SaveFile, 0x6705);	// signature
	PHYSFS_writeSLE16(SaveFile, game_top_fileinfo_version);
	PHYSFS_writeSLE32(SaveFile, 0);
	PHYSFS_write(SaveFile, Current_level_name.line(), 15, 1);
	PHYSFS_writeSLE32(SaveFile, Current_level_num);
	offset_offset = PHYSFS_tell(SaveFile);	// write the offsets later
	PHYSFS_writeSLE32(SaveFile, -1);
	PHYSFS_writeSLE32(SaveFile, 0);

#define WRITE_HEADER_ENTRY(t, n) do { PHYSFS_writeSLE32(SaveFile, -1); PHYSFS_writeSLE32(SaveFile, n); PHYSFS_writeSLE32(SaveFile, sizeof(t)); } while(0)

	WRITE_HEADER_ENTRY(object, Highest_object_index + 1);
	WRITE_HEADER_ENTRY(wall, Num_walls);
	WRITE_HEADER_ENTRY(active_door, ActiveDoors.get_count());
	WRITE_HEADER_ENTRY(trigger, Num_triggers);
	WRITE_HEADER_ENTRY(0, 0);		// links (removed by Parallax)
	WRITE_HEADER_ENTRY(control_center_triggers, 1);
	WRITE_HEADER_ENTRY(matcen_info, Num_robot_centers);

#if defined(DXX_BUILD_DESCENT_II)
	unsigned num_delta_lights = 0;
	if (game_top_fileinfo_version >= 29)
	{
		const unsigned Num_static_lights = Dl_indices.get_count();
		WRITE_HEADER_ENTRY(dl_index, Num_static_lights);
		WRITE_HEADER_ENTRY(delta_light, num_delta_lights = compute_num_delta_light_records());
	}

	// Write the mine name
	if (game_top_fileinfo_version >= 31)
#endif
		PHYSFSX_printf(SaveFile, "%s\n", static_cast<const char *>(Current_level_name));
#if defined(DXX_BUILD_DESCENT_II)
	else if (game_top_fileinfo_version >= 14)
		PHYSFSX_writeString(SaveFile, Current_level_name);

	if (game_top_fileinfo_version >= 19)
#endif
	{
		PHYSFS_writeSLE16(SaveFile, N_polygon_models);
		range_for (auto &i, partial_const_range(Pof_names, N_polygon_models))
			PHYSFS_write(SaveFile, &i, sizeof(i), 1);
	}

	//==================== SAVE PLAYER INFO ===========================

	player_offset = PHYSFS_tell(SaveFile);

	//==================== SAVE OBJECT INFO ===========================

	object_offset = PHYSFS_tell(SaveFile);
	range_for (const auto &&objp, vcobjptr)
	{
		write_object(objp, game_top_fileinfo_version, SaveFile);
	}

	//==================== SAVE WALL INFO =============================

	walls_offset = PHYSFS_tell(SaveFile);
	range_for (const auto &&w, vcwallptr)
		wall_write(SaveFile, *w, game_top_fileinfo_version);

	//==================== SAVE TRIGGER INFO =============================

	triggers_offset = PHYSFS_tell(SaveFile);
	range_for (const auto vt, vctrgptr)
	{
		auto &t = *vt;
		if (game_top_fileinfo_version <= 29)
			v29_trigger_write(SaveFile, t);
		else if (game_top_fileinfo_version <= 30)
			v30_trigger_write(SaveFile, t);
		else if (game_top_fileinfo_version >= 31)
			v31_trigger_write(SaveFile, t);
	}

	//================ SAVE CONTROL CENTER TRIGGER INFO ===============

	control_offset = PHYSFS_tell(SaveFile);
	control_center_triggers_write(&ControlCenterTriggers, SaveFile);


	//================ SAVE MATERIALIZATION CENTER TRIGGER INFO ===============

	matcen_offset = PHYSFS_tell(SaveFile);
	range_for (auto &r, partial_const_range(RobotCenters, Num_robot_centers))
		matcen_info_write(SaveFile, r, game_top_fileinfo_version);

	//================ SAVE DELTA LIGHT INFO ===============
#if defined(DXX_BUILD_DESCENT_II)
	if (game_top_fileinfo_version >= 29)
	{
		dl_indices_offset = PHYSFS_tell(SaveFile);
		range_for (const auto &&i, Dl_indices.vcptr)
			dl_index_write(i, SaveFile);

		delta_light_offset = PHYSFS_tell(SaveFile);
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
#if defined(DXX_BUILD_DESCENT_II)
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
}

// -----------------------------------------------------------------------------
// Save game
namespace dsx {
static int save_level_sub(fvmobjptridx &vmobjptridx, const char * filename)
{
	char temp_filename[PATH_MAX];
	int minedata_offset=0,gamedata_offset=0;

//	if ( !compiled_version )
	{
		write_game_text_file(filename);

		if (Errors_in_mine) {
			if (is_real_level(filename)) {
				gr_palette_load(gr_palette);
	 
				if (nm_messagebox( NULL, 2, "Cancel Save", "Save", "Warning: %i errors in this mine!\n", Errors_in_mine )!=1)	{
					return 1;
				}
			}
		}
//		change_filename_extension(temp_filename,filename,".LVL");
	}
//	else
	{
#if defined(DXX_BUILD_DESCENT_II)
		if (Gamesave_current_version > 3)
			change_filename_extension(temp_filename, filename, "." D2X_LEVEL_FILE_EXTENSION);
		else
#endif
			change_filename_extension(temp_filename, filename, "." D1X_LEVEL_FILE_EXTENSION);
	}

	auto SaveFile = PHYSFSX_openWriteBuffered(temp_filename);
	if (!SaveFile)
	{
		gr_palette_load(gr_palette);
		nm_messagebox( NULL, 1, "Ok", "ERROR: Cannot write to '%s'.", temp_filename);
		return 1;
	}

	if (Current_level_name[0] == 0)
		strcpy(Current_level_name.next().data(),"Untitled");

	clear_transient_objects(1);		//1 means clear proximity bombs

	compress_objects();		//after this, Highest_object_index == num objects

	//make sure player is in a segment
	{
		const auto &&plr = vmobjptridx(vcplayerptr(0u)->objnum);
		if (update_object_seg(plr) == 0) {
			if (plr->segnum > Highest_segment_index)
				plr->segnum = segment_first;
			compute_segment_center(vcvertptr, plr->pos, vcsegptr(plr->segnum));
		}
	}
 
	fix_object_segs();

	//Write the header

	PHYSFS_writeSLE32(SaveFile, MAKE_SIG('P','L','V','L'));
	PHYSFS_writeSLE32(SaveFile, Gamesave_current_version);

	//save placeholders
	PHYSFS_writeSLE32(SaveFile, minedata_offset);
	PHYSFS_writeSLE32(SaveFile, gamedata_offset);
#if defined(DXX_BUILD_DESCENT_I)
	int hostagetext_offset = 0;
	PHYSFS_writeSLE32(SaveFile, hostagetext_offset);
#endif

	//Now write the damn data

#if defined(DXX_BUILD_DESCENT_II)
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
		PHYSFS_writeSLE32(SaveFile, Base_control_center_explosion_time);
	if (Gamesave_current_version >= 4)
		PHYSFS_writeSLE32(SaveFile, Reactor_strength);

	if (Gamesave_current_version >= 7)
	{
		const auto Num_flickering_lights = Flickering_light_state.Num_flickering_lights;
		PHYSFS_writeSLE32(SaveFile, Num_flickering_lights);
		range_for (auto &i, partial_const_range(Flickering_light_state.Flickering_lights, Num_flickering_lights))
			flickering_light_write(i, SaveFile);
	}

	if (Gamesave_current_version >= 6)
	{
		PHYSFS_writeSLE32(SaveFile, Secret_return_segment);
		PHYSFSX_writeVector(SaveFile, Secret_return_orient.rvec);
		PHYSFSX_writeVector(SaveFile, Secret_return_orient.fvec);
		PHYSFSX_writeVector(SaveFile, Secret_return_orient.uvec);
	}
#endif

	minedata_offset = PHYSFS_tell(SaveFile);
#if 0	// only save compiled mine data
	if ( !compiled_version )	
		save_mine_data(SaveFile);
	else
#endif
		save_mine_data_compiled(SaveFile);
	gamedata_offset = PHYSFS_tell(SaveFile);
	save_game_data(SaveFile);
#if defined(DXX_BUILD_DESCENT_I)
	hostagetext_offset = PHYSFS_tell(SaveFile);
#endif

	PHYSFS_seek(SaveFile, sizeof(int) + sizeof(Gamesave_current_version));
	PHYSFS_writeSLE32(SaveFile, minedata_offset);
	PHYSFS_writeSLE32(SaveFile, gamedata_offset);
#if defined(DXX_BUILD_DESCENT_I)
	PHYSFS_writeSLE32(SaveFile, hostagetext_offset);
#elif defined(DXX_BUILD_DESCENT_II)
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

int save_level(const char * filename)
{
	int r1;

	// Save normal version...
	//save_level_sub(filename, 0);	// just save compiled one

	// Save compiled version...
	r1 = save_level_sub(vmobjptridx, filename);

	return r1;
}

#endif	//EDITOR
