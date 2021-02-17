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
 * Bitmap and palette loading functions.
 *
 */

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pstypes.h"
#include "inferno.h"
#include "gr.h"
#include "bm.h"
#include "u_mem.h"
#include "dxxerror.h"
#include "object.h"
#include "vclip.h"
#include "effects.h"
#include "polyobj.h"
#include "wall.h"
#include "textures.h"
#include "game.h"
#include "multi.h"
#include "iff.h"
#include "powerup.h"
#include "sounds.h"
#include "piggy.h"
#include "aistruct.h"
#include "robot.h"
#include "weapon.h"
#include "gauges.h"
#include "player.h"
#include "endlevel.h"
#include "cntrlcen.h"
#include "makesig.h"
#include "interp.h"
#include "console.h"
#include "rle.h"
#include "physfsx.h"
#include "strutil.h"

#if DXX_USE_EDITOR
#include "editor/texpage.h"
#endif

#include "compiler-range_for.h"
#include "d_range.h"
#include "partial_range.h"
#include <memory>

unsigned NumTextures;

#if DXX_USE_EDITOR
int Num_object_subtypes = 1;
#endif

namespace dsx {
#if defined(DXX_BUILD_DESCENT_I)
int Num_total_object_types;

sbyte	ObjType[MAX_OBJTYPE];
sbyte	ObjId[MAX_OBJTYPE];
fix	ObjStrength[MAX_OBJTYPE];
#elif defined(DXX_BUILD_DESCENT_II)
//the polygon model number to use for the marker
unsigned N_ObjBitmaps;
namespace {
static int extra_bitmap_num;
static void bm_free_extra_objbitmaps();
}
#endif

Textures_array Textures;		// All textures.
//for each model, a model number for dying & dead variants, or -1 if none
std::array<int, MAX_POLYGON_MODELS> Dying_modelnums, Dead_modelnums;
std::array<bitmap_index, N_COCKPIT_BITMAPS> cockpit_bitmap;
}

//right now there's only one player ship, but we can have another by
//adding an array and setting the pointer to the active ship.
namespace dcx {
std::array<uint8_t, ::d2x::MAX_SOUNDS> Sounds, AltSounds;
player_ship only_player_ship;

//----------------- Miscellaneous bitmap pointers ---------------
unsigned Num_cockpits;
}

//---------------- Variables for wall textures ------------------

//---------------- Variables for object textures ----------------

int             First_multi_bitmap_num=-1;

