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
 * Game loop for Inferno
 *
 */

#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#ifdef __unix__
#include <sys/stat.h>
#include <sys/types.h>
#else
#include <dir.h>
#endif
#ifndef _MSC_VER
#include <unistd.h>
#else
#include <io.h>
#define access(a,b) _access(a,b)
#endif
#include <SDL/SDL.h>
#ifdef OGL
#include "ogl_init.h"
#endif
#include "inferno.h"
#include "game.h"
#include "key.h"
#include "object.h"
#include "physics.h"
#include "error.h"
#include "joy.h"
#include "mono.h"
#include "iff.h"
#include "pcx.h"
#include "timer.h"
#include "render.h"
#include "laser.h"
#include "screens.h"
#include "textures.h"
#include "slew.h"
#include "gauges.h"
#include "texmap.h"
#include "3d.h"
#include "effects.h"
#include "menu.h"
#include "gameseg.h"
#include "wall.h"
#include "ai.h"
#include "hostage.h"
#include "fuelcen.h"
#include "switch.h"
#include "digi.h"
#include "gamesave.h"
#include "scores.h"
#include "ibitblt.h"
#include "u_mem.h"
#include "palette.h"
#include "morph.h"
#include "lighting.h"
#include "newdemo.h"
#include "titles.h"
#include "collide.h"
#include "weapon.h"
#include "sounds.h"
#include "args.h"
#include "gameseq.h"
#include "automap.h"
#include "text.h"
#include "powerup.h"
#include "fireball.h"
#include "controls.h"
#include "newmenu.h"
#include "network.h"
#include "gamefont.h"
#include "endlevel.h"
#include "joydefs.h"
#include "kconfig.h"
#include "mouse.h"
#include "multi.h"
#include "desc_id.h"
#include "cntrlcen.h"
#include "pcx.h"
#include "state.h"
#include "piggy.h"
#include "multibot.h"
#include "gr.h"
#include "reorder.h"
#include "hudmsg.h"
#include "timer.h"
#include "radar.h"
#include "vers_id.h"
#include "ban.h"
#include "vlcnfire.h"
#include "fvi.h"
//MD2211
#include "jukebox.h"

extern void change_res();
extern void write_player_file();
extern void newmenu_close();

#define	SHOW_EXIT_PATH	1
#define FINAL_CHEATS 1

#define key_isfunc(k) (((k&0xff)>=KEY_F1 && (k&0xff)<=KEY_F10) || (k&0xff)==KEY_F11 || (k&0xff)==KEY_F12)
#define key_ismod(k)  ((k&0xff)==KEY_LALT || (k&0xff)==KEY_RALT || (k&0xff)==KEY_LSHIFT || (k&0xff)==KEY_RSHIFT || (k&0xff)==KEY_LCTRL || (k&0xff)==KEY_RCTRL)

#ifdef EDITOR
#include "editor/editor.h"
#endif

void game_init_render_sub_buffers( int x, int y, int w, int h );
void draw_centered_text( int y, char * s );
void GameLoop(int RenderFlag, int ReadControlsFlag );
void powerup_grab_cheat_all(void);

int	Speedtest_on = 0;
#if !defined(NDEBUG) || defined(EDITOR)
int	Mark_count = 0; // number of debugging marks set
#endif
#ifndef NDEBUG
int	Speedtest_start_time;
int	Speedtest_segnum;
int	Speedtest_sidenum;
int	Speedtest_frame_start;
int	Speedtest_count=0; // number of times to do the debug test.
#endif
static	fix last_timer_value=0;

ubyte new_cheats[]= {	KEY_B^0xaa, KEY_B^0xaa, KEY_B^0xaa, KEY_F^0xaa, KEY_A^0xaa,
			KEY_U^0xaa, KEY_I^0xaa, KEY_R^0xaa, KEY_L^0xaa, KEY_H^0xaa,
			KEY_G^0xaa, KEY_G^0xaa, KEY_U^0xaa, KEY_A^0xaa, KEY_I^0xaa,
			KEY_G^0xaa, KEY_R^0xaa, KEY_I^0xaa, KEY_S^0xaa, KEY_M^0xaa,
			KEY_I^0xaa, KEY_E^0xaa, KEY_N^0xaa, KEY_H^0xaa, KEY_S^0xaa,
			KEY_N^0xaa, KEY_D^0xaa, KEY_X^0xaa, KEY_X^0xaa, KEY_A^0xaa };

int			VR_render_mode = VR_NONE;
int			VR_low_res = 3; // Default to low res
int 			VR_show_hud = 1;
int			VR_sensitivity = 1; // 0 - 2
grs_canvas		Screen_3d_window; // The rectangle for rendering the mine to
grs_canvas		*VR_offscreen_buffer = NULL; // The offscreen data buffer
grs_canvas		VR_render_buffer[2]; //  Two offscreen buffers for left/right eyes.
grs_canvas		VR_render_sub_buffer[2]; //  Two sub buffers for left/right eyes.
grs_canvas		VR_editor_canvas; //  The canvas that the editor writes to.

//added 07/11/99 by adb:
//added buffer pointer to allow different buffers for 3D game rendering and
//the 2D menus (for DX3D port)
grs_canvas		*VR_offscreen_menu = NULL; // The offscreen data buffer for menus
//end additions -- adb

int	Debug_pause=0; //John's debugging pause system
int	Cockpit_mode=CM_FULL_COCKPIT; //set game.h for values
int	old_cockpit_mode=-1;
int	force_cockpit_redraw=0;
int	netplayerinfo_on=0;
int	PaletteRedAdd, PaletteGreenAdd, PaletteBlueAdd;
int	Dummy_var;
int	*Toggle_var = &Dummy_var;
#ifdef EDITOR
char	faded_in;
#endif

#ifndef NDEBUG //these only exist if debugging
fix	fixed_frametime=0; //if non-zero, set frametime to this
#endif

int	Game_suspended=0; //if non-zero, nothing moves but player
int	create_special_path(void);
void	fill_background(int x,int y,int w,int h,int dx,int dy);
fix	Auto_fire_fusion_cannon_time = 0;
fix	Fusion_charge = 0;
fix	Fusion_next_sound_time = 0;
int	Debug_spew = 1;
int	Game_turbo_mode = 0;
int	Game_mode = GM_GAME_OVER;
int	Global_laser_firing_count = 0;
int	Global_missile_firing_count = 0;
grs_bitmap background_bitmap;
int	Game_aborted;
void	update_cockpits(int force_redraw);
extern	void newdemo_strip_frames(char *, int);
extern int HUD_nmessages;
extern char WaitForRefuseAnswer,RefuseThisPlayer;

#define BACKGROUND_NAME "statback.pcx"

//	==============================================================================================

#ifndef NDEBUG
void speedtest_init(void)
{
	Speedtest_start_time = timer_get_fixed_seconds();
	Speedtest_on = 1;
	Speedtest_segnum = 0;
	Speedtest_sidenum = 0;
	Speedtest_frame_start = FrameCount;
	mprintf((0, "Starting speedtest.  Will be %i frames.  Each . = 10 frames.\n", Highest_segment_index+1));
}

void speedtest_frame(void)
{
	vms_vector	view_dir, center_point;

	Speedtest_sidenum=Speedtest_segnum % MAX_SIDES_PER_SEGMENT;

	compute_segment_center(&Viewer->pos, &Segments[Speedtest_segnum]);
	Viewer->pos.x += 0x10;		Viewer->pos.y -= 0x10;		Viewer->pos.z += 0x17;

	obj_relink(Viewer-Objects, Speedtest_segnum);
	compute_center_point_on_side(&center_point, &Segments[Speedtest_segnum], Speedtest_sidenum);
	vm_vec_normalized_dir_quick(&view_dir, &center_point, &Viewer->pos);
	vm_vector_2_matrix(&Viewer->orient, &view_dir, NULL, NULL);

	if (((FrameCount - Speedtest_frame_start) % 10) == 0)
		mprintf((0, "."));

	Speedtest_segnum++;

	if (Speedtest_segnum > Highest_segment_index) {
		mprintf((0, "\nSpeedtest done:  %i frames, %7.3f seconds, %7.3f frames/second.\n",
			FrameCount-Speedtest_frame_start,
			f2fl(timer_get_fixed_seconds() - Speedtest_start_time),
			(float) (FrameCount-Speedtest_frame_start) / f2fl(timer_get_fixed_seconds() - Speedtest_start_time)));
		Speedtest_count--;
		if (Speedtest_count == 0)
			Speedtest_on = 0;
		else
			speedtest_init();
	}
}

#endif

//this is called once per game
void init_game()
{
	ubyte pal[256*3];
	int pcx_error;

	atexit(close_game); //for cleanup

	init_objects();

	hostage_init();

	init_special_effects();

	init_ai_system();

	init_gauge_canvases();

	init_exploding_walls();

	gr_init_bitmap_data (&background_bitmap);
	pcx_error = pcx_read_bitmap(BACKGROUND_NAME,&background_bitmap,BM_LINEAR,pal);
	if (pcx_error != PCX_ERROR_NONE)
		Error("File %s - PCX error: %s",BACKGROUND_NAME,pcx_errormsg(pcx_error));
	gr_remap_bitmap_good( &background_bitmap, pal, -1, -1 );

	Clear_window = 2; // do portal only window clear.
}


void reset_palette_add()
{
	PaletteRedAdd 		= 0;
	PaletteGreenAdd	= 0;
	PaletteBlueAdd		= 0;
}


void game_show_warning(char *s)
{

	if (!((Game_mode & GM_MULTI) && (Function_mode == FMODE_GAME)))
		stop_time();

	nm_messagebox( TXT_WARNING, 1, TXT_OK, s );

	if (!((Game_mode & GM_MULTI) && (Function_mode == FMODE_GAME)))
		start_time();
}


//these should be in gr.h
#define cv_w  cv_bitmap.bm_w
#define cv_h  cv_bitmap.bm_h

u_int32_t Game_screen_mode = SM(640,480);

int last_drawn_cockpit = -1;
extern void ogl_loadbmtexture(grs_bitmap *bm);
extern void write_player_file();
extern int Rear_view;

// This actually renders the new cockpit onto the screen.
void update_cockpits(int force_redraw)
{
	grs_bitmap * bm;
	PIGGY_PAGE_IN(cockpit_bitmap[Cockpit_mode]);
	bm = &GameBitmaps[cockpit_bitmap[Cockpit_mode].index];

	//Redraw the on-screen cockpit bitmaps
	if (VR_render_mode != VR_NONE )	return;

	switch( Cockpit_mode )	{
		case CM_FULL_COCKPIT:
		case CM_REAR_VIEW:
			gr_set_current_canvas(NULL);
			bm->bm_flags |= BM_FLAG_COCKPIT_TRANSPARENT;
#ifdef OGL
			ogl_ubitmapm_cs (0, 0, -1, grd_curcanv->cv_bitmap.bm_h, bm,255, F1_0);
#else
			gr_ubitmapm(0,0, bm);
#endif
			break;
		case CM_FULL_SCREEN:
			break;
		case CM_STATUS_BAR:
			gr_set_current_canvas(NULL);
#ifdef OGL
			bm->bm_flags |= BM_FLAG_TRANSPARENT;
			bm->bm_flags |= BM_FLAG_COCKPIT_TRANSPARENT;
			ogl_ubitmapm_cs (0, (SHEIGHT*2)/2.72, -1, ((int) ((double) (bm->bm_h) * ((double)grd_curscreen->sc_h/200) + 0.5)), bm,255, F1_0);
#else
			gr_ubitmapm(0,146,bm);
#endif
			break;
		case CM_LETTERBOX:
			gr_set_current_canvas(NULL);
			break;
	}

	gr_set_current_canvas(NULL);

	if (Cockpit_mode != last_drawn_cockpit || force_redraw )
		last_drawn_cockpit = Cockpit_mode;
	else
		return;

	if (Cockpit_mode==CM_FULL_COCKPIT || Cockpit_mode==CM_STATUS_BAR)
		init_gauges();
}

extern void change_res_poll();

//initialize the various canvases on the game screen
//called every time the screen mode or cockpit changes
void init_cockpit()
{
	//Initialize the on-screen canvases

	if (Screen_mode != SCREEN_GAME)
		return;

	if (( Screen_mode == SCREEN_EDITOR ) || ( VR_render_mode != VR_NONE ))
		Cockpit_mode = CM_FULL_SCREEN;

#ifndef OGL
	if ( Game_screen_mode != SM(320,200) && Cockpit_mode != CM_LETTERBOX) {
		Cockpit_mode = CM_FULL_SCREEN;
	}
#endif

#ifdef D1XD3D
	Cockpit_mode = CM_STATUS_BAR;
#endif
	gr_set_current_canvas(NULL);
	gr_set_curfont( GAME_FONT );

	switch( Cockpit_mode )	{
	case CM_FULL_COCKPIT:
	case CM_REAR_VIEW:		{
		if (Cockpit_mode == CM_FULL_COCKPIT)
			game_init_render_sub_buffers(0, 0, grd_curscreen->sc_w, (grd_curscreen->sc_h*2)/3);
		else if (Cockpit_mode == CM_REAR_VIEW)
			game_init_render_sub_buffers((16*grd_curscreen->sc_w)/640, (89*grd_curscreen->sc_h)/480, (604*grd_curscreen->sc_w)/640, (209*grd_curscreen->sc_h)/480);
		break;
		}

	case CM_FULL_SCREEN:
		game_init_render_sub_buffers(0, 0, SWIDTH, SHEIGHT);
		break;

	case CM_STATUS_BAR:
		game_init_render_sub_buffers( 0, 0, SWIDTH, (SHEIGHT*2)/2.72 );
		break;

	case CM_LETTERBOX:	{
		int x,y,w,h;

		x = 0; w = SM_W(Game_screen_mode);
		h = (SM_H(Game_screen_mode) * 3) / 4; // true letterbox size (16:9)
		y = (SM_H(Game_screen_mode)-h)/2;

		gr_rect(x,0,w,SM_H(Game_screen_mode)-h);
		gr_rect(x,SM_H(Game_screen_mode)-h,w,SM_H(Game_screen_mode));

		game_init_render_sub_buffers( x, y, w, h );
		break;
		}
	}
	gr_set_current_canvas(NULL);
}

//selects a given cockpit (or lack of one).  See types in game.h
void select_cockpit(int mode)
{
	if (mode != Cockpit_mode) { //new mode
		Cockpit_mode=mode;
		init_cockpit();
	}
}

//force cockpit redraw next time. call this if you've trashed the screen
void reset_cockpit()
{
	force_cockpit_redraw=1;
	last_drawn_cockpit = -1;
}

void HUD_clear_messages();

void toggle_cockpit()
{
	int new_mode=CM_FULL_SCREEN;

	switch (Cockpit_mode)
	{
		case CM_FULL_COCKPIT:
			new_mode = CM_STATUS_BAR;
			break;
		case CM_STATUS_BAR:
			new_mode = CM_FULL_SCREEN;
			break;
		case CM_FULL_SCREEN:
			new_mode = CM_FULL_COCKPIT;
			break;
	}

	select_cockpit(new_mode);
	HUD_clear_messages();
	write_player_file();
}

