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
 * Functions for managing the pig files.
 *
 */

#include <stdio.h>
#include <string.h>
#include <ranges>

#include "pstypes.h"
#include "strutil.h"
#include "inferno.h"
#include "gr.h"
#include "u_mem.h"
#include "iff.h"
#include "dxxerror.h"
#include "sounds.h"
#include "digi.h"
#include "bm.h"
#include "hash.h"
#include "common/2d/bitmap.h"
#include "args.h"
#include "palette.h"
#include "gamepal.h"
#include "physfsx.h"
#include "rle.h"
#include "piggy.h"
#include "gamemine.h"
#include "textures.h"
#include "texmerge.h"
#include "paging.h"
#include "game.h"
#include "text.h"
#include "vclip.h"
#include "makesig.h"
#include "console.h"
#include "compiler-cf_assert.h"
#include "compiler-range_for.h"
#include "d_range.h"
#include "d_zip.h"
#include "partial_range.h"
#include <memory>

#if defined(DXX_BUILD_DESCENT_I)
#include "custom.h"
#include "snddecom.h"
#define DEFAULT_PIGFILE_REGISTERED      "descent.pig"

#elif defined(DXX_BUILD_DESCENT_II)
#define DEFAULT_PIGFILE_REGISTERED      "groupa.pig"
#define DEFAULT_PIGFILE_SHAREWARE       "d2demo.pig"
#define DEFAULT_HAMFILE_REGISTERED      "descent2.ham"
#define DEFAULT_HAMFILE_SHAREWARE       "d2demo.ham"

#define D1_PALETTE "palette.256"

#define DEFAULT_SNDFILE ((Piggy_hamfile_version < pig_hamfile_version::_3) ? DEFAULT_HAMFILE_SHAREWARE : (GameArg.SndDigiSampleRate == sound_sample_rate::_22k) ? "descent2.s22" : "descent2.s11")

enum class pigfile_size : uint32_t
{
	mac_alien1_pigsize = 5013035,
	mac_alien2_pigsize = 4909916,
	mac_fire_pigsize = 4969035,
	mac_groupa_pigsize = 4929684, // also used for mac shareware
	mac_ice_pigsize = 4923425,
	mac_water_pigsize = 4832403,
};

namespace dsx {

#if DXX_USE_EDITOR
unsigned Num_aliases;
std::array<alias, MAX_ALIASES> alias_list;
#endif

pig_hamfile_version Piggy_hamfile_version;
uint8_t Pigfile_initialized;

char descent_pig_basename[]{D1_PIGFILE};

}
#endif

namespace {

static std::unique_ptr<ubyte[]> BitmapBits;
static std::unique_ptr<ubyte[]> SoundBits;

struct SoundFile
{
	std::array<char, 15> name;
};
}

#if defined(DXX_BUILD_DESCENT_II)
namespace {
#endif
hashtable AllBitmapsNames;
hashtable AllDigiSndNames;
enumerated_array<pig_bitmap_offset, MAX_BITMAP_FILES, bitmap_index> GameBitmapOffset;
#if defined(DXX_BUILD_DESCENT_II)
static std::unique_ptr<uint8_t[]> Bitmap_replacement_data;
static std::array<char, FILENAME_LEN> Current_pigfile;
}
#endif

unsigned Num_bitmap_files;

namespace dsx {
std::array<digi_sound, MAX_SOUND_FILES> GameSounds;
GameBitmaps_array GameBitmaps;
}

#if defined(DXX_BUILD_DESCENT_I)
#define DBM_FLAG_LARGE 	128		// Flags added onto the flags struct in b

static char default_pigfile_registered[]{DEFAULT_PIGFILE_REGISTERED};
static
#endif
enumerated_array<BitmapFile, MAX_BITMAP_FILES, bitmap_index> AllBitmaps;
namespace {
static std::array<SoundFile, MAX_SOUND_FILES> AllSounds;

#define DBM_FLAG_ABM    64 // animated bitmap

static int Piggy_bitmap_cache_size;
static int Piggy_bitmap_cache_next;
static uint8_t *Piggy_bitmap_cache_data;
static enumerated_array<uint8_t, MAX_BITMAP_FILES, bitmap_index> GameBitmapFlags;
static enumerated_array<bitmap_index, MAX_BITMAP_FILES, bitmap_index> GameBitmapXlat;
static RAIINamedPHYSFS_File Piggy_fp;
}

#if defined(DXX_BUILD_DESCENT_I)
#define PIGGY_BUFFER_SIZE (2048*1024)
#elif defined(DXX_BUILD_DESCENT_II)
#define PIGFILE_ID              MAKE_SIG('G','I','P','P') //PPIG
#define PIGFILE_VERSION         2
#define PIGGY_BUFFER_SIZE (2400*1024)
#endif
#define PIGGY_SMALL_BUFFER_SIZE (1400*1024)		// size of buffer when CGameArg.SysLowMem is set

ubyte bogus_bitmap_initialized=0;
std::array<uint8_t, 64 * 64> bogus_data;

#if defined(DXX_BUILD_DESCENT_I)
grs_bitmap bogus_bitmap;
int MacPig = 0;	// using the Macintosh pigfile?
int PCSharePig = 0; // using PC Shareware pigfile?
namespace {
static std::array<int, MAX_SOUND_FILES> SoundCompressed;
}
#elif defined(DXX_BUILD_DESCENT_II)
#define BM_FLAGS_TO_COPY (BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT \
                         | BM_FLAG_NO_LIGHTING | BM_FLAG_RLE | BM_FLAG_RLE_BIG)
#endif

#define DBM_NUM_FRAMES  63

namespace {

struct DiskBitmapHeader
{
	char name[8];
	ubyte dflags;           // bits 0-5 anim frame num, bit 6 abm flag
	ubyte width;            // low 8 bits here, 4 more bits in wh_extra
	ubyte height;           // low 8 bits here, 4 more bits in wh_extra
#if defined(DXX_BUILD_DESCENT_II)
	ubyte wh_extra;         // bits 0-3 width, bits 4-7 height
#endif
	ubyte flags;
	ubyte avg_color;
	int offset;
} __pack__;
#if defined(DXX_BUILD_DESCENT_I)
static_assert(sizeof(DiskBitmapHeader) == 0x11, "sizeof(DiskBitmapHeader) must be 0x11");
#elif defined(DXX_BUILD_DESCENT_II)
static_assert(sizeof(DiskBitmapHeader) == 0x12, "sizeof(DiskBitmapHeader) must be 0x12");

#define DISKBITMAPHEADER_D1_SIZE 17 // no wh_extra
#endif

struct DiskSoundHeader
{
	char name[8];
	int length;
	int data_length;
	int offset;
} __pack__;

}

namespace dsx {
namespace {
static void piggy_bitmap_page_out_all();
#if defined(DXX_BUILD_DESCENT_II)

/* the inverse of the d2 Textures array, but for the descent 1 pigfile.
 * "Textures" looks up a d2 bitmap index given a d2 tmap_num.
 * "d1_tmap_nums" looks up a d1 tmap_num given a d1 bitmap. "-1" means "None".
 */
using d1_tmap_nums_t = std::array<short, 1630>; // 1621 in descent.pig Mac registered
static std::unique_ptr<d1_tmap_nums_t> d1_tmap_nums;

static void free_d1_tmap_nums()
{
	d1_tmap_nums.reset();
}
#if DXX_USE_EDITOR
static int piggy_is_substitutable_bitmap(std::span<const char, 13> name, std::array<char, 32> &subst_name);
static void piggy_write_pigfile(std::span<const char, FILENAME_LEN> filename);
static void write_int(int i,PHYSFS_File *file);
#endif
#endif
}
}

namespace dcx {

namespace {

struct BitmapNameFromHeader
{
	BitmapFile storage;
	std::span<const char> name;
	BitmapNameFromHeader(const DiskBitmapHeader &);
};

BitmapNameFromHeader::BitmapNameFromHeader(const DiskBitmapHeader &bmh) :
	name{
		(bmh.dflags & DBM_FLAG_ABM)
			? (storage = {}, std::span<const char>(storage.name).first(snprintf(storage.name.data(), storage.name.size(), "%.8s#%d", bmh.name, bmh.dflags & DBM_NUM_FRAMES)))
			: std::span(bmh.name)
	}
{
}

static int piggy_is_needed(const int soundnum)
{
	if (!CGameArg.SysLowMem)
		return 1;
	constexpr std::size_t upper_bound = Sounds.size();
	for (const auto i : AltSounds)
	{
		if (i < upper_bound && Sounds[i] == soundnum)
			return 1;
	}
	return 0;
}

/*
 * reads a DiskSoundHeader structure from a PHYSFS_File
 */
static DiskSoundHeader DiskSoundHeader_read(const NamedPHYSFS_File fp)
{
	DiskSoundHeader dsh{};
	PHYSFSX_readBytes(fp, dsh.name, 8);
	dsh.length = PHYSFSX_readInt(fp);
	dsh.data_length = PHYSFSX_readInt(fp);
	dsh.offset = PHYSFSX_readInt(fp);
	return dsh;
}

}

}

/*
 * reads a DiskBitmapHeader structure from a PHYSFS_File
 */
