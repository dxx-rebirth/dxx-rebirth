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
 * Game loop for Inferno
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>

#ifdef OGL
#include "ogl_init.h"
#endif

#include "pstypes.h"
#include "console.h"
#include "gr.h"
#include "inferno.h"
#include "game.h"
#include "key.h"
#include "object.h"
#include "physics.h"
#include "error.h"
#include "joy.h"
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
#include "fuelcen.h"
#include "digi.h"
#include "u_mem.h"
#include "palette.h"
#include "morph.h"
#include "lighting.h"
#include "newdemo.h"
#include "collide.h"
#include "weapon.h"
#include "sounds.h"
#include "args.h"
#include "gameseq.h"
#include "automap.h"
#include "text.h"
#include "powerup.h"
#include "fireball.h"
#include "newmenu.h"
#include "gamefont.h"
#include "endlevel.h"
#include "kconfig.h"
#include "mouse.h"
#include "switch.h"
#include "controls.h"
#include "songs.h"
#include "rbaudio.h"
#include "gamepal.h"

#include "multi.h"
#include "desc_id.h"
#include "cntrlcen.h"
#include "pcx.h"
#include "state.h"
#include "piggy.h"
#include "multibot.h"
#include "ai.h"
#include "robot.h"
#include "playsave.h"
#include "fix.h"
#include "hudmsg.h"
#include "movie.h"

#ifdef MWPROFILER
#include <profiler.h>
#endif

//#define TEST_TIMER	1		//if this is set, do checking on timer

#define	SHOW_EXIT_PATH	1

#ifdef EDITOR
#include "editor/editor.h"
#endif

//#define _MARK_ON 1
#ifdef __WATCOMC__
#if __WATCOMC__ < 1000
#include <wsample.h>            //should come after inferno.h to get mark setting
#endif
#endif


extern void ReadControls(void);		// located in gamecntl.c
extern void do_final_boss_frame(void);

#ifndef NDEBUG
int	Mark_count = 0;                 // number of debugging marks set
#endif

static fix last_timer_value=0;
fix ThisLevelTime=0;

ubyte			VR_screen_flags	= 0;		//see values in screens.h
fix			VR_eye_width		= F1_0;
int			VR_render_mode		= VR_NONE;
int			VR_low_res 			= 3;				// Default to low res
int 			VR_show_hud = 1;
int			VR_sensitivity     = 1;		// 0 - 2

//NEWVR
int			VR_eye_offset		 = 0;
int			VR_eye_switch		 = 0;
int			VR_eye_offset_changed = 0;
int			VR_use_reg_code 	= 0;

grs_canvas	Screen_3d_window;							// The rectangle for rendering the mine to
grs_canvas  *VR_offscreen_buffer	= NULL;		// The offscreen data buffer
grs_canvas	VR_render_buffer[2];					//  Two offscreen buffers for left/right eyes.
grs_canvas	VR_render_sub_buffer[2];			//  Two sub buffers for left/right eyes.
grs_canvas	VR_editor_canvas;						//  The canvas that the editor writes to.

static int	old_cockpit_mode=-1;

int force_cockpit_redraw=0;

int PaletteRedAdd, PaletteGreenAdd, PaletteBlueAdd;

//	Toggle_var points at a variable which gets !ed on ctrl-alt-T press.
int	Dummy_var;
int	*Toggle_var = &Dummy_var;

#ifdef EDITOR
//flag for whether initial fade-in has been done
char faded_in;
#endif

int Game_suspended=0;           //if non-zero, nothing moves but player

fix	Auto_fire_fusion_cannon_time = 0;
fix	Fusion_charge = 0;
fix	Fusion_next_sound_time = 0;
fix	Fusion_last_sound_time = 0;

int Debug_spew = 1;
int Game_turbo_mode = 0;

int Game_mode = GM_GAME_OVER;

int	Global_laser_firing_count = 0;
int	Global_missile_firing_count = 0;

grs_bitmap background_bitmap;

int Game_aborted;

#define BACKGROUND_NAME "statback.pcx"

//	Function prototypes for GAME.C exclusively.

void GameLoop(int RenderFlag, int ReadControlsFlag);
void FireLaser(void);
void slide_textures(void);
void powerup_grab_cheat_all(void);

//	Other functions
extern void multi_check_for_killgoal_winner();
extern void RestoreGameSurfaces();

// text functions

void fill_background();

extern ubyte DefiningMarkerMessage;
extern char Marker_input[];

//	==============================================================================================