void game_init_render_sub_buffers( int x, int y, int w, int h )
{
	gr_clear_canvas(0);
	gr_init_sub_canvas( &Screen_3d_window, &grd_curscreen->sc_canvas, x, y, w, h );
	gr_init_sub_canvas( &VR_render_sub_buffer[0], &VR_render_buffer[0], x, y, w, h );
	gr_init_sub_canvas( &VR_render_sub_buffer[1], &VR_render_buffer[1], x, y, w, h );
}


// Sets up the canvases we will be rendering to
void game_init_render_buffers(int render_w, int render_h, int render_method )
{
// 	if (!VR_offscreen_buffer)	{
	VR_render_mode		= render_method;

	if (VR_offscreen_buffer) {
		gr_free_canvas(VR_offscreen_buffer);
	}

	if ( (VR_render_mode==VR_AREA_DET) || (VR_render_mode==VR_INTERLACED ) )	{
		if ( render_h*2 < 200 )
			VR_offscreen_buffer = gr_create_canvas( render_w, 200 );
		else
			VR_offscreen_buffer = gr_create_canvas( render_w, render_h*2 );
		gr_init_sub_canvas( &VR_render_buffer[0], VR_offscreen_buffer, 0, 0, render_w, render_h );
		gr_init_sub_canvas( &VR_render_buffer[1], VR_offscreen_buffer, 0, render_h, render_w, render_h );
	} else {
		if ( render_h < 200 )
			VR_offscreen_buffer = gr_create_canvas( render_w, 200 );
		else
			VR_offscreen_buffer = gr_create_canvas( render_w, render_h );
#ifdef OGL
		VR_offscreen_buffer->cv_bitmap.bm_type=BM_OGL;
#endif
		gr_init_sub_canvas( &VR_render_buffer[0], VR_offscreen_buffer, 0, 0, render_w, render_h );
		gr_init_sub_canvas( &VR_render_buffer[1], VR_offscreen_buffer, 0, 0, render_w, render_h );
	}
	VR_offscreen_menu = VR_offscreen_buffer;

	game_init_render_sub_buffers( 0, 0, render_w, render_h );
// 	}
}

//called to change the screen mode. Parameter sm is the new mode, one of
//SMODE_GAME or SMODE_EDITOR. returns mode acutally set (could be other
//mode if cannot init requested mode)
int set_screen_mode(int sm)
{
	if ( (Screen_mode == sm) && !((sm==SCREEN_GAME) && (grd_curscreen->sc_mode != Game_screen_mode)) && !(sm==SCREEN_MENU) )
	{
		gr_set_current_canvas(NULL);
#ifndef OGL
		gr_set_draw_buffer(0);  // Set to the front buffer
#endif
		return 1;
	}

	Screen_mode = sm;

#ifdef EDITOR
	Canv_editor = NULL;
#endif

	switch( Screen_mode )	{
		case SCREEN_MENU:
			/* give control back to the WM */
			if (GameArg.CtlGrabMouse)
				SDL_WM_GrabInput(SDL_GRAB_OFF);

			if  (grd_curscreen->sc_mode != Game_screen_mode)
				if (gr_set_mode(Game_screen_mode))
					Error("Cannot set screen mode.");

			break;
		case SCREEN_GAME:
			/* keep the mouse from wandering in SDL */
			if (GameArg.CtlGrabMouse && (Newdemo_state != ND_STATE_PLAYBACK))
				SDL_WM_GrabInput(SDL_GRAB_ON);

			if  (grd_curscreen->sc_mode != Game_screen_mode)
				if (gr_set_mode(Game_screen_mode))
					Error("Cannot set screen mode.");

			reset_cockpit();
			init_cockpit();
			break;
#ifdef EDITOR
		case SCREEN_EDITOR:
			/* give control back to the WM */
			if (GameArg.CtlGrabMouse)
				SDL_WM_GrabInput(SDL_GRAB_OFF);

			if (grd_curscreen->sc_mode != SM(800,600))	{
				int gr_error;
				if ((gr_error=gr_set_mode(SM(800,600)))!=0) { //force into game scrren
					Warning("Cannot init editor screen (error=%d)",gr_error);
					return 0;
				}
			}
			gr_palette_load( gr_palette );
	
			gr_init_sub_canvas( &VR_editor_canvas, &grd_curscreen->sc_canvas, 0, 0, grd_curscreen->sc_w, grd_curscreen->sc_h );
			Canv_editor = &VR_editor_canvas;
			gr_set_current_canvas( Canv_editor );
			init_editor_screen(); //setup other editor stuff
			break;
#endif
		default:
			Error("Invalid screen mode %d",sm);
	}

	gr_set_current_canvas(NULL);

#ifndef OGL
	gr_set_draw_buffer(((Screen_mode == SCREEN_GAME) && GameArg.DbgUseDoubleBuffer) ? 1 : 0); // Double buffering or 'front' buffer only
#endif

	return 1;
}

fix frame_time_list[8] = {0,0,0,0,0,0,0,0};
fix frame_time_total=0;
int frame_time_cntr=0;

void ftoa(char *string, fix f)
{
	int decimal, fractional;

	decimal = f2i(f);
	fractional = ((f & 0xffff)*100)/65536;
	if (fractional < 0 )
		fractional *= -1;
	if (fractional > 99 ) fractional = 99;
	sprintf( string, "%d.%02d", decimal, fractional );
}

void show_framerate()
{
	char temp[50];
	fix rate;

	frame_time_total += FrameTime - frame_time_list[frame_time_cntr];
	frame_time_list[frame_time_cntr] = FrameTime;
	frame_time_cntr = (frame_time_cntr+1)%8;

	if (frame_time_total) { // prevent division by zero
		rate = fixdiv(f1_0*8,frame_time_total);

		gr_set_curfont( GAME_FONT );
		gr_set_fontcolor(gr_getcolor(0,31,0),-1 );

		ftoa( temp, rate );	// Convert fixed to string
                gr_printf(SWIDTH-FONTSCALE_X(6.5*GAME_FONT->ft_w),(SHEIGHT-FONTSCALE_Y(GAME_FONT->ft_h))/2,"FPS: %s ", temp );
	}
}

#ifdef NETWORK
void show_netplayerinfo()
{
	int x=0, y=0, i=0, color=0;
	int line_spacing=FONTSCALE_Y(GAME_FONT->ft_h+1), char_spacing=FONTSCALE_X(GAME_FONT->ft_w+1);
	char *NetworkModeNames[]={"Anarchy","Team Anarchy","Robo Anarchy","Cooperative","Capture the Flag","Hoard","Team Hoard","Unknown"};

	x=SWIDTH/2-FONTSCALE_X(123);
	y=SHEIGHT/2-FONTSCALE_Y(74);

	if (GameArg.GfxUseHiresFont && SWIDTH>=640 && SHEIGHT>=480)
	{
		x=SWIDTH/2-FONTSCALE_X(232);
		y=SHEIGHT/2-FONTSCALE_Y(138);
	}
	else
	{
		x=SWIDTH/2-FONTSCALE_X(123);
		y=SHEIGHT/2-FONTSCALE_Y(74);
	}

	Gr_scanline_darkening_level = 2*7;
	gr_setcolor( BM_XRGB(0,0,0) );
	gr_rect(x,y,SWIDTH-x,SHEIGHT-y);
	Gr_scanline_darkening_level = GR_FADE_LEVELS;
	gr_set_fontcolor(255,-1);
	gr_set_curfont(GAME_FONT);

	// general game information
	y+=line_spacing;
	gr_printf(0x8000,y,"%s by %s",Netgame.game_name,Players[network_who_is_master()].callsign);
#ifndef SHAREWARE
	y+=line_spacing;
	gr_printf(0x8000,y,"%s - lvl: %i",Netgame.mission_title,Netgame.levelnum);
#endif

	x+=char_spacing;
	y+=line_spacing*2;
	gr_printf(x,y,"game mode: %s",NetworkModeNames[Netgame.gamemode]);
	y+=line_spacing;
	gr_printf(x,y,"difficulty: %s",MENU_DIFFICULTY_TEXT(Netgame.difficulty));
	y+=line_spacing;
	gr_printf(x,y,"level time: %i:%02i:%02i",Players[Player_num].hours_level,f2i(Players[Player_num].time_level) / 60 % 60,f2i(Players[Player_num].time_level) % 60);
	y+=line_spacing;
	gr_printf(x,y,"total time: %i:%02i:%02i",Players[Player_num].hours_total,f2i(Players[Player_num].time_total) / 60 % 60,f2i(Players[Player_num].time_total) % 60);
	y+=line_spacing;

	// player information (name, kills, ping, game efficiency)
	y+=line_spacing*2;
	gr_printf(x,y,"player");
	if (Game_mode & GM_MULTI_COOP)
		gr_printf(x+char_spacing*7,y,"score");
	else
	{
		gr_printf(x+char_spacing*7,y,"kills");
		gr_printf(x+char_spacing*12,y,"deaths");
	}
	gr_printf(x+char_spacing*18,y,"ping");
	gr_printf(x+char_spacing*23,y,"efficiency");

	network_ping_all();

	// process players table
	for (i=0; i<=MAX_PLAYERS; i++)
	{
		if (!Players[i].connected)
			continue;

		y+=line_spacing;

		if (Game_mode & GM_TEAM)
			color=get_team(i);
		else
			color=i;
		gr_set_fontcolor(gr_getcolor(player_rgb[color].r,player_rgb[color].g,player_rgb[color].b),-1 );
		gr_printf(x,y,"%s\n",Players[i].callsign);
		if (Game_mode & GM_MULTI_COOP)
			gr_printf(x+char_spacing*7,y,"%-6d",Players[i].score);
		else
		{
			gr_printf(x+char_spacing*7,y,"%-6d",Players[i].net_kills_total);
			gr_printf(x+char_spacing*12,y,"%-6d",Players[i].net_killed_total);
		}

		gr_printf(x+char_spacing*18,y,"%-6d",PingTable[i]);
		gr_printf(x+char_spacing*23,y,"%d/%d",kill_matrix[Player_num][i],kill_matrix[i][Player_num]);
	}

	y+=line_spacing*2;

	// printf team scores
	if (Game_mode & GM_TEAM)
	{
		gr_set_fontcolor(255,-1);
		gr_printf(x,y,"team");
		gr_printf(x+char_spacing*8,y,"score");
		y+=line_spacing;
		gr_set_fontcolor(gr_getcolor(player_rgb[get_team(0)].r,player_rgb[get_team(0)].g,player_rgb[get_team(0)].b),-1 );
		gr_printf(x,y,"%s:",Netgame.team_name[0]);
		gr_printf(x+char_spacing*8,y,"%i",team_kills[0]);
		y+=line_spacing;
		gr_set_fontcolor(gr_getcolor(player_rgb[get_team(1)].r,player_rgb[get_team(1)].g,player_rgb[get_team(1)].b),-1 );
		gr_printf(x,y,"%s:",Netgame.team_name[1]);
		gr_printf(x+char_spacing*8,y,"%i",team_kills[1]);
		y+=line_spacing*2;
	}
}
#endif

static int timer_paused=0;

void stop_time()
{
	if (timer_paused==0) {
		fix time;
		time = timer_get_fixed_seconds();
		last_timer_value = time - last_timer_value;
		if (last_timer_value < 0) {
			last_timer_value = 0;
		}
	}
	timer_paused++;
}

void start_time()
{
	timer_paused--;
	Assert(timer_paused >= 0);
	if (timer_paused==0) {
		fix time;
		time = timer_get_fixed_seconds();
		last_timer_value = time - last_timer_value;
	}
}

void game_flush_inputs()
{
	int dx,dy,dz;
	key_flush();
	joy_flush();
	mouse_flush();
	mouse_get_delta( &dx, &dy, &dz );	// Read mouse
	memset(&Controls,0,sizeof(control_info));
}

void reset_time()
{
	last_timer_value = timer_get_fixed_seconds();
}

void calc_frame_time()
{
	fix timer_value,last_frametime = FrameTime;

	timer_value = timer_get_fixed_seconds();
	FrameTime = timer_value - last_timer_value;

	// sleep for one frame to free CPU cycles if GameArg.SysUseNiceFPS is active.
	if (GameArg.SysUseNiceFPS)
	{
		// Timer which sleeps for the time between frames but substracts 10ms timer inaccuracy.
		// CPU usage not optimized, but more reliable to get desired FPS and smooth gameplay.
		// Should probably replaced by a better solution.
		int FrameDelay = (1000/GameArg.SysMaxFPS)	// ms to pause between frames for desired FPS rate
				 - FrameTime/(F1_0/1000)	// Substract the time the game needs to do it's operations
				 - 10;				// Substract 10ms inaccuracy due to OS scheduling

		if (FrameDelay > 0)
			SDL_Delay(FrameDelay);
	}

	while (FrameTime < f1_0 / GameArg.SysMaxFPS)
	{
// 		if (GameArg.SysUseNiceFPS)
// 			timer_delay(f1_0 / GameArg.SysMaxFPS - FrameTime);
		timer_value = timer_get_fixed_seconds();
		FrameTime = timer_value - last_timer_value;
	}

#ifndef NDEBUG
	if (!(((FrameTime > 0) && (FrameTime <= F1_0)) || (Function_mode == FMODE_EDITOR) || (Newdemo_state == ND_STATE_PLAYBACK))) {
		mprintf((1,"Bad FrameTime - value = %x\n",FrameTime));
		if (FrameTime == 0)
		{
			FrameTime = 1;
		}

	}
#endif

	if ( Game_turbo_mode )
		FrameTime *= 2;

	last_timer_value = timer_value;

	if (FrameTime < 0)				//if bogus frametime...
		FrameTime = last_frametime;		//...then use time from last frame

#ifndef NDEBUG
	if (fixed_frametime) FrameTime = fixed_frametime;
#endif

#ifndef NDEBUG
	// Pause here!!!
	if ( Debug_pause )      {
		int c;
		c = 0;
		while( c==0 )
			c = key_peekkey();

		if ( c == KEY_P )       {
			Debug_pause = 0;
			c = key_inkey();
		}
		last_timer_value = timer_get_fixed_seconds();
	}
#endif
}

void move_player_2_segment(segment *seg,int side)
{
	vms_vector vp;

	compute_segment_center(&ConsoleObject->pos,seg);
	compute_center_point_on_side(&vp,seg,side);
	vm_vec_sub2(&vp,&ConsoleObject->pos);
	vm_vector_2_matrix(&ConsoleObject->orient,&vp,NULL,NULL);

	obj_relink( ConsoleObject-Objects, SEG_PTR_2_NUM(seg) );

}

fix Show_view_text_timer = -1;

#ifndef NDEBUG