namespace dsx {

namespace {

static DiskBitmapHeader DiskBitmapHeader_read(const NamedPHYSFS_File fp)
{
	DiskBitmapHeader dbh{};
	PHYSFSX_readBytes(fp, dbh.name, 8);
	dbh.dflags = PHYSFSX_readByte(fp);
	dbh.width = PHYSFSX_readByte(fp);
	dbh.height = PHYSFSX_readByte(fp);
#if defined(DXX_BUILD_DESCENT_II)
	dbh.wh_extra = PHYSFSX_readByte(fp);
#endif
	dbh.flags = PHYSFSX_readByte(fp);
	dbh.avg_color = PHYSFSX_readByte(fp);
	dbh.offset = PHYSFSX_readInt(fp);
	return dbh;
}

#if defined(DXX_BUILD_DESCENT_II)
/*
 * reads a descent 1 DiskBitmapHeader structure from a PHYSFS_File
 */
static DiskBitmapHeader DiskBitmapHeader_d1_read(const NamedPHYSFS_File fp)
{
	DiskBitmapHeader dbh{};
	PHYSFSX_readBytes(fp, dbh.name, 8);
	dbh.dflags = PHYSFSX_readByte(fp);
	dbh.width = PHYSFSX_readByte(fp);
	dbh.height = PHYSFSX_readByte(fp);
	dbh.flags = PHYSFSX_readByte(fp);
	dbh.avg_color = PHYSFSX_readByte(fp);
	dbh.offset = PHYSFSX_readInt(fp);
	return dbh;
}
#endif

}

bitmap_index piggy_register_bitmap(grs_bitmap &bmp, const std::span<const char> name, const int in_file)
{
	assert(Num_bitmap_files < AllBitmaps.size());
	const bitmap_index temp{static_cast<uint16_t>(Num_bitmap_files)};

	if (!in_file) {
#if defined(DXX_BUILD_DESCENT_II)
#if DXX_USE_EDITOR
		if ( GameArg.EdiMacData )
			swap_0_255(bmp);
#endif
#endif
		if (CGameArg.DbgNoCompressPigBitmap)
			gr_bitmap_rle_compress(bmp);
	}

	auto &abn = AllBitmaps[temp].name;
	std::memcpy(abn.data(), name.data(), std::min(abn.size() - 1, name.size()));
	abn.back() = 0;
	hashtable_insert(&AllBitmapsNames, AllBitmaps[temp].name.data(), Num_bitmap_files);
#if defined(DXX_BUILD_DESCENT_I)
	GameBitmaps[temp] = bmp;
#endif
	if ( !in_file ) {
		GameBitmapOffset[temp] = pig_bitmap_offset::None;
		GameBitmapFlags[temp] = bmp.get_flags();
	}
	Num_bitmap_files++;

	return temp;
}

int piggy_register_sound(digi_sound &snd, const std::span<const char> name)
{
	int i;

	Assert( Num_sound_files < MAX_SOUND_FILES );

	auto &asn = AllSounds[Num_sound_files].name;
	std::memcpy(asn.data(), name.data(), std::min(asn.size() - 1, name.size()));
	asn.back() = 0;
	hashtable_insert(&AllDigiSndNames, asn.data(), Num_sound_files);
	auto &gs = GameSounds[Num_sound_files];
	gs = std::move(snd);
#if defined(DXX_BUILD_DESCENT_I)
//added/moved on 11/13/99 by Victor Rachels to ready for changing freq
//#ifdef ALLEGRO
        GameSounds[Num_sound_files].freq = snd.freq;

#endif
	i = Num_sound_files;
	Num_sound_files++;
	return i;
}

bitmap_index piggy_find_bitmap(const std::span<const char> entry_name)
{
	auto name = entry_name;
#if defined(DXX_BUILD_DESCENT_II) && DXX_USE_EDITOR
	size_t namelen;
	const auto t = strchr(name.data(), '#');
	if (t != nullptr)
		namelen = t - name.data();
	else
		namelen = strlen(name.data());

	std::array<char, FILENAME_LEN> temp;
	range_for (auto &i, partial_const_range(alias_list, Num_aliases))
		if (i.alias_name[namelen] == 0 && d_strnicmp(name.data(), i.alias_name.data(), namelen) == 0)
		{
			if (t) {                //extra stuff for ABMs
				const auto path = d_splitpath(i.file_name.data());
				snprintf(temp.data(), temp.size(), "%.*s%s\n", DXX_ptrdiff_cast_int(path.base_end - path.base_start), path.base_start, t);
				name = temp;
			}
			else
				name = i.file_name; 
			break;
		}
#endif

	const auto i = hashtable_search(&AllBitmapsNames, name.data());
	Assert( i != 0 );
	if ( i < 0 )
		return bitmap_index{};
	return bitmap_index{static_cast<uint16_t>(i)};
}

}

int piggy_find_sound(const std::span<const char> name)
{
	const auto i = hashtable_search(&AllDigiSndNames, name.data());
	if ( i < 0 )
		return 255;
	return i;
}

