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
 * Routines to parse bitmaps.tbl
 * Only used for editor, since the registered version of descent 1.
 *
 */

#include "dxxsconf.h"
#include <algorithm>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>

#include "pstypes.h"
#include "gr.h"
#include "bm.h"
#include "u_mem.h"
#include "dxxerror.h"
#include "object.h"
#include "physfsx.h"
#include "vclip.h"
#include "effects.h"
#include "polyobj.h"
#include "wall.h"
#include "textures.h"
#include "game.h"
#include "iff.h"
#include "hostage.h"
#include "powerup.h"
#include "sounds.h"
#include "piggy.h"
#include "aistruct.h"
#include "ai.h"
#include "weapon.h"
#include "gauges.h"
#include "player.h"
#include "endlevel.h"
#include "cntrlcen.h"
#include "physfs-serial.h"
#include "args.h"
#include "text.h"
#if defined(DXX_BUILD_DESCENT_I)
#include "fuelcen.h"
#elif defined(DXX_BUILD_DESCENT_II)
#include "gamepal.h"
#include "interp.h"
#endif
#include "strutil.h"
#if DXX_USE_EDITOR
#include "editor/texpage.h"
#endif

#include "compiler-cf_assert.h"
#include "compiler-range_for.h"
#include "partial_range.h"
#include "d_enumerate.h"
#include "d_range.h"
#include "d_zip.h"

using std::min;

namespace dcx {

namespace {

enum class bm_type : uint8_t
{
	none = 0xff,
	cockpit = 0,
	textures = 2,
	vclip = 4,
	effects = 5,
	eclip = 6,
	wall_anims = 12,
	wclip = 13,
	robot = 14,
	gauges = 20,
	/* if DXX_BUILD_DESCENT_II */
	gauges_hires = 21,
	/* endif */
};

static bm_type current_bm_type = bm_type::none;

uint8_t wall_explodes_flag;
uint8_t wall_blastable_flag;
uint8_t wall_hidden_flag;
uint8_t tmap1_flag;		//flag if this is used as tmap_num (not tmap_num2)

struct get_texture_result
{
	std::size_t texture_count;
	std::size_t idx_updated_texture;
	bool reuse_texture;
};

// remove extension from filename, doesn't work with paths.
[[nodiscard]]
static std::array<char, 20> removeext(const char *const filename)
{
	auto i = filename;
	auto p = i;
	for (; const char c = *i; ++i)
	{
		if (c == '.')
			p = i;
		/* No break - find the last '.', not the first. */
	}
	const std::size_t rawlen = p - filename;
	std::array<char, 20> out{};
	const std::size_t copy_len = rawlen < out.size() ? rawlen : 0;
	memcpy(out.data(), filename, copy_len);
	return out;
}

}

}

namespace dsx {
namespace {
#if defined(DXX_BUILD_DESCENT_I)
static short		N_ObjBitmaps=0;
#endif
}

#if DXX_USE_EDITOR
powerup_names_array Powerup_names;
robot_names_array Robot_names;
#endif
}

namespace dcx {
namespace {
static short		N_ObjBitmapPtrs;
static int			Num_robot_ais;
//---------------- Internal variables ---------------------------
static int			SuperX = -1;
static int			Installed=0;
static short 		tmap_count = 0;
}
}
namespace dsx {
namespace {
#if defined(DXX_BUILD_DESCENT_I)
static unsigned		clip_count;
static unsigned		clip_num;
static uint16_t		frames;
#elif defined(DXX_BUILD_DESCENT_II)
static char 		*arg;
static short 		clip_count = 0;
static short 		clip_num;
static short 		frames;
static char 		*dest_bm;		//clip number to play when destroyed
#endif
}
}
namespace dcx {
namespace {
static short 		sound_num;
static float 		play_time;
static int			hit_sound = -1;
static int 			abm_flag = 0;
static int 			rod_flag = 0;
static short		wall_open_sound, wall_close_sound;
static float		vlighting=0;
static int			obj_eclip;
static vclip_index	dest_vclip;		//what vclip to play when exploding
static int			dest_eclip;		//what eclip to play when exploding
static fix			dest_size;		//3d size of explosion
static int			crit_clip;		//clip number to play when destroyed
static int			crit_flag;		//flag if this is a destroyed eclip
static int			num_sounds=0;

static int linenum;		//line int table currently being parsed
}
}

//------------------- Useful macros and variables ---------------

#define IFTOK(str) if (!strcmp(arg, str))

namespace dsx {
namespace {
//	For the sake of LINT, defining prototypes to module's functions
#if defined(DXX_BUILD_DESCENT_I)
static void bm_read_sound(char *&arg, int skip, int pc_shareware);
static void bm_read_robot_ai(d_robot_info_array &Robot_info, char *&arg, int skip);
static void bm_read_robot(d_level_shared_robot_info_state &LevelSharedRobotInfoState, char *&arg, int skip);
static void bm_read_object(char *&arg, int skip);
static void bm_read_player_ship(char *&arg, int skip);
static std::size_t bm_read_some_file(d_vclip_array &Vclip, std::size_t texture_count, const std::string &dest_bm, char *&arg, int skip);
static void bm_read_weapon(char *&arg, int skip, int unused_flag);
static void bm_read_powerup(char *&arg, int unused_flag);
static void bm_read_hostage(char *&arg);
static void verify_textures();
#elif defined(DXX_BUILD_DESCENT_II)
#if DXX_USE_EDITOR
static void bm_read_alias(void);
#endif
static void bm_read_marker(void);
static void bm_read_robot_ai(d_robot_info_array &Robot_info, int skip);
static void bm_read_powerup(int unused_flag);
static void bm_read_hostage(void);
static void bm_read_robot(d_level_shared_robot_info_state &LevelSharedRobotInfoState, int skip);
static void bm_read_weapon(int skip, int unused_flag);
static void bm_read_reactor(void);
static void bm_read_exitmodel(void);
static void bm_read_player_ship(void);
static std::size_t bm_read_some_file(d_vclip_array &Vclip, std::size_t texture_count, int skip);
static void bm_read_sound(int skip);
static void clear_to_end_of_line(void);
static void verify_textures(void);
#endif
}

//---------------------------------------------------------------
int compute_average_pixel(grs_bitmap *n)
{
	int	total_red, total_green, total_blue;

#if defined(DXX_BUILD_DESCENT_II)
	auto pptr = n->bm_data;
#endif
	const unsigned bm_h = n->bm_h, bm_w = n->bm_w;

	total_red = 0;
	total_green = 0;
	total_blue = 0;

	const auto product = (bm_h * bm_w);
#if defined(DXX_BUILD_DESCENT_I)
	for (unsigned row = 0; row < bm_h; row++)
		for (unsigned column = 0; column < bm_w; column++)
#elif defined(DXX_BUILD_DESCENT_II)
	for (auto counter = product; counter--;)
#endif
	{
#if defined(DXX_BUILD_DESCENT_I)
		const auto color = gr_gpixel (*n, column, row);
		const auto &p = gr_palette[color];
#elif defined(DXX_BUILD_DESCENT_II)
		const auto &p = gr_palette[*pptr++];
#endif
		total_red += p.r;
		total_green += p.g;
		total_blue += p.b;
		}

	total_red /= product;
	total_green /= product;
	total_blue /= product;

	return BM_XRGB(total_red/2, total_green/2, total_blue/2);
}

namespace {

//---------------------------------------------------------------
// Loads a bitmap from either the piggy file, a r64 file, or a
// whatever extension is passed.

static bitmap_index bm_load_sub(const int skip, const char *const filename)
{
	palette_array_t newpal;

	if (skip) {
		return bitmap_index{};
	}

#if defined(DXX_BUILD_DESCENT_I)
	const auto &&fname = removeext(filename);
#elif defined(DXX_BUILD_DESCENT_II)
	std::array<char, 20> fname{};
	const auto path = d_splitpath(filename);
	if (path.base_end - path.base_start >= fname.size())
		Error("File <%s> - bitmap error, filename too long", filename);
	memcpy(fname.data(), path.base_start, path.base_end - path.base_start);
#endif

	if (const auto bitmap_num = piggy_find_bitmap(fname); bitmap_num != bitmap_index{})
	{
		return bitmap_num;
	}

	grs_bitmap n;
	if (const auto iff_error{iff_read_bitmap(filename, n, &newpal)}; iff_error != iff_status_code::no_error)
	{
		Error("File <%s> - IFF error: %s, line %d",filename,iff_errormsg(iff_error),linenum);
	}

	gr_remap_bitmap_good(n, newpal, iff_has_transparency ? iff_transparent_color : -1, SuperX);

#if !DXX_USE_OGL
	n.avg_color = compute_average_pixel(&n);
#endif
	return piggy_register_bitmap(n, fname, 0);
}

static void ab_load(int skip, const char * filename, std::array<bitmap_index, MAX_BITMAPS_PER_BRUSH> &bmp, unsigned *nframes )
{
	if (skip) {
		Assert( bogus_bitmap_initialized != 0 );
#if defined(DXX_BUILD_DESCENT_I)
		bmp[0] = piggy_register_bitmap(bogus_bitmap, "bogus", 0);
#elif defined(DXX_BUILD_DESCENT_II)
		bmp[0] = {};		//index of bogus bitmap==0 (I think)		//&bogus_bitmap;
#endif
		*nframes = 1;
		return;
	}


#if defined(DXX_BUILD_DESCENT_I)
	const auto &&fname = removeext(filename);
#elif defined(DXX_BUILD_DESCENT_II)
	const auto path = d_splitpath(filename);
#endif

	{
	unsigned i;
	for (i=0; i<MAX_BITMAPS_PER_BRUSH; i++ )	{
		std::array<char, 24> tempname;
#if defined(DXX_BUILD_DESCENT_I)
		const auto len = snprintf(tempname.data(), tempname.size(), "%.16s#%d", fname.data(), i);
#elif defined(DXX_BUILD_DESCENT_II)
		const auto len = snprintf(tempname.data(), tempname.size(), "%.*s#%d", DXX_ptrdiff_cast_int(path.base_end - path.base_start), path.base_start, i);
#endif
		const auto bi = piggy_find_bitmap(std::span<const char>(tempname.data(), len));
		if (bi == bitmap_index{})
			break;
		bmp[i] = bi;
	}

	if (i) {
		*nframes = i;
		return;
	}
	}

	auto read_result{iff_read_animbrush(filename)};
	auto &bm = read_result.bm;
	auto &newpal = read_result.palette;
	*nframes = read_result.n_bitmaps;
	if (const auto iff_error{read_result.status}; iff_error != iff_status_code::no_error)
	{
		Error("File <%s> - IFF error: %s, line %d",filename,iff_errormsg(iff_error),linenum);
	}

	const auto nf = *nframes;
#if defined(DXX_BUILD_DESCENT_I)
	if (nf >= bm.size())
		return;
#endif
	range_for (const uint_fast32_t i, xrange(nf))
	{
		std::array<char, 24> tempname;
		cf_assert(i < bm.size());
#if defined(DXX_BUILD_DESCENT_I)
		snprintf(tempname.data(), tempname.size(), "%s#%" PRIuFAST32, fname.data(), i);
#elif defined(DXX_BUILD_DESCENT_II)
		snprintf(tempname.data(), tempname.size(), "%.*s#%" PRIuFAST32, DXX_ptrdiff_cast_int(path.base_end - path.base_start), path.base_start, i );
#endif
		gr_remap_bitmap_good(*bm[i].get(), newpal, iff_has_transparency ? iff_transparent_color : -1, SuperX);
#if !DXX_USE_OGL
		bm[i]->avg_color = compute_average_pixel(bm[i].get());
#endif
		bmp[i] = piggy_register_bitmap(*bm[i].get(), tempname, 0);
	}
}

}

int ds_load(int skip, const char * filename )	{
	char rawname[100];

	if (skip) {
		// We tell piggy_register_sound it's in the pig file, when in actual fact it's in no file
		// This just tells piggy_close not to attempt to free it
		return piggy_register_sound(bogus_sound, "bogus");
	}

	const auto &&fname = removeext(filename);
#if defined(DXX_BUILD_DESCENT_I)
	snprintf(rawname, sizeof(rawname), "Sounds/%s.raw", fname.data());
#elif defined(DXX_BUILD_DESCENT_II)
	snprintf(rawname, sizeof(rawname), "Sounds/%s.r%s", fname.data(), (GameArg.SndDigiSampleRate == sound_sample_rate::_22k) ? "22" : "aw");
#endif

	const auto i = piggy_find_sound(fname);
	if (i!=255)	{
		return i;
	}
	if (auto cfp{PHYSFSX_openReadBuffered_updateCase(rawname).first})
	{
		digi_sound n;
		n.length	= PHYSFS_fileLength( cfp );
		n.data = digi_sound::allocated_data{std::make_unique<uint8_t[]>(n.length), game_sound_offset{}};
		PHYSFSX_readBytes(cfp, n.data.get(), n.length);
		n.freq = 11025;
		return piggy_register_sound(n, fname);
	} else {
		return 255;
	}
}
}

