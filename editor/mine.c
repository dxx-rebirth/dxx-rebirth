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
#include "error.h"
#include "textures.h"
#include "object.h"
#include "gamemine.h"
#include "gameseg.h"
#include "ui.h"			// Because texpage.h need UI_WINDOW type
#include "texpage.h"		// For texpage_goto_first
#include "medwall.h"
#include "switch.h"
#include "fuelcen.h"

#define REMOVE_EXT(s)  (*(strchr( (s), '.' ))='\0')

int CreateDefaultNewSegment();
int save_mine_data(PHYSFS_file * SaveFile);
int save_mine_data_compiled_new(PHYSFS_file * SaveFile);

static char	 current_tmap_list[MAX_TEXTURES][13];

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

	SaveFile = cfopen( filename, CF_WRITE_MODE );
	if (!SaveFile)
	{
#ifndef __LINUX__
		char fname[20];
		_splitpath( filename, NULL, NULL, fname, NULL );

		sprintf( ErrorMessage, \
			"ERROR: Cannot write to '%s'.\nYou probably need to check out a locked\nversion of the file. You should save\nthis under a different filename, and then\ncheck out a locked copy by typing\n\'co -l %s.lvl'\nat the DOS prompt.\n" 
                        , filename, fname);
#endif
		sprintf( ErrorMessage, "ERROR: Unable to open %s\n", filename );
		MessageBox( -2, -2, 1, ErrorMessage, "Ok" );
		return 1;
	}

	save_mine_data(SaveFile);
	
	//==================== CLOSE THE FILE =============================
	cfclose(SaveFile);

	return 0;

}

// -----------------------------------------------------------------------------
// saves to an already-open file
int save_mine_data(PHYSFS_file * SaveFile)
{
	int  header_offset, editor_offset, vertex_offset, segment_offset, doors_offset, texture_offset, walls_offset, triggers_offset; //, links_offset;
	int  newseg_verts_offset;
	int  newsegment_offset;
	int  i;

	med_compress_mine();
	warn_if_concave_segments();
	
	for (i=0;i<NumTextures;i++)
		strncpy(current_tmap_list[i], TmapInfo[i].filename, 13);

	//=================== Calculate offsets into file ==================

	header_offset = cftell(SaveFile) + sizeof(mine_fileinfo);
	editor_offset = header_offset + sizeof(mine_header);
	texture_offset = editor_offset + sizeof(mine_editor);
	vertex_offset  = texture_offset + (13*NumTextures);
	segment_offset = vertex_offset + (sizeof(vms_vector)*Num_vertices);
	newsegment_offset = segment_offset + (sizeof(segment)*Num_segments);
	newseg_verts_offset = newsegment_offset + sizeof(segment);
	walls_offset = newseg_verts_offset + (sizeof(vms_vector)*8);
	triggers_offset =	walls_offset + (sizeof(wall)*Num_walls);
	doors_offset = triggers_offset + (sizeof(trigger)*Num_triggers);

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
	if (header_offset != cftell(SaveFile))
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

	if (editor_offset != cftell(SaveFile))
		Error( "OFFSETS WRONG IN MINE.C!" );
	PHYSFS_write( SaveFile, &mine_editor, sizeof(mine_editor), 1 );

	//===================== SAVE TEXTURE INFO ==========================

	if (texture_offset != cftell(SaveFile))
		Error( "OFFSETS WRONG IN MINE.C!" );
	PHYSFS_write( SaveFile, current_tmap_list, 13, NumTextures );
	
	//===================== SAVE VERTEX INFO ==========================

	if (vertex_offset != cftell(SaveFile))
		Error( "OFFSETS WRONG IN MINE.C!" );
	PHYSFS_write( SaveFile, Vertices, sizeof(vms_vector), Num_vertices );

	//===================== SAVE SEGMENT INFO =========================

	if (segment_offset != cftell(SaveFile))
		Error( "OFFSETS WRONG IN MINE.C!" );
	PHYSFS_write( SaveFile, Segments, sizeof(segment), Num_segments );

	//===================== SAVE NEWSEGMENT INFO ======================

	if (newsegment_offset != cftell(SaveFile))
		Error( "OFFSETS WRONG IN MINE.C!" );
	PHYSFS_write( SaveFile, &New_segment, sizeof(segment), 1 );

	if (newseg_verts_offset != cftell(SaveFile))
		Error( "OFFSETS WRONG IN MINE.C!" );
	PHYSFS_write( SaveFile, &Vertices[New_segment.verts[0]], sizeof(vms_vector), 8 );

	//==================== CLOSE THE FILE =============================

	return 0;

}



#define COMPILED_MINE_VERSION 0

void dump_fix_as_short( fix value, int nbits, CFILE * SaveFile )
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

	PHYSFS_write( SaveFile, &short_value, sizeof(short_value), 1 );
}