namespace dsx {

enumerated_array<bitmap_index, MAX_OBJ_BITMAPS, object_bitmap_index> ObjBitmaps;
std::array<object_bitmap_index, MAX_OBJ_BITMAPS> ObjBitmapPtrs;     // These point back into ObjBitmaps, since some are used twice.

void gamedata_close()
{
	free_polygon_models(LevelSharedPolygonModelState);
#if defined(DXX_BUILD_DESCENT_II)
	bm_free_extra_objbitmaps();
#endif
	free_endlevel_data();
	rle_cache_close();
	piggy_close();
}

/*
 * reads n tmap_info structs from a PHYSFS_File
 */
#if defined(DXX_BUILD_DESCENT_I)
namespace {
static void tmap_info_read(tmap_info &ti, PHYSFS_File *fp)
{
	PHYSFS_read(fp, ti.filename, 13, 1);
	ti.flags = PHYSFSX_readByte(fp);
	ti.lighting = PHYSFSX_readFix(fp);
	ti.damage = PHYSFSX_readFix(fp);
	ti.eclip_num = PHYSFSX_readInt(fp);
}
}

//-----------------------------------------------------------------
// Initializes game properties data (including texture caching system) and sound data.
int gamedata_init()
{
	int retval;
	
	init_polygon_models(LevelSharedPolygonModelState);
	retval = properties_init();				// This calls properties_read_cmp if appropriate
	if (retval)
		gamedata_read_tbl(Vclip, retval == PIGGY_PC_SHAREWARE);

	piggy_read_sounds(retval == PIGGY_PC_SHAREWARE);
	
	return 0;
}

// Read compiled properties data from descent.pig
// (currently only ever called if D1)
void properties_read_cmp(d_vclip_array &Vclip, PHYSFS_File * fp)
{
	auto &Effects = LevelUniqueEffectsClipState.Effects;
	auto &Robot_joints = LevelSharedRobotJointState.Robot_joints;
	auto &TmapInfo = LevelUniqueTmapInfoState.TmapInfo;
	auto &WallAnims = GameSharedState.WallAnims;
	//  bitmap_index is a short
	
	NumTextures = PHYSFSX_readInt(fp);
	bitmap_index_read_n(fp, Textures);
	range_for (tmap_info &ti, TmapInfo)
		tmap_info_read(ti, fp);

	PHYSFS_read(fp, Sounds, sizeof(Sounds[0]), MAX_SOUNDS);
	PHYSFS_read(fp, AltSounds, sizeof(AltSounds[0]), MAX_SOUNDS);
	
	Num_vclips = PHYSFSX_readInt(fp);
	range_for (vclip &vc, Vclip)
		vclip_read(fp, vc);

	Num_effects = PHYSFSX_readInt(fp);
	range_for (eclip &ec, Effects)
		eclip_read(fp, ec);

	Num_wall_anims = PHYSFSX_readInt(fp);
	range_for (auto &w, WallAnims)
		wclip_read(fp, w);

	LevelSharedRobotInfoState.N_robot_types = PHYSFSX_readInt(fp);
	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	range_for (auto &r, Robot_info)
		robot_info_read(fp, r);

	LevelSharedRobotJointState.N_robot_joints = PHYSFSX_readInt(fp);
	range_for (auto &r, Robot_joints)
		jointpos_read(fp, r);

	N_weapon_types = PHYSFSX_readInt(fp);
	weapon_info_read_n(Weapon_info, MAX_WEAPON_TYPES, fp, 0);

	N_powerup_types = PHYSFSX_readInt(fp);
	range_for (auto &p, Powerup_info)
		powerup_type_info_read(fp, p);

	auto &Polygon_models = LevelSharedPolygonModelState.Polygon_models;
	const auto N_polygon_models = LevelSharedPolygonModelState.N_polygon_models = PHYSFSX_readInt(fp);
	{
		const auto &&r = partial_range(Polygon_models, N_polygon_models);
	range_for (auto &p, r)
		polymodel_read(&p, fp);

	range_for (auto &p, r)
		polygon_model_data_read(&p, fp);
	}

	bitmap_index_read_n(fp, partial_range(Gauges, MAX_GAUGE_BMS));
	
	range_for (auto &i, Dying_modelnums)
		i = PHYSFSX_readInt(fp);
	range_for (auto &i, Dead_modelnums)
		i = PHYSFSX_readInt(fp);

	bitmap_index_read_n(fp, ObjBitmaps);
	range_for (auto &i, ObjBitmapPtrs)
	{
		const auto oi = static_cast<object_bitmap_index>(PHYSFSX_readShort(fp));
		if (ObjBitmaps.valid_index(oi))
			i = oi;
		else
			i = {};
	}

	player_ship_read(&only_player_ship, fp);

	Num_cockpits = PHYSFSX_readInt(fp);
	bitmap_index_read_n(fp, cockpit_bitmap);

	PHYSFS_read(fp, Sounds, sizeof(Sounds[0]), MAX_SOUNDS);
	PHYSFS_read(fp, AltSounds, sizeof(AltSounds[0]), MAX_SOUNDS);

	Num_total_object_types = PHYSFSX_readInt(fp);
	PHYSFS_read( fp, ObjType, sizeof(ubyte), MAX_OBJTYPE );
	PHYSFS_read( fp, ObjId, sizeof(ubyte), MAX_OBJTYPE );
	range_for (auto &i, ObjStrength)
		i = PHYSFSX_readFix(fp);

	First_multi_bitmap_num = PHYSFSX_readInt(fp);
	Reactors[0].n_guns = PHYSFSX_readInt(fp);

	range_for (auto &i, Reactors[0].gun_points)
		PHYSFSX_readVector(fp, i);
	range_for (auto &i, Reactors[0].gun_dirs)
		PHYSFSX_readVector(fp, i);

	exit_modelnum = PHYSFSX_readInt(fp);	
	destroyed_exit_modelnum = PHYSFSX_readInt(fp);

#if DXX_USE_EDITOR
        //Build tmaplist
	auto &&effect_range = partial_const_range(Effects, Num_effects);
	LevelUniqueTmapInfoState.Num_tmaps = TextureEffects + std::count_if(effect_range.begin(), effect_range.end(), [](const eclip &e) { return e.changing_wall_texture >= 0; });
        #endif
}
#elif defined(DXX_BUILD_DESCENT_II)
namespace {
static void tmap_info_read(tmap_info &ti, PHYSFS_File *fp)
{
	ti.flags = PHYSFSX_readByte(fp);
	PHYSFSX_readByte(fp);
	PHYSFSX_readByte(fp);
	PHYSFSX_readByte(fp);
	ti.lighting = PHYSFSX_readFix(fp);
	ti.damage = PHYSFSX_readFix(fp);
	ti.eclip_num = PHYSFSX_readShort(fp);
	ti.destroyed = PHYSFSX_readShort(fp);
	ti.slide_u = PHYSFSX_readShort(fp);
	ti.slide_v = PHYSFSX_readShort(fp);
}
}

//-----------------------------------------------------------------
// Initializes game properties data (including texture caching system) and sound data.
int gamedata_init()
{
	init_polygon_models(LevelSharedPolygonModelState);

#if DXX_USE_EDITOR
	// The pc_shareware argument is currently unused for Descent 2,
	// but *may* be useful for loading Descent 1 Shareware texture properties.
	if (!gamedata_read_tbl(Vclip, 0))
#endif
		if (!properties_init())				// This calls properties_read_cmp
				Error("Cannot open ham file\n");

	piggy_read_sounds();

	return 0;
}

void bm_read_all(d_vclip_array &Vclip, PHYSFS_File * fp)
{
	auto &Effects = LevelUniqueEffectsClipState.Effects;
	auto &Robot_joints = LevelSharedRobotJointState.Robot_joints;
	auto &TmapInfo = LevelUniqueTmapInfoState.TmapInfo;
	auto &WallAnims = GameSharedState.WallAnims;
	unsigned t;

	NumTextures = PHYSFSX_readInt(fp);
	bitmap_index_read_n(fp, partial_range(Textures, NumTextures));
	range_for (tmap_info &ti, partial_range(TmapInfo, NumTextures))
		tmap_info_read(ti, fp);

	t = PHYSFSX_readInt(fp);
	PHYSFS_read( fp, Sounds, sizeof(ubyte), t );
	PHYSFS_read( fp, AltSounds, sizeof(ubyte), t );

	Num_vclips = PHYSFSX_readInt(fp);
	range_for (vclip &vc, partial_range(Vclip, Num_vclips))
		vclip_read(fp, vc);

	Num_effects = PHYSFSX_readInt(fp);
	range_for (eclip &ec, partial_range(Effects, Num_effects))
		eclip_read(fp, ec);

	Num_wall_anims = PHYSFSX_readInt(fp);
	range_for (auto &w, partial_range(WallAnims, Num_wall_anims))
		wclip_read(fp, w);

	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	LevelSharedRobotInfoState.N_robot_types = PHYSFSX_readInt(fp);
	range_for (auto &r, partial_range(Robot_info, LevelSharedRobotInfoState.N_robot_types))
		robot_info_read(fp, r);

	const auto N_robot_joints = LevelSharedRobotJointState.N_robot_joints = PHYSFSX_readInt(fp);
	range_for (auto &r, partial_range(Robot_joints, N_robot_joints))
		jointpos_read(fp, r);

	N_weapon_types = PHYSFSX_readInt(fp);
	weapon_info_read_n(Weapon_info, N_weapon_types, fp, Piggy_hamfile_version);

	N_powerup_types = PHYSFSX_readInt(fp);
	range_for (auto &p, partial_range(Powerup_info, N_powerup_types))
		powerup_type_info_read(fp, p);

	auto &Polygon_models = LevelSharedPolygonModelState.Polygon_models;
	const auto N_polygon_models = LevelSharedPolygonModelState.N_polygon_models = PHYSFSX_readInt(fp);
	{
		const auto &&r = partial_range(Polygon_models, N_polygon_models);
	range_for (auto &p, r)
		polymodel_read(&p, fp);

	range_for (auto &p, r)
		polygon_model_data_read(&p, fp);
	}

	range_for (auto &i, partial_range(Dying_modelnums, N_polygon_models))
		i = PHYSFSX_readInt(fp);
	range_for (auto &i, partial_range(Dead_modelnums, N_polygon_models))
		i = PHYSFSX_readInt(fp);

	t = PHYSFSX_readInt(fp);
	bitmap_index_read_n(fp, partial_range(Gauges, t));
	bitmap_index_read_n(fp, partial_range(Gauges_hires, t));

	N_ObjBitmaps = PHYSFSX_readInt(fp);
	bitmap_index_read_n(fp, partial_range(ObjBitmaps, N_ObjBitmaps));
	range_for (auto &i, partial_range(ObjBitmapPtrs, N_ObjBitmaps))
	{
		const auto oi = static_cast<object_bitmap_index>(PHYSFSX_readShort(fp));
		if (ObjBitmaps.valid_index(oi))
			i = oi;
		else
			i = {};
	}

	player_ship_read(&only_player_ship, fp);

	Num_cockpits = PHYSFSX_readInt(fp);
	bitmap_index_read_n(fp, partial_range(cockpit_bitmap, Num_cockpits));

//@@	PHYSFS_read( fp, &Num_total_object_types, sizeof(int), 1 );
//@@	PHYSFS_read( fp, ObjType, sizeof(byte), Num_total_object_types );
//@@	PHYSFS_read( fp, ObjId, sizeof(byte), Num_total_object_types );
//@@	PHYSFS_read( fp, ObjStrength, sizeof(fix), Num_total_object_types );

	First_multi_bitmap_num = PHYSFSX_readInt(fp);

	Num_reactors = PHYSFSX_readInt(fp);
	reactor_read_n(fp, partial_range(Reactors, Num_reactors));

	LevelSharedPolygonModelState.Marker_model_num = PHYSFSX_readInt(fp);

	//@@PHYSFS_read( fp, &N_controlcen_guns, sizeof(int), 1 );
	//@@PHYSFS_read( fp, controlcen_gun_points, sizeof(vms_vector), N_controlcen_guns );
	//@@PHYSFS_read( fp, controlcen_gun_dirs, sizeof(vms_vector), N_controlcen_guns );

	if (Piggy_hamfile_version < 3) { // D1
		exit_modelnum = PHYSFSX_readInt(fp);
		destroyed_exit_modelnum = PHYSFSX_readInt(fp);
	}
	else	// D2: to be loaded later
		exit_modelnum = destroyed_exit_modelnum = N_polygon_models;
}

namespace {

// this and below only really used for D2
bool Exit_bitmaps_loaded;
unsigned Exit_bitmap_index;

static void bm_free_extra_objbitmaps()
{
	int i;

	if (!extra_bitmap_num)
		extra_bitmap_num = Num_bitmap_files;

	for (i = Num_bitmap_files; i < extra_bitmap_num; i++)
	{
		N_ObjBitmaps--;
		d_free(GameBitmaps[i].bm_mdata);
	}
	extra_bitmap_num = Num_bitmap_files;
	Exit_bitmaps_loaded = false;
}

static void bm_free_extra_models(d_level_shared_polygon_model_state &LevelSharedPolygonModelState)
{
	LevelSharedPolygonModelState.Exit_models_loaded = false;
	const auto base = std::min(N_D2_POLYGON_MODELS.value, exit_modelnum);
	for (auto &p : partial_range(LevelSharedPolygonModelState.Polygon_models, base, std::exchange(LevelSharedPolygonModelState.N_polygon_models, base)))
		free_model(p);
}

}

//type==1 means 1.1, type==2 means 1.2 (with weapons)
void bm_read_extra_robots(const char *fname, Mission::descent_version_type type)
{
	auto &Robot_joints = LevelSharedRobotJointState.Robot_joints;
	int t,version;

	auto fp = PHYSFSX_openReadBuffered(fname);
	if (!fp)
	{
		Error("Failed to open HAM file \"%s\"", fname);
		return;
	}

	if (type == Mission::descent_version_type::descent2z)
	{
		int sig;

		sig = PHYSFSX_readInt(fp);
		if (sig != MAKE_SIG('X','H','A','M'))
			return;
		version = PHYSFSX_readInt(fp);
	}
	else
		version = 0;
	(void)version; // NOTE: we do not need it, but keep it for possible further use

	bm_free_extra_models(LevelSharedPolygonModelState);
	bm_free_extra_objbitmaps();

	//read extra weapons

	t = PHYSFSX_readInt(fp);
	N_weapon_types = N_D2_WEAPON_TYPES+t;
	weapon_info_read_n(Weapon_info, N_weapon_types, fp, 3, N_D2_WEAPON_TYPES);

	//now read robot info

	t = PHYSFSX_readInt(fp);
	const auto N_robot_types = LevelSharedRobotInfoState.N_robot_types = N_D2_ROBOT_TYPES + t;
	if (N_robot_types >= MAX_ROBOT_TYPES)
		Error("Too many robots (%d) in <%s>.  Max is %d.",t,fname,MAX_ROBOT_TYPES-N_D2_ROBOT_TYPES);
	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	range_for (auto &r, partial_range(Robot_info, N_D2_ROBOT_TYPES.value, N_robot_types))
		robot_info_read(fp, r);

	t = PHYSFSX_readInt(fp);
	const auto N_robot_joints = LevelSharedRobotJointState.N_robot_joints = N_D2_ROBOT_JOINTS+t;
	if (N_robot_joints >= MAX_ROBOT_JOINTS)
		Error("Too many robot joints (%d) in <%s>.  Max is %d.",t,fname,MAX_ROBOT_JOINTS-N_D2_ROBOT_JOINTS);
	range_for (auto &r, partial_range(Robot_joints, N_D2_ROBOT_JOINTS.value, N_robot_joints))
		jointpos_read(fp, r);

	unsigned u = PHYSFSX_readInt(fp);
	auto &Polygon_models = LevelSharedPolygonModelState.Polygon_models;
	const auto N_polygon_models = LevelSharedPolygonModelState.N_polygon_models = N_D2_POLYGON_MODELS+u;
	if (N_polygon_models >= MAX_POLYGON_MODELS)
		Error("Too many polygon models (%d) in <%s>.  Max is %d.",u,fname,MAX_POLYGON_MODELS-N_D2_POLYGON_MODELS);
	{
		const auto &&r = partial_range(Polygon_models, N_D2_POLYGON_MODELS.value, N_polygon_models);
		range_for (auto &p, r)
		polymodel_read(&p, fp);

		range_for (auto &p, r)
			polygon_model_data_read(&p, fp);
	}

	range_for (auto &i, partial_range(Dying_modelnums, N_D2_POLYGON_MODELS.value, N_polygon_models))
		i = PHYSFSX_readInt(fp);
	range_for (auto &i, partial_range(Dead_modelnums, N_D2_POLYGON_MODELS.value, N_polygon_models))
		i = PHYSFSX_readInt(fp);

	t = PHYSFSX_readInt(fp);
	if (N_D2_OBJBITMAPS+t >= ObjBitmaps.size())
		Error("Too many object bitmaps (%d) in <%s>.  Max is %" DXX_PRI_size_type ".", t, fname, ObjBitmaps.size() - N_D2_OBJBITMAPS);
	bitmap_index_read_n(fp, partial_range(ObjBitmaps, N_D2_OBJBITMAPS.value, N_D2_OBJBITMAPS + t));

	t = PHYSFSX_readInt(fp);
	if (N_D2_OBJBITMAPPTRS+t >= ObjBitmapPtrs.size())
		Error("Too many object bitmap pointers (%d) in <%s>.  Max is %" DXX_PRI_size_type ".", t, fname, ObjBitmapPtrs.size() - N_D2_OBJBITMAPPTRS);
	range_for (auto &i, partial_range(ObjBitmapPtrs, N_D2_OBJBITMAPPTRS.value, N_D2_OBJBITMAPPTRS + t))
	{
		const auto oi = static_cast<object_bitmap_index>(PHYSFSX_readShort(fp));
		if (ObjBitmaps.valid_index(oi))
			i = oi;
		else
			i = {};
	}
}

int Robot_replacements_loaded = 0;

void load_robot_replacements(const d_fname &level_name)
{
	auto &Robot_joints = LevelSharedRobotJointState.Robot_joints;
	int t,j;
	char ifile_name[FILENAME_LEN];

	change_filename_extension(ifile_name, level_name, ".HXM" );

	auto fp = PHYSFSX_openReadBuffered(ifile_name);
	if (!fp)		//no robot replacement file
		return;

	t = PHYSFSX_readInt(fp);			//read id "HXM!"
	if (t!= 0x21584d48)
		Error("ID of HXM! file incorrect");

	t = PHYSFSX_readInt(fp);			//read version
	if (t<1)
		Error("HXM! version too old (%d)",t);

	t = PHYSFSX_readInt(fp);			//read number of robots
	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	const auto N_robot_types = LevelSharedRobotInfoState.N_robot_types;
	for (j=0;j<t;j++) {
		const unsigned i = PHYSFSX_readInt(fp);		//read robot number
		if (i >= N_robot_types)
			Error("Robots number (%u) out of range in (%s).  Range = [0..%u].", i, static_cast<const char *>(level_name), N_robot_types- 1);
		robot_info_read(fp, Robot_info[i]);
	}

	t = PHYSFSX_readInt(fp);			//read number of joints
	const auto N_robot_joints = LevelSharedRobotJointState.N_robot_joints;
	for (j=0;j<t;j++) {
		const unsigned i = PHYSFSX_readInt(fp);		//read joint number
		if (i >= N_robot_joints)
			Error("Robots joint (%u) out of range in (%s).  Range = [0..%u].", i, static_cast<const char *>(level_name), N_robot_joints - 1);
		jointpos_read(fp, Robot_joints[i]);
	}

	auto &Polygon_models = LevelSharedPolygonModelState.Polygon_models;
	const auto N_polygon_models = LevelSharedPolygonModelState.N_polygon_models;
	t = PHYSFSX_readInt(fp);			//read number of polygon models
	for (j=0;j<t;j++)
	{
		const unsigned i = PHYSFSX_readInt(fp);		//read model number
		if (i >= N_polygon_models)
			Error("Polygon model (%u) out of range in (%s).  Range = [0..%u].", i, static_cast<const char *>(level_name), N_polygon_models - 1);

		free_model(Polygon_models[i]);
		polymodel_read(&Polygon_models[i], fp);
		polygon_model_data_read(&Polygon_models[i], fp);

		Dying_modelnums[i] = PHYSFSX_readInt(fp);
		Dead_modelnums[i] = PHYSFSX_readInt(fp);
	}

	t = PHYSFSX_readInt(fp);			//read number of objbitmaps
	for (j=0;j<t;j++) {
		const auto oi = static_cast<object_bitmap_index>(PHYSFSX_readInt(fp));		//read objbitmap number
		if (!ObjBitmaps.valid_index(oi))
			Error("Object bitmap number (%u) out of range in (%s).  Range = [0..%" DXX_PRI_size_type "].", static_cast<unsigned>(oi), static_cast<const char *>(level_name), ObjBitmaps.size() - 1);
		bitmap_index_read(fp, ObjBitmaps[oi]);
	}

	t = PHYSFSX_readInt(fp);			//read number of objbitmapptrs
	for (j=0;j<t;j++) {
		const unsigned i = PHYSFSX_readInt(fp);		//read objbitmapptr number
		if (i >= ObjBitmapPtrs.size())
			Error("Object bitmap pointer (%u) out of range in (%s).  Range = [0..%" DXX_PRI_size_type "].", i, static_cast<const char *>(level_name), ObjBitmapPtrs.size() - 1);
		const auto oi = static_cast<object_bitmap_index>(PHYSFSX_readShort(fp));
		if (!ObjBitmaps.valid_index(oi))
			Error("Object bitmap number (%u) out of range in (%s).  Range = [0..%" DXX_PRI_size_type "].", static_cast<unsigned>(oi), static_cast<const char *>(level_name), ObjBitmaps.size() - 1);
		ObjBitmapPtrs[i] = oi;
	}
	Robot_replacements_loaded = 1;
}

namespace {

/*
 * Routines for loading exit models
 *
 * Used by d1 levels (including some add-ons), and by d2 shareware.
 * Could potentially be used by d2 add-on levels, but only if they
 * don't use "extra" robots... or maybe they do
 */

// formerly exitmodel_bm_load_sub
static grs_bitmap *read_extra_bitmap_iff(const char * filename, grs_bitmap &n)
{
	palette_array_t newpal;
	int iff_error;		//reference parm to avoid warning message

	//MALLOC( new, grs_bitmap, 1 );
	iff_error = iff_read_bitmap(filename, n, &newpal);
	if (iff_error != IFF_NO_ERROR)		{
		con_printf(CON_DEBUG, "Error loading exit model bitmap <%s> - IFF error: %s", filename, iff_errormsg(iff_error));
		return nullptr;
	}

	gr_remap_bitmap_good(n, newpal, iff_has_transparency ? iff_transparent_color : -1, 254);

#if !DXX_USE_OGL
	n.avg_color = 0;	//compute_average_pixel(new);
#endif
	return &n;
}

// formerly load_exit_model_bitmap
static grs_bitmap *bm_load_extra_objbitmap(const char *name)
{
	const auto oi = static_cast<object_bitmap_index>(N_ObjBitmaps);
	if (!ObjBitmaps.valid_index(oi))
		return nullptr;
	{
		auto &bitmap_idx = ObjBitmaps[oi];
		const auto bitmap_store_index = bitmap_index{static_cast<uint16_t>(extra_bitmap_num)};
		grs_bitmap &n = GameBitmaps[bitmap_store_index.index];
		if (!read_extra_bitmap_iff(name, n))
		{
			RAIIdmem<char[]> name2(d_strdup(name));
			*strrchr(name2.get(), '.') = '\0';
			if (const auto r = read_extra_bitmap_d1_pig(name2.get(), n); !r)
				return r;
		}
		bitmap_idx = bitmap_store_index;
		++ extra_bitmap_num;

		if (n.bm_w != 64 || n.bm_h != 64)
			Error("Bitmap <%s> is not 64x64",name);
		ObjBitmapPtrs[N_ObjBitmaps] = oi;
		N_ObjBitmaps++;
		assert(N_ObjBitmaps < ObjBitmaps.size());
		return &n;
	}
}

static void bm_unload_last_objbitmaps(unsigned count)
{
	assert(N_ObjBitmaps >= count);

	unsigned new_N_ObjBitmaps = N_ObjBitmaps - count;
	range_for (auto &o, partial_range(ObjBitmaps, new_N_ObjBitmaps, N_ObjBitmaps))
		d_free(GameBitmaps[o.index].bm_mdata);
	N_ObjBitmaps = new_N_ObjBitmaps;
}

}

// only called for D2 registered, but there is a D1 check anyway for
// possible later use
int load_exit_models()
{
	int start_num;

/*
	don't free extra models in native D2 mode -- ziplantil. it's our
	responsibility to make sure the exit stuff is already loaded rather than
	loading it all again.

	however, in D1 mode, we always need to reload everything due to how
	the exit data is loaded (which is different from D2 native mode)
*/
	if (EMULATING_D1) // D1?
	{
		bm_free_extra_models(LevelSharedPolygonModelState);
		bm_free_extra_objbitmaps();
	}
	else if (!Exit_bitmaps_loaded)
	{
		extra_bitmap_num = Num_bitmap_files;
	}

	// make sure there is enough space to load textures and models
	if (!Exit_bitmaps_loaded && N_ObjBitmaps > ObjBitmaps.size() - 6)
	{
		return 0;
	}
	if (!LevelSharedPolygonModelState.Exit_models_loaded && LevelSharedPolygonModelState.N_polygon_models > MAX_POLYGON_MODELS - 2)
	{
		return 0;
	}

	start_num = N_ObjBitmaps;
	if (!Exit_bitmaps_loaded)
	{
		if (!bm_load_extra_objbitmap("steel1.bbm") ||
			!bm_load_extra_objbitmap("rbot061.bbm") ||
			!bm_load_extra_objbitmap("rbot062.bbm") ||
			!bm_load_extra_objbitmap("steel1.bbm") ||
			!bm_load_extra_objbitmap("rbot061.bbm") ||
			!bm_load_extra_objbitmap("rbot063.bbm"))
		{
			// unload the textures that we already loaded
			bm_unload_last_objbitmaps(N_ObjBitmaps - start_num);
			con_puts(CON_NORMAL, "Can't load exit models!");
			return 0;
		}	
		Exit_bitmap_index = start_num;
	}

	auto &Polygon_models = LevelSharedPolygonModelState.Polygon_models;
	if (LevelSharedPolygonModelState.Exit_models_loaded && exit_modelnum < LevelSharedPolygonModelState.N_polygon_models && destroyed_exit_modelnum < LevelSharedPolygonModelState.N_polygon_models)
	{
		// already loaded, just adjust texture indexes
		Polygon_models[exit_modelnum].first_texture = Exit_bitmap_index;
		Polygon_models[destroyed_exit_modelnum].first_texture = Exit_bitmap_index+3;
		return 1;
	}

	if (auto exit_hamfile = PHYSFSX_openReadBuffered("exit.ham"))
	{
		exit_modelnum = LevelSharedPolygonModelState.N_polygon_models++;
		destroyed_exit_modelnum = LevelSharedPolygonModelState.N_polygon_models++;
		polymodel_read(&Polygon_models[exit_modelnum], exit_hamfile);
		polymodel_read(&Polygon_models[destroyed_exit_modelnum], exit_hamfile);
		Polygon_models[exit_modelnum].first_texture = start_num;
		Polygon_models[destroyed_exit_modelnum].first_texture = start_num+3;

		polygon_model_data_read(&Polygon_models[exit_modelnum], exit_hamfile);

		polygon_model_data_read(&Polygon_models[destroyed_exit_modelnum], exit_hamfile);
	} else if (PHYSFSX_exists("exit01.pof",1) && PHYSFSX_exists("exit01d.pof",1)) {

		exit_modelnum = load_polygon_model("exit01.pof", 3, start_num, NULL);
		destroyed_exit_modelnum = load_polygon_model("exit01d.pof", 3, start_num + 3, NULL);

#if DXX_USE_OGL
		ogl_cache_polymodel_textures(exit_modelnum);
		ogl_cache_polymodel_textures(destroyed_exit_modelnum);
#endif
	}
	else if ((exit_hamfile = PHYSFSX_openReadBuffered(D1_PIGFILE)))
	{
		int offset, offset2;
		int hamsize;
		hamsize = PHYSFS_fileLength(exit_hamfile);
		switch (hamsize) { //total hack for loading models
		case D1_PIGSIZE:
			offset = 91848;     /* and 92582  */
			offset2 = 383390;   /* and 394022 */
			break;
		default:
		case D1_SHARE_BIG_PIGSIZE:
		case D1_SHARE_10_PIGSIZE:
		case D1_SHARE_PIGSIZE:
		case D1_10_BIG_PIGSIZE:
		case D1_10_PIGSIZE:
			Int3();             /* exit models should be in .pofs */
			DXX_BOOST_FALLTHROUGH;
		case D1_OEM_PIGSIZE:
		case D1_MAC_PIGSIZE:
		case D1_MAC_SHARE_PIGSIZE:
			// unload the textures that we already loaded
			bm_unload_last_objbitmaps(N_ObjBitmaps - start_num);
			con_puts(CON_NORMAL, "Can't load exit models!");
			return 0;
		}
		PHYSFSX_fseek(exit_hamfile, offset, SEEK_SET);
		exit_modelnum = LevelSharedPolygonModelState.N_polygon_models++;
		destroyed_exit_modelnum = LevelSharedPolygonModelState.N_polygon_models++;
		polymodel_read(&Polygon_models[exit_modelnum], exit_hamfile);
		polymodel_read(&Polygon_models[destroyed_exit_modelnum], exit_hamfile);
		Polygon_models[exit_modelnum].first_texture = start_num;
		Polygon_models[destroyed_exit_modelnum].first_texture = start_num+3;

		PHYSFSX_fseek(exit_hamfile, offset2, SEEK_SET);
		polygon_model_data_read(&Polygon_models[exit_modelnum], exit_hamfile);
		polygon_model_data_read(&Polygon_models[destroyed_exit_modelnum], exit_hamfile);
	} else {
		// unload the textures that we already loaded
		bm_unload_last_objbitmaps(N_ObjBitmaps - start_num);
		con_puts(CON_NORMAL, "Can't load exit models!");
		return 0;
	}

	// set to be loaded, but only on D2 - always reload the data on D1
	LevelSharedPolygonModelState.Exit_models_loaded = Exit_bitmaps_loaded = !EMULATING_D1;
	return 1;
}
#endif

}

