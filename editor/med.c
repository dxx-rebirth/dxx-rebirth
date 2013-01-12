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
 * Editor loop for Inferno
 *
 */

//#define DEMO 1

#define	DIAGNOSTIC_MESSAGE_MAX				90
#define	EDITOR_STATUS_MESSAGE_DURATION	4		//	Shows for 3+..4 seconds

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "inferno.h"
#include "segment.h"
#include "gr.h"
#include "palette.h"
#include "event.h"
#include "window.h"
#include "messagebox.h"
#include "ui.h"
#include "editor.h"
#include "editor/esegment.h"
#include "gamesave.h"
#include "gameseg.h"
#include "key.h"
#include "kconfig.h"
#include "mouse.h"
#include "dxxerror.h"
#include "kfuncs.h"
#include "macro.h"
#ifdef INCLUDE_XLISP
#include "medlisp.h"
#endif
#include "u_mem.h"
#include "physfsx.h"
#include "render.h"
#include "game.h"
#include "gamefont.h"
#include "gamepal.h"
#include "menu.h"
#include "slew.h"
#include "kdefs.h"
#include "func.h"
#include "textures.h"
#include "screens.h"
#include "texmap.h"
#include "object.h"
#include "effects.h"
#include "wall.h"
#include "info.h"
#include "ai.h"
#include "console.h"
#include "texpage.h"		// Textue selection paging stuff
#include "objpage.h"		// Object selection paging stuff

#include "medmisc.h"
#include "meddraw.h"
#include "medsel.h"
#include "medrobot.h"
#include "medwall.h"
#include "eswitch.h"
#include "ehostage.h"
#include "centers.h"

#include "fuelcen.h"
#include "gameseq.h"
#include "newmenu.h"

//#define _MARK_ON 1
//#include <wsample.h>		//should come after inferno.h to get mark setting //Not included here.

#define COMPRESS_INTERVAL	5			// seconds

//char *undo_status[128];

int initializing;

//these are instances of canvases, pointed to by variables below
grs_canvas _canv_editor_game, _canv_editor; //the game on the editor screen, the canvas that the editor writes to

//these are pointers to our canvases
grs_canvas *Canv_editor;			//the editor screen
grs_canvas *Canv_editor_game=&_canv_editor_game; //the game on the editor screen

window *Pad_info;		// Keypad text

grs_font *editor_font=NULL;

//where the editor is looking
vms_vector Ed_view_target=ZERO_VECTOR;

int gamestate_not_restored = 0;

UI_DIALOG * EditorWindow = NULL;

int	Large_view_index = -1;

UI_GADGET_USERBOX * GameViewBox;
UI_GADGET_USERBOX * LargeViewBox;
UI_GADGET_USERBOX * GroupViewBox;

#if ORTHO_VIEWS
UI_GADGET_USERBOX * TopViewBox;
UI_GADGET_USERBOX * FrontViewBox;
UI_GADGET_USERBOX * RightViewBox;
#endif

UI_GADGET_ICON * ViewIcon;
UI_GADGET_ICON * AllIcon;
UI_GADGET_ICON * AxesIcon;
UI_GADGET_ICON * ChaseIcon;
UI_GADGET_ICON * OutlineIcon;
UI_GADGET_ICON * LockIcon;
//-NOLIGHTICON- UI_GADGET_ICON * LightIcon;

UI_EVENT * DemoBuffer = NULL;

//grs_canvas * BigCanvas[2];
//int CurrentBigCanvas = 0;
//int BigCanvasFirstTime = 1;

int	Found_seg_index=0;				// Index in Found_segs corresponding to Cursegp


void print_status_bar( char message[DIAGNOSTIC_MESSAGE_MAX] ) {
	int w,h,aw;

	gr_set_current_canvas( NULL );
	gr_set_curfont(editor_font);
	gr_set_fontcolor( CBLACK, CGREY );
	gr_get_string_size( message, &w, &h, &aw );
	gr_string( 4, 583, message );
	gr_set_fontcolor( CBLACK, CWHITE );
	gr_setcolor( CGREY );
	gr_rect( 4+w, 583, 799, 599 );
}

static char status_line[DIAGNOSTIC_MESSAGE_MAX] = "";

struct tm	Editor_status_last_time;

void editor_status_fmt( const char *format, ... )
{
	va_list ap;

	va_start(ap, format);
	vsprintf(status_line, format, ap);
	va_end(ap);

	Editor_status_last_time = Editor_time_of_day;
}

void editor_status( const char *text)
{
	strcpy(status_line, text);
	Editor_status_last_time = Editor_time_of_day;
}

// 	int  tm_sec;	/* seconds after the minute -- [0,61] */
// 	int  tm_min;	/* minutes after the hour	-- [0,59] */
// 	int  tm_hour;	/* hours after midnight	-- [0,23] */
// 	int  tm_mday;	/* day of the month		-- [1,31] */
// 	int  tm_mon;	/* months since January	-- [0,11] */
// 	int  tm_year;	/* years since 1900				*/
// 	int  tm_wday;	/* days since Sunday		-- [0,6]  */
// 	int  tm_yday;	/* days since January 1	-- [0,365]*/
// 	int  tm_isdst;	/* Daylight Savings Time flag */

void clear_editor_status(void)
{
	int cur_time = Editor_time_of_day.tm_hour * 3600 + Editor_time_of_day.tm_min*60 + Editor_time_of_day.tm_sec;
	int erase_time = Editor_status_last_time.tm_hour * 3600 + Editor_status_last_time.tm_min*60 + Editor_status_last_time.tm_sec + EDITOR_STATUS_MESSAGE_DURATION;

	if (cur_time > erase_time) {
		int	i;

		for (i=0; i<DIAGNOSTIC_MESSAGE_MAX-1; i++)
			status_line[i] = ' ';

		status_line[i] = 0;
		Editor_status_last_time.tm_hour = 99;
	}
}

int DropIntoDebugger()
{
	Int3();
	return 1;
}


