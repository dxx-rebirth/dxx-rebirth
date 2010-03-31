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
#include "window.h"
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
#include "movie.h"
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
#include "escort.h"
#include "window.h"

#ifdef EDITOR
#include "editor/editor.h"
#endif

#include <SDL/SDL.h>

extern void object_goto_prev_viewer(void);

// Global Variables -----------------------------------------------------------

int	Debug_spew;

//	External Variables ---------------------------------------------------------

extern char WaitForRefuseAnswer,RefuseThisPlayer,RefuseTeam;

#ifndef NDEBUG
extern int	Mark_count;
#endif

extern int	Global_missile_firing_count;
extern int	Config_menu_flag;

extern int	Game_aborted;
extern int	*Toggle_var;

extern int	Physics_cheat_flag;

extern fix	Show_view_text_timer;

extern ubyte DefiningMarkerMessage;

//	Function prototypes --------------------------------------------------------


extern void CyclePrimary();
extern void CycleSecondary();
extern void InitMarkerInput();
extern int  MarkerInputMessage(int key);
extern int	allowed_to_fire_missile(void);
extern int	allowed_to_fire_flare(void);
extern void	check_rear_view(void);
extern int	create_special_path(void);
extern void move_player_2_segment(segment *seg, int side);
extern void	kconfig_center_headset(void);
extern void newdemo_strip_frames(char *, int);
extern void toggle_cockpit(void);
extern void dump_used_textures_all();
extern void DropMarker();
extern void DropSecondaryWeapon();
extern void DropCurrentWeapon();

void FinalCheats(int key);

#ifndef RELEASE
void do_cheat_menu(void);
#endif

int HandleGameKey(int key);
int HandleSystemKey(int key);
int HandleTestKey(int key);
int HandleVRKey(int key);
void advance_sound(void);
void play_test_sound(void);

#define key_isfunc(k) (((k&0xff)>=KEY_F1 && (k&0xff)<=KEY_F10) || (k&0xff)==KEY_F11 || (k&0xff)==KEY_F12)
#define key_ismod(k)  ((k&0xff)==KEY_LALT || (k&0xff)==KEY_RALT || (k&0xff)==KEY_LSHIFT || (k&0xff)==KEY_RSHIFT || (k&0xff)==KEY_LCTRL || (k&0xff)==KEY_RCTRL || (k&0xff)==KEY_LMETA || (k&0xff)==KEY_RMETA)

// Functions ------------------------------------------------------------------

#define CONVERTER_RATE  20		//10 units per second xfer rate
#define CONVERTER_SCALE  2		//2 units energy -> 1 unit shields

#define CONVERTER_SOUND_DELAY (f1_0/2)		//play every half second

void transfer_energy_to_shield(fix time)
{
	fix e;		//how much energy gets transfered
	static fix last_play_time=0;

	e = min(min(time*CONVERTER_RATE,Players[Player_num].energy - INITIAL_ENERGY),(MAX_SHIELDS-Players[Player_num].shields)*CONVERTER_SCALE);

	if (e <= 0) {

		if (Players[Player_num].energy <= INITIAL_ENERGY) {
			HUD_init_message("Need more than %i energy to enable transfer", f2i(INITIAL_ENERGY));
		}
		else if (Players[Player_num].shields == 200) {
			HUD_init_message("No transfer: Shields already at max");
		}
		return;
	}

	Players[Player_num].energy  -= e;
	Players[Player_num].shields += e/CONVERTER_SCALE;

	if (last_play_time > GameTime)
		last_play_time = 0;

	if (GameTime > last_play_time+CONVERTER_SOUND_DELAY) {
		digi_play_sample_once(SOUND_CONVERT_ENERGY, F1_0);
		last_play_time = GameTime;
	}

}

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

//returns which bomb will be dropped next time the bomb key is pressed
int which_bomb()
{
	int bomb;

	//use the last one selected, unless there aren't any, in which case use
	//the other if there are any


   // If hoard game, only let the player drop smart mines
   if (Game_mode & GM_HOARD)
		return SMART_MINE_INDEX;

	bomb = Secondary_last_was_super[PROXIMITY_INDEX]?SMART_MINE_INDEX:PROXIMITY_INDEX;

	if (Players[Player_num].secondary_ammo[bomb] == 0 &&
			Players[Player_num].secondary_ammo[SMART_MINE_INDEX+PROXIMITY_INDEX-bomb] != 0) {
		bomb = SMART_MINE_INDEX+PROXIMITY_INDEX-bomb;
		Secondary_last_was_super[bomb%SUPER_WEAPON] = (bomb == SMART_MINE_INDEX);
	}
	
	

	return bomb;
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
		do_missile_firing(0);
		Global_missile_firing_count--;
	}

	if (Controls.cycle_primary_count)
	{
		for (i=0;i<Controls.cycle_primary_count;i++)
			CyclePrimary ();
	}
	if (Controls.cycle_secondary_count)
	{
		for (i=0;i<Controls.cycle_secondary_count;i++)
			CycleSecondary ();
	}
	if (Controls.headlight_count)
	{
		for (i=0;i<Controls.headlight_count;i++)
			toggle_headlight_active ();
	}

	if (Global_missile_firing_count < 0)
		Global_missile_firing_count = 0;

	//	Drop proximity bombs.
	if (Controls.drop_bomb_down_count) {
		while (Controls.drop_bomb_down_count--)
			do_missile_firing(1);
	}
}


extern void game_render_frame();
extern void show_extra_views();
extern fix Flash_effect;