void draw_window_label()
{
	if ( Show_view_text_timer > 0 )
	{
		char *viewer_name,*control_name;

		Show_view_text_timer -= FrameTime;
		gr_set_curfont( GAME_FONT );

		switch( Viewer->type )
		{
			case OBJ_FIREBALL:	viewer_name = "Fireball"; break;
			case OBJ_ROBOT:		viewer_name = "Robot"; break;
			case OBJ_HOSTAGE:	viewer_name = "Hostage"; break;
			case OBJ_PLAYER:	viewer_name = "Player"; break;
			case OBJ_WEAPON:	viewer_name = "Weapon"; break;
			case OBJ_CAMERA:	viewer_name = "Camera"; break;
			case OBJ_POWERUP:	viewer_name = "Powerup"; break;
			case OBJ_DEBRIS:	viewer_name = "Debris"; break;
			case OBJ_CNTRLCEN:	viewer_name = "Control Center"; break;
			default:		viewer_name = "Unknown"; break;
		}

		switch ( Viewer->control_type) {
			case CT_NONE:		control_name = "Stopped"; break;
			case CT_AI:		control_name = "AI"; break;
			case CT_FLYING:		control_name = "Flying"; break;
			case CT_SLEW:		control_name = "Slew"; break;
			case CT_FLYTHROUGH:	control_name = "Flythrough"; break;
			case CT_MORPH:		control_name = "Morphing"; break;
			default:		control_name = "Unknown"; break;
		}
		gr_set_fontcolor( gr_getcolor(31, 0, 0), -1 );
		gr_printf( 0x8000, 45, "%s View - %s",viewer_name,control_name );
	}
}
#endif

void render_countdown_gauge()
{
	if (!Endlevel_sequence && Fuelcen_control_center_destroyed  && (Fuelcen_seconds_left>-1) && (Fuelcen_seconds_left<127))	{
		int	y;
		gr_set_curfont( GAME_FONT );
		gr_set_fontcolor(gr_getcolor(0,63,0), -1 );
		y = FONTSCALE_Y(/*20 + */GAME_FONT->ft_h*5);
		if (!(Cockpit_mode == CM_STATUS_BAR))
			y += 5;

		if (HUD_nmessages <= 4) // do not display if it may overlap with hud messages
			gr_printf(0x8000, y, "T-%d s", Fuelcen_seconds_left );
	}
}

#ifdef NETWORK
void game_draw_multi_message()
{
	char temp_string[MAX_MULTI_MESSAGE_LEN+25];

	if ( (Game_mode&GM_MULTI) && (multi_sending_message))	{
		gr_set_curfont( GAME_FONT );    //GAME_FONT );
		gr_set_fontcolor(gr_getcolor(0,63,0), -1 );
		sprintf( temp_string, "%s: %s_", TXT_MESSAGE, Network_message );
		draw_centered_text(grd_curcanv->cv_bitmap.bm_h/2-(16*SHEIGHT/200), temp_string );
	}

	if ( (Game_mode&GM_MULTI) && (multi_defining_message))	{
		gr_set_curfont( GAME_FONT );    //GAME_FONT );
		gr_set_fontcolor(gr_getcolor(0,63,0), -1 );
		sprintf( temp_string, "%s #%d: %s_", TXT_MACRO, multi_defining_message, Network_message );
		draw_centered_text(grd_curcanv->cv_bitmap.bm_h/2-(16*SHEIGHT/200), temp_string );
	}
}
#endif

// Returns the length of the first 'n' characters of a string.
int string_width( char * s, int n )
{
	int w,h,aw;
	char p;
	p = s[n];
	s[n] = 0;
	gr_get_string_size( s, &w, &h, &aw );
	s[n] = p;
	return w;
}

// Draw string 's' centered on a canvas... if wider than
// canvas, then wrap it.
void draw_centered_text( int y, char * s )
{
	int i, l;
	char p;

	l = strlen(s);

	if ( string_width( s, l ) < grd_curcanv->cv_bitmap.bm_w ) {
		gr_string( 0x8000, y, s );
		return;
	}

	for (i=0; i<l; i++ )	{
		if ( string_width(s,i) > (grd_curcanv->cv_bitmap.bm_w - 16) ) {
			p = s[i];
			s[i] = 0;
			gr_string( 0x8000, y, s );
			s[i] = p;
			gr_string( 0x8000, y+grd_curcanv->cv_font->ft_h+1, &s[i] );
			return;
		}
	}
}

extern fix Cruise_speed;

void game_draw_hud_stuff()
{
	#ifndef NDEBUG
	if (Debug_pause) {
		gr_set_curfont( HELP_FONT );
		gr_set_fontcolor( gr_getcolor(31, 31, 31), -1 );
		gr_ustring( 0x8000, 85/2, "Debug Pause - Press P to exit" );
	}
	#endif

	#ifdef CROSSHAIR
	if ( Viewer->type == OBJ_PLAYER )
		laser_do_crosshair(Viewer);
	#endif

	#ifndef NDEBUG
	draw_window_label();
	#endif

	#ifdef NETWORK
	game_draw_multi_message();
	#endif

	if ((Newdemo_state == ND_STATE_PLAYBACK) || (Newdemo_state == ND_STATE_RECORDING)) {
		char message[128];
		int h,w,aw,y;

		if (Newdemo_state == ND_STATE_PLAYBACK) {
			if (Newdemo_vcr_state != ND_STATE_PRINTSCREEN) {
			  	sprintf(message, "%s (%d%%%% %s)", TXT_DEMO_PLAYBACK, newdemo_get_percent_done(), TXT_DONE);
			} else {
				strcpy(message, "");
			}
		} else {
			extern int Newdemo_num_written;
			sprintf (message, "%s (%dK)", TXT_DEMO_RECORDING, (Newdemo_num_written / 1024));
		}
		gr_set_curfont( GAME_FONT );    //GAME_FONT );
		gr_set_fontcolor(gr_getcolor(27,0,0), -1 );

		gr_get_string_size(message, &w, &h, &aw );

		y = grd_curcanv->cv_bitmap.bm_h;

		if (Cockpit_mode == CM_FULL_COCKPIT)
			y = grd_curcanv->cv_bitmap.bm_h / 1.15;
		if (Cockpit_mode != CM_REAR_VIEW)
			gr_printf((grd_curcanv->cv_bitmap.bm_w-w)/2,y - h - 2, message );
	}

	render_countdown_gauge();

	if ( Player_num > -1 && Viewer->type==OBJ_PLAYER && Viewer->id==Player_num )	{
		int	x = 3;
		int	y = grd_curcanv->cv_bitmap.bm_h;

		gr_set_curfont( GAME_FONT );
		gr_set_fontcolor( gr_getcolor(0, 31, 0), -1 );
		if (Cruise_speed > 0)
		{
			if (Cockpit_mode==CM_FULL_SCREEN) {
				if (Game_mode & GM_MULTI)
					y -= FONTSCALE_Y(64);
				else
					y -= FONTSCALE_Y(24);
			} else {
				if (Cockpit_mode == CM_STATUS_BAR)
				{
					if (Game_mode & GM_MULTI)
						y -= FONTSCALE_Y(48);
					else
						y -= FONTSCALE_Y(24);
				} else {
					y = FONTSCALE_Y(12);
					x = FONTSCALE_Y(20);
				}
			}
			gr_printf( x, y, "%s %2d%%", TXT_CRUISE, f2i(Cruise_speed) );
		}
	}

	if (GameArg.SysFPSIndicator)
		show_framerate();

#ifndef SHAREWARE
	if ( (Newdemo_state == ND_STATE_PLAYBACK) )
		Game_mode = Newdemo_game_mode;
#endif

	draw_hud();

#ifndef SHAREWARE
	if ( (Newdemo_state == ND_STATE_PLAYBACK) )
		Game_mode = GM_NORMAL;
#endif

	if ( Player_is_dead )
		player_dead_message();
}

extern int gr_bitblt_dest_step_shift;
extern int gr_bitblt_double;

//render a frame for the game
void game_do_render_frame(int flip)
{
	gr_set_current_canvas(&Screen_3d_window);
	
	render_frame(0);

	game_draw_hud_stuff();

	update_cockpits(0);

	if (Cockpit_mode==CM_FULL_COCKPIT || Cockpit_mode==CM_STATUS_BAR) {

#ifndef SHAREWARE
		if ( (Newdemo_state == ND_STATE_PLAYBACK) )
			Game_mode = Newdemo_game_mode;
#endif
		render_gauges();

#ifndef SHAREWARE
		if ( (Newdemo_state == ND_STATE_PLAYBACK) )
			Game_mode = GM_NORMAL;
#endif
	}

	if(show_radar && !Endlevel_sequence)
		radar_render_frame();

#ifdef NETWORK
        if (netplayerinfo_on)
		show_netplayerinfo();
#endif

	if (flip)
		gr_flip();
	else
		gr_update();
}

void game_render_frame()
{
	set_screen_mode( SCREEN_GAME );
	play_homing_warning();
        game_do_render_frame(GameArg.DbgUseDoubleBuffer);
	stop_time();
	gr_palette_fade_in( gr_palette, 32, 0 );
	start_time();
}

void do_photos();
void level_with_floor();

#ifndef OGL
void save_screen_shot(int automap_flag)
{
	fix t1;
	char message[100];
	grs_canvas *screen_canv=&grd_curscreen->sc_canvas;
	grs_font *save_font;
        static int savenum=0;
	grs_canvas *temp_canv,*save_canv;
	char savename[13+sizeof(SCRNS_DIR)];
	ubyte pal[768];
	int w,h,aw,x,y;

	// Can't do screen shots in VR modes.
	if ( VR_render_mode != VR_NONE )
		return;

	stop_time();

	if (!cfexist(SCRNS_DIR))
		mkdir(SCRNS_DIR
#ifndef __WINDOWS__
		, 0775
#endif
		); //try making directory

	save_canv = grd_curcanv;
	temp_canv = gr_create_canvas(screen_canv->cv_bitmap.bm_w,screen_canv->cv_bitmap.bm_h);
	gr_set_current_canvas(temp_canv);
	gr_ubitmap(0,0,&screen_canv->cv_bitmap);

	if ( savenum == 9999 ) savenum = 0;
		sprintf(savename,"%sscrn%04d.pcx",SCRNS_DIR,savenum++);
	
	while(!access(savename,0))
	{
		if ( savenum == 9999 ) savenum = 0;
		sprintf(savename,"%sscrn%04d.pcx",SCRNS_DIR,savenum++);
	}
	sprintf( message, "%s '%s'", TXT_DUMPING_SCREEN, savename );

	gr_set_current_canvas(NULL);
	save_font = grd_curcanv->cv_font;
	gr_set_curfont(GAME_FONT);
	gr_set_fontcolor(gr_find_closest_color_current(0,31,0),-1);
	gr_get_string_size(message,&w,&h,&aw);
	x = (grd_curcanv->cv_w-w)/2;
	y = (grd_curcanv->cv_h-h)/2;

	if (automap_flag) {
		modex_print_message(32, y, message);
	} else {
		gr_setcolor(gr_find_closest_color_current(0,0,0));
		gr_rect(x-2,y-2,x+w+2,y+h+2);
		gr_printf(x,y,message);
		gr_set_curfont(save_font);
	}
	t1 = timer_get_fixed_seconds() + F1_0;

	gr_palette_read(pal);					//get actual palette from the hardware
	pcx_write_bitmap(savename,&temp_canv->cv_bitmap,pal);

	while ( timer_get_fixed_seconds() < t1 );		// Wait so that messag stays up at least 1 second.

	gr_set_current_canvas(screen_canv);

	if (!automap_flag)
		gr_ubitmap(0,0,&temp_canv->cv_bitmap);

	gr_free_canvas(temp_canv);

	gr_set_current_canvas(save_canv);
	key_flush();
	start_time();
}
#endif //OGL

//initialize flying
void fly_init(object *obj)
{
	obj->control_type = CT_FLYING;
	obj->movement_type = MT_PHYSICS;

	vm_vec_zero(&obj->mtype.phys_info.velocity);
	vm_vec_zero(&obj->mtype.phys_info.thrust);
	vm_vec_zero(&obj->mtype.phys_info.rotvel);
	vm_vec_zero(&obj->mtype.phys_info.rotthrust);
}

int sound_nums[] = {10,11,20,21,30,31,32,33,40,41,50,51,60,61,62,70,80,81,82,83,90,91};

#define N_TEST_SOUNDS (sizeof(sound_nums) / sizeof(*sound_nums))

int test_sound_num=0;

void play_test_sound()
{
	digi_play_sample(sound_nums[test_sound_num], F1_0);
}

//	------------------------------------------------------------------------------------
void advance_sound()
{
	if (++test_sound_num == N_TEST_SOUNDS)
		test_sound_num=0;
}

void test_anim_states();

//put up the help message
void do_show_help()
{
	show_help();
}

//--unused-- int save_newdemo_state;

extern int been_in_editor;

//	------------------------------------------------------------------------------------
void do_cloak_stuff(void)
{
	int i;
	for (i = 0; i < N_players; i++)
		if (Players[i].flags & PLAYER_FLAGS_CLOAKED) {
			if (GameTime - Players[i].cloak_time > CLOAK_TIME_MAX) {
				Players[i].flags &= ~PLAYER_FLAGS_CLOAKED;
				if (i == Player_num) {
					digi_play_sample( SOUND_CLOAK_OFF, F1_0);
#ifdef NETWORK
					if (Game_mode & GM_MULTI)
						multi_send_play_sound(SOUND_CLOAK_OFF, F1_0);
					maybe_drop_net_powerup(POW_CLOAK);
					if ( Newdemo_state != ND_STATE_PLAYBACK )
						multi_send_decloak(); // For demo recording
#endif
				}
			}
		}
}

//	------------------------------------------------------------------------------------
void do_invulnerable_stuff(void)
{
	if (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE) {
		if (GameTime - Players[Player_num].invulnerable_time > INVULNERABLE_TIME_MAX) {
			Players[Player_num].flags ^= PLAYER_FLAGS_INVULNERABLE;
#ifdef NETWORK
			maybe_drop_net_powerup(POW_INVULNERABILITY);
#endif
			digi_play_sample( SOUND_INVULNERABILITY_OFF, F1_0);
#ifdef NETWORK
			if (Game_mode & GM_MULTI)
				multi_send_play_sound(SOUND_INVULNERABILITY_OFF, F1_0);
#endif
			mprintf((0, " --- You have been DE-INVULNERABLEIZED! ---\n"));
		}
	}
}


//	Amount to diminish guns towards normal, per second.
#define	DIMINISH_RATE 16 // gots to be a power of 2, else change the code in diminish_palette_towards_normal