#ifdef INCLUDE_XLISP
int CallLisp()
{
	medlisp_go();
	return 1;
}
#endif


int ExitEditor()
{
	if (SafetyCheck())  {
		ModeFlag = 1;
	}
	return 1;
}

int	GotoGameScreen()
{
	stop_time();

//@@	init_player_stats();
//@@
//@@	Player_init.pos = Player->pos;
//@@	Player_init.orient = Player->orient;
//@@	Player_init.segnum = Player->segnum;	
	
// -- must always save gamesave.sav because the restore-objects code relies on it
// -- that code could be made smarter and use the original file, if appropriate.
//	if (mine_changed) 
	if (gamestate_not_restored == 0) {
		gamestate_not_restored = 1;
		save_level("GAMESAVE.LVL");
		editor_status("Gamestate saved.\n");
	}

	ai_reset_all_paths();

	start_time();

	ModeFlag = 3;
	return 1;
}

int GotoMainMenu()
{
	ModeFlag = 2;
	return 1;
}


#if 0
void ReadLispMacro( FILE * file, char * buffer )
{
//	char c;
//	int size=0;
//	int pcount = 0;
//	char text[100];
//	int i=0;
	
	fscanf( file, " { %s } ", buffer );

/*
	while (1)
	{
		c = text[i++];
		if (pcount > 0 )
			buffer[size++] = c;
		if ( c == '(' ) pcount++;
		if ( c == ')' ) break;
	}
	buffer[size++] = 0;
*/

	return;
}
#endif

static int (*KeyFunction[2048])();

void medkey_init()
{
	PHYSFS_file * keyfile;
	char keypress[100];
	char line_buffer[200];
	int key;
	int i;	//, size;
	int np;
	char * LispCommand;

	MALLOC( LispCommand, char, DIAGNOSTIC_MESSAGE_MAX );

	for (i=0; i<2048; i++ )
		KeyFunction[i] = NULL;

	keyfile = PHYSFSX_openReadBuffered( "GLOBAL.KEY" );
	if (keyfile)
	{
		while (PHYSFSX_fgets(line_buffer, 200, keyfile))
		{
			sscanf(line_buffer, " %s %s ", keypress, LispCommand);
			//ReadLispMacro( keyfile, LispCommand );

			if ( (key=DecodeKeyText( keypress ))!= -1 )
			{
				Assert( key < 2048);
				KeyFunction[key] = func_get( LispCommand, &np );
			} else {
				Error( "Bad key %s in GLOBAL.KEY!", keypress );
			}
		}
		PHYSFS_close(keyfile);
	}
	d_free( LispCommand );
}

static int padnum=0;
//@@short camera_objnum;			//a camera for viewing. Who knows, might become handy

void init_editor_screen();
void gamestate_restore_check();

void init_editor()
{
	void med_show_warning(char *s);

	// first, make sure we can find the files we need
	PHYSFSX_addRelToSearchPath("editor/data", 1);	// look in source directory first (for work in progress)
	PHYSFSX_addRelToSearchPath("editor", 1);		// then in editor directory
	PHYSFSX_addRelToSearchPath("editor.zip", 1);	// then in a zip file
	PHYSFSX_addRelToSearchPath("editor.dxa", 1);	// or addon pack

	ui_init();

	init_med_functions();	// Must be called before medlisp_init

	ui_pad_read( 0, "segmove.pad" );
	ui_pad_read( 1, "segsize.pad" );
	ui_pad_read( 2, "curve.pad" );
	ui_pad_read( 3, "texture.pad" );
	ui_pad_read( 4, "object.pad" );
	ui_pad_read( 5, "objmov.pad" );
	ui_pad_read( 6, "group.pad" );
	ui_pad_read( 7, "lighting.pad" );
	ui_pad_read( 8, "test.pad" );

	medkey_init();

	game_flush_inputs();
	
	editor_font = gr_init_font( "pc8x16.fnt" );
	
	menubar_init( "MED.MNU" );

	Draw_all_segments = 1;						// Say draw all segments, not just connected ones
	
	if (!Cursegp)
		Cursegp = &Segments[0];

	init_autosave();
  
//	atexit(close_editor);

	Clear_window = 1;	//	do full window clear.
	
	InitCurve();
	
	restore_effect_bitmap_icons();
	
	if (!set_screen_mode(SCREEN_EDITOR))	{
		set_screen_mode(SCREEN_MENU);
		show_menus();			//force back into menu
		return;
	}
	
	load_palette(Current_level_palette,1,0);
	
	//Editor renders into full (320x200) game screen 
	
	game_init_render_buffers(320, 200);
	gr_init_sub_canvas( &_canv_editor, &grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT );
	Canv_editor = &_canv_editor;
	gr_set_current_canvas( Canv_editor );
	init_editor_screen(); // load the main editor dialog
	gr_set_current_canvas( NULL );
	gr_set_curfont(editor_font);
	
	set_warn_func(med_show_warning);
	
	//	_MARK_("start of editor");//Nuked to compile -KRB
	
	//@@	//create a camera for viewing in the editor. copy position from ConsoleObject
	//@@	camera_objnum = obj_create(OBJ_CAMERA,0,ConsoleObject->segnum,&ConsoleObject->pos,&ConsoleObject->orient,0);
	//@@	Viewer = &Objects[camera_objnum];
	//@@	slew_init(Viewer);		//camera is slewing
	
	Viewer = ConsoleObject;
	slew_init(ConsoleObject);
	init_player_object();
	
	Update_flags = UF_ALL;
	
	//set the wire-frame window to be the current view
	current_view = &LargeView;
	
	if (faded_in==0)
	{
		faded_in = 1;
		//gr_pal_fade_in( grd_curscreen->pal );
	}
	
	gr_set_current_canvas( GameViewBox->canvas );
	gr_set_curfont(editor_font);
	//gr_setcolor( CBLACK );
	//gr_deaccent_canvas();
	//gr_grey_canvas();
	
	gr_set_curfont(editor_font);
	FNTScaleX = FNTScaleY = 1;		// No font scaling!
	ui_pad_goto(padnum);
	
	ModeFlag = 0;
	
	gamestate_restore_check();
}