namespace dsx {

namespace {

static void piggy_close_file()
{
	if (Piggy_fp)
	{
		Piggy_fp.reset();
#if defined(DXX_BUILD_DESCENT_II)
		Current_pigfile[0] = 0;
#endif
	}
}
}

#if defined(DXX_BUILD_DESCENT_I)
properties_init_result properties_init(d_level_shared_robot_info_state &LevelSharedRobotInfoState)
{
	int sbytes = 0;
	int N_sounds;
	int size;
	GameSounds = {};

	static_assert(GameBitmapXlat.size() == GameBitmaps.size(), "size mismatch");
	for (const uint16_t i : xrange(std::size(GameBitmaps)))
	{
		const bitmap_index bi{i};
		GameBitmapXlat[bi] = bi;
		GameBitmaps[bi].set_flags(BM_FLAG_PAGED_OUT);
	}

	if ( !bogus_bitmap_initialized )	{
		ubyte c;
		bogus_bitmap_initialized = 1;
		c = gr_find_closest_color( 0, 0, 63 );
		bogus_data.fill(c);
		c = gr_find_closest_color( 63, 0, 0 );
		// Make a big red X !
		range_for (const unsigned i, xrange(64u))
		{
			bogus_data[i*64+i] = c;
			bogus_data[i*64+(63-i)] = c;
		}
		gr_init_bitmap(bogus_bitmap, bm_mode::linear, 0, 0, 64, 64, 64, bogus_data.data());
		piggy_register_bitmap(bogus_bitmap, "bogus", 1);
#ifdef ALLEGRO
		bogus_sound.len = 64*64;
#else
        bogus_sound.length = 64*64;
#endif
		bogus_sound.data = digi_sound::allocated_data{bogus_data.data(), game_sound_offset{INT_MAX}};
//added on 11/13/99 by Victor Rachels to ready for changing freq
                bogus_sound.freq = 11025;
//end this section addition - VR
		GameBitmapOffset[(bitmap_index{0})] = pig_bitmap_offset::None;
	}
	
	Piggy_fp = PHYSFSX_openReadBuffered_updateCase(default_pigfile_registered).first;
	if (!Piggy_fp)
	{
		if (!PHYSFS_exists("BITMAPS.TBL") && !PHYSFS_exists("BITMAPS.BIN"))
			Error("Cannot find " DEFAULT_PIGFILE_REGISTERED " or BITMAPS.TBL");
		return properties_init_result::use_gamedata_read_tbl;	// need to run gamedata_read_tbl
	}

	unsigned Pigdata_start;
	switch (descent1_pig_size{PHYSFS_fileLength(Piggy_fp)}) {
		case descent1_pig_size::d1_share_big_pigsize:
		case descent1_pig_size::d1_share_10_pigsize:
		case descent1_pig_size::d1_share_pigsize:
			PCSharePig = 1;
			Pigdata_start = 0;
			break;
		case descent1_pig_size::d1_10_big_pigsize:
		case descent1_pig_size::d1_10_pigsize:
			Pigdata_start = 0;
			break;
		default:
			Warning_puts("Unknown size for " DEFAULT_PIGFILE_REGISTERED);
			Int3();
			[[fallthrough]];
		case descent1_pig_size::d1_mac_pigsize:
		case descent1_pig_size::d1_mac_share_pigsize:
			MacPig = 1;
			[[fallthrough]];
		case descent1_pig_size::d1_pigsize:
		case descent1_pig_size::d1_oem_pigsize:
			Pigdata_start = PHYSFSX_readInt(Piggy_fp );
			break;
	}
	
	HiresGFXAvailable = MacPig;	// for now at least

	properties_init_result retval;
	if (PCSharePig)
		retval = properties_init_result::shareware;	// run gamedata_read_tbl in shareware mode
	else if (GameArg.EdiNoBm || (!PHYSFS_exists("BITMAPS.TBL") && !PHYSFS_exists("BITMAPS.BIN")))
	{
		properties_read_cmp(LevelSharedRobotInfoState, Vclip, Piggy_fp);	// Note connection to above if!!!
		range_for (auto &i, GameBitmapXlat)
		{
			const auto bi = GameBitmapXlat.valid_index(PHYSFSX_readShort(Piggy_fp));
			i = bi ? *bi : bitmap_index::None;
			if (PHYSFS_eof(Piggy_fp))
				break;
		}
		retval = properties_init_result::skip_gamedata_read_tbl;	// don't run gamedata_read_tbl
	}
	else
		retval = properties_init_result::use_gamedata_read_tbl;	// run gamedata_read_tbl

	PHYSFS_seek(Piggy_fp, Pigdata_start);
	size = PHYSFS_fileLength(Piggy_fp) - Pigdata_start;

	const unsigned N_bitmaps = PHYSFSX_readInt(Piggy_fp);
	size -= sizeof(int);
	N_sounds = PHYSFSX_readInt(Piggy_fp);
	size -= sizeof(int);

	const unsigned header_size = (N_bitmaps * sizeof(DiskBitmapHeader)) + (N_sounds * sizeof(DiskSoundHeader));

	for (const unsigned i : xrange(N_bitmaps))
	{
		const auto bmh = DiskBitmapHeader_read(Piggy_fp);
		const bitmap_index bi{static_cast<uint16_t>(i + 1)};
		GameBitmapFlags[bi] = bmh.flags & (BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT | BM_FLAG_NO_LIGHTING | BM_FLAG_RLE);

		GameBitmapOffset[bi] = static_cast<pig_bitmap_offset>(bmh.offset + header_size + (sizeof(int)*2) + Pigdata_start);
		Assert( (i+1) == Num_bitmap_files );

		//size -= sizeof(DiskBitmapHeader);
		const BitmapNameFromHeader bitmap_name(bmh);
		const auto &temp_name = bitmap_name.name;

		grs_bitmap temp_bitmap{};
		const auto iwidth = (bmh.dflags & DBM_FLAG_LARGE) ? bmh.width + 256 : bmh.width;
		gr_init_bitmap(temp_bitmap, bm_mode::linear, 0, 0, 
			iwidth, bmh.height,
			iwidth, Piggy_bitmap_cache_data);
		temp_bitmap.add_flags(BM_FLAG_PAGED_OUT);
#if !DXX_USE_OGL
		temp_bitmap.avg_color = bmh.avg_color;
#endif

		if (MacPig)
		{
			// HACK HACK HACK!!!!!
			if (!d_strnicmp(bmh.name, "cockpit") || !d_strnicmp(bmh.name, "status") || !d_strnicmp(bmh.name, "rearview")) {
				temp_bitmap.bm_w = temp_bitmap.bm_rowsize = 640;
				if (auto &f = GameBitmapFlags[bi]; f & BM_FLAG_RLE)
					f |= BM_FLAG_RLE_BIG;
			}
			if (!d_strnicmp(bmh.name, "cockpit") || !d_strnicmp(bmh.name, "rearview"))
				temp_bitmap.bm_h = 480;
		}
		
		piggy_register_bitmap(temp_bitmap, temp_name, 1);
	}

	if (!MacPig)
	{
	for (unsigned i = 0; i < N_sounds; ++i)
	{
		const auto sndh = DiskSoundHeader_read(Piggy_fp);
		
		//size -= sizeof(DiskSoundHeader);
		digi_sound temp_sound;
		temp_sound.length = sndh.length;

//added on 11/13/99 by Victor Rachels to ready for changing freq
                temp_sound.freq = 11025;
//end this section addition - VR
		const game_sound_offset sound_offset{static_cast<int>(sndh.offset + header_size + (sizeof(int)*2)+Pigdata_start)};
		temp_sound.data = digi_sound::allocated_data{nullptr, sound_offset};
		if (PCSharePig)
			SoundCompressed[Num_sound_files] = sndh.data_length;
		piggy_register_sound(temp_sound, std::span(sndh.name).first<8>());
                sbytes += sndh.length;
	}

		SoundBits = std::make_unique<ubyte[]>(sbytes + 16);
	}

#if 1	//def EDITOR
	Piggy_bitmap_cache_size	= size - header_size - sbytes + 16;
	Assert( Piggy_bitmap_cache_size > 0 );
#else
	Piggy_bitmap_cache_size = PIGGY_BUFFER_SIZE;
	if (CGameArg.SysLowMem)
		Piggy_bitmap_cache_size = PIGGY_SMALL_BUFFER_SIZE;
#endif
	BitmapBits = std::make_unique<ubyte[]>(Piggy_bitmap_cache_size);
	Piggy_bitmap_cache_data = BitmapBits.get();
	Piggy_bitmap_cache_next = 0;

	return retval;
}
#elif defined(DXX_BUILD_DESCENT_II)

//initialize a pigfile, reading headers
//returns the size of all the bitmap data
void piggy_init_pigfile(const std::span<const char> filename)
{
	int i;
	int header_size, N_bitmaps;
#if DXX_USE_EDITOR
	int data_size;
#endif

	piggy_close_file();             //close old pig if still open

	auto &&[fp, physfserr] = PHYSFSX_openReadBuffered(filename.data());
#if !DXX_USE_EDITOR
	auto effective_filename = filename.data();
#endif
	//try pigfile for shareware
	if (!fp)
	{
		auto &&[fp2, physfserr2] = PHYSFSX_openReadBuffered(DEFAULT_PIGFILE_SHAREWARE);
		if (!fp2)
		{
#if DXX_USE_EDITOR
			static_cast<void>(physfserr);
			static_cast<void>(physfserr2);
			return;         //if editor, ok to not have pig, because we'll build one
#else
			Error("Failed to open required files <%s>, <" DEFAULT_PIGFILE_SHAREWARE ">: \"%s\", \"%s\"", filename.data(), PHYSFS_getErrorByCode(physfserr), PHYSFS_getErrorByCode(physfserr2));
#endif
		}
		fp = std::move(fp2);
#if !DXX_USE_EDITOR
		effective_filename = DEFAULT_PIGFILE_SHAREWARE;
#endif
	}
	Piggy_fp = std::move(fp);
	if (Piggy_fp) {                         //make sure pig is valid type file & is up-to-date
		int pig_id,pig_version;

		pig_id = PHYSFSX_readInt(Piggy_fp);
		pig_version = PHYSFSX_readInt(Piggy_fp);
		if (pig_id != PIGFILE_ID || pig_version != PIGFILE_VERSION) {
			Piggy_fp.reset(); //out of date pig
			                        //..so pretend it's not here
#if DXX_USE_EDITOR
			return;         //if editor, ok to not have pig, because we'll build one
		#else
			Error("Cannot load PIG file: expected (id=%.8lx version=%.8x), found (id=%.8x version=%.8x) in \"%s\"", PIGFILE_ID, PIGFILE_VERSION, pig_id, pig_version, effective_filename);
		#endif
		}
	}

	std::copy_n(filename.data(), std::min(filename.size(), std::size(Current_pigfile) - 1), Current_pigfile.begin());

	N_bitmaps = PHYSFSX_readInt(Piggy_fp);

	header_size = N_bitmaps * sizeof(DiskBitmapHeader);

	const unsigned data_start = header_size + PHYSFS_tell(Piggy_fp);
#if DXX_USE_EDITOR
	data_size = PHYSFS_fileLength(Piggy_fp) - data_start;
#endif
	Num_bitmap_files = 1;

	for (i=0; i<N_bitmaps; i++ )
	{
		int width;
		const bitmap_index bi{static_cast<uint16_t>(i + 1)};
		grs_bitmap *const bm = &GameBitmaps[bi];
		
		const auto bmh = DiskBitmapHeader_read(Piggy_fp);
		const BitmapNameFromHeader bitmap_name(bmh);
		const auto &temp_name = bitmap_name.name;
		width = bmh.width + (static_cast<short>(bmh.wh_extra & 0x0f) << 8);
		gr_init_bitmap(*bm, bm_mode::linear, 0, 0, width, bmh.height + (static_cast<short>(bmh.wh_extra & 0xf0) << 4), width, NULL);
		bm->set_flags(BM_FLAG_PAGED_OUT);
#if !DXX_USE_OGL
		bm->avg_color = bmh.avg_color;
#endif

		GameBitmapFlags[bi] = bmh.flags & BM_FLAGS_TO_COPY;
		GameBitmapOffset[bi] = pig_bitmap_offset{bmh.offset + data_start};
		Assert( (i+1) == Num_bitmap_files );
		piggy_register_bitmap(*bm, temp_name, 1);
	}

#if DXX_USE_EDITOR
	Piggy_bitmap_cache_size = data_size + (data_size/10);   //extra mem for new bitmaps
	Assert( Piggy_bitmap_cache_size > 0 );
#else
	Piggy_bitmap_cache_size = PIGGY_BUFFER_SIZE;
	if (CGameArg.SysLowMem)
		Piggy_bitmap_cache_size = PIGGY_SMALL_BUFFER_SIZE;
#endif
	BitmapBits = std::make_unique<ubyte[]>(Piggy_bitmap_cache_size);
	Piggy_bitmap_cache_data = BitmapBits.get();
	Piggy_bitmap_cache_next = 0;

	Pigfile_initialized=1;
}

//reads in a new pigfile (for new palette)
//returns the size of all the bitmap data
void piggy_new_pigfile(const std::span<char, FILENAME_LEN> pigname)
{
	int header_size, N_bitmaps;
#if DXX_USE_EDITOR
	int must_rewrite_pig = 0;
#endif

	d_strlwr(pigname.data());

	if (strcmp(Current_pigfile.data(), pigname.data()) == 0 // correct pig already loaded
	    && !Bitmap_replacement_data) // no need to reload: no bitmaps were altered
		return;

	if (!Pigfile_initialized) {                     //have we ever opened a pigfile?
		piggy_init_pigfile(pigname);            //..no, so do initialization stuff
		return;
	}
	else
		piggy_close_file();             //close old pig if still open

	Piggy_bitmap_cache_next = 0;            //free up cache

	std::copy_n(pigname.data(), std::min(pigname.size(), std::size(Current_pigfile) - 1), Current_pigfile.begin());

	auto &&[fp, physfserr] = PHYSFSX_openReadBuffered(pigname.data());
#if !DXX_USE_EDITOR
	const char *effective_filename = pigname.data();
#endif
	//try pigfile for shareware
	if (!fp)
	{
		if (auto &&[fp2, physfserr2] = PHYSFSX_openReadBuffered(DEFAULT_PIGFILE_SHAREWARE); fp2)
			fp = std::move(fp2);
		else
		{
			if constexpr (DXX_USE_EDITOR)
			{
				static_cast<void>(physfserr);
				static_cast<void>(physfserr2);
			}
			else
				Error("Failed to open required files <%s>, <" DEFAULT_PIGFILE_SHAREWARE ">: \"%s\", \"%s\"", pigname.data(), PHYSFS_getErrorByCode(physfserr), PHYSFS_getErrorByCode(physfserr2));
			/* In an editor build, a pig can be built.  In a non-editor build,
			 * Error was fatal.
			 */
			return;
		}
#if !DXX_USE_EDITOR
		effective_filename = DEFAULT_PIGFILE_SHAREWARE;
#endif
	}
	Piggy_fp = std::move(fp);
	if (Piggy_fp) {  //make sure pig is valid type file & is up-to-date
		int pig_id,pig_version;

		pig_id = PHYSFSX_readInt(Piggy_fp);
		pig_version = PHYSFSX_readInt(Piggy_fp);
		if (pig_id != PIGFILE_ID || pig_version != PIGFILE_VERSION) {
			Piggy_fp.reset();              //out of date pig
			                        //..so pretend it's not here
#if DXX_USE_EDITOR
			return;         //if editor, ok to not have pig, because we'll build one
		#else
			Error("Cannot load PIG file: expected (id=%.8lx version=%.8x), found (id=%.8x version=%.8x) in \"%s\"", PIGFILE_ID, PIGFILE_VERSION, pig_id, pig_version, effective_filename);
		#endif
		}
		N_bitmaps = PHYSFSX_readInt(Piggy_fp);

		header_size = N_bitmaps * sizeof(DiskBitmapHeader);

		const unsigned data_start = header_size + PHYSFS_tell(Piggy_fp);

		for (unsigned i = 1; i <= N_bitmaps; ++i)
		{
			const bitmap_index bi{static_cast<uint16_t>(i)};
			grs_bitmap *const bm = &GameBitmaps[bi];
			int width;
			
			const auto bmh = DiskBitmapHeader_read(Piggy_fp);
			const BitmapNameFromHeader bitmap_name(bmh);
			const auto &temp_name = bitmap_name.name;
			auto &abn = AllBitmaps[bi];
#if DXX_USE_EDITOR
			//Make sure name matches
			if (strcmp(temp_name.data(), abn.name.data()))
			{
				//Int3();       //this pig is out of date.  Delete it
				must_rewrite_pig=1;
			}
#endif
			abn = {};
			std::copy(temp_name.begin(), temp_name.end(), abn.name.begin());

			width = bmh.width + (static_cast<short>(bmh.wh_extra & 0x0f) << 8);
			gr_set_bitmap_data(*bm, NULL);	// free ogl texture
			gr_init_bitmap(*bm, bm_mode::linear, 0, 0, width, bmh.height + (static_cast<short>(bmh.wh_extra & 0xf0) << 4), width, NULL);
			bm->set_flags(BM_FLAG_PAGED_OUT);
#if !DXX_USE_OGL
			bm->avg_color = bmh.avg_color;
#endif

			GameBitmapFlags[bi] = bmh.flags & BM_FLAGS_TO_COPY;
			GameBitmapOffset[bi] = pig_bitmap_offset{bmh.offset + data_start};
		}
	}
	else
		N_bitmaps = 0;          //no pigfile, so no bitmaps

#if !DXX_USE_EDITOR

	Assert(N_bitmaps == Num_bitmap_files-1);

	#else

	if (must_rewrite_pig || (N_bitmaps < Num_bitmap_files-1)) {
		int size;

		//re-read the bitmaps that aren't in this pig

		for (unsigned i = N_bitmaps + 1; i < Num_bitmap_files; ++i)
		{
			const bitmap_index bi{static_cast<uint16_t>(i)};
			auto &abn = AllBitmaps[bi].name;
			if (const auto p = strchr(abn.data(), '#'))
			{   // this is an ABM == animated bitmap
				char abmname[FILENAME_LEN];
				unsigned fnum;
				snprintf(abmname, sizeof(abmname), "%.*s.abm", static_cast<int>(std::min<std::ptrdiff_t>(p - abn.data(), 8)), abn.data());

				auto read_result = iff_read_animbrush(abmname);
				auto &bm = read_result.bm;
				auto &newpal = read_result.palette;
				const auto nframes = read_result.n_bitmaps;
				if (const auto iff_error{read_result.status}; iff_error != iff_status_code::no_error)
				{
					Error("File %s - IFF error: %s",abmname,iff_errormsg(iff_error));
				}
				for (fnum=0;fnum<nframes; fnum++)       {
					int SuperX;

					//SuperX = (GameBitmaps[i+fnum].bm_flags&BM_FLAG_SUPER_TRANSPARENT)?254:-1;
					const bitmap_index bfi{static_cast<uint16_t>(i + fnum)};
					SuperX = (GameBitmapFlags[bfi] & BM_FLAG_SUPER_TRANSPARENT) ? 254 : -1;
					//above makes assumption that supertransparent color is 254

					gr_remap_bitmap_good(*bm[fnum].get(), newpal, iff_has_transparency ? iff_transparent_color : -1, SuperX);

#if !DXX_USE_OGL
					bm[fnum]->avg_color = compute_average_pixel(bm[fnum].get());
#endif

					if ( GameArg.EdiMacData )
						swap_0_255(*bm[fnum].get());

					if (CGameArg.DbgNoCompressPigBitmap)
						gr_bitmap_rle_compress(*bm[fnum].get());

					if (bm[fnum]->get_flag_mask(BM_FLAG_RLE))
						size = *reinterpret_cast<const int *>(bm[fnum]->bm_data);
					else
						size = bm[fnum]->bm_w * bm[fnum]->bm_h;

					memcpy( &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next],bm[fnum]->bm_data,size);
					d_free(bm[fnum]->bm_mdata);
					bm[fnum]->bm_mdata = &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next];
					Piggy_bitmap_cache_next += size;

					GameBitmaps[bfi] = std::move(*bm[fnum]);
				}

				i += nframes-1;         //filled in multiple bitmaps
			}
			else {          //this is a BBM

				grs_bitmap n;
				palette_array_t newpal;
				char bbmname[FILENAME_LEN];
				int SuperX;

				snprintf(bbmname, sizeof(bbmname), "%.8s.bbm", abn.data());
				if (const auto iff_error{iff_read_bitmap(bbmname, n, &newpal)}; iff_error != iff_status_code::no_error)
				{
					Error("File %s - IFF error: %s",bbmname,iff_errormsg(iff_error));
				}

				SuperX = (GameBitmapFlags[bi] & BM_FLAG_SUPER_TRANSPARENT) ? 254 : -1;
				//above makes assumption that supertransparent color is 254

				gr_remap_bitmap_good(n, newpal, iff_has_transparency ? iff_transparent_color : -1, SuperX);

#if !DXX_USE_OGL
				n.avg_color = compute_average_pixel(&n);
#endif

				if ( GameArg.EdiMacData )
					swap_0_255(n);

				if (CGameArg.DbgNoCompressPigBitmap)
					gr_bitmap_rle_compress(n);

				if (n.get_flag_mask(BM_FLAG_RLE))
					size = *reinterpret_cast<const int *>(n.bm_data);
				else
					size = n.bm_w * n.bm_h;

				memcpy( &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next],n.bm_data,size);
				d_free(n.bm_mdata);
				n.bm_mdata = &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next];
				Piggy_bitmap_cache_next += size;
				GameBitmaps[bi] = std::move(n);
			}
		}

		//@@Dont' do these things which are done when writing
		//@@for (i=0; i < Num_bitmap_files; i++ )       {
		//@@    bitmap_index bi;
		//@@    bi.index = i;
		//@@    PIGGY_PAGE_IN( bi );
		//@@}
		//@@
		//@@piggy_close_file();

		piggy_write_pigfile(pigname);

		Current_pigfile[0] = 0;                 //say no pig, to force reload

		piggy_new_pigfile(pigname);             //read in just-generated pig


	}
	#endif  //ifdef EDITOR
}

