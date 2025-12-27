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
 * Functions for loading mines in the game
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
#include "gameseg.h"
#include "physfsx.h"
#include "switch.h"
#include "game.h"
#include "fuelcen.h"
#include "hash.h"
#include "piggy.h"
#include "gamesave.h"
#include "compiler-poison.h"
#include "compiler-range_for.h"
#include "d_levelstate.h"
#include "partial_range.h"

namespace dcx {

namespace {

static uint8_t New_file_format_load{1}; // "new file format" is everything newer than d1 shareware

}

}

namespace dsx {

namespace {

static segment_special build_segment_special_from_untrusted(uint8_t untrusted)
{
	switch (untrusted)
	{
		case static_cast<uint8_t>(segment_special::nothing):
		case static_cast<uint8_t>(segment_special::fuelcen):
		case static_cast<uint8_t>(segment_special::repaircen):
		case static_cast<uint8_t>(segment_special::controlcen):
		case static_cast<uint8_t>(segment_special::robotmaker):
#if DXX_BUILD_DESCENT == 2
		case static_cast<uint8_t>(segment_special::goal_blue):
		case static_cast<uint8_t>(segment_special::goal_red):
#endif
			return segment_special{untrusted};
		default:
			return segment_special::nothing;
	}
}

static materialization_center_number build_materialization_center_number_from_untrusted(uint8_t untrusted)
{
	if (const auto o = decltype(d_level_shared_robotcenter_state::RobotCenters)::valid_index(untrusted))
		return *o;
	else
		return materialization_center_number::None;
}

static station_number build_station_number_from_untrusted(uint8_t untrusted)
{
	if (const auto o = decltype(d_level_unique_fuelcenter_state::Station)::valid_index(untrusted))
		return *o;
	else
		return station_number::None;
}

/*
 * reads a segment2 structure from a PHYSFS_File
 */
static void segment2_read(const msmusegment s2, const NamedPHYSFS_File fp)
{
	s2.s.special = build_segment_special_from_untrusted(PHYSFSX_readByte(fp));
	s2.s.matcen_num = build_materialization_center_number_from_untrusted(PHYSFSX_readByte(fp));
	/* station_idx is overwritten by the caller in some cases, but set
	 * it here for compatibility with how the game previously worked */
	s2.s.station_idx = build_station_number_from_untrusted(PHYSFSX_readByte(fp));
	const auto s2_flags = PHYSFSX_readByte(fp);
	/* Ambient sounds are recomputed by the level load code.  Ignore the value
	 * read from the file.
	 */
	(void)s2_flags;
	s2.s.s2_flags = {};
	s2.u.static_light = PHYSFSX_readFix(fp);
}

}

#if DXX_BUILD_DESCENT == 1
#elif DXX_BUILD_DESCENT == 2

uint8_t d1_pig_present{0}; // can descent.pig from descent 1 be loaded?

namespace {

/* Converts descent 1 texture numbers to descent 2 texture numbers.
 * Textures from d1 which are unique to d1 have extra spaces around "return".
 * If we can load the original d1 pig, we make sure this function is bijective.
 * This function was updated using the file config/convtabl.ini from devil 2.2.
 */
static texture_index convert_d1_tmap_num_unrotated(const texture_index d1_tmap_num)
{
	switch (d1_tmap_num) {
	case 0: case 2: case 4: case 5:
		// all refer to grey rock001 (exception to bijectivity rule)
		return  d1_pig_present ? texture_index{137} : texture_index{43}; // (devil:95)
	case   1: return texture_index{0};
	case   3: return texture_index{1}; // rock021
	case   6:  return  texture_index{270}; // blue rock002
	case   7:  return  texture_index{271}; // yellow rock265
	case   8: return texture_index{2}; // rock004
	case   9:  return  d1_pig_present ? texture_index{138} : texture_index{62}; // purple (devil:179)
	case  10:  return  texture_index{272}; // red rock006
	case  11:  return  d1_pig_present ? texture_index{139} : texture_index{117};
	case  12:  return  d1_pig_present ? texture_index{140} : texture_index{12}; //devil:43
	case  13: return texture_index{3}; // rock014
	case  14: return texture_index{4}; // rock019
	case  15: return texture_index{5}; // rock020
	case  16: return texture_index{6};
	case  17:  return  d1_pig_present ? texture_index{141} : texture_index{52};
	case  18:  return  texture_index{129};
	case  19: return texture_index{7};
	case  20:  return  d1_pig_present ? texture_index{142} : texture_index{22};
	case  21:  return  d1_pig_present ? texture_index{143} : texture_index{9};
	case  22: return texture_index{8};
	case  23: return texture_index{9};
	case  24: return texture_index{10};
	case  25:  return  d1_pig_present ? texture_index{144} : texture_index{12}; //devil:35
	case  26: return texture_index{11};
	case  27: return texture_index{12};
	case  28:  return  d1_pig_present ? texture_index{145} : texture_index{11}; //devil:43
	//range handled by default case, returns 13..21 (- 16)
	case  38:  return  texture_index{163}; //devil:27
	case  39:  return  texture_index{147}; //31
	case  40: return texture_index{22};
	case  41:  return  texture_index{266};
	case  42: return texture_index{23};
	case  43: return texture_index{24};
	case  44:  return  texture_index{136}; //devil:135
	case  45: return texture_index{25};
	case  46: return texture_index{26};
	case  47: return texture_index{27};
	case  48: return texture_index{28};
	case  49:  return  d1_pig_present ? texture_index{146} : texture_index{43}; //devil:60
	case  50:  return  texture_index{131}; //devil:138
	case  51: return texture_index{29};
	case  52: return texture_index{30};
	case  53: return texture_index{31};
	case  54: return texture_index{32};
	case  55:  return  texture_index{165}; //devil:193
	case  56: return texture_index{33};
	case  57:  return  texture_index{132}; //devil:119
	// range handled by default case, returns 34..63 (- 24)
	case  88:  return  texture_index{197}; //devil:15
	// range handled by default case, returns 64..106 (- 25)
	case 132:  return  texture_index{167};
        // range handled by default case, returns 107..114 (- 26)
	case 141:  return  d1_pig_present ? texture_index{148} : texture_index{110}; //devil:106
	case 142: return texture_index{115};
	case 143: return texture_index{116};
	case 144: return texture_index{117};
	case 145: return texture_index{118};
	case 146: return texture_index{119};
	case 147:  return  d1_pig_present ? texture_index{149} : texture_index{93};
	case 148: return texture_index{120};
	case 149: return texture_index{121};
	case 150: return texture_index{122};
	case 151: return texture_index{123};
	case 152: return texture_index{124};
	case 153: return texture_index{125}; // rock263
	case 154:  return  d1_pig_present ? texture_index{150} : texture_index{27};
	case 155:  return  texture_index{126}; // rock269
	case 156: return texture_index{200}; // metl002
	case 157: return texture_index{201}; // metl003
	case 158:  return  texture_index{186}; //devil:227
	case 159:  return  texture_index{190}; //devil:246
	case 160:  return  d1_pig_present ? texture_index{151} : texture_index{206};
	case 161:  return  d1_pig_present ? texture_index{152} : texture_index{114}; //devil:206
	case 162: return texture_index{202};
	case 163: return texture_index{203};
	case 164: return texture_index{204};
	case 165: return texture_index{205};
	case 166: return texture_index{206};
	case 167:  return  d1_pig_present ? texture_index{153} : texture_index{206};
	case 168:  return  d1_pig_present ? texture_index{154} : texture_index{206};
	case 169:  return  d1_pig_present ? texture_index{155} : texture_index{206};
	case 170:  return  d1_pig_present ? texture_index{156} : texture_index{227};//206;
	case 171:  return  d1_pig_present ? texture_index{157} : texture_index{206};//227;
	case 172: return texture_index{207};
	case 173: return texture_index{208};
	case 174:  return  d1_pig_present ? texture_index{158} : texture_index{202};
	case 175:  return  d1_pig_present ? texture_index{159} : texture_index{206};
	// range handled by default case, returns 209..217 (+ 33)
	case 185:  return  d1_pig_present ? texture_index{160} : texture_index{217};
	// range handled by default case, returns 218..224 (+ 32)
	case 193:  return  d1_pig_present ? texture_index{161} : texture_index{206};
	case 194:  return  d1_pig_present ? texture_index{162} : texture_index{203};//206;
	case 195:  return  d1_pig_present ? texture_index{166} : texture_index{234};
	case 196: return texture_index{225};
	case 197: return texture_index{226};
	case 198:  return  d1_pig_present ? texture_index{193} : texture_index{225};
	case 199:  return  d1_pig_present ? texture_index{168} : texture_index{206}; //devil:204
	case 200:  return  d1_pig_present ? texture_index{169} : texture_index{206}; //devil:204
	case 201: return texture_index{227};
	case 202:  return  d1_pig_present ? texture_index{170} : texture_index{206}; //devil:227
	// range handled by default case, returns 228..234 (+ 25)
	case 210:  return  d1_pig_present ? texture_index{171} : texture_index{234}; //devil:242
	case 211:  return  d1_pig_present ? texture_index{172} : texture_index{206}; //devil:240
	// range handled by default case, returns 235..242 (+ 23)
	case 220:  return  d1_pig_present ? texture_index{173} : texture_index{242}; //devil:240
	case 221: return texture_index{243};
	case 222: return texture_index{244};
	case 223:  return  d1_pig_present ? texture_index{174} : texture_index{313};
	case 224: return texture_index{245};
	case 225: return texture_index{246};
	case 226:  return  texture_index{164};//247; matching names but not matching textures
	case 227:  return  texture_index{179}; //devil:181
	case 228:  return  texture_index{196};//248; matching names but not matching textures
	case 229:  return  d1_pig_present ? texture_index{175} : texture_index{15}; //devil:66
	case 230:  return  d1_pig_present ? texture_index{176} : texture_index{15}; //devil:66
	// range handled by default case, returns 249..257 (+ 18)
	case 240:  return  d1_pig_present ? texture_index{177} : texture_index{6}; //devil:132
	case 241:  return  texture_index{130}; //devil:131
	case 242:  return  d1_pig_present ? texture_index{178} : texture_index{78}; //devil:15
	case 243:  return  d1_pig_present ? texture_index{180} : texture_index{33}; //devil:38
	case 244: return texture_index{258};
	case 245: return texture_index{259};
	case 246:  return  d1_pig_present ? texture_index{181} : texture_index{321}; // grate metl127
	case 247: return texture_index{260};
	case 248: return texture_index{261};
	case 249: return texture_index{262};
	case 250:  return  texture_index{340}; //  white doorframe metl126
	case 251:  return  texture_index{412}; //    red doorframe metl133
	case 252:  return  texture_index{410}; //   blue doorframe metl134
	case 253:  return  texture_index{411}; // yellow doorframe metl135
	case 254: return texture_index{263}; // metl136
	case 255: return texture_index{264}; // metl139
	case 256: return texture_index{265}; // metl140
	case 257:  return  d1_pig_present ? texture_index{182} : texture_index{249};//246; brig001
	case 258:  return  d1_pig_present ? texture_index{183} : texture_index{251};//246; brig002
	case 259:  return  d1_pig_present ? texture_index{184} : texture_index{252};//246; brig003
	case 260:  return  d1_pig_present ? texture_index{185} : texture_index{256};//246; brig004
	case 261: return texture_index{273}; // exit01
	case 262: return texture_index{274}; // exit02
	case 263:  return  d1_pig_present ? texture_index{187} : texture_index{281}; // ceil001
	case 264: return texture_index{275}; // ceil002
	case 265: return texture_index{276}; // ceil003
	case 266:  return  d1_pig_present ? texture_index{188} : texture_index{279}; //devil:291
	// range handled by default case, returns 277..291 (+ 10)
	case 282: return texture_index{293};
	case 283:  return  d1_pig_present ? texture_index{189} : texture_index{295};
	case 284: return texture_index{295};
	case 285: return texture_index{296};
	case 286: return texture_index{298};
	// range handled by default case, returns 300..310 (+ 13)
	case 298:  return  d1_pig_present ? texture_index{191} : texture_index{364}; // devil:374 misc010
	// range handled by default case, returns 311..326 (+ 12)
	case 315:  return  d1_pig_present ? texture_index{192} : texture_index{361}; // bad producer misc044
	// range handled by default case,  returns  327..337 (+ 11)
	case 327: return texture_index{352}; // arw01
	case 328: return texture_index{353}; // misc17
	case 329: return texture_index{354}; // fan01
	case 330:  return  texture_index{380}; // mntr04
	case 331:  return  texture_index{379};//373; matching names but not matching textures
	case 332:  return  texture_index{355};//344; matching names but not matching textures
 	case 333:  return  texture_index{409}; // lava misc11 //devil:404
	case 334: return texture_index{356}; // ctrl04
	case 335: return texture_index{357}; // ctrl01
	case 336: return texture_index{358}; // ctrl02
	case 337: return texture_index{359}; // ctrl03
	case 338: return texture_index{360}; // misc14
	case 339: return texture_index{361}; // producer misc16
	case 340: return texture_index{362}; // misc049
	case 341: return texture_index{364}; // misc060
	case 342: return texture_index{363}; // blown01
	case 343: return texture_index{366}; // misc061
	case 344: return texture_index{365};
	case 345: return texture_index{368};
	case 346: return texture_index{376};
	case 347: return texture_index{370};
	case 348: return texture_index{367};
	case 349:  return  texture_index{372};
	case 350: return texture_index{369};
	case 351:  return  texture_index{374};//429; matching names but not matching textures
	case 352:  return  texture_index{375};//387; matching names but not matching textures
	case 353:  return  texture_index{371};
	case 354:  return  texture_index{377};//425; matching names but not matching textures
	case 355:  return  texture_index{408};
	case 356: return texture_index{378}; // lava02
	case 357:  return  texture_index{383};//384; matching names but not matching textures
	case 358:  return  texture_index{384};//385; matching names but not matching textures
	case 359:  return  texture_index{385};//386; matching names but not matching textures
	case 360: return texture_index{386};
	case 361: return texture_index{387};
	case 362:  return  d1_pig_present ? texture_index{194} : texture_index{388}; // mntr04b (devil: -1)
	case 363: return texture_index{388};
	case 364: return texture_index{391};
	case 365: return texture_index{392};
	case 366: return texture_index{393};
	case 367: return texture_index{394};
	case 368: return texture_index{395};
	case 369: return texture_index{396};
	case 370:  return  d1_pig_present ? texture_index{195} : texture_index{392}; // mntr04d (devil: -1)
	case 570: return texture_index{635};
	// range 371..584 handled by default case (wall01 and door frames)
	default:
		// ranges:
		if (d1_tmap_num >= 29 && d1_tmap_num <= 37)
			return static_cast<texture_index>(d1_tmap_num - 16);
		if (d1_tmap_num >= 58 && d1_tmap_num <= 87)
			return static_cast<texture_index>(d1_tmap_num - 24);
		if (d1_tmap_num >= 89 && d1_tmap_num <= 131)
			return static_cast<texture_index>(d1_tmap_num - 25);
		if (d1_tmap_num >= 133 && d1_tmap_num <= 140)
			return static_cast<texture_index>(d1_tmap_num - 26);
		if (d1_tmap_num >= 176 && d1_tmap_num <= 184)
			return static_cast<texture_index>(d1_tmap_num + 33);
		if (d1_tmap_num >= 186 && d1_tmap_num <= 192)
			return static_cast<texture_index>(d1_tmap_num + 32);
		if (d1_tmap_num >= 203 && d1_tmap_num <= 209)
			return static_cast<texture_index>(d1_tmap_num + 25);
		if (d1_tmap_num >= 212 && d1_tmap_num <= 219)
			return static_cast<texture_index>(d1_tmap_num + 23);
		if (d1_tmap_num >= 231 && d1_tmap_num <= 239)
			return static_cast<texture_index>(d1_tmap_num + 18);
		if (d1_tmap_num >= 267 && d1_tmap_num <= 281)
			return static_cast<texture_index>(d1_tmap_num + 10);
		if (d1_tmap_num >= 287 && d1_tmap_num <= 297)
			return static_cast<texture_index>(d1_tmap_num + 13);
		if (d1_tmap_num >= 299 && d1_tmap_num <= 314)
			return static_cast<texture_index>(d1_tmap_num + 12);
		if (d1_tmap_num >= 316 && d1_tmap_num <= 326)
			 return  static_cast<texture_index>(d1_tmap_num + 11); // matching names but not matching textures
		// wall01 and door frames:
		if (d1_tmap_num > 370 && d1_tmap_num < 584) {
			if (New_file_format_load) return static_cast<texture_index>(d1_tmap_num + 64);
			// d1 shareware needs special treatment:
			if (d1_tmap_num < 410) return static_cast<texture_index>(d1_tmap_num + 68);
			if (d1_tmap_num < 417) return static_cast<texture_index>(d1_tmap_num + 73);
			if (d1_tmap_num < 446) return static_cast<texture_index>(d1_tmap_num + 91);
			if (d1_tmap_num < 453) return static_cast<texture_index>(d1_tmap_num + 104);
			if (d1_tmap_num < 462) return static_cast<texture_index>(d1_tmap_num + 111);
			if (d1_tmap_num < 486) return static_cast<texture_index>(d1_tmap_num + 117);
			if (d1_tmap_num < 494) return static_cast<texture_index>(d1_tmap_num + 141);
			if (d1_tmap_num < 584) return static_cast<texture_index>(d1_tmap_num + 147);
		}
				Warning("Failed to convert unknown Descent 1 texture #%d.", d1_tmap_num);
				return d1_tmap_num;
	}
}

}

texture_index convert_d1_tmap_num(const texture_index d1_tmap_num)
{
	constexpr uint16_t TMAP_NUM_MASK{0x3fff};
	/* Mask off the rotation bits, if any.  Convert the base texture number.
	 * Restore the masked rotation bits.
	 */
	return static_cast<texture_index>(convert_d1_tmap_num_unrotated(static_cast<texture_index>(d1_tmap_num & TMAP_NUM_MASK)) | (d1_tmap_num & ~TMAP_NUM_MASK));
}
#endif

}

