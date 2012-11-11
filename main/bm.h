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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Bitmap and Palette loading functions.
 *
 */

#ifndef _BM_H
#define _BM_H

#include "gr.h"
#include "piggy.h"

#define MAX_TEXTURES    1200

//tmapinfo flags
#define TMI_VOLATILE    1   //this material blows up when hit
#define TMI_WATER       2   //this material is water
#define TMI_FORCE_FIELD 4   //this is force field - flares don't stick
#define TMI_GOAL_BLUE   8   //this is used to remap the blue goal
#define TMI_GOAL_RED    16  //this is used to remap the red goal
#define TMI_GOAL_HOARD  32  //this is used to remap the goals

typedef struct {
	ubyte   flags;     //values defined above
	ubyte   pad[3];    //keep alignment
	fix     lighting;  //how much light this casts
	fix     damage;    //how much damage being against this does (for lava)
	short   eclip_num; //the eclip that changes this, or -1
	short   destroyed; //bitmap to show when destroyed, or -1
	short   slide_u,slide_v;    //slide rates of texture, stored in 8:8 fix
	#ifdef EDITOR
	char    filename[13];       //used by editor to remap textures
	char    pad2[3];
	#endif
} __pack__ tmap_info;

#define TMAP_INFO_SIZE 20   // how much space it takes up on disk

extern int Num_object_types;

#define N_COCKPIT_BITMAPS 6
struct player_ship;
extern struct player_ship only_player_ship;
extern int Num_cockpits;
extern bitmap_index cockpit_bitmap[N_COCKPIT_BITMAPS];
extern short tmap_xlate_table[MAX_TEXTURES];

extern int Num_tmaps;
#ifdef EDITOR
extern int TmapList[MAX_TEXTURES];
#endif

extern tmap_info TmapInfo[MAX_TEXTURES];

//the model number of the marker object
extern int Marker_model_num;

// Initializes the palette, bitmap system...
void gamedata_close();
int gamedata_init();
void bm_close();
void bm_free_extra_objbitmaps();

// Initializes the Texture[] array of bmd_bitmap structures.
void init_textures();

extern int  Num_object_subtypes;     // Number of possible IDs for the current type of object to be placed

#define MAX_OBJ_BITMAPS     610

extern int N_ObjBitmaps;
extern bitmap_index ObjBitmaps[MAX_OBJ_BITMAPS];
extern ushort ObjBitmapPtrs[MAX_OBJ_BITMAPS];
extern int First_multi_bitmap_num;
void compute_average_rgb(grs_bitmap *bm, fix *rgb);

// Initializes all bitmaps from BITMAPS.TBL file.
int gamedata_read_tbl(int pc_shareware);

extern void bm_read_all(PHYSFS_file * fp);

int load_exit_models();

#endif /* _BM_H */
