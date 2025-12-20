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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Mine specific editing functions, such as load_mine, save_mine
 *
 */

#include <cinttypes>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "gr.h"
#include "bm.h"			// for MAX_TEXTURES
#include "inferno.h"
#include "segment.h"
#include "editor.h"
#include "editor/esegment.h"
#include "wall.h"
#include "dxxerror.h"
#include "textures.h"
#include "physfsx.h"
#include "gamemine.h"
#include "gamesave.h"
#include "switch.h"
#include "fuelcen.h"

#include "compiler-range_for.h"
#include "d_enumerate.h"
#include "d_range.h"
#include "d_underlying_value.h"
#include "partial_range.h"

int New_file_format_save{1};

#if DXX_BUILD_DESCENT == 2
namespace dsx {

namespace {

// Converts descent 2 texture numbers back to descent 1 texture numbers.
// Only works properly when the full Descent 1 texture set (descent.pig) is available.
static texture_index convert_to_d1_tmap_num(const texture_index tmap_num)
{
	switch (tmap_num)
	{
		case 137: return   texture_index{0}; // grey rock001
		case   0: return   texture_index{1};
		case   1: return   texture_index{3}; // rock021
		case 270: return   texture_index{6}; // blue rock002
		case 271: return   texture_index{7}; // yellow rock265
		case   2: return   texture_index{8}; // rock004
		case 138: return   texture_index{9}; // purple (devil:179)
		case 272: return  texture_index{10}; // red rock006
		case 139: return  texture_index{11};
		case 140: return  texture_index{12}; //devil:43
		case   3: return  texture_index{13}; // rock014
		case   4: return  texture_index{14}; // rock019
		case   5: return  texture_index{15}; // rock020
		case   6: return  texture_index{16};
		case 141: return  texture_index{17};
		case 129: return  texture_index{18};
		case   7: return  texture_index{19};
		case 142: return  texture_index{20};
		case 143: return  texture_index{21};
		case   8: return  texture_index{22};
		case   9: return  texture_index{23};
		case  10: return  texture_index{24};
		case 144: return  texture_index{25}; //devil:35
		case  11: return  texture_index{26};
		case  12: return  texture_index{27};
		case 145: return  texture_index{28}; //devil:43
		//range handled by default case, returns 13..21 (- 16)
		case 163: return  texture_index{38}; //devil:27
		case 147: return  texture_index{39}; //31
		case  22: return  texture_index{40};
		case 266: return  texture_index{41};
		case  23: return  texture_index{42};
		case  24: return  texture_index{43};
		case 136: return  texture_index{44}; //devil:135
		case  25: return  texture_index{45};
		case  26: return  texture_index{46};
		case  27: return  texture_index{47};
		case  28: return  texture_index{48};
		case 146: return  texture_index{49}; //devil:60
		case 131: return  texture_index{50}; //devil:138
		case  29: return  texture_index{51};
		case  30: return  texture_index{52};
		case  31: return  texture_index{53};
		case  32: return  texture_index{54};
		case 165: return  texture_index{55}; //devil:193
		case  33: return  texture_index{56};
		case 132: return  texture_index{57}; //devil:119
		// range handled by default case, returns 58..88 (+ 24)
		case 197: return  texture_index{88}; //devil:15
		// range handled by default case, returns 89..106 (- 25)
		case 167: return texture_index{132};
		// range handled by default case, returns 107..114 (- 26)
		case 148: return texture_index{141}; //devil:106
		case 115: return texture_index{142};
		case 116: return texture_index{143};
		case 117: return texture_index{144};
		case 118: return texture_index{145};
		case 119: return texture_index{146};
		case 149: return texture_index{147};
		case 120: return texture_index{148};
		case 121: return texture_index{149};
		case 122: return texture_index{150};
		case 123: return texture_index{151};
		case 124: return texture_index{152};
		case 125: return texture_index{153}; // rock263
		case 150: return texture_index{154};
		case 126: return texture_index{155}; // rock269
		case 200: return texture_index{156}; // metl002
		case 201: return texture_index{157}; // metl003
		case 186: return texture_index{158}; //devil:227
		case 190: return texture_index{159}; //devil:246
		case 151: return texture_index{160};
		case 152: return texture_index{161}; //devil:206
		case 202: return texture_index{162};
		case 203: return texture_index{163};
		case 204: return texture_index{164};
		case 205: return texture_index{165};
		case 206: return texture_index{166};
		case 153: return texture_index{167};
		case 154: return texture_index{168};
		case 155: return texture_index{169};
		case 156: return texture_index{170};//206;
		case 157: return texture_index{171};//227;
		case 207: return texture_index{172};
		case 208: return texture_index{173};
		case 158: return texture_index{174};
		case 159: return texture_index{175};
		// range handled by default case, returns 209..217 (+ 33)
		case 160: return texture_index{185};
		// range handled by default case, returns 218..224 (+ 32)
		case 161: return texture_index{193};
		case 162: return texture_index{194};//206;
		case 166: return texture_index{195};
		case 225: return texture_index{196};
		case 226: return texture_index{197};
		case 193: return texture_index{198};
		case 168: return texture_index{199}; //devil:204
		case 169: return texture_index{200}; //devil:204
		case 227: return texture_index{201};
		case 170: return texture_index{202}; //devil:227
		// range handled by default case, returns 228..234 (+ 25)
		case 171: return texture_index{210}; //devil:242
		case 172: return texture_index{211}; //devil:240
		// range handled by default case, returns 235..242 (+ 23)
		case 173: return texture_index{220}; //devil:240
		case 243: return texture_index{221};
		case 244: return texture_index{222};
		case 174: return texture_index{223};
		case 245: return texture_index{224};
		case 246: return texture_index{225};
		case 164: return texture_index{226};//247; matching names but not matching textures
		case 179: return texture_index{227}; //devil:181
		case 196: return texture_index{228};//248; matching names but not matching textures
		case 175: return texture_index{229}; //devil:66
		case 176: return texture_index{230}; //devil:66
		// range handled by default case, returns 249..257 (+ 18)
		case 177: return texture_index{240}; //devil:132
		case 130: return texture_index{241}; //devil:131
		case 178: return texture_index{242}; //devil:15
		case 180: return texture_index{243}; //devil:38
		case 258: return texture_index{244};
		case 259: return texture_index{245};
		case 181: return texture_index{246}; // grate metl127
		case 260: return texture_index{247};
		case 261: return texture_index{248};
		case 262: return texture_index{249};
		case 340: return texture_index{250}; //  white doorframe metl126
		case 412: return texture_index{251}; //    red doorframe metl133
		case 410: return texture_index{252}; //   blue doorframe metl134
		case 411: return texture_index{253}; // yellow doorframe metl135
		case 263: return texture_index{254}; // metl136
		case 264: return texture_index{255}; // metl139
		case 265: return texture_index{256}; // metl140
		case 182: return texture_index{257};//246; brig001
		case 183: return texture_index{258};//246; brig002
		case 184: return texture_index{259};//246; brig003
		case 185: return texture_index{260};//246; brig004
		case 273: return texture_index{261}; // exit01
		case 274: return texture_index{262}; // exit02
		case 187: return texture_index{263}; // ceil001
		case 275: return texture_index{264}; // ceil002
		case 276: return texture_index{265}; // ceil003
		case 188: return texture_index{266}; //devil:291
		// range handled by default case, returns 277..291 (+ 10)
		case 293: return texture_index{282};
		case 189: return texture_index{283};
		case 295: return texture_index{284};
		case 296: return texture_index{285};
		case 298: return texture_index{286};
		// range handled by default case, returns 300..310 (+ 13)
		case 191: return texture_index{298}; // devil:374 misc010
		// range handled by default case, returns 311..326 (+ 12)
		case 192: return texture_index{315}; // bad producer misc044
		// range handled by default case,  returns  327..337 (+ 11)
		case 352: return texture_index{327}; // arw01
		case 353: return texture_index{328}; // misc17
		case 354: return texture_index{329}; // fan01
		case 380: return texture_index{330}; // mntr04
		case 379: return texture_index{331};//373; matching names but not matching textures
		case 355: return texture_index{332};//344; matching names but not matching textures
		case 409: return texture_index{333}; // lava misc11 //devil:404
		case 356: return texture_index{334}; // ctrl04
		case 357: return texture_index{335}; // ctrl01
		case 358: return texture_index{336}; // ctrl02
		case 359: return texture_index{337}; // ctrl03
		case 360: return texture_index{338}; // misc14
		case 361: return texture_index{339}; // producer misc16
		case 362: return texture_index{340}; // misc049
		case 364: return texture_index{341}; // misc060
		case 363: return texture_index{342}; // blown01
		case 366: return texture_index{343}; // misc061
		case 365: return texture_index{344};
		case 368: return texture_index{345};
		case 376: return texture_index{346};
		case 370: return texture_index{347};
		case 367: return texture_index{348};
		case 372: return texture_index{349};
		case 369: return texture_index{350};
		case 374: return texture_index{351};//429; matching names but not matching textures
		case 375: return texture_index{352};//387; matching names but not matching textures
		case 371: return texture_index{353};
		case 377: return texture_index{354};//425; matching names but not matching textures
		case 408: return texture_index{355};
		case 378: return texture_index{356}; // lava02
		case 383: return texture_index{357};//384; matching names but not matching textures
		case 384: return texture_index{358};//385; matching names but not matching textures
		case 385: return texture_index{359};//386; matching names but not matching textures
		case 386: return texture_index{360};
		case 387: return texture_index{361};
		case 194: return texture_index{362}; // mntr04b (devil: -1)
		case 388: return texture_index{363};
		case 391: return texture_index{364};
		case 392: return texture_index{365};
		case 393: return texture_index{366};
		case 394: return texture_index{367};
		case 395: return texture_index{368};
		case 396: return texture_index{369};
		case 195: return texture_index{370}; // mntr04d (devil: -1)
		// range 371..584 handled by default case (wall01 and door frames)
		default:
			// ranges:
			if (tmap_num >= 13 && tmap_num <= 21)
				return static_cast<texture_index>(tmap_num + 16);
			if (tmap_num >= 34 && tmap_num <= 63)
				return static_cast<texture_index>(tmap_num + 24);
			if (tmap_num >= 64 && tmap_num <= 106)
				return static_cast<texture_index>(tmap_num + 25);
			if (tmap_num >= 107 && tmap_num <= 114)
				return static_cast<texture_index>(tmap_num + 26);
			if (tmap_num >= 209 && tmap_num <= 217)
				return static_cast<texture_index>(tmap_num - 33);
			if (tmap_num >= 218 && tmap_num <= 224)
				return static_cast<texture_index>(tmap_num - 32);
			if (tmap_num >= 228 && tmap_num <= 234)
				return static_cast<texture_index>(tmap_num - 25);
			if (tmap_num >= 235 && tmap_num <= 242)
				return static_cast<texture_index>(tmap_num - 23);
			if (tmap_num >= 249 && tmap_num <= 257)
				return static_cast<texture_index>(tmap_num - 18);
			if (tmap_num >= 277 && tmap_num <= 291)
				return static_cast<texture_index>(tmap_num - 10);
			if (tmap_num >= 300 && tmap_num <= 310)
				return static_cast<texture_index>(tmap_num - 13);
			if (tmap_num >= 311 && tmap_num <= 326)
				return static_cast<texture_index>(tmap_num - 12);
			if (tmap_num >= 327 && tmap_num <= 337)
				return static_cast<texture_index>(tmap_num - 11); // matching names but not matching textures
			// wall01 and door frames:
			if (tmap_num > 434 && tmap_num < 731)
			{
				if (New_file_format_save) return static_cast<texture_index>(tmap_num - 64);
				// d1 shareware needs special treatment:
				if (tmap_num < 478) return static_cast<texture_index>(tmap_num - 68);
				if (tmap_num < 490) return static_cast<texture_index>(tmap_num - 73);
				if (tmap_num < 537) return static_cast<texture_index>(tmap_num - 91);
				if (tmap_num < 557) return static_cast<texture_index>(tmap_num - 104);
				if (tmap_num < 573) return static_cast<texture_index>(tmap_num - 111);
				if (tmap_num < 603) return static_cast<texture_index>(tmap_num - 117);
				if (tmap_num < 635) return static_cast<texture_index>(tmap_num - 141);
				if (tmap_num < 731) return static_cast<texture_index>(tmap_num - 147);
			}
			Warning("Failed to convert unknown texture #%hu to Descent 1 texture.", tmap_num);
			return tmap_num;
	}
}

}

}
#endif