#if DXX_USE_EDITOR
namespace dsx {
tmap_xlate_table_array tmap_xlate_table;
}
struct mtfi mine_top_fileinfo; // Should be same as first two fields below...
struct mfi mine_fileinfo;
struct mh mine_header;
struct me mine_editor;

// -----------------------------------------------------------------------------
//loads from an already-open file
// returns 0=everything ok, 1=old version, -1=error
#endif

#define COMPILED_MINE_VERSION 0

namespace {

static void read_children(shared_segment &segp, const sidemask_t bit_mask, const NamedPHYSFS_File LoadFile)
{
	for (const auto bit : MAX_SIDES_PER_SEGMENT)
	{
		if (bit_mask & build_sidemask(bit))
		{
			const segnum_t child_segment{PHYSFSX_readULE16(LoadFile)};
			segp.children[bit] = unlikely(child_segment == segment_exit)
				? child_segment
				: vmsegidx_t::check_nothrow_index(child_segment).value_or(segment_none);
		} else
			segp.children[bit] = segment_none;
	}
}

static void read_verts(shared_segment &segp, const NamedPHYSFS_File LoadFile)
{
	// Read short Segments[segnum].verts[MAX_VERTICES_PER_SEGMENT]
	range_for (auto &v, segp.verts)
	{
		const std::size_t i{PHYSFSX_readULE16(LoadFile)};
		if (i >= MAX_VERTICES)
			throw std::invalid_argument("vertex number too large");
		v = static_cast<vertnum_t>(i);
	}
}

static void read_special(shared_segment &segp, const sidemask_t bit_mask, const NamedPHYSFS_File LoadFile)
{
	if (bit_mask & build_sidemask(MAX_SIDES_PER_SEGMENT))
	{
		// Read ubyte	Segments[segnum].special
		segp.special = build_segment_special_from_untrusted(PHYSFSX_readByte(LoadFile));
		// Read byte	Segments[segnum].matcen_num
		segp.matcen_num = build_materialization_center_number_from_untrusted(PHYSFSX_readByte(LoadFile));
		// Read short	Segments[segnum].value
		segp.station_idx = build_station_number_from_untrusted(PHYSFSX_readSLE16(LoadFile));
	} else {
		segp.special = segment_special::nothing;
		segp.matcen_num = materialization_center_number::None;
		segp.station_idx = station_number::None;
	}
}

}

