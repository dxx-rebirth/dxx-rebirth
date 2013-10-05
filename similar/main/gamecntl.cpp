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

#define __STDC_FORMAT_MACROS	// For PRIi64

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "pstypes.h"
#include "window.h"
#include "console.h"
#include "inferno.h"
#include "game.h"
#include "player.h"
#include "key.h"
#include "object.h"
#include "menu.h"
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
#include "cntrlcen.h"
#include "fuelcen.h"
#include "pcx.h"
#include "state.h"
#include "piggy.h"
#include "multibot.h"
#include "ai.h"
#include "rbaudio.h"
#include "switch.h"
#if defined(DXX_BUILD_DESCENT_II)
#include "escort.h"
#include "movie.h"
#endif
#include "window.h"

#ifdef EDITOR
#include "editor/editor.h"
#include "editor/esegment.h"
#endif

#include <SDL.h>

using std::min;

// Global Variables -----------------------------------------------------------

int	Debug_spew;

//	Function prototypes --------------------------------------------------------
#ifndef RELEASE
static void do_cheat_menu();
static void advance_sound();
static void play_test_sound();
#endif

#define key_isfunc(k) (((k&0xff)>=KEY_F1 && (k&0xff)<=KEY_F10) || (k&0xff)==KEY_F11 || (k&0xff)==KEY_F12)

// Functions ------------------------------------------------------------------

#if defined(DXX_BUILD_DESCENT_II)
#define CONVERTER_RATE  20		//10 units per second xfer rate
#define CONVERTER_SCALE  2		//2 units energy -> 1 unit shields

#define CONVERTER_SOUND_DELAY (f1_0/2)		//play every half second

static void transfer_energy_to_shield()
{
	fix e;		//how much energy gets transfered
	static fix64 last_play_time=0;

	e = min(min(FrameTime*CONVERTER_RATE,Players[Player_num].energy - INITIAL_ENERGY),(MAX_SHIELDS-Players[Player_num].shields)*CONVERTER_SCALE);

	if (e <= 0) {

		if (Players[Player_num].energy <= INITIAL_ENERGY) {
			HUD_init_message(HM_DEFAULT, "Need more than %i energy to enable transfer", f2i(INITIAL_ENERGY));
		}
		else if (Players[Player_num].shields >= MAX_SHIELDS) {
			HUD_init_message_literal(HM_DEFAULT, "No transfer: Shields already at max");
		}
		return;
	}

	Players[Player_num].energy  -= e;
	Players[Player_num].shields += e/CONVERTER_SCALE;

	if (last_play_time > GameTime64)
		last_play_time = 0;

	if (GameTime64 > last_play_time+CONVERTER_SOUND_DELAY) {
		digi_play_sample_once(SOUND_CONVERT_ENERGY, F1_0);
		last_play_time = GameTime64;
	}

}
#endif


// Control Functions

static fix64 newdemo_single_frame_time;

static void update_vcr_state(void)
{
	if ((keyd_pressed[KEY_LSHIFT] || keyd_pressed[KEY_RSHIFT]) && keyd_pressed[KEY_RIGHT] && d_tick_step)
		Newdemo_vcr_state = ND_STATE_FASTFORWARD;
	else if ((keyd_pressed[KEY_LSHIFT] || keyd_pressed[KEY_RSHIFT]) && keyd_pressed[KEY_LEFT] && d_tick_step)
		Newdemo_vcr_state = ND_STATE_REWINDING;
	else if (!(keyd_pressed[KEY_LCTRL] || keyd_pressed[KEY_RCTRL]) && keyd_pressed[KEY_RIGHT] && ((GameTime64 - newdemo_single_frame_time) >= F1_0) && d_tick_step)
		Newdemo_vcr_state = ND_STATE_ONEFRAMEFORWARD;
	else if (!(keyd_pressed[KEY_LCTRL] || keyd_pressed[KEY_RCTRL]) && keyd_pressed[KEY_LEFT] && ((GameTime64 - newdemo_single_frame_time) >= F1_0) && d_tick_step)
		Newdemo_vcr_state = ND_STATE_ONEFRAMEBACKWARD;
	else if ((Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_REWINDING))
		Newdemo_vcr_state = ND_STATE_PLAYBACK;
}

#if defined(DXX_BUILD_DESCENT_II)
//returns which bomb will be dropped next time the bomb key is pressed
int which_bomb()
{
	int bomb;

	//use the last one selected, unless there aren't any, in which case use
	//the other if there are any


   // If hoard game, only let the player drop smart mines
   if (game_mode_hoard())
		return SMART_MINE_INDEX;

	bomb = Secondary_last_was_super[PROXIMITY_INDEX]?SMART_MINE_INDEX:PROXIMITY_INDEX;

	if (Players[Player_num].secondary_ammo[bomb] == 0 &&
			Players[Player_num].secondary_ammo[SMART_MINE_INDEX+PROXIMITY_INDEX-bomb] != 0) {
		bomb = SMART_MINE_INDEX+PROXIMITY_INDEX-bomb;
		Secondary_last_was_super[bomb%SUPER_WEAPON] = (bomb == SMART_MINE_INDEX);
	}



	return bomb;
}
#endif

static void do_weapon_n_item_stuff()
{
	int i;

	if (Controls.fire_flare_count > 0)
	{
		Controls.fire_flare_count = 0;
		if (allowed_to_fire_flare())
			Flare_create(ConsoleObject);
	}

	if (allowed_to_fire_missile() && Controls.fire_secondary_state)
		Global_missile_firing_count += Weapon_info[Secondary_weapon_to_weapon_info[Secondary_weapon]].fire_count;

	if (Global_missile_firing_count) {
		do_missile_firing(0);
		Global_missile_firing_count--;
	}

	if (Controls.cycle_primary_count > 0)
	{
		for (i=0;i<Controls.cycle_primary_count;i++)
			CyclePrimary ();
		Controls.cycle_primary_count = 0;
	}
	if (Controls.cycle_secondary_count > 0)
	{
		for (i=0;i<Controls.cycle_secondary_count;i++)
			CycleSecondary ();
		Controls.cycle_secondary_count = 0;
	}
	if (Controls.select_weapon_count > 0)
	{
		Controls.select_weapon_count--;
		do_weapon_select(Controls.select_weapon_count>4?Controls.select_weapon_count-5:Controls.select_weapon_count,Controls.select_weapon_count>4?1:0);
		Controls.select_weapon_count = 0;
	}
#if defined(DXX_BUILD_DESCENT_II)
	if (Controls.headlight_count > 0)
	{
		for (i=0;i<Controls.headlight_count;i++)
			toggle_headlight_active ();
		Controls.headlight_count = 0;
	}
#endif

	if (Global_missile_firing_count < 0)
		Global_missile_firing_count = 0;

	//	Drop proximity bombs.
	while (Controls.drop_bomb_count > 0)
	{
		do_missile_firing(1);
		Controls.drop_bomb_count--;
	}
#if defined(DXX_BUILD_DESCENT_II)
	if (Controls.toggle_bomb_count > 0)
	{
		int bomb = Secondary_last_was_super[PROXIMITY_INDEX]?PROXIMITY_INDEX:SMART_MINE_INDEX;
	
		if (!Players[Player_num].secondary_ammo[PROXIMITY_INDEX] && !Players[Player_num].secondary_ammo[SMART_MINE_INDEX])
		{
			digi_play_sample_once( SOUND_BAD_SELECTION, F1_0 );
			HUD_init_message_literal(HM_DEFAULT, "No bombs available!");
		}
		else
		{	
			if (Players[Player_num].secondary_ammo[bomb]==0)
			{
				digi_play_sample_once( SOUND_BAD_SELECTION, F1_0 );
				HUD_init_message(HM_DEFAULT, "No %s available!",(bomb==SMART_MINE_INDEX)?"Smart mines":"Proximity bombs");
			}
			else
			{
				Secondary_last_was_super[PROXIMITY_INDEX]=!Secondary_last_was_super[PROXIMITY_INDEX];
				digi_play_sample_once( SOUND_GOOD_SELECTION_SECONDARY, F1_0 );
			}
		}
		Controls.toggle_bomb_count = 0;
	}

	if (Controls.energy_to_shield_state && (Players[Player_num].flags & PLAYER_FLAGS_CONVERTER))
		transfer_energy_to_shield();
#endif
}