namespace {

static int save_mine_data(PHYSFS_File * SaveFile);

// -----------------------------------------------------------------------------
// Save mine will:
// 1. Write file info, header info, editor info, vertex data, segment data,
//    and new_segment in that order, marking their file offset.
// 2. Go through all the fields and fill in the offset, size, and sizeof
//    values in the headers.

static int med_save_mine(const char * filename)
{
	char ErrorMessage[256];

	auto &&[SaveFile, physfserr] = PHYSFSX_openWriteBuffered(filename);
	if (!SaveFile)
	{
		snprintf(ErrorMessage, sizeof(ErrorMessage), "ERROR: Unable to open %.200s: %s\n", filename, PHYSFS_getErrorByCode(physfserr));
		ui_messagebox( -2, -2, 1, ErrorMessage, "Ok" );
		return 1;
	}

	save_mine_data(SaveFile);
	
	//==================== CLOSE THE FILE =============================
	return 0;
}

// -----------------------------------------------------------------------------
// saves to an already-open file
static int save_mine_data(PHYSFS_File * SaveFile)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	int  header_offset, editor_offset, vertex_offset, segment_offset, texture_offset, walls_offset, triggers_offset; //, links_offset;
	int  newseg_verts_offset;
	int  newsegment_offset;
	med_compress_mine();
	warn_if_concave_segments();