//	------------------------------------------------------------------------------------
//	Diminish palette effects towards normal.
void diminish_palette_towards_normal(void)
{
	int	dec_amount = 0;

	if (FrameTime < F1_0/DIMINISH_RATE) {
		if (d_rand() < FrameTime*DIMINISH_RATE/2)	// Note: d_rand() is in 0..32767, and 8 Hz means decrement every frame
			dec_amount = 1;
	} else {
		dec_amount = f2i(FrameTime*DIMINISH_RATE);	// one second = DIMINISH_RATE counts
		if (dec_amount == 0)
			dec_amount++;				// make sure we decrement by something
	}

	if (PaletteRedAdd > 0 ) { PaletteRedAdd -= dec_amount; if (PaletteRedAdd < 0 ) PaletteRedAdd = 0; }
	if (PaletteRedAdd < 0 ) { PaletteRedAdd += dec_amount; if (PaletteRedAdd > 0 ) PaletteRedAdd = 0; }

	if (PaletteGreenAdd > 0 ) { PaletteGreenAdd -= dec_amount; if (PaletteGreenAdd < 0 ) PaletteGreenAdd = 0; }
	if (PaletteGreenAdd < 0 ) { PaletteGreenAdd += dec_amount; if (PaletteGreenAdd > 0 ) PaletteGreenAdd = 0; }

	if (PaletteBlueAdd > 0 ) { PaletteBlueAdd -= dec_amount; if (PaletteBlueAdd < 0 ) PaletteBlueAdd = 0; }
	if (PaletteBlueAdd < 0 ) { PaletteBlueAdd += dec_amount; if (PaletteBlueAdd > 0 ) PaletteBlueAdd = 0; }

	if ( (Newdemo_state==ND_STATE_RECORDING) && (PaletteRedAdd || PaletteGreenAdd || PaletteBlueAdd) )
		newdemo_record_palette_effect(PaletteRedAdd, PaletteGreenAdd, PaletteBlueAdd);

	gr_palette_step_up( PaletteRedAdd, PaletteGreenAdd, PaletteBlueAdd );
}

int	Redsave, Bluesave, Greensave;

void palette_save(void)
{
	Redsave = PaletteRedAdd; Bluesave = PaletteBlueAdd; Greensave = PaletteGreenAdd;
}

void palette_restore(void)
{
	PaletteRedAdd = Redsave; PaletteBlueAdd = Bluesave; PaletteGreenAdd = Greensave;
	gr_palette_step_up( PaletteRedAdd, PaletteGreenAdd, PaletteBlueAdd );
}

extern void dead_player_frame(void);

#ifndef RELEASE
void do_cheat_menu()
{
	int mmn;
	newmenu_item mm[16];
	char score_text[21];

	sprintf( score_text, "%d", Players[Player_num].score );

	mm[0].type=NM_TYPE_CHECK; mm[0].value=Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE; mm[0].text="Invulnerability";
	mm[1].type=NM_TYPE_CHECK; mm[1].value=Players[Player_num].flags & PLAYER_FLAGS_IMMATERIAL; mm[1].text="Immaterial";
	mm[2].type=NM_TYPE_CHECK; mm[2].value=0; mm[2].text="All keys";
	mm[3].type=NM_TYPE_NUMBER; mm[3].value=f2i(Players[Player_num].energy); mm[3].text="% Energy"; mm[3].min_value=0; mm[3].max_value=200;
	mm[4].type=NM_TYPE_NUMBER; mm[4].value=f2i(Players[Player_num].shields); mm[4].text="% Shields"; mm[4].min_value=0; mm[4].max_value=200;
	mm[5].type=NM_TYPE_TEXT; mm[5].text = "Score:";
	mm[6].type=NM_TYPE_INPUT; mm[6].text_len = 10; mm[6].text = score_text;
	mm[7].type=NM_TYPE_RADIO; mm[7].value=(Players[Player_num].laser_level==0); mm[7].group=0; mm[7].text="Laser level 1";
	mm[8].type=NM_TYPE_RADIO; mm[8].value=(Players[Player_num].laser_level==1); mm[8].group=0; mm[8].text="Laser level 2";
	mm[9].type=NM_TYPE_RADIO; mm[9].value=(Players[Player_num].laser_level==2); mm[9].group=0; mm[9].text="Laser level 3";
	mm[10].type=NM_TYPE_RADIO; mm[10].value=(Players[Player_num].laser_level==3); mm[10].group=0; mm[10].text="Laser level 4";
	mm[11].type=NM_TYPE_NUMBER; mm[11].value=Players[Player_num].secondary_ammo[CONCUSSION_INDEX]; mm[11].text="Missiles"; mm[11].min_value=0; mm[11].max_value=200;

	mmn = newmenu_do("Wimp Menu",NULL,12, mm, NULL );

	if (mmn > -1 )	{
		if ( mm[0].value )  {
			Players[Player_num].flags |= PLAYER_FLAGS_INVULNERABLE;
			Players[Player_num].invulnerable_time = GameTime+i2f(1000);
		} else
			Players[Player_num].flags &= ~PLAYER_FLAGS_INVULNERABLE;
		if ( mm[1].value )
			Players[Player_num].flags |= PLAYER_FLAGS_IMMATERIAL;
		else
			Players[Player_num].flags &= ~PLAYER_FLAGS_IMMATERIAL;
		if (mm[2].value) Players[Player_num].flags |= PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_GOLD_KEY;
		Players[Player_num].energy=i2f(mm[3].value);
		Players[Player_num].shields=i2f(mm[4].value);
		Players[Player_num].score = atoi(mm[6].text);
		if (mm[7].value) Players[Player_num].laser_level=0;
		if (mm[8].value) Players[Player_num].laser_level=1;
		if (mm[9].value) Players[Player_num].laser_level=2;
		if (mm[10].value) Players[Player_num].laser_level=3;
		Players[Player_num].secondary_ammo[CONCUSSION_INDEX] = mm[11].value;
		init_gauges();
	}
}
#endif

//	--------------------------------------------------------------------------------------------------
int allowed_to_fire_laser(void)
{
	if (Player_is_dead) {
		Global_missile_firing_count = 0;
		return 0;
	}

	//	Make sure enough time has elapsed to fire laser, but if it looks like it will
	//	be a long while before laser can be fired, then there must be some mistake!
	if (Next_laser_fire_time > GameTime)
		if (Next_laser_fire_time < GameTime + 2*F1_0)
			return 0;

	return 1;
}

fix	Next_flare_fire_time = 0;

int allowed_to_fire_flare(void)
{
	if (Next_flare_fire_time > GameTime)
		if (Next_flare_fire_time < GameTime + F1_0)	//	In case time is bogus, never wait > 1 second.
			return 0;

	Next_flare_fire_time = GameTime + F1_0/4;

	return 1;
}

int allowed_to_fire_missile(void)
{
	//	Make sure enough time has elapsed to fire missile, but if it looks like it will
	//	be a long while before missile can be fired, then there must be some mistake!
	if (Next_missile_fire_time > GameTime)
		if (Next_missile_fire_time < GameTime + 5*F1_0)
			return 0;

	return 1;
}

typedef struct bkg {
	short x, y, w, h;	// The location of the menu.
	grs_bitmap * bmp;	// The background under the menu.
} bkg;

bkg bg = {0,0,0,0,NULL};

//show a message in a nice little box
void show_boxed_message(char *msg)
{
	int w,h,aw;
	int x,y;

	gr_set_current_canvas(NULL);
	gr_set_curfont( HELP_FONT );

	gr_get_string_size(msg,&w,&h,&aw);

	x = (grd_curscreen->sc_w-w)/2;
	y = (grd_curscreen->sc_h-h)/2;

	nm_draw_background(x-(15*(SWIDTH/320)),y-(15*(SHEIGHT/200)),x+w+(15*(SWIDTH/320))-1,y+h+(15*(SHEIGHT/200))-1);

	gr_set_fontcolor( gr_getcolor(31, 31, 31), -1 );
	gr_ustring( (grd_curscreen->sc_w-w)/2, y, msg );
        gr_update();
#ifdef OGL
	gr_flip();
#endif
}

void clear_boxed_message()
{
	if (bg.bmp) {
		gr_bitmap(bg.x-(15*(SWIDTH/320)), bg.y-(15*(SHEIGHT/200)), bg.bmp);
		gr_free_bitmap(bg.bmp);
		bg.bmp = NULL;
		gr_update();
	}
	newmenu_close();
}

extern int Death_sequence_aborted;

//Process selected keys until game unpaused. returns key that left pause (p or esc)
int do_game_pause(int allow_menu)
{
	int paused;
	int key;

	if (Game_mode & GM_MULTI)
	{
		netplayerinfo_on= !netplayerinfo_on;
		return(KEY_PAUSE);
	}

	digi_pause_all();
	stop_time();
	palette_save();
	reset_palette_add();
	game_flush_inputs();
	paused=1;
	set_screen_mode( SCREEN_MENU );
	gr_palette_load( gr_palette );
	show_boxed_message(TXT_PAUSE);

	while (paused) {
		timer_delay(1);
#ifdef OGL
		show_boxed_message(TXT_PAUSE);
#endif
		key = key_inkey();
		
		switch (key) {
			case 0:
				break;
			case KEY_ESC:
				if (allow_menu)
					Function_mode = FMODE_MENU;
				clear_boxed_message();
				paused=0;
				break;
			case KEY_F1:
 				clear_boxed_message();
				do_show_help();
				show_boxed_message(TXT_PAUSE);
				break;
			case KEY_PRINT_SCREEN:
				save_screen_shot(0);
				break;
#ifndef RELEASE
			case KEY_BACKSP: Int3(); break;
#endif
			default:
				switch (key & 0xFF) {
					case KEY_LALT:case KEY_RALT:case KEY_TAB:case KEY_LSHIFT: case KEY_RSHIFT:
						break;//ignore (shift+)alt+tab while paused -MPM
					default:
						clear_boxed_message();
						paused=0;
						break;
				}
				break;
		}
	}

	game_flush_inputs();
	palette_restore();
	start_time();
	digi_resume_all();

	return key;
}

void show_help()
{
	newmenu_item m[25];
	int mc = 0;

	m[mc].type = NM_TYPE_TEXT; m[mc].text = TXT_HELP_ESC; mc++;
	m[mc].type = NM_TYPE_TEXT; m[mc].text = TXT_HELP_F2; mc++;
	m[mc].type = NM_TYPE_TEXT; m[mc].text = "Alt-F2/F3\t  SAVE/LOAD GAME"; mc++;
	m[mc].type = NM_TYPE_TEXT; m[mc].text = "F3\t  SWITCH COCKPIT MODES"; mc++;
	m[mc].type = NM_TYPE_TEXT; m[mc].text = "F4\t  TOGGLE RADAR"; mc++;
	m[mc].type = NM_TYPE_TEXT; m[mc].text = TXT_HELP_F5; mc++;
	m[mc].type = NM_TYPE_TEXT; m[mc].text = "ALT-F7\t  SWITCH HUD MODES"; mc++;
	m[mc].type = NM_TYPE_TEXT; m[mc].text = TXT_HELP_PAUSE; mc++;
	m[mc].type = NM_TYPE_TEXT; m[mc].text = TXT_HELP_PRTSCN; mc++;
	m[mc].type = NM_TYPE_TEXT; m[mc].text = TXT_HELP_1TO5; mc++;
	m[mc].type = NM_TYPE_TEXT; m[mc].text = TXT_HELP_6TO10; mc++;
	m[mc].type = NM_TYPE_TEXT; m[mc].text = ""; mc++;
	m[mc].type = NM_TYPE_TEXT; m[mc].text = "MULTIPLAYER:"; mc++;
	m[mc].type = NM_TYPE_TEXT; m[mc].text = "ALT-F4\t  SHOW RETICLE NAMES"; mc++;
	m[mc].type = NM_TYPE_TEXT; m[mc].text = "F7\t  SHOW KILL LIST"; mc++;
	m[mc].type = NM_TYPE_TEXT; m[mc].text = "F8\t  SEND MESSAGE"; mc++;
	m[mc].type = NM_TYPE_TEXT; m[mc].text = "F8 to F12\t  SEND MACRO"; mc++;
	m[mc].type = NM_TYPE_TEXT; m[mc].text = "SHIFT-F8 to SHIFT-F12\t  DEFINE MACRO"; mc++;
	m[mc].type = NM_TYPE_TEXT; m[mc].text = "PAUSE\t  SHOW NETGAME INFORMATION"; mc++;
	newmenu_dotiny( NULL, TXT_KEYS, mc, m, NULL );
}

//temp function until Matt cleans up game sequencing
extern void temp_reset_stuff_on_level();

//deal with rear view - switch it on, or off, or whatever
void check_rear_view()
{
	#define LEAVE_TIME 0x4000		//how long until we decide key is down	(Used to be 0x4000)

	static int leave_mode;
	static fix entry_time;

	if ( Controls.rear_view_down_count ) {	//key/button has gone down

		if (Rear_view) {
			Rear_view = 0;
			if (Cockpit_mode==CM_REAR_VIEW) {
				select_cockpit(old_cockpit_mode);
			}
			if (Newdemo_state == ND_STATE_RECORDING)
				newdemo_record_restore_rearview();
		}
		else {
			Rear_view = 1;
			leave_mode = 0;		//means wait for another key
			entry_time = timer_get_fixed_seconds();
			if (Cockpit_mode == CM_FULL_COCKPIT) {
				old_cockpit_mode = Cockpit_mode;
				select_cockpit(CM_REAR_VIEW);
			}
			if (Newdemo_state == ND_STATE_RECORDING)
				newdemo_record_rearview();
		}
	}
	else
		if (Controls.rear_view_down_state) {
			if (leave_mode==0 && (timer_get_fixed_seconds()-entry_time)>LEAVE_TIME)
				leave_mode = 1;
		}
		else
		{
			if (leave_mode==1 && Rear_view) {
				Rear_view = 0;
				if (Cockpit_mode==CM_REAR_VIEW) {
					select_cockpit(old_cockpit_mode);
				}
				if (Newdemo_state == ND_STATE_RECORDING)
					newdemo_record_restore_rearview();
			}
		}
}

void reset_rear_view(void)
{
	if (Rear_view) {
		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_restore_rearview();
	}

	Rear_view = 0;

	if (Cockpit_mode == CM_REAR_VIEW)
		select_cockpit(old_cockpit_mode);

}

int Automap_flag;
int Config_menu_flag;
jmp_buf LeaveGame;