static void format_time(char *str, int secs_int)
{
	int h, m, s;

	h = secs_int/3600;
	s = secs_int%3600;
	m = s / 60;
	s = s % 60;
	sprintf(str, "%1d:%02d:%02d", h, m, s );
}

//Process selected keys until game unpaused
static int pause_handler(window *wind, d_event *event, char *msg)
{
	int key;

	switch (event->type)
	{
		case EVENT_WINDOW_ACTIVATED:
			game_flush_inputs();
			break;

		case EVENT_KEY_COMMAND:
			key = event_key_get(event);

			switch (key)
			{
				case 0:
					break;
				case KEY_ESC:
					window_close(wind);
					return 1;
				case KEY_F1:
					show_help();
					return 1;
				case KEY_PAUSE:
					window_close(wind);
					return 1;
				default:
					break;
			}
			break;

		case EVENT_IDLE:
			timer_delay2(50);
			break;

		case EVENT_WINDOW_DRAW:
			show_boxed_message(msg, 1);
			break;

		case EVENT_WINDOW_CLOSE:
			songs_resume();
			d_free(msg);
			break;

		default:
			break;
	}

	return 0;
}

static int do_game_pause()
{
	char *msg;
	char total_time[9],level_time[9];

	if (Game_mode & GM_MULTI)
	{
		netplayerinfo_on= !netplayerinfo_on;
		return(KEY_PAUSE);
	}

	MALLOC(msg, char, 1024);
	if (!msg)
		return 0;

	songs_pause();

	format_time(total_time, f2i(Players[Player_num].time_total) + Players[Player_num].hours_total*3600);
	format_time(level_time, f2i(Players[Player_num].time_level) + Players[Player_num].hours_level*3600);
	if (Newdemo_state!=ND_STATE_PLAYBACK)
		snprintf(msg,1024,"PAUSE\n\nSkill level:  %s\nHostages on board:  %d\nTime on level: %s\nTotal time in game: %s",MENU_DIFFICULTY_TEXT(Difficulty_level),Players[Player_num].hostages_on_board,level_time,total_time);
	else
	  	snprintf(msg,1024,"PAUSE\n\nSkill level:  %s\nHostages on board:  %d\n",MENU_DIFFICULTY_TEXT(Difficulty_level),Players[Player_num].hostages_on_board);
	set_screen_mode(SCREEN_MENU);

	if (!window_create(&grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT, (int (*)(window *, d_event *, void *))pause_handler, msg))
		d_free(msg);

	return 0 /*key*/;	// Keycode returning ripped out (kreatordxx)
}

static int HandleEndlevelKey(int key)
{
	switch (key)
	{
		case KEY_COMMAND+KEY_P:
		case KEY_PAUSE:
			do_game_pause();
			return 1;

		case KEY_ESC:
			stop_endlevel_sequence();
			last_drawn_cockpit=-1;
			return 1;
	}

	return 0;
}

static int HandleDeathInput(d_event *event)
{
	if (event->type == EVENT_KEY_COMMAND)
	{
		int key = event_key_get(event);

		if (Player_exploded && !key_isfunc(key) && key != KEY_PAUSE && key)
			Death_sequence_aborted  = 1;		//Any key but func or modifier aborts
		if (key == KEY_ESC)
			if (ConsoleObject->flags & OF_EXPLODING)
				Death_sequence_aborted = 1;
	}

	if (Player_exploded && (event->type == EVENT_JOYSTICK_BUTTON_UP || event->type == EVENT_MOUSE_BUTTON_UP))
		Death_sequence_aborted = 1;

	if (Death_sequence_aborted)
	{
		game_flush_inputs();
		return 1;
	}

	return 0;
}

static int HandleDemoKey(int key)
{
	switch (key) {
		KEY_MAC(case KEY_COMMAND+KEY_1:)
		case KEY_F1:	show_newdemo_help();	break;
		KEY_MAC(case KEY_COMMAND+KEY_2:)
		case KEY_F2:	do_options_menu();	break;
		KEY_MAC(case KEY_COMMAND+KEY_3:)
		case KEY_F3:
			 if (Viewer->type == OBJ_PLAYER)
				toggle_cockpit();
			 break;
		KEY_MAC(case KEY_COMMAND+KEY_4:)
		case KEY_F4:	Newdemo_show_percentage = !Newdemo_show_percentage; break;
		KEY_MAC(case KEY_COMMAND+KEY_7:)
		case KEY_F7:
			Show_kill_list = (Show_kill_list+1) % ((Newdemo_game_mode & GM_TEAM) ? 4 : 3);
			break;
		case KEY_ESC:
			if (GameArg.SysAutoDemo)
			{
				int choice;
				choice = nm_messagebox( NULL, 2, TXT_YES, TXT_NO, TXT_ABORT_AUTODEMO );
				if (choice == 0)
					GameArg.SysAutoDemo = 0;
				else
					break;
			}
			newdemo_stop_playback();
			break;
		case KEY_UP:
			Newdemo_vcr_state = ND_STATE_PLAYBACK;
			break;
		case KEY_DOWN:
			Newdemo_vcr_state = ND_STATE_PAUSED;
			break;
		case KEY_LEFT:
			newdemo_single_frame_time = GameTime64;
			Newdemo_vcr_state = ND_STATE_ONEFRAMEBACKWARD;
			break;
		case KEY_RIGHT:
			newdemo_single_frame_time = GameTime64;
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
				render_frame(0, 0);
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
				game_render_frame_mono(0);
				if (GameArg.DbgUseDoubleBuffer)
					gr_flip();
				save_screen_shot(0);
				Newdemo_show_percentage = old_state;
			}
			break;
		}
#ifndef NDEBUG
		case KEY_DEBUGGED + KEY_I:
			Newdemo_do_interpolate = !Newdemo_do_interpolate;
			HUD_init_message(HM_DEFAULT, "Demo playback interpolation %s", Newdemo_do_interpolate?"ON":"OFF");
			break;
		case KEY_DEBUGGED + KEY_K: {
			int how_many, c;
			char filename[FILENAME_LEN], num[16];
			newmenu_item m[6];

			filename[0] = '\0';
			nm_set_item_text(& m[ 0], "output file name");
			nm_set_item_input(&m[ 1], 8, filename);
			c = newmenu_do( NULL, NULL, 2, m, NULL, NULL );
			if (c == -2)
				break;
			strcat(filename, DEMO_EXT);
			num[0] = '\0';
			nm_set_item_text(& m[ 0], "strip how many bytes");
			nm_set_item_input(&m[ 1], 16, num);
			c = newmenu_do( NULL, NULL, 2, m, NULL, NULL );
			if (c == -2)
				break;
			how_many = atoi(num);
			if (how_many <= 0)
				break;
			newdemo_strip_frames(filename, how_many);

			break;
		}
#endif

		default:
			return 0;
	}

	return 1;
}