#define HAMFILE_ID              MAKE_SIG('!','M','A','H') //HAM!
#define HAMFILE_VERSION 3
//version 1 -> 2:  save marker_model_num
//version 2 -> 3:  removed sound files

#define SNDFILE_ID              MAKE_SIG('D','N','S','D') //DSND
#define SNDFILE_VERSION 1

int read_hamfile(d_level_shared_robot_info_state &LevelSharedRobotInfoState)
{
	int ham_id;
	int sound_offset = 0;
	int shareware = 0;

	auto ham_fp = PHYSFSX_openReadBuffered(DEFAULT_HAMFILE_REGISTERED).first;
	
	if (!ham_fp)
	{
		ham_fp = PHYSFSX_openReadBuffered(DEFAULT_HAMFILE_SHAREWARE).first;
		if (ham_fp)
		{
			shareware = 1;
			GameArg.SndDigiSampleRate = sound_sample_rate::_11k;
			if (CGameArg.SndDisableSdlMixer)
			{
				digi_close();
				digi_init();
			}
		}
	}

	if (!ham_fp) {
		return 0;
	}

	//make sure ham is valid type file & is up-to-date
	ham_id = PHYSFSX_readInt(ham_fp);
	/* All code checks only that Piggy_hamfile_version is `< 3` or `! ( < 3 )`,
	 * so clamp any version higher than 3 to just 3.
	 */
	Piggy_hamfile_version = static_cast<pig_hamfile_version>(std::clamp(PHYSFSX_readInt(ham_fp), 0, 3));
	if (ham_id != HAMFILE_ID)
		Error("Failed to load ham file " DEFAULT_HAMFILE_REGISTERED ": expected ham_id=%.8lx, found ham_id=%.8x version=%.8x\n", HAMFILE_ID, ham_id, underlying_value(Piggy_hamfile_version));

	if (Piggy_hamfile_version < pig_hamfile_version::_3) // hamfile contains sound info, probably PC demo
	{
		sound_offset = PHYSFSX_readInt(ham_fp);
		
		if (shareware) // deal with interactive PC demo
		{
			GameArg.GfxSkipHiresGFX = 1;
			//CGameArg.SysLowMem = 1;
		}
	}

	#if 1 //ndef EDITOR
	{
		bm_read_all(LevelSharedRobotInfoState, Vclip, ham_fp);
		range_for (auto &i, GameBitmapXlat)
		{
			const auto bi = GameBitmapXlat.valid_index(PHYSFSX_readShort(ham_fp));
			i = bi ? *bi : bitmap_index::None;
			if (PHYSFS_eof(ham_fp))
				break;
		}
	}
	#endif

	if (Piggy_hamfile_version < pig_hamfile_version::_3) {
		int N_sounds;
		int sound_start;
		int header_size;
		int i;
		int sbytes = 0;
		static int justonce = 1;

		if (!justonce)
		{
			return 1;
		}
		justonce = 0;

		PHYSFS_seek(ham_fp, sound_offset);
		N_sounds = PHYSFSX_readInt(ham_fp);

		sound_start = PHYSFS_tell(ham_fp);

		header_size = N_sounds * sizeof(DiskSoundHeader);

		//Read sounds

		for (i=0; i<N_sounds; i++ ) {
			const auto sndh = DiskSoundHeader_read(ham_fp);
			digi_sound temp_sound;
			temp_sound.length = sndh.length;
			const game_sound_offset sound_offset{sndh.offset + header_size + sound_start};
			temp_sound.data = digi_sound::allocated_data{nullptr, sound_offset};
			piggy_register_sound(temp_sound, std::span(sndh.name).first<8>());
			if (piggy_is_needed(i))
				sbytes += sndh.length;
		}
		SoundBits = std::make_unique<ubyte[]>(sbytes + 16);
	}
	return 1;
}

void read_sndfile(const int required)
{
	int snd_id,snd_version;
	int N_sounds;
	int sound_start;
	int header_size;
	int i;
	int sbytes = 0;

	const auto filename = DEFAULT_SNDFILE;
	auto &&[snd_fp, physfserr] = PHYSFSX_openReadBuffered(filename);
	if (!snd_fp)
	{
		if (required)
			Error("Cannot open sound file: %s: %s", filename, PHYSFS_getErrorByCode(physfserr));
		return;
	}

	//make sure soundfile is valid type file & is up-to-date
	snd_id = PHYSFSX_readInt(snd_fp);
	snd_version = PHYSFSX_readInt(snd_fp);
	if (snd_id != SNDFILE_ID || snd_version != SNDFILE_VERSION) {
		if (required)
			Error("Cannot load sound file: expected (id=%.8lx version=%.8x), found (id=%.8x version=%.8x) in \"%s\"", SNDFILE_ID, SNDFILE_VERSION, snd_id, snd_version, filename);
		return;
	}

	N_sounds = PHYSFSX_readInt(snd_fp);

	sound_start = PHYSFS_tell(snd_fp);
	header_size = N_sounds*sizeof(DiskSoundHeader);

	//Read sounds

	for (i=0; i<N_sounds; i++ ) {
		const auto sndh = DiskSoundHeader_read(snd_fp);
		digi_sound temp_sound;
		temp_sound.length = sndh.length;
		const game_sound_offset sound_offset{sndh.offset + header_size + sound_start};
		temp_sound.data = digi_sound::allocated_data{nullptr, sound_offset};
		piggy_register_sound(temp_sound, std::span(sndh.name).first<8>());
		if (piggy_is_needed(i))
			sbytes += sndh.length;
	}
	SoundBits = std::make_unique<ubyte[]>(sbytes + 16);
}