//version of dump for unsigned values
void dump_fix_as_ushort( fix value, int nbits, CFILE * SaveFile )
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

	PHYSFS_write( SaveFile, &short_value, sizeof(short_value), 1 );
}

int	New_file_format_save = 1;

// -----------------------------------------------------------------------------
// saves compiled mine data to an already-open file...
int save_mine_data_compiled(PHYSFS_file * SaveFile)
{
	short i,segnum,sidenum;
	ubyte version = COMPILED_MINE_VERSION;

#ifndef SHAREWARE
	if (New_file_format_save)
		return save_mine_data_compiled_new(SaveFile);
#endif

	med_compress_mine();
	warn_if_concave_segments();

	if (Highest_segment_index >= MAX_GAME_SEGMENTS) {
		char	message[128];
		sprintf(message, "Error: Too many segments (%i > %i) for game (not editor)", Highest_segment_index+1, MAX_GAME_SEGMENTS);
		MessageBox( -2, -2, 1, message, "Ok" );
	}

	if (Highest_vertex_index >= MAX_GAME_VERTICES) {
		char	message[128];
		sprintf(message, "Error: Too many vertices (%i > %i) for game (not editor)", Highest_vertex_index+1, MAX_GAME_VERTICES);
		MessageBox( -2, -2, 1, message, "Ok" );
	}

	//=============================== Writing part ==============================
	PHYSFS_write( SaveFile, &version, sizeof(ubyte), 1 );						// 1 byte = compiled version
	PHYSFS_write( SaveFile, &Num_vertices, sizeof(int), 1 );					// 4 bytes = Num_vertices
	PHYSFS_write( SaveFile, &Num_segments, sizeof(int), 1 );						// 4 bytes = Num_segments
	PHYSFS_write( SaveFile, Vertices, sizeof(vms_vector), Num_vertices );

	for (segnum=0; segnum<Num_segments; segnum++ )	{
		// Write short Segments[segnum].children[MAX_SIDES_PER_SEGMENT]
 		PHYSFS_write( SaveFile, &Segments[segnum].children, sizeof(short), MAX_SIDES_PER_SEGMENT );
		// Write short Segments[segnum].verts[MAX_VERTICES_PER_SEGMENT]
		PHYSFS_write( SaveFile, &Segments[segnum].verts, sizeof(short), MAX_VERTICES_PER_SEGMENT );
		// Write ubyte	Segments[segnum].special
		PHYSFS_write( SaveFile, &Segments[segnum].special, sizeof(ubyte), 1 );
		// Write byte	Segments[segnum].matcen_num
		PHYSFS_write( SaveFile, &Segments[segnum].matcen_num, sizeof(ubyte), 1 );
		// Write short	Segments[segnum].value
		PHYSFS_write( SaveFile, &Segments[segnum].value, sizeof(short), 1 );
		// Write fix	Segments[segnum].static_light (shift down 5 bits, write as short)
		dump_fix_as_ushort( Segments[segnum].static_light, 4, SaveFile );
		//PHYSFS_write( SaveFile, &Segments[segnum].static_light , sizeof(fix), 1 );
	
		// Write the walls as a 6 byte array
		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++ )	{
			uint wallnum;
			ubyte byte_wallnum;
			if (Segments[segnum].sides[sidenum].wall_num<0)
				wallnum = 255;		// Use 255 to mark no walls
			else {
				wallnum = Segments[segnum].sides[sidenum].wall_num;
				Assert( wallnum < 255 );		// Get John or Mike.. can only store up to 255 walls!!! 
			}
			byte_wallnum = (ubyte)wallnum;
			PHYSFS_write( SaveFile, &byte_wallnum, sizeof(ubyte), 1 );
		}

		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++ )	{
			if ( (Segments[segnum].children[sidenum]==-1) || (Segments[segnum].sides[sidenum].wall_num!=-1) )	{
				// Write short Segments[segnum].sides[sidenum].tmap_num;
				PHYSFS_write( SaveFile, &Segments[segnum].sides[sidenum].tmap_num, sizeof(short), 1 );
				// Write short Segments[segnum].sides[sidenum].tmap_num2;
				PHYSFS_write( SaveFile, &Segments[segnum].sides[sidenum].tmap_num2, sizeof(short), 1 );
				// Write uvl Segments[segnum].sides[sidenum].uvls[4] (u,v>>5, write as short, l>>1 write as short)
				for (i=0; i<4; i++ )	{
					dump_fix_as_short( Segments[segnum].sides[sidenum].uvls[i].u, 5, SaveFile );
					dump_fix_as_short( Segments[segnum].sides[sidenum].uvls[i].v, 5, SaveFile );
					dump_fix_as_ushort( Segments[segnum].sides[sidenum].uvls[i].l, 1, SaveFile );
					//PHYSFS_write( SaveFile, &Segments[segnum].sides[sidenum].uvls[i].l, sizeof(fix), 1 );
				}	
			}
		}

	}

	return 0;
}

