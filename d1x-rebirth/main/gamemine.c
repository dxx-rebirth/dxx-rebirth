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
 * Functions for loading mines in the game
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "inferno.h"
#include "segment.h"
#include "textures.h"
#include "wall.h"
#include "object.h"
#include "gamemine.h"
#include "gamesave.h"
#include "dxxerror.h"
#include "gameseg.h"
#include "switch.h"
#include "game.h"
#include "newmenu.h"
#ifdef EDITOR
#include "editor/editor.h"
#include "editor/esegment.h"
#endif
#include "fuelcen.h"
#include "hash.h"
#include "key.h"
#include "piggy.h"

#define REMOVE_EXT(s)  (*(strchr( (s), '.' ))='\0')


int CreateDefaultNewSegment();
int load_mine_data_compiled_new(PHYSFS_file *LoadFile);

#ifdef EDITOR

static char old_tmap_list[MAX_TEXTURES][13];
short tmap_xlate_table[MAX_TEXTURES];
static short tmap_times_used[MAX_TEXTURES];
struct mtfi mine_top_fileinfo;    // Should be same as first two fields below...
struct mfi mine_fileinfo;
struct mh mine_header;
struct me mine_editor;

// -----------------------------------------------------------------------------
//loads from an already-open file
// returns 0=everything ok, 1=old version, -1=error
int load_mine_data(PHYSFS_file *LoadFile)
{
	int   i, j;
	short tmap_xlate;
	int 	translate;
	char 	*temptr;
	int	mine_start = PHYSFS_tell(LoadFile);

	fuelcen_reset();

	for (i=0; i<MAX_TEXTURES; i++ )
		tmap_times_used[i] = 0;
	
	#ifdef EDITOR
	// Create a new mine to initialize things.
	//texpage_goto_first();
	create_new_mine();
	#endif

	//===================== READ FILE INFO ========================

	// These are the default values... version and fileinfo_sizeof
	// don't have defaults.
	mine_fileinfo.header_offset     =   -1;
	mine_fileinfo.header_size       =   sizeof(mine_header);
	mine_fileinfo.editor_offset     =   -1;
	mine_fileinfo.editor_size       =   sizeof(mine_editor);
	mine_fileinfo.vertex_offset     =   -1;
	mine_fileinfo.vertex_howmany    =   0;
	mine_fileinfo.vertex_sizeof     =   sizeof(vms_vector);
	mine_fileinfo.segment_offset    =   -1;
	mine_fileinfo.segment_howmany   =   0;
	mine_fileinfo.segment_sizeof    =   sizeof(segment);
	mine_fileinfo.newseg_verts_offset     =   -1;
	mine_fileinfo.newseg_verts_howmany    =   0;
	mine_fileinfo.newseg_verts_sizeof     =   sizeof(vms_vector);
	mine_fileinfo.group_offset		  =	-1;
	mine_fileinfo.group_howmany	  =	0;
	mine_fileinfo.group_sizeof		  =	sizeof(group);
	mine_fileinfo.texture_offset    =   -1;
	mine_fileinfo.texture_howmany   =   0;
	mine_fileinfo.texture_sizeof    =   13;  // num characters in a name
 	mine_fileinfo.walls_offset		  =	-1;
	mine_fileinfo.walls_howmany	  =	0;
	mine_fileinfo.walls_sizeof		  =	sizeof(wall);  
 	mine_fileinfo.triggers_offset	  =	-1;
	mine_fileinfo.triggers_howmany  =	0;
	mine_fileinfo.triggers_sizeof	  =	sizeof(trigger);  
	mine_fileinfo.object_offset		=	-1;
	mine_fileinfo.object_howmany		=	1;
	mine_fileinfo.object_sizeof		=	sizeof(object);  

	// Read in mine_top_fileinfo to get size of saved fileinfo.
	
	memset( &mine_top_fileinfo, 0, sizeof(mine_top_fileinfo) );

	if (PHYSFSX_fseek( LoadFile, mine_start, SEEK_SET ))
		Error( "Error moving to top of file in gamemine.c" );

	if (PHYSFS_read( LoadFile, &mine_top_fileinfo, sizeof(mine_top_fileinfo), 1 )!=1)
		Error( "Error reading mine_top_fileinfo in gamemine.c" );

	if (mine_top_fileinfo.fileinfo_signature != 0x2884)
		return -1;

	// Check version number
	if (mine_top_fileinfo.fileinfo_version < COMPATIBLE_VERSION )
		return -1;

	// Now, Read in the fileinfo
	if (PHYSFSX_fseek( LoadFile, mine_start, SEEK_SET ))
		Error( "Error seeking to top of file in gamemine.c" );

	if (PHYSFS_read( LoadFile, &mine_fileinfo, mine_top_fileinfo.fileinfo_sizeof, 1 )!=1)
		Error( "Error reading mine_fileinfo in gamemine.c" );

	//===================== READ HEADER INFO ========================

	// Set default values.
	mine_header.num_vertices        =   0;
	mine_header.num_segments        =   0;

	if (mine_fileinfo.header_offset > -1 )
	{
		if (PHYSFSX_fseek( LoadFile, mine_fileinfo.header_offset, SEEK_SET ))
			Error( "Error seeking to header_offset in gamemine.c" );
	
		if (PHYSFS_read( LoadFile, &mine_header, mine_fileinfo.header_size, 1 )!=1)
			Error( "Error reading mine_header in gamemine.c" );
	}

	//===================== READ EDITOR INFO ==========================

	// Set default values
	mine_editor.current_seg         =   0;
	mine_editor.newsegment_offset   =   -1; // To be written
	mine_editor.newsegment_size     =   sizeof(segment);
	mine_editor.Curside             =   0;
	mine_editor.Markedsegp          =   -1;
	mine_editor.Markedside          =   0;

	if (mine_fileinfo.editor_offset > -1 )
	{
		if (PHYSFSX_fseek( LoadFile, mine_fileinfo.editor_offset, SEEK_SET ))
			Error( "Error seeking to editor_offset in gamemine.c" );
	
		if (PHYSFS_read( LoadFile, &mine_editor, mine_fileinfo.editor_size, 1 )!=1)
			Error( "Error reading mine_editor in gamemine.c" );
	}

	//===================== READ TEXTURE INFO ==========================

	if ( (mine_fileinfo.texture_offset > -1) && (mine_fileinfo.texture_howmany > 0))
	{
		if (PHYSFSX_fseek( LoadFile, mine_fileinfo.texture_offset, SEEK_SET ))
			Error( "Error seeking to texture_offset in gamemine.c" );

		for (i=0; i< mine_fileinfo.texture_howmany; i++ )
		{
			if (PHYSFS_read( LoadFile, &old_tmap_list[i], mine_fileinfo.texture_sizeof, 1 )!=1)
				Error( "Error reading old_tmap_list[i] in gamemine.c" );
		}
	}

	//=============== GENERATE TEXTURE TRANSLATION TABLE ===============

	translate = 0;
	
	Assert (NumTextures < MAX_TEXTURES);

	{
		hashtable ht;
	
		hashtable_init( &ht, NumTextures );
	
		// Remove all the file extensions in the textures list
	
		for (i=0;i<NumTextures;i++)	{
			temptr = strchr(TmapInfo[i].filename, '.');
			if (temptr) *temptr = '\0';
			hashtable_insert( &ht, TmapInfo[i].filename, i );
		}
	
		// For every texture, search through the texture list
		// to find a matching name.
		for (j=0;j<mine_fileinfo.texture_howmany;j++) 	{
			// Remove this texture name's extension
			temptr = strchr(old_tmap_list[j], '.');
			if (temptr) *temptr = '\0';
	
			tmap_xlate_table[j] = hashtable_search( &ht,old_tmap_list[j]);
			if (tmap_xlate_table[j]	< 0 )	{
				;
			}
			if (tmap_xlate_table[j] != j ) translate = 1;
			if (tmap_xlate_table[j] >= 0)
				tmap_times_used[tmap_xlate_table[j]]++;
		}
	
		{
			int count = 0;
			for (i=0; i<MAX_TEXTURES; i++ )
				if (tmap_times_used[i])
					count++;
		}
	
		hashtable_free( &ht );
	}

	//====================== READ VERTEX INFO ==========================

	// New check added to make sure we don't read in too many vertices.
	if ( mine_fileinfo.vertex_howmany > MAX_VERTICES )
	{
		mine_fileinfo.vertex_howmany = MAX_VERTICES;
	}

	if ( (mine_fileinfo.vertex_offset > -1) && (mine_fileinfo.vertex_howmany > 0))
	{
		if (PHYSFSX_fseek( LoadFile, mine_fileinfo.vertex_offset, SEEK_SET ))
			Error( "Error seeking to vertex_offset in gamemine.c" );

		for (i=0; i< mine_fileinfo.vertex_howmany; i++ )
		{
			// Set the default values for this vertex
			Vertices[i].x = 1;
			Vertices[i].y = 1;
			Vertices[i].z = 1;

			if (PHYSFS_read( LoadFile, &Vertices[i], mine_fileinfo.vertex_sizeof, 1 )!=1)
				Error( "Error reading Vertices[i] in gamemine.c" );
		}
	}

	//==================== READ SEGMENT INFO ===========================

	// New check added to make sure we don't read in too many segments.
	if ( mine_fileinfo.segment_howmany > MAX_SEGMENTS ) {
		mine_fileinfo.segment_howmany = MAX_SEGMENTS;
	}

	// [commented out by mk on 11/20/94 (weren't we supposed to hit final in October?) because it looks redundant.  I think I'll test it now...]  fuelcen_reset();

	if ( (mine_fileinfo.segment_offset > -1) && (mine_fileinfo.segment_howmany > 0))	{

		if (PHYSFSX_fseek( LoadFile, mine_fileinfo.segment_offset,SEEK_SET ))

			Error( "Error seeking to segment_offset in gamemine.c" );

		Highest_segment_index = mine_fileinfo.segment_howmany-1;

		for (i=0; i< mine_fileinfo.segment_howmany; i++ ) {
			segment v16_seg;

			// Set the default values for this segment (clear to zero )
			//memset( &Segments[i], 0, sizeof(segment) );

			if (mine_top_fileinfo.fileinfo_version >= 16) {

				Assert(mine_fileinfo.segment_sizeof == sizeof(v16_seg));

				if (PHYSFS_read( LoadFile, &v16_seg, mine_fileinfo.segment_sizeof, 1 )!=1)
					Error( "Error reading segments in gamemine.c" );

			}				
			else 
				Error("Invalid mine version");

			Segments[i] = v16_seg;

			Segments[i].objects = -1;
			#ifdef EDITOR
			Segments[i].group = -1;
			#endif

			if (mine_top_fileinfo.fileinfo_version < 15) {	//used old uvl ranges
				int sn,uvln;

				for (sn=0;sn<MAX_SIDES_PER_SEGMENT;sn++)
					for (uvln=0;uvln<4;uvln++) {
						Segments[i].sides[sn].uvls[uvln].u /= 64;
						Segments[i].sides[sn].uvls[uvln].v /= 64;
						Segments[i].sides[sn].uvls[uvln].l /= 32;
					}
			}

			fuelcen_activate( &Segments[i], Segments[i].special );

			if (translate == 1)
				for (j=0;j<MAX_SIDES_PER_SEGMENT;j++) {
					unsigned short orient;
					tmap_xlate = Segments[i].sides[j].tmap_num;
					Segments[i].sides[j].tmap_num = tmap_xlate_table[tmap_xlate];
					if ((WALL_IS_DOORWAY(&Segments[i],j) & WID_RENDER_FLAG))
						if (Segments[i].sides[j].tmap_num < 0)	{
							Int3();
							Segments[i].sides[j].tmap_num = 0;
						}
					tmap_xlate = Segments[i].sides[j].tmap_num2 & 0x3FFF;
					orient = Segments[i].sides[j].tmap_num2 & (~0x3FFF);
					if (tmap_xlate != 0) {
						int xlated_tmap = tmap_xlate_table[tmap_xlate];

						if ((WALL_IS_DOORWAY(&Segments[i],j) & WID_RENDER_FLAG))
							if (xlated_tmap <= 0)	{
								Int3();
								Segments[i].sides[j].tmap_num2 = 0;
							}
						Segments[i].sides[j].tmap_num2 = xlated_tmap | orient;
					}
				}
		}
	}

	//===================== READ NEWSEGMENT INFO =====================

	#ifdef EDITOR

	{		// Default segment created.
		vms_vector	sizevec;
		med_create_new_segment(vm_vec_make(&sizevec,DEFAULT_X_SIZE,DEFAULT_Y_SIZE,DEFAULT_Z_SIZE));		// New_segment = Segments[0];
		//memset( &New_segment, 0, sizeof(segment) );
	}

	if (mine_editor.newsegment_offset > -1)
	{
		if (PHYSFSX_fseek( LoadFile, mine_editor.newsegment_offset,SEEK_SET ))
			Error( "Error seeking to newsegment_offset in gamemine.c" );
		if (PHYSFS_read( LoadFile, &New_segment, mine_editor.newsegment_size,1 )!=1)
			Error( "Error reading new_segment in gamemine.c" );
	}

	if ( (mine_fileinfo.newseg_verts_offset > -1) && (mine_fileinfo.newseg_verts_howmany > 0))
	{
		if (PHYSFSX_fseek( LoadFile, mine_fileinfo.newseg_verts_offset, SEEK_SET ))
			Error( "Error seeking to newseg_verts_offset in gamemine.c" );
		for (i=0; i< mine_fileinfo.newseg_verts_howmany; i++ )
		{
			// Set the default values for this vertex
			Vertices[NEW_SEGMENT_VERTICES+i].x = 1;
			Vertices[NEW_SEGMENT_VERTICES+i].y = 1;
			Vertices[NEW_SEGMENT_VERTICES+i].z = 1;
			
			if (PHYSFS_read( LoadFile, &Vertices[NEW_SEGMENT_VERTICES+i], mine_fileinfo.newseg_verts_sizeof,1 )!=1)
				Error( "Error reading Vertices[NEW_SEGMENT_VERTICES+i] in gamemine.c" );

			New_segment.verts[i] = NEW_SEGMENT_VERTICES+i;
		}
	}

	#endif
															
	//========================= UPDATE VARIABLES ======================

	#ifdef EDITOR

	// Setting to Markedsegp to NULL ignores Curside and Markedside, which
	// we want to do when reading in an old file.
	
 	Markedside = mine_editor.Markedside;
	Curside = mine_editor.Curside;
	for (i=0;i<10;i++)
		Groupside[i] = mine_editor.Groupside[i];

	if ( mine_editor.current_seg != -1 )
		Cursegp = mine_editor.current_seg + Segments;
	else
 		Cursegp = NULL;

	if (mine_editor.Markedsegp != -1 ) 
		Markedsegp = mine_editor.Markedsegp + Segments;
	else
		Markedsegp = NULL;

	num_groups = 0;
	current_group = -1;

	#endif

	Num_vertices = mine_fileinfo.vertex_howmany;
	Num_segments = mine_fileinfo.segment_howmany;
	Highest_vertex_index = Num_vertices-1;
	Highest_segment_index = Num_segments-1;

	reset_objects(1);		//one object, the player

	#ifdef EDITOR
	Highest_vertex_index = MAX_SEGMENT_VERTICES-1;
	Highest_segment_index = MAX_SEGMENTS-1;
	set_vertex_counts();
	Highest_vertex_index = Num_vertices-1;
	Highest_segment_index = Num_segments-1;

	warn_if_concave_segments();
	#endif

	#ifdef EDITOR
		validate_segment_all();
	#endif

	//create_local_segment_data();

	//gamemine_find_textures();

	if (mine_top_fileinfo.fileinfo_version < MINE_VERSION )
		return 1;		//old version
	else
		return 0;

}
#endif

