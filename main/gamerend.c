/* $Id: gamerend.c,v 1.1.1.1 2006/03/17 19:57:07 zicodxx Exp $ */
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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: gamerend.c,v 1.1.1.1 2006/03/17 19:57:07 zicodxx Exp $";
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "pstypes.h"
#include "console.h"
#include "inferno.h"
#include "error.h"
#include "mono.h"
#include "gr.h"
#include "palette.h"
#include "ibitblt.h"
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
#include "powerup.h"
#include "laser.h"
#include "playsave.h"
#include "automap.h"
#include "mission.h"
#include "gameseq.h"

#ifdef NETWORK
#include "network.h"
#endif

#ifdef OGL
#include "ogl_init.h"
#endif

extern fix Cruise_speed;
extern int LinearSVGABuffer;


#ifndef NDEBUG
extern int Debug_pause;				//John's debugging pause system
#endif

#ifndef RELEASE
extern int Saving_movie_frames;
#else
#define Saving_movie_frames 0
#endif

extern void newmenu_close();
int netplayerinfo_on=0;

extern ubyte DefiningMarkerMessage;
extern char Marker_input[];

#define MAX_MARKER_MESSAGE_LEN 120
void game_draw_marker_message()
{
	char temp_string[MAX_MARKER_MESSAGE_LEN+25];

	if ( DefiningMarkerMessage)
	{
		gr_set_curfont(GAME_FONT);
		gr_set_fontcolor(BM_XRGB(0,63,0),-1);
		sprintf( temp_string, "Marker: %s_", Marker_input );
		gr_printf(0x8000, (SHEIGHT/5.555), temp_string );
	}

}

#ifdef NETWORK
void game_draw_multi_message()
{
	char temp_string[MAX_MULTI_MESSAGE_LEN+25];

	if ( (Game_mode&GM_MULTI) && (multi_sending_message))	{
		gr_set_curfont(GAME_FONT);
		gr_set_fontcolor(BM_XRGB(0,63,0),-1);
		sprintf( temp_string, "%s: %s_", TXT_MESSAGE, Network_message );
		gr_printf(0x8000, (SHEIGHT/5.555), temp_string );
	}

	if ( (Game_mode&GM_MULTI) && (multi_defining_message))	{
		gr_set_curfont(GAME_FONT);
		gr_set_fontcolor(BM_XRGB(0,63,0),-1);
		sprintf( temp_string, "%s #%d: %s_", TXT_MACRO, multi_defining_message, Network_message );
		gr_printf(0x8000, (SHEIGHT/5.555), temp_string );
	}
}
#endif

fix frame_time_list[8] = {0,0,0,0,0,0,0,0};
fix frame_time_total=0;
int frame_time_cntr=0;

void ftoa(char *string, fix f)
{
	int decimal, fractional;
	
	decimal = f2i(f);
	fractional = ((f & 0xffff)*100)/65536;
	if (fractional < 0 )
		fractional *= -1;
	if (fractional > 99 ) fractional = 99;
	sprintf( string, "FPS: %d.%02d", decimal, fractional );
}