namespace dcx {
namespace {

//parse a float
static float get_float()
{
	char *xarg;

	xarg = strtok( NULL, space_tab );
	return atof( xarg );
}

//parse an int
static int get_int()
{
	char *xarg;

	xarg = strtok( NULL, space_tab );
	return atoi( xarg );
}

static void set_texture_fname(d_fname &fname, const char *name)
{
	fname.copy_if(name, FILENAME_LEN);
	REMOVE_DOTS(fname.data());
}

}
}

namespace dsx {
namespace {

//loads a texture and returns the texture num
static get_texture_result get_texture(d_level_unique_tmap_info_state::TmapInfo_array &TmapInfo, const int skip, const char *const name, std::size_t texture_count)
{
	d_fname short_name;
	short_name.copy_if(name, FILENAME_LEN);
	REMOVE_DOTS(short_name.data());
	const unsigned t = texture_count;
	for (const auto &&[i, ti] : enumerate(partial_range(TmapInfo, t)))
	{
		if (!d_stricmp(ti.filename, short_name))
			return {
				.texture_count = texture_count,
				.idx_updated_texture = i,
				.reuse_texture = true
			};
	}
	Textures[t] = bm_load_sub(skip, name);
	TmapInfo[t].filename.copy_if(short_name);
	++texture_count;
	NumTextures = texture_count;
	return {
		.texture_count = texture_count,
		.idx_updated_texture = t,
		.reuse_texture = false
	};
}

}

#define LINEBUF_SIZE 600

#if defined(DXX_BUILD_DESCENT_I) || (defined(DXX_BUILD_DESCENT_II) && DXX_USE_EDITOR)
//-----------------------------------------------------------------
// Initializes all properties and bitmaps from BITMAPS.TBL file.
// This is called when the editor is IN.
// If no editor, properties_read_cmp() is called.
int gamedata_read_tbl(d_level_shared_robot_info_state &LevelSharedRobotInfoState, d_vclip_array &Vclip, int pc_shareware)
{
	std::size_t texture_count{};
	auto &Effects = LevelUniqueEffectsClipState.Effects;
	auto &TmapInfo = LevelUniqueTmapInfoState.TmapInfo;
	auto &WallAnims = GameSharedState.WallAnims;
	int	have_bin_tbl;

#if defined(DXX_BUILD_DESCENT_I)
	std::string dest_bm;
	ObjType[0] = OL_PLAYER;
	ObjId[0] = {};
	Num_total_object_types = 1;
#elif defined(DXX_BUILD_DESCENT_II)
	// Open BITMAPS.TBL for reading.
	have_bin_tbl = 0;
	auto InfoFile = PHYSFSX_openReadBuffered("BITMAPS.TBL").first;
	if (!InfoFile)
	{
		InfoFile = PHYSFSX_openReadBuffered("BITMAPS.BIN").first;
		if (!InfoFile)
			return 0;	//missing BITMAPS.TBL and BITMAPS.BIN file
		have_bin_tbl = 1;
	}

#define DEFAULT_PIG_PALETTE	"groupa.256"
	gr_use_palette_table(DEFAULT_PIG_PALETTE);
	load_palette_for_pig(DEFAULT_PIG_PALETTE);		//special: tell palette code which pig is loaded
#endif

	Sounds.fill(255);
	AltSounds.fill(255);

	DXX_MAKE_VAR_UNDEFINED(TmapInfo);
	range_for (auto &ti, TmapInfo)
	{
		ti.eclip_num = eclip_none;
		ti.flags = {};
#if defined(DXX_BUILD_DESCENT_II)
		ti.slide_u = ti.slide_v = 0;
		ti.destroyed = -1;
#endif
	}

#if defined(DXX_BUILD_DESCENT_II)
	DXX_MAKE_VAR_UNDEFINED(Reactors);
	range_for (auto &i, Reactors)
		i.model_num = polygon_model_index::None;
#endif

	Num_effects = 0;
	DXX_MAKE_VAR_UNDEFINED(Effects);
	range_for (auto &ec, Effects)
	{
		//Effects[i].bm_ptr = (grs_bitmap **) -1;
		ec.changing_wall_texture = -1;
		ec.changing_object_texture = object_bitmap_index::None;
		ec.segnum = segment_none;
		ec.vc.num_frames = -1;		//another mark of being unused
	}

	for (auto &&[dying, dead] : zip(Dying_modelnums, Dead_modelnums))
		dying = dead = polygon_model_index::None;

	Num_vclips = 0;
	DXX_MAKE_VAR_UNDEFINED(Vclip);
	range_for (auto &vc, Vclip)
	{
		vc.num_frames = -1;
		vc.flags = 0;
	}

	DXX_MAKE_VAR_UNDEFINED(WallAnims);
	range_for (auto &wa, WallAnims)
		wa.num_frames = wclip_frames_none;
	Num_wall_anims = 0;

	if (Installed)
		return 1;

	Installed = 1;

#if defined(DXX_BUILD_DESCENT_I)
	// Open BITMAPS.TBL for reading.
	have_bin_tbl = 0;
	auto &&[InfoFile, physfserr] = PHYSFSX_openReadBuffered("BITMAPS.TBL");
	if (!InfoFile)
	{
		auto &&[InfoFileBin, physfserr2] = PHYSFSX_openReadBuffered("BITMAPS.BIN");
		if (!InfoFileBin)
			Error("Failed to open BITMAPS.TBL and BITMAPS.BIN: \"%s\", \"%s\"\n", PHYSFS_getErrorByCode(physfserr), PHYSFS_getErrorByCode(physfserr2));
		InfoFile = std::move(InfoFileBin);
		have_bin_tbl = 1;
	}
#endif
	linenum = 0;

	PHYSFS_seek(InfoFile, 0L);

	for (PHYSFSX_gets_line_t<LINEBUF_SIZE> inputline; PHYSFSX_fgets(inputline, InfoFile);)
	{
		const char *temp_ptr;
		int skip;

		linenum++;
		if (have_bin_tbl) {				// is this a binary tbl file
			decode_text_line (inputline);
		} else {
			for (std::size_t l; inputline[(l=strlen(inputline))-2]=='\\';)
			{
				if (!isspace(inputline[l-3])) {		//if not space before backslash...
					inputline[l-2] = ' ';				//add one
					l++;
				}
				if (!PHYSFSX_fgets(inputline, InfoFile, l - 2))
					break;
				linenum++;
			}
		}

		REMOVE_EOL(inputline);
		if (strchr(inputline, ';')!=NULL) REMOVE_COMMENTS(inputline);

		if (strlen(inputline) == LINEBUF_SIZE-1)
			Error("Possible line truncation in BITMAPS.TBL on line %d\n",linenum);

		SuperX = -1;

		if ( (temp_ptr=strstr( inputline, "superx=" )) )	{
			/* Historically, this was done with atoi, so the input
			 * source was allowed to have unconvertible characters.
			 * Accept such lines by ignoring any trailing content.
			 */
			SuperX = strtol(&temp_ptr[7], nullptr, 10);
#if defined(DXX_BUILD_DESCENT_II)
			Assert(SuperX == 254);
				//the superx color isn't kept around, so the new piggy regeneration
				//code doesn't know what it is, so it assumes that it's 254, so
				//this code requires that it be 254
#endif
		}

#if defined(DXX_BUILD_DESCENT_I)
		char *arg = strtok( inputline, space_tab );
#elif defined(DXX_BUILD_DESCENT_II)
		arg = strtok( inputline, space_tab );
#endif
		if (arg && arg[0] == '@')
		{
			arg++;
			skip = pc_shareware;
		} else
			skip = 0;

		while (arg != NULL )
			{
			// Check all possible flags and defines.
			if (*arg == '$') current_bm_type = bm_type::none; // reset to no flags as default.

			IFTOK("$COCKPIT") 			current_bm_type = bm_type::cockpit;
			else IFTOK("$GAUGES")		{current_bm_type = bm_type::gauges;   clip_count = 0;}
#if defined(DXX_BUILD_DESCENT_I)
			else IFTOK("$SOUND") 		bm_read_sound(arg, skip, pc_shareware);
#elif defined(DXX_BUILD_DESCENT_II)
			else IFTOK("$GAUGES_HIRES"){current_bm_type = bm_type::gauges_hires; clip_count = 0;}
			else IFTOK("$ALIAS")			bm_read_alias();
			else IFTOK("$SOUND") 		bm_read_sound(skip);
#endif
			else IFTOK("$DOOR_ANIMS")	current_bm_type = bm_type::wall_anims;
			else IFTOK("$WALL_ANIMS")	current_bm_type = bm_type::wall_anims;
			else IFTOK("$TEXTURES") 	current_bm_type = bm_type::textures;
			else IFTOK("$VCLIP")			{current_bm_type = bm_type::vclip;		vlighting = 0;	clip_count = 0;}
			else IFTOK("$ECLIP")
			{
				current_bm_type = bm_type::eclip;
				vlighting = 0;
				clip_count = 0;
				obj_eclip=0;
#if defined(DXX_BUILD_DESCENT_I)
				dest_bm.clear();
#elif defined(DXX_BUILD_DESCENT_II)
				dest_bm=NULL;
#endif
				dest_vclip = vclip_index::None;
				dest_eclip=eclip_none;
				dest_size=-1;
				crit_clip=-1;
				crit_flag=0;
				sound_num=sound_none;
			}
			else IFTOK("$WCLIP")
			{
				current_bm_type = bm_type::wclip;
				vlighting = 0;
				clip_count = 0;
				wall_open_sound=wall_close_sound=sound_none;
				wall_explodes_flag = 0;
				wall_blastable_flag = 0;
				tmap1_flag=0;
				wall_hidden_flag = 0;
			}

			else IFTOK("$EFFECTS")		{current_bm_type = bm_type::effects;	clip_num = 0;}

#if DXX_USE_EDITOR
			else IFTOK("!METALS_FLAG")		TextureMetals = texture_count;
			else IFTOK("!LIGHTS_FLAG")		TextureLights = texture_count;
			else IFTOK("!EFFECTS_FLAG")	TextureEffects = texture_count;
			#else
#if defined(DXX_BUILD_DESCENT_I)
			else IFTOK("!METALS_FLAG") ;
			else IFTOK("!LIGHTS_FLAG") ;
			else IFTOK("!EFFECTS_FLAG") ;
#endif
			#endif

			else IFTOK("lighting") 			TmapInfo[texture_count-1].lighting = fl2f(get_float());
			else IFTOK("damage") 			TmapInfo[texture_count-1].damage = fl2f(get_float());
			else IFTOK("volatile") 			TmapInfo[texture_count-1].flags |= tmapinfo_flag::lava;
#if defined(DXX_BUILD_DESCENT_II)
			else IFTOK("goal_blue")			TmapInfo[texture_count-1].flags |= tmapinfo_flag::goal_blue;
			else IFTOK("goal_red")			TmapInfo[texture_count-1].flags |= tmapinfo_flag::goal_red;
			else IFTOK("water")	 			TmapInfo[texture_count-1].flags |= tmapinfo_flag::water;
			else IFTOK("force_field")		TmapInfo[texture_count-1].flags |= tmapinfo_flag::force_field;
			else IFTOK("slide")	 			{TmapInfo[texture_count-1].slide_u = fl2f(get_float())>>8; TmapInfo[texture_count-1].slide_v = fl2f(get_float())>>8;}
			else IFTOK("destroyed")
			{
				const auto t = texture_count - 1;
				const auto &&r = get_texture(LevelUniqueTmapInfoState.TmapInfo, 0, strtok(nullptr, space_tab), texture_count);
				texture_count = r.texture_count;
				TmapInfo[t].destroyed = r.idx_updated_texture;
			}
#endif
			//else IFTOK("Num_effects")		Num_effects = get_int();
			else IFTOK("Num_wall_anims")	Num_wall_anims = get_int();
			else IFTOK("clip_num")			clip_num = get_int();
#if defined(DXX_BUILD_DESCENT_I)
			else IFTOK("dest_bm")
			{
				char *p = strtok( NULL, space_tab );
				if (p)
					dest_bm = p;
				else
					dest_bm.clear();
			}
#elif defined(DXX_BUILD_DESCENT_II)
			else IFTOK("dest_bm")			dest_bm = strtok( NULL, space_tab );
#endif
			else IFTOK("dest_vclip")		dest_vclip = build_vclip_index_from_untrusted(get_int());
			else IFTOK("dest_eclip")		dest_eclip = get_int();
			else IFTOK("dest_size")			dest_size = fl2f(get_float());
			else IFTOK("crit_clip")			crit_clip = get_int();
			else IFTOK("crit_flag")			crit_flag = get_int();
			else IFTOK("sound_num") 		sound_num = get_int();
			else IFTOK("frames") 			frames = get_int();
			else IFTOK("time") 				play_time = get_float();
			else IFTOK("obj_eclip")			obj_eclip = get_int();
			else IFTOK("hit_sound") 		hit_sound = get_int();
			else IFTOK("abm_flag")			abm_flag = get_int();
			else IFTOK("tmap1_flag")		tmap1_flag = get_int() ? WCF_TMAP1 : 0;
			else IFTOK("vlighting")			vlighting = get_float();
			else IFTOK("rod_flag")			rod_flag = get_int();
			else IFTOK("superx") 			get_int();
			else IFTOK("open_sound") 		wall_open_sound = get_int();
			else IFTOK("close_sound") 		wall_close_sound = get_int();
			else IFTOK("explodes")	 		wall_explodes_flag = get_int() ? WCF_EXPLODES : 0;
			else IFTOK("blastable")	 		wall_blastable_flag = get_int() ? WCF_BLASTABLE : 0;
			else IFTOK("hidden")	 		wall_hidden_flag = get_int() ? WCF_HIDDEN : 0;
#if defined(DXX_BUILD_DESCENT_I)
			else IFTOK("$ROBOT_AI") 		bm_read_robot_ai(LevelSharedRobotInfoState.Robot_info, arg, skip);

			else IFTOK("$POWERUP")			{bm_read_powerup(arg, 0);		continue;}
			else IFTOK("$POWERUP_UNUSED")	{bm_read_powerup(arg, 1);		continue;}
			else IFTOK("$HOSTAGE")			{bm_read_hostage(arg);		continue;}
			else IFTOK("$ROBOT")				{bm_read_robot(LevelSharedRobotInfoState, arg, skip);			continue;}
			else IFTOK("$WEAPON")			{bm_read_weapon(arg, skip, 0);		continue;}
			else IFTOK("$WEAPON_UNUSED")	{bm_read_weapon(arg, skip, 1);		continue;}
			else IFTOK("$OBJECT")			{bm_read_object(arg, skip);		continue;}
			else IFTOK("$PLAYER_SHIP")		{bm_read_player_ship(arg, skip);	continue;}
#elif defined(DXX_BUILD_DESCENT_II)
			else IFTOK("$ROBOT_AI") 		bm_read_robot_ai(LevelSharedRobotInfoState.Robot_info, skip);

			else IFTOK("$POWERUP")			{bm_read_powerup(0);		continue;}
			else IFTOK("$POWERUP_UNUSED")	{bm_read_powerup(1);		continue;}
			else IFTOK("$HOSTAGE")			{bm_read_hostage();		continue;}
			else IFTOK("$ROBOT")				{bm_read_robot(LevelSharedRobotInfoState, skip);			continue;}
			else IFTOK("$WEAPON")			{bm_read_weapon(skip, 0);		continue;}
			else IFTOK("$WEAPON_UNUSED")	{bm_read_weapon(skip, 1);		continue;}
			else IFTOK("$REACTOR")			{bm_read_reactor();		continue;}
			else IFTOK("$MARKER")			{bm_read_marker();		continue;}
			else IFTOK("$PLAYER_SHIP")		{bm_read_player_ship();	continue;}
			else IFTOK("$EXIT") {
				if (pc_shareware)
					bm_read_exitmodel();
				else
					clear_to_end_of_line();
				continue;
			}
#endif
			else	{		//not a special token, must be a bitmap!

				// Remove any illegal/unwanted spaces and tabs at this point.
				while ((*arg=='\t') || (*arg==' ')) arg++;
				if (*arg == '\0') { break; }

#if defined(DXX_BUILD_DESCENT_II)
				//check for '=' in token, indicating error
				if (strchr(arg,'='))
					Error("Unknown token <'%s'> on line %d of BITMAPS.TBL",arg,linenum);
#endif

				// Otherwise, 'arg' is apparently a bitmap filename.
				// Load bitmap and process it below:
#if defined(DXX_BUILD_DESCENT_I)
				texture_count = bm_read_some_file(Vclip, texture_count, dest_bm, arg, skip);
#elif defined(DXX_BUILD_DESCENT_II)
				texture_count = bm_read_some_file(Vclip, texture_count, skip);
#endif

			}

			arg = strtok( NULL, equal_space );
			continue;
      }
	}

	NumTextures = texture_count;
	LevelUniqueTmapInfoState.Num_tmaps = tmap_count;

#if defined(DXX_BUILD_DESCENT_II)
	Textures[NumTextures++] = {};		//entry for bogus tmap
	InfoFile.reset();
#endif
	assert(LevelSharedRobotInfoState.N_robot_types == Num_robot_ais);		//should be one ai info per robot

	verify_textures();

	//check for refereced but unused clip count
	for (auto &&[idx, e] : enumerate(Effects))
	{
		if ((e.changing_wall_texture != -1 || e.changing_object_texture != object_bitmap_index::None) && e.vc.num_frames == ~0u)
			Error("EClip %" DXX_PRI_size_type " referenced (by polygon object?), but not defined", idx);
	}

#if defined(DXX_BUILD_DESCENT_II)
	#ifndef NDEBUG
	{
		//make sure all alt sounds refer to valid main sounds
		for (unsigned i = 0; i < num_sounds; ++i)
		{
			int alt = AltSounds[i];
			Assert(alt==0 || alt==-1 || Sounds[alt]!=255);
		}
	}
	#endif

	gr_use_palette_table(D2_DEFAULT_PALETTE);
#endif

	return 0;
}
#endif

namespace {

void verify_textures()
{
#if defined(DXX_BUILD_DESCENT_II)
	auto &Effects = LevelUniqueEffectsClipState.Effects;
#endif
	grs_bitmap * bmp;
	int j;
	j=0;
	const auto Num_tmaps = LevelUniqueTmapInfoState.Num_tmaps;
	for (uint_fast32_t i = 0; i < Num_tmaps; ++i)
	{
		bmp = &GameBitmaps[Textures[i]];
		if ( (bmp->bm_w!=64)||(bmp->bm_h!=64)||(bmp->bm_rowsize!=64) )	{
			j++;
		}
	}
	if (j)
		Error("%d textures were not 64x64.",j);

#if defined(DXX_BUILD_DESCENT_II)
	for (uint_fast32_t i = 0; i < Num_effects; ++i)
		if (const auto changing_object_texture = Effects[i].changing_object_texture; changing_object_texture != object_bitmap_index::None)
		{
			const auto o = ObjBitmaps[changing_object_texture];
			auto &gbo = GameBitmaps[o];
			if (gbo.bm_w != 64 || gbo.bm_h != 64)
				Error("Effect %" PRIuFAST32 " is used on object, but is not 64x64",i);
		}
#endif
}

#if defined(DXX_BUILD_DESCENT_II) && DXX_USE_EDITOR
void bm_read_alias()
{
	Assert(Num_aliases < MAX_ALIASES);
	auto &a = alias_list[Num_aliases];
	a = {};

	for (const auto b : {&a.alias_name, &a.file_name})
	{
		const auto t = strtok(nullptr, space_tab);
		if (!t)
			return;
		const auto c = std::min(strlen(t), std::size(*b) - 1);
		std::memcpy(std::data(*b), t, c);
	}
	Num_aliases++;
}
#endif

}

}