properties_init_result properties_init(d_level_shared_robot_info_state &LevelSharedRobotInfoState)
{
	int ham_ok=0,snd_ok=0;
	GameSounds = {};
	for (const uint16_t i : xrange(std::size(GameBitmapXlat)))
	{
		const bitmap_index bi{i};
		GameBitmapXlat[bi] = bi;
	}

	if ( !bogus_bitmap_initialized )        {
		ubyte c;

		bogus_bitmap_initialized = 1;
		c = gr_find_closest_color( 0, 0, 63 );
		bogus_data.fill(c);
		c = gr_find_closest_color( 63, 0, 0 );
		// Make a big red X !
		range_for (const unsigned i, xrange(64u))
		{
			bogus_data[i*64+i] = c;
			bogus_data[i*64+(63-i)] = c;
		}
		const bitmap_index bi{static_cast<uint16_t>(Num_bitmap_files)};
		gr_init_bitmap(GameBitmaps[bi], bm_mode::linear, 0, 0, 64, 64, 64, bogus_data.data());
		piggy_register_bitmap(GameBitmaps[bi], "bogus", 1);
		bogus_sound.length = 64*64;
		bogus_sound.data = digi_sound::allocated_data{bogus_data.data(), game_sound_offset{INT_MAX}};
		GameBitmapOffset[(bitmap_index{0})] = pig_bitmap_offset::None;
	}

	snd_ok = ham_ok = read_hamfile(LevelSharedRobotInfoState);

	if (Piggy_hamfile_version >= pig_hamfile_version::_3)
	{
		read_sndfile(1);
	}

	return (ham_ok && snd_ok) ? properties_init_result::success : properties_init_result::failure;               //read ok
}
#endif

#if defined(DXX_BUILD_DESCENT_I)
void piggy_read_sounds(int pc_shareware)
{
	uint8_t * ptr;
	int i;

	if (MacPig)
	{
		// Read Mac sounds converted to RAW format (too messy to read them directly from the resource fork code-wise)
		char soundfile[32] = "Sounds/sounds.array";
		// hack for Mac Demo
		if (auto array = PHYSFSX_openReadBuffered(soundfile).first)
		{
			/* Sounds.size() is the D2-compatible size, but this
			 * statement must read a D1 length, so use MAX_SOUNDS.
			 */
			if (PHYSFSX_readBytes(array, Sounds, MAX_SOUNDS) != MAX_SOUNDS)	// make the 'Sounds' index array match with the sounds we're about to read in
			{
				con_printf(CON_URGENT,"Warning: Can't read Sounds/sounds.array: %s", PHYSFS_getLastError());
				return;
			}
		}
		else if (descent1_pig_size{PHYSFSX_fsize(default_pigfile_registered)} == descent1_pig_size::d1_mac_share_pigsize)
		{
			con_printf(CON_URGENT,"Warning: Missing Sounds/sounds.array for Mac data files");
			return;
		}

		for (i = 0; i < MAX_SOUND_FILES; i++)
		{
			snprintf(soundfile, sizeof(soundfile), "SND%04d.raw", i);
			if (ds_load(0, soundfile) == 255)
				break;
		}

		return;
	}

	ptr = SoundBits.get();

	std::vector<uint8_t> lastbuf;
	for (i=0; i<Num_sound_files; i++ )
	{
		auto &snd = GameSounds[i];
		auto &d = snd.data.get_deleter();
		if (!d.must_free_buffer())
		{
			if ( piggy_is_needed(i) )
			{
				const auto current_sound_offset = d.offset;
				const auto sound_offset = underlying_value(current_sound_offset);
				PHYSFS_seek(Piggy_fp, sound_offset);

				// Read in the sound data!!!
				snd.data = digi_sound::allocated_data{ptr, current_sound_offset};
				ptr += snd.length;
		//Arne's decompress for shareware on all soundcards - Tim@Rikers.org
				if (pc_shareware)
				{
					const auto compressed_length = SoundCompressed[i];
					lastbuf.resize(compressed_length);
					PHYSFSX_readBytes(Piggy_fp, lastbuf.data(), compressed_length);
					sound_decompress(lastbuf.data(), compressed_length, snd.data.get());
				}
				else
					PHYSFSX_readBytes(Piggy_fp, snd.data.get(), snd.length);
			}
		}
	}
}
#elif defined(DXX_BUILD_DESCENT_II)
void piggy_read_sounds(void)
{
	uint8_t * ptr;
	int i;

	ptr = SoundBits.get();
	auto fp = PHYSFSX_openReadBuffered(DEFAULT_SNDFILE).first;
	if (!fp)
		return;

	for (i=0; i<Num_sound_files; i++ )      {
		auto &snd = GameSounds[i];
		auto &d = snd.data.get_deleter();
		if (!d.must_free_buffer())
		{
			if ( piggy_is_needed(i) )       {
				const auto current_sound_offset = snd.data.get_deleter().offset;
				const auto sound_offset = underlying_value(current_sound_offset);
				PHYSFS_seek(fp, sound_offset);

				// Read in the sound data!!!
				snd.data = digi_sound::allocated_data{ptr, current_sound_offset};
				ptr += snd.length;
				PHYSFSX_readBytes(fp, snd.data.get(), snd.length);
			}
			else
				snd.data.reset();
		}
	}
}
#endif

void piggy_bitmap_page_in(GameBitmaps_array &GameBitmaps, const bitmap_index entry_bitmap_index)
{
	const auto i = underlying_value(entry_bitmap_index);
	Assert( i < MAX_BITMAP_FILES );
	Assert( i < Num_bitmap_files );
	Assert( Piggy_bitmap_cache_size > 0 );

	if ( i < 1 ) return;
	if ( i >= MAX_BITMAP_FILES ) return;
	if ( i >= Num_bitmap_files ) return;

	if (GameBitmapOffset[entry_bitmap_index] == pig_bitmap_offset::None)
		return;		// A read-from-disk bitmap!!!

	const auto xlat_bitmap_index = CGameArg.SysLowMem ? GameBitmapXlat[entry_bitmap_index] : entry_bitmap_index;	// Xlat for low-memory settings!
	grs_bitmap *const bmp = &GameBitmaps[xlat_bitmap_index];

	if (bmp->get_flag_mask(BM_FLAG_PAGED_OUT))
	{
		pause_game_world_time p;

	ReDoIt:
		PHYSFS_seek(Piggy_fp, static_cast<unsigned>(GameBitmapOffset[xlat_bitmap_index]));

		gr_set_bitmap_flags(*bmp, GameBitmapFlags[xlat_bitmap_index]);
#if defined(DXX_BUILD_DESCENT_I)
		gr_set_bitmap_data (*bmp, &Piggy_bitmap_cache_data [Piggy_bitmap_cache_next]);
#endif

		if (bmp->get_flag_mask(BM_FLAG_RLE))
		{
			int zsize = PHYSFSX_readInt(Piggy_fp);
#if defined(DXX_BUILD_DESCENT_I)

			// GET JOHN NOW IF YOU GET THIS ASSERT!!!
			Assert( Piggy_bitmap_cache_next+zsize < Piggy_bitmap_cache_size );
			if ( Piggy_bitmap_cache_next+zsize >= Piggy_bitmap_cache_size ) {
				piggy_bitmap_page_out_all();
				goto ReDoIt;
			}
			memcpy( &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next], &zsize, sizeof(int) );
			Piggy_bitmap_cache_next += sizeof(int);
			PHYSFSX_readBytes(Piggy_fp, &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next], zsize - 4);
			if (MacPig)
			{
				rle_swap_0_255(*bmp);
				memcpy(&zsize, bmp->bm_data, 4);
			}
			Piggy_bitmap_cache_next += zsize-4;
#elif defined(DXX_BUILD_DESCENT_II)
			const pigfile_size pigsize{static_cast<uint32_t>(PHYSFS_fileLength(Piggy_fp))};

			// GET JOHN NOW IF YOU GET THIS ASSERT!!!
			//Assert( Piggy_bitmap_cache_next+zsize < Piggy_bitmap_cache_size );
			if ( Piggy_bitmap_cache_next+zsize >= Piggy_bitmap_cache_size ) {
				Int3();
				piggy_bitmap_page_out_all();
				goto ReDoIt;
			}
			PHYSFSX_readBytes(Piggy_fp, &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next + 4], zsize - 4);
			PUT_INTEL_INT(&Piggy_bitmap_cache_data[Piggy_bitmap_cache_next], zsize);
			gr_set_bitmap_data(*bmp, &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next]);

#ifndef MACDATA
			switch (pigsize) {
			default:
				if (!GameArg.EdiMacData)
					break;
				[[fallthrough]];
			case pigfile_size::mac_alien1_pigsize:
			case pigfile_size::mac_alien2_pigsize:
			case pigfile_size::mac_fire_pigsize:
			case pigfile_size::mac_groupa_pigsize:
			case pigfile_size::mac_ice_pigsize:
			case pigfile_size::mac_water_pigsize:
				rle_swap_0_255(*bmp);
				memcpy(&zsize, bmp->bm_data, 4);
				break;
			}
#endif

			Piggy_bitmap_cache_next += zsize;
			if ( Piggy_bitmap_cache_next+zsize >= Piggy_bitmap_cache_size ) {
				Int3();
				piggy_bitmap_page_out_all();
				goto ReDoIt;
			}
#endif

		} else {
			// GET JOHN NOW IF YOU GET THIS ASSERT!!!
			Assert( Piggy_bitmap_cache_next+(bmp->bm_h*bmp->bm_w) < Piggy_bitmap_cache_size );
			if ( Piggy_bitmap_cache_next+(bmp->bm_h*bmp->bm_w) >= Piggy_bitmap_cache_size ) {
				piggy_bitmap_page_out_all();
				goto ReDoIt;
			}
			PHYSFSX_readBytes(Piggy_fp, &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next], bmp->bm_h * bmp->bm_w);
#if defined(DXX_BUILD_DESCENT_I)
			Piggy_bitmap_cache_next+=bmp->bm_h*bmp->bm_w;
			if (MacPig)
				swap_0_255(*bmp);
#elif defined(DXX_BUILD_DESCENT_II)
			const pigfile_size pigsize{static_cast<uint32_t>(PHYSFS_fileLength(Piggy_fp))};
			gr_set_bitmap_data(*bmp, &Piggy_bitmap_cache_data[Piggy_bitmap_cache_next]);
			Piggy_bitmap_cache_next+=bmp->bm_h*bmp->bm_w;

