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
 * Print debugging info in ui.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef DO_MEMINFO
#include <malloc.h>
#endif

#include "inferno.h"
#include "window.h"
#include "segment.h"
#include "gr.h"
#include "ui.h"
#include "editor.h"
#include "editor/esegment.h"
#include "dxxerror.h"
#include "textures.h"
#include "object.h"
#include "ai.h"
#include "texpage.h"		// Textue selection paging stuff
#include "objpage.h"		// Object selection paging stuff
#include "wall.h"
#include "switch.h"

int init_info;

#ifdef DO_MEMINFO
struct meminfo {
    int LargestBlockAvail;
    int MaxUnlockedPage;
    int LargestLockablePage;
    int LinAddrSpace;
    int NumFreePagesAvail;
    int NumPhysicalPagesFree;
    int TotalPhysicalPages;
    int FreeLinAddrSpace;
    int SizeOfPageFile;
    int Reserved[3];
} MemInfo;

#define DPMI_INT        0x31

void read_mem_info()
{
    union REGS regs;
    struct SREGS sregs;

    regs.x.eax = 0x00000500;
    memset( &sregs, 0, sizeof(sregs) );
    sregs.es = FP_SEG( &MemInfo );
    regs.x.edi = FP_OFF( &MemInfo );

    int386x( DPMI_INT, &regs, &regs, &sregs );
}
#endif

char * get_object_type(int num, char *name)
{
	switch (num) {
		case OBJ_NONE:			strcpy(name, "OBJ_NONE    ");	break;
		case OBJ_WALL: 		strcpy(name, "OBJ_WALL    ");	break;
		case OBJ_FIREBALL: 	strcpy(name, "OBJ_FIREBALL");	break;
		case OBJ_ROBOT: 		strcpy(name, "OBJ_ROBOT   ");	break;
		case OBJ_HOSTAGE: 	strcpy(name, "OBJ_HOSTAGE ");	break;
		case OBJ_PLAYER: 		strcpy(name, "OBJ_PLAYER  ");	break;
		case OBJ_WEAPON: 		strcpy(name, "OBJ_WEAPON  ");	break;
		case OBJ_CAMERA: 		strcpy(name, "OBJ_CAMERA  ");	break;
		case OBJ_POWERUP: 	strcpy(name, "OBJ_POWERUP ");	break;
		default:					strcpy(name, " (unknown)  ");	break;
	}

	return name;
}

char * get_control_type(int num, char *name)
{
	switch (num) {
		case CT_NONE:					strcpy(name, "CT_NONE       ");	break;
		case CT_AI:						strcpy(name, "CT_AI         ");	break;
		case CT_EXPLOSION:			strcpy(name, "CT_EXPLOSION  ");	break;
		//case CT_MULTIPLAYER:			strcpy(name, "CT_MULTIPLAYER");	break;
		case CT_FLYING:				strcpy(name, "CT_FLYING     ");	break;
		case CT_SLEW:					strcpy(name, "CT_SLEW       ");	break;
		case CT_FLYTHROUGH:			strcpy(name, "CT_FLYTHROUGH ");	break;
		//case CT_DEMO:					strcpy(name, "CT_DEMO       ");	break;
		//case CT_ROBOT_FLYTHROUGH:	strcpy(name, "CT_FLYTHROUGH ");	break;
		case CT_WEAPON:				strcpy(name, "CT_WEAPON     ");	break;
		default:							strcpy(name, " (unknown)    ");	break;
	}
	return name;
}

char * get_movement_type(int num, char *name)
{
	switch (num) {
		case MT_NONE:			strcpy(name, "MT_NONE       ");	break;
		case MT_PHYSICS:		strcpy(name, "MT_PHYSICS    ");	break;
		//case MT_MULTIPLAYER:	strcpy(name, "MT_MULTIPLAYER");	break;
		default:					strcpy(name, " (unknown)    ");	break;
	}
	return name;
}

char * get_ai_behavior(int num, char *name)
{
#define	AIB_STILL						0x80
#define	AIB_NORMAL						0x81
#define	AIB_HIDE							0x82
#define	AIB_RUN_FROM					0x83
#define	AIB_FOLLOW_PATH				0x84

	switch (num) {
		case AIB_STILL:				strcpy(name, "STILL       ");	break;
		case AIB_NORMAL:				strcpy(name, "NORMAL      ");	break;
		case AIB_HIDE:					strcpy(name, "HIDE        ");	break;
		case AIB_RUN_FROM:			strcpy(name, "RUN_FROM    ");	break;
		case AIB_FOLLOW_PATH:		strcpy(name, "FOLLOW_PATH ");	break;
		default:							strcpy(name, " (unknown)  ");	break;
	}
	return name;
}

