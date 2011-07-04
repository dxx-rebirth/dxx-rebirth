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
 * Stuff for rendering the HUD
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "timer.h"
#include "pstypes.h"
#include "console.h"
#include "inferno.h"
#include "error.h"
#include "gr.h"
#include "palette.h"
#include "bm.h"
#include "player.h"
#include "render.h"
#include "menu.h"
#include "newmenu.h"
#include "screens.h"
#include "fix.h"
#include "robot.h"
#include "game.h"
#include "gauges.h"
#include "gamefont.h"
#include "newdemo.h"
#include "text.h"
#include "multi.h"
#include "endlevel.h"
#include "cntrlcen.h"
#include "fuelcen.h"
#include "powerup.h"
#include "laser.h"
#include "playsave.h"
#include "automap.h"
#include "mission.h"
#include "gameseq.h"

#ifdef OGL
#include "ogl_init.h"
#endif

extern fix Cruise_speed;

int	netplayerinfo_on=0;

#ifdef NETWORK
void game_draw_multi_message()
{
	char temp_string[MAX_MULTI_MESSAGE_LEN+25];
	
	if ( (Game_mode&GM_MULTI) && (multi_sending_message[Player_num]))	{
		gr_set_curfont(GAME_FONT);
		gr_set_fontcolor(BM_XRGB(0,63,0),-1);
		sprintf( temp_string, "%s: %s_", TXT_MESSAGE, Network_message );
		gr_printf(0x8000, (LINE_SPACING*5)+FSPACY(1), temp_string );
	}
	
	if ( (Game_mode&GM_MULTI) && (multi_defining_message))	{
		gr_set_curfont(GAME_FONT);
		gr_set_fontcolor(BM_XRGB(0,63,0),-1);
		sprintf( temp_string, "%s #%d: %s_", TXT_MACRO, multi_defining_message, Network_message );
		gr_printf(0x8000, (LINE_SPACING*5)+FSPACY(1), temp_string );
	}
}
#endif

void show_framerate()
{
	static int fps_count = 0, fps_rate = 0;
	int y = GHEIGHT;
	static fix64 fps_time = 0;

	gr_set_curfont(GAME_FONT);
	gr_set_fontcolor(BM_XRGB(0,31,0),-1);

	if (PlayerCfg.CockpitMode[1] == CM_FULL_SCREEN) {
		if ((Game_mode & GM_MULTI) || (Newdemo_state == ND_STATE_PLAYBACK && Newdemo_game_mode & GM_MULTI))
			y -= LINE_SPACING * 10;
		else
			y -= LINE_SPACING * 4;
	} else if (PlayerCfg.CockpitMode[1] == CM_STATUS_BAR) {
		if ((Game_mode & GM_MULTI) || (Newdemo_state == ND_STATE_PLAYBACK && Newdemo_game_mode & GM_MULTI))
			y -= LINE_SPACING * 6;
		else
			y -= LINE_SPACING * 1;
	} else {
		if ((Game_mode & GM_MULTI) || (Newdemo_state == ND_STATE_PLAYBACK && Newdemo_game_mode & GM_MULTI))
			y -= LINE_SPACING * 7;
		else
			y -= LINE_SPACING * 2;
	}

	fps_count++;
	if (timer_query() >= fps_time + F1_0)
	{
		fps_rate = fps_count;
		fps_count = 0;
		fps_time = timer_query();
	}
	gr_printf(SWIDTH-(GameArg.SysMaxFPS>999?FSPACX(43):FSPACX(37)),y,"FPS: %i",fps_rate);
}

