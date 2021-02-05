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

#define REMOVE_EXT(s)  (*(strchr( (s), '.' ))='\0')

int New_file_format_load = 1; // "new file format" is everything newer than d1 shareware

namespace dsx {
namespace {

/*
 * reads a segment2 structure from a PHYSFS_File
 */
static void segment2_read(const msmusegment s2, PHYSFS_File *fp)
{
	s2.s.special = PHYSFSX_readByte(fp);
	s2.s.matcen_num = PHYSFSX_readByte(fp);
	/* station_idx is overwritten by the caller in some cases, but set
	 * it here for compatibility with how the game previously worked */
	s2.s.station_idx = PHYSFSX_readByte(fp);
	const auto s2_flags = PHYSFSX_readByte(fp);
#if defined(DXX_BUILD_DESCENT_I)
	(void)s2_flags;	// descent 2 ambient sound handling
	if (s2.s.special >= MAX_CENTER_TYPES)
		s2.s.special = SEGMENT_IS_NOTHING; // remove goals etc.
#elif defined(DXX_BUILD_DESCENT_II)
	s2.s.s2_flags = s2_flags;
#endif
	s2.u.static_light = PHYSFSX_readFix(fp);
}
}

#if defined(DXX_BUILD_DESCENT_I)
#elif defined(DXX_BUILD_DESCENT_II)

int d1_pig_present = 0; // can descent.pig from descent 1 be loaded?

/* Converts descent 1 texture numbers to descent 2 texture numbers.
 * Textures from d1 which are unique to d1 have extra spaces around "return".
 * If we can load the original d1 pig, we make sure this function is bijective.
 * This function was updated using the file config/convtabl.ini from devil 2.2.
 */
uint16_t convert_d1_tmap_num(const uint16_t d1_tmap_num)
{
	switch (d1_tmap_num) {
	case 0: case 2: case 4: case 5:
		// all refer to grey rock001 (exception to bijectivity rule)
		return  d1_pig_present ? 137 : 43; // (devil:95)
	case   1: return 0;
	case   3: return 1; // rock021
	case   6:  return  270; // blue rock002
	case   7:  return  271; // yellow rock265
	case   8: return 2; // rock004
	case   9:  return  d1_pig_present ? 138 : 62; // purple (devil:179)
	case  10:  return  272; // red rock006
	case  11:  return  d1_pig_present ? 139 : 117;
	case  12:  return  d1_pig_present ? 140 : 12; //devil:43
	case  13: return 3; // rock014
	case  14: return 4; // rock019
	case  15: return 5; // rock020
	case  16: return 6;
	case  17:  return  d1_pig_present ? 141 : 52;
	case  18:  return  129;
	case  19: return 7;
	case  20:  return  d1_pig_present ? 142 : 22;
	case  21:  return  d1_pig_present ? 143 : 9;
	case  22: return 8;
	case  23: return 9;
	case  24: return 10;
	case  25:  return  d1_pig_present ? 144 : 12; //devil:35
	case  26: return 11;
	case  27: return 12;
	case  28:  return  d1_pig_present ? 145 : 11; //devil:43
	//range handled by default case, returns 13..21 (- 16)
	case  38:  return  163; //devil:27
	case  39:  return  147; //31
	case  40: return 22;
	case  41:  return  266;
	case  42: return 23;
	case  43: return 24;
	case  44:  return  136; //devil:135
	case  45: return 25;
	case  46: return 26;
	case  47: return 27;
	case  48: return 28;
	case  49:  return  d1_pig_present ? 146 : 43; //devil:60
	case  50:  return  131; //devil:138
	case  51: return 29;
	case  52: return 30;
	case  53: return 31;
	case  54: return 32;
	case  55:  return  165; //devil:193
	case  56: return 33;
	case  57:  return  132; //devil:119
	// range handled by default case, returns 34..63 (- 24)
	case  88:  return  197; //devil:15
	// range handled by default case, returns 64..106 (- 25)
	case 132:  return  167;
        // range handled by default case, returns 107..114 (- 26)
	case 141:  return  d1_pig_present ? 148 : 110; //devil:106
	case 142: return 115;
	case 143: return 116;
	case 144: return 117;
	case 145: return 118;
	case 146: return 119;
	case 147:  return  d1_pig_present ? 149 : 93;
	case 148: return 120;
	case 149: return 121;
	case 150: return 122;
	case 151: return 123;
	case 152: return 124;
	case 153: return 125; // rock263
	case 154:  return  d1_pig_present ? 150 : 27;
	case 155:  return  126; // rock269
	case 156: return 200; // metl002
	case 157: return 201; // metl003
	case 158:  return  186; //devil:227
	case 159:  return  190; //devil:246
	case 160:  return  d1_pig_present ? 151 : 206;
	case 161:  return  d1_pig_present ? 152 : 114; //devil:206
	case 162: return 202;
	case 163: return 203;
	case 164: return 204;
	case 165: return 205;
	case 166: return 206;
	case 167:  return  d1_pig_present ? 153 : 206;
	case 168:  return  d1_pig_present ? 154 : 206;
	case 169:  return  d1_pig_present ? 155 : 206;
	case 170:  return  d1_pig_present ? 156 : 227;//206;
	case 171:  return  d1_pig_present ? 157 : 206;//227;
	case 172: return 207;
	case 173: return 208;
	case 174:  return  d1_pig_present ? 158 : 202;
	case 175:  return  d1_pig_present ? 159 : 206;
	// range handled by default case, returns 209..217 (+ 33)
	case 185:  return  d1_pig_present ? 160 : 217;
	// range handled by default case, returns 218..224 (+ 32)
	case 193:  return  d1_pig_present ? 161 : 206;
	case 194:  return  d1_pig_present ? 162 : 203;//206;
	case 195:  return  d1_pig_present ? 166 : 234;
	case 196: return 225;
	case 197: return 226;
	case 198:  return  d1_pig_present ? 193 : 225;
	case 199:  return  d1_pig_present ? 168 : 206; //devil:204
	case 200:  return  d1_pig_present ? 169 : 206; //devil:204
	case 201: return 227;
	case 202:  return  d1_pig_present ? 170 : 206; //devil:227
	// range handled by default case, returns 228..234 (+ 25)
	case 210:  return  d1_pig_present ? 171 : 234; //devil:242
	case 211:  return  d1_pig_present ? 172 : 206; //devil:240
	// range handled by default case, returns 235..242 (+ 23)
	case 220:  return  d1_pig_present ? 173 : 242; //devil:240
	case 221: return 243;
	case 222: return 244;
	case 223:  return  d1_pig_present ? 174 : 313;
	case 224: return 245;
	case 225: return 246;
	case 226:  return  164;//247; matching names but not matching textures
	case 227:  return  179; //devil:181
	case 228:  return  196;//248; matching names but not matching textures
	case 229:  return  d1_pig_present ? 175 : 15; //devil:66
	case 230:  return  d1_pig_present ? 176 : 15; //devil:66
	// range handled by default case, returns 249..257 (+ 18)
	case 240:  return  d1_pig_present ? 177 : 6; //devil:132
	case 241:  return  130; //devil:131
	case 242:  return  d1_pig_present ? 178 : 78; //devil:15
	case 243:  return  d1_pig_present ? 180 : 33; //devil:38
	case 244: return 258;
	case 245: return 259;
	case 246:  return  d1_pig_present ? 181 : 321; // grate metl127
	case 247: return 260;
	case 248: return 261;
	case 249: return 262;
	case 250:  return  340; //  white doorframe metl126
	case 251:  return  412; //    red doorframe metl133
	case 252:  return  410; //   blue doorframe metl134
	case 253:  return  411; // yellow doorframe metl135
	case 254: return 263; // metl136
	case 255: return 264; // metl139
	case 256: return 265; // metl140
	case 257:  return  d1_pig_present ? 182 : 249;//246; brig001
	case 258:  return  d1_pig_present ? 183 : 251;//246; brig002
	case 259:  return  d1_pig_present ? 184 : 252;//246; brig003
	case 260:  return  d1_pig_present ? 185 : 256;//246; brig004
	case 261: return 273; // exit01
	case 262: return 274; // exit02
	case 263:  return  d1_pig_present ? 187 : 281; // ceil001
	case 264: return 275; // ceil002
	case 265: return 276; // ceil003
	case 266:  return  d1_pig_present ? 188 : 279; //devil:291
	// range handled by default case, returns 277..291 (+ 10)
	case 282: return 293;
	case 283:  return  d1_pig_present ? 189 : 295;
	case 284: return 295;
	case 285: return 296;
	case 286: return 298;
	// range handled by default case, returns 300..310 (+ 13)
	case 298:  return  d1_pig_present ? 191 : 364; // devil:374 misc010
	// range handled by default case, returns 311..326 (+ 12)
	case 315:  return  d1_pig_present ? 192 : 361; // bad producer misc044
	// range handled by default case,  returns  327..337 (+ 11)
	case 327: return 352; // arw01
	case 328: return 353; // misc17
	case 329: return 354; // fan01
	case 330:  return  380; // mntr04
	case 331:  return  379;//373; matching names but not matching textures
	case 332:  return  355;//344; matching names but not matching textures
 	case 333:  return  409; // lava misc11 //devil:404
	case 334: return 356; // ctrl04
	case 335: return 357; // ctrl01
	case 336: return 358; // ctrl02
	case 337: return 359; // ctrl03
	case 338: return 360; // misc14
	case 339: return 361; // producer misc16
	case 340: return 362; // misc049
	case 341: return 364; // misc060
	case 342: return 363; // blown01
	case 343: return 366; // misc061
	case 344: return 365;
	case 345: return 368;
	case 346: return 376;
	case 347: return 370;
	case 348: return 367;
	case 349:  return  372;
	case 350: return 369;
	case 351:  return  374;//429; matching names but not matching textures
	case 352:  return  375;//387; matching names but not matching textures
	case 353:  return  371;
	case 354:  return  377;//425; matching names but not matching textures
	case 355:  return  408;
	case 356: return 378; // lava02
	case 357:  return  383;//384; matching names but not matching textures
	case 358:  return  384;//385; matching names but not matching textures
	case 359:  return  385;//386; matching names but not matching textures
	case 360: return 386;
	case 361: return 387;
	case 362:  return  d1_pig_present ? 194 : 388; // mntr04b (devil: -1)
	case 363: return 388;
	case 364: return 391;
	case 365: return 392;
	case 366: return 393;
	case 367: return 394;
	case 368: return 395;
	case 369: return 396;
	case 370:  return  d1_pig_present ? 195 : 392; // mntr04d (devil: -1)
	// range 371..584 handled by default case (wall01 and door frames)
	default:
		// ranges:
		if (d1_tmap_num >= 29 && d1_tmap_num <= 37)
			return d1_tmap_num - 16;
		if (d1_tmap_num >= 58 && d1_tmap_num <= 87)
			return d1_tmap_num - 24;
		if (d1_tmap_num >= 89 && d1_tmap_num <= 131)
			return d1_tmap_num - 25;
		if (d1_tmap_num >= 133 && d1_tmap_num <= 140)
			return d1_tmap_num - 26;
		if (d1_tmap_num >= 176 && d1_tmap_num <= 184)
			return d1_tmap_num + 33;
		if (d1_tmap_num >= 186 && d1_tmap_num <= 192)
			return d1_tmap_num + 32;
		if (d1_tmap_num >= 203 && d1_tmap_num <= 209)
			return d1_tmap_num + 25;
		if (d1_tmap_num >= 212 && d1_tmap_num <= 219)
			return d1_tmap_num + 23;
		if (d1_tmap_num >= 231 && d1_tmap_num <= 239)
			return d1_tmap_num + 18;
		if (d1_tmap_num >= 267 && d1_tmap_num <= 281)
			return d1_tmap_num + 10;
		if (d1_tmap_num >= 287 && d1_tmap_num <= 297)
			return d1_tmap_num + 13;
		if (d1_tmap_num >= 299 && d1_tmap_num <= 314)
			return d1_tmap_num + 12;
		if (d1_tmap_num >= 316 && d1_tmap_num <= 326)
			 return  d1_tmap_num + 11; // matching names but not matching textures
		// wall01 and door frames:
		if (d1_tmap_num > 370 && d1_tmap_num < 584) {
			if (New_file_format_load) return d1_tmap_num + 64;
			// d1 shareware needs special treatment:
			if (d1_tmap_num < 410) return d1_tmap_num + 68;
			if (d1_tmap_num < 417) return d1_tmap_num + 73;
			if (d1_tmap_num < 446) return d1_tmap_num + 91;
			if (d1_tmap_num < 453) return d1_tmap_num + 104;
			if (d1_tmap_num < 462) return d1_tmap_num + 111;
			if (d1_tmap_num < 486) return d1_tmap_num + 117;
			if (d1_tmap_num < 494) return d1_tmap_num + 141;
			if (d1_tmap_num < 584) return d1_tmap_num + 147;
		}
		{ // handle rare case where orientation != 0
			short tmap_num = d1_tmap_num &  TMAP_NUM_MASK;
			short orient = d1_tmap_num & ~TMAP_NUM_MASK;
			if (orient != 0) {
				return orient | convert_d1_tmap_num(tmap_num);
			} else {
				Warning("can't convert unknown descent 1 texture #%d.\n", tmap_num);
				return d1_tmap_num;
			}
		}
	}
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

static void read_children(shared_segment &segp, const unsigned bit_mask, PHYSFS_File *const LoadFile)
{
	for (int bit=0; bit<MAX_SIDES_PER_SEGMENT; bit++) {
		if (bit_mask & (1 << bit)) {
			segp.children[bit] = PHYSFSX_readShort(LoadFile);
		} else
			segp.children[bit] = segment_none;
	}
}

static void read_verts(shared_segment &segp, PHYSFS_File *const LoadFile)
{
	// Read short Segments[segnum].verts[MAX_VERTICES_PER_SEGMENT]
	range_for (auto &v, segp.verts)
	{
		const std::size_t i = PHYSFSX_readShort(LoadFile);
		if (i >= MAX_VERTICES)
			throw std::invalid_argument("vertex number too large");
		v = static_cast<vertnum_t>(i);
	}
}

static void read_special(shared_segment &segp, const unsigned bit_mask, PHYSFS_File *const LoadFile)
{
	if (bit_mask & (1 << MAX_SIDES_PER_SEGMENT)) {
		// Read ubyte	Segments[segnum].special
		segp.special = PHYSFSX_readByte(LoadFile);
		// Read byte	Segments[segnum].matcen_num
		segp.matcen_num = PHYSFSX_readByte(LoadFile);
		// Read short	Segments[segnum].value
		segp.station_idx = PHYSFSX_readShort(LoadFile);
	} else {
		segp.special = 0;
		segp.matcen_num = -1;
		segp.station_idx = station_none;
	}
}

}

namespace dsx {

int load_mine_data_compiled(PHYSFS_File *LoadFile, const char *const Gamesave_current_filename)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	ubyte   compiled_version;
	short   temp_short;
	ubyte   bit_mask;

#if defined(DXX_BUILD_DESCENT_II)
	LevelSharedSeismicState.Level_shake_frequency = 0;
	LevelSharedSeismicState.Level_shake_duration = 0;
	d1_pig_present = PHYSFSX_exists(D1_PIGFILE,1);
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
	for (int i=0; i<MAX_TEXTURES; i++)
		tmap_xlate_table[i] = i;
#endif

//	memset( Segments, 0, sizeof(segment)*MAX_SEGMENTS );
	fuelcen_reset();

