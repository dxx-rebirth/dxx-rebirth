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

#ifndef _BM_H
#define _BM_H

#include "gr.h"
#include "piggy.h"

#define MAX_TEXTURES		1200

//tmapinfo flags
#define TMI_VOLATILE		1		//this material blows up when hit
#define TMI_WATER			2		//this material is water
#define TMI_FORCE_FIELD	4		//this is force field - flares don't stick
#define TMI_GOAL_BLUE	8		//this is used to remap the blue goal
#define TMI_GOAL_RED		16		//this is used to remap the red goal
#define TMI_GOAL_HOARD	32		//this is used to remap the goals

typedef struct {
	ubyte		flags;				//values defined above
	ubyte		pad[3];				//keep alignment
	fix		lighting;			//how much light this casts
	fix		damage;				//how much damage being against this does (for lava)
	short		eclip_num;			//the eclip that changes this, or -1
	short		destroyed;			//bitmap to show when destroyed, or -1
	short		slide_u,slide_v;	//slide rates of texture, stored in 8:8 fix
	#ifdef EDITOR
	char		filename[13];		//used by editor to remap textures
	char		pad2[3];
	#endif
} __pack__ tmap_info;

extern int Num_object_types;

#define N_COCKPIT_BITMAPS 6
extern int Num_cockpits;
extern bitmap_index cockpit_bitmap[N_COCKPIT_BITMAPS];

extern int Num_tmaps;
#ifdef EDITOR
extern int TmapList[MAX_TEXTURES];
#endif

extern tmap_info TmapInfo[MAX_TEXTURES];

//for each model, a model number for dying & dead variants, or -1 if none
extern int Dying_modelnums[];
extern int Dead_modelnums[];

//the model number of the marker object
extern int Marker_model_num;

// Initializes the palette, bitmap system...
int bm_init();
void bm_close();

// Initializes the Texture[] array of bmd_bitmap structures.
void init_textures();

#define OL_ROBOT 				1
#define OL_HOSTAGE 			2
#define OL_POWERUP 			3
#define OL_CONTROL_CENTER	4
#define OL_PLAYER				5
#define OL_CLUTTER			6		//some sort of misc object
#define OL_EXIT				7		//the exit model for external scenes
#define OL_WEAPON				8		//a weapon that can be placed

#define	MAX_OBJTYPE			140

extern int Num_total_object_types;		//	Total number of object types, including robots, hostages, powerups, control centers, faces
extern byte	ObjType[MAX_OBJTYPE];		// Type of an object, such as Robot, eg if ObjType[11] == OL_ROBOT, then object #11 is a robot
extern byte	ObjId[MAX_OBJTYPE];			// ID of a robot, within its class, eg if ObjType[11] == 3, then object #11 is the third robot
extern fix	ObjStrength[MAX_OBJTYPE];	// initial strength of each object

#define MAX_OBJ_BITMAPS				610

extern bitmap_index ObjBitmaps[MAX_OBJ_BITMAPS];
extern ushort ObjBitmapPtrs[MAX_OBJ_BITMAPS];
extern int First_multi_bitmap_num;

// Initializes all bitmaps from BITMAPS.TBL file.
int bm_init_use_tbl(void);

extern void bm_read_all(CFILE * fp);

void load_exit_models();
void free_exit_model_data();

#endif

