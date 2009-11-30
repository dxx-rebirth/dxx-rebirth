/* $Id: gamecntl.c,v 1.1.1.1 2006/03/17 19:56:33 zicodxx Exp $ */
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
 * Game Controls Stuff
 *
 */

//#define DOOR_DEBUGGING

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "pstypes.h"
#include "console.h"
#include "inferno.h"
#include "game.h"
#include "player.h"
#include "key.h"
#include "object.h"
#include "menu.h"
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
#include "gameseg.h"
#include "wall.h"
#include "ai.h"
#include "rbaudio.h"
#include "digi.h"
#include "u_mem.h"
#include "palette.h"
#include "morph.h"
#include "lighting.h"
#include "newdemo.h"
#include "weapon.h"
#include "sounds.h"
#include "args.h"
#include "gameseq.h"
#include "automap.h"
#include "text.h"
#include "powerup.h"
#include "songs.h"
#include "newmenu.h"
#include "gamefont.h"
#include "endlevel.h"
#include "config.h"
#include "kconfig.h"
#include "mouse.h"
#include "titles.h"
#include "gr.h"
#include "playsave.h"
#include "scores.h"

#include "multi.h"
#include "desc_id.h"
#include "cntrlcen.h"
#include "fuelcen.h"
#include "pcx.h"
#include "state.h"
#include "piggy.h"
#include "multibot.h"
#include "ai.h"
#include "rbaudio.h"
#include "switch.h"

#ifdef EDITOR
#include "editor/editor.h"
#endif

#include <SDL/SDL.h>

// Global Variables -----------------------------------------------------------

int	Debug_spew;

//	External Variables ---------------------------------------------------------

extern char WaitForRefuseAnswer,RefuseThisPlayer,RefuseTeam;

#ifndef NDEBUG
extern int	Mark_count;
#endif

extern int	Global_missile_firing_count;
extern int	Automap_flag;
extern int	Config_menu_flag;

extern int	Game_aborted;
extern int	*Toggle_var;

extern int	Physics_cheat_flag;

extern fix	Show_view_text_timer;

//	Function prototypes --------------------------------------------------------


extern void CyclePrimary();
extern void CycleSecondary();
extern int	allowed_to_fire_missile(void);
extern int	allowed_to_fire_flare(void);
extern void	check_rear_view(void);
extern int	create_special_path(void);
extern void move_player_2_segment(segment *seg, int side);
extern	void newdemo_strip_frames(char *, int);
extern void toggle_cockpit(void);
extern void dump_used_textures_all();

void FinalCheatsKey(int key);

#ifndef RELEASE
void do_cheat_menu(void);
#endif

void HandleGameKey(int key);
void advance_sound(void);
void play_test_sound(void);

#define key_isfunc(k) (((k&0xff)>=KEY_F1 && (k&0xff)<=KEY_F10) || (k&0xff)==KEY_F11 || (k&0xff)==KEY_F12)
#define key_ismod(k)  ((k&0xff)==KEY_LALT || (k&0xff)==KEY_RALT || (k&0xff)==KEY_LSHIFT || (k&0xff)==KEY_RSHIFT || (k&0xff)==KEY_LCTRL || (k&0xff)==KEY_RCTRL || (k&0xff)==KEY_LMETA || (k&0xff)==KEY_RMETA)

void update_vcr_state();
void do_weapon_stuff(void);


// Control Functions

fix newdemo_single_frame_time;

void update_vcr_state(void)
{
	if ((keyd_pressed[KEY_LSHIFT] || keyd_pressed[KEY_RSHIFT]) && keyd_pressed[KEY_RIGHT] && FixedStep & EPS20)
		Newdemo_vcr_state = ND_STATE_FASTFORWARD;
	else if ((keyd_pressed[KEY_LSHIFT] || keyd_pressed[KEY_RSHIFT]) && keyd_pressed[KEY_LEFT] && FixedStep & EPS20)
		Newdemo_vcr_state = ND_STATE_REWINDING;
	else if (!(keyd_pressed[KEY_LCTRL] || keyd_pressed[KEY_RCTRL]) && keyd_pressed[KEY_RIGHT] && ((GameTime - newdemo_single_frame_time) >= F1_0) && FixedStep & EPS20)
		Newdemo_vcr_state = ND_STATE_ONEFRAMEFORWARD;
	else if (!(keyd_pressed[KEY_LCTRL] || keyd_pressed[KEY_RCTRL]) && keyd_pressed[KEY_LEFT] && ((GameTime - newdemo_single_frame_time) >= F1_0) && FixedStep & EPS20)
		Newdemo_vcr_state = ND_STATE_ONEFRAMEBACKWARD;
	else if ((Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_REWINDING))
		Newdemo_vcr_state = ND_STATE_PLAYBACK;
}