namespace dsx {

int load_mine_data_compiled(const NamedPHYSFS_File LoadFile, const char *const Gamesave_current_filename)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	ubyte   compiled_version;
	short   temp_short;

#if DXX_BUILD_DESCENT == 2
	LevelSharedSeismicState.Level_shake_frequency = 0;
	LevelSharedSeismicState.Level_shake_duration = 0;
	d1_pig_present = PHYSFSX_exists_ignorecase(descent_pig_basename);
#endif
	if (!strcmp(strchr(Gamesave_current_filename, '.'), ".sdl"))
		New_file_format_load = 0; // descent 1 shareware
	else
		New_file_format_load = 1;

	//	For compiled levels, textures map to themselves, prevent tmap_override always being gray,
	//	bug which Matt and John refused to acknowledge, so here is Mike, fixing it.
	// 
	// Although in a cloud of arrogant glee, he forgot to ifdef it on EDITOR!
	// (Matt told me to write that!)
#if DXX_USE_EDITOR
	{
		constexpr std::size_t e{tmap_xlate_table.size()};
		for (std::size_t i{0}; i != e; ++i)
			/* Storing size_t into texture_index is a narrowing conversion, but
			 * the limited range of `i` ensures that the value is unchanged.
			 */
			tmap_xlate_table[i] = static_cast<texture_index>(i);
	}
#endif

//	memset( Segments, 0, sizeof(segment)*MAX_SEGMENTS );
	fuelcen_reset();

