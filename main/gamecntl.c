/* $Id: gamecntl.c,v 1.21 2003-10-10 09:36:35 btb Exp $ */
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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

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
#include "gameseg.h"
#include "wall.h"
#include "ai.h"
#include "digi.h"
#include "ibitblt.h"
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
#include "newmenu.h"
#ifdef NETWORK
#include "network.h"
#endif
#include "gamefont.h"
#include "endlevel.h"
#include "joydefs.h"
#include "kconfig.h"
#include "mouse.h"
#include "titles.h"
#include "gr.h"
#include "playsave.h"
#include "movie.h"
#include "scores.h"
#ifdef MACINTOSH
#include "songs.h"
#endif

#if defined (TACTILE)
#include "tactile.h"
#endif

#include "pa_enabl.h"
#include "multi.h"
#include "desc_id.h"
#include "cntrlcen.h"
#include "pcx.h"
#include "state.h"
#include "piggy.h"
#include "multibot.h"
#include "ai.h"
#include "rbaudio.h"
#include "switch.h"
#include "escort.h"

#ifdef POLY_ACC
# include "poly_acc.h"
#endif

//#define TEST_TIMER    1		//if this is set, do checking on timer

#define SHOW_EXIT_PATH  1

#define Arcade_mode 0


#ifdef EDITOR
#include "editor/editor.h"
#endif

//#define _MARK_ON 1
#ifdef __WATCOMC__
#if __WATCOMC__ < 1000
#include <wsample.h>		//should come after inferno.h to get mark setting
#endif
#endif

#ifdef SDL_INPUT
#include <SDL.h>
#endif

extern void full_palette_save(void);
extern void object_goto_prev_viewer(void);

// Global Variables -----------------------------------------------------------

int	redbook_volume = 255;


//	External Variables ---------------------------------------------------------

extern int	Speedtest_on;			 // Speedtest global adapted from game.c
extern int Guided_in_big_window;
extern char WaitForRefuseAnswer,RefuseThisPlayer,RefuseTeam;

#ifndef NDEBUG
extern int	Mark_count;
extern int	Speedtest_start_time;
extern int	Speedtest_segnum;
extern int	Speedtest_sidenum;
extern int	Speedtest_frame_start;
extern int	Speedtest_count;
#endif

extern int	Global_missile_firing_count;
extern int	Automap_flag;
extern int	Config_menu_flag;
extern int  EscortHotKeys;


extern int	Game_aborted;
extern int	*Toggle_var;

extern int	Physics_cheat_flag;

extern int	last_drawn_cockpit[2];

extern int	Debug_spew;
extern int	Debug_pause;
extern cvar_t   r_framerate;

extern fix	Show_view_text_timer;

extern ubyte DefiningMarkerMessage;

//	Function prototypes --------------------------------------------------------


extern void CyclePrimary();
extern void CycleSecondary();
extern void InitMarkerInput();
extern void MarkerInputMessage (int);
extern void grow_window(void);
extern void shrink_window(void);
extern int	allowed_to_fire_missile(void);
extern int	allowed_to_fire_flare(void);
extern void	check_rear_view(void);
extern int	create_special_path(void);
extern void move_player_2_segment(segment *seg, int side);
extern void	kconfig_center_headset(void);
extern void game_render_frame_mono(void);
extern void newdemo_strip_frames(char *, int);
extern void toggle_cockpit(void);
extern int  dump_used_textures_all(void);
extern void DropMarker();
extern void DropSecondaryWeapon();
extern void DropCurrentWeapon();

void FinalCheats(int key);

#ifndef RELEASE
void do_cheat_menu(void);
#endif

void HandleGameKey(int key);
int HandleSystemKey(int key);
void HandleTestKey(int key);
void HandleVRKey(int key);

void speedtest_init(void);
void speedtest_frame(void);
void advance_sound(void);
void play_test_sound(void);

#ifdef MACINTOSH
extern void macintosh_quit(void);	// dialog-style quit function
#endif

#define key_isfunc(k) (((k&0xff)>=KEY_F1 && (k&0xff)<=KEY_F10) || (k&0xff)==KEY_F11 || (k&0xff)==KEY_F12)
#define key_ismod(k)  ((k&0xff)==KEY_LALT || (k&0xff)==KEY_RALT || (k&0xff)==KEY_LSHIFT || (k&0xff)==KEY_RSHIFT || (k&0xff)==KEY_LCTRL || (k&0xff)==KEY_RCTRL)

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

		if (Players[Player_num].energy <= INITIAL_ENERGY)
			HUD_init_message("Need more than %i energy to enable transfer", f2i(INITIAL_ENERGY));
		else
			HUD_init_message("No transfer: Shields already at max");
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


void do_weapon_stuff(void)
{
  int i;

	if (Controls.fire_flare_down_count)
		if (allowed_to_fire_flare())
			Flare_create(ConsoleObject);

	if (allowed_to_fire_missile())
		Global_missile_firing_count += Weapon_info[Secondary_weapon_to_weapon_info[Secondary_weapon]].fire_count * (Controls.fire_secondary_state || Controls.fire_secondary_down_count);

	if (Global_missile_firing_count) {
		do_missile_firing(1);			//always enable autoselect for normal missile firing
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
		int ssw_save = Secondary_weapon;

		while (Controls.drop_bomb_down_count--) {
			int ssw_save2;

			ssw_save2 = Secondary_weapon = which_bomb();

			do_missile_firing(Secondary_weapon == ssw_save);	//only allow autoselect if bomb is actually selected

			if (Secondary_weapon != ssw_save2 && ssw_save == ssw_save2)
				ssw_save = Secondary_weapon;    //if bomb was selected, and we ran out & autoselect, then stick with new selection
		}

		Secondary_weapon = ssw_save;
	}
}


int Game_paused;
char *Pause_msg;

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

extern int Redbook_playing;
void do_show_netgame_help();

