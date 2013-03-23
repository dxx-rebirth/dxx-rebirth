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
 * object selection stuff.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include "inferno.h"
#include "screens.h"			// For GAME_SCREEN?????
#include "editor.h"			// For TMAP_CURBOX??????
#include "gr.h"				// For canves, font stuff
#include "ui.h"				// For UI_GADGET stuff
#include "object.h"			// For robot_bms
#include "dxxerror.h"
#include "objpage.h"
#include "bm.h"
#include "player.h"
#include "piggy.h"
#include "cntrlcen.h"

#define OBJS_PER_PAGE 8

static UI_GADGET_USERBOX * ObjBox[OBJS_PER_PAGE];
static UI_GADGET_USERBOX * ObjCurrent;

static int ObjectPage = 0;

#include "vecmat.h"
#include "3d.h"
#include "polyobj.h"
#include "texmap.h"

#include "hostage.h"
#include "powerup.h"

vms_angvec objpage_view_orient;
fix objpage_view_dist;

//this is bad to have the extern, but this snapshot stuff is special
extern int polyobj_lighting;


//canvas set
void draw_object_picture(int id, vms_angvec *orient_angles, int type)
{

	if (id >= Num_object_subtypes)
		return;

	switch (type) {

		case OBJ_HOSTAGE:
			PIGGY_PAGE_IN(Vclip[Hostage_vclip_num[id]].frames[0]);
			gr_bitmap(0,0,&GameBitmaps[Vclip[Hostage_vclip_num[id]].frames[0].index]);
			break;

		case OBJ_POWERUP:
			if ( Powerup_info[id].vclip_num > -1 )	{
				PIGGY_PAGE_IN(Vclip[Powerup_info[id].vclip_num].frames[0]);
				gr_bitmap(0,0,&GameBitmaps[Vclip[Powerup_info[id].vclip_num].frames[0].index]);
			}
			break;

		case OBJ_PLAYER:
			draw_model_picture(Player_ship->model_num,orient_angles);		// Draw a poly model below
			break;

		case OBJ_ROBOT:
			draw_model_picture(Robot_info[id].model_num,orient_angles);	// Draw a poly model below
			break;

		case OBJ_CNTRLCEN:
			draw_model_picture(get_reactor_model_number(id), orient_angles);
			break;
		case OBJ_CLUTTER:
			draw_model_picture(Reactors[id].model_num, orient_angles);
			break;
		default:
			//Int3();	// Invalid type!!!
			return;
	}

}

void redraw_current_object()
{
	grs_canvas * cc;

	cc = grd_curcanv;
	gr_set_current_canvas(ObjCurrent->canvas);
	draw_object_picture(Cur_object_id, &objpage_view_orient, Cur_object_type);
	gr_set_current_canvas(cc);
}

void gr_label_box( int i)
{

	gr_clear_canvas(BM_XRGB(0,0,0));
	draw_object_picture(i, &objpage_view_orient, Cur_object_type);

//	char s[20];
//	sprintf( s, " %d ", i );
//	gr_clear_canvas( BM_XRGB(0,15,0) );
//	gr_set_fontcolor( CWHITE, BM_XRGB(0,15,0) );
//	ui_string_centered(  grd_curcanv->cv_bitmap.bm_w/2, grd_curcanv->cv_bitmap.bm_h/2, Description[i] );
}

int objpage_goto_first()
{
	int i;

	ObjectPage=0;
	for (i=0;  i<OBJS_PER_PAGE; i++ ) {
		gr_set_current_canvas(ObjBox[i]->canvas);
		if (i+ObjectPage*OBJS_PER_PAGE < Num_object_subtypes ) {
			//gr_ubitmap(0,0, robot_bms[robot_bm_nums[ i+ObjectPage*OBJS_PER_PAGE ] ] );
			gr_label_box(i+ObjectPage*OBJS_PER_PAGE );
		} else
			gr_clear_canvas( CGREY );
	}

	return 1;
}

int objpage_goto_last()
{
	int i;

	ObjectPage=(Num_object_subtypes)/OBJS_PER_PAGE;
	for (i=0;  i<OBJS_PER_PAGE; i++ )
	{
		gr_set_current_canvas(ObjBox[i]->canvas);
		if (i+ObjectPage*OBJS_PER_PAGE < Num_object_subtypes )
		{
			//gr_ubitmap(0,0, robot_bms[robot_bm_nums[ i+ObjectPage*OBJS_PER_PAGE ] ] );
			gr_label_box(i+ObjectPage*OBJS_PER_PAGE );
		} else {
			gr_clear_canvas( CGREY );
		}
	}
	return 1;
}

static int objpage_goto_prev()
{
	int i;
	if (ObjectPage > 0) {
		ObjectPage--;
		for (i=0;  i<OBJS_PER_PAGE; i++ )
		{
			gr_set_current_canvas(ObjBox[i]->canvas);
			if (i+ObjectPage*OBJS_PER_PAGE < Num_object_subtypes)
			{
				//gr_ubitmap(0,0, robot_bms[robot_bm_nums[ i+ObjectPage*OBJS_PER_PAGE ] ] );
				gr_label_box(i+ObjectPage*OBJS_PER_PAGE );
			} else {
				gr_clear_canvas( CGREY );
			}
		}
	}
	return 1;
}

