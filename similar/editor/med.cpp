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
#include "args.h"
#include "segment.h"
#include "gr.h"
#include "palette.h"
#include "physfsx.h"
#include "event.h"
#include "window.h"
#include "game.h"
#include "messagebox.h"
#include "ui.h"
#include "editor.h"
#include "editor/esegment.h"
#include "state.h"
#include "gamesave.h"
#include "gameseg.h"
#include "key.h"
#include "kconfig.h"
#include "mouse.h"
#include "dxxerror.h"
#ifdef INCLUDE_XLISP
#include "medlisp.h"
#endif
#include "u_mem.h"
#include "render.h"
#include "game.h"
#include "gamefont.h"
#include "menu.h"
#include "slew.h"
#include "player.h"
#include "kdefs.h"
#include "func.h"
#include "textures.h"
#include "text.h"
#include "screens.h"
#include "texmap.h"
#include "object.h"
#include "effects.h"
#include "info.h"
#include "console.h"
#include "texpage.h"		// Textue selection paging stuff
#include "objpage.h"		// Object selection paging stuff
#include "d_enumerate.h"

#include "medmisc.h"
#include "meddraw.h"
#include "medsel.h"
#include "medwall.h"

#include "fuelcen.h"
#include "gameseq.h"
#include "mission.h"
#include "newmenu.h"

#if defined(DXX_BUILD_DESCENT_II)
#include "gamepal.h"
#endif

#include "compiler-range_for.h"
#include "d_levelstate.h"
#include "d_zip.h"

//#define _MARK_ON 1
//#include <wsample.h>		//should come after inferno.h to get mark setting //Not included here.

#define COMPRESS_INTERVAL	5			// seconds

static void med_show_warning(std::span<const char> s);

//char *undo_status[128];
int initializing;

//these are instances of canvases, pointed to by variables below
grs_subcanvas _canv_editor_game; // the game on the editor screen
static grs_subcanvas _canv_editor; // the canvas that the editor writes to

//these are pointers to our canvases
grs_canvas *Canv_editor;			//the editor screen
grs_subcanvas *const Canv_editor_game = &_canv_editor_game; //the game on the editor screen

window *Pad_info;		// Keypad text

grs_font_ptr editor_font;

//where the editor is looking
vms_vector Ed_view_target;

editor_gamestate gamestate = editor_gamestate::none;

editor_dialog *EditorWindow;

int	Large_view_index = -1;

std::unique_ptr<UI_GADGET_USERBOX> GameViewBox, LargeViewBox, GroupViewBox;

static std::unique_ptr<UI_GADGET_ICON>
	ViewIcon,
	AllIcon,
	AxesIcon,
	ChaseIcon,
	OutlineIcon,
	LockIcon;

//grs_canvas * BigCanvas[2];
//int CurrentBigCanvas = 0;
//int BigCanvasFirstTime = 1;

int	Found_seg_index=0;				// Index in Found_segs corresponding to Cursegp

static void print_status_bar(const std::array<char, DIAGNOSTIC_MESSAGE_MAX> &message)
{
	gr_set_default_canvas();
	auto &canvas = *grd_curcanv;
	const auto &editor_font = *::editor_font;
	gr_set_fontcolor(canvas, CBLACK, CGREY);
	const auto &&[w, h] = gr_get_string_size(editor_font, message.data());
	gr_string(canvas, editor_font, 4, 583, message.data(), w, h);
	gr_set_fontcolor(canvas, CBLACK, CWHITE);
	gr_rect(canvas, 4+w, 583, 799, 599, CGREY);
}

static std::array<char, DIAGNOSTIC_MESSAGE_MAX> status_line;

struct tm	Editor_status_last_time;

void (editor_status_fmt)( const char *format, ... )
{
	va_list ap;

	va_start(ap, format);
	vsnprintf(status_line.data(), status_line.size(), format, ap);
	va_end(ap);

	Editor_status_last_time = Editor_time_of_day;
}