namespace {

static void set_lighting_flag(grs_bitmap &bmp)
{
	bmp.set_flag_mask(vlighting < 0, BM_FLAG_NO_LIGHTING);
}

}

namespace dsx {
namespace {

static void set_texture_name(d_level_unique_tmap_info_state::TmapInfo_array &TmapInfo, const unsigned texture_idx, const char *name)
{
	set_texture_fname(TmapInfo[texture_idx].filename, name);
}

#if defined(DXX_BUILD_DESCENT_I)
static std::size_t bm_read_eclip(std::size_t texture_count, const std::string &dest_bm, const char *const arg, int skip)
#elif defined(DXX_BUILD_DESCENT_II)
static std::size_t bm_read_eclip(std::size_t texture_count, int skip)
#endif
{
	auto &Effects = LevelUniqueEffectsClipState.Effects;
	auto &TmapInfo = LevelUniqueTmapInfoState.TmapInfo;

	assert(clip_num < Effects.size());

	if (clip_num+1 > Num_effects)
		Num_effects = clip_num+1;

	Effects[clip_num].flags = 0;

#if defined(DXX_BUILD_DESCENT_II)
	unsigned dest_bm_num = 0;
	//load the dest bitmap first, so that after this routine, the last-loaded
	//texture will be the monitor, so that lighting parameter will be applied
	//to the correct texture
	if (dest_bm) {			//deal with bitmap for blown up clip
		const auto &&r = get_texture(LevelUniqueTmapInfoState.TmapInfo, skip, dest_bm, texture_count);
		texture_count = r.texture_count;
		if (!r.reuse_texture)
		{
		}
		else if (auto &t = Textures[r.idx_updated_texture]; t == bitmap_index{})		//was found, but registered out
			t = bm_load_sub(skip, dest_bm);
		dest_bm_num = r.idx_updated_texture;
	}
#endif

	if (!abm_flag)
	{
		const auto bitmap = bm_load_sub(skip, arg);

		Effects[clip_num].vc.play_time = fl2f(play_time);
		Effects[clip_num].vc.num_frames = frames;
		Effects[clip_num].vc.frame_time = fl2f(play_time)/frames;

		Assert(clip_count < frames);
		Effects[clip_num].vc.frames[clip_count] = bitmap;
		set_lighting_flag(GameBitmaps[bitmap]);

		Assert(!obj_eclip);		//obj eclips for non-abm files not supported!
		Assert(crit_flag==0);

		if (clip_count == 0) {
			Effects[clip_num].changing_wall_texture = texture_count;
			Assert(tmap_count < MAX_TEXTURES);
	  		tmap_count++;
			Textures[texture_count] = bitmap;
			set_texture_name(LevelUniqueTmapInfoState.TmapInfo, texture_count, arg);
			Assert(texture_count < MAX_TEXTURES);
			texture_count++;
			TmapInfo[texture_count].eclip_num = clip_num;
			NumTextures = texture_count;
		}

		clip_count++;

	} else {
		std::array<bitmap_index, MAX_BITMAPS_PER_BRUSH> bm;
		abm_flag = 0;

		ab_load(skip, arg, bm, &Effects[clip_num].vc.num_frames );

		Effects[clip_num].vc.play_time = fl2f(play_time);
		Effects[clip_num].vc.frame_time = Effects[clip_num].vc.play_time/Effects[clip_num].vc.num_frames;

		clip_count = 0;
		set_lighting_flag(GameBitmaps[bm[clip_count]]);
		Effects[clip_num].vc.frames[clip_count] = bm[clip_count];

		if (!obj_eclip && !crit_flag) {
			Effects[clip_num].changing_wall_texture = texture_count;
			Assert(tmap_count < MAX_TEXTURES);
  			tmap_count++;
			Textures[texture_count] = bm[clip_count];
			set_texture_name(LevelUniqueTmapInfoState.TmapInfo, texture_count, arg);
			Assert(texture_count < MAX_TEXTURES);
			TmapInfo[texture_count].eclip_num = clip_num;
			texture_count++;
			NumTextures = texture_count;
		}

		if (obj_eclip) {

			if (Effects[clip_num].changing_object_texture == object_bitmap_index::None)
			{		//first time referenced
				Effects[clip_num].changing_object_texture = static_cast<object_bitmap_index>(N_ObjBitmaps);		// XChange ObjectBitmaps
				N_ObjBitmaps++;
			}

			ObjBitmaps[Effects[clip_num].changing_object_texture] = Effects[clip_num].vc.frames[0];
		}

		//if for an object, Effects_bm_ptrs set in object load

		for(clip_count=1;clip_count < Effects[clip_num].vc.num_frames; clip_count++) {
			set_lighting_flag(GameBitmaps[bm[clip_count]]);
			Effects[clip_num].vc.frames[clip_count] = bm[clip_count];
		}

	}

	Effects[clip_num].crit_clip = crit_clip;
	Effects[clip_num].sound_num = sound_num;

#if defined(DXX_BUILD_DESCENT_I)
	if (!dest_bm.empty())
#elif defined(DXX_BUILD_DESCENT_II)
	if (dest_bm)
#endif
	{			//deal with bitmap for blown up clip
#if defined(DXX_BUILD_DESCENT_I)
		{
			const auto &&r = get_texture(LevelUniqueTmapInfoState.TmapInfo, skip, dest_bm.c_str(), texture_count);
			texture_count = r.texture_count;
			Effects[clip_num].dest_bm_num = r.idx_updated_texture;
		}
#elif defined(DXX_BUILD_DESCENT_II)
		Effects[clip_num].dest_bm_num = dest_bm_num;
#endif

		if (dest_vclip == vclip_index::None)
			Error("Destruction vclip missing on line %d", linenum);
		if (dest_size==-1)
			Error("Desctuction vclip missing on line %d",linenum);

		Effects[clip_num].dest_vclip = dest_vclip;
		Effects[clip_num].dest_size = dest_size;

		Effects[clip_num].dest_eclip = dest_eclip;
	}
	else {
		Effects[clip_num].dest_bm_num = ~0u;
		Effects[clip_num].dest_eclip = eclip_none;
	}

	if (crit_flag)
		Effects[clip_num].flags |= EF_CRITICAL;
	return texture_count;
}

#if defined(DXX_BUILD_DESCENT_I)
static void bm_read_gauges(const char *const arg, int skip)
#elif defined(DXX_BUILD_DESCENT_II)
static void bm_read_gauges(int skip)
#endif
{
	unsigned i, num_abm_frames;

	if (!abm_flag)	{
		const auto bitmap = bm_load_sub(skip, arg);
		Assert(clip_count < MAX_GAUGE_BMS);
		Gauges[clip_count] = bitmap;
		clip_count++;
	} else {
		std::array<bitmap_index, MAX_BITMAPS_PER_BRUSH> bm;
		abm_flag = 0;
		ab_load(skip, arg, bm, &num_abm_frames );
		for (i=clip_count; i<clip_count+num_abm_frames; i++) {
			Assert(i < MAX_GAUGE_BMS);
			Gauges[i] = bm[i-clip_count];
		}
		clip_count += num_abm_frames;
	}
}

#if defined(DXX_BUILD_DESCENT_I)
static std::size_t bm_read_wclip(std::size_t texture_count, char *const arg, int skip)
#elif defined(DXX_BUILD_DESCENT_II)
static void bm_read_gauges_hires()
{
	unsigned i, num_abm_frames;

	if (!abm_flag)	{
		const auto bitmap = bm_load_sub(0, arg);
		Assert(clip_count < MAX_GAUGE_BMS);
		Gauges_hires[clip_count] = bitmap;
		clip_count++;
	} else {
		std::array<bitmap_index, MAX_BITMAPS_PER_BRUSH> bm;
		abm_flag = 0;
		ab_load(0, arg, bm, &num_abm_frames );
		for (i=clip_count; i<clip_count+num_abm_frames; i++) {
			Assert(i < MAX_GAUGE_BMS);
			Gauges_hires[i] = bm[i-clip_count];
		}
		clip_count += num_abm_frames;
	}
}

static std::size_t bm_read_wclip(std::size_t texture_count, int skip)
#endif
{
	auto &TmapInfo = LevelUniqueTmapInfoState.TmapInfo;
	auto &WallAnims = GameSharedState.WallAnims;
	Assert(clip_num < MAX_WALL_ANIMS);

	auto &wa = WallAnims[clip_num];
	wa.flags = wall_explodes_flag | wall_blastable_flag | wall_hidden_flag | tmap1_flag;

	if (!abm_flag)	{
		const auto bitmap = bm_load_sub(skip, arg);
		if (wa.num_frames != wclip_frames_none && clip_count == 0)
			Error( "Wall Clip %d is already used!", clip_num );
		wa.play_time = fl2f(play_time);
		wa.num_frames = frames;
		//WallAnims[clip_num].frame_time = fl2f(play_time)/frames;
		Assert(clip_count < frames);
		wa.frames[clip_count++] = texture_count;
		wa.open_sound = wall_open_sound;
		wa.close_sound = wall_close_sound;
		Textures[texture_count] = bitmap;
		set_lighting_flag(GameBitmaps[bitmap]);
		set_texture_name(LevelUniqueTmapInfoState.TmapInfo, texture_count, arg);
		Assert(texture_count < MAX_TEXTURES);
		texture_count++;
		NumTextures = texture_count;
		if (clip_num >= Num_wall_anims) Num_wall_anims = clip_num+1;
	} else {
		std::array<bitmap_index, MAX_BITMAPS_PER_BRUSH> bm;
		unsigned nframes;
		if (wa.num_frames != wclip_frames_none)
			Error( "AB_Wall clip %d is already used!", clip_num );
		abm_flag = 0;
#if defined(DXX_BUILD_DESCENT_I)
		ab_load(skip, arg, bm, &nframes );
#elif defined(DXX_BUILD_DESCENT_II)
		ab_load(0, arg, bm, &nframes );
#endif
		wa.num_frames = nframes;
		wa.play_time = fl2f(play_time);
		//WallAnims[clip_num].frame_time = fl2f(play_time)/nframes;
		wa.open_sound = wall_open_sound;
		wa.close_sound = wall_close_sound;
		strcpy(&wa.filename[0], arg);
		REMOVE_DOTS(&wa.filename[0]);

		if (clip_num >= Num_wall_anims) Num_wall_anims = clip_num+1;

		set_lighting_flag(GameBitmaps[bm[clip_count]]);

		for (clip_count=0;clip_count < wa.num_frames; clip_count++)	{
			Textures[texture_count] = bm[clip_count];
			set_lighting_flag(GameBitmaps[bm[clip_count]]);
			wa.frames[clip_count] = texture_count;
			REMOVE_DOTS(arg);
			snprintf(&TmapInfo[texture_count].filename[0u], TmapInfo[texture_count].filename.size(), "%s#%d", arg, clip_count);
			Assert(texture_count < MAX_TEXTURES);
			texture_count++;
			NumTextures = texture_count;
		}
	}
	return texture_count;
}

#if defined(DXX_BUILD_DESCENT_I)
static void bm_read_vclip(d_vclip_array &Vclip, const char *const arg, int skip)
#elif defined(DXX_BUILD_DESCENT_II)
static void bm_read_vclip(d_vclip_array &Vclip, int skip)
#endif
{
	const std::size_t c = clip_num;
	assert(Vclip.valid_index(c));
	if (!Vclip.valid_index(c))
		return;
	const vclip_index vci{static_cast<uint8_t>(c)};
	auto &vc = Vclip[vci];

#if defined(DXX_BUILD_DESCENT_II)
	if (Num_vclips <= c)
		Num_vclips = c + 1;
#endif

	if (!abm_flag)	{
		if (vc.num_frames != ~0u && clip_count == 0)
			Error( "Vclip %d is already used!", clip_num );
		const auto bi = bm_load_sub(skip, arg);
		vc.play_time = fl2f(play_time);
		vc.num_frames = frames;
		vc.frame_time = fl2f(play_time) / frames;
		vc.light_value = fl2f(vlighting);
		vc.sound_num = sound_num;
		set_lighting_flag(GameBitmaps[bi]);
		Assert(clip_count < frames);
		vc.frames[clip_count++] = bi;
		if (rod_flag) {
			rod_flag=0;
			vc.flags |= VF_ROD;
		}

	} else	{
		std::array<bitmap_index, MAX_BITMAPS_PER_BRUSH> bm;
		abm_flag = 0;
		if (vc.num_frames != ~0u)
			Error( "AB_Vclip %d is already used!", clip_num );
		ab_load(skip, arg, bm, &vc.num_frames);

		if (rod_flag) {
			//int i;
			rod_flag=0;
			vc.flags |= VF_ROD;
		}
		vc.play_time = fl2f(play_time);
		vc.frame_time = fl2f(play_time) / vc.num_frames;
		vc.light_value = fl2f(vlighting);
		vc.sound_num = sound_num;
		set_lighting_flag(GameBitmaps[bm[clip_count]]);

		for (clip_count = 0; clip_count < vc.num_frames; ++clip_count)
		{
			set_lighting_flag(GameBitmaps[bm[clip_count]]);
			vc.frames[clip_count] = bm[clip_count];
		}
	}
}

}

}