void apply_modified_palette(void)
{
//@@    int				k,x,y;
//@@    grs_bitmap	*sbp;
//@@    grs_canvas	*save_canv;
//@@    int				color_xlate[256];
//@@
//@@
//@@    if (!Flash_effect && ((PaletteRedAdd < 10) || (PaletteRedAdd < (PaletteGreenAdd + PaletteBlueAdd))))
//@@		return;
//@@
//@@    reset_cockpit();
//@@
//@@    save_canv = grd_curcanv;
//@@    gr_set_current_canvas(&grd_curscreen->sc_canvas);
//@@
//@@    sbp = &grd_curscreen->sc_canvas.cv_bitmap;
//@@
//@@    for (x=0; x<256; x++)
//@@		color_xlate[x] = -1;
//@@
//@@    for (k=0; k<4; k++) {
//@@		for (y=0; y<grd_curscreen->sc_h; y+= 4) {
//@@			  for (x=0; x<grd_curscreen->sc_w; x++) {
//@@					int	color, new_color;
//@@					int	r, g, b;
//@@					int	xcrd, ycrd;
//@@
//@@					ycrd = y+k;
//@@					xcrd = x;
//@@
//@@					color = gr_ugpixel(sbp, xcrd, ycrd);
//@@
//@@					if (color_xlate[color] == -1) {
//@@						r = gr_palette[color*3+0];
//@@						g = gr_palette[color*3+1];
//@@						b = gr_palette[color*3+2];
//@@
//@@						r += PaletteRedAdd;		 if (r > 63) r = 63;
//@@						g += PaletteGreenAdd;   if (g > 63) g = 63;
//@@						b += PaletteBlueAdd;		if (b > 63) b = 63;
//@@
//@@						color_xlate[color] = gr_find_closest_color_current(r, g, b);
//@@
//@@					}
//@@
//@@					new_color = color_xlate[color];
//@@
//@@					gr_setcolor(new_color);
//@@					gr_upixel(xcrd, ycrd);
//@@			  }
//@@		}
//@@    }
}

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