void editor_status( const char *text)
{
	Editor_status_last_time = Editor_time_of_day;
	strcpy(status_line.data(), text);
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

static void clear_editor_status(void)
{
	int cur_time = Editor_time_of_day.tm_hour * 3600 + Editor_time_of_day.tm_min*60 + Editor_time_of_day.tm_sec;
	int erase_time = Editor_status_last_time.tm_hour * 3600 + Editor_status_last_time.tm_min*60 + Editor_status_last_time.tm_sec + EDITOR_STATUS_MESSAGE_DURATION;

	if (cur_time > erase_time) {
		std::fill(status_line.begin(), std::prev(status_line.end()), ' ');
		status_line.back() = 0;
		Editor_status_last_time.tm_hour = 99;
	}
}

static inline void editor_slew_init()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	Viewer = ConsoleObject;
	slew_init(vmobjptr(ConsoleObject));
	init_player_object(LevelSharedPolygonModelState, *ConsoleObject);
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
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	pause_game_world_time p;

//@@	init_player_stats();
//@@
//@@	Player_init.pos = Player->pos;
//@@	Player_init.orient = Player->orient;
//@@	Player_init.segnum = Player->segnum;	


// -- must always save gamesave.sge/lvl because the restore-objects code relies on it
// -- that code could be made smarter and use the original file, if appropriate.
//	if (mine_changed)
	switch (gamestate)
	{
		case editor_gamestate::none:
			// Always use the simple mission when playing level (for now at least)
			create_new_mission();
			Current_level_num = 1;
			if (save_level(
#if defined(DXX_BUILD_DESCENT_II)
					LevelSharedSegmentState.DestructibleLights,
#endif
					"GAMESAVE.LVL"))
				return 0;
			editor_status("Level saved.\n");
			break;

		case editor_gamestate::unsaved:
		case editor_gamestate::saved:
			if (!SafetyCheck())		// if mine edited but not saved, warn the user!
				return 0;

			const auto &&console = vmobjptr(get_local_player().objnum);
			Viewer = ConsoleObject = console;
			set_player_id(console, Player_num);
			fly_init(*ConsoleObject);

			if (!state_save_all_sub(PLAYER_DIRECTORY_STRING("gamesave.sge"), "Editor generated"))
			{
				editor_slew_init();

				return 0;
			}
			gamestate = editor_gamestate::saved;
			editor_status("Game saved.\n");
			break;
	}

	ModeFlag = 3;
	return 1;
}

int GotoMainMenu()
{
	ModeFlag = 2;
	return 1;
}

static std::array<int (*)(), 2048> KeyFunction;

static void medkey_init()
{
	char keypress[100];
	int key;
	int np;
	char LispCommand[DIAGNOSTIC_MESSAGE_MAX];

	KeyFunction = {};

	static char global_key_basename[]{"GLOBAL.KEY"};
	if (auto keyfile{PHYSFSX_openReadBuffered(global_key_basename).first})
	{
		for (PHYSFSX_gets_line_t<200> line_buffer; PHYSFSX_fgets(line_buffer, keyfile);)
		{
			sscanf(line_buffer, " %s %s ", keypress, LispCommand);
			//ReadLispMacro( keyfile, LispCommand );

			if ( (key=DecodeKeyText( keypress ))!= -1 )
			{
				Assert( key < 2048);
				KeyFunction[key] = func_get( LispCommand, &np );
			} else {
				UserError( "Bad key %s in GLOBAL.KEY!", keypress );
			}
		}
	}
}

static int padnum=0;
//@@short camera_objnum;			//a camera for viewing. Who knows, might become handy

static void init_editor_screen(grs_canvas &canvas, std::array<RAIIPHYSFS_ComputedPathMount, 4> &&search_path_editor);
static void gamestate_restore_check();
static void close_editor();