#if defined(DXX_BUILD_DESCENT_II)
//switch a cockpit window to the next function
static int select_next_window_function(int w)
{
	Assert(w==0 || w==1);

	switch (PlayerCfg.Cockpit3DView[w]) {
		case CV_NONE:
			PlayerCfg.Cockpit3DView[w] = CV_REAR;
			break;
		case CV_REAR:
			if (find_escort()) {
				PlayerCfg.Cockpit3DView[w] = CV_ESCORT;
				break;
			}
			//if no ecort, fall through
		case CV_ESCORT:
			Coop_view_player[w] = -1;		//force first player
			//fall through
		case CV_COOP:
			Marker_viewer_num[w] = -1;
			if ((Game_mode & GM_MULTI_COOP) || (Game_mode & GM_TEAM)) {
				PlayerCfg.Cockpit3DView[w] = CV_COOP;
				while (1) {
					Coop_view_player[w]++;
					if (Coop_view_player[w] == (MAX_PLAYERS-1)) {
						PlayerCfg.Cockpit3DView[w] = CV_MARKER;
						goto case_marker;
					}
					if (Players[Coop_view_player[w]].connected != CONNECT_PLAYING)
						continue;
					if (Coop_view_player[w]==Player_num)
						continue;

					if (Game_mode & GM_MULTI_COOP)
						break;
					else if (get_team(Coop_view_player[w]) == get_team(Player_num))
						break;
				}
				break;
			}
			//if not multi, fall through
		case CV_MARKER:
		case_marker:;
			if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP) && Netgame.Allow_marker_view) {	//anarchy only
				PlayerCfg.Cockpit3DView[w] = CV_MARKER;
				if (Marker_viewer_num[w] == -1)
					Marker_viewer_num[w] = Player_num * 2;
				else if (Marker_viewer_num[w] == Player_num * 2)
					Marker_viewer_num[w]++;
				else
					PlayerCfg.Cockpit3DView[w] = CV_NONE;
			}
			else
				PlayerCfg.Cockpit3DView[w] = CV_NONE;
			break;
	}
	write_player_file();

	return 1;	 //screen_changed
}
#endif

#ifdef DOOR_DEBUGGING
dump_door_debugging_info()
{
	object *obj;
	vms_vector new_pos;
	fvi_query fq;
	fvi_info hit_info;
	int fate;
	PHYSFS_file *dfile;
	int wall_num;

	obj = &Objects[Players[Player_num].objnum];
	vm_vec_scale_add(&new_pos,&obj->pos,&obj->orient.fvec,i2f(100));

	fq.p0						= &obj->pos;
	fq.startseg				= obj->segnum;
	fq.p1						= &new_pos;
	fq.rad					= 0;
	fq.thisobjnum			= Players[Player_num].objnum;
	fq.ignore_obj_list	= NULL;
	fq.flags					= 0;

	fate = find_vector_intersection(&fq,&hit_info);

	dfile = PHYSFSX_openWriteBuffered("door.out");

	PHYSFSX_printf(dfile,"FVI hit_type = %d\n",fate);
	PHYSFSX_printf(dfile,"    hit_seg = %d\n",hit_info.hit_seg);
	PHYSFSX_printf(dfile,"    hit_side = %d\n",hit_info.hit_side);
	PHYSFSX_printf(dfile,"    hit_side_seg = %d\n",hit_info.hit_side_seg);
	PHYSFSX_printf(dfile,"\n");

	if (fate == HIT_WALL) {

		wall_num = Segments[hit_info.hit_seg].sides[hit_info.hit_side].wall_num;
		PHYSFSX_printf(dfile,"wall_num = %d\n",wall_num);

		if (wall_num != -1) {
			wall *wall = &Walls[wall_num];
			active_door *d;
			int i;

			PHYSFSX_printf(dfile,"    segnum = %d\n",wall->segnum);
			PHYSFSX_printf(dfile,"    sidenum = %d\n",wall->sidenum);
			PHYSFSX_printf(dfile,"    hps = %x\n",wall->hps);
			PHYSFSX_printf(dfile,"    linked_wall = %d\n",wall->linked_wall);
			PHYSFSX_printf(dfile,"    type = %d\n",wall->type);
			PHYSFSX_printf(dfile,"    flags = %x\n",wall->flags);
			PHYSFSX_printf(dfile,"    state = %d\n",wall->state);
			PHYSFSX_printf(dfile,"    trigger = %d\n",wall->trigger);
			PHYSFSX_printf(dfile,"    clip_num = %d\n",wall->clip_num);
			PHYSFSX_printf(dfile,"    keys = %x\n",wall->keys);
			PHYSFSX_printf(dfile,"    controlling_trigger = %d\n",wall->controlling_trigger);
			PHYSFSX_printf(dfile,"    cloak_value = %d\n",wall->cloak_value);
			PHYSFSX_printf(dfile,"\n");


			for (i=0;i<Num_open_doors;i++) {		//find door
				d = &ActiveDoors[i];
				if (d->front_wallnum[0]==wall-Walls || d->back_wallnum[0]==wall-Walls || (d->n_parts==2 && (d->front_wallnum[1]==wall-Walls || d->back_wallnum[1]==wall-Walls)))
					break;
			}

			if (i>=Num_open_doors)
				PHYSFSX_printf(dfile,"No active door.\n");
			else {
				PHYSFSX_printf(dfile,"Active door %d:\n",i);
				PHYSFSX_printf(dfile,"    n_parts = %d\n",d->n_parts);
				PHYSFSX_printf(dfile,"    front_wallnum = %d,%d\n",d->front_wallnum[0],d->front_wallnum[1]);
				PHYSFSX_printf(dfile,"    back_wallnum = %d,%d\n",d->back_wallnum[0],d->back_wallnum[1]);
				PHYSFSX_printf(dfile,"    time = %x\n",d->time);
			}

		}
	}

	PHYSFSX_printf(dfile,"\n");
	PHYSFSX_printf(dfile,"\n");

	PHYSFS_close(dfile);

}
#endif