// -----------------------------------------------------------------------------
// saves compiled mine data to an already-open file...
int save_mine_data_compiled_new(PHYSFS_file * SaveFile)
{
	short		i, segnum, sidenum, temp_short;
	ubyte 	version = COMPILED_MINE_VERSION;
	ubyte		bit_mask = 0;

	med_compress_mine();
	warn_if_concave_segments();

	if (Highest_segment_index >= MAX_GAME_SEGMENTS) {
		char	message[128];
		sprintf(message, "Error: Too many segments (%i > %i) for game (not editor)", Highest_segment_index+1, MAX_GAME_SEGMENTS);
		MessageBox( -2, -2, 1, message, "Ok" );
	}

	if (Highest_vertex_index >= MAX_GAME_VERTICES) {
		char	message[128];
		sprintf(message, "Error: Too many vertices (%i > %i) for game (not editor)", Highest_vertex_index+1, MAX_GAME_VERTICES);
		MessageBox( -2, -2, 1, message, "Ok" );
	}

	//=============================== Writing part ==============================
	PHYSFS_write( SaveFile, &version, sizeof(ubyte), 1 );						// 1 byte = compiled version
	temp_short = Num_vertices;
	PHYSFS_write( SaveFile, &temp_short, sizeof(short), 1 );					// 2 bytes = Num_vertices
	temp_short = Num_segments;
	PHYSFS_write( SaveFile, &temp_short, sizeof(short), 1 );					// 2 bytes = Num_segments
	PHYSFS_write( SaveFile, Vertices, sizeof(vms_vector), Num_vertices );

	for (segnum=0; segnum<Num_segments; segnum++ )	{

		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++) {
 			if (Segments[segnum].children[sidenum] != -1)
				bit_mask |= (1 << sidenum);
		}

		if ((Segments[segnum].special != 0) || (Segments[segnum].matcen_num != 0) || (Segments[segnum].value != 0))
			bit_mask |= (1 << MAX_SIDES_PER_SEGMENT);

 		PHYSFS_write( SaveFile, &bit_mask, sizeof(ubyte), 1 );

		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++) {
 			if (bit_mask & (1 << sidenum))
		 		PHYSFS_write( SaveFile, &Segments[segnum].children[sidenum], sizeof(short), 1 );
		}

		PHYSFS_write( SaveFile, &Segments[segnum].verts, sizeof(short), MAX_VERTICES_PER_SEGMENT );

		if (bit_mask & (1 << MAX_SIDES_PER_SEGMENT)) {
			PHYSFS_write( SaveFile, &Segments[segnum].special, sizeof(ubyte), 1 );
			PHYSFS_write( SaveFile, &Segments[segnum].matcen_num, sizeof(ubyte), 1 );
			PHYSFS_write( SaveFile, &Segments[segnum].value, sizeof(short), 1 );
		}

		dump_fix_as_ushort( Segments[segnum].static_light, 4, SaveFile );
	
		// Write the walls as a 6 byte array
		bit_mask = 0;
		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++ )	{
			uint wallnum;
			if (Segments[segnum].sides[sidenum].wall_num >= 0) {
				bit_mask |= (1 << sidenum);
				wallnum = Segments[segnum].sides[sidenum].wall_num;
				Assert( wallnum < 255 );		// Get John or Mike.. can only store up to 255 walls!!! 
			}
		}
		PHYSFS_write( SaveFile, &bit_mask, sizeof(ubyte), 1 );

		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++ )	{
			if (bit_mask & (1 << sidenum))
				PHYSFS_write( SaveFile, &Segments[segnum].sides[sidenum].wall_num, sizeof(ubyte), 1 );
		}

		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++ )	{
			if ( (Segments[segnum].children[sidenum]==-1) || (Segments[segnum].sides[sidenum].wall_num!=-1) )	{
				ushort	tmap_num, tmap_num2;

				tmap_num = Segments[segnum].sides[sidenum].tmap_num;
				tmap_num2 = Segments[segnum].sides[sidenum].tmap_num2;
				if (tmap_num2 != 0)
					tmap_num |= 0x8000;

				PHYSFS_write( SaveFile, &tmap_num, sizeof(ushort), 1 );
				if (tmap_num2 != 0)
					PHYSFS_write( SaveFile, &tmap_num2, sizeof(ushort), 1 );

				for (i=0; i<4; i++ )	{
					dump_fix_as_short( Segments[segnum].sides[sidenum].uvls[i].u, 5, SaveFile );
					dump_fix_as_short( Segments[segnum].sides[sidenum].uvls[i].v, 5, SaveFile );
					dump_fix_as_ushort( Segments[segnum].sides[sidenum].uvls[i].l, 1, SaveFile );
				}	
			}
		}

	}

	return 0;
}