namespace dsx {
void init_editor()
{
	static const char pads[][13] = {
		"segmove.pad",
		"segsize.pad",
		"curve.pad",
		"texture.pad",
		"object.pad",
		"objmov.pad",
		"group.pad",
		"lighting.pad",
		"test.pad"
					};
	ModeFlag = Game_wind ? 3 : 2;	// go back to where we were unless we loaded everything properly
	std::array<RAIIPHYSFS_ComputedPathMount, 4> search_path_editor;
	// first, make sure we can find the files we need
	{
		static char relname[]{"editor/data"};
		search_path_editor[0] = make_PHYSFSX_ComputedPathMount(relname, physfs_search_path::append);	// look in source directory first (for work in progress)
	}
	{
		static char relname[]{"editor"};
		search_path_editor[1] = make_PHYSFSX_ComputedPathMount(relname, physfs_search_path::append);		// then in editor directory
	}
	{
		static char relname[]{"editor.zip"};
		search_path_editor[2] = make_PHYSFSX_ComputedPathMount(relname, physfs_search_path::append);	// then in a zip file
	}
	{
		static char relname[]{"editor.dxa"};
		search_path_editor[3] = make_PHYSFSX_ComputedPathMount(relname, physfs_search_path::append);	// or addon pack
	}

	if (!ui_init())
	{
		close_editor();
		return;
	}

	for (auto &&[idx, value] : enumerate(pads))
		if (!ui_pad_read(idx, value))
		{
			close_editor();
			return;
		}

	medkey_init();

	game_flush_inputs(Controls);
	
	editor_font = gr_init_font(*grd_curcanv, "pc8x16.fnt");
	if (!editor_font)
	{
		Warning_puts("Failed to init font pc8x16.fnt");
		close_editor();
		return;
	}
	
	if (!menubar_init(*grd_curcanv, "MED.MNU"))
	{
		close_editor();
		return;
	}

	Draw_all_segments = 1;						// Say draw all segments, not just connected ones
	
	if (!Cursegp)
		Cursegp = imsegptridx(segment_first);

	init_autosave();
  
	Clear_window = 1;	//	do full window clear.
	
	InitCurve();
	
	restore_effect_bitmap_icons();
	
	if (!set_screen_mode(SCREEN_EDITOR))
	{
		close_editor();
		return;
	}
#if defined(DXX_BUILD_DESCENT_I)
	gr_use_palette_table( "palette.256" );
	gr_palette_load( gr_palette );
#elif defined(DXX_BUILD_DESCENT_II)
	load_palette(Current_level_palette, load_palette_use::level, load_palette_change_screen::immediate);
#endif
	
	//Editor renders into full (320x200) game screen 
	
	game_init_render_sub_buffers(*grd_curcanv, 0, 0, 320, 200);
	gr_init_sub_canvas(_canv_editor, grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT);
	Canv_editor = &_canv_editor;
	gr_set_current_canvas(*Canv_editor);
	init_editor_screen(*grd_curcanv, std::move(search_path_editor)); // load the main editor dialog
	gr_set_default_canvas();
	set_warn_func(med_show_warning);
	
	//	_MARK_("start of editor");//Nuked to compile -KRB
	
	//@@	//create a camera for viewing in the editor. copy position from ConsoleObject
	//@@	camera_objnum = obj_create(OBJ_CAMERA,0,ConsoleObject->segnum,&ConsoleObject->pos,&ConsoleObject->orient,0);
	//@@	Viewer = &Objects[camera_objnum];
	//@@	slew_init(Viewer);		//camera is slewing
	
	editor_slew_init();
	
	Update_flags = UF_ALL;
	
	//set the wire-frame window to be the current view
	current_view = &LargeView;
	
	gr_set_current_canvas( GameViewBox->canvas );
	gr_set_curfont(*grd_curcanv, *editor_font);
	// No font scaling!
	FNTScaleX.reset(1);
	FNTScaleY.reset(1);
	ui_pad_goto(padnum);
	
	ModeFlag = 0;	// success!
	
	gamestate_restore_check();
}
}

int ShowAbout()
{
	ui_messagebox( -2, -2, 1, 	"INFERNO Mine Editor\n\n"		\
									"Copyright (c) 1993  Parallax Software Corp.",
									"OK");
	return 0;
}

int SetPlayerFromCurseg()
{
	move_player_2_segment(Cursegp,Curside);
	Update_flags |= UF_ED_STATE_CHANGED | UF_GAME_VIEW_CHANGED;
	return 1;
}

int fuelcen_create_from_curseg()
{
	Cursegp->special = segment_special::fuelcen;
	fuelcen_activate(Cursegp);
	return 1;
}

int repaircen_create_from_curseg()
{
	Cursegp->special = segment_special::repaircen;
	fuelcen_activate(Cursegp);
	return 1;
}

int controlcen_create_from_curseg()
{
	Cursegp->special = segment_special::controlcen;
	fuelcen_activate(Cursegp);
	return 1;
}