void show_framerate()
{
	char temp[13];
	fix rate;
	int aw,w,h;

	frame_time_total += FrameTime - frame_time_list[frame_time_cntr];
	frame_time_list[frame_time_cntr] = FrameTime;
	frame_time_cntr = (frame_time_cntr+1)%8;

	if (frame_time_total) {
		int y=GHEIGHT;
		if (Cockpit_mode==CM_FULL_SCREEN) {
			if (Game_mode & GM_MULTI)
				y -= LINE_SPACING * 10;
			else
				y -= LINE_SPACING * 5;
		} else if (Cockpit_mode == CM_STATUS_BAR) {
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

		rate = fixdiv(f1_0*8,frame_time_total);

		gr_set_curfont(GAME_FONT);
		gr_set_fontcolor(BM_XRGB(0,31,0),-1);

		ftoa( temp, rate );
		gr_get_string_size(temp,&w,&h,&aw);
		gr_printf(SWIDTH-w-FSPACX(1),y,"%s", temp );
	}
}

#ifdef NETWORK
void show_netplayerinfo()
{
	int x=0, y=0, i=0, color=0, eff=0;
	char *eff_strings[]={"trashing","really hurting","seriously effecting","hurting","effecting","tarnishing"};
	char *NetworkModeNames[]={"Anarchy","Team Anarchy","Robo Anarchy","Cooperative","Capture the Flag","Hoard","Team Hoard","Unknown"};

	gr_set_current_canvas(NULL);
	gr_set_curfont(GAME_FONT);
	gr_set_fontcolor(255,-1);

	x=(SWIDTH/2)-FSPACX(120);
	y=(SHEIGHT/2)-FSPACY(84);

	Gr_scanline_darkening_level = 2*7;
	gr_setcolor( BM_XRGB(0,0,0) );
	gr_rect((SWIDTH/2)-FSPACX(120),(SHEIGHT/2)-FSPACY(84),(SWIDTH/2)+FSPACX(120),(SHEIGHT/2)+FSPACY(84));
	Gr_scanline_darkening_level = GR_FADE_LEVELS;

	// general game information
	y+=LINE_SPACING;
	gr_printf(0x8000,y,"%s by %s",Netgame.game_name,Players[network_who_is_master()].callsign);
	y+=LINE_SPACING;
	gr_printf(0x8000,y,"%s - lvl: %i",Netgame.mission_title,Netgame.levelnum);

	x+=FSPACX(8);
	y+=LINE_SPACING*2;
	gr_printf(x,y,"game mode: %s",NetworkModeNames[Netgame.gamemode]);
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

	network_ping_all();

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
		gr_set_fontcolor( BM_XRGB(player_rgb[color].r,player_rgb[color].g,player_rgb[color].b),-1 );
		gr_printf(x,y,"%s\n",Players[i].callsign);
		if (Game_mode & GM_MULTI_COOP)
			gr_printf(x+FSPACX(8)*7,y,"%-6d",Players[i].score);
		else
		{
			gr_printf(x+FSPACX(8)*7,y,"%-6d",Players[i].net_kills_total);
			gr_printf(x+FSPACX(8)*12,y,"%-6d",Players[i].net_killed_total);
		}

		gr_printf(x+FSPACX(8)*18,y,"%-6d",PingTable[i]);
		if (i != Player_num)
			gr_printf(x+FSPACX(8)*23,y,"%d/%d",kill_matrix[Player_num][i],kill_matrix[i][Player_num]);
	}

	y+=(LINE_SPACING*2)+(LINE_SPACING*(MAX_PLAYERS-N_players));

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
	else
		y+=LINE_SPACING*4;

	gr_set_fontcolor(255,-1);

	// additional information about game - hoard, ranking
	eff=(int)((float)((float)Netlife_kills/((float)Netlife_killed+(float)Netlife_kills))*100.0);
	if (eff<0)
		eff=0;

	if (Game_mode & GM_HOARD)
	{
		if (PhallicMan==-1)
			gr_printf(0x8000,y,"There is no record yet for this level."); 
		else
			gr_printf(0x8000,y,"%s has the record at %d points.",Players[PhallicMan].callsign,PhallicLimit);
	}
	else if (!GameArg.MplNoRankings)
	{
		gr_printf(0x8000,y,"Your lifetime efficiency of %d%% (%d/%d)",eff,Netlife_kills,Netlife_killed);
		y+=LINE_SPACING;
		if (eff<60)
			gr_printf(0x8000,y,"is %s your ranking.",eff_strings[eff/10]);
		else
			gr_printf(0x8000,y,"is serving you well.");
		y+=LINE_SPACING;
		gr_printf(0x8000,y,"your rank is: %s",RankStrings[GetMyNetRanking()]);
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
		char	*viewer_id;
		Show_view_text_timer -= FrameTime;

		viewer_id = "";
		switch( Viewer->type )
		{
			case OBJ_FIREBALL:	viewer_name = "Fireball"; break;
			case OBJ_ROBOT:		viewer_name = "Robot";
#ifdef EDITOR
										viewer_id = Robot_names[Viewer->id];
#endif
				break;
			case OBJ_HOSTAGE:		viewer_name = "Hostage"; break;
			case OBJ_PLAYER:		viewer_name = "Player"; break;
			case OBJ_WEAPON:		viewer_name = "Weapon"; break;
			case OBJ_CAMERA:		viewer_name = "Camera"; break;
			case OBJ_POWERUP:		viewer_name = "Powerup";
#ifdef EDITOR
										viewer_id = Powerup_names[Viewer->id];
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
		gr_printf( 0x8000, (SHEIGHT/10), "%i: %s [%s] View - %s",Viewer-Objects, viewer_name, viewer_id, control_name );

	}
}
#endif

void render_countdown_gauge()
{
	if (!Endlevel_sequence && Control_center_destroyed  && (Countdown_seconds_left>-1)) { // && (Countdown_seconds_left<127))	{

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

		gr_set_curfont(GAME_FONT);
		gr_set_fontcolor(BM_XRGB(0,63,0),-1);
		gr_printf(0x8000, (SHEIGHT/6.666), "T-%d s", Countdown_seconds_left );
	}
}

void game_draw_hud_stuff()
{
	//mprintf ((0,"Linear is %d!\n",LinearSVGABuffer));
	
	#ifndef NDEBUG
	if (Debug_pause) {
		gr_set_curfont( MEDIUM1_FONT);
		gr_set_fontcolor(BM_XRGB(31, 31, 31), -1 );
		gr_printf( 0x8000, (SHEIGHT/10), "Debug Pause - Press P to exit" );
	}
	#endif

	#ifndef NDEBUG
	draw_window_label();
	#endif

#ifdef NETWORK
	game_draw_multi_message();
#endif

	game_draw_marker_message();

	if ((Newdemo_state == ND_STATE_PLAYBACK) || (Newdemo_state == ND_STATE_RECORDING)) {
		char message[128];
		int y;

		if (Newdemo_state == ND_STATE_PLAYBACK) {
			if (Newdemo_vcr_state != ND_STATE_PRINTSCREEN) {
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

		if (Cockpit_mode == CM_FULL_COCKPIT)
			y = grd_curcanv->cv_bitmap.bm_h / 1.2 ;
		if (Cockpit_mode != CM_REAR_VIEW)
			gr_printf(0x8000, y, message );
	}

	render_countdown_gauge();

	// this should be made part of hud code some day
	if ( Player_num > -1 && Viewer->type==OBJ_PLAYER && Viewer->id==Player_num && Cockpit_mode != CM_REAR_VIEW)	{
		int	x = FSPACX(1);
		int	y = grd_curcanv->cv_bitmap.bm_h;

		gr_set_curfont( GAME_FONT );
		gr_set_fontcolor( BM_XRGB(0, 31, 0), -1 );
		if (Cruise_speed > 0) {
			if (Cockpit_mode==CM_FULL_SCREEN) {
				if (Game_mode & GM_MULTI)
					y -= LINE_SPACING * 10;
				else
					y -= LINE_SPACING * 5;
			} else if (Cockpit_mode == CM_STATUS_BAR) {
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

	if (GameArg.SysFPSIndicator && Cockpit_mode != CM_REAR_VIEW)
		show_framerate();

	draw_hud();

	if ( Player_is_dead )
		player_dead_message();
}

extern int gr_bitblt_dest_step_shift;
extern int gr_bitblt_double;

#if 0
void expand_row(ubyte * dest, ubyte * src, int num_src_pixels );
#pragma aux expand_row parm [edi] [esi] [ecx] modify exact [ecx esi edi eax ebx] = \
	"add	esi, ecx"			\
	"dec	esi"					\
	"add	edi, ecx"			\
	"add	edi, ecx"			\
	"dec	edi"					\
	"dec	edi"					\
"nextpixel:"					\
	"mov	al,[esi]"			\
	"mov	ah, al"				\
	"dec	esi"					\
	"mov	[edi], ax"			\
	"dec	edi"					\
	"dec	edi"					\
	"dec	ecx"					\
	"jnz	nextpixel"			\
"done:"
#else
void expand_row(ubyte * dest, ubyte * src, int num_src_pixels )
{
	int i;
	
	for (i = 0; i < num_src_pixels; i++) {
		*dest++ = *src;
		*dest++ = *src++;
	}
}
#endif

// doubles the size in x or y of a bitmap in place.
void game_expand_bitmap( grs_bitmap * bmp, uint flags )
{
	int i;
	ubyte * dptr, * sptr;

	switch(flags & 3)	{
	case 2:	// expand x
		Assert( bmp->bm_rowsize == bmp->bm_w*2 );
		dptr = &bmp->bm_data[(bmp->bm_h-1)*bmp->bm_rowsize];
		for (i=bmp->bm_h-1; i>=0; i-- )	{
			expand_row( dptr, dptr, bmp->bm_w );	
			dptr -= bmp->bm_rowsize;
		}
		bmp->bm_w *= 2;
		break;
	case 1:	// expand y
		dptr = &bmp->bm_data[(2*(bmp->bm_h-1)+1)*bmp->bm_rowsize];
		sptr = &bmp->bm_data[(bmp->bm_h-1)*bmp->bm_rowsize];
		for (i=bmp->bm_h-1; i>=0; i-- )	{
			memcpy( dptr, sptr, bmp->bm_w );	
			dptr -= bmp->bm_rowsize;
			memcpy( dptr, sptr, bmp->bm_w );	
			dptr -= bmp->bm_rowsize;
			sptr -= bmp->bm_rowsize;
		}
		bmp->bm_h *= 2;
		break;
	case 3:	// expand x & y
		Assert( bmp->bm_rowsize == bmp->bm_w*2 );
		dptr = &bmp->bm_data[(2*(bmp->bm_h-1)+1)*bmp->bm_rowsize];
		sptr = &bmp->bm_data[(bmp->bm_h-1)*bmp->bm_rowsize];
		for (i=bmp->bm_h-1; i>=0; i-- )	{
			expand_row( dptr, sptr, bmp->bm_w );	
			dptr -= bmp->bm_rowsize;
			expand_row( dptr, sptr, bmp->bm_w );	
			dptr -= bmp->bm_rowsize;
			sptr -= bmp->bm_rowsize;
		}
		bmp->bm_w *= 2;
		bmp->bm_h *= 2;
		break;
	}
}

extern int SW_drawn[2], SW_x[2], SW_y[2], SW_w[2], SW_h[2];
extern int Guided_in_big_window;
ubyte RenderingType=0;
ubyte DemoDoingRight=0,DemoDoingLeft=0;
extern ubyte DemoDoRight,DemoDoLeft;
extern object DemoRightExtra,DemoLeftExtra;

char DemoWBUType[]={0,WBU_GUIDED,WBU_MISSILE,WBU_REAR,WBU_ESCORT,WBU_MARKER,0};
char DemoRearCheck[]={0,0,0,1,0,0,0};
char *DemoExtraMessage[]={"PLAYER","GUIDED","MISSILE","REAR","GUIDE-BOT","MARKER","SHIP"};

extern char guidebot_name[];

void show_extra_views()
{
	int did_missile_view=0;
	int save_newdemo_state = Newdemo_state;
	int w;

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
			do_cockpit_window_view(0,NULL,0,WBU_WEAPON,NULL);
	
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
			do_cockpit_window_view(1,NULL,0,WBU_WEAPON,NULL);
		
		DemoDoLeft=DemoDoRight=0;
		DemoDoingLeft=DemoDoingRight=0;
		return;
	}

	if (Guided_missile[Player_num] && Guided_missile[Player_num]->type==OBJ_WEAPON && Guided_missile[Player_num]->id==GUIDEDMISS_ID && Guided_missile[Player_num]->signature==Guided_missile_sig[Player_num]) 
	{
		if (Guided_in_big_window)
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
			if (!Guided_in_big_window)
				do_cockpit_window_view(1,NULL,0,WBU_STATIC,NULL);
			Guided_missile[Player_num] = NULL;
		}

		if (Missile_viewer) //do missile view
		{
			static int mv_sig=-1;
			if (mv_sig == -1)
				mv_sig = Missile_viewer->signature;
			if (Missile_view_enabled && Missile_viewer->type!=OBJ_NONE && Missile_viewer->signature == mv_sig) {
  				RenderingType=2+(1<<4);
				do_cockpit_window_view(1,Missile_viewer,0,WBU_MISSILE,"MISSILE");
				did_missile_view=1;
			}
			else {
				Missile_viewer = NULL;
				mv_sig = -1;
				RenderingType=255;
				do_cockpit_window_view(1,NULL,0,WBU_STATIC,NULL);
			}
		}
	}

	for (w=0;w<2;w++) {

		if (w==1 && did_missile_view)
			continue;		//if showing missile view in right window, can't show anything else

		//show special views if selected
		switch (Cockpit_3d_view[w]) {
			case CV_NONE:
				RenderingType=255;
				do_cockpit_window_view(w,NULL,0,WBU_WEAPON,NULL);
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
				object *buddy;
				buddy = find_escort();
				if (buddy == NULL) {
					do_cockpit_window_view(w,NULL,0,WBU_WEAPON,NULL);
					Cockpit_3d_view[w] = CV_NONE;
				}
				else {
					RenderingType=4+(w<<4);
					do_cockpit_window_view(w,buddy,0,WBU_ESCORT,guidebot_name);
				}
				break;
			}
#ifdef NETWORK
			case CV_COOP: {
				int player = Coop_view_player[w];

	         RenderingType=255; // don't handle coop stuff			
				
				if (player!=-1 && Players[player].connected && ((Game_mode & GM_MULTI_COOP) || ((Game_mode & GM_TEAM) && (get_team(player) == get_team(Player_num)))))
					do_cockpit_window_view(w,&Objects[Players[Coop_view_player[w]].objnum],0,WBU_COOP,Players[Coop_view_player[w]].callsign);
				else {
					do_cockpit_window_view(w,NULL,0,WBU_WEAPON,NULL);
					Cockpit_3d_view[w] = CV_NONE;
				}
				break;
			}
#endif
			case CV_MARKER: {
				char label[10];
				RenderingType=5+(w<<4);
				if (Marker_viewer_num[w] == -1 || MarkerObject[Marker_viewer_num[w]] == -1) {
					Cockpit_3d_view[w] = CV_NONE;
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
	RenderingType=0;
	Newdemo_state = save_newdemo_state;
}

int BigWindowSwitch=0;
extern int force_cockpit_redraw;
void draw_guided_crosshair(void);
void update_cockpits(int force_redraw);


//render a frame for the game
void game_render_frame_mono(int flip)
{
	int no_draw_hud=0;
	
	gr_set_current_canvas(&Screen_3d_window);
	
	if (Guided_missile[Player_num] && Guided_missile[Player_num]->type==OBJ_WEAPON && Guided_missile[Player_num]->id==GUIDEDMISS_ID && Guided_missile[Player_num]->signature==Guided_missile_sig[Player_num] && Guided_in_big_window) {
		object *viewer_save = Viewer;

		if (Cockpit_mode==CM_FULL_COCKPIT)
		{
			 BigWindowSwitch=1;
			 force_cockpit_redraw=1;
			 Cockpit_mode=CM_STATUS_BAR;
			 return;
		}

		Viewer = Guided_missile[Player_num];

		update_rendered_data(0, Viewer, 0, 0);
		render_frame(0, 0);

		wake_up_rendered_objects(Viewer, 0);
		Viewer = viewer_save;

		gr_set_curfont( GAME_FONT );
		gr_set_fontcolor( BM_XRGB(27,0,0), -1 );

		gr_printf(0x8000, FSPACY(1), "Guided Missile View");

		draw_guided_crosshair();

		HUD_render_message_frame();

		no_draw_hud=1;
	}
	else
	{
		if (BigWindowSwitch)
		{
			force_cockpit_redraw=1;
			Cockpit_mode=CM_FULL_COCKPIT;
			BigWindowSwitch=0;
			return;
		}
		update_rendered_data(0, Viewer, Rear_view, 0);
		render_frame(0, 0);
	}

	gr_set_current_canvas(&Screen_3d_window);

	update_cockpits(0);

	show_extra_views();		//missile view, buddy bot, etc.

	if (Cockpit_mode==CM_FULL_COCKPIT || Cockpit_mode==CM_STATUS_BAR)
		render_gauges();

	gr_set_current_canvas(&Screen_3d_window);
	if (!no_draw_hud)
		game_draw_hud_stuff();

	gr_set_current_canvas(NULL);

#ifdef NETWORK
        if (netplayerinfo_on)
		show_netplayerinfo();
#endif

	con_update();

	if (flip)
		gr_flip();
	else
		gr_update();
}

void toggle_cockpit()
{
	int new_mode=CM_FULL_SCREEN;

	if (Rear_view)
		return;

	switch (Cockpit_mode)
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
	write_player_file();
}

int last_drawn_cockpit = -1;
extern void ogl_loadbmtexture(grs_bitmap *bm);

// This actually renders the new cockpit onto the screen.
void update_cockpits(int force_redraw)
{
	//int x, y, w, h;

	grs_bitmap *bm;

	PIGGY_PAGE_IN(cockpit_bitmap[Cockpit_mode+(HIRESMODE?(Num_cockpits/2):0)]);
	bm=&GameBitmaps[cockpit_bitmap[Cockpit_mode+(HIRESMODE?(Num_cockpits/2):0)].index];

	//Redraw the on-screen cockpit bitmaps
	if (VR_render_mode != VR_NONE )	return;

	switch( Cockpit_mode )	{
		case CM_FULL_COCKPIT:
			gr_set_current_canvas(NULL);
			bm->bm_flags |= BM_FLAG_COCKPIT_TRANSPARENT;
#ifdef OGL
			ogl_ubitmapm_cs (0, 0, -1, -1, bm, 255, F1_0);
#else
			gr_ubitmapm(0,0, bm);
#endif
			break;
		case CM_REAR_VIEW:
			gr_set_current_canvas(NULL);
#ifdef OGL
			ogl_ubitmapm_cs (0, 0, -1, -1, bm, 255, F1_0);
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

	if (Cockpit_mode != last_drawn_cockpit || force_redraw )
		last_drawn_cockpit = Cockpit_mode;
	else
		return;

	if (Cockpit_mode==CM_FULL_COCKPIT || Cockpit_mode==CM_STATUS_BAR)
		init_gauges();

}

void game_render_frame()
{
	set_screen_mode( SCREEN_GAME );

	play_homing_warning();

	if (VR_render_mode == VR_NONE )
		game_render_frame_mono(GameArg.DbgUseDoubleBuffer);

	// Make sure palette is faded in
	stop_time();
	gr_palette_fade_in( gr_palette, 32, 0 );
	start_time();

	FrameCount++;
}

extern int Color_0_31_0;

//draw a crosshair for the guided missile
void draw_guided_crosshair(void)
{
	int x,y,w,h;

	gr_setcolor(Color_0_31_0);

	w = grd_curcanv->cv_bitmap.bm_w>>5;
	if (w < 5)
		w = 5;

	h = i2f(w) / grd_curscreen->sc_aspect;

	x = grd_curcanv->cv_bitmap.bm_w / 2;
	y = grd_curcanv->cv_bitmap.bm_h / 2;

	gr_uline(i2f(x-1),i2f(y-h/2),i2f(x-1),i2f(y+h/2));
	gr_uline(i2f(x-w/2),i2f(y),i2f(x+w/2),i2f(y));
}

//show a message in a nice little box
void show_boxed_message(char *msg, int RenderFlag)
{
	int w,h,aw;
	int x,y;

	if (Function_mode==FMODE_GAME && RenderFlag)
		game_render_frame_mono(0);

	gr_set_current_canvas(NULL);
	gr_set_curfont( MEDIUM1_FONT );
	gr_set_fontcolor( BM_XRGB(31, 31, 31), -1 );
	gr_get_string_size(msg,&w,&h,&aw);

	x = (SWIDTH-w)/2;
	y = (SHEIGHT-h)/2;

	nm_draw_background(x-BORDERX,y-BORDERY,x+w+BORDERX,y+h+BORDERY);

	gr_printf( 0x8000, y, msg );
        gr_update();
#ifdef OGL
	gr_flip();
#endif
}