int ShowAbout()
{
	ui_messagebox( -2, -2, 1, 	"INFERNO Mine Editor\n\n"		\
									"Copyright (c) 1993  Parallax Software Corp.",
									"OK");
	return 0;
}

void move_player_2_segment(segment *seg,int side);

int SetPlayerFromCurseg()
{
	move_player_2_segment(Cursegp,Curside);
	Update_flags |= UF_ED_STATE_CHANGED | UF_GAME_VIEW_CHANGED;
	return 1;
}

int fuelcen_create_from_curseg()
{
	Curseg2p->special = SEGMENT_IS_FUELCEN;
	fuelcen_activate(Cursegp, Curseg2p->special);
	return 1;
}

int repaircen_create_from_curseg()
{
	Int3();	//	-- no longer supported!
//	Curseg2p->special = SEGMENT_IS_REPAIRCEN;
//	fuelcen_activate(Cursegp, Curseg2p->special);
	return 1;
}

int controlcen_create_from_curseg()
{
	Curseg2p->special = SEGMENT_IS_CONTROLCEN;
	fuelcen_activate(Cursegp, Curseg2p->special);
	return 1;
}

int robotmaker_create_from_curseg()
{
	Curseg2p->special = SEGMENT_IS_ROBOTMAKER;
	fuelcen_activate(Cursegp, Curseg2p->special);
	return 1;
}

int fuelcen_reset_all()	{
	fuelcen_reset();
	return 1;
}

int fuelcen_delete_from_curseg() {
	fuelcen_delete( Cursegp );
	return 1;
}


//@@//this routine places the viewer in the center of the side opposite to curside,
//@@//with the view toward the center of curside
//@@int SetPlayerFromCursegMinusOne()
//@@{
//@@	vms_vector vp;
//@@
//@@//	int newseg,newside;
//@@//	get_previous_segment(SEG_PTR_2_NUM(Cursegp),Curside,&newseg,&newside);
//@@//	move_player_2_segment(&Segments[newseg],newside);
//@@
//@@	med_compute_center_point_on_side(&Player->obj_position,Cursegp,Side_opposite[Curside]);
//@@	med_compute_center_point_on_side(&vp,Cursegp,Curside);
//@@	vm_vec_sub2(&vp,&Player->position);
//@@	vm_vector_2_matrix(&Player->orient,&vp,NULL,NULL);
//@@
//@@	Player->seg = SEG_PTR_2_NUM(Cursegp);
//@@
//@@	Update_flags |= UF_GAME_VIEW_CHANGED;
//@@	return 1;
//@@}

//this constant determines how much of the window will be occupied by the
//viewed side when SetPlayerFromCursegMinusOne() is called.  It actually
//determine how from from the center of the window the farthest point will be
#define SIDE_VIEW_FRAC (f1_0*8/10)	//80%


void move_player_2_segment_and_rotate(segment *seg,int side)
{
	vms_vector vp;
	vms_vector	upvec;
        static int edgenum=0;

	compute_segment_center(&ConsoleObject->pos,seg);
	compute_center_point_on_side(&vp,seg,side);
	vm_vec_sub2(&vp,&ConsoleObject->pos);

	vm_vec_sub(&upvec, &Vertices[Cursegp->verts[Side_to_verts[Curside][edgenum%4]]], &Vertices[Cursegp->verts[Side_to_verts[Curside][(edgenum+3)%4]]]);
	edgenum++;

	vm_vector_2_matrix(&ConsoleObject->orient,&vp,&upvec,NULL);
//	vm_vector_2_matrix(&ConsoleObject->orient,&vp,NULL,NULL);

	obj_relink( ConsoleObject-Objects, SEG_PTR_2_NUM(seg) );
	
}

int SetPlayerFromCursegAndRotate()
{
	move_player_2_segment_and_rotate(Cursegp,Curside);
	Update_flags |= UF_ED_STATE_CHANGED | UF_GAME_VIEW_CHANGED;
	return 1;
}


//sets the player facing curseg/curside, normal to face0 of curside, and
//far enough away to see all of curside
int SetPlayerFromCursegMinusOne()
{
	vms_vector view_vec,view_vec2,side_center;
	vms_vector corner_v[4];
	vms_vector	upvec;
	g3s_point corner_p[4];
	int i;
	fix max,view_dist=f1_0*10;
        static int edgenum=0;
	int newseg;

	view_vec = Cursegp->sides[Curside].normals[0];
	vm_vec_negate(&view_vec);

	compute_center_point_on_side(&side_center,Cursegp,Curside);
	vm_vec_copy_scale(&view_vec2,&view_vec,view_dist);
	vm_vec_sub(&ConsoleObject->pos,&side_center,&view_vec2);

	vm_vec_sub(&upvec, &Vertices[Cursegp->verts[Side_to_verts[Curside][edgenum%4]]], &Vertices[Cursegp->verts[Side_to_verts[Curside][(edgenum+3)%4]]]);
	edgenum++;

	vm_vector_2_matrix(&ConsoleObject->orient,&view_vec,&upvec,NULL);

	gr_set_current_canvas(Canv_editor_game);
	g3_start_frame();
	g3_set_view_matrix(&ConsoleObject->pos,&ConsoleObject->orient,Render_zoom);

	for (i=max=0;i<4;i++) {
		corner_v[i] = Vertices[Cursegp->verts[Side_to_verts[Curside][i]]];
		g3_rotate_point(&corner_p[i],&corner_v[i]);
		if (labs(corner_p[i].p3_x) > max) max = labs(corner_p[i].p3_x);
		if (labs(corner_p[i].p3_y) > max) max = labs(corner_p[i].p3_y);
	}

	view_dist = fixmul(view_dist,fixdiv(fixdiv(max,SIDE_VIEW_FRAC),corner_p[0].p3_z));
	vm_vec_copy_scale(&view_vec2,&view_vec,view_dist);
	vm_vec_sub(&ConsoleObject->pos,&side_center,&view_vec2);

	//obj_relink(ConsoleObject-Objects, SEG_PTR_2_NUM(Cursegp) );
	//update_object_seg(ConsoleObject);		//might have backed right out of curseg

	newseg = find_point_seg(&ConsoleObject->pos,SEG_PTR_2_NUM(Cursegp) );
	if (newseg != -1)
		obj_relink(ConsoleObject-Objects,newseg);

	Update_flags |= UF_ED_STATE_CHANGED | UF_GAME_VIEW_CHANGED;
	return 1;
}