//Process selected keys until game unpaused. returns key that left pause (p or esc)
int do_game_pause()
{
	int key;
	char msg[1000];
	char total_time[9],level_time[9];

	key=0;

	if (Game_paused) {		//unpause!
		Game_paused=0;
      #if defined (TACTILE)
			if (TactileStick)
			  EnableForces();
		#endif
		return KEY_PAUSE;
	}

#ifdef NETWORK
	if (Game_mode & GM_NETWORK)
	{
	 do_show_netgame_help();
    return (KEY_PAUSE);
	}
	else if (Game_mode & GM_MULTI)
	 {
	  HUD_init_message ("You cannot pause in a modem/serial game!");
	  return (KEY_PAUSE);
	 }
#endif

	digi_pause_all();
	RBAPause();
	stop_time();

	palette_save();
	apply_modified_palette();
	reset_palette_add();

// -- Matt: This is a hacked-in test for the stupid menu/flash problem.
//	We need a new brightening primitive if we want to make this not horribly ugly.
//		  Gr_scanline_darkening_level = 2;
//		  gr_rect(0, 0, 319, 199);

	game_flush_inputs();

	Game_paused=1;

   #if defined (TACTILE)
	if (TactileStick)
		  DisableForces();
	#endif


//	set_screen_mode( SCREEN_MENU );
	set_popup_screen();
	gr_palette_load( gr_palette );

	format_time(total_time, f2i(Players[Player_num].time_total) + Players[Player_num].hours_total*3600);
	format_time(level_time, f2i(Players[Player_num].time_level) + Players[Player_num].hours_level*3600);

   if (Newdemo_state!=ND_STATE_PLAYBACK)
		sprintf(msg,"PAUSE\n\nSkill level:  %s\nHostages on board:  %d\nTime on level: %s\nTotal time in game: %s",(*(&TXT_DIFFICULTY_1 + (Difficulty_level))),Players[Player_num].hostages_on_board,level_time,total_time);
   else
	  	sprintf(msg,"PAUSE\n\nSkill level:  %s\nHostages on board:  %d\n",(*(&TXT_DIFFICULTY_1 + (Difficulty_level))),Players[Player_num].hostages_on_board);

	show_boxed_message(Pause_msg=msg);		  //TXT_PAUSE);
	gr_update();

#ifdef SDL_INPUT
	/* give control back to the WM */
	if (FindArg("-grabmouse"))
	    SDL_WM_GrabInput(SDL_GRAB_OFF);
#endif

	while (Game_paused) 
	{
		int screen_changed;

	#if defined (WINDOWS)

		if (!(VR_screen_flags & VRF_COMPATIBLE_MENUS)) {
			show_boxed_message(msg);
		}

	SkipPauseStuff:

		while (!(key = key_inkey()))
		{
			MSG wmsg;
			DoMessageStuff(&wmsg);
			if (_RedrawScreen) {
				mprintf((0, "Redrawing paused screen.\n"));
				_RedrawScreen = FALSE;
				if (VR_screen_flags & VRF_COMPATIBLE_MENUS) 
					game_render_frame();
				Screen_mode = -1;
				set_popup_screen();
				gr_palette_load(gr_palette);
				show_boxed_message(msg);
				if (Cockpit_mode==CM_FULL_COCKPIT || Cockpit_mode==CM_STATUS_BAR)
					if (!GRMODEINFO(modex)) render_gauges();
			}
		}

	#else
		key = key_getch();
	#endif

		#ifndef RELEASE
		HandleTestKey(key);
		#endif
		
		screen_changed = HandleSystemKey(key);

	#ifdef WINDOWS
		if (screen_changed == -1) {
			nm_messagebox(NULL,1, TXT_OK, "Unable to do this\noperation while paused under\n320x200 mode"); 
			goto SkipPauseStuff;
		}
	#endif

		HandleVRKey(key);

		if (screen_changed) {
//			game_render_frame();
			WIN(set_popup_screen());
			show_boxed_message(msg);
			//show_extra_views();
			if (Cockpit_mode==CM_FULL_COCKPIT || Cockpit_mode==CM_STATUS_BAR)
				render_gauges();
		}
	}

#ifdef SDL_INPUT
	/* keep the mouse from wandering in SDL */
	if (FindArg("-grabmouse"))
	    SDL_WM_GrabInput(SDL_GRAB_ON);
#endif

	if (VR_screen_flags & VRF_COMPATIBLE_MENUS) {
		clear_boxed_message();
	}

	game_flush_inputs();

	reset_cockpit();

	palette_restore();

	start_time();

	if (Redbook_playing)
		RBAResume();
	digi_resume_all();
	
	MAC(delay(500);)	// delay 1/2 second because of dumb redbook problem

	return key;
}

extern int newmenu_dotiny2( char * title, char * subtitle, int nitems, newmenu_item * item, void (*subfunction)(int nitems,newmenu_item * items, int * last_key, int citem) );
extern int network_who_is_master(),network_how_many_connected(),GetMyNetRanking();
extern int TotalMissedPackets,TotalPacketsGot;
extern char Pauseable_menu;
char *NetworkModeNames[]={"Anarchy","Team Anarchy","Robo Anarchy","Cooperative","Capture the Flag","Hoard","Team Hoard","Unknown"};
extern char *RankStrings[];
extern int PhallicLimit,PhallicMan;

