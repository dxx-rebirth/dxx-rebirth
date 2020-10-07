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
 * Bitmap and Palette loading functions.
 *
 */

#pragma once

#include <physfs.h>
#include "maths.h"
#include "fwd-vclip.h"
#include "d_array.h"

struct bitmap_index;

#include <cstdint>

struct grs_bitmap;

#ifdef dsx
namespace dsx {
#if defined(DXX_BUILD_DESCENT_I)
constexpr std::integral_constant<unsigned, 800> MAX_TEXTURES{};
#elif defined(DXX_BUILD_DESCENT_II)
constexpr std::integral_constant<unsigned, 1200> MAX_TEXTURES{};
#endif
}
#endif

//tmapinfo flags
#define TMI_VOLATILE    1   //this material blows up when hit
#if defined(DXX_BUILD_DESCENT_II)
#define TMI_WATER       2   //this material is water
#define TMI_FORCE_FIELD 4   //this is force field - flares don't stick
#define TMI_GOAL_BLUE   8   //this is used to remap the blue goal
#define TMI_GOAL_RED    16  //this is used to remap the red goal
#define TMI_GOAL_HOARD  32  //this is used to remap the goals
#endif

#ifdef dsx
#include "inferno.h"
namespace dsx {
struct tmap_info : prohibit_void_ptr<tmap_info>
{
#if defined(DXX_BUILD_DESCENT_I)
	d_fname filename;
	uint8_t			flags;
	fix			lighting;		// 0 to 1
	fix			damage;			//how much damage being against this does
	unsigned eclip_num;		//if not -1, the eclip that changes this   
#define N_COCKPIT_BITMAPS 4
#elif defined(DXX_BUILD_DESCENT_II)
	fix     lighting;  //how much light this casts
	fix     damage;    //how much damage being against this does (for lava)
	uint16_t eclip_num; //the eclip that changes this, or -1
	short   destroyed; //bitmap to show when destroyed, or -1
	short   slide_u,slide_v;    //slide rates of texture, stored in 8:8 fix
	uint8_t   flags;     //values defined above
#if DXX_USE_EDITOR
	d_fname filename;       //used by editor to remap textures
	#endif

#define TMAP_INFO_SIZE 20   // how much space it takes up on disk
#define N_COCKPIT_BITMAPS 6
#endif
};
}

namespace dcx {
extern int Num_object_types;

struct player_ship;
//right now there's only one player ship, but we can have another by
//adding an array and setting the pointer to the active ship.
extern struct player_ship only_player_ship;
constexpr struct player_ship *Player_ship = &only_player_ship;
extern unsigned Num_cockpits;
}

namespace dsx {
extern std::array<bitmap_index, N_COCKPIT_BITMAPS> cockpit_bitmap;
#if DXX_USE_EDITOR
using tmap_xlate_table_array = std::array<short, MAX_TEXTURES>;
extern tmap_xlate_table_array tmap_xlate_table;
#endif

/* This is level-unique because hoard mode assumes it can overwrite a
 * texture.
 */
struct d_level_unique_tmap_info_state
{
	using TmapInfo_array = std::array<tmap_info, MAX_TEXTURES>;
	unsigned Num_tmaps;
	TmapInfo_array TmapInfo;
};

extern d_level_unique_tmap_info_state LevelUniqueTmapInfoState;
// Initializes the palette, bitmap system...
void gamedata_close();
}
void bm_close();

// Initializes the Texture[] array of bmd_bitmap structures.
void init_textures();

#ifdef dsx

namespace dsx {

int gamedata_init();

#if defined(DXX_BUILD_DESCENT_I)

#define OL_ROBOT 				1
#define OL_HOSTAGE 			2
#define OL_POWERUP 			3
#define OL_CONTROL_CENTER	4
#define OL_PLAYER				5
#define OL_CLUTTER			6		//some sort of misc object
#define OL_EXIT				7		//the exit model for external scenes

#define	MAX_OBJTYPE			100

extern int Num_total_object_types;		//	Total number of object types, including robots, hostages, powerups, control centers, faces
extern int8_t	ObjType[MAX_OBJTYPE];		// Type of an object, such as Robot, eg if ObjType[11] == OL_ROBOT, then object #11 is a robot
extern int8_t	ObjId[MAX_OBJTYPE];			// ID of a robot, within its class, eg if ObjType[11] == 3, then object #11 is the third robot
extern fix	ObjStrength[MAX_OBJTYPE];	// initial strength of each object

constexpr std::integral_constant<unsigned, 210> MAX_OBJ_BITMAPS{};

#elif defined(DXX_BUILD_DESCENT_II)

extern int Robot_replacements_loaded;
constexpr std::integral_constant<unsigned, 610> MAX_OBJ_BITMAPS{};
extern unsigned N_ObjBitmaps;
#endif

enum class object_bitmap_index : uint16_t
{
	None = UINT16_MAX
};
extern enumerated_array<bitmap_index, MAX_OBJ_BITMAPS, object_bitmap_index> ObjBitmaps;
extern std::array<object_bitmap_index, MAX_OBJ_BITMAPS> ObjBitmapPtrs;

}

#endif

extern int  Num_object_subtypes;     // Number of possible IDs for the current type of object to be placed

extern int First_multi_bitmap_num;
void compute_average_rgb(grs_bitmap *bm, std::array<fix, 3> &rgb);

namespace dsx {
void load_robot_replacements(const d_fname &level_name);
// Initializes all bitmaps from BITMAPS.TBL file.
int gamedata_read_tbl(d_vclip_array &Vclip, int pc_shareware);

void bm_read_all(d_vclip_array &Vclip, PHYSFS_File * fp);
#if defined(DXX_BUILD_DESCENT_I)
void properties_read_cmp(d_vclip_array &Vclip, PHYSFS_File * fp);
#endif
int ds_load(int skip, const char * filename );
int compute_average_pixel(grs_bitmap *n);

#if defined(DXX_BUILD_DESCENT_II)
int load_exit_models();
//these values are the number of each item in the release of d2
//extra items added after the release get written in an additional hamfile
constexpr std::integral_constant<unsigned, 66> N_D2_ROBOT_TYPES{};
constexpr std::integral_constant<unsigned, 1145> N_D2_ROBOT_JOINTS{};
constexpr std::integral_constant<unsigned, 422> N_D2_OBJBITMAPS{};
constexpr std::integral_constant<unsigned, 502> N_D2_OBJBITMAPPTRS{};
constexpr std::integral_constant<unsigned, 62> N_D2_WEAPON_TYPES{};
#endif
}

#endif
