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
#include <SDL.h>

#ifdef OGL
#include "ogl_init.h"
#endif

#include "pstypes.h"
#include "console.h"
#include "gr.h"
#include "inferno.h"
#include "game.h"
#include "key.h"
#include "config.h"
#include "object.h"
#include "physics.h"
#include "dxxerror.h"
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

#include "multi.h"
#include "cntrlcen.h"
#include "pcx.h"
#include "state.h"
#include "piggy.h"
#include "multibot.h"
#include "fvi.h"
#include "ai.h"
#include "robot.h"
#include "playsave.h"
#include "maths.h"
#include "hudmsg.h"
#if defined(DXX_BUILD_DESCENT_II)
#include "gamepal.h"
#include "movie.h"
#endif
#include "event.h"
#include "window.h"

#ifdef EDITOR
#include "editor/editor.h"
#include "editor/esegment.h"
#endif

#include "compiler-range_for.h"
#include "highest_valid.h"
#include "partial_range.h"
#include "segiter.h"

#ifndef NDEBUG
int	Mark_count = 0;                 // number of debugging marks set
#endif

static fix64 last_timer_value=0;
fix ThisLevelTime=0;

grs_canvas	Screen_3d_window;							// The rectangle for rendering the mine to

int	force_cockpit_redraw=0;
int	PaletteRedAdd, PaletteGreenAdd, PaletteBlueAdd;

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

static void GameProcessFrame(void);
static void FireLaser(void);
static void powerup_grab_cheat_all(void);

#if defined(DXX_BUILD_DESCENT_II)
static void slide_textures(void);
static void flicker_lights();
#endif

// Cheats
game_cheats cheats;

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


u_int32_t Game_screen_mode = SM(640,480);

//initialize the various canvases on the game screen
//called every time the screen mode or cockpit changes
void init_cockpit()
{
	//Initialize the on-screen canvases

	if (Screen_mode != SCREEN_GAME)
		return;

	if ( Screen_mode == SCREEN_EDITOR )
		PlayerCfg.CockpitMode[1] = CM_FULL_SCREEN;

#ifndef OGL
#if defined(DXX_BUILD_DESCENT_II)
	int HiresGFXAvailable = !GameArg.GfxSkipHiresGFX;
#endif
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
			unsigned x1 = 0, y1 = 0, x2 = SWIDTH, y2 = (SHEIGHT*2)/3;
			int mode = PlayerCfg.CockpitMode[1];
#if defined(DXX_BUILD_DESCENT_II)
			mode += (HIRESMODE?(Num_cockpits/2):0);
#endif

			PIGGY_PAGE_IN(cockpit_bitmap[mode]);
			auto &bm = GameBitmaps[cockpit_bitmap[mode].index];
			gr_bitblt_find_transparent_area(bm, x1, y1, x2, y2);
			game_init_render_sub_buffers(x1*((float)SWIDTH/bm.bm_w), y1*((float)SHEIGHT/bm.bm_h), (x2-x1+1)*((float)SWIDTH/bm.bm_w), (y2-y1+2)*((float)SHEIGHT/bm.bm_h));
			break;
		}

		case CM_FULL_SCREEN:
			game_init_render_sub_buffers(0, 0, SWIDTH, SHEIGHT);
			break;

		case CM_STATUS_BAR:
			game_init_render_sub_buffers( 0, 0, SWIDTH, (HIRESMODE?(SHEIGHT*2)/2.6:(SHEIGHT*2)/2.72) );
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
void select_cockpit(cockpit_mode_t mode)
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
#if defined(DXX_BUILD_DESCENT_II)
		case SCREEN_MOVIE:
			if (grd_curscreen->sc_mode != SM(MOVIE_WIDTH,MOVIE_HEIGHT))	{
				if (gr_set_mode(SM(MOVIE_WIDTH,MOVIE_HEIGHT))) Error("Cannot set screen mode for game!");
				gr_palette_load( gr_palette );
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
	Controls = {};
}

/*
 * timer that every sets d_tick_step true and increments d_tick_count every 1000/DESIGNATED_GAME_FPS ms.
 */
void calc_d_tick()
{
	static fix timer = 0;

	d_tick_step = 0;

	timer += FrameTime;
	if (timer >= DESIGNATED_GAME_FRAMETIME)
	{
		d_tick_step = 1;
		d_tick_count++;
		if (d_tick_count > 1000000)
			d_tick_count = 0;
		timer -= DESIGNATED_GAME_FRAMETIME;
	}
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
		if (Game_mode & GM_MULTI)
			multi_do_frame(); // during long wait, keep packets flowing
		if (!GameArg.SysNoNiceFPS && !GameCfg.VSync)
			timer_delay(F1_0>>8);
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

	calc_d_tick();
}

void move_player_2_segment(const vsegptridx_t seg,int side)
{
	compute_segment_center(ConsoleObject->pos,seg);
	auto vp = compute_center_point_on_side(seg,side);
	vm_vec_sub2(vp,ConsoleObject->pos);
	vm_vector_2_matrix(ConsoleObject->orient,vp,nullptr,nullptr);

	obj_relink( ConsoleObject-Objects, seg );

}