#ifdef NETWORK
void do_show_netgame_help()
 {
	newmenu_item m[30];
   char mtext[30][50];
	int i,num=0,eff;
#ifndef RELEASE
	int pl;
#endif
	char *eff_strings[]={"trashing","really hurting","seriously effecting","hurting",
								"effecting","tarnishing"};

   for (i=0;i<30;i++)
	{
	 m[i].text=(char *)&mtext[i];
    m[i].type=NM_TYPE_TEXT;
	}

   sprintf (mtext[num],"Game: %s",Netgame.game_name); num++;
   sprintf (mtext[num],"Mission: %s",Netgame.mission_title); num++;
	sprintf (mtext[num],"Current Level: %d",Netgame.levelnum); num++;
	sprintf (mtext[num],"Difficulty: %s",MENU_DIFFICULTY_TEXT(Netgame.difficulty)); num++;
	sprintf (mtext[num],"Game Mode: %s",NetworkModeNames[Netgame.gamemode]); num++;
	sprintf (mtext[num],"Game Master: %s",Players[network_who_is_master()].callsign); num++;
   sprintf (mtext[num],"Number of players: %d/%d",network_how_many_connected(),Netgame.max_numplayers); num++;
   sprintf (mtext[num],"Packets per second: %d",Netgame.PacketsPerSec); num++;
   sprintf (mtext[num],"Short Packets: %s",Netgame.ShortPackets?"Yes":"No"); num++;

#ifndef RELEASE
		pl=(int)(((float)TotalMissedPackets/(float)TotalPacketsGot)*100.0);
		if (pl<0)
		  pl=0;
		sprintf (mtext[num],"Packets lost: %d (%d%%)",TotalMissedPackets,pl); num++;
#endif

   if (Netgame.KillGoal)
     { sprintf (mtext[num],"Kill goal: %d",Netgame.KillGoal*5); num++; }

   sprintf (mtext[num]," "); num++;
   sprintf (mtext[num],"Connected players:"); num++;

   NetPlayers.players[Player_num].rank=GetMyNetRanking();

   for (i=0;i<N_players;i++)
     if (Players[i].connected)
	  {		  
      if (!FindArg ("-norankings"))
		 {
			if (i==Player_num)
				sprintf (mtext[num],"%s%s (%d/%d)",RankStrings[NetPlayers.players[i].rank],Players[i].callsign,Netlife_kills,Netlife_killed); 
			else
				sprintf (mtext[num],"%s%s %d/%d",RankStrings[NetPlayers.players[i].rank],Players[i].callsign,kill_matrix[Player_num][i],
							kill_matrix[i][Player_num]); 
			num++;
		 }
	   else
  		 sprintf (mtext[num++],"%s",Players[i].callsign); 
	  }

	
  sprintf (mtext[num]," "); num++;

  eff=(int)((float)((float)Netlife_kills/((float)Netlife_killed+(float)Netlife_kills))*100.0);

  if (eff<0)
	eff=0;
  
  if (Game_mode & GM_HOARD)
	{
	 if (PhallicMan==-1)
		 sprintf (mtext[num],"There is no record yet for this level."); 
	 else
		 sprintf (mtext[num],"%s has the record at %d points.",Players[PhallicMan].callsign,PhallicLimit); 
	num++;
	}
  else if (!FindArg ("-norankings"))
	{
	  if (eff<60)
	   {
		 sprintf (mtext[num],"Your lifetime efficiency of %d%%",eff); num++;
		 sprintf (mtext[num],"is %s your ranking.",eff_strings[eff/10]); num++;
		}
	  else
	   {
		 sprintf (mtext[num],"Your lifetime efficiency of %d%%",eff); num++;
		 sprintf (mtext[num],"is serving you well."); num++;
	   }
	}  
	

  	full_palette_save();

   Pauseable_menu=1;
	newmenu_dotiny2( NULL, "Netgame Information", num, m, NULL);

	palette_restore();
}
#endif

void HandleEndlevelKey(int key)
{

	#ifdef MACINTOSH
	if ( key == (KEY_COMMAND + KEY_SHIFTED + KEY_3) )
		save_screen_shot(0);

	if ( key == KEY_COMMAND+KEY_Q && !(Game_mode & GM_MULTI) )
		macintosh_quit();
	#endif

	if (key==KEY_PRINT_SCREEN)
		save_screen_shot(0);

	#ifdef MACINTOSH
	if ( key == (KEY_COMMAND+KEY_P) )
		key = do_game_pause();
	#endif
	if (key == KEY_PAUSE)
		key = do_game_pause();		//so esc from pause will end level

	if (key == KEY_ESC) {
		stop_endlevel_sequence();
		last_drawn_cockpit[0]=-1;
		last_drawn_cockpit[1]=-1;
		return;
	}

	if (key == KEY_BACKSP)
		Int3();
}

