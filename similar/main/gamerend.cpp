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
#include "dxxerror.h"
#include "gr.h"
#include "palette.h"
#include "bm.h"
#include "player.h"
#include "render.h"
#include "menu.h"
#include "newmenu.h"
#include "screens.h"
#include "config.h"
#include "maths.h"
#include "robot.h"
#include "game.h"
#include "gauges.h"
#include "gamefont.h"
#include "newdemo.h"
#include "text.h"
#include "multi.h"
#include "hudmsg.h"
#include "endlevel.h"
#include "cntrlcen.h"
#include "powerup.h"
#include "laser.h"
#include "playsave.h"
#include "automap.h"
#include "mission.h"
#include "gameseq.h"
#include "args.h"
#include "object.h"

#include "compiler-range_for.h"
#include "highest_valid.h"

#ifdef OGL
#include "ogl_init.h"
#endif

int netplayerinfo_on=0;

#if defined(DXX_BUILD_DESCENT_I)
static inline void game_draw_marker_message()
{
}
#elif defined(DXX_BUILD_DESCENT_II)
static void game_draw_marker_message()
{
	if ( DefiningMarkerMessage)
	{
		gr_set_curfont(GAME_FONT);
		gr_set_fontcolor(BM_XRGB(0,63,0),-1);
		gr_printf(0x8000, (LINE_SPACING*5)+FSPACY(1), "Marker: %s%c", &Marker_input[0], Marker_input[Marker_input.size() - 2] ? 0 : '_');
	}
}
#endif

static void game_draw_multi_message()
{
	if (!(Game_mode&GM_MULTI))
		return;
	if (multi_sending_message[Player_num])
	{
		gr_set_curfont(GAME_FONT);
		gr_set_fontcolor(BM_XRGB(0,63,0),-1);
		gr_printf(0x8000, (LINE_SPACING*5)+FSPACY(1), "%s: %s_", TXT_MESSAGE, Network_message.data());
	}
	else if (multi_defining_message)
	{
		gr_set_curfont(GAME_FONT);
		gr_set_fontcolor(BM_XRGB(0,63,0),-1);
		gr_printf(0x8000, (LINE_SPACING*5)+FSPACY(1), "%s #%d: %s_", TXT_MACRO, multi_defining_message, Network_message.data());
	}
}

