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

#ifdef OGL
#include "ogl_init.h"
#endif

extern fix Cruise_speed;
extern int LinearSVGABuffer;
extern int Current_display_mode;
extern cvar_t r_framerate;


#ifndef NDEBUG
extern int Debug_pause;				//John's debugging pause system
#endif

#ifndef RELEASE
extern int Saving_movie_frames;
#else
#define Saving_movie_frames 0
#endif

extern void newmenu_close();

// Returns the length of the first 'n' characters of a string.
int string_width( char * s, int n )
{
	int w,h,aw;
	char p;
	p = s[n];
	s[n] = 0;
	gr_get_string_size( s, &w, &h, &aw );
	s[n] = p;
	return w;
}

// Draw string 's' centered on a canvas... if wider than
// canvas, then wrap it.
void draw_centered_text( int y, char * s )
{
	int i, l;
	char p;

	l = strlen(s);

	if ( string_width( s, l ) < grd_curcanv->cv_bitmap.bm_w )	{
		gr_string( 0x8000, y, s );
		return;
	}

	for (i=0; i<l; i++ )	{
		if ( string_width(s,i) > (grd_curcanv->cv_bitmap.bm_w - 16) )	{
			p = s[i];
			s[i] = 0;
			gr_string( 0x8000, y, s );
			s[i] = p;
			gr_string( 0x8000, y+grd_curcanv->cv_font->ft_h+1, &s[i] );
			return;
		}
	}
}

extern ubyte DefiningMarkerMessage;
extern char Marker_input[];

#define MAX_MARKER_MESSAGE_LEN 120
void game_draw_marker_message()
{
	char temp_string[MAX_MARKER_MESSAGE_LEN+25];

        if ( DefiningMarkerMessage)
          {
                gr_set_curfont( GAME_FONT );    //GAME_FONT 
		gr_set_fontcolor(gr_getcolor(0,63,0), -1 );
                sprintf( temp_string, "Marker: %s_", Marker_input );
		draw_centered_text(grd_curcanv->cv_bitmap.bm_h/2-16, temp_string );
          }

}

#ifdef NETWORK
void game_draw_multi_message()
{
	char temp_string[MAX_MULTI_MESSAGE_LEN+25];

	if ( (Game_mode&GM_MULTI) && (multi_sending_message))	{
		gr_set_curfont( GAME_FONT );    //GAME_FONT );
		gr_set_fontcolor(gr_getcolor(0,63,0), -1 );
		sprintf( temp_string, "%s: %s_", TXT_MESSAGE, Network_message );
		draw_centered_text(grd_curcanv->cv_bitmap.bm_h/2-(16*SHEIGHT/200), temp_string );

	}

	if ( (Game_mode&GM_MULTI) && (multi_defining_message))	{
		gr_set_curfont( GAME_FONT );    //GAME_FONT );
		gr_set_fontcolor(gr_getcolor(0,63,0), -1 );
		sprintf( temp_string, "%s #%d: %s_", TXT_MACRO, multi_defining_message, Network_message );
		draw_centered_text(grd_curcanv->cv_bitmap.bm_h/2-(16*SHEIGHT/200), temp_string );
	}
}
#endif

//these should be in gr.h 
#define cv_w  cv_bitmap.bm_w
#define cv_h  cv_bitmap.bm_h

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
	sprintf( string, "%d.%02d", decimal, fractional );
}

void show_framerate()
{
	char temp[50];
	fix rate;
	int x = 8, y = 5;

	frame_time_total += FrameTime - frame_time_list[frame_time_cntr];
	frame_time_list[frame_time_cntr] = FrameTime;
	frame_time_cntr = (frame_time_cntr+1)%8;

	if (frame_time_total) {
		rate = fixdiv(f1_0*8,frame_time_total);

		gr_set_curfont( GAME_FONT );
		gr_set_fontcolor(gr_getcolor(0,31,0),-1 );

		ftoa( temp, rate );
                gr_printf(grd_curcanv->cv_w-FONTSCALE_X(x*GAME_FONT->ft_w),grd_curcanv->cv_h-y*FONTSCALE_Y(GAME_FONT->ft_h+GAME_FONT->ft_h/4),"FPS: %s ", temp );
	}
}

#ifndef NDEBUG

fix Show_view_text_timer = -1;

void draw_window_label()
{
	if ( Show_view_text_timer > 0 )
	{
		char *viewer_name,*control_name;
		char	*viewer_id;
		Show_view_text_timer -= FrameTime;
		gr_set_curfont( GAME_FONT );

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

		gr_set_fontcolor( gr_getcolor(31, 0, 0), -1 );
		gr_printf( 0x8000, 45, "%i: %s [%s] View - %s",Viewer-Objects, viewer_name, viewer_id, control_name );

	}
}
#endif