#ifndef FINAL_CHEATS
ubyte	cheat_enable_keys[] = {KEY_G,KEY_A,KEY_B,KEY_B,KEY_A,KEY_G,KEY_A,KEY_B,KEY_B,KEY_A,KEY_H,KEY_E,KEY_Y};
#endif
sbyte	Enable_john_cheat_1, Enable_john_cheat_2, Enable_john_cheat_3, Enable_john_cheat_4;
int	cheat_enable_index;
#define CHEAT_ENABLE_LENGTH (sizeof(cheat_enable_keys) / sizeof(*cheat_enable_keys))
#ifdef FINAL_CHEATS
ubyte	cheat_enable_keys[] = {KEY_G,KEY_A,KEY_B,KEY_B,KEY_A,KEY_G,KEY_A,KEY_B,KEY_B,KEY_A,KEY_H,KEY_E,KEY_Y};
ubyte	cheat_wowie[] = {KEY_S,KEY_C,KEY_O,KEY_U,KEY_R,KEY_G,KEY_E};
ubyte	cheat_allkeys[] = {KEY_M,KEY_I,KEY_T,KEY_Z,KEY_I};
ubyte	cheat_invuln[] = {KEY_R,KEY_A,KEY_C,KEY_E,KEY_R,KEY_X};
ubyte	cheat_cloak[] = {KEY_G,KEY_U,KEY_I,KEY_L,KEY_E};
ubyte	cheat_shield[] = {KEY_T,KEY_W,KEY_I,KEY_L,KEY_I,KEY_G,KEY_H,KEY_T};
ubyte	cheat_warp[] = {KEY_F,KEY_A,KEY_R,KEY_M,KEY_E,KEY_R,KEY_J,KEY_O,KEY_E};
ubyte	cheat_astral[] = {KEY_A,KEY_S,KEY_T,KEY_R,KEY_A,KEY_L};
#define NUM_NEW_CHEATS 5

#define CHEAT_WOWIE_LENGTH (sizeof(cheat_wowie) / sizeof(*cheat_wowie))
#define CHEAT_ALLKEYS_LENGTH (sizeof(cheat_allkeys) / sizeof(*cheat_allkeys))
#define CHEAT_INVULN_LENGTH (sizeof(cheat_invuln) / sizeof(*cheat_invuln))
#define CHEAT_CLOAK_LENGTH (sizeof(cheat_cloak) / sizeof(*cheat_cloak))
#define CHEAT_SHIELD_LENGTH (sizeof(cheat_shield) / sizeof(*cheat_shield))
#define CHEAT_WARP_LENGTH (sizeof(cheat_warp) / sizeof(*cheat_warp))
#define CHEAT_ASTRAL_LENGTH (sizeof(cheat_astral) / sizeof(*cheat_astral))

#define CHEAT_TURBOMODE_OFS	0
#define CHEAT_WOWIE2_OFS	1
#define CHEAT_NEWLIFE_OFS	2
#define CHEAT_EXITPATH_OFS	3
#define CHEAT_ROBOTPAUSE_OFS	4
#define CHEAT_TURBOMODE_LENGTH	6
#define CHEAT_WOWIE2_LENGTH	6
#define CHEAT_NEWLIFE_LENGTH	5
#define CHEAT_EXITPATH_LENGTH	5
#define CHEAT_ROBOTPAUSE_LENGTH	6

int	cheat_wowie_index;
int	cheat_allkeys_index;
int	cheat_invuln_index;
int	cheat_cloak_index;
int	cheat_shield_index;
int	cheat_warp_index;
int	cheat_astral_index;
int	cheat_turbomode_index;
int	cheat_wowie2_index;
int	cheat_newlife_index;
int	cheat_exitpath_index;
int	cheat_robotpause_index;
#endif

// Frametime "cheat" code stuff

#define IMPLEMENT_CHEAT(name, action) if (key == cheat_ ## name [cheat_ ## name ## _index]) {\
			if (++cheat_ ## name ## _index == (sizeof(cheat_ ## name)/sizeof(*cheat_ ## name))) {\
				action;\
				cheat_ ## name ## _index = 0;\
			}\
		} else cheat_ ## name ## _index = 0;\

//DEFINE_CHEAT needs to be done this weird way since stupid c macros can't (portably) handle multiple args, nor can they realize that within {}'s should all be the same arg.  blah.
#define DEFINE_CHEAT(name) int cheat_ ## name ## _index;\
	ubyte cheat_ ## name []

int	Cheats_enabled=0;

extern	int Laser_rapid_fire, Ugly_robot_cheat;
extern	void do_lunacy_on(), do_lunacy_off();
extern	int Physics_cheat_flag;

void game_disable_cheats()
{
	Game_turbo_mode = 0;
	Cheats_enabled=0;
	do_lunacy_off();
	Laser_rapid_fire = 0;
	Ugly_robot_cheat = 0;
	Physics_cheat_flag = 0;
}

//	------------------------------------------------------------------------------------
//this function is the game.  called when game mode selected.  runs until
//editor mode or exit selected
void game()
{
	fix frame_rend_time=0;

	do_lunacy_on();			// Copy values for insane into copy buffer in ai.c
	do_lunacy_off();		// Restore true insane mode.
	Game_aborted = 0;
	last_drawn_cockpit = -1;	// Force cockpit to redraw next time a frame renders.
	Endlevel_sequence = 0;
	cheat_enable_index = 0;

#ifdef FINAL_CHEATS
	cheat_wowie_index = cheat_allkeys_index = cheat_invuln_index = cheat_cloak_index = cheat_shield_index = cheat_warp_index = cheat_astral_index = 0;
	cheat_turbomode_index = cheat_wowie2_index = 0;
#endif

	set_screen_mode(SCREEN_GAME);
	reset_palette_add();
	set_warn_func(game_show_warning);
	init_cockpit();
	init_gauges();
	keyd_repeat = 1; // Do allow repeat in game

#ifdef EDITOR
	if (Segments[ConsoleObject->segnum].segnum == -1)      //segment no longer exists
		obj_relink( ConsoleObject-Objects, SEG_PTR_2_NUM(Cursegp) );

	if (!check_obj_seg(ConsoleObject))
		move_player_2_segment(Cursegp,Curside);
#endif

	Viewer = ConsoleObject;
	fly_init(ConsoleObject);
	Game_suspended = 0;
	reset_time();
	FrameTime = 0;			//make first frame zero

#ifdef EDITOR
	if (Current_level_num == 0) {	//not a real level
		init_player_stats_game();
		init_ai_objects();
	}
#endif

	fix_object_segs();
	game_flush_inputs();

#ifdef NETWORK
	readbans();
#endif

	if ( setjmp(LeaveGame)==0 ) {

		while (1) {
			frame_rend_time = timer_get_fixed_seconds();

			// GAME LOOP!
			Automap_flag = 0;
			Config_menu_flag = 0;

			if ( ConsoleObject != &Objects[Players[Player_num].objnum] )
			  {
			    mprintf ((0,"Player_num=%d objnum=%d",Player_num,Players[Player_num].objnum));
			    //Assert( ConsoleObject == &Objects[Players[Player_num].objnum] );
			  }

                        GameLoop( 1, 1 );               // Do game loop with rendering and reading controls.

			if (Config_menu_flag)	{
				if (!(Game_mode&GM_MULTI)) palette_save();
				do_options_menu();
				if (!(Game_mode&GM_MULTI)) palette_restore();
			}

			if (Automap_flag) {
				do_automap(0);
				Screen_mode=-1; set_screen_mode(SCREEN_GAME);
				init_cockpit();
				last_drawn_cockpit = -1;
				game_flush_inputs();
			}

			if ( (Function_mode != FMODE_GAME) && GameArg.SysAutoDemo && (Newdemo_state != ND_STATE_NORMAL) )	{
				int choice, fmode;
				fmode = Function_mode;
				Function_mode = FMODE_GAME;
				choice=nm_messagebox( NULL, 2, TXT_YES, TXT_NO, TXT_ABORT_AUTODEMO );
				Function_mode = fmode;
				if (choice==0)	{
					GameArg.SysAutoDemo = 0;
					newdemo_stop_playback();
					Function_mode = FMODE_MENU;
				} else {
					Function_mode = FMODE_GAME;
				}
			}

			if ( (Function_mode != FMODE_GAME ) && (Newdemo_state != ND_STATE_PLAYBACK ) && (Function_mode!=FMODE_EDITOR) )		{
				int choice, fmode;
				fmode = Function_mode;
				Function_mode = FMODE_GAME;
				choice=nm_messagebox( NULL, 2, TXT_YES, TXT_NO, TXT_ABORT_GAME );
				Function_mode = fmode;
				if (choice != 0)
					Function_mode = FMODE_GAME;
			}

			if (Function_mode != FMODE_GAME)
				longjmp(LeaveGame,1);
		}
	}

#ifdef NETWORK
	vulcan_init();

	if(GameArg.MplSaveBans)
		writebans();

	restrict_mode = 0;
#endif

	show_radar = 0;
	Network_allow_radar = 0;
#ifdef NETWORK
	netplayerinfo_on=0;

	if(Game_mode & GM_MULTI)
	{
		multi_kills_stat += Players[Player_num].net_kills_total;
		multi_deaths_stat += Players[Player_num].net_killed_total;
	}
#endif
	digi_stop_all();

	if ( (Newdemo_state == ND_STATE_RECORDING) || (Newdemo_state == ND_STATE_PAUSED) )
		newdemo_stop_recording();

#ifdef NETWORK
	multi_leave_game();
#endif

	if ( Newdemo_state == ND_STATE_PLAYBACK )
 		newdemo_stop_playback();

	if (Function_mode != FMODE_EDITOR)
		gr_palette_fade_out(gr_palette,32,0);	// Fade out before going to menu

	clear_warn_func(game_show_warning);     //don't use this func anymore
	game_disable_cheats();
}

extern void john_cheat_func_1(int);
extern void john_cheat_func_2(int);
extern void john_cheat_func_3(int);
extern void john_cheat_func_4(int);

//called at the end of the program
void close_game()
{
	if (VR_offscreen_buffer)	{
		gr_free_canvas(VR_offscreen_buffer);
		VR_offscreen_buffer = NULL;
	}

	close_gauge_canvases();
	restore_effect_bitmap_icons();
	gr_free_bitmap_data (&background_bitmap);
	clear_warn_func(game_show_warning);     //don't use this func anymore
}

ubyte exploding_flag = 0;
extern void dump_used_textures_all();
int ostate_p=0;
int ostate_s=0;
#ifdef GP2X
extern int digi_volume;
#endif

void HandleDeathKey(int key)
{
	if (Player_exploded && !key_isfunc(key) && !key_ismod(key))
		Death_sequence_aborted  = 1;		//Any key but func or modifier aborts

	if (key==KEY_PRINT_SCREEN) {
//		save_screen_shot(0);
		Death_sequence_aborted  = 0;		// Clear because code above sets this for any key.
	}

	if (key == KEY_PAUSE)   {
//		key = do_game_pause();		//so esc from pause will end level
		Death_sequence_aborted  = 0;		// Clear because code above sets this for any key.
	}

	if (key == KEY_ESC) {
		if (ConsoleObject->flags & OF_EXPLODING)
			Death_sequence_aborted = 1;
	}

	if (key == KEY_BACKSP)  {
		Death_sequence_aborted  = 0;		// Clear because code above sets this for any key.
		Int3();
	}

	//don't abort death sequence for netgame join/refuse keys
	if (	(key == KEY_ALTED + KEY_1) ||
			(key == KEY_ALTED + KEY_2))
		Death_sequence_aborted  = 0;

	if (Death_sequence_aborted)
		game_flush_inputs();
}

void HandleEndlevelKey(int key)
{
	if (key==KEY_PRINT_SCREEN)
		save_screen_shot(0);

	if (key == KEY_PAUSE)
		key = do_game_pause(0);		//so esc from pause will end level

	if (key == KEY_ESC)	{
		stop_endlevel_sequence();
		last_drawn_cockpit=-1;
		return;
	}

	if (key == KEY_BACKSP)
		Int3();
}

static fix newdemo_single_frame_time;

void HandleDemoKey(int key)
{
	switch (key) {

		case KEY_DEBUGGED + KEY_I:
			Newdemo_do_interpolate = !Newdemo_do_interpolate;
			if (Newdemo_do_interpolate)
				mprintf ((0, "demo playback interpolation now on\n"));
			else
				mprintf ((0, "demo playback interpolation now off\n"));
		break;
#ifndef NDEBUG
		case KEY_DEBUGGED + KEY_K: {
			int how_many, c;
			char filename[13], num[16];
			newmenu_item m[6];

			filename[0] = '\0';
			m[ 0].type = NM_TYPE_TEXT; m[ 0].text = "output file name";
			m[ 1].type = NM_TYPE_INPUT;m[ 1].text_len = 8; m[1].text = filename;
			c = newmenu_do( NULL, NULL, 2, m, NULL );
			if (c == -2)
				break;
			strcat(filename, ".dem");
			num[0] = '\0';
			m[ 0].type = NM_TYPE_TEXT; m[ 0].text = "strip how many bytes";
			m[ 1].type = NM_TYPE_INPUT;m[ 1].text_len = 16; m[1].text = num;
			c = newmenu_do( NULL, NULL, 2, m, NULL );
			if (c == -2)
				break;
			how_many = atoi(num);
			if (how_many <= 0)
				break;
			newdemo_strip_frames(filename, how_many);
		}
		break;
#endif

		case KEY_F3:			toggle_cockpit();	break;
#ifdef GP2X
		case KEY_ALTED+KEY_F9:
			digi_set_digi_volume( digi_volume-2048 );
			break;
		case KEY_ALTED+KEY_F10:
			digi_set_digi_volume( digi_volume+2048 );
			break;
#endif
		case KEY_F2:			Config_menu_flag = 1;	break;
		case KEY_F7:
#ifdef NETWORK
			Show_kill_list = (Show_kill_list+1) % ((Newdemo_game_mode & GM_TEAM) ? 4 : 3);
#endif
		break;
		case KEY_BACKSP:
			Int3();
		break;
		case KEY_ESC:
			Function_mode = FMODE_MENU;
		break;
		case KEY_UP:
			Newdemo_vcr_state = ND_STATE_PLAYBACK;
		break;
		case KEY_DOWN:
			Newdemo_vcr_state = ND_STATE_PAUSED;
		break;
		case KEY_LEFT:
			newdemo_single_frame_time = timer_get_fixed_seconds();
			Newdemo_vcr_state = ND_STATE_ONEFRAMEBACKWARD;
		break;
		case KEY_RIGHT:
			newdemo_single_frame_time = timer_get_fixed_seconds();
			Newdemo_vcr_state = ND_STATE_ONEFRAMEFORWARD;
		break;
		case KEY_CTRLED + KEY_RIGHT:
			newdemo_goto_end();
		break;
		case KEY_CTRLED + KEY_LEFT:
			newdemo_goto_beginning();
		break;
		case KEY_PAUSE:
			do_game_pause(0);
		break;
		case KEY_PRINT_SCREEN: {
			int old_state;
			old_state = Newdemo_vcr_state;
			Newdemo_vcr_state = ND_STATE_PRINTSCREEN;
			game_do_render_frame(GameArg.DbgUseDoubleBuffer);
			save_screen_shot(0);
			Newdemo_vcr_state = old_state;
		case KEYS_GR_TOGGLE_FULLSCREEN:
			gr_toggle_fullscreen();
			break;
		break;
		}
	}
}

