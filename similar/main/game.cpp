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
#include "player.h"
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
#include "partial_range.h"
#include "segiter.h"

static fix64 last_timer_value=0;
fix ThisLevelTime=0;

grs_canvas	Screen_3d_window;							// The rectangle for rendering the mine to

namespace dcx {
int	force_cockpit_redraw=0;
int	PaletteRedAdd, PaletteGreenAdd, PaletteBlueAdd;

int	Game_suspended=0; //if non-zero, nothing moves but player
fix64	Auto_fire_fusion_cannon_time = 0;
fix	Fusion_charge = 0;
int	Game_mode = GM_GAME_OVER;
int	Global_laser_firing_count = 0;
int	Global_missile_firing_count = 0;
}

//	Function prototypes for GAME.C exclusively.

namespace dsx {
static void GameProcessFrame(void);
static void FireLaser();
static void powerup_grab_cheat_all();

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
	Clear_window = 2;		//	do portal only window clear.
}

}

namespace dcx {

void reset_palette_add()
{
	PaletteRedAdd 		= 0;
	PaletteGreenAdd	= 0;
	PaletteBlueAdd		= 0;
}

screen_mode Game_screen_mode{640, 480};

}

namespace dsx {

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
	if (PlayerCfg.CockpitMode[1] != CM_LETTERBOX)
	{
#if defined(DXX_BUILD_DESCENT_II)
		int HiresGFXAvailable = !GameArg.GfxSkipHiresGFX;
#endif
		auto full_screen_mode = HiresGFXAvailable ? screen_mode{640, 480} : screen_mode{320, 200};
		if (Game_screen_mode != full_screen_mode) {
			PlayerCfg.CockpitMode[1] = CM_FULL_SCREEN;
		}
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
			game_init_render_sub_buffers(x1*(static_cast<float>(SWIDTH)/bm.bm_w), y1*(static_cast<float>(SHEIGHT)/bm.bm_h), (x2-x1+1)*(static_cast<float>(SWIDTH)/bm.bm_w), (y2-y1+2)*(static_cast<float>(SHEIGHT)/bm.bm_h));
			break;
		}

		case CM_FULL_SCREEN:
			game_init_render_sub_buffers(0, 0, SWIDTH, SHEIGHT);
			break;

		case CM_STATUS_BAR:
			game_init_render_sub_buffers( 0, 0, SWIDTH, (HIRESMODE?(SHEIGHT*2)/2.6:(SHEIGHT*2)/2.72) );
			break;

		case CM_LETTERBOX:	{
			const unsigned gsm_height = SM_H(Game_screen_mode);
			const unsigned w = SM_W(Game_screen_mode);
			const unsigned h = (gsm_height * 3) / 4; // true letterbox size (16:9)
			const unsigned x = 0;
			const unsigned y = (gsm_height - h) / 2;

			const uint8_t color = 0;
			gr_rect(x, 0, w, gsm_height - h, color);
			gr_rect(x, gsm_height - h, w, gsm_height, color);

			game_init_render_sub_buffers( x, y, w, h );
			break;
		}
	}

	gr_set_current_canvas(NULL);
}

}

//selects a given cockpit (or lack of one).  See types in game.h
void select_cockpit(cockpit_mode_t mode)
{
	if (mode != PlayerCfg.CockpitMode[1]) {		//new mode
		PlayerCfg.CockpitMode[1]=mode;
		init_cockpit();
	}
}

namespace dcx {

//force cockpit redraw next time. call this if you've trashed the screen
void reset_cockpit()
{
	force_cockpit_redraw=1;
	last_drawn_cockpit = -1;
}

void game_init_render_sub_buffers( int x, int y, int w, int h )
{
	gr_clear_canvas(0);
	gr_init_sub_canvas(Screen_3d_window, grd_curscreen->sc_canvas, x, y, w, h);
}

}