#ifndef OGL
void save_screen_shot(int automap_flag)
{
	grs_canvas *screen_canv=&grd_curscreen->sc_canvas;
	static int savenum=0;
	grs_canvas *save_canv;
        char savename[FILENAME_LEN+sizeof(SCRNS_DIR)];
	palette_array_t pal;

	stop_time();

	if (!PHYSFSX_exists(SCRNS_DIR,0))
		PHYSFS_mkdir(SCRNS_DIR); //try making directory

	save_canv = grd_curcanv;
	auto temp_canv = gr_create_canvas(screen_canv->cv_bitmap.bm_w,screen_canv->cv_bitmap.bm_h);
	gr_set_current_canvas(temp_canv);
	gr_ubitmap(screen_canv->cv_bitmap);

	gr_set_current_canvas(save_canv);

	do
	{
		sprintf(savename, "%sscrn%04d.pcx",SCRNS_DIR, savenum++);
	} while (PHYSFSX_exists(savename,0));

	gr_set_current_canvas(NULL);

	if (!automap_flag)
		HUD_init_message(HM_DEFAULT, "%s 'scrn%04d.pcx'", TXT_DUMPING_SCREEN, savenum-1 );

	gr_palette_read(pal);		//get actual palette from the hardware
	pcx_write_bitmap(savename,&temp_canv->cv_bitmap,pal);
	gr_set_current_canvas(screen_canv);
	gr_ubitmap(temp_canv->cv_bitmap);
	gr_set_current_canvas(save_canv);

	start_time();
}

#endif

//initialize flying
void fly_init(const vobjptr_t obj)
{
	obj->control_type = CT_FLYING;
	obj->movement_type = MT_PHYSICS;

	vm_vec_zero(obj->mtype.phys_info.velocity);
	vm_vec_zero(obj->mtype.phys_info.thrust);
	vm_vec_zero(obj->mtype.phys_info.rotvel);
	vm_vec_zero(obj->mtype.phys_info.rotthrust);
}

//	------------------------------------------------------------------------------------
static void do_cloak_stuff(void)
{
	for (int i = 0; i < N_players; i++)
		if (Players[i].flags & PLAYER_FLAGS_CLOAKED) {
			if (GameTime64 > Players[i].cloak_time+CLOAK_TIME_MAX)
			{
				Players[i].flags &= ~PLAYER_FLAGS_CLOAKED;
				if (i == Player_num) {
					digi_play_sample( SOUND_CLOAK_OFF, F1_0);
					if (Game_mode & GM_MULTI)
						multi_send_play_sound(SOUND_CLOAK_OFF, F1_0);
					maybe_drop_net_powerup(POW_CLOAK);
					if ( Newdemo_state != ND_STATE_PLAYBACK )
						multi_send_decloak(); // For demo recording
				}
			}
		}
}

int FakingInvul=0;

//	------------------------------------------------------------------------------------
static void do_invulnerable_stuff(void)
{
	if (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE) {
		if (GameTime64 > Players[Player_num].invulnerable_time+INVULNERABLE_TIME_MAX)
		{
			Players[Player_num].flags ^= PLAYER_FLAGS_INVULNERABLE;
			if (FakingInvul==0)
			{
				digi_play_sample( SOUND_INVULNERABILITY_OFF, F1_0);
				if (Game_mode & GM_MULTI)
				{
					multi_send_play_sound(SOUND_INVULNERABILITY_OFF, F1_0);
					maybe_drop_net_powerup(POW_INVULNERABILITY);
				}
			}
			FakingInvul=0;
		}
	}
}

#if defined(DXX_BUILD_DESCENT_I)
static inline void do_afterburner_stuff()
{
}
#elif defined(DXX_BUILD_DESCENT_II)
ubyte	Last_afterburner_state = 0;
fix Last_afterburner_charge = 0;
fix64	Time_flash_last_played;

#define AFTERBURNER_LOOP_START	((GameArg.SndDigiSampleRate==SAMPLE_RATE_22K)?32027:(32027/2))		//20098
#define AFTERBURNER_LOOP_END		((GameArg.SndDigiSampleRate==SAMPLE_RATE_22K)?48452:(48452/2))		//25776

int	Ab_scale = 4;