	//=================== Calculate offsets into file ==================

	header_offset = PHYSFS_tell(SaveFile) + sizeof(mine_fileinfo);
	editor_offset = header_offset + sizeof(mine_header);
	texture_offset = editor_offset + sizeof(mine_editor);
	vertex_offset  = texture_offset + (13*NumTextures);
	segment_offset = vertex_offset + (sizeof(vms_vector) * LevelSharedVertexState.Num_vertices);
	newsegment_offset = segment_offset + (sizeof(segment) * LevelSharedSegmentState.Num_segments);
	newseg_verts_offset = newsegment_offset + sizeof(segment);
	walls_offset = newseg_verts_offset + (sizeof(vms_vector)*8);
	auto &Triggers = LevelUniqueWallSubsystemState.Triggers;
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	triggers_offset =	walls_offset + (sizeof(wall)*Walls.get_count());

	//===================== SAVE FILE INFO ========================

	mine_fileinfo.fileinfo_signature=	0x2884;
	mine_fileinfo.fileinfo_version  =   MINE_VERSION;
	mine_fileinfo.fileinfo_sizeof   =   sizeof(mine_fileinfo);
	mine_fileinfo.header_offset     =   header_offset;
	mine_fileinfo.header_size       =   sizeof(mine_header);
	mine_fileinfo.editor_offset     =   editor_offset;
	mine_fileinfo.editor_size       =   sizeof(mine_editor);
	mine_fileinfo.vertex_offset     =   vertex_offset;
	mine_fileinfo.vertex_howmany    =   LevelSharedVertexState.Num_vertices;
	mine_fileinfo.vertex_sizeof     =   sizeof(vms_vector);
	mine_fileinfo.segment_offset    =   segment_offset;
	mine_fileinfo.segment_howmany   =   LevelSharedSegmentState.Num_segments;
	mine_fileinfo.segment_sizeof    =   sizeof(segment);
	mine_fileinfo.newseg_verts_offset     =   newseg_verts_offset;
	mine_fileinfo.newseg_verts_howmany    =   8;
	mine_fileinfo.newseg_verts_sizeof     =   sizeof(vms_vector);
	mine_fileinfo.texture_offset    =   texture_offset;
	mine_fileinfo.texture_howmany   =   NumTextures;
	mine_fileinfo.texture_sizeof    =   13;  // num characters in a name
	mine_fileinfo.walls_offset		  =	walls_offset;
	mine_fileinfo.walls_howmany	  =	Walls.get_count();
	mine_fileinfo.walls_sizeof		  =	sizeof(wall);  
	mine_fileinfo.triggers_offset	  =	triggers_offset;
	mine_fileinfo.triggers_howmany  =	Triggers.get_count();
	mine_fileinfo.triggers_sizeof	  =	sizeof(trigger);  