//this is for system-level keys, such as help, etc.
//returns 1 if screen changed
static int HandleSystemKey(int key)
{
	if (!Player_is_dead)
		switch (key)
		{

			#ifdef DOOR_DEBUGGING
			case KEY_LAPOSTRO+KEY_SHIFTED:
				dump_door_debugging_info();
				return 1;
			#endif

			case KEY_ESC:
			{
				int choice;
				choice=nm_messagebox( NULL, 2, TXT_YES, TXT_NO, TXT_ABORT_GAME );
				if (choice == 0)
					window_close(Game_wind);

				return 1;
			}
#if defined(DXX_BUILD_DESCENT_II)
// fleshed these out because F1 and F2 aren't sequenctial keycodes on mac -- MWA

			KEY_MAC(case KEY_COMMAND+KEY_SHIFTED+KEY_1:)
			case KEY_SHIFTED+KEY_F1:
				select_next_window_function(0);
				return 1;
			KEY_MAC(case KEY_COMMAND+KEY_SHIFTED+KEY_2:)
			case KEY_SHIFTED+KEY_F2:
				select_next_window_function(1);
				return 1;
#endif
		}

	switch (key)
	{
		KEY_MAC( case KEY_COMMAND+KEY_P: )
		case KEY_PAUSE:
			do_game_pause();	break;


#ifdef macintosh
		case KEY_COMMAND + KEY_SHIFTED + KEY_3:
#endif
		case KEY_PRINT_SCREEN:
		{
			if (PlayerCfg.PRShot)
			{
				gr_set_current_canvas(NULL);
				render_frame(0, 0);
				gr_set_curfont(MEDIUM2_FONT);
				gr_printf(SWIDTH-FSPACX(92),SHEIGHT-LINE_SPACING,"DXX-Rebirth\n");
				gr_flip();
				save_screen_shot(0);
			}
			else
			{
				game_render_frame_mono(0);
				if(GameArg.DbgUseDoubleBuffer)
					gr_flip();
				save_screen_shot(0);
			}
			break;
		}

		KEY_MAC(case KEY_COMMAND+KEY_1:)
		case KEY_F1:				if (Game_mode & GM_MULTI) show_netgame_help(); else show_help();	break;

		KEY_MAC(case KEY_COMMAND+KEY_2:)
		case KEY_F2:
			{
				do_options_menu();
				break;
			}


		KEY_MAC(case KEY_COMMAND+KEY_3:)

		case KEY_F3:
			if (!Player_is_dead && Viewer->type==OBJ_PLAYER) //if (!(Guided_missile[Player_num] && Guided_missile[Player_num]->type==OBJ_WEAPON && Guided_missile[Player_num]->id==GUIDEDMISS_ID && Guided_missile[Player_num]->signature==Guided_missile_sig[Player_num] && PlayerCfg.GuidedInBigWindow))
			{
				toggle_cockpit();
			}
			break;

		KEY_MAC(case KEY_COMMAND+KEY_5:)
		case KEY_F5:
			if ( Newdemo_state == ND_STATE_RECORDING )
				newdemo_stop_recording();
			else if ( Newdemo_state == ND_STATE_NORMAL )
				newdemo_start_recording();
			break;
		KEY_MAC(case KEY_COMMAND+KEY_ALTED+KEY_4:)
		case KEY_ALTED + KEY_F4:
			Show_reticle_name = (Show_reticle_name+1)%2;
			break;

		KEY_MAC(case KEY_COMMAND+KEY_7:)
		case KEY_F7:
			Show_kill_list = (Show_kill_list+1) % ((Game_mode & GM_TEAM) ? 4 : 3);
			if (Game_mode & GM_MULTI)
				multi_sort_kill_list();
			break;

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


		KEY_MAC(case KEY_COMMAND+KEY_SHIFTED+KEY_S:)
		KEY_MAC(case KEY_COMMAND+KEY_ALTED+KEY_2:)
		case KEY_ALTED+KEY_F2:
			if (!Player_is_dead)
				state_save_all(0, NULL, 0); // 0 means not between levels.
			break;

		KEY_MAC(case KEY_COMMAND+KEY_S:)
		case KEY_ALTED+KEY_F1:
			if (!Player_is_dead)
				state_save_all(0, NULL, 1);
			break;
		KEY_MAC(case KEY_COMMAND+KEY_O:)
		KEY_MAC(case KEY_COMMAND+KEY_ALTED+KEY_3:)
		case KEY_ALTED+KEY_F3:
			if (!((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP)))
				state_restore_all(1, 0, NULL);
			break;

#if defined(DXX_BUILD_DESCENT_II)
		KEY_MAC(case KEY_COMMAND+KEY_SHIFTED+KEY_4:)
		case KEY_F4 + KEY_SHIFTED:
			do_escort_menu();
			break;


		KEY_MAC(case KEY_COMMAND+KEY_SHIFTED+KEY_ALTED+KEY_4:)
		case KEY_F4 + KEY_SHIFTED + KEY_ALTED:
			change_guidebot_name();
			break;
#endif

			/*
			 * Jukebox hotkeys -- MD2211, 2007
			 * Now for all music
			 * ==============================================
			 */
		case KEY_ALTED + KEY_SHIFTED + KEY_F9:
		KEY_MAC(case KEY_COMMAND+KEY_E:)
			if (GameCfg.MusicType == MUSIC_TYPE_REDBOOK)
			{
				songs_stop_all();
				RBAEjectDisk();
			}
			break;

		case KEY_ALTED + KEY_SHIFTED + KEY_F10:
		KEY_MAC(case KEY_COMMAND+KEY_UP:)
		KEY_MAC(case KEY_COMMAND+KEY_DOWN:)
			songs_pause_resume();
			break;

		case KEY_MINUS + KEY_ALTED:
		case KEY_ALTED + KEY_SHIFTED + KEY_F11:
		KEY_MAC(case KEY_COMMAND+KEY_LEFT:)
			songs_play_level_song( Current_level_num, -1 );
			break;
		case KEY_EQUAL + KEY_ALTED:
		case KEY_ALTED + KEY_SHIFTED + KEY_F12:
		KEY_MAC(case KEY_COMMAND+KEY_RIGHT:)
			songs_play_level_song( Current_level_num, 1 );
			break;

		default:
			return 0;
			break;
	}

	return 1;
}

static int HandleGameKey(int key)
{
	switch (key) {
#if defined(DXX_BUILD_DESCENT_II)
		case KEY_1 + KEY_SHIFTED:
		case KEY_2 + KEY_SHIFTED:
		case KEY_3 + KEY_SHIFTED:
		case KEY_4 + KEY_SHIFTED:
		case KEY_5 + KEY_SHIFTED:
		case KEY_6 + KEY_SHIFTED:
		case KEY_7 + KEY_SHIFTED:
		case KEY_8 + KEY_SHIFTED:
		case KEY_9 + KEY_SHIFTED:
		case KEY_0 + KEY_SHIFTED:
			if (PlayerCfg.EscortHotKeys)
			{
				if (!(Game_mode & GM_MULTI))
					set_escort_special_goal(key);
				else
					HUD_init_message_literal(HM_DEFAULT, "No Guide-Bot in Multiplayer!");
				game_flush_inputs();
				return 1;
			}
			else
				return 0;
#endif

		case KEY_ALTED+KEY_F7:
		KEY_MAC(case KEY_COMMAND+KEY_ALTED+KEY_7:)
			PlayerCfg.HudMode=(PlayerCfg.HudMode+1)%GAUGE_HUD_NUMMODES;
			write_player_file();
			switch (PlayerCfg.HudMode)
			{
				case 0: HUD_init_message_literal(HM_DEFAULT, "Standard HUD"); break;
				case 1: HUD_init_message_literal(HM_DEFAULT, "Alternative HUD #1"); break;
				case 2: HUD_init_message_literal(HM_DEFAULT, "Alternative HUD #2"); break;
				case 3: HUD_init_message_literal(HM_DEFAULT, "No HUD"); break;
			}
			return 1;

		KEY_MAC(case KEY_COMMAND+KEY_6:)
		case KEY_F6:
			if (Netgame.RefusePlayers && WaitForRefuseAnswer && !(Game_mode & GM_TEAM))
			{
				RefuseThisPlayer=1;
				HUD_init_message_literal(HM_MULTI, "Player accepted!");
			}
			return 1;
		case KEY_ALTED + KEY_1:
			if (Netgame.RefusePlayers && WaitForRefuseAnswer && (Game_mode & GM_TEAM))
				{
					RefuseThisPlayer=1;
					HUD_init_message_literal(HM_MULTI, "Player accepted!");
					RefuseTeam=1;
					game_flush_inputs();
				}
			return 1;
		case KEY_ALTED + KEY_2:
			if (Netgame.RefusePlayers && WaitForRefuseAnswer && (Game_mode & GM_TEAM))
				{
					RefuseThisPlayer=1;
					HUD_init_message_literal(HM_MULTI, "Player accepted!");
					RefuseTeam=2;
					game_flush_inputs();
				}
			return 1;

		default:
#if defined(DXX_BUILD_DESCENT_I)
			return 0;
#endif
			break;

	}	 //switch (key)

#if defined(DXX_BUILD_DESCENT_II)
	if (!Player_is_dead)
		switch (key)
		{
				KEY_MAC(case KEY_COMMAND+KEY_SHIFTED+KEY_5:)
			case KEY_F5 + KEY_SHIFTED:
				DropCurrentWeapon();
				break;

			KEY_MAC(case KEY_COMMAND+KEY_SHIFTED+KEY_6:)
			case KEY_F6 + KEY_SHIFTED:
				DropSecondaryWeapon();
				break;

			case KEY_0 + KEY_ALTED:
				DropFlag ();
				game_flush_inputs();
				break;

			KEY_MAC(case KEY_COMMAND+KEY_4:)
			case KEY_F4:
				if (!DefiningMarkerMessage)
					InitMarkerInput();
				break;

			default:
				return 0;
		}
	else
		return 0;
#endif

	return 1;
}