void do_weapon_stuff()
{
	int i;
	
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
	
	if (Controls.cycle_primary_down_count)
	{
		for (i=0;i<Controls.cycle_primary_down_count;i++)
			CyclePrimary ();
	}
	if (Controls.cycle_secondary_down_count)
	{
		for (i=0;i<Controls.cycle_secondary_down_count;i++)
			CycleSecondary ();
	}
}


extern void game_render_frame();

void format_time(char *str, int secs_int)
{
	int h, m, s;
	
	h = secs_int/3600;
	s = secs_int%3600;
	m = s / 60;
	s = s % 60;
	sprintf(str, "%1d:%02d:%02d", h, m, s );
}

extern int netplayerinfo_on;

//Process selected keys until game unpaused. returns key that left pause (p or esc)
int do_game_pause()
{
	int key;
	char msg[1000];
	char total_time[9],level_time[9];
	int Game_paused;
	
#ifdef NETWORK
	if (Game_mode & GM_MULTI)
	{
		netplayerinfo_on= !netplayerinfo_on;
		return(KEY_PAUSE);
	}
#endif
	
	digi_pause_all();
	ext_music_pause();
	stop_time();
	game_flush_inputs();
	gr_palette_load( gr_palette );
	format_time(total_time, f2i(Players[Player_num].time_total) + Players[Player_num].hours_total*3600);
	format_time(level_time, f2i(Players[Player_num].time_level) + Players[Player_num].hours_level*3600);
	if (Newdemo_state!=ND_STATE_PLAYBACK)
		sprintf(msg,"PAUSE\n\nSkill level:  %s\nHostages on board:  %d\nTime on level: %s\nTotal time in game: %s",(*(&TXT_DIFFICULTY_1 + (Difficulty_level))),Players[Player_num].hostages_on_board,level_time,total_time);
	else
	  	sprintf(msg,"PAUSE\n\nSkill level:  %s\nHostages on board:  %d\n",(*(&TXT_DIFFICULTY_1 + (Difficulty_level))),Players[Player_num].hostages_on_board);
	Game_paused=1;
	set_screen_mode(SCREEN_MENU);
	show_boxed_message(msg, 1);
	
	while (Game_paused) 
	{
		timer_delay2(50);
#ifdef OGL
		show_boxed_message(msg, 1);
#endif
		
		key = key_inkey();
		
		switch (key) {
			case 0:
				break;
			case KEY_ESC:
				Function_mode = FMODE_MENU;
				Game_paused=0;
				break;
			case KEY_F1:
				show_help();
				show_boxed_message(TXT_PAUSE, 1);
				break;
			case KEY_PAUSE:
				Game_paused=0;
				break;
			case KEY_ALTED+KEY_ENTER:
			case KEY_ALTED+KEY_PADENTER:
				gr_toggle_fullscreen();
				break;
			default:
				break;
		}
	}
	
	game_flush_inputs();
	reset_cockpit();
	start_time();
	if (EXT_MUSIC_ON)
		ext_music_resume();
	digi_resume_all();
	
	return key;
}

void HandleEndlevelKey(int key)
{
	
#ifdef macintosh
	if ( key == (KEY_COMMAND + KEY_SHIFTED + KEY_3) )
		save_screen_shot(0);
#endif
	
	if (key==KEY_PRINT_SCREEN)
		save_screen_shot(0);
	
#if defined(__APPLE__) || defined(macintosh)
	if ( key == (KEY_COMMAND+KEY_P) )
		key = do_game_pause();
#endif
	if (key == KEY_PAUSE)
		key = do_game_pause();		//so esc from pause will end level
	
	if (key == KEY_ESC)	{
		stop_endlevel_sequence();
		last_drawn_cockpit=-1;
		return;
	}
	
	if (key == KEY_BACKSP)
		Int3();
}