void HandleDeathKey(int key)
{
/*
	Commented out redundant calls because the key used here typically
	will be passed to HandleSystemKey later.  Note that I do this to pause
	which is supposed to pass the ESC key to leave the level.  This
	doesn't work in the DOS version anyway.   -Samir 
*/

	if (Player_exploded && !key_isfunc(key) && !key_ismod(key))
		Death_sequence_aborted  = 1;		//Any key but func or modifier aborts

	#ifdef MACINTOSH
	if ( key == (KEY_COMMAND + KEY_SHIFTED + KEY_3) ) {
//		save_screen_shot(0);
		Death_sequence_aborted  = 0;		// Clear because code above sets this for any key.
	}

	if ( key == KEY_COMMAND+KEY_Q && !(Game_mode & GM_MULTI) )
		macintosh_quit();
	#endif

	if (key==KEY_PRINT_SCREEN) {
//		save_screen_shot(0);
		Death_sequence_aborted  = 0;		// Clear because code above sets this for any key.
	}

	#ifdef MACINTOSH
	if ( key == (KEY_COMMAND+KEY_P) ) {
//		key = do_game_pause();
		Death_sequence_aborted  = 0;		// Clear because code above sets this for any key.
	}
	#endif

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

void HandleDemoKey(int key)
{
	switch (key) {

		case KEY_F3:
				
			#ifdef MACINTOSH
				#ifdef POLY_ACC
					if (PAEnabled)
					{
						HUD_init_message("Cockpit not available while using QuickDraw 3D.");
						return;
					}
				#endif
			#endif
				
			 PA_DFX (HUD_init_message ("Cockpit not available in 3dfx version."));
			 PA_DFX (break);
			 if (!(Guided_missile[Player_num] && Guided_missile[Player_num]->type==OBJ_WEAPON && Guided_missile[Player_num]->id==GUIDEDMISS_ID && Guided_missile[Player_num]->signature==Guided_missile_sig[Player_num] && Guided_in_big_window))
				toggle_cockpit();
			 break;

		case KEY_SHIFTED+KEY_MINUS:
		case KEY_MINUS:		shrink_window(); break;

		case KEY_SHIFTED+KEY_EQUAL:
		case KEY_EQUAL:		grow_window(); break;

		MAC(case KEY_COMMAND+KEY_2:)
		case KEY_F2:		Config_menu_flag = 1; break;

		MAC(case KEY_COMMAND+KEY_7:)
		case KEY_F7:
			#ifdef NETWORK
			Show_kill_list = (Show_kill_list+1) % ((Newdemo_game_mode & GM_TEAM) ? 4 : 3);
			#endif
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

		MAC(case KEY_COMMAND+KEY_P:)
		case KEY_PAUSE:
			do_game_pause();
			break;

		MAC(case KEY_COMMAND + KEY_SHIFTED + KEY_3:)
		case KEY_PRINT_SCREEN: {
			int old_state;

			old_state = Newdemo_vcr_state;
			Newdemo_vcr_state = ND_STATE_PRINTSCREEN;
			game_render_frame_mono();
			save_screen_shot(0);
			Newdemo_vcr_state = old_state;
			break;
		}

		#ifdef MACINTOSH
		case KEY_COMMAND+KEY_Q:
			if ( !(Game_mode & GM_MULTI) )
				macintosh_quit();
			break;
		#endif

		#ifndef NDEBUG
		case KEY_BACKSP:
			Int3();
			break;
		case KEY_DEBUGGED + KEY_I:
			Newdemo_do_interpolate = !Newdemo_do_interpolate;
			if (Newdemo_do_interpolate)
				mprintf ((0, "demo playback interpolation now on\n"));
			else
				mprintf ((0, "demo playback interpolation now off\n"));
			break;
		case KEY_DEBUGGED + KEY_K: {
			int how_many, c;
			char filename[FILENAME_LEN], num[16];
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

			break;
		}
		#endif

	}
}

//switch a cockpit window to the next function
int select_next_window_function(int w)
{
	Assert(w==0 || w==1);

	switch (Cockpit_3d_view[w]) {
		case CV_NONE:
			Cockpit_3d_view[w] = CV_REAR;
			break;
		case CV_REAR:
			if (find_escort()) {
				Cockpit_3d_view[w] = CV_ESCORT;
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
				Cockpit_3d_view[w] = CV_COOP;
				while (1) {
					Coop_view_player[w]++;
					if (Coop_view_player[w] == N_players) {
						Cockpit_3d_view[w] = CV_MARKER;
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
				Cockpit_3d_view[w] = CV_MARKER;
				if (Marker_viewer_num[w] == -1)
					Marker_viewer_num[w] = Player_num * 2;
				else if (Marker_viewer_num[w] == Player_num * 2)
					Marker_viewer_num[w]++;
				else
					Cockpit_3d_view[w] = CV_NONE;
			}
			else
#endif
				Cockpit_3d_view[w] = CV_NONE;
			break;
	}
	write_player_file();

	return 1;	 //screen_changed
}

extern int Game_paused;

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
	int screen_changed=0;

	if (!Player_is_dead)
		switch (key) {

			#ifdef DOOR_DEBUGGING
			case KEY_LAPOSTRO+KEY_SHIFTED:
				dump_door_debugging_info();
				break;
			#endif

			case KEY_ESC:
				if (Game_paused)
					Game_paused=0;
				else {
					Game_aborted=1;
					Function_mode = FMODE_MENU;
				}
				break;

// fleshed these out because F1 and F2 aren't sequenctial keycodes on mac -- MWA

			MAC(case KEY_COMMAND+KEY_SHIFTED+KEY_1:)
			case KEY_SHIFTED+KEY_F1:
				screen_changed = select_next_window_function(0);
				break;
			MAC(case KEY_COMMAND+KEY_SHIFTED+KEY_2:)
			case KEY_SHIFTED+KEY_F2:
				screen_changed = select_next_window_function(1);
				break;
		}

	switch (key) {

#if 1
		case KEY_SHIFTED + KEY_ESC:
			con_show();
			break;

#else
		case KEY_SHIFTED + KEY_ESC:     //quick exit
			#ifdef EDITOR
				if (! SafetyCheck()) break;
				close_editor_screen();
			#endif

			Game_aborted=1;
			Function_mode=FMODE_EXIT;
			break;
#endif

		MAC( case KEY_COMMAND+KEY_P: )
		case KEY_PAUSE: 
			do_game_pause();				break;

		#ifdef MACINTOSH
		case KEY_COMMAND + KEY_D:
			Scanline_double = !Scanline_double;
			init_cockpit();
			break;
		#endif

		MAC(case KEY_COMMAND + KEY_SHIFTED + KEY_3:)
		case KEY_PRINT_SCREEN:  save_screen_shot(0);		break;

		MAC(case KEY_COMMAND+KEY_1:)
		case KEY_F1:					do_show_help();			break;

		MAC(case KEY_COMMAND+KEY_2:)
		case KEY_F2:					//Config_menu_flag = 1; break;
			{
				int scanline_save = Scanline_double;

				if (!(Game_mode&GM_MULTI)) {palette_save(); apply_modified_palette(); reset_palette_add(); gr_palette_load(gr_palette); }
				do_options_menu();
				if (!(Game_mode&GM_MULTI)) palette_restore();
				if (scanline_save != Scanline_double)   init_cockpit();	// reset the cockpit after changing...
	         PA_DFX (init_cockpit());
				break;
			}


		MAC(case KEY_COMMAND+KEY_3:)

		case KEY_F3:
			#ifdef WINDOWS		// HACK! these shouldn't work in 320x200 pause or in letterbox.
				if (Player_is_dead) break;
				if (!(VR_screen_flags&VRF_COMPATIBLE_MENUS) && Game_paused) {
					screen_changed = -1;
					break;
				}
			#endif
	
			#ifdef MACINTOSH
				#ifdef POLY_ACC
					if (PAEnabled)
					{
						HUD_init_message("Cockpit not available while using QuickDraw 3D.");
						return;
					}
				#endif
			#endif

			PA_DFX (HUD_init_message ("Cockpit not available in 3dfx version."));
			PA_DFX (break);

			if (!(Guided_missile[Player_num] && Guided_missile[Player_num]->type==OBJ_WEAPON && Guided_missile[Player_num]->id==GUIDEDMISS_ID && Guided_missile[Player_num]->signature==Guided_missile_sig[Player_num] && Guided_in_big_window))
			{
				toggle_cockpit();	screen_changed=1;
			}
			break;

		MAC(case KEY_COMMAND+KEY_SHIFTED+KEY_7:)
		case KEY_F7+KEY_SHIFTED: palette_save(); joydefs_calibrate(); palette_restore(); break;

		case KEY_SHIFTED+KEY_MINUS:
		case KEY_MINUS:	
		#ifdef WINDOWS
			if (Player_is_dead) break;
			if (!(VR_screen_flags&VRF_COMPATIBLE_MENUS) && Game_paused) {
				screen_changed = -1;
				break;
			}
		#endif

			shrink_window(); 
			screen_changed=1; 
			break;

		case KEY_SHIFTED+KEY_EQUAL:
		case KEY_EQUAL:			
		#ifdef WINDOWS
			if (Player_is_dead) break;
			if (!(VR_screen_flags&VRF_COMPATIBLE_MENUS) && Game_paused) {
				screen_changed = -1;
				break;
			}
		#endif

			grow_window();  
			screen_changed=1; 
			break;

		MAC(case KEY_COMMAND+KEY_5:)
		case KEY_F5:
			if ( Newdemo_state == ND_STATE_RECORDING )
				newdemo_stop_recording();
			else if ( Newdemo_state == ND_STATE_NORMAL )
				if (!Game_paused)		//can't start demo while paused
					newdemo_start_recording();
			break;

		MAC(case KEY_COMMAND+KEY_ALTED+KEY_4:)
		case KEY_ALTED+KEY_F4:
			#ifdef NETWORK
			Show_reticle_name = (Show_reticle_name+1)%2;
			#endif
			break;

		MAC(case KEY_COMMAND+KEY_7:)
		case KEY_F7:
			#ifdef NETWORK
			Show_kill_list = (Show_kill_list+1) % ((Game_mode & GM_TEAM) ? 4 : 3);
			if (Game_mode & GM_MULTI)
				multi_sort_kill_list();
		#endif
			break;

		MAC(case KEY_COMMAND+KEY_8:)
		case KEY_F8:
			#ifdef NETWORK
			multi_send_message_start();
			#endif
			break;

		case KEY_F9:
		case KEY_F10:
		case KEY_F11:
		case KEY_F12:
			#ifdef NETWORK
			multi_send_macro(key);
			#endif
			break;		// send taunt macros

		#ifdef MACINTOSH
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
			#ifdef NETWORK
			multi_define_macro(key);
			#endif
			break;		// redefine taunt macros

		#ifdef MACINTOSH
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

		#if defined(MACINTOSH) && defined(POLY_ACC)
			case KEY_COMMAND+KEY_ALTED+KEY_1:
				if (PAEnabled)
				{	// hackish, to enable RAVE filtering hotkey,
					// not widely publicized
					pa_toggle_filtering();
				}
				break;
		#endif


		MAC(case KEY_COMMAND+KEY_S:)
		MAC(case KEY_COMMAND+KEY_ALTED+KEY_2:)
		case KEY_ALTED+KEY_F2:
			if (!Player_is_dead && !((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))) {
				int     rsave, gsave, bsave;
				rsave = PaletteRedAdd;
				gsave = PaletteGreenAdd;
				bsave = PaletteBlueAdd;

				full_palette_save();
				PaletteRedAdd = rsave;
				PaletteGreenAdd = gsave;
				PaletteBlueAdd = bsave;
				state_save_all( 0, 0, NULL );
				palette_restore();
			}
			break;  // 0 means not between levels.

		MAC(case KEY_COMMAND+KEY_O:)
		MAC(case KEY_COMMAND+KEY_ALTED+KEY_3:)
		case KEY_ALTED+KEY_F3:
			if (!Player_is_dead && !((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))) {
				full_palette_save();
				state_restore_all(1, 0, NULL);
				if (Game_paused)
					do_game_pause();
			}
			break;


		MAC(case KEY_COMMAND+KEY_SHIFTED+KEY_4:)
		case KEY_F4 + KEY_SHIFTED:
			do_escort_menu();
			break;


		MAC(case KEY_COMMAND+KEY_SHIFTED+KEY_ALTED+KEY_4:)
		case KEY_F4 + KEY_SHIFTED + KEY_ALTED:
			change_guidebot_name();
			break;

		case KEY_MINUS + KEY_ALTED:     songs_goto_prev_song(); break;
		case KEY_EQUAL + KEY_ALTED:     songs_goto_next_song(); break;

		#ifdef MACINTOSH
		
		case KEY_COMMAND+KEY_M:
			#if !defined(SHAREWARE) || defined(APPLE_DEMO)
			if ( (Game_mode & GM_MULTI) )		// don't process in multiplayer games
				break;

			key_close();		// no processing of keys with keyboard handler.. jeez				
			stop_time();
			show_boxed_message ("Mounting CD\nESC to quit");	
			RBAMountDisk();		// OS has totaly control of the CD.
			if (Function_mode == FMODE_MENU)
				songs_play_song(SONG_TITLE,1);
			else if (Function_mode == FMODE_GAME)
				songs_play_level_song( Current_level_num );
			clear_boxed_message();
			key_init();
			start_time();
			#endif
			
			break;

		case KEY_COMMAND+KEY_E:
			songs_stop_redbook();
			RBAEjectDisk();
			break;

		case KEY_COMMAND+KEY_RIGHT:
			songs_goto_next_song();
			break;
		case KEY_COMMAND+KEY_LEFT:
			songs_goto_prev_song();
			break;
		case KEY_COMMAND+KEY_UP:
			songs_play_level_song(1);
			break;
		case KEY_COMMAND+KEY_DOWN:
			songs_stop_redbook();
			break;

		case KEY_COMMAND+KEY_Q:
			if ( !(Game_mode & GM_MULTI) )
				macintosh_quit();
			break;
		#endif

//added 8/23/99 by Matt Mueller for hot key res/fullscreen changing, and menu access
		case KEY_CTRLED+KEY_SHIFTED+KEY_PADDIVIDE:
		case KEY_ALTED+KEY_CTRLED+KEY_PADDIVIDE:
		case KEY_ALTED+KEY_SHIFTED+KEY_PADDIVIDE:
			d2x_options_menu();
			break;
#if 0
		case KEY_CTRLED+KEY_SHIFTED+KEY_PADMULTIPLY:
		case KEY_ALTED+KEY_CTRLED+KEY_PADMULTIPLY:
		case KEY_ALTED+KEY_SHIFTED+KEY_PADMULTIPLY:
			change_res();
			break;
		case KEY_CTRLED+KEY_SHIFTED+KEY_PADMINUS:
		case KEY_ALTED+KEY_CTRLED+KEY_PADMINUS:
		case KEY_ALTED+KEY_SHIFTED+KEY_PADMINUS:
			//lower res 
			//should we just cycle through the list that is displayed in the res change menu?
			// what if their card/X/etc can't handle that mode? hrm. 
			//well, the quick access to the menu is good enough for now.
			break;
		case KEY_CTRLED+KEY_SHIFTED+KEY_PADPLUS:
		case KEY_ALTED+KEY_CTRLED+KEY_PADPLUS:
		case KEY_ALTED+KEY_SHIFTED+KEY_PADPLUS:
			//increase res
			break;
#endif
		case KEY_CTRLED+KEY_SHIFTED+KEY_PADENTER:
		case KEY_ALTED+KEY_CTRLED+KEY_PADENTER:
		case KEY_ALTED+KEY_SHIFTED+KEY_PADENTER:
			gr_toggle_fullscreen_game();
			break;
//end addition -MM
			
//added 11/01/98 Matt Mueller
#if 0
		case KEY_CTRLED+KEY_ALTED+KEY_LAPOSTRO:
			toggle_hud_log();
			break;
#endif
//end addition -MM

		default:
			break;

	}	 //switch (key)

	return screen_changed;
}


void HandleVRKey(int key)
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
	}
}