#if defined(DXX_BUILD_DESCENT_II)
static void kill_all_robots(void)
{
	int	i, dead_count=0;
	//int	boss_index = -1;

	// Kill all bots except for Buddy bot and boss.  However, if only boss and buddy left, kill boss.
	for (i=0; i<=Highest_object_index; i++)
		if (Objects[i].type == OBJ_ROBOT) {
			if (!Robot_info[Objects[i].id].companion && !Robot_info[Objects[i].id].boss_flag) {
				dead_count++;
				Objects[i].flags |= OF_EXPLODING|OF_SHOULD_BE_DEAD;
			}
		}

// --		// Now, if more than boss and buddy left, un-kill boss.
// --		if ((dead_count > 2) && (boss_index != -1)) {
// --			Objects[boss_index].flags &= ~(OF_EXPLODING|OF_SHOULD_BE_DEAD);
// --			dead_count--;
// --		} else if (boss_index != -1)
// --			HUD_init_message(HM_DEFAULT, "Toasted the BOSS!");

	// Toast the buddy if nothing else toasted!
	if (dead_count == 0)
		for (i=0; i<=Highest_object_index; i++)
			if (Objects[i].type == OBJ_ROBOT)
				if (Robot_info[Objects[i].id].companion) {
					Objects[i].flags |= OF_EXPLODING|OF_SHOULD_BE_DEAD;
					HUD_init_message_literal(HM_DEFAULT, "Toasted the Buddy! *sniff*");
					dead_count++;
				}

	HUD_init_message(HM_DEFAULT, "%i robots toasted!", dead_count);
}
#endif

//	--------------------------------------------------------------------------
//	Detonate reactor.
//	Award player all powerups in mine.
//	Place player just outside exit.
//	Kill all bots in mine.
//	Yippee!!
static void kill_and_so_forth(void)
{
	int     i, j;

	HUD_init_message_literal(HM_DEFAULT, "Killing, awarding, etc.!");

	for (i=0; i<=Highest_object_index; i++) {
		switch (Objects[i].type) {
			case OBJ_ROBOT:
				Objects[i].flags |= OF_EXPLODING|OF_SHOULD_BE_DEAD;
				break;
			case OBJ_POWERUP:
				do_powerup(&Objects[i]);
				break;
		}
	}

	do_controlcen_destroyed_stuff(NULL);

	for (i=0; i<Num_triggers; i++) {
		if (trigger_is_exit(&Triggers[i])) {
			for (j=0; j<Num_walls; j++) {
				if (Walls[j].trigger == i) {
					compute_segment_center(&ConsoleObject->pos, &Segments[Walls[j].segnum]);
					obj_relink(ConsoleObject-Objects,Walls[j].segnum);
					goto kasf_done;
				}
			}
		}
	}

kasf_done: ;

}

#ifndef RELEASE
#if defined(DXX_BUILD_DESCENT_II)
void kill_all_snipers(void)
{
	int     i, dead_count=0;

	//	Kill all snipers.
	for (i=0; i<=Highest_object_index; i++)
		if (Objects[i].type == OBJ_ROBOT)
			if (Objects[i].ctype.ai_info.behavior == AIB_SNIPE) {
				dead_count++;
				Objects[i].flags |= OF_EXPLODING|OF_SHOULD_BE_DEAD;
			}

	HUD_init_message(HM_DEFAULT, "%i robots toasted!", dead_count);
}

void kill_thief(void)
{
	int     i;

	//	Kill thief.
	for (i=0; i<=Highest_object_index; i++)
		if (Objects[i].type == OBJ_ROBOT)
			if (Robot_info[Objects[i].id].thief) {
				Objects[i].flags |= OF_EXPLODING|OF_SHOULD_BE_DEAD;
				HUD_init_message_literal(HM_DEFAULT, "Thief toasted!");
			}
}

void kill_buddy(void)
{
	int     i;

	//	Kill buddy.
	for (i=0; i<=Highest_object_index; i++)
		if (Objects[i].type == OBJ_ROBOT)
			if (Robot_info[Objects[i].id].companion) {
				Objects[i].flags |= OF_EXPLODING|OF_SHOULD_BE_DEAD;
				HUD_init_message_literal(HM_DEFAULT, "Buddy toasted!");
			}
}
#endif

static int HandleTestKey(int key)
{
	switch (key)
	{

#ifdef SHOW_EXIT_PATH
		case KEY_DEBUGGED+KEY_1:	create_special_path();	break;
#endif

		case KEY_DEBUGGED+KEY_Y:
			do_controlcen_destroyed_stuff(NULL);
			break;

#if defined(DXX_BUILD_DESCENT_II)
	case KEY_DEBUGGED+KEY_ALTED+KEY_D:
			PlayerCfg.NetlifeKills=4000; PlayerCfg.NetlifeKilled=5;
			multi_add_lifetime_kills();
			break;

		case KEY_DEBUGGED+KEY_R+KEY_SHIFTED:
			kill_all_robots();
			break;
#endif

		case KEY_BACKSP:
		case KEY_CTRLED+KEY_BACKSP:
		case KEY_ALTED+KEY_BACKSP:
		case KEY_SHIFTED+KEY_BACKSP:
		case KEY_SHIFTED+KEY_ALTED+KEY_BACKSP:
		case KEY_CTRLED+KEY_ALTED+KEY_BACKSP:
		case KEY_SHIFTED+KEY_CTRLED+KEY_BACKSP:
		case KEY_SHIFTED+KEY_CTRLED+KEY_ALTED+KEY_BACKSP:

			Int3(); break;

		case KEY_DEBUGGED+KEY_S:				digi_reset(); break;

		case KEY_DEBUGGED+KEY_P:
			if (Game_suspended & SUSP_ROBOTS)
				Game_suspended &= ~SUSP_ROBOTS;		//robots move
			else
				Game_suspended |= SUSP_ROBOTS;		//robots don't move
			break;



		case KEY_DEBUGGED+KEY_K:	Players[Player_num].shields = 1;	break;							//	a virtual kill
		case KEY_DEBUGGED+KEY_SHIFTED + KEY_K:  Players[Player_num].shields = -1;	 break;  //	an actual kill
		case KEY_DEBUGGED+KEY_X: Players[Player_num].lives++; break; // Extra life cheat key.
		case KEY_DEBUGGED+KEY_H:
			if (Player_is_dead)
				return 0;

			Players[Player_num].flags ^= PLAYER_FLAGS_CLOAKED;
			if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED) {
				if (Game_mode & GM_MULTI)
					multi_send_cloak();
				ai_do_cloak_stuff();
				Players[Player_num].cloak_time = GameTime64;
			}
			break;


		case KEY_DEBUGGED+KEY_R:
			cheats.robotfiringsuspended = !cheats.robotfiringsuspended;
			break;

#ifdef EDITOR		//editor-specific functions

		case KEY_E + KEY_DEBUGGED:
			window_set_visible(Game_wind, 0);	// don't let the game do anything while we set the editor up
			init_editor();
			window_close(Game_wind);
			break;
#if defined(DXX_BUILD_DESCENT_II)
	case KEY_Q + KEY_SHIFTED + KEY_DEBUGGED:
		{
			char pal_save[768];
			memcpy(pal_save,gr_palette,768);
			init_subtitles("end.tex");	//ingore errors
			PlayMovie ("end.mve",MOVIE_ABORT_ON);
			close_subtitles();
			Screen_mode = -1;
			set_screen_mode(SCREEN_GAME);
			reset_cockpit();
			memcpy(gr_palette,pal_save,768);
			gr_palette_load(gr_palette);
			break;
		}
#endif
		case KEY_C + KEY_SHIFTED + KEY_DEBUGGED:
			if (!Player_is_dead && !( Game_mode & GM_MULTI ))
				move_player_2_segment(Cursegp,Curside);
			break;   //move eye to curseg


		case KEY_DEBUGGED+KEY_W:	draw_world_from_game(); break;

		#endif  //#ifdef EDITOR

		case KEY_DEBUGGED+KEY_LAPOSTRO: Show_view_text_timer = 0x30000; object_goto_next_viewer(); break;
		case KEY_DEBUGGED+KEY_SHIFTED+KEY_LAPOSTRO: Viewer=ConsoleObject; break;
		case KEY_DEBUGGED+KEY_O: toggle_outline_mode(); break;
		case KEY_DEBUGGED+KEY_T:
#if defined(DXX_BUILD_DESCENT_II)
		{
			static int Toggle_var;
			Toggle_var = !Toggle_var;
			if (Toggle_var)
				GameArg.SysMaxFPS = 300;
			else
				GameArg.SysMaxFPS = 30;
		}
#endif
			break;
		case KEY_DEBUGGED + KEY_L:
			if (++Lighting_on >= 2) Lighting_on = 0; break;
		case KEY_DEBUGGED + KEY_SHIFTED + KEY_L:
			Beam_brightness=0x38000-Beam_brightness; break;
		case KEY_PAD5: slew_stop(); break;

#ifndef NDEBUG
		case KEY_DEBUGGED + KEY_F11: play_test_sound(); break;
		case KEY_DEBUGGED + KEY_SHIFTED+KEY_F11: advance_sound(); play_test_sound(); break;
#endif

		case KEY_DEBUGGED + KEY_M:
			Debug_spew = !Debug_spew;
			if (Debug_spew) {
				HUD_init_message_literal(HM_DEFAULT,  "Debug Spew: ON" );
			} else {
				HUD_init_message_literal(HM_DEFAULT,  "Debug Spew: OFF" );
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

		case KEY_DEBUGGED+KEY_SPACEBAR:		//KEY_F7:				// Toggle physics flying
			slew_stop();
			game_flush_inputs();
			if ( ConsoleObject->control_type != CT_FLYING ) {
				fly_init(ConsoleObject);
				Game_suspended &= ~SUSP_ROBOTS;	//robots move
			} else {
				slew_init(ConsoleObject);			//start player slewing
				Game_suspended |= SUSP_ROBOTS;	//robots don't move
			}
			break;

		case KEY_DEBUGGED+KEY_COMMA: Render_zoom = fixmul(Render_zoom,62259); break;
		case KEY_DEBUGGED+KEY_PERIOD: Render_zoom = fixmul(Render_zoom,68985); break;

		#ifndef NDEBUG
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

		case KEY_DEBUGGED+KEY_B: {
			newmenu_item m;
			char text[FILENAME_LEN]="";
			int item;
			nm_set_item_input(&m, FILENAME_LEN, text);
			item = newmenu_do( NULL, "Briefing to play?", 1, &m, NULL, NULL );
			if (item != -1) {
				do_briefing_screens(text,1);
			}
			break;
		}

		case KEY_DEBUGGED+KEY_SHIFTED+KEY_B:
			if (Player_is_dead)
				return 0;

			kill_and_so_forth();
			break;
		case KEY_DEBUGGED+KEY_G:
			GameTime64 = (0x7fffffffffffffffLL) - (F1_0*10);
			HUD_init_message(HM_DEFAULT, "GameTime %" PRIi64 " - Reset in 10 seconds!", GameTime64);
			break;
		default:
			return 0;
			break;
	}

	return 1;
}
#endif		//#ifndef RELEASE

