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
 * $Source: /cvs/cvsroot/d2x/main/editor/info.c,v $
 * $Revision: 1.1 $
 * $Author: btb $
 * $Date: 2004-12-19 13:54:27 $
 * 
 * Print debugging info in ui.
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  1999/06/14 22:03:18  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:35:34  john
 * Version 2.0! No anonymous unions, Watcom 10.0, with no need
 * for bitmaps.tbl.
 * 
 * Revision 1.42  1995/02/22  15:12:50  allender
 * remove anonymous unions from object structure
 * 
 * Revision 1.41  1994/12/08  13:59:39  matt
 * *** empty log message ***
 * 
 * Revision 1.40  1994/09/30  00:38:30  mike
 * Fix some diagnostic messages
 * 
 * Revision 1.39  1994/09/29  20:13:12  mike
 * Clean up some text, prevent it from writing outside canvas.
 * 
 * Revision 1.38  1994/09/29  09:32:17  mike
 * Fix text clipping problem in UI keypad info text.
 * 
 * Revision 1.37  1994/09/25  23:42:20  matt
 * Took out references to obsolete constants
 * 
 * Revision 1.36  1994/08/25  21:57:05  mike
 * IS_CHILD stuff.
 * 
 * Revision 1.35  1994/08/23  16:39:50  mike
 * mode replaced by behavior in ai_info.
 * 
 * Revision 1.34  1994/07/18  10:45:23  mike
 * Fix erase window in texture pads after adding more click-boxes.
 * 
 * Revision 1.33  1994/07/15  12:34:10  mike
 * Remove use of AIM_FOLLOW_PATH_CIRCULAR.
 * 
 * Revision 1.32  1994/06/17  17:13:46  yuan
 * Fixed text so it doesn't overflow screen
 * 
 * Revision 1.31  1994/06/01  17:22:31  matt
 * Set font color before drawing info; got rid of superfluous %d
 * 
 * Revision 1.30  1994/05/29  23:40:29  matt
 * Killed reference to now-unused movement type
 * 
 * Revision 1.29  1994/05/29  22:52:32  matt
 * Deleted unused stuff
 * 
 * Revision 1.28  1994/05/27  10:34:16  yuan
 * Added new Dialog boxes for Walls and Triggers.
 * 
 * Revision 1.27  1994/05/17  10:34:35  matt
 * Changed Num_objects to num_objects, since it's not really global anymore
 * 
 * Revision 1.26  1994/05/14  17:17:59  matt
 * Got rid of externs in source (non-header) files
 * 
 * Revision 1.25  1994/05/12  14:47:07  mike
 * Adjust for Ai_states killed, replaced by field in object structure.
 * 
 * Revision 1.24  1994/05/06  12:52:11  yuan
 * Adding some gamesave checks...
 * 
 * Revision 1.23  1994/05/03  19:21:28  matt
 * Removed reference to robot flythrough mode, which doesn't exist anymore
 * 
 * Revision 1.22  1994/05/03  11:03:06  mike
 * Customize text for segment sizing keypad.
 * 
 * Revision 1.21  1994/04/29  15:05:40  yuan
 * More info added...
 * 
 * Revision 1.20  1994/04/22  17:45:58  john
 * MAde top 2 bits of paste-ons pick the 
 * orientation of the bitmap.
 * 
 * Revision 1.19  1994/04/20  17:29:30  yuan
 * Added tmap_num info.
 * 
 * Revision 1.18  1994/04/13  19:12:55  mike
 * Fix font color problems in keypads.
 * 
 * Revision 1.17  1994/04/13  13:26:37  mike
 * Kill a mprintf.
 * 
 * Revision 1.16  1994/04/13  13:24:44  mike
 * Separate info display, customize for each keypad.
 * 
 * Revision 1.15  1994/03/19  17:21:31  yuan
 * Wall system implemented until specific features need to be added...
 * (Needs to be hammered on though.)
 * 
 * Revision 1.14  1994/02/22  18:13:13  yuan
 * Added tmap number field.
 * 
 * Revision 1.13  1994/02/17  09:46:27  matt
 * Removed include of slew.h
 * 
 * Revision 1.12  1994/02/16  19:58:56  yuan
 * Added type to info
 * 
 * Revision 1.11  1994/02/16  16:48:08  yuan
 * Added Curside.
 * 
 * Revision 1.10  1994/02/03  17:26:43  yuan
 * Fixed formatting of vertex numbering.
 * 
 * Revision 1.9  1994/01/31  12:17:06  yuan
 * Make sure Num_segments, etc. are drawn.
 * 
 * Revision 1.8  1994/01/22  13:43:12  yuan
 * Cosmetic fixes.
 * 
 * Revision 1.7  1994/01/21  12:14:59  yuan
 * Fixed cosmetic problem
 * 
 * Revision 1.6  1994/01/21  12:01:03  yuan
 * Added segment and vertex info
 * 
 * Revision 1.5  1994/01/20  11:28:11  john
 * *** empty log message ***
 * 
 * Revision 1.4  1994/01/19  10:44:42  john
 * *** empty log message ***
 * 
 * Revision 1.3  1994/01/19  10:32:36  john
 * *** empty log message ***
 * 
 * Revision 1.2  1994/01/19  09:34:31  john
 * First version.
 * 
 * Revision 1.1  1994/01/19  09:30:43  john
 * Initial revision
 * 
 * 
 */


#ifdef RCS
static char rcsid[] = "$Id: info.c,v 1.1 2004-12-19 13:54:27 btb Exp $";
#endif

#include <i86.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>												  
#include <malloc.h>

#include "inferno.h"
#include "segment.h"
#include "gr.h"
#include "ui.h"
#include "editor.h"

#include "mono.h"
#include "error.h"
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
		gr_uprintf( 0, 48, "Cursegp/side: %3d/%1d", Cursegp-Segments, Curside);
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
void info_display_all( UI_WINDOW * wnd )
{
        static int old_padnum = -1;
        int        padnum,show_all = 0;
	grs_canvas *save_canvas = grd_curcanv;

	wnd++;		//kill warning

	grd_curcanv = Pad_text_canvas;

	padnum = ui_pad_get_current();
	Assert(padnum <= MAX_PAD_ID);

	if (padnum != old_padnum) {
		clear_pad_display();
		old_padnum = padnum;
		show_all = 1;
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
}

