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
 * $Source: /cvs/cvsroot/d2x/main/editor/mine.c,v $
 * $Revision: 1.1 $
 * $Author: btb $
 * $Date: 2004-12-19 13:54:27 $
 * 
 * Mine specific editing functions, such as load_mine, save_mine
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  1999/06/14 22:04:07  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:34:38  john
 * Version 2.0! No anonymous unions, Watcom 10.0, with no need
 * for bitmaps.tbl.
 * 
 * Revision 1.82  1995/01/19  15:19:42  mike
 * New super-compressed registered file format.
 * 
 * Revision 1.81  1994/12/15  16:51:39  mike
 * fix error message.
 * 
 * Revision 1.80  1994/12/09  22:52:27  yuan
 * *** empty log message ***
 * 
 * Revision 1.79  1994/11/27  23:17:14  matt
 * Made changes for new mprintf calling convention
 * 
 * Revision 1.78  1994/11/26  21:48:24  matt
 * Fixed saturation in short light value
 * 
 * Revision 1.77  1994/11/18  09:43:22  mike
 * mprintf and clean up instead of Assert on values which don't fit in a short.
 * 
 * Revision 1.76  1994/11/17  20:37:37  john
 * Added comment to get mike or john.
 * 
 * Revision 1.75  1994/11/17  20:08:51  john
 * Added new compiled level format.
 * 
 * Revision 1.74  1994/11/17  11:39:00  matt
 * Ripped out code to load old mines
 * 
 * Revision 1.73  1994/10/20  12:47:47  matt
 * Replaced old save files (MIN/SAV/HOT) with new LVL files
 * 
 * Revision 1.72  1994/09/23  22:13:58  matt
 * Tooks out references to obsolete structure fields
 * 
 * Revision 1.71  1994/09/22  18:39:40  john
 * *** empty log message ***
 * 
 * Revision 1.70  1994/09/22  18:38:09  john
 * Added better help for locked files.
 * 
 * Revision 1.69  1994/08/01  11:04:44  yuan
 * New materialization centers.
 * 
 * Revision 1.68  1994/06/08  14:29:35  matt
 * Took out support for old mine versions
 * 
 * Revision 1.67  1994/05/27  10:34:37  yuan
 * Added new Dialog boxes for Walls and Triggers.
 * 
 * Revision 1.66  1994/05/23  14:48:08  mike
 * make current segment be add segment.
 * 
 * Revision 1.65  1994/05/17  10:34:52  matt
 * New parm to reset_objects; Num_objects no longer global
 * 
 * Revision 1.64  1994/05/12  14:46:46  mike
 * Load previous mine type.
 * 
 * Revision 1.63  1994/05/06  12:52:13  yuan
 * Adding some gamesave checks...
 * 
 * Revision 1.62  1994/05/05  12:56:32  yuan
 * Fixed a bunch of group bugs.
 * 
 * Revision 1.61  1994/05/03  11:36:55  yuan
 * Fixing mine save.
 * 
 * Revision 1.60  1994/03/19  17:22:14  yuan
 * Wall system implemented until specific features need to be added...
 * (Needs to be hammered on though.)
 * 
 * Revision 1.59  1994/03/17  18:08:32  yuan
 * New wall stuff... Cut out switches....
 * 
 * Revision 1.58  1994/03/15  16:34:15  yuan
 * Fixed bm loader (might have some changes in walls and switches)
 * 
 * Revision 1.57  1994/03/01  18:14:09  yuan
 * Added new walls, switches, and triggers.
 * 
 */


#ifdef RCS
static char rcsid[] = "$Id: mine.c,v 1.1 2004-12-19 13:54:27 btb Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "mono.h"
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

#include "nocfile.h"
#include "fuelcen.h"

#define REMOVE_EXT(s)  (*(strchr( (s), '.' ))='\0')

int CreateDefaultNewSegment();
int save_mine_data(CFILE * SaveFile);
int save_mine_data_compiled_new(FILE * SaveFile);

static char	 current_tmap_list[MAX_TEXTURES][13];

// -----------------------------------------------------------------------------
// Save mine will:
// 1. Write file info, header info, editor info, vertex data, segment data,
//    and new_segment in that order, marking their file offset.
// 2. Go through all the fields and fill in the offset, size, and sizeof
//    values in the headers.