extern void DropFlag();

void HandleGameKey(int key)
{
	switch (key) {

		#if defined(MACINTOSH)  && !defined(RELEASE)
		case KEY_COMMAND+KEY_F:	r_framerate.value = !r_framerate.value; break;
		#endif

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
		if (EscortHotKeys)
		{
			if (!(Game_mode & GM_MULTI))
				set_escort_special_goal(key);
			else
				HUD_init_message ("No Guide-Bot in Multiplayer!");
			break;
		}

		MAC(case KEY_COMMAND+KEY_SHIFTED+KEY_5:)
		case KEY_F5 + KEY_SHIFTED:
	 DropCurrentWeapon();
			break;

		MAC(case KEY_COMMAND+KEY_SHIFTED+KEY_6:)
		case KEY_F6 + KEY_SHIFTED:
	 DropSecondaryWeapon();
	 break;

#ifdef NETWORK
		case KEY_0 + KEY_ALTED:
			DropFlag ();
			break;
#endif

		MAC(case KEY_COMMAND+KEY_4:)
		case KEY_F4:
		if (!DefiningMarkerMessage)
		  InitMarkerInput();
		 break;

#ifdef NETWORK
		MAC(case KEY_COMMAND+KEY_6:)
		case KEY_F6:
			if (Netgame.RefusePlayers && WaitForRefuseAnswer && !(Game_mode & GM_TEAM))
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
				}
			break;
		case KEY_ALTED + KEY_2:
			if (Netgame.RefusePlayers && WaitForRefuseAnswer && (Game_mode & GM_TEAM))
				{
					RefuseThisPlayer=1;
					HUD_init_message ("Player accepted!");
					RefuseTeam=2;
				}
			break;
#endif

		default:
			break;

	}	 //switch (key)
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