//Process selected keys until game unpaused
int pause_handler(window *wind, d_event *event, char *msg)
{
	int key;

	switch (event->type)
	{
		case EVENT_WINDOW_ACTIVATED:
			game_flush_inputs();
			break;
			
		case EVENT_KEY_COMMAND:
			key = ((d_event_keycommand *)event)->keycode;
			
			switch (key)
			{
				case 0:
					break;
				case KEY_ESC:
					//Function_mode = FMODE_MENU;	// Don't like this, just press escape twice (kreatordxx)
					window_close(wind);
					return 1;
				case KEY_F1:
					show_help();
					return 1;
				case KEY_PAUSE:
					window_close(wind);
					return 1;
				case KEY_ALTED+KEY_ENTER:
				case KEY_ALTED+KEY_PADENTER:
					gr_toggle_fullscreen();
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
			reset_cockpit();
			if (EXT_MUSIC_ON)
				ext_music_resume();
			digi_resume_midi();		// sound pausing handled by game_handler
			d_free(msg);
			break;
			
		default:
			break;
	}

	return 0;
}

int do_game_pause()
{
	char *msg;
	char total_time[9],level_time[9];
	
	MALLOC(msg, char, 1024);
	if (!msg)
		return 0;

#ifdef NETWORK
	if (Game_mode & GM_MULTI)
	{
		netplayerinfo_on= !netplayerinfo_on;
		return(KEY_PAUSE);
	}
#endif
	
	digi_pause_midi();		// sound pausing handled by game_handler
	ext_music_pause();
	format_time(total_time, f2i(Players[Player_num].time_total) + Players[Player_num].hours_total*3600);
	format_time(level_time, f2i(Players[Player_num].time_level) + Players[Player_num].hours_level*3600);
	if (Newdemo_state!=ND_STATE_PLAYBACK)
		sprintf(msg,"PAUSE\n\nSkill level:  %s\nHostages on board:  %d\nTime on level: %s\nTotal time in game: %s",(*(&TXT_DIFFICULTY_1 + (Difficulty_level))),Players[Player_num].hostages_on_board,level_time,total_time);
	else
	  	sprintf(msg,"PAUSE\n\nSkill level:  %s\nHostages on board:  %d\n",(*(&TXT_DIFFICULTY_1 + (Difficulty_level))),Players[Player_num].hostages_on_board);
	set_screen_mode(SCREEN_MENU);
	
	if (!window_create(&grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT, (int (*)(window *, d_event *, void *))pause_handler, msg))
		d_free(msg);
	
	return 0 /*key*/;	// Keycode returning ripped out (kreatordxx)
}

extern int network_who_is_master(),network_how_many_connected(),GetMyNetRanking();
extern int TotalMissedPackets,TotalPacketsGot;
// char *NetworkModeNames[]={"Anarchy","Team Anarchy","Robo Anarchy","Cooperative","Capture the Flag","Hoard","Team Hoard","Unknown"};
extern char *RankStrings[];
extern int PhallicLimit,PhallicMan;

int HandleEndlevelKey(int key)
{
	switch (key)
	{
#ifdef macintosh
		case KEY_COMMAND + KEY_SHIFTED + KEY_3:
#endif
		case KEY_PRINT_SCREEN:
			save_screen_shot(0);
			return 1;
			
		case KEY_COMMAND+KEY_P:
		case KEY_PAUSE:
			do_game_pause();
			return 1;
			
		case KEY_ESC:
			stop_endlevel_sequence();
			last_drawn_cockpit=-1;
			return 1;
			
		case KEY_BACKSP:
			Int3();
			return 1;
	}

	return 0;
}

int HandleDeathKey(int key)
{
	if (Player_exploded && !key_isfunc(key) && !key_ismod(key) && key)
		Death_sequence_aborted  = 1;		//Any key but func or modifier aborts

	if (key == KEY_ESC) {
		if (ConsoleObject->flags & OF_EXPLODING)
			Death_sequence_aborted = 1;
	}

	if (Death_sequence_aborted)
	{
		game_flush_inputs();
		return 1;
	}
	
	return 0;
}

int HandleDemoKey(int key)
{
	switch (key) {
		KEY_MAC(case KEY_COMMAND+KEY_1:)
		case KEY_F1:	show_newdemo_help();	break;
		KEY_MAC(case KEY_COMMAND+KEY_2:)
		case KEY_F2:	Config_menu_flag = 1;	break;
		KEY_MAC(case KEY_COMMAND+KEY_3:)
		case KEY_F3:
			 if (Viewer->type == OBJ_PLAYER)
				toggle_cockpit();
			 break;
		KEY_MAC(case KEY_COMMAND+KEY_4:)
		case KEY_F4:	Newdemo_show_percentage = !Newdemo_show_percentage; break;
		KEY_MAC(case KEY_COMMAND+KEY_7:)
		case KEY_F7:
#ifdef NETWORK
			Show_kill_list = (Show_kill_list+1) % ((Newdemo_game_mode & GM_TEAM) ? 4 : 3);
#endif
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
				game_render_frame_mono(GameArg.DbgUseDoubleBuffer);
				save_screen_shot(0);
				Newdemo_show_percentage = old_state;
			}
			break;
		}
		case KEY_ALTED+KEY_ENTER:
		case KEY_ALTED+KEY_PADENTER:
			gr_toggle_fullscreen();
			break;

#ifndef NDEBUG
		case KEY_BACKSP:
			Int3();
			break;
		case KEY_DEBUGGED + KEY_I:
			Newdemo_do_interpolate = !Newdemo_do_interpolate;
			HUD_init_message("Demo playback interpolation %s", Newdemo_do_interpolate?"ON":"OFF");
			break;
		case KEY_DEBUGGED + KEY_K: {
			int how_many, c;
			char filename[FILENAME_LEN], num[16];
			newmenu_item m[6];

			filename[0] = '\0';
			m[ 0].type = NM_TYPE_TEXT; m[ 0].text = "output file name";
			m[ 1].type = NM_TYPE_INPUT;m[ 1].text_len = 8; m[1].text = filename;
			c = newmenu_do( NULL, NULL, 2, m, NULL, NULL );
			if (c == -2)
				break;
			strcat(filename, ".dem");
			num[0] = '\0';
			m[ 0].type = NM_TYPE_TEXT; m[ 0].text = "strip how many bytes";
			m[ 1].type = NM_TYPE_INPUT;m[ 1].text_len = 16; m[1].text = num;
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

//switch a cockpit window to the next function
int select_next_window_function(int w)
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
#ifdef NETWORK
			//fall through
		case CV_COOP:
			Marker_viewer_num[w] = -1;
			if ((Game_mode & GM_MULTI_COOP) || (Game_mode & GM_TEAM)) {
				PlayerCfg.Cockpit3DView[w] = CV_COOP;
				while (1) {
					Coop_view_player[w]++;
					if (Coop_view_player[w] == N_players) {
						PlayerCfg.Cockpit3DView[w] = CV_MARKER;
						goto case_marker;
					}
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
#endif
				PlayerCfg.Cockpit3DView[w] = CV_NONE;
			break;
	}
	write_player_file();

	return 1;	 //screen_changed
}

void songs_goto_next_song();
void songs_goto_prev_song();

#ifdef DOOR_DEBUGGING
dump_door_debugging_info()
{
	object *obj;
	vms_vector new_pos;
	fvi_query fq;
	fvi_info hit_info;
	int fate;
	FILE *dfile;
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

	dfile = fopen("door.out","at");

	fprintf(dfile,"FVI hit_type = %d\n",fate);
	fprintf(dfile,"    hit_seg = %d\n",hit_info.hit_seg);
	fprintf(dfile,"    hit_side = %d\n",hit_info.hit_side);
	fprintf(dfile,"    hit_side_seg = %d\n",hit_info.hit_side_seg);
	fprintf(dfile,"\n");

	if (fate == HIT_WALL) {

		wall_num = Segments[hit_info.hit_seg].sides[hit_info.hit_side].wall_num;
		fprintf(dfile,"wall_num = %d\n",wall_num);
	
		if (wall_num != -1) {
			wall *wall = &Walls[wall_num];
			active_door *d;
			int i;
	
			fprintf(dfile,"    segnum = %d\n",wall->segnum);
			fprintf(dfile,"    sidenum = %d\n",wall->sidenum);
			fprintf(dfile,"    hps = %x\n",wall->hps);
			fprintf(dfile,"    linked_wall = %d\n",wall->linked_wall);
			fprintf(dfile,"    type = %d\n",wall->type);
			fprintf(dfile,"    flags = %x\n",wall->flags);
			fprintf(dfile,"    state = %d\n",wall->state);
			fprintf(dfile,"    trigger = %d\n",wall->trigger);
			fprintf(dfile,"    clip_num = %d\n",wall->clip_num);
			fprintf(dfile,"    keys = %x\n",wall->keys);
			fprintf(dfile,"    controlling_trigger = %d\n",wall->controlling_trigger);
			fprintf(dfile,"    cloak_value = %d\n",wall->cloak_value);
			fprintf(dfile,"\n");
	
	
			for (i=0;i<Num_open_doors;i++) {		//find door
				d = &ActiveDoors[i];
				if (d->front_wallnum[0]==wall-Walls || d->back_wallnum[0]==wall-Walls || (d->n_parts==2 && (d->front_wallnum[1]==wall-Walls || d->back_wallnum[1]==wall-Walls)))
					break;
			} 
	
			if (i>=Num_open_doors)
				fprintf(dfile,"No active door.\n");
			else {
				fprintf(dfile,"Active door %d:\n",i);
				fprintf(dfile,"    n_parts = %d\n",d->n_parts);
				fprintf(dfile,"    front_wallnum = %d,%d\n",d->front_wallnum[0],d->front_wallnum[1]);
				fprintf(dfile,"    back_wallnum = %d,%d\n",d->back_wallnum[0],d->back_wallnum[1]);
				fprintf(dfile,"    time = %x\n",d->time);
			}
	
		}
	}

	fprintf(dfile,"\n");
	fprintf(dfile,"\n");

	fclose(dfile);

}
#endif

//this is for system-level keys, such as help, etc.
//returns 1 if screen changed
int HandleSystemKey(int key)
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
				Game_aborted=1;
				Function_mode = FMODE_MENU;
				return 1;

// fleshed these out because F1 and F2 aren't sequenctial keycodes on mac -- MWA

			KEY_MAC(case KEY_COMMAND+KEY_SHIFTED+KEY_1:)
			case KEY_SHIFTED+KEY_F1:
				select_next_window_function(0);
				return 1;
			KEY_MAC(case KEY_COMMAND+KEY_SHIFTED+KEY_2:)
			case KEY_SHIFTED+KEY_F2:
				select_next_window_function(1);
				return 1;
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
			}
			save_screen_shot(0);
			break;
		}

		KEY_MAC(case KEY_COMMAND+KEY_1:)
		case KEY_F1:				if (Game_mode & GM_MULTI) show_netgame_help(); else show_help();	break;

		KEY_MAC(case KEY_COMMAND+KEY_2:)
		case KEY_F2:				//Config_menu_flag = 1;	break;
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
#ifdef NETWORK
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

