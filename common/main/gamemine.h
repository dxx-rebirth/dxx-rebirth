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
 * Header for gamemine.c
 *
 */

#pragma once

#include <physfs.h>
#include "fwd-segment.h"
#include "maths.h"

#define TMAP_NUM_MASK 0x3FFF

#if defined(DXX_BUILD_DESCENT_I)
#define MINE_VERSION					17	// Current version expected
#elif defined(DXX_BUILD_DESCENT_II)
#define MINE_VERSION        20  // Current version expected
#include "vecmat.h"
#endif
#define COMPATIBLE_VERSION  16  // Oldest version that can safely be loaded.

#if DXX_USE_EDITOR
struct mtfi {
	ushort  fileinfo_signature;
	ushort  fileinfo_version;
	int     fileinfo_sizeof;
};    // Should be same as first two fields below...

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
struct mfi {
	ushort  fileinfo_signature;
	ushort  fileinfo_version;
	int     fileinfo_sizeof;
	int     header_offset;      // Stuff common to game & editor
	int     header_size;
	int     editor_offset;      // Editor specific stuff
	int     editor_size;
	int     segment_offset;
	int     segment_howmany;
	int     segment_sizeof;
	int     newseg_verts_offset;
	int     newseg_verts_howmany;
	int     newseg_verts_sizeof;
	int     group_offset;
	int     group_howmany;
	int     group_sizeof;
	int     vertex_offset;
	unsigned vertex_howmany;
	int     vertex_sizeof;
	int     texture_offset;
	uint32_t texture_howmany;
	int     texture_sizeof;
	int     walls_offset;
	int     walls_howmany;
	int     walls_sizeof;
	int     triggers_offset;
	int     triggers_howmany;
	int     triggers_sizeof;
	int     links_offset;
	int     links_howmany;
	int     links_sizeof;
	int     object_offset;      // Object info
	int     object_howmany;
	int     object_sizeof;
	int     unused_offset;      // was: doors_offset
	int     unused_howmamy;     // was: doors_howmany
	int     unused_sizeof;      // was: doors_sizeof
#if defined(DXX_BUILD_DESCENT_II)
	short   level_shake_frequency, level_shake_duration;
	// Shakes every level_shake_frequency seconds
	// for level_shake_duration seconds (on average, random).  In 16ths second.
	int     secret_return_segment;
	vms_matrix secret_return_orient;

	int     dl_indices_offset;
	int     dl_indices_howmany;
	int     dl_indices_sizeof;

	int     delta_light_offset;
	int     delta_light_howmany;
	int     delta_light_sizeof;

	int     segment2_offset;
	int     segment2_howmany;
	int     segment2_sizeof;
#endif
};
#endif
#endif // EDITOR

struct mh {
	int num_vertices;
	int num_segments;
};

struct me {
	int current_seg;
	int newsegment_offset;
	int newsegment_size;
	int Curside;
	int Markedsegp;
	int Markedside;
	int Groupsegp[10];
	int Groupside[10];
	int num_groups;
	int current_group;
	//int num_objects;
};

extern struct mtfi mine_top_fileinfo;   // Should be same as first two fields below...
extern struct mfi mine_fileinfo;
extern struct mh mine_header;
extern struct me mine_editor;

// loads from an already-open file
// returns 0=everything ok, 1=old version, -1=error
#ifdef dsx
namespace dsx {
int load_mine_data_compiled(PHYSFS_File *LoadFile, const char *Gamesave_current_filename);
}
#endif
#define TMAP_NUM_MASK 0x3FFF

#if defined(DXX_BUILD_DESCENT_II)
namespace dsx {
extern int d1_pig_present;

/* stuff for loading descent.pig of descent 1 */
uint16_t convert_d1_tmap_num(uint16_t d1_tmap_num);
}
#endif