	// Write the fileinfo
	PHYSFSX_writeBytes(SaveFile, &mine_fileinfo, sizeof(mine_fileinfo));

	//===================== SAVE HEADER INFO ========================

	mine_header.num_vertices        =   LevelSharedVertexState.Num_vertices;
	mine_header.num_segments        =   LevelSharedSegmentState.Num_segments;

	// Write the editor info
	if (header_offset != PHYSFS_tell(SaveFile))
		Error( "OFFSETS WRONG IN MINE.C!" );

	PHYSFSX_writeBytes(SaveFile, &mine_header, sizeof(mine_header));

	//===================== SAVE EDITOR INFO ==========================
	mine_editor.current_seg         =   Cursegp;
	mine_editor.newsegment_offset   =   newsegment_offset; 
	mine_editor.newsegment_size     =   sizeof(segment);

	// Next 3 vars added 10/07 by JAS
	mine_editor.Curside             =   underlying_value(Curside);
	if (Markedsegp)
		mine_editor.Markedsegp      =   Markedsegp;
	else									  
		mine_editor.Markedsegp       =   -1;
	mine_editor.Markedside          =   underlying_value(Markedside);
	range_for (const int i, xrange(10u))
		mine_editor.Groupsegp[i]	  =	vmsegptridx(Groupsegp[i]);
	range_for (const int i, xrange(10u))
		mine_editor.Groupside[i] = underlying_value(Groupside[i]);