#endif

		KEY_MAC(case KEY_COMMAND+KEY_SHIFTED+KEY_S:)
		KEY_MAC(case KEY_COMMAND+KEY_ALTED+KEY_2:)
		case KEY_ALTED+KEY_F2:
			if (!Player_is_dead && !((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP)))
				state_save_all(0, 0, NULL, 0); // 0 means not between levels.
			break;

		KEY_MAC(case KEY_COMMAND+KEY_S:)
		case KEY_ALTED+KEY_F1:	if (!Player_is_dead) state_save_all(0, 0, NULL, 1);	break;
		KEY_MAC(case KEY_COMMAND+KEY_O:)
		KEY_MAC(case KEY_COMMAND+KEY_ALTED+KEY_3:)
		case KEY_ALTED+KEY_F3:
			if (!Player_is_dead && !((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP)))
				state_restore_all(1, 0, NULL);
			break;


		KEY_MAC(case KEY_COMMAND+KEY_SHIFTED+KEY_4:)
		case KEY_F4 + KEY_SHIFTED:
			do_escort_menu();
			break;


		KEY_MAC(case KEY_COMMAND+KEY_SHIFTED+KEY_ALTED+KEY_4:)
		case KEY_F4 + KEY_SHIFTED + KEY_ALTED:
			change_guidebot_name();
			break;

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
		default:
			return 0;
			break;

	}

	return 1;
}


int HandleVRKey(int key)
{
	switch( key )   {

		case KEY_ALTED+KEY_F5:
			if ( VR_render_mode != VR_NONE )	{
				VR_reset_params();
				HUD_init_message( "-Stereoscopic Parameters Reset-" );
				HUD_init_message( "Interaxial Separation = %.2f", f2fl(VR_eye_width) );
				HUD_init_message( "Stereo balance = %.2f", (float)VR_eye_offset/30.0 );
			}
			break;

		case KEY_ALTED+KEY_F6:
			if ( VR_render_mode != VR_NONE )	{
				VR_low_res++;
				if ( VR_low_res > 3 ) VR_low_res = 0;
				switch( VR_low_res )    {
					case 0: HUD_init_message( "Normal Resolution" ); break;
					case 1: HUD_init_message( "Low Vertical Resolution" ); break;
					case 2: HUD_init_message( "Low Horizontal Resolution" ); break;
					case 3: HUD_init_message( "Low Resolution" ); break;
				}
			}
			break;

		case KEY_ALTED+KEY_F7:
			if ( VR_render_mode != VR_NONE )	{
				VR_eye_switch = !VR_eye_switch;
				HUD_init_message( "-Eyes toggled-" );
				if ( VR_eye_switch )
					HUD_init_message( "Right Eye -- Left Eye" );
				else
					HUD_init_message( "Left Eye -- Right Eye" );
			}
			break;

		case KEY_ALTED+KEY_F8:
			if ( VR_render_mode != VR_NONE )	{
			VR_sensitivity++;
			if (VR_sensitivity > 2 )
				VR_sensitivity = 0;
			HUD_init_message( "Head tracking sensitivy = %d", VR_sensitivity );
		 }
			break;
		case KEY_ALTED+KEY_F9:
			if ( VR_render_mode != VR_NONE )	{
				VR_eye_width -= F1_0/10;
				if ( VR_eye_width < 0 ) VR_eye_width = 0;
				HUD_init_message( "Interaxial Separation = %.2f", f2fl(VR_eye_width) );
				HUD_init_message( "(The default value is %.2f)", f2fl(VR_SEPARATION) );
			}
			break;
		case KEY_ALTED+KEY_F10:
			if ( VR_render_mode != VR_NONE )	{
				VR_eye_width += F1_0/10;
				if ( VR_eye_width > F1_0*4 )    VR_eye_width = F1_0*4;
				HUD_init_message( "Interaxial Separation = %.2f", f2fl(VR_eye_width) );
				HUD_init_message( "(The default value is %.2f)", f2fl(VR_SEPARATION) );
			}
			break;

		case KEY_ALTED+KEY_F11:
			if ( VR_render_mode != VR_NONE )	{
				VR_eye_offset--;
				if ( VR_eye_offset < -30 )	VR_eye_offset = -30;
				HUD_init_message( "Stereo balance = %.2f", (float)VR_eye_offset/30.0 );
				HUD_init_message( "(The default value is %.2f)", (float)VR_PIXEL_SHIFT/30.0 );
				VR_eye_offset_changed = 2;
			}
			break;
		case KEY_ALTED+KEY_F12:
			if ( VR_render_mode != VR_NONE )	{
				VR_eye_offset++;
				if ( VR_eye_offset > 30 )	 VR_eye_offset = 30;
				HUD_init_message( "Stereo balance = %.2f", (float)VR_eye_offset/30.0 );
				HUD_init_message( "(The default value is %.2f)", (float)VR_PIXEL_SHIFT/30.0 );
				VR_eye_offset_changed = 2;
			}
			break;
			
		default:
			return 0;
			break;
	}
	
	return 1;
}


extern void DropFlag();