int robotmaker_create_from_curseg()
{
	Cursegp->special = segment_special::robotmaker;
	fuelcen_activate(Cursegp);
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

static void move_player_2_segment_and_rotate(const vmsegptridx_t seg, const sidenum_t side)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	auto &vmobjptridx = Objects.vmptridx;
	static side_relative_vertnum edgenum;

	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vcvertptr = Vertices.vcptr;
	ConsoleObject->pos = compute_segment_center(vcvertptr, seg);
	auto vp{compute_center_point_on_side(vcvertptr, seg, side)};
	vm_vec_sub2(vp,ConsoleObject->pos);

	auto &sv = Side_to_verts[Curside];
	auto &verts = Cursegp->verts;
	const auto en = std::exchange(edgenum, next_side_vertex(edgenum));
	vm_vector_to_matrix_u(ConsoleObject->orient, vp, vm_vec_sub(vcvertptr(verts[sv[en]]), vcvertptr(verts[sv[next_side_vertex(en, 3)]])));
	obj_relink(vmobjptr, vmsegptr, vmobjptridx(ConsoleObject), seg);
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
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	auto &vmobjptridx = Objects.vmptridx;
	std::array<g3s_point, 4> corner_p;
	fix max,view_dist=f1_0*10;
	static side_relative_vertnum edgenum;
	const auto view_vec = vm_vec_negated(Cursegp->shared_segment::sides[Curside].normals[0]);

	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vcvertptr = Vertices.vcptr;
	const auto side_center{compute_center_point_on_side(vcvertptr, Cursegp, Curside)};
	const auto view_vec2{vm_vec_copy_scale(view_vec, view_dist)};
	vm_vec_sub(ConsoleObject->pos,side_center,view_vec2);

	auto &sv = Side_to_verts[Curside];
	auto &verts = Cursegp->verts;
	const auto en = std::exchange(edgenum, next_side_vertex(edgenum));
	vm_vector_to_matrix_u(ConsoleObject->orient, view_vec, vm_vec_sub(vcvertptr(verts[sv[en]]), vcvertptr(verts[sv[next_side_vertex(en, 3)]])));

	gr_set_current_canvas(*Canv_editor_game);
	g3_start_frame(*grd_curcanv);
	g3_set_view_matrix(ConsoleObject->pos,ConsoleObject->orient,Render_zoom);

	max = 0;
	for (auto &&[corner, vert] : zip(corner_p, sv))
	{
		g3_rotate_point(corner, vcvertptr(verts[vert]));
		if (labs(corner.p3_vec.x) > max) max = labs(corner.p3_vec.x);
		if (labs(corner.p3_vec.y) > max) max = labs(corner.p3_vec.y);
	}

	view_dist = fixmul(view_dist,fixdiv(fixdiv(max,SIDE_VIEW_FRAC),corner_p[0].p3_vec.z));
	const auto view_vec3{vm_vec_copy_scale(view_vec, view_dist)};
	vm_vec_sub(ConsoleObject->pos,side_center,view_vec3);

	//obj_relink(ConsoleObject-Objects, SEG_PTR_2_NUM(Cursegp) );
	//update_object_seg(ConsoleObject);		//might have backed right out of curseg

	const auto &&newseg = find_point_seg(LevelSharedSegmentState, LevelUniqueSegmentState, ConsoleObject->pos, Cursegp);
	if (newseg != segment_none)
		obj_relink(vmobjptr, vmsegptr, vmobjptridx(ConsoleObject), newseg);

	Update_flags |= UF_ED_STATE_CHANGED | UF_GAME_VIEW_CHANGED;
	return 1;
}

#if !DXX_USE_OGL
static int ToggleLighting(void)
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
#endif

int FindConcaveSegs()
{
	find_concave_segs();

	Update_flags |= UF_ED_STATE_CHANGED;		//list may have changed

	return 1;
}

static int ToggleOutlineMode()
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
//@@	* reinterpret_cast<uint8_t *>(0x417) &= ~0x20;
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


static int med_keypad_goto_0()	{	ui_pad_goto(0);	return 0;	}
static int med_keypad_goto_1()	{	ui_pad_goto(1);	return 0;	}
static int med_keypad_goto_2()	{	ui_pad_goto(2);	return 0;	}
static int med_keypad_goto_3()	{	ui_pad_goto(3);	return 0;	}
static int med_keypad_goto_4()	{	ui_pad_goto(4);	return 0;	}
static int med_keypad_goto_5()	{	ui_pad_goto(5);	return 0;	}
static int med_keypad_goto_6()	{	ui_pad_goto(6);	return 0;	}
static int med_keypad_goto_7()	{	ui_pad_goto(7);	return 0;	}
static int med_keypad_goto_8()	{	ui_pad_goto(8);	return 0;	}

#define	PAD_WIDTH	30
#define	PAD_WIDTH1	(PAD_WIDTH + 7)

int editor_screen_open = 0;