	if (editor_offset != PHYSFS_tell(SaveFile))
		Error( "OFFSETS WRONG IN MINE.C!" );
	PHYSFSX_writeBytes(SaveFile, &mine_editor, sizeof(mine_editor));

	//===================== SAVE TEXTURE INFO ==========================

	if (texture_offset != PHYSFS_tell(SaveFile))
		Error( "OFFSETS WRONG IN MINE.C!" );

	for (const auto &ti : std::span(LevelUniqueTmapInfoState.TmapInfo).first(NumTextures))
		PHYSFSX_writeBytes(SaveFile, ti.filename.data(), ti.filename.size());
	
	//===================== SAVE VERTEX INFO ==========================

	if (vertex_offset != PHYSFS_tell(SaveFile))
		Error( "OFFSETS WRONG IN MINE.C!" );
	auto &Vertices = LevelSharedVertexState.get_vertices();
	PHYSFSX_writeBytes(SaveFile, Vertices, sizeof(vms_vector) * LevelSharedVertexState.Num_vertices);

	//===================== SAVE SEGMENT INFO =========================

	if (segment_offset != PHYSFS_tell(SaveFile))
		Error( "OFFSETS WRONG IN MINE.C!" );
	Error("Sorry, v20 segment support is broken.");
#if 0
	PHYSFS_write(SaveFile, &Segments.front(), sizeof(segment), LevelSharedSegmentState.Num_segments);

	//===================== SAVE NEWSEGMENT INFO ======================

	if (newsegment_offset != PHYSFS_tell(SaveFile))
		Error( "OFFSETS WRONG IN MINE.C!" );
	PHYSFSX_writeBytes(SaveFile, &New_segment, sizeof(segment));
#endif

	if (newseg_verts_offset != PHYSFS_tell(SaveFile))
		Error( "OFFSETS WRONG IN MINE.C!" );
	PHYSFSX_writeBytes(SaveFile, &Vertices[New_segment.verts.front()], sizeof(vms_vector) * 8);

	//==================== CLOSE THE FILE =============================