int ToggleLighting(void)
{
	Lighting_on++;
	if (Lighting_on >= 2)
		Lighting_on = 0;

	Update_flags |= UF_GAME_VIEW_CHANGED;

	switch (Lighting_on) {
		case 0:
			diagnostic_message("Lighting off.");
			break;
		case 1:
			diagnostic_message("Static lighting.");
			break;
		case 2:
			diagnostic_message("Ship lighting.");
			break;
		case 3:
			diagnostic_message("Ship and static lighting.");
			break;
	}

	return Lighting_on;
}

void find_concave_segs();

int FindConcaveSegs()
{
	find_concave_segs();

	Update_flags |= UF_ED_STATE_CHANGED;		//list may have changed

	return 1;
}

int DosShell()
{
	int ok, w, h;
	grs_bitmap * save_bitmap;

	ok = 1;

	// Save the current graphics state.

	w = grd_curscreen->sc_canvas.cv_bitmap.bm_w;
	h = grd_curscreen->sc_canvas.cv_bitmap.bm_h;

	save_bitmap = gr_create_bitmap( w, h );
	gr_bm_ubitblt(w, h, 0, 0, 0, 0, &(grd_curscreen->sc_canvas.cv_bitmap), save_bitmap );

	// gr_set_mode( SM_ORIGINAL );

	fflush(stdout);

	key_close();
#ifdef __MSDOS__
	ok = spawnl(P_WAIT,getenv("COMSPEC"), NULL );
#endif
	key_init();

	gr_set_mode(grd_curscreen->sc_mode);
	gr_bm_ubitblt(w, h, 0, 0, 0, 0, save_bitmap, &(grd_curscreen->sc_canvas.cv_bitmap));
	gr_free_bitmap( save_bitmap );
	//gr_pal_setblock( 0, 256, grd_curscreen->pal );
	//gr_use_palette_table();

	return ok;

}

int ToggleOutlineMode()
{
#ifndef NDEBUG
	int mode;

	mode=toggle_outline_mode();

        if (mode)
         {
		//if (keypress != KEY_O)
			diagnostic_message("[Ctrl-O] Outline Mode ON.");
		//else
		//	diagnostic_message("Outline Mode ON.");
         }
	else
         {
		//if (keypress != KEY_O)
			diagnostic_message("[Ctrl-O] Outline Mode OFF.");
		//else
		//	diagnostic_message("Outline Mode OFF.");
         }

	Update_flags |= UF_GAME_VIEW_CHANGED;
	return mode;
#else
	return 1;
#endif
}

//@@int do_reset_orient()
//@@{
//@@	slew_reset_orient(SlewObj);
//@@
//@@	Update_flags |= UF_GAME_VIEW_CHANGED;
//@@
//@@	* (ubyte *) 0x417 &= ~0x20;
//@@
//@@	return 1;
//@@}

int GameZoomOut()
{
	Render_zoom = fixmul(Render_zoom,68985);
	Update_flags |= UF_GAME_VIEW_CHANGED;
	return 1;
}

int GameZoomIn()
{
	Render_zoom = fixmul(Render_zoom,62259);
	Update_flags |= UF_GAME_VIEW_CHANGED;
	return 1;
}


int med_keypad_goto_0()	{	ui_pad_goto(0);	return 0;	}
int med_keypad_goto_1()	{	ui_pad_goto(1);	return 0;	}
int med_keypad_goto_2()	{	ui_pad_goto(2);	return 0;	}
int med_keypad_goto_3()	{	ui_pad_goto(3);	return 0;	}
int med_keypad_goto_4()	{	ui_pad_goto(4);	return 0;	}
int med_keypad_goto_5()	{	ui_pad_goto(5);	return 0;	}
int med_keypad_goto_6()	{	ui_pad_goto(6);	return 0;	}
int med_keypad_goto_7()	{	ui_pad_goto(7);	return 0;	}
int med_keypad_goto_8()	{	ui_pad_goto(8);	return 0;	}

#define	PAD_WIDTH	30
#define	PAD_WIDTH1	(PAD_WIDTH + 7)

int editor_screen_open = 0;

int editor_handler(UI_DIALOG *dlg, d_event *event, void *data);