int HandleGameKey(int key)
{
	switch (key) {

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
					HUD_init_message ("No Guide-Bot in Multiplayer!");
				game_flush_inputs();
				return 1;
			}

		case KEY_ALTED+KEY_F7:
		KEY_MAC(case KEY_COMMAND+KEY_ALTED+KEY_7:)
			PlayerCfg.HudMode=(PlayerCfg.HudMode+1)%GAUGE_HUD_NUMMODES;
			write_player_file();
			return 1;

#ifdef NETWORK
		KEY_MAC(case KEY_COMMAND+KEY_6:)
		case KEY_F6:
			if (Netgame.RefusePlayers && WaitForRefuseAnswer && !(Game_mode & GM_TEAM))
			{
				RefuseThisPlayer=1;
				HUD_init_message ("Player accepted!");
			}
			return 1;
		case KEY_ALTED + KEY_1:
			if (Netgame.RefusePlayers && WaitForRefuseAnswer && (Game_mode & GM_TEAM))
				{
					RefuseThisPlayer=1;
					HUD_init_message ("Player accepted!");
					RefuseTeam=1;
					game_flush_inputs();
				}
			return 1;
		case KEY_ALTED + KEY_2:
			if (Netgame.RefusePlayers && WaitForRefuseAnswer && (Game_mode & GM_TEAM))
				{
					RefuseThisPlayer=1;
					HUD_init_message ("Player accepted!");
					RefuseTeam=2;
					game_flush_inputs();
				}
			return 1;
#endif

		default:
			break;

	}	 //switch (key)

	if (!Player_is_dead)
		switch (key)
		{
#ifndef D2X_KEYS // weapon selection handled in controls_read_all, d1x-style
				// MWA changed the weapon select cases to have each case call
				// do_weapon_select the macintosh keycodes aren't consecutive from 1
				// -- 0 on the keyboard -- boy is that STUPID!!!!
				//	Select primary or secondary weapon.
			case KEY_1:
				do_weapon_select(0 , 0);
				break;
			case KEY_2:
				do_weapon_select(1 , 0);
				break;
			case KEY_3:
				do_weapon_select(2 , 0);
				break;
			case KEY_4:
				do_weapon_select(3 , 0);
				break;
			case KEY_5:
				do_weapon_select(4 , 0);
				break;
				
			case KEY_6:
				do_weapon_select(0 , 1);
				break;
			case KEY_7:
				do_weapon_select(1 , 1);
				break;
			case KEY_8:
				do_weapon_select(2 , 1);
				break;
			case KEY_9:
				do_weapon_select(3 , 1);
				break;
			case KEY_0:
				do_weapon_select(4 , 1);
				break;
#endif
				
				KEY_MAC(case KEY_COMMAND+KEY_SHIFTED+KEY_5:)
			case KEY_F5 + KEY_SHIFTED:
				DropCurrentWeapon();
				break;
	
			KEY_MAC(case KEY_COMMAND+KEY_SHIFTED+KEY_6:)
			case KEY_F6 + KEY_SHIFTED:
				DropSecondaryWeapon();
				break;

#ifdef NETWORK
			case KEY_0 + KEY_ALTED:
				DropFlag ();
				game_flush_inputs();
				break;
#endif
	
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
	
	return 1;
}

void kill_all_robots(void)
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
// --			HUD_init_message("Toasted the BOSS!");

	// Toast the buddy if nothing else toasted!
	if (dead_count == 0)
		for (i=0; i<=Highest_object_index; i++)
			if (Objects[i].type == OBJ_ROBOT)
				if (Robot_info[Objects[i].id].companion) {
					Objects[i].flags |= OF_EXPLODING|OF_SHOULD_BE_DEAD;
					HUD_init_message("Toasted the Buddy! *sniff*");
					dead_count++;
				}

	HUD_init_message("%i robots toasted!", dead_count);
}

//	--------------------------------------------------------------------------
//	Detonate reactor.
//	Award player all powerups in mine.
//	Place player just outside exit.
//	Kill all bots in mine.
//	Yippee!!
void kill_and_so_forth(void)
{
	int     i, j;

	HUD_init_message("Killing, awarding, etc.!");

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
		if (Triggers[i].type == TT_EXIT) {
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

	HUD_init_message("%i robots toasted!", dead_count);
}

void kill_thief(void)
{
	int     i;

	//	Kill thief.
	for (i=0; i<=Highest_object_index; i++)
		if (Objects[i].type == OBJ_ROBOT)
			if (Robot_info[Objects[i].id].thief) {
				Objects[i].flags |= OF_EXPLODING|OF_SHOULD_BE_DEAD;
				HUD_init_message("Thief toasted!");
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
				HUD_init_message("Buddy toasted!");
			}
}

int HandleTestKey(int key)
{
	switch (key)
	{

#ifdef SHOW_EXIT_PATH
		case KEY_DEBUGGED+KEY_1:	create_special_path();	break;
#endif

		case KEY_DEBUGGED+KEY_Y:
			do_controlcen_destroyed_stuff(NULL);
			break;

#ifdef NETWORK
	case KEY_DEBUGGED+KEY_ALTED+KEY_D:
			PlayerCfg.NetlifeKills=4000; PlayerCfg.NetlifeKilled=5;
			multi_add_lifetime_kills();
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

		case KEY_DEBUGGED+KEY_R+KEY_SHIFTED:
			kill_all_robots();
			break;

#ifdef EDITOR		//editor-specific functions

		case KEY_E + KEY_DEBUGGED:
#ifdef NETWORK
			network_leave_game();
#endif
			Function_mode = FMODE_EDITOR;

			keyd_editor_mode = 1;
			editor();
			if ( Function_mode == FMODE_GAME ) {
				Game_mode = GM_EDITOR;
				editor_reset_stuff_on_level();
				N_players = 1;
			}
			break;
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
			*Toggle_var = !*Toggle_var;
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
				HUD_init_message( "Debug Spew: ON" );
			} else {
				HUD_init_message( "Debug Spew: OFF" );
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
			m.type=NM_TYPE_INPUT; m.text_len = FILENAME_LEN; m.text = text;
			item = newmenu_do( NULL, "Briefing to play?", 1, &m, NULL, NULL );
			if (item != -1) {
				do_briefing_screens(text,1);
				reset_cockpit();
			}
			break;
		}

		case KEY_DEBUGGED+KEY_ALTED+KEY_F5:
			GameTime = i2f(0x7fff - 840);		//will overflow in 14 minutes
			break;

		case KEY_DEBUGGED+KEY_SHIFTED+KEY_B:
			if (Player_is_dead)
				return 0;

			kill_and_so_forth();
			break;
		case KEY_DEBUGGED+KEY_G:
			GameTime = i2f(0x7fff - 600) - (F1_0*10);
			HUD_init_message("GameTime %i - Reset in 10 seconds!", GameTime);
			break;
		default:
			return 0;
			break;
	}
	
	return 1;
}
#endif		//#ifndef RELEASE

//	Cheat functions ------------------------------------------------------------
extern char *jcrypt (char *);