	return 0;

}



#define COMPILED_MINE_VERSION 0

static void dump_fix_as_short( fix value, int nbits, PHYSFS_File *SaveFile )
{
	short short_value;

	auto int_value = value >> nbits;
	if (int_value > INT16_MAX)
	{
		short_value = INT16_MAX;
	}
	else if( int_value < -0x7fff ) {
		short_value = -0x7fff;
	}
	else
		short_value = static_cast<short>(int_value);

	PHYSFS_writeSLE16(SaveFile, short_value);
}

//version of dump for unsigned values
static void dump_fix_as_ushort( fix value, int nbits, PHYSFS_File *SaveFile )
{
        uint int_value{0};
	ushort short_value;

	if (value < 0) {
		Int3();		//hey---show this to Matt
		value = 0;
	}
	else
		int_value = value >> nbits;

	if( int_value > 0xffff ) {
		short_value = 0xffff;
	}
	else
		short_value = int_value;

	PHYSFS_writeULE16(SaveFile, short_value);
}

static void write_children(const shared_segment &seg, const sidemask_t bit_mask, PHYSFS_File *const SaveFile)
{
	for (const auto &&[bit, child] : enumerate(seg.children))
	{
		if (bit_mask & build_sidemask(bit))
			PHYSFS_writeSLE16(SaveFile, child);
	}
}

static void write_verts(const shared_segment &seg, PHYSFS_File *const SaveFile)
{
	range_for (auto &i, seg.verts)
		PHYSFS_writeSLE16(SaveFile, static_cast<uint16_t>(i));
}

static void write_special(const shared_segment &seg, const sidemask_t bit_mask, PHYSFS_File *const SaveFile)
{
	if (bit_mask & build_sidemask(MAX_SIDES_PER_SEGMENT))
	{
		PHYSFSX_writeU8(SaveFile, underlying_value(seg.special));
		PHYSFSX_writeU8(SaveFile, underlying_value(seg.matcen_num));
		PHYSFS_writeULE16(SaveFile, underlying_value(seg.station_idx));
	}
}

}

int med_save_mine(const mine_filename_type &filename)
{
	return med_save_mine(filename.data());
}

// -----------------------------------------------------------------------------
// saves compiled mine data to an already-open file...
namespace dsx {
int save_mine_data_compiled(PHYSFS_File *SaveFile)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	ubyte 	version = COMPILED_MINE_VERSION;

	med_compress_mine();
	warn_if_concave_segments();

	if (Highest_segment_index >= MAX_SEGMENTS) {
		char	message[128];
		snprintf(message, sizeof(message), "Error: Too many segments (%i > %" PRIuFAST32 ") for game (not editor)", Highest_segment_index+1, static_cast<uint_fast32_t>(MAX_SEGMENTS));
		ui_messagebox( -2, -2, 1, message, "Ok" );
	}

	auto &Vertices = LevelSharedVertexState.get_vertices();
	if (Vertices.get_count() > MAX_VERTICES)
	{
		char	message[128];
		snprintf(message, sizeof(message), "Error: Too many vertices (%i > %" PRIuFAST32 ") for game (not editor)", Vertices.get_count(), static_cast<uint_fast32_t>(MAX_VERTICES));
		ui_messagebox( -2, -2, 1, message, "Ok" );
	}

	//=============================== Writing part ==============================
	PHYSFSX_writeU8(SaveFile, version);						// 1 byte = compiled version
	if (New_file_format_save)
	{
		PHYSFS_writeSLE16(SaveFile, LevelSharedVertexState.Num_vertices);					// 2 bytes = Num_vertices
		PHYSFS_writeSLE16(SaveFile, LevelSharedSegmentState.Num_segments);					// 2 bytes = Num_segments
	}
	else
	{
		PHYSFS_writeSLE32(SaveFile, LevelSharedVertexState.Num_vertices);					// 4 bytes = Num_vertices
		PHYSFS_writeSLE32(SaveFile, LevelSharedSegmentState.Num_segments);					// 4 bytes = Num_segments
	}