namespace dsx {

//called to change the screen mode. Parameter sm is the new mode, one of
//SMODE_GAME or SMODE_EDITOR. returns mode acutally set (could be other
//mode if cannot init requested mode)
int set_screen_mode(int sm)
{
	if ( (Screen_mode == sm) && !((sm==SCREEN_GAME) && (grd_curscreen->get_screen_mode() != Game_screen_mode)) && !(sm==SCREEN_MENU) )
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
			if  (grd_curscreen->get_screen_mode() != Game_screen_mode)
				if (gr_set_mode(Game_screen_mode))
					Error("Cannot set screen mode.");
			break;

		case SCREEN_GAME:
			if  (grd_curscreen->get_screen_mode() != Game_screen_mode)
				if (gr_set_mode(Game_screen_mode))
					Error("Cannot set screen mode.");
			break;
#ifdef EDITOR
		case SCREEN_EDITOR:
		{
			const screen_mode editor_mode{800, 600};
			if (grd_curscreen->get_screen_mode() != editor_mode)
			{
				int gr_error;
				if ((gr_error = gr_set_mode(editor_mode)) != 0) { //force into game scrren
					Warning("Cannot init editor screen (error=%d)",gr_error);
					return 0;
				}
			}
		}
			break;
#endif
#if defined(DXX_BUILD_DESCENT_II)
		case SCREEN_MOVIE:
		{
			const screen_mode movie_mode{MOVIE_WIDTH, MOVIE_HEIGHT};
			if (grd_curscreen->get_screen_mode() != movie_mode)
			{
				if (gr_set_mode(movie_mode))
					Error("Cannot set screen mode for game!");
				gr_palette_load( gr_palette );
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

}

namespace dcx {

namespace {

class game_world_time_paused
{
	unsigned time_paused;
public:
	explicit operator bool() const
	{
		return time_paused;
	}
	void increase_pause_count();
	void decrease_pause_count();
};

}

static game_world_time_paused time_paused;

void game_world_time_paused::increase_pause_count()
{
	if (time_paused==0) {
		const fix64 time = timer_update();
		last_timer_value = time - last_timer_value;
		if (last_timer_value < 0) {
			last_timer_value = 0;
		}
	}
	time_paused++;
}

void game_world_time_paused::decrease_pause_count()
{
	Assert(time_paused > 0);
	--time_paused;
	if (time_paused==0) {
		const fix64 time = timer_update();
		last_timer_value = time - last_timer_value;
	}
}

void start_time()
{
	time_paused.decrease_pause_count();
}

void stop_time()
{
	time_paused.increase_pause_count();
}

pause_game_world_time::pause_game_world_time()
{
	stop_time();
}

pause_game_world_time::~pause_game_world_time()
{
	start_time();
}

static void game_flush_common_inputs()
{
	event_flush();
	key_flush();
	joy_flush();
	mouse_flush();
}

}

namespace dsx {

void game_flush_inputs()
{
	Controls = {};
	game_flush_common_inputs();
}

}

namespace dcx {

void game_flush_respawn_inputs()
{
	static_cast<control_info::fire_controls_t &>(Controls.state) = {};
	game_flush_common_inputs();
}

/*
 * timer that every sets d_tick_step true and increments d_tick_count every 1000/DESIGNATED_GAME_FPS ms.
 */
void calc_d_tick()
{
	static fix timer = 0;
	auto t = timer + FrameTime;

	d_tick_step = t >= DESIGNATED_GAME_FRAMETIME;
	if (d_tick_step)
	{
		d_tick_count++;
		if (d_tick_count > 1000000)
			d_tick_count = 0;
		t -= DESIGNATED_GAME_FRAMETIME;
	}
	timer = t;
}

void reset_time()
{
	last_timer_value = timer_update();
}

}

void calc_frame_time()
{
	fix last_frametime = FrameTime;

	const auto vsync = CGameCfg.VSync;
	const auto bound = f1_0 / (likely(vsync) ? MAXIMUM_FPS : CGameArg.SysMaxFPS);
	const auto may_sleep = !CGameArg.SysNoNiceFPS && !vsync;
	for (;;)
	{
		const auto timer_value = timer_update();
		FrameTime = timer_value - last_timer_value;
		if (FrameTime >= bound)
		{
			last_timer_value = timer_value;
			break;
		}
		if (Game_mode & GM_MULTI)
			multi_do_frame(); // during long wait, keep packets flowing
		if (may_sleep)
			timer_delay_ms(1);
	}

	if ( cheats.turbo )
		FrameTime *= 2;

	if (FrameTime < 0)				//if bogus frametime...
		FrameTime = (last_frametime==0?1:last_frametime);		//...then use time from last frame

	GameTime64 += FrameTime;

	calc_d_tick();
#ifdef NEWHOMER
        calc_d_homer_tick();
#endif
}

namespace dsx {

void move_player_2_segment(const vsegptridx_t seg,int side)
{
	const auto &&console = vobjptridx(ConsoleObject);
	compute_segment_center(console->pos,seg);
	auto vp = compute_center_point_on_side(seg,side);
	vm_vec_sub2(vp, console->pos);
	vm_vector_2_matrix(console->orient, vp, nullptr, nullptr);
	obj_relink(console, seg );
}

}

#ifndef OGL
namespace dcx {
void save_screen_shot(int automap_flag)
{
	grs_canvas *screen_canv=&grd_curscreen->sc_canvas;
	static int savenum=0;
	grs_canvas *save_canv;
        char savename[FILENAME_LEN+sizeof(SCRNS_DIR)];
	palette_array_t pal;

	pause_game_world_time p;

	if (!PHYSFSX_exists(SCRNS_DIR,0))
		PHYSFS_mkdir(SCRNS_DIR); //try making directory

	save_canv = grd_curcanv;
	auto temp_canv = gr_create_canvas(screen_canv->cv_bitmap.bm_w,screen_canv->cv_bitmap.bm_h);
	gr_set_current_canvas(temp_canv);
	gr_ubitmap(screen_canv->cv_bitmap);

	gr_set_current_canvas(save_canv);

	do
	{
		snprintf(savename, sizeof(savename), "%sscrn%04d.pcx",SCRNS_DIR, savenum++);
                if (savenum >= 9999) break; // that's enough I think.
	} while (PHYSFSX_exists(savename,0));

	gr_set_current_canvas(NULL);

	if (!automap_flag)
		HUD_init_message(HM_DEFAULT, "%s 'scrn%04d.pcx'", TXT_DUMPING_SCREEN, savenum-1 );

	gr_palette_read(pal);		//get actual palette from the hardware
	pcx_write_bitmap(savename,&temp_canv->cv_bitmap,pal);
	gr_set_current_canvas(screen_canv);
	gr_ubitmap(temp_canv->cv_bitmap);
	gr_set_current_canvas(save_canv);
}

}
#endif

namespace dcx {

//initialize flying
void fly_init(object_base &obj)
{
	obj.control_type = CT_FLYING;
	obj.movement_type = MT_PHYSICS;

	obj.mtype.phys_info.velocity = {};
	obj.mtype.phys_info.thrust = {};
	obj.mtype.phys_info.rotvel = {};
	obj.mtype.phys_info.rotthrust = {};
}

}

namespace dsx {

//	------------------------------------------------------------------------------------
static void do_cloak_stuff(void)
{
	for (int i = 0; i < N_players; i++)
	{
		const auto &&plobj = vobjptr(Players[i].objnum);
		auto &player_info = plobj->ctype.player_info;
		auto &pl_flags = player_info.powerup_flags;
		if (pl_flags & PLAYER_FLAGS_CLOAKED)
		{
			if (GameTime64 > player_info.cloak_time+CLOAK_TIME_MAX)
			{
				pl_flags &= ~PLAYER_FLAGS_CLOAKED;
				if (i == Player_num) {
					multi_digi_play_sample(SOUND_CLOAK_OFF, F1_0);
					maybe_drop_net_powerup(POW_CLOAK, 1, 0);
					if ( Newdemo_state != ND_STATE_PLAYBACK )
						multi_send_decloak(); // For demo recording
				}
			}
		}
	}
}

}

//	------------------------------------------------------------------------------------
static void do_invulnerable_stuff(void)
{
	auto &plobj = get_local_plrobj();
	auto &player_info = plobj.ctype.player_info;
	auto &pl_flags = player_info.powerup_flags;
	if (pl_flags & PLAYER_FLAGS_INVULNERABLE)
	{
		if (GameTime64 > player_info.invulnerable_time + INVULNERABLE_TIME_MAX)
		{
			pl_flags &= ~PLAYER_FLAGS_INVULNERABLE;
			if (auto &FakingInvul = player_info.FakingInvul)
			{
				FakingInvul = 0;
				return;
			}
				multi_digi_play_sample(SOUND_INVULNERABILITY_OFF, F1_0);
				if (Game_mode & GM_MULTI)
				{
					maybe_drop_net_powerup(POW_INVULNERABILITY, 1, 0);
				}
		}
	}
}

namespace dsx {
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

static void do_afterburner_stuff(void)
{
	static sbyte func_play = 0;

	const auto have_afterburner = get_local_player_flags() & PLAYER_FLAGS_AFTERBURNER;
	if (!have_afterburner)
		Afterburner_charge = 0;

	const auto plobj = vcobjptridx(get_local_player().objnum);
	if (Endlevel_sequence || Player_dead_state != player_dead_state::no)
	{
		digi_kill_sound_linked_to_object(plobj);
		if (Game_mode & GM_MULTI && func_play)
		{
			multi_send_sound_function (0,0);
			func_play = 0;
		}
	}

	if ((Controls.state.afterburner != Last_afterburner_state && Last_afterburner_charge) || (Last_afterburner_state && Last_afterburner_charge && !Afterburner_charge)) {
		if (Afterburner_charge && Controls.state.afterburner && have_afterburner) {
			digi_link_sound_to_object3(SOUND_AFTERBURNER_IGNITE, plobj, 1, F1_0, vm_distance{i2f(256)}, AFTERBURNER_LOOP_START, AFTERBURNER_LOOP_END);
			if (Game_mode & GM_MULTI)
			{
				multi_send_sound_function (3,SOUND_AFTERBURNER_IGNITE);
				func_play = 1;
			}
		} else {
			digi_kill_sound_linked_to_object(plobj);
			digi_link_sound_to_object2(SOUND_AFTERBURNER_PLAY, plobj, 0, F1_0, vm_distance{i2f(256)});
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

namespace dsx {

//	------------------------------------------------------------------------------------
//	Diminish palette effects towards normal.
static void diminish_palette_towards_normal(void)
{
	int	dec_amount = 0;
	float brightness_correction = 1-(static_cast<float>(gr_palette_get_gamma())/64); // to compensate for brightness setting of the game

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

}

namespace {

int	Redsave, Bluesave, Greensave;

}

#if defined(DXX_BUILD_DESCENT_II)
static
#endif
void palette_save(void)
{
	Redsave = PaletteRedAdd; Bluesave = PaletteBlueAdd; Greensave = PaletteGreenAdd;
}

namespace dsx {
void palette_restore(void)
{
	float brightness_correction = 1-(static_cast<float>(gr_palette_get_gamma())/64);

	PaletteRedAdd = Redsave; PaletteBlueAdd = Bluesave; PaletteGreenAdd = Greensave;
	gr_palette_step_up( PaletteRedAdd*brightness_correction, PaletteGreenAdd*brightness_correction, PaletteBlueAdd*brightness_correction );

#if defined(DXX_BUILD_DESCENT_II)
	//	Forces flash effect to fixup palette next frame.
	Time_flash_last_played = 0;
#endif
}
}

//	--------------------------------------------------------------------------------------------------
int allowed_to_fire_laser(void)
{
	if (Player_dead_state != player_dead_state::no)
	{
		Global_missile_firing_count = 0;
		return 0;
	}

	auto &plrobj = get_local_plrobj();
	auto &Next_laser_fire_time = plrobj.ctype.player_info.Next_laser_fire_time;
	//	Make sure enough time has elapsed to fire laser
	if (Next_laser_fire_time > GameTime64)
		return 0;

	return 1;
}

namespace dsx {
int allowed_to_fire_flare(void)
{
	auto &plrobj = get_local_plrobj();
	auto &Next_flare_fire_time = plrobj.ctype.player_info.Next_flare_fire_time;
	if (Next_flare_fire_time > GameTime64)
		return 0;

#if defined(DXX_BUILD_DESCENT_II)
	if (plrobj.ctype.player_info.energy < Weapon_info[weapon_id_type::FLARE_ID].energy_usage)
#define	FLARE_BIG_DELAY	(F1_0*2)
		Next_flare_fire_time = GameTime64 + FLARE_BIG_DELAY;
	else
#endif
		Next_flare_fire_time = GameTime64 + F1_0/4;

	return 1;
}
}

int allowed_to_fire_missile(void)
{
	auto &plrobj = get_local_plrobj();
	auto &Next_missile_fire_time = plrobj.ctype.player_info.Next_missile_fire_time;
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

#if DXX_USE_SDLMIXER
#define EXT_MUSIC_TEXT "Jukebox/Audio CD"
#else
#define EXT_MUSIC_TEXT "Audio CD"
#endif

static int free_help(newmenu *, const d_event &event, newmenu_item *items)
{
	if (event.type == EVENT_WINDOW_CLOSE)
	{
		std::default_delete<newmenu_item[]>()(items);
	}
	return 0;
}

#if (defined(__APPLE__) || defined(macintosh))
#define _DXX_HELP_MENU_SAVE_LOAD(VERB)	\
	DXX_MENUITEM(VERB, TEXT, "Alt-F2/F3 (\x85-SHIFT-s/o)\t  SAVE/LOAD GAME", HELP_AF2_3)	\
	DXX_MENUITEM(VERB, TEXT, "Alt-Shift-F2/F3 (\x85-s/o)\t  Quick Save/Load", HELP_ASF2_3)
#define _DXX_HELP_MENU_PAUSE(VERB) DXX_MENUITEM(VERB, TEXT, "Pause (\x85-P)\t  Pause", HELP_PAUSE)
#define _DXX_HELP_MENU_AUDIO(VERB)	\
	DXX_MENUITEM(VERB, TEXT, "\x85-E\t  Eject Audio CD", HELP_ASF9)	\
	DXX_MENUITEM(VERB, TEXT, "\x85-Up/Down\t  Play/Pause " EXT_MUSIC_TEXT, HELP_ASF10)	\
	DXX_MENUITEM(VERB, TEXT, "\x85-Left/Right\t  Previous/Next Song", HELP_ASF11_12)
#define _DXX_HELP_MENU_HINT_CMD_KEY(VERB, PREFIX)	\
	DXX_MENUITEM(VERB, TEXT, "", PREFIX##_SEP_HINT_CMD)	\
	DXX_MENUITEM(VERB, TEXT, "(Use \x85-# for F#. e.g. \x85-1 for F1)", PREFIX##_HINT_CMD)
#define _DXX_NETHELP_SAVELOAD_GAME(VERB)	\
	DXX_MENUITEM(VERB, TEXT, "Alt-F2/F3 (\x85-SHIFT-s/\x85-o)\t  SAVE/LOAD COOP GAME", NETHELP_SAVELOAD)
#else
#define _DXX_HELP_MENU_SAVE_LOAD(VERB)	\
	DXX_MENUITEM(VERB, TEXT, "Alt-F2/F3\t  SAVE/LOAD GAME", HELP_AF2_3)	\
	DXX_MENUITEM(VERB, TEXT, "Alt-Shift-F2/F3\t  Fast Save", HELP_ASF2_3)
#define _DXX_HELP_MENU_PAUSE(VERB)	DXX_MENUITEM(VERB, TEXT, TXT_HELP_PAUSE, HELP_PAUSE)
#define _DXX_HELP_MENU_AUDIO(VERB)	\
	DXX_MENUITEM(VERB, TEXT, "Alt-Shift-F9\t  Eject Audio CD", HELP_ASF9)	\
	DXX_MENUITEM(VERB, TEXT, "Alt-Shift-F10\t  Play/Pause " EXT_MUSIC_TEXT, HELP_ASF10)	\
	DXX_MENUITEM(VERB, TEXT, "Alt-Shift-F11/F12\t  Previous/Next Song", HELP_ASF11_12)
#define _DXX_HELP_MENU_HINT_CMD_KEY(VERB, PREFIX)
#define _DXX_NETHELP_SAVELOAD_GAME(VERB)	\
	DXX_MENUITEM(VERB, TEXT, "Alt-F2/F3\t  SAVE/LOAD COOP GAME", NETHELP_SAVELOAD)
#endif

#if defined(DXX_BUILD_DESCENT_II)
#define _DXX_HELP_MENU_D2_DXX_F4(VERB)	DXX_MENUITEM(VERB, TEXT, TXT_HELP_F4, HELP_F4)
#define _DXX_HELP_MENU_D2_DXX_FEATURES(VERB)	\
	DXX_MENUITEM(VERB, TEXT, "Shift-F1/F2\t  Cycle left/right window", HELP_SF1_2)	\
	DXX_MENUITEM(VERB, TEXT, "Shift-F4\t  GuideBot menu", HELP_SF4)	\
	DXX_MENUITEM(VERB, TEXT, "Alt-Shift-F4\t  Rename GuideBot", HELP_ASF4)	\
	DXX_MENUITEM(VERB, TEXT, "Shift-F5/F6\t  Drop primary/secondary", HELP_SF5_6)	\
	DXX_MENUITEM(VERB, TEXT, "Shift-number\t  GuideBot commands", HELP_GUIDEBOT_COMMANDS)
#define _DXX_NETHELP_DROPFLAG(VERB)	\
	DXX_MENUITEM(VERB, TEXT, "ALT-0\t  DROP FLAG", NETHELP_DROPFLAG)
#else
#define _DXX_HELP_MENU_D2_DXX_F4(VERB)
#define _DXX_HELP_MENU_D2_DXX_FEATURES(VERB)
#define _DXX_NETHELP_DROPFLAG(VERB)
#endif

#define DXX_HELP_MENU(VERB)	\
	DXX_MENUITEM(VERB, TEXT, TXT_HELP_ESC, HELP_ESC)	\
	DXX_MENUITEM(VERB, TEXT, "SHIFT-ESC\t  SHOW GAME LOG", HELP_LOG)	\
	DXX_MENUITEM(VERB, TEXT, "F1\t  THIS SCREEN", HELP_HELP)	\
	DXX_MENUITEM(VERB, TEXT, TXT_HELP_F2, HELP_F2)	\
	_DXX_HELP_MENU_SAVE_LOAD(VERB)	\
	DXX_MENUITEM(VERB, TEXT, "F3\t  SWITCH COCKPIT MODES", HELP_F3)	\
	_DXX_HELP_MENU_D2_DXX_F4(VERB)	\
	DXX_MENUITEM(VERB, TEXT, TXT_HELP_F5, HELP_F5)	\
	DXX_MENUITEM(VERB, TEXT, "ALT-F7\t  SWITCH HUD MODES", HELP_AF7)	\
	_DXX_HELP_MENU_PAUSE(VERB)	\
	DXX_MENUITEM(VERB, TEXT, TXT_HELP_PRTSCN, HELP_PRTSCN)	\
	DXX_MENUITEM(VERB, TEXT, TXT_HELP_1TO5, HELP_1TO5)	\
	DXX_MENUITEM(VERB, TEXT, TXT_HELP_6TO10, HELP_6TO10)	\
	_DXX_HELP_MENU_D2_DXX_FEATURES(VERB)	\
	_DXX_HELP_MENU_AUDIO(VERB)	\
	_DXX_HELP_MENU_HINT_CMD_KEY(VERB, HELP)	\

enum {
	DXX_HELP_MENU(ENUM)
};

void show_help()
{
	const unsigned nitems = DXX_HELP_MENU(COUNT);
	auto m = new newmenu_item[nitems];
	DXX_HELP_MENU(ADD);
	newmenu_dotiny(NULL, TXT_KEYS, nitems, m, 0, free_help, m);
}

#undef DXX_HELP_MENU

#define DXX_NETHELP_MENU(VERB)	\
	DXX_MENUITEM(VERB, TEXT, "F1\t  THIS SCREEN", NETHELP_HELP)	\
	_DXX_NETHELP_DROPFLAG(VERB)	\
	_DXX_NETHELP_SAVELOAD_GAME(VERB)	\
	DXX_MENUITEM(VERB, TEXT, "ALT-F4\t  SHOW PLAYER NAMES ON HUD", NETHELP_HUDNAMES)	\
	DXX_MENUITEM(VERB, TEXT, "F7\t  TOGGLE KILL LIST", NETHELP_TOGGLE_KILL_LIST)	\
	DXX_MENUITEM(VERB, TEXT, "F8\t  SEND MESSAGE", NETHELP_SENDMSG)	\
	DXX_MENUITEM(VERB, TEXT, "(SHIFT-)F9 to F12\t  (DEFINE)SEND MACRO", NETHELP_MACRO)	\
	DXX_MENUITEM(VERB, TEXT, "PAUSE\t  SHOW NETGAME INFORMATION", NETHELP_GAME_INFO)	\
	DXX_MENUITEM(VERB, TEXT, "SHIFT-PAUSE\t  SHOW NETGAME INFO & RULES", NETHELP_GAME_INFORULES)	\
	_DXX_HELP_MENU_HINT_CMD_KEY(VERB, NETHELP)	\
	DXX_MENUITEM(VERB, TEXT, "", NETHELP_SEP1)	\
	DXX_MENUITEM(VERB, TEXT, "MULTIPLAYER MESSAGE COMMANDS:", NETHELP_COMMAND_HEADER)	\
	DXX_MENUITEM(VERB, TEXT, "(*): TEXT\t  SEND TEXT TO PLAYER/TEAM (*)", NETHELP_DIRECT_MESSAGE)	\
	DXX_MENUITEM(VERB, TEXT, "/Handicap: (*)\t  SET YOUR STARTING SHIELDS TO (*) [10-100]", NETHELP_COMMAND_HANDICAP)	\
	DXX_MENUITEM(VERB, TEXT, "/move: (*)\t  MOVE PLAYER (*) TO OTHER TEAM (Host-only)", NETHELP_COMMAND_MOVE)	\
	DXX_MENUITEM(VERB, TEXT, "/kick: (*)\t  KICK PLAYER (*) FROM GAME (Host-only)", NETHELP_COMMAND_KICK)	\
	DXX_MENUITEM(VERB, TEXT, "/KillReactor\t  BLOW UP THE MINE (Host-only)", NETHELP_COMMAND_KILL_REACTOR)	\

enum {
	DXX_NETHELP_MENU(ENUM)
};

void show_netgame_help()
{
	const unsigned nitems = DXX_NETHELP_MENU(COUNT);
	auto m = new newmenu_item[nitems];
	DXX_NETHELP_MENU(ADD);
	newmenu_dotiny(NULL, TXT_KEYS, nitems, m, 0, free_help, m);
}

#undef DXX_NETHELP_MENU

#define DXX_NEWDEMO_HELP_MENU(VERB)	\
	DXX_MENUITEM(VERB, TEXT, "ESC\t  QUIT DEMO PLAYBACK", DEMOHELP_QUIT)	\
	DXX_MENUITEM(VERB, TEXT, "F1\t  THIS SCREEN", DEMOHELP_HELP)	\
	DXX_MENUITEM(VERB, TEXT, TXT_HELP_F2, DEMOHELP_F2)	\
	DXX_MENUITEM(VERB, TEXT, "F3\t  SWITCH COCKPIT MODES", DEMOHELP_F3)	\
	DXX_MENUITEM(VERB, TEXT, "F4\t  TOGGLE PERCENTAGE DISPLAY", DEMOHELP_F4)	\
	DXX_MENUITEM(VERB, TEXT, "UP\t  PLAY", DEMOHELP_PLAY)	\
	DXX_MENUITEM(VERB, TEXT, "DOWN\t  PAUSE", DEMOHELP_PAUSE)	\
	DXX_MENUITEM(VERB, TEXT, "RIGHT\t  ONE FRAME FORWARD", DEMOHELP_FRAME_FORWARD)	\
	DXX_MENUITEM(VERB, TEXT, "LEFT\t  ONE FRAME BACKWARD", DEMOHELP_FRAME_BACKWARD)	\
	DXX_MENUITEM(VERB, TEXT, "SHIFT-RIGHT\t  FAST FORWARD", DEMOHELP_FAST_FORWARD)	\
	DXX_MENUITEM(VERB, TEXT, "SHIFT-LEFT\t  FAST BACKWARD", DEMOHELP_FAST_BACKWARD)	\
	DXX_MENUITEM(VERB, TEXT, "CTRL-RIGHT\t  JUMP TO END", DEMOHELP_JUMP_END)	\
	DXX_MENUITEM(VERB, TEXT, "CTRL-LEFT\t  JUMP TO START", DEMOHELP_JUMP_START)	\
	_DXX_HELP_MENU_HINT_CMD_KEY(VERB, DEMOHELP)	\

enum {
	DXX_NEWDEMO_HELP_MENU(ENUM)
};

void show_newdemo_help()
{
	const unsigned nitems = DXX_NEWDEMO_HELP_MENU(COUNT);
	auto m = new newmenu_item[nitems];
	DXX_NEWDEMO_HELP_MENU(ADD);
	newmenu_dotiny(NULL, "DEMO PLAYBACK CONTROLS", nitems, m, 0, free_help, m);
}

#undef DXX_NEWDEMO_HELP_MENU

#define LEAVE_TIME 0x4000		//how long until we decide key is down	(Used to be 0x4000)

enum class leave_type : uint_fast8_t
{
	none,
	maybe_on_release,
	wait_for_release,
	on_press,
};

static leave_type leave_mode;

static void end_rear_view()
{
	Rear_view = 0;
	if (PlayerCfg.CockpitMode[1] == CM_REAR_VIEW)
		select_cockpit(PlayerCfg.CockpitMode[0]);
	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_restore_rearview();
}

static void check_end_rear_view()
{
	leave_mode = leave_type::none;
	if (Rear_view)
		end_rear_view();
}

//deal with rear view - switch it on, or off, or whatever
void check_rear_view()
{
	static fix64 entry_time;

	if (Newdemo_state == ND_STATE_PLAYBACK)
		return;

	const auto rear_view = Controls.state.rear_view;
	switch (leave_mode)
	{
		case leave_type::none:
			if (!rear_view)
				return;
			if (Rear_view)
				end_rear_view();
			else
			{
				Rear_view = 1;
				leave_mode = leave_type::maybe_on_release;		//means wait for another key
				entry_time = timer_query();
				if (PlayerCfg.CockpitMode[1] == CM_FULL_COCKPIT)
					select_cockpit(CM_REAR_VIEW);
				if (Newdemo_state == ND_STATE_RECORDING)
					newdemo_record_rearview();
			}
			return;
		case leave_type::maybe_on_release:
			if (rear_view)
			{
				if (timer_query() - entry_time > LEAVE_TIME)
					leave_mode = leave_type::wait_for_release;
			}
			else
				leave_mode = leave_type::on_press;
			return;
		case leave_type::wait_for_release:
			if (!rear_view)
				check_end_rear_view();
			return;
		case leave_type::on_press:
			if (rear_view)
			{
				Controls.state.rear_view = 0;
				check_end_rear_view();
			}
			return;
		default:
			break;
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

int cheats_enabled()
{
	return cheats.enabled;
}

//turns off all cheats & resets cheater flag
void game_disable_cheats()
{
	cheats = {};
}

//	game_setup()
// ----------------------------------------------------------------------------

namespace dsx {

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
		Cursegp = segptridx(segment_first);
		Curside = 0;
	}
	
	if (vcsegptr(ConsoleObject->segnum)->segnum == segment_none)      //segment no longer exists
		obj_relink(vobjptridx(ConsoleObject), Cursegp);

	if (!check_obj_seg(ConsoleObject))
		move_player_2_segment(Cursegp,Curside);
#endif

	Viewer = ConsoleObject;
	fly_init(*ConsoleObject);
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
	if (CGameArg.SysAutoRecordDemo && Newdemo_state == ND_STATE_NORMAL)
		newdemo_start_recording();
	return game_wind;
}

}

window *Game_wind = NULL;

namespace dsx {

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

}

//called at the end of the program
void close_game()
{
	close_gauges();
	restore_effect_bitmap_icons();
}

#if defined(DXX_BUILD_DESCENT_II)
namespace dsx {
object *Missile_viewer=NULL;
object_signature_t Missile_viewer_sig;

array<int, 2> Marker_viewer_num{{-1,-1}};
array<int, 2> Coop_view_player{{-1,-1}};

//returns ptr to escort robot, or NULL
objptridx_t find_escort()
{
	range_for (const auto &&o, vobjptridx)
	{
		if (o->type == OBJ_ROBOT && Robot_info[get_robot_id(o)].companion)
			return objptridx_t(o);
	}
	return object_none;
}

//if water or fire level, make occasional sound
static void do_ambient_sounds()
{
	int has_water,has_lava;
	int sound;

	const auto s2_flags = vcsegptr(ConsoleObject->segnum)->s2_flags;
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
}
#endif

void game_leave_menus(void)
{
	if (!Game_wind)
		return;
	for (;;) // go through all windows and actually close them if they want to
	{
		const auto wind = window_get_front();
		if (!wind)
			break;
		if (wind == Game_wind)
			break;
		if (!window_close(wind))
			break;
	}
}

namespace dsx {

void GameProcessFrame(void)
{
	auto &local_player_shields_ref = get_local_player_shields();
	fix player_shields = local_player_shields_ref;
	const auto player_was_dead = Player_dead_state;

	update_player_stats();
	diminish_palette_towards_normal();		//	Should leave palette effect up for as long as possible by putting right before render.
	do_afterburner_stuff();
	do_cloak_stuff();
	do_invulnerable_stuff();
	remove_obsolete_stuck_objects();
#if defined(DXX_BUILD_DESCENT_II)
	init_ai_frame();
	do_final_boss_frame();

	auto &pl_flags = get_local_player_flags();
	if (pl_flags & PLAYER_FLAGS_HEADLIGHT_ON)
	{
		static int turned_off=0;
		auto &player_info = get_local_plrobj().ctype.player_info;
		auto &energy = player_info.energy;
		energy -= (FrameTime*3/8);
		if (energy < i2f(10)) {
			if (!turned_off) {
				pl_flags &= ~PLAYER_FLAGS_HEADLIGHT_ON;
				turned_off = 1;
				if (Game_mode & GM_MULTI)
					multi_send_flags(Player_num);
			}
		}
		else
			turned_off = 0;

		if (energy <= 0)
		{
			energy = 0;
			pl_flags &= ~PLAYER_FLAGS_HEADLIGHT_ON;
			if (Game_mode & GM_MULTI)
				multi_send_flags(Player_num);
		}
	}
#endif

#ifdef EDITOR
	check_create_player_path();
	player_follow_path(vobjptr(ConsoleObject));
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

		object_move_all();
		powerup_grab_cheat_all();

		if (Endlevel_sequence)	//might have been started during move
			return;

		fuelcen_update_all();

		do_ai_frame_all();

		if (allowed_to_fire_laser())
			FireLaser();				// Fire Laser!

		auto laser_firing_count = Global_laser_firing_count;
		if (Auto_fire_fusion_cannon_time) {
			auto &player_info = get_local_plrobj().ctype.player_info;
			if (player_info.Primary_weapon != primary_weapon_index_t::FUSION_INDEX)
				Auto_fire_fusion_cannon_time = 0;
			else if ((laser_firing_count = (GameTime64 + FrameTime/2 >= Auto_fire_fusion_cannon_time)))
			{
				Auto_fire_fusion_cannon_time = 0;
			} else if (d_tick_step) {
				const auto rx = (d_rand() - 16384) / 8;
				const auto rz = (d_rand() - 16384) / 8;
				const auto &&console = vobjptr(ConsoleObject);
				auto &rotvel = console->mtype.phys_info.rotvel;
				rotvel.x += rx;
				rotvel.z += rz;

				const auto bump_amount = Fusion_charge > F1_0*2 ? Fusion_charge*4 : F1_0*4;
				bump_one_object(console, make_random_vector(), bump_amount);
			}
		}

		if (laser_firing_count)
		{
			laser_firing_count -= do_laser_firing_player();
			if (laser_firing_count < 0)
				laser_firing_count = 0;
		}
		Global_laser_firing_count = laser_firing_count;
		delayed_autoselect();
	}

	if (Do_appearance_effect) {
		Do_appearance_effect = 0;
		create_player_appearance_effect(vobjptridx(ConsoleObject));
	}

#if defined(DXX_BUILD_DESCENT_II)
	omega_charge_frame();
	slide_textures();
	flicker_lights();

	//if the player is taking damage, give up guided missile control
	if (local_player_shields_ref != player_shields)
		release_guided_missile(Player_num);
#endif

	// Check if we have to close in-game menus for multiplayer
	if ((Game_mode & GM_MULTI) && (get_local_player().connected == CONNECT_PLAYING))
	{
		if (Endlevel_sequence || (Player_dead_state != player_was_dead) || (local_player_shields_ref < player_shields) || (Control_center_destroyed && Countdown_seconds_left < 10))
                        game_leave_menus();
	}
}

#if defined(DXX_BUILD_DESCENT_II)
void compute_slide_segs()
{
	range_for (const auto &&segp, vsegptr)
	{
		uint8_t slide_textures = 0;
		for (int sidenum=0;sidenum<6;sidenum++) {
			const auto &side = segp->sides[sidenum];
			const auto &ti = TmapInfo[side.tmap_num];
			if (!(ti.slide_u || ti.slide_v))
				continue;
			if (IS_CHILD(segp->children[sidenum]) && side.wall_num == wall_none)
				/* If a wall exists, it could be visible at start or
				 * become visible later, so always enable sliding for
				 * walls.
				 */
				continue;
			slide_textures |= 1 << sidenum;
		}
		segp->slide_textures = slide_textures;
	}
}

template <fix uvl::*p>
static void update_uv(array<uvl, 4> &uvls, uvl &i, fix a)
{
	if (!a)
		return;
	const auto ip = (i.*p += a);
	if (ip > f2_0)
		range_for (auto &j, uvls)
			j.*p -= f1_0;
	else if (ip < -f2_0)
		range_for (auto &j, uvls)
			j.*p += f1_0;
}

//	-----------------------------------------------------------------------------
static void slide_textures(void)
{
	range_for (const auto &&segp, vsegptr)
	{
		if (const auto slide_seg = segp->slide_textures)
		{
			for (int sidenum=0;sidenum<6;sidenum++) {
				if (slide_seg & (1 << sidenum))
				{
					auto &side = segp->sides[sidenum];
					const auto &ti = TmapInfo[side.tmap_num];
					const auto tiu = ti.slide_u;
					const auto tiv = ti.slide_v;
					if (tiu || tiv)
					{
						const auto frametime = FrameTime;
						const auto ua = fixmul(frametime, tiu << 8);
						const auto va = fixmul(frametime, tiv << 8);
						auto &uvls = side.uvls;
						range_for (auto &i, uvls)
						{
							update_uv<&uvl::u>(uvls, i, ua);
							update_uv<&uvl::v>(uvls, i, va);
						}
					}
				}
			}
		}
	}
}

Flickering_light_array_t Flickering_lights;
unsigned Num_flickering_lights;

constexpr fix flicker_timer_disabled = 0x80000000;

static void flicker_lights()
{
	range_for (auto &f, partial_range(Flickering_lights, Num_flickering_lights))
	{
		if (f.timer == flicker_timer_disabled)		//disabled
			continue;
		const auto &&segp = vsegptridx(f.segnum);
		const auto sidenum = f.sidenum;
		{
			auto &side = segp->sides[sidenum];
			if (!(TmapInfo[side.tmap_num].lighting || TmapInfo[side.tmap_num2 & 0x3fff].lighting))
				continue;
		}

		//make sure this is actually a light
		if (! (WALL_IS_DOORWAY(segp, sidenum) & WID_RENDER_FLAG))
			continue;

		if ((f.timer -= FrameTime) < 0)
		{
			while (f.timer < 0)
				f.timer += f.delay;
			f.mask = ((f.mask & 0x80000000) ? 1 : 0) + (f.mask << 1);
			if (f.mask & 1)
				add_light(segp, sidenum);
			else
				subtract_light(segp, sidenum);
		}
	}
}

//returns ptr to flickering light structure, or NULL if can't find
static std::pair<Flickering_light_array_t::iterator, Flickering_light_array_t::iterator> find_flicker(segnum_t segnum, int sidenum)
{
	//see if there's already an entry for this seg/side
	const auto &&pr = partial_range(Flickering_lights, Num_flickering_lights);
	auto predicate = [segnum, sidenum](const flickering_light &f) {
		return f.segnum == segnum && f.sidenum == sidenum;	//found it!
	};
	return {std::find_if(pr.begin(), pr.end(), predicate), pr.end()};
}

static void update_flicker(const segnum_t segnum, const unsigned sidenum, const fix timer)
{
	auto i = find_flicker(segnum, sidenum);
	if (i.first != i.second)
		i.first->timer = timer;
}

//turn flickering off (because light has been turned off)
void disable_flicker(segnum_t segnum,int sidenum)
{
	update_flicker(segnum, sidenum, flicker_timer_disabled);
}

//turn flickering off (because light has been turned on)
void enable_flicker(segnum_t segnum,int sidenum)
{
	update_flicker(segnum, sidenum, 0);
}
#endif

//	-----------------------------------------------------------------------------
//	Fire Laser:  Registers a laser fire, and performs special stuff for the fusion
//				    cannon.
void FireLaser()
{
	auto &player_info = get_local_plrobj().ctype.player_info;
	auto &Primary_weapon = player_info.Primary_weapon;
	Global_laser_firing_count = Controls.state.fire_primary
		? Weapon_info[Primary_weapon_to_weapon_info[Primary_weapon]].fire_count
		: 0;

	if ((Primary_weapon == primary_weapon_index_t::FUSION_INDEX) && (Global_laser_firing_count)) {
		auto &energy = player_info.energy;
		if (energy < F1_0 * 2 && Auto_fire_fusion_cannon_time == 0)
		{
			Global_laser_firing_count = 0;
		} else {
			static fix64 Fusion_next_sound_time = 0;

			if (Fusion_charge == 0)
				energy -= F1_0*2;

			Fusion_charge += FrameTime;
			energy -= FrameTime;

			if (energy <= 0)
			{
				energy = 0;
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
					create_awareness_event(vobjptr(ConsoleObject), player_awareness_type_t::PA_WEAPON_ROBOT_COLLISION);
					multi_digi_play_sample(SOUND_FUSION_WARMUP, F1_0);
				}
				Fusion_next_sound_time = GameTime64 + F1_0/8 + d_rand()/4;
			}
		}
	}
}


//	-------------------------------------------------------------------------------------------------------
//	If player is close enough to objnum, which ought to be a powerup, pick it up!
//	This could easily be made difficulty level dependent.
static void powerup_grab_cheat(object &player, const vobjptridx_t powerup)
{
	fix	powerup_size;
	fix	player_size;

	Assert(powerup->type == OBJ_POWERUP);

	powerup_size = powerup->size;
	player_size = player.size;

	const auto dist = vm_vec_dist_quick(powerup->pos, player.pos);

	if ((dist < 2*(powerup_size + player_size)) && !(powerup->flags & OF_SHOULD_BE_DEAD)) {
		const auto collision_point = vm_vec_avg(powerup->pos, player.pos);
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
	const auto &&console = vobjptr(ConsoleObject);
	range_for (const auto objnum, objects_in(vsegptr(console->segnum)))
		if (objnum->type == OBJ_POWERUP)
			powerup_grab_cheat(console, objnum);
}

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

		seg_center = Point_segs[player_hide_index+i].point;

		const auto &&obj = obj_create(OBJ_POWERUP, POW_ENERGY, vsegptridx(Point_segs[player_hide_index+i].segnum), seg_center, &vmd_identity_matrix, Powerup_info[POW_ENERGY].size, CT_POWERUP, MT_NONE, RT_POWERUP);
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
	range_for (const auto &&segp, vcsegptridx)
	{
		for (int j=0; j<MAX_SIDES_PER_SEGMENT; j++)
			if (segp->children[j] == segment_exit)
			{
				return mark_player_path_to_segment(segp);
			}
	}

	return 0;
}

#endif


#if defined(DXX_BUILD_DESCENT_II)
namespace dsx {
/*
 * reads a flickering_light structure from a PHYSFS_File
 */
void flickering_light_read(flickering_light *fl, PHYSFS_File *fp)
{
	fl->segnum = PHYSFSX_readShort(fp);
	fl->sidenum = PHYSFSX_readShort(fp);
	fl->mask = PHYSFSX_readInt(fp);
	fl->timer = PHYSFSX_readFix(fp);
	fl->delay = PHYSFSX_readFix(fp);
}

void flickering_light_write(const flickering_light *fl, PHYSFS_File *fp)
{
	PHYSFS_writeSLE16(fp, fl->segnum);
	PHYSFS_writeSLE16(fp, fl->sidenum);
	PHYSFS_writeULE32(fp, fl->mask);
	PHYSFSX_writeFix(fp, fl->timer);
	PHYSFSX_writeFix(fp, fl->delay);
}
}
#endif