//setup the editors windows, canvases, gadgets, etc.
static void init_editor_screen(grs_canvas &canvas, std::array<RAIIPHYSFS_ComputedPathMount, 4> &&search_path_editor)
{	
//	grs_bitmap * bmp;

	if (editor_screen_open) return;

	grd_curscreen->sc_canvas.cv_font = editor_font.get();
	
	//create canvas for game on the editor screen
	initializing = 1;
	gr_set_current_canvas(*Canv_editor);
	Canv_editor->cv_font = editor_font.get();
	gr_init_sub_canvas(*Canv_editor_game, *Canv_editor, GAMEVIEW_X, GAMEVIEW_Y, GAMEVIEW_W, GAMEVIEW_H);
	
	//Editor renders into full (320x200) game screen 

	init_info = 1;

	//do other editor screen setup

	// Since the palette might have changed, find some good colors...
	CBLACK = gr_find_closest_color( 1, 1, 1 );
	CGREY = gr_find_closest_color( 28, 28, 28 );
	CWHITE = gr_find_closest_color( 38, 38, 38 );
	CBRIGHT = gr_find_closest_color( 60, 60, 60 );
	CRED = gr_find_closest_color( 63, 0, 0 );

	gr_set_curfont(canvas, *editor_font);
	gr_set_fontcolor(canvas, CBLACK, CWHITE);

	auto e = window_create<editor_dialog>(0, 0, ED_SCREEN_W, ED_SCREEN_H, DF_FILLED, std::move(search_path_editor));
	EditorWindow = e;

	LargeViewBox = ui_add_gadget_userbox(*e, LVIEW_X, LVIEW_Y, LVIEW_W, LVIEW_H);
	ui_gadget_calc_keys(*e);	//make tab work for all windows

	GameViewBox	= ui_add_gadget_userbox(*e, GAMEVIEW_X, GAMEVIEW_Y, GAMEVIEW_W, GAMEVIEW_H);

//	GameViewBox->when_tab = GameViewBox->when_btab =  LargeViewBox;
//	LargeViewBox->when_tab = LargeViewBox->when_btab =  GameViewBox;

	ViewIcon	= ui_add_gadget_icon(*e, "Lock\nview", 455, 25 + 530, 40, 22,	KEY_V+KEY_CTRLED, ToggleLockViewToCursegp);
	AllIcon	= ui_add_gadget_icon(*e, "Draw\nall", 500, 25 + 530, 40, 22,	-1, ToggleDrawAllSegments);
	AxesIcon	= ui_add_gadget_icon(*e, "Coord\naxes", 545, 25 + 530, 40, 22,	KEY_D+KEY_CTRLED, ToggleCoordAxes);
	ChaseIcon	= ui_add_gadget_icon(*e, "Chase\nmode", 635, 25 + 530, 40, 22,	-1,				ToggleChaseMode);
	OutlineIcon = ui_add_gadget_icon(*e, "Out\nline", 680, 25 + 530, 40, 22,	KEY_O+KEY_CTRLED,			ToggleOutlineMode);
	LockIcon	= ui_add_gadget_icon(*e, "Lock\nstep", 725, 25 + 530, 40, 22,	KEY_L+KEY_CTRLED,			ToggleLockstep);

	meddraw_init_views(LargeViewBox->canvas.get());

	ui_pad_activate(*e, PAD_X, PAD_Y);
	Pad_info = info_window_create();
	e->pad_prev = ui_add_gadget_button(*e, PAD_X + 6, PAD_Y + (30 * 5) + 22, PAD_WIDTH, 20, "<<", med_keypad_goto_prev);
	e->pad_next = ui_add_gadget_button(*e, PAD_X + PAD_WIDTH1 + 6, PAD_Y + (30 * 5) + 22, PAD_WIDTH, 20, ">>", med_keypad_goto_next);

	{	int	i;
		i = 0;	e->pad_goto[i] = ui_add_gadget_button(*e, PAD_X+16+(i+2)*PAD_WIDTH1, PAD_Y+(30*5)+22, PAD_WIDTH, 20, "SR", med_keypad_goto_0);
		i++;		e->pad_goto[i] = ui_add_gadget_button(*e, PAD_X+16+(i+2)*PAD_WIDTH1, PAD_Y+(30*5)+22, PAD_WIDTH, 20, "SS", med_keypad_goto_1);
		i++;		e->pad_goto[i] = ui_add_gadget_button(*e, PAD_X+16+(i+2)*PAD_WIDTH1, PAD_Y+(30*5)+22, PAD_WIDTH, 20, "CF", med_keypad_goto_2);
		i++;		e->pad_goto[i] = ui_add_gadget_button(*e, PAD_X+16+(i+2)*PAD_WIDTH1, PAD_Y+(30*5)+22, PAD_WIDTH, 20, "TM", med_keypad_goto_3);
		i++;		e->pad_goto[i] = ui_add_gadget_button(*e, PAD_X+16+(i+2)*PAD_WIDTH1, PAD_Y+(30*5)+22, PAD_WIDTH, 20, "OP", med_keypad_goto_4);
		i++;		e->pad_goto[i] = ui_add_gadget_button(*e, PAD_X+16+(i+2)*PAD_WIDTH1, PAD_Y+(30*5)+22, PAD_WIDTH, 20, "OR", med_keypad_goto_5);
		i++;		e->pad_goto[i] = ui_add_gadget_button(*e, PAD_X+16+(i+2)*PAD_WIDTH1, PAD_Y+(30*5)+22, PAD_WIDTH, 20, "GE", med_keypad_goto_6);
		i++;		e->pad_goto[i] = ui_add_gadget_button(*e, PAD_X+16+(i+2)*PAD_WIDTH1, PAD_Y+(30*5)+22, PAD_WIDTH, 20, "LI", med_keypad_goto_7);
		i++;		e->pad_goto[i] = ui_add_gadget_button(*e, PAD_X+16+(i+2)*PAD_WIDTH1, PAD_Y+(30*5)+22, PAD_WIDTH, 20, "TT", med_keypad_goto_8);
	}

	menubar_show();

	// INIT TEXTURE STUFF
	texpage_init(e);
	objpage_init(e);

	e->keyboard_focus_gadget = LargeViewBox.get();

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

	close_all_windows();

	// CLOSE TEXTURE STUFF
	texpage_close();
	objpage_close();

	menubar_hide();

}