static void do_afterburner_stuff(void)
{
	static sbyte func_play = 0;

	if (!(Players[Player_num].flags & PLAYER_FLAGS_AFTERBURNER))
		Afterburner_charge = 0;

	const auto plobj = vcobjptridx(Players[Player_num].objnum);
	if (Endlevel_sequence || Player_is_dead)
	{
		digi_kill_sound_linked_to_object(plobj);
		if (Game_mode & GM_MULTI && func_play)
		{
			multi_send_sound_function (0,0);
			func_play = 0;
		}
	}

	if ((Controls.state.afterburner != Last_afterburner_state && Last_afterburner_charge) || (Last_afterburner_state && Last_afterburner_charge && !Afterburner_charge)) {
		if (Afterburner_charge && Controls.state.afterburner && (Players[Player_num].flags & PLAYER_FLAGS_AFTERBURNER)) {
			digi_link_sound_to_object3( SOUND_AFTERBURNER_IGNITE, plobj, 1, F1_0, i2f(256), AFTERBURNER_LOOP_START, AFTERBURNER_LOOP_END );
			if (Game_mode & GM_MULTI)
			{
				multi_send_sound_function (3,SOUND_AFTERBURNER_IGNITE);
				func_play = 1;
			}
		} else {
			digi_kill_sound_linked_to_object(plobj);
			digi_link_sound_to_object2(SOUND_AFTERBURNER_PLAY, plobj, 0, F1_0, i2f(256));
			if (Game_mode & GM_MULTI)
			{
			 	multi_send_sound_function (0,0);
				func_play = 0;
			}
		}
	}

	//@@if (Controls.state.afterburner && Afterburner_charge)
	//@@	afterburner_shake();

	Last_afterburner_state = Controls.state.afterburner;
	Last_afterburner_charge = Afterburner_charge;
}
#endif

//	Amount to diminish guns towards normal, per second.
#define	DIMINISH_RATE 16 // gots to be a power of 2, else change the code in diminish_palette_towards_normal

 //adds to rgb values for palette flash