#ifndef MACDATA
			switch (pigsize) {
			default:
				if (!GameArg.EdiMacData)
					break;
				[[fallthrough]];
			case pigfile_size::mac_alien1_pigsize:
			case pigfile_size::mac_alien2_pigsize:
			case pigfile_size::mac_fire_pigsize:
			case pigfile_size::mac_groupa_pigsize:
			case pigfile_size::mac_ice_pigsize:
			case pigfile_size::mac_water_pigsize:
				swap_0_255(*bmp);
				break;
			}
#endif
#endif
		}

		//@@if ( bmp->bm_selector ) {
		//@@#if !defined(WINDOWS) && !defined(MACINTOSH)
		//@@	if (!dpmi_modify_selector_base( bmp->bm_selector, bmp->bm_data ))
		//@@		Error( "Error modifying selector base in piggy.c\n" );
		//@@#endif
		//@@}

		compute_average_rgb(bmp, bmp->avg_color_rgb);

	}

	if (CGameArg.SysLowMem)
	{
		if (entry_bitmap_index != xlat_bitmap_index)
			GameBitmaps[entry_bitmap_index] = GameBitmaps[xlat_bitmap_index];
	}

//@@Removed from John's code:
//@@#ifndef WINDOWS
//@@    if ( bmp->bm_selector ) {
//@@            if (!dpmi_modify_selector_base( bmp->bm_selector, bmp->bm_data ))
//@@                    Error( "Error modifying selector base in piggy.c\n" );
//@@    }
//@@#endif
}

namespace {

void piggy_bitmap_page_out_all()
{
	Piggy_bitmap_cache_next = 0;

	texmerge_flush();
	rle_cache_flush();

	for (auto &&[gbo, gb] : zip(partial_range(GameBitmapOffset, Num_bitmap_files), GameBitmaps))
	{
		if (gbo != pig_bitmap_offset::None)
		{	// Don't page out bitmaps read from disk!!!
			gb.set_flags(BM_FLAG_PAGED_OUT);
			gr_set_bitmap_data(gb, nullptr);
		}
	}
}

}

void piggy_load_level_data()
{
	piggy_bitmap_page_out_all();
	paging_touch_all(Vclip);
}

namespace {
#if defined(DXX_BUILD_DESCENT_II)
#if DXX_USE_EDITOR

static void piggy_write_pigfile(const std::span<const char, FILENAME_LEN> filename)
{
	int bitmap_data_start, data_offset;
	DiskBitmapHeader bmh;
	int org_offset;
	for (const unsigned n = Num_bitmap_files; const uint16_t i : xrange(n))
	{
		const bitmap_index bi{i};
		PIGGY_PAGE_IN( bi );
	}

	piggy_close_file();

	auto pig_fp = PHYSFSX_openWriteBuffered(filename.data()).first;       //open PIG file
	if (!pig_fp)
		return;

	write_int(PIGFILE_ID,pig_fp);
	write_int(PIGFILE_VERSION,pig_fp);

	Num_bitmap_files--;
	PHYSFSX_writeBytes(pig_fp, &Num_bitmap_files, sizeof(int));
	Num_bitmap_files++;

	bitmap_data_start = PHYSFS_tell(pig_fp);
	bitmap_data_start += (Num_bitmap_files - 1) * sizeof(DiskBitmapHeader);
	data_offset = bitmap_data_start;

	std::array<char, FILENAME_LEN> tname;
	if (!change_filename_extension(tname, filename.data(), "lst"))
		return;
	auto fp1 = PHYSFSX_openWriteBuffered(tname.data()).first;
	if (!change_filename_extension(tname, filename.data(), "all"))
		return;
	auto fp2 = PHYSFSX_openWriteBuffered(tname.data()).first;

	for (unsigned i = 1; i < Num_bitmap_files; ++i)
	{
		grs_bitmap *bmp;

		const bitmap_index bi{static_cast<uint16_t>(i)};
		const std::span name = AllBitmaps[bi].name;
		{
			char *p1;
			if (const auto p = strchr(name.data(), '#')) {   // this is an ABM == animated bitmap
				int n;
				p1 = p; p1++; 
				n = atoi(p1);
				*p = 0;
				if (fp2 && n==0)
					PHYSFSX_printf(fp2, "%s.abm\n", name.data());
				memcpy(bmh.name, name.data(), 8);
				Assert( n <= DBM_NUM_FRAMES );
				bmh.dflags = DBM_FLAG_ABM + n;
				*p = '#';
			} else {
				if (fp2)
					PHYSFSX_printf(fp2, "%s.bbm\n", name.data());
				memcpy(bmh.name, name.data(), 8);
				bmh.dflags = 0;
			}
		}
		bmp = &GameBitmaps[bi];

		assert(!bmp->get_flag_mask(BM_FLAG_PAGED_OUT));

		if (fp1)
			PHYSFSX_printf(fp1, "BMP: %s, size %d bytes", name.data(), bmp->bm_rowsize * bmp->bm_h);
		org_offset = PHYSFS_tell(pig_fp);
		bmh.offset = data_offset - bitmap_data_start;
		PHYSFS_seek(pig_fp, data_offset);

		if (bmp->get_flag_mask(BM_FLAG_RLE))
		{
			const auto size = reinterpret_cast<const int *>(bmp->bm_data);
			PHYSFSX_writeBytes(pig_fp, bmp->bm_data, *size);
			data_offset += *size;
			if (fp1)
				PHYSFSX_printf( fp1, ", and is already compressed to %d bytes.\n", *size );
		} else {
			PHYSFSX_writeBytes(pig_fp, bmp->bm_data, bmp->bm_rowsize * bmp->bm_h);
			data_offset += bmp->bm_rowsize * bmp->bm_h;
			if (fp1)
				PHYSFSX_puts_literal( fp1, ".\n" );
		}
		PHYSFS_seek(pig_fp, org_offset);
		auto &gbi = GameBitmaps[bi];
		assert(gbi.bm_w < 4096);
		bmh.width = (gbi.bm_w & 0xff);
		bmh.wh_extra = ((gbi.bm_w >> 8) & 0x0f);
		assert(gbi.bm_h < 4096);
		bmh.height = gbi.bm_h;
		bmh.wh_extra |= ((gbi.bm_h >> 4) & 0xf0);
		bmh.flags = gbi.get_flags();
		if (std::array<char, 32> subst_name; piggy_is_substitutable_bitmap(name, subst_name))
		{
			const auto other_bitmap = piggy_find_bitmap(subst_name);
			GameBitmapXlat[bi] = other_bitmap;
			bmh.flags |= BM_FLAG_PAGED_OUT;
		} else {
			bmh.flags &= ~BM_FLAG_PAGED_OUT;
		}
		bmh.avg_color = compute_average_pixel(&GameBitmaps[bi]);
		PHYSFSX_writeBytes(pig_fp, &bmh, sizeof(DiskBitmapHeader));	// Mark as a bitmap
	}
	PHYSFSX_printf( fp1, " Dumped %d assorted bitmaps.\n", Num_bitmap_files );
}

static void write_int(int i, PHYSFS_File *file)
{
	if (PHYSFSX_writeBytes(file, &i, sizeof(i)) != sizeof(i))
		Error("Error writing int in " __FILE__);

}
#endif

/*
 * Functions for loading replacement textures
 *  1) From .pog files
 *  2) From descent.pig (for loading d1 levels)
 */
static void free_bitmap_replacements()
{
	Bitmap_replacement_data.reset();
}
#endif
}

void piggy_close()
{
#if defined(DXX_BUILD_DESCENT_I)
	custom_close();
#endif
	piggy_close_file();
	BitmapBits.reset();
	SoundBits.reset();
	for (auto &gs : partial_range(GameSounds, Num_sound_files))
		gs.data.reset();
#if defined(DXX_BUILD_DESCENT_II)
	free_bitmap_replacements();
	free_d1_tmap_nums();
#endif
}

#if defined(DXX_BUILD_DESCENT_II)
namespace {

#if DXX_USE_EDITOR
static const BitmapFile *piggy_does_bitmap_exist_slow(const char *const name)
{
	range_for (auto &i, partial_const_range(AllBitmaps, Num_bitmap_files))
	{
		if (!strcmp(i.name.data(), name))
			return &i;
	}
	return nullptr;
}

constexpr char gauge_bitmap_names[][9] = {
	"gauge01",
	"gauge02",
	"gauge06",
	"targ01",
	"targ02",
	"targ03",
	"targ04",
	"targ05",
	"targ06",
	"gauge18",
#if defined(DXX_BUILD_DESCENT_I)
	"targ01pc",
	"targ02pc",
	"targ03pc",
	"gaug18pc"
#elif defined(DXX_BUILD_DESCENT_II)
	"gauge01b",
	"gauge02b",
	"gauge06b",
	"targ01b",
	"targ02b",
	"targ03b",
	"targ04b",
	"targ05b",
	"targ06b",
	"gauge18b",
	"gauss1",
	"helix1",
	"phoenix1"
#endif
};

static const char (*piggy_is_gauge_bitmap(const char *const base_name))[9]
{
	range_for (auto &i, gauge_bitmap_names)
	{
		if (!d_stricmp(base_name, i))
			return &i;
	}
	return nullptr;
}

static int piggy_is_substitutable_bitmap(std::span<const char, 13> name, std::array<char, 32> &subst_name)
{
	std::copy(name.begin(), name.end(), subst_name.begin());
	if (const auto p = strchr(subst_name.data(), '#'))
	{
		const int frame = atoi(&p[1]);
		*p = 0;
		auto base_name = subst_name;
		if (!piggy_is_gauge_bitmap(base_name.data()))
		{
			snprintf(subst_name.data(), subst_name.size(), "%.13s#%d", base_name.data(), frame + 1);
			if (piggy_does_bitmap_exist_slow(subst_name.data()))
			{
				if ( frame & 1 ) {
					snprintf(subst_name.data(), subst_name.size(), "%.13s#%d", base_name.data(), frame - 1);
					return 1;
				}
			}
		}
		std::copy(name.begin(), name.end(), subst_name.begin());
	}
	return 0;
}
#endif

/* returns nonzero if d1_tmap_num references a texture which isn't available in d2. */
int d1_tmap_num_unique(uint16_t d1_tmap_num)
{
	switch (d1_tmap_num) {
	case   0: case   2: case   4: case   5: case   6: case   7: case   9:
	case  10: case  11: case  12: case  17: case  18:
	case  20: case  21: case  25: case  28:
	case  38: case  39: case  41: case  44: case  49:
	case  50: case  55: case  57: case  88:
	case 132: case 141: case 147:
	case 154: case 155: case 158: case 159:
	case 160: case 161: case 167: case 168: case 169:
	case 170: case 171: case 174: case 175: case 185:
	case 193: case 194: case 195: case 198: case 199:
	case 200: case 202: case 210: case 211:
	case 220: case 226: case 227: case 228: case 229: case 230:
	case 240: case 241: case 242: case 243: case 246:
	case 250: case 251: case 252: case 253: case 257: case 258: case 259:
	case 260: case 263: case 266: case 283: case 298:
	case 315: case 317: case 319: case 320: case 321:
	case 330: case 331: case 332: case 333: case 349:
	case 351: case 352: case 353: case 354:
	case 355: case 357: case 358: case 359:
	case 362: case 370: return 1;
	default: return 0;
	}
}

}