void toggle_movie_saving(void);
extern char Language[];

void HandleTestKey(int key)
{
	switch (key) {

		case KEY_DEBUGGED+KEY_0:	show_weapon_status();   break;

		#ifdef SHOW_EXIT_PATH
		case KEY_DEBUGGED+KEY_1:	create_special_path();  break;
		#endif

		case KEY_DEBUGGED+KEY_Y:
			do_controlcen_destroyed_stuff(NULL);
			break;

#ifdef NETWORK
	case KEY_DEBUGGED+KEY_ALTED+KEY_D:
			Netlife_kills=4000; Netlife_killed=5;
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
//				if (!(Game_mode & GM_MULTI) )   {
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
//				}
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
			if (!( Game_mode & GM_MULTI ))
				move_player_2_segment(Cursegp,Curside);
			break;   //move eye to curseg


		case KEY_DEBUGGED+KEY_W:	draw_world_from_game(); break;

		#endif  //#ifdef EDITOR

		//flythrough keys
		// case KEY_DEBUGGED+KEY_SHIFTED+KEY_F: toggle_flythrough(); break;
		// case KEY_LEFT:		ft_preference=FP_LEFT; break;
		// case KEY_RIGHT:				ft_preference=FP_RIGHT; break;
		// case KEY_UP:		ft_preference=FP_UP; break;
		// case KEY_DOWN:		ft_preference=FP_DOWN; break;

#ifndef NDEBUG
		case KEY_DEBUGGED+KEY_LAPOSTRO: Show_view_text_timer = 0x30000; object_goto_next_viewer(); break;
		case KEY_DEBUGGED+KEY_CTRLED+KEY_LAPOSTRO: Show_view_text_timer = 0x30000; object_goto_prev_viewer(); break;
#endif
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

#ifndef NDEBUG
		case KEY_DEBUGGED + KEY_F11: play_test_sound(); break;
		case KEY_DEBUGGED + KEY_SHIFTED+KEY_F11: advance_sound(); play_test_sound(); break;
#endif

		case KEY_DEBUGGED +KEY_F4: {
			//fvi_info hit_data;
			//vms_vector p0 = {-0x1d99a7,-0x1b20000,0x186ab7f};
			//vms_vector p1 = {-0x217865,-0x1b20000,0x187de3e};
			//find_vector_intersection(&hit_data,&p0,0x1b9,&p1,0x40000,0x0,NULL,-1);
			break;
		}

		case KEY_DEBUGGED + KEY_M:
			Debug_spew = !Debug_spew;
			if (Debug_spew) {
				mopen( 0, 8, 1, 78, 16, "Debug Spew");
				HUD_init_message( "Debug Spew: ON" );
			} else {
				mclose( 0 );
				HUD_init_message( "Debug Spew: OFF" );
			}
			break;

		case KEY_DEBUGGED + KEY_C:

			full_palette_save();
			do_cheat_menu();
			palette_restore();
			break;
		case KEY_DEBUGGED + KEY_SHIFTED + KEY_A:
			do_megawow_powerup(10);
			break;
		case KEY_DEBUGGED + KEY_A:	{
			do_megawow_powerup(200);
//								if ( Game_mode & GM_MULTI )     {
//									nm_messagebox( NULL, 1, "Damn", "CHEATER!\nYou cannot use the\nmega-thing in network mode." );
//									Network_message_reciever = 100;		// Send to everyone...
//									sprintf( Network_message, "%s cheated!", Players[Player_num].callsign);
//								} else {
//									do_megawow_powerup();
//								}
						break;
		}

		case KEY_DEBUGGED+KEY_F:	r_framerate.value = !r_framerate.value; break;

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

		case KEY_DEBUGGED+KEY_P+KEY_SHIFTED: Debug_pause = 1; break;

		//case KEY_F7: {
		//	char mystr[30];
		//	sprintf(mystr,"mark %i start",Mark_count);
		//	_MARK_(mystr);
		//	break;
		//}
		//case KEY_SHIFTED+KEY_F7: {
		//	char mystr[30];
		//	sprintf(mystr,"mark %i end",Mark_count);
		//	Mark_count++;
		//	_MARK_(mystr);
		//	break;
		//}


		#ifndef NDEBUG
		case KEY_DEBUGGED+KEY_F8: speedtest_init(); Speedtest_count = 1;	 break;
		case KEY_DEBUGGED+KEY_F9: speedtest_init(); Speedtest_count = 10;	 break;

		case KEY_DEBUGGED+KEY_D:
			if ((Game_double_buffer = !Game_double_buffer)!=0)
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
			item = newmenu_do( NULL, "Briefing to play?", 1, &m, NULL );
			if (item != -1) {
				do_briefing_screens(text,1);
				reset_cockpit();
			}
			break;
		}

		case KEY_DEBUGGED+KEY_F5:
			toggle_movie_saving();
			break;

		case KEY_DEBUGGED+KEY_SHIFTED+KEY_F5: {
			extern int Movie_fixed_frametime;
			Movie_fixed_frametime = !Movie_fixed_frametime;
			break;
		}

		case KEY_DEBUGGED+KEY_ALTED+KEY_F5:
			GameTime = i2f(0x7fff - 840);		//will overflow in 14 minutes
			mprintf((0,"GameTime bashed to %d secs\n",f2i(GameTime)));
			break;

		case KEY_DEBUGGED+KEY_SHIFTED+KEY_B:
			kill_and_so_forth();
			break;
	}
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
char *JohnHeadCheat     ="ou]];H:%";    // p-igfarmer
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
char john_head_on=0;
char AcidCheatOn=0;
char old_IntMethod;
char OldHomingState[20];
extern char Monster_mode;

