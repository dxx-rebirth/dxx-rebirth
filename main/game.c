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
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <SDL/SDL.h>
#include <setjmp.h>

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
#include "hostage.h"
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
#include "config.h"
#include "mouse.h"
#include "switch.h"
#include "controls.h"
#include "songs.h"
#include "rbaudio.h"
#include "multi.h"
#include "desc_id.h"
#include "cntrlcen.h"
#include "pcx.h"
#include "state.h"
#include "piggy.h"
#include "multibot.h"
#include "fvi.h"
#include "playsave.h"
#include "hudmsg.h"
#include "vers_id.h"
#include "event.h"
#include "window.h"

#ifdef EDITOR
#include "editor/editor.h"
#endif


#ifndef NDEBUG
int	Mark_count = 0;                 // number of debugging marks set
#endif

static fix64 last_timer_value=0;
fix ThisLevelTime=0;

fix			VR_eye_width = F1_0;
int			VR_render_mode = VR_NONE;
int			VR_low_res = 3; // Default to low res
int 			VR_show_hud = 1;
int			VR_sensitivity = 1; // 0 - 2

//NEWVR
int			VR_eye_offset		 = 0;
int			VR_eye_switch		 = 0;
int			VR_eye_offset_changed = 0;
int			VR_use_reg_code 	= 0;

grs_canvas	Screen_3d_window;							// The rectangle for rendering the mine to
grs_canvas	*VR_offscreen_buffer	= NULL;		// The offscreen data buffer
grs_canvas	VR_render_buffer[2];					//  Two offscreen buffers for left/right eyes.
grs_canvas	VR_render_sub_buffer[2];			//  Two sub buffers for left/right eyes.

int	force_cockpit_redraw=0;
int	PaletteRedAdd, PaletteGreenAdd, PaletteBlueAdd;

//	Toggle_var points at a variable which gets !ed on del-T press.
int	Dummy_var;
int	*Toggle_var = &Dummy_var;

#ifdef EDITOR
//flag for whether initial fade-in has been done
char	faded_in;
#endif

int	Game_suspended=0; //if non-zero, nothing moves but player
fix64	Auto_fire_fusion_cannon_time = 0;
fix	Fusion_charge = 0;
int	Game_mode = GM_GAME_OVER;
int	Global_laser_firing_count = 0;
int	Global_missile_firing_count = 0;
fix64	Next_flare_fire_time = 0;

//	Function prototypes for GAME.C exclusively.

void GameProcessFrame(void);
void FireLaser(void);
void powerup_grab_cheat_all(void);
void game_init_render_sub_buffers(int x, int y, int w, int h);

//	Other functions
extern void multi_check_for_killgoal_winner();

extern int ReadControls(d_event *event);		// located in gamecntl.c

// Cheats
game_cheats cheats;

//	==============================================================================================