namespace dcx {
namespace {

// ------------------------------------------------------------------------------
static void get4fix(enumerated_array<fix, NDL, Difficulty_level_type> &fixp)
{
	char	*curtext;
	range_for (auto &i, fixp)
	{
		curtext = strtok(NULL, space_tab);
		i = fl2f(atof(curtext));
	}
}

// ------------------------------------------------------------------------------
static void get4byte(enumerated_array<int8_t, NDL, Difficulty_level_type> &bytep)
{
	char	*curtext;
	range_for (auto &i, bytep)
	{
		curtext = strtok(NULL, space_tab);
		i = atoi(curtext);
	}
}

// ------------------------------------------------------------------------------
//	Convert field of view from an angle in 0..360 to cosine.
static void adjust_field_of_view(enumerated_array<fix, NDL, Difficulty_level_type> &fovp)
{
	fixang	tt;
	float		ff;
	range_for (auto &i, fovp)
	{
		ff = - f2fl(i);
		if (ff > 179) {
			ff = 179;
		}
		ff = ff/360;
		tt = fl2f(ff);
		i = fix_cos(tt);
	}
}

static polygon_simpler_model_index build_polygon_simpler_model_index_from_polygon_model_index(const polygon_model_index i)
{
	const auto ii = underlying_value(i) + 1;
	if (ii > MAX_POLYGON_MODELS)
		return polygon_simpler_model_index::None;
	return static_cast<polygon_simpler_model_index>(ii);
}

}
}

