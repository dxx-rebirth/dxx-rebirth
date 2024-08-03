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
 * Interface to piggy functions.
 *
 */

#pragma once

#include <ranges>
#include <span>
#include "dsx-ns.h"
#include "fwd-inferno.h"
#include "fwd-gr.h"
#include "fwd-partial_range.h"
#include "sounds.h"
#include "physfsx.h"

namespace dcx {

enum class game_sound_offset : int
{
};

struct digi_sound;
extern digi_sound bogus_sound;
extern unsigned Num_sound_files;
struct BitmapFile;

void swap_0_255(grs_bitmap &bmp);
void remove_char(char * s, char c);	// in piggy.cpp

}

#if defined(DXX_BUILD_DESCENT_II)
#define D1_PIGFILE              "descent.pig"
#define MAX_ALIASES 20

namespace dsx {
extern char descent_pig_basename[12];
struct alias;
#if DXX_USE_EDITOR
extern std::array<alias, MAX_ALIASES> alias_list;
extern unsigned Num_aliases;
#endif

extern uint8_t Pigfile_initialized;
}
#endif

// an index into the bitmap collection of the piggy file
enum class bitmap_index : uint16_t;

#if defined(DXX_BUILD_DESCENT_I)
extern int MacPig;
extern int PCSharePig;

extern grs_bitmap bogus_bitmap;
#endif
extern std::array<uint8_t, 64 * 64> bogus_data;

#ifdef dsx
namespace dsx {

void piggy_close();
bitmap_index piggy_register_bitmap(grs_bitmap &bmp, std::span<const char> name, int in_file);
int piggy_register_sound(digi_sound &snd, std::span<const char> name);
bitmap_index piggy_find_bitmap(std::span<const char> name);
void piggy_load_level_data();

#if defined(DXX_BUILD_DESCENT_I)
constexpr std::integral_constant<unsigned, 1800> MAX_BITMAP_FILES{};
#define PIGGY_PC_SHAREWARE 2
#elif defined(DXX_BUILD_DESCENT_II)
// Upped for CD Enhanced
constexpr std::integral_constant<unsigned, 2620> MAX_BITMAP_FILES{};
#endif
#define MAX_SOUND_FILES     MAX_SOUNDS

using GameBitmaps_array = enumerated_array<grs_bitmap, MAX_BITMAP_FILES, bitmap_index>;
extern std::array<digi_sound, MAX_SOUND_FILES> GameSounds;
extern GameBitmaps_array GameBitmaps;
void piggy_bitmap_page_in(GameBitmaps_array &, bitmap_index bmp);

#if defined(DXX_BUILD_DESCENT_I)
void piggy_read_sounds(int pc_shareware);
#elif defined(DXX_BUILD_DESCENT_II)
void piggy_init_pigfile(std::span<const char> filename);
void piggy_read_sounds(void);

//reads in a new pigfile (for new palette)
//returns the size of all the bitmap data
void piggy_new_pigfile(std::span<char, FILENAME_LEN> pigname);

//loads custom bitmaps for current level
void load_bitmap_replacements(std::span<const char, FILENAME_LEN> level_name);
//if descent.pig exists, loads descent 1 texture bitmaps
void load_d1_bitmap_replacements();
/*
 * Find and load the named bitmap from descent.pig
 */
grs_bitmap *read_extra_bitmap_d1_pig(std::span<const char> name, grs_bitmap &out);
void read_sndfile(int required);
#endif
/*
 * reads a bitmap_index structure from a PHYSFS_File
 */
void bitmap_index_read(NamedPHYSFS_File fp, bitmap_index &bi);
void bitmap_index_read_n(NamedPHYSFS_File fp, std::ranges::subrange<bitmap_index *> r);

}
int piggy_find_sound(std::span<const char> name);

void piggy_read_bitmap_data(grs_bitmap * bmp);

#define REMOVE_EOL(s)		remove_char((s),'\n')
#define REMOVE_COMMENTS(s)	remove_char((s),';')
#define REMOVE_DOTS(s)  	remove_char((s),'.')

extern unsigned Num_bitmap_files;
extern ubyte bogus_bitmap_initialized;
#endif
#define space_tab " \t"
#define equal_space " \t="
#if defined(DXX_BUILD_DESCENT_I)
#include "hash.h"
extern hashtable AllBitmapsNames;
extern hashtable AllDigiSndNames;
#elif defined(DXX_BUILD_DESCENT_II)
extern enumerated_array<BitmapFile, MAX_BITMAP_FILES, bitmap_index> AllBitmaps;
#endif

enum class pig_bitmap_offset : unsigned
{
	None = 0
};