#ifndef FINAL_CHEATS
void FinalCheatsKey(int key);
{
	if (Cheats_enabled && !(Game_mode&GM_MULTI))
		switch (key) {
			case KEY_ALTED+KEY_1: {
				int i;

				hud_message(MSGC_GAME_CHEAT, TXT_WOWIE_ZOWIE);
#ifndef SHAREWARE
				Players[Player_num].primary_weapon_flags = 0xff;
				Players[Player_num].secondary_weapon_flags = 0xff;
#else
				Players[Player_num].primary_weapon_flags = 0xff ^ (HAS_PLASMA_FLAG | HAS_FUSION_FLAG);
				Players[Player_num].secondary_weapon_flags = 0xff ^ (HAS_SMART_FLAG | HAS_MEGA_FLAG);
#endif

				for (i=0; i<MAX_PRIMARY_WEAPONS; i++)
					Players[Player_num].primary_ammo[i] = Primary_ammo_max[i];

				for (i=0; i<MAX_SECONDARY_WEAPONS; i++)
					Players[Player_num].secondary_ammo[i] = Secondary_ammo_max[i];

				if (Newdemo_state == ND_STATE_RECORDING)
					newdemo_record_laser_level(Players[Player_num].laser_level, MAX_LASER_LEVEL);

				Players[Player_num].energy = MAX_ENERGY;
				Players[Player_num].laser_level = MAX_LASER_LEVEL;
				Players[Player_num].flags |= PLAYER_FLAGS_QUAD_LASERS;
				update_laser_weapon_info();

				break;
				}

			case KEY_ALTED+KEY_2:
				hud_message(MSGC_GAME_CHEAT, TXT_ALL_KEYS);
				Players[Player_num].flags |= PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_GOLD_KEY;
				break;

			case KEY_ALTED+KEY_3:
				Players[Player_num].flags ^= PLAYER_FLAGS_INVULNERABLE;
				hud_message(MSGC_GAME_CHEAT, "%s %s!", TXT_INVULNERABILITY, (Players[Player_num].flags&PLAYER_FLAGS_INVULNERABLE)?TXT_ON:TXT_OFF);
				Players[Player_num].invulnerable_time = GameTime+i2f(1000);
				break;

			case KEY_ALTED+KEY_4:
				Players[Player_num].flags ^= PLAYER_FLAGS_CLOAKED;
				hud_message(MSGC_GAME_CHEAT, "%s %s!", TXT_CLOAK, (Players[Player_num].flags&PLAYER_FLAGS_CLOAKED)?TXT_ON:TXT_OFF);
				if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED) {
					ai_do_cloak_stuff();
					Players[Player_num].cloak_time = GameTime;
				}
				break;

			case KEY_ALTED+KEY_5:
				hud_message(MSGC_GAME_CHEAT, TXT_FULL_SHIELDS);
				Players[Player_num].shields = MAX_SHIELDS;
				break;

			case KEY_ALTED+KEY_6: {
				newmenu_item m;
				char text[10]="";
				int new_level_num;
				int item;
				m.type=NM_TYPE_INPUT; m.text_len = 10; m.text = text;
				item = newmenu_do( NULL, TXT_WARP_TO_LEVEL, 1, &m, NULL );
				if (item != -1) {
					new_level_num = atoi(m.text);
					if (new_level_num!=0 && new_level_num>=0 && new_level_num<=LAST_LEVEL)
						StartNewLevel(new_level_num);
				}
				break;
			}
		}
}
#endif

void HandleGameKey(int key)
{
	switch (key) {
		//================================================================================================
		//FIRST ARE ALL THE REAL GAME KEYS.  PUT TEST AND DEBUG KEYS LATER.

		case KEY_ESC:
			if (!Player_is_dead) {
				Game_aborted=1;
				Function_mode = FMODE_MENU;
			}
			break;
		case KEY_F1:				do_show_help();         break;
		case KEY_F2:				Config_menu_flag = 1;	break;
		case KEY_F3:				toggle_cockpit();       break;
		case KEY_F4:				if(!(Game_mode & GM_MULTI)||Network_allow_radar)
								show_radar = !show_radar; break;
		case KEY_ALTED + KEY_F4:
			Show_reticle_name = (Show_reticle_name+1)%2;
			break;
		case KEY_F5:
				if ( Newdemo_state == ND_STATE_RECORDING )
					newdemo_stop_recording();
				else if ( Newdemo_state == ND_STATE_NORMAL )
					newdemo_start_recording();
				break;
#ifdef NETWORK
		case KEY_F6:
			if (restrict_mode && WaitForRefuseAnswer)
			{
				RefuseThisPlayer=1;
				HUD_init_message ("Player accepted!");
			}
			break;
		case KEY_F7:
			Show_kill_list = (Show_kill_list+1) % ((Game_mode & GM_TEAM) ? 4 : 3);
			break;
#endif
		case KEY_ALTED+KEY_F7:
			GameArg.GfxGaugeHudMode=(GameArg.GfxGaugeHudMode+1)%GAUGE_HUD_NUMMODES;
			break;
#ifdef NETWORK
		case KEY_F8:
			multi_send_message_start();
			break;
	
		case KEY_SHIFTED+KEY_F8:
			mekh_resend_last();
			break;

		case KEY_F9:
		case KEY_F10:
		case KEY_F11:
		case KEY_F12:
			multi_send_macro(key);
			break;		// send taunt macros
		case KEY_SHIFTED + KEY_F9:
		case KEY_SHIFTED + KEY_F10:
		case KEY_SHIFTED + KEY_F11:
		case KEY_SHIFTED + KEY_F12:
			multi_define_macro(key);
			break;		// redefine taunt macros
#endif
		case KEY_PAUSE:			do_game_pause(1); 	break;
		case KEY_PRINT_SCREEN: 		save_screen_shot(0);	break;

#ifdef GP2X
		case KEY_ALTED+KEY_F9:
			digi_set_digi_volume( digi_volume-2048 );
			break;
		case KEY_ALTED+KEY_F10:
			digi_set_digi_volume( digi_volume+2048 );
			break;
#endif
		case KEYS_GR_TOGGLE_FULLSCREEN:
				gr_toggle_fullscreen();
				break;
		case KEY_SHIFTED + KEY_ESC: //quick exit
#ifdef EDITOR
			if (! SafetyCheck()) break;
			close_editor_screen();
#endif
			Game_aborted=1;
			Function_mode=FMODE_EXIT;
			break;
#ifdef GP2X
		case KEY_ALTED + KEY_ESC:
#endif
		case KEY_ALTED+KEY_F2:	if (!Player_is_dead) state_save_all( 0 );		break;	// 0 means not between levels.
		case KEY_ALTED+KEY_F3:	if (!Player_is_dead) state_restore_all(1);		break;

#ifdef USE_SDLMIXER
		/*
		 * Jukebox hotkeys -- MD2211, 2007
		 * ==============================================
		 */
		case KEY_ALTED + KEY_SHIFTED + KEY_F9:
			jukebox_play();
			break;
		case KEY_ALTED + KEY_SHIFTED + KEY_F10:
			jukebox_stop();
			break;
		case KEY_ALTED + KEY_SHIFTED + KEY_F11:
			jukebox_prev();
			break;
		case KEY_ALTED + KEY_SHIFTED + KEY_F12:
			jukebox_next();
			break;
#endif

		//use function keys for window sizing

		//	================================================================================================
		//ALL KEYS BELOW HERE GO AWAY IN RELEASE VERSION

#ifndef NDEBUG
#ifndef RELEASE
		case KEY_DEBUGGED+KEY_0:	show_weapon_status();	break;
#ifdef SHOW_EXIT_PATH
		case KEY_DEBUGGED+KEY_1:	create_special_path();	break;
#endif
		case KEY_DEBUGGED+KEY_Y:
			do_controlcen_destroyed_stuff(NULL);
			break;
		case KEY_BACKSP:
		case KEY_CTRLED+KEY_BACKSP:
		case KEY_SHIFTED+KEY_BACKSP:
		case KEY_SHIFTED+KEY_ALTED+KEY_BACKSP:
		case KEY_CTRLED+KEY_ALTED+KEY_BACKSP:
		case KEY_SHIFTED+KEY_CTRLED+KEY_BACKSP:
		case KEY_SHIFTED+KEY_CTRLED+KEY_ALTED+KEY_BACKSP:
				Int3(); break;
		case KEY_DEBUGGED+KEY_S:
				digi_reset(); break;
		case KEY_DEBUGGED+KEY_P:
			if (Game_suspended & SUSP_ROBOTS)
				Game_suspended &= ~SUSP_ROBOTS;         //robots move
			else
				Game_suspended |= SUSP_ROBOTS;          //robots don't move
			break;
		case KEY_DEBUGGED+KEY_K:
			Players[Player_num].shields = 1;
			break; // a virtual kill
		case KEY_DEBUGGED+KEY_SHIFTED + KEY_K:
			Players[Player_num].shields = -1;
			break; // an actual kill
		case KEY_DEBUGGED+KEY_X:
			Players[Player_num].lives++;
			break; // Extra life cheat key.
		case KEY_DEBUGGED+KEY_H:
			Players[Player_num].flags ^= PLAYER_FLAGS_CLOAKED;
			if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED) {
#ifdef NETWORK
				if (Game_mode & GM_MULTI)
					multi_send_cloak();
#endif
				ai_do_cloak_stuff();
				Players[Player_num].cloak_time = GameTime;
				mprintf((0, "You are cloaked!\n"));
			} else
				mprintf((0, "You are DE-cloaked!\n"));
			break;
		case KEY_DEBUGGED+KEY_R:
			Robot_firing_enabled = !Robot_firing_enabled;
			break;

#ifdef EDITOR //editor-specific functions
		case KEY_E + KEY_DEBUGGED:
			network_leave_game();
			Function_mode = FMODE_EDITOR;
			break;
		case KEY_C + KEY_SHIFTED + KEY_DEBUGGED:
			if (!( Game_mode & GM_MULTI ))
				move_player_2_segment(Cursegp,Curside);
			break; //move eye to curseg
		case KEY_DEBUGGED+KEY_W:
			draw_world_from_game();
			break;
#endif	//#ifdef EDITOR
		case KEY_DEBUGGED+KEY_LAPOSTRO: Show_view_text_timer = 0x30000; object_goto_next_viewer(); break;
		case KEY_DEBUGGED+KEY_SHIFTED+KEY_LAPOSTRO: Viewer=ConsoleObject; break;
#ifndef NDEBUG
		case KEY_DEBUGGED+KEY_O: toggle_outline_mode(); break;
#endif
		case KEY_DEBUGGED+KEY_T:
			*Toggle_var = !*Toggle_var;
			mprintf((0, "Variable at %08x set to %i\n", Toggle_var, *Toggle_var));
			break;
		case KEY_DEBUGGED + KEY_L:
			if (++Lighting_on >= 2) Lighting_on = 0; break;
		case KEY_DEBUGGED + KEY_SHIFTED + KEY_L:
			Beam_brightness=0x38000-Beam_brightness; break;
		case KEY_PAD5: slew_stop(); break;
		case KEY_DEBUGGED + KEY_F11: play_test_sound(); break;
		case KEY_DEBUGGED + KEY_SHIFTED+KEY_F11: advance_sound(); play_test_sound(); break;
		case KEY_DEBUGGED + KEY_M:
			Debug_spew = !Debug_spew;
			if (Debug_spew) {
				mopen( 0, 8, 1, 78, 16, "Debug Spew");
				hud_message( MSGC_GAME_FEEDBACK, "Debug Spew: ON" );
			} else {
				mclose( 0 );
				hud_message( MSGC_GAME_FEEDBACK, "Debug Spew: OFF" );
			}
			break;
		case KEY_DEBUGGED + KEY_C:
			do_cheat_menu();
			break;
		case KEY_DEBUGGED + KEY_SHIFTED + KEY_A:
			do_megawow_powerup(10);
			break;
		case KEY_DEBUGGED + KEY_A:	{
			do_megawow_powerup(200);
			break;
		}
		case KEY_DEBUGGED+KEY_F:
			GameArg.SysFPSIndicator = !GameArg.SysFPSIndicator;
			break;
		case KEY_DEBUGGED+KEY_SPACEBAR: // Toggle physics flying
			slew_stop();
			game_flush_inputs();
			if ( ConsoleObject->control_type != CT_FLYING ) {
				fly_init(ConsoleObject);
				Game_suspended &= ~SUSP_ROBOTS;         //robots move
			} else {
				slew_init(ConsoleObject);                                              //start player slewing
				Game_suspended |= SUSP_ROBOTS;          //robots don't move
			}
			break;
		case KEY_DEBUGGED+KEY_COMMA: Render_zoom = fixmul(Render_zoom,62259); break;
		case KEY_DEBUGGED+KEY_PERIOD: Render_zoom = fixmul(Render_zoom,68985); break;
		case KEY_DEBUGGED+KEY_P+KEY_SHIFTED: Debug_pause = 1; break;
#ifndef NDEBUG
		case KEY_DEBUGGED+KEY_F8: speedtest_init(); Speedtest_count = 1;	break;
		case KEY_DEBUGGED+KEY_F9: speedtest_init(); Speedtest_count = 10;	break;
		case KEY_DEBUGGED+KEY_D:
			if ((GameArg.DbgUseDoubleBuffer = !GameArg.DbgUseDoubleBuffer)!=0)
				init_cockpit();
			break;
#endif
#ifdef EDITOR
		case KEY_DEBUGGED+KEY_Q:
			stop_time();
			dump_used_textures_all();
			start_time();
			break;
#endif
#endif //#ifndef RELEASE
#endif
		default: break;
	}
}