	//=============================== Reading part ==============================
	compiled_version = PHYSFSX_readByte(LoadFile);
	(void)compiled_version;

	DXX_POISON_VAR(Vertices, 0xfc);
	const unsigned Num_vertices = New_file_format_load
		? PHYSFSX_readSLE16(LoadFile)
		: PHYSFSX_readInt(LoadFile);
	assert(Num_vertices <= MAX_VERTICES);
#if DXX_USE_EDITOR
	LevelSharedVertexState.Num_vertices = Num_vertices;
#endif

	DXX_POISON_VAR(Segments, 0xfc);
	if (New_file_format_load)
		LevelSharedSegmentState.Num_segments = PHYSFSX_readSLE16(LoadFile);
	else
		LevelSharedSegmentState.Num_segments = PHYSFSX_readInt(LoadFile);
	assert(LevelSharedSegmentState.Num_segments <= MAX_SEGMENTS);

	range_for (auto &i, partial_range(Vertices, Num_vertices))
		PHYSFSX_readVector(LoadFile, i);

	const auto Num_segments = LevelSharedSegmentState.Num_segments;
	/* Editor builds need both the segment index and segment pointer.
	 * Non-editor builds need only the segment pointer.
	 */
#if DXX_USE_EDITOR
#define dxx_segment_iteration_factory vmptridx
#else
#define dxx_segment_iteration_factory vmptr
#endif
	for (auto &&segpi : partial_range(Segments.dxx_segment_iteration_factory, Num_segments))
#undef dxx_segment_iteration_factory
	{
		const msmusegment segp = segpi;

#if DXX_USE_EDITOR
		segp.s.segnum = segpi;
		segp.s.group = 0;
		#endif

		const sidemask_t children_mask = New_file_format_load
			? static_cast<sidemask_t>(PHYSFSX_readByte(LoadFile))
			: sidemask_t{0x7f};	// read all six children and special stuff...

		if (Gamesave_current_version == 5) { // d2 SHAREWARE level
			read_special(segp, children_mask, LoadFile);
			read_verts(segp,LoadFile);
			read_children(segp, children_mask, LoadFile);
		} else {
			read_children(segp, children_mask, LoadFile);
			read_verts(segp,LoadFile);
			if (Gamesave_current_version <= 1) { // descent 1 level
				read_special(segp, children_mask, LoadFile);
			}
		}

		segp.u.objects = object_none;

		if (Gamesave_current_version <= 5) { // descent 1 thru d2 SHAREWARE level
			// Read fix	Segments[segnum].static_light (shift down 5 bits, write as short)
			const uint16_t temp_static_light = PHYSFSX_readSLE16(LoadFile);
			segp.u.static_light = static_cast<fix>(temp_static_light) << 4;
		}

		// Read the walls as a 6 byte array
		const sidemask_t wall_mask = New_file_format_load
			? static_cast<sidemask_t>(PHYSFSX_readByte(LoadFile))
			: sidemask_t{0x3f}; // read all six sides
		for (const auto sidenum : MAX_SIDES_PER_SEGMENT)
		{
			auto &sside = segp.s.sides[sidenum];
			if (wall_mask & build_sidemask(sidenum))
			{
				const uint8_t byte_wallnum = PHYSFSX_readByte(LoadFile);
				if ( byte_wallnum == 255 )
					sside.wall_num = wall_none;
				else
					sside.wall_num = wallnum_t{byte_wallnum};
			} else
					sside.wall_num = wall_none;
		}

		for (const auto sidenum : MAX_SIDES_PER_SEGMENT)
		{
			auto &uside = segp.u.sides[sidenum];
			if (segp.s.children[sidenum] == segment_none || segp.s.sides[sidenum].wall_num != wall_none)	{
				// Read short Segments[segnum].sides[sidenum].tmap_num;
				const uint16_t temp_tmap1_num = PHYSFSX_readSLE16(LoadFile);
#if DXX_BUILD_DESCENT == 1
				uside.tmap_num = build_texture1_value(convert_tmap(static_cast<texture_index>(temp_tmap1_num & 0x7fff)));

				if (New_file_format_load && !(temp_tmap1_num & 0x8000))
					uside.tmap_num2 = texture2_value::None;
				else {
					// Read short Segments[segnum].sides[sidenum].tmap_num2;
					const auto tmap_num2 = texture2_value{static_cast<uint16_t>(PHYSFSX_readSLE16(LoadFile))};
					uside.tmap_num2 = build_texture2_value(convert_tmap(get_texture_index(tmap_num2)), get_texture_rotation_high(tmap_num2));
				}
#elif DXX_BUILD_DESCENT == 2
				const uint16_t masked_temp_tmap1_num = New_file_format_load ? (temp_tmap1_num & 0x7fff) : temp_tmap1_num;
				uside.tmap_num = build_texture1_value(masked_temp_tmap1_num < Textures.size() ? texture_index{masked_temp_tmap1_num} : texture_index{UINT16_MAX});

				if (Gamesave_current_version <= 1)
					uside.tmap_num = build_texture1_value(convert_d1_tmap_num(get_texture_index(uside.tmap_num)));

				if (New_file_format_load && !(temp_tmap1_num & 0x8000))
					uside.tmap_num2 = texture2_value::None;
				else {
					// Read short Segments[segnum].sides[sidenum].tmap_num2;
					const auto tmap_num2 = static_cast<texture2_value>(PHYSFSX_readSLE16(LoadFile));
					uside.tmap_num2 = (Gamesave_current_version <= 1 && tmap_num2 != texture2_value::None)
						? build_texture2_value(convert_d1_tmap_num(get_texture_index(tmap_num2)), get_texture_rotation_high(tmap_num2))
						: tmap_num2;
				}
#endif

				// Read uvl Segments[segnum].sides[sidenum].uvls[4] (u,v>>5, write as short, l>>1 write as short)
				range_for (auto &i, uside.uvls) {
					temp_short = PHYSFSX_readSLE16(LoadFile);
					i.u = static_cast<fix>(temp_short) << 5;
					temp_short = PHYSFSX_readSLE16(LoadFile);
					i.v = static_cast<fix>(temp_short) << 5;
					const uint16_t temp_light = PHYSFSX_readSLE16(LoadFile);
					i.l = static_cast<fix>(temp_light) << 1;
				}
			} else {
				uside.tmap_num = texture1_value::None;
				uside.tmap_num2 = texture2_value::None;
				uside.uvls = {};
			}
		}
	}

	Vertices.set_count(Num_vertices);
	Segments.set_count(Num_segments);

	validate_segment_all(LevelSharedSegmentState);			// Fill in side type and normals.

	range_for (const auto &&pi, vmsegptridx)
	{
		if (Gamesave_current_version > 5)
			segment2_read(pi, LoadFile);
		fuelcen_activate(pi);
	}

	reset_objects(LevelUniqueObjectState, 1);		//one object, the player

	return 0;
}
}
