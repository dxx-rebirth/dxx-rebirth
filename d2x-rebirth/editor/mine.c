/*
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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "physfsx.h"
#include "key.h"
#include "gr.h"
#include "bm.h"			// for MAX_TEXTURES
#include "inferno.h"
#include "segment.h"
#include "editor.h"
#include "editor/esegment.h"
#include "dxxerror.h"
#include "textures.h"
#include "object.h"
#include "gamemine.h"
#include "gamesave.h"
#include "gameseg.h"
#include "ui.h"			// Because texpage.h need UI_DIALOG type
#include "texpage.h"		// For texpage_goto_first
#include "medwall.h"
#include "switch.h"
#include "fuelcen.h"

#define REMOVE_EXT(s)  (*(strchr( (s), '.' ))='\0')

int CreateDefaultNewSegment();
int save_mine_data(PHYSFS_file * SaveFile);

static char	 current_tmap_list[MAX_TEXTURES][13];

int	New_file_format_save = 1;

// Converts descent 2 texture numbers back to descent 1 texture numbers.
// Only works properly when the full Descent 1 texture set (descent.pig) is available.
short convert_to_d1_tmap_num(short tmap_num)
{
	switch (tmap_num)
	{
		case 137: return   0; // grey rock001
		case   0: return   1;
		case   1: return   3; // rock021
		case 270: return   6; // blue rock002
		case 271: return   7; // yellow rock265
		case   2: return   8; // rock004
		case 138: return   9; // purple (devil:179)
		case 272: return  10; // red rock006
		case 139: return  11;
		case 140: return  12; //devil:43
		case   3: return  13; // rock014
		case   4: return  14; // rock019
		case   5: return  15; // rock020
		case   6: return  16;
		case 141: return  17;
		case 129: return  18;
		case   7: return  19;
		case 142: return  20;
		case 143: return  21;
		case   8: return  22;
		case   9: return  23;
		case  10: return  24;
		case 144: return  25; //devil:35
		case  11: return  26;
		case  12: return  27;
		case 145: return  28; //devil:43
		//range handled by default case, returns 13..21 (- 16)
		case 163: return  38; //devil:27
		case 147: return  39; //31
		case  22: return  40;
		case 266: return  41;
		case  23: return  42;
		case  24: return  43;
		case 136: return  44; //devil:135
		case  25: return  45;
		case  26: return  46;
		case  27: return  47;
		case  28: return  48;
		case 146: return  49; //devil:60
		case 131: return  50; //devil:138
		case  29: return  51;
		case  30: return  52;
		case  31: return  53;
		case  32: return  54;
		case 165: return  55; //devil:193
		case  33: return  56;
		case 132: return  57; //devil:119
		// range handled by default case, returns 58..88 (+ 24)
		case 197: return  88; //devil:15
		// range handled by default case, returns 89..106 (- 25)
		case 167: return 132;
		// range handled by default case, returns 107..114 (- 26)
		case 148: return 141; //devil:106
		case 115: return 142;
		case 116: return 143;
		case 117: return 144;
		case 118: return 145;
		case 119: return 146;
		case 149: return 147;
		case 120: return 148;
		case 121: return 149;
		case 122: return 150;
		case 123: return 151;
		case 124: return 152;
		case 125: return 153; // rock263
		case 150: return 154;
		case 126: return 155; // rock269
		case 200: return 156; // metl002
		case 201: return 157; // metl003
		case 186: return 158; //devil:227
		case 190: return 159; //devil:246
		case 151: return 160;
		case 152: return 161; //devil:206
		case 202: return 162;
		case 203: return 163;
		case 204: return 164;
		case 205: return 165;
		case 206: return 166;
		case 153: return 167;
		case 154: return 168;
		case 155: return 169;
		case 156: return 170;//206;
		case 157: return 171;//227;
		case 207: return 172;
		case 208: return 173;
		case 158: return 174;
		case 159: return 175;
		// range handled by default case, returns 209..217 (+ 33)
		case 160: return 185;
		// range handled by default case, returns 218..224 (+ 32)
		case 161: return 193;
		case 162: return 194;//206;
		case 166: return 195;
		case 225: return 196;
		case 226: return 197;
		case 193: return 198;
		case 168: return 199; //devil:204
		case 169: return 200; //devil:204
		case 227: return 201;
		case 170: return 202; //devil:227
		// range handled by default case, returns 228..234 (+ 25)
		case 171: return 210; //devil:242
		case 172: return 211; //devil:240
		// range handled by default case, returns 235..242 (+ 23)
		case 173: return 220; //devil:240
		case 243: return 221;
		case 244: return 222;
		case 174: return 223;
		case 245: return 224;
		case 246: return 225;
		case 164: return 226;//247; matching names but not matching textures
		case 179: return 227; //devil:181
		case 196: return 228;//248; matching names but not matching textures
		case 175: return 229; //devil:66
		case 176: return 230; //devil:66
		// range handled by default case, returns 249..257 (+ 18)
		case 177: return 240; //devil:132
		case 130: return 241; //devil:131
		case 178: return 242; //devil:15
		case 180: return 243; //devil:38
		case 258: return 244;
		case 259: return 245;
		case 181: return 246; // grate metl127
		case 260: return 247;
		case 261: return 248;
		case 262: return 249;
		case 340: return 250; //  white doorframe metl126
		case 412: return 251; //    red doorframe metl133
		case 410: return 252; //   blue doorframe metl134
		case 411: return 253; // yellow doorframe metl135
		case 263: return 254; // metl136
		case 264: return 255; // metl139
		case 265: return 256; // metl140
		case 182: return 257;//246; brig001
		case 183: return 258;//246; brig002
		case 184: return 259;//246; brig003
		case 185: return 260;//246; brig004
		case 273: return 261; // exit01
		case 274: return 262; // exit02
		case 187: return 263; // ceil001
		case 275: return 264; // ceil002
		case 276: return 265; // ceil003
		case 188: return 266; //devil:291
		// range handled by default case, returns 277..291 (+ 10)
		case 293: return 282;
		case 189: return 283;
		case 295: return 284;
		case 296: return 285;
		case 298: return 286;
		// range handled by default case, returns 300..310 (+ 13)
		case 191: return 298; // devil:374 misc010
		// range handled by default case, returns 311..326 (+ 12)
		case 192: return 315; // bad producer misc044
		// range handled by default case,  returns  327..337 (+ 11)
		case 352: return 327; // arw01
		case 353: return 328; // misc17
		case 354: return 329; // fan01
		case 380: return 330; // mntr04
		case 379: return 331;//373; matching names but not matching textures
		case 355: return 332;//344; matching names but not matching textures
		case 409: return 333; // lava misc11 //devil:404
		case 356: return 334; // ctrl04
		case 357: return 335; // ctrl01
		case 358: return 336; // ctrl02
		case 359: return 337; // ctrl03
		case 360: return 338; // misc14
		case 361: return 339; // producer misc16
		case 362: return 340; // misc049
		case 364: return 341; // misc060
		case 363: return 342; // blown01
		case 366: return 343; // misc061
		case 365: return 344;
		case 368: return 345;
		case 376: return 346;
		case 370: return 347;
		case 367: return 348;
		case 372: return 349;
		case 369: return 350;
		case 374: return 351;//429; matching names but not matching textures
		case 375: return 352;//387; matching names but not matching textures
		case 371: return 353;
		case 377: return 354;//425; matching names but not matching textures
		case 408: return 355;
		case 378: return 356; // lava02
		case 383: return 357;//384; matching names but not matching textures
		case 384: return 358;//385; matching names but not matching textures
		case 385: return 359;//386; matching names but not matching textures
		case 386: return 360;
		case 387: return 361;
		case 194: return 362; // mntr04b (devil: -1)
		case 388: return 363;
		case 391: return 364;
		case 392: return 365;
		case 393: return 366;
		case 394: return 367;
		case 395: return 368;
		case 396: return 369;
		case 195: return 370; // mntr04d (devil: -1)
		// range 371..584 handled by default case (wall01 and door frames)
		default:
			// ranges:
			if (tmap_num >= 13 && tmap_num <= 21)
				return tmap_num + 16;
			if (tmap_num >= 34 && tmap_num <= 63)
				return tmap_num + 24;
			if (tmap_num >= 64 && tmap_num <= 106)
				return tmap_num + 25;
			if (tmap_num >= 107 && tmap_num <= 114)
				return tmap_num + 26;
			if (tmap_num >= 209 && tmap_num <= 217)
				return tmap_num - 33;
			if (tmap_num >= 218 && tmap_num <= 224)
				return tmap_num - 32;
			if (tmap_num >= 228 && tmap_num <= 234)
				return tmap_num - 25;
			if (tmap_num >= 235 && tmap_num <= 242)
				return tmap_num - 23;
			if (tmap_num >= 249 && tmap_num <= 257)
				return tmap_num - 18;
			if (tmap_num >= 277 && tmap_num <= 291)
				return tmap_num - 10;
			if (tmap_num >= 300 && tmap_num <= 310)
				return tmap_num - 13;
			if (tmap_num >= 311 && tmap_num <= 326)
				return tmap_num - 12;
			if (tmap_num >= 327 && tmap_num <= 337)
				return tmap_num - 11; // matching names but not matching textures
			// wall01 and door frames:
			if (tmap_num > 434 && tmap_num < 731)
			{
				if (New_file_format_save) return tmap_num - 64;
				// d1 shareware needs special treatment:
				if (tmap_num < 478) return tmap_num - 68;
				if (tmap_num < 490) return tmap_num - 73;
				if (tmap_num < 537) return tmap_num - 91;
				if (tmap_num < 557) return tmap_num - 104;
				if (tmap_num < 573) return tmap_num - 111;
				if (tmap_num < 603) return tmap_num - 117;
				if (tmap_num < 635) return tmap_num - 141;
				if (tmap_num < 731) return tmap_num - 147;
			}
			{ // handle rare case where orientation != 0
				short tmap_num_part = tmap_num &  TMAP_NUM_MASK;
				short orient = tmap_num & ~TMAP_NUM_MASK;
				if (orient != 0)
					return orient | convert_to_d1_tmap_num(tmap_num_part);
				else
				{
					Warning("can't convert unknown texture #%d to descent 1.\n", tmap_num_part);
					return tmap_num;
				}
			}
	}
}

// -----------------------------------------------------------------------------
// Save mine will:
// 1. Write file info, header info, editor info, vertex data, segment data,
//    and new_segment in that order, marking their file offset.
// 2. Go through all the fields and fill in the offset, size, and sizeof
//    values in the headers.

int med_save_mine(char * filename)
{
	PHYSFS_file *SaveFile;
	char ErrorMessage[256];

	SaveFile = PHYSFSX_openWriteBuffered( filename );
	if (!SaveFile)
	{
#if 0 //ndef __linux__
		char fname[20];
		d_splitpath( filename, NULL, NULL, fname, NULL );

		sprintf( ErrorMessage, \
			"ERROR: Cannot write to '%s'.\nYou probably need to check out a locked\nversion of the file. You should save\nthis under a different filename, and then\ncheck out a locked copy by typing\n\'co -l %s.lvl'\nat the DOS prompt.\n" 
                        , filename, fname);
#endif
		sprintf( ErrorMessage, "ERROR: Unable to open %s\n", filename );
		ui_messagebox( -2, -2, 1, ErrorMessage, "Ok" );
		return 1;
	}

	save_mine_data(SaveFile);
	
	//==================== CLOSE THE FILE =============================
	PHYSFS_close(SaveFile);

	return 0;

}

// -----------------------------------------------------------------------------
// saves to an already-open file
int save_mine_data(PHYSFS_file * SaveFile)
{
	int  header_offset, editor_offset, vertex_offset, segment_offset, texture_offset, walls_offset, triggers_offset; //, links_offset;
	int  newseg_verts_offset;
	int  newsegment_offset;
	int  i;

	med_compress_mine();
	warn_if_concave_segments();
	
	for (i=0;i<NumTextures;i++)
		strncpy(current_tmap_list[i], TmapInfo[i].filename, 13);

	//=================== Calculate offsets into file ==================

	header_offset = PHYSFS_tell(SaveFile) + sizeof(mine_fileinfo);
	editor_offset = header_offset + sizeof(mine_header);
	texture_offset = editor_offset + sizeof(mine_editor);
	vertex_offset  = texture_offset + (13*NumTextures);
	segment_offset = vertex_offset + (sizeof(vms_vector)*Num_vertices);
	newsegment_offset = segment_offset + (sizeof(segment)*Num_segments);
	newseg_verts_offset = newsegment_offset + sizeof(segment);
	walls_offset = newseg_verts_offset + (sizeof(vms_vector)*8);
	triggers_offset =	walls_offset + (sizeof(wall)*Num_walls);
// 	doors_offset = triggers_offset + (sizeof(trigger)*Num_triggers);

	//===================== SAVE FILE INFO ========================

	mine_fileinfo.fileinfo_signature=	0x2884;
	mine_fileinfo.fileinfo_version  =   MINE_VERSION;
	mine_fileinfo.fileinfo_sizeof   =   sizeof(mine_fileinfo);
	mine_fileinfo.header_offset     =   header_offset;
	mine_fileinfo.header_size       =   sizeof(mine_header);
	mine_fileinfo.editor_offset     =   editor_offset;
	mine_fileinfo.editor_size       =   sizeof(mine_editor);
	mine_fileinfo.vertex_offset     =   vertex_offset;
	mine_fileinfo.vertex_howmany    =   Num_vertices;
	mine_fileinfo.vertex_sizeof     =   sizeof(vms_vector);
	mine_fileinfo.segment_offset    =   segment_offset;
	mine_fileinfo.segment_howmany   =   Num_segments;
	mine_fileinfo.segment_sizeof    =   sizeof(segment);
	mine_fileinfo.newseg_verts_offset     =   newseg_verts_offset;
	mine_fileinfo.newseg_verts_howmany    =   8;
	mine_fileinfo.newseg_verts_sizeof     =   sizeof(vms_vector);
	mine_fileinfo.texture_offset    =   texture_offset;
	mine_fileinfo.texture_howmany   =   NumTextures;
	mine_fileinfo.texture_sizeof    =   13;  // num characters in a name
	mine_fileinfo.walls_offset		  =	walls_offset;
	mine_fileinfo.walls_howmany	  =	Num_walls;
	mine_fileinfo.walls_sizeof		  =	sizeof(wall);  
	mine_fileinfo.triggers_offset	  =	triggers_offset;
	mine_fileinfo.triggers_howmany  =	Num_triggers;
	mine_fileinfo.triggers_sizeof	  =	sizeof(trigger);  

	// Write the fileinfo
	PHYSFS_write( SaveFile, &mine_fileinfo, sizeof(mine_fileinfo), 1 );

	//===================== SAVE HEADER INFO ========================

	mine_header.num_vertices        =   Num_vertices;
	mine_header.num_segments        =   Num_segments;

	// Write the editor info
	if (header_offset != PHYSFS_tell(SaveFile))
		Error( "OFFSETS WRONG IN MINE.C!" );

	PHYSFS_write( SaveFile, &mine_header, sizeof(mine_header), 1 );

	//===================== SAVE EDITOR INFO ==========================
	mine_editor.current_seg         =   Cursegp - Segments;
	mine_editor.newsegment_offset   =   newsegment_offset; 
	mine_editor.newsegment_size     =   sizeof(segment);

	// Next 3 vars added 10/07 by JAS
	mine_editor.Curside             =   Curside;
	if (Markedsegp)
		mine_editor.Markedsegp       =   Markedsegp - Segments;
	else									  
		mine_editor.Markedsegp       =   -1;
	mine_editor.Markedside          =   Markedside;
	for (i=0;i<10;i++)
		mine_editor.Groupsegp[i]	  =	Groupsegp[i] - Segments;
	for (i=0;i<10;i++)
		mine_editor.Groupside[i]     =	Groupside[i];

	if (editor_offset != PHYSFS_tell(SaveFile))
		Error( "OFFSETS WRONG IN MINE.C!" );
	PHYSFS_write( SaveFile, &mine_editor, sizeof(mine_editor), 1 );

	//===================== SAVE TEXTURE INFO ==========================

	if (texture_offset != PHYSFS_tell(SaveFile))
		Error( "OFFSETS WRONG IN MINE.C!" );
	PHYSFS_write( SaveFile, current_tmap_list, 13, NumTextures );
	
	//===================== SAVE VERTEX INFO ==========================

	if (vertex_offset != PHYSFS_tell(SaveFile))
		Error( "OFFSETS WRONG IN MINE.C!" );
	PHYSFS_write( SaveFile, Vertices, sizeof(vms_vector), Num_vertices );

	//===================== SAVE SEGMENT INFO =========================

	if (segment_offset != PHYSFS_tell(SaveFile))
		Error( "OFFSETS WRONG IN MINE.C!" );
	PHYSFS_write( SaveFile, Segments, sizeof(segment), Num_segments );

	//===================== SAVE NEWSEGMENT INFO ======================

	if (newsegment_offset != PHYSFS_tell(SaveFile))
		Error( "OFFSETS WRONG IN MINE.C!" );
	PHYSFS_write( SaveFile, &New_segment, sizeof(segment), 1 );

	if (newseg_verts_offset != PHYSFS_tell(SaveFile))
		Error( "OFFSETS WRONG IN MINE.C!" );
	PHYSFS_write( SaveFile, &Vertices[New_segment.verts[0]], sizeof(vms_vector), 8 );

	//==================== CLOSE THE FILE =============================

	return 0;

}



#define COMPILED_MINE_VERSION 0

void dump_fix_as_short( fix value, int nbits, PHYSFS_file *SaveFile )
{
        int int_value=0; 
	short short_value;

        int_value = (int)(value>>nbits);
	if( int_value > 0x7fff ) {
		short_value = 0x7fff;
	}
	else if( int_value < -0x7fff ) {
		short_value = -0x7fff;
	}
	else
		short_value = (short)int_value;

	PHYSFS_writeSLE16(SaveFile, short_value);
}

//version of dump for unsigned values
void dump_fix_as_ushort( fix value, int nbits, PHYSFS_file *SaveFile )
{
        uint int_value=0;
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

void write_children(segment *seg, ubyte bit_mask, PHYSFS_file *SaveFile)
{
	int bit;

	for (bit = 0; bit < MAX_SIDES_PER_SEGMENT; bit++)
	{
		if (bit_mask & (1 << bit))
			PHYSFS_writeSLE16(SaveFile, seg->children[bit]);
	}
}

void write_verts(segment *seg, PHYSFS_file *SaveFile)
{
	int i;

	for (i = 0; i < MAX_VERTICES_PER_SEGMENT; i++)
		PHYSFS_writeSLE16(SaveFile, seg->verts[i]);
}

void write_special(segment2 *seg2, ubyte bit_mask, PHYSFS_file *SaveFile)
{
	if (bit_mask & (1 << MAX_SIDES_PER_SEGMENT))
	{
		PHYSFSX_writeU8(SaveFile, seg2->special);
		PHYSFSX_writeU8(SaveFile, seg2->matcen_num);
		PHYSFS_writeSLE16(SaveFile, seg2->value);
	}
}
// -----------------------------------------------------------------------------
// saves compiled mine data to an already-open file...
int save_mine_data_compiled(PHYSFS_file *SaveFile)
{
	short		i, segnum, sidenum;
	ubyte 	version = COMPILED_MINE_VERSION;
	ubyte		bit_mask = 0;

	med_compress_mine();
	warn_if_concave_segments();

	if (Highest_segment_index >= MAX_SEGMENTS) {
		char	message[128];
		sprintf(message, "Error: Too many segments (%i > %i) for game (not editor)", Highest_segment_index+1, MAX_SEGMENTS);
		ui_messagebox( -2, -2, 1, message, "Ok" );
	}

	if (Highest_vertex_index >= MAX_VERTICES) {
		char	message[128];
		sprintf(message, "Error: Too many vertices (%i > %i) for game (not editor)", Highest_vertex_index+1, MAX_VERTICES);
		ui_messagebox( -2, -2, 1, message, "Ok" );
	}

	//=============================== Writing part ==============================
	PHYSFSX_writeU8(SaveFile, version);						// 1 byte = compiled version
	if (New_file_format_save)
	{
		PHYSFS_writeSLE16(SaveFile, Num_vertices);					// 2 bytes = Num_vertices
		PHYSFS_writeSLE16(SaveFile, Num_segments);					// 2 bytes = Num_segments
	}
	else
	{
		PHYSFS_writeSLE32(SaveFile, Num_vertices);					// 4 bytes = Num_vertices
		PHYSFS_writeSLE32(SaveFile, Num_segments);					// 4 bytes = Num_segments
	}

	for (i = 0; i < Num_vertices; i++)
		PHYSFSX_writeVector(SaveFile, &(Vertices[i]));
	
	for (segnum = 0; segnum < Num_segments; segnum++)
	{
		segment *seg = &Segments[segnum];
		segment2 *seg2 = &Segment2s[segnum];

		for (sidenum = 0; sidenum < MAX_SIDES_PER_SEGMENT; sidenum++)
		{
 			if (seg->children[sidenum] != -1)
				bit_mask |= (1 << sidenum);
		}

		if ((seg2->special != 0) || (seg2->matcen_num != 0) || (seg2->value != 0))
			bit_mask |= (1 << MAX_SIDES_PER_SEGMENT);

		if (New_file_format_save)
			PHYSFSX_writeU8(SaveFile, bit_mask);
		else
			bit_mask = 0x7F;

		if (Gamesave_current_version == 5)	// d2 SHAREWARE level
		{
			write_special(seg2, bit_mask, SaveFile);
			write_verts(seg, SaveFile);
			write_children(seg, bit_mask, SaveFile);
		}
		else
		{
			write_children(seg, bit_mask, SaveFile);
			write_verts(seg, SaveFile);
			if (Gamesave_current_version <= 1) // descent 1 level
				write_special(seg2, bit_mask, SaveFile);
		}

		if (Gamesave_current_version <= 5) // descent 1 thru d2 SHAREWARE level
			dump_fix_as_ushort(seg2->static_light, 4, SaveFile);
	
		// Write the walls as a 6 byte array
		bit_mask = 0;
		for (sidenum = 0; sidenum < MAX_SIDES_PER_SEGMENT; sidenum++)
		{
			uint wallnum;

			if (seg->sides[sidenum].wall_num >= 0)
			{
				bit_mask |= (1 << sidenum);
				wallnum = seg->sides[sidenum].wall_num;
				Assert( wallnum < 255 );		// Get John or Mike.. can only store up to 255 walls!!! 
				(void)wallnum;
			}
		}
		if (New_file_format_save)
			PHYSFSX_writeU8(SaveFile, bit_mask);
		else
			bit_mask = 0x3F;

		for (sidenum = 0; sidenum < MAX_SIDES_PER_SEGMENT; sidenum++)
		{
			if (bit_mask & (1 << sidenum))
				PHYSFSX_writeU8(SaveFile, seg->sides[sidenum].wall_num);
		}

		for (sidenum = 0; sidenum < MAX_SIDES_PER_SEGMENT; sidenum++)
		{
			if ((seg->children[sidenum] == -1) || (seg->sides[sidenum].wall_num != -1))
			{
				ushort	tmap_num, tmap_num2;

				tmap_num = seg->sides[sidenum].tmap_num;
				tmap_num2 = seg->sides[sidenum].tmap_num2;

				if (Gamesave_current_version <= 3)	// convert texture numbers back to d1
				{
					tmap_num = convert_to_d1_tmap_num(tmap_num);
					if (tmap_num2)
						tmap_num2 = convert_to_d1_tmap_num(tmap_num2);
				}

				if (tmap_num2 != 0 && New_file_format_save)
					tmap_num |= 0x8000;

				PHYSFS_writeSLE16(SaveFile, tmap_num);
				if (tmap_num2 != 0 || !New_file_format_save)
					PHYSFS_writeSLE16(SaveFile, tmap_num2);

				for (i = 0; i < 4; i++)
				{
					dump_fix_as_short(seg->sides[sidenum].uvls[i].u, 5, SaveFile);
					dump_fix_as_short(seg->sides[sidenum].uvls[i].v, 5, SaveFile);
					dump_fix_as_ushort(seg->sides[sidenum].uvls[i].l, 1, SaveFile);
				}	
			}
		}

	}

	if (Gamesave_current_version > 5)
		for (i = 0; i < Num_segments; i++)
			segment2_write(&Segment2s[i], SaveFile);

	return 0;
}