#define CHEAT_MAX_LEN 15
typedef struct cheat_code
{
	const char string[CHEAT_MAX_LEN];
	int *stateptr;
} __pack__ cheat_code;

static const cheat_code cheat_codes[] = {
#if defined(DXX_BUILD_DESCENT_I)
	{ "gabbagabbahey", &cheats.enabled },
	{ "scourge", &cheats.wowie },
	{ "bigred", &cheats.wowie2 },
	{ "mitzi", &cheats.allkeys },
	{ "racerx", &cheats.invul },
	{ "guile", &cheats.cloak },
	{ "twilight", &cheats.shields },
	{ "poboys", &cheats.killreactor },
	{ "farmerjoe", &cheats.levelwarp },
	{ "bruin", &cheats.extralife },
	{ "porgys", &cheats.rapidfire },
	{ "ahimsa", &cheats.robotfiringsuspended },
#elif defined(DXX_BUILD_DESCENT_II)
	{ "gabbagabbahey", &cheats.lamer },
	{ "motherlode", &cheats.lamer },
	{ "currygoat", &cheats.lamer },
	{ "zingermans", &cheats.lamer },
	{ "eatangelos", &cheats.lamer },
	{ "ericaanne", &cheats.lamer },
	{ "joshuaakira", &cheats.lamer },
	{ "whammazoom", &cheats.lamer },
	{ "honestbob", &cheats.wowie },
	{ "algroove", &cheats.allkeys },
	{ "alifalafel", &cheats.accessory },
	{ "almighty", &cheats.invul },
	{ "blueorb", &cheats.shields },
	{ "delshiftb", &cheats.killreactor },
	{ "freespace", &cheats.levelwarp },
	{ "rockrgrl", &cheats.fullautomap },
	{ "wildfire", &cheats.rapidfire },
	{ "duddaboo", &cheats.bouncyfire },
	{ "imagespace", &cheats.robotfiringsuspended },
	{ "spaniard", &cheats.killallrobots },
	{ "silkwing", &cheats.robotskillrobots },
	{ "godzilla", &cheats.monsterdamage },
	{ "helpvishnu", &cheats.buddyclone },
	{ "gowingnut", &cheats.buddyangry },
#endif
	{ "flash", &cheats.exitpath },
	{ "astral", &cheats.ghostphysics },
	{ "buggin", &cheats.turbo },
	{ "bittersweet", &cheats.acid },
};