char *LamerCheats[]={   "!UyN#E$I",	// gabba-gabbahey
								"ei5cQ-ZQ", // mo-therlode
								"q^EpZxs8", // c-urrygoat
								"mxk(DyyP", // zi-ngermans
								"cBo#@y@P", // ea-tangelos
								"CLygLBGQ", // e-ricaanne
								"xAnHQxZX", // jos-huaakira
								"cKc[KUWo", // wh-ammazoom
							};

#define N_LAMER_CHEATS (sizeof(LamerCheats) / sizeof(*LamerCheats))

char *WowieCheat        ="F_JMO3CV";    //only Matt knows / h-onestbob
char *AllKeysCheat      ="%v%MrgbU";    //only Matt knows / or-algroove
char *InvulCheat        ="Wv_\\JJ\\Z";  //only Matt knows / almighty
char *HomingCheatString ="t\\LIhSB[";   //only Matt knows / l-pnlizard
char *BouncyCheat       ="bGbiChQJ";    //only Matt knows / duddaboo
char *FullMapCheat      ="PI<XQHRI";    //only Matt knows / rockrgrl
char *LevelWarpCheat    ="ZQHtqbb\"";   //only Matt knows / f-reespace
char *MonsterCheat      ="nfpEfRQp";    //only Matt knows / godzilla
char *BuddyLifeCheat    ="%A-BECuY";    //only Matt knows / he-lpvishnu
char *BuddyDudeCheat    ="u#uzIr%e";    //only Matt knows / g-owingnut
char *KillRobotsCheat   ="&wxbs:5O";    //only Matt knows / spaniard
char *FinishLevelCheat  ="%bG_bZ<D";    //only Matt knows / d-elshiftb
char *RapidFireCheat    ="*jLgHi'J";    //only Matt knows / wildfire

char *RobotsKillRobotsCheat ="rT6xD__S"; // New for 1.1 / silkwing
char *AhimsaCheat       ="!Uscq_yc";    // New for 1.1 / im-agespace 

char *AccessoryCheat    ="dWdz[kCK";    // al-ifalafel
char *AcidCheat         ="qPmwxz\"S";   // bit-tersweet
char *FramerateCheat    ="rQ60#ZBN";    // f-rametime

char CheatBuffer[]="AAAAAAAAAAAAAAA";

#define CHEATSPOT 14
#define CHEATEND 15

void do_cheat_penalty ()
 {
  digi_play_sample( SOUND_CHEATER, F1_0);
  Cheats_enabled=1;
  Players[Player_num].score=0;
 }


//	Main Cheat function

char BounceCheat=0;
char HomingCheat=0;
char AcidCheatOn=0;
char old_IntMethod;
char OldHomingState[20];
extern char Monster_mode;

extern int Robots_kill_robots_cheat;