//setup the editors windows, canvases, gadgets, etc.
void init_editor_screen()
{	
//	grs_bitmap * bmp;

	if (editor_screen_open) return;

	grd_curscreen->sc_canvas.cv_font = editor_font;
	
	//create canvas for game on the editor screen
	initializing = 1;
	gr_set_current_canvas(Canv_editor);
	Canv_editor->cv_font = editor_font;
	gr_init_sub_canvas(Canv_editor_game,Canv_editor,GAMEVIEW_X,GAMEVIEW_Y,GAMEVIEW_W,GAMEVIEW_H);
	
	//Editor renders into full (320x200) game screen 

	init_info = 1;

	//do other editor screen setup

	// Since the palette might have changed, find some good colors...
	CBLACK = gr_find_closest_color( 1, 1, 1 );
	CGREY = gr_find_closest_color( 28, 28, 28 );
	CWHITE = gr_find_closest_color( 38, 38, 38 );
	CBRIGHT = gr_find_closest_color( 60, 60, 60 );
	CRED = gr_find_closest_color( 63, 0, 0 );

	gr_set_curfont(editor_font);
	gr_set_fontcolor( CBLACK, CWHITE );

	EditorWindow = ui_create_dialog( 0 , 0, ED_SCREEN_W, ED_SCREEN_H, DF_FILLED, editor_handler, NULL );

	LargeViewBox	= ui_add_gadget_userbox( EditorWindow,LVIEW_X,LVIEW_Y,LVIEW_W,LVIEW_H);
#if ORTHO_VIEWS
	TopViewBox		= ui_add_gadget_userbox( EditorWindow,TVIEW_X,TVIEW_Y,TVIEW_W,TVIEW_H);
 	FrontViewBox	= ui_add_gadget_userbox( EditorWindow,FVIEW_X,FVIEW_Y,FVIEW_W,FVIEW_H);
	RightViewBox	= ui_add_gadget_userbox( EditorWindow,RVIEW_X,RVIEW_Y,RVIEW_W,RVIEW_H);
#endif
	ui_gadget_calc_keys(EditorWindow);	//make tab work for all windows

	GameViewBox	= ui_add_gadget_userbox( EditorWindow, GAMEVIEW_X, GAMEVIEW_Y, GAMEVIEW_W, GAMEVIEW_H );
//	GroupViewBox	= ui_add_gadget_userbox( EditorWindow,GVIEW_X,GVIEW_Y,GVIEW_W,GVIEW_H);

//	GameViewBox->when_tab = GameViewBox->when_btab = (UI_GADGET *) LargeViewBox;
//	LargeViewBox->when_tab = LargeViewBox->when_btab = (UI_GADGET *) GameViewBox;

//	ui_gadget_calc_keys(EditorWindow);	//make tab work for all windows

	ViewIcon	= ui_add_gadget_icon( EditorWindow, "Lock\nview",	455,25+530, 	40, 22,	KEY_V+KEY_CTRLED, ToggleLockViewToCursegp );
	AllIcon	= ui_add_gadget_icon( EditorWindow, "Draw\nall",	500,25+530,  	40, 22,	-1, ToggleDrawAllSegments );
	AxesIcon	= ui_add_gadget_icon( EditorWindow, "Coord\naxes",545,25+530,		40, 22,	KEY_D+KEY_CTRLED, ToggleCoordAxes );
//-NOLIGHTICON-	LightIcon	= ui_add_gadget_icon( EditorWindow, "Light\ning",	590,25+530, 	40, 22,	KEY_L+KEY_SHIFTED,ToggleLighting );
	ChaseIcon	= ui_add_gadget_icon( EditorWindow, "Chase\nmode",635,25+530,		40, 22,	-1,				ToggleChaseMode );
	OutlineIcon = ui_add_gadget_icon( EditorWindow, "Out\nline", 	680,25+530,  	40, 22,	KEY_O+KEY_CTRLED,			ToggleOutlineMode );
	LockIcon	= ui_add_gadget_icon( EditorWindow, "Lock\nstep", 725,25+530, 	40, 22,	KEY_L+KEY_CTRLED,			ToggleLockstep );

	meddraw_init_views(LargeViewBox->canvas);

	//ui_add_gadget_button( EditorWindow, 460, 510, 50, 25, "Quit", ExitEditor );
	//ui_add_gadget_button( EditorWindow, 520, 510, 50, 25, "Lisp", CallLisp );
	//ui_add_gadget_button( EditorWindow, 580, 510, 50, 25, "Mine", MineMenu );
	//ui_add_gadget_button( EditorWindow, 640, 510, 50, 25, "Help", DoHelp );
	//ui_add_gadget_button( EditorWindow, 460, 540, 50, 25, "Macro", MacroMenu );
	//ui_add_gadget_button( EditorWindow, 520, 540, 50, 25, "About", ShowAbout );
	//ui_add_gadget_button( EditorWindow, 640, 540, 50, 25, "Shell", DosShell );

	ui_pad_activate( EditorWindow, PAD_X, PAD_Y );
	Pad_info = info_window_create();
	ui_add_gadget_button( EditorWindow, PAD_X+6, PAD_Y+(30*5)+22, PAD_WIDTH, 20, "<<",  med_keypad_goto_prev );
	ui_add_gadget_button( EditorWindow, PAD_X+PAD_WIDTH1+6, PAD_Y+(30*5)+22, PAD_WIDTH, 20, ">>",  med_keypad_goto_next );

	{	int	i;
		i = 0;	ui_add_gadget_button( EditorWindow, PAD_X+16+(i+2)*PAD_WIDTH1, PAD_Y+(30*5)+22, PAD_WIDTH, 20, "SR",  med_keypad_goto_0 );
		i++;		ui_add_gadget_button( EditorWindow, PAD_X+16+(i+2)*PAD_WIDTH1, PAD_Y+(30*5)+22, PAD_WIDTH, 20, "SS",  med_keypad_goto_1 );
		i++;		ui_add_gadget_button( EditorWindow, PAD_X+16+(i+2)*PAD_WIDTH1, PAD_Y+(30*5)+22, PAD_WIDTH, 20, "CF",  med_keypad_goto_2 );
		i++;		ui_add_gadget_button( EditorWindow, PAD_X+16+(i+2)*PAD_WIDTH1, PAD_Y+(30*5)+22, PAD_WIDTH, 20, "TM",  med_keypad_goto_3 );
		i++;		ui_add_gadget_button( EditorWindow, PAD_X+16+(i+2)*PAD_WIDTH1, PAD_Y+(30*5)+22, PAD_WIDTH, 20, "OP",  med_keypad_goto_4 );
		i++;		ui_add_gadget_button( EditorWindow, PAD_X+16+(i+2)*PAD_WIDTH1, PAD_Y+(30*5)+22, PAD_WIDTH, 20, "OR",  med_keypad_goto_5 );
		i++;		ui_add_gadget_button( EditorWindow, PAD_X+16+(i+2)*PAD_WIDTH1, PAD_Y+(30*5)+22, PAD_WIDTH, 20, "GE",  med_keypad_goto_6 );
		i++;		ui_add_gadget_button( EditorWindow, PAD_X+16+(i+2)*PAD_WIDTH1, PAD_Y+(30*5)+22, PAD_WIDTH, 20, "LI",  med_keypad_goto_7 );
		i++;		ui_add_gadget_button( EditorWindow, PAD_X+16+(i+2)*PAD_WIDTH1, PAD_Y+(30*5)+22, PAD_WIDTH, 20, "TT",  med_keypad_goto_8 );
	}

	gr_set_curfont(editor_font);
	menubar_show();

	// INIT TEXTURE STUFF
	texpage_init( EditorWindow );
	objpage_init( EditorWindow );

	EditorWindow->keyboard_focus_gadget = (UI_GADGET *)LargeViewBox;

//	BigCanvas[0]->cv_font = grd_curscreen->sc_canvas.cv_font; 
//	BigCanvas[1]->cv_font = grd_curscreen->sc_canvas.cv_font; 
//	BigCanvasFirstTime = 1;

	Update_flags = UF_ALL;
	initializing = 0;
	editor_screen_open = 1;
}