static int FinalCheats(int key)
{
	static char cheat_buffer[CHEAT_MAX_LEN];
	int *gotcha;

	if (Game_mode & GM_MULTI)
		return 0;

	for (unsigned i = 1; i < CHEAT_MAX_LEN; i++)
		cheat_buffer[i-1] = cheat_buffer[i];
	cheat_buffer[CHEAT_MAX_LEN-1] = key_ascii();
	for (unsigned i = 0;; i++)
	{
		if (i >= sizeof(cheat_codes) / sizeof(cheat_codes[0]))
			return 0;
		int cheatlen = strlen(cheat_codes[i].string);
		Assert(cheatlen <= CHEAT_MAX_LEN);
		if (d_strnicmp(cheat_codes[i].string, cheat_buffer+CHEAT_MAX_LEN-cheatlen, cheatlen)==0)
		{
#if defined(DXX_BUILD_DESCENT_I)
			if (!cheats.enabled && *cheat_codes[i].stateptr != cheats.enabled)
				return 0;
			if (!cheats.enabled)
				HUD_init_message_literal(HM_DEFAULT, TXT_CHEATS_ENABLED);
#endif
			gotcha = cheat_codes[i].stateptr;
			*cheat_codes[i].stateptr = !*cheat_codes[i].stateptr;
			cheats.enabled = 1;
			digi_play_sample( SOUND_CHEATER, F1_0);
			Players[Player_num].score = 0;
			break;
		}
	}

#if defined(DXX_BUILD_DESCENT_I)
	if (gotcha == &cheats.wowie)
	{
		HUD_init_message_literal(HM_DEFAULT, TXT_WOWIE_ZOWIE);

		Players[Player_num].primary_weapon_flags |= 0xff ^ (HAS_PLASMA_FLAG | HAS_FUSION_FLAG);
		Players[Player_num].secondary_weapon_flags |= 0xff ^ (HAS_SMART_FLAG | HAS_MEGA_FLAG);

		Players[Player_num].vulcan_ammo = Primary_ammo_max[VULCAN_INDEX];
		for (unsigned i=0; i<3; i++)
			Players[Player_num].secondary_ammo[i] = Secondary_ammo_max[i];

		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_laser_level(Players[Player_num].laser_level, MAX_LASER_LEVEL);

		Players[Player_num].energy = MAX_ENERGY;
		Players[Player_num].laser_level = MAX_LASER_LEVEL;
		Players[Player_num].flags |= PLAYER_FLAGS_QUAD_LASERS;
		update_laser_weapon_info();
	}

	if (gotcha == &cheats.wowie2)
	{
		HUD_init_message(HM_DEFAULT, "SUPER %s",TXT_WOWIE_ZOWIE);

		Players[Player_num].primary_weapon_flags = 0xff;
		Players[Player_num].secondary_weapon_flags = 0xff;

		Players[Player_num].vulcan_ammo = Primary_ammo_max[VULCAN_INDEX];
		for (unsigned i=0; i<MAX_SECONDARY_WEAPONS; i++)
			Players[Player_num].secondary_ammo[i] = Secondary_ammo_max[i];

		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_laser_level(Players[Player_num].laser_level, MAX_LASER_LEVEL);

		Players[Player_num].energy = MAX_ENERGY;
		Players[Player_num].laser_level = MAX_LASER_LEVEL;
		Players[Player_num].flags |= PLAYER_FLAGS_QUAD_LASERS;
		update_laser_weapon_info();
	}
#elif defined(DXX_BUILD_DESCENT_II)
	if (gotcha == &cheats.lamer)
	{
		Players[Player_num].shields=Players[Player_num].energy=i2f(1);
		HUD_init_message_literal(HM_DEFAULT, "Take that...cheater!");
	}

	if (gotcha == &cheats.wowie)
	{
		HUD_init_message_literal(HM_DEFAULT, TXT_WOWIE_ZOWIE);

		if (Piggy_hamfile_version < 3) // SHAREWARE
		{
			Players[Player_num].primary_weapon_flags = ~((1<<PHOENIX_INDEX) | (1<<OMEGA_INDEX) | (1<<FUSION_INDEX) | HAS_FLAG(SUPER_LASER_INDEX));
			Players[Player_num].secondary_weapon_flags = ~((1<<SMISSILE4_INDEX) | (1<<MEGA_INDEX) | (1<<SMISSILE5_INDEX));
		}
		else
		{
			Players[Player_num].primary_weapon_flags = 0xffff ^ HAS_FLAG(SUPER_LASER_INDEX); //no super laser
			Players[Player_num].secondary_weapon_flags = 0xffff;
		}

		Players[Player_num].vulcan_ammo = Primary_ammo_max[VULCAN_INDEX];
		for (unsigned i=0; i<MAX_SECONDARY_WEAPONS; i++)
			Players[Player_num].secondary_ammo[i] = Secondary_ammo_max[i];

		if (Piggy_hamfile_version < 3) // SHAREWARE
		{
			Players[Player_num].secondary_ammo[SMISSILE4_INDEX] = 0;
			Players[Player_num].secondary_ammo[SMISSILE5_INDEX] = 0;
			Players[Player_num].secondary_ammo[MEGA_INDEX] = 0;
		}

		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_laser_level(Players[Player_num].laser_level, MAX_LASER_LEVEL);

		Players[Player_num].energy = MAX_ENERGY;
		Players[Player_num].laser_level = MAX_SUPER_LASER_LEVEL;
		Players[Player_num].flags |= PLAYER_FLAGS_QUAD_LASERS;
		update_laser_weapon_info();
	}

	if (gotcha == &cheats.accessory)
	{
		Players[Player_num].flags |=PLAYER_FLAGS_HEADLIGHT;
		Players[Player_num].flags |=PLAYER_FLAGS_AFTERBURNER;
		Players[Player_num].flags |=PLAYER_FLAGS_AMMO_RACK;
		Players[Player_num].flags |=PLAYER_FLAGS_CONVERTER;
		HUD_init_message_literal(HM_DEFAULT, "Accessories!!");
	}
#endif

	if (gotcha == &cheats.allkeys)
	{
		HUD_init_message_literal(HM_DEFAULT, TXT_ALL_KEYS);
		Players[Player_num].flags |= PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_GOLD_KEY;
	}

	if (gotcha == &cheats.invul)
	{
		Players[Player_num].flags ^= PLAYER_FLAGS_INVULNERABLE;
		HUD_init_message(HM_DEFAULT, "%s %s!", TXT_INVULNERABILITY, (Players[Player_num].flags&PLAYER_FLAGS_INVULNERABLE)?TXT_ON:TXT_OFF);
		Players[Player_num].invulnerable_time = GameTime64+i2f(1000);
	}

	if (gotcha == &cheats.shields)
	{
		HUD_init_message_literal(HM_DEFAULT, TXT_FULL_SHIELDS);
		Players[Player_num].shields = MAX_SHIELDS;
	}

#if defined(DXX_BUILD_DESCENT_I)
	if (gotcha == &cheats.cloak)
	{
		Players[Player_num].flags ^= PLAYER_FLAGS_CLOAKED;
		HUD_init_message(HM_DEFAULT, "%s %s!", TXT_CLOAK, (Players[Player_num].flags&PLAYER_FLAGS_CLOAKED)?TXT_ON:TXT_OFF);
		if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED)
		{
			ai_do_cloak_stuff();
			Players[Player_num].cloak_time = GameTime64;
		}
	}

	if (gotcha == &cheats.extralife)
	{
		if (Players[Player_num].lives<50)
		{
			Players[Player_num].lives++;
			HUD_init_message_literal(HM_DEFAULT, "Extra life!");
		}
	}
#endif

	if (gotcha == &cheats.killreactor)
	{
		kill_and_so_forth();
	}

	if (gotcha == &cheats.exitpath)
	{
		if (create_special_path())
			HUD_init_message_literal(HM_DEFAULT, "Exit path illuminated!");
	}

	if (gotcha == &cheats.levelwarp)
	{
		newmenu_item m;
		char text[10]="";
		int new_level_num;
		int item;
		nm_set_item_input(&m, 10, text);
		item = newmenu_do( NULL, TXT_WARP_TO_LEVEL, 1, &m, NULL, NULL );
		if (item != -1) {
			new_level_num = atoi(m.text);
			if (new_level_num!=0 && new_level_num>=0 && new_level_num<=Last_level) {
				window_set_visible(Game_wind, 0);
				StartNewLevel(new_level_num);
				window_set_visible(Game_wind, 1);
			}
		}
	}

	if (gotcha == &cheats.ghostphysics)
	{
		HUD_init_message(HM_DEFAULT, "%s %s!", "Ghosty mode", cheats.ghostphysics?TXT_ON:TXT_OFF);
	}

	if (gotcha == &cheats.rapidfire)
#if defined(DXX_BUILD_DESCENT_I)
	{
		do_megawow_powerup(200);
	}
#elif defined(DXX_BUILD_DESCENT_II)
	{
		HUD_init_message(HM_DEFAULT, "Rapid fire %s!", cheats.rapidfire?TXT_ON:TXT_OFF);
	}

	if (gotcha == &cheats.bouncyfire)
	{
		
		HUD_init_message(HM_DEFAULT, "Bouncing weapons %s!", cheats.bouncyfire?TXT_ON:TXT_OFF);
	}
#endif

	if (gotcha == &cheats.turbo)
	{
		HUD_init_message(HM_DEFAULT, "%s %s!", "Turbo mode", cheats.turbo?TXT_ON:TXT_OFF);
	}

	if (gotcha == &cheats.robotfiringsuspended)
	{
		HUD_init_message(HM_DEFAULT, "Robot firing %s!", cheats.robotfiringsuspended?TXT_OFF:TXT_ON);
	}