void FinalCheats(int key)
{
  int i;
  char *cryptstring;

   key=key_ascii();

	if (key == 255) return;

  for (i=0;i<15;i++)
   CheatBuffer[i]=CheatBuffer[i+1];

  CheatBuffer[CHEATSPOT]=key;

  cryptstring=jcrypt(&CheatBuffer[7]);

	for (i=0;i<N_LAMER_CHEATS;i++)
	  if (!(strcmp (cryptstring,LamerCheats[i])))
			{
				 do_cheat_penalty();
				 Players[Player_num].shields=i2f(1);
				 Players[Player_num].energy=i2f(1);
#ifdef NETWORK
		  if (Game_mode & GM_MULTI)
			{
			 Network_message_reciever = 100;		// Send to everyone...
				sprintf( Network_message, "%s is crippled...get him!",Players[Player_num].callsign);
			}
#endif
			HUD_init_message ("Take that...cheater!");
		}

  if (!(strcmp (cryptstring,AcidCheat)))
		{
				if (AcidCheatOn)
				{
				 AcidCheatOn=0;
				 Interpolation_method=old_IntMethod;
				 HUD_init_message ("Coming down...");
				}
				else
				{
				 AcidCheatOn=1;
				 old_IntMethod=Interpolation_method;
				 Interpolation_method=1;
				 HUD_init_message ("Going up!");
				}

		}

  if (!(strcmp (cryptstring,FramerateCheat)))
		{
			GameArg.SysFPSIndicator = !GameArg.SysFPSIndicator;
		}

  if (Game_mode & GM_MULTI)
   return;

  if (!(strcmp (&CheatBuffer[8],"blueorb")))
   {
	 	if (Players[Player_num].shields < MAX_SHIELDS) {
	 		fix boost = 3*F1_0 + 3*F1_0*(NDL - Difficulty_level);
	 		if (Difficulty_level == 0)
	 			boost += boost/2;
	 		Players[Player_num].shields += boost;
	 		if (Players[Player_num].shields > MAX_SHIELDS)
	 			Players[Player_num].shields = MAX_SHIELDS;
	 		powerup_basic(0, 0, 15, SHIELD_SCORE, "%s %s %d",TXT_SHIELD,TXT_BOOSTED_TO,f2ir(Players[Player_num].shields));
			do_cheat_penalty();
	 	} else
	 		HUD_init_message(TXT_MAXED_OUT,TXT_SHIELD);
   }

  if (!(strcmp(cryptstring,BuddyLifeCheat)))
   {
	 do_cheat_penalty();
	 HUD_init_message ("What's this? Another buddy bot!");
	 create_buddy_bot();
   }


  if (!(strcmp(cryptstring,BuddyDudeCheat)))
   {
	 do_cheat_penalty();
	 Buddy_dude_cheat = !Buddy_dude_cheat;
	 if (Buddy_dude_cheat) {
		HUD_init_message ("%s gets angry!",PlayerCfg.GuidebotName);
		strcpy(PlayerCfg.GuidebotName,"Wingnut");
	 }
	 else {
		strcpy(PlayerCfg.GuidebotName,PlayerCfg.GuidebotNameReal);
		HUD_init_message ("%s calms down",PlayerCfg.GuidebotName);
	 }
  }


  if (!(strcmp(cryptstring,MonsterCheat)))
   {
    Monster_mode=1-Monster_mode;
	 do_cheat_penalty();
	 HUD_init_message (Monster_mode?"Oh no, there goes Tokyo!":"What have you done, I'm shrinking!!");
   }


  if (!(strcmp (cryptstring,BouncyCheat)))
	{
		do_cheat_penalty();
		HUD_init_message ("Bouncing weapons!");
		BounceCheat=1;
	}

	if (!(strcmp(cryptstring,LevelWarpCheat)))
	 {
		newmenu_item m;
		char text[10]="";
		int new_level_num;
		int item;
		//digi_play_sample( SOUND_CHEATER, F1_0);
		m.type=NM_TYPE_INPUT; m.text_len = 10; m.text = text;
		item = newmenu_do( NULL, TXT_WARP_TO_LEVEL, 1, &m, NULL, NULL );
		if (item != -1) {
			new_level_num = atoi(m.text);
			if (new_level_num!=0 && new_level_num>=0 && new_level_num<=Last_level) {
				StartNewLevel(new_level_num, 0);
				do_cheat_penalty();
			}
		}
	 }

  if (!(strcmp (cryptstring,WowieCheat)))
	{

				HUD_init_message(TXT_WOWIE_ZOWIE);
		do_cheat_penalty();

			if (Piggy_hamfile_version < 3) // SHAREWARE
			{
				Players[Player_num].primary_weapon_flags = ~((1<<PHOENIX_INDEX) | (1<<OMEGA_INDEX) | (1<<FUSION_INDEX) | HAS_FLAG(SUPER_LASER_INDEX));
				Players[Player_num].secondary_weapon_flags = ~((1<<SMISSILE4_INDEX) | (1<<MEGA_INDEX) | (1<<SMISSILE5_INDEX));
			}
			else
			{
				Players[Player_num].primary_weapon_flags = 0xffff ^ HAS_FLAG(SUPER_LASER_INDEX);		//no super laser
				Players[Player_num].secondary_weapon_flags = 0xffff;
			}

			for (i=0; i<MAX_PRIMARY_WEAPONS; i++)
					Players[Player_num].primary_ammo[i] = Primary_ammo_max[i];

				for (i=0; i<MAX_SECONDARY_WEAPONS; i++)
					Players[Player_num].secondary_ammo[i] = Secondary_ammo_max[i];

				if (Piggy_hamfile_version < 3) // SHAREWARE
				{
					Players[Player_num].secondary_ammo[SMISSILE4_INDEX] = 0;
					Players[Player_num].secondary_ammo[SMISSILE5_INDEX] = 0;
					Players[Player_num].secondary_ammo[MEGA_INDEX] = 0;
				}

				if (Game_mode & GM_HOARD)
					Players[Player_num].secondary_ammo[PROXIMITY_INDEX] = 12;

				if (Newdemo_state == ND_STATE_RECORDING)
					newdemo_record_laser_level(Players[Player_num].laser_level, MAX_LASER_LEVEL);

				Players[Player_num].energy = MAX_ENERGY;
				Players[Player_num].laser_level = MAX_SUPER_LASER_LEVEL;
				Players[Player_num].flags |= PLAYER_FLAGS_QUAD_LASERS;
				update_laser_weapon_info();
	}


  if (!(strcmp (cryptstring,AllKeysCheat)))
	{
		do_cheat_penalty();
				HUD_init_message(TXT_ALL_KEYS);
				Players[Player_num].flags |= PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_GOLD_KEY;
	}


  if (!(strcmp (cryptstring,InvulCheat)))
		{
		do_cheat_penalty();
				Players[Player_num].flags ^= PLAYER_FLAGS_INVULNERABLE;
				HUD_init_message("%s %s!", TXT_INVULNERABILITY, (Players[Player_num].flags&PLAYER_FLAGS_INVULNERABLE)?TXT_ON:TXT_OFF);
				Players[Player_num].invulnerable_time = GameTime+i2f(1000);
		}
  if (!(strcmp (cryptstring,AccessoryCheat)))
		{
				do_cheat_penalty();
				Players[Player_num].flags |=PLAYER_FLAGS_HEADLIGHT;
				Players[Player_num].flags |=PLAYER_FLAGS_AFTERBURNER;
				Players[Player_num].flags |=PLAYER_FLAGS_AMMO_RACK;
				Players[Player_num].flags |=PLAYER_FLAGS_CONVERTER;

				HUD_init_message ("Accessories!!");
		}
  if (!(strcmp (cryptstring,FullMapCheat)))
		{
				do_cheat_penalty();
				Players[Player_num].flags |=PLAYER_FLAGS_MAP_ALL;

				HUD_init_message ("Full Map!!");
		}


  if (!(strcmp (cryptstring,HomingCheatString)))
		{
			if (!HomingCheat) {
				do_cheat_penalty();
				HomingCheat=1;
				for (i=0;i<20;i++)
				 {
				  OldHomingState[i]=Weapon_info[i].homing_flag;
				  Weapon_info[i].homing_flag=1;
				 }
				HUD_init_message ("Homing weapons!");
			}
		}

  if (!(strcmp (cryptstring,KillRobotsCheat)))
		{
				do_cheat_penalty();
				kill_all_robots();
		}

  if (!(strcmp (cryptstring,FinishLevelCheat)))
		{
				do_cheat_penalty();
				kill_and_so_forth();
		}

	if (!(strcmp (cryptstring,RobotsKillRobotsCheat))) {
		Robots_kill_robots_cheat = !Robots_kill_robots_cheat;
		if (Robots_kill_robots_cheat) {
			HUD_init_message ("Rabid robots!");
			do_cheat_penalty();
		}
		else
			HUD_init_message ("Kill the player!");
	}

	if (!(strcmp (cryptstring,AhimsaCheat))) {
		Robot_firing_enabled = !Robot_firing_enabled;
		if (!Robot_firing_enabled) {
			HUD_init_message("%s", "Robot firing OFF!");
			do_cheat_penalty();
		}
		else
			HUD_init_message("%s", "Robot firing ON!");
	}

	if (!(strcmp (cryptstring,RapidFireCheat))) {
		if (Laser_rapid_fire) {
			Laser_rapid_fire = 0;
			HUD_init_message("%s", "Rapid fire OFF!");
		}
		else {
			Laser_rapid_fire = 0xbada55;
			do_cheat_penalty();
			HUD_init_message("%s", "Rapid fire ON!");
		}
	}

}