void PALETTE_FLASH_ADD(int _dr, int _dg, int _db)
{
	int	maxval;

	PaletteRedAdd += _dr;
	PaletteGreenAdd += _dg;
	PaletteBlueAdd += _db;

#if defined(DXX_BUILD_DESCENT_II)
	if (Flash_effect)
		maxval = 60;
	else
#endif
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

static void diminish_palette_color_toward_zero(int& palette_color_add, const int& dec_amount)
{
	if (palette_color_add > 0 ) {
		if (palette_color_add < dec_amount)
			palette_color_add = 0;
		else
			palette_color_add -= dec_amount;
	} else if (palette_color_add < 0 ) {
		if (palette_color_add > -dec_amount )
			palette_color_add = 0;
		else
			palette_color_add += dec_amount;
	}
}

//	------------------------------------------------------------------------------------
//	Diminish palette effects towards normal.
static void diminish_palette_towards_normal(void)
{
	int	dec_amount = 0;
	float brightness_correction = 1-((float)gr_palette_get_gamma()/64); // to compensate for brightness setting of the game

	// Diminish at DIMINISH_RATE units/second.
	if (FrameTime < (F1_0/DIMINISH_RATE))
	{
		static fix diminish_timer = 0;
		diminish_timer += FrameTime;
		if (diminish_timer >= (F1_0/DIMINISH_RATE))
		{
			diminish_timer -= (F1_0/DIMINISH_RATE);
			dec_amount = 1;
		}
	}
	else
	{
		dec_amount = f2i(FrameTime*DIMINISH_RATE); // one second = DIMINISH_RATE counts
		if (dec_amount == 0)
			dec_amount++; // make sure we decrement by something
	}

#if defined(DXX_BUILD_DESCENT_II)
	if (Flash_effect) {
		int	force_do = 0;
		static fix Flash_step_up_timer = 0;

		// Part of hack system to force update of palette after exiting a menu.
		if (Time_flash_last_played) {
			force_do = 1;
			PaletteRedAdd ^= 1; // Very Tricky! In gr_palette_step_up, if all stepups same as last time, won't do anything!
		}

		if (Time_flash_last_played + F1_0/8 < GameTime64) {
			digi_play_sample( SOUND_CLOAK_OFF, Flash_effect/4);
			Time_flash_last_played = GameTime64;
		}

		Flash_effect -= FrameTime;
		Flash_step_up_timer += FrameTime;
		if (Flash_effect < 0)
			Flash_effect = 0;

		if (force_do || (Flash_step_up_timer >= F1_0/26)) // originally time interval based on (d_rand() > 4096)
		{
			Flash_step_up_timer -= (F1_0/26);
			if ( (Newdemo_state==ND_STATE_RECORDING) && (PaletteRedAdd || PaletteGreenAdd || PaletteBlueAdd) )
				newdemo_record_palette_effect(PaletteRedAdd, PaletteGreenAdd, PaletteBlueAdd);

			gr_palette_step_up( PaletteRedAdd*brightness_correction, PaletteGreenAdd*brightness_correction, PaletteBlueAdd*brightness_correction );

			return;
		}

	}
#endif

	diminish_palette_color_toward_zero(PaletteRedAdd, dec_amount);
	diminish_palette_color_toward_zero(PaletteGreenAdd, dec_amount);
	diminish_palette_color_toward_zero(PaletteBlueAdd, dec_amount);

	if ( (Newdemo_state==ND_STATE_RECORDING) && (PaletteRedAdd || PaletteGreenAdd || PaletteBlueAdd) )
		newdemo_record_palette_effect(PaletteRedAdd, PaletteGreenAdd, PaletteBlueAdd);

	gr_palette_step_up( PaletteRedAdd*brightness_correction, PaletteGreenAdd*brightness_correction, PaletteBlueAdd*brightness_correction );
}

int	Redsave, Bluesave, Greensave;

void palette_save(void)
{
	Redsave = PaletteRedAdd; Bluesave = PaletteBlueAdd; Greensave = PaletteGreenAdd;
}

void palette_restore(void)
{
	float brightness_correction = 1-((float)gr_palette_get_gamma()/64);

	PaletteRedAdd = Redsave; PaletteBlueAdd = Bluesave; PaletteGreenAdd = Greensave;
	gr_palette_step_up( PaletteRedAdd*brightness_correction, PaletteGreenAdd*brightness_correction, PaletteBlueAdd*brightness_correction );

#if defined(DXX_BUILD_DESCENT_II)
	//	Forces flash effect to fixup palette next frame.
	Time_flash_last_played = 0;
#endif
}

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

#if defined(DXX_BUILD_DESCENT_II)
	if (Players[Player_num].energy < Weapon_info[FLARE_ID].energy_usage)
#define	FLARE_BIG_DELAY	(F1_0*2)
		Next_flare_fire_time = GameTime64 + FLARE_BIG_DELAY;
	else
#endif
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

#if defined(DXX_BUILD_DESCENT_II)
void full_palette_save(void)
{
	palette_save();
	reset_palette_add();
	gr_palette_load( gr_palette );
}
#endif

#ifdef USE_SDLMIXER
#define EXT_MUSIC_TEXT "Jukebox/Audio CD"
#else
#define EXT_MUSIC_TEXT "Audio CD"
#endif

static int free_help(newmenu *menu,const d_event &event, const unused_newmenu_userdata_t *)
{
	if (event.type == EVENT_WINDOW_CLOSE)
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

	nm_set_item_text(& m[nitems++], TXT_HELP_ESC);
	nm_set_item_text(& m[nitems++], "SHIFT-ESC\t  SHOW GAME LOG");
	nm_set_item_text(& m[nitems++], "F1\t  THIS SCREEN");
	nm_set_item_text(& m[nitems++], TXT_HELP_F2);
#if !(defined(__APPLE__) || defined(macintosh))
	nm_set_item_text(& m[nitems++], "Alt-F2/F3\t  SAVE/LOAD GAME");
	nm_set_item_text(& m[nitems++], "Alt-Shift-F2/F3\t  Fast Save");
#else
	nm_set_item_text(& m[nitems++], "Alt-F2/F3 (\x85-SHIFT-s/o)\t  SAVE/LOAD GAME");
	nm_set_item_text(& m[nitems++], "Alt-Shift-F2/F3 (\x85-s/o)\t  Quick Save/Load");
#endif
	nm_set_item_text(& m[nitems++], "F3\t  SWITCH COCKPIT MODES");
#if defined(DXX_BUILD_DESCENT_II)
	nm_set_item_text(& m[nitems++], TXT_HELP_F4);
#endif
	nm_set_item_text(& m[nitems++], TXT_HELP_F5);
	nm_set_item_text(& m[nitems++], "ALT-F7\t  SWITCH HUD MODES");
#if !(defined(__APPLE__) || defined(macintosh))
	nm_set_item_text(& m[nitems++], TXT_HELP_PAUSE);
#else
	nm_set_item_text(& m[nitems++], "Pause (\x85-P)\t  Pause");
#endif
	nm_set_item_text(& m[nitems++], TXT_HELP_PRTSCN);
	nm_set_item_text(& m[nitems++], TXT_HELP_1TO5);
	nm_set_item_text(& m[nitems++], TXT_HELP_6TO10);
#if defined(DXX_BUILD_DESCENT_II)
	nm_set_item_text(& m[nitems++], "Shift-F1/F2\t  Cycle left/right window");
	nm_set_item_text(& m[nitems++], "Shift-F4\t  GuideBot menu");
	nm_set_item_text(& m[nitems++], "Alt-Shift-F4\t  Rename GuideBot");
	nm_set_item_text(& m[nitems++], "Shift-F5/F6\t  Drop primary/secondary");
	nm_set_item_text(& m[nitems++], "Shift-number\t  GuideBot commands");
#endif
#if !(defined(__APPLE__) || defined(macintosh))
	nm_set_item_text(& m[nitems++], "Alt-Shift-F9\t  Eject Audio CD");
	nm_set_item_text(& m[nitems++], "Alt-Shift-F10\t  Play/Pause " EXT_MUSIC_TEXT);
	nm_set_item_text(& m[nitems++], "Alt-Shift-F11/F12\t  Previous/Next Song");
#else
	nm_set_item_text(& m[nitems++], "\x85-E\t  Eject Audio CD");
	nm_set_item_text(& m[nitems++], "\x85-Up/Down\t  Play/Pause " EXT_MUSIC_TEXT);
	nm_set_item_text(& m[nitems++], "\x85-Left/Right\t  Previous/Next Song");
#endif
#if (defined(__APPLE__) || defined(macintosh))
	nm_set_item_text(& m[nitems++], "");
	nm_set_item_text(& m[nitems++], "(Use \x85-# for F#. e.g. \x85-1 for F1)");
#endif

	newmenu_dotiny( NULL, TXT_KEYS, nitems, m, 0, free_help, unused_newmenu_userdata );
}

void show_netgame_help()
{
	int nitems = 0;
	newmenu_item *m;

	MALLOC(m, newmenu_item, 17);
	if (!m)
		return;

	nm_set_item_text(& m[nitems++], "F1\t  THIS SCREEN");
#if defined(DXX_BUILD_DESCENT_II)
	nm_set_item_text(& m[nitems++], "ALT-0\t  DROP FLAG");
#endif
#if !(defined(__APPLE__) || defined(macintosh))
	nm_set_item_text(& m[nitems++], "Alt-F2/F3\t  SAVE/LOAD COOP GAME");
#else
	nm_set_item_text(& m[nitems++], "Alt-F2/F3 (\x85-SHIFT-s/\x85-o)\t  SAVE/LOAD COOP GAME");
#endif
	nm_set_item_text(& m[nitems++], "ALT-F4\t  SHOW PLAYER NAMES ON HUD");
	nm_set_item_text(& m[nitems++], "F7\t  TOGGLE KILL LIST");
	nm_set_item_text(& m[nitems++], "F8\t  SEND MESSAGE");
	nm_set_item_text(& m[nitems++], "(SHIFT-)F9 to F12\t  (DEFINE)SEND MACRO");
	nm_set_item_text(& m[nitems++], "PAUSE\t  SHOW NETGAME INFORMATION");

#if (defined(__APPLE__) || defined(macintosh))
	nm_set_item_text(& m[nitems++], "");
	nm_set_item_text(& m[nitems++], "(Use \x85-# for F#. e.g. \x85-1 for F1)");
#endif

	nm_set_item_text(& m[nitems++], "");
	nm_set_item_text(& m[nitems++], "MULTIPLAYER MESSAGE COMMANDS:");
	nm_set_item_text(& m[nitems++], "(*): TEXT\t  SEND TEXT TO PLAYER/TEAM (*)");
	nm_set_item_text(& m[nitems++], "/Handicap: (*)\t  SET YOUR STARTING SHIELDS TO (*) [10-100]");
	nm_set_item_text(& m[nitems++], "/move: (*)\t  MOVE PLAYER (*) TO OTHER TEAM (Host-only)");
	nm_set_item_text(& m[nitems++], "/kick: (*)\t  KICK PLAYER (*) FROM GAME (Host-only)");
	nm_set_item_text(& m[nitems++], "/KillReactor\t  BLOW UP THE MINE (Host-only)");

	newmenu_dotiny( NULL, TXT_KEYS, nitems, m, 0, free_help, unused_newmenu_userdata );
}

void show_newdemo_help()
{
	newmenu_item *m;
	int nitems = 0;

	MALLOC(m, newmenu_item, 15);
	if (!m)
		return;

	nm_set_item_text(& m[nitems++], "ESC\t  QUIT DEMO PLAYBACK");
	nm_set_item_text(& m[nitems++], "F1\t  THIS SCREEN");
	nm_set_item_text(& m[nitems++], TXT_HELP_F2);
	nm_set_item_text(& m[nitems++], "F3\t  SWITCH COCKPIT MODES");
	nm_set_item_text(& m[nitems++], "F4\t  TOGGLE PERCENTAGE DISPLAY");
	nm_set_item_text(& m[nitems++], "UP\t  PLAY");
	nm_set_item_text(& m[nitems++], "DOWN\t  PAUSE");
	nm_set_item_text(& m[nitems++], "RIGHT\t  ONE FRAME FORWARD");
	nm_set_item_text(& m[nitems++], "LEFT\t  ONE FRAME BACKWARD");
	nm_set_item_text(& m[nitems++], "SHIFT-RIGHT\t  FAST FORWARD");
	nm_set_item_text(& m[nitems++], "SHIFT-LEFT\t  FAST BACKWARD");
	nm_set_item_text(& m[nitems++], "CTRL-RIGHT\t  JUMP TO END");
	nm_set_item_text(& m[nitems++], "CTRL-LEFT\t  JUMP TO START");
#if (defined(__APPLE__) || defined(macintosh))
	nm_set_item_text(& m[nitems++], "");
	nm_set_item_text(& m[nitems++], "(Use \x85-# for F#. e.g. \x85-1 for F1)");
#endif

	newmenu_dotiny( NULL, "DEMO PLAYBACK CONTROLS", nitems, m, 0, free_help, unused_newmenu_userdata );
}

#define LEAVE_TIME 0x4000		//how long until we decide key is down	(Used to be 0x4000)

//deal with rear view - switch it on, or off, or whatever
void check_rear_view()
{
	static int leave_mode;
	static fix64 entry_time;

	if (Newdemo_state == ND_STATE_PLAYBACK)
		return;

	if ( Controls.state.rear_view) {	//key/button has gone down
		Controls.state.rear_view = 0;

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
		if (Controls.state.rear_view) {

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

//turns off all cheats & resets cheater flag
void game_disable_cheats()
{
	cheats = {};
}

//	game_setup()
// ----------------------------------------------------------------------------

window *game_setup(void)
{
	window *game_wind;

	PlayerCfg.CockpitMode[1] = PlayerCfg.CockpitMode[0];
	last_drawn_cockpit = -1;	// Force cockpit to redraw next time a frame renders.
	Endlevel_sequence = 0;

	game_wind = window_create(&grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT, game_handler, unused_window_userdata);
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
	
	if (Segments[ConsoleObject->segnum].segnum == segment_none)      //segment no longer exists
		obj_relink( ConsoleObject-Objects, Cursegp );

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

window *Game_wind = NULL;

// Event handler for the game
window_event_result game_handler(window *,const d_event &event, const unused_window_userdata_t *)
{
	switch (event.type)
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
				full_palette_save();

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

			multi_leave_game();

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
			break;

		default:
			break;
	}
	return window_event_result::ignored;
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
	close_gauges();
	restore_effect_bitmap_icons();
}

#if defined(DXX_BUILD_DESCENT_II)
object *Missile_viewer=NULL;
int Missile_viewer_sig=-1;

int Marker_viewer_num[2]={-1,-1};
int Coop_view_player[2]={-1,-1};

//returns ptr to escort robot, or NULL
objptridx_t find_escort()
{
	range_for (auto i, highest_valid(Objects))
		if (Objects[i].type == OBJ_ROBOT)
			if (Robot_info[get_robot_id(&Objects[i])].companion)
				return &Objects[i];
	return object_none;
}

//if water or fire level, make occasional sound
static void do_ambient_sounds()
{
	int has_water,has_lava;
	int sound;

	const auto s2_flags = Segments[ConsoleObject->segnum].s2_flags;
	has_lava = (s2_flags & S2F_AMBIENT_LAVA);
	has_water = (s2_flags & S2F_AMBIENT_WATER);

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
#endif

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
	int player_was_dead = Player_is_dead;

	update_player_stats();
	diminish_palette_towards_normal();		//	Should leave palette effect up for as long as possible by putting right before render.
	do_afterburner_stuff();
	do_cloak_stuff();
	do_invulnerable_stuff();
	remove_obsolete_stuck_objects();
#if defined(DXX_BUILD_DESCENT_II)
	init_ai_frame();
	do_final_boss_frame();

	if ((Players[Player_num].flags & PLAYER_FLAGS_HEADLIGHT) && (Players[Player_num].flags & PLAYER_FLAGS_HEADLIGHT_ON)) {
		static int turned_off=0;
		Players[Player_num].energy -= (FrameTime*3/8);
		if (Players[Player_num].energy < i2f(10)) {
			if (!turned_off) {
				Players[Player_num].flags &= ~PLAYER_FLAGS_HEADLIGHT_ON;
				turned_off = 1;
				if (Game_mode & GM_MULTI)
					multi_send_flags(Player_num);
			}
		}
		else
			turned_off = 0;

		if (Players[Player_num].energy <= 0) {
			Players[Player_num].energy = 0;
			Players[Player_num].flags &= ~PLAYER_FLAGS_HEADLIGHT_ON;
			if (Game_mode & GM_MULTI)
				multi_send_flags(Player_num);
		}
	}
#endif

#ifdef EDITOR
	check_create_player_path();
	player_follow_path(ConsoleObject);
#endif

	if (Game_mode & GM_MULTI)
	{
		multi_do_frame();
		if (Netgame.PlayTimeAllowed && ThisLevelTime>=i2f((Netgame.PlayTimeAllowed*5*60)))
			multi_check_for_killgoal_winner();
	}

	dead_player_frame();
	if (Newdemo_state != ND_STATE_PLAYBACK)
		do_controlcen_dead_frame();

#if defined(DXX_BUILD_DESCENT_II)
	process_super_mines_frame();
	do_seismic_stuff();
	do_ambient_sounds();
#endif

	if ((Game_mode & GM_MULTI) && Netgame.PlayTimeAllowed)
		ThisLevelTime +=FrameTime;

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
			} else if (d_tick_step) {
				fix			bump_amount;

				Global_laser_firing_count = 0;

				ConsoleObject->mtype.phys_info.rotvel.x += (d_rand() - 16384)/8;
				ConsoleObject->mtype.phys_info.rotvel.z += (d_rand() - 16384)/8;

				const auto rand_vec = make_random_vector();

				bump_amount = F1_0*4;

				if (Fusion_charge > F1_0*2)
					bump_amount = Fusion_charge*4;

				bump_one_object(ConsoleObject, rand_vec, bump_amount);
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
		create_player_appearance_effect(vobjptridx(ConsoleObject));
		Do_appearance_effect = 0;
		if ((Game_mode & GM_MULTI) && Netgame.InvulAppear)
		{
			Players[Player_num].flags |= PLAYER_FLAGS_INVULNERABLE;
			Players[Player_num].invulnerable_time = GameTime64-i2f(27);
			FakingInvul=1;
		}
	}

#if defined(DXX_BUILD_DESCENT_II)
	omega_charge_frame();
	slide_textures();
	flicker_lights();

	//if the player is taking damage, give up guided missile control
	if (Players[Player_num].shields != player_shields)
		release_guided_missile(Player_num);
#endif

	// Check if we have to close in-game menus for multiplayer
	if ((Game_mode & GM_MULTI) && (Players[Player_num].connected == CONNECT_PLAYING))
	{
		if ( Endlevel_sequence || ((Control_center_destroyed) && (Countdown_seconds_left <= 1)) || // close menus when end of level...
			(Automap_active && ((Player_is_dead != player_was_dead) || (Players[Player_num].shields<=0 && player_shields>0))) ) // close autmap when dying ...
			game_leave_menus();
	}
}

#if defined(DXX_BUILD_DESCENT_II)
ubyte	Slide_segs[MAX_SEGMENTS];
int	Slide_segs_computed;

static void compute_slide_segs(void)
{
	range_for (auto segnum, highest_valid(Segments))
	{
		Slide_segs[segnum] = 0;
		for (int sidenum=0;sidenum<6;sidenum++) {
			int tmn = Segments[segnum].sides[sidenum].tmap_num;
			if (TmapInfo[tmn].slide_u != 0 || TmapInfo[tmn].slide_v != 0)
				Slide_segs[segnum] |= 1 << sidenum;
		}
	}

	Slide_segs_computed = 1;
}

//	-----------------------------------------------------------------------------
static void slide_textures(void)
{
	if (!Slide_segs_computed)
		compute_slide_segs();

	range_for (auto segnum, highest_valid(Segments))
	{
		if (Slide_segs[segnum]) {
			for (int sidenum=0;sidenum<6;sidenum++) {
				if (Slide_segs[segnum] & (1 << sidenum)) {
					int tmn = Segments[segnum].sides[sidenum].tmap_num;
					if (TmapInfo[tmn].slide_u != 0 || TmapInfo[tmn].slide_v != 0) {
						for (int i=0;i<4;i++) {
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

Flickering_light_array_t Flickering_lights;
unsigned Num_flickering_lights;

static void flicker_lights()
{
	range_for (auto &rf, partial_range(Flickering_lights, Num_flickering_lights))
	{
		auto f = &rf;
		segment *segp = &Segments[f->segnum];

		//make sure this is actually a light
		if (! (WALL_IS_DOORWAY(segp, f->sidenum) & WID_RENDER_FLAG))
			continue;
		if (! (TmapInfo[segp->sides[f->sidenum].tmap_num].lighting | TmapInfo[segp->sides[f->sidenum].tmap_num2 & 0x3fff].lighting))
			continue;

		if (static_cast<unsigned>(f->timer) == 0x80000000)		//disabled
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
static std::pair<Flickering_light_array_t::iterator, Flickering_light_array_t::iterator> find_flicker(segnum_t segnum, int sidenum)
{
	//see if there's already an entry for this seg/side
	auto pr = partial_range(Flickering_lights, Num_flickering_lights);
	auto predicate = [segnum, sidenum](const flickering_light &f) {
		return f.segnum == segnum && f.sidenum == sidenum;	//found it!
	};
	return {std::find_if(pr.begin(), pr.end(), predicate), pr.end()};
}

template <typename F>
static inline void update_flicker(segnum_t segnum, int sidenum, F f)
{
	auto i = find_flicker(segnum, sidenum);
	if (i.first != i.second)
		f(*i.first);
}

//turn flickering off (because light has been turned off)
void disable_flicker(segnum_t segnum,int sidenum)
{
	auto F = [](flickering_light &f) {
		f.timer = 0x80000000;
	};
	update_flicker(segnum, sidenum, F);
}

//turn flickering off (because light has been turned on)
void enable_flicker(segnum_t segnum,int sidenum)
{
	auto F = [](flickering_light &f) {
		f.timer = 0;
	};
	update_flicker(segnum, sidenum, F);
}
#endif

//	-----------------------------------------------------------------------------
//	Fire Laser:  Registers a laser fire, and performs special stuff for the fusion
//				    cannon.
void FireLaser()
{

	Global_laser_firing_count = Controls.state.fire_primary?Weapon_info[Primary_weapon_to_weapon_info[Primary_weapon]].fire_count:0;

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
#if defined(DXX_BUILD_DESCENT_I)
					if(Game_mode & GM_MULTI)
						multi_send_play_sound(11, F1_0);
#endif
					const auto cobjp = vobjptridx(ConsoleObject);
					apply_damage_to_player(cobjp, cobjp, d_rand() * 4, 0);
				} else {
					create_awareness_event(ConsoleObject, PA_WEAPON_ROBOT_COLLISION);
					digi_play_sample( SOUND_FUSION_WARMUP, F1_0 );
					if (Game_mode & GM_MULTI)
						multi_send_play_sound(SOUND_FUSION_WARMUP, F1_0);
				}
				Fusion_next_sound_time = GameTime64 + F1_0/8 + d_rand()/4;
			}
		}
	}
}


//	-------------------------------------------------------------------------------------------------------
//	If player is close enough to objnum, which ought to be a powerup, pick it up!
//	This could easily be made difficulty level dependent.
static void powerup_grab_cheat(const vobjptr_t player, const vobjptridx_t powerup)
{
	fix	powerup_size;
	fix	player_size;
	fix	dist;

	Assert(powerup->type == OBJ_POWERUP);

	powerup_size = powerup->size;
	player_size = player->size;

	dist = vm_vec_dist_quick(powerup->pos, player->pos);

	if ((dist < 2*(powerup_size + player_size)) && !(powerup->flags & OF_SHOULD_BE_DEAD)) {
		const auto collision_point = vm_vec_avg(powerup->pos, player->pos);
		collide_player_and_powerup(player, powerup, collision_point);
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
	auto segp = &Segments[ConsoleObject->segnum];
	range_for (auto objnum, objects_in(*segp))
		if (objnum->type == OBJ_POWERUP)
			powerup_grab_cheat(ConsoleObject, objnum);
}

int	Last_level_path_created = -1;

#ifdef SHOW_EXIT_PATH

//	------------------------------------------------------------------------------------------------------------------
//	Create path for player from current segment to goal segment.
//	Return true if path created, else return false.
static int mark_player_path_to_segment(segnum_t segnum)
{
	short		player_path_length=0;
	int		player_hide_index=-1;

	if (Last_level_path_created == Current_level_num) {
		return 0;
	}

	Last_level_path_created = Current_level_num;

	auto objp = vobjptridx(ConsoleObject);
	if (create_path_points(objp, objp->segnum, segnum, Point_segs_free_ptr, &player_path_length, 100, 0, 0, segment_none) == -1) {
		return 0;
	}

	player_hide_index = Point_segs_free_ptr - Point_segs;
	Point_segs_free_ptr += player_path_length;

	if (Point_segs_free_ptr - Point_segs + MAX_PATH_LENGTH*2 > MAX_POINT_SEGS) {
		ai_reset_all_paths();
		return 0;
	}

	for (int i=1; i<player_path_length; i++) {
		vms_vector	seg_center;

		auto segnum = Point_segs[player_hide_index+i].segnum;
		seg_center = Point_segs[player_hide_index+i].point;

		auto obj = obj_create( OBJ_POWERUP, POW_ENERGY, segnum, seg_center, &vmd_identity_matrix, Powerup_info[POW_ENERGY].size, CT_POWERUP, MT_NONE, RT_POWERUP);
		if (obj == object_none) {
			Int3();		//	Unable to drop energy powerup for path
			return 1;
		}

		obj->rtype.vclip_info.vclip_num = Powerup_info[get_powerup_id(obj)].vclip_num;
		obj->rtype.vclip_info.frametime = Vclip[obj->rtype.vclip_info.vclip_num].frame_time;
		obj->rtype.vclip_info.framenum = 0;
		obj->lifeleft = F1_0*100 + d_rand() * 4;
	}

	return 1;
}

//	Return true if it happened, else return false.
int create_special_path(void)
{
	//	---------- Find exit doors ----------
	range_for (auto i, highest_valid(Segments))
		for (int j=0; j<MAX_SIDES_PER_SEGMENT; j++)
			if (Segments[i].children[j] == segment_exit) {
				return mark_player_path_to_segment(i);
			}

	return 0;
}

#endif


#if defined(DXX_BUILD_DESCENT_II)
/*
 * reads a flickering_light structure from a PHYSFS_file
 */
void flickering_light_read(flickering_light *fl, PHYSFS_file *fp)
{
	fl->segnum = PHYSFSX_readShort(fp);
	fl->sidenum = PHYSFSX_readShort(fp);
	fl->mask = PHYSFSX_readInt(fp);
	fl->timer = PHYSFSX_readFix(fp);
	fl->delay = PHYSFSX_readFix(fp);
}

void flickering_light_write(flickering_light *fl, PHYSFS_file *fp)
{
	PHYSFS_writeSLE16(fp, fl->segnum);
	PHYSFS_writeSLE16(fp, fl->sidenum);
	PHYSFS_writeULE32(fp, fl->mask);
	PHYSFSX_writeFix(fp, fl->timer);
	PHYSFSX_writeFix(fp, fl->delay);
}
#endif