namespace dsx {
namespace {

#if defined(DXX_BUILD_DESCENT_I)
static void clear_to_end_of_line(char *&arg)
{
	arg = NULL;
}
#elif defined(DXX_BUILD_DESCENT_II)
static void clear_to_end_of_line()
{
	arg = strtok( NULL, space_tab );
	while (arg != NULL)
		arg = strtok( NULL, space_tab );
}
#endif

#if defined(DXX_BUILD_DESCENT_I)
static void bm_read_sound(char *&arg, int skip, int pc_shareware)
#elif defined(DXX_BUILD_DESCENT_II)
void bm_read_sound(int skip)
#endif
{
	int alt_sound_num;

	const int read_sound_num = get_int();
#if defined(DXX_BUILD_DESCENT_I)
	alt_sound_num = pc_shareware ? read_sound_num : get_int();
#elif defined(DXX_BUILD_DESCENT_II)
	alt_sound_num = get_int();
#endif

	if ( read_sound_num>=MAX_SOUNDS )
		Error( "Too many sound files.\n" );

	if (read_sound_num >= num_sounds)
		num_sounds = read_sound_num+1;

#if defined(DXX_BUILD_DESCENT_II)
	if (Sounds[read_sound_num] != 255)
		Error("Sound num %d already used, bitmaps.tbl, line %d\n",read_sound_num,linenum);
#endif

	arg = strtok(NULL, space_tab);

	Sounds[read_sound_num] = ds_load(skip, arg);

	if ( alt_sound_num == 0 )
		AltSounds[read_sound_num] = sound_num;
	else if (alt_sound_num < 0 )
		AltSounds[read_sound_num] = 255;
	else
		AltSounds[read_sound_num] = alt_sound_num;

	if (Sounds[read_sound_num] == 255)
		Error("Can't load soundfile <%s>",arg);
}

// ------------------------------------------------------------------------------
#if defined(DXX_BUILD_DESCENT_I)
static void bm_read_robot_ai(d_robot_info_array &Robot_info, char *&arg, const int skip)
#elif defined(DXX_BUILD_DESCENT_II)
void bm_read_robot_ai(d_robot_info_array &Robot_info, const int skip)
#endif
{
	char			*robotnum_text;

	robotnum_text = strtok(NULL, space_tab);
	const auto l = strtoul(robotnum_text, nullptr, 10);
	const auto o = Robot_info.valid_index(l);
	if (!o)
		return;
	const auto robotnum = *o;
	auto &robptr = Robot_info[robotnum];

	assert(l == Num_robot_ais);		//make sure valid number

	if (skip) {
		Num_robot_ais++;
#if defined(DXX_BUILD_DESCENT_I)
		clear_to_end_of_line(arg);
#elif defined(DXX_BUILD_DESCENT_II)
		clear_to_end_of_line();
#endif
		return;
	}

	Num_robot_ais++;

	get4fix(robptr.field_of_view);
	get4fix(robptr.firing_wait);
#if defined(DXX_BUILD_DESCENT_II)
	get4fix(robptr.firing_wait2);
#endif
	get4byte(robptr.rapidfire_count);
	get4fix(robptr.turn_time);
#if defined(DXX_BUILD_DESCENT_I)
	enumerated_array<fix, NDL, Difficulty_level_type>		fire_power,						//	damage done by a hit from this robot
		shield;							//	shield strength of this robot
	get4fix(fire_power);
	get4fix(shield);
#elif defined(DXX_BUILD_DESCENT_II)
//	get4fix(robptr->fire_power);
//	get4fix(robptr->shield);
#endif
	get4fix(robptr.max_speed);
	get4fix(robptr.circle_distance);
	get4byte(robptr.evade_speed);

	robptr.always_0xabcd	= 0xabcd;
	adjust_field_of_view(robptr.field_of_view);
}

//	----------------------------------------------------------------------------------------------
//this will load a bitmap for a polygon models.  it puts the bitmap into
//the array ObjBitmaps[], and also deals with animating bitmaps
//returns a pointer to the bitmap
static grs_bitmap *load_polymodel_bitmap(int skip, const char *name)
{
	auto &Effects = LevelUniqueEffectsClipState.Effects;
	assert(N_ObjBitmaps < ObjBitmaps.size());

//	Assert( N_ObjBitmaps == N_ObjBitmapPtrs );

	if (name[0] == '%') {		//an animating bitmap!
		const unsigned eclip_num = atoi(name+1);

		auto &changing_object_texture = Effects[eclip_num].changing_object_texture;
		// On first reference, changing_object_texture will be None.
		// Assign it a value.
		if (changing_object_texture == object_bitmap_index::None)
			changing_object_texture = static_cast<object_bitmap_index>(N_ObjBitmaps++);
		ObjBitmapPtrs[N_ObjBitmapPtrs++] = changing_object_texture;
#if defined(DXX_BUILD_DESCENT_II)
		assert(N_ObjBitmaps < ObjBitmaps.size());
		assert(N_ObjBitmapPtrs < ObjBitmapPtrs.size());
#endif
		return NULL;
	}
	else 	{
		const auto loaded_value = bm_load_sub(skip, name);
		const auto oi = static_cast<object_bitmap_index>(N_ObjBitmaps);
		auto &ob = ObjBitmaps[oi];
		ob = loaded_value;
#if defined(DXX_BUILD_DESCENT_II)
		auto &gbo = GameBitmaps[ob];
		if (gbo.bm_w != 64 || gbo.bm_h != 64)
			Error("Bitmap <%s> is not 64x64",name);
#endif
		ObjBitmapPtrs[N_ObjBitmapPtrs++] = oi;
		N_ObjBitmaps++;
#if defined(DXX_BUILD_DESCENT_II)
		assert(N_ObjBitmaps < ObjBitmaps.size());
		assert(N_ObjBitmapPtrs < ObjBitmapPtrs.size());
#endif
		return &GameBitmaps[ob];
	}
}

#define MAX_MODEL_VARIANTS	4

// ------------------------------------------------------------------------------
#if defined(DXX_BUILD_DESCENT_I)
static void bm_read_robot(d_level_shared_robot_info_state &LevelSharedRobotInfoState, char *&arg, int skip)
#elif defined(DXX_BUILD_DESCENT_II)
void bm_read_robot(d_level_shared_robot_info_state &LevelSharedRobotInfoState, int skip)
#endif
{
	char			*model_name[MAX_MODEL_VARIANTS];
	int			n_models,i;
	int			first_bitmap_num[MAX_MODEL_VARIANTS];
	char			*equal_ptr;
	auto exp1_vclip_num{vclip_index::None};
	int exp1_sound_num = sound_none;
	auto exp2_vclip_num{vclip_index::None};
	int exp2_sound_num = sound_none;
	fix			lighting = F1_0/2;		// Default
	fix			strength = F1_0*10;		// Default strength
	fix			mass = f1_0*4;
	fix			drag = f1_0/2;
	weapon_id_type weapon_type = weapon_id_type::LASER_ID_L1;
	int			contains_count=0, contains_id=0, contains_prob=0, contains_type=0;
#if defined(DXX_BUILD_DESCENT_II)
	weapon_id_type weapon_type2 = weapon_id_type::unspecified;
	auto behavior = ai_behavior::AIB_NORMAL;
	int			companion = 0, smart_blobs=0, energy_blobs=0, badass=0, energy_drain=0, kamikaze=0, thief=0, pursuit=0, lightcast=0, death_roll=0;
	fix			glow=0, aim=F1_0;
	int			deathroll_sound = SOUND_BOSS_SHARE_DIE;	//default
	int			taunt_sound = ROBOT_SEE_SOUND_DEFAULT;
	ubyte flags=0;
#endif
	int			score_value=1000;
	int			cloak_type=0;		//	Default = this robot does not cloak
	int			attack_type=0;		//	Default = this robot attacks by firing (1=lunge)
	boss_robot_id	boss_flag{};				//	Default = robot is not a boss.
	int			see_sound = ROBOT_SEE_SOUND_DEFAULT;
	int			attack_sound = ROBOT_ATTACK_SOUND_DEFAULT;
	int			claw_sound = ROBOT_CLAW_SOUND_DEFAULT;

	assert(LevelSharedRobotInfoState.N_robot_types < MAX_ROBOT_TYPES);

	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	if (skip) {
		const auto N_robot_types = LevelSharedRobotInfoState.N_robot_types;
		if (const auto o = Robot_info.valid_index(N_robot_types))
		{
			const auto rid = *o;
			++ LevelSharedRobotInfoState.N_robot_types;
			auto &ri = Robot_info[rid];
			ri.model_num = polygon_model_index::None;
		}
#if defined(DXX_BUILD_DESCENT_I)
		Num_total_object_types++;
		clear_to_end_of_line(arg);
#elif defined(DXX_BUILD_DESCENT_II)
		clear_to_end_of_line();
#endif
		return;
	}

	model_name[0] = strtok( NULL, space_tab );
	first_bitmap_num[0] = N_ObjBitmapPtrs;
	n_models = 1;

	// Process bitmaps
	current_bm_type=bm_type::robot;
	arg = strtok( NULL, space_tab );
	while (arg!=NULL)	{
		equal_ptr = strchr( arg, '=' );
		if ( equal_ptr )	{
			*equal_ptr='\0';
			equal_ptr++;
			// if we have john=cool, arg is 'john' and equal_ptr is 'cool'
			if (!d_stricmp( arg, "exp1_vclip" ))	{
				exp1_vclip_num = build_vclip_index_from_untrusted(atoi(equal_ptr));
			} else if (!d_stricmp( arg, "exp2_vclip" ))	{
				exp2_vclip_num = build_vclip_index_from_untrusted(atoi(equal_ptr));
			} else if (!d_stricmp( arg, "exp1_sound" ))	{
				exp1_sound_num = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "exp2_sound" ))	{
				exp2_sound_num = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "lighting" ))	{
				lighting = fl2f(atof(equal_ptr));
				if ( (lighting < 0) || (lighting > F1_0 )) {
					Error( "In bitmaps.tbl, lighting value of %.2f is out of range 0..1.\n", f2fl(lighting));
				}
			} else if (!d_stricmp( arg, "weapon_type" )) {
				weapon_type = static_cast<weapon_id_type>(atoi(equal_ptr));
			}
#if defined(DXX_BUILD_DESCENT_II)
			else if (!d_stricmp( arg, "weapon_type2"))
			{
				weapon_type2 = static_cast<weapon_id_type>(atoi(equal_ptr));
			}
#endif
			else if (!d_stricmp( arg, "strength" )) {
				strength = i2f(atoi(equal_ptr));
			} else if (!d_stricmp( arg, "mass" )) {
				mass = fl2f(atof(equal_ptr));
			} else if (!d_stricmp( arg, "drag" )) {
				drag = fl2f(atof(equal_ptr));
			} else if (!d_stricmp( arg, "contains_id" )) {
				contains_id = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "contains_type" )) {
				contains_type = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "contains_count" )) {
				contains_count = atoi(equal_ptr);
			}
#if defined(DXX_BUILD_DESCENT_II)
			else if (!d_stricmp( arg, "companion" )) {
				companion = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "badass" )) {
				badass = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "lightcast" )) {
				lightcast = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "glow" )) {
				glow = fl2f(atof(equal_ptr));
			} else if (!d_stricmp( arg, "death_roll" )) {
				death_roll = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "deathroll_sound" )) {
				deathroll_sound = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "thief" )) {
				thief = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "kamikaze" )) {
				kamikaze = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "pursuit" )) {
				pursuit = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "smart_blobs" )) {
				smart_blobs = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "energy_blobs" )) {
				energy_blobs = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "energy_drain" )) {
				energy_drain = atoi(equal_ptr);
			}
#endif
			else if (!d_stricmp( arg, "contains_prob" )) {
				contains_prob = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "cloak_type" )) {
				cloak_type = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "attack_type" )) {
				attack_type = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "boss" )) {
				const auto i = strtoul(equal_ptr, nullptr, 10);
				boss_flag = (i == static_cast<uint8_t>(boss_robot_id::d1_1) || i == static_cast<uint8_t>(boss_robot_id::d1_superboss)) ? static_cast<boss_robot_id>(i) : boss_robot_id::None;
			} else if (!d_stricmp( arg, "score_value" )) {
				score_value = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "see_sound" )) {
				see_sound = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "attack_sound" )) {
				attack_sound = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "claw_sound" )) {
				claw_sound = atoi(equal_ptr);
			}
#if defined(DXX_BUILD_DESCENT_II)
			else if (!d_stricmp( arg, "taunt_sound" )) {
				taunt_sound = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "aim" )) {
				aim = fl2f(atof(equal_ptr));
			} else if (!d_stricmp( arg, "big_radius" )) {
				if (atoi(equal_ptr))
					flags |= RIF_BIG_RADIUS;
			} else if (!d_stricmp( arg, "behavior" )) {
				if (!d_stricmp(equal_ptr, "STILL"))
					behavior = ai_behavior::AIB_STILL;
				else if (!d_stricmp(equal_ptr, "NORMAL"))
					behavior = ai_behavior::AIB_NORMAL;
				else if (!d_stricmp(equal_ptr, "BEHIND"))
					behavior = ai_behavior::AIB_BEHIND;
				else if (!d_stricmp(equal_ptr, "RUN_FROM"))
					behavior = ai_behavior::AIB_RUN_FROM;
				else if (!d_stricmp(equal_ptr, "SNIPE"))
					behavior = ai_behavior::AIB_SNIPE;
				else if (!d_stricmp(equal_ptr, "STATION"))
					behavior = ai_behavior::AIB_STATION;
				else if (!d_stricmp(equal_ptr, "FOLLOW"))
					behavior = ai_behavior::AIB_FOLLOW;
				else
					Int3();	//	Error.  Illegal behavior type for current robot.
			}
#endif
			else if (!d_stricmp( arg, "name" )) {
#if DXX_USE_EDITOR
				auto &name = Robot_names[static_cast<robot_id>(LevelSharedRobotInfoState.N_robot_types)];
				const auto len = strlen(equal_ptr);
				assert(len < name.size());	//	Oops, name too long.
				memcpy(name.data(), &equal_ptr[1], len - 2);
				name[len - 2] = 0;
#endif
			} else if (!d_stricmp( arg, "simple_model" )) {
				model_name[n_models] = equal_ptr;
				first_bitmap_num[n_models] = N_ObjBitmapPtrs;
				n_models++;
#if defined(DXX_BUILD_DESCENT_II)
				Assert(n_models < MAX_MODEL_VARIANTS);
#endif
			}
#if defined(DXX_BUILD_DESCENT_II)
			else
			{
				Int3();
			}
#endif
		} else {			// Must be a texture specification...
			load_polymodel_bitmap(skip, arg);
		}
		arg = strtok( NULL, space_tab );
	}

	auto &current_robot_info = Robot_info[static_cast<robot_id>(LevelSharedRobotInfoState.N_robot_types)];
	//clear out anim info
	range_for (auto &g, current_robot_info.anim_states)
		range_for (auto &s, g)
			s.n_joints = 0;

	first_bitmap_num[n_models] = N_ObjBitmapPtrs;

	auto &Polygon_models = LevelSharedPolygonModelState.Polygon_models;
	for (i=0;i<n_models;i++) {
		int n_textures;

		n_textures = first_bitmap_num[i+1] - first_bitmap_num[i];

		const auto model_num = load_polygon_model(model_name[i], n_textures, first_bitmap_num[i], (i == 0) ? &current_robot_info : nullptr);

		if (i==0)
			current_robot_info.model_num = model_num;
		else
			Polygon_models.front().simpler_model = build_polygon_simpler_model_index_from_polygon_model_index(model_num);
	}

#if defined(DXX_BUILD_DESCENT_I)
	ObjType[Num_total_object_types] = OL_ROBOT;
	ObjId[Num_total_object_types] = static_cast<polygon_model_index>(LevelSharedRobotInfoState.N_robot_types);
#elif defined(DXX_BUILD_DESCENT_II)
	if ((glow > i2f(15)) || (glow < 0) || (glow != 0 && glow < 0x1000)) {
		Int3();
	}
#endif

	current_robot_info.exp1_vclip_num = exp1_vclip_num;
	current_robot_info.exp2_vclip_num = exp2_vclip_num;
	current_robot_info.exp1_sound_num = exp1_sound_num;
	current_robot_info.exp2_sound_num = exp2_sound_num;
	current_robot_info.lighting = lighting;
	current_robot_info.weapon_type = weapon_type;