int med_save_mine(char * filename)
{
	FILE * SaveFile;
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
int save_mine_data(CFILE * SaveFile)
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
	cfwrite( &mine_fileinfo, sizeof(mine_fileinfo), 1, SaveFile );

	//===================== SAVE HEADER INFO ========================

	mine_header.num_vertices        =   Num_vertices;
	mine_header.num_segments        =   Num_segments;

	// Write the editor info
	if (header_offset != cftell(SaveFile))
		Error( "OFFSETS WRONG IN MINE.C!" );

	cfwrite( &mine_header, sizeof(mine_header), 1, SaveFile );

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
	cfwrite( &mine_editor, sizeof(mine_editor), 1, SaveFile );

	//===================== SAVE TEXTURE INFO ==========================

	if (texture_offset != cftell(SaveFile))
		Error( "OFFSETS WRONG IN MINE.C!" );
	cfwrite( current_tmap_list, 13, NumTextures, SaveFile );
	
	//===================== SAVE VERTEX INFO ==========================

	if (vertex_offset != cftell(SaveFile))
		Error( "OFFSETS WRONG IN MINE.C!" );
	cfwrite( Vertices, sizeof(vms_vector), Num_vertices, SaveFile );

	//===================== SAVE SEGMENT INFO =========================

	if (segment_offset != cftell(SaveFile))
		Error( "OFFSETS WRONG IN MINE.C!" );
	cfwrite( Segments, sizeof(segment), Num_segments, SaveFile );

	//===================== SAVE NEWSEGMENT INFO ======================

	if (newsegment_offset != cftell(SaveFile))
		Error( "OFFSETS WRONG IN MINE.C!" );
	cfwrite( &New_segment, sizeof(segment), 1, SaveFile );

	if (newseg_verts_offset != cftell(SaveFile))
		Error( "OFFSETS WRONG IN MINE.C!" );
	cfwrite( &Vertices[New_segment.verts[0]], sizeof(vms_vector), 8, SaveFile );

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
		mprintf((1, "Warning: Fix (%8x) won't fit in short.  Saturating to %8x.\n", int_value, short_value<<nbits));
	}
	else if( int_value < -0x7fff ) {
		short_value = -0x7fff;
		mprintf((1, "Warning: Fix (%8x) won't fit in short.  Saturating to %8x.\n", int_value, short_value<<nbits));
	}
	else
		short_value = (short)int_value;

	cfwrite( &short_value, sizeof(short_value), 1, SaveFile );
}

//version of dump for unsigned values
void dump_fix_as_ushort( fix value, int nbits, CFILE * SaveFile )
{
        uint int_value=0;
	ushort short_value;

	if (value < 0) {
		mprintf((1, "Warning: fix (%8x) is signed...setting to zero.\n", value));
		Int3();		//hey---show this to Matt
		value = 0;
	}
	else
		int_value = value >> nbits;

	if( int_value > 0xffff ) {
		short_value = 0xffff;
		mprintf((1, "Warning: Fix (%8x) won't fit in unsigned short.  Saturating to %8x.\n", int_value, short_value<<nbits));
	}
	else
		short_value = int_value;

	cfwrite( &short_value, sizeof(short_value), 1, SaveFile );
}

int	New_file_format_save = 1;