#ifdef NETWORK
void show_netplayerinfo()
{
	int x=0, y=0, i=0, color=0;
	
	gr_set_current_canvas(NULL);
	gr_set_curfont(GAME_FONT);
	gr_set_fontcolor(255,-1);
	
	x=(SWIDTH/2)-FSPACX(120);
	y=(SHEIGHT/2)-FSPACY(84);
	
	gr_settransblend(14, GR_BLEND_NORMAL);
	gr_setcolor( BM_XRGB(0,0,0) );
	gr_rect((SWIDTH/2)-FSPACX(120),(SHEIGHT/2)-FSPACY(84),(SWIDTH/2)+FSPACX(120),(SHEIGHT/2)+FSPACY(84));
	gr_settransblend(GR_FADE_OFF, GR_BLEND_NORMAL);
	
	// general game information
	y+=LINE_SPACING;
	gr_printf(0x8000,y,"%s",Netgame.game_name);
#ifndef SHAREWARE
	y+=LINE_SPACING;
	gr_printf(0x8000,y,"%s - lvl: %i",Netgame.mission_title,Netgame.levelnum);
#endif
	
	x+=FSPACX(8);
	y+=LINE_SPACING*2;
	gr_printf(x,y,"game mode: %s",GMNames[Netgame.gamemode]);
	y+=LINE_SPACING;
	gr_printf(x,y,"difficulty: %s",MENU_DIFFICULTY_TEXT(Netgame.difficulty));
	y+=LINE_SPACING;
	gr_printf(x,y,"level time: %i:%02i:%02i",Players[Player_num].hours_level,f2i(Players[Player_num].time_level) / 60 % 60,f2i(Players[Player_num].time_level) % 60);
	y+=LINE_SPACING;
	gr_printf(x,y,"total time: %i:%02i:%02i",Players[Player_num].hours_total,f2i(Players[Player_num].time_total) / 60 % 60,f2i(Players[Player_num].time_total) % 60);
	y+=LINE_SPACING;
	if (Netgame.KillGoal)
		gr_printf(x,y,"Kill goal: %d",Netgame.KillGoal*5);
	
	// player information (name, kills, ping, game efficiency)
	y+=LINE_SPACING*2;
	gr_printf(x,y,"player");
	if (Game_mode & GM_MULTI_COOP)
		gr_printf(x+FSPACX(8)*7,y,"score");
	else
	{
		gr_printf(x+FSPACX(8)*7,y,"kills");
		gr_printf(x+FSPACX(8)*12,y,"deaths");
	}
	gr_printf(x+FSPACX(8)*18,y,"ping");
	gr_printf(x+FSPACX(8)*23,y,"efficiency");
	
	// process players table
	for (i=0; i<MAX_PLAYERS; i++)
	{
		if (!Players[i].connected)
			continue;
		
		y+=LINE_SPACING;
		
		if (Game_mode & GM_TEAM)
			color=get_team(i);
		else
			color=i;
		gr_set_fontcolor(BM_XRGB(player_rgb[color].r,player_rgb[color].g,player_rgb[color].b),-1 );
		gr_printf(x,y,"%s\n",Players[i].callsign);
		if (Game_mode & GM_MULTI_COOP)
			gr_printf(x+FSPACX(8)*7,y,"%-6d",Players[i].score);
		else
		{
			gr_printf(x+FSPACX(8)*7,y,"%-6d",Players[i].net_kills_total);
			gr_printf(x+FSPACX(8)*12,y,"%-6d",Players[i].net_killed_total);
		}
		
		gr_printf(x+FSPACX(8)*18,y,"%-6d",Netgame.players[i].ping);
		if (i != Player_num)
			gr_printf(x+FSPACX(8)*23,y,"%d/%d",kill_matrix[Player_num][i],kill_matrix[i][Player_num]);
	}
	
	y+=LINE_SPACING*2+(LINE_SPACING*(MAX_PLAYERS-N_players));
	
	// printf team scores
	if (Game_mode & GM_TEAM)
	{
		gr_set_fontcolor(255,-1);
		gr_printf(x,y,"team");
		gr_printf(x+FSPACX(8)*8,y,"score");
		y+=LINE_SPACING;
		gr_set_fontcolor(BM_XRGB(player_rgb[get_team(0)].r,player_rgb[get_team(0)].g,player_rgb[get_team(0)].b),-1 );
		gr_printf(x,y,"%s:",Netgame.team_name[0]);
		gr_printf(x+FSPACX(8)*8,y,"%i",team_kills[0]);
		y+=LINE_SPACING;
		gr_set_fontcolor(BM_XRGB(player_rgb[get_team(1)].r,player_rgb[get_team(1)].g,player_rgb[get_team(1)].b),-1 );
		gr_printf(x,y,"%s:",Netgame.team_name[1]);
		gr_printf(x+FSPACX(8)*8,y,"%i",team_kills[1]);
		y+=LINE_SPACING*2;
	}
}
#endif