//shutdown ui on the editor screen
void close_editor_screen()
{
	if (!editor_screen_open) return;

	editor_screen_open = 0;
	ui_pad_deactivate();
	if (Pad_info)
		window_close(Pad_info);

	//ui_close_dialog(EditorWindow);	// moved into handler, so we can handle the quit request
	//EditorWindow = NULL;

	close_all_windows();

	// CLOSE TEXTURE STUFF
	texpage_close();
	objpage_close();

	menubar_hide();

}

void med_show_warning(char *s)
{
	grs_canvas *save_canv=grd_curcanv;

	//gr_pal_fade_in(grd_curscreen->pal);	//in case palette is blacked

	ui_messagebox(-2,-2,1,s,"OK");

	gr_set_current_canvas(save_canv);

}

// Returns 1 if OK to trash current mine.
int SafetyCheck()
{
	int x;
			
	if (mine_changed) {
		x = nm_messagebox( "Warning!", 2, "Cancel", "OK", "You are about to lose work." );
		if (x<1) {
			return 0;
		}
	}
	return 1;
}

//called at the end of the program
void close_editor() {

	//	_MARK_("end of editor");//Nuked to compile -KRB
	
#ifndef __LINUX__
	set_warn_func(msgbox_warning);
#else
	clear_warn_func(NULL);
#endif
	
	close_editor_screen();
	
	//kill our camera object
	
	Viewer = ConsoleObject;					//reset viewer
	//@@obj_delete(camera_objnum);
	
	padnum = ui_pad_get_current();
	
	close_autosave();

	ui_close();

	gr_close_font(editor_font);

	PHYSFSX_removeRelFromSearchPath("editor/data");
	PHYSFSX_removeRelFromSearchPath("editor");
	PHYSFSX_removeRelFromSearchPath("editor.zip");
	PHYSFSX_removeRelFromSearchPath("editor.dxa");

	switch (ModeFlag)
	{
		case 1:
			if (Game_wind)
				window_close(Game_wind);
			break;

		case 2:
			if (Game_wind)
				window_close(Game_wind);
			set_screen_mode(SCREEN_MENU);		//put up menu screen
			show_menus();
			break;

		case 3:
			set_screen_mode(SCREEN_GAME);		//put up game screen
			Game_mode = GM_EDITOR;
			editor_reset_stuff_on_level();
			N_players = 1;
			break;
	}

	return;
}

//variables for find segments process

// ---------------------------------------------------------------------------------------------------
//	Subtract all elements in Found_segs from selected list.
void subtract_found_segments_from_selected_list(void)
{
	int	s,f;

	for (f=0; f<N_found_segs; f++) {
		int	foundnum = Found_segs[f];

		for (s=0; s<N_selected_segs; s++) {
			if (Selected_segs[s] == foundnum) {
				Selected_segs[s] = Selected_segs[N_selected_segs-1];
				N_selected_segs--;
				break;
			}
		}
	}
}

// ---------------------------------------------------------------------------------------------------
//	Add all elements in Found_segs to selected list.
void add_found_segments_to_selected_list(void) {
	int	s,f;

	for (f=0; f<N_found_segs; f++) {
		int	foundnum = Found_segs[f];

		for (s=0; s<N_selected_segs; s++)
			if (Selected_segs[s] == foundnum)
				break;

		if (s == N_selected_segs)
			Selected_segs[N_selected_segs++] = foundnum;
	}
}

void gamestate_restore_check()
{
	char Message[DIAGNOSTIC_MESSAGE_MAX];
	obj_position Save_position;

	if (gamestate_not_restored) {
		sprintf( Message, "Do you wish to restore game state?\n");
	
		if (ui_messagebox( -2, -2, 2, Message, "Yes", "No" )==1) {

			// Save current position
			Save_position.pos = ConsoleObject->pos;
			Save_position.orient = ConsoleObject->orient;
			Save_position.segnum = ConsoleObject->segnum;

			load_level("GAMESAVE.LVL");

			// Restore current position
			if (Save_position.segnum <= Highest_segment_index) {
				ConsoleObject->pos = Save_position.pos;
				ConsoleObject->orient = Save_position.orient;
				obj_relink(ConsoleObject-Objects,Save_position.segnum);
			}

			gamestate_not_restored = 0;
			Update_flags |= UF_WORLD_CHANGED;	
			}
		else
			gamestate_not_restored = 1;
		}
}

int RestoreGameState() {
	load_level("GAMESAVE.LVL");
	gamestate_not_restored = 0;

	editor_status("Gamestate restored.\n");

	Update_flags |= UF_WORLD_CHANGED;
	return 0;
}

extern void check_wall_validity(void);