// Internal Cheat Menu
#ifndef RELEASE
void do_cheat_menu()
{
	int mmn;
	newmenu_item mm[16];
	char score_text[21];

	sprintf( score_text, "%d", Players[Player_num].score );

	mm[0].type=NM_TYPE_CHECK; mm[0].value=Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE; mm[0].text="Invulnerability";
	mm[1].type=NM_TYPE_CHECK; mm[1].value=Players[Player_num].flags & PLAYER_FLAGS_CLOAKED; mm[1].text="Cloaked";
	mm[2].type=NM_TYPE_CHECK; mm[2].value=0; mm[2].text="All keys";
	mm[3].type=NM_TYPE_NUMBER; mm[3].value=f2i(Players[Player_num].energy); mm[3].text="% Energy"; mm[3].min_value=0; mm[3].max_value=200;
	mm[4].type=NM_TYPE_NUMBER; mm[4].value=f2i(Players[Player_num].shields); mm[4].text="% Shields"; mm[4].min_value=0; mm[4].max_value=200;
	mm[5].type=NM_TYPE_TEXT; mm[5].text = "Score:";
	mm[6].type=NM_TYPE_INPUT; mm[6].text_len = 10; mm[6].text = score_text;
	//mm[7].type=NM_TYPE_RADIO; mm[7].value=(Players[Player_num].laser_level==0); mm[7].group=0; mm[7].text="Laser level 1";
	//mm[8].type=NM_TYPE_RADIO; mm[8].value=(Players[Player_num].laser_level==1); mm[8].group=0; mm[8].text="Laser level 2";
	//mm[9].type=NM_TYPE_RADIO; mm[9].value=(Players[Player_num].laser_level==2); mm[9].group=0; mm[9].text="Laser level 3";
	//mm[10].type=NM_TYPE_RADIO; mm[10].value=(Players[Player_num].laser_level==3); mm[10].group=0; mm[10].text="Laser level 4";

	mm[7].type=NM_TYPE_NUMBER; mm[7].value=Players[Player_num].laser_level+1; mm[7].text="Laser Level"; mm[7].min_value=0; mm[7].max_value=MAX_SUPER_LASER_LEVEL+1;
	mm[8].type=NM_TYPE_NUMBER; mm[8].value=Players[Player_num].secondary_ammo[CONCUSSION_INDEX]; mm[8].text="Missiles"; mm[8].min_value=0; mm[8].max_value=200;

	mmn = newmenu_do("Wimp Menu",NULL,9, mm, NULL, NULL );

	if (mmn > -1 )  {
		if ( mm[0].value )  {
			Players[Player_num].flags |= PLAYER_FLAGS_INVULNERABLE;
			Players[Player_num].invulnerable_time = GameTime+i2f(1000);
		} else
			Players[Player_num].flags &= ~PLAYER_FLAGS_INVULNERABLE;
		if ( mm[1].value )
		{
			Players[Player_num].flags |= PLAYER_FLAGS_CLOAKED;
			#ifdef NETWORK
			if (Game_mode & GM_MULTI)
				multi_send_cloak();
			#endif
			ai_do_cloak_stuff();
			Players[Player_num].cloak_time = (GameTime+CLOAK_TIME_MAX>i2f(0x7fff-600)?GameTime-i2f(0x7fff-600):GameTime);
		}
		else
			Players[Player_num].flags &= ~PLAYER_FLAGS_CLOAKED;

		if (mm[2].value) Players[Player_num].flags |= PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_GOLD_KEY;
		Players[Player_num].energy=i2f(mm[3].value);
		Players[Player_num].shields=i2f(mm[4].value);
		Players[Player_num].score = atoi(mm[6].text);
		//if (mm[7].value) Players[Player_num].laser_level=0;
		//if (mm[8].value) Players[Player_num].laser_level=1;
		//if (mm[9].value) Players[Player_num].laser_level=2;
		//if (mm[10].value) Players[Player_num].laser_level=3;
		Players[Player_num].laser_level = mm[7].value-1;
		Players[Player_num].secondary_ammo[CONCUSSION_INDEX] = mm[8].value;
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


void advance_sound()
{
	if (++test_sound_num == N_TEST_SOUNDS)
		test_sound_num=0;

}


int     Test_sound = 251;

void play_test_sound()
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

	if (!Endlevel_sequence && !con_render)  //this was taken out of the if statement by WraithX
	{

		if ( (Newdemo_state == ND_STATE_PLAYBACK) || (DefiningMarkerMessage)
#ifdef NETWORK
			|| multi_sending_message || multi_defining_message
#endif
			) 	// WATCH OUT!!! WEIRD CODE ABOVE!!!
			memset( &Controls, 0, sizeof(control_info) );
		else
			controls_read_all(0);		//NOTE LINK TO ABOVE!!!

		check_rear_view();

		// If automap key pressed, enable automap unless you are in network mode, control center destroyed and < 10 seconds left
		if ( Controls.automap_down_count && !((Game_mode & GM_MULTI) && Control_center_destroyed && (Countdown_seconds_left < 10)))
		{
			do_automap(0);
			return 1;
		}
		
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

	if (event->type == EVENT_KEY_COMMAND)
	{
		key = ((d_event_keycommand *)event)->keycode;

		if (con_events(key) && con_render)
			return 1;

		if (DefiningMarkerMessage)
		{
			return MarkerInputMessage(key);
		}

#ifdef NETWORK
		if ( (Game_mode & GM_MULTI) && (multi_sending_message || multi_defining_message) )
		{
			return multi_message_input_sub(key);
		}
#endif

#ifndef RELEASE
#ifdef NETWORK
		if ((key&KEY_DEBUGGED)&&(Game_mode&GM_MULTI))   {
			Network_message_reciever = 100;		// Send to everyone...
			sprintf( Network_message, "%s %s", TXT_I_AM_A, TXT_CHEATER);
		}
#endif
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
			FinalCheats(key);

			if (HandleSystemKey(key)) return 1;
			if (HandleVRKey(key)) return 1;
			if (HandleGameKey(key)) return 1;
		}

#ifndef RELEASE
		if (HandleTestKey(key))
			return 1;
#endif
		
		if (Player_is_dead)
			return HandleDeathKey(key);
	}
	
	return 0;
}