static void show_framerate()
{
	static int fps_count = 0, fps_rate = 0;
	int y = GHEIGHT;
	static fix64 fps_time = 0;

	gr_set_curfont(GAME_FONT);
	gr_set_fontcolor(BM_XRGB(0,31,0),-1);

	const auto &&line_spacing = LINE_SPACING;
	if (PlayerCfg.CockpitMode[1] == CM_FULL_SCREEN) {
		if ((Game_mode & GM_MULTI) || (Newdemo_state == ND_STATE_PLAYBACK && Newdemo_game_mode & GM_MULTI))
			y -= line_spacing * 10;
		else
			y -= line_spacing * 4;
	} else if (PlayerCfg.CockpitMode[1] == CM_STATUS_BAR) {
		if ((Game_mode & GM_MULTI) || (Newdemo_state == ND_STATE_PLAYBACK && Newdemo_game_mode & GM_MULTI))
			y -= line_spacing * 6;
		else
			y -= line_spacing * 1;
	} else {
		if ((Game_mode & GM_MULTI) || (Newdemo_state == ND_STATE_PLAYBACK && Newdemo_game_mode & GM_MULTI))
			y -= line_spacing * 7;
		else
			y -= line_spacing * 2;
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

static void show_netplayerinfo()
{
	int x=0, y=0, eff=0;
	static const char *const eff_strings[]={"trashing","really hurting","seriously affecting","hurting","affecting","tarnishing"};

	gr_set_current_canvas(NULL);
	gr_set_curfont(GAME_FONT);
	gr_set_fontcolor(255,-1);

	const auto &&fspacx = FSPACX();
	const auto &&fspacx120 = fspacx(120);
	const auto &&fspacy84 = FSPACY(84);
	x = (SWIDTH / 2) - fspacx120;
	y = (SHEIGHT / 2) - fspacy84;

	gr_settransblend(14, GR_BLEND_NORMAL);
	gr_setcolor( BM_XRGB(0,0,0) );
	gr_rect((SWIDTH / 2) - fspacx120, (SHEIGHT / 2) - fspacy84, (SWIDTH / 2) + fspacx120, (SHEIGHT / 2) + fspacy84);
	gr_settransblend(GR_FADE_OFF, GR_BLEND_NORMAL);

	// general game information
	const auto &&line_spacing = LINE_SPACING;
	y += line_spacing;
	gr_string(0x8000,y,Netgame.game_name.data());
	y += line_spacing;
	gr_printf(0x8000, y, "%s - lvl: %i", Netgame.mission_title.data(), Netgame.levelnum);

	const auto &&fspacx8 = fspacx(8);
	x += fspacx8;
	y += line_spacing * 2;
	unsigned gamemode = Netgame.gamemode;
	gr_printf(x,y,"game mode: %s",gamemode < (sizeof(GMNames) / sizeof(GMNames[0])) ? GMNames[gamemode] : "INVALID");
	y += line_spacing;
	gr_printf(x,y,"difficulty: %s",MENU_DIFFICULTY_TEXT(Netgame.difficulty));
	y += line_spacing;
	gr_printf(x,y,"level time: %i:%02i:%02i",Players[Player_num].hours_level,f2i(Players[Player_num].time_level) / 60 % 60,f2i(Players[Player_num].time_level) % 60);
	y += line_spacing;
	gr_printf(x,y,"total time: %i:%02i:%02i",Players[Player_num].hours_total,f2i(Players[Player_num].time_total) / 60 % 60,f2i(Players[Player_num].time_total) % 60);
	y += line_spacing;
	if (Netgame.KillGoal)
		gr_printf(x,y,"Kill goal: %d",Netgame.KillGoal*5);

	// player information (name, kills, ping, game efficiency)
	y += line_spacing * 2;
	gr_string(x,y,"player");
	if (Game_mode & GM_MULTI_COOP)
		gr_string(x + fspacx8 * 7, y, "score");
	else
	{
		gr_string(x + fspacx8 * 7, y, "kills");
		gr_string(x + fspacx8 * 12, y, "deaths");
	}
	gr_string(x + fspacx8 * 18, y, "ping");
	gr_string(x + fspacx8 * 23, y, "efficiency");

	// process players table
	for (uint_fast32_t i = 0; i < MAX_PLAYERS; i++)
	{
		if (!Players[i].connected)
			continue;

		y += line_spacing;

		const auto color = get_player_or_team_color(i);
		gr_set_fontcolor( BM_XRGB(player_rgb[color].r,player_rgb[color].g,player_rgb[color].b),-1 );
		gr_printf(x,y,"%s\n",static_cast<const char *>(Players[i].callsign));
		if (Game_mode & GM_MULTI_COOP)
			gr_printf(x + fspacx8 * 7, y, "%-6d", Players[i].score);
		else
		{
			gr_printf(x + fspacx8 * 7, y,"%-6d", Players[i].net_kills_total);
			gr_printf(x + fspacx8 * 12, y,"%-6d", Players[i].net_killed_total);
		}

		gr_printf(x + fspacx8 * 18, y,"%-6d", Netgame.players[i].ping);
		if (i != Player_num)
			gr_printf(x + fspacx8 * 23, y, "%hu/%hu", kill_matrix[Player_num][i], kill_matrix[i][Player_num]);
	}

	y += (line_spacing * 2) + (line_spacing * (MAX_PLAYERS - N_players));

	// printf team scores
	if (Game_mode & GM_TEAM)
	{
		gr_set_fontcolor(255,-1);
		gr_string(x,y,"team");
		gr_string(x + fspacx8 * 8, y, "score");
		y += line_spacing;
		gr_set_fontcolor(BM_XRGB(player_rgb[0].r,player_rgb[0].g,player_rgb[0].b),-1 );
		gr_printf(x,y,"%s:",static_cast<const char *>(Netgame.team_name[0]));
		gr_printf(x + fspacx8 * 8, y, "%i", team_kills[0]);
		y += line_spacing;
		gr_set_fontcolor(BM_XRGB(player_rgb[1].r,player_rgb[1].g,player_rgb[1].b),-1 );
		gr_printf(x,y,"%s:",static_cast<const char *>(Netgame.team_name[1]));
		gr_printf(x + fspacx8 * 8, y, "%i", team_kills[1]);
		y += line_spacing * 2;
	}
	else
		y += line_spacing * 4;

	gr_set_fontcolor(255,-1);

	// additional information about game - hoard, ranking
	eff=(int)((float)((float)PlayerCfg.NetlifeKills/((float)PlayerCfg.NetlifeKilled+(float)PlayerCfg.NetlifeKills))*100.0);
	if (eff<0)
		eff=0;

#if defined(DXX_BUILD_DESCENT_II)
	if (game_mode_hoard())
	{
		if (PhallicMan==-1)
			gr_string(0x8000,y,"There is no record yet for this level.");
		else
			gr_printf(0x8000,y,"%s has the record at %d points.", static_cast<const char *>(Players[PhallicMan].callsign), PhallicLimit);
	}
	else
#endif
	if (!PlayerCfg.NoRankings)
	{
		gr_printf(0x8000,y,"Your lifetime efficiency of %d%% (%d/%d)",eff,PlayerCfg.NetlifeKills,PlayerCfg.NetlifeKilled);
		y += line_spacing;
		if (eff<60)
			gr_printf(0x8000,y,"is %s your ranking.",eff_strings[eff/10]);
		else
			gr_string(0x8000,y,"is serving you well.");
		y += line_spacing;
		gr_printf(0x8000,y,"your rank is: %s",RankStrings[GetMyNetRanking()]);
	}
}

#ifndef NDEBUG

fix Show_view_text_timer = -1;

static void draw_window_label()
{
	if ( Show_view_text_timer > 0 )
	{
		const char	*viewer_name,*control_name,*viewer_id;
		Show_view_text_timer -= FrameTime;

		viewer_id = "";
		switch( Viewer->type )
		{
			case OBJ_FIREBALL:	viewer_name = "Fireball"; break;
			case OBJ_ROBOT:		viewer_name = "Robot";
#ifdef EDITOR
										viewer_id = Robot_names[get_robot_id(Viewer)];
#endif
				break;
			case OBJ_HOSTAGE:		viewer_name = "Hostage"; break;
			case OBJ_PLAYER:		viewer_name = "Player"; break;
			case OBJ_WEAPON:		viewer_name = "Weapon"; break;
			case OBJ_CAMERA:		viewer_name = "Camera"; break;
			case OBJ_POWERUP:		viewer_name = "Powerup";
#ifdef EDITOR
										viewer_id = Powerup_names[get_powerup_id(Viewer)];
#endif
				break;
			case OBJ_DEBRIS:		viewer_name = "Debris"; break;
			case OBJ_CNTRLCEN:	viewer_name = "Reactor"; break;
			default:					viewer_name = "Unknown"; break;
		}

		switch ( Viewer->control_type) {
			case CT_NONE:			control_name = "Stopped"; break;
			case CT_AI:				control_name = "AI"; break;
			case CT_FLYING:		control_name = "Flying"; break;
			case CT_SLEW:			control_name = "Slew"; break;
			case CT_FLYTHROUGH:	control_name = "Flythrough"; break;
			case CT_MORPH:			control_name = "Morphing"; break;
			default:					control_name = "Unknown"; break;
		}

		gr_set_curfont(GAME_FONT);
		gr_set_fontcolor(BM_XRGB(31,0,0),-1);
		gr_printf(0x8000, (SHEIGHT/10), "%hu: %s [%s] View - %s", static_cast<objnum_t>(Viewer - Objects), viewer_name, viewer_id, control_name);

	}
}
#endif

static void render_countdown_gauge()
{
	if (!Endlevel_sequence && Control_center_destroyed  && (Countdown_seconds_left>-1)) { // && (Countdown_seconds_left<127))	{
#if defined(DXX_BUILD_DESCENT_II)
		if (!is_D2_OEM && !is_MAC_SHARE && !is_SHAREWARE)    // no countdown on registered only
		{
			//	On last level, we don't want a countdown.
			if (PLAYING_BUILTIN_MISSION && Current_level_num == Last_level)
			{
				if (!(Game_mode & GM_MULTI))
					return;
				if (Game_mode & GM_MULTI_ROBOTS)
					return;
			}
		}
#endif
		gr_set_curfont(GAME_FONT);
		gr_set_fontcolor(BM_XRGB(0,63,0),-1);
		gr_printf(0x8000, (LINE_SPACING*6)+FSPACY(1), "T-%d s", Countdown_seconds_left );
	}
}

static void game_draw_hud_stuff()
{
#ifndef NDEBUG
	draw_window_label();
#endif

	game_draw_multi_message();

	game_draw_marker_message();

	if (((Newdemo_state == ND_STATE_PLAYBACK) || (Newdemo_state == ND_STATE_RECORDING)) && (PlayerCfg.CockpitMode[1] != CM_REAR_VIEW)) {
		int y;

		gr_set_curfont( GAME_FONT );
		gr_set_fontcolor( BM_XRGB(27,0,0), -1 );

		y = GHEIGHT-(LINE_SPACING*2);

		if (PlayerCfg.CockpitMode[1] == CM_FULL_COCKPIT)
			y = grd_curcanv->cv_bitmap.bm_h / 1.2 ;
		if (Newdemo_state == ND_STATE_PLAYBACK) {
			if (Newdemo_show_percentage) {
				gr_printf(0x8000, y, "%s (%d%% %s)", TXT_DEMO_PLAYBACK, newdemo_get_percent_done(), TXT_DONE);
			}
		} else {
			gr_printf(0x8000, y, "%s (%dK)", TXT_DEMO_RECORDING, (Newdemo_num_written / 1024));
		}
	}

	render_countdown_gauge();

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

#if defined(DXX_BUILD_DESCENT_II)

ubyte RenderingType=0;
ubyte DemoDoingRight=0,DemoDoingLeft=0;

char DemoWBUType[]={0,WBU_GUIDED,WBU_MISSILE,WBU_REAR,WBU_ESCORT,WBU_MARKER,0};
char DemoRearCheck[]={0,0,0,1,0,0,0};
static const char DemoExtraMessage[][10] = {
	"PLAYER",
	"GUIDED",
	"MISSILE",
	"REAR",
	"GUIDE-BOT",
	"MARKER",
	"SHIP"
};

static const char *get_missile_name(const unsigned laser_type)
{
	switch(laser_type)
	{
		case CONCUSSION_ID:
			return "CONCUSSION";
		case HOMING_ID:
			return "HOMING";
		case SMART_ID:
			return "SMART";
		case MEGA_ID:
			return "MEGA";
		case FLASH_ID:
			return "FLASH";
		case MERCURY_ID:
			return "MERCURY";
		case EARTHSHAKER_ID:
			return "SHAKER";
		default:
			return "MISSILE";	// Bad!
	}
}

static void set_missile_viewer(vobjptridx_t o)
{
	Missile_viewer = o;
	Missile_viewer_sig = o->signature;
}

static void clear_missile_viewer()
{
	Missile_viewer = nullptr;
}

__attribute_warn_unused_result
static bool is_viewable_missile(weapon_type_t laser_type)
{
	return laser_type == CONCUSSION_ID ||
		laser_type == HOMING_ID ||
		laser_type == SMART_ID ||
		laser_type == MEGA_ID ||
		laser_type == FLASH_ID ||
		laser_type == GUIDEDMISS_ID ||
		laser_type == MERCURY_ID ||
		laser_type == EARTHSHAKER_ID;
}

static bool choose_missile_viewer()
{
	if (unlikely(!PlayerCfg.MissileViewEnabled))
		return false;
	const auto need_new_missile_viewer = []{
		if (!Missile_viewer)
			return true;
		if (Missile_viewer->type != OBJ_WEAPON)
			return true;
		if (Missile_viewer->signature != Missile_viewer_sig)
			return true;
		/* No check on parent here.  Missile_viewer is cleared if needed
		 * when a missile is fired.
		 */
		return false;
	};
	if (likely(!need_new_missile_viewer()))
		/* Valid viewer already set */
		return true;
	const auto better_match = [](cobjptridx_t a, vcobjptridx_t b) {
		if (a == object_none)
			return true;
		const vcobjptridx_t va{a};
		return va->lifeleft < b->lifeleft;
	};
	/* Find new missile */
	objptridx_t local_player_missile = object_none, other_player_missile = object_none;
	const auto game_mode = Game_mode;
	range_for (const auto i, highest_valid(Objects))
	{
		const auto o = vobjptridx(i);
		if (o->type != OBJ_WEAPON)
			continue;
		if (o->ctype.laser_info.parent_type != OBJ_PLAYER)
			continue;
		const auto laser_type = get_weapon_id(o);
		if (!is_viewable_missile(laser_type))
			continue;
		if (o->ctype.laser_info.parent_num == Players[Player_num].objnum)
		{
			if (!better_match(local_player_missile, o))
				continue;
			local_player_missile = o;
		}
		else
		{
			if (!better_match(other_player_missile, o))
				continue;
			if (game_mode & GM_MULTI_COOP)
			{
				/* Always allow missiles of other players */
			}
			else if (game_mode & GM_TEAM)
			{
				/* Allow missiles from same team */
				if (get_team(Player_num) != get_team(Objects[o->ctype.laser_info.parent_num].id))
					continue;
			}
			else
				continue;
			other_player_missile = o;
		}
	}
	if (local_player_missile != object_none)
		set_missile_viewer(local_player_missile);
	else if (other_player_missile != object_none)
		set_missile_viewer(other_player_missile);
	else
		return false;
	return true;
}

static void show_one_extra_view(const int w);
static void show_extra_views()
{
	int did_missile_view=0;
	int save_newdemo_state = Newdemo_state;
	if (Newdemo_state==ND_STATE_PLAYBACK)
	{
		if (DemoDoLeft)
		{
			DemoDoingLeft=DemoDoLeft;

			if (DemoDoLeft==3)
				do_cockpit_window_view(0,ConsoleObject,1,WBU_REAR,"REAR");
			else
				do_cockpit_window_view(0,&DemoLeftExtra,DemoRearCheck[DemoDoLeft],DemoWBUType[DemoDoLeft],DemoExtraMessage[DemoDoLeft]);
		}
		else
			do_cockpit_window_view(0,WBU_WEAPON);
	
		if (DemoDoRight)
		{
			DemoDoingRight=DemoDoRight;
			
			if (DemoDoRight==3)
				do_cockpit_window_view(1,ConsoleObject,1,WBU_REAR,"REAR");
			else
			{
				do_cockpit_window_view(1,&DemoRightExtra,DemoRearCheck[DemoDoRight],DemoWBUType[DemoDoRight],DemoExtraMessage[DemoDoRight]);
			}
		}
		else
			do_cockpit_window_view(1,WBU_WEAPON);
		
		DemoDoLeft=DemoDoRight=0;
		DemoDoingLeft=DemoDoingRight=0;
		return;
	}

	if (Guided_missile[Player_num] && Guided_missile[Player_num]->type==OBJ_WEAPON && Guided_missile[Player_num]->id==GUIDEDMISS_ID && Guided_missile[Player_num]->signature==Guided_missile_sig[Player_num]) 
	{
		if (PlayerCfg.GuidedInBigWindow)
		{
			RenderingType=6+(1<<4);
			do_cockpit_window_view(1,Viewer,0,WBU_MISSILE,"SHIP");
		}
		else
		{
			RenderingType=1+(1<<4);
			do_cockpit_window_view(1,Guided_missile[Player_num],0,WBU_GUIDED,"GUIDED");
		}
			
		did_missile_view=1;
	}
	else {

		if (Guided_missile[Player_num]) {		//used to be active
			if (!PlayerCfg.GuidedInBigWindow)
				do_cockpit_window_view(1,WBU_STATIC);
			Guided_missile[Player_num] = NULL;
		}
		if (choose_missile_viewer())
		//do missile view
			{
  				RenderingType=2+(1<<4);
				do_cockpit_window_view(1,Missile_viewer,0,WBU_MISSILE,get_missile_name(Missile_viewer->id));
				did_missile_view=1;
			}
			else {
				clear_missile_viewer();
				RenderingType=255;
				do_cockpit_window_view(1,WBU_STATIC);
			}
	}

	for (int w=0;w<2;w++) {

		if (w==1 && did_missile_view)
			continue;		//if showing missile view in right window, can't show anything else

		show_one_extra_view(w);
	}
	RenderingType=0;
	Newdemo_state = save_newdemo_state;
}

static void show_one_extra_view(const int w)
{
		//show special views if selected
		switch (PlayerCfg.Cockpit3DView[w]) {
			case CV_NONE:
				RenderingType=255;
				do_cockpit_window_view(w,WBU_WEAPON);
				break;
			case CV_REAR:
				if (Rear_view) {		//if big window is rear view, show front here
					RenderingType=3+(w<<4);				
					do_cockpit_window_view(w,ConsoleObject,0,WBU_REAR,"FRONT");
				}
				else {					//show normal rear view
					RenderingType=3+(w<<4);				
					do_cockpit_window_view(w,ConsoleObject,1,WBU_REAR,"REAR");
				}
			 	break;
			case CV_ESCORT: {
				auto buddy = find_escort();
				if (buddy == object_none) {
					do_cockpit_window_view(w,WBU_WEAPON);
					PlayerCfg.Cockpit3DView[w] = CV_NONE;
				}
				else {
					RenderingType=4+(w<<4);
					do_cockpit_window_view(w,buddy,0,WBU_ESCORT,PlayerCfg.GuidebotName);
				}
				break;
			}
			case CV_COOP: {
				int player = Coop_view_player[w];

	         RenderingType=255; // don't handle coop stuff			
				
				if (player!=-1 && Players[player].connected && ((Game_mode & GM_MULTI_COOP) || ((Game_mode & GM_TEAM) && (get_team(player) == get_team(Player_num)))))
					do_cockpit_window_view(w,&Objects[Players[Coop_view_player[w]].objnum],0,WBU_COOP,Players[Coop_view_player[w]].callsign);
				else {
					do_cockpit_window_view(w,WBU_WEAPON);
					PlayerCfg.Cockpit3DView[w] = CV_NONE;
				}
				break;
			}
			case CV_MARKER: {
				char label[10];
				RenderingType=5+(w<<4);
				if (Marker_viewer_num[w] == -1 || MarkerObject[Marker_viewer_num[w]] == object_none) {
					PlayerCfg.Cockpit3DView[w] = CV_NONE;
					break;
				}
				sprintf(label,"Marker %d",Marker_viewer_num[w]+1);
				do_cockpit_window_view(w,&Objects[MarkerObject[Marker_viewer_num[w]]],0,WBU_MARKER,label);
				break;
			}
			default:
				Int3();		//invalid window type
		}
}

int BigWindowSwitch=0;
#endif
static void update_cockpits();

//render a frame for the game
void game_render_frame_mono()
{
	int no_draw_hud=0;

	gr_set_current_canvas(&Screen_3d_window);
#if defined(DXX_BUILD_DESCENT_II)
	if (Guided_missile[Player_num] && Guided_missile[Player_num]->type==OBJ_WEAPON && Guided_missile[Player_num]->id==GUIDEDMISS_ID && Guided_missile[Player_num]->signature==Guided_missile_sig[Player_num] && PlayerCfg.GuidedInBigWindow) {
		object *viewer_save = Viewer;

		if (PlayerCfg.CockpitMode[1]==CM_FULL_COCKPIT || PlayerCfg.CockpitMode[1]==CM_REAR_VIEW)
		{
			 BigWindowSwitch=1;
			 force_cockpit_redraw=1;
			 PlayerCfg.CockpitMode[1]=CM_STATUS_BAR;
			 return;
		}

		Viewer = Guided_missile[Player_num];

		window_rendered_data window;
		update_rendered_data(window, Viewer, 0);
		render_frame(0, window);

		wake_up_rendered_objects(Viewer, window);
		show_HUD_names();

		Viewer = viewer_save;

		gr_set_curfont( GAME_FONT );
		gr_set_fontcolor( BM_XRGB(27,0,0), -1 );

		gr_string(0x8000, FSPACY(1), "Guided Missile View");

		show_reticle(RET_TYPE_CROSS_V1, 0);

		HUD_render_message_frame();

		no_draw_hud=1;
	}
	else
#endif
	{
#if defined(DXX_BUILD_DESCENT_II)
		if (BigWindowSwitch)
		{
			force_cockpit_redraw=1;
			PlayerCfg.CockpitMode[1]=(Rear_view?CM_REAR_VIEW:CM_FULL_COCKPIT);
			BigWindowSwitch=0;
			return;
		}
#endif
		window_rendered_data window;
#if defined(DXX_BUILD_DESCENT_II)
		update_rendered_data(window, Viewer, Rear_view);
#endif
		render_frame(0, window);
	}

#if defined(DXX_BUILD_DESCENT_II)
	gr_set_current_canvas(&Screen_3d_window);
#endif

	update_cockpits();

	if (Newdemo_state == ND_STATE_PLAYBACK)
		Game_mode = Newdemo_game_mode;

	if (PlayerCfg.CockpitMode[1]==CM_FULL_COCKPIT || PlayerCfg.CockpitMode[1]==CM_STATUS_BAR)
		render_gauges();

	if (Newdemo_state == ND_STATE_PLAYBACK)
		Game_mode = GM_NORMAL;

	gr_set_current_canvas(&Screen_3d_window);
	if (!no_draw_hud)
		game_draw_hud_stuff();

#if defined(DXX_BUILD_DESCENT_II)
	gr_set_current_canvas(NULL);

	show_extra_views();		//missile view, buddy bot, etc.
#endif

	if (netplayerinfo_on && Game_mode & GM_MULTI)
		show_netplayerinfo();
}

void toggle_cockpit()
{
	enum cockpit_mode_t new_mode=CM_FULL_SCREEN;

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
		case CM_REAR_VIEW:
		case CM_LETTERBOX:
			break;
	}

	select_cockpit(new_mode);
	HUD_clear_messages();
	PlayerCfg.CockpitMode[0] = new_mode;
	write_player_file();
}

int last_drawn_cockpit = -1;

// This actually renders the new cockpit onto the screen.
static void update_cockpits()
{
	grs_bitmap *bm;
	int mode = PlayerCfg.CockpitMode[1];
#if defined(DXX_BUILD_DESCENT_II)
	mode += (HIRESMODE?(Num_cockpits/2):0);
#endif

	switch( PlayerCfg.CockpitMode[1] )	{
		case CM_FULL_COCKPIT:
			PIGGY_PAGE_IN(cockpit_bitmap[mode]);
			bm=&GameBitmaps[cockpit_bitmap[mode].index];
			gr_set_current_canvas(NULL);
#ifdef OGL
			ogl_ubitmapm_cs (0, 0, -1, -1, *bm, 255, F1_0);
#else
			gr_ubitmapm(0,0, *bm);
#endif
			break;
		case CM_REAR_VIEW:
			PIGGY_PAGE_IN(cockpit_bitmap[mode]);
			bm=&GameBitmaps[cockpit_bitmap[mode].index];
			gr_set_current_canvas(NULL);
#ifdef OGL
			ogl_ubitmapm_cs (0, 0, -1, -1, *bm, 255, F1_0);
#else
			gr_ubitmapm(0,0, *bm);
#endif
			break;
	
		case CM_FULL_SCREEN:
			break;
	
		case CM_STATUS_BAR:
			PIGGY_PAGE_IN(cockpit_bitmap[mode]);
			bm=&GameBitmaps[cockpit_bitmap[mode].index];
			gr_set_current_canvas(NULL);
#ifdef OGL
			ogl_ubitmapm_cs (0, (HIRESMODE?(SHEIGHT*2)/2.6:(SHEIGHT*2)/2.72), -1, ((int) ((double) (bm->bm_h) * (HIRESMODE?(double)SHEIGHT/480:(double)SHEIGHT/200) + 0.5)), *bm,255, F1_0);
#else
			gr_ubitmapm(0,SHEIGHT-bm->bm_h,*bm);
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
	set_screen_mode( SCREEN_GAME );
	play_homing_warning();
	game_render_frame_mono();
}

//show a message in a nice little box
void show_boxed_message(const char *msg, int RenderFlag)
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
	
	gr_string( 0x8000, y, msg );
	
	// If we haven't drawn behind it, need to flip
	if (!RenderFlag)
		gr_flip();
}