void render_countdown_gauge()
{
	if (!Endlevel_sequence && Control_center_destroyed  && (Countdown_seconds_left>-1)) { // && (Countdown_seconds_left<127))	{
		int	y;

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

		gr_set_curfont( SMALL_FONT );
		gr_set_fontcolor(gr_getcolor(0,63,0), -1 );
		y = FONTSCALE_Y(SMALL_FONT->ft_h*4);
		if (Cockpit_mode == CM_FULL_SCREEN)
			y += FONTSCALE_Y(SMALL_FONT->ft_h*2);

		if (Player_is_dead)
			y += FONTSCALE_Y(SMALL_FONT->ft_h*2);

		//if (!((Cockpit_mode == CM_STATUS_BAR) && (Game_window_y >= 19)))
		//	y += 5;
		gr_printf(0x8000, y, "T-%d s", Countdown_seconds_left );
	}
}

void game_draw_hud_stuff()
{
	//mprintf ((0,"Linear is %d!\n",LinearSVGABuffer));
	
	#ifndef NDEBUG
	if (Debug_pause) {
		gr_set_curfont( MEDIUM1_FONT );
		gr_set_fontcolor( gr_getcolor(31, 31, 31), -1 ); // gr_getcolor(31,0,0));
		gr_ustring( 0x8000, 85/2, "Debug Pause - Press P to exit" );
	}
	#endif

	#ifndef NDEBUG
	draw_window_label();
	#endif

#ifdef NETWORK
	game_draw_multi_message();
#endif

   game_draw_marker_message();

//   if (Game_mode & GM_MULTI)
//    {
//     if (Netgame.PlayTimeAllowed)
//       game_draw_time_left ();
//  }

	if ((Newdemo_state == ND_STATE_PLAYBACK) || (Newdemo_state == ND_STATE_RECORDING)) {
		char message[128];
		int h,w,aw,y;

		if (Newdemo_state == ND_STATE_PLAYBACK) {
			if (Newdemo_vcr_state != ND_STATE_PRINTSCREEN) {
			  	sprintf(message, "%s (%d%%%% %s)", TXT_DEMO_PLAYBACK, newdemo_get_percent_done(), TXT_DONE);
			} else {
				sprintf (message, " ");
			}
		} else 
			sprintf (message, TXT_DEMO_RECORDING);

		gr_set_curfont( GAME_FONT );    //GAME_FONT );
		gr_set_fontcolor(gr_getcolor(27,0,0), -1 );

		gr_get_string_size(message, &w, &h, &aw );

		y = grd_curcanv->cv_bitmap.bm_h;

		if (Cockpit_mode == CM_FULL_COCKPIT)
			y = grd_curcanv->cv_bitmap.bm_h / 1.15;
		if (Cockpit_mode != CM_REAR_VIEW)
			gr_printf((grd_curcanv->cv_bitmap.bm_w-w)/2,y - h - 2, message );
	}

	render_countdown_gauge();

	if ( Player_num > -1 && Viewer->type==OBJ_PLAYER && Viewer->id==Player_num )	{
		int	x = 3;
		int	y = grd_curcanv->cv_bitmap.bm_h;

		gr_set_curfont( GAME_FONT );
		gr_set_fontcolor( gr_getcolor(0, 31, 0), -1 );
		if (Cruise_speed > 0) {
			int line_spacing = FONTSCALE_Y(GAME_FONT->ft_h + GAME_FONT->ft_h/4);

mprintf((0,"line_spacing=%d ",line_spacing));

			if (Cockpit_mode==CM_FULL_SCREEN) {
				if (Game_mode & GM_MULTI)
					y -= line_spacing * 11;	//64
				else
					y -= line_spacing * 6;	//32
			} else if (Cockpit_mode == CM_STATUS_BAR) {
				if (Game_mode & GM_MULTI)
					y -= line_spacing * 8;	//48
				else
					y -= line_spacing * 4;	//24
			} else {
				y = line_spacing * 2;	//12
				x = FONTSCALE_X(22);
			}

			gr_printf( x, y, "%s %2d%%", TXT_CRUISE, f2i(Cruise_speed) );
		}
	}

//	if (r_framerate.value) // ZICO - wtf?
	if (GameArg.SysFPSIndicator) // ZICO - should be better
		show_framerate();

	if ( (Newdemo_state == ND_STATE_PLAYBACK) )
		Game_mode = Newdemo_game_mode;

	draw_hud();

	if ( (Newdemo_state == ND_STATE_PLAYBACK) )
		Game_mode = GM_NORMAL;

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

char DemoWBUType[]={0,WBU_MISSILE,WBU_MISSILE,WBU_REAR,WBU_ESCORT,WBU_MARKER,WBU_MISSILE};
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
   	   do_cockpit_window_view(1,&DemoRightExtra,DemoRearCheck[DemoDoRight],DemoWBUType[DemoDoRight],DemoExtraMessage[DemoDoRight]);
		} 
     else
    	do_cockpit_window_view(1,NULL,0,WBU_WEAPON,NULL);
	  
      DemoDoLeft=DemoDoRight=0;
		DemoDoingLeft=DemoDoingRight=0;
    
   	return;
    } 

	if (Guided_missile[Player_num] && Guided_missile[Player_num]->type==OBJ_WEAPON && Guided_missile[Player_num]->id==GUIDEDMISS_ID && Guided_missile[Player_num]->signature==Guided_missile_sig[Player_num]) {
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

		if (Missile_viewer) {		//do missile view
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
		char *msg = "Guided Missile View";
		object *viewer_save = Viewer;
		int w,h,aw;

      if (Cockpit_mode==CM_FULL_COCKPIT)
			{
			 BigWindowSwitch=1;
			 force_cockpit_redraw=1;
			 Cockpit_mode=CM_STATUS_BAR;
			 return;
		   }
  
		Viewer = Guided_missile[Player_num];

  		{
			update_rendered_data(0, Viewer, 0, 0);
			render_frame(0, 0);
  
			wake_up_rendered_objects(Viewer, 0);
			Viewer = viewer_save;

			gr_set_curfont( GAME_FONT );    //GAME_FONT );
			gr_set_fontcolor(gr_getcolor(27,0,0), -1 );
			gr_get_string_size(msg, &w, &h, &aw );

			gr_printf((grd_curcanv->cv_bitmap.bm_w-w)/2, 3, msg );

			draw_guided_crosshair();
		}

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

	if (!no_draw_hud)
		game_draw_hud_stuff();

	if (Game_paused) {		//render pause message over off-screen 3d (to minimize flicker)
		extern char *Pause_msg;

		show_boxed_message(Pause_msg);
	}

	update_cockpits(0);

	show_extra_views();		//missile view, buddy bot, etc.

	if (Cockpit_mode==CM_FULL_COCKPIT || Cockpit_mode==CM_STATUS_BAR) {

		if ( (Newdemo_state == ND_STATE_PLAYBACK) )
			Game_mode = Newdemo_game_mode;

		render_gauges();

		if ( (Newdemo_state == ND_STATE_PLAYBACK) )
			Game_mode = GM_NORMAL;
	}

	con_update();

	if (flip)
		gr_flip();
	else
		gr_update();
}

void toggle_cockpit()
{
	int new_mode=CM_FULL_SCREEN;

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

	PIGGY_PAGE_IN(cockpit_bitmap[Cockpit_mode+(Current_display_mode?(Num_cockpits/2):0)]);
	bm=&GameBitmaps[cockpit_bitmap[Cockpit_mode+(Current_display_mode?(Num_cockpits/2):0)].index];

	//Redraw the on-screen cockpit bitmaps
	if (VR_render_mode != VR_NONE )	return;

	switch( Cockpit_mode )	{
		case CM_FULL_COCKPIT:
		case CM_REAR_VIEW:
			gr_set_current_canvas(NULL);
			bm->bm_flags |= BM_FLAG_COCKPIT_TRANSPARENT;
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
			bm->bm_flags |= BM_FLAG_TRANSPARENT;
			bm->bm_flags |= BM_FLAG_COCKPIT_TRANSPARENT;
			ogl_ubitmapm_cs (0, (Current_display_mode?(SHEIGHT*2)/2.6:(SHEIGHT*2)/2.72), -1, ((int) ((double) (bm->bm_h) * (Current_display_mode?(double)grd_curscreen->sc_h/480:(double)grd_curscreen->sc_h/200) + 0.5)), bm,255, F1_0);
#else
			gr_ubitmapm(0,grd_curscreen->sc_h-bm->bm_h,bm);
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
		game_render_frame_mono(Game_double_buffer);	 

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

	w = grd_curcanv->cv_w>>5;
	if (w < 5)
		w = 5;

	h = i2f(w) / grd_curscreen->sc_aspect;

	x = grd_curcanv->cv_w / 2;
	y = grd_curcanv->cv_h / 2;

	gr_scanline(x-w/2,x+w/2,y);
	gr_uline(i2f(x),i2f(y-h/2),i2f(x),i2f(y+h/2));
	gr_uline(i2f(x-w/2),i2f(y),i2f(x+w/2),i2f(y));
}

typedef struct bkg {
	short x, y, w, h;			// The location of the menu.
	grs_bitmap * bmp;			// The background under the menu.
} bkg;

bkg bg = {0,0,0,0,NULL};

//show a message in a nice little box
void show_boxed_message(char *msg)
{
	int w,h,aw;
	int x,y;

	gr_set_current_canvas(NULL);
	gr_set_curfont( MEDIUM1_FONT );

	gr_get_string_size(msg,&w,&h,&aw);

	x = (grd_curscreen->sc_w-w)/2;
	y = (grd_curscreen->sc_h-h)/2;

	nm_draw_background(x-(15*(SWIDTH/320)),y-(15*(SHEIGHT/200)),x+w+(15*(SWIDTH/320))-1,y+h+(15*(SHEIGHT/200))-1);

	gr_set_fontcolor( gr_getcolor(31, 31, 31), -1 );
	gr_ustring( 0x8000, y, msg );
        gr_update();
#ifdef OGL
	gr_flip();
#endif
}

void clear_boxed_message()
{
	if (bg.bmp) {

		gr_bitmap(bg.x-(15*(SWIDTH/320)), bg.y-(15*(SHEIGHT/200)), bg.bmp);
		gr_free_bitmap(bg.bmp);
		bg.bmp = NULL;
	}
	newmenu_close();
}