#ifndef NDEBUG

fix Show_view_text_timer = -1;

void draw_window_label()
{
	if ( Show_view_text_timer > 0 )
	{
		char *viewer_name,*control_name;
		
		Show_view_text_timer -= FrameTime;
		
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
		gr_set_curfont( GAME_FONT );
		gr_set_fontcolor( BM_XRGB(31, 0, 0), -1 );
		gr_printf( 0x8000, (SHEIGHT/10), "%s View - %s",viewer_name,control_name );
	}
}
#endif

void render_countdown_gauge()
{
	if (!Endlevel_sequence && Control_center_destroyed  && (Countdown_seconds_left>-1) && (Countdown_seconds_left<127))	{
		gr_set_curfont(GAME_FONT);
		gr_set_fontcolor(BM_XRGB(0,63,0),-1);
		gr_printf(0x8000, (LINE_SPACING*6)+FSPACY(1), "T-%d s", Countdown_seconds_left );
	}
}

void game_draw_hud_stuff()
{
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
		int y;
		
		if (Newdemo_state == ND_STATE_PLAYBACK) {
			if (Newdemo_show_percentage) {
			  	sprintf(message, "%s (%d%%%% %s)", TXT_DEMO_PLAYBACK, newdemo_get_percent_done(), TXT_DONE);
			} else {
				sprintf (message, " ");
			}
		} else {
			extern int Newdemo_num_written;
			sprintf (message, "%s (%dK)", TXT_DEMO_RECORDING, (Newdemo_num_written / 1024));
		}
		
		gr_set_curfont( GAME_FONT );
		gr_set_fontcolor( BM_XRGB(27,0,0), -1 );
		
		y = GHEIGHT-(LINE_SPACING*2);
		
		if (PlayerCfg.CockpitMode[1] == CM_FULL_COCKPIT)
			y = grd_curcanv->cv_bitmap.bm_h / 1.2 ;
		if (PlayerCfg.CockpitMode[1] != CM_REAR_VIEW)
			gr_printf(0x8000, y, message );
	}
	
	render_countdown_gauge();
	
	// this should be made part of hud code some day
	if ( Player_num > -1 && Viewer->type==OBJ_PLAYER && Viewer->id==Player_num && PlayerCfg.CockpitMode[1] != CM_REAR_VIEW)	{
		int	x = FSPACX(1);
		int	y = grd_curcanv->cv_bitmap.bm_h;
		
		gr_set_curfont( GAME_FONT );
		gr_set_fontcolor( BM_XRGB(0, 31, 0), -1 );
		if (Cruise_speed > 0) {
			if (PlayerCfg.CockpitMode[1]==CM_FULL_SCREEN) {
				if (Game_mode & GM_MULTI)
					y -= LINE_SPACING * 10;
				else
					y -= LINE_SPACING * 6;
			} else if (PlayerCfg.CockpitMode[1] == CM_STATUS_BAR) {
				if (Game_mode & GM_MULTI)
					y -= LINE_SPACING * 6;
				else
					y -= LINE_SPACING * 1;
			} else {
				if (Game_mode & GM_MULTI)
					y -= LINE_SPACING * 7;
				else
					y -= LINE_SPACING * 2;
			}
			
			gr_printf( x, y, "%s %2d%%", TXT_CRUISE, f2i(Cruise_speed) );
		}
	}
	
	if (GameCfg.FPSIndicator && PlayerCfg.CockpitMode[1] != CM_REAR_VIEW)
		show_framerate();

	if (Newdemo_state == ND_STATE_PLAYBACK)
		Game_mode = Newdemo_game_mode;

	draw_hud();

	if (Newdemo_state == ND_STATE_PLAYBACK)
		Game_mode = GM_NORMAL;
	
	if ( Player_is_dead )
		player_dead_message();
}

extern int gr_bitblt_dest_step_shift;
extern int gr_bitblt_double;
extern int force_cockpit_redraw;
void update_cockpits();