// -----------------------------------------------------------------------------
// saves compiled mine data to an already-open file...
int save_mine_data_compiled(FILE * SaveFile)
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
	cfwrite( &version, sizeof(ubyte), 1, SaveFile );						// 1 byte = compiled version
	cfwrite( &Num_vertices, sizeof(int), 1, SaveFile );					// 4 bytes = Num_vertices
	cfwrite( &Num_segments, sizeof(int), 1, SaveFile );						// 4 bytes = Num_segments
	cfwrite( Vertices, sizeof(vms_vector), Num_vertices, SaveFile );

	for (segnum=0; segnum<Num_segments; segnum++ )	{
		// Write short Segments[segnum].children[MAX_SIDES_PER_SEGMENT]
 		cfwrite( &Segments[segnum].children, sizeof(short), MAX_SIDES_PER_SEGMENT, SaveFile );
		// Write short Segments[segnum].verts[MAX_VERTICES_PER_SEGMENT]
		cfwrite( &Segments[segnum].verts, sizeof(short), MAX_VERTICES_PER_SEGMENT, SaveFile );
		// Write ubyte	Segments[segnum].special
		cfwrite( &Segments[segnum].special, sizeof(ubyte), 1, SaveFile );
		// Write byte	Segments[segnum].matcen_num
		cfwrite( &Segments[segnum].matcen_num, sizeof(ubyte), 1, SaveFile );
		// Write short	Segments[segnum].value
		cfwrite( &Segments[segnum].value, sizeof(short), 1, SaveFile );
		// Write fix	Segments[segnum].static_light (shift down 5 bits, write as short)
		dump_fix_as_ushort( Segments[segnum].static_light, 4, SaveFile );
		//cfwrite( &Segments[segnum].static_light , sizeof(fix), 1, SaveFile );
	
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
			cfwrite( &byte_wallnum, sizeof(ubyte), 1, SaveFile );
		}

		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++ )	{
			if ( (Segments[segnum].children[sidenum]==-1) || (Segments[segnum].sides[sidenum].wall_num!=-1) )	{
				// Write short Segments[segnum].sides[sidenum].tmap_num;
				cfwrite( &Segments[segnum].sides[sidenum].tmap_num, sizeof(short), 1, SaveFile );
				// Write short Segments[segnum].sides[sidenum].tmap_num2;
				cfwrite( &Segments[segnum].sides[sidenum].tmap_num2, sizeof(short), 1, SaveFile );
				// Write uvl Segments[segnum].sides[sidenum].uvls[4] (u,v>>5, write as short, l>>1 write as short)
				for (i=0; i<4; i++ )	{
					dump_fix_as_short( Segments[segnum].sides[sidenum].uvls[i].u, 5, SaveFile );
					dump_fix_as_short( Segments[segnum].sides[sidenum].uvls[i].v, 5, SaveFile );
					dump_fix_as_ushort( Segments[segnum].sides[sidenum].uvls[i].l, 1, SaveFile );
					//cfwrite( &Segments[segnum].sides[sidenum].uvls[i].l, sizeof(fix), 1, SaveFile );
				}	
			}
		}

	}

	return 0;
}

// -----------------------------------------------------------------------------
// saves compiled mine data to an already-open file...
int save_mine_data_compiled_new(FILE * SaveFile)
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
	cfwrite( &version, sizeof(ubyte), 1, SaveFile );						// 1 byte = compiled version
	temp_short = Num_vertices;
	cfwrite( &temp_short, sizeof(short), 1, SaveFile );					// 2 bytes = Num_vertices
	temp_short = Num_segments;
	cfwrite( &temp_short, sizeof(short), 1, SaveFile );					// 2 bytes = Num_segments
	cfwrite( Vertices, sizeof(vms_vector), Num_vertices, SaveFile );

	for (segnum=0; segnum<Num_segments; segnum++ )	{

		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++) {
 			if (Segments[segnum].children[sidenum] != -1)
				bit_mask |= (1 << sidenum);
		}

		if ((Segments[segnum].special != 0) || (Segments[segnum].matcen_num != 0) || (Segments[segnum].value != 0))
			bit_mask |= (1 << MAX_SIDES_PER_SEGMENT);

 		cfwrite( &bit_mask, sizeof(ubyte), 1, SaveFile );

		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++) {
 			if (bit_mask & (1 << sidenum))
		 		cfwrite( &Segments[segnum].children[sidenum], sizeof(short), 1, SaveFile );
		}

		cfwrite( &Segments[segnum].verts, sizeof(short), MAX_VERTICES_PER_SEGMENT, SaveFile );

		if (bit_mask & (1 << MAX_SIDES_PER_SEGMENT)) {
			cfwrite( &Segments[segnum].special, sizeof(ubyte), 1, SaveFile );
			cfwrite( &Segments[segnum].matcen_num, sizeof(ubyte), 1, SaveFile );
			cfwrite( &Segments[segnum].value, sizeof(short), 1, SaveFile );
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
		cfwrite( &bit_mask, sizeof(ubyte), 1, SaveFile );

		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++ )	{
			if (bit_mask & (1 << sidenum))
				cfwrite( &Segments[segnum].sides[sidenum].wall_num, sizeof(ubyte), 1, SaveFile );
		}

		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++ )	{
			if ( (Segments[segnum].children[sidenum]==-1) || (Segments[segnum].sides[sidenum].wall_num!=-1) )	{
				ushort	tmap_num, tmap_num2;

				tmap_num = Segments[segnum].sides[sidenum].tmap_num;
				tmap_num2 = Segments[segnum].sides[sidenum].tmap_num2;
				if (tmap_num2 != 0)
					tmap_num |= 0x8000;

				cfwrite( &tmap_num, sizeof(ushort), 1, SaveFile );
				if (tmap_num2 != 0)
					cfwrite( &tmap_num2, sizeof(ushort), 1, SaveFile );

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