	//=============================== Reading part ==============================
	compiled_version = PHYSFSX_readByte(LoadFile);
	(void)compiled_version;

	DXX_POISON_VAR(Vertices, 0xfc);
	const unsigned Num_vertices = New_file_format_load
		? PHYSFSX_readShort(LoadFile)
		: PHYSFSX_readInt(LoadFile);
	assert(Num_vertices <= MAX_VERTICES);
#if DXX_USE_EDITOR
	LevelSharedVertexState.Num_vertices = Num_vertices;
#endif

	DXX_POISON_VAR(Segments, 0xfc);
	if (New_file_format_load)
		LevelSharedSegmentState.Num_segments = PHYSFSX_readShort(LoadFile);
	else
		LevelSharedSegmentState.Num_segments = PHYSFSX_readInt(LoadFile);
	assert(LevelSharedSegmentState.Num_segments <= MAX_SEGMENTS);

	range_for (auto &i, partial_range(Vertices, Num_vertices))
		PHYSFSX_readVector(LoadFile, i);

	const auto Num_segments = LevelSharedSegmentState.Num_segments;
	for (segnum_t segnum=0; segnum < Num_segments; segnum++ )	{
		const msmusegment segp = vmsegptr(segnum);

#if DXX_USE_EDITOR
		segp.s.segnum = segnum;
		segp.s.group = 0;
		#endif

		if (New_file_format_load)
			bit_mask = PHYSFSX_readByte(LoadFile);
		else
			bit_mask = 0x7f; // read all six children and special stuff...

		if (Gamesave_current_version == 5) { // d2 SHAREWARE level
			read_special(segp,bit_mask,LoadFile);
			read_verts(segp,LoadFile);
			read_children(segp,bit_mask,LoadFile);
		} else {
			read_children(segp,bit_mask,LoadFile);
			read_verts(segp,LoadFile);
			if (Gamesave_current_version <= 1) { // descent 1 level
				read_special(segp,bit_mask,LoadFile);
			}
		}

		segp.u.objects = object_none;

		if (Gamesave_current_version <= 5) { // descent 1 thru d2 SHAREWARE level
			// Read fix	Segments[segnum].static_light (shift down 5 bits, write as short)
			const uint16_t temp_static_light = PHYSFSX_readShort(LoadFile);
			segp.u.static_light = static_cast<fix>(temp_static_light) << 4;
		}

		// Read the walls as a 6 byte array
		if (New_file_format_load)
			bit_mask = PHYSFSX_readByte(LoadFile);
		else
			bit_mask = 0x3f; // read all six sides
		for (int sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++) {

			auto &sside = segp.s.sides[sidenum];
			if (bit_mask & (1 << sidenum)) {
				const uint8_t byte_wallnum = PHYSFSX_readByte(LoadFile);
				if ( byte_wallnum == 255 )
					sside.wall_num = wall_none;
				else
					sside.wall_num = wallnum_t{byte_wallnum};
			} else
					sside.wall_num = wall_none;
		}

		for (int sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++ ) {
			auto &uside = segp.u.sides[sidenum];
			if (segp.s.children[sidenum] == segment_none || segp.s.sides[sidenum].wall_num != wall_none)	{
				// Read short Segments[segnum].sides[sidenum].tmap_num;
				const uint16_t temp_tmap1_num = PHYSFSX_readShort(LoadFile);
#if defined(DXX_BUILD_DESCENT_I)
				uside.tmap_num = build_texture1_value(convert_tmap(temp_tmap1_num & 0x7fff));

				if (New_file_format_load && !(temp_tmap1_num & 0x8000))
					uside.tmap_num2 = texture2_value::None;
				else {
					// Read short Segments[segnum].sides[sidenum].tmap_num2;
					const auto tmap_num2 = texture2_value{static_cast<uint16_t>(PHYSFSX_readShort(LoadFile))};
					uside.tmap_num2 = build_texture2_value(convert_tmap(get_texture_index(tmap_num2)), get_texture_rotation_high(tmap_num2));
				}
#elif defined(DXX_BUILD_DESCENT_II)
				const uint16_t masked_temp_tmap1_num = New_file_format_load ? (temp_tmap1_num & 0x7fff) : temp_tmap1_num;
				uside.tmap_num = build_texture1_value(masked_temp_tmap1_num);

				if (Gamesave_current_version <= 1)
					uside.tmap_num = build_texture1_value(convert_d1_tmap_num(get_texture_index(uside.tmap_num)));

				if (New_file_format_load && !(temp_tmap1_num & 0x8000))
					uside.tmap_num2 = texture2_value::None;
				else {
					// Read short Segments[segnum].sides[sidenum].tmap_num2;
					const auto tmap_num2 = static_cast<texture2_value>(PHYSFSX_readShort(LoadFile));
					uside.tmap_num2 = (Gamesave_current_version <= 1 && tmap_num2 != texture2_value::None)
						? build_texture2_value(convert_d1_tmap_num(get_texture_index(tmap_num2)), get_texture_rotation_high(tmap_num2))
						: tmap_num2;
				}
#endif

				// Read uvl Segments[segnum].sides[sidenum].uvls[4] (u,v>>5, write as short, l>>1 write as short)
				range_for (auto &i, uside.uvls) {
					temp_short = PHYSFSX_readShort(LoadFile);
					i.u = static_cast<fix>(temp_short) << 5;
					temp_short = PHYSFSX_readShort(LoadFile);
					i.v = static_cast<fix>(temp_short) << 5;
					const uint16_t temp_light = PHYSFSX_readShort(LoadFile);
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