void do_weapon_stuff()
{
	if (Controls.fire_flare_down_count)
		if (allowed_to_fire_flare())
			Flare_create(ConsoleObject);

	if (allowed_to_fire_missile())
		Global_missile_firing_count += Weapon_info[Secondary_weapon_to_weapon_info[Secondary_weapon]].fire_count * (Controls.fire_secondary_state || Controls.fire_secondary_down_count);

	if (Global_missile_firing_count) {
		do_missile_firing();
		Global_missile_firing_count--;
	}

	if (Global_missile_firing_count < 0)
		Global_missile_firing_count = 0;

	//	Drop proximity bombs.
	if (Controls.drop_bomb_down_count) {
		//changed on 9/16/98 by adb to distinguish between drop bomb and secondary fire
		while (Controls.drop_bomb_down_count--)
			do_drop_bomb();
		//end changes - adb
	}

	if (Controls.cycle_primary_down_count||Controls.cycle_primary_state!=ostate_p)
	{
		if((Controls.cycle_primary_state!=ostate_p)&&(Controls.cycle_primary_state==0))
		{
			ostate_p=Controls.cycle_primary_state;
		}
		else      //if(ostate_p!=Controls.cycle_primary_state)
		{
			int next_weapon;
			int weap_val=0;
			int i=0;

			if(LaserPowSelected&&Primary_weapon==0)
				next_weapon=LaserPowSelected;
			else
				next_weapon=Primary_weapon;
			weap_val=primary_order[next_weapon];
			if(highest_primary > 0)
			{
				do
				{
					if(highest_primary==1&&primary_order[next_weapon]==1)
					{
					maybe_select_primary(next_weapon);
					break;
					}
					weap_val--;
					if(weap_val < 1)
					weap_val=highest_primary;
					do
					{
					next_weapon++;
					if(next_weapon >= MAX_PRIMARY_WEAPONS + NEWPRIMS)
					next_weapon = 0;
					} while(primary_order[next_weapon]!=weap_val);
					i++;
				} while(player_has_weapon(next_weapon,0)!=7&&i<highest_primary);
			}
			if((next_weapon!=Primary_weapon)&&(player_has_weapon(next_weapon,0)==7))
				do_weapon_select(next_weapon,0);
			Controls.cycle_primary_down_count = 0;
			ostate_p=Controls.cycle_primary_state;
		}
	}
	if (Controls.cycle_secondary_down_count||Controls.cycle_secondary_state!=ostate_s)
	{
		if((Controls.cycle_secondary_state!=ostate_s)&&(Controls.cycle_secondary_state==0))
		{
			ostate_s=Controls.cycle_secondary_state;
		}
		else
		{
			int next_weapon;
			int weap_val=0;
			int i=0;

			next_weapon=Secondary_weapon;
			weap_val=secondary_order[next_weapon];

			if(highest_secondary > 0)
			{
				do
				{
					if(highest_secondary==1&&secondary_order[next_weapon]==1)
					{
						maybe_select_secondary(next_weapon);
						break;
					}
					weap_val--;
					if(weap_val < 1)
						weap_val=highest_secondary;
					do
					{
						next_weapon++;
						if(next_weapon >= 5)
							next_weapon = 0;
					} while(secondary_order[next_weapon]!=weap_val);
					i++;
				} while(player_has_weapon(next_weapon,1)!=7&&i<highest_secondary);
			}
			if(next_weapon!=Secondary_weapon)
				do_weapon_select(next_weapon,1);
			Controls.cycle_secondary_down_count = 0;
			ostate_s=Controls.cycle_secondary_state;
		}
	}
}

void ReadControls()
{
	int key;
	fix key_time;

		Player_fired_laser_this_frame=-1;

#ifndef NDEBUG
		if (Speedtest_on)
			speedtest_frame();
#endif

		if (!Endlevel_sequence) {  // && !Player_is_dead  //this was taken out of the if statement by WraithX

				if ( (Newdemo_state == ND_STATE_PLAYBACK)
					#ifdef NETWORK
					|| multi_sending_message || multi_defining_message
					#endif
					) 	// WATCH OUT!!! WEIRD CODE ABOVE!!!
					memset( &Controls, 0, sizeof(control_info) );
				else
					controls_read_all();		//NOTE LINK TO ABOVE!!!

			check_rear_view();

			// If automap key pressed, enable automap unless you are in network mode, control center destroyed and < 10 seconds left
			if ( Controls.automap_down_count && !((Game_mode & GM_MULTI) && Fuelcen_control_center_destroyed && (Fuelcen_seconds_left < 10)))
				Automap_flag = 1;

			do_weapon_stuff();
                }

	if (Player_exploded) {

		if (exploding_flag==0)  {
			exploding_flag = 1;			// When player starts exploding, clear all input devices...
			game_flush_inputs();
		} else {
			int i;

			for (i=0; i<4; i++ )
				if (isJoyRotationKey(i) != 1)
				{
					if (joy_get_button_down_cnt(i)>0) Death_sequence_aborted = 1;
				}

			for (i = 0; i < 3; i++)
				// the following "if" added by WraithX, 4/17/00
				if (isMouseRotationKey(i) != 1)
				{
					if (mouse_button_down_count(i) > 0)
						Death_sequence_aborted = 1;
				}// end "if" added by WraithX

			if (Death_sequence_aborted)
				game_flush_inputs();
		}
	} else {
		exploding_flag=0;
	}

		if (Newdemo_state == ND_STATE_PLAYBACK )	{
			if ((keyd_pressed[KEY_LSHIFT] || keyd_pressed[KEY_RSHIFT]) && keyd_pressed[KEY_RIGHT])
				Newdemo_vcr_state = ND_STATE_FASTFORWARD;
			else if ((keyd_pressed[KEY_LSHIFT] || keyd_pressed[KEY_RSHIFT]) && keyd_pressed[KEY_LEFT])
				Newdemo_vcr_state = ND_STATE_REWINDING;
			else if (!(keyd_pressed[KEY_LCTRL] || keyd_pressed[KEY_RCTRL]) && keyd_pressed[KEY_RIGHT] && ((timer_get_fixed_seconds() - newdemo_single_frame_time) >= F1_0))
				Newdemo_vcr_state = ND_STATE_ONEFRAMEFORWARD;
			else if (!(keyd_pressed[KEY_LCTRL] || keyd_pressed[KEY_RCTRL]) && keyd_pressed[KEY_LEFT] && ((timer_get_fixed_seconds() - newdemo_single_frame_time) >= F1_0))
				Newdemo_vcr_state = ND_STATE_ONEFRAMEBACKWARD;
			else if ((Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_REWINDING))
				Newdemo_vcr_state = ND_STATE_PLAYBACK;
		}

		while ((key=key_inkey_time(&key_time)) != 0)	{

			john_cheat_func_1(key);

			#ifdef NETWORK
			if ( (Game_mode&GM_MULTI) && (multi_sending_message || multi_defining_message ))	{
				multi_message_input_sub( key );
				continue;		//get next key
			}
			#endif

			if (!(Game_mode&GM_MULTI) && key == cheat_enable_keys[cheat_enable_index]) {
				if (++cheat_enable_index == CHEAT_ENABLE_LENGTH) {
					hud_message(MSGC_GAME_CHEAT, TXT_CHEATS_ENABLED);
					digi_play_sample( SOUND_CHEATER, F1_0);
					Cheats_enabled = 1;
					Players[Player_num].score = 0;
				}
			}
			else
				cheat_enable_index = 0;

			john_cheat_func_2(key);

#ifdef FINAL_CHEATS
		if (Cheats_enabled) {
			if (!(Game_mode&GM_MULTI) && key == cheat_wowie[cheat_wowie_index]) {
				if (++cheat_wowie_index == CHEAT_WOWIE_LENGTH) {
					int i;

					hud_message(MSGC_GAME_CHEAT, TXT_WOWIE_ZOWIE);
					digi_play_sample( SOUND_CHEATER, F1_0);

					Players[Player_num].primary_weapon_flags |= 0xff ^ (HAS_PLASMA_FLAG | HAS_FUSION_FLAG);
					Players[Player_num].secondary_weapon_flags |= 0xff ^ (HAS_SMART_FLAG | HAS_MEGA_FLAG);

					for (i=0; i<3; i++)
						Players[Player_num].primary_ammo[i] = Primary_ammo_max[i];

					for (i=0; i<3; i++)
						Players[Player_num].secondary_ammo[i] = Secondary_ammo_max[i];

					if (Newdemo_state == ND_STATE_RECORDING)
						newdemo_record_laser_level(Players[Player_num].laser_level, MAX_LASER_LEVEL);

					Players[Player_num].energy = MAX_ENERGY;
					Players[Player_num].laser_level = MAX_LASER_LEVEL;
					Players[Player_num].flags |= PLAYER_FLAGS_QUAD_LASERS;
					update_laser_weapon_info();

					cheat_wowie_index = 0;
				}
			}
			else
				cheat_wowie_index = 0;

			if (!(Game_mode&GM_MULTI) && key == (0xaa^new_cheats[cheat_wowie2_index*NUM_NEW_CHEATS+CHEAT_WOWIE2_OFS])) {
				if (++cheat_wowie2_index == CHEAT_WOWIE2_LENGTH) {
					int i;

					hud_message(MSGC_GAME_CHEAT, "SUPER %s",TXT_WOWIE_ZOWIE);
					digi_play_sample( SOUND_CHEATER, F1_0);

					Players[Player_num].primary_weapon_flags = 0xff;
					Players[Player_num].secondary_weapon_flags = 0xff;

					for (i=0; i<MAX_PRIMARY_WEAPONS; i++)
						Players[Player_num].primary_ammo[i] = Primary_ammo_max[i];

					for (i=0; i<MAX_SECONDARY_WEAPONS; i++)
						Players[Player_num].secondary_ammo[i] = Secondary_ammo_max[i];

					if (Newdemo_state == ND_STATE_RECORDING)
						newdemo_record_laser_level(Players[Player_num].laser_level, MAX_LASER_LEVEL);

					Players[Player_num].energy = MAX_ENERGY;
					Players[Player_num].laser_level = MAX_LASER_LEVEL;
					Players[Player_num].flags |= PLAYER_FLAGS_QUAD_LASERS;
					update_laser_weapon_info();

					cheat_wowie2_index = 0;
				}
			}
			else
				cheat_wowie2_index = 0;

			if (!(Game_mode&GM_MULTI) && key == cheat_allkeys[cheat_allkeys_index]) {
				if (++cheat_allkeys_index == CHEAT_ALLKEYS_LENGTH) {
					hud_message(MSGC_GAME_CHEAT, TXT_ALL_KEYS);
					digi_play_sample( SOUND_CHEATER, F1_0);
					Players[Player_num].flags |= PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_GOLD_KEY;

					cheat_allkeys_index = 0;
				}
			}
			else
				cheat_allkeys_index = 0;


			if (!(Game_mode&GM_MULTI) && key == cheat_invuln[cheat_invuln_index]) {
				if (++cheat_invuln_index == CHEAT_INVULN_LENGTH) {
					Players[Player_num].flags ^= PLAYER_FLAGS_INVULNERABLE;
					hud_message(MSGC_GAME_CHEAT, "%s %s!", TXT_INVULNERABILITY, (Players[Player_num].flags&PLAYER_FLAGS_INVULNERABLE)?TXT_ON:TXT_OFF);
					digi_play_sample( SOUND_CHEATER, F1_0);
					Players[Player_num].invulnerable_time = GameTime+i2f(1000);

					cheat_invuln_index = 0;
				}
			}
			else
				cheat_invuln_index = 0;

			if (!(Game_mode&GM_MULTI) && key == cheat_cloak[cheat_cloak_index]) {
				if (++cheat_cloak_index == CHEAT_CLOAK_LENGTH) {
					Players[Player_num].flags ^= PLAYER_FLAGS_CLOAKED;
					hud_message(MSGC_GAME_CHEAT, "%s %s!", TXT_CLOAK, (Players[Player_num].flags&PLAYER_FLAGS_CLOAKED)?TXT_ON:TXT_OFF);
					digi_play_sample( SOUND_CHEATER, F1_0);
					if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED) {
						ai_do_cloak_stuff();
						Players[Player_num].cloak_time = GameTime;
					}

					cheat_cloak_index = 0;
				}
			}
			else
				cheat_cloak_index = 0;

			if (!(Game_mode&GM_MULTI) && key == cheat_shield[cheat_shield_index]) {
				if (++cheat_shield_index == CHEAT_SHIELD_LENGTH) {
					hud_message(MSGC_GAME_CHEAT, TXT_FULL_SHIELDS);
					digi_play_sample( SOUND_CHEATER, F1_0);
					Players[Player_num].shields = MAX_SHIELDS;

					cheat_shield_index = 0;
				}
			}
			else
				cheat_shield_index = 0;

			if (!(Game_mode&GM_MULTI) && key == cheat_warp[cheat_warp_index]) {
				if (++cheat_warp_index == CHEAT_WARP_LENGTH) {
					newmenu_item m;
					char text[10]="";
					int new_level_num;
					int item;
					digi_play_sample( SOUND_CHEATER, F1_0);
					m.type=NM_TYPE_INPUT; m.text_len = 10; m.text = text;
					item = newmenu_do( NULL, TXT_WARP_TO_LEVEL, 1, &m, NULL );
					if (item != -1) {
						new_level_num = atoi(m.text);
						if (new_level_num!=0 && new_level_num>=0 && new_level_num<=Last_level)
							StartNewLevel(new_level_num);
					}

					cheat_warp_index = 0;
				}
			}
			else
				cheat_warp_index = 0;

			if (!(Game_mode&GM_MULTI) && key == cheat_astral[cheat_astral_index]) {
				if (++cheat_astral_index == CHEAT_ASTRAL_LENGTH) {
					digi_play_sample( SOUND_CHEATER, F1_0);
					if ( Physics_cheat_flag==0xBADA55 )	{
						Physics_cheat_flag = 0;
					} else {
						Physics_cheat_flag = 0xBADA55;
					}
					hud_message(MSGC_GAME_CHEAT, "%s %s!", "Ghosty mode", Physics_cheat_flag==0xBADA55?TXT_ON:TXT_OFF);
					cheat_astral_index = 0;
				}
			}
			else
				cheat_astral_index = 0;

			if (!(Game_mode&GM_MULTI) && key == (0xaa^new_cheats[cheat_turbomode_index*NUM_NEW_CHEATS+CHEAT_TURBOMODE_OFS])) {
				if (++cheat_turbomode_index == CHEAT_TURBOMODE_LENGTH) {
					Game_turbo_mode ^= 1;
					hud_message(MSGC_GAME_CHEAT, "%s %s!", "Turbo mode", Game_turbo_mode?TXT_ON:TXT_OFF);
					digi_play_sample( SOUND_CHEATER, F1_0);
				}
			}
			else
				cheat_turbomode_index = 0;

			if (!(Game_mode&GM_MULTI) && key == (0xaa^new_cheats[cheat_newlife_index*NUM_NEW_CHEATS+CHEAT_NEWLIFE_OFS])) {
				if (++cheat_newlife_index == CHEAT_NEWLIFE_LENGTH) {
					if (Players[Player_num].lives<50) {
						Players[Player_num].lives++;
						hud_message(MSGC_GAME_CHEAT, "Extra life!");
						digi_play_sample( SOUND_CHEATER, F1_0);
					}

					cheat_newlife_index = 0;
				}
			}
			else
				cheat_newlife_index = 0;

			if (!(Game_mode&GM_MULTI) && key == (0xaa^new_cheats[cheat_exitpath_index*NUM_NEW_CHEATS+CHEAT_EXITPATH_OFS])) {
				if (++cheat_exitpath_index == CHEAT_EXITPATH_LENGTH) {
#ifdef SHOW_EXIT_PATH
					if (create_special_path()) {
						hud_message(MSGC_GAME_CHEAT, "Exit path illuminated!");
						digi_play_sample( SOUND_CHEATER, F1_0);
					}
#endif
					cheat_exitpath_index = 0;
				}
			}
			else
				cheat_exitpath_index = 0;

			if (!(Game_mode&GM_MULTI) && key == (0xaa^new_cheats[cheat_robotpause_index*NUM_NEW_CHEATS+CHEAT_ROBOTPAUSE_OFS])) {
				if (++cheat_robotpause_index == CHEAT_ROBOTPAUSE_LENGTH) {
					Robot_firing_enabled = !Robot_firing_enabled;
					hud_message(MSGC_GAME_CHEAT, "%s %s!", "Robot firing", Robot_firing_enabled?TXT_ON:TXT_OFF);
					digi_play_sample( SOUND_CHEATER, F1_0);

					cheat_robotpause_index = 0;
				}

			}
			else
				cheat_robotpause_index = 0;
		}
#endif

			john_cheat_func_3(key);

			#ifndef RELEASE
			#ifdef NETWORK
			#define I_AM_NOT_A_CHEATER_BUT_MY_KEYBOARD_THINKS_I_PRESS_DEL // adb: sorry, but it's true
			#ifndef I_AM_NOT_A_CHEATER_BUT_MY_KEYBOARD_THINKS_I_PRESS_DEL
			if ((key&KEY_DEBUGGED)&&(Game_mode&GM_MULTI))	{
				Network_message_reciever = 100;		// Send to everyone...
				snprintf( Network_message, MAX_MESSAGE_LEN, "%s %s", TXT_I_AM_A, TXT_CHEATER);
			}
			#endif
			#endif
			#endif

			john_cheat_func_4(key);

			if (Player_is_dead)
				HandleDeathKey(key);

			if (Endlevel_sequence)
				HandleEndlevelKey(key);
			else if (Newdemo_state == ND_STATE_PLAYBACK )
				HandleDemoKey(key);
			else
			{
#ifndef FINAL_CHEATS
				FinalCheatsKey(key);
#endif
				HandleGameKey(key);
			}
		}
}