void fill_background();
void load_background_bitmap();

extern int Robots_kill_robots_cheat;

void FinalCheats(int key)
{
  int i;
  char *cryptstring;

  key=key_to_ascii(key);

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

  if (!(strcmp (cryptstring,JohnHeadCheat)))
		{
				john_head_on = !john_head_on;
				load_background_bitmap();
				fill_background();
				HUD_init_message (john_head_on?"Hi John!!":"Bye John!!");
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
			r_framerate.value = !r_framerate.value;
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
		HUD_init_message ("%s gets angry!",guidebot_name);
		strcpy(guidebot_name,"Wingnut");
	 }
	 else {
		strcpy(guidebot_name,real_guidebot_name);
		HUD_init_message ("%s calms down",guidebot_name);
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
		item = newmenu_do( NULL, TXT_WARP_TO_LEVEL, 1, &m, NULL );
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

			#ifdef SHAREWARE
				Players[Player_num].primary_weapon_flags = ~((1<<PHOENIX_INDEX) | (1<<OMEGA_INDEX) | (1<<FUSION_INDEX) | HAS_FLAG(SUPER_LASER_INDEX));
				Players[Player_num].secondary_weapon_flags = ~((1<<SMISSILE4_INDEX) | (1<<MEGA_INDEX) | (1<<SMISSILE5_INDEX));
			#else
				Players[Player_num].primary_weapon_flags = 0xffff ^ HAS_FLAG(SUPER_LASER_INDEX);		//no super laser
				Players[Player_num].secondary_weapon_flags = 0xffff;
			#endif

			for (i=0; i<MAX_PRIMARY_WEAPONS; i++)
					Players[Player_num].primary_ammo[i] = Primary_ammo_max[i];

				for (i=0; i<MAX_SECONDARY_WEAPONS; i++)
					Players[Player_num].secondary_ammo[i] = Secondary_ammo_max[i];

			#ifdef SHAREWARE
					Players[Player_num].secondary_ammo[SMISSILE4_INDEX] = 0;
					Players[Player_num].secondary_ammo[SMISSILE5_INDEX] = 0;
					Players[Player_num].secondary_ammo[MEGA_INDEX] = 0;
			#endif
						
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

	mmn = newmenu_do("Wimp Menu",NULL,9, mm, NULL );

	if (mmn > -1 )  {
		if ( mm[0].value )  {
			Players[Player_num].flags |= PLAYER_FLAGS_INVULNERABLE;
			Players[Player_num].invulnerable_time = GameTime+i2f(1000);
		} else
			Players[Player_num].flags &= ~PLAYER_FLAGS_INVULNERABLE;
		if ( mm[1].value ) {
			Players[Player_num].flags |= PLAYER_FLAGS_CLOAKED;
			#ifdef NETWORK
			if (Game_mode & GM_MULTI)
				multi_send_cloak();
			#endif
			ai_do_cloak_stuff();
			Players[Player_num].cloak_time = GameTime;
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
		char    msg[128];

		sprintf(msg, "\nSpeedtest done:  %i frames, %7.3f seconds, %7.3f frames/second.\n",
			FrameCount-Speedtest_frame_start,
			f2fl(timer_get_fixed_seconds() - Speedtest_start_time),
			(float) (FrameCount-Speedtest_frame_start) / f2fl(timer_get_fixed_seconds() - Speedtest_start_time));

		mprintf((0, "%s", msg));
		HUD_init_message(msg);

		Speedtest_count--;
		if (Speedtest_count == 0)
			Speedtest_on = 0;
		else
			speedtest_init();
	}

}


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





void ReadControls()
{
	int key;
	fix key_time;
	static ubyte exploding_flag=0;

	Player_fired_laser_this_frame=-1;

	if (!Endlevel_sequence && !Player_is_dead) {

			if ( (Newdemo_state == ND_STATE_PLAYBACK) || (DefiningMarkerMessage)
				#ifdef NETWORK
				|| multi_sending_message || multi_defining_message
				#endif
				)	 // WATCH OUT!!! WEIRD CODE ABOVE!!!
				memset( &Controls, 0, sizeof(control_info) );
			else
				#ifdef WINDOWS
					controls_read_all_win();
				#else
					controls_read_all();		//NOTE LINK TO ABOVE!!!
				#endif

		check_rear_view();

		//	If automap key pressed, enable automap unless you are in network mode, control center destroyed and < 10 seconds left
		if ( Controls.automap_down_count && !((Game_mode & GM_MULTI) && Control_center_destroyed && (Countdown_seconds_left < 10)))
			Automap_flag = 1;

		do_weapon_stuff();

	}

	if (Player_exploded) { //Player_is_dead && (ConsoleObject->flags & OF_EXPLODING) ) {

		if (exploding_flag==0)  {
			exploding_flag = 1;			// When player starts exploding, clear all input devices...
			game_flush_inputs();
		} else {
			int i;
			//if (key_down_count(KEY_BACKSP))
			//	Int3();
			//if (key_down_count(KEY_PRINT_SCREEN))
			//	save_screen_shot(0);

			#ifndef MACINTOSH
			for (i=0; i<4; i++ )
				if (joy_get_button_down_cnt(i)>0) Death_sequence_aborted = 1;
			#else
				if ( joy_get_any_button_down_cnt()>0 ) Death_sequence_aborted = 1;
			#endif
			for (i=0; i<3; i++ )
				if (mouse_button_down_count(i)>0) Death_sequence_aborted = 1;

			//for (i=0; i<256; i++ )
			//	if (!key_isfunc(i) && !key_ismod(i) && key_down_count(i)>0) Death_sequence_aborted = 1;

			if (Death_sequence_aborted)
				game_flush_inputs();
		}
	} else {
		exploding_flag=0;
	}

	if (Newdemo_state == ND_STATE_PLAYBACK )
		update_vcr_state();

	while ((key=key_inkey_time(&key_time)) != 0)    {

		if (DefiningMarkerMessage)
		 {
			MarkerInputMessage (key);
			continue;
		 }

		#ifdef NETWORK
		if ( (Game_mode&GM_MULTI) && (multi_sending_message || multi_defining_message ))	{
			multi_message_input_sub( key );
			continue;		//get next key
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

#ifdef CONSOLE
		if(!con_events(key))
			continue;
#endif

		if (Player_is_dead)
			HandleDeathKey(key);

		if (Endlevel_sequence)
			HandleEndlevelKey(key);
		else if (Newdemo_state == ND_STATE_PLAYBACK ) {
			HandleDemoKey(key);

			#ifndef RELEASE
			HandleTestKey(key);
			#endif
		} else {
			FinalCheats(key);

			HandleSystemKey(key);
			HandleVRKey(key);
			HandleGameKey(key);

			#ifndef RELEASE
			HandleTestKey(key);
			#endif
		}
	}


//	if ((Players[Player_num].flags & PLAYER_FLAGS_CONVERTER) && keyd_pressed[KEY_F8] && (keyd_pressed[KEY_LALT] || keyd_pressed[KEY_RALT]))
  //		transfer_energy_to_shield(key_down_time(KEY_F8));
}