#if defined(DXX_BUILD_DESCENT_II)
	current_robot_info.weapon_type2 = weapon_type2;
#endif
	current_robot_info.strength = strength;
	current_robot_info.mass = mass;
	current_robot_info.drag = drag;
	current_robot_info.cloak_type = cloak_type;
	current_robot_info.attack_type = attack_type;
	current_robot_info.boss_flag = boss_flag;

	/* The input file uses zero/non-zero to pick powerup/robot.  Other input
	 * sources store robot as OBJ_ROBOT and powerup as OBJ_POWERUP, so
	 * build_contained_object_parameters_from_untrusted expects those values.
	 * Convert the type here.
	 */
	current_robot_info.contains = build_contained_object_parameters_from_untrusted(contains_type ? underlying_value(contained_object_type::robot) : underlying_value(contained_object_type::powerup), contains_id, contains_count);
	current_robot_info.contains_prob = contains_prob;
#if defined(DXX_BUILD_DESCENT_II)
	current_robot_info.companion = companion;
	current_robot_info.badass = badass;
	current_robot_info.lightcast = lightcast;
	current_robot_info.glow = (glow>>12);		//convert to 4:4
	current_robot_info.death_roll = death_roll;
	current_robot_info.deathroll_sound = deathroll_sound;
	current_robot_info.thief = thief;
	current_robot_info.flags = flags;
	current_robot_info.kamikaze = kamikaze;
	current_robot_info.pursuit = pursuit;
	current_robot_info.smart_blobs = smart_blobs;
	current_robot_info.energy_blobs = energy_blobs;
	current_robot_info.energy_drain = energy_drain;
#endif
	current_robot_info.score_value = score_value;
	current_robot_info.see_sound = see_sound;
	current_robot_info.attack_sound = attack_sound;
	current_robot_info.claw_sound = claw_sound;
#if defined(DXX_BUILD_DESCENT_II)
	current_robot_info.taunt_sound = taunt_sound;
	current_robot_info.behavior = behavior;		//	Default behavior for this robot, if coming out of matcen.
	current_robot_info.aim = min(f2i(aim*255), 255);		//	how well this robot type can aim.  255=perfect
#endif

	++LevelSharedRobotInfoState.N_robot_types;
#if defined(DXX_BUILD_DESCENT_I)
	Num_total_object_types++;
#elif defined(DXX_BUILD_DESCENT_II)
	current_bm_type = bm_type::none;
#endif
}

#if defined(DXX_BUILD_DESCENT_I)
//read a polygon object of some sort
void bm_read_object(char *&arg, int skip)
#elif defined(DXX_BUILD_DESCENT_II)
//read a reactor model
void bm_read_reactor(void)
#endif
{
	char *model_name, *model_name_dead=NULL;
	int first_bitmap_num, first_bitmap_num_dead=0, n_normal_bitmaps;
	char *equal_ptr;
	fix	lighting = F1_0/2;		// Default
#if defined(DXX_BUILD_DESCENT_I)
	int type = -1;
	fix strength=0;
#elif defined(DXX_BUILD_DESCENT_II)
	assert(Num_reactors < Reactors.size());
#endif

	model_name = strtok( NULL, space_tab );

	// Process bitmaps
	current_bm_type = bm_type::none;
	arg = strtok( NULL, space_tab );
	first_bitmap_num = N_ObjBitmapPtrs;

	while (arg!=NULL)	{

		equal_ptr = strchr( arg, '=' );

		if ( equal_ptr )	{
			*equal_ptr='\0';
			equal_ptr++;

			// if we have john=cool, arg is 'john' and equal_ptr is 'cool'

#if defined(DXX_BUILD_DESCENT_I)
			if (!d_stricmp(arg,"type")) {
				if (!d_stricmp(equal_ptr,"controlcen"))
					type = OL_CONTROL_CENTER;
				else if (!d_stricmp(equal_ptr,"clutter"))
					type = OL_CLUTTER;
				else if (!d_stricmp(equal_ptr,"exit"))
					type = OL_EXIT;
			}
			else
#endif
			if (!d_stricmp( arg, "dead_pof" ))	{
				model_name_dead = equal_ptr;
				first_bitmap_num_dead=N_ObjBitmapPtrs;
			} else if (!d_stricmp( arg, "lighting" ))	{
				lighting = fl2f(atof(equal_ptr));
				if ( (lighting < 0) || (lighting > F1_0 )) {
					Error( "In bitmaps.tbl, lighting value of %.2f is out of range 0..1.\n", f2fl(lighting));
				}
			}
#if defined(DXX_BUILD_DESCENT_I)
			else if (!d_stricmp( arg, "strength" )) {
				strength = fl2f(atof(equal_ptr));
			}
#elif defined(DXX_BUILD_DESCENT_II)
			else {
				Int3();
			}
#endif
		} else {			// Must be a texture specification...
#if defined(DXX_BUILD_DESCENT_I)
			load_polymodel_bitmap(skip, arg);
#elif defined(DXX_BUILD_DESCENT_II)
			load_polymodel_bitmap(0, arg);
#endif
		}
		arg = strtok( NULL, space_tab );
	}

	if ( model_name_dead )
		n_normal_bitmaps = first_bitmap_num_dead-first_bitmap_num;
	else
		n_normal_bitmaps = N_ObjBitmapPtrs-first_bitmap_num;

	const auto model_num = load_polygon_model(model_name, n_normal_bitmaps, first_bitmap_num, nullptr);

#if defined(DXX_BUILD_DESCENT_I)
	if (type == OL_CONTROL_CENTER)
		read_model_guns(model_name, Reactors[0]);
#endif
	if ( model_name_dead )
		Dead_modelnums[model_num]  = load_polygon_model(model_name_dead,N_ObjBitmapPtrs-first_bitmap_num_dead,first_bitmap_num_dead,NULL);
	else
		Dead_modelnums[model_num] = polygon_model_index::None;

#if defined(DXX_BUILD_DESCENT_I)
	if (type == -1)
		Error("No object type specfied for object in BITMAPS.TBL on line %d\n",linenum);

	ObjType[Num_total_object_types] = type;
	ObjId[Num_total_object_types] = model_num;
	ObjStrength[Num_total_object_types] = strength;

	Num_total_object_types++;

	if (type == OL_EXIT) {
		exit_modelnum = model_num;
		destroyed_exit_modelnum = Dead_modelnums[model_num];
	}
#elif defined(DXX_BUILD_DESCENT_II)
	Reactors[Num_reactors].model_num = model_num;
	read_model_guns(model_name, Reactors[Num_reactors]);

	Num_reactors++;
#endif
}

#if defined(DXX_BUILD_DESCENT_II)
//read the marker object
void bm_read_marker()
{
	char *model_name;
	int first_bitmap_num, n_normal_bitmaps;
	char *equal_ptr;

	model_name = strtok( NULL, space_tab );

	// Process bitmaps
	current_bm_type = bm_type::none;
	arg = strtok( NULL, space_tab );
	first_bitmap_num = N_ObjBitmapPtrs;

	while (arg!=NULL)	{

		equal_ptr = strchr( arg, '=' );

		if ( equal_ptr )	{
			*equal_ptr='\0';
			equal_ptr++;

			// if we have john=cool, arg is 'john' and equal_ptr is 'cool'
			Int3();

		} else {			// Must be a texture specification...
			load_polymodel_bitmap(0, arg);
		}
		arg = strtok( NULL, space_tab );
	}

	n_normal_bitmaps = N_ObjBitmapPtrs-first_bitmap_num;

	LevelSharedPolygonModelState.Marker_model_num = load_polygon_model(model_name, n_normal_bitmaps, first_bitmap_num, NULL);
}

//read the exit model
void bm_read_exitmodel()
{
	char *model_name, *model_name_dead=NULL;
	int first_bitmap_num=0, first_bitmap_num_dead=0, n_normal_bitmaps;
	char *equal_ptr;

	model_name = strtok( NULL, space_tab );

	// Process bitmaps
	current_bm_type = bm_type::none;
	arg = strtok( NULL, space_tab );
	first_bitmap_num = N_ObjBitmapPtrs;

	while (arg!=NULL)	{

		equal_ptr = strchr( arg, '=' );

		if ( equal_ptr )	{
			*equal_ptr='\0';
			equal_ptr++;

			// if we have john=cool, arg is 'john' and equal_ptr is 'cool'

			if (!d_stricmp( arg, "dead_pof" ))	{
				model_name_dead = equal_ptr;
				first_bitmap_num_dead=N_ObjBitmapPtrs;
			} else {
				Int3();
			}
		} else {			// Must be a texture specification...
			load_polymodel_bitmap(0, arg);
		}
		arg = strtok( NULL, space_tab );
	}

	if ( model_name_dead )
		n_normal_bitmaps = first_bitmap_num_dead-first_bitmap_num;
	else
		n_normal_bitmaps = N_ObjBitmapPtrs-first_bitmap_num;

	const auto model_num = load_polygon_model(model_name, n_normal_bitmaps, first_bitmap_num, nullptr);

	if ( model_name_dead )
		Dead_modelnums[model_num]  = load_polygon_model(model_name_dead,N_ObjBitmapPtrs-first_bitmap_num_dead,first_bitmap_num_dead,NULL);
	else
		Dead_modelnums[model_num] = polygon_model_index::None;

	exit_modelnum = model_num;
	destroyed_exit_modelnum = Dead_modelnums[model_num];

}
#endif

#if defined(DXX_BUILD_DESCENT_I)
void bm_read_player_ship(char *&arg, int skip)
#elif defined(DXX_BUILD_DESCENT_II)
void bm_read_player_ship(void)
#endif
{
	char	*model_name_dying=NULL;
	char	*model_name[MAX_MODEL_VARIANTS];
	int	n_models=0,i;
	int	first_bitmap_num[MAX_MODEL_VARIANTS];
	char *equal_ptr;
	robot_info ri;
	int last_multi_bitmap_num=-1;

	// Process bitmaps
	current_bm_type = bm_type::none;

	arg = strtok( NULL, space_tab );

	Player_ship->mass = Player_ship->drag = 0;	//stupid defaults
	Player_ship->expl_vclip_num = vclip_index::None;

	while (arg!=NULL)	{

		equal_ptr = strchr( arg, '=' );

		if ( equal_ptr )	{

			*equal_ptr='\0';
			equal_ptr++;

			// if we have john=cool, arg is 'john' and equal_ptr is 'cool'

			if (!d_stricmp( arg, "model" )) {
				Assert(n_models==0);
				model_name[0] = equal_ptr;
				first_bitmap_num[0] = N_ObjBitmapPtrs;
				n_models = 1;
			} else if (!d_stricmp( arg, "simple_model" )) {
				model_name[n_models] = equal_ptr;
				first_bitmap_num[n_models] = N_ObjBitmapPtrs;
				n_models++;
#if defined(DXX_BUILD_DESCENT_II)
				Assert(n_models < MAX_MODEL_VARIANTS);
#endif

				if (First_multi_bitmap_num!=-1 && last_multi_bitmap_num==-1)
					last_multi_bitmap_num=N_ObjBitmapPtrs;
			}
			else if (!d_stricmp( arg, "mass" ))
				Player_ship->mass = fl2f(atof(equal_ptr));
			else if (!d_stricmp( arg, "drag" ))
				Player_ship->drag = fl2f(atof(equal_ptr));
//			else if (!d_stricmp( arg, "low_thrust" ))
//				Player_ship->low_thrust = fl2f(atof(equal_ptr));
			else if (!d_stricmp( arg, "max_thrust" ))
				Player_ship->max_thrust = fl2f(atof(equal_ptr));
			else if (!d_stricmp( arg, "reverse_thrust" ))
				Player_ship->reverse_thrust = fl2f(atof(equal_ptr));
			else if (!d_stricmp( arg, "brakes" ))
				Player_ship->brakes = fl2f(atof(equal_ptr));
			else if (!d_stricmp( arg, "wiggle" ))
				Player_ship->wiggle = fl2f(atof(equal_ptr));
			else if (!d_stricmp( arg, "max_rotthrust" ))
				Player_ship->max_rotthrust = fl2f(atof(equal_ptr));
			else if (!d_stricmp( arg, "dying_pof" ))
				model_name_dying = equal_ptr;
			else if (!d_stricmp( arg, "expl_vclip_num" ))
				Player_ship->expl_vclip_num = build_vclip_index_from_untrusted(atoi(equal_ptr));
#if defined(DXX_BUILD_DESCENT_II)
			else {
				Int3();
			}
#endif
		}
		else if (!d_stricmp( arg, "multi_textures" )) {

			First_multi_bitmap_num = N_ObjBitmapPtrs;
			first_bitmap_num[n_models] = N_ObjBitmapPtrs;

		}
		else			// Must be a texture specification...
		{
#if defined(DXX_BUILD_DESCENT_I)
			load_polymodel_bitmap(skip, arg);
#elif defined(DXX_BUILD_DESCENT_II)
			load_polymodel_bitmap(0, arg);
#endif
		}

		arg = strtok( NULL, space_tab );
	}
	if (First_multi_bitmap_num!=-1 && last_multi_bitmap_num==-1)
		last_multi_bitmap_num=N_ObjBitmapPtrs;

	if (First_multi_bitmap_num==-1)
		first_bitmap_num[n_models] = N_ObjBitmapPtrs;

	Assert(last_multi_bitmap_num-First_multi_bitmap_num == (MAX_PLAYERS-1)*2);

	auto &Polygon_models = LevelSharedPolygonModelState.Polygon_models;
	for (i=0;i<n_models;i++) {
		int n_textures;

		n_textures = first_bitmap_num[i+1] - first_bitmap_num[i];
		const auto model_num = load_polygon_model(model_name[i], n_textures, first_bitmap_num[i], (i == 0) ? &ri : nullptr);

		if (i==0)
			Player_ship->model_num = model_num;
		else
			Polygon_models.front().simpler_model = build_polygon_simpler_model_index_from_polygon_model_index(model_num);
	}

	if ( model_name_dying ) {
		Assert(n_models);
		Dying_modelnums[Player_ship->model_num]  = load_polygon_model(model_name_dying,first_bitmap_num[1]-first_bitmap_num[0],first_bitmap_num[0],NULL);
	}

	Assert(ri.n_guns == N_PLAYER_GUNS);

	//calc player gun positions

	{
		const auto &r = ri;
		const auto &pm = Polygon_models[Player_ship->model_num];

		/* Binding to the zip iterator produces references.  For r.gun_points
		 * and r.gun_submodels, a mutable local is desired instead.
		 *
		 * If there are no submodels, copy directly from `r.gun_points` to
		 * `plr_gun_point` (bound to an element of `Player_ship->gun_points`).
		 *
		 * If there are submodels:
		 * - Copy the submodel index into a local `mn`
		 * - Copy the r.gun_points vector into a local `pnt`
		 * - Redirect `ppnt` to the local.
		 * - Update the local as needed from the polygon models.
		 * - Copy that local to `plr_gun_point`.
		 *
		 * This minimizes unnecessary copying.
		 */
		for (auto &&[rpnt, rmn, plr_gun_point] : zip(partial_range(r.gun_points, r.n_guns), r.gun_submodels, Player_ship->gun_points))
		{
			auto ppnt = &rpnt;
			/* Create a local copy to be modified by the `while` loop. */
			vms_vector pnt;
			if (auto mn = rmn)
			{
			//instance up the tree for this gun
				pnt = rpnt;
				ppnt = &pnt;
				for (; mn && mn < std::size(pm.submodel_offsets); mn = pm.submodel_parents[mn])
					vm_vec_add2(pnt, pm.submodel_offsets[mn]);
			}
			plr_gun_point = *ppnt;
		}
	}
}