#ifndef	NDEBUG
int	Debug_slowdown=0;
#endif

#ifdef EDITOR
extern	void player_follow_path(object *objp);
extern	void check_create_player_path(void);
#endif
extern	int Do_appearance_effect;

void GameLoop(int RenderFlag, int ReadControlsFlag )
{
	static int desc_dead_countdown=100;   /*  used if player shouldn't be playing */

#ifndef	NDEBUG
	if (Debug_slowdown) {
		int h, i, j=0;
		for (h=0; h<Debug_slowdown; h++)
			for (i=0; i<1000; i++)
				j += i;
	}
#endif

		if (desc_id_exit_num) {			// are we supposed to be checking
			if (!(--desc_dead_countdown))	// if so, at zero, then pull the plug
				Error ("Loading overlay -- error number: %d\n", (int)desc_id_exit_num);
		}

		update_player_stats();
		diminish_palette_towards_normal();		//	Should leave palette effect up for as long as possible by putting right before render.
		do_cloak_stuff();
		do_invulnerable_stuff();
		remove_obsolete_stuck_objects();
#ifdef EDITOR
		check_create_player_path();
		player_follow_path(ConsoleObject);
#endif
#ifdef NETWORK
		if (Game_mode & GM_MULTI)
			multi_do_frame();
#endif

		if (RenderFlag) {
			if (force_cockpit_redraw) {    //screen need redrawing?
				init_cockpit();
				force_cockpit_redraw=0;
			}
			game_render_frame();
		}

//		mprintf(0,"Velocity %2.2f\n", f2fl(vm_vec_mag(&ConsoleObject->phys_info.velocity)));

		calc_frame_time();

		dead_player_frame();
		if (Newdemo_state != ND_STATE_PLAYBACK)
			do_controlcen_dead_frame();

		if (ReadControlsFlag)
			ReadControls();
		else
			memset(&Controls, 0, sizeof(Controls));

		GameTime += FrameTime;

		if (GameTime < 0 || GameTime > i2f(0x7fff - 600)) {
			GameTime = FrameTime;	//wrap when goes negative, or gets within 10 minutes
			mprintf((0,"GameTime reset to 0\n"));
		}

		digi_sync_sounds();

		if (Endlevel_sequence) {
			do_endlevel_frame();
			powerup_grab_cheat_all();
			do_special_effects();
			return; //skip everything else
		}

		if (Newdemo_state != ND_STATE_PLAYBACK)
			do_exploding_wall_frame();
		if ((Newdemo_state != ND_STATE_PLAYBACK) || (Newdemo_vcr_state != ND_STATE_PAUSED)) {
			do_special_effects();
			wall_frame_process();
			triggers_frame_process();
		}

		if (Fuelcen_control_center_destroyed) {
                        if (Newdemo_state==ND_STATE_RECORDING )
				newdemo_record_control_center_destroyed();
			flash_frame();
		}

		if ( Newdemo_state == ND_STATE_PLAYBACK ) {
			newdemo_playback_one_frame();
			if ( Newdemo_state != ND_STATE_PLAYBACK ) 	{
				longjmp( LeaveGame, 1 ); // Go back to menu
			}
		}
		else
		{ // Note the link to above!

			Players[Player_num].homing_object_dist = -1; // Assume not being tracked.  Laser_do_weapon_sequence modifies this.
                        object_move_all();
			powerup_grab_cheat_all();

			if (Endlevel_sequence)	//might have been started during move
				return;

			fuelcen_update_all();
			do_ai_frame_all();

			if (allowed_to_fire_laser()) {
				Global_laser_firing_count = Weapon_info[Primary_weapon_to_weapon_info[Primary_weapon]].fire_count * (Controls.fire_primary_state || Controls.fire_primary_down_count);
				if ((Primary_weapon == FUSION_INDEX) && (Global_laser_firing_count)) {
					if ((Players[Player_num].energy < F1_0*2) && (Auto_fire_fusion_cannon_time == 0)) {
						Global_laser_firing_count = 0;
					} else {
						if (Fusion_charge == 0)
							Players[Player_num].energy -= F1_0*2;

						Fusion_charge += FrameTime;
						Players[Player_num].energy -= FrameTime;

						if (Players[Player_num].energy <= 0) {
							Players[Player_num].energy = 0;
							Auto_fire_fusion_cannon_time = GameTime -1;				//	Fire now!
						} else
							Auto_fire_fusion_cannon_time = GameTime + FrameTime/2 + 1;		//	Fire the fusion cannon at this time in the future.

						if (Fusion_charge < F1_0*2)
							PALETTE_FLASH_ADD(Fusion_charge >> 11, 0, Fusion_charge >> 11);
						else
							PALETTE_FLASH_ADD(Fusion_charge >> 11, Fusion_charge >> 11, 0);

						if (Fusion_next_sound_time < GameTime) {
							if (Fusion_charge > F1_0*2) {
								digi_play_sample( 11, F1_0 );
#ifdef NETWORK
								if(Game_mode & GM_MULTI)
									multi_send_play_sound(11, F1_0);
#endif
								apply_damage_to_player(ConsoleObject, ConsoleObject, d_rand() * 4);
							} else {
								create_awareness_event(ConsoleObject, PA_WEAPON_ROBOT_COLLISION);
								digi_play_sample( SOUND_FUSION_WARMUP, F1_0 );
								#ifdef NETWORK
								if (Game_mode & GM_MULTI)
									multi_send_play_sound(SOUND_FUSION_WARMUP, F1_0);
								#endif
							}
							Fusion_next_sound_time = GameTime + F1_0/8 + d_rand()/4;
						}
					}
				}
			}

			if (Auto_fire_fusion_cannon_time) {
				if (Primary_weapon != FUSION_INDEX)
					Auto_fire_fusion_cannon_time = 0;
				else if (GameTime + FrameTime/2 >= Auto_fire_fusion_cannon_time) {
					Auto_fire_fusion_cannon_time = 0;
					Global_laser_firing_count = 1;
				} else {
					vms_vector	rand_vec;
					fix			bump_amount;

					Global_laser_firing_count = 0;

					ConsoleObject->mtype.phys_info.rotvel.x += (d_rand() - 16384)/8;
					ConsoleObject->mtype.phys_info.rotvel.z += (d_rand() - 16384)/8;
					make_random_vector(&rand_vec);

					bump_amount = F1_0*4;

					if (Fusion_charge > F1_0*2)
						bump_amount = Fusion_charge*4;

					bump_one_object(ConsoleObject, &rand_vec, bump_amount);
				}
			}

			if (Global_laser_firing_count) {
				if(!(use_alt_vulcanfire && (Primary_weapon==VULCAN_INDEX))){
					Global_laser_firing_count -= do_laser_firing_player();  //do_laser_firing(Players[Player_num].objnum, Primary_weapon);
				}
				else
					Global_laser_firing_count = 0;
			}
			if (Global_laser_firing_count < 0)
				Global_laser_firing_count = 0;
		}

#ifdef NETWORK
	if(use_alt_vulcanfire) //should only be active in multi games
		vulcanframe();
#endif

	if (Do_appearance_effect) {
		create_player_appearance_effect(ConsoleObject);
		Do_appearance_effect = 0;
	}
}

//	-------------------------------------------------------------------------------------------------------
//	If player is close enough to objnum, which ought to be a powerup, pick it up!
//	This could easily be made difficulty level dependent.
void powerup_grab_cheat(object *player, int objnum)
{
	fix	powerup_size;
	fix	player_size;
	fix	dist;

	Assert(Objects[objnum].type == OBJ_POWERUP);

	powerup_size = Objects[objnum].size;
	player_size = player->size;

	dist = vm_vec_dist_quick(&Objects[objnum].pos, &player->pos);

	if ((dist < 2*(powerup_size + player_size)) && !(Objects[objnum].flags & OF_SHOULD_BE_DEAD)) {
		vms_vector	collision_point;

		vm_vec_avg(&collision_point, &Objects[objnum].pos, &player->pos);
		collide_player_and_powerup(player, &Objects[objnum], &collision_point);
	}
}

//	-------------------------------------------------------------------------------------------------------
//	Make it easier to pick up powerups.
//	For all powerups in this segment, pick them up at up to twice pickuppable distance based on dot product
//	from player to powerup and player's forward vector.
//	This has the effect of picking them up more easily left/right and up/down, but not making them disappear
//	way before the player gets there.
void powerup_grab_cheat_all(void)
{
	segment	*segp;
	int 	objnum;

	segp = &Segments[ConsoleObject->segnum];
	objnum = segp->objects;

	while (objnum != -1) {
		if (Objects[objnum].type == OBJ_POWERUP)
			powerup_grab_cheat(ConsoleObject, objnum);
		objnum = Objects[objnum].next;
	}

}

int	Last_level_path_created = -1;

#ifdef SHOW_EXIT_PATH

//	------------------------------------------------------------------------------------------------------------------
//	Create path for player from current segment to goal segment.
//	Return true if path created, else return false.
int mark_player_path_to_segment(int segnum)
{
	int		i;
	object		*objp = ConsoleObject;
	short		player_path_length=0;
	int		player_hide_index=-1;

	if (Last_level_path_created == Current_level_num) {
		return 0;
	}

	Last_level_path_created = Current_level_num;

	if (create_path_points(objp, objp->segnum, segnum, Point_segs_free_ptr, &player_path_length, 100, 0, 0, -1) == -1) {
		mprintf((0, "Unable to form path of length %i for myself\n", 100));
		return 0;
	}

	player_hide_index = Point_segs_free_ptr - Point_segs;
	Point_segs_free_ptr += player_path_length;

	if (Point_segs_free_ptr - Point_segs + MAX_PATH_LENGTH*2 > MAX_POINT_SEGS) {
		mprintf((1, "Can't create path.  Not enough point_segs.\n"));
		ai_reset_all_paths();
		return 0;
	}

	for (i=1; i<player_path_length; i++) {
		int			segnum, objnum;
		vms_vector	seg_center;
		object		*obj;

		segnum = Point_segs[player_hide_index+i].segnum;
		mprintf((0, "%3i ", segnum));
		seg_center = Point_segs[player_hide_index+i].point;

		objnum = obj_create( OBJ_POWERUP, POW_ENERGY, segnum, &seg_center, &vmd_identity_matrix, Powerup_info[POW_ENERGY].size, CT_POWERUP, MT_NONE, RT_POWERUP);
		if (objnum == -1) {
			Int3(); // Unable to drop energy powerup for path
			return 1;
		}

		obj = &Objects[objnum];
		obj->rtype.vclip_info.vclip_num = Powerup_info[obj->id].vclip_num;
		obj->rtype.vclip_info.frametime = Vclip[obj->rtype.vclip_info.vclip_num].frame_time;
		obj->rtype.vclip_info.framenum = 0;
		obj->lifeleft = F1_0*100 + d_rand() * 4;
	}

	mprintf((0, "\n"));
	return 1;
}

//	Return true if it happened, else return false.
int create_special_path(void)
{
	int	i,j;

	//	---------- Find exit doors ----------
	for (i=0; i<=Highest_segment_index; i++)
		for (j=0; j<MAX_SIDES_PER_SEGMENT; j++)
			if (Segments[i].children[j] == -2) {
				mprintf((0, "Exit at segment %i\n", i));
				return mark_player_path_to_segment(i);
			}

	return 0;
}
#endif


#ifndef RELEASE
int	Max_obj_count_mike = 0;

//	Shows current number of used objects.
void show_free_objects(void)
{
	if (!(FrameCount & 8)) {
		int	i;
		int	count=0;

		mprintf((0, "Highest_object_index = %3i, MAX_OBJECTS = %3i, now used = ", Highest_object_index, MAX_OBJECTS));

		for (i=0; i<=Highest_object_index; i++)
			if (Objects[i].type != OBJ_NONE)
				count++;

		mprintf((0, "%3i", count));

		if (count > Max_obj_count_mike) {
			Max_obj_count_mike = count;
			mprintf((0, " ***"));
		}
		mprintf((0, "\n"));
	}
}

#define	FILL_VAL 0xcc // int 3 opcode value

extern void code_01s(void), code_01e(void);
extern void code_02s(void), code_02e(void);
extern void code_03s(void), code_03e(void);
extern void code_04s(void), code_04e(void);
extern void code_05s(void), code_05e(void);
extern void code_06s(void), code_06e(void);
extern void code_07s(void), code_07e(void);
extern void code_08s(void), code_08e(void);
extern void code_09s(void), code_09e(void);
extern void code_10s(void), code_10e(void);
extern void code_11s(void), code_11e(void);
extern void code_12s(void), code_12e(void);
extern void code_13s(void), code_13e(void);
extern void code_14s(void), code_14e(void);
extern void code_15s(void), code_15e(void);
extern void code_16s(void), code_16e(void);
extern void code_17s(void), code_17e(void);
extern void code_18s(void), code_18e(void);
extern void code_19s(void), code_19e(void);
extern void code_20s(void), code_20e(void);
extern void code_21s(void), code_21e(void);

int Mem_filled = 0;

void fill_func(char *start, char *end, char value)
{
	char	*i;

	mprintf((0, "Filling from %p to %p\n", start, end));
	for (i=start; i<end; i++)
		*i = value;

}

void check_func(char *start, char *end, char value)
{
	char	*i;
	for (i=start; i<end; i++)
		if (*i != value) {
			Int3(); // The nast triple aught six bug...we can smell it...contact Mike!
			Error("Oops, the nasty triple aught six bug.  Address == %p\n", i);
		}

}
#endif