#if defined(DXX_BUILD_DESCENT_II)
	if (gotcha == &cheats.killallrobots)
	{
		kill_all_robots();
	}

	if (gotcha == &cheats.robotskillrobots)
	{
		HUD_init_message_literal(HM_DEFAULT, cheats.robotskillrobots?"Rabid robots!":"Kill the player!");
	}

	if (gotcha == &cheats.monsterdamage)
	{
		HUD_init_message_literal(HM_DEFAULT, cheats.monsterdamage?"Oh no, there goes Tokyo!":"What have you done, I'm shrinking!!");
	}

	if (gotcha == &cheats.buddyclone)
	{
		HUD_init_message_literal(HM_DEFAULT, "What's this? Another buddy bot!");
		create_buddy_bot();
	}

	if (gotcha == &cheats.buddyangry)
	{
		
		if (cheats.buddyangry)
		{
			HUD_init_message(HM_DEFAULT, "%s gets angry!",PlayerCfg.GuidebotName);
			strcpy(PlayerCfg.GuidebotName,"Wingnut");
		}
		else
		{
			strcpy(PlayerCfg.GuidebotName,PlayerCfg.GuidebotNameReal);
			HUD_init_message(HM_DEFAULT, "%s calms down",PlayerCfg.GuidebotName);
		}
	}
#endif

	if (gotcha == &cheats.acid)
	{
		HUD_init_message_literal(HM_DEFAULT, cheats.acid?"Going up!":"Coming down!");
	}

	return 1;
}

// Internal Cheat Menu
#ifndef RELEASE
static void do_cheat_menu()
{
	int mmn;
	newmenu_item mm[16];
	char score_text[21];

	sprintf( score_text, "%d", Players[Player_num].score );

	nm_set_item_checkbox(&mm[0],TXT_INVULNERABILITY,Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE);
	nm_set_item_checkbox(&mm[1],TXT_CLOAKED,Players[Player_num].flags & PLAYER_FLAGS_CLOAKED);
	nm_set_item_checkbox(&mm[2],"All keys",0);
	nm_set_item_number(&mm[3], "% Energy", f2i(Players[Player_num].energy), 0, 200);
	nm_set_item_number(&mm[4], "% Shields", f2i(Players[Player_num].shields), 0, 200);
	nm_set_item_text(& mm[5], "Score:");
	nm_set_item_input(&mm[6], 10, score_text);
#if defined(DXX_BUILD_DESCENT_I)
	nm_set_item_radio(&mm[7], "Laser level 1", (Players[Player_num].laser_level==0), 0);
	nm_set_item_radio(&mm[8], "Laser level 2", (Players[Player_num].laser_level==1), 0);
	nm_set_item_radio(&mm[9], "Laser level 3", (Players[Player_num].laser_level==2), 0);
	nm_set_item_radio(&mm[10], "Laser level 4", (Players[Player_num].laser_level==3), 0);
	nm_set_item_number(&mm[11], "Missiles", Players[Player_num].secondary_ammo[CONCUSSION_INDEX], 0, 200);

	mmn = newmenu_do("Wimp Menu",NULL,12, mm, NULL, NULL );
#elif defined(DXX_BUILD_DESCENT_II)
	nm_set_item_number(&mm[7], "Laser Level", Players[Player_num].laser_level+1, 0, MAX_SUPER_LASER_LEVEL+1);
	nm_set_item_number(&mm[8], "Missiles", Players[Player_num].secondary_ammo[CONCUSSION_INDEX], 0, 200);

	mmn = newmenu_do("Wimp Menu",NULL,9, mm, NULL, NULL );
#endif

	if (mmn > -1 )  {
		if ( mm[0].value )  {
			Players[Player_num].flags |= PLAYER_FLAGS_INVULNERABLE;
			Players[Player_num].invulnerable_time = GameTime64+i2f(1000);
		} else
			Players[Player_num].flags &= ~PLAYER_FLAGS_INVULNERABLE;
		if ( mm[1].value )
		{
			Players[Player_num].flags |= PLAYER_FLAGS_CLOAKED;
			if (Game_mode & GM_MULTI)
				multi_send_cloak();
			ai_do_cloak_stuff();
			Players[Player_num].cloak_time = GameTime64;
		}
		else
			Players[Player_num].flags &= ~PLAYER_FLAGS_CLOAKED;

		if (mm[2].value) Players[Player_num].flags |= PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_GOLD_KEY;
		Players[Player_num].energy=i2f(mm[3].value);
		Players[Player_num].shields=i2f(mm[4].value);
		Players[Player_num].score = atoi(mm[6].text);
#if defined(DXX_BUILD_DESCENT_I)
		if (mm[7].value) Players[Player_num].laser_level=0;
		if (mm[8].value) Players[Player_num].laser_level=1;
		if (mm[9].value) Players[Player_num].laser_level=2;
		if (mm[10].value) Players[Player_num].laser_level=3;
		Players[Player_num].secondary_ammo[CONCUSSION_INDEX] = mm[11].value;
#elif defined(DXX_BUILD_DESCENT_II)
		Players[Player_num].laser_level = mm[7].value-1;
		Players[Player_num].secondary_ammo[CONCUSSION_INDEX] = mm[8].value;
#endif
		init_gauges();
	}
}
#endif



//	Testing functions ----------------------------------------------------------

#ifndef NDEBUG
//	Sounds for testing

int test_sound_num = 0;
int sound_nums[] = {10,11,20,21,30,31,32,33,40,41,50,51,60,61,62,70,80,81,82,83,90,91};

#define N_TEST_SOUNDS (sizeof(sound_nums) / sizeof(*sound_nums))


static void advance_sound()
{
	if (++test_sound_num == N_TEST_SOUNDS)
		test_sound_num=0;

}


int     Test_sound = 251;

static void play_test_sound()
{

	// -- digi_play_sample(sound_nums[test_sound_num], F1_0);
	digi_play_sample(Test_sound, F1_0);
}

#endif  //ifndef NDEBUG

int ReadControls(d_event *event)
{
	int key;
	static ubyte exploding_flag=0;

	Player_fired_laser_this_frame=-1;

	if (Player_exploded) {
		if (exploding_flag==0)  {
			exploding_flag = 1;			// When player starts exploding, clear all input devices...
			game_flush_inputs();
		}
	} else {
		exploding_flag=0;
	}
	if (Player_is_dead && !( (Game_mode & GM_MULTI) && (multi_sending_message[Player_num] || multi_defining_message) ))
		if (HandleDeathInput(event))
			return 1;

	if (Newdemo_state == ND_STATE_PLAYBACK)
		update_vcr_state();

	if (event->type == EVENT_KEY_COMMAND)
	{
		key = event_key_get(event);
#if defined(DXX_BUILD_DESCENT_II)
		if (DefiningMarkerMessage)
		{
			return MarkerInputMessage(key);
		}
#endif
		if ( (Game_mode & GM_MULTI) && (multi_sending_message[Player_num] || multi_defining_message) )
		{
			return multi_message_input_sub(key);
		}

#ifndef RELEASE
		if ((key&KEY_DEBUGGED)&&(Game_mode&GM_MULTI))   {
			Network_message_reciever = 100;		// Send to everyone...
			sprintf( Network_message, "%s %s", TXT_I_AM_A, TXT_CHEATER);
		}
#endif

		if (Endlevel_sequence)
		{
			if (HandleEndlevelKey(key))
				return 1;
		}
		else if (Newdemo_state == ND_STATE_PLAYBACK )
		{
			if (HandleDemoKey(key))
				return 1;
		}
		else
		{
			if (FinalCheats(key)) return 1;
			if (HandleSystemKey(key)) return 1;
			if (HandleGameKey(key)) return 1;
		}

#ifndef RELEASE
		if (HandleTestKey(key))
			return 1;
#endif

		if (call_default_handler(event))
			return 1;
	}

	if (!Endlevel_sequence && !Player_is_dead && (Newdemo_state != ND_STATE_PLAYBACK))
	{

		kconfig_read_controls(event, 0);

		check_rear_view();

		// If automap key pressed, enable automap unless you are in network mode, control center destroyed and < 10 seconds left
		if ( Controls.automap_count > 0 )
		{
			Controls.automap_count = 0;
			if (!((Game_mode & GM_MULTI) && Control_center_destroyed && (Countdown_seconds_left < 10)))
			{
				do_automap(0);
				return 1;
			}
		}

		do_weapon_n_item_stuff();
	}

	return 0;
}