// Handler for the main editor dialog
int editor_handler(UI_DIALOG *dlg, d_event *event, void *data)
{
	editor_view *new_cv;
	int keypress = 0;
	int rval = 0;

	if (event->type == EVENT_KEY_COMMAND)
		keypress = event_key_get(event);
	else if (event->type == EVENT_WINDOW_CLOSE)
	{
		close_editor();
		EditorWindow = NULL;
		return 0;
	}
	
	// Update the windows

	if (event->type == EVENT_UI_DIALOG_DRAW)
	{
		gr_set_curfont(editor_font);

		// Draw status box
		gr_set_current_canvas( NULL );
		gr_setcolor( CGREY );
		gr_rect(STATUS_X,STATUS_Y,STATUS_X+STATUS_W-1,STATUS_Y+STATUS_H-1);			//0, 582, 799, 599 );
		
		medlisp_update_screen();
		calc_frame_time();
		texpage_do(event);
		objpage_do(event);
		ui_pad_draw(EditorWindow, PAD_X, PAD_Y);

		print_status_bar(status_line);
		TimedAutosave(mine_filename);	// shows the time, hence here
		set_editor_time_of_day();
		return 1;
	}
	
	if ((selected_gadget == (UI_GADGET *)GameViewBox && !render_3d_in_big_window) ||
		(selected_gadget == (UI_GADGET *)LargeViewBox && render_3d_in_big_window))
		switch (event->type)
		{
			case EVENT_MOUSE_BUTTON_UP:
			case EVENT_MOUSE_BUTTON_DOWN:
				break;
			case EVENT_MOUSE_MOVED:
				if (!keyd_pressed[ KEY_LCTRL ] && !keyd_pressed[ KEY_RCTRL ])
					break;
			case EVENT_JOYSTICK_BUTTON_UP:
			case EVENT_JOYSTICK_BUTTON_DOWN:
			case EVENT_JOYSTICK_MOVED:
			case EVENT_KEY_COMMAND:
			case EVENT_KEY_RELEASE:
			case EVENT_IDLE:
				kconfig_read_controls(event, 1);

				if (slew_frame(0))
				{		//do movement and check keys
					Update_flags |= UF_GAME_VIEW_CHANGED;
					if (Gameview_lockstep)
					{
						Cursegp = &Segments[ConsoleObject->segnum];
						med_create_new_segment_from_cursegp();
						Update_flags |= UF_ED_STATE_CHANGED;
					}

					rval = 1;
				}
				break;
				
			default:
				break;
		}

	//do non-essential stuff in idle event
	if (event->type == EVENT_IDLE)
	{
		check_wall_validity();
		Assert(Num_walls>=0);

		if (Gameview_lockstep) {
			static segment *old_cursegp=NULL;
			static int old_curside=-1;

			if (old_cursegp!=Cursegp || old_curside!=Curside) {
				SetPlayerFromCursegMinusOne();
				old_cursegp = Cursegp;
				old_curside = Curside;
			}
		}

		if ( event_get_idle_seconds() > COMPRESS_INTERVAL ) 
		{
			med_compress_mine();
			event_reset_idle_seconds();
		}

	//	Commented out because it occupies about 25% of time in twirling the mine.
	// Removes some Asserts....
	//		med_check_all_vertices();
		clear_editor_status();		// if enough time elapsed, clear editor status message
	}

	gr_set_current_canvas( GameViewBox->canvas );
	
	// Remove keys used for slew
	switch(keypress)
	{
		case KEY_PAD9:
		case KEY_PAD7:
		case KEY_PADPLUS:
		case KEY_PADMINUS:
		case KEY_PAD8:
		case KEY_PAD2:
		case KEY_LBRACKET:
		case KEY_RBRACKET:
		case KEY_PAD1:
		case KEY_PAD3:
		case KEY_PAD6:
		case KEY_PAD4:
			keypress = 0;
	}
	if ((keypress&0xff)==KEY_LSHIFT) keypress=0;
	if ((keypress&0xff)==KEY_RSHIFT) keypress=0;
	if ((keypress&0xff)==KEY_LCTRL) keypress=0;
	if ((keypress&0xff)==KEY_RCTRL) keypress=0;
//		if ((keypress&0xff)==KEY_LALT) keypress=0;
//		if ((keypress&0xff)==KEY_RALT) keypress=0;

	//=================== DO FUNCTIONS ====================

	if ( KeyFunction[ keypress ] != NULL )
	{
		KeyFunction[keypress]();
		keypress = 0;
		rval = 1;
	}

	switch (keypress)
	{
		case 0:
		case KEY_Z:
		case KEY_G:
		case KEY_LALT:
		case KEY_RALT:
		case KEY_LCTRL:
		case KEY_RCTRL:
		case KEY_LSHIFT:
		case KEY_RSHIFT:
		case KEY_LAPOSTRO:
			break;
		case KEY_SHIFTED + KEY_L:
			ToggleLighting();
			rval = 1;
			break;
		case KEY_F1:
			render_3d_in_big_window = !render_3d_in_big_window;
			Update_flags |= UF_ALL;
			rval = 1;
			break;			
		default:
			if (!rval)
			{
				char kdesc[100];
				GetKeyDescription( kdesc, keypress );
				editor_status_fmt("Error: %s isn't bound to anything.", kdesc  );
			}
	}

	//================================================================

	if (ModeFlag)
	{
		ui_close_dialog(EditorWindow);
		return 0;
	}

//		if (EditorWindow->keyboard_focus_gadget == (UI_GADGET *)GameViewBox) current_view=NULL;
//		if (EditorWindow->keyboard_focus_gadget == (UI_GADGET *)GroupViewBox) current_view=NULL;

	new_cv = current_view;

#if ORTHO_VIEWS
	if (EditorWindow->keyboard_focus_gadget == (UI_GADGET *)LargeViewBox) new_cv=&LargeView;
	if (EditorWindow->keyboard_focus_gadget == (UI_GADGET *)TopViewBox)	new_cv=&TopView;
	if (EditorWindow->keyboard_focus_gadget == (UI_GADGET *)FrontViewBox) new_cv=&FrontView;
	if (EditorWindow->keyboard_focus_gadget == (UI_GADGET *)RightViewBox) new_cv=&RightView;
#endif
	if (new_cv != current_view ) {
		current_view->ev_changed = 1;
		new_cv->ev_changed = 1;
		current_view = new_cv;
	}

	// DO TEXTURE STUFF
	if (texpage_do(event))
		rval = 1;
	
	if (objpage_do(event))
		rval = 1;


	// Process selection of Cursegp using mouse.
	if (GADGET_PRESSED(LargeViewBox) && !render_3d_in_big_window) 
	{
		int	xcrd,ycrd;
		xcrd = LargeViewBox->b1_drag_x1;
		ycrd = LargeViewBox->b1_drag_y1;

		find_segments(xcrd,ycrd,LargeViewBox->canvas,&LargeView,Cursegp,Big_depth);	// Sets globals N_found_segs, Found_segs

		// If shift is down, then add segment to found list
		if (keyd_pressed[ KEY_LSHIFT ] || keyd_pressed[ KEY_RSHIFT ])
			subtract_found_segments_from_selected_list();
		else
			add_found_segments_to_selected_list();

		Found_seg_index = 0;	
	
		if (N_found_segs > 0) {
			sort_seg_list(N_found_segs,Found_segs,&ConsoleObject->pos);
			Cursegp = &Segments[Found_segs[0]];
			med_create_new_segment_from_cursegp();
			if (Lock_view_to_cursegp)
				set_view_target_from_segment(Cursegp);
		}

		Update_flags |= UF_ED_STATE_CHANGED | UF_VIEWPOINT_MOVED;
	}

	if ((event->type == EVENT_UI_USERBOX_DRAGGED) && (ui_event_get_gadget(event) == (UI_GADGET *)GameViewBox))
	{
		int	x, y;
		x = GameViewBox->b1_drag_x2;
		y = GameViewBox->b1_drag_y2;

		gr_set_current_canvas( GameViewBox->canvas );
		gr_setcolor( 15 );
		gr_rect( x-1, y-1, x+1, y+1 );
	}
	
	// Set current segment and side by clicking on a polygon in game window.
	//	If ctrl pressed, also assign current texture map to that side.
	//if (GameViewBox->mouse_onme && (GameViewBox->b1_done_dragging || GameViewBox->b1_clicked)) {
	if ((GADGET_PRESSED(GameViewBox) && !render_3d_in_big_window) ||
		(GADGET_PRESSED(LargeViewBox) && render_3d_in_big_window))
	{
		int	xcrd,ycrd;
		int seg,side,face,poly,tmap;

		if (render_3d_in_big_window) {
			xcrd = LargeViewBox->b1_drag_x1;
			ycrd = LargeViewBox->b1_drag_y1;
		}
		else {
			xcrd = GameViewBox->b1_drag_x1;
			ycrd = GameViewBox->b1_drag_y1;
		}

		//Int3();

		if (find_seg_side_face(xcrd,ycrd,&seg,&side,&face,&poly)) {


			if (seg<0) {							//found an object

				Cur_object_index = -seg-1;
				editor_status_fmt("Object %d selected.",Cur_object_index);

				Update_flags |= UF_ED_STATE_CHANGED;
			}
			else {

				//	See if either shift key is down and, if so, assign texture map
				if (keyd_pressed[KEY_LSHIFT] || keyd_pressed[KEY_RSHIFT]) {
					Cursegp = &Segments[seg];
					Curside = side;
					AssignTexture();
					med_create_new_segment_from_cursegp();
					editor_status("Texture assigned");
				} else if (keyd_pressed[KEY_G])	{
					tmap = Segments[seg].sides[side].tmap_num;
					texpage_grab_current(tmap);
					editor_status( "Texture grabbed." );
				} else if (keyd_pressed[ KEY_LAPOSTRO] ) {
					move_object_to_mouse_click();
				} else {
					Cursegp = &Segments[seg];
					Curside = side;
					med_create_new_segment_from_cursegp();
					editor_status("Curseg and curside selected");
				}
			}

			Update_flags |= UF_ED_STATE_CHANGED;
		}
		else 
			editor_status("Click on non-texture ingored");

	}

	// Allow specification of LargeView using mouse
	if (event->type == EVENT_MOUSE_MOVED && (keyd_pressed[ KEY_LCTRL ] || keyd_pressed[ KEY_RCTRL ]))
	{
		int dx, dy, dz;

		event_mouse_get_delta(event, &dx, &dy, &dz);
		if ((dx != 0) && (dy != 0))
		{
			vms_matrix	MouseRotMat,tempm;
			
			GetMouseRotation( dx, dy, &MouseRotMat );
			vm_matrix_x_matrix(&tempm,&LargeView.ev_matrix,&MouseRotMat);
			LargeView.ev_matrix = tempm;
			LargeView.ev_changed = 1;
			Large_view_index = -1;			// say not one of the orthogonal views
			rval = 1;
		}
	}

	if (event->type == EVENT_MOUSE_MOVED)
	{
		int dx, dy, dz;

		event_mouse_get_delta(event, &dx, &dy, &dz);
		if (dz != 0)
		{
			current_view->ev_dist += dz*10000;
			current_view->ev_changed = 1;
		}
	}
	
	return rval;
}

#ifndef NDEBUG
int MarkStart(void)
{
	char mystr[30];
	sprintf(mystr,"mark %i start",Mark_count);
//	_MARK_(mystr);//Nuked to compile -KRB

	return 1;
}

int MarkEnd(void)
{
	char mystr[30];
	sprintf(mystr,"mark %i end",Mark_count);
	Mark_count++;
//	_MARK_(mystr);//Nuked to compile -KRB

	return 1;
}
#endif