static void med_show_warning(std::span<const char> s)
{
	grs_canvas &save_canv = *grd_curcanv;

	//gr_pal_fade_in(grd_curscreen->pal);	//in case palette is blacked

	ui_messagebox(-2, -2, 1, s.data(), "OK");
	gr_set_current_canvas(save_canv);
}

// Returns 1 if OK to trash current mine.
int SafetyCheck()
{
	if (mine_changed) {
		const auto x = nm_messagebox_str(menu_title{"Warning!"}, nm_messagebox_tie("Cancel", TXT_OK), menu_subtitle{"You are about to lose work."});
		if (x<1) {
			return 0;
		}
	}
	return 1;
}

//called at the end of the program

static void close_editor()
{
	//	_MARK_("end of editor");//Nuked to compile -KRB
	
#if defined(WIN32) || defined(__APPLE__) || defined(__MACH__)
	set_warn_func(msgbox_warning);
#else
	clear_warn_func();
#endif
	
	close_editor_screen();
	
	//kill our camera object
	
	Viewer = ConsoleObject;					//reset viewer
	//@@obj_delete(camera_objnum);
	
	padnum = ui_pad_get_current();
	
	close_autosave();

	ui_close();

	editor_font.reset();

	switch (ModeFlag)
	{
		case 1:
			break;

		case 2:
			set_screen_mode(SCREEN_MENU);		//put up menu screen
			show_menus();
			break;

		case 3:
			if (Game_wind)
				return;	// if we're already playing a game, don't touch!

			switch (gamestate)
			{
				case editor_gamestate::none:
					StartNewGame(Current_level_num);
					break;

				case editor_gamestate::saved:
					state_restore_all_sub(
#if defined(DXX_BUILD_DESCENT_II)
						LevelSharedSegmentState.DestructibleLights, secret_restore::none,
#endif
						PLAYER_DIRECTORY_STRING("gamesave.sge")
					);
					break;

				default:
					Int3();	// shouldn't happen
					break;
			}
			break;
	}

	return;
}

//variables for find segments process

// ---------------------------------------------------------------------------------------------------
//	Subtract all elements in Found_segs from selected list.
static void subtract_found_segments_from_selected_list(void)
{
	range_for (const auto &foundnum, Found_segs)
	{
		selected_segment_array_t::iterator i = Selected_segs.find(foundnum), e = Selected_segs.end();
		if (i != e)
		{
			*i = *-- e;
			Selected_segs.erase(e);
		}
	}
}

// ---------------------------------------------------------------------------------------------------
//	Add all elements in Found_segs to selected list.
static void add_found_segments_to_selected_list(void) {
	range_for (const auto &foundnum, Found_segs)
	{
		selected_segment_array_t::iterator i = Selected_segs.find(foundnum), e = Selected_segs.end();
		if (i == e)
			Selected_segs.emplace_back(foundnum);
	}
}

void gamestate_restore_check()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	auto &vmobjptridx = Objects.vmptridx;
	obj_position Save_position;

	if (gamestate == editor_gamestate::saved)
	{
		if (ui_messagebox(-2, -2, 2, "Do you wish to restore game state?\n", "Yes", "No") == 1)
		{

			// Save current position
			Save_position.pos = ConsoleObject->pos;
			Save_position.orient = ConsoleObject->orient;
			Save_position.segnum = ConsoleObject->segnum;

			if (!state_restore_all_sub(
#if defined(DXX_BUILD_DESCENT_II)
					LevelSharedSegmentState.DestructibleLights, secret_restore::none,
#endif
					PLAYER_DIRECTORY_STRING("gamesave.sge")
			))
				return;

			// Switch back to slew mode - loading saved game made ConsoleObject flying
			editor_slew_init();

			// Restore current position
			if (Save_position.segnum <= Highest_segment_index) {
				ConsoleObject->pos = Save_position.pos;
				ConsoleObject->orient = Save_position.orient;
				obj_relink(vmobjptr, vmsegptr, vmobjptridx(ConsoleObject), vmsegptridx(Save_position.segnum));
			}

			Update_flags |= UF_WORLD_CHANGED;	
		}
		else
			gamestate = editor_gamestate::none;
	}
}

int RestoreGameState()
{
	if (!SafetyCheck())
		return 0;

	if (!state_restore_all_sub(
#if defined(DXX_BUILD_DESCENT_II)
			LevelSharedSegmentState.DestructibleLights, secret_restore::none,
#endif
			PLAYER_DIRECTORY_STRING("gamesave.sge")
	))
		return 0;

	// Switch back to slew mode - loading saved game made ConsoleObject flying
	editor_slew_init();
	
	gamestate = editor_gamestate::saved;

	editor_status("Gamestate restored.\n");

	Update_flags |= UF_WORLD_CHANGED;
	return 1;
}