#define COMPILED_MINE_VERSION 0

int	New_file_format_load = 1;

void read_children(int segnum,ubyte bit_mask,PHYSFS_file *LoadFile)
{
	int bit;
	
	for (bit=0; bit<MAX_SIDES_PER_SEGMENT; bit++) {
		if (bit_mask & (1 << bit)) {
			Segments[segnum].children[bit] = PHYSFSX_readShort(LoadFile);
		} else
			Segments[segnum].children[bit] = -1;
	}
}

void read_verts(int segnum,PHYSFS_file *LoadFile)
{
	int i;
	// Read short Segments[segnum].verts[MAX_VERTICES_PER_SEGMENT]
	for (i = 0; i < MAX_VERTICES_PER_SEGMENT; i++)
		Segments[segnum].verts[i] = PHYSFSX_readShort(LoadFile);
}

void read_special(int segnum,ubyte bit_mask,PHYSFS_file *LoadFile)
{
	if (bit_mask & (1 << MAX_SIDES_PER_SEGMENT)) {
		// Read ubyte	Segments[segnum].special
		Segments[segnum].special = PHYSFSX_readByte(LoadFile);
		// Read byte	Segments[segnum].matcen_num
		Segments[segnum].matcen_num = PHYSFSX_readByte(LoadFile);
		// Read short	Segments[segnum].value
		Segments[segnum].value = PHYSFSX_readShort(LoadFile);
	} else {
		Segments[segnum].special = 0;
		Segments[segnum].matcen_num = -1;
		Segments[segnum].value = 0;
	}
}