void HandleDeathKey(int key)
{
	if (Player_exploded && !key_isfunc(key) && !key_ismod(key) && key)
		Death_sequence_aborted  = 1;		//Any key but func or modifier aborts
	
#ifdef macintosh
	if ( key == (KEY_COMMAND + KEY_SHIFTED + KEY_3) ) {
		//		save_screen_shot(0);
		Death_sequence_aborted  = 0;		// Clear because code above sets this for any key.
	}
#endif
	
#if defined(__APPLE__) || defined(macintosh)
	if (key == KEY_COMMAND+KEY_Q)
		//macintosh_quit();
		Death_sequence_aborted  = 0;		// Clear because code above sets this for any key.
#endif
	
	if (key==KEY_PRINT_SCREEN) {
		Death_sequence_aborted  = 0;		// Clear because code above sets this for any key.
	}
	
#if defined(__APPLE__) || defined(macintosh)
	if ( key == (KEY_COMMAND+KEY_P) ) {
		//		key = do_game_pause();
		Death_sequence_aborted  = 0;		// Clear because code above sets this for any key.
	}
#endif
	
	if (key == KEY_PAUSE)   {
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
	if ( Game_mode & GM_MULTI && ((key == KEY_ALTED + KEY_1) || (key == KEY_ALTED + KEY_2) || key == KEY_F6))
		Death_sequence_aborted  = 0;
	
	if (Death_sequence_aborted)
		game_flush_inputs();
}

void HandleDemoKey(int key)
{
	switch (key) {
			
		case KEY_DEBUGGED + KEY_I:
			Newdemo_do_interpolate = !Newdemo_do_interpolate;
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
			KEY_MAC(case KEY_COMMAND+KEY_1:)
		case KEY_F1:	show_newdemo_help();	break;
			KEY_MAC(case KEY_COMMAND+KEY_2:)
		case KEY_F2:	Config_menu_flag = 1;	break;
			KEY_MAC(case KEY_COMMAND+KEY_3:)
		case KEY_F3:	toggle_cockpit();	break;
			KEY_MAC(case KEY_COMMAND+KEY_4:)
		case KEY_F4:	Newdemo_show_percentage = !Newdemo_show_percentage; break;
			KEY_MAC(case KEY_COMMAND+KEY_7:)
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
			newdemo_single_frame_time = GameTime;
			Newdemo_vcr_state = ND_STATE_ONEFRAMEBACKWARD;
			break;
		case KEY_RIGHT:
			newdemo_single_frame_time = GameTime;
			Newdemo_vcr_state = ND_STATE_ONEFRAMEFORWARD;
			break;
		case KEY_CTRLED + KEY_RIGHT:
			newdemo_goto_end(0);
			break;
		case KEY_CTRLED + KEY_LEFT:
			newdemo_goto_beginning();
			break;
			KEY_MAC(case KEY_COMMAND+KEY_P:)
		case KEY_PAUSE:
			do_game_pause();
			break;
#ifdef macintosh
		case KEY_COMMAND + KEY_SHIFTED + KEY_3:
#endif
		case KEY_PRINT_SCREEN:
		{
			if (PlayerCfg.PRShot)
			{
				gr_set_current_canvas(NULL);
				render_frame(0);
				gr_set_curfont(MEDIUM2_FONT);
				gr_printf(SWIDTH-FSPACX(92),SHEIGHT-LINE_SPACING,"DXX-Rebirth\n");
				gr_flip();
				save_screen_shot(0);
			}
			else
			{
				int old_state;
				old_state = Newdemo_show_percentage;
				Newdemo_show_percentage = 0;
				game_do_render_frame(GameArg.DbgUseDoubleBuffer);
				save_screen_shot(0);
				Newdemo_show_percentage = old_state;
			}
			break;
		}
		case KEY_ALTED+KEY_ENTER:
		case KEY_ALTED+KEY_PADENTER:
			gr_toggle_fullscreen();
			break;
			
			break;
	}
}

void songs_goto_next_song();
void songs_goto_prev_song();

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
			KEY_MAC(case KEY_COMMAND+KEY_1:)
			case KEY_F1:				if (Game_mode & GM_MULTI) show_netgame_help(); else show_help();         break;
			KEY_MAC(case KEY_COMMAND+KEY_2:)
			case KEY_F2:				Config_menu_flag = 1;	break;
			KEY_MAC(case KEY_COMMAND+KEY_3:)
			case KEY_F3:				toggle_cockpit();       break;
			
#ifdef NETWORK
			KEY_MAC(case KEY_COMMAND+KEY_ALTED+KEY_4:)
			case KEY_ALTED + KEY_F4:
			Show_reticle_name = (Show_reticle_name+1)%2;
			break;
#endif
			KEY_MAC(case KEY_COMMAND+KEY_5:)
			case KEY_F5:
			if ( Newdemo_state == ND_STATE_RECORDING )
				newdemo_stop_recording();
			else if ( Newdemo_state == ND_STATE_NORMAL )
				newdemo_start_recording();
			break;
#ifdef NETWORK
			KEY_MAC(case KEY_COMMAND+KEY_6:)
			case KEY_F6:
			if (Netgame.RefusePlayers && WaitForRefuseAnswer)
			{
				RefuseThisPlayer=1;
				HUD_init_message ("Player accepted!");
			}
			break;
			case KEY_ALTED + KEY_1:
				if (Netgame.RefusePlayers && WaitForRefuseAnswer && (Game_mode & GM_TEAM))
					{
						RefuseThisPlayer=1;
						HUD_init_message ("Player accepted!");
						RefuseTeam=1;
						game_flush_inputs();
					}
				break;
			case KEY_ALTED + KEY_2:
				if (Netgame.RefusePlayers && WaitForRefuseAnswer && (Game_mode & GM_TEAM))
					{
						RefuseThisPlayer=1;
						HUD_init_message ("Player accepted!");
						RefuseTeam=2;
						game_flush_inputs();
					}
				break;
			KEY_MAC(case KEY_COMMAND+KEY_7:)
			case KEY_F7:
			Show_kill_list = (Show_kill_list+1) % ((Game_mode & GM_TEAM) ? 4 : 3);
			break;
#endif
			case KEY_ALTED+KEY_F7:
			KEY_MAC(case KEY_COMMAND+KEY_ALTED+KEY_7:)
			PlayerCfg.HudMode=(PlayerCfg.HudMode+1)%GAUGE_HUD_NUMMODES;
			write_player_file();
			break;
#ifdef NETWORK
			KEY_MAC(case KEY_COMMAND+KEY_8:)
			case KEY_F8:
			multi_send_message_start();
			break;
			
			case KEY_F9:
			case KEY_F10:
			case KEY_F11:
			case KEY_F12:
			multi_send_macro(key);
			break;		// send taunt macros
			
#if defined(__APPLE__) || defined(macintosh)
			case KEY_9 + KEY_COMMAND:
			multi_send_macro(KEY_F9);
			break;
			case KEY_0 + KEY_COMMAND:
			multi_send_macro(KEY_F10);
			break;
			case KEY_1 + KEY_COMMAND + KEY_CTRLED:
			multi_send_macro(KEY_F11);
			break;
			case KEY_2 + KEY_COMMAND + KEY_CTRLED:
			multi_send_macro(KEY_F12);
			break;
#endif
			
			case KEY_SHIFTED + KEY_F9:
			case KEY_SHIFTED + KEY_F10:
			case KEY_SHIFTED + KEY_F11:
			case KEY_SHIFTED + KEY_F12:
			multi_define_macro(key);
			break;		// redefine taunt macros
			
#if defined(__APPLE__) || defined(macintosh)
			case KEY_9 + KEY_SHIFTED + KEY_COMMAND:
			multi_define_macro(KEY_F9);
			break;
			case KEY_0 + KEY_SHIFTED + KEY_COMMAND:
			multi_define_macro(KEY_F10);
			break;
			case KEY_1 + KEY_SHIFTED + KEY_COMMAND + KEY_CTRLED:
			multi_define_macro(KEY_F11);
			break;
			case KEY_2 + KEY_SHIFTED + KEY_COMMAND + KEY_CTRLED:
			multi_define_macro(KEY_F12);
			break;
#endif
			
#endif
			KEY_MAC( case KEY_COMMAND+KEY_P: )
			case KEY_PAUSE:			do_game_pause(); 	break;
#ifdef macintosh
			case KEY_COMMAND + KEY_SHIFTED + KEY_3:
#endif
			case KEY_PRINT_SCREEN:
		{
			if (PlayerCfg.PRShot)
			{
				gr_set_current_canvas(NULL);
				render_frame(0);
				gr_set_curfont(MEDIUM2_FONT);
				gr_printf(SWIDTH-FSPACX(92),SHEIGHT-LINE_SPACING,"DXX-Rebirth\n");
				gr_flip();
			}
			save_screen_shot(0);
			break;
		}
			
			KEY_MAC(case KEY_COMMAND+KEY_S:)
			KEY_MAC(case KEY_COMMAND+KEY_ALTED+KEY_2:)
			case KEY_ALTED+KEY_F2:	if (!Player_is_dead) state_save_all( 0 );		break;	// 0 means not between levels.
			KEY_MAC(case KEY_COMMAND+KEY_O:)
			KEY_MAC(case KEY_COMMAND+KEY_ALTED+KEY_3:)
			case KEY_ALTED+KEY_F3:	if (!Player_is_dead) state_restore_all(1);		break;
			
			/*
			 * Jukebox hotkeys -- MD2211, 2007
			 * Now for all music
			 * ==============================================
			 */
			case KEY_ALTED + KEY_SHIFTED + KEY_F9:
			KEY_MAC(case KEY_COMMAND+KEY_E:)
			songs_stop_extmusic();
			ext_music_eject_disk();
			break;
			
			case KEY_ALTED + KEY_SHIFTED + KEY_F10:
			KEY_MAC(case KEY_COMMAND+KEY_UP:)
			KEY_MAC(case KEY_COMMAND+KEY_DOWN:)
			if (EXT_MUSIC_ON && !ext_music_pause_resume())
			{
				if (Function_mode == FMODE_GAME)
					songs_play_level_song( Current_level_num );
				else if (Function_mode == FMODE_MENU)
					songs_play_song(SONG_TITLE, 1);
			}
			break;
			
			case KEY_MINUS + KEY_ALTED:
			case KEY_ALTED + KEY_SHIFTED + KEY_F11:
			KEY_MAC(case KEY_COMMAND+KEY_LEFT:)
			songs_goto_prev_song();
			break;
			case KEY_EQUAL + KEY_ALTED:
			case KEY_ALTED + KEY_SHIFTED + KEY_F12:
			KEY_MAC(case KEY_COMMAND+KEY_RIGHT:)
			songs_goto_next_song();
			break;
			
#if defined(__APPLE__) || defined(macintosh)
			case KEY_COMMAND+KEY_Q:
			macintosh_quit();
			break;
#endif
			
			//use function keys for window sizing
			
			//	================================================================================================
			//ALL KEYS BELOW HERE GO AWAY IN RELEASE VERSION
			
#ifndef NDEBUG
#ifndef RELEASE
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
				Players[Player_num].cloak_time = (GameTime+CLOAK_TIME_MAX>i2f(0x7fff-600)?GameTime-i2f(0x7fff-600):GameTime);
			}
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
			case KEY_DEBUGGED+KEY_O: toggle_outline_mode(); break;
			case KEY_DEBUGGED+KEY_T:
			*Toggle_var = !*Toggle_var;
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
				hud_message( MSGC_GAME_FEEDBACK, "Debug Spew: ON" );
			} else {
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
			KEY_MAC(case KEY_COMMAND+KEY_F:)
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
			case KEY_DEBUGGED+KEY_D:
			if ((GameArg.DbgUseDoubleBuffer = !GameArg.DbgUseDoubleBuffer)!=0)
				init_cockpit();
			break;
			case KEY_DEBUGGED+KEY_G: 
			GameTime = i2f(0x7fff - 600) - (F1_0*10);
			HUD_init_message("GameTime %i - Reset in 10 seconds!", GameTime);
			break;
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

sbyte	Enable_john_cheat_1, Enable_john_cheat_2, Enable_john_cheat_3, Enable_john_cheat_4;
int	cheat_enable_index;
#define CHEAT_ENABLE_LENGTH (sizeof(cheat_enable_keys) / sizeof(*cheat_enable_keys))
ubyte	cheat_enable_keys[] = {KEY_G,KEY_A,KEY_B,KEY_B,KEY_A,KEY_G,KEY_A,KEY_B,KEY_B,KEY_A,KEY_H,KEY_E,KEY_Y};
ubyte	cheat_wowie[] = {KEY_S,KEY_C,KEY_O,KEY_U,KEY_R,KEY_G,KEY_E};
ubyte	cheat_allkeys[] = {KEY_M,KEY_I,KEY_T,KEY_Z,KEY_I};
ubyte	cheat_invuln[] = {KEY_R,KEY_A,KEY_C,KEY_E,KEY_R,KEY_X};
ubyte	cheat_cloak[] = {KEY_G,KEY_U,KEY_I,KEY_L,KEY_E};
ubyte	cheat_shield[] = {KEY_T,KEY_W,KEY_I,KEY_L,KEY_I,KEY_G,KEY_H,KEY_T};
ubyte	cheat_warp[] = {KEY_F,KEY_A,KEY_R,KEY_M,KEY_E,KEY_R,KEY_J,KEY_O,KEY_E};
ubyte	cheat_astral[] = {KEY_A,KEY_S,KEY_T,KEY_R,KEY_A,KEY_L};

#define NUM_NEW_CHEATS 5
ubyte new_cheats[]= {	KEY_B^0xaa, KEY_B^0xaa, KEY_B^0xaa, KEY_F^0xaa, KEY_A^0xaa,
	KEY_U^0xaa, KEY_I^0xaa, KEY_R^0xaa, KEY_L^0xaa, KEY_H^0xaa,
	KEY_G^0xaa, KEY_G^0xaa, KEY_U^0xaa, KEY_A^0xaa, KEY_I^0xaa,
	KEY_G^0xaa, KEY_R^0xaa, KEY_I^0xaa, KEY_S^0xaa, KEY_M^0xaa,
	KEY_I^0xaa, KEY_E^0xaa, KEY_N^0xaa, KEY_H^0xaa, KEY_S^0xaa,
KEY_N^0xaa, KEY_D^0xaa, KEY_X^0xaa, KEY_X^0xaa, KEY_A^0xaa };

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

extern void john_cheat_func_1(int);
extern void john_cheat_func_2(int);
extern void john_cheat_func_3(int);
extern void john_cheat_func_4(int);

void FinalCheatsKey(int key)
{
	if (key == 0) return;
	
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
	
	if (Cheats_enabled)
	{
		john_cheat_func_2(key);
		
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
					Players[Player_num].cloak_time = (GameTime+CLOAK_TIME_MAX>i2f(0x7fff-600)?GameTime-i2f(0x7fff-600):GameTime);
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
		
		john_cheat_func_3(key);
		john_cheat_func_4(key);
		bald_guy_cheat(key);
	}
}

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

int test_sound_num=0;
int sound_nums[] = {10,11,20,21,30,31,32,33,40,41,50,51,60,61,62,70,80,81,82,83,90,91};

#define N_TEST_SOUNDS (sizeof(sound_nums) / sizeof(*sound_nums))


//	------------------------------------------------------------------------------------
void advance_sound()
{
	if (++test_sound_num == N_TEST_SOUNDS)
		test_sound_num=0;
}

void play_test_sound()
{
	digi_play_sample(sound_nums[test_sound_num], F1_0);
}

void ReadControls()
{
	int key;
	static ubyte exploding_flag=0;
	
	Player_fired_laser_this_frame=-1;
	
	if (!Endlevel_sequence && !con_render) {  // && !Player_is_dead  //this was taken out of the if statement by WraithX
		
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
		if ( Controls.automap_down_count && !((Game_mode & GM_MULTI) && Control_center_destroyed && (Fuelcen_seconds_left < 10)))
			Automap_flag = 1;
		
		do_weapon_stuff();
	}
	
	if (Player_exploded && !con_render) {
		
		if (exploding_flag==0)  {
			exploding_flag = 1;			// When player starts exploding, clear all input devices...
			game_flush_inputs();
		} else {
			int i;
			
			for (i=0; i < JOY_MAX_BUTTONS; i++ )
				if (joy_get_button_down_cnt(i) > 0) Death_sequence_aborted = 1;
			
			for (i = 0; i < MOUSE_MAX_BUTTONS; i++)
				if (mouse_button_down_count(i) > 0) Death_sequence_aborted = 1;
			
			if (Death_sequence_aborted)
				game_flush_inputs();
		}
	} else {
		exploding_flag=0;
	}
	
	if (Newdemo_state == ND_STATE_PLAYBACK )
		update_vcr_state();
	
	key=key_inkey();
	
	if (con_events(key) && con_render)
		return;
	
#ifdef NETWORK
	if ( (Game_mode & GM_MULTI) && (multi_sending_message || multi_defining_message) )
	{
		multi_message_input_sub(key);
		return;
	}
#endif
	
	if (Player_is_dead)
		HandleDeathKey(key);
	
	if (Endlevel_sequence)
		HandleEndlevelKey(key);
	else if (Newdemo_state == ND_STATE_PLAYBACK )
		HandleDemoKey(key);
	else
	{
		FinalCheatsKey(key);
		HandleGameKey(key);
	}
}