editor_dialog::editor_dialog(short x, short y, const short w, const short h, const enum dialog_flags flags, std::array<RAIIPHYSFS_ComputedPathMount, 4> &&search_path_editor) :
	UI_DIALOG(x, y, w, h, flags), search_path_editor{std::move(search_path_editor)}
{
}

// Handler for the main editor dialog
window_event_result editor_dialog::callback_handler(const d_event &event)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	editor_view *new_cv;
	int keypress = 0;
	window_event_result rval = window_event_result::ignored;

	if (event.type == event_type::window_created)
		return window_event_result::ignored;

	if (event.type == event_type::key_command)
		keypress = event_key_get(event);
	else if (event.type == event_type::window_close)
	{
		EditorWindow = NULL;
		close_editor();
		return window_event_result::ignored;
	}
	// Update the windows

	if (event.type == event_type::ui_dialog_draw)
	{
		// Draw status box
		gr_set_default_canvas();
		gr_rect(*grd_curcanv, STATUS_X,STATUS_Y,STATUS_X+STATUS_W-1,STATUS_Y+STATUS_H-1, CGREY);
		medlisp_update_screen();
		calc_frame_time();
		texpage_do(event);
		objpage_do(event);
		ui_pad_draw(EditorWindow, PAD_X, PAD_Y);

		print_status_bar(status_line);
		TimedAutosave(mine_filename);	// shows the time, hence here
		set_editor_time_of_day();
		return window_event_result::handled;
	}
	if ((selected_gadget == GameViewBox.get() && !render_3d_in_big_window) ||
		(selected_gadget == LargeViewBox.get() && render_3d_in_big_window))
		switch (event.type)
		{
			case event_type::mouse_button_up:
			case event_type::mouse_button_down:
				break;
			case event_type::mouse_moved:
				if (!keyd_pressed[ KEY_LCTRL ] && !keyd_pressed[ KEY_RCTRL ])
					break;
				[[fallthrough]];
#if DXX_MAX_BUTTONS_PER_JOYSTICK
			case event_type::joystick_button_up:
			case event_type::joystick_button_down:
#endif
#if DXX_MAX_AXES_PER_JOYSTICK
			case event_type::joystick_moved:
#endif
			case event_type::key_command:
			case event_type::key_release:
			case event_type::idle:
				kconfig_read_controls(Controls, event, 1);

				if (slew_frame(0))
				{		//do movement and check keys
					Update_flags |= UF_GAME_VIEW_CHANGED;
					if (Gameview_lockstep)
					{
						Cursegp = imsegptridx(ConsoleObject->segnum);
						med_create_new_segment_from_cursegp();
						Update_flags |= UF_ED_STATE_CHANGED;
					}

					rval = window_event_result::handled;
				}
				break;
			case event_type::loop_begin_loop:
				kconfig_begin_loop(Controls);
				break;
			case event_type::loop_end_loop:
				kconfig_end_loop(Controls, FrameTime);
				break;

			default:
				break;
		}

	//do non-essential stuff in idle event
	if (event.type == event_type::idle)
	{
		check_wall_validity();

		if (Gameview_lockstep) {
			static segment *old_cursegp=NULL;
			static sidenum_t old_curside = side_none;

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
	if ((keypress&0xff)==KEY_LMETA) keypress=0;
	if ((keypress&0xff)==KEY_RMETA) keypress=0;
//		if ((keypress&0xff)==KEY_LALT) keypress=0;
//		if ((keypress&0xff)==KEY_RALT) keypress=0;

	//=================== DO FUNCTIONS ====================

	if ( KeyFunction[ keypress ] != NULL )
	{
		KeyFunction[keypress]();
		keypress = 0;
		rval = window_event_result::handled;
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
#if !DXX_USE_OGL
			ToggleLighting();
#endif
			rval = window_event_result::handled;
			break;
		case KEY_F1:
			render_3d_in_big_window = !render_3d_in_big_window;
			Update_flags |= UF_ALL;
			rval = window_event_result::handled;
			break;			
		default:
			if (rval == window_event_result::ignored)
			{
				char kdesc[100];
				GetKeyDescription( kdesc, keypress );
				editor_status_fmt("Error: %s isn't bound to anything.", kdesc  );
			}
	}

	//================================================================

	if (ModeFlag)
	{
		return window_event_result::close;
	}

	new_cv = current_view;

	if (new_cv != current_view ) {
		current_view->ev_changed = 1;
		new_cv->ev_changed = 1;
		current_view = new_cv;
	}

	// DO TEXTURE STUFF
	if (texpage_do(event))
		rval = window_event_result::handled;
	
	if (objpage_do(event))
		rval = window_event_result::handled;


	// Process selection of Cursegp using mouse.
	if (GADGET_PRESSED(LargeViewBox.get()) && !render_3d_in_big_window) 
	{
		int	xcrd,ycrd;
		xcrd = LargeViewBox->b1_drag_x1;
		ycrd = LargeViewBox->b1_drag_y1;

		find_segments(xcrd,ycrd,LargeViewBox->canvas.get(),&LargeView,Cursegp,Big_depth);	// Sets globals N_found_segs, Found_segs

		// If shift is down, then add segment to found list
		if (keyd_pressed[ KEY_LSHIFT ] || keyd_pressed[ KEY_RSHIFT ])
			subtract_found_segments_from_selected_list();
		else
			add_found_segments_to_selected_list();

		Found_seg_index = 0;	
	
		if (!Found_segs.empty())
		{
			sort_seg_list(Found_segs,ConsoleObject->pos);
			Cursegp = imsegptridx(Found_segs[0]);
			med_create_new_segment_from_cursegp();
			if (Lock_view_to_cursegp)
			{
				auto &vcvertptr = Vertices.vcptr;
				set_view_target_from_segment(vcvertptr, Cursegp);
			}
		}

		Update_flags |= UF_ED_STATE_CHANGED | UF_VIEWPOINT_MOVED;
	}

	if (event.type == event_type::ui_userbox_dragged && &ui_event_get_gadget(event) == GameViewBox.get())
	{
		int	x, y;
		x = GameViewBox->b1_drag_x2;
		y = GameViewBox->b1_drag_y2;

		gr_set_current_canvas( GameViewBox->canvas );
		gr_rect(*grd_curcanv, x-1, y-1, x+1, y+1, 15);
	}
	
	// Set current segment and side by clicking on a polygon in game window.
	//	If ctrl pressed, also assign current texture map to that side.
	//if (GameViewBox->mouse_onme && (GameViewBox->b1_done_dragging || GameViewBox->b1_clicked)) {
	if ((GADGET_PRESSED(GameViewBox.get()) && !render_3d_in_big_window) ||
		(GADGET_PRESSED(LargeViewBox.get()) && render_3d_in_big_window))
	{
		int	xcrd,ycrd;
		int face;

		if (render_3d_in_big_window) {
			xcrd = LargeViewBox->b1_drag_x1;
			ycrd = LargeViewBox->b1_drag_y1;
		}
		else {
			xcrd = GameViewBox->b1_drag_x1;
			ycrd = GameViewBox->b1_drag_y1;
		}

		//Int3();

		segnum_t seg;
		objnum_t obj;
		sidenum_t side;
		if (find_seg_side_face(xcrd,ycrd,seg,obj,side,face))
		{

			if (obj != object_none) {							//found an object

				Cur_object_index = obj;
				editor_status_fmt("Object %d selected.",Cur_object_index);

				Update_flags |= UF_ED_STATE_CHANGED;
			}
			else {

				//	See if either shift key is down and, if so, assign texture map
				if (keyd_pressed[KEY_LSHIFT] || keyd_pressed[KEY_RSHIFT]) {
					Cursegp = imsegptridx(seg);
					Curside = side;
					AssignTexture();
					med_create_new_segment_from_cursegp();
					editor_status("Texture assigned");
				} else if (keyd_pressed[KEY_G])	{
					const unique_segment &us = vcsegptr(seg);
					const auto tmap = us.sides[side].tmap_num;
					texpage_grab_current(tmap);
					editor_status( "Texture grabbed." );
				} else if (keyd_pressed[ KEY_LAPOSTRO] ) {
					move_object_to_mouse_click();
				} else {
					Cursegp = imsegptridx(seg);
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
	if (event.type == event_type::mouse_moved && (keyd_pressed[ KEY_LCTRL ] || keyd_pressed[ KEY_RCTRL ]))
	{
		const auto [dx, dy, dz] = event_mouse_get_delta(event);
		if ((dx != 0) && (dy != 0))
		{
			vms_matrix	MouseRotMat;
			
			GetMouseRotation( dx, dy, &MouseRotMat );
			LargeView.ev_matrix = vm_matrix_x_matrix(LargeView.ev_matrix,MouseRotMat);
			LargeView.ev_changed = 1;
			Large_view_index = -1;			// say not one of the orthogonal views
			rval = window_event_result::handled;
		}
	}

	if (event.type == event_type::mouse_moved)
	{
		const auto [dx, dy, dz] = event_mouse_get_delta(event);
		if (dz != 0)
		{
			current_view->ev_dist += dz*10000;
			current_view->ev_changed = 1;
		}
	}
	
	return rval;
}