static int objpage_goto_next()
{
	int i;
	if ((ObjectPage+1)*OBJS_PER_PAGE < Num_object_subtypes) {
		ObjectPage++;
		for (i=0;  i<OBJS_PER_PAGE; i++ )
		{
			gr_set_current_canvas(ObjBox[i]->canvas);
			if (i+ObjectPage*OBJS_PER_PAGE < Num_object_subtypes)
			{
				//gr_ubitmap(0,0, robot_bms[robot_bm_nums[ i+ObjectPage*OBJS_PER_PAGE ] ] );
				gr_label_box(i+ObjectPage*OBJS_PER_PAGE );
			} else {
				gr_clear_canvas( CGREY );
			}
		}
	}
	return 1;
}

int objpage_grab_current(int n)
{
	int i;

	if ((n < 0) || (n >= Num_object_subtypes)) return 0;
	
	ObjectPage = n / OBJS_PER_PAGE;
	
	if (ObjectPage*OBJS_PER_PAGE < Num_object_subtypes) {
		for (i=0;  i<OBJS_PER_PAGE; i++ )
		{
			gr_set_current_canvas(ObjBox[i]->canvas);
			if (i + ObjectPage*OBJS_PER_PAGE < Num_object_subtypes)
			{
				//gr_ubitmap(0,0, robot_bms[robot_bm_nums[ i+ObjectPage*OBJS_PER_PAGE ] ] );
				gr_label_box(i+ObjectPage*OBJS_PER_PAGE );
			} else {
				gr_clear_canvas( CGREY );
			}
		}
	}

	Cur_object_id = n;
	gr_set_current_canvas(ObjCurrent->canvas);
	//gr_ubitmap(0,0, robot_bms[robot_bm_nums[ Cur_robot_type ] ] );
	gr_label_box(Cur_object_id);

	//objpage_display_name( Texture[Cur_robot_type]->filename );
	
	return 1;
}

int objpage_goto_next_object()
{
	// there should be a pop-up menu for this
	switch (Cur_object_type)
	{
		case OBJ_ROBOT:
			Cur_object_type = OBJ_HOSTAGE;
			Num_object_subtypes = 1;
			break;

		case OBJ_HOSTAGE:
			Cur_object_type = OBJ_PLAYER;
			Num_object_subtypes = 1;	// can have anarchy/coop, but this is handled automatically
			break;

		case OBJ_PLAYER:
			Cur_object_type = OBJ_POWERUP;
			Num_object_subtypes = N_powerup_types;
			break;

		case OBJ_POWERUP:
			Cur_object_type = OBJ_CNTRLCEN;
			Num_object_subtypes = get_num_reactor_models();
			break;

		case OBJ_CNTRLCEN:
		default:
			Cur_object_type = OBJ_ROBOT;
			Num_object_subtypes = N_robot_types;
			break;
	}

	objpage_grab_current(0);

	return 1;

}

#define OBJBOX_X	(TMAPBOX_X)	//location of first one
#define OBJBOX_Y	(OBJCURBOX_Y - 24 )
#define OBJBOX_W	64
#define OBJBOX_H	64

#define DELTA_ANG 0x800

int objpage_increase_pitch()
{
	objpage_view_orient.p += DELTA_ANG;
	redraw_current_object();
	return 1;
}

int objpage_decrease_pitch()
{
	objpage_view_orient.p -= DELTA_ANG;
	redraw_current_object();
	return 1;
}

int objpage_increase_heading()
{
	objpage_view_orient.h += DELTA_ANG;
	redraw_current_object();
	return 1;
}

int objpage_decrease_heading()
{
	objpage_view_orient.h -= DELTA_ANG;
	redraw_current_object();
	return 1;
}

int objpage_increase_bank()
{
	objpage_view_orient.b += DELTA_ANG;
	redraw_current_object();
	return 1;
}

int objpage_decrease_bank()
{
	objpage_view_orient.b -= DELTA_ANG;
	redraw_current_object();
	return 1;
}

int objpage_increase_z()
{
	objpage_view_dist -= 0x8000;
	redraw_current_object();
	return 1;
}

int objpage_decrease_z()
{
	objpage_view_dist += 0x8000;
	redraw_current_object();
	return 1;
}

int objpage_reset_orient()
{
	objpage_view_orient.p = 0;
	objpage_view_orient.b = 0;
	objpage_view_orient.h = -0x8000;
	//objpage_view_dist = DEFAULT_VIEW_DIST;
	redraw_current_object();
	return 1;
}


// INIT TEXTURE STUFF