//	---------------------------------------------------------------------------------------------------
void info_display_object_placement(int show_all)
{
	static	int	old_Cur_object_index;
	static	int	old_type;
	static	int	old_movement_type;
	static	int	old_control_type;
	static	int	old_mode;

	char		name[30];

	if (init_info | show_all) {
		old_Cur_object_index = -2;
		old_type = -2;
		old_movement_type = -2;
		old_control_type = -2;
		old_mode = -2;
	}

	if ( ( Cur_object_index != old_Cur_object_index) || 
			( Objects[Cur_object_index].type != old_type) || 
			( Objects[Cur_object_index].movement_type != old_movement_type) || 
			( Objects[Cur_object_index].control_type != old_control_type) || 
			( Objects[Cur_object_index].ctype.ai_info.behavior != old_mode) ) {

		gr_uprintf( 0, 0, "Object id: %4d\n", Cur_object_index);
		gr_uprintf( 0, 16, "Type: %s\n", get_object_type(Objects[Cur_object_index].type , name));
		gr_uprintf( 0, 32, "Movmnt: %s\n", get_movement_type(Objects[Cur_object_index].movement_type, name));
		gr_uprintf( 0, 48, "Cntrl: %s\n", get_control_type(Objects[Cur_object_index].control_type, name));
		gr_uprintf( 0, 64, "Mode: %s\n", get_ai_behavior(Objects[Cur_object_index].ctype.ai_info.behavior, name));

		old_Cur_object_index = Cur_object_index;
		old_type = Objects[Cur_object_index].type;
		old_movement_type = Objects[Cur_object_index].movement_type;
		old_mode = Objects[Cur_object_index].control_type;
		old_mode = Objects[Cur_object_index].ctype.ai_info.behavior;
	}

}

//	---------------------------------------------------------------------------------------------------
void info_display_segsize(int show_all)
{
	static	int	old_SegSizeMode;

	char		name[30];

	if (init_info | show_all) {
		old_SegSizeMode = -2;
	}

	if (old_SegSizeMode != SegSizeMode  ) {
		switch (SegSizeMode) {
			case SEGSIZEMODE_FREE:		strcpy(name, "free   ");	break;
			case SEGSIZEMODE_ALL:		strcpy(name, "all    ");	break;
			case SEGSIZEMODE_CURSIDE:	strcpy(name, "curside");	break;
			case SEGSIZEMODE_EDGE:		strcpy(name, "edge   ");	break;
			case SEGSIZEMODE_VERTEX:	strcpy(name, "vertex ");	break;
			default:
				Error("Illegal value for SegSizeMode in info.c/info_display_segsize\n");
		}

		gr_uprintf( 0, 0, "Mode: %s\n", name);

		old_SegSizeMode = SegSizeMode;
	}

}

extern int num_objects;