void load_bitmap_replacements(const std::span<const char, FILENAME_LEN> level_name)
{
	//first, free up data allocated for old bitmaps
	free_bitmap_replacements();

	std::array<char, FILENAME_LEN> ifile_name;
	if (!change_filename_extension(ifile_name, level_name.data(), "POG"))
		return;
	if (auto ifile = PHYSFSX_openReadBuffered(ifile_name.data()).first)
	{
		int id,version;
		unsigned n_bitmaps;
		int bitmap_data_size;

		id = PHYSFSX_readInt(ifile);
		version = PHYSFSX_readInt(ifile);

		if (id != MAKE_SIG('G','O','P','D') || version != 1) {
			return;
		}

		n_bitmaps = PHYSFSX_readInt(ifile);

		const auto indices = std::make_unique<uint16_t[]>(n_bitmaps);
		range_for (auto &i, unchecked_partial_range(indices.get(), n_bitmaps))
			i = PHYSFSX_readShort(ifile);

		bitmap_data_size = PHYSFS_fileLength(ifile) - PHYSFS_tell(ifile) - sizeof(DiskBitmapHeader) * n_bitmaps;
		Bitmap_replacement_data = std::make_unique<ubyte[]>(bitmap_data_size);

		range_for (const auto i, unchecked_partial_range(indices.get(), n_bitmaps))
		{
			const bitmap_index bi{i};
			grs_bitmap *const bm = &GameBitmaps[bi];
			int width;

			const auto bmh = DiskBitmapHeader_read(ifile);

			width = bmh.width + (static_cast<short>(bmh.wh_extra & 0x0f) << 8);
			gr_set_bitmap_data(*bm, NULL);	// free ogl texture
			gr_init_bitmap(*bm, bm_mode::linear, 0, 0, width, bmh.height + (static_cast<short>(bmh.wh_extra & 0xf0) << 4), width, NULL);
#if !DXX_USE_OGL
			bm->avg_color = bmh.avg_color;
#endif
			bm->bm_data = reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(bmh.offset));

			gr_set_bitmap_flags(*bm, bmh.flags & BM_FLAGS_TO_COPY);
			GameBitmapOffset[bi] = pig_bitmap_offset::None; // don't try to read bitmap from current pigfile
		}

		PHYSFSX_readBytes(ifile, Bitmap_replacement_data, bitmap_data_size);

		range_for (const auto i, unchecked_partial_range(indices.get(), n_bitmaps))
		{
			const bitmap_index bi{i};
			grs_bitmap *const bm = &GameBitmaps[bi];
			gr_set_bitmap_data(*bm, &Bitmap_replacement_data[reinterpret_cast<uintptr_t>(bm->bm_data)]);
		}
		last_palette_loaded_pig[0]= 0;  //force pig re-load
		texmerge_flush();       //for re-merging with new textures
	}
}

namespace {

/* calculate table to translate d1 bitmaps to current palette,
 * return -1 on error
 */
static int get_d1_colormap(palette_array_t &d1_palette, std::array<color_palette_index, 256> &colormap)
{
	auto palette_file = PHYSFSX_openReadBuffered(D1_PALETTE).first;
	if (!palette_file || PHYSFS_fileLength(palette_file) != 9472)
		return -1;
	PHYSFSX_readBytes(palette_file, d1_palette, sizeof(d1_palette[0]) * std::size(d1_palette));
	build_colormap_good(d1_palette, colormap);
	// don't change transparencies:
	colormap[254] = color_palette_index{254};
	colormap[255] = TRANSPARENCY_COLOR;
	return 0;
}

#define JUST_IN_CASE 132 /* is enough for d1 pc registered */
static void bitmap_read_d1( grs_bitmap *bitmap, /* read into this bitmap */
                     const NamedPHYSFS_File d1_Piggy_fp, /* read from this file */
                     int bitmap_data_start, /* specific to file */
                     const DiskBitmapHeader *const bmh, /* header info for bitmap */
                     uint8_t **next_bitmap, /* where to write it (if 0, use malloc) */
					 palette_array_t &d1_palette, /* what palette the bitmap has */
					 std::array<color_palette_index, 256> &colormap) /* how to translate bitmap's colors */
{
	int zsize;
	uint8_t *data;
	int width;

	width = bmh->width + (static_cast<short>(bmh->wh_extra & 0x0f) << 8);
	gr_set_bitmap_data(*bitmap, NULL);	// free ogl texture
	gr_init_bitmap(*bitmap, bm_mode::linear, 0, 0, width, bmh->height + (static_cast<short>(bmh->wh_extra & 0xf0) << 4), width, NULL);
#if !DXX_USE_OGL
	bitmap->avg_color = bmh->avg_color;
#endif
	gr_set_bitmap_flags(*bitmap, bmh->flags & BM_FLAGS_TO_COPY);

	PHYSFS_seek(d1_Piggy_fp, bitmap_data_start + bmh->offset);
	if (bmh->flags & BM_FLAG_RLE) {
		zsize = PHYSFSX_readInt(d1_Piggy_fp);
		PHYSFSX_fseek(d1_Piggy_fp, -4, SEEK_CUR);
	} else
		zsize = bitmap->bm_h * bitmap->bm_w;

	if (next_bitmap) {
		data = *next_bitmap;
		*next_bitmap += zsize;
	} else {
		MALLOC(data, ubyte, zsize + JUST_IN_CASE);
	}
	if (!data) return;

	PHYSFSX_readBytes(d1_Piggy_fp, data, zsize);
	gr_set_bitmap_data(*bitmap, data);
	switch (descent1_pig_size{PHYSFS_fileLength(d1_Piggy_fp)})
	{
		default:
			break;
		case descent1_pig_size::d1_mac_pigsize:
		case descent1_pig_size::d1_mac_share_pigsize:
		if (bmh->flags & BM_FLAG_RLE)
			rle_swap_0_255(*bitmap);
		else
			swap_0_255(*bitmap);
	}
	if (bmh->flags & BM_FLAG_RLE)
		rle_remap(*bitmap, colormap);
	else
		gr_remap_bitmap_good(*bitmap, d1_palette, TRANSPARENCY_COLOR, -1);
	if (bmh->flags & BM_FLAG_RLE) { // size of bitmap could have changed!
		int new_size;
		memcpy(&new_size, bitmap->bm_data, 4);
		if (next_bitmap) {
			*next_bitmap += new_size - zsize;
		} else {
			Assert( zsize + JUST_IN_CASE >= new_size );
			bitmap->bm_mdata = reinterpret_cast<uint8_t *>(d_realloc(bitmap->bm_mdata, new_size));
			Assert(bitmap->bm_data);
		}
	}
}

#define D1_MAX_TEXTURES 800

static void bm_read_d1_tmap_nums(const NamedPHYSFS_File d1pig)
{
	int i;

	PHYSFS_seek(d1pig, 8);
	d1_tmap_nums = std::make_unique<d1_tmap_nums_t>();
	auto &t = *d1_tmap_nums.get();
	t.fill(-1);
	for (i = 0; i < D1_MAX_TEXTURES; i++) {
		const uint16_t d1_index = PHYSFSX_readShort(d1pig);
		if (d1_index >= std::size(t))
			break;
		t[d1_index] = i;
		if (PHYSFS_eof(d1pig))
			break;
	}
}

// this function is at the same position in the d1 shareware piggy loading 
// algorithm as bm_load_sub in main/bmread.c
static int get_d1_bm_index(char *filename, const NamedPHYSFS_File d1_pig)
{
	int i, N_bitmaps;
	if (const auto p{strchr(filename, '.')})
		*p = '\0'; // remove extension
	PHYSFS_seek(d1_pig, 0);
	N_bitmaps = PHYSFSX_readInt(d1_pig);
	PHYSFS_seek(d1_pig, 8);
	for (i = 1; i <= N_bitmaps; i++) {
		const auto bmh{DiskBitmapHeader_d1_read(d1_pig)};
		if (!d_strnicmp(bmh.name, filename, 8))
			return i;
	}
	return -1;
}

// imitate the algorithm of gamedata_read_tbl in main/bmread.c
static void read_d1_tmap_nums_from_hog(const NamedPHYSFS_File d1_pig)
{
#define LINEBUF_SIZE 600
	int reading_textures = 0;
	short texture_count = 0;
	int bitmaps_tbl_is_binary = 0;

	auto &&[bitmaps, physfserr] = PHYSFSX_openReadBuffered("bitmaps.tbl");
	if (!bitmaps) {
		auto &&[bitmapbin, physfserr2] = PHYSFSX_openReadBuffered ("bitmaps.bin");
		if (!bitmapbin)
		{
			Warning("Failed to open \"bitmaps.tbl\", \"bitmaps.bin\": \"%s\", \"%s\".  Descent 1 textures will not be read.", PHYSFS_getErrorByCode(physfserr), PHYSFS_getErrorByCode(physfserr2));
			return;
		}
		bitmaps = std::move(bitmapbin);
		bitmaps_tbl_is_binary = 1;
	}

	d1_tmap_nums = std::make_unique<d1_tmap_nums_t>();
	auto &t = *d1_tmap_nums.get();
	t.fill(-1);

	for (PHYSFSX_gets_line_t<LINEBUF_SIZE> inputline; PHYSFSX_fgets (inputline, bitmaps);)
	{
		char *arg;

		if (bitmaps_tbl_is_binary)
			decode_text_line((inputline));
		else
			for (std::size_t i; inputline[(i=strlen(inputline))-2]=='\\';)
			{
				if (!PHYSFSX_fgets(inputline, bitmaps, i - 2)) // strip comments
					break;
			}
		REMOVE_EOL(inputline);
                if (strchr(inputline, ';')!=NULL) REMOVE_COMMENTS(inputline);
		if (strlen(inputline) == LINEBUF_SIZE-1) {
			Warning_puts("Possible line truncation in BITMAPS.TBL");
			return;
		}
		arg = strtok( inputline, space_tab );
                if (arg && arg[0] == '@') {
			arg++;
			//Registered_only = 1;
		}

                while (arg != NULL) {
			if (*arg == '$')
				reading_textures = 0; // default
			if (!strcmp(arg, "$TEXTURES")) // BM_TEXTURES
				reading_textures = 1;
			else if (! d_stricmp(arg, "$ECLIP") // BM_ECLIP
				   || ! d_stricmp(arg, "$WCLIP")) // BM_WCLIP
					texture_count++;
			else // not a special token, must be a bitmap!
				if (reading_textures) {
					while (*arg == '\t' || *arg == ' ')
						arg++;//remove unwanted blanks
					if (*arg == '\0')
						break;
					if (d1_tmap_num_unique(texture_count)) {
						const uint32_t d1_index = get_d1_bm_index(arg, d1_pig);
						if (d1_index < std::size(t))
						{
							t[d1_index] = texture_count;
							//int d2_index = d2_index_for_d1_index(d1_index);
						}
				}
				Assert (texture_count < D1_MAX_TEXTURES);
				texture_count++;
			}

			arg = strtok (NULL, equal_space);
		}
	}
}

/* If the given d1_index is the index of a bitmap we have to load
 * (because it is unique to descent 1), then returns the d2_index that
 * the given d1_index replaces.
 * Returns bitmap_index::None if the given d1_index is not unique to descent 1.
 */
static bitmap_index d2_index_for_d1_index(short d1_index)
{
	if (!d1_tmap_nums)
		return bitmap_index::None;
	auto &t = *d1_tmap_nums.get();
	assert(d1_index < std::size(t));
	if (d1_index >= std::size(t))
		return bitmap_index::None;
	const auto i = t[d1_index];
	if (i == -1 || !d1_tmap_num_unique(i))
		return bitmap_index::None;
	return Textures[convert_d1_tmap_num(i)];
}

}