//render a frame for the game
void game_render_frame_mono(int flip)
{
	gr_set_current_canvas(&Screen_3d_window);
	
	render_frame(0);
	
	update_cockpits();

	if (Newdemo_state == ND_STATE_PLAYBACK)
		Game_mode = Newdemo_game_mode;
	
	if (PlayerCfg.CockpitMode[1]==CM_FULL_COCKPIT || PlayerCfg.CockpitMode[1]==CM_STATUS_BAR)
		render_gauges();

	if (Newdemo_state == ND_STATE_PLAYBACK)
		Game_mode = GM_NORMAL;
	
	gr_set_current_canvas(&Screen_3d_window);
	game_draw_hud_stuff();
	
#ifdef NETWORK
	if (netplayerinfo_on && Game_mode & GM_MULTI)
		show_netplayerinfo();
#endif
}

void toggle_cockpit()
{
	int new_mode=CM_FULL_SCREEN;

	if (Rear_view || Player_is_dead)
		return;

	switch (PlayerCfg.CockpitMode[1])
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
	PlayerCfg.CockpitMode[0] = new_mode;
	write_player_file();
}

int last_drawn_cockpit = -1;
extern void ogl_loadbmtexture(grs_bitmap *bm);

// This actually renders the new cockpit onto the screen.
void update_cockpits()
{
	grs_bitmap * bm;
	PIGGY_PAGE_IN(cockpit_bitmap[PlayerCfg.CockpitMode[1]]);
	bm = &GameBitmaps[cockpit_bitmap[PlayerCfg.CockpitMode[1]].index];
	
	//Redraw the on-screen cockpit bitmaps
	if (VR_render_mode != VR_NONE )	return;
	
	switch( PlayerCfg.CockpitMode[1] )	{
		case CM_FULL_COCKPIT:
			gr_set_current_canvas(NULL);
#ifdef OGL
			ogl_ubitmapm_cs (0, 0, -1, grd_curcanv->cv_bitmap.bm_h, bm,255, F1_0);
#else
			gr_ubitmapm(0,0, bm);
#endif
			break;
		case CM_REAR_VIEW:
			gr_set_current_canvas(NULL);
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
			ogl_ubitmapm_cs (0, (HIRESMODE?(SHEIGHT*2)/2.6:(SHEIGHT*2)/2.72), -1, ((int) ((double) (bm->bm_h) * (HIRESMODE?(double)SHEIGHT/480:(double)SHEIGHT/200) + 0.5)), bm,255, F1_0);
#else
			gr_ubitmapm(0,SHEIGHT-bm->bm_h,bm);
#endif
			break;
		case CM_LETTERBOX:
			gr_set_current_canvas(NULL);
			break;
	}
	
	gr_set_current_canvas(NULL);
	
	if (PlayerCfg.CockpitMode[1] != last_drawn_cockpit)
		last_drawn_cockpit = PlayerCfg.CockpitMode[1];
	else
		return;
	
	if (PlayerCfg.CockpitMode[1]==CM_FULL_COCKPIT || PlayerCfg.CockpitMode[1]==CM_STATUS_BAR)
		init_gauges();
}

void game_render_frame()
{
	set_screen_mode(SCREEN_GAME);
	play_homing_warning();
	game_render_frame_mono(GameArg.DbgUseDoubleBuffer);
	FrameCount++;
}

//show a message in a nice little box
void show_boxed_message(char *msg, int RenderFlag)
{
	int w,h,aw;
	int x,y;
	
	gr_set_current_canvas(NULL);
	gr_set_curfont( MEDIUM1_FONT );
	gr_set_fontcolor(BM_XRGB(31, 31, 31), -1);
	gr_get_string_size(msg,&w,&h,&aw);
	
	x = (SWIDTH-w)/2;
	y = (SHEIGHT-h)/2;
	
	nm_draw_background(x-BORDERX,y-BORDERY,x+w+BORDERX,y+h+BORDERY);
	
	gr_printf( 0x8000, y, msg );
	
	// If we haven't drawn behind it, need to flip
	if (!RenderFlag)
		gr_flip();
}