	range_for (auto &i, partial_const_range(Vertices, LevelSharedVertexState.Num_vertices))
		PHYSFSX_writeVector(SaveFile, i);
	
	const auto Num_segments = LevelSharedSegmentState.Num_segments;
	auto &&segment_range = partial_const_range(Segments, Num_segments);
	for (const cscusegment seg : segment_range)
	{
		{
		sidemask_t bit_mask{};
		for (const auto &&[sidenum, child] : enumerate(seg.s.children))
		{
			if (child != segment_none)
				bit_mask |= build_sidemask(sidenum);
		}

		if (seg.s.special != segment_special::nothing || seg.s.matcen_num != materialization_center_number::None || seg.s.station_idx != station_number::None)
			bit_mask |= build_sidemask(MAX_SIDES_PER_SEGMENT);

		if (New_file_format_save)
			PHYSFSX_writeU8(SaveFile, underlying_value(bit_mask));
		else
			bit_mask = sidemask_t{0x7f};

		if (Gamesave_current_version == 5)	// d2 SHAREWARE level
		{
			write_special(seg, bit_mask, SaveFile);
			write_verts(seg, SaveFile);
			write_children(seg, bit_mask, SaveFile);
		}
		else
		{
			write_children(seg, bit_mask, SaveFile);
			write_verts(seg, SaveFile);
			if (Gamesave_current_version <= 1) // descent 1 level
				write_special(seg, bit_mask, SaveFile);
		}

		if (Gamesave_current_version <= 5) // descent 1 thru d2 SHAREWARE level
			dump_fix_as_ushort(seg.u.static_light, 4, SaveFile);
		}

		// Write the walls as a 6 byte array
		{
		sidemask_t bit_mask{};
		for (const auto &&[sidenum, side] : enumerate(seg.s.sides))
		{
			if (side.wall_num != wall_none)
				bit_mask |= build_sidemask(sidenum);
		}
		if (New_file_format_save)
			PHYSFSX_writeU8(SaveFile, underlying_value(bit_mask));
		else
			bit_mask = sidemask_t{0x3f};

		for (const auto &&[sidenum, side] : enumerate(seg.s.sides))
		{
			if (bit_mask & build_sidemask(sidenum))
				PHYSFSX_writeU8(SaveFile, underlying_value(side.wall_num));
		}
		}

		for (const auto sidenum : MAX_SIDES_PER_SEGMENT)
		{
			if (seg.s.children[sidenum] == segment_none || seg.s.sides[sidenum].wall_num != wall_none)
			{
				auto tmap_num = seg.u.sides[sidenum].tmap_num;
				auto tmap_num2 = seg.u.sides[sidenum].tmap_num2;

#if DXX_BUILD_DESCENT == 2
				if (Gamesave_current_version <= 3)	// convert texture numbers back to d1
				{
					tmap_num = build_texture1_value(convert_to_d1_tmap_num(get_texture_index(tmap_num)));
					if (tmap_num2 != texture2_value::None)
					{
						tmap_num2 = build_texture2_value(convert_to_d1_tmap_num(get_texture_index(tmap_num2)), get_texture_rotation_high(tmap_num2));
					}
				}
#endif

				uint16_t write_tmap_num = static_cast<uint16_t>(tmap_num);
				if (tmap_num2 != texture2_value::None && New_file_format_save)
					write_tmap_num |= 0x8000;

				PHYSFS_writeSLE16(SaveFile, write_tmap_num);
				if (tmap_num2 != texture2_value::None || !New_file_format_save)
					PHYSFS_writeSLE16(SaveFile, static_cast<uint16_t>(tmap_num2));

				range_for (auto &i, seg.u.sides[sidenum].uvls)
				{
					dump_fix_as_short(i.u, 5, SaveFile);
					dump_fix_as_short(i.v, 5, SaveFile);
					dump_fix_as_ushort(i.l, 1, SaveFile);
				}	
			}
		}

	}

#if DXX_BUILD_DESCENT == 2
	if (Gamesave_current_version > 5)
		for (auto &s : segment_range)
			segment2_write(s, SaveFile);
#endif

	return 0;
}
}