void compute_average_rgb(grs_bitmap *bm, std::array<fix, 3> &rgb)
{
	rgb = {};
	if (unlikely(!bm->get_bitmap_data()))
		return;
	const uint_fast32_t bm_h = bm->bm_h;
	const uint_fast32_t bm_w = bm->bm_w;
	if (unlikely(!bm_h) || unlikely(!bm_w))
		return;

	const auto process_one = [&rgb](uint8_t color) {
		if (color == TRANSPARENCY_COLOR)
			return;
		auto &t_rgb = gr_palette[color];
		if (t_rgb.r == t_rgb.g && t_rgb.r == t_rgb.b)
			return;
		rgb[0] += t_rgb.r;
		rgb[1] += t_rgb.g;
		rgb[2] += t_rgb.b;
	};
	if (bm->get_flag_mask(BM_FLAG_RLE))
	{
		bm_rle_expand expander(*bm);
		const auto &&buf = std::make_unique<uint8_t[]>(bm_w);
		range_for (const uint_fast32_t i, xrange(bm_h))
		{
			(void)i;
			const auto &&range = unchecked_partial_range(buf.get(), bm_w);
			if (expander.step(bm_rle_expand_range(range.begin(), range.end())) != bm_rle_expand::again)
				break;
			range_for (const auto color, range)
				process_one(color);
		}
	}
	else
	{
		range_for (const auto color, unchecked_partial_range(bm->bm_data, bm_w * bm_h))
			process_one(color);
	}
}