#define D1_BITMAPS_SIZE 300000
void load_d1_bitmap_replacements()
{
	int pig_data_start, bitmap_header_start, bitmap_data_start;
	int N_bitmaps;
	short d1_index;
	uint8_t * next_bitmap;
	palette_array_t d1_palette;
	char *p;

	auto &&[d1_Piggy_fp, physfserr] = PHYSFSX_openReadBuffered(descent_pig_basename);
#define D1_PIG_LOAD_FAILED "Failed loading " D1_PIGFILE
	if (!d1_Piggy_fp) {
		Warning("Failed to open " D1_PIGFILE ": %s", PHYSFS_getErrorByCode(physfserr));
		return;
	}

	//first, free up data allocated for old bitmaps
	free_bitmap_replacements();

	std::array<color_palette_index, 256> colormap;
	if (get_d1_colormap( d1_palette, colormap ) != 0)
		Warning_puts("Failed to load Descent 1 color palette");

	switch (descent1_pig_size{PHYSFS_fileLength(d1_Piggy_fp)})
	{
		case descent1_pig_size::d1_share_big_pigsize:
		case descent1_pig_size::d1_share_10_pigsize:
		case descent1_pig_size::d1_share_pigsize:
		case descent1_pig_size::d1_10_big_pigsize:
		case descent1_pig_size::d1_10_pigsize:
		pig_data_start = 0;
		// OK, now we need to read d1_tmap_nums by emulating d1's gamedata_read_tbl()
		read_d1_tmap_nums_from_hog(d1_Piggy_fp);
		break;
	default:
		Warning_puts("Unknown size for " D1_PIGFILE);
		Int3();
		[[fallthrough]];
		case descent1_pig_size::d1_pigsize:
		case descent1_pig_size::d1_oem_pigsize:
		case descent1_pig_size::d1_mac_pigsize:
		case descent1_pig_size::d1_mac_share_pigsize:
		pig_data_start = PHYSFSX_readInt(d1_Piggy_fp );
		bm_read_d1_tmap_nums(d1_Piggy_fp); //was: bm_read_all_d1(fp);
		//for (i = 0; i < 1800; i++) GameBitmapXlat[i] = PHYSFSX_readShort(d1_Piggy_fp);
		break;
	}

	PHYSFS_seek(d1_Piggy_fp, pig_data_start);
	N_bitmaps = PHYSFSX_readInt(d1_Piggy_fp);
	{
		int N_sounds = PHYSFSX_readInt(d1_Piggy_fp);
		int header_size = N_bitmaps * DISKBITMAPHEADER_D1_SIZE
			+ N_sounds * sizeof(DiskSoundHeader);
		bitmap_header_start = pig_data_start + 2 * sizeof(int);
		bitmap_data_start = bitmap_header_start + header_size;
	}

	Bitmap_replacement_data = std::make_unique<ubyte[]>(D1_BITMAPS_SIZE);
	if (!Bitmap_replacement_data) {
		Warning_puts(D1_PIG_LOAD_FAILED);
		return;
	}

	next_bitmap = Bitmap_replacement_data.get();

	for (d1_index = 1; d1_index <= N_bitmaps; d1_index++ ) {
		const auto d2_index = d2_index_for_d1_index(d1_index);
		// only change bitmaps which are unique to d1
		if (d2_index != bitmap_index::None)
		{
			PHYSFS_seek(d1_Piggy_fp, bitmap_header_start + (d1_index - 1) * DISKBITMAPHEADER_D1_SIZE);
			const auto bmh{DiskBitmapHeader_d1_read(d1_Piggy_fp)};

			bitmap_read_d1( &GameBitmaps[d2_index], d1_Piggy_fp, bitmap_data_start, &bmh, &next_bitmap, d1_palette, colormap );
			Assert(next_bitmap - Bitmap_replacement_data.get() < D1_BITMAPS_SIZE);
			GameBitmapOffset[d2_index] = pig_bitmap_offset::None; // don't try to read bitmap from current d2 pigfile
			GameBitmapFlags[d2_index] = bmh.flags;

			auto &abname = AllBitmaps[d2_index].name;
			if ((p = strchr(abname.data(), '#')) /* d2 BM is animated */
			     && !(bmh.dflags & DBM_FLAG_ABM) ) { /* d1 bitmap is not animated */
				int len = p - abname.data();
				const auto num_bitmap_files = Num_bitmap_files;
				for (const uint16_t i : xrange(num_bitmap_files))
				{
					const bitmap_index bi{i};
					if (bi != d2_index && ! memcmp(abname.data(), AllBitmaps[bi].name.data(), len))
					{
						gr_set_bitmap_data(GameBitmaps[bi], nullptr);	// free ogl texture
						GameBitmaps[bi] = GameBitmaps[d2_index];
						GameBitmapOffset[bi] = pig_bitmap_offset::None;
						GameBitmapFlags[bi] = bmh.flags;
					}
				}
			}
		}
	}
	last_palette_loaded_pig[0]= 0;  //force pig re-load

	texmerge_flush();       //for re-merging with new textures
}

/*
 * Find and load the named bitmap from descent.pig
 * similar to read_extra_bitmap_iff
 */
grs_bitmap *read_extra_bitmap_d1_pig(const std::span<const char> name, grs_bitmap &n)
{
	{
		int pig_data_start, bitmap_header_start, bitmap_data_start;
		int N_bitmaps;
		palette_array_t d1_palette;
		auto &&[d1_Piggy_fp, physfserr] = PHYSFSX_openReadBuffered(descent_pig_basename);
		if (!d1_Piggy_fp)
		{
			Warning("Failed to open " D1_PIGFILE ": %s", PHYSFS_getErrorByCode(physfserr));
			return nullptr;
		}

		std::array<color_palette_index, 256> colormap;
		if (get_d1_colormap( d1_palette, colormap ) != 0)
			Warning_puts("Failed to load Descent 1 color palette");

		switch (descent1_pig_size{PHYSFS_fileLength(d1_Piggy_fp)})
		{
			case descent1_pig_size::d1_share_big_pigsize:
			case descent1_pig_size::d1_share_10_pigsize:
			case descent1_pig_size::d1_share_pigsize:
			case descent1_pig_size::d1_10_big_pigsize:
			case descent1_pig_size::d1_10_pigsize:
			pig_data_start = 0;
			break;
		default:
			Warning_puts("Unknown size for " D1_PIGFILE);
			Int3();
			[[fallthrough]];
			case descent1_pig_size::d1_pigsize:
			case descent1_pig_size::d1_oem_pigsize:
			case descent1_pig_size::d1_mac_pigsize:
			case descent1_pig_size::d1_mac_share_pigsize:
			pig_data_start = PHYSFSX_readInt(d1_Piggy_fp );

			break;
		}

		PHYSFS_seek(d1_Piggy_fp, pig_data_start);
		N_bitmaps = PHYSFSX_readInt(d1_Piggy_fp);
		{
			int N_sounds = PHYSFSX_readInt(d1_Piggy_fp);
			int header_size = N_bitmaps * DISKBITMAPHEADER_D1_SIZE
				+ N_sounds * sizeof(DiskSoundHeader);
			bitmap_header_start = pig_data_start + 2 * sizeof(int);
			bitmap_data_start = bitmap_header_start + header_size;
		}

		for (unsigned i = 1;; ++i)
		{
			if (i > N_bitmaps)
			{
				con_printf(CON_DEBUG, "Failed to find bitmap: %s", name.data());
				return nullptr;
			}
			const auto bmh{DiskBitmapHeader_d1_read(d1_Piggy_fp)};
			if (!d_strnicmp(bmh.name, name.data(), std::min<std::size_t>(8u, name.size())))
			{
				bitmap_read_d1(&n, d1_Piggy_fp, bitmap_data_start, &bmh, 0, d1_palette, colormap);
				break;
			}
		}
	}

#if !DXX_USE_OGL
	n.avg_color = 0;	//compute_average_pixel(n);
#endif
	return &n;
}
#endif

/*
 * reads a bitmap_index structure from a PHYSFS_File
 */
void bitmap_index_read(const NamedPHYSFS_File fp, bitmap_index &bi)
{
	const auto i = GameBitmaps.valid_index(PHYSFSX_readShort(fp));
	bi = i ? *i : bitmap_index::None;
}

/*
 * reads n bitmap_index structs from a PHYSFS_File
 */
void bitmap_index_read_n(const NamedPHYSFS_File fp, const std::ranges::subrange<bitmap_index *> r)
{
	for (auto &bi : r)
	{
		const auto i = GameBitmaps.valid_index(PHYSFSX_readShort(fp));
		bi = i ? *i : bitmap_index::None;
	}
}

}