//this is called once per game
void init_game()
{
	init_objects();

	init_special_effects();

	init_exploding_walls();

	Clear_window = 2;		//	do portal only window clear.
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


u_int32_t Game_screen_mode = SM(640,480);

extern void newdemo_record_cockpit_change(int);

//initialize the various canvases on the game screen
//called every time the screen mode or cockpit changes
void init_cockpit()
{
	int x,y,w,h;

#ifndef OGL
	if ((SWIDTH == 320 && SHEIGHT == 200) || (GameArg.GfxHiresGFXAvailable && (SWIDTH == 640 && SHEIGHT == 480)))
#endif
	{
		VR_screen_flags = VRF_ALLOW_COCKPIT;
	}

	if ((!(VR_screen_flags & VRF_ALLOW_COCKPIT) && (PlayerCfg.CockpitMode==CM_FULL_COCKPIT || PlayerCfg.CockpitMode==CM_STATUS_BAR || PlayerCfg.CockpitMode==CM_REAR_VIEW)) || ( VR_render_mode != VR_NONE ) || ( Screen_mode == SCREEN_EDITOR ))
		PlayerCfg.CockpitMode = CM_FULL_SCREEN;

	gr_set_current_canvas(NULL);

	switch( PlayerCfg.CockpitMode ) {
	case CM_FULL_COCKPIT:
		game_init_render_sub_buffers(0, 0, grd_curscreen->sc_w,(grd_curscreen->sc_h*2)/3); 
		break;

	case CM_REAR_VIEW:
	case CM_FULL_SCREEN:
		game_init_render_sub_buffers(0, 0, SWIDTH, SHEIGHT);
		break;

	case CM_STATUS_BAR:
		game_init_render_sub_buffers( 0, 0, SWIDTH, (HIRESMODE?(SHEIGHT*2)/2.6:(SHEIGHT*2)/2.72) );
		break;

	case CM_LETTERBOX:
		x = 0; w = SM_W(Game_screen_mode);
		h = (SM_H(Game_screen_mode) * 3) / 4; // true letterbox size (16:9)
		y = (SM_H(Game_screen_mode)-h)/2;

		gr_rect(x,0,w,SM_H(Game_screen_mode)-h);
		gr_rect(x,SM_H(Game_screen_mode)-h,w,SM_H(Game_screen_mode));

		game_init_render_sub_buffers( x, y, w, h );
		break;
	}

	if (Newdemo_state==ND_STATE_RECORDING) {
		newdemo_record_cockpit_change(PlayerCfg.CockpitMode);
	}

	gr_set_current_canvas(NULL);
}

//selects a given cockpit (or lack of one).  See types in game.h
void select_cockpit(int mode)
{
	if (mode != PlayerCfg.CockpitMode) {		//new mode
		PlayerCfg.CockpitMode=mode;
		init_cockpit();
	}
}

//force cockpit redraw next time. call this if you've trashed the screen
void reset_cockpit()
{
	force_cockpit_redraw=1;
	last_drawn_cockpit = -1;
}

// void HUD_clear_messages();				//Already declared in gauges.h

//NEWVR
void VR_reset_params()
{
	VR_eye_width = VR_SEPARATION;
	VR_eye_offset = VR_PIXEL_SHIFT;
	VR_eye_offset_changed = 2;
}

void game_init_render_sub_buffers( int x, int y, int w, int h )
{
	gr_clear_canvas(0);
	gr_init_sub_canvas(&Screen_3d_window, &grd_curscreen->sc_canvas, x, y, w, h);
	gr_init_sub_canvas( &VR_render_sub_buffer[0], &VR_render_buffer[0], x, y, w, h );
	gr_init_sub_canvas( &VR_render_sub_buffer[1], &VR_render_buffer[1], x, y, w, h );
}


// Sets up the canvases we will be rendering to (NORMAL VERSION)
void game_init_render_buffers(int render_w, int render_h, int render_method, int flags )
{

	VR_screen_flags	=	flags;

	VR_reset_params();

	VR_render_mode 	=	render_method;


	if (VR_offscreen_buffer) {
		gr_free_canvas(VR_offscreen_buffer);
	}

	if ( (VR_render_mode==VR_AREA_DET) || (VR_render_mode==VR_INTERLACED ) )	{
		if ( render_h*2 < 200 )	{
			VR_offscreen_buffer = gr_create_canvas( render_w, 200 );
		}
		else {
			VR_offscreen_buffer = gr_create_canvas( render_w, render_h*2 );
		}

		gr_init_sub_canvas( &VR_render_buffer[0], VR_offscreen_buffer, 0, 0, render_w, render_h );
		gr_init_sub_canvas( &VR_render_buffer[1], VR_offscreen_buffer, 0, render_h, render_w, render_h );
	}
	else {
		if ( render_h < 200 ) {
			VR_offscreen_buffer = gr_create_canvas( render_w, 200 );
		}
		else {
            VR_offscreen_buffer = gr_create_canvas( render_w, render_h );
        }

#ifdef OGL
		VR_offscreen_buffer->cv_bitmap.bm_type = BM_OGL;
#endif

		gr_init_sub_canvas( &VR_render_buffer[0], VR_offscreen_buffer, 0, 0, render_w, render_h );
		gr_init_sub_canvas( &VR_render_buffer[1], VR_offscreen_buffer, 0, 0, render_w, render_h );
	}

	game_init_render_sub_buffers( 0, 0, render_w, render_h );
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

#ifdef EDITOR
	Canv_editor = NULL;
#endif

	Screen_mode = sm;

	switch( Screen_mode )
	{
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
			init_editor_screen();   //setup other editor stuff
			break;
#endif
		case SCREEN_MOVIE:
			/* give control back to the WM */
			if (GameArg.CtlGrabMouse)
				SDL_WM_GrabInput(SDL_GRAB_OFF);

			if (grd_curscreen->sc_mode != SM(MOVIE_WIDTH,MOVIE_HEIGHT))	{
				if (gr_set_mode(SM(MOVIE_WIDTH,MOVIE_HEIGHT))) Error("Cannot set screen mode for game!");
				gr_palette_load( gr_palette );
			}
			break;
		default:
			Error("Invalid screen mode %d",sm);
	}

	gr_set_current_canvas(NULL);

#ifndef OGL
	gr_set_draw_buffer(((Screen_mode == SCREEN_GAME) && GameArg.DbgUseDoubleBuffer) ? 1 : 0); // Double buffering or 'front' buffer only
#endif

	return 1;
}

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

/*
    Calculates several - common used - timesteps and stores into FixedStep
*/
void FixedStepCalc()
{
	int StepRes = 0;
	static fix Timer4 = 0, Timer20 = 0, Timer30 = 0;

	Timer4 += FrameTime;
	if (Timer4 >= F1_0/4)
	{
		StepRes |= EPS4;
		Timer4 = 0 + (Timer4-F1_0/4);
	}

	Timer20 += FrameTime;
	if (Timer20 >= F1_0/20)
	{
		StepRes |= EPS20;
		Timer20 = 0 + (Timer20-F1_0/20);
	}
	
	Timer30 += FrameTime;
	if (Timer30 >= F1_0/30)
	{
		StepRes |= EPS30;
		Timer30 = 0 + (Timer30-F1_0/30);
	}
	
	FixedStep = StepRes;
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

	while (FrameTime < f1_0 / (GameCfg.VSync?MAXIMUM_FPS:GameArg.SysMaxFPS))
	{
		if (GameArg.SysUseNiceFPS && !GameCfg.VSync)
			timer_delay(f1_0 / GameArg.SysMaxFPS - FrameTime);
		timer_value = timer_get_fixed_seconds();
		FrameTime = timer_value - last_timer_value;
	}

	if ( Game_turbo_mode )
		FrameTime *= 2;

	last_timer_value = timer_value;

	if (FrameTime < 0)				//if bogus frametime...
		FrameTime = (last_frametime==0?1:last_frametime);		//...then use time from last frame
		
	GameTime += FrameTime;

	if (GameTime < 0 || GameTime > i2f(0x7fff - 600))
		GameTime = FrameTime;	//wrap when goes negative, or ~9hrs
		
	FixedStepCalc();
}

//--unused-- int Auto_flythrough=0;  //if set, start flythough automatically

void move_player_2_segment(segment *seg,int side)
{
	vms_vector vp;

	compute_segment_center(&ConsoleObject->pos,seg);
	compute_center_point_on_side(&vp,seg,side);
	vm_vec_sub2(&vp,&ConsoleObject->pos);
	vm_vector_2_matrix(&ConsoleObject->orient,&vp,NULL,NULL);

	obj_relink( ConsoleObject-Objects, SEG_PTR_2_NUM(seg) );

}

void do_photos();
void level_with_floor();

#ifndef OGL
void save_screen_shot(int automap_flag)
{
	char message[100];
	grs_canvas *screen_canv=&grd_curscreen->sc_canvas;
	static int savenum=0;
	grs_canvas *temp_canv,*save_canv;
        char savename[FILENAME_LEN+sizeof(SCRNS_DIR)];
	ubyte pal[768];

	stop_time();

	if (!cfexist(SCRNS_DIR))
		PHYSFS_mkdir(SCRNS_DIR); //try making directory

	save_canv = grd_curcanv;
	temp_canv = gr_create_canvas(screen_canv->cv_bitmap.bm_w,screen_canv->cv_bitmap.bm_h);
	gr_set_current_canvas(temp_canv);
	gr_ubitmap(0,0,&screen_canv->cv_bitmap);

	gr_set_current_canvas(save_canv);

	do
	{
		sprintf(savename, "%sscrn%04d.pcx",SCRNS_DIR, savenum++);
	} while (PHYSFS_exists(savename));
	sprintf( message, "%s 'scrn%04d.pcx'", TXT_DUMPING_SCREEN, savenum-1 );

	gr_set_current_canvas(NULL);

	if (!automap_flag)
		hud_message(MSGC_GAME_FEEDBACK,message);

	gr_palette_read(pal);		//get actual palette from the hardware
	pcx_write_bitmap(savename,&temp_canv->cv_bitmap,pal);
	gr_set_current_canvas(screen_canv);
	gr_ubitmap(0,0,&temp_canv->cv_bitmap);
	gr_free_canvas(temp_canv);
	gr_set_current_canvas(save_canv);

	key_flush();
	start_time();
}

#endif

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

//void morph_test(), morph_step();


//	------------------------------------------------------------------------------------

void test_anim_states();

#include "fvi.h"

extern int been_in_editor;

//	------------------------------------------------------------------------------------
void do_cloak_stuff(void)
{
	int i;
	for (i = 0; i < N_players; i++)
		if (Players[i].flags & PLAYER_FLAGS_CLOAKED) {
			if (Players[Player_num].cloak_time+CLOAK_TIME_MAX-GameTime < 0 &&
				Players[Player_num].cloak_time+CLOAK_TIME_MAX-GameTime > -F1_0*2)
			{
				Players[i].flags &= ~PLAYER_FLAGS_CLOAKED;
				if (i == Player_num) {
					digi_play_sample( SOUND_CLOAK_OFF, F1_0);
					#ifdef NETWORK
					if (Game_mode & GM_MULTI)
						multi_send_play_sound(SOUND_CLOAK_OFF, F1_0);
					maybe_drop_net_powerup(POW_CLOAK);
					if ( Newdemo_state == ND_STATE_PLAYBACK )
						multi_send_decloak(); // For demo recording
					#endif
				}
			}
		}
}

int FakingInvul=0;

//	------------------------------------------------------------------------------------
void do_invulnerable_stuff(void)
{
	if (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE) {
		if (Players[Player_num].invulnerable_time+INVULNERABLE_TIME_MAX-GameTime < 0 &&
			Players[Player_num].invulnerable_time+INVULNERABLE_TIME_MAX-GameTime > -F1_0*2)
		{
			Players[Player_num].flags ^= PLAYER_FLAGS_INVULNERABLE;
			if (FakingInvul==0)
			{
				digi_play_sample( SOUND_INVULNERABILITY_OFF, F1_0);
				#ifdef NETWORK
				if (Game_mode & GM_MULTI)
				{
					multi_send_play_sound(SOUND_INVULNERABILITY_OFF, F1_0);
					maybe_drop_net_powerup(POW_INVULNERABILITY);
				}
				#endif
			}
			FakingInvul=0;
		}
	}
}

ubyte	Last_afterburner_state = 0;
fix Last_afterburner_charge = 0;

#define AFTERBURNER_LOOP_START	((GameArg.SndDigiSampleRate==SAMPLE_RATE_22K)?32027:(32027/2))		//20098
#define AFTERBURNER_LOOP_END		((GameArg.SndDigiSampleRate==SAMPLE_RATE_22K)?48452:(48452/2))		//25776

int	Ab_scale = 4;

//	------------------------------------------------------------------------------------
#ifdef NETWORK
extern void multi_send_sound_function (char,char);
#endif

void do_afterburner_stuff(void)
{
   if (!(Players[Player_num].flags & PLAYER_FLAGS_AFTERBURNER))
		Afterburner_charge=0;

	if (Endlevel_sequence || Player_is_dead)
		{
		 digi_kill_sound_linked_to_object( Players[Player_num].objnum);
#ifdef NETWORK
		 multi_send_sound_function (0,0);
#endif
		}

	if ((Controls.afterburner_state != Last_afterburner_state && Last_afterburner_charge) || (Last_afterburner_state && Last_afterburner_charge && !Afterburner_charge)) {

		if (Afterburner_charge && Controls.afterburner_state && (Players[Player_num].flags & PLAYER_FLAGS_AFTERBURNER)) {
			digi_link_sound_to_object3( SOUND_AFTERBURNER_IGNITE, Players[Player_num].objnum, 1, F1_0, i2f(256), AFTERBURNER_LOOP_START, AFTERBURNER_LOOP_END );
#ifdef NETWORK
			if (Game_mode & GM_MULTI)
				multi_send_sound_function (3,SOUND_AFTERBURNER_IGNITE);
#endif
		} else {
			digi_kill_sound_linked_to_object( Players[Player_num].objnum);
			digi_link_sound_to_object2( SOUND_AFTERBURNER_PLAY, Players[Player_num].objnum, 0, F1_0, i2f(256));
#ifdef NETWORK
			if (Game_mode & GM_MULTI)
			 	multi_send_sound_function (0,0);
#endif
		}
	}

	//@@if (Controls.afterburner_state && Afterburner_charge)
	//@@	afterburner_shake();

	Last_afterburner_state = Controls.afterburner_state;
	Last_afterburner_charge = Afterburner_charge;
}

// -- //	------------------------------------------------------------------------------------
// -- //	if energy < F1_0/2, recharge up to F1_0/2
// -- void recharge_energy_frame(void)
// -- {
// -- 	if (Players[Player_num].energy < Weapon_info[0].energy_usage) {
// -- 		Players[Player_num].energy += FrameTime/4;
// --
// -- 		if (Players[Player_num].energy > Weapon_info[0].energy_usage)
// -- 			Players[Player_num].energy = Weapon_info[0].energy_usage;
// -- 	}
// -- }

//	Amount to diminish guns towards normal, per second.
#define	DIMINISH_RATE	16		//	gots to be a power of 2, else change the code in diminish_palette_towards_normal

extern fix Flash_effect;

 //adds to rgb values for palette flash
void PALETTE_FLASH_ADD(int _dr,int _dg,int _db)
{
	int	maxval;

	PaletteRedAdd += _dr;
	PaletteGreenAdd += _dg;
	PaletteBlueAdd += _db;

	if (Flash_effect)
		maxval = 60;
	else
		maxval = MAX_PALETTE_ADD;

	if (PaletteRedAdd > maxval)
		PaletteRedAdd = maxval;

	if (PaletteGreenAdd > maxval)
		PaletteGreenAdd = maxval;

	if (PaletteBlueAdd > maxval)
		PaletteBlueAdd = maxval;

	if (PaletteRedAdd < -maxval)
		PaletteRedAdd = -maxval;

	if (PaletteGreenAdd < -maxval)
		PaletteGreenAdd = -maxval;

	if (PaletteBlueAdd < -maxval)
		PaletteBlueAdd = -maxval;
}

fix	Time_flash_last_played;

//	------------------------------------------------------------------------------------
//	Diminish palette effects towards normal.
void diminish_palette_towards_normal(void)
{
	int	dec_amount = 0;

	//	Diminish at DIMINISH_RATE units/second.
	//	For frame rates > DIMINISH_RATE Hz, use randomness to achieve this.
	if (FrameTime < F1_0/DIMINISH_RATE) {
		if (d_rand() < FrameTime*DIMINISH_RATE/2)	//	Note: d_rand() is in 0..32767, and 8 Hz means decrement every frame
			dec_amount = 1;
	} else {
		dec_amount = f2i(FrameTime*DIMINISH_RATE);		// one second = DIMINISH_RATE counts
		if (dec_amount == 0)
			dec_amount++;						// make sure we decrement by something
	}

	if (Flash_effect) {
		int	force_do = 0;

		//	Part of hack system to force update of palette after exiting a menu.
		if (Time_flash_last_played) {
			force_do = 1;
			PaletteRedAdd ^= 1;	//	Very Tricky!  In gr_palette_step_up, if all stepups same as last time, won't do anything!
		}

		if ((Time_flash_last_played + F1_0/8 < GameTime) || (Time_flash_last_played > GameTime)) {
			digi_play_sample( SOUND_CLOAK_OFF, Flash_effect/4);
			Time_flash_last_played = GameTime;
		}

		Flash_effect -= FrameTime;
		if (Flash_effect < 0)
			Flash_effect = 0;

		if (force_do || (d_rand() > 4096 )) {
      	if ( (Newdemo_state==ND_STATE_RECORDING) && (PaletteRedAdd || PaletteGreenAdd || PaletteBlueAdd) )
	      	newdemo_record_palette_effect(PaletteRedAdd, PaletteGreenAdd, PaletteBlueAdd);

			gr_palette_step_up( PaletteRedAdd, PaletteGreenAdd, PaletteBlueAdd );

			return;
		}

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

	//	Forces flash effect to fixup palette next frame.
	Time_flash_last_played = 0;
}

extern void dead_player_frame(void);


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
#define	FLARE_BIG_DELAY	(F1_0*2)

int allowed_to_fire_flare(void)
{
	if (Next_flare_fire_time > GameTime)
		if (Next_flare_fire_time < GameTime + FLARE_BIG_DELAY)	//	In case time is bogus, never wait > 1 second.
			return 0;

	if (Players[Player_num].energy >= Weapon_info[FLARE_ID].energy_usage)
		Next_flare_fire_time = GameTime + F1_0/4;
	else
		Next_flare_fire_time = GameTime + FLARE_BIG_DELAY;

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

void full_palette_save(void)
{
	palette_save();
	apply_modified_palette();
	reset_palette_add();
	gr_palette_load( gr_palette );
}

extern int Death_sequence_aborted;
extern int newmenu_dotiny2( char * title, char * subtitle, int nitems, newmenu_item * item, void (*subfunction)(int nitems,newmenu_item * items, int * last_key, int citem) );

#ifdef USE_SDLMIXER
#define EXT_MUSIC_TEXT "Jukebox/Audio CD"
#else
#define EXT_MUSIC_TEXT "Audio CD"
#endif

void show_help()
{
	int nitems = 0;
	newmenu_item m[25];
	
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = TXT_HELP_ESC;
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "SHIFT-ESC\t  SHOW GAME LOG";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "F1\t  THIS SCREEN";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = TXT_HELP_F2;
#if !(defined(__APPLE__) || defined(macintosh))
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "Alt-F2/F3\t  SAVE/LOAD GAME";
#else
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "Alt-F2/F3 (\x85-s/o)\t  SAVE/LOAD GAME";
#endif
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "F3\t  SWITCH COCKPIT MODES";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = TXT_HELP_F4;
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = TXT_HELP_F5;
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "ALT-F7\t  SWITCH HUD MODES";
#if !(defined(__APPLE__) || defined(macintosh))
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = TXT_HELP_PAUSE;
#else
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "Pause (\x85-P)\t  Pause";
#endif
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = TXT_HELP_PRTSCN;
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = TXT_HELP_1TO5;
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = TXT_HELP_6TO10;
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "Shift-F1/F2\t  Cycle left/right window";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "Shift-F4\t  GuideBot menu";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "Alt-Shift-F4\t  Rename GuideBot";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "Shift-F5/F6\t  Drop primary/secondary";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "Shift-number\t  GuideBot commands";
#if !(defined(__APPLE__) || defined(macintosh))
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "Alt-Shift-F9\t  Eject Audio CD";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "Alt-Shift-F10\t  Play/Pause " EXT_MUSIC_TEXT;
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "Alt-Shift-F11/F12\t  Previous/Next Song";
#else
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "\x85-E\t  Eject Audio CD";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "\x85-Up/Down\t  Play/Pause " EXT_MUSIC_TEXT;
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "\x85-Left/Right\t  Previous/Next Song";
#endif
#if (defined(__APPLE__) || defined(macintosh))
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "(Use \x85-# for F#. e.g. \x85-1 for F1)";
#endif

	full_palette_save();

	newmenu_dotiny2( NULL, TXT_KEYS, nitems, m, NULL );

	palette_restore();
}

void show_netgame_help()
{
	int nitems = 0;
	newmenu_item m[17];

	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "F1\t  THIS SCREEN";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "ALT-0\t  DROP FLAG";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "ALT-F4\t  SHOW RETICLE NAMES";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "F7\t  TOGGLE KILL LIST";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "F8\t  SEND MESSAGE";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "(SHIFT-)F8 to F12\t  (DEFINE)SEND MACRO";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "PAUSE\t  SHOW NETGAME INFORMATION";
#if (defined(__APPLE__) || defined(macintosh))
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "(Use \x85-# for F#. e.g. \x85-1 for F1)";
#endif
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "MULTIPLAYER MESSAGE COMMANDS:";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "!Names\t  TOGGLE NAMES RETURN";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "Handicap: (*)\t  SET YOUR STARTING SHIELDS TO (*) [10-100]";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "NoBombs\t  TOGGLE SMART BOMBS (Host-only)";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "move: (*)\t  MOVE PLAYER (*) TO OTHER TEAM (Host-only)";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "kick: (*)\t  KICK PLAYER (*) FROM GAME (Host-only)";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "KillReactor\t  BLOW UP THE MINE (Host-only)";

	full_palette_save();

	newmenu_dotiny2( NULL, TXT_KEYS, nitems, m, NULL );

	palette_restore();
}

void show_newdemo_help()
{
	newmenu_item m[15];
	int nitems = 0;

	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "ESC\t  QUIT DEMO PLAYBACK";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "F1\t  THIS SCREEN";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = TXT_HELP_F2;
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "F3\t  SWITCH COCKPIT MODES";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "F4\t  TOGGLE PERCENTAGE DISPLAY";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "UP\t  PLAY";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "DOWN\t  PAUSE";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "RIGHT\t  ONE FRAME FORWARD";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "LEFT\t  ONE FRAME BACKWARD";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "SHIFT-RIGHT\t  FAST FORWARD";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "SHIFT-LEFT\t  FAST BACKWARD";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "CTRL-RIGHT\t  JUMP TO END";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "CTRL-LEFT\t  JUMP TO START";
#if (defined(__APPLE__) || defined(macintosh))
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "(Use \x85-# for F#. e.g. \x85-1 for F1)";
#endif
	full_palette_save();
	newmenu_dotiny2( NULL, "DEMO PLAYBACK CONTROLS", nitems, m, NULL );
	palette_restore();
}

//temp function until Matt cleans up game sequencing
extern void temp_reset_stuff_on_level();

#define LEAVE_TIME 0x4000		//how long until we decide key is down	(Used to be 0x4000)

//deal with rear view - switch it on, or off, or whatever
void check_rear_view()
{
	static int leave_mode;
	static fix entry_time;

	if ( Controls.rear_view_down_count )	{		//key/button has gone down

		if (Rear_view) {
			Rear_view = 0;
			if (PlayerCfg.CockpitMode==CM_REAR_VIEW) {
				select_cockpit(old_cockpit_mode);
			}
			if (Newdemo_state == ND_STATE_RECORDING)
				newdemo_record_restore_rearview();
		}
		else {
			Rear_view = 1;
			if (LEAVE_TIME <= 0)
			{
				leave_mode = 1; // set leave mode on here otherwise we will always have to hold for at least 1 frame to get leave_mode on
			}
			else
			{
				leave_mode = 0; // means wait for another key
				entry_time = timer_get_fixed_seconds();
			}
			if (PlayerCfg.CockpitMode == CM_FULL_COCKPIT) {
				old_cockpit_mode = PlayerCfg.CockpitMode;
				select_cockpit(CM_REAR_VIEW);
			}
			if (Newdemo_state == ND_STATE_RECORDING)
				newdemo_record_rearview();
		}
	}
	else
		if (Controls.rear_view_down_state) {

			if (leave_mode == 0 && (timer_get_fixed_seconds() - entry_time) > LEAVE_TIME)
				leave_mode = 1;
		}
		else {

			//@@if (leave_mode==1 && PlayerCfg.CockpitMode==CM_REAR_VIEW) {

			if (leave_mode==1 && Rear_view) {
				Rear_view = 0;
				if (PlayerCfg.CockpitMode==CM_REAR_VIEW) {
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

	if (!(PlayerCfg.CockpitMode == CM_FULL_COCKPIT || PlayerCfg.CockpitMode == CM_STATUS_BAR || PlayerCfg.CockpitMode == CM_FULL_SCREEN)) {
		if (!(old_cockpit_mode == CM_FULL_COCKPIT || old_cockpit_mode == CM_STATUS_BAR || old_cockpit_mode == CM_FULL_SCREEN))
			old_cockpit_mode = CM_FULL_COCKPIT;
		select_cockpit(old_cockpit_mode);
	}

}

int Automap_flag;
int Config_menu_flag;

jmp_buf LeaveGame;

int Cheats_enabled=0;

extern int Laser_rapid_fire;
extern void do_lunacy_on(), do_lunacy_off();

extern int Physics_cheat_flag,Robots_kill_robots_cheat;
extern char BounceCheat,HomingCheat,OldHomingState[20];
extern char AcidCheatOn,old_IntMethod, Monster_mode;
extern int Buddy_dude_cheat;

//turns off active cheats
void turn_cheats_off()
{
	int i;

	if (HomingCheat)
		for (i=0;i<20;i++)
			Weapon_info[i].homing_flag=OldHomingState[i];

	if (AcidCheatOn)
	{
		AcidCheatOn=0;
		Interpolation_method=old_IntMethod;
	}

	Buddy_dude_cheat = 0;
	BounceCheat=0;
   HomingCheat=0;
	do_lunacy_off();
	Laser_rapid_fire = 0;
	Physics_cheat_flag = 0;
	Monster_mode = 0;
	Robots_kill_robots_cheat=0;
	Robot_firing_enabled = 1;
}

//turns off all cheats & resets cheater flag	
void game_disable_cheats()
{
	turn_cheats_off();
	Cheats_enabled=0;
}

//	game_setup()
// ----------------------------------------------------------------------------

void game_setup(void)
{
	//@@int demo_playing=0;
	//@@int multi_game=0;

	do_lunacy_on();		//	Copy values for insane into copy buffer in ai.c
	do_lunacy_off();		//	Restore true insane mode.

	Game_aborted = 0;
	last_drawn_cockpit = -1;				// Force cockpit to redraw next time a frame renders.
	Endlevel_sequence = 0;

	set_screen_mode(SCREEN_GAME);

	reset_palette_add();

	set_warn_func(game_show_warning);

	init_cockpit();
	init_gauges();
	//digi_init_sounds();

	//keyd_repeat = 0;                // Don't allow repeat in game
	keyd_repeat = 1;                // Do allow repeat in game

#ifdef __MSDOS__
	//_MARK_("start of game");
#endif

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
	if (Current_level_num == 0) {			//not a real level
		init_player_stats_game();
		init_ai_objects();
	}
	#endif

	fix_object_segs();

	game_flush_inputs();

}


#ifdef NETWORK
extern char IWasKicked;
#endif


//	------------------------------------------------------------------------------------
//this function is the game.  called when game mode selected.  runs until
//editor mode or exit selected
void game()
{
	game_setup();								// Replaces what was here earlier.
													// Good for Windows Sake.

#ifdef MWPROFILE
	ProfilerSetStatus(1);
#endif

	if ( setjmp(LeaveGame)==0 )	{
		while (1) {
			int player_shields;

			// GAME LOOP!
			Automap_flag = 0;
			Config_menu_flag = 0;

			player_shields = Players[Player_num].shields;

			ExtGameStatus=GAMESTAT_RUNNING;
			GameLoop( 1, 1 );		// Do game loop with rendering and reading controls.

			//if the player is taking damage, give up guided missile control
			if (Players[Player_num].shields != player_shields)
				release_guided_missile(Player_num);

			//see if redbook song needs to be restarted
			RBACheckFinishedHook();	// Handle RedBook Audio Repeating.

			if (Config_menu_flag) 	{
				if (!(Game_mode&GM_MULTI)) {palette_save(); reset_palette_add();	apply_modified_palette(); gr_palette_load( gr_palette ); }
				do_options_menu();
				if (!(Game_mode&GM_MULTI)) palette_restore();
			}

			if (Automap_flag) {
				game_flush_inputs();
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
				palette_save();
				apply_modified_palette();
				reset_palette_add();
				gr_palette_load( gr_palette );
				choice=nm_messagebox( NULL, 2, TXT_YES, TXT_NO, TXT_ABORT_AUTODEMO );
				palette_restore();
				Function_mode = fmode;
				if (choice==0)	{
					GameArg.SysAutoDemo = 0;
					newdemo_stop_playback();
					Function_mode = FMODE_MENU;
				} else {
					Function_mode = FMODE_GAME;
				}
			}

			if ( (Function_mode != FMODE_GAME ) && (Newdemo_state != ND_STATE_PLAYBACK ) && (Function_mode!=FMODE_EDITOR)
#ifdef NETWORK
				       	&& !IWasKicked
#endif
			   )		{
				int choice, fmode;
				fmode = Function_mode;
				Function_mode = FMODE_GAME;
				palette_save();
				apply_modified_palette();
				reset_palette_add();
				gr_palette_load( gr_palette );
				ExtGameStatus=GAMESTAT_ABORT_GAME;
				choice=nm_messagebox( NULL, 2, TXT_YES, TXT_NO, TXT_ABORT_GAME );
				palette_restore();
				Function_mode = fmode;
				if (choice != 0)
					Function_mode = FMODE_GAME;
			}

#ifdef NETWORK
			IWasKicked=0;
#endif
			if (Function_mode != FMODE_GAME)
				longjmp(LeaveGame,0);

			#ifdef APPLE_DEMO
			if ( (keyd_time_when_last_pressed + (F1_0 * 60)) < timer_get_fixed_seconds() )		// idle in game for 1 minutes means exit
				longjmp(LeaveGame,0);
			#endif
		}
	}

#ifdef MWPROFILE
	ProfilerSetStatus(0);
#endif

	digi_stop_all();

	if ( (Newdemo_state == ND_STATE_RECORDING) || (Newdemo_state == ND_STATE_PAUSED) )
		newdemo_stop_recording();

	#ifdef NETWORK
	multi_leave_game();
	#endif

	if ( Newdemo_state == ND_STATE_PLAYBACK )
 		newdemo_stop_playback();

	clear_warn_func(game_show_warning);     //don't use this func anymore

	game_disable_cheats();

	#ifdef APPLE_DEMO
	Function_mode = FMODE_EXIT;		// get out of game in Apple OEM version
	#endif
}

//called at the end of the program
void close_game()
{
	if (VR_offscreen_buffer)	{
		gr_free_canvas(VR_offscreen_buffer);
		VR_offscreen_buffer = NULL;
	}

	close_gauges();

	restore_effect_bitmap_icons();

	if (background_bitmap.bm_data)
		d_free(background_bitmap.bm_data);

	clear_warn_func(game_show_warning);     //don't use this func anymore
}


extern void kconfig_center_headset();

#ifdef EDITOR
extern void player_follow_path(object *objp);
extern void check_create_player_path(void);
#endif

extern	int	Do_appearance_effect;

object *Missile_viewer=NULL;
int Missile_viewer_sig=-1;

int Marker_viewer_num[2]={-1,-1};
int Coop_view_player[2]={-1,-1};

//returns ptr to escort robot, or NULL
object *find_escort()
{
	int i;

	for (i=0; i<=Highest_object_index; i++)
		if (Objects[i].type == OBJ_ROBOT)
			if (Robot_info[Objects[i].id].companion)
				return &Objects[i];

	return NULL;
}

extern void process_super_mines_frame(void);
extern void do_seismic_stuff(void);
extern int Level_shake_duration;

//if water or fire level, make occasional sound
void do_ambient_sounds()
{
	int has_water,has_lava;
	int sound;

	has_lava = (Segment2s[ConsoleObject->segnum].s2_flags & S2F_AMBIENT_LAVA);
	has_water = (Segment2s[ConsoleObject->segnum].s2_flags & S2F_AMBIENT_WATER);

	if (has_lava) {							//has lava
		sound = SOUND_AMBIENT_LAVA;
		if (has_water && (d_rand() & 1))	//both, pick one
			sound = SOUND_AMBIENT_WATER;
	}
	else if (has_water)						//just water
		sound = SOUND_AMBIENT_WATER;
	else
		return;

	if (((d_rand() << 3) < FrameTime)) {						//play the sound
		fix volume = d_rand() + f1_0/2;
		digi_play_sample(sound,volume);
	}
}

// -- extern void lightning_frame(void);

void game_render_frame();
extern void omega_charge_frame(void);

extern time_t t_current_time, t_saved_time;

void flicker_lights();

void GameLoop(int RenderFlag, int ReadControlsFlag )
{
	if (RenderFlag) {
		if (force_cockpit_redraw) {			//screen need redrawing?
			init_cockpit();
			force_cockpit_redraw=0;
		}
		game_render_frame();
	}

	calc_frame_time();

	if (ReadControlsFlag)
		ReadControls();
	else
		memset(&Controls, 0, sizeof(Controls));

	update_player_stats();
	diminish_palette_towards_normal();		//	Should leave palette effect up for as long as possible by putting right before render.
	do_afterburner_stuff();
	do_cloak_stuff();
	do_invulnerable_stuff();
	remove_obsolete_stuck_objects();
	init_ai_frame();
	do_final_boss_frame();

	if ((Players[Player_num].flags & PLAYER_FLAGS_HEADLIGHT) && (Players[Player_num].flags & PLAYER_FLAGS_HEADLIGHT_ON)) {
		static int turned_off=0;
		Players[Player_num].energy -= (FrameTime*3/8);
		if (Players[Player_num].energy < i2f(10)) {
			if (!turned_off) {
				Players[Player_num].flags &= ~PLAYER_FLAGS_HEADLIGHT_ON;
				turned_off = 1;
#ifdef NETWORK
				if (Game_mode & GM_MULTI)
					multi_send_flags(Player_num);		
#endif
			}
		}
		else
			turned_off = 0;

		if (Players[Player_num].energy <= 0) {
			Players[Player_num].energy = 0;
			Players[Player_num].flags &= ~PLAYER_FLAGS_HEADLIGHT_ON;
#ifdef NETWORK
			if (Game_mode & GM_MULTI)
				multi_send_flags(Player_num);		
#endif
		}
	}


#ifdef EDITOR
	check_create_player_path();
	player_follow_path(ConsoleObject);
#endif

#ifdef NETWORK
	if (Game_mode & GM_MULTI)
	{
		multi_do_frame();
		if (Netgame.PlayTimeAllowed && ThisLevelTime>=i2f((Netgame.PlayTimeAllowed*5*60)))
			multi_check_for_killgoal_winner();
	}
#endif

	dead_player_frame();
	if (Newdemo_state != ND_STATE_PLAYBACK)
		do_controlcen_dead_frame();

	process_super_mines_frame();
	do_seismic_stuff();
	do_ambient_sounds();

#ifdef NETWORK
	if ((Game_mode & GM_MULTI) && Netgame.PlayTimeAllowed)
		ThisLevelTime +=FrameTime;
#endif

	digi_sync_sounds();

	if (Endlevel_sequence) {
		do_endlevel_frame();
		powerup_grab_cheat_all();
		do_special_effects();
		return;					//skip everything else
	}

	if (Newdemo_state != ND_STATE_PLAYBACK)
		do_exploding_wall_frame();
	if ((Newdemo_state != ND_STATE_PLAYBACK) || (Newdemo_vcr_state != ND_STATE_PAUSED)) {
		do_special_effects();
		wall_frame_process();
		triggers_frame_process();
	}


	if (Control_center_destroyed)	{
		if (Newdemo_state==ND_STATE_RECORDING )
			newdemo_record_control_center_destroyed();
	}

	flash_frame();

	if ( Newdemo_state == ND_STATE_PLAYBACK )	{
		newdemo_playback_one_frame();
		if ( Newdemo_state != ND_STATE_PLAYBACK )		{
			longjmp( LeaveGame, 0 );		// Go back to menu
		}
	} else
	{ // Note the link to above!

		Players[Player_num].homing_object_dist = -1;		//	Assume not being tracked.  Laser_do_weapon_sequence modifies this.

		object_move_all();
		powerup_grab_cheat_all();

		if (Endlevel_sequence)	//might have been started during move
			return;

		fuelcen_update_all();

		do_ai_frame_all();

		if (allowed_to_fire_laser())
			FireLaser();				// Fire Laser!

		if (Auto_fire_fusion_cannon_time) {
			if (Primary_weapon != FUSION_INDEX)
				Auto_fire_fusion_cannon_time = 0;
			else if (GameTime + FrameTime/2 >= Auto_fire_fusion_cannon_time) {
				Auto_fire_fusion_cannon_time = 0;
				Global_laser_firing_count = 1;
			} else if (FixedStep & EPS20) {
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
			else
			{
				Global_laser_firing_count = 0;
			}
		}

		if (Global_laser_firing_count)
			Global_laser_firing_count -= do_laser_firing_player();

		if (Global_laser_firing_count < 0)
			Global_laser_firing_count = 0;
	}

	if (Do_appearance_effect) {
		create_player_appearance_effect(ConsoleObject);
		Do_appearance_effect = 0;
#ifdef NETWORK
		if ((Game_mode & GM_MULTI) && Netgame.InvulAppear)
		{
			Players[Player_num].flags |= PLAYER_FLAGS_INVULNERABLE;
			Players[Player_num].invulnerable_time = GameTime-i2f(27);
			FakingInvul=1;
		}
#endif
	}

	omega_charge_frame();
	slide_textures();
	flicker_lights();
}

//!!extern int Goal_blue_segnum,Goal_red_segnum;
//!!extern int Hoard_goal_eclip;
//!!
//!!//do cool pulsing lights in hoard goals
//!!hoard_light_pulse()
//!!{
//!!	if (Game_mode & GM_HOARD) {
//!!		fix light;
//!!		int frame;
//!!
//!!		frame = Effects[Hoard_goal_eclip].frame_count;
//!!
//!!		frame++;
//!!
//!!		if (frame >= Effects[Hoard_goal_eclip].vc.num_frames)
//!!			frame = 0;
//!!
//!!		light = abs(frame - 5) * f1_0 / 5;
//!!
//!!		Segment2s[Goal_red_segnum].static_light = Segment2s[Goal_blue_segnum].static_light = light;
//!!	}
//!!}


ubyte	Slide_segs[MAX_SEGMENTS];
int	Slide_segs_computed;

void compute_slide_segs(void)
{
	int	segnum, sidenum;

	for (segnum=0;segnum<=Highest_segment_index;segnum++) {
		Slide_segs[segnum] = 0;
		for (sidenum=0;sidenum<6;sidenum++) {
			int tmn = Segments[segnum].sides[sidenum].tmap_num;
			if (TmapInfo[tmn].slide_u != 0 || TmapInfo[tmn].slide_v != 0)
				Slide_segs[segnum] |= 1 << sidenum;
		}
	}

	Slide_segs_computed = 1;
}

//	-----------------------------------------------------------------------------
void slide_textures(void)
{
	int segnum,sidenum,i;

	if (!Slide_segs_computed)
		compute_slide_segs();

	for (segnum=0;segnum<=Highest_segment_index;segnum++) {
		if (Slide_segs[segnum]) {
			for (sidenum=0;sidenum<6;sidenum++) {
				if (Slide_segs[segnum] & (1 << sidenum)) {
					int tmn = Segments[segnum].sides[sidenum].tmap_num;
					if (TmapInfo[tmn].slide_u != 0 || TmapInfo[tmn].slide_v != 0) {
						for (i=0;i<4;i++) {
							Segments[segnum].sides[sidenum].uvls[i].u += fixmul(FrameTime,TmapInfo[tmn].slide_u<<8);
							Segments[segnum].sides[sidenum].uvls[i].v += fixmul(FrameTime,TmapInfo[tmn].slide_v<<8);
							if (Segments[segnum].sides[sidenum].uvls[i].u > f2_0) {
								int j;
								for (j=0;j<4;j++)
									Segments[segnum].sides[sidenum].uvls[j].u -= f1_0;
							}
							if (Segments[segnum].sides[sidenum].uvls[i].v > f2_0) {
								int j;
								for (j=0;j<4;j++)
									Segments[segnum].sides[sidenum].uvls[j].v -= f1_0;
							}
							if (Segments[segnum].sides[sidenum].uvls[i].u < -f2_0) {
								int j;
								for (j=0;j<4;j++)
									Segments[segnum].sides[sidenum].uvls[j].u += f1_0;
							}
							if (Segments[segnum].sides[sidenum].uvls[i].v < -f2_0) {
								int j;
								for (j=0;j<4;j++)
									Segments[segnum].sides[sidenum].uvls[j].v += f1_0;
							}
						}
					}
				}
			}
		}
	}
}

flickering_light Flickering_lights[MAX_FLICKERING_LIGHTS];

int Num_flickering_lights=0;

void flicker_lights()
{
	int l;
	flickering_light *f;

	f = Flickering_lights;

	for (l=0;l<Num_flickering_lights;l++,f++) {
		segment *segp = &Segments[f->segnum];

		//make sure this is actually a light
		if (! (WALL_IS_DOORWAY(segp, f->sidenum) & WID_RENDER_FLAG))
			continue;
		if (! (TmapInfo[segp->sides[f->sidenum].tmap_num].lighting | TmapInfo[segp->sides[f->sidenum].tmap_num2 & 0x3fff].lighting))
			continue;

		if (f->timer == 0x80000000)		//disabled
			continue;

		if ((f->timer -= FrameTime) < 0) {

			while (f->timer < 0)
				f->timer += f->delay;

			f->mask = ((f->mask&0x80000000)?1:0) + (f->mask<<1);

			if (f->mask & 1)
				add_light(f->segnum,f->sidenum);
			else
				subtract_light(f->segnum,f->sidenum);
		}
	}
}

//returns ptr to flickering light structure, or NULL if can't find
flickering_light *find_flicker(int segnum,int sidenum)
{
	int l;
	flickering_light *f;

	//see if there's already an entry for this seg/side

	f = Flickering_lights;

	for (l=0;l<Num_flickering_lights;l++,f++)
		if (f->segnum == segnum && f->sidenum == sidenum)	//found it!
			return f;

	return NULL;
}

//turn flickering off (because light has been turned off)
void disable_flicker(int segnum,int sidenum)
{
	flickering_light *f;

	if ((f=find_flicker(segnum,sidenum)) != NULL)
		f->timer = 0x80000000;
}

//turn flickering off (because light has been turned on)
void enable_flicker(int segnum,int sidenum)
{
	flickering_light *f;

	if ((f=find_flicker(segnum,sidenum)) != NULL)
		f->timer = 0;
}


#ifdef EDITOR

//returns 1 if ok, 0 if error
int add_flicker(int segnum, int sidenum, fix delay, unsigned long mask)
{
	int l;
	flickering_light *f;

	//see if there's already an entry for this seg/side

	f = Flickering_lights;

	for (l=0;l<Num_flickering_lights;l++,f++)
		if (f->segnum == segnum && f->sidenum == sidenum)	//found it!
			break;

	if (mask==0) {		//clearing entry
		if (l == Num_flickering_lights)
			return 0;
		else {
			int i;
			for (i=l;i<Num_flickering_lights-1;i++)
				Flickering_lights[i] = Flickering_lights[i+1];
			Num_flickering_lights--;
			return 1;
		}
	}

	if (l == Num_flickering_lights) {
		if (Num_flickering_lights == MAX_FLICKERING_LIGHTS)
			return 0;
		else
			Num_flickering_lights++;
	}

	f->segnum = segnum;
	f->sidenum = sidenum;
	f->delay = f->timer = delay;
	f->mask = mask;

	return 1;
}

#endif

//	-----------------------------------------------------------------------------
//	Fire Laser:  Registers a laser fire, and performs special stuff for the fusion
//				    cannon.
void FireLaser()
{

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
				Auto_fire_fusion_cannon_time = GameTime -1;	//	Fire now!
			} else
				Auto_fire_fusion_cannon_time = GameTime + FrameTime/2 + 1;
												//	Fire the fusion cannon at this time in the future.

			if (Fusion_charge < F1_0*2)
				PALETTE_FLASH_ADD(Fusion_charge >> 11, 0, Fusion_charge >> 11);
			else
				PALETTE_FLASH_ADD(Fusion_charge >> 11, Fusion_charge >> 11, 0);

			if (GameTime < Fusion_last_sound_time)		//gametime has wrapped
				Fusion_next_sound_time = Fusion_last_sound_time = GameTime;

			if (Fusion_next_sound_time < GameTime) {
				if (Fusion_charge > F1_0*2) {
					digi_play_sample( 11, F1_0 );
					apply_damage_to_player(ConsoleObject, ConsoleObject, d_rand() * 4);
				} else {
					create_awareness_event(ConsoleObject, PA_WEAPON_ROBOT_COLLISION);
					digi_play_sample( SOUND_FUSION_WARMUP, F1_0 );
					#ifdef NETWORK
					if (Game_mode & GM_MULTI)
						multi_send_play_sound(SOUND_FUSION_WARMUP, F1_0);
					#endif
				}
				Fusion_last_sound_time = GameTime;
				Fusion_next_sound_time = GameTime + F1_0/8 + d_rand()/4;
			}
		}
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
	int		objnum;

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
	object	*objp = ConsoleObject;
	short		player_path_length=0;
	int		player_hide_index=-1;

	if (Last_level_path_created == Current_level_num) {
		return 0;
	}

	Last_level_path_created = Current_level_num;

	if (create_path_points(objp, objp->segnum, segnum, Point_segs_free_ptr, &player_path_length, 100, 0, 0, -1) == -1) {
		return 0;
	}

	player_hide_index = Point_segs_free_ptr - Point_segs;
	Point_segs_free_ptr += player_path_length;

	if (Point_segs_free_ptr - Point_segs + MAX_PATH_LENGTH*2 > MAX_POINT_SEGS) {
		ai_reset_all_paths();
		return 0;
	}

	for (i=1; i<player_path_length; i++) {
		int			segnum, objnum;
		vms_vector	seg_center;
		object		*obj;

		segnum = Point_segs[player_hide_index+i].segnum;
		seg_center = Point_segs[player_hide_index+i].point;

		objnum = obj_create( OBJ_POWERUP, POW_ENERGY, segnum, &seg_center, &vmd_identity_matrix, Powerup_info[POW_ENERGY].size, CT_POWERUP, MT_NONE, RT_POWERUP);
		if (objnum == -1) {
			Int3();		//	Unable to drop energy powerup for path
			return 1;
		}

		obj = &Objects[objnum];
		obj->rtype.vclip_info.vclip_num = Powerup_info[obj->id].vclip_num;
		obj->rtype.vclip_info.frametime = Vclip[obj->rtype.vclip_info.vclip_num].frame_time;
		obj->rtype.vclip_info.framenum = 0;
		obj->lifeleft = F1_0*100 + d_rand() * 4;
	}

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

		for (i=0; i<=Highest_object_index; i++)
			if (Objects[i].type != OBJ_NONE)
				count++;

		if (count > Max_obj_count_mike) {
			Max_obj_count_mike = count;
		}
	}
}

#endif

/*
 * reads a flickering_light structure from a CFILE
 */
void flickering_light_read(flickering_light *fl, CFILE *fp)
{
	fl->segnum = cfile_read_short(fp);
	fl->sidenum = cfile_read_short(fp);
	fl->mask = cfile_read_int(fp);
	fl->timer = cfile_read_fix(fp);
	fl->delay = cfile_read_fix(fp);
}

void flickering_light_write(flickering_light *fl, PHYSFS_file *fp)
{
	PHYSFS_writeSLE16(fp, fl->segnum);
	PHYSFS_writeSLE16(fp, fl->sidenum);
	PHYSFS_writeULE32(fp, fl->mask);
	PHYSFSX_writeFix(fp, fl->timer);
	PHYSFSX_writeFix(fp, fl->delay);
}