#if defined(DXX_BUILD_DESCENT_I)
std::size_t bm_read_some_file(d_vclip_array &Vclip, std::size_t texture_count, const std::string &dest_bm, char *&arg, int skip)
#elif defined(DXX_BUILD_DESCENT_II)
std::size_t bm_read_some_file(d_vclip_array &Vclip, std::size_t texture_count, int skip)
#endif
{

	switch (current_bm_type) {
#if defined(DXX_BUILD_DESCENT_II)
		case bm_type::none:
		Error("Trying to read bitmap <%s> with current_bm_type==BM_NONE on line %d of BITMAPS.TBL",arg,linenum);
		break;
#endif
	case bm_type::cockpit:	{
		const auto bitmap = bm_load_sub(skip, arg);
		if (Num_cockpits >= N_COCKPIT_BITMAPS)
			throw std::runtime_error("too many cockpit bitmaps");
		cockpit_bitmap[static_cast<cockpit_mode_t>(Num_cockpits++)] = bitmap;
		return texture_count;
		}
	case bm_type::gauges:
#if defined(DXX_BUILD_DESCENT_I)
		bm_read_gauges(arg, skip);
#elif defined(DXX_BUILD_DESCENT_II)
		bm_read_gauges(skip);
		return texture_count;
	case bm_type::gauges_hires:
		bm_read_gauges_hires();
#endif
		return texture_count;
	case bm_type::vclip:
#if defined(DXX_BUILD_DESCENT_I)
		bm_read_vclip(Vclip, arg, skip);
#elif defined(DXX_BUILD_DESCENT_II)
		bm_read_vclip(Vclip, skip);
#endif
		return texture_count;
	case bm_type::eclip:
#if defined(DXX_BUILD_DESCENT_I)
		texture_count = bm_read_eclip(texture_count, dest_bm, arg, skip);
#elif defined(DXX_BUILD_DESCENT_II)
		texture_count = bm_read_eclip(texture_count, skip);
#endif
		return texture_count;
	case bm_type::textures:			{
		const auto bitmap = bm_load_sub(skip, arg);
		Assert(tmap_count < MAX_TEXTURES);
  		tmap_count++;
		Textures[texture_count] = bitmap;
		set_texture_name(LevelUniqueTmapInfoState.TmapInfo, texture_count, arg);
		Assert(texture_count < MAX_TEXTURES);
		texture_count++;
		NumTextures = texture_count;
		return texture_count;
		}
	case bm_type::wclip:
#if defined(DXX_BUILD_DESCENT_I)
		texture_count = bm_read_wclip(texture_count, arg, skip);
		return texture_count;
	default:
#elif defined(DXX_BUILD_DESCENT_II)
		texture_count = bm_read_wclip(texture_count, skip);
		return texture_count;
#endif
	case bm_type::effects:
	case bm_type::wall_anims:
	case bm_type::robot:
		return texture_count;
	}
#if defined(DXX_BUILD_DESCENT_II)
	Error("Trying to read bitmap <%s> with unknown current_bm_type <%x> on line %d of BITMAPS.TBL",arg,static_cast<unsigned>(current_bm_type), linenum);
#endif
}

// ------------------------------------------------------------------------------
//	If unused_flag is set, then this is just a placeholder.  Don't actually reference vclips or load bbms.
#if defined(DXX_BUILD_DESCENT_I)
void bm_read_weapon(char *&arg, int skip, int unused_flag)
#elif defined(DXX_BUILD_DESCENT_II)
void bm_read_weapon(int skip, int unused_flag)
#endif
{
	int	i,n;
	int	n_models=0;
	char 	*equal_ptr;
	char	*pof_file_inner=NULL;
	char	*model_name[MAX_MODEL_VARIANTS];
	int	first_bitmap_num[MAX_MODEL_VARIANTS];
	int	lighted;					//flag for whether is a texture is lighted

	Assert(N_weapon_types < MAX_WEAPON_TYPES);

	n = N_weapon_types;
	N_weapon_types++;
#if defined(DXX_BUILD_DESCENT_II)
	Assert(N_weapon_types <= MAX_WEAPON_TYPES);
#endif

	if (unused_flag) {
#if defined(DXX_BUILD_DESCENT_I)
		clear_to_end_of_line(arg);
#elif defined(DXX_BUILD_DESCENT_II)
		clear_to_end_of_line();
#endif
		return;
	}

	if (skip) {
#if defined(DXX_BUILD_DESCENT_I)
		clear_to_end_of_line(arg);
#elif defined(DXX_BUILD_DESCENT_II)
		clear_to_end_of_line();
#endif
		return;
	}

	// Initialize weapon array
	Weapon_info[n].render = WEAPON_RENDER_NONE;		// 0=laser, 1=blob, 2=object
	Weapon_info[n].bitmap = {};
	Weapon_info[n].model_num = polygon_model_index::None;
	Weapon_info[n].model_num_inner = polygon_model_index::None;
	Weapon_info[n].blob_size = 0x1000;									// size of blob
	Weapon_info[n].flash_vclip = vclip_index::None;
	Weapon_info[n].flash_sound = SOUND_LASER_FIRED;
	Weapon_info[n].flash_size = 0;
	Weapon_info[n].robot_hit_vclip = vclip_index::None;
	Weapon_info[n].robot_hit_sound = sound_none;
	Weapon_info[n].wall_hit_vclip = vclip_index::None;
	Weapon_info[n].wall_hit_sound = sound_none;
	Weapon_info[n].impact_size = 0;
	for (auto &i : Weapon_info[n].strength)
		i = F1_0;
	for (auto &i : Weapon_info[n].speed)
		i = F1_0*10;
	Weapon_info[n].mass = F1_0;
	Weapon_info[n].thrust = 0;
	Weapon_info[n].drag = 0;
	Weapon_info[n].persistent = weapon_info::persistence_flag::terminate_on_impact;

	Weapon_info[n].energy_usage = 0;					//	How much fuel is consumed to fire this weapon.
	Weapon_info[n].ammo_usage = 0;					//	How many units of ammunition it uses.
	Weapon_info[n].fire_wait = F1_0/4;				//	Time until this weapon can be fired again.
	Weapon_info[n].fire_count = 1;					//	Number of bursts fired from EACH GUN per firing.  For weapons which fire from both sides, 3*fire_count shots will be fired.
	Weapon_info[n].damage_radius = 0;				//	Radius of damage for missiles, not lasers.  Does damage to objects within this radius of hit point.
//--01/19/95, mk--	Weapon_info[n].damage_force = 0;					//	Force (movement) due to explosion
	Weapon_info[n].destroyable = 1;					//	Weapons default to destroyable
	Weapon_info[n].matter = weapon_info::matter_flag::energy;							//	Weapons default to not being constructed of matter (they are energy!)
	Weapon_info[n].bounce = weapon_info::bounce_type::never;							//	Weapons default to not bouncing off walls

#if defined(DXX_BUILD_DESCENT_II)
	Weapon_info[n].flags = 0;
#endif

	Weapon_info[n].lifetime = WEAPON_DEFAULT_LIFETIME;					//	Number of bursts fired from EACH GUN per firing.  For weapons which fire from both sides, 3*fire_count shots will be fired.

	Weapon_info[n].po_len_to_width_ratio = F1_0*10;

	Weapon_info[n].picture = {};
#if defined(DXX_BUILD_DESCENT_II)
	Weapon_info[n].hires_picture = {};
#endif
	Weapon_info[n].homing_flag = 0;

#if defined(DXX_BUILD_DESCENT_II)
	Weapon_info[n].flash = 0;
	Weapon_info[n].multi_damage_scale = F1_0;
	Weapon_info[n].afterburner_size = 0;
	Weapon_info[n].children = weapon_id_type::unspecified;
#endif

	// Process arguments
	arg = strtok( NULL, space_tab );

	lighted = 1;			//assume first texture is lighted

#if defined(DXX_BUILD_DESCENT_II)
	Weapon_info[n].speedvar = 128;
#endif

	while (arg!=NULL)	{
		equal_ptr = strchr( arg, '=' );
		if ( equal_ptr )	{
			*equal_ptr='\0';
			equal_ptr++;
			// if we have john=cool, arg is 'john' and equal_ptr is 'cool'
			if (!d_stricmp( arg, "laser_bmp" ))	{
				// Load bitmap with name equal_ptr

				Weapon_info[n].bitmap = bm_load_sub(skip, equal_ptr);		//load_polymodel_bitmap(equal_ptr);
				Weapon_info[n].render = WEAPON_RENDER_LASER;

			} else if (!d_stricmp( arg, "blob_bmp" ))	{
				// Load bitmap with name equal_ptr

				Weapon_info[n].bitmap = bm_load_sub(skip, equal_ptr);		//load_polymodel_bitmap(equal_ptr);
				Weapon_info[n].render = WEAPON_RENDER_BLOB;

			} else if (!d_stricmp( arg, "weapon_vclip" ))	{
				// Set vclip to play for this weapon.
				Weapon_info[n].bitmap = {};
				Weapon_info[n].render = WEAPON_RENDER_VCLIP;
				Weapon_info[n].weapon_vclip = build_vclip_index_from_untrusted(atoi(equal_ptr));

			} else if (!d_stricmp( arg, "none_bmp" )) {
				Weapon_info[n].bitmap = bm_load_sub(skip, equal_ptr);
				Weapon_info[n].render = WEAPON_RENDER_NONE;

			} else if (!d_stricmp( arg, "weapon_pof" ))	{
				// Load pof file
				Assert(n_models==0);
				model_name[0] = equal_ptr;
				first_bitmap_num[0] = N_ObjBitmapPtrs;
				n_models=1;
			} else if (!d_stricmp( arg, "simple_model" )) {
				model_name[n_models] = equal_ptr;
				first_bitmap_num[n_models] = N_ObjBitmapPtrs;
				n_models++;
#if defined(DXX_BUILD_DESCENT_II)
				Assert(n_models < MAX_MODEL_VARIANTS);
#endif
			} else if (!d_stricmp( arg, "weapon_pof_inner" ))	{
				// Load pof file
				pof_file_inner = equal_ptr;
			} else if (!d_stricmp( arg, "strength" )) {
				for (auto &i : unchecked_partial_range(Weapon_info[n].strength, Weapon_info[n].strength.size() - 1))
				{
#if defined(DXX_BUILD_DESCENT_I)
					i = i2f(atoi(equal_ptr));
#elif defined(DXX_BUILD_DESCENT_II)
					i = fl2f(atof(equal_ptr));
#endif
					equal_ptr = strtok(NULL, space_tab);
				}
				Weapon_info[n].strength.back() = i2f(atoi(equal_ptr));
			} else if (!d_stricmp( arg, "mass" )) {
				Weapon_info[n].mass = fl2f(atof(equal_ptr));
			} else if (!d_stricmp( arg, "drag" )) {
				Weapon_info[n].drag = fl2f(atof(equal_ptr));
			} else if (!d_stricmp( arg, "thrust" )) {
				Weapon_info[n].thrust = fl2f(atof(equal_ptr));
			} else if (!d_stricmp( arg, "matter" )) {
				Weapon_info[n].matter = static_cast<weapon_info::matter_flag>(atoi(equal_ptr));
			} else if (!d_stricmp( arg, "bounce" )) {
				Weapon_info[n].bounce = static_cast<weapon_info::bounce_type>(atoi(equal_ptr));
			} else if (!d_stricmp( arg, "speed" )) {
				for (auto &i : unchecked_partial_range(Weapon_info[n].speed, Weapon_info[n].speed.size() - 1))
				{
					i = i2f(atoi(equal_ptr));
					equal_ptr = strtok(NULL, space_tab);
				}
				Weapon_info[n].speed.back() = i2f(atoi(equal_ptr));
			}
#if defined(DXX_BUILD_DESCENT_II)
			else if (!d_stricmp( arg, "speedvar" ))	{
				Weapon_info[n].speedvar = (atoi(equal_ptr) * 128) / 100;
			}
#endif
			else if (!d_stricmp( arg, "flash_vclip" ))	{
				Weapon_info[n].flash_vclip = build_vclip_index_from_untrusted(atoi(equal_ptr));
			} else if (!d_stricmp( arg, "flash_sound" ))	{
				Weapon_info[n].flash_sound = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "flash_size" ))	{
				Weapon_info[n].flash_size = fl2f(atof(equal_ptr));
			} else if (!d_stricmp( arg, "blob_size" ))	{
				Weapon_info[n].blob_size = fl2f(atof(equal_ptr));
			} else if (!d_stricmp( arg, "robot_hit_vclip" ))	{
				Weapon_info[n].robot_hit_vclip = build_vclip_index_from_untrusted(atoi(equal_ptr));
			} else if (!d_stricmp( arg, "robot_hit_sound" ))	{
				Weapon_info[n].robot_hit_sound = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "wall_hit_vclip" ))	{
				Weapon_info[n].wall_hit_vclip = build_vclip_index_from_untrusted(atoi(equal_ptr));
			} else if (!d_stricmp( arg, "wall_hit_sound" ))	{
				Weapon_info[n].wall_hit_sound = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "impact_size" ))	{
				Weapon_info[n].impact_size = fl2f(atof(equal_ptr));
			} else if (!d_stricmp( arg, "lighted" ))	{
				lighted = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "lw_ratio" ))	{
				Weapon_info[n].po_len_to_width_ratio = fl2f(atof(equal_ptr));
			} else if (!d_stricmp( arg, "lightcast" ))	{
				Weapon_info[n].light = fl2f(atof(equal_ptr));
			} else if (!d_stricmp( arg, "persistent" ))	{
				Weapon_info[n].persistent = atoi(equal_ptr) ? weapon_info::persistence_flag::persistent : weapon_info::persistence_flag::terminate_on_impact;
			} else if (!d_stricmp(arg, "energy_usage" )) {
				Weapon_info[n].energy_usage = fl2f(atof(equal_ptr));
			} else if (!d_stricmp(arg, "ammo_usage" )) {
				Weapon_info[n].ammo_usage = atoi(equal_ptr);
			} else if (!d_stricmp(arg, "fire_wait" )) {
				Weapon_info[n].fire_wait = fl2f(atof(equal_ptr));
			} else if (!d_stricmp(arg, "fire_count" )) {
				Weapon_info[n].fire_count = atoi(equal_ptr);
			} else if (!d_stricmp(arg, "damage_radius" )) {
				Weapon_info[n].damage_radius = fl2f(atof(equal_ptr));
//--01/19/95, mk--			} else if (!d_stricmp(arg, "damage_force" )) {
//--01/19/95, mk--				Weapon_info[n].damage_force = fl2f(atof(equal_ptr));
			} else if (!d_stricmp(arg, "lifetime" )) {
				Weapon_info[n].lifetime = fl2f(atof(equal_ptr));
			} else if (!d_stricmp(arg, "destroyable" )) {
				Weapon_info[n].destroyable = atoi(equal_ptr);
			} else if (!d_stricmp(arg, "picture" )) {
				Weapon_info[n].picture = bm_load_sub(skip, equal_ptr);
			}