//this is called once per game
void init_game()
{
	init_objects();

	hostage_init();

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


u_int32_t Game_screen_mode = SM(640,480);

//initialize the various canvases on the game screen
//called every time the screen mode or cockpit changes
void init_cockpit()
{
	//Initialize the on-screen canvases

	if (Screen_mode != SCREEN_GAME)
		return;

	if (( Screen_mode == SCREEN_EDITOR ) || ( VR_render_mode != VR_NONE ))
		PlayerCfg.CockpitMode[1] = CM_FULL_SCREEN;

#ifndef OGL
	if ( Game_screen_mode != (HiresGFXAvailable? SM(640,480) : SM(320,200)) && PlayerCfg.CockpitMode[1] != CM_LETTERBOX) {
		PlayerCfg.CockpitMode[1] = CM_FULL_SCREEN;
	}
#endif

	gr_set_current_canvas(NULL);

	switch( PlayerCfg.CockpitMode[1] ) {
		case CM_FULL_COCKPIT:
			game_init_render_sub_buffers(0, 0, SWIDTH, (SHEIGHT*2)/3);
			break;

		case CM_REAR_VIEW:
		{
			int x1 = 0, y1 = 0, x2 = SWIDTH, y2 = (SHEIGHT*2)/3;
			grs_bitmap *bm;

			PIGGY_PAGE_IN(cockpit_bitmap[PlayerCfg.CockpitMode[1]]);
			bm = &GameBitmaps[cockpit_bitmap[PlayerCfg.CockpitMode[1]].index];
			gr_bitblt_find_transparent_area(bm, &x1, &y1, &x2, &y2);
			game_init_render_sub_buffers(x1*((float)SWIDTH/bm->bm_w), y1*((float)SHEIGHT/bm->bm_h), (x2-x1+1)*((float)SWIDTH/bm->bm_w), (y2-y1+2)*((float)SHEIGHT/bm->bm_h));
			break;
		}
		case CM_FULL_SCREEN:
			game_init_render_sub_buffers(0, 0, SWIDTH, SHEIGHT);
			break;

		case CM_STATUS_BAR:
			game_init_render_sub_buffers( 0, 0, SWIDTH, (HIRESMODE?(SHEIGHT*2)/2.6:(SHEIGHT*2)/2.72) );
			break;

		case CM_LETTERBOX:
		{
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
	if (mode != PlayerCfg.CockpitMode[1]) {		//new mode
		PlayerCfg.CockpitMode[1]=mode;
		init_cockpit();
	}
}

//force cockpit redraw next time. call this if you've trashed the screen
void reset_cockpit()
{
	force_cockpit_redraw=1;
	last_drawn_cockpit = -1;
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
	VR_render_mode 	=	render_method;

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
		return 1;
	}

#ifdef EDITOR
	Canv_editor = NULL;
#endif

	Screen_mode = sm;

	switch( Screen_mode )
	{
		case SCREEN_MENU:
			if  (grd_curscreen->sc_mode != Game_screen_mode)
				if (gr_set_mode(Game_screen_mode))
					Error("Cannot set screen mode.");
			break;

		case SCREEN_GAME:
			if  (grd_curscreen->sc_mode != Game_screen_mode)
				if (gr_set_mode(Game_screen_mode))
					Error("Cannot set screen mode.");
			break;
#ifdef EDITOR
		case SCREEN_EDITOR:
			if (grd_curscreen->sc_mode != SM(800,600))	{
				int gr_error;
				if ((gr_error=gr_set_mode(SM(800,600)))!=0) { //force into game scrren
					Warning("Cannot init editor screen (error=%d)",gr_error);
					return 0;
				}
			}
			break;
#endif
		default:
			Error("Invalid screen mode %d",sm);
	}

	gr_set_current_canvas(NULL);

	return 1;
}

static int time_paused=0;

void stop_time()
{
	if (time_paused==0) {
		fix64 time;
		timer_update();
		time = timer_query();
		last_timer_value = time - last_timer_value;
		if (last_timer_value < 0) {
			last_timer_value = 0;
		}
	}
	time_paused++;
}

void start_time()
{
	time_paused--;
	Assert(time_paused >= 0);
	if (time_paused==0) {
		fix64 time;
		timer_update();
		time = timer_query();
		last_timer_value = time - last_timer_value;
	}
}

void game_flush_inputs()
{
	int dx,dy,dz;
	event_flush();
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
		Timer4 = (Timer4-(F1_0/4));
	}

	Timer20 += FrameTime;
	if (Timer20 >= F1_0/20)
	{
		StepRes |= EPS20;
		Timer20 = (Timer20-(F1_0/20));
	}

	Timer30 += FrameTime;
	if (Timer30 >= F1_0/30)
	{
		StepRes |= EPS30;
		Timer30 = (Timer30-(F1_0/30));
	}

	FixedStep = StepRes;
}

void reset_time()
{
	timer_update();
	last_timer_value = timer_query();
}

void calc_frame_time()
{
	fix64 timer_value;
	fix last_frametime = FrameTime;

	timer_update();
	timer_value = timer_query();
	FrameTime = timer_value - last_timer_value;

	while (FrameTime < f1_0 / (GameCfg.VSync?MAXIMUM_FPS:GameArg.SysMaxFPS))
	{
		if (GameArg.SysUseNiceFPS && !GameCfg.VSync)
			timer_delay(f1_0 / GameArg.SysMaxFPS - FrameTime);
		timer_update();
		timer_value = timer_query();
		FrameTime = timer_value - last_timer_value;
	}

	if ( cheats.turbo )
		FrameTime *= 2;

	last_timer_value = timer_value;

	if (FrameTime < 0)				//if bogus frametime...
		FrameTime = (last_frametime==0?1:last_frametime);		//...then use time from last frame

	GameTime64 += FrameTime;

	FixedStepCalc();
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

	if (!PHYSFSX_exists(SCRNS_DIR,0))
		PHYSFS_mkdir(SCRNS_DIR); //try making directory

	save_canv = grd_curcanv;
	temp_canv = gr_create_canvas(screen_canv->cv_bitmap.bm_w,screen_canv->cv_bitmap.bm_h);
	gr_set_current_canvas(temp_canv);
	gr_ubitmap(0,0,&screen_canv->cv_bitmap);

	gr_set_current_canvas(save_canv);

	do
	{
		sprintf(savename, "%sscrn%04d.pcx",SCRNS_DIR, savenum++);
	} while (PHYSFSX_exists(savename,0));
	sprintf( message, "%s 'scrn%04d.pcx'", TXT_DUMPING_SCREEN, savenum-1 );

	gr_set_current_canvas(NULL);

	if (!automap_flag)
		HUD_init_message(HM_DEFAULT, message);

	gr_palette_read(pal);		//get actual palette from the hardware
	pcx_write_bitmap(savename,&temp_canv->cv_bitmap,pal);
	gr_set_current_canvas(screen_canv);
	gr_ubitmap(0,0,&temp_canv->cv_bitmap);
	gr_free_canvas(temp_canv);
	gr_set_current_canvas(save_canv);

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

void test_anim_states();

extern int been_in_editor;

//	------------------------------------------------------------------------------------
void do_cloak_stuff(void)
{
	int i;

	for (i = 0; i < N_players; i++)
		if (Players[i].flags & PLAYER_FLAGS_CLOAKED) {
			if (GameTime64 > Players[i].cloak_time+CLOAK_TIME_MAX)
			{
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

int FakingInvul=0;

//	------------------------------------------------------------------------------------
void do_invulnerable_stuff(void)
{
	if (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE) {
		if (GameTime64 > Players[Player_num].invulnerable_time+INVULNERABLE_TIME_MAX)
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


//	Amount to diminish guns towards normal, per second.
#define	DIMINISH_RATE 16 // gots to be a power of 2, else change the code in diminish_palette_towards_normal

 //adds to rgb values for palette flash
void PALETTE_FLASH_ADD(int _dr, int _dg, int _db)
{
	int	maxval;

	PaletteRedAdd += _dr;
	PaletteGreenAdd += _dg;
	PaletteBlueAdd += _db;

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

//	--------------------------------------------------------------------------------------------------
int allowed_to_fire_laser(void)
{
	if (Player_is_dead) {
		Global_missile_firing_count = 0;
		return 0;
	}

	//	Make sure enough time has elapsed to fire laser
	if (Next_laser_fire_time > GameTime64)
		return 0;

	return 1;
}

int allowed_to_fire_flare(void)
{
	if (Next_flare_fire_time > GameTime64)
		return 0;

	Next_flare_fire_time = GameTime64 + F1_0/4;

	return 1;
}

int allowed_to_fire_missile(void)
{
	//	Make sure enough time has elapsed to fire missile
	if (Next_missile_fire_time > GameTime64)
		return 0;

	return 1;
}

extern int Death_sequence_aborted;

#ifdef USE_SDLMIXER
#define EXT_MUSIC_TEXT "Jukebox/Audio CD"
#else
#define EXT_MUSIC_TEXT "Audio CD"
#endif

static int free_help(newmenu *menu, d_event *event, void *userdata)
{
	userdata = userdata;

	if (event->type == EVENT_WINDOW_CLOSE)
	{
		newmenu_item *items = newmenu_get_items(menu);
		d_free(items);
	}

	return 0;
}

void show_help()
{
	int nitems = 0;
	newmenu_item *m;

	MALLOC(m, newmenu_item, 26);
	if (!m)
		return;

	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = TXT_HELP_ESC;
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "SHIFT-ESC\t  SHOW GAME LOG";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "F1\t  THIS SCREEN";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = TXT_HELP_F2;
#if !(defined(__APPLE__) || defined(macintosh))
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "Alt-F2/F3\t  SAVE/LOAD GAME";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "Alt-F1\t  Fast Save";
#else
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "Alt-F2/F3 (\x85-SHIFT-s/\x85-o)\t  SAVE/LOAD GAME";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "Alt-F1 (\x85-s)\t  Fast Save";
#endif
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "F3\t  SWITCH COCKPIT MODES";
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

	newmenu_dotiny( NULL, TXT_KEYS, nitems, m, 0, free_help, NULL );
}

void show_netgame_help()
{
	int nitems = 0;
	newmenu_item *m;

	MALLOC(m, newmenu_item, 15);
	if (!m)
		return;

	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "F1\t  THIS SCREEN";
#if !(defined(__APPLE__) || defined(macintosh))
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "Alt-F2/F3\t  SAVE/LOAD COOP GAME";
#else
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "Alt-F2/F3 (\x85-SHIFT-s/\x85-o)\t  SAVE/LOAD COOP GAME";
#endif
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "ALT-F4\t  SHOW PLAYER NAMES ON HUD";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "F7\t  TOGGLE KILL LIST";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "F8\t  SEND MESSAGE";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "(SHIFT-)F9 to F12\t  (DEFINE)SEND MACRO";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "PAUSE\t  SHOW NETGAME INFORMATION";

#if (defined(__APPLE__) || defined(macintosh))
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "(Use \x85-# for F#. e.g. \x85-1 for F1)";
#endif

	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "MULTIPLAYER MESSAGE COMMANDS:";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "(*): TEXT\t  SEND TEXT TO PLAYER/TEAM (*)";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "/move: (*)\t  MOVE PLAYER (*) TO OTHER TEAM (Host-only)";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "/kick: (*)\t  KICK PLAYER (*) FROM GAME (Host-only)";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "/KillReactor\t  BLOW UP THE MINE (Host-only)";

	newmenu_dotiny( NULL, TXT_KEYS, nitems, m, 0, free_help, NULL );
}

void show_newdemo_help()
{
	newmenu_item *m;
	int nitems = 0;

	MALLOC(m, newmenu_item, 15);
	if (!m)
		return;

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

	newmenu_dotiny( NULL, "DEMO PLAYBACK CONTROLS", nitems, m, 0, free_help, NULL );
}

//temp function until Matt cleans up game sequencing
extern void temp_reset_stuff_on_level();

#define LEAVE_TIME 0x4000		//how long until we decide key is down	(Used to be 0x4000)

//deal with rear view - switch it on, or off, or whatever
void check_rear_view()
{
	static int leave_mode;
	static fix64 entry_time;

	if (Newdemo_state == ND_STATE_PLAYBACK)
		return;

	if ( Controls.rear_view_count > 0) {	//key/button has gone down
		Controls.rear_view_count = 0;

		if (Rear_view) {
			Rear_view = 0;
			if (PlayerCfg.CockpitMode[1]==CM_REAR_VIEW) {
				select_cockpit(PlayerCfg.CockpitMode[0]);
			}
			if (Newdemo_state == ND_STATE_RECORDING)
				newdemo_record_restore_rearview();
		}
		else {
			Rear_view = 1;
			leave_mode = 0;		//means wait for another key
			entry_time = timer_query();
			if (PlayerCfg.CockpitMode[1] == CM_FULL_COCKPIT) {
				select_cockpit(CM_REAR_VIEW);
			}
			if (Newdemo_state == ND_STATE_RECORDING)
				newdemo_record_rearview();
		}
	}
	else
		if (Controls.rear_view_state) {

			if (leave_mode == 0 && (timer_query() - entry_time) > LEAVE_TIME)
				leave_mode = 1;
		}
		else
		{
			if (leave_mode==1 && Rear_view) {
				Rear_view = 0;
				if (PlayerCfg.CockpitMode[1]==CM_REAR_VIEW) {
					select_cockpit(PlayerCfg.CockpitMode[0]);
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
	select_cockpit(PlayerCfg.CockpitMode[0]);
}

int Config_menu_flag;

void game_disable_cheats()
{
	memset(&cheats, 0, sizeof(cheats));
}

//	game_setup()
// ----------------------------------------------------------------------------

int game_handler(window *wind, d_event *event, void *data);

extern int netplayerinfo_on;

window *game_setup(void)
{
	window *game_wind;

	PlayerCfg.CockpitMode[1] = PlayerCfg.CockpitMode[0];
	last_drawn_cockpit = -1;	// Force cockpit to redraw next time a frame renders.
	Endlevel_sequence = 0;

	game_wind = window_create(&grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT, game_handler, NULL);
	if (!game_wind)
		return NULL;

	reset_palette_add();
	init_cockpit();
	init_gauges();
	netplayerinfo_on = 0;

#ifdef EDITOR
	if (!Cursegp)
	{
		Cursegp = &Segments[0];
		Curside = 0;
	}
	
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
		init_player_stats_game(Player_num);
		init_ai_objects();
	}
#endif

	fix_object_segs();

	return game_wind;
}

void game_render_frame();

window *Game_wind = NULL;

// Event handler for the game
int game_handler(window *wind, d_event *event, void *data)
{
	data = data;

	switch (event->type)
	{
		case EVENT_WINDOW_ACTIVATED:
			set_screen_mode(SCREEN_GAME);

			event_toggle_focus(1);
			key_toggle_repeat(0);
			game_flush_inputs();

			if (time_paused)
				start_time();

			if (!((Game_mode & GM_MULTI) && (Newdemo_state != ND_STATE_PLAYBACK)))
				digi_resume_digi_sounds();

			if (!((Game_mode & GM_MULTI) && (Newdemo_state != ND_STATE_PLAYBACK)))
				palette_restore();
			
			reset_cockpit();
			break;

		case EVENT_WINDOW_DEACTIVATED:
			if (!(((Game_mode & GM_MULTI) && (Newdemo_state != ND_STATE_PLAYBACK)) && (!Endlevel_sequence)) )
				stop_time();

			if (!((Game_mode & GM_MULTI) && (Newdemo_state != ND_STATE_PLAYBACK)))
				digi_pause_digi_sounds();

			if (!((Game_mode & GM_MULTI) && (Newdemo_state != ND_STATE_PLAYBACK)))
				palette_save();

			event_toggle_focus(0);
			key_toggle_repeat(1);
			break;

		case EVENT_JOYSTICK_BUTTON_UP:
		case EVENT_JOYSTICK_BUTTON_DOWN:
		case EVENT_JOYSTICK_MOVED:
		case EVENT_MOUSE_BUTTON_UP:
		case EVENT_MOUSE_BUTTON_DOWN:
		case EVENT_MOUSE_MOVED:
		case EVENT_KEY_COMMAND:
		case EVENT_KEY_RELEASE:
		case EVENT_IDLE:
			return ReadControls(event);

		case EVENT_WINDOW_DRAW:
			if (!time_paused)
			{
				calc_frame_time();
				GameProcessFrame();
			}

			if (!Automap_active)		// efficiency hack
			{
				if (force_cockpit_redraw) {			//screen need redrawing?
					init_cockpit();
					force_cockpit_redraw=0;
				}
				game_render_frame();
			}
			break;

		case EVENT_WINDOW_CLOSE:
			digi_stop_digi_sounds();

			if ( (Newdemo_state == ND_STATE_RECORDING) || (Newdemo_state == ND_STATE_PAUSED) )
				newdemo_stop_recording();

#ifdef NETWORK
			multi_leave_game();
#endif

			if ( Newdemo_state == ND_STATE_PLAYBACK )
				newdemo_stop_playback();

			songs_play_song( SONG_TITLE, 1 );

			game_disable_cheats();
			Game_mode = GM_GAME_OVER;
#ifdef EDITOR
			if (!EditorWindow)		// have to do it this way because of the necessary longjmp. Yuck.
#endif
				show_menus();
			Game_wind = NULL;
			event_toggle_focus(0);
			key_toggle_repeat(1);
			break;

		case EVENT_WINDOW_CLOSED:
			longjmp(LeaveEvents, 0);
			break;

		default:
			break;
	}

	return 0;
}

// Initialise game, actually runs in main event loop
void game()
{
	hide_menus();
	Game_wind = game_setup();
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
}


#ifdef EDITOR
extern void player_follow_path(object *objp);
extern void check_create_player_path(void);
#endif

extern	int Do_appearance_effect;

void game_leave_menus(void)
{
	window *wind;

	if (!Game_wind)
		return;

	while ((wind = window_get_front()) && (wind != Game_wind)) // go through all windows and actually close them if they want to
		window_close(wind);
}

void GameProcessFrame(void)
{
	fix player_shields = Players[Player_num].shields;
	int was_fuelcen_destroyed = Control_center_destroyed;
	int player_was_dead = Player_is_dead;

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
	{
		multi_do_frame();
		if (Netgame.PlayTimeAllowed && ThisLevelTime>=i2f((Netgame.PlayTimeAllowed*5*60)))
			multi_check_for_killgoal_winner();
	}
#endif

	dead_player_frame();
	if (Newdemo_state != ND_STATE_PLAYBACK)
		do_controlcen_dead_frame();

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

	if (Control_center_destroyed) {
		if (Newdemo_state==ND_STATE_RECORDING )
			newdemo_record_control_center_destroyed();
	}

	flash_frame();

	if ( Newdemo_state == ND_STATE_PLAYBACK ) {
		newdemo_playback_one_frame();
		if ( Newdemo_state != ND_STATE_PLAYBACK )		{
			if (Game_wind)
				window_close(Game_wind);		// Go back to menu
			return;
		}
	}
	else
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
			else if (GameTime64 + FrameTime/2 >= Auto_fire_fusion_cannon_time) {
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
			Players[Player_num].invulnerable_time = GameTime64-i2f(27);
			FakingInvul=1;
		}
#endif
	}

	// Check if we have to close in-game menus for multiplayer
	if (Endlevel_sequence || (Player_is_dead != player_was_dead) || (Players[Player_num].shields < player_shields))
		game_leave_menus();

	if ((Control_center_destroyed && !was_fuelcen_destroyed) || ((Control_center_destroyed) && (Countdown_seconds_left < 10)))
		game_leave_menus();
}

//	-----------------------------------------------------------------------------
//	Fire Laser:  Registers a laser fire, and performs special stuff for the fusion
//				    cannon.
void FireLaser()
{

	Global_laser_firing_count = Controls.fire_primary_state?Weapon_info[Primary_weapon_to_weapon_info[Primary_weapon]].fire_count:0;

	if ((Primary_weapon == FUSION_INDEX) && (Global_laser_firing_count)) {
		if ((Players[Player_num].energy < F1_0*2) && (Auto_fire_fusion_cannon_time == 0)) {
			Global_laser_firing_count = 0;
		} else {
			static fix64 Fusion_next_sound_time = 0;

			if (Fusion_charge == 0)
				Players[Player_num].energy -= F1_0*2;

			Fusion_charge += FrameTime;
			Players[Player_num].energy -= FrameTime;

			if (Players[Player_num].energy <= 0) {
				Players[Player_num].energy = 0;
				Auto_fire_fusion_cannon_time = GameTime64 -1;	//	Fire now!
			} else
				Auto_fire_fusion_cannon_time = GameTime64 + FrameTime/2 + 1;		//	Fire the fusion cannon at this time in the future.

			if (Fusion_charge < F1_0*2)
				PALETTE_FLASH_ADD(Fusion_charge >> 11, 0, Fusion_charge >> 11);
			else
				PALETTE_FLASH_ADD(Fusion_charge >> 11, Fusion_charge >> 11, 0);

			if (Fusion_next_sound_time > GameTime64 + F1_0/8 + D_RAND_MAX/4) // GameTime64 is smaller than max delay - player in new level?
				Fusion_next_sound_time = GameTime64 - 1;

			if (Fusion_next_sound_time < GameTime64) {
				if (Fusion_charge > F1_0*2) {
					digi_play_sample( 11, F1_0 );
#ifdef NETWORK
					if(Game_mode & GM_MULTI)
						multi_send_play_sound(11, F1_0);
#endif
					apply_damage_to_player(ConsoleObject, ConsoleObject, d_rand() * 4, 0);
				} else {
					create_awareness_event(ConsoleObject, PA_WEAPON_ROBOT_COLLISION);
					digi_play_sample( SOUND_FUSION_WARMUP, F1_0 );
					#ifdef NETWORK
					if (Game_mode & GM_MULTI)
						multi_send_play_sound(SOUND_FUSION_WARMUP, F1_0);
					#endif
				}
				Fusion_next_sound_time = GameTime64 + F1_0/8 + d_rand()/4;
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