//	---------------------------------------------------------------------------------------------------
void info_display_default(int show_all)
{
	static int old_Num_segments = -1;
	static int old_Num_vertices = -1;
	static int old_Num_objects = -1;
	static int old_Cursegp_num = -1;
	static int old_Curside = -1;
	static int old_Cursegp_num_for_verts = -1;
	static int old_CurrentTexture = -1;
	static int old_Num_walls = -1;
	static int old_Num_triggers = -1;

	if (init_info | show_all) {
		init_info = 0;
		old_Num_segments = -1;
		old_Num_vertices = -1;
		old_Num_objects = -1;
		old_Cursegp_num = -1;
		old_Cursegp_num_for_verts = -1;
		old_Curside = -1;
		old_CurrentTexture = -1;
		old_Num_walls = -1;
		old_Num_triggers = -1;
	}

	gr_set_fontcolor(CBLACK,CWHITE);

	//--------------- Number of segments ----------------

	if ( old_Num_segments != Num_segments )	{
		gr_uprintf( 0, 0, "Segments: %4d/%4d", Num_segments, MAX_SEGMENTS );
		old_Num_segments = Num_segments;
	}

	//---------------- Number of vertics -----------------
	
	if ( old_Num_vertices != Num_vertices )	{
		gr_uprintf( 0, 16, "Vertices: %4d/%4d", Num_vertices, MAX_VERTICES );
		old_Num_vertices = Num_vertices;
	}

	//---------------- Number of objects -----------------
	
	if ( old_Num_objects != num_objects )	{
		gr_uprintf( 0, 32, "Objs: %3d/%3d", num_objects, MAX_OBJECTS );
		old_Num_objects = num_objects;
	}

  	//--------------- Current_segment_number -------------
	//--------------- Current_side_number -------------

	if (( old_Cursegp_num != Cursegp-Segments ) || ( old_Curside != Curside ))	{
		gr_uprintf( 0, 48, "Cursegp/side: %3ld/%1d", Cursegp-Segments, Curside);
		gr_uprintf( 0, 128, " tmap1,2,o: %3d/%3dx%1d", Cursegp->sides[Curside].tmap_num, Cursegp->sides[Curside].tmap_num2 & 0x3FFF, (Cursegp->sides[Curside].tmap_num2 >> 14) & 3);
		old_Cursegp_num = Cursegp-Segments;
		old_Curside = Curside;
	}

	//--------------- Current_vertex_numbers -------------

	if ( old_Cursegp_num_for_verts != Cursegp-Segments )	{

		gr_uprintf( 0, 64, "{%3d,%3d,%3d,%3d,", Cursegp->verts[0],Cursegp->verts[1],
																							 Cursegp->verts[2],Cursegp->verts[3] );
		gr_uprintf( 0, 80," %3d,%3d,%3d,%3d}", Cursegp->verts[4],Cursegp->verts[5],
																							 Cursegp->verts[6],Cursegp->verts[7] );
		old_Cursegp_num_for_verts = Cursegp-Segments;
	}

	//--------------- Num walls/links/triggers -------------------------

	if ( old_Num_walls != Num_walls ) {
//		gr_uprintf( 0, 96, "Walls/Links %d/%d", Num_walls, Num_links );
		gr_uprintf( 0, 96, "Walls %3d", Num_walls );
		old_Num_walls = Num_walls;
	}

	//--------------- Num triggers ----------------------

	if ( old_Num_triggers != Num_triggers ) {
		gr_uprintf( 0, 112, "Num_triggers %2d", Num_triggers );
		old_Num_triggers = Num_triggers;
	}

	//--------------- Current texture number -------------

	if ( old_CurrentTexture != CurrentTexture )	{
		gr_uprintf( 0, 144, "Tex/Light: %3d %5.2f", CurrentTexture, f2fl(TmapInfo[CurrentTexture].lighting));
		old_CurrentTexture = CurrentTexture;
	}

}

//	------------------------------------------------------------------------------------
void clear_pad_display(void)
{
	gr_clear_canvas(CWHITE);
   gr_set_fontcolor( CBLACK, CWHITE );
}

//	------------------------------------------------------------------------------------
int info_display_all(window *wind, d_event *event, void *userdata)
{
	static int old_padnum = -1;
	int        padnum,show_all = 1;		// always redraw
	grs_canvas *save_canvas = grd_curcanv;

	switch (event->type)
	{
		case EVENT_WINDOW_DRAW:
			userdata++;		//kill warning

			gr_set_current_canvas(window_get_canvas(wind));

			padnum = ui_pad_get_current();
			Assert(padnum <= MAX_PAD_ID);

			if (padnum != old_padnum) {
				clear_pad_display();
				old_padnum = padnum;
				//show_all = 1;
			}

			switch (padnum) {
				case OBJECT_PAD_ID:			// Object placement
					info_display_object_placement(show_all);
					break;
				case SEGSIZE_PAD_ID:			// Segment sizing
					info_display_segsize(show_all);
					break;
				default:
					info_display_default(show_all);
					break;
			}
			grd_curcanv = save_canvas;
			return 1;
			
		case EVENT_WINDOW_CLOSE:
			Pad_info = NULL;
			break;
			
		default:
			break;
	}
	
	return 0;
}

//	------------------------------------------------------------------------------------
window *info_window_create(void)
{
	window *wind;
	
	wind = window_create(Canv_editor, PAD_X + 250, PAD_Y + 8, 180, 160, info_display_all, NULL);
	if (wind)
		window_set_modal(wind, 0);
	
	return wind;
}