#if defined(DXX_BUILD_DESCENT_II)
			else if (!d_stricmp(arg, "hires_picture" )) {
				Weapon_info[n].hires_picture = bm_load_sub(skip, equal_ptr);
			}
#endif
			else if (!d_stricmp(arg, "homing" )) {
				Weapon_info[n].homing_flag = !!atoi(equal_ptr);
			}
#if defined(DXX_BUILD_DESCENT_II)
			else if (!d_stricmp(arg, "flash" )) {
				Weapon_info[n].flash = atoi(equal_ptr);
			} else if (!d_stricmp(arg, "multi_damage_scale" )) {
				Weapon_info[n].multi_damage_scale = fl2f(atof(equal_ptr));
			} else if (!d_stricmp(arg, "afterburner_size" )) {
				Weapon_info[n].afterburner_size = f2i(16*fl2f(atof(equal_ptr)));
			} else if (!d_stricmp(arg, "children" )) {
				Weapon_info[n].children = static_cast<weapon_id_type>(atoi(equal_ptr));
			} else if (!d_stricmp(arg, "placable" )) {
				if (atoi(equal_ptr)) {
					Weapon_info[n].flags |= WIF_PLACABLE;
				}
			} else {
				Int3();
			}
#endif
		} else {			// Must be a texture specification...
			grs_bitmap *bm;

			bm = load_polymodel_bitmap(skip, arg);
#if defined(DXX_BUILD_DESCENT_I)
			if (bm && ! lighted)
#elif defined(DXX_BUILD_DESCENT_II)
			if (! lighted)
#endif
				bm->add_flags(BM_FLAG_NO_LIGHTING);

			lighted = 1;			//default for next bitmap is lighted
		}
		arg = strtok( NULL, space_tab );
	}

	first_bitmap_num[n_models] = N_ObjBitmapPtrs;

	auto &Polygon_models = LevelSharedPolygonModelState.Polygon_models;
	for (i=0;i<n_models;i++) {
		int n_textures;

		n_textures = first_bitmap_num[i+1] - first_bitmap_num[i];

		const auto model_num = load_polygon_model(model_name[i],n_textures,first_bitmap_num[i],NULL);

		if (i==0) {
			Weapon_info[n].render = WEAPON_RENDER_POLYMODEL;
			Weapon_info[n].model_num = model_num;
		}
		else
			Polygon_models.front().simpler_model = build_polygon_simpler_model_index_from_polygon_model_index(model_num);
	}

	if ( pof_file_inner )	{
		Assert(n_models);
		Weapon_info[n].model_num_inner = load_polygon_model(pof_file_inner,first_bitmap_num[1]-first_bitmap_num[0],first_bitmap_num[0],NULL);
	}
}

// ------------------------------------------------------------------------------
#define DEFAULT_POWERUP_SIZE i2f(3)

#if defined(DXX_BUILD_DESCENT_I)
void bm_read_powerup(char *&arg, int unused_flag)
#elif defined(DXX_BUILD_DESCENT_II)
void bm_read_powerup(int unused_flag)
#endif
{
	char 	*equal_ptr;

	Assert(N_powerup_types < MAX_POWERUP_TYPES);

	const auto n = N_powerup_types;
	N_powerup_types++;

	if (unused_flag) {
#if defined(DXX_BUILD_DESCENT_I)
		clear_to_end_of_line(arg);
#elif defined(DXX_BUILD_DESCENT_II)
		clear_to_end_of_line();
#endif
		return;
	}

	// Initialize powerup array
	const powerup_type_t pn{n};
	auto &pin = Powerup_info[pn];
	pin.light = F1_0 / 3;		//	Default lighting value.
	pin.vclip_num = vclip_index::None;
	pin.hit_sound = sound_none;
	pin.size = DEFAULT_POWERUP_SIZE;
#if DXX_USE_EDITOR
	auto &name = Powerup_names[pn];
	name[0] = 0;
#endif

	// Process arguments
	arg = strtok( NULL, space_tab );

	while (arg!=NULL)	{
		equal_ptr = strchr( arg, '=' );
		if ( equal_ptr )	{
			*equal_ptr='\0';
			equal_ptr++;
			// if we have john=cool, arg is 'john' and equal_ptr is 'cool'
			if (!d_stricmp( arg, "vclip_num" ))	{
				pin.vclip_num = build_vclip_index_from_untrusted(atoi(equal_ptr));
			} else if (!d_stricmp( arg, "light" ))	{
				pin.light = fl2f(atof(equal_ptr));
			} else if (!d_stricmp( arg, "hit_sound" ))	{
				pin.hit_sound = atoi(equal_ptr);
			} else if (!d_stricmp( arg, "name" )) {
#if DXX_USE_EDITOR
				const auto len = strlen(equal_ptr);
				assert(len < name.size());	//	Oops, name too long.
				memcpy(name.data(), &equal_ptr[1], len - 2);
				name[len - 2] = 0;
#endif
			} else if (!d_stricmp( arg, "size" ))	{
				pin.size = fl2f(atof(equal_ptr));
			}
#if defined(DXX_BUILD_DESCENT_II)
			else {
				Int3();
			}
#endif
		}
#if defined(DXX_BUILD_DESCENT_II)
		else {			// Must be a texture specification...
			Int3();
		}
#endif
		arg = strtok( NULL, space_tab );
	}
#if defined(DXX_BUILD_DESCENT_I)
	ObjType[Num_total_object_types] = OL_POWERUP;
	ObjId[Num_total_object_types] = static_cast<polygon_model_index>(n);
	Num_total_object_types++;
#endif
}

#if defined(DXX_BUILD_DESCENT_I)
void bm_read_hostage(char *&arg)
#elif defined(DXX_BUILD_DESCENT_II)
void bm_read_hostage()
#endif
{
	char 	*equal_ptr;

	Assert(N_hostage_types < MAX_HOSTAGE_TYPES);

	const auto n = N_hostage_types;
	N_hostage_types++;

	// Process arguments
	arg = strtok( NULL, space_tab );

	while (arg!=NULL)	{
		equal_ptr = strchr( arg, '=' );
		if ( equal_ptr )	{
			*equal_ptr='\0';
			equal_ptr++;

			if (!d_stricmp( arg, "vclip_num" ))

				Hostage_vclip_num[n] = build_vclip_index_from_untrusted(atoi(equal_ptr));

#if defined(DXX_BUILD_DESCENT_II)
			else {
				Int3();
			}
#endif
		}
#if defined(DXX_BUILD_DESCENT_II)
		else {
			Int3();
		}
#endif
		arg = strtok( NULL, space_tab );
	}
#if defined(DXX_BUILD_DESCENT_I)
	ObjType[Num_total_object_types] = OL_HOSTAGE;
	ObjId[Num_total_object_types] = static_cast<polygon_model_index>(n);
	Num_total_object_types++;
#endif
}

}

}

#if defined(DXX_BUILD_DESCENT_I)
DEFINE_SERIAL_UDT_TO_MESSAGE(tmap_info, t, (static_cast<const std::array<char, 13> &>(t.filename), t.flags, t.lighting, t.damage, t.eclip_num));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(tmap_info, 26);
#elif defined(DXX_BUILD_DESCENT_II)
DEFINE_SERIAL_UDT_TO_MESSAGE(tmap_info, t, (t.flags, serial::pad<3>(), t.lighting, t.damage, t.eclip_num, t.destroyed, t.slide_u, t.slide_v));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(tmap_info, 20);
#endif