/*
 * reads a segment2 structure from a PHYSFS_file
 */
void segment2_read(segment *s2, PHYSFS_file *fp)
{
	s2->special = PHYSFSX_readByte(fp);
	if (s2->special >= MAX_CENTER_TYPES)
		s2->special = SEGMENT_IS_NOTHING; // remove goals etc.
	s2->matcen_num = PHYSFSX_readByte(fp);
	s2->value = PHYSFSX_readByte(fp);
	/*s2->s2_flags =*/ PHYSFSX_readByte(fp);	// descent 2 ambient sound handling
	s2->static_light = PHYSFSX_readFix(fp);
}

int load_mine_data_compiled(PHYSFS_file *LoadFile)
{
	int     i, segnum, sidenum;
	ubyte   compiled_version;
	short   temp_short;
	ushort  temp_ushort = 0;
	ubyte   bit_mask;
	
	if (!strcmp(strchr(Gamesave_current_filename, '.'), ".sdl"))
		New_file_format_load = 0; // descent 1 shareware
	else
		New_file_format_load = 1;
	
	//	For compiled levels, textures map to themselves, prevent tmap_override always being gray,
	//	bug which Matt and John refused to acknowledge, so here is Mike, fixing it.
	// 
	// Although in a cloud of arrogant glee, he forgot to ifdef it on EDITOR!
	// (Matt told me to write that!)
#ifdef EDITOR
	for (i=0; i<MAX_TEXTURES; i++)
		tmap_xlate_table[i] = i;
#endif
//	memset( Segments, 0, sizeof(segment)*MAX_SEGMENTS );
	fuelcen_reset();

	//=============================== Reading part ==============================
	compiled_version = PHYSFSX_readByte(LoadFile);
	(void)compiled_version;

	if (New_file_format_load)
		Num_vertices = PHYSFSX_readShort(LoadFile);
	else
		Num_vertices = PHYSFSX_readInt(LoadFile);
	Assert( Num_vertices <= MAX_VERTICES );
	
	if (New_file_format_load)
		Num_segments = PHYSFSX_readShort(LoadFile);
	else
		Num_segments = PHYSFSX_readInt(LoadFile);
	Assert( Num_segments <= MAX_SEGMENTS );
	
	for (i = 0; i < Num_vertices; i++)
		PHYSFSX_readVector( &(Vertices[i]), LoadFile);
	
	for (segnum=0; segnum<Num_segments; segnum++ )	{

		#ifdef EDITOR
		Segments[segnum].segnum = segnum;
		Segments[segnum].group = 0;
		#endif

		if (New_file_format_load)
			bit_mask = PHYSFSX_readByte(LoadFile);
		else
			bit_mask = 0x7f; // read all six children and special stuff...
		
		if (Gamesave_current_version == 5) { // d2 SHAREWARE level
			read_special(segnum,bit_mask,LoadFile);
			read_verts(segnum,LoadFile);
			read_children(segnum,bit_mask,LoadFile);
		} else {
			read_children(segnum,bit_mask,LoadFile);
			read_verts(segnum,LoadFile);
			if (Gamesave_current_version <= 1) { // descent 1 level
				read_special(segnum,bit_mask,LoadFile);
			}
		}
		
		Segments[segnum].objects = -1;

		if (Gamesave_current_version <= 5) { // descent 1 thru d2 SHAREWARE level
											 // Read fix	Segments[segnum].static_light (shift down 5 bits, write as short)
			temp_ushort = PHYSFSX_readShort(LoadFile);
			Segments[segnum].static_light	= ((fix)temp_ushort) << 4;
			//PHYSFS_read( LoadFile, &Segments[segnum].static_light, sizeof(fix), 1 );
		}
		
		// Read the walls as a 6 byte array
		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++ )	{
			Segments[segnum].sides[sidenum].pad = 0;
		}
		
		if (New_file_format_load)
			bit_mask = PHYSFSX_readByte(LoadFile);
		else
			bit_mask = 0x3f; // read all six sides
		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++) {
			ubyte byte_wallnum;
			
			if (bit_mask & (1 << sidenum)) {
				byte_wallnum = PHYSFSX_readByte(LoadFile);
				if ( byte_wallnum == 255 )
					Segments[segnum].sides[sidenum].wall_num = -1;
				else
					Segments[segnum].sides[sidenum].wall_num = byte_wallnum;
			} else
				Segments[segnum].sides[sidenum].wall_num = -1;
		}
		
		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++ )	{
			
			if ( (Segments[segnum].children[sidenum]==-1) || (Segments[segnum].sides[sidenum].wall_num!=-1) )	{
				// Read short Segments[segnum].sides[sidenum].tmap_num;
				temp_ushort = PHYSFSX_readShort(LoadFile);

				Segments[segnum].sides[sidenum].tmap_num = convert_tmap(temp_ushort & 0x7fff);

				if (New_file_format_load && !(temp_ushort & 0x8000))
					Segments[segnum].sides[sidenum].tmap_num2 = 0;
				else {
					// Read short Segments[segnum].sides[sidenum].tmap_num2;
					Segments[segnum].sides[sidenum].tmap_num2 = PHYSFSX_readShort(LoadFile);
					Segments[segnum].sides[sidenum].tmap_num2 =
						(convert_tmap(Segments[segnum].sides[sidenum].tmap_num2 & 0x3fff)) |
						(Segments[segnum].sides[sidenum].tmap_num2 & 0xc000);
				}
				
				// Read uvl Segments[segnum].sides[sidenum].uvls[4] (u,v>>5, write as short, l>>1 write as short)
				for (i=0; i<4; i++ )	{
					temp_short = PHYSFSX_readShort(LoadFile);
					Segments[segnum].sides[sidenum].uvls[i].u = ((fix)temp_short) << 5;
					temp_short = PHYSFSX_readShort(LoadFile);
					Segments[segnum].sides[sidenum].uvls[i].v = ((fix)temp_short) << 5;
					temp_ushort = PHYSFSX_readShort(LoadFile);
					Segments[segnum].sides[sidenum].uvls[i].l = ((fix)temp_ushort) << 1;
					//PHYSFS_read( LoadFile, &Segments[segnum].sides[sidenum].uvls[i].l, sizeof(fix), 1 );
				}
			} else {
				Segments[segnum].sides[sidenum].tmap_num = 0;
				Segments[segnum].sides[sidenum].tmap_num2 = 0;
				for (i=0; i<4; i++ )	{
					Segments[segnum].sides[sidenum].uvls[i].u = 0;
					Segments[segnum].sides[sidenum].uvls[i].v = 0;
					Segments[segnum].sides[sidenum].uvls[i].l = 0;
				}
			}
		}
	}
	
	Highest_vertex_index = Num_vertices-1;
	Highest_segment_index = Num_segments-1;

	validate_segment_all();			// Fill in side type and normals.

	for (i=0; i<Num_segments; i++) {
		if (Gamesave_current_version > 5)
			segment2_read(&Segments[i], LoadFile);
		fuelcen_activate( &Segments[i], Segments[i].special );
	}
	
	reset_objects(1);		//one object, the player

	return 0;
}