void objpage_init( UI_DIALOG *dlg )
{
	int i;

	//Num_total_object_types = N_polygon_models + N_hostage_types + N_powerup_types;
	//Assert (N_polygon_models < MAX_POLYGON_MODELS);
	//Assert (Num_total_object_types < MAX_OBJTYPE );
	//Assert (N_hostage_types < MAX_HOSTAGE_TYPES ); 
	//Assert (N_powerup_types < MAX_POWERUP_TYPES );
	// Assert (N_robot_types < MAX_ROBOTS);

	ui_add_gadget_button( dlg, OBJCURBOX_X + 00, OBJCURBOX_Y - 27, 30, 20, "<<", objpage_goto_prev );
	ui_add_gadget_button( dlg, OBJCURBOX_X + 32, OBJCURBOX_Y - 27, 30, 20, ">>", objpage_goto_next );

	ui_add_gadget_button( dlg, OBJCURBOX_X + 00, OBJCURBOX_Y - 54, 30, 20, "B", objpage_goto_first );
	ui_add_gadget_button( dlg, OBJCURBOX_X + 32, OBJCURBOX_Y - 54, 30, 20, "E", objpage_goto_last );

	ui_add_gadget_button( dlg, OBJCURBOX_X + 25, OBJCURBOX_Y + 62, 22, 13, "P-", objpage_decrease_pitch );
	ui_add_gadget_button( dlg, OBJCURBOX_X + 25, OBJCURBOX_Y + 90, 22, 13, "P+", objpage_increase_pitch );
	ui_add_gadget_button( dlg, OBJCURBOX_X + 00, OBJCURBOX_Y + 90, 22, 13, "B-", objpage_decrease_bank );
	ui_add_gadget_button( dlg, OBJCURBOX_X + 50, OBJCURBOX_Y + 90, 22, 13, "B+", objpage_increase_bank );
	ui_add_gadget_button( dlg, OBJCURBOX_X + 00, OBJCURBOX_Y + 76, 22, 13, "H-", objpage_decrease_heading );
	ui_add_gadget_button( dlg, OBJCURBOX_X + 50, OBJCURBOX_Y + 76, 22, 13, "H+", objpage_increase_heading );
	ui_add_gadget_button( dlg, OBJCURBOX_X + 00, OBJCURBOX_Y + 62, 22, 13, "Z+", objpage_increase_z );
	ui_add_gadget_button( dlg, OBJCURBOX_X + 50, OBJCURBOX_Y + 62, 22, 13, "Z-", objpage_decrease_z );
	ui_add_gadget_button( dlg, OBJCURBOX_X + 25, OBJCURBOX_Y + 76, 22, 13, "R", objpage_reset_orient );

	for (i=0;i<OBJS_PER_PAGE;i++)
		ObjBox[i] = ui_add_gadget_userbox( dlg, OBJBOX_X + (i/2)*(2+OBJBOX_W), OBJBOX_Y + (i%2)*(2+OBJBOX_H), OBJBOX_W, OBJBOX_H);

	ObjCurrent = ui_add_gadget_userbox( dlg, OBJCURBOX_X, OBJCURBOX_Y-5, 64, 64 );

	objpage_reset_orient();

}

void objpage_close()
{
	//gr_free_sub_canvas(ObjnameCanvas);
}


// DO TEXTURE STUFF

int objpage_do(d_event *event)
{
	int i;
	
	if (event->type == EVENT_UI_DIALOG_DRAW)
	{
		for (i=0;  i<OBJS_PER_PAGE; i++ )
		{
			gr_set_current_canvas(ObjBox[i]->canvas);
			if (i+ObjectPage*OBJS_PER_PAGE < Num_object_subtypes)
			{
				//gr_ubitmap(0,0, robot_bms[robot_bm_nums[ i+ObjectPage*OBJS_PER_PAGE ] ] );
				gr_label_box(i+ObjectPage*OBJS_PER_PAGE );
			} else {
				gr_clear_canvas( CGREY );
			}
		}
		
		// Don't reset robot_type when we return to editor.
		//	Cur_robot_type = ObjectPage*OBJS_PER_PAGE;
		gr_set_current_canvas(ObjCurrent->canvas);
		//gr_ubitmap(0,0, robot_bms[robot_bm_nums[ Cur_robot_type ] ] );
		gr_label_box(Cur_object_id);
		
		//ObjnameCanvas = gr_create_sub_canvas(&grd_curscreen->sc_canvas, OBJCURBOX_X , OBJCURBOX_Y + OBJBOX_H + 10, 100, 20);
		//gr_set_current_canvas( ObjnameCanvas );
		//gr_set_curfont( ui_small_font ); 
		//gr_set_fontcolor( CBLACK, CWHITE );
		//objpage_display_name( Texture[Cur_robot_type]->filename );
		
		return 1;
	}

	for (i=0; i<OBJS_PER_PAGE; i++ )
	{
		if (GADGET_PRESSED(ObjBox[i]) && (i+ObjectPage*OBJS_PER_PAGE < Num_object_subtypes))
		{
			Cur_object_id = i+ObjectPage*OBJS_PER_PAGE;
			gr_set_current_canvas(ObjCurrent->canvas);
			//gr_ubitmap(0,0, robot_bms[robot_bm_nums[ Cur_robot_type ] ] );
			gr_label_box(Cur_object_id);
			//objpage_display_name( Texture[Cur_robot_type]->filename );
			return 1;
		}
	}
	
	return 0;
}
